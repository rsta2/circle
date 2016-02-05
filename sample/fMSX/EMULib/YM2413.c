/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         YM2413.c                        **/
/**                                                         **/
/** This file contains emulation for the OPLL sound chip    **/
/** produced by Yamaha (also see OPL2, OPL3, OPL4 chips).   **/
/** See YM2413.h for declarations.                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "YM2413.h"
#include "Sound.h"
#include <string.h>

/** Patches2413() ********************************************/
/** MIDI instruments corresponding to the OPLL patches.     **/
/*************************************************************/
static const byte Patches2413[16] =
{
      /*** OPLL ***/    /*** MIDI ***/
  90, /* Original */    /* User (Polysynth) */
  40, /* Violin */      /* Violin */
  27, /* Guitar */      /* Electric Guitar (clean) */
  1,  /* Piano */       /* Bright Acoustic Piano */
  73, /* Flute */       /* Flute */
  71, /* Clarinet */    /* Clarinet */
  68, /* Oboe */        /* Oboe */
  56, /* Trumpet */     /* Trumpet */
  20, /* Organ */       /* Reed Organ */
  58, /* Horn */        /* Tuba */
  50, /* Synthesizer */ /* Synth Strings 1 */
  6,  /* Harpsichord */ /* Harpsichord */
  11, /* Vibraphone */  /* Vibraphone */
  38, /* Synth Bass */  /* Synth Bass 1 */
  34, /* Wood Bass */   /* Electric Bass (pick) */
  33  /* Elec Guitar */ /* Electric Bass (finger) */
};

/** Drums2413() **********************************************/
/** MIDI instruments corresponding to the OPLL drums.       **/
/*************************************************************/
static const byte Drums2413[5] =
{
      /*** OPLL ***/    /*** MIDI ***/
  42, /* High Hat */    /* Closed Hi Hat */
  49, /* Top Cymbal */  /* Crash Cymbal 1 */
  47, /* Tom-Tom */     /* Low-Mid Tom */
  40, /* Snare Drum */  /* Electric Snare */
  36  /* Bass Drum */   /* Bass Drum 1 */
};

/** Synth2413() **********************************************/
/** Synthesizer parameters corresponding to OPLL patches.   **/
/*************************************************************/
static const byte Synth2413[19*16] =
{
  0x49,0x4c,0x4c,0x32,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x61,0x61,0x1e,0x17,0xf0,0x7f,0x00,0x17,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x13,0x41,0x16,0x0e,0xfd,0xf4,0x23,0x23,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x03,0x01,0x9a,0x04,0xf3,0xf3,0x13,0xf3,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x11,0x61,0x0e,0x07,0xfa,0x64,0x70,0x17,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x22,0x21,0x1e,0x06,0xf0,0x76,0x00,0x28,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x21,0x22,0x16,0x05,0xf0,0x71,0x00,0x18,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x21,0x61,0x1d,0x07,0x82,0x80,0x17,0x17,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x23,0x21,0x2d,0x16,0x90,0x90,0x00,0x07,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x21,0x21,0x1b,0x06,0x64,0x65,0x10,0x17,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x21,0x21,0x0b,0x1a,0x85,0xa0,0x70,0x07,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x23,0x01,0x83,0x10,0xff,0xb4,0x10,0xf4,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x97,0xc1,0x20,0x07,0xff,0xf4,0x22,0x22,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x61,0x00,0x0c,0x05,0xc2,0xf6,0x40,0x44,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x01,0x01,0x56,0x03,0x94,0xc2,0x03,0x12,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x21,0x01,0x89,0x03,0xf1,0xe4,0xf0,0x23,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x07,0x21,0x14,0x00,0xee,0xf8,0xff,0xf8,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x01,0x31,0x00,0x00,0xf8,0xf7,0xf8,0xf7,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x25,0x11,0x00,0x00,0xf8,0xfa,0xf8,0x55,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/** Reset2413() **********************************************/
/** Reset the sound chip and use sound channels from the    **/
/** one given in First.                                     **/
/*************************************************************/
void Reset2413(register YM2413 *D,int First)
{
  int J;

  /* All registers filled with 0x00 by default */
  memset(D->R,0x00,sizeof(D->R));

  /* Set initial frequencies, volumes, and instruments */
  for(J=0;J<YM2413_CHANNELS;J++)
  {
    SetSound(J+First,SND_MELODIC);
    D->Freq[J]   = 0;
    D->Volume[J] = 0;
    D->R[J+0x30] = 0x0F;
  }

  D->First    = First;
  D->Sync     = YM2413_ASYNC;
  D->Changed  = (1<<YM2413_CHANNELS)-1;
  D->PChanged = (1<<YM2413_CHANNELS)-1;
  D->DChanged = (1<<YM2413_CHANNELS)-1;
  D->Latch    = 0;
}

/** WrCtrl2413() *********************************************/
/** Write a value V to the OPLL Control Port.               **/
/*************************************************************/
void WrCtrl2413(register YM2413 *D,register byte V)
{
  D->Latch=V&0x3F;
}

/** WrData2413() *********************************************/
/** Write a value V to the OPLL Data Port.                  **/
/*************************************************************/
void WrData2413(register YM2413 *D,register byte V)
{
  Write2413(D,D->Latch,V);
}

/** Write2413() **********************************************/
/** Call this function to output a value V into the sound   **/
/** chip.                                                   **/
/*************************************************************/
void Write2413(register YM2413 *D,register byte R,register byte V)
{
  register byte C,Oct;
  register int Frq;

  /* OPLL registers are 0..63 */
  R&=0x3F;

  /* Lowest 4 bits are channel number */
  C=R&0x0F;

  switch(R>>4)
  {
    case 0:
      switch(C)
      {
        case 0x0E:
          if(V==D->R[R]) return;
          /* Keep all drums off when drum mode is off */
          if(!(V&0x20)) V&=0xE0;
          /* Mark all activated drums as changed */
          D->DChanged|=(V^D->R[R])&0x1F;
          /* If drum mode was turned on... */
          if((V^D->R[R])&V&0x20)
          {
            /* Turn off melodic channels 6,7,8 */
            D->Freq[6]=D->Freq[7]=D->Freq[8]=0;
            /* Mark channels 6,7,8 as changed */
            D->Changed|=0x1C0;
          }
          /* Done */
          break;
      }
      break;

    case 1:
      if((C>8)||(V==D->R[R])) return;
      if(!YM2413_DRUMS(D)||(C<6))
        if(D->R[R+0x10]&0x10)
        {
          /* Set channel frequency */
          Oct=D->R[R+0x10];
          Frq=((int)(Oct&0x01)<<8)+V;
          Oct=(Oct&0x0E)>>1;
          D->Freq[C]=(3125*Frq*(1<<Oct))>>15;

          /* Mark channel as changed */
          D->Changed|=1<<C;
        }
      /* Done */
      break;

    case 2:
      if(C>8) return;
      if(!YM2413_DRUMS(D)||(C<6))
      {
        /* Depending on whether channel is on/off... */
        if(!(V&0x10)) D->Freq[C]=0;
        else
        {
          /* Set channel frequency */
          Frq=((int)(V&0x01)<<8)+D->R[R-0x10];
          Oct=(V&0x0E)>>1;
          D->Freq[C]=(3125*Frq*(1<<Oct))>>15;
        }

        /* Mark channel as changed */
        D->Changed|=1<<C;
      }
      /* Done */
      break;

    case 3:
      if((C>8)||(V==D->R[R])) return;
      /* Register any patch changes */
      if((V^D->R[R])&0xF0) D->PChanged|=1<<C;
      /* Register any volume changes */
      if((V^D->R[R])&0x0F)
      {
        /* Set channel volume */
        D->Volume[C]=255*(~V&0x0F)/15;
        /* Mark channel as changed */
        D->Changed|=1<<C;
      }
      /* Register drum volume changes */
      if(YM2413_DRUMS(D))
        switch(C)
        {
          case 6: D->DChanged|=0x10&D->R[0x0E];break;
          case 7: D->DChanged|=0x09&D->R[0x0E];break;
          case 8: D->DChanged|=0x06&D->R[0x0E];break;
        }
      /* Done */
      break;
  }

  /* Write value into the register */
  D->R[R]=V;

  /* For asynchronous mode, make Sound() calls right away */
  if(!D->Sync&&(D->Changed||D->PChanged||D->DChanged))
    Sync2413(D,YM2413_FLUSH);
}

/** Sync2413() ***********************************************/
/** Flush all accumulated changes by issuing Sound() calls  **/
/** and set the synchronization on/off. The second argument **/
/** should be YM2413_SYNC/YM2413_ASYNC to set/reset sync,   **/
/** or YM2413_FLUSH to leave sync mode as it is.            **/
/*************************************************************/
void Sync2413(register YM2413 *D,register byte Sync)
{
  register int J,I;

  /* Change sync mode as requested */
  if(Sync!=YM2413_FLUSH) D->Sync=Sync;

  /* Convert channel instrument changes into SetSound() calls */
  for(J=0,I=D->PChanged;I&&(J<YM2413_CHANNELS);++J,I>>=1)
    if(I&1) SetSound(J+D->First,SND_MIDI|Patches2413[D->R[J+0x30]>>4]);

  /* Convert channel freq/volume changes into Sound() calls */
  for(J=0,I=D->Changed;I&&(J<YM2413_CHANNELS);++J,I>>=1)
    if(I&1) Sound(J+D->First,D->Freq[J],D->Volume[J]);

  /* If there were any changes to the drums... */
  I=D->DChanged;
  J=D->R[0x0E];
  if(I)
  {
    /* Turn drums on/off as requested */
    if(I&0x01) Drum(DRM_MIDI|Drums2413[0],J&0x01? 255*(D->R[0x37]>>4)/15:0);
    if(I&0x02) Drum(DRM_MIDI|Drums2413[1],J&0x02? 255*(D->R[0x38]&0x0F)/15:0);
    if(I&0x04) Drum(DRM_MIDI|Drums2413[2],J&0x04? 255*(D->R[0x38]>>4)/15:0);
    if(I&0x08) Drum(DRM_MIDI|Drums2413[3],J&0x08? 255*(D->R[0x37]&0x0F)/15:0);
    if(I&0x10) Drum(DRM_MIDI|Drums2413[4],J&0x10? 255*(D->R[0x36]&0x0F)/15:0);
  }

  D->Changed=D->PChanged=D->DChanged=0x000;
}
