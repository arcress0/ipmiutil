/*
 * ifruset  (copy of ifru.c with extended FRU writing)
 *
 * This tool reads the FRU configuration, and optionally sets the asset
 * tag field in the FRU data.  See IPMI v1.5 spec section 28.
 * It can use either the Intel /dev/imb driver or VALinux /dev/ipmikcs.
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 *
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 04/01/10 Andy Cress - created from ifru.c 2.6.1
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
#include <errno.h>
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

#include "ipmicmd.h" 
#include "ipicmg.h" // #define OEM_PICMG  12634

extern int get_LastError( void );  /* ipmilan.c */
extern int get_BiosVersion(char *str);
extern int get_SystemGuid(uchar *str);
extern void fmt_time(time_t etime, char *buf, int bufsz); /*see ievents.c*/

#define FIELD_LEN   24
#define NUM_BOARD_FIELDS    6
#define NUM_PRODUCT_FIELDS  8
#define NUM_CHASSIS_FIELDS  3
#define ERR_LENMAX   -7   /*same as LAN_ERR_BADLENGTH */
#define ERR_LENMIN   -10  /*same as LAN_ERR_TOO_SHORT */
#define ERR_OTHER    -13  /*same as LAN_ERR_OTHER     */

#define STRING_DATA_TYPE_BINARY         0x00
#define STRING_DATA_TYPE_BCD_PLUS       0x01
#define STRING_DATA_TYPE_SIX_BIT_ASCII  0x02
#define STRING_DATA_TYPE_LANG_DEPENDENT 0x03

#define FRUCHUNK_SZ   16
#define FRU_END         0xC1
#define FRU_EMPTY_FIELD 0xC0
#define FRU_TYPE_MASK   0xC0
#define FRU_LEN_MASK    0x3F
#define IPROD_MANUF  0
#define IPROD_NAME   1
#define IPROD_PART   2
#define IPROD_VERS   3
#define IPROD_SERNUM 4
#define IPROD_ASSET  5
#define IPROD_FRUID  6
#define IPROD_OEM    7

#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char *progname  = "ipmiutil fruset";
#else
static char * progver   = "3.12";
static char *progname  = "ifruset";
#endif
static char fdebug = 0;
static char fpicmg = 0;
static char fonlybase = 0;
static char fonlyhsc = 0;
static char fdevsdrs = 0;
static char fshowlen = 0;
static char fgetfru  = 0;
static char fdoanyway = 0;
static char fset_mc = 0;
static char fcanonical = 0;
static char fdump = 0;
static char frestore = 0;
static int  fwritefru = 0;
static int  prod_id, vend_id = 0;
static char bdelim = ':';
static char *binfile = NULL;
static uchar bmc_sa = BMC_SA;  /*defaults to 0x20*/
static uchar guid[17] = "";
static uchar *frubuf = NULL;
static uchar *newdata = NULL;  /* new FRU data, including the product area */
			       /* usually 4*64 + 3 = 259 bytes */
static int    sfru = 0;
static int    chassis_offset = -1;
static int    chassis_len = 0;
static char   chassis_name[FIELD_LEN] = {0};
static int    product_num = NUM_PRODUCT_FIELDS;
typedef struct { 
    int offset; 
    int len;
    char tag[FIELD_LEN];
   } FIELDTYPE;
static FIELDTYPE prodarea[NUM_PRODUCT_FIELDS] = {
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0} };
static FIELDTYPE prodnew[NUM_PRODUCT_FIELDS] = {
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0},
  -1, 0, {0} };
static int    maxprod = 0;
static int ctype;
static uchar g_bus = PUBLIC_BUS;
static uchar g_sa  = 0;
static uchar g_lun = BMC_LUN;
static uchar g_addrtype = ADDR_SMI;
static uchar g_fruid = 0;  /* default to fruid 0 */
static uchar g_frutype = 0; 
/* g_frutype values (detected), see also FruTypeString
 * --TAG----   FRU   IPMB
 * Baseboard = 0x0c  0x07 
 * PowerSply = 0x0a
 * PowerCage = 0x15
 * DIMM      = 0x20
 * HotSwapCt =       0x0f
 * ME        =       0x2e
 * Component =  *     *  (all others)
 */

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
free_fru(void)
{
   if (frubuf != NULL) {
      free(frubuf);
      frubuf = NULL;
   }
   if (newdata != NULL) {
      free(newdata);
      newdata = NULL;
   }
   return;
}

int
load_fru(uchar sa, uchar frudev, uchar frutype)
{
   int ret = 0;
   uchar indata[FRUCHUNK_SZ];
   uchar resp[18];
   int sresp;
   uchar cc;
   int sz;
   char fwords;
   ushort fruoff = 0;
   int i;
   int chunk;

   memset(indata, 0, sizeof(indata));
   indata[0] = frudev;
   sresp = sizeof(resp);
   if (fdebug) printf("load_fru: sa = %02x, frudev = %02x\n",sa,frudev);
   ret = ipmi_cmd_mc(GET_FRU_INV_AREA,indata,1,resp,&sresp,&cc,fdebug);
   if (fdebug) printf("load_fru: inv ret = %d, cc = %x\n",ret,cc);
   if (ret != 0) return(ret);
   if (cc != 0) { ret = (cc & 0x00ff); return(ret); }

   sz = resp[0] + (resp[1] << 8);
   if (resp[2] & 0x01) { fwords = 1; sz = sz * 2; }
   else fwords = 0;

   frubuf = malloc(sz);
   if (frubuf == NULL) return(get_errno());
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
	   indata[3] = chunk;
	   fruoff = i;
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
   if ((frudev == 0) && (sa == bmc_sa)) { /*main system fru*/
     sresp = sizeof(resp);
     ret = ipmi_cmd_mc(GET_SYSTEM_GUID,indata,0,resp,&sresp,&cc,fdebug);
     if ((ret != 0)  && !is_remote()) {  /* get UUID from SMBIOS */
        cc = 0; sresp = 16;
        ret = get_SystemGuid(resp);
     }
     if (fdebug) printf("system_guid: ret = %d, cc = %x\n",ret,cc);
     if (ret == 0 && cc == 0) {
	if (fdebug) {
	   printf("system guid (%d): ",sresp);
	   for (i=0; i<16; i++) printf("%02x ",resp[i]);
	   printf("\n");
	}
	memcpy(&guid,&resp,16);
   	guid[16] = 0;
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
  unsigned char *d = &target[0];
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
  if (type == STRING_DATA_TYPE_BCD_PLUS) { /*type 1 BCD plus*/
     for (k=0; k<len; k++)
         target[k] = bcd_plus[(s[k] & 0x0f)];
     target[k] = '\0';
  } else if (type == STRING_DATA_TYPE_SIX_BIT_ASCII) { /*type 2 6-bit ASCII*/
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
       target[len] = 0x0;
     } else {
       printf("Language 0x%x dependent decode not supported\n",language_code);
     }
  } else if (type == STRING_DATA_TYPE_BINARY) { /* 00: binary/unspecified */
     strncpy(target, buf2str(s, slen), len);
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

#define NSPDMFG 7
static struct { 
   uchar id;  char *str;
} spd_mfg[NSPDMFG] = {  /* see JEDEC JEP106 doc */
{ 0x2c, "Micron" },
{ 0x15, "Philips Semi" },
{ 0x1c, "Mitsubishi" },
{ 0xce, "Samsung" },
{ 0xc1, "Infineon" },
{ 0x98, "Kingston" },
{ 0x89, "Intel" }
};

int ValidTL(uchar typelen)
{
   if (vend_id != VENDOR_INTEL) return 1;
   if ((typelen & 0xC0) == 0xC0) return 1;
   else return 0;
}

char * FruTypeString(uchar frutype)
{
   char *pstr;
   switch (frutype) { 
            case 0x07:  pstr = "Baseboard"; break;  /*IPMB*/
            case 0x0c:  pstr = "Baseboard"; break;  /*FRU*/
            case 0x0a:  pstr = "PowerSply"; break;  /*FRU*/
            case 0x15:  pstr = "PowerCage"; break;  /*FRU*/
            case 0x20:  pstr = "DIMM     "; break;  /*FRU*/
            case 0x0f:  pstr = "HotSwapCt"; break;  /*IPMB*/
            case 0x2e:  pstr = "ME       "; break;  /*IPMB*/
            default:    pstr = "Component"; break; 
	}
   return(pstr);
}

static void 
show_spd(uchar *spd, int lenspd, uchar frudev, uchar frutype)
{
   int sz;
   char *pstr;
   uchar mrev;
   int i;
   char devstr[20];

   sz = spd[0];  /* sz should == lenspd */
   if (fcanonical) devstr[0] = 0;  /*default is empty string*/
   else sprintf(devstr,"[%s,%02x] ",FruTypeString(frutype),frudev);
   printf("%sMemory SPD Size     %c %d\n",
		devstr,bdelim,lenspd);
   if (spd[2] == 0x07) pstr = "DDR";
   else /* usu 0x04 */ pstr = "SDRAM";
   printf("%sMemory Type         %c %s\n",
		devstr,bdelim,pstr);
   printf("%sModule Density      %c %d MB per bank\n",
		devstr,bdelim, (spd[31] * 4));
   printf("%sModule Banks        %c %d banks\n", 
		devstr,bdelim,spd[5]);
   printf("%sModule Rows, Cols   %c %d rows, %d cols\n",
		devstr,bdelim, spd[3], spd[4]);
   if (spd[11] == 0x00) pstr = "Non-parity";
   else /* usu 0x02 */  pstr = "ECC";
   printf("%sDIMM Config Type    %c %s\n",devstr,bdelim,pstr);
   for (i = 0; i < NSPDMFG; i++) 
	if (spd_mfg[i].id == spd[64]) break;
   if (i == NSPDMFG) pstr = "";  /* not found, use null string */
   else pstr = spd_mfg[i].str;
   printf("%sManufacturer ID     %c %s (0x%02x)\n",
		devstr,bdelim, pstr, spd[64]);
   mrev = spd[91];  /* save this byte for later */
   spd[91] = 0;     /*stringify part number */
   printf("%sManufacturer Part#  %c %s\n",
		devstr,bdelim,&spd[73]);
   printf("%sManufacturer Rev    %c %02x %02x\n",
		devstr,bdelim,mrev,spd[92]);
   printf("%sManufacturer Date   %c year=%02d week=%02d\n",
		devstr,bdelim,spd[93],spd[94]);
   printf("%sAssembly Serial Num %c %02x%02x%02x%02x\n",
		devstr,bdelim,spd[95], spd[96], spd[97], spd[98]);
   spd[91] = mrev;  /* restore byte, so ok to repeat later */
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
   int vend, i;
   char mystr[256];
   char *s1, *s2;
   int v1, v2, v3, v4, v5, v6, v7, v8;
   uchar b1, b2;

   if (fdebug) dumpbuf(pdata,dlen);  /*multi-record area dump*/
   sprintf(mystr,"%sMulti[%d] ",tag,midx);
   i = strlen(mystr);
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
		    if (b1 & 0x10 != 0) s1 = "DeassertFail ";
		    else s1 = "AssertFail ";
		} else {
		    if (b1 & 0x10 != 0) s1 = "2pulses/rot ";
		    else s1 = "1pulse/rot ";
		}
		printf("\t Flags   \t%c %s%s%s%s%s\n",bdelim,
		  (b2 ? "PredictFail " : ""),
		  ((b1 & 0x02 == 0) ? "" : "PowerFactorCorrect "),
		  ((b1 & 0x04 == 0) ? "" : "AutoswitchVolt "),
		  ((b1 & 0x08 == 0) ? "" : "Hotswap "), s1);
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
		b2 = (pdata[0] & 0x80 != 0);
		printf("%sDC Output       %c number %d\n",mystr,bdelim,b1);
		printf("\t Standby power \t%c %s\n", bdelim,
			(b2 ? "Yes" : "No"));
		v1 = pdata[1] + (pdata[2] << 8);
		printf("\t Nominal voltage \t%c %.2f V\n", bdelim, v1 / 100);
		v2 = pdata[3] + (pdata[4] << 8);
		v3 = pdata[5] + (pdata[6] << 8);
		printf("\t Voltage deviation \t%c + %.2f V / - %.2f V\n", 
			bdelim, v3/100, v2/100);
		v4 = pdata[7] + (pdata[8] << 8);
		printf("\t Ripple and noise pk-pk \t%c %d mV\n", bdelim, v4);
		v5 = pdata[9] + (pdata[10] << 8);
		printf("\t Min current draw \t%c %.3f A\n", bdelim, v5/1000);
		v6 = pdata[11] + (pdata[12] << 8);
		printf("\t Max current draw \t%c %.3f A\n", bdelim, v6/1000);
		break;
	case 0x02:  /*DC Load*/
		b1 = pdata[0] & 0x0f;
		printf("%sDC Load         %c number %d\n",mystr,bdelim,b1);
		v1 = pdata[1] + (pdata[2] << 8);
		printf("\t Nominal voltage \t%c %.2f V\n", bdelim, v1 / 100);
		v2 = pdata[3] + (pdata[4] << 8);
		v3 = pdata[5] + (pdata[6] << 8);
		printf("\t Min voltage allowed \t%c %.2f A\n", bdelim, v2);
		printf("\t Max voltage allowed \t%c %.2f A\n", bdelim, v3);
		v4 = pdata[7] + (pdata[8] << 8);
		printf("\t Ripple and noise pk-pk \t%c %d mV\n", bdelim, v4);
		v5 = pdata[9] + (pdata[10] << 8);
		printf("\t Min current load \t%c %.3f A\n", bdelim, v5/1000);
		v6 = pdata[11] + (pdata[12] << 8);
		printf("\t Max current load \t%c %.3f A\n", bdelim, v6/1000);
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
show_fru(uchar sa, uchar frudev, uchar frutype)
{
   int ret = 0;
   int i, j, n, sz;
   uchar *pfru;
   uchar lang;
   TYPE_LEN tl;
   char newstr[64];
   int iaoff, ialen, bdoff, bdlen; 
   int proff, prlen, choff, chlen;
   int moff, mlen;
   char devstr[24];
   char *pstr;
   int extra = 0;

   if (frubuf[0] == 0x80) {    /* 0x80 = type for DIMMs (SPD) */
      /* FRU Header: 80 08 07 0c 0a 01 48 00 (DIMM) */
      sz = frubuf[0];
      if (fdebug) {
	printf("DIMM SPD Body (size=%d/%d): ",sz,sfru);
	dumpbuf(frubuf,sfru);
      }
      show_spd(frubuf,sfru, frudev,frutype);
      return(ret);
   }

   pstr = FruTypeString(frutype);
   if (fcanonical) devstr[0] = 0;  /*default is empty string*/
   else sprintf(devstr,"[%s,%02x] ",pstr,frudev);
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
   pfru = &frubuf[0];
   sz = 8;  /*minimum for common header*/
   for (i = 1; i < 6; i++) {	/* walk thru offsets */
 	 if (frubuf[i] != 0) sz = frubuf[i] * 8;
   }
   if (sz > 8) {   		/* if have at least one section */
	 if (frubuf[5] != 0) j = 5 + frubuf[sz+2]; /*if multi-record area*/
	 else j = frubuf[sz+1] * 8;   /* else add length of last section */
	 sz += j;
   }
   /* Now, sz = size used, sfru = total available size */
   if (sz > sfru) {
      if (fdebug) {
        printf("FRU Header: ");
        for (i = 0; i < 8; i++) printf("%02x ",frubuf[i]);
        printf("\n");
      }
      printf("FRU size out of bounds: available=%d used=%d\n",sfru,sz);
      printf("Please apply the correct FRU/SDR diskette\n");
      if (fdoanyway) {
          extra = sz - sfru;
	  sz = sfru;
      } else return(ERR_OTHER);
   }
   /* internal area offset, length */
   iaoff = frubuf[1] * 8;  
   ialen = frubuf[iaoff + 1] * 8;
   /* chassis area offset, length */
   choff = frubuf[2] * 8;  
   chlen = frubuf[choff + 1] * 8;
   /* board area offset, length */
   bdoff = frubuf[3] * 8;  
   bdlen = frubuf[bdoff + 1] * 8;
   /* product area offset, length */
   proff = frubuf[4] * 8;  
   prlen = frubuf[proff + 1] * 8;
   if (extra > 0) {   /*do fixup of extra in product area*/
	prlen -= extra;
	j = prlen / 8;
	prlen = j * 8; /*resolve to 8-byte bound*/
        frubuf[proff + 1] = j;
   }
   /* multi-record area offset, length */
   moff = frubuf[5] * 8;  
   mlen = 0;
   if (moff > 0) {
      for (i = moff; i < sfru; ) {
         if (frubuf[i] == 0 && frubuf[i+2] == 0) break; /*type/len invalid*/
         j = 5 + frubuf[i+2];
         mlen += j; 
         if (frubuf[i+1] & 0x80) break;
         i += j;
      }
   }

   if (fdebug) {
      printf("FRU Header: ");
      for (i = 0; i < 8; i++) printf("%02x ",frubuf[i]);
      printf("\n");
      printf("FRU Body (size=%d/%d): ",sz,sfru);
      dumpbuf(frubuf,sfru);
      printf("header, len=%d, cksum0 = %02x, cksum1 = %02x\n",
		8,frubuf[7],calc_cksum(&frubuf[0],7));
      printf("internal off=%d, len=%d, cksum = %02x\n",
		iaoff,ialen,calc_cksum(&frubuf[iaoff],ialen-1));
      printf("chassis off=%d, len=%d, cksum = %02x\n",
		choff,chlen,calc_cksum(&frubuf[choff],chlen-1));
      printf("board off=%d, len=%d, cksum = %02x\n",
		bdoff,bdlen,calc_cksum(&frubuf[bdoff],bdlen-1));
      printf("prod  off=%d, len=%d, cksum = %02x\n",
		proff,prlen,calc_cksum(&frubuf[proff],prlen-1));
      /* Multi-record area */
      printf("multi off=%d, len=%d, fru sz=%d\n", moff,mlen,sz);  
   }  /*endif fdebug, show header*/

   if (choff != 0) {
      /* show Chassis area fields */
      pfru = &frubuf[choff];
      lang = 25;  /* English */
      ctype = pfru[2];  /*chassis type*/
      if (fdebug) printf("ctype=%x\n",ctype);
      if (ctype >= MAX_CTYPE) ctype = MAX_CTYPE - 1;
      printf("%s%s%c %s\n",devstr, ctype_hdr,bdelim,ctypes[ctype]);
      pfru += 3;  /* skip chassis header */
      for (i = 0; i < NUM_CHASSIS_FIELDS; i++)
      {
         if (pfru[0] == FRU_END) break;  /*0xC1 = end of FRU area*/
         if (!ValidTL(pfru[0]))
	    printf("  ERROR - Invalid Type/Length %02x for %s\n",
			pfru[0],chassis[i]);
         tl.type = (pfru[0] & FRU_TYPE_MASK) >> 6;
         tl.len  = pfru[0] & FRU_LEN_MASK;
         if (i == 2) {  /* OEM field for chassis_name  */
	   chassis_offset = (int)(pfru - frubuf);
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
      pfru = &frubuf[bdoff];
      lang = pfru[2];
      /* Decode board mfg date-time (num minutes since 1/1/96) */
      nMin = pfru[3] + (pfru[4] << 8) + (pfru[5] << 16);
		  /* 13674540 min from 1/1/70 to 1/1/96 */
      nSec = (nMin + 13674540) * 60;
      tsec = (time_t)(nSec & 0x0ffffffff);
      // fmt_time(tsec,newstr,sizeof(newstr)); 
      printf("%sBoard Mfg DateTime  %c %s",devstr,bdelim,ctime(&tsec));
      pfru += 6;  /* skip board header */
      for (i = 0; i < NUM_BOARD_FIELDS; i++)
      {
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

   if (proff != 0) {
      /* show Product area fields */
      pfru = &frubuf[proff];
      maxprod = pfru[1] * 8;
      lang = pfru[2];
      pfru += 3;  /* skip product header */
      for (i = 0; i < NUM_PRODUCT_FIELDS; i++)
      {
         if (*pfru == FRU_END) {  /*0xC1 = end*/
            /* Wart for known Kontron 1-byte Product Version anomaly. */
            if ((vend_id == VENDOR_KONTRON) && (i == 3)) ;
            else break;
         }
         if (*pfru == 0) *pfru = FRU_EMPTY_FIELD; /* fix a broken table */
         if (!ValidTL(pfru[0]))
	    printf("  ERROR - Invalid Type/Length %02x for %s\n",
			pfru[0],product[i]);
         tl.type = (pfru[0] & FRU_TYPE_MASK) >> 6;
         tl.len  = pfru[0] & FRU_LEN_MASK;
         n = (int)(pfru - frubuf);
         prodarea[i].offset = n;
         prodarea[i].len    = tl.len;
         memcpy(prodarea[i].tag, &frubuf[n+1] ,tl.len);
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
      product_num = i;  /*save number of existing product fields*/
      if (*pfru == 0x00) *pfru = FRU_END;  /* insert end char if broken */
   }

   if (moff != 0) 
   {
      /* multi-record area may contain several record headers 
       * 0 = record type id
       * 1 = 0x02 or 0x80 if End-of-List
       * 2 = record len
       * 3 = record chksum
       * 4 = header chksum
       */
      pfru = &frubuf[moff];
      j = moff;
      for (i = 0; j < sz; i++)
      {
         if (pfru[0] == 0 && pfru[2] == 0) break; /*type/len invalid*/ 
         n = pfru[2];  /* len of this record */
         show_fru_multi(devstr,i,pfru[0],&pfru[5],n);
         j += (5 + n);
         if (pfru[1] & 0x80) j = sz;  /*0x80 = end of list*/
         pfru += (5 + n);
      }
   }

   if ((frudev == 0) && (sa == bmc_sa)) {
	char *s;
	printf("%sSystem GUID         %c ",devstr,bdelim);
	for (i=0; i<16; i++) {
	   if ((i == 4) || (i == 6) || (i == 8) || (i == 10)) s = "-";
	   else s = "";
	   printf("%s%02x",s,guid[i]);
	}
	printf("\n");
   }
   return(ret);
}

static int 
write_fru_data(uchar id, ushort offset, uchar *data, int dlen, char fdebug)
{
   int ret = -1;
   int chunk;
   ushort fruoff;
   uchar req[FRUCHUNK_SZ+9];
   uchar resp[16];
   int sresp;
   uchar cc;
   int i, j;

   /* Write the buffer in small 16-byte (FRUCHUNK_SZ) chunks */
   req[0] = id;  /* FRU Device ID (fruid) */
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
	fruoff += chunk;
   }
   return(ret);
}

/* 
 * write_product 
 *    Updates the FRU Product area only.
 * Note that this function will always provide >=8 product fields, 
 * even if the original had less than 8.
 * inputs: prodnew = array of new strings to write
 *         frubuf  = contains existing FRU data
 *         newdata = new product area buffer, malloc'd
 * outputs: returns 0 if successful
 */
int
write_product(void) 
{
   int ret = -1;
   uchar req[25];
   uchar resp[16];
   int sresp;
   uchar cc;
   ushort fruoff;
   int alen, clen;
   int snlen, verlen;
   uchar *pfru0;
   uchar *pfru;
   uchar *pnew;
   int i, j, k, n, plen, newlen, max, plimit; 
   int chas_offset; 
   int prod_offset; 
   int mult_offset, mult_len; 
   uchar chksum;
   int chunk;

   chas_offset = frubuf[2] * 8;           // offset of chassis data
   prod_offset = frubuf[4] * 8;           // offset of product data
   plen = frubuf[prod_offset+1] * 8;      // length of product data 
   mult_offset = frubuf[5] * 8;  
   mult_len = 0; 
   if (mult_offset > 0) {
      for (i = mult_offset; i < sfru; ) {
         mult_len += (5 + frubuf[i+2]);
         if (frubuf[i+1] & 0x80) break;
	 i = mult_len;
      }
   }
   /* Check if asset tag will fit in product data area of FRU. */
   if (fdebug) 
	printf("write_product: fru[4,p]=[%02x,%02x] prod_off=%d, plen=%d\n",
		frubuf[4],frubuf[prod_offset+1],prod_offset,plen);
   if (plen > sfru) return ERR_LENMAX; // product bigger than buffer
   if (prodnew[IPROD_ASSET].len > plen) return ERR_LENMAX; 
   /* if asset bigger than product data, error.  */
   /* asset comes after sernum, so this check works for both */

   newdata = malloc(sfru); /* but should not need more than plen bytes */
   if (newdata == NULL) return(get_errno());
   memset(newdata,0,sfru);
   pnew = &newdata[0];
   /* Set pointer to start of chassis area */
   pfru = &frubuf[chas_offset]; 

   /* Product Area Header (3 bytes):
      [0] = 0x01;     * format ver 1 *
      [1] = 0x0a;     *product area size (in 8-byte mult)*
      [2] = 0x00;     *lang=english *
      Usually max product area is 3 + 8*32 = 259 mod 8 = 264.  
   */
   pfru0 = &frubuf[prod_offset]; 
   pfru = &frubuf[prod_offset]; 
   j = 3;
   memcpy(pnew,pfru,j);
   pfru += j; 
   pnew += j;
   n = j;
   if (mult_offset > 0)  plimit = plen; 
   else plimit = sfru - prod_offset;   /*plen can expand*/

   for (i = 0; i < NUM_PRODUCT_FIELDS; i++) 
   {
      j = prodarea[i].len;  	   /*new len*/
      k = pfru[0] & FRU_LEN_MASK;  /*old len*/
      if (k == 1) {  /*was 0xC1 FRU_END*/
         if ((vend_id == VENDOR_KONTRON) && (i == 3) && (j == 1)) { 
            /* fix Kontron 1-byte Version */
            prodarea[i].tag[1] = ' ';
            j++;
         } else
	    k = 0;
      }
      /* check for product area overflow */
      if (n + 2 >= plimit) {
	if (fdebug) printf("Field %d is at %d, beyond product area size %d\n",
				i+1,n+2,plimit);
	break;
      } else if ((n + 1 + j) >= plimit) {
	if (fdebug) printf("Field %d at %d + %d, truncated, product size %d\n",
				i+1,n+1,j,plimit);
	j = 0;
      }
      pnew[0] = (j | FRU_TYPE_MASK);  /*add type=3 to len*/
      memcpy(&pnew[1],prodarea[i].tag,j);
      if (fdebug) {
	 printf("i=%d frubuf[%d]: %02x %02x %02x, j=%d k=%d n=%d\n",
			i,(pfru-frubuf),pfru[0],pfru[1],pfru[2],j,k,n);
	 if (i >= product_num) 
	    printf("Field %d is beyond existing %d fields\n",i+1,product_num);
      }
      pnew += j + 1;
      pfru += k + 1;
      n += j + 1;
   }

   // n = (int)(pnew - newdata);  /*new product area index*/
   k = (int)(pfru - pfru0);       /*old product area index*/
   if (mult_offset > 0)   /* do not expand if multi-record area there */
       max = plen - k; 
   else /* nothing else, can expand product area, up to sfru */
       max = sfru - (k + prod_offset); 
   if (fdebug) 
	printf("frubuf[%d]: %02x %02x %02x, j=%d k=%d n=%d remainder=%d\n",
		   (pfru-frubuf),pfru[0],pfru[1],pfru[2],j,k,n,max);
   if (max < 0) max = 0;
   /* copy trailing fru data from saved copy */
   for (i = 0; i < max; i++) {
     pnew[i] = pfru[i];
     if (pfru[i] == FRU_END) { i++; break; } /*0xC1*/
   }
   if (i == max) {  /*never found 0xC1 FRU_END*/
     pnew[0] = FRU_END; /*mark this trailing field as empty*/
     i = 1;
   } 
   newlen = n + i;
   if (fdebug) printf("newbuf[%d]: %02x %02x %02x, j=%d newlen=%d plen=%d\n",
		   n,pnew[0],pnew[1],pnew[2],j,newlen,plen);

   /* round up to next 8-byte boundary */
   /* need one more byte for checksum, so if j=0, add 8 anyway */
   j = 8 - (newlen % 8);  
   for (i = 0; i < j; i++) newdata[newlen++] = 0;
   if (newlen < plen) newlen = plen;  /* don't shrink the product area */
   newdata[1] = newlen / 8;     // set length of product data 

   /* include new checksum (calc over Product area) */
   chksum = calc_cksum(&newdata[0],newlen-1);
   newdata[newlen-1] = chksum;

   if (fdebug) {
	printf("old prod_area buffer (%d):",plen);
	dumpbuf(&frubuf[prod_offset],plen);
	printf("new prod_area buffer (%d):",newlen);
	dumpbuf(newdata,newlen);
   }
   if (prod_offset + newlen >= sfru) return ERR_LENMAX; 
   if ((mult_offset > 0) && (newlen > plen)) return ERR_LENMAX;
#ifdef TEST
	newlen = 0;  /*don't actually write the new data, if testing*/
#endif

   ret = write_fru_data(g_fruid, prod_offset, newdata, newlen, fdebug);
   return(ret);
}

#define CHUNKSZ   16
#define LAST_REC  0xffff
#define STR_OFF   16

int get_sdr(ushort recid, ushort resid, ushort *recnext, 
		uchar *sdr, int *slen, uchar *pcc)
{
	uchar idata[6];
	uchar rdata[64];
	int sresp;
	ushort cmd;
	uchar cc = 0;
        int len = 0;
	int rc;
	
	idata[0] = resid & 0x00ff;
	idata[1] = (resid & 0xff00) >> 8;
	idata[2] = recid & 0x00ff;
	idata[3] = (recid & 0xff00) >> 8;
	idata[4] = 0;  /*offset*/
	idata[5] = CHUNKSZ;  /*bytes to read*/
        if (fdevsdrs) cmd = GET_DEVICE_SDR;
        else cmd = GET_SDR;
	sresp = sizeof(rdata);
	rc = ipmi_cmd_mc(cmd, idata, 6, rdata, &sresp,&cc, fdebug);
	if (fdebug) printf("get_sdr[%x] ret = %d cc = %x sresp = %d\n",
				recid,rc,cc,sresp);
	if (rc == 0 && cc == 0xCA) {
	   idata[5] = 8;  /*bytes to read*/
	   sresp = sizeof(rdata);
	   rc = ipmi_cmd_mc(cmd, idata, 6, rdata, &sresp,&cc, fdebug);
	   if (fdebug) printf("get_sdr[%x] ret = %d cc = %x sresp = %d\n",
				recid,rc,cc,sresp);
	}
	*pcc = cc;
	if (rc == 0 && cc == 0) {
	   *recnext = rdata[0] + (rdata[1] << 8);
	   memcpy(sdr,&rdata[2],sresp-2);
           *slen = sdr[6] + 5;  /*get actual SDR size*/
	   len = sresp-2;
           /* if an SDR locator record, get the rest of it. */
	   if (sdr [3] == 0x11 || sdr[3] == 0x12)
	     if (*slen > CHUNKSZ) {
		idata[0] = resid & 0x00ff;
		idata[1] = (resid & 0xff00) >> 8;
		idata[2] = recid & 0x00ff;
		idata[3] = (recid & 0xff00) >> 8;
		idata[4] = CHUNKSZ;  /*offset*/
		idata[5] = *slen - CHUNKSZ;  /*bytes to read*/
		sresp = sizeof(rdata);
		rc = ipmi_cmd_mc(cmd, idata, 6, rdata, &sresp,&cc, fdebug);
		if (fdebug) printf("get_sdr[%x] 2nd ret=%d cc=%x sresp=%d\n",
				recid,rc,cc,sresp);
		if (rc == 0 && cc == 0) {
		   sresp -= 2;
	   	   memcpy(&sdr[len],&rdata[2],sresp);
		   len += sresp;
		}
	     }
	   *slen = len;
	}
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

#ifdef METACOMMAND
int i_ifruset(int argc, char **argv)
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
   char DevRecord[16];
   ushort recid;
   ushort nextid;
   ushort rsvid;
   uchar sdr[40];
   char biosver[80];
   uchar cc;
   uchar sa;
   uchar fruid = 0;
   uchar frutype = 0;
   int len, i;
   char *s1;
   FILE *fp;

   printf("%s version %s\n",progname,progver);
   parse_lan_options('V',"4",0);  /*request admin priv by default*/
   while ( (c = getopt( argc, argv,"a:bcd:h:i:f:m:n:o:p:r:s:u:v:xyz:T:V:J:EYF:P:N:R:U:Z:?")) != EOF )
      switch(c) {
          case 'x': fdebug = 1;      break;
          case 'y': fdoanyway = 1;   break;
          case 'b': fonlybase = 1; 
		    g_frutype = 0x07;  break;
          case 'c': fcanonical = 1; bdelim = BDELIM; break;
#ifdef TEST
          case 'h': 
		if (optarg) {
                   len = strlen(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
                   strncpy(chassis_name,optarg,len);
                   if (len == 1) {  /* add a space */
                       chassis_name[1] = ' ';
                       chassis_name[2] = 0;
                   }
                }
		break;
#else
          case 'h': fonlyhsc = 1;   /* can use -m00c000s instead */
		    g_frutype = 0x0f;   break;
#endif
          case 'a':
		fwritefru |= 0x01;
		if (optarg) {
                   len = strlen(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
		   /* if (len == 1), handle it in write_product() */
		   strncpy(prodnew[IPROD_ASSET].tag,optarg,len);
		   prodnew[IPROD_ASSET].len = len;
                }
		break;
          case 'f':
		fwritefru |= 0x04;
		if (optarg) {
                   len = strlen(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
		   /* if (len == 1), handle it in write_product() */
		   strncpy(prodnew[IPROD_FRUID].tag,optarg,len);
		   prodnew[IPROD_FRUID].len = len;
                }
		break;
          case 'n':
		fwritefru |= 0x20;
		if (optarg) {
                   len = strlen(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
		   /* if (len == 1), handle it in write_product() */
		   strncpy(prodnew[IPROD_NAME].tag,optarg,len);
		   prodnew[IPROD_NAME].len = len;
                }
		break;
          case 'o':
		fwritefru |= 0x10;
		if (optarg) {
                   len = strlen(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
		   /* if (len == 1), handle it in write_product() */
		   strncpy(prodnew[IPROD_OEM].tag,optarg,len);
		   prodnew[IPROD_OEM].len = len;
                }
		break;
          case 'p':
		fwritefru |= 0x40;
		if (optarg) {
                   len = strlen(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
		   /* if (len == 1), handle it in write_product() */
		   strncpy(prodnew[IPROD_PART].tag,optarg,len);
		   prodnew[IPROD_PART].len = len;
                }
		break;
          case 'u':
		fwritefru |= 0x80;
		if (optarg) {
                   len = strlen(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
		   /* if (len == 1), handle it in write_product() */
		   strncpy(prodnew[IPROD_MANUF].tag,optarg,len);
		   prodnew[IPROD_MANUF].len = len;
                }
		break;
          case 's':
		fwritefru |= 0x02; 
		if (optarg) { 
                   len = strlen(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
		   /* if (len == 1), handle it in write_product() */
		   strncpy(prodnew[IPROD_SERNUM].tag,optarg,len);
		   prodnew[IPROD_SERNUM].len = len;
                }
		break;
          case 'v':
		fwritefru |= 0x08; 
		if (optarg) { 
                   len = strlen(optarg);
                   if (len >= FIELD_LEN) len = FIELD_LEN - 1;
		   /* if (len == 1), handle it in write_product() */
		   strncpy(prodnew[IPROD_VERS].tag,optarg,len);
		   prodnew[IPROD_VERS].len = len;
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
          case 'z': /* set local IPMB MC sa */
                    sa  = htoi(&optarg[0]);  /*device slave address*/
		    ipmi_set_mymc(g_bus, sa, g_lun,ADDR_IPMB);
		    bmc_sa = sa;
          case 'i': fonlybase = 1;   /*specify a fru id*/
		    if (strncmp(optarg,"0x",2) == 0) g_fruid = htoi(&optarg[2]);
		    else g_fruid = htoi(optarg); 
		    printf("Using FRU ID 0x%02x\n",g_fruid);
                    break;
          case 'd': fdump = 1;      /*dump fru to a file*/
		binfile = optarg;
		break;
          case 'r': frestore = 1;   /*restore fru from a file*/
		fwritefru = 0x100; 
		binfile = optarg;
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
                parse_lan_options(c,optarg,fdebug);
                break;
          default:
                printf("Usage: %s [-bcimxy -unpvsafo -NUPREFTVY]\n",progname);
		printf("   -u manu  Sets Product Manufacturer (0)\n");
		printf("   -n name  Sets Product Name         (1)\n");
		printf("   -p part  Sets Product Part Number  (2)\n");
		printf("   -v vers  Sets Product Version      (3)\n");
		printf("   -s snum  Sets Product Serial Num   (4)\n");
		printf("   -a tag   Sets Product Asset Tag    (5)\n");
		printf("   -f fru   Sets Product FRU File ID  (6)\n");
		printf("   -o oem   Sets Product OEM Field    (7)\n");
		// printf("   -h chname   Sets the Chassis Name \n");
		printf("   -b       Only show Baseboard FRU data\n");
		printf("   -c       show Canonical, delimited output\n");
		printf("   -d       Dump FRU to a file\n");
		printf("   -r       Restore FRU from a file\n");
		printf("   -i 00    Get/Set a specific FRU ID\n");
        printf("   -m002000 specific MC (bus 00,sa 20,lun 00)\n");
		printf("   -x       Display extra debug messages\n");
		printf("   -y       Ignore the check for FRU size overflow\n");
		print_lan_opt_usage(0);
                ret = ERR_USAGE;
		goto do_exit;
      }

   // if (is_remote() && fwritefru) parse_lan_options('V',"4",0);

   ret = ipmi_getdeviceid( DevRecord, sizeof(DevRecord),fdebug);
   if (ret == 0) {
      uchar ipmi_maj, ipmi_min;
      ipmi_maj = DevRecord[4] & 0x0f;
      ipmi_min = DevRecord[4] >> 4;
      vend_id = DevRecord[6] + (DevRecord[7] << 8) + (DevRecord[8] << 16);
      prod_id = DevRecord[9] + (DevRecord[10] << 8);
      show_devid( DevRecord[2],  DevRecord[3], ipmi_maj, ipmi_min);
      if ((DevRecord[1] & 0x80) == 0x80) fdevsdrs = 1;
      if (vend_id == VENDOR_NEC) fdevsdrs = 0;
      else if (vend_id == VENDOR_INTEL) {
	 if (prod_id == 0x003E) {
		set_max_kcs_loops(URNLOOPS); /*longer for cmds (default 300)*/
	 }
      }
   } else { 
	goto do_exit;
   }
 
   ret = ipmi_getpicmg( DevRecord, sizeof(DevRecord),fdebug);
   if (ret == 0)  fpicmg = 1;
   if ((vend_id == VENDOR_INTEL) && (!fpicmg))
       fdevsdrs = 0;   /* override, use SDR repository*/
   if (fdebug) printf("bmc_sa = %02x  fdevsdrs = %d\n",bmc_sa,fdevsdrs);

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

   if (!fonlybase && !fonlyhsc) {
     /* loop thru SDRs to find FRU devices */
#ifdef NOT_YET
     {  /* get SDR Repository Info (needs to be copied here)*/
       ret = GetSDRRepositoryInfo(&j,&fdevsdrs);
       if (fdebug) printf("GetSDRRepositoryInfo: ret=%x nSDRs=%d\n",ret,j);
     }
#endif
     {  /* reserve the SDR repository */
	uchar resp[16];
	int sresp;
	uchar cc;
	ushort cmd;
        sresp = sizeof(resp);
        if (fdevsdrs) cmd = RESERVE_DEVSDR_REP;
        else cmd = RESERVE_SDR_REP;
        ret = ipmi_cmd_mc(cmd, NULL, 0, resp, &sresp, &cc, 0);
        if (fdebug) printf("ipmi_cmd RESERVE status = %d, cc = %x\n",ret,cc);
        rsvid = resp[0] + (resp[1] << 8);
     }
     recid = 0;
     while (recid != LAST_REC)
     {
	char idstr[32];
	int ilen;

	len = sizeof(sdr); /*sizeof(sdr); get 32 sdr bytes*/
	ret = get_sdr(recid,rsvid,&nextid,sdr,&len,&cc);
	if ((ret != 0) || (cc != 0)) {
	    printf("SDR[%04x] error %d ccode = %x\n",recid,ret,cc);
	    break;
	}
        fgetfru = 0;
	if ((sdr[3] == 0x11) || (sdr[3] == 0x12)) /* SDR FRU or IPMB type */
	{
	    if (len > STR_OFF) {
		ilen = len - STR_OFF;
		if (ilen >= sizeof(idstr)) ilen = sizeof(idstr) - 1;
		memcpy(idstr,&sdr[STR_OFF],ilen);
		idstr[ilen] = 0;
	    } else idstr[0] = 0;
	    sa = sdr[5];  /* usu 0x20 for bmc_sa */
	    /* Get its FRU data */
	    if ((sdr[3] == 0x11) && (sdr[7] & 0x80)) {  /* FRU SDRs */
	      /* It is a logical FRU device */
	      if (fcanonical)
	         printf("SDR[%04x] FRU  %c %s\n", recid, bdelim, idstr);
	      else
	         printf("SDR[%04x] FRU  %02x %02x %02x %02x %s\n", recid, 
			sdr[5],sdr[6],sdr[12],sdr[13],idstr);
	      fruid = sdr[6];
	      frutype = sdr[12];
              if (sa == bmc_sa && fruid == 0) /*do this below*/;
              else
                switch(sdr[12])   /*FRU entity id*/
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
		frutype = sdr[12];
                if (sa == bmc_sa && fruid == 0) {  /*do bmc_sa,0 below*/
                   if (fdebug) printf("do bmc_sa %02x below\n",sa);
		   g_frutype = frutype;
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
            if (fgetfru) {
                uchar adrtype;
                adrtype = g_addrtype;
                if (fdebug) printf("set_mc %02x:%02x:%02x type=%d fruid=%02x\n",
				g_bus,sa,g_lun,adrtype,fruid);
                ipmi_set_mc(g_bus, sa, g_lun,adrtype);
                ret = load_fru(sa,fruid,frutype);
                if (ret != 0) {
		   show_loadfru_error(sa,fruid,ret);
                   free_fru();
                } else {
                   ret = show_fru(sa,fruid,frutype);
                   if (ret != 0) printf("show_fru error = %d\n",ret);
                   free_fru();
                }
                ipmi_restore_mc();
            }
        }  /*endif FRU/IPMB SDR */
        recid = nextid;
     } /*end while sdrs*/
   } /*endif not fonlybase*/
 
   /* load the FRU data for Baseboard (address 0x20) */
   printf("\n");
   sa = g_sa;  /* bmc_sa = BMC_SA = 0x20 */
   if (fonlyhsc) { sa = 0xC0;  g_addrtype = ADDR_SMI; 
       ipmi_set_mc(g_bus,sa,g_lun,g_addrtype); 
   }
   if (g_addrtype == ADDR_IPMB) 
       ipmi_set_mc(g_bus,sa,g_lun,g_addrtype);
   ret = load_fru(sa,g_fruid,g_frutype);
   if (ret != 0) {
	show_loadfru_error(sa,g_fruid,ret);
	free_fru();
	goto do_exit;
   }

   /* display the Baseboard FRU data */
   ret = show_fru(sa,g_fruid,g_frutype);
   if (ret != 0) printf("show_fru error = %d\n",ret);

   if (!is_remote()) {
	char devstr[24];
        if (fcanonical) devstr[0] = 0;  /*default is empty string*/
        else sprintf(devstr,"[%s,%02x] ",FruTypeString(g_frutype),g_fruid);
	i = get_BiosVersion(biosver);
	if (i == 0) 
		printf("%sBIOS Version        %c %s\n",devstr,bdelim,biosver);
   }

   if (fdump && ret == 0) {
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
         len = fwrite(frubuf, 1, sfru, fp); 
         fclose(fp);
	 if (len <= 0) {
            ret = get_LastError();
	    printf("Error %d writing file %s\n",ret,binfile);
	 } else ret = 0;
      }
   } else if (frestore) {
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
	 len = fread(frubuf, 1, sfru, fp);
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
            free_fru();
	 }
	 if (ret == 0) {  /*successfully read data*/
            printf("Writing FRU size %d from %s  ...\n",sfru,binfile);
	    ret = write_fru_data(g_fruid, 0, frubuf, sfru, fdebug);
            free_fru();
            if (ret != 0) printf("write_fru error %d (0x%02x)\n",ret,ret);
	    else {  /* successful, show new data */
               ret = load_fru(sa,g_fruid,g_frutype);
	       if (ret != 0) show_loadfru_error(sa,g_fruid,ret);
               else ret = show_fru(sa,g_fruid,g_frutype);
               free_fru();
	    }
	 } 
      }
   }  /*end-else frestore */
   else if ((fwritefru != 0) && ret == 0) {
	printf("\nWriting new product data (%s,%s,%s,%s,%s,%s,%s,%s) ...\n",
                prodnew[0].tag, prodnew[1].tag, prodnew[2].tag,
                prodnew[3].tag, prodnew[4].tag, prodnew[5].tag,
                prodnew[6].tag, prodnew[7].tag);
	for (i = 0; i < NUM_PRODUCT_FIELDS; i++) {
	   len = prodnew[i].len;
	   if (len > 0) {
              if (len >= FIELD_LEN) len = FIELD_LEN - 1;
              if (len == 1) {
                 prodnew[i].tag[1] = ' ';
		 len++;
	      } 
	      prodarea[i].len = len;
	      memcpy(prodarea[i].tag,prodnew[i].tag,len);
	   }
        }
        ret = write_product();
        free_fru();
        if (ret != 0) printf("write_product error %d (0x%02x)\n",ret,ret);
	else {  /* successful, show new data */
           ret = load_fru(sa,g_fruid,g_frutype);
	   if (ret != 0) show_loadfru_error(sa,g_fruid,ret);
           else ret = show_fru(sa,g_fruid,g_frutype);
           free_fru();
	}
   }
   else 
	free_fru();

do_exit:
   ipmi_close_();
   show_outcome(progname,ret);  
   return(ret);
}

/* end ifruset.c */
