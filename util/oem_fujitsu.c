/*
 * oem_fujitsu.c
 *
 * This module handles OEM-specific functions for Fujitsu-Siemens firmware.
 *
 * References:
 *    http://manuals.ts.fujitsu.com/file/4390/irmc_s2-ug-en.pdf
 *
 * Authors: Andy Cress  arcress at users.sourceforge.net, and
 *          Dan Lukes   dan at obluda.cz
 *
 * Copyright (c) 2010 Kontron America, Inc.
 *
 * 08/17/10 Andy Cress v1.0  new, with source input from Dan Lukes
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
#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ipmicmd.h"
#include "ievents.h"
#include "oem_fujitsu.h"

/* extern void get_mfgid(int *vend, int *prod);  * from ipmicmd.h*/
/* extern int get_lan_options();  * from ipmicmd.h */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil fujitsuoem";
#else
static char * progver   = "3.08";
static char * progname  = "ifujitsuoem";
#endif
static char fdebug = 0;
static char freadok = 1;
#define ERRLED    0  /*GEL - red  Global Error LED*/
#define WARNLED   1  /*CSS - yellow warning LED*/
#define IDLED     2  /*ID  - blue Identify LED*/

#define NETFN_FUJITSU_IRMCS2  0xB8
#define CMD_FUJITSU_IRMCS2    0xF5
#define CMD_SPEC_FUJITSU_SET_ID_LED   0xB0
#define CMD_SPEC_FUJITSU_GET_ID_LED   0xB1
#define CMD_SPEC_FUJITSU_GET_ERR_LED  0xB3
#define LED_OFF   0
#define LED_ON    1
#define LED_BLINK 2

/*
 * get_alarmss_fujitsu
 * returns an array of 3 bytes:
 * offset 0 = ID blue LED state (0=off, 1=on, 2=blink)
 * offset 1 = CSS yellow LED state (0=off, 1=on, 2=blink)
 * offset 2 = GEL red LED state (0=off, 1=on, 2=blink)
 */
int get_alarms_fujitsu(uchar *rgalarms )
{
   int rv = -1;
   uchar idata[16];
   uchar rdata[16];
   int rlen;
   ushort icmd;
   uchar cc;
   int vend_id, prod_id;

   if (rgalarms == NULL) return(ERR_BAD_PARAM);
   memset(rgalarms,0,3);  
   /* check if iRMC s2 */
   get_mfgid(&vend_id,&prod_id);
   if (vend_id != VENDOR_FUJITSU) return(LAN_ERR_NOTSUPPORT);
   if (FUJITSU_PRODUCT_IS_iRMC_S1(prod_id)) return(LAN_ERR_NOTSUPPORT);

   /* get error, warning, id led statuses */
   icmd = CMD_FUJITSU_IRMCS2 | (IPMI_NET_FN_OEM_GROUP_RQ << 8);
   idata[0] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0x0000FF);
   idata[1] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0x00FF00) >> 8;
   idata[2] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0xFF0000) >> 16;
   idata[3] = CMD_SPEC_FUJITSU_GET_ID_LED;
   rlen = sizeof(rdata);
   rv = ipmi_cmd_mc(icmd, idata, 4, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   if (rv != 0) return(rv);
   rgalarms[0] = (rdata[3] & 0x03); /*id led, after the IANA response */
   icmd = CMD_FUJITSU_IRMCS2 | (IPMI_NET_FN_OEM_GROUP_RQ << 8);
   idata[0] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0x0000FF);
   idata[1] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0x00FF00) >> 8;
   idata[2] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0xFF0000) >> 16;
   idata[3] = CMD_SPEC_FUJITSU_GET_ERR_LED;
   rlen = sizeof(rdata);
   rv = ipmi_cmd_mc(icmd, idata, 4, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;
   switch(rdata[3]) {
   case 1: rgalarms[1] = LED_OFF;   rgalarms[2] = LED_ON; break;
   case 2: rgalarms[1] = LED_OFF;   rgalarms[2] = LED_BLINK;  break;
   case 3: rgalarms[1] = LED_ON;    rgalarms[2] = LED_OFF; break;
   case 4: rgalarms[1] = LED_ON;    rgalarms[2] = LED_ON; break;
   case 5: rgalarms[1] = LED_ON;    rgalarms[2] = LED_BLINK;  break;
   case 6: rgalarms[1] = LED_BLINK; rgalarms[2] = LED_OFF; break;
   case 7: rgalarms[1] = LED_BLINK; rgalarms[2] = LED_ON;  break;
   case 8: rgalarms[1] = LED_BLINK; rgalarms[2] = LED_BLINK; break;
   case 0: 
   default: rgalarms[1] = LED_OFF;  rgalarms[2] = LED_OFF; break;
   }

   return(rv);
}

static char *led_str(uchar b)
{
   char *pstr;
   switch(b) {
   case LED_ON:    pstr = "ON"; break;
   case LED_BLINK: pstr = "Blink"; break;
   case LED_OFF: 
   default:        pstr = "off"; break;
   }
   return(pstr);
}

/*
 * show_alarms_fujitsu
 * offset 0 = ID blue LED state (0=off, 1=on, 2=blink)
 * offset 1 = CSS yellow LED state (0=off, 1=on, 2=blink)
 * offset 2 = GEL red LED state (0=off, 1=on, 2=blink)
 */
int show_alarms_fujitsu(uchar *rgalarms)
{
   if (rgalarms == NULL) return(ERR_BAD_PARAM);
   /* show error, warning, id led statuses */
   printf("iRMC S2 ID  LED (blue)   = %s\n",led_str(rgalarms[0]));
   printf("iRMC S2 CSS LED (yellow) = %s\n",led_str(rgalarms[1]));
   printf("iRMC S2 GEL LED (red)    = %s\n",led_str(rgalarms[2]));
   return(0);
}

/* 
 * set_alarms_fujitsu
 * num: only the ID LED can be set, so num==0
 * val: 0=LED_OFF, 1=LED_ON
 */
int set_alarms_fujitsu(uchar num, uchar val)
{
   int rv = -1;
   uchar idata[16];
   uchar rdata[16];
   int rlen;
   ushort icmd;
   uchar cc;
   int vend_id, prod_id;

   get_mfgid(&vend_id,&prod_id);
   if (vend_id != VENDOR_FUJITSU) return(LAN_ERR_NOTSUPPORT);
   if (FUJITSU_PRODUCT_IS_iRMC_S1(prod_id)) return(LAN_ERR_NOTSUPPORT);

   /* set the specified LED number to val */
   icmd = CMD_FUJITSU_IRMCS2 | (IPMI_NET_FN_OEM_GROUP_RQ << 8);
   idata[0] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0x0000FF);
   idata[1] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0x00FF00) >> 8;
   idata[2] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0xFF0000) >> 16;
   idata[3] = CMD_SPEC_FUJITSU_SET_ID_LED;
   if (val == 0xFF) idata[4] = LED_ON;
   else if (val == 0) idata[4] = LED_OFF;
   else idata[4] = LED_BLINK;
   rlen = sizeof(rdata);
   rv = ipmi_cmd_mc(icmd, idata, 5, rdata, &rlen, &cc, fdebug);
   if ((rv == 0) && (cc != 0)) rv = cc;

   return(rv);
}

/* 
 * read_sel_fujitsu
 *
 * Fujitsu OEM
 * http://manuals.ts.fujitsu.com/file/4390/irmc_s2-ug-en.pdf
 *
 * Request
 * 0x2E - OEM network function
 * 0xF5 - OEM cmd
 * 0x?? - Fujitsu IANA (LSB first)
 * 0x?? - Fujitsu IANA
 * 0x?? - Fujitsu IANA
 * 0x43 - Command Specifier
 * 0x?? - Record ID (LSB first)
 * 0x?? - Record ID ; 0x0000 = "first record", 0xFFFF = "last record"
 * 0x?? - Offset (in response SEL text)
 * 0x?? - MaxResponseDataSize (size of converted SEL data 16:n in response, 
 *                             maximum is 100, some only handle 64)
 *
 * Response
 *    0xF5 - OEM cmd
 *    0x?? - Completion code
 *  0 0x?? - Fujitsu IANA (LSB first)
 *  1 0x?? - Fujitsu IANA
 *  2 0x?? - Fujitsu IANA
 *  3 0x?? - Next Record ID (LSB)
 *  4 0x?? - Next Record ID (MSB)
 *  5 0x?? - Actual Record ID (LSB)
 *  6 0x?? - Actual Record ID (MSB)
 *  7 0x?? - Record type
 *  8 0x?? - timestamp (LSB first)
 *  9 0x?? - timestamp
 * 10 0x?? - timestamp
 * 11 0x?? - timestamp
 * 12 0x?? - severity   
 *      bit 7   - CSS component
 *      bit 6-4 - 000 = INFORMATIONAL
 *                001 = MINOR
 *                010 = MAJOR
 *                011 = CRITICAL
 *                1xx = unknown
 *      bit 3-0 - reserved
 * 13 0x?? - data length (of the whole text)
 * 14 0x?? - converted SEL data
 *  requested number of bytes starting at requested offset 
 *  (MaxResponseDataSize-1 bytes of data) .....
 *    0x00 - trailing '\0' character
 */
int read_sel_fujitsu(uint16_t id, char *buf, int sz, char fdbg)
{
  int rv = -1;
  uint8_t bytes_rq[IPMI_OEM_MAX_BYTES];
  uint8_t bytes_rs[IPMI_OEM_MAX_BYTES];
  char    textbuf[IPMI_OEM_MAX_BYTES];
  int rs_len, text_len, data_len, chunk_len;
  int max_read_length;
  uint16_t actual_record_id = id;
  uint32_t timestamp = 0;
  // uint16_t next_record_id;
  // uint8_t record_type;
  uint8_t severity = 0;
  uint8_t ccode;
  char timestr[40]; 
  char *severity_text = NULL;
  uint8_t offset = 0;
  int vend_id, prod_id;

  fdebug = fdbg;
  if (buf == NULL) return(rv);
  max_read_length = IPMI_OEM_MAX_BYTES;
  data_len = IPMI_OEM_MAX_BYTES;
  /* This command requires admin privilege, so check and
     print a warning if Fujitsu and not admin. */
  get_mfgid(&vend_id,&prod_id);
  if (vend_id == VENDOR_FUJITSU) {  /* connected to Fujitsu MC */
     int auth, priv;
     if (FUJITSU_PRODUCT_IS_iRMC_S1(prod_id)) {
        max_read_length = 32;
        data_len = 80;
     }
     rv = get_lan_options(NULL,NULL,NULL,&auth,&priv,NULL,NULL,NULL);
     if ((rv == 0) && (priv < 4)) { /*remote and not admin priv*/
        printf("*** Admin privilege (-V 4) required for full OEM decoding.\n");
     }
  }

  memset(textbuf,0,sizeof(textbuf));
  bytes_rq[0] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0x0000FF);
  bytes_rq[1] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0x00FF00) >> 8;
  bytes_rq[2] = (IPMI_IANA_ENTERPRISE_ID_FUJITSU & 0xFF0000) >> 16;
  bytes_rq[3] = IPMI_OEM_FUJITSU_COMMAND_SPECIFIER_GET_SEL_ENTRY_LONG_TEXT;
  bytes_rq[4] = (id & 0x00FF);
  bytes_rq[5] = (id & 0xFF00) >> 8;
  bytes_rq[7] = (uchar)max_read_length;

  while (offset < data_len) 
  {
     bytes_rq[6] = offset;
     /* Do not ask BMC for data beyond data_len, request partial if so. */
     if (offset + max_read_length > data_len) 
  		bytes_rq[7] = data_len - offset;
     rs_len = sizeof(bytes_rs);
     rv = ipmi_cmdraw(IPMI_CMD_OEM_FUJITSU_SYSTEM, IPMI_NET_FN_OEM_GROUP_RQ,
		      BMC_SA, PUBLIC_BUS, BMC_LUN,
		      bytes_rq, 9, bytes_rs, &rs_len, &ccode, fdebug);
     if (fdebug) printf("read_sel_fujitsu rv = %d, cc = %x\n", rv, ccode);
     if (rv == 0 && ccode != 0) rv = ccode;
     if (rv != 0) return(rv);
     if (fdebug) dump_buf("read_sel_fujitsu data",bytes_rs,rs_len,1);

     if (offset == 0) { /* only need to set these on the first pass */
        actual_record_id = bytes_rs[5] + (bytes_rs[6] << 8);
        timestamp =   bytes_rs[8] + (bytes_rs[9] << 8) + 
		(bytes_rs[10] << 16) + (bytes_rs[11] << 24);
        severity = (bytes_rs[12] >> 3);
        data_len = bytes_rs[13];
        if (data_len > IPMI_OEM_MAX_BYTES) data_len = IPMI_OEM_MAX_BYTES;
      }
      chunk_len = rs_len - 14;
      if ((offset + chunk_len) > IPMI_OEM_MAX_BYTES) 
		chunk_len = IPMI_OEM_MAX_BYTES - offset; 
      memcpy(&textbuf[offset],&bytes_rs[14],chunk_len);
      offset += (uint8_t)chunk_len;
  }
  textbuf[IPMI_OEM_MAX_BYTES-1]='\0'; /*stringify*/
  text_len = strlen_(textbuf);

  switch (severity) {
  case  0: severity_text = "INFORMATIONAL:"; break;
  case  1: severity_text = "MINOR:"; break;
  case  2: severity_text = "MAJOR:"; break;
  case  3: severity_text = "CRITICAL:"; break;
  case  4: 
  case  5:
  case  6:
  case  7: severity_text = ""; break;
  case  8: severity_text = "INFORMATIONAL/CSS:"; break;
  case  9: severity_text = "MINOR/CSS:"; break;
  case 10: severity_text = "MAJOR/CSS:"; break;
  case 11: severity_text = "CRITICAL/CSS:"; break;
  case 12: 
  case 13:
  case 14:
  case 15: severity_text = "unknown/CSS:"; break;
  }

  fmt_time(timestamp, timestr, sizeof(timestr));
  snprintf(buf, sz, "%u | %s | %s %s\n", (int)actual_record_id, 
			timestr, severity_text, textbuf);
  return rv;
}

/* 
 * decode_sel_fujitsu
 * inputs:
 *    evt    = the 16-byte IPMI SEL event
 *    outbuf = points to the output string buffer
 *    outsz  = size of the output buffer
 * outputs: 
 *    rv     = 0 if this event was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    outbuf = will contain the interpreted event text string (if rv==0)
 */
int decode_sel_fujitsu(uint8_t *evt, char *outbuf, int outsz, char fdesc, 
			char fdbg)
{
   int rv = -1;
   uint16_t id;
   uint8_t rectype;
   int oemid;
   uint32_t timestamp;
   char mybuf[64]; 
   char *type_str = NULL;
   char *gstr = NULL;
   char *pstr = NULL;
   ushort genid;
   int sevid;

   fdebug = fdbg;
   id = evt[0] + (evt[1] << 8);
   if (freadok) {
      rv = read_sel_fujitsu(id, outbuf, outsz, fdbg);
      if (rv == 0) return(rv);  /*success, done*/
   }
   freadok = 0;  /*not fujitsu or not local, so do not retry */

   sevid = SEV_INFO;
   /* instead try to decode some events manually */
   rectype = evt[2];
   timestamp = evt[3] + (evt[4] << 8) + (evt[5] << 16) + (evt[6] << 24);
   genid = evt[7] | (evt[8] << 8);
   if (rectype == 0xc1) {  /* OEM type C1 */
      oemid = evt[7] + (evt[8] << 8) + (evt[9] << 16);
      if (oemid == VENDOR_FUJITSU) {
	 type_str = "Fujitsu";
         gstr = "BMC ";
	 switch(evt[10]) {
	   case 0x09:
		sprintf(mybuf,"iRMC S2 CLI/Telnet user %d login from %d.%d.%d.%d",
			evt[11], evt[12], evt[13], evt[14], evt[15]);
		break;
	   case 0x0a:
		sprintf(mybuf,"iRMC S2 CLI/Telnet user %d logout from %d.%d.%d.%d",
			evt[11], evt[12], evt[13], evt[14], evt[15]);
		break;
	   default:
		sprintf(mybuf,"iRMC S2 Event %02x %02x %02x %02x %02x %02x",
			evt[10], evt[11], evt[12], evt[13], evt[14], evt[15]);
		break;
	 }
	 format_event(id,timestamp, sevid, genid, type_str,
			evt[10],NULL,mybuf,NULL,outbuf,outsz);
	 rv = 0;
      } /*endif fujitsu oem */
   } else if (rectype == 0x02) {
      type_str = "iRMC S2";
      gstr = "BMC ";
      sprintf(mybuf,"%02x [%02x %02x %02x]", evt[12],evt[13],evt[14],evt[15]);
      switch(evt[10]) {  /*sensor type*/
	case 0xc8:
	 	switch(evt[13]) {
		  case 0x29:
		      sprintf(mybuf,"CLI/Telnet user %d login", evt[15]);
		      break;
		  case 0x2a:
		      sprintf(mybuf,"CLI/Telnet user %d logout", evt[15]);
		      break;
		  case 0x21:
		      sprintf(mybuf,"Browser user %d login", evt[15]);
		      break;
		  case 0x22:
		      sprintf(mybuf,"Browser user %d logout", evt[15]);
		      break;
		  case 0x23:
		      sprintf(mybuf,"Browser user %d auto-logout", evt[15]);
		      break;
		  default:  /*mybuf has the raw bytes*/
		      break;
		}
		pstr = mybuf;
		rv = 0;
		break;
	case 0xca:
		if (evt[13] == 0x26) pstr = "Paging: Email - notification failed";
		else if (evt[13] == 0xa6) pstr = "Paging: Email - DNS failed";
		else pstr = mybuf;
		rv = 0;
		break;
	case 0xe1:
		sevid = SEV_MAJ;
		if (evt[13] == 0x0f) pstr = "MC access degraded";
		else pstr = mybuf;
		rv = 0;
		break;
	case 0xec:
		if (evt[13] == 0xa0) {
		   sprintf(mybuf,"Firmware flash version %d.%d",
				(evt[14] & 0x0f),evt[15]);
		}
		pstr = mybuf;
		rv = 0;
		break;
	case 0xee:
		type_str = "iRMC S2";
		if (evt[12] == 0x0a && evt[13] == 0x80) 
			pstr = "Automatic restart after power fail";
		else pstr = mybuf;
		rv = 0;
		break;
	default:  break;
      }
      if (rv == 0) {
	 format_event(id,timestamp, sevid, genid, type_str,
			evt[11],NULL,pstr,NULL,outbuf,outsz);
                        /*evt[11] = sensor number */
      }
   } 
   return rv;
}


/*
 * Fujitsu iRMC S1 / iRMC S2 OEM Sensor logic
 */

/* 0xDD */
const char * const ipmi_sensor_type_oem_fujitsu_system_power_consumption[] =
  {
    /* EN 0x00 */	"System Power Consumption within Limit",
    /* EN 0x01 */	"System Power Consumption above Warning Level",
    /* EN 0x02 */	"System Power Consumption above Critical Level",
    /* EN 0x03 */	"System Power Consumption limiting disabled",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_system_power_consumption_max_index = 0x03;


/* 0xDE */
const char * const ipmi_sensor_type_oem_fujitsu_memory_status[] =
  {
    /* EN 0x00 */	"Empty slot",
    /* EN 0x01 */	"OK",
    /* EN 0x02 */	"Reserved",
    /* EN 0x03 */	"Error",
    /* EN 0x04 */	"Fail",
    /* EN 0x05 */	"Prefailure",
    /* EN 0x06 */	"Reserved",
    /* EN 0x07 */	"Unknown",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_memory_status_max_index = 0x07;

/* 0xDF */
const char * const ipmi_sensor_type_oem_fujitsu_memory_config[] =
  {
    /* EN 0x00 */	"Normal",
    /* EN 0x01 */	"Disabled",
    /* EN 0x02 */	"Spare module",
    /* EN 0x03 */	"Mirrored module",
    /* EN 0x04 */	"RAID module",
    /* EN 0x05 */	"Not Usable",
    /* EN 0x06 */	"Unspecified state(6)",
    /* EN 0x07 */	"Unspecified state(7)",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_memory_config_max_index = 0x07;

/* 0xE1 */
const char * const ipmi_sensor_type_oem_fujitsu_memory[] =
  {
    /* EN 0x00 */	"Non Fujitsu memory module detected",
    /* EN 0x01 */	"Memory module replaced",
    /* EN 0x02 */	"Fatal general memory error",
    /* EN 0x03 */	"Recoverable general memory error",
    /* EN 0x04 */	"Recoverable ECC memory error",
    /* EN 0x05 */	"Recoverable CRC memory error",
    /* EN 0x06 */	"Fatal CRC memory error",
    /* EN 0x07 */	"Recoverable thermal memory event",
    /* EN 0x08 */	"Fatal thermal memory error",
    /* EN 0x09 */	"Too many correctable memory errors",
    /* EN 0x0A */	"Uncorrectable Parity memory error",
    /* EN 0x0B */	"Memory Modules swapped",
    /* EN 0x0C */	"Memory Module moved",
    /* EN 0x0D */	"Memory removed",
    /* EN 0x0E */	"Memory Re-inserted",
    /* EN 0x0F */	"Memory module(s) changed",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_memory_max_index = 0x0F;

/* 0xE3 */
const char * const ipmi_sensor_type_oem_fujitsu_hw_error[] =
  {
    /* EN 0x00 */	"TPM Error",
    /* EN 0x01 */	"Reserved",
    /* EN 0x02 */	"No usable CPU",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_hw_error_max_index = 0x02;

/* 0xE4 */
const char * const ipmi_sensor_type_oem_fujitsu_sys_error[] =
  {
    /* EN 0x00 */	"System configuration Data error",
    /* EN 0x01 */	"Resource Conflict",                  /* Slot in EventData3 */
    /* EN 0x02 */	"IRQ not configured",                 /* Slot in EventData3 */
    /* EN 0x03 */	"Device node allocation error",       /* Device in EventData3 */
    /* EN 0x04 */	"Expansion ROM Slot not initialized", /* Slot in EventData3 */
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_sys_error_max_index = 0x04;

/* 0xE6 */
const char * const ipmi_sensor_type_oem_fujitsu_fan_status[] =
  {
    /* EN 0x00 */	"FAN on, running",
    /* EN 0x01 */	"FAN failed",
    /* EN 0x02 */	"FAN prefailure",
    /* EN 0x03 */	"Redundant FAN failed",
    /* EN 0x04 */	"FAN not manageable",
    /* EN 0x05 */	"FAN not installed",
    /* EN 0x06 */	"FAN unspecified state(6)",
    /* EN 0x07 */	"FAN in init phase",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_fan_status_max_index = 0x07;

/* 0xE8 */
const char * const ipmi_sensor_type_oem_fujitsu_psu_status[] =
  {
    /* EN 0x00 */	"Power supply - Not present",
    /* EN 0x01 */	"Power supply - OK",
    /* EN 0x02 */	"Power supply - Failed",
    /* EN 0x03 */	"Redundant power supply - AC failed",
    /* EN 0x04 */	"Redundant power supply - DC failed",
    /* EN 0x05 */	"Power supply - Critical Temperature",
    /* EN 0x06 */	"Power supply - Not manageable",
    /* EN 0x07 */	"Power supply - Fan failure predicted",
    /* EN 0x08 */	"Power supply - Fan failed",
    /* EN 0x09 */	"Power supply - Power Save Mode",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_psu_status_max_index = 0x09;

/* 0xE9 */
const char * const ipmi_sensor_type_oem_fujitsu_psu_redundancy[] =
  {
    /* EN 0x00 */	"Power Supply - redundancy present",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_psu_redundancy_max_index = 0x00;

/* 0xEC */
const char * const ipmi_sensor_type_oem_fujitsu_flash[] =
  {
    /* EN 0x00 */	"Online firmware flash",
    /* EN 0x01 */	"Online firmware flash: reboot",
    /* EN 0x02 */	"BIOS TFTP Flash: OK",
    /* EN 0x03 */	"BIOS TFTP Flash: failed",
    /* EN 0x04 */	"iRMC TFTP Flash: OK",
    /* EN 0x05 */	"iRMC TFTP Flash: failed",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_flash_max_index = 0x05;

/* 0xEF */
const char * const ipmi_sensor_type_oem_fujitsu_config_backup[] =
  {
    /* EN 0x00 */	"Chassis IDPROM: Motherboard Exchange detected",
    /* EN 0x01 */	"Chassis IDPROM: Read or Write error",
    /* EN 0x02 */	"Chassis IDPROM: Restore successful",
    /* EN 0x03 */	"Chassis IDPROM: Restore failed",
    /* EN 0x04 */	"Chassis IDPROM: Backup successful",
    /* EN 0x05 */	"Chassis IDPROM: Backup failed",
    /* EN 0x06 */	"Chassis IDPROM: Feature disabled",
    /* EN 0x07 */	"Chassis IDPROM: Function Not Available",
    /* EN 0x08 */	"Reserved",
    /* EN 0x09 */	"Reserved",
    /* EN 0x0A */	"Reserved",
    /* EN 0x0B */	"Reserved",
    /* EN 0x0C */	"Reserved",
    /* EN 0x0D */	"Reserved",
    /* EN 0x0E */	"Reserved",
    /* EN 0x0F */	"NVRAM defaults loaded",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_config_backup_max_index = 0x0F;

/* 0xEF */
const char * const ipmi_sensor_type_oem_fujitsu_i2c_bus[] =
  {
    /* EN 0x00 */	"I2C Bus Error",
    /* EN 0x01 */	"I2C Bus OK",
    /* EN 0x02 */	"I2C Bus Disabled",
    /* EN 0x03 */	"I2C Bus Failed",
    NULL
  };
unsigned int ipmi_sensor_type_oem_fujitsu_i2c_bus_max_index = 0x03;

static char * get_array_message (unsigned int offset,
                    unsigned int offset_max,
                    const char * const string_array[])
{
  if (offset > offset_max) return("unknown");
  return((char *)string_array[offset]);
}


static char *get_oem_reading_string(uchar sensor_type, uchar offset)
{
  char * pstr = "";
  /* 
   * OEM Interpretation
   * Fujitsu iRMC S1 / iRMC S2
   */
      switch (sensor_type)
        {
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_I2C_BUS:
          return (get_array_message (offset, 
                                      ipmi_sensor_type_oem_fujitsu_i2c_bus_max_index,
                                      ipmi_sensor_type_oem_fujitsu_i2c_bus));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_SYSTEM_POWER_CONSUMPTION:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_system_power_consumption_max_index,
                                      ipmi_sensor_type_oem_fujitsu_system_power_consumption));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_MEMORY_STATUS:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_memory_status_max_index,
                                      ipmi_sensor_type_oem_fujitsu_memory_status));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_MEMORY_CONFIG:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_memory_config_max_index,
                                      ipmi_sensor_type_oem_fujitsu_memory_config));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_MEMORY:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_memory_max_index,
                                      ipmi_sensor_type_oem_fujitsu_memory));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_HW_ERROR:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_hw_error_max_index,
                                      ipmi_sensor_type_oem_fujitsu_hw_error));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_SYS_ERROR:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_sys_error_max_index,
                                      ipmi_sensor_type_oem_fujitsu_sys_error));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_FAN_STATUS:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_fan_status_max_index,
                                      ipmi_sensor_type_oem_fujitsu_fan_status));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_PSU_STATUS:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_psu_status_max_index,
                                      ipmi_sensor_type_oem_fujitsu_psu_status));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_PSU_REDUNDANCY:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_psu_redundancy_max_index,
                                      ipmi_sensor_type_oem_fujitsu_psu_redundancy));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_FLASH:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_flash_max_index,
                                      ipmi_sensor_type_oem_fujitsu_flash));
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_CONFIG_BACKUP:
          return (get_array_message (offset,
                                      ipmi_sensor_type_oem_fujitsu_config_backup_max_index,
                                      ipmi_sensor_type_oem_fujitsu_config_backup));
        /* These are reserved */
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_COMMUNICATION:
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_EVENT:
        default:
          break;
        }
  return(pstr);
}


static char *get_oem_sensor_type_string (uint8_t sensor_type)
{
   switch (sensor_type) {
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_I2C_BUS       : return ("OEM I2C Bus");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_SYSTEM_POWER_CONSUMPTION: return ("OEM Power Consumption");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_MEMORY_STATUS : return ("OEM Memory Status");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_MEMORY_CONFIG : return ("OEM Memory Config");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_MEMORY        : return ("OEM Memory");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_FAN_STATUS    : return ("OEM Fan Status");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_PSU_STATUS    : return ("OEM PSU Status");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_PSU_REDUNDANCY: return ("OEM PSU Redundancy");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_COMMUNICATION : return ("OEM Communication");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_FLASH         : return ("OEM Flash");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_EVENT         : return ("OEM Event");
        case IPMI_SENSOR_TYPE_OEM_FUJITSU_CONFIG_BACKUP : return ("OEM Config Backup");
        default  : break; /* fall into generic case below */
    }
  return ("");
}


int decode_sensor_fujitsu(uchar *sdr,uchar *reading,char *pstring, int slen)
{
   int rv = -1;
   char *typestr = NULL;
   char *readstr = NULL;
   uchar stype;
   int vend_id, prod_id;

   /* Only get here if vend_id == VENDOR_FUJITSU */
   if (sdr == NULL || reading == NULL) return(rv);
   if (pstring == NULL || slen == 0) return(rv);
   stype = sdr[12];
   typestr = get_oem_sensor_type_string(stype);

   get_mfgid(&vend_id,&prod_id);
   if (vend_id == IPMI_IANA_ENTERPRISE_ID_FUJITSU
       && (prod_id >= IPMI_FUJITSU_PRODUCT_ID_MIN 
       &&  prod_id <= IPMI_FUJITSU_PRODUCT_ID_MAX)) {
          readstr = get_oem_reading_string(stype,reading[2]);
	  if (readstr != NULL && (readstr[0] != 0)) rv = 0; 
   } else readstr = "";
   snprintf (pstring, slen, "%s = %s",typestr,readstr);
   return(rv);
}

#ifdef METACOMMAND
int i_fujitsuoem(int argc, char **argv)
{
   printf("%s ver %s\n", progname,progver);
   return(0);
}
#endif
/* end oem_fujitsu.c */
