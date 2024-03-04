//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2023  R. Stange <rsta2@o2online.de>
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
#include <circle/machineinfo.h>
#include <circle/cputhrottle.h>

CActLED m_ActLED;

#define	PIGPIO_DATA 17
static CGPIOPin IO_DAT;

CKernel::CKernel (void) :
    mScreen (mOptions.GetWidth (), mOptions.GetHeight ()),
    mTimer (&mInterrupt),
    mLogger (mOptions.GetLogLevel (), &mTimer)
{
    if (mScreen.Initialize() == true)
    {
	mLogger.Initialize(&mScreen);
    }
    else if (mSerial.Initialize(115200) == true)
    {
	// as HDMI init fails in my setup, logging is needed via serial
    	mLogger.Initialize(&mSerial);
    }
    IO_DAT.AssignPin(PIGPIO_DATA); IO_DAT.SetMode(GPIOModeInput, true);
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
    boolean bOK;
    bOK = mInterrupt.Initialize();
    if (bOK) bOK = mTimer.Initialize();
    return bOK;
}

#define log(...) mLogger.Write("", LogNotice, __VA_ARGS__)

void CKernel::exec_test(int loopcount, const char *name, fn_ptr fn) 
{
    unsigned cta, ctb;

    DisableIRQs();
    ctb = CTimer::GetClockTicks();
    for (int i = 0; i < loopcount; i++)
    {
	(*fn)();
    }
    cta = CTimer::GetClockTicks();
    EnableIRQs();
    log ("%s,\t%d loops:\t%03dus,\t%.3fus avg.",
	 name, loopcount, (cta - ctb), (float)(cta - ctb) / (float)loopcount);
    CTimer::SimpleMsDelay (20);
}

TShutdownMode CKernel::Run (void)
{
    CMachineInfo *mi = CMachineInfo::Get();
    CCPUThrottle CPUThrottle;
    
    log("timing of GPIOs on %s", mi->GetMachineName());
    CPUThrottle.Update();
    if (CPUThrottle.SetSpeed(CPUSpeedMaximum, true) != CPUSpeedUnknown)
    {	
	log("maxed freq to %dMHz", CPUThrottle.GetClockRate() / 1000000L);
    }
    log("Temp %dC (max=%dC), dynamic adaption%spossible, current freq = %dMHz (max=%dMHz)", 
			CPUThrottle.GetTemperature(),
			CPUThrottle.GetMaxTemperature(), 
			CPUThrottle.IsDynamic() ? " " : " not ",
			CPUThrottle.GetClockRate() / 1000000L,
			CPUThrottle.GetMaxClockRate() / 1000000L);
    for (unsigned i = 1; i <= 10; i++)
    {
	log("loop %d -------------------------------------------------", i);
	exec_test(1000, "LED on      ", []()->void { m_ActLED.On(); });
	exec_test(1000, "LED off     ", []()->void { m_ActLED.Off(); });
	exec_test(1000, "SetMode(Out)", []()->void { IO_DAT.SetMode(GPIOModeOutput, false); });
	exec_test(1000, "Write()     ", []()->void { IO_DAT.Write(HIGH); });
	exec_test(1000, "SetMode(In) ", []()->void { IO_DAT.SetMode(GPIOModeInput, false); });
	exec_test(1000, "Read()      ", []()->void { (void)IO_DAT.Read(); });
	exec_test(1000, "all fns     ", []()->void {
	    m_ActLED.On();
	    IO_DAT.SetMode(GPIOModeOutput, false);
	    IO_DAT.Write(HIGH);
	    IO_DAT.SetMode(GPIOModeInput, false);
	    (void)IO_DAT.Read();
	    m_ActLED.Off();
	});
	CTimer::SimpleMsDelay (1000);
    }
    
    return ShutdownReboot;
}
