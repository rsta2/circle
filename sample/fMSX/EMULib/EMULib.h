/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        EMULib.h                         **/
/**                                                         **/
/** This file contains platform-independent definitions and **/
/** declarations for the emulation library.                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef EMULIB_H
#define EMULIB_H

/** ARMScaleImage() Attributes *******************************/
/** These values can be ORed and passed to ARMScaleImage(). **/
/*************************************************************/
#define ARMSI_VFILL  0x01
#define ARMSI_HFILL  0x02

/** SetEffects() Attributes **********************************/
/** These values can be ORed and passed to SetEffects().    **/
/*************************************************************/
#define EFF_NONE       0x0000  /* No special effects         */
#define EFF_SCALE      0x0001  /* Scale video to fill screen */
#define EFF_SOFTEN     0x0002  /* Scale + soften video       */
#define EFF_TVLINES    0x0004  /* Apply TV scanlines effect  */
#define EFF_SAVECPU    0x0008  /* Suspend when inactive      */
#define EFF_SYNC       0x0010  /* Wait for sync timer        */
#define EFF_PENCUES    0x0020  /* Draw pen input cue lines   */
#define EFF_DIALCUES   0x0040  /* Draw dialpad (with PENCUES)*/
#define EFF_VERBOSE    0x0080  /* Report problems via printf */
#define EFF_STRETCH    0x0100  /* Stretch video, fill screen */
#define EFF_SHOWFPS    0x0200  /* Show frames-per-sec count  */
#define EFF_LCDLINES   0x0400  /* Apply LCD scanlines effect */
#define EFF_VKBD       0x0800  /* Draw virtual keyboard      */
#define EFF_SOFTEN2    0x1000  /* Softening algorithm select */
#define EFF_FULLJOY    0x2000  /* Use full screen controls   */
#define EFF_TILTJOY    0x4000  /* Use accelerometer controls */
#define EFF_PENJOY     0x8000  /* Use touchpad controls      */
#define EFF_VSYNC     0x10000  /* Wait for VBlanks           */
#define EFF_DIRECT    0x20000  /* Copy whole VideoImg        */
#define EFF_CMYMASK   0x40000  /* Apply vertical CMY mask    */
#define EFF_RGBMASK   0x80000  /* Apply vertical RGB mask    */
#if defined(ANDROID)
#define EFF_FIXFFWD  0x100000  /* Persistent FFWD button     */
#define EFF_GLES     0x200000  /* OpenGLES video rendering   */
#define EFF_EXTVKBD  0x400000  /* Java virtual keyboard      */
#define EFF_LOCKED 0x80000000  /* SetEffects() now disabled  */
#elif defined(MAEMO)
#define EFF_NOVOLUME 0x100000  /* No volume control on F7/F8 */
#define EFF_FULLSCR  0x200000  /* Force full-screen mode     */
#define EFF_WINDOWED 0x400000  /* Force windowed mode        */
#define EFF_HAA      0x800000  /* Use HildonAnimationActor   */
#define EFF_ALWAYS1  0x80000000    /* 1: InitMaemo() success */
#elif defined(S60) || defined(UIQ)
#define EFF_FILL     0x100000  /* Fill display vertically    */
#define EFF_LIGHT    0x200000  /* Keep backlight on          */
#define EFF_USEHAL   0x400000  /* Use direct image rendering without DSB */
#elif defined(UNIX)
#define EFF_MITSHM   0x100000  /* Use MITSHM X11 extension   */
#define EFF_VARBPP   0x200000  /* X11 determines Image depth */
#elif defined(CIRCLE)
#define EFF_VARBPP   0x200000  /* X11 determines Image depth */
#endif

#define EFF_2XSAL    (EFF_SOFTEN)
#define EFF_EPX      (EFF_SOFTEN2)
#define EFF_EAGLE    (EFF_SOFTEN|EFF_SOFTEN2)

/** Button Bits **********************************************/
/** Bits returned by GetJoystick() and WaitJoystick().      **/
/*************************************************************/
#define BTN_LEFT     0x0001
#define BTN_RIGHT    0x0002
#define BTN_UP       0x0004
#define BTN_DOWN     0x0008
#define BTN_FIREA    0x0010
#define BTN_FIREB    0x0020
#define BTN_FIREL    0x0040
#define BTN_FIRER    0x0080
#define BTN_START    0x0100
#define BTN_SELECT   0x0200
#define BTN_EXIT     0x0400
#define BTN_FIREX    0x0800
#define BTN_FIREY    0x1000
#define BTN_FFWD     0x2000
#define BTN_MENU     0x4000
#define BTN_ALL      0x7FFF
#define BTN_OK       (BTN_FIREA|BTN_START)
#define BTN_FIRE     (BTN_FIREA|BTN_FIREB|BTN_FIREL|BTN_FIRER|BTN_FIREX|BTN_FIREY)
#define BTN_ARROWS   (BTN_LEFT|BTN_RIGHT|BTN_UP|BTN_DOWN)
#define BTN_SHIFT    CON_SHIFT
#define BTN_CONTROL  CON_CONTROL
#define BTN_ALT      CON_ALT
#define BTN_MODES    (BTN_SHIFT|BTN_CONTROL|BTN_ALT)

/** Mouse Bits ***********************************************/
/** Bits returned by GetMouse() and WaitKeyOrMouse().       **/
/*************************************************************/
#define MSE_RIGHT    0x80000000
#define MSE_LEFT     0x40000000
#define MSE_BUTTONS  (MSE_RIGHT|MSE_LEFT)
#define MSE_YPOS     0x3FFF0000
#define MSE_XPOS     0x0000FFFF

/** Special Key Codes ****************************************/
/** Modifiers returned by GetKey() and WaitKey().           **/
/*************************************************************/
#define CON_KEYCODE  0x03FFFFFF /* Key code                  */
#define CON_MODES    0xFC000000 /* Mode bits, as follows:    */
#define CON_CLICK    0x04000000 /* Key click (LiteS60 only)  */
#define CON_CAPS     0x08000000 /* CapsLock held             */
#define CON_SHIFT    0x10000000 /* SHIFT held                */
#define CON_CONTROL  0x20000000 /* CONTROL held              */
#define CON_ALT      0x40000000 /* ALT held                  */
#define CON_RELEASE  0x80000000 /* Key released (going up)   */

#define CON_F1       0xEE
#define CON_F2       0xEF
#define CON_F3       0xF0
#define CON_F4       0xF1
#define CON_F5       0xF2
#define CON_F6       0xF3
#define CON_F7       0xF4
#define CON_F8       0xF5
#define CON_F9       0xF6
#define CON_F10      0xF7
#define CON_F11      0xF8
#define CON_F12      0xF9
#define CON_LEFT     0xFA
#define CON_RIGHT    0xFB
#define CON_UP       0xFC
#define CON_DOWN     0xFD
#define CON_OK       0xFE
#define CON_EXIT     0xFF

#ifdef WINDOWS
#include "LibWin.h"
#endif
#ifdef MSDOS
#include "LibMSDOS.h"
#endif
#ifdef UNIX
#include "LibUnix.h"
#endif
#ifdef MAEMO
#define ARM_CPU
#include "LibMaemo.h"
#endif
#ifdef MEEGO
#define ARM_CPU
#include "LibMeego.h"
#endif
#ifdef NXC2600
#include "LibNXC2600.h"
#endif
#ifdef STMP3700
#define ARM_CPU
#include "LibSTMP3700.h"
#endif
#ifdef ANDROID
#include "LibAndroid.h"
#endif
#ifdef IOS
#define ARM_CPU
#include "LibApple.h"
#endif
#if defined(S60) || defined(UIQ)
#define ARM_CPU
#include "LibSym.h"
#include "LibSym.rh"
#endif
#if defined(CIRCLE)
#define ARM_CPU
#include "LibCircle.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** pixel ****************************************************/
/** Pixels may be either 8bit, or 16bit, or 32bit. When no  **/
/** BPP* specified, we assume the pixel to have the largest **/
/** size and default to GetColor().                         **/
/*************************************************************/
#ifndef PIXEL_TYPE_DEFINED
#define PIXEL_TYPE_DEFINED
#if defined(BPP32) || defined(BPP24)
typedef unsigned int pixel;
#elif defined(BPP16)
typedef unsigned short pixel;
#elif defined(BPP8)
typedef unsigned char pixel;
#else
typedef unsigned int pixel;
#define PIXEL(R,G,B) GetColor(R,G,B)
#endif
#endif

/** sample ***************************************************/
/** Audio samples may be either 8bit or 16bit.              **/
/*************************************************************/
#ifndef SAMPLE_TYPE_DEFINED
#define SAMPLE_TYPE_DEFINED
#ifdef BPS16
typedef signed short sample;
#else
typedef signed char sample;
#endif
#endif

/** Image ****************************************************/
/** This data type encapsulates a bitmap.                   **/
/*************************************************************/
typedef struct
{
  pixel *Data;               /* Buffer containing WxH pixels */
  int W,H,L,D;               /* Image size, pitch, depth     */
  char Cropped;              /* 1: Cropped, do not free()    */
#ifdef WINDOWS
  HDC hDC;                   /* Handle to device context     */
  HBITMAP hBMap;             /* Handle to bitmap             */
#endif
#ifdef MAEMO
  GdkImage *GImg;            /* Pointer to GdkImage object   */
#endif
#ifdef MEEGO
  QImage *QImg;              /* Pointer to QImage object     */
#endif
#ifdef UNIX
  XImage *XImg;              /* Pointer to XImage structure  */
  int Attrs;                 /* USE_SHM and other attributes */
#ifdef MITSHM
  XShmSegmentInfo SHMInfo;   /* Shared memory information    */
#endif
#endif
} Image;

/** Current Video Image **************************************/
/** These parameters are set with SetVideo() and used by    **/
/** ShowVideo() to show a WxH fragment from <X,Y> of Img.   **/
/*************************************************************/
extern Image *VideoImg;        /* Current ShowVideo() image  */
extern int VideoX;             /* X for ShowVideo()          */
extern int VideoY;             /* Y for ShowVideo()          */
extern int VideoW;             /* Width for ShowVideo()      */
extern int VideoH;             /* Height for ShowVideo()     */

/** KeyHandler ***********************************************/
/** This function receives key presses and releases.        **/
/*************************************************************/
extern void (*KeyHandler)(unsigned int Key);

/** NewImage() ***********************************************/
/** Create a new image of the given size. Returns pointer   **/
/** to the image data on success, 0 on failure.             **/
/*************************************************************/
pixel *NewImage(Image *Img,int Width,int Height);

/** FreeImage() **********************************************/
/** Free previously allocated image.                        **/
/*************************************************************/
void FreeImage(Image *Img);

/** CropImage() **********************************************/
/** Create a subimage Dst of the image Src. Returns Dst.    **/
/*************************************************************/
Image *CropImage(Image *Dst,const Image *Src,int X,int Y,int W,int H);

#if defined(WINDOWS) || defined(UNIX) || defined(MAEMO) || defined(MEEGO)
Image *GenericCropImage(Image *Dst,const Image *Src,int X,int Y,int W,int H);
#endif

/** ScaleImage() *********************************************/
/** Copy Src into Dst, scaling as needed.                   **/
/*************************************************************/
void ScaleImage(Image *Dst,const Image *Src,int X,int Y,int W,int H);

/** ARMScaleImage() ******************************************/
/** Copy Src into Dst using ARM-optimized assembly code.    **/
/** Returns 0 if this function is not supported or there is **/
/** an alignment problem. Returns destination height and    **/
/** width on success in <Height 31:16><Width 15:0> format.  **/
/*************************************************************/
unsigned int ARMScaleImage(Image *Dst,Image *Src,int X,int Y,int W,int H,int Attrs);

/** TelevizeImage() ******************************************/
/** Create televizion effect on the image.                  **/
/*************************************************************/
void TelevizeImage(Image *Img,int X,int Y,int W,int H);

/** LcdizeImage() ********************************************/
/** Create LCD effect on the image.                         **/
/*************************************************************/
void LcdizeImage(Image *Img,int X,int Y,int W,int H);

/** RasterizeImage() *****************************************/
/** Create raster effect on the image.                      **/
/*************************************************************/
void RasterizeImage(Image *Img,int X,int Y,int W,int H);

/** CMYizeImage() ********************************************/
/** Apply vertical CMY stripes to the image.                **/
/*************************************************************/
void CMYizeImage(Image *Img,int X,int Y,int W,int H);

/** RGBizeImage() ********************************************/
/** Apply vertical RGB stripes to the image.                **/
/*************************************************************/
void RGBizeImage(Image *Img,int X,int Y,int W,int H);

/** SoftenImage() ********************************************/
/** Uses softening algorithm to interpolate image Src into  **/
/** a bigger image Dst.                                     **/
/*************************************************************/
void SoftenImage(Image *Dst,const Image *Src,int X,int Y,int W,int H);

/** SoftenEPX() **********************************************/
/** Uses EPX softening algorithm to interpolate image Src   **/
/** into a bigger image Dst.                                **/
/*************************************************************/
void SoftenEPX(Image *Dst,const Image *Src,int X,int Y,int W,int H);

/** SoftenEAGLE() ********************************************/
/** Uses EAGLE softening algorithm to interpolate image Src **/
/** into a bigger image Dst.                                **/
/*************************************************************/
void SoftenEAGLE(Image *Dst,const Image *Src,int X,int Y,int W,int H);

/** ClearImage() *********************************************/
/** Clear image with a given color.                         **/
/*************************************************************/
void ClearImage(Image *Img,pixel Color);

/** IMGCopy() ************************************************/
/** Copy one image into another. Skips pixels of given      **/
/** color unless TColor=-1.                                 **/
/*************************************************************/
void IMGCopy(Image *Dst,int DX,int DY,const Image *Src,int SX,int SY,int W,int H,int TColor);

/** IMGDrawRect()/IMGFillRect() ******************************/
/** Draw filled/unfilled rectangle in a given image.        **/
/*************************************************************/
void IMGDrawRect(Image *Img,int X,int Y,int W,int H,pixel Color);
void IMGFillRect(Image *Img,int X,int Y,int W,int H,pixel Color);

/** IMGPrint() ***********************************************/
/** Print text in a given image.                            **/
/*************************************************************/
//@@@ NOT YET
//void IMGPrint(Image *Img,const char *S,int X,int Y,int FG,int BG);

/** SetVideo() ***********************************************/
/** Set part of the image as "active" for display.          **/
/*************************************************************/
void SetVideo(Image *Img,int X,int Y,int W,int H);

#if defined(UNIX) || defined(MAEMO) || defined(MEEGO) || defined(ANDROID)
void GenericSetVideo(Image *Img,int X,int Y,int W,int H);
#endif

/** ShowVideo() **********************************************/
/** Show "active" image at the actual screen or window.     **/
/*************************************************************/
int ShowVideo(void);

/** GetColor() ***********************************************/
/** Return pixel corresponding to the given <R,G,B> value.  **/
/** This only works for non-palletized modes.               **/
/*************************************************************/
pixel GetColor(unsigned char R,unsigned char G,unsigned char B);

/** SetPalette() *********************************************/
/** Set color N to the given <R,G,B> value. This only works **/
/** for palette-based modes.                                **/
/*************************************************************/
void SetPalette(pixel N,unsigned char R,unsigned char G,unsigned char B);

/** GetFreeAudio() *******************************************/
/** Get the amount of free samples in the audio buffer.     **/
/*************************************************************/
unsigned int GetFreeAudio(void);

/** GetTotalAudio() ******************************************/
/** Get total amount of samples in the audio buffer.        **/
/*************************************************************/
unsigned int GetTotalAudio(void);

/** WriteAudio() *********************************************/
/** Write up to a given number of samples to audio buffer.  **/
/** Returns the number of samples written.                  **/
/*************************************************************/
unsigned int WriteAudio(sample *Data,unsigned int Length);

/** PauseAudio() *********************************************/
/** Pause/resume audio playback. Returns current playback   **/
/** state.                                                  **/
/************************************************* OPTIONAL **/
int PauseAudio(int Switch);

/** GetJoystick() ********************************************/
/** Get the state of joypad buttons (1="pressed"). Refer to **/
/** the BTN_* #defines for the button mappings. Notice that **/
/** on Windows this function calls ProcessEvents() thus    **/
/** automatically handling all Windows messages.            **/
/*************************************************************/
unsigned int GetJoystick(void);

/** WaitJoystick() *******************************************/
/** Wait until one or more of the given buttons have been   **/
/** pressed. Returns the bitmask of pressed buttons. Refer  **/
/** to BTN_* #defines for the button mappings.              **/
/*************************************************************/
unsigned int WaitJoystick(unsigned int Mask);

/** GetMouse() ***********************************************/
/** Get mouse position and button states in the following   **/
/** format: RMB.LMB.Y[29-16].X[15-0]                        **/
/*************************************************************/
unsigned int GetMouse(void);

/** GetKey() *************************************************/
/** Get currently pressed key or 0 if none pressed. Returns **/
/** CON_* definitions for arrows and special keys.          **/
/*************************************************************/
unsigned int GetKey(void);

/** WaitKey() ************************************************/
/** Wait for a key to be pressed. Returns CON_* definitions **/
/** for arrows and special keys.                            **/
/*************************************************************/
unsigned int WaitKey(void);

/** WaitKeyOrMouse() *****************************************/
/** Wait for a key or a mouse button to be pressed. Returns **/
/** the same result as GetMouse(). If no mouse buttons      **/
/** reported to be pressed, call GetKey() to fetch a key.   **/
/*************************************************************/
unsigned int WaitKeyOrMouse(void);

/** SetKeyHandler() ******************************************/
/** Attach keyboard handler that will be called when a key  **/
/** is pressed or released.                                 **/
/*************************************************************/
void SetKeyHandler(void (*Handler)(unsigned int Key));

/** WaitSyncTimer() ******************************************/
/** Wait for the timer to become ready. Returns number of   **/
/** times timer has been triggered after the last call to   **/
/** WaitSyncTimer().                                        **/
/*************************************************************/
int WaitSyncTimer(void);

/** SyncTimerReady() *****************************************/
/** Return 1 if sync timer ready, 0 otherwise.              **/
/*************************************************************/
int SyncTimerReady(void);

/** SetSyncTimer() *******************************************/
/** Set synchronization timer to a given frequency in Hz.   **/
/*************************************************************/
int SetSyncTimer(int Hz);

/** ProcessEvents() ******************************************/
/** Process UI (X11,GTK,Qt,...) event messages. When Wait=1 **/
/** and there are no messages, wait for some. Returns 1 for **/
/** continued execution, 0 if application has been closed.  **/
/*************************************************************/
int ProcessEvents(int Wait);

/** SetEffects() *********************************************/
/** Set visual effects applied to video in ShowVideo().     **/
/*************************************************************/
void SetEffects(unsigned int NewEffects);

/** GetFilePath() ********************************************/
/** Extracts pathname from filename and returns a pointer   **/
/** to the internal buffer containing just the path name    **/
/** ending with "\".                                        **/
/*************************************************************/
const char *GetFilePath(const char *Name);

/** NewFile() ************************************************/
/** Given pattern NAME.EXT, generates a new filename in the **/
/** NAMEnnnn.EXT (nnnn = 0000..9999) format and returns a   **/
/** pointer to the internal buffer containing new filename. **/
/*************************************************************/
const char *NewFile(const char *Pattern);

/** ChangeDir() **********************************************/
/** This function is a wrapper for chdir(). Unlike chdir(), **/
/** it changes *both* the drive and the directory in MSDOS, **/
/** exactly like the "normal" chdir() should. Returns 0 on  **/
/** success, -1 on failure, just like chdir().              **/
/*************************************************************/
int ChangeDir(const char *Name);

/** MicroSleep() *********************************************/
/** Wait for a given number of microseconds.                **/
/*************************************************************/
void MicroSleep(unsigned int uS);

#ifdef GIFLIB
/** LoadGIF() ************************************************/
/** Load screen from .GIF file. Returns the number of used  **/
/** colors or 0 if failed.                                  **/
/*************************************************************/
int LoadGIF(const char *File);

/** SaveGIF() ************************************************/
/** Save screen to .GIF file. Returns the number of written **/
/** scanlines or 0 if failed.                               **/
/*************************************************************/
int SaveGIF(const char *File);
#endif /* GIFLIB */

#ifdef LIBPNG
/** LoadPNG() ************************************************/
/** Load image from a .PNG file. Returns 1 on success, 0 on **/
/** failure.                                                **/
/*************************************************************/
int LoadPNG(Image *Img,const char *FileName);
#endif

#ifdef __cplusplus
}
#endif
#endif /* EMULIB_H */
