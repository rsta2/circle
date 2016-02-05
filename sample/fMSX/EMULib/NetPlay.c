/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        NetPlay.c                        **/
/**                                                         **/
/** This file contains platform-independent part of the     **/
/** NetPlay implementation. See NetPlay.h for declarations. **/
/** Platform specific implementation for platform XXX goes  **/
/** into NetXXX.c.                                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"
#include "Console.h"
#include "NetPlay.h"
#include <stdio.h>
#include <string.h>

/** Dummy NET*() Functions ***********************************/
/** These dummy functions are for systems where networking  **/
/** is not implemented.                                     **/
/*************************************************************/
#if !defined(WINDOWS) && !defined(UNIX) && !defined(ANDROID) && !defined(MAEMO) && !defined(MEEGO) && !defined(S60) && !defined(UIQ)
const char *NETMyName(char *Buffer,int MaxChars) { return(0); }
int  NETConnect(const char *Server,unsigned short Port) { return(NET_OFF); }
int  NETConnectAsync(const char *Server,unsigned short Port) { return(0); }
int  NETSend(const char *Out,int N) { return(0); }
int  NETRecv(char *In,int N) { return(0); }
int  NETBlock(int Switch) { return(NET_OFF); }
int  NETConnected(void) { return(NET_OFF); }
void NETClose(void) { }
#endif

/** NETPlay() ************************************************/
/** Connect/disconnect NetPlay interface, toggle or query   **/
/** connection status (use NET_* as argument). Returns the  **/
/** NetPlay connection status.                              **/
/*************************************************************/
int NETPlay(int Switch)
{
  char S[256],T[256],*P;
  int J;

  // Toggle connection if requested
  if(Switch==NET_TOGGLE) Switch=NETConnected()? NET_OFF:NET_ON;

  switch(Switch)
  {
    case NET_OFF:
      // Disconnect NetPlay
      NETClose();
      break;
    case NET_ON:
    case NET_SERVER:
      // If already connected, drop out
      if(NETConnected()==Switch) break;
      // If connecting in client mode, ask for hostname
      if((Switch==NET_SERVER)||CONInput(
        -1,-1,NET_FGCOLOR,NET_BGCOLOR,
        "Enter Hostname",S,sizeof(S)
      ))
      {
        // Compose title
        T[0]='~';
        if(!NETMyName(T+1,sizeof(T)-1)) strcpy(T+1,"Network Play");
        // Show "Connecting" message
        CONMsg(-1,-1,-1,-1,NET_FGCOLOR,NET_OKCOLOR,T," \0 Connecting... \0 \0");
        ShowVideo();
        // Convert string to lower case, remove spaces
        if(Switch==NET_SERVER)
        {
          for(J=0,P=S;*P;++P)
            if(*P>' ') S[J++]=(*P>='A')&&(*P<='Z')? *P+'a'-'A':*P;
          S[J]='\0';
        }
        // Connect second player, report problems if present
        if(!NETConnect(Switch==NET_SERVER? 0:S,NET_PORT))
          CONMsg(
            -1,-1,-1,-1,NET_FGCOLOR,NET_ERRCOLOR,
            T+1," \0    Failed!    \0 \0"
          );
      }
      break;
  }

  // Always return connection status
  return(NETConnected());
}

/** NETExchange() ********************************************/
/** Exchanges given number of bytes with the other side and **/
/** returns 1 on success, 0 on failure.                     **/
/*************************************************************/
int NETExchange(char *In,const char *Out,int N)
{
  /* Send data */
  if(NETSend(Out,N)!=N) return(0);
  /* Receive data */
  if(NETRecv(In,N)!=N) return(0);
  /* Success */
  return(1);
}

/** NETJoystick() ********************************************/
/** When NetPlay disconnected, returns GetJoystick(). Else, **/
/** exchanges joystick states over the network and presents **/
/** server as player #1, client as player #2.               **/
/*************************************************************/
unsigned int NETJoystick(void)
{
  unsigned int J,I;

  /* Get local joystick state */
  J = GetJoystick();

  /* When ALT pressed, swap players back */
  if(J&BTN_ALT) J=(J&BTN_MODES)|((J>>16)&BTN_ALL)|((J&BTN_ALL)<<16);

  /* If exchanged joystick states over the network... */
  /* Assume server to be player #1, client to be player #2 */
  if(NETExchange((char *)&I,(const char *)&J,sizeof(J)))
    J = NETConnected()==NET_SERVER?
        ((J&(BTN_ALL|BTN_MODES))|((I&BTN_ALL)<<16))
      : ((I&(BTN_ALL|BTN_MODES))|((J&BTN_ALL)<<16));

  /* Done */
  return(J);
}
