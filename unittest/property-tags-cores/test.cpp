// SPDX-License-Identifier: GPL-3.0-or-later
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>

using u8 = unsigned char;
using u32 = unsigned int;
using uintptr = unsigned long;
using boolean = bool;
#define TRUE		true
#define FALSE		false
#define PACKED		__attribute__ ((packed))
#define TASK_LEVEL	0
#define BCM_MAILBOX_PROP_OUT		8
#define COHERENT_SLOT_PROP_MAILBOX	0
#define BUS_ADDRESS(addr)		((u32) (addr))

static const u32 Temperature = 61000;
static const u32 ArmClock = 2400000000;

inline void DataSyncBarrier () { std::atomic_thread_fence (std::memory_order_seq_cst); }
inline void DataMemBarrier () { std::atomic_thread_fence (std::memory_order_seq_cst); }

// the property buffer, one page that all cores share
alignas (64) static u8 s_Page[4096];

struct CMemorySystem
{
	static void *GetCoherentPage (unsigned) { return s_Page; }
};

// the firmware: WriteRead() can hold the first caller until a second caller
// arrives or waits for a lock, and can refuse the next request
static struct
{
	std::mutex Mutex;
	std::condition_variable Arrived;
	unsigned nCallers = 0;
	bool bSecondWaits = false;
	bool bHoldFirst = false;
	bool bRefuseNext = false;
}
s_Firmware;

// a spin lock for TASK_LEVEL with ARM_ALLOW_MULTI_CORE, once the cores run
struct CSpinLock
{
	CSpinLock (unsigned) {}

	void Acquire ()
	{
		if (!m_Mutex.try_lock ())
		{
			{
				std::lock_guard<std::mutex> Guard (s_Firmware.Mutex);
				s_Firmware.bSecondWaits = true;
			}
			s_Firmware.Arrived.notify_all ();
			m_Mutex.lock ();
		}
	}

	void Release () { m_Mutex.unlock (); }

	std::mutex m_Mutex;
};

class CBcmMailBox
{
public:
	CBcmMailBox (unsigned, boolean = FALSE) {}
	u32 WriteRead (unsigned nData);
};

#include "header.inc"
#include "source.inc"

// each thread that asks stands for a core

static u32 AskTemperature ()
{
	CBcmPropertyTags Tags;
	TPropertyTagTemperature Tag;
	Tag.nTemperatureId = TEMPERATURE_ID;
	boolean bOK = Tags.GetTag (PROPTAG_GET_TEMPERATURE, &Tag, sizeof Tag, 4);
	return bOK ? Tag.nValue : 0;
}

static u32 AskArmClock ()
{
	CBcmPropertyTags Tags;
	TPropertyTagClockRate Tag;
	Tag.nClockId = CLOCK_ID_ARM;
	boolean bOK = Tags.GetTag (PROPTAG_GET_CLOCK_RATE, &Tag, sizeof Tag, 4);
	return bOK ? Tag.nRate : 0;
}

u32 CBcmMailBox::WriteRead (unsigned nData)
{
	std::unique_lock<std::mutex> Lock (s_Firmware.Mutex);
	unsigned nCaller = ++s_Firmware.nCallers;
	s_Firmware.Arrived.notify_all ();

	if (nCaller == 1 && s_Firmware.bHoldFirst)
	{
		s_Firmware.Arrived.wait_for (Lock, std::chrono::seconds (1),
					     [] { return s_Firmware.nCallers >= 2 || s_Firmware.bSecondWaits; });
	}

	// answer the request that is in the page now
	TPropertyBuffer *pBuffer = (TPropertyBuffer *) s_Page;
	TPropertyTag *pTag = (TPropertyTag *) pBuffer->Tags;
	u32 *pValue = (u32 *) (pTag + 1);
	pValue[1] = pTag->nTagId == PROPTAG_GET_TEMPERATURE ? Temperature : ArmClock;
	pTag->nValueLength = (u32) VALUE_LENGTH_RESPONSE | (u32) (2 * sizeof (u32));
	pBuffer->nCode = s_Firmware.bRefuseNext ? CODE_RESPONSE_FAILURE : CODE_RESPONSE_SUCCESS;
	s_Firmware.bRefuseNext = false;

	return nData;
}

static void Reset (bool bHoldFirst, bool bRefuseNext)
{
	std::lock_guard<std::mutex> Guard (s_Firmware.Mutex);
	s_Firmware.nCallers = 0;
	s_Firmware.bSecondWaits = false;
	s_Firmware.bHoldFirst = bHoldFirst;
	s_Firmware.bRefuseNext = bRefuseNext;
}

// a call that does not return within 10 s fails the test, also when it hangs
static u32 Within (std::future<u32> &Call)
{
	bool bReturned = Call.wait_for (std::chrono::seconds (10)) == std::future_status::ready;
	assert (bReturned);
	return Call.get ();
}

int main ()
{
	// two cores ask at the same time, and each gets its own answer
	Reset (true, false);
	auto First = std::async (std::launch::async, AskTemperature);
	{
		std::unique_lock<std::mutex> Lock (s_Firmware.Mutex);
		s_Firmware.Arrived.wait_for (Lock, std::chrono::seconds (10),
					     [] { return s_Firmware.nCallers >= 1; });
	}
	auto Second = std::async (std::launch::async, AskArmClock);
	assert (Within (First) == Temperature);
	assert (Within (Second) == ArmClock);

	// a request the firmware refuses leaves the buffer to the next caller
	Reset (false, true);
	auto Refused = std::async (std::launch::async, AskTemperature);
	assert (Within (Refused) == 0);
	auto Next = std::async (std::launch::async, AskArmClock);
	assert (Within (Next) == ArmClock);

	printf ("Property tags passed\n");
}
