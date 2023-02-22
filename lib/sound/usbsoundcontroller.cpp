//
// usbsoundcontroller.cpp
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
#include <circle/sound/usbsoundcontroller.h>
#include <circle/sound/usbsoundbasedevice.h>
#include <circle/usb/usbaudio.h>
#include <circle/devicenameservice.h>
#include <assert.h>

CUSBSoundController::CUSBSoundController (CUSBSoundBaseDevice *pSoundDevice,
					  boolean bTX, boolean bRX, unsigned nDevice)
:	m_pSoundDevice (pSoundDevice),
	m_bTX (bTX),
	m_bRX (bRX),
	m_nDevice (nDevice),
	m_nTXInterface (0),
	m_pTXStreamingDevice (nullptr),
	m_nRXTerminal (0),
	m_pRXStreamingDevice (nullptr)
{
	assert (m_pSoundDevice);
}

CUSBSoundController::~CUSBSoundController (void)
{
	m_pTXStreamingDevice = nullptr;
	m_pSoundDevice = nullptr;
}

boolean CUSBSoundController::Probe (void)
{
	assert (m_pSoundDevice);

	if (m_bTX)
	{
		assert (!m_pTXStreamingDevice);
		m_pTXStreamingDevice = GetStreamingDevice (TRUE, m_nTXInterface);
		if (!m_pTXStreamingDevice)
		{
			return FALSE;
		}

		m_TXDeviceInfo = m_pTXStreamingDevice->GetDeviceInfo ();
	}

	if (m_bRX)
	{
		assert (!m_pRXStreamingDevice);
		m_pRXStreamingDevice = GetStreamingDevice (FALSE, 0);
		if (!m_pRXStreamingDevice)
		{
			return FALSE;
		}

		m_RXDeviceInfo = m_pRXStreamingDevice->GetDeviceInfo ();
	}

	return TRUE;
}

boolean CUSBSoundController::EnableJack (TJack Jack)
{
	assert (m_pSoundDevice);

	if (IsInputJack (Jack))
	{
		if (!m_bRX)
		{
			return FALSE;
		}

		return SelectInputTerminal (Jack);
	}

	if (!m_bTX)
	{
		return FALSE;
	}

	unsigned nBestInterface = 999;
	unsigned nBestPriority = NoMatch;

	// Match the terminal types of all output interfaces of the controlled sound device
	// against the jack type and return the interface with the best match.
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

			if (Info.IsOutput)
			{
				unsigned nPriority =
					MatchTerminalType (Info.Terminal[0].TerminalType, Jack);
				if (nPriority < nBestPriority)
				{
					nBestInterface = nInterface;
					nBestPriority = nPriority;
				}
			}
		}

		nInterface++;
	}
	while (pStreamingDevice);

	if (nBestPriority == NoMatch)
	{
		return FALSE;
	}

	m_pTXStreamingDevice = nullptr;
	m_nTXInterface = nBestInterface;

	m_pRXStreamingDevice = nullptr;

#ifndef NDEBUG
	boolean bOK =
#endif
		Probe ();
	assert (bOK);

	return m_pSoundDevice->SetTXInterface (nBestInterface);
}

const CSoundController::TControlInfo CUSBSoundController::GetControlInfo (TControl Control,
									  TJack Jack,
									  TChannel Channel) const
{
	if (   m_bTX
	    && IsOutputJack (Jack))
	{
		if (!m_pTXStreamingDevice)	// info is not valid otherwise
		{
			return { FALSE };
		}

		assert (m_TXDeviceInfo.NumTerminals == 1);

		switch (Control)
		{
		case ControlMute:
			if (Channel == ChannelAll)
			{
				return { m_TXDeviceInfo.Terminal[0].MuteSupported, 0, 1 };
			}
			break;

		case ControlVolume:
			if (   Channel != ChannelAll
			    && !m_TXDeviceInfo.Terminal[0].VolumePerChannel)
			{
				return { FALSE };
			}

			return { m_TXDeviceInfo.Terminal[0].VolumeSupported,
				 m_TXDeviceInfo.Terminal[0].MinVolume,
				 m_TXDeviceInfo.Terminal[0].MaxVolume };

		default:
			break;
		}
	}
	else if (   m_bRX
		 && IsInputJack (Jack))
	{
		if (!m_pRXStreamingDevice)	// info is not valid otherwise
		{
			return { FALSE };
		}

		assert (m_nRXTerminal < m_RXDeviceInfo.NumTerminals);

		switch (Control)
		{
		case ControlMute:
			if (Channel == ChannelAll)
			{
				return { m_RXDeviceInfo.Terminal[m_nRXTerminal].MuteSupported,
					 0, 1 };
			}
			break;

		case ControlVolume:
			if (   Channel != ChannelAll
			    && !m_RXDeviceInfo.Terminal[0].VolumePerChannel)
			{
				return { FALSE };
			}

			return { m_RXDeviceInfo.Terminal[m_nRXTerminal].VolumeSupported,
				 m_RXDeviceInfo.Terminal[m_nRXTerminal].MinVolume,
				 m_RXDeviceInfo.Terminal[m_nRXTerminal].MaxVolume };

		default:
			break;
		}
	}

	return { FALSE };
}

boolean CUSBSoundController::SetControl (TControl Control, TJack Jack, TChannel Channel, int nValue)
{
	switch (Control)
	{
	case ControlMute:
		if (Channel == ChannelAll)
		{
			return SetMute (Jack, !!nValue);
		}
		break;

	case ControlVolume:
		return SetVolume (Jack, Channel, nValue);

	default:
		break;
	}

	return FALSE;
}

boolean CUSBSoundController::SelectInputTerminal (TJack Jack)
{
	assert (IsInputJack (Jack));
	assert (m_bRX);

	if (!m_pRXStreamingDevice)
	{
		return FALSE;
	}

	unsigned nBestTerminal = 999;
	unsigned nBestPriority = NoMatch;

	// Match the terminal types of all input terminals of the controlled sound device
	// against the jack type and return the terminal with the best match.
	for (unsigned nTerminal = 0; nTerminal < m_RXDeviceInfo.NumTerminals; nTerminal++)
	{
		unsigned nPriority = MatchTerminalType (
			m_RXDeviceInfo.Terminal[nTerminal].TerminalType, Jack);
		if (nPriority < nBestPriority)
		{
			nBestTerminal = nTerminal;
			nBestPriority = nPriority;
		}
	}

	if (nBestPriority == NoMatch)
	{
		return FALSE;
	}

	return m_pRXStreamingDevice->SelectInputTerminal (nBestTerminal);
}

boolean CUSBSoundController::SetMute (TJack Jack, boolean bEnable)
{
	if (   IsOutputJack (Jack)
	    && m_pTXStreamingDevice)
	{
		return m_pTXStreamingDevice->SetMute (bEnable);
	}
	else if (   IsInputJack (Jack)
		 && m_pRXStreamingDevice)
	{
		return m_pRXStreamingDevice->SetMute (bEnable);
	}


	return FALSE;
}

boolean CUSBSoundController::SetVolume (TJack Jack, TChannel Channel, int ndB)
{
	if (   IsOutputJack (Jack)
	    && m_pTXStreamingDevice)
	{
		if (!m_TXDeviceInfo.Terminal[0].VolumePerChannel)
		{
			if (Channel == ChannelAll)
			{
				return m_pTXStreamingDevice->SetVolume (0, ndB);
			}
		}
		else
		{
			return m_pTXStreamingDevice->SetVolume (Channel, ndB);
		}
	}
	else if (   IsInputJack (Jack)
		 && m_pRXStreamingDevice)
	{
		if (!m_RXDeviceInfo.Terminal[m_nRXTerminal].VolumePerChannel)
		{
			if (Channel == ChannelAll)
			{
				return m_pRXStreamingDevice->SetVolume (0, ndB);
			}
		}
		else
		{
			return m_pRXStreamingDevice->SetVolume (Channel, ndB);
		}
	}

	return FALSE;
}

CUSBAudioStreamingDevice *CUSBSoundController::GetStreamingDevice (boolean bTX, unsigned nIndex)
{
	for (unsigned nInterface = 0; TRUE; nInterface++)
	{
		CString USBDeviceName;
		USBDeviceName.Format ("uaudio%u-%u", m_nDevice+1, nInterface+1);

		CDevice *pDevice = CDeviceNameService::Get ()->GetDevice (USBDeviceName, FALSE);
		if (!pDevice)
		{
			break;
		}

		CUSBAudioStreamingDevice *pUSBDevice =
			static_cast<CUSBAudioStreamingDevice *> (pDevice);

		CUSBAudioStreamingDevice::TDeviceInfo Info = pUSBDevice->GetDeviceInfo ();
		if (Info.IsOutput == bTX)
		{
			if (!nIndex--)
			{
				return pUSBDevice;
			}
		}
	}

	return nullptr;
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

	static const u16 LineInTable[] =
	{
		0x603,	// Line connector
		0x601,	// Analog connector
		0x602,	// Digital audio interface
		0x604,	// Legacy audio connector
		0
	};

	static const u16 MicrophoneTable[] =
	{
		0x201,	// Microphone
		0x202,	// Desktop microphone
		0x203,	// Personal microphone
		0x204,	// Omni-directional microphone
		0x205,	// Microphone array
		0x206,	// Processing microphone array
		0
	};

	const u16 *pMatchTable = nullptr;

	switch (Jack)
	{
	case JackDefaultOut:		// first interface always matches
	case JackDefaultIn:
		return 0;

	case JackLineOut:	pMatchTable = LineOutTable;	break;
	case JackSpeaker:	pMatchTable = SpeakerTable;	break;
	case JackHeadphone:	pMatchTable = HeadphoneTable;	break;
	case JackSPDIFOut:	pMatchTable = SPDIFOutTable;	break;

	case JackLineIn:	pMatchTable = LineInTable;	break;
	case JackMicrophone:	pMatchTable = MicrophoneTable;	break;

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
