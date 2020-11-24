//
// usbserialpl2303.cpp
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
#include <circle/usb/usbserialpl2303.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/synchronize.h>
#include <circle/util.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromPl2303[] = "pl2303";

#define DEFAULT_BAUD_RATE	9600

// Config request codes
#define VENDOR_READ_REQUEST	0x01
#define VENDOR_WRITE_REQUEST	0x01
#define GET_LINE_REQUEST	0x21
#define SET_LINE_REQUEST	0x20

CUSBSerialPL2303Device::CUSBSerialPL2303Device (CUSBFunction *pFunction)
:	CUSBSerialDevice (pFunction)
{
}

CUSBSerialPL2303Device::~CUSBSerialPL2303Device (void)
{
}

boolean CUSBSerialPL2303Device::Configure (void)
{
	if (!CUSBSerialDevice::Configure ())
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot configure serial device");

		return FALSE;
	}

	CUSBDevice *device = GetDevice ();
	const TUSBDeviceDescriptor *deviceDesc = device->GetDeviceDescriptor ();
	CString type = "0";
	if (deviceDesc->bDeviceClass == 0x02)
	{
		type = "0";
	}
	else if (deviceDesc->bMaxPacketSize0 == 0x40)
	{
		type = "HX";
	}
	else if (deviceDesc->bDeviceClass == 0x00 || deviceDesc->bDeviceClass == 0xFF)
	{
		type = "1";
	}
	CLogger::Get ()->Write (FromPl2303, LogNotice, "Part number: PL2303 type %s", (const char *)type);

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	DMA_BUFFER (u8, rxData, 1) = {0};
	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_IN | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_READ_REQUEST,
				   0x8484,
				   0,
				   rxData, 1) != (int) 1)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot read 0x8484");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_WRITE_REQUEST,
				   0x0404,
				   0,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot write 0x0404");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_IN | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_READ_REQUEST,
				   0x8484,
				   0,
				   rxData, 1) != (int) 1)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot read 0x8484");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_IN | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_READ_REQUEST,
				   0x8383,
				   0,
				   rxData, 1) != (int) 1)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot read 0x8383");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_IN | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_READ_REQUEST,
				   0x8484,
				   0,
				   rxData, 1) != (int) 1)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot read 0x8484");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_WRITE_REQUEST,
				   0x0404,
				   1,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot write 0x0404");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_IN | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_READ_REQUEST,
				   0x8484,
				   0,
				   rxData, 1) != (int) 1)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot read 0x8484");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_IN | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_READ_REQUEST,
				   0x8383,
				   0,
				   rxData, 1) != (int) 1)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot read 0x8383");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_WRITE_REQUEST,
				   0,
				   1,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot write 0");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_WRITE_REQUEST,
				   1,
				   0,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot write 1");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_WRITE_REQUEST,
				   2,
				   0x44,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot write 2");

		return FALSE;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   VENDOR_WRITE_REQUEST,
				   0,
				   0,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot write 0");

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

boolean CUSBSerialPL2303Device::SetBaudRate (unsigned nBaudRate)
{
	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	DMA_BUFFER (u8, rxData, 7) = {0};
	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE,
				   GET_LINE_REQUEST,
				   0,
				   0,
				   rxData, 7) != (int) 7)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot get baud rate");

		return FALSE;
	}

	memcpy (rxData, (u8 *) &nBaudRate, 4);
	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
				   SET_LINE_REQUEST,
				   0,
				   0,
				   rxData, 7) < 0)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot set baud rate");

		return FALSE;
	}

	m_nBaudRate = nBaudRate;

	//CLogger::Get ()->Write (FromPl2303, LogDebug, "Baud rate %d", m_nBaudRate);

	return TRUE;
}

boolean CUSBSerialPL2303Device::SetLineProperties (TUSBSerialDataBits nDataBits,
						   TUSBSerialParity nParity,
						   TUSBSerialStopBits nStopBits)
{
	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	DMA_BUFFER (u8, rxData, 7) = {0};
	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE,
				   GET_LINE_REQUEST,
				   0,
				   0,
				   rxData, 7) != (int) 7)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot get line properties");

		return FALSE;
	}

	CString framing;

	switch (nDataBits) {
	case USBSerialDataBits5:
		rxData[6] = 5;
		break;
	case USBSerialDataBits6:
		rxData[6] = 6;
		break;
	case USBSerialDataBits7:
		rxData[6] = 7;
		break;
	case USBSerialDataBits8:
		rxData[6] = 8;
		break;
	default:
		CLogger::Get ()->Write (FromPl2303, LogError, "Invalid data bits %d", nDataBits);
		return FALSE;
		break;
	}
	framing.Format ("%d", nDataBits);

	switch (nParity) {
	case USBSerialParityNone:
		rxData[5] = 0;
		framing.Append ("N");
		break;
	case USBSerialParityOdd:
		rxData[5] = 1;
		framing.Append ("O");
		break;
	case USBSerialParityEven:
		rxData[5] = 2;
		framing.Append ("E");
		break;
	default:
		CLogger::Get ()->Write (FromPl2303, LogError, "Invalid parity %d", nParity);
		return FALSE;
		break;
	}

	switch (nStopBits) {
	case USBSerialStopBits1:
		rxData[4] = 0;
		framing.Append ("1");
		break;
	case USBSerialStopBits2:
		rxData[4] = 2;
		framing.Append ("2");
		break;
	default:
		CLogger::Get ()->Write (FromPl2303, LogError, "Invalid stop bits %d", nStopBits);
		return FALSE;
		break;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
				   SET_LINE_REQUEST,
				   0,
				   0,
				   rxData, 7) < 0)
	{
		CLogger::Get ()->Write (FromPl2303, LogError, "Cannot set line properties");

		return FALSE;
	}

	m_nDataBits = nDataBits;
	m_nParity = nParity;
	m_nStopBits = nStopBits;

	//CLogger::Get ()->Write (FromPl2303, LogDebug, "Framing %s", (const char *)framing);

	return TRUE;
}

const TUSBDeviceID *CUSBSerialPL2303Device::GetDeviceIDTable (void)
{
	static const TUSBDeviceID DeviceIDTable[] =
	{
		{ USB_DEVICE (0x067B, 0x2303) },
		{ }
	};

	return DeviceIDTable;
}
