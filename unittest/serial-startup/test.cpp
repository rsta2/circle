// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <cstdio>
#include "pl011.inc"

using boolean = bool;
using u32 = unsigned;
constexpr bool TRUE = true, FALSE = false;
constexpr unsigned CLOCK_ID_UART = 2;
#if RASPPI <= 4
constexpr unsigned ARM_IRQ_UART = 57, ARM_FIQ_UART = 57;
#endif
constexpr unsigned SERIAL_DEVICES = 2;
static constexpr unsigned m_nBaseAddress = 0;
static unsigned mask, raw, readyMask;
static bool connected;

// Deliver an inherited TX or a newly arrived RX interrupt immediately if enabled.
static void CheckDelivery ()
{
	if (connected && (mask & raw))
	{
		assert (readyMask & 1);
	}
}

static void write32 (unsigned reg, unsigned value)
{
	if (reg == ARM_UART_IMSC)
	{
		mask = value;
		if (mask & INT_RX)
		{
			raw |= INT_RX;
		}
	}
	else if (reg == ARM_UART_ICR)
	{
		raw &= ~value;
	}
	CheckDelivery ();
}

static void PeripheralEntry () {}
static void PeripheralExit () {}
static void DataSyncBarrier () {}

struct CMachineInfo
{
	static CMachineInfo *Get () { static CMachineInfo info; return &info; }
	unsigned GetClockRate (unsigned) { return 48000000; }
};

struct CDeviceNameService
{
	static CDeviceNameService *Get () { static CDeviceNameService service; return &service; }
	void AddDevice (const char *, unsigned, void *, bool) {}
};

struct InterruptSystem
{
	void ConnectIRQ (unsigned, void (*) (void *), void *)
	{
		connected = true;
		CheckDelivery ();
	}

	void ConnectFIQ (unsigned irq, void (*handler) (void *), void *param)
	{
		ConnectIRQ (irq, handler, param);
	}
};

struct CSerialDevice
{
	enum TParity {ParityNone, ParityOdd, ParityEven, ParitySpace, ParityMark};
	boolean Initialize (unsigned, unsigned, unsigned, TParity);
	static void InterruptStub (void *) {}
	bool m_bValid = true, m_bUseFIQ = false;
	unsigned m_nDevice = 0, s_nInterruptUseCount = 0;
	unsigned &s_nInterruptDeviceMask = readyMask;
	InterruptSystem *m_pInterruptSystem = nullptr, *s_pInterruptSystem = nullptr;
	CSerialDevice *s_pThis[SERIAL_DEVICES] {};
	bool s_bUseFIQ = false;
	unsigned s_IRQ[SERIAL_DEVICES] {};
};

#include "initialize.inc"

int main ()
{
	InterruptSystem controller;
	for (unsigned mode = 0; mode < (RASPPI <= 4 ? 3U : 2U); ++mode)
	{
		raw = mask = INT_TX;
		connected = false;
		readyMask = 0;
		CSerialDevice serial;
		serial.m_pInterruptSystem = mode ? &controller : nullptr;
		serial.m_bUseFIQ = mode == 2;
		assert (serial.Initialize (400000, 8, 1, CSerialDevice::ParityNone));
		assert (connected == (mode != 0));
		assert (!(raw & INT_TX));
		assert (mask == (mode ? INT_RX | INT_RT | INT_OE : 0));
	}
	std::printf ("PL011 startup passed (RASPPI=%d)\n", RASPPI);
}
