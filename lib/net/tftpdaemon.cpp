//
// tftpdaemon.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/net/tftpdaemon.h>
#include <circle/net/retranstimeoutcalc.h>
#include <circle/net/in.h>
#include <circle/sched/scheduler.h>
#include <circle/logger.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/macros.h>
#include <assert.h>

#define TFTP_PORT		69

#define RECEIVE_TIMEOUT_HZ	(5 * HZ)
#define MAX_TIMEOUT_HZ		(25 * HZ)

struct TTFTPReqPacket
{
	u16	OpCode;
#define OP_CODE_RRQ		1
#define OP_CODE_WRQ		2

#define MAX_FILENAME_LEN	CTFTPDaemon::MaxFilenameLen
#define MAX_MODE_LEN		16
#define MIN_FILENAME_MODE_LEN	(1+1+1+1)
#define MAX_FILENAME_MODE_LEN	(MAX_FILENAME_LEN+1+MAX_MODE_LEN+1)
	char	FileNameMode[MAX_FILENAME_MODE_LEN];
}
PACKED;

struct TTFTPDataPacket
{
	u16	OpCode;
#define OP_CODE_DATA		3

	u16	BlockNumber;
#define MAX_DATA_LEN		512
	u8	Data[MAX_DATA_LEN];
}
PACKED;

struct TTFTPAckPacket
{
	u16	OpCode;
#define OP_CODE_ACK		4

	u16	BlockNumber;
}
PACKED;

struct TTFTPErrorPacket
{
	u16	OpCode;
#define OP_CODE_ERROR		5

	u16	ErrorCode;
#define ERROR_CODE_OTHER	0
#define ERROR_CODE_NO_FILE	1
#define ERROR_CODE_ACCESS	2
#define ERROR_CODE_DISK_FULL	3
#define ERROR_CODE_ILL_OPER	4
#define ERROR_CODE_INV_ID	5
#define ERROR_CODE_EXISTS	6
#define ERROR_CODE_INV_USER	7

#define MAX_ERRMSG_LEN		128
	char	ErrMsg[MAX_ERRMSG_LEN];
}
PACKED;

typedef unsigned TIMER;
#define START_TIMER(timer)		((timer) = CTimer::Get ()->GetTicks ())
#define TIMER_EXPIRED(timer, timeout)	(CTimer::Get ()->GetTicks () - (timer) >= (timeout))

static const char FromTFPTDaemon[] = "tftpd";

CTFTPDaemon::CTFTPDaemon (CNetSubSystem *pNetSubSystem)
:	m_pNetSubSystem (pNetSubSystem),
	m_pRequestSocket (0),
	m_pTransferSocket (0)
{
	SetName (FromTFPTDaemon);
}

CTFTPDaemon::~CTFTPDaemon (void)
{
	delete m_pTransferSocket;
	m_pTransferSocket = 0;

	delete m_pRequestSocket;
	m_pRequestSocket = 0;

	m_pNetSubSystem = 0;
}

void CTFTPDaemon::Run (void)
{
	assert (m_pRequestSocket == 0);
	assert (m_pNetSubSystem != 0);
	m_pRequestSocket = new CSocket (m_pNetSubSystem, IPPROTO_UDP);
	assert (m_pRequestSocket != 0);

	if (m_pRequestSocket->Bind (TFTP_PORT) < 0)
	{
		CLogger::Get ()->Write (FromTFPTDaemon, LogError, "Cannot bind to port %u", TFTP_PORT);

		return;
	}

	while (1)
	{
		UpdateStatus (StatusIdle, nullptr);

		TTFTPReqPacket ReqPacket;
		CIPAddress ForeignIP;
		u16 usForeignPort;

		// filled with 0 and max length is sizeof ReqPacket-2
		// to ensure we get two 0-terminated C-strings in a row
		memset (&ReqPacket, 0, sizeof ReqPacket);
		int nResult = m_pRequestSocket->ReceiveFrom (&ReqPacket, sizeof ReqPacket-2, 0,
							     &ForeignIP, &usForeignPort);
		if (nResult < 0)
		{
			CLogger::Get ()->Write (FromTFPTDaemon, LogError, "Cannot receive request");

			continue;
		}

		int nLength = nResult - sizeof ReqPacket.OpCode;
		if (nLength < MIN_FILENAME_MODE_LEN)
		{
			continue;
		}

		u16 usOpCode = be2le16 (ReqPacket.OpCode);
		if (   usOpCode != OP_CODE_RRQ
		    && usOpCode != OP_CODE_WRQ)
		{
			SendError (ERROR_CODE_ILL_OPER, "Invalid operation", &ForeignIP, usForeignPort);

			continue;
		}

		const char *pFileName = ReqPacket.FileNameMode;
		size_t nNameLen = strlen (pFileName);
		if (!(1 <= nNameLen && nNameLen <= MAX_FILENAME_LEN))
		{
			SendError (ERROR_CODE_OTHER, "Invalid file name", &ForeignIP, usForeignPort);

			continue;
		}

		strcpy (m_Filename, pFileName);

		const char *pMode = pFileName+nNameLen+1;
		if (   strcmp (pMode, "octet") != 0
		    && strcmp (pMode, "Octet") != 0
		    && strcmp (pMode, "OCTET") != 0)
		{
			SendError (ERROR_CODE_OTHER, "Binary mode supported only",
				   &ForeignIP, usForeignPort);

			continue;
		}

		CString IPString;
		ForeignIP.Format (&IPString);
		CLogger::Get ()->Write (FromTFPTDaemon, LogDebug, "Incoming %s request from %s",
					usOpCode == OP_CODE_RRQ ? "read" : "write",
					(const char *) IPString);

		assert (m_pTransferSocket == 0);
		m_pTransferSocket = new CSocket (m_pNetSubSystem, IPPROTO_UDP);
		assert (m_pTransferSocket != 0);

		if (m_pTransferSocket->Connect (ForeignIP, usForeignPort) < 0)
		{
			CLogger::Get ()->Write (FromTFPTDaemon, LogError, "Cannot connect to %s:%u",
						(const char *) IPString, (unsigned) usForeignPort);

			delete m_pTransferSocket;
			m_pTransferSocket = 0;

			continue;
		}

		boolean bOK = FALSE;

		switch (usOpCode)
		{
		case OP_CODE_RRQ:
			bOK = DoRead (pFileName);
			break;

		case OP_CODE_WRQ:
			bOK = DoWrite (pFileName);
			break;

		default:
			assert (0);
			break;
		}

		delete m_pTransferSocket;
		m_pTransferSocket = 0;

		if (bOK)
		{
			CLogger::Get ()->Write (FromTFPTDaemon, LogDebug, "Transfer %s %s completed",
						usOpCode == OP_CODE_RRQ ? "to" : "from",
						(const char *) IPString);
		}
	}
}

boolean CTFTPDaemon::DoRead (const char *pFileName)
{
	assert (m_pTransferSocket != 0);

	assert (pFileName != 0);
	if (!FileOpen (pFileName))
	{
		SendError (ERROR_CODE_NO_FILE, "File not found");

		return FALSE;
	}

	CRetransmissionTimeoutCalculator RTCalc;
	RTCalc.Initialize (0);

	int nDataLength = MAX_DATA_LEN;
	for (u16 usBlockNumber = 1; nDataLength == MAX_DATA_LEN; usBlockNumber++)
	{
		UpdateStatus (StatusReadInProgress, m_Filename);

		TTFTPDataPacket DataPacket;
		DataPacket.OpCode = BE (OP_CODE_DATA);
		DataPacket.BlockNumber = le2be16 (usBlockNumber);

		nDataLength = FileRead (DataPacket.Data, MAX_DATA_LEN);
		if (nDataLength < 0)
		{
			CLogger::Get ()->Write (FromTFPTDaemon, LogError, "Cannot read");

			SendError (ERROR_CODE_OTHER, "Error reading file");

			FileClose ();

			UpdateStatus (StatusReadAborted, m_Filename);

			return FALSE;
		}

		unsigned nPacketLength =   sizeof DataPacket.OpCode
					 + sizeof DataPacket.BlockNumber
					 + nDataLength;

		TIMER TransferTimer;
		START_TIMER (TransferTimer);
		while (!TIMER_EXPIRED (TransferTimer, MAX_TIMEOUT_HZ))
		{
			if (m_pTransferSocket->Send (&DataPacket, nPacketLength, MSG_DONTWAIT) < 0)
			{
				CLogger::Get ()->Write (FromTFPTDaemon, LogError, "Cannot send data");

				FileClose ();

				UpdateStatus (StatusReadAborted, m_Filename);

				return FALSE;
			}

			RTCalc.SegmentSent ((u32) (usBlockNumber-1) * MAX_DATA_LEN, nDataLength);

			TIMER ReceiveTimer;
			START_TIMER (ReceiveTimer);
			while (!TIMER_EXPIRED (ReceiveTimer, RTCalc.GetRTO ()))
			{
				CScheduler::Get ()->Yield ();

				TTFTPAckPacket AckPacket;
				int nResult = m_pTransferSocket->Receive (&AckPacket, sizeof AckPacket,
									  MSG_DONTWAIT);
				if (nResult < 0)
				{
					CLogger::Get ()->Write (FromTFPTDaemon, LogError,
								"Cannot receive ACK");

					FileClose ();

					UpdateStatus (StatusReadAborted, m_Filename);

					return FALSE;
				}

				if (   nResult == sizeof AckPacket
				    || AckPacket.OpCode == BE (OP_CODE_ACK)
				    || AckPacket.BlockNumber == le2be16 (usBlockNumber))
				{
					break;
				}
			}

			if (!TIMER_EXPIRED (ReceiveTimer, RTCalc.GetRTO ()))
			{
				RTCalc.SegmentAcknowledged ((u32) (usBlockNumber-1) * MAX_DATA_LEN);

				break;
			}
			else
			{
				RTCalc.RetransmissionTimerExpired ();
			}
		}

		if (TIMER_EXPIRED (TransferTimer, MAX_TIMEOUT_HZ))
		{
			CLogger::Get ()->Write (FromTFPTDaemon, LogDebug, "Transfer timed out");

			FileClose ();

			UpdateStatus (StatusReadAborted, m_Filename);

			return FALSE;
		}
	}

	FileClose ();

	UpdateStatus (StatusReadCompleted, m_Filename);

	return TRUE;
}

boolean CTFTPDaemon::DoWrite (const char *pFileName)
{
	assert (m_pTransferSocket != 0);

	assert (pFileName != 0);
	if (FileCreate (pFileName))
	{
		TTFTPAckPacket AckPacket;
		AckPacket.OpCode = BE (OP_CODE_ACK);
		AckPacket.BlockNumber = 0;
		if (m_pTransferSocket->Send (&AckPacket, sizeof AckPacket, MSG_DONTWAIT) < 0)
		{
			CLogger::Get ()->Write (FromTFPTDaemon, LogError, "Cannot send ACK");

			FileClose ();

			return FALSE;
		}
	}
	else
	{
		SendError (ERROR_CODE_ACCESS, "Access violation");

		return FALSE;
	}

	// We use a short time-out first, to fall back if the request ACK did not arrive.
	// After the first data packet has been received, use a longer time-out.
	unsigned nTimeout = RECEIVE_TIMEOUT_HZ;

	int nLength = MAX_DATA_LEN;
	for (u16 usBlockNumber = 1; nLength == MAX_DATA_LEN; usBlockNumber++)
	{
		UpdateStatus (StatusWriteInProgress, m_Filename);

		TTFTPDataPacket DataPacket;
		do
		{
			do
			{
				TIMER ReceiveTimer;
				START_TIMER (ReceiveTimer);

				int nResult;
				do
				{
					if (TIMER_EXPIRED (ReceiveTimer, nTimeout))
					{
						CLogger::Get ()->Write (FromTFPTDaemon, LogDebug,
									"Transfer timed out");

						FileClose ();

						UpdateStatus (StatusWriteAborted, m_Filename);

						return FALSE;
					}

					CScheduler::Get ()->Yield ();

					nResult = m_pTransferSocket->Receive (&DataPacket,
									      sizeof DataPacket,
									      MSG_DONTWAIT);
					if (nResult < 0)
					{
						CLogger::Get ()->Write (FromTFPTDaemon, LogError,
									"Cannot receive data");

						FileClose ();

						UpdateStatus (StatusWriteAborted, m_Filename);

						return FALSE;
					}
				}
				while (nResult == 0);

				nLength = nResult - (  sizeof DataPacket.OpCode
						     + sizeof DataPacket.BlockNumber);
			}
			while (   nLength < 0
			       || DataPacket.OpCode != BE (OP_CODE_DATA));

			if ((int) (usBlockNumber - be2le16 (DataPacket.BlockNumber)) >= 0)
			{
				TTFTPAckPacket AckPacket;
				AckPacket.OpCode = BE (OP_CODE_ACK);
				AckPacket.BlockNumber = DataPacket.BlockNumber;
				if (m_pTransferSocket->Send (&AckPacket, sizeof AckPacket,
							     MSG_DONTWAIT) < 0)
				{
					CLogger::Get ()->Write (FromTFPTDaemon, LogError,
								"Cannot send ACK");

					FileClose ();

					UpdateStatus (StatusWriteAborted, m_Filename);

					return FALSE;
				}
			}
		}
		while (DataPacket.BlockNumber != le2be16 (usBlockNumber));

		if (nLength > 0)
		{
			if (FileWrite (DataPacket.Data, (unsigned) nLength) != nLength)
			{
				CLogger::Get ()->Write (FromTFPTDaemon, LogError, "Cannot write");

				SendError (ERROR_CODE_DISK_FULL, "Disk full");

				FileClose ();

				UpdateStatus (StatusWriteAborted, m_Filename);

				return FALSE;
			}
		}

		nTimeout = MAX_TIMEOUT_HZ;
	}

	FileClose ();

	UpdateStatus (StatusWriteCompleted, m_Filename);

	return TRUE;
}

void CTFTPDaemon::SendError (u16 usErrorCode, const char *pErrorMessage, CIPAddress *pSendTo, u16 usPort)
{
	TTFTPErrorPacket ErrorPacket;
	ErrorPacket.OpCode = BE (OP_CODE_ERROR);
	ErrorPacket.ErrorCode = le2be16 (usErrorCode);
	strcpy (ErrorPacket.ErrMsg, pErrorMessage);

	if (pSendTo != 0)
	{
		assert (m_pRequestSocket != 0);
		assert (usPort != 0);
		m_pRequestSocket->SendTo (&ErrorPacket, sizeof ErrorPacket, MSG_DONTWAIT, *pSendTo, usPort);
	}
	else
	{
		assert (m_pTransferSocket != 0);
		m_pTransferSocket->Send (&ErrorPacket, sizeof ErrorPacket, MSG_DONTWAIT);
	}
}
