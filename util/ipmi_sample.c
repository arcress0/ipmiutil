/*
 * ipmi_sample.c
 *
 * A sample IPMI utility, to which more commands can be added.
 *
 * 02/27/06 Andy Cress - created 
 * 02/25/11 Andy Cress - added get_chassis_status 
 */
/*M*
Copyright (c) 2007, Kontron America, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 
  c.. Neither the name of Kontron America, Inc. nor the names of its contributors 
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
#ifdef GET_SENSORS
/* need to also include isensor.o, ievents.o when linking. */
#include "isensor.h"
#endif
#ifdef GET_FRU
#include "ifru.h"
#endif
 
/*
 * Global variables 
 */
static char * progname  = "ipmi_sample";
static char * progver   = "1.2";
static char   fdebug    = 0;
static char   fset_mc  = 0;
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;
static char  *mytag = NULL;
static char  *sdrfile = NULL;

static int get_chassis_status(uchar *rdata, int rlen)
{
   uchar idata[4];
   uchar ccode;
   int ret;

   ret = ipmi_cmdraw( CHASSIS_STATUS, NETFN_CHAS, g_sa,g_bus,g_lun,
                        idata,0, rdata,&rlen,&ccode, fdebug);
   if (ret == 0 && ccode != 0) ret = ccode;
   return(ret);
}  /*end get_chassis_status()*/

#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
{
   int ret = 0;
   char c;
   uchar devrec[16];
   uchar chstatus[4];
   char *s1;
   int loops = 1;
   int nsec = 10;
   char *nodefile = NULL;
   int done = 0;
   FILE *fp = NULL;
   char nod[40]; char usr[24]; char psw[24];
   char drvtyp[10];
   char biosstr[40];
   int n;
#ifdef GET_SENSORS
   uchar *sdrlist;
#endif

   printf("%s ver %s\n", progname,progver);

   while ((c = getopt( argc, argv,"i:l:m:p:f:s:t:xEF:N:P:R:T:U:V:YZ:?")) != EOF ) 
      switch(c) {
          case 'm': /* specific IPMB MC, 3-byte address, e.g. "409600" */
                    g_bus = htoi(&optarg[0]);  /*bus/channel*/
                    g_sa  = htoi(&optarg[2]);  /*device slave address*/
                    g_lun = htoi(&optarg[4]);  /*LUN*/
                    g_addrtype = ADDR_IPMB;
                    if (optarg[6] == 's') {
                             g_addrtype = ADDR_SMI;  s1 = "SMI";
                    } else { g_addrtype = ADDR_IPMB; s1 = "IPMB"; }
                    fset_mc = 1;
                    ipmi_set_mc(g_bus,g_sa,g_lun,g_addrtype);
                    printf("Use MC at %s bus=%x sa=%x lun=%x\n",
                            s1,g_bus,g_sa,g_lun);
                    break;
          case 'f': nodefile = optarg; break; /* specific sensor tag */
          case 'l': loops = atoi(optarg); break; 
          case 'i': nsec = atoi(optarg); break;  /*interval in sec*/
          case 's': sdrfile = optarg; break; 
          case 't': mytag = optarg; break; /* specific sensor tag */
          case 'x': fdebug = 1;     break;  /* debug messages */
          case 'p':    /* port */
          case 'N':    /* nodename */
          case 'U':    /* remote username */
          case 'P':    /* remote password */
          case 'R':    /* remote password */
          case 'E':    /* get password from IPMI_PASSWORD environment var */
          case 'F':    /* force driver type */
          case 'T':    /* auth type */
          case 'V':    /* priv level */
          case 'Y':    /* prompt for remote password */
          case 'Z':    /* set local MC address */
                parse_lan_options(c,optarg,fdebug);
		if (c == 'F') strncpy(drvtyp,optarg,sizeof(drvtyp));
                break;
	  default:
                printf("Usage: %s [-filmstx -NUPREFTVY]\n", progname);
                printf(" where -x       show eXtra debug messages\n");
                printf("       -f File  use list of remote nodes from File\n");
                printf("       -i 10    interval for each loop in seconds\n");
                printf("       -l 10    loops sensor readings 10 times\n");
		printf("       -m002000 specific MC (bus 00,sa 20,lun 00)\n");
                printf("       -s File  loads SDRs from File\n");
                printf("       -t tag   search for 'tag' in SDRs\n");
		print_lan_opt_usage(1);
                exit(1);
      }
   /* Rather than parse_lan_options above, the set_lan_options function 
    * could be used if the program already knows the nodename, etc. */
 ret = get_BiosVersion(biosstr);
 if (ret == 0) printf("BIOS Version: %s\n",biosstr);

 while(!done)
 {
   if (nodefile != NULL) {
      /* This will loop for each node in the file if -f was used.
       * The file should contain one line per node: 
       *    node1 user1 password1
       *    node2 user2 password2
       */
      if (fp == NULL) {
         fp = fopen(nodefile,"r");
         if (fp == NULL) {
            printf("Cannot open file %s\n",nodefile);
            ret = ERR_FILE_OPEN;
	    goto do_exit;
         }
         if (fdebug) printf("opened file %s ok\n",nodefile);
      }
      n = fscanf(fp,"%s %s %s", nod, usr, psw);
      if (fdebug) printf("fscanf returned %d \n",n);
      if (n == EOF || n <= 0) {
        fclose(fp);
	done = 1;
	goto do_exit;
      }
      printf("Using -N %s -U %s -P %s ...\n",nod,usr,psw);
      if (n > 0) parse_lan_options('N',nod,0);
      if (n > 1) parse_lan_options('U',usr,0);
      if (n > 2) parse_lan_options('P',psw,0);
      if (drvtyp != NULL) parse_lan_options('F',drvtyp,0);
   }

   ret = ipmi_getdeviceid(devrec,16,fdebug);
   if (ret != 0) {
	printf("Cannot do ipmi_getdeviceid, ret = %d\n",ret);
	goto do_exit;
   } else {  /*success*/
       uchar ipmi_maj, ipmi_min;
       ipmi_maj = devrec[4] & 0x0f;
       ipmi_min = devrec[4] >> 4;
       show_devid( devrec[2],  devrec[3], ipmi_maj, ipmi_min);
   }

   ret = get_chassis_status(chstatus,4);
   if (ret == 0) {
        if (chstatus[0] & 0x01) s1 = "on";
        else s1 = "off";
	printf("Chassis Status = %02x (%s)\n",chstatus[0],s1);
   }

#ifdef GET_FRU
   {
      uchar  *fru_data = NULL;
      printf("Getting FRU ...\n");
      ret = load_fru(0x20,0,0x07, &fru_data);
      if (ret == 0) 
	 ret = show_fru(0x20,0,0x07,fru_data);
      if (fru_data != NULL)
         free_fru(fru_data);
   }
#endif
#ifdef GET_SENSORS
   printf("Getting SDRs ...\n");
   if (sdrfile != NULL) {
      ret = get_sdr_file(sdrfile,&sdrlist);
   } else {
      ret = get_sdr_cache(&sdrlist);
   }
   printf("get_sdr_cache ret = %d\n",ret);
   if (ret == 0) {
	uchar sdrbuf[SDR_SZ];
	uchar reading[4];
	uchar snum = 0;
	ushort id;
	double val;
	char *typestr;
	char tag[17];
	int j;

	for (j = 0; j < loops; j++) 
	{
	 if (j > 0) {
	    printf("loop %d: wait %d seconds ...\n",j,nsec);
	    os_usleep(nsec,0);  /*sleep 5 sec between loops*/
	 }
	 id = 0;
	 /* Get sensor readings for all full SDRs */
         while(find_sdr_next(sdrbuf,sdrlist,id) == 0) {
           id = sdrbuf[0] + (sdrbuf[1] << 8); /*this SDR id*/
	   if (sdrbuf[3] != 0x01) continue; /*only type 1 full SDRs*/
	   strncpy(tag,&sdrbuf[48],16);
	   tag[16] = 0;
	   snum = sdrbuf[7];
           ret = GetSensorReading(snum, sdrbuf, reading);
	   if (ret == 0) { 
	      val = RawToFloat(reading[0], sdrbuf);
              typestr = get_unit_type( sdrbuf[20], sdrbuf[21], sdrbuf[22],0);
	   } else {
	      val = 0;
	      typestr = "na";
	      printf("%04x: get sensor %x reading ret = %d\n",id,snum,ret);
	   }
           printf("%04x: sensor %x %s  \treading = %.2f %s\n",
			id,snum,tag,val,typestr);
	   memset(sdrbuf,0,SDR_SZ);
         } /*end while*/
        } /*end for(loops) */

	/* Find a specific sensor by its tag and get a reading */
	if (mytag != NULL) {
	   /* see option -t, mytag = "System";  or "System Temp" */
	   memset(sdrbuf,0,SDR_SZ);
           ret = find_sdr_by_tag(sdrbuf, sdrlist, mytag, fdebug);
           printf("find_sdr_by_tag(%s) ret = %d\n",mytag,ret);
	   strncpy(tag,&sdrbuf[48],16); /*assume full sdr tag offset*/
	   tag[16] = 0;
	   snum = sdrbuf[7];
           ret = GetSensorReading(snum, sdrbuf, reading);
           printf("get sensor %x reading ret = %d\n",snum,ret);
	   if (sdrbuf[3] == 0x01) { /*full SDR*/
	      if (ret == 0) { 
	         val = RawToFloat(reading[0], sdrbuf);
                 typestr = get_unit_type(sdrbuf[20],sdrbuf[21],sdrbuf[22],0);
	      } else {
	         val = 0;
	         typestr = "na";
	      }
              printf("sensor %x %s reading = %.2f %s\n",snum,tag,val,typestr);
	   } else printf("sensor %x type %x reading = %02x\n",
			snum,sdrbuf[3],reading[2]);
	}

	free_sdr_cache(sdrlist);
   } /*endif sdr_cache is valid*/
#endif
   ipmi_close_();
   if (nodefile == NULL) done = 1;
  } /*end while not done */

do_exit:
   show_outcome(progname,ret);
   exit (ret);
}  /* end main()*/

/* end ipmi_sample.c */
