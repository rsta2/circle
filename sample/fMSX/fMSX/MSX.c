/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                          MSX.c                          **/
/**                                                         **/
/** This file contains implementation for the MSX-specific  **/
/** hardware: slots, memory mapper, PPIs, VDP, PSG, clock,  **/
/** etc. Initialization code and definitions needed for the **/
/** machine-dependent drivers are also here.                **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "MSX.h"
#include "Sound.h"
#include "Floppy.h"
#include "SHA1.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>

#ifdef __BORLANDC__
#include <dir.h>
#endif

#ifdef __WATCOMC__
#include <direct.h>
#endif

#if defined(UNIX) || defined(S60)
#include <unistd.h>
#endif

#ifdef ZLIB
#include <zlib.h>
#endif

#ifdef ANDROID
#include "MemFS.h"
#define puts   LOGI
#define printf LOGI
#endif

#define PRINTOK           if(Verbose) puts("OK")
#define PRINTFAILED       if(Verbose) puts("FAILED")
#define PRINTRESULT(R)    if(Verbose) puts((R)? "OK":"FAILED")

#define RGB2INT(R,G,B)    ((B)|((int)(G)<<8)|((int)(R)<<16))

/* MSDOS chdir() is broken and has to be replaced :( */
#ifdef MSDOS
#include "LibMSDOS.h"
#define chdir(path) ChangeDir(path)
#endif

/** User-defined parameters for fMSX *************************/
int  Mode        = MSX_MSX2|MSX_NTSC|MSX_GUESSA|MSX_GUESSB;
byte Verbose     = 1;              /* Debug msgs ON/OFF      */
byte UPeriod     = 75;             /* % of frames to draw    */
int  VPeriod     = CPU_VPERIOD;    /* CPU cycles per VBlank  */
int  HPeriod     = CPU_HPERIOD;    /* CPU cycles per HBlank  */
int  RAMPages    = 4;              /* Number of RAM pages    */
int  VRAMPages   = 2;              /* Number of VRAM pages   */
byte ExitNow     = 0;              /* 1 = Exit the emulator  */

/** Main hardware: CPU, RAM, VRAM, mappers *******************/
Z80 CPU;                           /* Z80 CPU state and regs */

byte *VRAM,*VPAGE;                 /* Video RAM              */

byte *RAM[8];                      /* Main RAM (8x8kB pages) */
byte *EmptyRAM;                    /* Empty RAM page (8kB)   */
byte SaveCMOS;                     /* Save CMOS.ROM on exit  */
byte *MemMap[4][4][8];   /* Memory maps [PPage][SPage][Addr] */

byte *RAMData;                     /* RAM Mapper contents    */
byte RAMMapper[4];                 /* RAM Mapper state       */
byte RAMMask;                      /* RAM Mapper mask        */

byte *ROMData[MAXSLOTS];           /* ROM Mapper contents    */
byte ROMMapper[MAXSLOTS][4];       /* ROM Mappers state      */
byte ROMMask[MAXSLOTS];            /* ROM Mapper masks       */
byte ROMType[MAXSLOTS];            /* ROM Mapper types       */

byte EnWrite[4];                   /* 1 if write enabled     */
byte PSL[4],SSL[4];                /* Lists of current slots */
byte PSLReg,SSLReg[4];   /* Storage for A8h port and (FFFFh) */

/** Memory blocks to free in TrashMSX() **********************/
const byte *Chunks[MAXCHUNKS];     /* Memory blocks to free  */
int NChunks;                       /* Number of memory blcks */

/** Working directory names **********************************/
const char *ProgDir = 0;           /* Program directory      */
const char *WorkDir;               /* Working directory      */

/** Cartridge files used by fMSX *****************************/
const char *ROMName[MAXCARTS] = { "CARTA.ROM","CARTB.ROM" };

/** On-cartridge SRAM data ***********************************/
char *SRAMName[MAXSLOTS] = {0,0,0,0,0,0};/* Filenames (gen-d)*/
byte SaveSRAM[MAXSLOTS] = {0,0,0,0,0,0}; /* Save SRAM on exit*/
byte *SRAMData[MAXSLOTS];          /* SRAM (battery backed)  */

/** Disk images used by fMSX *********************************/
const char *DSKName[MAXDRIVES] = { "DRIVEA.DSK","DRIVEB.DSK" };

/** Soundtrack logging ***************************************/
const char *SndName = "LOG.MID";   /* Sound log file         */

/** Emulation state saving ***********************************/
const char *STAName = "DEFAULT.STA";/* State file (autogen-d)*/

/** Fixed font used by fMSX **********************************/
const char *FNTName = "DEFAULT.FNT"; /* Font file for text   */
byte *FontBuf;                     /* Font for text modes    */

/** Printer **************************************************/
const char *PrnName = 0;           /* Printer redirect. file */
FILE *PrnStream;

/** Cassette tape ********************************************/
const char *CasName = "DEFAULT.CAS";  /* Tape image file     */
FILE *CasStream;

/** Serial port **********************************************/
const char *ComName = 0;           /* Serial redirect. file  */
FILE *ComIStream;
FILE *ComOStream;

/** Kanji font ROM *******************************************/
byte *Kanji;                       /* Kanji ROM 4096x32      */
int  KanLetter;                    /* Current letter index   */
byte KanCount;                     /* Byte count 0..31       */

/** Keyboard, joystick, and mouse ****************************/
volatile byte KeyState[16];        /* Keyboard map state     */
word JoyState;                     /* Joystick states        */
int  MouState[2];                  /* Mouse states           */
byte MouseDX[2],MouseDY[2];        /* Mouse offsets          */
byte OldMouseX[2],OldMouseY[2];    /* Old mouse coordinates  */
byte MCount[2];                    /* Mouse nibble counter   */

/** General I/O registers: i8255 *****************************/
I8255 PPI;                         /* i8255 PPI at A8h-ABh   */
byte IOReg;                        /* Storage for AAh port   */

/** Disk controller: WD1793 **********************************/
WD1793 FDC;                        /* WD1793 at 7FF8h-7FFFh  */
FDIDisk FDD[4];                    /* Floppy disk images     */

/** Sound hardware: PSG, SCC, OPLL ***************************/
AY8910 PSG;                        /* PSG registers & state  */
YM2413 OPLL;                       /* OPLL registers & state */
SCC  SCChip;                       /* SCC registers & state  */
byte SCCOn[2];                     /* 1 = SCC page active    */
word FMPACKey;                     /* MAGIC = SRAM active    */

/** Serial I/O hardware: i8251+i8253 *************************/
I8251 SIO;                         /* SIO registers & state  */

/** Real-time clock ******************************************/
byte RTCReg,RTCMode;               /* RTC register numbers   */
byte RTC[4][13];                   /* RTC registers          */

/** Video processor ******************************************/
byte *ChrGen,*ChrTab,*ColTab;      /* VDP tables (screen)    */
byte *SprGen,*SprTab;              /* VDP tables (sprites)   */
int  ChrGenM,ChrTabM,ColTabM;      /* VDP masks (screen)     */
int  SprTabM;                      /* VDP masks (sprites)    */
word VAddr;                        /* VRAM address in VDP    */
byte VKey,PKey,WKey;               /* Status keys for VDP    */
byte FGColor,BGColor;              /* Colors                 */
byte XFGColor,XBGColor;            /* Second set of colors   */
byte ScrMode;                      /* Current screen mode    */
byte VDP[64],VDPStatus[16];        /* VDP registers          */
byte IRQPending;                   /* Pending interrupts     */
int  ScanLine;                     /* Current scanline       */
byte VDPData;                      /* VDP data buffer        */
byte PLatch;                       /* Palette buffer         */
byte ALatch;                       /* Address buffer         */
int  Palette[16];                  /* Current palette        */

/** Cheat codes **********************************************/
byte CheatsON    = 0;              /* 1: Cheats are on       */
int  CheatCount  = 0;              /* # cheats, <=MAXCHEATS  */

struct
{
  unsigned int Addr;
  word Data,Orig;
  byte Size,Text[14];
} CheatCodes[MAXCHEATS];

/** Places in DiskROM to be patched with ED FE C9 ************/
static const word DiskPatches[] =
{ 0x4010,0x4013,0x4016,0x401C,0x401F,0 };

/** Places in BIOS to be patched with ED FE C9 ***************/
static const word BIOSPatches[] =
{ 0x00E1,0x00E4,0x00E7,0x00EA,0x00ED,0x00F0,0x00F3,0 };

/** Cartridge map, by primary and secondary slots ************/
static const byte CartMap[4][4] =
{ { 255,3,4,5 },{ 0,0,0,0 },{ 1,1,1,1 },{ 2,255,255,255 } };

/** Screen Mode Handlers [number of screens + 1] *************/
void (*RefreshLine[MAXSCREEN+2])(byte Y) =
{
  RefreshLine0,   /* SCR 0:  TEXT 40x24  */
  RefreshLine1,   /* SCR 1:  TEXT 32x24  */
  RefreshLine2,   /* SCR 2:  BLK 256x192 */
  RefreshLine3,   /* SCR 3:  64x48x16    */
  RefreshLine4,   /* SCR 4:  BLK 256x192 */
  RefreshLine5,   /* SCR 5:  256x192x16  */
  RefreshLine6,   /* SCR 6:  512x192x4   */
  RefreshLine7,   /* SCR 7:  512x192x16  */
  RefreshLine8,   /* SCR 8:  256x192x256 */
  0,              /* SCR 9:  NONE        */
  RefreshLine10,  /* SCR 10: YAE 256x192 */
  RefreshLine10,  /* SCR 11: YAE 256x192 */
  RefreshLine12,  /* SCR 12: YJK 256x192 */
  RefreshLineTx80 /* SCR 0:  TEXT 80x24  */
};

/** VDP Address Register Masks *******************************/
static const struct { byte R2,R3,R4,R5,M2,M3,M4,M5; } MSK[MAXSCREEN+2] =
{
  { 0x7F,0x00,0x3F,0x00,0x00,0x00,0x00,0x00 }, /* SCR 0:  TEXT 40x24  */
  { 0x7F,0xFF,0x3F,0xFF,0x00,0x00,0x00,0x00 }, /* SCR 1:  TEXT 32x24  */
  { 0x7F,0x80,0x3C,0xFF,0x00,0x7F,0x03,0x00 }, /* SCR 2:  BLK 256x192 */
  { 0x7F,0x00,0x3F,0xFF,0x00,0x00,0x00,0x00 }, /* SCR 3:  64x48x16    */
  { 0x7F,0x80,0x3C,0xFC,0x00,0x7F,0x03,0x03 }, /* SCR 4:  BLK 256x192 */
  { 0x60,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 5:  256x192x16  */
  { 0x60,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 6:  512x192x4   */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 7:  512x192x16  */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 8:  256x192x256 */
  { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }, /* SCR 9:  NONE        */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 10: YAE 256x192 */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 11: YAE 256x192 */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 12: YJK 256x192 */
  { 0x7C,0xF8,0x3F,0x00,0x03,0x07,0x00,0x00 }  /* SCR 0:  TEXT 80x24  */
};

/** MegaROM Mapper Names *************************************/
static const char *ROMNames[MAXMAPPERS+1] = 
{ 
  "GENERIC/8kB","GENERIC/16kB","KONAMI5/8kB",
  "KONAMI4/8kB","ASCII/8kB","ASCII/16kB",
  "GMASTER2/SRAM","FMPAC/SRAM","UNKNOWN"
};

/** Keyboard Mapping *****************************************/
/** This keyboard mapping is used by KBD_SET()/KBD_RES()    **/
/** macros to modify KeyState[] bits.                       **/
/*************************************************************/
const byte Keys[][2] =
{
  { 0,0x00 },{ 8,0x10 },{ 8,0x20 },{ 8,0x80 }, /* None,LEFT,UP,RIGHT */
  { 8,0x40 },{ 6,0x01 },{ 6,0x02 },{ 6,0x04 }, /* DOWN,SHIFT,CONTROL,GRAPH */
  { 7,0x20 },{ 7,0x08 },{ 6,0x08 },{ 7,0x40 }, /* BS,TAB,CAPSLOCK,SELECT */
  { 8,0x02 },{ 7,0x80 },{ 8,0x08 },{ 8,0x04 }, /* HOME,ENTER,DELETE,INSERT */
  { 6,0x10 },{ 7,0x10 },{ 6,0x20 },{ 6,0x40 }, /* COUNTRY,STOP,F1,F2 */
  { 6,0x80 },{ 7,0x01 },{ 7,0x02 },{ 9,0x08 }, /* F3,F4,F5,PAD0 */
  { 9,0x10 },{ 9,0x20 },{ 9,0x40 },{ 7,0x04 }, /* PAD1,PAD2,PAD3,ESCAPE */
  { 9,0x80 },{ 10,0x01 },{ 10,0x02 },{ 10,0x04 }, /* PAD4,PAD5,PAD6,PAD7 */
  { 8,0x01 },{ 0,0x02 },{ 2,0x01 },{ 0,0x08 }, /* SPACE,[!],["],[#] */
  { 0,0x10 },{ 0,0x20 },{ 0,0x80 },{ 2,0x01 }, /* [$],[%],[&],['] */
  { 1,0x02 },{ 0,0x01 },{ 1,0x01 },{ 1,0x08 }, /* [(],[)],[*],[=] */
  { 2,0x04 },{ 1,0x04 },{ 2,0x08 },{ 2,0x10 }, /* [,],[-],[.],[/] */
  { 0,0x01 },{ 0,0x02 },{ 0,0x04 },{ 0,0x08 }, /* 0,1,2,3 */
  { 0,0x10 },{ 0,0x20 },{ 0,0x40 },{ 0,0x80 }, /* 4,5,6,7 */
  { 1,0x01 },{ 1,0x02 },{ 1,0x80 },{ 1,0x80 }, /* 8,9,[:],[;] */
  { 2,0x04 },{ 1,0x08 },{ 2,0x08 },{ 2,0x10 }, /* [<],[=],[>],[?] */
  { 0,0x04 },{ 2,0x40 },{ 2,0x80 },{ 3,0x01 }, /* [@],A,B,C */
  { 3,0x02 },{ 3,0x04 },{ 3,0x08 },{ 3,0x10 }, /* D,E,F,G */
  { 3,0x20 },{ 3,0x40 },{ 3,0x80 },{ 4,0x01 }, /* H,I,J,K */
  { 4,0x02 },{ 4,0x04 },{ 4,0x08 },{ 4,0x10 }, /* L,M,N,O */
  { 4,0x20 },{ 4,0x40 },{ 4,0x80 },{ 5,0x01 }, /* P,Q,R,S */
  { 5,0x02 },{ 5,0x04 },{ 5,0x08 },{ 5,0x10 }, /* T,U,V,W */
  { 5,0x20 },{ 5,0x40 },{ 5,0x80 },{ 1,0x20 }, /* X,Y,Z,[[] */
  { 1,0x10 },{ 1,0x40 },{ 0,0x40 },{ 1,0x04 }, /* [\],[]],[^],[_] */
  { 2,0x02 },{ 2,0x40 },{ 2,0x80 },{ 3,0x01 }, /* [`],a,b,c */
  { 3,0x02 },{ 3,0x04 },{ 3,0x08 },{ 3,0x10 }, /* d,e,f,g */
  { 3,0x20 },{ 3,0x40 },{ 3,0x80 },{ 4,0x01 }, /* h,i,j,k */
  { 4,0x02 },{ 4,0x04 },{ 4,0x08 },{ 4,0x10 }, /* l,m,n,o */
  { 4,0x20 },{ 4,0x40 },{ 4,0x80 },{ 5,0x01 }, /* p,q,r,s */
  { 5,0x02 },{ 5,0x04 },{ 5,0x08 },{ 5,0x10 }, /* t,u,v,w */
  { 5,0x20 },{ 5,0x40 },{ 5,0x80 },{ 1,0x20 }, /* x,y,z,[{] */
  { 1,0x10 },{ 1,0x40 },{ 2,0x02 },{ 8,0x08 }, /* [|],[}],[~],DEL */
  { 10,0x08 },{ 10,0x10 }                      /* PAD8,PAD9 */
};

/** Internal Functions ***************************************/
/** These functions are defined and internally used by the  **/
/** code in MSX.c.                                          **/
/*************************************************************/
byte *LoadROM(const char *Name,int Size,byte *Buf);
int  GuessROM(const byte *Buf,int Size);
int  FindState(const char *Name);
void SetMegaROM(int Slot,byte P0,byte P1,byte P2,byte P3);
void MapROM(word A,byte V);       /* Switch MegaROM banks            */
void PSlot(byte V);               /* Switch primary slots            */
void SSlot(byte V);               /* Switch secondary slots          */
void VDPOut(byte R,byte V);       /* Write value into a VDP register */
void Printer(byte V);             /* Send a character to a printer   */
void PPIOut(byte New,byte Old);   /* Set PPI bits (key click, etc.)  */
void CheckSprites(void);          /* Check collisions and 5th sprite */
byte RTCIn(byte R);               /* Read RTC registers              */
byte SetScreen(void);             /* Change screen mode              */
word SetIRQ(byte IRQ);            /* Set/Reset IRQ                   */
word StateID(void);               /* Compute emulation state ID      */
int  ApplyCheats(void);           /* Apply RAM-based cheats          */

static int hasext(const char *FileName,const char *Ext);
static byte *GetMemory(int Size); /* Get memory chunk                */
static void FreeMemory(byte *Ptr);/* Free memory chunk               */
static void FreeAllMemory(void);  /* Free all memory chunks          */

/** hasext() *************************************************/
/** Check if file name has given extension.                 **/
/*************************************************************/
static int hasext(const char *FileName,const char *Ext)
{
  const char *P;
  int J;

  /* Start searching from the end, terminate at directory name */
  for(P=FileName+strlen(FileName);(P>=FileName)&&(*P!='/')&&(*P!='\\');--P)
  {
    /* Locate start of the next extension */
    for(--P;(P>=FileName)&&(*P!='/')&&(*P!='\\')&&(*P!=*Ext);--P);
    /* If next extension not found, return FALSE */
    if((P<FileName)||(*P=='/')||(*P=='\\')) return(0);
    /* Compare with the given extension */
    for(J=0;P[J]&&Ext[J]&&(toupper(P[J])==toupper(Ext[J]));++J);
    /* If extension matches, return TRUE */
    if(!Ext[J]&&(!P[J]||(P[J]==*Ext))) return(1);
  }

  /* Extensions do not match */
  return(0);
}

/** GetMemory() **********************************************/
/** Allocate a memory chunk of given size using malloc().   **/
/** Store allocated address in Chunks[] for later disposal. **/
/*************************************************************/
static byte *GetMemory(int Size)
{
  byte *P;

  if((Size<=0)||(NChunks>=MAXCHUNKS)) return(0);
  P=(byte *)malloc(Size);
  if(P) Chunks[NChunks++]=P;

  return(P);
}

/** FreeMemory() *********************************************/
/** Free memory allocated by a previous GetMemory() call.   **/
/*************************************************************/
static void FreeMemory(byte *Ptr)
{
  int J;

  /* Special case: we do not free EmptyRAM! */
  if(!Ptr||(Ptr==EmptyRAM)) return;

  for(J=0;(J<NChunks)&&(Ptr!=Chunks[J]);++J);
  if(J<NChunks)
  {
    for(--NChunks;J<NChunks;++J) Chunks[J]=Chunks[J+1];
    free(Ptr);
  }
}

/** FreeAllMemory() ******************************************/
/** Free all memory allocated by GetMemory() calls.         **/
/*************************************************************/
static void FreeAllMemory(void)
{
  int J;

  for(J=0;J<NChunks;++J) free((void *)Chunks[J]);
  NChunks=0;
}

/** StartMSX() ***********************************************/
/** Allocate memory, load ROM images, initialize hardware,  **/
/** CPU and start the emulation. This function returns 0 in **/
/** the case of failure.                                    **/
/*************************************************************/
int StartMSX(int NewMode,int NewRAMPages,int NewVRAMPages)
{
  /*** Joystick types: ***/
  static const char *JoyTypes[] =
  {
    "nothing","normal joystick",
    "mouse in joystick mode","mouse in real mode"
  };

  /*** CMOS ROM default values: ***/
  static const byte RTCInit[4][13]  =
  {
    {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    {  0, 0, 0, 0,40,80,15, 4, 4, 0, 0, 0, 0 },
    {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  };

  int *T,I,J,K;
  byte *P;
  word A;

  /*** STARTUP CODE starts here: ***/

  T=(int *)"\01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
#ifdef LSB_FIRST
  if(*T!=1)
  {
    printf("********** This machine is high-endian. **********\n");
    printf("Take #define LSB_FIRST out and compile fMSX again.\n");
    return(0);
  }
#else
  if(*T==1)
  {
    printf("********* This machine is low-endian. **********\n");
    printf("Insert #define LSB_FIRST and compile fMSX again.\n");
    return(0);
  }
#endif

  /* Zero everyting */
  CasStream=PrnStream=ComIStream=ComOStream=0;
  FontBuf     = 0;
  RAMData     = 0;
  VRAM        = 0;
  Kanji       = 0;
  WorkDir     = 0;
  SaveCMOS    = 0;
  FMPACKey    = 0x0000;
  ExitNow     = 0;
  NChunks     = 0;
  CheatsON    = 0;
  CheatCount  = 0;

  /* Zero cartridge related data */
  for(J=0;J<MAXSLOTS;++J)
  {
    ROMMask[J]  = 0;
    ROMData[J]  = 0;
    ROMType[J]  = 0;
    SRAMData[J] = 0;
    SRAMName[J] = 0;
    SaveSRAM[J] = 0; 
  }

  /* UPeriod has ot be in 1%..100% range */
  UPeriod=UPeriod<1? 1:UPeriod>100? 100:UPeriod;

  /* Allocate 16kB for the empty space (scratch RAM) */
  if(Verbose) printf("Allocating 16kB for empty space...\n");
  if(!(EmptyRAM=GetMemory(0x4000))) { PRINTFAILED;return(0); }
  memset(EmptyRAM,NORAM,0x4000);

  /* Reset memory map to the empty space */
  for(I=0;I<4;++I)
    for(J=0;J<4;++J)
      for(K=0;K<8;++K)
        MemMap[I][J][K]=EmptyRAM;

  /* Save current directory */
  if(ProgDir)
    if(WorkDir=getcwd(0,1024)) Chunks[NChunks++]=WorkDir;

  /* Set invalid modes and RAM/VRAM sizes before calling ResetMSX() */
  Mode      = ~NewMode;
  RAMPages  = 0;
  VRAMPages = 0;

  /* Try resetting MSX, allocating memory, loading ROMs */
  if((ResetMSX(NewMode,NewRAMPages,NewVRAMPages)^NewMode)&MSX_MODEL) return(0);
  if(!RAMPages||!VRAMPages) return(0);

  /* Change to the program directory */
  if(ProgDir && chdir(ProgDir))
  { if(Verbose) printf("Failed changing to '%s' directory!\n",ProgDir); }

  /* Try loading font */
  if(FNTName)
  {
    if(Verbose) printf("Loading %s font...",FNTName);
    J=LoadFNT(FNTName);
    PRINTRESULT(J);
  }

  if(Verbose) printf("Loading optional ROMs: ");

  /* Try loading CMOS memory contents */
  if(LoadROM("CMOS.ROM",sizeof(RTC),(byte *)RTC))
  { if(Verbose) printf("CMOS.ROM.."); }
  else memcpy(RTC,RTCInit,sizeof(RTC));

  /* Try loading Kanji alphabet ROM */
  if(Kanji=LoadROM("KANJI.ROM",0x20000,0))
  { if(Verbose) printf("KANJI.ROM.."); }

  /* Try loading RS232 support ROM to slot */
  if(P=LoadROM("RS232.ROM",0x4000,0))
  {
    if(Verbose) printf("RS232.ROM..");
    MemMap[3][3][2]=P;
    MemMap[3][3][3]=P+0x2000;
  }

  PRINTOK;

  /* Start loading system cartridges */
  J=MAXCARTS;

  /* If MSX2 or better and DiskROM present...  */
  /* ...try loading MSXDOS2 cartridge into 3:0 */
  if(!MODEL(MSX_MSX1)&&(MemMap[3][1][2]!=EmptyRAM)&&!ROMData[2])
    if(LoadCart("MSXDOS2.ROM",2,MAP_GEN16))
      SetMegaROM(2,0,1,ROMMask[J]-1,ROMMask[J]);

  /* If MSX2 or better, load PAINTER cartridge */
  if(!MODEL(MSX_MSX1))
  {
    for(;(J<MAXSLOTS)&&ROMData[J];++J);
    if((J<MAXSLOTS)&&LoadCart("PAINTER.ROM",J,0)) ++J;
  }

  /* Load FMPAC cartridge */
  for(;(J<MAXSLOTS)&&ROMData[J];++J);
  if((J<MAXSLOTS)&&LoadCart("FMPAC.ROM",J,MAP_FMPAC)) ++J;

  /* Load Konami GameMaster2/GameMaster cartridges */
  for(;(J<MAXSLOTS)&&ROMData[J];++J);
  if(J<MAXSLOTS)
  {
    if(LoadCart("GMASTER2.ROM",J,MAP_GMASTER2)) ++J;
    else if(LoadCart("GMASTER.ROM",J,0)) ++J;
  }

  /* We are now back to working directory */
  if(WorkDir && chdir(WorkDir))
  { if(Verbose) printf("Failed changing to '%s' directory!\n",WorkDir); }

  /* For each user cartridge slot, try loading cartridge */
  for(J=0;J<MAXCARTS;++J) LoadCart(ROMName[J],J,ROMGUESS(J)|ROMTYPE(J));

  /* Open stream for a printer */
  if(Verbose)
    printf("Redirecting printer output to %s...OK\n",PrnName? PrnName:"STDOUT");
  ChangePrinter(PrnName);

  /* Open streams for serial IO */
  if(!ComName) { ComIStream=stdin;ComOStream=stdout; }
  else
  {
    if(Verbose) printf("Redirecting serial I/O to %s...",ComName);
    if(!(ComOStream=ComIStream=fopen(ComName,"r+b")))
    { ComIStream=stdin;ComOStream=stdout; }
    PRINTRESULT(ComOStream!=stdout);
  }

  /* Open casette image */
  if(CasName&&ChangeTape(CasName))
    if(Verbose) printf("Using %s as a tape\n",CasName);

  /* Initialize floppy disk controller */
  Reset1793(&FDC,FDD,WD1793_INIT);
  FDC.Verbose=Verbose&0x04;

  /* Open disk images */
  for(J=0;J<MAXDRIVES;++J)
  {
    FDD[J].Verbose=Verbose&0x04;
    if(ChangeDisk(J,DSKName[J]))
      if(Verbose) printf("Inserting %s into drive %c\n",DSKName[J],J+'A');  
  }

  /* Initialize sound logging */
  InitMIDI(SndName);

  /* Done with initialization */
  if(Verbose)
  {
    printf("Initializing VDP, FDC, PSG, OPLL, SCC, and CPU...\n");
    printf("  Attached %s to joystick port A\n",JoyTypes[JOYTYPE(0)]);
    printf("  Attached %s to joystick port B\n",JoyTypes[JOYTYPE(1)]);
    printf("  %d CPU cycles per HBlank\n",HPeriod);
    printf("  %d CPU cycles per VBlank\n",VPeriod);
    printf("  %d scanlines\n",VPeriod/HPeriod);
  }

  /* Start execution of the code */
  if(Verbose) printf("RUNNING ROM CODE...\n");
  A=RunZ80(&CPU);

  /* Exiting emulation... */
  if(Verbose) printf("EXITED at PC = %04Xh.\n",A);
  return(1);
}

/** TrashMSX() ***********************************************/
/** Free resources allocated by StartMSX().                 **/
/*************************************************************/
void TrashMSX(void)
{
  FILE *F;
  int J;

  /* CMOS.ROM is saved in the program directory */
  if(ProgDir && chdir(ProgDir))
  { if(Verbose) printf("Failed changing to '%s' directory!\n",ProgDir); }

  /* Save CMOS RAM, if present */
  if(SaveCMOS)
  {
    if(Verbose) printf("Writing CMOS.ROM...");
    if(!(F=fopen("CMOS.ROM","wb"))) SaveCMOS=0;
    else
    {
      if(fwrite(RTC,1,sizeof(RTC),F)!=sizeof(RTC)) SaveCMOS=0;
      fclose(F);
    }
    PRINTRESULT(SaveCMOS);
  }

  /* Change back to working directory */
  if(WorkDir && chdir(WorkDir))
  { if(Verbose) printf("Failed changing to '%s' directory!\n",WorkDir); }

  /* Shut down sound logging */
  TrashMIDI();

  /* Eject disks, free disk buffers */
  Reset1793(&FDC,FDD,WD1793_EJECT);

  /* Close printer output */
  ChangePrinter(0);

  /* Close tape */
  ChangeTape(0);
  
  /* Close all IO streams */
  if(ComOStream&&(ComOStream!=stdout)) fclose(ComOStream);
  if(ComIStream&&(ComIStream!=stdin))  fclose(ComIStream);

  /* Eject all cartridges (will save SRAM) */
  for(J=0;J<MAXSLOTS;++J) LoadCart(0,J,ROMType[J]);

  /* Eject all disks */
  for(J=0;J<MAXDRIVES;++J) ChangeDisk(J,0);

  /* Free all remaining allocated memory */
  FreeAllMemory();
}

/** ResetMSX() ***********************************************/
/** Reset MSX hardware to new operating modes. Returns new  **/
/** modes, possibly not the same as NewMode.                **/
/*************************************************************/
int ResetMSX(int NewMode,int NewRAMPages,int NewVRAMPages)
{
  /*** VDP status register states: ***/
  static const byte VDPSInit[16] = { 0x9F,0,0x6C,0,0,0,0,0,0,0,0,0,0,0,0,0 };

  /*** VDP control register states: ***/
  static const byte VDPInit[64]  =
  {
    0x00,0x10,0xFF,0xFF,0xFF,0xFF,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  };

  /*** Initial palette: ***/
  static const unsigned int PalInit[16] =
  {
    0x00000000,0x00000000,0x0020C020,0x0060E060,
    0x002020E0,0x004060E0,0x00A02020,0x0040C0E0,
    0x00E02020,0x00E06060,0x00C0C020,0x00C0C080,
    0x00208020,0x00C040A0,0x00A0A0A0,0x00E0E0E0
  };

  byte *P1,*P2;
  int J,I;

  /* If changing hardware model, load new system ROMs */
  if((Mode^NewMode)&MSX_MODEL)
  {
    /* Change to the program directory */
    if(ProgDir && chdir(ProgDir))
    { if(Verbose) printf("  Failed changing to '%s' directory!\n",ProgDir); }

    switch(NewMode&MSX_MODEL)
    {
      case MSX_MSX1:
        if(Verbose) printf("  Opening MSX.ROM...");
        P1=LoadROM("MSX.ROM",0x8000,0);
        PRINTRESULT(P1);
        if(!P1) NewMode=(NewMode&~MSX_MODEL)|(Mode&MSX_MODEL);
        else
        {
          FreeMemory(MemMap[0][0][0]);
          FreeMemory(MemMap[3][1][0]);
          MemMap[0][0][0]=P1;
          MemMap[0][0][1]=P1+0x2000;
          MemMap[0][0][2]=P1+0x4000;
          MemMap[0][0][3]=P1+0x6000;
          MemMap[3][1][0]=EmptyRAM;
          MemMap[3][1][1]=EmptyRAM;
        }
        break;

      case MSX_MSX2:
        if(Verbose) printf("  Opening MSX2.ROM...");
        P1=LoadROM("MSX2.ROM",0x8000,0);
        PRINTRESULT(P1);
        if(Verbose) printf("  Opening MSX2EXT.ROM...");
        P2=LoadROM("MSX2EXT.ROM",0x4000,0);
        PRINTRESULT(P2);
        if(!P1||!P2) 
        {
          NewMode=(NewMode&~MSX_MODEL)|(Mode&MSX_MODEL);
          FreeMemory(P1);
          FreeMemory(P2);
        }
        else
        {
          FreeMemory(MemMap[0][0][0]);
          FreeMemory(MemMap[3][1][0]);
          MemMap[0][0][0]=P1;
          MemMap[0][0][1]=P1+0x2000;
          MemMap[0][0][2]=P1+0x4000;
          MemMap[0][0][3]=P1+0x6000;
          MemMap[3][1][0]=P2;
          MemMap[3][1][1]=P2+0x2000;
        }
        break;

      case MSX_MSX2P:
        if(Verbose) printf("  Opening MSX2P.ROM...");
        P1=LoadROM("MSX2P.ROM",0x8000,0);
        PRINTRESULT(P1);
        if(Verbose) printf("  Opening MSX2PEXT.ROM...");
        P2=LoadROM("MSX2PEXT.ROM",0x4000,0);
        PRINTRESULT(P2);
        if(!P1||!P2) 
        {
          NewMode=(NewMode&~MSX_MODEL)|(Mode&MSX_MODEL);
          FreeMemory(P1);
          FreeMemory(P2);
        }
        else
        {
          FreeMemory(MemMap[0][0][0]);
          FreeMemory(MemMap[3][1][0]);
          MemMap[0][0][0]=P1;
          MemMap[0][0][1]=P1+0x2000;
          MemMap[0][0][2]=P1+0x4000;
          MemMap[0][0][3]=P1+0x6000;
          MemMap[3][1][0]=P2;
          MemMap[3][1][1]=P2+0x2000;
        }
        break;

      default:
        /* Unknown MSX model, keep old model */
        if(Verbose) printf("ResetMSX(): INVALID HARDWARE MODEL!\n");
        NewMode=(NewMode&~MSX_MODEL)|(Mode&MSX_MODEL);
        break;
    }

    /* Change to the working directory */
    if(WorkDir && chdir(WorkDir))
    { if(Verbose) printf("Failed changing to '%s' directory!\n",WorkDir); }
  }

  /* If hardware model changed ok, patch freshly loaded BIOS */
  if((Mode^NewMode)&MSX_MODEL)
  {
    /* Apply patches to BIOS */
    if(Verbose) printf("  Patching BIOS: ");
    for(J=0;BIOSPatches[J];++J)
    {
      if(Verbose) printf("%04X..",BIOSPatches[J]);
      P1=MemMap[0][0][0]+BIOSPatches[J];
      P1[0]=0xED;P1[1]=0xFE;P1[2]=0xC9;
    }
    PRINTOK;
  }

  /* If toggling BDOS patches... */
  if((Mode^NewMode)&MSX_PATCHBDOS)
  {
    /* Change to the program directory */
    if(ProgDir && chdir(ProgDir))
    { if(Verbose) printf("  Failed changing to '%s' directory!\n",ProgDir); }

    /* Try loading DiskROM */
    if(Verbose) printf("  Opening DISK.ROM...");
    P1=LoadROM("DISK.ROM",0x4000,0);
    PRINTRESULT(P1);

    /* Change to the working directory */
    if(WorkDir && chdir(WorkDir))
    { if(Verbose) printf("  Failed changing to '%s' directory!\n",WorkDir); }

    /* If failed loading DiskROM, ignore the new PATCHBDOS bit */
    if(!P1) NewMode=(NewMode&~MSX_PATCHBDOS)|(Mode&MSX_PATCHBDOS);
    else
    {
      /* Assign new DiskROM */
      FreeMemory(MemMap[3][1][2]);
      MemMap[3][1][2]=P1;
      MemMap[3][1][3]=P1+0x2000;

      /* If BDOS patching requested... */
      if(NewMode&MSX_PATCHBDOS)
      {
        if(Verbose) printf("  Patching BDOS: ");
        /* Apply patches to BDOS */
        for(J=0;DiskPatches[J];++J)
        {
          if(Verbose) printf("%04X..",DiskPatches[J]);
          P2=P1+DiskPatches[J]-0x4000;
          P2[0]=0xED;P2[1]=0xFE;P2[2]=0xC9;
        }
        PRINTOK;
      }
    }
  }

  /* Assign new modes */
  Mode           = NewMode;

  /* Set ROM types for cartridges A/B */
  ROMType[0]     = ROMTYPE(0);
  ROMType[1]     = ROMTYPE(1);

  /* Set CPU timings */
  VPeriod        = (VIDEO(MSX_PAL)? VPERIOD_PAL:VPERIOD_NTSC)/6;
  HPeriod        = HPERIOD/6;
  CPU.IPeriod    = CPU_H240;
  CPU.IAutoReset = 0;

  /* Numbers of RAM/VRAM pages should be power of 2 */
  for(J=1;J<NewRAMPages;J<<=1);
  NewRAMPages=J;
  for(J=1;J<NewVRAMPages;J<<=1);
  NewVRAMPages=J;

  /* Correct RAM and VRAM sizes */
  if((NewRAMPages<(MODEL(MSX_MSX1)? 4:8))||(NewRAMPages>256))
    NewRAMPages=MODEL(MSX_MSX1)? 4:8;
  if((NewVRAMPages<(MODEL(MSX_MSX1)? 2:8))||(NewVRAMPages>8))
    NewVRAMPages=MODEL(MSX_MSX1)? 2:8;

  /* If changing amount of RAM... */
  if(NewRAMPages!=RAMPages)
  {
    if(Verbose) printf("Allocating %dkB for RAM...",NewRAMPages*16);
    if(P1=GetMemory(NewRAMPages*0x4000))
    {
      memset(P1,NORAM,NewRAMPages*0x4000);
      FreeMemory(RAMData);
      RAMPages = NewRAMPages;
      RAMMask  = NewRAMPages-1;
      RAMData  = P1;
    }
    PRINTRESULT(P1);
  }

  /* If changing amount of VRAM... */
  if(NewVRAMPages!=VRAMPages)
  {
    if(Verbose) printf("Allocating %dkB for VRAM...",NewVRAMPages*16);
    if(P1=GetMemory(NewVRAMPages*0x4000))
    {
      memset(P1,0x00,NewVRAMPages*0x4000);
      FreeMemory(VRAM);
      VRAMPages = NewVRAMPages;
      VRAM      = P1;
    }
    PRINTRESULT(P1);
  }

  /* For all slots... */
  for(J=0;J<4;++J)
  {
    /* Slot is currently read-only */
    EnWrite[J]          = 0;
    /* PSL=0:0:0:0, SSL=0:0:0:0 */
    PSL[J]              = 0;
    SSL[J]              = 0;
    /* RAMMap=3:2:1:0 */
    MemMap[3][2][J*2]   = RAMData+(3-J)*0x4000;
    MemMap[3][2][J*2+1] = MemMap[3][2][J*2]+0x2000;
    RAMMapper[J]        = 3-J;
    /* Setting address space */
    RAM[J*2]            = MemMap[0][0][J*2];
    RAM[J*2+1]          = MemMap[0][0][J*2+1];
  }

  /* For all MegaROMs... */
  for(J=0;J<MAXSLOTS;++J)
    if((I=ROMMask[J]+1)>4)
    {
      /* For normal MegaROMs, set first four pages */
      if((ROMData[J][0]=='A')&&(ROMData[J][1]=='B'))
        SetMegaROM(J,0,1,2,3);
      /* Some MegaROMs default to last pages on reset */
      else if((ROMData[J][(I-2)<<13]=='A')&&(ROMData[J][((I-2)<<13)+1]=='B'))
        SetMegaROM(J,I-2,I-1,I-2,I-1);
      /* If 'AB' signature is not found at the beginning or the end */
      /* then it is not a MegaROM but rather a plain 64kB ROM       */
    }

  /* Reset sound chips */
  Reset8910(&PSG,PSG_CLOCK,0);
  ResetSCC(&SCChip,AY8910_CHANNELS);
  Reset2413(&OPLL,AY8910_CHANNELS);
  Sync8910(&PSG,AY8910_SYNC);
  SyncSCC(&SCChip,SCC_SYNC);
  Sync2413(&OPLL,YM2413_SYNC);

  /* Reset serial I/O */
  Reset8251(&SIO,ComIStream,ComOStream);

  /* Reset PPI chips and slot selectors */
  Reset8255(&PPI);
  PPI.Rout[0]=PSLReg=0x00;
  PPI.Rout[2]=IOReg=0x00;
  SSLReg[0]=0x00;
  SSLReg[1]=0x00;
  SSLReg[2]=0x00;
  SSLReg[3]=0x00;

  /* Reset floppy disk controller */
  Reset1793(&FDC,FDD,WD1793_KEEP);

  /* Reset VDP */
  memcpy(VDP,VDPInit,sizeof(VDP));
  memcpy(VDPStatus,VDPSInit,sizeof(VDPStatus));

  /* Reset keyboard */
  memset((void *)KeyState,0xFF,16);

  /* Set initial palette */
  for(J=0;J<16;++J)
  {
    Palette[J]=PalInit[J];
    SetColor(J,(Palette[J]>>16)&0xFF,(Palette[J]>>8)&0xFF,Palette[J]&0xFF);
  }

  /* Reset mouse coordinates/counters */
  for(J=0;J<2;++J)
    MouState[J]=MouseDX[J]=MouseDY[J]=OldMouseX[J]=OldMouseY[J]=MCount[J]=0;

  IRQPending=0x00;                      /* No IRQs pending  */
  SCCOn[0]=SCCOn[1]=0;                  /* SCCs off for now */
  RTCReg=RTCMode=0;                     /* Clock registers  */
  KanCount=0;KanLetter=0;               /* Kanji extension  */
  ChrTab=ColTab=ChrGen=VRAM;            /* VDP tables       */
  SprTab=SprGen=VRAM;
  ChrTabM=ColTabM=ChrGenM=SprTabM=~0;   /* VDP addr. masks  */
  VPAGE=VRAM;                           /* VRAM page        */
  FGColor=BGColor=XFGColor=XBGColor=0;  /* VDP colors       */
  ScrMode=0;                            /* Screen mode      */
  VKey=PKey=1;WKey=0;                   /* VDP keys         */
  VAddr=0x0000;                         /* VRAM access addr */
  ScanLine=0;                           /* Current scanline */
  VDPData=NORAM;                        /* VDP data buffer  */
  JoyState=0;                           /* Joystick state   */

  /* Set "V9958" VDP version for MSX2+ */
  if(MODEL(MSX_MSX2P)) VDPStatus[1]|=0x04;

  /* Reset CPU */
  ResetZ80(&CPU);

  /* Done */
  return(Mode);
}

/** RdZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** address A in the Z80 address space. Also see OpZ80() in **/
/** Z80.c which is a simplified code-only RdZ80() version.  **/
/*************************************************************/
byte RdZ80(word A)
{
  /* Filter out everything but [xx11 1111 1xxx 1xxx] */
  if((A&0x3F88)!=0x3F88) return(RAM[A>>13][A&0x1FFF]);

  /* Secondary slot selector */
  if(A==0xFFFF) return(~SSLReg[PSL[3]]);

  /* Floppy disk controller */
  /* 7FF8h..7FFFh Standard DiskROM  */
  /* BFF8h..BFFFh MSX-DOS BDOS      */
  /* 7F80h..7F87h Arabic DiskROM    */
  /* 7FB8h..7FBFh SV738/TechnoAhead */
  if((PSL[A>>14]==3)&&(SSL[A>>14]==1))
    switch(A)
    {
      /* Standard      MSX-DOS       Arabic        SV738            */
      case 0x7FF8: case 0xBFF8: case 0x7F80: case 0x7FB8: /* STATUS */
      case 0x7FF9: case 0xBFF9: case 0x7F81: case 0x7FB9: /* TRACK  */
      case 0x7FFA: case 0xBFFA: case 0x7F82: case 0x7FBA: /* SECTOR */
      case 0x7FFB: case 0xBFFB: case 0x7F83: case 0x7FBB: /* DATA   */
        return(Read1793(&FDC,A&0x0003));
      case 0x7FFF: case 0xBFFF: case 0x7F84: case 0x7FBC: /* SYSTEM */
        return(Read1793(&FDC,WD1793_READY));
    }

  /* Default to reading memory */
  return(RAM[A>>13][A&0x1FFF]);
}

/** WrZ80() **************************************************/
/** Z80 emulation calls this function to write byte V to    **/
/** address A of Z80 address space.                         **/
/*************************************************************/
void WrZ80(word A,byte V)
{
  /* Secondary slot selector */
  if(A==0xFFFF) { SSlot(V);return; }

  /* Floppy disk controller */
  /* 7FF8h..7FFFh Standard DiskROM  */
  /* BFF8h..BFFFh MSX-DOS BDOS      */
  /* 7F80h..7F87h Arabic DiskROM    */
  /* 7FB8h..7FBFh SV738/TechnoAhead */
  if(((A&0x3F88)==0x3F88)&&(PSL[A>>14]==3)&&(SSL[A>>14]==1))
    switch(A)
    {
      /* Standard      MSX-DOS       Arabic        SV738             */
      case 0x7FF8: case 0xBFF8: case 0x7F80: case 0x7FB8: /* COMMAND */
      case 0x7FF9: case 0xBFF9: case 0x7F81: case 0x7FB9: /* TRACK   */
      case 0x7FFA: case 0xBFFA: case 0x7F82: case 0x7FBA: /* SECTOR  */
      case 0x7FFB: case 0xBFFB: case 0x7F83: case 0x7FBB: /* DATA    */
        Write1793(&FDC,A&0x0003,V);
        return;
      case 0xBFFC: /* Standard/MSX-DOS */
      case 0x7FFC: /* Side: [xxxxxxxS] */
        Write1793(&FDC,WD1793_SYSTEM,FDC.Drive|S_DENSITY|(V&0x01? 0:S_SIDE));
        return;
      case 0xBFFD: /* Standard/MSX-DOS  */
      case 0x7FFD: /* Drive: [xxxxxxxD] */
        Write1793(&FDC,WD1793_SYSTEM,(V&0x01)|S_DENSITY|(FDC.Side? 0:S_SIDE));
        return;
      case 0x7FBC: /* Arabic/SV738 */
      case 0x7F84: /* Side/Drive/Motor: [xxxxMSDD] */
        Write1793(&FDC,WD1793_SYSTEM,(V&0x03)|S_DENSITY|(V&0x04? 0:S_SIDE));
        return;
    }

  /* Write to RAM, if enabled */
  if(EnWrite[A>>14]) { RAM[A>>13][A&0x1FFF]=V;return; }

  /* Switch MegaROM pages */
  if((A>0x3FFF)&&(A<0xC000)) MapROM(A,V);
}

/** InZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** a given I/O port.                                       **/
/*************************************************************/
byte InZ80(word Port)
{
  /* MSX only uses 256 IO ports */
  Port&=0xFF;

  /* Return an appropriate port value */
  switch(Port)
  {

case 0x90: return(0xFD);                   /* Printer READY signal */
case 0xB5: return(RTCIn(RTCReg));          /* RTC registers        */

case 0xA8: /* Primary slot state   */
case 0xA9: /* Keyboard port        */
case 0xAA: /* General IO register  */
case 0xAB: /* PPI control register */
  PPI.Rin[1]=KeyState[PPI.Rout[2]&0x0F];
  return(Read8255(&PPI,Port-0xA8));

case 0xFC: /* Mapper page at 0000h */
case 0xFD: /* Mapper page at 4000h */
case 0xFE: /* Mapper page at 8000h */
case 0xFF: /* Mapper page at C000h */
  return(RAMMapper[Port-0xFC]|~RAMMask);

case 0xD9: /* Kanji support */
  Port=Kanji? Kanji[KanLetter+KanCount]:NORAM;
  KanCount=(KanCount+1)&0x1F;
  return(Port);

case 0x80: /* SIO data */
case 0x81:
case 0x82:
case 0x83:
case 0x84:
case 0x85:
case 0x86:
case 0x87:
  return(NORAM);
  /*return(Rd8251(&SIO,Port&0x07));*/

case 0x98: /* VRAM read port */
  /* Read from VRAM data buffer */
  Port=VDPData;
  /* Reset VAddr latch sequencer */
  VKey=1;
  /* Fill data buffer with a new value */
  VDPData=VPAGE[VAddr];
  /* Increment VRAM address */
  VAddr=(VAddr+1)&0x3FFF;
  /* If rolled over, modify VRAM page# */
  if(!VAddr&&(ScrMode>3))
  {
    VDP[14]=(VDP[14]+1)&(VRAMPages-1);
    VPAGE=VRAM+((int)VDP[14]<<14);
  }
  return(Port);

case 0x99: /* VDP status registers */
  /* Read an appropriate status register */
  Port=VDPStatus[VDP[15]];
  /* Reset VAddr latch sequencer */
// @@@ This breaks Sir Lancelot on ColecoVision, so it must be wrong! 
//  VKey=1;
  /* Update status register's contents */
  switch(VDP[15])
  {
    case 0: VDPStatus[0]&=0x5F;SetIRQ(~INT_IE0);break;
    case 1: VDPStatus[1]&=0xFE;SetIRQ(~INT_IE1);break;
    case 7: VDPStatus[7]=VDP[44]=VDPRead();break;
  }
  /* Return the status register value */
  return(Port);

case 0xA2: /* PSG input port */
  /* PSG[14] returns joystick/mouse data */
  if(PSG.Latch==14)
  {
    int DX,DY,L,J;

    /* Number of a joystick port */
    Port = (PSG.R[15]&0x40)>>6;
    L    = JOYTYPE(Port);

    /* If no joystick, return dummy value */
    if(L==JOY_NONE) return(0x7F);

    /* Compute mouse offsets, if needed */
    if(MCount[Port]==1)
    {
      /* Get new mouse coordinates */
      DX=MouState[Port]&0xFF;
      DY=(MouState[Port]>>8)&0xFF;
      /* Compute offsets and store coordinates  */
      J=OldMouseX[Port]-DX;OldMouseX[Port]=DX;DX=J;
      J=OldMouseY[Port]-DY;OldMouseY[Port]=DY;DY=J;
      /* For 512-wide mode, double horizontal offset */
      if((ScrMode==6)||((ScrMode==7)&&!ModeYJK)||(ScrMode==MAXSCREEN+1)) DX<<=1;
      /* Adjust offsets */
      MouseDX[Port]=(DX>127? 127:(DX<-127? -127:DX))&0xFF;
      MouseDY[Port]=(DY>127? 127:(DY<-127? -127:DY))&0xFF;
    }

    /* Get joystick state */
    J=~(Port? (JoyState>>8):JoyState)&0x3F;

    /* Determine return value */
    switch(MCount[Port])
    {
      case 0: Port=PSG.R[15]&(0x10<<Port)? 0x3F:J;break;
      case 1: Port=(MouseDX[Port]>>4)|(J&0x30);break;
      case 2: Port=(MouseDX[Port]&0x0F)|(J&0x30);break;
      case 3: Port=(MouseDY[Port]>>4)|(J&0x30);break;
      case 4: Port=(MouseDY[Port]&0x0F)|(J&0x30);break;
    }

    /* 6th bit is always 1 */
    return(Port|0x40);
  }

  /* PSG[15] resets mouse counters (???) */
  if(PSG.Latch==15)
  {
    /* @@@ For debugging purposes */
    /*printf("Reading from PSG[15]\n");*/

    /*MCount[0]=MCount[1]=0;*/
    return(PSG.R[15]&0xF0);
  }

  /* Return PSG[0-13] as they are */
  return(RdData8910(&PSG));

case 0xD0: /* FDC status  */
case 0xD1: /* FDC track   */
case 0xD2: /* FDC sector  */
case 0xD3: /* FDC data    */
case 0xD4: /* FDC IRQ/DRQ */
  /* Brazilian DiskROM I/O ports */
  return(Read1793(&FDC,Port-0xD0));

  }

  /* Return NORAM for non-existing ports */
  if(Verbose&0x20) printf("I/O: Read from unknown PORT[%02Xh]\n",Port);
  return(NORAM);
}

/** OutZ80() *************************************************/
/** Z80 emulation calls this function to write byte V to a  **/
/** given I/O port.                                         **/
/*************************************************************/
void OutZ80(word Port,byte Value)
{
  register byte I,J;

  Port&=0xFF;
  switch(Port)
  {

case 0x7C: WrCtrl2413(&OPLL,Value);return;        /* OPLL Register# */
case 0x7D: WrData2413(&OPLL,Value);return;        /* OPLL Data      */
case 0x91: Printer(Value);return;                 /* Printer Data   */
case 0xA0: WrCtrl8910(&PSG,Value);return;         /* PSG Register#  */
case 0xB4: RTCReg=Value&0x0F;return;              /* RTC Register#  */ 

case 0xD8: /* Upper bits of Kanji ROM address */
  KanLetter=(KanLetter&0x1F800)|((int)(Value&0x3F)<<5);
  KanCount=0;
  return;

case 0xD9: /* Lower bits of Kanji ROM address */
  KanLetter=(KanLetter&0x007E0)|((int)(Value&0x3F)<<11);
  KanCount=0;
  return;

case 0x80: /* SIO data */
case 0x81:
case 0x82:
case 0x83:
case 0x84:
case 0x85:
case 0x86:
case 0x87:
  return;
  /*Wr8251(&SIO,Port&0x07,Value);
  return;*/

case 0x98: /* VDP Data */
  VKey=1;
  if(WKey)
  {
    /* VDP set for writing */
    VDPData=VPAGE[VAddr]=Value;
    VAddr=(VAddr+1)&0x3FFF;
  }
  else
  {
    /* VDP set for reading */
    VDPData=VPAGE[VAddr];
    VAddr=(VAddr+1)&0x3FFF;
    VPAGE[VAddr]=Value;
  }
  /* If VAddr rolled over, modify VRAM page# */
  if(!VAddr&&(ScrMode>3)) 
  {
    VDP[14]=(VDP[14]+1)&(VRAMPages-1);
    VPAGE=VRAM+((int)VDP[14]<<14);
  }
  return;

case 0x99: /* VDP Address Latch */
  if(VKey) { ALatch=Value;VKey=0; }
  else
  {
    VKey=1;
    switch(Value&0xC0)
    {
      case 0x80:
        /* Writing into VDP registers */
        VDPOut(Value&0x3F,ALatch);
        break;
      case 0x00:
      case 0x40:
        /* Set the VRAM access address */
        VAddr=(((word)Value<<8)+ALatch)&0x3FFF;
        /* WKey=1 when VDP set for writing into VRAM */
        WKey=Value&0x40;
        /* When set for reading, perform first read */
        if(!WKey)
        {
          VDPData=VPAGE[VAddr];
          VAddr=(VAddr+1)&0x3FFF;
          if(!VAddr&&(ScrMode>3))
          {
            VDP[14]=(VDP[14]+1)&(VRAMPages-1);
            VPAGE=VRAM+((int)VDP[14]<<14);
          }
        }
        break;
    }
  }
  return;

case 0x9A: /* VDP Palette Latch */
  if(PKey) { PLatch=Value;PKey=0; }
  else
  {
    byte R,G,B;
    /* New palette entry written */
    PKey=1;
    J=VDP[16];
    /* Compute new color components */
    R=(PLatch&0x70)*255/112;
    G=(Value&0x07)*255/7;
    B=(PLatch&0x07)*255/7;
    /* Set new color for palette entry J */
    Palette[J]=RGB2INT(R,G,B);
    SetColor(J,R,G,B);
    /* Next palette entry */
    VDP[16]=(J+1)&0x0F;
  }
  return;

case 0x9B: /* VDP Register Access */
  J=VDP[17]&0x3F;
  if(J!=17) VDPOut(J,Value);
  if(!(VDP[17]&0x80)) VDP[17]=(J+1)&0x3F;
  return;

case 0xA1: /* PSG Data */
  /* PSG[15] is responsible for joystick/mouse */
  if(PSG.Latch==15)
  {
    /* @@@ For debugging purposes */
    /*printf("Writing PSG[15] <- %02Xh\n",Value);*/

    /* For mouse, update nibble counter      */
    /* For joystick, set nibble counter to 0 */
    if((Value&0x0C)==0x0C) MCount[1]=0;
    else if((JOYTYPE(1)==JOY_MOUSE)&&((Value^PSG.R[15])&0x20))
           MCount[1]+=MCount[1]==4? -3:1;

    /* For mouse, update nibble counter      */
    /* For joystick, set nibble counter to 0 */
    if((Value&0x03)==0x03) MCount[0]=0;
    else if((JOYTYPE(0)==JOY_MOUSE)&&((Value^PSG.R[15])&0x10))
           MCount[0]+=MCount[0]==4? -3:1;
  }

  /* Put value into a register */
  WrData8910(&PSG,Value);
  return;

case 0xA8: /* Primary slot state   */
case 0xA9: /* Keyboard port        */
case 0xAA: /* General IO register  */
case 0xAB: /* PPI control register */
  /* Write to PPI */
  Write8255(&PPI,Port-0xA8,Value);
  /* If general I/O register has changed... */
  if(PPI.Rout[2]!=IOReg) { PPIOut(PPI.Rout[2],IOReg);IOReg=PPI.Rout[2]; }
  /* If primary slot state has changed... */
  if(PPI.Rout[0]!=PSLReg) PSlot(PPI.Rout[0]);
  /* Done */  
  return;

case 0xB5: /* RTC Data */
  if(RTCReg<13)
  {
    /* J = register bank# now */
    J=RTCMode&0x03;
    /* Store the value */
    RTC[J][RTCReg]=Value;
    /* If CMOS modified, we need to save it */
    if(J>1) SaveCMOS=1;
    return;
  }
  /* RTC[13] is a register bank# */
  if(RTCReg==13) RTCMode=Value;
  return;

case 0xD0: /* FDC command */
case 0xD1: /* FDC track   */
case 0xD2: /* FDC sector  */
case 0xD3: /* FDC data    */
  /* Brazilian DiskROM I/O ports */
  Write1793(&FDC,Port-0xD0,Value);
  return;

case 0xD4: /* FDC system  */
  /* Brazilian DiskROM drive/side: [xxxSxxDx] */
  Value=((Value&0x02)>>1)|S_DENSITY|(Value&0x10? 0:S_SIDE);
  Write1793(&FDC,WD1793_SYSTEM,Value);
  return;

case 0xFC: /* Mapper page at 0000h */
case 0xFD: /* Mapper page at 4000h */
case 0xFE: /* Mapper page at 8000h */
case 0xFF: /* Mapper page at C000h */
  J=Port-0xFC;
  Value&=RAMMask;
  if(RAMMapper[J]!=Value)
  {
    if(Verbose&0x08) printf("RAM-MAPPER: block %d at %Xh\n",Value,J*0x4000);
    I=J<<1;
    RAMMapper[J]      = Value;
    MemMap[3][2][I]   = RAMData+((int)Value<<14);
    MemMap[3][2][I+1] = MemMap[3][2][I]+0x2000;
    if((PSL[J]==3)&&(SSL[J]==2))
    {
      EnWrite[J] = 1;
      RAM[I]     = MemMap[3][2][I];
      RAM[I+1]   = MemMap[3][2][I+1];
    }
  }
  return;

  }

  /* Unknown port */
  if(Verbose&0x20)
    printf("I/O: Write to unknown PORT[%02Xh]=%02Xh\n",Port,Value);
}

/** MapROM() *************************************************/
/** Switch ROM Mapper pages. This function is supposed to   **/
/** be called when ROM page registers are written to.       **/
/*************************************************************/
void MapROM(register word A,register byte V)
{
  byte I,J,PS,SS,*P;

/* @@@ For debugging purposes
printf("(%04Xh) = %02Xh at PC=%04Xh\n",A,V,CPU.PC.W);
*/

  J  = A>>14;           /* 16kB page number 0-3  */
  PS = PSL[J];          /* Primary slot number   */
  SS = SSL[J];          /* Secondary slot number */
  I  = CartMap[PS][SS]; /* Cartridge number      */

  /* Drop out if no cartridge in that slot */
  if(I>=MAXSLOTS) return;

  /* SCC: enable/disable for no cart */
  if(!ROMData[I]&&(A==0x9000)) SCCOn[I]=(V==0x3F)? 1:0;

  /* SCC: types 0, 2, or no cart */
  if(((A&0xFF00)==0x9800)&&SCCOn[I])
  {
    /* Compute SCC register number */
    J=A&0x00FF;

    /* When no MegaROM present, we allow the program */
    /* to write into SCC wave buffer using EmptyRAM  */
    /* as a scratch pad.                             */
    if(!ROMData[I]&&(J<0x80)) EmptyRAM[0x1800+J]=V;

    /* Output data to SCC chip */
    WriteSCC(&SCChip,J,V);
    return;
  }

  /* SCC+: types 0, 2, or no cart */
  if(((A&0xFF00)==0xB800)&&SCCOn[I])
  {
    /* Compute SCC register number */
    J=A&0x00FF;

    /* When no MegaROM present, we allow the program */
    /* to write into SCC wave buffer using EmptyRAM  */
    /* as a scratch pad.                             */
    if(!ROMData[I]&&(J<0xA0)) EmptyRAM[0x1800+J]=V;

    /* Output data to SCC chip */
    WriteSCCP(&SCChip,J,V);
    return;
  }

  /* If no cartridge or no mapper, exit */
  if(!ROMData[I]||!ROMMask[I]) return;

  switch(ROMType[I])
  {
    case MAP_GEN8: /* Generic 8kB cartridges (Konami, etc.) */
      /* Only interested in writes to 4000h-BFFFh */
      if((A<0x4000)||(A>0xBFFF)) break;
      J=(A-0x4000)>>13;
      /* Turn SCC on/off on writes to 8000h-9FFFh */
      if(J==2) SCCOn[I]=(V==0x3F)? 1:0;
      /* Switch ROM pages */
      V&=ROMMask[I];
      if(V!=ROMMapper[I][J])
      {
        RAM[J+2]=MemMap[PS][SS][J+2]=ROMData[I]+((int)V<<13);
        ROMMapper[I][J]=V;
      }
      if(Verbose&0x08)
        printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n",I+'A',V,PS,SS,J*0x2000+0x4000);
      return;

    case MAP_GEN16: /* Generic 16kB cartridges (MSXDOS2, HoleInOneSpecial) */
      /* Only interested in writes to 4000h-BFFFh */
      if((A<0x4000)||(A>0xBFFF)) break;
      J=(A&0x8000)>>14;
      /* Switch ROM pages */
      V=(V<<1)&ROMMask[I];
      if(V!=ROMMapper[I][J])
      {
        RAM[J+2]=MemMap[PS][SS][J+2]=ROMData[I]+((int)V<<13);
        RAM[J+3]=MemMap[PS][SS][J+3]=RAM[J+2]+0x2000;
        ROMMapper[I][J]=V;
      }
      if(Verbose&0x08)
        printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n",I+'A',V>>1,PS,SS,J*0x2000+0x4000);
      return;

    case MAP_KONAMI5: /* KONAMI5 8kB cartridges */
      /* Only interested in writes to 5000h/7000h/9000h/B000h */
      if((A<0x5000)||(A>0xB000)||((A&0x1FFF)!=0x1000)) break;
      J=(A-0x5000)>>13;
      /* Turn SCC on/off on writes to 9000h */
      if(J==2) SCCOn[I]=(V==0x3F)? 1:0;
      /* Switch ROM pages */
      V&=ROMMask[I];
      if(V!=ROMMapper[I][J])
      {
        RAM[J+2]=MemMap[PS][SS][J+2]=ROMData[I]+((int)V<<13);
        ROMMapper[I][J]=V;
      }
      if(Verbose&0x08)
        printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n",I+'A',V,PS,SS,J*0x2000+0x4000);
      return;

    case MAP_KONAMI4: /* KONAMI4 8kB cartridges */
      /* Only interested in writes to 6000h/8000h/A000h */
      /* (page at 4000h is fixed) */
      if((A<0x6000)||(A>0xA000)||(A&0x1FFF)) break;
      J=(A-0x4000)>>13;
      /* Switch ROM pages */
      V&=ROMMask[I];
      if(V!=ROMMapper[I][J])
      {
        RAM[J+2]=MemMap[PS][SS][J+2]=ROMData[I]+((int)V<<13);
        ROMMapper[I][J]=V;
      }
      if(Verbose&0x08)
        printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n",I+'A',V,PS,SS,J*0x2000+0x4000);
      return;

    case MAP_ASCII8: /* ASCII 8kB cartridges */
      /* If switching pages... */
      if((A>=0x6000)&&(A<0x8000))
      {
        J=(A&0x1800)>>11;
        /* If selecting SRAM... */
        if(V&(ROMMask[I]+1))
        {
          /* Select SRAM page */
          V=0xFF;
          P=SRAMData[I];
          if(Verbose&0x08)
            printf("ROM-MAPPER %c: 8kB SRAM at %d:%d:%04Xh\n",I+'A',PS,SS,J*0x2000+0x4000);
        }
        else
        {
          /* Select ROM page */
          V&=ROMMask[I];
          P=ROMData[I]+((int)V<<13);
          if(Verbose&0x08)
            printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n",I+'A',V,PS,SS,J*0x2000+0x4000);
        }
        /* If page was actually changed... */
        if(V!=ROMMapper[I][J])
        {
          MemMap[PS][SS][J+2]=P;
          ROMMapper[I][J]=V;
          /* Only update memory when cartridge's slot selected */
          if((PSL[(J>>1)+1]==PS)&&(SSL[(J>>1)+1]==SS)) RAM[J+2]=P;
        }
        /* Done with page switch */
        return;
      }
      /* Write to SRAM */
      if((A>=0x8000)&&(A<0xC000)&&(ROMMapper[I][((A>>13)&1)+2]==0xFF))
      {
        RAM[A>>13][A&0x1FFF]=V;
        SaveSRAM[I]=1;
        /* Done with SRAM write */
        return;
      }
      break;

    case MAP_ASCII16: /*** ASCII 16kB cartridges ***/
      /* If switching pages... */
      if((A>=0x6000)&&(A<0x8000))
      {
        J=(A&0x1000)>>11;
        /* If selecting SRAM... */
        if(V&(ROMMask[I]+1))
        {
          /* Select SRAM page */
          V=0xFF;
          P=SRAMData[I];
          if(Verbose&0x08)
            printf("ROM-MAPPER %c: 2kB SRAM at %d:%d:%04Xh\n",I+'A',PS,SS,J*0x2000+0x4000);
        }
        else
        {
          /* Select ROM page */
          V=(V<<1)&ROMMask[I];
          P=ROMData[I]+((int)V<<13);
          if(Verbose&0x08)
            printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n",I+'A',V>>1,PS,SS,J*0x2000+0x4000);
        }
        /* If page was actually changed... */
        if(V!=ROMMapper[I][J])
        {
          MemMap[PS][SS][J+2]=P;
          MemMap[PS][SS][J+3]=P+0x2000;
          ROMMapper[I][J]=V;
          /* Only update memory when cartridge's slot selected */
          if((PSL[(J>>1)+1]==PS)&&(SSL[(J>>1)+1]==SS))
          {
            RAM[J+2]=P;
            RAM[J+3]=P+0x2000;
          }
        }
        /* Done with page switch */
        return;
      }
      /* Write to SRAM */
      if((A>=0x8000)&&(A<0xC000)&&(ROMMapper[I][2]==0xFF))
      {
        P=RAM[A>>13];
        A&=0x07FF;
        P[A+0x0800]=P[A+0x1000]=P[A+0x1800]=
        P[A+0x2000]=P[A+0x2800]=P[A+0x3000]=
        P[A+0x3800]=P[A]=V;
        SaveSRAM[I]=1;
        /* Done with SRAM write */
        return;
      }
      break;

    case MAP_GMASTER2: /* Konami GameMaster2+SRAM cartridge */
      /* Switch ROM and SRAM pages, page at 4000h is fixed */
      if((A>=0x6000)&&(A<=0xA000)&&!(A&0x1FFF))
      {
        /* Figure out which ROM page gets switched */
        J=(A-0x4000)>>13;
        /* If changing SRAM page... */
        if(V&0x10)
        {
          /* Select SRAM page */
          RAM[J+2]=MemMap[PS][SS][J+2]=SRAMData[I]+(V&0x20? 0x2000:0);
          /* SRAM is now on */
          ROMMapper[I][J]=0xFF;
          if(Verbose&0x08)
            printf("GMASTER2 %c: 4kB SRAM page #%d at %d:%d:%04Xh\n",I+'A',(V&0x20)>>5,PS,SS,J*0x2000+0x4000);
        }
        else
        {
          /* Compute new ROM page number */
          V&=ROMMask[I];
          /* If ROM page number has changed... */
          if(V!=ROMMapper[I][J])
          {
            RAM[J+2]=MemMap[PS][SS][J+2]=ROMData[I]+((int)V<<13);
            ROMMapper[I][J]=V;
          }
          if(Verbose&0x08)
            printf("GMASTER2 %c: 8kB ROM page #%d at %d:%d:%04Xh\n",I+'A',V,PS,SS,J*0x2000+0x4000);
        }
        /* Done with page switch */
        return;
      }
      /* Write to SRAM */
      if((A>=0xB000)&&(A<0xC000)&&(ROMMapper[I][3]==0xFF))
      {
        RAM[5][(A&0x0FFF)|0x1000]=RAM[5][A&0x0FFF]=V;
        SaveSRAM[I]=1;
        /* Done with SRAM write */
        return;
      }
      break;

    case MAP_FMPAC: /* Panasonic FMPAC+SRAM cartridge */
      /* See if any switching occurs */
      switch(A)
      {
        case 0x7FF7: /* ROM page select */
          V=(V<<1)&ROMMask[I];
          ROMMapper[I][0]=V;
          /* 4000h-5FFFh contains SRAM when correct FMPACKey supplied */
          if(FMPACKey!=FMPAC_MAGIC)
          {
            P=ROMData[I]+((int)V<<13);
            RAM[2]=MemMap[PS][SS][2]=P;
            RAM[3]=MemMap[PS][SS][3]=P+0x2000;
          }
          if(Verbose&0x08)
            printf("FMPAC %c: 16kB ROM page #%d at %d:%d:4000h\n",I+'A',V>>1,PS,SS);
          return;
        case 0x7FF6: /* OPL1 enable/disable? */
          if(Verbose&0x08)
            printf("FMPAC %c: (7FF6h) = %02Xh\n",I+'A',V);
          V&=0x11;
          return;
        case 0x5FFE: /* Write 4Dh, then (5FFFh)=69h to enable SRAM */
        case 0x5FFF: /* (5FFEh)=4Dh, then write 69h to enable SRAM */
          FMPACKey=A&1? ((FMPACKey&0x00FF)|((int)V<<8))
                      : ((FMPACKey&0xFF00)|V);
          P=FMPACKey==FMPAC_MAGIC?
            SRAMData[I]:(ROMData[I]+((int)ROMMapper[I][0]<<13));
          RAM[2]=MemMap[PS][SS][2]=P;
          RAM[3]=MemMap[PS][SS][3]=P+0x2000;
          if(Verbose&0x08)
            printf("FMPAC %c: 8kB SRAM %sabled at %d:%d:4000h\n",I+'A',FMPACKey==FMPAC_MAGIC? "en":"dis",PS,SS);
          return;
      }
      /* Write to SRAM */
      if((A>=0x4000)&&(A<0x5FFE)&&(FMPACKey==FMPAC_MAGIC))
      {
        RAM[A>>13][A&0x1FFF]=V;
        SaveSRAM[I]=1;
        return;
      }
      break;
  }

  /* No MegaROM mapper or there is an incorrect write */     
  if(Verbose&0x08) printf("MEMORY: Bad write (%d:%d:%04Xh) = %02Xh\n",PS,SS,A,V);
}

/** PSlot() **************************************************/
/** Switch primary memory slots. This function is called    **/
/** when value in port A8h changes.                         **/
/*************************************************************/
void PSlot(register byte V)
{
  register byte J,I;
  
  if(PSLReg!=V)
    for(PSLReg=V,J=0;J<4;++J,V>>=2)
    {
      I          = J<<1;
      PSL[J]     = V&3;
      SSL[J]     = (SSLReg[PSL[J]]>>I)&3;
      RAM[I]     = MemMap[PSL[J]][SSL[J]][I];
      RAM[I+1]   = MemMap[PSL[J]][SSL[J]][I+1];
      EnWrite[J] = (PSL[J]==3)&&(SSL[J]==2)&&(MemMap[3][2][I]!=EmptyRAM);
    }
}

/** SSlot() **************************************************/
/** Switch secondary memory slots. This function is called  **/
/** when value in (FFFFh) changes.                          **/
/*************************************************************/
void SSlot(register byte V)
{
  register byte J,I;

  /* Cartridge slots do not have subslots, fix them at 0:0:0:0 */
  if((PSL[3]==1)||(PSL[3]==2)) V=0x00;
  /* In MSX1, slot 0 does not have subslots either */
  if(!PSL[3]&&((Mode&MSX_MODEL)==MSX_MSX1)) V=0x00;

  if(SSLReg[PSL[3]]!=V)
    for(SSLReg[PSL[3]]=V,J=0;J<4;++J,V>>=2)
    {
      if(PSL[J]==PSL[3])
      {
        I          = J<<1;
        SSL[J]     = V&3;
        RAM[I]     = MemMap[PSL[J]][SSL[J]][I];
        RAM[I+1]   = MemMap[PSL[J]][SSL[J]][I+1];
        EnWrite[J] = (PSL[J]==3)&&(SSL[J]==2)&&(MemMap[3][2][I]!=EmptyRAM);
      }
    }
}

/** SetIRQ() *************************************************/
/** Set or reset IRQ. Returns IRQ vector assigned to        **/
/** CPU.IRequest. When upper bit of IRQ is 1, IRQ is reset. **/
/*************************************************************/
word SetIRQ(register byte IRQ)
{
  if(IRQ&0x80) IRQPending&=IRQ; else IRQPending|=IRQ;
  CPU.IRequest=IRQPending? INT_IRQ:INT_NONE;
  return(CPU.IRequest);
}

/** SetScreen() **********************************************/
/** Change screen mode. Returns new screen mode.            **/
/*************************************************************/
byte SetScreen(void)
{
  register byte I,J;

  switch(((VDP[0]&0x0E)>>1)|(VDP[1]&0x18))
  {
    case 0x10: J=0;break;
    case 0x00: J=1;break;
    case 0x01: J=2;break;
    case 0x08: J=3;break;
    case 0x02: J=4;break;
    case 0x03: J=5;break;
    case 0x04: J=6;break;
    case 0x05: J=7;break;
    case 0x07: J=8;break;
    case 0x12: J=MAXSCREEN+1;break;
    default:   J=ScrMode;break;
  }

  /* Recompute table addresses */
  I=(J>6)&&(J!=MAXSCREEN+1)? 11:10;
  ChrTab  = VRAM+((int)(VDP[2]&MSK[J].R2)<<I);
  ChrGen  = VRAM+((int)(VDP[4]&MSK[J].R4)<<11);
  ColTab  = VRAM+((int)(VDP[3]&MSK[J].R3)<<6)+((int)VDP[10]<<14);
  SprTab  = VRAM+((int)(VDP[5]&MSK[J].R5)<<7)+((int)VDP[11]<<15);
  SprGen  = VRAM+((int)VDP[6]<<11);
  ChrTabM = ((int)(VDP[2]|~MSK[J].M2)<<I)|((1<<I)-1);
  ChrGenM = ((int)(VDP[4]|~MSK[J].M4)<<11)|0x007FF;
  ColTabM = ((int)(VDP[3]|~MSK[J].M3)<<6)|0x1C03F;
  SprTabM = ((int)(VDP[5]|~MSK[J].M5)<<7)|0x1807F;

  /* Return new screen mode */
  ScrMode=J;
  return(J);
}

/** SetMegaROM() *********************************************/
/** Set MegaROM pages for a given slot. SetMegaROM() always **/
/** assumes 8kB pages.                                      **/
/*************************************************************/
void SetMegaROM(int Slot,byte P0,byte P1,byte P2,byte P3)
{
  byte PS,SS;

  /* @@@ ATTENTION: MUST ADD SUPPORT FOR SRAM HERE!   */
  /* @@@ The FFh value must be treated as a SRAM page */

  /* Slot number must be valid */
  if((Slot<0)||(Slot>=MAXSLOTS)) return;
  /* Find primary/secondary slots */
  for(PS=0;PS<4;++PS)
  {
    for(SS=0;(SS<4)&&(CartMap[PS][SS]!=Slot);++SS);
    if(SS<4) break;
  }
  /* Drop out if slots not found */
  if(PS>=4) return;

  /* Apply masks to ROM pages */
  P0&=ROMMask[Slot];
  P1&=ROMMask[Slot];
  P2&=ROMMask[Slot];
  P3&=ROMMask[Slot];
  /* Set memory map */
  MemMap[PS][SS][2]=ROMData[Slot]+P0*0x2000;
  MemMap[PS][SS][3]=ROMData[Slot]+P1*0x2000;
  MemMap[PS][SS][4]=ROMData[Slot]+P2*0x2000;
  MemMap[PS][SS][5]=ROMData[Slot]+P3*0x2000;
  /* Set ROM mappers */
  ROMMapper[Slot][0]=P0;
  ROMMapper[Slot][1]=P1;
  ROMMapper[Slot][2]=P2;
  ROMMapper[Slot][3]=P3;
}

/** VDPOut() *************************************************/
/** Write value into a given VDP register.                  **/
/*************************************************************/
void VDPOut(register byte R,register byte V)
{ 
  register byte J;

  switch(R)  
  {
    case  0: /* Reset HBlank interrupt if disabled */
             if((VDPStatus[1]&0x01)&&!(V&0x10))
             {
               VDPStatus[1]&=0xFE;
               SetIRQ(~INT_IE1);
             }
             /* Set screen mode */
             if(VDP[0]!=V) { VDP[0]=V;SetScreen(); }
             break;
    case  1: /* Set/Reset VBlank interrupt if enabled or disabled */
             if(VDPStatus[0]&0x80) SetIRQ(V&0x20? INT_IE0:~INT_IE0);
             /* Set screen mode */
             if(VDP[1]!=V) { VDP[1]=V;SetScreen(); }
             break;
    case  2: J=(ScrMode>6)&&(ScrMode!=MAXSCREEN+1)? 11:10;
             ChrTab  = VRAM+((int)(V&MSK[ScrMode].R2)<<J);
             ChrTabM = ((int)(V|~MSK[ScrMode].M2)<<J)|((1<<J)-1);
             break;
    case  3: ColTab  = VRAM+((int)(V&MSK[ScrMode].R3)<<6)+((int)VDP[10]<<14);
             ColTabM = ((int)(V|~MSK[ScrMode].M3)<<6)|0x1C03F;
             break;
    case  4: ChrGen  = VRAM+((int)(V&MSK[ScrMode].R4)<<11);
             ChrGenM = ((int)(V|~MSK[ScrMode].M4)<<11)|0x007FF;
             break;
    case  5: SprTab  = VRAM+((int)(V&MSK[ScrMode].R5)<<7)+((int)VDP[11]<<15);
             SprTabM = ((int)(V|~MSK[ScrMode].M5)<<7)|0x1807F;
             break;
    case  6: V&=0x3F;SprGen=VRAM+((int)V<<11);break;
    case  7: FGColor=V>>4;BGColor=V&0x0F;break;
    case 10: V&=0x07;
             ColTab=VRAM+((int)(VDP[3]&MSK[ScrMode].R3)<<6)+((int)V<<14);
             break;
    case 11: V&=0x03;
             SprTab=VRAM+((int)(VDP[5]&MSK[ScrMode].R5)<<7)+((int)V<<15);
             break;
    case 14: V&=VRAMPages-1;VPAGE=VRAM+((int)V<<14);
             break;
    case 15: V&=0x0F;break;
    case 16: V&=0x0F;PKey=1;break;
    case 17: V&=0xBF;break;
    case 25: VDP[25]=V;
             SetScreen();
             break;
    case 44: VDPWrite(V);break;
    case 46: VDPDraw(V);break;
  }

  /* Write value into a register */
  VDP[R]=V;
} 

/** Printer() ************************************************/
/** Send a character to the printer.                        **/
/*************************************************************/
void Printer(byte V)
{
  if(!PrnStream)
  {
    PrnStream = PrnName?   fopen(PrnName,"ab"):0;
    PrnStream = PrnStream? PrnStream:stdout;
  }
  fputc(V,PrnStream);
}

/** PPIOut() *************************************************/
/** This function is called on each write to PPI to make    **/
/** key click sound, motor relay clicks, and so on.         **/
/*************************************************************/
void PPIOut(register byte New,register byte Old)
{
  /* Keyboard click bit */
  if((New^Old)&0x80) Drum(DRM_CLICK,64);
  /* Motor relay bit */
  if((New^Old)&0x10) Drum(DRM_CLICK,255);
}

/** RTCIn() **************************************************/
/** Read value from a given RTC register.                   **/
/*************************************************************/
byte RTCIn(register byte R)
{
  static time_t PrevTime;
  static struct tm TM;
  register byte J;
  time_t CurTime;

  /* Only 16 registers/mode */
  R&=0x0F;

  /* Bank mode 0..3 */
  J=RTCMode&0x03;

  if(R>12) J=R==13? RTCMode:NORAM;
  else
    if(J) J=RTC[J][R];
    else
    {
      /* Retrieve system time if any time passed */
      CurTime=time(NULL);
      if(CurTime!=PrevTime)
      {
        TM=*localtime(&CurTime);
        PrevTime=CurTime;
      }

      /* Parse contents of last retrieved TM */
      switch(R)
      {
        case 0:  J=TM.tm_sec%10;break;
        case 1:  J=TM.tm_sec/10;break;
        case 2:  J=TM.tm_min%10;break;
        case 3:  J=TM.tm_min/10;break;
        case 4:  J=TM.tm_hour%10;break;
        case 5:  J=TM.tm_hour/10;break;
        case 6:  J=TM.tm_wday;break;
        case 7:  J=TM.tm_mday%10;break;
        case 8:  J=TM.tm_mday/10;break;
        case 9:  J=(TM.tm_mon+1)%10;break;
        case 10: J=(TM.tm_mon+1)/10;break;
        case 11: J=(TM.tm_year-80)%10;break;
        case 12: J=((TM.tm_year-80)/10)%10;break;
        default: J=0x0F;break;
      } 
    }

  /* Four upper bits are always high */
  return(J|0xF0);
}

/** LoopZ80() ************************************************/
/** Refresh screen, check keyboard and sprites. Call this   **/
/** function on each interrupt.                             **/
/*************************************************************/
word LoopZ80(Z80 *R)
{
  static byte BFlag=0;
  static byte BCount=0;
  static int  UCount=0;
  static byte ACount=0;
  static byte Drawing=0;
  register int J;

  /* Flip HRefresh bit */
  VDPStatus[2]^=0x20;

  /* If HRefresh is now in progress... */
  if(!(VDPStatus[2]&0x20))
  {
    /* HRefresh takes most of the scanline */
    R->IPeriod=!ScrMode||(ScrMode==MAXSCREEN+1)? CPU_H240:CPU_H256;

    /* New scanline */
    ScanLine=ScanLine<(PALVideo? 312:261)? ScanLine+1:0;

    /* If first scanline of the screen... */
    if(!ScanLine)
    {
      /* Drawing now... */
      Drawing=1;

      /* Reset VRefresh bit */
      VDPStatus[2]&=0xBF;

      /* Refresh display */
      if(UCount>=100) { UCount-=100;RefreshScreen(); }
      UCount+=UPeriod;

      /* Blinking for TEXT80 */
      if(BCount) BCount--;
      else
      {
        BFlag=!BFlag;
        if(!VDP[13]) { XFGColor=FGColor;XBGColor=BGColor; }
        else
        {
          BCount=(BFlag? VDP[13]&0x0F:VDP[13]>>4)*10;
          if(BCount)
          {
            if(BFlag) { XFGColor=FGColor;XBGColor=BGColor; }
            else      { XFGColor=VDP[12]>>4;XBGColor=VDP[12]&0x0F; }
          }
        }
      }
    }

    /* Line coincidence is active at 0..255 */
    /* in PAL and 0..234/244 in NTSC        */
    J=PALVideo? 256:ScanLines212? 245:235;

    /* When reaching end of screen, reset line coincidence */
    if(ScanLine==J)
    {
      VDPStatus[1]&=0xFE;
      SetIRQ(~INT_IE1);
    }

    /* When line coincidence is active... */
    if(ScanLine<J)
    {
      /* Line coincidence processing */
      J=(((ScanLine+VScroll)&0xFF)-VDP[19])&0xFF;
      if(J==2)
      {
        /* Set HBlank flag on line coincidence */
        VDPStatus[1]|=0x01;
        /* Generate IE1 interrupt */
        if(VDP[0]&0x10) SetIRQ(INT_IE1);
      }
      else
      {
        /* Reset flag immediately if IE1 interrupt disabled */
        if(!(VDP[0]&0x10)) VDPStatus[1]&=0xFE;
      }
    }

    /* Return whatever interrupt is pending */
    R->IRequest=IRQPending? INT_IRQ:INT_NONE;
    return(R->IRequest);
  }

  /*********************************/
  /* We come here for HBlanks only */
  /*********************************/

  /* HBlank takes HPeriod-HRefresh */
  R->IPeriod=!ScrMode||(ScrMode==MAXSCREEN+1)? CPU_H240:CPU_H256;
  R->IPeriod=HPeriod-R->IPeriod;

  /* If last scanline of VBlank, see if we need to wait more */
  J=PALVideo? 313:262;
  if(ScanLine>=J-1)
  {
    J*=CPU_HPERIOD;
    if(VPeriod>J) R->IPeriod+=VPeriod-J;
  }

  /* If first scanline of the bottom border... */
  if(ScanLine==(ScanLines212? 212:192)) Drawing=0;

  /* If first scanline of VBlank... */
  J=PALVideo? (ScanLines212? 212+42:192+52):(ScanLines212? 212+18:192+28);
  if(!Drawing&&(ScanLine==J))
  {
    /* Set VBlank bit, set VRefresh bit */
    VDPStatus[0]|=0x80;
    VDPStatus[2]|=0x40;

    /* Generate VBlank interrupt */
    if(VDP[1]&0x20) SetIRQ(INT_IE0);
  }

  /* Run V9938 engine */
  LoopVDP();

  /* Refresh scanline, possibly with the overscan */
  if((UCount>=100)&&Drawing&&(ScanLine<256))
  {
    if(!ModeYJK||(ScrMode<7)||(ScrMode>8))
      (RefreshLine[ScrMode])(ScanLine);
    else
      if(ModeYAE) RefreshLine10(ScanLine);
      else RefreshLine12(ScanLine);
  }

  /* Keyboard, sound, and other stuff always runs at line 192    */
  /* This way, it can't be shut off by overscan tricks (Maarten) */
  if(ScanLine==192)
  {
    /* Check sprites and set Collision, 5Sprites, 5thSprite bits */
    if(!SpritesOFF&&ScrMode&&(ScrMode<MAXSCREEN+1)) CheckSprites();

    /* Count MIDI ticks and update AY8910 state */
    J=1000*VPeriod/CPU_CLOCK;
    MIDITicks(J);
    Loop8910(&PSG,J);

    /* Flush changes to the sound channels */
    Sync8910(&PSG,AY8910_FLUSH|(OPTION(MSX_DRUMS)? AY8910_DRUMS:0));
    SyncSCC(&SCChip,SCC_FLUSH);
    Sync2413(&OPLL,YM2413_FLUSH);

    /* Apply RAM-based cheats */
    if(CheatsON&&CheatCount) ApplyCheats();

    /* Check joystick */
    JoyState=Joystick();

    /* Check keyboard */
    Keyboard();

    /* Exit emulation if requested */
    if(ExitNow) return(INT_QUIT);

    /* Check mouse in joystick port #1 */
    if(JOYTYPE(0)>=JOY_MOUSTICK)
    {
      /* Get new mouse state */
      MouState[0]=Mouse(0);
      /* Merge mouse buttons into joystick buttons */
      JoyState|=(MouState[0]>>12)&0x0030;
      /* If mouse-as-joystick... */
      if(JOYTYPE(0)==JOY_MOUSTICK)
      {
        J=MouState[0]&0xFF;
        JoyState|=J>OldMouseX[0]? 0x0008:J<OldMouseX[0]? 0x0004:0;
        OldMouseX[0]=J;
        J=(MouState[0]>>8)&0xFF;
        JoyState|=J>OldMouseY[0]? 0x0002:J<OldMouseY[0]? 0x0001:0;
        OldMouseY[0]=J;
      }
    }

    /* Check mouse in joystick port #2 */
    if(JOYTYPE(1)>=JOY_MOUSTICK)
    {
      /* Get new mouse state */
      MouState[1]=Mouse(1);
      /* Merge mouse buttons into joystick buttons */
      JoyState|=(MouState[1]>>4)&0x3000;
      /* If mouse-as-joystick... */
      if(JOYTYPE(1)==JOY_MOUSTICK)
      {
        J=MouState[1]&0xFF;
        JoyState|=J>OldMouseX[1]? 0x0800:J<OldMouseX[1]? 0x0400:0;
        OldMouseX[1]=J;
        J=(MouState[1]>>8)&0xFF;
        JoyState|=J>OldMouseY[1]? 0x0200:J<OldMouseY[1]? 0x0100:0;
        OldMouseY[1]=J;
      }
    }

    /* If any autofire options selected, run autofire counter */
    if(OPTION(MSX_AUTOSPACE|MSX_AUTOFIREA|MSX_AUTOFIREB))
      if((ACount=(ACount+1)&0x07)>3)
      {
        /* Autofire spacebar if needed */
        if(OPTION(MSX_AUTOSPACE)) KBD_RES(' ');
        /* Autofire FIRE-A if needed */
        if(OPTION(MSX_AUTOFIREA)) JoyState&=~(JST_FIREA|(JST_FIREA<<8));
        /* Autofire FIRE-B if needed */
        if(OPTION(MSX_AUTOFIREB)) JoyState&=~(JST_FIREB|(JST_FIREB<<8));
      }
  }

  /* Return whatever interrupt is pending */
  R->IRequest=IRQPending? INT_IRQ:INT_NONE;
  return(R->IRequest);
}

/** CheckSprites() *******************************************/
/** Check for sprite collisions and 5th/9th sprite in a     **/
/** row.                                                    **/
/*************************************************************/
void CheckSprites(void)
{
  register word LS,LD;
  register byte DH,DV,*PS,*PD,*T;
  byte I,J,N,M,*S,*D;

  /* Clear 5Sprites, Collision, and 5thSprite bits */
  VDPStatus[0]=(VDPStatus[0]&0x9F)|0x1F;

  for(N=0,S=SprTab;(N<32)&&(S[0]!=208);N++,S+=4);
  M=SolidColor0;

  if(Sprites16x16)
  {
    for(J=0,S=SprTab;J<N;++J,S+=4)
      if((S[3]&0x0F)||M)
        for(I=J+1,D=S+4;I<N;++I,D+=4)
          if((D[3]&0x0F)||M) 
          {
            DV=S[0]-D[0];
            if((DV<16)||(DV>240))
	    {
              DH=S[1]-D[1];
              if((DH<16)||(DH>240))
	      {
                PS=SprGen+((int)(S[2]&0xFC)<<3);
                PD=SprGen+((int)(D[2]&0xFC)<<3);
                if(DV<16) PD+=DV; else { DV=256-DV;PS+=DV; }
                if(DH>240) { DH=256-DH;T=PS;PS=PD;PD=T; }
                while(DV<16)
                {
                  LS=((word)*PS<<8)+*(PS+16);
                  LD=((word)*PD<<8)+*(PD+16);
                  if(LD&(LS>>DH)) break;
                  else { DV++;PS++;PD++; }
                }
                if(DV<16) { VDPStatus[0]|=0x20;return; }
              }
            }
          }
  }
  else
  {
    for(J=0,S=SprTab;J<N;++J,S+=4)
      if((S[3]&0x0F)||M)
        for(I=J+1,D=S+4;I<N;++I,D+=4)
          if((D[3]&0x0F)||M)
          {
            DV=S[0]-D[0];
            if((DV<8)||(DV>248))
            {
              DH=S[1]-D[1];
              if((DH<8)||(DH>248))
              {
                PS=SprGen+((int)S[2]<<3);
                PD=SprGen+((int)D[2]<<3);
                if(DV<8) PD+=DV; else { DV=256-DV;PS+=DV; }
                if(DH>248) { DH=256-DH;T=PS;PS=PD;PD=T; }
                while((DV<8)&&!(*PD&(*PS>>DH))) { DV++;PS++;PD++; }
                if(DV<8) { VDPStatus[0]|=0x20;return; }
              }
            }
          }
  }
}

/** StateID() ************************************************/
/** Compute 16bit emulation state ID used to identify .STA  **/
/** files.                                                  **/
/*************************************************************/
word StateID(void)
{
  word ID;
  int J,I;

  ID=0x0000;

  /* Add up cartridge ROMs, BIOS, BASIC, ExtBIOS, and DiskBIOS bytes */
  for(I=0;I<MAXSLOTS;++I)
    if(ROMData[I]) for(J=0;J<(ROMMask[I]+1)*0x2000;++J) ID+=I^ROMData[I][J];
  if(MemMap[0][0][0]&&(MemMap[0][0][0]!=EmptyRAM))
    for(J=0;J<0x8000;++J) ID+=MemMap[0][0][0][J];
  if(MemMap[3][1][0]&&(MemMap[3][1][0]!=EmptyRAM))
    for(J=0;J<0x4000;++J) ID+=MemMap[3][1][0][J];
  if(MemMap[3][1][2]&&(MemMap[3][1][2]!=EmptyRAM))
    for(J=0;J<0x4000;++J) ID+=MemMap[3][1][2][J];

  return(ID);
}

/** MakeFileName() *******************************************/
/** Make a copy of the file name, replacing the extension.  **/
/** Returns allocated new name or 0 on failure.             **/
/*************************************************************/
char *MakeFileName(const char *FileName,const char *Extension)
{
  char *Result,*P;

  Result = malloc(strlen(FileName)+strlen(Extension)+1);
  if(!Result) return(0);

  strcpy(Result,FileName);
  if(P=strrchr(Result,'.')) strcpy(P,Extension); else strcat(Result,Extension);
  return(Result);
}

/** ChangeTape() *********************************************/
/** Change tape image. ChangeTape(0) closes current image.  **/
/** Returns 1 on success, 0 on failure.                     **/
/*************************************************************/
byte ChangeTape(const char *FileName)
{
  if(CasStream) fclose(CasStream);
  CasStream = FileName? fopen(FileName,"r+b"):0;
  return(!FileName||CasStream);
}

/** RewindTape() *********************************************/
/** Rewind currenly open tape.                              **/
/*************************************************************/
void RewindTape(void) { if(CasStream) rewind(CasStream); }

/** ChangePrinter() ******************************************/
/** Change printer output to a given file. The previous     **/
/** file is closed. ChangePrinter(0) redirects output to    **/
/** stdout. Returns 1 on success, 0 on failure.             **/
/*************************************************************/
void ChangePrinter(const char *FileName)
{
  if(PrnStream&&(PrnStream!=stdout)) fclose(PrnStream);
  PrnName   = FileName;
  PrnStream = 0;
}

/** ChangeDisk() *********************************************/
/** Change disk image in a given drive. Closes current disk **/
/** image if Name=0 was given. Creates a new disk image if  **/
/** Name="" was given. Returns 1 on success or 0 on failure.**/
/*************************************************************/
byte ChangeDisk(byte N,const char *FileName)
{
  int NeedState;
  byte *P;

  /* We only have MAXDRIVES drives */
  if(N>=MAXDRIVES) return(0);

  /* Load state when inserting first disk into drive A: */
  NeedState = FileName && *FileName && !N && !FDD[N].Data;

  /* Reset FDC, in case it was running a command */
  Reset1793(&FDC,FDD,WD1793_KEEP);

  /* Eject disk if requested */
  if(!FileName) { EjectFDI(&FDD[N]);return(1); }

  /* If FileName not empty, try loading disk image */
  if(*FileName&&LoadFDI(&FDD[N],FileName,FMT_AUTO))
  {
    /* If first disk, also try loading state */
    if(NeedState) FindState(FileName);
    /* Done */
    return(1);
  }

  /*
   * Failed to open as a plain file
   */

  /* Create a new 720kB disk image */
  P = NewFDI(&FDD[N],
      DSK_SIDS_PER_DISK,
      DSK_TRKS_PER_SIDE,
      DSK_SECS_PER_TRCK,
      DSK_SECTOR_SIZE
    );

  /* If FileName not empty, treat it as directory, otherwise new disk */
  if(P&&!(*FileName? DSKLoad(FileName,P):DSKCreate(P)))
  { EjectFDI(&FDD[N]);return(0); }

  /* Done */
  return(!!P);
}

/** LoadFile() ***********************************************/
/** Simple utility function to load cartridge, state, font  **/
/** or a disk image, based on the file extension, etc.      **/
/*************************************************************/
int LoadFile(const char *FileName)
{
  int J;

  /* Try loading as a disk */
  if(hasext(FileName,".DSK")||hasext(FileName,".FDI"))
  {
    /* Change disk image in drive A: */
    if(!ChangeDisk(0,FileName)) return(0);
    /* Eject all user cartridges if successful */
    for(J=0;J<MAXCARTS;++J) LoadCart(0,J,ROMType[J]);
    /* Done */
    return(1);
  }

  /* Try loading as a cartridge */
  if(hasext(FileName,".ROM")||hasext(FileName,".MX1")||hasext(FileName,".MX2"))
    return(!!LoadCart(FileName,0,ROMGUESS(0)|ROMTYPE(0)));

  /* Try loading as a tape */
  if(hasext(FileName,".CAS")) return(!!ChangeTape(FileName));
  /* Try loading as a font */
  if(hasext(FileName,".FNT")) return(!!LoadFNT(FileName));
  /* Try loading as palette */
  if(hasext(FileName,".PAL")) return(!!LoadPAL(FileName));
  /* Try loading as cheats */
  if(hasext(FileName,".CHT")) return(!!LoadCHT(FileName));

  /* Unknown file type */
  return(0);
}

/** SaveCHT() ************************************************/
/** Save cheats to a given text file. Returns the number of **/
/** cheats on success, 0 on failure.                        **/
/*************************************************************/
int SaveCHT(const char *Name)
{
  FILE *F;
  int J;

  /* Open .CHT text file with cheats */
  F = fopen(Name,"wb");
  if(!F) return(0);

  /* Save cheats */
  for(J=0;J<CheatCount;++J)
    fprintf(F,"%s\n",CheatCodes[J].Text);

  /* Done */
  fclose(F);
  return(CheatCount);
}

/** AddCheat() ***********************************************/
/** Add a new cheat. Returns 0 on failure or the number of  **/
/** cheats on success.                                      **/
/*************************************************************/
int AddCheat(const char *Cheat)
{
  static const char *Hex = "0123456789ABCDEF";
  unsigned int A,D;
  char *P;
  int J,N;

  /* Table full: no more cheats */
  if(CheatCount>=MAXCHEATS) return(0);

  /* Check cheat length and decode */
  N=strlen(Cheat);

  if(((N==13)||(N==11))&&(Cheat[8]=='-'))
  {
    for(J=0,A=0;J<8;J++)
    {
      P=strchr(Hex,toupper(Cheat[J]));
      if(!P) return(0); else A=(A<<4)|(P-Hex);
    }
    for(J=9,D=0;J<N;J++)
    {
      P=strchr(Hex,toupper(Cheat[J]));
      if(!P) return(0); else D=(D<<4)|(P-Hex);
    }
  }
  else if(((N==9)||(N==7))&&(Cheat[4]=='-'))
  {
    for(J=0,A=0x0100;J<4;J++)
    {
      P=strchr(Hex,toupper(Cheat[J]));
      if(!P) return(0); else A=(A<<4)|(P-Hex);
    }
    for(J=5,D=0;J<N;J++)
    {
      P=strchr(Hex,toupper(Cheat[J]));
      if(!P) return(0); else D=(D<<4)|(P-Hex);
    }
  }
  else
  {
    /* Cannot parse this cheat */
    return(0);
  }

  /* Add cheat */
  strcpy(CheatCodes[CheatCount].Text,Cheat);
  if(N==13)
  {
    CheatCodes[CheatCount].Addr = A;
    CheatCodes[CheatCount].Data = D&0xFFFF;
    CheatCodes[CheatCount].Size = 2;
  }
  else
  {
    CheatCodes[CheatCount].Addr = A;
    CheatCodes[CheatCount].Data = D&0xFF;
    CheatCodes[CheatCount].Size = 1;
  }

  /* Successfully added a cheat! */
  return(++CheatCount);
}

/** DelCheat() ***********************************************/
/** Delete a cheat. Returns 0 on failure, 1 on success.     **/
/*************************************************************/
int DelCheat(const char *Cheat)
{
  int I,J;

  /* Scan all cheats */
  for(J=0;J<CheatCount;++J)
  {
    /* Match cheat text */
    for(I=0;Cheat[I]&&CheatCodes[J].Text[I];++I)
      if(CheatCodes[J].Text[I]!=toupper(Cheat[I])) break;
    /* If cheat found... */
    if(!Cheat[I]&&!CheatCodes[J].Text[I])
    {
      /* Shift cheats by one */
      if(--CheatCount!=J)
        memcpy(&CheatCodes[J],&CheatCodes[J+1],(CheatCount-J)*sizeof(CheatCodes[0]));
      /* Cheat deleted */
      return(1);
    }
  }

  /* Cheat not found */
  return(0);
}

/** ResetCheats() ********************************************/
/** Remove all cheats.                                      **/
/*************************************************************/
void ResetCheats(void) { Cheats(CHTS_OFF);CheatCount=0; }

/** ApplyCheats() ********************************************/
/** Apply RAM-based cheats. Returns the number of applied   **/
/** cheats.                                                 **/
/*************************************************************/
int ApplyCheats(void)
{
  int J,I;

  /* For all current cheats that look like 01AAAAAA-DD/DDDD... */
  for(J=I=0;J<CheatCount;++J)
    if((CheatCodes[J].Addr>>24)==0x01)
    {
      WrZ80(CheatCodes[J].Addr&0xFFFF,CheatCodes[J].Data&0xFF);
      if(CheatCodes[J].Size>1)
        WrZ80((CheatCodes[J].Addr+1)&0xFFFF,CheatCodes[J].Data>>8);
      ++I;
    }

  /* Return number of applied cheats */
  return(I);
}

/** Cheats() *************************************************/
/** Toggle cheats on (1), off (0), inverse state (2) or     **/
/** query (3).                                              **/
/*************************************************************/
int Cheats(int Switch)
{
  byte *P,*Base;
  int J,Size;

  switch(Switch)
  {
    case CHTS_ON:
    case CHTS_OFF:    if(Switch==CheatsON) return(CheatsON);
    case CHTS_TOGGLE: Switch=!CheatsON;break;
    default:          return(CheatsON);
  }

  /* Find valid cartridge */
  for(J=1;(J<=2)&&!ROMData[J];++J);

  /* Must have ROM */
  if(J>2) return(Switch=CHTS_OFF);

  /* Compute ROM address and size */
  Base = ROMData[J];
  Size = ((int)ROMMask[J]+1)<<14;

  /* If toggling cheats... */
  if(Switch!=CheatsON)
  {
    /* If enabling cheats... */
    if(Switch)
    {
      /* Patch ROM with the cheat values */
      for(J=0;J<CheatCount;++J)
        if(!(CheatCodes[J].Addr>>24)&&(CheatCodes[J].Addr+CheatCodes[J].Size<=Size))
        {
          P = Base + CheatCodes[J].Addr;
          CheatCodes[J].Orig = P[0];
          P[0] = CheatCodes[J].Data;
          if(CheatCodes[J].Size>1)
          {
            CheatCodes[J].Orig |= (int)P[1]<<8;
            P[1] = CheatCodes[J].Data>>8;
          }
        }
    }
    else
    {
      /* Restore original ROM values */
      for(J=0;J<CheatCount;++J)
        if(!(CheatCodes[J].Addr>>24)&&(CheatCodes[J].Addr+CheatCodes[J].Size<=Size))
        {
          P = Base + CheatCodes[J].Addr;
          P[0] = CheatCodes[J].Orig;
          if(CheatCodes[J].Size>1)
            P[1] = CheatCodes[J].Orig>>8;
        }
    }

    /* Done toggling cheats */
    CheatsON = Switch;
  }

  /* Done */
  if(Verbose) printf("Cheats %s\n",CheatsON? "ON":"OFF");
  return(CheatsON);
}

#if defined(ANDROID)
#undef  feof
#define fopen           mopen
#define fclose          mclose
#define fread           mread
#define fwrite          mwrite
#define fgets           mgets
#define fseek           mseek
#define rewind          mrewind
#define fgetc           mgetc
#define ftell           mtell
#define feof            meof
#elif defined(ZLIB)
#undef  feof
#define fopen(N,M)      (FILE *)gzopen(N,M)
#define fclose(F)       gzclose((gzFile)(F))
#define fread(B,L,N,F)  gzread((gzFile)(F),B,(L)*(N))
#define fwrite(B,L,N,F) gzwrite((gzFile)(F),B,(L)*(N))
#define fgets(B,L,F)    gzgets((gzFile)(F),B,L)
#define fseek(F,O,W)    gzseek((gzFile)(F),O,W)
#define rewind(F)       gzrewind((gzFile)(F))
#define fgetc(F)        gzgetc((gzFile)(F))
#define ftell(F)        gztell((gzFile)(F))
#define feof(F)         gzeof((gzFile)(F))
#endif

/** GuessROM() ***********************************************/
/** Guess MegaROM mapper of a ROM.                          **/
/*************************************************************/
int GuessROM(const byte *Buf,int Size)
{
  int J,I,K,ROMCount[MAXMAPPERS];
  char S[256];
  FILE *F;

  /* Try opening file with CRCs */
  if(F=fopen("CARTS.CRC","rb"))
  {
    /* Compute ROM's CRC */
    for(J=K=0;J<Size;++J) K+=Buf[J];

    /* Scan file comparing CRCs */
    while(fgets(S,sizeof(S)-4,F))
      if(sscanf(S,"%08X %d",&J,&I)==2)
        if(K==J) { fclose(F);return(I); }

    /* Nothing found */
    fclose(F);
  }

  /* Try opening file with SHA1 sums */
  if(F=fopen("CARTS.SHA","rb"))
  {
    char S1[41],S2[41];
    SHA1 C;

    /* Compute ROM's SHA1 */
    ResetSHA1(&C);
    InputSHA1(&C,Buf,Size);
    if(ComputeSHA1(&C))
    {
      sprintf(S1,"%08x%08x%08x%08x%08x",C.Msg[0],C.Msg[1],C.Msg[2],C.Msg[3],C.Msg[4]);

      /* Search for computed SHA1 in the file */
      while(fgets(S,sizeof(S)-4,F))
        if((sscanf(S,"%40s %d",S2,&J)==2) && !strcmp(S1,S2))
        { fclose(F);return(J); }
    }

    /* Nothing found */
    fclose(F);
  }

  /* Clear all counters */
  for(J=0;J<MAXMAPPERS;++J) ROMCount[J]=1;
  /* Generic 8kB mapper is default */
  ROMCount[MAP_GEN8]+=1;
  /* ASCII 16kB preferred over ASCII 8kB */
  ROMCount[MAP_ASCII16]-=1;

  /* Count occurences of characteristic addresses */
  for(J=0;J<Size-2;++J)
  {
    I=Buf[J]+((int)Buf[J+1]<<8)+((int)Buf[J+2]<<16);
    switch(I)
    {
      case 0x500032: ROMCount[MAP_KONAMI5]++;break;
      case 0x900032: ROMCount[MAP_KONAMI5]++;break;
      case 0xB00032: ROMCount[MAP_KONAMI5]++;break;
      case 0x400032: ROMCount[MAP_KONAMI4]++;break;
      case 0x800032: ROMCount[MAP_KONAMI4]++;break;
      case 0xA00032: ROMCount[MAP_KONAMI4]++;break;
      case 0x680032: ROMCount[MAP_ASCII8]++;break;
      case 0x780032: ROMCount[MAP_ASCII8]++;break;
      case 0x600032: ROMCount[MAP_KONAMI4]++;
                     ROMCount[MAP_ASCII8]++;
                     ROMCount[MAP_ASCII16]++;
                     break;
      case 0x700032: ROMCount[MAP_KONAMI5]++;
                     ROMCount[MAP_ASCII8]++;
                     ROMCount[MAP_ASCII16]++;
                     break;
      case 0x77FF32: ROMCount[MAP_ASCII16]++;break;
    }
  }

  /* Find which mapper type got more hits */
  for(I=0,J=0;J<MAXMAPPERS;++J)
    if(ROMCount[J]>ROMCount[I]) I=J;

  /* Return the most likely mapper type */
  return(I);
}

/** LoadFNT() ************************************************/
/** Load fixed 8x8 font used in text screen modes when      **/
/** MSX_FIXEDFONT option is enabled. LoadFNT(0) frees the   **/
/** font buffer. Returns 1 on success, 0 on failure.        **/
/*************************************************************/
byte LoadFNT(const char *FileName)
{
  FILE *F;

  /* Drop out if no new font requested */
  if(!FileName) { FreeMemory(FontBuf);FontBuf=0;return(1); }
  /* Try opening font file */
  if(!(F=fopen(FileName,"rb"))) return(0);
  /* Allocate memory for 256 8x8 characters, if needed */
  if(!FontBuf) FontBuf=GetMemory(256*8);
  /* Drop out if failed memory allocation */
  if(!FontBuf) { fclose(F);return(0); }
  /* Read font, ignore short reads */
  fread(FontBuf,1,256*8,F);
  /* Done */
  fclose(F);
  return(1);  
}

/** LoadROM() ************************************************/
/** Load a file, allocating memory as needed. Returns addr. **/
/** of the alocated space or 0 if failed.                   **/
/*************************************************************/
byte *LoadROM(const char *Name,int Size,byte *Buf)
{
  FILE *F;
  byte *P;
  int J;

  /* Can't give address without size! */
  if(Buf&&!Size) return(0);

  /* Open file */
  if(!(F=fopen(Name,"rb"))) return(0);

  /* Determine data size, if wasn't given */
  if(!Size)
  {
    /* Determine size via ftell() or by reading entire [GZIPped] stream */
    if(!fseek(F,0,SEEK_END)) Size=ftell(F);
    else
    {
      /* Read file in 16kB increments */
      while((J=fread(EmptyRAM,1,0x4000,F))==0x4000) Size+=J;
      if(J>0) Size+=J;
      /* Clean up the EmptyRAM! */
      memset(EmptyRAM,NORAM,0x4000);
    }
    /* Rewind file to the beginning */
    rewind(F);
  }

  /* Allocate memory */
  P=Buf? Buf:GetMemory(Size);
  if(!P)
  {
    fclose(F);
    return(0);
  }

  /* Read data */
  if((J=fread(P,1,Size,F))!=Size)
  {
    if(!Buf) FreeMemory(P);
    fclose(F);
    return(0);
  }

  /* Done */
  fclose(F);
  return(P);
}

/** FindState() **********************************************/
/** Compute state file name corresponding to given filename **/
/** and try loading state. Returns 1 on success, 0 on       **/
/** failure.                                                **/
/*************************************************************/
int FindState(const char *Name)
{
  int J,I;
  char *P;

  /* No result yet */
  J = 0;

  /* Remove old state name */
  FreeMemory((char *)STAName);

  /* If STAName gets created... */
  if(STAName=MakeFileName(Name,".sta"))
  {
    /* Try loading state */
    if(Verbose) printf("Loading state from %s...",STAName);
    J=LoadSTA(STAName);
    PRINTRESULT(J);
  }

  /* Generate cheat file name and try loading it */
  if(P=MakeFileName(Name,".cht"))
  {
    I=LoadCHT(P);
    if(I&&Verbose) printf("Loaded %d cheats from %s\n",I,P);
    FreeMemory(P);
  }

  /* Generate palette file name and try loading it */
  if(P=MakeFileName(Name,".pal"))
  {
    I=LoadPAL(P);
    if(I&&Verbose) printf("Loaded palette from %s\n",P);
    FreeMemory(P);
  }

  /* Done */
  return(J);
}

/** LoadCart() ***********************************************/
/** Load cartridge into given slot. Returns cartridge size  **/
/** in 16kB pages on success, 0 on failure.                 **/
/*************************************************************/
int LoadCart(const char *FileName,int Slot,int Type)
{
  int C1,C2,Len,Pages,ROM64;
  byte *P,PS,SS;
  FILE *F;

  /* Slot number must be valid */
  if((Slot<0)||(Slot>=MAXSLOTS)) return(0);
  /* Find primary/secondary slots */
  for(PS=0;PS<4;++PS)
  {
    for(SS=0;(SS<4)&&(CartMap[PS][SS]!=Slot);++SS);
    if(SS<4) break;
  }
  /* Drop out if slots not found */
  if(PS>=4) return(0);

  /* If there is a SRAM in this cartridge slot... */
  if(SRAMData[Slot]&&SaveSRAM[Slot]&&SRAMName[Slot])
  {
    /* Open .SAV file */
    if(Verbose) printf("Writing %s...",SRAMName[Slot]);
    if(!(F=fopen(SRAMName[Slot],"wb"))) SaveSRAM[Slot]=0;
    else
    {
      /* Write .SAV file */
      switch(ROMType[Slot])
      {
        case MAP_ASCII8:
        case MAP_FMPAC:
          if(fwrite(SRAMData[Slot],1,0x2000,F)!=0x2000) SaveSRAM[Slot]=0;
          break;
        case MAP_ASCII16:
          if(fwrite(SRAMData[Slot],1,0x0800,F)!=0x0800) SaveSRAM[Slot]=0;
          break;
        case MAP_GMASTER2:
          if(fwrite(SRAMData[Slot],1,0x1000,F)!=0x1000)        SaveSRAM[Slot]=0;
          if(fwrite(SRAMData[Slot]+0x2000,1,0x1000,F)!=0x1000) SaveSRAM[Slot]=0;
          break;
      }

      /* Done with .SAV file */
      fclose(F);
    }

    /* Done saving SRAM */
    PRINTRESULT(SaveSRAM[Slot]);
  }

  /* If ejecting cartridge... */
  if(!FileName)
  {
    if(ROMData[Slot])
    {
      /* Free memory if present */
      FreeMemory(ROMData[Slot]);
      ROMData[Slot] = 0;
      ROMMask[Slot] = 0;
      /* Set memory map to dummy RAM */
      for(C1=0;C1<8;++C1) MemMap[PS][SS][C1]=EmptyRAM;
      /* Restart MSX */
      ResetMSX(Mode,RAMPages,VRAMPages);
      /* Cartridge ejected */
      if(Verbose) printf("Ejected cartridge from slot %c\n",Slot+'A');
    }

    /* Nothing else to do */
    return(0);
  }

  /* Try opening file */
  if(!(F=fopen(FileName,"rb"))) return(0);
  if(Verbose) printf("Found %s:\n",FileName);

  /* Determine size via ftell() or by reading entire [GZIPped] stream */
  if(!fseek(F,0,SEEK_END)) Len=ftell(F);
  else
  {
    /* Read file in 16kB increments */
    for(Len=0;(C2=fread(EmptyRAM,1,0x4000,F))==0x4000;Len+=C2);
    if(C2>0) Len+=C2;
    /* Clean up the EmptyRAM! */
    memset(EmptyRAM,NORAM,0x4000);
  }

  /* Rewind file */
  rewind(F);

  /* Compute size in 8kB pages */
  Len>>=13;
  /* Calculate 2^n closest to number of pages */
  for(Pages=1;Pages<Len;Pages<<=1);

  /* Check "AB" signature in a file */
  ROM64=0;
  C1=fgetc(F);
  C2=fgetc(F);

  /* Maybe this is a flat 64kB ROM? */
  if((C1!='A')||(C2!='B'))
    if(fseek(F,0x4000,SEEK_SET)>=0)
    {
      C1=fgetc(F);
      C2=fgetc(F);
      ROM64=(C1=='A')&&(C2=='B');
    }

  /* Maybe it is the last page that contains "AB" signature? */
  if((Len>=2)&&((C1!='A')||(C2!='B')))
    if(fseek(F,0x2000*(Len-2),SEEK_SET)>=0)
    {
      C1=fgetc(F);
      C2=fgetc(F);
    }

  /* If we can't find "AB" signature, drop out */     
  if((C1!='A')||(C2!='B'))
  {
    if(Verbose) puts("  Not a valid cartridge ROM");
    fclose(F);
    return(0);
  }

  if(Verbose) printf("  Cartridge %c: ",'A'+Slot);

  /* Done with the file */
  fclose(F);

  /* Show ROM type and size */
  if(Verbose)
    printf
    (
      "%dkB %s ROM..",Len*8,
      ROM64||(Len<=4)? "NORMAL":Type>=MAP_GUESS? "UNKNOWN":ROMNames[Type]
    );

  /* Assign ROMMask for MegaROMs */
  ROMMask[Slot]=!ROM64&&(Len>4)? (Pages-1):0x00;
  /* Allocate space for the ROM */
  ROMData[Slot]=GetMemory(Pages<<13);
  if(!ROMData[Slot]) { PRINTFAILED;return(0); }

  /* Try loading ROM */
  if(!LoadROM(FileName,Len<<13,ROMData[Slot])) { PRINTFAILED;return(0); }

  /* Mirror ROM if it is smaller than 2^n pages */
  if(Len<Pages)
    memcpy
    (
      ROMData[Slot]+Len*0x2000,
      ROMData[Slot]+(Len-Pages/2)*0x2000,
      (Pages-Len)*0x2000
    ); 

  /* Set memory map depending on the ROM size */
  switch(Len)
  {
    case 1:
      /* 8kB ROMs are mirrored 8 times: 0:0:0:0:0:0:0:0 */
      MemMap[PS][SS][0]=ROMData[Slot];
      MemMap[PS][SS][1]=ROMData[Slot];
      MemMap[PS][SS][2]=ROMData[Slot];
      MemMap[PS][SS][3]=ROMData[Slot];
      MemMap[PS][SS][4]=ROMData[Slot];
      MemMap[PS][SS][5]=ROMData[Slot];
      MemMap[PS][SS][6]=ROMData[Slot];
      MemMap[PS][SS][7]=ROMData[Slot];
      break;

    case 2:
      /* 16kB ROMs are mirrored 4 times: 0:1:0:1:0:1:0:1 */
      MemMap[PS][SS][0]=ROMData[Slot];
      MemMap[PS][SS][1]=ROMData[Slot]+0x2000;
      MemMap[PS][SS][2]=ROMData[Slot];
      MemMap[PS][SS][3]=ROMData[Slot]+0x2000;
      MemMap[PS][SS][4]=ROMData[Slot];
      MemMap[PS][SS][5]=ROMData[Slot]+0x2000;
      MemMap[PS][SS][6]=ROMData[Slot];
      MemMap[PS][SS][7]=ROMData[Slot]+0x2000;
      break;

    case 3:
    case 4:
      /* 24kB and 32kB ROMs are mirrored twice: 0:1:0:1:2:3:2:3 */
      MemMap[PS][SS][0]=ROMData[Slot];
      MemMap[PS][SS][1]=ROMData[Slot]+0x2000;
      MemMap[PS][SS][2]=ROMData[Slot];
      MemMap[PS][SS][3]=ROMData[Slot]+0x2000;
      MemMap[PS][SS][4]=ROMData[Slot]+0x4000;
      MemMap[PS][SS][5]=ROMData[Slot]+0x6000;
      MemMap[PS][SS][6]=ROMData[Slot]+0x4000;
      MemMap[PS][SS][7]=ROMData[Slot]+0x6000;
      break;

    default:
      if(ROM64)
      {
        /* 64kB ROMs are loaded to fill slot: 0:1:2:3:4:5:6:7 */
        MemMap[PS][SS][0]=ROMData[Slot];
        MemMap[PS][SS][1]=ROMData[Slot]+0x2000;
        MemMap[PS][SS][2]=ROMData[Slot]+0x4000;
        MemMap[PS][SS][3]=ROMData[Slot]+0x6000;
        MemMap[PS][SS][4]=ROMData[Slot]+0x8000;
        MemMap[PS][SS][5]=ROMData[Slot]+0xA000;
        MemMap[PS][SS][6]=ROMData[Slot]+0xC000;
        MemMap[PS][SS][7]=ROMData[Slot]+0xE000;
      }
      break;
  }

  /* Show starting address */
  if(Verbose)
    printf
    (
      "starts at %04Xh..",
      MemMap[PS][SS][2][2]+256*MemMap[PS][SS][2][3]
    );

  /* Guess MegaROM mapper type if not given */
  if((Type>=MAP_GUESS)&&(ROMMask[Slot]+1>4))
  {
    Type=GuessROM(ROMData[Slot],0x2000*(ROMMask[Slot]+1));
    if(Verbose) printf("guessed %s..",ROMNames[Type]);
    if(Slot<MAXCARTS) SETROMTYPE(Slot,Type);
  }

  /* Save MegaROM type */
  ROMType[Slot]=Type;

  /* For Generic/16kB carts, set ROM pages as 0:1:N-2:N-1 */
  if((Type==MAP_GEN16)&&(ROMMask[Slot]+1>4))
    SetMegaROM(Slot,0,1,ROMMask[Slot]-1,ROMMask[Slot]);

  /* If cartridge may need a SRAM... */
  if(MAP_SRAM(Type))
  {
    /* Free previous SRAM resources */
    FreeMemory(SRAMData[Slot]);
    FreeMemory(SRAMName[Slot]);

    /* Get SRAM memory */
    SRAMData[Slot]=GetMemory(0x4000);
    if(!SRAMData[Slot])
    {
      if(Verbose) printf("scratch SRAM..");
      SRAMData[Slot]=EmptyRAM;
    }
    else
    {
      if(Verbose) printf("got 16kB SRAM..");
      memset(SRAMData[Slot],NORAM,0x4000);
    }

    /* Generate SRAM file name and load SRAM contents */
    if(SRAMName[Slot]=GetMemory(strlen(FileName)+5))
    {
      /* Compose SRAM file name */
      strcpy(SRAMName[Slot],FileName);      
      P=strrchr(SRAMName[Slot],'.');
      if(P) strcpy(P,".sav"); else strcat(SRAMName[Slot],".sav");
      /* Try opening file... */
      if(F=fopen(SRAMName[Slot],"rb"))
      {
        /* Read SRAM file */
        Len=fread(SRAMData[Slot],1,0x4000,F);
        fclose(F);
        /* Print information if needed */
        if(Verbose) printf("loaded %d bytes from %s..",Len,SRAMName[Slot]);
        /* Mirror data according to the mapper type */
        P=SRAMData[Slot];
        switch(Type)
        {
          case MAP_FMPAC:
            memset(P+0x2000,NORAM,0x2000);
            P[0x1FFE]=FMPAC_MAGIC&0xFF;
            P[0x1FFF]=FMPAC_MAGIC>>8;
            break;
          case MAP_GMASTER2:
            memcpy(P+0x2000,P+0x1000,0x1000);
            memcpy(P+0x3000,P+0x1000,0x1000);
            memcpy(P+0x1000,P,0x1000);
            break;
          case MAP_ASCII16:
            memcpy(P+0x0800,P,0x0800);
            memcpy(P+0x1000,P,0x0800);
            memcpy(P+0x1800,P,0x0800);
            memcpy(P+0x2000,P,0x0800);
            memcpy(P+0x2800,P,0x0800);
            memcpy(P+0x3000,P,0x0800);
            memcpy(P+0x3800,P,0x0800);
            break;
        }
      }
    } 
  }

  /* Done setting up cartridge */
  ResetMSX(Mode,RAMPages,VRAMPages);
  PRINTOK;

  /* If first used user slot, try loading state */
  if(!Slot||((Slot==1)&&!ROMData[0])) FindState(FileName);

  /* Done loading cartridge */
  return(Pages);
}

/** LoadCHT() ************************************************/
/** Load cheats from .CHT file. Cheat format is either      **/
/** 00XXXXXX-XX (one byte) or 00XXXXXX-XXXX (two bytes) for **/
/** ROM-based cheats and XXXX-XX or XXXX-XXXX for RAM-based **/
/** cheats. Returns the number of cheats on success, 0 on   **/
/** failure.                                                **/
/*************************************************************/
int LoadCHT(const char *Name)
{
  char Buf[256],S[16];
  int Status;
  FILE *F;

  /* Open .CHT text file with cheats */
  F = fopen(Name,"rb");
  if(!F) return(0);

  /* Switch cheats off for now and remove all present cheats */
  Status = Cheats(CHTS_QUERY);
  Cheats(CHTS_OFF);
  ResetCheats();

  /* Try adding cheats loaded from file */
  while(!feof(F))
    if(fgets(Buf,sizeof(Buf),F) && (sscanf(Buf,"%13s",S)==1))
      AddCheat(S);

  /* Done with the file */
  fclose(F);

  /* Turn cheats back on, if they were on */
  Cheats(Status);

  /* Done */
  return(CheatCount);
}

/** LoadPAL() ************************************************/
/** Load new palette from .PAL file. Returns number of      **/
/** loaded colors on success, 0 on failure.                 **/
/*************************************************************/
int LoadPAL(const char *Name)
{
  static const char *Hex = "0123456789ABCDEF";
  char S[256],*P,*T,*H;
  FILE *F;
  int J,I;

  if(!(F=fopen(Name,"rb"))) return(0);

  for(J=0;(J<16)&&fgets(S,sizeof(S),F);++J)
  {
    /* Skip white space and optional '#' character */
    for(P=S;*P&&(*P<=' ');++P);
    if(*P=='#') ++P;
    /* Parse six hexadecimal digits */
    for(T=P,I=0;*T&&(H=strchr(Hex,toupper(*T)));++T) I=(I<<4)+(H-Hex);
    /* If we have got six digits, parse and set color */
    if(T-P==6) SetColor(J,I>>16,(I>>8)&0xFF,I&0xFF);
  }

  fclose(F);
  return(J);
}


#ifdef NEW_STATES
#include "State.h"
#else

/** SaveSTA() ************************************************/
/** Save emulation state to a .STA file.                    **/
/*************************************************************/
int SaveSTA(const char *FileName)
{
  static byte Header[16] = "STE\032\003\0\0\0\0\0\0\0\0\0\0\0";
  unsigned int State[256],J,I,K;
  FILE *F;

  /* Open state file */
  if(!(F=fopen(FileName,"wb"))) return(0);

  /* Prepare the header */
  J=StateID();
  Header[5] = RAMPages;
  Header[6] = VRAMPages;
  Header[7] = J&0x00FF;
  Header[8] = J>>8;

  /* Write out the header */
  if(fwrite(Header,1,sizeof(Header),F)!=sizeof(Header))
  { fclose(F);return(0); }

  /* Fill out hardware state */
  J=0;
  State[J++] = VDPData;
  State[J++] = PLatch;
  State[J++] = ALatch;
  State[J++] = VAddr;
  State[J++] = VKey;
  State[J++] = PKey;
  State[J++] = WKey;
  State[J++] = IRQPending;
  State[J++] = ScanLine;
  State[J++] = RTCReg;
  State[J++] = RTCMode;
  State[J++] = KanLetter;
  State[J++] = KanCount;
  State[J++] = IOReg;
  State[J++] = PSLReg;
  State[J++] = FMPACKey;

  /* Memory setup */
  for(I=0;I<4;++I)
  {
    State[J++] = SSLReg[I];
    State[J++] = PSL[I];
    State[J++] = SSL[I];
    State[J++] = EnWrite[I];
    State[J++] = RAMMapper[I];
  }  

  /* Cartridge setup */
  for(I=0;I<MAXSLOTS;++I)
  {
    State[J++] = ROMType[I];
    for(K=0;K<4;++K) State[J++]=ROMMapper[I][K];
  }

  /* Write out hardware state */
  if(fwrite(&CPU,1,sizeof(CPU),F)!=sizeof(CPU))
  { fclose(F);return(0); }
  if(fwrite(&PPI,1,sizeof(PPI),F)!=sizeof(PPI))
  { fclose(F);return(0); }
  if(fwrite(VDP,1,sizeof(VDP),F)!=sizeof(VDP))
  { fclose(F);return(0); }
  if(fwrite(VDPStatus,1,sizeof(VDPStatus),F)!=sizeof(VDPStatus))
  { fclose(F);return(0); }
  if(fwrite(Palette,1,sizeof(Palette),F)!=sizeof(Palette))
  { fclose(F);return(0); }
  if(fwrite(&PSG,1,sizeof(PSG),F)!=sizeof(PSG))
  { fclose(F);return(0); }
  if(fwrite(&OPLL,1,sizeof(OPLL),F)!=sizeof(OPLL))
  { fclose(F);return(0); }
  if(fwrite(&SCChip,1,sizeof(SCChip),F)!=sizeof(SCChip))
  { fclose(F);return(0); }
  if(fwrite(State,1,sizeof(State),F)!=sizeof(State))
  { fclose(F);return(0); }

  /* Save memory contents */
  if(fwrite(RAMData,1,RAMPages*0x4000,F)!=RAMPages*0x4000)
  { fclose(F);return(0); }
  if(fwrite(VRAM,1,VRAMPages*0x4000,F)!=VRAMPages*0x4000)
  { fclose(F);return(0); }

  /* Done */
  fclose(F);
  return(1);
}

/** LoadSTA() ************************************************/
/** Load emulation state from a .STA file.                  **/
/*************************************************************/
int LoadSTA(const char *FileName)
{
  unsigned int State[256],J,I,K;
  byte Header[16];
  FILE *F;

  /* Open state file */
  if(!(F=fopen(FileName,"rb"))) return(0);

  /* Read the header */
  if(fread(Header,1,sizeof(Header),F)!=sizeof(Header))
  { fclose(F);return(0); }

  /* Verify the header */
  if(memcmp(Header,"STE\032\003",5))
  { fclose(F);return(0); }
  if(Header[7]+Header[8]*256!=StateID())
  { fclose(F);return(0); }
  if((Header[5]!=(RAMPages&0xFF))||(Header[6]!=(VRAMPages&0xFF)))
  { fclose(F);return(0); }

  /* Read the hardware state */
  if(fread(&CPU,1,sizeof(CPU),F)!=sizeof(CPU))
  { fclose(F);return(0); }
  if(fread(&PPI,1,sizeof(PPI),F)!=sizeof(PPI))
  { fclose(F);return(0); }
  if(fread(VDP,1,sizeof(VDP),F)!=sizeof(VDP))
  { fclose(F);return(0); }
  if(fread(VDPStatus,1,sizeof(VDPStatus),F)!=sizeof(VDPStatus))
  { fclose(F);return(0); }
  if(fread(Palette,1,sizeof(Palette),F)!=sizeof(Palette))
  { fclose(F);return(0); }
  if(fread(&PSG,1,sizeof(PSG),F)!=sizeof(PSG))
  { fclose(F);return(0); }
  if(fread(&OPLL,1,sizeof(OPLL),F)!=sizeof(OPLL))
  { fclose(F);return(0); }
  if(fread(&SCChip,1,sizeof(SCChip),F)!=sizeof(SCChip))
  { fclose(F);return(0); }
  if(fread(State,1,sizeof(State),F)!=sizeof(State))
  { fclose(F);return(0); }

  /* Load memory contents */
  if(fread(RAMData,1,Header[5]*0x4000,F)!=Header[5]*0x4000)
  { fclose(F);return(0); }
  if(fread(VRAM,1,Header[6]*0x4000,F)!=Header[6]*0x4000)
  { fclose(F);return(0); }

  /* Done with the file */
  fclose(F);

  /* Parse hardware state */
  J=0;
  VDPData    = State[J++];
  PLatch     = State[J++];
  ALatch     = State[J++];
  VAddr      = State[J++];
  VKey       = State[J++];
  PKey       = State[J++];
  WKey       = State[J++];
  IRQPending = State[J++];
  ScanLine   = State[J++];
  RTCReg     = State[J++];
  RTCMode    = State[J++];
  KanLetter  = State[J++];
  KanCount   = State[J++];
  IOReg      = State[J++];
  PSLReg     = State[J++];
  FMPACKey   = State[J++];

  /* Memory setup */
  for(I=0;I<4;++I)
  {
    SSLReg[I]       = State[J++];
    PSL[I]          = State[J++];
    SSL[I]          = State[J++];
    EnWrite[I]      = State[J++];
    RAMMapper[I]    = State[J++];
  }  

  /* Cartridge setup */
  for(I=0;I<MAXSLOTS;++I)
  {
    ROMType[I]      = State[J++];
    for(K=0;K<4;++K) ROMMapper[I][K]=State[J++];
  }

  /* Set RAM mapper pages */
  if(RAMMask)
    for(I=0;I<4;++I)
    {
      RAMMapper[I]       &= RAMMask;
      MemMap[3][2][I*2]   = RAMData+RAMMapper[I]*0x4000;
      MemMap[3][2][I*2+1] = MemMap[3][2][I*2]+0x2000;
    }

  /* Set ROM mapper pages */
  for(I=0;I<MAXSLOTS;++I)
    if(ROMData[I]&&ROMMask[I])
      SetMegaROM(I,ROMMapper[I][0],ROMMapper[I][1],ROMMapper[I][2],ROMMapper[I][3]);

  /* Set main address space pages */
  for(I=0;I<4;++I)
  {
    RAM[2*I]   = MemMap[PSL[I]][SSL[I]][2*I];
    RAM[2*I+1] = MemMap[PSL[I]][SSL[I]][2*I+1];
  }

  /* Set palette */
  for(I=0;I<16;++I)
    SetColor(I,(Palette[I]>>16)&0xFF,(Palette[I]>>8)&0xFF,Palette[I]&0xFF);

  /* Set screen mode and VRAM table addresses */
  SetScreen();

  /* Set some other variables */
  VPAGE    = VRAM+((int)VDP[14]<<14);
  FGColor  = VDP[7]>>4;
  BGColor  = VDP[7]&0x0F;
  XFGColor = FGColor;
  XBGColor = BGColor;

  /* All sound channels could have been changed */
  PSG.Changed     = (1<<AY8910_CHANNELS)-1;
  SCChip.Changed  = (1<<SCC_CHANNELS)-1;
  SCChip.WChanged = (1<<SCC_CHANNELS)-1;
  OPLL.Changed    = (1<<YM2413_CHANNELS)-1;
  OPLL.PChanged   = (1<<YM2413_CHANNELS)-1;
  OPLL.DChanged   = (1<<YM2413_CHANNELS)-1;

  /* Done */
  return(1);
}

#endif /* !NEW_STATES */

#if defined(ZLIB) || defined(ANDROID)
#undef fopen
#undef fclose
#undef fread
#undef fwrite
#undef fgets
#undef fseek
#undef ftell
#undef fgetc
#undef feof
#endif
