/*
 * ipmi_sample_evt.c
 *
 * A sample IPMI utility to get IPMI SEL events.
 *
 * 09/10/12 Andy Cress - created 
 */
/*M*
Copyright (c) 2012, Kontron America, Inc.
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
#include <stdarg.h>
#include "getopt.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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
 
extern int  decode_sel_entry(uchar *evt, char *obuf, int sz); /*see ievents.c*/
extern void set_sel_opts(int sensdesc, int canon, void *sdrs, char dbg, char u);
extern char *get_sensor_type_desc(uchar stype);  /*see ievents.c*/
extern int  get_sdr_cache(uchar **pret);  /*see isensor.c*/
extern void free_sdr_cache(uchar *pret);  /*see isensor.c*/
extern int  write_syslog(char *msg);      /*see isel.c*/

/*
 * Global variables 
 */
static char * progname  = "ipmi_sample_evt";
static char * progver   = "1.0";
static char   fdebug    = 0;
static char   fset_mc   = 0;
static char   fipmilan  = 0;
static char   finit_ok  = 0;
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;
static char  *mytag = NULL;
static char  *sdrfile = NULL;
static ushort sel_recid = 0;
static uint   sel_time  = 0;
static int    wait_interval = 3; /* 3 seconds between calls */
static uchar *sdrs = NULL;
static int drvtype = 0;  /* driver_type from ipmicmd.h: 3=MV_OpenIPMI */
FILE  *fdout = NULL;
#define LAST_REC  0xFFFF
#ifdef WIN32
static char idxfile[80] = "c:\\ipmievt.idx";
static char outfile[80] = "c:\\ipmievt.log";
#else
static char idxfile[80] = "/var/lib/ipmiutil/ipmievt.idx";
static char outfile[80] = "/var/log/ipmievt.log";
#endif

/* These routines were copied from igetevent.c 
 * msgout, getevent_sel, syncevent_sel, show_event, ievt_cleanup
 */
static void ievt_cleanup(void);
static void msgout(char *pattn, ...)
{
    va_list arglist;

    if (fdout == NULL) return;
    va_start( arglist, pattn );
    vfprintf( fdout, pattn, arglist );
    va_end( arglist );
    fflush( fdout );
}

#if defined(WIN32) | defined(DOS)
/* no daemon code */
static void ievt_siginit(void) { return; }
#else
/* Linux daemon code */
#include <signal.h>
static void ievt_sighnd(int sig)
{
   ievt_cleanup();
   exit(EXIT_SUCCESS);
}

static void ievt_siginit(void);
static void ievt_siginit(void)
{
   struct sigaction sact;
 
   /* handle signals for cleanup */
   sact.sa_handler = ievt_sighnd;
   sact.sa_flags = 0;
   sigemptyset(&sact.sa_mask);
   sigaction(SIGINT, &sact, NULL);
   sigaction(SIGQUIT, &sact, NULL);
   sigaction(SIGTERM, &sact, NULL);
}
#endif

static int get_sel_entry(ushort recid, ushort *nextid, uchar *rec)
{
   uchar ibuf[6];
   uchar rbuf[32];
   int rlen;
   ushort xid, id = 0;
   uchar cc;
   int rv; 

   ibuf[0] = 0;
   ibuf[1] = 0;
   ibuf[2] = (recid & 0x00ff);
   ibuf[3] = (recid & 0xff00) >> 8;
   ibuf[4] = 0;
   ibuf[5] = 0xFF;  /*get entire record*/
   rlen = sizeof(rbuf);
   rv = ipmi_cmd(GET_SEL_ENTRY, ibuf, 6, rbuf, &rlen, &cc, fdebug);
   if (rv == 0) {
       if (cc != 0) rv = cc;
       else {  /*success*/
          xid = rbuf[0] + (rbuf[1] << 8);  /*next rec id*/
          memcpy(rec,&rbuf[2],16);
          *nextid = xid;
          id = rbuf[2] + (rbuf[3] << 8);  /*curr rec id*/
	  /* recid (requested) should match newid (received) */
          if (fdebug) {
            if ((recid != id) && (recid != LAST_REC) && (recid != 0)) {
              /* the OpenIPMI driver does this sometimes */
              msgout("get_sel MISMATCH: recid=%x newid=%x next=%x\n",
	   		      recid,id,xid);
              dump_buf("get_sel cmd",ibuf,6,0);
              dump_buf("get_sel rsp",rbuf,rlen,0);
            }
          }
       }
   }
   if (fdebug) msgout("get_sel(%x) rv=%d cc=%x id=%x next=%x\n",
			recid,rv,cc,id,*nextid);
   return(rv);
}

static int getevent_sel(uchar *rdata, int *rlen, uchar *ccode)
{
    uchar rec[24];
    int rv = 0;
    ushort newid;
    ushort nextid;
    ushort recid;
    
    /* get current last record */
    recid = sel_recid;
    rv = get_sel_entry(recid,&nextid,rec);
    if (rv == 0xCB && recid == 0) {  /* SEL is empty */
        *ccode = (uchar)rv;  /* save the real ccode */
        rv = 0x80;    /* this is ok, just keep waiting  */
    }
    if (rv == 0) {
       if (fdebug) msgout("sel ok, id=%x next=%x\n",recid,nextid);
       if ((nextid == LAST_REC) || (recid == nextid)) { 
           *ccode = 0x80;  /*nothing new*/
       } else {
         recid = nextid;  /* else get new one */
         rv = get_sel_entry(recid,&nextid,rec);
         if (rv == 0) {  /* new event */ 
            newid = rec[0] + (rec[1] << 8);
            if (drvtype == DRV_MV && recid != newid) {  
               /* handle MV driver bug, try to get next one. */
               if (fdebug) msgout("%s bug, record mismatch\n",
				   show_driver_type(DRV_MV));
            }
            if (fdebug) msgout("recid=%x newid=%x next=%x\n",
	   		       recid,newid,nextid);
            memcpy(rdata,rec,16);
            *rlen = 16;
            *ccode = 0;
            sel_recid = recid;  /*or newid*/
            memcpy(&sel_time,&rec[2],4);
         }
       }
    }
    else {  /* Error reading last recid saved */
       if (fdebug) msgout("sel recid %x error, rv = %d\n",recid,rv);
       /* We want to set sel_recid = 0 here for some errors. */
       if (rv == 0xCB || rv == 0xCD) { /* empty, or wrong SDR id */
	  sel_recid = 0;
          *ccode = (uchar)rv;
          rv = 0x80; /* wait again */
       }
    }
    return(rv); 
}

static int startevent_sel(ushort *precid, uint *ptime) 
{
    FILE *fd;
    uchar rec[24];
    uint t = 0;
    ushort r = 0;
    ushort r2 = 0;
    int rv = -1;

    fd = fopen(idxfile,"r");
    if (fdebug) msgout("start: idxfile=%s fd=%p\n",idxfile,fd);
    if (fd != NULL) {
        // Read the file, get savtime & savid
        rv = fscanf(fd,"%x %x",&t,&r);
        fclose(fd);
        if (r == LAST_REC) r = 0;
	rv = 0; /*read it, success*/
    } else {  /* treat as first time */
        r = LAST_REC;
        rv = get_sel_entry(r,&r2,rec);
        if (rv == 0) {
            memcpy(&t,&rec[2],4);
            r = rec[0] + (rec[1] << 8); /*use current rec id*/
        } else r = 0;
	rv = 1;  /*first time*/
    }
    if (fdebug) msgout("start: recid=%x time=%x\n",r,t);
    *ptime  = t;
    *precid = r;
    return(rv);
}

static int syncevent_sel(ushort recid, uint itime)
{
    FILE *fd;
    int rv;
    // Rewrite the saved time & record id
    if (fdebug) msgout("sync: recid=%x time=%x\n",recid,itime);
    fd = fopen(idxfile,"w");
    if (fd == NULL) {
	msgout("syncevent: cannot open %s for writing\n",idxfile);
	rv = -1;
    } else {
        fprintf(fd,"%x %x\n",itime,recid);
        fclose(fd);
	rv = 0;
    }
    return(rv);
}

static void ievt_cleanup(void)
{
   char obuf[48];
   snprintf(obuf,sizeof(obuf),"%s exiting.\n",progname);
   msgout(obuf);
   write_syslog(obuf);
   if (finit_ok) {
      syncevent_sel(sel_recid,sel_time);
      free_sdr_cache(sdrs);
   }
   ipmi_close_();   /*inert if not opened*/
   exit(EXIT_SUCCESS);
}

void show_event(uchar *evt,char *obuf, int sz)
{
   int i;
   char sysbuf[250];
   /* obuf should be 132 chars or more */

   msgout("event data: ");
   for (i=0; i<16; i++) msgout("%02x ",evt[i]);
   msgout("\n");

   decode_sel_entry(evt,obuf,sz);
   msgout(obuf);  /*writes to outfile*/
   /* write the message to syslog also. */
   snprintf(sysbuf,sizeof(sysbuf),"%s: %s",progname,obuf);
   write_syslog(sysbuf);
}

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
   uchar event[32]; /*usu 16 bytes */
   char outbuf[160];
   char *s1;
   int i, rlen;
   uchar ccode;
   uchar sensor_type;

   fdout = stderr;
   printf("%s ver %s\n", progname,progver);

   while ( (c = getopt( argc, argv,"m:p:s:t:xEF:N:P:R:T:U:V:YZ:?")) != EOF ) 
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
                break;
	  default:
                printf("Usage: %s [-msx -NUPREFTVY]\n", progname);
                printf(" where -x       show eXtra debug messages\n");
		printf("       -m002000 specific MC (bus 00,sa 20,lun 00)\n");
                printf("       -s File  loads SDRs from File\n");
                printf("       -t tag   search for 'tag' in SDRs\n");
		print_lan_opt_usage(1);
                exit(1);
      }
   /* Rather than parse_lan_options above, the set_lan_options function 
    * could be used if the program already knows the nodename, etc. */

   fipmilan = is_remote();
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
   drvtype = get_driver_type();

   /* could make it a daemon here, if desired */
   fdout = fopen(outfile,"a");
   if (fdout == NULL) {
        printf("%s: Cannot open %s\n", progname,outfile);
	fdout = stderr;
   } else {
	sprintf(outbuf,"%s ver %s started\n", progname,progver);
	msgout(outbuf);
   }
   sprintf(outbuf,"%s reading sensors ...\n",progname);
   write_syslog(outbuf);
   ret = get_sdr_cache(&sdrs);
   if (fdebug) msgout("get_sdr_cache ret = %d\n",ret);
   if (ret == 0) set_sel_opts(1,0, sdrs,fdebug,0);

   if (fipmilan) {
          char *node;
          node = get_nodename();
          strcat(idxfile,"-");
          strcat(idxfile,node);
          strcat(outfile,"-");
          strcat(outfile,node);
   }
   ret = startevent_sel(&sel_recid,&sel_time);

   ievt_siginit();
   finit_ok = 1;
   ret = 0;  /*ignore any earlier errors, keep going*/
   i = 0;

   while (ret == 0) 
   {            /*wait for bmc message events*/
	 if (fdebug) 
            msgout("%s: %d Polling every %d sec for a new event after id %x\n",
			progname, i, wait_interval, sel_recid);
	 rlen = sizeof(event);
         ret = getevent_sel(event,&rlen,&ccode);
         if (ret == 0) ret = ccode;
         if (fdebug) msgout("getevent_sel ret = %d\n",ret);
         if (ret == 0) { /* got an event successfully */
	    sensor_type = event[10];
            msgout("got event id %04x, sensor_type = %02x\n",
			sel_recid,sensor_type);
	    show_event(event,outbuf,sizeof(outbuf));
            syncevent_sel(sel_recid,sel_time);
         } else if (ret == 0x80) {
	    ret = 0;  /*keep waiting*/
         } else {
            msgout("get_event error: ret = 0x%x\n",ret);
	    break;
         }
	 i++;
	 os_usleep(wait_interval,0); 
   }  /*end while loop*/

do_exit:
   ievt_cleanup();
   show_outcome(progname,ret);
   exit (ret);
}  /* end main()*/

/* end ipmi_sample_evt.c */
