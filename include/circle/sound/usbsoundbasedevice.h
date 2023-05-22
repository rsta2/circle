//
// usbsoundbasedevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2022-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/sound/usbsoundcontroller.h>
#include <circle/usb/usbaudiostreaming.h>
#include <circle/device.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CUSBSoundBaseDevice : public CSoundBaseDevice	/// High-level driver for USB audio streaming devices
{
public:
	enum TDeviceMode
	{
		DeviceModeTXOnly,	///< Streaming output
		DeviceModeRXOnly,	///< Streaming input
		DeviceModeTXRX,		///< Streaming output and input
		DeviceModeUnknown
	};

public:
	/// \param nSampleRate Requested sample rate in Hz
	/// \param DeviceMode Select output operation, input or both
	/// \param nDevice 0-based index of USB audio device to be used
	CUSBSoundBaseDevice (unsigned nSampleRate   = 48000,
			     TDeviceMode DeviceMode = DeviceModeTXOnly,
			     unsigned nDevice       = 0);

	~CUSBSoundBaseDevice (void);

	/// \brief Starts USB audio streaming
	boolean Start (void) override;

	/// \brief Cancels USB audio streaming
	/// \note Cancel takes effect after a short delay
	void Cancel (void) override;

	/// \return Is USB audio streaming running?
	boolean IsActive (void) const override;

	/// \return Pointer to sound controller object
	CSoundController *GetController (void) override;

protected:
	/// \brief May override this to provide the sound samples
	/// \param pBuffer    Buffer where the samples have to be placed
	/// \param nChunkSize Size of the buffer in words
	/// \return Number of words written to the buffer (normally nChunkSize),\n
	///	    Transfer will stop if 0 is returned
	/// \note Each sample consists of two words (Left channel, right channel)\n
	///	  Each word must be between GetRangeMin() and GetRangeMax()
	/// virtual unsigned GetChunk (s16 *pBuffer, unsigned nChunkSize);

	/// \brief May override this to consume the received sound samples
	/// \param pBuffer    Buffer where the samples have been placed
	/// \param nChunkSize Size of the buffer in words
	/// \note Each sample consists of two words (Left channel, right channel)
	/// virtual void PutChunk (const s16 *pBuffer, unsigned nChunkSize);

private:
	// get streaming device with 0-based index of the given kind (TX or RX)
	CUSBAudioStreamingDevice *GetStreamingDevice (boolean bTX, unsigned nIndex);

	boolean SendChunk (void);
	boolean ReceiveChunk (void);

	void TXCompletionRoutine (unsigned nBytesTransferred);
	static void TXCompletionStub (unsigned nBytesTransferred, void *pParam);

	void RXCompletionRoutine (unsigned nBytesTransferred);
	static void RXCompletionStub (unsigned nBytesTransferred, void *pParam);

	void DeviceRemovedHandler (CDevice *pDevice);
	static void DeviceRemovedStub (CDevice *pDevice, void *pContext);

	friend class CUSBSoundController;
	boolean SetTXInterface (unsigned nInterface);

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
	unsigned m_nBitResolution;
	unsigned m_nSubframeSize;

	unsigned m_nSampleRate;
	TDeviceMode m_DeviceMode;
	unsigned m_nDevice;
	unsigned m_nTXInterface;

	TDeviceState m_State;
	CUSBAudioStreamingDevice *m_pTXUSBDevice;
	CUSBAudioStreamingDevice *m_pRXUSBDevice;

	unsigned m_nTXChunkSizeBytes;
	u8 *m_pTXBuffer[2];
	unsigned m_nTXCurrentBuffer;

	unsigned m_nRXChunkSizeBytes;
	u8 *m_pRXBuffer[2];
	unsigned m_nRXCurrentBuffer;

	int m_nOutstanding;

	CSpinLock m_SpinLock;

	CUSBSoundController *m_pSoundController;

	TRegistrationHandle m_hRemoveRegistration;
};

#endif
