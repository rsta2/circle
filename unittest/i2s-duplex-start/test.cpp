// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <initializer_list>

using boolean = bool;
using u32 = unsigned;
using uintptr = uintptr_t;
constexpr bool TRUE = true, FALSE = false;
constexpr unsigned TXFR_LEN_MAX_LITE = 65535, DMA_CHANNEL_MAX = 14, DMA_CHANNEL_LITE = 1;
constexpr unsigned ARM_DMA_ENABLE = 1, ARM_DMA_INT_STATUS = 2, ARM_PCM_CS_A = 3, ARM_PCM_DREQ_A = 4;
constexpr unsigned CS_ACTIVE = 1, CS_RESET = 2, CS_INT = 4, CS_WAIT_FOR_OUTSTANDING_WRITES = 8;
constexpr unsigned CLOCKHZ = 1000000;
constexpr unsigned DEFAULT_PRIORITY = 1, DEFAULT_PANIC_PRIORITY = 1;
constexpr unsigned CS_PRIORITY_SHIFT = 4, CS_PANIC_PRIORITY_SHIFT = 8, ARM_IRQ_DMA0 = 16;
constexpr unsigned CS_A_DMAEN = 512, CS_A_TXON = 4, CS_A_RXON = 2, CS_A_RXSEX = 8;
constexpr unsigned DREQ_A_TX__MASK = 255, DREQ_A_RX__MASK = 255 << 8;
constexpr unsigned DREQ_A_TX__SHIFT = 0, DREQ_A_RX__SHIFT = 8;
constexpr unsigned PWM_DMAC = 120, PWM_CTL = 121, CS_ERROR = 1024;
constexpr unsigned ARM_PWM_DMAC_ENAB = 1U << 31, ARM_PWM_DMAC_PANIC__SHIFT = 8, ARM_PWM_DMAC_DREQ__SHIFT = 0;
constexpr unsigned ARM_PWM_CTL_RPTL1 = 4, ARM_PWM_CTL_RPTL2 = 4 << 8;
static unsigned ARM_DMACHAN_CS (unsigned c) { return 32 + c; }
static unsigned ARM_DMACHAN_CONBLK_AD (unsigned c) { return 64 + c; }
static unsigned ARM_DMACHAN_NEXTCONBK (unsigned c) { return 96 + c; }
static u32 BUS_ADDRESS (uintptr p) { return static_cast<u32> (p); }

struct Hardware
{
	unsigned reg[128] {};
	unsigned allocations = 0, setups = 0, failAllocation = 0, failSetup = 0;
	unsigned starts = 0, irqConnections = 0;
	unsigned ticks = 0, busyReads = 0, resetReads = 0;
	unsigned disconnections = 0, freedChannels = 0, freedBlocks = 0;
	bool stuckReset = false;
	unsigned rxChannel = 0, rxBytes = 0;
	uintptr rxBuffer[2] {};
	bool rxClean[2] {};
} hw;
static unsigned read32 (unsigned reg)
{
	if (reg >= 32 && reg < 64 && (hw.reg[reg] & CS_RESET))
	{
		// Fail an unbounded reset loop without hanging the test runner.
		assert (++hw.resetReads <= 20);
		if (!hw.stuckReset && hw.resetReads > hw.busyReads) hw.reg[reg] &= ~CS_RESET;
	}
	return hw.reg[reg];
}
static void write32 (unsigned reg, unsigned value)
{
	if (reg == ARM_DMA_INT_STATUS) value = hw.reg[reg] & ~value;
	if (reg >= 32 && reg < 64)
	{
		if ((value & CS_ACTIVE) && !(hw.reg[reg] & CS_ACTIVE))
		{
			if (reg == ARM_DMACHAN_CS (hw.rxChannel))
			{
				assert (hw.rxClean[0] && hw.rxClean[1]);
				// Require a fresh handoff on the next start.
				hw.rxClean[0] = hw.rxClean[1] = false;
			}
			++hw.starts;
		}
		if (value & CS_RESET) hw.resetReads = 0;
		value &= ~CS_INT;
	}
	hw.reg[reg] = value;
}
static void PeripheralEntry () {}
static void PeripheralExit () {}
static void CleanAndInvalidateDataCacheRange (uintptr address, unsigned bytes)
{
	for (unsigned i = 0; i < 2; ++i)
	{
		if (address == hw.rxBuffer[i])
		{
			assert (bytes == hw.rxBytes);
			hw.rxClean[i] = true;
		}
	}
}
struct CTimer
{
	static void SimpleusDelay (unsigned) {}
	static unsigned GetClockTicks ()
	{
		// Model 100 us of elapsed time per reset poll, including any interrupt delay.
		if (hw.resetReads) hw.ticks += 100;
		return hw.ticks;
	}
};
struct CMachineInfo
{
	static CMachineInfo *Get () { static CMachineInfo info; return &info; }
	void FreeDMAChannel (unsigned) { ++hw.freedChannels; }
	unsigned AllocateDMAChannel (unsigned)
	{
		++hw.allocations;
		return hw.allocations == hw.failAllocation ? DMA_CHANNEL_MAX+1 : hw.allocations;
	}
};
struct CDMASoundBuffers
{
	enum TState {StateCancelled, StateCreated, StateIdle, StateFailed, StateRunning, StateTerminating};
	using TChunkCompletedHandler = unsigned (boolean, u32 *, unsigned, void *);
	explicit CDMASoundBuffers (bool out) : m_bDirectionOut (out) {}
	~CDMASoundBuffers ();
	bool m_bDirectionOut, m_bIRQConnected = false;
	TState m_State = StateCreated;
	unsigned m_nChunkSize = 32, m_nDMAChannel = DMA_CHANNEL_MAX+1;
	unsigned m_nNextBuffer = 0;
	struct TDMAControlBlock
	{
		uintptr nNextControlBlockAddress;
		unsigned nTransferLength;
		~TDMAControlBlock () { ++hw.freedBlocks; }
	} *m_pControlBlock[2] {};
	struct IRQ
	{
		void ConnectIRQ (unsigned, int, void *) { ++hw.irqConnections; }
		void DisconnectIRQ (unsigned) { ++hw.disconnections; }
	} irq, *m_pInterruptSystem = &irq;
	struct Lock { void Acquire () {} void Release () {} } m_SpinLock;
	int InterruptStub = 0;
	TChunkCompletedHandler *m_pHandler = nullptr;
	void *m_pParam = nullptr;
	bool SetupDMAControlBlock (unsigned id)
	{
		if (++hw.setups == hw.failSetup) return false;
		m_pControlBlock[id] = new TDMAControlBlock {};
		m_pDMABuffer[id] = new u32[m_nChunkSize] {};
		if (!m_bDirectionOut)
		{
			hw.rxChannel = m_nDMAChannel;
			hw.rxBuffer[id] = (uintptr) m_pDMABuffer[id];
			hw.rxBytes = m_nChunkSize * sizeof (u32);
		}
		return true;
	}
	u32 *m_pDMABuffer[2] {};
	unsigned m_nNullLevel = 64;
	bool GetNextChunk (bool first);
	bool PutChunk ();
	void InterruptHandler ();
	void Cancel ();
	bool IsActive () const;
	boolean ResetChannel ();
	boolean Prepare ();
	boolean Start (TChunkCompletedHandler *, void *);
};
struct CI2SSoundBaseDevice
{
	enum {DeviceModeTXOnly, DeviceModeRXOnly, Duplex};
	unsigned m_DeviceMode = Duplex, m_nChunkSize = 32, callbacks = 0;
	bool m_bControllerInited = false, m_bError = false;
	CDMASoundBuffers m_TXBuffers {true}, m_RXBuffers {false};
	bool ControllerFactory () { return true; }
	static unsigned TXCompletedHandler (bool, u32 *, unsigned n, void *self)
	{
		++static_cast<CI2SSoundBaseDevice *> (self)->callbacks;
		return n;
	}
	static unsigned RXCompletedHandler (bool, u32 *, unsigned n, void *) { return n; }
	boolean Start ();
};
struct CPWMSoundBaseDevice
{
	CDMASoundBuffers m_DMABuffers {true};
	bool m_bError = false;
	unsigned callbacks = 0;
	unsigned GetChunk (u32 *buffer, unsigned words)
	{
		++callbacks;
		for (unsigned i = 0; i < words; ++i) buffer[i] = callbacks;
		return words;
	}
	static unsigned ChunkCompletedHandler (bool, u32 *, unsigned, void *);
	boolean Start ();
	void Cancel ();
	boolean IsActive () const;
};
#include "startup.inc"

static void CompleteBuffer (CDMASoundBuffers &dma)
{
	hw.reg[ARM_DMA_INT_STATUS] |= 1U << dma.m_nDMAChannel;
	hw.reg[ARM_DMACHAN_CS (dma.m_nDMAChannel)] |= CS_INT;
	if (dma.m_State == CDMASoundBuffers::StateTerminating)
	{
		hw.reg[ARM_DMACHAN_CS (dma.m_nDMAChannel)] &= ~CS_ACTIVE;
	}
	dma.InterruptHandler ();
}

static void TestPWM ()
{
	hw = {};
	CPWMSoundBaseDevice pwm;
	auto &dma = pwm.m_DMABuffers;
	for (unsigned run = 1; run <= 2; ++run)
	{
		unsigned before = pwm.callbacks;
		assert (pwm.Start () && pwm.IsActive ());
		assert (hw.reg[PWM_DMAC] & ARM_PWM_DMAC_ENAB);
		assert (hw.allocations == 1 && hw.irqConnections == 1 && hw.starts == run);
		assert (dma.m_pDMABuffer[0][0] == dma.m_nNullLevel && dma.m_pDMABuffer[1][0] == before+1);
		for (unsigned i = 0; i < 16; ++i) CompleteBuffer (dma);
		assert (pwm.callbacks == before+17);
		pwm.Cancel ();
		CompleteBuffer (dma);
		CompleteBuffer (dma);
		assert (!pwm.IsActive () && pwm.callbacks == before+17);
	}
}

static void TestPWMFailures ()
{
	for (unsigned fault : {1u, 2u, 3u})
	{
		hw = {};
		if (fault == 1) hw.failAllocation = 1;
		else hw.failSetup = fault-1;
		CPWMSoundBaseDevice failed;
		assert (!failed.Start () && !failed.IsActive () && !failed.Start ());
		assert (!hw.starts && !failed.callbacks);
	}
}

static void TestPrepareReset ()
{
	hw = {};
	hw.stuckReset = true;
	unsigned start = hw.ticks = ~0u - 499;
	{
		CDMASoundBuffers dma (true);
		assert (!dma.Prepare () && !dma.Prepare ());
		assert (hw.ticks - start == CLOCKHZ / 1000);
		assert (!hw.irqConnections && !hw.starts && !hw.setups);
		// The hardware recovers before destruction.
		hw.stuckReset = false;
	}
	assert (hw.freedChannels == 1 && !hw.reg[ARM_DMA_ENABLE]);

	// The bit clears between the last busy poll and the timeout check.
	hw = {};
	hw.busyReads = 10;
	{
		CDMASoundBuffers dma (true);
		assert (dma.Prepare () && hw.ticks == CLOCKHZ / 1000);
	}
	assert (hw.freedChannels == 1 && hw.freedBlocks == 2);
}

static void TestTeardownReset ()
{
	hw = {};
	hw.busyReads = 3;
	hw.ticks = ~0u - 199;
	CDMASoundBuffers::TDMAControlBlock *blocks[2];
	u32 *samples[2];
	unsigned start;
	{
		CPWMSoundBaseDevice pwm;
		auto &dma = pwm.m_DMABuffers;
		assert (pwm.Start () && hw.resetReads == 4);
		pwm.Cancel ();
		CompleteBuffer (dma);
		CompleteBuffer (dma);
		for (unsigned i = 0; i < 2; ++i)
		{
			blocks[i] = dma.m_pControlBlock[i];
			samples[i] = dma.m_pDMABuffer[i];
		}
		hw.stuckReset = true;
		start = hw.ticks = ~0u - 499;
	}
	assert (hw.ticks - start == CLOCKHZ / 1000 && hw.disconnections == 1);
	assert (!hw.freedChannels && !hw.freedBlocks && hw.reg[ARM_DMA_ENABLE] == (1u << 1));
	// Only the simulated hardware can release the retained resources now.
	for (unsigned i = 0; i < 2; ++i)
	{
		delete blocks[i];
		delete [] samples[i];
	}
}

int main ()
{
	TestPrepareReset ();
	TestTeardownReset ();
	for (unsigned fault : {2u, 1u, 3u, 4u, 5u, 6u})
	{
		hw = {};
		if (fault <= 2) hw.failAllocation = fault;
		else hw.failSetup = fault - 2;
		CI2SSoundBaseDevice device;
		assert (!device.Start () && device.m_bError);
		assert (!hw.starts && !device.callbacks);
		assert (!(hw.reg[ARM_PCM_CS_A] & (CS_A_DMAEN | CS_A_TXON | CS_A_RXON)));
		assert (!device.Start ());
	}
	for (auto mode : {CI2SSoundBaseDevice::DeviceModeTXOnly, CI2SSoundBaseDevice::DeviceModeRXOnly,
			  CI2SSoundBaseDevice::Duplex})
	{
		hw = {};
		CI2SSoundBaseDevice device;
		device.m_DeviceMode = mode;
		assert (device.Start ());
		unsigned channels = mode == device.Duplex ? 2 : 1;
		assert (hw.starts == channels && hw.allocations == channels && hw.irqConnections == channels);
		assert (bool (hw.reg[ARM_PCM_CS_A] & CS_A_TXON) == (mode != device.DeviceModeRXOnly));
		assert (bool (hw.reg[ARM_PCM_CS_A] & CS_A_RXON) == (mode != device.DeviceModeTXOnly));

		// A completed stop leaves prepared buffers idle for the next Start().
		for (unsigned c = 1; c <= channels; ++c) hw.reg[ARM_DMACHAN_CS (c)] = 0;
		device.m_TXBuffers.m_State = device.m_RXBuffers.m_State = CDMASoundBuffers::StateIdle;
		assert (device.Start ());
		assert (hw.starts == 2*channels && hw.allocations == channels && hw.irqConnections == channels);
		for (auto *dma : {&device.m_TXBuffers, &device.m_RXBuffers})
		{
			if (dma->IsActive ())
			{
				dma->Cancel ();
				CompleteBuffer (*dma);
				CompleteBuffer (*dma);
			}
		}
	}
	TestPWM ();
	assert (hw.freedChannels == 1 && hw.freedBlocks == 2 && hw.disconnections == 1);
	TestPWMFailures ();
	puts ("PASS I2S allocation failures, startup and RX cache handoff on restart; "
	      "PWM priming, refill, cancel, restart and allocation failures; bounded DMA resets");
}
