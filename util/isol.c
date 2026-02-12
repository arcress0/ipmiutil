/*
 * isol.c (was isolconsole.c)
 *      IPMI Serial-Over-LAN console application
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2007 Intel Corporation
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 10/12/06 Andy Cress - created, just a framework for now
 * 02/08/07 Andy Cress - now working for IPMI LAN 2.0 SOL in Linux,
 *                       still stdio problems in Windows
 * 02/23/07 Andy Cress - fixed Windows stdin, still Windows output issues,
 *                       need more in xlate_vt100_win().
 * 04/06/07 Andy Cress - more vt100-to-windows translation, 
 *                       added -r for raw termio
 * 04/19/07 Andy Cress - more vt100-to-windows translation, 
 * 05/08/07 Andy Cress - more logic for 1.5 SOL.  Session opens but 
 *                       1.5 SOL data packets are not yet working.
 * 05/24/07 Andy Cress - Fix Enter key confusion in BIOS Setup (use CR+LF)
 * 06/15/07 Andy Cress - More fixes to Windows translation (fUseWinCon)
 * 08/20/07 Andy Cress - moved Windows translation to isolwin.c
 * 01/31/26 Andy Cress - added WIN32_LEAN_AND_MEAN
 * See ChangeLog for further changes
 *
 * NOTE: There are a few other options for Serial-Over-LAN console 
 * applications:
 *   ipmitool.sf.net has v2 sol console capability for Linux (BSD)
 *   Intel dpccli, a CLI console for Linux and Windows (proprietary)
 *   Intel ISM console for Windows (proprietary)
 *   Intel System Management Software (LANDesk) console, proprietary 
 * The Intel applications support both IPMI 1.5 and 2.0 SOL 
 * protocols.
 */
/*M*
Copyright (c) 2007 Intel Corporation
Copyright (c) 2009 Kontron America, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 
  c.. Neither the name of Kontron nor the names of its contributors 
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
#if defined(DOS)
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"
int i_sol(int argc, char **argv)
{
   printf("IPMI SOL console is not supported under DOS.\n");
   return(1);
}
#else

/* All other OSs:  Linux, Windows, Solaris, BSD */
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#ifdef HAVE_IPV6 
#include <winsock2.h>
#else
#include <winsock.h>
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "getopt.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h> 
#include <termios.h>
#include <unistd.h> 
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#else
#include <getopt.h>
#endif
#endif
#include <string.h>
#include <time.h>
#include "ipmicmd.h"
#include "ipmilanplus.h"
 
#define ACTIVATE_PAYLOAD    0x48
#define DEACTIVATE_PAYLOAD  0x49
#define PAYLOAD_TYPE_SOL    0x01
#define SOL_SERIAL_ALERT_MASK_DEFERRED 0x04
#define SOL_BMC_ASSERTS_CTS_MASK_TRUE  0x00  
#define SOL_BMC_ASSERTS_CTS_MASK_FALSE 0x02
#define IPMI_PAYLOAD_TYPE_SOL  0x01
#define AUTHTYPE_RMCP_PLUS  0x06  
#define RV_END   -2
#define CH_CR   '\r'    /*0x0D =='\r'*/
#define CH_LF   '\n'    /*0x0A =='\n' LineFeed(Down) */
#define CH_ESC  '\033'  /*0x1B == ASCII Escape */
#define CH_CTL  '\316'  /*0xCE == VT100 Control */
#define CH_DEL  '\177'  /*0x7F == Delete key */
#define CH_BS   '\010'  /*0x08 == Backspace */
#define CH_TAB  '\011'  /*0x09 == Tab */
#define CH_UP   '\013'  /*0x0B == Up Arrow */

/* Input data buffer size: screen size (80*24) = 1920 */
/* IPMI_BUF_SIZE = 1024, less RMCP+IPMI hdr */
/* IDATA_SZ=2048 was too big, used =300 thru v2.1.0 */
#define IDATA_SZ  512
#define IPKT_SZ   512
#define MAX_INTEL_DATA  203  /*sol_send >= 204 bytes gives error w Intel BMC*/
#define MAX_DELL_DATA    46  /*sol_send >= 46 bytes gives error w Dell BMC*/
#define MAX_KONTRON_DATA 74  /*sol_send >= 75 bytes gives error w Kontron BMC*/
#define MAX_OTHER_DATA   45  /*use a small sol_send limit of 45 by default */

typedef struct {
  int type;
  int len;
  char *data;
  } SOL_RSP_PKT;

extern void tty_setraw(int mode);      /*from ipmicmd.c*/
extern void tty_setnormal(int mode);   /*from ipmicmd.c*/
/* extern int  tty_getattr(int *lflag,int *oflag,int *iflag);  *from ipmicmd.c*/
extern SockType lan_get_fd(void);          /*from ipmilan.c*/
extern int  lan_send_sol( uchar *payload, int len, SOL_RSP_PKT *rsp);
extern int  lan_recv_sol( SOL_RSP_PKT *rsp );
extern int  lan_keepalive(uchar type);
extern void lan_get_sol_data(uchar fEnc, uchar iseed, uint32 *seed);
extern void lan_set_sol_data(uchar fEnc, uchar auth, uchar iseed, 
				int insize, int outsize);
/* lan2 routines, from ipmilanplus.c, even if not HAVE_LANPLUS: */
extern int  lan2_get_fd(void);          /*from ipmilanplus.c*/
extern int  lan2_send_sol( uchar *payload, int len, SOL_RSP_PKT *rsp);
extern int  lan2_recv_sol( SOL_RSP_PKT *rsp );
extern void lan2_set_sol_data(int in, int out, int port, void*hnd, char esc);
extern int  lan2_keepalive(int type, SOL_RSP_PKT *rsp);
extern void lan2_recv_handler( void *rs );
extern long lan2_get_latency( void );  /*from ipmilanplus.c*/
extern void lanplus_set_recvdelay( int delay );  /*lib/lanplus/lanplus.c*/
extern int  lan2_send_break( SOL_RSP_PKT *rsp );

extern char  fdbglog;    /*see ipmilanplus.c*/
extern char log_name[];  /*see ipmicmd.c*/
extern int  is_lan2intel(int vend, int prod); /*see oem_intel.c*/
void dbglog( char *pattn, ... ); /*local prototype*/

/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil sol";
#else
static char * progver   = "3.08";
static char * progname  = "isol";
#endif
static char  fdebug        = 0;
static char  fpicmg        = 0;
static char  factivate     = 0;
static char  fdeactivate   = 0;
static char  fScript       = 0;
static char  fTrace        = 0;
static char  fprivset      = 0;
static char  bDriver       = 0;
static uchar bSolVer       = 0;
static uchar bKeepAlive    = 1;   /*1=GetDeviceID, 2=empty SOL data*/
static char  sol_esc_len   = 2;   /* SOL escape seq ("~.") */
static char  sol_esc_ch    = '~'; 
static char  sol_esc_pending = 0;
static char  sol_esc_fn[4] = {'.','b','d', '?'}; /* SOL escape functions */
static char  file_scr[80] = {""};
static char  file_trc[80] = {""};
static char  dbglog_name[40] = "isoldbg.log";
static FILE *fp_scr = NULL;
static FILE *fp_trc = NULL;
static uchar ipmi_maj = 0;
static uchar ipmi_min = 0;
static int   vend_id  = 0;  /* Manufacturer IANA vendor id */
static int   prod_id  = 0;
static uchar fSolEncryption     = 1;
static uchar fSolAuthentication = 1;
static int   sol_timeout = 30;  /* default: send keepalive every 30 seconds */
static time_t keepalive_start; 
static int   max_bmc_data = MAX_OTHER_DATA;
static int   fgotrecv = 0;
static int   fsentok = 0;
static uchar payload_instance = 1;
char  fUseWinCon    = 1;
uchar fCRLF     = 0;  /* 0=use CR, 1=use legacy CR+LF, 2=use LF (like raw) */
uchar fRaw      = 0;
uchar bTextMode = 0;
int   sol_done = 0;
int   wait_time = 500;           /*number of msec to wait*/
int   retry_time = 5;            /*number of msec between retries*/

static int    sol_ver = 2;         /*use IPMI 2.0 SOL by default*/
static int    sol_retries = 1;     /*send retries for sol_input_handler*/
static int    sol_recvdelay = 0;   /*if != 0, num us to delay before recv*/
static uchar  sol15_iseed = 0x01;  /*initial seed count, usu 0 or 1 */
static uint32 sol15_wseed = 0;     /*32-bit seed value*/
static long lan2_latency = 0;      /*lan2 latency in msec */
static long lan2_slow = 100;       /*slow if latency > 100 msec */

extern FILE *fplog;  /*see ipmicmd.c*/
#ifdef WIN32
/* these routines are in isolwin.c */
extern DWORD WINAPI input_thread( LPVOID lpParam);
extern void console_open(char fdebugcmd);
extern void console_close(void);
extern void console_out(uchar *pdata, int len);
extern int  console_in(DWORD keydata, uchar *pdata, int len);
extern int  os_select(int infd, SockType sfd, fd_set *read_fds, fd_set *error_fds);
extern int  os_read(int fd, uchar *buf, int sz);
extern void Ascii2KeyCode(uchar c, uchar *buf);
#else

/*======== start Linux ===============================*/
int os_read(int fd, uchar *buf, int sz)
{
    int len = 0;
    len = read(fd, buf, sz);
    return(len);
}

int os_select(int infd, SockType sfd, fd_set *read_fds, fd_set *error_fds)
{
   int rv = 0;
   struct timeval tv;

   /* Linux handles both stdin & socket via select() */
   FD_ZERO(read_fds);
   FD_SET(infd, read_fds);
   FD_SET(sfd, read_fds);
   FD_ZERO(error_fds);
   FD_SET(sfd, error_fds);
   tv.tv_sec =  0;
   tv.tv_usec = wait_time * 1000;  /* 500000 usec, 500 msec, 0.5 sec*/
   rv = select((int)(sfd + 1), read_fds, NULL, error_fds, &tv);
   return rv;
}

void CheckTextMode(uchar *buffer, int sbuf)
{
    int c, i, j, imatch;
    uchar pattern[5] = {CH_ESC,'[','0','0','m'}; /*was [0m */
    int spattern = 5;
 
    /* if CH_ESC,"[0m", TextMode change */
    j = 0;
    imatch = 0;
    for (j = 0; j < sbuf; j++) {
        if ((sbuf - j) < spattern && imatch == 0) break;
        c = buffer[j];
        if (c == pattern[imatch]) {
            imatch++;
        } else if (pattern[imatch] == '?') {  /*wildcard char*/
            imatch++;
        } else if (pattern[imatch] == '0' && (c>='0' && c <='9')) { /*numeric*/
            imatch++;
        } else {
            if (imatch > 0) {
               if (j > 0) j--; /* try again with the first match char */
               imatch = 0;
            } 
        }
        if (imatch == spattern) { /*match found*/
	    char mystr[5];
            i = (j+1) - imatch;  /*buffer[i] is the match */
            // bTextMode = buffer[i+2] & 0x0f;  /* usu 0 or 1 */
	    mystr[0] = buffer[i+2];
	    mystr[1] = buffer[i+3];
	    mystr[2] = 0;
            bTextMode = atob(mystr);
            dbglog("TextMode changed to %d\n",bTextMode);
        }
        else if (imatch > spattern) break;
    }
    return;
}

/*======== end Linux =================================*/
#endif

void printm( char *pattn, ... )
{ /* all sol error/info messages go to log or stderr by default */
    va_list arglist;
    FILE *fp;
    if (fdbglog && fplog != NULL) fp = fplog;
    else fp = stderr;
    va_start(arglist, pattn);
    vfprintf(fp, pattn, arglist);
    va_end(arglist);
    fprintf(fp,"\r\n");
}

void dbglog( char *pattn, ... )
{
    va_list arglist;
    int fadd = 0;
    int l;
#ifdef WIN32
	FILE *fp;
	// char _pattn[80];
	if (fdebug == 0 && fdbglog == 0) return;
	if (fdbglog && fplog != NULL) fp = fplog;
	else fp = stderr;
	l = (int)strlen(pattn);
	if ((l >= 1) && (pattn[l-1] == '\n')) { /*check EOL chars*/
	   if (pattn[l-2] == '\r') fadd = 0;
	   else fadd = 0;  /* '\n' only */
	   // else _pattn[l-1] = 0;  /*truncate, add \r\n below, dangerous*/
	} else fadd = 1;  /*no line-end, so add it below*/
        va_start(arglist, pattn);
        vfprintf(fp, pattn, arglist);
        va_end(arglist);
	if (fadd) fprintf(fp,"\r\n");
#else
    char logtmp[LOG_MSG_LENGTH];
    if (fdebug == 0 && fdbglog == 0) return;
    l = strlen(pattn);
    if ((l >= 1) && (pattn[l-1] == '\n')) fadd = 0;
    else fadd = 1;
    va_start( arglist, pattn );
    vsnprintf(logtmp, LOG_MSG_LENGTH, pattn, arglist);
    va_end( arglist );
    if (fadd) strcat(logtmp,"\n");
    if (fdbglog) print_log(logtmp);
    else if (fdebug) fprintf( stderr, "%s", logtmp);
#endif
}
             
void dbg_dump(char *tag, uchar *pdata, int len, int fascii)
{
    FILE *fp = NULL;
    if (fdebug == 0 && fdbglog == 0) return;
    if (fdbglog && (fplog != NULL)) fp = fplog;
    dump_log(fp,tag,pdata,len,(char)fascii); /*uses fplog or fpdbg */
}

#ifdef OLD
static void set_latency( struct timeval *t1, struct timeval *t2, long *latency)
{
   long nsec;
   nsec = t2->tv_sec - t1->tv_sec;
   if ((ulong)(nsec) > 1) nsec = 1;
   *latency = nsec*1000 + (t2->tv_usec - t1->tv_usec)/1000;
}
#endif

static int sol_send(uchar *buf, int len, SOL_RSP_PKT *rsp)
{
    int rv = 0;
    if (bSolVer == 1) {
       rv = lan_send_sol(buf, len, rsp);
    } else {
       rv = lan2_send_sol(buf, len, rsp);
       if (rv == 0x01) rv = 0;  
       /* else bit-mapped error, 0x02 = need retry*/
    }
    return(rv);
}

static int sol_recv( SOL_RSP_PKT *rsp)
{
    int rv = 0;
    if (bSolVer == 1) {
       rv = lan_recv_sol( rsp );
    } else 
       rv = lan2_recv_sol( rsp );
    return(rv);
}

static SockType  sol_get_fd(void)
{
    SockType fd = 0;
    if (bSolVer == 1) {
       fd = lan_get_fd();
    } else 
       fd = lan2_get_fd();
    return(fd);
}

/*
 * sol_output_handler 
 * This routine is called both in isol.c and ipmilanplus.c/lan2_recv_handler
 * whenever SOL data is received from the BMC for output to the console. 
 */
void sol_output_handler(SOL_RSP_PKT *rsp)
{
    if (rsp == NULL) return;
    if (rsp->type == PAYLOAD_TYPE_SOL)
    {
#ifdef WIN32
	 fgotrecv = 1; 
         if (fdbglog) dbg_dump("sol_output(console_out)",rsp->data,rsp->len,1);
	 console_out(rsp->data,rsp->len);
#else
         uchar *pdata;
         int len;
         int i;
         uchar c;
         // int mode;

	 fgotrecv = 1; 
         pdata = rsp->data;
         len   = rsp->len;
         if (fdbglog) {
             // dbglog("sol_output: len = %d\n",len);
             dbg_dump("sol_output",pdata,len,1); /*like printlog*/
         }
         CheckTextMode(pdata,len);
         for (i = 0; i < len; i++) {
	      c = pdata[i];
	      /* do special character handling here?  
	       * CH_CR, 0xb0-0xb3, 0xdb */
              putc(c, stdout);
         }
         fflush(stdout);
#endif
	 if (fTrace) { fwrite(rsp->data,1,rsp->len,fp_trc); fflush(fp_trc); }
    } else { 
         dbglog("sol_output: rsp.type=%x, rsp.len=%d\n", rsp->type, rsp->len);
    }
}

static int sol_keepalive(int type)
{
   int rv = 0;
   time_t end; 

   time(&end); 
   if (sol_timeout == 0) return(rv);
   if ((end - keepalive_start) > sol_timeout) {
       dbglog("sol_keepalive: timeout, lan%d\n",bSolVer);  /*++++*/
       if (bSolVer == 1) {
           rv = lan_keepalive(type);   
       } else {
           SOL_RSP_PKT  rs;
           rv = lan2_keepalive(type, &rs);
           if (rv >= 0 && rs.len != 0) { /*handle any rsp data*/
               if (rs.type == IPMI_PAYLOAD_TYPE_SOL) {
	       dbglog("keepalive: output_handler(%d)\n",rs.len);
               sol_output_handler(&rs);
	       }
	   }
       }
       time(&keepalive_start);  /*time in seconds*/
       dbglog("sol_keepalive sent, rv = %d\n",rv);
       flush_log();
   }
   return(rv);
}

static void sol_keepalive_reset(void)
{
    /* not idle, so reset the keepalive timer */
    time(&keepalive_start);  /*time in seconds*/
}

static int send_break( void )
{
    SOL_RSP_PKT  rs;
    int rv;

    if (sol_ver == 1) return LAN_ERR_NOTSUPPORT;
    rv = lan2_send_break(&rs);
    dbglog("lan2_send_break rv=%d rs.len=%d\n", rv,rs.len);
    return(rv);
}

static int send_ctlaltdel( void )
{
    uchar buf[8];
    int rv, rlen;

// #define TESTALT  1
#ifdef TESTALT
    SOL_RSP_PKT  rs;
    /* This doesn't work.  Fix it later. */
    /* Problem is that keycodes must be translated for serial console */
    /* CtrlL = 0x0706, AltL = 0x0703, Del = 0x007f, MetaDel = 0x087f 
     * Del = keycode 0x53 
     * 1b 53 33 7e       = alt+del
     * 1b 4f 48          = ctl-alt-home
     * 1b 4f 46          = ctl-alt-end
     * 1b 53 33 3b 38 7e = ctl+alt+shift+del
     * 1b 53 33 3b 33 7e = ctl+alt+ins
     * 1b 53 33 3b 35 7e = ctl+alt+ins
     * 1b 53 35 3b 37 7e = ctl+alt+pgup
     * 1b 53 36 3b 37 7e = ctl+alt+pgdn
     * Need to find a sequence at serial console that does a reboot
     * when the command prompt is not up.
     */
    buf[0] = 0x1b;
    buf[1] = 0x53;
    buf[2] = 0x33;
    buf[3] = 0x3b;
    buf[4] = 0x36;
    buf[5] = 0x7e;
    rv = sol_send(buf, 6, &rs);
    rlen = rs.len;
#else
   uchar rbuf[16];
   uchar cc;

   rlen = sizeof(rbuf);
   buf[0] = 5;  /*soft reset via ACPI */
   rv = ipmi_cmdraw(CHASSIS_CTL,NETFN_CHAS,BMC_SA, PUBLIC_BUS, BMC_LUN,
                   buf,1,rbuf,&rlen,&cc,fdebug);
   if (rv == 0 && cc != 0) rv = cc;
#endif
    dbglog("send_ctlaltdel rv=%d rlen=%d\n", rv,rlen);
    return(rv);
}

static void show_esc_help( void )
{
	printf("%c?\r\n",sol_esc_ch);
        printf("Supported SOL escape sequences:\r\n");
        printf("\t%c.  - terminate connection\r\n",sol_esc_ch);
        printf("\t%cB  - send break\r\n",sol_esc_ch);
        printf("\t%cD  - send ctl-alt-delete (chassis soft reset)\r\n",sol_esc_ch);
        printf("\t%c?  - show this help message\r\n",sol_esc_ch);
        printf("\t%c%c  - send the escape character by typing it twice\r\n",
		sol_esc_ch, sol_esc_ch);
}

int sol_input_handler( uchar *input, int ilen)
{
   int rv = 0;
   uchar payload[IPKT_SZ]; 
   SOL_RSP_PKT  rs;
   int i, length, t;
   static uchar lastc = 0;
   char rvend = 0;
   char fdoinput = 1;
   uchar c;
#ifdef WIN32
   uchar c1, c2, c3;
   unsigned long dwKeyCode;
#endif

   memset(&payload, 0, sizeof(payload)); 
   length = 0;
   rs.len = 0;

   if ((fdebug > 2) && (ilen > 4)) 
      dbg_dump("sol_input dump:",input,ilen,1);
   // i = 0;
   for (i = 0; i < ilen; i++)
   {
    if (fdebug > 2) 
      dbglog("sol_input keys[%d]: %02x %02x %02x %02x, ilen=%d\n",  
	  i, input[i], input[i+1], input[i+2], input[i+3], ilen);
   /* look for console escape sequences (~) */

    if (sol_esc_pending) {
	sol_esc_pending = 0;
	/* next char in 2-char esc seq */
#ifdef WIN32
	if ( ((input[i+3] & 0x01) == 0) ||  /* h.o byte, if not keydown */
	     (i < 0)) {  /* skip h.o. bytes in dwChar for windows */
	   fdoinput = 0;  /*skip this input*/
	   sol_esc_pending = 1;  /*if here, esc was pending*/
   	   if (fdebug > 2)
      	      dbglog("sol_input: skip, %02x!=X1 for i=%d\n",input[i+3],i);
	} else
#endif
	switch(input[i]) {
	   case '.':
		dbglog("sol_input RV_END (%02x %02x)\n",sol_esc_ch,input[i]);
		rvend = 1;
		fdoinput = 0;
		break;
	   case 'b':
	   case 'B':
		rv = send_break();
		fdoinput = 0;
		break;
	   case 'd':
	   case 'D':
		rv = send_ctlaltdel();
		fdoinput = 0;
		break;
	   case '?':
		show_esc_help();
		fdoinput = 0;
		break;
	   default:
		/* includes entering "~~" to output '~' */
                dbglog("sol_input sol_esc no match (%02x %02x/%c%c)\n",
			sol_esc_ch,input[i], sol_esc_ch,input[i]);
		fdoinput = 1;
		break;
	}
    } else if (input[i] == sol_esc_ch) {
	/* start of new sol_esc seq */
        if (fdebug > 2)
           dbglog("sol_input esc_pending (%02x %02x)\n",input[i],input[i+1]);
	if ((sol_esc_len == 1) ||
	    ((ilen > (i+1)) && (input[i+1] == '.')) ) {   /*then exit now*/
           dbglog("sol_input RV_END (%02x %02x)\n",input[i],input[i+1]);
	   rvend = 1;
	}
	fdoinput = 0;
	sol_esc_pending = 1;
    }
    else fdoinput = 1;  /*send as normal input*/

    if (fdoinput) 
    {
        c = input[i];
#ifdef WIN32
        c1 = input[i+1];
        c2 = input[i+2];
        c3 = input[i+3];

        dwKeyCode = (c << 24) | (c1 << 16) | (c2 << 8) | c3;
        dbglog("sol_input[%d]: KeyCode=0x%08X, '%c'\n", 
               i, dwKeyCode, (0x20 <= c && c <= 0x7E ? c : '.') );

	/* check Windows input to see if it should be sent */
        t = console_in(dwKeyCode, &payload[length], sizeof(payload) - length);
        if (t > 0) length += t;
        lastc = c;
#else
        /*
         * Linux end-of-line handling with fCRLF: 
	 *      (0) CR, (1) CR+LF, (2) LF
         * Note that the CR+LF logic works for BIOS, but causes
         * extra returns after Linux commands. Using -l sets CR+LF.
	 * Using bTextMode detects if bg color, as in BIOS Setup
	 * or other apps that need CR+LF.
	 * 
	 * If stty has '-onlcr', then LF (fCRLF=2) is fine, but 
	 * if stty has 'onlcr', LF causes two output lines,
	 * so CR is the default (fCRLF=0), which works for 
	 * either 'onlcr' or '-onlcr'.
	 * Using -r (fRaw) has the same effect as LF (fCRLF=2).
         */
        if (c == CH_LF && !fRaw) { // input=enter/LF and not raw (-r)
            if ((fCRLF == 1) || (bTextMode > 40)) { /*-l option or bg color*/
		/*CR+LF: insert CR here, LF below*/
		payload[length++] = CH_CR; 
	    } 
	    else if (fCRLF == 2) ; /*LF: same as raw, use CH_LF*/
            else  c = CH_CR;  /* CR: switch LF to CR */
        }
        /* copy other data to payload */
        payload[length++] = c;
        lastc = c;
        if (c == CH_CR || c == CH_LF)
	    dbglog("sol_input[%d] payload: %02x %02x, bTextMode=%d\n",
			i,payload[0],payload[1],bTextMode);
#endif
	if (length >= max_bmc_data) {  /*overflow, truncate*/
	    dbglog("sol_send: ilen(%d) >= BMC max data(%d), truncated\n", 
		   ilen,max_bmc_data);
	    break;
	} else if (length >= sizeof(payload)) {  /*overflow, truncate*/
	    dbglog("sol_send: ilen(%d) >= buffer size(%d), truncated\n", 
		   ilen,sizeof(payload));
	    break;
	} 
    }  /*endif fdoinput*/
#ifdef WIN32
    i += 3;  /*increment to next dwKeyCode (3+1)*/
#endif
   }  /*end-for input[ilen] */

   if (length > 0) {
      /* Send the SOL payload */
      for (t = 0; t < sol_retries; t++) 
      {
         if (t > 0) 
	    os_usleep(0,(retry_time * 1000)); /*wait between retries 5000 us*/
         rv = sol_send(payload, length, &rs);
	 dbglog("sol_send(%d,%d) rv=%d rs.len=%d\n", t,length,rv,rs.len);
	 if (rv >= 0) {  /* rv ok */
	    if (rs.len != 0) {   /* have some rsp data, so handle it */
             if (rs.type == IPMI_PAYLOAD_TYPE_SOL) {
	       dbglog("output: handler(%d)\n",rs.len);
               sol_output_handler(&rs);
	     } else {
	       dbglog("WARNING: after sol_send, rs.type=%d, not SOL\n",rs.type);
	     }
	    } /*endif have rsp data*/
	    rv = 0;  /*recvd something, so dont retry */
         }
	 if (rv == 0) break;
	 /* else retry again if rv < 0 or rv == 0x02 */
      } /*end for loop*/
      if (rv < 0) rv = LAN_ERR_DROPPED;
   }
   if (rvend) rv = RV_END;
   return(rv);
}

static int send_activate_15sol(void)
{
   int rv = 0;
   uchar actcmd;
   uchar netfn;
   uchar ibuf[64];
   uchar rbuf[64];
   int rlen;
   uchar ilen, cc;

   lan_get_sol_data(fSolEncryption,sol15_iseed,&sol15_wseed);
   actcmd  = (uchar)(ACTIVATE_SOL1 & 0x00ff); 
   netfn   = NETFN_SOL; 
   if (fSolEncryption) ibuf[0] = 0x11;   /* use encryption, activate SOL */
   else ibuf[0] = 0x01;   /*no encryptionn, no serial alerts, activate SOL */
   memcpy(&ibuf[1],&sol15_wseed,4);  /*32-bit seed*/
   ilen = 5;
   rlen = sizeof(rbuf);
   rv = ipmi_cmdraw(actcmd,netfn,BMC_SA, PUBLIC_BUS, BMC_LUN,
                   ibuf,ilen,rbuf,&rlen,&cc,fdebug);
   if (rv == 0 && cc != 0) rv = cc;
   /* switch(cc) { case 0x80: SOL already active on another session
    *              case 0x81: SOL disabled
    *              case 0x82: SOL activation limit reached  }
    */
   if (rv == 0) {  /*success*/
      dbg_dump("sol 15 act_resp",rbuf,rlen,0);
      /* sol 15 act_resp (len=4): 0000: 00 01 64 64 */
      /* 00=auth, 01=seed_cnt, 64=in_payload_sz, 64=out_payload_sz */
      lan_set_sol_data(fSolEncryption,rbuf[0],rbuf[1], rbuf[2], rbuf[3]);
   }
   return(rv);
}

static int send_deactivate_15sol(void)
{
   int rv = -1;

   uchar actcmd;
   uchar netfn;
   uchar ibuf[64];
   uchar rbuf[64];
   int  rlen;
   uchar ilen, cc;

   /* SOL for IPMI 1.5 does not have an explicit deactivate, so
    * use an activate with data0 = 00. 
    */
   actcmd  = (uchar)(ACTIVATE_SOL1 & 0x00ff); 
   netfn   = NETFN_SOL; 
   if (fSolEncryption) ibuf[0] = 0x10;  /* deactivate, use encryption */
   else ibuf[0] = 0x00;    /*deactivate*/
   memcpy(&ibuf[1],&sol15_wseed,4);  /*32-bit seed*/
   ilen = 5;
   rlen = sizeof(rbuf);
   rv = ipmi_cmdraw(actcmd,netfn,BMC_SA, PUBLIC_BUS, BMC_LUN,
                   ibuf,ilen,rbuf,&rlen,&cc,fdebug);
   if (rv == 0 && cc != 0) rv = cc;
   dbg_dump("sol 15 deact_resp",rbuf,rlen,0); /* 80 00 32 ff */
   return(rv);
}

int send_activate_sol(uchar verSOL)
{
   int rv = 0;
   int in_payload_size, out_payload_size, udp_port;
   uchar actcmd;
   uchar netfn;
   uchar ibuf[64];
   uchar rbuf[64];
   int rlen;
   uchar ilen, cc;
   
   sol_ver = verSOL;
   /* activate SOL = 0x01, get SOL status = 0x05 */
   if (verSOL == 1) {
      rv = send_activate_15sol();
      return(rv);
   } else {   /* use IPMI 2.0 method */
      actcmd  = ACTIVATE_PAYLOAD;
      netfn   = NETFN_APP;  
      ibuf[0] = PAYLOAD_TYPE_SOL;
      ibuf[1] = payload_instance;  /* payload instance */
      ibuf[2] = SOL_SERIAL_ALERT_MASK_DEFERRED;
      if (fSolEncryption)     ibuf[2] |= 0x80;
      if (fSolAuthentication) ibuf[2] |= 0x40;
      if (vend_id == VENDOR_INTEL) 
           ibuf[2] |= SOL_BMC_ASSERTS_CTS_MASK_TRUE;
      else ibuf[2] |= SOL_BMC_ASSERTS_CTS_MASK_FALSE;
      ibuf[3] = 0;
      ibuf[4] = 0;
      ibuf[5] = 0;
      ilen = 6;
   }
   rlen = sizeof(rbuf);
   rv = ipmi_cmdraw(actcmd,netfn,BMC_SA, PUBLIC_BUS, BMC_LUN,
                   ibuf,ilen,rbuf,&rlen,&cc,fdebug);
   dbg_dump("sol act_cmd",ibuf,ilen,0);
   dbglog("send_activate v2(%x,%x) rv = %d cc = %x\n",actcmd,netfn,rv,cc);
   if (rv >= 0) {
      rv = cc;
      switch(cc) {
         case 0x00: 
	       if (rlen != 12) {
	         printm("Unexpected SOL response data received, len=%d\n",rlen);
	         rv = -1;
               }
               break;
         case 0x80: 
	       printm("SOL payload already active on another session\n");
               break;
         case 0x81: 
	       printm("SOL payload disabled\n");
               break;
         case 0x82: 
	       printm("SOL payload activation limit reached\n");
               break;
         case 0x83: 
	       printm("Cannot activate SOL payload with encryption\n");
               break;
         case 0x84: 
	       printm("Cannot activate SOL payload without encryption\n");
               break;
         default:
	       printm("Cannot activate SOL, ccode = 0x%02x\n",cc);
               break;
      }
   }
   if (rv == 0) {   /* success, use the response data */
      /* only here if response data is from IPMI 2.0 method */
      dbg_dump("sol act_resp",rbuf,rlen,0);
      in_payload_size  = rbuf[4] + (rbuf[5] << 8);
      out_payload_size = rbuf[6] + (rbuf[7] << 8);
      udp_port         = rbuf[8] + (rbuf[9] << 8);
      dbglog("activate ok, in=%d out=%d port=%d\n",
			in_payload_size, out_payload_size, udp_port);
      if (bSolVer == 2) 
         lan2_set_sol_data(in_payload_size, out_payload_size, udp_port,
			   (void *)lan2_recv_handler, sol_esc_ch);
   }
   return(rv);
}

int send_deactivate_sol(uchar verSOL)
{
   int rv = 0;
   uchar deactcmd;
   uchar netfn;
   uchar ibuf[64];
   uchar rbuf[64];
   int rlen;
   uchar ilen, cc;

   if (verSOL == 1) {
      return(send_deactivate_15sol());
   } else {   /* use IPMI 2.0 method */
      deactcmd  = DEACTIVATE_PAYLOAD;
      netfn   = NETFN_APP;
      ibuf[0] = PAYLOAD_TYPE_SOL;   /* payload type */
      ibuf[1] = payload_instance;  /* payload instance */
      ibuf[2] = 0;
      ibuf[3] = 0;
      ibuf[4] = 0;
      ibuf[5] = 0;
      ilen = 6;
   }
   rlen = sizeof(rbuf);
   rv = ipmi_cmdraw(deactcmd,netfn,BMC_SA, PUBLIC_BUS, BMC_LUN,
                   ibuf,ilen,rbuf,&rlen,&cc,fdebug);
   dbglog("sol deactivate rv = %d, cc = %x\n",rv,cc);
   if (rv == 0 && cc != 0) rv = cc;
   return(rv);
}

int sol_data_loop( void )
{
   int istdin;
   int rv = 0;
   uchar ibuf[IDATA_SZ];
   uchar cbuf[IDATA_SZ]; 
   int szibuf, szcbuf;
   SockType fd;
   int len;
   fd_set read_fds;
   fd_set error_fds;
   static uchar lastc = 0;
   int c;
#ifdef WIN32
   int c1;
   time_t ltime1 = 0;
   time_t ltime2 = 0;
   HANDLE thnd;
   istdin = 0;
#else
   long ltime1 = 0;
   long ltime2 = 0;
   istdin = fileno(stdin);
#endif

   /* 
    * OK, now loop and listen to 
    *   stdin for user input 
    *   interface fd for incoming SOL data 
    */
   /* sizeof(ibuf/cbuf)=IDATA_SZ(512), max_bmc_data=203 for sol data */
   szibuf = max_bmc_data;
   szcbuf = max_bmc_data;  
   fd = sol_get_fd();
   if (fScript) { /* open file_scr */
      fp_scr = fopen(file_scr,"r");
      if (fp_scr == NULL) {
	  printm("Cannot open %s, ignoring\n",file_scr);
          fScript = 0;
      }
   }
   if (fTrace) { /* open file_trc for writing */
      fp_trc = fopen(file_trc,"a");
      if (fp_trc == NULL) {
	  printm("Cannot open %s, ignoring\n",file_trc);
          fTrace = 0;
      }
   }

   dbglog("stdin = %d, intf->fd = %d\n",istdin,fd);
   if (sol_esc_len == 1) 
     printf("[SOL session is running, use '%c' to end session.]\n",sol_esc_ch);
   else 
     printf("[SOL session is running, use '%c.' to end, '%c?' for help.]\n",
		sol_esc_ch, sol_esc_ch);

   tty_setraw(2);
   sol_keepalive_reset();
#ifdef WIN32
   thnd = CreateThread(NULL, 0, &input_thread, NULL, 0, NULL);
   if (thnd == NULL) { 
       printm("CreateThread error, aborting\n");
       rv = -1; 
       sol_done = 1; 
   }
   console_open(fdebug);
#else
   if (bSolVer == 1) {   /* jump start the session v1.5 data */
      ibuf[0] = 0x1b; /*escape*/
      ibuf[1] = '{';
      rv = sol_input_handler(ibuf,2);
   }
#endif
   if (fdbglog) time((time_t *)&ltime1);
		   
   while (sol_done == 0) 
   {
       if (bKeepAlive > 0) {  /* send KeepAlive if needed */
           rv = sol_keepalive(bKeepAlive);   
	   /* if keepalive error, try to keep going anyway */
       }
       if (fdebug > 2) dbglog("os_select(%d,%d) called\n",istdin,fd);

       rv = os_select(istdin,fd, &read_fds, &error_fds);
       if (rv < 0) { 
	   dbglog("os_select(%d,%d) error %d\n",istdin,fd,rv);
	   perror("select"); 
           sol_done = 1;
       } else if (rv > 0) {
	   if (fdbglog) { 
	       time((time_t *)&ltime2);
	       dbglog("select rv = %d sockfd=%x stdin=%x time=%ld\n", rv,
			FD_ISSET(fd, &read_fds),FD_ISSET(istdin, &read_fds),
			(ltime2 - ltime1)); 
	   } 

           if (FD_ISSET(fd, &read_fds)) {
               SOL_RSP_PKT rs;
               rs.len = 0;
               /* receive output from BMC SOL */
               rv = sol_recv(&rs);
#ifdef WIN32
		/* Windows sometimes gets rv = -1 here */
               if (rv == -1) rv = -(WSAGetLastError());
#endif
               dbglog("output: recv rv = %d, len = %d\n",rv,rs.len);
               if (rv < 0) {
                   dbglog("Error %d reading SOL socket\n",rv);
                   printm("Error %d reading SOL socket\n",rv);
                   sol_done = 1;
               } else {
		   /*sol_output_handler sets fgotrecv*/
		   sol_output_handler(&rs);  
                   sol_keepalive_reset();
		   /* go back to select until there is no more socket data */
		   continue;  
	       }
           }  /*endif fd*/

           if (FD_ISSET(fd, &error_fds)) {
                   dbglog("Error selecting SOL socket\n",rv);
           }  /*endif fd*/

           if (FD_ISSET(istdin, &read_fds)) {
               /* input from stdin (user) if not WIN32 */
               memset(ibuf,0,szibuf);
               len = os_read(istdin, ibuf, szibuf);
               dbglog("stdin: read len=%d, %02x %02x %02x %02x\n",
					len,ibuf[0],ibuf[1],ibuf[2],ibuf[3]);
               if (len <= 0) {
                  dbglog("Error %d reading stdin\n",len);
                  printm("Error %d reading stdin\n",len);
                  sol_done = 1;
               } else {
                  rv = sol_input_handler(ibuf,len);
                  dbglog("sol_input: handler rv = %d\n",rv);
                  if (rv < 0) {
                      sol_done = 1;
                      if (rv == RV_END) {
		          dbglog("sol_data RV_END\n");
		          printf("\n%s exit via user input \n",progname);
                      } /* else drop through to show_outcomes */
                  }
		  else sol_keepalive_reset();
               }
           }  /*endif stdin*/

       }  /*endif select rv > 0 */
       else if (fScript) {   /*rv == 0*/
	   /* if we sent a script line, but no receive yet, keep waiting*/
	   if (fsentok && (fgotrecv == 0)) {
		dbglog("sol_data script sentok, but no recv\n");
		// continue; 
	   }
	   fsentok = 0;
	   /* read input stream from script file */
           memset(cbuf,0,szcbuf);
           /* read one line at a time */
	   rv = 0;
	   for (len = 0; len < szcbuf; ) {
                  c = fgetc(fp_scr);
		  if (c == EOF) { rv = -2; break; }
#ifdef WIN32
		  if ((c == '\n') && (lastc != '\r')) c1 = '\r'; /*Enter*/
		  else c1 = c;
		  Ascii2KeyCode((uchar)c1,&cbuf[len]);
		  len += 4;
#else
		  cbuf[len++] = c;
#endif
		  if (c == '\n') break; 
		  lastc = (uchar)c;
	   } 
           dbglog("script: read len=%d, %02x %02x %02x\n",
					len,cbuf[0],cbuf[1],cbuf[2]);
           if (rv != 0 || len == 0) {
                  dbglog("Stop reading file %s, rv=%d\n",file_scr,rv);
		  fclose(fp_scr); 
                  fScript = 0;
           } else {
		  /* TODO: add processing for some wait/sleep/prompt commands */
                  rv = sol_input_handler(cbuf,len);
                  dbglog("input: handler rv = %d\n",rv);
                  if (rv < 0) {
                      sol_done = 1;
                      if (rv == RV_END) {
		          dbglog("sol_data RV_END\n");
		          printf("\n%s exit via user input \n",progname);
                      } else printf("Error %d processing input\n",rv);
                  } else {
		      fsentok = 1;   /*sent a script line successfully*/
		      fgotrecv = 0;  /*need to wait for recv*/
		  }
           }
       } /*endif fScript*/
   }  /*end while*/
   if (rv == RV_END) rv = 0;
#ifdef WIN32
   os_usleep(0,5000);   /*wait 5 ms for thread to close itself*/ 
   CloseHandle(thnd);
   console_close();
#endif
   tty_setnormal(2);
   if (fScript) fclose(fp_scr); /* close file_scr */
   if (fTrace) fclose(fp_trc); /* close file_trc */
   return(rv);
}

static void show_usage(void)
{
                printf("Usage: %s [-acdeiolnrsvxz -NUPREFTVY]\n", progname);
                printf(" where -a     activate SOL console session\n");
                printf("       -d     deactivate SOL console session\n");
                printf("       -c'^'  set escape Char to '^', default='~'\n");
                printf("       -e     Encryption off for SOL session\n");
                printf("       -i file  Input script file\n");
                printf("       -o file  Output trace file\n");
                printf("       -l     Legacy mode for BIOS/DOS CR+LF\n");
                printf("       -n 1   Payload instance Number, default=1\n");
                printf("       -r     Raw termio, no VT-ANSI translation\n");
                printf("       -s NNN Slow link delay, default=100usec\n");
#ifdef NOT_DOCUMENTED
                printf("       -t N   SOL send timeout reTries, default=1\n");
                printf("       -u N   Wait time in msec for tuning, default=500\n");
#endif
#ifdef WIN32
                printf("       -w     do not use Windows Console buffer, use stdio\n");
#endif
                printf("       -v     debug log filename (default=isoldbg.log)\n");
                printf("       -x     show eXtra debug messages in debug log\n");
                printf("       -z     show even more debug messages\n");
		print_lan_opt_usage(1);
}

#ifdef METACOMMAND
int i_sol(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
#endif
{
   int ret = 0;
   int i;
   uchar devrec[16];
   uchar bmcver[2];
   int c;

   printf("%s ver %s\n", progname,progver);

   parse_lan_options('V',"2",0);  /*default to user priv*/

   while ( (c = getopt( argc, argv,"ac:dei:k:ln:o:p:rs:t:u:wv:xzEF:J:N:P:R:T:U:V:YZ:?")) != EOF ) 
      switch(c) {
          case 'a': factivate = 1;   break;    /*activate*/
          case 'd': fdeactivate = 1; break;    /*deactivate*/
          case 'c':     		       /* escape char */
                if (strncmp(optarg,"0x",2) == 0) 
			sol_esc_ch = htoi(&optarg[2]);  /*hex char value*/
                else if (optarg[0] >= '0' && optarg[0] <= '9') 
			sol_esc_ch = atob(optarg); /*decimal char value*/
		else    sol_esc_ch = optarg[0];  /*single ascii char*/
		sol_esc_len = 1;  /*set single-char esc sequence */
		break;
          case 'e': fSolEncryption = 0; break; /*encryption off*/
          case 'k': i = atoi(optarg); /*sol keepalive timeout, default = 30*/
		    if (i < 0) 
			printf("Invalid keepalive timeout %d, ignoring.\n",i);
		    else sol_timeout = i;
		    if (i == 0) bKeepAlive = 0;
		    if (i == 32) bKeepAlive = 2; /*++++ for debug*/
		    break;
          case 'l': fCRLF = 1; break;          /*do legacy CR+LF translation*/
          case 'i': strncpy(file_scr,optarg,sizeof(file_scr)); 
		    fScript = 1;  break;    /*script input file*/
          case 'n': i = atoi(optarg);     /* payload_instance */
		    if ((i <= 0) || (i > 255)) 
			printf("Invalid payload instance %d, ignoring.\n",i);
		    else payload_instance = i;
		    break;
          case 'o': strncpy(file_trc,optarg,sizeof(file_trc));
		    fTrace = 1;  break;     /*trace output file*/
          case 'r': fRaw = 1; fCRLF = 0; fUseWinCon = 0;
			break; /*raw termio, xlate off*/
          case 's': sol_recvdelay = atoi(optarg); break; /*slow recv delay*/
          case 't': sol_retries = atoi(optarg); break;   /*timeout/retries*/
          case 'u': wait_time = atoi(optarg); break;  /*wait_time for tuning*/
          case 'v': strncpy(dbglog_name,optarg,sizeof(dbglog_name));
		    fdbglog = 1;  break;      /*name of debug log file*/
          case 'w': fUseWinCon = 0;  break;    /*do not use Console routines*/
          case 'x': 
		    if (fdebug == 0) fdebug = 2; /*only normal via dbglog */
		    else fdebug++;  /* -xx = full, -xxx = max */
		    break; 
          case 'z': fdebug = 3;  break;  /*full debug messages */
          case 'V':    /* priv level */
		fprivset = 1;
          case 'p':    /* port */
          case 'N':    /* nodename */
          case 'U':    /* remote username */
          case 'P':    /* remote password */
          case 'R':    /* remote password */
          case 'E':    /* get password from IPMI_PASSWORD environment var */
          case 'F':    /* force driver type */
          case 'T':    /* auth type */
          case 'J':    /* cipher suite */ 
          case 'Y':    /* prompt for remote password */
          case 'Z':    /* set local MC address */
                parse_lan_options(c,optarg,fdebug);
                break;
	  default:
                show_usage();
		ret = ERR_USAGE;
		goto do_exit;
      }

   if (is_remote() == 0) {  /* no node specified */
      printf("Serial-Over-Lan Console requires a -N nodename\n");
      ret = ERR_BAD_PARAM;
      goto do_exit;
   }
   if (fdeactivate && !fprivset) 
      parse_lan_options('V',"4",0); /*deactivate requires admin priv */

   /* fdebug: 1=debug/no packets, 2=debug log only, 3=full debug, 4=max debug*/
   if (fdebug > 1) fdbglog = 1;

   if (factivate == 0 && fdeactivate == 0) {
      show_usage();
      printf("Error: Need either -a or -d for sol\n");
      ret = ERR_BAD_PARAM;
      goto do_exit;
   }
#ifdef WIN32
   else if (factivate) {
      if (fdebug) printf("%s activate, connecting ...\n", progname);
   }
#endif
   if (fdbglog) {
      // strcpy(log_name,dbglog_name);
      open_log(dbglog_name);
      dbglog("%s ver %s\r\n", progname,progver);
   }

   i = get_driver_type(); /*see if user explictly set type*/
   if (i == DRV_UNKNOWN) bDriver = DRV_UNKNOWN;  /*no driver type specified*/
   else bDriver = (uchar)i;

   ret = ipmi_getdeviceid(devrec,16,fdebug);
   if (ret != 0) {
      goto do_exit;
   } else {
      ipmi_maj = devrec[4] & 0x0f;
      ipmi_min = devrec[4] >> 4;
      vend_id  = devrec[6] + (devrec[7] << 8) + (devrec[8] << 16); 
      prod_id  = devrec[9] + (devrec[10] << 8);
      show_devid( devrec[2],  devrec[3], ipmi_maj, ipmi_min);
      bmcver[0] = devrec[2];
      bmcver[1] = devrec[3];
      switch(vend_id) {
	case VENDOR_DELL:  /*Dell == 0x0002A2*/
	   max_bmc_data = MAX_DELL_DATA; /*shorter max data*/
	   break;
	case VENDOR_SUPERMICROX:
	case VENDOR_SUPERMICRO:
   		if (!fprivset) parse_lan_options('V',"4",0); /*requires admin priv*/
	case VENDOR_LMC:
	case VENDOR_PEPPERCON: /* 0x0028C5  Peppercon/Raritan*/
           sol_timeout = 10;  /* shorter 10 sec SOL keepalive timeout */
	   max_bmc_data = MAX_OTHER_DATA;
	   break;
	case VENDOR_INTEL:
	   max_bmc_data = MAX_INTEL_DATA;
	   break;
	case VENDOR_KONTRON:
	   max_bmc_data = MAX_KONTRON_DATA; 
	   // bKeepAlive = 1;  
	   break;
	default:
	   max_bmc_data = MAX_OTHER_DATA;
	   break;
      }
   }

   ret = ipmi_getpicmg(devrec,16,fdebug);
   if (ret == 0) fpicmg = 1;

   dbglog("driver=%d fdebug=%d vend_id=%x bmcver=%x.%x ipmi %d.%d\n",
	  bDriver,fdebug,vend_id,bmcver[0],bmcver[1],ipmi_maj,ipmi_min); 
   /* check for SOL support */
   if (ipmi_maj >= 2) { 
       if ((bDriver == DRV_LAN) && (vend_id == VENDOR_INTEL)) {
          /* user specified to use IPMI LAN 1.5 SOL on Intel */
          bSolVer = 1;      /*IPMI 1.5 SOL*/
       } else {
          bSolVer = 2;      /*IPMI 2.0 SOL*/
          if (get_driver_type() == DRV_LAN) { /*now using IPMI LAN 1.5*/
	    char *ptyp; 
            ipmi_close_();           /*close current IPMI LAN 1.5*/
	    if (is_lan2intel(vend_id,prod_id)) ptyp = "lan2i";
            else ptyp = "lan2";
            i = set_driver_type(ptyp); /*switch to IPMI LAN 2.0*/
          }
       } 
   } else if (ipmi_maj == 1) {
       if (ipmi_min >= 5) bSolVer = 1;  /* IPMI 1.5 */
       else bSolVer = 0;  /* IPMI 1.0 */
   } else bSolVer = 0;
   if (bSolVer == 0) {
        printf("Serial Over Lan not supported for this IPMI version\n");
	ret = LAN_ERR_NOTSUPPORT;
        goto do_exit;
   }
#ifndef HAVE_LANPLUS
   if (bSolVer == 2) {
       printf("2.0 LanPlus module not available, trying 1.5 SOL instead\n");
       bSolVer = 1;
   }
#endif
   /* May also want to verify that SOL is implemented here */

   /*
    * Spawn a console raw terminal thread now, which will wait for the 
    * "Activating cmd (0x02)" on success 
    */
   if (fdeactivate) {
      /* Request admin privilege by default, since deactivate requires it. */
      ret = send_deactivate_sol(bSolVer); 
      dbglog("send_deactivate_sol rv = %d\n",ret);
   } else if (factivate) {
      ret = send_activate_sol(bSolVer);
      dbglog("send_activate_sol(%d) rv = %d\n",bSolVer,ret);
      if (bSolVer == 2) {
	 lan2_latency = lan2_get_latency();
	 retry_time = lan2_latency;
         dbglog("lan2_latency = %ld msec (slow > %ld msec)\n",
		lan2_latency,lan2_slow);
	 if ((sol_recvdelay == 0) && (lan2_latency > lan2_slow))
		sol_recvdelay = 500;  /* it is slow, set recvdelay */
	 if (sol_recvdelay != 0) {
            dbglog("set_recvdelay to %dus\n",sol_recvdelay);
 	    lanplus_set_recvdelay(sol_recvdelay);
	 }
      } /*endif bSolVer==2*/
      if (ret == 0) {
         ret = sol_data_loop();
      } /*endif activate ok*/
   }  /*endif do activate */

do_exit:
   if (ret != LAN_ERR_DROPPED) { /*if link dropped, no need to close*/
      ipmi_close_();
      os_usleep(0,(wait_time * 1000));  /*wait 0.5 sec for BMC teardown*/
   }
   if (fdebug) show_outcome(progname,ret);  
   if (fdbglog) close_log();
   return(ret);
}  /* end main()*/
#endif

/* end isol.c */
