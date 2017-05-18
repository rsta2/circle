/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         AY8910.c                        **/
/**                                                         **/
/** This file contains emulation for the AY8910 sound chip  **/
/** produced by General Instruments, Yamaha, etc. See       **/
/** AY8910.h for declarations.                              **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
/** Slightly Modified for SPC-1000 emulation,               **/
/** Added libsdl-releated routines                          **/
/**     by Kue-Hwan Sihn (ionique), (2006)                  **/
/*************************************************************/

#include "AY8910.h"
#include <circle/bcmrandom.h>
//#include "common.h"
//#include "Sound.h"
//#include <string.h>
//#include <stdlib.h>
//#include <stdio.h>
#ifndef NULL
#define NULL 0
#endif

#define RNG_WARMUP_COUNT	0x40000

CBcmRandomNumberGenerator m_Random;

static const unsigned char Envelopes[16][32] =
{
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 },
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 },
  { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15 },
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 },
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15 },
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 },
  { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
};

static const int Volumes[16] =
{ 0,1,2,4,6,8,11,16,23,32,45,64,90,128,180,255 };
//{ 0,16,33,50,67,84,101,118,135,152,169,186,203,220,237,254 };

/** Reset8910() **********************************************/
/** Reset the sound chip and use sound channels from the    **/
/** one given in First.                                     **/
/*************************************************************/
void Reset8910(register AY8910 *D,int First)
{
  static byte RegInit[16] =
  {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFD,
    0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00
  };
  int J;
#define ClockHz 4000000
  /* Reset state */
  memcpy(D->R,RegInit,sizeof(D->R));
  D->EPhase  = 0;
  D->Clock   = ClockHz>>4;
  D->First   = First;
  D->Sync    = AY8910_ASYNC;
  D->Changed = 0x00;
  D->EPeriod = 0;
  D->ECount  = 0;
  D->Latch   = 0x00;

  /* Set sound types */
  SetSound(0+First,SND_MELODIC);
  SetSound(1+First,SND_MELODIC);
  SetSound(2+First,SND_MELODIC);
  SetSound(3+First,SND_NOISE);
  SetSound(4+First,SND_NOISE);
  SetSound(5+First,SND_NOISE);

  /* Silence all channels */
  for(J=0;J<AY8910_CHANNELS;J++)
  {
    D->Freq[J]=D->Volume[J]=0;
    Sound(J+First,0,0);
  }
}

/** WrCtrl8910() *********************************************/
/** Write a value V to the PSG Control Port.                **/
/*************************************************************/
void WrCtrl8910(AY8910 *D,byte V)
{
  D->Latch=V&0x0F;
}

/** WrData8910() *********************************************/
/** Write a value V to the PSG Data Port.                   **/
/*************************************************************/
void WrData8910(AY8910 *D,byte V)
{
  Write8910(D,D->Latch,V);
}

/** RdData8910() *********************************************/
/** Read a value from the PSG Data Port.                    **/
/*************************************************************/
byte RdData8910(AY8910 *D)
{
  return(D->R[D->Latch]);
}

/** Write8910() **********************************************/
/** Call this function to output a value V into the sound   **/
/** chip.                                                   **/
/*************************************************************/
void Write8910(register AY8910 *D,register byte R,register byte V)
{
  register int J,I;

  /* Exit if no change */
  if((R>15)||((V==D->R[R])&&(R<8)&&(R>10))) return;
  switch(R)
  {
    case 1:
    case 3:
    case 5:
      V&=0x0F;
      /* Fall through */
    case 0:
    case 2:
    case 4:
      /* Write value */
      D->R[R]=V;
      /* Exit if the channel is silenced */
      if(D->R[7]&(1<<(R>>1))) return;
      /* Go to the first register in the pair */
      R&=0xFE;
      /* Compute frequency */
      J=((int)(D->R[R+1]&0x0F)<<8)+D->R[R];
      /* Compute channel number */
      R>>=1;
      /* Assign frequency */
      D->Freq[R]=D->Clock/(J? J:0x1000);
      /* Compute changed channels mask */
      D->Changed|=1<<R;
      break;

    case 6:
      /* Write value */
      D->R[6]=V&=0x1F;
      /* Exit if noise channels are silenced */
      if(!(~D->R[7]&0x38)) return;
      /* Compute and assign noise frequency */
      /* Shouldn't do <<2, but need to keep frequency down */
      J=D->Clock/((V&0x1F? (V&0x1F):0x20)<<2);
      if(!(D->R[7]&0x08)) D->Freq[3]=J;
      if(!(D->R[7]&0x10)) D->Freq[4]=J;
      if(!(D->R[7]&0x20)) D->Freq[5]=J;
      /* Compute changed channels mask */
      D->Changed|=0x38&~D->R[7];
      break;

    case 7:
      if (V & 0x80)
      {
          //printf("Print Strobe Req.\n");
      }
      /* Find changed channels */
      R=(V^D->R[7])&0x3F;
      D->Changed|=R;
      /* Write value */
      D->R[7]=V;
      /* Update frequencies */
      for(J=0;R&&(J<AY8910_CHANNELS);J++,R>>=1,V>>=1)
        if(R&1)
        {
          if(V&1) D->Freq[J]=0;
          else if(J<3)
          {
            I=((int)(D->R[J*2+1]&0x0F)<<8)+D->R[J*2];
            D->Freq[J]=D->Clock/(I? I:0x1000);
          }
          else
          {
            /* Shouldn't do <<2, but need to keep frequency down */
            I=D->R[6]&0x1F;
            D->Freq[J]=D->Clock/((I? I:0x20)<<2);
          }
        }
      break;

    case 8: // amplitude control
    case 9:
    case 10:
      /* Write value */
      D->R[R]=V&=0x1F;
      /* Compute channel number */
      R-=8;
      /* Compute and assign new volume */
      J=Volumes[V&0x10? Envelopes[D->R[13]&0x0F][D->EPhase]:(V&0x0F)];
      D->Volume[R]=J;
      D->Volume[R+3]=(J+1)>>1;
      /* Compute changed channels mask */
      D->Changed|=(0x09<<R)&~D->R[7];
      break;

    case 11: // envelope period control
    case 12:
      /* Write value */
      D->R[R]=V;
      /* Compute envelope period (why not <<4?) */
      J=((int)D->R[12]<<8)+D->R[11];
      D->EPeriod=1000*(J? J:0x10000)/D->Clock;
      /* No channels changed */
      return;

	case 13: // envelope shape/cycle control (ionique. originally the same as 14,15)
      /* Write value */
      D->R[R]=V&=0x0F;
      /* Reset envelope */
      D->ECount = 0;
      D->EPhase = 0;
      for(J=0;J<AY8910_CHANNELS/2;++J)
        if(D->R[J+8]&0x10)
        {
          I = Volumes[Envelopes[V][0]];
          D->Volume[J]   = I;
          D->Volume[J+3] = (I+1)>>1;
          D->Changed    |= (0x09<<J)&~D->R[7];
        }
      break;

	//case 13:
    case 14:
      return;
    case 15:
      /* Write Printer value */
      //printf("PRT<-%c (%d)\n", V, V);
      D->R[R]=V;
      /* No channels changed */
      return;

    default:
      /* Wrong register, do nothing */
      return;
  }

  /* For asynchronous mode, make Sound() calls right away */
  if(!D->Sync&&D->Changed) Sync8910(D,AY8910_FLUSH);
}

/** Loop8910() ***********************************************/
/** Call this function periodically to update volume        **/
/** envelopes. Use mS to pass the time since the last call  **/
/** of Loop8910() in milliseconds.                          **/
/*************************************************************/
// Modified by ionique
void Loop8910(register AY8910 *D,int mS)
{
  register int J,I;
  /* Exit if no envelope running */
  if(!D->EPeriod) return;

  /* Count milliseconds */
  D->ECount += mS;
  if(D->ECount<D->EPeriod) return;

  /* Count steps */
  J = D->ECount/D->EPeriod;
  D->ECount -= J*D->EPeriod;

  /* Count phases */
  D->EPhase += J;
  if(D->EPhase>31)
    D->EPhase = (D->R[13]&0x09)==0x08? (D->EPhase&0x1F):31;

  /* Set envelope volumes for relevant channels */
  for(I=0;I<3;++I)
    if(D->R[I+8]&0x10)
    {
      J = Volumes[Envelopes[D->R[13]&0x0F][D->EPhase]];
      D->Volume[I]   = J;
      D->Volume[I+3] = (J+1)>>1;
      D->Changed    |= (0x09<<I)&~D->R[7];
    }

  /* For asynchronous mode, make Sound() calls right away */
  if(!D->Sync&&D->Changed) Sync8910(D,AY8910_FLUSH);
}

/** Sync8910() ***********************************************/
/** Flush all accumulated changes by issuing Sound() calls  **/
/** and set the synchronization on/off. The second argument **/
/** should be AY8910_SYNC/AY8910_ASYNC to set/reset sync,   **/
/** or AY8910_FLUSH to leave sync mode as it is. To emulate **/
/** noise channels with MIDI drums, OR second argument with **/
/** AY8910_DRUMS.                                           **/
/*************************************************************/
void Sync8910(register AY8910 *D,register byte Sync)
{
  register int J,I;

  /* Hit MIDI drums for noise channels, if requested */
#define DRM_MIDI        0x100  /* MIDI drum (ORable)         */
  if(Sync&AY8910_DRUMS)
  {
    Sync&=~AY8910_DRUMS;
    J = (D->Freq[3]? D->Volume[3]:0)
      + (D->Freq[4]? D->Volume[4]:0)
      + (D->Freq[5]? D->Volume[5]:0);
    //if(J) Drum(DRM_MIDI|28,(J+2)/3);
  }

  if(Sync!=AY8910_FLUSH) D->Sync=Sync;

  for(J=0,I=D->Changed;I&&(J<AY8910_CHANNELS);J++,I>>=1)
    if(I&1) Sound(J+D->First,D->Freq[J],D->Volume[J]);
	//if(I&1) SndEnqueue(J+D->First,D->Freq[J],D->Volume[J]); // ionique
  	D->Changed=0x00;
}

/*************************************************************/
/** Libsdl sound driver + sound queue processing            **/
/** Sound queue is for precise, delayed sound processing.   **/
/**                             ionique, 2006               **/
/*************************************************************/

/*************************************************************/
/** Sound queue processing                                  **/
/*************************************************************/

/**
 * Sound Queue entry
 */
typedef struct {
	int chn;
	int freq;
	int vol;
	int time; // in ms
} TSndQEntry;

#define MAX_SNDQ 1000	// 1000 is enough for most cases

/**
 * Sound Queue structure
 */
typedef struct {
	int front;
	int rear;
	TSndQEntry qentry[MAX_SNDQ];
} TSndQ;

TSndQ SndQ;
int LastBufTime = 0;
int *sound_mutex;

TSndQEntry *SndDequeue(void);

void SndQueueInit(void)
{
	int i;

	lock_mutex_mmu(sound_mutex);
	LastBufTime = SDL_GetTicks();
	SndQ.front = SndQ.rear = 0;
	unlock_mutex_mmu(sound_mutex);

	for (i = 0; i < 6; i++)
		Sound(i, 0, 0);
}

int SndEnqueue(int Chn, int Freq, int Vol)
{
	int tfront;

	lock_mutex_mmu(sound_mutex);
	tfront = (SndQ.front + 1) % MAX_SNDQ;
	if (tfront == SndQ.rear)
	{
//#define QUEUEFULL_PROCESS
#ifdef QUEUEFULL_PROCESS // not working yet
		TSndQEntry *qentry;
		//printf("Queue Full!!\n");
		qentry = SndDequeue();
		Sound(qentry->chn, qentry->freq, qentry->vol);
#else
		//printf("Queue Full!!\n");
#endif
		unlock_mutex_mmu(sound_mutex);
		return 0;
	}
	SndQ.qentry[SndQ.front].chn = Chn;
	SndQ.qentry[SndQ.front].freq = Freq;
	SndQ.qentry[SndQ.front].vol = Vol;
	SndQ.qentry[SndQ.front].time = SDL_GetTicks() - LastBufTime;
	SndQ.front = tfront;
	unlock_mutex_mmu(sound_mutex);
	return 1;
}

TSndQEntry *SndDequeue(void)
{
	int trear;

	lock_mutex_mmu(sound_mutex);
	trear = SndQ.rear;
	if (SndQ.front == SndQ.rear)
	{
		unlock_mutex_mmu(sound_mutex);
		return NULL; // queue empty
	}
	SndQ.rear = (SndQ.rear + 1) % MAX_SNDQ;
	unlock_mutex_mmu(sound_mutex);

	return &(SndQ.qentry[trear]);
}

#define SndPeekQueue() ((SndQ.front == SndQ.rear) ? -1: SndQ.qentry[SndQ.rear].time)

/*************************************************************/
/** Dispatch sound queue entry and make real sound for SDL  **/
/*************************************************************/

static int Freq[6];
static int Vol[6];
static int Interval[6];
static int NoiseInterval[6];
static int Phase[6];
static int DevFreq;
static volatile int JF = 0;

//#define DEVFREQ 22050
#define DEVFREQ 44100

void Sound(int Chn,int Freq,int Volume)
{
	int interval;

	interval = Freq? (DevFreq/Freq):0;
	if (interval != Interval[Chn])
	{
		Interval[Chn] = interval;
		Phase[Chn] = 0;
	}
	if (Interval[Chn] == 0)
		Vol[Chn] = 0;
	else
		Vol[Chn] = Volume * 16384;//spconf.snd_vol; // do not exceed 32767
}

static void DSPCallBack(void* unused, unsigned char *stream, int len)
{
	register int   J;
	static int R1 = 0,R2 = 0;
	int i;
	int vTime, qTime; // virtual Time, queue Time
	TSndQEntry *qentry = NULL;

	for(J=0;J<len;J+=4) // for the requested amount of buffer
	{
		vTime = (250*J)/DevFreq;
		qTime = SndPeekQueue();

		while ((qTime != -1) && (vTime - qTime) > 0) // Check Sound Queue
		{
			qentry = SndDequeue();

			Sound(qentry->chn, qentry->freq, qentry->vol);
			qTime = SndPeekQueue();
		}

		R1 = 0;
		for (i = 0; i < 3; i++) // Tone Generation
		{
			if (Interval[i] && Vol[i])
			{
				if (Phase[i] < (Interval[i]/2))
				{
					R1 += Vol[i];
					R1 = (R1 > 32767)? 32767: R1;
				}
				else if (Phase[i] >= (Interval[i]/2) && Phase[i] < Interval[i])
				{
					R1 -= Vol[i];
					R1 = (R1 < -32768)? -32768: R1;
				}
				Phase[i] ++;
				if (Phase[i] >= Interval[i])
				{
					Phase[i] = 0;
					//R1 += Vol[i];
				}
			}
		}

		for (i = 3; i < 6; i++) // Noise Generation
		{
			if (Interval[i] && Vol[i])
			{
				if (Phase[i] == 0)
					NoiseInterval[i] = Interval[i] + (((150 * m_Random.GetNumber ()) / RNG_WARMUP_COUNT) - 75);
				if (Phase[i] < (NoiseInterval[i]/2))
				{
					R1 += Vol[i];
					R1 = (R1 > 32767)? 32767: R1;
				}
				else if (Phase[i] >= (NoiseInterval[i]/2) && Phase[i] < NoiseInterval[i])
				{
					R1 -= Vol[i];
					R1 = (R1 < -32768)? -32768: R1;
				}
				Phase[i] ++;
				if (Phase[i] >= NoiseInterval[i])
				{
					Phase[i] = 0;
					//R1 += Vol[i];
				}
			}
		}

		R2 = R1;
		stream[J+0]=R2&0x00FF;
		stream[J+1]=R2>>8;
		stream[J+2]=R1&0x00FF;
		stream[J+3]=R1>>8;
	}

	LastBufTime = SDL_GetTicks();
	while (qentry = SndDequeue())
		Sound(qentry->chn, qentry->freq, qentry->vol);

}

void SetSound(int Channel,int NewType)
{
//	printf("SetSound: Chn=%d, NewType=%d\n", Channel, NewType);
}


// adapted from sdl patch for fmsx
#if 0
void OpenSoundDevice(void)
{
  /* Open the audio device */
  SDL_AudioSpec *desired, *obtained;
  int SndBufSize = 2048; // 1024 for 22050, 2048 for 44100
  int Rate = DEVFREQ;

  sound_mutex=SDL_CreateMutex();

  /* Allocate a desired SDL_AudioSpec */
  desired=(SDL_AudioSpec*)malloc(sizeof(SDL_AudioSpec));

  /* Allocate space for the obtained SDL_AudioSpec */
  obtained=(SDL_AudioSpec*)malloc(sizeof(SDL_AudioSpec));

  /* set audio parameters */
  desired->freq=Rate;
  desired->format=AUDIO_S16LSB; /* 16-bit signed audio */
  desired->samples=SndBufSize;  /* size audio buffer */
  desired->channels=2;

  /* Our callback function */
  desired->callback=DSPCallBack;
  desired->userdata=NULL;

  /* Open the audio device */
  if(SDL_OpenAudio(desired, obtained)<0)
  {
      //printf("Audio Open failed\n");
      return;
  }
  //printf("Audio Open successed\n");

  Rate=obtained->freq;

  /* Adjust buffer size to the obtained buffer size */
  SndBufSize=obtained->samples;

  /* Start playing */
  SDL_PauseAudio(0);

  /* Free memory */
  free(obtained);
  free(desired);

  DevFreq = Rate;
  SndQueueInit();
}
#endif
