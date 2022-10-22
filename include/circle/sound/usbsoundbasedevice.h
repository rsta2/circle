//
// usbsoundbasedevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_usbsoundbasedevice_h
#define _circle_usb_usbsoundbasedevice_h

#include <circle/sound/soundbasedevice.h>
#include <circle/usb/usbaudiostreaming.h>
#include <circle/device.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CUSBSoundBaseDevice : public CSoundBaseDevice	/// High-level driver for USB audio streaming devices
{
public:
	/// \param nSampleRate Requested sample rate in Hz
	CUSBSoundBaseDevice (unsigned nSampleRate = 48000);

	~CUSBSoundBaseDevice (void);

	/// \return Minium value of one sample
	int GetRangeMin (void) const override;
	/// \return Maximum value of one sample
	int GetRangeMax (void) const override;

	/// \brief Starts USB audio streaming
	boolean Start (void) override;

	/// \brief Cancels USB audio streaming
	/// \note Cancel takes effect after a short delay
	void Cancel (void) override;

	/// \return Is USB audio streaming running?
	boolean IsActive (void) const override;

protected:
	/// \brief May overload this to provide the sound samples
	/// \param pBuffer    Buffer where the samples have to be placed
	/// \param nChunkSize Size of the buffer in words
	/// \return Number of words written to the buffer (normally nChunkSize),\n
	///	    Transfer will stop if 0 is returned
	/// \note Each sample consists of two words (Left channel, right channel)\n
	///	  Each word must be between GetRangeMin() and GetRangeMax()
	/// virtual unsigned GetChunk (s16 *pBuffer, unsigned nChunkSize);

private:
	boolean SendChunk (void);

	void CompletionRoutine (void);
	static void CompletionStub (void *pParam);

	static void DeviceRemovedHandler (CDevice *pDevice, void *pContext);

private:
	enum TDeviceState
	{
		StateCreated,
		StateIdle,
		StateRunning,
		StateCanceled,
		StateUnknown
	};

private:
	unsigned m_nSampleRate;

	TDeviceState m_State;
	CUSBAudioStreamingDevice *m_pUSBDevice;

	unsigned m_nChunkSizeBytes;
	u8 *m_pBuffer[2];
	unsigned m_nCurrentBuffer;

	CSpinLock m_SpinLock;
};

#endif
