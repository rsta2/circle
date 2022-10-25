//
// usbsoundbasedevice.cpp
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
#include <circle/sound/usbsoundbasedevice.h>
#include <circle/devicenameservice.h>
#include <circle/sched/scheduler.h>
#include <circle/sysconfig.h>
#include <circle/string.h>
#include <circle/logger.h>
#include <assert.h>

LOGMODULE ("sndusb");
static const char DeviceName[] = "sndusb";

CUSBSoundBaseDevice::CUSBSoundBaseDevice (unsigned nSampleRate, unsigned nDevice)
:	CSoundBaseDevice (SoundFormatSigned16, 0, nSampleRate),
	m_nSampleRate (nSampleRate),
	m_nDevice (nDevice),
	m_nInterface (0),
	m_State (StateCreated),
	m_pUSBDevice (nullptr),
	m_nChunkSizeBytes (0),
	m_pBuffer {nullptr, nullptr},
	m_nCurrentBuffer (0),
	m_SoundController (this, nDevice)
{
	CDeviceNameService::Get ()->AddDevice (DeviceName, this, FALSE);
}

CUSBSoundBaseDevice::~CUSBSoundBaseDevice (void)
{
	assert (   m_State == StateCreated
		|| m_State == StateIdle);
	m_State = StateUnknown;

	CDeviceNameService::Get ()->RemoveDevice (DeviceName, FALSE);

	m_pUSBDevice = nullptr;

	delete [] m_pBuffer[0];
	delete [] m_pBuffer[1];
	m_pBuffer[0] = nullptr;
	m_pBuffer[1] = nullptr;
}

int CUSBSoundBaseDevice::GetRangeMin (void) const
{
	return -32768;
}

int CUSBSoundBaseDevice::GetRangeMax (void) const
{
	return 32767;
}

boolean CUSBSoundBaseDevice::Start (void)
{
	assert (   m_State == StateCreated
		|| m_State == StateIdle);
	if (m_State == StateCreated)
	{
		if (!m_SoundController.Probe ())
		{
			LOGWARN ("Probing sound controller failed");

			return FALSE;
		}

		CString USBDeviceName;
		USBDeviceName.Format ("uaudio%u-%u", m_nDevice+1, m_nInterface+1);

		assert (!m_pUSBDevice);
		m_pUSBDevice = static_cast<CUSBAudioStreamingDevice *>
			(CDeviceNameService::Get ()->GetDevice (USBDeviceName, FALSE));
		if (!m_pUSBDevice)
		{
			LOGWARN ("USB audio streaming device not found");

			return FALSE;
		}

		m_pUSBDevice->RegisterRemovedHandler (DeviceRemovedHandler, this);

		if (!m_pUSBDevice->Setup (m_nSampleRate))
		{
			LOGWARN ("USB audio device setup failed");

			return FALSE;
		}

		assert (!m_nChunkSizeBytes);
		m_nChunkSizeBytes = m_pUSBDevice->GetChunkSizeBytes ();

		// The actual chunk size varies in operation. A maximum of twice
		// the initial size should not be exceeded.
		assert (!m_pBuffer[0]);
		assert (!m_pBuffer[1]);
		m_pBuffer[0] = new u8[m_nChunkSizeBytes * 2];
		m_pBuffer[1] = new u8[m_nChunkSizeBytes * 2];

		m_State = StateIdle;
	}

	m_nCurrentBuffer = 0;

	assert (m_State == StateIdle);
	m_State = StateRunning;

	// Two pending transfers on Raspberry Pi 4,
	// RPi 1-3 and Zero cannot handle this yet.
	if (   !SendChunk ()
#if RASPPI >= 4
	    || !SendChunk ()
#endif
	   )
	{
		LOGWARN ("Cannot send chunk");

		m_State = StateIdle;

		return FALSE;
	}

	return TRUE;
}

void CUSBSoundBaseDevice::Cancel (void)
{
	m_SpinLock.Acquire ();

	if (m_State == StateRunning)
	{
#if RASPPI >= 4
		m_State = StateCanceled;
#else
		m_State = StateCanceled2;
#endif
	}

	m_SpinLock.Release ();
}

boolean CUSBSoundBaseDevice::IsActive (void) const
{
	return    m_State == StateRunning
	       || m_State == StateCanceled
	       || m_State == StateCanceled2;
}

CSoundController *CUSBSoundBaseDevice::GetController (void)
{
	return &m_SoundController;
}

boolean CUSBSoundBaseDevice::SendChunk (void)
{
	assert (m_pUSBDevice);
	unsigned nChunkSizeBytes = m_pUSBDevice->GetChunkSizeBytes ();
	assert (nChunkSizeBytes);
	assert (nChunkSizeBytes % sizeof (s16) == 0);
	assert (nChunkSizeBytes <= m_nChunkSizeBytes * 2);

	assert (m_nCurrentBuffer < 2);
	assert (m_pBuffer[m_nCurrentBuffer]);
	unsigned nChunkSize = GetChunk (reinterpret_cast<s16 *> (m_pBuffer[m_nCurrentBuffer]),
					nChunkSizeBytes / sizeof (s16));
	if (!nChunkSize)
	{
		return FALSE;
	}

	if (!m_pUSBDevice->SendChunk (m_pBuffer[m_nCurrentBuffer], nChunkSize * sizeof (s16),
				      CompletionStub, this))
	{
		return FALSE;
	}

	m_nCurrentBuffer ^= 1;

	return TRUE;
}

void CUSBSoundBaseDevice::CompletionRoutine (void)
{
	boolean bContinue = FALSE;

	m_SpinLock.Acquire ();

	switch (m_State)
	{
	case StateCreated:
		break;

	case StateRunning:
		bContinue = TRUE;
		break;

	case StateCanceled:
		m_State = StateCanceled2;
		break;

	case StateCanceled2:
		m_State = StateIdle;
		break;

	case StateIdle:
	default:
		assert (0);
		break;
	}

	m_SpinLock.Release ();

	if (!bContinue)
	{
		return;
	}

	if (!SendChunk ())
	{
		LOGWARN ("Cannot send chunk");

		m_State = StateIdle;
	}
}

void CUSBSoundBaseDevice::CompletionStub (void *pParam)
{
	CUSBSoundBaseDevice *pThis = static_cast<CUSBSoundBaseDevice *> (pParam);
	assert (pThis);

	pThis->CompletionRoutine ();
}

void CUSBSoundBaseDevice::DeviceRemovedHandler (CDevice *pDevice, void *pContext)
{
	CUSBSoundBaseDevice *pThis = static_cast<CUSBSoundBaseDevice *> (pContext);
	assert (pThis);

	pThis->m_SpinLock.Acquire ();

	assert (pThis->m_State != StateCreated);
	pThis->m_State = StateCreated;

	pThis->m_SpinLock.Release ();

	pThis->m_nChunkSizeBytes = 0;
}

boolean CUSBSoundBaseDevice::SetInterface (unsigned nInterface)
{
	boolean bWasActive = IsActive ();
	if (bWasActive)
	{
		Cancel ();

		while (IsActive ())
		{
#ifdef NO_BUSY_WAIT
			CScheduler::Get ()->Yield ();
#endif
		}
	}

	assert (   m_State == StateCreated
	        || m_State == StateIdle);
	if (m_State == StateIdle)
	{
		assert (m_pUSBDevice);
		m_pUSBDevice->RegisterRemovedHandler (nullptr);
		m_pUSBDevice = nullptr;

		delete [] m_pBuffer[0];
		delete [] m_pBuffer[1];
		m_pBuffer[0] = nullptr;
		m_pBuffer[1] = nullptr;

		m_nChunkSizeBytes = 0;

		m_State = StateCreated;
	}

	m_nInterface = nInterface;

	if (bWasActive)
	{
		return Start ();
	}

	return TRUE;
}
