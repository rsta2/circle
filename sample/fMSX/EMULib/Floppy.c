/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         Floppy.c                        **/
/**                                                         **/
/** This file implements functions to operate on 720kB      **/
/** floppy disk images. See Floppy.h for declarations.      **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2004-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "Floppy.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>

#if defined(UIQ) || defined(S60) || defined(UNIX) || defined(MAEMO) || defined(MEEGO) || defined(ANDROID)
#include <dirent.h>
#else
#include <direct.h>
#endif

#ifdef ZLIB
#include <zlib.h>
#endif

#define DSK_RESERVED_SECS 0
#define DSK_FATS_PER_DISK 2
#define DSK_SECS_PER_BOOT 1
#define DSK_SECS_PER_FAT  3
#define DSK_SECS_PER_DIR  7
#define DSK_SECS_PER_CLTR 2
#define DSK_CLUSTER_SIZE  (DSK_SECTOR_SIZE*DSK_SECS_PER_CLTR)
#define DSK_DIR_SIZE      112
#define DSK_DISK_SIZE     (DSK_SECS_PER_DISK*DSK_SECTOR_SIZE)

#define DSK_SECS_PER_DISK (\
  DSK_SIDS_PER_DISK \
* DSK_TRKS_PER_SIDE \
* DSK_SECS_PER_TRCK)

#define DSK_FAT_SIZE ((\
  DSK_SECS_PER_DISK \
- DSK_SECS_PER_BOOT \
- DSK_RESERVED_SECS \
- DSK_FATS_PER_DISK*DSK_SECS_PER_FAT \
- DSK_SECS_PER_DIR)/2)

#define DIRENTRY(Dsk,ID) (\
  (Dsk) \
+ DSK_SECTOR_SIZE*(DSK_RESERVED_SECS+DSK_SECS_PER_BOOT) \
+ DSK_SECTOR_SIZE*DSK_SECS_PER_FAT*DSK_FATS_PER_DISK \
+ ((ID)*32) \
)

/** FindFreeCluster() ****************************************/
/** Finds first free cluster starting from the given one.   **/
/** Returns cluster number on success or 0 on failure.      **/
/*************************************************************/
static int FindFreeCluster(const byte *Dsk,int Start)
{
  const byte *FE;
  int J;

  /* Search for the first zeroed entry */
  for(Start=Start>=2? Start:2;Start<DSK_FAT_SIZE;Start++)
  {
    /* FAT entry address */
    FE = Dsk + DSK_SECTOR_SIZE*(DSK_RESERVED_SECS+1) + 3*(Start>>1);
    /* Read entry contents */
    J = Start&0x001? (FE[1]>>4)+((int)FE[2]<<4)
                   : FE[0]+((int)(FE[1]&0x0F)<<8);
    if(!J) return(Start);
  }

  /* No free clusters found */
  return(0);
}

/** DSKCreate() **********************************************/
/** Create disk image in Dsk or allocate memory when Dsk=0. **/
/** Returns pointer to the disk image or 0 on failure.      **/
/*************************************************************/
byte *DSKCreate(byte *Dsk)
{
  byte *FAT,*DIR,*DAT;

  /* Allocate memory if needed */
  if(!Dsk)
  {
    Dsk=(byte *)malloc(DSK_DISK_SIZE);
    if(!Dsk) return(0);
  }

  /* FAT and directory pointers */
  FAT = Dsk + DSK_SECTOR_SIZE*(DSK_RESERVED_SECS+1);
  DIR = FAT + DSK_SECTOR_SIZE*DSK_SECS_PER_FAT*DSK_FATS_PER_DISK;
  DAT = DIR + DSK_DIR_SIZE*32;

  /* Clear disk image */
  memset(Dsk,0x00,DSK_DISK_SIZE);

  /* Boot sector parameters */
  Dsk[0x00] = 0xC9;
  Dsk[0x01] = 0xC9;
  Dsk[0x02] = 0xC9;
  memcpy(Dsk+3,"MSX-DISK",8);
  Dsk[0x0B] = DSK_SECTOR_SIZE&0xFF; 
  Dsk[0x0C] = (DSK_SECTOR_SIZE>>8)&0xFF;
  Dsk[0x0D] = DSK_SECS_PER_CLTR;
  Dsk[0x0E] = DSK_SECS_PER_BOOT;
  Dsk[0x10] = DSK_FATS_PER_DISK;
  Dsk[0x11] = DSK_DIR_SIZE&0xFF;
  Dsk[0x12] = (DSK_DIR_SIZE>>8)&0xFF;
  Dsk[0x13] = DSK_SECS_PER_DISK&0xFF;
  Dsk[0x14] = (DSK_SECS_PER_DISK>>8)&0xFF;
  Dsk[0x15] = 0xF9;                         /* Media ID */
  Dsk[0x16] = DSK_SECS_PER_FAT&0xFF;
  Dsk[0x17] = (DSK_SECS_PER_FAT>>8)&0xFF;
  Dsk[0x18] = 9;                            /* Sectors per track */
  Dsk[0x19] = 0;
  Dsk[0x1A] = 2;                            /* Heads per disk */
  Dsk[0x1B] = 0;
  Dsk[0x1C] = DSK_RESERVED_SECS&0xFF;
  Dsk[0x1D] = (DSK_RESERVED_SECS>>8)&0xFF;
  Dsk[0x1E] = 0xC9;

  /* First two FAT entries are not used */
  FAT[0] = 0xF9;
  FAT[1] = 0xFF;
  FAT[2] = 0xFF;

  /* Done */
  return(Dsk);
}

/** DSKFile() ************************************************/
/** Create file with a given name, return file ID, or 0 on  **/
/** failure.                                                **/
/*************************************************************/
int DSKFile(byte *Dsk,const char *FileName)
{
  byte *P;
  int I,J;

  /* See if file exists */
  for(J=1;J<=DSK_DIR_SIZE;J++)
    if(DSKFileName(Dsk,J)&&!memcmp(FileName,DSKFileName(Dsk,J),11))
      return(0);

  /* Find empty spot */
  for(J=1;J<=DSK_DIR_SIZE;J++)
    if(!DSKFileName(Dsk,J))
    {
      I=FindFreeCluster(Dsk,2);
      P=DIRENTRY(Dsk,J-1);
      memset(P,0x00,32);
      memcpy(P,FileName,11);
      P[0x0B] = 0x00;           /* Attributes    */
      P[0x0C] = 0x00;           /* Reserved      */
      P[0x1A] = I&0xFF;         /* First cluster */
      P[0x1B] = (I>>8)&0x0F;
      P[0x1C] = 0x00;           /* File size     */
      P[0x1D] = 0x00;
      P[0x1E] = 0x00;
      P[0x1F] = 0x00;
      return(J);
    }

  /* No free entries */
  return(0);
}

/** DSKFileName() ********************************************/
/** Return name of a file with a given ID or 0 on failure.  **/
/*************************************************************/
const char *DSKFileName(const byte *Dsk,int ID)
{
  const unsigned char *Name;

  /* Can't have ID that is out of bounds */
  if((ID<1)||(ID>DSK_DIR_SIZE)) return(0);
  /* Return file name */
  Name=DIRENTRY(Dsk,ID-1);
  return(!Name[0]||(Name[0]==0xE5)? 0:Name);
}

/** DSKFileSize() ********************************************/
/** Return side of a file with a given ID or 0 on failure.  **/
/*************************************************************/
int DSKFileSize(const byte *Dsk,int ID)
{
  const byte *Name;

  /* Can't have ID that is out of bounds */
  if((ID<1)||(ID>DSK_DIR_SIZE)) return(0);
  /* Return file size */
  Name=DIRENTRY(Dsk,ID-1);
  return(!Name[0]||(Name[0]==0xE5)? 0
  : (Name[0x1C]+Name[0x1D]*256)
  + (Name[0x1E]+Name[0x1F]*256)*256
  );
}

/** DSKRead()/DSKWrite() *************************************/
/** Read or write a given number of bytes to a given file.  **/
/** Both functions return the number of bytes written or    **/
/** read from the file, or 0 on failure.                    **/
/*************************************************************/
int DSKWrite(byte *Dsk,int ID,const byte *Buf,int Size)
{
  byte *DE,*FE,*FE2,*P;
  int I,J,Written;

  /* Can't have ID that is out of bounds */
  if((ID<1)||(ID>DSK_DIR_SIZE)) return(0);

  /* Get directory entry */
  DE=DIRENTRY(Dsk,ID-1);
  if(!DE[0]||(DE[0]==0xE5)) return(0);

  /* Delete file contents, leave name intact */
  J=DE[0];
  DSKDelete(Dsk,ID);
  DE[0]=J;

  /* Find first free cluster and store it */
  J=FindFreeCluster(Dsk,2);
  if(!J) { DSKDelete(Dsk,ID);return(0); }
  DE[0x1A] = J&0xFF;
  DE[0x1B] = (J>>8)&0x0F;

  for(Written=0;(J!=0xFFF)&&(Written<Size);J=I)
  {
    P       = Dsk
            + DSK_SECTOR_SIZE*(DSK_RESERVED_SECS+DSK_SECS_PER_BOOT)
            + DSK_SECTOR_SIZE*DSK_SECS_PER_FAT*DSK_FATS_PER_DISK
            + DSK_SECTOR_SIZE*DSK_SECS_PER_DIR;
    I       = Size-Written<DSK_CLUSTER_SIZE? Size-Written:DSK_CLUSTER_SIZE;
    memcpy(P+DSK_CLUSTER_SIZE*(J-2),Buf,I);
    Buf    += I;
    Written+= I;

    /* Find next free cluster */
    I       = Written<Size? FindFreeCluster(Dsk,J+1):0xFFF;
    I       = I? I:0xFFF;

    /* FAT entry address */
    FE  = Dsk
        + DSK_SECTOR_SIZE*(DSK_RESERVED_SECS+DSK_SECS_PER_BOOT)
        + 3*(J>>1);
    FE2 = FE
        + DSK_SECTOR_SIZE*DSK_SECS_PER_FAT; 

    /* Chain clusters */
    if(J&0x001) { FE2[1]=FE[1]=(FE[1]&0x0F)|((I&0x0F)<<4);FE2[2]=FE[2]=I>>4; }
    else        { FE2[1]=FE[1]=(FE[1]&0xF0)|(I>>8);FE2[0]=FE[0]=I&0xFF; }
  }

  /* Store new file size */
  DE[0x1C] = Written&0xFF;
  DE[0x1D] = (Written>>8)&0xFF;
  DE[0x1E] = (Written>>16)&0xFF;
  DE[0x1F] = (Written>>24)&0xFF;
   
  /* Done */
  return(Written);
}

int DSKRead(const byte *Dsk,int ID,byte *Buf,int Size)
{
  const byte *P;
  int I,J,Read;

  /* Can't have ID that is out of bounds */
  if((ID<1)||(ID>DSK_DIR_SIZE)) return(0);

  /* Get directory entry */
  P=DIRENTRY(Dsk,ID-1);
  if(!P[0]||(P[0]==0xE5)) return(0);

  /* Find first free cluster and store it */
  J=P[0x1A]+((int)(P[0x1B]&0x0F)<<8);

  /* Update size to read */
  if(DSKFileSize(Dsk,ID)<Size) Size=DSKFileSize(Dsk,ID);

  /* Read data */
  for(Read=0;(Read<Size)&&(J>1)&&(J<0xFF1);)
  {
    P    = Dsk
         + DSK_SECTOR_SIZE*(DSK_RESERVED_SECS+DSK_SECS_PER_BOOT)
         + DSK_SECTOR_SIZE*DSK_SECS_PER_FAT*DSK_FATS_PER_DISK
         + DSK_SECTOR_SIZE*DSK_SECS_PER_DIR;
    I    = Size-Read<DSK_CLUSTER_SIZE? Size-Read:DSK_CLUSTER_SIZE;
    memcpy(Buf,P+DSK_CLUSTER_SIZE*(J-2),I);
    Buf += I;
    Read+= I;
    P    = Dsk
         + DSK_SECTOR_SIZE*(DSK_RESERVED_SECS+DSK_SECS_PER_BOOT)
         + 3*(J>>1);
    J    = J&0x001? (P[1]>>4)+((int)P[2]<<4):P[0]+((int)(P[1]&0x0F)<<8);
  }

  /* Done */
  return(Read);
}

/** DSKDelete() **********************************************/
/** Delete file with a given ID. Returns ID on success or 0 **/
/** on failure.                                             **/
/*************************************************************/
int DSKDelete(byte *Dsk,int ID)
{
  byte *DE,*FE,*FE2;
  int J;

  /* Can't have ID that is out of bounds */
  if((ID<1)||(ID>DSK_DIR_SIZE)) return(0);

  /* Get directory entry */
  DE=DIRENTRY(Dsk,ID-1);
  if(!DE[0]||(DE[0]==0xE5)) return(0);

  /* First cluster */
  J=DE[0x1A]+((int)(DE[0x1B]&0x0F)<<8);

  /* Free clusters */
  while((J>1)&&(J<0xFF1))
  {
    /* FAT entry address */
    FE  = Dsk
        + DSK_SECTOR_SIZE*(DSK_RESERVED_SECS+DSK_SECS_PER_BOOT)
        + 3*(J>>1);
    FE2 = FE
        + DSK_SECTOR_SIZE*DSK_SECS_PER_FAT; 

    /* Delete entry */
    if(J&0x001)
    {
      J=(FE[1]>>4)+((int)FE[2]<<4);
      if((J<0xFF1)||(J>0xFF7)) { FE2[1]=FE[1]&=0x0F;FE2[2]=FE[2]=0x00; }
    }
    else
    {
      J=FE[0]+((int)(FE[1]&0x0F)<<8);
      if((J<0xFF1)||(J>0xFF7)) { FE2[1]=FE[1]&=0xF0;FE2[0]=FE[0]=0x00; }
    }
  }

  /* Mark file as deleted */
  DE[0]=0xE5;
  /* Done */ 
  return(ID);
}

/** DSKLoad()/DSKSave() **************************************/
/** Load or save disk contents from/to a disk image or a    **/
/** directory. DSKLoad() will allocate space if Dsk=0. Both **/
/** functions return pointer to disk contents on success or **/
/** 0 on failure.                                           **/
/*************************************************************/
byte *DSKLoad(const char *Name,byte *Dsk)
{
  byte *Dsk1,*Buf,FN[32],*Path;
  struct stat FS;
  struct dirent *DE;
  FILE *F;
  DIR *D;
  int J,I;

  /* Create disk image */
  Dsk1=DSKCreate(Dsk);
  if(!Dsk1) return(0);

  /* If <Name> is a directory... */
  if(!stat(Name,&FS)&&S_ISDIR(FS.st_mode))
  {
    /* Open directory */
    D=opendir(Name);
    if(!D) { if(!Dsk) free(Dsk1);return(0); }

    /* Scan, read, store files */
    while(DE=readdir(D))
      if(Path=malloc(strlen(Name)+strlen(DE->d_name)+5))
      {
        /* Compose full input file name */
        strcpy(Path,Name);
        I=strlen(Path);
        if(Path[I-1]!='/') Path[I++]='/';
        strcpy(Path+I,DE->d_name);

        /* Compose 8.3 file name */
        for(J=0;(J<8)&&DE->d_name[J]&&(DE->d_name[J]!='.');J++)
          FN[J]=toupper(DE->d_name[J]);
        for(I=J;I<8;I++) FN[I]=' ';
        for(;DE->d_name[J]&&(DE->d_name[J]!='.');J++);
        if(DE->d_name[J]) J++;
        for(;(I<11)&&DE->d_name[J];I++,J++)
          FN[I]=toupper(DE->d_name[J]);
        for(;I<11;I++) FN[I]=' ';
        FN[I]='\0';

        /* Open input file */
        if(!stat(Path,&FS)&&S_ISREG(FS.st_mode)&&FS.st_size)
          if(F=fopen(Path,"rb"))
          {
            /* Allocate input buffer */
            if(Buf=malloc(FS.st_size))
            {
              /* Read file into the buffer */
              if(fread(Buf,1,FS.st_size,F)==FS.st_size)
              {
                /* Create and write floppy file */
                if(I=DSKFile(Dsk1,FN))
                  if(DSKWrite(Dsk1,I,Buf,FS.st_size)!=FS.st_size)
                    DSKDelete(Dsk1,I);
              }
              /* Done with the input buffer */
              free(Buf);
            }
            /* Done with the input file */
            fclose(F);
          }

        /* Done with the full input file name */
        free(Path);
      }

    /* Done processing directory */
    closedir(D);
    return(Dsk1);
  }

#ifdef ZLIB
#define fopen(N,M)      (FILE *)gzopen(N,M)
#define fclose(F)       gzclose((gzFile)(F))
#define fread(B,L,N,F)  gzread((gzFile)(F),B,(L)*(N))
#endif

  /* Assume <Name> to be a disk image file */
  F=fopen(Name,"rb");
  if(!F) { if(!Dsk) free(Dsk1);return(0); }

  /* Read data */
  if(fread(Dsk1,1,DSK_DISK_SIZE,F)!=DSK_DISK_SIZE)
  { if(!Dsk) free(Dsk1);fclose(F);return(0); }

  /* Done */
  fclose(F);
  return(Dsk1);

#ifdef ZLIB
#undef fopen
#undef fclose
#undef fread
#endif
}

const byte *DSKSave(const char *Name,const byte *Dsk)
{
  const char *T;
  byte *Path,*P;
  struct stat FS;
  FILE *F;
  int J,I,K;

  /* If <Name> is a directory... */
  if(!stat(Name,&FS)&&S_ISDIR(FS.st_mode))
  {
    /* Compose path name */
    Path=malloc(strlen(Name)+20);
    if(!Path) return(0);
    strcpy(Path,Name);
    I=strlen(Path);
    if(Path[I-1]!='/') Path[I++]='/';

    /* Scan, read, dump files */
    for(J=1;J<=DSK_DIR_SIZE;J++)
      if(DSKFileSize(Dsk,J))
      {
        /* Compose file name */
        T=DSKFileName(Dsk,J);
        P=Path+I;
        for(K=0;(K<8)&&T[K]&&(T[K]!=' ');K++) *P++=T[K];
        if(T[8]!=' ')
          for(*P++='.',K=8;(K<11)&&T[K]&&(T[K]!=' ');K++) *P++=T[K];
        *P='\0';

        /* Read and dump file */
        if(P=malloc(DSKFileSize(Dsk,J)))
        {
          if(K=DSKRead(Dsk,J,P,DSKFileSize(Dsk,J)))
            if(F=fopen(Path,"wb")) { fwrite(P,1,K,F);fclose(F); }      
          free(P);
        }
      }

    /* Done processing directory */
    return(Dsk);
  }

#ifdef ZLIB
#define fopen(N,M)      (FILE *)gzopen(N,M)
#define fclose(F)       gzclose((gzFile)F)
#define fwrite(B,L,N,F) gzwrite((gzFile)F,(byte *)B,(L)*(N))
#endif

  /* Assume <Name> to be a disk image file */
  F=fopen(Name,"wb");
  if(!F) return(0);

  /* Write data */
  if(fwrite(Dsk,1,DSK_DISK_SIZE,F)!=DSK_DISK_SIZE)         
  { fclose(F);return(0); }

  /* Done */
  fclose(F);
  return(Dsk);

#ifdef ZLIB
#undef fopen
#undef fclose
#undef fwrite
#endif
}
