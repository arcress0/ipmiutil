/*
 * oem_asus.c
 * Handle ASUS OEM command functions 
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Change history:
 *  01/23/2017 ARCress - created
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

extern uchar bitnum(ushort value);  /*isensor.c*/
extern char fdebug;  /*ipmicmd.c*/
void set_loglevel(int level);  /*prototype */

#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil asusoem";
#else
static char * progver   = "3.08";
static char * progname  = "iasusoem";
#endif
static int verbose = 0;
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;
static int vend_id = 0;
static int prod_id = 0;

/*
 * decode_sensor_asus
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
int decode_sensor_asus(uchar *sdr,uchar *reading,char *pstring, int slen)
{
   int rv = -1;
   uchar stype, etype, entity;
   uchar bval;
   ushort rval;
   char *pstr = NULL;
   int b;

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
   if (fdebug) printf("oem_asus: sensor type=%x evt=%x entity=%x rval=%04x\n",
					stype,etype,entity,rval);
   switch(stype) {
       case 0xC5:      /* OEM for Memory_Train_Err */
           if (etype == 0x6F) {
              switch(bval) {
              case 0x00: pstr = "OK"; break;
              default: pstr = "Error"; break;
              }
              snprintf(pstring,slen,"%s",pstr);
              rv = 0;
           }
           break;
       default: break;
   }
   return(rv);
}

/*
 * decode_sel_asus
 * inputs:
 *    evt    = the 16-byte IPMI SEL event
 *    outbuf = points to the output string buffer
 *    outsz  = size of the output buffer
 * outputs:
 *    rv     = 0 if this event was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    outbuf = will contain the interpreted event text string (if rv==0)
 */
int decode_sel_asus(uchar *evt, char *outbuf, int outsz, char fdesc,
                        char fdbg)
{
   int rv = -1;
   ushort id;
   uchar rectype;
   ulong timestamp;
   char mybuf[64]; 
   char *type_str = "";
   char *pstr = NULL;
   int sevid;
   ushort genid;
   uchar snum;
   uchar data1, data2, data3;
         
   fdebug = fdbg;
   sevid = SEV_INFO;
   id = evt[0] + (evt[1] << 8);
   rectype = evt[2];
   snum = evt[11];
   timestamp = evt[3] + (evt[4] << 8) + (evt[5] << 16) + (evt[6] << 24);
   genid = evt[7] | (evt[8] << 8);
   data1 = evt[13];
   data2 = evt[14];
   data3 = evt[15];
   if (rectype == 0x02) 
   {
     sprintf(mybuf,"%02x [%02x %02x %02x]", evt[12],data1,data2,data3);
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

static int ipmi_oemasus_main(int  argc, char **argv)
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
int i_oemasus(int argc, char **argv)
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

   rv = ipmi_oemasus_main(argc, argv);

   ipmi_close_();
   return(rv);
}
/* end oem_asus.c */
