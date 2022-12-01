//
// usbtouchscreen.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include <circle/usb/usbtouchscreen.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/usb/usbhid.h>
#include <circle/synchronize.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <assert.h>

#define HID_USAGE_PAGE		0x04
#define HID_USAGE		0x08
#define HID_REPORT_SIZE		0x74
#define HID_INPUT		0x80
#define HID_REPORT_ID		0x84
#define HID_REPORT_COUNT	0x94
#define HID_COLLECTION		0xA0
#define HID_END_COLLECTION	0xC0

#define HID_USAGE_PAGE_GENERIC_DESKTOP_CTRLS 0x01
	#define HID_USAGE_X			0x30
	#define HID_USAGE_Y			0x31

#define HID_USAGE_PAGE_DIGITIZER	0x0D
	#define HID_USAGE_TOUCH_SCREEN		0x04
	#define HID_USAGE_FINGER		0x22
	#define HID_USAGE_TIP_PRESSURE		0x30
	#define HID_USAGE_TIP_SWITCH		0x42
	#define HID_USAGE_WIDTH			0x48
	#define HID_USAGE_CONTACT_IDENTIFIER 	0x51
	#define HID_USAGE_CONTACT_COUNT		0x54
	#define HID_USAGE_SCAN_TIME		0x56

#define HID_COLLECTION_APPLICATION	0x01
#define HID_COLLECTION_LOGICAL		0x02

LOGMODULE ("utouch");

CUSBTouchScreenDevice::CUSBTouchScreenDevice (CUSBFunction *pFunction)
:	CUSBHIDDevice (pFunction),
	m_pDevice (0)
{
	memset (&m_Report, 0, sizeof m_Report);

	for (unsigned i = 0; i < MaxContactCount; i++)
	{
		m_bFingerIsDown[i] = FALSE;
	}
}

CUSBTouchScreenDevice::~CUSBTouchScreenDevice (void)
{
	delete m_pDevice;
	m_pDevice = 0;
}

boolean CUSBTouchScreenDevice::Configure (void)
{
	TUSBHIDDescriptor *pHIDDesc = (TUSBHIDDescriptor *) GetDescriptor (DESCRIPTOR_HID);
	if (   pHIDDesc == 0
	    || pHIDDesc->wReportDescriptorLength == 0)
	{
		ConfigurationError (From);

		return FALSE;
	}

	u16 nDescSize = pHIDDesc->wReportDescriptorLength;
	DMA_BUFFER (u8, ReportDescriptor, nDescSize);

	if (   GetHost ()->GetDescriptor (GetEndpoint0 (),
					  pHIDDesc->bReportDescriptorType, DESCRIPTOR_INDEX_DEFAULT,
					  ReportDescriptor, nDescSize,
					  REQUEST_IN | REQUEST_TO_INTERFACE, GetInterfaceNumber ())
	    != nDescSize)
	{
		LOGERR ("Cannot get HID report descriptor");

		return FALSE;
	}

	//debug_hexdump (ReportDescriptor, nDescSize, From);

	if (!DecodeReportDescriptor (ReportDescriptor, nDescSize))
	{
		LOGERR ("Invalid HID report descriptor");

		return FALSE;
	}

	if (!CUSBHIDDevice::ConfigureHID (m_Report.ByteSize))
	{
		LOGERR ("Cannot configure HID device");

		return FALSE;
	}

	if (!StartRequest ())
	{
		return FALSE;
	}

	assert (m_pDevice == 0);
	m_pDevice = new CTouchScreenDevice;
	assert (m_pDevice != 0);

	return TRUE;
}

void CUSBTouchScreenDevice::ReportHandler (const u8 *pReport, unsigned nReportSize)
{
	assert (m_pDevice != 0);

	if (   pReport == 0
	    || nReportSize != m_Report.ByteSize)
	{
		return;
	}

	// if Report ID was specified in report descriptor, it must match
	if (   m_Report.ReportID != 0
	    && pReport[0] != m_Report.ReportID)
	{
		return;
	}

	unsigned nContactCount = GetValue (pReport, m_Report.ContactCount, 1);
	assert (nContactCount <= MaxContactCount);
	u8 ucDownIDs[MaxContactCount];

	for (unsigned i = 0; i < nContactCount; i++)
	{
		if (   GetValue (pReport, m_Report.Finger[i].TipSwitch, 1)
		    && GetValue (pReport, m_Report.Finger[i].TipPressure, 1))
		{
			u8 ucContactID = GetValue (pReport, m_Report.Finger[i].ContactIdentifier);
			ucDownIDs[i] = ucContactID;

			unsigned x = GetValue (pReport, m_Report.Finger[i].X);
			unsigned y = GetValue (pReport, m_Report.Finger[i].Y);

			// was this Contact ID already known?
			unsigned j;
			for (j = 0; j < m_Report.MaxContactCountActual; j++)
			{
				if (   m_bFingerIsDown[j]
				    && m_ucContactID[j] == ucContactID)
				{
					break;
				}
			}

			if (j == m_Report.MaxContactCountActual)
			{
				// not known: new finger down

				// find free slot; take last one, if not available
				for (j = 0; j < m_Report.MaxContactCountActual-1; j++)
				{
					if (!m_bFingerIsDown[j])
					{
						break;
					}
				}

				m_pDevice->ReportHandler (TouchScreenEventFingerDown, j, x, y);

				m_bFingerIsDown[j] = TRUE;
				m_ucContactID[j] = ucContactID;
			}
			else if (   x != m_usLastX[j]
				 || y != m_usLastY[j])
			{
				// known, and position changed

				m_pDevice->ReportHandler (TouchScreenEventFingerMove, j, x, y);
			}

			m_usLastX[j] = x;
			m_usLastY[j] = y;
		}
		else
		{
			ucDownIDs[i] = 255;
		}
	}

	for (unsigned i = 0; i < m_Report.MaxContactCountActual; i++)
	{
		if (m_bFingerIsDown[i])
		{
			// is this Contact ID still known?
			unsigned j;
			for (j = 0; j < nContactCount; j++)
			{
				if (m_ucContactID[i] == ucDownIDs[j])
				{
					break;
				}
			}

			if (j == nContactCount)
			{
				// not known any more

				m_pDevice->ReportHandler (TouchScreenEventFingerUp, i, 0, 0);

				m_bFingerIsDown[i] = FALSE;
			}
		}
	}
}

boolean CUSBTouchScreenDevice::DecodeReportDescriptor (const u8 *pDesc, unsigned nDescSize)
{
	enum
	{
		StateStart,
		StateTouchScreen,
		StateFinger,
		StateUnknown
	}
	State = StateStart;

	u32 nUsagePage = 0;
	u32 nUsage = 0;
	u32 nReportID = 0;
	u32 nReportCount = 0;
	u32 nReportSize = 0;
	u32 nBitOffset = 0;
	unsigned nFinger = 0;

	while (nDescSize)
	{
		u8 ucItem = *pDesc++;
		nDescSize--;

		u32 nArg;

		switch (ucItem & 0x03)
		{
		case 0:
			nArg = 0;
			break;

		case 1:
			nArg = *pDesc++;
			nDescSize--;
			break;

		case 2:
			nArg  = *pDesc++;
			nArg |= *pDesc++ << 8;
			nDescSize -= 2;
			break;

		case 3:
			nArg  = *pDesc++;
			nArg |= *pDesc++ << 8;
			nArg |= *pDesc++ << 16;
			nArg |= *pDesc++ << 24;
			nDescSize -= 4;
			break;
		}

		ucItem &= 0xFC;

		switch (ucItem)
		{
		case HID_USAGE_PAGE:	nUsagePage = nArg;	break;
		case HID_USAGE:		nUsage = nArg;		break;
		case HID_REPORT_SIZE:	nReportSize = nArg;	break;
		case HID_REPORT_ID:	nReportID = nArg;	break;
		case HID_REPORT_COUNT:	nReportCount = nArg;	break;
		}

		switch (State)
		{
		case StateStart:
			if (   ucItem == HID_COLLECTION
			    && nArg == HID_COLLECTION_APPLICATION
			    && nUsagePage == HID_USAGE_PAGE_DIGITIZER
			    && nUsage == HID_USAGE_TOUCH_SCREEN)
			{
				State = StateTouchScreen;
			}
			break;

		case StateTouchScreen:
			switch (ucItem)
			{
			case HID_INPUT:
				if (   (nArg & 0x03) == 0x02
				    && nUsagePage == HID_USAGE_PAGE_DIGITIZER
				    && nUsage == HID_USAGE_CONTACT_COUNT)
				{
					if (nReportCount != 1) return FALSE;
					m_Report.ContactCount.BitOffset = nBitOffset;
					m_Report.ContactCount.BitSize = nReportSize;
				}
				break;

			case HID_COLLECTION:
				if (   nArg == HID_COLLECTION_LOGICAL
				    && nUsagePage == HID_USAGE_PAGE_DIGITIZER
				    && nUsage == HID_USAGE_FINGER)
				{
					if (m_Report.ReportID == 0)
					{
						m_Report.ReportID = nReportID;
						nBitOffset += 8;
					}

					State = StateFinger;
				}
				break;

			case HID_END_COLLECTION:
				State = StateStart;
				break;
			}
			break;

#define DO_INPUT(var)	if (nReportCount != 1) return FALSE;	\
			if (nFinger < MaxContactCount)		\
			{					\
				var.BitOffset = nBitOffset;	\
				var.BitSize = nReportSize;	\
			}

		case StateFinger:
			switch (ucItem)
			{
			case HID_INPUT:
				if ((nArg & 0x03) == 0x02)
				{
					switch (nUsagePage)
					{
					case HID_USAGE_PAGE_GENERIC_DESKTOP_CTRLS:
						switch (nUsage)
						{
						case HID_USAGE_X:
							DO_INPUT (m_Report.Finger[nFinger].X);
							break;

						case HID_USAGE_Y:
							DO_INPUT (m_Report.Finger[nFinger].Y);
							break;
						}
						break;

					case HID_USAGE_PAGE_DIGITIZER:
						switch (nUsage)
						{
						case HID_USAGE_TIP_PRESSURE:
							DO_INPUT (m_Report.Finger[nFinger].TipPressure);
							break;

						case HID_USAGE_TIP_SWITCH:
							DO_INPUT (m_Report.Finger[nFinger].TipSwitch);
							break;

						case HID_USAGE_CONTACT_IDENTIFIER:
							DO_INPUT (m_Report.Finger[nFinger].ContactIdentifier);
							break;
						}
						break;
					}
				}
				break;

			case HID_COLLECTION:
				return FALSE;

			case HID_END_COLLECTION:
				if (nFinger < MaxContactCount)
				{
					m_Report.MaxContactCountActual++;
					nFinger++;
				}
				State = StateTouchScreen;
				break;
			}
			break;

		default:
			assert (0);
			break;
		}

		if (ucItem == HID_INPUT)
		{
			nBitOffset += nReportCount * nReportSize;
		}
	}

	m_Report.ByteSize = (nBitOffset + 7) / 8;

	if (m_Report.MaxContactCountActual > 1)		// multi-touch:
	{
		if (!m_Report.ContactCount.BitSize)	// requires Contact Count
		{
			return FALSE;
		}
							// and Contact ID
		for (unsigned i = 0; i < m_Report.MaxContactCountActual; i++)
		{
			if (!m_Report.Finger[i].ContactIdentifier.BitSize)
			{
				return FALSE;
			}
		}
	}

	return State == StateStart && m_Report.MaxContactCountActual > 0;
}

u32 CUSBTouchScreenDevice::GetValue (const void *pBuffer, const TReportItem &Item, u32 nDefault)
{
	assert (pBuffer != 0);
	const u8 *pBits = (const u8 *) pBuffer;

	unsigned nBitSize = Item.BitSize;
	assert (nBitSize <= 32);
	if (nBitSize == 0)
	{
		return nDefault;
	}

	unsigned nBitOffset = Item.BitOffset;
	pBits += nBitOffset / 8;
	u32 nValue = *(u32 *) pBits;
	nBitOffset = nBitOffset % 8;

#define MASKOUT(num, off, bits)	(((num) >> (off)) & ((1 << (bits)) - 1))

	if (nBitSize <= 24)
	{
		return MASKOUT (nValue, nBitOffset, nBitSize);
	}

	u32 nLowBits = MASKOUT (nValue, nBitOffset, 24);

	pBits += 3;
	nValue = *(u32 *) pBits;
	nBitSize -= 24;

	return nLowBits | MASKOUT (nValue, nBitOffset, nBitSize) << 24;
}
