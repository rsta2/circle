//
// usbaudiostreaming.h
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
#ifndef _circle_usb_usbaudiostreaming_h
#define _circle_usb_usbaudiostreaming_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/synchronize.h>
#include <circle/numberpool.h>
#include <circle/spinlock.h>
#include <circle/types.h>

/// \note Supports 16-bit signed Stereo PCM output only

class CUSBAudioStreamingDevice : public CUSBFunction	/// Low-level driver for USB audio streaming devices
{
public:
	static const unsigned MaxSampleRatesRanges = 8;

	struct TFormatInfo
	{
		unsigned SampleRateRanges;	///< Number of valid entries in SampleRateRange[]

		struct
		{
			unsigned Min;
			unsigned Max;		///< equal to Min for discrete sample rate
			unsigned Resolution;	///< 0 for discrete sample rate, or if unknown
		}
		SampleRateRange[MaxSampleRatesRanges];
	};

	typedef void TCompletionRoutine (void *pParam);

public:
	CUSBAudioStreamingDevice (CUSBFunction *pFunction);
	~CUSBAudioStreamingDevice (void);

	boolean Initialize (void);
	boolean Configure (void);

	/// \return Format information struct
	TFormatInfo GetFormatInfo (void) const;

	/// \brief Setup format
	/// \param nSampleRate Sample rate in Hz
	/// \return Operation successful?
	boolean Setup (unsigned nSampleRate);

	/// \return Size of a send chunk in bytes
	/// \note Must be called after Setup()
	/// \note Varies in operation, first call returns mean value
	unsigned GetChunkSizeBytes (void) const;

	/// \brief Send a chunk of audio data to the audio streaming device
	/// \param pBuffer Pointer to the audio data buffer
	/// \param nChunkSizeBytes Number of bytes to be send
	/// \param pCompletionRoutine Optional pointer to a completion routine
	/// \param pParam Optional user parameter, handed over to the completion routine
	/// \return Operation successful?
	boolean SendChunk (const void *pBuffer, unsigned nChunkSizeBytes,
			   TCompletionRoutine *pCompletionRoutine = 0, void *pParam = 0);

private:
	static void CompletionHandler (CUSBRequest *pURB, void *pParam, void *pContext);
	static void SyncCompletionHandler (CUSBRequest *pURB, void *pParam, void *pContext);

	void UpdateChunkSize (void);

private:
	boolean m_bVer200;

	CUSBEndpoint *m_pEndpointOut;
	CUSBEndpoint *m_pEndpointSync;		// feedback EP

	TFormatInfo m_FormatInfo;
	unsigned m_nSampleRate;
	volatile unsigned m_nChunkSizeBytes;

	boolean m_bSynchronousSync;
	unsigned m_nPacketsPerChunk;
	u16 m_usPacketSizeBytes[CUSBRequest::MaxIsoPackets];

	volatile boolean m_bSyncEPActive;
	DMA_BUFFER (u32, m_SyncEPBuffer, 1);
	unsigned m_nSyncAccu;

	u8 m_uchClockSourceID;

	CSpinLock m_SpinLock;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;
};

#endif
