/*
 * ipicmg.c
 *
 * This module handles PICMG extended IPMI commands.
 *
 * Change History:
 *  06/03/2010 ARCress - new, included in source tree
 *  08/15/2011 ARCress - updated with PICMG 2.3
 *
 *----------------------------------------------------------------------
 * Copyright (c) 2010 Kontron. All right reserved
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
 * Neither the name of Kontron, or the names of
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * This software is provided "AS IS," without a warranty of any kind.
 * ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED.
 * THE COPYRIGHT HOLDER AND ITS LICENSORS SHALL NOT BE LIABLE
 * FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING
 * OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.  IN NO EVENT WILL THE
 * COPYRIGHT HOLDER OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR 
 * DATA, OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
 * PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY OF
 * LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *----------------------------------------------------------------------
 */
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "ipicmg.h"

/* Misc PICMG defines */
#define PICMG_EXTENSION_ATCA_MAJOR_VERSION  2
#define PICMG_EXTENSION_AMC0_MAJOR_VERSION  4
#define PICMG_EXTENSION_UTCA_MAJOR_VERSION  5


#define PICMG_EKEY_MODE_QUERY          0
#define PICMG_EKEY_MODE_PRINT_ALL      1
#define PICMG_EKEY_MODE_PRINT_ENABLED  2
#define PICMG_EKEY_MODE_PRINT_DISABLED 3

#define PICMG_EKEY_MAX_CHANNEL          16
#define PICMG_EKEY_MAX_FABRIC_CHANNEL   15
#define PICMG_EKEY_MAX_INTERFACE 3

#define PICMG_EKEY_AMC_MAX_CHANNEL  16
#define PICMG_EKEY_AMC_MAX_DEVICE   15 /* 4 bits */

/* Global data */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil picmg";
#else
static char * progver   = "3.08";
static char * progname  = "ipicmg";
#endif
static char   fdebug    = 0;
static char   fset_mc   = 0;
static uint8_t g_bus  = PUBLIC_BUS;
static uint8_t g_sa   = BMC_SA;
static uint8_t g_lun  = BMC_LUN;
static uint8_t g_addrtype = ADDR_SMI;
static uint8_t g_fruid = 0;
static unsigned char PicmgExtMajorVersion;

/* the LED color capabilities */
static const char* led_color_str[] = { //__attribute__((unused)) = {
   "reserved",
   "BLUE",
   "RED",
   "GREEN",
   "AMBER",
   "ORANGE",
   "WHITE",
   "reserved"
};


static const char* amc_link_type_str[] = { // __attribute__((unused)) = {
   "RESERVED",
   "RESERVED1",
   "PCI EXPRESS",
   "ADVANCED SWITCHING1",
   "ADVANCED SWITCHING2",
   "ETHERNET",
   "RAPIDIO",
   "STORAGE",
};

static const char* amc_link_type_ext_str[][16]= { //  __attribute__((unused))
	/* FRU_PICMGEXT_AMC_LINK_TYPE_RESERVED */
	{
		"", "", "", "", "", "", "", "",   "", "", "", "", "", "", "", ""
	},
	/* FRU_PICMGEXT_AMC_LINK_TYPE_RESERVED1 */
	{
		"", "", "", "", "", "", "", "",   "", "", "", "", "", "", "", ""
	},
	/* FRU_PICMGEXT_AMC_LINK_TYPE_PCI_EXPRESS */
	{
		"Gen 1 - NSSC",
		"Gen 1 - SSC",
		"Gen 2 - NSSC",
		"Gen 2 - SSC",
		"", "", "", "",
		"", "", "", "", 
		"", "", "", ""
	},
	/* FRU_PICMGEXT_AMC_LINK_TYPE_ADVANCED_SWITCHING1 */
	{
		"Gen 1 - NSSC",
		"Gen 1 - SSC",
		"Gen 2 - NSSC",
		"Gen 2 - SSC",
		"", "", "", "",
		"", "", "", "", 
		"", "", "", ""
	},
	/* FRU_PICMGEXT_AMC_LINK_TYPE_ADVANCED_SWITCHING2 */
	{
		"Gen 1 - NSSC",
		"Gen 1 - SSC",
		"Gen 2 - NSSC",
		"Gen 2 - SSC",
		"", "", "", "",
		"", "", "", "", 
		"", "", "", ""
	},
	/* FRU_PICMGEXT_AMC_LINK_TYPE_ETHERNET */
	{
   		"1000BASE-BX (SerDES Gigabit)",
   		"10GBASE-BX410 Gigabit XAUI",
   		"", "", 
   		"", "", "", "",
		"", "", "", "", 
		"", "", "", ""
	},
	/* FRU_PICMGEXT_AMC_LINK_TYPE_RAPIDIO */
	{
   		"1.25 Gbaud transmission rate",
   		"2.5 Gbaud transmission rate",
   		"3.125 Gbaud transmission rate",
   		"", "", "", "", "",
		"", "", "", "", "", "", "", ""
	},
	/* FRU_PICMGEXT_AMC_LINK_TYPE_STORAGE */
	{
   		"Fibre Channel", 
   		"Serial ATA", 
   		"Serial Attached SCSI",
   		"", "", "", "", "",
		"", "", "", "", "", "", "", ""
	}
};

typedef enum picmg_bused_resource_mode {
	PICMG_BUSED_RESOURCE_SUMMARY,
} t_picmg_bused_resource_mode ;


typedef enum picmg_card_type {
	PICMG_CARD_TYPE_CPCI,
	PICMG_CARD_TYPE_ATCA,
	PICMG_CARD_TYPE_AMC,
	PICMG_CARD_TYPE_RESERVED
} t_picmg_card_type ;

/* This is the version of the PICMG Extenstion */
static t_picmg_card_type PicmgCardType = PICMG_CARD_TYPE_RESERVED;

const struct valstr picmg_frucontrol_vals[] = {
	{ 0, "Cold Reset" },
	{ 1, "Warm Reset"  },
	{ 2, "Graceful Reboot" },
	{ 3, "Issue Diagnostic Interrupt" },
	{ 4, "Quiesce" },
	{ 5, NULL },
};

const struct valstr picmg_clk_family_vals[] = {
	{ 0x00, "Unspecified" },
	{ 0x01, "SONET/SDH/PDH" },
	{ 0x02, "Reserved for PCI Express" },
	{ 0x03, "Reserved" }, /* from 03h to C8h */
	{ 0xC9, "Vendor defined clock family" }, /* from C9h to FFh */
	{ 0x00, NULL },
};

const struct oemvalstr picmg_clk_accuracy_vals[] = {
	{ 0x01, 10, "PRS" },
	{ 0x01, 20, "STU" },
	{ 0x01, 30, "ST2" },
	{ 0x01, 40, "TNC" },
	{ 0x01, 50, "ST3E" },
	{ 0x01, 60, "ST3" },
	{ 0x01, 70, "SMC" },
	{ 0x01, 80, "ST4" },
	{ 0x01, 90, "DUS" },
   { 0x02, 0xE0, "PCI Express Generation 2" },
   { 0x02, 0xF0, "PCI Express Generation 1" },
   { 0xffffff, 0x00,  NULL }
};

const struct oemvalstr picmg_clk_resource_vals[] = {
	{ 0x0, 0, "On-Carrier Device 0" },
	{ 0x0, 1, "On-Carrier Device 1" },
	{ 0x1, 1, "AMC Site 1 - A1" },
	{ 0x1, 2, "AMC Site 1 - A2" },
	{ 0x1, 3, "AMC Site 1 - A3" },
	{ 0x1, 4, "AMC Site 1 - A4" },
	{ 0x1, 5, "AMC Site 1 - B1" },
	{ 0x1, 6, "AMC Site 1 - B2" },
	{ 0x1, 7, "AMC Site 1 - B3" },
	{ 0x1, 8, "AMC Site 1 - B4" },
   { 0x2, 0, "ATCA Backplane" },
   { 0xffffff, 0x00,  NULL }
};

const struct oemvalstr picmg_clk_id_vals[] = {
	{ 0x0, 0, "Clock 0" },
	{ 0x0, 1, "Clock 1" },
	{ 0x0, 2, "Clock 2" },
	{ 0x0, 3, "Clock 3" },
	{ 0x0, 4, "Clock 4" },
	{ 0x0, 5, "Clock 5" },
	{ 0x0, 6, "Clock 6" },
	{ 0x0, 7, "Clock 7" },
	{ 0x0, 8, "Clock 8" },
	{ 0x0, 9, "Clock 9" },
	{ 0x0, 10, "Clock 10" },
	{ 0x0, 11, "Clock 11" },
	{ 0x0, 12, "Clock 12" },
	{ 0x0, 13, "Clock 13" },
	{ 0x0, 14, "Clock 14" },
	{ 0x0, 15, "Clock 15" },
	{ 0x1, 1, "TCLKA" },
	{ 0x1, 2, "TCLKB" },
	{ 0x1, 3, "TCLKC" },
	{ 0x1, 4, "TCLKD" },
	{ 0x1, 5, "FLCKA" },
   { 0x2, 1, "CLK1A" },
   { 0x2, 2, "CLK1A" },
   { 0x2, 3, "CLK1A" },
   { 0x2, 4, "CLK1A" },
   { 0x2, 5, "CLK1A" },
   { 0x2, 6, "CLK1A" },
   { 0x2, 7, "CLK1A" },
   { 0x2, 8, "CLK1A" },
   { 0x2, 9, "CLK1A" },
   { 0xffffff, 0x00,  NULL }
};

const struct valstr picmg_busres_id_vals[] = {
   { 0x0, "Metallic Test Bus pair #1" },
   { 0x1, "Metallic Test Bus pair #2" },
   { 0x2, "Synch clock group 1 (CLK1)" },
   { 0x3, "Synch clock group 2 (CLK2)" },
   { 0x4, "Synch clock group 3 (CLK3)" },
	{ 0x5, NULL }
};
const struct valstr picmg_busres_board_cmd_vals[] = {
  { 0x0, "Query" },
  { 0x1, "Release" },
  { 0x2, "Force" },
  { 0x3, "Bus Free" },
  { 0x4, NULL }
};

const struct valstr picmg_busres_shmc_cmd_vals[] = {
  { 0x0, "Request" },
  { 0x1, "Relinquish" },
  { 0x2, "Notify" },
  { 0x3, NULL }
};

const struct oemvalstr picmg_busres_board_status_vals[] = {
 { 0x0, 0x0, "In control" },
 { 0x0, 0x1, "No control" },
 { 0x1, 0x0, "Ack" },
 { 0x1, 0x1, "Refused" },
 { 0x1, 0x2, "No control" },
 { 0x2, 0x0, "Ack" },
 { 0x2, 0x1, "No control" },
 { 0x3, 0x0, "Accept" },
 { 0x3, 0x1, "Not Needed" },
 { 0xffffff, 0x00,  NULL }
};

const struct oemvalstr picmg_busres_shmc_status_vals[] = {
 { 0x0, 0x0, "Grant" },
 { 0x0, 0x1, "Busy" },
 { 0x0, 0x2, "Defer" },
 { 0x0, 0x3, "Deny" },

 { 0x1, 0x0, "Ack" },
 { 0x1, 0x1, "Error" },

 { 0x2, 0x0, "Ack" },
 { 0x2, 0x1, "Error" },
 { 0x2, 0x2, "Deny" },

 { 0xffffff, 0x00,  NULL }
};

void
ipmi_picmg_help (void)
{
	printf(" properties           - get PICMG properties\n");
	printf(" frucontrol           - FRU control\n");
	printf(" addrinfo             - get address information\n");
	printf(" activate             - activate a FRU\n");
	printf(" deactivate           - deactivate a FRU\n");
	printf(" policy get           - get the FRU activation policy\n");
	printf(" policy set           - set the FRU activation policy\n");
	printf(" portstate get        - get port state \n");
	printf(" portstate getdenied  - get all denied[disabled] port description\n");
	printf(" portstate getgranted - get all granted[enabled] port description\n");
	printf(" portstate getall     - get all port state description\n");
	printf(" portstate set        - set port state \n");
	printf(" amcportstate get     - get port state \n");
	printf(" amcportstate set     - set port state \n");
	printf(" led prop             - get led properties\n");
	printf(" led cap              - get led color capabilities\n");
	printf(" led get              - get led state\n");
	printf(" led set              - set led state\n");
	printf(" power get            - get power level info\n");
	printf(" power set            - set power level\n");
	printf(" clk get              - get clk state\n");
	printf(" clk getdenied        - get all(up to 16) denied[disabled] clock descriptions\n");
	printf(" clk getgranted       - get all(up to 16) granted[enabled] clock descriptions\n");
	printf(" clk getall           - get all(up to 16) clock descriptions\n");
	printf(" clk set              - set clk state\n");
	printf(" busres summary       - display brief bused resource status info \n");
}

struct sAmcAddrMap {
	unsigned char ipmbLAddr;
	char*         amcBayId;
	unsigned char siteNum;
} amcAddrMap[] = {
	{0xFF, "reserved", 0},
	{0x72, "A1"      , 1},
	{0x74, "A2"      , 2},
	{0x76, "A3"      , 3},
	{0x78, "A4"      , 4},
	{0x7A, "B1"      , 5},
	{0x7C, "B2"      , 6},
	{0x7E, "B3"      , 7},
	{0x80, "B4"      , 8},
	{0x82, "reserved", 0},
	{0x84, "reserved", 0},
	{0x86, "reserved", 0},
	{0x88, "reserved", 0},
};

int
ipmi_picmg_getaddr(void * intf, int  argc, char ** argv)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;
	unsigned char msg_data[5];

	memset(&req, 0, sizeof(req));
	req.msg.netfn = IPMI_NETFN_PICMG;
	req.msg.cmd = PICMG_GET_ADDRESS_INFO_CMD;
	req.msg.data = msg_data;
	req.msg.data_len = 2;
	msg_data[0] = 0;   /* picmg identifier */
	msg_data[1] = 0;   /* default fru id */

	if(argc > 0) {
		msg_data[1] = (uint8_t)strtoul(argv[0], NULL,0); /* FRU ID */
	}

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		printf("Error getting address information CC: 0x%02x\n", rv);
		return rv;
	}

	printf("Hardware Address : 0x%02x\n", rsp[1]);
	printf("IPMB-0 Address   : 0x%02x\n", rsp[2]);
	printf("FRU ID           : 0x%02x\n", rsp[4]);
	printf("Site ID          : 0x%02x\n", rsp[5]);

	printf("Site Type        : ");
	switch (rsp[6]) {
	case PICMG_ATCA_BOARD:
		printf("ATCA board\n");
		break;
	case PICMG_POWER_ENTRY:
		printf("Power Entry Module\n");
		break;
	case PICMG_SHELF_FRU:
		printf("Shelf FRU\n");
		break;
	case PICMG_DEDICATED_SHMC:
		printf("Dedicated Shelf Manager\n");
		break;
	case PICMG_FAN_TRAY:
		printf("Fan Tray\n");
		break;
	case PICMG_FAN_FILTER_TRAY:
		printf("Fan Filter Tray\n");
		break;
	case PICMG_ALARM:
		printf("Alarm module\n");
		break;
	case PICMG_AMC:
		printf("AMC");
		printf("  -> IPMB-L Address: 0x%02x\n", amcAddrMap[rsp[5]].ipmbLAddr);
		break;
	case PICMG_PMC:
		printf("PMC\n");
		break;
	 case PICMG_RTM:
		printf("RTM\n");
		break;
	default:
		if (rsp[6] >= 0xc0 && rsp[6] <= 0xcf) {
			printf("OEM\n");
		} else {
			printf("unknown\n");
		}
	}

	return 0;
}

int
ipmi_picmg_properties(void * intf, int show )
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	unsigned char PicmgExtMajorVersion;
	struct ipmi_rq req;
	unsigned char msg_data;

	memset(&req, 0, sizeof(req));
	req.msg.netfn    = IPMI_NETFN_PICMG;
	req.msg.cmd      = PICMG_GET_PICMG_PROPERTIES_CMD;
	req.msg.data     = &msg_data;
	req.msg.data_len = 1;
	msg_data = 0;

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		printf("Error %d getting picmg properties\n", rv);
		return rv;
	}

	if( show )
	{
		printf("PICMG identifier	: 0x%02x\n", rsp[0]);
		printf("PICMG Ext. Version 	: %i.%i\n", rsp[1]&0x0f,
							 (rsp[1]&0xf0) >> 4);
		printf("Max FRU Device ID	: 0x%02x\n", rsp[2]);
		printf("FRU Device ID		: 0x%02x\n", rsp[3]);
	}

   /* We cache the major extension version ...
      to know how to format some commands */
	PicmgExtMajorVersion = rsp[1]&0x0f;

	if( PicmgExtMajorVersion == PICMG_CPCI_MAJOR_VERSION  ) { 
		PicmgCardType = PICMG_CARD_TYPE_CPCI;
   }
	else if(  PicmgExtMajorVersion == PICMG_ATCA_MAJOR_VERSION) {
		PicmgCardType = PICMG_CARD_TYPE_ATCA;
   }
	else if(  PicmgExtMajorVersion == PICMG_AMC_MAJOR_VERSION) {
		PicmgCardType = PICMG_CARD_TYPE_AMC;
   }
    
	return 0;
}



#define PICMG_FRU_DEACTIVATE	(unsigned char) 0x00
#define PICMG_FRU_ACTIVATE	(unsigned char) 0x01

int
ipmi_picmg_fru_activation(void * intf, int  argc, char ** argv, unsigned char state)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	struct picmg_set_fru_activation_cmd cmd;

	memset(&req, 0, sizeof(req));
	req.msg.netfn    = IPMI_NETFN_PICMG;
	req.msg.cmd      = PICMG_FRU_ACTIVATION_CMD;
	req.msg.data     = (unsigned char*) &cmd;
	req.msg.data_len = 3;

	cmd.picmg_id  = 0;				/* PICMG identifier */
	cmd.fru_id    = atob(argv[0]);			/* FRU ID	*/
	cmd.fru_state = state;

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		printf("Error %d on activation/deactivation of FRU\n",rv);
		return rv;
	}
	if (rsp[0] != 0x00) {
		printf("Error, invalid response\n");
	}

	return 0;
}


int
ipmi_picmg_fru_activation_policy_get(void * intf, int  argc, char ** argv)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[4];

	memset(&req, 0, sizeof(req));
	req.msg.netfn    = IPMI_NETFN_PICMG;
	req.msg.cmd      = PICMG_GET_FRU_POLICY_CMD;
	req.msg.data     = msg_data;
	req.msg.data_len = 2;

	msg_data[0] = 0;								/* PICMG identifier */
	msg_data[1] = atob(argv[0]);	/* FRU ID			*/


	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	printf(" %s\n", ((rsp[1] & 0x01) == 0x01) ?
	                           "activation locked" : "activation not locked");
	printf(" %s\n", ((rsp[1] & 0x02) == 0x02) ?
	                            "deactivation locked" : "deactivation not locked");

	return 0;
}

int
ipmi_picmg_fru_activation_policy_set(void * intf, int  argc, char ** argv)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[4];

	memset(&req, 0, sizeof(req));
	req.msg.netfn    = IPMI_NETFN_PICMG;
	req.msg.cmd      = PICMG_SET_FRU_POLICY_CMD;
	req.msg.data     = msg_data;
	req.msg.data_len = 4;

	msg_data[0] = 0;		            /* PICMG identifier */
	msg_data[1] = atob(argv[0]);	      /* FRU ID */
	msg_data[2] = atob(argv[1])& 0x03; /* FRU act policy mask  */
	msg_data[3] = atob(argv[2])& 0x03; /* FRU act policy set bits */

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	return 0;
}

#define PICMG_MAX_LINK_PER_CHANNEL 4

int
ipmi_picmg_portstate_get(void * intf, int interfc, uchar channel, int mode)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;
	unsigned char msg_data[4];
	struct fru_picmgext_link_desc* d; /* descriptor pointer for rec. data */

	memset(&req, 0, sizeof(req));

	req.msg.netfn    = IPMI_NETFN_PICMG;
	req.msg.cmd      = PICMG_GET_PORT_STATE_CMD;
	req.msg.data     = msg_data;
	req.msg.data_len = 2;

	msg_data[0] = 0x00;			/* PICMG identifier */
	msg_data[1] = (interfc & 0x3)<<6;	/* interface      */
	msg_data[1] |= (channel & 0x3F);	/* channel number */

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else if( mode == PICMG_EKEY_MODE_QUERY )
			printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	if (rsp_len >= 6) {
		int index;
		/* add support for more than one link per channel */
		for(index=0;index<PICMG_MAX_LINK_PER_CHANNEL;index++){
			if( rsp_len > (1+ (index*5))){
				d = (struct fru_picmgext_link_desc *) &(rsp[1 + (index*5)]);

				if
				(
					mode == PICMG_EKEY_MODE_PRINT_ALL
					||
					mode == PICMG_EKEY_MODE_QUERY
					||
					(
						mode == PICMG_EKEY_MODE_PRINT_ENABLED
						&&
						rsp[5 + (index*5) ] == 0x01
					)
					||
					(
						mode == PICMG_EKEY_MODE_PRINT_DISABLED
						&&
						rsp[5 + (index*5) ] == 0x00
					)
				)
				{
					printf("      Link Grouping ID:     0x%02x\n", d->grouping);
					printf("      Link Type Extension:  0x%02x\n", d->ext);
					printf("      Link Type:            0x%02x  ", d->type);
					if (d->type == 0 || d->type == 0xff)
					{
						printf("Reserved %d\n",d->type);
					}
					else if (d->type >= 0x06 && d->type <= 0xef)
					{
						printf("Reserved %d\n",d->type);
					}
					else if (d->type >= 0xf0 && d->type <= 0xfe)
					{
						printf("OEM GUID Definition %d\n",d->type);
					}
					else
					{
						switch (d->type)
						{
							case FRU_PICMGEXT_LINK_TYPE_BASE:
								printf("PICMG 3.0 Base Interface 10/100/1000\n");
							break;
							case FRU_PICMGEXT_LINK_TYPE_FABRIC_ETHERNET:
								printf("PICMG 3.1 Ethernet Fabric Interface\n");
							break;
							case FRU_PICMGEXT_LINK_TYPE_FABRIC_INFINIBAND:
								printf("PICMG 3.2 Infiniband Fabric Interface\n");
							break;
							case FRU_PICMGEXT_LINK_TYPE_FABRIC_STAR:
								printf("PICMG 3.3 Star Fabric Interface\n");
							break;
							case  FRU_PICMGEXT_LINK_TYPE_PCIE:
								printf("PCI Express Fabric Interface\n");
							break;
							default:
							printf("Invalid\n");
							break;
						}
					}
					printf("      Link Designator: \n");
					printf("        Port Flag:          0x%02x\n", d->desig_port);
					printf("        Interface:          0x%02x - ", d->desig_if);
					switch (d->desig_if)
					{
						case FRU_PICMGEXT_DESIGN_IF_BASE:
							printf("Base Interface\n");
						break;
						case FRU_PICMGEXT_DESIGN_IF_FABRIC:
							printf("Fabric Interface\n");
						break;
						case FRU_PICMGEXT_DESIGN_IF_UPDATE_CHANNEL:
							printf("Update Channel\n");
						break;
						case FRU_PICMGEXT_DESIGN_IF_RESERVED:
							printf("Reserved\n");
						break;
						default:
							printf("Invalid");
						break;
					}
					printf("        Channel Number:     0x%02x\n", d->desig_channel);
					printf("      STATE:                %s\n",
							( rsp[5 +(index*5)] == 0x01) ?"enabled":"disabled");
					printf("\n");
				}
			}
		}
	}
	else
	{
		printf("Unexpected answer len %d, can't print result\n",rsp_len);
	}

	return 0;
}


int
ipmi_picmg_portstate_set(void * intf, int interfc, uchar channel,
		 uchar port, int type, int typeext, int group, int enable)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;
	unsigned char msg_data[6];
	// struct fru_picmgext_link_desc* d;

	memset(&req, 0, sizeof(req));

	req.msg.netfn    = IPMI_NETFN_PICMG;
	req.msg.cmd      = PICMG_SET_PORT_STATE_CMD;
	req.msg.data     = msg_data;
	req.msg.data_len = 6;

	msg_data[0] = 0x00;												/* PICMG identifier */
	msg_data[1] = (channel & 0x3f) | ((interfc & 3) << 6);
	msg_data[2] = (port & 0xf) | ((type & 0xf) << 4);
	msg_data[3] = ((type >> 4) & 0xf) | ((typeext & 0xf) << 4);
	msg_data[4] = group & 0xff;
	msg_data[5]	  = (unsigned char) (enable & 0x01);		/* en/dis */

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	return 0;
}



/* AMC.0 commands */

#define PICMG_AMC_MAX_LINK_PER_CHANNEL 4

int
ipmi_picmg_amc_portstate_get(void * intf,int device,uchar channel,
								 int mode)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[4];

	struct fru_picmgext_amc_link_info* d; /* descriptor pointer for rec. data */

	memset(&req, 0, sizeof(req));

	req.msg.netfn	  = IPMI_NETFN_PICMG;
	req.msg.cmd		  = PICMG_AMC_GET_PORT_STATE_CMD;
	req.msg.data	  = msg_data;

	/* FIXME : add check for AMC or carrier device */
	if(device == -1 || PicmgCardType != PICMG_CARD_TYPE_ATCA ){
		req.msg.data_len = 2;	/* for amc only channel */
	}else{
		req.msg.data_len = 3;	/* for carrier channel and device */
	}

	msg_data[0] = 0x00;	/* PICMG identifier */
	msg_data[1] = channel ;
	msg_data[2] = (uchar)device ;


	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else if ( mode == PICMG_EKEY_MODE_QUERY )
			printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	if (rsp_len >= 5) {
		int index;

		/* add support for more than one link per channel */
		for(index=0;index<PICMG_AMC_MAX_LINK_PER_CHANNEL;index++){

			if( rsp_len > (1+ (index*4))){
				unsigned char type;
				unsigned char ext;
				unsigned char grouping;
				unsigned char port;
				unsigned char enabled;
				d = (struct fru_picmgext_amc_link_info *)&(rsp[1 + (index*4)]);


				/* Removed endianness check here, probably not required
					as we dont use bitfields  */
				port = d->linkInfo[0] & 0x0F;
				type = ((d->linkInfo[0] & 0xF0) >> 4 )|(d->linkInfo[1] & 0x0F );
				ext  = ((d->linkInfo[1] & 0xF0) >> 4 );
				grouping = d->linkInfo[2];


				enabled =  rsp[4 + (index*4) ];

				if
				(
					mode == PICMG_EKEY_MODE_PRINT_ALL
					||
					mode == PICMG_EKEY_MODE_QUERY
					||
					(
						mode == PICMG_EKEY_MODE_PRINT_ENABLED
						&&
						enabled == 0x01
					)
					||
					(
						mode == PICMG_EKEY_MODE_PRINT_DISABLED
						&&
						enabled	== 0x00
					)
				)
				{
					if(device == -1 || PicmgCardType != PICMG_CARD_TYPE_ATCA ){
						printf("   Link device :         AMC\n");
					}else{
                  printf("   Link device :         0x%02x\n", device );
					}

					printf("   Link Grouping ID:     0x%02x\n", grouping);

					if (type == 0 || type == 1 ||type == 0xff)
					{
						printf("   Link Type Extension:  0x%02x\n", ext);
						printf("   Link Type:            Reserved\n");
					}
					else if (type >= 0xf0 && type <= 0xfe)
					{
						printf("   Link Type Extension:  0x%02x\n", ext);
						printf("   Link Type:            OEM GUID Definition\n");
					}
					else
					{
						if (type <= FRU_PICMGEXT_AMC_LINK_TYPE_STORAGE )
						{
							printf("   Link Type Extension:  %s\n",
                                      amc_link_type_ext_str[type][ext]);
							printf("   Link Type:            %s\n",
                                      amc_link_type_str[type]);
						}
						else{
							printf("   Link Type Extension:  0x%02x\n", ext);
							printf("   Link Type:            undefined\n");
						}
					}
					printf("   Link Designator: \n");
					printf("      Channel Number:    0x%02x\n", channel);
					printf("      Port Flag:         0x%02x\n", port );
					printf("   STATE:                %s\n",
                              ( enabled == 0x01 )?"enabled":"disabled");
					printf("\n");
				}
			}
		}
	}
	else
	{
		printf("ipmi_picmg_amc_portstate_get: "
				"Unexpected answer, can't print result\n");
	}

	return 0;
}


int
ipmi_picmg_amc_portstate_set(void * intf, uchar channel, uchar port,
		  int type, int typeext, int group, int enable, int device)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq	 req;
	unsigned char	 msg_data[7];

	memset(&req, 0, sizeof(req));

	req.msg.netfn	  = IPMI_NETFN_PICMG;
	req.msg.cmd	  = PICMG_AMC_SET_PORT_STATE_CMD;
	req.msg.data	  = msg_data;

	msg_data[0]	 = 0x00;	 /* PICMG identifier*/
	msg_data[1]	 = channel;	 /* channel id */
	msg_data[2]	 = port & 0xF;	 /* port flags */
	msg_data[2] |= (type & 0x0F)<<4;	 /* type	 */
	msg_data[3]	 = (type & 0xF0)>>4;	 /* type */
	msg_data[3] |= (typeext & 0x0F)<<4;	 /* extension */
	msg_data[4]	 = (group & 0xFF);	 /* group */
	msg_data[5]	 = (enable & 0x01);	 /* state */
	req.msg.data_len = 6;

	/* device id - only for carrier needed */
	if (device >= 0) {
		msg_data[6]	 = (uchar)device;
		req.msg.data_len = 7;
	}

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	return 0;
}


int
ipmi_picmg_get_led_properties(void * intf, int  argc, char ** argv)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[6];

	memset(&req, 0, sizeof(req));

	req.msg.netfn = IPMI_NETFN_PICMG;
	req.msg.cmd	  = PICMG_GET_FRU_LED_PROPERTIES_CMD;
	req.msg.data  = msg_data;
	req.msg.data_len = 2;

	msg_data[0] = 0x00;		/* PICMG identifier */
	msg_data[1] = atob(argv[0]);	/* FRU-ID */

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	printf("General Status LED Properties:  0x%2x\n\r", rsp[1] );
	printf("App. Specific  LED Count:       0x%2x\n\r", rsp[2] );

	return 0;
}

int
ipmi_picmg_get_led_capabilities(void * intf, int  argc, char ** argv)
{
	int i;
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[6];

	memset(&req, 0, sizeof(req));

	req.msg.netfn = IPMI_NETFN_PICMG;
	req.msg.cmd	  = PICMG_GET_LED_COLOR_CAPABILITIES_CMD;
	req.msg.data  = msg_data;
	req.msg.data_len = 3;

	msg_data[0] = 0x00;		/* PICMG identifier */
	msg_data[1] = atob(argv[0]);	/* FRU-ID */
	msg_data[2] = atob(argv[1]);	/* LED-ID */


	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	printf("LED Color Capabilities: %02x", rsp[1] );
	for ( i=0 ; i<8 ; i++ ) {
		if ( rsp[1] & (0x01 << i) ) {
			printf("%s, ", led_color_str[ i ]);
		}
	}
	printf("\n\r");

	printf("Default LED Color in\n\r");
	printf("      LOCAL control:  %s\n\r", led_color_str[ rsp[2] ] );
	printf("      OVERRIDE state: %s\n\r", led_color_str[ rsp[3] ] );

	return 0;
}

int
ipmi_picmg_get_led_state(void * intf, int  argc, char ** argv)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[6];

	memset(&req, 0, sizeof(req));

	req.msg.netfn = IPMI_NETFN_PICMG;
	req.msg.cmd	  = PICMG_GET_FRU_LED_STATE_CMD;
	req.msg.data  = msg_data;
	req.msg.data_len = 3;

	msg_data[0] = 0x00;		/* PICMG identifier */
	msg_data[1] = atob(argv[0]);	/* FRU-ID */
	msg_data[2] = atob(argv[1]);	/* LED-ID */


	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	printf("LED states:						  %x	", rsp[1] );
	if (rsp[1] == 0x1)
		printf("[LOCAL CONTROL]\n\r");
	else if (rsp[1] == 0x2)
		printf("[OVERRIDE]\n\r");
	else if (rsp[1] == 0x4)
		printf("[LAMPTEST]\n\r");
	else
		printf("\n\r");

	printf("  Local Control function:     %x  ", rsp[2] );
	if (rsp[2] == 0x0)
		printf("[OFF]\n\r");
	else if (rsp[2] == 0xff)
		printf("[ON]\n\r");
	else
		printf("[BLINKING]\n\r");

	printf("  Local Control On-Duration:  %x\n\r", rsp[3] );
	printf("  Local Control Color:        %x  [%s]\n\r", rsp[4], led_color_str[ rsp[4] ]);

	/* override state or lamp test */
	if (rsp[1] == 0x02) {
		printf("  Override function:     %x  ", rsp[5] );
		if (rsp[2] == 0x0)
			printf("[OFF]\n\r");
		else if (rsp[2] == 0xff)
			printf("[ON]\n\r");
		else
			printf("[BLINKING]\n\r");

		printf("  Override On-Duration:  %x\n\r", rsp[6] );
		printf("  Override Color:        %x  [%s]\n\r", rsp[7], led_color_str[ rsp[7] ]);

	}else if (rsp[1] == 0x06) {
		printf("  Override function:     %x  ", rsp[5] );
		if (rsp[2] == 0x0)
			printf("[OFF]\n\r");
		else if (rsp[2] == 0xff)
			printf("[ON]\n\r");
		else
			printf("[BLINKING]\n\r");
		printf("  Override On-Duration:  %x\n\r", rsp[6] );
		printf("  Override Color:        %x  [%s]\n\r", rsp[7], led_color_str[ rsp[7] ]);
		printf("  Lamp test duration:    %x\n\r", rsp[8] );
	}

	return 0;
}

int
ipmi_picmg_set_led_state(void * intf, int  argc, char ** argv)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[6];

	memset(&req, 0, sizeof(req));

	req.msg.netfn = IPMI_NETFN_PICMG;
	req.msg.cmd	  = PICMG_SET_FRU_LED_STATE_CMD;
	req.msg.data  = msg_data;
	req.msg.data_len = 6;

	msg_data[0] = 0x00;		/* PICMG identifier */
	msg_data[1] = atob(argv[0]);  /* FRU-ID		  */
	msg_data[2] = atob(argv[1]);  /* LED-ID		  */
	msg_data[3] = atob(argv[2]);  /* LED function	  */
	msg_data[4] = atob(argv[3]);  /* LED on duration   */
	msg_data[5] = atob(argv[4]);  /* LED color	  */

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}


	return 0;
}

int
ipmi_picmg_get_power_level(void * intf, int  argc, char ** argv)
{
	int i;
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[6];

	memset(&req, 0, sizeof(req));

	req.msg.netfn = IPMI_NETFN_PICMG;
	req.msg.cmd	  = PICMG_GET_POWER_LEVEL_CMD;
	req.msg.data  = msg_data;
	req.msg.data_len = 3;

	msg_data[0] = 0x00;		/* PICMG identifier */
	msg_data[1] = atob(argv[0]);	/* FRU-ID	  */
	msg_data[2] = atob(argv[1]);	/* Power type	  */


	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	printf("Dynamic Power Configuration: %s\n", (rsp[1]&0x80)==0x80?"enabled":"disabled" );
	printf("Actual Power Level:          %i\n", (rsp[1] & 0xf));
	printf("Delay to stable Power:       %i\n", rsp[2]);
	printf("Power Multiplier:            %i\n", rsp[3]);


	for ( i = 1; i+3 < rsp_len ; i++ ) {
		printf("   Power Draw %i:            %i\n", i, (rsp[i+3]) * rsp[3] / 10);
	}
	return 0;
}

int
ipmi_picmg_set_power_level(void * intf, int  argc, char ** argv)
{
	// int i;
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[6];

	memset(&req, 0, sizeof(req));

	req.msg.netfn = IPMI_NETFN_PICMG;
	req.msg.cmd	  = PICMG_SET_POWER_LEVEL_CMD;
	req.msg.data  = msg_data;
	req.msg.data_len = 4;

	msg_data[0] = 0x00;			/* PICMG identifier	 */
	msg_data[1] = atob(argv[0]);		/* FRU-ID		 */
	msg_data[2] = atob(argv[1]);		/* power level		 */
	msg_data[3] = atob(argv[2]);		/* present to desired */

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("no response\n");
		else printf("returned Completion Code 0x%02x\n", rv);
		return rv;
	}

	return 0;
}

int
ipmi_picmg_bused_resource(void * intf, t_picmg_bused_resource_mode mode)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[6];
	memset(&req, 0, sizeof(req));

   switch ( mode ) {
      case PICMG_BUSED_RESOURCE_SUMMARY:
      {
         t_picmg_busres_resource_id resource;
         t_picmg_busres_board_cmd_types cmd =PICMG_BUSRES_BOARD_CMD_QUERY;

         req.msg.netfn	  = IPMI_NETFN_PICMG;
         req.msg.cmd	     = PICMG_BUSED_RESOURCE_CMD;
         req.msg.data	  = msg_data;
         req.msg.data_len = 3;

         /* IF BOARD
            query for all resources
         */
         for( resource=PICMG_BUSRES_METAL_TEST_BUS_1;resource<=PICMG_BUSRES_SYNC_CLOCK_GROUP_3;resource+=(t_picmg_busres_resource_id)1 ) {
            msg_data[0] = 0x00;					/* PICMG identifier */
            msg_data[1] = (unsigned char) cmd;
            msg_data[2] = (unsigned char) resource;
	    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	    if (rv) {
		if (rv < 0) printf("bused resource control: no response\n");
		else printf("bused resource control: returned Completion Code 0x%02x\n",rv);
                return rv;
            } else {
               printf("Resource 0x%02x '%-26s' : 0x%02x [%s] \n" , 
                       resource, val2str(resource,picmg_busres_id_vals),
                       rsp[1], oemval2str(cmd,rsp[1],
                      picmg_busres_board_status_vals));
            }
         }
      }
      break;
      default:  rv = -1;
      break;
   }

   return rv;
}

int
ipmi_picmg_fru_control(void * intf, int  argc, char ** argv)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[6];

	memset(&req, 0, sizeof(req));

	req.msg.netfn	  = IPMI_NETFN_PICMG;
	req.msg.cmd	  = PICMG_FRU_CONTROL_CMD;
	req.msg.data	  = msg_data;
	req.msg.data_len = 3;

	msg_data[0] = 0x00;			/* PICMG identifier */
	msg_data[1] = atob(argv[0]);		/* FRU-ID	  */
	msg_data[2] = atob(argv[1]);		/* control option  */

	printf("FRU Device Id: %d FRU Control Option: %s\n\r", msg_data[1],  \
				val2str( msg_data[2], picmg_frucontrol_vals));

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
		if (rv < 0) printf("frucontrol: no response\n");
		else printf("frucontrol: returned Completion Code 0x%02x\n",rv);
		return rv;
	} else {
		printf("frucontrol: ok\n");
	}

	return 0;
}


int
ipmi_picmg_clk_get(void * intf,  int clk_id,int clk_res,int mode)
{
	// int i;
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char enabled;
	unsigned char direction;

	unsigned char msg_data[6];

	memset(&req, 0, sizeof(req));

	req.msg.netfn = IPMI_NETFN_PICMG;
	req.msg.cmd   = PICMG_AMC_GET_CLK_STATE_CMD;
	req.msg.data  = msg_data;

	msg_data[0] = 0x00;									/* PICMG identifier	 */
	msg_data[1] = clk_id;

	if(clk_res == -1 || PicmgCardType != PICMG_CARD_TYPE_ATCA ){
		req.msg.data_len = 2;	/* for amc only channel */
	}else{
		req.msg.data_len = 3;	/* for carrier channel and device */
		msg_data[2] = clk_res;
	}

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
	   if (rv < 0) printf("no response\n");
	   else printf("returned Completion Code 0x%02x\n", rv);
	   return rv;
	}

	{
		enabled  = (rsp[1]&0x8)!=0;
		direction = (rsp[1]&0x4)!=0;

		if
		( 
			mode == PICMG_EKEY_MODE_QUERY
 			||
 			mode == PICMG_EKEY_MODE_PRINT_ALL
 			||
 			(
 				mode == PICMG_EKEY_MODE_PRINT_DISABLED
 				&&
 				enabled == 0
 			)
 			||
 			(
 				mode == PICMG_EKEY_MODE_PRINT_ENABLED
 				&&
 				enabled == 1
         )	
		) {
			if( PicmgCardType != PICMG_CARD_TYPE_AMC ) {
				printf("CLK resource id   : %3d [ %s ]\n", clk_res ,
					oemval2str( ((clk_res>>6)&0x03), (clk_res&0x0F),
														picmg_clk_resource_vals));				
			} else {
				printf("CLK resource id   : N/A [ AMC Module ]\n");
				clk_res = 0x40; /* Set */
			} 
         printf("CLK id            : %3d [ %s ]\n", clk_id,
					oemval2str( ((clk_res>>6)&0x03), clk_id ,
														picmg_clk_id_vals));				


			printf("CLK setting       : 0x%02x\n", rsp[1]);
			printf(" - state:     %s\n", (enabled)?"enabled":"disabled");
			printf(" - direction: %s\n", (direction)?"Source":"Receiver");
			printf(" - PLL ctrl:  0x%x\n", rsp[1]&0x3);

		   if(enabled){
		      unsigned long freq = 0;
		      freq = (  rsp[5] <<  0
			      | rsp[6] <<  8
			      | rsp[7] << 16
			      | rsp[8] << 24 );
		      printf("  - Index:  %3d\n", rsp[2]);
		      printf("  - Family: %3d [ %s ]\n", rsp[3],
			val2str( rsp[3], picmg_clk_family_vals));
		      printf("  - AccLVL: %3d [ %s ]\n", rsp[4],
			oemval2str(rsp[3],rsp[4],picmg_clk_accuracy_vals));
		      printf("  - Freq:   %lu\n", freq);
		   }
		}
	}
	return 0;
}


int
ipmi_picmg_clk_set(void * intf, int  argc, char ** argv)
{
	// int i;
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	unsigned char msg_data[11];
	unsigned long freq=0;

	memset(&req, 0, sizeof(req));

	req.msg.netfn = IPMI_NETFN_PICMG;
	req.msg.cmd	  = PICMG_AMC_SET_CLK_STATE_CMD;
	req.msg.data  = msg_data;

	msg_data[0] = 0x00;				/* PICMG identifier	 */
	msg_data[1] = (uchar)strtoul(argv[0], NULL,0);	/* clk id	 */
	msg_data[2] = (uchar)strtoul(argv[1], NULL,0);	/* clk index	 */
	msg_data[3] = (uchar)strtoul(argv[2], NULL,0);	/* setting	 */
	msg_data[4] = (uchar)strtoul(argv[3], NULL,0);	/* family	 */
	msg_data[5] = (uchar)strtoul(argv[4], NULL,0);	/* acc		 */

	freq = strtoul(argv[5], NULL,0);
	msg_data[6] = (uchar)((freq >> 0)& 0xFF); 	/* freq		 */
	msg_data[7] = (uchar)((freq >> 8)& 0xFF); 	/* freq		 */
	msg_data[8] = (uchar)((freq >>16)& 0xFF); 	/* freq		 */
	msg_data[9] = (uchar)((freq >>24)& 0xFF); 	/* freq		 */

	req.msg.data_len = 10;
   if( PicmgCardType == PICMG_CARD_TYPE_ATCA  )
   {
      if( argc > 7)
      {
         req.msg.data_len = 11;
         msg_data[10] = (uchar)strtoul(argv[6], NULL,0);	/* resource id			 */
      }
      else
      {
         printf("missing resource id for atca board\n");
         return -1;
      }
   }

#if 1
printf("## ID:      %d\n", msg_data[1]);
printf("## index:   %d\n", msg_data[2]);
printf("## setting: 0x%02x\n", msg_data[3]);
printf("## family:  %d\n", msg_data[4]);
printf("## acc:     %d\n", msg_data[5]);
printf("## freq:    %lu\n", freq );
printf("## res:     %d\n", msg_data[10]);
#endif

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
	   if (rv < 0) printf("no response\n");
	   else printf("returned Completion Code 0x%02x\n", rv);
	   return rv;
	}

	return 0;
}


#ifdef METACOMMAND
int i_picmg(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
#endif
{
	int rc = 0;
	int showProperties = 0;
	void *intf = NULL;
	int c, i;
	char *s1;

	printf("%s ver %s\n", progname,progver);

	  while ( (c = getopt( argc, argv,"m:i:T:V:J:EYF:P:N:R:U:Z:x?")) != EOF)
	switch (c) {
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
	    case 'i':     /*specify a fru id*/
	             if (strncmp(optarg,"0x",2) == 0) g_fruid = htoi(&optarg[2]);
	              else g_fruid = htoi(optarg);
	              printf("Using FRU ID 0x%02x\n",g_fruid);
	              break;
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
	    case 'Z':    /* set local MC address */
	          parse_lan_options(c,optarg,fdebug);
	          break;
	    case '?':
	        ipmi_picmg_help();
	        return ERR_USAGE;
	          break;
	}
	  for (i = 0; i < optind; i++) { argv++; argc--; }

	/* Get PICMG properties is called to obtain version information */
	if (argc !=0 && !strncmp(argv[0], "properties", 10)) {
		showProperties =1;
	}

	if (argc == 0 || (!strncmp(argv[0], "help", 4))) {
		ipmi_picmg_help();
		return ERR_USAGE;
	}

	rc = ipmi_picmg_properties(intf,showProperties);
	if (rc < 0) {  /*cannot contact MC, so exit*/
	        goto do_exit;
	}

	/* address info command */
	else if (!strncmp(argv[0], "addrinfo", 8)) {
		rc = ipmi_picmg_getaddr(intf, argc-1, &argv[1]);
	}
	else if (!strncmp(argv[0], "busres", 6)) {
		if (argc > 1) {
			if (!strncmp(argv[1], "summary", 7)) {
				ipmi_picmg_bused_resource(intf, PICMG_BUSED_RESOURCE_SUMMARY );
			}
		} else {
				printf("usage: busres summary\n");
      }
	}
	/* fru control command */
	else if (!strncmp(argv[0], "frucontrol", 10)) {
		if (argc > 2) {
			rc = ipmi_picmg_fru_control(intf, argc-1, &(argv[1]));
		}
		else {
			printf("usage: frucontrol <FRU-ID> <OPTION>\n");
			printf("   OPTION:\n");
			printf("      0      - Cold Reset\n");
			printf("      1      - Warm Reset\n");
			printf("      2      - Graceful Reboot\n");
			printf("      3      - Issue Diagnostic Interrupt\n");
			printf("      4      - Quiesce [AMC only]\n");
			printf("      5-255  - Reserved\n");

			rc = -1;
		}

	}

	/* fru activation command */
	else if (!strncmp(argv[0], "activate", 8)) {
		if (argc > 1) {
			rc = ipmi_picmg_fru_activation(intf, argc-1, &(argv[1]), PICMG_FRU_ACTIVATE);
		}
		else {
			printf("specify the FRU to activate\n");
			rc = -1;
		}
	}

	/* fru deactivation command */
	else if (!strncmp(argv[0], "deactivate", 10)) {
		if (argc > 1) {
			rc = ipmi_picmg_fru_activation(intf, argc-1, &(argv[1]), PICMG_FRU_DEACTIVATE);
		}else {
			printf("specify the FRU to deactivate\n");
			rc = -1;
		}
	}

	/* activation policy command */
	else if (!strncmp(argv[0], "policy", 6)) {
		if (argc > 1) {
			if (!strncmp(argv[1], "get", 3)) {
				if (argc > 2) {
					rc = ipmi_picmg_fru_activation_policy_get(intf, argc-1, &(argv[2]));
				} else {
					printf("usage: get <fruid>\n");
				}
			} else if (!strncmp(argv[1], "set", 3)) {
				if (argc > 4) {
					rc = ipmi_picmg_fru_activation_policy_set(intf, argc-1, &(argv[2]));
				} else {
					printf("usage: set <fruid> <lockmask> <lock>\n");
					printf("    lockmask:  [1] affect the deactivation locked bit\n");
					printf("               [0] affect the activation locked bit\n");
					printf("    lock:      [1] set/clear deactivation locked\n");
					printf("               [0] set/clear locked \n");
				}
			}
			else {
				printf("specify fru\n");
				rc = -1;
			}
		} else {
			printf("wrong parameters\n");
			rc = -1;
		}
	}

	/* portstate command */
	else if (!strncmp(argv[0], "portstate", 9)) {

		if (fdebug) printf("PICMG: portstate API");

		if (argc > 1) {
			if (!strncmp(argv[1], "get", 3)) {

				int iface;
				int channel;

				if (fdebug) printf("PICMG: get");

				if(!strncmp(argv[1], "getall", 6)) {
					for(iface=0;iface<=PICMG_EKEY_MAX_INTERFACE;iface++) {
						for(channel=1;channel<=PICMG_EKEY_MAX_CHANNEL;channel++) {
							if(!(( iface == FRU_PICMGEXT_DESIGN_IF_FABRIC ) &&
							      ( channel > PICMG_EKEY_MAX_FABRIC_CHANNEL ) ))
							{
								rc = ipmi_picmg_portstate_get(intf,iface,(uchar)channel,
								        PICMG_EKEY_MODE_PRINT_ALL);
							}
						}
					}
				}
				else if(!strncmp(argv[1], "getgranted", 10)) {
					for(iface=0;iface<=PICMG_EKEY_MAX_INTERFACE;iface++) {
						for(channel=1;channel<=PICMG_EKEY_MAX_CHANNEL;channel++) {
							rc = ipmi_picmg_portstate_get(intf,iface,(uchar)channel,
							            PICMG_EKEY_MODE_PRINT_ENABLED);
						}
					}
				}
				else if(!strncmp(argv[1], "getdenied", 9)){
					for(iface=0;iface<=PICMG_EKEY_MAX_INTERFACE;iface++) {
						for(channel=1;channel<=PICMG_EKEY_MAX_CHANNEL;channel++) {
							rc = ipmi_picmg_portstate_get(intf,iface,(uchar)channel,
							           PICMG_EKEY_MODE_PRINT_DISABLED);
						}
					}
				}
				else if (argc > 3){
					iface     = atob(argv[2]);
					channel   = atob(argv[3]);
					if (fdebug) printf("PICMG: requesting interface %d",iface);
					if (fdebug) printf("PICMG: requesting channel %d",channel);

					rc = ipmi_picmg_portstate_get(intf,iface,(uchar)channel,
					            PICMG_EKEY_MODE_QUERY );
				}
				else {
					printf("<intf> <chn>|getall|getgranted|getdenied\n");
				}
			}
			else if (!strncmp(argv[1], "set", 3)) {
					if (argc == 9) {
						int interfc = strtoul(argv[2], NULL, 0);
						int channel   = strtoul(argv[3], NULL, 0);
						int port      = strtoul(argv[4], NULL, 0);
						int type      = strtoul(argv[5], NULL, 0);
						int typeext   = strtoul(argv[6], NULL, 0);
						int group     = strtoul(argv[7], NULL, 0);
						int enable    = strtoul(argv[8], NULL, 0);

						if (fdebug) printf("PICMG: interface %d",interfc);
						if (fdebug) printf("PICMG: channel %d",channel);
						if (fdebug) printf("PICMG: port %d",port);
						if (fdebug) printf("PICMG: type %d",type);
						if (fdebug) printf("PICMG: typeext %d",typeext);
						if (fdebug) printf("PICMG: group %d",group);
						if (fdebug) printf("PICMG: enable %d",enable);

						rc = ipmi_picmg_portstate_set(intf, interfc,
						    (uchar)channel, (uchar)port, type, typeext  ,group ,enable);
					}
					else {
						printf("<intf> <chn> <port> <type> <ext> <group> <1|0>\n");
						rc = -1;
					}
			}
		}
		else {
			printf("<set>|<getall>|<getgranted>|<getdenied>\n");
			rc = -1;
		}
	}
	/* amc portstate command */
	else if (!strncmp(argv[0], "amcportstate", 12)) {

		if (fdebug) printf("PICMG: amcportstate API");

		if (argc > 1) {
			if (!strncmp(argv[1], "get", 3)){
				int channel;
				int device;

				if (fdebug) printf("PICMG: get");

				if(!strncmp(argv[1], "getall", 6)){
					int maxDevice = PICMG_EKEY_AMC_MAX_DEVICE;
					if( PicmgCardType != PICMG_CARD_TYPE_ATCA ){
						maxDevice = 0;
					}
					for(device=0;device<=maxDevice;device++){
						for(channel=0;channel<=PICMG_EKEY_AMC_MAX_CHANNEL;channel++){
							rc = ipmi_picmg_amc_portstate_get(intf,device,(uchar)channel,
																	PICMG_EKEY_MODE_PRINT_ALL);
						}
					}
				}
				else if(!strncmp(argv[1], "getgranted", 10)){
					int maxDevice = PICMG_EKEY_AMC_MAX_DEVICE;
					if( PicmgCardType != PICMG_CARD_TYPE_ATCA ){
						maxDevice = 0;
					}
					for(device=0;device<=maxDevice;device++){
						for(channel=0;channel<=PICMG_EKEY_AMC_MAX_CHANNEL;channel++){
							rc = ipmi_picmg_amc_portstate_get(intf,(uchar)device,(uchar)channel,
																  PICMG_EKEY_MODE_PRINT_ENABLED);
						}
					}
				}
				else if(!strncmp(argv[1], "getdenied", 9)){
					int maxDevice = PICMG_EKEY_AMC_MAX_DEVICE;
					if( PicmgCardType != PICMG_CARD_TYPE_ATCA ){
						maxDevice = 0;
					}
					for(device=0;device<=maxDevice;device++){
						for(channel=0;channel<=PICMG_EKEY_AMC_MAX_CHANNEL;channel++){
							rc = ipmi_picmg_amc_portstate_get(intf,(uchar)device,(uchar)channel,
                                                 PICMG_EKEY_MODE_PRINT_DISABLED);
						}
					}
				}
				else if (argc > 2){
					channel     = atoi(argv[2]);
					if (argc > 3){
					device      = atoi(argv[3]);
					}else{
					   device = -1;
				    }
					if (fdebug) printf("PICMG: requesting device %d",device);
					if (fdebug) printf("PICMG: requesting channel %d",channel);

					rc = ipmi_picmg_amc_portstate_get(intf,(uchar)device,(uchar)channel,
                                             PICMG_EKEY_MODE_QUERY );
				}
				else {
					printf("<chn> <device>|getall|getgranted|getdenied\n");
				}
			}
			else if (!strncmp(argv[1], "set", 3)) {
				if (argc > 7) {
					int channel  = atoi(argv[2]);
					int port     = atoi(argv[3]);
					int type     = atoi(argv[4]);
					int typeext  = atoi(argv[5]);
					int group    = atoi(argv[6]);
					int enable   = atoi(argv[7]);
					int device   = -1;
					if(argc > 8){
						device   = atoi(argv[8]);
					}

					if (fdebug) printf("PICMG: channel %d",channel);
					if (fdebug) printf("PICMG: portflags %d",port);
					if (fdebug) printf("PICMG: type %d",type);
					if (fdebug) printf("PICMG: typeext %d",typeext);
					if (fdebug) printf("PICMG: group %d",group);
					if (fdebug) printf("PICMG: enable %d",enable);
					if (fdebug) printf("PICMG: device %d",device);

					rc = ipmi_picmg_amc_portstate_set(intf, (uchar)channel, (uchar)port, type,
                                               typeext, group, enable, device);
				}
				else {
				printf("<chn> <portflags> <type> <ext> <group> <1|0> [<device>]\n");
					rc = -1;
				}
			}
		}
		else {
			printf("<set>|<get>|<getall>|<getgranted>|<getdenied>\n");
			rc = -1;
		}
	}
	/* ATCA led commands */
	else if (!strncmp(argv[0], "led", 3)) {
		if (argc > 1) {
			if (!strncmp(argv[1], "prop", 4)) {
				if (argc > 2) {
					rc = ipmi_picmg_get_led_properties(intf, argc-1, &(argv[2]));
				}
				else {
					printf("led prop <FRU-ID>\n");
				}
			}
			else if (!strncmp(argv[1], "cap", 3)) {
				if (argc > 3) {
					rc = ipmi_picmg_get_led_capabilities(intf, argc-1, &(argv[2]));
				}
				else {
					printf("led cap <FRU-ID> <LED-ID>\n");
				}
			}
			else if (!strncmp(argv[1], "get", 3)) {
				if (argc > 3) {
					rc = ipmi_picmg_get_led_state(intf, argc-1, &(argv[2]));
				}
				else {
					printf("led get <FRU-ID> <LED-ID>\n");
				}
			}
			else if (!strncmp(argv[1], "set", 3)) {
				if (argc > 6) {
					rc = ipmi_picmg_set_led_state(intf, argc-1, &(argv[2]));
				}
				else {
					printf("led set <FRU-ID> <LED-ID> <function> <duration> <color>\n");
					printf("   <FRU-ID>\n");
					printf("   <LED-ID>    0:         Blue LED\n");
					printf("               1:         LED 1\n");
					printf("               2:         LED 2\n");
					printf("               3:         LED 3\n");
					printf("               0x04-0xFE: OEM defined\n");
					printf("               0xFF:      All LEDs under management control\n");
					printf("   <function>  0:       LED OFF override\n");
					printf("               1 - 250: LED blinking override (off duration)\n");
					printf("               251:     LED Lamp Test\n");
					printf("               252:     LED restore to local control\n");
					printf("               255:     LED ON override\n");
					printf("   <duration>  1 - 127: LED Lamp Test / on duration\n");
					printf("   <color>     0:   reserved\n");
					printf("               1:   BLUE\n");
					printf("               2:   RED\n");
					printf("               3:   GREEN\n");
					printf("               4:   AMBER\n");
					printf("               5:   ORANGE\n");
					printf("               6:   WHITE\n");
					printf("               7:   reserved\n");
					printf("               0xE: do not change\n");
					printf("               0xF: use default color\n");
				}
			}
			else {
				printf("prop | cap | get | set\n");
			}
		}
	}
	/* power commands */
	else if (!strncmp(argv[0], "power", 5)) {
		if (argc > 1) {
			if (!strncmp(argv[1], "get", 3)) {
				if (argc > 3) {
					rc = ipmi_picmg_get_power_level(intf, argc-1, &(argv[2]));
				}
				else {
					printf("power get <FRU-ID> <type>\n");
					printf("   <type>   0 : steady state powert draw levels\n");
					printf("            1 : desired steady state draw levels\n");
					printf("            2 : early power draw levels\n");
					printf("            3 : desired early levels\n");

					rc = -1;
				}
			}
			else if (!strncmp(argv[1], "set", 3)) {
				if (argc > 4) {
					rc = ipmi_picmg_set_power_level(intf, argc-1, &(argv[2]));
				}
				else {
					printf("power set <FRU-ID> <level> <present-desired>\n");
					printf("   <level>  0 :        Power Off\n");
					printf("            0x1-0x14 : Power level\n");
					printf("            0xFF :     do not change\n");
					printf("\n");
					printf("   <present-desired> 0: do not change present levels\n");
					printf("                     1: copy desired to present level\n");

					rc = -1;
				}
			}
			else {
				printf("<set>|<get>\n");
				rc = -1;
			}
		}
		else {
			printf("<set>|<get>\n");
			rc = -1;
		}
	}/* clk commands*/
	else if (!strncmp(argv[0], "clk", 3)) {
		if (argc > 1) {
			if (!strncmp(argv[1], "get", 3)) {
				int clk_id;
				int clk_res = -1;            
				int max_res = 15;

				if( PicmgCardType == PICMG_CARD_TYPE_AMC ) {
					max_res = 0;
				}

				if(!strncmp(argv[1], "getall", 6)) {
					if(fdebug) printf("Getting all clock state\n");
					for(clk_res=0;clk_res<=max_res;clk_res++) {
						for(clk_id=0;clk_id<=15;clk_id++) {
								rc = ipmi_picmg_clk_get(intf,clk_id,clk_res,
								        PICMG_EKEY_MODE_PRINT_ALL);
						}
					}
				}
				else if(!strncmp(argv[1], "getdenied", 6)) {
					if( fdebug ) { printf("Getting disabled clocks\n") ;}	
					for(clk_res=0;clk_res<=max_res;clk_res++) {
						for(clk_id=0;clk_id<=15;clk_id++) {
								rc = ipmi_picmg_clk_get(intf,clk_id,clk_res,
								        PICMG_EKEY_MODE_PRINT_DISABLED);
						}
					}
				}
				else if(!strncmp(argv[1], "getgranted", 6)) {
					if( fdebug ) { printf("Getting enabled clocks\n") ;}	
					for(clk_res=0;clk_res<=max_res;clk_res++) {
						for(clk_id=0;clk_id<=15;clk_id++) {
								rc = ipmi_picmg_clk_get(intf,clk_id,clk_res,
								        PICMG_EKEY_MODE_PRINT_ENABLED);
						}
					}
				}
				else if (argc > 2) {
					clk_id = atoi(argv[2]);
					if (argc > 3) {
						clk_res = atoi(argv[3]);
					}

					rc = ipmi_picmg_clk_get(intf, clk_id, clk_res,
							PICMG_EKEY_MODE_QUERY );
				}
				else {
					printf("clk get ");
					printf("<CLK-ID> [<DEV-ID>] |getall|getgranted|getdenied\n");
					rc = -1;
				}
			}
			else if (!strncmp(argv[1], "set", 3)) {
				if (argc > 7) {
					rc = ipmi_picmg_clk_set(intf, argc-1, &(argv[2]));
				}
				else {
					printf("clk set <CLK-ID> <index> <setting> <family> <acc-lvl> <freq> [<DEV-ID>] \n");

					rc = -1;
				}
			}
			else {
				printf("<set>|<get>|<getall>|<getgranted>|<getdenied>\n");
				rc = -1;
			}
		}
		else {
			printf("<set>|<get>|<getall>|<getgranted>|<getdenied>\n");
			rc = -1;
		}
	}

	else if(showProperties == 0 ){

		ipmi_picmg_help();
		rc = ERR_USAGE;
	}

do_exit:
	ipmi_close_();
	return rc;
} /*end i_picmg*/

/* end ipicmg.c */

