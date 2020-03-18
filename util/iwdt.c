/*
 * wdt.c
 *
 * This tool reads and enables the watchdog timer via IPMI.
 * Note that there are other methods for doing this, and the 
 * standard interface is for the driver to expose a /dev/watchdog 
 * device interface.
 * WARNING: If you enable the watchdog, make sure you have something
 * set up to keep resetting the timer at regular intervals, or it 
 * will reset your system. 
 * The Intel Server Management software does this automatically, 
 * and also continues to reset the timer as long as it is running.
 * 
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2003-2006 Intel Corporation. 
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 02/25/03 Andy Cress - created
 * 03/05/03 Andy Cress - added -d option to disable watchdog timer
 * 06/11/03 Andy Cress - new ver 1.2 for EMSGSIZE fix 
 * 10/28/03 Andy Cress - fixed cc error in set_wdt (idata size),
 *                       show action.
 * 02/06/04 Andy Cress - added WIN32 flags
 * 03/11/04 Andy Cress - 1.4  fixed cc=0xcc if pretimeout not zero.
 * 05/05/04 Andy Cress - 1.5  call ipmi_close before exit
 * 11/01/04 Andy Cress - 1.6  add -N / -R for remote nodes   
 * 11/16/04 Andy Cress - 1.7  add -U for remote username
 * 12/02/04 Andy Cress - 1.8  add counter & pretimeout display in show_wdt
 *                            added EFI ifdefs
 * 04/13/06 Andy Cress - 1.9  fix -t if nsec > 255 
 * 06/22/06 Andy Cress - 1.10 add -a action and -l dontlog options
 * 06/25/08 Andy Cress - 2.13 add -p for pre-timeout action
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
#elif defined(EFI)
 // EFI: defined (EFI32) || defined (EFI64) || defined(EFIX64)
 #ifndef NULL
    #define NULL    0
 #endif
 #include <types.h>
 #include <libdbg.h>
 #include <unistd.h>
 #include <errno.h>
#elif defined(DOS)
 #include <dos.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "getopt.h"
#else
/* Linux, Solaris, BSD */
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
 
/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil wdt";
#else
static char * progver   = "3.08";
static char * progname  = "iwdt";
#endif
static char   fdebug    = 0;
static char   fdontlog  = 0;
static char   fcanonical = 0;
static char   bdelim    = BDELIM; /* '|' */
static uchar  action    = 1;  /* default is Hard Reset */
                       /* 0 = "No action"
                        * 1 = "Hard Reset"
                        * 2 = "Power down"
                        * 3 = "Power cycle" */
static uchar  preaction = 0;  /* default is None */
                       /* 1 = "SMI "
                        * 2 = "NMI "
                        * 3 = "Messaging Interrupt" */
static uchar  pretime = 0;   /* usually 30 second default pre-timeout */
static uchar ipmi_maj = 0;
static uchar ipmi_min = 0;

static int reset_wdt(void)
{
	uchar idata[4];
	uchar rdata[16];
	int rlen = 4;
	uchar ccode;
	int ret;

        ret = ipmi_cmd(WATCHDOG_RESET, idata, 0, rdata, &rlen, &ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}

static int get_wdt(uchar *rdata, int rlen)
{
	uchar idata[4];
	uchar ccode;
	int ret;

        ret = ipmi_cmd(WATCHDOG_GET, idata, 0, rdata, &rlen, &ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end get_wdt()*/

static int set_wdt(int val)
{
	uchar idata[6];
	uchar rdata[16];
	int rlen = 8;
	uchar ccode, bl;
	int ret, t;

	t = val * 10;      /* val is in sec, make timeout in 100msec */
        if (fdontlog) bl = 0x80;
        else bl = 0x00;
        if ((ipmi_maj > 1) ||      /* IPMI 1.5 or greater */
            (ipmi_maj == 1 && ipmi_min >= 5)) {
             idata[0] = 0x44 | bl; /* DontLog=0, DontStop=1 & use SMS/OS */
        } else idata[0] = 0x04 | bl;    /* IPMI 1.0 or less */
	idata[1] = (preaction << 4) | action; /* preaction/action */
	idata[2] = 0;      /* pretimeout: 0=disabled, or less than timeout */
	if (preaction != 0) {  /* enabled pretimeout */
	   idata[2] = pretime; /* pretimeout value in seconds */
        }  /*endif preaction/pretime */
	idata[3] = 0x10;   /* clear SMS/OS when done */
	idata[4] = t & 0x00ff;   /*timeout in 100msec: 0x4B0 = 1200. */
	idata[5] = (t & 0xff00) >> 8;
        ret = ipmi_cmd(WATCHDOG_SET, idata, 6, rdata, &rlen, &ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end set_wdt()*/

static int clear_wdt(void)
{
	uchar idata[6];
	uchar rdata[16];
	int rlen = 8;
	uchar ccode = 0;
	int ret;

	idata[0] = 0x01;   /* Use FRB2, stop timer */
	idata[1] = 0x00;   /* no action */
	idata[2] = 30;     /* pretimeout: 30 sec (disabled anyway) */
	idata[3] = 0x02;   /* clear FRB2*/
	idata[4] = 0xB0;
	idata[5] = 0x04;
        ret = ipmi_cmd(WATCHDOG_SET, idata, 6, rdata, &rlen, &ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end clear_wdt()*/

char *usedesc[6] = {"reserved", "BIOS FRB2", "BIOS/POST",
		    "OS Load", "SMS/OS", "OEM" };

void show_wdt(uchar *wdt)
{
   uchar use; 
   char *pstr; 
   char *pstr2;
   int i,j;

   if (!fcanonical) {
      printf("wdt data: ");
      for (i=0; i<8; i++) printf("%02x ",wdt[i]);
      printf("\n");
   }

   use = wdt[0] & 0x07;
   if (use > 5) use = 0; 
   if ((wdt[0] & 0x40) == 0x40) pstr = "started";
   else pstr = "stopped";
   if ((wdt[0] & 0x80) == 0x80) pstr2 = "DontLog";
   else pstr2 = "Logging";
   if (fcanonical) {
      printf("Watchdog timer state\t%c %s\n",bdelim,pstr);
      printf("  Use with \t\t%c %s\n", bdelim,usedesc[use]);
      printf("  Log mode \t\t%c %s\n", bdelim,pstr2);
   } else {
   printf("Watchdog timer is %s for use with %s. %s\n",pstr,usedesc[use],pstr2);
   }
   switch (wdt[1] & 0x70)
   {
	case 0x10:  pstr = "SMI"; break;
	case 0x20:  pstr = "NMI"; break;
	case 0x30:  pstr = "MsgInt"; break;
	default:    pstr = "None";
   }
   if (fcanonical) {
      printf("  Pretimeout\t\t%c %d seconds\n", bdelim,wdt[2]);
      printf("  Pre-action\t\t%c %s\n", bdelim,pstr);
   } else {
      printf("               pretimeout is %d seconds, pre-action is %s\n",
				wdt[2],pstr);
   }
   /* wdt[3] is the TimerUseExpFlags */
   i = (wdt[4] + (wdt[5] << 8)) / 10;
   j = (wdt[6] + (wdt[7] << 8)) / 10;
   if (fcanonical) {
      printf("  Timeout  \t\t%c %d seconds\n", bdelim,i);
      printf("  Counter  \t\t%c %d seconds\n", bdelim,j);
   } else {
   printf("               timeout is %d seconds, counter is %d seconds\n",i,j);
   }
   switch (wdt[1] & 0x07)
   {
	case 0:  pstr = "No action"; break;
	case 1:  pstr = "Hard Reset"; break;
	case 2:  pstr = "Power down"; break;
	case 3:  pstr = "Power cycle"; break;
	default:  pstr = "Reserved";
   }
   if (fcanonical) {
      printf("  Action  \t\t%c %s\n", bdelim,pstr);
   } else {
   printf("               action is %s\n",pstr);
   }
}


#ifdef METACOMMAND
int i_wdt(int argc, char **argv)
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
   int rv = 0;
   int c;
   uchar freadonly = 1;
   uchar freset = 0;
   uchar fdisable = 0;
   uchar wdtbuf[8];
   uchar devrec[16];
   int t = 0;
   int a;

#if defined (EFI)
        InitializeLib(_LIBC_EFIImageHandle, _LIBC_EFISystemTable);
#else
        printf("%s ver %s\n", progname,progver);
#endif
   parse_lan_options('V',"4",0);  /*default to admin priv*/

   while ((c = getopt(argc,argv,"cdelra:p:q:t:T:V:J:EYF:P:N:R:U:Z:x?")) != EOF )
      switch(c) {
          case 'r': freset  = 1;   break;  /* reset watchdog timer */
          case 'l': fdontlog = 1;  break;  /* dont log the wdt events */
          case 'a': a = atoi(optarg);      /* set wd action */
                    if (a >= 0 && a < 4) action = (uchar)a;
                    freadonly = 0;
                    break; 
          case 'p': a = atoi(optarg);      /* set wd preaction */
                    if (a >= 0 && a < 4) preaction = (uchar)a;
                    freadonly = 0;
                    break; 
          case 'q': a = atoi(optarg);      /* set wd pretimeout */
		    if (a > 255) pretime = 255;
                    else if (a >= 5) pretime = (uchar)a;
		    /* else arg is out of bounds */
                    freadonly = 0;
                    break; 
          case 'c': fcanonical = 1; break;  /* canonical output */
          case 'd': fdisable = 1;   break;  /* disable wdt */
          case 'e': freadonly = 0; break;      /* enable wdt */
          case 't': t = atoi(optarg); freadonly = 0; break;  /*timeout*/
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
                printf("Usage: %s [-acdelpqrx -t sec -NUPRETVF]\n", progname);
                printf(" where -r      reset watchdog timer\n");
                printf("       -a N    set watchdog Action (N=0,1,2,3)\n");
                printf("       -c      canonical output format\n");
                printf("       -d      disable watchdog timer\n");
                printf("       -e      enable watchdog timer\n");
                printf("       -l      don't Log watchdog events\n");
                printf("       -p N    set watchdog Preaction (N=0,1,2,3)\n");
                printf("       -q N    set watchdog pretimeout to N sec\n");
                printf("       -t N    set timeout to N seconds\n");
                printf("       -x      show eXtra debug messages\n");
		print_lan_opt_usage(0);
		ret = ERR_USAGE;
		goto do_exit;
      }
   if (t == 0) t = 120;  /* default timeout, 120 seconds*/
   if ((pretime != 0) && (preaction == 0)) preaction = 2; /*default is NMI*/
   if (preaction != 0) {
	if (pretime == 0) {
	   if (t >= 280) { pretime = 255;
	   } else {
	      a = t * 10;      /* t is in sec, make timeout(a) in 100msec */
	      pretime = (a - t)/10;  /*90% of timeout, in seconds*/
	   }
	}
	if ((t - pretime) < 5) {  /*if not enough difference*/
	   /* (timeout - pretimeout) must be >= 5 sec */
	   /* if val < 50 sec, 10% diff < 5 */
	   if (t < 20) pretime = 0;  /* not enough headroom */
           else pretime = t - 5;
	}
   }

   ret = ipmi_getdeviceid(devrec,16,fdebug);
   if (ret != 0) {
      goto do_exit;
   } else {
      ipmi_maj = devrec[4] & 0x0f;
      ipmi_min = devrec[4] >> 4;
#ifndef EFI
      show_devid( devrec[2],  devrec[3], ipmi_maj, ipmi_min);
#endif
   }

   ret = get_wdt(&wdtbuf[0],8);
   if (ret != 0) {
	printf("get_wdt error: ret = %x\n",ret);
	goto do_exit;
   }
   show_wdt(wdtbuf);
   
   if (fdisable) {
      printf("Disabling watchdog timer ...\n");
      ret = clear_wdt();
      if (ret != 0) printf("clear_wdt error: ret = %x\n",ret);
      ret = get_wdt(wdtbuf,8);
      show_wdt(wdtbuf);
   } else if (!freadonly) {
      printf("Setting watchdog timer to %d seconds ...\n",t);
      ret = set_wdt(t);
      if (ret != 0) printf("set_wdt error: ret = %x\n",ret);
      rv = get_wdt(wdtbuf,8);
      show_wdt(wdtbuf);
      /* 
       * If we set the wd timer, we need to set up a cron job, daemon,
       * or script to reset the timer also.  (e.g.: "sleep 1; wdt -r")
       */
   } 
   if (freset && !fdisable) {
      printf("Resetting watchdog timer ...\n");
      ret = reset_wdt();
      if (ret == 0xC0) /*node busy*/
          printf("Node busy: set the timeout longer.\n");
#ifndef EFI
      printf("reset_wdt: ret = %d\n",ret);
      rv = get_wdt(wdtbuf,8);
      show_wdt(wdtbuf);
#endif
   }
#ifndef EFI
   printf("\n");
#endif
do_exit:
   ipmi_close_();
   // show_outcome(progname,ret); 
   return (ret);
}  /* end main()*/

/* end wdt.c */
