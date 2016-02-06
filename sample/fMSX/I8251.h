/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         I8251.h                         **/
/**                                                         **/
/** This file contains definitions and declarations for     **/
/** routines in I8251.c.                                    **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2004-2015                 **/
/**               Maarten ter Huurne 2000                   **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef I8251_H
#define I8251_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

/** I8251 ****************************************************/
/** This data structure stores I8251 state.                 **/
/*************************************************************/
typedef struct
{
  byte Control;  /* 8251 ACIA control reg. */
  byte IRQMask;  /* RS232 interrupt mask   */
  byte IRQs;     /* RS232 interrupts       */
  byte Mode;     /* 8251 mode/cmd select   */
  byte Flow;     /* Flow control state     */
  int  NextChr;  /* Next char or -1        */
  FILE *In;      /* Input stream           */
  FILE *Out;     /* Output stream          */
} I8251;

/** Reset8251 ************************************************/
/** Reset 8251 chip, assigning In and Out to the input and  **/
/** output streams.                                         **/
/*************************************************************/
void Reset8251(register I8251 *D,FILE *In,FILE *Out);

/** Rd8251 ***************************************************/
/** Read a byte from a given 8251 register. All values of R **/
/** will be truncated to 3 bits as there are only 8 regs.   **/
/*************************************************************/
byte Rd8251(register I8251 *D,register byte R);

/** Wr8251 ***************************************************/
/** Write a byte to a given 8251 register. All values of R  **/
/** will be truncated to 3 bits as there are only 8 regs.   **/
/*************************************************************/
void Wr8251(register I8251 *D,register byte R,register byte V);

#ifdef __cplusplus
}
#endif
#endif /* I8251_H */
