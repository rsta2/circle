// SPDX-License-Identifier: GPL-3.0-or-later
//
// usbaudiogadgetfunction.h
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
#ifndef _circle_usb_gadget_usbaudiogadgetfunction_h
#define _circle_usb_gadget_usbaudiogadgetfunction_h

#include <circle/usb/usb.h>
#include <circle/spinlock.h>
#include <circle/types.h>

/// One UAC1 full-speed adaptive OUT stream, independent of the USB controller.
/// All public methods are single-core, task/IRQ safe. Process() is task-only
/// and must not be called recursively. No application callbacks run in IRQ.
class CUSBAudioGadgetFunction
{
public:
	static const unsigned MaxAlternates = 5;
	static const unsigned MaxRates = 16;
	static const unsigned MaxChannels = 8;
	static const unsigned MaxPacketSize = 1023;
	static const unsigned MaxDescriptorSize = 512;
	static const unsigned QueuePackets = 8;

	enum TFormat { PCM8, PCM16, PCM24, PCM32, Float32 };

	struct TAlternate
	{
		TFormat Format;
		unsigned nRates;
		unsigned Rates[MaxRates];
	};

	struct TConfiguration
	{
		unsigned nControlInterface;
		unsigned nStreamingInterface;
		unsigned nEndpoint; // OUT endpoint number, 1..7; reserve in composite gadget
		unsigned nInputTerminal;
		unsigned nFeatureUnit;
		unsigned nOutputTerminal;
		unsigned nChannels; // Same channel cluster for all alternates
		u16 wChannelConfig; // UAC1 spatial map, or zero for unspecified channels
		unsigned nAlternates;
		TAlternate Alternates[MaxAlternates]; // USB alternate numbers are index+1
	};

	struct TStream
	{
		TFormat Format;
		unsigned nChannels;
		unsigned nSampleRate;
	};

	struct TStatistics
	{
		unsigned nPackets;
		unsigned nMalformed;
		unsigned nDropped;
		unsigned nTransitions;
		unsigned nFailures;
	};

	/// The sink MUST adapt its consumption clock to USB (or resample).
	/// A fixed free-running DAC clock without adaptation is not sufficient.
	class CSink
	{
	public:
		virtual ~CSink (void) {}
		/// Stop and discard old audio. Called before configuration changes.
		virtual void Stop (void) = 0;
		/// Configure and prepare playback; FALSE leaves the stream muted.
		/// Must return in bounded time; do not call Process() from here.
		virtual boolean Configure (const TStream &Stream) = 0;
		/// Master controls: signed dB/256, -96 dB..0 dB. Sink applies both.
		virtual void SetControls (boolean bMute, int nVolume) = 0;
		/// Consume/copy interleaved little-endian samples; valid only during call.
		/// Returns FALSE if the sink cannot accept this packet (counted as drop).
		virtual boolean Write (const void *pData, unsigned nFrames,
				       const TStream &Stream) = 0;
	};

	CUSBAudioGadgetFunction (const TConfiguration &Config, CSink *pSink);
	boolean IsValid (void) const;

	/// Generate AC+AS descriptors (without configuration header or IAD).
	/// Returns bytes written, zero on invalid config/insufficient capacity.
	unsigned GetDescriptor (void *pBuffer, unsigned nSize) const;
	unsigned GetMaxPacketSize (void) const;
	unsigned GetStreamingInterface (void) const { return m_Config.nStreamingInterface; }
	/// Maximum packet length for this USB alternate, zero for idle or invalid.
	unsigned GetAlternatePacketSize (unsigned nAlternate) const;

	boolean SetInterface (unsigned nInterface, unsigned nAlternate);
	int GetInterface (unsigned nInterface);
	/// Returns response length, zero for accepted SET, -1 for STALL.
	int ControlRequest (const TSetupData *pSetup, u8 *pData);

	void SetConfigured (boolean bConfigured);
	void SetSuspended (boolean bSuspended);
	void Receive (const void *pData, unsigned nLength);
	/// Apply pending transitions/controls and deliver at most QueuePackets.
	void Process (void);
	void GetStatistics (TStatistics *pStatistics);

	static unsigned GetSubframeSize (TFormat Format);
	static unsigned GetPacketSize (unsigned nRate, unsigned nChannels, TFormat Format);

private:
	void Changed (void); // lock held
	unsigned NearestRate (unsigned nAlternate, unsigned nRate) const;
	unsigned PacketSize (unsigned nAlternate) const;

	TConfiguration m_Config;
	CSink *m_pSink;
	boolean m_bValid;
	CSpinLock m_Lock;
	boolean m_bConfigured;
	boolean m_bSuspended;
	boolean m_bReady;
	unsigned m_nAlternate;
	unsigned m_nRate;
	unsigned m_nRevision;
	unsigned m_nAppliedRevision;
	boolean m_bMute;
	int m_nVolume;
	boolean m_bControlsPending;
	TStream m_Stream;
	struct TPacket { unsigned nLength; u8 Data[MaxPacketSize]; };
	TPacket m_Queue[QueuePackets];
	unsigned m_nHead;
	unsigned m_nTail;
	unsigned m_nCount;
	TStatistics m_Statistics;
};

#endif
