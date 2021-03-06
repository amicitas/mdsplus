#include <config.h>
#ifdef _WIN32
#include <ws2tcpip.h>
#define SHUT_RDWR SD_BOTH
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include <mdsshr.h>
#include <libroutines.h>
#define LOAD_INITIALIZESOCKETS
#include "mdsshrthreadsafe.h"

extern int UdpEventGetPort(unsigned short *port);
extern int UdpEventGetAddress(char **addr_format, unsigned char *arange);
extern int UdpEventGetTtl(char *ttl);
extern int UdpEventGetLoop(char *loop);
extern int UdpEventGetInterface(struct in_addr **interface_addr);

#define MAX_MSG_LEN 4096
#define MAX_EVENTS 1000000	/*Maximum number of events handled by a single process */

typedef struct _EventList {
  int eventid;
  pthread_t thread;
  int socket;
  struct _EventList *next;
} EventList;

struct EventInfo {
  int socket;
  char *eventName;
  void *arg;
  void (*astadr) (void *, int, char *);
};

static int EVENTID = 0;
static EventList *EVENTLIST = 0;
static int sendSocket = 0;
static pthread_mutex_t eventIdMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sendEventMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t getSocketMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t initializeMutex = PTHREAD_MUTEX_INITIALIZER;

extern void InitializeEventSettings();



/**********************
 Message structure:

- event name len (4 byte int)
- event name (event name len bytes)
- buf len (4 byte int)
- buf (buf len chars)

***********************/

static void *handleMessage(void *info_in)
{
  struct EventInfo *info = (struct EventInfo *)info_in;
  pthread_mutex_lock(&eventIdMutex);
  int socket = info->socket;
  size_t thisNameLen = strlen(info->eventName);
  char *thisEventName = strcpy(alloca(thisNameLen+1),info->eventName);
  void *arg = info->arg;
  void (*astadr) (void *, int, char *) = info->astadr;
  ssize_t recBytes;
  char recBuf[MAX_MSG_LEN];
  struct sockaddr clientAddr;
  int addrSize = sizeof(clientAddr);
  unsigned int nameLen,bufLen;
  char *eventName;
  char *currPtr;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,0);
  free(info->eventName);
  free(info);
  pthread_mutex_unlock(&eventIdMutex);

  while (1) {
#ifdef _WIN32
    if ((recBytes = recvfrom(socket, (char *)recBuf, MAX_MSG_LEN, 0,
			     (struct sockaddr *)&clientAddr, &addrSize)) < 0) {
      int error = WSAGetLastError();
      if (error == WSAESHUTDOWN || error == WSAEINTR || error == WSAENOTSOCK) {
	break;
      } else {
	fprintf(stderr,"Error getting data - %d\n", error);

      }
      continue;
    }
#else
    if ((recBytes = recvfrom(socket, (char *)recBuf, MAX_MSG_LEN, 0,
			     (struct sockaddr *)&clientAddr, (socklen_t *) & addrSize)) < 0) {
      break;
    }
#endif
    if (recBytes == 0)
      break;
    if (recBytes < (int)(sizeof(int) * 2 + thisNameLen))
      continue;
    currPtr = recBuf;
    memcpy(&nameLen,currPtr,sizeof(nameLen));
    nameLen = ntohl(nameLen);
    if (nameLen != thisNameLen)
      continue;
    currPtr += sizeof(int);
    eventName = currPtr;
    currPtr += nameLen;
    memcpy(&bufLen, currPtr, sizeof(bufLen));
    bufLen = ntohl(bufLen);
    currPtr += sizeof(int);
    if ((size_t)recBytes != (nameLen + bufLen + 8)) /*** check for invalid buffer ***/
      continue;
    if (strncmp(thisEventName, eventName, nameLen))   /*** check to see if this message matches the event name ***/
      continue;
    astadr(arg, (int)bufLen, currPtr);
  }
  return 0;
}

static void initialize()
{
  INITIALIZESOCKETS;
  pthread_mutex_lock(&initializeMutex);
  InitializeEventSettings();
  pthread_mutex_unlock(&initializeMutex);
}

static int pushEvent(pthread_t thread, int socket) {
  pthread_mutex_lock(&eventIdMutex);
  EventList *ev = malloc(sizeof(EventList));
  ev->eventid = ++EVENTID;
  ev->socket = socket;
  ev->thread = thread;
  ev->next = EVENTLIST;
  EVENTLIST=ev;
  pthread_mutex_unlock(&eventIdMutex);
  return ev->eventid;
}

static EventList *popEvent(int eventid) {
  pthread_mutex_lock(&eventIdMutex);
  EventList *ev,*ev_prev;
  for (ev=EVENTLIST,ev_prev=0; ev && ev->eventid != eventid; ev_prev=ev,ev=ev->next);
  if (ev) {
    if (ev_prev) {
      ev_prev->next = ev->next;
    } else {
      EVENTLIST = ev->next;
    }
  }
  pthread_mutex_unlock(&eventIdMutex);
  return ev;
}

static int getSendSocket()
{
  pthread_mutex_lock(&getSocketMutex);
  if (!sendSocket) {
    if ((sendSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
#ifdef _WIN32
      int error = WSAGetLastError();
      fprintf(stderr, "Error creating socket - errcode=%d\n", error);
#else
      perror("Error creating socket\n");
#endif
      sendSocket = -1;
    }
  }
  pthread_mutex_unlock(&getSocketMutex);
  return sendSocket;
}


static void getMulticastAddr(char const *eventName, char *retIp)
{
  char *addr_format;
  size_t i,len = strlen(eventName);
  unsigned char arange[2];
  unsigned int hash = 0, hashnew;
  UdpEventGetAddress(&addr_format,arange);
  for (i = 0; i < len; i++)
    hash += (unsigned int)eventName[i];
  hashnew =
      (unsigned int)((float)arange[0] +
		     ((float)(hash % 256) / 256.) * ((float)arange[1] - (float)arange[0] + 1.));
  sprintf(retIp, addr_format, hashnew);
  free(addr_format);
}

int MDSUdpEventAst(char const *eventName, void (*astadr) (void *, int, char *), void *astprm,
		   int *eventid)
{
  int check_bind_in_directive;
  struct sockaddr_in serverAddr;
#ifdef _WIN32
  char flag = 1;
#else
  int flag = 1;
  int const SOCKET_ERROR = -1;
#endif
  int udpSocket;
  char ipAddress[64];
  struct ip_mreq ipMreq;
  struct EventInfo *currInfo;
  unsigned short port;
  pthread_t thread;
  memset(&ipMreq, 0, sizeof(ipMreq));

  initialize();

  if ((udpSocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
#ifdef _WIN32
    int error = WSAGetLastError();
    fprintf(stderr, "Error creating socket - errcode=%d\n", error);
#else
    perror("Error creating socket\n");
#endif
    return 0;
  }
  //    serverAddr.sin_len = sizeof(serverAddr);
  serverAddr.sin_family = AF_INET;
  UdpEventGetPort(&port);
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Allow multiple connections
  if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == SOCKET_ERROR) {
    printf("Cannot set REUSEADDR option\n");
#ifdef _WIN32
    switch (WSAGetLastError()) {
    case WSANOTINITIALISED:
      printf("WSAENETDOWN\n");
      break;
    case WSAENETDOWN:
      printf("WSAENETDOWN\n");
      break;
    case WSAEADDRINUSE:
      printf("WSAEADDRINUSE\n");
      break;
    case WSAEINTR:
      printf("WSAEINTR\n");
      break;
    case WSAEINPROGRESS:
      printf("WSAEINPROGRESS\n");
      break;
    case WSAEALREADY:
      printf("WSAEALREADY\n");
      break;
    case WSAEADDRNOTAVAIL:
      printf("WSAEADDRNOTAVAIL\n");
      break;
    case WSAEAFNOSUPPORT:
      printf("WSAEAFNOSUPPORT\n");
      break;
    case WSAECONNREFUSED:
      printf("WSAECONNREFUSED\n");
      break;
    case WSAEFAULT:
      printf("WSAEFAULT\n");
      break;
    default:
      printf("BOH\n");
    }
#else
    perror("\n");
#endif
    return 0;
  }
#ifdef _WIN32
  check_bind_in_directive = (bind(udpSocket, (SOCKADDR *) & serverAddr, sizeof(serverAddr)) != 0);
#else
  check_bind_in_directive = (bind(udpSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)) != 0);
#endif
  if (check_bind_in_directive){
    perror("Cannot bind socket\n");
    return 0;
  }

  getMulticastAddr(eventName, ipAddress);
  ipMreq.imr_multiaddr.s_addr = inet_addr(ipAddress);
  ipMreq.imr_interface.s_addr = INADDR_ANY;
  if (setsockopt(udpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&ipMreq, sizeof(ipMreq)) < 0) {
#ifdef _WIN32
    int error = WSAGetLastError();
    char *errorstr = 0;
    switch (error) {
    case WSANOTINITIALISED:
      errorstr = "WSAENETDOWN";
      break;
    case WSAENETDOWN:
      errorstr = "WSAENETDOWN";
      break;
    case WSAEADDRINUSE:
      errorstr = "WSAEADDRINUSE";
      break;
    case WSAEINTR:
      errorstr = "WSAEINTR";
      break;
    case WSAEINPROGRESS:
      errorstr = "WSAEINPROGRESS";
      break;
    case WSAEALREADY:
      errorstr = "WSAEALREADY";
      break;
    case WSAEADDRNOTAVAIL:
      errorstr = "WSAEADDRNOTAVAIL";
      break;
    case WSAEAFNOSUPPORT:
      errorstr = "WSAEAFNOSUPPORT";
      break;
    case WSAECONNREFUSED:
      errorstr = "WSAECONNREFUSED";
      break;
    case WSAENOPROTOOPT:
      errorstr = "WSAENOPROTOOPT";
      break;
    case WSAEFAULT:
      errorstr = "WSAEFAULT";
      break;
    default:
      errorstr = 0;
    }
    if (errorstr)
      fprintf(stderr, "Error setting socket options IP_ADD_MEMBERSHIP in udpStartReceiver - %s\n",
	      errorstr);
    else
      fprintf(stderr,
	      "Error setting socket options IP_ADD_MEMBERSHIP in udpStartReceiver - error code %d\n",
	      error);
#else
    perror("Error setting socket options IP_ADD_MEMBERSHIP in udpStartReceiver\n");
#endif
    return 0;
  }

  currInfo = (struct EventInfo *)malloc(sizeof(struct EventInfo));
  currInfo->eventName = strdup(eventName);
  currInfo->socket = udpSocket;
  currInfo->arg = astprm;
  currInfo->astadr = astadr;
  pthread_create(&thread, 0, handleMessage, (void *)currInfo);
//  pthread_detach(thread);
  *eventid = pushEvent(thread, udpSocket);
  return 1;
}

int MDSUdpEventCan(int eventid)
{
  EventList *ev = popEvent(eventid);
  if (ev) {
#ifdef _WIN32
    // For some reason shutdown does not abort the recvfrom and hangs the process joining
    // the handleMessage thread. closesocket seems to work though.
    closesocket(ev->socket);
#else
    shutdown(ev->socket, SHUT_RDWR);
    close(ev->socket);
#endif
//    pthread_cancel(ev->thread);
    pthread_join(ev->thread,0);
    free(ev);
    return 1;
  } else {
    printf("invalid eventid %d\n",eventid);
    return 0;
  }
}

int MDSUdpEvent(char const *eventName, unsigned int bufLen, char const *buf)
{
  char multiIp[64];
  uint32_t buflen_net_order = (uint32_t)htonl(bufLen);
  int udpSocket;
  struct sockaddr_in sin;
  char *msg=0, *currPtr;
  unsigned int msgLen, nameLen = (unsigned int)strlen(eventName), actBufLen;
  uint32_t namelen_net_order = (uint32_t)htonl(nameLen);
  int status;
  unsigned short port;
  char ttl, loop;
  struct in_addr *interface_addr=0;

  initialize();
  getMulticastAddr(eventName, multiIp);
  udpSocket = getSendSocket();

  memset((char *)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  int addr;
  GETHOSTBYNAMEORADDR(multiIp,addr);
  if (hp)
    memcpy(&sin.sin_addr, hp->h_addr_list[0], (size_t)hp->h_length);
  FREE_HP;
  UdpEventGetPort(&port);
  sin.sin_port = htons(port);
  nameLen = (unsigned int)strlen(eventName);
  if (bufLen < MAX_MSG_LEN - (4u + 4u + nameLen))
    actBufLen = bufLen;
  else
    actBufLen = MAX_MSG_LEN - (4u + 4u + nameLen);
  msgLen = 4u + nameLen + 4u + actBufLen;
  msg = malloc(msgLen);
  currPtr = msg;

  memcpy(currPtr, &namelen_net_order, sizeof(namelen_net_order));
  currPtr += sizeof(buflen_net_order);

  memcpy(currPtr, eventName, nameLen);
  currPtr += nameLen;

  memcpy(currPtr, &buflen_net_order, sizeof(buflen_net_order));
  currPtr += sizeof(buflen_net_order);

  if (buf && bufLen > 0)
    memcpy(currPtr, buf, bufLen);

  pthread_mutex_lock(&sendEventMutex);
  if (UdpEventGetTtl(&ttl))
    setsockopt(udpSocket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
  if (UdpEventGetLoop(&loop))
    setsockopt(udpSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
  if (UdpEventGetInterface(&interface_addr)) {
    status = setsockopt(udpSocket, IPPROTO_IP, IP_MULTICAST_IF, (char *)interface_addr, sizeof(*interface_addr));
    free(interface_addr);
  }
  if (sendto(udpSocket, msg, msgLen, 0, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
    perror("Error sending UDP message!\n");
#ifdef _WIN32
    switch (WSAGetLastError()) {
    case WSANOTINITIALISED:
      printf("WSAENETDOWN\n");
      break;
    case WSAENETDOWN:
      printf("WSAENETDOWN\n");
      break;
    case WSAEADDRINUSE:
      printf("WSAEADDRINUSE\n");
      break;
    case WSAEINTR:
      printf("WSAEINTR\n");
      break;
    case WSAEINPROGRESS:
      printf("WSAEINPROGRESS\n");
      break;
    case WSAEALREADY:
      printf("WSAEALREADY\n");
      break;
    case WSAEADDRNOTAVAIL:
      printf("WSAEADDRNOTAVAIL\n");
      break;
    case WSAEAFNOSUPPORT:
      printf("WSAEAFNOSUPPORT\n");
      break;
    case WSAECONNREFUSED:
      printf("WSAECONNREFUSED\n");
      break;
    case WSAEFAULT:
      printf("WSAEFAULT\n");
      break;
    default:
      printf("Unknown error occurred while sending event msg.\n");
    }
#endif
    status = 0;
  } else
    status = 1;
  if (msg)
    free(msg);
  pthread_mutex_unlock(&sendEventMutex);
  return status;
}

/**********************
 Message structure:

- event name len (4 byte int)
- event name (event name len bytes)
- buf len (4 byte int)
- buf (buf len chars)

***********************/
