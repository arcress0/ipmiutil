/*M*
//      $Workfile:   ipmibmc.c  $
//      $Revision:   1.0  $
//      $Modtime:   12 Nov 2008 15:20:14  $
//      $Author:   arcress at users.sourceforge.net  $  
//
//  This implements support for the /dev/bmc interface from
//  the Solaris 10 IPMI driver.
// 
//  11/12/08 ARC - created 
 *M*/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2008, Intel Corporation
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

#ifdef SOLARIS
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stropts.h>
#include <stddef.h>
#include <stropts.h>
#include <iso/stdlib_iso.h>

#include "ipmicmd.h"   /* for uchar, NCMDS */

#define MAX_SEND_SIZE   34
#define MAX_RECV_SIZE   33
#define MIN_RQ_SIZE   2
#define MIN_RS_SIZE   3
#define MAX_BUF_SIZE  256
#define MSG_BUF_SIZE  1024

// #define IOCTL_IPMI_KCS_ACTION           0x01
// #define IOCTL_IPMI_INTERFACE_METHOD     0x02
#define BMC_MSG_REQ  1
#define BMC_MSG_RSP  2
#define BMC_MSG_ERR  3

extern ipmi_cmd_t ipmi_cmds[NCMDS];
static int ipmi_fd = -1;

typedef struct bmc_rq {
   uchar netfn;
   uchar lun;
   uchar cmd;
   uchar dlen;
   uchar data[MAX_SEND_SIZE];
} bmc_rq_t;

typedef struct bmc_rs {
   uchar netfn;
   uchar lun;
   uchar cmd;
   uchar ccode;
   uchar dlen;
   uchar data[MAX_RECV_SIZE];
} bmc_rs_t;

typedef struct bmc_ioctl_t {
        bmc_rq_t       req; 
        bmc_rs_t       rsp; 
} bmc_ioctl_t;

typedef struct bmc_msg {
        uchar       m_type;         /* Message type (1=req, 2=resp, 3=error)*/
        uint32      m_id;           /* Message ID */
        uchar       reserved[32];
        uchar       msg[1];         /* Variable length message data */
} bmc_msg_t;


int ipmi_open_bmc(char fdebugcmd)
{
    int rc = -1;
    char *pdev;

    if (ipmi_fd != -1) return(0);
    pdev = "/dev/bmc";
    ipmi_fd = open(pdev, O_RDWR);
    if (ipmi_fd == -1) {
	if (fdebugcmd) printf("ipmi_open_bmc: cannot open %s, errno=%d\n",pdev,errno);
	return(rc);
    }

    /* dont bother to check for ioctl method, just use putmsg method */

    rc = 0;
    if (fdebugcmd) printf("ipmi_open_bmc: successfully opened bmc\n");
    return(rc);
}

int ipmi_close_bmc(void)
{
    int rc = 0;
    if (ipmi_fd != -1) { 
	close(ipmi_fd);
	ipmi_fd = -1; 
    }
    return(rc);
}

int ipmi_cmdraw_bmc( uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
		uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd)
{
    int rv = -1;
    static uint32 msg_seq = 0;
    int flags = 0;
    struct strbuf sendbuf;
    struct strbuf recvbuf;
    bmc_msg_t *msg;
    bmc_rq_t  *rq;
    bmc_rs_t  *rs;
    int sz, i;

    if (sdata > MAX_SEND_SIZE) i = sdata - MAX_SEND_SIZE;
    else i = 0;
    sz = (sizeof(bmc_msg_t) - 1) + sizeof(bmc_rq_t) + i;
    msg = malloc(sz);
    if (msg == NULL) return(rv);
    rq = (bmc_rq_t *)&msg->msg[0];

    msg->m_type = BMC_MSG_REQ;
    msg->m_id   = msg_seq++;
    rq->netfn = netfn;
    rq->lun   = lun;
    rq->cmd   = cmd;
    rq->dlen  = sdata;
    memcpy(rq->data, pdata, sdata);
    sendbuf.len = sz;
    sendbuf.buf = (uchar *)msg;
    if (fdebugcmd) {
	dump_buf("ipmi_cmdraw_bmc sendbuf",sendbuf.buf,sendbuf.len,0);
    }

    rv = putmsg(ipmi_fd, NULL, &sendbuf, 0);
    if (rv < 0) {
	perror("BMC putmsg: ");
        free(msg);
	return(rv);
    }
    free(msg);

    recvbuf.buf = malloc(MSG_BUF_SIZE);
    recvbuf.maxlen = MSG_BUF_SIZE;
    rv = getmsg(ipmi_fd, NULL, &recvbuf, &flags);
    if (rv < 0) {
	perror("BMC getmsg: ");
        free(recvbuf.buf);
	return(rv);
    }

    msg = (bmc_msg_t *)recvbuf.buf;
    if (fdebugcmd) {
	dump_buf("ipmi_cmdraw_bmc recvbuf",recvbuf.buf,recvbuf.len,0);
    }
    switch (msg->m_type) {
	case BMC_MSG_RSP:
	   rs = (bmc_rs_t *)&msg->msg[0];
	   *pcc = rs->ccode;
	   i = rs->dlen;
	   if (i < 0) i = 0;
	   *sresp = i;
	   if (*pcc == 0 && i > 0) 
		memcpy(presp,rs->data,i);
	   rv = 0;
	   break;
        case BMC_MSG_ERR:
        default:
	   rv = msg->msg[0];
           printf("ipmi_cmdraw_bmc: %s\n", strerror(rv));
           break;
    }
    free(recvbuf.buf);

    return(rv);
}

int ipmi_cmd_bmc(ushort cmd, uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd)
{
    int rc, i;
 
    for (i = 0; i < NCMDS; i++) {
       if (ipmi_cmds[i].cmdtyp == cmd) break;
    }
    if (i >= NCMDS) {
        printf("ipmi_cmd_bmc: Unknown command %x\n",cmd);
        return(-1);
        }
    if (cmd >= CMDMASK) cmd = cmd & CMDMASK;  /* unmask it */
    rc = ipmi_cmdraw_bmc(cmd, ipmi_cmds[i].netfn, ipmi_cmds[i].lun,
                        ipmi_cmds[i].sa, ipmi_cmds[i].bus,
                        pdata,sdata,presp,sresp,pcc,fdebugcmd);
    return(rc);
}
#endif
/* end of ipmibmc.c */
