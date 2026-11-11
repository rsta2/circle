// Runs production EP0, endpoint and polling code. MMIO models W1C
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
void rawsetup (CDWUSBGadgetEndpoint0 &ep, TSetupData s)
{
	std::memcpy (ep.m_OutBuffer, &s, sizeof s);
	m.regs[DWHCI_DEV_OUT_EP_CTRL (0)] &= ~DWHCI_DEV_EP_CTRL_EP_ENABLE;
	m.regs[DWHCI_DEV_OUT_EP_XFER_SIZ (0)] = 0;
	m.regs[DWHCI_DEV_OUT_EP_INT (0)] |= DWHCI_DEV_OUT_EP_INT_SETUP_DONE;
	ep.HandleOutInterrupt ();
}
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
	for (unsigned requested : {64U, 128U})
	{
		test (requested == 64 ? "exact-length IN needs no ZLP" : "short IN needs terminating ZLP",
		      [requested]
		{
			CDWUSBGadget g;
			g.descriptorSize = 64;
			CDWUSBGadgetEndpoint0 ep (64, &g);
			ep.OnActivate ();
			rawsetup (ep, {0x80, GET_DESCRIPTOR, 0x100, 0, (u16) requested});
			complete (ep, true);
			if (requested > 64)
			{
				require (ep.m_State == ep.StateInDataPhase, "missing ZLP");
				require (ep.m_nBytesLeft == 0, "ZLP must contain no data");
				complete (ep, true);
			}
			require (ep.m_State == ep.StateOutStatusPhase, "missing OUT status");
			complete (ep, false);
		});
	}
	test ("GET_CONFIGURATION reflects selection", []
	{
		CDWUSBGadget g;
		CDWUSBGadgetEndpoint0 ep (64, &g);
		ep.OnActivate ();
		rawsetup (ep, {0, SET_CONFIGURATION, 1, 0, 0});
		complete (ep, true);
		rawsetup (ep, {0x80, 8, 0, 0, 1});
		require (ep.m_InBuffer[0] == 1, "wrong configuration");
		complete (ep, true); complete (ep, false);
	});
	test ("repeated activation preserves 274-byte IN phase and final 18 bytes",
	      []
	      {
		      CDWUSBGadget g;
		      g.descriptorSize = 274;
		      CDWUSBGadgetEndpoint0 ep (64, &g);
		      ep.OnActivate ();
		      setup (ep, GET_DESCRIPTOR, true);
		      ep.OnActivate ();
		      require (ep.m_State == CDWUSBGadgetEndpoint0::StateInDataPhase,
			       "activation replaced IN phase");
		      for (unsigned i = 0; i < 5; ++i)
		      {
			      complete (ep, true);
		      }
		      require (ep.m_State == CDWUSBGadgetEndpoint0::StateOutStatusPhase,
			       "descriptor did not reach OUT status");
		      ep.OnActivate ();
		      require (ep.m_State == CDWUSBGadgetEndpoint0::StateOutStatusPhase,
			       "activation replaced OUT status");
		      complete (ep, false);
		      require (ep.m_State == CDWUSBGadgetEndpoint0::StateIdle,
			       "descriptor control transfer incomplete");
	      });
	test ("SETUP plus XFRC acknowledged before IN status callback",
	      []
	      {
		      CDWUSBGadget g;
		      CDWUSBGadgetEndpoint0 ep (64, &g);
		      ep.OnActivate ();
		      setup (ep, SET_ADDRESS, true);
		      require (!m.regs[DWHCI_DEV_OUT_EP_INT (0)], "SETUP left XFRC latched");
		      ep.HandleOutInterrupt ();
		      ep.OnActivate ();
		      require (ep.m_State == CDWUSBGadgetEndpoint0::StateInStatusPhase,
			       "status phase changed");
		      complete (ep, true);
		      require (!m.unsafeRearm, "next SETUP rearmed with stale status");
	      });
	test ("repeated activation preserves OUT data then IN status",
	      []
	      {
		      CDWUSBGadget g;
		      CDWUSBGadgetEndpoint0 ep (64, &g);
		      ep.OnActivate ();
		      setup (ep, 0x20, true, 7);
		      ep.OnActivate ();
		      require (ep.m_State == CDWUSBGadgetEndpoint0::StateOutDataPhase,
			       "activation replaced OUT data");
		      complete (ep, false);
		      ep.OnActivate ();
		      require (ep.m_State == CDWUSBGadgetEndpoint0::StateInStatusPhase,
			       "activation replaced IN status");
		      complete (ep, true);
		      require (!m.unsafeRearm, "OUT phase left stale status");
	      });
	for (unsigned residual : {120u, 127u, 1u})
	{
		test (("invalid SETUP residual recovers: " + std::to_string (residual)).c_str (),
		      [&]
		      {
			      CDWUSBGadget g;
			      CDWUSBGadgetEndpoint0 ep (64, &g);
			      ep.OnActivate ();
			      m.regs[DWHCI_DEV_OUT_EP_CTRL (0)] &= ~DWHCI_DEV_EP_CTRL_EP_ENABLE;
			      m.regs[DWHCI_DEV_OUT_EP_XFER_SIZ (0)] = 0x40080000u | residual;
			      m.regs[DWHCI_DEV_OUT_EP_INT (0)] = DWHCI_DEV_OUT_EP_INT_SETUP_DONE |
								 DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE;
			      ep.HandleOutInterrupt ();
			      require (ep.m_State == CDWUSBGadgetEndpoint0::StateIdle &&
					   ep.IsTransferActive (),
				       "invalid SETUP did not rearm");
			      require (!m.unsafeRearm && !m.outDisables, "unsafe SETUP recovery");
			      setup (ep, SET_ADDRESS, true);
			      complete (ep, true);
			      require (ep.m_State == CDWUSBGadgetEndpoint0::StateIdle,
				       "host retry did not complete");
		      });
	}
	for (unsigned phase = 0; phase < 5; ++phase)
	{
		test (("SETUP aborts previous control phase " + std::to_string (phase)).c_str (),
		      [&]
		      {
			      CDWUSBGadget g;
			      CDWUSBGadgetEndpoint0 ep (64, &g);
			      ep.OnActivate ();
			      if (phase == 0)
			      {
				      setup (ep, 0x20, true, 7); // OUT data
			      }
			      if (phase == 1)
			      {
				      setup (ep, GET_DESCRIPTOR, true);
				      complete (ep, true);
			      } // OUT status
			      if (phase == 2)
			      {
				      setup (ep, GET_DESCRIPTOR, true); // IN data still enabled
			      }
			      if (phase == 3)
			      {
				      setup (ep, SET_ADDRESS, true); // IN status still enabled
			      }
			      if (phase == 4)
			      {
				      ep.InitTransfer (); // no active software transfer
			      }
			      m.regs[DWHCI_DEV_OUT_EP_CTRL (0)] &= ~DWHCI_DEV_EP_CTRL_EP_ENABLE;
			      m.regs[DWHCI_DEV_OUT_EP_XFER_SIZ (0)] = 0x40080078;
			      m.regs[DWHCI_DEV_OUT_EP_INT (0)] = DWHCI_DEV_OUT_EP_INT_SETUP_DONE |
								 DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE;
			      ep.HandleOutInterrupt ();
			      require (ep.m_State == CDWUSBGadgetEndpoint0::StateIdle &&
					   ep.m_TransferMode ==
					       CDWUSBGadgetEndpoint::TransferSetupOut,
				       "aborted control phase not rearmed");
			      require (!m.outDisables && !m.unsafeDisable && !m.unsafeFlush &&
					   !m.unsafeRearm,
				       "unsafe abort recovery");
			      if (phase == 2 || phase == 3)
			      {
				      require (m.disables == 1 && m.flushes == 1,
					       "active IN not synchronously retired");
			      }
			      setup (ep, SET_ADDRESS, true);
			      complete (ep, true);
			      require (ep.m_State == CDWUSBGadgetEndpoint0::StateIdle,
				       "retry after aborted phase failed");
		      });
	}
	test ("SETUP completion uses one validated residual snapshot",
	      []
	      {
		      CDWUSBGadget g;
		      CDWUSBGadgetEndpoint0 ep (64, &g);
		      ep.OnActivate ();
		      m.changeSetupSize = true;
		      setup (ep, SET_ADDRESS, true);
		      require (m.setupSizeReads == 1, "SETUP counter was read twice");
		      require (ep.m_State == CDWUSBGadgetEndpoint0::StateInStatusPhase,
			       "valid request did not complete");
		      m.changeSetupSize = false;
		      complete (ep, true);
	      });
	for (unsigned fault = 0; fault < 3; ++fault)
	{
		test (
		    ("failed IN cancellation retains transfer: " + std::to_string (fault)).c_str (),
		    [&]
		    {
			    CDWUSBGadget g;
			    CDWUSBGadgetEndpoint0 ep (64, &g);
			    ep.OnActivate ();
			    setup (ep, GET_DESCRIPTOR, true);
			    if (fault == 0)
			    {
				    m.allowNak = false;
			    }
			    if (fault == 1)
			    {
				    m.allowDisable = false;
			    }
			    if (fault == 2)
			    {
				    m.allowFlush = false;
			    }
			    m.regs[DWHCI_DEV_OUT_EP_XFER_SIZ (0)] = 120;
			    m.regs[DWHCI_DEV_OUT_EP_INT (0)] = DWHCI_DEV_OUT_EP_INT_SETUP_DONE;
			    bool panicked = false;
			    try
			    {
				    ep.HandleOutInterrupt ();
			    }
			    catch (const Failure &e)
			    {
				    panicked = std::string (e.what ()).find ("timeout") !=
					       std::string::npos;
			    }
			    require (panicked,
				     "failed cancellation did not reach bounded fatal path");
			    require (ep.m_TransferMode == CDWUSBGadgetEndpoint::TransferDataIn,
				     "live transfer retired after timeout");
			    require (!m.unsafeRearm && !m.outDisables,
				     "timeout rearmed SETUP unsafely");
			    require (m.now >= 10000 && m.now < 11000,
				     "cancellation timeout is not bounded");
		    });
	}
	test ("class SET validated once before IN status acknowledgement", []
	{
		CDWUSBGadget g; CDWUSBGadgetEndpoint0 ep (64, &g); ep.OnActivate ();
		rawsetup (ep, {0x22, 1, 0x100, 1, 3});
		require (!g.classCalls, "class callback before data received");
		g.handler = [&] (const TSetupData *s, u8 *p)
		{
			require (s->wLength == 3 && p[0] == 0x80, "wrong parameter block");
			require (!(m.regs[DWHCI_DEV_IN_EP_CTRL (0)] & DWHCI_DEV_EP_CTRL_EP_ENABLE),
				 "status ACK sent before validation");
			return 0;
		};
		ep.m_OutBuffer[0] = 0x80; complete (ep, false);
		require (g.classCalls == 1 && ep.m_State == CDWUSBGadgetEndpoint0::StateInStatusPhase,
			 "accepted SET lacks status");
		complete (ep, true); require (g.classCalls == 1, "SET applied twice");
	});
	test ("invalid class SET stalls without acknowledging success", []
	{
		CDWUSBGadget g; g.classResult = -1;
		CDWUSBGadgetEndpoint0 ep (64, &g); ep.OnActivate ();
		rawsetup (ep, {0x22, 1, 0x100, 1, 3}); complete (ep, false);
		require (g.classCalls == 1 && ep.m_TransferMode == CDWUSBGadgetEndpoint::TransferSetupOut,
			 "invalid SET sent success");
		require (m.regs[DWHCI_DEV_IN_EP_CTRL (0)] & DWHCI_DEV_EP_CTRL_STALL, "no STALL");
	});
	test ("truncated class parameter block is not applied", []
	{
		CDWUSBGadget g; CDWUSBGadgetEndpoint0 ep (64, &g); ep.OnActivate ();
		rawsetup (ep, {0x22, 1, 0x100, 1, 3});
		m.regs[DWHCI_DEV_OUT_EP_CTRL (0)] &= ~DWHCI_DEV_EP_CTRL_EP_ENABLE;
		m.regs[DWHCI_DEV_OUT_EP_XFER_SIZ (0)] = 1;
		m.regs[DWHCI_DEV_OUT_EP_INT (0)] |= DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE;
		ep.HandleOutInterrupt ();
		require (!g.classCalls && ep.m_TransferMode == CDWUSBGadgetEndpoint::TransferSetupOut,
			 "partial SET accepted");
	});
	test ("class request number cannot alias standard SET_ADDRESS", []
	{
		CDWUSBGadget g; CDWUSBGadgetEndpoint0 ep (64, &g); ep.OnActivate ();
		rawsetup (ep, {0x21, SET_ADDRESS, 0, 0, 0});
		require (g.classCalls == 1 && !g.addresses, "class request dispatched as standard");
	});
	test ("alternate setting request validates recipient and length", []
	{
		CDWUSBGadget g; CDWUSBGadgetEndpoint0 ep (64, &g); ep.OnActivate ();
		require (g.GetInterface (1) < 0, "unconfigured interface accepted");
		g.SetConfiguration (1);
		require (g.GetInterface (0) == 0 && !g.SetInterface (1, 1)
			 && !g.SetInterface (2, 0), "invalid interface or alternate accepted");
		rawsetup (ep, {0x81, 10, 0, 1, 1});
		require (ep.m_State == CDWUSBGadgetEndpoint0::StateInDataPhase && ep.m_InBuffer[0] == 0,
			 "GET_INTERFACE failed");
		complete (ep, true); complete (ep, false);
		rawsetup (ep, {0x01, SET_INTERFACE, 0, 1, 0});
		require (ep.m_State == CDWUSBGadgetEndpoint0::StateInStatusPhase, "SET_INTERFACE failed");
		complete (ep, true);
		rawsetup (ep, {0x00, SET_INTERFACE, 1, 1, 0});
		require (ep.m_TransferMode == CDWUSBGadgetEndpoint::TransferSetupOut, "wrong recipient accepted");
	});
	test ("oversized class IN reply is rejected", []
	{
		CDWUSBGadget g; g.classResult = 4;
		CDWUSBGadgetEndpoint0 ep (64, &g); ep.OnActivate ();
		rawsetup (ep, {0xA2, 0x81, 0x100, 1, 3});
		require (ep.m_TransferMode == CDWUSBGadgetEndpoint::TransferSetupOut, "oversized reply accepted");
	});
	std::cout << passed << " passed, " << failed << " failed\n";
	return failed ? 1 : 0;
}
