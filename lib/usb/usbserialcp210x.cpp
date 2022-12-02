//
// usbserialcp210x.cpp
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
#include <circle/usb/usbserialcp210x.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/devicenameservice.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <assert.h>

static const char FromCp210x[] = "cp210x";

#define DEFAULT_BAUD_RATE	9600

// Config request codes
#define CP210X_IFC_ENABLE	0x00
#define CP210X_SET_LINE_CTL	0x03
#define CP210X_SET_BAUDRATE	0x1E
#define CP210X_VENDOR_SPECIFIC	0xFF

// CP210X_IFC_ENABLE
#define UART_ENABLE		0x0001
#define UART_DISABLE		0x0000

// CP210X_(SET|GET)_LINE_CTL
#define BITS_DATA_5		0X0500
#define BITS_DATA_6		0X0600
#define BITS_DATA_7		0X0700
#define BITS_DATA_8		0X0800

#define BITS_PARITY_NONE	0x0000
#define BITS_PARITY_ODD		0x0010
#define BITS_PARITY_EVEN	0x0020

#define BITS_STOP_1		0x0000
#define BITS_STOP_2		0x0002

// CP210X_VENDOR_SPECIFIC values
#define CP210X_GET_PARTNUM	0x370B

const CUSBSerialCP210xDevice::TDeviceInfo CUSBSerialCP210xDevice::s_DeviceInfo[] =
{
	{0x01,  921600U, USBSerialDataBits8, USBSerialStopBits1, "CP2101"},
	{0x02,  921600U, USBSerialDataBits5, USBSerialStopBits2, "CP2102"},
	{0x03,  921600U, USBSerialDataBits5, USBSerialStopBits2, "CP2103"},
	{0x04,  921600U, USBSerialDataBits5, USBSerialStopBits2, "CP2104"},
	{0x05,  921600U, USBSerialDataBits5, USBSerialStopBits2, "CP2105"},
	{0x08,  921600U, USBSerialDataBits5, USBSerialStopBits2, "CP2108"},
	{0x09,  921600U, USBSerialDataBits5, USBSerialStopBits2, "CP2109"},
	{0x20, 3000000U, USBSerialDataBits5, USBSerialStopBits2, "CP2102N"},
	{0x21, 3000000U, USBSerialDataBits5, USBSerialStopBits2, "CP2102N"},
	{0x22, 3000000U, USBSerialDataBits5, USBSerialStopBits2, "CP2102N"},

	{0x00,       0U}
};

CUSBSerialCP210xDevice::CUSBSerialCP210xDevice (CUSBFunction *pFunction)
:	CUSBSerialDevice (pFunction),
	m_pDeviceInfo (0)
{
}

CUSBSerialCP210xDevice::~CUSBSerialCP210xDevice (void)
{
}

boolean CUSBSerialCP210xDevice::Configure (void)
{
	if (!CUSBSerialDevice::Configure ())
	{
		CLogger::Get ()->Write (FromCp210x, LogError, "Cannot configure serial device");

		return FALSE;
	}

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	DMA_BUFFER (u8, rxData, 2) = {0};
	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_IN | REQUEST_VENDOR | REQUEST_TO_DEVICE,
				   CP210X_VENDOR_SPECIFIC,
				   CP210X_GET_PARTNUM,
				   0,
				   rxData, 1) != (int) 1)
	{
		CLogger::Get ()->Write (FromCp210x, LogError, "Cannot get part number byte");

		return FALSE;
	}

	m_pDeviceInfo = 0;
	for (const TDeviceInfo *pDeviceInfo = s_DeviceInfo; pDeviceInfo->MaxBaudRate; pDeviceInfo++)
	{
		if (pDeviceInfo->PartNumber == rxData[0])
		{
			m_pDeviceInfo = pDeviceInfo;

			break;
		}
	}

	if (m_pDeviceInfo == 0)
	{
		CLogger::Get ()->Write (FromCp210x, LogError, "Unsupported part number: 0x%02X",
					rxData[0]);

		return FALSE;
	}
	CLogger::Get ()->Write (FromCp210x, LogNotice, "Part number: %s", m_pDeviceInfo->Name);

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_INTERFACE,
				   CP210X_IFC_ENABLE,
				   UART_ENABLE,
				   0,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromCp210x, LogError, "Cannot enable serial port");

		return FALSE;
	}
	//CLogger::Get ()->Write (FromCp210x, LogDebug, "Serial port enabled!");

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

boolean CUSBSerialCP210xDevice::SetBaudRate (unsigned nBaudRate)
{
	assert (m_pDeviceInfo != 0);
	if (nBaudRate > m_pDeviceInfo->MaxBaudRate)
	{
		CLogger::Get ()->Write (FromCp210x, LogError, "Unsupported baud rate (%u)",
					nBaudRate);

		return FALSE;
	}

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_INTERFACE,
				   CP210X_SET_BAUDRATE,
				   0,
				   0,
				   &nBaudRate, 4) < 0)
	{
		CLogger::Get ()->Write (FromCp210x, LogError, "Cannot set baud rate");

		return FALSE;
	}

	m_nBaudRate = nBaudRate;
	CLogger::Get ()->Write (FromCp210x, LogDebug, "Baud rate %d", m_nBaudRate);

	return TRUE;
}

boolean CUSBSerialCP210xDevice::SetLineProperties (TUSBSerialDataBits nDataBits,
						   TUSBSerialParity nParity,
						   TUSBSerialStopBits nStopBits)
{
	assert (m_pDeviceInfo != 0);
	if (nDataBits < m_pDeviceInfo->MinDataBits)
	{
		CLogger::Get ()->Write (FromCp210x, LogError, "Unsupported data bits %d", nDataBits);

		return FALSE;
	}

	if (nStopBits > m_pDeviceInfo->MaxStopBits)
	{
		CLogger::Get ()->Write (FromCp210x, LogError, "Unsupported stop bits %d", nStopBits);

		return FALSE;
	}

	CUSBHostController *pHost = GetHost ();
	assert (pHost != 0);

	CString framing;
	u16 lcr = 0;

	switch (nDataBits) {
	case USBSerialDataBits5:
		lcr |= BITS_DATA_5;
		break;
	case USBSerialDataBits6:
		lcr |= BITS_DATA_6;
		break;
	case USBSerialDataBits7:
		lcr |= BITS_DATA_7;
		break;
	case USBSerialDataBits8:
		lcr |= BITS_DATA_8;
		break;
	default:
		CLogger::Get ()->Write (FromCp210x, LogError, "Invalid data bits %d", nDataBits);
		return FALSE;
		break;
	}
	framing.Format ("%d", nDataBits);

	switch (nParity) {
	case USBSerialParityNone:
		framing.Append ("N");
		break;
	case USBSerialParityOdd:
		lcr |= BITS_PARITY_ODD;
		framing.Append ("O");
		break;
	case USBSerialParityEven:
		lcr |= BITS_PARITY_EVEN;
		framing.Append ("E");
		break;
	default:
		CLogger::Get ()->Write (FromCp210x, LogError, "Invalid parity %d", nParity);
		return FALSE;
		break;
	}

	switch (nStopBits) {
	case USBSerialStopBits1:
		framing.Append ("1");
		break;
	case USBSerialStopBits2:
		lcr |= BITS_STOP_2;
		framing.Append ("2");
		break;
	default:
		CLogger::Get ()->Write (FromCp210x, LogError, "Invalid stop bits %d", nStopBits);
		return FALSE;
		break;
	}

	if (pHost->ControlMessage (GetEndpoint0 (),
				   REQUEST_OUT | REQUEST_VENDOR | REQUEST_TO_INTERFACE,
				   CP210X_SET_LINE_CTL,
				   lcr,
				   0,
				   0, 0) < 0)
	{
		CLogger::Get ()->Write (FromCp210x, LogError, "Cannot set line properties");

		return FALSE;
	}

	m_nDataBits = nDataBits;
	m_nParity = nParity;
	m_nStopBits = nStopBits;

	//CLogger::Get ()->Write (FromCp210x, LogDebug, "Framing %s", (const char *)framing);

	return TRUE;
}

const TUSBDeviceID *CUSBSerialCP210xDevice::GetDeviceIDTable (void)
{
	static const TUSBDeviceID DeviceIDTable[] =
	{
		{ USB_DEVICE (0x10C4, 0xEA60) },
		{ }
	};

	return DeviceIDTable;
}
