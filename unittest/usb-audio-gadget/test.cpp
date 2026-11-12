// SPDX-License-Identifier: GPL-3.0-or-later
//
// test.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <circle/usb/gadget/usbaudiogadgetfunction.h>
#include <circle/usb/gadget/usbaudiogadgetbuffer.h>
#include <circle/usb/usbaudiofunctopology.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <vector>
#include <random>

unsigned g_nLocked = 0;
using F = CUSBAudioGadgetFunction;
struct CSink : F::CSink
{
	bool good = true, accept = true, active = false;
	unsigned stops = 0, configurations = 0, writes = 0, frames = 0;
	int volume = 0;
	bool mute = false;
	F::TStream stream {};
	std::function<void ()> duringConfigure;
	std::vector<u8> last;
	void Stop () override { assert (!g_nLocked); active = false; ++stops; }
	boolean Configure (const F::TStream &s) override
	{
		assert (!g_nLocked); stream = s; ++configurations;
		if (duringConfigure) duringConfigure ();
		return active = good;
	}
	void SetControls (boolean m, int v) override
	{ assert (!g_nLocked); mute = m; volume = v; }
	boolean Write (const void *p, unsigned n, const F::TStream &s) override
	{
		assert (!g_nLocked && active); ++writes; frames += n;
		assert (s.Format == stream.Format && s.nSampleRate == stream.nSampleRate);
		const u8 *b = (const u8 *) p;
		last.assign (b, b + n*s.nChannels*F::GetSubframeSize (s.Format));
		return accept;
	}
};
F::TConfiguration Configuration (unsigned channels = 2)
{
	F::TConfiguration c {};
	c.nControlInterface = 4; c.nStreamingInterface = 9; c.nEndpoint = 5;
	c.nInputTerminal = 7; c.nFeatureUnit = 8; c.nOutputTerminal = 10;
	c.nChannels = channels; c.nAlternates = F::MaxAlternates;
	for (unsigned i = 0; i < c.nAlternates; i++)
	{
		c.Alternates[i].Format = (F::TFormat) i;
		c.Alternates[i].nRates = 3;
		c.Alternates[i].Rates[0] = 24000;
		c.Alternates[i].Rates[1] = 44100;
		c.Alternates[i].Rates[2] = 48000;
	}
	return c;
}
int Request (F &f, unsigned type, unsigned req, unsigned value,
	     unsigned index, unsigned length, u8 *data)
{
	TSetupData s {(u8) type, (u8) req, (u16) value, (u16) index, (u16) length};
	return f.ControlRequest (&s, data);
}
void Descriptors ()
{
	for (unsigned channels = 1; channels <= 8; channels++)
	for (unsigned fmt = 0; fmt < F::MaxAlternates; fmt++)
	for (unsigned rate : {8000u, 16000u, 24000u, 32000u, 44100u, 48000u, 96000u, 192000u})
	{
		auto c = Configuration (channels);
		c.nAlternates = 1; c.Alternates[0] = {(F::TFormat) fmt, 1, {rate}};
		CSink sink; F f (c, &sink);
		unsigned packet = ((rate+999)/1000)*channels*F::GetSubframeSize ((F::TFormat) fmt);
		assert (f.IsValid () == (packet <= 1023));
		u8 d[512]; memset (d, 0xA5, sizeof d);
		unsigned size = f.GetDescriptor (d, sizeof d);
		if (!f.IsValid ()) { assert (!size && d[0] == 0xA5); continue; }
		assert (size && size <= sizeof d && f.GetMaxPacketSize () == packet);
		// Parse the complete generated AC block using Circle's production host
		// parser. The following standard interface terminates its descriptor walk.
		CUSBAudioFunctionTopology Topology;
		assert (Topology.Parse (reinterpret_cast<const TUSBAudioControlInterfaceDescriptor *> (d+9)));
		auto *pInput = static_cast<CUSBAudioTerminal *> (Topology.GetEntity (c.nInputTerminal));
		auto *pFeature = static_cast<CUSBAudioFeatureUnit *> (Topology.GetEntity (c.nFeatureUnit));
		auto *pOutput = static_cast<CUSBAudioTerminal *> (Topology.GetEntity (c.nOutputTerminal));
		assert (pInput && pFeature && pOutput);
		assert (pInput->GetEntityType () == CUSBAudioEntity::EntityTerminal);
		assert (pInput->IsInput () && pInput->GetTerminalType () == USB_AUDIO_TERMINAL_TYPE_USB_STREAMING);
		assert (pInput->GetNumChannels () == channels && !pInput->GetNumSources ());
		assert (!pOutput->IsInput () && pOutput->GetTerminalType () == USB_AUDIO_TERMINAL_TYPE_SPEAKER);
		assert (pOutput->GetSourceID (0) == c.nFeatureUnit);
		assert (pFeature->GetEntityType () == CUSBAudioEntity::EntityFeatureUnit);
		assert (pFeature->GetSourceID (0) == c.nInputTerminal);
		assert (Topology.FindEntity (CUSBAudioEntity::EntityFeatureUnit, pOutput, TRUE) == pFeature);
		assert (Topology.FindEntity (CUSBAudioEntity::EntityTerminal, pFeature, TRUE) == pInput);
		for (unsigned ch = 0; ch <= channels; ch++)
		{
			auto Expected = ch ? CUSBAudioFeatureUnit::ControlNotPresent : CUSBAudioFeatureUnit::ControlReadWrite;
			assert (pFeature->GetControlStatus (ch, CUSBAudioFeatureUnit::MuteControl) == Expected);
			assert (pFeature->GetControlStatus (ch, CUSBAudioFeatureUnit::VolumeControl) == Expected);
		}
		unsigned interface = 255, alt = 0, eps = 0, tags = 0, types = 0, controls = 0;
		for (unsigned pos = 0; pos < size; pos += d[pos])
		{
			unsigned n = d[pos]; assert (n >= 2 && pos+n <= size);
			if (d[pos+1] == 4) { interface = d[pos+2]; alt = d[pos+3]; }
			if (interface == 4 && d[pos+1] == 0x24 && d[pos+2] == 1)
			{
				unsigned end = pos + d[pos+5] + (unsigned (d[pos+6])<<8);
				assert (d[end] == 9 && d[end+1] == 4 && d[end+2] == 9); ++controls;
			}
			if (interface == 9 && d[pos+1] == 0x24 && d[pos+2] == 1)
			{
				unsigned tag = d[pos+5] | (unsigned) d[pos+6]<<8;
				assert (tag == (fmt == F::U8 ? 2u : fmt == F::Float32 ? 3u : 1u));
				++tags;
			}
			if (interface == 9 && d[pos+1] == 0x24 && d[pos+2] == 2)
			{
				assert (alt == 1 && d[pos+4] == channels && d[pos+7] == 1);
				assert ((d[pos+8] | unsigned(d[pos+9])<<8 | unsigned(d[pos+10])<<16) == rate);
				assert (d[pos+5] == F::GetSubframeSize ((F::TFormat) fmt)); ++types;
			}
			if (d[pos+1] == 5)
			{
				assert (interface == 9 && alt == 1 && d[pos+2] == 5 && d[pos+3] == 9);
				assert ((d[pos+4] | unsigned(d[pos+5])<<8) == packet); ++eps;
			}
		}
		assert (eps == 1 && tags == 1 && types == 1 && controls == 1);
		for (unsigned cap = 0; cap < size; cap++)
		{
			memset (d, 0xA5, sizeof d); assert (!f.GetDescriptor (d, cap));
			for (auto b : d) assert (b == 0xA5);
		}
	}
	CSink sink;
	auto c = Configuration (); F all (c, &sink); u8 d[512]; assert (all.GetDescriptor (d, 512));
	auto largest = Configuration (F::MaxChannels);
	for (auto &alternate : largest.Alternates)
	{
		alternate.nRates = F::MaxRates;
		for (unsigned i = 0; i < F::MaxRates; i++) alternate.Rates[i] = 8000 + i*1000;
	}
	F maximum (largest, &sink);
	u8 full[F::MaxDescriptorSize];
	assert (maximum.IsValid () && maximum.GetDescriptor (full, sizeof full) == sizeof full);
	c.Alternates[1].Format = F::PCM8; assert (!F (c, &sink).IsValid ());
	c = Configuration (); c.Alternates[0].Rates[1] = 24000; assert (!F (c, &sink).IsValid ());
	c = Configuration (); c.nEndpoint = 0x85; assert (!F (c, &sink).IsValid ());
	c = Configuration (); c.nFeatureUnit = c.nInputTerminal; assert (!F (c, &sink).IsValid ());
	c = Configuration (); c.wChannelConfig = 7; assert (!F (c, &sink).IsValid ());
	assert (!F::GetPacketSize (~0u, 2, F::PCM16));
	assert (!F::GetPacketSize (48000, ~0u, F::PCM16));
	puts ("PASS descriptors: 384 channel/format/rate profiles; bounds and production host topology parser");
}
void ControlsAndLifecycle ()
{
	CSink sink; F f (Configuration (), &sink); u8 data[4] {};
	assert (!f.SetInterface (9, 1)); assert (f.GetInterface (9) == -1);
	assert (Request (f, 0xA2, 0x81, 0x100, 5, 3, data) == -1);
	f.SetConfigured (true); assert (f.GetInterface (9) == 0);
	assert (f.SetInterface (4, 0) && !f.SetInterface (4, 1));
	assert (!f.SetInterface (8, 1) && !f.SetInterface (9, F::MaxAlternates+1));
	assert (f.SetInterface (9, 2)); f.Process ();
	assert (sink.active && sink.stream.Format == F::PCM16 && sink.stream.nSampleRate == 24000);
	data[0] = 0x44; data[1] = 0xAC; data[2] = 0;
	assert (!Request (f, 0x22, 1, 0x100, 5, 3, data));
	assert (sink.stream.nSampleRate == 24000); // Deferred until Process
	f.Process (); assert (sink.stream.nSampleRate == 44100);
	data[0] = 0x80; data[1] = 0xBB; data[2] = 0;
	assert (!Request (f, 0x22, 1, 0x100, 5, 3, data)); f.Process ();
	assert (sink.stream.nSampleRate == 48000);
	for (unsigned badType : {0x02u, 0x62u, 0x82u, 0xE2u})
		assert (Request (f, badType, 1, 0x100, 5, 3, data) == -1);
	assert (Request (f, 0x22, 1, 0x101, 5, 3, data) == -1);
	assert (Request (f, 0x22, 1, 0x100, 0x105, 3, data) == -1);
	for (unsigned len : {0u, 1u, 2u, 4u, 65535u})
		assert (Request (f, 0x22, 1, 0x100, 5, len, data) == -1);
	data[0] = data[1] = data[2] = 0;
	assert (!Request (f, 0x22, 1, 0x100, 5, 3, data)); // Round to nearest
	assert (Request (f, 0xA2, 0x81, 0x100, 5, 3, data) == 3);
	assert ((data[0] | unsigned(data[1])<<8 | unsigned(data[2])<<16) == 24000);
	for (unsigned r = 0x82; r <= 0x84; r++) assert (Request (f, 0xA2, r, 0x100, 5, 3, data) == 3);
	data[0] = 1; assert (!Request (f, 0x21, 1, 0x100, 0x804, 1, data));
	data[0] = 0; data[1] = 0xFA; assert (!Request (f, 0x21, 1, 0x200, 0x804, 2, data));
	f.Process (); assert (sink.mute && sink.volume == -6*256);
	data[0] = 1; data[1] = 0; assert (Request (f, 0x21, 1, 0x200, 0x804, 2, data) == -1);
	data[0] = 2; assert (Request (f, 0x21, 1, 0x100, 0x804, 1, data) == -1);
	assert (Request (f, 0xA1, 0x81, 0x201, 0x804, 2, data) == -1);
	for (unsigned r = 0x81; r <= 0x84; r++) assert (Request (f, 0xA1, r, 0x200, 0x804, 2, data) == 2);
	std::vector<u8> packet (96, 7);
	f.Receive (packet.data (), 95); f.Receive (nullptr, 96); f.Receive (packet.data (), 1024);
	for (unsigned i = 0; i < 20; i++) f.Receive (packet.data (), 96);
	f.Process (); assert (sink.writes == 8 && sink.frames == 192);
	F::TStatistics stats; f.GetStatistics (&stats);
	assert (stats.nMalformed == 3 && stats.nDropped == 12);
	f.Receive (packet.data (), 96); f.SetSuspended (true); f.Process ();
	assert (!sink.active); f.Receive (packet.data (), 96); f.SetSuspended (false); f.Process ();
	assert (sink.active && sink.writes == 8); // No stale queue after suspend
	f.Receive (packet.data (), 96); f.SetConfigured (false); f.Process (); assert (!sink.active);
	f.SetConfigured (true); f.Process (); assert (!sink.active && f.GetInterface (9) == 0);
	sink.good = false; assert (f.SetInterface (9, 2)); f.Process (); assert (!sink.active);
	f.GetStatistics (&stats); assert (stats.nFailures == 1);
	sink.good = true; assert (f.SetInterface (9, 2)); f.Process (); assert (sink.active);
	sink.duringConfigure = [&] { f.SetConfigured (false); };
	assert (f.SetInterface (9, 3)); f.Process ();
	f.Receive (packet.data (), 96); f.Process ();
	assert (!sink.active && f.GetInterface (9) == -1);
	puts ("PASS controls/lifecycle: strict requests, deferred transitions, suspend/reset, failure and stale callbacks");
}
void Fuzz ()
{
	CSink sink; F f (Configuration (), &sink); f.SetConfigured (true);
	std::mt19937 random (1234); u8 data[1024] {};
	for (unsigned i = 0; i < 100000; i++)
	{
		TSetupData s {(u8)random (), (u8)random (), (u16)random (), (u16)random (), (u16)random ()};
		f.ControlRequest (&s, data);
		switch (random () % 7)
		{
		case 0: f.SetInterface (9, random () % 7); break;
		case 1: f.SetConfigured (random () % 2); break;
		case 2: f.SetSuspended (random () % 2); break;
		case 3: f.Receive (data, random () % 1025); break;
		default: f.Process (); break;
		}
	}
	puts ("PASS 100000 deterministic malformed-request/lifecycle interleavings (ASan/UBSan)");
}

void Buffer ()
{
	using B = CUSBAudioGadgetBuffer;
	u8 bytes[4] = {0, 0, 0, 0x80};
	assert (B::DecodeSample (bytes, F::PCM32) == (-2147483647-1));
	bytes[0] = 0xFF; bytes[1] = 0xFF; bytes[2] = 0xFF; bytes[3] = 0x7F;
	assert (B::DecodeSample (bytes, F::PCM32) == 2147483647);
	bytes[0] = 0; bytes[1] = 0; bytes[2] = 0x80; bytes[3] = 0x3F;
	assert (B::DecodeSample (bytes, F::Float32) == 2147483647);
	bytes[3] = 0xBF; assert (B::DecodeSample (bytes, F::Float32) == (-2147483647-1));
	bytes[3] = 0x7F; assert (B::DecodeSample (bytes, F::Float32) == 0);
	bytes[3] = 0x3E; assert (B::DecodeSample (bytes, F::Float32) == 536870912);
	for (unsigned n = 0; n < 256; n++)
	{
		bytes[0] = n;
		assert (B::DecodeSample (bytes, F::U8) == (int(n)-128)*16777216);
		assert (B::DecodeSample (bytes, F::PCM8) == (int(n)-((n&128)?256:0))*16777216);
	}
	for (int drift : {-800, 0, 800})
	{
		B b (48000, 2, 512); assert (b.IsValid ());
		F::TStream stream {F::PCM16, 2, 48000}; assert (b.Configure (stream));
		u8 packet[49*4];
		for (unsigned i = 0; i < 49; i++)
		{
			packet[i*4] = 0; packet[i*4+1] = 0x20;
			packet[i*4+2] = 0; packet[i*4+3] = 0xE0;
		}
		s32 out[96]; long long fraction = 0; unsigned gaps = 0;
		// 120 seconds of deterministic clock drift, packetized at 1 ms.
		for (unsigned ms = 0; ms < 120000; ms++)
		{
			fraction += 48000LL*(1000000+drift);
			unsigned frames = fraction/1000000000LL; fraction %= 1000000000LL;
			assert (b.Write (packet, frames, stream));
			unsigned rendered = b.ReadFrames (out, 48);
			if (ms > 1000 && rendered != 48) ++gaps;
			if (rendered == 48) for (unsigned i = 0; i < 48; i++)
			{
				assert (out[2*i] == 536870912 && out[2*i+1] == -536870912);
			}
		}
		assert (!gaps);
		assert (b.GetCorrectionPPM () >= -2000 && b.GetCorrectionPPM () <= 2000);
		b.SetControls (true, 0); b.ReadFrames (out, 48); for (auto n : out) assert (!n);
		b.SetControls (false, -6*256); b.ReadFrames (out, 48);
		assert (out[0] > 260000000 && out[0] < 280000000);
		b.Stop (); b.ReadFrames (out, 48); for (auto n : out) assert (!n);
	}
	for (unsigned rate : {32000u, 44100u, 48000u})
	{
		B b (48000, 2, 512); F::TStream stream {F::PCM16, 2, rate}; assert (b.Configure (stream));
		u8 packet[192] {}; s32 out[96]; unsigned fraction = 0;
		for (unsigned i = 0; i < 10000; i++)
		{
			fraction += rate; unsigned frames = fraction/1000; fraction %= 1000;
			assert (b.Write (packet, frames, stream));
			unsigned n = b.ReadFrames (out, 48); if (i > 100) assert (n == 48);
		}
	}
	for (unsigned rate : {44100u, 48000u})
	{
		unsigned frames = (rate + 999) / 1000;
		F::TStream stream {F::PCM16, 2, rate};
		B tooSmall (48000, 2, frames-1), exact (48000, 2, frames);
		u8 packet[48*4] {};
		assert (!tooSmall.Configure (stream));
		assert (exact.Configure (stream) && exact.Write (packet, frames, stream));
	}
	// PCM32 exact endpoints, deliberately unaligned input and a non-power-of-two
	// ring. Repeated four-frame writes cross both contiguous and split spans.
	for (unsigned channels = 1; channels <= 8; channels++)
	{
		B b (16000, channels, 17);
		F::TStream stream {F::PCM32, channels, 16000};
		assert (b.Configure (stream));
		std::vector<u8> input (17*channels*4+1, 0);
		for (unsigned frame = 0; frame < 17; frame++)
		for (unsigned ch = 0; ch < channels; ch++)
		{
			u32 value = ch%2 ? 0x7FFFFFFF : 0x80000000;
			for (unsigned byte = 0; byte < 4; byte++)
				input[1+(frame*channels+ch)*4+byte] = (u8) (value >> (byte*8));
		}
		assert (b.Write (input.data ()+1, 17, stream));
		assert (b.Write (input.data ()+1, 0, stream));
		assert (!b.Write (input.data ()+1, 18, stream));
		s32 out[4*8];
		for (unsigned n = 0; n < 100; n++)
		{
			assert (b.ReadFrames (out, 4) == 4);
			for (unsigned frame = 0; frame < 4; frame++)
			for (unsigned ch = 0; ch < channels; ch++)
				assert (out[frame*channels+ch] == (ch%2 ? 2147483647 : (-2147483647-1)));
			assert (b.Write (input.data ()+1, 4, stream));
		}
	}
	puts ("PASS adaptive buffer: signed/float conversion, controls, rates, 120-second +/-800 ppm simulations");
}
void WrapperTests (void);
void StringTests ();
int main () { StringTests (); Descriptors (); ControlsAndLifecycle (); Fuzz (); Buffer (); WrapperTests (); }
