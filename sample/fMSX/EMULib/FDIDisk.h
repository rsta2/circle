/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        FDIDisk.h                        **/
/**                                                         **/
/** This file declares routines to load, save, and access   **/
/** disk images in various formats. The internal format is  **/
/** always .FDI. See FDIDisk.c for the actual code.         **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2007-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef FDIDISK_H
#define FDIDISK_H
#ifdef __cplusplus
extern "C" {
#endif

#define FMT_AUTO   0
#define FMT_IMG    1
#define FMT_MGT    2
#define FMT_TRD    3
#define FMT_FDI    4
#define FMT_SCL    5
#define FMT_HOBETA 6
#define FMT_DSK    7
#define FMT_CPCDSK 8
#define FMT_SF7000 9

#define DataFDI(D) ((D)->Data+(D)->Data[10]+((int)((D)->Data[11])<<8))

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

/** FDIDisk **************************************************/
/** This structure contains all disk image information and  **/
/** also the result of the last SeekFDI() call.             **/
/*************************************************************/
typedef struct
{
  byte Format;     /* Original disk format (FMT_*) */
  int  Sides;      /* Sides per disk */
  int  Tracks;     /* Tracks per side */
  int  Sectors;    /* Sectors per track */
  int  SecSize;    /* Bytes per sector */

  byte *Data;      /* Disk data */
  int  DataSize;   /* Disk data size */

  byte Header[6];  /* Current header, result of SeekFDI() */
  byte Verbose;    /* 1: Print debugging messages */
} FDIDisk;

/** InitFDI() ************************************************/
/** Clear all data structure fields.                        **/
/*************************************************************/
void InitFDI(FDIDisk *D);

/** EjectFDI() ***********************************************/
/** Eject disk image. Free all allocated memory.            **/
/*************************************************************/
void EjectFDI(FDIDisk *D);

/** NewFDI() *************************************************/
/** Allocate memory and create new .FDI disk image of given **/
/** dimensions. Returns disk data pointer on success, 0 on  **/
/** failure.                                                **/
/*************************************************************/
byte *NewFDI(FDIDisk *D,int Sides,int Tracks,int Sectors,int SecSize);

/** LoadFDI() ************************************************/
/** Load a disk image from a given file, in a given format  **/
/** (see FMT_* #defines). Guess format from the file name   **/
/** when Format=FMT_AUTO. Returns format ID on success or   **/
/** 0 on failure. When FileName=0, ejects the disk freeing  **/
/** memory and returns 0.                                   **/
/*************************************************************/
int LoadFDI(FDIDisk *D,const char *FileName,int Format);

/** SaveFDI() ************************************************/
/** Save a disk image to a given file, in a given format    **/
/** (see FMT_* #defines). Use the original format when      **/
/** when Format=FMT_AUTO. Returns format ID on success or   **/
/** 0 on failure.                                           **/
/*************************************************************/
int SaveFDI(FDIDisk *D,const char *FileName,int Format);

/** SeekFDI() ************************************************/
/** Seek to given side/track/sector. Returns sector address **/
/** on success or 0 on failure.                             **/
/*************************************************************/
byte *SeekFDI(FDIDisk *D,int Side,int Track,int SideID,int TrackID,int SectorID);

#ifdef __cplusplus
}
#endif
#endif /* FDIDISK_H */
