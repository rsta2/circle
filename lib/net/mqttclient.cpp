//
// mqttclient.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/net/mqttclient.h>
#include <circle/net/dnsclient.h>
#include <circle/net/ipaddress.h>
#include <circle/net/in.h>
#include <circle/sched/scheduler.h>
#include <circle/bcmpropertytags.h>
#include <circle/logger.h>
#include <assert.h>

const char *CMQTTClient::s_pErrorMsg[MQTTDisconnectUnknown+1] =
{
	"Disconnect from application",
	"Unacceptable protocol version",
	"Identifier rejected",
	"Server unavailable",
	"Bad username or password",
	"Not authorized",
	"DNS error",
	"Connect failed",
	"Disconnect from peer",
	"Invalid packet received",
	"Invalid identifier",
	"Subscribe error",
	"Send failed",
	"Ping timeout",
	"Not supported",
	"Insufficient resources",
	"Unknown reason"
};

static const char FromMQTTClient[] = "mqtt";

CMQTTClient::CMQTTClient (CNetSubSystem *pNetSubSystem, size_t nMaxPacketSize,
			  size_t nMaxPacketsQueued, size_t nMaxTopicSize)
:	m_pNetSubSystem (pNetSubSystem),
	m_nMaxPacketSize (nMaxPacketSize),
	m_nMaxTopicSize (nMaxTopicSize),
	m_pTimer (CTimer::Get ()),
	m_pSocket (0),
	m_ConnectStatus (MQTTStatusDisconnected),
	m_ReceivePacket (nMaxPacketSize, nMaxPacketsQueued)
{
	SetName (FromMQTTClient);

	m_pTopicBuffer = new char [m_nMaxTopicSize+1];
}

CMQTTClient::~CMQTTClient (void)
{
	assert (m_ConnectStatus == MQTTStatusDisconnected);
	assert (m_pSocket == 0);

	CleanupQueue ();
	CleanupPacketIdentifierStore ();

	delete [] m_pTopicBuffer;
	m_pTopicBuffer = 0;

	m_pTimer = 0;
	m_pNetSubSystem = 0;
}

boolean CMQTTClient::IsConnected (void) const
{
	return m_ConnectStatus >= MQTTStatusConnected;
}

void CMQTTClient::Connect (const char *pHost, u16 usPort, const char *pClientIdentifier,
			   const char *pUsername, const char *pPassword,
			   u16 usKeepAliveSeconds, boolean bCleanSession,
			   const char *pWillTopic, u8 uchWillQoS, boolean bWillRetain,
			   const u8 *pWillPayload, size_t nWillPayloadLength)
{
	assert (m_ConnectStatus == MQTTStatusDisconnected);

	if (m_pTopicBuffer == 0)
	{
		OnDisconnect (MQTTDisconnectInsufficientResources);

		return;
	}

	assert (m_pNetSubSystem != 0);
	CDNSClient DNSClient (m_pNetSubSystem);

	CIPAddress IPServer;
	if (!DNSClient.Resolve (pHost, &IPServer))
	{
		CLogger::Get ()->Write (FromMQTTClient, LogError, "Cannot resolve: %s", pHost);

		OnDisconnect (MQTTDisconnectDNSError);

		return;
	}

	IPServer.Format (&m_IPServerString);
	CLogger::Get ()->Write (FromMQTTClient, LogDebug, "%s: Opening connection",
				(const char *) m_IPServerString);

	assert (m_pSocket == 0);
	m_pSocket = new CSocket (m_pNetSubSystem, IPPROTO_TCP);
	assert (m_pSocket != 0);

	if (m_pSocket->Connect (IPServer, usPort) != 0)
	{
		CLogger::Get ()->Write (FromMQTTClient, LogError, "Cannot connect: %s", pHost);

		delete m_pSocket;
		m_pSocket = 0;

		OnDisconnect (MQTTDisconnectConnectFailed);

		return;
	}

	m_nKeepAliveSeconds = usKeepAliveSeconds;
	m_bTimerRunning = FALSE;
	m_usNextPacketIdentifier = 1;
	m_ReceivePacket.Reset ();
	m_ConnectStatus = MQTTStatusConnectPending;

	CString ClientIdentifier;
	if (pClientIdentifier != 0)
	{
		ClientIdentifier = pClientIdentifier;

		if (ClientIdentifier.GetLength () > 23)
		{
			CLogger::Get ()->Write (FromMQTTClient, LogWarning,
						"Client ID exceeds 23 characters");
		}
	}
	else
	{
		CBcmPropertyTags Tags;
		TPropertyTagSerial Serial;
		if (!Tags.GetTag (PROPTAG_GET_BOARD_SERIAL, &Serial, sizeof Serial))
		{
			Serial.Serial[0] = 0;
			Serial.Serial[1] = 0;
		}

		ClientIdentifier.Format ("raspi%08x%08x", Serial.Serial[1], Serial.Serial[0]);
	}

	u8 uchConnectFlags = 0;
	if (bCleanSession)
	{
		uchConnectFlags |= MQTT_CONNECT_FLAG_CLEAN_SESSION;
	}

	if (pWillTopic != 0)
	{
		uchConnectFlags |= MQTT_CONNECT_FLAG_WILL;

		assert (uchWillQoS <= MQTT_QOS_EXACTLY_ONCE);
		uchConnectFlags |= uchWillQoS << MQTT_CONNECT_FLAG_WILL_QOS__SHIFT;

		if (bWillRetain)
		{
			uchConnectFlags |= MQTT_CONNECT_FLAG_WILL_RETAIN;
		}
	}

	if (pUsername != 0)
	{
		uchConnectFlags |= MQTT_CONNECT_FLAG_USER_NAME;

		if (pPassword != 0)
		{
			uchConnectFlags |= MQTT_CONNECT_FLAG_PASSWORD;
		}
	}

	CMQTTSendPacket Packet (MQTTConnect, m_nMaxPacketSize);
	Packet.AppendString ("MQTT");
	Packet.AppendByte (MQTT_PROTOCOL_LEVEL);
	Packet.AppendByte (uchConnectFlags);
	Packet.AppendWord (usKeepAliveSeconds);
	Packet.AppendString (ClientIdentifier);

	if (pWillTopic != 0)
	{
		Packet.AppendString (pWillTopic);

		assert (nWillPayloadLength < 0x10000);
		Packet.AppendWord (nWillPayloadLength);
		if (nWillPayloadLength > 0)
		{
			assert (pWillPayload != 0);
			Packet.AppendData (pWillPayload, nWillPayloadLength);
		}
	}

	if (pUsername != 0)
	{
		Packet.AppendString (pUsername);

		if (pPassword != 0)
		{
			Packet.AppendString (pPassword);
		}
	}

	if (!SendPacket (&Packet))
	{
		CloseConnection (MQTTDisconnectSendFailed);

		return;
	}
}

void CMQTTClient::Disconnect (boolean bForce)
{
	if (!bForce)
	{
		CMQTTSendPacket Packet (MQTTDisconnect);

		SendPacket (&Packet);
	}

	CloseConnection (MQTTDisconnectFromApplication);
}

void CMQTTClient::Subscribe (const char *pTopic, u8 uchQoS)
{
	assert (pTopic != 0);
	assert (uchQoS <= MQTT_QOS_EXACTLY_ONCE);

	u16 usPacketIdentifier = m_usNextPacketIdentifier;
	if (++m_usNextPacketIdentifier == 0)
	{
		m_usNextPacketIdentifier++;
	}

	CMQTTSendPacket *pPacket = new CMQTTSendPacket (MQTTSubscribe, m_nMaxPacketSize);
	assert (pPacket != 0);

	pPacket->AppendWord (usPacketIdentifier);
	pPacket->AppendString (pTopic);
	pPacket->AppendByte (uchQoS);

	if (!SendPacket (pPacket))
	{
		delete pPacket;

		CloseConnection (MQTTDisconnectSendFailed);

		return;
	}

	pPacket->SetQoS (MQTT_QOS_AT_LEAST_ONCE);
	pPacket->SetPacketIdentifier (usPacketIdentifier);

	InsertPacketIntoQueue (pPacket, m_pTimer->GetTicks () + MQTT_RESEND_TIMEOUT);
}

void CMQTTClient::Unsubscribe (const char *pTopic)
{
	assert (pTopic != 0);

	u16 usPacketIdentifier = m_usNextPacketIdentifier;
	if (++m_usNextPacketIdentifier == 0)
	{
		m_usNextPacketIdentifier++;
	}

	CMQTTSendPacket *pPacket = new CMQTTSendPacket (MQTTUnsubscribe, m_nMaxPacketSize);
	assert (pPacket != 0);

	pPacket->AppendWord (usPacketIdentifier);
	pPacket->AppendString (pTopic);

	if (!SendPacket (pPacket))
	{
		delete pPacket;

		CloseConnection (MQTTDisconnectSendFailed);

		return;
	}

	pPacket->SetQoS (MQTT_QOS_AT_LEAST_ONCE);
	pPacket->SetPacketIdentifier (usPacketIdentifier);

	InsertPacketIntoQueue (pPacket, m_pTimer->GetTicks () + MQTT_RESEND_TIMEOUT);
}

void CMQTTClient::Publish (const char *pTopic, const u8 *pPayload, size_t nPayloadLength,
			   u8 uchQoS, boolean bRetain)
{
	assert (pTopic != 0);

	assert (uchQoS <= MQTT_QOS_EXACTLY_ONCE);
	u8 uchFlags = uchQoS << MQTT_FLAG_QOS__SHIFT;
	if (bRetain)
	{
		uchFlags |= MQTT_FLAG_RETAIN;
	}

	u16 usPacketIdentifier = m_usNextPacketIdentifier;
	if (++m_usNextPacketIdentifier == 0)
	{
		m_usNextPacketIdentifier++;
	}

	CMQTTSendPacket *pPacket = new CMQTTSendPacket (MQTTPublish, m_nMaxPacketSize);
	assert (pPacket != 0);

	pPacket->SetFlags (uchFlags);
	pPacket->AppendString (pTopic);
	pPacket->AppendWord (usPacketIdentifier);

	if (nPayloadLength > 0)
	{
		assert (pPayload != 0);
		pPacket->AppendData (pPayload, nPayloadLength);
	}

	if (!SendPacket (pPacket))
	{
		delete pPacket;

		CloseConnection (MQTTDisconnectSendFailed);

		return;
	}

	if (uchQoS >= MQTT_QOS_AT_LEAST_ONCE)
	{
		pPacket->SetQoS (uchQoS);
		pPacket->SetPacketIdentifier (usPacketIdentifier);

		InsertPacketIntoQueue (pPacket, m_pTimer->GetTicks () + MQTT_RESEND_TIMEOUT);
	}
	else
	{
		delete pPacket;
	}
}

void CMQTTClient::Run (void)
{
	while (1)
	{
		if (m_ConnectStatus != MQTTStatusDisconnected)
		{
			Receiver ();
			Sender ();
			KeepAliveHandler ();

			CScheduler::Get ()->MsSleep (50);
		}
		else
		{
			CScheduler::Get ()->MsSleep (200);
		}

		OnLoop ();
	}
}

void CMQTTClient::Receiver (void)
{
	while (1)
	{
		assert (m_pSocket != 0);
		TMQTTReceiveStatus Status = m_ReceivePacket.Receive (m_pSocket);
		switch (Status)
		{
		case MQTTReceiveContinue:
			return;

		case MQTTReceivePacketOK:
			break;

		case MQTTReceivePacketInvalid:
			CloseConnection (MQTTDisconnectInvalidPacket);
			return;

		case MQTTReceiveDisconnect:
			CloseConnection (MQTTDisconnectFromPeer);
			return;

		case MQTTReceiveNotEnoughMemory:
			CloseConnection (MQTTDisconnectInsufficientResources);
			return;

		default:
			assert (0);
			break;
		}

		switch (m_ReceivePacket.GetType ())
		{
		case MQTTConnAck: {
			u8 uchConnAckFlags = m_ReceivePacket.GetByte ();
			u8 uchReturnCode = m_ReceivePacket.GetByte ();
			m_ReceivePacket.Complete ();

			if (m_ConnectStatus != MQTTStatusConnectPending)
			{
				CloseConnection (MQTTDisconnectInvalidPacket);
			}
			else if (uchReturnCode == 0)
			{
				m_ConnectStatus = MQTTStatusConnected;

				OnConnect (uchConnAckFlags & MQTT_CONNACK_FLAG_SP ? TRUE : FALSE);
			}
			else
			{
				if (uchReturnCode > 5)
				{
					uchReturnCode = MQTTDisconnectUnknown;
				}

				CloseConnection ((TMQTTDisconnectReason) uchReturnCode);
			}
			} break;

		case MQTTPublish: {
			u8 uchFlags = m_ReceivePacket.GetFlags ();
			u8 uchQoS = (uchFlags & MQTT_FLAG_QOS__MASK) >> MQTT_FLAG_QOS__SHIFT;
			boolean bRetain = uchFlags & MQTT_FLAG_RETAIN ? TRUE : FALSE;

			size_t nTopicLen = m_ReceivePacket.GetString (m_pTopicBuffer,
								      m_nMaxTopicSize+1);
			if (nTopicLen == 0)
			{
				CloseConnection (MQTTDisconnectNotSupported);

				break;
			}

			size_t nPayloadLength = m_ReceivePacket.GetRemainingLength () - (nTopicLen+2);

			u16 usPacketIdentifier = 0;
			if (uchQoS > MQTT_QOS_AT_MOST_ONCE)
			{
				usPacketIdentifier = m_ReceivePacket.GetWord ();
				if (   usPacketIdentifier == 0
				    || nPayloadLength < 2)
				{
					CloseConnection (MQTTDisconnectInvalidPacket);

					break;
				}

				nPayloadLength -= 2;
			}

			const u8 *pPayload = m_ReceivePacket.GetData (nPayloadLength);
			m_ReceivePacket.Complete ();

			if (uchQoS == MQTT_QOS_AT_MOST_ONCE)
			{
				OnMessage (m_pTopicBuffer, pPayload, nPayloadLength, bRetain);
			}
			else if (uchQoS == MQTT_QOS_AT_LEAST_ONCE)
			{
				CMQTTSendPacket Packet (MQTTPubAck);
				Packet.AppendWord (usPacketIdentifier);

				if (!SendPacket (&Packet))
				{
					CloseConnection (MQTTDisconnectSendFailed);

					break;
				}

				OnMessage (m_pTopicBuffer, pPayload, nPayloadLength, bRetain);
			}
			else if (uchQoS == MQTT_QOS_EXACTLY_ONCE)
			{
				CMQTTSendPacket Packet (MQTTPubRec);
				Packet.AppendWord (usPacketIdentifier);

				if (!SendPacket (&Packet))
				{
					CloseConnection (MQTTDisconnectSendFailed);

					break;
				}

				if (!IsPacketIdentifierInStore (usPacketIdentifier))
				{
					InsertPacketIdentifierIntoStore (usPacketIdentifier);

					OnMessage (m_pTopicBuffer, pPayload, nPayloadLength, bRetain);
				}
			}
			} break;

		case MQTTPubAck: {
			u16 usPacketIdentifier = m_ReceivePacket.GetWord ();
			m_ReceivePacket.Complete ();

			CMQTTSendPacket *pPacket = RemovePacketFromQueue (usPacketIdentifier);
			if (   pPacket == 0
			    || pPacket->GetQoS () != MQTT_QOS_AT_LEAST_ONCE)
			{
				CloseConnection (MQTTDisconnectPacketIdentifier);
			}

			delete pPacket;
			} break;

		case MQTTPubRec: {
			u16 usPacketIdentifier = m_ReceivePacket.GetWord ();
			m_ReceivePacket.Complete ();

			CMQTTSendPacket *pPacket = RemovePacketFromQueue (usPacketIdentifier);
			if (   pPacket == 0
			    || pPacket->GetQoS () != MQTT_QOS_EXACTLY_ONCE)
			{
				delete pPacket;

				CloseConnection (MQTTDisconnectPacketIdentifier);

				break;
			}

			delete pPacket;

			pPacket = new CMQTTSendPacket (MQTTPubRel);
			assert (pPacket != 0);
			pPacket->AppendWord (usPacketIdentifier);

			if (!SendPacket (pPacket))
			{
				delete pPacket;

				CloseConnection (MQTTDisconnectSendFailed);

				break;
			}

			pPacket->SetQoS (MQTT_QOS_EXACTLY_ONCE);
			pPacket->SetPacketIdentifier (usPacketIdentifier);
			InsertPacketIntoQueue (pPacket, m_pTimer->GetTicks () + MQTT_RESEND_TIMEOUT);
			} break;

		case MQTTPubRel: {
			u16 usPacketIdentifier = m_ReceivePacket.GetWord ();
			m_ReceivePacket.Complete ();

			if (usPacketIdentifier == 0)
			{
				CloseConnection (MQTTDisconnectInvalidPacket);

				break;
			}

			CMQTTSendPacket Packet (MQTTPubComp);
			Packet.AppendWord (usPacketIdentifier);

			if (!SendPacket (&Packet))
			{
				CloseConnection (MQTTDisconnectSendFailed);

				break;
			}

			RemovePacketIdentifierFromStore (usPacketIdentifier);
			} break;

		case MQTTPubComp: {
			u16 usPacketIdentifier = m_ReceivePacket.GetWord ();
			m_ReceivePacket.Complete ();

			CMQTTSendPacket *pPacket = RemovePacketFromQueue (usPacketIdentifier);
			if (   pPacket == 0
			    || pPacket->GetQoS () != MQTT_QOS_EXACTLY_ONCE)
			{
				CloseConnection (MQTTDisconnectPacketIdentifier);
			}

			delete pPacket;
			} break;

		case MQTTSubAck: {
			u16 usPacketIdentifier = m_ReceivePacket.GetWord ();
			u8 uchReturnCode = m_ReceivePacket.GetByte ();
			m_ReceivePacket.Complete ();

			if (uchReturnCode > MQTT_QOS_EXACTLY_ONCE)
			{
				CloseConnection (MQTTDisconnectSubscribeError);

				break;
			}

			CMQTTSendPacket *pPacket = RemovePacketFromQueue (usPacketIdentifier);
			if (   pPacket == 0
			    || pPacket->GetQoS () != MQTT_QOS_AT_LEAST_ONCE)
			{
				CloseConnection (MQTTDisconnectPacketIdentifier);
			}

			delete pPacket;
			} break;

		case MQTTUnsubAck: {
			u16 usPacketIdentifier = m_ReceivePacket.GetWord ();
			m_ReceivePacket.Complete ();

			CMQTTSendPacket *pPacket = RemovePacketFromQueue (usPacketIdentifier);
			if (   pPacket == 0
			    || pPacket->GetQoS () != MQTT_QOS_AT_LEAST_ONCE)
			{
				CloseConnection (MQTTDisconnectPacketIdentifier);
			}

			delete pPacket;
			} break;

		case MQTTPingResp:
			m_ReceivePacket.Complete ();

			m_ConnectStatus = MQTTStatusConnected;

			if (m_nKeepAliveSeconds > 0)
			{
				m_nTimerElapsesAt = m_pTimer->GetTicks () + m_nKeepAliveSeconds*HZ;
				m_bTimerRunning = TRUE;
			}
			break;

		default:
			assert (0);
			break;
		}
	}
}

void CMQTTClient::Sender (void)
{
	unsigned nTicks = m_pTimer->GetTicks ();

	TPtrListElement *pElement = m_RetransmissionQueue.GetFirst ();
	while (pElement != 0)
	{
		CMQTTSendPacket *pPacket = (CMQTTSendPacket *) m_RetransmissionQueue.GetPtr (pElement);
		assert (pPacket != 0);

		// leave if scheduled time is after current time (queue is sorted)
		if ((int) (pPacket->GetScheduledTime () - nTicks) > 0)
		{
			break;
		}

		TPtrListElement *pNextElement = m_RetransmissionQueue.GetNext (pElement);
		m_RetransmissionQueue.Remove (pElement);
		pElement = pNextElement;

		// retransmit packet
		if (pPacket->GetType () == MQTTPublish)
		{
			pPacket->SetFlags (pPacket->GetFlags () | MQTT_FLAG_DUP);
		}

		// SendPacket() fails on too many retries
		if (!SendPacket (pPacket))
		{
			delete pPacket;

			CloseConnection (MQTTDisconnectSendFailed);

			return;
		}

		InsertPacketIntoQueue (pPacket, m_pTimer->GetTicks () + MQTT_RESEND_TIMEOUT);
	}
}

void CMQTTClient::KeepAliveHandler (void)
{
	if (   m_bTimerRunning
	    && (int) (m_nTimerElapsesAt - m_pTimer->GetTicks ()) <= 0)
	{
		m_bTimerRunning = FALSE;

		if (m_ConnectStatus == MQTTStatusConnected)
		{
			CMQTTSendPacket Packet (MQTTPingReq);

			if (!SendPacket (&Packet))
			{
				CloseConnection (MQTTDisconnectSendFailed);
			}
		}
		else
		{
			assert (m_ConnectStatus == MQTTStatusWaitForPingResp);

			CloseConnection (MQTTDisconnectPingFailed);
		}
	}
}

void CMQTTClient::CloseConnection (TMQTTDisconnectReason Reason)
{
	if (m_ConnectStatus == MQTTStatusDisconnected)
	{
		return;
	}

	assert (Reason <= MQTTDisconnectUnknown);
	CLogger::Get ()->Write (FromMQTTClient,
				Reason == MQTTDisconnectFromApplication ? LogDebug : LogError,
				"%s: %s", (const char *) m_IPServerString, s_pErrorMsg[Reason]);

	m_ConnectStatus = MQTTStatusDisconnected;

	m_bTimerRunning = FALSE;
	CleanupQueue ();
	CleanupPacketIdentifierStore ();

	assert (m_pSocket != 0);
	delete m_pSocket;
	m_pSocket = 0;

	OnDisconnect (Reason);
}

boolean CMQTTClient::SendPacket (CMQTTSendPacket *pPacket)
{
	if (m_ConnectStatus == MQTTStatusDisconnected)
	{
		return FALSE;
	}

	assert (pPacket != 0);
	assert (m_pSocket != 0);
	if (!pPacket->Send (m_pSocket))
	{
		return FALSE;
	}

	// keep alive handling
	switch (pPacket->GetType ())
	{
	case MQTTConnect:
	case MQTTPublish:
	case MQTTPubAck:
	case MQTTPubRec:
	case MQTTPubRel:
	case MQTTPubComp:
	case MQTTSubscribe:
	case MQTTUnsubscribe:
		if (   m_ConnectStatus == MQTTStatusConnected
		    && m_nKeepAliveSeconds > 0)
		{
			m_nTimerElapsesAt = m_pTimer->GetTicks () + m_nKeepAliveSeconds*HZ;
			m_bTimerRunning = TRUE;
		}
		break;

	case MQTTPingReq:
		assert (m_nKeepAliveSeconds > 0);
		assert (m_ConnectStatus == MQTTStatusConnected);
		m_ConnectStatus = MQTTStatusWaitForPingResp;
		assert (!m_bTimerRunning);
		m_nTimerElapsesAt = m_pTimer->GetTicks () + MQTT_PING_TIMEOUT;
		m_bTimerRunning = TRUE;
		break;

	case MQTTDisconnect:
		break;

	default:
		assert (0);
		break;
	}

	return TRUE;
}

void CMQTTClient::InsertPacketIntoQueue (CMQTTSendPacket *pPacket, unsigned nScheduledTime)
{
	assert (pPacket != 0);
	pPacket->SetScheduledTime (nScheduledTime);

	TPtrListElement *pPrevElement = 0;
	TPtrListElement *pElement = m_RetransmissionQueue.GetFirst ();
	while (pElement != 0)
	{
		CMQTTSendPacket *pPacket2 = (CMQTTSendPacket *) m_RetransmissionQueue.GetPtr (pElement);
		assert (pPacket2 != 0);

		if ((int) (pPacket2->GetScheduledTime () - nScheduledTime) > 0)
		{
			break;
		}

		pPrevElement = pElement;
		pElement = m_RetransmissionQueue.GetNext (pElement);
	}

	if (pElement != 0)
	{
		m_RetransmissionQueue.InsertBefore (pElement, pPacket);
	}
	else
	{
		m_RetransmissionQueue.InsertAfter (pPrevElement, pPacket);
	}
}

CMQTTSendPacket *CMQTTClient::RemovePacketFromQueue (u16 usPacketIdentifier)
{
	TPtrListElement *pElement = m_RetransmissionQueue.GetFirst ();
	while (pElement != 0)
	{
		CMQTTSendPacket *pPacket = (CMQTTSendPacket *) m_RetransmissionQueue.GetPtr (pElement);
		assert (pPacket != 0);

		if (pPacket->GetPacketIdentifier () == usPacketIdentifier)
		{
			m_RetransmissionQueue.Remove (pElement);

			return pPacket;
		}

		pElement = m_RetransmissionQueue.GetNext (pElement);
	}

	return 0;
}

void CMQTTClient::CleanupQueue (void)
{
	TPtrListElement *pElement = m_RetransmissionQueue.GetFirst ();
	while (pElement != 0)
	{
		CMQTTSendPacket *pPacket = (CMQTTSendPacket *) m_RetransmissionQueue.GetPtr (pElement);
		assert (pPacket != 0);

		TPtrListElement *pNextElement = m_RetransmissionQueue.GetNext (pElement);
		m_RetransmissionQueue.Remove (pElement);
		pElement = pNextElement;

		delete pPacket;
	}
}

void CMQTTClient::InsertPacketIdentifierIntoStore (u16 usPacketIdentifier)
{
	void *pPtr = (void *) (uintptr) usPacketIdentifier;

	TPtrListElement *pFirst = m_PacketIdentifierStore.GetFirst ();
	if (pFirst == 0)
	{
		m_PacketIdentifierStore.InsertAfter (0, pPtr);
	}
	else
	{
		m_PacketIdentifierStore.InsertBefore (pFirst, pPtr);
	}
}

boolean CMQTTClient::IsPacketIdentifierInStore (u16 usPacketIdentifier)
{
	void *pPtr = (void *) (uintptr) usPacketIdentifier;

	return m_PacketIdentifierStore.Find (pPtr) != 0;
}

boolean CMQTTClient::RemovePacketIdentifierFromStore (u16 usPacketIdentifier)
{
	void *pPtr = (void *) (uintptr) usPacketIdentifier;

	TPtrListElement *pElement = m_PacketIdentifierStore.Find (pPtr);
	if (pElement == 0)
	{
		return FALSE;
	}

	m_PacketIdentifierStore.Remove (pElement);

	return TRUE;
}

void CMQTTClient::CleanupPacketIdentifierStore (void)
{
	TPtrListElement *pElement = m_PacketIdentifierStore.GetFirst ();
	while (pElement != 0)
	{
		TPtrListElement *pNextElement = m_PacketIdentifierStore.GetNext (pElement);
		m_PacketIdentifierStore.Remove (pElement);
		pElement = pNextElement;
	}
}
