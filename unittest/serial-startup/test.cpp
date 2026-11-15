// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <cstdio>
#include "pl011.inc"

using boolean = bool;
using u32 = unsigned;
constexpr bool TRUE = true, FALSE = false;
[[maybe_unused]] constexpr unsigned CLOCK_ID_UART = 2, ARM_IRQ_UART = 0, ARM_FIQ_UART = 0;
constexpr unsigned SERIAL_DEVICES = 3, Stride = 0x1000;

struct Port { unsigned mask, raw, delivered, writes; };
static Port ports[SERIAL_DEVICES];
using Handler = void (*) (void *);
static Handler lines[SERIAL_DEVICES];

static void CheckDelivery ()
{
	for (unsigned i = 0; i < SERIAL_DEVICES; ++i)
	{
		Handler handler = lines[RASPPI <= 4 ? 0 : i];
		if (handler && (ports[i].raw & ports[i].mask))
		{
			handler (nullptr);
			assert (!(ports[i].raw & ports[i].mask));
		}
	}
}

static void write32 (unsigned reg, unsigned value)
{
	const unsigned m_nBaseAddress = 0;
	CheckDelivery ();	// a pending IRQ/FIQ may preempt the next write
	Port &port = ports[reg / Stride];
	++port.writes;
	if (reg % Stride == ARM_UART_IMSC)
	{
		port.mask = value;
		port.raw |= value & INT_RX;
	}
	else if (reg % Stride == ARM_UART_ICR) port.raw &= ~value;
	CheckDelivery ();
}

static void PeripheralEntry () {}
static void PeripheralExit () {}
static void DataSyncBarrier () {}
static void DataMemBarrier () {}
struct CMachineInfo
{
	static CMachineInfo *Get () { static CMachineInfo info; return &info; }
	unsigned GetClockRate (unsigned) { return 48000000; }
};
struct CDeviceNameService
{
	static CDeviceNameService *Get () { static CDeviceNameService service; return &service; }
	void AddDevice (const char *, unsigned, void *, bool) {}
	void RemoveDevice (const char *, unsigned, bool) {}
};
using TGPIOMode = int;
constexpr int GPIOModeUnknown = 1, GPIOModeInput = 0;
#define ALT_FUNC(device, pin) 0
struct Pin { void SetMode (int) {} };
struct InterruptSystem
{
	void ConnectIRQ (unsigned irq, Handler h, void *) { assert (!lines[irq]); lines[irq]=h; CheckDelivery (); }
	void ConnectFIQ (unsigned irq, Handler h, void *p) { ConnectIRQ (irq,h,p); }
	void DisconnectIRQ (unsigned irq) { assert (lines[irq]); lines[irq]=nullptr; }
	void DisconnectFIQ () { DisconnectIRQ (0); }
};

struct CSerialDevice
{
	enum TParity {ParityNone, ParityOdd, ParityEven, ParitySpace, ParityMark};
	CSerialDevice (unsigned i, InterruptSystem *controller, bool fiq=false)
	: m_bUseFIQ (fiq), m_nDevice (i), m_nBaseAddress (i*Stride), m_pInterruptSystem (controller)
	{ assert (!s_pThis[i]); s_pThis[i]=this; }
	~CSerialDevice ();
	boolean Initialize (unsigned=400000, unsigned=8, unsigned=1, TParity=ParityNone);
	static void InterruptStub (void *);
	void InterruptHandler ()
	{
		Port &p = ports[m_nDevice];
		if (p.mask & p.raw) { ++p.delivered; p.raw &= ~p.mask; }
	}
	bool m_bValid=true, m_bUseFIQ;
	unsigned m_nDevice, m_nBaseAddress;
	InterruptSystem *m_pInterruptSystem;
	Pin m_TxDPin, m_RxDPin;
	inline static InterruptSystem *s_pInterruptSystem=nullptr;
	inline static unsigned s_nInterruptUseCount=0, s_nInterruptDeviceMask=0;
	inline static CSerialDevice *s_pThis[SERIAL_DEVICES]{};
	inline static bool s_bUseFIQ=false;
	inline static unsigned s_IRQ[SERIAL_DEVICES]={0,1,2};
};

#include "initialize.inc"

int main ()
{
	InterruptSystem controller, other;
	for (unsigned mode=0; mode<(RASPPI <= 4 ? 3U : 2U); ++mode)
	{
		ports[0].mask=ports[0].raw=INT_TX;
		CSerialDevice first (0, mode ? &controller : nullptr, mode==2);
		assert (first.Initialize ());
		assert (!(ports[0].raw & INT_TX));
		if (!mode || RASPPI < 4) continue;
		if (RASPPI == 4)
		{
			CSerialDevice incompatible (2,&controller,mode!=2);
			unsigned writes = ports[2].writes;
			assert (!incompatible.Initialize () && ports[2].writes==writes);
		}
		const unsigned enabled = INT_RX | INT_RT | INT_OE;
		{
			CSerialDevice second (2,&controller,mode==2);
			ports[0].raw |= INT_RX;
			unsigned previous = ports[0].delivered;
			assert (second.Initialize ());
			assert (ports[0].delivered > previous && ports[0].mask == enabled);
			assert (ports[2].mask == enabled);
			ports[2].raw |= INT_RX;	// pending at destructor entry
		}
		{
			CSerialDevice polling (2,nullptr);
			assert (polling.Initialize ());
		}
		{ CSerialDevice unused (2,&controller,mode==2); }
		{
			CSerialDevice rejected (2,&other,mode==2);
			unsigned writes = ports[2].writes;
			assert (!rejected.Initialize () && ports[2].writes==writes);
		}
		assert (CSerialDevice::s_nInterruptUseCount == 1);
		unsigned previous = ports[0].delivered;
		ports[0].raw |= INT_RX;
		CheckDelivery ();
		assert (ports[0].delivered==previous+1 && ports[0].mask==enabled);
	}
	if (RASPPI >= 4)
	{
		for (unsigned mode=0; mode<(RASPPI == 4 ? 2U : 1U); ++mode)
		{
			auto *first = new CSerialDevice (0,&controller,mode!=0);
			assert (first->Initialize ());
			CSerialDevice second (2,&controller,mode!=0);
			assert (second.Initialize ());
			ports[0].raw |= INT_RX;
			delete first;
			unsigned previous = ports[2].delivered;
			ports[2].raw |= INT_RX;
			CheckDelivery ();
			assert (ports[2].delivered==previous+1);
		}
	}
	assert (!CSerialDevice::s_nInterruptUseCount && !lines[0] && !lines[2]);
	std::printf ("PL011 lifecycle passed (RASPPI=%d)\n", RASPPI);
}
