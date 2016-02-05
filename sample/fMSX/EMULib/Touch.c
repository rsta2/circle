/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          Touch.c                        **/
/**                                                         **/
/** This file contains functions that simulate joystick and **/
/** dialpad with the touch screen. It is normally used from **/
/** the platform-dependent functions that know where to get **/
/** pen coordinates from and where to draw pen cues to.     **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2008-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef DEFINE_ONCE
#define DEFINE_ONCE

#include "EMULib.h"
#include "Console.h"
#include "Touch.h"
#include <string.h>
#include <stdio.h>

#define XKEYS       12                   /* Number of keys in a row */
#define YKEYS       6                    /* Number of key rows      */

#define MAX_PENJOY_WIDTH 320             /* Max pen joystick width  */

#if defined(ANDROID)
static int KEYSTEP = 39;                 /* Key step in pixels      */
static int KEYSIZE = 31;                 /* Key size in pixels      */
static int CHRSIZE = 16;                 /* Key label size          */
#elif defined(MEEGO)                
static int KEYSTEP = 46;                 /* Key step in pixels      */
static int KEYSIZE = 38;                 /* Key size in pixels      */
static int CHRSIZE = 16;                 /* Key label size          */
#elif defined(MAEMO)                
static int KEYSTEP = 34;                 /* Key step in pixels      */
static int KEYSIZE = 30;                 /* Key size in pixels      */
static int CHRSIZE = 16;                 /* Key label size          */
#else                               
static int KEYSTEP = 14;                 /* Key step in pixels      */
static int KEYSIZE = 12;                 /* Key size in pixels      */
static int CHRSIZE = 8;                  /* Key label size          */
#endif

/* Currently selected virtual keyboard key */
static int KBDXPos = 0;
static int KBDYPos = 0;

/* Horizontal offsets of virtual keyboard lines */
#if defined(ANDROID) || defined(MEEGO)
static const int KBDOffsets[YKEYS] = { 0,0,0,0,32,16 };
#elif defined(MAEMO)
static const int KBDOffsets[YKEYS] = { 0,0,0,0,20,10 };
#else
static const int KBDOffsets[YKEYS] = { 0,0,0,0,8,4 };
#endif

/* Characters printed on virtual keyboard keys */
static const char *KBDLines[YKEYS+1] =
{
  "\33\20\21\22\23\24\25\26\27\16\17\32",
  "1234567890-=",
  "\11QWERTYUIOP\10",
  "^ASDFGHJKL;\15",
  "ZXCVBNM,./",
  "[]     \\'",
  0
};

static const char *PenCues[32] =
{
  "LEFT","RIGHT","UP","DOWN","A","B","L","R",
  "START","SELECT","EXIT","X","Y",0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static int CueSizes[32] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* Characters returned from virtual keyboard */
static const unsigned char KBDKeys[YKEYS][XKEYS] =
{
  {
    0x1B,CON_F1,CON_F2,CON_F3,CON_F4,CON_F5,
    CON_F6,CON_F7,CON_F8,CON_INSERT,CON_DELETE,CON_STOP
  },
  { '1','2','3','4','5','6','7','8','9','0','-','=' },
  { CON_TAB,'Q','W','E','R','T','Y','U','I','O','P',CON_BS },
  { '^','A','S','D','F','G','H','J','K','L',';',CON_ENTER },
  { 'Z','X','C','V','B','N','M',',','.','/',0,0 },
  { '[',']',' ',' ',' ',' ',' ','\\','\'',0,0,0 }
};

static unsigned int FFWDButtons = 0;
static unsigned int MENUButtons = 0;

static int JoyCuesSetup = 0;
static int OldCueImgW   = 0;

static const struct { int W,H,X,Y; } DefButtons[] =
{
  { 256,256, 8,     -48-16-256, },
  { 128,128, -128-8,48+8,       },
  { 128,128, -128-8,48+8+128+8, },
  { 128,48,  0,     0,          },
  { 128,48,  -128,  0,          },
  { 128,48,  0,     -48,        },
  { 128,48,  -128,  -48,        },
  { 128,48,  -256-8,-48,        },
};

static struct
{
  int Bit;
  Image Img;
  int W,H,X,Y;
  unsigned int Flags;
} Buttons[] =
{
  {  4, {0}, 0,0,0,0,0 }, // FIRE-A
  {  5, {0}, 0,0,0,0,0 }, // FIRE-B
  {  6, {0}, 0,0,0,0,0 }, // FIRE-L
  {  7, {0}, 0,0,0,0,0 }, // FIRE-R
  {  9, {0}, 0,0,0,0,0 }, // SELECT
  {  8, {0}, 0,0,0,0,0 }, // START
  { 11, {0}, 0,0,0,0,0 }, // FIRE-X
  { -1, {0}, 0,0,0,0,0 }, // Arrows (have to be last)
  { -2, {0}, 0,0,0,0,0 }
};

static int abs(int X) { return(X>=0? X:-X); }

/** GetKbdWidth()/GetKbdHeight() *****************************/
/** Return virtual keyboard dimensions.                     **/
/*************************************************************/
unsigned int GetKbdWidth()  { return(KEYSTEP*XKEYS+8); }
unsigned int GetKbdHeight() { return(KEYSTEP*YKEYS+8+CHRSIZE); }

/** GenericFullJoystick() ************************************/
/** Treat whole screen as one big directional pad. Result   **/
/** compatible with GetJoystick() (arrows only though).     **/
/*************************************************************/
unsigned int GenericFullJoystick(int X,int Y,int W,int H)
{
  /* Just consider the whole screen as one big directional pad */
  return(
    (X<(W>>1)-(W>>3)? BTN_LEFT : X>(W>>1)+(W>>3)? BTN_RIGHT : 0)
  | (Y<(H>>1)-(H>>3)? BTN_UP   : Y>(H>>1)+(H>>3)? BTN_DOWN  : 0)
  );
}

/** GenericPenJoystick() *************************************/
/** Get simulated joystick buttons from touch screen UI.    **/
/** Result compatible with GetJoystick().                   **/
/*************************************************************/
unsigned int GenericPenJoystick(int X,int Y,int W,int H)
{
  unsigned int J;
  int W3;

  /* Simulate joystick when pen touches the screen at X,Y */
  J = 0;

  /* Don't accept touches outside of the window frame */
  if((X<0)||(Y<0)||(X>=W)||(Y>=H)) return(0);
  W3 = W>MAX_PENJOY_WIDTH*3? MAX_PENJOY_WIDTH:W/3;

  /* Top 1/16 of the widget: FIREL and FIRER */
  if(Y<(H>>3))
  { if(X<W3) J|=BTN_FIREL; else if(X>=W-W3) J|=BTN_FIRER; }

  /* Bottom 1/16 of the widget: SELECT/EXIT and START */
  if(!J&&(Y>=(H-(H>>3))))
  { if(X<W3) J|=BTN_SELECT|BTN_EXIT; else if(X>=W-W3) J|=BTN_START; }

  /* Right upper corner of the screen is the fire buttons */
  if(!J&&(X>=W-(W3>>1))&&(Y>=(H>>3))&&(Y<(H>>3)+W3+(W3>>3)))
  {
    /* Fire buttons overlap */
    if(Y<=(H>>3)+(W3>>1)+(W3>>3)) J|=BTN_FIREA;
    if(Y>=(H>>3)+(W3>>1))         J|=BTN_FIREB;
  }

  /* Left 1/3 of the screen is the directional pad */
  if(!J&&(X<W3)&&(Y>=H-(H>>3)-W3))
  {
    Y-=H-W3-(H>>3);
    W3/=3;
    if(X<W3) J|=BTN_LEFT; else if(X>=(W3<<1)) J|=BTN_RIGHT;
    if(Y<W3) J|=BTN_UP;   else if(Y>=(W3<<1)) J|=BTN_DOWN;
  }

  /* Apply dynamically assigned FFWD and MENU buttons */
  J |= J&FFWDButtons? BTN_FFWD:0;
  J |= J&MENUButtons? BTN_MENU:0;

  /* Done, return simulated "joystick" state */
  return(J);
}

/** GenericPenDialpad() **************************************/
/** Get simulated dialpad buttons from touch screen UI.     **/
/*************************************************************/
unsigned char GenericPenDialpad(int X,int Y,int W,int H)
{
  int W3,H2;

  /* Dialpad is the middle 1/3 of the screen */
  W3 = W>MAX_PENJOY_WIDTH*3? MAX_PENJOY_WIDTH:W/3;
  H2 = H>W3? ((H-W3)>>1):0;
  W3 = (W-W3)>>1;
  return(
    (Y>=H2)&&(Y<H-H2)&&(X>=W3)&&(X<W-W3)?
    3*(X-W3)/(W-(W3<<1))+3*((Y-H2)/((H-(H2<<1))>>2))+1 : 0
  );
}

/** GenericPenKeyboard() *************************************/
/** Get virtual on-screen keyboard buttons.                 **/
/*************************************************************/
unsigned char GenericPenKeyboard(int X,int Y,int W,int H)
{
  int J;

  /* Pen coordinates relative to keyboard's top left corner */
  X -= W-KEYSTEP*XKEYS-8;
  Y -= H-KEYSTEP*YKEYS-8;

  /* Pen must be inside the keyboard */
  if((X<0)||(Y<0)) return(0);

  /* Keyboard row index */
  Y/= KEYSTEP;
  if(Y>=YKEYS) return(0);

  /* Adjust for row position on screen */
  for(J=0;J<=Y;++J) X-=KBDOffsets[J];
  if(X<0) return(0);

  /* Keyboard column index */
  X/= KEYSTEP;
  if(X>=XKEYS) return(0);

  /* Memorize last pressed key */
  KBDXPos = X;
  KBDYPos = Y;

  /* Return key */
  return(KBDKeys[Y][X]);
}

/** GenericDialKeyboard() ************************************/
/** Process dialpad input to the virtual keyboard. Returns  **/
/** virtual keyboard key if selected, or 0 if not.          **/
/*************************************************************/
unsigned char GenericDialKeyboard(unsigned char Key)
{
  /* Interpret input key */
  switch(Key)
  {
    case CON_LEFT:
      KBDXPos = (KBDXPos>0? KBDXPos:strlen(KBDLines[KBDYPos]))-1;
      break;
    case CON_RIGHT:
      KBDXPos = KBDXPos<strlen(KBDLines[KBDYPos])-1? KBDXPos+1:0;
      break;
    case CON_UP:
      KBDYPos = KBDYPos>0? KBDYPos-1:YKEYS-1;
      KBDXPos = KBDXPos<strlen(KBDLines[KBDYPos])? KBDXPos:strlen(KBDLines[KBDYPos])-1;
      break;
    case CON_DOWN:
      KBDYPos = KBDYPos<YKEYS-1? KBDYPos+1:0;
      KBDXPos = KBDXPos<strlen(KBDLines[KBDYPos])? KBDXPos:strlen(KBDLines[KBDYPos])-1;
      break;
    case CON_OK:
      /* Return ASCII character */
      return(KBDLines[KBDYPos][KBDXPos]);
  }

  /* Key has not been interpreted */
  return(0);
}

/** SetPenCues() *********************************************/
/** Set pen cues for given buttons to a given string.       **/
/*************************************************************/
void SetPenCues(unsigned int Buttons,const char *CueText)
{
  unsigned int J;

  if(!strcmp(CueText,"FFWD")||!strcmp(CueText,"SLOW")) FFWDButtons|=Buttons; else FFWDButtons&=~Buttons;
  if(!strcmp(CueText,"MENU")) MENUButtons|=Buttons; else MENUButtons&=~Buttons;

  for(J=0;J<sizeof(PenCues)/sizeof(PenCues[0]);++J)
    if(Buttons&(1<<J))
    {
      PenCues[J]  = CueText;
      CueSizes[J] = strlen(CueText)*CHRSIZE;
    }

#ifdef ANDROID
  // On Android, request update to the screen overlay
  UpdateOverlay();
#endif 
}

/** SetPenKeyboard() *****************************************/
/** Set pen keyboard dimensions.                            **/
/*************************************************************/
void SetPenKeyboard(unsigned int KeyStep,unsigned int KeySize,unsigned int ChrSize)
{
  int J;

  /* Character size must be a multiple of 8 */
  CHRSIZE = ChrSize>8? (ChrSize&~7):8;
  /* Make sure keys do not overlap */
  if(KeySize+4>KeyStep) KeySize=KeyStep-4;
  /* Make sure key labels fit into keys */
  if(CHRSIZE+2>KeySize) KeySize=CHRSIZE+2;
  /* Now grow key step again, if needed */
  if(KeySize+4>KeyStep) KeyStep=KeySize+4;
  /* Assign new virtual key dimensions */
  KEYSIZE = KeySize;
  KEYSTEP = KeyStep;

  /* Recompute cue lengths */
  for(JoyCuesSetup=1,J=0;J<sizeof(PenCues)/sizeof(PenCues[0]);++J)
    CueSizes[J] = PenCues[J]? strlen(PenCues[J])*CHRSIZE:0;
}

/** InitFinJoystick() ****************************************/
/** Initialize finger joystick images by cropping them from **/
/** the given source image. Returns number of buttons set   **/
/** successfully (i.e. found inside the Src bounds).        **/
/*************************************************************/
int InitFinJoystick(const Image *Src)
{
  int J,X0,Y0;

  /* Initialize virtual joystick images */  
  for(J=0;Buttons[J].Bit>-2;++J)
  {
    /* Free the existing image */
    FreeImage(&Buttons[J].Img);

    /* If source image supplied... */
    if(Src)
    {
      /* Compute button's top-left corner position in the Src */
      X0 = DefButtons[J].X + (DefButtons[J].X<0? Src->W:0);
      Y0 = DefButtons[J].Y + (DefButtons[J].Y<0? Src->H:0);

      /* Crop the button out of the Src image */
      CropImage(&Buttons[J].Img,Src,X0,Y0,DefButtons[J].W,DefButtons[J].H);
    }

    /* Reset button size and position to the defaults */
    Buttons[J].X = DefButtons[J].X;
    Buttons[J].Y = DefButtons[J].Y;
    Buttons[J].W = DefButtons[J].W;
    Buttons[J].H = DefButtons[J].H;

    /* Button now visible and enabled */
    Buttons[J].Flags = 0;
  }

  /* Return number of buttons initialized */
  return(J);
}

/** SetFinButton() *******************************************/
/** Set finger joystick button(s) to given location. When   **/
/** Img=0, create wireframe buttons. When Mask=0, set the   **/
/** directional buttons image and location. When Mask ORed  **/
/** with BTN_INVISIBLE, create invisible buttons. Returns   **/
/** the number of virtual buttons set or 0 for none.        **/
/*************************************************************/
int SetFinButton(unsigned int Mask,const Image *Img,int X,int Y,int W,int H)
{
  unsigned int Flags;
  int I,J,Result;

  /* Special Mask bits: make button invisible or disable it */
  Flags = Mask&BTN_INVISIBLE;
  Mask  = Mask&~BTN_INVISIBLE;

  /* No image for invisible buttons */
  if(Flags&BTN_INVISIBLE) Img=0;

  /* When Mask=0, we are assigning the arrow buttons */
  if(!Mask) Mask=0x80000000;

  for(J=Result=0;Mask;++J,Mask>>=1)
    if(Mask&1)
      for(I=0;Buttons[I].Bit>-2;++I)
        if((Buttons[I].Bit==J) || ((Buttons[I].Bit==-1)&&(J==31)))
        {
          /* When Img=0, make a wireframe or invisible button */
          if(Img) CropImage(&Buttons[I].Img,Img,0,0,W,H);
          else    FreeImage(&Buttons[I].Img);

          Buttons[I].Flags = Flags;
          Buttons[I].X = X;
          Buttons[I].Y = Y;
          Buttons[I].W = W;
          Buttons[I].H = H;
          ++Result;
        }

  /* Return number of modified buttons */
  return(Result);
}

/** GenericFinJoystick() *************************************/
/** Return the BTN_* bits corresponding to position X,Y of  **/
/** the finger joystick shown in Dst.                       **/
/*************************************************************/
unsigned int GenericFinJoystick(int X,int Y,int W,int H,unsigned int CurState)
{
  unsigned int Result,I,J;
  int X0,Y0,AX,AY,W0,H0;

  /* For every known button... */
  for(J=Result=0;Buttons[J].Bit>-2;++J)
  {
    /* Compute finger position relative to the button */
    X0 = X - Buttons[J].X - (Buttons[J].X<0? W:0);
    Y0 = Y - Buttons[J].Y - (Buttons[J].Y<0? H:0);

    /* If button has been pressed... */
    if(
       ((X0>=0) && (Y0>=0) && (X0<Buttons[J].W) && (Y0<Buttons[J].H))
    || ((Buttons[J].Bit<0) && !Result && (CurState&BTN_ARROWS))
    )
    {
      /* ...and it is a normal button... */
      if(Buttons[J].Bit>=0)
      {
        /* Compute button mask from bit number */
        I = 1<<Buttons[J].Bit;
        /* Also apply dynamically assigned FFWD and MENU buttons */
        Result |= I|(I&FFWDButtons? BTN_FFWD:0)|(I&MENUButtons? BTN_MENU:0);
      }
      else
      {
        /* We are dealing with the joypad arrows here */
        W0  = Buttons[J].W>>1;
        H0  = Buttons[J].H>>1;
        X0 -= W0;
        Y0 -= H0;
        AX  = abs(X0);
        AY  = abs(Y0);

        /* If finger is outside the joypad, keep pressing previous arrows */
        if((AX>W0) || (AY>H0))
          Result |= CurState & ((X0<0? BTN_LEFT:BTN_RIGHT)|(Y0<0? BTN_UP:BTN_DOWN));

        /* Joypad's center is inactive at 1/16 of the radius */
        else if((AX>=(W0>>3)) || (AY>=(H0>>3)))
        {
          /* This is joypad's edge at 1/4 of the radius */
          W0 = W0>>1;
          H0 = H0>>1;
   
          Result |=
            ((X0<0) && ((AX>AY) || (AX>W0))? BTN_LEFT:0)
          | ((Y0<0) && ((AY>AX) || (AY>H0))? BTN_UP:0)
          | ((X0>0) && ((AX>AY) || (AX>W0))? BTN_RIGHT:0)
          | ((Y0>0) && ((AY>AX) || (AY>H0))? BTN_DOWN:0);
        }
      }
    }
  }

  /* Done */
  return(Result);
}

#endif /* DEFINE_ONCE */

#if !defined(BPP32) && !defined(BPP24) && !defined(BPP16) && !defined(BPP8)
/* When pixel size not defined, compile in the universal multiplexer */
#include "TouchMux.h"
#else

#if defined(ANDROID) || defined(MEEGO)
#define CLR_NORMALF PIXEL(64,255,64)     /* Normal key foreground   */
#else
#define CLR_NORMALF PIXEL(0,0,0)         /* Normal key foreground   */
#define CLR_NORMALB PIXEL(255,255,255)   /* Normal key background   */
#endif
#define CLR_ACTIVEF PIXEL(255,255,255)   /* Active key foreground   */
#define CLR_ACTIVEB PIXEL(255,64,64)     /* Active key background   */
#define CLR_FPS     PIXEL(255,128,255)   /* Framerate counter color */
#define CLR_CUES    PIXEL(127,127,127)   /* Touch joypad cues color */

/** PrintXY2() ***********************************************/
/** Print a string in given color on transparent background.**/
/*************************************************************/
static void PrintXY2(Image *Img,const char *S,int X,int Y,pixel FG)
{
  const unsigned char *C;
  pixel *P;
  int I,J,K,N;

  X = X<0? 0:X>Img->W-CHRSIZE? Img->W-CHRSIZE:X;
  Y = Y<0? 0:Y>Img->H-CHRSIZE? Img->H-CHRSIZE:Y;

  for(K=X;*S;S++)
    switch(*S)
    {
      case '\n':
        K=X;Y+=CHRSIZE;
        if(Y>Img->H-CHRSIZE) Y=0;
        break;
      default:
        P=(pixel *)Img->Data+Img->L*Y+K;
        for(C=CONGetFont()+(*S<<3),J=8;J;P+=Img->L*(CHRSIZE/8),++C,--J)
          for(I=0,N=(int)*C<<24;N&&(I<CHRSIZE);I+=CHRSIZE/8,N<<=1)
            if(N&0x80000000) P[I+1]=P[I]=FG;
        K+=CHRSIZE;
        if(X>Img->W-CHRSIZE)
        {
          K=0;Y+=CHRSIZE;
          if(Y>Img->H-CHRSIZE) Y=0;
        }
        break;
    }
}

/** DrawVLine()/DrawHLine() **********************************/
/** Draw dotted lines used to show cues for PenJoystick().  **/
/*************************************************************/
static void DrawVLine(Image *Img,int X,int Y1,int Y2,pixel Color)
{
  pixel *P;
  int J;

  if((X<0)||(X>=Img->W)) return;

  Y1 = Y1<0? 0:Y1>=Img->H? Img->H-1:Y1;
  Y2 = Y2<0? 0:Y2>=Img->H? Img->H-1:Y2;
  if(Y1>Y2) { J=Y1;Y1=Y2;Y2=J; }

  P = (pixel *)Img->Data+Img->L*Y1+X;  
  for(J=Y1;J<=Y2;J+=4) { *P=Color;P+=Img->L<<2; }
}

static void DrawHLine(Image *Img,int X1,int X2,int Y,pixel Color)
{
  pixel *P;
  int J;

  if((Y<0)||(Y>=Img->H)) return;

  X1 = X1<0? 0:X1>=Img->W? Img->W-1:X1;
  X2 = X2<0? 0:X2>=Img->W? Img->W-1:X2;
  if(X1>X2) { J=X1;X1=X2;X2=J; }

  P = (pixel *)Img->Data+Img->L*Y+X1;
  for(J=X1;J<=X2;J+=4) { *P=Color;P+=4; }
}

/** DrawDialpad() ********************************************/
/** Draw virtual dialpad in a given image.                  **/
/*************************************************************/
void DrawDialpad(Image *Img,int Color)
{
  pixel *P;
  int W,H,H2,W9,W3;

  /* Use default color, if requested */
  if(Color<0) Color=CLR_CUES;

  P  = (pixel *)Img->Data;
  W  = Img->W;
  H  = Img->H;
  W3 = W>MAX_PENJOY_WIDTH*3? MAX_PENJOY_WIDTH:W/3;
  H2 = H>W3? ((H-W3)>>1):0;
  W9 = W3/3;
  W3 = (W-W3)>>1;

  DrawHLine(Img,W3,W-W3,H2,Color);
  DrawHLine(Img,W3,W-W3,(H2>>1)+(H>>2),Color);
  DrawHLine(Img,W3,W-W3,H>>1,Color);
  DrawHLine(Img,W3,W-W3,(H>>1)-(H2>>1)+(H>>2),Color);
  DrawHLine(Img,W3,W-W3,H-H2-1,Color);
  DrawVLine(Img,W3,H2,H-H2-1,Color);
  DrawVLine(Img,W3+W9,H2,H-H2-1,Color);
  DrawVLine(Img,W-W3-W9,H2,H-H2-1,Color);
  DrawVLine(Img,W-W3,H2,H-H2-1,Color);
  PrintXY2(Img,"1",W3+2,H2+2,Color);
  PrintXY2(Img,"2",W3+W9+2,H2+2,Color);
  PrintXY2(Img,"3",W-W3-W9+2,H2+2,Color);
  PrintXY2(Img,"4",W3+2,(H2>>1)+(H>>2)+2,Color);
  PrintXY2(Img,"5",W3+W9+2,(H2>>1)+(H>>2)+2,Color);
  PrintXY2(Img,"6",W-W3-W9+2,(H2>>1)+(H>>2)+2,Color);
  PrintXY2(Img,"7",W3+2,(H>>1)+2,Color);
  PrintXY2(Img,"8",W3+W9+2,(H>>1)+2,Color);
  PrintXY2(Img,"9",W-W3-W9+2,(H>>1)+2,Color);
  PrintXY2(Img,"*",W3+2,(H>>1)-(H2>>1)+(H>>2)+2,Color);
  PrintXY2(Img,"0",W3+W9+2,(H>>1)-(H2>>1)+(H>>2)+2,Color);
  PrintXY2(Img,"#",W-W3-W9+2,(H>>1)-(H2>>1)+(H>>2)+2,Color);
}

/** DrawPenCues() ********************************************/
/** Overlay dotted cue lines for using PenJoystick() onto a **/
/** given image. Show dialpad cues if requested.            **/
/*************************************************************/
void DrawPenCues(Image *Img,int ShowDialpad,int Color)
{
  pixel *P;
  int W,H,W9,W3,X,Y,J;

  /* Use default color, if requested */
  if(Color<0) Color=CLR_CUES;

  /* Set up pen keyboard for the first time */
  if(!JoyCuesSetup) SetPenKeyboard(KEYSTEP,KEYSIZE,CHRSIZE);

  P  = (pixel *)Img->Data;
  W  = Img->W;
  H  = Img->H;
  W3 = Img->W>MAX_PENJOY_WIDTH*3? MAX_PENJOY_WIDTH:W/3;
  W9 = W3/3;

  /* Vertical edges */
  DrawVLine(Img,W3,0,H>>3,Color);
  DrawVLine(Img,W3,H-W3-(H>>3),H-1,Color);
  DrawVLine(Img,W-W3,0,H>>3,Color);
  DrawVLine(Img,W-W3,H-(H>>3),H,Color);

  /* Corner buttons */
  DrawHLine(Img,0,W3,H>>3,Color);
  DrawHLine(Img,W-W3,W-1,H>>3,Color);
  DrawHLine(Img,0,W3,H-(H>>3),Color);
  DrawHLine(Img,W-W3,W-1,H-(H>>3),Color);

  /* Fire buttons (with overlap) */
  DrawHLine(Img,W-(W3>>1),W-1,(H>>3)+(W3>>1),Color);
  DrawHLine(Img,W-(W3>>1),W-1,(H>>3)+(W3>>1)+(W3>>3),Color);
  DrawHLine(Img,W-(W3>>1),W-1,(H>>3)+W3+(W3>>3),Color);
  DrawVLine(Img,W-(W3>>1),H>>3,(H>>3)+W3+(W3>>3),Color);

  /* Directional buttons */
  DrawVLine(Img,W9,H-W3-(H>>3),H-(H>>3),Color);
  DrawVLine(Img,W3-W9,H-W3-(H>>3),H-(H>>3),Color);
  DrawHLine(Img,0,W3,H-W3-(H>>3),Color);
  DrawHLine(Img,0,W3,H-(W9<<1)-(H>>3),Color);
  DrawHLine(Img,0,W3,H-W9-(H>>3),Color);

  /* Button labels */
  if(PenCues[4]) PrintXY2(Img,PenCues[4],W-CueSizes[4]-2,(H>>3)+2,Color);
  if(PenCues[5]) PrintXY2(Img,PenCues[5],W-CueSizes[5]-2,(H>>3)+(W3>>1)+(W3>>3)+2,Color);
  if(PenCues[6]) PrintXY2(Img,PenCues[6],2,2,Color);
  if(PenCues[7]) PrintXY2(Img,PenCues[7],W-CueSizes[7]-2,2,Color);
  if(PenCues[8]) PrintXY2(Img,PenCues[8],W-CueSizes[8]-2,H-(H>>3)+2,Color);
  if(PenCues[9]) PrintXY2(Img,PenCues[9],2,H-(H>>3)+2,Color);

  /* Arrow labels */
  if(PenCues[0]&&(CueSizes[0]<=W9)) PrintXY2(Img,PenCues[0],2,H-(W9<<1)-(H>>3)+2,Color);
  if(PenCues[1]&&(CueSizes[1]<=W9)) PrintXY2(Img,PenCues[1],(W9<<1)+2,H-(W9<<1)-(H>>3)+2,Color);
  if(PenCues[2]&&(CueSizes[2]<=W9)) PrintXY2(Img,PenCues[2],W9+2,H-W3-(H>>3)+2,Color);
  if(PenCues[3]&&(CueSizes[3]<=W9)) PrintXY2(Img,PenCues[3],W9+2,H-W9-(H>>3)+2,Color);

  /* If requested, show on-screen dialpad */
  if(ShowDialpad) DrawDialpad(Img,Color);
}

/** DrawKeyboard() *******************************************/
/** Draw virtual keyboard in a given image. Key modifiers   **/
/** and the key code passed in CurKey are highlighted.      **/
/*************************************************************/
void DrawKeyboard(Image *Img,unsigned int CurKey)
{
  int X,Y,J,I,K,L;
  char S[2];
  pixel *P;

  /* Keyboard in the right-bottom corner by default */
  X = Img->W-GetKbdWidth();
  Y = Img->H-GetKbdHeight();
  if((X<0)||(Y<0)) return;

  /* Draw modifiers */
  if(CurKey&CON_MODES)
  {
    J=X;
    if(CurKey&CON_SHIFT)   { PrintXY2(Img,"SHIFT",J,Y,CLR_ACTIVEB);J+=6*CHRSIZE; }
    if(CurKey&CON_CONTROL) { PrintXY2(Img,"CTRL",J,Y,CLR_ACTIVEB);J+=5*CHRSIZE; }
    if(CurKey&CON_ALT)     { PrintXY2(Img,"ALT",J,Y,CLR_ACTIVEB);J+=4*CHRSIZE; }
  }

  /* Draw keyboard under modifiers */
  Y += CHRSIZE;

  /* Draw keys */
  for(I=J=0,S[1]='\0';KBDLines[I];++I,Y+=KEYSTEP,X+=KBDOffsets[I]-J*KEYSTEP)
    for(J=0;KBDLines[I][J];++J,X+=KEYSTEP)
    {
      /* Draw key frame */
      P = (pixel *)Img->Data
        + Img->L*(Y+(KEYSTEP-KEYSIZE)/2)
        + X+(KEYSTEP-KEYSIZE)/2;

      /* Highlight current key */
      if(KBDKeys[I][J]==(CurKey&CON_KEYCODE))
      {
#ifdef CLR_ACTIVEB
        for(K=1;K<KEYSIZE-1;++K) P[K]=CLR_ACTIVEB;
        for(K=1,P+=Img->L;K<KEYSIZE-1;++K,P+=Img->L)
          for(L=0;L<KEYSIZE;++L) P[L]=CLR_ACTIVEB;
        for(K=1;K<KEYSIZE-1;++K) P[K]=CLR_ACTIVEB;
#endif
        K=CLR_ACTIVEF;
      }
      else
      {
        for(K=1;K<KEYSIZE-1;++K) P[K]=CLR_NORMALF;
        for(K=1,P+=Img->L;K<KEYSIZE-1;++K,P+=Img->L)
        {
#ifdef CLR_NORMALB
          for(L=1;L<KEYSIZE-1;++L) P[L]=CLR_NORMALB;
#endif
          P[0]=P[KEYSIZE-1]=CLR_NORMALF;
        }
        for(K=1;K<KEYSIZE-1;++K) P[K]=CLR_NORMALF;
        K=CLR_NORMALF;
      }

      /* Draw key label */
      S[0]=KBDLines[I][J];
      PrintXY2(Img,S,X+(KEYSTEP-CHRSIZE)/2,Y+(KEYSTEP-CHRSIZE)/2,K);
    }
}

/** DrawFinJoystick() ****************************************/
/** Draw finger joystick into given destination image. When **/
/** DW=0 or DWH=0, the whole image will be updated. When    **/
/** DWxDH+DX+DY represent a dirty rectangle inside Dst, the **/
/** function will only update joystick buttons overlapping  **/
/** this rectangle, representing them with dotted lines.    **/
/** Returns the number of buttons overlapping the dirty     **/
/** rectangle.                                              **/
/*************************************************************/
int DrawFinJoystick(Image *Dst,int DX,int DY,int DW,int DH,int TextColor)
{
  int Result,X0,Y0,X1,Y1,XL,YL,J,NeedFrame,NeedLabel;
  Image Dirty;

  /* Use default cue color, if requested */
  if(TextColor<0) TextColor=CLR_CUES;

  /* Compute position of the "dirty" rectangle in the middle */
  if(!DW) DX=0;
  if(!DH) DY=0;

  /* This image corresponds to the "dirty" rectangle, if given */
  CropImage(&Dirty,Dst,DX,DY,DW? DW:Dst->W,DH? DH:Dst->H);

  /* Draw controls */
  for(J=Result=0;Buttons[J].Bit>-2;++J)
  {
    X0 = Buttons[J].X + (Buttons[J].X<0? Dst->W:0) - DX;
    Y0 = Buttons[J].Y + (Buttons[J].Y<0? Dst->H:0) - DY;
    X1 = X0 + Buttons[J].W;
    Y1 = Y0 + Buttons[J].H;
    NeedLabel = Buttons[J].W && Buttons[J].H;
    NeedFrame = 0;

    /* If need to draw something... */
    if(!(Buttons[J].Flags&BTN_INVISIBLE) && NeedLabel)
    {
      /* If "dirty" rectangle given... */
      if(DW && DH)
      {
        /* Image obscures "dirty" rectangle: will draw frame and label */
        if((X0<DW) && (Y0<DH) && (X1>=0) && (Y1>=0))
        { NeedFrame=NeedLabel=1;++Result; }
      }
      else
      {
        /* Draw frame if no image data */
        if(!Buttons[J].Img.Data) NeedFrame=1;
        else IMGCopy(Dst,X0,Y0,&Buttons[J].Img,0,0,Buttons[J].W,Buttons[J].H,PIXEL(255,0,255));
        /* Draw label */
        NeedLabel=1;
      }
    }

    /* If need to draw a frame... */
    if(NeedFrame)
    {
      /* Draw button frame */
      DrawVLine(&Dirty,X0,Y0,Y1-1,TextColor);
      DrawVLine(&Dirty,X1-1,Y0,Y1-1,TextColor);
      DrawHLine(&Dirty,X0,X1-1,Y0,TextColor);
      DrawHLine(&Dirty,X0,X1-1,Y1-1,TextColor);

      /* Draw arrow pad */
      if(Buttons[J].Bit<0)
      {
        int I;
        I = (X1-X0)/3; 
        DrawVLine(&Dirty,X0+I,Y0,Y1-1,TextColor);
        DrawVLine(&Dirty,X1-I,Y0,Y1-1,TextColor);
        I = (Y1-Y0)/3;
        DrawHLine(&Dirty,X0,X1-1,Y0+I,TextColor);
        DrawHLine(&Dirty,X0,X1-1,Y1-I,TextColor);
      }
    }

    /* If need to draw label and the label exists... */
    if(NeedLabel && (Buttons[J].Bit>=0) && PenCues[Buttons[J].Bit])
      PrintXY2(
        Dst,
        PenCues[Buttons[J].Bit],
        ((X0+X1-CueSizes[Buttons[J].Bit])>>1)+DX,
        ((Y0+Y1-CHRSIZE)>>1)+DY,
        TextColor
      );
  }

  /* Return number of buttons overlapping "safe" rectangle */
  return(Result);
}

/** RenderVideo() ********************************************/
/** Draw video buffer to a given image. Return 0 on failure **/
/** or destination rectangle size inside OutImg on success. **/
/*************************************************************/
unsigned int RenderVideo(Image *OutImg,Image *CueImg,int Effects,int PenKey,int FrameRate)
{
  unsigned int DW,DH,SW,SH,X,Y,W,H,J;
  Image TmpImg;

  /* Safety check */
  if(!VideoImg || !VideoImg->Data) return(0);

  if(Effects&EFF_DIRECT)
  {
    W = VideoImg->W;
    H = VideoImg->H;
    X = 0;
    Y = 0;
  }
  else
  {
    W = VideoW;
    H = VideoH;
    X = VideoX;
    Y = VideoY;
  }

  /* Determine destination image dimensions */
  if(!OutImg)
  {
    /* If no destination image given, we assume VideoImg */
    CropImage(&TmpImg,VideoImg,X,Y,W,H);
    CueImg = CueImg? CueImg:&TmpImg;
    OutImg = &TmpImg;
    DW     = W;
    DH     = H;
  }
  else if(!(Effects&EFF_SCALE)) { DW=W;DH=H; }
  else if(Effects&EFF_STRETCH)  { DW=OutImg->W;DH=OutImg->H; }
  else
  {
    DW = W/H;
    DH = H/W;
    SW = W*(DH>0? DH:1);
    SH = H*(DW>0? DW:1);
    DW = SW*OutImg->H/SH;
    DH = SH*OutImg->W/SW;
    if(DW>OutImg->W) DW=OutImg->W; else DH=OutImg->H;
  }

  /* If destination image has not been given... */
  if(OutImg==&TmpImg)
  {
    /* We do not copy or scale */
  }
  /* EFF_SOFTEN/EFF_SOFTEN2: Soften image using pixel interpolation */
  else if(J=Effects&(EFF_SOFTEN|EFF_SOFTEN2))
  {
    CropImage(&TmpImg,OutImg,(OutImg->W-DW)>>1,(OutImg->H-DH)>>1,DW,DH);
    if(J==EFF_2XSAL)
      SoftenImage(&TmpImg,VideoImg,X,Y,W,H);
    else if(J==EFF_EPX)
      SoftenEPX(&TmpImg,VideoImg,X,Y,W,H);
    else if(J==EFF_EAGLE)
      SoftenEAGLE(&TmpImg,VideoImg,X,Y,W,H);
  }
  /* EFF_SCALE: Scale image to the screen size */
  else if(Effects&EFF_SCALE)
  {
    /* EFF_STRETCH: Stretch image to fill the whole screen */
    if(Effects&EFF_STRETCH)
      ScaleImage(OutImg,VideoImg,X,Y,W,H);
    else
    {
#if defined(ARM_CPU)
      /* Scale image to fill the screen, using ARM assembler */
      DW = ARMScaleImage(OutImg,VideoImg,X,Y,W,H,0);
      /* Update destination image dimensions to optimized values */
      DH = DW>>16;
      DW&= 0xFFFF;
#else
      /* Scale image to fill the screen, using C code */
      CropImage(&TmpImg,OutImg,(OutImg->W-DW)>>1,(OutImg->H-DH)>>1,DW,DH);
      ScaleImage(&TmpImg,VideoImg,X,Y,W,H);
#endif
    }
  }
  /* DEFAULT: Not scaling, stretching, or softening image */
  else
  {
    /* Center image at the screen */
    IMGCopy(OutImg,(OutImg->W-W)>>1,(OutImg->H-H)>>1,VideoImg,X,Y,W,H,-1);
    DW = W;
    DH = H;
  }

  /* EFF_CMYMASK/EFF_RGBMASK: Apply color component masks */
  if(Effects&EFF_CMYMASK)
    CMYizeImage(OutImg,(OutImg->W-DW)>>1,(OutImg->H-DH)>>1,DW,DH);
  else if(Effects&EFF_RGBMASK)
    RGBizeImage(OutImg,(OutImg->W-DW)>>1,(OutImg->H-DH)>>1,DW,DH);

  /* EFF_TVLINES/EFF_LCDLINES: Apply "scanlines" effect  */
  X = Effects&(EFF_TVLINES|EFF_LCDLINES);
  if(X)
  {
    /* Make sure width is a multiple of 8/16 pixels for optimization */
#if !defined(ARM_CPU)
    Y = X==EFF_TVLINES? DW:(DW&~1);
#elif defined(BPP32) || defined(BPP24)
    Y = DW&~7;
#else
    Y = DW&~15;
#endif

    if(X==EFF_TVLINES)
      TelevizeImage(OutImg,(OutImg->W-Y)>>1,(OutImg->H-DH)>>1,Y,DH);
    else if(X==EFF_LCDLINES)
      LcdizeImage(OutImg,(OutImg->W-Y)>>1,(OutImg->H-DH)>>1,Y,DH);
    else
      RasterizeImage(OutImg,(OutImg->W-Y)>>1,(OutImg->H-DH)>>1,Y,DH);
  }

  /* If drawing any touch input cues... */
  if(Effects&(EFF_VKBD|EFF_PENCUES))
  {
    /* If no image supplied for the input cues... */
    if(!CueImg)
    {
      /* In landscape mode, draw input cues on top of OutImg */
      if(OutImg->H<=OutImg->W) CueImg=OutImg;
      else
      {
        /* In portrait mode, draw input cues below OutImg */
        CropImage(&TmpImg,OutImg,0,DH,OutImg->W,OutImg->H-DH);
        CueImg = &TmpImg;
      }
    }

#if defined(ANDROID) || defined(MEEGO)
    /* If cue image width changed... */
    if(OldCueImgW!=CueImg->W)
    {
      /* Adjust virtual keyboard and cues size to fit screen */
      if(CueImg->W>=1024)     SetPenKeyboard(72,64,16);
      else if(CueImg->W>=768) SetPenKeyboard(60,52,16);
      else if(CueImg->W>=640) SetPenKeyboard(46,38,16);
      else if(CueImg->W>=480) SetPenKeyboard(39,31,16);
      else if(CueImg->W>=320) SetPenKeyboard(24,20,16);
      else                    SetPenKeyboard(16,14,8);
      /* New cue image width now in effect */
      OldCueImgW = CueImg->W;
    }

    /* Draw virtual joystick */
    if(Effects&EFF_PENCUES)
      DrawFinJoystick(CueImg,(OutImg->W-DW)>>1,(OutImg->H-DH)>>1,DW,DH,CLR_CUES);
#else
    /* Draw virtual joystick */
    if(Effects&EFF_PENCUES) DrawPenCues(CueImg,Effects&EFF_DIALCUES,CLR_CUES);
#endif

    /* Draw virtual keyboard */
    if(Effects&EFF_VKBD) DrawKeyboard(CueImg,PenKey);
  }

  /* Show framerate if requested */
  if((Effects&EFF_SHOWFPS)&&(FrameRate>0))
  {
    char S[8];
    sprintf(S,"%dfps",FrameRate);
    PrintXY2(OutImg,S,((OutImg->W-DW)>>1)+8,((OutImg->H-DH)>>1)+8,CLR_FPS);
  }

  /* Done with the screen */
  return((DW>1)&&(DH>1)? ((DW&0xFFFF)|(DH<<16)):0);
}

#undef CLR_NORMALF
#undef CLR_NORMALB
#undef CLR_ACTIVEF
#undef CLR_ACTIVEB
#undef CLR_FPS
#undef CLR_CUES

#endif /* BPP32||BPP24||BPP16||BPP8 */
