//
// ili9341display.cpp
//
// Based on a driver by u77345@GitHub:
//	https://github.com/u77345/circle/blob/master/addon/display/ili9341.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#include <display/ili9341display.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <circle/stdarg.h>
#include <assert.h>

// Display commands list
#define ILI9341_NOP			0x00
#define ILI9341_SOFTWARE_RESET		0x01
#define ILI9341_READ_IDENTIFICATION	0x04
#define ILI9341_READ_STATUS		0x09
#define ILI9341_READ_POWER_MODE		0x0A
#define ILI9341_READ_MADCTL		0x0B
#define ILI9341_READ_PIXEL_FORMAT	0x0C
#define ILI9341_READ_IMAGE_FORMAT	0x0D
#define ILI9341_READ_SIGNAL_MODE	0x0E
#define ILI9341_READ_SELF_DIAGNOSTIC	0x0F
#define ILI9341_SLEEP_IN		0x10
#define ILI9341_SLEEP_OUT		0x11
#define ILI9341_PARTIAL_MODE_ON		0x12
#define ILI9341_NORMAL_DISPLAY_MODE_ON	0x13
#define ILI9341_INVERSION_OFF		0x20
#define ILI9341_INVERSION_ON		0x21
#define ILI9341_GAMMA_SET		0x26
#define ILI9341_DISPLAY_OFF		0x28
#define ILI9341_DISPLAY_ON		0x29
#define ILI9341_COLUMN_ADDRESS_SET	0x2A
#define ILI9341_PAGE_ADDRESS_SET	0x2B
#define ILI9341_MEMORY_WRITE		0x2C
#define ILI9341_COLOR_SET		0x2D
#define ILI9341_MEMORY_READ		0x2E
#define ILI9341_PARTIAL_AREA		0x30
#define ILI9341_VERTICAL_SCROLLING_DEF	0x33
#define ILI9341_TEARING_LINE_OFF	0x34
#define ILI9341_TEARING_LINE_ON		0x35
#define ILI9341_MEMORY_ACCESS_CONTROL	0x36
	#define ILI9341_MADCTL_RGB		0x00
	#define ILI9341_MADCTL_MH		0x04
	#define ILI9341_MADCTL_BGR		0x08
	#define ILI9341_MADCTL_ML		0x10
	#define ILI9341_MADCTL_MV		0x20
	#define ILI9341_MADCTL_MX		0x40
	#define ILI9341_MADCTL_MY		0x80
#define ILI9341_VERTICAL_SCROLLING	0x37
#define ILI9341_IDLE_MODE_OFF		0x38
#define ILI9341_IDLE_MODE_ON		0x39
#define ILI9341_PIXEL_FORMAT_SET	0x3A
#define ILI9341_WRITE_MEMORY_CONTINUE	0x3C
#define ILI9341_READ_MEMORY_CONTINUE	0x3E
#define ILI9341_SET_TEAR_SCANLINE	0x44
#define ILI9341_GET_SCANLINE		0x45
#define ILI9341_WRITE_BRIGHTNESS	0x51
#define ILI9341_READ_BRIGHTNESS		0x52
#define ILI9341_WRITE_CTRL_DISPLAY	0x53
#define ILI9341_READ_CTRL_DISPLAY	0x54
#define ILI9341_WRITE_CA_BRIGHTNESS	0x55
#define ILI9341_READ_CA_BRIGHTNESS	0x56
#define ILI9341_WRITE_CA_MIN_BRIGHTNESS	0x5E
#define ILI9341_READ_CA_MIN_BRIGHTNESS	0x5F
#define ILI9341_READ_ID1		0xDA
#define ILI9341_READ_ID2		0xDB
#define ILI9341_READ_ID3		0xDC
#define ILI9341_RGB_INTERFACE_CONTROL	0xB0
#define ILI9341_FRAME_RATE_CONTROL_1	0xB1
#define ILI9341_FRAME_RATE_CONTROL_2	0xB2
#define ILI9341_FRAME_RATE_CONTROL_3	0xB3
#define ILI9341_DISPLAY_INVERSION_CONTROL 0xB4
#define ILI9341_BLANKING_PORCH_CONTROL	0xB5
#define ILI9341_DISPLAY_FUNCTION_CONTROL 0xB6
#define ILI9341_ENTRY_MODE_SET		0xB7
#define ILI9341_BACKLIGHT_CONTROL_1	0xB8
#define ILI9341_BACKLIGHT_CONTROL_2	0xB9
#define ILI9341_BACKLIGHT_CONTROL_3	0xBA
#define ILI9341_BACKLIGHT_CONTROL_4	0xBB
#define ILI9341_BACKLIGHT_CONTROL_5	0xBC
#define ILI9341_BACKLIGHT_CONTROL_7	0xBE
#define ILI9341_BACKLIGHT_CONTROL_8	0xBF
#define ILI9341_POWER_CONTROL_1		0xC0
#define ILI9341_POWER_CONTROL_2		0xC1
#define ILI9341_VCOM_CONTROL_1		0xC5
#define ILI9341_VCOM_CONTROL_2		0xC7
#define ILI9341_POWERA			0xCB
#define ILI9341_POWERB			0xCF
#define ILI9341_NV_MEMORY_WRITE		0xD0
#define ILI9341_NV_PROTECTION_KEY	0xD1
#define ILI9341_NV_STATUS_READ		0xD2
#define ILI9341_READ_ID4		0xD3
#define ILI9341_POSITIVE_GAMMA_CORRECTION 0xE0
#define ILI9341_NEGATIVE_GAMMA_CORRECTION 0xE1
#define ILI9341_DIGITAL_GAMMA_CONTROL_1	0xE2
#define ILI9341_DIGITAL_GAMMA_CONTROL_2	0xE3
#define ILI9341_DTCA			0xE8
#define ILI9341_DTCB			0xEA
#define ILI9341_POWER_SEQ		0xED
#define ILI9341_3GAMMA_EN		0xF2
#define ILI9341_INTERFACE_CONTROL	0xF6
#define ILI9341_PUMP_RATIO_CONTROL	0xF7

CILI9341Display::CILI9341Display (CSPIMaster *pSPIMaster,
				  unsigned nDCPin, unsigned nResetPin, unsigned nBackLightPin,
				  unsigned nWidth, unsigned nHeight,
				  unsigned nCPOL, unsigned nCPHA, unsigned nClockSpeed,
				  unsigned nChipSelect, boolean bSwapColorBytes)
:	CDisplay (bSwapColorBytes ? RGB565_BE : RGB565),
	m_pSPIMaster (pSPIMaster),
	m_nResetPin (nResetPin),
	m_nBackLightPin (nBackLightPin),
	m_nWidth (nWidth),
	m_nHeight (nHeight),
	m_nCPOL (nCPOL),
	m_nCPHA (nCPHA),
	m_nClockSpeed (nClockSpeed),
	m_nChipSelect (nChipSelect),
	m_bSwapColorBytes (bSwapColorBytes),
	m_pBuffer (nullptr),
	m_nRotation (0),
	m_DCPin (nDCPin, GPIOModeOutput)
{
	assert (nDCPin != None);

	if (m_nBackLightPin != None)
	{
		m_BackLightPin.AssignPin (m_nBackLightPin);
		m_BackLightPin.SetMode (GPIOModeOutput, FALSE);
	}

	if (m_nResetPin != None)
	{
		m_ResetPin.AssignPin (m_nResetPin);
		m_ResetPin.SetMode (GPIOModeOutput, FALSE);
	}
}

CILI9341Display::~CILI9341Display (void)
{
	delete [] m_pBuffer;
};

void CILI9341Display::SetRotation (unsigned nDegrees)
{
	assert (nDegrees < 360 && nDegrees % 90 == 0);

	m_nRotation = nDegrees;
}

boolean CILI9341Display::Initialize (void)
{
	assert (m_pSPIMaster != 0);

	if (!m_bSwapColorBytes)
	{
		assert (!m_pBuffer);
		m_pBuffer = new u16[m_nWidth * m_nHeight];
		assert (m_pBuffer);
	}

	if (m_nBackLightPin != None)
	{
		m_BackLightPin.Write (HIGH);
	}

	if (m_nResetPin != None)
	{
		m_ResetPin.Write (HIGH);
		CTimer::SimpleMsDelay (10);
		m_ResetPin.Write (LOW);
		CTimer::SimpleMsDelay (10);
		m_ResetPin.Write (HIGH);
	}
	else
	{
		Command (ILI9341_SOFTWARE_RESET);
	}

	CTimer::SimpleMsDelay (150);

	// Init sequence

	Off ();

	CommandAndData (ILI9341_POWERB, 3, 0x00, 0x83, 0x30);

	CommandAndData (ILI9341_POWER_SEQ, 4, 0x64, 0x03, 0x12, 0x81);
	//CommandAndData (ILI9341_POWER_SEQ, 4, 0x55, 0x01, 0x23, 0x01);

	// Driver timing control A
	CommandAndData (ILI9341_DTCA, 3, 0x85, 0x01, 0x79);
	//CommandAndData (ILI9341_DTCA, 3, 0x84, 0x11, 0x7A);

	CommandAndData (ILI9341_POWERA, 5, 0x39, 0x2C, 0x00, 0x34, 0x02);

	CommandAndData (ILI9341_PUMP_RATIO_CONTROL, 1, 0x20);

	// Driver timing control B
	CommandAndData (ILI9341_DTCB, 2, 0x00, 0x00);

	CommandAndData (ILI9341_POWER_CONTROL_1, 1, 0x26);
	CommandAndData (ILI9341_POWER_CONTROL_2, 1, 0x11);

	CommandAndData (ILI9341_VCOM_CONTROL_1, 2, 0x35, 0x3E);
	CommandAndData (ILI9341_VCOM_CONTROL_2, 1, 0xBE);

	u8 uchMADCtl = ILI9341_MADCTL_BGR;
	switch (m_nRotation)
	{
	case 0:
		uchMADCtl |= ILI9341_MADCTL_MX;
		break;

	case 90:
		uchMADCtl |= ILI9341_MADCTL_MV;
		break;

	case 180:
		uchMADCtl |= ILI9341_MADCTL_MY;
		break;

	case 270:
		uchMADCtl |= ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV;
		break;

	default:
		assert (0);
		break;
	}

	CommandAndData (ILI9341_MEMORY_ACCESS_CONTROL, 1, uchMADCtl);

	if (m_nRotation % 180)
	{
		unsigned nTemp = m_nWidth;
		m_nWidth = m_nHeight;
		m_nHeight = nTemp;
	}

	CommandAndData (ILI9341_PIXEL_FORMAT_SET, 1, 0x55);	// 16 bpp

	CommandAndData (ILI9341_FRAME_RATE_CONTROL_1, 2, 0x00, 0x1B);

	CommandAndData (ILI9341_3GAMMA_EN, 1, 0x08);	// Gamma Function Disable
	CommandAndData (ILI9341_GAMMA_SET, 1, 0x01);	// Gamma set for curve 01/02/04/08

	CommandAndData (ILI9341_ENTRY_MODE_SET, 1, 0x06);

	CommandAndData (ILI9341_DISPLAY_FUNCTION_CONTROL, 4, 0x0A, 0x82, 0x27, 0x00);

	CommandAndData (ILI9341_INTERFACE_CONTROL, 3, 0x00, 0x00, 0x00);	// Set WEMODE=0

	//CommandAndData (ILI9341_WRITE_CTRL_DISPLAY, 1, 0x0C);
	//CommandAndData (ILI9341_WRITE_BRIGHTNESS, 1, 0xFF);

	Command (ILI9341_SLEEP_OUT);
	CTimer::SimpleMsDelay (5);

	On ();
	Clear ();

	return TRUE;
}

void CILI9341Display::On (void)
{
	Command (ILI9341_DISPLAY_ON);
}

void CILI9341Display::Off (void)
{
	Command (ILI9341_DISPLAY_OFF);
}

void CILI9341Display::Clear (TRawColor nColor)
{
	assert (m_nWidth > 0);
	assert (m_nHeight > 0);

	SetWindow (0, 0, m_nWidth-1, m_nHeight-1);

	u16 Buffer[m_nWidth];
	for (unsigned x = 0; x < m_nWidth; x++)
	{
		Buffer[x] = (u16) nColor;
	}

	for (unsigned y = 0; y < m_nHeight; y++)
	{
		SendData (Buffer, sizeof Buffer);
	}
}

void CILI9341Display::SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor)
{
	SetWindow (nPosX, nPosY, nPosX, nPosY);

	if (!m_bSwapColorBytes)
	{
		nColor = bswap16 (nColor & 0xFFFF);
	}

	SendData (&nColor, sizeof (u16));
}

void CILI9341Display::SetArea (const TArea &rArea, const void *pPixels,
			       TAreaCompletionRoutine *pRoutine,
			       void *pParam)
{
	SetWindow (rArea.x1, rArea.y1, rArea.x2, rArea.y2);

	size_t ulSize = (rArea.y2 - rArea.y1 + 1) * (rArea.x2 - rArea.x1 + 1) * sizeof (u16);

	if (!m_bSwapColorBytes)
	{
		// We swap bytes here, because there are swapped by default by the hardware.
		unsigned nPixels = ulSize / sizeof (u16);
		assert (nPixels < m_nWidth * m_nHeight);

		const u16 *p = (const u16 *) pPixels;
		u16 *q = m_pBuffer;
		while (nPixels--)
		{
			*q++ = bswap16 (*p++);
		}

		pPixels = m_pBuffer;
	}

	while (ulSize)
	{
		// The BCM2835 SPI master has a transfer size limit.
		// TODO: Request this parameter from the SPI master driver.
		const size_t MaxTransferSize = 0xFFFC;

		size_t ulBlockSize = ulSize >= MaxTransferSize ? MaxTransferSize : ulSize;

		SendData (pPixels, ulBlockSize);

		pPixels = (const void *) ((uintptr) pPixels + ulBlockSize);

		ulSize -= ulBlockSize;
	}

	if (pRoutine)
	{
		(*pRoutine) (pParam);
	}
}

void CILI9341Display::SetWindow (unsigned x0, unsigned y0, unsigned x1, unsigned y1)
{
	assert (x0 <= x1);
	assert (y0 <= y1);
	assert (x1 < m_nWidth);
	assert (y1 < m_nHeight);

	Command (ILI9341_COLUMN_ADDRESS_SET);

	u8 ColumnData[4] = {(u8) (x0 >> 8), (u8) x0, (u8) (x1 >> 8), (u8) x1};
	SendData (ColumnData, sizeof ColumnData);

	Command (ILI9341_PAGE_ADDRESS_SET);

	u8 PageData[4] = {(u8) (y0 >> 8), (u8) y0, (u8) (y1 >> 8), (u8) y1};
	SendData (PageData, sizeof PageData);

	Command (ILI9341_MEMORY_WRITE);
}

void CILI9341Display::SendByte (u8 uchByte, boolean bIsData)
{
	assert (m_pSPIMaster != 0);

	m_DCPin.Write (bIsData ? HIGH : LOW);

	m_pSPIMaster->SetClock (m_nClockSpeed);
	m_pSPIMaster->SetMode (m_nCPOL, m_nCPHA);

#ifndef NDEBUG
	int nResult =
#endif
		m_pSPIMaster->Write (m_nChipSelect, &uchByte, sizeof uchByte);
	assert (nResult == (int) sizeof uchByte);
}

void CILI9341Display::SendData (const void *pData, size_t nLength)
{
	assert (pData != 0);
	assert (nLength > 0);
	assert (m_pSPIMaster != 0);

	m_DCPin.Write (HIGH);

	m_pSPIMaster->SetClock (m_nClockSpeed);
	m_pSPIMaster->SetMode (m_nCPOL, m_nCPHA);

#ifndef NDEBUG
	int nResult =
#endif
		m_pSPIMaster->Write (m_nChipSelect, pData, nLength);
	assert (nResult == (int) nLength);
}

void CILI9341Display::CommandAndData (u8 uchCmd, unsigned nDataLen, ...)
{
	va_list var;
	va_start (var, nDataLen);

	u8 Buffer[nDataLen];
	for (unsigned i = 0; i < nDataLen; i++)
	{
		Buffer[i] = (u8) va_arg (var, int);
	}

	va_end (var);

	Command (uchCmd);

	SendData (Buffer, nDataLen);
}
