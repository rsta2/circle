/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         I8251.c                         **/
/**                                                         **/
/** This file contains emulation for the Intel 8251 UART    **/
/** chip and the 8253 timer chip implementing a generic     **/
/** RS232 interface.                                        **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2004-2015                 **/
/**               Maarten ter Huurne 2000                   **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "I8251.h"

/** Reset8251 ************************************************/
/** Reset 8251 chip, assigning In and Out to the input and  **/
/** output streams.                                         **/
/*************************************************************/
void Reset8251(register I8251 *D,FILE *In,FILE *Out)
{
  D->IRQMask = 0x0F;  /* All interrupts on */
  D->IRQs    = 0x00;  /* No interrupts yet */
  D->Mode    = 1;     /* Setting mode next */
  D->Flow    = 0x00;  /* Flow control off  */
  D->NextChr = -1;    /* No data yet       */

  /* Assign input and output streams */
  D->In      = In? In:stdin;
  D->Out     = Out? Out:stdout;
}

/** Rd8251 ***************************************************/
/** Read a byte from a given 8251 register. All values of R **/
/** will be truncated to 3 bits as there are only 8 regs.   **/
/*************************************************************/
byte Rd8251(register I8251 *D,register byte R)
{
  register int J;

  /* We only have 8 addressable ports */
  R&=0x07;

  switch(R)
  {
    case 0: /* Data */
      if(D->Flow)
      {
        if(D->NextChr<0) D->NextChr=fgetc(D->In);
        J=D->NextChr;
        D->NextChr=-1;
        return((J<0? 0xFF:J)&((0x20<<((D->Control&0x0C)>>2))-1));
      }
      return(0xFF);

    case 1: /* Status */
      if(D->NextChr<0) D->NextChr=fgetc(D->In);
      return(D->Flow&&(D->NextChr>=0)? 0x87:0x85);
      /*
      76543210
      1.......  data set ready (dsr)
      .1......  sync detect, sync only
      ..1.....  framing error (fe), async only
      ...1....  overrun error (oe)
      ....1...  parity error (pe)
      .....1..  transmitter empty
      ......1.  receiver ready
      .......1  transmitter ready
      
      D->Flow is checked first, so that stdin input doesn't block
      fMSX until it's actually being read by the emulated MSX.
      */

    case 0x82: /* Status of CTS, timer/counter 2, RI and CD */
      return(0x7F);
      /*
      76543210
      1.......  CTS (Clear To Send) 0=asserted
      .1......  timer/counter output-2 from 8253
      ..XXXX..  reserved
      ......1.  RI (Ring Indicator) 0=asserted
      .......1  CD (Carrier Detect) 0=asserted

      RI and CD are optional. If only one of them is implemented,
      it must be CD. Implemented like this:
      - CTS always returns 0 (asserted)
      - Everything else is not implemented
      */
  }

  /* Other RS232 ports:
  3: not standardised    -  unused
  4: 8253 counter 0      -  not implemented
  5: 8253 counter 1      -  not implemented
  6: 8253 counter 2      -  not implemented
  7: 8253 mode register  -  write only
  */

  return(0xFF); 
}

/** Wr8251 ***************************************************/
/** Write a byte to a given 8251 register. All values of R  **/
/** will be truncated to 3 bits as there are only 8 regs.   **/
/*************************************************************/
void Wr8251(register I8251 *D,register byte R,register byte V)
{
  /* We only have 8 addressable ports */
  R&=0x07;

  switch(R)
  {
    case 0: /* Data */
      fputc(V&((0x20<<((D->Control&0x0C)>>2))-1),D->Out);
      fflush(D->Out);
      return;
      /*
      TODO: Flush the stream at appropriate intervals, instead of
      flushing for every single character. If flushing after every
      character remains, make the stream unbuffered.
      */

    case 1: /* Command */
      if(D->Mode)
      {
        D->Control=V;
        D->Mode=0;
        /* Set mode:
        ......00  sync mode
        ......01  async, baud rate factor is 1
        ......10  async, baud rate factor is 16
        ......11  async, baud rate factor is 64
        ....00..  character length is 5 bits
        ....01..  character length is 6 bits
        ....10..  character length is 7 bits
        ....11..  character length is 8 bits
        ...1....  parity enable
        ..1.....  even/odd parity
        .1......  sync:  external sync detect (syndet) is an input/output
        1.......  sync:  single/double character sync
        00......  async: invalid
        01......  async: 1   stop bit
        10......  async: 1.5 stop bits
        11......  async: 2   stop bits
        */
      }
      else
      {
        D->Mode=V&0x40;
        D->Flow=(V>>4)&0x02;
        /* Execute command:
        76543210
        1.......  enter hunt mode (enable search for sync characters)
        .1......  internal reset
        ..1.....  request to send (rts)
        ...1....  reset error flags (pe,oe,fe)
        ....1...  send break character
        .....1..  receive enable
        ......1.  data terminal ready (dtr)
        .......1  transmit enable

        Only RESET (bit6) and DTR (bit2) are implemented.
        */
      }
      return;

    case 2: /* Interrupt mask register */
      D->IRQMask=V;
      return;
      /*
      76543210
      XXXX....  reserved
      ....1...  timer interrupt from 8253 channel-2 (0=enable int)
      .....1..  sync character detect/break detect (0=enable int)
      ......1.  transmit data ready (TxReady) (0=enable int)
      .......1  receive data ready (RxReady) (0=enable int)

      Initially all interrupts are disabled. RxReady interrupt must be
      implemented, the others are optional.

      However, currently no interrupt at all is implemented. The result
      is that only one character is read per VDP interrupt, so serial
      communication is 50/60 baud.
      */
  }

  /* Other RS232 ports:
  3: not standardised    -  unused
  4: 8253 counter 0      -  not implemented
  5: 8253 counter 1      -  not implemented
  6: 8253 counter 2      -  not implemented
  7: 8253 mode register  -  not implemented
  */
}
