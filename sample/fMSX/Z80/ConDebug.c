/** Z80: portable Z80 emulator *******************************/
/**                                                         **/
/**                       ConDebug.c                        **/
/**                                                         **/
/** This file contains a console version of the built-in    **/
/** debugger, using EMULib's Console.c. When -DCONDEBUG is  **/
/** ommitted, ConDebug.c just includes the default command  **/
/** line based debugger (Debug.c).                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2005-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifdef DEBUG

#ifndef CONDEBUG
/** Normal DebugZ80() ****************************************/
/** When CONDEBUG #undefined, we use plain command line.    **/
/*************************************************************/
#include "Debug.c"

#else
/** Console DebugZ80() ***************************************/
/** When CONDEBUG #defined, we use EMULib console.          **/
/*************************************************************/

#include "Z80.h"
#include "Console.h"
#include <stdlib.h>

#define DebugZ80 OriginalDebugZ80
#include "Debug.c"
#undef DebugZ80

#define CLR_BACK   PIXEL(255,255,255)
#define CLR_TEXT   PIXEL(0,0,0)
#define CLR_DIALOG PIXEL(0,100,0)
#define CLR_PC     PIXEL(255,0,0)
#define CLR_SP     PIXEL(0,0,100)

static byte ChrDump(byte C)
{
  return((C>=32)&&(C<128)? C:'.');
}
 
/** DebugZ80() ***********************************************/
/** This function should exist if DEBUG is #defined. When   **/
/** Trace!=0, it is called after each command executed by   **/
/** the CPU, and given the Z80 registers.                   **/
/*************************************************************/
byte DebugZ80(Z80 *R)
{
  char S[1024];
  word A,Addr,ABuf[20];
  int J,I,K,X,Y,MemoryDump,DrawWindow,ExitNow;

  /* If we don't have enough screen estate... */
  if((VideoW<32*8)||(VideoH<23*8))
  {
    /* Show warning message */
    CONMsg(
      -1,-1,-1,-1,PIXEL(255,255,255),PIXEL(255,0,0),
      "Error","Screen is\0too small!\0\0"
    );
    /* Continue emulation */
    R->Trace=0;
    return(1);
  }

#ifdef SPECCY
  /* Show currently refreshed scanline on Speccy */
  RefreshScreen();
#endif

  X    = ((VideoW>>3)-32)>>1;
  Y    = ((VideoH>>3)-23)>>1;
  Addr = R->PC.W;
  A    = ~Addr;
  K    = 0;
  
  for(DrawWindow=1,MemoryDump=ExitNow=0;!ExitNow&&VideoImg;)
  {
    if(DrawWindow)
    {
      CONWindow(X,Y,32,23,CLR_TEXT,CLR_BACK,"Z80 Debugger");

      sprintf(S,"PC %04X",R->PC.W);
      CONSetColor(CLR_BACK,CLR_PC);
      CONPrint(X+24,Y+18,S);
      sprintf(S,"SP %04X",R->SP.W);
      CONSetColor(CLR_BACK,CLR_SP);
      CONPrint(X+24,Y+19,S);

      CONSetColor(CLR_TEXT,CLR_BACK);
      sprintf(S,
        " %c%c%c%c%c%c\n\n"
        "AF %04X\nBC %04X\nDE %04X\nHL %04X\nIX %04X\nIY %04X\n\n"
        "AF'%04X\nBC'%04X\nDE'%04X\nHL'%04X\n\n"
        "IR %02X%02X",
        R->AF.B.l&0x80? 'S':'.',R->AF.B.l&0x40? 'Z':'.',R->AF.B.l&0x10? 'H':'.',
        R->AF.B.l&0x04? 'P':'.',R->AF.B.l&0x02? 'N':'.',R->AF.B.l&0x01? 'C':'.',
        R->AF.W,R->BC.W,R->DE.W,R->HL.W,
        R->IX.W,R->IY.W,
        R->AF1.W,R->BC1.W,R->DE1.W,R->HL1.W,
        R->I,R->R
      );
      CONPrint(X+24,Y+2,S);
      sprintf(S,
        "%s %s",
        R->IFF&0x04? "IM2":R->IFF&0x02? "IM1":"IM0",
        R->IFF&0x01? "EI":"DI"
      );
      CONPrint(X+25,Y+21,S);
      DrawWindow=0;
      A=~Addr;
    }

    /* If top address has changed... */
    if(A!=Addr)
    {
      /* Clear display */
      CONBox((X+1)<<3,(Y+2)<<3,23*8,20*8,CLR_BACK);

      if(MemoryDump)
      {
        /* Draw memory dump */
        for(J=0,A=Addr;J<20;J++,A+=4)
        {
          if(A==R->PC.W)      CONSetColor(CLR_BACK,CLR_PC);
          else if(A==R->SP.W) CONSetColor(CLR_BACK,CLR_SP);
          else                CONSetColor(CLR_TEXT,CLR_BACK);
          sprintf(S,"%04X%c",A,A==R->PC.W? CON_MORE:A==R->SP.W? CON_LESS:':');
          CONPrint(X+1,Y+J+2,S);

          CONSetColor(CLR_TEXT,CLR_BACK);
          sprintf(S,
            "%02X %02X %02X %02X %c%c%c%c",
            RdZ80(A),RdZ80(A+1),RdZ80(A+2),RdZ80(A+3),
            ChrDump(RdZ80(A)),ChrDump(RdZ80(A+1)),
            ChrDump(RdZ80(A+2)),ChrDump(RdZ80(A+3))
          );
          CONPrint(X+7,Y+J+2,S);
        }
      }
      else
      {
        /* Draw listing */
        for(J=0,A=Addr;J<20;J++)
        {
          if(A==R->PC.W)      CONSetColor(CLR_BACK,CLR_PC);
          else if(A==R->SP.W) CONSetColor(CLR_BACK,CLR_SP);
          else                CONSetColor(CLR_TEXT,CLR_BACK);
          sprintf(S,"%04X%c",A,A==R->PC.W? CON_MORE:A==R->SP.W? CON_LESS:':');
          CONPrint(X+1,Y+J+2,S);

          ABuf[J]=A;
          A+=DAsm(S,A);

          CONSetColor(CLR_TEXT,CLR_BACK);
          CONPrintN(X+7,Y+J+2,S,23);
        }
      }

      /* Display redrawn */
      A=Addr;
    }

    /* Draw pointer */
    CONChar(X+6,Y+K+2,CON_ARROW);

    /* Show screen buffer */
    ShowVideo();

    /* Get key code */
    GetKey();
    I=WaitKey();

    /* Clear pointer */
    CONChar(X+6,Y+K+2,' ');

    /* Get and process key code */
    switch(I)
    {
      case 'H':
        CONMsg(
          -1,-1,-1,-1,
          CLR_BACK,CLR_DIALOG,
          "Debugger Help",
          "ENTER - Execute next opcode\0"
          "   UP - Previous opcode\0"
          " DOWN - Next opcode\0"
          " LEFT - Page up\0"
          "RIGHT - Page down\0"
          "    H - This help page\0"
          "    G - Go to address\0"
          "    D - Disassembler view\0"
          "    M - Memory dump view\0"
          "    S - Show stack\0"
          "    J - Jump to cursor\0"
          "    R - Run to cursor\0"
          "    C - Continue execution\0"
          "    Q - Quit emulator\0"
        );
        DrawWindow=1;
        break;
      case CON_UP:
        if(K) --K;
        else
          if(MemoryDump) Addr-=4;
          else for(--Addr;Addr+DAsm(S,Addr)>A;--Addr);
        break;
      case CON_DOWN:
        if(K<19) ++K;
        else
          if(MemoryDump) Addr+=4;
          else Addr+=DAsm(S,Addr);
        break;
      case CON_LEFT:
        if(MemoryDump)
          Addr-=4*20;
        else
        {
          for(I=20,Addr=~A;(Addr>A)||((A^Addr)&~Addr&0x8000);++I)
            for(J=0,Addr=A-I;J<20;++J) Addr+=DAsm(S,Addr);
          Addr=A-I+1;
        }
        break; 
      case CON_RIGHT:
        if(MemoryDump)
          Addr+=4*20;
        else
          for(J=0;J<20;++J) Addr+=DAsm(S,Addr);
        break;
      case CON_OK:
        ExitNow=1;
        break;
      case '\0':
      case 'Q':
        return(0);
      case CON_EXIT:
      case 'C':
        R->Trap=0xFFFF;
        R->Trace=0;
        ExitNow=1;
        break;
      case 'R':
        R->Trap=ABuf[K];
        R->Trace=0;
        ExitNow=1;
        break;
      case 'M':
        MemoryDump=1;
        A=~Addr;
        break;
      case 'S':
        MemoryDump=1;
        Addr=R->SP.W;
        K=0;
        A=~Addr;
        break;
      case 'D':
        MemoryDump=0;
        A=~Addr;
        break;        
      case 'G':
        if(CONInput(-1,-1,CLR_BACK,CLR_DIALOG,"Go to Address:",S,5|CON_HEX))
        { Addr=strtoul(S,0,16);K=0; }
        DrawWindow=1;
        break;
      case 'J':
        R->PC.W=ABuf[K];
        A=~Addr;
        break;
    }
  }

  /* Continue emulation */
  return(1);
}

#endif /* CONDEBUG */
#endif /* DEBUG */
