//
// bcm4343.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  R. Stange <rsta2@o2online.de>
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
#include <wifi/bcm4343.h>
#include <wifi/p9compat.h>
#include <assert.h>

extern "C" void ether4330link (void);

static ether_pnp_t *s_pEtherPnpHandler = 0;
static Ether s_EtherDevice;

CBcm4343Device *CBcm4343Device::s_pThis = 0;

CBcm4343Device::CBcm4343Device (const char *pFirmwarePath)
:	m_FirmwarePath (pFirmwarePath),
	m_AuthMode (AuthModeNone)
{
	s_pThis = this;
}

CBcm4343Device::~CBcm4343Device (void)
{
	delete s_EtherDevice.oq;

	s_pThis = 0;
}

void CBcm4343Device::SetESSID (const char *pESSID)
{
	m_ESSIDCmd.Format ("essid %s", pESSID);
}

void CBcm4343Device::SetAuth (TAuthMode Mode, const char *pKey)
{
	m_AuthMode = Mode;
	if (Mode == AuthModeNone)
	{
		return;
	}

	assert (pKey != 0);
	size_t nLength = strlen (pKey);
	assert (8 <= nLength && nLength <= 63);

	m_AuthCmd.Format ("auth %02X%02X", Mode == AuthModeWPA ? 0xDD : 0x30, nLength);

	while (nLength-- > 0)
	{
		CString Number;
		Number.Format ("%02X", (unsigned) *pKey++);

		m_AuthCmd.Append (Number);
	}
}

boolean CBcm4343Device::Initialize (void)
{
	p9arch_init ();
	p9chan_init (m_FirmwarePath);

	ether4330link ();
	assert (s_pEtherPnpHandler != 0);
	(*s_pEtherPnpHandler) (&s_EtherDevice);

	s_EtherDevice.oq = new Queue;
	memset (s_EtherDevice.oq, 0, sizeof *s_EtherDevice.oq);

	assert (s_EtherDevice.attach != 0);
	(*s_EtherDevice.attach) (&s_EtherDevice);

	m_MACAddress.Set (s_EtherDevice.ea);

	assert (s_EtherDevice.ctl != 0);
	if (m_AuthMode != AuthModeNone)
	{
		(*s_EtherDevice.ctl) (&s_EtherDevice, (const char *) m_AuthCmd,
				      m_AuthCmd.GetLength ());
	}

	(*s_EtherDevice.ctl) (&s_EtherDevice, (const char *) m_ESSIDCmd, m_ESSIDCmd.GetLength ());

	AddNetDevice ();

	return TRUE;
}

const CMACAddress *CBcm4343Device::GetMACAddress (void) const
{
	return &m_MACAddress;
}

boolean CBcm4343Device::SendFrame (const void *pBuffer, unsigned nLength)
{
	//hexdump (pBuffer, nLength, "wifitx");

	Block *pBlock = allocb (nLength);
	assert (pBlock != 0);

	assert (pBlock->wp != 0);
	assert (pBuffer != 0);
	memcpy (pBlock->wp, pBuffer, nLength);
	pBlock->wp += nLength;

	assert (s_EtherDevice.oq != 0);
	qpass (s_EtherDevice.oq, pBlock);

	assert (s_EtherDevice.transmit != 0);
	(*s_EtherDevice.transmit) (&s_EtherDevice);

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

	//hexdump (pBuffer, nLength, "wifirx");

	return TRUE;
}

boolean CBcm4343Device::IsLinkUp (void)
{
	return TRUE;
}

void CBcm4343Device::DumpStatus (void)
{
	char Buffer[200];

	assert (s_EtherDevice.ifstat != 0);
	long nLength = (*s_EtherDevice.ifstat) (&s_EtherDevice, Buffer, sizeof Buffer, 0);
	Buffer[nLength] = '\0';

	print (Buffer);
}

void CBcm4343Device::FrameReceived (void *pBuffer, unsigned nLength)
{
	assert (s_pThis != 0);
	s_pThis->m_RxQueue.Enqueue (pBuffer, nLength);
}

void etheriq (Ether *pEther, Block *pBlock, unsigned nFlag)
{
	assert (pBlock != 0);
	CBcm4343Device::FrameReceived (pBlock->rp, BLEN (pBlock));

	freeb (pBlock);
}

void addethercard (const char *pName, ether_pnp_t *pEtherPnpHandler)
{
	assert (pEtherPnpHandler != 0);
	s_pEtherPnpHandler = pEtherPnpHandler;
}
