// SPDX-License-Identifier: GPL-3.0-or-later
#include <assert.h>
#include <stdio.h>
#include <string.h>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using boolean = bool;
#define TRUE		true
#define FALSE		false
#define HZ		100
#define IPPROTO_UDP	17
#define MSG_DONTWAIT	0x40

enum TLogSeverity {LogPanic, LogError, LogWarning, LogNotice, LogDebug};

struct CLogger
{
	static CLogger *Get () { static CLogger Logger; return &Logger; }
	void Write (const char *, TLogSeverity, const char *, ...) {}
};

struct CSpinLock
{
	void Acquire () {}
	void Release () {}
};

typedef boolean TUpdateTimeHandler (unsigned nNewTime, unsigned nOldTime);

class CTimer
{
public:
	boolean SetTimeZone (int nMinutesDiff);
	int GetTimeZone (void) const;
	boolean SetTime (unsigned nTime, boolean bLocal = TRUE);
	unsigned GetTicks (void) const;
	unsigned GetUptime (void) const;
	boolean GetUptime (unsigned *pSeconds, unsigned *pMicroSeconds);
	unsigned GetTime (void) const;
	boolean GetLocalTime (unsigned *pSeconds, unsigned *pMicroSeconds);
	unsigned GetUniversalTime (void) const;
	boolean GetUniversalTime (unsigned *pSeconds, unsigned *pMicroSeconds);

private:
	TUpdateTimeHandler *m_pUpdateTimeHandler = 0;
	volatile unsigned m_nTicks = 0;
	volatile unsigned m_nUptime = 0;
	volatile unsigned m_nTime = 0;
	CSpinLock m_TimeSpinLock;
	int m_nMinutesDiff = 0;
};

#include "timer.inc"

struct CNetSubSystem {};
struct CIPAddress {};

struct CScheduler
{
	static CScheduler *Get () { static CScheduler Scheduler; return &Scheduler; }
	void MsSleep (unsigned) {}
};

// the server's reply, as each case sets it
static u8 s_Reply[48];
static int s_nReplyLength;

class CSocket
{
public:
	CSocket (CNetSubSystem *, int) {}
	int Connect (CIPAddress &, u16) { return 0; }
	int Send (const void *, unsigned nLength, int) { return nLength; }
	int Receive (void *pBuffer, unsigned, int) { memcpy (pBuffer, s_Reply, s_nReplyLength); return s_nReplyLength; }
};

#include "ntpclient.h.inc"
#include "ntpclient.inc"

static const unsigned After2038 = 0x80000000U;
static const unsigned Era0Date = 3976214400U;		// 2026-01-01 in NTP seconds
static const unsigned Era0DateUnix = 1767225600U;

// the time the client takes from a reply with this Transmit Timestamp and length
static unsigned Ask (unsigned nSeconds, unsigned nFraction = 0x80000000U, int nLength = 48)
{
	memset (s_Reply, 0, sizeof s_Reply);
	for (unsigned i = 0; i < 4; i++)
	{
		s_Reply[40 + i] = nSeconds >> (24 - 8 * i);
		s_Reply[44 + i] = nFraction >> (24 - 8 * i);
	}
	s_nReplyLength = nLength;

	CNetSubSystem Net;
	CNTPClient Client (&Net);
	CIPAddress ServerIP;
	return Client.GetTime (ServerIP);
}

int main ()
{
	CTimer Timer;
	unsigned nSeconds, nMicroSeconds;

	// UTC after 2038 and up to 2106
	assert (Timer.SetTime (After2038, FALSE));
	assert (Timer.GetUniversalTime () == After2038);
	assert (Timer.GetUniversalTime (&nSeconds, &nMicroSeconds) && nSeconds == After2038);
	assert (Timer.SetTime (0xFFFFFFFFU, FALSE));
	assert (Timer.GetUniversalTime () == 0xFFFFFFFFU);

	// one hour east of UTC
	assert (Timer.SetTimeZone (60));
	assert (Timer.SetTime (After2038, FALSE));
	assert (Timer.GetTime () == After2038 + 3600 && Timer.GetUniversalTime () == After2038);

	// one hour west of UTC
	assert (Timer.SetTimeZone (-60));
	assert (Timer.SetTime (After2038, FALSE));
	assert (Timer.GetTime () == After2038 - 3600 && Timer.GetUniversalTime () == After2038);
	assert (!Timer.SetTime (1000, FALSE));				// local time would lie before 1970

	// NTP era 0, its last second, the rollover in 2036, and 2038
	assert (Ask (Era0Date) == Era0DateUnix);
	assert (Ask (0xFFFFFFFFU) == 2085978495U);
	assert (Ask (0, 1) == 2085978496U);
	assert (Ask (61505152U) == After2038);

	// a reply without a Transmit Timestamp, or cut off before its end, is refused
	assert (Ask (0, 0) == 0);
	assert (Ask (Era0Date, 0x80000000U, 44) == 0);

	printf ("Time after 2038 passed\n");
}
