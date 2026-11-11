#pragma once
#include <circle/types.h>
struct TUSBEndpointDescriptor
{
	u8 bLength, bDescriptorType, bEndpointAddress, bmAttributes;
	u16 wMaxPacketSize;
	u8 bInterval;
};
struct TSetupData
{
	u8 bmRequestType, bRequest;
	u16 wValue, wIndex, wLength;
};
#define REQUEST_IN 0x80
#define REQUEST_CLASS 0x20
#define REQUEST_VENDOR 0x40
#define GET_DESCRIPTOR 6
#define GET_STATUS 0
#define SET_ADDRESS 5
#define SET_INTERFACE 11
#define SET_CONFIGURATION 9

#define DESCRIPTOR_CONFIGURATION 2
struct __attribute__((packed)) TUSBConfigurationDescriptor
{
	u8 bLength, bDescriptorType;
	u16 wTotalLength;
	u8 bNumInterfaces, bConfigurationValue, iConfiguration, bmAttributes, bMaxPower;
};
