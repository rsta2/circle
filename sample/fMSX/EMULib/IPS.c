#include "IPS.h"

#include <stdio.h>
#include <string.h>

#if defined(ANDROID)
#include "MemFS.h"
#define fopen           mopen
#define fclose          mclose
#define fread           mread
#define fseek           mseek
#elif defined(ZLIB)
#include <zlib.h>
#define fopen(F,M)      (FILE *)gzopen(F,M)
#define fclose(F)       gzclose((gzFile)(F))
#define fread(B,N,L,F)  gzread((gzFile)(F),B,(L)*(N))
#define fseek(F,N,W)    (gzseek((gzFile)(F),N,W)<0? -1:0)
#endif

/** MeasureIPS() *********************************************/
/** Find total data size assumed by a given .IPS file.      **/
/*************************************************************/
unsigned int MeasureIPS(const char *FileName)
{
  return(ApplyIPS(FileName,0,0));
}

/** ApplyIPS() ***********************************************/
/** Loads patches from an .IPS file and applies them to the **/
/** given data buffer. Returns number of patches applied.   **/
/*************************************************************/
unsigned int ApplyIPS(const char *FileName,unsigned char *Data,unsigned int Size)
{
  unsigned char Buf[16];
  unsigned int Result,Count,J,N;
  FILE *F;

  F = fopen(FileName,"rb");
  if(!F) return(0);

  /* Verify file header */
  if(fread(Buf,1,5,F)!=5)   { fclose(F);return(0); }
  if(memcmp(Buf,"PATCH",5)) { fclose(F);return(0); }

  for(Result=0,Count=1;fread(Buf,1,5,F)==5;++Count)
  {
    J = Buf[2]+((unsigned int)Buf[1]<<8)+((unsigned int)Buf[0]<<16);
    N = Buf[4]+((unsigned int)Buf[3]<<8);

    /* Apparently, these may signal the end of .IPS file */
    if((J==0xFFFFFF) || !memcmp(Buf,"EOF",3)) break;

    /* If patching with a block of data... */
    if(N)
    {
      /* Copying data */

      if(!Data)
      {
        /* Just measuring patch size */
        if(Result<J+N) Result=J+N;
        if(fseek(F,N,SEEK_CUR)<0) break;
      }
      else if(J+N>Size)
      {
        printf("IPS: Failed applying COPY patch #%d to 0x%X..0x%X of 0x%X bytes.\n",Count,J,J+N-1,Size);
        if(fseek(F,N,SEEK_CUR)<0) break;
      }
      else if(fread(Data+J,1,N,F)==N)
      {
//        printf("IPS: Applied COPY patch #%d to 0x%X..0x%X.\n",Count,J,J+N-1);
        ++Result;
      }
      else
      {
        printf("IPS: Failed reading COPY patch #%d from the file.\n",Count);
        break;
      }
    }
    else
    {
      /* Filling with a value */

      if(fread(Buf,1,3,F)!=3)
      {
        if(Data) printf("IPS: Failed reading FILL patch #%d from the file.\n",Count);
        break;
      }

      /* Compute fill length */
      N = ((unsigned int)Buf[0]<<8)+Buf[1];

      if(!Data)
      {
        if(Result<J+N) Result=J+N;
      }
      else if(!N || (J+N>Size))
      {
        printf("IPS: Failed applying FILL patch #%d (0x%02X) to 0x%X..0x%X of 0x%X bytes.\n",Count,Buf[1],J,J+N-1,Size);
      }
      else
      {
//        printf("IPS: Applied FILL patch #%d (0x%02X) to 0x%X..0x%X.\n",Count,Buf[1],J,J+N-1);
        memset(Data+J,Buf[2],N);
        ++Result;
      }
    }
  }

  fclose(F);
  return(Result);
}

#if defined(ZLIB) || defined(ANDROID)
#undef fopen
#undef fclose
#undef fread
#undef fseek
#endif
