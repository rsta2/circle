// SPDX-License-Identifier: GPL-3.0-or-later
//
// usbaudiogadgetbuffer.h
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
#ifndef _circle_usb_gadget_usbaudiogadgetbuffer_h
#define _circle_usb_gadget_usbaudiogadgetbuffer_h

#include <circle/usb/gadget/usbaudiogadgetfunction.h>

/// Optional software-adaptive sink for a fixed-rate DAC.
/// Converts Type-I samples to signed 32-bit, with bounded linear interpolation
/// and queue-fill clock correction. This is a reference resampler, not a
/// high-quality anti-aliasing sample-rate converter. Prefer equal nominal rates.
/// Single core: Write/Configure/Stop in task context, ReadFrames in task or IRQ.
class CUSBAudioGadgetBuffer : public CUSBAudioGadgetFunction::CSink
{
public:
	/// Buffer size >=16 frames. Channels 1..8, output rate 8..192 kHz.
	CUSBAudioGadgetBuffer (unsigned nOutputRate, unsigned nChannels,
			      unsigned nBufferFrames = 512);
	~CUSBAudioGadgetBuffer (void);
	boolean IsValid (void) const;

	void Stop (void) override;
	boolean Configure (const CUSBAudioGadgetFunction::TStream &Stream) override;
	void SetControls (boolean bMute, int nVolume) override;
	boolean Write (const void *pData, unsigned nFrames,
		       const CUSBAudioGadgetFunction::TStream &Stream) override;

	/// Fill all requested interleaved frames, with silence when unready.
	/// Return number of nonsilence-source frames (not a sample-level meter).
	unsigned ReadFrames (s32 *pBuffer, unsigned nFrames);
	unsigned GetBufferedFrames (void);
	int GetCorrectionPPM (void);
	static s32 DecodeSample (const u8 *pData, CUSBAudioGadgetFunction::TFormat Format);

private:
	CSpinLock m_Lock;
	unsigned m_nOutputRate;
	unsigned m_nChannels;
	unsigned m_nCapacity;
	s32 *m_pBuffer;
	unsigned m_nHead;
	unsigned m_nTail;
	unsigned m_nCount;
	boolean m_bConfigured;
	boolean m_bStarted;
	CUSBAudioGadgetFunction::TStream m_Stream;
	u64 m_nPhase;
	u64 m_nStep;
	int m_nCorrectionPPM;
	int m_nGain;
};

#endif
