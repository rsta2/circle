/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        LibUnix.h                        **/
/**                                                         **/
/** This file contains Unix-dependent definitions and       **/
/** declarations for the emulation library.                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef LIBUNIX_H
#define LIBUNIX_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef MITSHM
#include <X11/extensions/XShm.h>
#endif

/* X11 defines "Status" to be "int" but this may shadow some */
/* of our application variables!                             */
#undef Status

#ifdef __cplusplus
extern "C" {
#endif

#define SND_CHANNELS    16     /* Number of sound channels   */
#define SND_BITS        8
#define SND_BUFSIZE     (1<<SND_BITS)

/** PIXEL() **************************************************/
/** Unix may use multiple pixel formats.                    **/
/*************************************************************/
#if defined(BPP32) || defined(BPP24)
#define PIXEL(R,G,B)  (pixel)(((int)R<<16)|((int)G<<8)|B)
#define PIXEL2MONO(P) (((P>>16)&0xFF)+((P>>8)&0xFF)+(P&0xFF))/3)
#define RMASK 0xFF0000
#define GMASK 0x00FF00
#define BMASK 0x0000FF

#elif defined(BPP16)
#define PIXEL(R,G,B)  (pixel)(((31*(R)/255)<<11)|((63*(G)/255)<<5)|(31*(B)/255))
#define PIXEL2MONO(P) (522*(((P)&31)+(((P)>>5)&63)+(((P)>>11)&31))>>8)
#define RMASK 0xF800
#define GMASK 0x07E0
#define BMASK 0x001F

#elif defined(BPP8)
#define PIXEL(R,G,B)  (pixel)(((7*(R)/255)<<5)|((7*(G)/255)<<2)|(3*(B)/255))
#define PIXEL2MONO(P) (3264*((((P)<<1)&7)+(((P)>>2)&7)+(((P)>>5)&7))>>8)
#define RMASK 0xE0
#define GMASK 0x1C
#define BMASK 0x03
#endif

int  ARGC;
char **ARGV;

/** InitUnix() ***********************************************/
/** Initialize Unix/X11 resources and set initial window.   **/
/** title and dimensions.                                   **/
/*************************************************************/
int InitUnix(const char *Title,int Width,int Height);

/** TrashUnix() **********************************************/
/** Free resources allocated in InitUnix()                  **/
/*************************************************************/
void TrashUnix(void);

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent).         **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate,unsigned int Latency);

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void);

/** PauseAudio() *********************************************/
/** Pause/resume audio playback.                            **/
/*************************************************************/
int PauseAudio(int Switch);

/** X11Window() **********************************************/
/** Open a window of a given size with a given title.       **/
/*************************************************************/
Window X11Window(const char *Title,int Width,int Height);

/** X11GetColor **********************************************/
/** Get pixel for the current screen depth based on the RGB **/
/** values.                                                 **/
/*************************************************************/
unsigned int X11GetColor(unsigned char R,unsigned char G,unsigned char B);

#ifdef __cplusplus
}
#endif
#endif /* LIBUNIX_H */
