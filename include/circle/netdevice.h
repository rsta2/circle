//
// netdevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_netdevice_h
#define _circle_netdevice_h

#include <circle/macaddress.h>
#include <circle/types.h>

#define FRAME_BUFFER_SIZE	1600

#define MAX_NET_DEVICES		5

enum TNetDeviceType
{
	NetDeviceTypeEthernet,
	NetDeviceTypeWLAN,
	NetDeviceTypeAny,
	NetDeviceTypeUnknown
};

enum TNetDeviceSpeed
{
	NetDeviceSpeed10Half,
	NetDeviceSpeed10Full,
	NetDeviceSpeed100Half,
	NetDeviceSpeed100Full,
	NetDeviceSpeed1000Half,
	NetDeviceSpeed1000Full,
	NetDeviceSpeedUnknown
};

class CNetDevice	/// Base class (interface) of net devices
{
public:
	virtual ~CNetDevice (void) {}

	/// \return Type of this net device
	virtual TNetDeviceType GetType (void)		{ return NetDeviceTypeEthernet; }

	/// \return Pointer to a MAC address object, which holds our own address
	virtual const CMACAddress *GetMACAddress (void) const = 0;

	/// \return TRUE if it is advisable to call SendFrame()
	/// \note SendFrame() can be called at any time, but may fail when the TX queue is full.\n
	///	  This method gives a hint, if calling SendFrame() is advisable.
	virtual boolean IsSendFrameAdvisable (void)	{ return TRUE; }

	/// \brief Send a valid Ethernet frame to the network
	/// \param pBuffer Pointer to the frame, does not contain FCS
	/// \param nLength Frame length in bytes, does not need to be padded
	virtual boolean SendFrame (const void *pBuffer, unsigned nLength) = 0;

	/// \brief Poll for a received Ethernet frame
	/// \param pBuffer Frame will be placed here, buffer must have size FRAME_BUFFER_SIZE
	/// \param pResultLength Pointer to variable, which receives the valid frame length
	/// \return TRUE if a frame is returned in buffer, FALSE if nothing has been received
	virtual boolean ReceiveFrame (void *pBuffer, unsigned *pResultLength) = 0;

	/// \return TRUE if PHY link is up
	virtual boolean IsLinkUp (void)			{ return TRUE; }

	/// \return The speed of the PHY link, if it is up
	virtual TNetDeviceSpeed GetLinkSpeed (void)	{ return NetDeviceSpeedUnknown; }

	/// \brief Update device settings according to PHY status
	/// \return FALSE if not supported
	/// \note This is called continuously every 2 seconds by the net PHY task
	virtual boolean UpdatePHY (void)		{ return FALSE; }

	/// \param Speed A value returned by GetLinkSpeed()
	/// \return Description for this speed value
	static const char *GetSpeedString (TNetDeviceSpeed Speed);

	/// \param nDeviceNumber Zero-based number of a net device (normally only 0 is used)
	/// \return Pointer to the device object
	static CNetDevice *GetNetDevice (unsigned nDeviceNumber);

	/// \param Type Specific net device type to search for (or NetDeviceTypeAny)
	/// \return Pointer to the first device object of this type
	static CNetDevice *GetNetDevice (TNetDeviceType Type);

protected:
	void AddNetDevice (void);

private:
	static unsigned s_nDeviceNumber;
	static CNetDevice *s_pDevice[MAX_NET_DEVICES];

	static const char *s_SpeedString[NetDeviceSpeedUnknown];
};

#endif
