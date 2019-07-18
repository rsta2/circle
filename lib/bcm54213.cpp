//
// bcm54213.cpp
//
// Driver for BCM54213PE Gigabit Ethernet Transceiver of Raspberry Pi 4
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2019  R. Stange <rsta2@o2online.de>
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
#include <circle/bcm54213.h>
#include <circle/bcmpropertytags.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <assert.h>

static const char FromBcm54213[] = "genet";

CBcm54213Device::CBcm54213Device (void)
{
}

CBcm54213Device::~CBcm54213Device (void)
{
}

boolean CBcm54213Device::Initialize (void)
{
	CBcmPropertyTags Tags;
	TPropertyTagMACAddress MACAddress;
	if (!Tags.GetTag (PROPTAG_GET_MAC_ADDRESS, &MACAddress, sizeof MACAddress))
	{
		CLogger::Get ()->Write (FromBcm54213, LogError, "Cannot get MAC address");

		return FALSE;
	}

	m_MACAddress.Set (MACAddress.Address);

	CString MACString;
	m_MACAddress.Format (&MACString);
	CLogger::Get ()->Write (FromBcm54213, LogDebug, "MAC address is %s",
				(const char *) MACString);

	AddNetDevice ();

	return TRUE;
}

const CMACAddress *CBcm54213Device::GetMACAddress (void) const
{
	return &m_MACAddress;
}

boolean CBcm54213Device::SendFrame (const void *pBuffer, unsigned nLength)
{
	return TRUE;
}

boolean CBcm54213Device::ReceiveFrame (void *pBuffer, unsigned *pResultLength)
{
	return FALSE;
}

boolean CBcm54213Device::IsLinkUp (void)
{
	return TRUE;
}

TNetDeviceSpeed CBcm54213Device::GetLinkSpeed (void)
{
	return NetDeviceSpeedUnknown;
}
