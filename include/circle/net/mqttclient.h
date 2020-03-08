//
// mqttclient.h
//
// Class interface was inspired by:
//	https://github.com/marvinroger/async-mqtt-client
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
#ifndef _circle_net_mqttclient_h
#define _circle_net_mqttclient_h

#include <circle/sched/task.h>
#include <circle/net/mqtt.h>
#include <circle/net/mqttsendpacket.h>
#include <circle/net/mqttreceivepacket.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/socket.h>
#include <circle/ptrlist.h>
#include <circle/string.h>
#include <circle/timer.h>
#include <circle/types.h>

enum TMQTTDisconnectReason
{
	MQTTDisconnectFromApplication			= 0,

	// CONNECT errors
	MQTTDisconnectUnacceptableProtocolVersion	= 1,
	MQTTDisconnectIdentifierRejected		= 2,
	MQTTDisconnectServerUnavailable			= 3,
	MQTTDisconnectBadUsernameOrPassword		= 4,
	MQTTDisconnectNotAuthorized			= 5,

	// additional errors
	MQTTDisconnectDNSError,
	MQTTDisconnectConnectFailed,
	MQTTDisconnectFromPeer,
	MQTTDisconnectInvalidPacket,
	MQTTDisconnectPacketIdentifier,
	MQTTDisconnectSubscribeError,
	MQTTDisconnectSendFailed,
	MQTTDisconnectPingFailed,
	MQTTDisconnectNotSupported,
	MQTTDisconnectInsufficientResources,

	MQTTDisconnectUnknown
};

enum TMQTTConnectStatus
{
	MQTTStatusDisconnected,
	MQTTStatusConnectPending,
	MQTTStatusConnected,
	MQTTStatusWaitForPingResp,
	MQTTStatusUnknown
};

/// \note See the MQTT v3.1.1 specification for a detailed description of the parameters:\n
///       http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf

/// \warning This implementation does not support multi-byte-characters in strings so far.

class CMQTTClient : public CTask	/// Client for the MQTT IoT protocol
{
public:
	/// \param pNetSubSystem     Pointer to the network subsystem
	/// \param nMaxPacketSize    Maximum allowed size of a MQTT packet sent or received\n
	/// (topic size + payload size + some bytes protocol overhead)
	/// \param nMaxPacketsQueued Maximum number of MQTT packets queue-able on receive\n
	/// If processing a received packet takes longer, further packets have to be queued.
	/// \param nMaxTopicSize     Maximum allowed size of a received topic string
	CMQTTClient (CNetSubSystem *pNetSubSystem,
		     size_t nMaxPacketSize    = 1024,
		     size_t nMaxPacketsQueued = 4,
		     size_t nMaxTopicSize     = 256);

	~CMQTTClient (void);

	/// \return TRUE if an active connection to the MQTT broker extists
	boolean IsConnected (void) const;

	/// \brief Establish connection to MQTT broker
	/// \param pHost              Host name or IP address (dotted string) of MQTT broker
	/// \param usPort             Port number of MQTT broker service (default 1883)
	/// \param pClientIdentifier  Identifier string of this client\n
	/// (if 0 set internally to "raspiNNNNNNNNNNNNNNNN", N = hex digits of the serial number)
	/// \param pUsername          User name for authorization (0 if not required)
	/// \param pPassword          Password for authorization (0 if not required)
	/// \param usKeepAliveSeconds Keep alive period in seconds (default 60)
	/// \param bCleanSession      Should this be a clean MQTT session? (default TRUE)
	/// \param pWillTopic         Topic string for last will message (no last will message if 0)
	/// \param uchWillQoS         QoS setting for last will message (default unused)
	/// \param bWillRetain        Retain parameter for last will message (default unused)
	/// \param pWillPayload       Pointer to the last will message payload (default unused)
	/// \param nWillPayloadLength Length of the last will message payload (default unused)
	void Connect (const char *pHost, u16 usPort = MQTT_PORT, const char *pClientIdentifier = 0,
		      const char *pUsername = 0, const char *pPassword = 0,
		      u16 usKeepAliveSeconds = 60, boolean bCleanSession = TRUE,
		      const char *pWillTopic = 0, u8 uchWillQoS = 0, boolean bWillRetain = FALSE,
		      const u8 *pWillPayload = 0, size_t nWillPayloadLength = 0);

	/// \brief Close connection to MQTT broker
	/// \param bForce Force TCP disconnect only (does not send MQTT DISCONNECT packet)
	void Disconnect (boolean bForce = FALSE);

	/// \brief Subscribe to a MQTT topic
	/// \param pTopic Topic to be subscribed to (may include wildchars)
	/// \param uchQoS Maximum QoS value for receiving messages with this topic (default QoS 2)
	void Subscribe (const char *pTopic, u8 uchQoS = MQTT_QOS2);

	/// \brief Unsubscribe from a MQTT topic
	/// \param pTopic Topic to be unsubscribed from
	void Unsubscribe (const char *pTopic);

	/// \brief Publish MQTT topic
	/// \param pTopic         Topic string of the published message
	/// \param pPayload       Pointer to the message payload (default unused)
	/// \param nPayloadLength Length of the message payload (default 0)
	/// \param uchQoS         QoS value for sending the PUBLISH message (default QoS 1)
	/// \param bRetain        Retain parameter for the message (default FALSE)
	void Publish (const char *pTopic, const u8 *pPayload = 0, size_t nPayloadLength = 0,
		      u8 uchQoS = MQTT_QOS1, boolean bRetain = FALSE);


	/// \brief Callback entered when the connection to the MQTT broker has been established
	/// \param bSessionPresent Was a session already present on the server for this client?
	virtual void OnConnect (boolean bSessionPresent) {}

	/// \brief Callback entered when the connection to the MQTT broker has been closed
	/// \param Reason The reason for closing the connection (see: TMQTTDisconnectReason)
	virtual void OnDisconnect (TMQTTDisconnectReason Reason) {}

	/// \brief Callback entered when a PUBLISH message has been received for a subscribed topic
	/// \param pTopic         Topic of the received message
	/// \param pPayload       Pointer to the payload of the received message
	/// \param nPayloadLength Length of the payload of the received message
	/// \param bRetain        Retain parameter of the received message
	virtual void OnMessage (const char *pTopic,
				const u8 *pPayload, size_t nPayloadLength,
				boolean bRetain) {}

	/// \brief Callback regularly entered from the MQTT client task
	virtual void OnLoop (void) {}

private:
	void Run (void);

	void Receiver (void);
	void Sender (void);
	void KeepAliveHandler (void);

	void CloseConnection (TMQTTDisconnectReason Reason);

	boolean SendPacket (CMQTTSendPacket *pPacket);

	// retransmission queue (for sender)
	void InsertPacketIntoQueue (CMQTTSendPacket *pPacket, unsigned nScheduledTime);
	CMQTTSendPacket *RemovePacketFromQueue (u16 usPacketIdentifier);
	void CleanupQueue (void);

	// packet identifier store (for QoS 2 receiver)
	void InsertPacketIdentifierIntoStore (u16 usPacketIdentifier);
	boolean IsPacketIdentifierInStore (u16 usPacketIdentifier);
	boolean RemovePacketIdentifierFromStore (u16 usPacketIdentifier);
	void CleanupPacketIdentifierStore (void);

private:
	CNetSubSystem *m_pNetSubSystem;
	size_t m_nMaxPacketSize;
	size_t m_nMaxTopicSize;

	char *m_pTopicBuffer;

	CTimer *m_pTimer;

	CSocket *m_pSocket;
	TMQTTConnectStatus m_ConnectStatus;
	CString m_IPServerString;		// IP address for logging

	unsigned m_nKeepAliveSeconds;
	boolean m_bTimerRunning;		// keep alive timer
	unsigned m_nTimerElapsesAt;

	u16 m_usNextPacketIdentifier;

	CMQTTReceivePacket m_ReceivePacket;

	CPtrList m_RetransmissionQueue;		// sorted according to time
	CPtrList m_PacketIdentifierStore;	// for QoS 2 receiving PUBLISH

	static const char *s_pErrorMsg[MQTTDisconnectUnknown+1];
};

#endif
