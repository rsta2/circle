//
// usbaudiostreaming.h
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
#ifndef _circle_usb_usbaudiostreaming_h
#define _circle_usb_usbaudiostreaming_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbaudiocontrol.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/synchronize.h>
#include <circle/spinlock.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/types.h>

/// \note Supports 16-bit and 24-bit signed Stereo PCM only

class CUSBAudioStreamingDevice : public CUSBFunction	/// Low-level driver for USB audio streaming devices
{
public:
	static const unsigned MaxTerminals = 4;		///< multiple Input Terminals allowed

	static const unsigned MaxSampleRatesRanges = 8;

	struct TDeviceInfo
	{
		boolean IsOutput;		///< Direction output (or input)
		unsigned NumChannels;		///< Number of audio channels

		unsigned SampleRateRanges;	///< Number of valid entries in SampleRateRange[]
		struct
		{
			unsigned Min;
			unsigned Max;		///< equal to Min for discrete sample rate
			unsigned Resolution;	///< 0 for discrete sample rate, or if unknown
		}
		SampleRateRange[MaxSampleRatesRanges];

		unsigned NumTerminals;		///< Number of terminals (always 1 for output)
		struct TTerminalInfo
		{
			u16 TerminalType;	///< Type of the terminal (e.g. Speaker, Microphone)

			boolean MuteSupported;

			boolean VolumeSupported;
			boolean VolumePerChannel;
			int MinVolume;		///< in dB
			int MaxVolume;		///< in dB
		}
		Terminal[MaxTerminals];
	};

	typedef void TCompletionRoutine (unsigned nBytesTransferred, void *pParam);

public:
	CUSBAudioStreamingDevice (CUSBFunction *pFunction);
	~CUSBAudioStreamingDevice (void);

	boolean Initialize (void);
	boolean Configure (void);

	/// \return Device information struct
	TDeviceInfo GetDeviceInfo (void) const;

	/// \brief Setup format
	/// \param nSampleRate Sample rate in Hz
	/// \return Operation successful?
	boolean Setup (unsigned nSampleRate);

	/// \return Size of a chunk in bytes
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

	/// \brief Receive a chunk of audio data from the audio streaming device
	/// \param pBuffer Pointer to the audio data buffer
	/// \param nChunkSizeBytes Maximum number of bytes to be received
	/// \param pCompletionRoutine Optional pointer to a completion routine
	/// \param pParam Optional user parameter, handed over to the completion routine
	/// \return Operation successful?
	boolean ReceiveChunk (void *pBuffer, unsigned nChunkSizeBytes,
			      TCompletionRoutine *pCompletionRoutine = 0, void *pParam = 0);

	/// \brief Select Input Terminal to be used for input
	/// \param nIndex Index of the Input Terminal (0 .. NumTerminals-1)
	/// \return Operation successful?
	/// \note Can be called from TASK_LEVEL only.
	boolean SelectInputTerminal (unsigned nIndex);

	/// \param bEnable Set to TRUE to enable mute, FALSE to disable
	/// \return Operation successful?
	/// \note Can be called from TASK_LEVEL only.
	boolean SetMute (boolean bEnable);

	/// \param nChannel Addressed audio channel (0: all, 1: front left, 2: front right, ...)
	/// \param ndB Volume value to be set (in dB)
	/// \return Operation successful?
	/// \note Can be called from TASK_LEVEL only.
	boolean SetVolume (unsigned nChannel, int ndB);

private:
	boolean InitTerminalControlInfo (CUSBAudioControlDevice *pControlDevice);

	static void CompletionHandler (CUSBRequest *pURB, void *pParam, void *pContext);
	static void SyncCompletionHandler (CUSBRequest *pURB, void *pParam, void *pContext);

	void UpdateChunkSize (void);

private:
	unsigned m_nBitResolution;
	unsigned m_nSubframeSize;

	boolean m_bVer200;

	CUSBEndpoint *m_pEndpointData;
	CUSBEndpoint *m_pEndpointSync;		// feedback EP

	unsigned m_nDataIntervalFactor;

	boolean m_bIsOutput;
	unsigned m_nChannels;

	unsigned m_nTerminals;
	unsigned m_nActiveTerminal;

	TDeviceInfo m_DeviceInfo;
	unsigned m_nSampleRate;
	volatile unsigned m_nChunkSizeBytes;

	boolean m_bSynchronousSync;
	unsigned m_nPacketsPerChunk;
	u16 m_usPacketSizeBytes[CUSBRequest::MaxIsoPackets];

	volatile boolean m_bSyncEPActive;
	DMA_BUFFER (u32, m_SyncEPBuffer, 1);
	unsigned m_nSyncAccu;

	u8 m_uchClockSourceID;
	u8 m_uchSelectorUnitID;
	u8 m_uchFeatureUnitID[MaxTerminals];

	CSpinLock m_SpinLock;

	CString m_DeviceName;

	const char *From;			// for logger
};

#endif
