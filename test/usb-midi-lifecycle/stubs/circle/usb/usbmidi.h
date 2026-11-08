#pragma once
#include <circle/types.h>
#include <functional>
class CUSBMIDIDevice
{
public:
	static constexpr unsigned EventPacketSize = 4;
	using Handler = boolean (*) (const u8 *, unsigned, void *);
	void RegisterSendEventsHandler (Handler h, void *p)
	{
		Send = h;
		Param = p;
	}
	bool SendEvents (const u8 *p, unsigned n)
	{
		return Send (p, n, Param);
	}
	void CallPacketHandler (const u8 *, size_t)
	{
		++Received;
		if (OnPacket)
		{
			OnPacket ();
		}
	}
	unsigned Received = 0;
	std::function<void ()> OnPacket;

private:
	Handler Send = nullptr;
	void *Param = nullptr;
};
