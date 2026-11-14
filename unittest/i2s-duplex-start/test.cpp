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
constexpr unsigned DEFAULT_PRIORITY = 1, DEFAULT_PANIC_PRIORITY = 1;
constexpr unsigned CS_PRIORITY_SHIFT = 4, CS_PANIC_PRIORITY_SHIFT = 8, ARM_IRQ_DMA0 = 16;
constexpr unsigned CS_A_DMAEN = 512, CS_A_TXON = 4, CS_A_RXON = 2, CS_A_RXSEX = 8;
constexpr unsigned DREQ_A_TX__MASK = 255, DREQ_A_RX__MASK = 255 << 8;
constexpr unsigned DREQ_A_TX__SHIFT = 0, DREQ_A_RX__SHIFT = 8;
static unsigned ARM_DMACHAN_CS (unsigned c) { return 32 + c; }
static unsigned ARM_DMACHAN_CONBLK_AD (unsigned c) { return 64 + c; }
static unsigned ARM_DMACHAN_NEXTCONBK (unsigned c) { return 96 + c; }
static uintptr BUS_ADDRESS (uintptr p) { return p; }

struct Hardware
{
	unsigned reg[128] {};
	unsigned allocations = 0, setups = 0, failAllocation = 0, failSetup = 0;
	unsigned starts = 0, irqConnections = 0;
} hw;
static unsigned read32 (unsigned reg) { return hw.reg[reg]; }
static void write32 (unsigned reg, unsigned value)
{
	if (reg >= 32 && reg < 64)
	{
		if (value & CS_ACTIVE) ++hw.starts;
		value &= ~CS_RESET;
	}
	hw.reg[reg] = value;
}
static void PeripheralEntry () {}
static void PeripheralExit () {}
static void CleanAndInvalidateDataCacheRange (uintptr, unsigned) {}
struct CTimer { static void SimpleusDelay (unsigned) {} };
struct CMachineInfo
{
	static CMachineInfo *Get () { static CMachineInfo info; return &info; }
	unsigned AllocateDMAChannel (unsigned)
	{
		++hw.allocations;
		return hw.allocations == hw.failAllocation ? DMA_CHANNEL_MAX+1 : hw.allocations;
	}
};
struct CDMASoundBuffers
{
	enum {StateCreated, StateIdle, StateFailed, StateRunning, StateTerminating};
	using TChunkCompletedHandler = unsigned (boolean, u32 *, unsigned, void *);
	explicit CDMASoundBuffers (bool out) : m_bDirectionOut (out) {}
	bool m_bDirectionOut, m_bIRQConnected = false;
	unsigned m_State = StateCreated, m_nChunkSize = 32, m_nDMAChannel = DMA_CHANNEL_MAX+1;
	unsigned m_nNextBuffer = 0;
	struct TDMAControlBlock { uintptr nNextControlBlockAddress; } blocks[2] {}, *m_pControlBlock[2] {};
	struct IRQ { void ConnectIRQ (unsigned, int, void *) { ++hw.irqConnections; } } irq, *m_pInterruptSystem = &irq;
	struct Lock { void Acquire () {} void Release () {} } m_SpinLock;
	int InterruptStub = 0;
	TChunkCompletedHandler *m_pHandler = nullptr;
	void *m_pParam = nullptr;
	bool SetupDMAControlBlock (unsigned id)
	{
		if (++hw.setups == hw.failSetup) return false;
		m_pControlBlock[id] = &blocks[id];
		return true;
	}
	bool GetNextChunk (bool first) { return first || m_pHandler (TRUE, nullptr, m_nChunkSize, m_pParam); }
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
#include "startup.inc"

int main ()
{
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
		device.m_TXBuffers.m_State = device.m_RXBuffers.m_State = CDMASoundBuffers::StateIdle;
		assert (device.Start ());
		assert (hw.starts == 2*channels && hw.allocations == channels && hw.irqConnections == channels);
	}
	hw = {};
	CDMASoundBuffers output (true);
	CI2SSoundBaseDevice owner;
	assert (output.Start (owner.TXCompletedHandler, &owner)); // PWM uses Start() directly.
	assert (hw.starts == 1 && hw.allocations == 1 && owner.callbacks == 1);
	puts ("PASS duplex allocation failures, single-direction and duplex startup, restart and direct DMA startup");
}
