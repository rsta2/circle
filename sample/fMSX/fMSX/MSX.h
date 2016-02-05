/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                           MSX.h                         **/
/**                                                         **/
/** This file contains declarations relevant to the drivers **/
/** and MSX emulation itself. See Z80.h for #defines        **/
/** related to Z80 emulation.                               **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef MSX_H
#define MSX_H

#include "Z80.h"            /* Z80 CPU emulation             */
#include "V9938.h"          /* V9938 VDP opcode emulation    */
#include "AY8910.h"         /* AY8910 PSG emulation          */
#include "YM2413.h"         /* YM2413 OPLL emulation         */
#include "SCC.h"            /* Konami SCC chip emulation     */
#include "I8255.h"          /* Intel 8255 PPI emulation      */
#include "I8251.h"          /* Intel 8251 UART emulation     */
#include "WD1793.h"         /* WD1793 FDC emulation          */

#include <stdio.h>

/** INLINE ***************************************************/
/** C99 standard has "inline", but older compilers've used  **/
/** __inline for the same purpose.                          **/
/*************************************************************/
#ifdef __C99__
#define INLINE static inline
#else
#define INLINE static __inline
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CPU_CLOCK    3579545       /* CPU clock frequency Hz */
#define VDP_CLOCK    (CPU_CLOCK*6) /* VDP clock frequency Hz */
#define PSG_CLOCK    (CPU_CLOCK/2) /* PSG clock frequency Hz */

#define HPERIOD      1368        /* HPeriod, VDP cycles      */
#define VPERIOD_PAL  (HPERIOD*313) /* PAL VPeriod, VDP ccls  */
#define VPERIOD_NTSC (HPERIOD*262) /* NTSC VPeriod, VDP ccls */ 
#define HREFRESH_240 960         /* 240dot scanline refresh  */
#define HREFRESH_256 1024        /* 256dot scanline refresh  */

#define CPU_VPERIOD  (VPERIOD_NTSC/6)
#define CPU_V262     (VPERIOD_NTSC/6)
#define CPU_V313     (VPERIOD_PAL/6)
#define CPU_HPERIOD  (HPERIOD/6)
#define CPU_H240     (HREFRESH_240/6)
#define CPU_H256     (HREFRESH_256/6)

#define MAX_STASIZE  0x48000         /* Max. state data size */   

#define INT_IE0      0x01   /* VDP interrupt modes           */
#define INT_IE1      0x02
#define INT_IE2      0x04

#define JST_UP       0x01   /* Joystick() bits (shift by 8   */
#define JST_DOWN     0x02   /* for the second joystick)      */
#define JST_LEFT     0x04
#define JST_RIGHT    0x08
#define JST_FIREA    0x10
#define JST_FIREB    0x20

                            /* Joystick/Mouse types:         */
#define JOY_NONE     0      /* No joystick                   */ 
#define JOY_STICK    1      /* Joystick                      */ 
#define JOY_MOUSTICK 2      /* Mouse acting as joystick      */ 
#define JOY_MOUSE    3      /* Mouse                         */ 

                            /* ROM mapper types:             */
#define MAP_GEN8     0      /* Generic switch, 8kB pages     */
#define MAP_GEN16    1      /* Generic switch, 16kB pages    */
#define MAP_KONAMI5  2      /* Konami 5000/7000/9000/B000h   */
#define MAP_KONAMI4  3      /* Konami 4000/6000/8000/A000h   */
#define MAP_ASCII8   4      /* ASCII 6000/6800/7000/7800h    */
#define MAP_ASCII16  5      /* ASCII 6000/7000h              */
#define MAP_GMASTER2 6      /* Konami GameMaster2 cartridge  */
#define MAP_FMPAC    7      /* Panasonic FMPAC cartridge     */
#define MAP_GUESS    8      /* Guess mapper automatically    */

#define MAP_SRAM(N) \
  (((N)==MAP_ASCII8)||((N)==MAP_ASCII16)|| \
   ((N)==MAP_GMASTER2)||((N)==MAP_FMPAC))

#define FMPAC_MAGIC 0x694D  /* FMPAC SRAM "magic value"      */

#define PAGESIZE    0x4000L /* Size of a RAM page            */
#define NORAM       0xFF    /* Byte to be returned from      */
                            /* non-existing pages and ports  */
#define MAXSCREEN   12      /* Highest screen mode supported */
#define MAXSPRITE1  4       /* Sprites/line in SCREEN 1-3    */
#define MAXSPRITE2  8       /* Sprites/line in SCREEN 4-8    */
#define MAXDRIVES   2       /* Number of disk drives         */
#define MAXDISKS    32      /* Number of disks for a drive   */
#define MAXSLOTS    6       /* Number of cartridge slots     */
#define MAXCARTS    2       /* Number of user cartridges     */
#define MAXMAPPERS  8       /* Total defined MegaROM mappers */
#define MAXCHUNKS   256     /* Max number of memory blocks   */
#define MAXCHEATS   256     /* Max number of cheats          */

#define MAXCHANNELS (AY8910_CHANNELS+YM2413_CHANNELS)
  /* Number of sound channels used by the emulation */

/** Model and options bits and macros ************************/
#define MODEL(M)        ((Mode&MSX_MODEL)==(M))
#define VIDEO(M)        ((Mode&MSX_VIDEO)==(M))
#define OPTION(M)       (Mode&(M))
#define ROMTYPE(N)      ((Mode>>(8+4*(N)))&0x0F)
#define ROMGUESS(N)     (Mode&(MSX_GUESSA<<(N)))
#define JOYTYPE(N)      ((Mode>>(4+2*(N)))&0x03)
#define SETROMTYPE(N,T) Mode=(Mode&~(0xF00<<(4*(N))))|((T)<<(8+4*(N)))
#define SETJOYTYPE(N,T) Mode=(Mode&~(0x030<<(2*(N))))|((T)<<(4+2*(N))) 

#define MSX_MODEL     0x00000003 /* Hardware Model:          */
#define MSX_MSX1      0x00000000 /* MSX1 computer (TMS9918)  */
#define MSX_MSX2      0x00000001 /* MSX2 computer (V9938)    */
#define MSX_MSX2P     0x00000002 /* MSX2+ computer (V9958)   */

#define MSX_VIDEO     0x00000004 /* Video System:            */
#define MSX_NTSC      0x00000000 /* NTSC computer (US/Japan) */
#define MSX_PAL       0x00000004 /* PAL computer (Europe)    */

#define MSX_JOYSTICKS 0x000000F0 /* Joystick Sockets:        */
#define MSX_SOCKET1   0x00000030 /* Joystick socket #1       */
#define MSX_SOCKET2   0x000000C0 /* Joystick socket #2       */
#define MSX_NOJOY1    0x00000000 /* No joystick in socket #1 */
#define MSX_JOY1      0x00000010 /* Joystick in socket #1    */
#define MSX_MOUSTICK1 0x00000020 /* Mouse as joystick #1     */
#define MSX_MOUSE1    0x00000030 /* Mouse in socket #1       */
#define MSX_NOJOY2    0x00000000 /* No joystick in socket #2 */
#define MSX_JOY2      0x00000040 /* Joystick in socket #2    */
#define MSX_MOUSTICK2 0x00000080 /* Mouse as joystick #2     */
#define MSX_MOUSE2    0x000000C0 /* Mouse in socket #2       */

#define MSX_MAPPERS   0x0003FF00 /* Cartridge Slots:         */
#define MSX_SLOTA     0x00010F00 /* Cartridge slot A (1:x)   */
#define MSX_SLOTB     0x0002F000 /* Cartridge slot B (2:x)   */
#define MSX_MAPPERA   0x00000F00 /* ROM mapper A (0..15)     */
#define MSX_MAPPERB   0x0000F000 /* ROM mapper B (0..15)     */
#define MSX_GUESSA    0x00010000 /* Guess ROM mapper type A  */
#define MSX_GUESSB    0x00020000 /* Guess ROM mapper type B  */

#define MSX_OPTIONS   0x7FFC0000 /* Miscellaneous Options:   */
#define MSX_ALLSPRITE 0x00800000 /* Show ALL sprites         */
#define MSX_AUTOFIREA 0x01000000 /* Autofire joystick FIRE-A */
#define MSX_AUTOFIREB 0x02000000 /* Autofire joystick FIRE-B */
#define MSX_AUTOSPACE 0x04000000 /* Autofire SPACE button    */
#define MSX_DRUMS     0x08000000 /* Hit MIDI drums for noise */
#define MSX_PATCHBDOS 0x10000000 /* Patch DiskROM routines   */
#define MSX_FIXEDFONT 0x20000000 /* Use fixed 8x8 text font  */
/*************************************************************/

/** Keyboard codes and macros ********************************/
extern const byte Keys[130][2];
extern volatile byte KeyState[16];

#define KBD_SET(K)   KeyState[Keys[K][0]]&=~Keys[K][1]
#define KBD_RES(K)   KeyState[Keys[K][0]]|=Keys[K][1]

#define KBD_LEFT     0x01
#define KBD_UP       0x02
#define KBD_RIGHT    0x03
#define KBD_DOWN     0x04
#define KBD_SHIFT    0x05
#define KBD_CONTROL  0x06
#define KBD_GRAPH    0x07
#define KBD_BS       0x08
#define KBD_TAB      0x09
#define KBD_CAPSLOCK 0x0A
#define KBD_SELECT   0x0B
#define KBD_HOME     0x0C
#define KBD_ENTER    0x0D
#define KBD_DELETE   0x0E
#define KBD_INSERT   0x0F
#define KBD_COUNTRY  0x10
#define KBD_STOP     0x11
#define KBD_F1       0x12
#define KBD_F2       0x13
#define KBD_F3       0x14
#define KBD_F4       0x15
#define KBD_F5       0x16
#define KBD_NUMPAD0  0x17
#define KBD_NUMPAD1  0x18
#define KBD_NUMPAD2  0x19
#define KBD_NUMPAD3  0x1A
#define KBD_ESCAPE   0x1B
#define KBD_NUMPAD4  0x1C
#define KBD_NUMPAD5  0x1D
#define KBD_NUMPAD6  0x1E
#define KBD_NUMPAD7  0x1F
#define KBD_SPACE    0x20
#define KBD_NUMPAD8  0x80
#define KBD_NUMPAD9  0x81
/*************************************************************/

/** Cheats() arguments ***************************************/
#define CHTS_OFF      0               /* Turn all cheats off */
#define CHTS_ON       1               /* Turn all cheats on  */
#define CHTS_TOGGLE   2               /* Toggle cheats state */
#define CHTS_QUERY    3               /* Query cheats state  */
/*************************************************************/

/** Following macros can be used in screen drivers ***********/
#define BigSprites    (VDP[1]&0x01)   /* Zoomed sprites      */
#define Sprites16x16  (VDP[1]&0x02)   /* 16x16/8x8 sprites   */
#define ScreenON      (VDP[1]&0x40)   /* Show screen         */
#define SpritesOFF    (VDP[8]&0x02)   /* Don't show sprites  */
#define SolidColor0   (VDP[8]&0x20)   /* Solid/Tran. COLOR 0 */
#define PALVideo      (VDP[9]&0x02)   /* PAL/NTSC video      */
#define FlipEvenOdd   (VDP[9]&0x04)   /* Flip even/odd pages */
#define InterlaceON   (VDP[9]&0x08)   /* Interlaced screen   */
#define ScanLines212  (VDP[9]&0x80)   /* 212/192 scanlines   */
#define HScroll512    (VDP[25]&0x01)  /* HScroll both pages  */
#define MaskEdges     (VDP[25]&0x02)  /* Mask 8-pixel edges  */
#define ModeYJK       (VDP[25]&0x08)  /* YJK screen mode     */
#define ModeYAE       (VDP[25]&0x10)  /* YJK/YAE screen mode */
#define VScroll       VDP[23]
#define HScroll       ((VDP[27]&0x07)|((int)(VDP[26]&0x3F)<<3))
#define VAdjust       (-((signed char)(VDP[18])>>4))
#define HAdjust       (-((signed char)(VDP[18]<<4)>>4))
/*************************************************************/

/** Variables used to control emulator behavior **************/
extern byte Verbose;                  /* Debug msgs ON/OFF   */
extern int  Mode;                     /* ORed MSX_* bits     */
extern int  RAMPages,VRAMPages;       /* Number of RAM pages */
extern byte UPeriod;                  /* % of frames to draw */
/*************************************************************/

/** Screen Mode Handlers [number of screens + 1] *************/
extern void (*RefreshLine[MAXSCREEN+2])(byte Y);
/*************************************************************/

extern Z80  CPU;                      /* CPU state/registers */
extern byte *VRAM;                    /* Video RAM           */
extern byte VDP[64];                  /* VDP control reg-ers */
extern byte VDPStatus[16];            /* VDP status reg-ers  */
extern byte *ChrGen,*ChrTab,*ColTab;  /* VDP tables (screen) */
extern byte *SprGen,*SprTab;          /* VDP tables (sprites)*/
extern int  ChrGenM,ChrTabM,ColTabM;  /* VDP masks (screen)  */
extern int  SprTabM;                  /* VDP masks (sprites) */
extern byte FGColor,BGColor;          /* Colors              */
extern byte XFGColor,XBGColor;        /* Alternative colors  */
extern byte ScrMode;                  /* Current screen mode */
extern int  ScanLine;                 /* Current scanline    */
extern byte *FontBuf;                 /* Optional fixed font */

extern byte ExitNow;                  /* 1: Exit emulator    */

extern byte PSLReg;                   /* Primary slot reg.   */
extern byte SSLReg[4];                /* Secondary slot reg. */

extern const char *ProgDir;           /* Program directory   */
extern const char *ROMName[MAXCARTS]; /* Cart A/B ROM files  */
extern const char *DSKName[MAXDRIVES];/* Disk A/B images     */
extern const char *SndName;           /* Soundtrack log file */
extern const char *PrnName;           /* Printer redir. file */
extern const char *CasName;           /* Tape image file     */
extern const char *ComName;           /* Serial redir. file  */
extern const char *STAName;           /* State save name     */
extern const char *FNTName;           /* Font file for text  */ 

extern FDIDisk FDD[4];                /* Floppy disk images  */
extern FILE *CasStream;               /* Cassette I/O stream */

/** StartMSX() ***********************************************/
/** Allocate memory, load ROM image, initialize hardware,   **/
/** CPU and start the emulation. This function returns 0 in **/
/** the case of failure.                                    **/
/*************************************************************/
int StartMSX(int NewMode,int NewRAMPages,int NewVRAMPages);

/** TrashMSX() ***********************************************/
/** Free memory allocated by StartMSX().                    **/
/*************************************************************/
void TrashMSX(void);

/** ResetMSX() ***********************************************/
/** Reset MSX hardware to new operating modes. Returns new  **/
/** modes, possibly not the same as NewMode.                **/
/*************************************************************/
int ResetMSX(int NewMode,int NewRAMPages,int NewVRAMPages);

/** MenuMSX() ************************************************/
/** Invoke a menu system allowing to configure the emulator **/
/** and perform several common tasks.                       **/
/*************************************************************/
void MenuMSX(void);

/** LoadFile() ***********************************************/
/** Simple utility function to load cartridge, state, font  **/
/** or a disk image, based on the file extension, etc.      **/
/*************************************************************/
int LoadFile(const char *FileName);

/** LoadCart() ***********************************************/
/** Load cartridge into given slot. Returns cartridge size  **/
/** in 16kB pages on success, 0 on failure.                 **/
/*************************************************************/
int LoadCart(const char *FileName,int Slot,int Type);

/** SaveSTA() ************************************************/
/** Save emulation state to a .STA file.                    **/
/*************************************************************/
int SaveSTA(const char *FileName);

/** LoadSTA() ************************************************/
/** Load emulation state from a .STA file.                  **/
/*************************************************************/
int LoadSTA(const char *FileName);

/** LoadCHT() ************************************************/
/** Load cheats from .CHT file. Cheat format is either      **/
/** 00XXXXXX-XX (one byte) or 00XXXXXX-XXXX (two bytes) for **/
/** ROM-based cheats and XXXX-XX or XXXX-XXXX for RAM-based **/
/** cheats. Returns the number of cheats on success, 0 on   **/
/** failure.                                                **/
/*************************************************************/
int LoadCHT(const char *Name);

/** SaveCHT() ************************************************/
/** Save cheats to a given text file. Returns the number of **/
/** cheats on success, 0 on failure.                        **/
/*************************************************************/
int SaveCHT(const char *Name);

/** LoadPAL() ************************************************/
/** Load new palette from .PAL file. Returns number of      **/
/** loaded colors on success, 0 on failure.                 **/
/*************************************************************/
int LoadPAL(const char *Name);

/** MakeFileName() *******************************************/
/** Make a copy of the file name, replacing the extension.  **/
/** Returns allocated new name or 0 on failure.             **/
/*************************************************************/
char *MakeFileName(const char *FileName,const char *Extension);

/** ChangePrinter() ******************************************/
/** Change printer output to a given file. The previous     **/
/** file is closed. ChangePrinter(0) redirects output to    **/
/** stdout. Returns 1 on success, 0 on failure.             **/
/*************************************************************/
void ChangePrinter(const char *FileName);

/** ChangeTape() *********************************************/
/** Change tape image. ChangeTape(0) closes current image.  **/
/** Returns 1 on success, 0 on failure.                     **/
/*************************************************************/
byte ChangeTape(const char *FileName);

/** RewindTape() *********************************************/
/** Rewind currenly open tape.                              **/
/*************************************************************/
void RewindTape(void);

/** ChangeDisk() *********************************************/
/** Change disk image in a given drive. Closes current disk **/
/** image if Name=0 was given. Creates a new disk image if  **/
/** Name="" was given. Returns 1 on success or 0 on failure.**/
/*************************************************************/
byte ChangeDisk(byte N,const char *FileName);

/** LoadFNT() ************************************************/
/** Load fixed 8x8 font used in text screen modes when      **/
/** MSX_FIXEDFONT option is enabled. LoadFNT(0) frees the   **/
/** font buffer. Returns 1 on success, 0 on failure.        **/
/*************************************************************/
byte LoadFNT(const char *FileName);

/** SetScreenDepth() *****************************************/
/** Set screen depth for the display drivers. Returns 1 on  **/
/** success, 0 on failure.                                  **/
/*************************************************************/
int SetScreenDepth(int Depth);

/** AddCheat() ***********************************************/
/** Add a new cheat. Returns 0 on failure or the number of  **/
/** cheats on success.                                      **/
/*************************************************************/
int AddCheat(const char *Cheat);

/** DelCheat() ***********************************************/
/** Delete a cheat. Returns 0 on failure, 1 on success.     **/
/*************************************************************/
int DelCheat(const char *Cheat);

/** ResetCheats() ********************************************/
/** Remove all cheats.                                      **/
/*************************************************************/
void ResetCheats(void);

/** Cheats() *************************************************/
/** Toggle cheats on (1), off (0), inverse state (2) or     **/
/** query (3).                                              **/
/*************************************************************/
int Cheats(int Switch);

#ifdef NEW_STATES
/** SaveState() **********************************************/
/** Save emulation state to a memory buffer. Returns size   **/
/** on success, 0 on failure.                               **/
/*************************************************************/
unsigned int SaveState(unsigned char *Buf,unsigned int MaxSize);

/** LoadState() **********************************************/
/** Load emulation state from a memory buffer. Returns size **/
/** on success, 0 on failure.                               **/
/*************************************************************/
unsigned int LoadState(unsigned char *Buf,unsigned int MaxSize);
#endif /* NEW_STATES */

/** InitMachine() ********************************************/
/** Allocate resources needed by the machine-dependent code.**/
/************************************ TO BE WRITTEN BY USER **/
int InitMachine(void);

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/************************************ TO BE WRITTEN BY USER **/
void TrashMachine(void);

/** Keyboard() ***********************************************/
/** This function is periodically called to poll keyboard.  **/
/************************************ TO BE WRITTEN BY USER **/
void Keyboard(void);

/** Joystick() ***********************************************/
/** Query positions of two joystick connected to ports 0/1. **/
/** Returns 0.0.B2.A2.R2.L2.D2.U2.0.0.B1.A1.R1.L1.D1.U1.    **/
/************************************ TO BE WRITTEN BY USER **/
unsigned int Joystick(void);

/** Mouse() **************************************************/
/** Query coordinates of a mouse connected to port N.       **/
/** Returns F2.F1.Y.Y.Y.Y.Y.Y.Y.Y.X.X.X.X.X.X.X.X.          **/
/************************************ TO BE WRITTEN BY USER **/
unsigned int Mouse(byte N);

/** DiskPresent()/DiskRead()/DiskWrite() *********************/
/*** These three functions are called to check for floppyd  **/
/*** disk presence in the "drive", and to read/write given  **/
/*** sector to the disk.                                    **/
/************************************ TO BE WRITTEN BY USER **/
byte DiskPresent(byte ID);
byte DiskRead(byte ID,byte *Buf,int N);
byte DiskWrite(byte ID,const byte *Buf,int N);

/** SetColor() ***********************************************/
/** Set color N (0..15) to (R,G,B).                         **/
/************************************ TO BE WRITTEN BY USER **/
void SetColor(byte N,byte R,byte G,byte B);

/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/************************************ TO BE WRITTEN BY USER **/
void RefreshScreen(void);

/** RefreshLine#() *******************************************/
/** Refresh line Y (0..191/211), on an appropriate SCREEN#, **/
/** including sprites in this line.                         **/
/************************************ TO BE WRITTEN BY USER **/
void RefreshLineTx80(byte Y);
void RefreshLine0(byte Y);
void RefreshLine1(byte Y);
void RefreshLine2(byte Y);
void RefreshLine3(byte Y);
void RefreshLine4(byte Y);
void RefreshLine5(byte Y);
void RefreshLine6(byte Y);
void RefreshLine7(byte Y);
void RefreshLine8(byte Y);
void RefreshLine10(byte Y);
void RefreshLine12(byte Y);

#ifdef __cplusplus
}
#endif
#endif /* MSX_H */
