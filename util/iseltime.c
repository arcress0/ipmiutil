/*
 * iseltime.c
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 *
 * 05/11/09 Andy Cress v1.0 - created
 * 07/23/10 Andy Cress v1.1 - always show System time also
 * 08/20/10 Andy Cress v1.2 - show/set RTC time also if Linux
 * 03/16/17 Andy Cress 3.03 - show/set UTC offset also
 */
/*M*
Copyright (c) 2013, Andy Cress
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 

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
#include <time.h>
#include <string.h>
#include "ipmicmd.h"
 
#define GET_SELTIME            0x48
#define SET_SELTIME            0x49
#define GET_SELUTC             0x5C
#define SET_SELUTC             0x5D
/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil seltime";
#else
static char * progver   = "3.08";
static char * progname  = "iseltime";
#endif
static char   fdebug    = 0;
static char   fset      = 0;
static uchar ipmi_maj = 0;
static uchar ipmi_min = 0;

static int get_sel_utc(uchar *rdata, int rlen)
{
	uchar idata[4];
	uchar ccode;
	int ret;
	ret = ipmi_cmdraw(GET_SELUTC, NETFN_STOR, BMC_SA, PUBLIC_BUS, BMC_LUN,
			  idata, 0, rdata, &rlen, &ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end get_sel_utc()*/

static int set_sel_utc(short offset)
{
	uchar idata[4];
	uchar rdata[16];
	int rlen = 8;
	int ret;
	uchar ccode;

	idata[0] = (uchar)(offset & 0x00ff);
	idata[1] = (uchar)((offset >> 8) & 0x00ff);
	ret = ipmi_cmdraw(SET_SELUTC, NETFN_STOR, BMC_SA, PUBLIC_BUS, BMC_LUN,
			  idata, 2, rdata, &rlen, &ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end set_sel_utc()*/

static int get_sel_time(uchar *rdata, int rlen)
{
	uchar idata[4];
	uchar ccode;
	int ret;
	ret = ipmi_cmdraw(GET_SELTIME, NETFN_STOR, BMC_SA, PUBLIC_BUS, BMC_LUN,
			  idata, 0, rdata, &rlen, &ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end get_sel_time()*/

static int set_sel_time(time_t newtime)
{
	uchar idata[4];
	uchar rdata[16];
	int rlen = 8;
	int ret;
	uchar ccode;

	idata[0] = (uchar)(newtime & 0x00ff);
	idata[1] = (uchar)((newtime >> 8) & 0x00ff);
	idata[2] = (uchar)((newtime >> 16) & 0x00ff);
	idata[3] = (uchar)((newtime >> 24) & 0x00ff);
	ret = ipmi_cmdraw(SET_SELTIME, NETFN_STOR, BMC_SA, PUBLIC_BUS, BMC_LUN,
			  idata, 4, rdata, &rlen, &ccode, fdebug);
	if (ret == 0 && ccode != 0) ret = ccode;
	return(ret);
}  /*end set_sel_time()*/

time_t utc2local(time_t t)
{
   struct tm * tm_tmp;
   int gt_year,gt_yday,gt_hour,lt_year,lt_yday,lt_hour;
   int delta_hour;
   time_t lt;

   //modify UTC time to local time expressed in number of seconds from 1/1/70 0:0:0 1970 GMT
   // check for dst?
   tm_tmp=gmtime(&t);
   gt_year=tm_tmp->tm_year;
   gt_yday=tm_tmp->tm_yday;
   gt_hour=tm_tmp->tm_hour;
   tm_tmp=localtime(&t);
   lt_year=tm_tmp->tm_year;
   lt_yday=tm_tmp->tm_yday;
   lt_hour=tm_tmp->tm_hour;
   delta_hour=lt_hour - gt_hour;
   if ( (lt_year > gt_year) || ((lt_year == gt_year) && (lt_yday > gt_yday)) )
           delta_hour += 24;
   if ( (lt_year < gt_year) || ((lt_year == gt_year) && (lt_yday < gt_yday)) )
           delta_hour -= 24;
   if (fdebug) printf("utc2local: delta_hour = %d\n",delta_hour);
   lt = t + (delta_hour * 60 * 60);
   return(lt);
}

#define TIMESTR_SZ  30
void show_time(time_t etime, int doutc)
{
	char buf[TIMESTR_SZ];
	int bufsz = TIMESTR_SZ;
	time_t t;

	strcpy(buf,"00/00/00 00:00:00");
	if (doutc) t = utc2local(etime);
	else t = etime;
	strftime(buf,bufsz, "%x %H:%M:%S", gmtime(&t)); /*or "%x %T"*/
	printf("%s\n",buf);
	return;
}

#ifdef METACOMMAND
int i_iseltime(int argc, char **argv)
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
   uchar devrec[20];
   uchar timebuf[4];
   time_t ltime1, ltime2, ltime3;
   short offset;
   int c;

#if defined (EFI)
        InitializeLib(_LIBC_EFIImageHandle, _LIBC_EFISystemTable);
#else
        printf("%s ver %s\n", progname,progver);
#endif

   while ( (c = getopt( argc, argv,"sT:V:J:EYF:P:N:R:U:x?")) != EOF ) 
      switch(c) {
          case 's': fset  = 1;   break;  /* read only */
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
                parse_lan_options(c,optarg,fdebug);
                break;
	  default:
                printf("Usage: %s [-sx -NUPRETVF]\n", progname);
                printf(" where -s      Set SEL time (usually once a day)\n");
                printf("       -x      show eXtra debug messages\n");
		print_lan_opt_usage(0);
                exit(1);
      }

   rv = ipmi_getdeviceid(devrec,16,fdebug);
   if (rv != 0) {
        show_outcome(progname,rv); 
	ipmi_close_();
	exit(rv);
   } else {
      ipmi_maj = devrec[4] & 0x0f;
      ipmi_min = devrec[4] >> 4;
#ifndef EFI
      printf("-- BMC version %x.%x, IPMI version %d.%d \n",
             devrec[2],  devrec[3], ipmi_maj, ipmi_min);
#endif
   }

   offset = 0;
   rv = get_sel_utc((uchar *)&offset,2);
   if (fdebug) printf("get_sel_utc: ret=%x offset=%d\n",rv,offset);
   if (rv == 0) {
      /* TODO: get system UTC offset */
      rv = set_sel_utc(offset);
      printf("set_sel_utc(%d): ret=%x\n",offset,rv);
   }
   rv = get_sel_time(&timebuf[0],4);
   if (rv != 0) {
	  printf("get_sel_time error: ret = %x\n",rv);
	  ipmi_close_();
	  exit(1);
   }
   time(&ltime2);
   printf("Current System time: "); show_time(ltime2,1);
   ltime1 = timebuf[0] + (timebuf[1] << 8) + (timebuf[2] << 16) + 
		(timebuf[3] << 24);
   printf("Current SEL time:    "); show_time(ltime1,0);
   
   // if (fdebug) ltime3 = utc2local(ltime1); 

   if (fset == 1) {
      /* get current time */
      time(&ltime2);
      rv = set_sel_time(ltime2);
      printf("Setting SEL time to System Time: ret = %x\n",rv); 
      if (rv != 0) printf("set_sel_time error: ret = %x\n",rv);
      else {  /*successful*/
	rv = get_sel_time(timebuf,8);
	if (rv != 0) printf("get_sel_time error: ret = %x\n",rv);
	else {
	   ltime3 = timebuf[0] + (timebuf[1] << 8) + (timebuf[2] << 16) + 
			(timebuf[3] << 24);
	   printf("New SEL time:        "); show_time(ltime3,0);
	}
      }
   } 
#ifdef LINUX
   if (is_remote() == 0) {
      c = system("echo \"Current RTC time:    `hwclock`\"");
      if (fset == 1) {
         c = system("hwclock --systohc");
	 printf("Copying System Time to RTC: ret = %d\n",c); 
      }
   }
#endif
   ipmi_close_();
   show_outcome(progname,rv); 
   exit (rv);
}  /* end main()*/

/* end iseltime.c */
