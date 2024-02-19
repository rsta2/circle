//
// usbserial.h
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
#ifndef _circle_usb_usbserial_h
#define _circle_usb_usbserial_h

#include <circle/device.h>
#include <circle/numberpool.h>
#include <circle/types.h>

enum TUSBSerialDataBits
{
	USBSerialDataBits5	 = 5,
	USBSerialDataBits6	 = 6,
	USBSerialDataBits7	 = 7,
	USBSerialDataBits8	 = 8,
};

enum TUSBSerialStopBits
{
	USBSerialStopBits1	 = 1,
	USBSerialStopBits2	 = 2,
};

enum TUSBSerialParity
{
	USBSerialParityNone	 = 0,
	USBSerialParityOdd	 = 1,
	USBSerialParityEven	 = 2,
};

// serial options
#define SERIAL_OPTION_ONLCR	(1 << 0)	///< Translate NL to CR+NL on output

class CUSBSerialDevice : public CDevice		/// Interface device for USB serial devices
{
public:
	CUSBSerialDevice (void);
	~CUSBSerialDevice (void);

	boolean Configure (void);

	int Write (const void *pBuffer, size_t nCount);
	int Read (void *pBuffer, size_t nCount);

	boolean SetBaudRate (unsigned nBaudRate);
	boolean SetLineProperties (TUSBSerialDataBits nDataBits,
				   TUSBSerialParity nParity,
				   TUSBSerialStopBits nStopBits);

	/// \return Serial options mask (see serial options)
	unsigned GetOptions (void) const;
	/// \param nOptions Serial options mask (see serial options)
	void SetOptions (unsigned nOptions);

private:
	typedef int TWriteHandler (const void *pBuffer, size_t nCount, void *pParam);
	typedef int TReadHandler (void *pBuffer, size_t nCount, void *pParam);
	typedef boolean TSetBaudRateHandler (unsigned nBaudRate, void *pParam);
	typedef boolean TSetLinePropertiesHandler (TUSBSerialDataBits nDataBits,
						   TUSBSerialParity nParity,
						   TUSBSerialStopBits nStopBits,
						   void *pParam);

	void RegisterWriteHandler (TWriteHandler *pHandler, void *pParam);
	void RegisterReadHandler (TReadHandler *pHandler, void *pParam);
	void RegisterSetBaudRateHandler (TSetBaudRateHandler *pHandler, void *pParam);
	void RegisterSetLinePropertiesHandler (TSetLinePropertiesHandler *pHandler, void *pParam);

	friend class CUSBSerialHostDevice;
	friend class CUSBCDCGadgetEndpoint;

private:
	TWriteHandler *m_pWriteHandler;
	TReadHandler *m_pReadHandler;
	TSetBaudRateHandler *m_pSetBaudRateHandler;
	TSetLinePropertiesHandler *m_pSetLinePropertiesHandler;

	void *m_pWriteParam;
	void *m_pReadParam;
	void *m_pSetBaudRateParam;
	void *m_pSetLinePropertiesParam;

	unsigned m_nOptions;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;
};

#endif
