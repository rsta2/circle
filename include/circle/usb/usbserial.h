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

#include <circle/usb/usbfunction.h>
#include <circle/usb/usbendpoint.h>
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

class CUSBSerialDevice : public CUSBFunction
{
public:
	CUSBSerialDevice (CUSBFunction *pFunction,
			  size_t nReadHeaderBytes = 0);		// number of bytes to be ignored
	virtual ~CUSBSerialDevice (void);

	boolean Configure (void);

	int Write (const void *pBuffer, size_t nCount);
	int Read (void *pBuffer, size_t nCount);

	virtual boolean SetBaudRate (unsigned nBaudRate);
	virtual boolean SetLineProperties (TUSBSerialDataBits nDataBits, TUSBSerialParity nParity, TUSBSerialStopBits nStopBits);

private:
	void CompletionRoutine (CUSBRequest *pURB);
	static void CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext);

protected:
	unsigned m_nBaudRate;
	TUSBSerialDataBits m_nDataBits;
	TUSBSerialParity m_nParity;
	TUSBSerialStopBits m_nStopBits;

private:
	size_t m_nReadHeaderBytes;

	CUSBEndpoint *m_pEndpointIn;
	CUSBEndpoint *m_pEndpointOut;

	u8 *m_pBufferIn;
	size_t m_nBufferInSize;
	size_t m_nBufferInValid;
	unsigned m_nBufferInPtr;

	volatile boolean m_bInRequestActive;

	unsigned m_nDeviceNumber;
	static CNumberPool s_DeviceNumberPool;
};

#endif
