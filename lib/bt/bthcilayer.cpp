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
#include <circle/bt/bthcilayer.h>
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <assert.h>

static const char FromHCILayer[] = "bthci";

CBTHCILayer *CBTHCILayer::s_pThis = 0;

CBTHCILayer::CBTHCILayer (u32 nClassOfDevice, const char *pLocalName)
:	m_pHCITransportUSB (0),
	m_pHCITransportUART (0),
	m_DeviceManager (this, &m_DeviceEventQueue, nClassOfDevice, pLocalName),
	m_pEventBuffer (0),
	m_nEventLength (0),
	m_nEventFragmentOffset (0),
	m_pBuffer (0),
	m_nCommandPackets (1)
{
	assert (s_pThis == 0);
	s_pThis = this;
}

CBTHCILayer::~CBTHCILayer (void)
{
	m_pHCITransportUSB = 0;
	m_pHCITransportUART = 0;

	delete [] m_pBuffer;
	m_pBuffer = 0;

	delete [] m_pEventBuffer;
	m_pEventBuffer = 0;

	s_pThis = 0;
}

boolean CBTHCILayer::Initialize (void)
{
	m_pHCITransportUSB = (CUSBBluetoothDevice *) CDeviceNameService::Get ()->GetDevice ("ubt1", FALSE);
	if (m_pHCITransportUSB == 0)
	{
		m_pHCITransportUART = (CBTUARTTransport *) CDeviceNameService::Get ()->GetDevice ("ttyBT1", FALSE);
		if (m_pHCITransportUART == 0)
		{
			CLogger::Get ()->Write (FromHCILayer, LogError, "Bluetooth controller not found");

			return FALSE;
		}
	}

	m_pEventBuffer = new u8[BT_MAX_HCI_EVENT_SIZE];
	assert (m_pEventBuffer != 0);

	m_pBuffer = new u8[BT_MAX_DATA_SIZE];
	assert (m_pBuffer != 0);

	if (m_pHCITransportUSB != 0)
	{
		m_pHCITransportUSB->RegisterHCIEventHandler (EventStub);
	}
	else
	{
		assert (m_pHCITransportUART != 0);
		m_pHCITransportUART->RegisterHCIEventHandler (EventStub);
	}

	return m_DeviceManager.Initialize ();
}

TBTTransportType CBTHCILayer::GetTransportType (void) const
{
	if (m_pHCITransportUSB != 0)
	{
		return BTTransportTypeUSB;
	}

	if (m_pHCITransportUART != 0)
	{
		return BTTransportTypeUART;
	}

	return BTTransportTypeUnknown;
}

void CBTHCILayer::Process (void)
{
	assert (m_pHCITransportUSB != 0 || m_pHCITransportUART != 0);
	assert (m_pBuffer != 0);

	unsigned nLength;
	while (   m_nCommandPackets > 0
	       && (nLength = m_CommandQueue.Dequeue (m_pBuffer)) > 0)
	{
		if (  m_pHCITransportUSB != 0
		    ? !m_pHCITransportUSB->SendHCICommand (m_pBuffer, nLength)
		    : !m_pHCITransportUART->SendHCICommand (m_pBuffer, nLength))
		{
			CLogger::Get ()->Write (FromHCILayer, LogError, "HCI command dropped");

			break;
		}

		m_nCommandPackets--;
	}

	m_DeviceManager.Process ();
}

void CBTHCILayer::SendCommand (const void *pBuffer, unsigned nLength)
{
	m_CommandQueue.Enqueue (pBuffer, nLength);
}

boolean CBTHCILayer::ReceiveLinkEvent (void *pBuffer, unsigned *pResultLength)
{
	unsigned nLength = m_LinkEventQueue.Dequeue (pBuffer);
	if (nLength > 0)
	{
		assert (pResultLength != 0);
		*pResultLength = nLength;

		return TRUE;
	}

	return FALSE;
}

void CBTHCILayer::SetCommandPackets (unsigned nCommandPackets)
{
	m_nCommandPackets = nCommandPackets;
}

CBTDeviceManager *CBTHCILayer::GetDeviceManager (void)
{
	return &m_DeviceManager;
}

void CBTHCILayer::EventHandler (const void *pBuffer, unsigned nLength)
{
	assert (pBuffer != 0);
	assert (nLength > 0);

	if (m_nEventFragmentOffset == 0)
	{
		if (nLength < sizeof (TBTHCIEventHeader))
		{
			CLogger::Get ()->Write (FromHCILayer, LogWarning, "Short event ignored");

			return;
		}

		TBTHCIEventHeader *pHeader = (TBTHCIEventHeader *) pBuffer;

		assert (m_nEventLength == 0);
		m_nEventLength = sizeof (TBTHCIEventHeader) + pHeader->ParameterTotalLength;
	}

	assert (m_pEventBuffer != 0);
	memcpy (m_pEventBuffer + m_nEventFragmentOffset, pBuffer, nLength);

	m_nEventFragmentOffset += nLength;
	if (m_nEventFragmentOffset < m_nEventLength)
	{
		return;
	}

	TBTHCIEventHeader *pHeader = (TBTHCIEventHeader *) m_pEventBuffer;
	switch (pHeader->EventCode)
	{
	case EVENT_CODE_COMMAND_COMPLETE:
	case EVENT_CODE_COMMAND_STATUS:
		m_DeviceEventQueue.Enqueue (m_pEventBuffer, m_nEventLength);
		break;

	default:
		m_LinkEventQueue.Enqueue (m_pEventBuffer, m_nEventLength);
		break;
	}

	m_nEventLength = 0;
	m_nEventFragmentOffset = 0;
}

void CBTHCILayer::EventStub (const void *pBuffer, unsigned nLength)
{
	assert (s_pThis != 0);
	s_pThis->EventHandler (pBuffer, nLength);
}
