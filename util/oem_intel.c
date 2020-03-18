/*
 * oem_intel.c
 *
 * This module handles code specific to Intel platforms, 
 * including the Intel/Kontron Telco Alarms panel.  
 * 
 * Note that the Intel BMC TAM will set these alarms
 * based on firmware-detected thresholds and events.
 * 
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2005 Intel Corporation
 * Copyright (c) 2010 Kontron America, Inc.
 *
 * Compile flags for oem_intel.c:
 * NO_CMD would be defined if linking with ievents.c only (no ipmicmd code)
 * NO_EVENTS would be defined if linking with ialarms.c (no ievents code)
 *
 * 09/02/10 Andy Cress - separated from ialarms.c
 */
/*M*
Copyright (c) 2005 Intel Corporation
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#if defined(DOS)
#include <dos.h>
#endif
#include "ipmicmd.h"
#include "oem_intel.h"
 
#ifdef METACOMMAND
#include "ievents.h"
extern char fsm_debug;     /*mem_if.c*/
extern int  sens_verbose;  /*isensor.c*/
extern int get_sensdesc(uchar sa, int snum, char *sdesc, int *pstyp, int *pidx);
extern int get_MemDesc(int array, int dimm, char *desc, int *psz); /*mem_if.c*/
#else
static char fsm_debug = 0;   
static int  sens_verbose = 0; 
static int  get_MemDesc(int array, int dimm, char *desc, int *psz) { return -1;}
#if !defined(NO_CMD)
int get_sensdesc(uchar sa, int snum, char *sensdesc, int *pstyp, int *pidx)
{ return(-1); }
#endif
static char *get_sensor_type_desc(uchar stype) 
{ 
    static char tstr[12];
    sprintf(tstr,"%02x",stype);
    return(tstr); 
}
#endif
extern char  fdebug;   /*ipmicmd.c*/ 

/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil inteloem";
#else
static char * progver   = "3.08";
static char * progname  = "iinteloem";
#endif
static char  fRelayBits = 0;
static uchar g_bus = PUBLIC_BUS;
static uchar g_sa  = BMC_SA;
static uchar g_lun = BMC_LUN;
//static uchar g_addrtype = ADDR_SMI;

#ifdef OLD
#ifdef WIN32
/* Windows tamutil is installed by the ipmirastools package from Kontron. */
static char *tam1cmd = "\"\"%ProgramFiles%\"\\Intel\\ipmirastools\\tamutil\" >NUL: 2>NUL:";
static char *tam2cmd = "\"\"%ProgramFiles%\"\\Intel\\ipmirastools\\tamutil\" |findstr TAM.Status >NUL:";
//static char * tam3cmd = "ipmiutil sensor |findstr BMC_TAM >NUL:"; 
//static char * tambcmd = "bmcTamActive"; /*old*/
#define RET_NOT_FOUND  1  /*command not found (%ERRORLEVE%=9009)*/
#else
/* Linux tamutil is installed by the ipmimisc package from Kontron. */
static char *tam1cmd = "/usr/share/ipmimisc/tamutil >/dev/null 2>&1";
static char *tam2cmd = "/usr/share/ipmimisc/tamutil 2>/dev/null |grep TAM.Status >/dev/null";
//static char * tam3cmd = "ipmiutil sensor |grep BMC_TAM  >/dev/null"; 
//static char * tambcmd = "/usr/local/tam/bin/bmcTamActive 2>/dev/null"; /*old*/
//#define RET_TAMB_ACTIVE 256   /*from bmcTamActive, if BMC TAM is enabled*/
#define RET_NOT_FOUND 32512   /*command not found ($?=127 if shell)*/ 
#endif
#endif

#ifdef NOT
#define PRIVATE_BUS_ID      0x03 // w Sahalee,  the 8574 is on Private Bus 1
#define PRIVATE_BUS_ID5     0x05 // for Intel TIGI2U
#define PRIVATE_BUS_ID7     0x07 // for Intel S5000
#define PERIPHERAL_BUS_ID   0x24 // w mBMC, the 8574 is on the Peripheral Bus
#define ALARMS_PANEL_WRITE  0x40 
#define ALARMS_PANEL_READ   0x41
#define DISK_LED_WRITE      0x44 // only used for Chesnee mBMC
#define DISK_LED_READ       0x45 // only used for Chesnee mBMC
#endif

#if defined(NO_CMD)
const char * val2str(ushort val, const struct valstr *vs)
{
        static char un_str[32];
        int i;
        for (i = 0; vs[i].str != NULL; i++) 
                if (vs[i].val == val) return vs[i].str;
        memset(un_str, 0, 32);
        snprintf(un_str, 32, "Unknown (0x%x)", val);
        return un_str;
}
#else
uchar get_nsc_diskleds(uchar busid)
{
	uchar inputData[4];
	uchar rdata[16];
	int responseLength = 4;
	uchar completionCode;
	int ret;

	inputData[0] = busid;
	inputData[1] = DISK_LED_READ;
	inputData[2] = 0x1;   // return one byte of LED data
	inputData[3] = 0x00;  // init data to zero
        ret = ipmi_cmd(MASTER_WRITE_READ, inputData, 3, rdata,
                        &responseLength, &completionCode, fdebug);
	if (ret != 0) {
           printf("get_nsc_diskleds: ret = %d, ccode %02x, leds = %02x\n",
                    ret, completionCode, rdata[0]);
	   return(0);
	   }
	return(rdata[0]);
}  /*end get_nsc_diskleds()*/

int set_nsc_diskleds(uchar val, uchar busid)
{
	uchar inputData[4];
	uchar rdata[16];
	int responseLength = 4;
	uchar completionCode;
	int ret;

	inputData[0] = busid;
	inputData[1] = DISK_LED_WRITE;
	inputData[2] = 0x01;   // len = one byte of LED data
	inputData[3] = val;    
        ret = ipmi_cmd(MASTER_WRITE_READ, inputData, 4, rdata,
                        &responseLength, &completionCode, fdebug);
	if (ret != 0) {
           printf("set_nsc_diskleds: ret = %d, ccode %02x, leds = %02x\n",
                    ret, completionCode, val);
	   return(0);
	   }
	return(ret);
}  /*end set_nsc_diskleds()*/

void show_nsc_diskleds(uchar val)
{
	if (fdebug) printf("diskled = %02x\n",val);
	printf("disk A: ");
	if ((val & 0x20) == 0) printf("present");
	else printf("not present");
	if ((val & 0x02) == 0) printf("/faulted ");
	printf("\ndisk B: ");
	if ((val & 0x10) == 0) printf("present");
	else printf("not present");
	if ((val & 0x01) == 0) printf("/faulted ");
	printf("\n");
}

uchar get_alarms_intel(uchar busid)
{
	uchar inputData[4];
	uchar rdata[16];
	int responseLength = 4;
	uchar completionCode;
	int ret;

	inputData[0] = busid;
	inputData[1] = ALARMS_PANEL_READ;
	inputData[2] = 0x1;   // return one byte of alarms data
	inputData[3] = 0x00;  // init data to zero
        ret = ipmi_cmd(MASTER_WRITE_READ, inputData, 3, rdata,
                        &responseLength, &completionCode, fdebug);
	if (ret != 0 || completionCode != 0) {
           printf("get_alarms: ret = %d, ccode %02x, alarms = %02x\n",
                    ret, completionCode, rdata[0]);
	   return(0);
	   }
	return(rdata[0]);
}  /*end get_alarms()*/

int set_alarms_intel(uchar val, uchar busid)
{
	uchar inputData[4];
	uchar rdata[16];
	int responseLength = 4;
	uchar completionCode;
	int ret;

	inputData[0] = busid; 
	inputData[1] = ALARMS_PANEL_WRITE;
	inputData[2] = 0x1;   // one byte of alarms data
	inputData[3] = val;  
        ret = ipmi_cmd(MASTER_WRITE_READ, inputData, 4, rdata,
                        &responseLength, &completionCode, fdebug);
	if (ret != 0) {
           printf("set_alarms: ret = %d, ccode %02x, value = %02x\n",
                    ret, completionCode, val);
	   return(ret);
	   }
	if (completionCode != 0) ret = completionCode;
	return(ret);
}  /*end set_alarms()*/

/* 
 * show_alarms
 *
 * The alarm control/status byte is decoded as follows:
 * bit
 * 7 = reserved, always write 1
 * 6 = LED colors, 1 = amber (default), 0 = red
 *     Colors were added in some later firmware versions, but
 *     not for all platforms.
 * 5 = Minor Relay bit, 0 = on, 1 = off, always write 1
 * 4 = Major Relay bit, 0 = on, 1 = off, always write 1
 * 3 = Minor LED bit, 0 = on, 1 = off
 * 2 = Major LED bit, 0 = on, 1 = off
 * 1 = Critical LED bit, 0 = on, 1 = off
 * 0 = Power LED bit, 0 = on, 1 = off
 *
 * Note that the Power LED is also wired to the System Fault LED 
 * in the back of the system, so this state may be off for Power,
 * but the LED could be lit for a System Fault reason instead.
 */
void show_alarms_intel(uchar val)
{
   char *scrit = "ON ";
   char *smaj = "ON ";
   char *smin = "ON ";
   char *spow = "ON ";
   char *rmaj = "ON";
   char *rmin = "ON";
   if (fdebug) printf("alarms = %02x\n",val);
    
   if (val & 0x01) spow = "off";
   if (val & 0x02) scrit = "off";
   if (val & 0x04) smaj = "off";
   if (val & 0x08) smin = "off";
   printf("Alarm LEDs:   critical = %s major = %s minor = %s power = %s\n",
           scrit,smaj,smin,spow);
   if (fRelayBits == 1) {  /*CG2100 platforms have Relay bits reversed*/
      if (val & 0x10) rmin = "off ";
      if (val & 0x20) rmaj = "off ";
   } else {
      if (val & 0x10) rmaj = "off ";
      if (val & 0x20) rmin = "off ";
   }
   printf("Alarm Relays: major = %s minor = %s\n", rmaj, rmin);
}

/*
 * get_led_status_intel
 * uses Intel OEM command to get the status of the ID LED.
 * if success, rv=0, pstate:  0=off, 1=on, 2=blinking
 */
int get_led_status_intel(uchar *pstate)
{
   uchar rdata[64];
   int rv, rlen;
   uchar cc, b_leds, bstate;

   /* This command is only supported on Intel S5000 motherboards */
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(0x40,0x32, g_sa,g_bus,g_lun,
                        NULL, 0, rdata, &rlen, &cc, fdebug);
   if (fdebug) printf("get_led_status_intel: rv = %d, cc=%02x\n", rv,cc);
   if (rv == 0 && cc != 0) rv = cc;
   if (rv == 0) {
	b_leds = rdata[0];
	bstate = 0;   /*off*/
	if (b_leds & 0x80) { bstate = 1;  /*on*/ }
	else if (b_leds & 0x40) { bstate = 2;  /*blink*/ }
	if (pstate != NULL) *pstate = bstate;
   }
   return(rv);
}


int detect_capab_intel(int vend_id,int prod_id, int *cap, int *ndisk,char fdbg)
{
     int busid = PRIVATE_BUS_ID;
     int f = 0;
     char fbmctam = 0;
     char fHasAlarms = 0;
     char fHasEnc = 0; 
     char fpicmg = 0;
     char fChesnee = 0;
     int styp, idx, rv;
     char desc[20];

     fdebug = fdbg;
     if (vend_id == VENDOR_NSC)  { /*NSC mBMC, Chesnee*/
	  busid = PERIPHERAL_BUS_ID;
          fHasAlarms = 1;
          fChesnee = 1;
     } else if (vend_id == VENDOR_INTEL) { /*Intel BMC*/
          switch (prod_id) {
             case 0x0022:   
		busid = PRIVATE_BUS_ID5;  /* Intel TIGI2U */
		fbmctam = 1;  /* Intel TIGI2U may have bmc tam */
                fHasAlarms = 1;
                fHasEnc = 2;
                break;
             case 0x000C:     /* TSRLT2 or TSRMT2 */
		busid = PRIVATE_BUS_ID;
		fbmctam = 0;  /* no BMC TAM */
                fHasAlarms = 1;
                fHasEnc = 0;
                break;
             case 0x001B:   
	        busid = PRIVATE_BUS_ID;
		fbmctam = 1;  /* Intel TIGPR2U may have bmc tam */
                fHasAlarms = 1;
                break;
             case 0x0808:   
             case 0x0841:   
	        fpicmg = 1;   /* Intel ATCA platform, supports PICMG */
                fHasAlarms = 1;
                break;
             case 0x4311:  
	        busid = PERIPHERAL_BUS_ID; /* SJR2 (NSI2U) mBMC */
                break;
             case 0x0026:     /*BridgePort*/
             case 0x0028:     /*S5000PAL*/
	     case 0x0029:     /*S5000PSL*/
             case 0x0811:     /*S5000PHB*/   
		busid = PRIVATE_BUS_ID7;  /* Intel Harbision (TIGW1U/NSW1U) */
		/* Check for SAS Drv Pres sensor on HSC, if TIGW1U */
		rv = get_sensdesc(0xC0,0x09,desc,&styp, &idx);
		if (fdebug) printf("get_sensdesc rv = %d\n",rv);
		if (rv == ERR_NOT_FOUND) { /* NSW1U does not have alarm panel*/
		   fHasAlarms = 0;
                   fHasEnc = 0;
		} else {   /* has HSC, like TIGW1U */
		   fbmctam = 1;  /* TIGW1U may have bmc tam */
                   fHasAlarms = 1;
		   if (prod_id == 0x0811) fHasEnc = 3;
                   else fHasEnc = 6;
		} 
                break;
        case 0x003E:     /*S5520UR*/
	    busid = PRIVATE_BUS_ID;
            fbmctam = 1;  /* CG2100 has bmc tam */
            fHasAlarms = 1;
            fHasEnc = 8; /* CG2100 has 8 disks */
            fRelayBits = 1;
	    set_max_kcs_loops(URNLOOPS); /*longer for cmds (default 300)*/
        break;
	case 0x005D:      /* Copper Pass, CG2200*/
	    busid = PRIVATE_BUS_ID;
            fbmctam = 1;  /* CG2200 has bmc tam */
            fHasAlarms = 1;
            fRelayBits = 1;
            fHasEnc = 6; /* 6 disks */
            set_max_kcs_loops(URNLOOPS); /*longer for cmds (default 300)*/
         break;
	 case 0x0071:      /* CG2300*/
	    busid = PRIVATE_BUS_ID;
            fbmctam = 1;  /* CG2300 has bmc tam */
            fHasAlarms = 1;
            fRelayBits = 1;
            fHasEnc = 6; /* 6 disks */
	        set_max_kcs_loops(URNLOOPS); /*longer for cmds (default 300)*/
         break;
	 case 0x0051:  /*  Eagle Pass */
            busid = PRIVATE_BUS_ID;
            fbmctam = 1;  /* CG1200 has bmc tam */
            fHasAlarms = 1;
            fRelayBits = 1;
            fHasEnc = 4; /* 4 disks */
	    set_max_kcs_loops(URNLOOPS); /*longer for cmds (default 300)*/
        break;
        case 0x0048:  /* "(S1200BT)"  *BearTooth Pass*/
        case 0x004A:  /* "(S2600CP)"  *Canoe Pass*/
        case 0x0055:  /*  Iron Pass */
        case 0x005C:  /*  Lizard Head Pass */
            fHasEnc = 8;
	        set_max_kcs_loops(URNLOOPS); /*longer for cmds (default 300)*/
	        break;
        default:
            busid = PRIVATE_BUS_ID;
            fHasEnc = 8;
        break;
          }
   } 
   if (fHasAlarms) f |= HAS_ALARMS_MASK;
   if (fbmctam)    f |= HAS_BMCTAM_MASK;
   if (fHasEnc > 0) {
		   f |= HAS_ENCL_MASK;
		   if (ndisk != NULL) *ndisk = fHasEnc;
   }
   if (fpicmg)     f |= HAS_PICMG_MASK;
   if (fChesnee)   f |= HAS_NSC_MASK;
   if (is_romley(vend_id,prod_id)) 
   {
        if (prod_id == 0x005D) fHasEnc = 6;   /*CG2200*/
        fHasEnc = 8;
        set_max_kcs_loops(URNLOOPS); /*longer for cmds (default 300)*/
        f |= HAS_ROMLEY_MASK;
   }
   if (is_grantley(vend_id,prod_id)) 
   {
        if (prod_id == 0x0071) fHasEnc = 6;   /*CG2300*/
        fHasEnc = 8;
        set_max_kcs_loops(URNLOOPS); /*longer for cmds (default 300)*/
        f |= HAS_ROMLEY_MASK;
   }
   
   *cap = f;
   return(busid); 
}

int check_bmctam_intel(void)
{
   int ret, rlen;
   uchar rdata[16];
   uchar cc;

   /* Check if BMC TAM is enabled */
   rlen = sizeof(rdata);
   ret = ipmi_cmdraw(0x00, 0x36, g_sa,g_bus,g_lun,
			NULL, 0, rdata, &rlen, &cc, fdebug);
   if ((ret == 0) && (cc == 0)) {
	printf("Warning: BMC TAM is active and managing the LEDs.\n"
		"Use tamutil (from ipmimisc rpm) to set alarms instead.\n");
	return(LAN_ERR_ABORT);
   } else return(0);
#ifdef OLD
   ret = system(tam1cmd);
   if (fdebug) printf("%s ret = %d\n",tam1cmd,ret);
   if (ret == RET_NOT_FOUND) { /*command not found, no such file*/
	      /* Could also do "ipmiutil sensor |grep BMC_TAM" (tam3cmd),
	       * but this would take a while to complete. */
	      printf("Warning: BMC TAM may be active and managing the LEDs.\n"
		  "If so, use tamutil to set the alarm LEDs instead.\n");
   } else if (ret == 0) {  
	      /*the command was found, check if BMC TAM enabled*/
	      ret = system(tam2cmd);
	      if (fdebug) printf("%s ret = %d\n",tam2cmd,ret);
	      if (ret == 0) {
	         /*If so, print warning, use Intel tamutil instead.*/
	         printf("Warning: BMC TAM is active and managing the LEDs.\n"
		   "Use tamutil or the Intel TAM API to set alarms instead.\n"
		   "Aborting.\n");
	         return(LAN_ERR_ABORT);
	      }
   } 
   /* else tamutil was there but did not show BMC TAM active, so
    *	assume BMC TAM is not active and do nothing. */
    return(ret);
#endif
}

int soft_reset_intel(uchar func)
{
   int ret, rlen;
   uchar idata[16];
   uchar rdata[16];
   uchar cc;

   /* Do an Intel S5000 soft reset, via an OS bridge agent (ipmiutil_asy) */
   rlen = sizeof(rdata);
   idata[0] = func;  /* 0=read, 1=shutdown, 2=reset */
   ret = ipmi_cmdraw(0x70, 0x30, g_sa,g_bus,g_lun,
			idata, 1, rdata, &rlen, &cc, fdebug);
   if ((ret == 0) && (cc != 0)) ret = cc;
   return(ret);
}

int lan_failover_intel(uchar func, uchar *mode)
{
   int ret, rlen;
   uchar idata[16];
   uchar rdata[16];
   uchar cc;

   /* Do an Intel S2600 LAN Failover command, where func is: 
    *  0x00=disable,
    *  0x01=enable with leash monitor, 
    *  0x02=enable with ARP monitor if blade,
    *  0xFF=no set, just get current mode
    */
   rlen = sizeof(rdata);
   idata[0] = func;  /* 0=read, 1=shutdown, 2=reset */
   ret = ipmi_cmdraw(0x40, 0x3E, g_sa,g_bus,g_lun,
			idata, 1, rdata, &rlen, &cc, fdebug);
   if ((ret == 0) && (cc != 0)) ret = cc;
   if (ret == 0 && mode != NULL) *mode = rdata[0];
   return(ret);
}

int get_power_restore_delay_intel(int *delay)
{
   int ret, rlen;
   uchar idata[16];
   uchar rdata[16];
   uchar cc;

   rlen = sizeof(rdata);
   ret = ipmi_cmdraw(0x55, 0x30, g_sa,g_bus,g_lun,
			idata, 0, rdata, &rlen, &cc, fdebug);
   if ((ret == 0) && (cc != 0)) ret = cc;
   if (ret == 0 && delay != NULL) 
	 *delay = ( rdata[1] + ((rdata[0] & 0x07) << 8) );
   return(ret);
}

/* end-else NO_CMD not defined */
#endif

#define NTAMSEV  8
static char *tam_sev[] = {
/*0*/ "OFF",
/*1*/ "MNR",
/*2*/ "MNR+P",
/*3*/ "MJR",
/*4*/ "MJR+P",
/*5*/ "CRT",
/*6*/ "CRT+P",
/*7*/ "UNK"
};

int decode_sensor_intel_nm(uchar *sdr,uchar *reading,char *pstring, int slen)
{
   int rv = -1;
   uchar stype;
   char mystr[60];
   uchar nm_sa, chan, lun;

   if (sdr == NULL) return(rv);
   if (pstring == NULL || slen == 0) return(rv);
   switch(sdr[3]) {  /*SDR type*/
	case 0xC0:  /*OEM sensor*/
	   if (sdr[8] == 0x0D) {   /* OEM NM Reference SDR, no reading */
		nm_sa = sdr[10];
		chan = (sdr[11] & 0xf0) >> 4;
		lun = (sdr[11] & 0x0f);
		/* show NM location and sensor numbers for NM sensors */
		sprintf(mystr,"NM(%x,%x,%x) health=%x excep=%x capab=%x thresh=%x",
			chan,nm_sa,lun, sdr[12],sdr[13],sdr[14],sdr[15]);
		strncpy(pstring, mystr, slen);
		if ((int)strlen(mystr) > slen) pstring[slen-1] = 0; /*string*/
		rv = 0;
	   }
	   break;
	case 0x02:	/*compact sensor*/
	   if (reading == NULL) return(rv);
	   stype = sdr[12];  /*sensor type*/
	   if (stype == 0xDC) { /* NM Capabilities sensor, usu snum 0x1a (26.)*/
		mystr[0] = 0;
		if (reading[2] == 0x00) strcat(mystr,"None"); 
		else {  
		   if (reading[2] & 0x01) strcat(mystr,"Policy "); 
		   if (reading[2] & 0x02) strcat(mystr,"Monitor "); 
		   if (reading[2] & 0x04) strcat(mystr,"Power "); 
		}
		strncpy(pstring, mystr, slen);
		if ((int)strlen(mystr) > slen) pstring[slen-1] = 0; /*string*/
		rv = 0;
	   }
	   break;
	default:
	   break;
   }
   return(rv);
}

static void show_oem_hex(uchar *sdr, int slen)
{
   int i;
   for (i = 8; i < slen; i++)
	printf("%02x ",sdr[i]);
   printf("\n");
}

void show_oemsdr_nm(uchar *sdr)
{
    int rv, len;
    char mystr[60];

    /* vendor id has already been shown */
    len = sdr[4] + 5;
    rv = decode_sensor_intel_nm(sdr,NULL,mystr,sizeof(mystr));
    if (rv == 0) printf("%s\n",mystr);
    else show_oem_hex(sdr, len);
    return;
}

/* 
 * show_oemsdr_intel
 */
void show_oemsdr_intel(uchar *sdr)
{
    uchar idx, len, c, i, n, j, k, t;
    int vend;

    len = sdr[4] + 5;
    /*double-check that this is an Intel OEM SDR*/
    vend = sdr[5] | (sdr[6] << 8) | (sdr[7] << 16);
    if (vend != VENDOR_INTEL) {
	if (fdebug) printf("show_oemsdr_intel: vendor %x != %x (Intel)\n",
				vend,VENDOR_INTEL);
	return;
    }
    printf("Intel: "); 
    switch(sdr[8]) {  /*OEM subtype*/
    case 0x53:   /* SDR version subtype (has ASCII) */
       for (i = 8; i < len; i++) {
           c = sdr[i];
           if (c < 0x20 || c > 0x7f) printf("[%02x]",c);
           else printf("%c",c);
       }
       printf("\n");
       break;
    case 0x60:  /* BMC TAM subtype */
       idx = (sdr[10] & 0xf0) >> 4;
       n = (sdr[10] & 0x0f) + 1;  /*number of TAM records*/
       printf("BMC_TAM%d ",idx);
       for (i = 8; i < len; i++) 
   	printf("%02x ",sdr[i]);
       if (idx == 0) {
           printf(" nrec=%d cfg=%02x",n,sdr[11]);
       }
       printf("\n");
       if (fdebug || sens_verbose) {
         /* show decoded BMC_TAM rules */
         if (idx > 0) {
           uchar map, off, sev, sa;
           const char *tstr;
           sa = sdr[12];
           for (i = 13; i < len; ) {
              k = (sdr[i] & 0xf0) >> 4;
              t = sdr[i+1];
	      tstr = get_sensor_type_desc(t); 
              printf("\tBMC_TAM%d sa=%02x %s (",idx,sa,tstr);
              for (j = 0; j < k; j++) {
                 map = sdr[i+3+j];
                 off = (map & 0xf0) >> 4;
                 sev = map & 0x0f;
                 if (sev >= NTAMSEV) sev = NTAMSEV - 1;
                 printf("%d=%s ",off,tam_sev[sev]);
              }
              printf(")\n");
              i += 3 + k;
           }
         }
       }
       break;
    case 0x0C: /* Fan Speed Control */
       printf("FanCtl ");
       show_oem_hex(sdr, len);
       break;
    case 0x0D: /* ME NM Reference SDR */
       show_oemsdr_nm(sdr);
       break;
    case 0x02: /*S5500 Power Unit Redundancy subtype*/
    case 0x05: /*S5500 Fan Redundancy subtype*/
    case 0x06: /*S5000 System Information/Capab */
    case 0x09: /*S5500 Voltage sensor scaling*/
    case 0x0A: /*S5500 Fan sensor scaling*/
    case 0x0B: /*S5500 Thermal Profile data*/
    default:   /* other subtypes 07,0e,15 etc. */
       show_oem_hex(sdr, len);
       break;
    } /*end switch*/
} /*end show_oemsdr_intel*/

#ifdef NO_EVENTS
/* if not linking with ievents.c, need to skip decode_sel_intel because it 
 * would have unresolved externals for fmt_time, get_sev_str, get_sensor_tag */
#else
/* 
 * decode_sel_intel
 * inputs:
 *    evt    = the 16-byte IPMI SEL event
 *    outbuf = points to the output string buffer
 *    outsz  = size of the output buffer
 * outputs: 
 *    rv     = 0 if this event was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    outbuf = will contain the interpreted event text string (if rv==0)
 */
int decode_sel_intel(uchar *evt, char *outbuf, int outsz, char fdesc,
			char fdebug)
{
   int rv = -1;
   ushort id;
   uchar rectype;
   ulong timestamp;
   char mybuf[64]; 
   char oembuf[64]; 
   char *type_str = NULL;
   char *pstr = NULL;
   int sevid;
   ushort genid;
   uchar snum;
   char *p1;
   int d, f;
         
   sevid = SEV_INFO;
   id = evt[0] + (evt[1] << 8);
   rectype = evt[2];
   snum = evt[11];
   timestamp = evt[3] + (evt[4] << 8) + (evt[5] << 16) + (evt[6] << 24);
   genid = evt[7] | (evt[8] << 8);
   if (rectype == 0x00) {
      if (snum == 0x0A && evt[12] == 0x03) rectype = 0x02; /*internal wdog*/
   } 
   if (rectype == 0x02) {
      type_str = "";
      sprintf(mybuf,"%02x [%02x %02x %02x]", evt[12],evt[13],evt[14],evt[15]);
      pstr = ""; /*default*/
      switch(evt[10]) {  /*sensor type*/
	case 0x00:	/* type undefined */
	   // type_str = get_sensor_type_desc(0x28);
	   type_str = "Management Subsystem Health";
           if (snum == 0x0A && evt[12] == 0x03) {  /*internal wd event*/
		pstr = "BMC wd restart";
		sevid = SEV_CRIT; 
		rv = 0;
	   }
	   break;
	case 0x13:	/* Critical Interrupt */
	   type_str = "Critical Interrupt";
	   if (evt[12] == 0x70) { /*event trigger/type = OEM AER events */
	      /* AER doc uses 'Fatal' here, but they are not really fatal.*/
	      pstr = &oembuf[0];
	      switch(evt[13]) {  /*data1/offset*/
              case 0xA0: p1 = "PCIe Data Link Protocol Error"; break;
              case 0xA1: p1 = "PCIe Surprise Link Down"; break;
              case 0xA2: p1 = "PCIe Unexpected Completion"; break;
              case 0xA3: p1 = "PCIe Unsupported Request"; break;
              case 0xA4: p1 = "PCIe Poisoned TLP"; break;
              case 0xA5: p1 = "PCIe Flow Control Protocol"; break;
              case 0xA6: p1 = "PCIe Completion Timeout"; break;
              case 0xA7: p1 = "PCIe Completer Abort"; break;
              case 0xA8: p1 = "PCIe Recv Buffer Overflow"; break;
              case 0xA9: p1 = "PCIe ACS Violation"; break;
              case 0xAA: p1 = "PCIe Malformed TLP"; break;
              case 0xAB: p1 = "PCIe Recvd Fatal Message"; break;
              case 0xAC: p1 = "PCIe Unexpected Completion Error"; break;
              case 0xAD: p1 = "PCIe Recvd Warning Message"; break;
              default:   p1 = "PCIe Other AER"; break;
              }
	      rv = 0;  
	      sevid = SEV_MAJ;
	      /* also include the bus dev/func bytes (as shown by lspci) */
	      d = (evt[15] & 0xF8) >> 3;
	      f = (evt[15] & 0x07);
              snprintf(oembuf,sizeof(oembuf),"%s on (%02x:%02x.%d)",
			p1,evt[14],d,f);
	   }
	   if (evt[12] == 0x71) { /*event trigger/type = OEM AER warnings */
	      pstr = &oembuf[0];
	      switch(evt[13]) {  /*data1/offset*/
              case 0xA0: p1 = "PCIe Warn Receiver Error"; break;
              case 0xA1: p1 = "PCIe Warn Bad DLLP"; break;
              case 0xA2: p1 = "PCIe Warn Bad TLLP"; break;
              case 0xA3: p1 = "PCIe Warn Replay Num Rollover"; break;
              case 0xA4: p1 = "PCIe Warn Replay Timeout"; break;
              case 0xA5: p1 = "PCIe Warn Advisory Non-Fatal"; break;
              case 0xA6: p1 = "PCIe Warn Link BW Changed"; break;
              default:   p1 = "PCIe Warn Other AER"; break;
	      }
	      rv = 0;  
	      sevid = SEV_MIN;
	      /* also include the bus dev/func bytes (as shown by lspci) */
	      d = (evt[15] & 0xF8) >> 3;
	      f = (evt[15] & 0x07);
              snprintf(oembuf,sizeof(oembuf),"%s on (%02x:%02x.%d)",
			p1,evt[14],d,f);
	   }
           break;
	case 0x2B:	/* Version Change */
	   type_str = "Version Change";
	   if (evt[12] == 0x70) { /*event trigger/type */
	      switch(evt[13]) {  /*data1/offset*/
              case 0x00: pstr = "Update started"; break;
              case 0x01: pstr = "Update completed"; break;
              case 0x02: pstr = "Update failed"; sevid = SEV_MIN; break;
	      default:   pstr = "-"; break;
	      }
	      rv = 0;
	   }
           break;
	default:  break;
      }
      if (rv == 0) {
	 format_event(id,timestamp, sevid, genid, type_str,
			snum,NULL,pstr,mybuf,outbuf,outsz);
      }
   } 
   return rv;
}  /*end decode_sel_intel*/
#endif

const struct valstr intel_mem_s2600[] = {  
 {  0, "DIMM_A1" },
 {  1, "DIMM_A2" },
 {  2, "DIMM_A3" },
 {  3, "DIMM_B1" },
 {  4, "DIMM_B2" },
 {  5, "DIMM_B3" },
 {  6, "DIMM_C1" },
 {  7, "DIMM_C2" },
 {  8, "DIMM_C3" },
 {  9, "DIMM_D1" },
 { 10, "DIMM_D2" },
 { 11, "DIMM_D3" },
 { 12, "DIMM_E1" },
 { 13, "DIMM_E2" },
 { 14, "DIMM_E3" },
 { 15, "DIMM_F1" },
 { 16, "DIMM_F2" },
 { 17, "DIMM_F3" },
 { 18, "DIMM_G1" },
 { 19, "DIMM_G2" },
 { 20, "DIMM_G3" },
 { 21, "DIMM_H1" },
 { 22, "DIMM_H2" },
 { 23, "DIMM_H3" },
 { 24, "DIMM_I1" },
 { 25, "DIMM_I2" },
 { 26, "DIMM_I3" },
 { 27, "DIMM_J1" },
 { 28, "DIMM_J2" },
 { 29, "DIMM_J3" },
 { 30, "DIMM_K1" },
 { 31, "DIMM_K2" },
 { 32, "DIMM_K3" },
 { 33 , NULL }  /*end of list*/
};

const struct valstr intel_mem_s5520ur[] = {  
 {  0, "DIMM_A1" },
 {  1, "DIMM_A2" },
 {  2, "DIMM_B1" },
 {  3, "DIMM_B2" },
 {  4, "DIMM_C1" },
 {  5, "DIMM_C2" },
 {  6, "DIMM_D1" },
 {  7, "DIMM_D2" },
 {  8, "DIMM_E1" },
 {  9, "DIMM_E2" },
 { 10, "DIMM_F1" },
 { 11, "DIMM_F2" },
 { 12 , NULL }  /*end of list*/
};
const struct valstr intel_mem_s5000phb[] = {  
 { 0, "DIMM_A1" },
 { 1, "DIMM_A2" },
 { 2, "DIMM_A3" },
 { 3, "DIMM_B1" },
 { 4, "DIMM_B2" },
 { 5, "DIMM_B3" },
 { 6 , NULL }  /*end of list*/
};
const struct valstr intel_mem_s5000pal[] = {  
 { 0, "DIMM_A1" },
 { 1, "DIMM_A2" },
 { 2, "DIMM_B1" },
 { 3, "DIMM_B2" },
 { 4, "DIMM_C1" },
 { 5, "DIMM_C2" },
 { 6, "DIMM_D1" },
 { 7, "DIMM_D2" },
 { 8 , NULL }  /*end of list*/
};
const struct valstr intel_mem_tigi2u[] = {  
 { 0, "DIMM_1B" },
 { 1, "DIMM_1A" },
 { 2, "DIMM_2B" },
 { 3, "DIMM_2A" },
 { 4, "DIMM_3B" },
 { 5, "DIMM_3A" },
 { 6 , NULL }  /*end of list*/
};

#define LIDS   8
ushort lan2i_ids[LIDS] = {  /*Intel prod_ids that use lan2i, rest use lan2*/
                  0x0000, /*uninitialized */
                  0x0022, /*TIGI2U */
                  0x0026, /*Bridgeport */
                  0x0028, /*S5000PAL, Alcolu*/
                  0x0029, /*S5000PSL, StarLake*/
                  0x002B, /*S5000VSA */
                  0x002D, /*ClearBay*/
                  0x0811  /*S5000PHB, TIGW1U*/ };

#define RIDS   21 /* Intel Romley product ids: */
struct { ushort id; char *desc; } romleys[RIDS] = {
 { 0x0048, "S1200BT" },  /* S1200BT, BearTooth Pass  */
 { 0x0049, "S2600GL" },  /* S2600GL, S2600GZ   */
 { 0x004A, "S2600CP" },  /* S2600CP, Canoe Pass  */
 { 0x004D, "S2600JF" },  /* S2600JF, Jefferson Pass, Appro 512X */
 { 0x004E, "S2600WP" },  /* S2600WP  */
 { 0x004F, "S2400SC" },  /* S2400SC  */
 { 0x0050, "S2400LP" },  /* S2400LP   */
 { 0x0051, "S2400EP" },  /* S2400EP, Eagle Pass */
 { 0x0052, "S1400FP" },  /* S1400FP   */
 { 0x0053, "S1400SP" },  /* S1400SP   */
 { 0x0054, "S2600KI" },  /* S2600KI   */
 { 0x0055, "S2600IP" },  /* S2600IP, Iron Pass   */
 { 0x0056, "W2600CR" },  /* W2600CR   */
 { 0x0057, "S2400GP" },  /* S2400GP   */
 { 0x0058, "Badger Pass" },  /* Badger Pass  */
 { 0x0059, "S2400BB" },  /* S2400BB   */
 { 0x005A, "Taylor Pass" },  /* Taylor Pass  */
 { 0x005B, "S1600JP" },  /* S1600JP   */
 { 0x005C, "S4600LH" },  /* S4600LH, Lizard Head Pass  */
 { 0x005D, "CG2200" },   /* S2600CO, Copper Pass, Kontron CG2200  */
 { 0x005E, "Big Ridge"} /* Big Ridge */
};

#define GIDS   3 /* Intel Grantley product ids: */
struct { ushort id; char *desc; } grantleys[GIDS] = {
 { 0x006F, "AXXRMM4" },  /* S2600, AXXRMM4LITE */
 { 0x0070, "S2600" },    /* S2600CO, KP, TP, */
 { 0x0071, "S2600CW" }   /* CG2300, Kontron CG2300  */
};

#define TIDS   5
ushort thurley_ids[TIDS] = { /* Intel Thurley product ids: */
  0x003A,   /* Snow Hill */
  0x003B,   /* Shoffner */
  0x003D,   /* Melstone */
  0x003E,   /* S5520UR, S5500WB, Kontron CG2100, Penguin Computing Relion 700 */
  0x0040 }; /* Stoutland, Quanta QSSC-S4R/Appro GB812X-CN (Nehalem-EX) */

int is_romley(int vend, int prod) 
{
  int ret = 0;
  int i;
  if (vend != VENDOR_INTEL) return(ret);
  for (i = 0; i < RIDS; i++) 
     if ((ushort)prod == romleys[i].id) { ret = 1; break; }
  return(ret);
}

int intel_romley_desc(int vend, int prod, char **pdesc) 
{
  int ret = -1;
  int i;
  if (vend != VENDOR_INTEL) return(ret);
  if (pdesc == NULL) return(ret);
  for (i = 0; i < RIDS; i++) {
     if ((ushort)prod == romleys[i].id) { 
	*pdesc = romleys[i].desc;
	ret = 0; 
	break; 
     }
  }
  return(ret);
}

int is_grantley(int vend, int prod) 
{
  int ret = 0;
  int i;
  if (vend != VENDOR_INTEL) return(ret);
  for (i = 0; i < GIDS; i++) 
     if ((ushort)prod == grantleys[i].id) { ret = 1; break; }
  /* For now, assume all Intel prods > 0071 act same as Grantley */
  if (((ushort)prod > grantleys[GIDS-1].id) && ((ushort)prod < 0x00FF)) 
     ret = 1; 
  return(ret);
}

int intel_grantley_desc(int vend, int prod, char **pdesc) 
{
  int ret = -1;
  int i;
  if (vend != VENDOR_INTEL) return(ret);
  if (pdesc == NULL) return(ret);
  for (i = 0; i < GIDS; i++) {
     if ((ushort)prod == grantleys[i].id) { 
	*pdesc = grantleys[i].desc;
	ret = 0; 
	break; 
     }
  }
  if (ret == 0 && (**pdesc == '\0')) *pdesc = "S2600";
  return(ret);
}

int is_thurley(int vend, int prod) 
{
  int ret = 0;
  int i;
  if (vend != VENDOR_INTEL) return(ret);
  for (i = 0; i < TIDS; i++) 
     if ((ushort)prod == thurley_ids[i]) { ret = 1; break; }
  return(ret);
}

int is_lan2intel(int vend, int prod) 
{
  int ret = 0;
  int i; 
  if (vend != VENDOR_INTEL) return(ret);
  if (is_thurley(vend,prod) || is_romley(vend,prod)) 
    ret = 0; /*iBMC does not use lan2i*/
  else {
    for (i = 0; i < LIDS; i++) 
       if ((ushort)prod == lan2i_ids[i]) { ret = 1; break; }
  }
  return(ret);
}

int decode_mem_intel(int prod, uchar b2, uchar b3, char *desc, int *psz)
{
   const char *pstr = NULL;
   int array, dimm;
   int node, chan, sock, n;
   int rv = -1;
   uchar bdata;
   int vend;
   int iBMC = 0;
			      
   if ((desc == NULL) || (psz == NULL)) return -1;

   vend = VENDOR_INTEL;
   if (is_thurley(vend,prod)) iBMC = 1;
   if (is_romley(vend,prod))  iBMC = 2;
   if (iBMC != 0) {
      /* custom DIMM decoding for iBMC on Intel S5500 and S2600 */
      // rank = (b2 & 0x03); /* psel->event_data2 & 0x03; */
      node = (b3 & 0xE0) >> 5; /*socket*/
      chan = (b3 & 0x18) >> 3;
      sock = (b3 & 0x07);
      array = 0; /* is 0 for Thurley & Romley currently, else see b2. */
      if (iBMC == 1) dimm = (node * 6) + (chan * 2) + sock;
      else           dimm = (node * 12) + (chan * 3) + sock;
      if (fdebug) printf("iBMC DIMM (%d,%d,%d) = idx %d\n",
			  node,chan,sock,dimm);
   } else {  /* use straight DIMM index */
      /* for mini-BMC, data2 is dimm index, data3 is syndrome */
      if (prod == 0x4311) bdata = b2;  /*mini-BMC*/
      else if (b3 == 0xff) bdata = b2;  /*ff is reserved*/
      else  bdata = b3; /* normal case */
      /* (data3 & 0xc0) = SMBIOS type 16 mem array */
      array = (bdata & 0xc0) >> 6;
      /* (data3 & 0x3f) = SMBIOS type 17 dimm index */
      dimm = bdata & 0x3f;
   }
			
   if (! is_remote()) {
      fsm_debug = fdebug;
      rv = get_MemDesc(array,dimm,desc,psz);
      /* if (rv != 0) desc has "DIMM(%d)" */ 
   } 
   if (rv != 0) {  
      /* either remote, or get_MemDesc failed, use common product defaults*/
      switch(prod) {
      case 0x0811:  /*S5000PHB*/
	pstr = val2str(dimm,intel_mem_s5000phb);
	break;
      case 0x0028:  /*S5000PAL*/
	pstr = val2str(dimm,intel_mem_s5000pal);
	break;
      case 0x0022:  /*TIGI2U*/
	pstr = val2str(dimm,intel_mem_tigi2u);
	break;
      default:
        if (iBMC == 1)      pstr = val2str(dimm,intel_mem_s5520ur); 
        else if (iBMC == 2) pstr = val2str(dimm,intel_mem_s2600); 
	else  rv = -2;  /*do not guess, use raw index below*/
	break;
      }
      if (pstr != NULL) rv = 0;
      if (rv == 0) {
        /* These strings are usually 7 chars, desc is 80 chars */
        n = strlen_(pstr);
        strncpy(desc, pstr, n+1);
      } else {
        if (bdata == 0xFF) n = sprintf(desc,DIMM_UNKNOWN);  /* invalid */
        else n = sprintf(desc,DIMM_NUM,dimm);
      } 
      *psz = n;
   }
   return(rv);
} /*end decode_mem_intel*/

/*
 * decode_sensor_intel
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
int decode_sensor_intel(uchar *sdr,uchar *reading,char *pstring, int slen)
{
   int rv = -1;
   uchar stype;
   char *pstr = NULL;

   if (sdr == NULL || reading == NULL) return(rv);
   if (pstring == NULL || slen == 0) return(rv);
   if (sdr[3] == 0x02) {  /*Compact SDR*/
     stype = sdr[12];
     switch(stype) {
	case 0xC0:	/* SMI State, NMI State */
	case 0xC7:	/* FanBoost */
	case 0xCC:	/* Debug Info */
	case 0xD8:	/* BIST */
	case 0xF0:	/* ATCA HotSwap, TODO: refine this */
	case 0xF3:	/* SMI Timeout, etc. */
	case 0xF6:	/* Sensor Failure */
	case 0xF7:	/* FSB Mismatch */
	   if (reading[2] & 0x01) pstr = "Asserted"; /*Asserted, error*/
           else pstr = "OK";               /*deasserted*/
	   strncpy(pstring, pstr, slen);
	   rv = 0;
	   break;
	case 0xDC:	/* NM Capabilities sensor */
	   rv = decode_sensor_intel_nm(sdr,reading,pstring,slen);
	   break;
	default:
	   break;
     }
   } else if (sdr[3] == 0xC0) {  /*OEM SDR*/
	   rv = decode_sensor_intel_nm(sdr,reading,pstring,slen);
   }
   return(rv);
}

const struct valstr intel_s5000_post[] = {  /*from S5000 TPS*/
 { 0x0012, "CMOS date/time not set" },
 { 0x0048, "Password check failed" },
 { 0x004C, "Keyboard/interface error" },
 { 0x0108, "Keyboard locked error" },
 { 0x0109, "Keyboard stuck key error" },
 { 0x0113, "The SAS RAID firmware cannot run properly, reflash" },
 { 0x0140, "PCI PERR detected" },
 { 0x0141, "PCI resource conflict" },
 { 0x0146, "Insufficient memory to shadow PCI ROM" },
 { 0x0192, "L3 cache size mismatch" },
 { 0x0194, "CPUID, processor family are different" },
 { 0x0195, "Front side bus mismatch" },
 { 0x0197, "Processor speeds mismatched" },
 { 0x5220, "Configuration cleared by jumper" },
 { 0x5221, "Passwords cleared by jumper" },
 { 0x5223, "Configuration default loaded" },
 { 0x8110, "Proc1 internal error (IERR) on last boot" },
 { 0x8111, "Proc2 internal error (IERR) on last boot" },
 { 0x8120, "Proc1 thermal trip error on last boot" },
 { 0x8121, "Proc2 thermal trip error on last boot" },
 { 0x8130, "Proc1 disabled" },
 { 0x8131, "Proc2 disabled" },
 { 0x8160, "Proc1 unable to apply BIOS update" },
 { 0x8161, "Proc2 unable to apply BIOS update" },
 { 0x8170, "Proc1 failed Self Test (BIST)" },
 { 0x8171, "Proc2 failed Self Test (BIST)" },
 { 0x8180, "Proc1 BIOS does not support current CPU stepping" },
 { 0x8181, "Proc2 BIOS does not support current CPU stepping" },
 { 0x8190, "Watchdog timer failed on last boot" },
 { 0x8198, "OS boot watchdog timer expired on last boot" },
 { 0x8300, "Baseboard management controller failed self-test" },
 { 0x8306, "Front panel controller locked" },
 { 0x8305, "Hot swap controller failed" },
 { 0x84F2, "Baseboard management controller failed to respond" },
 { 0x84F3, "Baseboard management controller in update mode" },
 { 0x84F4, "Sensor data record empty" },
 { 0x84FF, "System event log full" },
 { 0x8500, "Memory could not be configured in the selected RAS mode" },
 { 0x8510, "Memory above 16GB maximum" }, /*S5000V only*/
 { 0x8520, "DIMM_A1 failed Self Test (BIST)" },
 { 0x8521, "DIMM_A2 failed Self Test (BIST)" },
 { 0x8522, "DIMM_A3 failed Self Test (BIST)" },
 { 0x8523, "DIMM_A4 failed Self Test (BIST)" },
 { 0x8524, "DIMM_B1 failed Self Test (BIST)" },
 { 0x8525, "DIMM_B2 failed Self Test (BIST)" },
 { 0x8526, "DIMM_B3 failed Self Test (BIST)" },
 { 0x8527, "DIMM_B4 failed Self Test (BIST)" },
 { 0x8528, "DIMM_C1 failed Self Test (BIST)" },
 { 0x8529, "DIMM_C2 failed Self Test (BIST)" },
 { 0x852A, "DIMM_C3 failed Self Test (BIST)" },
 { 0x852B, "DIMM_C4 failed Self Test (BIST)" },
 { 0x852C, "DIMM_D1 failed Self Test (BIST)" },
 { 0x852D, "DIMM_D2 failed Self Test (BIST)" },
 { 0x852E, "DIMM_D3 failed Self Test (BIST)" },
 { 0x852F, "DIMM_D4 failed Self Test (BIST)" },
 { 0x8540, "Memory lost redundancy during last boot" },
 { 0x8580, "DIMM_A1 Correctable ECC error" },
 { 0x8581, "DIMM_A2 Correctable ECC error" },
 { 0x8582, "DIMM_A3 Correctable ECC error" },
 { 0x8583, "DIMM_A4 Correctable ECC error" },
 { 0x8584, "DIMM_B1 Correctable ECC error" },
 { 0x8585, "DIMM_B2 Correctable ECC error" },
 { 0x8586, "DIMM_B3 Correctable ECC error" },
 { 0x8587, "DIMM_B4 Correctable ECC error" },
 { 0x8588, "DIMM_C1 Correctable ECC error" },
 { 0x8589, "DIMM_C2 Correctable ECC error" },
 { 0x858A, "DIMM_C3 Correctable ECC error" },
 { 0x858B, "DIMM_C4 Correctable ECC error" },
 { 0x858C, "DIMM_D1 Correctable ECC error" },
 { 0x858D, "DIMM_D2 Correctable ECC error" },
 { 0x858E, "DIMM_D3 Correctable ECC error" },
 { 0x858F, "DIMM_D4 Correctable ECC error" },
 { 0x8600, "Primary and secondary BIOS IDs do not match" },
 { 0x8601, "BIOS Bank Override jumper set to lower bank" },
 { 0x8602, "WatchDog timer expired (check secondary BIOS bank)" },
 { 0x8603, "Secondary BIOS checksum fail" },
 { 0xffff , NULL }  /*end of list*/
};
const struct valstr intel_s5500_post[] = {  /*from S5520UR TPS*/
 { 0x0012, "CMOS date/time not set" },
 { 0x0048, "Password check failed" },
 { 0x0108, "Keyboard locked error" },
 { 0x0109, "Keyboard stuck key error" },
 { 0x0113, "The SAS RAID firmware cannot run properly" },
 { 0x0140, "PCI PERR detected" },
 { 0x0141, "PCI resource conflict" },
 { 0x0146, "PCI out of resources error" },
 { 0x0192, "Processor cache size mismatch" },
 { 0x0194, "Processor family mismatch" },
 { 0x0195, "Processor QPI speed mismatch" },
 { 0x0196, "Processor Model mismatch" },
 { 0x0197, "Processor speeds mismatched" },
 { 0x0198, "Processor family is unsupported" },
 { 0x019F, "Processor/chipset stepping configuration is unsupported" },
 { 0x5220, "CMOS/NVRAM Configuration Cleared" },
 { 0x5221, "Passwords cleared by jumper" },
 { 0x5224, "Password clear jumper is Set" },
 { 0x8110, "Proc1 internal error (IERR) on last boot" }, /*not used*/
 { 0x8111, "Proc2 internal error (IERR) on last boot" }, /*not used*/
 { 0x8120, "Proc1 thermal trip error on last boot" }, /*not used*/
 { 0x8121, "Proc2 thermal trip error on last boot" }, /*not used*/
 { 0x8130, "Proc1 disabled" }, /*not used*/
 { 0x8131, "Proc2 disabled" }, /*not used*/
 { 0x8140, "Proc1 Failed FRB-3 Timer" }, /*not used*/
 { 0x8141, "Proc2 Failed FRB-3 Timer" }, /*not used*/
 { 0x8160, "Proc1 unable to apply microcode update" },
 { 0x8161, "Proc2 unable to apply microcode update" },
 { 0x8170, "Proc1 failed Self Test (BIST)" },  /*not used*/
 { 0x8171, "Proc2 failed Self Test (BIST)" },  /*not used*/
 { 0x8180, "Processor microcode update not found" },
 { 0x8190, "Watchdog timer failed on last boot" },
 { 0x8198, "OS boot watchdog timer expired on last boot" },
 { 0x8300, "iBMC failed self-test" },
 { 0x8305, "Hotswap controller failure" },
 { 0x84F2, "iBMC failed to respond" },
 { 0x84F3, "iBMC in update mode" },
 { 0x84F4, "Sensor data record empty" },
 { 0x84FF, "System event log full" },
 { 0x8500, "Memory could not be configured in the selected RAS mode" },
 { 0x8520, "DIMM_A1 failed Self Test (BIST)" },
 { 0x8521, "DIMM_A2 failed Self Test (BIST)" },
 { 0x8522, "DIMM_B1 failed Self Test (BIST)" },
 { 0x8523, "DIMM_B2 failed Self Test (BIST)" },
 { 0x8524, "DIMM_C1 failed Self Test (BIST)" },
 { 0x8525, "DIMM_C2 failed Self Test (BIST)" },
 { 0x8526, "DIMM_D1 failed Self Test (BIST)" },
 { 0x8527, "DIMM_D2 failed Self Test (BIST)" },
 { 0x8528, "DIMM_E1 failed Self Test (BIST)" },
 { 0x8529, "DIMM_E2 failed Self Test (BIST)" },
 { 0x852A, "DIMM_F1 failed Self Test (BIST)" },
 { 0x852B, "DIMM_F2 failed Self Test (BIST)" },
 { 0x8540, "DIMM_A1 Disabled" },
 { 0x8541, "DIMM_A2 Disabled" },
 { 0x8542, "DIMM_B1 Disabled" },
 { 0x8543, "DIMM_B2 Disabled" },
 { 0x8544, "DIMM_C1 Disabled" },
 { 0x8545, "DIMM_C2 Disabled" },
 { 0x8546, "DIMM_D1 Disabled" },
 { 0x8547, "DIMM_D2 Disabled" },
 { 0x8548, "DIMM_E1 Disabled" },
 { 0x8549, "DIMM_E2 Disabled" },
 { 0x854A, "DIMM_F1 Disabled" },
 { 0x854B, "DIMM_F2 Disabled" },
 { 0x8560, "DIMM_A1 SPD fail error." },
 { 0x8561, "DIMM_A2 SPD fail error" },
 { 0x8562, "DIMM_B1 SPD fail error" },
 { 0x8563, "DIMM_B2 SPD fail error" },
 { 0x8564, "DIMM_C1 SPD fail error" },
 { 0x8565, "DIMM_C2 SPD fail error" },
 { 0x8566, "DIMM_D1 SPD fail error" },
 { 0x8567, "DIMM_D2 SPD fail error" },
 { 0x8568, "DIMM_E1 SPD fail error" },
 { 0x8569, "DIMM_E2 SPD fail error" },
 { 0x856A, "DIMM_F1 SPD fail error" },
 { 0x856B, "DIMM_F2 SPD fail error" },
 { 0x8580, "DIMM_A1 Correctable ECC error" },
 { 0x8581, "DIMM_A2 Correctable ECC error" },
 { 0x8582, "DIMM_B1 Correctable ECC error" },
 { 0x8583, "DIMM_B2 Correctable ECC error" },
 { 0x8584, "DIMM_C1 Correctable ECC error" },
 { 0x8585, "DIMM_C2 Correctable ECC error" },
 { 0x8586, "DIMM_D1 Correctable ECC error" },
 { 0x8587, "DIMM_D2 Correctable ECC error" },
 { 0x8588, "DIMM_E1 Correctable ECC error" },
 { 0x8589, "DIMM_E2 Correctable ECC error" },
 { 0x858A, "DIMM_F1 Correctable ECC error" },
 { 0x858B, "DIMM_F2 Correctable ECC error" },
 { 0x85A0, "DIMM_A1 Uncorrectable ECC error" },
 { 0x85A1, "DIMM_A2 Uncorrectable ECC error" },
 { 0x85A2, "DIMM_B1 Uncorrectable ECC error" },
 { 0x85A3, "DIMM_B2 Uncorrectable ECC error" },
 { 0x85A4, "DIMM_C1 Uncorrectable ECC error" },
 { 0x85A5, "DIMM_C2 Uncorrectable ECC error" },
 { 0x85A6, "DIMM_D1 Uncorrectable ECC error" },
 { 0x85A7, "DIMM_D2 Uncorrectable ECC error" },
 { 0x85A8, "DIMM_E1 Uncorrectable ECC error" },
 { 0x85A9, "DIMM_E2 Uncorrectable ECC error" },
 { 0x85AA, "DIMM_F1 Uncorrectable ECC error" },
 { 0x85AB, "DIMM_F2 Uncorrectable ECC error" },
 { 0x8601, "BIOS Bank Override jumper set to lower bank" }, /*not used*/
 { 0x8602, "WatchDog timer expired (check secondary BIOS bank)" }, /*not used*/
 { 0x8603, "Secondary BIOS checksum fail" }, /*not used*/
 { 0x8604, "Chipset Reclaim of non critical variables complete" },
 { 0x9000, "Unspecified processor component error" },
 { 0x9223, "Keyboard was not detected" }, /*not used*/
 { 0x9226, "Keyboard controller error" },
 { 0x9243, "Mouse was not detected" },
 { 0x9246, "Mouse controller error" },
 { 0x9266, "Local Console controller error" },
 { 0x9268, "Local Console output error" },
 { 0x9269, "Local Console resource conflict error" },
 { 0x9286, "Remote Console controller error" },
 { 0x9287, "Remote Console input error" },
 { 0x9288, "Remote Console output error" },
 { 0x92A3, "Serial port was not detected" },
 { 0x92A9, "Serial port resource conflict error" },
 { 0x92C6, "Serial Port controller error" },
 { 0x92C7, "Serial Port input error" },
 { 0x92C8, "Serial Port output error" },
 { 0x94C6, "LPC controller error" },
 { 0x94C9, "LPC resource conflict error" },
 { 0x9506, "ATA/ATPI controller error" },
 { 0x95A6, "PCI controller error" },
 { 0x95A7, "PCI read error" },
 { 0x95A8, "PCI write error" },
 { 0x9609, "Unspecified software start error" },
 { 0x9641, "PEI Core load error" },
 { 0x9667, "PEI module Illegal software state error" },
 { 0x9687, "DXE core Illegal software state error" },
 { 0x96A7, "DXE driver Illegal software state error" },
 { 0x96AB, "DXE driver Invalid configuration" },
 { 0x96E7, "SMM driver Illegal software state error" },
 { 0xA000, "TPM device not detected" },
 { 0xA001, "TPM device missing" },
 { 0xA002, "TPM device failure" },
 { 0xA003, "TPM device failed self-test" },
 { 0xA022, "Processor mismatch error" },
 { 0xA027, "Processor low voltage error" },
 { 0xA028, "Processor high voltage error" },
 { 0xA421, "PCI SERR detected" },
 { 0xA500, "ATA/ATPI ATA bus SMART not supported" },
 { 0xA501, "ATA/ATPI ATA SMART is disabled" },
 { 0xA5A0, "PCI Express PERR" },
 { 0xA5A1, "PCI Express SERR" },
 { 0xA5A4, "PCI Express IBIST error" },
 { 0xA6A0, "DXE driver Not enough memory to shadow legacy OpROM" },
 { 0xB6A3, "DXE driver unrecognized" },
 { 0xffff , NULL }  /*end of list*/
};
const struct valstr intel_s2600_post[] = {  /*from S2600CP TPS*/
 { 0x0012, "CMOS date/time not set" },
 { 0x0048, "Password check failed" },
 { 0x0108, "Keyboard locked error" },
 { 0x0109, "Keyboard stuck key error" },
 { 0x0113, "The SAS RAID firmware cannot run properly" },
 { 0x0140, "PCI PERR detected" },
 { 0x0141, "PCI resource conflict" },
 { 0x0146, "PCI out of resources error" },
 { 0x0191, "Processor core/thread count mismatch" },
 { 0x0192, "Processor cache size mismatch" },
 { 0x0194, "Processor family mismatch" },
 { 0x0195, "Processor QPI speed mismatch" },
 { 0x0196, "Processor Model mismatch" },
 { 0x0197, "Processor speeds mismatched" },
 { 0x0198, "Processor family is unsupported" },
 { 0x019F, "Processor/chipset stepping configuration is unsupported" },
 { 0x5220, "CMOS/NVRAM Configuration Cleared" },
 { 0x5221, "Passwords cleared by jumper" },
 { 0x5224, "Password clear jumper is Set" },
 { 0x8110, "Proc1 internal error (IERR) on last boot" }, /*not used*/
 { 0x8111, "Proc2 internal error (IERR) on last boot" }, /*not used*/
 { 0x8120, "Proc1 thermal trip error on last boot" }, /*not used*/
 { 0x8121, "Proc2 thermal trip error on last boot" }, /*not used*/
 { 0x8130, "Proc1 disabled" }, /*not used*/
 { 0x8131, "Proc2 disabled" }, /*not used*/
 { 0x8140, "Proc1 Failed FRB-3 Timer" }, /*not used*/
 { 0x8141, "Proc2 Failed FRB-3 Timer" }, /*not used*/
 { 0x8160, "Proc1 unable to apply microcode update" },
 { 0x8161, "Proc2 unable to apply microcode update" },
 { 0x8170, "Proc1 failed Self Test (BIST)" },  /*not used*/
 { 0x8171, "Proc2 failed Self Test (BIST)" },  /*not used*/
 { 0x8180, "Processor microcode update not found" },
 { 0x8181, "Proc2 microcode update not found" },
 { 0x8190, "Watchdog timer failed on last boot" },
 { 0x8198, "OS boot watchdog timer expired on last boot" },
 { 0x8300, "iBMC failed self-test" },
 { 0x8305, "Hotswap controller failure" },
 { 0x84F2, "iBMC failed to respond" },
 { 0x84F3, "iBMC in update mode" },
 { 0x84F4, "Sensor data record empty" },
 { 0x84FF, "System event log full" },
 { 0x8500, "Memory could not be configured in the selected RAS mode" },
 { 0x8501, "DIMM Population Error" },
 { 0x8520, "DIMM_A1 failed test/initialization" },
 { 0x8521, "DIMM_A2 failed test/initialization" },
 { 0x8522, "DIMM_A3 failed test/initialization" },
 { 0x8523, "DIMM_B1 failed test/initialization" },
 { 0x8524, "DIMM_B2 failed test/initialization" },
 { 0x8525, "DIMM_B3 failed test/initialization" },
 { 0x8526, "DIMM_C1 failed test/initialization" },
 { 0x8527, "DIMM_C2 failed test/initialization" },
 { 0x8528, "DIMM_C3 failed test/initialization" },
 { 0x8529, "DIMM_D1 failed test/initialization" },
 { 0x852A, "DIMM_D2 failed test/initialization" },
 { 0x852B, "DIMM_D3 failed test/initialization" },
 { 0x852C, "DIMM_E1 failed test/initialization" },
 { 0x852D, "DIMM_E2 failed test/initialization" },
 { 0x852E, "DIMM_E3 failed test/initialization" },
 { 0x852F, "DIMM_F1 failed test/initialization" },
 { 0x8530, "DIMM_F2 failed test/initialization" },
 { 0x8531, "DIMM_F3 failed test/initialization" },
 { 0x8532, "DIMM_G1 failed test/initialization" },
 { 0x8533, "DIMM_G2 failed test/initialization" },
 { 0x8534, "DIMM_G3 failed test/initialization" },
 { 0x8535, "DIMM_H1 failed test/initialization" },
 { 0x8536, "DIMM_H2 failed test/initialization" },
 { 0x8537, "DIMM_H3 failed test/initialization" },
 { 0x8538, "DIMM_I1 failed test/initialization" },
 { 0x8539, "DIMM_I2 failed test/initialization" },
 { 0x853A, "DIMM_I3 failed test/initialization" },
 { 0x853B, "DIMM_J1 failed test/initialization" },
 { 0x853C, "DIMM_J2 failed test/initialization" },
 { 0x853D, "DIMM_J3 failed test/initialization" },
 { 0x853E, "DIMM_K1 failed test/initialization" },
 { 0x853F, "DIMM_K2 failed test/initialization" },
 { 0x8540, "DIMM_A1 Disabled" },
 { 0x8541, "DIMM_A2 Disabled" },
 { 0x8542, "DIMM_A3 Disabled" },
 { 0x8543, "DIMM_B1 Disabled" },
 { 0x8544, "DIMM_B2 Disabled" },
 { 0x8545, "DIMM_B3 Disabled" },
 { 0x8546, "DIMM_C1 Disabled" },
 { 0x8547, "DIMM_C2 Disabled" },
 { 0x8548, "DIMM_C3 Disabled" },
 { 0x8549, "DIMM_D1 Disabled" },
 { 0x854A, "DIMM_D2 Disabled" },
 { 0x854B, "DIMM_D3 Disabled" },
 { 0x854C, "DIMM_E1 Disabled" },
 { 0x854D, "DIMM_E2 Disabled" },
 { 0x854E, "DIMM_E3 Disabled" },
 { 0x854F, "DIMM_F1 Disabled" },
 { 0x8550, "DIMM_F2 Disabled" },
 { 0x8551, "DIMM_F3 Disabled" },
 { 0x8552, "DIMM_G1 Disabled" },
 { 0x8553, "DIMM_G2 Disabled" },
 { 0x8554, "DIMM_G3 Disabled" },
 { 0x8555, "DIMM_H1 Disabled" },
 { 0x8556, "DIMM_H1 Disabled" },
 { 0x8557, "DIMM_H1 Disabled" },
 { 0x8558, "DIMM_I1 Disabled" },
 { 0x8559, "DIMM_I2 Disabled" },
 { 0x855A, "DIMM_I3 Disabled" },
 { 0x855B, "DIMM_J1 Disabled" },
 { 0x855C, "DIMM_J2 Disabled" },
 { 0x855D, "DIMM_J3 Disabled" },
 { 0x855E, "DIMM_K1 Disabled" },
 { 0x855F, "DIMM_K2 Disabled" },
 { 0x8560, "DIMM_A1 SPD fail error" },
 { 0x8561, "DIMM_A2 SPD fail error" },
 { 0x8562, "DIMM_A3 SPD fail error" },
 { 0x8563, "DIMM_B1 SPD fail error" },
 { 0x8564, "DIMM_B2 SPD fail error" },
 { 0x8565, "DIMM_B3 SPD fail error" },
 { 0x8566, "DIMM_C1 SPD fail error" },
 { 0x8567, "DIMM_C2 SPD fail error" },
 { 0x8568, "DIMM_C3 SPD fail error" },
 { 0x8569, "DIMM_D1 SPD fail error" },
 { 0x856A, "DIMM_D2 SPD fail error" },
 { 0x856B, "DIMM_D3 SPD fail error" },
 { 0x856C, "DIMM_E1 SPD fail error" },
 { 0x856D, "DIMM_E2 SPD fail error" },
 { 0x856E, "DIMM_E3 SPD fail error" },
 { 0x856F, "DIMM_F1 SPD fail error" },
 { 0x8570, "DIMM_F2 SPD fail error" },
 { 0x8571, "DIMM_F3 SPD fail error" },
 { 0x8572, "DIMM_G1 SPD fail error" },
 { 0x8573, "DIMM_G2 SPD fail error" },
 { 0x8574, "DIMM_G3 SPD fail error" },
 { 0x8575, "DIMM_H1 SPD fail error" },
 { 0x8576, "DIMM_H1 SPD fail error" },
 { 0x8577, "DIMM_H1 SPD fail error" },
 { 0x8578, "DIMM_I1 SPD fail error" },
 { 0x8579, "DIMM_I2 SPD fail error" },
 { 0x857A, "DIMM_I3 SPD fail error" },
 { 0x857B, "DIMM_J1 SPD fail error" },
 { 0x857C, "DIMM_J2 SPD fail error" },
 { 0x857D, "DIMM_J3 SPD fail error" },
 { 0x857E, "DIMM_K1 SPD fail error" },
 { 0x857F, "DIMM_K2 SPD fail error" },
 /* some missing here for DIMM_K3 thru DIMM_P3 */
 { 0x8604, "Chipset Reclaim of non critical variables complete" },
 { 0x8605, "BIOS settings are corrupt" },
 { 0x92A3, "Serial port was not detected" },
 { 0x92A9, "Serial port resource conflict error" },
 { 0xA000, "TPM device not detected" },
 { 0xA001, "TPM device missing" },
 { 0xA002, "TPM device failure" },
 { 0xA003, "TPM device failed self-test" },
 { 0xA100, "BIOS ACM errord" },
 { 0xA421, "PCI SERR detected" },
 { 0xA5A0, "PCI Express PERR" },
 { 0xA5A1, "PCI Express SERR" },
 { 0xffff , NULL }  /*end of list*/
};

/* decode Intel POST codes for some platforms*/
int decode_post_intel(int prod, ushort code, char *outbuf,int szbuf)
{
   int rv = -1;
   const char *poststr = NULL;
   if (is_thurley(VENDOR_INTEL,prod))  /* S5520UR/T5520UR (CG2100 or NSN2U)*/
      poststr = val2str(code,intel_s5500_post);
   else if (is_romley(VENDOR_INTEL,prod))  /* S2600CO, CG2200 */
      poststr = val2str(code,intel_s2600_post);
   else switch(prod) {
      case 0x0028:   /*S5000PAL*/
      case 0x0029:   /*S5000PSL*/
      case 0x0811:   /*S5000PHB*/
	  poststr = val2str(code,intel_s5000_post);
	  break;
      default:   break;
   }
   if (poststr != NULL) {
      strncpy(outbuf, poststr, szbuf);
      rv = 0;
   }
   return(rv);
}

#define ENC_LED_SLEEP     50000
#define ENC_RCMD_SLEEP   500000

int get_hsbp_version_intel(uchar *maj, uchar *min)
{
   uchar idata[8];
   uchar rdata[4];
   int rv, rlen, i;
   uchar cc;

   *maj = 0;
   *min = 0;
   /* For Romley/CG2200 get HSBP FW Version */
   for (i = 0; i < 3; i++) { 
     idata[0] = 0x0A;   // 0x0A
     idata[1] = 0xD0;
     idata[2] = 0x01;   // return one byte of LED data
     idata[3] = 0x0A;   // HSBP Major version
     rlen = sizeof(rdata);
     rv = ipmi_cmd(MASTER_WRITE_READ, idata, 4, rdata, &rlen, &cc, fdebug);
     if (rv == 0 && cc != 0) rv = cc;
     os_usleep(0,ENC_LED_SLEEP);  /* wait before re-reading values */
     if (rv == 0) {
	*maj = rdata[0];
	break;
     }
     /*else retry reading it*/
   }
   for (i = 0; i < 3; i++) { 
     idata[0] = 0x0A;   // 0x0A
     idata[1] = 0xD0;
     idata[2] = 0x01;   // return one byte of LED data
     idata[3] = 0x0B;   // HSBP Minor version
     rlen = sizeof(rdata);
     rv = ipmi_cmd(MASTER_WRITE_READ, idata, 4, rdata, &rlen, &cc, fdebug);
     if (rv == 0 && cc != 0) rv = cc;
     os_usleep(0,ENC_LED_SLEEP);  /* wait before re-reading values */
     if (rv == 0) {
	*min = rdata[0];
	break;
     }
     /*else retry reading it*/
   }
   return (rv);
}

static uchar rdisk_led_method = 0;       /* 0=initial, 1=rcmd, 2=i2c */
static uchar rdisk_led_override = 0x00;  /*override is off by default*/

static int get_enc_leds_i2c(uchar *val)
{
   uchar idata[8];
   uchar rdata[4];
   int rv, rv2, rlen, i;
   uchar cc;

   *val = 0;  /*make sure to initialize the reading*/
   for (i = 0; i < 3; i++) { 
     /* For Romley/Patsburg get disk fault LED status */
     idata[0] = 0x0A;   // 0x0A
     idata[1] = 0xD0;
     idata[2] = 0x01;   // return one byte of LED data
     idata[3] = 0x0E;   // LED_Status
     rlen = sizeof(rdata);
     rv = ipmi_cmd(MASTER_WRITE_READ, idata, 4, rdata, &rlen, &cc, fdebug);
     if (rv == 0 && cc != 0) rv = cc;
     os_usleep(0,ENC_LED_SLEEP);  /* wait before re-reading values */
     if (rv == 0) {
	*val = rdata[0];
	break;
     }
     /*else retry reading the LED Status*/
   }

   /* get & save the override status for use in show() */
   idata[0] = 0x0A;   // 0x0A
   idata[1] = 0xD0;
   idata[2] = 0x01;   // return one byte of LED data
   idata[3] = 0x0D;   // LED_Override
   rlen = sizeof(rdata);
   rv2 = ipmi_cmd(MASTER_WRITE_READ, idata, 4, rdata, &rlen, &cc, fdebug);
   if (rv2 == 0 && cc != 0) rv2 = cc;
   if (rv2 == 0) rdisk_led_override = rdata[0];

   return (rv);
}

static int set_enc_override_i2c(uchar val)
{
   uchar idata[8];
   uchar rdata[4];
   int rv, rlen, i;
   uchar cc;

   if (val != 0) val = 1;
   for (i = 0; i < 3; i++) { 
      /* Set LED Override to 0=off or 1=on */
      idata[0] = 0x0A;   // 0x0A bus
      idata[1] = 0xD0;
      idata[2] = 0x00;   // return no data
      idata[3] = 0x0D;   // LED_Override
      idata[4] = val;   /* turn on override */
      rlen = sizeof(rdata);
      rv = ipmi_cmd(MASTER_WRITE_READ, idata, 5, rdata, &rlen, &cc, fdebug);
      if (rv == 0 && cc != 0) rv = cc;
      os_usleep(0,ENC_LED_SLEEP);  /* wait before next LED cmd */
      if (rv == 0) break;
   }
   return(rv);
}

static int set_enc_leds_i2c(uchar val)
{
   uchar idata[8];
   uchar rdata[4];
   int rv, rv2, rlen, i;
   uchar cc;

   if ((val != 0) && (rdisk_led_override == 0)) {  
      /* setting some LED, so set disk led override*/
      rv2 = set_enc_override_i2c(1);
   } 

   for (i = 0; i < 3; i++) { 
      /* For Romley/Patsburg set disk fault LED status */
      idata[0] = 0x0A;   // 0x0A bus
      idata[1] = 0xD0;
      idata[2] = 0x00;   // return one byte of LED data
      idata[3] = 0x0E;   // LED_Status
      idata[4] = val;
      rlen = sizeof(rdata);
      rv = ipmi_cmd(MASTER_WRITE_READ, idata, 5, rdata, &rlen, &cc, fdebug);
      if ((rv == 0) && (cc != 0)) rv = cc;
      os_usleep(0,ENC_LED_SLEEP);  /* wait before next LED cmd */
      if (rv == 0) break;
   }

   if (val == 0x00) {  /* clear the disk led override */
      rv2 = set_enc_override_i2c(0);
   }
   return (rv);  
}

void show_enc_leds_i2c(uchar val, int numd)
{
    char *enc_pattn = "disk slot %d LED:   %s %s\n";
    char *pstat;
    char *pover;
    uchar mask;
    int i;
    if (fdebug) printf("leds = %02x override = %02x\n",val,rdisk_led_override); 
    /* Some backplanes support 6 slots, but max is 8. */
    if (numd > 8) numd = 8;
    mask = 0x01;
    for (i = 0; i < numd; i++) {
       if (val & mask) pstat = "ON";
       else pstat = "off";
       if (rdisk_led_override) pover = "(override)";
       else pover = "";
       printf(enc_pattn,i,pstat,pover);
       mask = (mask << 1);
    }
}

static int set_enc_override_rcmd(uchar val)
{
   rdisk_led_override = val;
   return(0);
}

static int get_enc_leds_rcmd(uchar *val)
{
   uchar rdata[8];
   uchar slot[12]; 
   int rv, rlen, i;
   uchar cc;
   uchar v = 0;

   /* This command is only supported on Intel Romley w BMC >= 1.10 */
   for (i = 0; i < 12; i++) slot[i] = 0;
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(0x65, 0x30, g_sa,g_bus,g_lun,
                        NULL, 0, rdata, &rlen, &cc, fdebug);
   if (fdebug) printf("get_enc_leds_rcmd: rv = %d, cc=%02x\n", rv,cc);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (rv == 0) {
       rdisk_led_override = (rdata[0] & 0x07);
/*
rdata[1]: Slot 1 to 4 status
  [7:6]Slot4 status
     00-Off
     01-Locate (Amber 4HZ)
     10-Fail (Amber solid on)
     11-Rebuild (Amber 1HZ)
  [5:4] Slot3 status
     00-Off
     01-Locate (Amber 4HZ)
     10-Fail (Amber solid on)
     11-Rebuild (Amber 1HZ)
  [3:2] Slot2 status
     00-Off
     01-Locate (Amber 4HZ)
     10-Fail (Amber solid on)
     11-Rebuild (Amber 1HZ)
  [1:0] Slot1 status
     00-Off
     01-Locate (Amber 4HZ)
     10-Fail (Amber solid on)
     11-Rebuild (Amber 1HZ)
 */
       if (rdata[1] & 0x02) slot[0] = 1;
       if (rdata[1] & 0x08) slot[1] = 1;
       if (rdata[1] & 0x20) slot[2] = 1;
       if (rdata[1] & 0x80) slot[3] = 1;
       if (rdata[2] & 0x02) slot[4] = 1;
       if (rdata[2] & 0x08) slot[5] = 1;
       if (rdata[2] & 0x20) slot[6] = 1;
       if (rdata[2] & 0x80) slot[7] = 1;
       if (rdata[3] & 0x02) slot[8] = 1;
       if (rdata[3] & 0x08) slot[9] = 1;
       if (rdata[3] & 0x20) slot[10] = 1;
       if (rdata[3] & 0x80) slot[11] = 1;
   }
   for (i = 0; i < 8; i++) {
      if (slot[i] == 1) v |= (1 << i);
   }
   *val = v;
   return(rv);
}

static int set_enc_leds_rcmd(uchar val)
{
   uchar idata[8];
   uchar rdata[4];
   int rlen, rv;
   uchar cc;

   if ((val != 0) && (rdisk_led_override == 0)) 
        set_enc_override_rcmd(1);

   /* This command is only supported on Intel Romley w BMC >= 1.10 */
   idata[0] = rdisk_led_override;
   idata[1] = val;  /*first 8 slots*/
   idata[2] = 0;
   idata[3] = 0;
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(0x64, 0x30, g_sa,g_bus,g_lun,
                        idata, 4, rdata, &rlen, &cc, fdebug);
   if (fdebug) printf("set_enc_leds_rcmd: rv = %d, cc=%02x\n", rv,cc);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (rv == 0) os_usleep(0,ENC_RCMD_SLEEP);

   if ((rv == 0) && (val == 0)) {  /*repeat with override off*/
      os_usleep(1,0);  /* wait before next LED cmd */
      idata[0] = 0;
      idata[1] = val;
      idata[2] = 0;
      idata[3] = 0;
      rlen = sizeof(rdata);
      rv = ipmi_cmdraw(0x64, 0x30, g_sa,g_bus,g_lun,
                        idata, 4, rdata, &rlen, &cc, fdebug);
      if (fdebug) printf("set_enc_leds_rcmd0: rv = %d, cc=%02x\n", rv,cc);
      if ((rv == 0) && (cc != 0)) rv = cc;
      if (rv == 0) os_usleep(0,ENC_RCMD_SLEEP);
      set_enc_override_rcmd(0);
   }
   os_usleep(1,0);  /* wait before next LED cmd */
   return(rv);
}

static void show_enc_leds_rcmd(uchar val, int numd)
{
    char *enc_pattn = "disk slot %d LED:   %s %s\n";
    char *pstat;
    char *pover;
    uchar mask;
    int i;
    if (fdebug) printf("leds = %02x override = %02x\n",val,rdisk_led_override); 
    /* Some backplanes support 6 slots, but max is 8. */
    if (numd > 8) numd = 8;
    mask = 0x01;
    for (i = 0; i < numd; i++) {
       if (val & mask) pstat = "ON";
       else pstat = "off";
       if (rdisk_led_override) pover = "(override)";
       else pover = "";
       printf(enc_pattn,i,pstat,pover);
       mask = (mask << 1);
    }
}

int set_enc_override_intel(uchar val)
{
    int rv;
    if (rdisk_led_method == 1) rv = set_enc_override_rcmd(val);
    else rv = set_enc_override_i2c(val);
    return(rv);
}

int get_enc_leds_intel(uchar *val)
{
   int rv = -1;
   if (rdisk_led_method < 2) rv = get_enc_leds_rcmd(val);
   if ((rv != 0) && (rdisk_led_method == 0)) rdisk_led_method = 2;
   if (rdisk_led_method == 2) rv = get_enc_leds_i2c(val);
   return(rv);
}

int set_enc_leds_intel(uchar val)
{
   int rv = -1;
   if (rdisk_led_method < 2) rv = set_enc_leds_rcmd(val);
   if ((rv != 0) && (rdisk_led_method == 0)) rdisk_led_method = 2;
   if (rdisk_led_method == 2) rv = set_enc_leds_i2c(val);
   return(rv);
}

void show_enc_leds_intel(uchar val, int numd)
{
   if (rdisk_led_method == 1) show_enc_leds_rcmd(val, numd);
   else show_enc_leds_i2c(val, numd);
}

#ifdef METACOMMAND
int i_inteloem(int argc, char **argv)
{
   printf("%s ver %s\n", progname,progver);
   return(0);
}
#endif
/* end oem_intel.c */
