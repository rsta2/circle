// SPDX-License-Identifier: GPL-3.0-or-later
// Host differential regression and benchmark of the production adaptive buffer.
#include <circle/usb/gadget/usbaudiogadgetbuffer.h>
#undef _circle_usb_gadget_usbaudiogadgetbuffer_h
#define CUSBAudioGadgetBuffer CReferenceAudioBuffer
#include <circle/usb/gadget/usbaudiogadgetbuffer.h>
#undef CUSBAudioGadgetBuffer
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

unsigned g_nLocked = 0;
using F = CUSBAudioGadgetFunction;

static void Regression ()
{
	std::mt19937 Random (821);
	unsigned nCases = 0;
	for (unsigned nChannels = 1; nChannels <= 8; nChannels++)
	for (unsigned nCapacity : {24U, 25U, 257U, 512U})
	for (unsigned nFormat = 0; nFormat < 5; nFormat++)
	for (unsigned nRate : {8000U, 24000U})
	{
		F::TStream Stream {(F::TFormat) nFormat, nChannels, nRate};
		CUSBAudioGadgetBuffer Actual (16000, nChannels, nCapacity);
		CReferenceAudioBuffer Expected (16000, nChannels, nCapacity);
		assert (Actual.Configure (Stream) && Expected.Configure (Stream));
		unsigned nFrameBytes = nChannels*F::GetSubframeSize (Stream.Format);
		std::vector<u8> Input ((nCapacity+1)*nFrameBytes+4);
		std::vector<s32> A ((nCapacity+1)*nChannels), B (A.size ());
		for (unsigned n = 0; n < 1000; n++)
		{
			for (auto &Byte : Input) Byte = (u8) Random ();
			// Include unaligned packets, zero/full/oversized writes, wraparound,
			// overflow, starvation, up/downsampling, mute, gain and restarts.
			unsigned nFrames = n < 3 ? nCapacity+n-1 : Random () % (nCapacity+2);
			const u8 *p = Input.data () + n%4;
			assert (Actual.Write (p, nFrames, Stream) == Expected.Write (p, nFrames, Stream));
			if (n%31 == 0)
			{
				boolean bMute = n%62 == 0;
				Actual.SetControls (bMute, -6*256);
				Expected.SetControls (bMute, -6*256);
			}
			nFrames = Random () % (nCapacity+2);
			assert (Actual.ReadFrames (A.data (), nFrames) == Expected.ReadFrames (B.data (), nFrames));
			assert (!memcmp (A.data (), B.data (), nFrames*nChannels*sizeof (s32)));
			assert (Actual.GetBufferedFrames () == Expected.GetBufferedFrames ());
			assert (Actual.GetCorrectionPPM () == Expected.GetCorrectionPPM ());
			if (n%101 == 0)
			{
				Actual.Stop (); Expected.Stop ();
				assert (Actual.Configure (Stream) && Expected.Configure (Stream));
			}
		}
		++nCases;
	}
	printf ("PASS %u differential scenarios, 1000 operations each\n", nCases);
}

template <class TBuffer>
static double Measure (F::TFormat Format, unsigned nChannels, bool bRead, unsigned long long &Checksum)
{
	const unsigned nFrames = nChannels == 8 ? 24 : 48;
	const unsigned nRate = nFrames*1000;
	TBuffer Buffer (nRate, nChannels, 257);
	F::TStream Stream {Format, nChannels, nRate};
	assert (Buffer.Configure (Stream));
	std::vector<u8> Input (nFrames*nChannels*F::GetSubframeSize (Format)+1);
	for (unsigned i = 0; i < Input.size (); i++) Input[i] = (u8) (i*37);
	s32 Output[48*8];
	const unsigned nIterations = 100000;
	auto Start = std::chrono::steady_clock::now ();
	for (unsigned n = 0; n < nIterations; n++)
	{
		Checksum += Buffer.Write (Input.data ()+1, nFrames, Stream);
		if (bRead)
		{
			Checksum += Buffer.ReadFrames (Output, nFrames);
			Checksum += (u32) Output[n % (nFrames*nChannels)];
		}
	}
	auto End = std::chrono::steady_clock::now ();
	return std::chrono::duration<double, std::nano> (End-Start).count () / nIterations;
}

int main (int argc, char **)
{
	if (argc == 1)
	{
		Regression ();
		return 0;
	}
	puts ("format,channels,operation,round,baseline_ns,optimized_ns");
	for (auto Format : {F::PCM16, F::PCM32})
	for (unsigned nChannels : {2U, 8U})
	for (bool bRead : {false, true})
	for (unsigned nRound = 0; nRound < 9; nRound++)
	{
		unsigned long long A = 0, B = 0;
		double a, b;
		if (nRound%2)
		{
			b = Measure<CUSBAudioGadgetBuffer> (Format, nChannels, bRead, B);
			a = Measure<CReferenceAudioBuffer> (Format, nChannels, bRead, A);
		}
		else
		{
			a = Measure<CReferenceAudioBuffer> (Format, nChannels, bRead, A);
			b = Measure<CUSBAudioGadgetBuffer> (Format, nChannels, bRead, B);
		}
		assert (A == B);
		printf ("%s,%u,%s,%u,%.3f,%.3f\n", Format == F::PCM32 ? "PCM32" : "PCM16",
			nChannels, bRead ? "write+read" : "write", nRound, a, b);
	}
}
