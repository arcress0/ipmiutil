/*
 * iuser.c
 * Handle IPMI user command functions
 *
 * Change history:
 *  03/22/2017 ARCress - included in source tree
 *
 *----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2017, Andy Cress. All rights reserved.

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
#ifdef WIN32
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "getopt.h"
#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#else
#include <getopt.h>
#endif
#endif
#include "ipmicmd.h"

/* global variables */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil user";
#else
static char * progver   = "3.08";
static char * progname  = "iuser";
#endif
static char   fdebug    = 0;
static char   fcanonical = 0;
static char   bdelim = '|';
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;
extern int verbose;  /*see ipmilanplus.c*/

extern void lprintf(int level, const char * format, ...);  /*ipmilanplus.c*/

/* from ilan.c */
char *parse_priv(uchar c);
int DisableUser (int unum, uchar chan);
int SetPasswd(int unum, char *uname, char *upswd, uchar chan, uchar priv);
int GetBmcEthDevice(uchar lan_parm, uchar *pchan);
int GetUserInfo(uchar unum, uchar chan, uchar *enab, uchar *priv, char *uname, char fdebug);

static void printf_user_usage(void)
{
	printf("ipmiutil user Commands:\n");
	printf("\tuser list [channel]\n");
	printf("\tuser enable <user_num> [channel]\n");
	printf("\tuser disable <user_num>\n");
	printf("\tuser set <user_num> name <username> \n");
	printf("\tuser set <user_num> password <password> \n");
	printf("\tuser set <user_num> priv <priv_level> [channel]\n");
	printf("\t\twhere priv_level: 4=Admin, 3=Operator, 2=User\n");
}

/* ipmi_user_list - print out info for users
 *
 * @intf:	ipmi inteface
 * @argc:	argument count
 * @argv:	argument list
 * returns 0 on success, -1 on error
 */
static int ipmi_user_list(void * intf, int  argc, char ** argv)
{
	int i,ret = 0;
	int rv = -1;
	uchar num = 1;
	uchar chan = 1;
	uchar enab, priv;
	char uname[21];
	char *privstr;

	if (argv[0] != NULL) chan = atoi(argv[0]);
	else i = GetBmcEthDevice(0xFF, &chan);
	while (ret == 0) {
		uname[0] = '\0';
		ret = GetUserInfo(num, chan, &enab, &priv, uname, fdebug);
		if (ret == 0) {
			privstr = parse_priv(priv);
			if (fcanonical) 
				printf("User %2d %c chan=%d %c %8s %c %8s %c %s\n",
						num,bdelim,chan,bdelim,(enab? "enabled": "disabled"),
						bdelim,privstr,bdelim,uname);
			else
				printf("User %2d: chan=%d \t%8s \t%8s \t%s\n",
						num,chan,(enab? "enabled": "disabled"),privstr,uname);
			rv = 0;
		}
		num++;
	}
	return(rv);
}

/* ipmi_user_enable_disable  -  enable/disable BMC functions
 *
 * @intf:	ipmi inteface
 * @enable:     whether to enable or disable
 * @argc:	argument count
 * @argv:	argument list
 *
 * returns 0 on success, -1 on error
 */
static int
ipmi_user_enable_disable(void * intf, int enable, int  argc, char ** argv)
{
	int i, ret = -1;
	uchar unum, chan, enab;
	uchar priv = 3;
	char uname[21] = {'\0'};

	if (argc < 1 || strncmp(argv[0], "help", 4) == 0) {
		printf_user_usage();
		return ERR_USAGE;
	}
	if (argv[0] == NULL) { printf_user_usage(); return ERR_USAGE; }
	unum = atoi(argv[0]);

	if (argv[1] != NULL) chan = atoi(argv[1]);
	else i = GetBmcEthDevice(0xFF, &chan);

	if (enable == 0) {
	  ret = DisableUser(unum, chan);
	} else {
	  ret = GetUserInfo(unum, chan, &enab, &priv, uname, fdebug);
	  ret = SetPasswd(unum, uname, NULL, chan, priv);
	}
	return ret;
}

/* ipmi_user_reset - reset firmware user to enable everything
 *
 * @intf:	ipmi inteface
 * @argc:	argument count
 * @argv:	argument list
 *
 * returns 0 on success, -1 on error
 */
static int
ipmi_user_set(void * intf, int  argc, char ** argv)
{
	int ret = -1;
	int i, func = -1;
	char *uname = NULL;
	char *upass = NULL;
	uchar unum, chan, enab, dummy;
	uchar priv = 3;
	char name[21] = {'\0'};

	if ((argc < 2) || (argc > 0 && strncmp(argv[0], "help", 4) == 0)) {
		printf_user_usage();
		return ERR_USAGE;
	}
	if ((argv[1] == NULL) || (argv[2] == NULL)) { 
		printf_user_usage(); 
		return ERR_USAGE; 
	}
	unum = atoi(argv[0]);
	if (strncmp(argv[1], "name", 4) == 0) {
	    func = 1; 
		uname = argv[2];
	} else if (strncmp(argv[1], "password", 8) == 0) {
		func = 2; 
		upass = argv[2];
	} else if (strncmp(argv[1], "priv", 4) == 0) {
	    func = 3; 
		priv = atoi(argv[2]);
	} else {
		printf_user_usage();
		return(ERR_USAGE);
	}
	if (argv[3] != NULL) chan = atoi(argv[3]);
	else i = GetBmcEthDevice(0xFF, &chan);

	switch(func) {
    case 1:  /*set name*/ 
	  	ret = GetUserInfo(unum, chan, &enab, &priv, name, fdebug);
		ret = SetPasswd(unum, uname, upass, chan, priv);
		break;
    case 2:  /*set password*/ 
	  	ret = GetUserInfo(unum, chan, &enab, &priv, name, fdebug);
		if (ret == 0) uname = name;
		ret = SetPasswd(unum, uname, upass, chan, priv);
		break;
    case 3:  /*set priv*/ 
	  	ret = GetUserInfo(unum, chan, &enab, &dummy, name, fdebug);
		if (ret == 0) uname = name;
		ret = SetPasswd(unum, uname, upass, chan, priv);
		break;
	}
	return ret;
}

#ifdef METACOMMAND
int i_user(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
#endif
{
	void *intf = NULL;
	int rc = 0;
	int c, i;
	char *s1;

	printf("%s ver %s\n", progname,progver);

	parse_lan_options('V',"4",0);  /*default to admin priv*/

    while ( (c = getopt( argc, argv,"cm:T:V:J:EYF:P:N:R:U:Z:x?")) != EOF )
	switch (c) {
          case 'c': fcanonical = 1; break;
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
          case 'x': fdebug = 1; verbose = 1;
			break;  /* debug messages */
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
				print_lan_opt_usage(0);
				printf_user_usage();
				return ERR_USAGE;
                break;
	}
    for (i = 0; i < optind; i++) { argv++; argc--; }

	if (argc < 1 || strncmp(argv[0], "help", 4) == 0) {
		printf_user_usage();
	} else if (strncmp(argv[0], "list", 4) == 0) {
		rc = ipmi_user_list(intf, argc-1, &(argv[1]));
	} else if (strncmp(argv[0], "enable", 6) == 0) {
		rc = ipmi_user_enable_disable(intf, 1, argc-1, &(argv[1]));
	} else if (strncmp(argv[0], "disable", 7) == 0) {
		rc = ipmi_user_enable_disable(intf, 0, argc-1, &(argv[1]));
	} else if (strncmp(argv[0], "set", 3) == 0) {
		rc = ipmi_user_set(intf, argc-1, &(argv[1]));
	} else {
		printf_user_usage();
		rc = ERR_BAD_PARAM;
	}
    ipmi_close_();
	return rc;
}
