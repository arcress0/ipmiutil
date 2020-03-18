/*
 * idcmi.c
 * Data Center Manageability Interface (DCMI) command support 
 *
 * Change history:
 *  11/17/2011 ARCress - created
 *
 *---------------------------------------------------------------------
 */
/*M*
Copyright (c) 2011 Kontron America, Inc.
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
#include <stdlib.h>
#include "getopt.h"
#else
#include <sys/types.h>
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#include <sys/time.h>
#else
#include <stdlib.h>
#include <getopt.h>
#endif
#endif
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "ipmicmd.h"
#include "isensor.h"
#include "idcmi.h"

#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil dcmi";
#else
static char * progver   = "3.08";
static char * progname  = "idcmi";
#endif
extern char   fdebug;  /*from ipmicmd.c*/
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;
static uchar  dcmi_ver = 0x00;  /* 0x10, 0x11, or 0x15 */
static uchar  fpwm = 0;  /* =1 if Power Management supported */
static uchar  do_sensors = 0;
static uchar  set_asset  = 0;
static uchar  set_mcid   = 0;
static char  *asset_new = NULL;
static char  *mcid_new  = NULL;
static int asset_len = 0;
static int mcid_len  = 0;
static char mc_id[64];
static char asset[64];

#ifdef NOT
/* see idcmi.h */
#define NETFN_DCMI  0x2C
#define CMD_DCMI_GET_CAPAB     0x01
#define CMD_DCMI_GET_POWREAD   0x02
#define CMD_DCMI_GET_POWLIMIT  0x03
#define CMD_DCMI_SET_POWLIMIT  0x04
#define CMD_DCMI_ACT_POWLIMIT  0x05
#define CMD_DCMI_GET_ASSETTAG  0x06
#define CMD_DCMI_GET_SENSORINF 0x07
#define CMD_DCMI_SET_ASSETTAG  0x08
#define CMD_DCMI_GET_MCIDSTR   0x09
#define CMD_DCMI_SET_MCIDSTR   0x0A
/* for DCMI 1.5  only */
#define CMD_DCMI_SET_THERMLIM  0x0B
#define CMD_DCMI_GET_THERMLIM  0x0C
#define CMD_DCMI_GET_TEMPRDNGS 0x10
#define CMD_DCMI_SET_CONFIG    0x12
#define CMD_DCMI_GET_CONFIG    0x13
#endif

static int dcmi_usage(void)
{
   printf("Usage: %s [-admsx -NUPREFTVY] <function>\n", progname);
   printf("       -a       Set DCMI Asset Tag to this string\n");
   printf("       -d       Set DCMI MC ID to this string\n");
   printf("       -m002000 specific MC (bus 00,sa 20,lun 00)\n");
   printf("       -s       Get DCMI sensor info\n");
   printf("       -x       Display extra debug messages\n");
   print_lan_opt_usage(1);
   printf("where <function> is one of:\n");
   printf("   info        Get DCMI Capabilities, MC ID, asset tag (default)\n");
   printf("   power [get]               Get Power reading & limit\n");
   printf("   power set_limit <power>   Set Power limit\n");
   printf("   power set_action <action> Set Power limit exception action\n");
   printf("                             (action = no_action | power_off | log_sel)\n");
   printf("   power set_correction <ms> Set Power limit correction time (in ms)\n");
   printf("   power set_sample <sec>    Set Power limit sampling period (in sec)\n");
   printf("   power activate            Activate Power limit\n");
   printf("   power deactivate          Deactivate Power limit\n");
   printf("   thermal     Get/Set DCMI Thermal parameters\n");
   printf("   config      Get/Set DCMI Configuration parameters\n");
   printf("   help        Show this help message\n");
   return(ERR_USAGE);
}

static int dcmi_get_capab(int param, uchar *pdata, int sdata)
{
	uchar idata[4];
	uchar rdata[32];
	int rlen;
	uchar cc;
	int rv, i;

	if (pdata == NULL || sdata == 0) return(ERR_BAD_PARAM);
	idata[0] = 0xDC;
	idata[1] = (uchar)param;
	rlen = sizeof(rdata);
        rv = ipmi_cmdraw( CMD_DCMI_GET_CAPAB, NETFN_DCMI,
			    g_sa, g_bus, g_lun,
                        idata,2, rdata, &rlen, &cc, fdebug);
	if ((rv != 0) || (cc != 0)) {
	   if (fdebug)
              printf("dcmi_get_capab(%d): rv = %d, ccode %02x\n",param,rv,cc);
	   if (rv == 0) rv = cc;
	   return(rv);
	}
	/* if here, success */
	if (fdebug) {  /*show raw response*/
           printf("dcmi_get_capab(%d): rlen = %d\n",param,rlen);
           for (i = 0; i < rlen; i++) printf("%02x ",rdata[i]);
           printf("\n");
	}
        if (rlen > sdata) {
	   if (fdebug) 
		printf("dcmi_get_capab(%d): data truncated from %d to %d\n",
			param,rlen,sdata);
	   rlen = sdata;
	}
	memcpy(pdata,rdata,rlen);
	return(rv);
}

static int dcmi_get_power_read(int param, uchar *pdata, int sdata)
{
	uchar idata[5];
	uchar rdata[32];
	int rlen;
	uchar cc;
	int rv, i;

	if (pdata == NULL || sdata == 0) return(ERR_BAD_PARAM);
	idata[0] = 0xDC;
	idata[1] = (uchar)param; /*mode 1 or 2*/
	idata[2] = 0x00;
	idata[3] = 0x00;
	rlen = sizeof(rdata);
        rv = ipmi_cmdraw( CMD_DCMI_GET_POWREAD, NETFN_DCMI,
			    g_sa, g_bus, g_lun,
                        idata,4, rdata, &rlen, &cc, fdebug);
	if (rv == 0) rv = cc;
	if (fdebug) {  /*show raw response*/
           printf("dcmi_get_power_read(%d): rv = %d rlen = %d\n",param,rv,rlen);
           for (i = 0; i < rlen; i++) printf("%02x ",rdata[i]);
           printf("\n");
	}
	if (rv == 0) { /* if here, success */
          if (rlen > sdata) {
	     if (fdebug) 
		printf("dcmi_get_power_read(%d): data truncated from %d to %d\n",
			param,rlen,sdata);
	     rlen = sdata;
	  }
	  memcpy(pdata,rdata,rlen);
	}
	return(rv);
}

static int dcmi_get_power_limit( uchar *pdata, int sdata)
{
	uchar idata[5];
	uchar rdata[32];
	int rlen;
	uchar cc;
	int rv, i;

	if (pdata == NULL || sdata == 0) return(ERR_BAD_PARAM);
	idata[0] = 0xDC;
	idata[1] = 0x00;
	idata[2] = 0x00;
	rlen = sizeof(rdata);
        rv = ipmi_cmdraw( CMD_DCMI_GET_POWLIMIT, NETFN_DCMI,
			    g_sa, g_bus, g_lun,
                        idata,3, rdata, &rlen, &cc, fdebug);
	if (rv == 0) rv = cc;
	if (fdebug) {  /*show raw response*/
           printf("dcmi_get_power_limit: rv = %d rlen = %d\n",rv,rlen);
           for (i = 0; i < rlen; i++) printf("%02x ",rdata[i]);
           printf("\n");
	}
	if (rv == 0) { /* if here, success */
          if (rlen > sdata) {
	     if (fdebug) 
		printf("dcmi_get_power_limit: data truncated from %d to %d\n",
			rlen,sdata);
	     rlen = sdata;
  	  }
	  memcpy(pdata,rdata,rlen);
	}
	return(rv);
}

static int dcmi_set_power_limit(int param,int value, uchar *pow, int spow)
{
	int rv = 0;
	uchar idata[32];
	uchar rdata[32];
	int rlen;
	uchar cc;

	if (spow > 15) spow = 15;
	memcpy(idata,pow,spow);
	switch(param)
	{
		case CORRECTION_TIME:
			idata[10]=(uchar)((value >> 24) & 0xff);
			idata[9] =(uchar)((value >> 16) & 0xff);
			idata[8] =(uchar)((value >> 8) & 0xff);
			idata[7] =(uchar)((value) & 0xff);
			break;
		case EXCEPTION_ACTION:
			idata[4] = (uchar)(value & 0xff);
			break;
		case POWER_LIMIT:
			idata[5] = (uchar)(value & 0xff);
			idata[6] = (uchar)((value >> 8) & 0xff);
			break;
		case SAMPLE_PERIOD:
			idata[14] = (uchar)((value >> 8) & 0xff);
			idata[13] = (uchar)(value & 0xff);
			break;
		default:
			return ERR_BAD_PARAM;
			break;
	}
	rlen = sizeof(rdata);
	rv = ipmi_cmdraw( CMD_DCMI_SET_POWLIMIT, NETFN_DCMI,
			    g_sa, g_bus, g_lun,
				idata,15, rdata, &rlen,&cc, fdebug);
	if (fdebug)  
		printf("dcmi_set_power_limit(%d,%d): rv = %d cc = %x\n",
			param,value,rv,cc);
	if (rv == 0) rv = cc;
        return (rv);
}


static int dcmi_power_limit_activate ( int yes)
{
	int rv;
	uchar idata[5];
	uchar rdata[32];
	int rlen;
	uchar cc;
	if (yes < 0) return (ERR_BAD_PARAM);
	if (yes > 1) return (ERR_BAD_PARAM);

	idata[0] = 0xDC;
	idata[1] = yes;
	idata[2] = 0x00;
	idata[3] = 0x00;
	rlen = sizeof(rdata);
	rv = ipmi_cmdraw( CMD_DCMI_ACT_POWLIMIT, NETFN_DCMI,
			    g_sa, g_bus, g_lun,
                        idata,4, rdata, &rlen, &cc, fdebug);
	if (fdebug)  
           printf("dcmi_power_limit_activate(%d): rv = %d cc = %x\n",yes,rv,cc);
	if (rv == 0) rv = cc;
	return(rv);
}

void dcmi_show_power_read(int parm, uchar *cdata, int sdata)
{
	int i;
	ulong sample_period;
	time_t t = 0;
	uchar state;

	if (fdebug) { /*++++*/
           printf("dcmi_show_power_read(%d,%p,%d) called\n",parm,cdata,sdata);
  	   for (i = 0; i < sdata; i++) printf("%02x ",cdata[i]);
	   printf("\n");
	}
	if (sdata < 18) { 
	   printf("power_read data length %d is too short\n",sdata); 
	   return; 
	}
	if (cdata[0] != 0xDC) {
	   printf("power_read: invalid first data byte (0x%02x)\n",cdata[0]); 
	   return; 
	}
	memcpy(&t,&cdata[9],4);
	sample_period =   cdata[13];
	sample_period += (cdata[14] << 8);
	sample_period += (cdata[15] << 16);
	sample_period += (cdata[16] << 24);
	state = cdata[17];
	switch(parm) {
		case 1: /* Mode 1 - System Power Statistics */
    printf("  Current Power:                   %d Watts\n",cdata[1]+(cdata[2]<<8));
    printf("  Min Power over sample duration:  %d Watts\n",cdata[3]+(cdata[4]<<8));
    printf("  Max Power over sample duration:  %d Watts\n",cdata[5]+(cdata[6]<<8));
    printf("  Avg Power over sample duration:  %d Watts\n",cdata[7]+(cdata[8]<<8));
    printf("  Timestamp:                       %s\n",ctime(&t));
    printf("  Sampling period:                 %lu ms\n",sample_period);
    printf("  Power reading state is:          %s\n",(state&0x40)? "active":"not active");
		break;
		case 2: /* Mode 2 - Enhanced System Power Statistics */
		 printf("Enhanced Power Mode 2 decoding not yet implemented\n");
			/* TODO */
			// break;
		default:
			for (i = 0; i < sdata; i++) printf("%02x ",cdata[i]);
			printf("\n");
			break;
	}
}

void dcmi_show_power_limit(uchar *cdata, int sdata, int rv)
{
   ulong correction_time;
   char *pstr;

   correction_time =   cdata[6];
   correction_time += (cdata[7] << 8);
   correction_time += (cdata[8] << 16);
   correction_time += (ulong)(cdata[9] << 24);

   if (rv == 0) pstr = "(active)";
   else if (rv == 0x80) pstr = "(inactive)";
   else pstr = "(error)";
   printf("  Exception Action:  ");
   if (cdata[3] & 0x01)
	printf("Hard Power off\n");
   else if ((cdata[3] & 0x11) == 0x11)
	printf("SEL logging\n");
   else
	printf("OEM defined\n");
   printf("  Power Limit:       %d Watts %s\n",cdata[4]+(cdata[5]<<8), pstr);
   printf("  Correction Time:   %lu ms\n",   correction_time);
   printf("  Sampling period:   %d sec\n",  cdata[12]+(cdata[13]<<8));
}

static int dcmi_get_sensorinf(uchar styp, uchar snum, uchar offset, 
				uchar *pdata, int sdata)
{
	uchar idata[5];
	uchar rdata[32];
	int rlen;
	uchar cc;
	int rv, i;

	if (pdata == NULL || sdata == 0) return(ERR_BAD_PARAM);
	idata[0] = 0xDC;
	idata[1] = styp;  /*sensor type, 0x01 = Temperature*/
	idata[2] = snum;  /*sensor number */
	idata[3] = 0x00;  /*Entity Instance, 0 = all instances*/
	idata[4] = offset;  /*Entity Instance start/offset */
	rlen = sizeof(rdata);
        rv = ipmi_cmdraw( CMD_DCMI_GET_SENSORINF, NETFN_DCMI,
			    g_sa, g_bus, g_lun,
                        idata,5, rdata, &rlen, &cc, fdebug);
	if (rv == 0) rv = cc;
	if (fdebug) {  /*show raw response*/
           printf("dcmi_get_sensorinf(%d): rlen = %d\n",snum,rlen);
           for (i = 0; i < rlen; i++) printf("%02x ",rdata[i]);
           printf("\n");
	}
	if (rv == 0) { /* if here, success */
          if (rlen > sdata) {
	     if (fdebug) 
		printf("dcmi_get_sensorinf(%d): data truncated from %d to %d\n",
			snum,rlen,sdata);
	     rlen = sdata;
	  }
	  memcpy(pdata,rdata,rlen);
	}
	return(rv);
}

static char *entstr(int i)
{
   char *pstr = NULL;
   switch(i) {
   case 0: pstr = "Inlet"; break;
   case 1: pstr = "CPU"; break;
   case 2: 
   default:
	  pstr = "Baseboard"; break;
   }
   return(pstr);
}

static int dcmi_get_sensors(int styp)
{
  int i, j, n, r, recs;
  uchar offset, id;
  uchar dbuf[20];
  uchar s1;
  int rv;
  uchar *sdrs = NULL;
  uchar sdr[128];

  offset = 0;
  r = 1; n = 8;
  s1 = 0x40;  /*0x40,0x41,0x42*/
  rv = get_sdr_cache(&sdrs);

  printf("---Sensors---\n");
  for (i = 0; i < 3; i++) 
  {
     rv = dcmi_get_sensorinf(styp,s1+i,offset,dbuf,sizeof(dbuf));
     if (rv != 0) break;
     n = dbuf[1];
     recs = dbuf[2];
     printf("   %d %s temp sensors:  \tn_returned=%d\n",n,entstr(i),recs);
     for (r = 0; r < n; r += recs)  
     {
        recs = dbuf[2];
        if (recs == 0) break;
        for (j = 0; j < r; j++) {
            id = (dbuf[4 + (2*j)] << 8) + dbuf[3 + (2*j)]; 
            if (fdebug) printf("j=%d id=%x \n",j,id);
            /* get the sensor info for each record id */
	    rv = find_sdr_next(sdr,sdrs,(id-1));
	    ShowSDR("",sdr);
        }
        offset += recs;
        rv = dcmi_get_sensorinf(1,s1+i,offset,dbuf,sizeof(dbuf));
        if (rv != 0) break;
     }
  }
  if (rv != 0) printf("dcmi_get_sensors(%d,%d) error %d\n",styp,i,rv);
  free_sdr_cache(sdrs);
  return(rv);
}

static int dcmi_get_asset_tag(char *pdata, int sdata, int *dlen)
{
	uchar idata[4];
	uchar rdata[32];
	int rlen;
	uchar cc;
	int n, rv = -1;
	int sz_chunk = 16;
	int sz_all;

	if (pdata == NULL || sdata < 16) return(ERR_BAD_PARAM);
        memset(pdata,0,sdata);
	sz_all = sdata;
	for (n = 0; (n < sdata && n < sz_all); n += sz_chunk) {
	   idata[0] = 0xDC;
	   idata[1] = n;   /*offset*/
	   idata[2] = sz_chunk;  /*bytes to read*/
	   rlen = sizeof(rdata);
	   rv = ipmi_cmdraw( CMD_DCMI_GET_ASSETTAG, NETFN_DCMI,
			    g_sa, g_bus, g_lun,
                           idata,3, rdata, &rlen, &cc, fdebug);
	   if (fdebug)
              printf("dcmi_get_asset(%d): rv=%d ccode=%02x rlen=%d\n",
			n,rv,cc,rlen);
	   if (rv == 0) rv = cc;
	   if (rv == 0) { /* if here, success */
	      if (n == 0) sz_all = rdata[1];
		  if ((rlen - 2) < sz_chunk) sz_chunk = rlen - 2;
	      if ((n + sz_chunk) > sdata) {
	         if (fdebug) 
		    printf("dcmi_get_asset(%d): data truncated from %d to %d\n",
			   n,(n+sz_chunk),sdata);
	         sz_chunk = (sdata - n);
	      }
	      memcpy(&pdata[n],&rdata[2],sz_chunk);
	   } else break;
	} /*end-for loop*/
	pdata[n] = 0;  /*stringify*/
	if (dlen != NULL) *dlen = n;
	return(rv);
}

static int dcmi_get_mc_id(char *pdata, int sdata, int *dlen)
{
	uchar idata[4];
	uchar rdata[32];
	int rlen;
	uchar cc;
	int n, rv = -1;
	int sz_chunk = 16;
	int sz_all;

	if (pdata == NULL || sdata < 16) return(ERR_BAD_PARAM);
        memset(pdata,0,sdata);
	sz_all = sdata;
	for (n = 0; (n < sdata && n < sz_all); n += sz_chunk) {
	   idata[0] = 0xDC;
	   idata[1] = n;   /*offset*/
	   idata[2] = sz_chunk;  /*bytes to read*/
	   rlen = sizeof(rdata);
	   rv = ipmi_cmdraw( CMD_DCMI_GET_MCIDSTR, NETFN_DCMI,
			    g_sa, g_bus, g_lun,
                           idata,3, rdata, &rlen, &cc, fdebug);
	   if (fdebug)
              printf("dcmi_get_mc_id(%d): rv=%d ccode=%02x rlen=%d\n",
			n,rv,cc,rlen);
	   if (rv == 0) rv = cc;
	   if (rv == 0) { /* if here, success */
	      if (n == 0) sz_all = rdata[1];
		  if ((rlen - 2) < sz_chunk) sz_chunk = rlen - 2;
	      if ((n + sz_chunk) > sdata) {
	         if (fdebug) 
		    printf("dcmi_get_mc_id(%d): data truncated from %d to %d\n",
			   n,(n+sz_chunk),sdata);
	         sz_chunk = (sdata - n);
	      }
	      memcpy(&pdata[n],&rdata[2],sz_chunk);
	   } else break;
	} /*end-for loop*/
	pdata[n] = 0;  /*stringify*/
	if (dlen != NULL) *dlen = n;
	return(rv);
}

static int dcmi_set_asset_tag(char *pdata, int sdata)
{
	int rv = -1;
	uchar idata[32];
	uchar rdata[8];
	uchar cc;
	int ilen, rlen, n;
	int sz_chunk = 16;

	if (pdata == NULL || sdata < 2) return(ERR_BAD_PARAM);
	for (n = 0; (n < sdata); n += sz_chunk) {
	   idata[0] = 0xDC;
	   idata[1] = n;   /*offset*/
	   idata[2] = sz_chunk;  /*bytes to read*/
	   memset(&idata[3],0x20,sz_chunk);
	   ilen = 3 + sz_chunk;
	   if ((n + sz_chunk) > sdata) sz_chunk = sdata - n;
	   memcpy(&idata[3],&pdata[n],sz_chunk);
	   rlen = sizeof(rdata);
	   rv = ipmi_cmdraw( CMD_DCMI_SET_ASSETTAG, NETFN_DCMI,
			    g_sa, g_bus, g_lun,
                           idata,ilen, rdata, &rlen, &cc, fdebug);
	   if (rv == 0) {
		if (fdebug) 
			printf("dcmi_set_asset_tag(%d,%d,%d) cc=%x resp: %02x %02x %02x\n",
				sdata,n,sz_chunk,cc,rdata[0],rdata[1],rdata[2]);
		rv = cc;
	   }
	   if (rv != 0) break;
	}
	return(rv);
}

static int dcmi_set_mc_id(char *pdata, int sdata)
{
	int rv = -1;
	uchar idata[32];
	uchar rdata[8];
	uchar cc;
	int ilen, rlen, n;
	int sz_chunk = 16;

	if (pdata == NULL || sdata < 2) return(ERR_BAD_PARAM);
	for (n = 0; (n < sdata); n += sz_chunk) {
	   if ((n + sz_chunk) > sdata) sz_chunk = sdata - n;
	   idata[0] = 0xDC;
	   idata[1] = n;   /*offset*/
	   idata[2] = sz_chunk;  /*bytes to read*/
	   memset(&idata[3],0x20,sz_chunk);
	   ilen = 3 + sz_chunk;
	   if ((n + sz_chunk) > sdata) sz_chunk = sdata - n;
	   memcpy(&idata[3],&pdata[n],sz_chunk);
	   rlen = sizeof(rdata);
	   rv = ipmi_cmdraw( CMD_DCMI_SET_MCIDSTR, NETFN_DCMI,
			    g_sa, g_bus, g_lun,
                           idata,ilen, rdata, &rlen, &cc, fdebug);
	   if (rv == 0) {
		if (fdebug) printf("dcmi_set_mc_id(%d,%d,%d) resp: %02x %02x %02x\n",
				sdata,n,sz_chunk,rdata[0], rdata[1], rdata[2]);
		rv = cc;
	   }
	   if (rv != 0) break;
	}
	return(rv);
}

static int dcmi_show_asset_tag(void)
{
   int rv = -1;
   rv = dcmi_get_asset_tag(asset,sizeof(asset),&asset_len);
   if (rv == 0) printf("DCMI Asset Tag:          \t%s\n",asset);
   return(rv);
}

static int dcmi_show_mc_id(void)
{
   int rv = -1;
   rv = dcmi_get_mc_id(mc_id,sizeof(mc_id),&mcid_len);
   if (rv == 0) printf("DCMI Mgt Controller ID:  \t%s\n",mc_id);
   return(rv);
}

static char *supported[2] = { "Unsupported", "Supported" };

void dcmi_show_capab(int parm, uchar *cdata, int sdata)
{
   char mystr[64] = "";
   int i, j, k, n, f;
   switch(parm) 
   {
	case 1:
	     printf("DCMI Version:           \t%d.%d\n",cdata[1],cdata[2]);
	     dcmi_ver = ((cdata[1] & 0x0f) << 4) + (cdata[2] & 0x0f);
	     if (cdata[5] & 0x01) { fpwm = 1; }
	     else { fpwm = 0; }
	     printf("DCMI Power Management:   \t%s\n",supported[fpwm]);
	     if (cdata[6] & 0x01) f = 1;
	     else f = 0;
	     printf("DCMI System Interface Access:\t%s\n",supported[f]);
	     if (cdata[6] & 0x02) f = 1;
	     else f = 0;
	     printf("DCMI Serial TMode Access:\t%s\n",supported[f]);
	     if (cdata[6] & 0x02) f = 1;
	     else f = 0;
	     printf("DCMI Secondary LAN Channel:\t%s\n",supported[f]);
	     break;
	case 2:
	     mystr[0] = 0;
	     if (cdata[5] & 0x80) strcat(mystr,"Overwrite ");
	     else strcat(mystr,"NoOverwrite ");
	     if (cdata[5] & 0x40) strcat(mystr,"FlushAll ");
	     if (cdata[5] & 0x20) strcat(mystr,"FlushRec");
	     printf("DCMI SEL Management:     \t%s\n",mystr);
	     n = ((cdata[5] & 0x0F) << 8) + cdata[4];
	     printf("DCMI SEL num entries:    \t%d\n",n);
	     i = cdata[8]; 
	     printf("DCMI Temperature Polling: \t%d sec\n",i);
	     break;
	case 3:
	     n = ((cdata[4] & 0xFE) >> 1);
	     printf("DCMI PWM Slave_Address:  \t%02x\n",n);
	     n = ((cdata[5] & 0xF0) >> 4);
	     printf("DCMI PWM Channel:        \t%02x\n",n);
	     printf("DCMI PWM Dev_Rev:        \t%02x\n",(cdata[5] & 0x0F));
	     break;
	case 4:
	     printf("DCMI LanPlus primary chan:\t%02x\n",cdata[4]);
	     printf("DCMI LanPlus secondary chan:\t%02x\n",cdata[5]);
	     printf("DCMI Serial channel:     \t%02x\n",cdata[6]);
	     break;
	case 5:
	     n = cdata[4];
	     if (n > (5 + sdata)) n = sdata - 5; /*truncate*/
	     for (i = 0; i < n; i++) {
	        j = (cdata[5+i] & 0x0f);
	        k = ((cdata[5+i] & 0xf0) >> 4);
		switch(k) {
		case 3: strcpy(mystr,"days");  break;
		case 2: strcpy(mystr,"hrs");  break;
		case 1: strcpy(mystr,"min");  break;
		case 0:
		default: strcpy(mystr,"sec");  break;
		}
	        printf("DCMI Power Stats Duration(%d):\t%d %s\n",i,j,mystr);
	     }
	     break;
	default:
	     printf("DCMI(%d) data: %02x %02x %02x %02x %02x %02x %02x %02x\n",
			parm,cdata[1],cdata[2],cdata[3],cdata[4],
			cdata[5], cdata[6], cdata[7], cdata[8]);
	     break;
   }
}

static int dcmi_show_all_capab(void)
{
   int rv = -1;
   int i;
   uchar cdata[32];

   for (i = 1; i <= 5; i++) 
   {
       /* only read power stats(5) when power management is supported */
       if (i == 5 && fpwm == 0) continue;
       rv = dcmi_get_capab(i, cdata, sizeof(cdata));
       if (rv != 0) {
	  if (i == 1 && rv == CC_DCMI_INVALID_COMMAND) {  /*0xC1 on first*/
		printf("DCMI not supported on this platform\n");
		break;
	  } else if (i == 5 && rv == CC_DCMI_RECORD_NOT_PRESENT)  /*0xCB*/
		rv = 0;   /*optional, ignore this error*/
	  /*else just dont show the param*/
       } else dcmi_show_capab(i,cdata,sizeof(cdata));

   } /*end-for dcmi_capab loop */
   return(rv);
}

#ifdef METACOMMAND
int i_dcmi(int argc, char **argv)
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
	int c, i;
	char *s1;
	uchar cdata[32];
	uchar powdata[32];

	printf("%s ver %s\n", progname,progver);
	parse_lan_options('V',"4",0);  /*default to admin priv*/

        while ( (c = getopt( argc, argv,"a:d:m:p:sT:V:J:EYF:P:N:R:U:Z:x?")) != EOF )
	switch (c) {
          case 'a': set_asset = 1; asset_new = optarg;  break;
          case 'd': set_mcid = 1; mcid_new = optarg; break;
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
          case 's': do_sensors = 1; break;
          case 'x': fdebug = 1; break;  /* debug messages */
          case 'p':    /* port */
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
          case '?': 
		return(dcmi_usage());
                break;
	}
        for (i = 0; i < optind; i++) { argv++; argc--; }

	if ((argc > 0) && strcmp(argv[0], "help") == 0) {
	   return(dcmi_usage());
        } 

        rv = ipmi_getdeviceid( cdata, sizeof(cdata),fdebug);
	if (rv == 0) {
	   uchar ipmi_maj, ipmi_min;
	   ipmi_maj = cdata[4] & 0x0f;
	   ipmi_min = cdata[4] >> 4;
	   show_devid( cdata[2],  cdata[3], ipmi_maj, ipmi_min);
	} else goto do_exit;

	if (set_asset) {
	   rv = dcmi_get_asset_tag(asset,sizeof(asset),&asset_len);
	   memset(asset,' ',asset_len); /*fill with spaces*/
           asset_len = strlen_(asset_new); 
	   if (asset_len >= sizeof(asset)) asset_len = sizeof(asset) - 1;
	   memcpy(asset,asset_new,asset_len);
	   asset[asset_len] = 0;
	   rv = dcmi_set_asset_tag(asset, asset_len); 
	   printf("Set DCMI Asset Tag to %s, ret = %d\n",asset_new,rv);
	}
	if (set_mcid) {
	   rv = dcmi_get_mc_id(mc_id,sizeof(mc_id),&mcid_len);
	   memset(mc_id,' ',mcid_len); /*fill with spaces*/
           mcid_len = strlen_(mcid_new); 
	   if (mcid_len >= sizeof(mc_id)) mcid_len = sizeof(mc_id) - 1;
	   memcpy(mc_id,mcid_new,mcid_len);
	   mc_id[mcid_len] = 0;
	   rv = dcmi_set_mc_id(mcid_new, mcid_len);
	   printf("Set DCMI MC ID to %s, ret = %d\n",mcid_new,rv);
	}

        if ( (argc == 0) || (strcmp(argv[0], "info") == 0) ) {
	   rv = dcmi_show_all_capab();
	   if (rv == 0) {
	      rv = dcmi_show_mc_id();
	      rv = dcmi_show_asset_tag();
	      if (do_sensors) rv = dcmi_get_sensors(1);  /*temp sensors*/
	      rv = 0;  /*ignore errors for optional features*/
	   }
        } else 
        {
         rv = dcmi_get_capab(1, cdata, sizeof(cdata));
	 if (rv == 0) dcmi_show_capab(1,cdata,sizeof(cdata));
	 else if (rv == CC_DCMI_INVALID_COMMAND)   /*0xC1 on first*/
	   printf("DCMI not supported on this platform\n");

         if (strncmp(argv[0], "power",5) == 0)  {
	   if (fpwm == 0) {  /*not supported in capab */
	      printf("DCMI Power functions not supported on this platform.\n"); 
              rv = LAN_ERR_NOTSUPPORT;
	   } else {
	      rv = dcmi_get_power_read(1, cdata, sizeof(cdata));
	      if (rv == 0) {
		 dcmi_show_power_read(1,cdata,sizeof(cdata));
	         rv = dcmi_get_power_limit(powdata, sizeof(powdata));
	         if (rv == 0) 
		    dcmi_show_power_limit(powdata,sizeof(powdata),rv);
	         else if (rv == 0x80) {
		    dcmi_show_power_limit(powdata,sizeof(powdata),rv);
		    rv = 0;
	         }
	      }
	      if (argc < 2) ;   /*just get, done above*/
	      else if (strncmp(argv[1],"get",3) == 0) ;  /*done above*/
	      else if (strncmp(argv[1],"set_limit",9) == 0) {
	         if (argc < 3) return(dcmi_usage());
		 i = atoi(argv[2]);
		 {  /*let range checking be done by DCMI*/
		     rv = dcmi_set_power_limit (POWER_LIMIT, i, powdata,16);
		     switch(rv) {
		     case 0: 
			printf("DCMI Power limit applied successfully.\n");
			break;
		     case 0x84:  printf("Power limit out of range\n"); break;
		     case 0x85:  printf("Correction time out of range\n"); 
			break;
		     case 0x89:  
			printf("Statistics reporting period out of range\n"); 
			break;
		     default:
		        printf("DCMI Power Limit Set error %d\n",rv); break;
		     }
		     if (rv == 0) {
			rv = dcmi_power_limit_activate(1);
			printf ("DCMI Power Limit Activate returned %d\n",rv);
		     }
		 } 
	      }
	      else if (strncmp(argv[1],"activate",8) == 0) {
		 rv = dcmi_power_limit_activate(1);
		 if (rv == 0)
			printf ("DCMI Power Limit Activated.\n");
		 else   printf("DCMI Power Limit Activate error %d\n",rv);
	      }
	      else if (strncmp(argv[1],"deactivate",10) == 0) {
	   	 rv = dcmi_power_limit_activate(0);
		 if (rv == 0)
			printf("DCMI Power Limit Deactivated.\n");
		 else   printf("DCMI Power Limit Deactivate error %d\n",rv);
	      }
	      else if (strcmp(argv[1],"set_action")==0) {
	         if (argc < 3) return(dcmi_usage());
		 else if (strcmp(argv[2],"no_action") == 0)
			rv = dcmi_set_power_limit(EXCEPTION_ACTION,0x00,
						  powdata,16);
		 else if (strcmp(argv[2],"log_sel") == 0)
			rv = dcmi_set_power_limit(EXCEPTION_ACTION,0x11,
						  powdata,16);
		 else if (strcmp(argv[2],"power_off") == 0)
			rv = dcmi_set_power_limit(EXCEPTION_ACTION,0x01,
						  powdata,16);
		 else return(dcmi_usage());
		 if (rv == 0) 
			printf("exception action set successfully.\n");
		 else   printf("set_exception action error %d\n",rv);
	      }
	      else if (strcmp(argv[1],"set_sample")==0) {
	         if (argc < 3) return(dcmi_usage());
		 i = atoi(argv[2]);
		 if (i != 0) {
			rv = dcmi_set_power_limit (SAMPLE_PERIOD,i,powdata,16);
			if (rv == 0x00)
				printf("sample period set successfully\n");
			else if (rv == 0x89)
				printf("sample period %d out of range\n",i);
			else    printf("set_sample period error %d\n",rv);
		 }
		 else { printf("invalid sample period %d\n",i);
			rv = ERR_USAGE; }
	      }
	      else if (strcmp(argv[1],"set_correction")==0) {
	         if (argc < 3) return(dcmi_usage());
		 i = atoi(argv[2]);
		 if (i != 0) {
			rv = dcmi_set_power_limit(CORRECTION_TIME,i,powdata,16);
			if (rv == 0x00)
				printf("correction time set successfully\n");
			else if (rv == 0x85)
				printf("correction time %d out of range\n",i);
			else {
				dcmi_usage();
				rv = ERR_USAGE;
			}
	   	 } 
		 else { printf("correction time %d invalid\n",i);
			rv = ERR_USAGE; }
	      }
	      else {
		 printf("invalid subfunction %s\n",argv[1]);
		 rv = ERR_USAGE; 
	      }
	   } /* endif power functions supported */

         } else if (strcmp(argv[0], "thermal") == 0)  {
	   if (dcmi_ver < 0x15) {  /*not supported in DCMI < 1.5 */
	      printf("DCMI 1.5 Thermal functions not supported on this platform.\n");
              rv = LAN_ERR_NOTSUPPORT;
	   } else {
	      rv = dcmi_get_sensors(1);  /*temp sensors*/
/* These are DCMI 1.5 commands */
// #define CMD_DCMI_SET_THERMLIM  0x0B
// #define CMD_DCMI_GET_THERMLIM  0x0C
// #define CMD_DCMI_GET_TEMPRDNGS 0x10
/* TODO: implement these 3 DCMI thermal commands */
	      printf("DCMI 1.5 Thermal functions not yet implemented\n"); 
              rv = ERR_USAGE;
	   }
         } else if (strcmp(argv[0], "config") == 0)  {
	   if (dcmi_ver < 0x15) {  /*not supported in DCMI < 1.5 */
	      printf("DCMI 1.5 Config functions not supported on this platform.\n");
              rv = LAN_ERR_NOTSUPPORT;
	   } else { /* These are DCMI 1.5 commands */
// #define CMD_DCMI_SET_CONFIG    0x12
// #define CMD_DCMI_GET_CONFIG    0x13
/* TODO: implement these 2 DCMI config commands */
	      printf("DCMI 1.5 Config get/set functions not yet implemented\n");
              rv = ERR_USAGE;
	   }
         } else {
	   return(dcmi_usage());
	 }
        } /*else not info command*/

do_exit:
        ipmi_close_();
	return rv;
}
/* end idcmi.c */
