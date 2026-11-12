//
// usbrpdisp.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2018-2026  R. Stange <rsta2@gmx.net>
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
// Copyright (C) 2026,  S. Spencer (SteveSpencer on Github)
//
#include <circle/usb/usbrpdisp.h>
#include <circle/usb/usbhostcontroller.h>
#include <circle/logger.h>
#include <circle/util.h>
#include <circle/devicenameservice.h>

LOGMODULE ("urpdisp");

// Level of firmware supporting compression and fillrect commands
#define FW_MIN	0x0104

// Macro to store a 16-bit pixel value in LE format at
// the specified address, ignores any alignment issues.
#define COPY_PIXEL(dst, pixel) \
	((u8*)(dst))[0] = ((pixel)>>8)&0xff; \
	((u8*)(dst))[1] = (pixel)&0xff;


CUSBRoboPeakDispDevice::CUSBRoboPeakDispDevice (CUSBFunction *pFunction)
	: CUSBFunction (pFunction),
	  CDisplay (RGB565_BE),
	  m_pEndpointIn (0),
	  m_pEndpointOut (0),
	  m_pTouch (0),
	  m_Report (0),
	  m_payload (0),
	  m_nPayloadMax (0),
	  m_bDown (FALSE),
	  m_fw (0)
{
	CDeviceNameService::Get ()->AddDevice ("rpusbdisp", this, FALSE);
}
CUSBRoboPeakDispDevice::~CUSBRoboPeakDispDevice (void)
{
	CDeviceNameService::Get ()->RemoveDevice ("rpusbdisp", FALSE);
}

boolean CUSBRoboPeakDispDevice::Configure (void)
{
	SelectInterfaceByClass (255, 193, 0);
	if (GetNumEndpoints () != 2)
	{
		ConfigurationError (From);
		return FALSE;
	}
	CUSBDevice *pDev = GetDevice ();
	if (pDev != 0)
	{
		const TUSBDeviceDescriptor *pDD = pDev->GetDeviceDescriptor ();
		if (pDD)
		{
			m_fw = pDD->bcdDevice;
		}
		const TUSBEndpointDescriptor *pEPD;
		while ((pEPD = (TUSBEndpointDescriptor*) GetDescriptor (DESCRIPTOR_ENDPOINT)) != 0)
		{
			int x = pEPD->bmAttributes & 0x3f;
			if (x == 0x02)
			{
				if ((pEPD->bEndpointAddress & 0x80) == 0)
				{
					if (m_pEndpointOut != 0)
					{
						ConfigurationError (From);
						return FALSE;
					}
					m_pEndpointOut = new CUSBEndpoint (GetDevice (), pEPD);
				}
			}
			else if (x == 0x03)
			{
				if ((pEPD->bEndpointAddress & 0x80) == 0x80)
				{
					if (m_pEndpointIn != 0)
					{
						ConfigurationError (From);
						return FALSE;
					}
					m_pEndpointIn = new CUSBEndpoint (GetDevice (), pEPD);
				}
			}
		}
	}
	// Do we have endpoints?
	if ( m_pEndpointIn == 0 || m_pEndpointOut == 0)
	{
		ConfigurationError (From);
		return FALSE;
	}
	if (!CUSBFunction::Configure ())
	{
		CLogger::Get ()->Write (From, LogError, "Cannot set interface");
		return FALSE;
	}
	m_Report = new u8[32];
	
	// Calculate size needed for RLE compression/pixel swapping
	// Worst case is we have an extra byte for each 128 pixels, so...
	size_t nPix = RPD_WIDTH*RPD_HEIGHT;
	nPix = (nPix + ((nPix/2)+127)/128);
	m_nPayloadMax = nPix*2;
	m_payload = new u8[m_nPayloadMax];
	m_pTouch = new CTouchScreenDevice ( 0, 0, this);
	assert ( m_pTouch != 0);

	// Clear screen to black to show it is working
	{
		TCmdFillRect cmd;
		cmd.bCmdFlags = 0x40|Tcmd::cmdFill;
		cmd.uX = 0;
		cmd.uY = 0;
		cmd.uH = RPD_HEIGHT;
		cmd.uW = RPD_WIDTH;
		COPY_PIXEL(&cmd.uColour, 0);
		cmd.bROP = TROP::ROP_COPY;
		sendData ((u8*)&cmd, sizeof(cmd), 0, 0);
	}
	if (MakeRequest ())
	{
		return TRUE;
	}
	return TRUE;
}

void CUSBRoboPeakDispDevice::CompletionRoutine (CUSBRequest *pURB)
{
	boolean bRestart = TRUE;
	assert (pURB != 0);
	if (pURB->GetStatus () != 0)
	{
		TStatus* t = (TStatus*)m_Report;
		if (t->nTouchStatus == 1)
		{
			if (!m_bDown)
			{
				m_bDown = TRUE;
				m_pTouch->ReportHandler (TouchScreenEventFingerDown, 0, t->sTouchX, t->sTouchY); 
			}
			else
			{
				m_pTouch->ReportHandler (TouchScreenEventFingerMove, 0, t->sTouchX, t->sTouchY);
			}
		}
		else
		{
			if (m_bDown)
			{
				m_bDown = FALSE;
				m_pTouch->ReportHandler (TouchScreenEventFingerUp, 0, 0, 0);
			}
		}
	}
	delete pURB;
	if (bRestart && !MakeRequest ())
	{
		CLogger::Get ()->Write (From, LogError, "Cannot restart request");
	}
}

void CUSBRoboPeakDispDevice::CompletionStub (CUSBRequest *pURB, void *pParam, void *pContext)
{
	CUSBRoboPeakDispDevice *pThis = (CUSBRoboPeakDispDevice *)pContext;
	assert (pThis != 0);
	pThis->CompletionRoutine (pURB);
}

boolean CUSBRoboPeakDispDevice::MakeRequest (void)
{
	CUSBRequest *pURB = new CUSBRequest (m_pEndpointIn, m_Report, 32);
	assert (pURB != 0);
	pURB->SetCompletionRoutine (CompletionStub, 0, this);
	
	return GetHost ()->SubmitAsyncRequest (pURB);
}

// MAXCMDSIZE is how large the device allows each display command
// packet to be. This cannot be changed.
#define	MAXCMDSIZE	64

// This defines how many packets we accumulate into a single USB
// request when sending pixel data. Obviously, the larger the better,
// but we don't want to cause memory issues. This is a reasonable compromise.
#define	NUMPKTS		8

// Send a block of data to the display.
void CUSBRoboPeakDispDevice::sendData( u8* pHeader, size_t sHeaderSize, const u8* pData, size_t sDataSize)
{
	assert (pHeader != 0);
	DMA_BUFFER (u8, tx, MAXCMDSIZE * NUMPKTS);
	int pktno = 0;

	// if the command and any data all fits in a single packet, this
	// simplifies things, and we don't batch into USB requests.
	if ((sHeaderSize + sDataSize) <= MAXCMDSIZE)
	{
		// Just send single packet.
		memcpy (tx, pHeader, sHeaderSize);
		tx[0] |= 0x80; // Set flag to say it's a new command
		if (sDataSize != 0)
		{
			memcpy (tx + sHeaderSize, pData, sDataSize);
		}
		int res = GetHost() ->Transfer (m_pEndpointOut, tx, sHeaderSize + sDataSize);
		if (res < 0)
		{
			CLogger::Get ()->Write (From, LogError, "I/O error");
		}
	}
	else
	{
		// We need to send at least one sub-packet, so we
		// batch them into groups for each request.

		u8* ptx = tx;
		int x;
		size_t sndSize = 0;
		size_t pktSize = sHeaderSize;
		size_t totSize = sDataSize;

		// Construct first start packet
		memcpy(tx, pHeader, sHeaderSize);
		tx[0] |= 0x80; // Flag this first one as a start packet.
		size_t bufSpace = MAXCMDSIZE - sHeaderSize;
		// We know this first one won't overrun the source data
		memcpy(tx+sHeaderSize, pData, bufSpace);
		pktSize += bufSpace;
		sndSize += pktSize;

		pData += bufSpace;
		totSize -= bufSpace;
		pktno++;
		ptx += pktSize;

		while (totSize > 0)
		{
			pktSize = 1;
			// Add next sub packet
			ptx[0] = pHeader[0] & 0x3F;
			if (totSize < (MAXCMDSIZE-1))
			{
				bufSpace = totSize;
			}
			else
			{
				bufSpace = MAXCMDSIZE-1;
			}
			memcpy(ptx+1, pData, bufSpace);
			pData += bufSpace;
			totSize -= bufSpace;
			ptx += bufSpace+1;
			pktno++;
			sndSize += bufSpace+1;
			if (pktno >= NUMPKTS)
			{
				x = GetHost() ->Transfer (m_pEndpointOut, tx, sndSize);
				if (x < 0)
				{
					CLogger::Get ()->Write (From, LogError, "I/O error");
				}	
				sndSize = 0; ptx = tx; pktno = 0;
			}
		}
		// If we have any pending (because we don't have an exact
		// multiple of NUMPKTS), then send the remainder...
		if (sndSize > 0)
		{
			x = GetHost() ->Transfer (m_pEndpointOut, tx, sndSize);
			if (x < 0)
			{
				CLogger::Get ()->Write (From, LogError, "I/O error");
			}	
		}
	}
}

void CUSBRoboPeakDispDevice::SetPixel (unsigned nPosX, unsigned nPosY, TRawColor nColor)
{
	// This is horrendously inefficient, since it's a USB request per pixel
	// Send a FillRect for a single pixel (!)
	TCmdFillRect cmd;
	cmd.bCmdFlags = 0x40|Tcmd::cmdFill;
	cmd.uX = nPosX;
	cmd.uY = nPosY;
	cmd.uH = cmd.uW = 1;
	COPY_PIXEL(&cmd.uColour, (u16)nColor);
	cmd.bROP = TROP::ROP_COPY;
	sendData ((u8*)&cmd, sizeof(cmd), 0, 0);
}

void CUSBRoboPeakDispDevice::SetArea (const TArea &rArea, const void *pPixels,
			      TAreaCompletionRoutine *pRoutine,
			      void *pParam)
{
	// Send pixel data, but must not exceed 64 bytes per individual packet
	TCmdBitBlt cmd;
	cmd.bCmdFlags = 0x40|Tcmd::cmdBitBlt;
	cmd.bROP = TROP::ROP_COPY;
	cmd.uX = rArea.x1; cmd.uW = (rArea.x2-rArea.x1)+1;
	cmd.uY = rArea.y1; cmd.uH = (rArea.y2-rArea.y1)+1;
	
	size_t nPix = (cmd.uW*cmd.uH);

	size_t cb;
	if (m_fw >= FW_MIN)
	{
		// Firmware supports RLE compression, much faster
		cb = RLEcompress ((const u16 *)pPixels, nPix);
		cmd.bCmdFlags = 0x40|Tcmd::cmdBitBltRLE;
	}
	else
	{
		// Copy the provided pixels into the right format
		const u16* src = (const u16*)pPixels;
		u16* dst = (u16 *)m_payload;
		size_t cnt = nPix;
		while (cnt--)
		{
			*dst++ = bswap16(*src++);
		}
		cb = nPix*2;
	}
	sendData ((u8*)&cmd, sizeof(cmd), m_payload, cb);

	if (pRoutine!=0)
	{
		(*pRoutine)(pParam);
	}
}

//
// RLE compress the pixel data. Output format is:
//  [session] [pixel lo] [pixel hi]
// where the pixel data is a 16-bit RGB565 value stored in LE format
// and session is encoded as:
// Top bit (0x80) set indicates a valid session
// Bottom 7 bits (0x7f) indicates the number of pixels in the run.
//
size_t CUSBRoboPeakDispDevice::RLEcompress (const u16 *pixels, size_t nPix)
{
	size_t pixelPos = 0;
	u16 lastPixelValue;
	u8* encodingPos = m_payload;
	u8* sectionHeader;
	memset(m_payload, 0, m_nPayloadMax);
	
	// Initial setup: 
	sectionHeader = encodingPos++;
	*sectionHeader = 0;
	lastPixelValue = pixels[pixelPos++];
	COPY_PIXEL(encodingPos, lastPixelValue);
	encodingPos += 2;
	
	while (pixelPos < nPix)
	{
		boolean isCommonSection = *sectionHeader & 0x80;
		if (isCommonSection)
		{
			while (pixelPos < nPix)
			{
				u16 curPix = pixels[pixelPos++];
				if (curPix == lastPixelValue)
				{
					if ((*sectionHeader) == (0x7f|0x80))
					{
						// No room, need a new section
						sectionHeader = encodingPos;
						encodingPos++;
						*sectionHeader = 0x80;
						COPY_PIXEL(encodingPos, curPix);
						encodingPos += 2;
					}
					else
					{
						// Add to this section
						++(*sectionHeader);
					}
				}
				else
				{
					// Pixel changed, start a new section
					sectionHeader = encodingPos++;
					*sectionHeader = 0;
					COPY_PIXEL(encodingPos, curPix); //*((u16*)encodingPos) = curPix;
					encodingPos += 2;
					lastPixelValue = curPix;
					break;
				}
			}
		}
		else
		{
			while (pixelPos < nPix)
			{
				u16 curPix = pixels[pixelPos++];
				if (curPix == lastPixelValue)
				{
					if (*sectionHeader == 0)
					{
						++(*sectionHeader);
						*sectionHeader |= 0x80;
					}
					else
					{
						(*sectionHeader)--;
						encodingPos -= 2;
						sectionHeader = encodingPos;
						encodingPos++;
						
						*sectionHeader = 0x80 + 1;
						COPY_PIXEL(encodingPos, curPix);
						encodingPos += 2;
					}
					break;
				}
				else
				{
					if (*sectionHeader == 0x7f)
					{
						sectionHeader = encodingPos;
						encodingPos++;
						*sectionHeader = 0;
					}
					else
					{
						++(*sectionHeader);
					}
					COPY_PIXEL(encodingPos, curPix);
					encodingPos += 2;
					lastPixelValue = curPix;
				}
			}
		}
	}
	return encodingPos - m_payload;
}

