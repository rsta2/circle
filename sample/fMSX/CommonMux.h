/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                       CommonMux.h                       **/
/**                                                         **/
/** This file instantiates MSX screen drivers for every     **/
/** possible screen depth. It includes common driver code   **/
/** from Common.h and Wide.h.                               **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef COMMONMUX_H
#define COMMONMUX_H

#include "Common.h"
#include "Wide.h"

#undef BPP8
#undef BPP16
#undef BPP24
#undef BPP32

/** Screen Mode Handlers [number of screens + 1] *************/
extern void (*RefreshLine[MAXSCREEN+2])(byte Y);

#define BPP8
#define pixel            unsigned char
#define FirstLine        FirstLine_8
#define Sprites          Sprites_8
#define ColorSprites     ColorSprites_8
#define RefreshBorder    RefreshBorder_8
#define RefreshBorder512 RefreshBorder512_8
#define ClearLine        ClearLine_8
#define ClearLine512     ClearLine512_8
#define YJKColor         YJKColor_8
#define RefreshScreen    RefreshScreen_8
#define RefreshLineF     RefreshLineF_8
#define RefreshLine0     RefreshLine0_8
#define RefreshLine1     RefreshLine1_8
#define RefreshLine2     RefreshLine2_8
#define RefreshLine3     RefreshLine3_8
#define RefreshLine4     RefreshLine4_8
#define RefreshLine5     RefreshLine5_8
#define RefreshLine6     RefreshLine6_8
#define RefreshLine7     RefreshLine7_8
#define RefreshLine8     RefreshLine8_8
#define RefreshLine10    RefreshLine10_8
#define RefreshLine12    RefreshLine12_8
#define RefreshLineTx80  RefreshLineTx80_8
#include "Common.h"
#include "Wide.h"
#undef pixel
#undef FirstLine
#undef Sprites
#undef ColorSprites   
#undef RefreshBorder  
#undef RefreshBorder512
#undef ClearLine      
#undef ClearLine512
#undef YJKColor       
#undef RefreshScreen  
#undef RefreshLineF   
#undef RefreshLine0   
#undef RefreshLine1   
#undef RefreshLine2   
#undef RefreshLine3   
#undef RefreshLine4   
#undef RefreshLine5   
#undef RefreshLine6   
#undef RefreshLine7   
#undef RefreshLine8   
#undef RefreshLine10  
#undef RefreshLine12  
#undef RefreshLineTx80
#undef BPP8

#define BPP16
#define pixel            unsigned short
#define FirstLine        FirstLine_16
#define Sprites          Sprites_16
#define ColorSprites     ColorSprites_16
#define RefreshBorder    RefreshBorder_16
#define RefreshBorder512 RefreshBorder512_16
#define ClearLine        ClearLine_16
#define ClearLine512     ClearLine512_16
#define YJKColor         YJKColor_16
#define RefreshScreen    RefreshScreen_16
#define RefreshLineF     RefreshLineF_16
#define RefreshLine0     RefreshLine0_16
#define RefreshLine1     RefreshLine1_16
#define RefreshLine2     RefreshLine2_16
#define RefreshLine3     RefreshLine3_16
#define RefreshLine4     RefreshLine4_16
#define RefreshLine5     RefreshLine5_16
#define RefreshLine6     RefreshLine6_16
#define RefreshLine7     RefreshLine7_16
#define RefreshLine8     RefreshLine8_16
#define RefreshLine10    RefreshLine10_16
#define RefreshLine12    RefreshLine12_16
#define RefreshLineTx80  RefreshLineTx80_16
#include "Common.h"
#include "Wide.h"
#undef pixel
#undef FirstLine
#undef Sprites
#undef ColorSprites   
#undef RefreshBorder  
#undef RefreshBorder512
#undef ClearLine      
#undef ClearLine512
#undef YJKColor       
#undef RefreshScreen  
#undef RefreshLineF   
#undef RefreshLine0   
#undef RefreshLine1   
#undef RefreshLine2   
#undef RefreshLine3   
#undef RefreshLine4   
#undef RefreshLine5   
#undef RefreshLine6   
#undef RefreshLine7   
#undef RefreshLine8   
#undef RefreshLine10  
#undef RefreshLine12  
#undef RefreshLineTx80
#undef BPP16

#define BPP32
#define pixel            unsigned int
#define FirstLine        FirstLine_32
#define Sprites          Sprites_32
#define ColorSprites     ColorSprites_32
#define RefreshBorder    RefreshBorder_32
#define RefreshBorder512 RefreshBorder512_32
#define ClearLine        ClearLine_32
#define ClearLine512     ClearLine512_32
#define YJKColor         YJKColor_32
#define RefreshScreen    RefreshScreen_32
#define RefreshLineF     RefreshLineF_32
#define RefreshLine0     RefreshLine0_32
#define RefreshLine1     RefreshLine1_32
#define RefreshLine2     RefreshLine2_32
#define RefreshLine3     RefreshLine3_32
#define RefreshLine4     RefreshLine4_32
#define RefreshLine5     RefreshLine5_32
#define RefreshLine6     RefreshLine6_32
#define RefreshLine7     RefreshLine7_32
#define RefreshLine8     RefreshLine8_32
#define RefreshLine10    RefreshLine10_32
#define RefreshLine12    RefreshLine12_32
#define RefreshLineTx80  RefreshLineTx80_32
#include "Common.h"
#include "Wide.h"
#undef pixel
#undef FirstLine
#undef Sprites
#undef ColorSprites   
#undef RefreshBorder  
#undef RefreshBorder512
#undef ClearLine      
#undef ClearLine512
#undef YJKColor       
#undef RefreshScreen  
#undef RefreshLineF   
#undef RefreshLine0   
#undef RefreshLine1   
#undef RefreshLine2   
#undef RefreshLine3   
#undef RefreshLine4   
#undef RefreshLine5   
#undef RefreshLine6   
#undef RefreshLine7   
#undef RefreshLine8   
#undef RefreshLine10  
#undef RefreshLine12  
#undef RefreshLineTx80
#undef BPP32

/** SetScreenDepth() *****************************************/
/** Fill fMSX screen driver array with pointers matching    **/
/** the given image depth.                                  **/
/*************************************************************/
int SetScreenDepth(int Depth)
{
  if(Depth<=8)
  {
    Depth=8;
    RefreshLine[0]  = RefreshLine0_8;
    RefreshLine[1]  = RefreshLine1_8;
    RefreshLine[2]  = RefreshLine2_8;
    RefreshLine[3]  = RefreshLine3_8;
    RefreshLine[4]  = RefreshLine4_8;
    RefreshLine[5]  = RefreshLine5_8;
    RefreshLine[6]  = RefreshLine6_8;
    RefreshLine[7]  = RefreshLine7_8;
    RefreshLine[8]  = RefreshLine8_8;
    RefreshLine[9]  = 0;
    RefreshLine[10] = RefreshLine10_8;
    RefreshLine[11] = RefreshLine10_8;
    RefreshLine[12] = RefreshLine12_8;
    RefreshLine[13] = RefreshLineTx80_8;
  }
  else if(Depth<=16)
  {
    Depth=16;
    RefreshLine[0]  = RefreshLine0_16;
    RefreshLine[1]  = RefreshLine1_16;
    RefreshLine[2]  = RefreshLine2_16;
    RefreshLine[3]  = RefreshLine3_16;
    RefreshLine[4]  = RefreshLine4_16;
    RefreshLine[5]  = RefreshLine5_16;
    RefreshLine[6]  = RefreshLine6_16;
    RefreshLine[7]  = RefreshLine7_16;
    RefreshLine[8]  = RefreshLine8_16;
    RefreshLine[9]  = 0;
    RefreshLine[10] = RefreshLine10_16;
    RefreshLine[11] = RefreshLine10_16;
    RefreshLine[12] = RefreshLine12_16;
    RefreshLine[13] = RefreshLineTx80_16;
  }
  else if(Depth<=32)
  {
    Depth=32;
    RefreshLine[0]  = RefreshLine0_32;
    RefreshLine[1]  = RefreshLine1_32;
    RefreshLine[2]  = RefreshLine2_32;
    RefreshLine[3]  = RefreshLine3_32;
    RefreshLine[4]  = RefreshLine4_32;
    RefreshLine[5]  = RefreshLine5_32;
    RefreshLine[6]  = RefreshLine6_32;
    RefreshLine[7]  = RefreshLine7_32;
    RefreshLine[8]  = RefreshLine8_32;
    RefreshLine[9]  = 0;
    RefreshLine[10] = RefreshLine10_32;
    RefreshLine[11] = RefreshLine10_32;
    RefreshLine[12] = RefreshLine12_32;
    RefreshLine[13] = RefreshLineTx80_32;
  }
  else
  {
    /* Wrong screen depth */
    Depth=0;
  }

  /* Done */
  return(Depth);
}
 
#endif /* COMMONMUX_H */
