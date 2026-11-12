// SPDX-License-Identifier: GPL-3.0-or-later
//
// main.cpp
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
// Software-path test: no emulated USB device controller or PWM is assumed.
#include <circle/usb/gadget/usbaudiogadgetfunction.h>
#include <circle/usb/gadget/usbaudiogadgetbuffer.h>
#include <circle/qemu.h>
#include <circle/startup.h>
#include <qemu/qemuhostfile.h>

static void Message (const char *pText)
{
	CallSemihostingSingle (SEMIHOSTING_SYS_WRITE0, (uintptr) pText);
}

static void Check (boolean bOK)
{
	if (!bOK)
	{
		Message ("FAIL QEMU audio check\n");
		SemihostingExit (1);
	}
}

int main (void)
{
	Message ("Circle ARM UAC1 software path in QEMU\n");
	using F = CUSBAudioGadgetFunction;
	F::TConfiguration Config {};
	Config.nControlInterface = 0;
	Config.nStreamingInterface = 1;
	Config.nEndpoint = 1;
	Config.nInputTerminal = 1;
	Config.nFeatureUnit = 2;
	Config.nOutputTerminal = 3;
	Config.nChannels = 2;
	Config.wChannelConfig = 3;
	Config.nAlternates = F::MaxAlternates;
	const unsigned Rates[] = {32000, 44100, 48000};
	for (unsigned i = 0; i < Config.nAlternates; i++)
	{
		Config.Alternates[i].Format = (F::TFormat) i;
		Config.Alternates[i].nRates = 3;
		for (unsigned j = 0; j < 3; j++)
		{
			Config.Alternates[i].Rates[j] = Rates[j];
		}
	}

	CUSBAudioGadgetBuffer Buffer (48000, 2, 512);
	F Function (Config, &Buffer);
	Check (Function.IsValid () && Buffer.IsValid ());
	u8 Descriptor[512];
	Check (Function.GetDescriptor (Descriptor, sizeof Descriptor) > 0);
	Function.SetConfigured (TRUE);

	{
		CQEMUHostFile Input ("input.bin", FALSE);
		CQEMUHostFile Output ("rendered.s32", TRUE);
		Check (Input.IsOpen () && Output.IsOpen ());
		u32 PacketStorage[97];
		u8 *Packet = (u8 *) PacketStorage+1; // Exercise unaligned USB data, including PCM32.
		s32 Samples[96];
		for (unsigned i = 0; i < Config.nAlternates; i++)
		{
			for (unsigned j = 0; j < 3; j++)
			{
				Check (Function.SetInterface (1, i+1));
				u8 Rate[] = {(u8) Rates[j], (u8) (Rates[j]>>8), (u8) (Rates[j]>>16)};
				TSetupData Request {0x22, 1, 0x100, 1, 3};
				Check (Function.ControlRequest (&Request, Rate) == 0);
				Function.Process ();
				unsigned nFraction = 0;
				for (unsigned n = 0; n < 1000; n++)
				{
					nFraction += Rates[j];
					unsigned nFrames = nFraction / 1000;
					nFraction %= 1000;
					unsigned nBytes = nFrames * 2 * F::GetSubframeSize ((F::TFormat) i);
					Check (Input.Read (Packet, nBytes) == (int) nBytes);
					Function.Receive (Packet, nBytes);
					Function.Process ();
					unsigned nRendered = Buffer.ReadFrames (Samples, 48);
					Check (n <= 30 || nRendered == 48);
					Check (Output.Write (Samples, sizeof Samples) == sizeof Samples);
				}
				Message ("PASS format/rate render\n");
			}
		}
		Check (Input.Read (Packet, 1) == 0);
	}

	// Exercise mute, alternate zero, suspend and resume using the same code.
	u8 Mute = 1;
	TSetupData Control {0x21, 1, 0x100, 0x200, 1};
	Check (Function.ControlRequest (&Control, &Mute) == 0);
	Function.Process ();
	s32 Samples[96];
	Buffer.ReadFrames (Samples, 48);
	for (unsigned i = 0; i < 96; i++) Check (Samples[i] == 0);
	Function.SetSuspended (TRUE);
	Function.Process ();
	Check (Buffer.GetBufferedFrames () == 0);
	Function.SetSuspended (FALSE);
	Function.Process ();
	Check (Function.SetInterface (1, 0));
	Function.Process ();
	Check (Buffer.GetBufferedFrames () == 0);
	F::TStatistics Statistics;
	Function.GetStatistics (&Statistics);
	Check (Statistics.nPackets == 18000 && !Statistics.nMalformed
	       && !Statistics.nDropped && !Statistics.nFailures);
	Message ("PASS 18000 packets; no drops, malformed packets or failed transitions\n");
	Message ("PASS mute, suspend/resume and alternate zero\n");
	SemihostingExit (0);
	return EXIT_HALT;
}
