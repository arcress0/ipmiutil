/*
 * icmd.c
 *
 * This tool takes command line input as an IPMI command.
 * 
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2003-2006 Intel Corporation. 
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 03/30/04 Andy Cress - created
 * 04/08/04 Andy Cress - display response data, changed usage order
 * 05/05/04 Andy Cress - call ipmi_close before exit
 * 11/01/04 Andy Cress - add -N / -R for remote nodes   
 * 12/08/04 Andy Cress v1.5 changed usage order, moved bus to first byte,
 *                       gives better compatibility with ipmicmd.exe.
 * 04/13/06 Andy Cress v1.6 fixed istart for -U -R
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
#if defined(EFI)
 #ifndef NULL
    #define NULL    0
 #endif
 #include <types.h>
 #include <libdbg.h>
 #include <unistd.h>
 #include <errno.h>
#else
 /* Linux, Solaris, BSD, Windows */
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
#endif
#include "ipmicmd.h"
extern void ipmi_lan_set_timeout(int ipmito, int tries, int pingto); 
 
/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil cmd";
#else
static char * progver   = "3.08";
static char * progname  = "icmd";
#endif
static char   fdebug    = 0;
static char   fquiet    = 0;
static char   fset_mc   = 0;
static char   ftest     = 0;
static int    vend_id;
static int    prod_id;
static char   fmBMC = 0;
static char   fdecimal = 0; 
extern int   fjustpass;  /*see ipmicmd.c*/

#define PRIVATE_BUS_ID      0x03 // w Sahalee,  the 8574 is on Private Bus 1
#define PERIPHERAL_BUS_ID   0x24 // w mBMC, the 8574 is on the Peripheral Bus
#define MAXRQLEN 64
#define MAXRSLEN 200

static char usagemsg[] = "Usage: %s [-kqmsxEFNPRTUV] bus rsSa netFn/lun cmd [data bytes]\n";
static uchar busid = PRIVATE_BUS_ID;
static uchar g_bus = PUBLIC_BUS;
static uchar g_sa  = 0;    /*0x20*/
static uchar g_lun = BMC_LUN;
static uchar g_addrtype = ADDR_SMI;

static int send_icmd(uchar *cmd, int len)
{
	uchar responseData[MAXRSLEN];
	int responseLength;
	uchar completionCode;
	int ret, i;
	uchar netfn, lun;
        ushort icmd;

        /* icmd format: 0=bus, 1=rsSa, 2=netFn/lun, 3=cmd,  4-n=data */
	netfn = cmd[2] >> 2;
	lun = cmd[2] & 0x03;
        icmd = cmd[3] + (netfn << 8);
        if (fjustpass) {
           ret = ipmi_cmdraw(IPMB_SEND_MESSAGE,NETFN_APP,BMC_SA,0,0,
			&cmd[0],(uchar)(len), responseData, &responseLength, 
			&completionCode, fdebug);
        } else {
	   responseLength = sizeof(responseData);
           if (g_addrtype == ADDR_IPMB) {  /* && (g_sa != 0) */
              /* if -m used, do ipmi_cmd_mc for IPMB commands  */
              ipmi_set_mc(g_bus,g_sa,g_lun,g_addrtype);
              ret = ipmi_cmd_mc(icmd, &cmd[4], (uchar)(len-4), responseData, 
				&responseLength, &completionCode, fdebug);
              ipmi_restore_mc();
           } else {
	      /* ipmi_cmdraw: bus, cmd, netfn, sa, lun, pdata, ... */
              ret = ipmi_cmdraw(cmd[3], netfn, cmd[1], cmd[0], lun, &cmd[4], 
			(uchar)(len-4), responseData, &responseLength, 
			&completionCode, fdebug);
           }
        }
	if (ret < 0) {
           printf("ipmi_cmd: ret = %d %s\n",ret,decode_rv(ret));
	   return(ret);
        } else if ((ret != 0) || (completionCode != 0)) {
           printf("ipmi_cmd: ret = %d, ccode %02x %s\n",
                    ret, completionCode, decode_cc(icmd,completionCode));
	   return(ret);
	} else if (responseLength > 0) {
	   /* show the response data */
	   printf("respData[len=%d]: ",responseLength);
	   for (i = 0; i < responseLength; i++)
	      printf("%02x ",responseData[i]);
	   printf("\n");
	}
	return(ret);
}  /*end send_icmd()*/


#ifdef METACOMMAND
int i_cmd(int argc, char **argv)
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
   uchar devrec[17];
   uchar cmdbuf[MAXRQLEN];
   int maxlen = MAXRQLEN;
   int i, j, istart, cmdlen;
   int fskipdevid = 0;
   int fprivset = 0;
   int cmdmin;
   char *s1;

   istart = 1;
   while ( (c = getopt( argc, argv,"djkm:qst:xp:N:P:R:U:EF:J:T:V:YZ:?")) != EOF ) 
      switch(c) {
          case 'j': /* just pass the bytes to KCS */
		    fjustpass = 1;
		    break;
          case 'd': /* decimal command bytes, skipping bus and sa, 
		       which matches 'ipmitool raw' format. */
		    fdecimal = 1;
		    break;
          case 'm': /* specific MC, 3-byte address, e.g. "409600" */
                    g_bus = htoi(&optarg[0]);  /*bus/channel*/
                    g_sa  = htoi(&optarg[2]);  /*device slave address*/
                    g_lun = htoi(&optarg[4]);  /*LUN*/
                    if (optarg[6] == 's') {
                             g_addrtype = ADDR_SMI;  s1 = "SMI";
                    } else { g_addrtype = ADDR_IPMB; s1 = "IPMB"; }
		    fset_mc = 1;
                    printf("set MC at %s bus=%x sa=%x lun=%x\n",
                            s1,g_bus,g_sa,g_lun);
                    break;
          case 'q': fquiet = 1; fskipdevid = 1; break; /* minimal output */
          case 's': fskipdevid = 1;   break;  /* skip devid */
          case 't': /* set IPMI timeout for ipmilan, usu 2 * 4 = 8 sec */
		    i = atoi(optarg);
		    if (i >= 2) { j = i / 2; i = j; }
		    else { i = 1; j = 1; }
		    ipmi_lan_set_timeout(i,j,1);
		    break;
          case 'k': /* check for IPMI access */
		    ftest = 1;
		    break;
          case 'x': fdebug = 1;       break;  /* debug messages */
          case 'V':    /* priv level */
		fprivset = 1;
          case 'p':    /* port */
          case 'N':    /* nodename */
          case 'U':    /* remote username */
          case 'P':    /* remote password */
          case 'R':    /* remote password */
          case 'E':    /* get password from IPMI_PASSWORD environment var */
          case 'F':    /* force driver type */
          case 'T':    /* auth type */
          case 'J':    /* cipher suite */ 
          case 'Y':    /* prompt for remote password */
          case 'Z':    /* set local MC address */
                parse_lan_options(c,optarg,fdebug);
                break;
	  default:
		printf("%s ver %s\n", progname,progver);
                printf(usagemsg, progname);
                printf(" where -x   shows eXtra debug messages\n"); 
                printf("       -d   decimal input, matching ipmitool syntax\n");
                printf("       -k   check for IPMI access\n"); 
                printf("       -m002000 specific MC (bus 00,sa 20,lun 00)\n"); 
                printf("       -q   quiet mode, with minimal headers\n"); 
                printf("       -s   skips the GetDeviceID command\n"); 
		print_lan_opt_usage(1);
                ret = ERR_USAGE;
		goto do_exit;
      }

   if (ftest) {
	ret = ipmi_getdeviceid(devrec,16,fdebug);
	if (ret == 0) {
	   /*check if a driver is loaded, or direct*/
	   i = get_driver_type();
	   printf("IPMI access is ok, driver type = %s\n",show_driver_type(i));
	   if ((i ==  DRV_KCS) || (i == DRV_SMB)) 
	      printf("Using driverless method\n");
	}
	else printf("IPMI access error %d\n",ret);
	goto do_exit;
   }

   if (!fquiet) {
      printf("%s ver %s\n", progname,progver);
      printf("This is a test tool to compose IPMI commands.\n");
      printf("Do not use without knowledge of the IPMI specification.\n");
   }
   istart = optind;
   if (fdebug) printf("icmd: argc=%d istart=%d\n",argc,istart);

   if (fset_mc == 1) {
      /* target a specific MC via IPMB (usu a picmg blade) */
      ipmi_set_mc(g_bus,g_sa,g_lun,g_addrtype);
   } else g_sa = BMC_SA;

   for (i = 0; i < istart; i++) {
      argv++; argc--;
   }
   if (argc < maxlen) maxlen = argc;
   if (fdebug) printf("icmd: len=%d, cmd byte0=%s\n",maxlen,argv[0]);
   if (fdecimal) {
      uchar b;
      cmdbuf[0] = g_bus;
      cmdbuf[1] = g_sa;
      for (i = 0; i < maxlen; i++) {
	 if (argv[i][0] == '0' && argv[i][1] == 'x')  /*0x00*/
              b = htoi(&argv[i][2]);
         else b = atob(argv[i]);  /*decimal default, or 0x00*/
	 if (i == 0) { /*special handling for netfn */
	    cmdbuf[i+2] = (b << 2) | (g_lun & 0x03);
	 } else 
            cmdbuf[i+2] = b;
      }
      cmdlen = i + 2;
   } else {
      for (i = 0; i < maxlen; i++) {
         cmdbuf[i] = htoi(argv[i]);
      }
      cmdlen = i;
   } 

   if (fdebug) {
        printf("ipmi_cmd: ");
	for (i = 0; i < cmdlen; i++)
	    printf("%02x ",cmdbuf[i]);
        printf("\n");
   }

   if (is_remote() && fprivset == 0) { /*IPMI LAN, privilege not set by user*/
       /* commands to other MCs require admin privilege */
       // if ((g_sa != BMC_SA) || (cmdbuf[1] != BMC_SA))
	   parse_lan_options('V',"4",0);
   }

   if (!fskipdevid) {
    /* 
     * Check the Device ID to determine which bus id to use.
     */
    ret = ipmi_getdeviceid(devrec,16,fdebug);
    if (ret != 0) {
	goto do_exit;
    } else {
      uchar b, j;
      char *pstr;

      if (fdebug) {
	printf("devid: ");
	for (j = 0; j < 16; j++) printf("%02x ",devrec[j]);
	printf("\n");
      }
      b = devrec[4] & 0x0f;
      j = devrec[4] >> 4;
      prod_id = devrec[9] + (devrec[10] << 8);
      vend_id = devrec[6] + (devrec[7] << 8) + (devrec[8] << 16);
      if (vend_id == VENDOR_NSC) {         /* NSC = 0x000322 */
	  fmBMC = 1;  /*NSC miniBMC*/
      } else if (vend_id == VENDOR_INTEL) {  /* Intel = 0x000157 */
         switch(prod_id) {
            case 0x4311:  /* Intel NSI2U*/
 	      fmBMC = 1;  /* Intel miniBMC*/
              break;
            case 0x003E:  /* NSN2U or  CG2100*/
	      set_max_kcs_loops(URNLOOPS); /*longer for cmds (default 300)*/
              break;
	    default:
              break;
	 }
      }
      if (fmBMC) {  /*NSC mini-BMC*/
	  pstr = "mBMC";
	  busid = PERIPHERAL_BUS_ID;  /*used by alarms MWR*/
      } else {  /* treat like Intel Sahalee = 57 01 */
	  pstr = "BMC";
	  busid = PRIVATE_BUS_ID;     /*used by alarms MWR*/
      }
      printf("-- %s version %x.%x, IPMI version %d.%d \n",
             pstr, devrec[2],  devrec[3], b, j);
    }
    // ret = ipmi_getpicmg( devrec, sizeof(devrec),fdebug);
    // if (ret == 0) fpicmg = 1;
   } /*endif skip devid*/

   if (fjustpass) cmdmin = 2;
   else cmdmin = 4;
   if (cmdlen < cmdmin) {
	printf("command length (%d) is too short\n",cmdlen);      
	printf(usagemsg, progname);
   } else {
	ret = send_icmd(cmdbuf,cmdlen); 
        if (!fquiet) printf("send_icmd ret = %d\n",ret); 
   }
do_exit:
   ipmi_close_();
   // if (!fquiet) show_outcome(progname,ret); 
   return (ret);
}  /* end main()*/

/* end icmd.c */
