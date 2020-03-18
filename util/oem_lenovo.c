/*
 * oem_lenovo.c
 * Handle Lenovo OEM command functions 
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Change history:
 *  11/21/2016 ARCress - created
 *
 *---------------------------------------------------------------------
 */
/*M*
Copyright (c) 2016 Andy Cress
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 
  c.. Neither the name of the copyright holder nor the names of contributors 
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
#include "getopt.h"
#else
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#include <sys/time.h>
#else
#include <getopt.h>
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ipmicmd.h"
#include "ievents.h"
#include "oem_lenovo.h"

extern uchar bitnum(ushort value);  /*isensor.c*/
extern char fdebug;  /*ipmicmd.c*/
void set_loglevel(int level);  /*prototype */

#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil lenovooem";
#else
static char * progver   = "3.08";
static char * progname  = "ilenovooem";
#endif
static int verbose = 0;
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;
static int vend_id = 0;
static int prod_id = 0;

/* See oem_ibm_sel_map from ipmitool contrib, interpretation not verified yet */
#define NIBMEVT 16
struct { uchar stype; uchar snum; uchar etype; 
	uchar data1; uchar data2; uchar data3; 
	uchar sev; char *desc; } oem_ibm_events[NIBMEVT] = {
0xC1, 0xFF, 0xFF, 0xFF, 0x01, 0x00, SEV_MIN, "CPU shutdown - Potential cause 'triple fault' a software address problem",
0xC1, 0xFF, 0xFF, 0xFF, 0x02, 0x01, SEV_INFO, "Memory Mirrored Failover Occurred - System running from mirrored memory image",
0xC1, 0xFF, 0xFF, 0xFF, 0x02, 0x04, SEV_INFO, "Memory hot replace event",
0xC1, 0xFF, 0xFF, 0xFF, 0x02, 0x05, SEV_INFO, "Memory hot add event",
0xC1, 0xFF, 0xFF, 0xFF, 0x03, 0x00, SEV_INFO, "Scalability link down",
0xC1, 0xFF, 0xFF, 0xFF, 0x03, 0x01, SEV_INFO, "Scalability link up",
0xC1, 0xFF, 0xFF, 0xFF, 0x03, 0x02, SEV_INFO, "Scalability link double wide down",
0xC1, 0xFF, 0xFF, 0xFF, 0x03, 0x03, SEV_INFO, "Scalability link double wide up",
0xC1, 0xFF, 0xFF, 0xFF, 0x03, 0x80, SEV_INFO, "Scalability link PFA",
0xC1, 0xFF, 0xFF, 0xFF, 0x03, 0x81, SEV_INFO, "Scalability link invalid port",
0xC1, 0xFF, 0xFF, 0xFF, 0x03, 0x82, SEV_INFO, "Scalability link invalid node",
0xC1, 0xFF, 0xFF, 0xFF, 0x03, 0x83, SEV_INFO, "Scalability link kill",
0xE0, 0x00, 0x00, 0xFF, 0xFF, 0xFF, SEV_INFO, "Device OK",
0xE0, 0x00, 0x01, 0xFF, 0xFF, 0xFF, SEV_MAJ, "Required ROM space not available",
0xE0, 0x00, 0x02, 0xFF, 0xFF, 0xFF, SEV_MAJ, "Required I/O Space not available",
0xE0, 0x00, 0x03, 0xFF, 0xFF, 0xFF, SEV_MAJ, "Required memory not available"
};

/*------------------------------------------------------------------------ 
 * get_ibm_desc 
 * Uses the oem_ibm_events to decode IBM events not otherwise handled.
 * Called by decode_sel_lenovo
 *------------------------------------------------------------------------*/
char * get_ibm_desc(uchar type, uchar num, uchar trig,
		 uchar data1, uchar data2, uchar data3, uchar *sev)
{
	int i;
	char *pstr = NULL; 

	/* Use oem_ibm_events array for other misc descriptions */
	data1 &= 0x0f;  /*ignore top half of sensor offset for matching */
	for (i = 0; i < NIBMEVT; i++) {
           if ((oem_ibm_events[i].stype == 0xff) || 
               (oem_ibm_events[i].stype == type)) {
	      if (oem_ibm_events[i].snum != 0xff &&
	         oem_ibm_events[i].snum != num)
			    continue;
	      if (oem_ibm_events[i].etype != 0xff &&
	         oem_ibm_events[i].etype != trig)
				    continue;
	      if (oem_ibm_events[i].data1 != 0xff && 
      	          (oem_ibm_events[i].data1 & 0x0f) != (data1 & 0x0f))
				    continue;
	      if (oem_ibm_events[i].data2 != 0xff && 
      	          oem_ibm_events[i].data2 != data2)
				    continue;
	      if (oem_ibm_events[i].data3 != 0xff && 
      	          oem_ibm_events[i].data3 != data3)
				    continue;
	      /* have a match, use description */
	      pstr = (char *)oem_ibm_events[i].desc; 
	      if (sev != NULL) *sev = oem_ibm_events[i].sev; 
	      break;
           }
	} /*end for*/
	return(pstr);
}  /* end get_ibm_desc() */

/*
 * decode_sensor_lenovo
 * inputs:
 *    sdr     = the SDR buffer
 *    reading = the 3 or 4 bytes of data from GetSensorReading
 *    pstring = points to the output string buffer
 *    slen    = size of the output buffer
 * outputs:
 *    rv     = 0 if this sensor was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    pstring = contains the sensor reading interpretation string (if rv==0)
 */
int decode_sensor_lenovo(uchar *sdr,uchar *reading,char *pstring, int slen)
{
   int rv = -1;
   uchar stype, etype, entity;
   uchar bval;
   ushort rval;
   char *pstr = NULL;
   int b;
   int vend, prod;

   if (sdr == NULL || reading == NULL) return(rv);
   if (pstring == NULL || slen == 0) return(rv);
   /* sdr[3] is 0x01 for Full, 0x02 for Compact */
   bval = reading[2];
   if (sdr[3] == 0x01) return(rv); /*skip if full sensor*/
   entity = sdr[8];
   stype = sdr[12];
   etype = sdr[13];
   rval = reading[2] | ((reading[3] & 0x7f) << 8);
   b = bitnum(rval);
   get_mfgid(&vend,&prod); /*saved from ipmi_getdeviceid */
   if (fdebug) printf("oem_lenovo: mfg=%04x:%04x sensor type=%x evt=%x entity=%x rval=%04x\n",
					vend,prod,stype,etype,entity,rval);
   switch(stype) {
       case 0x07:      /* CPU sensor type */
           if ((etype == 0x6F) && (entity == 25)) {  /*All CPUs*/
              switch(b) {
              case 0x00: pstr = "OK"; break;
              case 0x02: pstr = "BIST_Fail"; break;
              case 0x05: pstr = "Config_Error"; break;
              case 0x06: pstr = "Uncorr_Error"; break;
              case 0x08: pstr = "Disabled"; break;
              default: pstr = "OK*"; break;
              }
              rv = 0;
           }
           else if ((etype == 0x6F) && (entity == 3)) {  /*CPU status*/
           /* Special CPU status reported on IBM/Lenovo */
              switch(rval) {
              case 0x00: pstr = "Absent"; break;
              case 0x80: pstr = "Present"; break;
              default:   pstr = "Error"; break;
              }
              rv = 0;
           }
           break;
       case 0x0C:      /* DIMM slot status */
           /* Special DIMM status reported on IBM/Lenovo */
           /* IBM x3650 M2 (0x0002, 0x00dc) and 
			* Lenovo x3650 M4 (0x4f4d, 0x0143) */
           if (etype == 0x6F) {
              switch(rval) {
              case 0x00: pstr = "Absent"; break;
              case 0x40: pstr = "Present"; break;
              default:   pstr = "Error"; break;
              }
              rv = 0;
           }
           break;
       case 0x0D:      /* Disk Drive slots */
           if (etype == 0x6F) {
              switch(bval) {
              case 0x00: pstr = "Absent"; break;
              case 0x01: pstr = "Present"; break;
              case 0x21: pstr = "NotRedundant"; break;
              case 0x40: pstr = "NotAvailable"; break; /*not initialized*/
              default: pstr = "Faulty"; break; /*e.g. 0x03*/
              }
              rv = 0;
           }
           break;
       case 0x0F:      /* ABR Status, Firmware Error, Sys Boot Status, etc. */
           if (etype == 0x6F) { /* should be entity 0x22 */
              if (bval == 0x00) pstr = "OK";
              else if (bval & 0x01) pstr = "FirmwareError"; /*bit 0*/
              else if ((bval & 0x02) != 0) pstr = "FirmwareHang"; /*bit 1*/
              else pstr = "OK*";
              rv = 0;
           }
           break;
       case 0x17:      /* RSA II Detect, Mem1 Detect (mfg=0002:0077) */
			/* mfg=0002:0077 sensor type=17 evt=8 entity=b rval=0002, RSA II*/
			/* mfg=0002:0077 sensor type=17 evt=8 entity=8 rval=0002, Mem1  */
              switch(rval) {
              case 0x02: pstr = "OK"; break;
              default:   pstr = "Unknown"; break;
              }
              rv = 0;
           break;
       case 0x1B:      /* Front USB, nvDIMM Cable, etc. */
           if (etype == 0x6F) {
              /* 0x01 = connected/OK, 0x02 = cable connect error */
              switch(bval) {
              case 0x01: pstr = "OK"; break;
              case 0x02: pstr = "CableError"; break;
              default: pstr = "OK*"; break;
              }
              rv = 0;
           }
           break;
       case 0x1E:      /* No Boot Device */
           if (etype == 0x6F) {
              switch(b) {
              case 0x00: pstr = "OK"; break; /*No Media*/
              default: pstr = "Asserted"; break;
              }
              rv = 0;
           }
       case 0x21:      /*All PCI Error, PCI 1, Internal RAID, No Op ROM, etc.*/
           if (etype == 0x6F) { 
              if (bval == 0x00) pstr = "OK";
              else pstr = "Fault";
              rv = 0;
           }
           break;
       case 0x25:      /* DASD Backplane*/
		   /*spec says 00 = Present, 01 = Absent, but not match results*/
		   /*
0066 SDR Comp 02 6f 20 a 25 snum 20 DASD Backplane 1 = 0001 OK*
0067 SDR Comp 02 6f 20 a 25 snum 21 DASD Backplane 2 = 0000 Absent
0068 SDR Comp 02 6f 20 a 25 snum 22 DASD Backplane 3 = 0000 Absent
0069 SDR Comp 02 6f 20 a 25 snum 23 DASD Backplane 4 = 0000 Absent
006a SDR Comp 02 6f 20 a 25 snum 24 DASD Backplane 5 = 0000 Absent
006b SDR Comp 02 6f 20 a 25 snum 25 DASD Backplane 6 = 0000 Absent
		   */
              rv = -1;
           break;
       case 0x2B:      /* ROM Recovery, Bkup Auto Update, IMM*/
           if (etype == 0x6F) {
              switch(bval) {
              case 0x00: pstr = "OK"; break;
              case 0x01: pstr = "Changed"; break; /*entity 0x22*/
              case 0x05: pstr = "Invalid"; break; /*entity 0x21*/
              case 0x07: pstr = "ChangedOK"; break; /*entity 21,22*/
              default: pstr = "_"; break;
              }
              rv = 0;
           }
           break;
       case 0x02:      /* SysBrd Vol Fault: et 07 8004=error */
           if (etype == 0x07) {
			  if ((bval & 0x04) != 0) pstr = "Faulty"; 
			  else pstr = "OK";
              rv = 0;
           }
           break;
       case 0x28:      /* Low Security Jmp et 08 8002=error*/
           if (etype == 0x08) {
			  if ((bval & 0x02) != 0) pstr = "Faulty"; 
			  else pstr = "OK";
              rv = 0;
           }
           break;

       default: break;
   }
   if (rv == 0) snprintf(pstring,slen,"%s",pstr);
   // if (rv == 0) strncpy(pstring, pstr, slen);
   return(rv);
}

/*
 * decode_sel_lenovo
 * inputs:
 *    evt    = the 16-byte IPMI SEL event
 *    outbuf = points to the output string buffer
 *    outsz  = size of the output buffer
 * outputs:
 *    rv     = 0 if this event was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    outbuf = will contain the interpreted event text string (if rv==0)
 */
int decode_sel_lenovo(uchar *evt, char *outbuf, int outsz, char fdesc,
                        char fdbg)
{
   int rv = -1;
   ushort id;
   uchar rectype;
   ulong timestamp;
   char mybuf[64]; 
   char *type_str = "";
   char *pstr = NULL;
   uchar sevid;
   ushort genid;
   uchar snum, etype;
   uchar data1, data2, data3;
         
   fdebug = fdbg;
   sevid = SEV_INFO;
   id = evt[0] + (evt[1] << 8);
   rectype = evt[2];
   snum = evt[11];
   timestamp = evt[3] + (evt[4] << 8) + (evt[5] << 16) + (evt[6] << 24);
   genid = evt[7] | (evt[8] << 8);
   etype = evt[12];
   data1 = evt[13];
   data2 = evt[14];
   data3 = evt[15];

   if (rectype == 0x02) {
     sprintf(mybuf,"%02x [%02x %02x %02x]", etype,data1,data2,data3);
     pstr = get_ibm_desc(evt[10], snum, etype, data1, data2, data3, &sevid);
     if (pstr != NULL) {
	   type_str = "IBM_type";
       rv = 0;
     } else {
       switch(evt[10]) {  /*sensor type*/
       case 0xC0:      /* OEM type */
		 type_str = "OEM_type";
		 pstr = "OEM Sensor   "; 
		 sevid = SEV_MAJ; 
		 rv = -1;
		 break;
       default: /*other sensor types*/
		 break;
       }
     }
   }
   if (rv == 0) {
	 format_event(id,timestamp, sevid, genid, type_str,
			snum,NULL,pstr,mybuf,outbuf,outsz);
   }
   return(rv);
}

static void usage(void)
{
   printf("Usage: %s <command> [arg]\n",progname);
   printf("These commands may not work on all Lenovo systems\n");
}

static int ipmi_oemlenovo_main(int  argc, char **argv)
{
   int rv = 0;

   if (strncmp(argv[0],"other",9) == 0) {
      usage();
      rv = ERR_USAGE;
   } else {
      usage();
      rv = ERR_USAGE;
   }
   return(rv);
}

#ifdef METACOMMAND
int i_oemlenovo(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
#endif
{
   int rv = 0;
   uchar devrec[16];
   int c, i;
   char *s1;

   printf("%s ver %s\n", progname,progver);
   set_loglevel(LOG_NOTICE);
   parse_lan_options('V',"4",0);  /*default to admin priv*/

   while ( (c = getopt( argc, argv,"m:xzEF:J:N:P:R:T:U:V:YZ:?")) != EOF )
      switch(c) {
          case 'm': /* specific IPMB MC, 3-byte address, e.g. "409600" */
                    g_bus = htoi(&optarg[0]);  /*bus/channel*/
                    g_sa  = htoi(&optarg[2]);  /*device slave address*/
                    g_lun = htoi(&optarg[4]);  /*LUN*/
                    g_addrtype = ADDR_IPMB;
                    if (optarg[6] == 's') {
                             g_addrtype = ADDR_SMI;  s1 = "SMI";
                    } else { g_addrtype = ADDR_IPMB; s1 = "IPMB"; }
                    ipmi_set_mc(g_bus,g_sa,g_lun,g_addrtype);
                    printf("Use MC at %s bus=%x sa=%x lun=%x\n",
                            s1,g_bus,g_sa,g_lun);
                    break;
          case 'x': fdebug = 2; /* normal (dbglog if isol) */
                    verbose = 1;
                    break;
          case 'z': fdebug = 3; /*full debug (for isol)*/
                    verbose = 1;
                    break;
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
		usage();
		return(ERR_USAGE);
                break;
      }
   for (i = 0; i < optind; i++) { argv++; argc--; }
   if (argc == 0) {
	usage();
	return(ERR_USAGE);
   }

   rv = ipmi_getdeviceid(devrec,16,fdebug);
   if (rv == 0) {
      char ipmi_maj, ipmi_min;
      ipmi_maj = devrec[4] & 0x0f;
      ipmi_min = devrec[4] >> 4;
      vend_id = devrec[6] + (devrec[7] << 8) + (devrec[8] << 16);
      prod_id = devrec[9] + (devrec[10] << 8);
      show_devid( devrec[2],  devrec[3], ipmi_maj, ipmi_min);
   }

   rv = ipmi_oemlenovo_main(argc, argv);

   ipmi_close_();
   return(rv);
}
/* end oem_lenovo.c */
