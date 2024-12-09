/*
 * idiscover.c
 * Discover all IPMI-LAN-enabled nodes on this network or subnet.
 * This program is not completely reliable yet, not all IPMI-LAN-enabled
 * nodes respond.
 * Currently this utility is compiled with NO_THREADS, but threads can
 * be enabled by commenting out this flag.
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2006 Intel Corporation. 
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 10/27/06 Andy Cress - created
 * 05/01/07 Andy Cress - added -g for GetChannelAuthCap method,
 *                       added -a logic for broadcast ping,
 *                       updated WIN32 logic
 * 09/20/07 Andy Cress - fixed send/receive thread order
 * 07/15/08 Andy Cress - added -r for ping repeats
 * 11/21/08 Andy Cress - detect eth intf and broadcast ip addr
 * 01/04/16 Andy Cress - v1.11, allow 0 if fBroadcastOk (-a) 
 * 11/30/24 Andy Cress - v1.12, detect if IP address
 */
/*M*
Copyright (c) 2006, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 
  c.. Neither the name of Intel Corporation nor the names of its contributors 
      may be used to endorse or promote products derived from this software 
      without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *M*/
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>
#include <io.h>
#include <time.h>
#include "getopt.h"
#define NO_THREADS  1
typedef unsigned int socklen_t;
WSADATA lan_ws;  /*global for WSA*/

#elif defined(DOS)
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"
#define NO_THREADS  1

#else
/* Linux, BSD, Solaris */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#include <sys/sockio.h>
#ifndef HAVE_CONFIG_H
typedef unsigned int socklen_t;
#endif
#else
#include <getopt.h>
#endif

/* comment out NO_THREADS to use this utility in Linux with threads */
#define NO_THREADS  1
#endif
#ifndef ETH_P_IP
#define ETH_P_IP  0x0800 /* Internet Protocol packet, see linux/if_ether.h */
#endif

/* TODO: fix RAW for -m in Solaris, FreeBSD, Windows (works in Linux) */
#ifdef SOLARIS
#include <sys/sockio.h>
#define RAW_DOMAIN  AF_INET
#define RAW_PROTO   IPPROTO_RAW
static char frawok = 0;  /*raw not working in Solaris*/ 
#elif BSD
#define RAW_DOMAIN  AF_INET
#define RAW_PROTO   IPPROTO_RAW
static char frawok = 0;  /*raw not working in FreeBSD*/ 
#elif HPUX
#define RAW_DOMAIN  AF_INET
#define RAW_PROTO   IPPROTO_RAW
static char frawok = 0;  /*raw not working in HPUX*/ 
#elif MACOS
#define RAW_DOMAIN  AF_INET
#define RAW_PROTO   IPPROTO_RAW
static char frawok = 0;  /*raw not working in MacOS*/ 
#elif WIN32
#define RAW_DOMAIN  AF_INET
#define RAW_PROTO   IPPROTO_ICMP
static char frawok = 0;  /*raw not working in Windows*/ 
#else
#define RAW_DOMAIN  AF_PACKET
#define RAW_PROTO   htons(ETH_P_IP)
static char frawok = 1;  /*raw works in Linux*/
#endif

#include <string.h>
#include "ipmicmd.h"
 
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define RMCP_PRI_RMCP_PORT     0x26F
#define SZ_PING    12
#define IPMI_PING_MAX_LEN  50  /* usu 28 */
#define CMD_GET_CHAN_AUTH_CAP      0x38

#ifdef WIN32
int GetFirstIP(uchar *ipaddr, uchar *macadr, char *ipname, char fdb); /*ilan.c*/
#endif

/*
 * Global variables 
 */
static char * progver   = "1.12";
static char * progname  = "idiscover";
static char   fdebug    = 0;
static char   fping     = 1;
static char   fraw      = 0;
static char   fBroadcastOk = 0;
static char   fcanonical   = 0;
static int broadcast_pings = 1;
//static uchar ipmi_maj = 0;
//static uchar ipmi_min = 0;
//static uchar netfn;
static ushort   g_port   = RMCP_PRI_RMCP_PORT; 
static SockType g_sockfd = 0; 
static SockType g_sockraw = 0; 
static int    g_limit   = 30;   /* after this many 'other' packets, stop. */
static struct sockaddr_in _srcaddr;
// static struct sockaddr_in _destaddrlist[255];
static struct  in_addr    _startAddr, _endAddr; 
static char g_startDest[MAXHOSTNAMELEN+1] = {'\0'};  
static char g_endDest[MAXHOSTNAMELEN+1] = {'\0'};
static char g_interface[INET_ADDRSTRLEN+1] = {""};  /*e.g. "eth0"*/
static int  g_num_packets = 0;
static int  g_npings = 0;
static int  g_npongs = 0;
static int  g_recv_status = 0;
static int  g_wait   = 1; /* num sec to wait */
static int  g_delay  = 0; /* num usec between sends */
static int  g_repeat = 1; /* number of times to repeat ping to each node */
static char bdelim = BDELIM;  /* '|' */

#ifdef METACOMMAND
extern FILE *fpdbg;     /*from ipmicmd.c*/
extern char *strlasterr(int rv); /*from ipmilan.c*/
#else
void dump_buf(char *tag, uchar *pbuf, int sz, char fshowascii)
{
   uchar line[17];
   uchar a;
   int i, j;
   FILE *fpdbg;

   fpdbg = stdout;
   line[0] = 0; line[16] = 0;
   j = 0;
   fprintf(fpdbg,"%s (len=%d): ", tag,sz);
   for (i = 0; i < sz; i++) {
      if (i % 16 == 0) { j = 0; fprintf(fpdbg,"%s\n  %04x: ",line,i); }
      if (fshowascii) {
         a = pbuf[i];
         if (a < 0x20 || a > 0x7f) a = '.';
         line[j++] = a;
      }
      fprintf(fpdbg,"%02x ",pbuf[i]);
   }
   if (j < 16) {
      line[j] = 0;
      for (i = 0; i < (16-j); i++) fprintf(fpdbg,"   ");
   }
   fprintf(fpdbg,"%s\n",line);
   return;
} 
#endif

void printerr( const char *pattn, ...)
{
   va_list arglist;
   FILE *fderr;

   // fderr = fopen("/tmp/idiscover.log","a+");
   // if (fderr == NULL) return;
   fderr = stderr;

   va_start(arglist, pattn);
   vfprintf(fderr, pattn, arglist);
   va_end(arglist);

   // fclose(fderr);
}

static char *showlasterr(void)
{
   char *str;
#ifdef WIN32
   static char strbuf[80];
   int rv;
   char *desc;
   rv = WSAGetLastError();
#ifdef METACOMMAND
   /* get descriptions from strlasterr in ipmilan.c */
   desc = strlasterr(rv);
#else
   if (rv == WSAECONNRESET) desc = "Connection reset"; /*10054*/
   else if (rv == WSAECONNREFUSED) desc = "Connection refused"; /*10061*/
   else if (rv == WSAEHOSTUNREACH) desc = "No route to host"; /*10065*/
   else desc = "";
#endif
   sprintf(strbuf,"LastError = %d  %s",rv,desc);
   str = strbuf;
#else
   str = strerror(errno);
#endif
   return(str);
}

static void cleanup(void)
{
   SockType *pfd;
   int i;
   for (i = 0; i < 2; i++) {
     if (i == 0) pfd = &g_sockfd;
     else pfd = &g_sockraw;
     if (*pfd > 0) {
#ifdef WIN32
        closesocket(*pfd);
        WSACleanup();
#else
        close(*pfd);
#endif
     }
     *pfd = 0;
   }
}

void show_usage(void)
{
   printf("Usage: %s [-abeghimprx] \n",progname);
   printf("  -a            all nodes, enables broadcast ping\n");
   printf("  -b <ip>       beginning IP address (x.x.x.x), required\n");
   printf("  -e <ip>       ending IP address (x.x.x.x), default is begin IP\n");
   printf("  -g            use GetChanAuthCap instead of RMCP ping\n");
   printf("  -h            print this help text\n");
   printf("  -i <name|ip>  interface to use: name, IP address or 0.0.0.0\n");
   printf("                defaults to first network interface (e.g.  eth0)\n");
   printf("  -m            get MAC addresses with a raw broadcast ping\n");
   printf("  -p <N>        specific port (IPMI LAN port=623)\n");
   printf("  -r <N>        number of repeat pings to each node (default=1)\n");
// printf("  -s            specific subnet\n");
   printf("  -x            show extra debug messages\n");
}

static int os_sleep(unsigned int s, unsigned int u)
{
#ifdef WIN32
  if (s == 0) {
     if (u >= 1000) Sleep(u/1000);
  } else {
     Sleep(s * 1000);
  }
#else
/*Linux*/
#ifdef SELECT_TIMER
  struct timeval tv;
  tv.tv_sec  = s;
  tv.tv_usec = u;
  if (select(1, NULL, NULL, NULL, &tv) < 0)
     printerr("select: %s\n", showlasterr());
#else
  if (s == 0) {
      usleep(u);
  } else {
      sleep(s);
  }
#endif
#endif
  return 0;
}

void split_ip(uint addr, uchar *ip)
{
   ip[3] = (addr & 0x000000ff);
   ip[2] = (addr & 0x0000ff00) >> 8;
   ip[1] = (addr & 0x00ff0000) >> 16;
   ip[0] = (addr & 0xff000000) >> 24;
}

int ntoi(int addr)
{
   return(addr);
}

void show_ip(int saddr)
{
   uchar ip[4];
   split_ip(saddr,ip);
   printerr("%d.%d.%d.%d\n",ip[0],ip[1],ip[2],ip[3]);
}

static int ipmilan_sendto(SockType s, const void *msg, size_t len, int flags, 
				const struct sockaddr *to, socklen_t tolen)
{
    int n;
#ifdef NEED_PAD
    int fusepad = 0;
    /* Never need a pad byte for ping/pong packets */
    if (len == 56 || len == 84 || len == 112 || len == 128 || len == 156) {
        fusepad = 1;
        len += 1;
    }
#endif
    n = (int)sendto(s,msg,len,flags,to,tolen);
    // if (fusepad && (n > 0)) n--;
    return(n);
}

static int ipmilan_recvfrom(SockType s, void *buf, size_t len, int flags, 
		struct sockaddr *from, socklen_t *fromlen)
{
    int rv;
    rv = (int)recvfrom(s,buf,len,flags,from,fromlen);
    /* Sometimes the OS sends an ECONNREFUSED error, but 
     * retrying will catch the BMC's reply packet. */
#ifdef WIN32
    if (rv < 0) {
        int err;
        err = WSAGetLastError();
        if (err == WSAECONNREFUSED) /*10061*/
            rv = (int)recvfrom(s,buf,len,flags,from,fromlen);
    }
#else
    if ((rv < 0) && (errno == ECONNREFUSED))
        rv = (int)recvfrom(s,buf,len,flags,from,fromlen);
#endif
    return(rv);
}

#if defined(WIN32)
int inet_aton(const char *cp, struct in_addr *inp)
{
   int rv;
   int adr;
   inp->s_addr = inet_addr(cp);
   adr = (int)inp->s_addr;
   if (adr == INADDR_NONE) rv = 0;
   else rv = 1;  /*success*/
   return(rv);
}
#elif defined(SOLARIS) || defined(HPUX)
int find_ifname(char *ifname)
{ return(-1); }
#else
#include <ifaddrs.h>
int find_ifname(char *ifname)
{
    struct ifaddrs *ifaddr, *ifa;
    int rv = -1;
    if (getifaddrs(&ifaddr) == -1) return(rv);
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) continue;
		if ((ifa->ifa_addr->sa_family != AF_INET) &&
		    (ifa->ifa_addr->sa_family != AF_INET6)) continue;
		if (strcmp(ifa->ifa_name,"lo") == 0) continue;
		/* if here, we have a valid ifname */
		strcpy(ifname,ifa->ifa_name);
		if (fdebug) printf("find_ifname: found %s\n",ifname);
		rv = 0;
		break;
    }
    freeifaddrs(ifaddr);
    return(rv);
}
#endif

int sock_init( char *_interface, char *_startIP, char *_endIP)
{
  int rv;
  uchar *pb;
  uchar val;
 
#ifdef WIN32
  DWORD rvl;
  rvl = WSAStartup(0x0101,&lan_ws);
  if (rvl != 0) {
      printerr("init: WSAStartup(1.1) error %ld\n", rvl);
      return((int)rvl);
  }
#else
  char findif;
#endif

  if ((g_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SockInvalid) {
    printerr("socket: %s\n", showlasterr());
    return(-1);
  }

  memset(&_srcaddr, 0, sizeof(_srcaddr));
  _srcaddr.sin_family = AF_INET;
  _srcaddr.sin_port = htons(0);
  _srcaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 
#ifdef WIN32
  {
     int ret;
     uchar osip[4];
     uchar osmac[6];
     uchar osname[64];
     char ipstr[20];
     char *temp_start;
     if (fBroadcastOk && (g_startDest[0] == 0)) {
	  ret = GetFirstIP(osip,osmac,osname,fdebug);  /*ilan.c*/
	  if (ret == 0) {
		 // osip[3] = 0xFF;  /*255 for broadcast*/
		 sprintf(ipstr,"%d.%d.%d.255",osip[0],osip[1],osip[2]);
	         temp_start = ipstr;
		 strcpy(g_startDest,temp_start);
		 strcpy(g_endDest,temp_start);
	  } else {  /*use some defaults*/
		 strcpy(g_startDest,"255.255.255.255");
		 strcpy(g_endDest,"255.255.255.255");
	  }
     }
  }
#elif defined(HPUX)
  {  /*find the OS eth interface to use*/
   char devname[INET_ADDRSTRLEN+1];
   int i, n;
   n = 0;
   sprintf(devname,"lan%d",n);
  }
#else
  {  /*find the OS eth interface to use*/
   struct sockaddr_in temp_sockaddr;
   char *temp_start;
   struct ifreq ifr;
   char devname[INET_ADDRSTRLEN+1];
   int i, n;
   if (_interface == NULL) findif = 1;
   else if (_interface[0] == 0) findif = 1;
   else findif = 0;
   if (findif) {
      n = find_ifname(devname);
      if (n >= 0) {
         _interface = devname;
         findif = 0; /*found, do not find again below */
      }
   }
   if (findif)
   {   /* try again to find the first active ethN interface */
      n = -1;
      for (i = 0; (i < 16) && (n == -1); i++) {
#ifdef SOLARIS
          sprintf(devname,"e1000g%d",i);
#elif  BSD
          sprintf(devname,"em%d",i);
#else
          sprintf(devname,"eth%d",i);
#endif
          strcpy(ifr.ifr_name, devname);
          ifr.ifr_addr.sa_family = AF_INET;
          if (ioctl(g_sockfd, SIOCGIFADDR, &ifr) >= 0) {
             /* valid IP address, so active interface, use it */
             temp_sockaddr = *((struct sockaddr_in *)&ifr.ifr_addr);
             memcpy(&_srcaddr.sin_addr.s_addr, &temp_sockaddr.sin_addr.s_addr,
                 sizeof(_srcaddr.sin_addr.s_addr));
             strcpy(g_interface, devname);  
	     temp_start = inet_ntoa(temp_sockaddr.sin_addr);
	     if (temp_start == NULL) temp_start = "";
	     else if (fBroadcastOk && (g_startDest[0] == 0)) {
		 pb = (uchar *)&temp_sockaddr.sin_addr.s_addr;
		 pb[3] = 0xFF;  /*255 for broadcast*/
	         temp_start = inet_ntoa(temp_sockaddr.sin_addr);
		 strcpy(g_startDest,temp_start);
		 strcpy(g_endDest,temp_start);
	     }
	     printf("sock_init: found %s with %s\n",devname,temp_start);
	     n = i;
	     break;
          }
      }
      if (n < 0) rv = LAN_ERR_OTHER; /*-13*/
   } else { /* valid _interface string */
      if (strchr(_interface, '.') != NULL)
      {   /* assume it is an IP address*/
         rv = inet_pton(AF_INET, _interface, &_srcaddr.sin_addr);
         if (rv < 0)
         {
           printerr("inet_pton: %s\n", showlasterr());
           return(-1);
         }
         if (rv == 0)
         {
           printerr("invalid interface address\n");
           return(-1);
         }
      }
      else
      {   /* assume interface name, like eth0 */
        if (strchr(_interface, '.') != NULL)
        {   /* assume it is an IP address*/
          if ((rv = inet_pton(AF_INET, _interface, &_srcaddr.sin_addr)) < 0)
            printerr("inet_pton: %s\n", showlasterr());
          if (rv == 0) 
            printerr("invalid interface address\n");
          return(rv);
        }
        else
        {   /* assume interface name, like eth0 */
          strncpy(ifr.ifr_name, _interface, IFNAMSIZ);
          ifr.ifr_addr.sa_family = AF_INET;
          if (ioctl(g_sockfd, SIOCGIFADDR, &ifr) < 0) {
             printerr("ioctl(%s): %s\n", _interface, showlasterr());
             return(-1);
          }
 
          temp_sockaddr = *((struct sockaddr_in *)&ifr.ifr_addr);
          memcpy(&_srcaddr.sin_addr.s_addr, &temp_sockaddr.sin_addr.s_addr,
                 sizeof(_srcaddr.sin_addr.s_addr));
	  if (fBroadcastOk && (g_startDest[0] == 0)) {
		 pb = (uchar *)&temp_sockaddr.sin_addr.s_addr;
		 pb[3] = 0xFF;  /*255 for broadcast*/
	         temp_start = inet_ntoa(temp_sockaddr.sin_addr);
		 strcpy(g_startDest,temp_start);
		 strcpy(g_endDest,temp_start);
	  }
        }
      }
    }
   }
#endif

   if (fBroadcastOk) {
        rv = setsockopt(g_sockfd, SOL_SOCKET, SO_BROADCAST,
                       (char *)&broadcast_pings, sizeof(broadcast_pings));
	if (rv) {
	   printerr("setsockopt: %s\n", showlasterr());
	   return(-1);
	}
   }
 
   if (bind(g_sockfd, (struct sockaddr *)&_srcaddr, sizeof(_srcaddr)) < 0) {
      printerr("bind: %s\n", showlasterr());
      return(-1);
   }
 
   rv = inet_aton(_startIP, &_startAddr);
   if (rv ) {
        _startAddr.s_addr = ntohl(_startAddr.s_addr);
        if (fdebug) show_ip(_startAddr.s_addr);
        pb = (unsigned char*)&_startAddr.s_addr;
        if (!fBroadcastOk && (pb[0] < 1) )
                printerr("Malformed begin IP: %s\n", _startIP);
        else if (!fBroadcastOk && (pb[0] >254) )
                printerr("Malformed begin IP: %s\n", _startIP);
        else if (fBroadcastOk) {
                val = pb[0] & 0x0f;
                if (val == 0x0f) rv = 0;
                else if (val == 0x00) rv = 0;
                else printerr("Malformed begin broadcast IP: %s\n", _startIP);
        } else rv = 0;
   } else {
        printerr("Invalid begin IP: %s\n", _startIP);
   }
   if (rv) return(rv);
 
   rv = inet_aton(_endIP, &_endAddr);
   if (rv ) {
        _endAddr.s_addr = ntohl(_endAddr.s_addr);
        if (fdebug) show_ip(_endAddr.s_addr);
        pb = (unsigned char*)&_endAddr.s_addr;
        if (!fBroadcastOk && (pb[0] < 1) )
              printerr("Malformed end IP: %s\n", _endIP);
        else if (!fBroadcastOk && (pb[0] >254) )
              printerr("Malformed end IP: %s\n", _endIP);
        else rv = 0;
   } else {
        printerr("Invalid end IP: %s\n", _endIP);
   }

   /* calculate g_num_packets */
   g_num_packets = ntoi(_endAddr.s_addr) - ntoi(_startAddr.s_addr) + 1;
   if (fdebug) printerr("g_num_packets = %d\n",g_num_packets);
   if (g_num_packets < 1) g_num_packets = 0;

   return(rv);
}  /*end sock_init*/

void *receiveThread(void *p)
{
   uchar buffer[IPMI_PING_MAX_LEN];
   struct timeval tv;
   fd_set rset;
   int rv, len;
   static int needlf = 0;
   char host[200];
   SockType sockrecv;
   int nothers = 0;
   int addr_type = AF_INET;   /*or AF_INET6*/
#ifndef WIN32
   struct hostent *h_ent = NULL;
   char serv[200];
   int r;
#endif

   sockrecv = g_sockfd;
   if (fraw) {  /* opening SOCK_RAW requires admin/root privilege. */
      if ((g_sockraw = socket(RAW_DOMAIN, SOCK_RAW,RAW_PROTO)) == SockInvalid) 
      {
        printerr("raw socket: %s\n", showlasterr());
        fraw = 0;
      } else {
        sockrecv = g_sockraw;
        if (fdebug) printf("g_sockraw = %d\n",g_sockraw);
      }
   }

   /* receive while loop */
   do
   {
      tv.tv_sec  = g_wait;
      tv.tv_usec = 0;
      FD_ZERO(&rset);
      FD_SET(sockrecv, &rset);
      g_recv_status = 0;

      if (fdebug) printerr("waiting for ping %d response\n",g_npings);
      if ((rv = select((int)(sockrecv+1), &rset, NULL, NULL, &tv)) < 0) {
          printerr("select: %s\n", showlasterr());
          cleanup();
          exit(rv);  // error, exit idiscover recv thread
      }
      
      if (fdebug) printerr("select rv = %d\n",rv);
      if (rv > 0)
          {
              struct sockaddr_in from;
              socklen_t fromlen;
	      char rstr[40];
	      char macstr[20];
	      struct in_addr from_ip;
	      char estr[40];
 
	      if (needlf) { printf("\n"); needlf = 0; }
              g_recv_status = 1;
              fromlen = sizeof(from);
              len = ipmilan_recvfrom(sockrecv, buffer, IPMI_PING_MAX_LEN, 0,
                                     (struct sockaddr *)&from, &fromlen);
              if (fdebug) printerr("recvfrom rv = %d\n",rv);
              if (len < 0) { 
		printerr("ipmilan_recvfrom: %s\n", showlasterr());
		continue; 
              }
              if (fdebug) {
		/* can we get the MAC addr of the responder also? */
		// dump_buf("from_addr",(uchar *)&from,fromlen,0);
		dump_buf("recvfrom",buffer,len,0);
	      }
              g_recv_status = 2;
              g_npongs++;
	      macstr[0] = 0;
	      if (fraw) {  /* Raw packet, include MAC address string */
		  /*
		   * Sample raw packet for UDP ping response
		   *       dst_mac           src_mac           eth_p iphdr
  		   * 0000: 00 07 e9 06 15 31 00 0e 0c 2b b5 81 08 00 45 00
		   *                            udp      src_ip      dst_ip
  		   * 0010: 00 38 00 00 40 00 40 11 b6 01 c0 a8 01 c2 c0 a8
		   *                                     rmcp
  		   * 0020: 01 a1 02 6f 80 1e 00 24 0e d1 06 00 ff 06 00 00
  		   * 0030: 11 be
		   */
		  if ((buffer[23] != 0x11) || (buffer[42] != 0x06)) { 
		     /* [23]: 0x11==UDP, 0x06==TCP ; [42]: 0x06 ==RMCP */
		     if (nothers > g_limit) {
			if (fdebug) 
			    printf("got %d other packets, stop.\n",nothers);
			break;
		     }
		     nothers++;
		     continue;
		  }
		  if (buffer[6] == 0xFF || buffer[26] == 0xFF) /*broadcast*/
		     continue;
                  sprintf(macstr,"%02x:%02x:%02x:%02x:%02x:%02x %c",
			buffer[6], buffer[7], buffer[8],  /*06=src_mac*/
			buffer[9], buffer[10], buffer[11], bdelim);
		  memcpy(&from_ip,&buffer[26],4);  /*26=src_ip*/
	      } else {
		  memcpy(&from_ip,&from.sin_addr,4);
	      }
              host[0] = 0;
#ifndef WIN32
/* Linux, BSD, Solaris, MacOS */
#if !defined(CROSS_COMPILE)
	      h_ent = gethostbyaddr((void *)&from_ip,4,addr_type);
	      if (h_ent != NULL) {
		 strncpy(host,h_ent->h_name,sizeof(host));
	      } else if (!fraw) 
#endif
	      {	 
                 r = getnameinfo((struct sockaddr *)&from, fromlen, 
			       host, sizeof(host), serv, sizeof(serv), 0);
	         if (r) host[0] = 0;
	         else if (host[0] >= '0' && host[0] <= '9') host[0] = 0;
	      }
#endif
              // parse the received pong 
	      rstr[0] = 0;
	      if (fping == 0 && len > 0) {   /* -g and got rsp data */
		 /* parse GetChanAuthcap response into rstr */
		 /* 4 bytes RMCP, 10 bytes IPMI session, then msg */
		 /* 6 bytes msg hdr, then rsp data */
		 /* 20 = ccode, 21 = chan, 22 = auth type support */
		 if (buffer[20] != 0)  /*ccode error*/
		    sprintf(rstr,"%c (ccode=0x%02x)",bdelim,buffer[20]);
		 else  /*ccode is ok*/
		    sprintf(rstr,"%c (channel %d)",bdelim,buffer[21]);
	      }
	      if (fcanonical) {
		 estr[0] = 0;
		 rstr[0] = 0;
	      } else {
		 sprintf(estr,"response from %c ",bdelim);
	      }
	      /* &buffer[0] = source MAC, &buffer[6] = destination MAC */
              printf("%.2d%c %s%s %s \t%c %s %s\n",
		    g_npongs,bdelim,estr,macstr,inet_ntoa(from_ip),
		    bdelim,host,rstr);
          }
      else {   /*ping, no answer*/
	  if (!fBroadcastOk) {
		printf("."); fflush(stdout); /*show progress*/
		needlf = 1;
	  }
      }
   }
#ifdef NO_THREADS
     while(fBroadcastOk && rv > 0);
#else
     while(1);
#endif
   return(p);
} 

/*
 * send_ping_pkt:
 * RMCP Ping buffer, sent as a UDP packet to port 0x026f.
 * rmcp.ver     = 0x06          // RMCP Version 1.0
 * rmcp.resvd   = 0x00          // RESERVED
 * rmcp.seq     = 0xff          // no RMCP ACK 
 * rmcp.class   = 0x06          // RMCP_CLASS_ASF
 * asf.iana     = 0x000011BE    // ASF_RMCP_IANA (hi-lo)
 * asf.type     = 0x80          // ASF_TYPE_PING
 * asf.tag      = 0x00          // ASF sequence number
 * asf.resvd    = 0x00          // RESERVED
 * asf.len      = 0x00
 */
int send_ping_pkt(struct sockaddr_in *_daddr, uchar seq)
{
   uchar pingbuf[SZ_PING] = {06,0,0xFF,06,0x00,0x00,0x11,0xBE,0x80,0,0,0 };
   int rv, len;

   pingbuf[9] = seq;
   len = sizeof(pingbuf);
   if (fdebug) dump_buf("send_ping",pingbuf,len,0);
   rv = ipmilan_sendto(g_sockfd, pingbuf, len, 0, 
	          (struct sockaddr *)_daddr, sizeof(struct sockaddr_in));
   return(rv);
}

static int send_poke1(struct sockaddr_in *_daddr)
{
   int rv;
   uchar asfpkt[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x07, 0x20, 0x18, 0xc8, 0xc2, 0x01, 0x01, 0x3c };
   if (fdebug) dump_buf("send_poke1",asfpkt,16,0);
   rv = ipmilan_sendto(g_sockfd, asfpkt, 16, 0, 
	          (struct sockaddr *)_daddr, sizeof(struct sockaddr_in));
   return rv;
}

static uchar cksum(const uchar *buf, register int len)
{
        register uchar csum;
        register int i;
 
        /* 8-bit 2s compliment checksum */
        csum = 0;
        for (i = 0; i < len; i++)
           csum = (csum + buf[i]) % 256;
        csum = -csum;
        return(csum);
}

static int send_getauth(struct sockaddr_in *_daddr, uchar seq)
{
   int rv, len;
   // static uchar swseq = 0;
   uchar getauthpkt[23] = { 0x06, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 
   		            0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x20, 0x18, 
   			    0xc8, 0x81, 0x04, 0x38, 0x0e, 0x04, 0x31 };
   /* 
    *  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22
    * 06 00 ff 07 00 00 00 00 00 00 00 00 00 09 20 18 c8 81 04 38 0e 04 31 
    * [RMCP hdr ] [IPMI session hdr      (len)] [IPMI msg       ] [data]
    */
   // getauthpkt[8]  = 0x00;  /* seq always 00 for GetChanAuthCap */
   getauthpkt[21] = 0x02;  /*requested priv_level: 2=user, 4=admin*/
   getauthpkt[22] = cksum(&getauthpkt[17],5);
   len = sizeof(getauthpkt);
   if (fdebug) dump_buf("send_getauth",getauthpkt,len,0);
   rv = ipmilan_sendto(g_sockfd, getauthpkt, len, 0, 
   	          (struct sockaddr *)_daddr, sizeof(struct sockaddr_in));
   if (fdebug) 
       printf("send_getauth: rv = %d\n",rv);
   return rv;
}

int send_probe(struct sockaddr_in *_daddr, uchar seq)
{
    int rv;

    if (fBroadcastOk) {
       rv = setsockopt(g_sockfd, SOL_SOCKET, SO_BROADCAST,
                       (char *)&broadcast_pings, sizeof(broadcast_pings));
       if (fdebug) {
	  char *estr;
	  if (rv) estr = showlasterr();
	  else estr = "";
	  printerr("setsockopt(broadcast): rv=%d %s\n", rv,estr);
       }
    }
    if (fping) 
       rv = send_ping_pkt( _daddr, seq);
    else 
       rv = send_getauth( _daddr, seq);
    return(rv);
}

void *sendThread(void *p)
{
   int i, j, n;
   // char _dest[MAXHOSTNAMELEN+1];
   char _dest_ip[INET_ADDRSTRLEN+1];
   struct sockaddr_in _destaddr;
   uchar o[4];
   uint ip;
   uchar _seq;
   int rv;

   n = g_num_packets;  /*num*/
   ip = _startAddr.s_addr;

   for (i = 0; i < n; i++) 
   {
      split_ip(ip,o);
      if (o[3] == 0) continue;
      if (!fBroadcastOk && (o[3] == 255)) continue;
      sprintf(_dest_ip,"%d.%d.%d.%d",o[0],o[1],o[2],o[3]);

      /* set _destaddr */
      _destaddr.sin_family = AF_INET;
      _destaddr.sin_port = htons(g_port);
      if ( !inet_aton( _dest_ip, &_destaddr.sin_addr)) {
          printerr("inet_aton error %s\n",_dest_ip);
          continue;
      }

     for (j=0; j<g_repeat; j++)
     {
      /* send ping buffer */
      _seq = 0;
      rv = send_probe(&_destaddr,_seq);
      g_npings++;
      if (fdebug) printerr("sendto[%d,%d] %s rv = %d\n",
			    g_npings,_seq,_dest_ip,rv);
      if (rv < 0) {  /*try to send again*/
          rv = send_probe(&_destaddr,++_seq);
          if (rv < 0) {
              printerr("sendto[%d,%d] %s error %s\n",
			g_npings,_seq,_dest_ip,showlasterr()); 
              continue;
          }
      } 

#ifdef NO_THREADS
      receiveThread(NULL);
      if (g_recv_status == 0 && !fBroadcastOk) {
          /* nothing returned, try again */
          if (fping) {
             rv = send_poke1(&_destaddr);
             if (fdebug) printerr("sendto[%d,%d] %s poke rv = %d\n",
				   g_npings,_seq,_dest_ip,rv);
          }
          rv = send_probe(&_destaddr,++_seq);
          if (fdebug) printerr("sendto[%d,%d] %s rv = %d\n",
				g_npings,_seq,_dest_ip,rv);
          if (rv >= 0) {
             receiveThread(NULL);
          }
      }
#endif

      /* sleep an interval (g_delay usec) */
      if (g_delay > 0) os_sleep(0,g_delay);
     }  /*end-for g_repeat*/

     ip++; /* increment to next IP */
   }
   return(p);
}

#ifdef METACOMMAND
int i_discover(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
#endif
{
   int rv = 0;
   int c;
#ifndef NO_THREADS
   char message[32];
   pthread_t thread[2];
   int  iret[2];
#endif

#ifdef METACOMMAND
   fpdbg = stdout;
#endif
   printf("%s ver %s\n", progname,progver);

   while ( (c = getopt( argc, argv,"ab:ce:ghi:l:mp:r:s:x?")) != EOF )
      switch(c) {
          case 'a': fBroadcastOk = 1; fping = 1;
		break;  /*all (broadcast ping)*/
	  case 'c':  /*canonical, CSV*/
		fcanonical = 1;
		bdelim = BCOMMA;
		break; 
          case 'm':  /* show MAC address, use raw, requires root priv */
		fBroadcastOk = 1; fping = 1; fraw = 1; 
		break;  /*all (broadcast ping)*/
          case 'l': g_limit = atoi(optarg); break;
          case 'g': fping = 0; break;   /*use get chan auth cap method */
          case 'b':   /*begin*/
                strncpy(g_startDest,optarg,MAXHOSTNAMELEN);
		break;
          case 'e':   /*end*/
                strncpy(g_endDest,optarg,MAXHOSTNAMELEN);
		break;
          case 'i':   /*interface*/
                strncpy(g_interface,optarg,sizeof(g_interface));
		break;
          case 'p':   /*port/socket*/
                g_port = (ushort)atoi(optarg);
		break;
          case 'r':   /*repeat N times*/
                g_repeat = atoi(optarg);
		break;
          case 's':   /*subnet*/
                /* copy optarg from 10.243.42.0 or similar, to
                 * begin/end range. */
		break;
          case 'x': fdebug = 1;     break;  /* debug messages */
	  case 'h': default:
		if (fdebug) printerr("getopt(%c) default\n",c);
                show_usage();
                rv = ERR_USAGE;
		goto do_exit;
      }
#ifdef WIN32
   /* Winsock inet_aton() does not like 255.255.255.255 */
   if (!fBroadcastOk && (g_startDest[0] == 0) ) {
      show_usage();
      printerr("A beginning IP is required, using -b\n");
      goto do_exit;
   }
#else
   if (g_startDest[0] == 0)  {
      strcpy(g_startDest,"255.255.255.255"); /* INADDR_BROADCAST */
      fBroadcastOk = 1;  
   }
#endif
    if (fraw == 1) {
      if (frawok == 0) {
    	printf("Warning: SOCK_RAW not yet implemented on this OS\n");
      } 
#ifdef LINUX
     else {
        c = geteuid();
        if (c > 1) printf("Must be root/superuser to use SOCK_RAW\n");
      }
#endif
   }
   if (g_endDest[0] == 0 || fBroadcastOk) 
      strcpy(g_endDest,g_startDest);   /*only one IP address*/
   if (fdebug) 
      printerr("intf=%s begin=%s end=%s port=%d\n",
		g_interface,g_startDest,g_endDest,g_port);

   rv = sock_init(g_interface, g_startDest, g_endDest); 
   if (fdebug) printerr("sock_init rv = %d, sockfd = %d\n",rv,g_sockfd);
   if (rv != 0) {
      show_usage();
      printerr("sock_init error %d\n",rv);
      goto do_exit;
   }

   printf("Discovering IPMI Devices:\n");
#ifdef NO_THREADS
   sendThread(NULL);
#else
   iret[1] = pthread_create( &thread[1], NULL, receiveThread, (void*) message);
   iret[0] = pthread_create( &thread[0], NULL, sendThread, (void*) message);
   pthread_join( thread[0], NULL);
   pthread_join( thread[1], NULL);
#endif

   // if (fdebug) 
   printf("\n%s: %d pings sent, %d responses\n",progname,g_npings,g_npongs);

do_exit:
   cleanup();
   return(rv);
}  /* end main()*/

/* end idiscover.c */
