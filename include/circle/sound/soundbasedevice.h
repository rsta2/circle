//
// soundbasedevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_sound_soundbasedevice_h
#define _circle_sound_soundbasedevice_h

#include <circle/device.h>
#include <circle/sound/soundcontroller.h>
#include <circle/spinlock.h>
#include <circle/types.h>
#include <assert.h>

#define SOUND_MAX_CHANNELS	32
#define SOUND_MAX_SAMPLE_SIZE	(sizeof (u32))
#define SOUND_MAX_FRAME_SIZE	(SOUND_MAX_CHANNELS * SOUND_MAX_SAMPLE_SIZE)

ASSERT_STATIC (SOUND_MAX_CHANNELS+1 == CSoundController::ChannelUnknown);

// IEC958 (S/PDIF)
#define IEC958_HW_CHANNELS		2		// more channels unsupported
#define IEC958_FRAMES_PER_BLOCK		192
#define IEC958_SUBFRAMES_PER_BLOCK	(IEC958_HW_CHANNELS * IEC958_FRAMES_PER_BLOCK)
#define IEC958_STATUS_BYTES		5
#define IEC958_B_FRAME_PREAMBLE		0x0F

enum TSoundFormat			/// All supported formats are interleaved little endian
{
	SoundFormatUnsigned8,		/// Write/Read, HWFormat none
	SoundFormatSigned16,		/// Write/Read, HWFormat output/input
	SoundFormatSigned24,		/// Write/Read, HWFormat output/input (occupies 3 bytes)
	SoundFormatSigned24_32,		/// Write/Read, HWFormat output/input (occupies 4 bytes)
	SoundFormatUnsigned32,		/// HWFormat output only
	SoundFormatIEC958,		/// HWFormat output only
	SoundFormatUnknown
};

typedef void TSoundDataCallback (void *pParam);

/// \note There are two methods to provide the sound samples:\n
///	  1. By overloading GetChunk()\n
///	  2. By using Write()

/// \note There are two methods to retrieve the sound samples:\n
///	  1. By overloading PutChunk()\n
///	  2. By using Read()

/// \note In a multi-core environment all methods, except if otherwise noted,
///	  have to be called or will be called (for callbacks) on core 0.

class CSoundBaseDevice : public CDevice		/// Base class of sound devices
{
public:
	/// \note Call Setup() afterwards to set-up device, before calling any other method.
	CSoundBaseDevice (void);

	/// \param HWFormat    Selects the sound format used by the hardware
	/// \param nRange32    Defines the maximum range value for SoundFormatUnsigned32\n
	///		       (must be 0 otherwise)
	/// \param nSampleRate Sample rate in Hz
	/// \param bSwapChannels Swap stereo channels on output?
	/// \note Sets up device for Stereo on output and input.
	CSoundBaseDevice (TSoundFormat HWFormat, u32 nRange32, unsigned nSampleRate,
			  boolean bSwapChannels = FALSE);

	virtual ~CSoundBaseDevice (void);

	/// \param HWFormat    Selects the sound format used by the hardware
	/// \param nRange32    Defines the maximum range value for SoundFormatUnsigned32\n
	///		       (must be 0 otherwise)
	/// \param nSampleRate Sample rate in Hz
	/// \param nHWTXChannels Number of hardware TX channels (1 .. SOUND_MAX_CHANNELS)
	/// \param nHWRXChannels Number of hardware RX channels (1 .. SOUND_MAX_CHANNELS)
	/// \param bSwapChannels Swap stereo channels on output?
	void Setup (TSoundFormat HWFormat, u32 nRange32, unsigned nSampleRate,
		    unsigned nHWTXChannels, unsigned nHWRXChannels,
		    boolean bSwapChannels);

	/// \return Number of hardware output channels
	/// \note Can be called on any core.
	unsigned GetHWTXChannels (void) const;
	/// \return Number of hardware input channels
	/// \note Can be called on any core.
	unsigned GetHWRXChannels (void) const;

	/// \return Minium value of one sample
	/// \note Can be called on any core.
	virtual int GetRangeMin (void) const;
	/// \return Maximum value of one sample
	/// \note Can be called on any core.
	virtual int GetRangeMax (void) const;

	/// \brief Starts the transmission of sound data
	/// \return Operation successful?
	/// \note Initializes the device at the first call
	virtual boolean Start (void) = 0;

	/// \brief Cancels the transmission of sound data
	/// \note Cancel takes effect after a short delay
	virtual void Cancel (void) = 0;

	/// \return Is sound data transmission running?
	/// \note Can be called on any core.
	virtual boolean IsActive (void) const = 0;

	/// \return Pointer to sound controller object or nullptr, if not supported.
	virtual CSoundController *GetController (void)		{ return nullptr; }

	// Output /////////////////////////////////////////////////////////////

	/// \brief Allocate the queue used for Write()
	/// \param nSizeMsecs Size of the queue in milliseconds duration of the stream
	/// \note Not used, if GetChunk() is overloaded.
	boolean AllocateQueue (unsigned nSizeMsecs);

	/// \brief Allocate the queue used for Write()
	/// \param nSizeFrames Size of the queue in frames of audio
	/// \note Not used, if GetChunk() is overloaded.
	boolean AllocateQueueFrames (unsigned nSizeFrames);

	/// \param Format    Format of sound data used for Write()
	/// \param nChannels 1 or 2 channels
	/// \note Not used, if GetChunk() is overloaded.
	void SetWriteFormat (TSoundFormat Format, unsigned nChannels = 2);

	/// \param pBuffer Contains the samples
	/// \param nCount  Size of the buffer in bytes (multiple of frame size)
	/// \return Number of bytes consumed
	/// \note Not used, if GetChunk() is overloaded.
	/// \note Can be called on any core.
	int Write (const void *pBuffer, size_t nCount);

	/// \return Queue size in number of frames
	/// \note Not used, if GetChunk() is overloaded.
	/// \note Can be called on any core.
	unsigned GetQueueSizeFrames (void);

	/// \return Number of frames available in the queue waiting to be sent
	/// \note Not used, if GetChunk() is overloaded.
	/// \note Can be called on any core.
	unsigned GetQueueFramesAvail (void);

	/// \param pCallback Callback which is called, when more sound data is needed
	/// \param pParam User parameter to be handed over to the callback
	/// \note Is called, when at least half of the queue is empty
	/// \note Not used, if GetChunk() is overloaded.
	void RegisterNeedDataCallback (TSoundDataCallback *pCallback, void *pParam);

	/// \return TRUE: Have to write right channel first into buffer in GetChunk()
	boolean AreChannelsSwapped (void) const;

	// Input //////////////////////////////////////////////////////////////

	/// \brief Allocate the queue used for Read()
	/// \param nSizeMsecs Size of the queue in milliseconds duration of the stream
	/// \note Not used, if PutChunk() is overloaded.
	boolean AllocateReadQueue (unsigned nSizeMsecs);

	/// \brief Allocate the queue used for Read()
	/// \param nSizeFrames Size of the queue in frames of audio
	/// \note Not used, if PutChunk() is overloaded.
	boolean AllocateReadQueueFrames (unsigned nSizeFrames);

	/// \param Format    Format of sound data returned from Read()
	/// \param nChannels 1 or 2 channels
	/// \param bLeftChannel Return left channel (for nChannels = 1)
	/// \note Not used, if PutChunk() is overloaded.
	void SetReadFormat (TSoundFormat Format, unsigned nChannels = 2,
			    boolean bLeftChannel = TRUE);

	/// \param pBuffer Receives the samples
	/// \param nCount  Size of the buffer in bytes (multiple of frame size)
	/// \return Number of bytes returned
	/// \note Not used, if PutChunk() is overloaded.
	/// \note Can be called on any core.
	int Read (void *pBuffer, size_t nCount);

	/// \return Read queue size in number of frames
	/// \note Not used, if PutChunk() is overloaded.
	/// \note Can be called on any core.
	unsigned GetReadQueueSizeFrames (void);

	/// \return Number of frames available in the read queue
	/// \note Not used, if PutChunk() is overloaded.
	/// \note Can be called on any core.
	unsigned GetReadQueueFramesAvail (void);

	/// \param pCallback Callback which is called, when data is available for Read()
	/// \param pParam User parameter to be handed over to the callback
	/// \note Is called, when at least half of the queue is full
	/// \note Not used, if PutChunk() is overloaded.
	void RegisterHaveDataCallback (TSoundDataCallback *pCallback, void *pParam);

protected:
	/// \brief May override this to provide the sound samples
	/// \param pBuffer    Buffer where the samples have to be placed
	/// \param nChunkSize Size of the buffer in words
	/// \return Number of words written to the buffer (normally nChunkSize),\n
	///	    Transfer will stop if 0 is returned
	/// \note Each sample consists of two words (Left channel, right channel)\n
	///	  Each word must be between GetRangeMin() and GetRangeMax()
	virtual unsigned GetChunk (s16 *pBuffer, unsigned nChunkSize);
	virtual unsigned GetChunk (u32 *pBuffer, unsigned nChunkSize);

	/// \brief May override this to consume the received sound samples
	/// \param pBuffer    Buffer where the samples have been placed
	/// \param nChunkSize Size of the buffer in words
	/// \note Each sample consists of two words (Left channel, right channel)
	virtual void PutChunk (const s16 *pBuffer, unsigned nChunkSize);
	virtual void PutChunk (const u32 *pBuffer, unsigned nChunkSize);

	/// \brief Called from GetChunk() to apply framing on IEC958 samples
	/// \param nSample 24-bit signed sample value as u32, upper bits don't care
	/// \param nFrame Number of the IEC958 frame, this sample belongs to (0..191)
	u32 ConvertIEC958Sample (u32 nSample, unsigned nFrame);

private:
	// Output /////////////////////////////////////////////////////////////

	void ConvertSoundFormat (void *pTo, const void *pFrom);

	unsigned GetChunkInternal (void *pBuffer, unsigned nChunkSize);

	unsigned GetQueueBytesFree (void);
	unsigned GetQueueBytesAvail (void);
	void Enqueue (const void *pBuffer, unsigned nCount);
	void Dequeue (void *pBuffer, unsigned nCount);

	// Input //////////////////////////////////////////////////////////////

	void ConvertReadSoundFormat (void *pTo, const void *pFrom);

	void PutChunkInternal (const void *pBuffer, unsigned nChunkSize);

	unsigned GetReadQueueBytesFree (void);
	unsigned GetReadQueueBytesAvail (void);
	void ReadEnqueue (const void *pBuffer, unsigned nCount);
	void ReadDequeue (void *pBuffer, unsigned nCount);

private:
	TSoundFormat m_HWFormat;
	unsigned m_nSampleRate;
	unsigned m_nHWTXChannels;
	unsigned m_nHWRXChannels;
	boolean m_bSwapChannels;

	unsigned m_nHWSampleSize;
	unsigned m_nHWTXFrameSize;
	unsigned m_nHWRXFrameSize;
	unsigned m_nQueueSize;
	unsigned m_nNeedDataThreshold;

	int m_nRangeMin;
	int m_nRangeMax;
	u8 m_NullFrame[SOUND_MAX_FRAME_SIZE];

	TSoundFormat m_WriteFormat;
	unsigned m_nWriteChannels;
	unsigned m_nWriteSampleSize;
	unsigned m_nWriteFrameSize;

	u8 *m_pQueue;			// Ring buffer
	unsigned m_nInPtr;
	unsigned m_nOutPtr;

	TSoundDataCallback *m_pCallback;
	void *m_pCallbackParam;

	CSpinLock m_SpinLock;

	u8 m_uchIEC958Status[IEC958_STATUS_BYTES];

	// Input //////////////////////////////////////////////////////////////

	unsigned m_nReadQueueSize;
	unsigned m_nHaveDataThreshold;

	TSoundFormat m_ReadFormat;
	unsigned m_nReadChannels;
	boolean m_bLeftChannel;
	unsigned m_nReadSampleSize;
	unsigned m_nReadFrameSize;

	u8 *m_pReadQueue;		// Ring buffer
	unsigned m_nReadInPtr;
	unsigned m_nReadOutPtr;

	TSoundDataCallback *m_pReadCallback;
	void *m_pReadCallbackParam;

	CSpinLock m_ReadSpinLock;
};

#endif
