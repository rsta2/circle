#include <circle/usb/dwhci.h>
#include <assert.h>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
using u32 = uint32_t;
static constexpr u32 Causes = DWHCI_DEV_OUT_EP_INT_SETUP_DONE | DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE |
			      DWHCI_DEV_OUT_EP_INT_EP_DISABLED | DWHCI_DEV_OUT_EP_INT_AHB_ERROR;
struct Model
{
	std::array<u32, 16> Status{};
	std::array<unsigned, 16> Writes{};
	u32 Mask = 0xFFFF0000U;
	u32 Summary () const
	{
		u32 result = 0;
		for (unsigned i = 0; i < Status.size (); ++i)
		{
			if (Status[i] & Causes)
			{
				result |= 1U << (16 + i);
			}
		}
		return result;
	}
	bool Pending () const
	{
		return (Summary () & Mask) != 0;
	}
} M;
class CDWHCIRegister
{
public:
	CDWHCIRegister (u32 address, u32 value = 0) : Address (address), Value (value)
	{
	}
	u32 Read ()
	{
		if (Address == DWHCI_DEV_ALL_EPS_INT_STAT)
		{
			return Value = M.Summary ();
		}
		if (Address == DWHCI_DEV_ALL_EPS_INT_MASK)
		{
			return Value = M.Mask;
		}
		return Value = M.Status.at (Index ());
	}
	void Set (u32 value)
	{
		Value = value;
	}
	void Write ()
	{
		unsigned i = Index ();
		M.Status.at (i) &= ~Value;
		++M.Writes.at (i);
	}

private:
	unsigned Index () const
	{
		if (Address < DWHCI_DEV_OUT_EP_INT (0) ||
		    (Address - DWHCI_DEV_OUT_EP_INT (0)) % 0x20)
		{
			throw std::runtime_error ("unexpected register access");
		}
		return (Address - DWHCI_DEV_OUT_EP_INT (0)) / 0x20;
	}
	u32 Address, Value;
};
class Endpoint
{
public:
	unsigned Calls = 0;
	void HandleOutInterrupt ()
	{
		++Calls;
	}
};
class CDWUSBGadget
{
public:
	enum TState
	{
		StatePowered,
		StateSuspended,
		StateResetDone,
		StateEnumDone,
		StateConfigured
	};
	static constexpr unsigned NumberOfEPs = 3;
	TState m_State = StateConfigured;
	Endpoint *m_pEP[NumberOfEPs + 1]{};
	void HandleOutEPInterrupt (void);
};
#include "real-gadget.inc"
static void Check (bool ok)
{
	if (!ok)
	{
		throw std::runtime_error ("unexpected pending status or dispatch");
	}
}
int main ()
{
	unsigned passed = 0, failed = 0;
	auto Run = [&] (std::string name, std::function<void ()> test)
	{
		M = {};
		try
		{
			test ();
			++passed;
			std::cout << "PASS " << name << '\n';
		}
		catch (const std::exception &e)
		{
			++failed;
			std::cout << "FAIL " << name << ": " << e.what () << '\n';
		}
	};
	for (unsigned context = 0; context < 3; ++context)
	{
		for (u32 flags : {u32 (DWHCI_DEV_OUT_EP_INT_SETUP_DONE),
				  u32 (DWHCI_DEV_OUT_EP_INT_XFER_COMPLETE),
				  u32 (DWHCI_DEV_OUT_EP_INT_EP_DISABLED),
				  u32 (DWHCI_DEV_OUT_EP_INT_AHB_ERROR), Causes})
		{
			Run ("ignored context " + std::to_string (context) + " flags " +
				 std::to_string (flags),
			     [&]
			     {
				     CDWUSBGadget g;
				     Endpoint ep;
				     unsigned n = context == 0 ? 0 : 1;
				     g.m_State = context == 2 ? g.StateResetDone : g.StateSuspended;
				     g.m_pEP[n] = &ep;
				     M.Status[n] = flags;
				     Check (M.Pending ());
				     g.HandleOutEPInterrupt ();
				     Check (!M.Pending ());
				     Check (M.Status[n] == 0);
				     Check (ep.Calls == 0);
				     Check (M.Writes[n] == 1);
				     g.HandleOutEPInterrupt ();
				     Check (M.Writes[n] == 1); // No immediate repeated IRQ work.
			     });
		}
	}
	for (auto state : {CDWUSBGadget::StateResetDone, CDWUSBGadget::StateEnumDone,
			   CDWUSBGadget::StateConfigured})
	{
		Run ("active dispatch state " + std::to_string (state),
		     [&]
		     {
			     CDWUSBGadget g;
			     Endpoint ep;
			     unsigned n = state == g.StateResetDone ? 0 : 1;
			     g.m_State = state;
			     g.m_pEP[n] = &ep;
			     M.Status[n] = Causes;
			     g.HandleOutEPInterrupt ();
			     Check (ep.Calls == 1);
			     Check (M.Status[n] == Causes);
			     Check (M.Writes[n] == 0);
		     });
	}
	Run ("masked endpoint untouched",
	     []
	     {
		     CDWUSBGadget g;
		     g.m_State = g.StateSuspended;
		     M.Status[1] = Causes;
		     M.Mask &= ~(1U << 17);
		     g.HandleOutEPInterrupt ();
		     Check (M.Status[1] == Causes);
		     Check (M.Writes[1] == 0);
	     });
	Run ("reset dispatches EP0 and retires multiple noncontrol endpoints",
	     []
	     {
		     CDWUSBGadget g;
		     Endpoint ep;
		     g.m_State = g.StateResetDone;
		     g.m_pEP[0] = &ep;
		     M.Status[0] = DWHCI_DEV_OUT_EP_INT_SETUP_DONE;
		     M.Status[1] = DWHCI_DEV_OUT_EP_INT_EP_DISABLED;
		     M.Status[3] = DWHCI_DEV_OUT_EP_INT_AHB_ERROR;
		     g.HandleOutEPInterrupt ();
		     Check (ep.Calls == 1);
		     Check (M.Writes[0] == 0);
		     Check (M.Status[1] == 0);
		     Check (M.Status[3] == 0);
		     Check (M.Summary () == (1U << 16));
	     });
	std::cout << passed << " passed, " << failed << " failed\n";
	return failed ? 1 : 0;
}
