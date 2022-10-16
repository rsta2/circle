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
#include <circle/logger.h>
#include <assert.h>

LOGMODULE ("sndusb");
static const char DeviceName[] = "sndusb";

CUSBSoundBaseDevice::CUSBSoundBaseDevice (unsigned nSampleRate)
:	CSoundBaseDevice (SoundFormatSigned16, 0, nSampleRate),
	m_nSampleRate (nSampleRate),
	m_State (StateCreated),
	m_pUSBDevice (nullptr),
	m_nChunkSizeBytes (0),
	m_pBuffer (nullptr)
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

	delete [] m_pBuffer;
	m_pBuffer = nullptr;
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
		assert (!m_pUSBDevice);
		m_pUSBDevice = static_cast<CUSBAudioStreamingDevice *>
			(CDeviceNameService::Get ()->GetDevice ("uaudio1", FALSE));
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
		assert (!m_pBuffer);
		m_pBuffer = new u8[m_nChunkSizeBytes * 2];

		m_State = StateIdle;
	}

	assert (m_pBuffer);
	assert (m_nChunkSizeBytes);
	assert (m_nChunkSizeBytes % sizeof (s16) == 0);
	unsigned nChunkSize = GetChunk (reinterpret_cast<s16 *> (m_pBuffer),
					m_nChunkSizeBytes / sizeof (s16));
	if (!nChunkSize)
	{
		LOGWARN ("No sound data");

		return FALSE;
	}

	m_State = StateRunning;

	assert (m_pUSBDevice);
	if (!m_pUSBDevice->SendChunk (m_pBuffer, nChunkSize * sizeof (s16), CompletionStub, this))
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
		m_State = StateCanceled;
	}

	m_SpinLock.Release ();
}

boolean CUSBSoundBaseDevice::IsActive (void) const
{
	return    m_State == StateRunning
	       || m_State == StateCanceled;
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

	assert (m_pBuffer);

	unsigned nChunkSizeBytes = m_pUSBDevice->GetChunkSizeBytes ();
	assert (nChunkSizeBytes);
	assert (nChunkSizeBytes % sizeof (s16) == 0);
	assert (nChunkSizeBytes <= m_nChunkSizeBytes * 2);
	unsigned nChunkSize = GetChunk (reinterpret_cast<s16 *> (m_pBuffer),
					nChunkSizeBytes / sizeof (s16));
	if (!nChunkSize)
	{
		m_State = StateIdle;

		return;
	}

	assert (m_pUSBDevice);
	if (!m_pUSBDevice->SendChunk (m_pBuffer, nChunkSize * sizeof (s16), CompletionStub, this))
	{
		LOGWARN ("Cannot send chunk");

		m_State = StateIdle;

		return;
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

	pThis->m_pUSBDevice = nullptr;

	delete [] pThis->m_pBuffer;
	pThis->m_pBuffer = nullptr;

	pThis->m_nChunkSizeBytes = 0;
}
