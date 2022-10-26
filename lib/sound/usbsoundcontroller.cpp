//
// usbsoundcontroller.cpp
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
#include <circle/sound/usbsoundcontroller.h>
#include <circle/sound/usbsoundbasedevice.h>
#include <circle/usb/usbaudio.h>
#include <circle/devicenameservice.h>
#include <assert.h>

CUSBSoundController::CUSBSoundController (CUSBSoundBaseDevice *pSoundDevice, unsigned nDevice)
:	m_pSoundDevice (pSoundDevice),
	m_nDevice (nDevice),
	m_nInterface (0),
	m_pStreamingDevice (nullptr)
{
	assert (m_pSoundDevice);
}

CUSBSoundController::~CUSBSoundController (void)
{
	m_pStreamingDevice = nullptr;
	m_pSoundDevice = nullptr;
}

boolean CUSBSoundController::Probe (void)
{
	assert (m_pSoundDevice);
	assert (!m_pStreamingDevice);

	CString DeviceName;
	DeviceName.Format ("uaudio%u-%u", m_nDevice+1, m_nInterface+1);

	m_pStreamingDevice = static_cast<CUSBAudioStreamingDevice *>
		(CDeviceNameService::Get ()->GetDevice (DeviceName, FALSE));
	if (!m_pStreamingDevice)
	{
		return FALSE;
	}

	m_DeviceInfo = m_pStreamingDevice->GetDeviceInfo ();

	return TRUE;
}

u32 CUSBSoundController::GetOutputProperties (void) const
{
	u32 Result = PropertyDirectionSupported;

	if (m_DeviceInfo.VolumeSupported)
	{
		Result |=   PropertyVolumeSupported
			  | PropertyVolumePerChannel;
	}

	return Result;
}

boolean CUSBSoundController::EnableJack (TJack Jack)
{
	assert (m_pSoundDevice);

	unsigned nBestInterface = 999;
	unsigned nBestPriority = NoMatch;

	// Match the terminal types of all interfaces of the controlled sound device
	// against the output selector and return the interface with the best match.
	unsigned nInterface = 0;
	CUSBAudioStreamingDevice *pStreamingDevice;
	do
	{
		CString DeviceName;
		DeviceName.Format ("uaudio%u-%u", m_nDevice+1, nInterface+1);

		pStreamingDevice = static_cast<CUSBAudioStreamingDevice *>
			(CDeviceNameService::Get ()->GetDevice (DeviceName, FALSE));
		if (pStreamingDevice)
		{
			CUSBAudioStreamingDevice::TDeviceInfo Info =
				pStreamingDevice->GetDeviceInfo ();

			unsigned nPriority = MatchTerminalType (Info.TerminalType, Jack);
			if (nPriority < nBestPriority)
			{
				nBestInterface = nInterface;
				nBestPriority = nPriority;
			}
		}

		nInterface++;
	}
	while (pStreamingDevice);

	if (nBestPriority == NoMatch)
	{
		return FALSE;
	}

	m_pStreamingDevice = nullptr;

	m_nInterface = nBestInterface;

	return m_pSoundDevice->SetInterface (nBestInterface);
}

boolean CUSBSoundController::SetVolume (TJack Jack, int ndB, TChannel Channel)
{
	assert (m_pStreamingDevice);

	if (!IsOutputJack (Jack))
	{
		return FALSE;
	}

	if (Channel & ChannelLeft)
	{
		if (!m_pStreamingDevice->SetVolume (0, ndB))
		{
			return FALSE;
		}
	}

	if (Channel & ChannelRight)
	{
		if (!m_pStreamingDevice->SetVolume (1, ndB))
		{
			return FALSE;
		}
	}

	return TRUE;
}

const CUSBSoundController::TRange CUSBSoundController::GetVolumeRange (TJack Jack) const
{
	assert (m_pStreamingDevice);	// info is not valid otherwise

	return { m_DeviceInfo.MinVolume, m_DeviceInfo.MaxVolume };
}

unsigned CUSBSoundController::MatchTerminalType (u16 usTerminalType, TJack Jack)
{
	// These 0-terminated tables contain the USB audio class terminal types
	// for different jacks in decreasing priority order.

	static const u16 LineOutTable[] =
	{
		0x603,	// Line connector
		0x601,	// Analog connector
		0x602,	// Digital audio interface
		0x604,	// Legacy audio connector
		0x301,	// Speaker
		0x302,	// Headphones
		0
	};

	static const u16 SpeakerTable[] =
	{
		0x301,	// Speaker
		0x304,	// Desktop speaker
		0x305,	// Room speaker
		0x306,	// Communication speaker
		0x302,	// Headphones
		0x601,	// Analog connector
		0
	};

	static const u16 HeadphoneTable[] =
	{
		0x302,	// Headphones
		0x301,	// Speaker
		0
	};

	static const u16 SPDIFOutTable[] =
	{
		0x605,	// S/PDIF interface
		0x602,	// Digital audio interface
		0
	};

	const u16 *pMatchTable = nullptr;

	switch (Jack)
	{
	case JackDefaultOut:		// first interface always matches
		return 0;

	case JackLineOut:	pMatchTable = LineOutTable;	break;
	case JackSpeaker:	pMatchTable = SpeakerTable;	break;
	case JackHeadphone:	pMatchTable = HeadphoneTable;	break;
	case JackSPDIFOut:	pMatchTable = SPDIFOutTable;	break;

	default:
		return NoMatch;
	}

	assert (pMatchTable);
	for (unsigned i = 0; pMatchTable[i]; i++)
	{
		if (pMatchTable[i] == usTerminalType)
		{
			return i;
		}
	}

	return NoMatch;
}
