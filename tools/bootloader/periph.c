
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

#define ARM_TIMER_CTL   (PBASE+0x0000B408)
#define ARM_TIMER_CNT   (PBASE+0x0000B420)

#define GPFSEL1         (PBASE+0x00200004)
#define GPSET0          (PBASE+0x0020001C)
#define GPCLR0          (PBASE+0x00200028)
#define GPPUD           (PBASE+0x00200094)
#define GPPUDCLK0       (PBASE+0x00200098)

#define AUX_ENABLES     (PBASE+0x00215004)
#define AUX_MU_IO_REG   (PBASE+0x00215040)
#define AUX_MU_IER_REG  (PBASE+0x00215044)
#define AUX_MU_IIR_REG  (PBASE+0x00215048)
#define AUX_MU_LCR_REG  (PBASE+0x0021504C)
#define AUX_MU_MCR_REG  (PBASE+0x00215050)
#define AUX_MU_LSR_REG  (PBASE+0x00215054)
#define AUX_MU_MSR_REG  (PBASE+0x00215058)
#define AUX_MU_SCRATCH  (PBASE+0x0021505C)
#define AUX_MU_CNTL_REG (PBASE+0x00215060)
#define AUX_MU_STAT_REG (PBASE+0x00215064)
#define AUX_MU_BAUD_REG (PBASE+0x00215068)

//GPIO14  TXD0 and TXD1
//GPIO15  RXD0 and RXD1

#else

#define ARM_UART0_BASE	(PBASE + 0x1001000)

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
#define CLOCK_ID_CORE		4

unsigned get_clock (unsigned nClockID);
unsigned div (unsigned nDividend, unsigned nDivisor);
//------------------------------------------------------------------------
#ifndef RPI5
unsigned int uart_lcr ( void )
{
    return(GET32(AUX_MU_LSR_REG));
}
#endif
//------------------------------------------------------------------------
unsigned int uart_recv ( void )
{
#ifndef RPI5
    while(1)
    {
        if(GET32(AUX_MU_LSR_REG)&0x01) break;
    }
    return(GET32(AUX_MU_IO_REG)&0xFF);
#else
    while(1)
    {
        if(!(GET32(ARM_UART0_FR)&0x10)) break;
    }
    return(GET32(ARM_UART0_DR)&0xFF);
#endif
}
//------------------------------------------------------------------------
unsigned int uart_check ( void )
{
#ifndef RPI5
    if(GET32(AUX_MU_LSR_REG)&0x01) return(1);
#else
    if(!(GET32(ARM_UART0_FR)&0x10)) return(1);
#endif
    return(0);
}
//------------------------------------------------------------------------
void uart_send ( unsigned int c )
{
#ifndef RPI5
    while(1)
    {
        if(GET32(AUX_MU_LSR_REG)&0x20) break;
    }
    PUT32(AUX_MU_IO_REG,c);
#else
    while(1)
    {
        if(!(GET32(ARM_UART0_FR)&0x20)) break;
    }
    PUT32(ARM_UART0_DR,c);
#endif
}
//------------------------------------------------------------------------
#ifndef RPI5
void uart_flush ( void )
{
    while(1)
    {
        if((GET32(AUX_MU_LSR_REG)&0x100)==0) break;
    }
}
#endif
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

    PUT32(AUX_ENABLES,1);
    PUT32(AUX_MU_IER_REG,0);
    PUT32(AUX_MU_CNTL_REG,0);
    PUT32(AUX_MU_LCR_REG,3);
    PUT32(AUX_MU_MCR_REG,0);
    PUT32(AUX_MU_IER_REG,0);
    PUT32(AUX_MU_IIR_REG,0xC6);
    PUT32(AUX_MU_BAUD_REG,div(get_clock(CLOCK_ID_CORE)/8 + DEFAULTBAUD/2, DEFAULTBAUD) - 1);
    ra=GET32(GPFSEL1);
    ra&=~(7<<12); //gpio14
    ra|=2<<12;    //alt5
    ra&=~(7<<15); //gpio15
    ra|=2<<15;    //alt5
    PUT32(GPFSEL1,ra);
    PUT32(GPPUD,0);
    for(ra=0;ra<150;ra++) dummy(ra);
    PUT32(GPPUDCLK0,(1<<14)|(1<<15));
    for(ra=0;ra<150;ra++) dummy(ra);
    PUT32(GPPUDCLK0,0);
    PUT32(AUX_MU_CNTL_REG,3);
#else
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
#endif
}
//------------------------------------------------------------------------
void  timer_init ( void )
{
#ifndef RPI5
    //0xF9+1 = 250
    //250MHz/250 = 1MHz
    PUT32(ARM_TIMER_CTL,0x00F90000);
    PUT32(ARM_TIMER_CTL,0x00F90200);
#endif
}
//-------------------------------------------------------------------------
unsigned int timer_tick ( void )
{
#ifndef RPI5
    return(GET32(ARM_TIMER_CNT));
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
#ifdef RPI5
	nData |= 0xC0000000U;		// convert to bus address
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
	for (volatile unsigned i = 0; i < 10000; i++);

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

	return proptag_measured[6];
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
