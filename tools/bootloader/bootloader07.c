
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

int nibble_from_hex(int ascii)
{
    if (ascii>='0' && ascii<='9')
        return ascii & 0x0F;
    if (ascii>='A' && ascii<='F')
        return (ascii - 7) & 0x0F;
    return -1;
}

void uart_send_hex_nibble(int rc)
{
    if(rc>9) rc+=0x37; else rc+=0x30;
    uart_send(rc);
}

void uart_send_hex_byte(int val)
{
    uart_send_hex_nibble((val >> 4) & 0x0f);
    uart_send_hex_nibble(val & 0x0f);
}

void uart_send_str(const char* psz)
{
    while (*psz)
    {
        uart_send(*psz++);
    }
}

void delay_micros(unsigned int period)
{
    unsigned int start = timer_tick();
    while (timer_tick() - start < period)
    {
    }
}

// Main receive mode state
typedef enum 
{
    recv_state_ready,
    recv_state_hex,
    recv_state_binary,
    recv_state_eof,
    recv_state_error,
} recv_state;

// State within the current ihex record
typedef enum
{
    ihex_state_none,
    ihex_state_start,
    ihex_state_addr_hi,
    ihex_state_addr_lo,
    ihex_state_rectype,
    ihex_state_seg_hi,
    ihex_state_seg_lo,
    ihex_state_write_bytes,
    ihex_state_end,
} ihex_state;

//------------------------------------------------------------------------
int notmain ( void )
{
    recv_state recvstate;
    ihex_state datastate;
    unsigned int byte_count;
    unsigned int address;
    unsigned int record_type;
    unsigned int segment;
    unsigned int sum;
    unsigned int ra;
    unsigned int start_delay = 10000;       // 10ms default

    uart_init();
    timer_init();

restart:
    recvstate = recv_state_ready;
    datastate = ihex_state_none;
    byte_count=0;
    address=0;
    record_type=0;
    segment=0;
    sum=0;

    // Send ready signal ("-F" indicates fast mode supported)
    uart_send_str("IHEX-F\r\n");

    while(1)
    {
        ra=uart_recv();

        // Restart command accepted in all states except binary state
        if (ra=='R' && recvstate != recv_state_binary)
        {
            goto restart;
        }

        switch (recvstate)
        {
            case recv_state_ready:
                if(ra==':')
                {
                    // Start a hex record
                    recvstate = recv_state_hex;
                    datastate = ihex_state_start;
                    sum = 0;
                    continue;
                }
                if (ra=='=')
                {
                    // Start a binary record
                    recvstate = recv_state_binary;
                    datastate = ihex_state_start;
                    sum = 0;
                    continue;
                }
                if(ra==0x0D || ra==0x0A)
                {
                    // Ignore whitepsace
                    continue;
                }
                if(ra==0x80)
                {
                    // The flush byte 0x80 is sent by the flasher tool on startup 
                    // to flush the bootloader out of a previously cancelled unknown
                    // state.  We can safely ignore it here.
                    continue;
                }
                // Format error
                uart_send_str("#ERR:format\r\n");
                recvstate = recv_state_error;
                continue;

            case recv_state_hex:
            {
                // Read hex byte
                int hi = nibble_from_hex(ra);
                int lo = nibble_from_hex(uart_recv()); 

                // Check valid
                if (hi < 0 || lo < 0)
                {
                    uart_send_str("#ERR:hex\r\n");
                    recvstate = recv_state_error;
                    continue;
                }

                ra = (hi << 4) | lo;
                break;
            }

            case recv_state_binary:
                // In binary record, don't need special handling
                break;

            case recv_state_eof:
                if (ra=='s')
                {
                    // Set a start delay (n hex digits, in micros)
                    start_delay = 0;
                    int nibble;
                    while ((nibble = nibble_from_hex(uart_recv())) >= 0)
                    {
                        start_delay = start_delay << 4 | nibble;
                    }
                    continue;
                }

                // Eof record received, wait for go command
                if (ra=='g' || ra=='G')
                {
                    // Send ack
                    uart_send(0x0D);
                    uart_send('-');
                    uart_send('-');
                    uart_send(0x0D);
                    uart_send(0x0A);
                    uart_send(0x0A);

                    // Delay before start
                    if (start_delay)
                        delay_micros(start_delay);

                    // Jump to loaded program
                    #if AARCH == 32
                    BRANCHTO(0x8000);
                    #else
                    BRANCHTO(0x80000);
                    #endif
                }
                continue;

            case recv_state_error:
                // Error state, ignore everything
                continue;
        }
    
        // Update checksum
        sum += ra;

        // Process data byte
        switch (datastate)
        {
            case ihex_state_none:
                uart_send_str("#ERR:internal\r\n");
                recvstate = recv_state_error;
                break;

            case ihex_state_start:
                byte_count=ra;
                datastate = ihex_state_addr_hi;
                break;

            case ihex_state_addr_hi:
                address = ra << 8;
                datastate = ihex_state_addr_lo;
                break;

            case ihex_state_addr_lo:
                address |= ra;
                datastate = ihex_state_rectype;
                break;

            case ihex_state_rectype:
                record_type = ra;
                switch (record_type)
                {
                    case 0x00:
                        datastate = ihex_state_write_bytes;
                        break;

                    case 0x02:
                    case 0x04:
                        datastate = ihex_state_seg_hi;
                        break;

                    default:
                        datastate = ihex_state_end;
                        break;
                }
                break;

            case ihex_state_write_bytes:
                PUT8(segment | address, ra);
                address++;
                byte_count--;
                if (byte_count == 0)
                    datastate = ihex_state_end;
                break;

            case ihex_state_seg_hi:
                segment = ra << 8;
                byte_count--;
                datastate = ihex_state_seg_lo;
                break;

            case ihex_state_seg_lo:
                segment |= ra;
                byte_count--;
                if (record_type == 0x04)
                    segment<<=16;
                else
                    segment<<=4;
                datastate = ihex_state_end;
                break;

            case ihex_state_end:
                if (byte_count == 0)
                {
                    // Check checksum
                    if ((sum & 0xFF) != 0)
                    {
                        uart_send_str("#ERR:checksum\r\n");
                        recvstate = recv_state_error;
                    }
                    else if (record_type == 0x01)
                    {
                        // EOF record received, can now accept go command
                        uart_send_str("#EOF:ok\r\n");
                        recvstate = recv_state_eof;
                    }
                    else
                    {
                        // Next record
                        recvstate = recv_state_ready;
                    }
                }
                else
                {
                    byte_count--;
                }
                break;
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
