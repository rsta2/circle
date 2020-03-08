//
// netsocket.h
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
#ifndef _circle_net_netsocket_h
#define _circle_net_netsocket_h

#include <circle/net/ipaddress.h>
#include <circle/types.h>

class CNetSubSystem;

class CNetSocket	/// Base class of networking sockets
{
public:
	/// \param pNetSubSystem Pointer to the network subsystem
	CNetSocket (CNetSubSystem *pNetSubSystem);

	/// \brief Destructor (terminates an active connection)
	virtual ~CNetSocket (void);

	/// \brief Bind own port number to this socket
	/// \param nOwnPort Port number
	/// \return Status (0 success, < 0 on error)
	virtual int Bind (u16 nOwnPort) { return -1; }

	/// \brief Connect to foreign host/port (TCP), setup foreign host/port address (UDP)
	/// \param rForeignIP IP address of host to be connected
	/// \param nForeignPort   Number of port to be connected
	/// \return Status (0 success, < 0 on error)
	virtual int Connect (CIPAddress &rForeignIP, u16 nForeignPort) = 0;

	/// \brief Connect to foreign host/port (TCP), setup foreign host/port address (UDP)
	/// \param pHost Hostname or IP address string of host to be connected
	/// \param pPort Decimal number string of port to be connected
	/// \return Status (0 success, < 0 on error)
	virtual int Connect (const char *pHost, const char *pPort);

	/// \brief Listen for incoming connections (TCP only, must call Bind() before)
	/// \param nBackLog Maximum number of simultaneous connections which may be accepted\n
	/// in a row before Accept() is called
	/// \return Status (0 success, < 0 on error)
	virtual int Listen (unsigned nBackLog)  { return -1; }
	/// \brief Accept an incoming connection (TCP only, must call Listen() before)
	/// \param pForeignIP	IP address of the remote host will be returned here
	/// \param pForeignPort	Remote port number will be returned here
	/// \return Newly created socket to be used to communicate with the remote host (0 on error)
	virtual CNetSocket *Accept (CIPAddress *pForeignIP, u16 *pForeignPort) { return 0; }

	/// \brief Send a message to a remote host
	/// \param pBuffer Pointer to the message
	/// \param nLength Length of the message
	/// \param nFlags  MSG_DONTWAIT (non-blocking operation) or 0 (blocking operation)
	/// \return Length of the sent message (< 0 on error)
	virtual int Send (const void *pBuffer, unsigned nLength, int nFlags) = 0;

	/// \brief Receive a message from a remote host
	/// \param pBuffer Pointer to the message buffer
	/// \param nLength Size of the message buffer in bytes\n
	/// Should be at least FRAME_BUFFER_SIZE, otherwise data may get lost on some sockets
	/// \param nFlags MSG_DONTWAIT (non-blocking operation) or 0 (blocking operation)
	/// \return Length of received message (0 with MSG_DONTWAIT if no message available, < 0 on error)
	virtual int Receive (void *pBuffer, unsigned nLength, int nFlags) = 0;

	/// \brief Send a message to a specific remote host
	/// \param pBuffer	Pointer to the message
	/// \param nLength	Length of the message
	/// \param nFlags	MSG_DONTWAIT (non-blocking operation) or 0 (blocking operation)
	/// \param rForeignIP	IP address of host to be sent to (ignored on TCP socket)
	/// \param nForeignPort	Number of port to be sent to (ignored on TCP socket)
	/// \return Length of the sent message (< 0 on error)
	virtual int SendTo (const void *pBuffer, unsigned nLength, int nFlags,
			    CIPAddress &rForeignIP, u16 nForeignPort) { return -1; }

	/// \brief Receive a message from a remote host, return host/port of remote host
	/// \param pBuffer Pointer to the message buffer
	/// \param nLength Size of the message buffer in bytes\n
	/// Should be at least FRAME_BUFFER_SIZE, otherwise data may get lost on some sockets
	/// \param nFlags MSG_DONTWAIT (non-blocking operation) or 0 (blocking operation)
	/// \param pForeignIP	IP address of host which has sent the message will be returned here
	/// \param pForeignPort	Number of port from which the message has been sent will be returned here
	/// \return Length of received message (0 with MSG_DONTWAIT if no message available, < 0 on error)
	virtual int ReceiveFrom (void *pBuffer, unsigned nLength, int nFlags,
				 CIPAddress *pForeignIP, u16 *pForeignPort) { return -1; }

	/// \brief Call this with bAllowed == TRUE after Bind() or Connect() to be able\n
	/// to send and receive broadcast messages (ignored on TCP socket)
	/// \param bAllowed Sending and receiving broadcast messages allowed on this socket? (default FALSE)
	/// \return Status (0 success, < 0 on error)
	virtual int SetOptionBroadcast (boolean bAllowed) { return -1; }

	/// \brief Get IP address of connected remote host
	/// \return Pointer to IP address (four bytes, 0-pointer if not connected)
	virtual const u8 *GetForeignIP (void) const = 0;

protected:
	CNetSubSystem *GetNetSubSystem (void);

private:
	CNetSubSystem *m_pNetSubSystem;
};

#endif
