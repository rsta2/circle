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
#include <circle/usb/usbaudiostreaming.h>
#include <circle/usb/usbaudio.h>
#include <circle/devicenameservice.h>
#include <assert.h>

CUSBSoundController::CUSBSoundController (CUSBSoundBaseDevice *pSoundDevice)
:	m_pSoundDevice (pSoundDevice)
{
	assert (m_pSoundDevice);
}

CUSBSoundController::~CUSBSoundController (void)
{
	m_pSoundDevice = nullptr;
}

void CUSBSoundController::SelectOutput (TOutputSelector Selector)
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
		DeviceName.Format ("uaudio%u-%u", m_pSoundDevice->GetDeviceIndex ()+1, nInterface+1);

		pStreamingDevice = static_cast<CUSBAudioStreamingDevice *>
			(CDeviceNameService::Get ()->GetDevice (DeviceName, FALSE));
		if (pStreamingDevice)
		{
			CUSBAudioStreamingDevice::TFormatInfo Info =
				pStreamingDevice->GetFormatInfo ();

			unsigned nPriority = MatchTerminalType (Info.TerminalType, Selector);
			if (nPriority < nBestPriority)
			{
				nBestInterface = nInterface;
				nBestPriority = nPriority;
			}
		}

		nInterface++;
	}
	while (pStreamingDevice);

	if (nBestPriority == NoMatch)		// no match, do nothing
	{
		return;
	}

	// Set the interface and restart the device, if it was active before.
	if (m_pSoundDevice->IsActive ())
	{
		m_pSoundDevice->Disconnect ();
		m_pSoundDevice->SetInterface (nBestInterface);
		m_pSoundDevice->Start ();
	}
	else
	{
		m_pSoundDevice->Disconnect ();
		m_pSoundDevice->SetInterface (nBestInterface);
	}
}

unsigned CUSBSoundController::MatchTerminalType (u16 usTerminalType, TOutputSelector Selector)
{
	// These 0-terminated tables contain the USB audio class terminal types
	// for different Selectors in decreasing priority order.

	static const u16 LineTable[] =
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

	static const u16 HeadphonesTable[] =
	{
		0x302,	// Headphones
		0x301,	// Speaker
		0
	};

	static const u16 SPDIFTable[] =
	{
		0x605,	// S/PDIF interface
		0x602,	// Digital audio interface
		0
	};

	const u16 *pMatchTable = nullptr;

	switch (Selector)
	{
	case OutputSelectorDefault:		// first interface always matches
		return 0;

	case OutputSelectorLine:	pMatchTable = LineTable;	break;
	case OutputSelectorSpeaker:	pMatchTable = SpeakerTable;	break;
	case OutputSelectorHeadphones:	pMatchTable = HeadphonesTable;	break;
	case OutputSelectorSPDIF:	pMatchTable = SPDIFTable;	break;

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
