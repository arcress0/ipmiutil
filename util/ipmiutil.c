/***********************************************
 * ipmiutil.c 
 *
 * This is a meta-command utility to invoke each of the
 * other sub-commands in a consolidated interface.
 * To build this, compile with -DMETACOMMAND.
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2006-2007 Intel Corporation.
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 01/03/07 ARCress - created
 * 01/05/07 ARCress - version 1.0
 * 01/10/07 ARCress - version 1.1
 * 02/07/07 ARCress - version 1.3 adding isolconsole
 * 02/26/07 ARCress - updated sub-command names
 * 08/31/07 ARCress - added "leds" subcommand
 *
 ***********************************************/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2007, Intel Corporation
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
 *----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "ipmicmd.h"
#include "ipmiutil.h"

static char *progname  = "ipmiutil";
char *progver   = "3.18";
// static char fdebug = 0;
/*int ipmiutil(int argc, char **argv); */

#define NSUBCMDS  29
static struct {
  int idx;
  char tag[16];
  int (*rtn)(int argc, char **argv);
  char desc[64];
  } subcmds[NSUBCMDS] = {
 {  0, "alarms", i_alarms,  "show/set the front panel alarm LEDs and relays" },
 {  1, "leds",   i_alarms,  "show/set the front panel alarm LEDs and relays" },
 {  2, "discover", i_discover, "discover all IPMI servers on this LAN" },
 {  3, "cmd",    i_cmd ,    "send a specified raw IPMI command to the BMC" },
 {  4, "config", i_config,  "list/save/restore BMC configuration parameters" },
 { 26, "dcmi",   i_dcmi,    "get/set DCMI parameters" },
 {  5, "ekanalyzer", i_ekanalyzer, "run EKeying analyzer on FRU files (deprecated, see fru)" },
 {  6, "events", i_events,  "decode IPMI events and display them" },
 {  7, "firewall",   i_firewall, "show/set firmware firewall functions" },
 {  8, "fru",    i_fru,     "show decoded FRU inventory data, write asset tag"},
 {  9, "fwum",   i_fwum,    "OEM firmware update manager extensions" },
 { 10, "getevt", i_getevt,   "get IPMI events and display them, event daemon" },
 { 11, "getevent", i_getevt, "get IPMI events and display them, event daemon" },
 { 12, "health", i_health,  "check and show the basic health of the IPMI BMC"},
 { 13, "hpm",    i_hpm,     "HPM firmware update manager extensions" },
 { 14, "lan",    i_lan,     "show/set IPMI LAN parameters, users, PEF rules"},
 { 15, "picmg",  i_picmg,   "show/set picmg extended functions" },
 { 25, "power",  i_reset,   "issue IPMI reset or power control to the system"},
 { 16, "reset",  i_reset,   "issue IPMI reset or power control to the system"}, 
 { 17, "sel",    i_sel,     "show/clear firmware System Event Log records" },
 { 18, "sensor", i_sensor,  "show Sensor Data Records, readings, thresholds" },
 { 19, "serial", i_serial,  "show/set IPMI Serial & Terminal Mode parameters"},
 { 20, "sol",    i_sol,     "start/stop an SOL console session" },
 { 21, "smcoem", i_smcoem,  "SuperMicro OEM functions" },
 { 22, "sunoem", i_sunoem,  "Sun OEM functions" },
 { 23, "delloem",i_delloem, "Dell OEM functions" },
 { 24, "tsol",   i_tsol,    "Tyan SOL console start/stop session" },
 { 28, "user",   i_user,    "list or modify IPMI LAN users" },
 { 27, "wdt",    i_wdt,     "show/set/reset the watchdog timer" }
 };

static char usagemsg[] = "Usage: ipmiutil <command> [other options]\n"
                         "   where <command> is one of the following:\n";
static char helpmsg[]  = "For help on each command (e.g. 'sel'), enter:\n"
			 "   ipmiutil sel -?\n";

static void show_usage()
{
    int i;
    printf("%s", usagemsg);
    for (i=0; i<NSUBCMDS; i++)
       printf("\t%s\t%s\n",subcmds[i].tag,subcmds[i].desc);
    printf("   common IPMI LAN options:\n");
    print_lan_opt_usage(0);
    printf("%s", helpmsg);
}

#ifdef DOS
int i_discover(int argc, char **argv)
{
    printf("The discover function is not supported in DOS.\n");
    return(1);
}
#endif

#ifndef _WINDLL
/* omit main if compiled as a Windows DLL */
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
{
   int ret = 1;
   int i;
   char *psubcmd = "";
   
   if (argc < 2) {
      printf("%s ver %s\n", progname,progver); 
      show_usage();
      ret = ERR_USAGE;
      goto do_exit;
   }
#ifdef TEST_LOOP
   /* special subcommand processing loop for testing */
#ifdef WIN32
   while ( !(( _kbhit() ) &&  (_getch() == 'q')) )
#else
   while ( 1 )
#endif
   {
        for (i = 0; i < NSUBCMDS; i++)
        {
            if (strcmp(argv[1],subcmds[i].tag) == 0) {
                psubcmd = argv[1];
                argc--;
                argv++;
                ret = subcmds[i].rtn(argc,argv);

                argc++; argv--;  /*requeue the same subcmd*/
                os_usleep( 1, 0 ); /*sleep 1 sec*/
                break;
            }
        }
   }
#else
   for (i = 0; i < NSUBCMDS; i++)
   {
       if (strcmp(argv[1],subcmds[i].tag) == 0) {
          psubcmd = argv[1];
          argc--;
          argv++;
          ret = subcmds[i].rtn(argc,argv);
	  break;
       }
   }
#endif
   if (i >= NSUBCMDS) {
#ifdef LINUX
      if ((strcmp(argv[1],"svc") == 0) && (argc >= 3)) {
	 char mycmd[80];
	 char *pfunc;
	 char *psvc;
	 char fchkok;
	 /* undocumented: start a given service, only works locally */
	 psvc = "ipmi_port";
	 if (argc > 2) pfunc = argv[2];
	 else pfunc = "on";
	 ret = system("ls /sbin/chkconfig >/dev/null 2>&1");
	 if (ret == 0) fchkok = 1;
	 else fchkok = 0;
 	 if (strcmp(pfunc,"off") == 0) {
	    sprintf(mycmd,"service %s stop\n",psvc);
	    printf("%s\n",mycmd);
	    ret = system(mycmd);
	    if (fchkok) {
	       sprintf(mycmd,"/sbin/chkconfig --del %s\n",psvc);
	       printf("%s\n",mycmd);
	       ret = system(mycmd);
	    }
	 } else {
	    if (fchkok) {
		sprintf(mycmd,"/sbin/chkconfig --add %s\n",psvc);
		printf("%s\n",mycmd);
		ret = system(mycmd);
		sprintf(mycmd,"/sbin/chkconfig --level 345 %s on\n",psvc);
		// printf("%s\n",mycmd);
		ret = system(mycmd);
	    }
	    sprintf(mycmd,"service %s start\n",psvc);
	    printf("%s\n",mycmd);
	    ret = system(mycmd);
	 }
      } else 
#endif
      {
         printf("%s ver %s\n", progname,progver); 
         show_usage();
         ret = ERR_USAGE;
      }
   } 

do_exit:
   {
      char tag[30];
      sprintf(tag,"%s %s",progname,psubcmd);
      show_outcome(tag,ret); 
   }
   return(ret);
}
#endif

/*end ipmiutil.c*/
