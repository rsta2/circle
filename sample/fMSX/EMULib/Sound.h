/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          Sound.h                        **/
/**                                                         **/
/** This file defines standard sound generation API and     **/
/** functions needed to log soundtrack into a MIDI file.    **/
/** See Sound.c and the sound drivers for the code.         **/ 
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef SOUND_H
#define SOUND_H

#include "EMULib.h"

#ifdef __cplusplus
extern "C" {
#endif

                               /* SetSound() arguments:      */
#define SND_MELODIC     0      /* Melodic sound (default)    */
#define SND_RECTANGLE   0      /* Rectangular wave           */
#define SND_TRIANGLE    1      /* Triangular wave (1/2 rect.)*/
#define SND_NOISE       2      /* White noise                */
#define SND_PERIODIC    3      /* Periodic noise (not im-ed) */
#define SND_WAVE        4      /* Wave sound set by SetWave()*/
#define SND_MIDI        0x100  /* MIDI instrument (ORable)   */

                               /* Drum() arguments:          */
#define DRM_CLICK       0      /* Click (default)            */
#define DRM_MIDI        0x100  /* MIDI drum (ORable)         */

                               /* MIDI characteristics:      */
#define MIDI_CHANNELS   16     /* Number of MIDI channels    */
#define MIDI_MINFREQ    9      /* Min MIDI frequency (Hz)    */
#define MIDI_MAXFREQ    12285  /* Max MIDI frequency (Hz)    */
#define MIDI_DIVISIONS  1000   /* Number of ticks per second */

                               /* MIDILogging() arguments:   */
#define MIDI_OFF        0      /* Turn MIDI logging off      */
#define MIDI_ON         1      /* Turn MIDI logging on       */
#define MIDI_TOGGLE     2      /* Toggle MIDI logging        */
#define MIDI_QUERY      3      /* Query MIDI logging status  */

/** InitSound() **********************************************/
/** Initialize RenderSound() with given parameters.         **/
/*************************************************************/
unsigned int InitSound(unsigned int Rate,unsigned int Latency);

/** TrashSound() *********************************************/
/** Shut down RenderSound() driver.                         **/
/*************************************************************/
void TrashSound(void);

/** RenderAudio() ********************************************/
/** Render given number of melodic sound samples into an    **/
/** integer buffer for mixing.                              **/
/*************************************************************/
void RenderAudio(int *Wave,unsigned int Samples);

/** PlayAudio() **********************************************/
/** Normalize and play given number of samples from the mix **/
/** buffer. Returns the number of samples actually played.  **/
/*************************************************************/
unsigned int PlayAudio(int *Wave,unsigned int Samples);

/** RenderAndPlayAudio() *************************************/
/** Render and play a given number of samples. Returns the  **/
/** number of samples actually played.                      **/
/*************************************************************/
unsigned int RenderAndPlayAudio(unsigned int Samples);

/** Sound() **************************************************/
/** Generate sound of given frequency (Hz) and volume       **/
/** (0..255) via given channel. Setting Freq=0 or Volume=0  **/
/** turns sound off.                                        **/
/*************************************************************/
void Sound(int Channel,int Freq,int Volume);

/** Drum() ***************************************************/
/** Hit a drum of given type with given force (0..255).     **/
/** MIDI drums can be used by ORing their numbers with      **/
/** SND_MIDI.                                               **/
/*************************************************************/
void Drum(int Type,int Force);

/** SetSound() ***********************************************/
/** Set sound type at a given channel. MIDI instruments can **/
/** be set directly by ORing their numbers with SND_MIDI.   **/
/*************************************************************/
void SetSound(int Channel,int NewType);

/** SetChannels() ********************************************/
/** Set master volume (0..255) and switch channels on/off.  **/
/** Each channel N has corresponding bit 2^N in Switch. Set **/
/** or reset this bit to turn the channel on or off.        **/ 
/*************************************************************/
void SetChannels(int Volume,int Switch);

/** SetNoise() ***********************************************/
/** Initialize random noise generator to the given Seed and **/
/** then take random output from OUTBit and XOR it with     **/
/** XORBit.                                                 **/
/*************************************************************/
void SetNoise(int Seed,int OUTBit,int XORBit);

/** SetWave() ************************************************/
/** Set waveform for a given channel. The channel will be   **/
/** marked with sound type SND_WAVE. Set Rate=0 if you want **/
/** waveform to be an instrument or set it to the waveform  **/
/** own playback rate.                                      **/
/*************************************************************/
void SetWave(int Channel,const signed char *Data,int Length,int Rate);

/** GetWave() ************************************************/
/** Get current read position for the buffer set with the   **/
/** SetWave() call. Returns 0 if no buffer has been set, or **/
/** if there is no playrate set (i.e. wave is instrument).  **/
/*************************************************************/
const signed char *GetWave(int Channel);

/** GetSndRate() *********************************************/
/** Get current sampling rate used for synthesis.           **/
/*************************************************************/
unsigned int GetSndRate(void);

/** InitMIDI() ***********************************************/
/** Initialize soundtrack logging into MIDI file FileName.  **/
/** Repeated calls to InitMIDI() will close current MIDI    **/
/** file and continue logging into a new one.               **/ 
/*************************************************************/
void InitMIDI(const char *FileName);

/** TrashMIDI() **********************************************/
/** Finish logging soundtrack and close the MIDI file.      **/
/*************************************************************/
void TrashMIDI(void);

/** MIDILogging() ********************************************/
/** Turn soundtrack logging on/off and return its current   **/
/** status. Possible values of Switch are MIDI_OFF (turn    **/
/** logging off), MIDI_ON (turn logging on), MIDI_TOGGLE    **/
/** (toggle logging), and MIDI_QUERY (just return current   **/
/** state of logging).                                      **/
/*************************************************************/
int MIDILogging(int Switch);

/** MIDITicks() **********************************************/
/** Log N 1ms MIDI ticks.                                   **/
/*************************************************************/
void MIDITicks(int N);

#if !defined(MSDOS) & !defined(UNIX) & !defined(MAEMO) & !defined(WINDOWS) & !defined(S60) & !defined(UIQ) && !defined(ANDROID)
#define SND_CHANNELS MIDI_CHANNELS         /* Default number */
#endif

/** SndDriver ************************************************/
/** Each sound driver should fill this structure with       **/
/** pointers to hardware-dependent handlers. This has to be **/
/** done inside the InitSound() function.                   **/
/*************************************************************/
struct SndDriverStruct
{
  void (*SetSound)(int Channel,int NewType);
  void (*Drum)(int Type,int Force);
  void (*SetChannels)(int Volume,int Switch);
  void (*Sound)(int Channel,int NewFreq,int NewVolume);
  void (*SetWave)(int Channel,const signed char *Data,int Length,int Freq);
  const signed char *(*GetWave)(int Channel);
};
extern struct SndDriverStruct SndDriver;

#ifdef __cplusplus
}
#endif
#endif /* SOUND_H */
