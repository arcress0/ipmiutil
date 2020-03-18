/*
 * oem_quanta.c
 * Handle Quanta OEM command functions 
 *
 * Change history:
 *  01/11/2012 ARCress - included in source tree
 *
 *---------------------------------------------------------------------
 */
/*M*
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
 *M*/
#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ipmicmd.h"
#include "ievents.h"

extern int decode_sensor_intel_nm(uchar *sdr,uchar *reading,
				  char *pstring,int slen);  /*oem_intel.c*/
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil quantaoem";
#else
static char * progver   = "3.08";
static char * progname  = "iquantaoem";
#endif
static char fdbg = 0;

/*
 * decode_sensor_quanta
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
int decode_sensor_quanta(uchar *sdr,uchar *reading,char *pstring, int slen)
{
   int rv = -1;
   int vend, prod;

   if (sdr == NULL || reading == NULL) return(rv);
   if (pstring == NULL || slen == 0) return(rv);
   get_mfgid(&vend,&prod); /*saved from ipmi_getdeviceid */
   if (vend != VENDOR_QUANTA) return(rv);
   if (prod != PRODUCT_QUANTA_S99Q) return(rv);  /*only handle S99Q product*/
   if (sdr[3] != 0x02) return(rv);  /*skip if not compact sensor*/
           
   /* OEM SDR for Intel Node Manager */
   rv = decode_sensor_intel_nm(sdr,reading,pstring,slen);
   return(rv);
}

/*
 * decode_sel_quanta
 * inputs:
 *    evt    = the 16-byte IPMI SEL event
 *    outbuf = points to the output string buffer
 *    outsz  = size of the output buffer
 * outputs:
 *    rv     = 0 if this event was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    outbuf = will contain the interpreted event text string (if rv==0)
 *
 * Also case 7 was added to mem_str() in ievents.c for Quanta QSSC-S4R.
 */
int decode_sel_quanta(uchar *evt, char *outbuf, int outsz, char fdesc,
                        char fdebug)
{
   int rv = -1;
   ushort id;
   uchar rectype;
   ulong timestamp;
   char mybuf[64]; 
   char *type_str = "";
   char *gstr = NULL;
   char *pstr = NULL;
   int sevid;
   ushort genid;
   uchar snum;
   int vend, prod;
         
   fdbg = fdebug;
   get_mfgid(&vend,&prod); /*saved from ipmi_getdeviceid */
   if (vend != VENDOR_QUANTA) return(rv);
   if (prod != PRODUCT_QUANTA_S99Q) return(rv);  /*only handle S99Q product*/

   sevid = SEV_INFO;
   id = evt[0] + (evt[1] << 8);
   rectype = evt[2];
   snum = evt[11];
   timestamp = evt[3] + (evt[4] << 8) + (evt[5] << 16) + (evt[6] << 24);
   genid = evt[7] | (evt[8] << 8);
   if (genid == 0x0033) gstr = "Bios";
   else gstr = "BMC ";
   if (rectype == 0x02) {
     sprintf(mybuf,"%02x [%02x %02x %02x]", evt[12],evt[13],evt[14],evt[15]);
     switch(evt[10]) {  /*sensor type*/
        case 0xC0:      /* CPU Temp Sensor, EvTyp=0x70 */
	   switch(evt[13]) {  /*offset/data1*/
           case 0x02: pstr = "Overheat"; sevid = SEV_MAJ; rv = 0;  break;
	   default: break;
	   }
	   break;
	default:
	   break;
     }
   }
   if (rv == 0) {
	 format_event(id,timestamp, sevid, genid, type_str,
			snum,NULL,pstr,mybuf,outbuf,outsz);
   }

   return(rv);
}

#ifdef METACOMMAND
int i_quantaoem(int argc, char **argv)
{
   printf("%s ver %s\n", progname,progver);
   return(0);
}
#endif
/* end oem_quanta.c */
