/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         Touch.h                         **/
/**                                                         **/
/** This file declares functions that simulate joystick and **/
/** dialpad with the touch screen.                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2008-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef TOUCH_H
#define TOUCH_H

#include "EMULib.h"

/* SetFinButton() special mask bits **************************/
#define BTN_INVISIBLE 0x80000000

/* GenericPenDialpad() result ********************************/
#define DIAL_NONE         0    /* No dialpad buttons pressed */
#define DIAL_1            1
#define DIAL_2            2
#define DIAL_3            3
#define DIAL_4            4
#define DIAL_5            5
#define DIAL_6            6
#define DIAL_7            7
#define DIAL_8            8
#define DIAL_9            9
#define DIAL_STAR        10
#define DIAL_0           11
#define DIAL_POUND       12

#ifdef __cplusplus
extern "C" {
#endif

/** RenderVideo() ********************************************/
/** Draw video buffer to a given image. Return 0 on failure **/
/** or destination rectangle size inside OutImg on success. **/
/*************************************************************/
unsigned int RenderVideo(Image *OutImg,Image *CueImg,int Effects,int PenKey,int FrameRate);

/** GenericFullJoystick() ************************************/
/** Treat whole screen as one big directional pad. Result   **/
/** compatible with GetJoystick() (arrows only though).     **/
/*************************************************************/
unsigned int GenericFullJoystick(int X,int Y,int W,int H);

/** GenericPenJoystick() *************************************/
/** Get simulated joystick buttons from touch screen UI.    **/
/** Result compatible with GetJoystick().                   **/
/*************************************************************/
unsigned int GenericPenJoystick(int X,int Y,int W,int H);

/** GenericPenDialpad() **************************************/
/** Get simulated dialpad buttons from touch screen UI.     **/
/*************************************************************/
unsigned char GenericPenDialpad(int X,int Y,int W,int H);

/** GenericPenKeyboard() *************************************/
/** Get virtual on-screen keyboard buttons.                 **/
/*************************************************************/
unsigned char GenericPenKeyboard(int X,int Y,int W,int H);

/** GenericDialKeyboard() ************************************/
/** Process dialpad input to the virtual keyboard. Returns  **/
/** virtual keyboard key if selected, or 0 if not.          **/
/*************************************************************/
unsigned char GenericDialKeyboard(unsigned char Key);

/** GenericFinJoystick() *************************************/
/** Return the BTN_* bits corresponding to position X,Y of  **/
/** the finger joystick shown in Dst.                       **/
/*************************************************************/
unsigned int GenericFinJoystick(int X,int Y,int W,int H,unsigned int CurState);

/** SetPenCues() *********************************************/
/** Set pen cues for given buttons to a given string.       **/
/*************************************************************/
void SetPenCues(unsigned int Buttons,const char *CueText);

/** SetPenKeyboard() *****************************************/
/** Set pen keyboard dimensions.                            **/
/*************************************************************/
void SetPenKeyboard(unsigned int KeyStep,unsigned int KeySize,unsigned int ChrSize);

/** InitFinJoystick() ****************************************/
/** Initialize finger joystick images by cropping them from **/
/** the given source image. Returns number of buttons set   **/
/** successfully (i.e. found inside the Src bounds).        **/
/*************************************************************/
int InitFinJoystick(const Image *Src);

/** SetFinButton() *******************************************/
/** Set finger joystick button(s) to given location. When   **/
/** Img=0, create wireframe buttons. When Mask=0, set the   **/
/** directional buttons image and location. When Mask ORed  **/
/** with BTN_INVISIBLE, create invisible buttons. Returns   **/
/** the number of virtual buttons set or 0 for none.        **/
/*************************************************************/
int SetFinButton(unsigned int Mask,const Image *Src,int X,int Y,int W,int H);

/** DrawPenCues() ********************************************/
/** Overlay dotted cue lines for using PenJoystick() onto a **/
/** given image. Show dialpad cues if requested.            **/
/*************************************************************/
void DrawPenCues(Image *Img,int ShowDialpad,int Color);

/** DrawDialpad() ********************************************/
/** Draw virtual dialpad in a given image.                  **/
/*************************************************************/
void DrawDialpad(Image *Img,int Color);

/** DrawKeyboard() *******************************************/
/** Draw virtual keyboard in a given image. Key modifiers   **/
/** and the key code passed in CurKey are highlighted.      **/
/*************************************************************/
void DrawKeyboard(Image *Img,unsigned int CurKey);

/** DrawFinJoystick() ****************************************/
/** Draw finger joystick into given destination image. When **/
/** DW=0 or DWH=0, the whole image will be updated. When    **/
/** DWxDH+DX+DY represent a dirty rectangle inside Dst, the **/
/** function will only update joystick buttons overlapping  **/
/** this rectangle, representing them with dotted lines.    **/
/** Returns the number of buttons overlapping the dirty     **/
/** rectangle.                                              **/
/*************************************************************/
int DrawFinJoystick(Image *Dst,int DX,int DY,int DW,int DH,int TextColor);

/** GetKbdWidth()/GetKbdHeight() *****************************/
/** Return virtual keyboard dimensions.                     **/
/*************************************************************/
unsigned int GetKbdWidth();
unsigned int GetKbdHeight();

#ifdef __cplusplus
}
#endif
#endif /* TOUCH_H */
