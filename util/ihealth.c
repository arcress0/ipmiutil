/*
 * ihealth.c (was bmchealth.c)
 *
 * This tool checks the health of the BMC via IPMI.
 * 
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2006 Intel Corporation. 
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 03/22/06 Andy Cress - created
 * 06/20/06 Andy Cress 0.6 - more vendor strings, add ping_node() stub for now
 * 10/20/06 Andy Cress 1.1 - added -g for guid
 * 01/10/07 Andy Cress 1.4 - added product strings
 * 02/25/07 Andy Cress 2.8 - added more Chassis Status decoding
 * 09/18/17 Andy Cress 3.07 - Set do_powerstate=0 for Sun, continue if failure
 */
/*M*
Copyright (c) 2006, Intel Corporation
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
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "getopt.h"
#elif defined(DOS)
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include "getopt.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#else
#include <getopt.h>
#endif
#endif
#include <string.h>
#include "ipmicmd.h"
#include "oem_intel.h"
 
#define  SELFTEST_STATUS   0x04
#define  GET_POWER_STATE   0x07
extern int get_BiosVersion(char *str);
extern int get_SystemGuid(uchar *guid);
extern int GetSDR(int id, int *next, uchar *recdata, int srecdata, int *rlen);
extern int get_device_guid(uchar *pbuf, int *sz); /*subs.c*/
extern int oem_supermicro_get_health(char *pstr, int sz);  /*oem_supermicro.c*/
extern int oem_supermicro_get_firmware_str(char *pstr, int sz); /*oem_supermicro.c*/

/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil health";
#else
static char * progver   = "3.08";
static char * progname  = "ihealth";
#endif
static char   fdebug    = 0;
static char   fipmilan  = 0;
static char   fcanonical = 0;
static char   do_hsc    = 0;
static char   do_me     = 0;
static char   do_frusdr = 0;
static char   do_guid   = 0;
static char   do_powerstate  = 1;
static char   do_lanstats    = 0;
static char   do_session     = 0;
static char   do_systeminfo  = 0;
static char   set_restore    = 0;
static char   set_name       = 0;
static char   set_os         = 0;
static char   set_os2        = 0;
static uchar  restore_policy = 0;
static uchar  bChan     = 0x0e;
static char   fmBMC     = 0;
static char   bdelim    = '=';  /*delimiter to separate name/value pairs*/
static char   bcomma    = ',';  /*comma delimiter, may change if CSV*/
static char   lan_ch_restrict = 0;
static int    kcs_loops = 0;
static int    vend_id = 0;
static int    prod_id = 0;
static char  *pname = NULL;
static char  *pos   = NULL;
static char  *pos2  = NULL;
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;


int oem_get_health(char *pstr, int sz)
{
   int rv;
   switch(vend_id) {
   case VENDOR_PEPPERCON:
   case VENDOR_SUPERMICRO:
	rv = oem_supermicro_get_health(pstr, sz);
	break;
   case VENDOR_SUPERMICROX:
	rv = oem_supermicro_get_firmware_str(pstr,sz);
	break;
   default:
	rv = LAN_ERR_NOTSUPPORT;
	break;
   }
   if (fdebug) printf("oem_get_health rv = %d\n",rv);
   return rv;
}

char *getdmiprod(void)
{
   char *prod = "";
#ifdef LINUX
   static char dmiprod[32];
   char cmd[160];
   char *dmitmp = "/tmp/dmi.tmp";
   FILE *fp;
   int rv, i;
   if (fipmilan) return(prod);
   sprintf(cmd,"dmidecode | grep -A6 \"^Base Board\" |grep 'Product Name' |cut -f2 -d':' |awk '{ print $1 }' >%s",dmitmp);
   rv = system(cmd); 
   if (rv == 0) { 
    fp = fopen(dmitmp,"r");
    if (fp == NULL) rv = -1;
    else {
      if (fgets(dmiprod, sizeof(dmiprod), fp) == NULL) rv = -2;
      else {
          for (i=0; i<sizeof(dmiprod); i++) {
             if (dmiprod[i] == '\n') { dmiprod[i] = '\0'; break; }
          }
          prod = dmiprod;
      }
      fclose(fp);
    }
   }
#endif
   return(prod);
}

int get_lan_stats(uchar chan)
{
   uchar idata[2];
   uchar rdata[20];
   int rlen, rv;
   uchar cc;
   ushort *rw;

   /* get BMC LAN Statistics */
   idata[0] = chan;
   idata[1] = 0x00;  /*do not clear stats*/
   rlen = sizeof(rdata);
   rv = ipmi_cmd(GET_LAN_STATS, idata,2, rdata,&rlen, &cc, fdebug); 
   if (fdebug) printf("get_lan_stats: rv = %d, cc = %02x\n",rv,cc);
   if (rv == 0) {
     if (cc == 0) {  /*success, show BMC LAN stats*/
	rw = (ushort *)&rdata[0];
	printf("IPMI LAN channel %d statistics: \n",chan);
	printf(" \tReceived IP Packets      %c %d\n",bdelim,rw[0]);
	printf(" \tRecvd IP Header errors   %c %d\n",bdelim,rw[1]);
	printf(" \tRecvd IP Address errors  %c %d\n",bdelim,rw[2]);
	printf(" \tRecvd IP Fragments       %c %d\n",bdelim,rw[3]);
	printf(" \tTransmitted IP Packets   %c %d\n",bdelim,rw[4]);
	printf(" \tReceived UDP Packets     %c %d\n",bdelim,rw[5]);
	printf(" \tReceived Valid RMCP Pkts %c %d\n",bdelim,rw[6]);
	printf(" \tReceived UDP Proxy Pkts  %c %d\n",bdelim,rw[7]);
	printf(" \tDropped UDP Proxy Pkts   %c %d\n",bdelim,rw[8]);
     } else if (cc == 0xc1) {
	printf("IPMI LAN channel %d statistics: not supported\n",chan);
     }
   }
   return(rv);
}

int get_session_info(uchar idx, int hnd, uchar *rdata, int *len)
{
   uchar idata[5];
   int ilen, rlen, rv;
   uchar cc;

   ilen = 1;
   idata[0] = idx;
   if (idx == 0xFE) {
      idata[1] = (uchar)hnd;
      ilen = 2;
   } else if (idx == 0xFF) {
      idata[1] = (uchar)(hnd & 0x000000FF);
      idata[2] = (uchar)((hnd & 0x0000FF00) >> 8);
      idata[3] = (uchar)((hnd & 0x00FF0000) >> 16);
      idata[4] = (uchar)((hnd & 0xFF000000) >> 24);
      ilen = 5;
   }
   rlen = *len;
   *len = 0;
   rv = ipmi_cmdraw(CMD_GET_SESSION_INFO,NETFN_APP,
                        g_sa, g_bus, g_lun,
                        idata,ilen, rdata,&rlen,&cc, fdebug);
   if (fdebug) printf("get_lan_stats: rv = %d, cc = %02x\n",rv,cc);
   if (rv == 0) *len = rlen;
   if ((rv == 0) && (cc != 0)) rv = cc;
   return(rv); 
}

static char *sesstype_str(uchar c)
{
   uchar b;
   char *s;
   b = ((c & 0xf0) >> 4);
   switch(b) {
      case 0: s = "IPMIv1.5"; break;
      case 1: s = "IPMIv2/RMCP+"; break;
      default: s = "Other"; break;
   }
   return s;
}
      
static void show_session_info(uchar idx, uchar *sinfo,int len)
{
   int i;
   char lan_type = 1;
   if (fdebug) {
      printf("Raw Session Info[%d]: ",idx);
      for (i = 0; i < len; i++) printf("%02x ",sinfo[i]);
      printf("\n");
   }

   printf("Session Info[%d]:\n",idx);
   printf("\tSession Handle      %c %d\n",bdelim,sinfo[0]);
   printf("\tSession Slot Count  %c %d\n",bdelim,(sinfo[1] & 0x3f));
   printf("\tActive Sessions     %c %d\n",bdelim,(sinfo[2] & 0x3f));
   if (len <= 3) return;
   printf("\tUser ID             %c %d\n",bdelim,(sinfo[3] & 0x3f));
   printf("\tPrivilege Level     %c %d\n",bdelim,(sinfo[4] & 0x0f));
   printf("\tSession Type        %c %s\n",bdelim,sesstype_str(sinfo[5]));
   printf("\tChannel Number      %c %d\n",bdelim,(sinfo[5] & 0x0f));
   if (len <= 6) return;
   if (lan_type) {
       printf("\tConsole IP          %c %d.%d.%d.%d\n",bdelim,
			sinfo[6],sinfo[7],sinfo[8],sinfo[9]);
       printf("\tConsole MAC         %c %02x:%02x:%02x:%02x:%02x:%02x\n",bdelim,
			sinfo[10], sinfo[11], sinfo[12],
			sinfo[13], sinfo[14], sinfo[15]);
       printf("\tConsole Port        %c %d\n",bdelim,
			sinfo[16]+ (sinfo[17] << 8));
   }
}

int get_session_info_all(void)
{
   int rv, len, nslots, i;
   uchar sinfo[24];
   nslots = 1;
   for (i = 1; i <= nslots; i++) {
      len = sizeof(sinfo);
      rv = get_session_info(i,0,sinfo,&len);
      if (fdebug) printf("get_session_info(%d): rv = %d\n",i,rv);
      if (rv != 0) {
         if ((rv == 0xCB) || (rv == 0xCC)) {
            if (len >= 3) show_session_info(i,sinfo,len);
	    if (i > 1) rv = 0; /*no such idx, end */
	 }
         break;
      }
      nslots = (sinfo[1] & 0x3F);
      show_session_info(i,sinfo,len);
   }
   return(rv); 
}

static int get_selftest_status(uchar *rdata, int rlen)
{
	uchar idata[4];
	uchar ccode;
	int ret;

        ret = ipmi_cmdraw( SELFTEST_STATUS, NETFN_APP,
                        g_sa, g_bus, g_lun,
                        idata,0, rdata,&rlen,&ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end get_selftest_status()*/

static int get_last_selftest(uchar *val, int vlen)
{
   uchar idata[4]; 
   uchar rdata[16]; 
   int rlen;
   uchar ccode;
   int ret;

   if (val == NULL) return(ERR_BAD_PARAM);
   idata[0] = 0;  /*0=first, 1=next*/
   memset(rdata,0xFF,2);  /*initial value = end-of-list*/
   rlen = sizeof(rdata);
   ret = ipmi_cmdraw( 0x16, 0x30, g_sa, g_bus, g_lun,
                        idata,1, rdata,&rlen,&ccode, fdebug);
   if (ret == 0 && ccode != 0) ret = ccode;
   if (ret == 0) {
      if (rlen <= 0) ret = LAN_ERR_BADLENGTH; 
      else {
        if (rlen > vlen) rlen = vlen;  /*truncate if too long*/
        memcpy(val,rdata,rlen);
      }
   }
   return(ret);
}

static int get_chassis_status(uchar *rdata, int *rsz)
{
	uchar idata[4];
	uchar ccode;
        int rlen;
	int ret;

	rlen = *rsz;
        ret = ipmi_cmdraw( CHASSIS_STATUS, NETFN_CHAS,
			g_sa, g_bus, g_lun,
                        idata,0, rdata,&rlen,&ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
        if (ret == 0) *rsz = rlen;
	return(ret);
}  /*end chassis_status()*/

static void show_chs_status(uchar *sbuf, int slen)
{
   char chs_strbuf[80];
   char *pstr;
   uchar state, b2, b3, b4;

   pstr = &chs_strbuf[0];
   state = sbuf[0] & 0x7f;
   b2 = sbuf[1];
   b3 = sbuf[2];
   sprintf(pstr,"%s",(state & 0x01) ? "on" : "off");
   printf("Chassis Status    %c %02x %02x %02x %02x (%s, see below)\n",
		bdelim,state,sbuf[1],sbuf[2],sbuf[3],pstr);
   sprintf(pstr,"\tchassis_power       %c ",bdelim);
   if (state & 0x01) strcat(pstr,"on");
   else strcat(pstr,"off");
   if (state & 0x02) strcat(pstr,", overload");
   if (state & 0x04) strcat(pstr,", interlock");
   if (state & 0x08) strcat(pstr,", fault");
   if (state & 0x10) strcat(pstr,", control error");
   printf("%s\n",pstr);

   sprintf(pstr,"\tpwr_restore_policy  %c ",bdelim);
   if (state & 0x20) strcat(pstr,"last_state");
   else if (state & 0x40) strcat(pstr,"turn_on");
   else strcat(pstr,"stay_off");
   printf("%s\n",pstr);

   if (b2 != 0) {
      sprintf(pstr,"\tlast_power_event    %c ",bdelim);
      if (b2 & 0x10) strcat(pstr,"IPMI ");
      if (b2 & 0x08) strcat(pstr,"fault ");
      if (b2 & 0x04) strcat(pstr,"interlock ");
      if (b2 & 0x02) strcat(pstr,"overload ");
      if (b2 & 0x01) strcat(pstr,"ACfailed");
      printf("%s\n",pstr);
   }
   printf("\tchassis_intrusion   %c %s\n", bdelim,
		(b3 & 0x01) ? "active":"inactive");
   printf("\tfront_panel_lockout %c %s\n", bdelim,
		(b3 & 0x02) ? "active":"inactive"); 
   printf("\tdrive_fault         %c %s\n", bdelim,
		(b3 & 0x04) ? "true":"false"); 
   printf("\tcooling_fan_fault   %c %s\n", bdelim,
		(b3 & 0x08) ? "true":"false"); 
   if (slen > 3) {
      b4 = sbuf[3];
      if (b4 & 0x80) {
	  printf("\tFP sleep_button_disable %c allowed, button %s\n",bdelim,
			(b4 & 0x08) ? "disabled":"enabled");
      }
      if (b4 & 0x40) {
	  printf("\tFP diag_button_disable  %c allowed, button %s\n",bdelim,
			(b4 & 0x04) ? "disabled":"enabled");
      }
      if (b4 & 0x20) {
	  printf("\tFP reset_button_disable %c allowed, button %s\n",bdelim,
			(b4 & 0x02) ? "disabled":"enabled");
      }
      if (b4 & 0x10) {
	  printf("\tFP power_button_disable %c allowed, button %s\n",bdelim,
			(b4 & 0x01) ? "disabled":"enabled");
      }
   }
   return;
}

static int get_power_state(uchar *rdata, int rlen)
{
	uchar idata[4];
	uchar ccode;
	int ret;

    ret = ipmi_cmdraw( GET_POWER_STATE, NETFN_APP, g_sa, g_bus, g_lun,
                        idata,0, rdata,&rlen,&ccode, fdebug);
	if (ret == 0 && ccode == 193) { /*0xB7, usu. SuperMicro, retry */
       ret = ipmi_cmdraw( GET_POWER_STATE, NETFN_APP, g_sa, g_bus, g_lun,
                        idata,0, rdata,&rlen,&ccode, fdebug);
	}
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end get_power_state()*/

static char *pwr_string(uchar pstate)
{
   char *pstr;
   switch(pstate) {
      case 0x00: pstr = "S0: working"; break; 
      case 0x01: pstr = "S1: clock stopped, context ok"; break; 
      case 0x02: pstr = "S2: clock stopped, context lost"; break; 
      case 0x03: pstr = "S3: suspend-to-RAM"; break; 
      case 0x04: pstr = "S4: suspend-to-Disk"; break; 
      case 0x05: pstr = "S5: soft off"; break; 
      case 0x06: pstr = "S4/S5: soft off, either S4 or S5"; break; 
      case 0x07: pstr = "G3: mechanical off"; break; 
      case 0x08: pstr = "S1-S3: sleeping"; break; 
      case 0x09: pstr = "S1-S4: sleeping"; break; 
      case 0x0A: pstr = "S5/o: soft off by override"; break; 
      case 0x20: pstr = "legacy on"; break; 
      case 0x21: pstr = "legacy soft-off"; break; 
      case 0x2a:  /* not initialized or device lost track of state */
      default:   pstr = "unknown"; break;
   }
   return(pstr);
}

#ifdef PING_OK
extern int ping_bmc(char *node, char fdebug);

static int ping_node(char *node)
{
   int rv = 0;
   /* verify that the BMC LAN channel is configured & active */
   /* send rmcp_ping to node's BMC */
   rv = ping_bmc(node,fdebug);
   return(rv);
}
#endif

#define MIN_SDR_SZ  8
static int get_frusdr_version(char *pver, int sver)
{
   ushort recid;
   int recnext;
   int ret, sz, i, len;
   uchar sdr[MAX_BUFFER_SIZE];
   char verstr[30];
   char fIntel;
   int verlen;

   recid = 0;
   verstr[0] = 0;
   verlen = 0;
   while (recid != 0xffff) 
   {
      memset(sdr,0,sizeof(sdr)); 
      ret = GetSDR(recid,&recnext,sdr,sizeof(sdr),&sz);
      if (fdebug)
         printf("GetSDR[%04x]: ret = %x, next=%x\n",recid,ret,recnext);
      if (ret != 0) {
         if (ret > 0) {  /* ret is a completion code error */
            if (fdebug)
                printf("%04x GetSDR error 0x%02x %s, rlen=%d\n",recid,ret,
			decode_cc((ushort)0,(uchar)ret),sz);
         } else printf("%04x GetSDR error %d, rlen = %d\n", recid,ret,sz);
         if (sz < MIN_SDR_SZ) {  /* don't have recnext, so abort */
              break;
         } /* else fall through & continue */
      } else { /*got SDR */
	 len = sdr[4] + 5;
	 if (sdr[3] == 0xC0) {  /* OEM SDR */
	     /* check for Intel mfg id  */
	     if ((sdr[5] == 0x57) && (sdr[6] == 0x01) && (sdr[7] == 0x00))
		fIntel = 1;
	     else fIntel = 0;
	     if (sdr[8] == 0x53) {  /*Intel OEM subtype, ASCII 'S' */
		verlen = 0;
		for (i = 8; i < len; i++) {
		   if (sdr[i] == 0) break;
		   if (i >= sizeof(verstr)) break;
                   verstr[verlen++] = sdr[i];
            	}
                verstr[verlen] = 0;  /*stringify*/
		/* continue on past SDR File, get SDR Package version */
		// break;
	     }
         }  /*endif OEM SDR*/
      }
      if (recnext == recid) recid = 0xffff; /*break;*/
      else recid = (ushort)recnext;
   }
   if (verlen > sver) verlen = sver;
   if (fdebug) 
	printf("get_frusdr_version: verstr=%s, verlen=%d\n",verstr,verlen);
   strncpy(pver,verstr,verlen);
   return(ret);
}

static int get_hsc_devid(uchar *rdata, int rlen)
{
	uchar ccode;
	int ret;

        ret = ipmi_cmdraw( 0x01, /*(GET_DEVICEID & 0x00ff)*/ 
			NETFN_APP, 0xC0,PUBLIC_BUS,BMC_LUN,
                        NULL,0, rdata,&rlen,&ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end get_hsc_devid*/

static int get_chan_auth(uchar chan, uchar *rdata, int rlen)
{
	uchar idata[4];
	uchar ccode;
	int ret;

        idata[0] = chan; /*0x0e = this channel*/
        idata[1] = 0x02; /*priv level = user*/
        ret = ipmi_cmdraw( 0x38, NETFN_APP, /*CMD_GET_CHAN_AUTH_CAP*/ 
			g_sa, g_bus, g_lun,
                        idata,2, rdata,&rlen,&ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end get_chan_auth*/

void show_chan_auth(char *tag, uchar *rec, int srec)
{
   char pstr[40];
   pstr[0] = 0;
   if (rec[1] & 0x01) strcat(pstr,"None ");
   if (rec[1] & 0x02) strcat(pstr,"MD2 ");
   if (rec[1] & 0x04) strcat(pstr,"MD5 ");
   if (rec[1] & 0x10) strcat(pstr,"Straight_Passwd ");
   if (rec[1] & 0x20) strcat(pstr,"OEM ");
   printf("Chan %d AuthTypes  %c %s\n",rec[0],bdelim,pstr);
   if (do_hsc)  /*only show this if extra output*/
     printf("Chan %d Status     %c %02x, OEM ID %02x%02x%02x OEM Aux %02x\n",
            rec[0],bdelim,rec[2],rec[4],rec[5],rec[6],rec[7]);
}

#define BMC  1
#define HSC  2

#ifdef MOVED
/* moved to subs.c*/
#define N_MFG  41
struct { int val; char *pstr; } mfgs[N_MFG] = { };
char * get_iana_str(int mfg);
#endif

char * get_mfg_str(uchar *rgmfg, int *pmfg)
{
      char *mfgstr = "";
      int mfg;
      mfg  = rgmfg[0] + (rgmfg[1] << 8) + (rgmfg[2] << 16);
      if (pmfg != NULL) *pmfg = mfg;  /*vend_id*/
      mfgstr = get_iana_str(mfg);
      return(mfgstr);
}

/* int get_system_info(uchar parm, char *pbuf, int *szbuf);  *see subs.c*/
/* int set_system_info(uchar parm, uchar *pbuf, int *szbuf);  *see subs.c*/

void show_devid_all(int dtype, uchar *devrec, int sdevrec)
{
   uchar ipmi_maj = 0;
   uchar ipmi_min = 0;
   char *tag; 
   int mfg, prod;
   char *mfgstr = "";
   char *prodstr = "";
   char prodoem[40];
   char extraver[32];
   int i, j, k, l, rv;

      ipmi_maj = devrec[4] & 0x0f;
      ipmi_min = devrec[4] >> 4;
      prod = devrec[9] + (devrec[10] << 8);
      mfgstr = get_mfg_str(&devrec[6],&mfg);
      vend_id = mfg;
      prod_id = prod;
      extraver[0] = 0;
      if (dtype == HSC) tag = "HSC";
      else {
	tag = "BMC";
	/* The product ids below only apply to BMCs */
	switch(mfg) {
         case VENDOR_NSC:      /*=0x000322*/
             fmBMC = 1;
             if (dtype == BMC) tag="mBMC";
             if (prod == 0x4311) prodstr = "(TIGPT1U)"; /*Intel*/
             break;
         case VENDOR_SUN:      /*=0x00002a*/
             if (prod == 0x4701) prodstr = "(X4140)"; 
             do_powerstate = 0;
             break;
         case VENDOR_TYAN:      /*=0x0019fd*/
             switch(prod) {     /* show product names for some */
                 case 0x0b41:   prodstr = "(M3289)"; break;
                 case 0x0f98:   prodstr = "(M3291)"; break;
                 case 0x137d:   prodstr = "(S4989)"; break;
                 case 0x13ee:   prodstr = "(S5102)"; break;
                 case 0x14fc:   prodstr = "(S5372)"; break;
                 default:       prodstr = ""; break;
             }
             break;
         case VENDOR_FUJITSU:      /*=0x002880*/
             if (prod >= 0x200) prodstr = "(iRMC S2)"; 
             else prodstr = "";
             break;
         case VENDOR_CISCO:      /*=0x00168b, 5771.*/
             if (prod == 0x0005) prodstr = "(UCS C200)"; 
             else prodstr = "";
             if (fipmilan) lan_ch_restrict = 1; /*fw bug, gets 0xC1 on ipmilan*/
             break;
         case 0x003C0A:        /*=15370, Giga-Byte*/
             prodstr = "";
             lan_ch_restrict = 1; /*fw bug, gets 0xC1*/
             break;
         case VENDOR_INTEL:     /*=0x000157*/
	         if (do_hsc && (dtype == BMC)) /*if HSC option, also show extra*/
	         sprintf(extraver," (Boot %x.%x PIA %x.%x)", /*BMC extra*/
			     devrec[11],devrec[12],devrec[13],devrec[14]);
             switch(prod) {     /* show product names for some */
                 case 0x000C:   prodstr = "(TSRLT2)";    /*SCB2*/
                                bChan = 7;  break;
                 case 0x001B:   prodstr = "(TIGPR2U)";   /*SWV2*/
                                bChan = 7;  break;
                 case 0x0022:   prodstr = "(TIGI2U)"; break;   /*SJR2*/
                 case 0x0026:   prodstr = "(Bridgeport)"; break;
                 case 0x0028:   prodstr = "(S5000PAL)"; break; /*Alcolu*/
                 case 0x0029:   prodstr = "(S5000PSL)"; break; /*StarLake*/
                 case 0x002B:   prodstr = "(S5000VSA)"; break; 
                 case 0x002D:   prodstr = "(MFSYS25)"; break; /*ClearBay*/
                 case 0x003E:   prodstr = "(S5520UR)";   /*CG2100 or NSN2U*/
                                do_me = 1; kcs_loops = URNLOOPS;
                                bChan = 1;  break;
                 case 0x0040:   prodstr = "(QSSC-S4R)";  /*Stoutland*/
                                do_me = 1; kcs_loops = URNLOOPS;
                                bChan = 1;  break;
                 case 0x0100:   prodstr = "(Tiger4)"; break;
                 case 0x0103:   prodstr = "(McCarran)";  /*BladeCenter*/
                                do_powerstate = 0;   break; 
                 case 0x0800:   prodstr = "(ZT5504)";    /*ZiaTech*/
                                do_powerstate = 0;   break; 
                 case 0x0808:   prodstr = "(MPCBL0001)";   /*ATCA Blade*/
                                do_powerstate = 0;   break; 
                 case 0x0841:   prodstr = "(MPCMM0001)";   /*ATCA CMM*/
                                do_powerstate = 0;   break; 
                 case 0x0811:   prodstr = "(TIGW1U)";  break; /*S5000PHB*/
                 case 0x4311:   prodstr = "(NSI2U)";     /*SE7520JR23*/
                                if (dtype == BMC) tag="mBMC";
                                fmBMC = 1; break;
                 default:       prodstr = ""; break;
             }
             if (is_romley(mfg,prod)) {
	        intel_romley_desc(mfg,prod,&prodstr);
	        snprintf(prodoem,sizeof(prodoem),"(%s)",prodstr);
	        prodstr = prodoem;
	        do_me = 1; kcs_loops = URNLOOPS; 
	        do_hsc = 1; /*the HSC is embedded, so not the same*/
	        sprintf(extraver,".%d (Boot %x.%x)", /*BMC extra*/
			(devrec[13] + (devrec[14] << 8)),devrec[11],devrec[12]);
             }
             if (is_grantley(mfg,prod)) {
	           intel_grantley_desc(mfg,prod,&prodstr);
             }
             break;
         case VENDOR_KONTRON:     /*=0x003A98=15000.*/
	     i = devrec[11] + (devrec[12] << 8);
	     j = devrec[13] + (devrec[14] << 8);
	     k = 0; l = 0;
	     { /* get Kontron firmware version with OEM cmd */
		int rlen;
		uchar idata[4];
		uchar rdata[16];
		uchar cc;
                rlen = sizeof(rdata);
	        idata[0] = 0;
	        idata[1] = 0;
	        idata[2] = 1;
	        rv = ipmi_cmdraw(0x2f, 0x2c, g_sa, g_bus, g_lun,
				   idata,3,rdata,&rlen,&cc,fdebug);
                if (rv == 0 && cc == 0) {
		   k = rdata[1];
		   l = rdata[2];
	        }
	     }
	     sprintf(extraver,".%02d.%02d (FW %x.%x)",i,j,k,l);
             switch(prod) {     /* show product names for some */
                 case 0x1590:   prodstr = "(KTC5520)";  break;
                 default:       prodstr = ""; break;
             }
             break;
         case VENDOR_PEPPERCON:  /*=0x0028c5 Peppercon/Raritan */
	     if (prod == 0x0004) prodstr = "(AOC-IPMI20)"; /*SuperMicro*/
	     else if (prod == 0x0007) prodstr = "(RMM2)";  /*Intel RMM2*/
             break;
         case VENDOR_HP:        /*=0x00000B*/
             switch(prod) {     /* show product names for some */
                 case 0x2000:   prodstr = "(Proliant ML/DL)"; break; /*DL380*/
                 case 0x2020:   prodstr = "(Proliant BL)"; break;
                 default:  
                   if ((prod & 0xff00) == 0x8300) prodstr = "(Proliant SL)";
                   else prodstr = ""; 
                   break;
             }
             do_powerstate = 0; /*HP does not support get_power_state cmd*/
             if (!fipmilan) lan_ch_restrict = 1; /*fw bug, gets 0xcc locally*/
             break;
         case VENDOR_DELL:        /*=0x0002A2*/
             switch(prod) {     /* show product names for some */
                 case 0x0100:   prodstr = "(PE R610)"; break;
                 default:       prodstr = ""; break;
             }
             break;
         case VENDOR_MAGNUM:     /* =5593. used by SuperMicro*/
             switch(prod) {     /* show product names for some */
		 case 6:	prodstr = "(X8DTL)"; break;
                 default:       prodstr = ""; break;
             }
             break;
         case VENDOR_SUPERMICRO:    /* =10876. used by SuperMicro*/
         case VENDOR_SUPERMICROX:   /* =47488. used by Winbond/SuperMicro*/
             switch(prod) {    /* decode some SuperMicro product ids */
		 case 4:	prodstr = "(X7DBR)"; break;
		 case 6:	prodstr = "(X8DTL)"; break;
		 case 1037:     prodstr = "(X8SIE)"; break;
		 case 1541:     prodstr = "(X8SIL)"; break;
		 case 1547:	prodstr = "(X8SIA)"; break; /*0x060b*/
		 case 1549:     prodstr = "(X8DTU)"; break;
		 case 1551:     prodstr = "(X8DTN)"; break;
		 case 1562:	prodstr = "(X8SIU-F)"; break; /*0x061a*/
		 case 1572:     prodstr = "(X9SCM)"; break; /*or X9SCL*/
		 case 1576:     prodstr = "(X9DRi)"; break; 
		 case 1585:     prodstr = "(X9SCA)"; break;
		 case 1603:     prodstr = "(X9SPU)"; break; /*0x0643*/
		 case 1636:     prodstr = "(X9DRH)"; break; /*0x0664*/
		 case 1643:     prodstr = "(X9SRL)"; break; /*0x066b*/
		 case 1797:	prodstr = "(X9DR7)";  break; /*0x0705*/
		 case 2097:	prodstr = "(X10DRL)";  /*0x0831*/
                    do_powerstate = 0;   break; 
		 case 2137:	prodstr = "(X10DRH)"; break; /*0x0859*/
		 case 2203:	prodstr = "(X11SSW-F)"; break; /*0x089b*/
		 case 2327:	prodstr = "(X11DPi)"; break; /*0x0917*/
		 case 4520:	prodstr = "(H8DGU)";  break;
		 case 43025:	prodstr = "(H8DGU-F)"; break;
		 case 43707:	prodstr = "(X8DTH)"; break;
		 case 48145:	prodstr = "(H8DG6)"; break;
                 default:       prodstr = ""; break;
             }
             if (!fipmilan) lan_ch_restrict = 1; /*fw bug, gets 0xd4 locally*/
             break;
         case VENDOR_QUANTA:    /*=7244.*/
             switch(prod) {     /* show product names for some */
                 case 21401:    prodstr = "(S99Q)"; break;
                 default:       prodstr = ""; break;
             }
             break;
         case VENDOR_LENOVO: 
         case VENDOR_LENOVO2: 
	     	if (prod == 0x143) prodstr = "(x3650 M4)"; 
	     	else prodstr = "";
	     	do_powerstate = 0;
         	break;
         case VENDOR_IBM:    /*=0x0002*/
             switch(prod) {     /* show product names for some */
             case 0x000e:   prodstr = "(x3755)"; break;
             case 0x0011:   prodstr = "(x3650)"; break;
             case 0x00dc:   prodstr = "(x3650 M2)"; break; /*M2,M3*/
             case 0x00fa:   prodstr = "(x3850 X5)"; break;
             case 0x8848:   prodstr = "(eServer 360S)"; break;
                 default:       prodstr = ""; break;
             }
             break;
         default:
             prodstr = "";
             break;
	} /*end switch(prod)*/
	if (prodstr[0] == '\0') prodstr = getdmiprod();
	if (kcs_loops != 0) set_max_kcs_loops(kcs_loops); 
      } /*end-else BMC*/

      printf("%s manufacturer  %c %06x (%s)%c product %c %04x %s\n",
		tag, bdelim,mfg,mfgstr,bcomma,bdelim,prod,prodstr);
      {  /* BMC version */
         printf("%s version       %c %x.%02x%s%c IPMI v%d.%d\n", tag,bdelim,
		devrec[2],devrec[3],extraver,bcomma,ipmi_maj,ipmi_min);
      }
      /* could show product rev, if available (sdevrec > 14) */
      return;
}

int GetPowerOnHours(unsigned int *val)
{
   uchar resp[MAX_BUFFER_SIZE];
   int sresp = MAX_BUFFER_SIZE;
   uchar cc;
   int rc = -1;
   int i;
   unsigned int hrs;

   *val = 0;
   if (fmBMC) return(rc);
   sresp = MAX_BUFFER_SIZE;
   memset(resp,0,6);  /* default response size is 5 */
   rc = ipmi_cmd_mc(GET_POWERON_HOURS, NULL, 0, resp, &sresp, &cc, fdebug);
   if (rc == 0 && cc == 0) {
      /* show the hours (32-bits) */
      hrs = resp[1] | (resp[2] << 8) | (resp[3] << 16) | (resp[4] << 24);
      /* 60=normal, more is OOB, so avoid div-by-zero*/ 
      if ((resp[0] <= 0) || (resp[0] >= 60)) i = 1; 
      else {
                i = 60 / resp[0];
                hrs = hrs / i;
      }
      *val = hrs;
   }
   return(rc);
}

char *decode_selftest(int stat)
{
   char *s;
   uchar b;
   if (stat == 0x0055) s = "(OK)";
   else {
      s = "(Error)";
      if ((stat & 0x00ff) == 0x0057) {
         b = ((stat & 0xff00) >> 8);
         if (b & 0x80) s = "(No SEL Access)"; 
         if (b & 0x40) s = "(No SDR Access)"; 
         if (b & 0x20) s = "(No FRU Access)"; 
         if (b & 0x10) s = "(IPMB Error)"; 
         if (b & 0x08) s = "(SDR Empty)"; 
         if (b & 0x02) s = "(BootCode Corrupt)"; 
         if (b & 0x01) s = "(OpCode Corrupt)"; 
      }
   }
   return(s);
}

#ifdef METACOMMAND
int i_health(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
#endif
{
   int ret = 0;
   int c;
   uchar selfbuf[16];
   uchar devrec[30];
   char biosver[80];
   uchar cc;
   int selfstatus;
   uchar pwr_state;
   char selfstr[36];
   char *s;
   char *s1;
   int i, sresp;
   uint n;
   int rlen, len;
   uchar idata[4];
   uchar rdata[16];

   printf("%s ver %s\n", progname,progver);

   while ( (c = getopt( argc, argv,"cfghiln:o:p:q:sT:V:J:YEF:P:N:R:U:Z:x?")) != EOF ) 
      switch(c) {
          case 'c': fcanonical = 1;
		    bdelim = BDELIM;  break;  /* canonical output */
          case 'f': do_frusdr = 1;  break;  /* check the FRUSDR too */
          case 'g': do_guid = 1;    break;  /* get the System GUID also */
          case 'h': do_hsc = 1;     break;  /* check the HSC too */
          case 'i': do_systeminfo = 1;  break;  /* get system info too */
          case 'l': do_lanstats = 1;  break;  /* get the LAN stats too */
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
          case 'n': set_name = 1; 		/* set the system name*/
		    pname = optarg;
		    break; 
          case 'o': set_os = 1; 		/* set the Operating System*/
		    pos = optarg;
		    break; 
          case 'q': set_os2 = 1; 		/* set the Operating System*/
		    pos2 = optarg;
		    break; 
          case 'p': set_restore = 1; 		/* set the restore policy */
		    restore_policy = atob(optarg);
		    if (restore_policy > 2) restore_policy = 1;
		    break; 
          case 's': do_session = 1;  break;  /* get session info too */
          case 'x': fdebug = 1;     break;  /* debug messages */
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
                printf("Usage: %s [-cfghilnopsx -N node -U user -P/-R pswd -EFTVY]\n", progname);
                printf(" where -x   show eXtra debug messages\n");
                printf("       -c   canonical output\n");
                printf("       -f   get the FRUSDR version also\n");
                printf("       -g   get the System GUID also\n");
                printf("       -h   check the HotSwap Controller also\n");
                printf("       -i   get System Info also: Name and OS\n");
                printf("       -l   get the IPMI LAN statistics also\n");
		printf("       -n   set System Name to this string \n");
		printf("       -o   set Operating System to this string\n");
		printf("       -p1  set restore policy: 0=off, 1=last, 2=on\n");
                printf("       -s   get the IPMI Session info also\n");
		print_lan_opt_usage(0);
		ret = ERR_USAGE;
		goto health_end;
      }

   fipmilan = is_remote();
   if (fipmilan && set_restore) 
             parse_lan_options('V',"4",0);  /*if set, request admin priv*/
	
   ret = ipmi_getdeviceid(devrec,16,fdebug);
   if (ret != 0) {
	goto health_end;
   } else {
       show_devid_all(BMC,devrec,16);
   }

   if (!fipmilan) {  /*get local BIOS version*/
	biosver[0] = 0;
	ret = get_BiosVersion(biosver);
	if (ret == 0) printf("BIOS Version      %c %s\n",bdelim,biosver);
   }
   if (do_me) {  /* ME version for Intel S5500 motherboards */
        rlen = sizeof(rdata);
        ret = ipmi_cmdraw((GET_DEVICE_ID & 0xff), NETFN_APP,ME_SA,ME_BUS,0x00,
			  idata,0,rdata,&rlen,&cc,fdebug);
        if (ret == 0 && cc != 0) ret = cc;
	if (ret == 0) {
		uchar m,n;
		m = (rdata[3] & 0xf0) >> 4;
		n = (rdata[3] & 0x0f);
		printf("ME Firmware Ver   %c %02x.%02x.%02x.%02x%02x\n",bdelim,
			     rdata[2],m,n,rdata[12],rdata[13]);
		/* If rdata[2] has 0x80 bit on, ME is in update/recovery mode.
		   That can be cleared by removing input power. */
	} else 
	   if (fdebug) printf("GetDeviceID(ME) error ret = %d\n",ret);
   }

   if (do_hsc) {
      if (fmBMC) printf("No HSC present\n");
      else {
	 /* Get HSC status */
	 if (is_romley(vend_id, prod_id)) {
	    uchar maj, min;
	    ret = get_hsbp_version_intel(&maj, &min);
	    if (fdebug) printf("get_hsbp_version_intel ret = %d\n",ret);
	    if (ret == 0) 
               printf("HSC version       %c %d.%02d\n", bdelim,maj,min);
	 } else {
	    ret = get_hsc_devid(&devrec[0],sizeof(devrec));
	    if (fdebug) printf("get_hsc_devid ret = %d\n",ret);
	    if (ret == 0)   /* only if HSC is detected */
               show_devid_all(HSC,devrec,14);
	 }
      }
   }

   i = get_driver_type();
   printf("IPMI driver type  %c %d        (%s)\n",bdelim,i,show_driver_type(i));

   if (do_powerstate) 
   {  /* Some BMCs dont support get_power_state*/
     ret = get_power_state(selfbuf,4);
     if (ret != 0) {
        printf("ipmi_getpowerstate error, ret = %d\n",ret);
        pwr_state = 0;
     } else {
        pwr_state = selfbuf[0] & 0x7f;
        printf("Power State       %c %02x       (%s)\n",
        	bdelim,pwr_state,pwr_string(pwr_state));
     }
   }

   ret = get_selftest_status(&selfbuf[0],sizeof(selfbuf));
   if (ret != 0) {
	printf("get_selftest_status error, ret = %x\n",ret);
	goto health_end;
   } else {
        selfstatus = selfbuf[0] + (selfbuf[1] << 8);
        s = decode_selftest(selfstatus);
        if (fmBMC) {
           sprintf(selfstr,"%s",s);
	} else {
	   ret = get_last_selftest(&selfbuf[0],sizeof(selfbuf));
	   if (fdebug) printf("get_last_selftest ret = %x, %02x%02x\n",
				ret, selfbuf[1],selfbuf[0]);
	   if (ret == 0 && (selfbuf[0] != 0xFF)) {
	      sprintf(selfstr,"%s, last = %02x%02x",s,selfbuf[1],selfbuf[0]);
	   } else sprintf(selfstr,"%s",s);
	   ret = 0;  /*ignore any errors with get_last_selftest*/
	}
	printf("Selftest status   %c %04x     %s\n",bdelim,selfstatus,selfstr);
   }

   ret = oem_get_health(&selfstr[0],sizeof(selfstr));
   if (ret == 0) {
	printf("%s\n",selfstr);
   }

   rlen = 4;
   ret = get_chassis_status(selfbuf,&rlen);
   if (ret != 0) {
	printf("Cannot do get_chassis_status, ret = %d\n",ret);
	goto health_end;
   } else {
        show_chs_status(selfbuf,rlen);
   }

   if (vend_id == VENDOR_INTEL) {
      int pwr_delay = 0;
      ret = get_power_restore_delay_intel(&pwr_delay);
      if (fdebug) printf("get_power_restore_delay_intel ret = %d\n",ret);
      if (ret == 0) {
         printf("PowerRestoreDelay %c %d seconds\n",bdelim,pwr_delay);
      } else ret = 0;
   }

   if (do_guid) {
      sresp = sizeof(devrec);
      ret = ipmi_cmd(GET_SYSTEM_GUID,NULL,0,devrec,&sresp,&cc,fdebug);
      if (ret != 0) {
	 if (!is_remote()) {  /* get UUID from SMBIOS */
	    cc = 0;  sresp = 16;
            ret = get_SystemGuid(devrec);
         } else {
	    cc = 0;
            sresp = sizeof(devrec);
            ret = get_device_guid(devrec,&sresp);
         }
      }
      if (fdebug) printf("system_guid: ret = %d, cc = %x\n",ret,cc);
      if (ret == 0 && cc == 0) {
         printf("System GUID       %c ",bdelim);
         for (i=0; i<16; i++) {
            if ((i == 4) || (i == 6) || (i == 8) || (i == 10)) s = "-";
            else s = "";
            printf("%s%02x",s,devrec[i]);
         }
         printf("\n");
      }
   }

   if (bChan != 7)  /* do not get first lan chan if set above */
      ret = get_lan_channel(1,&bChan);

   if (fmBMC == 0) {
      ret = GetPowerOnHours(&n);
      if (ret == 0) 
	 printf("Power On Hours    %c %d hours (%d days)\n",bdelim,n,(n/24));
      if (ret == 0xC1) ret = 0; /* not supporting poweron hours is ok. */

      printf("BMC LAN Channels  %c ",bdelim);
      for (i = 1; i<= 16; ) {
         c = get_lan_channel(i,&cc);
	 if (c != 0) break;
	 printf("%d ",cc);
	 i = cc+1;
      }
      printf("\n");

      if (lan_ch_restrict) ;   /*skip if vendor fw bug*/
      else {
         ret = get_chan_auth(bChan,&devrec[0],sizeof(devrec));
         if (ret == 0) 
            show_chan_auth("Channel Auth Cap",devrec,8);
         else 
            printf("get_chan_auth error: ret = %x\n",ret);
      }
   }

   if (do_systeminfo) {
      char infostr[64];
      len = sizeof(infostr);
      ret = get_system_info(1,infostr,&len);  /*Firmware Version*/
      len = sizeof(infostr);
      ret = get_system_info(2,infostr,&len);  
      if (ret == 0) {
	 printf("System Name           %c %s\n",bdelim,infostr);
         len = sizeof(infostr);
         ret = get_system_info(3,infostr,&len);
         if (ret == 0) printf("Pri Operating System  %c %s\n",bdelim,infostr);
         len = sizeof(infostr);
         ret = get_system_info(4,infostr,&len);
         if (ret == 0) printf("Sec Operating System  %c %s\n",bdelim,infostr);
      } else {  
         if (ret == 0xC1) /*only supported on later IPMI 2.0 firmware */
	    printf("GetSystemInfo not supported on this platform\n");
      }
   }

   if (do_frusdr) {
      ret = get_frusdr_version((char *)&devrec[0],sizeof(devrec));
      if (ret == 0) printf("FRU/SDR Version   %c %s\n",bdelim,devrec);
      else          printf("FRU/SDR Version   %c error %d\n",bdelim,ret);
   }

   if (do_lanstats) {
	ret = get_lan_stats(bChan);
   }

   if (do_session) {
	i = get_session_info_all();
	if (i != 0) printf("get_session_info error %d, %s\n",i,decode_rv(i));
   }

#ifdef PING_OK
   {
     char *node;
     /* Currently some problems with this:
      * works first time, but locks up BMC LAN on subsequent attempts.
      */
     node = get_nodename();
     ret = ping_node(node);
     printf("ping_node(%s): ret = %d\n",node,ret);
   }
#endif

   if (set_name) { 
	len = (int)strlen(pname);
        ret = set_system_info(2,pname,len);
        printf("Set System Name to '%s', ret = %d\n",pname,ret);
        if (ret == 0xC1) /*only supported on later IPMI 2.0 firmware */
	    printf("SetSystemInfo not supported on this platform\n");
   }

   if (set_os) { 
	len = (int)strlen(pos);
        ret = set_system_info(3,pos,len);
        printf("Set Pri Operating System to '%s', ret = %d\n",pos,ret);
        if (ret == 0xC1) /*only supported on later IPMI 2.0 firmware */
	    printf("SetSystemInfo not supported on this platform\n");
   }

   if (set_os2) { 
	len = (int)strlen(pos2);
        ret = set_system_info(4,pos2,len);
        printf("Set Sec Operating System to '%s', ret = %d\n",pos2,ret);
        if (ret == 0xC1) /*only supported on later IPMI 2.0 firmware */
	    printf("SetSystemInfo not supported on this platform\n");
   }

   if (set_restore) {
	idata[0] = restore_policy; /* 1=last_state, 2=turn_on, 0=stay_off*/
        rlen = sizeof(rdata);
        ret = ipmi_cmdraw(0x06 , NETFN_CHAS,  g_sa, g_bus, g_lun,
			  idata,1,rdata,&rlen,&cc,fdebug);
        if (ret == 0 && cc != 0) ret = cc;
        printf("set_restore_policy(%x): ret = %d\n",restore_policy,ret);
   }

health_end:
   ipmi_close_();
   // show_outcome(progname,ret);  
   return (ret);
}  /* end main()*/

/* end bmchealth.c */
