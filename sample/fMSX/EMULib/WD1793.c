/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         WD1793.c                        **/
/**                                                         **/
/** This file contains emulation for the WD1793/2793 disk   **/
/** controller produced by Western Digital. See WD1793.h    **/
/** for declarations.                                       **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2005-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "WD1793.h"
#include <stdio.h>

/** Reset1793() **********************************************/
/** Reset WD1793. When Disks=WD1793_INIT, also initialize   **/
/** disks. When Disks=WD1793_EJECT, eject inserted disks,   **/
/** freeing memory.                                         **/
/*************************************************************/
void Reset1793(register WD1793 *D,FDIDisk *Disks,register byte Eject)
{
  int J;

  D->R[0]     = 0x00;
  D->R[1]     = 0x00;
  D->R[2]     = 0x00;
  D->R[3]     = 0x00;
  D->R[4]     = S_RESET|S_HALT;
  D->Drive    = 0;
  D->Side     = 0;
  D->LastS    = 0;
  D->IRQ      = 0;
  D->WRLength = 0;
  D->RDLength = 0;
  D->Wait     = 0;
  D->Cmd      = 0xD0;

  /* For all drives... */
  for(J=0;J<4;++J)
  {
    /* Reset drive-dependent state */
    D->Disk[J]  = Disks? &Disks[J]:0;
    D->Track[J] = 0;
    /* Initialize disk structure, if requested */
    if((Eject==WD1793_INIT)&&D->Disk[J])  InitFDI(D->Disk[J]);
    /* Eject disk image, if requested */
    if((Eject==WD1793_EJECT)&&D->Disk[J]) EjectFDI(D->Disk[J]);
  }
}

/** Read1793() ***********************************************/
/** Read value from a WD1793 register A. Returns read data  **/
/** on success or 0xFF on failure (bad register address).   **/
/*************************************************************/
byte Read1793(register WD1793 *D,register byte A)
{
  switch(A)
  {
    case WD1793_STATUS:
      A=D->R[0];
      /* If no disk present, set F_NOTREADY */
      if(!D->Disk[D->Drive]||!D->Disk[D->Drive]->Data) A|=F_NOTREADY;
      if(D->Cmd<0x80)
      {
        /* Keep flipping F_INDEX bit as the disk rotates (Sam Coupe) */
        D->R[0]=(D->R[0]^F_INDEX)&(F_INDEX|F_BUSY|F_NOTREADY|F_READONLY|F_TRACK0);
      }
      else
      {
        /* When reading status, clear all bits but F_BUSY and F_NOTREADY */
        D->R[0]&=F_BUSY|F_NOTREADY|F_READONLY|F_DRQ;
      }
      return(A);
    case WD1793_TRACK:
    case WD1793_SECTOR:
      /* Return track/sector numbers */
      return(D->R[A]);
    case WD1793_DATA:
      /* When reading data, load value from disk */
      if(!D->RDLength)
      { if(D->Verbose) printf("WD1793: EXTRA DATA READ\n"); }
      else
      {
        /* Read data */
        D->R[A]=*D->Ptr++;
        /* Decrement length */
        if(--D->RDLength)
        {
          /* Reset timeout watchdog */
          D->Wait=255;
          /* Advance to the next sector as needed */
          if(!(D->RDLength&(D->Disk[D->Drive]->SecSize-1))) ++D->R[2];
        }
        else
        {
          /* Read completed */
          if(D->Verbose) printf("WD1793: READ COMPLETED\n");
          D->R[0]&= ~(F_DRQ|F_BUSY);
          D->IRQ  = WD1793_IRQ;
        }
      }
      return(D->R[A]);
    case WD1793_READY:
      /* After some idling, stop read/write operations */
      if(D->Wait)
        if(!--D->Wait)
        {
          if(D->Verbose) printf("WD1793: COMMAND TIMED OUT\n");
          D->RDLength=D->WRLength=0;
          D->R[0] = (D->R[0]&~(F_DRQ|F_BUSY))|F_LOSTDATA;
          D->IRQ  = WD1793_IRQ;
        }
      /* Done */
      return(D->IRQ);
  }

  /* Bad register */
  return(0xFF);
}

/** Write1793() **********************************************/
/** Write value V into WD1793 register A. Returns DRQ/IRQ   **/
/** values.                                                 **/
/*************************************************************/
byte Write1793(register WD1793 *D,register byte A,register byte V)
{
  int J;

  switch(A)
  {
    case WD1793_COMMAND:
      /* Reset interrupt request */
      D->IRQ=0;
      /* If it is FORCE-IRQ command... */
      if((V&0xF0)==0xD0)
      {
        if(D->Verbose) printf("WD1793: FORCE-INTERRUPT (%02Xh)\n",V);
        /* Reset any executing command */
        D->RDLength=D->WRLength=0;
        D->Cmd=0xD0;
        /* Either reset BUSY flag or reset all flags if BUSY=0 */
        if(D->R[0]&F_BUSY) D->R[0]&=~F_BUSY;
        else               D->R[0]=D->Track[D->Drive]? 0:F_TRACK0;
        /* Cause immediate interrupt if requested */
        if(V&C_IRQ) D->IRQ=WD1793_IRQ;
        /* Done */
        return(D->IRQ);
      }
      /* If busy, drop out */
      if(D->R[0]&F_BUSY) break;
      /* Reset status register */
      D->R[0]=0x00;
      D->Cmd=V;
      /* Depending on the command... */
      switch(V&0xF0)
      {
        case 0x00: /* RESTORE (seek track 0) */
          if(D->Verbose) printf("WD1793: RESTORE-TRACK-0 (%02Xh)\n",V);
          D->Track[D->Drive]=0;
          D->R[0] = F_INDEX|F_TRACK0|(V&C_LOADHEAD? F_HEADLOAD:0);
          D->R[1] = 0;
          D->IRQ  = WD1793_IRQ;
          break;

        case 0x10: /* SEEK */
          if(D->Verbose) printf("WD1793: SEEK-TRACK %d (%02Xh)\n",D->R[3],V);
          /* Reset any executing command */
          D->RDLength=D->WRLength=0;
          D->Track[D->Drive]=D->R[3];
          D->R[0] = F_INDEX
                  | (D->Track[D->Drive]? 0:F_TRACK0)
                  | (V&C_LOADHEAD? F_HEADLOAD:0);
          D->R[1] = D->Track[D->Drive];
          D->IRQ  = WD1793_IRQ;
          break;

        case 0x20: /* STEP */
        case 0x30: /* STEP-AND-UPDATE */
        case 0x40: /* STEP-IN */
        case 0x50: /* STEP-IN-AND-UPDATE */
        case 0x60: /* STEP-OUT */
        case 0x70: /* STEP-OUT-AND-UPDATE */
          if(D->Verbose) printf("WD1793: STEP%s%s (%02Xh)\n",
            V&0x40? (V&0x20? "-OUT":"-IN"):"",
            V&0x10? "-AND-UPDATE":"",
            V
          );
          /* Either store or fetch step direction */
          if(V&0x40) D->LastS=V&0x20; else V=(V&~0x20)|D->LastS;
          /* Step the head, update track register if requested */
          if(V&0x20) { if(D->Track[D->Drive]) --D->Track[D->Drive]; }
          else ++D->Track[D->Drive];
          /* Update track register if requested */
          if(V&C_SETTRACK) D->R[1]=D->Track[D->Drive];
          /* Update status register */
          D->R[0] = F_INDEX|(D->Track[D->Drive]? 0:F_TRACK0);
// @@@ WHY USING J HERE?
//                  | (J&&(V&C_VERIFY)? 0:F_SEEKERR);
          /* Generate IRQ */
          D->IRQ  = WD1793_IRQ;          
          break;

        case 0x80:
        case 0x90: /* READ-SECTORS */
          if(D->Verbose) printf("WD1793: READ-SECTOR%s %c:%d:%d:%d (%02Xh)\n",V&0x10? "S":"",D->Drive+'A',D->Side,D->R[1],D->R[2],V);
          /* Seek to the requested sector */
          D->Ptr=SeekFDI(
            D->Disk[D->Drive],D->Side,D->Track[D->Drive],
            V&C_SIDECOMP? !!(V&C_SIDE):D->Side,D->R[1],D->R[2]
          );
          /* If seek successful, set up reading operation */
          if(!D->Ptr)
          {
            if(D->Verbose) printf("WD1793: READ ERROR\n");
            D->R[0]     = (D->R[0]&~F_ERRCODE)|F_NOTFOUND;
            D->IRQ      = WD1793_IRQ;
          }
          else
          {
            D->RDLength = D->Disk[D->Drive]->SecSize
                        * (V&0x10? (D->Disk[D->Drive]->Sectors-D->R[2]+1):1);
            D->R[0]    |= F_BUSY|F_DRQ;
            D->IRQ      = WD1793_DRQ;
            D->Wait     = 255;
          }
          break;

        case 0xA0:
        case 0xB0: /* WRITE-SECTORS */
          if(D->Verbose) printf("WD1793: WRITE-SECTOR%s %c:%d:%d:%d (%02Xh)\n",V&0x10? "S":"",'A'+D->Drive,D->Side,D->R[1],D->R[2],V);
          /* Seek to the requested sector */
          D->Ptr=SeekFDI(
            D->Disk[D->Drive],D->Side,D->Track[D->Drive],
            V&C_SIDECOMP? !!(V&C_SIDE):D->Side,D->R[1],D->R[2]
          );
          /* If seek successful, set up writing operation */
          if(!D->Ptr)
          {
            if(D->Verbose) printf("WD1793: WRITE ERROR\n");
            D->R[0]     = (D->R[0]&~F_ERRCODE)|F_NOTFOUND;
            D->IRQ      = WD1793_IRQ;
          }
          else
          {
            D->WRLength = D->Disk[D->Drive]->SecSize
                        * (V&0x10? (D->Disk[D->Drive]->Sectors-D->R[2]+1):1);
            D->R[0]    |= F_BUSY|F_DRQ;
            D->IRQ      = WD1793_DRQ;
            D->Wait     = 255;
          }
          break;

        case 0xC0: /* READ-ADDRESS */
          if(D->Verbose) printf("WD1793: READ-ADDRESS (%02Xh)\n",V);
          /* Read first sector address from the track */
          if(!D->Disk[D->Drive]) D->Ptr=0;
          else
            for(J=0;J<256;++J)
            {
              D->Ptr=SeekFDI(
                D->Disk[D->Drive],
                D->Side,D->Track[D->Drive],
                D->Side,D->Track[D->Drive],J
              );
              if(D->Ptr) break;
            }
          /* If address found, initiate data transfer */
          if(!D->Ptr)
          {
            if(D->Verbose) printf("WD1793: READ-ADDRESS ERROR\n");
            D->R[0]    |= F_NOTFOUND;
            D->IRQ      = WD1793_IRQ;
          }
          else
          {
            D->Ptr      = D->Disk[D->Drive]->Header;
            D->RDLength = 6;
            D->R[0]    |= F_BUSY|F_DRQ;
            D->IRQ      = WD1793_DRQ;
            D->Wait     = 255;
          }
          break;

        case 0xE0: /* READ-TRACK */
          if(D->Verbose) printf("WD1793: READ-TRACK %d (%02Xh) UNSUPPORTED!\n",D->R[1],V);
          break;

        case 0xF0: /* WRITE-TRACK */
          if(D->Verbose) printf("WD1793: WRITE-TRACK %d (%02Xh) UNSUPPORTED!\n",D->R[1],V);
          break;

        default: /* UNKNOWN */
          if(D->Verbose) printf("WD1793: UNSUPPORTED OPERATION %02Xh!\n",V);
          break;
      }
      break;

    case WD1793_TRACK:
    case WD1793_SECTOR:
      if(!(D->R[0]&F_BUSY)) D->R[A]=V;
      break;

    case WD1793_SYSTEM:
// @@@ Too verbose
//      if(D->Verbose) printf("WD1793: Drive %c, %cD side %d\n",'A'+(V&S_DRIVE),V&S_DENSITY? 'D':'S',V&S_SIDE? 0:1);
      /* Reset controller if S_RESET goes up */
      if((D->R[4]^V)&V&S_RESET) Reset1793(D,D->Disk[0],WD1793_KEEP);
      /* Set disk #, side #, ignore the density (@@@) */
      D->Drive = V&S_DRIVE;
      D->Side  = !(V&S_SIDE);
      /* Save last written value */
      D->R[4]  = V;
      break;

    case WD1793_DATA:
      /* When writing data, store value to disk */
      if(!D->WRLength)
      { if(D->Verbose) printf("WD1793: EXTRA DATA WRITE (%02Xh)\n",V); }
      else
      {
        /* Write data */
        *D->Ptr++=V;
        /* Decrement length */
        if(--D->WRLength)
        {
          /* Reset timeout watchdog */
          D->Wait=255;
          /* Advance to the next sector as needed */
          if(!(D->WRLength&(D->Disk[D->Drive]->SecSize-1))) ++D->R[2];
        }
        else
        {
          /* Write completed */
          if(D->Verbose) printf("WD1793: WRITE COMPLETED\n");
          D->R[0]&= ~(F_DRQ|F_BUSY);
          D->IRQ  = WD1793_IRQ;
        }
      }
      /* Save last written value */
      D->R[A]=V;
      break;
  }

  /* Done */
  return(D->IRQ);
}
