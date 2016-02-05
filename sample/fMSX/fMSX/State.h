/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                         State.h                         **/
/**                                                         **/
/** This file contains routines to save and load emulation  **/
/** state.                                                  **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2015                 **/
/**     The contents of this file are property of Marat     **/
/**     Fayzullin and should only be used as agreed with    **/
/**     him. The file is confidential. Absolutely no        **/
/**     distribution allowed.                               **/
/*************************************************************/
#ifndef STATE_H
#define STATE_H

#define SaveSTRUCT(Name) \
  if(Size+sizeof(Name)>MaxSize) return(0); \
  else { memcpy(Buf+Size,&(Name),sizeof(Name));Size+=sizeof(Name); }

#define SaveARRAY(Name) \
  if(Size+sizeof(Name)>MaxSize) return(0); \
  else { memcpy(Buf+Size,(Name),sizeof(Name));Size+=sizeof(Name); }

#define SaveDATA(Name,DataSize) \
  if(Size+(DataSize)>MaxSize) return(0); \
  else { memcpy(Buf+Size,(Name),(DataSize));Size+=(DataSize); }

#define LoadSTRUCT(Name) \
  if(Size+sizeof(Name)>MaxSize) return(0); \
  else { memcpy(&(Name),Buf+Size,sizeof(Name));Size+=sizeof(Name); }

#define SkipSTRUCT(Name) \
  if(Size+sizeof(Name)>MaxSize) return(0); \
  else Size+=sizeof(Name)

#define LoadARRAY(Name) \
  if(Size+sizeof(Name)>MaxSize) return(0); \
  else { memcpy((Name),Buf+Size,sizeof(Name));Size+=sizeof(Name); }

#define LoadDATA(Name,DataSize) \
  if(Size+(DataSize)>MaxSize) return(0); \
  else { memcpy((Name),Buf+Size,(DataSize));Size+=(DataSize); }

#define SkipDATA(DataSize) \
  if(Size+(DataSize)>MaxSize) return(0); \
  else Size+=(DataSize)

/** SaveState() **********************************************/
/** Save emulation state to a memory buffer. Returns size   **/
/** on success, 0 on failure.                               **/
/*************************************************************/
unsigned int SaveState(unsigned char *Buf,unsigned int MaxSize)
{
  unsigned int State[256],Size;
  int J,I,K;

  /* No data written yet */
  Size = 0;

  /* Fill out hardware state */
  J=0;
  memset(State,0,sizeof(State));
  State[J++] = VDPData;
  State[J++] = PLatch;
  State[J++] = ALatch;
  State[J++] = VAddr;
  State[J++] = VKey;
  State[J++] = PKey;
  State[J++] = WKey;
  State[J++] = IRQPending;
  State[J++] = ScanLine;
  State[J++] = RTCReg;
  State[J++] = RTCMode;
  State[J++] = KanLetter;
  State[J++] = KanCount;
  State[J++] = IOReg;
  State[J++] = PSLReg;
  State[J++] = FMPACKey;

  /* Memory setup */
  for(I=0;I<4;++I)
  {
    State[J++] = SSLReg[I];
    State[J++] = PSL[I];
    State[J++] = SSL[I];
    State[J++] = EnWrite[I];
    State[J++] = RAMMapper[I];
  }  

  /* Cartridge setup */
  for(I=0;I<MAXSLOTS;++I)
  {
    State[J++] = ROMType[I];
    for(K=0;K<4;++K) State[J++]=ROMMapper[I][K];
  }

  /* Write out data structures */
  SaveSTRUCT(CPU);
  SaveSTRUCT(PPI);
  SaveSTRUCT(VDP);
  SaveARRAY(VDPStatus);
  SaveARRAY(Palette);
  SaveSTRUCT(PSG);
  SaveSTRUCT(OPLL);
  SaveSTRUCT(SCChip);
  SaveARRAY(State);
  SaveDATA(RAMData,RAMPages*0x4000);
  SaveDATA(VRAM,VRAMPages*0x4000);

  /* Return amount of data written */
  return(Size);
}

/** LoadState() **********************************************/
/** Load emulation state from a memory buffer. Returns size **/
/** on success, 0 on failure.                               **/
/*************************************************************/
unsigned int LoadState(unsigned char *Buf,unsigned int MaxSize)
{
  int State[256],J,I,K;
  unsigned int Size;

  /* No data read yet */
  Size = 0;

  /* Load hardware state */
  LoadSTRUCT(CPU);
  LoadSTRUCT(PPI);
  LoadSTRUCT(VDP);
  LoadARRAY(VDPStatus);
  LoadARRAY(Palette);
  LoadSTRUCT(PSG);
  LoadSTRUCT(OPLL);
  LoadSTRUCT(SCChip);
  LoadARRAY(State);
  LoadDATA(RAMData,RAMPages*0x4000);
  LoadDATA(VRAM,VRAMPages*0x4000);

  /* Parse hardware state */
  J=0;
  VDPData    = State[J++];
  PLatch     = State[J++];
  ALatch     = State[J++];
  VAddr      = State[J++];
  VKey       = State[J++];
  PKey       = State[J++];
  WKey       = State[J++];
  IRQPending = State[J++];
  ScanLine   = State[J++];
  RTCReg     = State[J++];
  RTCMode    = State[J++];
  KanLetter  = State[J++];
  KanCount   = State[J++];
  IOReg      = State[J++];
  PSLReg     = State[J++];
  FMPACKey   = State[J++];

  /* Memory setup */
  for(I=0;I<4;++I)
  {
    SSLReg[I]       = State[J++];
    PSL[I]          = State[J++];
    SSL[I]          = State[J++];
    EnWrite[I]      = State[J++];
    RAMMapper[I]    = State[J++];
  }  

  /* Cartridge setup */
  for(I=0;I<MAXSLOTS;++I)
  {
    ROMType[I] = State[J++];
    for(K=0;K<4;++K) ROMMapper[I][K]=State[J++];
  }

  /* Set RAM mapper pages */
  if(RAMMask)
    for(I=0;I<4;++I)
    {
      RAMMapper[I]       &= RAMMask;
      MemMap[3][2][I*2]   = RAMData+RAMMapper[I]*0x4000;
      MemMap[3][2][I*2+1] = MemMap[3][2][I*2]+0x2000;
    }

  /* Set ROM mapper pages */
  for(I=0;I<MAXSLOTS;++I)
    if(ROMData[I]&&ROMMask[I])
      SetMegaROM(I,ROMMapper[I][0],ROMMapper[I][1],ROMMapper[I][2],ROMMapper[I][3]);

  /* Set main address space pages */
  for(I=0;I<4;++I)
  {
    RAM[2*I]   = MemMap[PSL[I]][SSL[I]][2*I];
    RAM[2*I+1] = MemMap[PSL[I]][SSL[I]][2*I+1];
  }

  /* Set palette */
  for(I=0;I<16;++I)
    SetColor(I,(Palette[I]>>16)&0xFF,(Palette[I]>>8)&0xFF,Palette[I]&0xFF);

  /* Set screen mode and VRAM table addresses */
  SetScreen();

  /* Set some other variables */
  VPAGE    = VRAM+((int)VDP[14]<<14);
  FGColor  = VDP[7]>>4;
  BGColor  = VDP[7]&0x0F;
  XFGColor = FGColor;
  XBGColor = BGColor;

  /* All sound channels could have been changed */
  PSG.Changed     = (1<<AY8910_CHANNELS)-1;
  SCChip.Changed  = (1<<SCC_CHANNELS)-1;
  SCChip.WChanged = (1<<SCC_CHANNELS)-1;
  OPLL.Changed    = (1<<YM2413_CHANNELS)-1;
  OPLL.PChanged   = (1<<YM2413_CHANNELS)-1;
  OPLL.DChanged   = (1<<YM2413_CHANNELS)-1;

  /* Return amount of data read */
  return(Size);
}

/** SaveSTA() ************************************************/
/** Save emulation state into a .STA file. Returns 1 on     **/
/** success, 0 on failure.                                  **/
/*************************************************************/
int SaveSTA(const char *Name)
{
  static byte Header[16] = "STE\032\003\0\0\0\0\0\0\0\0\0\0\0";
  unsigned int J,Size;
  byte *Buf;
  FILE *F;

  /* Fail if no state file */
  if(!Name) return(0);

  /* Allocate temporary buffer */
  Buf = malloc(MAX_STASIZE);
  if(!Buf) return(0);

  /* Try saving state */
  Size = SaveState(Buf,MAX_STASIZE);
  if(!Size) { free(Buf);return(0); }

  /* Open new state file */
  F = fopen(Name,"wb");
  if(!F) { free(Buf);return(0); }

  /* Prepare the header */
  J=StateID();
  Header[5] = RAMPages;
  Header[6] = VRAMPages;
  Header[7] = J&0x00FF;
  Header[8] = J>>8;

  /* Write out the header and the data */
  if(F && (fwrite(Header,1,16,F)!=16))  { fclose(F);F=0; }
  if(F && (fwrite(Buf,1,Size,F)!=Size)) { fclose(F);F=0; }

  /* If failed writing state, delete open file */
  if(F) fclose(F); else unlink(Name);

  /* Done */
  free(Buf);
  return(!!F);
}

/** LoadSTA() ************************************************/
/** Load emulation state from a .STA file. Returns 1 on     **/
/** success, 0 on failure.                                  **/
/*************************************************************/
int LoadSTA(const char *Name)
{
  int Size,OldMode,OldRAMPages,OldVRAMPages;
  byte Header[16],*Buf;
  FILE *F;

  /* Fail if no state file */
  if(!Name) return(0);

  /* Open saved state file */
  if(!(F=fopen(Name,"rb"))) return(0);

  /* Read and check the header */
  if(fread(Header,1,16,F)!=16)           { fclose(F);return(0); }
  if(memcmp(Header,"STE\032\003",5))     { fclose(F);return(0); }
  if(Header[7]+Header[8]*256!=StateID()) { fclose(F);return(0); }
  if((Header[5]!=(RAMPages&0xFF))||(Header[6]!=(VRAMPages&0xFF)))
  { fclose(F);return(0); }

  /* Allocate temporary buffer */
  Buf = malloc(MAX_STASIZE);
  if(!Buf) { fclose(F);return(0); }

  /* Save current configuration */
  OldMode      = Mode;
  OldRAMPages  = RAMPages;
  OldVRAMPages = VRAMPages;

  /* Read state into temporary buffer, then load it */
  Size = fread(Buf,1,MAX_STASIZE,F);
  Size = Size>0? LoadState(Buf,Size):0;

  /* If failed loading state, reset hardware */
  if(!Size) ResetMSX(OldMode,OldRAMPages,OldVRAMPages);

  /* Done */
  free(Buf);
  fclose(F);
  return(!!Size);
}

#endif /* STATE_H */
