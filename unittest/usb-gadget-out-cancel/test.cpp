// Exercises production endpoint cancellation and polling with modeled MMIO.
#include <map>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <functional>
#include <iostream>
#define private public
#define protected public
#include <circle/usb/gadget/dwusbgadgetendpoint.h>
#undef private
#undef protected
#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/dwhciregister.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <assert.h>

struct Failure : std::runtime_error
{
	using std::runtime_error::runtime_error;
};
struct Model
{
	std::map<u32, u32> regs;
	unsigned critical = 0, now = 0, nakReads = 0, disableReads = 0, disables = 0, clears = 0;
	bool allowNak = false, allowDisable = true, finishDuringNak = false;
	CDWUSBGadgetEndpoint *endpoint = nullptr;
	void *buffer = nullptr;
} m;
void require (bool ok, const char *s)
{
	if (!ok)
	{
		throw Failure (s);
	}
}
CTimer *CTimer::Get ()
{
	static CTimer t;
	return &t;
}
unsigned CTimer::GetClockTicks ()
{
	return m.now;
}
void CTimer::usDelay (unsigned us)
{
	m.now += us;
}
void EnterCritical (unsigned)
{
	++m.critical;
}
void LeaveCritical ()
{
	require (m.critical, "unbalanced critical section");
	--m.critical;
}
[[noreturn]] void ModelPanic (const char *format, ...)
{
	char s[200];
	va_list args;
	va_start (args, format);
	vsnprintf (s, sizeof s, format, args);
	va_end (args);
	throw Failure (s);
}
extern "C" void assertion_failed (const char *expression, const char *, unsigned)
{
	throw Failure (std::string ("assert: ") + expression);
}
u32 CDWHCIRegister::Read ()
{
	if (address == DWHCI_CORE_INT_STAT && m.nakReads && !--m.nakReads)
	{
		if (m.allowNak)
		{
			m.regs[address] |= DWHCI_CORE_INT_STAT_GLOBAL_OUT_NAK_EFF;
		}
		if (m.finishDuringNak)
		{
			m.regs[DWHCI_DEV_OUT_EP_CTRL (1)] &= ~DWHCI_DEV_EP_CTRL_EP_ENABLE;
			m.regs[DWHCI_DEV_OUT_EP_INT (1)] |= DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE;
		}
	}
	if (address == DWHCI_DEV_OUT_EP_INT (1) && m.disableReads)
	{
		require (m.critical && m.endpoint->IsTransferActive (),
			 "transfer retired before disable");
		require (m.endpoint->m_pTransferBuffer == m.buffer,
			 "DMA buffer changed before disable");
		if (m.allowDisable && !--m.disableReads)
		{
			m.regs[address] |= DWHCI_DEV_OUT_EP_INT_EP_DISABLED;
			m.regs[DWHCI_DEV_OUT_EP_CTRL (1)] &=
			    ~(DWHCI_DEV_EP_CTRL_EP_ENABLE | DWHCI_DEV_EP_CTRL_EP_DISABLE);
		}
	}
	return value = m.regs[address];
}
void CDWHCIRegister::Write ()
{
	if (address == DWHCI_DEV_OUT_EP_INT (1) || address == DWHCI_DEV_IN_EP_INT (1) ||
	    address == DWHCI_CORE_INT_STAT)
	{
		m.regs[address] &= ~value;
		return;
	}
	if (address == DWHCI_DEV_CTRL)
	{
		if (value & DWHCI_DEV_CTRL_SET_GLOBAL_OUT_NAK)
		{
			m.nakReads = 3;
		}
		if (value & DWHCI_DEV_CTRL_CLEAR_GLOBAL_OUT_NAK)
		{
			require (!(m.regs[DWHCI_DEV_OUT_EP_CTRL (1)] & DWHCI_DEV_EP_CTRL_EP_ENABLE),
				 "global NAK cleared before endpoint stopped");
			++m.clears;
			m.regs[DWHCI_CORE_INT_STAT] &= ~DWHCI_CORE_INT_STAT_GLOBAL_OUT_NAK_EFF;
		}
		value &= ~(DWHCI_DEV_CTRL_SET_GLOBAL_OUT_NAK | DWHCI_DEV_CTRL_CLEAR_GLOBAL_OUT_NAK);
	}
	if (address == DWHCI_DEV_OUT_EP_CTRL (1))
	{
		if (value & DWHCI_DEV_EP_CTRL_EP_DISABLE)
		{
			require (value & DWHCI_DEV_EP_CTRL_EP_ENABLE,
				 "EPENA removed from disable command");
			require (
			    !(m.regs[DWHCI_DEV_OUT_EP_INT (1)] & DWHCI_DEV_OUT_EP_INT_EP_DISABLED),
			    "stale disable not cleared");
			++m.disables;
			m.disableReads = 3;
		}
		value &= ~(DWHCI_DEV_EP_CTRL_SET_NAK | DWHCI_DEV_EP_CTRL_CLEAR_NAK);
	}
	m.regs[address] = value;
}
#include "real-gadget.inc"

class CBulkOutTest : public CDWUSBGadgetEndpoint
{
      public:
	static TUSBEndpointDescriptor Descriptor ()
	{
		return {7, 5, 1, 2, 64, 0};
	}
	CBulkOutTest (CDWUSBGadget *g, const TUSBEndpointDescriptor *d)
	    : CDWUSBGadgetEndpoint (d, g)
	{
	}
	u8 buffer[64] = {};
	void OnActivate () override
	{
		BeginTransfer (TransferDataOut, buffer, 64);
		m.endpoint = this;
		m.buffer = m_pTransferBuffer;
	}
	void OnDeactivate () override
	{
		CancelTransfer ();
	}
	void OnTransferComplete (boolean, size_t) override
	{
	}
};
int main ()
{
	unsigned passed = 0, failed = 0;
	auto test = [&] (const char *name, std::function<void ()> run)
	{
		try
		{
			m = Model{};
			run ();
			++passed;
			std::cout << "PASS " << name << '\n';
		}
		catch (const std::exception &e)
		{
			++failed;
			std::cout << "FAIL " << name << ": " << e.what () << '\n';
		}
	};
	for (bool acknowledge : {false, true})
	{
		test (acknowledge ? "normal global NAK cancellation"
				  : "missing global NAK still disables endpoint",
		      [&]
		      {
			      CDWUSBGadget g;
			      auto d = CBulkOutTest::Descriptor ();
			      CBulkOutTest ep (&g, &d);
			      ep.OnActivate ();
			      m.allowNak = acknowledge;
			      m.regs[DWHCI_DEV_OUT_EP_INT (1)] = DWHCI_DEV_OUT_EP_INT_EP_DISABLED |
								 DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE;
			      ep.OnDeactivate ();
			      require (m.disables == 1 && !m.disableReads,
				       "disable did not complete");
			      require (!ep.IsTransferActive () && !m.critical,
				       "cancellation incomplete");
			      require (!m.regs[DWHCI_DEV_OUT_EP_INT (1)], "stale IRQ retained");
			      require (m.clears == 1, "owned global NAK not released");
			      if (!acknowledge)
			      {
				      require (m.now >= 10000 && m.now < 11000,
					       "NAK wait not bounded");
			      }
			      ep.OnActivate ();
			      require (ep.IsTransferActive (), "endpoint cannot restart");
		      });
	}
	test ("failed disable retains DMA ownership despite stale disabled status",
	      []
	      {
		      CDWUSBGadget g;
		      auto d = CBulkOutTest::Descriptor ();
		      CBulkOutTest ep (&g, &d);
		      ep.OnActivate ();
		      m.allowDisable = false;
		      m.regs[DWHCI_DEV_OUT_EP_INT (1)] = DWHCI_DEV_OUT_EP_INT_EP_DISABLED;
		      bool rejected = false;
		      try
		      {
			      ep.OnDeactivate ();
		      }
		      catch (const Failure &e)
		      {
			      rejected = std::string (e.what ()).find ("disable timeout") !=
					 std::string::npos;
		      }
		      require (rejected && ep.IsTransferActive () &&
				   ep.m_pTransferBuffer == m.buffer,
			       "expected disable timeout with DMA ownership retained");
		      require (!m.clears && m.disables == 1, "unsafe failed-disable cleanup");
		      require (m.now >= 20000 && m.now < 21000, "two-stage stop not bounded");
	      });
	test ("completion during missing NAK wait needs no second disable",
	      []
	      {
		      CDWUSBGadget g;
		      auto d = CBulkOutTest::Descriptor ();
		      CBulkOutTest ep (&g, &d);
		      ep.OnActivate ();
		      m.finishDuringNak = true;
		      ep.OnDeactivate ();
		      require (!ep.IsTransferActive () && !m.disables,
			       "completed transfer disabled again");
		      require (m.clears == 1 && !m.regs[DWHCI_DEV_OUT_EP_INT (1)],
			       "completion cleanup missing");
	      });
	test ("pre-existing global NAK is not released",
	      []
	      {
		      CDWUSBGadget g;
		      auto d = CBulkOutTest::Descriptor ();
		      CBulkOutTest ep (&g, &d);
		      ep.OnActivate ();
		      m.regs[DWHCI_CORE_INT_STAT] = DWHCI_CORE_INT_STAT_GLOBAL_OUT_NAK_EFF;
		      ep.OnDeactivate ();
		      require (!ep.IsTransferActive () && m.disables == 1, "cancel failed");
		      require (!m.clears && (m.regs[DWHCI_CORE_INT_STAT] &
					     DWHCI_CORE_INT_STAT_GLOBAL_OUT_NAK_EFF),
			       "unowned global NAK cleared");
	      });
	test ("already stopped endpoint retires without NAK or disable",
	      []
	      {
		      CDWUSBGadget g;
		      auto d = CBulkOutTest::Descriptor ();
		      CBulkOutTest ep (&g, &d);
		      ep.OnActivate ();
		      m.regs[DWHCI_DEV_OUT_EP_CTRL (1)] &= ~DWHCI_DEV_EP_CTRL_EP_ENABLE;
		      m.regs[DWHCI_DEV_OUT_EP_INT (1)] = DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE;
		      ep.OnDeactivate ();
		      require (!ep.IsTransferActive () && !m.disables && !m.clears,
			       "unnecessary stop sequence");
		      require (!m.regs[DWHCI_DEV_OUT_EP_INT (1)], "stale completion retained");
	      });
	std::cout << passed << " passed, " << failed << " failed\n";
	return failed ? 1 : 0;
}
