/*M*
//      $Workfile:   ipmilipmi.c  $
//      $Revision:   0.1  $
//      $Modtime:   18 Oct 2010 15:20:14  $
//      $Author:   arcress at users.sourceforge.net  $  
//
//  This implements support for the /dev/lipmi interface from
//  the Solaris 8 & 9 LIPMI driver.
// 
//  10/18/10 ARC - created 
 *M*/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2010 Kontron America, Inc.
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

#include "ipmicmd.h"   /* for uchar, NCMDS */

#define MAX_SEND_SIZE   34
#define MAX_RECV_SIZE   33  /*TODO: compare with RECV_MAX_PAYLOAD_SIZE */
#define IOCTL_IPMI_KCS_ACTION           0x01
/* I_STR  should be defined in stropts.h */

extern ipmi_cmd_t ipmi_cmds[NCMDS];
static int  ipmi_fd = -1;
static char *pdev = "/dev/lipmi";

#ifndef _SYS_STROPTS_H
/* see sys/stropts.h, sys/lipmi/lipmi_intf.h*/ 
typedef struct strioctl {   
    int ic_cmd;
    int ic_timout;
    int ic_len;
    char *ic_dp;
} strioctl_t;
#endif

typedef struct bmc_req {
   uchar fn;
   uchar lun;
   uchar cmd;
   uchar datalength;
   uchar data[MAX_SEND_SIZE];
} bmc_req_t;

typedef struct bmc_rsp {
   uchar fn;
   uchar lun;
   uchar cmd;
   uchar ccode;
   uchar datalength;
   uchar data[MAX_RECV_SIZE];
} bmc_rsp_t;

typedef struct lipmi_reqrsp {    /*TODO: see sys/lipmi/lipmi_intf.h*/
   bmc_req_t       req; 
   bmc_rsp_t       rsp; 
} lipmi_reqrsp_t;


int ipmi_open_lipmi(char fdebugcmd)
{
    int rc = -1;

    if (ipmi_fd != -1) return(0);
    ipmi_fd = open(pdev, O_RDWR);
    if (ipmi_fd < 0) {
	if (fdebugcmd) printf("ipmi_open_lipmi: cannot open %s, errno=%d\n",
				pdev,errno);
	return(rc);
    }
    rc = 0;
    if (fdebugcmd) printf("ipmi_open_lipmi: successfully opened\n");
    return(rc);
}

int ipmi_close_lipmi(void)
{
    int rc = 0;
    if (ipmi_fd != -1) { 
	close(ipmi_fd);
	ipmi_fd = -1; 
    }
    return(rc);
}

int ipmi_cmdraw_lipmi( uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
		uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd)
{
    int rv = -1;
    struct strioctl istr;
    static struct lipmi_reqrsp reqrsp;
    int len;
    uchar cc;

    if (ipmi_fd == -1) return -1;

    memset(&reqrsp, 0, sizeof(reqrsp));
    reqrsp.req.fn = netfn;
    reqrsp.req.lun = lun;
    reqrsp.req.cmd = cmd;
    reqrsp.req.datalength = sdata; 
    memcpy(reqrsp.req.data, pdata, sdata); 
    reqrsp.rsp.datalength = MAX_RECV_SIZE;

    istr.ic_cmd = IOCTL_IPMI_KCS_ACTION;
    istr.ic_timout = 0;
    istr.ic_dp = (char *)&reqrsp;
    istr.ic_len = sizeof(struct lipmi_reqrsp);

    rv = ioctl(ipmi_fd, I_STR, &istr);
    if (rv < 0) { 
        perror("LIPMI IOCTL: I_STR");
        return rv;
    }

    cc = reqrsp.rsp.ccode;
    len = reqrsp.rsp.datalength;
    *pcc = cc;
    *sresp = len;	
    if (cc == 0 && len > 0) 
        memcpy(presp, reqrsp.rsp.data, len);
    return(rv);
}

int ipmi_cmd_lipmi(ushort cmd, uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd)
{
    int rc, i;
 
    for (i = 0; i < NCMDS; i++) {
       if (ipmi_cmds[i].cmdtyp == cmd) break;
    }
    if (i >= NCMDS) {
        printf("ipmi_cmd_lipmi: Unknown command %x\n",cmd);
        return(-1);
        }
    if (cmd >= CMDMASK) cmd = cmd & CMDMASK;  /* unmask it */
    rc = ipmi_cmdraw_lipmi(cmd, ipmi_cmds[i].netfn, ipmi_cmds[i].lun,
                        ipmi_cmds[i].sa, ipmi_cmds[i].bus,
                        pdata,sdata,presp,sresp,pcc,fdebugcmd);
    return(rc);
}
#endif
/* end of ipmilipmi.c */
