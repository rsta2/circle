//
// bcm4343.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020-2021  R. Stange <rsta2@o2online.de>
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
#include <wlan/bcm4343.h>
#include <wlan/p9compat.h>
#include <circle/sysconfig.h>
#include <assert.h>

#if RASPPI <= 3 && !defined (USE_SDHOST)
	#warning WLAN cannot be used parallel with SD card access in this configuration!
#endif

extern "C" void ether4330link (void);

static ether_pnp_t *s_pEtherPnpHandler = 0;
static Ether s_EtherDevice;

CBcm4343Device *CBcm4343Device::s_pThis = 0;

CBcm4343Device::CBcm4343Device (const char *pFirmwarePath)
:	m_FirmwarePath (pFirmwarePath)
{
	s_pThis = this;
}

CBcm4343Device::~CBcm4343Device (void)
{
	assert (s_EtherDevice.shutdown != 0);
	(*s_EtherDevice.shutdown) (&s_EtherDevice);

	delete s_EtherDevice.oq;

	s_pThis = 0;
}

boolean CBcm4343Device::Initialize (void)
{
	p9arch_init ();
	p9chan_init (m_FirmwarePath);
	p9proc_init ();

	ether4330link ();
	assert (s_pEtherPnpHandler != 0);
	(*s_pEtherPnpHandler) (&s_EtherDevice);

	s_EtherDevice.oq = new Queue;
	memset (s_EtherDevice.oq, 0, sizeof *s_EtherDevice.oq);

	if (waserror ())
	{
		return FALSE;
	}

	assert (s_EtherDevice.attach != 0);
	(*s_EtherDevice.attach) (&s_EtherDevice);

	m_MACAddress.Set (s_EtherDevice.ea);

	AddNetDevice ();

	poperror ();

	return TRUE;
}

const CMACAddress *CBcm4343Device::GetMACAddress (void) const
{
	return &m_MACAddress;
}

boolean CBcm4343Device::SendFrame (const void *pBuffer, unsigned nLength)
{
	//hexdump (pBuffer, nLength, "wlantx");

	Block *pBlock = allocb (nLength);
	assert (pBlock != 0);

	assert (pBlock->wp != 0);
	assert (pBuffer != 0);
	memcpy (pBlock->wp, pBuffer, nLength);
	pBlock->wp += nLength;

	assert (s_EtherDevice.oq != 0);
	qpass (s_EtherDevice.oq, pBlock);

	if (waserror ())
	{
		return FALSE;
	}

	assert (s_EtherDevice.transmit != 0);
	(*s_EtherDevice.transmit) (&s_EtherDevice);

	poperror ();

	return TRUE;
}

boolean CBcm4343Device::ReceiveFrame (void *pBuffer, unsigned *pResultLength)
{
	assert (pBuffer != 0);
	unsigned nLength = m_RxQueue.Dequeue (pBuffer);
	if (nLength == 0)
	{
		return FALSE;
	}

	assert (pResultLength != 0);
	*pResultLength = nLength;

	//hexdump (pBuffer, nLength, "wlanrx");

	return TRUE;
}

void CBcm4343Device::RegisterEventHandler (TBcm4343EventHandler *pHandler, void *pContext)
{
	assert (s_EtherDevice.setevhndlr != 0);
	(*s_EtherDevice.setevhndlr) (&s_EtherDevice, pHandler, pContext);
}

boolean CBcm4343Device::Control (const char *pFormat, ...)
{
	assert (pFormat != 0);

	va_list var;
	va_start (var, pFormat);

	CString Command;
	Command.FormatV (pFormat, var);

	va_end (var);

	if (waserror ())
	{
		return FALSE;
	}

	assert (s_EtherDevice.ctl != 0);
	(*s_EtherDevice.ctl) (&s_EtherDevice, (const char *) Command, 0);

	poperror ();

	return TRUE;
}

boolean CBcm4343Device::ReceiveScanResult (void *pBuffer, unsigned *pResultLength)
{
	assert (pBuffer != 0);
	unsigned nLength = m_ScanResultQueue.Dequeue (pBuffer);
	if (nLength == 0)
	{
		return FALSE;
	}

	assert (pResultLength != 0);
	*pResultLength = nLength;

	//hexdump (pBuffer, nLength, "wlanscan");

	return TRUE;
}

const CMACAddress *CBcm4343Device::GetBSSID (void)
{
	u8 BSSID[MAC_ADDRESS_SIZE];
	assert (s_EtherDevice.getbssid != 0);
	(*s_EtherDevice.getbssid) (&s_EtherDevice, BSSID);

	m_BSSID.Set (BSSID);

	return &m_BSSID;
}

boolean CBcm4343Device::JoinOpenNet (const char *pSSID)
{
	assert (pSSID != 0);
	return Control ("join %s %s 0 off", pSSID, "FFFFFFFFFFFF");
}

// by @sebastienNEC
boolean CBcm4343Device::CreateOpenNet (const char *pSSID, int nChannel, bool bHidden)
{
	assert (pSSID != 0);
	return Control ("create %s %d %d", pSSID, nChannel, bHidden);
}

void CBcm4343Device::DumpStatus (void)
{
	char Buffer[200];

	assert (s_EtherDevice.ifstat != 0);
	long nLength = (*s_EtherDevice.ifstat) (&s_EtherDevice, Buffer, sizeof Buffer, 0);
	Buffer[nLength] = '\0';

	print (Buffer);
}

void CBcm4343Device::FrameReceived (const void *pBuffer, unsigned nLength)
{
	assert (s_pThis != 0);
	s_pThis->m_RxQueue.Enqueue (pBuffer, nLength);
}

void CBcm4343Device::ScanResultReceived (const void *pBuffer, unsigned nLength)
{
	assert (s_pThis != 0);
	s_pThis->m_ScanResultQueue.Enqueue (pBuffer, nLength);
}

void etheriq (Ether *pEther, Block *pBlock, unsigned nFlag)
{
	assert (pBlock != 0);
	CBcm4343Device::FrameReceived (pBlock->rp, BLEN (pBlock));

	freeb (pBlock);
}

void etherscanresult (Ether *pEther, const void *pBuffer, long nLength)
{
	assert (pBuffer != 0);
	CBcm4343Device::ScanResultReceived (pBuffer, (unsigned) nLength);
}

void addethercard (const char *pName, ether_pnp_t *pEtherPnpHandler)
{
	assert (pEtherPnpHandler != 0);
	s_pEtherPnpHandler = pEtherPnpHandler;
}
