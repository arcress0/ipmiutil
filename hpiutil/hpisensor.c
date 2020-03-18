/*
 * hpisensor.c
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2003 Intel Corporation.
 *
 * 04/15/03 Andy Cress - created
 * 04/17/03 Andy Cress - mods for resourceid, first good run 
 * 04/28/03 Andy Cress - adding Convert
 * 05/02/03 Andy Cress v0.6 more Convert changes in ShowSensor (e.g. units)
 * 06/06/03 Andy Cress v0.7 add more units, and buffer type
 * 06/09/03 Andy Cress v0.8 added more for Compact Sensors
 * 06/26/03 Andy Cress v1.0 added -t for thresholds
 * 02/19/04 Andy Cress v1.1 recognize common errors for Compact & mBMC sensors.
 * 10/14/04 Andy Cress v1.2 added HPI_A/HPI_B logic
 * 01/07/05 Andy Cress v1.3 added HPI_B sleep for OpenHPI bug
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
#include <string.h>
#include <unistd.h>
#include "SaHpi.h"

#ifdef HPI_A
char progver[] = "1.3 HPI-A";
#else
char progver[] = "1.3 HPI-B";
#endif
int fdebug = 0;
int fshowthr = 0;
char *rtypes[5] = {"None    ", "Control ", "Sensor  ", "Invent  ", "Watchdog"};

#define NSU  77
char *units[NSU] = {
"units", "degrees C", "degrees F", "degrees K", "volts", "amps", 
"watts", "joules",    "coulombs",  "va",        "nits",  "lumen",
"lux",   "candela",   "kpa",       "psi",     "newton",  "cfm",
"rpm",   "Hz",        "us",        "ms",      "sec",     "min",
"hours", "days",      "weeks",     "mil",     "in",     "ft",
/*30*/ "cu in", "cu ft", "mm", "cm", "m", "cu cm", "cu m", 
"liters", "fl oz", "radians", "sterad", "rev", 
"cycles", "grav", "oz", "lbs", "ft lb", "oz in", "gauss", 
"gilberts", "henry", "mhenry", "farad", "ufarad", "ohms",
/*55*/  "", "", "", "", "", "", "", "", "", "", 
	"", "", "", "", "Gb", 
/*70*/ "bytes", "KB", "MB",  "GB", 
/*74*/ "", "", "line"
};

#define NECODES  27
struct { int code; char *str;
} ecodes[NECODES] = { 
 {  0,    "Success" },
 { -1001, "HPI unspecified error" },
 { -1002, "HPI unsupported function" },
 { -1003, "HPI busy" },
 { -1004, "HPI request invalid" },
 { -1005, "HPI command invalid" },
 { -1006, "HPI timeout" },
 { -1007, "HPI out of space" },
 { -1008, "HPI data truncated" },
#ifdef HPI_A
 { -1009, "HPI data length invalid" },
#else
 { -1009, "HPI invalid parameter" },
#endif
 { -1010, "HPI data exceeds limits" },
 { -1011, "HPI invalid params" },
 { -1012, "HPI invalid data" },
 { -1013, "HPI not present" },
 { -1014, "HPI invalid data field" },
 { -1015, "HPI invalid sensor command" },
 { -1016, "HPI no response" },
 { -1017, "HPI duplicate request" },
 { -1018, "HPI updating" },
 { -1019, "HPI initializing" },
 { -1020, "HPI unknown error" },
 { -1021, "HPI invalid session" },
 { -1022, "HPI invalid domain" },
 { -1023, "HPI invalid resource id" },
 { -1024, "HPI invalid request" },
 { -1025, "HPI entity not present" },
 { -1026, "HPI uninitialized" }
};
char def_estr[15] = "HPI error %d   ";

static
char *decode_error(SaErrorT code)
{
   int i;
   char *str = NULL;
   for (i = 0; i < NECODES; i++) {
	if (code == ecodes[i].code) { str = ecodes[i].str; break; }
   }
   if (str == NULL) { 
	sprintf(&def_estr[10],"%d",code);
	str = &def_estr[0];
   }
   return(str);
}

static
void ShowSensor(
   SaHpiSessionIdT sessionid,
   SaHpiResourceIdT resourceid,
   SaHpiSensorRecT *sensorrec )
{
   SaHpiSensorNumT sensornum;
   SaHpiSensorReadingT reading;
   SaHpiSensorThresholdsT senstbuff; 
#ifdef HPI_A
   SaHpiSensorReadingT conv_reading;
#else
   SaHpiEventStateT  evtstate;
#endif
   SaErrorT rv;
   char *unit;
   int i;

   sensornum = sensorrec->Num;
#ifdef HPI_A
   rv = saHpiSensorReadingGet(sessionid,resourceid, sensornum, &reading);
#else
   rv = saHpiSensorReadingGet(sessionid,resourceid, sensornum, &reading,&evtstate);
#endif
   /* ReadingGet returns -1024 if SDR type is mBMC */
   if (!fdebug && rv == SA_ERR_HPI_INVALID_REQUEST) {
	printf(" = no reading\n");
	return;
   } else if (rv != SA_OK)  {
	printf("ReadingGet ret=%d\n", rv);
	return;
   }

#ifdef HPI_A
   if ((reading.ValuesPresent & SAHPI_SRF_INTERPRETED) == 0) { 
     if ((reading.ValuesPresent & SAHPI_SRF_RAW) == 0) {
	/* no raw or interpreted, so just show event status */
	/* This is a Compact Sensor */
        if (reading.ValuesPresent & SAHPI_SRF_EVENT_STATE) 
	   printf(" = %x %x\n", reading.EventStatus.SensorStatus,
			reading.EventStatus.EventStatus);
	else printf(" = no reading\n");
	return;
     } else {
	/* have raw, but not interpreted, so try convert. */
        rv = saHpiSensorReadingConvert(sessionid, resourceid, sensornum,
					&reading, &conv_reading);
        if (rv != SA_OK) {
		/* conv rv=-1012 on Compact sensors */
		if (!fdebug && rv == SA_ERR_HPI_INVALID_DATA) 
			printf(" = %02x raw\n", reading.Raw);
		else    printf("raw=%x conv_ret=%d\n", reading.Raw, rv);
		/* printf("conv_rv=%s\n", decode_error(rv)); */
		return;
        }
        else {
	   if (fdebug) printf("conv ok: raw=%x conv=%x\n", reading.Raw,
		   conv_reading.Interpreted.Value.SensorUint32);
	   reading.Interpreted.Type = conv_reading.Interpreted.Type;
	   if (reading.Interpreted.Type == SAHPI_SENSOR_INTERPRETED_TYPE_BUFFER)
	   {
	      memcpy(reading.Interpreted.Value.SensorBuffer,
		   conv_reading.Interpreted.Value.SensorBuffer,
		   4); /* SAHPI_SENSOR_BUFFER_LENGTH); */
	      /* IPMI 1.5 only returns 4 bytes */
	   } else 
	      reading.Interpreted.Value.SensorUint32 = 
		   conv_reading.Interpreted.Value.SensorUint32;
	}
     }
   }
   /* Also show units of interpreted reading */
   i = sensorrec->DataFormat.BaseUnits;
   if (i >= NSU) i = 0;
   unit = units[i];
   switch(reading.Interpreted.Type)
   {
	case SAHPI_SENSOR_INTERPRETED_TYPE_FLOAT32:
	     printf(" = %5.2f %s\n", 
		reading.Interpreted.Value.SensorFloat32,unit);
	     break;
	case SAHPI_SENSOR_INTERPRETED_TYPE_UINT32:
	     printf(" = %d %s\n", 
		reading.Interpreted.Value.SensorUint32, unit);
	     break;
	case SAHPI_SENSOR_INTERPRETED_TYPE_BUFFER:
	     printf(" = %02x %02x %02x %02x\n", 
		reading.Interpreted.Value.SensorBuffer[0],
		reading.Interpreted.Value.SensorBuffer[1],
		reading.Interpreted.Value.SensorBuffer[2],
		reading.Interpreted.Value.SensorBuffer[3]);
	     break;
	default: 
	     printf(" = %x (itype=%x)\n", 
		reading.Interpreted.Value.SensorUint32, 
		reading.Interpreted.Type);
   }

   if (fshowthr) {    /* Show thresholds, if any */
#ifdef SHOWMAX
     if ( sensorrec->DataFormat.Range.Flags & SAHPI_SRF_MAX )
         printf( "    Max of Range: %5.2f\n",
         sensorrec->DataFormat.Range.Max.Interpreted.Value.SensorFloat32);
     if ( sensorrec->DataFormat.Range.Flags & SAHPI_SRF_MIN )
         printf( "    Min of Range: %5.2f\n",
         sensorrec->DataFormat.Range.Min.Interpreted.Value.SensorFloat32);
#endif
     if ((!sensorrec->Ignore) && (sensorrec->ThresholdDefn.IsThreshold)) {
	rv = saHpiSensorThresholdsGet(sessionid, resourceid, 
				sensornum, &senstbuff);
	if (rv != 0) { printf("ThresholdsGet ret=%d\n", rv); return; }
	printf( "\t\t\t");
	if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_LOW_MINOR ) {
           printf( "LoMin %5.2f ",
            senstbuff.LowMinor.Interpreted.Value.SensorFloat32);
	} 
        if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_LOW_MAJOR ) {
           printf( "LoMaj %5.2f ",
            senstbuff.LowMajor.Interpreted.Value.SensorFloat32);
	} 
	if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_LOW_CRIT ) {
           printf( "LoCri %5.2f ",
            senstbuff.LowCritical.Interpreted.Value.SensorFloat32);
	} 
        if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_UP_MINOR ) {
           printf( "HiMin %5.2f ",
		senstbuff.UpMinor.Interpreted.Value.SensorFloat32);
	} 
        if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_UP_MAJOR ) {
           printf( "HiMaj %5.2f ",
		senstbuff.UpMajor.Interpreted.Value.SensorFloat32);
	} 
        if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_UP_CRIT ) {
           printf( "HiCri %5.2f ",
		senstbuff.UpCritical.Interpreted.Value.SensorFloat32);
	} 
#ifdef SHOWMAX
        if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_UP_HYSTERESIS ) {
           printf( "Hi Hys %5.2f ",
		senstbuff.PosThdHysteresis.Interpreted.Value.SensorFloat32);
	} 
        if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_LOW_HYSTERESIS ) {
           printf( "Lo Hys %5.2f ",
		senstbuff.NegThdHysteresis.Interpreted.Value.SensorFloat32);
	} 
#endif
	printf( "\n");
     } /* endif valid threshold */
   } /* endif showthr */

#else		 /* HPI_B: HPI B.01.01 logic */
   i = sensorrec->DataFormat.BaseUnits;
   if (i >= NSU) i = 0;
   unit = units[i];
   switch(reading.Type)
   {
	case SAHPI_SENSOR_READING_TYPE_FLOAT64:
	     printf(" = %5.2f %s\n", 
		reading.Value.SensorFloat64,unit);
	     break;
	case SAHPI_SENSOR_READING_TYPE_UINT64:
	     printf(" = %lld %s\n", 
		reading.Value.SensorUint64, unit);
	     break;
	case SAHPI_SENSOR_READING_TYPE_INT64:
	     printf(" = %lld %s\n", 
		reading.Value.SensorInt64, unit);
	     break;
	case SAHPI_SENSOR_READING_TYPE_BUFFER:
	     printf(" = %02x %02x %02x %02x\n", 
		reading.Value.SensorBuffer[0],
		reading.Value.SensorBuffer[1],
		reading.Value.SensorBuffer[2],
		reading.Value.SensorBuffer[3]);
	     break;
	default: 
	     printf(" = %llx (itype=%x)\n", 
		reading.Value.SensorUint64, 
		reading.Type);
   }
   if (fshowthr) {    /* Show thresholds, if any */
     if (sensorrec->ThresholdDefn.IsAccessible) {
       rv = saHpiSensorThresholdsGet(sessionid, resourceid, 
				sensornum, &senstbuff);
       if (rv != SA_OK) { printf("ThresholdsGet ret=%d\n", rv);  
       } else {
	printf( "\t\t\t");
	if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_LOW_MINOR ) {
           printf( "LoMin %5.2f ",
            senstbuff.LowMinor.Value.SensorFloat64);
	} 
        if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_LOW_MAJOR ) {
           printf( "LoMaj %5.2f ",
            senstbuff.LowMajor.Value.SensorFloat64);
	} 
	if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_LOW_CRIT ) {
           printf( "LoCri %5.2f ",
            senstbuff.LowCritical.Value.SensorFloat64);
	} 
        if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_UP_MINOR ) {
           printf( "HiMin %5.2f ",
		senstbuff.UpMinor.Value.SensorFloat64);
	} 
        if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_UP_MAJOR ) {
           printf( "HiMaj %5.2f ",
		senstbuff.UpMajor.Value.SensorFloat64);
	} 
        if ( sensorrec->ThresholdDefn.ReadThold & SAHPI_STM_UP_CRIT ) {
           printf( "HiCri %5.2f ",
		senstbuff.UpCritical.Value.SensorFloat64);
	} 
	printf( "\n");
       } /*endif get ok*/
     } /* endif valid threshold */
   } /* endif showthr */
#endif
   return;
}  /*end ShowSensor*/

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

  printf("%s: version %s\n",argv[0],progver); 

  while ( (c = getopt( argc, argv,"tx?")) != EOF )
  switch(c) {
        case 't': fshowthr = 1; break;
        case 'x': fdebug = 1; break;
        default:
                printf("Usage %s [-t -x]\n",argv[0]);
                printf("where -t = show Thresholds also\n");
                printf("      -x = show eXtra debug messages\n");
                exit(1);
  }
  if (fdebug) printf("fshowthr = %d, fdebug = %d\n",fshowthr,fdebug);
#ifdef HPI_A
  rv = saHpiInitialize(&hpiVer);
  if (rv != SA_OK) {
	printf("saHpiInitialize: %s\n",decode_error(rv));
	exit(-1);
	}
  rv = saHpiSessionOpen(SAHPI_DEFAULT_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
	if (rv == SA_ERR_HPI_ERROR) 
	   printf("saHpiSessionOpen: error %d, SpiLibd not running\n",rv);
	else
	   printf("saHpiSessionOpen: %s\n",decode_error(rv));
	exit(-1);
	}
  if (fdebug) printf("Starting Discovery ...\n");
  rv = saHpiResourcesDiscover(sessionid);
  if (fdebug) printf("saHpiResourcesDiscover %s\n",decode_error(rv));
  rv = saHpiRptInfoGet(sessionid,&rptinfo);
  if (fdebug) printf("saHpiRptInfoGet %s\n",decode_error(rv));
  printf("RptInfo: UpdateCount = %x, UpdateTime = %lx\n",
         rptinfo.UpdateCount, (unsigned long)rptinfo.UpdateTimestamp);
#else
  rv = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
	printf("saHpiSessionOpen: %s\n",decode_error(rv));
	exit(-1);
	}
  if (fdebug) printf("Starting Discovery ...\n");
  rv = saHpiDiscover(sessionid);
  if (fdebug) printf("saHpiDiscover %s\n",decode_error(rv));
  if (fshowthr) {
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
     if (fdebug) printf("saHpiRptEntryGet %s\n",decode_error(rv));
     if (rv == SA_OK) {
	/* walk the RDR list for this RPT entry */
	entryid = SAHPI_FIRST_ENTRY;
	resourceid = rptentry.ResourceId;
#ifdef HPI_A
        rptentry.ResourceTag.Data[rptentry.ResourceTag.DataLength] = 0; 
#endif
	printf("rptentry[%d] resourceid=%d tag: %s\n",
		rptentryid,resourceid, rptentry.ResourceTag.Data);
	while ((rv == SA_OK) && (entryid != SAHPI_LAST_ENTRY))
	{
		rv = saHpiRdrGet(sessionid,resourceid,
				entryid,&nextentryid, &rdr);
  		if (fdebug) printf("saHpiRdrGet[%d] rv = %d\n",entryid,rv);
		if (rv == SA_OK) {
			char *eol;
			rdr.IdString.Data[rdr.IdString.DataLength] = 0;
			if (rdr.RdrType == SAHPI_SENSOR_RDR) eol = "    \t";
			else eol = "\n";
			printf("RDR[%02d]: %s %s %s",rdr.RecordId,
				rtypes[rdr.RdrType],rdr.IdString.Data,eol);
			if (rdr.RdrType == SAHPI_SENSOR_RDR) {
			    ShowSensor(sessionid,resourceid,
					&rdr.RdrTypeUnion.SensorRec);
			} 
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
  return(0);
}
 
/* end hpisensor.c */
