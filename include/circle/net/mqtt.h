//
// mqtt.h
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
#ifndef _circle_net_mqtt_h
#define _circle_net_mqtt_h

#define MQTT_PORT		1883

#define MQTT_PROTOCOL_LEVEL	4	// MQTT v3.1.1

#define MQTT_QOS0		0
#define MQTT_QOS1		1
#define MQTT_QOS2		2
#define MQTT_QOS_AT_MOST_ONCE	MQTT_QOS0
#define MQTT_QOS_AT_LEAST_ONCE	MQTT_QOS1
#define MQTT_QOS_EXACTLY_ONCE	MQTT_QOS2

#define MQTT_FLAG_RETAIN	(1 << 0)
#define MQTT_FLAG_QOS__SHIFT	1
#define MQTT_FLAG_QOS__MASK	(3 << MQTT_FLAG_QOS__SHIFT)
#define MQTT_FLAG_DUP		(1 << 3)

#define MQTT_CONNECT_FLAG_CLEAN_SESSION		(1 << 1)
#define MQTT_CONNECT_FLAG_WILL			(1 << 2)
#define MQTT_CONNECT_FLAG_WILL_QOS__SHIFT	3
#define MQTT_CONNECT_FLAG_WILL_QOS__MASK	(3 << CONNECT_FLAG_WILL_QOS__SHIFT)
#define MQTT_CONNECT_FLAG_WILL_RETAIN		(1 << 5)
#define MQTT_CONNECT_FLAG_PASSWORD		(1 << 6)
#define MQTT_CONNECT_FLAG_USER_NAME		(1 << 7)

#define MQTT_CONNACK_FLAG_SP			(1 << 0)

enum TMQTTPacketType
{
	MQTTConnect = 1,
	MQTTConnAck,
	MQTTPublish,
	MQTTPubAck,
	MQTTPubRec,
	MQTTPubRel,
	MQTTPubComp,
	MQTTSubscribe,
	MQTTSubAck,
	MQTTUnsubscribe,
	MQTTUnsubAck,
	MQTTPingReq,
	MQTTPingResp,
	MQTTDisconnect,
	MQTTPacketTypeUnknown
};

#define MQTT_SEND_TRIES		5

#define MQTT_RESEND_TIMEOUT	(5*HZ)
#define MQTT_PING_TIMEOUT	(5*HZ)

#endif
