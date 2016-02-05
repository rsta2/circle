/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         NetUnix.c                       **/
/**                                                         **/
/** This file contains standard communication routines for  **/
/** Unix using TCP/IP via sockets.                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1997-2014                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"
#include "NetPlay.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <pthread.h>

#ifdef ANDROID
#define puts   LOGI
#define printf LOGI
#endif

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

static volatile int IsServer  = 0;
static volatile int Socket    = -1;
static volatile int Blocking  = 1;
static volatile int UseUDP    = 0;
static volatile pthread_t Thr = 0;
static struct sockaddr_in PeerAddr;

/** ThrHandler() *********************************************/
/** This is the thread function responsible for asyncronous **/
/** connection process.                                     **/
/*************************************************************/
static void *ThrHandler(void *Arg)
{
  void **Args  = (void **)Arg;
  char *Server = (char *)Args[0];
  unsigned int Port = (unsigned int)Args[1];

  /* Try connecting */
  NETConnect(Server,Port);

  /* No longer need strdup()ed server name */
  if(Server) free(Server);

  /* Thread done */
  Thr = 0;
}

/** NETConnected() *******************************************/
/** Returns NET_SERVER, NET_CLIENT, or NET_OFF.             **/
/*************************************************************/
int NETConnected(void)
{ return(Socket<0? NET_OFF:IsServer? NET_SERVER:NET_CLIENT); }

/** NETMyName() **********************************************/
/** Returns local hostname/address or 0 on failure.         **/
/*************************************************************/
const char *NETMyName(char *Buffer,int MaxChars)
{
  struct hostent *Host;

  /* Have to have enough characters */
  if(MaxChars<16) return(0);
  /* Get local hostname */
  gethostname(Buffer,MaxChars);
  /* Look up address */
  if(!(Host=gethostbyname(Buffer))) return(0);
  /* Must have an address */
  if(!Host->h_addr_list||!Host->h_addr_list[0]) return(0);
  /* Copy address */
  sprintf(Buffer,"%d.%d.%d.%d",
    Host->h_addr_list[0][0],
    Host->h_addr_list[0][1],
    Host->h_addr_list[0][2],
    Host->h_addr_list[0][3]
  );
  return(Buffer);
}

/** NETConnectAsync() ****************************************/
/** Asynchronous version of NETConnect().                   **/
/*************************************************************/
int NETConnectAsync(const char *Server,unsigned short Port)
{
  static void *Args[2];

  /* Thread already exists, nothing to do */
  if(Thr) return(1);

  /* Close existing network connection */
  NETClose();

  /* Set up arguments */
  Args[0] = (void *)(Server? strdup(Server):0);
  Args[1] = (void *)(unsigned int)Port;
  if(!Args[0]&&Server) return(0);

  /* Create connection thread */
  return(!!pthread_create((pthread_t *)&Thr,0,ThrHandler,Args));
}

/** UDPConnect() *********************************************/
/** Connects to Server:Port. When Server=0, becomes server, **/
/** waits at for an incoming request, and connects. Returns **/
/** the NetPlay connection status, like NETConnected().     **/
/*************************************************************/
int UDPConnect(const char *Server,unsigned short Port)
{
  struct sockaddr_in MyAddr;
  struct hostent *Host;
  int SSocket,J;
  pthread_t T;

  /* Store current thread ID (or 0 if called synchronously) */
  T = Thr;

  /* Close existing network connection */
  if(!T) NETClose();

  /* Create UDP socket */
  if((SSocket=socket(AF_INET,SOCK_DGRAM,0))<0) return(NET_OFF);

  /* Set fields of the address structure */
  memset(&MyAddr,0,sizeof(MyAddr));
  MyAddr.sin_family      = AF_INET;
  MyAddr.sin_port        = htons(Port);
  MyAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  memcpy(&PeerAddr,&MyAddr,sizeof(PeerAddr));

  /* If the server name is given, we first try to */
  /* connect as a client.                         */
  if(Server)
  {
    printf("UDP-CLIENT: Looking up '%s'...\n",Server);

    /* Look up server address */
    if(!(Host=gethostbyname(Server))) { close(SSocket);return(NET_OFF); }

    /* Set server address */
    memcpy(&PeerAddr.sin_addr,Host->h_addr,Host->h_length);

    /* Bind socket */
    if(bind(SSocket,(struct sockaddr *)&MyAddr,sizeof(MyAddr))<0)
    { close(SSocket);return(NET_OFF); }

    printf("UDP-CLIENT: Client established.\n");
  }
  else
  {
    printf("UDP-SERVER: Becoming server...\n");

    /* Accepting messages from any address */
    PeerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Bind socket */
    if(bind(SSocket,(struct sockaddr *)&MyAddr,sizeof(MyAddr))<0)
    { close(SSocket);return(NET_OFF); }

    printf("UDP-SERVER: Server established.\n");
  }

  /* Make communication socket blocking/non-blocking */
  J=!Blocking;
  if(ioctl(SSocket,FIONBIO,&J)<0) { close(SSocket);return(NET_OFF); }

  /* Success */
  IsServer = !Server;
  UseUDP   = 1;
  Socket   = SSocket;

  return(Server? NET_SERVER:NET_CLIENT);
}

/** NETConnect() *********************************************/
/** Connects to Server:Port. When Server=0, becomes server, **/
/** waits at for an incoming request, and connects. Returns **/
/** the NetPlay connection status, like NETConnected().     **/
/*************************************************************/
int NETConnect(const char *Server,unsigned short Port)
{
  struct sockaddr_in Addr;
  struct hostent *Host;
  struct timeval TV;
  int AddrLength,LSocket,SSocket;
  unsigned long J;
  pthread_t T;
  fd_set FDs;

  /* Store current thread ID (or 0 if called synchronously) */
  T = Thr;

  /* Close existing network connection */
  if(!T) NETClose();

  /* Clear the address structure */
  memset(&Addr,0,sizeof(Addr));

  /* If the server name is given, we first try to */
  /* connect as a client.                         */
  if(Server)
  {
    printf("NET-CLIENT: Connecting to '%s'...\n",Server);

    /* Look up server address */
    if(!(Host=gethostbyname(Server))) return(NET_OFF);

    printf("NET-CLIENT: Got server's IP address...\n");

    /* Set fields of the address structure */
    memcpy(&Addr.sin_addr,Host->h_addr,Host->h_length);
    Addr.sin_family = AF_INET;
    Addr.sin_port   = htons(Port);

    /* Create a socket */
    if((SSocket=socket(AF_INET,SOCK_STREAM,0))<0) return(NET_OFF);

    printf("NET-CLIENT: Created socket...\n");

    /* Connecting... */
    if(connect(SSocket,(struct sockaddr *)&Addr,sizeof(Addr))>=0)
    {
      printf("NET-CLIENT: Connected to the server...\n");

      /* Make communication socket blocking/non-blocking */
      J=!Blocking;
      if(ioctl(SSocket,FIONBIO,&J)<0)
      { close(SSocket);return(NET_OFF); }

      /* Disable Nagle algorithm */
      J=1;
      setsockopt(SSocket,SOL_TCP,TCP_NODELAY,&J,sizeof(J));

      /* Succesfully connected as client */
      IsServer = 0;
      UseUDP   = 0;
      Socket   = SSocket;

      printf("NET-CLIENT: Connected to '%s'.\n",Server);

      return(NET_CLIENT);
    }

    printf("NET-CLIENT: Failed connecting to '%s'.\n",Server);

    /* Failed to connect as a client */
    close(SSocket);

#ifdef ANDROID
    /* Do not try becoming server! */
    return(NET_OFF);
#endif
  }

  /* Connection as client either failed or hasn't */
  /* been attempted at all. Becoming a server and */
  /* waiting for connection request.              */

  printf("NET-SERVER: Becoming server...\n");

  /* Set fields of the address structure */
  Addr.sin_addr.s_addr = htonl(INADDR_ANY);
  Addr.sin_family      = AF_INET;
  Addr.sin_port        = htons(Port);

  /* Create a listening socket */
  if((LSocket=socket(AF_INET,SOCK_STREAM,0))<0) return(NET_OFF);

  printf("NET-SERVER: Created socket...\n");

  /* Bind listening socket */
  if(bind(LSocket,(struct sockaddr *)&Addr,sizeof(Addr))<0)
  { close(LSocket);return(NET_OFF); }

  printf("NET-SERVER: Bound socket...\n");

  /* Listen for one client */
  if(listen(LSocket,1)<0)
  { close(LSocket);return(NET_OFF); }

  /* We will need address length */
  AddrLength=sizeof(Addr);

  printf("NET-SERVER: Accepting calls...\n");

  /* No sockets yet */
  FD_ZERO(&FDs);

#ifdef ANDROID
  /* Accepting calls... */
  for(SSocket=-1;(SSocket<0)&&VideoImg&&(T==Thr);)
  {
    /* Prepare data for select() */
    FD_SET(LSocket,&FDs);
    TV.tv_sec  = 1;
    TV.tv_usec = 0;
    /* Listen and accept connection */
    if(select(LSocket+1,&FDs,0,0,&TV)>0)
      SSocket=accept(LSocket,(struct sockaddr *)&Addr,&AddrLength);
  }
#else
  /* Accepting calls... */
  for(SSocket=-1,GetKey();(SSocket<0)&&VideoImg&&!GetKey()&&(T==Thr);)
  {
    /* Prepare data for select() */
    FD_SET(LSocket,&FDs);
    TV.tv_sec  = 0;
    TV.tv_usec = 100000;
    /* Make sure UI is still running */
    ProcessEvents(0);
    /* Listen and accept connection */
    if(select(LSocket+1,&FDs,0,0,&TV)>0)
      SSocket=accept(LSocket,(struct sockaddr *)&Addr,&AddrLength);
  }
#endif

  printf("NET-SERVER: Client %s...\n",SSocket>=0? "connected":"failed to connect");

  /* Done listening */
  close(LSocket);

  /* Client failed to connect */
  if(SSocket<0) return(NET_OFF);

  /* Make communication socket blocking/non-blocking */
  J=!Blocking;
  if(ioctl(SSocket,FIONBIO,&J)<0)
  { close(SSocket);return(NET_OFF); }

  /* Disable Nagle algorithm */
  J=1;
  setsockopt(SSocket,SOL_TCP,TCP_NODELAY,&J,sizeof(J));

  /* Client connected succesfully */
  IsServer = 1;
  UseUDP   = 0;
  Socket   = SSocket;

  printf("NET-SERVER: Client connected.\n");

  return(NET_SERVER);
}

/** NETClose() ***********************************************/
/** Closes connection open with NETConnect().               **/
/*************************************************************/
void NETClose(void)
{
  pthread_t T = Thr;

  /* If there is a connection thread running, stop it */
  if(T) { Thr=0;pthread_join(T,0); }

  if(Socket>=0) close(Socket);
  Socket   = -1;
  IsServer = 0;
}

/** NETBlock() ***********************************************/
/** Makes NETSend()/NETRecv() blocking or not blocking.     **/
/*************************************************************/
int NETBlock(int Switch)
{
  unsigned long J;

  /* Toggle blocking if requested */
  if(Switch==NET_TOGGLE) Switch=!Blocking;

  /* If switching blocking... */
  if((Switch==NET_OFF)||(Switch==NET_ON))
  {
    J=!Switch;
    if((Socket<0)||(ioctl(Socket,FIONBIO,&J)>=0)) Blocking=Switch;
  }

  /* Return blocking state */
  return(Blocking);
}

/** NETSend() ************************************************/
/** Send N bytes. Returns number of bytes sent or 0.        **/
/*************************************************************/
int NETSend(const char *Out,int N)
{
  int J,I;

  /* Have to have a socket and an address */
  if((Socket<0)||(UseUDP&&(PeerAddr.sin_addr.s_addr==INADDR_ANY))) return(0);

  /* Send data */
  for(I=J=N;(J>=0)&&I;)
  {
    J = UseUDP? sendto(Socket,Out,I,0,(struct sockaddr *)&PeerAddr,sizeof(PeerAddr)):send(Socket,Out,I,0);
    if(J>0) { Out+=J;I-=J; }
  }

  /* Return number of bytes sent */
  return(N-I);
}

/** NETRecv() ************************************************/
/** Receive N bytes. Returns number of bytes received or 0. **/
/*************************************************************/
int NETRecv(char *In,int N)
{
  socklen_t AddrLen;
  int J,I;

  /* Have to have a socket */
  if(Socket<0) return(0);

  /* This is needed for recvfrom() */
  AddrLen = sizeof(PeerAddr);

  /* Receive data */
  for(I=J=N;(J>=0)&&I;)
  {
    J = UseUDP? recvfrom(Socket,In,I,0,(struct sockaddr *)&PeerAddr,&AddrLen):recv(Socket,In,I,0);
    if(J>0) { In+=J;I-=J; }
  }

  /* Return number of bytes received */
  return(N-I);
}
