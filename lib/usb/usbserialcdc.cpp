//
// usbserialcdc.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbserialcdc.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/macros.h>
#include <assert.h>

#define SET_LINE_CODING			0x20

struct TLineCoding
{
	u32	dwDTERate;			// bits per second
	u8	bCharFormat;			// stop bits
#define LINE_CODING_STOP_BITS_1		0
#define LINE_CODING_STOP_BITS_1_5	1
#define LINE_CODING_STOP_BITS_2		2
	u8	bParityType;
#define LINE_CODING_PARITY_NONE		0
#define LINE_CODING_PARITY_ODD		1
#define LINE_CODING_PARITY_EVEN		2
#define LINE_CODING_PARITY_MARK		3
#define LINE_CODING_PARITY_SPACE	4
	u8	bDataBits;			// 5..8, 16
}
PACKED;

static const char From[] = "uttycdc";

CUSBSerialCDCDevice::CUSBSerialCDCDevice (CUSBFunction *pFunction)
:	CUSBSerialDevice (pFunction),
	m_ucCommunicationsInterfaceNumber (GetInterfaceNumber ()),
	m_bInterfaceOK (SelectInterfaceByClass (10, 0, 0))
{
}

CUSBSerialCDCDevice::~CUSBSerialCDCDevice (void)
{
}

boolean CUSBSerialCDCDevice::Configure (void)
{
	if (!m_bInterfaceOK)
	{
		ConfigurationError (From);

		return FALSE;
	}

	if (!CUSBSerialDevice::Configure ())
	{
		CLogger::Get ()->Write (From, LogError, "Cannot configure serial device");

		return FALSE;
	}

	return SetLineCoding ();
}

boolean CUSBSerialCDCDevice::SetBaudRate (unsigned nBaudRate)
{
	m_nBaudRate = nBaudRate;

	return SetLineCoding ();
}

boolean CUSBSerialCDCDevice::SetLineProperties (TUSBSerialDataBits DataBits,
						TUSBSerialParity Parity,
						TUSBSerialStopBits StopBits)
{
	m_nDataBits = DataBits;
	m_nParity = Parity;
	m_nStopBits = StopBits;

	return SetLineCoding ();
}

boolean CUSBSerialCDCDevice::SetLineCoding (void)
{
	DMA_BUFFER (u8, Buffer, sizeof (TLineCoding));
	TLineCoding *pLineCoding = (TLineCoding *) Buffer;

	pLineCoding->dwDTERate = m_nBaudRate;
	pLineCoding->bDataBits = m_nDataBits;
	pLineCoding->bCharFormat =   m_nStopBits == USBSerialStopBits1
				   ? LINE_CODING_STOP_BITS_1
				   : LINE_CODING_STOP_BITS_2;
	switch (m_nParity)
	{
	case USBSerialParityNone:
		pLineCoding->bParityType = LINE_CODING_PARITY_NONE;
		break;

	case USBSerialParityOdd:
		pLineCoding->bParityType = LINE_CODING_PARITY_ODD;
		break;

	case USBSerialParityEven:
		pLineCoding->bParityType = LINE_CODING_PARITY_EVEN;
		break;

	default:
		assert (0);
		break;
	}

	if (GetHost ()->ControlMessage (GetEndpoint0 (),
					   REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
					   SET_LINE_CODING, 0, m_ucCommunicationsInterfaceNumber,
					   pLineCoding, sizeof *pLineCoding) < 0)
	{
		CLogger::Get ()->Write (From, LogWarning, "Cannot set line coding");

		return FALSE;
	}

	return TRUE;
}
