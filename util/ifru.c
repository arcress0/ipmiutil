/*
 * ifru  (was fruconfig.c)
 *
 * This tool reads the FRU configuration, and optionally sets the asset
 * tag field in the FRU data.  See IPMI v1.5 spec section 28.
 * It can use either the Intel /dev/imb driver or VALinux /dev/ipmikcs.
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 *
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 10/28/02 Andy Cress - created
 * 12/11/02 Andy Cress v0.9  - disable write until checksum fixed.
 * 12/13/02 Andy Cress v0.91 - don't abort if cc=c9 in load_fru loop.
 * 01/27/03 Andy Cress v0.92 - more debug, do checksums,
 * 01/28/03 Andy Cress v1.0  do writes in small chunks, tested w SCB2 & STL2
 * 02/19/03 Andy Cress v1.1  also get System GUID
 * 03/10/03 Andy Cress v1.2  do better bounds checking on FRU size
 * 04/30/03 Andy Cress v1.3  Board Part# & Serial# reversed
 * 05/13/03 Andy Cress v1.4  Added Chassis fields
 * 06/19/03 Andy Cress v1.5  added errno.h (email from Travers Carter)
 * 05/03/04 Andy Cress v1.6  BladeCenter has no product area, only board area
 * 05/05/04 Andy Cress v1.7  call ipmi_close before exit,
 *                           added WIN32 compile options.
 * 11/01/04 Andy Cress v1.8  add -N / -R for remote nodes   
 * 12/10/04 Andy Cress v1.9  add gnu freeipmi interface
 * 01/13/05 Andy Cress v1.10 add logic to scan SDRs for all FRU devices,
 *                           and interpret them
 * 01/17/05 Andy Cress v1.11 decode SPD Manufacturer
 * 01/21/05 Andy Cress v1.12 format SystemGUID display
 * 02/03/05 Andy Cress v1.13 fixed fwords bit mask in load_fru,
 *                           decode DIMM size from SPD also.
 * 02/04/05 Andy Cress v1.14 decode FRU Board Mfg DateTime
 * 03/16/05 Andy Cress v1.15 show Asset Tag Length earlier
 * 05/24/05 Andy Cress v1.16 only do write_asset if successful show_fru
 * 06/20/05 Andy Cress v1.17 handle Device SDRs also for ATCA
 * 08/22/05 Andy Cress v1.18 allow setting Product Serial Number also (-s),
 *                           also add -b option to show only baseboard data.
 * 10/31/06 Andy Cress v1.25 handle 1-char asset/serial strings (avoid c1)
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
#include <string.h>
#include "getopt.h"
#elif defined(DOS)
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#else
#include <errno.h>
#endif
#endif
#include <time.h>

#include "ipmicmd.h" 
#include "ipicmg.h" 
#include "oem_intel.h" 
#include "ifru.h" 

#define PICMG_CHILD   1
#define MIN_SDR_SZ  8
#ifndef URNLOOPS
#define URNLOOPS 1000
#endif
extern int get_BiosVersion(char *str);
extern int get_SystemGuid(uchar *guid);
extern void fmt_time(time_t etime, char *buf, int bufsz); /*see ievents.c*/
extern int get_LastError( void );  /* ipmilan.c */
extern int GetSDRRepositoryInfo(int *nret, int *fdev);  /*isensor.h*/
#ifdef METACOMMAND
extern int ipmi_kontronoem_main(void * intf, int  argc, char ** argv);
extern char *progver;  /*from ipmiutil.c*/
static char *progname  = "ipmiutil fru";
#else
static char *progver   = "3.11";
static char *progname  = "ifru";
#endif

static int  vend_id = 0; 
static int  prod_id = 0; 
static char fdebug = 0;
static char fpicmg = 0;
static char fonlybase = 0;
static char fonlyhsc = 0;
static int  fwritefru = 0;
static int  fdevsdrs = 0;
static char fshowlen = 0;
static char ftestshow = 0;
// static char fgetfru  = 0;
static char fdoanyway = 0;
static char fprivset = 0;
static char fset_mc = 0;
static char fcanonical = 0;
static char foemkontron = 0;
static char fbasefru = 1;
static char fdump = 0;
static char frestore = 0;
static char fchild = 0;        /* =1 follow child MCs if picmg bladed*/
static char do_systeminfo = 1;
static char do_guid = 1;
static char bdelim = ':';
static uchar bmc_sa = BMC_SA;  /*defaults to 0x20*/
static uchar guid[17] = "";
static char *binfile = NULL;
static uchar *frubuf = NULL;
#define FIELD_LEN   20
#define SZ_PRODAREA   520 /* product area max is 8*32 + 3 = 259 mod 8 = 264 */
static int    sfru = 0;
static int    asset_offset = -1;
static int    asset_len = 0;
static int    sernum_offset = -1;
static int    sernum_len = 0;
static int    prodver_offset = -1;
static int    prodver_len = 0;
static int    chassis_offset = -1;
static int    chassis_len = 0;
static char   asset_tag[FIELD_LEN]  = {0};
static char   serial_num[FIELD_LEN] = {0};
static char   prod_ver[FIELD_LEN] = {0};
static char   chassis_name[FIELD_LEN] = {0};
static char   ps_prod[FIELD_LEN] = {0};
static int    maxprod = 0;
static int ctype;
static uchar g_bus = PUBLIC_BUS;
static uchar g_sa  = 0;
static uchar g_lun = BMC_LUN;
static uchar g_addrtype = ADDR_SMI;
static uchar g_fruid = 0;  /* default to fruid 0 */
static uchar g_frutype = 0; 
static uchar lastfru[3] = {0,0,0};
/* g_frutype values (detected), see also FruTypeString
 * --TAG----   FRU   IPMB
 * Baseboard = 0x0c  0x07 
 * PowerSply = 0x0a
 * PowerCage = 0x15
 * DIMM      = 0x20
 * HotSwapCt =       0x0f
 * ME        =       0x2e
 * SysInfo   =    0x40
 * Component =  *     *  (all others)
 */
#ifdef METACOMMAND
extern int verbose;  
extern int sdr_get_reservation(uchar *res_id, int fdev);
#else
static int verbose;  
static int sdr_get_reservation(uchar *res_id, int fdev) { return(-1); }
#endif

#define ERR_LENMAX   -7   /*same as LAN_ERR_BADLENGTH */
#define ERR_LENMIN   -10  /*same as LAN_ERR_TOO_SHORT */
#define ERR_OTHER    -13  /*same as LAN_ERR_OTHER     */

#define STRING_DATA_TYPE_BINARY         0x00
#define STRING_DATA_TYPE_BCD_PLUS       0x01
#define STRING_DATA_TYPE_SIX_BIT_ASCII  0x02
#define STRING_DATA_TYPE_LANG_DEPENDENT 0x03

#define FRUCHUNK_SZ   16   /* optimal chunk = 16 bytes */
#define FRU_END         0xC1
#define FRU_EMPTY_FIELD 0xC0
#define FRU_TYPE_MASK   0xC0
#define FRU_LEN_MASK    0x3F

#define NUM_BOARD_FIELDS    6
#define NUM_PRODUCT_FIELDS  8
#define NUM_CHASSIS_FIELDS  3
#define MAX_CTYPE   26
char *ctypes[MAX_CTYPE] = { "", "Other", "Unknown", "Desktop", 
	"Low Profile Desktop", "Pizza Box", "Mini-Tower", "Tower", 
	"Portable", "Laptop", "Notebook", "Handheld", "Docking Station", 
	"All-in-One", "Sub-Notebook", "Space-saving", "Lunch Box", 
	"Main Server Chassis", "Expansion Chassis", "SubChassis", 
	"Bus Expansion Chassis", "Peripheral Chassis", 
	"RAID Chassis", /*0x17=23.*/ "Rack-Mount Chassis", 
	"New24" , "New25"};
	/* what about bladed chassies? */
char *ctype_hdr = 
"Chassis Type        "; 
char *chassis[NUM_CHASSIS_FIELDS] = {
"Chassis Part Number ", 
"Chassis Serial Num  ",
"Chassis OEM Field   " };
char *board[NUM_BOARD_FIELDS] = {
"Board Manufacturer  ",
"Board Product Name  ",
"Board Serial Number ",
"Board Part Number   ",
"Board FRU File ID   ",
"Board OEM Field     " };
char *product[NUM_PRODUCT_FIELDS] = {
"Product Manufacturer",
"Product Name        ",
"Product Part Number ",
"Product Version     ",
"Product Serial Num  ",  
"Product Asset Tag   ",
"Product FRU File ID ",
"Product OEM Field   " };
char *asset_hdr = 
"   Asset Tag Length ";

#if 0
typedef struct {
	uchar len :6;
	uchar type:2;
	} TYPE_LEN;  /*old, assumes lo-hi byte order*/
#else
typedef struct {
	uchar len;
	uchar type;
	} TYPE_LEN;
#endif

void
free_fru(uchar *pfrubuf)
{
   if (pfrubuf != NULL) {
      if (frubuf != NULL) free(frubuf);
      frubuf = NULL;
   }
   return;
}

int
load_fru(uchar sa, uchar frudev, uchar frutype, uchar **pfrubuf)
{
   int ret = 0;
   uchar indata[FRUCHUNK_SZ+9];
   uchar resp[18];
   int sresp;
   uchar cc;
   int sz;
   char fwords;
   ushort fruoff = 0;
   int i, rv;
   int chunk;

   if (pfrubuf == NULL) return(ERR_BAD_PARAM);
   *pfrubuf = NULL;
   memset(indata, 0, sizeof(indata));
   indata[0] = frudev;
   sresp = sizeof(resp);
   if (fdebug) printf("load_fru: sa=%02x, frudev=%02x, addrtype=%d\n",
			sa,frudev,g_addrtype);
   ret = ipmi_cmd_mc(GET_FRU_INV_AREA,indata,1,resp,&sresp,&cc,fdebug);
   if (fdebug) printf("load_fru: inv ret=%d, cc=%x, resp=%02x %02x %02x\n",
			ret,cc, resp[0], resp[1], resp[2]);
   if (ret != 0) return(ret);
   if (cc != 0) { ret = (cc & 0x00ff); return(ret); }

   sz = resp[0] + (resp[1] << 8);
   if (resp[2] & 0x01) { fwords = 1; sz = sz * 2; }
   else fwords = 0;

   frubuf = malloc(sz);
   if (frubuf == NULL) return(get_errno());
   *pfrubuf = frubuf;
   sfru = sz;
      
   /* Loop on READ_FRU_DATA */
   for (i = 0, chunk=FRUCHUNK_SZ; i < sz; i+=chunk)
   {
	if ((i+chunk) >= sz) chunk = sz - i;
	indata[0] = frudev;  /* FRU Device ID */
	if (fwords) {
	   indata[3] = chunk / 2;
	   fruoff = (i/2);
	} else {
	   indata[3] = (uchar)chunk;
	   fruoff = (ushort)i;
	}
        indata[1] = fruoff & 0x00FF;
        indata[2] = (fruoff & 0xFF00) >> 8;
        sresp = sizeof(resp);
        ret = ipmi_cmd_mc(READ_FRU_DATA,indata,4,resp,&sresp,&cc,fdebug);
        if (ret != 0) break;
        else if (cc != 0) {
           if (i == 0) ret = cc & 0x00ff; 
           if (fdebug) printf("read_fru[%d]: ret = %d cc = %x\n",i,ret,cc);
           break; 
        }
        memcpy(&frubuf[i],&resp[1],chunk);
   }

   if ((frudev == 0) && (sa == bmc_sa) && do_guid) 
   { /*main system fru, so get GUID*/
     sresp = sizeof(resp);
     rv = ipmi_cmd_mc(GET_SYSTEM_GUID,indata,0,resp,&sresp,&cc,fdebug);
     if (fdebug) printf("system_guid: ret = %d, cc = %x\n",rv,cc);
     if (rv == 0) rv = cc;
     if ((rv != 0) && !is_remote()) {  /* get UUID from SMBIOS */
        cc = 0; sresp = 16;
        rv = get_SystemGuid(resp); 
        if (fdebug) printf("get_SystemGuid: ret = %d\n",rv);
     }
     if (rv == 0 && cc == 0) {
       if (fdebug) {
	     printf("system guid (%d): ",sresp);
	     for (i=0; i<16; i++) printf("%02x ",resp[i]);
	     printf("\n");
       }
       memcpy(&guid,&resp,16);
       guid[16] = 0;
     } else {
       printf("WARNING: GetSystemGuid error %d, %s\n",rv,decode_rv(rv));
       /*do not pass this error upstream*/
     }
   } /*endif*/
   return(ret);
}

static void decode_string(unsigned char type,
                          unsigned char language_code,
                          unsigned char *source,
                          int           slen,
                          char          *target,
                          int           tsize)
{
  static const char bcd_plus[] = "0123456789 -.:,_";
  unsigned char *s = &source[0];
  int len, i, j, k;
  union { uint32_t bits; char chars[4]; } u;
 
  if (slen == 0 || slen == 1) return;
  switch(type) {
     case STRING_DATA_TYPE_BINARY: /* 00: binary/unspecified */
        len = (slen*2); break; /* hex dump -> 2x length */
     case STRING_DATA_TYPE_SIX_BIT_ASCII:  /*type 2 6-bit ASCII*/
        /* 4 chars per group of 1-3 bytes */
        len = ((((slen+2)*4)/3) & ~3); break;
     case STRING_DATA_TYPE_LANG_DEPENDENT:  /* 03 language dependent */
     case STRING_DATA_TYPE_BCD_PLUS: /* 01b: BCD plus */
     default:  
	len = slen; break;
  }
  if (len >= tsize) len = tsize - 1;
  memset(target, 0, len);
  if (type == STRING_DATA_TYPE_BCD_PLUS) { /*type 1: BCD plus*/
     for (k=0; k<len; k++)
         target[k] = bcd_plus[(s[k] & 0x0f)];
     target[k] = '\0';
  } else if (type == STRING_DATA_TYPE_SIX_BIT_ASCII) { /*type 2: 6-bit ASCII*/
     for (i=j=0; i<slen; i+=3) {
            u.bits = 0;
            k = ((slen-i) < 3 ? (slen-i) : 3);
#if WORDS_BIGENDIAN
            u.chars[3] = s[i];
            u.chars[2] = (k > 1 ? s[i+1] : 0);
            u.chars[1] = (k > 2 ? s[i+2] : 0);
#define CHAR_IDX 3
#else
            memcpy((void *)&u.bits, &s[i], k);
#define CHAR_IDX 0
#endif
            for (k=0; k<4; k++) {
                    target[j++] = ((u.chars[CHAR_IDX] & 0x3f) + 0x20);
                    u.bits >>= 6;
            }
     }
     target[j] = '\0';
  } else if (type == STRING_DATA_TYPE_LANG_DEPENDENT) {  /*type 3*/
     if ((language_code == 0x00) || (language_code == 0x25) || 
	(language_code == 0x19)) {
       memcpy(target, source, len);
       target[len] = 0;
     } else {
       printf("Language 0x%x dependent decode not supported\n",language_code);
     }
  } else if (type == STRING_DATA_TYPE_BINARY) { /* 00: binary/unspecified */
     strncpy(target, buf2str(s, slen), len);
     target[len] = '\0';  /* add end-of-string char */
  } else {   /* other */
     printf("Unable to decode type 0x%.2x\n",type);
  }
  return;
}

uchar calc_cksum(uchar *pbuf,int len)
{
   int i; 
   uchar cksum;
   uchar sum = 0;
  
   for (i = 0; i < len; i++) sum += pbuf[i];
   cksum = 0 - sum; 
   return(cksum);
}

static void dumpbuf(uchar *pbuf,int sz)
{
   uchar line[17];
   uchar a;
   int i, j;

   line[0] = 0; line[16] = 0;
   j = 0;
   for (i = 0; i < sz; i++) {
	   if (i % 16 == 0) { j = 0; printf("%s\n  %04d: ",line,i); }
	   a = pbuf[i];
	   if (a < 0x20 || a > 0x7f) a = '.';
	   line[j++] = a;
	   printf("%02x ",pbuf[i]);
   }
   line[j] = 0;
   printf("%s\n",line);
   return;
} 

#define NSPDMFG 10
static struct { 
   uchar id;  char *str;
} spd_mfg[NSPDMFG] = {  /* see JEDEC JEP106 doc */
{ 0x02, "AMI" },
{ 0x04, "Fujitsu" },
{ 0x15, "Philips Semi" },
{ 0x1c, "Mitsubishi" },
{ 0x2c, "Micron" },
{ 0x89, "Intel" },
{ 0x98, "Kingston" },
{ 0xA8, "US Modular" },
{ 0xc1, "Infineon" },
{ 0xce, "Samsung" }
};

int ValidTL(uchar typelen)
{
   if (vend_id != VENDOR_INTEL) return 1;
   if ((typelen & 0xC0) == 0xC0) return 1;  /* validate C type */
   if ((typelen & 0x00) == 0x00) return 1;  /* validate 0 type too */
   else return 0;
}

#define SYS_FRUTYPE 0x40
char * FruTypeString(uchar frutype, uchar dev)
{
   char *pstr;
   switch (frutype) { 
            case 0x07:        /*IPMB*/
            case 0x0c: 	      /*FRU*/
			if (dev == 0) pstr = "Baseboard"; 
			else          pstr = "Board    "; 
			break; 
            case 0x0a:  pstr = "PowerSply"; break;  /*FRU*/
            case 0x15:  pstr = "PowerCage"; break;  /*FRU*/
            case 0x20:  pstr = "DIMM     "; break;  /*FRU*/
            case 0x0f:  pstr = "HotSwapCt"; break;  /*IPMB*/
            case 0x2e:  pstr = "ME       "; break;  /*IPMB*/
            case SYS_FRUTYPE:  pstr = "SysInfo  "; break;  /*SysInfo*/
            default:    pstr = "Component"; break; 
	}
   return(pstr);
}

static int ChkOverflow(int idx, int sz, uchar len) 
{
   if (idx >= sz) {
        printf("  ERROR - FRU Buffer Overflow (%d >= %d), last len=%d\n",
		idx,sz,len);
	return 1;   /*overflow error*/
   }
   return 0;
}

static void 
show_spd(uchar *spd, int lenspd, uchar frudev, uchar frutype)
{
   int sz;
   char *pstr;
   int sdcap, ranks, busw, sdw, totcap;
   uchar srev, mfgid, mrev1, mrev2, yr, wk;
   uchar isdcap, iranks, ibusw, isdw;
   uchar iparity, iser, ipart, irev;
   int i;
   char devstr[24];

   /* Sample SPD Headers: 
      80 08 07 0c 0a 01 48 00 (DDR SDRAM DIMM) 
      92 10 0b 01 03 1a 02 00 (DDR3 SDRAM DIMMs) */
   sz = spd[0];  /* sz should == lenspd */
   srev = spd[1];  /* SPD Rev: 8 for DDR, 10 for DDR3 SPD spec */
   if (fcanonical) devstr[0] = 0;  /*default is empty string*/
   else sprintf(devstr,"[%s,   %02x] ",FruTypeString(frutype,frudev),frudev);
   printf("%sMemory SPD Size     %c %d\n",
		devstr,bdelim,lenspd);
   switch (spd[2]) {
     case 0x02: pstr = "EDO"; break;
     case 0x04: pstr = "SDRAM"; break;
     case 0x05: pstr = "ROM"; break;
     case 0x06: pstr = "DDR SGRAM"; break;
     case 0x07: pstr = "DDR SDRAM"; break;
     case 0x08: pstr = "DDR2 SDRAM"; break;
     case 0x09: pstr = "DDR2 SDRAM FB"; break;
     case 0x0A: pstr = "DDR2 SDRAM FB PROBE"; break;
     case 0x0B: pstr = "DDR3 SDRAM"; break;
     default:   pstr = "DRAM"; break;  /*FPM DRAM or other*/
   }
   printf("%sMemory Type         %c %s\n", devstr,bdelim,pstr);
   if (srev < 0x10) {  /*used rev 0.8 of SPD spec*/
      printf("%sModule Density      %c %d MB per bank\n",
		devstr,bdelim, (spd[31] * 4));
      printf("%sModule Banks        %c %d banks\n", 
		devstr,bdelim,spd[5]);
      printf("%sModule Rows, Cols   %c %d rows, %d cols\n",
		devstr,bdelim, spd[3], spd[4]);
      iparity = spd[11];
      mfgid = spd[64];
   } else { /* use 1.0 SPD spec with DDR3 */
      /* SDRAM CAPACITY    = SPD byte 4 bits 3~0 */
      /* PRIMARY BUS WIDTH = SPD byte 8 bits 2~0 */
      /* SDRAM WIDTH       = SPD byte 7 bits 2~0 */
      /* RANKS             = SPD byte 7 bits 5~3 */
      isdcap = (spd[4] & 0x0f);
      iranks = (spd[7] & 0x38) >> 3;
      isdw   = (spd[7] & 0x07);
      ibusw  = (spd[8] & 0x07);
      iparity = (spd[8] & 0x38) >> 3;
      mfgid = spd[118];
      switch(isdcap) {
        case 0:  sdcap =   256; /*MB*/ break;
        case 1:  sdcap =   512; /*MB*/ break;
        case 2:  sdcap =  1024; /*MB*/ break;
        case 3:  sdcap =  2048; /*MB*/ break;
        case 4:  sdcap =  4096; /*MB*/ break;
        case 5:  sdcap =  8192; /*MB*/ break;
        case 6:  sdcap = 16384; /*MB*/ break;
        default: sdcap = 32768; /*MB*/ break;
      }
      switch(iranks) {
        case 0:  ranks =  1; break;
        case 1:  ranks =  2; break;
        case 2:  ranks =  3; break;
        case 3: 
        default: ranks =  4; break;
      }
      switch(isdw) {
        case 0:  sdw =  4; break;
        case 1:  sdw =  8; break;
        case 2:  sdw = 16; break;
        case 3: 
        default: sdw = 32; break;
      }
      switch(ibusw) {
        case 0:  busw =  8; break;
        case 1:  busw = 16; break;
        case 2:  busw = 32; break;
        case 3: 
        default: busw = 64; break;
      }
      totcap = sdcap / 8 * busw / sdw * ranks;
      printf("%sModule Density      %c %d Mbits\n",devstr,bdelim,sdcap);
      printf("%sModule Ranks        %c %d ranks\n",devstr,bdelim,ranks);
      printf("%sModule Capacity     %c %d MB\n",devstr,bdelim,totcap);
   }
   if (iparity == 0x00) pstr = "Non-parity";
   else /* usu 0x02 */  pstr = "ECC";
   printf("%sDIMM Config Type    %c %s\n", devstr,bdelim,pstr);
   for (i = 0; i < NSPDMFG; i++) 
	if (spd_mfg[i].id == mfgid) break;
   if (i == NSPDMFG) pstr = "";  /* not found, use null string */
   else pstr = spd_mfg[i].str;
   printf("%sManufacturer ID     %c %s (0x%02x)\n", devstr,bdelim,pstr,mfgid);
   if (srev < 0x10) {
      yr = spd[93];
      wk = spd[94];
      iser = 95;
      ipart = 73;
      irev = 91;
   } else { /* 1.0 SPD spec with DDR3 */
      yr = spd[120];
      wk = spd[121];
      iser = 122;
      ipart = 128;
      irev = 146;
   }
   mrev1 = spd[irev];  /* save this byte for later */
   mrev2 = spd[irev+1]; 
   spd[irev] = 0;      /*stringify part number */
   printf("%sManufacturer Part#  %c %s\n",
		devstr,bdelim,&spd[ipart]);
   printf("%sManufacturer Rev    %c %02x %02x\n",
		devstr,bdelim,mrev1,mrev2);
   printf("%sManufacturer Date   %c year=%02x week=%02x\n",
		devstr,bdelim, yr,wk);
   printf("%sAssembly Serial Num %c %02x%02x%02x%02x\n",
		devstr,bdelim,spd[iser],spd[iser+1],spd[iser+2],spd[iser+3]);
   spd[irev] = mrev1;  /* restore byte, so ok to repeat later */
   return;
}

void show_guid(uchar *pguid)
{
   char *s;
   int i;
	for (i=0; i<16; i++) {
	   if ((i == 4) || (i == 6) || (i == 8) || (i == 10)) s = "-";
	   else s = "";
	   printf("%s%02x",s,pguid[i]);
	}
}

static char *volt_desc(uchar b)
{
   char *s;
   switch(b) {
   case 0x03: s = "5V"; break;
   case 0x02: s = "3.3V"; break;
   case 0x01: s = "-12V"; break;
   case 0x00: 
   default:   s = "12V"; break;
   }
   return(s);
}

static char *mgt_type(uchar b)
{
   char *s;
   switch(b) {
   case 0x01: s = "SysMgt_URL"; break;
   case 0x02: s = "SystemName"; break;
   case 0x03: s = "SysPingAddr"; break;
   case 0x04: s = "Compon_URL"; break;
   case 0x05: s = "ComponName"; break;
   case 0x06: s = "ComponPing"; break;
   case 0x07: 
   default:   s = "SysGUID"; break;
   }
   return(s);
}


static
void show_fru_multi(char *tag, int midx, uchar mtype, uchar *pdata, int dlen)
{
   int vend; 
   char mystr[256];
   char *s1; 
   int v1, v2, v3, v4, v5, v6, v7; 
   uchar b1, b2;

   if (fdebug) dumpbuf(pdata,dlen);  /*multi-record area dump*/
   sprintf(mystr,"%sMulti[%d] ",tag,midx);
   switch(mtype) {
	case 0x00:  /*Power Supply*/
		printf("%sPower Supply Record %c \n",mystr,bdelim);
		v1 = pdata[0] + ((pdata[1] & 0x0f) << 8);
		printf("\t Capacity  \t%c %d W\n",bdelim, v1);
		v2 = pdata[2] + (pdata[3] << 8);
		printf("\t Peak VA   \t%c %d VA\n",bdelim, v2);
		printf("\t Inrush Current\t%c %d A\n",bdelim, pdata[4]);
		printf("\t Inrush Interval\t%c %d ms\n",bdelim, pdata[5]);
		v3 = pdata[6] + (pdata[7] << 8);
		v4 = pdata[8] + (pdata[9] << 8);
		printf("\t Input Voltage Range1\t%c %d-%d V\n",
			bdelim,v3/100,v4/100);
		v3 = pdata[10] + (pdata[11] << 8);
		v4 = pdata[12] + (pdata[13] << 8);
		printf("\t Input Voltage Range2\t%c %d-%d V\n",
			bdelim,v3/100,v4/100);
		printf("\t Input Frequency Range\t%c %d-%d Hz\n",
			bdelim,pdata[14],pdata[15]);
		printf("\t AC Dropout Tolerance\t%c %d ms\n",bdelim, pdata[16]);
		b1 = pdata[17];
                b2 = (b1 & 0x01);
		if (b2) {  /*predictive fail*/
		    if ((b1 & 0x10) != 0) s1 = "DeassertFail ";
		    else s1 = "AssertFail ";
		} else {
		    if ((b1 & 0x10) != 0) s1 = "2pulses/rot ";
		    else s1 = "1pulse/rot ";
		}
		printf("\t Flags   \t%c %s%s%s%s%s\n",bdelim,
		  b2 ? "PredictFail " : "",
		  ((b1 & 0x02) == 0) ? "" : "PowerFactorCorrect ",
		  ((b1 & 0x04) == 0) ? "" : "AutoswitchVolt ",
		  ((b1 & 0x08) == 0) ? "" : "Hotswap ", s1);
		v5 = pdata[18] + ((pdata[19] & 0x0f) << 8);
		v6 = (pdata[19] & 0xf0) >> 4;
		printf("\t Peak Capacity \t%c %d W for %d s\n",bdelim, v5,v6);
		if (pdata[20] == 0) {
		   printf("\t Combined Capacity\t%c not specified\n",bdelim);
		} else {
		   b1 = pdata[20] & 0x0f;
		   b2 = (pdata[20] & 0xf0) >> 4;
		   v7 = pdata[21] + (pdata[22] << 8);
		   printf("\t Combined Capacity\t%c %d W (%s and %s)\n",
			bdelim, v7,volt_desc(b1),volt_desc(b2));
		}
		if (b2)   /*predictive fail*/
		   printf("\t Fan low threshold\t%c %d RPS\n",bdelim,pdata[23]);
		break;
	case 0x01:  /*DC Output*/
		b1 = pdata[0] & 0x0f;
		b2 = ((pdata[0] & 0x80) != 0);
		printf("%sDC Output       %c number %d\n",mystr,bdelim,b1);
		printf("\t Standby power \t%c %s\n", bdelim,
			(b2 ? "Yes" : "No"));
		v1 = pdata[1] + (pdata[2] << 8);
		printf("\t Nominal voltage \t%c %.2f V\n", bdelim, (double)v1 / 100);
		v2 = pdata[3] + (pdata[4] << 8);
		v3 = pdata[5] + (pdata[6] << 8);
		printf("\t Voltage deviation \t%c + %.2f V / - %.2f V\n", 
			bdelim, (double)v3/100, (double)v2/100);
		v4 = pdata[7] + (pdata[8] << 8);
		printf("\t Ripple and noise pk-pk \t%c %d mV\n", bdelim, v4);
		v5 = pdata[9] + (pdata[10] << 8);
		printf("\t Min current draw \t%c %.3f A\n", bdelim, (double)v5/1000);
		v6 = pdata[11] + (pdata[12] << 8);
		printf("\t Max current draw \t%c %.3f A\n", bdelim, (double)v6/1000);
		break;
	case 0x02:  /*DC Load*/
		b1 = pdata[0] & 0x0f;
		printf("%sDC Load         %c number %d\n",mystr,bdelim,b1);
		v1 = pdata[1] + (pdata[2] << 8);
		printf("\t Nominal voltage \t%c %.2f V\n", bdelim, (double)v1 / 100);
		v2 = pdata[3] + (pdata[4] << 8);
		v3 = pdata[5] + (pdata[6] << 8);
		printf("\t Min voltage allowed \t%c %.2f A\n", bdelim, (double)v2);
		printf("\t Max voltage allowed \t%c %.2f A\n", bdelim, (double)v3);
		v4 = pdata[7] + (pdata[8] << 8);
		printf("\t Ripple and noise pk-pk \t%c %d mV\n", bdelim, v4);
		v5 = pdata[9] + (pdata[10] << 8);
		printf("\t Min current load \t%c %.3f A\n", bdelim, (double)v5/1000);
		v6 = pdata[11] + (pdata[12] << 8);
		printf("\t Max current load \t%c %.3f A\n", bdelim, (double)v6/1000);
		break;
	case 0x03:  /*Management Access*/
		b1 = pdata[0];
		printf("%sManagemt Access %c %s ",mystr,bdelim,mgt_type(b1));
		memcpy(mystr,&pdata[1],dlen-1);
		mystr[dlen-1] = 0;
		printf("%s\n",mystr);
		break;
	case 0x04:  /*Base Compatibility*/
		vend = pdata[0] + (pdata[1] << 8) + (pdata[2] << 16);
		printf("%sBasic Compat    %c %06x\n",mystr,bdelim,vend);
		break;
	case 0x05:  /*Extended Compatibility*/
		vend = pdata[0] + (pdata[1] << 8) + (pdata[2] << 16);
		printf("%sExtended Compat %c %06x\n",mystr,bdelim,vend);
		break;
	case 0xC0:  /*OEM Extension*/
		vend = pdata[0] + (pdata[1] << 8) + (pdata[2] << 16);
		if (vend == OEM_PICMG) {
		   printf("%sOEM PICMG  %c \n", mystr,bdelim);
		   show_fru_picmg(pdata,dlen);
		} else 
		   printf("%sOEM Ext    %c %06x %02x\n",
			  mystr,bdelim, vend, pdata[3]);
		break;
	default:
		printf("%s %02x %c %02x\n", mystr,mtype,bdelim, pdata[0]);
		break;
   }
}

int
show_fru(uchar sa, uchar frudev, uchar frutype, uchar *pfrubuf)
{
   int ret = 0;
   int i, j, n, sz;
   uchar *pfru0;
   uchar *pfru;
   uchar lang;
   TYPE_LEN tl;
   char newstr[64];
   int iaoff, ialen, bdoff, bdlen; 
   int proff, prlen, choff, chlen;
   int moff, mlen;
   char devstr[24];
   char *pstr;

   if ((pfrubuf[0] & 0x80) == 0x80) {  /* 0x80 = type for DIMMs (SPD) */
      /* FRU Header: 80 08 07 0c 0a 01 48 00 (DIMM) */
      /* FRU Header: 92 10 0b 01 03 1a 02 00 (DDR3 DIMMs) */
      sz = pfrubuf[0];
      if (fdebug) {
	printf("DIMM SPD Body (size=%d/%d): ",sz,sfru);
	dumpbuf(pfrubuf,sfru);
      }
      show_spd(pfrubuf,sfru, frudev,frutype);
      return(ret);
   }

   pstr = FruTypeString(frutype,frudev);
   if (fcanonical) devstr[0] = 0;  /*default is empty string*/
   else sprintf(devstr,"[%s,%02x,%02x] ",pstr,sa,frudev);
   printf("%s%s FRU Size  %c %d\n",devstr,pstr,bdelim,sfru);

   /*
    * FRU header:
    *  0 = format_ver (01 is std, usu 0x80 if DIMM)
    *  1 = internal_use offset
    *  2 = chassis_info offset
    *  3 = board_info offset   (usu 6 fields)
    *  4 = product_info offset (usu 8 fields)
    *  5 = multirecord offset
    *  6 = pad (00)
    *  7 = header checksum (zero checksum)
    * FRU Header: 01 01 02 09 13 00 00 e0 (BMC)
    * FRU Header: 01 00 00 00 01 07 00 f7 (Power Cage)
    */
   pfru0 = &pfrubuf[0];  /*pointer to fru start*/
   pfru = &pfrubuf[0];
   sz = 8;  /*minimum for common header*/
   for (i = 1; i < 6; i++) {	/* walk thru offsets */
	 if (pfrubuf[i] != 0) sz = pfrubuf[i] * 8;
   }
   if (sz > 8) {   		/* if have at least one section */
	 if (pfrubuf[5] != 0) j = 5 + pfrubuf[sz+2]; /*if multi-record area*/
	 else j = pfrubuf[sz+1] * 8;   /* else add length of last section */
	 sz += j;
   }

   /* Now, sz = size used, sfru = total available size */
   if (sz > sfru) {
      if (fdebug) {
        uchar hsum;
        printf("FRU Header: ");
        for (i = 0; i < 8; i++) printf("%02x ",pfrubuf[i]);
        printf("\n");
        hsum = calc_cksum(&pfrubuf[0],7);
        if (pfrubuf[7] != hsum)
           printf("FRU Header checksum mismatch (%x != %x)\n",pfrubuf[7],hsum);
      }
      printf("FRU size used=%d > available=%d\n",sz,sfru);
      if (fpicmg || fdoanyway) sz = sfru;  /*do it anyway*/
      else {
        printf("Please apply the correct FRU/SDR diskette\n");
        return(ERR_OTHER);
      }
   }
   /* internal area offset, length */
   iaoff = pfrubuf[1] * 8;  
   ialen = pfrubuf[iaoff + 1] * 8;
   /* chassis area offset, length */
   choff = pfrubuf[2] * 8;  
   chlen = pfrubuf[choff + 1] * 8;
   /* board area offset, length */
   bdoff = pfrubuf[3] * 8;  
   bdlen = pfrubuf[bdoff + 1] * 8;
   /* product area offset, length */
   proff = pfrubuf[4] * 8;  
   prlen = pfrubuf[proff + 1] * 8;
   /* multi-record area offset, length */
   moff = pfrubuf[5] * 8;  
   mlen = 0;
   if (moff > 0) {
      for (i = moff; i < sfru; ) {
         if (pfrubuf[i] == 0 && pfrubuf[i+2] == 0) break; /*type/len invalid*/
         j = 5 + pfrubuf[i+2];
         mlen += j;
         if (pfrubuf[i+1] & 0x80) break;
         i += j;
      }
   }

   if (fdebug) {
      printf("FRU Header: ");
      for (i = 0; i < 8; i++) printf("%02x ",pfrubuf[i]);
      printf("\n");
      printf("FRU Body (size=%d/%d): ",sz,sfru);
      dumpbuf(pfrubuf,sfru);
      printf("header, len=%d, cksum0 = %02x, cksum1 = %02x\n",
                8,pfrubuf[7],calc_cksum(&pfrubuf[0],7));
      printf("internal off=%d, len=%d, cksum = %02x\n",
		iaoff,ialen,calc_cksum(&pfrubuf[iaoff],ialen-1));
      printf("chassis off=%d, len=%d, cksum = %02x\n",
		choff,chlen,calc_cksum(&pfrubuf[choff],chlen-1));
      printf("board off=%d, len=%d, cksum = %02x\n",
		bdoff,bdlen,calc_cksum(&pfrubuf[bdoff],bdlen-1));
      printf("prod  off=%d, len=%d, cksum = %02x\n",
		proff,prlen,calc_cksum(&pfrubuf[proff],prlen-1));
      /* Multi-record area */
      printf("multi off=%d, len=%d, fru sz=%d\n", moff,mlen,sz);  
   }  /*endif fdebug, show header*/

   if (choff != 0) {
      /* show Chassis area fields */
      pfru = &pfrubuf[choff];
      lang = 25;  /* English */
      ctype = pfru[2];  /*chassis type*/
      if (fdebug) printf("ctype=%x\n",ctype);
      if (ctype >= MAX_CTYPE) ctype = MAX_CTYPE - 1;
      printf("%s%s%c %s\n",devstr, ctype_hdr,bdelim,ctypes[ctype]);
      pfru += 3;  /* skip chassis header */
      tl.len  = 0;
      for (i = 0; i < NUM_CHASSIS_FIELDS; i++)
      {
         if (ChkOverflow((int)(pfru - pfru0),sfru,tl.len)) break;
         if (pfru[0] == FRU_END) break;  /*0xC1 = end of FRU area*/
         if (!ValidTL(pfru[0]))
	    printf("  ERROR - Invalid Type/Length %02x for %s\n",
			pfru[0],chassis[i]);
         tl.type = (pfru[0] & FRU_TYPE_MASK) >> 6;
         tl.len  = pfru[0] & FRU_LEN_MASK;
         if (i == 2) {  /* OEM field for chassis_name  */
	   chassis_offset = (int)(pfru - pfrubuf);
	   chassis_len    = tl.len;
	   if (fdebug) printf("chassis oem dtype=%d lang=%d len=%d\n",
				tl.type,lang,tl.len);
         }
         pfru++;
	 {
	   newstr[0] = 0;
	   decode_string(tl.type,lang,pfru,tl.len,newstr,sizeof(newstr));
	   printf("%s%s%c %s\n",devstr, chassis[i],bdelim,newstr);
         }
         pfru += tl.len;
      }
      if (fdebug) printf("num Chassis fields = %d\n",i);
   }

   if (bdoff != 0) {
      long nMin, nSec;
      time_t tsec;
      /* show Board area fields */
      pfru = &pfrubuf[bdoff];
      lang = pfru[2];
      /* Decode board mfg date-time (num minutes since 1/1/96) */
      nMin = pfru[3] + (pfru[4] << 8) + (pfru[5] << 16);
		  /* 820,454,400 sec from 1/1/70 to 1/1/96 */
      nSec = (nMin * 60) + 820454400;
      tsec = (time_t)(nSec & 0x0ffffffff);
      // fmt_time(tsec,newstr,sizeof(newstr)); 
      printf("%sBoard Mfg DateTime  %c %s",devstr,bdelim,ctime(&tsec));
      pfru += 6;  /* skip board header */
      tl.len  = 0;
      for (i = 0; i < NUM_BOARD_FIELDS; i++)
      {
         if (ChkOverflow((int)(pfru - pfru0),sfru,tl.len)) break;
         if (pfru[0] == FRU_END) break;  /*0xC1 = end*/
         if (!ValidTL(pfru[0]))
	    printf("  ERROR - Invalid Type/Length %02x for %s\n",
			pfru[0],board[i]);
         tl.type = (pfru[0] & FRU_TYPE_MASK) >> 6;
         tl.len  = pfru[0] & FRU_LEN_MASK;
         pfru++;
         {
	   newstr[0] = 0;
	   decode_string(tl.type,lang,pfru,tl.len,newstr,sizeof(newstr));
	   printf("%s%s%c %s\n",devstr, board[i],bdelim,newstr);
         }
         pfru += tl.len;
      }
      if (fdebug) printf("num Board fields = %d\n",i);
   }

   if (proff != 0) 
   {
      /* show Product area fields */
      pfru = &pfrubuf[proff];
      maxprod = pfru[1] * 8;
      lang = pfru[2];
      pfru += 3;  /* skip product header */
      tl.len  = 0;
      for (i = 0; i < NUM_PRODUCT_FIELDS; i++)
      {
         if (ChkOverflow((int)(pfru - pfru0),sfru,tl.len)) break;
         if (*pfru == FRU_END) {  /*0xC1 = end*/
            /* Wart for known Kontron 1-byte Product Version anomaly. */
            if (vend_id == VENDOR_KONTRON && i == 3) ; 
	    else break;
	 }
         if (*pfru == 0) *pfru = FRU_EMPTY_FIELD; /* fix a broken table */
         if (!ValidTL(pfru[0]))
	    printf("  ERROR - Invalid Type/Length %02x for %s\n",
			pfru[0],product[i]);
         tl.type = (pfru[0] & FRU_TYPE_MASK) >> 6;
         tl.len  = pfru[0] & FRU_LEN_MASK;
	 if ((frudev == 0) && (sa == bmc_sa)) {  /*baseboard FRU*/
           if (i == 5) {  /* asset tag */
	      asset_offset = (int)(pfru - pfrubuf);
	      asset_len    = tl.len;
	      if (fdebug) printf("asset off=%d dtype=%d lang=%d len=%d\n",
				 asset_offset,tl.type,lang,tl.len);
   	      if (fshowlen)  /* show asset tag length for main board */
		 printf("%s%c %d\n",asset_hdr,bdelim,asset_len);
           } else if (i == 4) {  /* serial number */
	      sernum_offset = (int)(pfru - pfrubuf);
	      sernum_len    = tl.len;
	      if (fdebug) printf("sernum dtype=%d lang=%d len=%d\n",
				 tl.type, lang, tl.len);
           } else if (i == 3) {  /* product version number */
	      prodver_offset = (int)(pfru - pfrubuf);
	      prodver_len    = tl.len;
	      if (fdebug) printf("prodver dtype=%d lang=%d len=%d\n",
				 tl.type, lang, tl.len);
	   }
	 }  /*if baseboard sa*/
         pfru++;
         {
	   newstr[0] = 0;
	   decode_string(tl.type,lang,pfru,tl.len,newstr,sizeof(newstr));
	   printf("%s%s%c %s\n",devstr, product[i],bdelim,newstr);
         }
         pfru += tl.len;
      }
      if (fdebug) 
	   printf("num Product fields = %d, last=%x, max = %d\n", 
		   i,*pfru,maxprod );
      if (*pfru == 0x00) *pfru = FRU_END;  /* insert end char if broken */
   }

   if (moff != 0) 
   {
      /* multi-record area may contain several record headers 
       * 0 = record type id
       * 1 = 0x02 or 0x82 if End-of-List
       * 2 = record len
       * 3 = record chksum
       * 4 = header chksum
       */
      pfru = &pfrubuf[moff];
      j = moff;
      for (i = 0; j < sz ; i++)
      {
         if (pfru[0] == 0 && pfru[2] == 0) break; /*type/len invalid*/ 
         n = pfru[2];  /* len of this record */
         show_fru_multi(devstr,i,pfru[0],&pfru[5],n);
         j += (5 + n);
         if (pfru[1] & 0x80) j = sz;  /*0x80 = last in list, break*/
         pfru += (5 + n);
      }
   }

   if ((frudev == 0) && (sa == bmc_sa) && do_guid) {
	printf("%sSystem GUID         %c ",devstr,bdelim);
	show_guid(guid);
	printf("\n");
   }
   return(ret);
}

int write_fru_data(uchar id, ushort offset, uchar *data, int dlen, char fdebug)
{
   int ret = -1;
   int chunk;
   ushort fruoff;
   uchar req[FRUCHUNK_SZ+9];
   uchar resp[FRUCHUNK_SZ];
   int sresp;
   uchar cc;
   int i, j;

   /* Write the buffer in small 16-byte (FRUCHUNK_SZ) chunks */
   req[0] = 0x00;  /* override FRU Device ID (fruid) */
   fruoff = offset;
   chunk = FRUCHUNK_SZ;
   for (i = 0; i < dlen; i += chunk) {
	req[1] = fruoff & 0x00ff;
	req[2] = (fruoff & 0xff00) >> 8;
	if ((i + chunk) > dlen) chunk = dlen - i;
	memcpy(&req[3],&data[i],chunk);
	if (fdebug) {
	   printf("write_fru_data[%d] (len=%d): ",i,chunk+3);
	   for (j = 0; j < chunk+3; j++) printf("%02x ",req[j]);
	   printf("\n");
	}
	sresp = sizeof(resp);
	ret = ipmi_cmd_mc(WRITE_FRU_DATA,req,(uchar)(chunk+3),resp,&sresp,
			&cc,fdebug);
	if ((ret == 0) && (cc != 0)) ret = cc & 0x00ff; 
	if (fdebug && ret == 0) 
		printf("write_fru_data[%d]: %d bytes written\n",i,resp[0]);
	if (ret != 0) break;
	fruoff += (ushort)chunk;
   }
   return(ret);
}

/* write_asset updates the FRU Product area only. */
int
write_asset(char *tag, char *sernum, char *prodver, int flag, uchar *pfrubuf)
{
   int ret = -1;
   uchar newdata[SZ_PRODAREA];
   int alen, clen;
   int snlen, verlen;
   uchar *pfru0;
   uchar *pfru;
   uchar *pnew;
   int i, j, m, n, newlen, max; 
   int chas_offset; 
   int prod_offset, prod_len; 
   int mult_offset, mult_len; 
   uchar chksum;
   char fdoasset  = 0;
   char fdosernum = 0;
   char fdoprodver = 0;
   char fdochassis = 0;

   if (flag == 0)  return ERR_OTHER;
   if (pfrubuf == NULL) pfrubuf = frubuf;
   if (pfrubuf == NULL) return ERR_OTHER;
   if ((flag & 0x01) != 0) {
      fdoasset = 1;
      alen = strlen_(tag);  /*new*/
   } else {
      alen = asset_len;  /*old*/
   }
   if ((flag & 0x02) != 0) {
      fdosernum = 1;
      snlen = strlen_(sernum); /*new*/
   } else {
      snlen = sernum_len;  /*old*/
   }
   if ((flag & 0x200) != 0) {
      fdochassis = 1;
      // clen = strlen(chassis_nm); /*new, ignore*/
      // if (clen <= 1) return ERR_LENMIN;
      clen = chassis_len; 
   } else {
      clen = chassis_len;  /*old*/
   }
   if ((flag & 0x08) != 0) {
      fdoprodver = 1;
      verlen = strlen_(prodver); /*new*/
   } else verlen = prodver_len;  /*old*/
   /* 
    * Abort if offset is negative or if it would clobber
    * fru header (8 bytes). 
    * Abort if asset tag is too big for fru buffer.
    */
   if (fdebug) {
       printf("write_asset: asset_off=%d asset_len=%d alen=%d  sFRU=%d\n",
		   asset_offset,asset_len,alen,sfru);
       printf("            sernum_off=%d sernm_len=%d snlen=%d maxprod=%d\n",
		   sernum_offset,sernum_len,snlen,maxprod);
       printf("            prodver=%d prodver_len=%d verlen=%d maxprod=%d\n",
		   prodver_offset,prodver_len,verlen,maxprod);
   }
   if (fdoasset && (alen <= 1)) return ERR_LENMIN;
   if (fdosernum && (snlen <= 1)) return ERR_LENMIN;
   if (asset_offset < 8) return ERR_LENMIN;
   if (asset_offset + alen > sfru) return ERR_LENMAX;
   if (sernum_offset < 8) return ERR_LENMIN;
   if (sernum_offset + snlen > sfru) return ERR_LENMAX;
   if (prodver_offset < 8) return ERR_LENMIN;
   if (prodver_offset + verlen > sfru) return ERR_LENMAX;
   
   pfru0 = &pfrubuf[0]; 
   chas_offset = pfrubuf[2] * 8;           // offset of chassis data
   prod_offset = pfrubuf[4] * 8;           // offset of product data
   prod_len = pfrubuf[prod_offset+1] * 8;      // length of product data 
   mult_offset = pfrubuf[5] * 8;  
   mult_len = 0;
   if (mult_offset > 0) {
      for (i = mult_offset; i < sfru; ) {
         mult_len += (5 + pfrubuf[i+2]);
         if (pfrubuf[i+1] & 0x80) break;
	 i = mult_len;
      }
   }
   /* Check if asset tag will fit in product data area of FRU. */
   if (fdebug) 
     printf("write_asset: fru[4,p]=[%02x,%02x] prod_off=%d plen=%d veroff=%d\n",
	pfrubuf[4],pfrubuf[prod_offset+1],prod_offset,prod_len, prodver_offset);
   /* asset comes after sernum, so this check works for both */
   if (prod_offset > prodver_offset) return ERR_LENMAX; // offsets wrong
   if (prod_len > sizeof(newdata)) return ERR_LENMAX; // product > buffer

   memset(newdata,0,prod_len);
   pnew = &newdata[0];
   /* Copy fru data from start to chassis OEM */
   pfru = &pfrubuf[chas_offset]; 

   /* Copy fru data before serial number */
   pfru = &pfrubuf[prod_offset]; 
   j = prodver_offset - prod_offset;
   memcpy(pnew,pfru,j);
   pfru += j; 
   pnew += j;
   if (fdebug) {
	printf("write_asset: fru[4,p]=[%02x,%02x] sernum_off=%d snlen=%d plen=%d\n",
		pfrubuf[4],pfrubuf[sernum_offset+1],sernum_offset,snlen,prod_len);
   }
   if (fdoprodver) {
      pnew[0] = (verlen | FRU_TYPE_MASK);  /*add type=3 to snlen*/
      memcpy(++pnew,prodver,verlen);
   } else {
      pnew[0] = (verlen | FRU_TYPE_MASK);  /*type/len byte*/
      memcpy(++pnew,&pfrubuf[prodver_offset+1],verlen);
   }
   j += prodver_len + 1;
   pfru += prodver_len + 1;
   pnew += verlen;  /*already ++pnew above*/

   /* Copy new or old serial number */
   if (fdebug) printf("pfrubuf[%ld]: %02x %02x %02x, j=%d, snlen=%d/%d\n",
		   (pfru-pfrubuf),pfru[0],pfru[1],pfru[2],j,snlen,sernum_len);
   if (fdosernum) {
      pnew[0] = (snlen | FRU_TYPE_MASK);  /*add type=3 to snlen*/
      memcpy(++pnew,sernum,snlen);
   } else {
      pnew[0] = (snlen | FRU_TYPE_MASK);  /*type/len byte*/
      memcpy(++pnew,&pfrubuf[sernum_offset+1],snlen);
   }
   /* increment past serial number, to asset tag */
   j += sernum_len + 1;
   pfru += sernum_len + 1;
   pnew += snlen;  /*already ++pnew above*/

   /* Copy new or old asset tag */
   if (fdebug) printf("pfrubuf[%ld]: %02x %02x %02x, j=%d, alen=%d/%d\n",
		   (pfru-pfrubuf),pfru[0],pfru[1],pfru[2],j,alen,asset_len);
   if (fdoasset) {
      pnew[0] = (alen | FRU_TYPE_MASK);  /*add type=3 to len*/
      memcpy(++pnew,tag,alen);
   } else {
      pnew[0] = (alen | FRU_TYPE_MASK);  /*type/len byte*/
      memcpy(++pnew,&pfrubuf[asset_offset+1],alen);
   }
   j += asset_len + 1;
   pfru += asset_len + 1;
   pnew += alen;

   /* copy trailing fru data from saved copy */
   n = (int)(pnew - newdata);  /* new num bytes used */
   m = (int)(pfru - pfru0);    /* old num bytes used */
   if (fdebug) printf("pfrubuf[%d]: %02x %02x %02x, j=%d, n=%d, plen=%d\n",
			m,pfru[0],pfru[1],pfru[2],j,n,prod_len);
   if (mult_offset > 0) {  /* do not expand if multi-record area there */
       max = prod_len - j;
   } else /* nothing else, can expand product area, up to sfru */
       max = sfru - (j + prod_offset); 
   if (fdebug) printf("pfrubuf[%ld]: %02x %02x %02x, j=%d n=%d remainder=%d\n",
		   (pfru-pfrubuf),pfru[0],pfru[1],pfru[2],j,n,max);
   if (max < 0) max = 0;
   for (i = 0; i < max; i++) {
     pnew[i] = pfru[i];
     if (pfru[i] == FRU_END) { i++; break; }
   }
   if (i == max) {  /*never found 0xC1 FRU_END*/
     pnew[0] = FRU_END;
     i = 1;
   } 

   newlen = n + i;
   if (fdebug) printf("newbuf[%d]: %02x %02x %02x, j=%d, newlen=%d\n",
		   n,pnew[0],pnew[1],pnew[2],j,newlen);
   /* round up to next 8-byte boundary */
   /* need one more byte for checksum, so if mod=0, adds 8 anyway */
   j = 8 - (newlen % 8);
   for (i = 0; i < j; i++) newdata[newlen++] = 0;

   if (newlen < prod_len) newlen = prod_len;  /* don't shrink product area */
   newdata[1] = newlen / 8;     // set length of product data 

   /* include new checksum (calc over Product area) */
   chksum = calc_cksum(&newdata[0],newlen-1);
   newdata[newlen-1] = chksum;

#ifdef NEEDED
   /* This caused a problem on Radisys systems, and is not needed, so skip it.*/
   if (mult_offset > 0) {
      /* add on any more data from multi-record area */
      pnew = &newdata[newlen];
      pfru = &pfrubuf[mult_offset];
      memcpy(pnew,pfru,mult_len);
      newlen += mult_len;
   }
#endif

   if (fdebug) {
	printf("old buffer (%d):",prod_len);
	dumpbuf(&pfrubuf[prod_offset],prod_len);
	printf("new buffer (%d):",newlen);
	dumpbuf(newdata,newlen);
   }
   if (prod_offset + newlen >= sfru) return ERR_LENMAX;
   if ((mult_offset > 0) && (newlen > prod_len)) return ERR_LENMAX;
#ifdef TEST
   newlen = 0;
#endif

   ret = write_fru_data(0, (ushort)prod_offset, newdata, newlen, fdebug);
   return(ret);
}

#define SDR_CHUNKSZ  16
#define LAST_REC   0xffff
#define SDR_STR_OFF  16

int get_sdr(ushort recid, ushort resid, ushort *recnext, 
		uchar *sdr, int *slen, uchar *pcc)
{
	uchar idata[6];
	uchar rdata[64];
	int sresp;
	ushort cmd;
	uchar cc = 0;
        int len = 0;
	int szsdr, thislen;
	int rc;
	
	memset(sdr,0,*slen); /*clear sdr */
	idata[0] = resid & 0x00ff;
	idata[1] = (resid & 0xff00) >> 8;
	idata[2] = recid & 0x00ff;
	idata[3] = (recid & 0xff00) >> 8;
	idata[4] = 0;  /*offset*/
	idata[5] = SDR_CHUNKSZ;  /*bytes to read*/
        if (fdevsdrs) cmd = GET_DEVICE_SDR;
        else cmd = GET_SDR;
	sresp = sizeof(rdata);
	rc = ipmi_cmd_mc(cmd, idata, 6, rdata, &sresp,&cc, fdebug);
	if (fdebug) printf("get_sdr[%x] ret = %d cc = %x sresp = %d\n",
				recid,rc,cc,sresp);
	if (rc == 0 && cc == 0xCA) {  /*cannot read SDR_CHUNKSZ bytes*/
	   idata[5] = 8;  /*bytes to read*/
	   sresp = sizeof(rdata);
	   rc = ipmi_cmd_mc(cmd, idata, 6, rdata, &sresp,&cc, fdebug);
	   if (fdebug) printf("get_sdr[%x] ret = %d cc = %x sresp = %d\n",
				recid,rc,cc,sresp);
	}
	*pcc = cc;
	if (rc == 0 && cc == 0) {
	   *recnext = rdata[0] + (rdata[1] << 8);
           if (sresp < 2) len = 0;
	   else len = sresp-2;
	   if (len > *slen) len = *slen;
	   memcpy(sdr,&rdata[2],len);
           szsdr = sdr[4] + 5;  /*get actual SDR size*/
	   if (szsdr > *slen) szsdr = *slen;
           /* if an SDR locator record, get the rest of it. */
	   if (sdr[3] == 0x11 || sdr[3] == 0x12)
	   if (szsdr > SDR_CHUNKSZ) {
		ushort resid2;
		rc = sdr_get_reservation((uchar *)&resid2,fdevsdrs);
		if (fdebug) printf("2nd sdr_get_reservation ret=%d\n",rc);
		thislen = szsdr - SDR_CHUNKSZ;
		if (thislen > SDR_CHUNKSZ) thislen = SDR_CHUNKSZ;
		idata[0] = resid2 & 0x00ff;
		idata[1] = (resid2 & 0xff00) >> 8;
		idata[2] = recid & 0x00ff;
		idata[3] = (recid & 0xff00) >> 8;
		idata[4] = SDR_CHUNKSZ;  /*offset*/
		idata[5] = (uchar)thislen;  /*bytes to read*/
		sresp = sizeof(rdata);
		rc = ipmi_cmd_mc(cmd, idata, 6, rdata, &sresp,&cc, fdebug);
		if (fdebug) printf("get_sdr[%x] 2nd ret=%d cc=%x sresp=%d\n",
				recid,rc,cc,sresp);
		if (rc == 0 && cc == 0) {
		   if (sresp < 2) sresp = 0;
		   else sresp -= 2;
		   if (sresp > thislen) sresp = thislen;
	   	   memcpy(&sdr[len],&rdata[2],sresp);
		   len += sresp;
		} else rc = 0;  /*ok because got first part*/
	   }  /*endif got first chunk, need more*/
	   *slen = len;
	} else *slen = 0;  
	return(rc);
}

void show_loadfru_error(uchar sa, uchar fruid, int ret)
{
   if (ret == 0) return;
   switch(ret) {
      case 0x081: printf("\tFRU(%x,%x) device busy\n",sa,fruid); break;
      case 0x0C3: printf("\tFRU(%x,%x) timeout, not found\n",sa,fruid); break;
      case 0x0CB: printf("\tFRU(%x,%x) not present\n",sa,fruid); break;
      default:    printf("load_fru(%x,%x) error = %d (0x%x)\n",
			sa,fruid,ret,ret);
		  break;
   }
   return;
}

int get_show_fru(ushort recid, uchar *sdr, int sdrlen)
{
   int ret = 0;
   int ilen;
   char idstr[32];
   uchar sa, fruid =0;
   uchar frutype = 0;
   char fgetfru = 0;
   char fisbase = 0;
   uchar *pfru;

   if (sdrlen > SDR_STR_OFF) {
	ilen = sdrlen - SDR_STR_OFF;
	if (ilen >= sizeof(idstr)) ilen = sizeof(idstr) - 1;
	memcpy(idstr,&sdr[SDR_STR_OFF],ilen);
	idstr[ilen] = 0;
   } else idstr[0] = 0;
   sa = sdr[5];  /* usu 0x20 for bmc_sa */
   if (fbasefru) fisbase = 1;

	    /* Get its FRU data */
	    if ((sdr[3] == 0x11) && (sdr[7] & 0x80)) {  /* FRU SDRs */
	      /* It is a logical FRU device */
	      if (fcanonical)
	         printf("SDR[%04x] FRU  %c %s\n", recid, bdelim, idstr);
	      else
	         printf("SDR[%04x] FRU  %02x %02x %02x %02x %s\n", recid, 
			sdr[5],sdr[6],sdr[12],sdr[13],idstr);
	      fruid = sdr[6];
	      if (sdrlen > 12) frutype = sdr[12];
              if (sa == bmc_sa && fruid == 0) { /*dont repeat base fru */
		frutype = 0x0c;
		fisbase = 1;
                if (fbasefru) fgetfru = 1;
              } else
                switch(frutype)   /*FRU entity id*/
                {
                  case 0x0a:   /*Power Supply*/
                  case 0x20:   /*DIMM*/
                  case 0x15:   /*Power Cage*/
                  default:
                        fgetfru = 1;
                        break;
                }
            } else if (sdr[3] == 0x12) {  /* IPMB SDRs (DLRs for MCs) */
	        if (fcanonical)
	           printf("SDR[%04x] IPMB %c %s\n", recid, bdelim, idstr);
		else
                   printf("SDR[%04x] IPMB %02x %02x %02x %02x %s\n", recid,
                        sdr[5],sdr[6],sdr[12],sdr[13],idstr);
                fruid = 0;  /*every MC must have fruid 0*/
	      if (sdrlen > 12) frutype = sdr[12];
                if (sa == bmc_sa && fruid == 0) { /*dont repeat base fru */
		   fisbase = 1;
		   frutype = 0x07;
                   if (fdebug) printf("do bmc_sa %02x once\n",sa);
		   g_frutype = frutype;
                   if (fbasefru) fgetfru = 1;
		} else if (frutype == 0x2e) { /*skip ME*/
                   if (fdebug) printf("skipping ME sa %02x, %02x\n",sa,fruid);
                } else if (sa == 0x28) { /*do nothing for Bridge Ctlr sa=0x28*/
                   if (fdebug) printf("skipping IPMB sa %02x, %02x\n",sa,fruid);
                } else if (sa == 0xC0) { /* HotSwap Backplane (sa=0xC0) */
                   /* Note: Loading sa 0xC0 over ipmi lan gives a timeout
                    * error, but it works locally. */
                   fgetfru = 1;
                } else  { /* other misc sa,fruid */
                   fgetfru = 1;
                }
            }
	
	    /* Check if matches previous FRU, if so don't repeat */
	    if ((sa == lastfru[0]) && (fruid == lastfru[1]) )
		fgetfru = 0;  

            if (fgetfru) {
                uchar adrtype;
                adrtype = g_addrtype;
                if (fdebug) printf("set_mc %02x:%02x:%02x type=%d fruid=%02x\n",
				g_bus,sa,g_lun,adrtype,fruid);
                ipmi_set_mc(g_bus, sa, g_lun,adrtype);
                ret = load_fru(sa,fruid,frutype,&pfru);
                if (ret != 0) {
		   show_loadfru_error(sa,fruid,ret);
                } else {
                   ret = show_fru(sa,fruid,frutype,pfru);
                   if (ret != 0) printf("show_fru error = %d\n",ret);
                   if (sa == bmc_sa && fruid == 0) fbasefru = 0;
                }
		free_fru(pfru);
		pfru = NULL;
                ipmi_restore_mc();
		lastfru[0] = sa;
		lastfru[1] = fruid;
		lastfru[2] = frutype;
            }
   return(ret);
}

/*
 * test_show_fru
 *
 * Reads the FRU buffer dump from a file to test decoding the FRU.
 * This file uses the format output from 'ipmiutil fru -x'.
 *
[Component,20,00] Component FRU Size  : 4096
FRU Header: 01 0a 00 01 20 00 00 d4
FRU Body (size=256/4096):
  0000: 01 0a 00 01 20 00 00 d4 01 09 00 c0 3b 7e 83 64 .... .......;~.d
 *
 */
static int test_show_fru(char *infile)
{
   int rv = -1;
   FILE *fp;
   int len, i, idx, sz, off;
   char buff[256];
   uchar sa = 0x20; 
   uchar fruid =0;
   uchar frutype = 0;
   uchar *pfru = NULL;
   char *p1;
   char *p2;

   fp = fopen(infile,"r");
   if (fp == NULL) {
	printf("Cannot open file %s\n",infile);
	return(ERR_FILE_OPEN);
   } else {
	if (fdebug) printf("decoding text file with FRU buffer bytes\n");
	idx = 0;
	off = 0;
	sz = sizeof(buff);
        while (fgets(buff, 255, fp)) {
   	       len = strlen_(buff);
	       if (fdebug) printf("fgets[%d]: %s",idx,buff); /*has '\n'*/
	       if (idx == 0) {   /* the FRU Size line*/
		   p1 = strstr(buff,"FRU Size");
		   if (p1 == NULL) continue; 
		   p1 = strchr(buff,',');
		   if (p1 == NULL) continue; 
		   p1++;
		   p2 = strchr(p1,',');
		   if (p2 == NULL) p2 = &buff[len];
 		   *p2 = 0;  /*stringify*/
		   sa = htoi(p1);
		   p1 = p2 + 1;
		   p2 = strchr(p1,']');
		   if (p2 == NULL) p2 = &buff[len];
 		   *p2 = 0;  /*stringify*/
		   fruid = htoi(p1);
		   frutype = 0;
		   if (fdebug) printf("read[%d] sa=%x fruid=%x\n",idx,sa,fruid);
		   idx++;
	       } else if (idx == 1) {   /* the FRU Header line*/
		   p1 = strstr(buff,"FRU Header");
		   // if (p1 == NULL) continue;  
		   if (fdebug) printf("read[%d] p1=%p\n",idx,p1);
		   idx++;
	       } else if (idx == 2) {   /* the FRU Bodu line*/
		   p1 = strstr(buff,"FRU Body");
		   if (p1 == NULL) continue;
		   p1 = strchr(buff,'=');
		   if (p1 == NULL) continue;
		   ++p1;
		   p2 = strchr(p1,'/');
		   if (p2 == NULL) p2 = &buff[len];
 		   *p2 = 0;  /*stringify*/
		   sz = atoi(p1);
		   p1 = p2 + 1;
		   p2 = strchr(p1,')');
		   if (p2 == NULL) p2 = &buff[len];
 		   *p2 = 0;  /*stringify*/
		   sfru = atoi(p1);  /*global sfru*/
		   pfru = malloc(sfru);
		   if (fdebug) 
			printf("read[%d] sz=%d/%d pfru=%p\n",idx,sz,sfru,pfru);
		   idx++;
	       } else {  /*idx==3, this has the FRU buffer data */
		   p1 = strchr(buff,':');
		   if (p1 == NULL) continue;
		   p1 += 2;
		   for (i = 0; i < 16; i++) {
		      pfru[off+i] = htoi(&p1[i*3]);
   	           }
		   off += 16;
		   if (fdebug) printf("read[%d] offset=%d\n",idx,off);
	       } /*endif */
	       if (off >= sz) { rv = 0; break; }
        } /*end while*/
                   
	fclose(fp);
        if (rv == 0) {
	   if (pfru == NULL) {
		printf("Invalid input file format (mode %d)\n",idx); 
		return(ERR_BAD_FORMAT);
	   }
           rv = show_fru(sa,fruid,frutype,pfru);
           if (rv != 0) printf("show_fru error = %d\n",rv);
	}
        free_fru(pfru);
   }
   return(rv);
}

#ifdef ALONE
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int  argc, char **argv)
#else
/* METACOMMAND */
int i_fru(int argc, char **argv)
#endif
{
   int ret, rv;
   int c;
   uchar DevRecord[16];
   ushort recid;
   ushort nextid;
   ushort rsvid;
   uchar sdr[48];
   char biosver[80];
   uchar cc;
   uchar sa;
   //uchar fruid = 0;
   //uchar frutype = 0;
   int len, i, j;
   char *s1;
   uchar *pfru = NULL;
   FILE *fp;
   int ipass, npass, nsdrs;
   char *tag;
   char do_reserve = 1;
   char devstr[32];

   printf("%s version %s\n",progname,progver);
          
   parse_lan_options('V',"4",0);  /*default to admin privilege*/
   while ( (c = getopt( argc, argv,"a:bcd:efhkl:m:n:i:p:r:s:t:v:xyzT:V:J:EYF:P:N:R:U:Z:?")) != EOF )
      switch(c) {
          case 'x': fdebug = 1;  break;
          case 'z': fdebug = 3;  break; /*do more LAN debug detail*/
          case 'b': fonlybase = 1; 
		    g_frutype = 0x07;  break;
          case 'c': fcanonical = 1; bdelim = BDELIM; break;
          case 'd': fdump = 1;      /*dump fru to a file*/
                binfile = optarg;
                break;
          case 'e': fchild = 1;  break;  /*extra child MCs if bladed*/
          case 'f': fchild = 1;  break;  /*follow child MCs if bladed*/
          case 'h': fonlyhsc = 1;     /* show HSC FRU, same as -m00c000s */
		    g_frutype = 0x0f;   break;
          case 'k': foemkontron = 1;  break;
          case 'n': 
		fwritefru |= 0x200; 
		if (optarg) {
                   len = strlen_(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
                   strncpy(chassis_name,optarg,len);
                   if (len == 1) {  /* add a space */
                       chassis_name[1] = ' ';
                       chassis_name[2] = 0;
                   }
                }
		break;
          case 'p':  /*Power Supply Product */
		// fwritefru |= 0x10; 
		if (optarg) {
                   len = strlen_(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
                   strncpy(ps_prod,optarg,len);
                   if (len == 1) {  /* add a space */
                       ps_prod[1] = ' ';
                       ps_prod[2] = 0;
                   }
                }
		break;
          case 'r': frestore = 1;   /*restore fru from a file*/
                fwritefru = 0x100;
                binfile = optarg;
                break;
          case 't': ftestshow = 1;  /*test show_fru from a file*/
                binfile = optarg;
                break;
          case 'a':
		fwritefru |= 0x01;
		if (optarg) {
                   len = strlen_(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
                   strncpy(asset_tag,optarg,len);
                   if (len == 1) {  /* add a space */
                       asset_tag[1] = ' ';
                       asset_tag[2] = 0;
                   }
                }
		break;
          case 's':
		fwritefru |= 0x02; 
		if (optarg) { 
                   len = strlen_(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
                   strncpy(serial_num,optarg,len);
                   if (len == 1) {  /* add a space */
                       serial_num[1] = ' ';
                       serial_num[2] = 0;
                   }
                }
		break;
          case 'v':
		fwritefru |= 0x08; 
		if (optarg) { 
                   len = strlen_(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
                   strncpy(prod_ver,optarg,len);
                   if (len == 1) {  /* add a space */
                       prod_ver[1] = ' ';
                       prod_ver[2] = 0;
                   }
                }
		break;
          case 'm': /* specific IPMB MC, 3-byte address, e.g. "409600" */
                    g_bus = htoi(&optarg[0]);  /*bus/channel*/
                    g_sa  = htoi(&optarg[2]);  /*device slave address*/
                    g_lun = htoi(&optarg[4]);  /*LUN*/
                    g_addrtype = ADDR_IPMB;
		    if (optarg[6] == 's') {
		             g_addrtype = ADDR_SMI;  s1 = "SMI";
		    } else { g_addrtype = ADDR_IPMB; s1 = "IPMB"; }
		    fset_mc = 1;
		    printf("set MC at %s bus=%x sa=%x lun=%x\n",
		            s1,g_bus,g_sa,g_lun);
                    break;
          case 'l': /* set local IPMB MC sa (same as -Z but sets bmc_sa) */
                    sa  = htoi(&optarg[0]);  /*device slave address*/
		    ipmi_set_mymc(g_bus, sa, g_lun,ADDR_IPMB);
		    bmc_sa = sa;
                    break;
          case 'i': fonlybase = 1;   /*specify a fru id*/
		    if (strncmp(optarg,"0x",2) == 0) g_fruid = htoi(&optarg[2]);
		    else g_fruid = htoi(optarg); 
		    printf("Using FRU ID 0x%02x\n",g_fruid);
                    break;
          case 'y': fdoanyway = 1;  break;
          case 'V':    /* priv level */
		    fprivset = 1;
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
                printf("Usage: %s [-bceikmtvx -a asset_tag -s ser_num -NUPREFTVYZ]\n",
			 progname);
		printf("   -a tag   Sets the Product Asset Tag\n");
		printf("   -b       Only show Baseboard FRU data\n");
		printf("   -c       show canonical, delimited output\n");
		printf("   -d file  Dump the binary FRU data to a file\n");
		printf("   -e       walk Every child FRU, for blade MCs\n");
		printf("   -i 00    Get a specific FRU ID\n");
		printf("   -k       Kontron setsn, setmfgdate\n");
                printf("   -m002000 specific MC (bus 00,sa 20,lun 00)\n");
		printf("   -s snum  Sets the Product Serial Number\n");
		printf("   -v pver  Sets the Product Version Number\n");
		printf("   -x       Display extra debug messages\n");
		print_lan_opt_usage(0);
		ret = ERR_USAGE;
		goto do_exit; 
      }

   if (ftestshow) {
       ret = test_show_fru(binfile);
       goto do_exit;
   }

   if (is_remote() && (!fprivset)) {
       /* If priv not specified, not writing, and only targetted FRU, 
          can default to request user privilege. */
       if (!fwritefru && fonlybase) parse_lan_options('V',"2",0);
   }
 
   ret = ipmi_getdeviceid( DevRecord, sizeof(DevRecord),fdebug);
   if (ret == 0) {
      uchar ipmi_maj, ipmi_min;
      ipmi_maj = DevRecord[4] & 0x0f;
      ipmi_min = DevRecord[4] >> 4;
      vend_id = DevRecord[6] + (DevRecord[7] << 8) + (DevRecord[8] << 16);
      prod_id = DevRecord[9] + (DevRecord[10] << 8);
      show_devid( DevRecord[2],  DevRecord[3], ipmi_maj, ipmi_min);
      if (ipmi_maj < 2) do_systeminfo = 0;
      if ((DevRecord[1] & 0x80) == 0x80) fdevsdrs = 1;
      if (vend_id == VENDOR_NEC) fdevsdrs = 0;
      else if (vend_id == VENDOR_SUN) {
	  do_guid = 0;       /*Sun doesn't support GUID*/
	  do_systeminfo = 0; /*Sun doesn't support system info*/
      } else if (vend_id == VENDOR_INTEL) {
	 if (is_thurley(vend_id,prod_id) || is_romley(vend_id,prod_id)) 
		set_max_kcs_loops(URNLOOPS); /*longer for cmds (default 300)*/
      }
   } else { 
	goto do_exit; 
   }

   ret = ipmi_getpicmg( DevRecord, sizeof(DevRecord),fdebug);
   if (ret == 0)  fpicmg = 1;
   if (!fpicmg) fdevsdrs = 0; /* override, use SDR repository for FRU */
   if (fdebug) printf("bmc_sa = %02x, fdevsdrs = %d\n",bmc_sa,fdevsdrs);

   if (fset_mc) {
       /* target a specific MC via IPMB (usu a picmg blade) */
       if (fdebug) printf("set_mc: %02x:%02x:%02x type=%d\n",
		          g_bus,g_sa,g_lun,g_addrtype);
       ipmi_set_mc(g_bus,g_sa,g_lun,g_addrtype);
       fonlybase = 1;   /*only show this MC*/
   } else {
       g_sa  = bmc_sa;  /* BMC_SA = 0x20 */
   }
   if (g_frutype == 0) {
       g_frutype = 0x01;   /* other = "Component" */
   }

   if (foemkontron) {
#ifdef METACOMMAND
       for (i = 0; i < optind; i++) { argv++; argc--; }
       if (fdebug) verbose = 1;
       ret = ipmi_kontronoem_main(NULL,argc,argv);
#else
       ret = LAN_ERR_NOTSUPPORT;
#endif
       goto do_exit;
   }
   npass = 1;

   if (fonlybase || fonlyhsc) fbasefru = 1;
   else {
     /* find FRU devices from SDRs */
     if (fpicmg && fdevsdrs) { 
	/* npass = 2 will get both SdrRep & DevSdr passes on CMM */
        npass = 2;
        g_addrtype = ADDR_IPMB;
     }

     nsdrs = 0;
     for (ipass = 0; ipass < npass; ipass++)
     {
       ret = GetSDRRepositoryInfo(&j,&fdevsdrs);
       if (fdebug) printf("GetSDRRepositoryInfo: ret=%x nSDRs=%d fdevsdrs=%d\n",
			   ret,j,fdevsdrs);
       if (j == 0) {
          /* this is an error, probably because fdevsdrs is wrong.*/
          fdevsdrs = (fdevsdrs ^ 1);
          if (fdebug) printf("nsdrs=0, retrying with fdevsdrs=%d\n",fdevsdrs);
          j = 60; /*try some default num SDRs*/
       }
       nsdrs = j;
       if (fdevsdrs) tag="Device SDRs";
       else tag="SDR Repository";
       printf("--- Scanning %s for %d SDRs ---\n",tag,nsdrs);

       /* loop thru SDRs to find FRU devices */
       recid = 0;
       while (recid != LAST_REC)
       {
         if (do_reserve) {
             /* reserve the SDR repository */
             ret = sdr_get_reservation((uchar *)&rsvid,fdevsdrs);
             if (fdebug) printf("sdr_get_reservation ret=%d\n",ret);
             if (ret == 0) do_reserve = 0;
         }

         len = sizeof(sdr); /*sizeof(sdr); get 32 sdr bytes*/
         ret = get_sdr(recid,rsvid,&nextid,sdr,&len,&cc);
         if ((ret != 0) || (cc != 0)) {
             printf("SDR[%04x] error %d ccode = %x\n",recid,ret,cc);
             if (cc == 0xC5) do_reserve = 1; /*retry w reserve*/
             if (cc == 0x83) os_usleep(0,100); /*busy, retry*/
             else break;  /*stop if errors*/
         }
         if (len >= MIN_SDR_SZ) {
	        if ((sdr[3] == 0x11) || (sdr[3] == 0x12)) /*SDR FRU or IPMB type*/
	           ret = get_show_fru(recid, sdr,len);
	        do_reserve = 1;
         }  /*endif get_show_fru */
#ifdef PICMG_CHILD
           /*
            * Special logic for blade child MCs in PICMG ATCA systems 
            * if fchild, try all child MCs within the chassis.
            * SDR type 12 capabilities bits (sdrdata[8]):
            *    80 = Chassis Device
            *    40 = Bridge
            *    20 = IPMB Event Generator
            *    10 = IPMB Event Receiver
            *    08 = FRU Device
            *    04 = SEL Device
            *    02 = SDR Repository Device
            *    01 = Sensor Device
            *    But all child MCs use Device SDRs anyway.
            */
           if (fpicmg && fchild && (sdr[3] == 0x12)) { /* PICMG MC DLR */
              ushort  _recid, _recnext; 
	      int _sz;
              uchar _sdrdata[MAX_SDR_SIZE];
              int devsdrs_save;
              uchar cc;

              /* save the BMC globals, use IPMB MC */
              devsdrs_save  = fdevsdrs;
              fdevsdrs = 1;   /* use Device SDRs for the children*/
              if (fdebug)
                printf(" --- IPMB MC (sa=%02x cap=%02x id=%02x devsdrs=%d):\n",
                       sdr[5],sdr[8],sdr[12],fdevsdrs);
              ipmi_set_mc(PICMG_SLAVE_BUS,sdr[5],sdr[6],g_addrtype);

              _sz = 16;
              ret = ipmi_cmd_mc(GET_DEVICE_ID,NULL,0,_sdrdata,&_sz,&cc,fdebug);
              if (ret == 0 && cc == 0) {
                /* get a new SDR Reservation ID */
	        ret = sdr_get_reservation((uchar *)&rsvid,fdevsdrs);
		if (fdebug) printf("sdr_get_reservation ret=%d\n",ret);
                /* Get the SDRs from the IPMB MC */
                _recid = 0;
                while (_recid != 0xffff) 
                {
                  _sz = sizeof(_sdrdata);
		  ret = get_sdr(_recid,rsvid,&_recnext,_sdrdata,&_sz,&cc);
		  if (fdebug) printf("get_sdr(%x) rv=%d cc=%x rlen=%d\n",
					_recid,ret,cc,_sz);
  	          if (ret != 0) {
  	             printf("%04x get_sdr error %d, rlen=%d\n",_recid,ret,_sz);
                     break;
                  }
                  else if (_sz >= MIN_SDR_SZ) {
		     if ((_sdrdata[3] == 0x11) || (_sdrdata[3] == 0x12)) 
			ret = get_show_fru(_recid, _sdrdata, _sz);
		  }
                  if (_recnext == _recid) _recid = 0xffff;
                  else _recid = _recnext;
                } /*end while*/
              } /*endif ret==0*/

              /* restore BMC globals */
              fdevsdrs = devsdrs_save;
              ipmi_restore_mc();
	      do_reserve = 1; /* get a new SDR Reservation ID */
           }  /*endif fpicmg && fchild*/
#endif

           recid = nextid;
        } /*end while sdrs*/
        if (npass > 1) {   /*npass==2 for PICMG child*/
           /* Switch fdevsdrs from Device to Repository (or vice-versa) */
           if (fdevsdrs == 0) fdevsdrs = 1;
           else fdevsdrs = 0;
        }
     } /*end ipass loop*/
   } /*endif not fonlybase*/
 
   /* load the FRU data for Baseboard (address 0x20) */
   printf("\n");
   sa = g_sa;  /* bmc_sa = BMC_SA = 0x20 */
   if (fonlyhsc) { sa = 0xC0;  g_addrtype = ADDR_SMI; 
      ipmi_set_mc(g_bus,sa,g_lun,g_addrtype); 
   }
   if (g_addrtype == ADDR_IPMB) 
      ipmi_set_mc(g_bus,sa,g_lun,g_addrtype);

   if (fbasefru) {
      /* get and display the Baseboard FRU data */
      ret = load_fru(sa,g_fruid,g_frutype,&pfru);
      if (ret != 0) {
	 show_loadfru_error(sa,g_fruid,ret);
	 free_fru(pfru);
	 pfru = NULL;
 	 goto do_exit;
      }
      ret = show_fru(sa,g_fruid,g_frutype,pfru);
      if (ret != 0) printf("show_fru error = %d\n",ret);
   }

   if (fcanonical) devstr[0] = 0;  /*default is empty string*/
   else sprintf(devstr,"[%s,%02x,%02x] ", /*was by g_frutype*/
		FruTypeString(SYS_FRUTYPE,g_fruid),sa,g_fruid);

   if (!is_remote()) {
	i = get_BiosVersion(biosver);
	if (i == 0) 
		printf("%sBIOS Version        %c %s\n",devstr,bdelim,biosver);
   }

   if (do_systeminfo) {
      char infostr[64];
      int len;
      len = sizeof(infostr);
      rv = get_system_info(1,infostr,&len);  /*Firmware Version*/
      if (rv == 0) {
         len = sizeof(infostr);
         rv = get_system_info(2,infostr,&len);
         if (rv == 0) printf("%sSystem Name         %c %s\n",devstr,bdelim,infostr);
         len = sizeof(infostr);
         rv = get_system_info(3,infostr,&len);
         if (rv == 0) printf("%sPri Operating System%c %s\n",devstr,bdelim,infostr);
         len = sizeof(infostr);
         rv = get_system_info(4,infostr,&len);
         if (rv == 0) printf("%sSec Operating System%c %s\n",devstr,bdelim,infostr);
      } else {
         if (fdebug && (rv == 0xC1)) /*only supported on later IPMI 2.0 fw */
            printf("GetSystemInfo not supported on this platform\n");
      }
   }

   if (fdump && ret == 0) {
      ret = load_fru(sa,g_fruid,g_frutype,&pfru);
      if (ret != 0) {
	 show_loadfru_error(sa,g_fruid,ret);
	 free_fru(pfru);
	 pfru = NULL;
 	 goto do_exit;
      }
      /* Dump FRU to a binary file */
#ifdef WIN32
      fp = fopen(binfile,"wb");
#else
      fp = fopen(binfile,"w");
#endif
      if (fp == NULL) {
         ret = get_LastError();
         printf("Cannot open file %s, error %d\n",binfile,ret);
      } else {
         printf("Writing FRU size %d to %s  ...\n",sfru,binfile);
         len = (int)fwrite(frubuf, 1, sfru, fp);
         fclose(fp);
         if (len <= 0) {
            ret = get_LastError();
            printf("Error %d writing file %s\n",ret,binfile);
         } else ret = 0;
      }
      goto do_exit;
   }
   else if (frestore) {
      uchar cksum;
      /* Restore FRU from a binary file */
#ifdef WIN32
      fp = fopen(binfile,"rb");
#else
      fp = fopen(binfile,"r");
#endif
      if (fp == NULL) {
         ret = get_LastError();
         printf("Cannot open file %s, error %d\n",binfile,ret);
      } else {
	 ret = 0;
	 /* sfru and frubuf were set from load_fru above. */
	 len = (int)fread(frubuf, 1, sfru, fp);
	 if (len <= 0) { 
	     ret = get_LastError();
             printf("Error %d reading file %s\n",ret,binfile);
	     sfru = 0;	/*for safety*/
	 }
         fclose(fp);
         if (fdebug) {
	    printf("FRU buffer from file (%d):",sfru);
	    dumpbuf(frubuf,sfru);
         }
	 /* Do some validation of the FRU buffer header */
	 cksum = calc_cksum(&frubuf[0],7);
         if (fdebug) 
	     printf("header, len=8, cksum0 = %02x, cksum1 = %02x\n",
		    frubuf[7],cksum); 
	 if (frubuf[7] != cksum) {
	    printf("Not a valid FRU file\n");
	    ret = ERR_BAD_FORMAT;
            free_fru(frubuf);
	 }
	 if (ret == 0) {  /*successfully read data*/
        printf("Writing FRU size %d from %s  ...\n",sfru,binfile);
        ret = write_fru_data(g_fruid, 0, frubuf, sfru, fdebug);
        free_fru(frubuf);
        if (ret != 0) printf("write_fru error %d (0x%02x)\n",ret,ret);
	    else {  /* successful, show new data */
               ret = load_fru(sa,g_fruid,g_frutype,&pfru);
	       if (ret != 0) show_loadfru_error(sa,g_fruid,ret);
               else ret = show_fru(sa,g_fruid,g_frutype,pfru);
               free_fru(pfru);
	       pfru = NULL;
	    }
	 } 
      }
   }  /*end-else frestore */
   else if ((fwritefru != 0) && ret == 0) {
	if (fbasefru == 0) {
	   /* if not fbasefru, must reload fru because freed in get_show_fru */
	   ret = load_fru(sa,g_fruid,g_frutype,&pfru);
	   if (ret != 0) {
	      show_loadfru_error(sa,g_fruid,ret);
	      free_fru(pfru);
	      pfru = NULL;
	      goto do_exit;
	   }
	   if (fdebug) printf("Baseboard FRU buffer reloaded (%d):",sfru);
	}
	printf("\nWriting new product data (%s,%s,%s) ...\n",
		prod_ver,serial_num,asset_tag);
        ret = write_asset(asset_tag,serial_num,prod_ver,fwritefru,pfru);
        free_fru(pfru);
        if (ret != 0) printf("write_asset error %d (0x%02x)\n",ret,ret);
	else {  /* successful, show new data */
           ret = load_fru(sa,g_fruid,g_frutype,&pfru);
	   if (ret != 0) show_loadfru_error(sa,g_fruid,ret);
           else ret = show_fru(sa,g_fruid,g_frutype,pfru);
           free_fru(pfru);
	}
	pfru = NULL;
   }
   else 
	free_fru(pfru);

do_exit:
   ipmi_close_();
   // show_outcome(progname,ret);  
   return(ret);
}

/* end ifru.c */
