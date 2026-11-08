#include <circle/usb/gadget/usbmidigadgetendpoint.h>
#include <circle/usb/gadget/usbmidigadget.h>
#include <circle/usb/usbmidi.h>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

static const u8 Event[] = {9, 0x90, 60, 100};
static void Check (bool value)
{
	if (!value)
	{
		throw std::runtime_error ("unexpected endpoint state or packet data");
	}
}
struct Fixture
{
	CUSBMIDIGadget Gadget;
	TUSBEndpointDescriptor Descriptor;
	CUSBMIDIDevice Interface;
	CUSBMIDIGadgetEndpoint EP;
	Fixture (bool in = true)
	    : Descriptor{static_cast<u8> (in ? 0x81 : 1)}, EP (&Descriptor, &Gadget)
	{
		EP.AttachInterface (&Interface);
	}
	bool Send ()
	{
		return Interface.SendEvents (Event, sizeof Event);
	}
	void Complete ()
	{
		EP.Armed = false;
		EP.OnTransferComplete (true, 4);
	}
};
int main ()
{
	unsigned passed = 0, failed = 0;
	auto Run = [&] (const char *name, std::function<void ()> test)
	{
		try
		{
			test ();
			Check (LockDepth == 0);
			++passed;
			std::cout << "PASS " << name << '\n';
		}
		catch (const std::exception &e)
		{
			++failed;
			std::cout << "FAIL " << name << ": " << e.what () << '\n';
		}
		OnUnlock = nullptr;
	};
	Run ("reject before configuration",
	     []
	     {
		     Fixture f;
		     Check (!f.Send ());
		     Check (f.EP.Starts == 0);
	     });
	Run ("send and drain",
	     []
	     {
		     Fixture f;
		     f.EP.OnActivate ();
		     Check (f.Send ());
		     Check (f.Send ());
		     Check (f.EP.Starts == 1);
		     f.Complete ();
		     Check (f.EP.Starts == 2);
		     f.Complete ();
		     Check (!f.EP.Armed);
		     Check (f.Send ());
		     Check (f.EP.Starts == 3);
		     for (auto &p : f.EP.Packets)
		     {
			     Check (p == std::vector<u8> (Event, Event + 4));
		     }
	     });
	Run ("reject after deconfiguration",
	     []
	     {
		     Fixture f;
		     f.EP.OnActivate ();
		     Check (f.Send ());
		     Check (f.Send ());
		     f.EP.OnDeactivate ();
		     Check (!f.Send ());
		     Check (!f.EP.Armed);
	     });
	Run ("reactivation discards old queue",
	     []
	     {
		     Fixture f;
		     f.EP.OnActivate ();
		     Check (f.Send ());
		     Check (f.Send ());
		     f.EP.OnDeactivate ();
		     f.EP.OnActivate ();
		     Check (f.Send ());
		     f.Complete ();
		     Check (f.EP.Starts == 2);
	     });
	Run ("reject after suspend",
	     []
	     {
		     Fixture f;
		     f.EP.OnActivate ();
		     f.EP.OnSuspend ();
		     Check (!f.Send ());
		     Check (f.EP.Starts == 0);
	     });
	Run ("reset clears active queue",
	     []
	     {
		     Fixture f;
		     f.EP.OnActivate ();
		     Check (f.Send ());
		     Check (f.Send ());
		     f.EP.OnUSBReset ();
		     Check (!f.Send ());
		     f.EP.OnActivate ();
		     Check (f.Send ());
		     Check (f.EP.Starts == 2);
		     f.Complete ();
		     Check (f.EP.Starts == 2);
	     });
	Run ("reset at first send unlock cannot start stale DMA",
	     []
	     {
		     Fixture f;
		     f.EP.OnActivate ();
		     OnUnlock = [&]
		     {
			     f.EP.OnDeactivate ();
			     f.EP.OnUSBReset ();
		     };
		     Check (f.Send ());
		     Check (!f.EP.Armed);
		     Check (f.EP.Starts == 1);
		     Check (!f.Send ());
	     });
	Run ("completion at first send unlock preserves packet ordering",
	     []
	     {
		     Fixture f;
		     f.EP.OnActivate ();
		     const u8 next[] = {8, 0x80, 60, 0};
		     OnUnlock = [&]
		     {
			     f.Complete ();
			     Check (f.Interface.SendEvents (next, 4));
		     };
		     Check (f.Send ());
		     Check (f.EP.Packets.size () == 2);
		     Check (f.EP.Packets[0] == std::vector<u8> (Event, Event + 4));
		     Check (f.EP.Packets[1] == std::vector<u8> (next, next + 4));
	     });
	Run ("late OUT completion stays stopped",
	     []
	     {
		     Fixture f (false);
		     f.EP.OnActivate ();
		     f.EP.OnDeactivate ();
		     f.EP.OnTransferComplete (false, 4);
		     Check (f.EP.Starts == 1);
		     Check (f.Interface.Received == 0);
	     });
	Run ("OUT callback can deactivate without rearm or deadlock",
	     []
	     {
		     Fixture f (false);
		     f.EP.OnActivate ();
		     f.Interface.OnPacket = [&] { f.EP.OnDeactivate (); };
		     f.EP.OnTransferComplete (false, 4);
		     Check (f.Interface.Received == 1);
		     Check (f.EP.Starts == 1);
		     Check (!f.EP.Armed);
	     });
	Run ("OUT malformed length discarded and valid packet delivered",
	     []
	     {
		     Fixture f (false);
		     f.EP.OnActivate ();
		     f.EP.OnTransferComplete (false, 3);
		     Check (f.Interface.Received == 0);
		     Check (f.EP.Starts == 2);
		     f.EP.OnTransferComplete (false, 4);
		     Check (f.Interface.Received == 1);
		     Check (f.EP.Starts == 3);
	     });
	Run ("late IN completion after deactivation stays stopped",
	     []
	     {
		     Fixture f;
		     f.EP.OnActivate ();
		     Check (f.Send ());
		     Check (f.Send ());
		     f.EP.OnDeactivate ();
		     f.Complete ();
		     Check (f.EP.Starts == 1);
		     Check (!f.EP.Armed);
	     });
	Run ("queue capacity rejection and ordered wraparound",
	     []
	     {
		     Fixture f;
		     f.EP.OnActivate ();
		     Check (f.Send ());
		     std::vector<u8> full (8192);
		     for (unsigned i = 0; i < full.size (); ++i)
		     {
			     full[i] = i % 251;
		     }
		     Check (f.Interface.SendEvents (full.data (), full.size ()));
		     Check (!f.Send ());
		     for (unsigned i = 0; i < 16; ++i)
		     {
			     f.Complete ();
		     }
		     f.Complete ();
		     std::vector<u8> received;
		     for (size_t i = 1; i < f.EP.Packets.size (); ++i)
		     {
			     received.insert (received.end (), f.EP.Packets[i].begin (),
					      f.EP.Packets[i].end ());
		     }
		     Check (received == full);
		     Check (f.Send ());
		     f.Complete ();
		     Check (f.EP.Packets.back () == std::vector<u8> (Event, Event + 4));
	     });
	std::cout << passed << " passed, " << failed << " failed\n";
	return failed ? 1 : 0;
}
