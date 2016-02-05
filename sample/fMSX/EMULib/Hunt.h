/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         Hunt.h                          **/
/**                                                         **/
/** This file declares functions to search for possible     **/
/** cheats inside running game data. Also see Hunt.c.       **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2013-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef HUNT_H
#define HUNT_H

#define HUNT_BUFSIZE 1024

/** Flags used in AddHUNT() **********************************/
#define HUNT_MASK_ID     0x00FF
#define HUNT_MASK_CHANGE 0x0700
#define   HUNT_CONSTANT    0x0000
#define   HUNT_PLUSONE     0x0100
#define   HUNT_PLUSMANY    0x0200
#define   HUNT_MINUSONE    0x0300
#define   HUNT_MINUSMANY   0x0400
#define HUNT_MASK_SIZE   0x1800
#define   HUNT_8BIT        0x0000
#define   HUNT_16BIT       0x0800
#define   HUNT_32BIT       0x1000

/** Types used in HUNT2Cheat() *******************************/
#define HUNT_GBA_GS 0           /* GBA GameShark Advance     */
#define HUNT_GBA_CB 1           /* GBA CodeBreaker Advance   */
#define HUNT_SMS_AR 2           /* SMS Pro Action Replay     */
#define HUNT_NES_AR 3           /* NES Pro Action Replay     */
#define HUNT_GB_GS  4           /* GB GameShark              */
#define HUNT_COLECO 5           /* ColecoVision cheats       */
#define HUNT_MSX    6           /* MSX cheats                */
#define HUNT_ZXS    7           /* ZX Spectrum cheats        */

/** HUNTEntry ************************************************/
/** Search entry containing data on one memory location.    **/
/*************************************************************/
typedef struct
{
  unsigned int Addr;    /* Memory location address           */
  unsigned int Orig;    /* Original value at the address     */
  unsigned int Value;   /* Current value at the address      */
  unsigned short Flags; /* Options supplied in AddHUNT()     */
  unsigned short Count; /* Number of detected changes        */
} HUNTEntry;

/** InitHUNT() ***********************************************/
/** Initialize cheat search, clearing all data.             **/
/*************************************************************/
void InitHUNT(void);

/** AddHUNT() ************************************************/
/** Add a new value to search for, with the address range   **/
/** to search in. Returns number of memory locations found. **/
/*************************************************************/
int AddHUNT(unsigned int Addr,unsigned int Size,unsigned int Value,unsigned int NewValue,unsigned int Flags);

/** ScanHUNT() ***********************************************/
/** Scan memory for changed values and update search data.  **/
/** Returns number of memory locations updated.             **/
/*************************************************************/
int ScanHUNT(void);

/** TotalHUNT() **********************************************/
/** Get total number of currently watched locations.        **/
/*************************************************************/
int TotalHUNT(void);

/** GetHUNT() ************************************************/
/** Get Nth memory location. Returns 0 for invalid N.       **/
/*************************************************************/
HUNTEntry *GetHUNT(int N);

/** HUNT2Cheat() *********************************************/
/** Create cheat code from Nth hunt entry. Returns 0 if the **/
/** entry is invalid.                                       **/
/*************************************************************/
const char *HUNT2Cheat(int N,unsigned int Type);

#endif /* HUNT_H */
