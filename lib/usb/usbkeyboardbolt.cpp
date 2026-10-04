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
