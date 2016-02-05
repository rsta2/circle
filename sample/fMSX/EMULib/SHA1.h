#ifndef SHA1_H
#define SHA1_H

typedef struct
{
  unsigned int  Done;
  unsigned int  Error;
  unsigned int  LenL,LenH;
  unsigned int  Ptr;
  unsigned char Buf[64];
  unsigned int  Msg[5];
} SHA1;

void ResetSHA1(SHA1 *State);
int ComputeSHA1(SHA1 *State);
int InputSHA1(SHA1 *State,const unsigned char *Data,unsigned int Size);

#endif /* SHA1_H */
