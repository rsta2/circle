//
// gpiopin2712.h
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
#ifndef _circle_gpiopin2712_h
#define _circle_gpiopin2712_h

#ifndef _circle_gpiopin_h
	#error Do not include this header file directly!
#endif

#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/types.h>

enum TGPIODriveStrength
{
	GPIODriveStrength2mA,
	GPIODriveStrength4mA,
	GPIODriveStrength8mA,
	GPIODriveStrength12mA,
	GPIODriveStrengthUnknown
};

enum TGPIOSlewRate
{
	GPIOSlewRateSlow,
	GPIOSlewRateLimited	= GPIOSlewRateSlow,
	GPIOSlewRateFast,
	GPIOSlewRateNotLimited	= GPIOSlewRateFast,
	GPIOSlewRateUnknown
};

class CGPIOManager;

class CGPIOPin
{
public:
	CGPIOPin (void);
	CGPIOPin (unsigned nPin, TGPIOMode Mode, CGPIOManager *pManager = nullptr);
	~CGPIOPin (void);

	void AssignPin (unsigned nPin);

	void SetMode (TGPIOMode Mode, boolean bInitPin = TRUE);

	void SetPullMode (TGPIOPullMode Mode);

	void SetSchmittTrigger (boolean bEnable);
	void SetFilterConstant (unsigned nFilterConstant);		// 127 max.
	void SetDriveStrength (TGPIODriveStrength DriveStrength);
	void SetSlewRate (TGPIOSlewRate SlewRate);

	void Write (unsigned nValue);

	unsigned Read (void) const;

	void Invert (void);

	void ConnectInterrupt (TGPIOInterruptHandler *pHandler, void *pParam,
			       boolean bAutoAck = TRUE);
	void DisconnectInterrupt (void);

	void EnableInterrupt (TGPIOInterrupt Interrupt);
	void DisableInterrupt (void);

	void EnableInterrupt2 (TGPIOInterrupt Interrupt);
	void DisableInterrupt2 (void);

	void AcknowledgeInterrupt (void);

	static void SetModeAll (u32 nInputMask, u32 nOutputMask);
	static void WriteAll (u32 nValue, u32 nMask);
	static u32 ReadAll (void);

#ifndef NDEBUG
	static void DumpStatus (void);
#endif

private:
	void SetAlternateFunction (unsigned nFunction);

	void InterruptHandler (void);
	static void DisableAllInterrupts (unsigned nPin);
	friend class CGPIOManager;

	static void Pin2Bank (unsigned nPin, unsigned *pBank, unsigned *pBankPin);

private:
	unsigned  m_nPin;
	unsigned  m_nBank;
	unsigned  m_nBankPin;
	TGPIOMode m_Mode;

	CGPIOManager		*m_pManager;
	TGPIOInterruptHandler	*m_pHandler;
	void			*m_pParam;
	boolean			 m_bAutoAck;
	TGPIOInterrupt		 m_Interrupt;
	TGPIOInterrupt		 m_Interrupt2;

	static u32 s_nInterruptMap[GPIOInterruptUnknown];

	CSpinLock m_SpinLock;

	static CSpinLock s_SpinLock;
};

#endif
