/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        TouchMux.h                       **/
/**                                                         **/
/** This file wraps Touch.c routines for multiple display   **/
/** depths (BPP8,BPP16,BPP32). It is used automatically     **/
/** when none of BPP* values are defined.                   **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2008-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef TOUCHMUX_H
#define TOUCHMUX_H

#include "EMULib.h"
#include "Touch.h"

#undef BPP8
#undef BPP16
#undef BPP24
#undef BPP32
#undef PIXEL

#define BPP8
#define pixel           unsigned char
#define PIXEL(R,G,B)    (pixel)(((7*(R)/255)<<5)|((7*(G)/255)<<2)|(3*(B)/255))
#define DrawHLine       DrawHLine_8
#define DrawVLine       DrawVLine_8
#define DrawPenCues     DrawPenCues_8
#define DrawDialpad     DrawDialpad_8
#define DrawKeyboard    DrawKeyboard_8
#define DrawFinJoystick DrawFinJoystick_8
#define RenderVideo     RenderVideo_8
#define PrintXY         PrintXY_8
#define PrintXY2        PrintXY2_8
#include "Touch.c"
#undef pixel
#undef PIXEL
#undef DrawHLine
#undef DrawVLine
#undef DrawPenCues
#undef DrawDialpad
#undef DrawKeyboard
#undef DrawFinJoystick
#undef RenderVideo
#undef PrintXY
#undef PrintXY2
#undef BPP8

#define BPP16
#define pixel           unsigned short
#if defined(UNIX) || defined(ANDROID) || defined(S60) || defined(UIQ) || defined(NXC2600) || defined(STMP3700)
/* Symbian and Unix use true 16BPP color */
#define PIXEL(R,G,B)    (pixel)(((31*(R)/255)<<11)|((63*(G)/255)<<5)|(31*(B)/255))
#else
/* Other platforms use 15BPP color */
#define PIXEL(R,G,B)    (pixel)(((31*(R)/255)<<10)|((31*(G)/255)<<5)|(31*(B)/255))
#endif	 
#define DrawHLine       DrawHLine_16
#define DrawVLine       DrawVLine_16
#define DrawPenCues     DrawPenCues_16
#define DrawDialpad     DrawDialpad_16
#define DrawKeyboard    DrawKeyboard_16
#define DrawFinJoystick DrawFinJoystick_16
#define RenderVideo     RenderVideo_16
#define PrintXY         PrintXY_16
#define PrintXY2        PrintXY2_16
#include "Touch.c"
#undef pixel
#undef PIXEL
#undef DrawHLine
#undef DrawVLine
#undef DrawPenCues
#undef DrawDialpad
#undef DrawKeyboard
#undef DrawFinJoystick
#undef RenderVideo
#undef PrintXY
#undef PrintXY2
#undef BPP16

#define BPP32
#define pixel           unsigned int
#if defined(ANDROID)
#define PIXEL(R,G,B)    (pixel)(((int)R<<16)|((int)G<<8)|B|0xFF000000)
#else
#define PIXEL(R,G,B)    (pixel)(((int)R<<16)|((int)G<<8)|B)
#endif
#define DrawHLine       DrawHLine_32
#define DrawVLine       DrawVLine_32
#define DrawPenCues     DrawPenCues_32
#define DrawDialpad     DrawDialpad_32
#define DrawKeyboard    DrawKeyboard_32
#define DrawFinJoystick DrawFinJoystick_32
#define RenderVideo     RenderVideo_32
#define PrintXY         PrintXY_32
#define PrintXY2        PrintXY2_32
#include "Touch.c"
#undef pixel
#undef PIXEL
#undef DrawHLine
#undef DrawVLine
#undef DrawPenCues
#undef DrawDialpad
#undef DrawKeyboard
#undef DrawFinJoystick
#undef RenderVideo
#undef PrintXY
#undef PrintXY2
#undef BPP32

/** DrawPenCues() ********************************************/
/** Overlay dotted cue lines for using PenJoystick() onto a **/
/** given image. Show dialpad cues if requested.            **/
/*************************************************************/
void DrawPenCues(Image *Img,int ShowDialpad,int Color)
{
  switch(Img->D)
  {
    case 8:  DrawPenCues_8(Img,ShowDialpad,Color);break;
    case 16: DrawPenCues_16(Img,ShowDialpad,Color);break;
    case 24:
    case 32: DrawPenCues_32(Img,ShowDialpad,Color);break;
  }
}

/** DrawDialpad() ********************************************/
/** Draw virtual dialpad in a given image.                  **/
/*************************************************************/
void DrawDialpad(Image *Img,int Color)
{
  switch(Img->D)
  {
    case 8:  DrawDialpad_8(Img,Color);break;
    case 16: DrawDialpad_16(Img,Color);break;
    case 24:
    case 32: DrawDialpad_32(Img,Color);break;
  }
}

/** DrawKeyboard() *******************************************/
/** Draw virtual keyboard in a given image. Key modifiers   **/
/** and the key code passed in CurKey are highlighted.      **/
/*************************************************************/
void DrawKeyboard(Image *Img,unsigned int CurKey)
{
  switch(Img->D)
  {
    case 8:  DrawKeyboard_8(Img,CurKey);break;
    case 16: DrawKeyboard_16(Img,CurKey);break;
    case 24:
    case 32: DrawKeyboard_32(Img,CurKey);break;
  }
}

/** DrawFinJoystick() ****************************************/
/** Draw finger joystick into given destination image. If   **/
/** the destination if too small, this function returns 0,  **/
/** it returns 1 otherwise.                                 **/
/*************************************************************/
int DrawFinJoystick(Image *Dst,int DX,int DY,int DW,int DH,int TextColor)
{
  switch(Dst->D)
  {
    case 8:  return(DrawFinJoystick_8(Dst,DX,DY,DW,DH,TextColor));
    case 16: return(DrawFinJoystick_16(Dst,DX,DY,DW,DH,TextColor));
    case 24:
    case 32: return(DrawFinJoystick_32(Dst,DX,DY,DW,DH,TextColor));
  }

  /* Wrong depth */
  return(0);
}

/** RenderVideo() ********************************************/
/** Draw video buffer to a given image. Return 0 on failure **/
/** or destination rectangle size inside OutImg on success. **/
/*************************************************************/
unsigned int RenderVideo(Image *OutImg,Image *CueImg,int Effects,int PenKey,int FrameRate)
{
  int D = OutImg? OutImg->D:VideoImg? VideoImg->D:0;

  /* Check that the depths are the same */
  if(CueImg && (D!=CueImg->D)) return(0);

  switch(D)
  {
    case 8:  return(RenderVideo_8(OutImg,CueImg,Effects,PenKey,FrameRate));
    case 16: return(RenderVideo_16(OutImg,CueImg,Effects,PenKey,FrameRate));
    case 24:
    case 32: return(RenderVideo_32(OutImg,CueImg,Effects,PenKey,FrameRate));
  }

  /* Wrong depth */
  return(0);
}

#endif /* TOUCHMUX_H */
