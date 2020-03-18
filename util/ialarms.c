/*
 * ialarms.c
 *
 * This tool reads and sets the alarms panel on an Intel Telco chassis.
 * Note that the Intel Server Management software will set these alarms
 * based on firmware-detected thresholds and events.
 * 
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2003-2006 Intel Corporation. 
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 02/25/03 Andy Cress - created
 * 04/08/03 Andy Cress - added -i for ChassisIdentify
 * 04/30/03 Andy Cress - only try to set ID on/off if option specified
 * 01/20/04 Andy Cress - mods for mBMC w Chesnee platform
 * 05/05/04 Andy Cress - call ipmi_close before exit
 * 10/11/04 Andy Cress 1.4 - if -o set relays too (fsetall)
 * 11/01/04 Andy Cress 1.5 - add -N / -R for remote nodes
 * 03/07/05 Andy Cress 1.6 - add bus for Intel TIGI2U
 * 03/28/05 Andy Cress 1.7 - add check for BMC TAM if setting alarms
 * 06/22/05 Andy Cress 1.8 - adding fpicmg for ATCA alarm LEDs
 * 03/17/06 Andy Cress 1.9 - adding BUS_ID7 for Harbison 
 * 04/20/07 Andy Cress 1.25 - adding disk Enclosure HSC LEDs
 */
/*M*
Copyright (c) 2003-2006, Intel Corporation
Copyright (c) 2009 Kontron America, Inc.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#ifdef WIN32
#include "getopt.h"
#elif defined(DOS)
#include <dos.h>
#include "getopt.h"
#elif defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#else
#include <getopt.h>
#endif
#include "ipmicmd.h"
#include "oem_intel.h"
 
/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil alarms";
#else
static char * progver   = "3.08";
static char * progname  = "ialarms";
#endif
static char   fdebug    = 0;
static char   fbmctam   = 0;
static char   fpicmg    = 0;
static char   fHasAlarms = 0;
static char   fNSC       = 0;
static char   fHasEnc    = 0;     /* Has disk Enclosure HSC? */
static int    maxdisks   = 6;     /* default to max of 6 disks */
static uchar  fdoencl    = 1;
static uchar picmg_id    = 0;     /* always 0 for picmg */
static uchar fru_id      = 0;     /* fru device id */
//static uchar led_id      = 0;   /* 0 = blue led, 1,2,3=led1,2,3 */
static uchar ipmi_maj, ipmi_min;

#define ENC_LED_WRITE       0x21 // only used for Ballenger-CT HSC
#define ENC_LED_READ        0x20 // only used for Ballenger-CT HSC

#define NETFN_ENC                 0x30
#define NETFN_PICMG               0x2c
#define PICMG_GET_LED_PROPERTIES  0x05
#define PICMG_SET_LED_STATE       0x07
#define PICMG_GET_LED_STATE       0x08

#ifdef METACOMMAND
extern int get_alarms_fujitsu(uchar *rgalarms);
extern int show_alarms_fujitsu(uchar *rgalarms);
extern int set_alarms_fujitsu(uchar num, uchar val);
extern int get_led_status_intel(uchar *pstate);
#endif
#ifdef ALONE
extern int get_led_status_intel(uchar *pstate);
#endif

static uchar busid = PRIVATE_BUS_ID;
static uchar enc_sa = HSC_SA;
static char fRomley = 0;
static char fGrantley = 0;

static int get_enc_leds(uchar *val)
{
    uchar idata[16];
    uchar rdata[16];
    int rlen, i;
    int rv = 0;
    uchar cc;
    char *pstr = NULL;
    rlen = sizeof(rdata);
    for (i = 0; i < 3; i++) {
	rv = ipmi_cmdraw( ENC_LED_READ, NETFN_ENC, enc_sa, PUBLIC_BUS,BMC_LUN,
	                 idata,0, rdata, &rlen, &cc, fdebug);
	if (fdebug) 
		printf("get_enc_leds() rv=%d cc=%x val=%02x\n",rv,cc,rdata[0]);
	if (rv == 0 && cc == 0x83) os_usleep(0,50000); /* HSC busy, wait 50ms */
	else break;
    }
    if (rv != 0) pstr = decode_rv(rv);
    else if (cc != 0) pstr = decode_cc(0,cc);
    /* if get cc==0x83 here, the power state may be soft-off */
    if (rv == 0 && cc != 0) rv = cc;
    if (rv == 0) { /*success*/
       *val = rdata[0];
    } else /*error*/ 
	printf("get_enc_leds: error %s\n",pstr);
    return(rv);
}

static int set_enc_leds(uchar val)
{
    uchar idata[16];
    uchar rdata[16];
    int rlen;
    int rv = 0;
    uchar cc;

    idata[0] = val;
    rlen = sizeof(rdata);
    rv = ipmi_cmdraw( ENC_LED_WRITE, NETFN_ENC, enc_sa, PUBLIC_BUS,BMC_LUN,
                      idata,1, rdata, &rlen, &cc, fdebug);
    if (fdebug) printf("set_enc_leds(%02x) rv = %d, cc = %x\n",val,rv,cc);
    if (rv == 0 && cc != 0) rv = cc;
    return(rv);
}

static void show_enc_leds(uchar val)
{
    char *enc_pattn = "disk slot %d LED:   %s\n";
    uchar mask;
    int i, n;
    if (fdebug) printf("val = %02x\n",val);
    n = maxdisks;
    if (n > 8) n = 8;
    mask = 0x01;
    /* Ballenger HSC only supports 6 slots, some support 8 */
    for (i = 0; i < n; i++) {    
       if (val & mask) printf(enc_pattn,i,"ON");
       else printf(enc_pattn,i,"off");
       mask = (mask << 1);
    }
}

static int set_chassis_id(uchar val)
{
	uchar inputData[4];
	uchar responseData[16];
	int responseLength = 4;
	uchar completionCode;
	int ret;
        uchar ilen;

        ilen = 1;
	inputData[0] = val;   /* #seconds to turn on id, 0=turn off */
        /* IPMI 2.0 has an optional 2nd byte, 
         *       if 01, turn ID on indefinitely.  */
        if (val == 255 && ipmi_maj >= 2) {
           inputData[1] = 1;
           ilen = 2;
        }
        /* CHASSIS_IDENTIFY=0x04, using NETFN_CHAS (=00) */
        ret = ipmi_cmd(CHASSIS_IDENTIFY, inputData, ilen, responseData,
                        &responseLength, &completionCode, fdebug);
	if (ret == 0 && completionCode != 0) ret = completionCode;
	if (ret != 0) {
           printf("set_chassis_id: ret = %d, ccode %02x, value = %02x\n",
                    ret, completionCode, val);
	   }
	return(ret);
} /*end set_chassis_id*/


static int get_alarms_picmg(uchar *rgv, uchar picmgid, uchar fruid, uchar led)
{
	uchar inputData[4];
	uchar responseData[16];
	int responseLength;
	uchar completionCode;
	int ret, i;

	if (rgv == NULL) return(ERR_BAD_PARAM);
	inputData[0] = picmgid;
	inputData[1] = fruid;
	inputData[2] = led;   // 0 = blue led
	responseLength = sizeof(responseData);
        ret = ipmi_cmdraw( PICMG_GET_LED_STATE, NETFN_PICMG,
			BMC_SA, PUBLIC_BUS,BMC_LUN,
                        inputData,3, responseData, &responseLength, 
			&completionCode, fdebug);
	if ((ret != 0) || (completionCode != 0)) { 
	   if (fdebug)
              printf("get_alarms_picmg(%d,%d,%d): ret = %d, ccode %02x\n",
                    picmgid,fruid,led,ret, completionCode);
	   if (ret == 0) ret = completionCode;
	   return(ret);
	}
	/* if here, success */
	if (fdebug) {
	   printf("get_alarms_picmg(%d,%d,%d): ", picmgid,fruid,led);
           for (i = 0; i < responseLength; i++)
		printf("%02x ",responseData[i]);
           printf("\n");
	}
	memcpy(rgv,responseData,responseLength);
	return(ret);
}

static int set_alarms_picmg(uchar val, uchar picmgid, uchar fruid, uchar led,
				char color)
{
	uchar inputData[6];
	uchar responseData[16];
	int responseLength;
	uchar completionCode;
	int ret, i;

	inputData[0] = picmgid;
	inputData[1] = fruid;
	inputData[2] = led;   // 0 = blue led
	inputData[3] = val; 
	inputData[4] = 0;  
	switch(color) {
	   case 'w': i = 6; break;
	   case 'o': i = 5; break;
	   case 'a': i = 4; break;
	   case 'g': i = 3; break;
	   case 'r': i = 2; break;
	   case 'b': 
	   default:  i = 1; break;
	}
	inputData[5] = (uchar)i;   // 1 = blue 
	responseLength = sizeof(responseData);
        ret = ipmi_cmdraw( PICMG_SET_LED_STATE, NETFN_PICMG,
			BMC_SA, PUBLIC_BUS,BMC_LUN,
                        inputData,6, responseData, &responseLength, 
			&completionCode, fdebug);
	if ((ret != 0) || (completionCode != 0)) { 
           printf("set_alarms_picmg(%02x,%d,%d,%d): ret = %d, ccode %02x\n",
                    val,picmgid,fruid,led,ret, completionCode);
	   if (ret == 0) ret = completionCode;
	   return(ret);
	}
        printf("set_alarms_picmg(%02x,%d,%d,%d): ", val,picmgid,fruid,led);
        for (i = 0; i < responseLength; i++)
		printf("%02x ",responseData[i]);
        printf("\n");
	return(ret);  /* success */
}

static void show_alarms_picmg(uchar *v, uchar pid, uchar fruid, uchar led)
{
    char led_str[10]; 
    char state_str[20]; 
    char *func_str;
    char *color_str;
    if (v == NULL) return;
    if (fdebug) 
	printf("picmg(%d,%d,%d) alarm LED state is %02x %02x %02x %02x %02x\n",
		pid,fruid,led,v[0],v[1],v[2],v[3],v[4]);
    switch(led) {
	case 0: strcpy(led_str,"HSLed"); break;  /*Blue LED*/
	default: sprintf(led_str," Led%d",led); break;
    }
    state_str[0] = 0;
    if (v[1] & 0x01) strcat(state_str,"local");
    if (v[1] & 0x02) strcat(state_str," override");
    if (v[1] & 0x04) strcat(state_str," lamptest");
    switch(v[2]) {
	case 0x00: func_str = "off"; break;
	case 0xFF: func_str = "ON"; break;
	default:   func_str = "Blink"; break;  /*duration of blink/off*/
    }
    /* v[3] is duration of blink/on in tens of msec */
    switch(v[4]) {
	case 6: color_str = "white"; break;
	case 5: color_str = "orange"; break;
	case 4: color_str = "amber"; break;
	case 3: color_str = "green"; break;
	case 2: color_str = "red"; break;
	case 1:
	default:  color_str = "blue"; break;
    }
    printf("picmg(%d,%d) %s is %s,%s,%s\n",fruid,led,led_str,
			state_str,func_str,color_str);
}


#ifdef METACOMMAND
int i_alarms(int argc, char **argv)
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
   uchar fsetled = 0;
   uchar fsetall = 0;
   uchar fsetdisk = 0;
   uchar fsetid = 0;
   uchar fcrit = 2;
   uchar fmaj = 2;
   uchar fmin = 2;
   uchar fpow = 2;
   uchar fdiska = 2;
   uchar fdiskb = 2;
   uchar fdiskn = 2;
   uchar ndisk  = 0;
   int   fid = 0;
   uchar alarms = 0;
   uchar rgalarms[10] = {0,0,0,0,0,0};
   uchar newvalue = 0xff;
   uchar diskled = 0xff;
   uchar encled = 0;
   uchar devrec[16];
   uchar ledn = 0;
   uchar ledv = 0;
   char ledc = 'c';
   int i;
   int flags = 0;
   int prod_id, vend_id;

   printf("%s ver %s\n", progname,progver);
   /* default to admin privilege if get/set alarms remotely */
   parse_lan_options('V',"4",0);

   while ( (c = getopt( argc, argv,"rxa:b:c:d:efm:n:p:i:ow:Z:EF:P:N:R:U:T:V:J:Y?")) != EOF ) 
      switch(c) {
          case 'r': fsetled=0; fsetid=0; fsetdisk = 0; break; /* read only */
          case 'a': fdiska = atob(optarg); /* set disk A LED value */
		    fsetdisk = 1; break; 
          case 'b': fdiskb = atob(optarg); /* set disk B LED value */
		    fsetdisk = 2; break; 
          case 'c': fcrit = atob(optarg);  /* set critical alarm value */
		    fsetled = 1; break; 
          case 'd': ndisk  = optarg[0] & 0x0f;  /* set disk N LED on or off */
                    fdiskn = optarg[1] & 0x0f;
		    fsetdisk = 3; break; 
          case 'e': fdoencl = 0;          /* skip disk/enclosure LEDs */
          case 'f': fdiska  = 10;         /* set all disk LEDs off */
		    fsetdisk = 1; break; 
          case 'm': fmaj = atob(optarg);  /* set major alarm value */
		    fsetled = 1; break; 
          case 'n': fmin = atob(optarg);  /* set minor alarm value */
		    fsetled = 1; break; 
          case 'p': fpow = atob(optarg);  /* set power alarm value */
		    fsetled = 1; break; 
          case 'w': ledn = optarg[0] & 0x0f; /* set picmg LED N on or off */
                    ledv = optarg[1] & 0x0f;
                    ledc = optarg[2];   /*color char, usu 'b' for blue*/
		    fsetled = 1; break; 
          case 'i': fid = atoi(optarg);   /* set chassis id on/off */
		    if (fid > 255) {
			printf("Adjusting %d to max 255 sec for ID\n",fid);
			fid = 255;
		    }
		    fsetid=1; break; 
          case 'o': fcrit=0; fmaj=0; fmin=0; fpow=0; /* set all alarms off */
		    fsetdisk = 1; fdiska = 0; fdiskb = 0; fsetall=1;
		    fsetled = 1; fsetid=1; fid=0; break; 
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
                printf("Usage: %s [-abcdfimnoprx -NUPRETVFY]\n", progname);
                printf(" where -r   means read-only\n");
                printf("       -i5  sets Chassis ID on for 5 sec\n");
                printf("       -i0  sets Chassis ID off\n");
                printf("       -a1  sets Disk A Fault on\n");
                printf("       -a0  sets Disk A Fault off\n");
                printf("       -b1  sets Disk B Fault on\n");
                printf("       -b0  sets Disk B Fault off\n");
                printf("       -c1  sets Critical Alarm on\n");
                printf("       -c0  sets Critical Alarm off\n");
                printf("       -d31 sets Disk 3 Fault on (disks 0-6)\n");
                printf("       -d30 sets Disk 3 Fault off\n");
                printf("       -e   skip disk Enclosure LEDs\n");
                printf("       -f   sets all Disk Fault LEDs off\n");
                printf("       -m1  sets Major Alarm on\n");
                printf("       -m0  sets Major Alarm off\n");
                printf("       -n1  sets Minor Alarm on\n");
                printf("       -n0  sets Minor Alarm off\n");
                printf("       -p1  sets Power Alarm on\n");
                printf("       -p0  sets Power Alarm off\n");
                printf("       -o   sets all Alarms off\n");
                printf("       -w21b writes picmg LED 2 on(1) color=blue(b)\n");
                printf("       -x   show eXtra debug messages\n");
		print_lan_opt_usage(0);
		ret = ERR_USAGE;
        	goto do_exit;
      }

   /* 
    * Check the Device ID to determine which bus id to use.
    */
   ret = ipmi_getdeviceid(devrec,16,fdebug);
   if (ret != 0) {
        goto do_exit;
   } else {
      int j;

      if (fdebug) {
	printf("devid: ");
	for (j = 0; j < 16; j++) printf("%02x ",devrec[j]);
	printf("\n");
      }
      ipmi_maj = devrec[4] & 0x0f;
      ipmi_min = devrec[4] >> 4;
      prod_id = devrec[9] + (devrec[10] << 8);
      vend_id = devrec[6] + (devrec[7] << 8) + (devrec[8] << 16);

      printf("-- %s version %x.%x, IPMI version %d.%d \n",
             "BMC", devrec[2],  devrec[3], ipmi_maj, ipmi_min);
   }

#ifdef TEST_ENC
	fHasEnc = 1;
	fHasAlarms = 0;
	fbmctam = 0;
	fpicmg = 0;
	fRomley = 1;
	maxdisks = 8; 
#else
   ret = ipmi_getpicmg(devrec,16,fdebug);
   if (ret == 0) {
	fpicmg = 1;
	fHasAlarms = 1;
   } else ret = 0;  /* ignore error if not picmg */

#if defined(METACOMMAND) || defined(ALONE)
   {
     uchar rgalarms[3];
     uchar idstate;
     char *pmsg;
     switch(vend_id) {
     case VENDOR_NSC:    /* for Intel TIGPT1U */
     case VENDOR_INTEL:  /* requires oem_intel.c */
	/* check_prod_capab */
	ret = detect_capab_intel(vend_id,prod_id,&flags,&maxdisks,fdebug); 
	busid = (uchar)ret;
	if (fdebug) printf("prod_capab: busid=%x, flags=%02x\n",busid,flags);
	if ((flags & HAS_ALARMS_MASK) != 0)   fHasAlarms = 1;
	if ((flags & HAS_BMCTAM_MASK) != 0)   fbmctam = 1;
	if ((flags & HAS_ENCL_MASK) != 0)     fHasEnc = 1;
	if ((flags & HAS_PICMG_MASK) != 0)    fpicmg = 1;
	if ((flags & HAS_NSC_MASK) != 0)      fNSC = 1;
	if ((flags & HAS_ROMLEY_MASK) != 0)   fRomley = 1;
	if ((flags & HAS_GRANTLEY_MASK) != 0) fGrantley = 1;
	/* get ID LED status */
	ret = get_led_status_intel(&idstate);
        if (ret == 0) {
	   switch(idstate) {
	   case 1:  pmsg = "ON"; break;
	   case 2:  pmsg = "Blink"; break;
	   default: pmsg = "off"; break;
	   }
	   printf("ID LED:       %s\n",pmsg);
	}
	break;
#ifdef METACOMMAND
     case VENDOR_KONTRON:
	if (prod_id == 1590) { fHasEnc = 1;  maxdisks = 8; }
	break;
     case VENDOR_FUJITSU:
	printf("Getting Fujitsu alarm LEDs ...\n"); 
	ret = get_alarms_fujitsu(rgalarms);
        if (fdebug) printf("get_alarms_fujitsu ret = %d\n",ret);
	/* if ret != 0, fall through and try default methods */
	if (ret == 0) {
	   show_alarms_fujitsu(rgalarms);
           if (fsetid) {
              printf("\nSetting fujitsu ID LED to %02x ...\n", fid);
              ret = set_alarms_fujitsu(0, (uchar)fid);
              printf("set_alarms_fujitsu ret = %d\n",ret);
	      ret = get_alarms_fujitsu(rgalarms);
	      if (ret == 0) show_alarms_fujitsu(rgalarms);
	   }
	   fHasAlarms = 0;  /*skip the other LED functions*/
	}
	break;
     case VENDOR_SUN:
	printf("Do get_alarms_sun() \n"); // TODO: add this
/*
	ret = get_alarms_sun();
	if (ret == 0) show_alarms_sun()
        if (fsetled) {
          printf("\nSetting sun(%d) alarm LED to %02x %c...\n",
			ledn,ledv,ledc);
          ret = set_alarms_sun(ledv,ledn,ledc);
          printf("set_alarms_sun ret = %d\n",ret);
	  ret = get_alarms_sun(rgalarms,ledn);
	  if (ret == 0) show_alarms_sun(rgalarms,ledn);
	}
*/
	break;
#endif
     default:
	break;
     }
   }
#endif

#endif
   if (fHasAlarms) 
   {  /* get the telco picmg or intel alarm LED states */
     if (fpicmg) {
       for (i = 0; i < 5; i++) {
  	ret = get_alarms_picmg(rgalarms,picmg_id,fru_id,(uchar)i);
  	if (ret == 0)
  	   show_alarms_picmg(rgalarms,picmg_id,fru_id,(uchar)i);
       }
     } else {
       alarms = get_alarms_intel(busid);
       if (alarms == 0) {  /* failed to get alarm panel data */
  	if (fHasAlarms) {  /* baseboard which may have Telco alarm panel*/
  	   printf("Could not obtain Telco LED states, Telco alarm panel "
  		"may not be present.\n\n");
          }
          fHasAlarms = 0;
       } else {   /* have Telco alarm panel data */
        ret = 0;
  	show_alarms_intel(alarms);
       }
     }

     if (fsetled) {
       if (fpicmg) {
         printf("\nSetting picmg(%d,%d) alarm LED to %02x %c...\n",
			fru_id,ledn,ledv,ledc);
         ret = set_alarms_picmg(ledv,picmg_id,fru_id,ledn,ledc);
         printf("set_alarms_picmg ret = %d\n",ret);
	 ret = get_alarms_picmg(rgalarms,picmg_id,fru_id,ledn);
	 if (ret == 0) show_alarms_picmg(rgalarms,picmg_id,fru_id,ledn);
       } else {   /* not picmg, set Intel Telco Alarm LEDs */
         if (fbmctam) {    /* Platform supports BMC Telco Alarms Manager */
	   ret = check_bmctam_intel();
         } /*endif fbmctam*/
	 if (ret == LAN_ERR_ABORT) {
	    printf("Conflict with BMC TAM - Skipping TAM LEDs.\n");
	 } else {
          if (fsetall) newvalue = 0xFF;   /* alarms and relays */
          else {
            newvalue = alarms;
            if      (fcrit == 1) newvalue &= 0xFD; /*bit1 = 0*/
            else if (fcrit == 0) newvalue |= 0xF2; 
            if      (fmaj == 1)  newvalue &= 0xFB; /*bit2 = 0*/
            else if (fmaj == 0)  newvalue |= 0xF4; 
            if      (fmin == 1)  newvalue &= 0xF7; /*bit3 = 0*/
            else if (fmin == 0)  newvalue |= 0xF8; 
            if      (fpow == 1)  newvalue &= 0xFE; /*bit0 = 0*/
            else if (fpow == 0)  newvalue |= 0xF1; 
          }
          printf("\nSetting alarms to %02x ...\n",newvalue);
          ret = set_alarms_intel(newvalue,busid);
          alarms = get_alarms_intel(busid);
          show_alarms_intel(alarms);
	 }
       }  /*end else Intel*/
     } /*endif fsetled*/
   }  /*endif fHasAlarms*/

   if (fsetid) {
	printf("Setting ID LED to %d ...\n\n",fid);
	ret = set_chassis_id((uchar)fid);
   }

   if (fHasEnc && fdoencl) {  /* disk enclosure exists */
     if (fRomley || fGrantley) {   /* Romley (Patsburg) */
       int rv;  /*do not change ret*/
       rv = get_enc_leds_intel(&encled);
       if (rv == 0) {
         show_enc_leds_intel(encled,maxdisks);
         if (fsetdisk) {
	    /* Set fault if user param, and disk is present. */
            if (fsetall) newvalue = 0x00;   /* all LEDs off */
	    else if (fdiska == 10) newvalue = 0x00;
            else {
               newvalue = encled;
	       if (fdiskb == 1)  newvalue |= 0x02;
	       else if (fdiskb == 0) newvalue &= 0xFD;
	       if (fdiska == 1)  newvalue |= 0x01; 
	       else if (fdiska == 0) newvalue &= 0xFE;
               if (fdiskn == 1) newvalue |= (0x01 << ndisk);
               else if (fdiskn == 0) newvalue &= ~(0x01 << ndisk);
            }
	    printf("\nSetting Enclosure LEDs to %02x ...\n",newvalue);
	    ret = set_enc_leds_intel(newvalue);
            ret = get_enc_leds_intel(&encled);
	    show_enc_leds_intel(encled,maxdisks);
	 }
       }
     } else { /* Vitesse disk Enclosure chipset */
       ret = get_enc_leds(&encled);
       if (ret == 0) {
	 show_enc_leds(encled);
         if (fsetdisk) {
	    /* Set fault if user param, and disk is present. */
            if (fsetall) newvalue = 0x00;   /* all LEDs off */
	    else if (fdiska == 10) newvalue = 0x00;
            else {
               newvalue = encled;
	       if (fdiskb == 1)  newvalue |= 0x02;
	       else if (fdiskb == 0) newvalue &= 0xFD;
	       if (fdiska == 1)  newvalue |= 0x01; 
	       else if (fdiska == 0) newvalue &= 0xFE;
               if (fdiskn == 1) newvalue |= (0x01 << ndisk);
               else if (fdiskn == 0) newvalue &= ~(0x01 << ndisk);
            }
	    printf("\nSetting Enclosure LEDs to %02x ...\n",newvalue);
	    ret = set_enc_leds(newvalue);
            ret = get_enc_leds(&encled);
	    show_enc_leds(encled);
	 }
       }
     } /*end-else Vitesse*/
   } /*endif fHasEnc*/
   else if (fNSC && fdoencl) { /*Chesnee NSC platform has special disk LEDs*/
        diskled = get_nsc_diskleds(busid);
	show_nsc_diskleds(diskled);
	if (fsetdisk) {
	   newvalue = diskled;
	   // newvalue |= 0xFC;  /*leave upper bits high (off) */
	   /* Set fault if user param, and disk is present. */
	   if (fdiskb == 1)  newvalue &= 0xFE;               /*bit0=0*/
	   else if (fdiskb == 0) newvalue |= 0x01;
	   if (fdiska == 1) newvalue &= 0xFD;               /*bit1=0*/
	   else if (fdiska == 0) newvalue |= 0x02;
	   else if (fdiska == 10) newvalue = 0x00;
	   printf("\nSetting Disk LEDs to %02x ...\n",newvalue);
	   ret = set_nsc_diskleds(newvalue,busid);
           diskled = get_nsc_diskleds(busid);
	   show_nsc_diskleds(diskled);
	}
   }
do_exit:
   ipmi_close_();
   // show_outcome(progname,ret);
   return (ret);
}  /* end main()*/

/* end ialarms.c */
