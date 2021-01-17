
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

// 2  outer corner
// 4
// 6
// 8  TX out
// 10 RX in

extern void PUT32 ( unsigned int, unsigned int );
extern void PUT16 ( unsigned int, unsigned int );
extern void PUT8 ( unsigned int, unsigned int );
extern unsigned int GET32 ( unsigned int );
extern unsigned int GETPC ( void );
extern void BRANCHTO ( unsigned int );
extern void dummy ( unsigned int );

extern void uart_init ( void );
extern unsigned int uart_lcr ( void );
extern void uart_flush ( void );
extern void uart_send ( unsigned int );
extern unsigned int uart_recv ( void );
extern unsigned int uart_check ( void );
extern void hexstring ( unsigned int );
extern void hexstrings ( unsigned int );
extern void timer_init ( void );
extern unsigned int timer_tick ( void );

extern void timer_init ( void );
extern unsigned int timer_tick ( void );

int fast_state;
int fast_bytes_recv;
int fast_bytes_done;
unsigned char fast_pending_byte;
unsigned char fast_byte_buffer[256];

// Convert a byte value to a hex nibble character
char to_nibble(int val)
{
    val = (val & 0x0f) + 0x30;
    if (val > 0x39)
        val += 7;
    return val;
}

// Helper to decode the fast flash format. This just watches for the binary
// chunks and reformats them back to hex to for main routine.
unsigned int uart_recv2 ( void )
{
    unsigned int ra;
    if (fast_state == 0)
    {
        ra = uart_recv();
        if (ra == '=')
        {
            // Start binary chunk...

            // Receive the whole chunk before regenerating the hex
            fast_bytes_recv = uart_recv();
            fast_bytes_done = 0;
            for (int i=0; i<fast_bytes_recv; i++)
            {
                fast_byte_buffer[i] = uart_recv();
            }

            // Next byte must be a CR, LF or Ctrl+Z
            fast_pending_byte = uart_recv();
            if (fast_pending_byte != '\r' && fast_pending_byte != '\n' && fast_pending_byte != 26)
            {
                // Something bad happened, force a reset
                return 'R'; 
            }

            // Start hex generation
            fast_state = 1;
            return ':';
        }
        else
        {
            return ra;
        }
    }
    else if (fast_state == 1)
    {
        // Return first nibble of current byte
        ra = (fast_byte_buffer[fast_bytes_done] >> 4) & 0x0f;
        fast_state = 2;
    }
    else if (fast_state == 2)
    {
        // Return second nibble of current byte
        ra = fast_byte_buffer[fast_bytes_done] & 0x0f;

        // Move to next byte, or switch out of binary mode
        fast_bytes_done++;
        if (fast_bytes_done == fast_bytes_recv)
        {
            if (fast_pending_byte == 26)
                fast_state = 0;
            else
                fast_state = 3;
        }
        else
        {
            fast_state = 1;
        }
    }
    else
    {
        // Return the terminating CR or LF
        fast_state = 0;
        return fast_pending_byte;
    }

    // Regenerate hex nibble
    ra += 0x30;
    if (ra > 0x39) ra+=7;
    return ra;
}

//------------------------------------------------------------------------
int notmain ( void )
{
    unsigned int state;
    unsigned int byte_count;
    unsigned int digits_read;
    unsigned int address;
    unsigned int record_type;
    unsigned int segment;
    unsigned int data;
    unsigned int sum;
    unsigned int ra;

    fast_state = 0;

    uart_init();

    hexstring(0x12345678);
    hexstring(GETPC());

restart:
    state=0;
    byte_count=0;
    digits_read=0;
    address=0;
    record_type=0;
    segment=0;
    data=0;
    sum=0;

    uart_send('I');
    uart_send('H');
    uart_send('E');
    uart_send('X');
    uart_send('-');     // Indicates to flasher tool that fast transfer supported
    uart_send('F');
    uart_send(0x0D);
    uart_send(0x0A);

    while(1)
    {
        ra=uart_recv2();
        if(ra==':')
        {
            state=1;
            continue;
        }
        if(ra==0x0D || ra==0x0A || ra==0x00)
        {
            state=0;
            continue;
        }
        if((ra=='g')||(ra=='G'))
        {
            uart_send(0x0D);
            uart_send('-');
            uart_send('-');
            uart_send(0x0D);
            uart_send(0x0A);
            uart_send(0x0A);
            // Small delay to let the response ack be sent
        	for (volatile unsigned i = 0; i < 10000; i++);
#if AARCH == 32
            BRANCHTO(0x8000);
#else
            BRANCHTO(0x80000);
#endif
            state=0;
            break;
        }
        if (ra=='R')
        {
            goto restart;
        }
        switch(state)
        {
            case 0:
            {
                break;
            }
            case 1:
            case 2:
            {
                byte_count<<=4;
                if(ra>0x39) ra-=7;
                byte_count|=(ra&0xF);
                byte_count&=0xFF;
                digits_read=0;
                state++;
                break;
            }
            case 3:
            case 4:
            case 5:
            case 6:
            {
                address<<=4;
                if(ra>0x39) ra-=7;
                address|=(ra&0xF);
                address&=0xFFFF;
                address|=segment;
                state++;
                break;
            }
            case 7:
            {
                record_type<<=4;
                if(ra>0x39) ra-=7;
                record_type|=(ra&0xF);
                record_type&=0xFF;
                state++;
                break;
            }
            case 8:
            {
                record_type<<=4;
                if(ra>0x39) ra-=7;
                record_type|=(ra&0xF);
                record_type&=0xFF;
                switch(record_type)
                {
                    case 0x00:
                    {
                        state=14;
                        break;
                    }
                    case 0x01:
                    {
                        hexstring(sum);
                        state=0;
                        break;
                    }
                    case 0x02:
                    case 0x04:
                    {
                        state=9;
                        break;
                    }
                    default:
                    {
                        state=0;
                        break;
                    }
                }
                break;
            }
            case 9:
            case 10:
            case 11:
            case 12:
            {
                segment<<=4;
                if(ra>0x39) ra-=7;
                segment|=(ra&0xF);
                segment&=0xFFFF;
                state++;
                break;
            }
            case 13:
            {
                segment<<=4;
                if(record_type==0x04)
                {
                    segment<<=12;
                }
                state=0;
                break;
            }
            case 14:
            {
                data<<=4;
                if(ra>0x39) ra-=7;
                data|=(ra&0xF);
                if (++digits_read%8==0||digits_read==byte_count*2)
                {
                    ra=(data>>24)|(data<<24);
                    ra|=(data>>8)&0x0000FF00;
                    ra|=(data<<8)&0x00FF0000;
                    if(digits_read%8!=0)
                    {
                        ra>>=(8-digits_read%8)*4;
                    }
                    data=ra;
                    PUT32(address,data);
                    sum+=address;
                    sum+=data;
                    address+=4;
                    if(digits_read==byte_count*2)
                    {
                        state=0;
                    }
                }
                break;
            }
        }
    }
    return(0);
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
//
// Copyright (c) 2014 David Welch dwelch@dwelch.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------
