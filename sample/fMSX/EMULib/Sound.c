/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          Sound.c                        **/
/**                                                         **/
/** This file file implements core part of the sound API    **/
/** and functions needed to log soundtrack into a MIDI      **/
/** file. See Sound.h for declarations.                     **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "Sound.h"

#include <stdio.h>
#include <string.h>

#if defined(UNIX) || defined(MAEMO) || defined(STMP3700) || defined(NXC2600) || defined(ANDROID)
#include <unistd.h>
#endif

typedef unsigned char byte;
typedef unsigned short word;

struct SndDriverStruct SndDriver =
{
  (void (*)(int,int))0,
  (void (*)(int,int))0,
  (void (*)(int,int))0,
  (void (*)(int,int,int))0,
  (void (*)(int,const signed char *,int,int))0,
  (const signed char *(*)(int))0
};

static const struct { byte Note;word Wheel; } Freqs[4096] =
{
#include "MIDIFreq.h"
};

static const int Programs[] =
{
  80,  /* SND_MELODIC/SND_RECTANGLE */
  80,  /* SND_TRIANGLE */
  122, /* SND_NOISE */
  122, /* SND_PERIODIC */
  80,  /* SND_WAVE */
};

static struct
{
  int Type;
  int Note;
  int Pitch;
  int Level;
} MidiCH[MIDI_CHANNELS] =
{
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 },
  { -1,-1,-1,-1 }
};

static struct
{
  int Type;                       /* Channel type (SND_*)             */
  int Freq;                       /* Channel frequency (Hz)           */
  int Volume;                     /* Channel volume (0..255)          */

  const signed char *Data;        /* Wave data (-128..127 each)       */
  int Length;                     /* Wave length in Data              */
  int Rate;                       /* Wave playback rate (or 0Hz)      */
  int Pos;                        /* Wave current position in Data    */  

  int Count;                      /* Phase counter                    */
} WaveCH[SND_CHANNELS] =
{
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 },
  { SND_MELODIC,0,0,0,0,0,0,0 }
};

/** RenderAudio() Variables *******************************************/
static int SndRate    = 0;        /* Sound rate (0=Off)               */
static int NoiseGen   = 0x10000;  /* Noise generator seed             */
static int NoiseOut   = 16;       /* NoiseGen bit used for output     */
static int NoiseXor   = 14;       /* NoiseGen bit used for XORing     */
int MasterSwitch      = 0xFFFF;   /* Switches to turn channels on/off */
int MasterVolume      = 192;      /* Master volume                    */

/** MIDI Logging Variables ********************************************/
static const char *LogName = 0;   /* MIDI logging file name           */
static int  Logging   = MIDI_OFF; /* MIDI logging state (MIDI_*)      */
static int  TickCount = 0;        /* MIDI ticks since WriteDelta()    */
static int  LastMsg   = -1;       /* Last MIDI message                */
static int  DrumOn    = 0;        /* 1: MIDI drums are ON             */
static FILE *MIDIOut  = 0;        /* MIDI logging file handle         */

static void MIDISound(int Channel,int Freq,int Volume);
static void MIDISetSound(int Channel,int Type);
static void MIDIDrum(int Type,int Force);
static void MIDIMessage(byte D0,byte D1,byte D2);
static void NoteOn(byte Channel,byte Note,byte Level);
static void NoteOff(byte Channel);
static void WriteDelta(void);
static void WriteTempo(int Freq);

/** SHIFT() **************************************************/
/** Make MIDI channel#10 last, as it is normally used for   **/
/** percussion instruments only and doesn't sound nice.     **/
/*************************************************************/
#define SHIFT(Ch) (Ch==15? 9:Ch>8? Ch+1:Ch)

/** GetSndRate() *********************************************/
/** Get current sampling rate used for synthesis.           **/
/*************************************************************/
unsigned int GetSndRate(void) { return(SndRate); }

/** Sound() **************************************************/
/** Generate sound of given frequency (Hz) and volume       **/
/** (0..255) via given channel. Setting Freq=0 or Volume=0  **/
/** turns sound off.                                        **/
/*************************************************************/
void Sound(int Channel,int Freq,int Volume)
{
  /* All parameters have to be valid */
  if((Channel<0)||(Channel>=SND_CHANNELS)) return;
  Freq   = Freq<0? 0:Freq;
  Volume = Volume<0? 0:Volume>255? 255:Volume;

  /* Modify wave channel */ 
  WaveCH[Channel].Volume = Volume;
  WaveCH[Channel].Freq   = Freq;

  /* Call sound driver if present */
  if(SndDriver.Sound) (*SndDriver.Sound)(Channel,Freq,Volume);

  /* Log sound to MIDI file */
  MIDISound(Channel,Freq,Volume);
}

/** Drum() ***************************************************/
/** Hit a drum of given type with given force (0..255).     **/
/** MIDI drums can be used by ORing their numbers with      **/
/** SND_MIDI.                                               **/
/*************************************************************/
void Drum(int Type,int Force)
{
  /* Drum force has to be valid */
  Force = Force<0? 0:Force>255? 255:Force;

  /* Call sound driver if present */
  if(SndDriver.Drum) (*SndDriver.Drum)(Type,Force);

  /* Log drum to MIDI file */
  MIDIDrum(Type,Force);
}

/** SetSound() ***********************************************/
/** Set sound type at a given channel. MIDI instruments can **/
/** be set directly by ORing their numbers with SND_MIDI.   **/
/*************************************************************/
void SetSound(int Channel,int Type)
{
  /* Channel has to be valid */
  if((Channel<0)||(Channel>=SND_CHANNELS)) return;

  /* Set wave channel type */
  WaveCH[Channel].Type = Type;

  /* Call sound driver if present */
  if(SndDriver.SetSound) (*SndDriver.SetSound)(Channel,Type);

  /* Log instrument change to MIDI file */
  MIDISetSound(Channel,Type);
}

/** SetChannels() ********************************************/
/** Set master volume (0..255) and switch channels on/off.  **/
/** Each channel N has corresponding bit 2^N in Switch. Set **/
/** or reset this bit to turn the channel on or off.        **/ 
/*************************************************************/
void SetChannels(int Volume,int Switch)
{
  /* Volume has to be valid */
  Volume = Volume<0? 0:Volume>255? 255:Volume;

  /* Call sound driver if present */
  if(SndDriver.SetChannels) (*SndDriver.SetChannels)(Volume,Switch);

  /* Modify wave master settings */ 
  MasterVolume = Volume;
  MasterSwitch = Switch&((1<<SND_CHANNELS)-1);
}

/** SetNoise() ***********************************************/
/** Initialize random noise generator to the given Seed and **/
/** then take random output from OUTBit and XOR it with     **/
/** XORBit.                                                 **/
/*************************************************************/
void SetNoise(int Seed,int OUTBit,int XORBit)
{
  NoiseGen = Seed;
  NoiseOut = OUTBit;
  NoiseXor = XORBit;
}

/** SetWave() ************************************************/
/** Set waveform for a given channel. The channel will be   **/
/** marked with sound type SND_WAVE. Set Rate=0 if you want **/
/** waveform to be an instrument or set it to the waveform  **/
/** own playback rate.                                      **/
/*************************************************************/
void SetWave(int Channel,const signed char *Data,int Length,int Rate)
{
  /* Channel and waveform length have to be valid */
  if((Channel<0)||(Channel>=SND_CHANNELS)||(Length<=0)) return;

  /* Set wave channel parameters */
  WaveCH[Channel].Type   = SND_WAVE;
  WaveCH[Channel].Length = Length;
  WaveCH[Channel].Rate   = Rate;
  WaveCH[Channel].Pos    = Length? WaveCH[Channel].Pos%Length:0;
  WaveCH[Channel].Count  = 0;
  WaveCH[Channel].Data   = Data;

  /* Call sound driver if present */
  if(SndDriver.SetWave) (*SndDriver.SetWave)(Channel,Data,Length,Rate);

  /* Log instrument change to MIDI file */
  MIDISetSound(Channel,Rate? -1:SND_MELODIC);
}

/** GetWave() ************************************************/
/** Get current read position for the buffer set with the   **/
/** SetWave() call. Returns 0 if no buffer has been set, or **/
/** if there is no playrate set (i.e. wave is instrument).  **/
/*************************************************************/
const signed char *GetWave(int Channel)
{
  /* Channel has to be valid */
  if((Channel<0)||(Channel>=SND_CHANNELS)) return(0);

  /* If driver present, call it */
  if(SndDriver.GetWave) return((*SndDriver.GetWave)(Channel));

  /* Return current read position */
  return(
    WaveCH[Channel].Rate&&(WaveCH[Channel].Type==SND_WAVE)?
    WaveCH[Channel].Data+WaveCH[Channel].Pos:0
  );
}

/** InitMIDI() ***********************************************/
/** Initialize soundtrack logging into MIDI file FileName.  **/
/** Repeated calls to InitMIDI() will close current MIDI    **/
/** file and continue logging into a new one.               **/ 
/*************************************************************/
void InitMIDI(const char *FileName)
{
  int WasLogging;

  /* Must pass a name! */
  if(!FileName) return;

  /* Memorize logging status */
  WasLogging=Logging;

  /* If MIDI logging in progress, close current file */
  if(MIDIOut) TrashMIDI();

  /* Set log file name and ticks/second parameter, no logging yet */
  LogName   = FileName;
  Logging   = MIDI_OFF;
  LastMsg   = -1;
  TickCount = 0;
  MIDIOut   = 0;
  DrumOn    = 0;

  /* If was logging, restart */
  if(WasLogging) MIDILogging(MIDI_ON);
}

/** TrashMIDI() **********************************************/
/** Finish logging soundtrack and close the MIDI file.      **/
/*************************************************************/
void TrashMIDI(void)
{
  long Length;
  int J;

  /* If not logging, drop out */
  if(!MIDIOut) return;
  /* Turn sound off */
  for(J=0;J<MIDI_CHANNELS;J++) NoteOff(J);
  /* End of track */
  MIDIMessage(0xFF,0x2F,0x00);
  /* Put track length in file */
  fseek(MIDIOut,0,SEEK_END);
  Length=ftell(MIDIOut)-22;
  fseek(MIDIOut,18,SEEK_SET);
  fputc((Length>>24)&0xFF,MIDIOut);
  fputc((Length>>16)&0xFF,MIDIOut);
  fputc((Length>>8)&0xFF,MIDIOut);
  fputc(Length&0xFF,MIDIOut);

  /* Done logging */
  fclose(MIDIOut);
  Logging   = MIDI_OFF;
  LastMsg   = -1;
  TickCount = 0;
  MIDIOut   = 0;
}

/** MIDILogging() ********************************************/
/** Turn soundtrack logging on/off and return its current   **/
/** status. Possible values of Switch are MIDI_OFF (turn    **/
/** logging off), MIDI_ON (turn logging on), MIDI_TOGGLE    **/
/** (toggle logging), and MIDI_QUERY (just return current   **/
/** state of logging).                                      **/
/*************************************************************/
int MIDILogging(int Switch)
{
  static const char MThd[] = "MThd\0\0\0\006\0\0\0\1";
                           /* ID  DataLen   Fmt Trks */
  static const char MTrk[] = "MTrk\0\0\0\0";
                           /* ID  TrkLen   */
  int J,I;

  /* Toggle logging if requested */
  if(Switch==MIDI_TOGGLE) Switch=!Logging;

  if((Switch==MIDI_ON)||(Switch==MIDI_OFF))
    if(Switch^Logging)
    {
      /* When turning logging off, silence all channels */
      if(!Switch&&MIDIOut)
        for(J=0;J<MIDI_CHANNELS;J++) NoteOff(J);

      /* When turning logging on, open MIDI file */
      if(Switch&&!MIDIOut&&LogName)
      {
        /* No messages have been sent yet */
        LastMsg=-1;

        /* Clear all storage */
        for(J=0;J<MIDI_CHANNELS;J++)
          MidiCH[J].Note=MidiCH[J].Pitch=MidiCH[J].Level=-1;

        /* Open new file and write out the header */
        MIDIOut=fopen(LogName,"wb");
        if(!MIDIOut) return(MIDI_OFF);
        if(fwrite(MThd,1,12,MIDIOut)!=12)
        { fclose(MIDIOut);MIDIOut=0;return(MIDI_OFF); }
        fputc((MIDI_DIVISIONS>>8)&0xFF,MIDIOut);
        fputc(MIDI_DIVISIONS&0xFF,MIDIOut);
        if(fwrite(MTrk,1,8,MIDIOut)!=8)
        { fclose(MIDIOut);MIDIOut=0;return(MIDI_OFF); }

        /* Write out the tempo */
        WriteTempo(MIDI_DIVISIONS);
      }

      /* Turn logging off on failure to open MIDIOut */
      if(!MIDIOut) Switch=MIDI_OFF;

      /* Assign new switch value */
      Logging=Switch;

      /* If switching logging on... */
      if(Switch)
      {
        /* Start logging without a pause */
        TickCount=0;

        /* Write instrument changes */
        for(J=0;J<MIDI_CHANNELS;J++)
          if((MidiCH[J].Type>=0)&&(MidiCH[J].Type&0x10000))
          {
            I=MidiCH[J].Type&~0x10000;
            MidiCH[J].Type=-1;
            MIDISetSound(J,I);
          }
      }
    }

  /* Return current logging status */
  return(Logging);
}

/** MIDITicks() **********************************************/
/** Log N 1ms MIDI ticks.                                   **/
/*************************************************************/
void MIDITicks(int N)
{
  if(Logging&&MIDIOut&&(N>0)) TickCount+=N;
}

/** MIDISound() **********************************************/
/** Set sound frequency (Hz) and volume (0..255) for a      **/
/** given channel.                                          **/
/*************************************************************/
void MIDISound(int Channel,int Freq,int Volume)
{
  int MIDIVolume,MIDINote,MIDIWheel;

  /* If logging off, file closed, or invalid channel, drop out */
  if(!Logging||!MIDIOut||(Channel>=MIDI_CHANNELS-1)||(Channel<0)) return;
  /* Frequency must be in range */
  if((Freq<MIDI_MINFREQ)||(Freq>MIDI_MAXFREQ)) Freq=0;
  /* Volume must be in range */
  if(Volume<0) Volume=0; else if(Volume>255) Volume=255;
  /* Instrument number must be valid */
  if(MidiCH[Channel].Type<0) Freq=0;

  if(!Volume||!Freq) NoteOff(Channel);
  else
  {
    /* SND_TRIANGLE is twice quieter than SND_MELODIC */
    if(MidiCH[Channel].Type==SND_TRIANGLE) Volume=(Volume+1)/2;
    /* Compute MIDI note parameters */
    MIDIVolume = (127*Volume+128)/255;
    MIDINote   = Freqs[Freq/3].Note;
    MIDIWheel  = Freqs[Freq/3].Wheel;

    /* Play new note */
    NoteOn(Channel,MIDINote,MIDIVolume);

    /* Change pitch */
    if(MidiCH[Channel].Pitch!=MIDIWheel)
    {
      MIDIMessage(0xE0+SHIFT(Channel),MIDIWheel&0x7F,(MIDIWheel>>7)&0x7F);
      MidiCH[Channel].Pitch=MIDIWheel;
    }
  }
}

/** MIDISetSound() *******************************************/
/** Set sound type for a given channel.                     **/
/*************************************************************/
void MIDISetSound(int Channel,int Type)
{
  /* Channel must be valid */
  if((Channel>=MIDI_CHANNELS-1)||(Channel<0)) return;

  /* If instrument changed... */
  if(MidiCH[Channel].Type!=Type)
  {
    /* If logging off or file closed, drop out */
    if(!Logging||!MIDIOut) MidiCH[Channel].Type=Type|0x10000;
    else
    {
      MidiCH[Channel].Type=Type;
      if(Type<0) NoteOff(Channel);
      else
      {
        Type=Type&SND_MIDI? (Type&0x7F):Programs[Type%5];
        MIDIMessage(0xC0+SHIFT(Channel),Type,255);
      }
    }
  }
}

/** MIDIDrum() ***********************************************/
/** Hit a drum of a given type with given force.            **/
/*************************************************************/
void MIDIDrum(int Type,int Force)
{
  /* If logging off or invalid channel, drop out */
  if(!Logging||!MIDIOut) return;
  /* The only non-MIDI drum is a click ("Low Wood Block") */
  Type=Type&DRM_MIDI? (Type&0x7F):77;
  /* Release previous drum */
  if(DrumOn) MIDIMessage(0x89,DrumOn,127);
  /* Hit next drum */
  if(Type) MIDIMessage(0x99,Type,(Force&0xFF)/2);
  DrumOn=Type;
}

/** MIDIMessage() ********************************************/
/** Write out a MIDI message.                               **/
/*************************************************************/
void MIDIMessage(byte D0,byte D1,byte D2)
{
  /* Write number of ticks that passed */
  WriteDelta();

  /* Write out the command */
  if(D0!=LastMsg) { LastMsg=D0;fputc(D0,MIDIOut); }

  /* Write out the arguments */
  if(D1<128)
  {
    fputc(D1,MIDIOut);
    if(D2<128) fputc(D2,MIDIOut);
  }
}

/** NoteOn() *************************************************/
/** Turn on a note on a given channel.                      **/
/*************************************************************/
void NoteOn(byte Channel,byte Note,byte Level)
{
  Note  = Note>0x7F? 0x7F:Note;
  Level = Level>0x7F? 0x7F:Level;

  if((MidiCH[Channel].Note!=Note)||(MidiCH[Channel].Level!=Level))
  {
    if(MidiCH[Channel].Note>=0) NoteOff(Channel);
    MIDIMessage(0x90+SHIFT(Channel),Note,Level);
    MidiCH[Channel].Note=Note;
    MidiCH[Channel].Level=Level;
  }
}

/** NoteOff() ************************************************/
/** Turn off a note on a given channel.                     **/
/*************************************************************/
void NoteOff(byte Channel)
{
  if(MidiCH[Channel].Note>=0)
  {
    MIDIMessage(0x80+SHIFT(Channel),MidiCH[Channel].Note,127);
    MidiCH[Channel].Note=-1;
  }
}

/** WriteDelta() *********************************************/
/** Write number of ticks since the last MIDI command and   **/
/** reset the counter.                                      **/
/*************************************************************/
void WriteDelta(void)
{
  if(TickCount<128) fputc(TickCount,MIDIOut);
  else
  {
    if(TickCount<128*128)
    {
      fputc((TickCount>>7)|0x80,MIDIOut);
      fputc(TickCount&0x7F,MIDIOut);
    }
    else
    {
      fputc(((TickCount>>14)&0x7F)|0x80,MIDIOut);
      fputc(((TickCount>>7)&0x7F)|0x80,MIDIOut);
      fputc(TickCount&0x7F,MIDIOut);
    }
  }

  TickCount=0;
}

/** WriteTempo() *********************************************/
/** Write out soundtrack tempo (Hz).                        **/
/*************************************************************/
void WriteTempo(int Freq)
{
  int J;

  J=500000*MIDI_DIVISIONS*2/Freq;
  WriteDelta();
  fputc(0xFF,MIDIOut);
  fputc(0x51,MIDIOut);
  fputc(0x03,MIDIOut);
  fputc((J>>16)&0xFF,MIDIOut);
  fputc((J>>8)&0xFF,MIDIOut);
  fputc(J&0xFF,MIDIOut);
}

/** InitSound() **********************************************/
/** Initialize RenderSound() with given parameters.         **/
/*************************************************************/
unsigned int InitSound(unsigned int Rate,unsigned int Latency)
{
  int I;

  /* Shut down current sound */
  TrashSound();

  /* Initialize internal variables (keeping MasterVolume/MasterSwitch!) */
  SndRate = 0;

  /* Reset sound parameters */
  for(I=0;I<SND_CHANNELS;I++)
  {
    /* NOTICE: Preserving Type value! */
    WaveCH[I].Count  = 0;
    WaveCH[I].Volume = 0;
    WaveCH[I].Freq   = 0;
  }

  /* Initialize platform-dependent audio */
#if defined(WINDOWS)
  Rate = WinInitSound(Rate,Latency);
#else
  Rate = InitAudio(Rate,Latency);
#endif

  /* Rate=0 means silence */
  if(!Rate) { SndRate=0;return(0); }

  /* Done */
  SetChannels(MasterVolume,MasterSwitch);
  return(SndRate=Rate);
}

/** TrashSound() *********************************************/
/** Shut down RenderSound() driver.                         **/
/*************************************************************/
void TrashSound(void)
{
  /* Sound is now off */
  SndRate = 0;
  /* Shut down platform-dependent audio */
#if !defined(NO_AUDIO_PLAYBACK)
#if defined(WINDOWS)
  WinTrashSound();
#else
  TrashAudio();
#endif
#endif
}

#if !defined(NO_AUDIO_PLAYBACK)
/** RenderAudio() ********************************************/
/** Render given number of melodic sound samples into an    **/
/** integer buffer for mixing.                              **/
/*************************************************************/
void RenderAudio(int *Wave,unsigned int Samples)
{
  register int N,J,K,I,L,L1,L2,V,A1,A2;

  /* Exit if wave sound not initialized */
  if(SndRate<8192) return;

  /* Keep GCC happy about variable initialization */
  N=L=A2=0;
  /* Waveform generator */
  for(J=0;J<SND_CHANNELS;J++)
    if(WaveCH[J].Freq&&(V=WaveCH[J].Volume)&&(MasterSwitch&(1<<J)))
      switch(WaveCH[J].Type)
      {
        case SND_WAVE: /* Custom Waveform */
          /* Waveform data must have correct length! */
          if(WaveCH[J].Length<=0) break;
          /* Start counting */
          K  = WaveCH[J].Rate>0?
               (SndRate<<15)/WaveCH[J].Freq/WaveCH[J].Rate
             : (SndRate<<15)/WaveCH[J].Freq/WaveCH[J].Length;
          L1 = WaveCH[J].Pos%WaveCH[J].Length;
          L2 = WaveCH[J].Count;
          A1 = WaveCH[J].Data[L1]*V;
#ifdef NO_WAVE_INTERPOLATION
          /* Add waveform to the buffer */
          for(I=0;I<Samples;I++)
          {
            /* If next step... */
            if(L2>=K)
            {
              L1 = (L1+L2/K)%WaveCH[J].Length;
              A1 = WaveCH[J].Data[L1]*V;
              L2 = L2%K;
            }
            /* Output waveform */
            Wave[I]+=A1;
            /* Next waveform step */
            L2+=0x8000;
          }
#else
          /* If expecting interpolation... */
          if(L2<K)
          {
            /* Compute interpolation parameters */
            A2 = WaveCH[J].Data[(L1+1)%WaveCH[J].Length]*V;
            L  = (L2>>15)+1;
            N  = ((K-(L2&0x7FFF))>>15)+1;
          }
          /* Add waveform to the buffer */
          for(I=0;I<Samples;I++)
            if(L2<K)
            {
              /* Interpolate linearly */
              Wave[I]+=A1+L*(A2-A1)/N;
              /* Next waveform step */
              L2+=0x8000;
              /* Next interpolation step */
              L++;
            }
            else
            {
              L1 = (L1+L2/K)%WaveCH[J].Length;
              L2 = (L2%K)+0x8000;
              A1 = WaveCH[J].Data[L1]*V;
              Wave[I]+=A1;
              /* If expecting interpolation... */
              if(L2<K)
              {
                /* Compute interpolation parameters */
                A2 = WaveCH[J].Data[(L1+1)%WaveCH[J].Length]*V;
                L  = 1;
                N  = ((K-L2)>>15)+1;
              }
            }
#endif /* !NO_WAVE_INTERPOLATION */
          /* End counting */
          WaveCH[J].Pos   = L1;
          WaveCH[J].Count = L2;
          break;

        case SND_NOISE: /* White Noise */
          /* For high frequencies, recompute volume */
          if(WaveCH[J].Freq<SndRate)
            K=((unsigned int)WaveCH[J].Freq<<16)/SndRate;
          else
            K=0x10000;
          L1=WaveCH[J].Count;
          for(I=0;I<Samples;I++)
          {
            /* Use NoiseOut bit for output */
            Wave[I]+=((NoiseGen>>NoiseOut)&1? 127:-128)*V;
            L1+=K;
            if(L1&0xFFFF0000)
            {
              /* XOR NoiseOut and NoiseXOR bits and feed them back */
              NoiseGen=
                (((NoiseGen>>NoiseOut)^(NoiseGen>>NoiseXor))&1)
              | ((NoiseGen<<1)&((2<<NoiseOut)-1));
              L1&=0xFFFF;
            }
          }
          WaveCH[J].Count=L1;
          break;

        case SND_MELODIC:  /* Melodic Sound   */
        case SND_TRIANGLE: /* Triangular Wave */
        default:           /* Default Sound   */
          /* Do not allow frequencies that are too high */
          if(WaveCH[J].Freq>=SndRate/2) break;
          K=0x10000*WaveCH[J].Freq/SndRate;
          L1=WaveCH[J].Count;
#ifndef SLOW_MELODIC_AUDIO
          for(I=0;I<Samples;I++,L1+=K)
            Wave[I]+=((L1-K)^(L1+K))&0x8000? 0:(L1&0x8000? 127:-128)*V;
#else
          for(I=0;I<Samples;I++,L1+=K)
          {
            L2 = L1+K;
            A1 = L1&0x8000? 127:-128;
            if((L1^L2)&0x8000)
              A1=A1*(0x8000-(L1&0x7FFF)-(L2&0x7FFF))/K;
            Wave[I]+=A1*V;
          }
#endif
          WaveCH[J].Count=L1&0xFFFF;
          break;
      }
}

/** PlayAudio() **********************************************/
/** Normalize and play given number of samples from the mix **/
/** buffer. Returns the number of samples actually played.  **/
/*************************************************************/
unsigned int PlayAudio(int *Wave,unsigned int Samples)
{
  sample Buf[256];
  unsigned int I,J,K;
  int D;

  /* Exit if wave sound not initialized */
  if(SndRate<8192) return(0);

  /* Check if the buffer contains enough free space */
  J = GetFreeAudio();
  if(J<Samples) Samples=J;

  /* Spin until all samples played or WriteAudio() fails */
  for(K=I=J=0;(K<Samples)&&(I==J);K+=I)
  {
    /* Compute number of samples to convert */
    J = sizeof(Buf)/sizeof(sample);
    J = Samples-K>J? J:Samples-K;

    /* Convert samples */
    for(I=0;I<J;++I)
    {
      D      = ((*Wave++)*MasterVolume)>>8;
      D      = D>32767? 32767:D<-32768? -32768:D;
#if defined(BPU16)
      Buf[I] = D+32768;
#elif defined(BPS16)
      Buf[I] = D;
#elif defined(BPU8)
      Buf[I] = (D>>8)+128;
#else
      Buf[I] = D>>8;
#endif
    }

    /* Play samples */
    I = WriteAudio(Buf,J);
  }

  /* Return number of samples played */
  return(K);
}

/** RenderAndPlayAudio() *************************************/
/** Render and play a given number of samples. Returns the  **/
/** number of samples actually played.                      **/
/*************************************************************/
unsigned int RenderAndPlayAudio(unsigned int Samples)
{
  int Buf[256];
  unsigned int J,I;

  /* Exit if wave sound not initialized */
  if(SndRate<8192) return(0);

  J       = GetFreeAudio();
  Samples = Samples<J? Samples:J;
 
  /* Render and play sound */
  for(I=0;I<Samples;I+=J)
  {
    J = Samples-I;
    J = J<sizeof(Buf)/sizeof(Buf[0])? J:sizeof(Buf)/sizeof(Buf[0]);
    memset(Buf,0,J*sizeof(Buf[0]));
    RenderAudio(Buf,J);
    if(PlayAudio(Buf,J)<J) { I+=J;break; }
  }

  /* Return number of samples rendered */
  return(I);
}
#endif /* !NO_AUDIO_PLAYBACK */
