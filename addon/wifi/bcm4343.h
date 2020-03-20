//
// bcm4343.h
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
#ifndef _wifi_bcm4343_h
#define _wifi_bcm4343_h

#include <circle/netdevice.h>
#include <circle/macaddress.h>
#include <circle/net/netqueue.h>
#include <circle/string.h>
#include <circle/types.h>

class CBcm4343Device : public CNetDevice	/// Driver for BCM4343x Wi-Fi device
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

	// returns TRUE if PHY link is up
	boolean IsLinkUp (void);

public:
	/// \param pFormat Device specific control command (0-terminated)
	/// \return Operation successful?
	boolean Control (const char *pFormat, ...);

	/// \brief Poll for a received scan result message
	/// \param pBuffer Message will be placed here, buffer must have size FRAME_BUFFER_SIZE
	/// \param pResultLength Pointer to variable, which receives the valid message length
	/// \return TRUE if a message is returned in buffer, FALSE if nothing has been received
	boolean ReceiveScanResult (void *pBuffer, unsigned *pResultLength);

	const CMACAddress *GetBSSID (void);

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
