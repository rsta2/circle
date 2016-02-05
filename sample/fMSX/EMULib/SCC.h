/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                           SCC.h                         **/
/**                                                         **/
/** This file contains definitions and declarations for     **/
/** routines in SCC.c.                                      **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef SCC_H
#define SCC_H

#define SCC_BASE     111861    /* Base frequency for SCC     */
#define SCC_CHANNELS 5         /* 5 melodic channels         */

#define SCC_ASYNC    0         /* Asynchronous emulation     */
#define SCC_SYNC     1         /* Synchronous emulation mode */
#define SCC_FLUSH    2         /* Flush buffers only         */

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

/** SCC ******************************************************/
/** This data structure stores SCC state.                   **/
/*************************************************************/
typedef struct
{
  byte R[256];                /* SCC register contents       */
  int Freq[SCC_CHANNELS];     /* Frequencies (0 for off)     */
  int Volume[SCC_CHANNELS];   /* Volumes (0..255)            */
  int First;                  /* First used Sound() channel  */
  byte Changed;               /* Bitmap of changed channels  */
  byte WChanged;              /* Bitmap of changed waveforms */
  byte Sync;                  /* SCC_SYNC/SCC_ASYNC          */
} SCC;

/** ResetSCC() ***********************************************/
/** Reset the sound chip and use sound channels from the    **/
/** one given in First.                                     **/
/*************************************************************/
void ResetSCC(register SCC *D,int First);

/** ReadSCC() ************************************************/
/** Call this function to read contents of the generic SCC  **/
/** sound chip registers.                                   **/
/*************************************************************/
byte ReadSCC(register SCC *D,register byte R);

/** ReadSCCP() ***********************************************/
/** Call this function to read contents of the newer SCC+   **/
/** sound chip registers.                                   **/
/*************************************************************/
byte ReadSCCP(register SCC *D,register byte R);
           
/** WriteSCC() ***********************************************/
/** Call this function to output a value V into the generic **/
/** SCC sound chip.                                         **/
/*************************************************************/
void WriteSCC(register SCC *D,register byte R,register byte V);

/** WriteSCCP() **********************************************/
/** Call this function to output a value V into the newer   **/
/** SCC+ sound chip.                                        **/
/*************************************************************/
void WriteSCCP(register SCC *D,register byte R,register byte V);

/** SyncSCC() ************************************************/
/** Flush all accumulated changes by issuing Sound() calls  **/
/** and set the synchronization on/off. The second argument **/
/** should be SCC_SYNC/SCC_ASYNC to set/reset sync, or      **/
/** SCC_FLUSH to leave sync mode as it is.                  **/
/*************************************************************/
void SyncSCC(register SCC *D,register byte Sync);

#endif /* SCC_H */
