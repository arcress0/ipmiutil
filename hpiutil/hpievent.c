/*
 * hpievent.c
 *
 * Author:  Bill Barkley
 * Copyright (c) 2003 Intel Corporation.
 *
 * 05/14/03 
 * 06/06/03 - complete working version
 * 06/20/03 ARCress - added option for Mullins w default=Langley-type sensor
 *                    added progver
 * 02/26/04 ARCress - added general search for any Fan sensor.
 * 03/17/04 ARCress - changed to Temp sensor (always has Lower Major)
 * 03/30/04 ARCress - fixed rv with saHpiSensorThresholdsSet
 * 01/03/05 ARCress - some edits for HPI_A/B
 * 01/07/05 ARCress - clean compile for HPI B, but ThresholdGet returns -1006
 * 04/01/05 ARCress - set UpMajor if not fFan
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
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "SaHpi.h"

char progname[] = "hpievent";
char progver[] = "1.2";
char s_name[] = "80mm Sys Fan (R)";  /*TSRLT2, default*/
char sm_name[] = "Baseboard Fan 2";  /*TSRMT2, Mullins*/
int fFan = 1;
char s_pattn[] = "Temp";  /* else use the first temperature SDR found */
char *sensor_name;
int sensor_name_len = 0;
int fdebug = 0;
int fxdebug = 0;
int fsensor = 0;
int slist = 0;
int i,j,k = 0;
char inbuff[1024];
char outbuff[256];
SaHpiUint32T buffersize;
SaHpiUint32T actualsize;
SaHpiTextBufferT *strptr;
#ifdef HPI_A
SaHpiInventoryDataT *inv;
SaHpiInventGeneralDataT *dataptr;
SaHpiSensorEvtEnablesT enables1;
SaHpiSensorEvtEnablesT enables2;
#endif

char *rdrtypes[5] = {
	"None    ",
	"Control ",
	"Sensor  ",
	"Invent  ",
	"Watchdog"};

#define HPI_NSEC_PER_SEC 1000000000LL
#define NSU   32
char *units[NSU] = {
	"units", "deg C",     "deg F",     "deg K",     "volts", "amps",
	"watts", "joules",    "coulombs",  "va",        "nits",  "lumen",
	"lux",   "candela",   "kpa",       "psi",     "newton",  "cfm",
	"rpm",   "Hz",        "us",        "ms",      "sec",     "min",
	"hours", "days",      "weeks",     "mil",     "in",     "ft",
	"mm",    "cm"
};

#ifdef NOT
void
fixstr(SaHpiTextBufferT *strptr)
{ 
	size_t datalen;
	outbuff[0] = 0;
	if (strptr == NULL) return;
	datalen = strptr->DataLength;
	if (datalen != 0) {
		strncpy ((char *)outbuff, (char *)strptr->Data, datalen);
		outbuff[datalen] = 0;
	}
}
#endif

static void ShowThresh(
   SaHpiSensorThresholdsT *sensbuff, SaHpiSensorThdMaskT ReadThold)
{
      printf( "    Supported Thresholds:\n");
#ifdef HPI_A
      if (sensbuff->LowCritical.ValuesPresent & SAHPI_SRF_INTERPRETED)
         printf( "      Lower Critical Threshold: %5.2f\n",
               sensbuff->LowCritical.Interpreted.Value.SensorFloat32);
      if (sensbuff->LowMajor.ValuesPresent & SAHPI_SRF_INTERPRETED)
         printf( "      Lower Major Threshold: %5.2f\n",
               sensbuff->LowMajor.Interpreted.Value.SensorFloat32);
      if (sensbuff->LowMinor.ValuesPresent & SAHPI_SRF_INTERPRETED)
         printf( "      Lower Minor Threshold: %5.2f\n",
               sensbuff->LowMinor.Interpreted.Value.SensorFloat32);

      if (sensbuff->UpCritical.ValuesPresent & SAHPI_SRF_INTERPRETED)
         printf( "      Upper Critical Threshold: %5.2f\n",
               sensbuff->UpCritical.Interpreted.Value.SensorFloat32);
      if (sensbuff->UpMajor.ValuesPresent & SAHPI_SRF_INTERPRETED)
         printf( "      Upper Major Threshold: %5.2f\n",
               sensbuff->UpMajor.Interpreted.Value.SensorFloat32);
      if (sensbuff->UpMinor.ValuesPresent & SAHPI_SRF_INTERPRETED)
         printf( "      Upper Minor Threshold: %5.2f\n",
               sensbuff->UpMinor.Interpreted.Value.SensorFloat32);

      if (sensbuff->PosThdHysteresis.ValuesPresent & SAHPI_SRF_RAW)
         printf( "      Positive Threshold Hysteresis in RAW\n");
      if (sensbuff->PosThdHysteresis.ValuesPresent & SAHPI_SRF_INTERPRETED)
         printf( "      Positive Threshold Hysteresis: %5.2f\n",
               sensbuff->PosThdHysteresis.Interpreted.Value.SensorFloat32);
      if (sensbuff->NegThdHysteresis.ValuesPresent & SAHPI_SRF_RAW)
         printf( "      Negative Threshold Hysteresis in RAW\n");
      if (sensbuff->NegThdHysteresis.ValuesPresent & SAHPI_SRF_INTERPRETED)
         printf( "      Negative Threshold Hysteresis: %5.2f\n",
               sensbuff->NegThdHysteresis.Interpreted.Value.SensorFloat32);
#else
      if ( ReadThold & SAHPI_STM_LOW_CRIT ) {
         printf( "      Lower Critical Threshold: %5.2f\n",
               sensbuff->LowCritical.Value.SensorFloat64);
	}
      if ( ReadThold & SAHPI_STM_LOW_MAJOR ) {
         printf( "      Lower Major Threshold: %5.2f\n",
               sensbuff->LowMajor.Value.SensorFloat64);
	}
      if ( ReadThold & SAHPI_STM_LOW_MINOR ) {
         printf( "      Lower Minor Threshold: %5.2f\n",
               sensbuff->LowMinor.Value.SensorFloat64);
	}

      if ( ReadThold & SAHPI_STM_UP_CRIT ) {
         printf( "      Upper Critical Threshold: %5.2f\n",
               sensbuff->UpCritical.Value.SensorFloat64);
	}
      if ( ReadThold & SAHPI_STM_UP_MAJOR ) {
         printf( "      Upper Major Threshold: %5.2f\n",
               sensbuff->UpMajor.Value.SensorFloat64);
	}
      if ( ReadThold & SAHPI_STM_UP_MINOR ) {
         printf( "      Upper Minor Threshold: %5.2f\n",
               sensbuff->UpMinor.Value.SensorFloat64);
	}

      if ( ReadThold & SAHPI_STM_UP_HYSTERESIS ) {
         printf( "      Positive Threshold Hysteresis: %5.2f\n",
               sensbuff->PosThdHysteresis.Value.SensorFloat64);
	}
      if ( ReadThold & SAHPI_STM_LOW_HYSTERESIS ) {
         printf( "      Negative Threshold Hysteresis: %5.2f\n",
               sensbuff->NegThdHysteresis.Value.SensorFloat64);
	}
#endif
      printf( "\n");
}

static
SaErrorT DoEvent(
   SaHpiSessionIdT sessionid,
   SaHpiResourceIdT resourceid,
   SaHpiSensorRecT *sensorrec )
{
   SaHpiSensorNumT sensornum;
   SaHpiSensorReadingT reading;
   SaHpiSensorThresholdsT senstbuff1;
   SaHpiSensorThresholdsT senstbuff2;
   SaErrorT rv, rv2;
#ifdef HPI_A
   SaHpiSensorReadingT conv_reading;
#else
   SaHpiEventStateT  evtstate; 
#endif

   SaHpiTimeoutT timeout = (SaHpiInt64T)(12 * HPI_NSEC_PER_SEC); /* 12 seconds*/
   SaHpiEventT event;
   SaHpiRptEntryT rptentry;
   SaHpiRdrT rdr;
   char *unit;
   int eventflag = 0;
   int i;

   sensornum = sensorrec->Num;

/* Get current sensor reading */
#ifdef HPI_A
   rv = saHpiSensorReadingGet( sessionid, resourceid, sensornum, &reading);
#else
   rv = saHpiSensorReadingGet(sessionid,resourceid, sensornum, &reading,&evtstate);
#endif
   if (rv != SA_OK)  {
	printf( "saHpiSensorReadingGet error %d\n",rv);
/*	printf("ReadingGet ret=%d\n", rv); */
	return(rv);
   }

#ifdef HPI_A
   /* HPI A has both interpreted and raw.  HPI B has only interpreted. */
   if ((reading.ValuesPresent & SAHPI_SRF_INTERPRETED) == 0 &&
       (reading.ValuesPresent & SAHPI_SRF_RAW)) {
        /* only try convert if intrepreted not available. */
        rv = saHpiSensorReadingConvert(sessionid, resourceid, sensornum,
					&reading, &conv_reading);
        if (rv != SA_OK) {
		printf("raw=%x conv_ret=%d\n", reading.Raw, rv);
	     /* printf("conv_rv=%s\n", decode_error(rv)); */
		return(rv);
        } else {
	   reading.Interpreted.Type = conv_reading.Interpreted.Type;
	   reading.Interpreted.Value.SensorUint32 = 
		   conv_reading.Interpreted.Value.SensorUint32;
	}
   }
#endif

   /* Determine units of interpreted reading */
   i = sensorrec->DataFormat.BaseUnits;
   if (i > NSU) i = 0;
   unit = units[i];
   /* We know that reading.Type is float for the chosen sensor. */
#ifdef HPI_A
   printf(" = %05.2f %s \n", 
	reading.Interpreted.Value.SensorFloat32, unit);
#else
   printf(" = %05.2f %s \n", reading.Value.SensorFloat64, unit);
#endif

/* Retrieve current threshold setings, twice */
/* once for backup and once for modification */

   /* Get backup copy */
   rv = saHpiSensorThresholdsGet(
	sessionid, resourceid, sensornum, &senstbuff1);
   if (rv != SA_OK) {
	printf( "saHpiSensorThresholdsGet error %d\n",rv); return(rv); }
	/* OpenHPI 2.0.0 ipmi plugin returns -1006 timeout here, why ???? */

   /* Get modification copy */
   rv = saHpiSensorThresholdsGet(
	sessionid, resourceid, sensornum, &senstbuff2);
   if (rv != SA_OK) {
	printf( "saHpiSensorThresholdsGet error %d\n",rv); return(rv); }

   /* Display current thresholds */ 
   if (rv == SA_OK) {
     printf( "   Current\n");
     ShowThresh( &senstbuff2, sensorrec->ThresholdDefn.ReadThold);
   }

/* Set new threshold to current reading + 10% */
#ifdef HPI_A
   if (fFan) {
      senstbuff2.LowMajor.Interpreted.Value.SensorFloat32 =
           reading.Interpreted.Value.SensorFloat32 * (SaHpiFloat32T)1.10;
   } else {
      senstbuff2.UpMinor.Interpreted.Value.SensorFloat32 =
           reading.Interpreted.Value.SensorFloat32 * (SaHpiFloat32T)0.90;
      senstbuff2.UpMajor.Interpreted.Value.SensorFloat32 =
           reading.Interpreted.Value.SensorFloat32 * (SaHpiFloat32T)0.05;
   }
   if (fdebug) {
	printf( "ValuesPresent = %x\n", senstbuff2.LowMajor.ValuesPresent);
	printf( "Values Mask   = %x\n", (SAHPI_SRF_RAW));
   }
   senstbuff2.LowMajor.ValuesPresent =
        senstbuff2.LowMajor.ValuesPresent ^ (SAHPI_SRF_RAW);
   if (fdebug) 
	printf( "ValuesPresent = %x\n", senstbuff2.LowMajor.ValuesPresent);
#else
   /* If fan type, set LowMajor, if temp type, set HighMinor. */
   if (fFan) {
      senstbuff2.LowMajor.Value.SensorFloat64 =
		   reading.Value.SensorFloat64 * (SaHpiFloat64T)1.10; 
   } else {  /* usually Temperature sensor */
      senstbuff2.UpMinor.Value.SensorFloat64 =
		   reading.Value.SensorFloat64 * (SaHpiFloat64T)0.90;
      senstbuff2.UpMajor.Value.SensorFloat64 =
		   reading.Value.SensorFloat64 * (SaHpiFloat64T)0.95;
   }
#endif

   /* Display new current thresholds */ 
   if (rv == SA_OK) {
      printf( "   New\n");
      ShowThresh( &senstbuff2, sensorrec->ThresholdDefn.ReadThold);
   }

#ifdef HPI_A
   /* See what Events are Enabled */
   rv = saHpiSensorEventEnablesGet(
		sessionid, resourceid, sensornum, &enables1);
   if (rv != SA_OK) {
	printf( "saHpiSensorEventEnablesGet error %d\n",rv); return(rv); }

   printf( "Sensor Event Enables: \n");
   printf( "  Sensor Status = %x\n", enables1.SensorStatus);
   printf( "  Assert Events = %x\n", enables1.AssertEvents);
   printf( "  Deassert Events = %x\n", enables1.DeassertEvents);
#endif

/* 
   enables1.AssertEvents = 0x0400;
   enables1.DeassertEvents = 0x0400;
   rv = saHpiSensorEventEnablesSet(
		sessionid, resourceid, sensornum, &enables1);
   if (rv != SA_OK) return(rv);
*/


/************************
   Temporary exit */
/*
 return(rv);
*/

/* Subscribe to New Events, only */
printf( "Subscribe to events\n");
#ifdef HPI_A
   rv = saHpiSubscribe( sessionid, (SaHpiBoolT)0 );
#else
   rv = saHpiSubscribe( sessionid );
#endif
   if (rv != SA_OK) {
	printf( "saHpiSubscribe error %d\n",rv); return(rv); }

/* Set new thresholds */
printf( "Set new thresholds\n");

   rv = saHpiSensorThresholdsSet(
	sessionid, resourceid, sensornum, &senstbuff2);
   if (rv != SA_OK) {
	printf( "saHpiSensorThresholdsSet error %d\n",rv); 
	/* rv2 = saHpiUnsubscribe( sessionid ); 
        if (fxdebug) printf("saHpiUnsubscribe, rv = %d\n",rv2);
	return(rv); */
	}

/* Go wait on event to occur */
printf( "Go and get the event\n");
   eventflag = 0;
   while ( eventflag == 0) {
#ifdef HPI_A
     rv = saHpiEventGet( sessionid, timeout, &event, &rdr, &rptentry );
#else
     timeout = SAHPI_TIMEOUT_BLOCK;
     timeout = SAHPI_TIMEOUT_IMMEDIATE;  /* use this if not threaded */
     rv = saHpiEventGet( sessionid, timeout, &event, &rdr, &rptentry, NULL);
#endif
     if (rv != SA_OK) { 
	if (rv == SA_ERR_HPI_TIMEOUT) {
	  printf( "Time expired during EventGet - Test FAILED\n");
            /* Reset to the original thresholds */
            printf( "Reset thresholds\n");
               rv = saHpiSensorThresholdsSet(
                    sessionid, resourceid, sensornum, &senstbuff1);
               if (rv != SA_OK) return(rv);

            /* Re-read threshold values */
               rv = saHpiSensorThresholdsGet(
                    sessionid, resourceid, sensornum, &senstbuff2);
               if (rv != SA_OK) return(rv);
	  return(rv);
        } else {
	  printf( "Error %d during EventGet - Test FAILED\n",rv);
	  return(rv);
	}
     }

/* Decode the event information */
printf( "Decode event info\n");
     if (event.EventType == SAHPI_ET_SENSOR) {
       printf( "Sensor # = %2d  Severity = %2x\n", 
	    event.EventDataUnion.SensorEvent.SensorNum, event.Severity );
       if (event.EventDataUnion.SensorEvent.SensorNum == sensornum) {
	 eventflag = 1;
	 printf( "Got it - Test PASSED\n");
       }
     }
   }
/* Reset to the original thresholds */
printf( "Reset thresholds\n");
   rv = saHpiSensorThresholdsSet(
	sessionid, resourceid, sensornum, &senstbuff1);
   if (rv != SA_OK) {
	printf( "saHpiSensorThresholdsSet error %d\n",rv); 
        rv2 = saHpiUnsubscribe( sessionid );
        if (fxdebug) printf("saHpiUnsubscribe, rv = %d\n",rv2);
	return(rv);
   } else {
	/* Re-read threshold values */
	rv = saHpiSensorThresholdsGet( sessionid, resourceid, sensornum, 
					&senstbuff2);
	if (rv != SA_OK) {
	   printf( "saHpiSensorThresholdsGet error %d\n",rv); 
	} else {  /* rv == SA_OK */
	   /* Display reset thresholds */ 
           printf( "   Reset\n");
	   ShowThresh( &senstbuff2, sensorrec->ThresholdDefn.ReadThold);
	}
   }  /*end-else SA_OK*/

/* Unsubscribe to future events */
printf( "Unsubscribe\n");
   rv2 = saHpiUnsubscribe( sessionid );
   if (fxdebug) printf("saHpiUnsubscribe, rv = %d\n",rv2);

   return(rv);
}  /*end DoEvent*/

/*
 * findmatch
 * returns offset of the match if found, or -1 if not found.
 */
static int
findmatch(char *buffer, int sbuf, char *pattern, int spattern, char figncase)
{
    int c, j, imatch;
    j = 0;
    imatch = 0;
    for (j = 0; j < sbuf; j++) {
        if (sbuf - j < spattern && imatch == 0) return (-1);
        c = buffer[j];
        if (c == pattern[imatch]) {
            imatch++;
            if (imatch == spattern) return (++j - imatch);
        } else if (pattern[imatch] == '?') {  /*wildcard char*/
            imatch++;
            if (imatch == spattern) return (++j - imatch);
        } else if (figncase == 1) {
            if ((c & 0x5f) == (pattern[imatch] & 0x5f)) {
                imatch++;
                if (imatch == spattern) return (++j - imatch);
            } else
                imatch = 0;
        } else
            imatch = 0;
    }
    return (-1);
}                               /*end findmatch */

int
main(int argc, char **argv)
{
  int c;
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
  int firstpass = 1;
  SaErrorT rv;

  printf("%s  ver %s\n", progname,progver);
  sensor_name = (char *)strdup(s_name);
  while ( (c = getopt( argc, argv,"ms:xz?")) != EOF )
  switch(c)
  {
    case 'z': fxdebug = 1; /* fall thru to include next setting */
    case 'x': fdebug = 1; break;
    case 'm': 
	      sensor_name = (char *)strdup(sm_name);
	      fFan = 0;  /*assume not a Fan */
	      break;
    case 's':
	  fsensor = 1;
          if (optarg) {
	    sensor_name = (char *)strdup(optarg);
	  }
          break;
    default:
          printf("Usage: %s [-xm] [-s sensor_descriptor]\n", progname);
          printf("   -s  Sensor descriptor\n");
          printf("   -m  use Mullins sensor descriptor\n");
          printf("   -x  Display debug messages\n");
          printf("   -z  Display extra debug messages\n");
          exit(1);
  }
#ifdef HPI_A
  inv = (SaHpiInventoryDataT *)&inbuff[0];

  rv = saHpiInitialize(&hpiVer);

  if (rv != SA_OK) {
    printf("saHpiInitialize error %d\n",rv);
    exit(-1);
  }
  rv = saHpiSessionOpen(SAHPI_DEFAULT_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
    printf("saHpiSessionOpen error %d\n",rv);
    exit(-1);
  }
 
  if (fdebug) printf("Starting Discovery ...\n");
  rv = saHpiResourcesDiscover(sessionid);
  if (fdebug) printf("saHpiResourcesDiscover rv = %d\n",rv);

  rv = saHpiRptInfoGet(sessionid,&rptinfo);
  if (fxdebug) printf("saHpiRptInfoGet rv = %d\n",rv);
  if (fdebug) printf("RptInfo: UpdateCount = %x, UpdateTime = %lx\n",
         rptinfo.UpdateCount, (unsigned long)rptinfo.UpdateTimestamp);
 
#else
  rv = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
    printf("saHpiSessionOpen error %d\n",rv);
    exit(-1);
  }
  if (fdebug) printf("Starting Discovery ...\n");
  rv = saHpiDiscover(sessionid);
  if (fdebug) printf("saHpiResourcesDiscover rv = %d\n",rv);

  // rv = saHpiDomainInfoGet(sessionid,&rptinfo);
  {
        /*
         * If OpenHPI, we need to wait extra time before doing
         * ThresholdsGet because its discovery isn't really done.
         */
        sleep(5);
  }
#endif

  /* walk the RPT list */
  rptentryid = SAHPI_FIRST_ENTRY;
  while ((rv == SA_OK) && (rptentryid != SAHPI_LAST_ENTRY))
  {
    rv = saHpiRptEntryGet(sessionid,rptentryid,&nextrptentryid,&rptentry);
    if (rv == SA_OK)
    {
      /* walk the RDR list for this RPT entry */
      entryid = SAHPI_FIRST_ENTRY;
#ifdef HPI_A
      rptentry.ResourceTag.Data[rptentry.ResourceTag.DataLength] = 0;
#endif
      resourceid = rptentry.ResourceId;
      
      if (fdebug) printf("rptentry[%d] resourceid=%d\n", entryid,resourceid);

      printf("Resource Tag: %s\n", rptentry.ResourceTag.Data);
      while ((rv == SA_OK) && (entryid != SAHPI_LAST_ENTRY))
      {
        rv = saHpiRdrGet(sessionid,resourceid, entryid,&nextentryid, &rdr);

  	if (fxdebug) printf("saHpiRdrGet[%d] rv = %d\n",entryid,rv);

	if (rv == SA_OK)
	{
	  if (rdr.RdrType == SAHPI_SENSOR_RDR)
	  { 
	    /*type 2 includes sensor and control records*/
	    rdr.IdString.Data[rdr.IdString.DataLength] = 0;
#ifdef OLD
	    if (strncmp(rdr.IdString.Data, sensor_name,
		rdr.IdString.DataLength) == 0)
#else
	    if (findmatch(rdr.IdString.Data,rdr.IdString.DataLength,
		sensor_name, strlen(sensor_name),0) >= 0)
#endif
	    {
	      printf( "%02d %s\t", rdr.RecordId, rdr.IdString.Data);
	      rv = DoEvent( sessionid, resourceid, &rdr.RdrTypeUnion.SensorRec);
	      if (rv != SA_OK)
	        printf( "Returned Error from DoEvent: rv=%d\n", rv);
	      break;  /* done with rdr loop */
	    }
	  } 
	  if (rv != SA_OK)
	      printf( "Returned HPI Error: rv=%d\n", rv);
	  entryid = nextentryid;
        }
	if (firstpass && entryid == SAHPI_LAST_ENTRY) {
		/* Not found yet, so try again, looking for any Temp */
		sensor_name = s_pattn;
		entryid = SAHPI_FIRST_ENTRY;
		firstpass = 0;
		fFan = 0;  /*Temp not Fan*/
	}
      }  /*while rdr loop */
      rptentryid = nextrptentryid;
    }
  }  /*while rpt loop*/
  rv = saHpiSessionClose(sessionid);
#ifdef HPI_A
  rv = saHpiFinalize();
#endif
  exit(0);
}
 /* end hpievent.c */
