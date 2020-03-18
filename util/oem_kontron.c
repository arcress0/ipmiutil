/*
 * oem_kontron.c
 * This code implements Kontron OEM proprietary commands.
 *
 * Change history:
 *  08/25/2010 ARCress - included in source tree
 *
 *---------------------------------------------------------------------
 */
/*
 * Copyright (c) 2004 Kontron Canada, Inc.  All Rights Reserved.
 *
 * Base on code from
 * Copyright (c) 2003 Sun Microsystems, Inc.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistribution of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistribution in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of Sun Microsystems, Inc. or the names of
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * This software is provided "AS IS," without a warranty of any kind.
 * ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED.
 * SUN MICROSYSTEMS, INC. ("SUN") AND ITS LICENSORS SHALL NOT BE LIABLE
 * FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING
 * OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.  IN NO EVENT WILL
 * SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA,
 * OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
 * PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY OF
 * LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

/*
 * Tue Mar 7 14:36:12 2006
 * <stephane.filion@ca.kontron.com>
 *
 * This code implements an Kontron OEM proprietary commands.
 */

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#include <time.h>
#define uint8_t  unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
typedef uint32_t   socklen_t; 
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipmicmd.h"
#include "ievents.h"
#include "oem_kontron.h"

#define FRU_TYPE_COMPONENT  0x01
#define FRU_TYPE_BASEBOARD  0x07

#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil kontronoem";
#else
static char * progver   = "3.08";
static char * progname  = "ikontronoem";
#endif
const struct valstr ktc5520_post[] = {  /*from EAS*/
 { 0x0003, "Start POST Init" },
 { 0x0004, "Check CMOS" },
 { 0x0005, "Init interrupt HW" },
 { 0x0006, "Init INT1Ch" },
 { 0x0008, "Init CPUs" },
 { 0x000A, "Init 8042 Keyboard" },
 { 0x000B, "Detect PS2 Mouse" },
 { 0x000C, "Detect KBC Keyboard" },
 { 0x000E, "Test Input Devices" },
 { 0x0013, "PreInit chipset regs" },
 { 0x0024, "Init platform modules" },
 { 0x002A, "Init PCI devices" },
 { 0x002C, "Init Video" },
 { 0x002E, "Init Output devices" },
 { 0x0030, "Init SMI" },
 { 0x0031, "Init ADM module" },
 { 0x0033, "Init Silent boot" },
 { 0x0037, "Show first message" },
 { 0x0038, "Init Buses" },
 { 0x0039, "Init DMAC" },
 { 0x003A, "Init RTC" },
 { 0x003B, "Memory Test" },
 { 0x003C, "MidInit chipset regs" },
 { 0x0040, "Detect Ports" },
 { 0x0050, "Adjust RAM size" },
 { 0x0052, "Update CMOS size" },
 { 0x0060, "Init Kbd Numlock" },
 { 0x0075, "Init Int13" },
 { 0x0078, "Init IPL" },
 { 0x007A, "Init OpRoms" },
 { 0x007C, "Write ESCD to NV" },
 { 0x0084, "Log errors" },
 { 0x0085, "Display errors" },
 { 0x0087, "Run BIOS Setup" },
 { 0x008C, "LateInit chipset regs" },
 { 0x008D, "Build ACPI Tables" },
 { 0x008E, "Program peripherals" },
 { 0x0090, "LateInit SMI" },
 { 0x00A0, "Check Boot password" },
 { 0x00A1, "PreBoot Cleanup" },
 { 0x00A2, "Init Runtime image" },
 { 0x00A4, "Init Runtime lang" },
 { 0x00A7, "Show Config, Init MTRR" },
 { 0x00A8, "Prep CPU for OS boot" },
 { 0x00A9, "Wait for input" },
 { 0x00AA, "Deinit ADM, Int1C" },
 { 0x00AB, "Prepare BBS" },
 { 0x00AC, "EndInit chipse regs" },
 { 0x00B1, "Save context for ACPI" },
 { 0x00C0, "EarlyCPU Init APIC" },
 { 0x00C1, "Setup BSP info" },
 { 0x00C2, "Setup BSP for POST" },
 { 0x00C5, "Setup App Processors" },
 { 0x00C6, "Setup BSP cache" },
 { 0x00C7, "EarlyCPU Init exit" },
 { 0x0000, "OS Loader" },
 { 0xffff , NULL }  /*end of list*/
};
/* 
61-70, OEM POST Error. This range is reserved for chipset vendors & system manufacturers. The error associated with this value may be different from one platform to the next.
DD-DE, OEM PCI init debug POST code during DIMM init, See DIM Code Checkpoints section of document for more information.
DD-DE, OEM PCI init debug POST code during DIMM init. DEh during BUS number
*/

extern int verbose;  /*ipmilanplus.c*/
#ifdef METACOMMAND
extern int load_fru(uchar sa, uchar frudev, uchar frutype, uchar **pfrubuf);
extern int write_fru_data(uchar id, ushort offset, uchar *data, int dlen, 
				char fdebug);  /*ifru.c*/
#endif

/* local function prototypes */
static void ipmi_kontron_help(void);
static int ipmi_kontron_set_serial_number(void * intf);
static int ipmi_kontron_set_mfg_date (void * intf);

static void print_buf( uint8_t * buf, int len,  char * desc)
{ 
   dump_buf(desc,buf,len,0); 
}

#ifdef METACOMMAND
/* get_fru_area_str  -  Parse FRU area string from raw data
 * @data:	raw FRU data
 * @offset:	offset into data for area
 * returns pointer to FRU area string
 */
static char * get_fru_area_str(uint8_t * data, uint32_t * offset)
{
	static const char bcd_plus[] = "0123456789 -.:,_";
	char * str;
	int len, off, size, i, j, k, typecode;
	union {
		uint32_t bits;
		char chars[4];
	} u;

	size = 0;
	off = *offset;
	/* bits 6:7 contain format */
	typecode = ((data[off] & 0xC0) >> 6);

	/* bits 0:5 contain length */
	len = data[off++];
	len &= 0x3f;

	switch (typecode) {
	case 0:				/* 00b: binary/unspecified */
		/* hex dump -> 2x length */
		size = (len*2);
		break;
	case 2:				/* 10b: 6-bit ASCII */
		/* 4 chars per group of 1-3 bytes */
		size = ((((len+2)*4)/3) & ~3);
		break;
	case 3:				/* 11b: 8-bit ASCII */
	case 1:				/* 01b: BCD plus */
		/* no length adjustment */
		size = len;
		break;
	}

	if (size < 1) {
		*offset = off;
		return NULL;
	}
	str = malloc(size+1);
	if (str == NULL)
		return NULL;
	memset(str, 0, size+1);

	if (len == 0) {
		str[0] = '\0';
		*offset = off;
		return str;
	}

	switch (typecode) {
	case 0:			/* Binary */
		strncpy(str, buf2str(&data[off], len), len*2);
		break;

	case 1:			/* BCD plus */
		for (k=0; k<len; k++)
			str[k] = bcd_plus[(data[off+k] & 0x0f)];
		str[k] = '\0';
		break;

	case 2:			/* 6-bit ASCII */
		for (i=j=0; i<len; i+=3) {
			u.bits = 0;
			k = ((len-i) < 3 ? (len-i) : 3);
#if WORDS_BIGENDIAN
			u.chars[3] = data[off+i];
			u.chars[2] = (k > 1 ? data[off+i+1] : 0);
			u.chars[1] = (k > 2 ? data[off+i+2] : 0);
#define CHAR_IDX 3
#else
			memcpy((void *)&u.bits, &data[off+i], k);
#define CHAR_IDX 0
#endif
			for (k=0; k<4; k++) {
				str[j++] = ((u.chars[CHAR_IDX] & 0x3f) + 0x20);
				u.bits >>= 6;
			}
		}
		str[j] = '\0';
		break;

	case 3:
		memcpy(str, &data[off], len);
		str[len] = '\0';
		break;
	}

	off += len;
	*offset = off;

	return str;
}
#endif

static char *bootdev[] = {"BIOS", "FDD", "HDD", "CDROM", "network", 0};

static void
ipmi_kontron_nextboot_help(void)
{
   int i;
   printf("nextboot <device>\n"
          "Supported devices:\n");
   for (i = 0; bootdev[i] != 0; i++) {
       printf("- %s\n", bootdev[i]);
   }
}

/* ipmi_kontron_next_boot_set - Select the next boot order on CP6012
 *
 * @intf:               ipmi interface
 * @id:         fru id
 *
 * returns -1 on error
 * returns 1 if successful
 */
static int
ipmi_kontron_nextboot_set(void * intf, int argc, char **argv)
{
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   uint8_t msg_data[8];
   int i;

   memset(msg_data, 0, sizeof(msg_data));
   msg_data[0] = 0xb4;
   msg_data[1] = 0x90;
   msg_data[2] = 0x91;
   msg_data[3] = 0x8b;
   msg_data[4] = 0x9d;
   msg_data[5] = 0xFF;
   msg_data[6] = 0xFF; /* any */

   for (i = 0; bootdev[i] != 0; i++) {
       if (strcmp(argv[0], bootdev[i]) == 0) {
           msg_data[5] = (uchar)i;
           break;
       }
   }

   /* Invalid device selected? */
   if (msg_data[5] == 0xFF) {
       printf("Unknown boot device: %s\n", argv[0]);
       return -1;
   }

   memset(&req, 0, sizeof(req));
   req.msg.netfn = 0x3E;
   req.msg.cmd = 0x02;
   req.msg.data = msg_data;
   req.msg.data_len = 7;

   /* Set Lun temporary, necessary for this oem command */
   req.msg.lun = 0x03;

   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv < 0)
   {
      printf(" Device not present (No Response)\n");
      return (rv);
   }
   if (rv > 0) {
      printf(" NextBoot Device error (%s)\n",decode_cc(0,rv));
      return(rv);
   }
   return (rv);
}

#ifdef METACOMMAND
int
ipmi_kontronoem_main(void * intf, int argc, char ** argv)
{
   int rc = 0;

   if (argc == 0)
      ipmi_kontron_help();
   else if (strncmp(argv[0], "help", 4) == 0)
      ipmi_kontron_help();

   else if(!strncmp(argv[0], "setsn", 5))
   {
      if(argc >= 1)
      {
         rc = ipmi_kontron_set_serial_number(intf);
         if(rc == 0) {
            printf("FRU serial number set successfully\n");
         } else {
            printf("FRU serial number set failed\n");
         }
      }
      else
      {
         printf("fru setsn\n");
      }
   }
   else if(!strncmp(argv[0], "setmfgdate", 5))
   {
      if(argc >= 1)
      {
         rc = ipmi_kontron_set_mfg_date(intf);
	 if (rc == 0) {
            printf("FRU manufacturing date set successfully\n");
         } else {
            printf("FRU manufacturing date set failed\n");
         }      
      }
      else
      {
         printf("fru setmfgdate\n");
      }

   }
   else if (!strncmp(argv[0], "nextboot", sizeof("nextboot")-1))
   {
      if (argc > 1)
      {
         if ((rc = ipmi_kontron_nextboot_set(intf, argc-1, argv+1)) == 0)
         {
            printf("Nextboot set successfully\n");
         } else {
            printf("Nextboot set failed\n");
         }
      } else {
         ipmi_kontron_nextboot_help();
      }
   }

   else 
   {
      printf("Invalid Kontron command: %s", argv[0]);
      ipmi_kontron_help();
      rc = ERR_USAGE;
   }

   return rc;
}

static void ipmi_kontron_help(void)
{
   printf("Kontron Commands:  setsn setmfgdate nextboot\n");
}   

/* ipmi_fru_set_serial_number -  Set the Serial Number in FRU
 *
 * @intf:		ipmi interface
 * @id:		fru id
 *
 * returns -1 on error
 * returns 1 if successful
 */
static int
ipmi_kontron_set_serial_number(void * intf)
{
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   struct fru_info fru;
   struct fru_header header;
   uint8_t msg_data[4];
   char *sn;
   uint8_t sn_size, checksum;
   uint8_t  *fru_data;
   char *fru_area;
   uint32_t fru_data_offset, fru_data_offset_tmp, board_sec_len, prod_sec_len, i;
   uint8_t bus, sa, lun, mtype;
   
   sn = NULL;
   fru_data = NULL;
   
   ipmi_get_mc(&bus, &sa, &lun, &mtype);

   memset(msg_data, 0, 4);
   msg_data[0] = 0xb4;
   msg_data[1] = 0x90;
   msg_data[2] = 0x91;
   msg_data[3] = 0x8b;
   
   memset(&req, 0, sizeof(req));
   req.msg.netfn = 0x3E;
   req.msg.cmd = 0x0C;
   req.msg.data = msg_data;
   req.msg.data_len = 4;
   
   /* Set Lun, necessary for this oem command */
   req.msg.lun = 0x03;

   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv < 0)
   {
      printf(" Device not present (No Response)\n");
      return (rv);
   }
   
   if (rv > 0) 
   {
      if (verbose) printf("sernum cmd ccode = %02x\n",rv);
      printf(" This option is not implemented for this board\n");
#ifdef TEST
      strcpy(rsp,"serialnumber");
      rsp_len = 12;
#else
      return (rv);
#endif
   }
  
   sn_size = (uchar)rsp_len;
   if (sn_size < 1)
   {
      printf(" Original serial number is zero length, was cleared.\n");
      return(ERR_BAD_LENGTH);
   }
   sn = malloc(sn_size + 1);
   if(sn == NULL)
   {
      printf("Out of memory!");
      return(-1);
   }

   memset(sn, 0, sn_size + 1);
   memcpy(sn, rsp, sn_size);
   
   if(verbose >= 1)
   {
      printf("Original serial number is : [%s]\n", sn);
   }
   
   memset(msg_data, 0, 4);
   msg_data[0] = 0;
   
   memset(&req, 0, sizeof(req));
   req.msg.netfn = IPMI_NETFN_STORAGE;
   req.msg.cmd = GET_FRU_INFO;
   req.msg.data = msg_data;
   req.msg.data_len = 1;
   
   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv < 0) {
      printf(" Device not present (No Response)\n");
      free(sn);
      return (rv);
   }
   if (rv > 0) {
      printf(" Device not present (%s)\n",decode_cc(0,rv));
      free(sn);
      return (rv);
   }

   fru.size = (rsp[1] << 8) | rsp[0];
   fru.access = rsp[2] & 0x1;

   if (fru.size < 1) {
      printf(" Invalid FRU size %d", fru.size);
      free(sn);
      return(ERR_BAD_LENGTH);
   }
 
   /*
    * retrieve the FRU header
    */
   msg_data[0] = 0;
   msg_data[1] = 0;
   msg_data[2] = 0;
   msg_data[3] = 8;

   memset(&req, 0, sizeof(req));
   req.msg.netfn = IPMI_NETFN_STORAGE;
   req.msg.cmd = GET_FRU_DATA;
   req.msg.data = msg_data;
   req.msg.data_len = 4;

   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv < 0) 
   {
      printf(" Device not present (No Response)\n");
      free(sn);
      return(rv);
   }
   if (rv > 0) 
   {
      printf(" Device not present (%s)\n",decode_cc(0,rv));
      free(sn);
      return(rv);
   }

   if (verbose >= 1)
      print_buf(rsp, rsp_len, "FRU DATA");

   memcpy(&header, rsp + 1, 8);

   if (header.version != 1) 
   {
      printf(" Unknown FRU header version 0x%02x\n", header.version);
      free(sn);
      return(-1);
   }   
   
   /* Set the Board Section */
   board_sec_len =   (header.offset.product * 8) - (header.offset.board * 8);

   rv = load_fru(sa,0, FRU_TYPE_BASEBOARD, &fru_data);
   if (rv != 0) 
   {
      printf(" Cannot Read FRU, error %d\n",rv);
      free(sn);
      if (fru_data != NULL) free(fru_data);
      return(rv);
   }
   
   if (verbose >= 1)
      print_buf(fru_data, fru.size, "FRU BUFFER");

   /*Position at Board Manufacturer*/
   fru_data_offset = (header.offset.board * 8) + 6;
   fru_area = &fru_data[fru_data_offset];
   if (verbose) 
      printf("board_area: offset=0x%x len=%d\n",fru_data_offset,board_sec_len);

   fru_area = get_fru_area_str(fru_data, &fru_data_offset);

   /*Position at Board Product Name*/
   fru_area = get_fru_area_str(fru_data, &fru_data_offset);

   fru_data_offset_tmp = fru_data_offset;
   
   /*Position at Serial Number*/
   fru_area = get_fru_area_str(fru_data, &fru_data_offset_tmp);
   if (fru_area == NULL) {
	printf("Bad FRU area data read.\n");
	free(sn);
	free(fru_data);
	return(-1);
   }

   fru_data_offset ++;
   if (verbose) 
      printf("%x: Old board sernum [%s], New board sernum [%s]\n",
		fru_data_offset,fru_area,sn);

   if (strlen(fru_area) != sn_size)
   {
      printf("The length of the serial number in the FRU Board Area is wrong.\n");
      free(sn);
      free(fru_data);
      return(ERR_BAD_LENGTH);
   }
   
   /* Copy the new serial number in the board section saved in memory*/
   memcpy(fru_data + fru_data_offset, sn, sn_size);
   
   /* Calculate Header Checksum */
   checksum = 0;
   for( i = (header.offset.board * 8); i < (((header.offset.board * 8)+board_sec_len) - 2) ; i ++ )
   {
      checksum += fru_data[i];
   }
   checksum = (~checksum) + 1;
   fru_data[(header.offset.board * 8)+board_sec_len - 1] = checksum;

#ifdef TEST
   if (verbose >= 1)
      print_buf(fru_data, fru.size, "FRU BUFFER, New");

   /* Write the new FRU Board section */
   rv = write_fru_data(0, 0, fru_data, fru.size, verbose);
   // if(write_fru_area(intf, &fru, 0, (header.offset.board * 8), (header.offset.board * 8), board_sec_len, fru_data) < 0)
   if ( rv != 0 )
   {
      printf(" Cannot Write FRU, error %d\n",rv);
      free(sn);
      free(fru_data);
      return(rv);
   }
   
   if (fru_data != NULL) { free(fru_data); fru_data = NULL; }
   rv = load_fru(sa,0, FRU_TYPE_BASEBOARD, &fru_data);
   if (rv != 0) 
   {
      printf(" Cannot Read FRU, error %d\n",rv);
      free(sn);
      if (fru_data != NULL) free(fru_data);
      return(rv);
   }
#endif

   /* Set the Product Section */
   prod_sec_len =   (header.offset.multi * 8) - (header.offset.product * 8);
   
   /*Position at Product Manufacturer*/
   fru_data_offset = (header.offset.product * 8) + 3;
   fru_area = get_fru_area_str(fru_data, &fru_data_offset);

   /*Position at Product Name*/
   fru_area = get_fru_area_str(fru_data, &fru_data_offset);
   
   /*Position at Product Part*/
   fru_area = get_fru_area_str(fru_data, &fru_data_offset);
   
   /*Position at Product Version*/
   fru_area = get_fru_area_str(fru_data, &fru_data_offset);
   
   

   fru_data_offset_tmp = fru_data_offset;
   
   /*Position at Serial Number*/
   fru_area = get_fru_area_str(fru_data, &fru_data_offset_tmp);

   fru_data_offset ++;
   if (verbose) 
      printf("%x: Old product sernum [%s], New product sernum [%s]\n",
		fru_data_offset,fru_area,sn);

   if(strlen(fru_area) != sn_size)
   {
      free(sn);
      free(fru_data);
      printf("The length of the serial number in the FRU Product Area is wrong.\n");
      return(ERR_BAD_LENGTH);
   }
   
   /* Copy the new serial number in the product section saved in memory*/
   memcpy(fru_data + fru_data_offset, sn, sn_size);
   
   /* Calculate Header Checksum */
   checksum = 0;
   for( i = (header.offset.product * 8); i < (((header.offset.product * 8)+prod_sec_len) - 2) ; i ++ )
   {
      checksum += fru_data[i];
   }
   checksum = (~checksum) + 1;
   fru_data[(header.offset.product * 8)+prod_sec_len - 1] = checksum;

   if (verbose >= 1)
      print_buf(fru_data, fru.size, "FRU BUFFER, New");
   
   /* Write the new FRU Board section */
   rv = write_fru_data(0, 0, fru_data, fru.size, (char)verbose);
   if ( rv != 0 )
   {
      printf(" Cannot Write FRU, error %d\n",rv);
      /* fall through and free(sn); free(fru_data); return(rv); */
   }   
   
   free(sn);
   free(fru_data);
   return(rv);
}


/* ipmi_fru_set_mfg_date -  Set the Manufacturing Date in FRU
 *
 * @intf:		ipmi interface
 * @id:		fru id
 *
 * returns -1 on error
 * returns 1 if successful
 */
static int
ipmi_kontron_set_mfg_date (void * intf)
{
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   struct fru_info fru;
   struct fru_header header;
   uint8_t msg_data[4];
   uint8_t mfg_date[3];
   
   uint32_t board_sec_len, i;
   uint8_t *fru_data, checksum;
   uint8_t bus, sa, lun, mtype;
   
   ipmi_get_mc(&bus, &sa, &lun, &mtype);
   
   memset(msg_data, 0, 4);
   msg_data[0] = 0xb4;
   msg_data[1] = 0x90;
   msg_data[2] = 0x91;
   msg_data[3] = 0x8b;
   
   memset(&req, 0, sizeof(req));
   req.msg.netfn = 0x3E;
   req.msg.cmd = 0x0E;
   req.msg.data = msg_data;
   req.msg.data_len = 4;
   
   /* Set Lun temporary, necessary for this oem command */
   req.msg.lun = 0x03;
      
   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv < 0) 
   {
      printf("Device not present (No Response)\n");
      return(rv);
   }
   
   if (rv > 0) 
   {
      printf("This option is not implemented for this board\n");
      return(rv);
   }
     
   if(rsp_len != 3)
   {
      printf("Invalid response for the Manufacturing date\n");
      return(ERR_BAD_LENGTH);
   }  
   if(rsp[0] == 0 && rsp[1] == 0 && rsp[2] == 0) 
   {
      printf("Manufacturing date is zero, has been cleared\n");
      return(ERR_BAD_FORMAT);
   }  
   
   memset(mfg_date, 0, 3);
   memcpy(mfg_date, rsp, 3);

   memset(msg_data, 0, 4);
   msg_data[0] = 0;
   
   memset(&req, 0, sizeof(req));
   req.msg.netfn = IPMI_NETFN_STORAGE;
   req.msg.cmd = GET_FRU_INFO;
   req.msg.data = msg_data;
   req.msg.data_len = 1;
   
   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv < 0) {
      printf(" Device not present (No Response)\n");
      return(rv);
   }
   if (rv > 0) {
      printf(" Device not present (%s)\n",decode_cc(0,rv));
      return(rv);
   }

   fru.size = (rsp[1] << 8) | rsp[0];
   fru.access = rsp[2] & 0x1;

   if (fru.size < 1) {
      printf(" Invalid FRU size %d", fru.size);
      return(ERR_BAD_LENGTH);
   }
 
   /*
    * retrieve the FRU header
    */
   msg_data[0] = 0;
   msg_data[1] = 0;
   msg_data[2] = 0;
   msg_data[3] = 8;

   memset(&req, 0, sizeof(req));
   req.msg.netfn = IPMI_NETFN_STORAGE;
   req.msg.cmd = GET_FRU_DATA;
   req.msg.data = msg_data;
   req.msg.data_len = 4;

   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv < 0) 
   {
      printf(" Device not present (No Response)\n");
      return(rv);
   }
   if (rv > 0) 
   {
      printf(" Device not present (%s)\n",decode_cc(0,rv));
      return(rv);
   }

   if (verbose > 1)
      print_buf(rsp, rsp_len, "FRU DATA");

   memcpy(&header, &rsp[1], 8);

   if (header.version != 1) 
   {
      printf(" Unknown FRU header version 0x%02x",
         header.version);
      return(ERR_BAD_FORMAT);
   } 
   
   
   board_sec_len =   (header.offset.product * 8) - (header.offset.board * 8);
   
   rv = load_fru(sa,0, FRU_TYPE_BASEBOARD, &fru_data);
   if (rv != 0) 
#ifdef NOT
   /* do not re-read the same fru buffer */
   fru_data = malloc( fru.size);
   if(fru_data == NULL)
   {
      printf("Out of memory!");
      return(-1);
   }
   
   memset(fru_data, 0, fru.size); 
   rv = read_fru_area(intf ,&fru ,0 ,(header.offset.board * 8) ,board_sec_len , fru_data);
   if(rv < 0)
#endif
   {
      if (fru_data != NULL) free(fru_data);
      return(rv);
   }
   
   /* Copy the new manufacturing date in the board section saved in memory*/
   memcpy(fru_data + (header.offset.board * 8) + 3, mfg_date, 3);
   
   checksum = 0;
   /* Calculate Header Checksum */
   for( i = (header.offset.board * 8); i < (((header.offset.board * 8)+board_sec_len) - 2) ; i ++ )
   {
      checksum += fru_data[i];
   }
   checksum = (~checksum) + 1;

   fru_data[(header.offset.board * 8)+board_sec_len - 1] = checksum;
   
   /* Write the new FRU Board section */
#ifdef NOT
   if(write_fru_area(intf, &fru, 0, (header.offset.board * 8), (header.offset.board * 8), board_sec_len, fru_data) < 0)
#endif
   rv = write_fru_data(0, 0, fru_data, fru.size, (char)verbose);
   if ( rv != 0 )
   {
      printf(" Cannot Write FRU, error %d\n",rv);
      /* fall through and free(fru_data); return(rv); */
   }

   free(fru_data);
   return(rv);
}
#endif

static char *assert_str(uchar val)
{
   char *pstr;
   if ((val & 0x0f) == 0) pstr = "Deasserted";
   else pstr = "Asserted";
   return(pstr);
}

/* 
 * decode_sel_kontron
 * inputs:
 *    evt    = the 16-byte IPMI SEL event
 *    outbuf = points to the output string buffer
 *    outsz  = size of the output buffer
 * outputs: 
 *    rv     = 0 if this event was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    outbuf = will contain the interpreted event text string (if rv==0)
 */
int decode_sel_kontron(uint8_t *evt, char *outbuf, int outsz, char fdesc,
			char fdebug)
{
   int rv = -1;
   uint16_t id;
   uint8_t rectype;
   int oemid;
   uint32_t timestamp;
   char timestr[40]; 
   char mybuf[64]; 
   char oembuf[64]; 
   char *type_str = NULL;
   char *gstr = NULL;
   char *pstr = NULL;
   int sevid;
   ushort genid;
   uchar snum;
   char *p1, *p2;
         
   sevid = SEV_INFO;
   id = evt[0] + (evt[1] << 8);
   /* instead try to decode some events manually */
   rectype = evt[2];
   timestamp = evt[3] + (evt[4] << 8) + (evt[5] << 16) + (evt[6] << 24);
   genid = evt[7] | (evt[8] << 8);
   snum = evt[11];
   if (rectype == 0xc1) {  /* OEM type C1 */
      oemid = evt[7] + (evt[8] << 8) + (evt[9] << 16);
      if (oemid == VENDOR_KONTRON) {
		 fmt_time(timestamp, timestr, sizeof(timestr));
		 type_str = "Kontron";
		 gstr = "BMC ";
		 switch(evt[10]) {
		  case 0x01:
		  default:
		  sprintf(mybuf,"OEM Event %02x %02x %02x %02x %02x %02x",
			evt[10], evt[11], evt[12], evt[13], evt[14], evt[15]);
		  break;
		 }
		 snprintf(outbuf, outsz, "%04x %s %s %s %s %s\n",
                        id,timestr,get_sev_str(sevid), gstr, type_str, mybuf);
		 rv = 0;
      } /*endif kontron*/
   } else if (rectype == 0x02) {
      type_str = "";
      gstr = "BMC ";
      sprintf(mybuf,"%02x [%02x %02x %02x]", evt[12],evt[13],evt[14],evt[15]);
      switch(evt[10]) {  /*sensor type*/
	/* TODO:  0xC0, 0xC2, 0xC6 */
	case 0xCF:	/* Board Reset */
	   type_str = "Board Reset";
	   if (evt[12] == 0x03) { /*event trigger/type = discrete [de]assert */
	      pstr = ""; /*default*/
	      switch(evt[13]) {  /*data1/offset*/
              case 0x01: 
		pstr = "Asserted";
		break;
              case 0xa1:   /*data2,3 bytes have meaning*/
		/* data2: usually 0x01 */
		/* data3: 05, 08, ff */
		switch(evt[14]) { /*data2*/
		case 0x00: p1 = "warm reset"; break;
		case 0x01: p1 = "cold reset"; break;
		case 0x02: p1 = "forced cold"; break;
		case 0x03: p1 = "soft reset"; break;
		case 0x04: p1 = "hard reset"; break;
		case 0x05: p1 = "forced hard"; break;
		default:   p1 = "other"; break;
		}
		switch(evt[15]) { /*data3*/
		case 0x00: p2 = "IPMI watchdog"; break;
		case 0x01: p2 = "IPMI command"; break;
		case 0x02: p2 = "Proc check stop"; break; /*internal*/
		case 0x03: p2 = "Proc reset request"; break; /*internal*/
		case 0x04: p2 = "Reset button"; break;
		case 0x05: p2 = "Power up"; break;
		case 0x06: p2 = "Legacy int watchdog"; break; /*internal*/
		case 0x07: p2 = "Legacy prg watchdog"; break; /*programmable*/
		case 0x08: p2 = "Software initiated"; break;
		case 0x09: p2 = "Setup reset"; break;
		case 0x0a: p2 = "Power cycle"; break;
		default:   p2 = "Other"; break;
		}
		sprintf(oembuf,"%s/%s",p1,p2);
		pstr = oembuf;
		break;
	      default: break;
              }
	      rv = 0;  
	   }
           break;
	case 0x12:	/* System Event */
	   /* snum 38, 6f 00, or ef 00 */
	   /* if (snum == 0x38) */
	   type_str = "System Event";
	   if (evt[12] == 0x6f) pstr = assert_str(1); /*asserted*/
	   else pstr = assert_str(0);  /*0xef means deasserted*/
	   rv = 0;  
           break;
	case 0x24:	/* Platform Alert */
	   /* usu 03 01 */
	   type_str = "Platform Alert";
	   pstr = assert_str(evt[13]); /*data1*/
	   rv = 0;
           break;
	case 0x2B:	/* Version Change */
	   /* 6f c1 02 ff */
	   type_str = "Version Change";
	   if ((evt[13] & 0x80) == 0) pstr = "";
	   else switch(evt[14]) {  /*data2*/
	   case 0x01: pstr = "HW Changed"; break;
	   case 0x02: pstr = "SW Changed"; break;
	   case 0x03: pstr = "HW incompatible"; break;
	   default:   pstr = "Change failed"; break; /*TODO: add detail here*/
	   }
	   rv = 0;  
           break;
	case 0x70:
	   type_str = "OEM Firmware Info 1";
	   pstr = assert_str(evt[13]); /*data1*/
	   rv = 0;  
           break;
	case 0x71:
	   type_str = "OEM Firmware Info 2";
	   pstr = assert_str(evt[13]); /*data1*/
	   rv = 0;  
           break;
	default:  break;
      }
      if (rv == 0) {
	 format_event(id,timestamp, sevid, genid, type_str,
			snum,NULL,pstr,mybuf,outbuf,outsz);
      }
   } 
   return rv;
}  /*end decode_sel_kontron*/

int decode_sensor_kontron(uchar *sdr,uchar *reading,char *pstring, int slen)
{
   int rv = -1;
   uchar stype;
   char *pstr = NULL;
   int rdval;

   if (sdr == NULL || reading == NULL) return(rv);
   if (pstring == NULL || slen == 0) return(rv);
   if (sdr[3] != 0x02) return(rv);  /*skip if not compact sensor*/
   stype = sdr[12];
   rdval = reading[2] | ((reading[3] & 0x7f) << 8);
   /* usually reading3 == 0x80 */
   switch(stype) {
	case 0x08:	/* Power Supply or PwrGood ...  */
	   if (sdr[14] == 0x02) {
	      if (reading[2] & 0x02) pstr = "Failed";
	      else pstr = "OK";
	      rv = 0;
	   }
	   break;
	case 0xC0:	/* IPMI Info-N */
	   if (reading[2] == 0) pstr = "OK";
	   else pstr = "Asserted";
	   rv = 0;
	   break;
	case 0xC2:	/* IniAgent Err */
	   if ((reading[2] & 0x01) == 1) pstr = "OK";
	   else pstr = "Error"; /*could be 0x02*/
	   rv = 0;
	   break;
	case 0xC6:	/* POST Value */
           pstr = (char *)val2str((ushort)rdval,ktc5520_post);
	   if (pstr == NULL) {
              if (rdval >= 0x61 && rdval <= 0x71) pstr = "Error";
              else if (rdval >= 0xDD && rdval <= 0xDE) pstr = "DIMM Debug";
              else pstr = "Other";
	   }
	   rv = 0;
	   break;
	case 0xCF:	/* Board Reset */
	   if ((reading[2] & 0x01) == 1) pstr = "OK";
	   else pstr = "Asserted";
	   rv = 0;
	   break;
	default:
	   break;
   }
   if (rv == 0) strncpy(pstring, pstr, slen);
   return(rv);
}

#ifdef METACOMMAND
int i_kontronoem(int argc, char **argv)
{
   printf("%s ver %s\n", progname,progver);
   return(0);
}
#endif
/* end oem_kontron.c */
