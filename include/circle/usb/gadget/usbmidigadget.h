//
// usbmidigadget.h
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
#ifndef _circle_usb_gadget_usbmidigadget_h
#define _circle_usb_gadget_usbmidigadget_h

#include <circle/usb/gadget/dwusbgadget.h>
#include <circle/usb/gadget/usbmidigadgetendpoint.h>
#include <circle/usb/usbmidi.h>
#include <circle/usb/usb.h>
#include <circle/usb/usbaudio.h>
#include <circle/interrupt.h>
#include <circle/macros.h>
#include <circle/types.h>

class CUSBMIDIGadget : public CDWUSBGadget	/// USB MIDI (v1.0) gadget
{
public:
	/// \param pInterruptSystem Pointer to the interrupt system object
	CUSBMIDIGadget (CInterruptSystem *pInterruptSystem);

	~CUSBMIDIGadget (void);

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
	CUSBMIDIDevice *m_pInterface;

	enum TEPNumber
	{
		EPOut = 1,
		EPIn  = 2,
		NumEPs
	};

	CUSBMIDIGadgetEndpoint *m_pEP[NumEPs];

	u8 m_StringDescriptorBuffer[80];

private:
	static const TUSBDeviceDescriptor s_DeviceDescriptor;

	struct TUSBMIDIGadgetConfigurationDescriptor
	{
		TUSBConfigurationDescriptor			Configuration;

		TUSBInterfaceDescriptor				AudioControlInterface;
		TUSBAudioControlInterfaceDescriptorHeader	AudioControlHeader;

		TUSBInterfaceDescriptor				MIDIInterface;
		TUSBMIDIStreamingInterfaceDescriptorHeader	MIDIHeader;
		TUSBMIDIStreamingInterfaceDescriptorInJack	MIDIInJackEmbedded;
		TUSBMIDIStreamingInterfaceDescriptorInJack	MIDIInJackExternal;
		TUSBMIDIStreamingInterfaceDescriptorOutJack	MIDIOutJackEmbedded;
		TUSBMIDIStreamingInterfaceDescriptorOutJack	MIDIOutJackExternal;

		TUSBAudioEndpointDescriptor			EndpointOut;
		TUSBMIDIStreamingEndpointDescriptor		MIDIEndpointOut;

		TUSBAudioEndpointDescriptor			EndpointIn;
		TUSBMIDIStreamingEndpointDescriptor		MIDIEndpointIn;
	}
	PACKED;

	static const TUSBMIDIGadgetConfigurationDescriptor s_ConfigurationDescriptor;

	static const char *const s_StringDescriptor[];
};

#endif
