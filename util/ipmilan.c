/*M*
//  PVCS:
//      $Workfile:   ipmilan.c  $
//      $Revision:   1.0  $
//      $Modtime:   2 Feb 2006 15:31:14  $
//      $Author:   arcress at users.sourceforge.net  $  
//
//  This implements support for the IPMI LAN interface natively.
// 
//  02/02/06 ARC - created.
//  05/16/06 ARC - more added.
//  06/19/06 ARCress - fixed sequence numbers for 1.7.1
//  08/08/06 ARCress - fix AUTHTYPE masks
//  08/11/06 ARCress - added lan command retries
//  10/17/06 ARCress - special case for no MsgAuth headers
//  11/28/06 ARCress - added ipmi_cmd_ipmb routine
//  05/08/07 ARCress - added 1.5 SOL data packet format to _send_lan_cmd,
//                     not working yet.
//  08/21/07 ARCress - handle Dell 1855 blades that return different authcode
//  04/17/08 ARCress - check FD_ISSET in fd_wait
 *M*/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2005-2006, Intel Corporation
Copyright (c) 2009 Kontron America, Inc.
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
 *----------------------------------------------------------------------*/

#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <time.h>
#include <signal.h>

//#define HAVE_IPV6  1
#ifdef HAVE_IPV6 
#include <winsock2.h>
//#include <ws2tcpip.h>
//#include <tpipv6.h>
#else
#include <winsock.h>
#define MSG_WAITALL  0x100   /* Wait for a full request */ 
#endif

#define INET_ADDRSTRLEN 16
#define MSG_NONE     0x000 
#define int32_t     int
#define u_int32_t   unsigned int
#define uint32      unsigned int
#define uchar       unsigned char
#define RECV_MSG_FLAGS  MSG_NONE
typedef unsigned int socklen_t;

#elif defined(DOS)
#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#undef HAVE_LANPLUS

#else   /* Linux, BSD, etc. */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#ifdef HPUX
#define RECV_MSG_FLAGS  0x40  /*match MSG_WAITALL for HPUX*/
#else
#define RECV_MSG_FLAGS  MSG_WAITALL
#endif
#endif
#include "ipmicmd.h"
#include "ipmilan.h"

#if defined(HAVE_CONFIG_H) 
#include "config.h"
#else
#if defined(MACOS)
typedef unsigned int socklen_t;
#endif
#endif

#if defined(LINUX) 
/* TODO: fixups in BSD/Solaris for ipv6 method */
#define HAVE_IPV6  1
#endif

#if defined(CROSS_COMPILE)
#undef HAVE_IPV6 
static struct hostent *xgethostbyname(const char *nstr)
{
   static struct hostent hptr;
   if (StrIsIp(nstr)) {  /* the string is an IP address */
      hptr.h_name = (char *)nstr;
      inet_aton(nstr,(struct in_addr *)&hptr.h_addr);
      return(&hptr);
   } else {  /*the string is a node name */
      return(NULL);
   }
}
#else
#define xgethostbyname gethostbyname
#endif

// #define TEST_LAN   1      /*++++TEST_LAN for DEBUG++++*/
#if defined(ALLOW_GNU) 
#define MD2OK     1  /*use md2.h GPL code*/
#else
/* if here, ALLOW_GPL is not defined, check for lanplus libcrypto. */
#if defined(HAVE_LANPLUS)
#define MD2OK     1  /*use MD2 version from lanplus libcrypto */
#endif
/* if libcrypto does not have EVP_md2, then skip MD2. */
#if defined(SKIP_MD2)
#undef  MD2OK    
#endif
#endif

/*  Connection States:
 *  0 = init, 1 = socket() complete, 2 = bind/gethost complete,
 *  3 = ping sent, 4 = pong received, 5 = session activated.  
 *  see also conn_state_str[] below */
#define CONN_STATE_INIT   0
#define CONN_STATE_SOCK   1
#define CONN_STATE_BIND   2
#define CONN_STATE_PING   3
#define CONN_STATE_PONG   4
#define CONN_STATE_ACTIVE 5

#define SOL_DATA   0xFD       /*SOL Data command*/
#define SOL_MSG    0x10000000 /*SOL message type*/
#define SOL_HLEN   14 
// SZGNODE == 80
#define SZ_CMD_HDR  4  /*cmd, netfn/lun, sa*/
#define SWID_REMOTE  0x81
#define SWID_SMSOS   0x41

#pragma pack(1)
typedef struct {  /*first 30 bytes conform to IPMI header format*/
  uchar rmcp_ver; 
  uchar rmcp_res; 
  uchar rmcp_seq; 
  uchar rmcp_type; 
  uchar auth_type;
  uint32 seq_num;   /*outgoing seq*/
  uint32 sess_id;
  uchar  auth_code[16];
  uchar  msg_len;  /* size here = 30 bytes = RQ_HDR_LEN */
  uchar  swid;     /* usu SWID_REMOTE. From here down, order is changeable */
  uchar  swseq;    /* outgoing swseq */
  uchar  swlun;
  uchar  priv_level;
  uint32 iseq_num;  /*incoming seq */
  uchar  bmc_addr;  /*usu BMC_SA*/
  uchar  target_addr;
  uchar  target_chan;
  uchar  target_lun;
  uchar  target_cmd;
  uchar  target_netfn;
  uchar  transit_addr;
  uchar  transit_chan;
  uchar  bridge_level;
  uchar  password[16];
  uchar  challenge[16];
  } IPMI_HDR;
#pragma pack()

typedef struct {
  int type;
  int len;
  char *data;
  } SOL_RSP_PKT;

/* extern void atoip(uchar *array,char *instr);  *subs.c*/
extern FILE *open_log(char *mname);  /*ipmicmd.c*/
extern char * get_iana_str(int mfg); /*subs.c*/

#define dbglog   printf
#define dbg_dump dump_buf
extern FILE *fperr;  /*defined in ipmicmd.c, usu stderr */
extern FILE *fpdbg;  /*defined in ipmicmd.c, usu stdout */
extern int  gshutdown;   /* from ipmicmd.c, usu =0 */
extern int  fauth_type_set; /* from ipmicmd.c */
extern LAN_OPT lanp;     /* from ipmicmd.c */
//extern char *gnode;      /* from ipmicmd.c */
//extern char *guser;      /* from ipmicmd.c */
//extern char *gpswd;      /* from ipmicmd.c */
//extern int  gauth_type;  /* from ipmicmd.c */
//extern int  gpriv_level; /* from ipmicmd.c */
extern ipmi_cmd_t ipmi_cmds[NCMDS];

static IPMI_HDR ipmi_hdr = { 0x06, 0, 0xFF, 0x07, 0x00,   0,   0, 
     /*auth_code*/{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 0, /*msg_len*/
     /*swid*/SWID_REMOTE, 1,0,0,0, BMC_SA,0,0,0,0,0,0,0,0 /*bridge_level*/
  };
// static IPMI_HDR *phdr;
static uchar  bmc_sa   = BMC_SA;      /*usu 0x20*/
static uchar  sms_swid = SWID_REMOTE;  /*usu 0x81*/
static int    vend_id  = 0;   
static int    prod_id  = 0;   

#if defined(DOS) || defined(EFI)
int ipmi_open_lan(char *node, char *user, int port, char *pswd, int fdebugcmd)
{
   printf("IPMI LAN is not supported under DOS.\n");
   return(-1);
}
int ipmi_close_lan(char *node)
{
   printf("IPMI LAN is not supported under DOS.\n");
   return(-1);
}
int ipmi_cmd_lan(char *node, ushort cmd, uchar *pdata, int sdata,
                uchar *presp, int *sresp, uchar *pcc, char fdebugcmd)
{
   printf("IPMI LAN is not supported under DOS.\n");
   return(-1);
}
int ipmi_cmdraw_lan(char *node, uchar cmd, uchar netfn, uchar lun, uchar sa,
                uchar bus, uchar *pdata, int sdata, uchar *presp, int *sresp,
                uchar *pcc, char fdebugcmd)
{
   printf("IPMI LAN is not supported under DOS.\n");
   return(-1);
}
int ipmicmd_lan(char *node, uchar cmd, uchar netfn, uchar lun, uchar sa,
                uchar bus, uchar *pdata, int sdata, uchar *presp, int *sresp,
                uchar *pcc, char fdebugcmd)
{
   printf("IPMI LAN is not supported under DOS.\n");
   return(-1);
}
#else
/* All other OSs can support IPMI LAN */

/* 
 * These variables pertain to ipmilan, for the node given at open time.
 * The assumption here is that these are utilities, so no more than one
 * node will be open at a given time.  
 * See also gnode, guser, gpswd in ipmicmd.c
 */
typedef struct {
  int connect_state;    /*=CONN_STATE_INIT*/
  SockType sockfd;
  int finsession;
  uint32 session_id;
  uint32 in_seq;        /*=1*/
  uint32 start_out_seq; /*=1*/
  uchar  fMsgAuth;      /*0=AuthNone 1=PerMsgAuth 2=UserLevelAuth*/
  uchar  auth_type;     /*=AUTHTYPE_INIT*/
  } LAN_CONN;    /*see also IPMI_HDR below*/
 // static int connect_state = CONN_STATE_INIT;   
 // static int sockfd = 0;
 // static int   finsession = 0;
 // static uint32 session_id = 0;
 // static uint32 in_seq = 0x01;    /* inbound sequence num */
 // static uint32 start_out_seq = 0x01;   /* initial outbound sequence num */
 // static uchar  fMsgAuth   = 1;
 static uchar  auth_type = AUTHTYPE_INIT;  /*initial value, not set*/
 static char nodename[SZGNODE+1] = "";
 static char gnodename[SZGNODE+1] = ""; /*nodename returned after connection*/
#if defined(AI_NUMERICSERV)
static int my_ai_flags = AI_NUMERICSERV; /*0x0400 Dont use name resolution NEW*/
// static int my_ai_flags = AI_NUMERICHOST; /*0x0004 Dont use name resolution*/
#else
#undef HAVE_IPV6
#endif

#ifdef HAVE_IPV6
#define SOCKADDR_T  struct sockaddr_storage
#else
#define SOCKADDR_T  struct sockaddr_in
static char _dest_ip[INET_ADDRSTRLEN+1];
// static char _dest[MAXHOSTNAMELEN+1];
#endif
static SOCKADDR_T _srcaddr;
static SOCKADDR_T _destaddr;
static int  _destaddr_len = 0;
#ifdef TEST_LAN
static int fdebuglan = 3;
#else
static int fdebuglan = 0;
#endif
static int fdoping   = 0;  /* =1 do ping first, set to 0 b/c no added value*/
static int fdopoke1  = 0;
static int fdopoke2  = 0;
static int frequireping = 0; /*=1 if ping is required, =0 ignore ping error */
static LAN_CONN conn = {CONN_STATE_INIT,0,0,0,1,1,1}; 
static LAN_CONN *pconn = &conn;
static char  *authcode = NULL;
static int    authcode_len = 0;
static int ping_timeout = 1;       /* timeout: 1 sec */
static int ipmi_timeout = 2;       /* timeout: 10 sec -> 2 sec */
static int ipmi_try = 4;           /* retries: 4 */
static uchar bridgePossible = 0;
static char *conn_state_str[6] = {
	"init state", "socket complete", "bind complete", 
	"ping sent", "pong received", "session activated" };
int lasterr = 0;

static uchar sol_op       = 0x80;  /* encrypted/not */
static uchar sol_snd_seq  = 0;     /* valid if non-zero*/
static uchar sol_rcv_seq  = 0;
static uchar sol_rcv_cnt  = 0;
static uchar sol_rcv_ctl  = 0x00;
// static uchar sol_offset   = 0;
static uchar sol_seed_cnt = 0x01;  /* set after activate response */
static char     sol_Encryption = 0;      /*for SOL 1.5*/
static uint32   g_Seed[ 16 ];          /*for SOL 1.5*/
static uchar    g_Cipher[ 16 ][ 16 ];  /*SeedCount x CipherHash for SOL 1.5*/
#define BUSY_MAX  10  /*for Dell FS12-TY Node Busy errors*/

#ifdef WIN32
int econnrefused = WSAECONNREFUSED; /*=10061.*/
int econnreset   = WSAECONNRESET; /*=10054.*/
#else
int econnrefused = ECONNREFUSED; /*=111. from Linux asm/errno.h */
int econnreset   = ECONNRESET;  /*=104.*/
#endif

#ifdef WIN32
WSADATA lan_ws; 

/* See http://msdn2.microsoft.com/en-us/library/ms740668.aspx 
 * or doc/winsockerr.txt  */
#define NWINERRS  21
static struct { int err; char *desc; } winerrs[NWINERRS] = {
    WSAEINTR /*10004*/, "Interrupted function call",
    WSAEBADF /*10009*/, "File handle is not valid",
    WSAEACCES /*10013*/, "Permission denied",
    WSAEFAULT /*10014*/, "Bad address",
    WSAEINVAL /*10022*/, "Invalid argument",
    WSAEMFILE /*10024*/, "Too many open files",
    WSAENOTSOCK /*10038*/, "Socket operation on nonsocket", 
    WSAEDESTADDRREQ /*10039*/, "Destination address required", 
    WSAEMSGSIZE /*10040*/, "Message too long",
    WSAEOPNOTSUPP /*10045*/, "Operation not supported",
    WSAEADDRINUSE /*10048*/, "Address already in use",
    WSAEADDRNOTAVAIL /*10049*/, "Cannot assign requested address",
    WSAENETDOWN /*10050*/, "Network is down",
    WSAENETUNREACH /*10051*/, "Network is unreachable",
    WSAENETRESET /*10052*/, "Network dropped connection on reset",
    WSAECONNABORTED /*10053*/, "Software caused connection abort",
    WSAECONNRESET /*10054*/, "Connection reset by peer",
    WSAENOTCONN /*10057*/, "Socket is not connected",
    WSAECONNREFUSED /*10061*/, "Connection refused",
    WSAEHOSTDOWN /*10064*/, "Host is down",
    WSAEHOSTUNREACH  /*10065*/, "No route to host"
};

char * strlasterr(int rv)
{
   char *desc;
   int i;
   for (i = 0; i < NWINERRS; i++) {
       if (winerrs[i].err == rv) {
	  desc = winerrs[i].desc;
	  break;
       }
   }
   if (i >= NWINERRS) desc = "";

   return(desc);
}
#endif

#ifdef MD2OK
extern void md2_sum(uchar *string, int len, uchar *mda); /*from md2.c*/
#endif
extern void md5_sum(uchar *string, int len, uchar *mda); /*from md5.c*/

int _ipmilan_cmd(SockType s, struct sockaddr *to, int tolen,
     		uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
		uchar *sdata, int slen, uchar *rdata, int *rlen, 
		int fdebugcmd);
static int _send_lan_cmd(SockType s, uchar *pcmd, int scmd, uchar *presp, 
		int *sresp, struct sockaddr *to, int tolen);
static int ipmilan_open_session(SockType sfd, struct sockaddr *destaddr, 
			int destaddr_len, uchar auth_type, char *username, 
			char *authcode, int authcode_len, 
			uchar priv_level, uint32 init_out_seqnum, 
			uint32 *session_seqnum, uint32 *session_id);
static int ipmilan_close_session(SockType sfd, struct sockaddr *destaddr, 
			int destaddr_len, uint32 session_id);
uchar cksum(const uchar *buf, register int len);
char *decode_rv(int rv);  /*moved to ipmicmd.c*/

void show_LastError(char *tag, int err)
{
#ifdef WIN32
    fprintf(fperr,"%s LastError = %d  %s\n",tag,err,strlasterr(err));
#else
    char *s;
    s = strerror(err);
    if (s == NULL) s = "error";
    fprintf(fperr,"%s errno = %d, %s\n",tag,err,s);
#endif
}

int get_LastError( void )
{
#ifdef WIN32
   lasterr = WSAGetLastError();
#else
   lasterr = errno;
#endif
   return(lasterr);
}

static void cc_challenge(int cc)
{
   switch(cc) {
      case 0x81:  
          printf("GetSessChallenge: Invalid user name\n");
          break;
      case 0x82: 
          printf("GetSessChallenge: Null user name not enabled\n");
          break;
      default:
          printf("GetSessChallenge: %s\n",decode_cc((ushort)0,cc));
          break;
   }
}

static void cc_session(int cc)
{
   switch(cc) {
      case 0x81:  
          printf("ActivateSession: No session slots available from BMC\n");
          break;
      case 0x82: 
          printf("ActivateSession: No sessions available for this user\n");
          break;
      case 0x83: 
          printf("ActivateSession: No sessions for this user/privilege\n");
          break;
      case 0x84: 
          printf("ActivateSession: Session sequence number out of range\n");
          break;
      case 0x85: 
          printf("ActivateSession: Invalid session ID in request\n");
          break;
      case 0x86: 
          printf("ActivateSession: Privilege level exceeds user/channel limit\n");
          break;
      default:
          printf("%s\n",decode_cc((ushort)0,cc));
          break;
   }
   return;
}

int StrIsIp(char *str)
{
   int i, j, n;
   char ipchars[11] = "0123456789.";
   int ndot = 0;
   int rv = 0;
   /* checks if the string looks like an IP address. */
   if (str == NULL) return(rv);
   n = (int)strlen(str);
   for (i = 0; i < n; i++) {
      for (j = 0; j < 11; j++)
         if (str[i] == ipchars[j]) break;
      if (j >= 11) break; /*some other char, not valid*/
      if (str[i] == '.') ndot++;
   }
   if ((i == n) && (ndot == 3)) rv = 1; /*valid*/
   return(rv);
}

void close_sockfd(SockType sfd);
void close_sockfd(SockType sfd)
{
    if (sfd == 0) return;
#ifdef WIN32
    // shutdown(sfd,SD_SEND);  /*done sending*/
    closesocket(sfd);   /*close lan socket */
    WSACleanup(); 
#else
    alarm(0);
    signal(SIGALRM,SIG_DFL);
    signal(SIGINT,SIG_DFL);
    close(sfd);   /*close lan socket */
#endif
    pconn->sockfd = 0;   /*set global to zero */
}

int open_sockfd(char *node, int port, SockType *sfd, SOCKADDR_T  *daddr, 
                       int *daddr_len, int foutput);
int open_sockfd(char *node, int port, SockType *sfd, SOCKADDR_T  *daddr, 
                       int *daddr_len, int foutput)
{
    int rv = 0;
    SockType s, _sockfd = -1;
#ifdef HAVE_IPV6
    struct addrinfo hints;
    struct addrinfo *res, *res0;
    char service[NI_MAXSERV];
#else
    struct hostent *hptr;
#endif

#ifdef WIN32
    DWORD rvl;

    if (sfd == NULL || daddr == NULL || daddr_len == NULL)
        return(-3);  /* invalid pointer */
    rvl = WSAStartup(0x0202,&lan_ws);
    if (rvl != 0) {
       fprintf(fperr,"lan, WSAStartup(2.2) error %ld, try 1.1\n", rvl);
       WSACleanup(); 
       rvl = WSAStartup(0x0101,&lan_ws);
       if (rvl != 0) {
	   fprintf(fperr,"lan, WSAStartup(1.1) error %ld\n", rvl);
	   WSACleanup(); 
           return((int)rvl);
       }
    }
#else
    if (sfd == NULL || daddr == NULL || daddr_len == NULL)
        return(-3);  /* invalid pointer */
#endif
        pconn->connect_state = CONN_STATE_INIT;  

#ifdef HAVE_IPV6
	memset(&_srcaddr, 0, sizeof(_srcaddr));
	memset(daddr, 0, sizeof(_destaddr)); 
	sprintf(service, "%d", port);
	/* Obtain address(es) matching host/port */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM;   /* Datagram socket */
	hints.ai_flags    = my_ai_flags;
	hints.ai_protocol = IPPROTO_UDP; /*  */
	rv = getaddrinfo(node, service, NULL, &res);
	if (rv != 0) {
	   printf("Address lookup for %s failed, getaddrinfo error %d\n", 
			node,rv);
	   return rv;
	}

	/* getaddrinfo() returns a list of address structures.
	 * Try each address until we successfully connect(2).
	 * If socket(2) (or connect(2)) fails, we (close the socket
	 * and) try the next address. 
	 */
	for (res0 = res; res0 != NULL; res0 = res0->ai_next) {
           /* valid protocols are IPPROTO_UDP, IPPROTO_IPV6 */
	   if (res0->ai_protocol == IPPROTO_TCP) continue;  /*IPMI != TCP*/
	   s = socket(res0->ai_family, res0->ai_socktype, res0->ai_protocol);
	   if (s == SockInvalid) continue;
	   else _sockfd = s;
	   pconn->connect_state = CONN_STATE_SOCK;  
	   rv = connect(_sockfd, res0->ai_addr, res0->ai_addrlen); 
	   if (fdebuglan) printf("socket(%d,%d,%d), connect(%d) rv = %d\n",
				res0->ai_family, res0->ai_socktype, 
				res0->ai_protocol, s,rv);
	   if (rv != -1) {
		memcpy(daddr, res0->ai_addr, res0->ai_addrlen);
		*daddr_len = res0->ai_addrlen;
		break;  /* Success */
	   }
	   close_sockfd(_sockfd);
	   _sockfd = -1;
	}
	freeaddrinfo(res);  /* Done with addrinfo */
	if (_sockfd < 0) {
	   printf("Connect to %s failed\n", node);
	   // ipmi_close_();
	   rv = -1;
	}
#else
	/* Open lan interface (ipv4 method) */
	s = socket(AF_INET, SOCK_DGRAM, 0); 
	if (s == SockInvalid) return (-1);
	else _sockfd = s;

	pconn->connect_state = CONN_STATE_SOCK;  
	memset(&_srcaddr, 0, sizeof(_srcaddr));
	_srcaddr.sin_family = AF_INET;
	_srcaddr.sin_port = htons(0);
	_srcaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	rv = bind(_sockfd, (struct sockaddr *)&_srcaddr, sizeof(_srcaddr));
	if (rv < 0) {
            close_sockfd(_sockfd);
            return (rv);
        }

	memset(daddr, 0, sizeof(SOCKADDR_T));
	daddr->sin_family = AF_INET;
	daddr->sin_port = htons(port);  /*RMCP_PRI_RMCP_PORT 0x26f = 623.*/
	if (StrIsIp(node)) {  /* the node string is an IP address */
	    uchar in_ip[4];
            atoip(in_ip,node);
            memcpy(&daddr->sin_addr.s_addr,in_ip,4);
	    if ((hptr = xgethostbyname(node)) == NULL) /*gethost error*/
	         strncpy(gnodename,node,SZGNODE);     /*but not fatal*/
	    else strncpy(gnodename,hptr->h_name,SZGNODE);
	}
	else if ((hptr = xgethostbyname(node)) == NULL) {
            if (foutput) {
#ifdef WIN32
                fprintf(fperr,"lan, gethostbyname(%s): errno=%d\n", node,get_errno());
#elif SOLARIS
                fprintf(fperr,"lan, gethostbyname(%s): errno=%d\n", node,get_errno());
#elif defined(HPUX)
				/*added by ugene	*/
				fprintf(fperr,"lan, gethostbyname(%s): errno=%d\n", node,errno);
#else
				fprintf(fperr,"lan, gethostbyname(%s): %s\n", node,hstrerror(errno));
#endif
            }
	    close_sockfd(_sockfd);
	    return(LAN_ERR_HOSTNAME);
	} else {   /*gethostbyname(name) succeeded*/
	    daddr->sin_addr = *((struct in_addr *)hptr->h_addr);
	    strncpy(gnodename,hptr->h_name,SZGNODE);
	}
        *daddr_len = sizeof(SOCKADDR_T);
#endif

     *sfd = _sockfd;
     return(rv);
}

static void
sig_timeout(int sig)
{
#ifndef WIN32
   alarm(0);
   signal(SIGALRM,SIG_DFL);
#endif
   fprintf(fpdbg,"ipmilan_cmd timeout, after %s\n",conn_state_str[pconn->connect_state]);
   _exit(LAN_ERR_TIMEOUT);   /*timeout signal*/
}

static void
sig_abort(int sig)
{
   static int sig_aborting = 0;
   int rv;

   if (sig_aborting == 0) {
     sig_aborting = 1;
     if (pconn->sockfd != 0) {  /* socket is open */
	  if (pconn->session_id != 0) {  /* session is open */
  	    // cmd_rs = buf_rs;
	    rv = ipmilan_close_session(pconn->sockfd, 
			(struct sockaddr *)&_destaddr, 
			  _destaddr_len, ipmi_hdr.sess_id);
	  }
          close_sockfd(pconn->sockfd);
     }
     signal(SIGINT,SIG_DFL);
     fprintf(fpdbg,"ipmilan_cmd interrupt, after %s\n", conn_state_str[pconn->connect_state]);
     _exit(LAN_ERR_ABORT);  /*abort signal*/
   }  /*endif*/
} /*end sig_abort*/

int fd_wait(SockType fd, int nsec, int usec)
{
    fd_set readfds;
    struct timeval tv;
    int rv;
              
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    tv.tv_sec  = nsec; 
    tv.tv_usec = usec;
    rv = select((int)(fd+1), &readfds, NULL, NULL, &tv);
    if (rv <= 0 || !FD_ISSET(fd,&readfds)) return(-1);
    else return(0);
}

#ifndef WIN32
                  /* LINUX, Solaris, BSD */
/* do_sleep() is currently unused. */
/* signal handlers + sleep(3) is a bad idea */
int do_sleep(unsigned int sleep_len)
{
  struct timeval tv;
  
  if (sleep_len == 0)
    return 0;

  tv.tv_sec = sleep_len;
  tv.tv_usec = 0;
  if (select(1, NULL, NULL, NULL, &tv) < 0)
    {
      if (errno != EINTR) return(errno);
    }
  return 0;
}
#endif

static void h2net(uint h, uchar *net, int n)
{
   int i = 0;
   net[i++] = h & 0xff;
   net[i++] = (h >> 8)  & 0xff;
   if (n == 2) return;
   net[i++] = (h >> 16)  & 0xff;
   net[i++] = (h >> 24)  & 0xff;
   return;
}

static void net2h(uint32 *h, uchar *net, int n)
{
   uint32 v;
   v  = (net[1] << 8) | net[0];
   if (n == 2) { *h = v; return; }
   v |= (uint32)(net[3] << 24) | (net[2] << 16);
   *h = v;
   return;
}

/* 
 * _ipmilan_cmd 
 * local routine to send & receive each command.
 * called by global ipmicmd_lan()
 */
int _ipmilan_cmd(SockType sockfd, struct sockaddr *hostaddr, int hostaddr_len,
     uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
     uchar *sdata, int slen, uchar *rdata, int *rlen, int fdebugcmd)
{
  uchar cmd_rq[RQ_LEN_MAX+SZ_CMD_HDR];
  uchar cmd_rs[RS_LEN_MAX+SZ_CMD_HDR+1];
  int rv = 0;
  int rs_len;
  int clen;
  uchar cc = 0;

#ifndef TEST_LAN
  fdebuglan = fdebugcmd;  
#endif
  if (sockfd == 0 || hostaddr == NULL ||
      sdata == NULL || rdata == NULL)
  	return(LAN_ERR_INVPARAM);;

  if (fdebuglan > 2) 
	dbglog("_ipmilan_cmd(%02x,%02x,%02x,%02x,%02x)\n",
				cmd,netfn,lun,sa,bus);
  clen = SZ_CMD_HDR;
  cmd_rq[0] = cmd;
  cmd_rq[1] = (netfn << 2) + (lun & 0x03);
  cmd_rq[2] = sa;
  cmd_rq[3] = bus;
  memcpy(&cmd_rq[clen],sdata,slen);
  rs_len = sizeof(cmd_rs);
  memset(cmd_rs, 0, rs_len);
  rv = _send_lan_cmd(sockfd, cmd_rq, slen+clen, cmd_rs, &rs_len, 
			hostaddr, hostaddr_len);
  if (rv == 0 && rs_len == 0) cc = 0;
  else cc = cmd_rs[0];
  if (fdebugcmd) fprintf(fpdbg,"_ipmilan_cmd[%02x]: rv = %d, cc=%x rs_len=%d\n",
			 cmd_rq[0],rv,cc,rs_len);
  if (rv == 0 && cc != 0) {     /* completion code error */
  	if (fdebugcmd) {
                dump_buf("cmd_rq",cmd_rq,slen+clen,0);
                dump_buf("cmd_rs",cmd_rs,rs_len,0);
	}
  }
  if (rv == 0) {
      if (rs_len < 0) rs_len = 0;
      if (*rlen <= 0) *rlen = 1;   /*failsafe*/
      if (rs_len > 0) {
          if (rs_len > *rlen) rs_len = *rlen;
          memcpy(rdata,&cmd_rs[0],rs_len);
      } else { /*(rs_len == 0)*/
          rv = LAN_ERR_TOO_SHORT; /*no completion code returned*/
      }
  } else {
      rdata[0] = cc;
      if (rs_len < 1) rs_len = 1;
  }
  *rlen = rs_len;

  return (rv);
}  /*end _ipmilan_cmd()*/


static void hash(uchar *pwd, uchar *id, uchar *chaldata, int chlen, uint32 seq, 
		 uchar *mda, uchar md)
{
   uchar pbuf[80];  /* 16 + 4 + 16 + 4 + 16 = 56 */
   int blen, n, i;

   blen = 0;      n = 16;
   memcpy(&pbuf[blen], pwd,n);   /* password   */
   blen += n;     n = 4;
   memcpy(&pbuf[blen],id,n);     /* session id */
   blen += n; 
   memcpy(&pbuf[blen],chaldata,chlen);  /* ipmi msg data, incl challenge */
   blen += chlen; n = 4;
   h2net(seq,&pbuf[blen],n);     /* in_seq num */
   blen += n;     n = 16;
   memcpy(&pbuf[blen],pwd,n);    /* password   */
   blen += n;
   if (md == IPMI_SESSION_AUTHTYPE_MD2) i = 2;
   else i = 5;
#ifdef TEST_AUTH
   if (fdebuglan) {
      fprintf(fpdbg,"hash: calling md%d_sum with seq %u\n",i,seq);
      dump_buf("pbuf",pbuf,blen,0);
      }
#endif
#ifdef MD2OK
   if (md == IPMI_SESSION_AUTHTYPE_MD2)
      md2_sum(pbuf,blen,mda);
   else   /* assume md5 */
#endif
      md5_sum(pbuf,blen,mda);
#ifdef TEST_AUTH
   if (fdebuglan) {
      fprintf(fpdbg,"Hashed MD%d AuthCode: \n",i);
      dump_buf("AuthCode",mda,16,0);
      }
#endif
} /* end hash() */

static void hash_special(uchar *pwd, uchar *chaldata, uchar *mda)
{
   uchar md_pwd[16];
   uchar challenge[16];
   int i;
   /* Only used by SuperMidro, must be MD5 auth_type */
   memset(md_pwd, 0, 16);
   md5_sum(pwd,16,md_pwd);
   memset(challenge, 0, 16);
   for (i = 0; i < 16; i++)
       challenge[i] = chaldata[i] ^ md_pwd[i];
   memset(mda, 0, 16);
   md5_sum(challenge,16,mda);
}

static int ipmilan_sendto(SockType s, const void *msg, size_t len, int flags, 
				const struct sockaddr *to, int tolen)
{
    int fusepad = 0;
    int n;
    if (fdebuglan > 2) {
        dbg_dump("ipmilan_sendto",(uchar *)msg,(int)len,0);
    }
    /* Check whether we need a pad byte */
    /* Note from Table 12-8, RMCP Packet for IPMI via Ethernet footnote. */
    if (len == 56 || len == 84 || len == 112 || len == 128 || len == 156) {
        /* include pad byte at end, require input buffer to have one extra */
        fusepad = 1;
        len += 1;
    }
    n = (int)sendto(s,msg,len,flags,to,(socklen_t)tolen);
    if (fusepad && (n > 0)) n--;
    return(n);
}

static int ipmilan_recvfrom(SockType s, void *buf, size_t len, int flags, 
		struct sockaddr *from, int *fromlen)
{
    int rv;
    rv = (int)recvfrom(s,buf,len,flags,from,(socklen_t *)fromlen);
#ifdef OLD
    /* Sometimes the OS sends an ECONNREFUSED error, but
     * retrying will catch the BMC's reply packet. */
    if (rv < 0) {
        int err;
        err = get_LastError();
        if (err == econnrefused) {
           if (fdebuglan) 
              fprintf(fpdbg,"ipmilan_recvfrom rv=%d econnrefused, retry\n",rv);
            rv = recvfrom(s,buf,len,flags,from,(socklen_t *)fromlen);
	}
    }
#endif
    return(rv);
}

static int ipmilan_poke1(SockType sfd, struct sockaddr *destaddr, int destlen)
{
   int rv;
   uchar asfpkt[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x07, 0x20, 0x18, 0xc8, 0xc2, 0x01, 0x01, 0x3c };
   if (fdebuglan) fprintf(fpdbg,"sending ipmilan poke1\n");
   rv = ipmilan_sendto(sfd, asfpkt, 16, 0, destaddr, destlen);
   os_usleep(0,100);
   return rv;
}

static int ipmilan_poke2(SockType sfd, struct sockaddr *destaddr, int destlen)
{
   int rv;
   uchar asfpkt[16] = "poke2";  /*any junk*/
   if (fdebuglan) fprintf(fpdbg,"sending ipmilan poke2\n");
   rv = ipmilan_sendto(sfd, asfpkt, 10, 0, destaddr, destlen);
   os_usleep(0,100);
   return rv;
}


static void do_hash(uchar *password, uchar *sessid, uchar *pdata, int sdata, 
             uint32 seq_num, uchar auth_type, uchar *auth_out)
{
       /* finish header with auth_code */
       if (auth_type != IPMI_SESSION_AUTHTYPE_NONE)  {  /*fdoauth==1*/
         if (auth_type == IPMI_SESSION_AUTHTYPE_MD5)
            hash(password, sessid, pdata, sdata, seq_num, 
		 auth_out,IPMI_SESSION_AUTHTYPE_MD5);
#ifdef MD2OK
         else if (auth_type == IPMI_SESSION_AUTHTYPE_MD2)
            hash(password, sessid, pdata, sdata, seq_num, 
		 auth_out,IPMI_SESSION_AUTHTYPE_MD2);
#endif
         else  /* IPMI_SESSION_AUTHTYPE_PASSWORD */ 
            memcpy(auth_out,password,16);
      }
}

static uint32 inc_seq_num(uint32 seq)
{
    seq++;
    if (seq == 0) seq = 1;  
    return(seq);
}

static int inc_sol_seq(int seq)
{
  seq++;
  if (seq > 0x0f) seq = 1;  /*limit to 4 bits*/
  return(seq);
}

void ipmi_lan_set_timeout(int ipmito, int tries, int pingto)
{
    ipmi_timeout = ipmito;  /*default 2*/
    ipmi_try = tries;    /*default 4*/
    ping_timeout = pingto;  /*default 1*/
}

/* 
 * _send_lan_cmd 
 * Internal routine called by local _ipmilan_cmd() and by
 * ipmilan_open_session().
 * Writes the data to the lan socket in IPMI LAN format.
 * Authentication, sequence numbers, checksums are handled here.
 *
 * Input Parameters:
 * s    = socket descriptor for this session
 * pcmd = buffer for the command and data
 *        Arbitrary pcmd format:
 *           cmd[0] = IPMI command 
 *           cmd[1] = NETFN/LUN byte
 *           cmd[2] = Slave Address (usu 0x20)
 *           cmd[3] = Bus  (usu 0x00)
 *           cmd[4-N] = command data
 * scmd = size of command buffer (3+sdata)
 * presp = pointer to existing response buffer
 * sresp = On input, size of response buffer,
 *         On output, length of response data.
 * to    = sockaddr structure for to/destination
 * tolen = length of to sockaddr 
 */
static int _send_lan_cmd(SockType s, uchar *pcmd, int scmd, uchar *presp, 
			int *sresp, struct sockaddr *to, int tolen)
{
    uchar cbuf[SEND_BUF_SZ];
    uchar rbuf[RECV_BUF_SZ];
    // uint32 out_seq = 0;   /* outbound */
    int clen, rlen, hlen, msglen;
    int flags; 
    int sz, n, i, j;
    int cs1, cs2, cs3 = 0, cs4 = 0;
    uchar *pdata;
    int sdata;
    IPMI_HDR *phdr;
    uchar *psessid;
    uchar iauth[16];
    int fdoauth = 1;
    uint32 sess_id_tmp;
    int rv = 0; 
    int itry;
    uchar fsentok;
    
    /* set up LAN req hdr */
    phdr = &ipmi_hdr;
    hlen = RQ_HDR_LEN;
    /* phdr->bmc_addr set in open_session */
    phdr->target_addr = pcmd[2];
    phdr->target_chan = pcmd[3];
    phdr->target_lun  = (pcmd[1] & 0x03);
    phdr->target_netfn = (pcmd[1] >> 2);
    phdr->target_cmd   = pcmd[0];

    if (phdr->seq_num != 0) pconn->finsession = 1;
    if ( ((phdr->target_cmd == CMD_ACTIVATE_SESSION) ||
         (phdr->target_cmd == CMD_SET_SESSION_PRIV)) &&
         (phdr->target_netfn == NETFN_APP) ) {
       pconn->finsession = 1;  /*so do seq_num*/
       fdoauth = 1;     /*use msg auth*/
    }
    if (phdr->auth_type == IPMI_SESSION_AUTHTYPE_NONE) fdoauth = 0; 
    else if (pconn->finsession && (pconn->fMsgAuth == 0)) {
	/* Forcing the type may be necessary with IBM eServer 360S. */
	if (fauth_type_set) fdoauth = 1;  /*user set it, so try anyway*/
	else fdoauth = 0; /*auth not supported*/
    }
    if (fdoauth == 0) hlen = RQ_HDR_LEN - 16;

    /* copy command */
    if (scmd < SZ_CMD_HDR) return(LAN_ERR_INVPARAM);
    sdata = scmd - SZ_CMD_HDR;          /* scmd = 4 + datalen */
    msglen = 7 + sdata;
    if (fdebuglan > 2) {
        dbglog("_send_lan_cmd: cmd=%02x, hlen=%d, msglen=%02x, authtype=%02x\n",
		pcmd[0], hlen, msglen, phdr->auth_type);
    }
    clen = hlen + msglen;
    if (clen > sizeof(cbuf)) {
        fprintf(fpdbg,"message size %d > buffer size %lu\n",clen,sizeof(cbuf));
        return(LAN_ERR_TOO_SHORT);
    }

    if ((phdr->target_cmd == SOL_DATA) && 
         (phdr->target_netfn == NETFN_SOL) )  /*SOL 1.5 data packet*/
    { 
        hlen = SOL_HDR_LEN;   /*RMCP header + 26 */
        if (fdoauth == 0) hlen = SOL_HDR_LEN - 16;
        msglen = sdata;
        memcpy(&cbuf[0], phdr, 4);   /* copy RMCP header to buffer */
        pdata = &cbuf[4];
        pdata[0] = phdr->auth_type;
        memcpy(&pdata[1],&phdr->seq_num,4);
        sess_id_tmp = phdr->sess_id | SOL_MSG;
        memcpy(&pdata[5],&sess_id_tmp,4);
        if (fdebuglan > 2)
           dbglog("auth_type=%x/%x fdoauth=%d hlen=%d seq_num=%x\n", /*SOL*/
		 phdr->auth_type,lanp.auth_type,fdoauth,hlen,phdr->seq_num);
        if (fdoauth) {
           psessid = (uchar *)&sess_id_tmp;
           do_hash(phdr->password, psessid, &cbuf[hlen],msglen, 
		   phdr->seq_num, phdr->auth_type, iauth);
           /* copy hashed authcode to header */
           memcpy(&pdata[9],iauth,16);
        }
        // pdata[hlen-1] = msglen;
        pdata = &cbuf[hlen];
        memcpy(pdata,&pcmd[SZ_CMD_HDR],msglen); /*copy the data*/
        clen = hlen + msglen;
        if (fdebuglan > 2)
            dbg_dump("sol data, before",pdata,sdata,1);  /*SOL  TEST*/
    } else {   /*not SOL packet, normal IPMI packet*/
        pdata = &cbuf[hlen];
        j = cs1 = 0;
	if ((phdr->target_addr == phdr->bmc_addr) || !bridgePossible ||
	    (phdr->target_addr == SWID_REMOTE) ||
	    (phdr->target_addr == SWID_SMSOS)) {
	    phdr->bridge_level = 0;
            msglen = sdata + 7;
            clen = hlen + msglen;
	} else {
	    phdr->bridge_level = 1;
	    if (phdr->transit_addr != 0 && phdr->transit_addr != phdr->bmc_addr)
	    {
               msglen = sdata + 15 + 8;
	       phdr->bridge_level++;
	    } else {
               msglen = sdata + 15;
	    }
	    if (fdebuglan > 2) {
		dbglog("bridge_level=%d cmd=%02x target=%02x transit=%02x\n",
				phdr->bridge_level, phdr->target_cmd, 
				phdr->target_addr, phdr->transit_addr);
		dbglog("bridge  target_ch=%x transit_ch=%x\n",
				phdr->target_chan, phdr->transit_chan);
	    }
            clen = hlen + msglen;
            pdata[j++] = bmc_sa;           /*[0]=sa */
            pdata[j++] = (NETFN_APP << 2); /*[1]=netfn/lun*/
            pdata[j] = cksum(&pdata[cs1],j-cs1); /*[2]=cksum1*/
	    j++;
            cs3 = j;  /*start for cksum3*/
	    pdata[j++] = phdr->swid;     /*[3]=swid, usu SWID_REMOTE */
            pdata[j++] = (phdr->swseq << 2); /*[4]=swseq/lun*/
            pdata[j++] = CMD_SEND_MESSAGE;    /*[5]=cmd*/
	    if (phdr->bridge_level == 1) {
		pdata[j++] = (0x40|phdr->target_chan); /*channel*/
	    } else {   /*double bridge*/
		pdata[j++] = (0x40|phdr->transit_chan); /*channel*/
		cs1 = j;
		pdata[j++] = phdr->transit_addr;   /*[0]=sa */
		pdata[j++] = (NETFN_APP << 2);     /*[1]=netfn/lun*/
		pdata[j] = cksum(&pdata[cs1],j-cs1); /*[2]=cksum1*/
		j++;
                cs4 = j;  /*start for cksum4*/
		pdata[j++] = bmc_sa;           /*[3]=swid */
		pdata[j++] = (phdr->swseq << 2); /*[4]=swseq/lun*/
		pdata[j++] = CMD_SEND_MESSAGE;    /*[5]=cmd*/
		pdata[j++] = (0x40|phdr->target_chan); /*channel*/
	    }
	}  /*end-else bridged*/
        /* normal IPMI LAN commmand packet */
        cs1 = j;  /*start for cksum1*/
        pdata[j++] = pcmd[2];        /*[0]=sa (phdr->target_addr)*/
        pdata[j++] = pcmd[1];        /*[1]=netfn/lun*/
        pdata[j] = cksum(&pdata[cs1],j-cs1); /*[2]=cksum1*/
	j++;
        cs2 = j;  /*start for cksum2*/
	if (phdr->bridge_level == 0) 
	   pdata[j++] = phdr->swid;     /*[3]=swid*/
	else  /*bridged message*/
	   pdata[j++] = phdr->bmc_addr; /*[3]=swid*/
        pdata[j++] = (phdr->swseq << 2) + phdr->swlun; /*[4]=swseq/lun*/
        pdata[j++] = phdr->target_cmd;       /*[5]=cmd (pcmd[0])*/
	if (sdata > 0) {
	   memcpy(&pdata[j],&pcmd[SZ_CMD_HDR],sdata); /*[6]=data*/
	   j += sdata;
	}
        pdata[j] = cksum(&pdata[cs2],j-cs2); /*cksum2*/
	j++;

	if (phdr->bridge_level > 0) {
	   if (phdr->bridge_level > 1) {
		pdata[j] = cksum(&pdata[cs4],j-cs4); /*cksum4*/
		j++;
	   }
	   pdata[j] = cksum(&pdata[cs3],j-cs3); /*cksum3*/
	   j++;
	}
	if (fdebuglan && (msglen != j)) 
	    fprintf(fpdbg,"warning: msglen(%d)!=j(%d), hlen=%d clen=%d\n",
			msglen,j,hlen,clen);
    
        if (fdoauth) {
           psessid = (uchar *)&phdr->sess_id;
           do_hash(phdr->password, psessid, &cbuf[hlen],msglen, 
		   phdr->seq_num, phdr->auth_type, iauth);
           /* copy hashed authcode to header */
           memcpy(phdr->auth_code,iauth,16);
        }
        memcpy(&cbuf[0], phdr, hlen);   /* copy header to buffer */
    }  /*end-else normal IPMI */

    if (fdoauth == 0 && phdr->auth_type != IPMI_SESSION_AUTHTYPE_NONE) {  
        /* force the packet auth type to NONE (0) */
        IPMI_HDR *pchdr;
        pchdr = (IPMI_HDR *)&cbuf[0];
        pchdr->auth_type = IPMI_SESSION_AUTHTYPE_NONE;
        // memset(phdr->auth_code,0,16);
    }
    cbuf[hlen-1] = (uchar)msglen;     /* IPMI Message Length = 7 + data */
    if (fdebuglan > 2) {
       if ((phdr->target_cmd == SOL_DATA) && 
           (phdr->target_netfn == NETFN_SOL) )  {
          // phdr->sess_id = sess_id_sav; /*restore sess_id*/
          if (fdebuglan) fprintf(fpdbg,"sending lan sol data (len=%d)\n",clen);
       } else {
          if (fdebuglan) fprintf(fpdbg,"sending ipmi lan data (len=%d)\n",clen);
       }
    }
    flags = 0;
    rlen = 0;

    if ((fdebuglan > 2)  &&
        (phdr->target_cmd == CMD_GET_CHAN_AUTH_CAP) && 
        (phdr->target_netfn == NETFN_APP) ) 
        dbg_dump("get_chan_auth_cap command",cbuf,clen,1);
    memset(rbuf,0,sizeof(rbuf));
    fsentok = 0;
    for (itry = 0; (itry < ipmi_try) && (rlen == 0); itry++)
    {
      if (fdebuglan > 2)
          dbglog("ipmilan_cmd(seq=%x) fsentok=%d itry=%d\n",
		   phdr->seq_num, fsentok, itry);
      if (fsentok == 0) {
         if (fdebuglan) 
           fprintf(fpdbg,"ipmilan_sendto(seq=%x,clen=%d)\n",
		   phdr->seq_num, clen);
         sz = ipmilan_sendto(s,cbuf,clen,flags,to,tolen);
         if (sz < 1) {
           lasterr = get_LastError();
           if (fdebuglan) show_LastError("ipmilan_sendto",lasterr);
           rv = LAN_ERR_SEND_FAIL; 
           os_usleep(0,5000);
           continue;  /* retry  */
         }
         fsentok = 1; /*sent ok, no need to resend*/
      }

      /* receive the response */
      rv = fd_wait(s, ipmi_timeout,0);
      if (rv != 0) {
        if (fdebuglan)
           fprintf(fpdbg,"ipmilan_cmd timeout, after request, seq=%x itry=%d\n",
		   phdr->seq_num, itry);
        rv = LAN_ERR_RECV_FAIL;
        if (fdopoke2) ipmilan_poke2(s, to, tolen);
        os_usleep(0,5000);
        continue;  /* retry  */
      }
      flags = RECV_MSG_FLAGS;
      rlen = ipmilan_recvfrom(s,rbuf,sizeof(rbuf),flags,to,&tolen);
      if (rlen < 0) {
        lasterr = get_LastError();
        if (fdebuglan) { 
           fprintf(fpdbg,"ipmilan_recvfrom rlen=%d, err=%d iseq=%x itry=%d\n", 
			rlen,lasterr,phdr->iseq_num,itry);
           show_LastError("ipmilan_recvfrom",lasterr);
        }
        rv = rlen;   /* -3 = LAN_ERR_RECV_FAIL */
        rlen = 0;
        *sresp = rlen;
	/* Sometimes the OS sends an ECONNREFUSED error, but
	 * retrying will catch the BMC's reply packet. */
        if (lasterr == econnrefused) continue;  /*try again*/
        else if (lasterr == econnreset) continue;  /*try again*/
        else break; /* goto EXIT; */
      } else {  /* successful receive */
        net2h(&phdr->iseq_num,&rbuf[5],4);  /*incoming seq_num from hdr*/
        if (fdebuglan) {
           fprintf(fpdbg,"ipmilan_recvfrom rlen=%d, iseq=%x\n", 
			rlen,phdr->iseq_num);
	   if (fdebuglan > 2)
              dbg_dump("ipmilan_recvfrom", rbuf,rlen,1);
        }

        /* incoming auth_code may differ from request auth code, (Dell 1855)*/
        if (rbuf[4] == IPMI_SESSION_AUTHTYPE_NONE) {  /* if AUTH_NONE */
            phdr->auth_type = IPMI_SESSION_AUTHTYPE_NONE;
            hlen = RQ_HDR_LEN - 16;  /*RMCP header + 10*/
	} else {
            hlen = RQ_HDR_LEN;   /*RMCP header + 26 */
            // memcpy(phdr->iauth_code,&rbuf[13],16); /*iauthcode*/
        }

        i = hlen + 6;
        if (rlen <= i) rv = LAN_ERR_TOO_SHORT;
        else {              /* successful */
          n = rlen - i - 1;
	  if (bridgePossible && (phdr->target_addr != bmc_sa)) {
	     if (phdr->bridge_level &&
		 ((rbuf[hlen+1] >> 2) == (NETFN_APP + 1)) &&  /*0x07*/
		 rbuf[hlen+5] == CMD_SEND_MESSAGE)   /*0x34*/
	     {
		phdr->bridge_level--;
		if (n <= 1) { /* no data, wait for next */
		   if (fdebuglan) 
		      fprintf(fpdbg,"bridged response empty, cc=%x\n",rbuf[i]);
		   if (rbuf[i] == 0) { /* if sendmsg cc==0, recv again*/
		      rlen = 0;
		      continue;  
		   }  /*else done*/
		} else { /* has data, copy it*/
		   //if (fdebuglan) fprintf(fpdbg,"bridged response, n=%d\n",n);
		   // memmove(&rbuf[i-7],&rbuf[i],n); 
		   phdr->swseq = rbuf[i-3] >> 2; /*needed?*/
		   rbuf[i-8] -= 8;
		   n -= 8;
		   i -= 7;
		   if (fdebuglan) dump_buf("bridged response",&rbuf[i],n,1);
		   continue;  /*recv again*/
		}
	     }
	  } /*endif bridge*/
          if (n > *sresp) n = *sresp;
          memcpy(presp,&rbuf[i],n);
          *sresp = n;
          rv = 0;
        }
      }  /*end else success*/
    } /*end for loop*/
// EXIT:
#ifdef NOT
    /* do not increment sequence numbers for SEND_MESSAGE command */
    if ((phdr->target_cmd == IPMB_SEND_MESSAGE) && 
        (phdr->target_netfn == NETFN_APP) ) 
        pconn->finsession = 0;
#endif
    if (pconn->finsession) {
        /* increment seqnum - even if error */
        phdr->seq_num = inc_seq_num( phdr->seq_num );
        if (rlen > 0) pconn->in_seq = phdr->iseq_num;
        else pconn->in_seq = inc_seq_num(pconn->in_seq); 
        phdr->swseq = (uchar)inc_seq_num(phdr->swseq); 
    }
    return(rv);
}   /*end _send_lan_cmd*/

static char *auth_type_str(int authcode)
{
   char *pstr;
   switch(authcode) {
   case IPMI_SESSION_AUTHTYPE_MD5:  pstr = "MD5";  break;
   case IPMI_SESSION_AUTHTYPE_MD2:  pstr = "MD2";  break;
   case IPMI_SESSION_AUTHTYPE_PASSWORD: pstr = "PSWD"; break;
   case IPMI_SESSION_AUTHTYPE_OEM:  pstr = "OEM";  break;
   case IPMI_SESSION_AUTHTYPE_NONE: pstr = "NONE"; break;
   default: pstr = "Init"; break; /* AUTHTYPE_INIT, etc. */
   }
   return (pstr);
}

#ifdef NOT_USED
static uchar auth_type_mask(int authcode, uchar allowed)
{
   uchar mask;
   switch(authcode) {
   case IPMI_SESSION_AUTHTYPE_MD5:  
	mask = IPMI_MASK_AUTHTYPE_MD5;
	break;
   case IPMI_SESSION_AUTHTYPE_MD2:  
	mask = IPMI_MASK_AUTHTYPE_MD2;
	break;
   case IPMI_SESSION_AUTHTYPE_PASSWORD: 
	mask = IPMI_MASK_AUTHTYPE_PASSWORD;
	break;
   case IPMI_SESSION_AUTHTYPE_OEM: 
	mask = IPMI_MASK_AUTHTYPE_OEM;
	break;
   case IPMI_SESSION_AUTHTYPE_NONE: 
	mask = IPMI_MASK_AUTHTYPE_NONE;
	break;
   default: /* AUTHTYPE_INIT, etc. */
	mask = 0;  
	break;
   }
   return (mask);
}
#endif

/* 
 * ipmilan_open_session
 * Performs the various command/response sequence needed to 
 * initiate an IPMI LAN session.
 */
static int ipmilan_open_session(SockType sfd, struct sockaddr *destaddr, 
			int destaddr_len, uchar auth_type, char *username, 
			char *authcode, int authcode_len, 
			uchar priv_level, uint32 init_out_seqnum, 
			uint32 *session_seqnum, uint32 *session_id)
{
    int rv = 0;
    uchar ibuf[RQ_LEN_MAX+3];
    uchar rbuf[RS_LEN_MAX+4];
    uint32 iseqn;
    uchar ipasswd[16];
    uchar iauthtype;
    uchar iauthcap, imsgauth;
    int rlen, ilen;
    IPMI_HDR *phdr;
    uchar cc;
    int busy_tries = 0;

    if (fdebuglan) 
        fprintf(fpdbg,"ipmilan_open_session(%d,%02x,%s,%02x,%x) called\n",
		sfd,auth_type,username,priv_level,init_out_seqnum);
    if (sfd == 0 || destaddr == NULL) return LAN_ERR_INVPARAM;
    phdr = &ipmi_hdr;
    /* Initialize ipmi_hdr fields */
    memset(phdr,0,sizeof(ipmi_hdr));
    phdr->rmcp_ver  = 0x06; 
    phdr->rmcp_res  = 0x00; 
    phdr->rmcp_seq  = 0xFF; 
    phdr->rmcp_type = 0x07; 
    phdr->swid  = sms_swid;
    phdr->swseq = 1; 
    phdr->priv_level = priv_level; 

    /* Get Channel Authentication */
    phdr->auth_type = IPMI_SESSION_AUTHTYPE_NONE; /*use none(0) at first*/
    ibuf[0] = 0x0e;  /*this channel*/
    ibuf[1] = phdr->priv_level; 
    rlen = sizeof(rbuf);
    if (fdebuglan) 
        fprintf(fpdbg,"GetChanAuth(sock %x, level %x) called\n",sfd,ibuf[1]);
    rv = _ipmilan_cmd(sfd,  destaddr, destaddr_len, CMD_GET_CHAN_AUTH_CAP, 
                  NETFN_APP,BMC_LUN,bmc_sa, PUBLIC_BUS,
		  ibuf,2, rbuf,&rlen, fdebuglan);
    if (rv != 0) {  /*retry if error*/
        rv = _ipmilan_cmd(sfd,  destaddr, destaddr_len, CMD_GET_CHAN_AUTH_CAP, 
                     NETFN_APP,BMC_LUN,bmc_sa, PUBLIC_BUS,
                     ibuf,2, rbuf,&rlen, fdebuglan);
    }
    cc = rbuf[0];
    if (fdebuglan) 
	 fprintf(fpdbg,"GetChanAuth rv = %d, cc=%x rbuf: %02x %02x %02x "
		 		"%02x %02x %02x %02x\n",
		  rv, rbuf[0],rbuf[1],rbuf[2],rbuf[3],
		      rbuf[4],rbuf[5],rbuf[6],rbuf[7]);
    if (rv != 0 || cc != 0) goto ERREXIT; 

    /* Check Channel Auth params */
    if ((rbuf[2] & 0x80) != 0) {  /*have IPMI 2.0 extended capab*/
        if ( ((rbuf[4]&0x02) != 0) && ((rbuf[4]&0x01) == 0) ) {
            if (fdebuglan)
                fprintf(fpdbg,"GetChanAuth reports only v2 capability\n");
            rv = LAN_ERR_V2;  /*try v2 instead*/
            goto ERREXIT;
        } else {
	    /* Always switch to IPMI LAN 2.0 if detected. */
	    /* This avoids errors from Dell & Huawei firmware */
            if (fdebuglan)
                fprintf(fpdbg,"GetChanAuth detected v2, so switch to v2\n");
            rv = LAN_ERR_V2;  /*use v2 instead*/
            goto ERREXIT;
	}
    }
    /* Check authentication support */
    imsgauth = rbuf[3];
    if ((imsgauth & 0x10) == 0) pconn->fMsgAuth = 1;  /*per-message auth*/
    else if ((imsgauth & 0x08) == 0) pconn->fMsgAuth = 2;  /*user-level auth*/
    else pconn->fMsgAuth = 0;   /*no auth support*/
    iauthcap = rbuf[2] & 0x3f;
    if (fauth_type_set) {
        iauthtype = (uchar)lanp.auth_type;  // set by user 
	auth_type = iauthtype;
    } else {
        iauthtype = AUTHTYPE_INIT;  /*initial value, not set*/
    }
    if (auth_type != iauthtype) {
	/* default of MD5 is preferred, but try what BMC allows */
	if (iauthcap & IPMI_MASK_AUTHTYPE_MD5) {
	   iauthtype = IPMI_SESSION_AUTHTYPE_MD5;
	   auth_type = iauthtype;
#ifdef MD2OK
	} else if (iauthcap & IPMI_MASK_AUTHTYPE_MD2) {
	   iauthtype = IPMI_SESSION_AUTHTYPE_MD2;
	   auth_type = iauthtype;
	   if (fdebuglan) 
              fprintf(fpdbg,"auth_type set to MD2 (%02x)\n",iauthtype);
#endif
	} else if (iauthcap & IPMI_MASK_AUTHTYPE_PASSWORD) {
	   iauthtype = IPMI_SESSION_AUTHTYPE_PASSWORD;
	   auth_type = iauthtype;
	   if (fdebuglan) 
              fprintf(fpdbg,"auth_type set to Password (%02x)\n",iauthtype);
	} else {
	   if (fdebuglan) fprintf(fpdbg,
			"auth_type set to %02x, using None\n",auth_type);
	   iauthtype = IPMI_SESSION_AUTHTYPE_NONE;
	   auth_type = iauthtype;
	}
    }
    if (fdebuglan) fprintf(fpdbg,
	    "auth_type=%02x(%s) allow=%02x iauthtype=%02x msgAuth=%d(%02x)\n",
		auth_type,auth_type_str(auth_type),iauthcap,iauthtype,
		pconn->fMsgAuth,imsgauth);

    /* get session challenge */
    phdr->auth_type = IPMI_SESSION_AUTHTYPE_NONE;
    memset(ibuf,0,17);
    ibuf[0] = iauthtype;
    if (username != NULL) 
        strncpy(&ibuf[1],username,16);
    while (busy_tries < BUSY_MAX) {
       rlen = sizeof(rbuf);
       rv = _ipmilan_cmd(sfd, destaddr,destaddr_len, CMD_GET_SESSION_CHALLENGE,
		     NETFN_APP,BMC_LUN,bmc_sa, PUBLIC_BUS,
		     ibuf,17, rbuf,&rlen, fdebuglan);
       cc = rbuf[0];
       if (rv != 0) break;
       else if (cc == 0xc0) busy_tries++;
       else break;
    }
    if (fdebuglan) {
	if ((rv == 0) && (cc == 0))
           dump_buf("GetSessionChallenge rv=0, rbuf",rbuf,rlen,0);
	else
	   fprintf(fpdbg,"GetSessionChallenge rv=%d cc=%x rlen=%d tries=%d\n", 
			rv, cc, rlen,busy_tries);
    }
    if (rv != 0) goto ERREXIT;
    else if (cc != 0) { cc_challenge(cc); goto ERREXIT; }

    /* save challenge response data */
    memcpy(&phdr->sess_id,  &rbuf[1], 4);
    memcpy(phdr->challenge, &rbuf[5], 16);

    /* Save authtype/authcode in ipmi_hdr for later use in _send_lan_cmd. */
    phdr->bmc_addr = bmc_sa;
    phdr->auth_type = iauthtype;
    if (authcode_len > 16 || authcode_len < 0) authcode_len = 16;
    memset(&ipasswd,0,16);
    if (authcode != NULL && authcode_len > 0) 
        memcpy(&ipasswd,(uchar *)authcode,authcode_len); /* AuthCode=passwd */
    memcpy(phdr->password,&ipasswd,16);              /* save password */

    /* ActivateSession request */
    ibuf[0] = phdr->auth_type;  
    ibuf[1] = phdr->priv_level;
    if (vend_id == VENDOR_SUPERMICRO) {
          /* if supermicro, do special auth logic here */
          hash_special(phdr->password, phdr->challenge, ipasswd);
          memcpy(phdr->password,ipasswd,16); 
          memset(&ibuf[2],0,16);   /*zero challenge string here*/
          if (fdebuglan) printf("Using supermicro OEM challenge\n");
    } else {
          memcpy(&ibuf[2],phdr->challenge,16); /*copy challenge string to data*/
    }
    phdr->seq_num = 0;
    iseqn = init_out_seqnum; 
    h2net(iseqn,&ibuf[18],4);       /* write iseqn to buffer */
    ilen = 22;
    if (fdebuglan) dump_buf("ActivateSession req",ibuf,ilen,0);

    rlen = sizeof(rbuf);
    rv = _ipmilan_cmd(sfd,destaddr,destaddr_len,CMD_ACTIVATE_SESSION,
		     NETFN_APP,BMC_LUN,bmc_sa, PUBLIC_BUS,
		     ibuf, ilen, rbuf, &rlen, fdebuglan);
    cc = rbuf[0];
    if (fdebuglan) {
        if (rv > 0) fprintf(fpdbg,"ActivateSession rv = 0x%02x\n",rv); /*cc*/
        else fprintf(fpdbg,"ActivateSession rv = %d\n",rv);
    }
    if (rv != 0) goto ERREXIT;
    else if (cc != 0) { cc_session(cc); goto ERREXIT; }

    if (pconn->fMsgAuth == 2)  /*user-level auth*/
       phdr->auth_type = IPMI_SESSION_AUTHTYPE_NONE;

    memcpy(&phdr->sess_id,&rbuf[2],4);  /* save new session id */
    net2h(&iseqn,  &rbuf[6],4);     /* save returned out_seq_num */
    if (iseqn == 0) iseqn = inc_seq_num(iseqn); /* was ++iseqn */
    phdr->seq_num  = iseqn;   /* new session seqn */
    if (fdebuglan)
        fprintf(fpdbg,"sess_id=%x seq_num=%x priv_allow=%x priv_req=%x\n",
		phdr->sess_id,phdr->seq_num,rbuf[10],phdr->priv_level);

    /* set session privileges (set_session_privilege_level) */
    ibuf[0] = phdr->priv_level;
    rlen = sizeof(rbuf);
    rv = _ipmilan_cmd(sfd,  destaddr, destaddr_len, CMD_SET_SESSION_PRIV, 
		     NETFN_APP,BMC_LUN,bmc_sa, PUBLIC_BUS,
		     ibuf,1, rbuf,&rlen, fdebuglan);
    cc = rbuf[0];
    if (fdebuglan) fprintf(fpdbg,"SetSessionPriv(%x) rv = %d\n",ibuf[0], rv);

    bridgePossible = 1;
    *session_id     = phdr->sess_id;
    *session_seqnum = phdr->seq_num;
ERREXIT:
    if (rv == 0 && cc != 0) rv = cc;
    return(rv);
}      /*end ipmilan_open_session*/

/* 
 * ipmilan_close_session
 */
static int ipmilan_close_session(SockType sfd, struct sockaddr *destaddr, 
			int destaddr_len, uint32 session_id)
{
    uchar ibuf[RQ_LEN_MAX+3];
    uchar rbuf[RS_LEN_MAX+4];
    int rlen;
    int rv = 0;

    if (session_id == 0) return(0);
    /* send close session command */
    memcpy(ibuf,&session_id,4);
    rlen = sizeof(rbuf);
    bridgePossible = 0;
    rv = _ipmilan_cmd(sfd,  destaddr, destaddr_len, CMD_CLOSE_SESSION, 
                  NETFN_APP,BMC_LUN,bmc_sa,PUBLIC_BUS,
		  ibuf,4, rbuf,&rlen, fdebuglan);
    if (fdebuglan) fprintf(fpdbg,"CloseSession rv = %d, cc = %02x\n",
			  rv, rbuf[0]);
    if (rbuf[0] != 0) rv = rbuf[0];  /*comp code*/
    if (rv == 0) pconn->session_id = 0;
    ipmi_hdr.seq_num  = 0;
    ipmi_hdr.swseq    = 1;
    ipmi_hdr.iseq_num = 0;
    ipmi_hdr.sess_id  = 0;
    pconn->finsession = 0;
    return(rv);
}


int rmcp_ping(SockType sfd, struct sockaddr *saddr, int saddr_len, int foutput)
{
   /* The ASF spec says to use network byte order */
   uchar asf_pkt[40] = {06,0,0xFF,06,0x00,0x00,0x11,0xBE,0x80,0,0,0 };
   struct sockaddr from_addr;
   int from_len; 
   int rv;
   int iana;

   /* Send RMCP ASF ping to verify IPMI LAN connection. */
	asf_pkt[9] = 1; /*tag*/	
	rv = ipmilan_sendto(sfd, asf_pkt, 12, 0, saddr, saddr_len);
	if (foutput) 
		fprintf(fpdbg,"ipmilan ping, sendto len=%d\n",rv);
	if (rv < 0) return(LAN_ERR_PING);
        pconn->connect_state = CONN_STATE_PING; /*ping was sent ok*/

	from_len = sizeof(struct sockaddr);
        rv = fd_wait(sfd,ping_timeout,0);
        if (rv != 0) {
            fprintf(fpdbg,"ping timeout, after %s\n",
			conn_state_str[pconn->connect_state]);
	    rv = LAN_ERR_CONNECT;
        } else {
	   rv = ipmilan_recvfrom(sfd, asf_pkt, sizeof(asf_pkt), 0,
			       &from_addr,&from_len);
	   if (foutput) {
		fprintf(fpdbg,"ipmilan pong, recvfrom len=%d\n", rv);
		if (rv > 0) {
		    iana = asf_pkt[15] + (asf_pkt[14] << 8) + 
				(asf_pkt[13] << 16) + (asf_pkt[12] << 24);
		    dump_buf("ping response",asf_pkt, rv,0);
		    printf("ping IANA = %d (%s)\n",iana,get_iana_str(iana));
		}
	   }
	   if (rv < 0) return(LAN_ERR_CONNECT);
        }
   return(0);
}

int ping_bmc(char *node, int fdebugcmd)
{
    SOCKADDR_T  toaddr;
    int toaddr_len;
    SockType sfd;
    int rv;

    rv = open_sockfd(node, RMCP_PRI_RMCP_PORT, &sfd, &toaddr, &toaddr_len, fdebugcmd);
    if (rv != 0) return(rv);

    rv = rmcp_ping(sfd, (struct sockaddr *)&toaddr,toaddr_len, fdebugcmd);

    close_sockfd(sfd);
    return(rv);
}

static int get_rand(void *data, int len)
{
        int rv = 0;
        int fd;

#if defined(LINUX)
	fd = open("/dev/urandom", O_RDONLY);
        if (fd < 0 || len < 0) return errno;
        rv = read(fd, data, len);
        close(fd);
#else 
	fd = rand();
	if (fd == 0) fd = 1;
	if (len > sizeof(int)) len = sizeof(int);
	memcpy(data,&fd,len);
#endif
        return rv;
}

/* 
 * ipmi_open_lan
 */
int ipmi_open_lan(char *node, int port, char *user, char *pswd, int fdebugcmd)
{
   char *username;
   uchar priv_level;
   int rv = -1;
#ifndef HAVE_IPV6
   char *temp;
#endif

#ifndef TEST_LAN
   fdebuglan = fdebugcmd;
   if (fdebugcmd) fprintf(fpdbg,"ipmi_open_lan: fdebug = %d\n",fdebugcmd);
#endif
   if (fdebugcmd > 2) fdoping = 1;
   get_mfgid(&vend_id,&prod_id);
   if (nodeislocal(node)) {
        fprintf(fpdbg,"ipmi_open_lan: node %s is local!\n",node);
        rv = LAN_ERR_INVPARAM;
        goto EXIT;
   } else {

        if (fdebugcmd) 
	   fprintf(fpdbg,"Opening lan connection to node %s ...\n",node);
	/* save nodename for sig_abort later */
	if (strlen(node) > SZGNODE) {
	       strncpy(nodename, node, SZGNODE); nodename[SZGNODE] = 0;
	} else strcpy(nodename, node);
        rv = open_sockfd(node, port, &(pconn->sockfd), &_destaddr, &_destaddr_len, 1);
	if (fdebugcmd)  
	   printf("open_sockfd returned %d, fd=%d\n", rv, pconn->sockfd);
        if (rv != 0) goto EXIT;

#ifdef HAVE_IPV6
	strcpy(gnodename,nodename);
        if (fdebugcmd) 
	   fprintf(fpdbg,"Connecting to node %s\n",gnodename);
#else
#ifdef WIN32
        /* check for ws2_32.lib(getnameinfo) resolution */
        gnodename[0] = 0;
/*
	int getnameinfo( const struct sockaddr  * sa, socklen_t    salen,
	     char  *      host, DWORD        hostlen,
	     char  *      serv, DWORD        servlen,
	     int          flags);
        rv = getnameinfo((SOCKADDR *)&_destaddr, _destaddr_len,
			gnodename,SZGNODE, NULL,0,0);
*/
#else
        rv = getnameinfo((struct sockaddr *)&_destaddr, _destaddr_len,
			gnodename,SZGNODE, NULL,0,0);
#endif
        if (rv != 0) {
            if (fdebugcmd) 
	        fprintf(fpdbg,"ipmi_open_lan: getnameinfo rv = %d\n",rv);
	    gnodename[0] = 0;
        }
	temp = inet_ntoa(_destaddr.sin_addr);
	fprintf(fpdbg,"Connecting to node %s %s\n",gnodename, temp);
	strncpy(_dest_ip, temp, INET_ADDRSTRLEN);
	_dest_ip[INET_ADDRSTRLEN] = 0;
#endif

#ifndef WIN32
	/* Linux: Set up signals to handle errors & timeouts. */
	signal(SIGINT,sig_abort);
	signal(SIGALRM,sig_timeout);
#endif

	pconn->connect_state = CONN_STATE_BIND;

	if (fdoping) {
           rv = rmcp_ping(pconn->sockfd,(struct sockaddr *)&_destaddr,
			_destaddr_len, fdebugcmd);
           if (fdopoke1 && rv != 0) { 
	      /* May sometimes need a poke to free up the BMC (cant hurt) */
	      ipmilan_poke1(pconn->sockfd,(struct sockaddr *)&_destaddr,
				_destaddr_len);
           }
		
           if (rv != 0) {
	       if (rv == LAN_ERR_CONNECT && frequireping == 0) {
                  /* keep going even if ping/pong failure */
                  rv = 0;
               } else {
                  close_sockfd(pconn->sockfd);
                  rv = LAN_ERR_CONNECT;
                  goto EXIT;
               }
           }
	   pconn->connect_state = CONN_STATE_PONG;
        }

	{
            auth_type  = (uchar)lanp.auth_type;
            priv_level = (uchar)lanp.priv;
	    username = user;
	    authcode = pswd;
	    authcode_len = (pswd) ? strlen_(authcode) : 0;
	    if ((vend_id == VENDOR_INTEL) || (vend_id == VENDOR_IBM)) 
		    pconn->start_out_seq = 1;
	    else {
		if (fdebugcmd) 
		    printf("calling get_rand(%d)\n", pconn->start_out_seq);
		get_rand(&pconn->start_out_seq,sizeof(pconn->start_out_seq));
	    }
	}
	rv = ipmilan_open_session(pconn->sockfd, (struct sockaddr *)&_destaddr, 
			_destaddr_len, auth_type, username, 
			authcode, authcode_len,priv_level, pconn->start_out_seq,
			&pconn->in_seq, &pconn->session_id);
	if (rv == 0) { /* successful (session active) */
	   pconn->connect_state = CONN_STATE_ACTIVE; /*set connection state to active*/
	} else {  /* open_session rv != 0 */
           if ((gshutdown==0) || fdebugcmd) {
              if (rv < 0) 
                   fprintf(fpdbg,"ipmilan_open_session error, rv = %d\n",rv);
              else fprintf(fpdbg,"ipmilan_open_session error, rv = 0x%x\n",rv);
           }
           close_sockfd(pconn->sockfd);
        }
   }
EXIT:
   if (rv != 0) {
      // if ((gshutdown==0) || fdebugcmd) 
          printf("ipmilan %s\n",decode_rv(rv));
          if (rv == -1 && lasterr != 0) show_LastError("ipmilan",lasterr);
   }
   return(rv);
}

int ipmi_flush_lan(char *node)
{
   int rv = 0;
   /* could match node via pconn = find_conn(node); */
   if (!nodeislocal(node)) {  /* ipmilan, need to close & cleanup */
	if (pconn->sockfd != 0) close_sockfd(pconn->sockfd);
   } else {  /* kcs cleanup */
#ifndef WIN32
	alarm(0);
	signal(SIGALRM,SIG_DFL);
#endif
   }  /* endif */
   pconn->connect_state = CONN_STATE_INIT;
   pconn->finsession = 0;
   pconn->session_id = 0;
   pconn->sockfd = 0;
   pconn->in_seq = 1;
   pconn->start_out_seq = 1;
   pconn->fMsgAuth = 1; /*1=PerMsgAuth*/
   pconn->auth_type = AUTHTYPE_INIT;
   return (rv);
}

int ipmi_close_lan(char *node)
{
   int rv = 0;

   /* could match node via pconn = find_conn(node); */
   if (fdebuglan) fprintf(fpdbg,"ipmi_close_lan(%s) entry, sockfd=%d\n",
				node,pconn->sockfd);
   if (!nodeislocal(node)) {  /* ipmilan, need to close & cleanup */
	if (pconn->sockfd != 0) {  /* socket is open */
          if (gshutdown) pconn->session_id = 0;
	  if (pconn->session_id != 0) {  /* session is open */
  	    // cmd_rs = buf_rs;
	    rv = ipmilan_close_session(pconn->sockfd, 
			(struct sockaddr *)&_destaddr, 
			  _destaddr_len, ipmi_hdr.sess_id);
	    /* flush session_id even if error, let it time out */
	    pconn->session_id = 0;
	  }
          close_sockfd(pconn->sockfd);
          pconn->sockfd = 0;
	}
	pconn->connect_state = CONN_STATE_INIT;
	pconn->finsession = 0;
   } else {  /* kcs cleanup */
#ifndef WIN32
	alarm(0);
	signal(SIGALRM,SIG_DFL);
#endif
   }  /* endif */
   if (fdebuglan) fprintf(fpdbg,"ipmi_close_lan(%s) rv=%d sockfd=%d\n",
				node,rv,pconn->sockfd);
   return (rv);
}
 
/* 
 * ipmicmd_lan
 * This is called by ipmi_cmd_lan, all commands come through here.
 */
int ipmicmd_lan(char *node, 
		uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
		uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd)
{
   uchar rq_data[RQ_LEN_MAX+3];
   uchar cmd_rs[RS_LEN_MAX+4];
   uchar cc = 0;
   int rlen; 
   int rv = -1;

#ifndef TEST_LAN
   fdebuglan = fdebugcmd;
#endif
   /* check sdata/sresp against MAX_ */
   if (sdata > RQ_LEN_MAX)  {
        if (fdebugcmd) printf("cmd %x sdata(%d) > RQ_LEN_MAX(%d)\n",
				cmd,sdata,RQ_LEN_MAX);
        return(LAN_ERR_BADLENGTH);
   }
   if (*sresp > RS_LEN_MAX) {
        if (fdebugcmd) printf("cmd %x sresp(%d) > RS_LEN_MAX(%d), use less\n",
				 cmd,*sresp,RS_LEN_MAX);
	/* This is ok, just receive up to the max. */
	*sresp = RS_LEN_MAX;
   }
   if (pdata == NULL) { pdata = rq_data; sdata = 0; }
   rlen = *sresp;
 
   if (nodeislocal(node)) {  /*local, use kcs*/
      fprintf(fpdbg,"ipmicmd_lan: node %s is local", node);
      goto EXIT;
   } else { /* ipmilan */
        if (pconn->sockfd == 0) {  /* closed, do re-open */
            if (fdebugcmd)
		fprintf(fpdbg,"sockfd==0, node %s needs re-open\n",node);
            rv = ipmi_open_lan(lanp.node, lanp.port, lanp.user, lanp.pswd, fdebugcmd);
            if (rv != 0) goto EXIT;
        }
        if (fdebugcmd) {
            fprintf(fpdbg,"lan_cmd(seq=%x) %02x %02x %02x %02x, (dlen=%d): ",
		    ipmi_hdr.seq_num, cmd,netfn,lun,sa,sdata);
            dump_buf("cmd data",pdata,sdata,0);
        }
	if (fdebuglan > 2)
           dbglog("calling _ipmilan_cmd(%02x,%02x)\n",cmd,netfn); 
        rlen = sizeof(cmd_rs);
        rv = _ipmilan_cmd(pconn->sockfd, (struct sockaddr *)&_destaddr, 
		_destaddr_len, cmd, netfn, lun, sa, bus, pdata, sdata,
                cmd_rs, &rlen, fdebugcmd);
   }

   cc = cmd_rs[0];
   if (rv == 0 && cc == 0) {  /* success */
	if (fdebugcmd) {
	   fprintf(fpdbg,"lan_rsp rv=0 cc=0 (rlen=%d): ",rlen);
           dump_buf("cmd rsp",cmd_rs,rlen,0);
	}
        rlen--;
        if (rlen > *sresp) {  /*received data > receive buffer*/
           if (fdebugcmd) 
		printf("rlen(%d) > sresp(%d), truncated\n",rlen,*sresp);
	   rlen = *sresp;
        }
	memcpy(presp,&cmd_rs[1],rlen);
	*sresp = rlen;
   } else {  /* error */
        if (fdebugcmd) 
	   fprintf(fpdbg,"ipmicmd_lan: cmd=%02x rv=%d, cc=%02x, rlen=%d\n",
		   cmd,rv,cc,rlen);
	presp[0] = 0; /*memset(presp,0,*sresp);*/
	*sresp = 0;
   }
 
EXIT:
   *pcc = cc;
   return(rv);
}  /*end ipmicmd_lan()*/

/* 
 * ipmi_cmd_lan
 * This is the entry point, called from ipmicmd.c
 */
int ipmi_cmd_lan(char *node, ushort cmd, uchar *pdata, int sdata,
                uchar *presp, int *sresp, uchar *pcc, char fdebugcmd)
{
    int rc, i;
    uchar mycmd;

    for (i = 0; i < NCMDS; i++) {
       if (ipmi_cmds[i].cmdtyp == cmd) break;
    }
    if (i >= NCMDS) {
        fprintf(fperr, "ipmi_cmd_lan: Unknown command %x\n",cmd);
        return(-1);
        }
   if (cmd >= CMDMASK) mycmd = (uchar)(cmd & CMDMASK);  /* unmask it */
   else mycmd = (uchar)cmd;
   if (fdebuglan > 2)
      dbglog("ipmi_cmd_lan: cmd=%04x, mycmd=%02x\n",cmd,mycmd);
   rc = ipmicmd_lan(node,mycmd,ipmi_cmds[i].netfn,ipmi_cmds[i].lun,
                ipmi_cmds[i].sa, ipmi_cmds[i].bus, 
		pdata,sdata,presp,sresp,pcc,fdebugcmd);
   return (rc);
}

int ipmi_cmdraw_lan(char *node, uchar cmd, uchar netfn, uchar lun, uchar sa,
		uchar bus, uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd)
{
   int rc;
   if (fdebuglan > 2)
      dbglog("ipmi_cmdraw_lan: cmd=%02x, netfn=%02x\n",cmd,netfn);
   /* bus is not used for lan */
   rc = ipmicmd_lan(node, cmd, netfn, lun, sa, bus,
                    pdata,sdata,presp,sresp,pcc,fdebugcmd);
   return (rc);
}


SockType  lan_get_fd(void)
{
   return(pconn->sockfd);
}

/* static SOL v1.5 encryption routines */

static void sol15_cipherinit( uchar SeedCount, char *password, uint32 out_seq)
{
	uchar temp[ 40 ]; /* 16 + 4 + 8 + 4 = 32 bytes */
        int i;

        i = SeedCount & 0x0f;

        srand((unsigned int)time(NULL));
        g_Seed[i] = (int32_t)rand();

        if (password == NULL) 
             memset(&temp[0], 0, 16);
        else memcpy(&temp[0], password,16); /*16-byte password*/
        h2net(g_Seed[i],&temp[16],4);       /*4-byte seed */
	memset(&temp[20], 0, 8 );           /*8-byte pad*/
        h2net(out_seq,&temp[28],4);         /*4-byte seq num */
        md5_sum(temp,32, g_Cipher[i]);
}

#ifdef NOT_USED
static void sol15_encrypt( uchar *dst, uchar *src, int len, uchar SeedCount )
{
     uchar cipher;
     unsigned int val;
     int i;

	SeedCount &= 0x0f;
	if ( sol_Encryption ) {
		for (i = 0; i < len; i++, dst++, src++) {
			cipher = g_Cipher[ SeedCount ][ (i & 0x0f) ];
			val = (*src) << (cipher & 0x07);
			*dst = (uchar)((val | (val >> 8)) ^ cipher);
		}
	} else {
		memcpy( dst, src, len );
	}
}

static void sol15_decrypt( uchar *dst, uchar *src, int len, uchar SeedCount )
{
     uchar cipher;
     unsigned int val;
     int i;
	SeedCount &= 0x0f;
	if ( sol_Encryption ) {
		for (i = 0; i < len; i++, dst++, src++) {
			cipher = g_Cipher[ SeedCount ][ (i & 0x0f) ];
			val = ((*src) ^ cipher) << 8;
			val >>= (cipher & 0x07);
			*dst = (uchar)((val >> 8) | val);
		}
	} else {
		memcpy( dst, src, len );
	}
}
#endif

/* 
 * lan_get_sol_data
 * Called before ACTIVATE_SOL1 command 
 */
void lan_get_sol_data(uchar fEnc, uchar seed_cnt, uint32 *seed)
{
   if (seed_cnt != sol_seed_cnt && (seed_cnt < 16)) 
	sol_seed_cnt = seed_cnt;
   pconn->start_out_seq = ipmi_hdr.seq_num;
   sol_snd_seq   = (uchar)pconn->start_out_seq;
   sol15_cipherinit(sol_seed_cnt, lanp.pswd, pconn->start_out_seq);
   *seed = g_Seed[sol_seed_cnt];
   if (fdebuglan > 2)
      dbglog("lan_get_sol_data: %02x %02x %02x\n",   /*SOL*/
		fEnc, seed_cnt, ipmi_hdr.seq_num);
}

/* 
 * lan_set_sol_data
 * Called after ACTIVATE_SOL1 response received 
 */
void lan_set_sol_data(uchar fenc, uchar auth, uchar seed_cnt, 
		      int insize, int outsize)
{
   if (fdebuglan > 2)
      dbglog("lan_set_sol_data: %02x %02x %02x %02x\n",   /*SOL*/
		auth,seed_cnt,insize,outsize);
   if (fenc || (auth & 0x07) == 1) {
      sol_op = 0x80;  /*OEM encryption*/
      sol_Encryption = 1;
   } else {
      sol_op = 0x00;  /*no encryption*/
      sol_Encryption = 0;
   }
   if (seed_cnt != sol_seed_cnt && (seed_cnt < 16))  {
      /* if seed count changed, re-init the cipher. */
      sol_seed_cnt = seed_cnt;
      sol15_cipherinit(sol_seed_cnt, lanp.pswd, pconn->start_out_seq);
   }
}

/*
 * lan_send_sol
 * buffer contains characters entered at console.
 * build an SOL data frame, which ends up
 * calling _send_lan_cmd().
 *
 * SOL 1.5 Message Format
 *  0 : Packet Sequence Number
 *  1 : Packet ack/nak seq num (recvd)
 *  2 : Character offset (of recvd)
 *  3 : Seed count
 *  4 : Operation/Status
 */
int  lan_send_sol( uchar *buffer, int len, SOL_RSP_PKT *rsp)
{
   int rv = -1;
   int ilen;
   uchar idata[IPMI_REQBUF_SIZE]; /*=80 bytes*/
   uchar *pdata;
   int hlen, msglen;
   int sz;
   IPMI_HDR *phdr;
   int fdoauth = 1;
   uchar iauth[16];
   uint32 sess_id_tmp;
   uchar *psessid;
   int flags; 
              
   phdr = &ipmi_hdr;
   hlen = 4 + 10 + 16;  // was SOL_HLEN (14);

   pdata = &idata[0];
   memset(pdata,0,hlen);
   memcpy(&pdata[0], phdr, 4);   /* copy RMCP header to buffer */
   if (phdr->auth_type == IPMI_SESSION_AUTHTYPE_NONE) fdoauth = 0; 
   else fdoauth = 1;
   if (fdoauth == 0 && phdr->auth_type != IPMI_SESSION_AUTHTYPE_NONE) 
          pdata[4] = IPMI_SESSION_AUTHTYPE_NONE;
   else   pdata[4] = phdr->auth_type;
   memcpy(&pdata[5],&phdr->seq_num,4);  /*session sequence number*/
   sess_id_tmp = phdr->sess_id | SOL_MSG;
   memcpy(&pdata[9],&sess_id_tmp,4);    /*session id*/
   if (fdoauth == 0) hlen -= 16;

   pdata = &idata[hlen];
   if (len == 0) {
      pdata[0] = 0x00;  /*ack for keepalive*/
   } else {
      sol_snd_seq = (uchar)inc_sol_seq(sol_snd_seq);
      pdata[0] = sol_snd_seq;
      // sol15_encrypt(&pdata[5],buffer,len, sol_seed_cnt);
      memcpy(&pdata[5],buffer,len);
   }
   pdata[1] = sol_rcv_seq; /* seq to ack*/
   pdata[2] = sol_rcv_cnt;  /* num bytes ack'd */
   pdata[3] = sol_seed_cnt;
   pdata[4] = 0x00;  /*Operation/Status*/
   msglen = len + 5;
   {
	if (fdebuglan > 2) { /*SOL*/
           dbg_dump("lan_send_sol input", buffer,len,1);
           dbglog("auth_type=%x/%x fdoauth=%d hlen=%d seq_num=%x enc=%d\n", 
                 phdr->auth_type,lanp.auth_type,fdoauth,hlen,phdr->seq_num,
		 sol_Encryption);
           dbg_dump("send_sol buf", pdata,msglen,1);
	}
        if (fdoauth) {
           psessid = (uchar *)&sess_id_tmp;
           do_hash(phdr->password, psessid, &idata[hlen],msglen,
                   phdr->seq_num, phdr->auth_type, iauth);
           /* copy hashed authcode to header */
           memcpy(&pdata[13],iauth,16);
        }
   }
   idata[hlen-1] = (uchar)msglen;
   ilen = hlen + msglen;
   // rlen = sizeof(rdata);;

   if (fdebuglan > 2) 
      dbg_dump("lan_send_sol sendto",idata,ilen,1);
   flags = 0;
   sz = ipmilan_sendto(pconn->sockfd,idata,ilen,flags,
			(struct sockaddr *)&_destaddr,_destaddr_len); 
   if (fdebuglan) dbglog("lan_send_sol, sent %d bytes\n",sz);
   if (sz < 1) {
      lasterr = get_LastError();
      if (fdebuglan) show_LastError("lan_send_sol",lasterr);
      rv = LAN_ERR_SEND_FAIL;
      os_usleep(0,5000);
   }
   else {
      phdr->seq_num = inc_seq_num( phdr->seq_num );
      rv = 0;
   }

   if (rsp != NULL) {
      rsp->len  = 0;
#ifdef RCV_EXTRA
      if (rv == 0) { /*send was ok, try to receive*/
         rv =  lan_recv_sol( rsp );
         if (fdebuglan) printf("lan_recv_sol ret = %d, len = %d\n",rv,rsp->len);
      }
#endif
   }
   return(rv);
}

int  lan_keepalive( uchar bType )
{
   int rv = 0;
#ifdef KAL_OK
   if (bType == 2) {
	/* don't try if no data sent yet */
	if (sol_snd_seq == 0) return(rv);
	/* use lan_send_sol, but with 0 length data */
	rv = lan_send_sol(NULL,0,NULL);
   } 
   else
#endif
   {
	uchar devrec[16];
	rv = ipmi_getdeviceid(devrec,16,0);
   }
   return(rv);
}

int  lan_recv_sol( SOL_RSP_PKT *rsp )
{
   static uchar rsdata[MAX_BUFFER_SIZE];  /*LANplus allows 1024*/
   uchar rdata[MAX_BUFFER_SIZE];
   int  rlen, hlen;
   IPMI_HDR *phdr;
   uchar *pdata;
   int itry;
   int flags; 
   int rv = -1;

   phdr = &ipmi_hdr;
   rsp->data = rsdata;
   hlen = SOL_HLEN;
   rlen = 0;
   if (fdebuglan)
      printf("lan_recv_sol, fdebug=%d, fpdbg=%p\n",fdebuglan,fpdbg);
   // for (itry = 0; (itry < ipmi_try) && (rlen == 0); itry++)
   for (itry = 0; (itry < 1) && (rlen == 0); itry++)
   {
      /* receive the response */
      rv = fd_wait(pconn->sockfd, ipmi_timeout,0);
      if (rv != 0) {
         if (fdebuglan) fprintf(fpdbg,"lan_recv_sol timeout\n");
         rv = LAN_ERR_RECV_FAIL;
         os_usleep(0,5000);
         continue; /* ok to retry  */
      }
      flags = RECV_MSG_FLAGS;
      rlen = ipmilan_recvfrom(pconn->sockfd,rdata,sizeof(rdata),flags,
                        (struct sockaddr *)&_destaddr,&_destaddr_len);
      if (rlen < 0) {
         lasterr = get_LastError();
         if (fdebuglan) show_LastError("ipmilan_recvfrom",lasterr);
	 rv = rlen;   /* -3 = LAN_ERR_RECV_FAIL */
	 rlen = 0;
         rsp->len  = rlen;
         if (lasterr == econnrefused) continue;  /*try again*/
         else break; /* goto EXIT; */
      } else {  /* successful receive */
         rv = 0;
         if (fdebuglan) {
            dump_buf("lan_recv_sol rdata",rdata,rlen,1);  /*SOL*/
         }
         if (rdata[4] == IPMI_SESSION_AUTHTYPE_NONE) {  /* if AUTH_NONE */
            /* may have tried with auth, but denied, Dell 1855 */
            phdr->auth_type = IPMI_SESSION_AUTHTYPE_NONE;
            hlen = SOL_HLEN;
         }
         net2h(&phdr->iseq_num,&rdata[5],4);  /*incoming seq_num from hdr*/ 
         if (rlen >= hlen) {
            pdata = &rdata[hlen];
            if (fdebuglan) 
		dump_buf("lan_recv_sol rsp",rdata,rlen,1);  /*SOL*/
            rlen -= hlen;
            if (rlen >= 5) {  /*have some data*/
		/* 0=seq, 1=ack, 2=cnt, 3=ctl, 4=rsv */
		sol_rcv_seq = pdata[0];
                sol_rcv_ctl = pdata[3];
                pdata = &rdata[hlen+5];
		rlen -= 5;
		/* rlen should match rdata[hlen-1] */
                sol_rcv_cnt = (uchar)rlen;
	    }
            rsp->type = PAYLOAD_TYPE_SOL;
            rsp->len  = rlen;
            // sol15_decrypt(rsp->data,pdata,rlen, sol_seed_cnt);
            memcpy(rsp->data,pdata,rlen);
         } else {
            if (fdebuglan) 
               printf("lan_recv_sol rlen %d < %d\n",rlen,hlen); /*fpdbg*/
            rsp->type = PAYLOAD_TYPE_SOL;
            rsp->len  = 0;
         }
         break;
      }
   } /*end for*/
   return(rv);
}
#endif

uchar
cksum(const uchar *buf, register int len)
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

/* 
 * ipmi_cmd_ipmb()
 * This is an entry point for IPMB indirect commands.
 * Embed the command withn a SendMessage command.
 *
 * The iseq value needs to be the same as ipmi_hdr.swseq if
 * the session is ipmilan, so this routine was moved here.
 * However, ipmi_cmd_ipmb will also work ok via ipmi_cmdraw 
 * for local commands.
 */
int ipmi_cmd_ipmb(uchar cmd, uchar netfn, uchar sa, uchar bus, uchar lun,
		uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd)
{
    int rc, i, j;
    uchar idata[IPMI_REQBUF_SIZE];
    uchar rdata[MAX_BUFFER_SIZE];
    uchar ilen;
    int rlen;
    uchar iseq;
    IPMI_HDR *phdr;
    char *pstr;
    uchar fneedclear = 0;
    uchar cc;

    if (fdebugcmd) printf("ipmi_cmd_ipmb(%02x,%02x,%02x,%02x,%02x) sdata=%d\n",
			cmd,netfn,sa,bus,lun,sdata);
    phdr = &ipmi_hdr;
    iseq = phdr->swseq;
    i = 0;
    idata[i++] = bus;
    j = i;
    idata[i++] = sa; /*rsAddr (target)*/
    idata[i++] = (netfn << 2) | (lun & 0x03);  /*metFm/rsLUN*/
    idata[i++] = cksum(&idata[j],2); 
    j = i;
    idata[i++] = bmc_sa; /* rqAddr (main sa) */
    idata[i++] = (iseq << 2) | SMS_LUN; /*rqSeq num, SMS Message LUN = 0x02*/
    idata[i++] = cmd; 
    if (sdata > 0) {
       memcpy(&idata[i],pdata,sdata);
       i += sdata;
    }
    idata[i] = cksum(&idata[j],(i - j));
    ilen = ++i;
    rlen = sizeof(rdata);
    rc = ipmi_cmdraw(IPMB_SEND_MESSAGE,NETFN_APP, bmc_sa,PUBLIC_BUS,BMC_LUN,
                  idata, ilen, rdata, &rlen, pcc, fdebugcmd);
    if (rc == 0x83 || *pcc == 0x83) { /*retry*/
       rlen = sizeof(rdata);
       rc = ipmi_cmdraw(IPMB_SEND_MESSAGE,NETFN_APP, bmc_sa,PUBLIC_BUS,BMC_LUN,
                     idata, ilen, rdata, &rlen, pcc, fdebugcmd);
    }
    if (fdebugcmd) {
       if (rc == 0 && *pcc == 0) 
            dump_buf("ipmb sendmsg ok",rdata,rlen,0);
       else {
          if (*pcc == 0x80) {    pstr = "Invalid session handle"; }
          else if (*pcc == 0x81) pstr = "Lost Arbitration";
          else if (*pcc == 0x82) pstr = "Bus Error";
          else if (*pcc == 0x83) pstr = "NAK on Write";
          else pstr = "";
          fprintf(fpdbg,"ipmb sendmsg error %d, cc %x %s\n",rc,*pcc,pstr);
       }
    }
    if (presp == NULL || sresp == NULL) return(LAN_ERR_INVPARAM);
    if (rc != 0 || *pcc != 0) { *sresp = 0; return(rc); }
    if (*sresp < 0) return(LAN_ERR_TOO_SHORT);

    /* sent ok, now issue a GET_MESSAGE command to get the response. */
    for (i = 0; i < 10; i++)
    {
       rlen = sizeof(rdata);
       rc = ipmi_cmdraw(IPMB_GET_MESSAGE,NETFN_APP, bmc_sa,PUBLIC_BUS,BMC_LUN,
                        idata, 0, rdata, &rlen, pcc, fdebugcmd);
       if (fdebugcmd) printf("ipmb get_message rc=%d cc=%x\n",rc,*pcc);
       if (rc == 0x80 || *pcc == 0x80)    /*empty, so retry*/;
       else if (rc == 0x83 || *pcc == 0x83) /*busy, so retry*/;
       else break;       /* done w success or error */
#ifdef DOS
       ; /*short wait*/
#else
       fd_wait(0,0,10);  /* wait 1 msec before retry */
#endif
       /* will retry up to 10 msec for a get_message response on ipmb */
    }
    if (rc == 0 && *pcc == 0) {
        if (fdebugcmd) 
           dump_buf("ipmb getmsg ok",rdata,rlen,0);
	i = 0;
	if (rlen >= 8) { /* strip out the getmsg header */
	   *pcc = rdata[6];
	   rlen -= 8;
	   i = 7;
	}
        /* copy the data */
        if (rlen > *sresp) rlen = *sresp;
        memcpy(presp,&rdata[i],rlen);
    } else {
        if (*pcc == 0x80) pstr = "buffer empty";
        else { fneedclear = 1; pstr = ""; }
        if (fdebugcmd) 
           fprintf(fpdbg,"ipmb getmsg[%d] error %d, cc %x %s\n",i,rc,*pcc,pstr);
        if (fneedclear) {  /* Clear pending message flags */
              idata[0] = 0x03;  /* clear EvtMsgBuf & RecvMsgQueue */
              rlen = 16;
              rc = ipmi_cmdraw(IPMB_CLEAR_MSGF, NETFN_APP, 
			bmc_sa, PUBLIC_BUS,BMC_LUN, 
                        idata, 1, rdata, &rlen, &cc, fdebugcmd);
        }
        rlen = 0;
    }
    *sresp = rlen;
    return(rc);
}
/* end ipmilan.c */

