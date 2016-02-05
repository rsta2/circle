/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                           SCC.c                         **/
/**                                                         **/
/** This file contains emulation for the SCC sound chip     **/
/** produced by Konami.                                     **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "SCC.h"
#include "Sound.h"
#include <string.h>

/** ResetSCC() ***********************************************/
/** Reset the sound chip and use sound channels from the    **/
/** one given in First.                                     **/
/*************************************************************/
void ResetSCC(register SCC *D,int First)
{
  int J;

  /* Reset registers */
  memset(D->R,0x00,sizeof(D->R));

  /* Set instruments, frequencies, volumes */
  for(J=0;J<SCC_CHANNELS;J++)
  {
    SetSound(0+First,SND_MELODIC);
    D->Freq[J]=D->Volume[J]=0;
  }

  D->First    = First;
  D->Sync     = SCC_ASYNC;
  D->Changed  = 0x00;
  D->WChanged = 0x00;
}

/** ReadSCC() ************************************************/
/** Call this function to read contents of the generic SCC  **/
/** sound chip registers.                                   **/
/*************************************************************/
byte ReadSCC(register SCC *D,register byte R)
{
  return(R<0x80? D->R[R]:0xFF);
}
           
/** ReadSCCP() ***********************************************/
/** Call this function to read contents of the newer SCC+   **/
/** sound chip registers.                                   **/
/*************************************************************/
byte ReadSCCP(register SCC *D,register byte R)
{
  return(R<0xA0? D->R[R]:0xFF);
}
           
/** WriteSCC() ***********************************************/
/** Call this function to output a value V into the generic **/
/** SCC sound chip.                                         **/
/*************************************************************/
void WriteSCC(register SCC *D,register byte R,register byte V)
{
  /* Prevent rollover */
  if(R>=0xE0) return;
  /* Generic SCC has one waveform less than SCC+ */
  if(R>=0x80) { WriteSCCP(D,R+0x20,V);return; }
  /* The last waveform applies to both channels 3 and 4 */
  if(R>=0x60) { WriteSCCP(D,R,V);WriteSCCP(D,R+0x20,V);return; }
  /* Other waveforms are the same */
  WriteSCCP(D,R,V);
}

/** WriteSCCP() **********************************************/
/** Call this function to output a value V into the newer   **/
/** SCC+ sound chip.                                        **/
/*************************************************************/
void WriteSCCP(register SCC *D,register byte R,register byte V)
{
  register int J;
  register byte I;

  /* Exit if no change */
  if(V==D->R[R]) return;

  if((R&0xE0)==0xA0)
  {
    /* Emulate melodic features */

    /* Save active channel mask in I */
    I=D->R[0xAF];
    /* Write to a register */
    R&=0xEF;
    D->R[R]=D->R[R+0x10]=V;

    /* Melodic register number 0..15 */
    R&=0x0F;
    switch(R)
    {
      case 0: case 1: case 2: case 3: case 4:
      case 5: case 6: case 7: case 8: case 9:
        /* Exit if the channel is silenced */
        if(!(I&(1<<(R>>1)))) return;
        /* Go to the first register of the pair */
        R=(R&0xFE)+0xA0;
        /* Compute frequency */
        J=((int)(D->R[R+1]&0x0F)<<8)+D->R[R];
        /* Compute channel number */
        R=(R&0x0F)>>1;
        /* Assign frequency */
        D->Freq[R]=J? SCC_BASE/J:0;
        /* Compute changed channels mask */
        D->Changed|=1<<R;
        break;

      case 10: case 11: case 12: case 13: case 14:
        /* Compute channel number */
        R-=10;
        /* Compute and assign new volume */
        D->Volume[R]=255*(V&0x0F)/15;
        /* Compute changed channels mask */
        D->Changed|=(1<<R)&I;
        break;

      case 15:
        /* Find changed channels */
        R=(V^I)&0x1F;
        D->Changed|=R;
        /* Update frequencies */
        for(I=0;R&&(I<SCC_CHANNELS);I++,R>>=1,V>>=1)
          if(R&1)
          {
            if(!(V&1)) D->Freq[I]=0;
            else
            {
              J=I*2+0xA0;
              J=((int)(D->R[J+1]&0x0F)<<8)+D->R[J];
              D->Freq[I]=J? SCC_BASE/J:0;
            }
          }
        break;

      default:
        /* Wrong register, do nothing */
        return;
    }
  }
  else
  {
    /* Emulate wave table features */

    /* Write data to SCC */
    D->R[R]=V;
    /* Wrong register, do nothing */
    if(R>=0xA0) return;
    /* Mark channel waveform as changed */
    D->WChanged|=1<<(R>>5);
  }

  /* For asynchronous mode, make Sound() calls right away */
  if(!D->Sync&&(D->Changed||D->WChanged)) SyncSCC(D,SCC_FLUSH);
}

/** SyncSCC() ************************************************/
/** Flush all accumulated changes by issuing Sound() calls  **/
/** and set the synchronization on/off. The second argument **/
/** should be SCC_SYNC/SCC_ASYNC to set/reset sync, or      **/
/** SCC_FLUSH to leave sync mode as it is.                  **/
/*************************************************************/
void SyncSCC(register SCC *D,register byte Sync)
{
  register int J,I;

  if(Sync!=SCC_FLUSH) D->Sync=Sync;

  /* Modify waveforms */
  for(J=0,I=D->WChanged;I&&(J<SCC_CHANNELS);J++,I>>=1)
    if(I&1) SetWave(J+D->First,(signed char *)(D->R+(J<<5)),32,0);

  /* Modify frequencies and volumes */
  for(J=0,I=D->Changed;I&&(J<SCC_CHANNELS);J++,I>>=1)
    if(I&1) Sound(J+D->First,D->Freq[J],D->Volume[J]);

  D->Changed=D->WChanged=0x00;
}
