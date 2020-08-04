//
// socket.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_net_socket_h
#define _circle_net_socket_h

#include <circle/net/netsocket.h>
#include <circle/net/ipaddress.h>
#include <circle/net/netconfig.h>
#include <circle/net/transportlayer.h>
#include <circle/types.h>

#define SOCKET_MAX_LISTEN_BACKLOG	32

class CNetSubSystem;

class CSocket : public CNetSocket	/// Application programming interface to the TCP/IP network
{
public:
	/// \param pNetSubSystem Pointer to the network subsystem
	/// \param nProtocol	 IPPROTO_TCP or IPPROTO_UDP (include circle/net/in.h)
	CSocket (CNetSubSystem *pNetSubSystem, int nProtocol);

	/// \brief Destructor (terminates an active connection)
	~CSocket (void);

	/// \brief Bind own port number to this socket
	/// \param nOwnPort Port number
	/// \return Status (0 success, < 0 on error)
	int Bind (u16 nOwnPort);

	/// \brief Connect to foreign host/port (TCP), setup foreign host/port address (UDP)
	/// \param rForeignIP IP address of host to be connected
	/// \param nForeignPort Number of port to be connected
	/// \return Status (0 success, < 0 on error)
	int Connect (CIPAddress &rForeignIP, u16 nForeignPort);

	/// \brief Listen for incoming connections (TCP only, must call Bind() before)
	/// \param nBackLog Maximum number of simultaneous connections which may be accepted\n
	/// in a row before Accept() is called (up to SOCKET_MAX_LISTEN_BACKLOG)
	/// \return Status (0 success, < 0 on error)
	int Listen (unsigned nBackLog = 4);
	/// \brief Accept an incoming connection (TCP only, must call Listen() before)
	/// \param pForeignIP	IP address of the remote host will be returned here
	/// \param pForeignPort	Remote port number will be returned here
	/// \return Newly created socket to be used to communicate with the remote host (0 on error)
	CSocket *Accept (CIPAddress *pForeignIP, u16 *pForeignPort);

	/// \brief Send a message to a remote host
	/// \param pBuffer Pointer to the message
	/// \param nLength Length of the message
	/// \param nFlags  MSG_DONTWAIT (non-blocking operation) or 0 (blocking operation)
	/// \return Length of the sent message (< 0 on error)
	int Send (const void *pBuffer, unsigned nLength, int nFlags);

	/// \brief Receive a message from a remote host
	/// \param pBuffer Pointer to the message buffer
	/// \param nLength Size of the message buffer in bytes\n
	/// Should be at least FRAME_BUFFER_SIZE, otherwise data may get lost
	/// \param nFlags MSG_DONTWAIT (non-blocking operation) or 0 (blocking operation)
	/// \return Length of received message (0 with MSG_DONTWAIT if no message available, < 0 on error)
	int Receive (void *pBuffer, unsigned nLength, int nFlags);

	/// \brief Send a message to a specific remote host
	/// \param pBuffer	Pointer to the message
	/// \param nLength	Length of the message
	/// \param nFlags	MSG_DONTWAIT (non-blocking operation) or 0 (blocking operation)
	/// \param rForeignIP	IP address of host to be sent to (ignored on TCP socket)
	/// \param nForeignPort	Number of port to be sent to (ignored on TCP socket)
	/// \return Length of the sent message (< 0 on error)
	int SendTo (const void *pBuffer, unsigned nLength, int nFlags,
		    CIPAddress &rForeignIP, u16 nForeignPort);

	/// \brief Receive a message from a remote host, return host/port of remote host
	/// \param pBuffer Pointer to the message buffer
	/// \param nLength Size of the message buffer in bytes\n
	/// Should be at least FRAME_BUFFER_SIZE, otherwise data may get lost
	/// \param nFlags MSG_DONTWAIT (non-blocking operation) or 0 (blocking operation)
	/// \param pForeignIP	IP address of host which has sent the message will be returned here
	/// \param pForeignPort	Number of port from which the message has been sent will be returned here
	/// \return Length of received message (0 with MSG_DONTWAIT if no message available, < 0 on error)
	int ReceiveFrom (void *pBuffer, unsigned nLength, int nFlags,
			 CIPAddress *pForeignIP, u16 *pForeignPort);

	/// \brief Call this with bAllowed == TRUE after Bind() or Connect() to be able\n
	/// to send and receive broadcast messages (ignored on TCP socket)
	/// \param bAllowed Sending and receiving broadcast messages allowed on this socket? (default FALSE)
	/// \return Status (0 success, < 0 on error)
	int SetOptionBroadcast (boolean bAllowed);

	/// \brief Get IP address of connected remote host
	/// \return Pointer to IP address (four bytes, 0-pointer if not connected)
	const u8 *GetForeignIP (void) const;

private:
	CSocket (CSocket &rSocket, int hConnection);

private:
	CNetConfig	*m_pNetConfig;
	CTransportLayer	*m_pTransportLayer;

	int m_nProtocol;
	u16 m_nOwnPort;
	int m_hConnection;

	unsigned m_nBackLog;
	int m_hListenConnection[SOCKET_MAX_LISTEN_BACKLOG];
};

#endif
