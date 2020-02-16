//
// gpiopin.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_gpiopin_h
#define _circle_gpiopin_h

#include <circle/spinlock.h>
#include <circle/types.h>

#define GPIO_PINS	54

enum TGPIOVirtualPin
{
	GPIOPinAudioLeft	= GPIO_PINS,
	GPIOPinAudioRight,
	GPIOPinUnknown
};

enum TGPIOMode
{
	GPIOModeInput,
	GPIOModeOutput,
	GPIOModeInputPullUp,
	GPIOModeInputPullDown,
	GPIOModeAlternateFunction0,
	GPIOModeAlternateFunction1,
	GPIOModeAlternateFunction2,
	GPIOModeAlternateFunction3,
	GPIOModeAlternateFunction4,
	GPIOModeAlternateFunction5,
	GPIOModeUnknown
};

enum TGPIOPullMode
{
	GPIOPullModeOff,
	GPIOPullModeDown,
	GPIOPullModeUp,
	GPIOPullModeUnknown
};

enum TGPIOInterrupt
{
	GPIOInterruptOnRisingEdge,
	GPIOInterruptOnFallingEdge,
	GPIOInterruptOnHighLevel,
	GPIOInterruptOnLowLevel,
	GPIOInterruptOnAsyncRisingEdge,
	GPIOInterruptOnAsyncFallingEdge,
	GPIOInterruptUnknown
};

typedef void TGPIOInterruptHandler (void *pParam);

class CGPIOManager;

class CGPIOPin		/// Encapsulates a GPIO pin
{
public:
	/// \brief Default constructor
	CGPIOPin (void);

	/// \param nPin Pin number, can be physical (Broadcom) number or TGPIOVirtualPin
	/// \param Mode Pin mode to be set
	/// \param pManager Is only required for using interrupts (IRQ)
	CGPIOPin (unsigned nPin, TGPIOMode Mode, CGPIOManager *pManager = 0);
	virtual ~CGPIOPin (void);

	/// \param nPin Pin number, can be physical (Broadcom) number or TGPIOVirtualPin
	/// \note To be used together with the default constructor and SetMode()
	void AssignPin (unsigned nPin);

	/// \param Mode Pin mode to be set
	/// \param bInitPin Also init pullup/down mode and output level
	void SetMode (TGPIOMode	Mode,
		      boolean	bInitPin = TRUE);
	
	/// \param Mode Pull mode to be set
	void SetPullMode (TGPIOPullMode Mode);

	/// \param nValue Value to be written to the pin (LOW or HIGH)
	void Write (unsigned nValue);
	/// \return Value read from pin (LOW or HIGH)
	unsigned Read (void) const;
#define LOW		0
#define HIGH		1

	/// \brief Write inverted value to pin
	void Invert (void);

	/// \param pHandler Interrupt handler to be called on GPIO event
	/// \param pParam Any parameter, will be handed over to the interrupt handler
	/// \param bAutoAck Automatically acknowledge GPIO event detect status?
	/// \note If bAutoAck = FALSE, must call AcknowledgeInterrupt() from interrupt handler!
	void ConnectInterrupt (TGPIOInterruptHandler *pHandler, void *pParam,
			       boolean bAutoAck = TRUE);
	void DisconnectInterrupt (void);

	/// \brief Enable interrupt on GPIO event
	void EnableInterrupt (TGPIOInterrupt Interrupt);
	void DisableInterrupt (void);

	/// \brief Enable interrupt on GPIO event (for a 2nd interrupt source use this)
	void EnableInterrupt2 (TGPIOInterrupt Interrupt);
	void DisableInterrupt2 (void);

	/// \brief Manually acknowledge GPIO event detect status from interrupt handler
	void AcknowledgeInterrupt (void);

	/// \param nValue Level of GPIO0-31 in the respective bits to be written (masked by nMask)
	/// \param nMask  Bit mask for the written value (only those GPIOs are affected, for which
	///		  the respective bit is set in nMask, the others are not touched)
	static void WriteAll (u32 nValue, u32 nMask);
	/// \return Level of GPIO0-31 in the respective bits
	static u32 ReadAll (void);

private:
	void SetAlternateFunction (unsigned nFunction);

	void InterruptHandler (void);
	static void DisableAllInterrupts (unsigned nPin);
	friend class CGPIOManager;

protected:
	unsigned  m_nPin;
	unsigned  m_nRegOffset;
	u32       m_nRegMask;
	TGPIOMode m_Mode;
	unsigned  m_nValue;

	CGPIOManager		*m_pManager;
	TGPIOInterruptHandler	*m_pHandler;
	void			*m_pParam;
	boolean			 m_bAutoAck;
	TGPIOInterrupt		 m_Interrupt;
	TGPIOInterrupt		 m_Interrupt2;

	static CSpinLock s_SpinLock;
};

#endif
