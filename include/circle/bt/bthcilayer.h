//
// bthcilayer.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bt_bthcilayer_h
#define _circle_bt_bthcilayer_h

#include <circle/bt/bttransportlayer.h>
#include <circle/usb/usbbluetooth.h>
#include <circle/bt/btuarttransport.h>
#include <circle/bt/bluetooth.h>
#include <circle/bt/btdevicemanager.h>
#include <circle/bt/btqueue.h>
#include <circle/types.h>

class CBTHCILayer
{
public:
	CBTHCILayer (u32 nClassOfDevice, const char *pLocalName);
	~CBTHCILayer (void);

	boolean Initialize (void);

	TBTTransportType GetTransportType (void) const;

	void Process (void);

	void SendCommand (const void *pBuffer, unsigned nLength);

	// pBuffer must have size BT_MAX_HCI_EVENT_SIZE
	boolean ReceiveLinkEvent (void *pBuffer, unsigned *pResultLength);

	void SetCommandPackets (unsigned nCommandPackets);	// set commands allowed to be sent

	CBTDeviceManager *GetDeviceManager (void);

private:
	void EventHandler (const void *pBuffer, unsigned nLength);
	static void EventStub (const void *pBuffer, unsigned nLength);

private:
	CUSBBluetoothDevice *m_pHCITransportUSB;
	CBTUARTTransport *m_pHCITransportUART;

	CBTDeviceManager m_DeviceManager;

	CBTQueue m_CommandQueue;
	CBTQueue m_DeviceEventQueue;
	CBTQueue m_LinkEventQueue;

	u8 *m_pEventBuffer;
	unsigned m_nEventLength;
	unsigned m_nEventFragmentOffset;

	u8 *m_pBuffer;

	unsigned m_nCommandPackets;		// commands allowed to be sent

	static CBTHCILayer *s_pThis;
};

#endif
