
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

#if defined (RPI4)
#include "BCM2711.h" /* Raspberry Pi 4 */
#elif defined (RPI5)
#include "BCM2712.h" /* Raspberry Pi 5 */
#elif defined (RPI2) || AARCH == 64
#include "BCM2836.h" /* Raspberry Pi 2 */
#else
#include "BCM2835.h" /* Original B,A,A+,B+ */
#endif

extern void PUT32 ( unsigned long, unsigned int );
extern void PUT16 ( unsigned long, unsigned int );
extern void PUT8 ( unsigned long, unsigned int );
extern unsigned int GET32 ( unsigned long );
extern void dummy ( unsigned int );

#ifndef RPI5

#define ARM_SYSTIMER_CLO (PBASE + 0x3004)

#define GPFSEL1         (PBASE+0x00200004)
#define GPSET0          (PBASE+0x0020001C)
#define GPCLR0          (PBASE+0x00200028)
#define GPPUD           (PBASE+0x00200094)
#define GPPUDCLK0       (PBASE+0x00200098)

#define ARM_UART0_BASE  (PBASE + 0x201000)

#else

#define ARM_UART0_BASE	(PBASE + 0x1001000)

#endif

#define ARM_UART0_DR	(ARM_UART0_BASE + 0x00)
#define ARM_UART0_FR    (ARM_UART0_BASE + 0x18)
#define ARM_UART0_IBRD  (ARM_UART0_BASE + 0x24)
#define ARM_UART0_FBRD  (ARM_UART0_BASE + 0x28)
#define ARM_UART0_LCRH  (ARM_UART0_BASE + 0x2C)
#define ARM_UART0_CR    (ARM_UART0_BASE + 0x30)
#define ARM_UART0_IFLS  (ARM_UART0_BASE + 0x34)
#define ARM_UART0_IMSC  (ARM_UART0_BASE + 0x38)
#define ARM_UART0_RIS   (ARM_UART0_BASE + 0x3C)
#define ARM_UART0_MIS   (ARM_UART0_BASE + 0x40)
#define ARM_UART0_ICR   (ARM_UART0_BASE + 0x44)

//------------------------------------------------------------------------
#ifdef RPI4
#define ARM_GPIO_GPPUPPDN0  (PBASE+0x002000E4)

#define ARM_LOCAL_PRESCALER (LBASE + 0x008)
#endif

//------------------------------------------------------------------------
#ifndef RPI5
#define MAILBOX_BASE		(PBASE + 0xB880)
#else
#define MAILBOX_BASE		(PBASE + 0x13880)
#endif

#define MAILBOX0_READ  		(MAILBOX_BASE + 0x00)
#define MAILBOX0_STATUS 	(MAILBOX_BASE + 0x18)
	#define MAILBOX_STATUS_EMPTY	0x40000000
#define MAILBOX1_WRITE		(MAILBOX_BASE + 0x20)
#define MAILBOX1_STATUS 	(MAILBOX_BASE + 0x38)
	#define MAILBOX_STATUS_FULL	0x80000000

#define BCM_MAILBOX_PROP_OUT	8

#define CODE_REQUEST		0x00000000
#define CODE_RESPONSE_SUCCESS	0x80000000
#define CODE_RESPONSE_FAILURE	0x80000001

#define PROPTAG_GET_CLOCK_RATE	0x00030002
#define PROPTAG_GET_CLOCK_RATE_MEASURED 0x00030047
#define PROPTAG_END		0x00000000

#define CLOCK_ID_UART		2

unsigned get_clock (unsigned nClockID);
unsigned div (unsigned nDividend, unsigned nDivisor);
//------------------------------------------------------------------------
unsigned int uart_lcr ( void )
{
    return(GET32(ARM_UART0_LCRH));
}
//------------------------------------------------------------------------
unsigned int uart_recv ( void )
{
    while(1)
    {
        if(!(GET32(ARM_UART0_FR)&0x10)) break;
    }
    return(GET32(ARM_UART0_DR)&0xFF);
}
//------------------------------------------------------------------------
unsigned int uart_check ( void )
{
    if(!(GET32(ARM_UART0_FR)&0x10)) return(1);
    return(0);
}
//------------------------------------------------------------------------
void uart_send ( unsigned int c )
{
    while(1)
    {
        if(!(GET32(ARM_UART0_FR)&0x20)) break;
    }
    PUT32(ARM_UART0_DR,c);
}
//------------------------------------------------------------------------
void uart_flush ( void )
{
    while(1)
    {
        if((GET32(ARM_UART0_FR)&0x08)==0) break;
    }
}
//------------------------------------------------------------------------
void hexstrings ( unsigned int d )
{
    //unsigned int ra;
    unsigned int rb;
    unsigned int rc;

    rb=32;
    while(1)
    {
        rb-=4;
        rc=(d>>rb)&0xF;
        if(rc>9) rc+=0x37; else rc+=0x30;
        uart_send(rc);
        if(rb==0) break;
    }
    uart_send(0x20);
}
//------------------------------------------------------------------------
void hexstring ( unsigned int d )
{
    hexstrings(d);
    uart_send(0x0D);
    uart_send(0x0A);
}
//------------------------------------------------------------------------
void uart_init ( void )
{
#ifndef RPI5
    unsigned int ra;

    ra=GET32(GPFSEL1);
    ra&=~(7<<12); //gpio14
    ra|=4<<12;    //alt0
    ra&=~(7<<15); //gpio15
    ra|=4<<15;    //alt0
    PUT32(GPFSEL1,ra);
#ifndef RPI4
    PUT32(GPPUD,0);     // no pull
    for(ra=0;ra<150;ra++) dummy(ra);
    PUT32(GPPUDCLK0,1<<14);
    for(ra=0;ra<150;ra++) dummy(ra);
    PUT32(GPPUDCLK0,0);
    PUT32(GPPUD,2);     // pull up
    for(ra=0;ra<150;ra++) dummy(ra);
    PUT32(GPPUDCLK0,1<<15);
    for(ra=0;ra<150;ra++) dummy(ra);
    PUT32(GPPUDCLK0,0);
#else
    ra=GET32(ARM_GPIO_GPPUPPDN0);
    ra &= ~(3 << (14 * 2)); // no pull
    ra &= ~(3 << (15 * 2));
    ra |= 1 << (15 * 2);    // pull up
    PUT32(ARM_GPIO_GPPUPPDN0, ra);
#endif
#endif
    unsigned nBaudrate = DEFAULTBAUD;
    unsigned nClockRate = get_clock(CLOCK_ID_UART);
    unsigned nBaud16 = nBaudrate * 16;
    unsigned nIntDiv = nClockRate / nBaud16;
    unsigned nFractDiv2 = (nClockRate % nBaud16) * 8 / nBaudrate;
    unsigned nFractDiv = nFractDiv2 / 2 + nFractDiv2 % 2;

    PUT32(ARM_UART0_IMSC, 0);
    PUT32(ARM_UART0_ICR, 0x7FF);
    PUT32(ARM_UART0_IBRD, nIntDiv);
    PUT32(ARM_UART0_FBRD, nFractDiv);
    PUT32(ARM_UART0_LCRH, (1 << 4) | (3 << 5));
    PUT32(ARM_UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}
//------------------------------------------------------------------------
void  timer_init ( void )
{
#if defined(RPI4) && AARCH == 32
    PUT32(ARM_LOCAL_PRESCALER, 39768216U);      // 1 MHz clock
#endif
}
//-------------------------------------------------------------------------
unsigned int timer_tick ( void )
{
#ifndef RPI5
    return(GET32(ARM_SYSTIMER_CLO));
#else
    asm volatile ("isb" ::: "memory");

    unsigned long nCNTPCT;
    asm volatile ("mrs %0, CNTPCT_EL0" : "=r" (nCNTPCT));
    unsigned long nCNTFRQ;
    asm volatile ("mrs %0, CNTFRQ_EL0" : "=r" (nCNTFRQ));

    return (int) (nCNTPCT * 1000000 / nCNTFRQ);
#endif
}
//-------------------------------------------------------------------------
unsigned mbox_writeread (unsigned nData)
{
#if defined(RPI2) || defined(RPI4) || defined(RPI5) || AARCH == 64
	nData |= 0xC0000000U;		// convert to bus address
#else
	nData |= 0x40000000U;
#endif

	while (GET32 (MAILBOX1_STATUS) & MAILBOX_STATUS_FULL)
	{
		// do nothing
	}

	PUT32 (MAILBOX1_WRITE, BCM_MAILBOX_PROP_OUT | nData);

	unsigned nResult;
	do
	{
		while (GET32 (MAILBOX0_STATUS) & MAILBOX_STATUS_EMPTY)
		{
			// do nothing
		}

		nResult = GET32 (MAILBOX0_READ);
	}
	while ((nResult & 0xF) != BCM_MAILBOX_PROP_OUT);

	return nResult & ~0xF;
}
//-------------------------------------------------------------------------
unsigned get_clock (unsigned nClockID)
{
	// does not work without a short delay with newer firmware on RPi 1
	for (volatile unsigned i = 0; i < 100000; i++);

	unsigned proptag[] __attribute__ ((aligned (16))) =
	{
		8*4,
		CODE_REQUEST,
		PROPTAG_GET_CLOCK_RATE,
		4*4,
		1*4,
		nClockID,
		0,
		PROPTAG_END
	};

	mbox_writeread ((unsigned) (unsigned long) &proptag);

	if (proptag[6] != 0)
	{
		return proptag[6];
	}

	unsigned proptag_measured[] __attribute__ ((aligned (16))) =
	{
		8*4,
		CODE_REQUEST,
		PROPTAG_GET_CLOCK_RATE_MEASURED,
		4*4,
		1*4,
		nClockID,
		0,
		PROPTAG_END
	};

	mbox_writeread ((unsigned) (unsigned long) &proptag_measured);

	if (proptag[6] != 0)
	{
		return proptag[6];
	}

	return nClockID == CLOCK_ID_UART ? 48000000U : 0;      // default value
}
//-------------------------------------------------------------------------
unsigned div (unsigned nDividend, unsigned nDivisor)
{
	if (nDivisor == 0)
	{
		return 0;
	}

	unsigned long long ullDivisor = nDivisor;

	unsigned nCount = 1;
	while (nDividend > ullDivisor)
	{
		ullDivisor <<= 1;
		nCount++;
	}

	unsigned nQuotient = 0;
	while (nCount--)
	{
		nQuotient <<= 1;

		if (nDividend >= ullDivisor)
		{
			nQuotient |= 1;
			nDividend -= ullDivisor;
		}

		ullDivisor >>= 1;
	}

	return nQuotient;
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
//
// Copyright (c) 2012 David Welch dwelch@dwelch.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------
