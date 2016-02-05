/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        Record.h                         **/
/**                                                         **/
/** This file contains routines for gameplay recording and  **/
/** replay.                                                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2013-2015                 **/
/**     The contents of this file are property of Marat     **/
/**     Fayzullin and should only be used as agreed with    **/
/**     him. The file is confidential. Absolutely no        **/
/**     distribution allowed.                               **/
/*************************************************************/
#ifndef RECORD_H
#define RECORD_H

#include "EMULib.h"

#ifdef __cplusplus
extern "C" {
#endif

/** RPLPlay()/RPLRecord() arguments **************************/
#define RPL_OFF     0xFFFFFFFF       /* Disable              */
#define RPL_ON      0xFFFFFFFE       /* Enable               */
#define RPL_TOGGLE  0xFFFFFFFD       /* Toggle               */
#define RPL_RESET   0xFFFFFFFC       /* Reset and enable     */
#define RPL_NEXT    0xFFFFFFFB       /* Play the next record */
#define RPL_QUERY   0xFFFFFFFA       /* Query state          */

/** RPLPlay(RPL_NEXT) results ********************************/
#define RPL_ENDED   0xFFFFFFFF       /* Finished or stopped  */

/** RPLInit() ************************************************/
/** Initialize record/relay subsystem.                      **/
/*************************************************************/
void RPLInit(unsigned int (*SaveHandler)(unsigned char *,unsigned int),unsigned int (*LoadHandler)(unsigned char *,unsigned int),unsigned int MaxSize);

/** RPLTrash() ***********************************************/
/** Free all record/replay resources.                       **/
/*************************************************************/
void RPLTrash(void);

/** RPLRecord() **********************************************/
/** Record emulation state and joystick input for replaying **/
/** it back later.                                          **/
/*************************************************************/
int RPLRecord(unsigned int JoyState);

/** RPLRecordKeys() ******************************************/
/** Record emulation state, keys, and joystick input for    **/
/** replaying them back later.                              **/
/*************************************************************/
int RPLRecordKeys(unsigned int JoyState,const unsigned char *Keys,unsigned int KeySize);

/** RPLPlay() ************************************************/
/** Replay gameplay saved with RPLRecord().                 **/
/*************************************************************/
unsigned int RPLPlay(int Cmd);

/** RPLPlayKeys() ********************************************/
/** Replay gameplay saved with RPLRecordKeys().             **/
/*************************************************************/
unsigned int RPLPlayKeys(int Cmd,unsigned char *Keys,unsigned int KeySize);

/** RPLShow() ************************************************/
/** Draw replay icon when active.                           **/
/*************************************************************/
void RPLShow(Image *Img,int X,int Y);

/** SaveRPL() ************************************************/
/** Save gameplay recording into given file.                **/
/*************************************************************/
int SaveRPL(const char *FileName);

/** LoadRPL() ************************************************/
/** Load gameplay recording from given file.                **/
/*************************************************************/
int LoadRPL(const char *FileName);

#ifdef __cplusplus
}
#endif
#endif /* RECORD_H */
