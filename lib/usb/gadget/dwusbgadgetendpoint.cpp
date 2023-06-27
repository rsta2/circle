//
// dwusbgadgetendpoint.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/gadget/dwusbgadgetendpoint.h>
#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/dwhciregister.h>
#include <circle/logger.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

LOGMODULE ("dwgadgetep");

u8 CDWUSBGadgetEndpoint::s_NextEPSeq[DWHCI_MAX_EPS_CHANNELS];
u8 CDWUSBGadgetEndpoint::s_uchFirstInNextEPSeq;

CDWUSBGadgetEndpoint::CDWUSBGadgetEndpoint (size_t nMaxPacketSize, CDWUSBGadget *pGadget)
:	m_pGadget (pGadget),
	m_Direction (DirectionInOut),
	m_Type (TypeControl),
	m_nEP (0),
	m_nMaxPacketSize (nMaxPacketSize)
{
	InitTransfer ();

	assert (m_pGadget);
	m_pGadget->AssignEndpoint (m_nEP, this);
}

CDWUSBGadgetEndpoint::CDWUSBGadgetEndpoint (const TUSBEndpointDescriptor *pDesc,
					    CDWUSBGadget *pGadget)
:	m_pGadget (pGadget),
	m_Direction (pDesc->bEndpointAddress & 0x80 ? DirectionIn : DirectionOut),
	m_Type (TypeBulk),
	m_nEP (pDesc->bEndpointAddress & 0xF),
	m_nMaxPacketSize (pDesc->wMaxPacketSize & 0x7FF)
{
	assert ((pDesc->bmAttributes & 0x03) == 2);	// Bulk

	InitTransfer ();

	assert (m_pGadget);
	m_pGadget->AssignEndpoint (m_nEP, this);
}

CDWUSBGadgetEndpoint::~CDWUSBGadgetEndpoint (void)
{
	assert (m_pGadget);
	m_pGadget->RemoveEndpoint (m_nEP);
}

void CDWUSBGadgetEndpoint::OnUSBReset (void)
{
	InitTransfer ();

	if (!m_nEP)
	{
		u32 nValue = 0;
		switch (m_nMaxPacketSize)
		{
		case 8:		nValue = DWHCI_DEV_EP0_CTRL_MAX_PACKET_SIZ_8;	break;
		case 16:	nValue = DWHCI_DEV_EP0_CTRL_MAX_PACKET_SIZ_16;	break;
		case 32:	nValue = DWHCI_DEV_EP0_CTRL_MAX_PACKET_SIZ_32;	break;
		case 64:	nValue = DWHCI_DEV_EP0_CTRL_MAX_PACKET_SIZ_64;	break;

		default:
			assert (0);
			break;
		}

		CDWHCIRegister InEP0Ctrl (DWHCI_DEV_IN_EP_CTRL (0));
		InEP0Ctrl.Read ();
		InEP0Ctrl.And (~DWHCI_DEV_EP0_CTRL_MAX_PACKET_SIZ__MASK);
		InEP0Ctrl.Or (nValue << DWHCI_DEV_EP_CTRL_MAX_PACKET_SIZ__SHIFT);
		InEP0Ctrl.Write ();

		CDWHCIRegister OutEP0Ctrl (DWHCI_DEV_OUT_EP_CTRL (0));
		OutEP0Ctrl.Read ();
		OutEP0Ctrl.And (~DWHCI_DEV_EP0_CTRL_MAX_PACKET_SIZ__MASK);
		OutEP0Ctrl.Or (nValue << DWHCI_DEV_EP_CTRL_MAX_PACKET_SIZ__SHIFT);
		OutEP0Ctrl.Write ();
	}
	else
	{
		// dwc_otg_ep_activate()
		CDWHCIRegister EPCtrl (m_Direction == DirectionIn ? DWHCI_DEV_IN_EP_CTRL (m_nEP)
								  : DWHCI_DEV_OUT_EP_CTRL (m_nEP));
		EPCtrl.Read ();

		assert (!(EPCtrl.Get () & DWHCI_DEV_EP_CTRL_ACTIVE_EP));

		EPCtrl.And (~DWHCI_DEV_EP_CTRL_MAX_PACKET_SIZ__MASK);
		EPCtrl.Or (m_nMaxPacketSize << DWHCI_DEV_EP_CTRL_MAX_PACKET_SIZ__SHIFT);

		assert (m_Type == TypeBulk);	// TODO: Bulk only
		EPCtrl.And (~DWHCI_DEV_EP_CTRL_EP_TYPE__MASK);
		EPCtrl.Or (DWHCI_DEV_EP_CTRL_EP_TYPE_BULK << DWHCI_DEV_EP_CTRL_EP_TYPE__SHIFT);
		EPCtrl.Or (DWHCI_DEV_EP_CTRL_SETDPID_D0);

		EPCtrl.Or (DWHCI_DEV_EP_CTRL_ACTIVE_EP);

		if (m_Direction == DirectionIn)
		{
			// Assign dedicated TX FIFO to EP
			EPCtrl.And (~DWHCI_DEV_IN_EP_CTRL_TX_FIFO_NUM__MASK);
			EPCtrl.Or (m_nEP << DWHCI_DEV_IN_EP_CTRL_TX_FIFO_NUM__SHIFT);

			// Update s_NextEPSeq[]
			unsigned i;
			for (i = 0; i <= CDWUSBGadget::NumberOfInEPs; i++)
			{
				if (s_NextEPSeq[i] == s_uchFirstInNextEPSeq)
				{
					break;
				}
			}

			assert (i <= CDWUSBGadget::NumberOfInEPs);
			s_NextEPSeq[i] = m_nEP;
			s_NextEPSeq[m_nEP] = s_uchFirstInNextEPSeq;

			EPCtrl.And (~DWHCI_DEV_IN_EP_CTRL_NEXT_EP__MASK);
			EPCtrl.Or (s_NextEPSeq[m_nEP] << DWHCI_DEV_IN_EP_CTRL_NEXT_EP__SHIFT);

#ifdef USB_GADGET_DEBUG
			LOGDBG ("First in next EP sequence is %u", s_uchFirstInNextEPSeq);
			debug_hexdump (s_NextEPSeq, sizeof s_NextEPSeq, From);
#endif

			// Update EP mismatch count
			CDWHCIRegister DeviceConfig (DWHCI_DEV_CFG);
			DeviceConfig.Read ();
			u32 nCount =    (DeviceConfig.Get () & DWHCI_DEV_CFG_EP_MISMATCH_COUNT__MASK)
				     >> DWHCI_DEV_CFG_EP_MISMATCH_COUNT__SHIFT;
			nCount++;
			DeviceConfig.And (~DWHCI_DEV_CFG_EP_MISMATCH_COUNT__MASK);
			DeviceConfig.Or (nCount << DWHCI_DEV_CFG_EP_MISMATCH_COUNT__SHIFT);
			DeviceConfig.Write ();
		}

		EPCtrl.Write ();
	}

	// Enable EP interrupt
	CDWHCIRegister AllEPsIntMask (DWHCI_DEV_ALL_EPS_INT_MASK);
	AllEPsIntMask.Read ();
	switch (m_Direction)
	{
	case DirectionInOut:
		AllEPsIntMask.Or (DWHCI_DEV_ALL_EPS_INT_MASK_IN_EP (m_nEP));
		AllEPsIntMask.Or (DWHCI_DEV_ALL_EPS_INT_MASK_OUT_EP (m_nEP));
		break;

	case DirectionOut:
		AllEPsIntMask.Or (DWHCI_DEV_ALL_EPS_INT_MASK_OUT_EP (m_nEP));
		break;

	case DirectionIn:
		AllEPsIntMask.Or (DWHCI_DEV_ALL_EPS_INT_MASK_IN_EP (m_nEP));
		break;
	}
	AllEPsIntMask.Write ();
}

void CDWUSBGadgetEndpoint::BeginTransfer (TTransferMode Mode, void *pBuffer, size_t nLength)
{
	assert (Mode < TransferUnknown);
	assert (m_TransferMode == TransferUnknown);
	m_TransferMode = Mode;

	assert (pBuffer || !nLength);
	assert (!m_pTransferBuffer);
	m_pTransferBuffer = pBuffer;

	assert (m_nTransferLength == (size_t) -1);
	m_nTransferLength = nLength;

	unsigned nPacketCount = 1;
	if (nLength)
	{
		nPacketCount = (nLength + m_nMaxPacketSize-1) / m_nMaxPacketSize;

		// EP0 limits for transfer size and packet count
		assert (m_nEP || nLength <= 0x7F);
		assert (m_nEP || nPacketCount <= 3);

		CleanAndInvalidateDataCacheRange ((uintptr) pBuffer, nLength);
	}
	else
	{
		pBuffer = m_DummyBuffer;
	}

	if (Mode == TransferDataIn)
	{
		CDWHCIRegister InEPXferSize (DWHCI_DEV_IN_EP_XFER_SIZ (m_nEP), 0);
		InEPXferSize.Or (nPacketCount << DWHCI_DEV_EP_XFER_SIZ_PKT_CNT__SHIFT);
		InEPXferSize.Or (nLength << DWHCI_DEV_EP_XFER_SIZ_XFER_SIZ__SHIFT);
		InEPXferSize.Write ();

		CDWHCIRegister InEPDMAAddress (DWHCI_DEV_IN_EP_DMA_ADDR (m_nEP),
					       BUS_ADDRESS ((uintptr) pBuffer));
		InEPDMAAddress.Write ();

		CDWHCIRegister InEPCtrl (DWHCI_DEV_IN_EP_CTRL (m_nEP), 0);
		InEPCtrl.Read ();
		InEPCtrl.And (~DWHCI_DEV_IN_EP_CTRL_NEXT_EP__MASK);
		InEPCtrl.Or (s_NextEPSeq[m_nEP] << DWHCI_DEV_IN_EP_CTRL_NEXT_EP__SHIFT);
		InEPCtrl.Or (DWHCI_DEV_EP_CTRL_EP_ENABLE);
		InEPCtrl.Or (DWHCI_DEV_EP_CTRL_CLEAR_NAK);
		InEPCtrl.Write ();
	}
	else
	{
		CDWHCIRegister OutEPXferSize (DWHCI_DEV_OUT_EP_XFER_SIZ (m_nEP), 0);

		if (Mode == TransferSetupOut)
		{
			assert (m_nEP == 0);
			assert (nLength == sizeof (TSetupData));

			OutEPXferSize.Or (   nPacketCount
					  << DWHCI_DEV_EP0_XFER_SIZ_SETUP_PKT_CNT__SHIFT);
		}

		OutEPXferSize.Or (nPacketCount << DWHCI_DEV_EP_XFER_SIZ_PKT_CNT__SHIFT);
		OutEPXferSize.Or (nLength << DWHCI_DEV_EP_XFER_SIZ_XFER_SIZ__SHIFT);
		OutEPXferSize.Write ();

		CDWHCIRegister OutEPDMAAddress (DWHCI_DEV_OUT_EP_DMA_ADDR (m_nEP),
						BUS_ADDRESS ((uintptr) pBuffer));
		OutEPDMAAddress.Write ();

		CDWHCIRegister OutEPCtrl (DWHCI_DEV_OUT_EP_CTRL (m_nEP));
		OutEPCtrl.Read ();
		OutEPCtrl.Or (DWHCI_DEV_EP_CTRL_EP_ENABLE);
		OutEPCtrl.Or (DWHCI_DEV_EP_CTRL_CLEAR_NAK);
		OutEPCtrl.Write ();
	}
}

size_t CDWUSBGadgetEndpoint::FinishTransfer (void)
{
	assert (m_TransferMode < TransferUnknown);
	CDWHCIRegister EPXferSize (  m_TransferMode == TransferDataIn
				   ? DWHCI_DEV_IN_EP_XFER_SIZ (m_nEP)
				   : DWHCI_DEV_OUT_EP_XFER_SIZ (m_nEP));
	size_t nXferSize = EPXferSize.Read ();

	nXferSize &= !m_nEP ? DWHCI_DEV_EP0_XFER_SIZ_XFER_SIZ__MASK
			    : DWHCI_DEV_EP_XFER_SIZ_XFER_SIZ__MASK;
	nXferSize >>= DWHCI_DEV_EP_XFER_SIZ_XFER_SIZ__SHIFT;

	assert (nXferSize <= m_nTransferLength);
	nXferSize = m_nTransferLength - nXferSize;

#ifdef USB_GADGET_DEBUG
	LOGDBG ("%u byte(s) transferred", nXferSize);

	if (nXferSize)
	{
		debug_hexdump (m_pTransferBuffer, nXferSize, From);
	}
#endif

	InitTransfer ();

	return nXferSize;
}

void CDWUSBGadgetEndpoint::InitTransfer (void)
{
	m_TransferMode = TransferUnknown;
	m_pTransferBuffer = nullptr;
	m_nTransferLength = -1;
}

void CDWUSBGadgetEndpoint::Stall (boolean bIn)
{
	LOGWARN ("EP%u: Send STALL response", m_nEP);

	CDWHCIRegister EPCtrl (bIn ? DWHCI_DEV_IN_EP_CTRL (m_nEP)
				   : DWHCI_DEV_OUT_EP_CTRL (m_nEP));
	EPCtrl.Read ();
	EPCtrl.Or (DWHCI_DEV_EP_CTRL_STALL);
	EPCtrl.Or (DWHCI_DEV_EP_CTRL_CLEAR_NAK);
	EPCtrl.Write ();
}

void CDWUSBGadgetEndpoint::OnControlMessage (void)
{
	assert (0);
}

void CDWUSBGadgetEndpoint::HandleOutInterrupt (void)
{
	CDWHCIRegister OutEPCommonIntMask (DWHCI_DEV_OUT_EP_COMMON_INT_MASK);
	CDWHCIRegister OutEPInt (DWHCI_DEV_OUT_EP_INT (m_nEP));
	OutEPCommonIntMask.Read ();
	OutEPInt.Read ();
	OutEPInt.And (OutEPCommonIntMask.Get ());

#ifdef USB_GADGET_DEBUG
	LOGDBG ("EP%u: Interrupt status is 0x%08X", m_nEP, OutEPInt.Get ());
#endif

	if (OutEPInt.Get () & DWHCI_DEV_OUT_EP_INT_SETUP_DONE)
	{
		assert (m_nEP == 0);
		CDWHCIRegister OutEPIntAck (DWHCI_DEV_OUT_EP_INT (m_nEP));
		OutEPIntAck.Set (DWHCI_DEV_OUT_EP_INT_SETUP_DONE);
		OutEPIntAck.Write ();

#ifndef NDEBUG
		size_t nLength =
#endif
			FinishTransfer ();
		assert (nLength == sizeof (TSetupData));

		OnControlMessage ();

		OutEPInt.And (~DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE);
	}

	if (OutEPInt.Get () & DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE)
	{
		CDWHCIRegister OutEPIntAck (DWHCI_DEV_OUT_EP_INT (m_nEP));
		OutEPIntAck.Set (DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE);
		OutEPIntAck.Write ();

		size_t nLength = FinishTransfer ();

		OnTransferComplete (FALSE, nLength);
	}

	if (OutEPInt.Get () & DWHCI_DEV_OUT_EP_INT_AHB_ERROR)
	{
		LOGPANIC ("AHB error");
	}

	if (OutEPInt.Get () & DWHCI_DEV_OUT_EP_INT_EP_DISABLED)
	{
		LOGPANIC ("EP%u disabled", m_nEP);
	}
}

void CDWUSBGadgetEndpoint::HandleInInterrupt (void)
{
	CDWHCIRegister InEPCommonIntMask (DWHCI_DEV_IN_EP_COMMON_INT_MASK);
	CDWHCIRegister InEPInt (DWHCI_DEV_IN_EP_INT (m_nEP));
	InEPCommonIntMask.Read ();
	InEPInt.Read ();
	InEPInt.And (InEPCommonIntMask.Get ());

#ifdef USB_GADGET_DEBUG
	LOGDBG ("EP%u: Interrupt status is 0x%08X", m_nEP, InEPInt.Get ());
#endif

	if (InEPInt.Get () & DWHCI_DEV_IN_EP_INT_XFER_COMPLETE)
	{
		CDWHCIRegister InEPIntAck (DWHCI_DEV_IN_EP_INT (m_nEP));
		InEPIntAck.Set (DWHCI_DEV_IN_EP_INT_XFER_COMPLETE);
		InEPIntAck.Write ();

		size_t nLength = FinishTransfer ();

		OnTransferComplete (TRUE, nLength);
	}

	if (InEPInt.Get () & DWHCI_DEV_IN_EP_INT_TIMEOUT)
	{
		LOGPANIC ("Timeout (EP %u)", m_nEP);
	}

	if (InEPInt.Get () & DWHCI_DEV_IN_EP_INT_AHB_ERROR)
	{
		LOGPANIC ("AHB error");
	}

	if (InEPInt.Get () & DWHCI_DEV_IN_EP_INT_EP_DISABLED)
	{
		LOGPANIC ("EP%u disabled", m_nEP);
	}
}

void CDWUSBGadgetEndpoint::HandleUSBReset (void)
{
	memset (s_NextEPSeq, 0xFF, sizeof s_NextEPSeq);
	s_NextEPSeq[0] = 0;
	s_uchFirstInNextEPSeq = 0;
}
