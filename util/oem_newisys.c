/*
 * oem_newisys.c
 *
 * This module handles OEM-specific functions for newisys firmware.
 *
 * Copyright (c) 2012 Kontron America, Inc.
 *
 * 04/05/12 Andy Cress v1.0  new, with input from ipmitool
 */
/*M*
Copyright (c) 2012 Kontron America, Inc.
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

#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil newisysoem";
#else
static char * progver   = "3.08";
static char * progname  = "inewisysoem";
#endif
static char  fdebug = 0;
static uchar g_bus = PUBLIC_BUS;
static uchar g_sa  = BMC_SA;
static uchar g_lun = BMC_LUN;
//static uchar g_addrtype = ADDR_SMI;

#define DESC_MAX  200
/* 
 * decode_sel_newisys
 * inputs:
 *    evt    = the 16-byte IPMI SEL event
 *    outbuf = points to the output string buffer
 *    outsz  = size of the output buffer
 * outputs: 
 *    rv     = 0 if this event was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    outbuf = will contain the interpreted event text string (if rv==0)
 */
int decode_sel_newisys(uchar *evt, char *outbuf, int outsz, char fdesc, 
			char fdbg)
{
   int rv = -1;
   ushort id, genid;
   ulong timestamp;
   char *type_str = NULL;
   char *gstr = NULL;
   char *pstr = NULL;
   int sevid;
   char description[DESC_MAX];
   int rlen, i;
   uchar idata[16];
   uchar rdata[DESC_MAX];
   uchar cc;
   uchar snum, stype, rectype;

   fdebug = fdbg;
   id = evt[0] + (evt[1] << 8);
   rectype = evt[2];
   timestamp = evt[3] + (evt[4] << 8) + (evt[5] << 16) + (evt[6] << 24);
   genid = evt[7] | (evt[8] << 8);
   stype = evt[10];
   snum = evt[11];
   gstr = "BMC ";
   if (genid == 0x0033) gstr = "Bios";
   type_str = "";
   if (rectype == 0x02) type_str = get_sensor_type_desc(stype);
   sevid = SEV_INFO;

   idata[0] = 0x15; /* IANA LSB */
   idata[1] = 0x24; /* IANA     */
   idata[2] = 0x00; /* IANA MSB */
   idata[3] = 0x01; /* Subcommand */
   idata[4] = id & 0x00FF;        /* SEL Record ID LSB */
   idata[5] = (id & 0xFF00) >> 8; /* SEL Record ID MSB */
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(0x01, 0x2E, g_sa,g_bus,g_lun,
                        idata, 6, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;

   if (rv == 0) {
        if (rlen < 5) {
                printf("Newisys OEM response too short");
                return LAN_ERR_TOO_SHORT;
        } else if (rlen != (4 + rdata[3])) {
                printf("Newisys OEM response has unexpected length");
                return LAN_ERR_TOO_SHORT;
        }

	/* copy the description */
	i = rdata[3];
	if (i >= DESC_MAX) i = DESC_MAX - 1;
	if (i > (rlen-4)) i = rlen - 4;
	memcpy(description, &rdata[4], i);
	description[i] = 0;;
	pstr = description;
	/*TODO: parse for severity, setting sevid */

	format_event(id,timestamp, sevid, genid, type_str,
			snum,NULL,pstr,NULL,outbuf,outsz);
   }
   return rv;
}

#ifdef METACOMMAND
int i_newisysoem(int argc, char **argv)
{
   printf("%s ver %s\n", progname,progver);
   return(0);
}
#endif
/* end oem_newisys.c */
