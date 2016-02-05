/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         Floppy.h                        **/
/**                                                         **/
/** This file declares functions to operate on 720kB        **/
/** floppy disk images. See Floppy.c for implementation.    **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2004-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef FLOPPY_H
#define FLOPPY_H

#define DSK_SIDS_PER_DISK 2
#define DSK_TRKS_PER_SIDE 80
#define DSK_SECS_PER_TRCK 9
#define DSK_SECTOR_SIZE   512

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

/** DSKCreate() **********************************************/
/** Create disk image in Dsk or allocate memory when Dsk=0. **/
/** Returns pointer to the disk image or 0 on failure.      **/
/*************************************************************/
byte *DSKCreate(byte *Dsk);

/** DSKFile() ************************************************/
/** Create file with a given name, return file ID or 0 on   **/
/** failure.                                                **/
/*************************************************************/
int DSKFile(byte *Dsk,const char *FileName);

/** DSKFileName() ********************************************/
/** Return name of a file with a given ID or 0 on failure.  **/
/*************************************************************/
const char *DSKFileName(const byte *Dsk,int ID);

/** DSKFileSize() ********************************************/
/** Return side of a file with a given ID or 0 on failure.  **/
/*************************************************************/
int DSKFileSize(const byte *Dsk,int ID);

/** DSKRead()/DSKWrite() *************************************/
/** Read or write a given number of bytes to a given file.  **/
/** Both functions return the number of bytes written or    **/
/** read from the file, or 0 on failure.                    **/
/*************************************************************/
int DSKWrite(byte *Dsk,int ID,const byte *Buf,int Size);
int DSKRead(const byte *Dsk,int ID,byte *Buf,int Size);

/** DSKDelete() **********************************************/
/** Delete file with a given ID. Returns ID on success or 0 **/
/** on failure.                                             **/
/*************************************************************/
int DSKDelete(byte *Dsk,int ID);

/** DSKLoad()/DSKSave() **************************************/
/** Load or save disk contents from/to a disk image or a    **/
/** directory. DSKLoad() will allocate space if Dsk=0. Both **/
/** functions return pointer to disk contents on success or **/
/** 0 on failure.                                           **/
/*************************************************************/
byte *DSKLoad(const char *Name,byte *Dsk);
const byte *DSKSave(const char *Name,const byte *Dsk);

#endif /* FLOPPY_H */
