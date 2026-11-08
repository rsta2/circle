// Runs production suspend/resume, reset, endpoint and polling code. Models W1C
// status, NAK taking effect, delayed disable and FIFO flush completion.
#include <map>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <iostream>
#define private public
#define protected public
#include <circle/usb/gadget/dwusbgadgetendpoint0.h>
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
	unsigned critical = 0, now = 0, nakReads = 0, disableReads = 0, flushReads = 0;
	unsigned disables = 0, outDisables = 0, flushes = 0;
	bool allowNak = true, allowDisable = true, allowFlush = true, finishDuringNak = false;
	bool unsafeDisable = false, unsafeFlush = false, unsafeRearm = false;
	unsigned setupSizeReads = 0;
	bool changeSetupSize = false;
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
	if (address == DWHCI_DEV_OUT_EP_XFER_SIZ (0) && m.changeSetupSize && ++m.setupSizeReads > 1)
	{
		m.regs[address] = 120;
	}

	if (address == DWHCI_DEV_IN_EP_INT (0))
	{
		if (m.nakReads && m.allowNak && !--m.nakReads)
		{
			m.regs[address] |= DWHCI_DEV_IN_EP_INT_IN_EP_NAK_EFF;
			if (m.finishDuringNak)
			{
				m.regs[DWHCI_DEV_IN_EP_CTRL (0)] &= ~DWHCI_DEV_EP_CTRL_EP_ENABLE;
				m.regs[address] |= DWHCI_DEV_IN_EP_INT_XFER_COMPLETE;
			}
		}
		if (m.disableReads && m.allowDisable && !--m.disableReads)
		{
			m.regs[address] |= DWHCI_DEV_IN_EP_INT_EP_DISABLED;
			m.regs[DWHCI_DEV_IN_EP_CTRL (0)] &=
			    ~(DWHCI_DEV_EP_CTRL_EP_ENABLE | DWHCI_DEV_EP_CTRL_EP_DISABLE);
		}
	}
	if (address == DWHCI_CORE_RESET && m.flushReads && m.allowFlush && !--m.flushReads)
	{
		m.regs[address] &= ~DWHCI_CORE_RESET_TX_FIFO_FLUSH;
	}
	return value = m.regs[address];
}
void CDWHCIRegister::Write ()
{

	for (unsigned ep = 0; ep < 8; ++ep)
	{
		if (address == DWHCI_DEV_IN_EP_INT (ep) || address == DWHCI_DEV_OUT_EP_INT (ep))
		{
			m.regs[address] &= ~value;
			return;
		}
	}
	if (address == DWHCI_DEV_OUT_EP_CTRL (0))
	{
		if (value & DWHCI_DEV_EP_CTRL_EP_DISABLE)
		{
			++m.outDisables;
		}
		if (value & DWHCI_DEV_EP_CTRL_CLEAR_NAK)
		{
			if (m.disableReads || m.regs[DWHCI_DEV_IN_EP_INT (0)] ||
			    m.regs[DWHCI_DEV_OUT_EP_INT (0)])
			{
				m.unsafeRearm = true;
			}
		}
	}
	if (address == DWHCI_DEV_IN_EP_CTRL (0))
	{
		if (value & DWHCI_DEV_EP_CTRL_SET_NAK)
		{
			m.nakReads = 2;
		}
		if (value & DWHCI_DEV_EP_CTRL_EP_DISABLE)
		{
			++m.disables;
			if (!(value & DWHCI_DEV_EP_CTRL_EP_ENABLE) ||
			    !(m.regs[DWHCI_DEV_IN_EP_INT (0)] & DWHCI_DEV_IN_EP_INT_IN_EP_NAK_EFF))
			{
				m.unsafeDisable = true;
			}
			m.disableReads = 3;
		}
	}
	if (address == DWHCI_CORE_RESET && (value & DWHCI_CORE_RESET_TX_FIFO_FLUSH))
	{
		if ((m.regs[DWHCI_DEV_IN_EP_CTRL (0)] & DWHCI_DEV_EP_CTRL_EP_ENABLE) ||
		    m.disableReads)
		{
			m.unsafeFlush = true;
		}
		++m.flushes;
		m.flushReads = 2;
	}
	if (address == DWHCI_CORE_INT_STAT)
	{
		m.regs[address] &= ~value;
		return;
	}
	// NAK command bits self-clear; EPENA/EPDIS remain until hardware stops.
	if (address == DWHCI_DEV_IN_EP_CTRL (0) || address == DWHCI_DEV_OUT_EP_CTRL (0))
	{
		value &= ~(DWHCI_DEV_EP_CTRL_SET_NAK | DWHCI_DEV_EP_CTRL_CLEAR_NAK);
	}
	m.regs[address] = value;
}
#include "real-gadget.inc"

void init ()
{
	m = Model{};
	m.regs[DWHCI_DEV_IN_EP_COMMON_INT_MASK] = ~0U;
	m.regs[DWHCI_DEV_OUT_EP_COMMON_INT_MASK] = ~0U;
}
void setup (CDWUSBGadgetEndpoint0 &ep, u8 request, bool both = false, unsigned outLength = 0)
{
	TSetupData s{};
	s.bmRequestType = request == GET_DESCRIPTOR ? 0x80 : 0;
	s.bRequest = request;
	s.wValue = request == GET_DESCRIPTOR ? 0x100 : 1;
	s.wLength = request == GET_DESCRIPTOR ? 274 : 0;
	if (outLength)
	{
		s.bmRequestType = 0x21;
		s.wLength = outLength;
	}
	std::memcpy (ep.m_OutBuffer, &s, sizeof s);
	m.regs[DWHCI_DEV_OUT_EP_CTRL (0)] &= ~DWHCI_DEV_EP_CTRL_EP_ENABLE;
	m.regs[DWHCI_DEV_OUT_EP_XFER_SIZ (0)] = 0;
	m.regs[DWHCI_DEV_OUT_EP_INT (0)] |=
	    DWHCI_DEV_OUT_EP_INT_SETUP_DONE | (both ? DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE : 0);
	ep.HandleOutInterrupt ();
}
void complete (CDWUSBGadgetEndpoint0 &ep, bool in)
{
	m.regs[in ? DWHCI_DEV_IN_EP_CTRL (0) : DWHCI_DEV_OUT_EP_CTRL (0)] &=
	    ~DWHCI_DEV_EP_CTRL_EP_ENABLE;
	m.regs[in ? DWHCI_DEV_IN_EP_XFER_SIZ (0) : DWHCI_DEV_OUT_EP_XFER_SIZ (0)] = 0;
	m.regs[in ? DWHCI_DEV_IN_EP_INT (0) : DWHCI_DEV_OUT_EP_INT (0)] |=
	    DWHCI_DEV_IN_EP_INT_XFER_COMPLETE;
	if (in)
	{
		ep.HandleInInterrupt ();
	}
	else
	{
		ep.HandleOutInterrupt ();
	}
}
class CCountingEndpoint : public CDWUSBGadgetEndpoint
{
      public:
	CCountingEndpoint (CDWUSBGadget *g, const TUSBEndpointDescriptor *d)
	    : CDWUSBGadgetEndpoint (d, g)
	{
	}
	unsigned suspends = 0, in = 0, out = 0;
	void OnActivate () override
	{
	}
	void OnDeactivate () override
	{
	}
	void OnTransferComplete (boolean, size_t) override
	{
	}
	void OnSuspend () override
	{
		++suspends;
	}
	void HandleInInterrupt () override
	{
		++in;
		m.regs[DWHCI_DEV_IN_EP_INT (1)] = 0;
	}
	void HandleOutInterrupt () override
	{
		++out;
		m.regs[DWHCI_DEV_OUT_EP_INT (1)] = 0;
	}
};
int main ()
{
	unsigned passed = 0, failed = 0;
	auto test = [&] (const char *name, std::function<void ()> run)
	{
		try
		{
			init ();
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
	test ("wake interrupt enabled and pending status cleared",
	      []
	      {
		      CDWUSBGadget g;
		      m.regs[DWHCI_CORE_INT_STAT] = ~0U;
		      g.EnableDeviceInterrupts ();
		      require (m.regs[DWHCI_CORE_INT_MASK] & DWHCI_CORE_INT_MASK_WKUP_INTR,
			       "wake masked");
		      require (!m.regs[DWHCI_CORE_INT_STAT], "pending status not cleared");
	      });
	for (auto state : {CDWUSBGadget::StateResetDone, CDWUSBGadget::StateEnumDone,
			   CDWUSBGadget::StateConfigured})
	{
		test (("restore pre-suspend state " + std::to_string (state)).c_str (),
		      [&]
		      {
			      CDWUSBGadget g;
			      g.retain = true;
			      g.m_State = state;
			      CDWUSBGadgetEndpoint0 ep (64, &g);
			      ep.OnActivate ();
			      auto buffer = ep.m_pTransferBuffer;
			      auto length = ep.m_nTransferLength;
			      g.HandleUSBSuspend ();
			      g.HandleUSBSuspend ();
			      require (g.IsBusSuspended () && g.suspends == 1,
				       "suspend callback not idempotent");
			      require (ep.m_pTransferBuffer == buffer &&
					   ep.m_nTransferLength == length,
				       "transfer changed on suspend");
			      g.HandleUSBResume ();
			      g.HandleUSBResume ();
			      require (g.m_State == state && !g.IsBusSuspended () && g.resumes == 1,
				       "state or resume callback incorrect");
		      });
	}
	for (unsigned phase = 0; phase < 4; ++phase)
	{
		test (("retain EP0 control phase " + std::to_string (phase)).c_str (),
		      [&]
		      {
			      CDWUSBGadget g;
			      g.retain = true;
			      g.m_State = CDWUSBGadget::StateConfigured;
			      CDWUSBGadgetEndpoint0 ep (64, &g);
			      ep.OnActivate ();
			      if (phase == 0)
			      {
				      setup (ep, GET_DESCRIPTOR, true);
			      }
			      if (phase == 1)
			      {
				      setup (ep, GET_DESCRIPTOR, true);
				      complete (ep, true);
			      }
			      if (phase == 2)
			      {
				      setup (ep, 0x20, true, 7);
			      }
			      if (phase == 3)
			      {
				      setup (ep, SET_ADDRESS, true);
			      }
			      auto state = ep.m_State;
			      auto buffer = ep.m_pTransferBuffer;
			      auto length = ep.m_nTransferLength;
			      g.HandleUSBSuspend ();
			      g.HandleUSBResume ();
			      require (ep.m_State == state && ep.m_pTransferBuffer == buffer &&
					   ep.m_nTransferLength == length,
				       "control phase lost");
			      require (g.suspends == 1 && g.resumes == 1, "bus callbacks missing");
			      complete (ep, phase == 0 || phase == 3);
		      });
	}
	test ("reset wins over coalesced wake and suspend",
	      []
	      {
		      CDWUSBGadget g;
		      g.retain = true;
		      g.m_State = CDWUSBGadget::StateConfigured;
		      CDWUSBGadgetEndpoint0 ep (64, &g);
		      ep.OnActivate ();
		      g.HandleUSBSuspend ();
		      m.regs[DWHCI_CORE_INT_STAT] = DWHCI_CORE_INT_MASK_USB_RESET_INTR |
						    DWHCI_CORE_INT_MASK_WKUP_INTR |
						    DWHCI_CORE_INT_MASK_USB_SUSPEND;
		      m.regs[DWHCI_CORE_INT_MASK] = ~0U;
		      g.InterruptHandler ();
		      require (g.m_State == CDWUSBGadget::StateResetDone && !g.IsBusSuspended () &&
				   !g.resumes,
			       "old configuration resurrected");
		      require (!m.regs[DWHCI_CORE_INT_STAT], "coalesced lifecycle IRQ retained");
		      g.HandleUSBResume ();
		      require (!g.resumes && g.m_State == CDWUSBGadget::StateResetDone,
			       "late wake revived configuration");
	      });
	test ("wake wins over stale suspend",
	      []
	      {
		      CDWUSBGadget g;
		      g.retain = true;
		      g.m_State = CDWUSBGadget::StateConfigured;
		      g.HandleUSBSuspend ();
		      m.regs[DWHCI_CORE_INT_STAT] =
			  DWHCI_CORE_INT_MASK_WKUP_INTR | DWHCI_CORE_INT_MASK_USB_SUSPEND;
		      m.regs[DWHCI_CORE_INT_MASK] = ~0U;
		      g.InterruptHandler ();
		      require (g.m_State == CDWUSBGadget::StateConfigured && g.resumes == 1,
			       "wake lost");
		      require (!m.regs[DWHCI_CORE_INT_STAT], "wake/suspend not acknowledged");
	      });
	for (bool incoming : {false, true})
	{
		test (incoming ? "retained OUT completion dispatched while suspended"
			       : "retained IN completion dispatched while suspended",
		      [&]
		      {
			      CDWUSBGadget g;
			      g.retain = true;
			      g.m_State = CDWUSBGadget::StateConfigured;
			      TUSBEndpointDescriptor d = {7, 5, 1, 2, 64, 0};
			      CCountingEndpoint ep (&g, &d);
			      g.HandleUSBSuspend ();
			      unsigned bit = incoming ? (1u << 17) : 2u;
			      m.regs[DWHCI_DEV_ALL_EPS_INT_STAT] =
				  m.regs[DWHCI_DEV_ALL_EPS_INT_MASK] = bit;
			      if (incoming)
			      {
				      g.HandleOutEPInterrupt ();
			      }
			      else
			      {
				      g.HandleInEPInterrupt ();
			      }
			      require ((incoming ? ep.out : ep.in) == 1 && !ep.suspends,
				       "completion dropped or endpoint suspended");
		      });
	}
	test ("EP0 IN completion retires ownership while suspended",
	      []
	      {
		      CDWUSBGadget g;
		      g.retain = true;
		      g.m_State = CDWUSBGadget::StateConfigured;
		      CDWUSBGadgetEndpoint0 ep (64, &g);
		      ep.OnActivate ();
		      setup (ep, SET_ADDRESS);
		      g.HandleUSBSuspend ();
		      m.regs[DWHCI_DEV_IN_EP_CTRL (0)] &= ~DWHCI_DEV_EP_CTRL_EP_ENABLE;
		      m.regs[DWHCI_DEV_IN_EP_XFER_SIZ (0)] = 0;
		      m.regs[DWHCI_DEV_IN_EP_INT (0)] = DWHCI_DEV_IN_EP_INT_XFER_COMPLETE;
		      m.regs[DWHCI_DEV_ALL_EPS_INT_STAT] = m.regs[DWHCI_DEV_ALL_EPS_INT_MASK] = 1;
		      g.HandleInEPInterrupt ();
		      require (ep.m_State == CDWUSBGadgetEndpoint0::StateIdle,
			       "completed status phase lost");
		      g.HandleUSBResume ();
		      require (g.resumes == 1, "resume missing");
	      });
	test ("legacy policy still notifies endpoints and waits for reset",
	      []
	      {
		      CDWUSBGadget g;
		      g.m_State = CDWUSBGadget::StateConfigured;
		      TUSBEndpointDescriptor d = {7, 5, 1, 2, 64, 0};
		      CCountingEndpoint ep (&g, &d);
		      g.HandleUSBSuspend ();
		      g.HandleUSBSuspend ();
		      g.HandleUSBResume ();
		      require (ep.suspends == 1 && !g.suspends && !g.resumes,
			       "legacy callback contract changed");
		      require (g.m_State == CDWUSBGadget::StateSuspended && !g.IsBusSuspended (),
			       "legacy policy restored stale transfers");
		      m.regs[DWHCI_DEV_ALL_EPS_INT_STAT] = m.regs[DWHCI_DEV_ALL_EPS_INT_MASK] =
			  2 | (1u << 17);
		      m.regs[DWHCI_DEV_IN_EP_INT (1)] = DWHCI_DEV_IN_EP_INT_XFER_COMPLETE;
		      m.regs[DWHCI_DEV_OUT_EP_INT (1)] = DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE;
		      g.HandleInEPInterrupt ();
		      g.HandleOutEPInterrupt ();
		      require (!ep.in && !ep.out && !m.regs[DWHCI_DEV_IN_EP_INT (1)] &&
				   !m.regs[DWHCI_DEV_OUT_EP_INT (1)],
			       "legacy completion policy changed");
	      });
	test ("powered state and unsolicited wake do not invoke callbacks",
	      []
	      {
		      CDWUSBGadget g;
		      g.retain = true;
		      g.HandleUSBSuspend ();
		      g.HandleUSBResume ();
		      require (g.m_State == CDWUSBGadget::StatePowered && !g.suspends && !g.resumes,
			       "powered state changed");
		      g.m_State = CDWUSBGadget::StateConfigured;
		      g.HandleUSBResume ();
		      require (!g.resumes && g.m_State == CDWUSBGadget::StateConfigured,
			       "unsolicited wake changed state");
	      });
	std::cout << passed << " passed, " << failed << " failed\n";
	return failed ? 1 : 0;
}
