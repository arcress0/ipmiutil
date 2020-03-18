/*M*
//  PVCS:
//      $Workfile:   ipmild.c  $
//      $Revision:   1.0  $
//      $Modtime:   23 Feb 2005 15:20:14  $
//      $Author:   arcress at users.sourceforge.net  $  
//
//  This implements support for the /dev/ldipmi native interface from
//  the LanDesk IPMI driver.
//  Requires linking with libldipmi.a, or equivalent.
// 
//  02/23/05 ARC - created, but stubbed out, since the
//                 LanDesk libipmiapi.a isn't clean yet.
 *M*/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2005, Intel Corporation
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

#ifdef LINUX
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ipmicmd.h"   /* for uchar, NCMDS */
// #define uchar    unsigned char
#define MAX_NO_OF_RETRIES       3 

// Request structure provided to SendTimedImbpRequest()
#pragma pack(1)
typedef struct {
        uchar   cmdType;        // IMB command
        uchar   rsSa;           // command destination address
        uchar   busType;        // not used
        uchar   netFn;          // IMB command class (network function)
        uchar   rsLun;          // subsystem on destination
        uchar * data;           // command body
        int     dataLength;     // body size
} IMBPREQUESTDATA;
#pragma pack() 

#ifdef LINK_LANDESK
/* typedef enum { 0, 1, 2, 3, 4, 5, 6 } ACCESN_STATUS; */
/* Note that this routine name conflicts with ia_ imb routine. */
extern int SendTimedImbpRequest (
        IMBPREQUESTDATA *reqPtr,
        int             timeOut,
        uchar *          respDataPtr,
        int *           respDataLen,
        uchar *          completionCode);
extern int initIPMI();
extern int termIPMI();
static int ipmi_timeout_ld = 100000;  /*100 * 1000 ms = 100 sec */
#endif

extern FILE *fperr;  /*defined in ipmicmd.c*/           
extern FILE *fpdbg;  /*defined in ipmicmd.c*/           
extern ipmi_cmd_t ipmi_cmds[NCMDS];
#ifdef LINK_LANDESK
static int ipmi_fd = -1;
#endif

int ipmi_open_ld(char fdebugcmd)
{
    int rc = -1;
#ifdef TEST
    char *pdev;

    if (ipmi_fd != -1) return(0);
    pdev = "/dev/ldipmi";
    ipmi_fd = open(pdev, O_RDWR);
    if (ipmi_fd == -1) {
	if (fdebugcmd) printf("ipmi_open_ld: cannot open %s\n",pdev);
	pdev = "/dev/ldipmi/0";
        ipmi_fd = open(pdev, O_RDWR);
    }
#endif
#ifdef  LINK_LANDESK
    rc = initIPMI();
    if (rc != 0) {
	if (fdebugcmd) printf("ipmi_open_ld: cannot open ldipmi, rc=%d errno=%d\n",rc,errno);
	return(-1);
    }
    ipmi_fd = 1;  /*show that open was ok*/
    if (fdebugcmd) printf("ipmi_open_ld: successfully opened ldipmi\n");
#endif
    return(rc);
}

int ipmi_close_ld(void)
{
    int rc = 0;
#ifdef  LINK_LANDESK
    if (ipmi_fd != -1) { 
	termIPMI();
	ipmi_fd = -1; 
    }
#endif
    return(rc);
}

int ipmicmd_ld( uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
		uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd)
{
#ifdef  LINK_LANDESK
   IMBPREQUESTDATA requestData;
   int status = 0;
   uchar * pc;
   int sz, i;

   requestData.cmdType	= cmd;
   requestData.rsSa	= sa;
   requestData.busType	= bus;
   requestData.netFn	= netfn;
   requestData.rsLun	= lun;
   requestData.dataLength = sdata;
   requestData.data       = pdata; 

   if (fdebugcmd) {
          sz = sizeof(IMBPREQUESTDATA);
          pc = (uchar *)&requestData.cmdType;
          fprintf(fpdbg,"ipmicmd_ld: request (len=%d): ",sz);
          for (i = 0; i < sz; i++) fprintf(fpdbg,"%02x ",pc[i]);
          fprintf(fpdbg,"\n");
          pc = requestData.data;
          sz = requestData.dataLength;
          fprintf(fpdbg,"  req.data=%p, dlen=%d: ", pc, sz);
          for (i = 0; i < sz; i++) fprintf(fpdbg,"%02x ",pc[i]);
          fprintf(fpdbg,"\n");
        }

   if (ipmi_fd == -1) {
	status = ipmi_open_ld(fdebugcmd);
	if (status != 0) return(status);
   }

   { 
	   sz = *sresp;  /* note that sresp must be pre-set */
	   memset(presp, 0, sz);
           for ( i =0 ; i < MAX_NO_OF_RETRIES; i++) 
	   {
		*sresp = sz;   /* retries may need to re-init *sresp */
		if((status =SendTimedImbpRequest(&requestData, 
				ipmi_timeout_ld, presp, sresp, 
				pcc)) == 0 ) {
			break;
			}
		if (fdebugcmd)   // only gets here if error
	          fprintf(fpdbg,"ipmi_cmd_ld: sendImbRequest error status=%x, ccode=%x\n",
                            (uint)status, *pcc);
	   } 
   }

    if (fdebugcmd) {  /* if debug, show both good and bad statuses */
	    fprintf(fpdbg,"ipmi_cmd_ld: sendImbRequest status=%x, ccode=%x\n", 
                      (uint)status, *pcc);
            if (status == 0) {  
               uchar * pc; int sz;
               sz = *sresp;
               pc = (uchar *)presp;
               fprintf(fpdbg,"ipmi_cmd_ld: response (len=%d): ",sz);
               for (i = 0; i < sz; i++) fprintf(fpdbg,"%02x ",pc[i]);
               fprintf(fpdbg,"\n");
            }
        }

    return(status);
#else
    return(-1);
#endif
} /* end ipmicmd_ld() */

int ipmi_cmdraw_ld( uchar cmd, uchar netfn, uchar lun, uchar sa, uchar bus,
		uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd)
{
    int rc;
    rc = ipmicmd_ld(cmd, netfn, lun, sa, bus,
                        pdata,sdata,presp,sresp,pcc,fdebugcmd);
    return(rc);
}

int ipmi_cmd_ld(ushort cmd, uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd)
{
    int rc, i;
 
    for (i = 0; i < NCMDS; i++) {
       if (ipmi_cmds[i].cmdtyp == cmd) break;
    }
    if (i >= NCMDS) {
        fprintf(fperr, "ipmi_cmd_ld: Unknown command %x\n",cmd);
        return(-1);
        }
    if (cmd >= CMDMASK) cmd = cmd & CMDMASK;  /* unmask it */
    rc = ipmicmd_ld(cmd, ipmi_cmds[i].netfn, ipmi_cmds[i].lun,
                        ipmi_cmds[i].sa, ipmi_cmds[i].bus,
                        pdata,sdata,presp,sresp,pcc,fdebugcmd);
    return(rc);
}

#ifdef LINK_LANDESK
/* define extra stuff that the LanDesk library needs */
void * _Unwind_Resume(void *context)
{
   printf("called Unwind_Resume\n");
   return(NULL);
}

int  __gxx_personality_v0(void *a)
{
   return(0);
}

#endif

#endif

