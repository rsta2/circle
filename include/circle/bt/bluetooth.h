//
// bluetooth.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_bt_bluetooth_h
#define _circle_bt_bluetooth_h

#include <circle/macros.h>
#include <circle/types.h>

////////////////////////////////////////////////////////////////////////////////
//
// General definitions
//
////////////////////////////////////////////////////////////////////////////////

// Class of Device

#define BT_CLASS_DESKTOP_COMPUTER	0x002104

// Sizes

#define BT_MAX_HCI_EVENT_SIZE	257
#define BT_MAX_HCI_COMMAND_SIZE	258
#define BT_MAX_DATA_SIZE	BT_MAX_HCI_COMMAND_SIZE

#define BT_BD_ADDR_SIZE		6
#define BT_CLASS_SIZE		3
#define BT_NAME_SIZE		248

// Error codes

#define BT_STATUS_SUCCESS	0

////////////////////////////////////////////////////////////////////////////////
//
// HCI
//
////////////////////////////////////////////////////////////////////////////////

// Commands

struct TBTHCICommandHeader
{
	u16	OpCode;
#define OGF_LINK_CONTROL		(1 << 10)
	#define OP_CODE_INQUIRY			(OGF_LINK_CONTROL | 0x001)
	#define OP_CODE_REMOTE_NAME_REQUEST	(OGF_LINK_CONTROL | 0x019)
#define OGF_HCI_CONTROL_BASEBAND	(3 << 10)
	#define OP_CODE_RESET			(OGF_HCI_CONTROL_BASEBAND | 0x003)
	#define OP_CODE_WRITE_LOCAL_NAME	(OGF_HCI_CONTROL_BASEBAND | 0x013)
	#define OP_CODE_WRITE_SCAN_ENABLE	(OGF_HCI_CONTROL_BASEBAND | 0x01A)
	#define OP_CODE_WRITE_CLASS_OF_DEVICE	(OGF_HCI_CONTROL_BASEBAND | 0x024)
#define OGF_INFORMATIONAL_COMMANDS	(4 << 10)
	#define OP_CODE_READ_BD_ADDR		(OGF_INFORMATIONAL_COMMANDS | 0x009)
#define OGF_VENDOR_COMMANDS		(0x3F << 10)
	u8	ParameterTotalLength;
}
PACKED;

#define PARM_TOTAL_LEN(cmd)		(sizeof (cmd) - sizeof (TBTHCICommandHeader))

struct TBTHCIInquiryCommand
{
	TBTHCICommandHeader	Header;

	u8	LAP[BT_CLASS_SIZE];
#define INQUIRY_LAP_GIAC		0x9E8B33	// General Inquiry Access Code
	u8	InquiryLength;
#define INQUIRY_LENGTH_MIN		0x01		// 1.28s
#define INQUIRY_LENGTH_MAX		0x30		// 61.44s
#define INQUIRY_LENGTH(secs)		(((secs) * 100 + 64) / 128)
	u8	NumResponses;
#define INQUIRY_NUM_RESPONSES_UNLIMITED	0x00
}
PACKED;

struct TBTHCIRemoteNameRequestCommand
{
	TBTHCICommandHeader	Header;

	u8	BDAddr[BT_BD_ADDR_SIZE];
	u8	PageScanRepetitionMode;
#define PAGE_SCAN_REPETITION_R0		0x00
#define PAGE_SCAN_REPETITION_R1		0x01
#define PAGE_SCAN_REPETITION_R2		0x02
	u8	Reserved;				// set to 0
	u16	ClockOffset;
#define CLOCK_OFFSET_INVALID		0		// bit 15 is not set
}
PACKED;

struct TBTHCIWriteLocalNameCommand
{
	TBTHCICommandHeader	Header;

	u8	LocalName[BT_NAME_SIZE];
}
PACKED;

struct TBTHCIWriteScanEnableCommand
{
	TBTHCICommandHeader	Header;

	u8	ScanEnable;
#define SCAN_ENABLE_NONE		0x00
#define SCAN_ENABLE_INQUIRY_ENABLED	0x01
#define SCAN_ENABLE_PAGE_ENABLED	0x02
#define SCAN_ENABLE_BOTH_ENABLED	0x03
}
PACKED;

struct TBTHCIWriteClassOfDeviceCommand
{
	TBTHCICommandHeader	Header;

	u8	ClassOfDevice[BT_CLASS_SIZE];
}
PACKED;

// Events

struct TBTHCIEventHeader
{
	u8	EventCode;
#define EVENT_CODE_INQUIRY_COMPLETE		0x01
#define EVENT_CODE_INQUIRY_RESULT		0x02
#define EVENT_CODE_REMOTE_NAME_REQUEST_COMPLETE	0x07
#define EVENT_CODE_COMMAND_COMPLETE		0x0E
#define EVENT_CODE_COMMAND_STATUS		0x0F
	u8	ParameterTotalLength;
}
PACKED;

struct TBTHCIEventInquiryComplete
{
	TBTHCIEventHeader	Header;

	u8	Status;
}
PACKED;

struct TBTHCIEventInquiryResult
{
	TBTHCIEventHeader	Header;

	u8	NumResponses;

//	u8	BDAddr[NumResponses][BT_BD_ADDR_SIZE];
//	u8	PageScanRepetitionMode[NumResponses];
//	u8	Reserved[NumResponses][2];
//	u8	ClassOfDevice[NumResponses][BT_CLASS_SIZE];
//	u16	ClockOffset[NumResponses];
	u8	Data[0];
#define INQUIRY_RESP_SIZE			14
#define INQUIRY_RESP_BD_ADDR(p, i)		(&(p)->Data[(i)*BT_BD_ADDR_SIZE])
#define INQUIRY_RESP_PAGE_SCAN_REP_MODE(p, i)	((p)->Data[(p)->NumResponses*BT_BD_ADDR_SIZE + (i)])
#define INQUIRY_RESP_CLASS_OF_DEVICE(p, i)	(&(p)->Data[(p)->NumResponses*(BT_BD_ADDR_SIZE+1+2) \
							   + (i)*BT_CLASS_SIZE])
}
PACKED;

struct TBTHCIEventRemoteNameRequestComplete
{
	TBTHCIEventHeader	Header;

	u8	Status;
	u8	BDAddr[BT_BD_ADDR_SIZE];
	u8	RemoteName[BT_NAME_SIZE];
}
PACKED;

struct TBTHCIEventCommandComplete
{
	TBTHCIEventHeader	Header;

	u8	NumHCICommandPackets;
	u16	CommandOpCode;
	u8	Status;				// normally part of ReturnParameter[]
	u8	ReturnParameter[0];
}
PACKED;

struct TBTHCIEventReadBDAddrComplete
{
	TBTHCIEventCommandComplete	Header;

	u8	BDAddr[BT_BD_ADDR_SIZE];
}
PACKED;

struct TBTHCIEventCommandStatus
{
	TBTHCIEventHeader	Header;

	u8	Status;
	u8	NumHCICommandPackets;
	u16	CommandOpCode;
}
PACKED;

#endif
