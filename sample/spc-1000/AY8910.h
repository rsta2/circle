/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         AY8910.h                        **/
/**                                                         **/
/** This file contains emulation for the AY8910 sound chip  **/
/** produced by General Instruments, Yamaha, etc. See       **/
/** AY8910.c for the actual code.                           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2005                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef AY8910_H
#define AY8910_H

#define AY8910_BASE     111861 /* Base frequency for AY8910  */
#define AY8910_CHANNELS 6      /* 3 melodic + 3 noise chanls */

#define AY8910_ASYNC    0      /* Asynchronous emulation     */
#define AY8910_SYNC     1      /* Synchronous emulation      */
#define AY8910_FLUSH    2      /* Flush buffers only         */
#define AY8910_DRUMS    0x80   /* Hit drums for noise chnls  */

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;

                               /* SetSound() arguments:      */
#define SND_MELODIC     0      /* Melodic sound (default)    */
#define SND_RECTANGLE   0      /* Rectangular wave           */
#define SND_TRIANGLE    1      /* Triangular wave (1/2 rect.)*/
#define SND_NOISE       2      /* White noise                */

/** Sound() **************************************************/
/** Generate sound of given frequency (Hz) and volume       **/
/** (0..255) via given channel. Setting Freq=0 or Volume=0  **/
/** turns sound off.                                        **/
/*************************************************************/
void Sound(int Channel,int Freq,int Volume);
int SndEnqueue(int Channel,int Freq,int Volume);

/** SetSound() ***********************************************/
/** Set sound type at a given channel. MIDI instruments can **/
/** be set directly by ORing their numbers with SND_MIDI.   **/
/*************************************************************/
void SetSound(int Channel,int NewType);
#endif

/** AY8910 ***************************************************/
/** This data structure stores AY8910 state.                **/
/*************************************************************/
typedef struct
{
  byte R[16];                  /* PSG registers contents     */
  int Freq[AY8910_CHANNELS];   /* Frequencies (0 for off)    */
  int Volume[AY8910_CHANNELS]; /* Volumes (0..255)           */
  int Clock;                   /* Base clock used by PSG     */
  int First;                   /* First used Sound() channel */
  byte Changed;                /* Bitmap of changed channels */
  byte Sync;                   /* AY8910_SYNC/AY8910_ASYNC   */
  byte Latch;                  /* Latch for the register num */
  int EPeriod;                 /* Envelope step in msecs     */
  int ECount;                  /* Envelope step counter      */
  int EPhase;                  /* Envelope phase             */
} AY8910;

/** Reset8910() **********************************************/
/** Reset the sound chip and use sound channels from the    **/
/** one given in First.                                     **/
/*************************************************************/
void Reset8910(register AY8910 *D,int First);

/** Write8910() **********************************************/
/** Call this function to output a value V into the sound   **/
/** chip.                                                   **/
/*************************************************************/
void Write8910(register AY8910 *D,register byte R,register byte V);

/** WrCtrl8910() *********************************************/
/** Write a value V to the PSG Control Port.                **/
/*************************************************************/
void WrCtrl8910(AY8910 *D,byte V);

/** WrData8910() *********************************************/
/** Write a value V to the PSG Data Port.                   **/
/*************************************************************/
void WrData8910(AY8910 *D,byte V);

/** RdData8910() *********************************************/
/** Read a value from the PSG Data Port.                    **/
/*************************************************************/
byte RdData8910(AY8910 *D);

/** Sync8910() ***********************************************/
/** Flush all accumulated changes by issuing Sound() calls  **/
/** and set the synchronization on/off. The second argument **/
/** should be AY8910_SYNC/AY8910_ASYNC to set/reset sync,   **/
/** or AY8910_FLUSH to leave sync mode as it is. To emulate **/
/** noise channels with MIDI drums, OR second argument with **/
/** AY8910_DRUMS.                                           **/
/*************************************************************/
void Sync8910(register AY8910 *D,register byte Sync);

/** Loop8910() ***********************************************/
/** Call this function periodically to update volume        **/
/** envelopes. Use mS to pass the time since the last call  **/
/** of Loop8910() in milliseconds.                          **/
/*************************************************************/
void Loop8910(register AY8910 *D,int mS);

/*************************************************************/
/** SPC-1000 specific - ionique                             **/
/*************************************************************/
void SndQueueInit(void);
void OpenSoundDevice(void);

#endif /* AY8910_H */
