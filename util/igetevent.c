/*
 * igetevent.c
 *
 * This utility waits for IPMI Event Messages.
 * Some server management functions want to trigger custom actions or 
 * alerts when IPMI hardware-related events occur, but do not want to
 * track all events, just newly occurring events.
 * The IPMI events also include BIOS events such as Memory and POST errors,
 * which would not be captured by reading the IPMI sensors.
 * This utility waits a specified timeout period for any events, and 
 * returns interpreted output for each event.  It is designed as a
 * scriptable command-line utility, but if the timeout is infinite
 * (-t 0), then this code could be used for a sample service as well.
 *
 * There are several methods to do this which are implemented here.
 * The SEL method (-s):     
 *    This method polls the SEL once a second, keeps track of the last 
 *    SEL event read, and only new events are processed.  This ensures 
 *    that in a series of rapid events, all events are received in order, 
 *    however, some transition-to-OK events may not be configured to 
 *    write to the SEL on certain platforms.  
 *    This method is used if getevent -s is specified.
 * The ReadEventMessageBuffer method (-m getmessage option): 
 *    This uses an IPMI Message Buffer in the BMC firmware to read
 *    each new event.  This receives any event, but if two events 
 *    occur nearly simultaneously, only the most recent of the two 
 *    will be returned with this method.  An example of simultaneous 
 *    events might be, if a fan stops/fails, both the non-critical 
 *    and critical fan threshold events would occur at that time.
 *    This is the default method for getevent.  It would be used 
 *    locally with the Intel IMB driver or with direct/driverless.
 * The OpenIPMI custom method (-m getmessage_mv option if DRV_MV):  
 *    Different IPMI drivers may have varying behavior.  For instance,
 *    the OpenIPMI driver uses the IPMI GetMessage commands internally 
 *    and does not allow client programs to use those commands.  It has 
 *    its own custom mechanism, see getevent_mv().
 *    This method is used locally if the OpenIPMI driver is detected.
 * The Async Event method (-a):
 *    This only gets certain Asynchronous events, like a shutdown 
 *    request from the BMC to an SMS OS service, and get_software_id.
 *    This is supported for Intel IMB driver and OpenIPMI driver only.
 *    This method is only used locally if getevent option -a is used,
 *    and if either MV (openipmi) or IMB driver is loaded.
 *    The ipmiutil_asy init script controls the getevent -a service.
 *    (see DO_ASYNC compile flag comments)
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2005-2006 Intel Corporation. 
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 02/11/05 Andy Cress - created
 * 05/18/05 Andy Cress - modified bmc_enable bits
 * 05/26/05 Andy Cress - added call to decode_sel_entry
 * 09/09/05 Andy Cress - added sensor_type filtering & return type.
 * 03/16/05 Andy Cress - added loop, and -o for frunOnce
 * 06/27/06 Andy Cress 1.1 - specific message for cc=0x80 (no data)
 * 07/18/06 Andy Cress 1.1 - added getevent_mv, etc.
 * 07/26/06 Andy Cress 1.1 - added msgout() routine for fflush
 * 08/08/06 Andy Cress 1.2 - added -s for SEL method
 * 08/08/06 Andy Cress 1.2 - added -s for SEL method
 * 08/22/06 Andy Cress 1.3 - direct IOs added with ipmiutil-1.7.5
 * 09/13/06 Andy Cress 1.4 - handle empty SEL (0xCB), 
 *                           call syncevent_sel after every new event.
 * 09/21/07 Andy Cress 1.21 - implemented IMB Async method for remote
 *                            OS shutdown via SMS requests.
 */
/*M*
Copyright (c) 2009 Kontron America, Inc.
Copyright (c) 2013 Andy Cress <arcress at users.sourceforge.net>
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
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "getopt.h"
#elif defined(DOS)
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "getopt.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(HPUX)
/* getopt is defined in stdio.h */
#include <limits.h> /* for _SC_OPEN_MAX, usu 1024. */ 
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#else
#include <getopt.h>
#include <unistd.h>
#endif
#include <pthread.h>
#include <sys/utsname.h>
#endif
#include <string.h>
#ifdef SOLARIS
/* Solaris */
#define HandleType  long
#elif WIN32
#include "imb_api.h"
#define DO_ASYNC  1   
#elif LINUX
#define LINUX   1   
#include "imb_api.h"
#define DO_ASYNC  1   
#define DO_MVL  1   
#elif BSD
#define DO_MVL  1   
#define HandleType  long
#else
/* other OS */
#define HandleType  long
#endif
#include "ipmicmd.h"
 
#define THREADS_OK 1  
#define IMBPTIMEOUT  200   /*200 ms*/
// #define IPMB_CHANNEL  0x00
// #define LAN_CHANNEL   0x02  
#define ulong  unsigned long
#define uint   unsigned int
#define ushort unsigned short
#define uchar  unsigned char

#define CMD_GET_SOFTWARE_ID  0x00
#define CMD_SMS_OS_REQUEST   0x10

extern int  decode_sel_entry(uchar *evt, char *obuf, int sz); /*see ievents.c*/
extern void set_sel_opts(int sensdesc, int canon, void *sdrs, char fdbg, char utc); /* ievents.c */
extern char *get_sensor_type_desc(uchar stype);  /*see ievents.c*/
extern int write_syslog(char *msg);      /*see isel.c*/
extern char *show_driver_type(int idx);  /*see ipmicmd.h*/
extern int get_sdr_cache(uchar **pret);  /*see isensor.c*/
extern void free_sdr_cache(uchar *pret); /*see isensor.c*/

/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil getevent";
#else
static char * progver   = "3.08";
static char * progname  = "igetevent";
#endif
static char   fdebug    = 0;
static char   fipmilan  = 0;
static char   frunonce  = 0;
static char   futc      = 0;
static char   fAsync    = 0; 
static char   fAsyncOK  = 0;     /*=1 if drvtype detected for it*/
static char   fAsyncNOP = 0;     /*=1 if skip Async actions*/
static char   fbackground = 0; 
static char   frunscript  = 0;
static char   fcanonical  = 0;
static char   fsettime    = 0;   /* =1 if timeout is set by -t */
static uchar  evt_stype = 0xff;  /* event sensor type, 0xff = get any events */
static uchar  evt_snum  = 0xff;  /* event sensor num, 0xff = get any events */
static int    timeout     = 120; /* 120 seconds default timeout */
static int    wait_interval = 1; /* 1 second between calls */
static FILE  *fdout = NULL;
static char  *run_script = NULL;
static uchar  ipmi_maj    = 0;
static uchar  ipmi_min    = 0;
static HandleType imb_handle = 0;
static int drvtype = 0;  /* driver_type from ipmicmd.h: 1=Intel_imb, 3=MV_OpenIPMI */
static int vend_id = 0;
static int prod_id = 0;
static char fselevts = 0;
static char fmsgevts = 0;
static ushort sel_recid = 0;
static uint   sel_time  = 0;
static uchar sms_sa = 0x81;
static uchar *sdrs = NULL;
#define LAST_REC  0xFFFF
#ifdef WIN32
#define IDXFILE  "ipmi_evt.idx"
static char idxfile[80]  = IDXFILE;
static char idxfile2[80] = "c:\\ipmi_evt.idx";
static char outfile[80] = "c:\\ipmiutil_evt.log";
#define   SHUTDOWN_CMD  "shutdown -s -d p:01:01 -t 10"
#define   REBOOT_CMD    "shutdown -r -d p:01:01 -t 10"
#else
static char idxfile[80] = "/var/lib/ipmiutil/evt.idx";
static char idxfile2[80] = "/usr/share/ipmiutil/evt.idx";
static char outfile[80] = "/var/log/ipmiutil_evt.log";
#define   SHUTDOWN_CMD  "init 0"   // or shutdown now
#define   REBOOT_CMD    "init 6"
#endif
#ifdef METACOMMAND
extern FILE *fpdbg;  /*from ipmicmd.c*/
extern FILE *fperr;  /*from ipmicmd.c*/
#endif
/* prototypes */
static void iclose(void); 
static void ievt_siginit(void);
static void ievt_cleanup(void);

#define METHOD_UNKNOWN    0
#define METHOD_SEL_EVTS   1
#define METHOD_MSG_GET    2
#define METHOD_MSG_MV     3
#define METHOD_ASYNC_MV   4
#define METHOD_ASYNC_IMB  5
static char *methodstr[6] = {
  "unknown",
  "SEL_events",
  "GetMessage",
  "GetMessage_mv",
  "Async_mv",
  "Async_imb" };

static int do_wait(int nsec)
{
   int rv = 0;
   if (nsec > 0) os_usleep(nsec,0);  /*declared in ipmicmd.h*/
   return(rv);
}

static int get_event_receiver(uchar *sa, uchar *lun)
{
	uchar rdata[30];
	int rlen;
	uchar ccode;
	int ret;

	rlen = 2;
#if 0
        ret = ipmi_cmdraw( 0x01,NETFN_SEVT,BMC_SA,PUBLIC_BUS,BMC_LUN,
			idata,0, rdata,&rlen,&ccode, fdebug);
#endif
        ret = ipmi_cmd(GET_EVENT_RECEIVER,NULL,0, rdata,&rlen,&ccode, 0);
	if (ret == 0 && ccode != 0) ret = ccode;
	if (ret == 0) {
	   *sa  = rdata[0];
	   *lun = rdata[1];
	}
	return(ret);
}  

static int get_msg_flags(uchar *flags)
{
	uchar rdata[8];
	int rlen = 1;
	uchar ccode;
	int ret;
        ret = ipmi_cmdraw( 0x31,NETFN_APP,BMC_SA,PUBLIC_BUS,BMC_LUN,
			NULL,0, rdata,&rlen,&ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	if ((ret == 0) && (flags != NULL)) *flags = rdata[0];
	return(ret);
}

static int set_bmc_enables(uchar enab)
{
	uchar idata[8];
	uchar rdata[30];
	int rlen;
	uchar ccode;
	int ret;

	idata[0] = enab;
	rlen = 1;
        ret = ipmi_cmdraw( 0x2E,NETFN_APP,BMC_SA,PUBLIC_BUS,BMC_LUN,
			idata,1, rdata,&rlen,&ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}

static int get_bmc_enables(uchar *enab)
{
	uchar rdata[30];
	int rlen;
	uchar ccode;
	int ret;

	rlen = 1;
        ret = ipmi_cmdraw( 0x2F,NETFN_APP,BMC_SA,PUBLIC_BUS,BMC_LUN,
			NULL,0, rdata,&rlen,&ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;

	if (ret == 0) *enab  = rdata[0];
	return(ret);
}

/* 
 * msgout
 * wrapper for printf() to include fflush
 */
void msgout(char *pattn, ...)
{
    va_list arglist;

    if (fdout == NULL) return;
    va_start( arglist, pattn );
    vfprintf( fdout, pattn, arglist );
    va_end( arglist );
    fflush( fdout );
}

#ifdef DO_ASYNC
/* The DO_ASYNC flag enables the IMB Async Message method via get_imb_event.
 * This requires the Intel IMB driver, and is used only for remote shutdown
 * and software ID events.  */

/* The LANDesk library has the same function names as the imbapi.c */
#ifdef LINK_LANDESK
#define StartAsyncMesgPoll ia_StartAsyncMesgPoll
#define SendAsyncImbpRequest ia_SendAsyncImbpRequest
#define GetAsyncImbpMessage ia_GetAsyncImbpMessage
#define GetAsyncImbpMessage_Ex ia_GetAsyncImbpMessage_Ex
#define IsAsyncMessageAvailable ia_IsAsyncMessageAvailable
#define RegisterForImbAsyncMessageNotification ia_RegisterForImbAsyncMessageNotification
#define UnRegisterForImbAsyncMessageNotification ia_UnRegisterForImbAsyncMessageNotification
#define SendTimedLanMessageResponse_Ex ia_SendTimedLanMessageResponse_Ex
#define SendTimedEmpMessageResponse_Ex ia_SendTimedEmpMessageResponse_Ex
#endif  /*endif LINK_LANDESK*/

typedef struct {
        uchar rsSa;
        uchar nfLn;
        uchar cSum1;
        uchar rqSa;
        uchar seqLn;
        uchar cmd;
        uchar data[1];
} AsyImbPacket;

#ifdef THREADS_OK
   char message[32];
#ifdef WIN32
   HANDLE    threadid = NULL;
#else
   pthread_t threadid = 0;
#endif
#endif

#ifdef WIN32
#define ThreadRType DWORD
ThreadRType WINAPI pollThread( LPVOID p)
#else
#define ThreadRType void *
ThreadRType pollThread(void *p)
#endif
{
   int i;
   int ret, limit;
#ifdef THREADS_OK
   limit = 0;
#else
   limit = 30;
#endif
   for (i = 0; (limit == 0) || (i < limit); i++)
   {
      ret = StartAsyncMesgPoll();
      if (fdebug && i < 5) 
          msgout("StartAsyncMesgPoll [%d] ret = %d\n",i,ret);
      // os_usleep(0,5000);  /* poll interval 5 msec */
      os_usleep(1,0);  /* poll interval 1 sec */
   }
   return((ThreadRType)0);
}

static int GetBmcLanChannel(uchar *chan)
{
    int ret = 0;
    int j;
    int rlen;
    uchar iData[2];
    uchar rData[10];
    uchar cc;
    uchar mtype;
    uchar chn = 1;

    if (vend_id == VENDOR_INTEL) {
        if (prod_id == 0x000C || prod_id == 0x001B) {
           *chan = 7;
           return(ret);
        }
    }
    for (j = 1; j < 12; j++) {
        rlen = sizeof(rData);
        iData[0] = (uchar)j; /*channel #*/
        memset(rData,0,9); /*initialize recv data*/
        ret = ipmi_cmd(GET_CHANNEL_INFO, iData, 1, rData, &rlen, &cc, fdebug);
        if (ret == 0xcc || cc == 0xcc) /* special case for ipmi_lan */
           continue;
        if (ret != 0) {
           if (fdebug) printf("get_chan_info rc = %x\n",ret);
           break;
        }
        mtype = rData[1];  /* channel medium type */
        if (mtype == 4) {  /* 802.3 LAN type*/
           if (fdebug) printf("chan[%d] = lan\n",j);
           chn = (uchar)j;
           break;
        }
    }
    *chan = chn;
    return(ret);
}

int SoftwareIdResponse(uchar *buf, int blen, uchar hnd, uchar chan)
{
   int rv = 0;
   uchar resp[12] = {0,0xa6,0,0,0,1,0,0x00,0x01,0x57,0x00,0x01};
#ifdef WIN32
   /* check OS version & arch (32/64) */
#else
   struct utsname uts;
   rv = uname(&uts);
   // uts.release=`uname -r`  uts.machine=x86_64,ia64,i586,i386 
   // kver <= 24 bytes, mach/arch <= 6 bytes
#endif
   
#ifdef USE_LANMSG
   rv = SendTimedLanMessageResponse_Ex( (ImbPacket *)buf, (char *)(&resp), 12, 
				IMBPTIMEOUT, hnd, chan);
#else
   rv = SendTimedEmpMessageResponse_Ex((ImbPacket *)buf, (char *)(&resp), 12, 
				IMBPTIMEOUT, hnd,chan);
#endif
   if (fdebug) msgout("SoftwareIdResponse(%d) ret = %d\n",chan,rv);
   return(rv);
}


int SmsOsResponse(uchar *buf, int blen, uchar func, uchar hnd, uchar chan)
{
   int rv = 0;
   char cc = 0;
   if (frunscript) {
	   write_syslog("igetevent -a running script\n");
           rv = system(run_script);
           if (fdebug) msgout("run(%s) ret = %d\n",run_script,rv);
   }
   switch(func)   /*data byte has function*/
   {
      case 0x01:    /*shutdown & power down*/
        rv = SendTimedEmpMessageResponse_Ex((ImbPacket *)buf, &cc,1, 
			IMBPTIMEOUT, hnd,chan);
        if (fdebug) msgout("OsResponse(%d) ret = %d\n",chan,rv);
	if (!fAsyncNOP) {
	   write_syslog("igetevent -a initiating OS shutdown\n");
           rv = system(SHUTDOWN_CMD);
           if (fdebug) msgout("shutdown ret = %d\n",rv);
	}
	break;
      case 0x02:    /*shutdown & reset*/
        rv = SendTimedEmpMessageResponse_Ex((ImbPacket *)buf, &cc,1, 
			IMBPTIMEOUT, hnd,chan);
        if (fdebug) msgout("OsResponse(%d) ret = %d\n",chan,rv);
	if (!fAsyncNOP) {
	   write_syslog("igetevent -a initiating OS reboot\n");
           rv = system(REBOOT_CMD);
           if (fdebug) msgout("reboot ret = %d\n",rv);
	}
	break;
      default:     
        if (fdebug) msgout("igetevent -a unknown function %d\n",func);
	rv = 1;
	break;
   }
   return(rv);
}

/*
 * get_imb_event
 * This only gets certain IMB events, like
 * OS requests (e.g. shutdown), and get_software_id 
 */
static int get_imb_event(uchar etype, int timeout, uchar *evt)
{
   	int ret = -1;
	int i;
        int done = 0;
	ulong mlen;
	uchar buffer[512];
	uchar sendbuf[18];
	static uint asyseqnum = 0;
        uchar chan;
        uchar sessHandle = 0;
        uchar privilege = 0;
	uchar cmd, func;
	// uchar *pbuf;

        ret = GetBmcLanChannel(&chan);

	/* clean out pre-existing async messages */
	while(1) {
	   mlen = sizeof(buffer);
	   if (GetAsyncImbpMessage((ImbPacket *)buffer,&mlen, IMBPTIMEOUT, 
				&asyseqnum, IPMB_CHANNEL) != 0) 
		break;
           if (fdebug) msgout("cleaned out an IPMB message seq=%d\n",asyseqnum);
	}
	while(1) {
	   mlen = sizeof(buffer);
	   if (GetAsyncImbpMessage((ImbPacket *)buffer,&mlen, IMBPTIMEOUT, 
				&asyseqnum, LAN_CHANNEL) != 0) 
		break;
           if (fdebug) msgout("cleaned out a LAN message seq=%d\n",asyseqnum);
	}
	ret = RegisterForImbAsyncMessageNotification(&imb_handle);
	if (fdebug) 
            msgout("RegisterForImbAsync ret=%d, handle=%x\n",ret,imb_handle);
	if (ret != 0) { 
		msgout("RegisterAsync error %d\n",ret);
		return(ret);
	}

	for (i = 0; (timeout == 0) || (i < timeout); i++)
	{  /*get one imb event*/
             if (fdebug) msgout("IsAsyncMessageAvailable ...\n");
	     if (IsAsyncMessageAvailable(imb_handle) == 0) 
             {
                if (fdebug) msgout("Async Message is Available\n");
		mlen = sizeof(buffer);
		ret = GetAsyncImbpMessage_Ex ((ImbPacket *)buffer, &mlen,
                                IMBPTIMEOUT, &asyseqnum, ANY_CHANNEL,
                                 &sessHandle, &privilege);
		/* Hack: buffer contains an extra byte to return channel */
                if (fdebug) 
		   msgout("GetAsync(%d,%d) ret = %d, newchan=%x\n",
			asyseqnum,chan,ret,buffer[mlen]);
		if (ret == 0) {
		   /* get the async message command */
                   if (fdebug) dump_buf("async msg",buffer,mlen+1,0);
		   chan = buffer[mlen];
                   if (mlen > 16) mlen = 16;
		   if (buffer[0] == sms_sa) { 
			memcpy(&sendbuf[0],buffer,mlen);
		   } else {  /* handle shorter format for some BMCs */
			sendbuf[0] = sms_sa;
			memcpy(&sendbuf[1],buffer,mlen);
		   }
		   cmd = sendbuf[5]; func = sendbuf[6]; 
                   msgout("got async msg: cmd=%02x len=%d\n",cmd,mlen);
                   memcpy(evt,sendbuf,mlen);

		   switch(cmd) {
		     case CMD_GET_SOFTWARE_ID:  /*Get Software ID*/
			ret = SoftwareIdResponse(sendbuf,mlen,sessHandle,chan);
			break;
		     case CMD_SMS_OS_REQUEST:  /*SMS OS Request*/
			ret = SmsOsResponse(sendbuf,mlen,func,sessHandle,chan);
			if (ret == 0) done = 1;
			break;
		     default:
		        ret = LAN_ERR_INVPARAM;
		        msgout("SmsOS cmd %02x unknown, ret = %d\n",cmd,ret);
		   }
		   if (fdebug)
		        msgout("async msg cmd=%02x ret = %d\n",cmd,ret);
		   if (done == 1) { 
			if (func == 0x01) msgout("shutting down\n");
			else msgout("rebooting\n");
			ret = 0x81; 
			break; 
		   }
	        } 
	     }   /*endif have an event*/
	     else ret = 0x80;  /* no event yet */
	}   /*loop for one event*/
        if (fdebug) msgout("Unregister for imb events\n");
	UnRegisterForImbAsyncMessageNotification (imb_handle,0);
	return(ret);
}
#endif
   /*endif DO_ASYNC*/

#ifdef DO_MVL
/* Linux, enable MV OpenIPMI interface */
extern int register_async_mv(uchar cmd, uchar netfn);   /*see ipmimv.c*/
extern int unregister_async_mv(uchar cmd, uchar netfn); /*see ipmimv.c*/
extern int getevent_mv(uchar *evt_data, int *evt_len, uchar *cc, int t);
extern int ipmi_rsp_mv(uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
			uchar *pdata, int sdata, char fdebugcmd);

static int send_mv_asy_resp(uchar *evt)
{
   uchar cmd, sa, bus, cc;
   uchar data0[12] = {0,0xa6,0,0,0,1,0,0x00,0x01,0x57,0x00,0x01}; 
   uchar data1[1] = { 0x00 };
   uchar rdata[80];
   int sdata, rlen, i;
   uchar *pdata; 
   int rv = -1;

   cmd = evt[2];
   sa = sms_sa; /* sa = SMS_SA (0x81) */
   bus = 0x01;  /* usu lan_ch == 1 */
   switch(cmd) {
      case CMD_GET_SOFTWARE_ID:  /*software id*/
          pdata = &data0[0];
	  sdata = sizeof(data0);
	  break;
      case CMD_SMS_OS_REQUEST:  /*restart*/
          pdata = &data1[0];
	  sdata = sizeof(data1);
	  break;
      default: rv = LAN_ERR_INVPARAM; return (rv);
	  break;
   }
   rv = ipmi_rsp_mv(cmd, (NETFN_APP | 0x01), sa, bus, BMC_LUN,
			pdata,sdata, fdebug);
   if (rv == 0) {
     for (i = 0; i < 5; i++) 
     {
       rlen = sizeof(rdata);
       rv = getevent_mv(rdata,&rlen,&cc,1);
       if (fdebug) msgout("send_mv_asy_resp: rv=%d cc=%x\n",rv,cc);
       if (rv == 0 && cc == 0) {
	    if (fdebug) msgout("got rsp ccode: type=%02x len=%d cc=%x\n",
				rdata[0],rlen,rdata[3]);
            if (rlen > 0) rv = rdata[3]; /*cc*/
	    break;
       }
       os_usleep(0,5000);  /*wait 5 ms*/
     }
   }
   return (rv);
}

static int get_mv_asy_event(uchar cmd, int timeout, uchar *evt)
{
   int ret = -1;
   int rv = -1;
   uchar cc = 0;
   uchar rdata[120];
   int rlen, i;

   rv = register_async_mv(cmd,NETFN_APP); /*reserved,GetSoftwareID*/
   if (rv != 0) return(rv);

   for (i = 0; (timeout == 0) || (i < timeout); i++)
   {  /*get one async event*/
       rv = getevent_mv(rdata,&rlen,&cc,timeout);
       if (fdebug) 
	    msgout("get_mv_asy_event: i=%d cmd=%x rv=%d cc=%x\n",i,cmd,rv,cc);
       if (rv == 0 && cc == 0) {
            msgout("got async msg: type=%02x cmd=%x len=%d\n",
			rdata[0],rdata[2],rlen);
            if (fdebug) dump_buf("async msg",rdata,rlen,0);
	    /* check recv_type == 3 (IPMI_CMD_RECV_TYPE) */
	    if (rdata[0] == 3 && rdata[2] == cmd) {
		if (rlen > 16) rlen = 16;  
		memcpy(evt,rdata,rlen);
		break;
	    } else { /*else msg, but no match*/
		rv = ERR_BAD_PARAM;
		break;
	    }
       }
       else do_wait(wait_interval);  /*wait 1 sec*/ 
   }
   ret = unregister_async_mv(cmd,NETFN_APP); /*reserved,GetSoftwareID*/
   return(rv);
}
#endif

static int get_evt_method(char *evtmethod)
{
   int method = METHOD_UNKNOWN;
   if (fAsync) { 
      if (drvtype == DRV_MV) method = METHOD_ASYNC_MV;
      else /* if (drvtype == DRV_IMB) */ method = METHOD_ASYNC_IMB;
   } else if (fselevts) method = METHOD_SEL_EVTS;
   else {  /*fmsgevts*/
      if (drvtype == DRV_MV) method = METHOD_MSG_MV;
      else method = METHOD_MSG_GET;
   }
   if (evtmethod != NULL) 
      strcpy(evtmethod,methodstr[method]); 
   return(method);
}

static int get_sel_entry(ushort recid, ushort *nextid, uchar *rec)
{
   uchar ibuf[6];
   uchar rbuf[32];
   int rlen;
   ushort xid, id = 0;
   uchar cc;
   int rv; 

   ibuf[0] = 0;
   ibuf[1] = 0;
   ibuf[2] = (recid & 0x00ff);
   ibuf[3] = (recid & 0xff00) >> 8;
   ibuf[4] = 0;
   ibuf[5] = 0xFF;  /*get entire record*/
   rlen = sizeof(rbuf);
   rv = ipmi_cmd(GET_SEL_ENTRY, ibuf, 6, rbuf, &rlen, &cc, fdebug);
   if (rv == 0) {
       if (cc != 0) rv = cc;
       else {  /*success*/
          xid = rbuf[0] + (rbuf[1] << 8);  /*next rec id*/
          memcpy(rec,&rbuf[2],16);
          *nextid = xid;
          id = rbuf[2] + (rbuf[3] << 8);  /*curr rec id*/
	  /* recid (requested) should match newid (received) */
          if (fdebug) {
            if ((recid != id) && (recid != LAST_REC) && (recid != 0)) {
              /* the OpenIPMI driver does this sometimes */
              msgout("get_sel MISMATCH: recid=%x newid=%x next=%x\n",
	   		      recid,id,xid);
              dump_buf("get_sel cmd",ibuf,6,0);
              dump_buf("get_sel rsp",rbuf,rlen,0);
            }
          }
       }
   }
   if (fdebug) msgout("get_sel(%x) rv=%d cc=%x id=%x next=%x\n",
			recid,rv,cc,id,*nextid);
   return(rv);
}

static int startevent_sel(ushort *precid, uint *ptime) 
{
    FILE *fd;
    uchar rec[24];
    uint t = 0;
    ushort r = 0;
    ushort r2 = 0;
    int rv = -1;

    fd = fopen(idxfile,"r");
    if (fd == NULL) fd = fopen(idxfile2,"r"); /*handle old location*/
    if (fdebug) msgout("start: idxfile=%s fd=%p\n",idxfile,fd);
    if (fd != NULL) {
        // Read the file, get savtime & savid
        rv = fscanf(fd,"%x %x",&t,(uint *)&r);
        fclose(fd);
        if (r == LAST_REC) r = 0;
	rv = 0; /*read it, success*/
    } else {  /* treat as first time */
        r = LAST_REC;
        rv = get_sel_entry(r,&r2,rec);
        if (rv == 0) {
            memcpy(&t,&rec[3],4);
            r = rec[0] + (rec[1] << 8); /*use current rec id*/
        } else r = 0;
	rv = 1;  /*first time*/
    }
    if (fdebug) msgout("start: recid=%x time=%x\n",r,t);
    *ptime  = t;
    *precid = r;
    return(rv);
}

static int syncevent_sel(ushort recid, uint itime)
{
    FILE *fd;
    int rv;
    // Rewrite the saved time & record id
    if (fdebug) msgout("sync: recid=%x time=%x\n",recid,itime);
    fd = fopen(idxfile,"w");
    if (fd == NULL) {
	msgout("syncevent: cannot open %s for writing\n",idxfile);
	rv = -1;
    } else {
        fprintf(fd,"%x %x\n",itime,recid);
        fclose(fd);
	rv = 0;
    }
    return(rv);
}

int getevent_sel(uchar *rdata, int *rlen, uchar *ccode)
{
    uchar rec[24];
    int rv = 0;
    ushort newid;
    ushort nextid;
    ushort recid;
    
    /* get current last record */
    recid = sel_recid;
    rv = get_sel_entry(recid,&nextid,rec);
    if (rv == 0xCB && recid == 0) {  /* SEL is empty */
        *ccode = (uchar)rv;  /* save the real ccode */
        rv = 0x80;    /* this is ok, just keep waiting  */
    }
    if (rv == 0) {
       if (fdebug) msgout("sel ok, id=%x next=%x\n",recid,nextid);
       if ((nextid == LAST_REC) || (recid == nextid)) { 
           *ccode = 0x80;  /*nothing new*/
       } else {
         recid = nextid;  /* else get new one */
         rv = get_sel_entry(recid,&nextid,rec);
         if (rv == 0) {  /* new event */ 
            newid = rec[0] + (rec[1] << 8);
            if (drvtype == DRV_MV && recid != newid) {  
               /* handle MV driver bug, try to get next one. */
               if (fdebug) msgout("%s bug, record mismatch\n",
				   show_driver_type(DRV_MV));
            }
            if (fdebug) msgout("recid=%x newid=%x next=%x\n",
	   		       recid,newid,nextid);
            memcpy(rdata,rec,16);
            *rlen = 16;
            *ccode = 0;
            sel_recid = recid;  /*or newid*/
            memcpy(&sel_time,&rec[3],4);
         }
       }
    }
    else {  /* Error reading last recid saved */
       if (fdebug) msgout("sel recid %x error, rv = %d\n",recid,rv);
       /* We want to set sel_recid = 0 here for some errors. */
       if (rv == 0xCB || rv == 0xCD) { /* empty, or wrong SDR id */
	  sel_recid = 0;
          *ccode = (uchar)rv;
          rv = 0x80; /* wait again */
       }
    }
    return(rv); 
}

static int get_event(uchar etype, uchar snum, int timeout, 
			uchar *evt, uchar *stype)
{
   	int ret = 0;
   	uchar rdata[64];
   	int rlen;
	uchar ccode;
	int fretry;
	int i;

	for (i = 0; (timeout == 0) || (i < timeout); i++)
	{
	   rlen = sizeof(rdata);
	   fretry = 0;  ccode = 0;
           if (fselevts) {
             ret = getevent_sel(rdata,&rlen,&ccode);
           } else 
#ifdef DO_MVL
           if (drvtype == DRV_MV) { /* if MV OpenIPMI driver (Linux only) */
	     if (timeout == 0) wait_interval = 0;
             /* use special MV API instead (see ipmimv.c) */
             ret = getevent_mv(rdata,&rlen,&ccode,timeout);
           } else
#endif
             ret = ipmi_cmd(READ_EVENT_MSGBUF,NULL,0,rdata,&rlen,&ccode,fdebug);
	   /* now we have an event from one of the above methods*/

           /* IPMI 1.5 spec, section 18.8 says cc 0x80 means 
            * "data not available (queue/buffer empty)" */
           if (ret == 0 && ccode != 0) { ret = ccode; }
           if (ret == 0x80) { 
		fretry = 1; 
		do_wait(wait_interval); /*wait 1 sec*/ 
           } else {
        	if (ret == 0) {
		    char ismatch = 0;
	    	    /* parse event types for a specified type */
		    /* etype param == 0xff means get any event */
		    /* rdata[10] is sensor_type, rdata[11] is sensor_num */
            	    if ((etype == 0xff) || (etype == rdata[10])) ismatch++;
            	    if ((snum == 0xff) || (snum == rdata[11])) ismatch++;
		    if (ismatch == 2) {
			/*event sensor type matches*/
	       	        memcpy(evt,rdata,rlen);
			*stype = rdata[10];  /* return sensor type */
            	    } else {       /* keep looking */
			do_wait(wait_interval);
			continue; 
		    }
        	}
		/* if here, either got one, or need to return error */
		break;  
	    }
	}  /*end for loop*/
	return(ret);
}

int send_nmi(void)
{
	uchar idata[8];
	uchar rdata[30];
	int rlen;
	uchar ccode;
	int ret;

        idata[0] = 4;  /* do NMI */
        rlen = sizeof(rdata);
        ret = ipmi_cmdraw( CHASSIS_CTL, NETFN_CHAS, BMC_SA,PUBLIC_BUS,BMC_LUN,
			idata,1, rdata,&rlen,&ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}

void show_event(uchar *evt,char *obuf, int sz)
{
   int i;
   char sysbuf[250];
   /* obuf should be 132 chars or more */

   msgout("event data: ");
   for (i=0; i<16; i++) msgout("%02x ",evt[i]);
   msgout("\n");

   decode_sel_entry(evt,obuf,sz);
   msgout(obuf);  /*writes to outfile*/
   /* write the message to syslog also. */
   snprintf(sysbuf,sizeof(sysbuf),"%s: %s",progname,obuf);
   write_syslog(sysbuf);
}

static void ievt_cleanup(void)
{
   char obuf[48];
   if (fselevts) syncevent_sel(sel_recid,sel_time);
   snprintf(obuf,sizeof(obuf),"%s exiting.\n",progname);
   msgout(obuf);
   write_syslog(obuf);
   free_sdr_cache(sdrs);
   iclose(); 
   exit(EXIT_SUCCESS);
}

#if defined(WIN32) | defined(DOS)
/* no daemon code */
static void ievt_siginit(void) { return; }
#else
/* Linux daemon code */
#include <signal.h>
static void ievt_sighnd(int sig)
{
   ievt_cleanup();
   exit(EXIT_SUCCESS);
}

static void ievt_siginit(void);
static void ievt_siginit(void)
{
   struct sigaction sact;
 
   /* handle signals for cleanup */
   sact.sa_handler = ievt_sighnd;
   sact.sa_flags = 0;
   sigemptyset(&sact.sa_mask);
   sigaction(SIGINT, &sact, NULL);
   sigaction(SIGQUIT, &sact, NULL);
   sigaction(SIGTERM, &sact, NULL);
}

static int mkdaemon(int fchdir, int fclose);
static int mkdaemon(int fchdir, int fclose)
{
    int fdlimit = sysconf(_SC_OPEN_MAX); /*fdlimit usu = 1024.*/
    int fd = 0;
 

    fdlimit = fileno(stderr);  /*only close files up to stderr*/
    switch (fork()) {
        case 0:  break;
        case -1: return -1;
        default: _exit(0);          /* exit the original process */
    }
    if (setsid() < 0) return -1;    /* shouldn't fail */
    switch (fork()) {
        case 0:  break;
        case -1: return -1;
        default: _exit(0);          /* exit the original process */
    }
    if (fchdir) {
	chdir("/");
	/* umask(0022); * leave file creation mask at default (0022) & 0777 */
    }
    if (fclose) {
        /* Close stdin,stdout,stderr and replace them with /dev/null */
        for (fd = 0; fd < fdlimit; fd++) close(fd);
        open("/dev/null",O_RDWR);
        dup(0); dup(0);
    }
    return 0;
}
#endif

static void iclose()
{
   /* close out any IPMI handles or sessions */
#ifdef THREADS_OK
#ifdef WIN32
   if (threadid != NULL) CloseHandle(threadid);
#else
   /* thread close not needed in Linux */
#endif
#endif
   ipmi_close_();
   if (fbackground && fdout != NULL) fclose(fdout);
}

#ifdef METACOMMAND
int i_getevt(int argc, char **argv)
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
   int c, j;
   uchar devrec[16];
   uchar event[16];
   uchar sa, lun;
   uchar enables = 0;
   uchar fevmsgok = 0;
   uchar sensor_type = 0;
   uchar msg_flags = 0;
   FILE *fp;
   char outbuf[160];
   char tmpout[20];
   char *sdesc;

   fdout = stdout;
   msgout("%s ver %s\n", progname,progver);

   while ( (c = getopt(argc,argv,"abce:lmn:op:r:st:uvT:V:J:YEF:P:N:R:U:Z:x?")) != EOF ) 
      switch(c) {
          case 'a': fAsync = 1;    /* imb async message method */   
		    /* chenge the output log filename */
		    sdesc = strstr(outfile,"evt.log");
		    if (sdesc != NULL) strcpy(sdesc,"asy.log");
		    break;
          case 'b': fbackground = 1; break; /* background */
          case 'c': fcanonical = 1; break; /* canonical */
          case 'e':   /* event sensor type */
		if (strncmp(optarg,"0x",2) == 0) 
		     evt_stype = htoi(&optarg[2]);
		else evt_stype = atob(optarg);
		break;
          case 'l': fAsyncNOP = 1; break;   /* do not reset (for testing)*/
          case 'm': fmsgevts = 1; break;   /* use local getmessage method */
          case 'n':   /* event sensor num, always hex */
		if (strncmp(optarg,"0x",2) == 0) 
		     evt_snum = htoi(&optarg[2]);
		else evt_snum = htoi(&optarg[0]);
		break;
          case 'o': frunonce = 1;   break;  /* only run once for first event */
          case 'r':   /* run script (or binary) on an event */
		run_script = optarg;
		fp = fopen(run_script,"r");
		if (fp == NULL) {
		   printf("cannot open %s\n",run_script);
		   ret = ERR_FILE_OPEN;
		   goto do_exit;
		} else {
		   fclose(fp);
		   frunscript = 1;
		}
		break;
          case 's': fselevts = 1;   break;  /* use SEL event method*/
          case 't': timeout = atoi(optarg); fsettime = 1; break; /*timeout*/
          case 'u': futc = 1; break; 
          case 'x': fdebug = 1;     break;  /* debug messages */
          case 'v': fdebug = 3;     break;  /* verbose debug with lan */
          case 'p': 
          case 'N':    /* nodename */
          case 'U':    /* remote username */
          case 'P':    /* remote password */
          case 'R':    /* remote password */
          case 'E':    /* get password from IPMI_PASSWORD environment var */
          case 'F':    /* force driver type */
          case 'T':    /* auth type */
          case 'J':    /* cipher suite */ 
          case 'V':    /* priv level */
          case 'Y':    /* prompt for remote password */
          case 'Z':    /* set local MC address */
                parse_lan_options(c,optarg,fdebug);
                break;
	  default:
                printf("Usage: %s [-abenorsux -t sec -NPRUEFTVY]\n", progname);
                printf(" where -a     use Async method\n");
                printf("       -b     run in Background\n");
                printf("       -c     use Canonical/delimited event format\n");
                printf("       -e T   wait for specific event sensor type T\n");
                printf("       -n N   wait for specific event sensor num  N\n");
                printf("       -o     run Once for the first event\n");
                printf("       -r F   Run file F when an event occurs\n");
                printf("       -s     use SEL event method\n");
                printf("       -t N   set timeout to N seconds\n"); 
                printf("       -u     use raw UTC time\n");
                printf("       -x     show eXtra debug messages\n");
		print_lan_opt_usage(1);
		ret = ERR_USAGE;
		goto do_exit;
      }

   fipmilan = is_remote();
   ret = ipmi_getdeviceid(devrec,16,fdebug);
   if (ret != 0) {
      goto do_exit;
   } else {
      ipmi_maj = devrec[4] & 0x0f;
      ipmi_min = devrec[4] >> 4;
      show_devid( devrec[2],  devrec[3], ipmi_maj, ipmi_min);
      vend_id = devrec[6] + (devrec[7] << 8) + (devrec[8] << 16);
      prod_id = devrec[9] + (devrec[10] << 8);
      if (vend_id == VENDOR_INTEL) {
         if (prod_id == 0x003E) /* NSN2U or CG2100 Urbanna */
            sms_sa = 0x41;
      }
   }

   /* get event receiver */
   ret = get_event_receiver(&sa ,&lun);
   if (ret != 0)
        msgout("event receiver error %d\n",ret);
   else msgout("event receiver sa = %02x lun = %02x\n",sa,lun);

   ret = get_bmc_enables(&enables);
   if (ret != 0) msgout("bmc enables error %d\n",ret);
   else {
        msgout("bmc enables = %02x\n",enables);
	if ((enables & 0x02) == 2) fevmsgok = 1;
	else fevmsgok = 0;
   }
   if (fevmsgok == 0 && !fipmilan) { 
      msgout("Event Message Buffers not enabled.\n");
      enables |= 0x0f;  /* 0x08=SEL, 0x02=EvtMsgBuf, rest is gravy */
      ret = set_bmc_enables(enables);
      if (ret != 0) {
	 msgout("set_bmc_enables error 0x%x\n",ret);
      }
      else msgout("set_bmc_enables success\n");
   }
   if (fipmilan && !fselevts) {
      msgout("Only the SEL method (-s) is supported over IPMI LAN\n");
      ret = LAN_ERR_NOTSUPPORT;
      goto do_exit;
   }
   ret = get_msg_flags(&msg_flags);
   msgout("igetevent reading sensors ...\n");
   write_syslog("igetevent reading sensors ...\n");
   ret = get_sdr_cache(&sdrs);
   // if (!fipmilan) set_sel_opts(1,0, NULL,fdebug);
   if (fdebug) msgout("get_sdr_cache ret = %d\n",ret);
   if (ret == 0) set_sel_opts(1, fcanonical, sdrs,fdebug,futc);
   else ret = 0;  /*if error, keep going anyway*/

   if (fselevts) {
#ifdef WIN32
       {  /*resolve path of idxfile*/
	  char *ipath;
	  ipath = getenv("ipmiutildir");  /*ipmiutil directory path*/
	  if (ipath != NULL) {
	     if (strlen(ipath)+12 < sizeof(idxfile)) {
	        sprintf(idxfile,"%s\\%s",ipath,IDXFILE);
	     }
	  }
       }
#endif

       if (fipmilan) {
	  char *node;
	  node = get_nodename();
          strcat(idxfile,"-");
          strcat(idxfile,node);
          strcat(idxfile2,"-");
          strcat(idxfile2,node);
          strcat(outfile,"-");
          strcat(outfile,node);
       }
       ret = startevent_sel(&sel_recid,&sel_time);
       ret = 0;  /*ignore any earlier errors, keep going*/
   }

   drvtype = get_driver_type();
   if (fdebug) msgout("driver_type = %d (%s)\n",
			drvtype,show_driver_type(drvtype));

   if (evt_stype == 0xFF) sdesc = "any event";
   else sdesc = get_sensor_type_desc(evt_stype);
   if (evt_snum == 0xFF) tmpout[0] = 0;
   else sprintf(tmpout,"with snum %02x",evt_snum);
   if (evt_stype != 0xFF || evt_snum != 0xFF)
      msgout("Look for event sensor type %02x (%s) %s\n", evt_stype,sdesc,tmpout);

#ifdef TEST_SEL
   /* This is used to verify that the interface returns valid next ids,
    * and that the get_sel_entry is ok. */
   {
      int i;
      ushort r, r1, r2;
      uchar rec[40];
      int rv;
      r = 0;
      for (i = 0; i < 4; i++) 
      {
          rv = get_sel_entry(r,&r2,rec);
          if (rv == 0) {
             r1 = rec[0] + (rec[1] << 8); /*get current rec id*/
             if (fdebug) msgout("get_sel: r=%x r1=%x rnext=%x\n",r,r1,r2);
	     show_event(&rec[0],outbuf,sizeof(outbuf));
             r = r2;
          } else break;
      }
   }
#endif
   if (drvtype == DRV_IMB) fAsyncOK = 1;
   else if (drvtype == DRV_MV) {
	if (fsettime == 0) timeout = 0; /*DRV_MV default to infinite*/
	fAsyncOK = 1;
   } else fAsyncOK = 0;

   if (fAsync && (!fAsyncOK)) {
      msgout("Cannot open %s or %s driver, required for -a\n",
		show_driver_type(DRV_IMB),show_driver_type(DRV_MV));
      ret = ERR_NO_DRV;
      goto do_exit;
   }

   if (fbackground) { /* convert to a daemon if background */
#ifdef WIN32
      msgout("Background not implemented for Windows\n");
      ret = LAN_ERR_NOTSUPPORT;
      goto do_exit;
#elif defined(DOS)
      msgout("Background not implemented for DOS\n");
      ret = LAN_ERR_NOTSUPPORT;
      goto do_exit;
#else
      /* make sure we can open the log file before doing mkdaemon */
      fdout = fopen(outfile,"a");
      if (fdout == NULL)   
        printf("%s: Cannot open %s\n", progname,outfile);
      else {
	pid_t p;
	fclose(fdout);

        ret = mkdaemon(1,1);
        if (ret != 0) {
           msgout("%s: Cannot become daemon, ret = %d\n", progname,ret);
	   goto do_exit;
        }
        /* open a log file for messages, set fdout */
        fdout = fopen(outfile,"a");
#ifdef METACOMMAND
        /* make sure driver debug also goes to log */
        fpdbg = fdout;  
        fperr = fdout; 
#endif
        p = getpid();
        msgout("PID %d: %s ver %s\n",p,progname,progver); /*log start message*/
      }
#endif
   }
   ievt_siginit();

   if (fAsync && (fAsyncOK))   /*use imb/mv async messages*/
   {
     msgout("Wait for an async event\n");  /* no timeout */
     if (drvtype == DRV_IMB) 
     {
#ifdef DO_ASYNC
#ifdef THREADS_OK
#ifdef WIN32
      /* Windows threads */
      threadid = CreateThread(NULL, 0, &pollThread, NULL, 0, NULL);
      if (threadid == NULL) pollThread(NULL);
#else
      /* Linux threads */
      ret = pthread_create( &threadid, NULL, pollThread, (void*) message);
      // if (ret == 0) pthread_join( threadid, NULL);
      // if (ret == 0) pthread_detach( threadid);
#endif
      if (fdebug) msgout("pollThread create ret=%d handle=%x\n",ret,threadid);
#else
      /* no threads */
      pollThread(NULL);
#endif
      while (ret == 0) 
      {            /*wait for imb message events*/
        msgout("Waiting %d seconds for an async event ...\n",timeout);
           ret = get_imb_event(0xff,timeout,event);
	   if (ret == 0x81) {  /*ok, shutting down OS*/
	       ret = 0; 
	       break; 
           }
        if (frunonce) break;
        if (timeout == 0 && ret == 0x80) {  /*0x80 = no data yet */
            if (fdebug) msgout("get_event timeout, no event yet.\n");
            do_wait(wait_interval);
            ret = 0;  /*ok, keep going*/
        }
      }
#endif
     }  /*endif DRV_IMB*/
#ifdef DO_MVL
     else {  /*DRV_MV*/
	int stage;
	stage = 0;
	while (stage < 3) 
	{            /*wait for mv message events*/
          if (fdebug) msgout("Waiting for async_mv event, stage %d\n",stage);
	  if (stage == 0) {
            ret = get_mv_asy_event(CMD_GET_SOFTWARE_ID,timeout,event);
	    if (fdebug) msgout("got SmsOS GetSWID event ret = %d\n",ret);
            /* send a reply */
	    if (ret == 0) ret = send_mv_asy_resp(event);
	    if (ret == 0) stage = 1; /* got the SoftwareID request/response */
	    else stage = 0;
	  } 
          if (stage == 1) {  
	   /* get the reset command */
	   memset(event,0,sizeof(event));
           ret = get_mv_asy_event(CMD_SMS_OS_REQUEST,timeout,event);
	   if (fdebug) msgout("got SmsOS GetSmsOS event ret = %d\n",ret);
	   if (ret == 0) ret = send_mv_asy_resp(event);
	   if (ret == 0) stage = 2;
	   else stage = 0;
          }
          if (stage == 2) {  /* got the SmsOs request in event */ 
	    uchar cmd, func;
	    cmd = event[2];
	    func = event[3];
	    if (cmd != CMD_SMS_OS_REQUEST) {  /*cmd*/
		ret = LAN_ERR_INVPARAM;
		if (fdebug) msgout("SmsOS cmd %x ret = %d\n",cmd,ret);
	    } else {
		if (frunscript) {
		   write_syslog("igetevent -a running script\n");
		   ret = system(run_script);
		   if (fdebug) msgout("run(%s) ret = %d\n",run_script,ret);
		}
		if (!fAsyncNOP) 
		switch(func) {  /*subfunction*/
		case 0x01:   /*shutdown & power down*/
		   write_syslog("igetevent -a OS shutdown\n");
		   ret = system(SHUTDOWN_CMD);
		   msgout("SmsOs shutdown, ret = %d\n",ret);
		   break;
		case 0x02:   /*shutdown & reboot*/
		   write_syslog("igetevent -a OS reboot\n");
		   ret = system(REBOOT_CMD);
		   msgout("SmsOs reboot, ret = %d\n",ret);
		   break;
		case 0x03:   /*send NMI locally*/
		   write_syslog("igetevent -a NMI\n");
		   ret = send_nmi();
		   msgout("SmsOs NMI, ret = %d\n",ret);
		default:
		   ret = LAN_ERR_INVPARAM;
		   msgout("SmsOS func %02x, ret = %d\n",func,ret);
		   break;
	        }
	    }
	    if (ret == 0) stage = 3; /*done, exit loop*/
	    else {
	        if (fdebug) msgout("SmsOS error = %d, start over\n",ret);
		stage = 0;  /*start over*/
	    }
	  } /*endif stage 2*/
	} /*end-while*/
     } /*end else DRV_MV*/
#endif

   } else { /*not Async, std IPMI events */
      if (fselevts) {
          msgout("Get IPMI SEL events after ID %04x\n",sel_recid);
      } else 
          msgout("Get IPMI events from %s driver\n",show_driver_type(drvtype));
      j = get_evt_method(tmpout);
      sprintf(outbuf,"igetevent waiting for events via method %d (%s)\n",
		j, tmpout);
      msgout(outbuf);
      write_syslog(outbuf);

      /* loop on events here, like a daemon would. */
      while (ret == 0) 
      {            /*wait for bmc message events*/
         msgout("Waiting %d seconds for an event ...\n",timeout);
         /* note: could also get message flags here */
         ret = get_event(evt_stype,evt_snum,timeout,event,&sensor_type);
         if (fdebug) msgout("get_event ret = %d\n",ret);
         if (ret == 0) { /* got an event successfully */
            msgout("got event id %04x, sensor_type = %02x\n",
			sel_recid, sensor_type);
	    show_event(event,outbuf,sizeof(outbuf));
            if (fselevts) syncevent_sel(sel_recid,sel_time);
	    if (frunscript) {  /*run some script for each event*/
		char run_cmd[256];
		sprintf(run_cmd,"%s \"%s\"\n",run_script,outbuf);
		j = system(run_cmd);
		msgout("run(%s $1), ret = %d\n",run_script,j);
		ret = j;  /*if that failed, exit loop*/
	    }
         } else {
            if (ret == 0x80) msgout("get_event timeout\n");
            else msgout("get_event error: ret = 0x%x\n",ret);
         }
         if (frunonce) break;
         if (timeout == 0 && ret == 0x80) {  /*0x80 = no data yet */
            if (fdebug) msgout("get_event timeout, no data yet.\n");
            do_wait(wait_interval);
            ret = 0;  /*ok, keep going*/
         }
      }  /*end while loop*/
   }
   
do_exit:
   ievt_cleanup();
   if (ret == 0x80) ret = 0;
   // show_outcome(progname,ret);  /*inert if background*/
   return(ret);
}  /* end main()*/

/* end getevent.c */
