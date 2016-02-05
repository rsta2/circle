/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         Hunt.c                          **/
/**                                                         **/
/** This file implements mechanism for searching possible   **/
/** cheats inside running game data. Also see Hunt.h.       **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2013-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "Hunt.h"
#include <stdio.h>

#if defined(VGBA)
#include "ARM.h"
#define MEMREAD32(A) QRdARM(A)
#define MEMREAD16(A) WRdARM(A)
#define MEMREAD8(A)  BRdARM(A)
#elif defined(INES)
#include "M6502.h"
#define MEMREAD32(A) (Rd6502(A)+((int)Rd6502(A+1)<<8)+((int)Rd6502(A+2)<<16)+((int)Rd6502(A+3)<<24))
#define MEMREAD16(A) (Rd6502(A)+((int)Rd6502(A+1)<<8))
#define MEMREAD8(A)  Rd6502(A)
#elif defined(VGB) || defined(MG) || defined(COLEM) || defined(SPECCY) || defined(FMSX)
#include "Z80.h"
#define MEMREAD32(A) (RdZ80(A)+((int)RdZ80(A+1)<<8)+((int)RdZ80(A+2)<<16)+((int)RdZ80(A+3)<<24))
#define MEMREAD16(A) (RdZ80(A)+((int)RdZ80(A+1)<<8))
#define MEMREAD8(A)  RdZ80(A)
#else
#define MEMREAD32(A) (0)
#define MEMREAD16(A) (0)
#define MEMREAD8(A)  (0)
#endif

static HUNTEntry Buf[HUNT_BUFSIZE];
static int Count;

/** InitHUNT() ***********************************************/
/** Initialize cheat search, clearing all data.             **/
/*************************************************************/
void InitHUNT(void) { Count=0; }

/** TotalHUNT() **********************************************/
/** Get total number of currently watched locations.        **/
/*************************************************************/
int TotalHUNT(void) { return(Count); }

/** GetHUNT() ************************************************/
/** Get Nth memory location. Returns 0 for invalid N.       **/
/*************************************************************/
HUNTEntry *GetHUNT(int N)
{ return((N>=0)&&(N<Count)? &Buf[N]:0); }

/** AddHUNT() ************************************************/
/** Add a new value to search for, with the address range   **/
/** to search in. Returns number of memory locations found. **/
/*************************************************************/
int AddHUNT(unsigned int Addr,unsigned int Size,unsigned int Value,unsigned int NewValue,unsigned int Flags)
{
  unsigned int J,M;
  int I;

  /* Force 32bit/16bit mode for large values */
  if((Value>=0x10000)||(NewValue>=0x10000))
    Flags = (Flags&~HUNT_MASK_SIZE)|HUNT_32BIT;
  else if((Value>=0x100)||(NewValue>=0x100))
    Flags = (Flags&~HUNT_MASK_SIZE)|HUNT_16BIT;

  /* Compute mask for given value size and truncate value */
  M = Flags&HUNT_32BIT? 0xFFFFFFFF:Flags&HUNT_16BIT? 0xFFFF:0x00FF;

#ifdef VGBA
  /* ARM aligns data to the size boundary */
  if(M>0xFFFF) Addr&=~3; else if(M>0xFF) Addr&=~1;
#endif

  /* Scan memory for given value */
  for(Size+=Addr,I=0;(Addr<Size)&&(Count<HUNT_BUFSIZE);++Addr)
  {
    J = (M>0xFFFF? MEMREAD32(Addr):M>0xFF? MEMREAD16(Addr):MEMREAD8(Addr))&M;

    if((J==Value)||(J==((Value-1)&M)))
    {
      Buf[Count].Addr  = Addr;
      Buf[Count].Flags = Flags;
      Buf[Count].Value = J;
      Buf[Count].Orig  = J==Value? NewValue:((NewValue-1)&M);
      Buf[Count].Count = 0;
      ++Count;
      ++I;
    }

#ifdef VGBA
    /* ARM aligns data to the size boundary */
    if(M>0xFFFF) Addr+=3; else if(M>0xFF) Addr+=1;
#endif
  }

  /* Return the number of matches found */
  return(I);
}

/** ScanHUNT() ***********************************************/
/** Scan memory for changed values and update search data.  **/
/** Returns number of memory locations updated.             **/
/*************************************************************/
int ScanHUNT(void)
{
  unsigned int K,L;
  int J,I;

  /* Scan active search entries */
  for(J=I=0;J<Count;++J)
  {
    L = Buf[J].Flags&HUNT_32BIT? 0xFFFFFFFF:Buf[J].Flags&HUNT_16BIT? 0xFFFF:0x00FF;
    K = (L>0xFFFF? MEMREAD32(Buf[J].Addr):L>0xFF? MEMREAD16(Buf[J].Addr):MEMREAD8(Buf[J].Addr))&L;

    /* Check for expected changes */
    switch(Buf[J].Flags&HUNT_MASK_CHANGE)
    {
      case HUNT_PLUSONE:   L = K==((Buf[J].Value+1)&L);break;
      case HUNT_PLUSMANY:  L = K>Buf[J].Value;break;
      case HUNT_MINUSONE:  L = K==((Buf[J].Value-1)&L);break;
      case HUNT_MINUSMANY: L = K<Buf[J].Value;break;
      default:
      case HUNT_CONSTANT:  L = K==Buf[J].Value;break;
    }

    /* Delete any entry that does not change as expected */
    if(L)
    {
      if(Buf[J].Count<(1<<(sizeof(Buf[J].Count)<<3))-1) ++Buf[J].Count;
      Buf[J].Value = K;
      Buf[I++] = Buf[J];
    }
  }

  /* Return number of successfully updated entries */
  return(Count=I);
}

/** HUNT2Cheat() *********************************************/
/** Create cheat code from Nth hunt entry. Returns 0 if the **/
/** entry is invalid.                                       **/
/*************************************************************/
const char *HUNT2Cheat(int N,unsigned int Type)
{
  static char Buf[32];
  HUNTEntry *HE;

  /* Must have a valid entry */
  if(!(HE=GetHUNT(N))) return(0);

  /* Depending on cheat type... */
  switch(Type)
  {
/** GameBoy Advance ******************************************/
/** There are two versions of GameShark (v1/v3) differing   **/
/** by encryption and CodeBreaker. Not encrypting here.     **/
/*************************************************************/
    case HUNT_GBA_GS:
      /* 00AAAAAA NNNNNNDD - Multiple 8bit RAM Write   */
      /* 02AAAAAA NNNNDDDD - Multiple 816bit RAM Write */
      sprintf(Buf,"0%c%06X 0000%04X",
        HE->Flags&HUNT_16BIT? '2':'0',
        (HE->Addr&0x000FFFFF)|((HE->Addr&0x0F000000)>>4),
        HE->Orig
      );
      return(Buf);

    case HUNT_GBA_CB:
      /* 2AAAAAAA XXXX - 16bit RAM Write */
      /* 3AAAAAAA 00XX - 8bit RAM Write  */
      sprintf(Buf,"%c%07X %04X",
        HE->Flags&HUNT_16BIT? '2':'3',
        HE->Addr&0x0FFFFFFF,
        HE->Orig
      );
      return(Buf);

/** GameBoy **************************************************/
/** GameBoy has GameShark and GameGenie, but only GameShark **/
/** can modify RAM.                                         **/
/*************************************************************/
    case HUNT_GB_GS:
      /* 00DDAAAA - 8bit RAM Write, No Bank */
      sprintf(Buf,"00%02X%02X%02X",(HE->Orig)&0xFF,HE->Addr&0x00FF,(HE->Addr&0xFF00)>>8);
      if(HE->Flags&HUNT_16BIT)
        sprintf(Buf+8,";00%02X%02X%02X",HE->Orig>>8,(HE->Addr+1)&0x00FF,((HE->Addr+1)&0xFF00)>>8);
      return(Buf);

/** NES ******************************************************/
/** NES has both Pro Action Replay and GameGenie, but only  **/
/** Pro Action Replay can modify RAM.                       **/
/*************************************************************/
    case HUNT_NES_AR:
      /* 00AAAADD - 8bit RAM Write */
      sprintf(Buf,"00%04X%02X",HE->Addr&0xFFFF,HE->Orig&0xFF);
      if(HE->Flags&HUNT_16BIT)
        sprintf(Buf+8,";00%04X%02X",(HE->Addr+1)&0xFFFF,HE->Orig>>8);
      return(Buf);

/** Sega MasterSystem and GameGear ***************************/
/** MasterSystem has Pro Action Replay, while GameGear has  **/
/** GameGenie. Only Pro Action Replay can modify RAM.       **/
/*************************************************************/
    case HUNT_SMS_AR:
      /* 00AA-AADD - 8bit RAM Write */
      sprintf(Buf,"00%02X-%02X%02X",(HE->Addr&0xFF00)>>8,HE->Addr&0x00FF,HE->Orig&0xFF);
      if(HE->Flags&HUNT_16BIT)
        sprintf(Buf+9,";00%02X-%02X%02X",((HE->Addr+1)&0xFF00)>>8,(HE->Addr+1)&0x00FF,HE->Orig>>8);
      return(Buf);

/** ColecoVision, MSX, ZX Spectrum ***************************/
/** There was no official cheating hardware for these       **/
/** systems, so we come up with our own "hardware".         **/
/*************************************************************/
    case HUNT_MSX:
    case HUNT_ZXS:
    case HUNT_COLECO:
      /* AAAA-DD[DD] - 8bit[16bit] RAM Write */
      if(HE->Flags&HUNT_16BIT)
        sprintf(Buf,"%04X-%04X",HE->Addr,HE->Orig&0xFFFF);
      else
        sprintf(Buf,"%04X-%02X",HE->Addr,HE->Orig&0x00FF);
      return(Buf);
  }

  /* Invalid cheat type */
  return(0);
}
