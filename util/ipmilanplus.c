/*
 * ipmilanplus.c
 * 
 * Interface to call libintf_lanplus from ipmitool to do RMCP+ protocol.
 *
 * 01/09/07 Andy Cress - created
 * 02/22/07 Andy Cress - initialize cipher_suite to 3 (was 0)
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#if defined(LINUX) || defined(BSD) || defined(MACOS)
#include <sys/time.h>
#endif

// #define DEBUG  1
int   verbose = 0;  
char  fdbglog = 0;

#ifndef HAVE_LANPLUS
#ifdef WIN32
#include <windows.h>
extern int strncasecmp(char *s1, char *s2, int n); /*ipmicmd.c*/
#define snprintf _snprintf
#define SockType SOCKET
#else
#define SockType int
#endif
/* No lanplus, so stub these functions returning errors. */
#define uchar  unsigned char
#define ushort  unsigned short
#define LAN_ERR_INVPARAM  -8
#define LOG_MSG_LENGTH 1024  /*usu. ipmicmd.h*/
struct valstr {   /*usually in ipmicmd.h*/
        ushort val;
        char * str;
};
struct oemvalstr {
        unsigned int  oem;
        ushort val;
        const char * str;
};
int ipmi_open_lan2(char *node, char *user, char *pswd, int fdebugcmd)
{ if (fdebugcmd) verbose = 1; return(LAN_ERR_INVPARAM); }

int ipmi_close_lan2(char *node)
{ return(LAN_ERR_INVPARAM); }

int ipmi_cmdraw_lan2(char *node, uchar cmd, uchar netfn, uchar lun, 
		uchar sa, uchar bus, uchar *pdata, int sdata,
                uchar *presp, int *sresp, uchar *pcc, char fdebugcmd)
{ printf("lanplus not configured\n"); return(LAN_ERR_INVPARAM); }

int ipmi_cmd_lan2(char *node, ushort cmd, uchar *pdata, int sdata,
                uchar *presp, int *sresp, uchar *pcc, char fdebugcmd)
{ return(LAN_ERR_INVPARAM); }

int lan2_send_sol( uchar *payload, int len, void *rsp)
{ return(LAN_ERR_INVPARAM); }
int lan2_recv_sol( void *rsp )
{ return(LAN_ERR_INVPARAM); }
int lan2_keepalive(int type, void *rsp)
{ return(LAN_ERR_INVPARAM); }
void lan2_recv_handler( void *rs )
{ return; }
void lan2_set_sol_data(int insize, int outsize, int port, void *handler,
			char esc_char)
{ return; }
SockType lan2_get_fd(void) { return(1); }
void lanplus_set_recvdelay( int delay ) { return; }
long lan2_get_latency( void ) { return(1); }
int lan2_send_break( void *rsp) { return(LAN_ERR_INVPARAM); }
int lan2_send_ctlaltdel( void *rsp) { return(LAN_ERR_INVPARAM); }

#else /* else HAVE_LANPLUS is defined */

#include "ipmilanplus.h"
#include "ipmicmd.h"
void set_loglevel(int level);   /*defined in subs.c*/
void lprintf(int level, const char * format, ...);
// #define LOG_WARN  4
// #define LOG_INFO  6

#ifdef METACOMMAND
extern void dbglog( char *pattn, ... );    /*from isolconsole.c*/
extern void sol_output_handler(void *rsp); /*from isolconsole.c*/
extern void dbg_dump(char *tag, uchar *pdata, int len, int fascii); 
#else
static void dbglog( char *pattn, ... ) { return; }
static void sol_output_handler(void *rsp) { return; }
static void dbg_dump(char *tag, uchar *pdata, int len, int fascii) { return; }
#endif
int ipmi_close_lan2(char *node);  /*prototype*/
extern LAN_OPT lanp;     /* from ipmicmd.c */
//extern char *gnode;      /* from ipmicmd.c */
//extern char *guser;      /* from ipmicmd.c */
//extern char *gpswd;      /* from ipmicmd.c */
//extern int  gauth_type;  /* from ipmicmd.c */
//extern int  gpriv_level; /* from ipmicmd.c */
//extern int gcipher_suite; /*from ipmicmd.c, see table 22-19 IPMI 2.0 spec*/
extern int  gshutdown;   /* from ipmicmd.c */
extern FILE *fpdbg;      /* == stdout, from ipmicmd.c */
extern FILE *fperr;      /* == stderr, from ipmicmd.c */
extern FILE *fplog;      /* from ipmicmd.c */
extern ipmi_cmd_t ipmi_cmds[]; /* from ipmicmd.c */
//extern char lan2_nodename[]; /*from lib/lanplus/lanplus.c */
extern struct ipmi_intf ipmi_lanplus_intf;  /*from libintf_lanplus.a*/

typedef struct {
  int type;
  int len;
  char *data;
  } SOL_RSP_PKT;
typedef struct {
  struct ipmi_intf *intf;
  SockType lan2_fd;
  uchar sol_seq;  /*sending SOL sequence num, will call inc_sol_seq*/
  uchar sol_len;  /*sending SOL num chars */
  uchar sol_seq_acked; /*last acked sent SOL sequence num*/
  uchar sol_rseq; /*received SOL sequence num*/
  uchar sol_rlen; /*received SOL num chans*/
  } LAN2_CONN;    /*see also IPMI_HDR below*/

static int loglvl = LOG_WARN; /*3=LOG_ERR 4=LOG_WARN 6=LOG_INFO 7=LOG_DEBUG*/
static LAN2_CONN conn = {NULL,0,0,0,0,0,0}; 
static LAN2_CONN *pconn = &conn;
// static SockType lan2_fd = 0;
// static struct ipmi_intf *intf = NULL;
static uchar sol_seq = 0;  /*sending SOL sequence num, will call inc_sol_seq*/
static uchar sol_len = 0;  /*sending SOL num chars */
static uchar sol_seq_acked = 0; /*last acked sent SOL sequence num*/
static uchar sol_rseq = 0; /*received SOL sequence num*/
static uchar sol_rlen = 0; /*received SOL num chans*/
static uchar chars_to_resend = 0; 
static long lan2_latency = 0; 

// #define LAN_ERR_INVPARAM  -8
#define PSWD_MAX       16

#if defined(WIN32)
/* Windows does not have gettimeofday, so do it here */
#include <windows.h>
#define UNIX_EPOCH_USEC  11644473600000000ULL
struct timezone 
{
  int  tz_minuteswest; 
  int  tz_dsttime;    
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 nusec, tmpres = 0;
  static int tzflag = 0;

  if (tv != NULL) {
    GetSystemTimeAsFileTime(&ft);
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
    nusec = tmpres / 10;  /*convert 100-nanosec into microseconds*/
    nusec -= UNIX_EPOCH_USEC;   /*windows -> unix epoch*/
    tv->tv_sec = (long)(nusec / 1000000UL);
    tv->tv_usec = (long)(nusec % 1000000UL);
  }

  if (tz != NULL) {
    if (!tzflag) {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
  return 0;
}
#endif

#if defined(WIN32) || defined(SOLARIS)
#ifdef OLD
/* Now moved to lib/lanplus/lanplus.c */
void ipmilanplus_init(struct ipmi_intf *intf)
{
         strcpy(intf->name,"lanplus");
         intf->setup  =      ipmi_lanplus_setup;
         intf->open   =      ipmi_lanplus_open;
         intf->close  =      ipmi_lanplus_close;
         intf->sendrecv  =   ipmi_lanplus_send_ipmi_cmd;
         intf->recv_sol  =   ipmi_lanplus_recv_sol;
         intf->send_sol  =   ipmi_lanplus_send_sol;
         intf->keepalive =   ipmi_lanplus_keepalive;
         intf->target_addr = IPMI_BMC_SLAVE_ADDR;  /*0x20*/
         intf->my_addr     = IPMI_BMC_SLAVE_ADDR;  /*0x20*/
}
#else
extern void ipmilanplus_init(struct ipmi_intf *intf);
#endif
#endif

struct ipmi_intf * ipmi_intf_load(char * name)
{
    if (strcmp(name,"lanplus") == 0) {
#if defined(WIN32) || defined(SOLARIS)
         /* initialize the intf */
         ipmilanplus_init(&ipmi_lanplus_intf);
#endif
         return (&ipmi_lanplus_intf);
    }
    else return (NULL);
}

long lan2_get_latency(void)
{
   return(lan2_latency);
}

static void set_latency( struct timeval *t1, struct timeval *t2, long *latency)
{
   long nsec;
   nsec = t2->tv_sec - t1->tv_sec;
   if ((ulong)(nsec) > 1) nsec = 1;
   *latency = nsec*1000 + (t2->tv_usec - t1->tv_usec)/1000;
}

/* 
 * ipmi_open_lan2
 */
int ipmi_open_lan2(char *node, char *puser, char *pswd, int fdebugcmd)
{
   char *user = "";
   int rv = -1;
   size_t n;
   struct ipmi_intf *intf;

   if (puser != NULL) user = puser;
#ifdef DEBUG
   if (fdbglog && fdebugcmd) fdebugcmd = 3;  /*full debug*/
   else if (fdbglog) fdebugcmd = 2;  /*special log only from isolconsole.c*/
   else if (fdebugcmd) fdebugcmd = 1;  /*debug, no packets*/
   if (fdebugcmd) fdebugcmd = 4; /*max debug*/
#endif
   switch (fdebugcmd) {
      case 4:   /* max debug */
	  loglvl = 8; /* 8=(LOG_DEBUG=1, max), 7=LOG_DEBUG */
	  verbose = 8;  /* usually 0 or 1, but max is 8. */
	  break;
      case 3:   /* full debug */
	  loglvl = 7; /* 7=LOG_DEBUG; max=8 (LOG_DEBUG+1) */
	  verbose = 4;  /* show packets */
	  break;
      case 2:   /* debug log file only*/
	  loglvl = 6; /* 6=LOG_INFO, 4=LOG_WARN */
	  verbose = 1;  /* usually 0 or 1, but could be up to 8. */
	  break;
      case 1:   /* debug, no packets */
	  loglvl = 7; /* 7=LOG_DEBUG; max=8 (LOG_DEBUG+1) */
	  verbose = 1;  /* usually 0 or 1, but could be up to 8. */
	  break;
      case 0:   /* no debug*/
	  /* by default loglevel = 4; (4=LOG_WARN) verbose = 0; */
      default:  break;
   }
   if (fdbglog) 
      dbglog("ipmi_open_lan2(%s,%s,%p,%d) verbose=%d loglevel=%d\n",
		node,user,pswd,fdebugcmd,verbose,loglvl);
   else if (fdebugcmd) 
      fprintf(fpdbg,"ipmi_open_lan2(%s,%s,%p,%d) verbose=%d loglevel=%d\n",
		node,user,pswd,fdebugcmd,verbose,loglvl);
   set_loglevel(loglvl);
   intf = pconn->intf;

   if (nodeislocal(node)) {
        fprintf(fpdbg,"ipmi_open_lan2: node %s is local!\n",node);
        rv = LAN_ERR_INVPARAM;
        goto EXIT;
   } else {
	/* if attempting re-open to a new node, close the previous one. */
	if (intf != NULL) {
	    if ((intf->session != NULL) &&
	        (strcmp(intf->session->hostname,node) != 0)) {
		rv = ipmi_close_lan2(intf->session->hostname);
	    }
	}
        if ((gshutdown==0) || fdebugcmd) 
	   fprintf(fpdbg,"Opening lanplus connection to node %s ...\n",node);

        rv = 0;
        if (intf == NULL) {
            /* fill in the intf structure */
            intf = ipmi_intf_load("lanplus");
	    if (intf == NULL) return(-1);
        } 
	if (intf->session == NULL && intf->opened == 0) {
	    if (intf->setup == NULL) return(-1);
            rv = intf->setup(intf);  /*allocates session struct*/
            if (fdebugcmd) printf("lan2 intf setup returned %d\n",rv);
	}

        if (rv == 0) {
	    if (intf->open == NULL) return(-1);
            if (intf->session == NULL) return(-1);
            intf->session->authtype_set = (uchar)lanp.auth_type; 
	    intf->session->privlvl      = (uchar)lanp.priv;
            intf->session->cipher_suite_id = (uchar)lanp.cipher; 
            if (node != NULL) { strcpy(intf->session->hostname,node); }
            if (user != NULL) { strcpy(intf->session->username,user); }
            if (pswd == NULL || pswd[0] == 0) 
	      intf->session->password = 0;
	    else {
	      intf->session->password = 1;
              n = strlen(pswd);
              if (n > PSWD_MAX) n = PSWD_MAX;
              memset(intf->session->authcode,0,PSWD_MAX);
              strncpy(intf->session->authcode, pswd, n);
            }
            rv = intf->open(intf);
            if (fdebugcmd)
                printf("lan2 open.intf(auth=%d,priv=%d,cipher=%d) returned %d\n",
			 lanp.auth_type,lanp.priv,lanp.cipher, rv);
            if (rv != -1) { /*success is >= 0*/
		sol_seq = 0;  /*init new session, will call inc_sol_seq*/
		sol_len = 0;
		sol_seq_acked = 0;
		pconn->lan2_fd = intf->fd; /*not same as rv if Windows*/
		rv = 0;
            }
        }
        pconn->intf = intf;
   }
EXIT:
   if (rv != 0) {
      if ((gshutdown == 0) || fdebugcmd) 
             fprintf(fperr, "ipmi_open_lan2 error %d\n",rv);
   }
   return(rv);
}

int ipmi_close_lan2(char *node)
{
   int rv = 0;
   struct ipmi_intf *intf;

   intf = pconn->intf;
   if (!nodeislocal(node)) {  /* ipmilan, need to close & cleanup */
      if (fdbglog) dbglog("ipmi_close_lan2(%s) intf=%p\n",node,intf);
      if (intf != NULL) { 
        if (intf->opened > 0 && intf->close != NULL) {
            intf->close(intf);   /* do the close */
	    intf->fd = -1;
	    intf->opened = 0;
	}
      }
      pconn->lan2_fd = -1;
      sol_seq = 0;  sol_len = 0;
      sol_rseq = 0; sol_rlen = 0;
      sol_seq_acked = 0;
   }
   return (rv);
}
 
int ipmi_cmdraw_lan2(char *node, uchar cmd, uchar netfn, uchar lun, 
		uchar sa, uchar bus, uchar *pdata, int sdata,
                uchar *presp, int *sresp, uchar *pcc, char fdebugcmd)
{
   int rc, n; 
   struct ipmi_rq req;
   struct ipmi_rs *rsp;
   struct timeval t1, t2;
   struct ipmi_intf *intf = pconn->intf;

   if (fdebugcmd) verbose = 5;  /* show packets */
#ifdef DEBUG
   if (fdebugcmd) verbose = 8; 
#endif
   if (intf == NULL || (intf->opened == 0)) {
        rc = ipmi_open_lan2(node,lanp.user,lanp.pswd,fdebugcmd);
        if (rc != 0) {
           if (fdebugcmd)
              fprintf(fperr, "ipmi_cmd_lan2: interface open error %d\n",rc);
           return(rc);
        }
        intf = pconn->intf;
   }
 
   /* do the command */
   memset(&req, 0, sizeof(req));
   req.msg.cmd      = cmd;
   req.msg.netfn    = netfn;
   req.msg.lun      = lun;
   req.msg.target_cmd  = cmd;
   req.msg.data     = pdata;
   req.msg.data_len = sdata;
   intf->target_addr = sa;  /*usu 0x20*/
   intf->target_lun  = lun;
   intf->target_channel = bus;
  

   gettimeofday(&t1, NULL);
   rsp = intf->sendrecv(intf, &req);
   if (rsp == NULL) rc = -1;
   else {
      *pcc = rsp->ccode;
      rc = rsp->ccode; 
   }
   gettimeofday(&t2, NULL);
   set_latency(&t1,&t2,&lan2_latency);
   if (rc == 0) {
      /* copy data */
      if (rsp->data_len > *sresp) n = *sresp;
      else n = rsp->data_len;
      memcpy(presp,rsp->data,n);
      *sresp = n;
   } else {
      *sresp = 0;
      if (fdebugcmd)
          fprintf(fperr, "ipmi_cmd_lan2 error %d\n",rc);
   }
   return (rc);
}

/* 
 * ipmi_cmd_lan2
 * This is the entry point, called from ipmicmd.c
 */
int ipmi_cmd_lan2(char *node, ushort cmd, uchar *pdata, int sdata,
                uchar *presp, int *sresp, uchar *pcc, char fdebugcmd)
{
   int rc, i;
   uchar mycmd;

   for (i = 0; i < NCMDS; i++) {
       if (ipmi_cmds[i].cmdtyp == cmd) break;
   }
   if (i >= NCMDS) {
        fprintf(fperr, "ipmi_cmd_lan2: Unknown command %x\n",cmd);
        return(-1);
        }
   if (cmd >= CMDMASK) mycmd = (uchar)(cmd & CMDMASK);  /* unmask it */
   else mycmd = (uchar)cmd;
   
   rc = ipmi_cmdraw_lan2(node,mycmd, ipmi_cmds[i].netfn, ipmi_cmds[i].lun,
			ipmi_cmds[i].sa, ipmi_cmds[i].bus,
                        pdata, sdata, presp, sresp, pcc, fdebugcmd);
   return(rc);
}



#define NOEM 4
static struct { int id; char *name; } oem_list[NOEM] = {
   {0x000157, "intelplus"},   /*VENDOR_INTEL*/
   {0x002A7C, "supermicro"},  /*VENDOR_SUPERMICRO*/
   /* {       0, "icts"},  *ICTS testware, needs user option*/
   {0x00000B, "hp"},    /*VENDOR_HP*/
   {0x000002, "ibm"}    /*VENDOR_IBM*/
};
#ifdef METACOMMAND
extern int is_lan2intel(int vend, int prod);  /*oem_intel.c*/
#else
static int is_lan2intel(int vend, int prod) {
   int rv = 0;
   /* Older Intel BMCs use oem lan2i method, newer Intel FW uses lan2. */
   if (vend == VENDOR_INTEL) 
      if ((prod < 0x0030) || (prod == 0x0811)) rv = 1;
   return(rv);
}
#endif

int ipmi_oem_active(struct ipmi_intf * intf, const char * oemtype)
{
        int i, vend, prod, dtype;
	get_mfgid(&vend,&prod);
	dtype = get_driver_type();
        if (verbose) 
	    lprintf(LOG_INFO,"oem_active(is_type==%s ?) vend=%x prod=%x", oemtype,vend,prod);
        if (strncmp("intelplus", oemtype, strlen(oemtype)) == 0) {
            /* special case to detect intelplus, not all Intel platforms */
            if (dtype == DRV_LAN2I) {
		i = 1;
	    } else {
		if (is_lan2intel(vend,prod)) {
		   i = 1;
		   set_driver_type("lan2i");
		} else { /* If iBMC, does not use intelplus */
                   if (verbose) lprintf(LOG_WARN,"detected as not intelplus");
		   i = 0;
                }
            }
            if (verbose && i == 1) lprintf(LOG_WARN,"intelplus detected, vend=%x prod=%x",vend, prod);
	    return i;
        } else {
           // if (intf->oem == NULL) return 0;
           for (i = 0; i < NOEM; i++) {
             if (strncmp(oem_list[i].name,oemtype,strlen(oemtype)) == 0)
                if (oem_list[i].id == vend) {
                   if (verbose) lprintf(LOG_WARN,"%s detected, vend=%x",oemtype,vend);
                   return 1;
                }
           }
        }
        return 0;
}

/*
 * lan2_validate_solrcv()
 * Validate the SOL receive packet, and save received SOL info 
 * (recv seq num, char count) for use in send_sol header.
 * return value (bitwise):
 *   0x01 bit set: received packet is present with data len > 0
 *   0x02 bit set: received packet flags error in the previously sent packet
 *   0x04 bit set: received packet is a retry of previous
 *   0x08 bit set: break detected
 */
static int lan2_validate_solrcv(struct ipmi_rs * rs)
{
    int rv = 0;
    if (rs == NULL) return(rv);
    if (verbose > 4) 
	dbg_dump("rs_sol_hdr",
		(uchar *)&rs->payload.sol_packet.packet_sequence_number,8,1);
    chars_to_resend = 0;
    sol_rlen = (uchar)rs->data_len;
    if (sol_rlen > 4)  sol_rlen -= 4;
    else sol_rlen = 0;
    if (sol_rlen > 0) rv |= 0x01;  /* have a receive packet with dlen > 0 */
    if (rs->payload.sol_packet.packet_sequence_number != 0) {
	  if (rs->payload.sol_packet.packet_sequence_number == sol_rseq) {
		lprintf(LOG_INFO,"received retry of sol_rseq %d, rlen=%d",
			sol_rseq,sol_rlen);
		/* do nothing because rs->data_len was set to 4 in lanplus */
		// rv |= 0x04;
		/* return, don't process this again */
		return(rv);  
	  } else sol_rseq = rs->payload.sol_packet.packet_sequence_number;
    }
    /* check for errors in previously sent packet */
    if (rs->payload.sol_packet.acked_packet_number != 0) {
        if (rs->payload.sol_packet.acked_packet_number != sol_seq) rv |= 0x02;
        else if ((rs->payload.sol_packet.accepted_character_count < sol_len) 
                  && (sol_seq_acked < sol_seq)) { 
                lprintf(LOG_INFO,"partial_ack, seq=%d: acked=%d < sent=%d",
			sol_seq,rs->payload.sol_packet.accepted_character_count,
			sol_len);
		chars_to_resend = sol_len - 
				rs->payload.sol_packet.accepted_character_count;
		rv |= 0x02;
	}
        sol_seq_acked = rs->payload.sol_packet.acked_packet_number;
    }
    if (sol_seq != 0) {  /*if we have sent something*/
	if (rs->payload.sol_packet.is_nack) rv |= 0x02;
	if (rs->payload.sol_packet.transfer_unavailable) rv |= 0x02;
	if (rs->payload.sol_packet.sol_inactive) rv |= 0x02;
	if (rs->payload.sol_packet.transmit_overrun) rv |= 0x02;
    }
    if (rs->payload.sol_packet.break_detected) rv |= 0x08;
    if (rv & 0x02) {
        if (sol_seq_acked < sol_seq) {  /*not already acked, needs retry*/
	   lprintf(LOG_INFO,"need to retry sol_seq=%d, acked=%d len=%d rv=%x",
			sol_seq,sol_seq_acked,sol_len,rv);
           if (chars_to_resend == 0) chars_to_resend = sol_len;
	} else  rv &= 0xFD; 
    }
    return(rv);
}

/*
 * lan2_set_sol_data
 * called from isolconsole when SOL 2.0 session is activated.
 */
void lan2_set_sol_data(int insize, int outsize, int port, void *handler,
			char esc_char)
{
    struct ipmi_intf *intf = pconn->intf;
    if (intf == NULL) return;
    lprintf(LOG_INFO,"setting lanplus intf params(%d,%d,%d,%p,%c)",
		insize,outsize,port,handler,esc_char);
    intf->session->sol_data.max_inbound_payload_size = (ushort)insize;
    intf->session->sol_data.max_outbound_payload_size = (ushort)outsize;
    intf->session->sol_data.port = (ushort)port;
    intf->session->sol_data.sol_input_handler = handler; 
    intf->session->timeout = 1; /* lib/.../lanplus.h: IPMI_LAN_TIMEOUT =1sec*/
    intf->session->sol_escape_char = esc_char;  /*usu '~'*/
}

int lan2_keepalive(int type, SOL_RSP_PKT *rsp)
{
    int rv = 0;
    struct ipmi_intf *intf = pconn->intf;
    if (fdbglog) dbglog("lan2_keepalive(%d,%p) called\n",type,rsp); /*++++*/
    if (intf == NULL) return -1;
    if (rsp) rsp->len  = 0;
    if (type == 2) {  /*send empty SOL data*/
        struct ipmi_v2_payload v2_payload;
        struct ipmi_rs * rs = NULL;
        memset(&v2_payload, 0, sizeof(v2_payload));
        v2_payload.payload.sol_packet.packet_sequence_number = 0;
        v2_payload.payload.sol_packet.character_count = 0;
	v2_payload.payload.sol_packet.acked_packet_number = 0;
	v2_payload.payload.sol_packet.accepted_character_count = 0;
        rs = intf->send_sol(intf, &v2_payload);
        if (rs == NULL) {
           rv = -1;
        } else {    /*may sometimes get data back*/
           rsp->type = rs->session.payloadtype;
           rsp->len  = rs->data_len;
           rsp->data = rs->data;
           lprintf(LOG_INFO,
		"keepalive: rq_seq=%d rs_seq=%d (0x%02x) rseq=%d rlen=%d",
	        // v2_payload.payload.ipmi_request.rq_seq,
                v2_payload.payload.sol_packet.packet_sequence_number,
		rs->session.seq, rs->session.seq,
	   	rs->payload.sol_packet.packet_sequence_number,rs->data_len);
	   rv = lan2_validate_solrcv(rs);
	   if (rv > 1) lprintf(LOG_INFO,
		"keepalive: rv=%x need retry of sol_seq=%d(%d) sol_len=%d(%d)",
                rv,v2_payload.payload.sol_packet.packet_sequence_number,
		sol_seq,v2_payload.payload.sol_packet.character_count,sol_len);
           rv = 0;  /* 0 = have recv buffer to process*/
	}
    } else {
        rv = intf->keepalive(intf);  /*get_device_id*/
    }
    if (fdbglog) dbglog("lan2_keepalive rv = %d\n",rv); /*++++*/
    return(rv);
}

static uchar  inc_sol_seq( uchar lastseq )
{
    uchar seq;
    seq = lastseq + 1;
    if (seq > 15) seq = 1;
    pconn->intf->session->sol_data.sequence_number = seq;
    return(seq);
}

int lan2_send_break( SOL_RSP_PKT *rsp)
{
    struct ipmi_rs *rs;
    static struct ipmi_v2_payload v2_payload;
    int rv = 0;
    struct ipmi_intf *intf = pconn->intf;

    if (intf == NULL) return -1;
    if (rsp == NULL) return -1;
    rsp->len  = 0;  /*just in case*/
    memset(&v2_payload, 0, sizeof(v2_payload));
    v2_payload.payload.sol_packet.character_count = 0;
    v2_payload.payload.sol_packet.generate_break  = 1;

    rs = intf->send_sol(intf, &v2_payload);
    if (rs == NULL) {
	rv = -1;
	lprintf(LOG_INFO,"send_break error");
    } else {
	rv = 0;
	rsp->type = rs->session.payloadtype;
	rsp->len  = rs->data_len;
	rsp->data = rs->data;
        lprintf(LOG_INFO,"send_break(rs): sol_seq=%d rs_sol=%d "
		"rs_seq=%d (0x%02x) rseq=%d rlen=%d",
		v2_payload.payload.sol_packet.packet_sequence_number,
		rs->payload.sol_packet.packet_sequence_number,
		rs->session.seq, rs->session.seq,
	   	rs->payload.sol_packet.packet_sequence_number,rs->data_len);
    }
    return(rv);
}

int lan2_send_sol( uchar *payload, int len, SOL_RSP_PKT *rsp)
{
    struct ipmi_rs *rs;
    static struct ipmi_v2_payload v2_payload;
    int rv = 0;
    struct ipmi_intf *intf = pconn->intf;

    if (rsp) rsp->len  = 0;  /*just in case*/
    if (intf == NULL) return -1;
    memset(&v2_payload, 0, sizeof(v2_payload));
    memcpy(v2_payload.payload.sol_packet.data,payload,len);
    sol_seq = inc_sol_seq(sol_seq);
    sol_len = (uchar)len;
    v2_payload.payload.sol_packet.packet_sequence_number = sol_seq;
    v2_payload.payload.sol_packet.character_count = (uchar)len;
#ifdef TEST
    /* Note that the lanplus layer already did auto-ack of sol recv pkts,
     * but we can put the info in send_sol also for completeness. */
    /* Further debug shows that this doesn't matter, so skip it. */
    v2_payload.payload.sol_packet.acked_packet_number = sol_rseq;
    v2_payload.payload.sol_packet.accepted_character_count = sol_rlen;
    /* These flags were initialized to zero above via memset. */
    v2_payload.payload.sol_packet.is_nack          = 0;
    v2_payload.payload.sol_packet.assert_ring_wor  = 0;
    v2_payload.payload.sol_packet.generate_break   = 0;
    v2_payload.payload.sol_packet.deassert_cts     = 0;
    v2_payload.payload.sol_packet.deassert_dcd_dsr = 0;
    v2_payload.payload.sol_packet.flush_inbound    = 0;
    v2_payload.payload.sol_packet.flush_outbound   = 0;
#endif
    lprintf(LOG_INFO,"send_sol(rq): sol_seq=%d acked=%d chars=%d len=%d",
           v2_payload.payload.sol_packet.packet_sequence_number,
           v2_payload.payload.sol_packet.acked_packet_number, 
           v2_payload.payload.sol_packet.accepted_character_count,len);
    if (verbose > 4)
       dbg_dump("rq_sol_hdr", 
		(uchar *)&v2_payload.payload.sol_packet.packet_sequence_number,
			10,1);
    rs = intf->send_sol(intf, &v2_payload);
    if (rs == NULL) {
       rv = -1;
       lprintf(LOG_INFO,"send_sol error (%d bytes)",len);
    } else {
       rsp->type = rs->session.payloadtype;
       rsp->len  = rs->data_len;
       rsp->data = rs->data;
       lprintf(LOG_INFO,"send_sol(rs): sol_seq=%d rs_sol=%d rs_seq=%d (0x%02x)"
		" rseq=%d rlen=%d",
		v2_payload.payload.sol_packet.packet_sequence_number,
		// v2_payload.payload.sol_packet.acked_packet_number,
		rs->payload.sol_packet.packet_sequence_number,
		rs->session.seq, rs->session.seq,
	   	rs->payload.sol_packet.packet_sequence_number,rs->data_len);
       rv = lan2_validate_solrcv(rs);
       if (rv > 1) lprintf(LOG_INFO,
		"send_sol: rv=%x sol_seq=%d(%d) sol_len=%d(%d) not acked",
                rv,v2_payload.payload.sol_packet.packet_sequence_number,
		sol_seq,v2_payload.payload.sol_packet.character_count,sol_len);
       rv = 0;  /* 0 = have recv buffer to process*/
    }
    return(rv);
}

int lan2_recv_sol( SOL_RSP_PKT *rsp )
{
    struct ipmi_rs * rs;
    int rv;
    struct ipmi_intf *intf = pconn->intf;

    if (rsp == NULL) return -1;
    rsp->len  = 0;
    if (intf == NULL) return -1;
    rs = intf->recv_sol(intf);
    if (rs == NULL) return -1;
    rsp->type = rs->session.payloadtype;
    rsp->len  = rs->data_len;
    rsp->data = rs->data;
    lprintf(LOG_INFO,"recv_sol: rs_sol=%d rs_seq=%d (0x%02x) rseq=%d rlen=%d",
		rs->payload.sol_packet.packet_sequence_number,
		rs->session.seq, rs->session.seq,
	   	rs->payload.sol_packet.packet_sequence_number,rs->data_len);
    rv = lan2_validate_solrcv(rs);
    if (rv > 1) {
	lprintf(LOG_INFO,
		"recv_sol: rv=%x sol_seq=%d sol_len=%d not acked",
		rv,sol_seq,sol_len);
    }
    return(rsp->len);
}

void lan2_recv_handler( void *rs0)
{
    struct ipmi_rs *rs = rs0;
    SOL_RSP_PKT rsp;
    int rv;
    
    rsp.len  = 0;
    rsp.type = 0;
    if (rs == NULL) return;
    lprintf(LOG_INFO,"recv_handler: len=%d rs_seq=%d (0x%02x) rseq=%d rlen=%d",
		rs->data_len, rs->session.seq, rs->session.seq,
	   	rs->payload.sol_packet.packet_sequence_number,rs->data_len);
    rsp.type = rs->session.payloadtype;
    rsp.len  = rs->data_len;
    rsp.data = rs->data;
    rv = lan2_validate_solrcv(rs);
    if (rv > 1) lprintf(LOG_INFO,
		    "recv_handler: rv=%x sol_seq=%d sol_len=%d not acked",
		    rv,sol_seq,sol_len);
    sol_output_handler(&rsp);
    return;
}

SockType lan2_get_fd(void)
{
   if (pconn->intf == NULL) return pconn->lan2_fd;
   return(pconn->intf->fd);
}
#endif

/* end ipmilanplus.c */
