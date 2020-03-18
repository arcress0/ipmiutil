/*
 * hpialarmpanel.c
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2003 Intel Corporation.
 *
 * 04/15/03 Andy Cress - created
 * 04/17/03 Andy Cress - mods for resourceid, first good run
 * 04/23/03 Andy Cress - first run with good ControlStateGet   
 * 04/24/03 Andy Cress - v0.5 with good ControlStateSet
 * 04/29/03 Andy Cress - v0.6 changed control.oem values
 * 06/06/03 Andy Cress - v1.0 check for Analog States
 * 02/23/04 Andy Cress - v1.1 add checking/setting disk LEDs
 * 10/13/04 Andy Cress - v1.2 add ifdefs for HPI_A & HPI_B, added -d/raw
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
#define  SAHPI_OEM_ALARM_LED 0x10 
#define  SAHPI_OEM_DISK_LED  0x20 
#ifdef HPI_A
char *progver  = "1.2 HPI-A";
#else
char *progver  = "1.2 HPI-B";
#endif
char fdebug = 0;
char *states[3] = {"off", "ON ", "unknown" };
uchar fsetid = 0;
uchar fid = 0;
#define NLEDS  6
struct { 
   uchar fset;
   uchar val;
} leds[NLEDS] = {  /* rdr.RdrTypeUnion.CtrlRec.Oem is an index for this */
{/*pwr*/ 0, 0},
{/*crt*/ 0, 0},
{/*maj*/ 0, 0},
{/*min*/ 0, 0},
{/*diska*/ 0, 0},
{/*diskb*/ 0, 0} };


int
main(int argc, char **argv)
{
  int c;
  SaErrorT rv;
  SaHpiSessionIdT sessionid;
#ifdef HPI_A
  SaHpiVersionT hpiVer;
  SaHpiRptInfoT rptinfo;
#endif
  SaHpiRptEntryT rptentry;
  SaHpiEntryIdT rptentryid;
  SaHpiEntryIdT nextrptentryid;
  SaHpiEntryIdT entryid;
  SaHpiEntryIdT nextentryid;
  SaHpiResourceIdT resourceid;
  SaHpiRdrT rdr;
  SaHpiCtrlTypeT  ctltype;
  SaHpiCtrlNumT   ctlnum;
  SaHpiCtrlStateT ctlstate;
  int raw_val = 0;
  int j;
  uchar b = 0;

  printf("%s ver %s\n", argv[0],progver);
  while ( (c = getopt( argc, argv,"rxa:b:c:m:n:p:i:d:o?")) != EOF )
     switch(c) {
          
	case 'c': b = atoi(optarg);      /* set crit alarm value */
		  leds[1].fset = 1; 
		  leds[1].val = b;
                  break;
	case 'm': b = atoi(optarg);      /* set major alarm value */
		  leds[2].fset = 1; 
		  leds[2].val = b;
                  break;
	case 'n': b = atoi(optarg);      /* set minor alarm value */
		  leds[3].fset = 1; 
		  leds[3].val = b;
                  break;
	case 'a': b = atoi(optarg);      /* set disk a fault led */
		  leds[4].fset = 1; 
		  leds[4].val = b;
                  break;
	case 'b': b = atoi(optarg);      /* set disk b fault led */
		  leds[5].fset = 1; 
		  leds[5].val = b;
                  break;
	case 'p': b = atoi(optarg);      /* set power alarm value */
		  leds[0].fset = 1; 
		  leds[0].val = b;
                  break;
	case 'i': fid = atoi(optarg);     /* set chassis id on/off */
		  fsetid = 1;
                  break;
	case 'd': raw_val = atoi(optarg);  /* set raw alarm byte  */
                  break;
	case 'o': fsetid=1; fid=0; 	  /* set all alarms off */
		  for (j = 0; j < NLEDS; j++) { 
			leds[j].fset = 1; 
			leds[j].val = 0; 
		  }
                  break;
	case 'x': fdebug = 1;     break;  /* debug messages */
	default:
                printf("Usage: %s [-a -b -c -i -m -n -p -o -x]\n", argv[0]);
                printf(" where -c1  sets Critical Alarm on\n");
                printf("       -c0  sets Critical Alarm off\n");
                printf("       -m1  sets Major Alarm on\n");
                printf("       -m0  sets Major Alarm off\n");
                printf("       -n1  sets Minor Alarm on\n");
                printf("       -n0  sets Minor Alarm off\n");
                printf("       -p1  sets Power Alarm on\n");
                printf("       -p0  sets Power Alarm off\n");
                printf("       -i5  sets Chassis ID on for 5 sec\n");
                printf("       -i0  sets Chassis ID off\n");
                printf("       -a1  sets Disk A fault on\n");
                printf("       -a0  sets Disk A fault off\n");
                printf("       -b1  sets Disk B fault on\n");
                printf("       -b0  sets Disk B fault off\n");
#ifndef HPI_A
                printf("       -d[byte] sets raw Alarm byte\n");
#endif
                printf("       -o   sets all Alarms off\n");
                printf("       -x   show eXtra debug messages\n");
		exit(1);
     }

#ifdef HPI_A
  rv = saHpiInitialize(&hpiVer);
  if (rv != SA_OK) {
	printf("saHpiInitialize error %d\n",rv);
	exit(-1);
	}
  if (fdebug) printf("Initialize complete, rv = %d\n",rv);
  rv = saHpiSessionOpen(SAHPI_DEFAULT_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
        if (rv == SA_ERR_HPI_ERROR)
           printf("saHpiSessionOpen: error %d, HPI daemon not running\n",rv);
        else
	   printf("saHpiSessionOpen error %d\n",rv);
	exit(-1);
	}
  rv = saHpiResourcesDiscover(sessionid);
  if (fdebug) printf("saHpiResourcesDiscover complete, rv = %d\n",rv);
  rv = saHpiRptInfoGet(sessionid,&rptinfo);
  if (fdebug) printf("saHpiRptInfoGet rv = %d\n",rv);
  printf("RptInfo: UpdateCount = %x, UpdateTime = %lx\n",
         rptinfo.UpdateCount, (unsigned long)rptinfo.UpdateTimestamp);
#else   /* HPI B.01.01 */
  rv = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
	printf("saHpiSessionOpen error %d\n",rv);
	exit(-1);
	}
  rv = saHpiDiscover(sessionid);
  if (fdebug) printf("saHpiDiscover complete, rv = %d\n",rv);

  if (fsetid) {   /* HPI B.01.01 Identify logic */
	printf("Setting ID led to %d sec\n", fid);
	/* do saHpiIdentify function */
	printf("Not yet supported\n");
  }
#endif
 
  /* walk the RPT list */
  rptentryid = SAHPI_FIRST_ENTRY;
  while ((rv == SA_OK) && (rptentryid != SAHPI_LAST_ENTRY))
  {
     rv = saHpiRptEntryGet(sessionid,rptentryid,&nextrptentryid,&rptentry);
     if (rv != SA_OK) printf("RptEntryGet: rv = %d\n",rv);
     if (rv == SA_OK) {
	/* Walk the RDR list for this RPT entry */
	entryid = SAHPI_FIRST_ENTRY;
	resourceid = rptentry.ResourceId;
#ifdef HPI_A
	 rptentry.ResourceTag.Data[rptentry.ResourceTag.DataLength] = 0;
#else
	/* 
	 * Don't stringify here, since OpenHPI returns a valid string, but
	 * a DataLength of zero here (for processor, bios).
	 */
#endif
	//if (fdebug) 
	   printf("rptentry[%d] resourceid=%d tag: %s\n",
		  rptentryid, resourceid, rptentry.ResourceTag.Data);
	while ((rv == SA_OK) && (entryid != SAHPI_LAST_ENTRY))
	{
		rv = saHpiRdrGet(sessionid,resourceid,
				entryid,&nextentryid, &rdr);
  		if (fdebug) printf("saHpiRdrGet[%d] rv = %d\n",entryid,rv);
		if (rv == SA_OK) {
		   if (rdr.RdrType == SAHPI_CTRL_RDR) { 
			/*type 1 includes alarm LEDs*/
			ctlnum = rdr.RdrTypeUnion.CtrlRec.Num;
			rdr.IdString.Data[rdr.IdString.DataLength] = 0;
			if (fdebug) printf("Ctl[%d]: %d %d %s\n",
				ctlnum, rdr.RdrTypeUnion.CtrlRec.Type,
				rdr.RdrTypeUnion.CtrlRec.OutputType,
				rdr.IdString.Data);
			rv = saHpiControlTypeGet(sessionid,resourceid,
					ctlnum,&ctltype);
  			if (fdebug) printf("saHpiControlTypeGet[%d] rv = %d, type = %d\n",ctlnum,rv,ctltype);
#ifdef HPI_A
			rv = saHpiControlStateGet(sessionid,resourceid,
					ctlnum,&ctlstate);
#else
			rv = saHpiControlGet(sessionid, resourceid, ctlnum,
					NULL, &ctlstate);
#endif
  			if (fdebug) 
			   printf("saHpiControlStateGet[%d] rv = %d v = %x\n",
				ctlnum,rv,ctlstate.StateUnion.Digital);
			printf("RDR[%d]: ctltype=%d:%d oem=%02x %s  \t",
				rdr.RecordId, 
				rdr.RdrTypeUnion.CtrlRec.Type,
				rdr.RdrTypeUnion.CtrlRec.OutputType,
				rdr.RdrTypeUnion.CtrlRec.Oem,
				rdr.IdString.Data);
			if (rv == SA_OK) {
			   if (ctlstate.Type == SAHPI_CTRL_TYPE_ANALOG)
			        b = 2;  /*Identify*/
			   else {
			      b = ctlstate.StateUnion.Digital;
			      if (b > 2) b = 2; 
			   }
			   printf("state = %s\n",states[b]);
			} else { printf("\n"); }

			if (rdr.RdrTypeUnion.CtrlRec.Type == SAHPI_CTRL_TYPE_ANALOG &&
			    rdr.RdrTypeUnion.CtrlRec.OutputType == SAHPI_CTRL_LED) {
			    /* This is a Chassis Identify */
			    if (fsetid) {
				printf("Setting ID led to %d sec\n", fid);
				ctlstate.Type = SAHPI_CTRL_TYPE_ANALOG;
				ctlstate.StateUnion.Analog = fid;
#ifdef HPI_A
				rv = saHpiControlStateSet(sessionid,
					   resourceid, ctlnum,&ctlstate);
#else
				rv = saHpiControlSet(sessionid, resourceid,
						ctlnum, SAHPI_CTRL_MODE_MANUAL,
						&ctlstate);
#endif
				printf("saHpiControlStateSet[%d] rv = %d\n",ctlnum,rv);
			    }
			} else 
			if (rdr.RdrTypeUnion.CtrlRec.Type == SAHPI_CTRL_TYPE_DIGITAL &&
			    (rdr.RdrTypeUnion.CtrlRec.Oem & 0xf0) == SAHPI_OEM_ALARM_LED &&
			    rdr.RdrTypeUnion.CtrlRec.OutputType == SAHPI_CTRL_LED) {
				/* this is an alarm LED */
				b = (uchar)rdr.RdrTypeUnion.CtrlRec.Oem & 0x0f;
				if ((b < NLEDS) && leds[b].fset) {
				   printf("Setting alarm led %d to %d\n",b,leds[b].val);
				   if (leds[b].val == 0) 
					ctlstate.StateUnion.Digital = SAHPI_CTRL_STATE_OFF;
				   else 
					ctlstate.StateUnion.Digital = SAHPI_CTRL_STATE_ON;
#ifdef HPI_A
				   rv = saHpiControlStateSet(sessionid,
						resourceid, ctlnum,&ctlstate);
#else
				   rv = saHpiControlSet(sessionid, resourceid,
						ctlnum, SAHPI_CTRL_MODE_MANUAL,
						&ctlstate);
#endif
  				   /* if (fdebug)  */
					printf("saHpiControlStateSet[%d] rv = %d\n",ctlnum,rv);
				}
			}
			else if (rdr.RdrTypeUnion.CtrlRec.Type == SAHPI_CTRL_TYPE_DIGITAL &&
			    (rdr.RdrTypeUnion.CtrlRec.Oem & 0xf0) == SAHPI_OEM_DISK_LED &&
			    rdr.RdrTypeUnion.CtrlRec.OutputType == SAHPI_CTRL_LED) {
				/* this is a disk LED */
				b = (uchar)rdr.RdrTypeUnion.CtrlRec.Oem & 0x0f;
				if ((b < NLEDS) && leds[b].fset) {
				   printf("Setting disk led %d to %d\n",b,leds[b].val);
				   if (leds[b].val == 0) 
					ctlstate.StateUnion.Digital = SAHPI_CTRL_STATE_OFF;
				   else 
					ctlstate.StateUnion.Digital = SAHPI_CTRL_STATE_ON;
#ifdef HPI_A
				   rv = saHpiControlStateSet(sessionid,
						resourceid, ctlnum,&ctlstate);
#else
				   rv = saHpiControlSet(sessionid, resourceid,
						ctlnum, SAHPI_CTRL_MODE_MANUAL,
						&ctlstate);
#endif
				   printf("saHpiControlStateSet[%d] rv = %d\n",ctlnum,rv);
				}
			}
#ifndef HPI_A
                        else if (rdr.RdrTypeUnion.CtrlRec.Type == SAHPI_CTRL_TYPE_OEM &&
                                 rdr.RdrTypeUnion.CtrlRec.OutputType == SAHPI_CTRL_FRONT_PANEL_LOCKOUT) {
				/* This is a raw control for alarm panel, 
				 * but HPI never populates an RDR for this. */
                                printf("Found raw alarm control\n");
                                if (raw_val != 0) {
                                ctlstate.Type       = SAHPI_CTRL_TYPE_OEM;
                                ctlstate.StateUnion.Oem.BodyLength = 1;
                                ctlstate.StateUnion.Oem.Body[0]    = raw_val;
                                printf("Set raw alarm control to %x\n",raw_val);
                                rv = saHpiControlSet(sessionid, resourceid, ctlnum,
                                                     SAHPI_CTRL_MODE_MANUAL, &ctlstate);
                                printf("saHpiControlSet[%d] raw, rv = %d\n",ctlnum,rv);
				}
                        }
#endif
			rv = SA_OK;  /* ignore errors & continue */
		    }
		    j++;
		    entryid = nextentryid;
		}
	}  /* RdrGet loop */
	rptentryid = nextrptentryid;
     }
  }  /* RptEntryGet loop */
 
  rv = saHpiSessionClose(sessionid);
#ifdef HPI_A
  rv = saHpiFinalize();
#endif

  exit(0);
  return(0);
}
 
/*-----------Sample output-----------------------------------
hpialarmpanel ver 0.6
RptInfo: UpdateCount = 5, UpdateTime = 8a2dc6c0
rptentry[0] resourceid=1 tag: Mullins
RDR[45]: ctltype=2:1 oem=0  Chassis Identify Control
RDR[48]: ctltype=0:1 oem=10 Front Panel Power Alarm LED         state = off
RDR[51]: ctltype=0:1 oem=13 Front Panel Minor Alarm LED         state = ON
RDR[46]: ctltype=0:0 oem=0  Cold Reset Control
RDR[49]: ctltype=0:1 oem=11 Front Panel Critical Alarm LED      state = off
RDR[50]: ctltype=0:1 oem=12 Front Panel Major Alarm LED         state = off
 *-----------------------------------------------------------*/
/* end hpialarmpanel.c */
