//
// usbkeyboardbolt.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2026  R. Stange <rsta2@gmx.net>
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
#include <circle/usb/usbkeyboardbolt.h>
#include <circle/logger.h>

#define USBKEYB_BOLT_REPORT_SIZE 16

// Bolt's bitmap starts at usage 0x04, followed by two international-key ranges.
static int USBKeyboardBoltDecodeReport (const unsigned char *input,
				       unsigned length, unsigned char output[8])
{
	if (input == 0 || length != USBKEYB_BOLT_REPORT_SIZE)
	{
		return 0;
	}

	output[0] = input[0];
	for (unsigned i = 1; i < 8; i++)
	{
		output[i] = 0;
	}

	unsigned slot = 2;
	for (unsigned bit = 0; bit < 120 && slot < 8; bit++)
	{
		if (input[1 + bit / 8] & (1U << (bit % 8)))
		{
			unsigned usage = bit < 112 ? 0x04 + bit
				       : bit < 117 ? 0x87 + bit - 112
				       : 0x90 + bit - 117;
			output[slot++] = (unsigned char) usage;
		}
	}

	return 1;
}

CUSBKeyboardBoltDevice::CUSBKeyboardBoltDevice (CUSBFunction *pFunction)
:	CUSBKeyboardDevice (pFunction)
{
}

boolean CUSBKeyboardBoltDevice::Configure (void)
{
	return ConfigureKeyboard (USBKEYB_BOLT_REPORT_SIZE);
}

u16 CUSBKeyboardBoltDevice::GetHIDProtocol (void) const
{
	// Bolt shares protocol state with its report-protocol mouse interface.
	return REPORT_PROTOCOL;
}

void CUSBKeyboardBoltDevice::ReportHandler (const u8 *pReport, unsigned nReportSize)
{
	if (pReport == 0 || nReportSize == USBKEYB_REPORT_SIZE)
	{
		CUSBKeyboardDevice::ReportHandler (pReport, nReportSize);
		return;
	}

	u8 Report[USBKEYB_REPORT_SIZE];
	if (!USBKeyboardBoltDecodeReport (pReport, nReportSize, Report))
	{
		CLogger::Get ()->Write ("ukbd-bolt", LogWarning,
				       "Unexpected keyboard report size %u", nReportSize);
		return;
	}

	CUSBKeyboardDevice::ReportHandler (Report, sizeof Report);
}
