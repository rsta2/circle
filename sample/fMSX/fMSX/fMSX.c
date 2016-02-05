/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                          fMSX.c                         **/
/**                                                         **/
/** This file contains generic main() procedure statrting   **/
/** the emulation.                                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2015                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "MSX.h"
#include "Help.h"
#include "EMULib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static const char *Options[]=
{ 
  "verbose","skip","pal","ntsc","help",
  "printer","serial","diska","diskb","tape","font","logsnd","state",
  "ram","vram","rom","auto","noauto","msx1","msx2","msx2+","joy",
  "home","simbdos","wd1793","sound","nosound","trap","sync","nosync",
  "tv","notv","lcd","nolcd","cmy","rgb","soft","epx","eagle",
  "saver","nosaver","shm","noshm","scale","static","nostatic",
  "vsync","480","200",
  0
};

extern const char *Title;/* Program title                       */
extern int   UseSound;   /* Sound mode                          */
extern int   UseZoom;    /* Zoom factor (#ifdef UNIX)           */
extern int   UseEffects; /* EFF_* bits, ORed (UNIX/MAEMO/MSDOS) */
extern int   UseStatic;  /* Use static colors (#ifdef MSDOS)    */
extern int   FullScreen; /* Use 640x480 screen (#ifdef MSDOS)   */
extern int   SyncFreq;   /* Sync scr updates (UNIX/MAEMO/MSDOS) */
extern int   ARGC;       /* argc/argv from main (#ifdef UNIX)   */
extern char **ARGV;

/** Zero-terminated arrays of disk names for each drive ******/
extern const char *Disks[2][MAXDISKS+1];

/** main() ***************************************************/
/** This is a main() function used in Unix and MSDOS ports. **/
/** It parses command line arguments, sets emulation        **/
/** parameters, and passes control to the emulation itself. **/
/*************************************************************/
int main(int argc,char *argv[])
{
  int CartCount,TypeCount;
  int JoyCount,DiskCount[2];
  int N,J;

#ifdef DEBUG
  CPU.Trap  = 0xFFFF;
  CPU.Trace = 0;
#endif

#if defined(UNIX) || defined(MAEMO)
  ARGC      = argc;
  ARGV      = argv;
#endif

#if defined(MSDOS) || defined(WINDOWS)
  /* No separate console, so no messages */
  Verbose=0;
  /* Figure out program directory name */
  ProgDir=GetFilePath(argv[0]);
#else
  Verbose=1;
#endif

  /* Clear everything */
  CartCount=TypeCount=JoyCount=0;
  DiskCount[0]=DiskCount[1]=0;

  /* Default disk images */
  Disks[0][1]=Disks[1][1]=0;
  Disks[0][0]=DSKName[0];
  Disks[1][0]=DSKName[1];

  for(N=1;N<argc;N++)
    if(*argv[N]!='-')
    {
      if(CartCount<MAXCARTS) ROMName[CartCount++]=argv[N];
      else printf("%s: Excessive filename '%s'\n",argv[0],argv[N]);
    }
    else
    {    
      for(J=0;Options[J];J++)
        if(!strcmp(argv[N]+1,Options[J])) break;

      switch(J)
      {
        case 0:  N++;
                 if(N<argc) Verbose=atoi(argv[N]);
                 else printf("%s: No verbose level supplied\n",argv[0]);
                 break;
  	case 1:  N++;
                 if(N>=argc)
                   printf("%s: No skipped frames percentage supplied\n",argv[0]);
                 else
                 {
                   J=atoi(argv[N]);
                   if((J>=0)&&(J<=99)) UPeriod=100-J; 
                 }
                 break;
        case 2:  Mode=(Mode&~MSX_VIDEO)|MSX_PAL;break;
        case 3:  Mode=(Mode&~MSX_VIDEO)|MSX_NTSC;break;
	case 4:  printf
                 ("%s by Marat Fayzullin (C)1994-2015\n",Title);
                 for(J=0;HelpText[J];J++) puts(HelpText[J]);
                 return(0);
        case 5:  N++;
                 if(N<argc) PrnName=argv[N];
                 else printf("%s: No file for printer redirection\n",argv[0]);
                 break;
        case 6:  N++;
                 if(N<argc) ComName=argv[N];
                 else printf("%s: No file for serial redirection\n",argv[0]);
                 break;
        case 7:  N++;
                 if(N>=argc)
                   printf("%s: No file for drive A\n",argv[0]);
                 else
                   if(DiskCount[0]>=MAXDISKS)
                     printf("%s: Too many disks for drive A\n",argv[0]);
                   else
                     Disks[0][DiskCount[0]++]=argv[N];
                 break;
        case 8:  N++;
                 if(N>=argc) 
                   printf("%s: No file for drive B\n",argv[0]);
                 else
                   if(DiskCount[1]>=MAXDISKS) 
                     printf("%s: Too many disks for drive B\n",argv[0]);
                   else
                     Disks[1][DiskCount[1]++]=argv[N];
                 break;
        case 9:  N++;  
                 if(N<argc) CasName=argv[N];
                 else printf("%s: No file for the tape\n",argv[0]);
                 break;
        case 10: N++;
                 if(N<argc) FNTName=argv[N];  
                 else printf("%s: No font name supplied\n",argv[0]);
                 break;
        case 11: N++;
                 if(N<argc) SndName=argv[N];
                 else printf("%s: No file for soundtrack logging\n",argv[0]);
                 break;
        case 12: N++;
                 if(N<argc) STAName=argv[N];
                 else printf("%s: No file to save emulation state\n",argv[0]);
                 break;
        case 13: N++;
                 if(N>=argc)
                   printf("%s: No number of RAM pages supplied\n",argv[0]);
                 else
                   RAMPages=atoi(argv[N]);
                 break;
        case 14: N++;
                 if(N>=argc)
                   printf("%s: No number of VRAM pages supplied\n",argv[0]);
                 else
                   VRAMPages=atoi(argv[N]);
                 break;
        case 15: N++;  
                 if(N>=argc)
                   printf("%s: No ROM mapper type supplied\n",argv[0]);
                 else
                   if(TypeCount>=MAXCARTS)
                     printf("%s: Excessive -rom option\n",argv[0]);
                   else
                   {
                     J=atoi(argv[N]);
                     if(J>=MAP_GUESS) Mode|=(MSX_GUESSA<<TypeCount);
                     else
                     {
                       Mode&=~(MSX_GUESSA<<TypeCount);
                       SETROMTYPE(TypeCount,J);
                     }            
                     ++TypeCount;
                   }
                 break;
        case 16: Mode|=MSX_AUTOSPACE;break;
        case 17: Mode&=~MSX_AUTOSPACE;break;
        case 18: Mode=(Mode&~MSX_MODEL)|MSX_MSX1;break;
        case 19: Mode=(Mode&~MSX_MODEL)|MSX_MSX2;break;
        case 20: Mode=(Mode&~MSX_MODEL)|MSX_MSX2P;break;
        case 21: N++;
                 if(N>=argc)
                   printf("%s: No joystick type supplied\n",argv[0]);
                 else
                   switch(JoyCount++)
                   {
                     case 0: SETJOYTYPE(0,atoi(argv[N])&0x03);break;
                     case 1: SETJOYTYPE(1,atoi(argv[N])&0x03);break;
                     default: printf("%s: Excessive -joy option\n",argv[0]);
                   }
                 break;
        case 22: N++;
                 if(N>=argc)
                   printf("%s: No home directory name supplied\n",argv[0]);
                 else
                   ProgDir=argv[N];
                 break;
        case 23: Mode|=MSX_PATCHBDOS;break;
        case 24: Mode&=~MSX_PATCHBDOS;break;
        case 25: N++;
                 if(N>=argc) { UseSound=1;N--; }
                 else if(sscanf(argv[N],"%d",&UseSound)!=1)
                      { UseSound=1;N--; }
                 break;
        case 26: UseSound=0;break;
 
#if defined(DEBUG)
        case 27: N++;
                 if(N>=argc)
                   printf("%s: No trap address supplied\n",argv[0]);
                 else
                   if(!strcmp(argv[N],"now")) CPU.Trace=1;
                   else sscanf(argv[N],"%hX",&(CPU.Trap));
                 break;
#endif /* DEBUG */

#if defined(MSDOS) || defined(UNIX) || defined(MAEMO)
        case 28: N++;
                 if(N<argc) SyncFreq=atoi(argv[N]);
                 else printf("%s: No sync frequency supplied\n",argv[0]);
                 break;
        case 29: SyncFreq=0;break;
        case 30: UseEffects|=EFF_TVLINES;break;
        case 31: UseEffects&=~EFF_TVLINES;break;
        case 32: UseEffects|=EFF_LCDLINES;break;
        case 33: UseEffects&=~EFF_LCDLINES;break;
        case 34: UseEffects|=EFF_CMYMASK;break;
        case 35: UseEffects|=EFF_RGBMASK;break;
        case 36: UseEffects=(UseEffects&~(EFF_SOFTEN|EFF_SOFTEN2))|EFF_2XSAL;break;
        case 37: UseEffects=(UseEffects&~(EFF_SOFTEN|EFF_SOFTEN2))|EFF_EPX;break;
        case 38: UseEffects=(UseEffects&~(EFF_SOFTEN|EFF_SOFTEN2))|EFF_EAGLE;break;
#endif /* MSDOS || UNIX || MAEMO */

#if defined(UNIX) || defined(MAEMO)
        case 39: UseEffects|=EFF_SAVECPU;break;
        case 40: UseEffects&=~EFF_SAVECPU;break;
#endif /* UNIX || MAEMO */

#if defined(UNIX)
#if defined(MITSHM)
        case 41: UseEffects|=EFF_MITSHM;break;
        case 42: UseEffects&=~EFF_MITSHM;break;
#endif
        case 43: N++;
                 if(N<argc) UseZoom=atoi(argv[N]);
                 else printf("%s: No scaling factor supplied\n",argv[0]);
                 break;
#endif /* UNIX */

#if defined(MSDOS)
        case 44: UseStatic=1;break;
        case 45: UseStatic=0;break;
        case 46: SyncFreq=-1;break;
        case 47: FullScreen=1;break;
        case 48: FullScreen=0;break;
#endif /* MSDOS */

        default: printf("%s: Wrong option '%s'\n",argv[0],argv[N]);
      }
    }

  /* Terminate disk lists and set initial disk names */
  if(DiskCount[0]) { Disks[0][DiskCount[0]]=0;DSKName[0]=Disks[0][0]; }
  if(DiskCount[1]) { Disks[1][DiskCount[1]]=0;DSKName[1]=Disks[1][0]; }

  /* Start fMSX! */
  if(!InitMachine()) return(1);
  StartMSX(Mode,RAMPages,VRAMPages);
  TrashMSX();
  TrashMachine();
  return(0);
}
