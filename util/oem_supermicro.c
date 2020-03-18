/*
 * oem_supermicro.c
 * Handle SuperMicro OEM command functions 
 *
 * Change history:
 *  12/06/2010 ARCress - included in source tree
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
#include "isensor.h"
#include "oem_supermicro.h"

#ifdef MOVED  /*moved to oem_supermicro.h*/
#define SUPER_NETFN_OEM           0x30
#define SUPER_CMD_BMCSTATUS       0x70
#define SUPER_CMD_RESET_INTRUSION 0x03
#define SUPER_NETFN_OEMFW     0x3C  
#define SUPER_CMD_OEMFWINFO   0x20
#endif 

void set_loglevel(int level);  /*prototype */
extern char fsm_debug;  /*mem_if.c*/

#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil smcoem";
#else
static char * progver   = "3.08";
static char * progname  = "ismcoem";
#endif
static int verbose = 0;
static char fdebug = 0;
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;
static int vend_id = 0;
static int prod_id = 0;

int oem_supermicro_get_bmc_status(uchar *sts)
{
   int rv;
   int rlen, ilen;
   uchar rdata[16];
   uchar idata[16];
   uchar cc;

   if (sts == NULL) return(LAN_ERR_INVPARAM);
   if ((vend_id == VENDOR_SUPERMICROX) || 
       (vend_id == VENDOR_SUPERMICRO)) { 
      /* subfunc 0xF0 is invalid for newer SMC systems */
      idata[0] = 0x02;  /* action: get status */
      ilen = 1;
   } else {
      idata[0] = 0xF0;  /* subfunction */
      idata[1] = 0x02;  /* action: get status */
      // idata[2] = 0;
      ilen = 2;
   }
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(SUPER_CMD_BMCSTATUS, SUPER_NETFN_OEM, 
		    BMC_SA, PUBLIC_BUS, BMC_LUN,
		    idata, ilen, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (rv == 0) { *sts = rdata[0]; }
   return(rv);
}

/* 
 * oem_supermicro_get_bmc_services_status
 *
 * Request
 * 0x30 - OEM network function
 * 0x70 - OEM cmd  
 * 0xF0 - subcommand
 * 0x?? - action 00=disable, 01=enable, 02=status
 *
 * Response
 * 0x?? - if action=status: 00=disabled, 01=enabled
 */
int oem_supermicro_set_bmc_status(uchar sts)
{
   int rv;
   int rlen, ilen;
   uchar rdata[16];
   uchar idata[16];
   uchar cc;

   if (sts > 1) sts = 1;  /* actions: 0=disable, 1=enable, 2=status*/ 
   if ((vend_id == VENDOR_SUPERMICROX) || 
       (vend_id == VENDOR_SUPERMICRO)) { 
      idata[0] = sts;
      ilen = 1;
   } else {
      idata[0] = 0xF0;  /* subfunction */
      idata[1] = sts;
      ilen = 2;
   }
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(SUPER_CMD_BMCSTATUS, SUPER_NETFN_OEM, 
		    BMC_SA, PUBLIC_BUS, BMC_LUN,
		    idata, ilen, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   return(rv);
}

/* 
   oem_supermicro_psstatus1(uchar psnum, uchar *val)
*/
int oem_supermicro_psstatus1(uchar psnum, uchar *val)
{
   int rv;
   int rlen;
   uchar idata[16];
   uchar rdata[16];
   uchar cc;

   idata[0] = 0x07;  /*busid*/
   if (psnum <= 1) idata[1] = 0x70; /* PS 1 */
   else if (psnum == 2) idata[1] = 0x72; /* PS 2 */
   else /*if (psnum == 3)*/ idata[1] = 0x74; /* PS 3 */
   idata[2] = 0x01;  /* return one byte of PS status data */
   idata[3] = 0x0C;  
   rlen = sizeof(rdata);
   rv = ipmi_cmd(MASTER_WRITE_READ, idata, 4, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (rv == 0) { *val = rdata[0]; }
   return(rv);
}

/* 
   oem_supermicro_psstatus2(uchar psnum, uchar *val)
   Get PMBus Power Supply Status, for X10 motherboards
*/
int oem_supermicro_psstatus2(uchar psnum, uchar *val)
{
   int rv;
   int rlen;
   uchar idata[16];
   uchar rdata[16];
   uchar cc;

   idata[0] = 0x07;  /*busid*/
   if (psnum <= 1) idata[1] = 0x78; /* PS 1 */
   else if (psnum == 2) idata[1] = 0x7A; /* PS 2 */
   else /*if (psnum == 3)*/ idata[1] = 0x7C; /* PS 3 */
   idata[2] = 0x01;  /* return one byte of PS status data */
   idata[3] = 0x78;  
   rlen = sizeof(rdata);
   rv = ipmi_cmd(MASTER_WRITE_READ, idata, 4, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (rv == 0) { *val = rdata[0]; }
   return(rv);
}

/* 
   oem_supermicro_psstatus3(uchar psnum, uchar *val)
   Get PMBus Power Supply Status, for X10 motherboards

   raw 0x06 0x52 0x07 0xb0 0x01 0x0c for power supply 1
   raw 0x06 0x52 0x07 0xb2 0x01 0x0c for power supply 2
*/
int oem_supermicro_psstatus3(uchar psnum, uchar *val)
{
   int rv;
   int rlen;
   uchar idata[16];
   uchar rdata[16];
   uchar cc;

   idata[0] = 0x07;  /*busid*/
   if (psnum <= 1) idata[1] = 0xB0; /* PS 1 */
   else if (psnum == 2) idata[1] = 0xB2; /* PS 2 */
   else /*if (psnum == 3)*/ idata[1] = 0xB4; /* PS 3 */
   idata[2] = 0x01;  /* return one byte of PS status data */
   idata[3] = 0x0C;  
   rlen = sizeof(rdata);
   rv = ipmi_cmd(MASTER_WRITE_READ, idata, 4, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (rv == 0) { *val = rdata[0]; }
   return(rv);
}

int oem_supermicro_get_lan_port(uchar *val)
{
   int rv;
   int rlen, ilen;
   uchar rdata[16];
   uchar idata[16];
   uchar cc;

   idata[0] = 0x0c;  /* subfunction */
   idata[1] = 0x00;  /* get */
   ilen = 2;
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(SUPER_CMD_BMCSTATUS, SUPER_NETFN_OEM, 
		    BMC_SA, PUBLIC_BUS, BMC_LUN,
		    idata, ilen, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (rv == 0) { *val = rdata[0]; }
   return(rv);
}

int oem_supermicro_set_lan_port(uchar val)
{
   int rv;
   int rlen, ilen;
   uchar rdata[16];
   uchar idata[16];
   uchar cc;

   idata[0] = 0x0c;  /* subfunction */
   idata[1] = 0x01;  /* set */
   idata[2] = val;
   ilen = 3;
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(SUPER_CMD_BMCSTATUS, SUPER_NETFN_OEM, 
		    BMC_SA, PUBLIC_BUS, BMC_LUN,
		    idata, ilen, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   return(rv);
}

char *oem_supermicro_lan_port_string(uchar val)
{
	char *p;
	switch(val) {
	case 0: p = "Dedicated"; break;
	case 1: p = "Onboard_LAN1"; break;
	case 2: p = "Failover"; break;
	default: p = "unknown"; break;
	}
	return(p);
}

static void oem_supermicro_show_lan_port(uchar val)
{
	printf("Current LAN interface is %s\n",
		oem_supermicro_lan_port_string(val));
}

int oem_supermicro_get_health(char *pstr, int sz) 
{
   int rv;
   uchar bsts;
   char *str;

   rv = oem_supermicro_get_bmc_status(&bsts);
   if (rv == 0) {
	if (bsts == 0x01) str = "BMC status = enabled";
	else str = "BMC status = disabled";
	strncpy(pstr, str, sz);
   }
   return(rv);
}

/* 
 * oem_supermicro_get_firmware_info
 *
 * From post by ipmitool developer.
 * http://sourceforge.net/mailarchive/message.php?msg_name=49ABCCC3.4040004%40cern.ch
 *
 * Request
 * 0x3C - OEM network function
 * 0x20 - OEM cmd  (SUPER_CMD_OEMFWINFO)
 *
 * Response data:
 * 4 bytes - firmware major version (LSB first)
 * 4 bytes - firmware minor version (LSB first)
 * 4 bytes - firmware sub version (LSB first)
 * 4 bytes - firmware build number (LSB first)
 * 1 byte - hardware ID
 * ? bytes - firmware tag, null terminated string
 */
int oem_supermicro_get_firmware_info(uchar *info)
{
   int rv;
   int rlen;
   uchar rdata[32];
   uchar idata[16];
   uchar cc;

   if (info == NULL) return(LAN_ERR_INVPARAM);
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(SUPER_CMD_OEMFWINFO, SUPER_NETFN_OEM, 
		    BMC_SA, PUBLIC_BUS, BMC_LUN,
		    idata, 0, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (rv == 0) { memcpy(info,rdata,rlen); }
   return(rv);
}

int oem_supermicro_get_firmware_str(char *pstr, int sz)
{
   int rv;
   uchar info[32];
   uint32 fwmaj;
   uint32 fwmin;
   uint32 fwsub;
   uint32 fwbld;
   uchar hwid;
   rv = oem_supermicro_get_firmware_info(info);
   if (rv == 0) {
        fwmaj = info[0] + (info[1] << 8) + (info[2] << 16) + (info[3] << 24);
        fwmin = info[4] + (info[5] << 8) + (info[6] << 16) + (info[7] << 24);
        fwsub = info[8] + (info[9] << 8) + (info[10] << 16) + (info[11] << 24);
        fwbld = info[12] +(info[13] << 8) + (info[14] << 16) + (info[15] << 24);
	hwid = info[16];
        /*info[17] = fw tag string */
        snprintf(pstr,sz,"Firmware %d.%d.%d.%d HW %d %s\n",fwmaj,fwmin,fwsub,
			fwbld,hwid,&info[17]);
   }
   return(rv);
}


int oem_supermicro_reset_intrusion(void)
{
   int rv;
   int rlen;
   uchar rdata[32];
   uchar idata[16];
   uchar cc;
   // if (state == NULL) return(LAN_ERR_INVPARAM);
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(SUPER_CMD_RESET_INTRUSION, SUPER_NETFN_OEM, 
		    BMC_SA, PUBLIC_BUS, BMC_LUN,
		    idata, 0, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (rv == 0) { /* check rdata for more info */ }
   return(rv);
}

/* decode_threshold_supermicro() assumes Fans, not Temp
 *    Temp thresholds are different order. */
int decode_threshold_supermicro(uchar rval, uchar *thresh)
{
	int idx = 0;
	uchar bits;

	bits = thresh[0];
	if (bits & 0x20) {  /*hi-unrec*/
	   if (rval >= thresh[6]) { idx = 6; return(idx); }
	}
	if (bits & 0x10) {  /*hi-crit*/
	   if (rval >= thresh[5]) { idx = 5; return(idx); }
	}
	if (bits & 0x08) {  /*hi-noncr*/
	   if (rval >= thresh[4]) { idx = 4; return(idx); }
	}
	if (bits & 0x01) {  /*lo-noncr*/
	   if (rval <= thresh[1]) { idx = 1; }
	}
	if (bits & 0x02) {  /*lo-crit*/
	   if (rval <= thresh[2]) { idx = 2; }
	}
	if (bits & 0x04) {  /*lo-unrec*/
	   if (rval <= thresh[3]) { idx = 3; }
	}
	return(idx);
}

/*
 * decode_sensor_supermicro
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
int decode_sensor_supermicro(uchar *sdr,uchar *reading,char *pstring, int slen, 
		int fsimple, char fdbg)
{
   int i, rv = -1;
   uchar stype, etype, snum;
   uchar bval;
   char *pstr = NULL;
   uchar thresh[7];
   SDR01REC *sdr01;
   char *typestr = NULL;
   double val;

   if (sdr == NULL || reading == NULL) return(rv);
   if (pstring == NULL || slen == 0) return(rv);
   fdebug = fdbg;
   bval = (reading[2] & 0x3f);
   snum = sdr[7];   /*sdr01->sens_num*/
   stype = sdr[12]; /*sensor_type*/
   etype = sdr[13]; /*sdr01->ev_type*/
   get_mfgid(&vend_id,&prod_id); /*saved from ipmi_getdeviceid */
   /* sdr[3] rec_type is 0x01 for Full, 0x02 for Compact */
   if ((sdr[3] == 0x01) && (etype == 0x01)) { /* full threshold sensor */
	  /*if Temp sensor, bits==0 would show BelowCrit, so handle normally */
	  if ((stype == 0x01) && (bval == 0)) return(-1); 
	  /*Cannot rely upon the sensor reading[2], so get thresholds and compare*/
	  rv = GetSensorThresholds(snum,&thresh[0]);
	  if (rv != 0) return(rv);
      i = decode_threshold_supermicro(reading[0],thresh);
      if (fdebug) 
	    printf("decode_sensor_supermicro: snum=%x stype=%x rdg=%x:%x thresh=%x:%x:%x:%x:%x:%x:%x i=%d rv=%d\n",
			snum,stype,reading[0],reading[2], thresh[0], thresh[1], thresh[2], thresh[3], 
			thresh[4], thresh[5], thresh[6], i,rv);
      switch(i) {
         case 0: pstr = "OK";  break;
         case 1: pstr = "Warn-lo"; break;
         case 2: pstr = "Crit-lo"; break;
         case 3: pstr = "BelowCrit"; break;
         case 4: pstr = "Warn-hi"; break;
         case 5: pstr = "Crit-hi"; break;
         case 6: pstr = "AboveCrit"; break;
	     default: pstr = "OK*";  break;
      }
	  sdr01 = (SDR01REC *)sdr;
	  val = RawToFloat(reading[0],sdr);
      typestr = get_unit_type(sdr01->sens_units,sdr01->sens_base,sdr01->sens_mod, 0);
	  if (fsimple) 
	       snprintf(pstring, slen, "%s | %.2f %s",pstr,val,typestr);
	  else snprintf(pstring, slen, "%s %.2f %s",pstr,val,typestr);
	  return(rv);
   }
   switch(stype) {
        case 0xC0:      /* CPU Temp Sensor, EvTyp=0x70 (Full) */
	   //if (dbg) printf("supermicro %x sensor reading %x\n",stype,reading);
           rv = 0;
	   switch(bval) {
           case 0x0000: pstr = "Low";    break;
           case 0x0001: pstr = "Medium"; break;
           case 0x0002: pstr = "High";   break;
           case 0x0004: pstr = "Overheat";   break;
           case 0x0007: pstr = "Not Installed";   break;
	   default:  rv = -1;  break;
	   }
           break;
        case 0x08:  /* Power Supply Status (Full/Discrete) Table 42-3 */
          rv = 0;
          switch(bval) {
           case 0x00: pstr = "Absent"; break;     /*bit 0*/
           case 0x01: pstr = "Present"; break;    /*bit 0*/
           case 0x02: pstr = "Failure"; break;    /*bit 1*/
           case 0x04: pstr = "Predict Fail"; break;  /*bit 2*/
           case 0x08: pstr = "Input Lost"; break;    /*bit 3*/
           default:  rv = -1;  break;
          }
           break;
        case 0x0D:  /* HDD Status (Full/Discrete) Table 42-3 */
          rv = 0;
          switch(bval) {
             case 0x00: pstr = "Absent"; break;     /*bit 0*/
             case 0x01: pstr = "Present"; break;    /*bit 0*/
             case 0x02: pstr = "Failure"; break;    /*bit 1*/
             case 0x04: pstr = "Predict Fail"; break;  /*bit 2*/
             default:  rv = -1;  break;
          }
           break;
        case 0x29:  /* VBAT sensor */
           if (prod_id == 0x0917) { /*X11DRi has inert VBAT full sensor*/
			 if (bval == 0) {
                pstr = "NotAvailable";
			    rv = 0;
			 }
           }
           break;
        default:
           break;
   }
   if (rv == 0) strncpy(pstring, pstr, slen);
   return(rv);
}

extern int get_MemDesc(int array, int dimm, char *desc, int *psz); /*mem_if.c*/
#define NPAIRS  26
char rgpair[NPAIRS] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
int decode_mem_supermicro(int prod, uchar b2, uchar b3, char *desc, int *psz)
{
   int cpu, pair, dimm, n;
   int rv = 0;
   int ver = 0;
   uchar bdata;
   if ((desc == NULL) || (psz == NULL)) return -1;
   if (b2 == 0xff) { ver = 1; bdata = b3; }  /*ff is reserved*/
   else { ver = 2; bdata = b2; } /* normal case */
#ifdef SMC_OLD
   /* normal method for other vendors */
   cpu = (bdata & 0xc0) >> 6;
   dimm = bdata & 0x3f;
   /* SuperMicro advertised method (wrong) */
   /* bdata = 0x10 (16.) means CPU 1, DIMM 6 */
   cpu = bdata / 10;
   dimm  = bdata % 10;
#endif
   /* ver 0 previous SuperMicro firmware returned all zeros here.
    *        Memory #00 Correctable ECC, <pstr> 6f [00 00 00]
    * ver 1 returns data3 with some info (X9)
    * ver 2 returns data2 with some info (X9,X10)
    * There have been bugs in these SuperMicro events, but this 
    * has been derived from test data by comparing BIOS data. */
   if (ver == 0) {
      cpu = (bdata & 0xc0) >> 6;
      dimm =bdata & 0x3f;
	  pair = 0;
      n = sprintf(desc,"CPU%d/DIMM%d",cpu,dimm);
   } else if (ver == 1) {
	  cpu = 1;
      if (bdata > 0x80) cpu = 2;
      pair = (bdata & 0x70) >> 4;
      if (pair > NPAIRS) pair = NPAIRS - 1;
	  dimm = (bdata & 0x07) + 1;  /*convert to 1-based*/
      n = sprintf(desc,"P%d_DIMM%c%d",cpu,rgpair[pair],dimm);
   } else {
      /* ver 2 method: 2A 80 = P1_DIMMB1 */
	  /* SuperMicro says:
	   *  pair: %c (data2 >> 4) + 0x40 + (data3 & 0x3) * 3, (='B')
	   *  dimm: %c (data2 & 0xf) + 0x27,    
	   *  cpu:  %x (data3 & 0x03) + 1);
	   */
      cpu = (b3 & 0x0F) + 1; /*0x80=CPU1, 0x81=CPU2*/
      pair = ((bdata & 0xF0) >> 4) - 1; /*0x10=pairA, 0x20=pairB*/
      if (pair < 0) pair = 0;
      if (pair > NPAIRS) pair = NPAIRS - 1;
      dimm = (bdata & 0x0F) - 9; /*0x0A=dimmX1, 0x0B=dimmX2*/
      if (dimm < 0) 
         n = sprintf(desc,DIMM_UNKNOWN);  /* invalid */
      else 
         n = sprintf(desc,"P%d_DIMM%c%d",cpu,rgpair[pair],dimm);
   }
   /* Use DMI if we get confirmation about cpu/dimm indices. */
   if (! is_remote()) {
      fsm_debug = fdebug;
      rv = get_MemDesc(cpu,dimm,desc,psz);
      /* if (rv != 0) desc has "DIMM[%d}" */ 
   } 
   if ((bdata == 0xFF) || (rv != 0)) n = sprintf(desc,DIMM_UNKNOWN);  
   if (fdebug) 
	 printf("decode_mem_supermicro: v%d bdata=%02x(%d) cpu=%d dimm=%d pair=%d\n",ver,bdata,bdata,cpu,dimm,pair);

   *psz = n;
   rv = 0;
   return(rv);
} /*end decode_mem_supermicro*/

/*
 * decode_sel_supermicro
 * inputs:
 *    evt    = the 16-byte IPMI SEL event
 *    outbuf = points to the output string buffer
 *    outsz  = size of the output buffer
 * outputs:
 *    rv     = 0 if this event was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    outbuf = will contain the interpreted event text string (if rv==0)
 * See also decode_mem_supermicro above 
 */
int decode_sel_supermicro(uchar *evt, char *outbuf, int outsz, char fdesc,
                        char fdbg)
{
   int rv = -1;
   ushort id;
   uchar rectype;
   ulong timestamp;
   char mybuf[64]; 
   char mytype[64]; 
   char *type_str = "";
   char *pstr = NULL;
   int sevid, d1;
   ushort genid;
   uchar snum;
   uchar data1, data2, data3;
   char *psens = NULL;
         
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
     sprintf(mybuf,"%02x [%02x %02x %02x]", evt[12],evt[13],evt[14],evt[15]);
     switch(evt[10]) {  /*sensor type*/
     case 0xC0:      /* CPU Temp Sensor */
	   type_str = "OEM_CpuTemp";
	   d1 = (evt[13] & 0x0f);  /*offset/data1 l.o. nibble*/
	   switch(d1) {  
           case 0x02:   /* CPU Temp Sensor Overheat event offset */
		if (evt[12] & 0x80) {  /*EvTyp==0xF0 if deassert*/
		   pstr = "CpuTemp Overheat OK"; sevid = SEV_INFO; 
		} else {  /* EvTyp=0x70 assert */
		   pstr = "CpuTemp Overheat   "; sevid = SEV_MAJ; 
		}
		break;
	   default: pstr = "CpuTemp Event";
		sprintf(mytype,"CpuTemp_%02x", d1);
		type_str = mytype;
		sevid = SEV_MIN; 
		break;
	   }
	   rv = 0; 
	   break;
     case 0xC2:      /* CPLD Event */
	   type_str = "OEM_CPLD";
	   switch((evt[13] & 0x0f)) {  /* data1 usu 0xa0*/
	   case 0x00:
		if (evt[14] == 0x1c) 
		     { pstr = "CPLD CATERR Asserted"; sevid = SEV_CRIT; }
		else { pstr = "CPLD Event Asserted";  sevid = SEV_MIN;  }
		break;
	   default: pstr = "CPLD Event"; sevid = SEV_MIN; break;
	   }
	   rv = 0;
	   break;
     case 0xD0:      /* BMC Event */
	   type_str = "OEM_BMC";
	   pstr = "BMC unknown event";
	   sevid = SEV_CRIT; 
	   if (data1 == 0x80 && data3 == 0xFF) {
			switch(data2) {
			  case 0x00: pstr = "BMC unexpected reset"; break;
			  case 0x01: pstr = "BMC cold reset"; break;
			  case 0x02: pstr = "BMC warm reset"; break;
			}
	   }
	   rv = 0;
	   break;
     case 0xC5:      /* Observed Event: Storage, drive slot */
	   /* usually OEM(c5) #52 - 6f [01 00 00]  */
	   type_str = "OEM_C5";
	   pstr = "Storage/DriveSlot fault";
	   sevid = SEV_MAJ; 
	   rv = 0;
	   break;
     case 0xC8:      /* Observed Event: AC Power on event */
	   /* usually OEM(c8) #ff - 6f [a0 ff ff] */
	   type_str = "OEM_C8";
	   pstr = "AC Power On";
	   sevid = SEV_MAJ; 
	   rv = 0;
	   break;
     default: /*other sensor types*/
	   break;
     }
   }
   if (rv == 0) {
	 format_event(id,timestamp, sevid, genid, type_str,
			snum,psens,pstr,mybuf,outbuf,outsz);
   }
   return(rv);
}

/* factory_defaults: set SMC factory defaults 
 * mode: 0 = detect, default is c0 41 method
 *       2 = use c0 41 method, also get guid
 *       3 = use f0 40 method (usu X11)
 */
static int factory_defaults(int mode)
{
   int rv = -1;
   int rlen;
   uchar rdata[32];
   uchar idata[16];
   uchar cc;
  /* 
   From SMC IPMICFG -fde  factory defaults session:
   start_kcs_transaction - 18 01    (get_device_id)
   start_kcs_transaction - 18 25    (watchdog get)
   start_kcs_transaction - b0 00 00 (unknown, invalid)
   start_kcs_transaction - 18 01    (get_device_id)
   start_kcs_transaction - 18 37    (get_system_guid)
   start_kcs_transaction - 18 01    (get_device_id)
   start_kcs_transaction - c0 41    (reset_factory_defaults, -fde)
  */
   if ((prod_id > 2200) && (prod_id < 2300) && (mode == 0)) {
      /* Looks like X11 board, may use netfn 3c, cmd 40 ? */
      /* do not auto-set this if mode param is not 3 */
      // mode = 3; /* cmd=0x40 netfn=0x3c (netfn/lun=0xF0) */
      if (fdebug) printf("may also need ipmiutil cmd 00 20 f0 40\n");
   }

   /* b0 00 00: cmd=00 netfn=2C */
   rlen = sizeof(rdata);
   idata[0] = 0x00;  
   rv = ipmi_cmdraw(0x00, (0xb0 >> 2), BMC_SA, PUBLIC_BUS, BMC_LUN,
		    idata, 1, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (fdebug) printf("b0 00 returned %d\n",rv);
   /* if (rv != 0) return(rv); */

   if (mode == 2) {  /* (get_system_guid) */
     rlen = sizeof(rdata);
     rv = ipmi_cmdraw(0x37, (0x18 >> 2), BMC_SA, PUBLIC_BUS, BMC_LUN,
			    idata, 0, rdata, &rlen, &cc, fdebug);
     if ((rv == 0) && (cc != 0)) rv = cc;
     if (fdebug) printf("get_guid returned %d\n",rv);
     if (rv != 0) return(rv);
   }

   /* reset factory defaults */
   rlen = sizeof(rdata);
   if (mode == 3) { /* cmd=0x40 netfn=0x3c (netfn/lun=0xF0) */
     rv = ipmi_cmdraw(0x40, 0x3c, BMC_SA, PUBLIC_BUS, BMC_LUN,
		       idata, 0, rdata, &rlen, &cc, fdebug);
   } else { /* cmd=0x41 netfn=0x30 (netfn/lun=0xC0) if X10 or less */
     rv = ipmi_cmdraw(0x41, (0xc0 >> 2), BMC_SA, PUBLIC_BUS, BMC_LUN,
		       idata, 0, rdata, &rlen, &cc, fdebug);
   }
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (fdebug) printf("factory default reset returned %d\n",rv);
   if (rv != 0) return(rv);

   return(rv);
}

static void usage(void)
{
   printf("Usage: %s <command> [arg]\n",progname);
   printf("   intrusion                   = reset chassis intrusion\n");
   printf("   bmcstatus [enable| disable] = get/set BMC status\n");
   printf("   firmware                    = get extra firmware info\n");
   printf("   factory                     = reset to factory defaults\n");
   printf("   lanport  [dedicated| lan1| failover] = get/set IPMI LAN port\n");
   printf("   powersupply <num>           = get PMBus PowerSupply status\n");
   printf("These commands may not work on all SuperMicro systems\n");
}

static int ipmi_smcoem_main(int  argc, char **argv)
{
   int n,rv = 0;
   char msg[80];
   uchar val;

   if ((vend_id != VENDOR_SUPERMICRO) && (vend_id != VENDOR_SUPERMICROX)) {
      printf("Not SuperMicro firmware, ignoring smcoem\n");
      usage();
      return(ERR_USAGE);
   }
   if (strncmp(argv[0],"intrusion",9) == 0) {
      printf("Clearing Chassis Intrusion ...\n");
      rv = oem_supermicro_reset_intrusion();
   } else if (strncmp(argv[0],"bmcstatus",9) == 0) {
      printf("Getting BMC status ...\n");
      rv = oem_supermicro_get_health(msg, sizeof(msg));
      if (rv != 0) return(rv);
      printf("%s\n",msg);
      if (argv[1] != NULL) {
         if (strncmp(argv[1],"disable",7) == 0) {
	    val = 0;
         } else if (strncmp(argv[1],"enable",6) == 0) {
	    val = 1;
	 } else {
	    usage();
	    return(ERR_USAGE);
	 }
         printf("Setting BMC status to %s ...\n",argv[1]);
         rv = oem_supermicro_set_bmc_status(val);
         if (rv != 0) return(rv);
         rv = oem_supermicro_get_health(msg, sizeof(msg));
         if (rv == 0) printf("%s\n",msg);
      }
   } else if (strncmp(argv[0],"firmware",8) == 0) {
      printf("Getting SMC Firmare Information ...\n");
      rv = oem_supermicro_get_firmware_str(msg, sizeof(msg));
      if (rv == 0) printf("%s\n",msg);
   } else if (strncmp(argv[0],"factory",7) == 0) {
      rv = factory_defaults(0);
      if (rv == 0) printf("Reset firmware to factory defaults\n");
      else printf("Error %d resetting firmware to factory defaults\n",rv);
   } else if (strncmp(argv[0],"lanport",7) == 0) {
      rv = oem_supermicro_get_lan_port(&val);
      if (rv == 0) {
        oem_supermicro_show_lan_port(val);
        if (argv[1] != NULL) {
           if (strncmp(argv[1],"dedicated",9) == 0) {
	      val = 0;
           } else if (strncmp(argv[1],"lan1",4) == 0) {
	      val = 1;
           } else if (strncmp(argv[1],"failover",8) == 0) {
	      val = 2;
	   } else {
	      usage();
	      return(ERR_USAGE);
	   }
           printf("Setting LAN interface to %s ...\n",argv[1]);
	   rv = oem_supermicro_set_lan_port(val);
           if (rv != 0) return(rv);
           rv = oem_supermicro_get_lan_port(&val);
	   if (rv == 0) oem_supermicro_show_lan_port(val);
        }
      }
   } else if (strncmp(argv[0],"powersupply",11) == 0) {
      if (argv[1] == NULL) {
	      usage();
          rv = ERR_USAGE;
      } else {
          char DevRec[16];
		  int xver = 9;
	      n = atoi(argv[1]);  /* power supply number */
          rv = ipmi_getdeviceid( DevRec, sizeof(DevRec),fdebug);
          if (rv == 0) {
             int  vend_id, prod_id;
		     /* 1562 (0x061A) = X8SIU */
		     /* 1572 (0x0624) = X9SCM */
		     /* 1797 (0x0705) = X9DR7 */
		     /* 2137 (0x0859) = X10DRH */
             vend_id = DevRec[6] + (DevRec[7] << 8) + (DevRec[8] << 16);
             prod_id = DevRec[9] + (DevRec[10] << 8);
			 if (prod_id > 0x0800) xver = 10;
			 else if (prod_id > 1570) xver = 9;
			 else xver = 8;
	      }
          if (xver == 10)     rv = oem_supermicro_psstatus3(n, &val);
          else if (xver == 9) rv = oem_supermicro_psstatus2(n, &val);
          else /*xver==8*/    rv = oem_supermicro_psstatus1(n, &val);
		  if (rv == 0) {
             if (val == 0x00) strcpy(msg,"good");
             else if (val == 0x02) strcpy(msg,"ok");
			 else sprintf(msg,"bad 0x%02x",val);
             printf("X%d Power Supply %d status = %d (%s)\n",xver,n,val,msg);
		  } else {
             printf("X%d Power Supply %d error = %d\n",xver,n,rv);
		  }
      }
   } else {
      usage();
      rv = ERR_USAGE;
   }
   return(rv);
}

#ifdef METACOMMAND
int i_smcoem(int argc, char **argv)
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

   rv = ipmi_smcoem_main(argc, argv);

   ipmi_close_();
   return(rv);
}
/* end oem_supermicro.c */
