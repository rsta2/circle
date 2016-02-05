/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         WD1793.h                        **/
/**                                                         **/
/** This file contains emulation for the WD1793/2793 disk   **/
/** controller produced by Western Digital. See WD1793.c    **/
/** for implementation.                                     **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2005-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef WD1793_H
#define WD1793_H

#include "FDIDisk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WD1793_KEEP    0
#define WD1793_INIT    1
#define WD1793_EJECT   2

#define WD1793_COMMAND 0
#define WD1793_STATUS  0
#define WD1793_TRACK   1
#define WD1793_SECTOR  2
#define WD1793_DATA    3
#define WD1793_SYSTEM  4
#define WD1793_READY   4

#define WD1793_IRQ     0x80
#define WD1793_DRQ     0x40

                           /* Common status bits:               */
#define F_BUSY     0x01    /* Controller is executing a command */
#define F_READONLY 0x40    /* The disk is write-protected       */
#define F_NOTREADY 0x80    /* The drive is not ready            */

                           /* Type-1 command status:            */
#define F_INDEX    0x02    /* Index mark detected               */
#define F_TRACK0   0x04    /* Head positioned at track #0       */
#define F_CRCERR   0x08    /* CRC error in ID field             */
#define F_SEEKERR  0x10    /* Seek error, track not verified    */
#define F_HEADLOAD 0x20    /* Head loaded                       */

                           /* Type-2 and Type-3 command status: */
#define F_DRQ      0x02    /* Data request pending              */
#define F_LOSTDATA 0x04    /* Data has been lost (missed DRQ)   */
#define F_ERRCODE  0x18    /* Error code bits:                  */
#define F_BADDATA  0x08    /* 1 = bad data CRC                  */
#define F_NOTFOUND 0x10    /* 2 = sector not found              */
#define F_BADID    0x18    /* 3 = bad ID field CRC              */
#define F_DELETED  0x20    /* Deleted data mark (when reading)  */
#define F_WRFAULT  0x20    /* Write fault (when writing)        */

#define C_DELMARK  0x01
#define C_SIDECOMP 0x02
#define C_STEPRATE 0x03
#define C_VERIFY   0x04
#define C_WAIT15MS 0x04
#define C_LOADHEAD 0x08
#define C_SIDE     0x08
#define C_IRQ      0x08
#define C_SETTRACK 0x10
#define C_MULTIREC 0x10

#define S_DRIVE    0x03
#define S_RESET    0x04
#define S_HALT     0x08
#define S_SIDE     0x10
#define S_DENSITY  0x20

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

typedef struct
{
  FDIDisk *Disk[4]; /* Disk images */

  byte R[5];        /* Registers */
  byte Drive;       /* Current disk # */
  byte Side;        /* Current side # */
  byte Track[4];    /* Current track # */
  byte LastS;       /* Last STEP direction */
  byte IRQ;         /* 0x80: IRQ pending, 0x40: DRQ pending */
  byte Wait;        /* Expiration counter */
  byte Cmd;         /* Last command */

  int  WRLength;    /* Data left to write */
  int  RDLength;    /* Data left to read */
  byte *Ptr;        /* Pointer to data */

  byte Verbose;     /* 1: Print debugging messages */
} WD1793;

/** Reset1793() **********************************************/
/** Reset WD1793. When Disks=WD1793_INIT, also initialize   **/
/** disks. When Disks=WD1793_EJECT, eject inserted disks,   **/
/** freeing memory.                                         **/
/*************************************************************/
void Reset1793(register WD1793 *D,FDIDisk *Disks,register byte Eject);

/** Read1793() ***********************************************/
/** Read value from a WD1793 register A. Returns read data  **/
/** on success or 0xFF on failure (bad register address).   **/
/*************************************************************/
byte Read1793(register WD1793 *D,register byte A);

/** Write1793() **********************************************/
/** Write value V into WD1793 register A. Returns DRQ/IRQ   **/
/** values.                                                 **/
/*************************************************************/
byte Write1793(register WD1793 *D,register byte A,register byte V);

#ifdef __cplusplus
}
#endif
#endif /* WD1793_H */
