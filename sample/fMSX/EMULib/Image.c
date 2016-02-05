/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          Image.c                        **/
/**                                                         **/
/** This file contains non-essential functions that operate **/
/** on images.                                              **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#if !defined(BPP32) && !defined(BPP24) && !defined(BPP16) && !defined(BPP8)
#include "ImageMux.h"
#else

#include "EMULib.h"
#include <string.h>

#if defined(ARM_CPU)
#include "LibARM.h"
#endif

#ifdef LIBPNG
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef ANDROID
#define puts   LOGI
#define printf LOGI
#endif

#if defined(BPP16) || defined(BPP24) || defined(BPP32)
/** Merge2() *************************************************/
/** Merges pixels A and B with bias X.                      **/
/*************************************************************/
static __inline pixel Merge2(pixel A,pixel B,unsigned int X)
{
#if defined(ARM_CPU)
  return(MERGE2(A,B,X));
#else
  unsigned int DA,DB;

  // Do not interpolate if values are the same
  if(A==B) return A;

  // Reduce 16 bit fraction to 5 bits
  DB = (X>>11)&0x001F;
  DA = 0x20 - DB;

#if defined(BPP24) || defined(BPP32)
  X  = (DA*(A&GMASK)+DB*(B&GMASK))>>5;
  DA*= A&(RMASK|BMASK);
  DB*= B&(RMASK|BMASK);
  DA = (DA+DB)>>5;
  return((DA&(RMASK|BMASK))|(X&GMASK));
#else
  DA*= (A&(RMASK|BMASK)) | ((A&GMASK)<<16);
  DB*= (B&(RMASK|BMASK)) | ((B&GMASK)<<16);
  DA = (DA+DB)>>5;
  return((DA&(RMASK|BMASK))|((DA>>16)&GMASK));
#endif
#endif /* ARM_CPU */
}

/** Merge4() *************************************************/
/** Merges pixels A,B,C,D with biases X,Y.                  **/
/*************************************************************/
static __inline pixel Merge4(pixel A,pixel B,pixel C,pixel D,unsigned int X,unsigned int Y)
{
#if defined(ARM_CPU)
  return(MERGE4(A,B,C,D,X,Y));
#else
  unsigned int DA,DB,DC,DD;

  X  = (X>>11)&0x001F;
  Y  = (Y>>11)&0x001F;
  DD = (X*Y)>>5;
  DA = 0x20 + DD - X - Y;
  DB = X - DD;
  DC = Y - DD;

#if defined(BPP24) || defined(BPP32)
  X  = (DA*(A&GMASK)+DB*(B&GMASK)+DC*(C&GMASK)+DD*(D&GMASK))>>5;
  DA*= A&(RMASK|BMASK);
  DB*= B&(RMASK|BMASK);
  DC*= C&(RMASK|BMASK);
  DD*= D&(RMASK|BMASK);
  DD = (DA+DB+DC+DD)>>5;
  return((DD&(RMASK|BMASK))|(X&GMASK));
#else
  DA*= (A&(RMASK|BMASK)) | ((A&GMASK)<<16);
  DB*= (B&(RMASK|BMASK)) | ((B&GMASK)<<16);
  DC*= (C&(RMASK|BMASK)) | ((C&GMASK)<<16);
  DD*= (D&(RMASK|BMASK)) | ((D&GMASK)<<16);
  DD = (DA+DB+DC+DD)>>5;
  return((DD&(RMASK|BMASK))|((DD>>16)&GMASK));
#endif
#endif /* ARM_CPU */
}
#endif /* BPP16||BPP24||BPP32 */

/** GetColor() ***********************************************/
/** Return pixel corresponding to the given <R,G,B> value.  **/
/** This only works for non-palletized modes.               **/
/*************************************************************/
pixel GetColor(unsigned char R,unsigned char G,unsigned char B)
{
  return(PIXEL(R,G,B));
}

/** ClearImage() *********************************************/
/** Clear image with a given color.                         **/
/*************************************************************/
void ClearImage(Image *Img,pixel Color)
{
  pixel *P;
  int X,Y;

  for(P=(pixel *)Img->Data,Y=Img->H;Y;--Y,P+=Img->L)
    for(X=0;X<Img->W;++X) P[X]=Color;
}

/** IMGCopy() ************************************************/
/** Copy one image into another.                            **/
/*************************************************************/
void IMGCopy(Image *Dst,int DX,int DY,const Image *Src,int SX,int SY,int W,int H,int TColor)
{
  const pixel *S;
  pixel *D;
  int X;

  if(DX<0) { W+=DX;SX-=DX;DX=0; }
  if(DY<0) { H+=DY;SY-=DY;DY=0; }
  if(SX<0) { W+=SX;DX-=SX;SX=0; } else if(SX+W>Src->W) W=Src->W-SX;
  if(SY<0) { H+=SY;DY-=SY;SY=0; } else if(SY+H>Src->H) H=Src->H-SY;
  if(DX+W>Dst->W) W=Dst->W-DX;
  if(DY+H>Dst->H) H=Dst->H-DY;
  if((W<=0)||(H<=0)) return;

  S = (pixel *)Src->Data+Src->L*SY+SX;
  D = (pixel *)Dst->Data+Dst->L*DY+DX;

#if defined(ARM_CPU) && (defined(BPP16) || defined(BPP24) || defined(BPP32))
  /* ARM optimizations require W aligned to 16 pixels, */
  /* coordinates and strides aligned to 2 pixels...    */
  if(!(W&15)&&!(SX&1)&&!(DX&1)&&!(Src->L&1)&&!(Dst->L&1)&&(TColor<0))
  {
    for(;H;--H,S+=Src->L,D+=Dst->L) C256T256(D,S,W);
    return;
  }
#endif /* ARM_CPU && (BPP16 || BPP24 || BPP32) */

  if(TColor<0)
    for(;H;--H,S+=Src->L,D+=Dst->L)
      memcpy(D,S,W*sizeof(pixel));
  else
    for(;H;--H,S+=Src->L,D+=Dst->L)
      for(X=0;X<W;++X)
        if(S[X]!=TColor) D[X]=S[X];
}

/** IMGDrawRect()/IMGFillRect() ******************************/
/** Draw filled/unfilled rectangle in a given image.        **/
/*************************************************************/
void IMGDrawRect(Image *Img,int X,int Y,int W,int H,pixel Color)
{
  pixel *P;
  int J;

  if(X<0) { W+=X;X=0; } else if(X+W>Img->W) W=Img->W-X;
  if(Y<0) { H+=Y;Y=0; } else if(Y+H>Img->H) H=Img->H-Y;

  if((W>0)&&(H>0))
  {
    for(P=(pixel *)Img->Data+Img->L*Y+X,J=0;J<W;++J) P[J]=Color;
    for(H-=2,P+=Img->L;H;--H,P+=Img->L) P[0]=P[W-1]=Color;
    for(J=0;J<W;++J) P[J]=Color;
  }
}

void IMGFillRect(Image *Img,int X,int Y,int W,int H,pixel Color)
{
  pixel *P;

  if(X<0) { W+=X;X=0; } else if(X+W>Img->W) W=Img->W-X;
  if(Y<0) { H+=Y;Y=0; } else if(Y+H>Img->H) H=Img->H-Y;

  if((W>0)&&(H>0))
    for(P=(pixel *)Img->Data+Img->L*Y+X;H;--H,P+=Img->L)
      for(X=0;X<W;++X) P[X]=Color;
}

/** IMGPrint() ***********************************************/
/** Print text in a given image.                            **/
/*************************************************************/
/*** @@@ NOT YET
void IMGPrint(Image *Img,const char *S,int X,int Y,pixel FG,pixel BG)
{
  const unsigned char *C;
  pixel *P;
  int I,J,K;

  X = X<0? 0:X>Img->W-8? Img->W-8:X;
  Y = Y<0? 0:Y>Img->H-8? Img->H-8:Y;

  for(K=X;*S;S++)
    switch(*S)
    {
      case '\n':
        K=X;Y+=8;
        if(Y>Img->H-8) Y=0;
        break;
      default:
        P=(pixel *)Img->Data+Img->L*Y+K;
        for(C=CurFont+(*S<<3),J=8;J;P+=Img->L,++C,--J)
          for(I=0;I<8;++I) P[I]=*C&(0x80>>I)? FG:BG;
        K+=8;
        if(X>Img->W-8)
        {
          K=0;Y+=8;
          if(Y>Img->H-8) Y=0;
        }

        break;
    }
}
***/

/** ScaleImage() *********************************************/
/** Copy Src into Dst, scaling as needed                    **/
/*************************************************************/
void ScaleImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  register pixel *DP,*SP,*S;
  register unsigned int DX,DY;

  /* If not scaling image, use plain copy */
  if((Dst->W==W)&&(Dst->H==H)) { IMGCopy(Dst,0,0,Src,X,Y,W,H,-1);return; }

  /* Check arguments, compute start pointer */
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X  = X<0? 0:X>Src->W? Src->W:X;
  Y  = Y<0? 0:Y>Src->H? Src->H:Y;
  W  = X+W>Src->W? Src->W-X:W;
  H  = Y+H>Src->H? Src->H-Y:H;
  if(!W||!H) return;
  SP = (pixel *)Src->Data+Y*Src->L+X;
  DP = (pixel *)Dst->Data;
  W  = W<<16;
  H  = H<<16;
  DX = (W+Dst->W-1)/Dst->W;
  DY = (H+Dst->H-1)/Dst->H;

  for(Y=0;Y<H;Y+=DY)
  {
    S=SP+(Y>>16)*Src->L;
    for(X=0;X<W;X+=DX) *DP++=S[X>>16];
    DP+=Dst->L-Dst->W;
  }
}

/** TelevizeImage() ******************************************/
/** Create televizion effect on the image.                  **/
/*************************************************************/
void TelevizeImage(Image *Img,int X,int Y,int W,int H)
{
#if defined(BPP32) || defined(BPP24) || defined(BPP16)
  register pixel *P;

  /* Check arguments, compute start pointer */
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X = X<0? 0:X>Img->W? Img->W:X;
  Y = Y<0? 0:Y>Img->H? Img->H:Y;
  W = X+W>Img->W? Img->W-X:W;
  H = Y+H>Img->H? Img->H-Y:H;
  if(!W||!H) return;
  P = (pixel *)Img->Data+Y*Img->L+X;

#if defined(ARM_CPU)
  /* ARM optimizations require W aligned to 16 pixels, */
  /* coordinates and strides aligned to 2 pixels...    */
  if(!(W&15)&&!(X&1)&&!(Img->L&1))
  {
    for(Y=H;Y>0;--Y,P+=Img->L)
      if(Y&1) TELEVIZE1(P,W); else TELEVIZE0(P,W);
    return;
  }
#endif /* ARM_CPU */

#if defined(BPP32) || defined(BPP24)
  for(Y=H;Y>0;--Y,P+=Img->L)
    if(Y&1) for(X=0;X<W;++X) P[X]-=(P[X]>>4)&0x000F0F0F;
    else    for(X=0;X<W;++X) P[X]+=(~P[X]>>4)&0x000F0F0F;
#elif defined(UNIX) || defined(ANDROID) || defined(ARM_CPU) || defined(NXC2600)
  /* Unix/X11, ARM, and MIPS platforms all use 16bit color */
  for(Y=H;Y>0;--Y,P+=Img->L)
    if(Y&1) for(X=0;X<W;++X) P[X]-=(P[X]>>3)&0x18E3;
    else    for(X=0;X<W;++X) P[X]+=(~P[X]>>3)&0x18E3;
#else
  /* Other platforms use 15bit color */
  for(Y=H;Y>0;--Y,P+=Img->L)
    if(Y&1) for(X=0;X<W;++X) P[X]-=(P[X]>>3)&0x0C63;//0x0421;//0x0C63;
    else    for(X=0;X<W;++X) P[X]+=(~P[X]>>3)&0x0C63;//0x0421;//0x0C63;
#endif /* !UNIX && !ANDROID && !ARM_CPU && !NXC2600 */
#endif /* BPP32 || BPP24 || BPP16 */
}

/** LcdizeImage() ********************************************/
/** Create LCD effect on the image.                         **/
/*************************************************************/
void LcdizeImage(Image *Img,int X,int Y,int W,int H)
{
#if defined(BPP32) || defined(BPP24) || defined(BPP16)
  register pixel *P;

  /* Check arguments, compute start pointer */
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X = X<0? 0:X>Img->W? Img->W:X;
  Y = Y<0? 0:Y>Img->H? Img->H:Y;
  W = X+W>Img->W? Img->W-X:W;
  H = Y+H>Img->H? Img->H-Y:H;
  if(!W||!H) return;
  P = (pixel *)Img->Data+Y*Img->L+X;

#if defined(ARM_CPU)
  /* ARM optimizations require W aligned to 16 pixels, */
  /* coordinates and strides aligned to 2 pixels...    */
  if(!(W&15)&&!(X&1)&&!(Img->L&1))
  {
    for(Y=H;Y>0;--Y,P+=Img->L) LCDIZE(P,W);
    return;
  }
#endif /* ARM_CPU */

#if defined(BPP32) || defined(BPP24)
  for(Y=H,W&=~1;Y>0;--Y,P+=Img->L)
    for(X=0;X<W;++X)
    { P[X]-=(P[X]>>4)&0x000F0F0F;++X;P[X]+=(~P[X]>>4)&0x000F0F0F; }
#elif defined(UNIX) || defined(ANDROID) || defined(ARM_CPU) || defined(NXC2600)
  /* Unix/X11, ARM, and MIPS platforms all use 16bit color */
  for(Y=H,W&=~1;Y>0;--Y,P+=Img->L)
    for(X=0;X<W;++X)
    { P[X]-=(P[X]>>3)&0x18E3;++X;P[X]+=(~P[X]>>3)&0x18E3; }
#else
  /* Other platforms use 15bit color */
  for(Y=H,W&=~1;Y>0;--Y,P+=Img->L)
    for(X=0;X<W;++X)
    { P[X]-=(P[X]>>3)&0x0C63;++X;P[X]+=(~P[X]>>3)&0x0C63; }
#endif /* !UNIX && !ANDROID && !ARM_CPU && !NXC2600 */
#endif /* BPP32 || BPP24 || BPP16 */
}

/** RasterizeImage() *****************************************/
/** Create raster effect on the image.                      **/
/*************************************************************/
void RasterizeImage(Image *Img,int X,int Y,int W,int H)
{
#if defined(BPP32) || defined(BPP24) || defined(BPP16)
  register pixel *P;

  /* Check arguments, compute start pointer */
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X = X<0? 0:X>Img->W? Img->W:X;
  Y = Y<0? 0:Y>Img->H? Img->H:Y;
  W = X+W>Img->W? Img->W-X:W;
  H = Y+H>Img->H? Img->H-Y:H;
  if(!W||!H) return;
  P = (pixel *)Img->Data+Y*Img->L+X;

#if defined(ARM_CPU)
  /* ARM optimizations require W aligned to 16 pixels, */
  /* coordinates and strides aligned to 2 pixels...    */
  if(!(W&15)&&!(X&1)&&!(Img->L&1))
  {
    for(Y=H;Y>0;--Y,P+=Img->L)
      if(Y&1) TELEVIZE0(P,W); else LCDIZE(P,W);
    return;
  }
#endif /* ARM_CPU */

#if defined(BPP32) || defined(BPP24)
  for(Y=H,W&=~1;Y>0;--Y,P+=Img->L)
    if(Y&1) for(X=0;X<W;++X) P[X]-=(P[X]>>4)&0x000F0F0F;
    else    for(X=0;X<W;++X) { P[X]-=(P[X]>>4)&0x000F0F0F;++X;P[X]+=(~P[X]>>4)&0x000F0F0F; }
#elif defined(UNIX) || defined(ANDROID) || defined(ARM_CPU) || defined(NXC2600)
  /* Unix/X11, ARM, and MIPS platforms all use 16bit color */
  for(Y=H,W&=~1;Y>0;--Y,P+=Img->L)
    if(Y&1) for(X=0;X<W;++X) P[X]-=(P[X]>>3)&0x18E3;
    else    for(X=0;X<W;++X) { P[X]-=(P[X]>>3)&0x18E3;++X;P[X]+=(~P[X]>>3)&0x18E3; }
#else
  /* Other platforms use 15bit color */
  for(Y=H,W&=~1;Y>0;--Y,P+=Img->L)
    if(Y&1) for(X=0;X<W;++X) P[X]-=(P[X]>>3)&0x0C63;
    else    for(X=0;X<W;++X) { P[X]-=(P[X]>>3)&0x0C63;++X;P[X]+=(~P[X]>>3)&0x0C63; }
#endif /* !UNIX && !ANDROID && !ARM_CPU && !NXC2600 */
#endif /* BPP32 || BPP24 || BPP16 */
}

/** CMYizeImage() ********************************************/
/** Apply vertical CMY stripes to the image.                **/
/*************************************************************/
void CMYizeImage(Image *Img,int X,int Y,int W,int H)
{
#if defined(BPP32) || defined(BPP24) || defined(BPP16)
  register pixel *P;

  /* Check arguments, compute start pointer */
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X = X<0? 0:X>Img->W? Img->W:X;
  Y = Y<0? 0:Y>Img->H? Img->H:Y;
  W = X+W>Img->W? Img->W-X:W;
  H = Y+H>Img->H? Img->H-Y:H;
  if(!W||!H) return;
  P = (pixel *)Img->Data+Y*Img->L+X;

#if defined(BPP32) || defined(BPP24)
  for(Y=H,X=0;Y>0;--Y,P+=Img->L-W+X)
    for(X=W;X>0;X-=3,P+=3)
    {
      P[0]-=(P[0]>>4)&0x0000000F;
      if(X>1) P[1]-=(P[1]>>4)&0x000F0000;
      if(X>2) P[2]-=(P[2]>>4)&0x00000F00;
    }
#elif defined(UNIX) || defined(ANDROID) || defined(ARM_CPU) || defined(NXC2600)
  /* Unix/X11, ARM, and MIPS platforms all use 16bit color */
  for(Y=H,X=0;Y>0;--Y,P+=Img->L-W+X)
    for(X=W;X>0;X-=3,P+=3)
    {
      P[0]-=(P[0]>>3)&0x0003;
      if(X>1) P[1]-=(P[1]>>3)&0x1800;
      if(X>2) P[2]-=(P[2]>>3)&0x00E0;
    }
#else
  /* Other platforms use 15bit color */
  for(Y=H,X=0;Y>0;--Y,P+=Img->L-W+X)
    for(X=W;X>0;X-=3,P+=3)
    {
      P[0]-=(P[0]>>3)&0x0003;
      if(X>1) P[1]-=(P[1]>>3)&0x0C00;
      if(X>2) P[2]-=(P[2]>>3)&0x0060;
    }
#endif /* !UNIX && !ANDROID && !ARM_CPU && !NXC2600 */
#endif /* BPP32 || BPP24 || BPP16 */
}

/** RGBizeImage() ********************************************/
/** Apply vertical RGB stripes to the image.                **/
/*************************************************************/
void RGBizeImage(Image *Img,int X,int Y,int W,int H)
{
#if defined(BPP32) || defined(BPP24) || defined(BPP16)
  register pixel *P;

  /* Check arguments, compute start pointer */
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X = X<0? 0:X>Img->W? Img->W:X;
  Y = Y<0? 0:Y>Img->H? Img->H:Y;
  W = X+W>Img->W? Img->W-X:W;
  H = Y+H>Img->H? Img->H-Y:H;
  if(!W||!H) return;
  P = (pixel *)Img->Data+Y*Img->L+X;

#if defined(BPP32) || defined(BPP24)
  for(Y=H,X=0;Y>0;--Y,P+=Img->L-W+X)
    for(X=W;X>0;X-=3,P+=3)
    {
      P[0]-=(P[0]>>4)&0x00000F0F;
      if(X>1) P[1]-=(P[1]>>4)&0x000F000F;
      if(X>2) P[2]-=(P[2]>>4)&0x000F0F00;
    }
#elif defined(UNIX) || defined(ANDROID) || defined(ARM_CPU) || defined(NXC2600)
  /* Unix/X11, ARM, and MIPS platforms all use 16bit color */
  for(Y=H,X=0;Y>0;--Y,P+=Img->L-W+X)
    for(X=W;X>0;X-=3,P+=3)
    {
      P[0]-=(P[0]>>3)&0x00E3;
      if(X>1) P[1]-=(P[1]>>3)&0x1803;
      if(X>2) P[2]-=(P[2]>>3)&0x18E0;
    }
#else
  /* Other platforms use 15bit color */
  for(Y=H,X=0;Y>0;--Y,P+=Img->L-W+X)
    for(X=W;X>0;X-=3,P+=3)
    {
      P[0]-=(P[0]>>3)&0x0063;
      if(X>1) P[1]-=(P[1]>>3)&0x0C03;
      if(X>2) P[2]-=(P[2]>>3)&0x0C60;
    }
#endif /* !UNIX && !ANDROID && !ARM_CPU && !NXC2600 */
#endif /* BPP32 || BPP24 || BPP16 */
}

/** SoftenImage() ********************************************/
/** Uses softening algorithm to interpolate image Src into  **/
/** a bigger image Dst.                                     **/
/*************************************************************/
void SoftenImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
#ifdef BPP8
  /* Things look real nasty in BPP8, better just scale */
  ScaleImage(Dst,Src,X,Y,W,H);
#else
  register unsigned int DX,DY;
  register const pixel *S,*SP;
  register pixel *DP,*D;

  /* Check arguments, compute start pointer */
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X  = X<0? 0:X>Src->W? Src->W:X;
  Y  = Y<0? 0:Y>Src->H? Src->H:Y;
  W  = X+W>Src->W-3? Src->W-3-X:W;
  H  = Y+H>Src->H-3? Src->H-3-Y:H;
  // Source image must be at least 4x4 pixels
  if((W<=0)||(H<=0)) return;
  S  = (pixel *)Src->Data+Y*Src->L+Src->L+X+1;

  // Convert to 16:16 fixed point values
  W  = (W-2)<<16;
  DX = (W-0x10001+Dst->W)/Dst->W;
  H  = (H-2)<<16;
  DY = (H-0x10001+Dst->H)/Dst->H;

  for(Y=0x10000,DP=D=(pixel *)Dst->Data;Y<H;Y+=DY,DP=(D+=Dst->L))
  {
    unsigned int A,B,C,D,X1,X2,Y1,Y2,F1,F2;

    // Compute new source pointer
    SP = S+(Y>>16)*Src->L;
    // Get fractional part of Y
    Y1 = Y&0xFFFF;
    Y2 = 0x10000-Y1;

    for(X=0x10000;X<W;SP-=X>>16,X+=DX,*DP++=X2)
    {
      // SP points to [A]:
      //
      //   E F
      // G[A]B I
      // H C D J
      //   K L

      // Get next 2x2 core pixels
      SP+= X>>16;
      A  = SP[0];
      B  = SP[1];
      C  = SP[Src->L];
      D  = SP[Src->L+1];

      // If all 4x4 pixels are the same... 
      if(A==B && C==D && A==C)
      {
        // Do not interpolate
        X2 = A;
      }
      // If diagonal line "\"...
      else if(A==D && B!=C)
      {
        X1 = X&0xFFFF;
        F1 = (X1>>1) + (0x10000>>2);
        F2 = (Y1>>1) + (0x10000>>2);

        if(Y1<=F1 && A==SP[Src->L+2] && A!=SP[-Src->L])
          X2 = Merge2(A,B,F1-Y1);
        else if(Y1>=F1 && A==SP[-1] && A!=SP[(Src->L<<1)+1])
          X2 = Merge2(A,C,Y1-F1);
        else if(X1>=F2 && A==SP[-Src->L] && A!=SP[Src->L+2]) 
          X2 = Merge2(A,B,X1-F2);
        else if(X1<=F2 && A==SP[(Src->L<<1)+1] && A!=SP[-1]) 
          X2 = Merge2(A,C,F2-X1);
        else if(Y1>=X1)
          X2 = Merge2(A,C,Y1-X1);
        else
          X2 = Merge2(A,B,X1-Y1);
      }
      // If diagonal line "/"...
      else if(B==C && A!=D)
      {
        X1 = X&0xFFFF;
        X2 = 0x10000-X1;
        F1 = (X1>>1) + (0x10000>>2);
        F2 = (Y1>>1) + (0x10000>>2);

        if(Y2>=F1 && B==SP[Src->L-1] && B!=SP[-Src->L+1])
          X2 = Merge2(B,A,Y2-F1);
        else if(Y2<=F1 && B==SP[2] && B!=SP[Src->L<<1])
          X2 = Merge2(B,D,F1-Y2);
        else if(X2>=F2 && B==SP[-Src->L+1] && B!=SP[Src->L-1])
          X2 = Merge2(B,A,X2-F2);
        else if(X2<=F2 && B==SP[Src->L<<1] && B!=SP[2])
          X2 = Merge2(B,D,F2-X2);
        else if(Y2>=X1)
          X2 = Merge2(B,A,Y2-X1);
        else
          X2 = Merge2(B,D,X1-Y2);
      }
      // No diagonal lines detected...
      else
      {
        X2 = Merge4(A,B,C,D,X&0xFFFF,Y1);
      }
    }
  }
#endif /* !BPP8 */
}

/** SoftenEPX() **********************************************/
/** Uses EPX softening algorithm to interpolate image Src   **/
/** into a bigger image Dst.                                **/
/*************************************************************/
void SoftenEPX(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
#ifdef BPP8
  /* Things look real nasty in BPP8, better just scale */
  ScaleImage(Dst,Src,X,Y,W,H);
#else
  register unsigned int DX,DY;
  register const pixel *S,*SP;
  register pixel *DP,*D;

  /* Check arguments, compute start pointer */
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X  = X<0? 0:X>Src->W? Src->W:X;
  Y  = Y<0? 0:Y>Src->H? Src->H:Y;
  W  = X+W>Src->W-2? Src->W-2-X:W;
  H  = Y+H>Src->H-2? Src->H-2-Y:H;
  // Source image must be at least 3x3 pixels
  if((W<=0)||(H<=0)) return;
  S  = (pixel *)Src->Data+Y*Src->L+Src->L+X+1;

  // Convert to 16:16 fixed point values
  W  = (W-2)<<16;
  DX = (W-0x10001+Dst->W)/Dst->W;
  H  = (H-2)<<16;
  DY = (H-0x10001+Dst->H)/Dst->H;

  for(Y=0x10000,DP=D=(pixel *)Dst->Data;Y<H;Y+=DY,DP=(D+=Dst->L))
  {
    unsigned int A,B,C,D,P,NW,NE,SW,SE,X1,Y1;

    // Compute new source pointer
    SP = S+(Y>>16)*Src->L;
    Y1 = Y&0xFFFF;

    for(X=0x10000;X<W;SP-=X>>16,X+=DX,*DP++=P)
    {
      // SP points to [P]:
      //
      //   A
      // C[P]B
      //   D

      // Get next 3x3 core pixels
      SP+= X>>16;
      A  = SP[-Src->L];
      B  = SP[1];
      C  = SP[-1];
      D  = SP[Src->L];
      P  = SP[0];

      // Compute 2x2 output pixels
      NW = (C==A) && (C!=D) && (A!=B)? A:P;
      NE = (A==B) && (A!=C) && (B!=D)? B:P;
      SW = (D==C) && (D!=B) && (C!=A)? C:P;
      SE = (B==D) && (B!=A) && (D!=C)? D:P;
      X1 = X&0xFFFF;

      // Merge pixels, with thresholds
      if(X1<=0x4000)
        P = Y1<=0x4000? NW : Y1>=0xC000? SW : Merge2(NW,SW,Y1);
      else if(X1>=0xC000)
        P = Y1<=0x4000? NE : Y1>=0xC000? SE : Merge2(NE,SE,Y1);
      else
        P = Merge4(NW,NE,SW,SE,X1,Y1);
    }
  }
#endif /* !BPP8 */
}

/** SoftenEAGLE() ********************************************/
/** Uses EAGLE softening algorithm to interpolate image Src **/
/** into a bigger image Dst.                                **/
/*************************************************************/
void SoftenEAGLE(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
#ifdef BPP8
  /* Things look real nasty in BPP8, better just scale */
  ScaleImage(Dst,Src,X,Y,W,H);
#else
  register unsigned int DX,DY;
  register const pixel *S,*SP;
  register pixel *DP,*D;

  /* Check arguments, compute start pointer */
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X  = X<0? 0:X>Src->W? Src->W:X;
  Y  = Y<0? 0:Y>Src->H? Src->H:Y;
  W  = X+W>Src->W-2? Src->W-2-X:W;
  H  = Y+H>Src->H-2? Src->H-2-Y:H;
  // Source image must be at least 3x3 pixels
  if((W<=0)||(H<=0)) return;
  S  = (pixel *)Src->Data+Y*Src->L+Src->L+X+1;

  // Convert to 16:16 fixed point values
  W  = (W-2)<<16;
  DX = (W-0x10001+Dst->W)/Dst->W;
  H  = (H-2)<<16;
  DY = (H-0x10001+Dst->H)/Dst->H;

  for(Y=0x10000,DP=D=(pixel *)Dst->Data;Y<H;Y+=DY,DP=(D+=Dst->L))
  {
    unsigned int A,B,C,D,P,NW,NE,SW,SE,X1,Y1;

    // Compute new source pointer
    SP = S+(Y>>16)*Src->L;
    Y1 = Y&0xFFFF;

    for(X=0x10000;X<W;SP-=X>>16,X+=DX,*DP++=P)
    {
      // SP points to [P]:
      //
      //   A
      // C[P]B
      //   D

      // Get next 3x3 core pixels
      SP+= X>>16;
      A  = SP[-Src->L];
      B  = SP[1];
      C  = SP[-1];
      D  = SP[Src->L];
      P  = SP[0];

      // Compute 2x2 output pixels
      NW = (A==C) && (A==SP[-Src->L-1])? A:P;
      NE = (B==A) && (B==SP[-Src->L+1])? B:P;
      SW = (C==D) && (C==SP[Src->L-1])?  C:P;
      SE = (D==B) && (D==SP[Src->L+1])?  D:P;
      X1 = X&0xFFFF;

      // Merge pixels, with thresholds
      if(X1<=0x4000)
        P = Y1<=0x4000? NW : Y1>=0xC000? SW : Merge2(NW,SW,Y1);
      else if(X1>=0xC000)
        P = Y1<=0x4000? NE : Y1>=0xC000? SE : Merge2(NE,SE,Y1);
      else
        P = Merge4(NW,NE,SW,SE,X1,Y1);
    }
  }
#endif /* !BPP8 */
}

#if defined(ARM_CPU) && (defined(BPP16) || defined(BPP24) || defined(BPP32))
unsigned int ARMScaleImage(Image *Dst,Image *Src,int X,int Y,int W,int H,int Attrs)
{
  void (*F)(pixel *D,const pixel *S,int L);
  pixel *S,*D;
  int DW,DY,J,I;

  /* Check arguments */
  if(W<0) { X+=W;W=-W; }
  if(H<0) { Y+=H;H=-H; }
  X  = X<0? 0:X>Src->W? Src->W:X;
  Y  = Y<0? 0:Y>Src->H? Src->H:Y;
  W  = X+W>Src->W? Src->W-X:W;
  H  = Y+H>Src->H? Src->H-Y:H;
  if(!W||!H) return(0);

  /* ARM optimizations require W and X aligned to */
  /* 16 pixels and strides aligned to 2 pixels... */
  if((W&15)||(X&1)||(Src->L&1)||(Dst->L&1))
  {
    ScaleImage(Dst,Src,X,Y,W,H);
    return((Dst->H<<16)|Dst->W);
  }

  /* Determine optimal handler depending on DstW/SrcW ratio */
  F  = 0;
  I  = Dst->W;
  DW = (I<<8)/W;
  DY = Attrs&ARMSI_HFILL;
  if((DW>1536)&&((J=W<<3)<=I)&&((H<<3)<=Dst->H))            F=C256T2048;
  else if((DW>1280)&&((J=(W<<2)+(W<<1))<=I)&&((H<<2)+(H<<1)<=Dst->H)) F=C256T1536;
  else if((DW>1024)&&((J=(W<<2)+W)<=I)&&((H<<2)+H<=Dst->H)) F=C256T1280;
  else if((DW>768)&&((J=W<<2)<=I)&&((H<<2)<=Dst->H))        F=C256T1024;
  else if((DW>512)&&((J=(W<<1)+W)<=I)&&((H<<1)+H<=Dst->H))  F=C256T768;
  else if((DW>480)&&((J=W<<1)<=I)&&((H<<1)<=Dst->H))        F=C256T512;
  else if((DW>448)&&((J=(480*W)>>8)<=I))     F=C256T480;
  else if((DW>416)&&((J=(448*W)>>8)<=I))     F=C256T448;
  else if(DY&&(DW>352)&&((J=(416*W)>>8)<=I)) F=C256T416;
  else if(DY&&(DW>320)&&((J=(352*W)>>8)<=I)) F=C256T352;
  else if(DY&&((J=I)==320)&&(W==240))        F=C240T320;
  else if(DY&&(DW>256)&&((J=(320*W)>>8)<=I)) F=C256T320;
  else if((DW>240)&&((J=W)<=I))              F=C256T256;
  else if((DW>224)&&((J=(240*W)>>8)<=I))     F=C256T240;
  else if((DW>208)&&((J=(224*W)>>8)<=I))     F=C256T224;
  else if((DW>176)&&((J=(208*W)>>8)<=I))     F=C256T208;
  else if((DW>160)&&((J=(176*W)>>8)<=I))     F=C256T176;
  else if((DW>120)&&((J=(160*W)>>8)<=I))     F=C256T160;
  else if((J=(120*W)>>8)<=I)                 F=C256T120;

  /* If handler found... */
  if(F)
  {
    /* Compute source and destination pointers (align destination to 32bit) */
    DW = J;
    J  = Attrs&ARMSI_VFILL? Dst->H:((H*(W>H? W/H:1))*DW/W);
    J  = J>Dst->H? Dst->H:J;
    D  = (pixel *)Dst->Data+((Dst->H-J)>>1)*Dst->L+(((Dst->W-DW)>>1)&~1);
    S  = (pixel *)Src->Data+Y*Src->L+X;
    DY = ((H<<16)+J-1)/J;

/* Show selected video resolution */ 
//printf("%dx%d into %dx%d => %dx%d\n",W,H,Dst->W,Dst->H,DW,J);

    /* Scale the first line */
    (*F)(D,S,W);

    /* Scale line by line, copy repeating lines */
    for(H=J-1,D+=Dst->L,Y=0;H;--H,D+=Dst->L,Y=X)
    {
      X=Y+DY;
      if((Y^X)>>16) (*F)(D,S+(X>>16)*Src->L,W); else C256T256(D,D-Dst->L,DW);
    }

    /* Done, return destination width/height */
    return((J<<16)|DW);
  }

  /* ARMScaleImage() does not support this platform */
  ScaleImage(Dst,Src,X,Y,W,H);
  return((Dst->H<<16)|Dst->W);
}
#endif /* ARM_CPU && (BPP16||BPP24||BPP32) */

#ifdef LIBPNG
int LoadPNG(Image *Img,const char *FileName)
{
  png_structp PNGRead;
  png_infop PNGInfo;
  png_infop PNGEnd;
  png_bytepp Rows;
  png_bytep Src;
  png_uint_32 W,H;
  int J,D,X,Y;
  char Buf[16];
  pixel *Dst;
  FILE *F;

  /* Open file */
  F=fopen(FileName,"rb");
  if(!F) return(0);

  /* Read and verify header */
//  J=fread(Buf,1,8,F);
//printf("Checking PNG tag %s (%d)\n",Buf,J);fflush(stdout);
//  if(!J||!png_sig_cmp(Buf,0,J)) { fclose(F);return(0); }

  PNGRead = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
  PNGInfo = PNGRead? png_create_info_struct(PNGRead):0;
  PNGEnd  = PNGInfo? png_create_info_struct(PNGRead):0;

  if(!PNGRead||!PNGInfo||!PNGEnd)
  {
    png_destroy_read_struct(PNGRead? &PNGRead:0,PNGInfo? &PNGInfo:0,PNGEnd? &PNGEnd:0);
    fclose(F);
    return(0);
  }

  if(setjmp(png_jmpbuf(PNGRead)))
  {
    png_destroy_read_struct(&PNGRead,&PNGInfo,&PNGEnd);
    fclose(F);
    return(0);
  }

  /* Read PNG file into memory */
  png_init_io(PNGRead,F);
  png_read_png(PNGRead,PNGInfo,PNG_TRANSFORM_EXPAND,0);

  /* Get metadata */
  Rows = png_get_rows(PNGRead,PNGInfo);
  W    = png_get_image_width(PNGRead,PNGInfo);
  H    = png_get_image_height(PNGRead,PNGInfo);
  D    = png_get_bit_depth(PNGRead,PNGInfo);
  W    = W>Img->W? Img->W:W;
  H    = H>Img->H? Img->H:H;

  /* Copy image */
  for(Y=0,Dst=Img->Data;Y<H;++Y,Dst+=Img->L-W)
    for(X=0,Src=Rows[Y];X<W;++X,++Dst,Src+=3)
      *Dst = PIXEL(Src[0],Src[1],Src[2]);

  /* Done */
  png_destroy_read_struct(&PNGRead,&PNGInfo,&PNGEnd);
  fclose(F);
  return(1);
}
#endif /* LIBPNG */

#endif /* BPP32||BPP24||BPP16||BPP8 */
