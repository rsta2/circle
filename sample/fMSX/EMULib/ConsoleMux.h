/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                       ConsoleMux.h                      **/
/**                                                         **/
/** This file wraps Console.c routines for multiple display **/
/** depths (BPP8,BPP16,BPP32). It is used automatically     **/
/** when none of BPP* values are defined.                   **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2008-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef CONSOLEMUX_H
#define CONSOLEMUX_H

#include "EMULib.h"
#include "Console.h"

#undef BPP8
#undef BPP16
#undef BPP24
#undef BPP32
#undef PIXEL

#define BPP8
#define pixel           unsigned char
#define PIXEL(R,G,B)    (pixel)(((7*(R)/255)<<5)|((7*(G)/255)<<2)|(3*(B)/255))
#define FG              FG_8
#define BG              BG_8
#define CONSetColor     CONSetColor_8
#define CONClear        CONClear_8
#define CONBox          CONBox_8
#define CONFrame        CONFrame_8
#define CONChar         CONChar_8
#define PrintXY         PrintXY_8
#define CONPrint        CONPrint_8
#define CONPrintN       CONPrintN_8
#define CONMsg          CONMsg_8
#define CONInput        CONInput_8
#define CONWindow       CONWindow_8
#define CONSelector     CONSelector_8
#define CONMenu         CONMenu_8
#define CONFile         CONFile_8
#include "Console.c"
#undef pixel
#undef PIXEL
#undef FG
#undef BG
#undef CONSetColor
#undef CONClear
#undef CONBox
#undef CONFrame
#undef CONChar
#undef PrintXY
#undef CONPrint
#undef CONPrintN
#undef CONMsg
#undef CONInput
#undef CONWindow
#undef CONSelector
#undef CONMenu
#undef CONFile
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
#define FG              FG_16
#define BG              BG_16
#define CONSetColor     CONSetColor_16
#define CONClear        CONClear_16
#define CONBox          CONBox_16
#define CONFrame        CONFrame_16
#define CONChar         CONChar_16
#define PrintXY         PrintXY_16
#define CONPrint        CONPrint_16
#define CONPrintN       CONPrintN_16
#define CONMsg          CONMsg_16
#define CONInput        CONInput_16
#define CONWindow       CONWindow_16
#define CONSelector     CONSelector_16
#define CONMenu         CONMenu_16
#define CONFile         CONFile_16
#include "Console.c"
#undef pixel
#undef PIXEL
#undef FG
#undef BG
#undef CONSetColor
#undef CONClear
#undef CONBox
#undef CONFrame
#undef CONChar
#undef PrintXY
#undef CONPrint
#undef CONPrintN
#undef CONMsg
#undef CONInput
#undef CONWindow
#undef CONSelector
#undef CONMenu
#undef CONFile
#undef BPP16

#define BPP32
#define pixel           unsigned int
#if defined(ANDROID)
#define PIXEL(R,G,B)    (pixel)(((int)R<<16)|((int)G<<8)|B|0xFF000000)
#else
#define PIXEL(R,G,B)    (pixel)(((int)R<<16)|((int)G<<8)|B)
#endif
#define FG              FG_32
#define BG              BG_32
#define CONSetColor     CONSetColor_32
#define CONClear        CONClear_32
#define CONBox          CONBox_32
#define CONFrame        CONFrame_32
#define CONChar         CONChar_32
#define PrintXY         PrintXY_32
#define CONPrint        CONPrint_32
#define CONPrintN       CONPrintN_32
#define CONMsg          CONMsg_32
#define CONInput        CONInput_32
#define CONWindow       CONWindow_32
#define CONSelector     CONSelector_32
#define CONMenu         CONMenu_32
#define CONFile         CONFile_32
#include "Console.c"
#undef pixel
#undef PIXEL
#undef FG
#undef BG
#undef CONSetColor
#undef CONClear
#undef CONBox
#undef CONFrame
#undef CONChar
#undef PrintXY
#undef CONPrint
#undef CONPrintN
#undef CONMsg
#undef CONInput
#undef CONWindow
#undef CONSelector
#undef CONMenu
#undef CONFile
#undef BPP32

/** CONSetColor() ********************************************/
/** Set current foreground and background colors.           **/
/*************************************************************/
void CONSetColor(pixel FGColor,pixel BGColor)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  CONSetColor_8(FGColor,BGColor);break;
      case 16: CONSetColor_16(FGColor,BGColor);break;
      case 24:
      case 32: CONSetColor_32(FGColor,BGColor);break;
    }
}

/** CONClear() ***********************************************/
/** Clear screen with a given color.                        **/
/*************************************************************/
void CONClear(pixel BGColor)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  CONClear_8(BGColor);break;
      case 16: CONClear_16(BGColor);break;
      case 24:
      case 32: CONClear_32(BGColor);break;
    }
}

/** CONBox() *************************************************/
/** Draw a filled box with a given color.                   **/
/*************************************************************/
void CONBox(int X,int Y,int Width,int Height,pixel BGColor)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  CONBox_8(X,Y,Width,Height,BGColor);break;
      case 16: CONBox_16(X,Y,Width,Height,BGColor);break;
      case 24:
      case 32: CONBox_32(X,Y,Width,Height,BGColor);break;
    }
}

/** CONFrame() ***********************************************/
/** Draw a frame with a given color.                        **/
/*************************************************************/
void CONFrame(int X,int Y,int Width,int Height,pixel BGColor)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  CONFrame_8(X,Y,Width,Height,BGColor);break;
      case 16: CONFrame_16(X,Y,Width,Height,BGColor);break;
      case 24:
      case 32: CONFrame_32(X,Y,Width,Height,BGColor);break;
    }
}

/** CONChar() ************************************************/
/** Print a character at given coordinates.                 **/
/*************************************************************/
void CONChar(int X,int Y,char V)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  CONChar_8(X,Y,V);break;
      case 16: CONChar_16(X,Y,V);break;
      case 24:
      case 32: CONChar_32(X,Y,V);break;
    }
}

/** PrintXY() ************************************************/
/** Print text at given pixel coordinates in given colors.  **/
/** When BG=-1, use transparent background.                 **/
/*************************************************************/
void PrintXY(Image *Img,const char *S,int X,int Y,pixel FG,int BG)
{
  switch(Img->D)
  {
    case 8:  PrintXY_8(Img,S,X,Y,FG,BG);break;
    case 16: PrintXY_16(Img,S,X,Y,FG,BG);break;
    case 24:
    case 32: PrintXY_32(Img,S,X,Y,FG,BG);break;
  }
}

/** CONPrint() ***********************************************/
/** Print a text at given coordinates with current colors.  **/
/*************************************************************/
void CONPrint(int X,int Y,const char *S)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  CONPrint_8(X,Y,S);break;
      case 16: CONPrint_16(X,Y,S);break;
      case 24:
      case 32: CONPrint_32(X,Y,S);break;
    }
}

/** CONPrintN() **********************************************/
/** Print a text at given coordinates with current colors.  **/
/** Truncate with "..." if text length exceeds N.           **/
/*************************************************************/
void CONPrintN(int X,int Y,const char *S,int N)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  CONPrintN_8(X,Y,S,N);break;
      case 16: CONPrintN_16(X,Y,S,N);break;
      case 24:
      case 32: CONPrintN_32(X,Y,S,N);break;
    }
}

/** CONMsg() *************************************************/
/** Show a message box.                                     **/
/*************************************************************/
void CONMsg(int X,int Y,int W,int H,pixel FGColor,pixel BGColor,const char *Title,const char *Text)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  CONMsg_8(X,Y,W,H,FGColor,BGColor,Title,Text);break;
      case 16: CONMsg_16(X,Y,W,H,FGColor,BGColor,Title,Text);break;
      case 24:
      case 32: CONMsg_32(X,Y,W,H,FGColor,BGColor,Title,Text);break;
    }
}

/** CONInput() ***********************************************/
/** Show an input box. Input modes (text/hex/dec) are ORed  **/
/** to the Length argument.                                 **/
/*************************************************************/
char *CONInput(int X,int Y,pixel FGColor,pixel BGColor,const char *Title,char *Input,unsigned int Length)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  return(CONInput_8(X,Y,FGColor,BGColor,Title,Input,Length));
      case 16: return(CONInput_16(X,Y,FGColor,BGColor,Title,Input,Length));
      case 24:
      case 32: return(CONInput_32(X,Y,FGColor,BGColor,Title,Input,Length));
    }

  return(0);
}

/** CONWindow() **********************************************/
/** Show a titled window.                                   **/
/*************************************************************/
void CONWindow(int X,int Y,int W,int H,pixel FGColor,pixel BGColor,const char *Title)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  CONWindow_8(X,Y,W,H,FGColor,BGColor,Title);break;
      case 16: CONWindow_16(X,Y,W,H,FGColor,BGColor,Title);break;
      case 24:
      case 32: CONWindow_32(X,Y,W,H,FGColor,BGColor,Title);break;
    }
}

/** CONSelector() ********************************************/
/** Common code used by CONMenu() and CONFile().            **/
/*************************************************************/
static const char *CONSelector(int X,int Y,int W,int H,pixel FGColor,pixel BGColor,const char *Items,int Item)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  return(CONSelector_8(X,Y,W,H,FGColor,BGColor,Items,Item));
      case 16: return(CONSelector_16(X,Y,W,H,FGColor,BGColor,Items,Item));
      case 24:
      case 32: return(CONSelector_32(X,Y,W,H,FGColor,BGColor,Items,Item));
    }

  return(0);
}

/** CONMenu() ************************************************/
/** Show a menu.                                            **/
/*************************************************************/
int CONMenu(int X,int Y,int W,int H,pixel FGColor,pixel BGColor,const char *Items,int Item)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  return(CONMenu_8(X,Y,W,H,FGColor,BGColor,Items,Item));
      case 16: return(CONMenu_16(X,Y,W,H,FGColor,BGColor,Items,Item));
      case 24:
      case 32: return(CONMenu_32(X,Y,W,H,FGColor,BGColor,Items,Item));
    }

  return(0);
}

/** CONFile() ************************************************/
/** Show a file selector.                                   **/
/*************************************************************/
const char *CONFile(pixel FGColor,pixel BGColor,const char *Ext)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  return(CONFile_8(FGColor,BGColor,Ext));
      case 16: return(CONFile_16(FGColor,BGColor,Ext));
      case 24:
      case 32: return(CONFile_32(FGColor,BGColor,Ext));
    }

  return(0);
}

#endif /* CONSOLEMUX_H */
