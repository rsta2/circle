//
// bcmpropertytags.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bcmpropertytags_h
#define _circle_bcmpropertytags_h

#include <circle/bcmmailbox.h>
#include <circle/types.h>

#define PROPTAG_END			0x00000000

#define PROPTAG_GET_FIRMWARE_REVISION	0x00000001
#define PROPTAG_GET_BOARD_MODEL		0x00010001
#define PROPTAG_GET_BOARD_REVISION	0x00010002
#define PROPTAG_GET_MAC_ADDRESS		0x00010003
#define PROPTAG_GET_BOARD_SERIAL	0x00010004
#define PROPTAG_GET_ARM_MEMORY		0x00010005
#define PROPTAG_GET_VC_MEMORY		0x00010006
#define PROPTAG_SET_POWER_STATE		0x00028001
#define PROPTAG_GET_CLOCK_RATE		0x00030002
#define PROPTAG_GET_TEMPERATURE		0x00030006
#define PROPTAG_GET_EDID_BLOCK		0x00030020
#define PROPTAG_SET_CLOCK_RATE		0x00038002
#define PROPTAG_GET_DISPLAY_DIMENSIONS	0x00040003
#define PROPTAG_GET_GPIO_VIRTBUF	0x00040010
#define PROPTAG_SET_VIRTUAL_OFFSET	0x00048009
#define PROPTAG_GET_COMMAND_LINE	0x00050001

struct TPropertyTag
{
	u32	nTagId;
	u32	nValueBufSize;			// bytes, multiple of 4
	u32	nValueLength;			// bytes
	#define VALUE_LENGTH_RESPONSE	(1 << 31)
	//u8	ValueBuffer[0];			// must be padded to be 4 byte aligned
};

struct TPropertyTagSimple
{
	TPropertyTag	Tag;
	u32		nValue;
};

struct TPropertyTagMACAddress
{
	TPropertyTag	Tag;
	u8		Address[6];
	u8		Padding[2];
};

struct TPropertyTagSerial
{
	TPropertyTag	Tag;
	u32		Serial[2];
};

struct TPropertyTagMemory
{
	TPropertyTag	Tag;
	u32		nBaseAddress;
	u32		nSize;			// bytes
};

struct TPropertyTagPowerState
{
	TPropertyTag	Tag;
	u32		nDeviceId;
	#define DEVICE_ID_SD_CARD	0
	#define DEVICE_ID_USB_HCD	3
	u32		nState;
	#define POWER_STATE_OFF		(0 << 0)
	#define POWER_STATE_ON		(1 << 0)
	#define POWER_STATE_WAIT	(1 << 1)
	#define POWER_STATE_NO_DEVICE	(1 << 1)	// in response
};

struct TPropertyTagClockRate
{
	TPropertyTag	Tag;
	u32		nClockId;
	#define CLOCK_ID_EMMC		1
	#define CLOCK_ID_UART		2
	#define CLOCK_ID_CORE		4
	u32		nRate;			// Hz
};

struct TPropertyTemperature
{
	TPropertyTag	Tag;
	u32		nTemperatureId;
	#define TEMPERATURE_ID		0
	u32		nValue;			// degree Celsius * 1000
};

struct TPropertyTagEDIDBlock
{
	TPropertyTag	Tag;
	u32		nBlockNumber;
	#define EDID_FIRST_BLOCK	0
	u32		nStatus;
	#define EDID_STATUS_SUCCESS	0
	u8		Block[128];
};

struct TPropertyTagSetClockRate
{
	TPropertyTag	Tag;
	u32		nClockId;
	u32		nRate;			// Hz
	u32		nSkipSettingTurbo;
};

struct TPropertyTagDisplayDimensions
{
	TPropertyTag	Tag;
	u32		nWidth;
	u32		nHeight;
};

struct TPropertyTagVirtualOffset
{
	TPropertyTag	Tag;
	u32		nOffsetX;
	u32		nOffsetY;
};

struct TPropertyTagCommandLine
{
	TPropertyTag	Tag;
	u8		String[1024];
};

class CBcmPropertyTags
{
public:
	CBcmPropertyTags (void);
	~CBcmPropertyTags (void);

	boolean GetTag (u32	  nTagId,			// tag identifier
			void	 *pTag,				// pointer to tag struct
			unsigned  nTagSize,			// size of tag struct
			unsigned  nRequestParmSize = 0);	// number of parameter bytes
	
private:
	CBcmMailBox m_MailBox;
};

#endif
