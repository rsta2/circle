/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        ImageMux.h                       **/
/**                                                         **/
/** This file wraps Image.c routines for multiple display   **/
/** depths (BPP8,BPP16,BPP32). It is used automatically     **/
/** when none of BPP* values are defined.                   **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2008-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef IMAGEMUX_H
#define IMAGEMUX_H

#include "EMULib.h"

#undef BPP8
#undef BPP16
#undef BPP24
#undef BPP32
#undef RMASK
#undef GMASK
#undef BMASK
#undef PIXEL

#define BPP8
#define pixel           unsigned char
#define PIXEL(R,G,B)    (pixel)(((7*(R)/255)<<5)|((7*(G)/255)<<2)|(3*(B)/255))
#define GetColor        GetColor_8
#define IMGCopy         IMGCopy_8
#define IMGDrawRect     IMGDrawRect_8
#define IMGFillRect     IMGFillRect_8
#define CropImage       CropImage_8
#define ClearImage      ClearImage_8
#define ScaleImage      ScaleImage_8
#define RasterizeImage  RasterizeImage_8
#define TelevizeImage   TelevizeImage_8
#define LcdizeImage     LcdizeImage_8
#define CMYizeImage     CMYizeImage_8
#define RGBizeImage     RGBizeImage_8
#define SoftenImage     SoftenImage_8
#define SoftenEPX       SoftenEPX_8
#define SoftenEAGLE     SoftenEAGLE_8
#define LoadPNG         LoadPNG_8
#include "Image.c"
#undef pixel
#undef PIXEL
#undef GetColor
#undef IMGCopy
#undef IMGDrawRect
#undef IMGFillRect
#undef CropImage
#undef ClearImage
#undef ScaleImage
#undef RasterizeImage
#undef TelevizeImage
#undef LcdizeImage
#undef CMYizeImage
#undef RGBizeImage
#undef SoftenImage
#undef SoftenEPX
#undef SoftenEAGLE
#undef LoadPNG
#undef BPP8

#define BPP16
#define pixel           unsigned short
#if defined(UNIX) || defined(ANDROID) || defined(S60) || defined(UIQ) || defined(NXC2600) || defined(STMP3700)
/* Symbian and Unix use true 16BPP color */
#define PIXEL(R,G,B)    (pixel)(((31*(R)/255)<<11)|((63*(G)/255)<<5)|(31*(B)/255))
#define RMASK           0xF800
#define GMASK           0x07E0
#define BMASK           0x001F
#else
/* Other platforms use 15BPP color */
#define PIXEL(R,G,B)    (pixel)(((31*(R)/255)<<10)|((31*(G)/255)<<5)|(31*(B)/255))
#define RMASK           0x7C00
#define GMASK           0x03E0
#define BMASK           0x001F
#endif
#define GetColor        GetColor_16
#define IMGCopy         IMGCopy_16
#define IMGDrawRect     IMGDrawRect_16
#define IMGFillRect     IMGFillRect_16
#define CropImage       CropImage_16
#define ClearImage      ClearImage_16
#define ScaleImage      ScaleImage_16
#define RasterizeImage  RasterizeImage_16
#define TelevizeImage   TelevizeImage_16
#define LcdizeImage     LcdizeImage_16
#define CMYizeImage     CMYizeImage_16
#define RGBizeImage     RGBizeImage_16
#define SoftenImage     SoftenImage_16
#define SoftenEPX       SoftenEPX_16
#define SoftenEAGLE     SoftenEAGLE_16
#define LoadPNG         LoadPNG_16
#define ARMScaleImage   ARMScaleImage_16
#define Merge2          Merge2_16
#define Merge4          Merge4_16
#undef LIBARM_H
#include "Image.c"
#undef pixel
#undef PIXEL
#undef RMASK
#undef GMASK
#undef BMASK
#undef GetColor
#undef IMGCopy
#undef IMGDrawRect
#undef IMGFillRect
#undef CropImage
#undef ClearImage
#undef ScaleImage
#undef RasterizeImage
#undef TelevizeImage
#undef LcdizeImage
#undef CMYizeImage
#undef RGBizeImage
#undef SoftenImage
#undef SoftenEPX
#undef SoftenEAGLE
#undef LoadPNG
#undef ARMScaleImage
#undef Merge2
#undef Merge4
#undef BPP16

#define BPP32
#define pixel           unsigned int
#if defined(ANDROID)
#define PIXEL(R,G,B)    (pixel)(((int)R<<16)|((int)G<<8)|B|0xFF000000)
#else
#define PIXEL(R,G,B)    (pixel)(((int)R<<16)|((int)G<<8)|B)
#endif
#define RMASK           0xFF0000
#define GMASK           0x00FF00
#define BMASK           0x0000FF
#define GetColor        GetColor_32
#define IMGCopy         IMGCopy_32
#define IMGDrawRect     IMGDrawRect_32
#define IMGFillRect     IMGFillRect_32
#define CropImage       CropImage_32
#define ClearImage      ClearImage_32
#define ScaleImage      ScaleImage_32
#define RasterizeImage  RasterizeImage_32
#define TelevizeImage   TelevizeImage_32
#define LcdizeImage     LcdizeImage_32
#define CMYizeImage     CMYizeImage_32
#define RGBizeImage     RGBizeImage_32
#define SoftenImage     SoftenImage_32
#define SoftenEPX       SoftenEPX_32
#define SoftenEAGLE     SoftenEAGLE_32
#define LoadPNG         LoadPNG_32
#define ARMScaleImage   ARMScaleImage_32
#define Merge2          Merge2_32
#define Merge4          Merge4_32
#undef LIBARM_H
#include "Image.c"
#undef pixel
#undef PIXEL
#undef RMASK
#undef GMASK
#undef BMASK
#undef GetColor
#undef IMGCopy
#undef IMGDrawRect
#undef IMGFillRect
#undef CropImage
#undef ClearImage
#undef ScaleImage
#undef RasterizeImage
#undef TelevizeImage
#undef LcdizeImage
#undef CMYizeImage
#undef RGBizeImage
#undef SoftenImage
#undef SoftenEPX
#undef SoftenEAGLE
#undef LoadPNG
#undef ARMScaleImage
#undef Merge2
#undef Merge4
#undef BPP32

/** GetColor() ***********************************************/
/** Return pixel corresponding to the given <R,G,B> value.  **/
/** This only works for non-palletized modes.               **/
/*************************************************************/
pixel GetColor(unsigned char R,unsigned char G,unsigned char B)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  return(GetColor_8(R,G,B));
      case 16: return(GetColor_16(R,G,B));
      case 24:
      case 32: return(GetColor_32(R,G,B));
    }

  return(0);
}

/** ClearImage() *********************************************/
/** Clear image with a given color.                         **/
/*************************************************************/
void ClearImage(Image *Img,pixel Color)
{
  switch(Img->D)
  {
    case 8:  ClearImage_8(Img,Color);break;
    case 16: ClearImage_16(Img,Color);break;
    case 24:
    case 32: ClearImage_32(Img,Color);break;
  }
}

/** IMGCopy() ************************************************/
/** Copy one image into another.                            **/
/*************************************************************/
void IMGCopy(Image *Dst,int DX,int DY,const Image *Src,int SX,int SY,int W,int H,int TColor)
{
  if(Src->D!=Dst->D) return;

  switch(Dst->D)
  {
    case 8:  IMGCopy_8(Dst,DX,DY,Src,SX,SY,W,H,TColor);break;
    case 16: IMGCopy_16(Dst,DX,DY,Src,SX,SY,W,H,TColor);break;
    case 24:
    case 32: IMGCopy_32(Dst,DX,DY,Src,SX,SY,W,H,TColor);break;
  }
}

/** IMGDrawRect()/IMGFillRect() ******************************/
/** Draw filled/unfilled rectangle in a given image.        **/
/*************************************************************/
void IMGDrawRect(Image *Img,int X,int Y,int W,int H,pixel Color)
{
  switch(Img->D)
  {
    case 8:  IMGDrawRect_8(Img,X,Y,W,H,Color);break;
    case 16: IMGDrawRect_16(Img,X,Y,W,H,Color);break;
    case 24:
    case 32: IMGDrawRect_32(Img,X,Y,W,H,Color);break;
  }
}

void IMGFillRect(Image *Img,int X,int Y,int W,int H,pixel Color)
{
  switch(Img->D)
  {
    case 8:  IMGFillRect_8(Img,X,Y,W,H,Color);break;
    case 16: IMGFillRect_16(Img,X,Y,W,H,Color);break;
    case 24:
    case 32: IMGFillRect_32(Img,X,Y,W,H,Color);break;
  }
}

/** SoftenImage() ********************************************/
/** Uses softening algorithm to interpolate image Src into  **/
/** a bigger image Dst.                                     **/
/*************************************************************/
void SoftenImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  if(Src->D!=Dst->D) return;

  switch(Dst->D)
  {
    case 8:  SoftenImage_8(Dst,Src,X,Y,W,H);break;
    case 16: SoftenImage_16(Dst,Src,X,Y,W,H);break;
    case 24:
    case 32: SoftenImage_32(Dst,Src,X,Y,W,H);break;
  }
}

/** SoftenEPX() **********************************************/
/** Uses softening algorithm to interpolate image Src into  **/
/** a bigger image Dst.                                     **/
/*************************************************************/
void SoftenEPX(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  if(Src->D!=Dst->D) return;

  switch(Dst->D)
  {
    case 8:  SoftenEPX_8(Dst,Src,X,Y,W,H);break;
    case 16: SoftenEPX_16(Dst,Src,X,Y,W,H);break;
    case 24:
    case 32: SoftenEPX_32(Dst,Src,X,Y,W,H);break;
  }
}

/** SoftenEAGLE() ********************************************/
/** Uses softening algorithm to interpolate image Src into  **/
/** a bigger image Dst.                                     **/
/*************************************************************/
void SoftenEAGLE(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  if(Src->D!=Dst->D) return;

  switch(Dst->D)
  {
    case 8:  SoftenEAGLE_8(Dst,Src,X,Y,W,H);break;
    case 16: SoftenEAGLE_16(Dst,Src,X,Y,W,H);break;
    case 24:
    case 32: SoftenEAGLE_32(Dst,Src,X,Y,W,H);break;
  }
}

/** ScaleImage() *********************************************/
/** Copy Src into Dst, scaling as needed                    **/
/*************************************************************/
void ScaleImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  if(Src->D!=Dst->D) return;

  switch(Dst->D)
  {
    case 8:  ScaleImage_8(Dst,Src,X,Y,W,H);break;
    case 16: ScaleImage_16(Dst,Src,X,Y,W,H);break;
    case 24:
    case 32: ScaleImage_32(Dst,Src,X,Y,W,H);break;
  }
}

/** TelevizeImage() ******************************************/
/** Create televizion effect on the image.                  **/
/*************************************************************/
void TelevizeImage(Image *Img,int X,int Y,int W,int H)
{
  switch(Img->D)
  {
    case 8:  TelevizeImage_8(Img,X,Y,W,H);break;
    case 16: TelevizeImage_16(Img,X,Y,W,H);break;
    case 24:
    case 32: TelevizeImage_32(Img,X,Y,W,H);break;
  }
}

/** LcdizeImage() ********************************************/
/** Create LCD effect on the image.                         **/
/*************************************************************/
void LcdizeImage(Image *Img,int X,int Y,int W,int H)
{
  switch(Img->D)
  {
    case 8:  LcdizeImage_8(Img,X,Y,W,H);break;
    case 16: LcdizeImage_16(Img,X,Y,W,H);break;
    case 24:
    case 32: LcdizeImage_32(Img,X,Y,W,H);break;
  }
}

/** RasterizeImage() *****************************************/
/** Create raster effect on the image.                      **/
/*************************************************************/
void RasterizeImage(Image *Img,int X,int Y,int W,int H)
{
  switch(Img->D)
  {
    case 8:  RasterizeImage_8(Img,X,Y,W,H);break;
    case 16: RasterizeImage_16(Img,X,Y,W,H);break;
    case 24:
    case 32: RasterizeImage_32(Img,X,Y,W,H);break;
  }
}

/** CMYizeImage() ********************************************/
/** Apply vertical CMY stripes to the image.                **/
/*************************************************************/
void CMYizeImage(Image *Img,int X,int Y,int W,int H)
{
  switch(Img->D)
  {
    case 8:  CMYizeImage_8(Img,X,Y,W,H);break;
    case 16: CMYizeImage_16(Img,X,Y,W,H);break;
    case 24:
    case 32: CMYizeImage_32(Img,X,Y,W,H);break;
  }
}

/** RGBizeImage() ********************************************/
/** Apply vertical RGB stripes to the image.                **/
/*************************************************************/
void RGBizeImage(Image *Img,int X,int Y,int W,int H)
{
  switch(Img->D)
  {
    case 8:  RGBizeImage_8(Img,X,Y,W,H);break;
    case 16: RGBizeImage_16(Img,X,Y,W,H);break;
    case 24:
    case 32: RGBizeImage_32(Img,X,Y,W,H);break;
  }
}

#ifdef ARM_CPU
/** ARMScaleImage() ******************************************/
/** Copy Src into Dst using ARM-optimized assembly code.    **/
/** Returns 0 if this function is not supported or there is **/
/** an alignment problem. Returns destination height and    **/
/** width on success in <Height 31:16><Width 15:0> format.  **/
/*************************************************************/
unsigned int ARMScaleImage(Image *Dst,Image *Src,int X,int Y,int W,int H,int Attrs)
{
  if(Src->D!=Dst->D) return(0);

  switch(Dst->D)
  {
    case 16: return(ARMScaleImage_16(Dst,Src,X,Y,W,H,Attrs));
    case 24:
    case 32: return(ARMScaleImage_32(Dst,Src,X,Y,W,H,Attrs));
  }

  /* ARMScaleImage() does not support this image depth */
  ScaleImage(Dst,Src,X,Y,W,H);
  return((Dst->H<<16)|Dst->W);
}
#endif /* ARM_CPU */

#endif /* IMAGEMUX_H */
