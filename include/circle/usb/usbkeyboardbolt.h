#ifndef _circle_usb_usbkeyboardbolt_h
#define _circle_usb_usbkeyboardbolt_h

#include <circle/usb/usbkeyboard.h>

class CUSBKeyboardBoltDevice : public CUSBKeyboardDevice
{
public:
	CUSBKeyboardBoltDevice (CUSBFunction *pFunction);
	boolean Configure (void);

protected:
	u16 GetHIDProtocol (void) const override;
	void ReportHandler (const u8 *pReport, unsigned nReportSize);
};

#endif
