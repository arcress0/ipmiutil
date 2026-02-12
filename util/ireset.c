/*
 * ireset.c (was hwreset.c)
 *
 * This tool power cycles (or powers off) the IPMI system.
 *    
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2009 Kontron America, Inc.
 * Copyright (c) 2002-2008 Intel Corporation
 *
 * 01/08/02 Andy Cress - created
 * 01/31/02 Andy Cress - converted to use ipmi_cmd_ia, 
 *                       added more user options
 * 02/06/02 Andy Cress - added ipmi_cmd_va
 * 02/22/02 Andy Cress - added -s to reboot to service partition
 * 07/02/02 Andy Cress v1.3 added more Usage text
 * 08/02/02 Andy Cress v1.4 moved common ipmi_cmd() code to ipmicmd.c
 * 09/24/02 Andy Cress - stubbed in OS shutdown option
 * 01/29/03 Andy Cress v1.5 added MV OpenIPMI support
 * 04/08/03 Andy Cress v1.6 added OS shutdown option (-o)
 * 05/02/03 Andy Cress v1.7 leave console redir alone in SET_BOOT_OPTIONS
 * 05/05/04 Andy Cress v1.8 call ipmi_close before exit, did WIN32 port.
 * 08/09/04 Andy Cress v1.9 make sure to show error if ccode != 0, and
 *                          detect Langley platforms to do special 
 *                          watchdog method for -o option.
 * 11/01/04 Andy Cress 1.10 add -N / -R for remote nodes   
 * 11/16/04 Andy Cress 1.11 add -U for remote username
 * 11/30/04 Andy Cress 1.12 fix bug 1075550 with -o -N, skip -o if not local.
 * 03/28/05 Andy Cress 1.13 add netapp_reset commands for platforms that
 *                          use this instead of chassis_reset.
 * 05/16/05 Andy Cress 1.14 add -u option for power up
 * 03/31/06 Andy Cress 1.15 add -e -p -m options
 * 09/18/06 Andy Cress 1.20 allow more platforms to do soft reset, and
 *                          if Tyan, ignore set boot options errors.
 * 01/10/07 Andy Cress 1.25 added reset_str(), modify initchar (6) if -o.
 * 01/31/26 Andy Cress 2.21 SR62:interpret -e as modifier, not target (SMagnani)
 */
/*M*
Copyright (c) 2009 Kontron America, Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include "getopt.h"
#elif defined(EFI)
 // EFI: defined (EFI32) || defined (EFI64) || defined(EFIX64)
 // also would define ALONE (not METACOMMAND) to minimize externals
 #ifndef NULL
    #define NULL    0
 #endif
 #include <types.h>
 #include <libdbg.h>
 #include <unistd.h>
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
#include "oem_intel.h"  /* for is_romley*/
 
#define platIntel  1    /*Intel Sahalee servers, use alt soft-shutdown.*/
#define platMBMC   2    /*mini-BMC platforms */
#define platS5500  3    /*Intel S5500 platforms */
#define platOther  4    /*Other platforms */
#define  GET_POWER_STATE   0x07 
#define  INIT_VAL   0xff

/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil reset";
#else
static char * progver   = "3.08";
static char * progname  = "ireset";
#endif
static uchar  ipmi_maj;
static uchar  ipmi_min;
static uchar  sms_sa = 0x81;
static char   fdebug         = 0;
static char   fipmilan       = 0;
static char   fignore_opterr = 0;
static char   fwait          = 0;
static char   fpersist       = 0;
static char   platform  = 0;   /* platform type: MBMC or TSR */
static int    shuttime  = 60;  /* shutdown timeout in seconds */
#define MAX_INIT    77    /* 80 minus 3 for IANA */
static char * initstr = NULL;   /* boot initiator mailbox string */
static uchar  iana[3] = { 0x00, 0x00, 0x00 }; /*default to devid, see -j */
static uchar g_bus = PUBLIC_BUS;
static uchar g_sa  = BMC_SA;
static uchar g_lun = BMC_LUN;
static uchar g_addrtype = ADDR_SMI;
static uchar gbootparm = 0x48; //verbose disply, bypass pswd, same console redir

#if defined(EFI)
int getopt(int  argc, char **argv, const char *opts)
{  /*very simple getopt */
  int c = EOF;
  static int iopt = 0;
  iopt++;
  if ((argc <= iopt) || (argv[iopt] == NULL)) return c;
  if (argv[iopt][0] == '-') c = argv[iopt][1];
  return(c);
}
#endif
#if defined(ALONE)
int is_romley(int vend, int prod)
{
  int ret = 0;
  if (vend != VENDOR_INTEL) return(ret);
  if (prod >= 0x0048 && prod <= 0x005e) ret = 1;
  return(ret);
}
int write_syslog(char *msg) { return(0); } /*stub*/
#else
/*extern*/ int write_syslog(char *msg);  /*from showsel.c*/
#endif

static void show_error(char *tag, int rv, uchar cc)
{
   if (rv > 0 && cc == 0) cc = (uchar)rv;
   printf("%s: error %d ccode = %x %s\n", 
		tag,rv,cc,decode_cc((ushort)0,cc));
   return;
}

static int set_wdt(uchar val, uchar act)
{
        uchar idata[6];
        uchar rdata[16];
        int rlen = 8;
        uchar ccode;
        int ret, t;
 
        t = val * 10;      /* val is in sec, make timeout in 100msec */
        if ((ipmi_maj > 1) ||      /* IPMI 1.5 or greater */
            (ipmi_maj == 1 && ipmi_min >= 5))
             idata[0] = 0x44;    /* DontLog=0, DontStop=1 & use SMS/OS */
        else idata[0] = 0x04;    /* IPMI 1.0 or less */
        idata[1] = act; 
                /* 0x01;    * action: no pretimeout, hard reset action */
		/* 0x02;    * action value for power down instead */
        idata[2] = 0;      /* pretimeout: 30 sec (but disabled) */
        idata[3] = 0x10;   /* clear SMS/OS when done */
        idata[4] = t & 0x00ff;   /*timeout in 100msec: 0x4B0 = 1200. */
        idata[5] = (t & 0xff00) >> 8;
        ret = ipmi_cmd_mc(WATCHDOG_SET, idata, 6, rdata, &rlen, &ccode, fdebug);
	if (fdebug) printf("set_wdt: wd set(%d,%d) rv=%d cc=%x\n",
				val,act,ret,ccode);
        if (ret == 0 && ccode != 0) ret = ccode;

	if (ret == 0) {  /* Start the timer by issuing a watchdog reset */
	   ret = ipmi_cmd_mc(WATCHDOG_RESET,idata,0,rdata,&rlen, &ccode,fdebug);
	   if (fdebug) printf("set_wdt: wd reset rv=%d cc=%x\n",ret,ccode);
	   if (ret == 0 && ccode != 0) ret = ccode;
	}
        return(ret);
}  /*end set_wdt()*/

char *reset_str(uchar breset)
{
   char *str;
   switch(breset) {
        case 0:  str = "powering down"; break;
        case 1:  str = "powering up"; break;
        case 2:  str = "power cycling"; break;
        case 3:  str = "resetting"; break;  /* -r, etc.*/
        case 4:  str = "sending NMI"; break;
        case 5:  str = "shutdown/reset"; break;  
        case 6:  str = "shutdown/power_off"; break; /*via agent*/
        case 7:  str = "cold reset BMC"; break; 
        default: str = "resetting"; break;
   }
   return(str);
}

const char *target_str(uchar bopt)
{
   char *str = "";
   if (bopt > 0) 
     switch(bopt) {
        case 1:  str = " to Svc partition"; break;
        case 2:  str = " to EFI"; break;
        case 3:  str = " to PXE"; break;
        case 4:  str = " to CDROM"; break;
        case 5:  str = " to hard disk"; break;
        case 6:  str = " to BIOS Setup"; break;
        case 7:  str = " to floppy"; break;
        default: break;
     }
   return(str);
}

const char *instance_str(int instance)
{
   static char str[16] = { '\0' };
   if (instance > 0)
   {
      snprintf(str, sizeof(str), " %d", instance);
   }

   return str;
}

int set_boot_init_string(char *istr)
{
   int rv = 0;
   uchar idata[20];  /*need 17 bytes*/
   uchar rdata[MAX_BUFFER_SIZE];
   uchar cc;
   int i, n, len, rlen;

   /* set param 7 for boot initiator mailbox */
   len = (int)strlen(istr);
   n = 0;
   for (i = 0; n < len; i++) 
   {
      memset(idata,0,18);
      idata[0] = 0x07;  /* param, 7 = boot init mailbox */
      idata[1] = i;     /* set selector */
      if (i == 0) {  /* insert IANA */
	  idata[2] = iana[0];
	  idata[3] = iana[1];
	  idata[4] = iana[2];
	  strncpy((char *)&idata[5],&istr[n],13);
	  n += 13;
      } else {
	  strncpy((char *)&idata[2],&istr[n],16);
	  n += 16;
      }
      rlen = MAX_BUFFER_SIZE;
      rv = ipmi_cmd_mc(SET_BOOT_OPTIONS, idata, 18, rdata, &rlen, &cc, fdebug);
      if (rv == 0 && cc != 0) rv = cc;
      if (rv != 0) break;
   }
   return(rv);
}

static int IPMI_Reset(uchar bpower, uchar bootopt, uchar bEfiboot, int internal_instance)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar completionCode;
	uchar inputData[20];
	char initmsg[80];
	int status = 0;
	uchar cmd;

	/* May want to GetSystemBootOptions first to show existing. */

	/* if (bootopt != 0) then set param 3 to not clear boot valid flag */
	if (bootopt != 0) 
	{
	  inputData[0] = 0x03;  // param, 3 = boot clear 
	  if (fpersist) 
	     inputData[1] = 0x1F;  // persist all available conditions
	  else inputData[1] = 0x00;  // clear after next boot
	  responseLength = MAX_BUFFER_SIZE;
          status = ipmi_cmd_mc(SET_BOOT_OPTIONS, inputData, 2, responseData,
                        &responseLength, &completionCode, fdebug);
	  if (status == 0) status = completionCode;
	  if (status != 0) {
            if (fdebug || !fignore_opterr) 
              printf("set_boot_options ccode %x, resp[0] = %x, resp[1] =  %x\n",
                   completionCode, responseData[0], responseData[1]);
            if (!fignore_opterr) 
               return(status);  /* abort if the boot options can't be set */
	  }

	  inputData[0] = 0x05;  // param, 5 = boot flags
	  if (fpersist)
	     inputData[1] = 0xC0;  // valid flags, persistent
	  else inputData[1] = 0x80;  // valid flags, next boot only
          if (bEfiboot) inputData[1] |= 0x20;  // add boot to EFI (modifier)
          if (bootopt == 1)      inputData[2] = 0x10;   // boot to svc partition
          else if (bootopt == 3) inputData[2] = 0x04;   // boot to PXE
          else if (bootopt == 4) inputData[2] = 0x14;   // boot to CDROM
          else if (bootopt == 5) inputData[2] = 0x08;   // boot to Hard Disk
          else if (bootopt == 6) inputData[2] = 0x18;   // boot to BIOS Setup
          else if (bootopt == 7) inputData[2] = 0x3C;   // boot to Floppy/Remov
          else if (bootopt == 8) inputData[2] = 0x0C;   // boot to HardDisk/Safe
	  else inputData[2] = 0x00;  // normal boot
	  inputData[3] = gbootparm;
	  inputData[4] = 0x00; //no overrides
	  inputData[5] = internal_instance ? (INTERNAL_INSTANCE_FLAG | internal_instance)
	                                   : 0x00;
	  responseLength = MAX_BUFFER_SIZE;
          status = ipmi_cmd_mc(SET_BOOT_OPTIONS, inputData, 6, responseData,
                        &responseLength, &completionCode, fdebug);
	  if (status == 0) status = completionCode;
	  if (status != 0) {
            if (fdebug || !fignore_opterr) 
              printf("set_boot_options ccode %x, resp[0] = %x, resp[1] =  %x\n",
                   completionCode, responseData[0], responseData[1]);
            if (!fignore_opterr) 
               return(status);  /* abort if the boot options can't be set */
	  }
	  if (initstr != NULL) {
               status = set_boot_init_string(initstr);
               if (fdebug) printf("set_boot_init_string(%s) status = %d\n",
					initstr,status);
               if (status != 0) {
		  if (!fignore_opterr) return(status);
		  else status = 0;
               }
	  }
	}

	/* 
	 * fshutdown (bpower >= 5) for Intel:
	 * Since we are in user mode, we can't wait for it to fully 
	 * shut down and then issue the IPMI Chassis Reset. 
	 * IPMI can trigger this by emulating an overtemp event.
	 * There is also a watchdog/init0 method used by platIntel. 
	 * bpower was set by caller to 5 or 6
	 */
	if ((bpower >= 5) && (platform == platIntel))
        {     /*Intel os shutdown requested*/
	  if (fipmilan) { 
#ifdef EXPERIMENTAL
                int rv;
		/*Try remote shutdown via Bridged SMS */
	        inputData[0] = 5;  /* chassis ctl soft shutdown option */
	        responseLength = MAX_BUFFER_SIZE;
		rv = ipmi_cmdraw( CHASSIS_CTL,
			NETFN_APP,BMC_SA,PUBLIC_BUS, SMS_LUN,
                        inputData,1,responseData,&responseLength,
			&completionCode, fdebug);
	        printf("Remote soft shutdown initiated (%d,%d).\n",status,rv);
#else
	       /* abort if call this with fipmi_lan, platIntel. */
	       /* should have invoked remote agent method before this. */
               return(LAN_ERR_NOTSUPPORT);  
#endif
	 } else {  /* do local shutdown with wdt*/
           uchar action;
           char initcmd[16];
           char initchar, shutchar;

	   /*
	    * Special OS shutdown method for CG Servers 
	    * Set up a watchdog event to do reset after timeout. 
	    * Valid on other platforms too if they support watchdog.
	    * Note that the "init 0" only makes sense if local. 
	    */
           if (bpower == 6) { action = 0x02;  /*do power_down*/
               initchar = '0';
               shutchar = 's';
           } else { action = 0x01;  /*do hard_reset*/
               initchar = '6';
               shutchar = 'r';
           }
	   status = set_wdt((uchar)shuttime, action);
           if (status == 0) 
           {  /*local shutdown */
                sprintf(initmsg,"%s: soft shutdown -%c initiated\n",progname,shutchar);
		write_syslog(initmsg);
#ifdef WIN32
                sprintf(initcmd,"shutdown -%c -c %s",shutchar,progname);
		status = system(initcmd); /* do the OS shutdown */
	        printf("Windows soft shutdown initiated (%s).\n",initcmd);
#else
                sprintf(initcmd,"init %c",initchar);
		status = system(initcmd); /* do the OS shutdown */
	        printf("Linux soft shutdown initiated (%s).\n",initcmd);
#endif
	   }
	   /* 
            * Note that this can generate a Watchdog 2 event in the SEL. 
            * If the init 0/6 is successful within the 60 second timeout,
            * BIOS will stop the watchdog.
            */
	   return(status);
	 }  /*endif local*/
	} /*endif Intel os shutdown*/

        /* 0 = power down, 1 = power up, 2 = power cycle, 3 = hard reset */
        /* 4 = NMI interrupt, 5 = soft shutdown OS via ACPI  */
	if (bpower > 5) bpower = 5;  /* if invalid, try shutdown */
	if (!fipmilan) {   /*only write to syslog if local*/
	 sprintf(initmsg,"%s: chassis %s%s%s%s\n",progname,reset_str(bpower), target_str(bootopt),
	         instance_str(internal_instance), bEfiboot ? " (EFI)" : "");
         write_syslog(initmsg);
	}
	inputData[0] = bpower;  // chassis control reset
        responseLength = MAX_BUFFER_SIZE;
        status = ipmi_cmd_mc(CHASSIS_CTL, inputData, 1, responseData,
                        &responseLength, &completionCode, fdebug);
        if (fdebug) {
                printf("Chassis_Ctl(%x) ccode=%x, resp[0]=%x, resp[1]=%x\n",
		       bpower, completionCode, 
		       responseData[0], responseData[1]);
                }
	if (status == ACCESS_OK && completionCode == 0) {
		printf("chassis_reset(%x) ok\n",bpower);
		//successful, done
		return(0);
	} else if (fipmilan && (status < 0)) {
           /* Remote IPMI LAN reset could not connect, 
            * no point in continuing.  */
	   return(status);
	} else {
	   if (bpower == 5 && completionCode == 0xcc) {
		/* See IPMI spec 22.3 Chassis Control, Table 22-4 */
		printf("Optional soft-shutdown mode not supported\n");
		/* If get here, need to use method like platIntel above. */
	   } else {
		show_error("chassis_reset",status,completionCode);
		// status = -1;

		/* Try net_app warm/cold reset commands instead */
		if (bpower == 2) cmd = 2; /* cold reset */
		else cmd = 3;             /* warm reset */
                responseLength = MAX_BUFFER_SIZE;
		status = ipmi_cmdraw( cmd,NETFN_APP, g_sa, g_bus, g_lun,
                        inputData,0,responseData,&responseLength,
			&completionCode, fdebug);
		if (status == ACCESS_OK && completionCode == 0) {
		   printf("netapp_reset ok\n");
		} else {
		   show_error("netapp_reset",status,completionCode);
		   if (status == 0) status = completionCode;
		}
	   }
	} /*end else*/
	return(status);
}  /*end IPMI_Reset()*/

static void wait_ready(void)
{
   int i, c;
   uchar devrec[16];
   /* wait for BMC ready again */
   os_usleep(1,0);   /*delay 1 sec*/
   for (i = 0; i < 15; i++) {
   	os_usleep(1,0);   /*delay 1 sec*/
   	c = ipmi_getdeviceid(devrec,16,fdebug);
   	if (c == 0) break;
   	else {  /* expect LAN_ERR_RECV_FAIL if BMC not ready */
		if (fdebug) printf("after reset, try%d ret = %d\n",i,c);
		if (c != LAN_ERR_RECV_FAIL) break;
   	}
   } /*end-for*/
}

static void show_usage(void)
{
                printf("Usage: %s [-bcdDefhkmnoprsuwxy -N node -U user -P/-R pswd -EFTVY]\n",
			progname);
                printf(" where -c  power Cycles the system\n");
                printf("       -d  powers Down the system\n");
                printf("       -D  soft-shutdown OS and power down\n");
                printf("       -k  do Cold Reset of the BMC firmware\n");
                printf("       -i<str>  set boot Initiator mailbox string\n");
                printf("       -j<num>  set IANA number for boot Initiator\n");
                printf("       -l<num>  select internal Logical instance to boot\n");
                printf("       -n  sends NMI to the system\n");
                printf("       -o  soft-shutdown OS and reset\n");
                printf("       -r  hard Resets the system\n");
                printf("       -u  powers Up the system\n");
                printf("       -m002000 specific MC (bus 00,sa 20,lun 00)\n");
                printf("       -b  reboots to BIOS Setup\n");
                printf("       -e  modifier: interpret 'reboot to' as an EFI target\n");
                printf("       -f  reboots to Floppy/Removable\n");
                printf("       -h  reboots to Hard Disk\n");
                printf("       -p  reboots to PXE via network\n");
                printf("       -s  reboots to Service Partition\n");
                printf("       -v  reboots to DVD/CDROM Media\n");
                printf("       -w  Wait for BMC ready after reset\n");
                printf("       -x  show eXtra debug messages\n");
                printf("       -y  Yes, persist boot options [-befhpms]\n");
		print_lan_opt_usage(0);
}

void parse_boot_device(int c, uchar* breset, uchar* bopt)
{
   switch(c) {
       case 's': *bopt = 1; break; /* hard reset to svc part */
       case 'p': *bopt = 3; break; /* hard reset to PXE */
       case 'v': *bopt = 4; break; /* hard reset to DVD/CD Media*/
       case 'h': *bopt = 5; break; /* hard reset to Hard Disk  */
       case 'b': *bopt = 6; break; /* hard reset to BIOS Setup */
       case 'f': *bopt = 7; break; /* hard reset to floppy/remov*/
   }

   // This allows use of a boot device with -u and -c
   if (*breset == INIT_VAL)
      *breset = 3;
}

#ifdef METACOMMAND
int i_reset(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
#endif
{
   int ret;
   int c;
   uchar breset;
   uchar bopt;
   uchar fshutdown = 0;
   uchar devrec[16];
   int rlen;
   uchar rqdata[10];
   uchar rsdata[32];
   int rslen;
   int mfg = 0;
   int internal_instance = 0;
   uchar cc;
   char *s1;
   uchar bEfiboot = 0;

#if defined (EFI)
   InitializeLib(_LIBC_EFIImageHandle, _LIBC_EFISystemTable);
#endif
   // progname = argv[0];
   printf("%s ver %s\n", progname,progver);
   breset = INIT_VAL; /* invalid as default, require an option */
   bopt = 0;    /* Boot Options default */
   /* Request admin privilege by default, since power control requires it. */
   parse_lan_options('V',"4",0); 

   while ((c = getopt(argc,argv,"bcdDefhi:j:kl:m:noprsuvwyT:V:J:YEF:N:P:R:U:Z:x?")) != EOF)
      switch(c) {
          case 'd': breset = 0;     break;  /* power down */
          case 'u': breset = 1;     break;  /* power up */
          case 'c': breset = 2;     break;  /* power cycle */
          case 'o': breset = 5; fshutdown = 1; break; /*soft shutdown,reset*/
          case 'D': breset = 6; fshutdown = 1; break; /*soft shutdown,pwrdown*/
          case 'n': breset = 4;     break;  /* interrupt (NMI) */
          case 'r': breset = 3;     break;  /* hard reset */
          case 'e': bEfiboot = 1; break;  /* Perform EFI boot (instead of legacy/"PC compatible" boot) */

          case 's':  /* hard reset to svc part */
          case 'p':  /* hard reset to PXE */
          case 'v':  /* hard reset to DVD/CD Media*/
          case 'h':  /* hard reset to Hard Disk  */
          case 'b':  /* hard reset to BIOS Setup */
          case 'f':  /* hard reset to floppy/remov*/
            parse_boot_device(c, &breset, &bopt);
            break;

          case 'i': if (strlen(optarg) < MAX_INIT) initstr = optarg; break; 
          case 'j': mfg = atoi(optarg);     /*IANA number*/
		    iana[0] = ((mfg & 0xFF0000) >> 16);
		    iana[1] = ((mfg & 0x00FF00) >> 8);
		    iana[2] = (mfg & 0x0000FF);
		    break;
          case 'k': breset = 7;    break;  /* cold reset to BMC */
          case 'l': internal_instance = atoi(optarg);  break; /* internal boot device instance number */
          case 'w': fwait = 1;     break;  /* wait for ready */
          case 'y': fpersist = 1;  break;  /* yes, persist boot options */
          case 'm': /* specific MC, 3-byte address, e.g. "409600" */
                    g_bus = htoi(&optarg[0]);  /*bus/channel*/
                    g_sa  = htoi(&optarg[2]);  /*device slave address*/
                    g_lun = htoi(&optarg[4]);  /*LUN*/
                    if (optarg[6] == 's') {
                             g_addrtype = ADDR_SMI;  s1 = "SMI";
                    } else { g_addrtype = ADDR_IPMB; s1 = "IPMB"; }
		    ipmi_set_mc(g_bus,g_sa,g_lun,g_addrtype);
                    printf("set MC at %s bus=%x sa=%x lun=%x\n",
                            s1,g_bus,g_sa,g_lun);
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
          case 'x': fdebug = 1;     break;  /* debug messages */
	  default:
		show_usage();
		ret = ERR_USAGE;
		goto do_exit;
      }

   if (breset == INIT_VAL) {
	show_usage();
	printf("An option is required\n");
	ret = ERR_BAD_PARAM;
	goto do_exit;
   }

   if (   (internal_instance < 0)
       || (internal_instance > MAX_LOGICAL_INSTANCE)) {
	printf("Please specify an instance number between 0 and %d\n", MAX_LOGICAL_INSTANCE);
	ret = ERR_BAD_PARAM;
	goto do_exit;
   }

   fipmilan = is_remote();
   /* 
    * Check the Device ID to determine the platform type.
    */
   ret = ipmi_getdeviceid(devrec,16,fdebug);
   if (ret != 0) {
	ipmi_close_(); 
	goto do_exit;
   } else {
      char *pstr;
      int    vendid;
      ushort prodid;
      uchar j;

      if (fdebug) {
	printf("devid: ");
	for (j = 0; j < 16; j++) printf("%02x ",devrec[j]);
	printf("\n");
      }
      ipmi_maj = devrec[4] & 0x0f;
      ipmi_min = devrec[4] >> 4;
      vendid = devrec[6] + (devrec[7] << 8) + (devrec[8] << 16);
      if (mfg == 0) memcpy(iana,&devrec[6],3); /*not set, use default*/
      prodid = devrec[9] + (devrec[10] << 8);
      pstr = "BMC";
      if (fdebug) printf("vendor = %06x, product_id = %04x\n",vendid,prodid);
      if (vendid == VENDOR_NSC) {   /* NSC mBMC */
	  pstr = "mBMC";
	  platform = platMBMC;
      } else if (vendid == VENDOR_HP) {   /* HP */
	  platform = platOther;   /* other platform types */
	  gbootparm = 0x00;  
          fignore_opterr = 1;  /* ignore boot options errors */
      } else if (vendid == VENDOR_TYAN) {   /* Tyan */
	  platform = platOther;   /* other platform types */
          fignore_opterr = 1;  /* ignore boot options errors */
      } else if (vendid == VENDOR_INTEL) {   /* Intel */
	  if (prodid != 0x0100)  /* ia64 Itanium2 is different */
	      platform = platIntel;  /* else handle as Intel Sahalee */
#ifdef OBSOLETE	
	  if (is_romley(vendid,prodid) || (prodid == 0x003E)) {  
		/* Romley or Thurley/S5520UR */
	      platform = platS5500;  /* not like Intel Sahalee */
	      set_max_kcs_loops(URNLOOPS); /*longer KCS timeout*/
	  }
#endif
      } else if (vendid == VENDOR_KONTRON) {   /* Kontron */
          fignore_opterr = 1;  /* ignore boot options errors */
	  /* supports Chassis Soft Power command 0x05, so not platIntel */
	  platform = platOther;  /* handle like other platforms */
      } else {             /* other vendors */
	  platform = platOther;   /* other platform types */
      }
      printf("-- %s version %x.%x, IPMI version %d.%d \n",
             pstr, devrec[2],  devrec[3], ipmi_maj, ipmi_min);
   }

   { /* show current power state */
       char *pstr;
       uchar pstate;
       rlen = sizeof(devrec);
       ret = ipmi_cmdraw( GET_POWER_STATE, NETFN_APP,
                        g_sa, g_bus, g_lun,
                        NULL,0, devrec,&rlen,&cc, fdebug);
       if (ret == 0) {
          pstate = devrec[0] & 0x7f;
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
             default:   pstr = "unknown"; break;
          }
          if (cc == 0) 
             printf("Power State      = %02x   (%s)\n",pstate,pstr);
       }
   }

   if (breset == 7) { /*do Cold Reset to BMC */
      printf("%s: %s ...\n",progname,reset_str(breset));
      rslen = sizeof(rsdata);
      ret = ipmi_cmdraw( 0x02, NETFN_APP, g_sa, g_bus, g_lun,
                        rqdata,0, rsdata, &rslen, &cc, fdebug);
      if (fdebug) 
         printf("cold_reset(%02x) ret=%d cc=%x, rslen=%d\n",g_sa,ret,cc,rslen);
      if (ret == 0) ret = cc;
      if (ret == 0) 
  	 printf("%s: Cold_Reset to BMC ok\n",progname);
      else
  	 printf("%s: Cold_Reset to BMC error %d\n",progname,ret);
      ipmi_close_(); 

   } else if (fshutdown && fipmilan && (platform == platIntel)) { /*soft reset*/
      int fdaemonok = 0;
      /* Either do special remote soft-shutdown, or
       * handle it within IPMI_Reset. */
      /* Special remote soft-shutdown, requires a service to
       * be running on the target node.
       * GET_SOFTWARE_ID == 0x00 
       * SMS_OS_REQUEST  == 0x10 : (down=0, reset=1)
       * BRIDGE_REQUEST  == 0x20 : (down=0, reset=1)
       * SMS_SA  == 0x81
       */
      rslen = sizeof(rsdata);
      ret = ipmi_cmdraw( 0x00, NETFN_APP,sms_sa,PUBLIC_BUS, SMS_LUN,
                        rqdata,0, rsdata, &rslen, &cc, fdebug);
      if (fdebug) 
	   printf("ipmilan getswid ret=%d cc=%x, rslen=%d\n",ret,cc,rslen);
      if (ret == 0 && cc == 0) {
         ushort v,x;
         v = (rsdata[6] << 16) + (rsdata[7] << 8) + rsdata[8];
         x = (rsdata[9] << 8) + rsdata[10];
         if (fdebug) printf("swid v: %06x x: %04x\n",v,x);
	 if (v == 0x000157 && x == 0x0001) fdaemonok = 1;
      }
      if (fdaemonok) {
	 /* os_usleep(0,50000);   *delay 50 ms, not needed*/
         if (breset == 0 || breset == 6)
              rqdata[0] = 0x01;   /* shutdown & power down */
         else rqdata[0] = 0x02;   /* shutdown & reset */
         if (fdebug) printf("ipmilan shutdown action=%x\n",rqdata[0]);
         rslen = sizeof(rsdata);
         ret = ipmi_cmdraw( SMS_OS_REQUEST, NETFN_APP,sms_sa,PUBLIC_BUS,SMS_LUN,
                        rqdata,1, rsdata, &rslen, &cc, fdebug);
         printf("ipmilan shutdown request: ret = %d, cc = %x\n", ret,cc);
         if (fipmilan && fwait) {
	  	ipmi_close_();  /* to try new connection */
		wait_ready();
	 }
      } 
      else printf("ipmilan async bridge agent not present\n");
      ipmi_close_(); 
   } else {
      printf("%s: %s%s%s%s ...\n",progname,reset_str(breset), target_str(bopt),
             instance_str(internal_instance), bEfiboot ? " (EFI)" : "");
      ret = IPMI_Reset(breset, bopt, bEfiboot, internal_instance);
      if (ret == 0) {   /* if ok */
  	  printf("%s: IPMI_Reset ok\n",progname);
	  /* It starts resetting by this point, so do not close. */
	  if (breset == 4) ipmi_close_();   /*NMI, so close*/
          if (fipmilan && fwait) {
	  	ipmi_close_();  /* to try new connection */
		wait_ready();
	  	ipmi_close_(); 
	  }
      } else {
	  printf("%s: IPMI_Reset error %d\n",progname,ret);
	  ipmi_close_(); 
      }
   }
do_exit:
   // show_outcome(progname,ret);  
   return(ret);
}  /* end main()*/

/* end ireset.c */
