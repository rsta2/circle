//
// usbcdcgadget.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_usb_gadget_usbcdcgadget_h
#define _circle_usb_gadget_usbcdcgadget_h

#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/gadget/usbcdcgadgetendpoint.h>
#include <circle/usb/usb.h>
#include <circle/interrupt.h>
#include <circle/device.h>
#include <circle/macros.h>
#include <circle/types.h>

struct TUSBCDCACMInterfaceDescriptor
{
	// header functional descriptor
	unsigned char	bFunctionLength1;
	unsigned char	bDescriptorType1;
	unsigned char	bDescriptorSubtype1;
	unsigned short	bcdCDC;
	// abstract control management functional descriptor
	unsigned char	bFunctionLength2;
	unsigned char	bDescriptorType2;
	unsigned char	bDescriptorSubtype2;
	unsigned char	bmCapabilities1;
	// union functional descriptor
	unsigned char	bFunctionLength3;
	unsigned char	bDescriptorType3;
	unsigned char	bDescriptorSubtype3;
	unsigned char	bMasterInterface;
	unsigned char	bSubordinateInterface0;
	// call management functional descriptor
	/*unsigned char	bFunctionLength4;
	unsigned char	bDescriptorType4;
	unsigned char	bDescriptorSubtype4;
	unsigned char	bmCapabilities2;
	unsigned char	bDataInterface;*/
}
PACKED;

/// \param pBuffer Pointer to the data received over USB serial
/// \param nLength Number of valid bytes in the buffer
typedef void TCDCGadgetReceiveHandler (void *pBuffer, unsigned nLength);


class CUSBCDCGadget : public CDWUSBGadget, CDevice	/// USB CDC gadget
{
public:
	/// \param pInterruptSystem Pointer to the interrupt system object
	CUSBCDCGadget (CInterruptSystem *pInterruptSystem);

	~CUSBCDCGadget (void);

	/// \param pBuffer Buffer to send over USB
	/// \param nCount Number of bytes to be written
	/// \return Number of written bytes or < 0 on failure
	int Write (const void *pBuffer, size_t nCount) override;

	/// \brief Register a handler, which is called, when USB serial data arrives
	/// \param pReceiveHandler Pointer to the handler
	void RegisterReceiveHandler (TCDCGadgetReceiveHandler *pReceiveHandler);


protected:
	/// \brief Get device-specific descriptor
	/// \param wValue Parameter from setup packet (descriptor type (MSB) and index (LSB))
	/// \param wIndex Parameter from setup packet (e.g. language ID for string descriptors)
	/// \param pLength Pointer to variable, which receives the descriptor size
	/// \return Pointer to descriptor or nullptr, if not available
	/// \note May override this to personalize device.
	const void *GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength) override;

	/// \brief Convert string to UTF-16 string descriptor
	/// \param pString Pointer to ASCII C-string
	/// \param pLength Pointer to variable, which receives the descriptor size
	/// \return Pointer to string descriptor in class-internal buffer
	const void *ToStringDescriptor (const char *pString, size_t *pLength);

private:
	void AddEndpoints (void) override;

	void CreateDevice (void) override;

	void OnSuspend (void) override;

private:
	enum TEPNumber
	{
		EPNotif = 1,
		EPOut = 2,
		EPIn  = 3,
		NumEPs
	};

	CUSBCDCGadgetEndpoint *m_pEP[NumEPs];

	u8 m_StringDescriptorBuffer[80];

	TCDCGadgetReceiveHandler *m_pReceiveHandler;

	boolean m_bIsConfigured;

private:
	static const TUSBDeviceDescriptor s_DeviceDescriptor;

	struct TUSBCDCGadgetConfigurationDescriptor
	{
		TUSBConfigurationDescriptor			Configuration;
		TUSBInterfaceDescriptor				Interface0;
		TUSBCDCACMInterfaceDescriptor		CDCACMHeader;
		TUSBEndpointDescriptor				EndpointNotif;
		TUSBInterfaceDescriptor				Interface1;
		TUSBEndpointDescriptor				EndpointOut;
		TUSBEndpointDescriptor				EndpointIn;
	}
	PACKED;

	static const TUSBCDCGadgetConfigurationDescriptor s_ConfigurationDescriptor;

	static const char *const s_StringDescriptor[];
};

#endif
