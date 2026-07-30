//
// usbkeyboard8bitdo.cpp
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
#include <circle/usb/usbkeyboard8bitdo.h>

#define USBKEYB8BITDO_REPORT_SIZE	17

CUSBKeyboard8BitDoDevice::CUSBKeyboard8BitDoDevice (CUSBFunction *pFunction)
: 	CUSBKeyboardDevice (pFunction)
{
}

CUSBKeyboard8BitDoDevice::~CUSBKeyboard8BitDoDevice (void)
{
}

boolean CUSBKeyboard8BitDoDevice::Configure (void)
{
	return ConfigureKeyboard (USBKEYB8BITDO_REPORT_SIZE);
}

void CUSBKeyboard8BitDoDevice::ReportHandler (const u8 *pReport, unsigned nReportSize)
{
	// Forward HID transfer failures to the common keyboard driver.
	if (pReport == 0)
	{
		CUSBKeyboardDevice::ReportHandler (0, 0);

		return;
	}

	// Convert the 8BitDo NKRO bitmap report to a boot keyboard report.
	if (   nReportSize == USBKEYB8BITDO_REPORT_SIZE
	    && (pReport[0] == 0x0A || pReport[0] == 0x0C))
	{
		u8 Report[USBKEYB_REPORT_SIZE] = {pReport[1]};
		unsigned nSlots = 0;

		for (unsigned nUsage = 0x04; nUsage <= 0x77 && nSlots < 6; nUsage++)
		{
			if (pReport[2 + nUsage / 8] & (1U << (nUsage % 8)))
			{
				Report[2 + nSlots++] = nUsage;
			}
		}

		CUSBKeyboardDevice::ReportHandler (Report, sizeof Report);

		return;
	}

	// Strip the report ID from a standard boot keyboard report.
	if (nReportSize == 9 && pReport[0] == 0x01)
	{
		CUSBKeyboardDevice::ReportHandler (pReport + 1, USBKEYB_REPORT_SIZE);

		return;
	}

	// Forward an unprefixed standard boot keyboard report.
	if (nReportSize == USBKEYB_REPORT_SIZE)
	{
		CUSBKeyboardDevice::ReportHandler (pReport, nReportSize);

		return;
	}
}
