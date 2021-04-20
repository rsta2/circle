//
// usbserialft231x.cpp
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
#include <circle/usb/usbserialft231x.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/synchronize.h>
#include <circle/util.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromFt231x[] = "ft231x";

#define DEFAULT_BAUD_RATE		9600

#define FTDI_SIO_GET_LATENCY_TIMER	10
#define FTDI_SIO_RESET			0
#define FTDI_SIO_RESET_SIO		0x0
#define FTDI_SIO_SET_FLOW_CTRL		2
#define FTDI_SIO_DISABLE_FLOW_CTRL	0x0
#define FTDI_SIO_SET_BAUD_RATE		3
#define FTDI_SIO_SET_DATA		4
#define FTDI_SIO_SET_DATA_PARITY_NONE	(0x0 << 8)
#define FTDI_SIO_SET_DATA_PARITY_ODD	(0x1 << 8)
#define FTDI_SIO_SET_DATA_PARITY_EVEN	(0x2 << 8)
#define FTDI_SIO_SET_DATA_STOP_BITS_1	(0x0 << 11)
#define FTDI_SIO_SET_DATA_STOP_BITS_2	(0x2 << 11)

CUSBSerialFT231XDevice::CUSBSerialFT231XDevice (CUSBFunction *pFunction)
:	CUSBSerialDevice (pFunction, 2)
{
}

CUSBSerialFT231XDevice::~CUSBSerialFT231XDevice (void)
{
}

boolean CUSBSerialFT231XDevice::Configure (void)
{
	if (!CUSBSerialDevice::Configure ())
	{
		CLogger::Get ()->Write (FromFt231x, LogError, "Cannot configure serial device");

		return FALSE;
	}

	CUSBDevice *device = GetDevice ();
	const TUSBDeviceDescriptor *deviceDesc = device->GetDeviceDescriptor ();
	CString type = "??";
	if (deviceDesc->bcdDevice == 0x1000)
	{
		type = "FT-X";
	}
	else if (deviceDesc->bcdDevice == 0x600)
	{
		type = "FT232RL";
	}
	else if (deviceDesc->bcdDevice == 0x700 || deviceDesc->bcdDevice == 0x900)
	{
		type = "FT232H";
	}
	CLogger::Get ()->Write (FromFt231x, LogNotice, "Part number: %s", (const char *)type);

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   FTDI_SIO_RESET,
				   0,
				   0,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromFt231x, LogError, "Cannot reset device");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   FTDI_SIO_SET_FLOW_CTRL,
				   0,
				   0,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromFt231x, LogError, "Cannot disable flow control");

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

boolean CUSBSerialFT231XDevice::SetBaudRate (unsigned nBaudRate)
{
	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	const int base = 48000000;
	const u8 divfrac[8] = { 0, 3, 2, 4, 1, 5, 6, 7 };
	u32 divisor;
	// divisor shifted 3 bits to the left
	int divisor3 = base / 2 / nBaudRate;
	divisor = divisor3 >> 3;
	divisor |= (u32) divfrac[divisor3 & 0x7] << 14;
	// deal with special cases for highest baud rates
	if (divisor == 1)
		divisor = 0;
	else if (divisor == 0x4001)
		divisor = 1;

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   FTDI_SIO_SET_BAUD_RATE,
				   (u16) divisor,
				   (u16) (divisor >> 16),
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromFt231x, LogError, "Cannot set baud rate");

		return FALSE;
	}

	m_nBaudRate = nBaudRate;
	CLogger::Get ()->Write (FromFt231x, LogDebug, "Baud rate %d", m_nBaudRate);

	return TRUE;
}

boolean CUSBSerialFT231XDevice::SetLineProperties (TUSBSerialDataBits nDataBits,
						   TUSBSerialParity nParity,
						   TUSBSerialStopBits nStopBits)
{
	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	CString framing;
	u16 lcr = 0;

	switch (nDataBits) {
	case USBSerialDataBits5:
	case USBSerialDataBits6:
		CLogger::Get ()->Write (FromFt231x, LogError, "Unsupported data bits %d", nDataBits);
		return FALSE;
		break;
	case USBSerialDataBits7:
		lcr |= 7;
		break;
	case USBSerialDataBits8:
		lcr |= 8;
		break;
	default:
		CLogger::Get ()->Write (FromFt231x, LogError, "Unsupported data bits %d", nDataBits);
		return FALSE;
		break;
	}
	framing.Format ("%d", nDataBits);

	switch (nParity) {
	case USBSerialParityNone:
		lcr |= FTDI_SIO_SET_DATA_PARITY_NONE;
		framing.Append ("N");
		break;
	case USBSerialParityOdd:
		lcr |= FTDI_SIO_SET_DATA_PARITY_ODD;
		framing.Append ("O");
		break;
	case USBSerialParityEven:
		lcr |= FTDI_SIO_SET_DATA_PARITY_EVEN;
		framing.Append ("E");
		break;
	default:
		CLogger::Get ()->Write (FromFt231x, LogError, "Invalid parity %d", nParity);
		return FALSE;
		break;
	}

	switch (nStopBits) {
	case USBSerialStopBits1:
		lcr |= FTDI_SIO_SET_DATA_STOP_BITS_1;
		framing.Append ("1");
		break;
	case USBSerialStopBits2:
		lcr |= FTDI_SIO_SET_DATA_STOP_BITS_2;
		framing.Append ("2");
		break;
	default:
		CLogger::Get ()->Write (FromFt231x, LogError, "Invalid stop bits %d", nStopBits);
		return FALSE;
		break;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   FTDI_SIO_SET_DATA,
				   lcr,
				   0,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromFt231x, LogError, "Cannot set line properties");

		return FALSE;
	}

	m_nDataBits = nDataBits;
	m_nParity = nParity;
	m_nStopBits = nStopBits;

	CLogger::Get ()->Write (FromFt231x, LogDebug, "Framing %s", (const char *)framing);

	return TRUE;
}

const TUSBDeviceID *CUSBSerialFT231XDevice::GetDeviceIDTable (void)
{
	static const TUSBDeviceID DeviceIDTable[] =
	{
		{ USB_DEVICE (0x0403, 0x6001) },
		{ USB_DEVICE (0x0403, 0x6010) },
		{ USB_DEVICE (0x0403, 0x6014) },	// FT232H
		{ USB_DEVICE (0x0403, 0x6015) },
		{ }
	};

	return DeviceIDTable;
}
