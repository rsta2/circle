//
// usbsubsystem.h
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
#include <circle/usb/usbsubsystem.h>
#include <circle/usb/xhciconfig.h>
#include <circle/memory.h>
#include <circle/logger.h>
#include <assert.h>

LOGMODULE ("usb");

CUSBSubSystem *CUSBSubSystem::s_pThis = nullptr;

CUSBSubSystem::CUSBSubSystem (CInterruptSystem *pInterruptSystem, CTimer *pTimer,
			      boolean bPlugAndPlay)
:	m_bPlugAndPlay (bPlugAndPlay),
	m_Southbridge (pInterruptSystem),
	m_SharedMemAllocator (
		CMemorySystem::GetCoherentPage (COHERENT_SLOT_XHCI_START),
		CMemorySystem::GetCoherentPage (COHERENT_SLOT_XHCI_END) + PAGE_SIZE - 1)
{
	assert (!s_pThis);
	s_pThis = this;

	for (unsigned i = 0; i < NumDevices; i++)
	{
		m_pXHCIDevice[i] = new CXHCIDevice (pInterruptSystem, pTimer, bPlugAndPlay,
						    i, &m_SharedMemAllocator);
		assert (m_pXHCIDevice[i]);
	}
}

CUSBSubSystem::~CUSBSubSystem (void)
{
	for (int i = NumDevices-1; i >= 0; i--)
	{
		delete m_pXHCIDevice[i];
		m_pXHCIDevice[i] = nullptr;
	}

	s_pThis = nullptr;
}

boolean CUSBSubSystem::Initialize (boolean bScanDevices)
{
	if (!m_Southbridge.Initialize ())
	{
		LOGERR ("Cannot initialize RP1 device");

		return FALSE;
	}

	for (unsigned i = 0; i < NumDevices; i++)
	{
		assert (m_pXHCIDevice[i]);
		if (!m_pXHCIDevice[i]->Initialize (bScanDevices))
		{
			LOGERR ("Cannot initialize xHCI device #%d", i);

			return FALSE;
		}
	}

#if !defined (NDEBUG) && defined (XHCI_DEBUG2)
	DumpStatus ();
#endif

	return TRUE;
}


boolean CUSBSubSystem::UpdatePlugAndPlay (void)
{
	boolean bResult = FALSE;

	for (unsigned i = 0; i < NumDevices; i++)
	{
		assert (m_pXHCIDevice[i]);
		if (m_pXHCIDevice[i]->UpdatePlugAndPlay ())
		{
			bResult = TRUE;
		}
	}

	return bResult;
}

int CUSBSubSystem::GetDescriptor (CUSBEndpoint *pEndpoint,
				  unsigned char ucType, unsigned char ucIndex,
				  void *pBuffer, unsigned nBufSize,
				  unsigned char ucRequestType,
				  unsigned short wIndex)
{
	assert (pEndpoint);
	CUSBDevice *pUSBDevice = pEndpoint->GetDevice ();

	assert (pUSBDevice);
	CUSBHostController *pHost = pUSBDevice->GetHost ();

	assert (pHost);
	return pHost->GetDescriptor (pEndpoint, ucType, ucIndex,
				     pBuffer, nBufSize, ucRequestType, wIndex);
}

boolean CUSBSubSystem::IsPlugAndPlay (void) const
{
	return m_bPlugAndPlay;
}

boolean CUSBSubSystem::IsActive (void)
{
	return !!s_pThis;
}

CUSBSubSystem *CUSBSubSystem::Get (void)
{
	assert (s_pThis);
	return s_pThis;
}

#ifndef NDEBUG

void CUSBSubSystem::DumpStatus (void)
{
	for (unsigned i = 0; i < NumDevices; i++)
	{
		if (m_pXHCIDevice[i])
		{
			m_pXHCIDevice[i]->DumpStatus ();
		}
	}

	m_Southbridge.DumpStatus ();

	LOGDBG ("%u KB shared memory free",
		(unsigned) (m_SharedMemAllocator.GetFreeSpace () / 1024));
}

#endif
