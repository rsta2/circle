//
// usbkeyboard8bitdo.cpp
//

#include <circle/usb/usbkeyboard8bitdo.h>

#define USBKEYB8BITDO_REPORT_SIZE	17

static const char FromUSBKbd8BitDo[] = "ukbd8bitdo";

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
			// The physical up-arrow can be reported as keypad up (0x60), so map it
			// to the normal up-arrow usage (0x52).
			if (pReport[2 + nUsage / 8] & (1U << (nUsage % 8)))
			{
				Report[2 + nSlots++] = nUsage == 0x60 ? 0x52 : nUsage;
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