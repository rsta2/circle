// SPDX-License-Identifier: GPL-3.0-or-later
#include <circle/usb/gadget/usbaudiogadgetcapture.h>
#include <circle/usb/gadget/usbaudiogadget.h>
#include <cassert>
#include <cstdio>
#include <initializer_list>

namespace
{
class CToneSource : public CUSBAudioCaptureSource
{
public:
	void SetRecording (boolean) override {}
	unsigned ReadPacket (s16 *pSamples) override
	{
		for (unsigned i = 0; i < MaxFrames * Channels; ++i) pSamples[i] = 42;
		return nFrames;
	}
	unsigned nFrames = NominalFrames;
};

void TestSynchronousEndpoint (CDWUSBGadget *pGadget)
{
	TUSBAudioCaptureDescriptors Desc (1, 1, 1, 2, 0x0603,
					  TUSBAudioCaptureDescriptors::Synchronous);
	assert (Desc.Endpoint.bmAttributes == 13 && Desc.Endpoint.wMaxPacketSize == 192);
	assert (Desc.Streaming[39] == 13 && Desc.Streaming[40] == 192);
	CToneSource Source;
	CUSBAudioCaptureEndpoint Endpoint (Desc, pGadget, &Source);
	Endpoint.OnActivate ();
	assert (Endpoint.SetInterface (1, 1));
	for (unsigned count : {48U, 0U, 47U, 49U, 48U})
	{
		Source.nFrames = count;
		Endpoint.active = false;
		Endpoint.OnTransferComplete (TRUE, 192);
		assert (Endpoint.active && Endpoint.length == 192);
		auto *p = static_cast<const s16 *> (Endpoint.buffer);
		for (unsigned i = 0; i < 96; ++i) assert (p[i] == (count == 48 ? 42 : 0));
	}
	Endpoint.OnDeactivate ();
}

void WriteRamp (CUSBAudioCaptureBuffer &Buffer, unsigned nFirst, unsigned nFrames)
{
	s16 Samples[128];
	while (nFrames)
	{
		unsigned n = nFrames > 64 ? 64 : nFrames;
		for (unsigned i = 0; i < n; ++i, ++nFirst)
		{
			Samples[2*i] = (s16) nFirst;
			Samples[2*i+1] = (s16) ~nFirst;
		}
		Buffer.WriteFrames (Samples, n);
		nFrames -= n;
	}
}

void Restart (CUSBAudioCaptureBuffer &Buffer, unsigned nBase)
{
	s16 Packet[98];
	Buffer.SetRecording (TRUE);
	assert (Buffer.ReadPacket (Packet) == 48);
	for (unsigned i = 0; i < 96; ++i) assert (Packet[i] == 0);

	// Delay the host's first completion while DMA advances by 12 blocks.
	WriteRamp (Buffer, nBase, 768);
	unsigned nNext = nBase + 768 - 256;
	unsigned nProduced = nBase + 768;
	for (unsigned i = 2; i <= 64; ++i)
	{
		unsigned nFrames = Buffer.ReadPacket (Packet);
		// Force positive drift with 64-frame DMA blocks. No consecutive
		// long packets and no stale, duplicated or reordered samples.
		assert (nFrames == (i % 8 == 0 ? 49U : 48U));
		for (unsigned j = 0; j < nFrames; ++j, ++nNext)
		{
			assert (Packet[2*j] == (s16) nNext);
			assert (Packet[2*j+1] == (s16) ~nNext);
		}
		unsigned nBlock = i < 9 ? 64 : 48;
		WriteRamp (Buffer, nProduced, nBlock);
		nProduced += nBlock;
	}
	Buffer.ReadPacket (Packet); // Stop off the eight-packet boundary.
	Buffer.SetRecording (FALSE);
	WriteRamp (Buffer, 0, 800);
}

void Starvation (CUSBAudioCaptureBuffer &Buffer)
{
	s16 Packet[98];
	Buffer.SetRecording (TRUE);
	WriteRamp (Buffer, 0, 256);
	for (unsigned i = 1; i <= 8; ++i)
	{
		assert (Buffer.ReadPacket (Packet) == (i == 8 ? 47U : 48U));
		WriteRamp (Buffer, i * 32, 32);
	}
	for (unsigned i = 0; i < 16; ++i) Buffer.ReadPacket (Packet);
	CUSBAudioCaptureBuffer::TStatistics Stats;
	Buffer.GetStatistics (&Stats);
	assert (Stats.nUnderruns == 1);
	for (unsigned i = 0; i < 96; ++i) assert (Packet[i] == 0);
	WriteRamp (Buffer, 1000, 1200);
	assert (Buffer.ReadPacket (Packet) == 48);
	assert (Packet[0] == 1000 + 1200 - 256);
	Buffer.SetRecording (FALSE);
}
}

void CaptureTests (void)
{
	CUSBAudioCaptureBuffer Buffer;
	Restart (Buffer, 0);
	Restart (Buffer, 8000);
	Starvation (Buffer);

	TUSBAudioCaptureDescriptors Desc (3, 2, 9, 10, 0x0603);
	assert (Desc.Endpoint.bEndpointAddress == 0x82 && Desc.Endpoint.wMaxPacketSize == 196);
	assert (Desc.Terminals[3] == 9 && Desc.Terminals[15] == 10 && Desc.Terminals[19] == 9);
	assert (Desc.Streaming[2] == 3 && Desc.Streaming[11] == 3 && Desc.Streaming[21] == 10);
	assert (Desc.Streaming[38] == 0x82 && Desc.Streaming[40] == 196);

	CInterruptSystem IRQ;
	CUSBAudioGadgetFunction::TConfiguration Config {};
	CUSBAudioGadget Gadget (&IRQ, Config, nullptr, 1, 1);
	TestSynchronousEndpoint (&Gadget);
	CUSBAudioCaptureEndpoint Endpoint (Desc, &Gadget, &Buffer);
	assert (!Endpoint.SetInterface (3, 1));
	Endpoint.OnActivate ();
	assert (!Endpoint.SetInterface (1, 1) && !Endpoint.SetInterface (3, 2));
	assert (Endpoint.SetInterface (3, 1) && Endpoint.active && Endpoint.length == 192);
	assert (Endpoint.SetInterface (3, 1) && Endpoint.arms == 1);
	Endpoint.active = false;	// hardware completed the armed IN packet
	Endpoint.OnTransferComplete (TRUE, 192);
	assert (Endpoint.arms == 2 && Endpoint.active);
	assert (Endpoint.SetInterface (3, 0) && Endpoint.cancels == 1);
	Endpoint.OnTransferComplete (TRUE, 192);
	assert (!Endpoint.active && Endpoint.GetInterface (3) == 0);
	assert (Endpoint.SetInterface (3, 1));
	Endpoint.OnUSBReset ();
	assert (!Endpoint.active && !Endpoint.SetInterface (3, 1));
	Endpoint.OnActivate ();
	assert (Endpoint.SetInterface (3, 1));
	Endpoint.OnSuspend ();
	assert (!Endpoint.active && !Endpoint.SetInterface (3, 1));
	puts ("PASS capture: delayed restart, stereo continuity, clock corrections, starvation and endpoint lifecycle");
}
