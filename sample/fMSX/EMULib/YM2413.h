/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          YM2413.h                       **/
/**                                                         **/
/** This file contains emulation for the OPLL sound chip    **/
/** produced by Yamaha (also see OPL2, OPL3, OPL4 chips).   **/
/** See YM2413.h for the code.                              **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef YM2413_H
#define YM2413_H
#ifdef __cplusplus
extern "C" {
#endif

#define YM2413_BASE     3125   /* Base frequency for OPLL    */
#define YM2413_CHANNELS 9      /* 9 melodic channels         */

#define YM2413_ASYNC    0      /* Asynchronous emulation     */
#define YM2413_SYNC     1      /* Synchronous emulation mode */
#define YM2413_FLUSH    2      /* Flush buffers only         */

#define YM2413_DRUMS(D) ((D)->R[0x0E]&0x20)

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

/** YM2413 ***************************************************/
/** This data structure stores OPLL state.                  **/
/*************************************************************/
typedef struct
{
  byte R[64];                  /* OPLL register contents     */
  int Freq[YM2413_CHANNELS];   /* Frequencies (0 for off)    */
  int Volume[YM2413_CHANNELS]; /* Volumes (0..255)           */
  int First;                   /* First used Sound() channel */
  int Changed;                 /* Bitmap of changed channels */
  int PChanged;                /* Bitmap of changed patches  */
  int DChanged;                /* Bitmap of changed drums    */
  byte Sync;                   /* YM2413_SYNC/YM2413_ASYNC   */
  byte Latch;                  /* Latch for the register num */
} YM2413;

/** Reset2413() **********************************************/
/** Reset the sound chip and use sound channels from the    **/
/** one given in First.                                     **/
/*************************************************************/
void Reset2413(register YM2413 *D,int First);

/** WrCtrl2413() *********************************************/
/** Write a value V to the OPLL Control Port.               **/
/*************************************************************/
void WrCtrl2413(register YM2413 *D,register byte V);

/** WrData2413() *********************************************/
/** Write a value V to the OPLL Data Port.                  **/
/*************************************************************/
void WrData2413(register YM2413 *D,register byte V);

/** Write2413() **********************************************/
/** Call this function to output a value V into given OPLL  **/
/** register R.                                             **/
/*************************************************************/
void Write2413(register YM2413 *D,register byte R,register byte V);

/** Sync2413() ***********************************************/
/** Flush all accumulated changes by issuing Sound() calls  **/
/** and set the synchronization on/off. The second argument **/
/** should be YM2413_SYNC/YM2413_ASYNC to set/reset sync,   **/
/** or YM2413_FLUSH to leave sync mode as it is.            **/
/*************************************************************/
void Sync2413(register YM2413 *D,register byte Sync);

#ifdef __cplusplus
}
#endif
#endif /* YM2413_H */
