/*
 * subs.c
 *
 * Some common helper subroutines
 * 
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2010 Kontron America, Inc.
 *
 * 08/18/11 Andy Cress - created to consolidate subroutines
 */
/*M*
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
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#ifdef WIN32
#include <windows.h>
// #if !defined(LONG_MAX)
// # if __WORDSIZE == 64
// #  define LONG_MAX     9223372036854775807L
// # else
// #  define LONG_MAX     2147483647L
// # endif
// # define LONG_MIN      (-LONG_MAX - 1L)
// #endif

#else
/* Linux */
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#endif

#include "ipmicmd.h"

extern char  fdebug;   /*ipmicmd.c*/
extern char  fdbglog;  /*ipmilanplus.c*/
extern int   verbose;  /*ipmilanplus.c*/
extern FILE *fpdbg;    /*ipmicmd.c*/
extern FILE *fplog;    /*ipmicmd.c */
extern char log_name[60];  /*log_name global, from ipmicmd.c*/

static int loglevel = LOG_WARN; 
#ifdef WIN32
#define SELMSG_ID  0x40000101   /* showselmsg.dll EventID 257. = 0x101 */
static HANDLE hLog = NULL;
#endif

/* decode_rv, decode_cc are in ipmicmd.c */

/* strlen_ wrapper for size_t/int warnings*/
int strlen_(const char *s)
{
   return((int)strlen(s));
}

/* Need our own copy of strdup(), named strdup_(), since Windows does
 * not have the same subroutine. */
char * strdup_(const char *instr)
{
    char *newstr = NULL;
    if (instr != NULL) {
       newstr = malloc(strlen_(instr)+1);
       if (newstr != NULL) strcpy(newstr,instr);
    }
    return (newstr);
}

#ifdef WIN32
int strncasecmp(const char *s1, const char *s2, int n)
{
   int i, val;
   char c1, c2;
   if (s1 == NULL || s2 == NULL) return (-1);
   val = 0;
   for (i = 0; i < n; i++) {
      c1 = s1[i] & 0x5f;
      c2 = s2[i] & 0x5f;
      if (c1 < c2) { val = -1; break; }
      if (c1 > c2) { val = 1; break; }
   }
   return(val);
}
#endif

/* case insensitive string compare */
int str_icmp(char *s1, char *s2)
{
   int n1, n2, val;
   if (s1 == NULL || s2 == NULL) return (-1);
   n1 = strlen_(s1);
   n2 = strlen_(s2);
   if (n1 != n2) return(-1);
   val = strncasecmp(s1,s2,n1);
   return(val);
}
void set_loglevel(int level) 
{ 
   loglevel = level; 
}

void lprintf(int level, const char * format, ...)
{
        va_list vptr;
        static char logtmp[LOG_MSG_LENGTH];
	FILE *fp = stderr; 
	if (!verbose && (level > loglevel)) return;
	if (level > LOG_WARN) fp = stdout; /*NOTICE,INFO*/
        if (fdbglog && (fplog != NULL)) fp = fplog;
#ifdef WIN32
        va_start(vptr, format);
        vfprintf(fp, format, vptr);
        va_end(vptr);
        fprintf(fp,"\r\n");
#else
        va_start(vptr, format);
        vsnprintf(logtmp, LOG_MSG_LENGTH, format, vptr);
        va_end(vptr);
        fprintf(fp, "%s\r\n", logtmp);
#endif
        return;
}

void lperror(int level, const char * format, ...)
{
        va_list vptr;
        FILE *fp;
        if (level > loglevel) return;
        fp = stderr;
        if (fdbglog && verbose > 1) {
           if (fplog != NULL) fp = fplog;
        }
        va_start(vptr, format);
        vfprintf(fp, format, vptr);
        va_end(vptr);
        fprintf(fp,"\r\n");
        return;
}

#ifdef LANHELPER
uint32_t buf2long(uint8_t * buf);
uint16_t buf2short(uint8_t * buf);
void printbuf(const uint8_t * buf, int len, const char * desc);
const char * buf2str(uint8_t * buf, int len);
const char * oemval2str(uint16_t oem, uint16_t val, const struct oemvalstr *vs);
const char * val2str(uint16_t val, const struct valstr *vs);
uint16_t str2val(const char *str, const struct valstr *vs);
#else
ulong buf2long(uchar * buf)
{
        return (ulong)(buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0]);
}
ushort buf2short(uchar * buf)
{
        return (ushort)(buf[1] << 8 | buf[0]);
}
void printbuf(const uchar * buf, int len, const char * desc)
{
        int i;
        FILE *fp = stderr;

        if (len <= 0) return;
        if (verbose < 1) return;
        if (fdbglog && (fplog != NULL)) fp = fplog;
        fprintf(fp,  "%s (%d bytes)\r\n", desc, len);
        for (i=0; i<len; i++) {
                if (((i%16) == 0) && (i != 0))
                        fprintf(fp,   "\r\n");
                fprintf(fp,  " %2.2x", buf[i]);
        }
        fprintf(fp,  "\r\n");
}

const char * buf2str(uchar * buf, int len)
{
        static char str[1024];
        int i;
        if (len <= 0 || len > sizeof(str)) return NULL;
        memset(str, 0, sizeof(str));
        for (i=0; i<len; i++) sprintf(str+i+i, "%2.2x", buf[i]);
        str[len*2] = '\0';
        return (const char *)str;
}

#define  IPMI_OEM_PICMG  12634
#define  SZUN  32
const char * oemval2str(ushort oem, uchar val, const struct oemvalstr *vs)
{
   static char un_str[SZUN];
   int i;
   for (i = 0; vs[i].oem != 0x00 &&  vs[i].str != NULL; i++) {
      if ( ( vs[i].oem == oem || vs[i].oem == IPMI_OEM_PICMG )
         &&  vs[i].val == val  ) {
         return vs[i].str;
      }
   }
   memset(un_str, 0, SZUN);
   snprintf(un_str, SZUN, "OEM reserved #%02x", val);
   return un_str;
}
const char * val2str(ushort val, const struct valstr *vs)
{
        static char un_str[SZUN];
        int i;
        for (i = 0; vs[i].str != NULL; i++) {
                if (vs[i].val == val) return vs[i].str;
        }
        memset(un_str, 0, SZUN);
        snprintf(un_str, SZUN, "Unknown (0x%x)", val);
        return un_str;
}
ushort str2val( char *str,  struct valstr *vs)
{
        int i, x, y;
        for (i = 0; vs[i].str != NULL; i++) {
	   x = strlen_(str);
	   y = strlen_(vs[i].str);
           if (strncasecmp(vs[i].str, str, (x > y)? x : y) == 0)
                   return vs[i].val;
        }
        return vs[i].val;
}
#endif


void dump_buf(char *tag, uchar *pbuf, int sz, char fshowascii)
{
   uchar line[17];
   uchar a;
   int i, j;
   char *stag;
   FILE *fpdbg1;

   if (fpdbg != NULL) fpdbg1 = fpdbg;
   else fpdbg1 = stdout;
   if (tag == NULL) stag = "dump_buf"; /*safety valve*/
   else stag = tag;
   fprintf(fpdbg1,"%s (len=%d): ", stag,sz);
   line[0] = 0; line[16] = 0;
   j = 0;
   if (sz < 0) { fprintf(fpdbg1,"\n"); return; } /*safety valve*/
   for (i = 0; i < sz; i++) {
      if (i % 16 == 0) { 
 	 line[j] = 0; 
         j = 0; 
	 fprintf(fpdbg1,"%s\n  %04x: ",line,i); 
      }
      if (fshowascii) {
         a = pbuf[i];
         if (a < 0x20 || a > 0x7f) a = '.';
         line[j++] = a;
      }
      fprintf(fpdbg1,"%02x ",pbuf[i]);
   }
   if (fshowascii) {
     if ((j > 0) && (j < 16)) {
       /* space over the remaining number of hex bytes */
       for (i = 0; i < (16-j); i++) fprintf(fpdbg1,"   ");
     } 
     else j = 16;
     line[j] = 0;
   }
   fprintf(fpdbg1,"%s\n",line);
   return;
}

void close_log(void)
{
    if ((fplog != NULL) && (fplog != stderr) && (fplog != stdout)) {
       fclose(fplog);
       fplog = NULL;
    }
}

FILE *open_log(char *mname)
{
    FILE *fp = NULL;
    char *pname;
    int len;

    /* log_name is global, for reuse */
    if (log_name[0] == 0) {
      if (mname == NULL) {  /*make a default name*/
	 pname = "ipmiutil";
#ifdef WIN32
	 sprintf(log_name,"%s.log",pname);
#elif defined(DOS)
	 sprintf(log_name,"%s.log",pname);
#else
	 /*LINUX, SOLARIS, BSD */
	 sprintf(log_name,"/var/log/%s.log",pname);
#endif
      } else {  /*use mname arg*/
	 len = strlen_(mname);
	 if (len >= sizeof(log_name)) len = sizeof(log_name) - 1;
	 strncpy(log_name, mname, len);
      }
    }
    close_log();
    if (log_name[0] != 0) 
       fp = fopen( log_name, "a+" );
    if (fp == NULL) {
       fp = stdout;  /*was stderr*/
       fprintf(fp,"cannot open log: %s\n",log_name);
    } 
    fplog = fp;
    return fp;
}

void flush_log(void)
{
    if (fplog != NULL) fflush(fplog);
}

void print_log( char *pattn, ... )
{
    va_list arglist;
    if (fplog == NULL) fplog = open_log(NULL);
    /* if error, open_log sets fplog = stdout */
    va_start( arglist, pattn );
    vfprintf( fplog, pattn, arglist );
    va_end( arglist );
}

/* 
 * logmsg
 * This does an open/close if no log is already open, but does not set fplog. 
 */
void logmsg( char *pname, char *pattn, ... )
{
    va_list arglist;
    FILE *fp = NULL;
    int opened = 0;
 
    if (fplog == NULL) { /*no log alread open, open temp fp*/
        fp = open_log(pname);
        if (fp == NULL) return;
        opened = 1; 
    } else fp = fplog;
    va_start( arglist, pattn );
    vfprintf( fp, pattn, arglist );
    va_end( arglist );
    if ((opened) && (fp != stderr))  /*opened temp fp, so close it*/
	{ fclose(fp); }
}

void dump_log(FILE *fp, char *tag, uchar *pbuf, int sz, char fshowascii)
{ 
    FILE *fpsav;
    fpsav = fpdbg;  
    if (fplog != NULL) fpdbg = fplog;
    if (fp != NULL)    fpdbg = fp;
    dump_buf(tag, pbuf, sz, fshowascii);  /*uses fpdbg*/
    fflush(fpdbg);
    fpdbg = fpsav; 
}


extern int lasterr;  /*defined in ipmilan.c */
extern void show_LastError(char *tag, int err);  /*ipmilan.c */

#ifdef WIN32
/* Windows stdlib.h: extern int * __cdecl _errno(void); */
int get_errno(void)
{
   return(errno);
}
#else
extern int errno; /* Linux<errno.h has this also */
int get_errno(void)
{
   return(errno);
}
#endif

 /* For a list of all IANA enterprise mfg vendor numbers, 
  * see http://www.iana.org/assignments/enterprise-numbers 
  * Product numbers are different for each mfg vendor.  */
#define N_MFG  48
static struct { int val; char *pstr; } mfgs[N_MFG] = {
    {0,   " "},
    {0x0000BA, "Toshiba"},
    {0x000074, "Hitachi"},
    {0x00018F, "Hitachi"},
    {0x000175, "Tatung"}, 
    {0x000614, "Performance Technologies"},
    {0x000F85, "Aelita Software"}, /*3973. HP DL140*/
    {0x0028B2, "Avocent"}, 
    {0x002B5E, "OSA"},
    {0x0035AE, "Raritan"},   /*13742.*/
    {0x0051EE, "AMI"},   
    {      94, "Nokia-Siemens"},
    {     107, "Bull"},
    {    4337, "Radisys"},
    {    4542, "ASF"},
    {    6569, "Inventec"},
    {    7154, "IPMI forum"},
    {   11129, "Google"},
    {   12634, "PICMG"},
	{   15370, "Giga-Byte"},  /*0x3C0A*/
    {   16394, "Pigeon Point"},
    {   20569, "Inventec ESC"},
    {   24673, "ServerEngines"},
    {   27768, "NAT"},
    {VENDOR_MITAC, "MiTAC"},  /*=6653.*/
    {VENDOR_CISCO, "Cisco"},  /*=5771.*/
    {VENDOR_IBM, "IBM"},  /*0x000002*/
    {VENDOR_NEWISYS, "Newisys"}, /*=9237. */
    {VENDOR_XYRATEX, "Xyratex"}, /*=1993. */
    {VENDOR_QUANTA, "Quanta"}, /*=7244. */
    {VENDOR_MAGNUM, "Magnum Technologies"}, /*=5593. */
    {VENDOR_SUPERMICROX, "xSuperMicro"}, /* 47488. used by Winbond/SuperMicro*/
    {VENDOR_HP,   "HP"},     /* 0x00000B (11.) for HP */
    {VENDOR_DELL, "Dell"},    /*0x0002A2 (674.) */
    {VENDOR_KONTRON,    "Kontron"},     /*=0x003A98, 15000.*/
    {VENDOR_SUPERMICRO, "SuperMicro"},  /*=0x002A7C, 10876. used in AOC-SIMSO*/
    {VENDOR_FUJITSU,    "Fujitsu-Siemens"}, /* 0x002880, 10368. */
    {VENDOR_PEPPERCON,  "Peppercon"},  /* 0x0028C5, 10437. now w Raritan*/
    {VENDOR_MICROSOFT,  "Microsoft"},  /*=0x000137, 311.*/
    {VENDOR_NEC,  "NEC"},     /*=0x000077*/
    {VENDOR_NSC,  "NSC"},     /*=0x000322*/
    {VENDOR_LMC,  "LMC"},     /*=0x000878 with SuperMicro*/
    {VENDOR_TYAN, "Tyan"},    /*=0x0019FD*/
    {VENDOR_SUN,  "Sun"},     /*=0x00002A*/
    {VENDOR_LENOVO, "Lenovo"},  /*=0x004A66*/
    {VENDOR_LENOVO2, "Lenovo"}, /*=0x004F4D*/
    {VENDOR_ASUS, "ASUS"},  /*=0x000A3F*/
    {VENDOR_INTEL, "Intel"}   /*=0x000157*/
};

char * get_iana_str(int mfg)
{
   char *mfgstr = "";
   int i;
   for (i = 0; i < N_MFG; i++) {
      if (mfgs[i].val == mfg) {
	    mfgstr = mfgs[i].pstr; 
	    break; 
      }
   }
   if (i >= N_MFG) mfgstr = mfgs[0].pstr;
   return(mfgstr);
}

/* 
 * str2uchar
 * Convert string into unsigned char and check for overflows
 * @str:      (input) array of chars to parse from
 * @uchr_ptr: (output) pointer to address where uint8_t will be stored
 * returns 0 if successful, or -1,-2,-3 if error
 */
int str2uchar(char *str, uchar *uchr_ptr) 
{
       ulong lval = 0;
       char *end_ptr = NULL;
       if (str == NULL || uchr_ptr == NULL) return -1; /*NULL pointer arg*/
       *uchr_ptr = 0; /*seed with default result*/
       errno = 0;
       /* handle use of 08, 09 to avoid octal overflow */
       if (strncmp(str,"08",2) == 0) lval = 8;
       else if (strncmp(str,"09",2) == 0) lval = 9;
       else {  /*else do strtoul*/
         lval = strtoul(str, &end_ptr, 0);
         if ((end_ptr == NULL) || *end_ptr != '\0' || errno != 0) 
		return -2; /* invalid input given by user/overflow occurred */
         if (lval > 0xFF || lval == LONG_MIN || lval == LONG_MAX) 
		return -3; /* Argument is too big to fit unsigned char */
       }
       *uchr_ptr = (uchar)lval;
       return 0;
}

/* atob is like atoi, but using str2uchar */
uchar atob(char *str_in)
{
    uchar b = 0;
    int rv;
    rv = str2uchar(str_in,&b);
    /* if error, show error, but use default. */
    switch (rv) {
    case -1:
        printf("atob error: input pointer is NULL\n");
	break;
    case -2:
        printf("atob error: string-to-number conversion overflow\n");
	break;
    case -3:
        printf("atob error: numeric argument is too big for one byte\n");
	break;
    default:
	break;
    }
    return(b);
}

void atoip(uchar *array,char *instr)
{
   int i,j,n;
   char *pi;
   char tmpstr[16];
   /* converts ASCII input string into binary IP Address (array) */
   if (array == NULL || instr == NULL) {
        if (fdebug) printf("atoip(%p,%p) NULL pointer error\n",array,instr);
        return;
   }
   j = 0;
   n = strlen_(instr);
   n++; /*include the null char*/
   if (n > sizeof(tmpstr)) n = sizeof(tmpstr);
   memcpy(tmpstr,instr,n);
   pi = tmpstr;
   for (i = 0; i < n; i++) {
      if (tmpstr[i] == '.') {
	tmpstr[i] = 0;
	array[j++] = atob(pi);
	pi = &tmpstr[i+1];
	}
      else if (tmpstr[i] == 0) {
	array[j++] = atob(pi);
	}
   }
   if (fdebug) 
      printf("atoip: %d %d %d %d\n", array[0],array[1],array[2],array[3]);
}  /*end atoip()*/

/*
 * htoi
 * Almost all of the utilities use this subroutine 
 * Input:  a 2 character string of hex digits.
 * Output: a hex byte.
 */
uchar htoi(char *inhex)
{
   // char rghex[16] = "0123456789ABCDEF";
   uchar val;
   uchar c;
   if (inhex[1] == 0) { /* short string, one char */
      c = inhex[0] & 0x5f;  /* force cap */
      if (c > '9') c += 9;  /* c >= 'A' */
      val = (c & 0x0f);
   } else {
      c = inhex[0] & 0x5f;  /* force cap */
      if (c > '9') c += 9;  /* c >= 'A' */
      val = (c & 0x0f) << 4;
      c = inhex[1] & 0x5f;  /* force cap */
      if (c > '9') c += 9;  /* c >= 'A' */
      val += (c & 0x0f);
   }
   return(val);
}


void os_usleep(int s, int u)
{
#ifdef WIN32
   if (s == 0) {
      int i;
      if (u >= 1000) Sleep(u/1000);
      else for (i=0; i<u; i++) s = 0;  /*spin for u loops*/
   } else {
      Sleep(s * 1000);
   }
#elif defined(DOS)
   if (s == 0) delay(u);
   else delay(s * 1000);
#else
   if (s == 0) {
      usleep(u);
   } else {
      sleep(s);
   }
#endif
}

#define  SYS_INFO_MAX    64

static int sysinfo_has_len(uchar enc, int vendor)
{
   int rv = 1;
   int vend;
   if (enc > (uchar)2) return(0);  /*encoding max is 2*/
   if (vendor == 0) get_mfgid(&vend, NULL);
   else vend = vendor;
   if (vend == VENDOR_INTEL) rv = 0;
   if (vend == VENDOR_SUPERMICRO) rv = 0;
   return(rv);
}

int get_device_guid(uchar *pbuf, int *szbuf)
{
   int rv = -1;
   //uchar idata[8];
   uchar rdata[32];
   int rlen, len;
   uchar cc;
   ushort cmdw;
   
   len = *szbuf;
   *szbuf = 0;
   cmdw = 0x08 | (NETFN_APP << 8); 
   rv = ipmi_cmd_mc(cmdw, NULL,0,rdata,&rlen,&cc,fdebug);
   if (rv == 0 && cc != 0) rv = cc;
   if (rv == 0) {
      if (rlen > len) rlen = len;
      memcpy(pbuf,rdata,rlen);
      *szbuf = rlen;
   }
   return(rv);
}

int get_sysinfo(uchar parm, uchar set, uchar block, uchar *pbuf, int *szbuf)
{
   uchar idata[8];
   uchar rdata[32];
   int rlen, j, len;
   int rv = -1;
   uchar cc;
   ushort cmdw;
  
   if (pbuf == NULL || szbuf == NULL) return(rv);
   len = 0;
   idata[0] = 0; 
   idata[1] = parm; 
   idata[2] = set;
   idata[3] = block; 
   rlen = sizeof(rdata);
   cmdw = CMD_GET_SYSTEM_INFO | (NETFN_APP << 8);
   rv = ipmi_cmd_mc(cmdw, idata,4,rdata,&rlen,&cc,fdebug);
   if (rv == 0 && cc != 0) rv = cc;
   if (rv == 0) {
      j = 2;
      if (set == 0) {  /*NEW_FMT: first set includes type, len */
	 if (sysinfo_has_len(rdata[2],0)) { /*but len not used if Intel*/
	    j = 4;
	    len = rdata[3];
	 }
      }
      rdata[rlen] = 0; /*stringify for debug below*/
      rlen -= j;
      if (fdebug) printf("get_sysinfo(%d,%d) j=%d len=%d %s\n",
				parm,set,j,rlen,&rdata[j]);
      if (rlen > *szbuf) rlen = *szbuf;
      memcpy(pbuf,&rdata[j],rlen);
      *szbuf = rlen;
   }
   return(rv);
}

int set_system_info(uchar parm, char *pbuf, int szbuf)
{
   uchar idata[32];
   uchar rdata[8];
   int rlen, ilen, i, j, n;
   int rv = -1;
   uchar cc, set;
   ushort cmdw;
  
   if (pbuf == NULL) return(rv);
   if (szbuf > SYS_INFO_MAX) szbuf = SYS_INFO_MAX;
   n = 0; set = 0;
   while ((n < szbuf) || (n == 0)) {
      ilen = 16;
      j = 2;
      memset(idata,0,sizeof(idata));
      idata[0] = parm; 
      idata[1] = set; 
      if (set == 0) {  /*NEW_FMT: first set includes type, len */
	 if (sysinfo_has_len(0,0)) { /*but len not used if Intel*/
	    j = 4;
            idata[2] = 0;   /*type = ASCII+Latin1*/
            idata[3] = szbuf;  /*overall length*/
	 }
      }
      i = ilen;
      if (i > (szbuf - n)) i = (szbuf - n);
      memcpy(&idata[j],&pbuf[n],i);
      rlen = sizeof(rdata);
      cmdw = CMD_SET_SYSTEM_INFO | (NETFN_APP << 8);
      rv = ipmi_cmd_mc(cmdw, idata,(ilen+j),rdata,&rlen,&cc,fdebug);
      if (rv == 0 && cc != 0) rv = cc;
      if (fdebug) printf("set_system_info(%d,%d) rv=%d j=%d ilen=%d %s\n",
				parm,set,rv,j,ilen,&pbuf[n]);
      if (rv != 0) break;
      else {
	 n += ilen;
         set++;
      }
   }
   return(rv);
}

int get_system_info(uchar parm, char *pbuf, int *szbuf)
{
   int rv = -1;
   int i, off, len, szchunk;
   
   off = 0; len = *szbuf;
   /* SYS_INFO_MAX = 64 (4 * 16) */
   for (i = 0; i < 4; i++) {
      szchunk = 16;
      if ((off + szchunk) > *szbuf) break;
      rv = get_sysinfo(parm,i,0,&pbuf[off],&szchunk);
      if (rv != 0) break;
      off += szchunk;
      if (off >= len) break;
   }
   if (off < *szbuf) *szbuf = off;
   return(rv);
}

int ipmi_reserved_user(int vend, int userid)
{
   int ret = 0;
   if (userid == 1) {
      switch(vend) {
      case VENDOR_INTEL: ret = 0; break;
      case VENDOR_KONTRON: ret = 1; break;
      case VENDOR_SUPERMICRO: ret = 1; break;
      case VENDOR_SUPERMICROX: ret = 1; break;
      default:  ret = 0; break;
      }
   }
   return(ret);
}
	
#define NSEV  4
static char *sev_str[NSEV] = {
  /*0*/ "INF",
  /*1*/ "MIN",
  /*2*/ "MAJ",
  /*3*/ "CRT" };

uchar find_msg_sev(char *msg)
{
   int i;
   char *p;
   uchar sev = SEV_INFO;

   if (msg == NULL) return(sev);
   for (i = 0; i < NSEV; i++) {
	p = strstr(msg,sev_str[i]);
	if (p != NULL) { sev = (uchar)i; break; }
   }
   return(sev);
}

int OpenSyslog(char *tag)
{
	int ret = -1; 
	if (tag == NULL) tag = "ipmiutil";
#ifdef WIN32
	/* Requires showselmsg.dll and this Registry entry:
	   HKLM/SYSTEM/CurrentControlSet/Services/EventLog/Application/showsel 
	   EventMessageFile REG_EXPAND_SZ "%SystemRoot%\system32\showselmsg.dll"
	   TypesSupported   REG_DWORD     0x000000007 
	*/
	hLog = RegisterEventSource(NULL, "showsel");
	if (hLog == (void *)ERROR_INVALID_HANDLE) { hLog = NULL; }
	if (hLog == NULL) 
		printf("RegisterEventSource error, %lx\n", GetLastError());
	else ret = 0; /*success*/
#elif defined(DOS)
	ret = LAN_ERR_NOTSUPPORT;
#else
	// Open syslog
        openlog( tag,  LOG_CONS, LOG_KERN);
	ret = 0;   /* success if here */
#endif
	return(ret);
}

void CloseSyslog(void)
{
#ifdef WIN32
	DeregisterEventSource(hLog);
#elif defined(DOS)
	;
#else
	// Close syslog
	closelog();
#endif
}

void WriteSyslog(char *msgbuf)
{
	uchar sev;
#ifdef WIN32
	BOOL status;
	char *rgstrings[2] = {NULL, NULL};
	WORD level;
	rgstrings[0] = msgbuf; /*decoded SEL entry*/
	sev = find_msg_sev(msgbuf);
	switch(sev) {
	case SEV_MIN:  level = EVENTLOG_WARNING_TYPE; break;
	case SEV_MAJ:  level = EVENTLOG_ERROR_TYPE; break;
	case SEV_CRIT: level = EVENTLOG_ERROR_TYPE; break;
	case SEV_INFO: 
	default:       level = EVENTLOG_INFORMATION_TYPE; break;
	}
	if (hLog != NULL) {
		status = ReportEvent(hLog,EVENTLOG_INFORMATION_TYPE,
					0, SELMSG_ID, NULL, 1,0,
					rgstrings,NULL);
					/* showsel eventid = 0x101. */
		if (fdebug || (status == 0)) {  /*error or debug*/
		      printf("ReportEvent status=%d, %lx\n",
					status,GetLastError());
		} 
	}
#elif defined(DOS)
	;
#else
	int level;
	sev = find_msg_sev(msgbuf);
	switch(sev) {
	case SEV_MIN:  level = LOG_WARN; break;
	case SEV_MAJ:  level = LOG_ERR; break;
	case SEV_CRIT: level = LOG_CRIT; break;
	case SEV_INFO: 
	default:       level = LOG_INFO; break;
	}
	syslog(level,"%s",msgbuf);
#endif
} /*end WriteSyslog*/

int write_syslog(char *msg)
{		/* not used in showsel, but used by getevent, hwreset */
   int rv;
   rv = OpenSyslog("ipmiutil");
   if (rv == 0) {
      WriteSyslog(msg);
      CloseSyslog();
   }
   return(rv);
}

/* end subs.c */
