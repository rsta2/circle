//
// xpt2046touchscreen.cpp
//
// This has been ported to Circle from:
//
// Touchscreen library for XPT2046 Touch Controller Chip
// Copyright (c) 2015, Paul Stoffregen, paul@pjrc.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice, development funding notice, and this permission
// notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include <circle/input/xpt2046touchscreen.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/macros.h>
#include <circle/util.h>
#include <assert.h>

// XPT2046 Control byte
#define CONTROL_PD__SHIFT	0		// Power-Down Mode
	#define CONTROL_PD_POWER_DOWN	0
	#define CONTROL_PD_ADC_ON	1
	#define CONTROL_PD_REF_ON	2
	#define CONTROL_PD_BOTH_ON	3
#define CONTROL_SER_nDFR	BIT (2)		// Single-Ended / Differential Reference Select
#define CONTROL_MODE_8_BIT	BIT (3)		// 12-Bit / 8-Bit Conversion Select
#define CONTROL_A_SHIFT		4		// Channel Select
	#define CONTROL_A_POS_Y		1
	#define CONTROL_A_POS_Z1	3
	#define CONTROL_A_POS_Z2	4
	#define CONTROL_A_POS_X		5
#define CONTROL_S		BIT (7)		// Start bit (always set)

#define CONTROL(a, pd)		(CONTROL_S | (a) << CONTROL_A_SHIFT | (pd) << CONTROL_PD__SHIFT)

#define CONTROL_READ_X		CONTROL (CONTROL_A_POS_X, CONTROL_PD_ADC_ON)
#define CONTROL_READ_X_PD	CONTROL (CONTROL_A_POS_X, CONTROL_PD_POWER_DOWN)
#define CONTROL_READ_Y		CONTROL (CONTROL_A_POS_Y, CONTROL_PD_ADC_ON)
#define CONTROL_READ_Z1		CONTROL (CONTROL_A_POS_Z1, CONTROL_PD_ADC_ON)
#define CONTROL_READ_Z2		CONTROL (CONTROL_A_POS_Z2, CONTROL_PD_ADC_ON)

// Values from ADC
#define ADC_VALUE(v)		(bswap16 (v) >> 3)	// 12 bits, MSB first, starting at pos 1
#define MAX_ADC_VALUE		4095

LOGMODULE ("xpt2046");

CXPT2046TouchScreen::CXPT2046TouchScreen (CSPIMaster *pSPIMaster, unsigned nChipSelect,
					  CGPIOManager *pGPIOManager, unsigned nIRQPin)
:	m_pSPIMaster (pSPIMaster),
	m_nChipSelect (nChipSelect),
	m_IRQPin (nIRQPin, GPIOModeInputPullUp, pGPIOManager),
	m_bInterruptConnected (FALSE),
	m_nRotation (0),
	m_pDevice (nullptr),
	m_bActive (FALSE),
	m_nLastTicks (0),
	m_bFingerDown (FALSE)
{
}

CXPT2046TouchScreen::~CXPT2046TouchScreen (void)
{
	if (m_bInterruptConnected)
	{
		m_IRQPin.DisableInterrupt ();
		m_IRQPin.DisconnectInterrupt ();
	}

	delete m_pDevice;
	m_pDevice = nullptr;
}

void CXPT2046TouchScreen::SetRotation (unsigned nDegrees)
{
	assert (nDegrees < 360 && nDegrees % 90 == 0);

	m_nRotation = nDegrees;
}

boolean CXPT2046TouchScreen::Initialize (void)
{
	m_IRQPin.ConnectInterrupt (GPIOInterruptHandler, this);
	m_IRQPin.EnableInterrupt (GPIOInterruptOnFallingEdge);
	m_bInterruptConnected = TRUE;

	assert (!m_pDevice);
	m_pDevice = new CTouchScreenDevice (UpdateStub, this);
	assert (m_pDevice);

	return TRUE;
}

void CXPT2046TouchScreen::Update (void)
{
	if (!m_bActive)
	{
		return;
	}

	unsigned nTicks = CTimer::GetClockTicks ();
	if (nTicks - m_nLastTicks < ThresholdMicros * (CLOCKHZ / 1000000))
	{
		return;
	}

	static const u16 WriteBuffer[] =
	{
		CONTROL_READ_Z1,
		CONTROL_READ_Z2,
		CONTROL_READ_Y,		// result ignored (first value is always noisy)
		CONTROL_READ_Y,		// read x/y 3 times and calculate average of best 2
		CONTROL_READ_X,
		CONTROL_READ_Y,
		CONTROL_READ_X,
		CONTROL_READ_Y,
		CONTROL_READ_X_PD,	// power down
		0			// drain out
	};

	u8 ReadBuffer[sizeof WriteBuffer];

	assert (m_pSPIMaster);
	m_pSPIMaster->SetClock (SPIClockSpeed);
	m_pSPIMaster->SetMode (0, 0);

	if (m_pSPIMaster->WriteRead (m_nChipSelect, WriteBuffer, ReadBuffer,
				     sizeof WriteBuffer) != sizeof WriteBuffer)
	{
		LOGWARN ("SPI transfer failed");

		return;
	}

	s16 *p = reinterpret_cast<s16 *> (ReadBuffer + 1);	// ignore first byte

	s16 z = ADC_VALUE (p[0]) + MAX_ADC_VALUE - ADC_VALUE (p[1]);
	s16 y = BestTwoAvg (ADC_VALUE (p[3]), ADC_VALUE (p[5]), ADC_VALUE (p[7]));
	s16 x = BestTwoAvg (ADC_VALUE (p[4]), ADC_VALUE (p[6]), ADC_VALUE (p[8]));

	s16 temp;
	switch (m_nRotation)
	{
	case 0:
		x = MAX_ADC_VALUE - x;
		break;

	case 90:
		temp = x; x = y; y = temp;
		break;

	case 180:
		y = MAX_ADC_VALUE - y;
		break;

	case 270:
		temp = x; x = y; y = temp;
		x = MAX_ADC_VALUE - x;
		y = MAX_ADC_VALUE - y;
		break;

	default:
		assert (0);
		break;
	}

	if (z < Z_Threshold_Active)
	{
		if (m_bFingerDown)
		{
			m_pDevice->ReportHandler (TouchScreenEventFingerUp, 0, 0, 0);

			m_bFingerDown = FALSE;
		}

		m_bActive = FALSE;
	}
	else if (z >= Z_Threshold_Pressed)
	{
		m_nLastTicks = nTicks;

		if (!m_bFingerDown)
		{
			m_pDevice->ReportHandler (TouchScreenEventFingerDown, 0, x,  y);

			m_bFingerDown = TRUE;
		}
		else
		{
			m_pDevice->ReportHandler (TouchScreenEventFingerMove, 0, x,  y);
		}
	}
}

void CXPT2046TouchScreen::UpdateStub (void *pParam)
{
	CXPT2046TouchScreen *pThis = static_cast<CXPT2046TouchScreen *> (pParam);
	assert (pThis != 0);

	pThis->Update ();
}

void CXPT2046TouchScreen::GPIOInterruptHandler (void *pParam)
{
	CXPT2046TouchScreen *pThis = static_cast<CXPT2046TouchScreen *> (pParam);
	assert (pThis != 0);

	pThis->m_bActive = TRUE;
}

s16 CXPT2046TouchScreen::BestTwoAvg (s16 x, s16 y, s16 z)
{
	s16 da, db, dc;
	s16 reta = 0;

	if ( x > y ) da = x - y; else da = y - x;
	if ( x > z ) db = x - z; else db = z - x;
	if ( z > y ) dc = z - y; else dc = y - z;

	if ( da <= db && da <= dc ) reta = (x + y) >> 1;
	else if ( db <= da && db <= dc ) reta = (x + z) >> 1;
	else reta = (y + z) >> 1;	// else if ( dc <= da && dc <= db ) reta = (x + y) >> 1;

	return reta;
}
