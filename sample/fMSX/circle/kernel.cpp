//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifdef __cplusplus
extern "C" {
#endif
int printf(const char *format, ...);

#ifdef __cplusplus
 }
#endif
#include "kernel.h"
#include <circle/string.h>
#include <circle/screen.h>
#include <circle/debug.h>
#include <assert.h>

typedef char byte;
enum colorNum {
	COLOR_BLACK, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
	COLOR_RED, COLOR_BUFF, COLOR_CYAN, COLOR_MAGENTA,
	COLOR_ORANGE, COLOR_CYANBLUE, COLOR_LGREEN, COLOR_DGREEN };
CScreenDevice m_Screen(320,240);

void memset(byte *p, int length, int b);
int bpp;
static const char FromKernel[] = "kernel";
//TKeyMap spcKeyHash[0x200];
unsigned char keyMatrix[10];
CKernel *CKernel::s_pThis = 0;   
CKernel::CKernel (void)
:	m_Memory (TRUE),
	m_Timer(&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_DWHCI (&m_Interrupt, &m_Timer),
	m_ShutdownMode (ShutdownNone)	
{
	m_ActLED.Blink (5);	// show we are alive
	s_pThis = this;
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}
	// if (bOK)
	// {
		// bOK = m_Serial.Initialize (115200);
	// }
	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}
		//bOK = m_Logger.Initialize (pTarget);
		//m_Logger.Off();
	}
	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}
	if (bOK)
	{
		bOK = m_DWHCI.Initialize ();
	}                       	
	int num = 0;
#if 0	
	memset(spcKeyHash, sizeof(spcKeyHash)*0xff, 0);
	do {
		spcKeyHash[spcKeyMap[num].sym] = spcKeyMap[num];
	} while(spcKeyMap[num++].sym != 0);
	memset(spcsys.RAM, 65536, 0x0);
	memset(spcsys.VRAM, 0x2000, 0x0);
	memset(keyMatrix, 10, 0xff);
	bpp = m_Screen.GetDepth();
	InitMC6847(m_Screen.GetBuffer(), &spcsys.VRAM[0], 256,192);	
	spcsys.IPLK = 1;
	spcsys.GMODE = 0;
//	strcpy((char *)&spcsys.VRAM, "SAMSUNG ELECTRONICS");
#endif
	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	int frame = 0, ticks = 0, pticks = 0, d = 0;
	int count = 0;	
//	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
	//m_Logger.On();
	CUSBKeyboardDevice *pKeyboard = (CUSBKeyboardDevice *) m_DeviceNameService.GetDevice ("ukbd1", FALSE);
	if (pKeyboard == 0)
	{
		m_Logger.Write (FromKernel, LogError, "Keyboard not found");
	} 
	else
	{
		pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw); 		
	}

	return ShutdownHalt;
}

void CKernel::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
{
	CString Message;
	Message.Format ("Key status (modifiers %02X)", (unsigned) ucModifiers);
#if 0	
	TKeyMap *map;
	memset(keyMatrix, 10, 0xff);
	if (ucModifiers != 0)
	{
		for(int i = 0; i < 8; i++)
			if ((ucModifiers & (1 << i)) != 0)
			{
				map = &spcKeyHash[0x100 | (1 << i)];
				if (map != 0)
					keyMatrix[map->keyMatIdx] &= ~ map->keyMask;
			}
	}

	for (unsigned i = 0; i < 6; i++)
	{
		if (RawKeys[i] != 0)
		{
			map = &spcKeyHash[RawKeys[i]];
			if (map != 0)
				keyMatrix[map->keyMatIdx] &= ~ map->keyMask;
		}
	}
#endif	
//	s_pThis->m_Logger.Write (FromKernel, LogNotice, Message);
}

int printf(const char *format, ...)
{
	va_list args;
	va_start(args,format);
	CString str;
	str.FormatV(format, args);
	va_end(args);
	//m_Logger.Write (str);
	return 1;
}
void memset(byte *p, int length, int b)
{
	for (int i=0; i<length; i++)
		*p++ = b;
}