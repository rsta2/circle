//
// usbserialch341.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2020  H. Kocevar <hinxx@protonmail.com>
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
#include <circle/usb/usbserialch341.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromCh341[] = "ch341";

#define DEFAULT_BAUD_RATE      9600
#define DEFAULT_TIMEOUT        1000
#define CH341_BAUDBASE_FACTOR  1532620800
#define CH341_BAUDBASE_DIVMAX  3

#define CH341_REQ_READ_VERSION 0x5F
#define CH341_REQ_WRITE_REG    0x9A
#define CH341_REQ_SERIAL_INIT  0xA1
#define CH341_REQ_MODEM_CTRL   0xA4

#define CH341_LCR_ENABLE_RX    0x80
#define CH341_LCR_ENABLE_TX    0x40
#define CH341_LCR_PAR_EVEN     0x10
#define CH341_LCR_ENABLE_PAR   0x08
#define CH341_LCR_STOP_BITS_2  0x04
#define CH341_LCR_CS8          0x03
#define CH341_LCR_CS7          0x02
#define CH341_LCR_CS6          0x01
#define CH341_LCR_CS5          0x00

CUSBSerialCH341Device::CUSBSerialCH341Device (CUSBFunction *pFunction)
:	CUSBSerialDevice (pFunction)
{
}

CUSBSerialCH341Device::~CUSBSerialCH341Device (void)
{
}

boolean CUSBSerialCH341Device::Configure (void)
{
	if (!CUSBSerialDevice::Configure ())
	{
		CLogger::Get ()->Write (FromCh341, LogError, "Cannot configure serial device");

		return FALSE;
	}

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	DMA_BUFFER (u8, rxData, 2) = {0};
	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_IN | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   CH341_REQ_READ_VERSION,
				   0,
				   0,
				   rxData, 2) != (int) 2)
	{
		CLogger::Get ()->Write (FromCh341, LogError, "Cannot get version bytes");

		return FALSE;
	}
	CLogger::Get ()->Write (FromCh341, LogNotice, "Chip version: 0x%2X", rxData[0]);

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   CH341_REQ_SERIAL_INIT,
				   0,
				   0,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromCh341, LogError, "Cannot init serial port");

		return FALSE;
	}

	if (!SetBaudRate (DEFAULT_BAUD_RATE))
	{
		return FALSE;
	}

	if (!SetLineProperties (USBSerialDataBits8, USBSerialParityNone, USBSerialStopBits1))
	{
		return FALSE;
	}

	return TRUE;
}

boolean CUSBSerialCH341Device::SetBaudRate (unsigned nBaudRate)
{
	u32 factor = (CH341_BAUDBASE_FACTOR / nBaudRate);
	u16 divisor = CH341_BAUDBASE_DIVMAX;

	while ((factor > 0xfff0) && divisor)
	{
		factor >>= 3;
		divisor--;
	}

	if (factor > 0xfff0)
	{
		CLogger::Get ()->Write (FromCh341, LogError, "factor > 0xfff0");

		return FALSE;
	}

	factor = 0x10000 - factor;
	u16 a = (factor & 0xff00) | divisor;

	// CH341A buffers data until a full endpoint-size packet (32 bytes)
	// has been received unless bit 7 is set.
	a |= BIT(7);

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   CH341_REQ_WRITE_REG,
				   0x1312,
				   a,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromCh341, LogError, "Cannot setup baud rate");

		return FALSE;
	}

	m_nBaudRate = nBaudRate;
	//CLogger::Get ()->Write (FromCh341, LogDebug, "Baud rate %d", m_nBaudRate);

	return TRUE;
}

boolean CUSBSerialCH341Device::SetLineProperties (TUSBSerialDataBits nDataBits,
						  TUSBSerialParity nParity,
						  TUSBSerialStopBits nStopBits)
{
	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	CString framing;

	u8 lcr = CH341_LCR_ENABLE_RX | CH341_LCR_ENABLE_TX;

	switch (nDataBits) {
	case USBSerialDataBits5:
		lcr |= CH341_LCR_CS5;
		break;
	case USBSerialDataBits6:
		lcr |= CH341_LCR_CS6;
		break;
	case USBSerialDataBits7:
		lcr |= CH341_LCR_CS7;
		break;
	case USBSerialDataBits8:
		lcr |= CH341_LCR_CS8;
		break;
	default:
		CLogger::Get ()->Write (FromCh341, LogError, "Invalid data bits %d", nDataBits);
		return FALSE;
		break;
	}
	framing.Format ("%d", nDataBits);

	switch (nParity) {
	case USBSerialParityNone:
		framing.Append ("N");
		break;
	case USBSerialParityOdd:
		lcr |= CH341_LCR_ENABLE_PAR;
		framing.Append ("O");
		break;
	case USBSerialParityEven:
		lcr |= CH341_LCR_ENABLE_PAR | CH341_LCR_PAR_EVEN;
		framing.Append ("E");
		break;
	default:
		CLogger::Get ()->Write (FromCh341, LogError, "Invalid parity %d", nParity);
		return FALSE;
		break;
	}

	switch (nStopBits) {
	case USBSerialStopBits1:
		framing.Append ("1");
		break;
	case USBSerialStopBits2:
		lcr |= CH341_LCR_STOP_BITS_2;
		framing.Append ("2");
		break;
	default:
		CLogger::Get ()->Write (FromCh341, LogError, "Invalid stop bits %d", nStopBits);
		return FALSE;
		break;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   CH341_REQ_WRITE_REG,
				   0x2518,
				   lcr,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromCh341, LogError, "Cannot init serial port lcr");

		return FALSE;
	}

	u8 mcr = 0;
	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   CH341_REQ_MODEM_CTRL,
				   ~mcr,
				   0,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromCh341, LogError, "Cannot init serial port mcr");

		return FALSE;
	}

	m_nDataBits = nDataBits;
	m_nParity = nParity;
	m_nStopBits = nStopBits;

	//CLogger::Get ()->Write (FromCh341, LogDebug, "Framing %s", (const char *)framing);

	return TRUE;
}

const TUSBDeviceID *CUSBSerialCH341Device::GetDeviceIDTable (void)
{
	static const TUSBDeviceID DeviceIDTable[] =
	{
		{ USB_DEVICE (0x4348, 0x5523) },
		{ USB_DEVICE (0x1A86, 0x7522) },
		{ USB_DEVICE (0x1A86, 0x7523) },
		{ USB_DEVICE (0x1A86, 0x5523) },
		{ }
	};

	return DeviceIDTable;
}
