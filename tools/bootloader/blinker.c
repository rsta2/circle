
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

#include "BCM2835.h" /* Original B,A,A+,B+ */

extern void PUT32 ( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );
extern void dummy ( unsigned int );

#define ARM_TIMER_LOD   (PBASE+0x0000B400)
#define ARM_TIMER_VAL   (PBASE+0x0000B404)
#define ARM_TIMER_CTL   (PBASE+0x0000B408)
#define ARM_TIMER_DIV   (PBASE+0x0000B41C)
#define ARM_TIMER_CNT   (PBASE+0x0000B420)

#define SYSTIMERCLO     (PBASE+0x00003004)
#define GPFSEL1         (PBASE+0x00200004)
#define GPSET0          (PBASE+0x0020001C)
#define GPCLR0          (PBASE+0x00200028)

#define TIMEOUT 1000000

//-------------------------------------------------------------------------
int notmain ( void )
{
    unsigned int ra;
    unsigned int rb;

    ra=GET32(GPFSEL1);
    ra&=~(7<<18);
    ra|=1<<18;
    PUT32(GPFSEL1,ra);

    PUT32(ARM_TIMER_CTL,0x00F90000);
    PUT32(ARM_TIMER_CTL,0x00F90200);

    rb=GET32(ARM_TIMER_CNT);
    while(1)
    {
        PUT32(GPSET0,1<<16);
        while(1)
        {
            ra=GET32(ARM_TIMER_CNT);
            if((ra-rb)>=TIMEOUT) break;
        }
        rb+=TIMEOUT;
        PUT32(GPCLR0,1<<16);
        while(1)
        {
            ra=GET32(ARM_TIMER_CNT);
            if((ra-rb)>=TIMEOUT) break;
        }
        rb+=TIMEOUT;
    }
    return(0);
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
