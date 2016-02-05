/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          I8255.h                        **/
/**                                                         **/
/** This file contains emulation for the i8255 parallel     **/
/** port interface (PPI) chip from Intel. See I8255.h for   **/
/** the actual code.                                        **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2001-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef I8255_H
#define I8255_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

/** I8255 ****************************************************/
/** This data structure stores i8255 state and port values. **/
/*************************************************************/
typedef struct
{
  byte R[4];         /* Registers    */
  byte Rout[3];      /* Output ports */
  byte Rin[3];       /* Input ports  */
} I8255;

/** Reset8255 ************************************************/
/** Reset the i8255 chip. Set all data to 0x00. Set all     **/
/** ports to "input" mode.                                  **/
/*************************************************************/
void Reset8255(register I8255 *D);

/** Write8255 ************************************************/
/** Write value V into i8255 register A. Returns 0 when A   **/
/** is out of range, 1 otherwise.                           **/
/*************************************************************/
byte Write8255(register I8255 *D,register byte A,register byte V);

/** Read8255 *************************************************/
/** Read value from an i8255 register A. Returns 0 when A   **/
/** is out of range.                                        **/
/*************************************************************/
byte Read8255(register I8255 *D,register byte A);

#ifdef __cplusplus
}
#endif
#endif /* I8255_H */
