/*M*
//  PVCS:
//      $Workfile:   ipmimv.c  $
//      $Revision:   1.1  $
//      $Modtime:   08 Apr 2003 15:31:14  $
//      $Author:   arcress at users.sourceforge.net  $  
//
//  This implements support for the /dev/ipmi0 native interface from
//  the MontaVista OpenIPMI Linux driver.
//
//  To build this as a standalone test program, do this:
//  # gcc -DLINUX  -g -O2  -DTEST_BIN -o ipmimv ipmimv.c
// 
//  01/29/03 ARC - created, derived from ipmi_test.c and ipmitool_mv.c
//  04/08/03 ARC - don't watch stdin on select, since stdin may not be
//                 valid if invoked from cron, etc.
//  06/11/03 ARC - ignore EMSGSIZE errno for get_wdt command
//  05/05/04 ARC - only open/close device once per application,
//		   rely on each app calling ipmi_close, helps performance.
//  08/10/04 ARC - handle alternate device filenames for some 2.6 kernels
//  03/01/05 ARC - fix /dev/ipmi0 IPMB requests (to other than BMC_SA)
//  04/12/07 ARC - check for IPMI_ASYNC_EVENT_RECV_TYPE in ipmicmd_mv
 *M*/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2002-2005, Intel Corporation
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

#if defined(LINUX) || defined(BSD) || defined(MACOS) || defined(HPUX)
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#if defined(MACOS)
#include <sys/time.h>
#else
#include <sys/poll.h>
#endif
#ifdef SCO_UW
#include <sys/ioccom.h>
#endif

#ifdef TEST_BIN
#define ALONE  1
#endif

static void dbgmsg(char *pattn, ...);
#ifdef ALONE
#define uchar    unsigned char
#define ACCESS_OK        0
#define GET_SEL_ENTRY    0x43
static FILE *fperr = NULL;
static FILE *fpdbg = NULL;
void dump_buf(char *tag,uchar *pbuf,int sz, char fshowascii)
{
   if (pbuf == NULL || sz == 0) return;
   dbgmsg("%s sz=%d buf: %02x %02x %02x %02x\n",tag,sz,
		pbuf[0],pbuf[1], pbuf[2],pbuf[3]);
}
#else
#include "ipmicmd.h"
extern FILE *fperr;  /*defined in ipmicmd.c*/
extern FILE *fpdbg;  /*defined in ipmicmd.c*/
extern ipmi_cmd_t ipmi_cmds[NCMDS];
#endif

#define MV_BUFFER_SIZE   300    /*see IPMI_RSPBUF_SIZE also */
#define IPMI_MAX_ADDR_SIZE 32
#define IPMI_RESPONSE_RECV_TYPE         1
#define IPMI_ASYNC_EVENT_RECV_TYPE      2
#define IPMI_CMD_RECV_TYPE              3
#define IPMI_RESPONSE_RESPONSE_TYPE     4

#ifdef TV_PORT
/* use this to define timeval if it is a portability issue */
struct timeval {
        long int     tv_sec;         /* (time_t) seconds */
        long int     tv_usec;        /* (suseconds_t) microseconds */
};
#endif

int ipmi_timeout_mv = 10;   /* 10 seconds, was 5 sec */
#if defined(BSD7)
#pragma pack(1)
#endif

struct ipmi_addr
{
        int   adrtype;
        short channel;
        char  data[IPMI_MAX_ADDR_SIZE];
};

struct ipmi_msg
{
        uchar  netfn;
        uchar  cmd;
        ushort data_len;
        uchar  *data;
};

struct ipmi_req
{
        unsigned char *addr; /* Address to send the message to. */
        unsigned int  addr_len;
        long    msgid; /* The sequence number for the message.  */
        struct ipmi_msg msg;
};
 
struct ipmi_recv
{
        int     recv_type;  	/* Is this a command, response, etc. */
        unsigned char *addr;    /* Address the message was from */
	int  addr_len;  	/* The size of the address buffer. */
        long    msgid;  	/* The sequence number from the request */
        struct ipmi_msg msg; 	/* The data field must point to a buffer. */
};

struct ipmi_cmdspec
{
        unsigned char netfn;
        unsigned char cmd;
};
#if defined(BSD7)
#pragma pack()
#endif
#if defined(BSD) || defined(MACOS) || defined(HPUX)
/* FreeBSD 7.x ipmi ioctls, use _IOW */
#define IPMI_IOC_MAGIC             'i'
#define IPMICTL_RECEIVE_MSG_TRUNC  _IOWR(IPMI_IOC_MAGIC, 11, struct ipmi_recv)
#define IPMICTL_RECEIVE_MSG        _IOWR(IPMI_IOC_MAGIC, 12, struct ipmi_recv)
#define IPMICTL_SEND_COMMAND       _IOW(IPMI_IOC_MAGIC, 13, struct ipmi_req)
#define IPMICTL_REGISTER_FOR_CMD   _IOW(IPMI_IOC_MAGIC, 14, struct ipmi_cmdspec)
#define IPMICTL_UNREGISTER_FOR_CMD _IOW(IPMI_IOC_MAGIC, 15, struct ipmi_cmdspec)
#define IPMICTL_SET_GETS_EVENTS_CMD _IOW(IPMI_IOC_MAGIC, 16, int)
#define IPMICTL_SET_MY_ADDRESS_CMD  _IOW(IPMI_IOC_MAGIC, 17, unsigned int)
#define IPMICTL_GET_MY_ADDRESS_CMD  _IOW(IPMI_IOC_MAGIC, 18, unsigned int)

#else
/* Linux ipmi ioctls */
#define IPMI_IOC_MAGIC 'i'
#define IPMICTL_RECEIVE_MSG_TRUNC   _IOWR(IPMI_IOC_MAGIC, 11, struct ipmi_recv)
#define IPMICTL_RECEIVE_MSG         _IOWR(IPMI_IOC_MAGIC, 12, struct ipmi_recv)
#define IPMICTL_SEND_COMMAND        _IOR(IPMI_IOC_MAGIC,  13, struct ipmi_req)
#define IPMICTL_REGISTER_FOR_CMD    _IOR(IPMI_IOC_MAGIC, 14,struct ipmi_cmdspec)
#define IPMICTL_UNREGISTER_FOR_CMD  _IOR(IPMI_IOC_MAGIC, 15,struct ipmi_cmdspec)
#define IPMICTL_SET_GETS_EVENTS_CMD _IOR(IPMI_IOC_MAGIC,  16, int)
#define IPMICTL_SET_MY_ADDRESS_CMD  _IOR(IPMI_IOC_MAGIC, 17, unsigned int)
#define IPMICTL_GET_MY_ADDRESS_CMD  _IOR(IPMI_IOC_MAGIC, 18, unsigned int)
#endif

/* MAINT ioctls used below, but only valid for Linux */
#define IPMICTL_GET_MAINTENANCE_MODE_CMD _IOR(IPMI_IOC_MAGIC, 30, int)
#define IPMICTL_SET_MAINTENANCE_MODE_CMD _IOW(IPMI_IOC_MAGIC, 31, int)
#define IPMICTL_GETMAINT  0x8004691e   /*only valid if OpenIPMI >=v39.1 */
#define IPMICTL_SETMAINT  0x4004691f   /*only valid if OpenIPMI >=v39.1 */

#define BMC_SA   	  0x20
#define IPMI_BMC_CHANNEL  0xf
#define IPMI_SYSTEM_INTERFACE_ADDR_TYPE 0x0c
#define IPMI_IPMB_ADDR_TYPE             0x01
#define IPMI_IPMB_BROADCAST_ADDR_TYPE   0x41
/* Broadcast get device id is used as described in IPMI 1.5 section 17.9. */
#define IPMI_LAN_ADDR_TYPE              0x04

struct ipmi_system_interface_addr
{
        int           adrtype;
        short         channel;
        unsigned char lun;
};

struct ipmi_ipmb_addr
{
        int    adrtype;
        short  channel;
        uchar  slave_addr;
        uchar  lun;
};

struct ipmi_lan_addr
{
        int           adrtype;
        short         channel;
        unsigned char privilege;
        unsigned char session_handle;
        unsigned char remote_SWID;
        unsigned char local_SWID;
        unsigned char lun;
};

static int ipmi_fd = -1;
static int curr_seq = 0;
static int fdebugmv = 0;
static struct ipmi_addr rsp_addr;  /*used in getevent_mv, ipmi_rsp_mv*/
static int rsp_addrlen = 0;        /*used in getevent_mv, ipmi_rsp_mv*/

void ipmi_get_mymc(uchar *bus, uchar *sa, uchar *lun, uchar *type);

static void dbgmsg(char *pattn, ...)
{
    va_list arglist;

    if (fpdbg == NULL) return;
    // if (fdebugmv == 0) return;
    va_start( arglist, pattn );
    vfprintf( fpdbg, pattn, arglist );
    va_end( arglist );
    fflush( fpdbg );
}

int set_cloexec(int fd, int fdebugcmd)
{
    int flags;
	flags = fcntl(fd, F_GETFD);
	if (flags == -1) {
	   if (fdebugcmd) printf("fcntl(get) errno = %d\n",errno);
       return -1;
    }
	flags |= FD_CLOEXEC;
	if (fcntl(fd, F_SETFD, flags) == -1)
    {
	   if (fdebugcmd) printf("fcntl(set) errno = %d\n",errno);
       return -1;
    }
	return 0;
}

int ipmi_open_mv(char fdebugcmd)
{
    char *pdev;
    uchar bus, sa, lun;

#ifdef ALONE
    fperr = stderr;
    fpdbg = stdout;
#endif

    if (ipmi_fd != -1) return(0); /*already open*/
    fdebugmv = fdebugcmd;
    pdev = "/dev/ipmi/0";
    ipmi_fd = open("/dev/ipmi/0", O_RDWR);
    if (ipmi_fd == -1) {
		if (fdebugcmd) dbgmsg("ipmi_open_mv: cannot open %s\n",pdev);
		pdev = "/dev/ipmi0";
		ipmi_fd = open(pdev, O_RDWR);
    }
    if (ipmi_fd == -1) {
		if (fdebugcmd) dbgmsg("ipmi_open_mv: cannot open %s\n",pdev);
		pdev = "/dev/ipmidev0";
		ipmi_fd = open(pdev, O_RDWR);
    }
    if (ipmi_fd == -1) {
		if (fdebugcmd) dbgmsg("ipmi_open_mv: cannot open %s\n",pdev);
		pdev = "/dev/ipmidev/0";
		ipmi_fd = open(pdev, O_RDWR);
    }
    if (ipmi_fd == -1) { 
		if (fdebugcmd) dbgmsg("ipmi_open_mv: cannot open %s\n",pdev);
		return(-1);
    }
    ipmi_get_mymc(&bus,&sa,&lun,NULL);
    if (sa != BMC_SA) { /* user specified my slave address*/
	int a, rv;
	a = sa;
	rv = ioctl(ipmi_fd, IPMICTL_SET_MY_ADDRESS_CMD, &a);
	if (fdebugcmd) dbgmsg("ipmi_open_mv: set_my_address(%x) rv=%d\n",sa,rv);
	if (rv < 0) {
	   return(rv);
	}
    }
    
	set_cloexec(ipmi_fd,fdebugcmd);
    if (fdebugcmd) {
	dbgmsg("ipmi_open_mv: successfully opened %s, fd=%d\n",pdev,ipmi_fd);
    }
    return(0);
}

int ipmi_close_mv(void)
{
    int rc = 0;
    if (ipmi_fd != -1) { 
	rc = close(ipmi_fd);
	ipmi_fd = -1; 
    }
    return(rc);
}

int ipmi_rsp_mv(uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
		uchar *pdata, int sdata, char fdebugcmd)
{
    struct ipmi_req       req;
    // struct ipmi_recv      rsp;
    struct ipmi_lan_addr  lan_addr;
    // int    i, done;
    int    rv;

    rv = ipmi_open_mv(fdebugcmd);
    if (rv != 0) return(rv);

    if (rsp_addrlen > 0) {
        /* rsp_addr was previously saved in getevent_mv */
	req.addr = (char *)&rsp_addr;
	req.addr_len = rsp_addrlen;
    } else {  /* try some defaults */
	lan_addr.adrtype = IPMI_LAN_ADDR_TYPE;
	lan_addr.channel = bus;   /* usu lan_ch == 1 */
        lan_addr.privilege = 0x04;  /*admin*/
	lan_addr.session_handle = 0x01; /*may vary*/
	lan_addr.remote_SWID = sa; /*usu 0x81*/
	lan_addr.local_SWID = 0x81;
	lan_addr.lun = lun;
	req.addr = (char *) &lan_addr;
	req.addr_len = sizeof(lan_addr);
    }
    req.msg.cmd = cmd;
    req.msg.netfn = (netfn | 0x01);
    req.msgid = curr_seq;
    req.msg.data = pdata;
    req.msg.data_len = sdata;
    rv = ioctl(ipmi_fd, IPMICTL_SEND_COMMAND, &req);
    curr_seq++;
    if (rv == -1) { 
        if (fdebugcmd) dbgmsg("mv IPMICTL_SEND_COMMAND errno %d\n",errno);
	rv = errno; 
    }
    return(rv);
}

int ipmicmd_mv(uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
		uchar *pdata, int sdata, uchar *presp, int sresp, int *rlen)
{
    fd_set readfds;
    struct timeval tv;
    struct ipmi_req       req;
    struct ipmi_recv      rsp;
    struct ipmi_addr      addr;
    struct ipmi_ipmb_addr             ipmb_addr;
    struct ipmi_system_interface_addr bmc_addr;
    static int need_set_events = 1;
    int    i, done;
    int    rv;

    rv = ipmi_open_mv(fdebugmv);
    if (rv != 0) return(rv);

    if (need_set_events) {
	i = 1;
        rv = ioctl(ipmi_fd, IPMICTL_SET_GETS_EVENTS_CMD, &i);
	if (fdebugmv) 
	    dbgmsg("getevent_mv: set_gets_events rv=%d errno=%d, n=%d\n",
			rv,errno,i);
        if (rv) { return(errno); }
	need_set_events = 0;
    }

    FD_ZERO(&readfds);
    // FD_SET(0, &readfds);  /* dont watch stdin */
    FD_SET(ipmi_fd, &readfds);  /* only watch ipmi_fd for input */

    /* Special handling for ReadEventMsgBuffer, etc. */
#ifdef TEST_MSG
    recv.msg.data = data;
    recv.msg.data_len = sizeof(data);
    recv.addr = (unsigned char *) &addr;
    recv.addr_len = sizeof(addr);
    rv = ioctl(fd, IPMICTL_RECEIVE_MSG_TRUNC, &recv);
    if (rv == -1) {
        if (errno == EMSGSIZE) {
            /* The message was truncated, handle it as such. */
            data[0] = IPMI_REQUESTED_DATA_LENGTH_EXCEEDED_CC;
            rv = 0;
        } else
            goto out;
    }
#endif

    /* Send the IPMI command */ 
    if (sa == BMC_SA) {
	i = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
	bmc_addr.adrtype = i;
	bmc_addr.channel = IPMI_BMC_CHANNEL;
	bmc_addr.lun = lun;       /* usu BMC_LUN = 0 */
	req.addr = (char *) &bmc_addr;
	req.addr_len = sizeof(bmc_addr);
    } else {
	i = IPMI_IPMB_ADDR_TYPE;
	ipmb_addr.adrtype = i;
	ipmb_addr.channel = bus;   /* usu PUBLIC_BUS = 0 */
	ipmb_addr.slave_addr = sa;
	ipmb_addr.lun = lun;
	req.addr = (char *) &ipmb_addr;
	req.addr_len = sizeof(ipmb_addr);
    }
    if (fdebugmv) 
	dbgmsg("mv cmd=%02x netfn=%02x mc=%02x;%02x;%02x adrtype=%x\n",
			cmd,netfn,bus,sa,lun,i);
    req.msg.cmd = cmd;
    req.msg.netfn = netfn;   
    req.msgid = curr_seq;
    req.msg.data = pdata;
    req.msg.data_len = sdata;
    rv = ioctl(ipmi_fd, IPMICTL_SEND_COMMAND, &req);
    curr_seq++;
    if (rv == -1) { 
        if (fdebugmv) dbgmsg("mv IPMICTL_SEND_COMMAND errno %d\n",errno);
	rv = errno; 
	}

    if (netfn & 0x01) done = 1;  /*sending response only*/
    else done = 0;  /*normal request/response*/

    if (rv == 0) while (!done) {
        done = 1;
	tv.tv_sec=ipmi_timeout_mv;
	tv.tv_usec=0;
	rv = select(ipmi_fd+1, &readfds, NULL, NULL, &tv);
	/* expect select rv = 1 here */
	if (rv <= 0) { /* no data within 5 seconds */
	   if (fdebugmv) 
              fprintf(fperr,"mv select timeout, fd = %d, isset = %d, rv = %d, errno = %d\n",
		  ipmi_fd,FD_ISSET(ipmi_fd, &readfds),rv,errno);
	   if (rv == 0) rv = -3;
	   else rv = errno;
	} else {
	   /* receive the IPMI response */
	   rsp.addr = (char *) &addr;
	   rsp.addr_len = sizeof(addr);
	   rsp.msg.data = presp;
	   rsp.msg.data_len = sresp;
	   rv = ioctl(ipmi_fd, IPMICTL_RECEIVE_MSG_TRUNC, &rsp);
	   if (rv == -1) { 
	      if ((errno == EMSGSIZE) && (rsp.msg.data_len == sresp))
		 rv = 0;   /* errno 90 is ok */
	      else { 
		 rv = errno; 
                 fprintf(fperr,"mv rcv_trunc errno = %d, len = %d\n",
			errno, rsp.msg.data_len);
	      }
	   } else rv = 0;
	   /* Driver should ensure matching req.msgid and rsp.msgid */
           /* Skip & retry if async events, only listen for those in 
            * getevent_mv() below. */
           // if (rsp.recv_type == IPMI_ASYNC_EVENT_RECV_TYPE) 
	   if (rsp.recv_type != IPMI_RESPONSE_RECV_TYPE) {
		if (fdebugmv)
		   dbgmsg("mv cmd=%02x netfn=%02x, got recv_type %d\n",
				cmd,netfn,rsp.recv_type);
	       done = 0;
	   }
	   *rlen = rsp.msg.data_len;
	}
    } /*endif send ok, while select/recv*/

    /* ipmi_close_mv();  * rely on the app calling ipmi_close */
    return(rv);
}

int ipmi_cmdraw_mv(uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
		uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd)
{
    uchar  buf[MV_BUFFER_SIZE];
    int rc, szbuf;
    int rlen = 0;
    uchar cc;

    if (fdebugcmd) { 
         dbgmsg("mv cmd=%02x netfn=%02x lun=%02x sdata=%d sresp=%d\n",
		 cmd,netfn,lun,sdata,*sresp); 
         dump_buf("mv cmd data",pdata,sdata,0);
    }
    szbuf = sizeof(buf);
    if (*sresp <  2) ;   /*just completion code*/
    else if (*sresp < szbuf) szbuf = *sresp + 1;
    else if (fdebugcmd) 
	dbgmsg("mv sresp %d >= szbuf %d, truncated\n",*sresp,szbuf);
    rc = ipmicmd_mv(cmd,netfn,lun,sa, bus, pdata,sdata, buf,szbuf,&rlen);
    cc = buf[0];
    if (fdebugcmd) {
        dbgmsg("ipmi_cmdraw_mv: status=%d ccode=%x rlen=%d\n",
		(uint)rc,cc,rlen);
        if (rc == 0) dump_buf("mv rsp data",buf,rlen,0);
    }
    if (rlen > 0) { /* copy data, except first byte */
       rlen -= 1;
       if (rlen > *sresp) rlen = *sresp;
       memcpy(presp,&buf[1],rlen);
    }
    *pcc = cc;
    *sresp = rlen;
    return(rc);
}

#ifdef ALONE
void ipmi_get_mymc(uchar *bus, uchar *sa, uchar *lun, uchar *type)
{
   if (bus != NULL)  *bus = 0;  //PUBLIC_BUS;
   if (sa != NULL)   *sa  = BMC_SA;
   if (lun != NULL)  *lun = 0;  //BMC_LUN;
   if (type != NULL) *type = 1; //ADDR_SMI;
}
#else
int ipmi_cmd_mv(ushort cmd, uchar *pdata, int sdata, uchar *presp, 
		int *sresp, uchar *pcc, char fdebugcmd)
{
    uchar  buf[MV_BUFFER_SIZE];
    int rc, i, szbuf;
    uchar   cc;
    int rlen = 0;
    int xlen, j;
    uchar sa, lun, bus, mtype;

    for (i = 0; i < NCMDS; i++) {
       if (ipmi_cmds[i].cmdtyp == cmd) break;
    }
    if (i >= NCMDS) {
	fprintf(fperr, "ipmi_cmd_mv: Unknown command %x\n",cmd);
	return(-1); 
	}
    if (cmd >= CMDMASK) cmd = cmd & CMDMASK;  /* unmask it */

    if (fdebugcmd) { 
         dbgmsg( "mv cmd=%02x netfn=%02x lun=%02x sdata=%d sresp=%d\n",
		cmd,ipmi_cmds[i].netfn,ipmi_cmds[i].lun,sdata, *sresp); 
         dump_buf("mv cmd data",pdata,sdata,0);
    }
    szbuf = sizeof(buf);
    if (*sresp < szbuf && *sresp >= 2) szbuf = *sresp + 1;
    else if (fdebugcmd) 
	dbgmsg("mv sresp %d >= szbuf %d, truncated\n",*sresp,szbuf);
    // ipmi_cmds[i].lun, ipmi_cmds[i].sa, ipmi_cmds[i].bus,
    ipmi_get_mc(&bus, &sa, &lun, &mtype);
    rc = ipmicmd_mv(cmd,ipmi_cmds[i].netfn, lun, sa, bus,
			pdata,sdata,buf,szbuf,&rlen);
    // if (rc == -1) dbgmsg("ipmi_cmd_mv: cannot open /dev/ipmi0\n");
    cc = buf[0];
    if (fdebugcmd) {
	    dbgmsg("ipmi_cmd_mv: ipmicmd_mv status=%x, ccode=%x\n", 
                      (uint)rc, cc);
            if (rc == ACCESS_OK) {
               uchar * pc; int sz;
               sz = rlen;
               pc = (uchar *)buf;
               dbgmsg("ipmi_cmd_mv: response (len=%d): ",sz);
               for (j = 0; j < sz; j++) dbgmsg("%02x ",pc[j]);
               dbgmsg("\n");
            }
        }
    xlen = ipmi_cmds[i].rslen + 1;
    if ((ipmi_cmds[i].cmdtyp == GET_SEL_ENTRY) && 
        (rlen < xlen) && (rc == 0) && (cc != 0) &&
        (i > 0) && (rlen > 1))                  /*not temp slot, have data*/ 
    {
          /* Detect/Handle MV driver SEL bug returning missing bytes */
          if (fdebugcmd) {
              dbgmsg("ipmi_cmd_mv[%d] BUG: returned %d, expected %d\n",
				i,rlen,xlen);
          }
          cc = 0x80;  /*flag as busy, retry*/
          j = xlen - rlen;
          j--;  /* omit cc */
          for (i = 0; i < j; i++) presp[i] = 0xff;
          if ((rlen+j) > *sresp) rlen = *sresp - j; 
          memcpy(&presp[j],&buf[0],rlen);
          rlen += j;
    }
    if (rlen > 0) {
       /* copy data, except first byte */
       rlen -= 1;
       if (rlen > *sresp) rlen = *sresp;
       memcpy(presp,&buf[1],rlen);
    }
    *pcc = cc;
    *sresp = rlen;

    return(rc);
}  /*end ipmi_cmd_mv*/
#endif

int setmaint_mv(uchar mode, uchar *cc)
{
    int data[2]; 
    int rv;
    /* 
     * Normally OpenIPMI driver issues ReadEventMessageBuffer cmds
     * every 1 second.  Maint mode disables that polling in 
     * driver v39.1 and greater.
     * maintenance mode values: 
     * 2 = turn on maint mode
     * 1 = turn off maint mode
     * 0 = automatic maint mode (on for 30 sec if reset or fw request)
     */

    /* should have called ipmi_open_mv in a previous call */
    rv = ioctl(ipmi_fd, IPMICTL_GETMAINT, &data);
    if (rv == -1) {
        if (errno != 0) *cc = errno;
    } else *cc = 0;
    if (fdebugmv) dbgmsg("getmaint: rv=%d mode=%d\n",rv,data[0]);  

    data[0] = mode;  
    rv = ioctl(ipmi_fd, IPMICTL_SETMAINT, &data);
    if (rv == -1) {
        if (errno != 0) *cc = errno;
    } else *cc = 0;
    return(rv);
}

int register_async_mv(uchar cmd, uchar netfn)
{
    uchar data[2]; 
    int rv;

    data[0] = netfn;
    data[1] = cmd;
    rv = ioctl(ipmi_fd, IPMICTL_REGISTER_FOR_CMD, &data);
    if (fdebugmv) dbgmsg("register_async_mv(%x,%x) rv=%d\n",cmd,netfn,rv);
    return(rv);
}

int unregister_async_mv(uchar cmd, uchar netfn)
{
    uchar data[2]; 
    int rv;

    data[0] = netfn;
    data[1] = cmd;
    rv = ioctl(ipmi_fd, IPMICTL_UNREGISTER_FOR_CMD, &data);
    if (fdebugmv) dbgmsg("unregister_async_mv(%x,%x) rv=%d\n",cmd,netfn,rv);
    return(rv);
}

int getevent_mv(uchar *evt_data, int *evt_len, uchar *cc, int timeout)
{
    struct ipmi_recv rsp;
    uchar data[36];  /* #define MAX_IPMI_DATA_SIZE 36 */
    struct ipmi_addr  addr;
    static int need_set_events = 1;
    int rv = 0;
    int n;
 
    if (need_set_events) {
	n = 1;
        rv = ioctl(ipmi_fd, IPMICTL_SET_GETS_EVENTS_CMD, &n);
	if (fdebugmv) 
	    dbgmsg("getevent_mv: set_gets_events rv=%d errno=%d, n=%d\n",
			rv,errno,n);
	need_set_events = 0;
    }

    /* wait for the mv openipmi driver to provide input to fd */
    if (timeout == 0) 
    {   /*do poll*/
#if defined(MACOS)
	/* there is no poll function in MACOS, so skip this. */
#else
    	struct pollfd myfd;
    	myfd.fd = ipmi_fd;
    	myfd.events = POLLIN;
    	myfd.revents = 0;
    	rv = poll(&myfd,1,-1);
    	if (rv <= 0) {
		if (fdebugmv) dbgmsg("getevent_mv poll rv=%d\n",rv);
		return(rv);
    	}
        /* else have input ready to read (myfd.revents & POLLIN)*/
	if (fdebugmv) dbgmsg("getevent_mv poll revents %x\n",myfd.revents);
#endif
    }

    /* read the message from the ipmi_fd */
    rsp.msg.data = data;
    rsp.msg.data_len = sizeof(data);
    rsp.addr = (unsigned char *) &addr;
    rsp.addr_len = sizeof(addr);
    rv = ioctl(ipmi_fd, IPMICTL_RECEIVE_MSG_TRUNC, &rsp);
    if (rv < 0) {  
	if (fdebugmv) dbgmsg("getevent_mv rv=%d, errno=%d\n",rv,errno);
        if (errno == EMSGSIZE) { /* The message was truncated */
            *cc = 0xC8;   /*IPMI_REQUESTED_DATA_LENGTH_EXCEEDED_CC;*/
            rsp.msg.data_len = sizeof(data);
            rv = 0;
        }
        else if (errno == EINTR) { return(EINTR); }
    } else *cc = 0;
    if (rv == 0) {
        n = rsp.msg.data_len;
	if (fdebugmv) {
	    dbgmsg("getevent_mv: recv_type=%x cmd=%x data_len=%d\n",
		   rsp.recv_type,rsp.msg.cmd,n);
	    // if (n > 0) dump_buf("mv rsp.msg.data",rsp.msg.data, n, 0);
	    // dump_buf("mv rsp.addr",rsp.addr,rsp.addr_len,0);
	}
	if (rsp.recv_type == IPMI_CMD_RECV_TYPE) {
	    evt_data[0] = rsp.recv_type;
            evt_data[1] = rsp.msg.netfn;
            evt_data[2] = rsp.msg.cmd;
	    if (n > 0) 
               memcpy(&evt_data[3],&data[0],n);
	    n += 3;
	    /* save the response address */
	    memcpy(&rsp_addr,rsp.addr,rsp.addr_len);
	    rsp_addrlen = rsp.addr_len;
	} else if (rsp.recv_type == IPMI_RESPONSE_RESPONSE_TYPE) {
	    evt_data[0] = rsp.recv_type;
            evt_data[1] = rsp.msg.netfn;
            evt_data[2] = rsp.msg.cmd;
            evt_data[3] = data[0];
	    n += 3;
        } else {  /* rsp.recv_type == IPMI_ASYNC_EVENT_RECV_TYPE */
	    if (n > 0) 
               memcpy(evt_data,&data[0],n);
	}
        *evt_len = n;
    } else if (rv == -1 || rv == -11) {
        rv = 0x80;  /* -EAGAIN, no data, try again */
    }
    return(rv);
}

#ifdef TEST_BIN
int main(int argc, char *argv[])
{
    fd_set readfds;
    struct timeval tv;
    uchar  data[40];
    int    i, j;
    int    err, rv;
    int    rlen = 0;
    uchar cc;

    fperr = stderr;
    fpdbg = stdout;
    fdebugmv = 1;
    rlen = sizeof(data);
    err = ipmi_cmdraw_mv(0x01, 0x06, 0, 0x20, 0,  /*get_device_id*/
			NULL, 0, data, &rlen, &cc, fdebugmv);
    dbgmsg("ipmi_cmdraw_mv ret=%d, cc=%02x\n",err,cc);
    rv = err;
    if (err == 0) {
       rv = cc;
       dbgmsg(" ** Return Code: %2.2X\n", cc);
       dbgmsg(" ** Data[%d]:",rlen);
       for (i=0; i < rlen; i++)
	    dbgmsg(" %2.2X", (uchar)data[i]);
       dbgmsg("\n");
    }

    err = setmaint_mv(2,&cc);
    dbgmsg("setmaint_mv(2) err=%d cc=%d\n",err,cc);
    err = setmaint_mv(1,&cc);
    dbgmsg("setmaint_mv(1) err=%d cc=%d\n",err,cc);
    err = setmaint_mv(0,&cc);
    dbgmsg("setmaint_mv(0) err=%d cc=%d\n",err,cc);

    err = register_async_mv(0x10,0x06);
    dbgmsg("register_async_mv(10,6) rv=%d\n",err);
    err = register_async_mv(0x01,0x06);
    dbgmsg("register_async_mv(01,6) rv=%d\n",err);
    err = unregister_async_mv(0x01,0x06);
    dbgmsg("unregister_async_mv(01,6) rv=%d\n",err);
    err = unregister_async_mv(0x10,0x06);
    dbgmsg("unregister_async_mv(10,6) rv=%d\n",err);

    dbgmsg("\n");
    ipmi_close_mv();
    return rv;
}
#endif

#endif
