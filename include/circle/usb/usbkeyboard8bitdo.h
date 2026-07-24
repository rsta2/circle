//
// usbkeyboard8bitdo.h
//

#ifndef _circle_usb_usbkeyboard8bitdo_h
#define _circle_usb_usbkeyboard8bitdo_h

#include <circle/usb/usbkeyboard.h>

class CUSBKeyboard8BitDoDevice : public CUSBKeyboardDevice
{
public:
	CUSBKeyboard8BitDoDevice (CUSBFunction *pFunction);
	~CUSBKeyboard8BitDoDevice (void);

	boolean Configure (void);

protected:
	void ReportHandler (const u8 *pReport, unsigned nReportSize);
};

#endif