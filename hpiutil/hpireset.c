/*
 * hpireset.c
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2003 Intel Corporation.
 *
 * 05/02/03 Andy Cress - created
 * 06/06/03 Andy Cress - beta2
 * 06/26/03 Andy Cress - exclude stuff that isn't in HPI 1.0 (MAYBELATER)
 * 12/07/04 Andy Cress - added HPI_A/B compile flags
 */
/*M*
Copyright (c) 2003, Intel Corporation
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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "SaHpi.h"

#define  uchar  unsigned char
char *progver  = "1.1";
char fdebug = 0;

static void Usage(char *pname)
{
                printf("Usage: %s [-r -d -x]\n", pname);
                printf(" where -r  hard Resets the system\n");
                printf("       -d  powers Down the system\n");
#ifdef MAYBELATER
/*++++ not implemented in HPI 1.0 ++++
                printf("       -c  power Cycles the system\n");
                printf("       -n  sends NMI to the system\n");
                printf("       -o  soft-shutdown OS\n");
                printf("       -s  reboots to Service Partition\n");
 ++++*/
#endif
                printf("       -x  show eXtra debug messages\n");
}

int
main(int argc, char **argv)
{
  int c;
  SaErrorT rv;
#ifdef HPI_A
  SaHpiVersionT hpiVer;
  SaHpiRptInfoT rptinfo;
#endif
  SaHpiSessionIdT sessionid;
  SaHpiRptEntryT rptentry;
  SaHpiEntryIdT rptentryid;
  SaHpiEntryIdT nextrptentryid;
  SaHpiEntryIdT entryid;
  SaHpiEntryIdT nextentryid;
  SaHpiResourceIdT resourceid;
  SaHpiRdrT rdr;
  SaHpiCtrlNumT   ctlnum;
  int j;
  uchar breset;
  uchar bopt;
  uchar fshutdown = 0;
  SaHpiCtrlStateT ctlstate;
 
  printf("%s ver %s\n", argv[0],progver);
  breset = 3; /* hard reset as default */
  bopt = 0;    /* Boot Options default */
  while ( (c = getopt( argc, argv,"rdx?")) != EOF )
     switch(c) {
          case 'd': breset = 0;     break;  /* power down */
          case 'r': breset = 3;     break;  /* hard reset */
          case 'x': fdebug = 1;     break;  /* debug messages */
#ifdef MAYBELATER
          case 'c': breset = 2;     break;  /* power cycle */
          case 'o': fshutdown = 1;  break;  /* shutdown OS */
          case 'n': breset = 4;     break;  /* interrupt (NMI) */
          case 's': bopt   = 1;     break;  /* hard reset to svc part */
#endif
          default:
		Usage(argv[0]);
                exit(1);
  }
  if (fshutdown) breset = 5;     /* soft shutdown option */

#ifdef HPI_A
  rv = saHpiInitialize(&hpiVer);
  if (rv != SA_OK) {
	printf("saHpiInitialize error %d\n",rv);
	exit(-1);
	}
  rv = saHpiSessionOpen(SAHPI_DEFAULT_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
        if (rv == SA_ERR_HPI_ERROR)
           printf("saHpiSessionOpen: error %d, HPI Library not running\n",rv);
        else
	   printf("saHpiSessionOpen error %d\n",rv);
	exit(-1);
	}
  rv = saHpiResourcesDiscover(sessionid);
  if (fdebug) printf("saHpiResourcesDiscover rv = %d\n",rv);
  rv = saHpiRptInfoGet(sessionid,&rptinfo);
  if (fdebug) printf("saHpiRptInfoGet rv = %d\n",rv);
  printf("RptInfo: UpdateCount = %x, UpdateTime = %lx\n",
         rptinfo.UpdateCount, (unsigned long)rptinfo.UpdateTimestamp);
#else
  rv = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
        printf("saHpiSessionOpen: error %d\n",rv);
        exit(-1);
        }
  rv = saHpiDiscover(sessionid);
  if (fdebug) printf("saHpiDiscover error %d\n",rv);
#endif

  /* walk the RPT list */
  rptentryid = SAHPI_FIRST_ENTRY;
  while ((rv == SA_OK) && (rptentryid != SAHPI_LAST_ENTRY))
  {
     rv = saHpiRptEntryGet(sessionid,rptentryid,&nextrptentryid,&rptentry);
     if (rv != SA_OK) printf("RptEntryGet: rv = %d\n",rv);
     if (rv == SA_OK) {
	/* walk the RDR list for this RPT entry */
	entryid = SAHPI_FIRST_ENTRY;
	resourceid = rptentry.ResourceId;
#ifdef HPI_A
	rptentry.ResourceTag.Data[rptentry.ResourceTag.DataLength] = 0;
#endif
	printf("rptentry[%d] resourceid=%d tag: %s\n",
		entryid,resourceid, rptentry.ResourceTag.Data);
	while ((rv == SA_OK) && (entryid != SAHPI_LAST_ENTRY))
	{
		rv = saHpiRdrGet(sessionid,resourceid,
				entryid,&nextentryid, &rdr);
  		if (fdebug) printf("saHpiRdrGet[%d] rv = %d\n",entryid,rv);
		if (rv == SA_OK) {
		   if (rdr.RdrType == SAHPI_CTRL_RDR) { 
			/*type 1 includes Cold Reset Control */
			ctlnum = rdr.RdrTypeUnion.CtrlRec.Num;
			rdr.IdString.Data[rdr.IdString.DataLength] = 0;
			if (fdebug) printf("Ctl[%d]: %d %d %s\n",
				ctlnum, rdr.RdrTypeUnion.CtrlRec.Type,
				rdr.RdrTypeUnion.CtrlRec.OutputType,
				rdr.IdString.Data);
			if ((rdr.RdrTypeUnion.CtrlRec.Type == SAHPI_CTRL_TYPE_DIGITAL) &&
			   (rdr.RdrTypeUnion.CtrlRec.OutputType == SAHPI_CTRL_GENERIC))
			{  /* This is the Reset control */
			   printf("RDR[%d]: %d,%d %s\n",
				rdr.RecordId, 
				rdr.RdrTypeUnion.CtrlRec.Type,
				rdr.RdrTypeUnion.CtrlRec.OutputType,
				rdr.IdString.Data);
#ifdef MAYBELATER
			{
			   SaHpiResetActionT resetact;
			   rv = saHpiResourceResetStateGet(sessionid, 
					resourceid, &resetact);
			   printf("ResetStateGet status = %d, act=%d\n",
					rv,resetact);
			   rv = saHpiResourceResetStateSet(sessionid, 
					resourceid, resetact);
			   printf("ResetStateSet status = %d\n",rv);
			}
#endif

			   ctlstate.Type = SAHPI_CTRL_TYPE_DIGITAL;
			   if (breset == 0) {  /*power off*/ 
			     ctlstate.StateUnion.Digital = SAHPI_CTRL_STATE_OFF;
			     printf("Powering down ... \n");
			   } else {
			     ctlstate.StateUnion.Digital = SAHPI_CTRL_STATE_ON;
			     printf("Resetting ... \n");
			   }
#ifdef HPI_A
                                rv = saHpiControlStateSet(sessionid,
                                           resourceid, ctlnum,&ctlstate);
#else
                                rv = saHpiControlSet(sessionid, resourceid,
                                                ctlnum, SAHPI_CTRL_MODE_MANUAL,
                                                &ctlstate);
#endif
			   printf("Reset status = %d\n",rv);
			   break;
			}
		    }
		    j++;
		    entryid = nextentryid;
		}
	}
	rptentryid = nextrptentryid;
     }
  }
 
  rv = saHpiSessionClose(sessionid);
#ifdef HPI_A
  rv = saHpiFinalize();
#endif

  exit(0);
}
 
/* end hpireset.c */
