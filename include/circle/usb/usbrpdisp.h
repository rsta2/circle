//
// usbrpdisp.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2026  R. Stange <rsta2@gmx.net>
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
// Copyright (C) 2026  S. Spencer (SteveSpencer on Github)
//
// Driver for RoboPeak RP-USBDISP display with touchscreen.
// Unlike many other displays which have touch functionality, this board
// only needs a USB connection to provide power and interface. It has a
// screen (320x240) supporting 16-bit RGB656 color, and a touchscreen which
// does not require any external calibration, as calibration is built into
// the firmware. Screens were normally calibrated before shipping. To
// recalibrate at any time do the following:
// 
// 1. Disconnect the RoboPeak display from Pi.
// 2. Connect it to a 5V power supply using a micro B USB cable.
// 3. When the RoboPeak logo is displayed, touch and hold on the screen.
// 4. The screen should go black, then display the text
//     Touch Screen Calibration
//     Please press the red flashing dot on the screen
// 5. Press each red dot for about 1 second, when it appears.
//    (There should be one in each corner).
// 6. After the calibration completes, the data will be saved to
//    flash memory on the display, and available from next time.
//
// The device supports CDisplay, so can be used anywhere a CDisplay
// object is needed, such as with LVGL. However, unlike other
// CDisplay-derived objects, it is dynamically created, not as a member
// of the CKernel object. Typically, you would use a code block similar
// to the following to initialize the LVGL environment:
//
// CUSBRoboPeakDispDevice *pRP = 
//         (CUSBRoboPeakDispDevice *) m_DeviceNameService.GetDevice ("rpusbdisp",FALSE);
// if (pRP == 0)
// {
//         m_Logger.Write (FromKernel, LogPanic, "RoboPeak device not found");
// }
// CLVGL *pLVGL = new CLVGL (pRP);
// if ( pLVGL && pLVGL->Initialize())
// {
//    suceeded so carry on ...
// }
//
// Note: If you are not using CLVGL, then the Setup method of the touch device
//       needs to be called (the LVGL wrapper does this for you).
//
#ifndef _usb_rpdisp_h
#define _usb_rpdisp_h

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
#include <circle/usb/usbrequest.h>
#include <circle/timer.h>
#include <circle/types.h>
#include <circle/input/touchscreen.h>

#define	RPD_WIDTH	320
#define	RPD_HEIGHT	240

class CUSBRoboPeakDispDevice : public CUSBFunction, public CDisplay
{
public:
	CUSBRoboPeakDispDevice (CUSBFunction *pFunction);
	~CUSBRoboPeakDispDevice (void);

	boolean Configure (void) override;

	unsigned GetWidth (void) const override { return 320; }
	unsigned GetHeight (void) const override { return 240; }
	unsigned GetDepth (void) const override { return 16; }

	/// \param nPosX 
	/// \param nPosY 
	/// \param nColor
	void SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor) override;

	/// \param rArea
	/// \param pPixels
	/// \param pRoutine
	/// \param pParam
	void SetArea (const TArea &rArea, const void *pPixels,
			      TAreaCompletionRoutine *pRoutine = nullptr,
			      void *pParam = nullptr) override;

private:
	// Format of the report packet for a touchscreen event
	struct TStatus
	{
		u8	nPacketType;
		u8	nDisplayStatus;
		u8	nTouchStatus;
		s32	sTouchX;
		s32	sTouchY;
	} PACKED;
	// Generic structure of a display command packet
	struct TCmdPacket
	{
		u8	bCmdFlags;
		u16	uX;
		u16	uY;
		u16	uW;
		u16	uH;
		u8	bROP;
		u8  data[54];	// Space left
	} PACKED;
	// Generic structure of a display command packet
	struct TCmdSubPacket
	{
		u8  bCmdFlags;
		u8  data[63];
	};	
	// Format of a Fill packet
	struct TCmdFill
	{
		u8	bCmdFlags;
		u16	uColour;
	} PACKED;
	
	// Format of a BitBlt or BitBltRLE packet
	struct TCmdBitBlt
	{
		u8	bCmdFlags;
		u16	uX;
		u16	uY;
		u16	uW;
		u16	uH;
		u8	bROP;
	} PACKED;

	// Format of a FillRectangle packet
	struct TCmdFillRect
	{
		u8	bCmdFlags;
		u16	uX;
		u16	uY;
		u16	uW;
		u16	uH;
		u16	uColour;
		u8	bROP;
	} PACKED;

	enum Tcmd {
		cmdFill = 0x1,
		cmdBitBlt = 0x2,
		cmdRect = 0x3,
		cmdFillRect = 0x04, // FW 1.04 and above
		cmdBitBltRLE = 0x5, // FW 1.04 and above
	};

	enum TROP {
		ROP_COPY = 0x0,
		ROP_XOR = 0x1,
		ROP_OR = 0x2,
		ROP_AND = 0x3
	};

	static void CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext);
	boolean MakeRequest (void);
	void CompletionRoutine (CUSBRequest *pURB);
	/// \param pHeader Pointer to the display command
	/// \param sHeaderSize Size of the display command packet (varies by command)
	/// \param pData Pointer to additional data to send (can be null)
	/// \param sDataSize Number of bytes of additional data (can be zero)
	void sendData( u8* pHeader, size_t sHeaderSize, const u8* pData, size_t sDataSize);
	/// \param pixels Pixel data (RGB565) to compress
	/// \param nPix Number of pixels to process
	/// \return Number of compressed bytes in payload buffer
        size_t RLEcompress (const u16* pixels, size_t nPix);
private:
	CUSBEndpoint *m_pEndpointIn;
	CUSBEndpoint *m_pEndpointOut;
	CTouchScreenDevice *m_pTouch;

	u8	*m_Report;
	
	u8  *m_payload;
	size_t m_nPayloadMax;

	boolean	m_bDown;
	u16	m_fw;
};

#endif
