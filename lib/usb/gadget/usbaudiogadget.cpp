// SPDX-License-Identifier: GPL-3.0-or-later
//
// usbaudiogadget.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
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
#include <circle/usb/gadget/usbaudiogadget.h>
#include <circle/usb/gadget/usbstringdescriptor.h>
#include <circle/util.h>
#include <assert.h>

CUSBAudioGadgetEndpoint::CUSBAudioGadgetEndpoint (const TUSBEndpointDescriptor *pDescriptor,
	CDWUSBGadget *pGadget, CUSBAudioGadgetFunction *pFunction)
:	CDWUSBGadgetEndpoint (pDescriptor, pGadget), m_pFunction (pFunction),
	m_bActive (FALSE), m_nPacketSize (pDescriptor->wMaxPacketSize)
{
	assert (pFunction);
	assert (m_nPacketSize && m_nPacketSize <= CUSBAudioGadgetFunction::MaxPacketSize);
}

void CUSBAudioGadgetEndpoint::OnUSBReset (void)
{
	m_bActive = FALSE;
	m_pFunction->SetConfigured (FALSE);
	CDWUSBGadgetEndpoint::OnUSBReset ();
}

void CUSBAudioGadgetEndpoint::OnActivate (void)
{
	m_pFunction->SetConfigured (TRUE);
	// SET_CONFIGURATION selects the zero-bandwidth alternate.
}

boolean CUSBAudioGadgetEndpoint::SetInterface (u16 nInterface, u16 nAlternate)
{
	if (!m_pFunction->SetInterface (nInterface, nAlternate))
	{
		return FALSE;
	}
	if (nInterface != m_pFunction->GetStreamingInterface ())
	{
		return TRUE;
	}

	m_bActive = FALSE;
	if (IsTransferActive ())
	{
		CancelTransfer ();
	}
	m_nPacketSize = m_pFunction->GetAlternatePacketSize (nAlternate);
	m_bActive = m_nPacketSize != 0;
	if (m_bActive)
	{
		BeginTransfer (TransferDataOut, m_Buffer, m_nPacketSize);
	}
	return TRUE;
}

void CUSBAudioGadgetEndpoint::OnDeactivate (void)
{
	m_bActive = FALSE;
	m_pFunction->SetConfigured (FALSE);
	if (IsTransferActive ())
	{
		CancelTransfer ();
	}
}

void CUSBAudioGadgetEndpoint::OnSuspend (void)
{
	m_bActive = FALSE;
	m_pFunction->SetConfigured (FALSE);
}

void CUSBAudioGadgetEndpoint::OnTransferComplete (boolean bIn, size_t nLength)
{
	assert (!bIn);
	if (m_bActive)
	{
		m_pFunction->Receive (m_Buffer, nLength);
		BeginTransfer (TransferDataOut, m_Buffer, m_nPacketSize);
	}
}

CUSBAudioGadget::CUSBAudioGadget (CInterruptSystem *pInterruptSystem,
	const CUSBAudioGadgetFunction::TConfiguration &Config,
	CUSBAudioGadgetFunction::CSink *pSink, u16 usVendorID, u16 usProductID)
:	CDWUSBGadget (pInterruptSystem, FullSpeed), m_Function (Config, pSink),
	m_pEndpoint (nullptr),
	m_DeviceDescriptor {sizeof (TUSBDeviceDescriptor), DESCRIPTOR_DEVICE, 0x200,
		0, 0, 0, 64, usVendorID, usProductID, 0x100, 1, 2, 0, 1},
	m_EndpointDescriptor {sizeof (TUSBEndpointDescriptor), DESCRIPTOR_ENDPOINT,
		(u8) Config.nEndpoint, 9, (u16) m_Function.GetMaxPacketSize (), 1},
	m_nDescriptorSize (0)
{
	if (Config.nControlInterface != 0 || Config.nStreamingInterface != 1)
	{
		return;
	}
	unsigned nFunctionSize = m_Function.GetDescriptor (m_ConfigurationDescriptor+9,
						 sizeof m_ConfigurationDescriptor-9);
	if (!nFunctionSize)
	{
		return;
	}
	m_nDescriptorSize = nFunctionSize+9;
	u8 Header[] = {9, 2, (u8) m_nDescriptorSize, (u8) (m_nDescriptorSize>>8),
			2, 1, 0, 0x80, 250};
	memcpy (m_ConfigurationDescriptor, Header, sizeof Header);
}

CUSBAudioGadget::~CUSBAudioGadget (void)
{
	// Like other Circle gadgets, lifetime extends until shutdown.
	assert (!m_pEndpoint);
}

boolean CUSBAudioGadget::Initialize (boolean bScanDevices)
{
	return m_nDescriptorSize && CDWUSBGadget::Initialize (bScanDevices);
}

void CUSBAudioGadget::Process (void)
{
	m_Function.Process ();
}

void CUSBAudioGadget::GetStatistics (CUSBAudioGadgetFunction::TStatistics *pStatistics)
{
	m_Function.GetStatistics (pStatistics);
}

const void *CUSBAudioGadget::GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength)
{
	if (!pLength)
	{
		return nullptr;
	}
	unsigned nType = wValue >> 8;
	unsigned nIndex = wValue & 255;
	if (nType == DESCRIPTOR_DEVICE && !nIndex)
	{
		*pLength = sizeof m_DeviceDescriptor;
		return &m_DeviceDescriptor;
	}
	if (nType == DESCRIPTOR_CONFIGURATION && !nIndex && m_nDescriptorSize)
	{
		*pLength = m_nDescriptorSize;
		return m_ConfigurationDescriptor;
	}
	if (nType == DESCRIPTOR_STRING)
	{
		static const u8 Language[] = {4, 3, 9, 4};
		if (!nIndex)
		{
			*pLength = sizeof Language;
			return Language;
		}
		if (wIndex != 0x409 || nIndex > 2)
		{
			return nullptr;
		}
		const char *p = nIndex == 1 ? "Circle" : "UAC1 Playback";
		return USBStringToDescriptor (p, m_StringDescriptor,
					      sizeof m_StringDescriptor, pLength);
	}
	return nullptr;
}

boolean CUSBAudioGadget::SetInterface (u16 nInterface, u16 nAlternate)
{
	return m_pEndpoint && m_pEndpoint->SetInterface (nInterface, nAlternate);
}

int CUSBAudioGadget::GetInterface (u16 nInterface)
{
	return m_Function.GetInterface (nInterface);
}

int CUSBAudioGadget::OnClassOrVendorRequest (const TSetupData *pSetup, u8 *pData)
{
	return m_Function.ControlRequest (pSetup, pData);
}

void CUSBAudioGadget::AddEndpoints (void)
{
	assert (!m_pEndpoint);
	m_pEndpoint = new CUSBAudioGadgetEndpoint (&m_EndpointDescriptor, this, &m_Function);
}

void CUSBAudioGadget::CreateDevice (void)
{
	// The application-owned sink is stable across USB configurations.
}

void CUSBAudioGadget::OnSuspend (void)
{
	m_Function.SetConfigured (FALSE);
	delete m_pEndpoint;
	m_pEndpoint = nullptr;
}

boolean CUSBAudioGadget::RetainTransfersOnSuspend (void) const
{
	return TRUE;
}

void CUSBAudioGadget::OnBusSuspend (void)
{
	m_Function.SetSuspended (TRUE);
}

void CUSBAudioGadget::OnBusResume (void)
{
	m_Function.SetSuspended (FALSE);
}
