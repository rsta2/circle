/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        NetPlay.h                        **/
/**                                                         **/
/** This file contains declarations for routines that let   **/
/** people play over the network. See NetPlay.c for the     **/
/** platform-independent implementation part. Platform      **/
/** specific implementation for platform XXX goes into      **/
/** NetXXX.c.                                               **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef NETPLAY_H
#define NETPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/** NetPlay **************************************************/
/** NET_PORT is used by NetPlay to exchange joypad states   **/
/** with the other player. NET_* values are arguments to    **/
/** NETPlay().                                              **/
/*************************************************************/
#define NET_PORT     6666

#define NET_OFF      0
#define NET_ON       1
#define NET_CLIENT   1    
#define NET_SERVER   2
#define NET_TOGGLE   3
#define NET_QUERY    4

#define NET_FGCOLOR  PIXEL(255,255,255)
#define NET_BGCOLOR  PIXEL(80,40,0)
#define NET_OKCOLOR  PIXEL(0,80,0)
#define NET_ERRCOLOR PIXEL(200,0,0)

/** NETPlay() ************************************************/
/** Connect/disconnect NetPlay interface, toggle or query   **/
/** connection status (use NET_* as argument). Returns the  **/
/** NetPlay connection status, like NETConnected().         **/
/*************************************************************/
int NETPlay(int Switch);

/** NETExchange() ********************************************/
/** Exchanges given number of bytes with the other side and **/
/** returns 1 on success, 0 on failure.                     **/
/*************************************************************/
int NETExchange(char *In,const char *Out,int N);

/** NETJoystick() ********************************************/
/** When NetPlay disconnected, returns GetJoystick(). Else, **/
/** exchanges joystick states over the network and presents **/
/** server as player #1, client as player #2.               **/
/*************************************************************/
unsigned int NETJoystick(void);

/** NETConnected() *******************************************/
/** Returns NET_SERVER, NET_CLIENT, or NET_OFF.             **/
/*************************************** PLATFORM DEPENDENT **/
int NETConnected(void);

/** NETConnect() *********************************************/
/** Connects to Server:Port. When Host=0, becomes a server, **/
/** waits at for an incoming request, and connects. Returns **/
/** the NetPlay connection status, like NETConnected().     **/
/*************************************** PLATFORM DEPENDENT **/
int NETConnect(const char *Server,unsigned short Port);

/** UDPConnect() *********************************************/
/** Connects to Server:Port. When Server=0, becomes server, **/
/** waits at for an incoming request, and connects. Returns **/
/** the NetPlay connection status, like NETConnected().     **/
/*************************************** PLATFORM DEPENDENT **/
int UDPConnect(const char *Server,unsigned short Port);

/** NETClose() ***********************************************/
/** Closes connection open with NETConnect().               **/
/*************************************** PLATFORM DEPENDENT **/
void NETClose(void);

/** NETConnectAsync() ****************************************/
/** Asynchronous version of NETConnect().                   **/
/*************************************** PLATFORM DEPENDENT **/
int NETConnectAsync(const char *Server,unsigned short Port);

/** NETBlock() ***********************************************/
/** Makes NETSend()/NETRecv() blocking or not blocking.     **/
/*************************************** PLATFORM DEPENDENT **/
int NETBlock(int Switch);

/** NETSend() ************************************************/
/** Send N bytes. Returns number of bytes sent or 0.        **/
/*************************************** PLATFORM DEPENDENT **/
int NETSend(const char *Out,int N);

/** NETRecv() ************************************************/
/** Receive N bytes. Returns number of bytes received or 0. **/
/*************************************** PLATFORM DEPENDENT **/
int NETRecv(char *In,int N);

/** NETMyName() **********************************************/
/** Returns local hostname/address or 0 on failure.         **/
/*************************************** PLATFORM DEPENDENT **/
const char *NETMyName(char *Buffer,int MaxChars);

#ifdef __cplusplus
}
#endif
#endif /* NETPLAY_H */
