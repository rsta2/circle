/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        EMULib.c                         **/
/**                                                         **/
/** This file contains platform-independent implementation  **/
/** part of the emulation library.                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"
#include "Console.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#if defined(WINDOWS) || defined(UNIX) || defined(MAEMO) || defined(MEEGO) || defined(ANDROID)
#define NewImage GenericNewImage
#endif

#if defined(WINDOWS) || defined(UNIX) || defined(MAEMO) || defined(MEEGO)
#define FreeImage GenericFreeImage
#define CropImage GenericCropImage
extern Image BigScreen;
#endif

#if defined(UNIX) || defined(MAEMO) || defined(MEEGO) || defined(ANDROID)
#define SetVideo GenericSetVideo
#endif

/** Current Video Image **************************************/
/** These parameters are set with SetVideo() and used by    **/
/** ShowVideo() to show a WxH fragment from <X,Y> of Img.   **/
/*************************************************************/
Image *VideoImg = 0;           /* Current ShowVideo() image  */
int VideoX;                    /* X for ShowVideo()          */
int VideoY;                    /* Y for ShowVideo()          */
int VideoW;                    /* Width for ShowVideo()      */
int VideoH;                    /* Height for ShowVideo()     */

/** KeyHandler ***********************************************/
/** This function receives key presses and releases.        **/
/*************************************************************/
void (*KeyHandler)(unsigned int Key) = 0;

/** NewImage() ***********************************************/
/** Create a new image of the given size. Returns pointer   **/
/** to the image data on success, 0 on failure.             **/
/*************************************************************/
pixel *NewImage(Image *Img,int Width,int Height)
{
  Img->Data    = (pixel *)malloc(Width*Height*sizeof(pixel));
  Img->Cropped = 0;

  if(!Img->Data) Img->W=Img->H=Img->L=Img->D=0;
  else
  {
    memset(Img->Data,0,Width*Height*sizeof(pixel));
    Img->D = sizeof(pixel)<<3;
    Img->W = Width;
    Img->H = Height;
    Img->L = Width;
  }

  return(Img->Data);
}

/** FreeImage() **********************************************/
/** Free previously allocated image.                        **/
/*************************************************************/
void FreeImage(Image *Img)
{
  /* If image is used for video, unselect it */
  if(VideoImg==Img) VideoImg=0;
  /* Image gone */
  if(Img->Data&&!Img->Cropped) free(Img->Data);
  Img->Data    = 0;
  Img->Cropped = 0;
  Img->W       = 0;
  Img->H       = 0;
  Img->L       = 0;
  Img->D       = 0;
}

/** CropImage() **********************************************/
/** Create a subimage Dst of the image Src. Returns Dst.    **/
/*************************************************************/
Image *CropImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  Dst->Data    = (void *)((char *)Src->Data+(Src->L*Y+X)*(Src->D>>3));
  Dst->Cropped = 1;
  Dst->W       = W;
  Dst->H       = H;
  Dst->L       = Src->L;
  Dst->D       = Src->D;
  return(Dst);
}

/** SetVideo() ***********************************************/
/** Set part of the image as "active" for display.          **/
/*************************************************************/
void SetVideo(Image *Img,int X,int Y,int W,int H)
{
  VideoImg = Img;
  VideoX   = X<0? 0:X>=Img->W? Img->W-1:X;
  VideoY   = Y<0? 0:Y>=Img->H? Img->H-1:Y;
  VideoW   = VideoX+W>Img->W? Img->W-VideoX:W;
  VideoH   = VideoY+H>Img->H? Img->H-VideoY:H;
#ifdef WINDOWS
  FreeImage(&BigScreen);
#endif
}

/** WaitJoystick() *******************************************/
/** Wait until one or more of the given buttons have been   **/
/** pressed. Returns the bitmask of pressed buttons. Refer  **/
/** to BTN_* #defines for the button mappings.              **/
/*************************************************************/
unsigned int WaitJoystick(unsigned int Mask)
{
  unsigned int I;

#if defined(ANDROID)
  /* Wait for all requested buttons to be released first */
  while(GetJoystick()&Mask) { ShowVideo();usleep(100000); }
  /* Wait for any of the buttons to become pressed */
  do { I=GetJoystick()&Mask;ShowVideo();usleep(100000); } while(!I);
#elif defined(UNIX) || defined(MAEMO) || defined(NXC2600) || defined(STMP3700) || defined(ANDROID)
  /* Wait for all requested buttons to be released first */
  while(GetJoystick()&Mask) usleep(100000);
  /* Wait for any of the buttons to become pressed */
  do { I=GetJoystick()&Mask;usleep(100000); } while(!I);
#else
  /* Wait for all requested buttons to be released first */
  while(GetJoystick()&Mask);
  /* Wait for any of the buttons to become pressed */
  do I=GetJoystick()&Mask; while(!I);
#endif

  /* Return pressed buttons */
  return(I);
}

/** SetKeyHandler() ******************************************/
/** Attach keyboard handler that will be called when a key  **/
/** is pressed or released.                                 **/
/*************************************************************/
void SetKeyHandler(void (*Handler)(unsigned int Key))
{
  KeyHandler=Handler;
}

/** GetFilePath() ********************************************/
/** Extracts pathname from filename and returns a pointer   **/
/** to the internal buffer containing just the path name    **/
/** ending with "\".                                        **/
/*************************************************************/
const char *GetFilePath(const char *Name)
{
  static char Path[256];
  const char *P;
  char *T;

  P=strrchr(Name,'\\');

  /* If path not found or too long, assume current */
  if(!P||(P-Name>200)) { strcpy(Path,"");return(Path); }

  /* Copy and return the pathname */
  for(T=Path;Name<P;*T++=*Name++);
  *T='\0';return(Path);
}

/** NewFile() ************************************************/
/** Given pattern NAME.EXT, generates a new filename in the **/
/** NAMEnnnn.EXT (nnnn = 0000..9999) format and returns a   **/
/** pointer to the internal buffer containing new filename. **/
/*************************************************************/
const char *NewFile(const char *Pattern)
{
  static char Name[256];
  struct stat FInfo;
  const char *P;
  char S[256],*T;
  int J;

  /* If too long name, fall out */
  if(strlen(Pattern)>200) { strcpy(Name,"");return(Name); }

  /* Make up the format string */
  for(T=S,P=Pattern;*P&&(*P!='.');) *T++=*P++;
  *T='\0';
  strcat(S,"%04d");
  strcat(S,P);

  /* Scan through the filenames */
  for(J=0;J<10000;J++)
  {
    sprintf(Name,S,J);
    if(stat(Name,&FInfo)) break;
  }

  if(J==10000) strcpy(Name,"");
  return(Name);
}
