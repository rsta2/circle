//
// kernel.cpp
// Test for USB Mass Storage Gadget
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2023-2024  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include <assert.h>

LOGMODULE ("kernel");

//#define MEM_STORAGE
#define MEM_STORAGE_BLOCKS 16000

#ifdef MEM_STORAGE
 #define DEVICE_DESC "Memory"
#else
 #define DEVICE_DESC "EMMC"
#endif



CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_MSDGadget (&m_Interrupt,nullptr),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
	m_MemStorage (MEM_STORAGE_BLOCKS*BLOCK_SIZE),
	m_pStgDevice (nullptr) 
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;
	int failVal=0;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
		if(!bOK)failVal=1;
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
		if(!bOK)failVal=2;
	}

	if (bOK)
	{
		// CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		// if (pTarget == 0)
		// {
		// 	pTarget = &m_Screen;
		// }

		// //bOK = m_Logger.Initialize (pTarget);
		// //if(!bOK)failVal=2;
		// m_Logger.Initialize (pTarget);
		m_Logger.Initialize(&m_Serial);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
		if(!bOK)failVal=3;
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
		if(!bOK)failVal=4;
	}

#ifndef MEM_STORAGE	

	if (bOK)
	{
		bOK = m_EMMC.Initialize ();
		if(!bOK)failVal=5;
		m_pStgDevice = &m_EMMC;
	}
#else
      m_pStgDevice = &m_MemStorage;
#endif
    if(bOK){
		bOK=m_MSDGadget.Initialize();
		if(!bOK)failVal=6;
	}

    if(bOK){
        m_ActLED.Blink(2,200,500);
	} else {
        m_ActLED.Blink(3+failVal,100,200);
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	LOGNOTE ("Compile time: " __DATE__ " " __TIME__);


    m_MSDGadget.SetDevice(m_pStgDevice);
	LOGNOTE ("cmdline deviceblocks = %s ", m_Options.GetAppOptionString("deviceblocks","[not specified]") );
	u64 numBlocks=m_Options.GetAppOptionDecimal("deviceblocks",0);//MEM_STORAGE_BLOCKS);
	if(numBlocks>0){
		LOGNOTE ("overriding device size");
		m_MSDGadget.SetDeviceBlocks(numBlocks);	
	}
	LOGNOTE ("%s device, number of blocks = %llu ", DEVICE_DESC, m_MSDGadget.GetBlocks() );
	unsigned count = m_Timer.GetTicks();
	while (1)
	{
		if (m_MSDGadget.UpdatePlugAndPlay ())
		{
          
		}
		m_MSDGadget.Update();//IO
		//CTimer::Get ()->MsDelay (25);
		if(m_Timer.GetTicks() > count+200){
		  m_ActLED.Blink(1,200,200);
		  count =m_Timer.GetTicks();
		}
	}

	return ShutdownHalt;
}

void CKernel::DeviceRemovedHandler (CDevice *pDevice, void *pContext)
{
	CKernel *pThis = static_cast<CKernel *> (pContext);
	assert (pThis);

	LOGDBG ("Device has been detached");
}
