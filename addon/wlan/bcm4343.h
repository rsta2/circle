//
// bcm4343.h
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
#ifndef _wlan_bcm4343_h
#define _wlan_bcm4343_h

#include <circle/netdevice.h>
#include <circle/macaddress.h>
#include <circle/net/netqueue.h>
#include <circle/string.h>
#include <circle/types.h>
#include "etherevent.h"

typedef ether_event_handler_t TBcm4343EventHandler;

class CBcm4343Device : public CNetDevice	/// Driver for BCM4343x WLAN device
{
public:
	CBcm4343Device (const char *pFirmwarePath);		// e.g. "USB:/firmware/"
	~CBcm4343Device (void);

	TNetDeviceType GetType (void)	{ return NetDeviceTypeWLAN; }

	boolean Initialize (void);

	const CMACAddress *GetMACAddress (void) const;

	boolean SendFrame (const void *pBuffer, unsigned nLength);

	// pBuffer must have size FRAME_BUFFER_SIZE
	boolean ReceiveFrame (void *pBuffer, unsigned *pResultLength);

public:
	/// \param pHandler Pointer to event handler (0 for unregister)
	/// \param pContext Pointer to be handed over to the handler
	void RegisterEventHandler (TBcm4343EventHandler *pHandler, void *pContext);

	/// \param pFormat Device specific control command (0-terminated)
	/// \return Operation successful?
	boolean Control (const char *pFormat, ...);

	/// \brief Poll for a received scan result message
	/// \param pBuffer Message will be placed here, buffer must have size FRAME_BUFFER_SIZE
	/// \param pResultLength Pointer to variable, which receives the valid message length
	/// \return TRUE if a message is returned in buffer, FALSE if nothing has been received
	boolean ReceiveScanResult (void *pBuffer, unsigned *pResultLength);

	const CMACAddress *GetBSSID (void);

	/// \param pSSID SSID of open network to be joined
	/// \return Operation successful?
	boolean JoinOpenNet (const char *pSSID);

	/// \param pSSID SSID of open network to be created
	/// \param nChannel 802.11 channel of open network to be created
	/// \param bHidden Whether to hide the SSID
	/// \return Operation successful?
	boolean CreateOpenNet (const char *pSSID, int nChannel, bool bHidden);

	void DumpStatus (void);

public:
	static void FrameReceived (const void *pBuffer, unsigned nLength);
	static void ScanResultReceived (const void *pBuffer, unsigned nLength);

private:
	CString m_FirmwarePath;

	CMACAddress m_MACAddress;
	CMACAddress m_BSSID;

	CNetQueue m_RxQueue;
	CNetQueue m_ScanResultQueue;

	static CBcm4343Device *s_pThis;
};

#endif
