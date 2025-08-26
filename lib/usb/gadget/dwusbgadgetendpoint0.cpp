//
// dwusbgadgetendpoint0.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023-2025  R. Stange <rsta2@gmx.net>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <circle/usb/gadget/dwusbgadgetendpoint0.h>
#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/dwhci.h>
#include <circle/util.h>
#include <assert.h>

CDWUSBGadgetEndpoint0::CDWUSBGadgetEndpoint0 (size_t nMaxPacketSize, CDWUSBGadget *pGadget)
:	CDWUSBGadgetEndpoint (nMaxPacketSize, pGadget),
	m_State (StateDisconnect)
{
}

CDWUSBGadgetEndpoint0::~CDWUSBGadgetEndpoint0 (void)
{
}

void CDWUSBGadgetEndpoint0::OnActivate (void)
{
	m_State = StateIdle;

	BeginTransfer (TransferSetupOut, m_OutBuffer, sizeof (TSetupData));
}

void CDWUSBGadgetEndpoint0::OnDeactivate (void)
{
	m_State = StateDisconnect;

	CancelTransfer ();
}

void CDWUSBGadgetEndpoint0::OnControlMessage (void)
{
	assert (m_pGadget);

	const TSetupData *pSetupData = reinterpret_cast<TSetupData *> (m_OutBuffer);

	// copy setup packet to be used in OnTransferComplete()
	memcpy (&m_SetupData, pSetupData, sizeof m_SetupData);

	if (pSetupData->bmRequestType & REQUEST_IN)
	{
		switch (pSetupData->bRequest)
		{
		case GET_DESCRIPTOR: {
			size_t nLength;
			const void *pDesc = m_pGadget->GetDescriptor (pSetupData->wValue,
								      pSetupData->wIndex,
								      &nLength);
			if (!pDesc)
			{
				Stall (TRUE);

				BeginTransfer (TransferSetupOut, m_OutBuffer, sizeof (TSetupData));

				return;
			}

			assert (nLength <= BufferSize);
			memcpy (m_InBuffer, pDesc, nLength);

			if (nLength > pSetupData->wLength)
			{
				nLength = pSetupData->wLength;
			}

			m_State = StateInDataPhase;

			// EP0 can transfer only up to 127 bytes at once. Therefore we split greater
			// descriptors into multiple transfers, with up to max. packet size each.
			m_nBytesLeft = nLength;
			m_pBufPtr = m_InBuffer;

			BeginTransfer (TransferDataIn, m_pBufPtr,
				         m_nBytesLeft <= m_nMaxPacketSize
				       ? m_nBytesLeft : m_nMaxPacketSize);
			} break;

		case GET_STATUS:
			if (pSetupData->wLength != 2)
			{
				Stall (TRUE);

				BeginTransfer (TransferSetupOut, m_OutBuffer, sizeof (TSetupData));

				return;
			}

			m_InBuffer[0] = 0;
			m_InBuffer[1] = 0;

			m_State = StateInDataPhase;

			m_nBytesLeft = 2;
			m_pBufPtr = m_InBuffer;
			BeginTransfer (TransferDataIn, m_pBufPtr, m_nBytesLeft);
			break;

		default:
			if (   pSetupData->bmRequestType & (REQUEST_CLASS | REQUEST_VENDOR)
			    && pSetupData->wLength > 0
			    && pSetupData->wLength <= BufferSize)
			{
				assert (m_pGadget);
				int nLen = m_pGadget->OnClassOrVendorRequest (pSetupData, m_InBuffer);
				if (nLen > 0)
				{
					m_State = StateInDataPhase;

					// EP0 can transfer only up to 127 bytes at once. Therefore
					// we split greater transfers into multiple transfers, with
					// up to max. packet size each.
					m_nBytesLeft = nLen;
					m_pBufPtr = m_InBuffer;

					BeginTransfer (TransferDataIn, m_pBufPtr,
							 m_nBytesLeft <= m_nMaxPacketSize
						       ? m_nBytesLeft : m_nMaxPacketSize);
					break;
				}
			}

			Stall (TRUE);
			BeginTransfer (TransferSetupOut, m_OutBuffer, sizeof (TSetupData));
			break;
		}
	}
	else
	{
		switch (pSetupData->bRequest)
		{
		case SET_ADDRESS:
			m_pGadget->SetDeviceAddress (pSetupData->wValue & 0xFF);

			m_State = StateInStatusPhase;

			BeginTransfer (TransferDataIn, nullptr, 0);
			break;

		case SET_CONFIGURATION:
			if (!m_pGadget->SetConfiguration (pSetupData->wValue & 0xFF))
			{
				Stall (TRUE);

				BeginTransfer (TransferSetupOut, m_OutBuffer, sizeof (TSetupData));

				return;
			}

			m_State = StateInStatusPhase;

			BeginTransfer (TransferDataIn, nullptr, 0);
			break;

		default:
			if (pSetupData->wLength)
			{
				m_State = StateOutDataPhase;

				// EP0 can transfer only up to 127 bytes at once. Therefore
				// we split greater transfers into multiple transfers, with
				// up to max. packet size each.
				assert (pSetupData->wLength <= sizeof m_OutBuffer);
				m_nBytesLeft = pSetupData->wLength;
				m_pBufPtr = m_OutBuffer;

				BeginTransfer (TransferDataOut, m_pBufPtr,
						 m_nBytesLeft <= m_nMaxPacketSize
					       ? m_nBytesLeft : m_nMaxPacketSize);
			}
			else
			{
				m_State = StateInStatusPhase;

				BeginTransfer (TransferDataIn, nullptr, 0);
			}
			break;
		}
	}
}

void CDWUSBGadgetEndpoint0::OnTransferComplete (boolean bIn, size_t nLength)
{
	switch (m_State)
	{
	case StateInDataPhase:
		assert (m_nBytesLeft >= nLength);
		m_nBytesLeft -= nLength;
		if (m_nBytesLeft)
		{
			m_pBufPtr += nLength;

			BeginTransfer (TransferDataIn, m_pBufPtr,
				         m_nBytesLeft <= m_nMaxPacketSize
				       ? m_nBytesLeft : m_nMaxPacketSize);

			break;
		}

		m_State = StateOutStatusPhase;
		BeginTransfer (TransferDataOut, nullptr, 0);
		break;

	case StateOutDataPhase:
		assert (m_nBytesLeft >= nLength);
		m_nBytesLeft -= nLength;
		if (m_nBytesLeft)
		{
			m_pBufPtr += nLength;

			BeginTransfer (TransferDataOut, m_pBufPtr,
				         m_nBytesLeft <= m_nMaxPacketSize
				       ? m_nBytesLeft : m_nMaxPacketSize);

			break;
		}

		m_State = StateInStatusPhase;
		BeginTransfer (TransferDataIn, nullptr, 0);
		break;

	case StateInStatusPhase:
		assert (m_pGadget);
		if (   m_SetupData.bmRequestType & (REQUEST_CLASS | REQUEST_VENDOR)
		    && m_pGadget->OnClassOrVendorRequest (&m_SetupData, m_OutBuffer) < 0)
		{
			Stall (TRUE);
		}
		// fall through

	case StateOutStatusPhase:
		m_State = StateIdle;
		BeginTransfer (TransferSetupOut, m_OutBuffer, sizeof (TSetupData));
		break;

	default:
		assert (0);
		break;
	}
}
