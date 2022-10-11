//
// vchiqsoundbasedevice.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _vc4_sound_vchiqsoundbasedevice_h
#define _vc4_sound_vchiqsoundbasedevice_h

#include <circle/sound/soundbasedevice.h>
#include <circle/interrupt.h>
#include <circle/sched/synchronizationevent.h>
#include <circle/types.h>
#include <vc4/sound/vchiqsoundcontroller.h>
#include <vc4/vchiq/vchiqdevice.h>
#include <vc4/vchi/vchi.h>
#include "vc_vchi_audioserv_defs.h"

enum TVCHIQSoundState
{
	VCHIQSoundCreated,
	VCHIQSoundIdle,
	VCHIQSoundRunning,
	VCHIQSoundCancelled,
	VCHIQSoundTerminating,
	VCHIQSoundError,
	VCHIQSoundUnknown
};

class CVCHIQSoundBaseDevice : public CSoundBaseDevice	/// Low level access to the VCHIQ sound service
{
public:
	/// \param pVCHIQDevice	pointer to the VCHIQ interface device
	/// \param nSampleRate	sample rate in Hz (44100..48000)
	/// \param nChunkSize	number of samples transfered at once
	/// \param Destination	the target device, the sound data is sent to\n
	///			(detected automatically, if equal to VCHIQSoundDestinationAuto)
	CVCHIQSoundBaseDevice (CVCHIQDevice *pVCHIQDevice,
			       unsigned nSampleRate = 44100,
			       unsigned nChunkSize  = 4000,
			       TVCHIQSoundDestination Destination = VCHIQSoundDestinationAuto);

	virtual ~CVCHIQSoundBaseDevice (void);

	/// \return Minium value of one sample
	int GetRangeMin (void) const override;
	/// \return Maximum value of one sample
	int GetRangeMax (void) const override;

	/// \brief Connects to the VCHIQ sound service and starts sending sound data
	/// \return Operation successful?
	boolean Start (void) override;

	/// \brief Stops the transmission of sound data
	/// \note Cancel takes effect after a short delay
	void Cancel (void) override;

	/// \return Is the sound data transmission running?
	boolean IsActive (void) const override;

	/// \param nVolume	Output volume to be set (-10000..400)
	/// \param Destination	the target device, the sound data is sent to\n
	///			(not modified, if equal to VCHIQSoundDestinationUnknown)
	/// \note This method can be called, while the sound data transmission is running.
	void SetControl (int nVolume,
			 TVCHIQSoundDestination Destination = VCHIQSoundDestinationUnknown);

	/// \return Pointer to sound controller object
	CSoundController *GetController (void) override;

protected:
	/// \brief May overload this to provide the sound samples!
	/// \param pBuffer	buffer where the samples have to be placed
	/// \param nChunkSize	size of the buffer in s16 words
	/// \return Number of s16 words written to the buffer (normally nChunkSize),\n
	///	    Transfer will stop if 0 is returned
	/// \note Each sample consists of two words (Left channel, right channel)\n
	///	  Each word must be between GetRangeMin() and GetRangeMax()
	/// virtual unsigned GetChunk (s16 *pBuffer, unsigned nChunkSize);

private:
	int CallMessage (VC_AUDIO_MSG_T *pMessage);	// waits for completion
	int QueueMessage (VC_AUDIO_MSG_T *pMessage);	// does not wait for completion

	int WriteChunk (void);

	void Callback (const VCHI_CALLBACK_REASON_T Reason, void *hMessage);
	static void CallbackStub (void *pParam, const VCHI_CALLBACK_REASON_T Reason, void *hMessage);

private:
	unsigned m_nSampleRate;
	unsigned m_nChunkSize;
	TVCHIQSoundDestination m_Destination;

	volatile TVCHIQSoundState m_State;

	VCHI_INSTANCE_T m_VCHIInstance;
	VCHI_SERVICE_HANDLE_T m_hService;

	CSynchronizationEvent m_Event;
	int m_nResult;

	unsigned m_nWritePos;
	unsigned m_nCompletePos;

	CVCHIQSoundController m_Controller;
};

#endif
