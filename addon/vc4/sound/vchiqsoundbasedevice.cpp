//
// vchiqsoundbasedevice.cpp
//
// Portions have been taken from the Linux VCHIQ ALSA sound driver which is:
//	Copyright 2011 Broadcom Corporation. All rights reserved.
//	Licensed under GPLv2
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
#include <vc4/sound/vchiqsoundbasedevice.h>
#include <circle/devicenameservice.h>
#include <circle/sched/scheduler.h>
#include <circle/logger.h>
#include <assert.h>

#define VOLUME_TO_CHIP(volume)		((unsigned) -(((volume) << 8) / 100))

static const char FromVCHIQSound[] = "sndvchiq";

CVCHIQSoundBaseDevice::CVCHIQSoundBaseDevice (CVCHIQDevice *pVCHIQDevice,
					      unsigned nSampleRate,
					      unsigned nChunkSize,
					      TVCHIQSoundDestination Destination)
:	CSoundBaseDevice (SoundFormatSigned16, 0, nSampleRate),
	m_nSampleRate (nSampleRate),
	m_nChunkSize (nChunkSize),
	m_Destination (Destination),
	m_State (VCHIQSoundCreated),
	m_VCHIInstance (0),
	m_hService (0),
	m_Controller (this, Destination)
{
	//assert (44100 <= nSampleRate && nSampleRate <= 48000);
	assert (Destination < VCHIQSoundDestinationUnknown);

	CDeviceNameService::Get ()->AddDevice ("sndvchiq", this, FALSE);
}

CVCHIQSoundBaseDevice::~CVCHIQSoundBaseDevice (void)
{
	assert (m_State <= VCHIQSoundIdle);

	CDeviceNameService::Get ()->RemoveDevice ("sndvchiq", FALSE);
}

int CVCHIQSoundBaseDevice::GetRangeMin (void) const
{
	return -32768;
}

int CVCHIQSoundBaseDevice::GetRangeMax (void) const
{
	return 32767;
}

boolean CVCHIQSoundBaseDevice::Start (void)
{
	if (m_State > VCHIQSoundIdle)
	{
		return FALSE;
	}

	VC_AUDIO_MSG_T Msg;
	int nResult;

	if (m_State == VCHIQSoundCreated)
	{
		nResult = vchi_initialise (&m_VCHIInstance);
		if (nResult != 0)
		{
			CLogger::Get ()->Write (FromVCHIQSound, LogError,
						"Cannot initialize VCHI (%d)", nResult);

			m_State = VCHIQSoundError;

			return FALSE;
		}

		nResult = vchi_connect (0, 0, m_VCHIInstance);
		if (nResult != 0)
		{
			CLogger::Get ()->Write (FromVCHIQSound, LogError,
						"Cannot connect VCHI (%d)", nResult);

			m_State = VCHIQSoundError;

			return FALSE;
		}

		SERVICE_CREATION_T Params =
		{
			VCHI_VERSION_EX (VC_AUDIOSERV_VER, VC_AUDIOSERV_MIN_VER),
			VC_AUDIO_SERVER_NAME,
			0, 0, 0,		// unused
			CallbackStub, this,
			1, 1, 0			// unused (bulk)
		};

		nResult = vchi_service_open (m_VCHIInstance, &Params, &m_hService);
		if (nResult != 0)
		{
			CLogger::Get ()->Write (FromVCHIQSound, LogError,
						"Cannot open AUDS service (%d)", nResult);

			m_State = VCHIQSoundError;

			return FALSE;
		}

		vchi_service_release (m_hService);

		Msg.type = VC_AUDIO_MSG_TYPE_CONFIG;
		Msg.u.config.channels = 2;
		Msg.u.config.samplerate = m_nSampleRate;
		Msg.u.config.bps = 16;

		nResult = CallMessage (&Msg);
		if (nResult != 0)
		{
			CLogger::Get ()->Write (FromVCHIQSound, LogError,
						"Cannot set config (%d)", nResult);

			m_State = VCHIQSoundError;

			return FALSE;
		}

		Msg.type = VC_AUDIO_MSG_TYPE_CONTROL;
		Msg.u.control.dest = m_Destination;
		Msg.u.control.volume = VOLUME_TO_CHIP (VCHIQ_SOUND_VOLUME_DEFAULT);

		nResult = CallMessage (&Msg);
		if (nResult != 0)
		{
			CLogger::Get ()->Write (FromVCHIQSound, LogError,
						"Cannot set control (%d)", nResult);

			m_State = VCHIQSoundError;

			return FALSE;
		}

		Msg.type = VC_AUDIO_MSG_TYPE_OPEN;

		nResult = QueueMessage (&Msg);
		if (nResult != 0)
		{
			CLogger::Get ()->Write (FromVCHIQSound, LogError,
						"Cannot open audio (%d)", nResult);

			m_State = VCHIQSoundError;

			return FALSE;
		}

		m_State = VCHIQSoundIdle;
	}

	assert (m_State == VCHIQSoundIdle);

	Msg.type = VC_AUDIO_MSG_TYPE_START;

	nResult = QueueMessage (&Msg);
	if (nResult != 0)
	{
		CLogger::Get ()->Write (FromVCHIQSound, LogError,
					"Cannot start audio (%d)", nResult);

		m_State = VCHIQSoundError;

		return FALSE;
	}

	m_State = VCHIQSoundRunning;

	vchi_service_use (m_hService);

	short usPeerVersion = 0;
	nResult = vchi_get_peer_version (m_hService, &usPeerVersion);
	if (nResult != 0)
	{
		vchi_service_release (m_hService);

		CLogger::Get ()->Write (FromVCHIQSound, LogError,
					"Cannot get peer version (%d)", nResult);

		m_State = VCHIQSoundError;

		return FALSE;
	}

	// we need peer version 2, because we do not support bulk transfers
	if (usPeerVersion < 2)
	{
		vchi_service_release (m_hService);

		CLogger::Get ()->Write (FromVCHIQSound, LogError,
					"Peer version does not match (%u)", (unsigned) usPeerVersion);

		m_State = VCHIQSoundError;

		return FALSE;
	}

	m_nWritePos = 0;
	m_nCompletePos = 0;

	nResult = WriteChunk ();
	if (nResult == 0)
	{
		nResult = WriteChunk ();
	}

	if (nResult != 0)
	{
		vchi_service_release (m_hService);

		CLogger::Get ()->Write (FromVCHIQSound, LogError,
					"Cannot write audio data (%d)", nResult);

		m_State = VCHIQSoundError;

		return FALSE;
	}

	vchi_service_release (m_hService);

	return TRUE;
}

void CVCHIQSoundBaseDevice::Cancel (void)
{
	if (m_State != VCHIQSoundRunning)
	{
		return;
	}

	m_State = VCHIQSoundCancelled;
	if (m_nWritePos - m_nCompletePos > 0)
	{
		while (m_State == VCHIQSoundCancelled)
		{
			CScheduler::Get ()->Yield ();
		}
	}
	else
	{
		m_State = VCHIQSoundTerminating;
	}

	assert (m_State == VCHIQSoundTerminating);

	vchi_service_use (m_hService);

	VC_AUDIO_MSG_T Msg;

	Msg.type = VC_AUDIO_MSG_TYPE_STOP;
	Msg.u.stop.draining = 0;

	int nResult = QueueMessage (&Msg);
	if (nResult != 0)
	{
		CLogger::Get ()->Write (FromVCHIQSound, LogError,
					"Cannot stop audio (%d)", nResult);
	}

	vchi_service_release (m_hService);

	m_State = VCHIQSoundIdle;
}

boolean CVCHIQSoundBaseDevice::IsActive (void) const
{
	return m_State >= VCHIQSoundRunning;
}

void CVCHIQSoundBaseDevice::SetControl (int nVolume, TVCHIQSoundDestination Destination)
{
	if (!(VCHIQ_SOUND_VOLUME_MIN <= nVolume && nVolume <= VCHIQ_SOUND_VOLUME_MAX))
	{
		nVolume = VCHIQ_SOUND_VOLUME_DEFAULT;
	}

	if (Destination < VCHIQSoundDestinationUnknown)
	{
		m_Destination = Destination;
	}

	VC_AUDIO_MSG_T Msg;
	Msg.type = VC_AUDIO_MSG_TYPE_CONTROL;
	Msg.u.control.dest = m_Destination;
	Msg.u.control.volume = VOLUME_TO_CHIP (nVolume);

	int nResult = CallMessage (&Msg);
	if (nResult != 0)
	{
		CLogger::Get ()->Write (FromVCHIQSound, LogWarning,
					"Cannot set control (%d)", nResult);
	}
}

CSoundController *CVCHIQSoundBaseDevice::GetController (void)
{
	return &m_Controller;
}

int CVCHIQSoundBaseDevice::CallMessage (VC_AUDIO_MSG_T *pMessage)
{
	m_Event.Clear ();

	int nResult = QueueMessage (pMessage);
	if (nResult == 0)
	{
		m_Event.Wait ();
	}
	else
	{
		m_nResult = nResult;
	}

	return m_nResult;
}

int CVCHIQSoundBaseDevice::QueueMessage (VC_AUDIO_MSG_T *pMessage)
{
	vchi_service_use (m_hService);

	int nResult = vchi_msg_queue (m_hService, pMessage, sizeof *pMessage,
				      VCHI_FLAGS_BLOCK_UNTIL_QUEUED, 0);

	vchi_service_release (m_hService);

	return nResult;
}

int CVCHIQSoundBaseDevice::WriteChunk (void)
{
	s16 Buffer[m_nChunkSize];
	unsigned nWords = GetChunk (Buffer, m_nChunkSize);
	if (nWords == 0)
	{
		m_State = VCHIQSoundIdle;

		return 0;
	}

	unsigned nBytes = nWords * sizeof (s16);

	VC_AUDIO_MSG_T Msg;

	Msg.type = VC_AUDIO_MSG_TYPE_WRITE;
	Msg.u.write.count = nBytes;
	Msg.u.write.max_packet = 4000;
	Msg.u.write.cookie1 = VC_AUDIO_WRITE_COOKIE1;
	Msg.u.write.cookie2 = VC_AUDIO_WRITE_COOKIE2;
	Msg.u.write.silence = 0;

	int nResult = vchi_msg_queue (m_hService, &Msg, sizeof Msg, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, 0);
	if (nResult != 0)
	{
		return nResult;
	}

	m_nWritePos += nBytes;

	u8 *pBuffer8 = (u8 *) Buffer;
	while (nBytes > 0)
	{
		unsigned nBytesToQueue =   nBytes <= Msg.u.write.max_packet
					 ? nBytes
					 : Msg.u.write.max_packet;

		nResult = vchi_msg_queue (m_hService, pBuffer8, nBytesToQueue,
					  VCHI_FLAGS_BLOCK_UNTIL_QUEUED, 0);
		if (nResult != 0)
		{
			return nResult;
		}

		pBuffer8 += nBytesToQueue;
		nBytes -= nBytesToQueue;
	}

	return 0;
}

void CVCHIQSoundBaseDevice::Callback (const VCHI_CALLBACK_REASON_T Reason, void *hMessage)
{
	if (Reason != VCHI_CALLBACK_MSG_AVAILABLE)
	{
		assert (0);
		return;
	}

	vchi_service_use (m_hService);

	VC_AUDIO_MSG_T Msg;
	uint32_t nMsgLen;
	int nResult = vchi_msg_dequeue (m_hService, &Msg, sizeof Msg, &nMsgLen, VCHI_FLAGS_NONE);
	if (nResult != 0)
	{
		vchi_service_release (m_hService);

		m_State = VCHIQSoundError;

		assert (0);
		return;
	}

	switch (Msg.type)
	{
	case VC_AUDIO_MSG_TYPE_RESULT:
		m_nResult = Msg.u.result.success;
		m_Event.Set ();
		break;

	case VC_AUDIO_MSG_TYPE_COMPLETE:
		if (   m_State == VCHIQSoundIdle
		    || m_State == VCHIQSoundError)
		{
			break;
		}
		assert (m_State >= VCHIQSoundRunning);

		if (   Msg.u.complete.cookie1 != VC_AUDIO_WRITE_COOKIE1
		    || Msg.u.complete.cookie2 != VC_AUDIO_WRITE_COOKIE2)
		{
			m_State = VCHIQSoundError;

			assert (0);
			break;
		}

		m_nCompletePos += Msg.u.complete.count & 0x3FFFFFFF;

		// if there is no more than one chunk left queued
		if (m_nWritePos-m_nCompletePos <= m_nChunkSize*sizeof (s16))
		{
			if (m_State == VCHIQSoundCancelled)
			{
				m_State = VCHIQSoundTerminating;

				break;
			}

			if (WriteChunk () != 0)
			{
				assert (0);

				m_State = VCHIQSoundError;
			}
		}
		break;

	default:
		assert (0);
		break;
	}

	vchi_service_release (m_hService);
}

void CVCHIQSoundBaseDevice::CallbackStub (void *pParam, const VCHI_CALLBACK_REASON_T Reason,
					  void *hMessage)
{
	CVCHIQSoundBaseDevice *pThis = (CVCHIQSoundBaseDevice *) pParam;
	assert (pThis != 0);

	pThis->Callback (Reason, hMessage);
}
