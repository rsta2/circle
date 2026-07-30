//
// usbgamepadxbox360pcwireless.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2026  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbgamepadxbox360pcwireless.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

#define XBOX360_PC_WIRELESS_REPORT_SIZE		32
#define XBOX360_PC_WIRELESS_STATUS_SIZE		2
#define XBOX360_PC_WIRELESS_PAYLOAD_SIZE	20
#define XBOX360_PC_WIRELESS_STATE_SIZE	(4 + XBOX360_PC_WIRELESS_PAYLOAD_SIZE)

static const char FromUSBPadXbox360PCWireless[] = "usbpadxbox360pcw";

CUSBGamePadXbox360PCWirelessDevice::CUSBGamePadXbox360PCWirelessDevice (CUSBFunction *pFunction)
: CUSBGamePadDevice (pFunction),
	m_bInterfaceOK (SelectInterfaceByClass (0xFF, 0x5D, 0x81, 2)),
	m_bControllerPresent (FALSE)
{
}

CUSBGamePadXbox360PCWirelessDevice::~CUSBGamePadXbox360PCWirelessDevice (void)
{
}

boolean CUSBGamePadXbox360PCWirelessDevice::Configure (void)
{
	if (!m_bInterfaceOK)
	{
		ConfigurationError (FromUSBPadXbox360PCWireless);

		return FALSE;
	}

	m_usReportSize = XBOX360_PC_WIRELESS_REPORT_SIZE;
	if (!CUSBGamePadDevice::Configure ())
	{
		CLogger::Get ()->Write (FromUSBPadXbox360PCWireless, LogError,
					"Cannot configure wireless receiver interface");

		return FALSE;
	}

	m_State.nbuttons = GAMEPAD_BUTTONS_STANDARD;
	m_State.naxes = 6;
	for (unsigned i = 0; i < 4; i++)
	{
		m_State.axes[i].minimum = GAMEPAD_AXIS_DEFAULT_MINIMUM;
		m_State.axes[i].maximum = GAMEPAD_AXIS_DEFAULT_MAXIMUM;
	}
	for (unsigned i = 4; i < 6; i++)
	{
		m_State.axes[i].minimum = 0;
		m_State.axes[i].maximum = 255;
	}
	m_State.nhats = 0;

	if (!StartRequest ())
	{
		CLogger::Get ()->Write (FromUSBPadXbox360PCWireless, LogError,
					"Cannot start receiver input request");

		return FALSE;
	}

	if (!SendPresenceInquiry ())
	{
		CLogger::Get ()->Write (FromUSBPadXbox360PCWireless, LogError,
					"Cannot query receiver controller presence");

		return FALSE;
	}

	return TRUE;
}

void CUSBGamePadXbox360PCWirelessDevice::ReportHandler (const u8 *pReport, unsigned nReportSize)
{
	if (pReport == 0 || nReportSize < XBOX360_PC_WIRELESS_STATUS_SIZE)
	{
		return;
	}

	if ((pReport[0] & 0x08) != 0)
	{
		boolean bControllerPresent = (pReport[1] & 0x80) != 0;
		if (bControllerPresent != m_bControllerPresent)
		{
			m_bControllerPresent = bControllerPresent;
			CLogger::Get ()->Write (FromUSBPadXbox360PCWireless, LogNotice,
						"Controller %s", bControllerPresent ? "connected" : "disconnected");

			if (bControllerPresent)
			{
				// Sets the top left of the green circle LED to be on, 
				// the other three off (Xbox 360 controller player 1)
				DMA_BUFFER (u8, Command, 12);
				memset (Command, 0, sizeof Command);
				Command[2] = 0x08;
				Command[3] = 0x42;
				SendToEndpointOutAsync (Command, sizeof Command);
			}
		}
	}

	if (   nReportSize < XBOX360_PC_WIRELESS_STATE_SIZE
		|| pReport[1] != 0x01
		|| m_pStatusHandler == 0)
	{
		return;
	}

	DecodeReport (pReport + 4);
	(*m_pStatusHandler) (m_nDeviceNumber-1, &m_State);
}

void CUSBGamePadXbox360PCWirelessDevice::DecodeReport (const u8 *pReport)
{
	assert (pReport != 0);

	static const u32 ButtonMap[] =
	{
		GamePadButtonUp,
		GamePadButtonDown,
		GamePadButtonLeft,
		GamePadButtonRight,
		GamePadButtonStart,
		GamePadButtonBack,
		GamePadButtonL3,
		GamePadButtonR3,
		GamePadButtonLB,
		GamePadButtonRB,
		GamePadButtonXbox,
		0,
		GamePadButtonA,
		GamePadButtonB,
		GamePadButtonX,
		GamePadButtonY
	};

	u16 usButtons = pReport[2] | pReport[3] << 8;
	m_State.buttons = 0;
	for (unsigned i = 0; i < sizeof ButtonMap / sizeof ButtonMap[0]; i++)
	{
		if ((usButtons & (1 << i)) != 0)
		{
			m_State.buttons |= ButtonMap[i];
		}
	}

	m_State.axes[GamePadAxisButtonLT].value = pReport[4];
	m_State.axes[GamePadAxisButtonRT].value = pReport[5];
	if (pReport[4] >= 128)
	{
		m_State.buttons |= GamePadButtonLT;
	}
	if (pReport[5] >= 128)
	{
		m_State.buttons |= GamePadButtonRT;
	}

	static const unsigned AxisMap[] =
	{
		GamePadAxisLeftX,
		GamePadAxisLeftY,
		GamePadAxisRightX,
		GamePadAxisRightY
	};

	for (unsigned i = 0; i < sizeof AxisMap / sizeof AxisMap[0]; i++)
	{
		unsigned nOffset = 6 + 2*i;
		s16 nValue = (s16) (pReport[nOffset] | pReport[nOffset+1] << 8);
		unsigned nAxis = AxisMap[i];
		unsigned nMappedValue = (unsigned) (nValue - (-32768)) >> 8;
		if (   nAxis == GamePadAxisLeftY
			|| nAxis == GamePadAxisRightY)
		{
			nMappedValue = GAMEPAD_AXIS_DEFAULT_MAXIMUM - nMappedValue;
		}
		m_State.axes[nAxis].value = nMappedValue;
	}
}

boolean CUSBGamePadXbox360PCWirelessDevice::SendPresenceInquiry (void)
{
	DMA_BUFFER (u8, Command, 12);
	memset (Command, 0, sizeof Command);
	Command[0] = 0x08;
	Command[2] = 0x0F;
	Command[3] = 0xC0;

	return SendToEndpointOut (Command, sizeof Command);
}

boolean CUSBGamePadXbox360PCWirelessDevice::SetLEDMode (TGamePadLEDMode Mode)
{
	if (Mode >= GamePadLEDModeUnknown)
	{
		return FALSE;
	}

	DMA_BUFFER (u8, Command, 12);
	memset (Command, 0, sizeof Command);
	Command[2] = 0x08;
	Command[3] = 0x40 + Mode;

	return SendToEndpointOut (Command, sizeof Command);
}

boolean CUSBGamePadXbox360PCWirelessDevice::SetRumbleMode (TGamePadRumbleMode Mode)
{
	DMA_BUFFER (u8, Command, 12);
	memset (Command, 0, sizeof Command);
	Command[1] = 0x01;
	Command[2] = 0x0F;
	Command[3] = 0xC0;

	switch (Mode)
	{
	case GamePadRumbleModeOff:
		break;

	case GamePadRumbleModeLow:
		Command[6] = 0xFF;
		break;

	case GamePadRumbleModeHigh:
		Command[5] = 0xFF;
		break;

	default:
		return FALSE;
	}

	return SendToEndpointOut (Command, sizeof Command);
}
