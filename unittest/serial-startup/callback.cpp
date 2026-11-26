// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <cstdio>
#include "pl011.inc"

using u8 = unsigned char;
using u32 = unsigned;
using boolean = bool;
constexpr bool TRUE = true, FALSE = false;
constexpr unsigned m_nBaseAddress = 0, SERIAL_BUF_MASK = 7;
enum {SERIAL_ERROR_BREAK = 1, SERIAL_ERROR_OVERRUN, SERIAL_ERROR_FRAMING, SERIAL_ERROR_PARITY};

static unsigned rx;
static bool pending, locked;

static void PeripheralEntry () {}
static void PeripheralExit () {}
static void write32 (unsigned, unsigned) {}

static unsigned read32 (unsigned reg)
{
	if (reg == ARM_UART_DR)
	{
		assert (pending);
		pending = false;
		return rx;
	}
	if (reg == ARM_UART_FR)
	{
		return FR_TXFF_MASK | (pending ? 0 : FR_RXFE_MASK);
	}
	return 0;
}

static void DeliverPending ();

struct CSerialDevice
{
	using TCharReceivedHandler = void (u8, int, void *);
	void RegisterCharReceivedHandler (TCharReceivedHandler *, void *);
	void InterruptHandler ();

	struct
	{
		void Acquire ()
		{
			assert (!locked);
			locked = true;
		}
		void Release ()
		{
			assert (locked);
			locked = false;
			DeliverPending ();
		}
	} m_SpinLock;
	void *m_pInterruptSystem = this;
	TCharReceivedHandler *m_pCharReceivedHandler = nullptr;
	struct
	{
		void *value = nullptr;
		operator void *() const { return value; }
		// Model a pending IRQ arriving immediately after the context store.
		void operator= (void *p)
		{
			value = p;
			DeliverPending ();
		}
	} m_pParam;

	int m_nRxStatus = 0;
	unsigned m_nRxInPtr = 0, m_nRxOutPtr = 0, m_nTxInPtr = 0, m_nTxOutPtr = 0;
	u8 m_RxBuffer[8] {}, m_TxBuffer[8] {};
	const char *m_pMagic = nullptr, *m_pMagicPtr = nullptr;
	void (*m_pMagicReceivedHandler) () = nullptr;
};

#include "callback.inc"

static CSerialDevice serial;
static unsigned oldCalls, newCalls;

static void DeliverPending ()
{
	if (pending && !locked)
	{
		serial.InterruptHandler ();
	}
}

static void OldHandler (u8 c, int status, void *context)
{
	assert (!locked && context == &oldCalls);
	assert (c == 'x' && status == -SERIAL_ERROR_PARITY);
	++oldCalls;
}

static void NewHandler (u8 c, int status, void *context)
{
	assert (!locked && context == &newCalls);
	assert (c == (newCalls == 0 ? 'y' : 'x') && status == 0);
	++newCalls;
}

static void ReplaceHandler ()
{
	serial.RegisterCharReceivedHandler (NewHandler, &newCalls);
}

static void RemoveHandler ()
{
	serial.RegisterCharReceivedHandler (nullptr, nullptr);
}

int main ()
{
	serial.RegisterCharReceivedHandler (OldHandler, &oldCalls);
	serial.m_pMagic = serial.m_pMagicPtr = "x";
	serial.m_pMagicReceivedHandler = ReplaceHandler;
	rx = 'x' | DR_PE_MASK;
	pending = true;
	DeliverPending ();
	// The magic handler replaces the registration before the byte is dispatched.
	assert (oldCalls == 1 && newCalls == 0);

	rx = 'y';
	pending = true;
	DeliverPending ();
	assert (oldCalls == 1 && newCalls == 1);

	// Removal from the magic handler also leaves the current byte with its owner.
	serial.m_pMagicReceivedHandler = RemoveHandler;
	rx = 'x';
	pending = true;
	DeliverPending ();
	assert (oldCalls == 1 && newCalls == 2);

	// Unregister with a byte pending. Normal buffered reception must resume.
	ReplaceHandler ();
	rx = 'z';
	pending = true;
	RemoveHandler ();
	assert (!pending && oldCalls == 1 && newCalls == 2);
	assert (serial.m_nRxInPtr == 1 && serial.m_RxBuffer[0] == 'z');
	std::puts ("PL011 callback replacement and removal passed");
}
