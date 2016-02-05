/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        Console.h                        **/
/**                                                         **/
/** This file contains platform-independent definitions and **/
/** declarations for the EMULib-based console.              **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2005-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef CONSOLE_H
#define CONSOLE_H

#include "EMULib.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Special Characters ***************************************/
/** These are special characters implemented in the default **/
/** font and used in various standard dialogs.              **/
/*************************************************************/
#define CON_ARROW  0x01
#define CON_CHECK  0x02
#define CON_FOLDER 0x03
#define CON_FILE   0x04
#define CON_LESS   0x05
#define CON_MORE   0x06
#define CON_BUTTON 0x07
#define CON_BS     0x08
#define CON_TAB    0x09
#define CON_DOTS   0x0B
#define CON_ENTER  0x0D
#define CON_INSERT 0x0E
#define CON_DELETE 0x0F
#define CON_FUNC   0x10 /* 10 keys */ 
#define CON_STOP   0x1A
#define CON_ESCAPE 0x1B

/** CONSetFont Modes *****************************************/
/** Special font designators passed to CONSetFont().        **/
/*************************************************************/
#define FNT_NORMAL (const unsigned char *)0
#define FNT_BOLD   (const unsigned char *)1

/** CONInput() Modes *****************************************/
/** These are passed to CONInput().                         **/
/*************************************************************/
#define CON_TEXT   0x00000000
#define CON_DEC    0x80000000
#define CON_HEX    0x40000000
#define CON_HIDE   0x20000000

/** CONSetColor()/CONSetFont() *******************************/
/** Set current foreground and background colors, and font. **/
/*************************************************************/
void CONSetColor(pixel FGColor,pixel BGColor);
void CONSetFont(const unsigned char *Font);
const unsigned char *CONGetFont();

/** CONClear() ***********************************************/
/** Clear screen with a given color.                        **/
/*************************************************************/
void CONClear(pixel BGColor);

/** CONBox() *************************************************/
/** Draw a filled box with a given color.                   **/
/*************************************************************/
void CONBox(int X,int Y,int Width,int Height,pixel BGColor);

/** CONFrame() ***********************************************/
/** Draw a frame with a given color.                        **/
/*************************************************************/
void CONFrame(int X,int Y,int Width,int Height,pixel BGColor);

/** CONChar() ************************************************/
/** Print a character at given coordinates.                 **/
/*************************************************************/
void CONChar(int X,int Y,char V);

/** PrintXY() ************************************************/
/** Print text at given pixel coordinates in given colors.  **/
/** When BG=-1, use transparent background.                 **/
/*************************************************************/
void PrintXY(Image *Img,const char *S,int X,int Y,pixel FG,int BG);

/** CONPrint() ***********************************************/
/** Print a text at given coordinates with current colors.  **/
/*************************************************************/
void CONPrint(int X,int Y,const char *S);

/** CONPrintN() **********************************************/
/** Print a text at given coordinates with current colors.  **/
/** Truncate with "..." if text length exceeds N.           **/
/*************************************************************/
void CONPrintN(int X,int Y,const char *S,int N);

/** CONMsg() *************************************************/
/** Show a message box.                                     **/
/*************************************************************/
void CONMsg(int X,int Y,int W,int H,pixel FGColor,pixel BGColor,const char *Title,const char *Text);

/** CONInput() ***********************************************/
/** Show an input box. Input modes (text/hex/dec) are ORed  **/
/** to the Length argument.                                 **/
/*************************************************************/
char *CONInput(int X,int Y,pixel FGColor,pixel BGColor,const char *Title,char *Input,unsigned int Length);

/** CONWindow() **********************************************/
/** Show a titled window.                                   **/
/*************************************************************/
void CONWindow(int X,int Y,int W,int H,pixel FGColor,pixel BGColor,const char *Title);

/** CONMenu() ************************************************/
/** Show a menu.                                            **/
/*************************************************************/
int CONMenu(int X,int Y,int W,int H,pixel FGColor,pixel BGColor,const char *Items,int Item);

/** CONFile() ************************************************/
/** Show a file selector.                                   **/
/*************************************************************/
const char *CONFile(pixel FGColor,pixel BGColor,const char *Ext);

#ifdef __cplusplus
}
#endif
#endif /* CONSOLE_H */
