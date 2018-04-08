//
// mqttsampleclient.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018  R. Stange <rsta2@o2online.de>
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
#include "mqttsampleclient.h"
#include <circle/bcmpropertytags.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/timer.h>
#include <circle/util.h>

#define MQTT_BROKER_HOSTNAME	"192.168.0.170"
//#define MQTT_BROKER_HOSTNAME	"test.mosquitto.org"

#define TOPIC			"circle/temp"
//#define TOPIC			"temp/random"

// See: include/circle/net/mqttclient.h
#define MAX_PACKET_SIZE		1024
#define MAX_PACKETS_QUEUED	4
#define MAX_TOPIC_SIZE		256

static const char FromSampleClient[] = "mqttsample";

CMQTTSampleClient::CMQTTSampleClient (CNetSubSystem *pNetSubSystem)
:	CMQTTClient (pNetSubSystem, MAX_PACKET_SIZE, MAX_PACKETS_QUEUED, MAX_TOPIC_SIZE),
	m_bTimerRunning (FALSE)
{
	Connect (MQTT_BROKER_HOSTNAME);
}

CMQTTSampleClient::~CMQTTSampleClient (void)
{
}

void CMQTTSampleClient::OnConnect (boolean bSessionPresent)
{
	CLogger::Get ()->Write (FromSampleClient, LogDebug, "OnConnect (present %u)",
				bSessionPresent ? 1 : 0);

	m_nConnectTime = CTimer::Get ()->GetUptime ();
	m_nLastPublishTime = m_nConnectTime;
	m_bTimerRunning = TRUE;

	Subscribe (TOPIC);
}

void CMQTTSampleClient::OnDisconnect (TMQTTDisconnectReason Reason)
{
	m_bTimerRunning = FALSE;

	CLogger::Get ()->Write (FromSampleClient, LogDebug, "OnDisconnect (reason %u)",
				(unsigned) Reason);
}

void CMQTTSampleClient::OnMessage (const char *pTopic, const u8 *pPayload,
				   size_t nPayloadLength, boolean bRetain)
{
	// pPayload is not 0-terminated, make a C-string out of it
	char PayloadString[256] = "";
	if (nPayloadLength > 0)
	{
		if (nPayloadLength > sizeof PayloadString-1)
		{
			nPayloadLength = sizeof PayloadString-1;
		}

		memcpy (PayloadString, pPayload, nPayloadLength);
		PayloadString[nPayloadLength] = '\0';
	}

	CLogger::Get ()->Write (FromSampleClient, LogNotice,
				"OnMessage (\"%s\", \"%s\")", pTopic, PayloadString);
}

void CMQTTSampleClient::OnLoop (void)
{
	if (   !IsConnected ()
	    || !m_bTimerRunning)
	{
		return;
	}

	unsigned nRunTime = CTimer::Get ()->GetUptime () - m_nConnectTime;

	// disconnect after 120 seconds
	if (nRunTime >= 120)
	{
		Disconnect ();

		return;
	}

	// publish the CPU temperature every 30 seconds, starting 5 seconds after connect
	if (   nRunTime % 30 == 5
	    && nRunTime != m_nLastPublishTime)
	{
		m_nLastPublishTime = nRunTime;

		CString Payload;
		Payload.Format ("%.1f", GetTemperature ());

		Publish (TOPIC, (const u8 *) (const char *) Payload, Payload.GetLength ());
	}
}

float CMQTTSampleClient::GetTemperature (void)
{
	CBcmPropertyTags Tags;
	TPropertyTagTemperature TagTemperature;
	TagTemperature.nTemperatureId = TEMPERATURE_ID;
	if (!Tags.GetTag (PROPTAG_GET_TEMPERATURE, &TagTemperature, sizeof TagTemperature, 4))
	{
		return 0.0;
	}

	return (float) TagTemperature.nValue / 1000.0;
}
