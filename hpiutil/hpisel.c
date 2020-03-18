/*
 * hpisel.c
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2003-2005 Intel Corporation.
 *
 * 04/28/03 Andy Cress - created
 * 04/30/03 Andy Cress  v0.6 first good run with common use cases
 * 05/06/03 Andy Cress  v0.7 added -c option to clear it
 * 05/29/03 Andy Cress  v0.8 fixed pstr warnings
 * 06/13/03 Andy Cress  v0.9 fixed strcpy bug, 
 *                          workaround for SensorEvent.data3
 * 06/19/03 Andy Cress  0.91 added low SEL free space warning
 * 06/25/03 Andy Cress  v1.0 rework event data logic 
 * 07/23/03 Andy Cress  workaround for OpenHPI BUGGY stuff
 * 11/12/03 Andy Cress  v1.1 check for CAPABILITY_SEL
 * 03/03/04 Andy Cress  v1.2 use better free space logic now
 * 10/14/04 Andy Cress  v1.3 added HPI_A/HPI_B logic
 * 02/11/05 Andy Cress  v1.4 added decode routines from showsel.c
 * 03/30/05 Dave Howell v1.5 added support to enable displaying domain 
 *                           event log for HPI_B
 * 04/07/05 Andy Cress  v1.6 fix domain resourceid & HPI_A/B flags,
 *                           add logic to clear domain event log.
 * 
 * Note that hpiutil 1.0.0 did not return all event data fields, so event
 * types other than 'user' did not have all bytes filled in as they
 * would have from IPMI alone.  A patch to the Intel HPI library in
 * version 1.0.1 resolved this.
 */
/*M*
Copyright (c) 2003-2005, Intel Corporation
All rights reserved.

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
 *M*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include "SaHpi.h"

#ifdef HPI_A
/* HPI A (1.0) spec */
char progver[] = "1.6 HPI-A";
#else
/* HPI B name changes */
#define SaHpiSelEntryT    SaHpiEventLogEntryT
#define SaHpiSelEntryIdT  SaHpiEventLogEntryIdT
#define SaHpiSelInfoT     SaHpiEventLogInfoT
#define SAHPI_CAPABILITY_SEL SAHPI_CAPABILITY_EVENT_LOG  /*0x00000004*/
#define SAHPI_EC_USER     SAHPI_EC_SENSOR_SPECIFIC       /*0x7E*/
char progver[] = "1.6 HPI-B";
#endif
int fdebug  = 0;
int fclear  = 0;
int fdomain = 0;

#ifdef HPI_A
#define NEVTYPES  5
char *evtypes[NEVTYPES] = {"sensor","hotswap","watchdog","oem   ","user  "};
#else
#define NEVTYPES  9
char *evtypes[NEVTYPES] = {"resource","domain","sensor","sens_enable",
			  "hotswap","watchdog","HPI sw", "oem   ","user  "};
#endif

#define NECODES  27
struct { int code; char *str;
} ecodes[NECODES] = { 
 {  0,    "Success" },
 { -1001, "HPI unspecified error" },
 { -1002, "HPI unsupported function" },
 { -1003, "HPI busy" },
 { -1004, "HPI request invalid" },
 { -1005, "HPI command invalid" },
 { -1006, "HPI timeout" },
 { -1007, "HPI out of space" },
 { -1008, "HPI data truncated" },
#ifdef HPI_A
 { -1009, "HPI data length invalid" },
#else
 { -1009, "HPI invalid parameter" },
#endif
 { -1010, "HPI data exceeds limits" },
 { -1011, "HPI invalid params" },
 { -1012, "HPI invalid data" },
 { -1013, "HPI not present" },
 { -1014, "HPI invalid data field" },
 { -1015, "HPI invalid sensor command" },
 { -1016, "HPI no response" },
 { -1017, "HPI duplicate request" },
 { -1018, "HPI updating" },
 { -1019, "HPI initializing" },
 { -1020, "HPI unknown error" },
 { -1021, "HPI invalid session" },
 { -1022, "HPI invalid domain" },
 { -1023, "HPI invalid resource id" },
 { -1024, "HPI invalid request" },
 { -1025, "HPI entity not present" },
 { -1026, "HPI uninitialized" }
};
char def_estr[15] = "HPI error %d   ";

#define NUM_EC  14
struct { int val; char *str;
} eventcats[NUM_EC] = {
{ /*0x00*/ SAHPI_EC_UNSPECIFIED,  "unspecified"},
{ /*0x01*/ SAHPI_EC_THRESHOLD, 	  "Threshold"},
{ /*0x02*/ SAHPI_EC_USAGE, 	  "Usage    "},
{ /*0x03*/ SAHPI_EC_STATE, 	  "State    "},
{ /*0x04*/ SAHPI_EC_PRED_FAIL, 	  "Predictive"},
{ /*0x05*/ SAHPI_EC_LIMIT, 	  "Limit    "},
{ /*0x06*/ SAHPI_EC_PERFORMANCE,  "Performnc"},
{ /*0x07*/ SAHPI_EC_SEVERITY, 	  "Severity "},
{ /*0x08*/ SAHPI_EC_PRESENCE, 	  "DevPresen"},
{ /*0x09*/ SAHPI_EC_ENABLE, 	  "DevEnable"},
{ /*0x0a*/ SAHPI_EC_AVAILABILITY, "Availabil"},
{ /*0x0b*/ SAHPI_EC_REDUNDANCY,   "Redundanc"},
{ /*0x7e*/ SAHPI_EC_USER,         "SensorSpc"},  /*UserDefin*/
{ /*0x7f*/ SAHPI_EC_GENERIC,      "OemDefin " }};

#define NUM_ES  6
struct { int val; char *str; } eventstates[NUM_ES] = {
{ SAHPI_ES_LOWER_MINOR , "lo-min" },
{ SAHPI_ES_LOWER_MAJOR , "lo-maj" },
{ SAHPI_ES_LOWER_CRIT  , "lo-crt" },
{ SAHPI_ES_UPPER_MINOR , "hi-min" },
{ SAHPI_ES_UPPER_MAJOR , "hi-maj" },
{ SAHPI_ES_UPPER_CRIT  , "hi-crt" } };

#define uchar   unsigned char
#define ushort  unsigned short
#define uint    unsigned int
#define ulong   unsigned long
typedef struct
{
        short              record_id;
        uchar              record_type;
        uint               timestamp;
        short              generator_id;    /*slave_addr/channel*/
        uchar              evm_rev;         //event message revision
        uchar              sensor_type;
        uchar              sensor_number;
        uchar              event_trigger;
        uchar              event_data1;
        uchar              event_data2;
        uchar              event_data3;
}       SEL_RECORD;

/* sensor_types: See IPMI 1.5 Table 36-3, IPMI 2.0 Table 42-3 */
#define NSTYPES   0x2D
static char *sensor_types[NSTYPES] = {  
/* 00h */ "reserved",
/* 01h */ "Temperature",
/* 02h */ "Voltage",
/* 03h */ "Current",
/* 04h */ "Fan",
/* 05h */ "Platform Chassis Intrusion",
/* 06h */ "Security Violation",
/* 07h */ "Processor",
/* 08h */ "Power Supply",
/* 09h */ "Power Unit",
/* 0Ah */ "Cooling Device",
/* 0Bh */ "FRU Sensor",
/* 0Ch */ "Memory",
/* 0Dh */ "Drive Slot",
/* 0Eh */ "POST Memory Resize",
/* 0Fh */ "System Firmware",    /*incl POST code errors*/
/* 10h */ "SEL Disabled",
/* 11h */ "Watchdog 1",
/* 12h */ "System Event",          /* offset 0,1,2 */
/* 13h */ "Critical Interrupt",        /* offset 0,1,2 */
/* 14h */ "Button",                /* offset 0,1,2 */
/* 15h */ "Board",
/* 16h */ "Microcontroller",
/* 17h */ "Add-in Card",
/* 18h */ "Chassis",
/* 19h */ "Chip Set",
/* 1Ah */ "Other FRU",
/* 1Bh */ "Cable/Interconnect",
/* 1Ch */ "Terminator",
/* 1Dh */ "System Boot Initiated",
/* 1Eh */ "Boot Error",
/* 1Fh */ "OS Boot",
/* 20h */ "OS Critical Stop",
/* 21h */ "Slot/Connector",
/* 22h */ "ACPI Power State",
/* 23h */ "Watchdog 2",
/* 24h */ "Platform Alert",
/* 25h */ "Entity Presence",
/* 26h */ "Monitor ASIC",
/* 27h */ "LAN",
/* 28h */ "Management Subsystem Health",
/* 29h */ "Battery",
/* 2Ah */ "Session Audit",
/* 2Bh */ "Version Change",
/* 2Ch */ "FRU State"
};

#define NFWERRS  15
static struct {    /* See Table 36-3, type 0Fh, offset 00h */
  int code; char *msg; 
  } fwerrs[NFWERRS] = {
 { 0x00, "Unspecified"},
 { 0x01, "No system memory"},
 { 0x02, "No usable memory"},
 { 0x03, "Unrecovered Hard Disk"},
 { 0x04, "Unrecovered System Board"},
 { 0x05, "Unrecovered Diskette"},
 { 0x06, "Unrecovered Hard Disk Ctlr"},
 { 0x07, "Unrecovered PS2 USB"},
 { 0x08, "Boot media not found"},
 { 0x09, "Unrecovered video controller"},
 { 0x0A, "No video device"},
 { 0x0B, "Firmware ROM corruption"},
 { 0x0C, "CPU voltage mismatch"},
 { 0x0D, "CPU speed mismatch"},
 { 0x0E, "Reserved" }
};

#define NFWSTAT  27
static struct {    /* See Table 36-3, type 0Fh, offset 01h & 02h */
  int code; char *msg; 
  } fwstat[NFWSTAT] = {
 { 0x00, "Unspecified"},
 { 0x01, "Memory init"},
 { 0x02, "Hard disk init"},
 { 0x03, "Secondary processor"},
 { 0x04, "User authentication"},
 { 0x05, "User-init sys setup"},
 { 0x06, "USB configuration"},
 { 0x07, "PCI configuration"},
 { 0x08, "Option ROM init"},
 { 0x09, "Video init"},
 { 0x0a, "Cache init"},
 { 0x0b, "SM Bus init"},
 { 0x0c, "Keyboard init"},
 { 0x0d, "Mgt controller"},
 { 0x0e, "Docking attach"},
 { 0x0f, "Enabling docking"},
 { 0x10, "Docking eject"},
 { 0x11, "Disabling docking"},
 { 0x12, "OS wake-up"},
 { 0x13, "Starting OS boot"},
 { 0x14, "Baseboard init"},
 { 0x15, "reserved"},
 { 0x16, "Floppy init"},
 { 0x17, "Keyboard test"},
 { 0x18, "Mouse test"},
 { 0x19, "Primary processor"},
 { 0x1A, "Reserved"}
};

#define NGDESC   7
struct {
 ushort g_id;
 char desc[10];
} gen_desc[NGDESC] = {  /*empirical, format defined in IPMI 1.5 Table 23-5 */
 { 0x0000, "IPMB"},
 { 0x0001, "EFI "},
 { 0x0003, "BIOS"},
 { 0x0020, "BMC "},
 { 0x0021, "SMI "},
 { 0x0028, "CHAS"},    /* Chassis Bridge Controller */
/*{0x0031, "SMI/IPMB"}, * ia64/mBMC POST errors   (rqSa/rqLun)  */
/*{0x0033, "MemCtlr"},  * ia64 memory controller? (rqSa/rqLun) */
 { 0x00c0, "HSC "}     /* SAF-TE Hot-Swap Controller*/
};

#define NCRITS  10
char * crit_int_str[NCRITS] = {  /* Critical Interrupt descriptions */
 /*00*/ "FP NMI  ",  /* Front Panel NMI */
 /*01*/ "Bus Timout",
 /*02*/ "IOch NMI ", /* IO channel check NMI */
 /*03*/ "Soft NMI ",
 /*04*/ "PCI PERR ",
 /*05*/ "PCI SERR ",
 /*06*/ "EISA Timout",
 /*07*/ "Bus Warn ",  /* Bus Correctable Error */
 /*08*/ "Bus Error",  /* Bus Uncorrectable Error */
 /*09*/ "Fatal NMI" };

#define NSLOTC  9
char * slot_str[NSLOTC] = {  /* Slot/Connector descriptions */
 /*00*/ "Fault   ", 
 /*01*/ "Identify",
 /*02*/ "Inserted",
 /*03*/ "InsReady",
 /*04*/ "RemReady",
 /*05*/ "PowerOff",
 /*06*/ "RemRequest",
 /*07*/ "Interlock",
 /*08*/ "Disabled" }; 

#define NSDESC   47
struct {
 ushort genid;  /*generator id: BIOS, BMC, etc. (slave_addr/channel)*/
 uchar s_typ;   /*1=temp,2=voltage,4=fan,... */
 uchar s_num;
 uchar evtrg;   /*event trigger/type, see IPMI 1.5 table 36.1 & 36.2 */
 uchar data1;
 uchar data2;
 uchar data3;
 char desc[40];
} sens_desc[NSDESC] = {
{0xffff,0x05, 0x05, 0x6f, 0x44, 0xff, 0xff, "LAN unplugged"},   /*chassis*/
{0xffff,0x05, 0x05, 0xef, 0x44, 0xff, 0xff, "LAN restored "},   /*chassis*/
{0xffff,0x06, 0xff, 0xff, 0x45, 0xff, 0xff, "Password"},        /*security*/
{0xffff,0x07, 0xff, 0xff, 0x41, 0xff, 0xff, "Thermal trip"},    /*processor*/
{0xffff,0x08, 0xff, 0x6f, 0x40, 0xff, 0xff, "Presence detected"},/*PowerSupply*/
{0xffff,0x08, 0xff, 0x6f, 0x41, 0xff, 0xff, "Failure detected"},/*power supply*/
{0xffff,0x08, 0xff, 0x6f, 0x42, 0xff, 0xff, "Predictive failure"},
{0xffff,0x08, 0xff, 0x6f, 0x43, 0xff, 0xff, "AC Lost"},
{0xffff,0x08, 0xff, 0xef, 0x40, 0xff, 0xff, "not present"},     /*power supply*/
{0xffff,0x08, 0xff, 0xef, 0x41, 0xff, 0xff, "is OK  "},         /*power supply*/
{0xffff,0x08, 0xff, 0xef, 0x42, 0xff, 0xff, "Predictive OK"},
{0xffff,0x08, 0xff, 0xef, 0x43, 0xff, 0xff, "AC Regained"},     /*power supply*/
{0xffff,0x09, 0xff, 0xff, 0x40, 0xff, 0xff, "Redundancy OK  "}, /*power unit*/
{0xffff,0x09, 0xff, 0xff, 0x41, 0xff, 0xff, "Redundancy Lost"},
{0xffff,0x09, 0xff, 0xff, 0x44, 0xff, 0xff, "AC Lost"},
{0xffff,0x09, 0xff, 0xff, 0x46, 0xff, 0xff, "Failure detected"},
{0xffff,0x09, 0xff, 0xff, 0x47, 0xff, 0xff, "Predictive failure"},/*power unit*/
{0xffff,0x0c, 0xff, 0xff, 0x00, 0xff, 0xff, "Correctable ECC"},  /*memory*/
{0xffff,0x0f, 0x06, 0xff, 0xff, 0xff, 0xff, "POST Code"},
{0xffff,0x10, 0x09, 0xff, 0x42, 0x0f, 0xff, "Log Cleared"},
{0xffff,0x10, 0x09, 0xff, 0xc0, 0x03, 0xff, "ECC Memory Errors"},
     /*  Often see these 3 Boot records with reboot:
      * 12 83 6f 05 00 ff = System/SEL Time Sync 1 (going down)
      * 12 83 6f 05 80 ff = System/SEL Time Sync 2 (coming up)
      * 12 83 6f 01 ff ff = OEM System Boot Event  (Completed POST) */
{0xffff,0x12, 0x83, 0xff, 0x05, 0x00, 0xff, "Boot: ClockSync 1"},   
{0xffff,0x12, 0x83, 0xff, 0x05, 0x80, 0xff, "Boot: ClockSync 2"},   
{0xffff,0x12, 0x83, 0xff, 0x01, 0xff, 0xff, "OEM System Booted"},   
{0xffff,0x12, 0x08, 0xff, 0x01, 0xff, 0xff, "OEM System Booted"},    /*ia64*/
{0xffff,0x12, 0x00, 0x6f, 0xff, 0xff, 0xff, "PEF Action"},           /*ia64*/
{0xffff,0x12, 0x83, 0x6f, 0x80, 0xff, 0xff, "System Reconfigured"},  /*BMC*/
{0x00c0,0x0d, 0xff, 0x08, 0x00, 0xff, 0xff, "Device Removed"},    /*HSC*/
{0x00c0,0x0d, 0xff, 0x08, 0x01, 0xff, 0xff, "Device Inserted"},   /*HSC*/
{0xffff,0x14, 0xff, 0xff, 0x42, 0xff, 0xff, "Reset Button pressed"},
{0xffff,0x14, 0xff, 0xff, 0x40, 0xff, 0xff, "Power Button pressed"},
{0xffff,0x14, 0xff, 0xff, 0x01, 0xff, 0xff, "ID Button pressed"},
{0xffff,0x23, 0xff, 0xff, 0x40, 0xff, 0xff, "Expired, no action"},/*watchdog2*/
{0xffff,0x23, 0xff, 0xff, 0x41, 0xff, 0xff, "Hard Reset action"}, /*watchdog2*/
{0xffff,0x23, 0xff, 0xff, 0x42, 0xff, 0xff, "Power down action"}, /*watchdog2*/
{0xffff,0x23, 0xff, 0xff, 0x43, 0xff, 0xff, "Power cycle action"},/*watchdog2*/
{0xffff,0x23, 0xff, 0xff, 0x48, 0xff, 0xff, "Timer interrupt"},   /*watchdog2*/
{0xffff,0xf3, 0x85, 0xff, 0x41, 0xff, 0xff, "State is now OK"},
{0xffff,0x20, 0x00, 0xff, 0xff, 0xff, 0xff, "OS Kernel Panic"},
{0xffff,0xff, 0xff, 0x01, 0x57, 0xff, 0xff, "Hi Noncrit thresh"},
{0xffff,0xff, 0xff, 0x01, 0x59, 0xff, 0xff, "Hi Crit thresh"},
{0xffff,0xff, 0xff, 0x01, 0x50, 0xff, 0xff, "Lo Noncrit thresh"},
{0xffff,0xff, 0xff, 0x01, 0x52, 0xff, 0xff, "Lo Crit thresh"},
{0xffff,0xff, 0xff, 0x81, 0x57, 0xff, 0xff, "HiN thresh OK now"},
{0xffff,0xff, 0xff, 0x81, 0x59, 0xff, 0xff, "HiC thresh OK now"},
{0xffff,0xff, 0xff, 0x81, 0x50, 0xff, 0xff, "LoN thresh OK now"},
{0xffff,0xff, 0xff, 0x81, 0x52, 0xff, 0xff, "LoC thresh OK now"}
	/*Thresholds apply to sensor types 1=temp, 2=voltage, 4=fan */
	/*Note that last 2 bytes usu show actual & threshold values*/
	/*evtrg: 0x01=thresh, 0x81=restored */
};

/*------------------------------------------------------------------------ 
 * get_misc_desc
 * Uses the sens_desc array to decode misc entries not otherwise handled.
 * Called by decode_sel_entry
 *------------------------------------------------------------------------*/
char * get_misc_desc(uchar type, uchar num, ushort genid, uchar trig,
		 uchar data1, uchar data2, uchar data3);
char *
get_misc_desc(uchar type, uchar num, ushort genid, uchar trig,
		 uchar data1, uchar data2, uchar data3)
{
	int i;
	char *pstr = NULL; 

	/* Use sens_desc array for other misc descriptions */
	data1 &= 0x0f;  /*ignore top half of sensor offset for matching */
	for (i = 0; i < NSDESC; i++) {
	     if ((sens_desc[i].s_typ == 0xff) || 
		     (sens_desc[i].s_typ == type)) {
	      if (sens_desc[i].s_num != 0xff &&
	         sens_desc[i].s_num != num)
			    continue;
	      if (sens_desc[i].genid != 0xffff &&
	         sens_desc[i].genid != genid)
			    continue;
	      if (sens_desc[i].evtrg != 0xff &&
	         sens_desc[i].evtrg != trig)
				    continue;
	      if (sens_desc[i].data1 != 0xff && 
      	          (sens_desc[i].data1 & 0x0f) != data1)
				    continue;
	      if (sens_desc[i].data2 != 0xff && 
      	          sens_desc[i].data2 != data2)
				    continue;
	      if (sens_desc[i].data3 != 0xff && 
      	          sens_desc[i].data3 != data3)
				    continue;
	      /* have a match, use description */
	      pstr = (char *)sens_desc[i].desc; 
	      break;
 	   }
	} /*end for*/
	return(pstr);
}  /* end get_misc_desc() */

void decode_sensor_event(uchar sensor_type, uchar sensor_number, ushort gen_id,
		uchar event_trigger, uchar data1, uchar data2, uchar data3,
		char *outbuf);
void
decode_sensor_event(uchar sensor_type, uchar sensor_number,  ushort gen_id,
		uchar event_trigger, uchar data1, uchar data2, uchar data3,
		char *outbuf)
{
	char mystr[26] = "(123)";  /*for panic string*/
	char poststr[24] = "OEM Post Code = %x%x";
	char genstr[10] = "03  ";
	char *gstr;
	char *pstr;
	uchar dtype;
	int i;

	 /* set dtype, used as array index below */
	if (sensor_type >= NSTYPES) dtype = 0;
	else dtype = sensor_type;
	switch(sensor_type) {
		     case 0x20:   /*OS Crit Stop*/
		        /* Show first 3 chars of panic string */
		        mystr[0] = '(';
		        mystr[1] = sensor_number & 0x7f;
		        mystr[2] = data2 & 0x7f;
		        mystr[3] = data3 & 0x7f;
			mystr[4] = ')';
			mystr[5] = 0;
			pstr = mystr;
			if (sensor_number & 0x80) strcat(mystr,"Oops!");
			if (data2 & 0x80) strcat(mystr,"Int!");
			if (data3 & 0x80) strcat(mystr,"NullPtr!");
		        break;
		     case 0x0f:    /*Post Errs*/
			switch (data1)
			{ 
			   case 0x00:  /* System firmware errors */
				i = data2;
				if (i >= NFWERRS) i = NFWERRS - 1;
        		        pstr = fwerrs[i].msg;
				break;
			   case 0x01:  /* System firmware hang  */
				i = data2;
				if (i >= NFWSTAT) i = NFWSTAT - 1;
				sprintf(poststr,"hang, %s",fwstat[i].msg);
				pstr = poststr;
				break;
			   case 0x02:  /* System firmware progress */
				i = data2;
				if (i >= NFWSTAT) i = NFWSTAT - 1;
				sprintf(poststr,"prog, %s",fwstat[i].msg);
				pstr = poststr;
				break;
			   case 0xa0:  /* OEM post codes */
				/* OEM post codes in bytes 2 & 3 (lo-hi) */
				sprintf(poststr,"POST Code %02x%02x",
					data3,data2);
				pstr = poststr;
				break;
			   default:  
				pstr = "POST Error";  /*default string*/
			}  /*end switch(data1)*/
		        break;
		     case 0x13:   /*Crit Int*/
			i = data1 & 0x0f;
			if (i >= NCRITS) i = NCRITS - 1;
			pstr = crit_int_str[i];
		        break;
		     case 0x21:   /*Slot/Con*/
			i = data1 & 0x0f;
			if (i >= NSLOTC) i = NSLOTC - 1;
			sprintf(mystr,"%s",slot_str[i]);
			/* could also decode data2/data3 here if valid */
			pstr = mystr;
		        break;
		     default:   /* all other sensor types, see sens_desc */
			pstr = get_misc_desc( sensor_type,
			     sensor_number, gen_id,
			     event_trigger, data1, data2, data3);
			if (pstr == NULL) {
			   mystr[0] = '-'; mystr[1] = 0;
			   pstr = mystr;
			}
		    
	}  /*end switch(sensor_type)*/

	/* show the Generator ID */
	sprintf(genstr,"%04x",gen_id);
	gstr = genstr;  /* default */
	for (i = 0; i < NGDESC; i++)
		    {
			if (gen_desc[i].g_id == gen_id)
			   gstr = (char *)gen_desc[i].desc;  
		    }

	/*firmware timestamp is #seconds since 1/1/1970 localtime*/
	/* timestamp handled by caller in HPI */
		    
	if ((event_trigger == 0x01) || /*threshold*/
	    (event_trigger == 0x81)) { /*threshold ok*/
		    sprintf(outbuf,
			"%s %02x %s act=%02x thr=%02x",
			sensor_types[dtype], sensor_number, pstr,
			data2,    /*actual reading*/
			data3 );  /*threshold value*/
		    } else {
		    sprintf(outbuf,
			"%s %02x %s %02x [%02x %02x %02x]",
			sensor_types[dtype] , sensor_number, pstr,
			event_trigger, data1, data2, data3 );
		    }
}

/*------------------------------------------------------------------------ 
 * decode_ipmi_event
 * Parse and decode the SEL record into readable format.
 * This routine is constructed so that it could be used as a library
 * function.
 * Input:   psel, a pointer to the IPMI SEL record
 * Output:  outbuf, a description of the event, max 80 chars.
 * Called by ReadSEL
 *------------------------------------------------------------------------*/
void decode_ipmi_event( SEL_RECORD *psel, char *outbuf);
void decode_ipmi_event( SEL_RECORD *psel, char *outbuf)
{
	char genstr[10] = "03  ";
	int i, j;

	if (psel->record_type >= 0xe0) { /*OEM Record 26.3*/
		   char *pc;
		   pc = (char *)&psel->timestamp;  /*bytes 4:16*/
		   sprintf(outbuf," OEM Event %02x %02x ", pc[0], pc[1]);
		   j = strlen(outbuf);
		   for (i = 2; i < 13; i++) {  /* 4:16 = 13 bytes data */
		      if (psel->record_type == 0xf0) { 
		        /* panic string will be type 0xf0 */
			if (pc[i] == 0) break;
			outbuf[j++] = pc[i];
		      } else {
			sprintf(&outbuf[j],"%02x ",pc[i]);
			j += 3;
		      }
		   }
		   outbuf[j++] = '\n';
		   outbuf[j++] = 0;
	} else if (psel->record_type == 0x02) {
		  /* most records are record type 2 */
		  decode_sensor_event(psel->sensor_type, 
			psel->sensor_number, psel->generator_id, 
			psel->event_trigger, psel->event_data1, 
			psel->event_data2, psel->event_data3, outbuf);

	}  /*endif type 2 */
	else {  /* other misc record type */
		   uchar *pc;
		   pc = (uchar *)&psel->record_type; 
		   sprintf(outbuf," Type%02x ", pc[0]);
		   j = strlen(outbuf);
		   for (i = 1; i < 14; i++) {
			sprintf(genstr,"%02x ",pc[i]);
			strcat(outbuf,genstr);
		   }
	}  /*endif misc type*/
	return;
}  /*end decode_ipmi_event()*/

#define IF_IPMI    0 	/* other interfaces, like H8, have diff numbers */
			/*Note that OpenHPI 2.0.0 does not yet fill this in*/

/*------------------------------------------------------------------------ 
 * decode_hpi_event
 * Parse and decode the SensorEvent record into readable format.
 * Input:   pevent, a pointer to the HPI SensorEvent record
 * Output:  outbuf, a description of the event, max 80 chars.
 * Called by ReadSEL
 *------------------------------------------------------------------------*/
void decode_hpi_event( SaHpiSensorEventT *pevent, char *outbuf);
void decode_hpi_event( SaHpiSensorEventT *pevent, char *outbuf)
{
   int ec, eci; 
   int es, esi; 
   int styp; 
   uchar data1, data2, data3, if_type;
   uchar etrig;
   ushort genid;
   char estag[8];
   char sbuf[80];
   char *ecstr;
   // char *pstr;

   /* decode event category */
   ec = pevent->EventCategory;
   for (eci = 0; eci < NUM_EC; eci++) 
	if (eventcats[eci].val == ec) break; 
   if (eci >= NUM_EC) eci = 0;
   ecstr = eventcats[eci].str;
   genid = 0x0020;   /* default to BMC */
   etrig = 0x6f;     /* default to sensor-specific*/
   /* decode event state */
   es = pevent->EventState;
   if (ec == SAHPI_EC_USER) {  /*OpenHPI usu returns this (0x7e)*/
        sprintf(estag,"%02x",es);
   } else if (eci == SAHPI_EC_THRESHOLD ) {  /*0x01*/
	for (esi = 0; esi < NUM_ES; esi++) 
		if (eventstates[esi].val == es) break; 
	if (esi >= NUM_ES) esi = 0;
	strcpy(estag,eventstates[esi].str);
        if (pevent->Assertion) etrig = 0x01;
        else etrig = 0x81;
   } else {
	sprintf(estag,"%02x:%02x",ec,es);
   }

   /* decode sensor type */
   styp = pevent->SensorType;
   if (styp >= NSTYPES) { styp = 0; }

   /* get if_type from SensorSpecific h.o. byte */
   if_type = (pevent->SensorSpecific & 0xff000000) >> 24;
   data3 = (pevent->SensorSpecific & 0x00ff0000) >> 16;
   data2 = (pevent->SensorSpecific & 0x0000ff00) >> 8;
   data1 = (pevent->SensorSpecific & 0x000000ff);
   if (if_type == IF_IPMI) { /* handle IPMI SensorSpecific stuff */
	/* fortunately, HPI & IPMI have the same sensor_type values */
	decode_sensor_event(styp, pevent->SensorNum, genid,
			etrig, data1, data2, data3, sbuf);
        sprintf(outbuf,"%s %s", estag, sbuf); 
   } 
   else {
      sprintf(outbuf,"%s, %s %s %x [%02x %02x %02x]", 
	   sensor_types[styp], ecstr, estag, 
	   pevent->SensorNum, data1, data2, data3);
   }

   return;
}

static
char *decode_error(SaErrorT code)
{
   int i;
   char *str = NULL;
   for (i = 0; i < NECODES; i++) {
	if (code == ecodes[i].code) { str = ecodes[i].str; break; }
   }
   if (str == NULL) { 
	sprintf(&def_estr[10],"%d",code);
	str = &def_estr[0];
   }
   return(str);
}

void ShowSel( SaHpiSelEntryT  *sel, SaHpiRdrT *rdr, SaHpiRptEntryT *rptentry);
void ShowSel( SaHpiSelEntryT  *sel, SaHpiRdrT *rdr, 
		SaHpiRptEntryT *rptentry )
{
  unsigned char evtype;
  char timestr[40];
  time_t tt1;
  char *srctag;
  char *rdrtag;
  char *pstr;
  unsigned char *pd;
  int outlen;
  char outbuf[132];
  int styp; 
  unsigned char data3;

  /*format & print the EventLog entry*/
  if (sel->Event.Timestamp > SAHPI_TIME_MAX_RELATIVE) { /*absolute time*/
     tt1 = sel->Event.Timestamp / 1000000000;
     strftime(timestr,sizeof(timestr),"%x %H:%M:%S", localtime(&tt1));
  } else if (sel->Event.Timestamp > SAHPI_TIME_UNSPECIFIED) { /*invalid time*/
     strcpy(timestr,"invalid time     ");
  } else {   /*relative time*/
     tt1 = sel->Event.Timestamp / 1000000000;
     sprintf(timestr,"rel(%lx)", (unsigned long)tt1);  
  }
  if (rptentry->ResourceId == sel->Event.Source) 
	srctag = rptentry->ResourceTag.Data;
  else  srctag = "unspec ";  /* SAHPI_UNSPECIFIED_RESOURCE_ID */
  evtype = sel->Event.EventType;
  if (evtype > NEVTYPES) evtype = NEVTYPES - 1;
  if (rdr->RdrType == SAHPI_NO_RECORD) rdrtag = "rdr-unkn"; 
  else {
	rdr->IdString.Data[rdr->IdString.DataLength] = 0; 
	rdrtag = &rdr->IdString.Data[0];
  }
  sprintf(outbuf,"%04x %s %s ", sel->EntryId, timestr, evtypes[evtype] );
  outlen = strlen(outbuf);
  pstr = "";
#ifdef HPI_A
  pd = &sel->Event.EventDataUnion.UserEvent.UserEventData[0];
#else
  pd = &sel->Event.EventDataUnion.UserEvent.UserEventData.Data[0];
#endif

  switch(evtype)
  {
     case SAHPI_ET_SENSOR:   /*Sensor*/
	pd = (void *)&(sel->Event.EventDataUnion.SensorEvent);
        decode_hpi_event((void *)pd,&outbuf[outlen]);
	break;
     case SAHPI_ET_USER:   /*User, usu 16-byte IPMI SEL record */
        decode_ipmi_event((void *)pd,&outbuf[outlen]);
	break;
     default:
        // decode_ipmi_event((void *)pd,&outbuf[outlen]);
	styp = pd[10];
	data3 = pd[15];
	   /* *sel->Event.EventDataUnion.SensorEvent.SensorSpecific+1 */
	if (styp >= NSTYPES) { 
		if (fdebug) printf("sensor type %d >= max %d\n",styp,NSTYPES);
		styp = 0; 
	}
	pstr = (char *)sensor_types[styp];
        sprintf(&outbuf[outlen], "%s, %x %x, %02x %02x %02x [%02x %02x %02x/%02x]",
			pstr, pd[0], pd[7], pd[10], pd[11], pd[12], 
				pd[13], pd[14], pd[15], data3);
	break;
   }
   printf("%s\n",outbuf);
}

static SaErrorT
DoEventLogForResource(SaHpiSessionIdT	sessionid,
		      SaHpiResourceIdT	resourceid,
		      char		*tagname)
{
    SaErrorT		rv;
    SaHpiSelInfoT	info;
    int			free = 50;
    SaHpiSelEntryIdT	entryid;
    SaHpiSelEntryIdT	nextentryid;
    SaHpiSelEntryIdT	preventryid;
    SaHpiSelEntryT	sel;
    SaHpiRdrT		rdr;
    SaHpiRptEntryT	rptentry;

    printf("rptentry[%d] tag: %s\n", resourceid, tagname);

    rv = saHpiEventLogInfoGet(sessionid, resourceid, &info);
    if (fdebug)
	printf("saHpiEventLogInfoGet %s\n", decode_error(rv));
    if (rv == SA_OK) {
   	free = info.Size - info.Entries;
	printf("EventLog entries=%d, size=%d, enabled=%d, free=%d\n",
			info.Entries, info.Size, info.Enabled, free);
    } 

    entryid = SAHPI_OLDEST_ENTRY;
    while ((rv == SA_OK) && (entryid != SAHPI_NO_MORE_ENTRIES))
    {
	/* rv = saHpiEventLogEntryGet(sessionid,SAHPI_DOMAIN_CONTROLLER_ID,entryid, */
	rv = saHpiEventLogEntryGet(sessionid, resourceid, entryid,
			&preventryid, &nextentryid, &sel, &rdr, &rptentry);
	if (fdebug)
	    printf("saHpiEventLogEntryGet[%x] %s, next=%x\n",
			entryid, decode_error(rv), nextentryid);
	if (rv == SA_OK) {
	    ShowSel(&sel, &rdr, &rptentry);
	    if (entryid == nextentryid)
		break;
	    preventryid = entryid;
	    entryid = nextentryid;
	}
    }

    if (free < 6) {
	/* 
	 * This test could be more generic if the log info fields above 
	 * were accurate.  IPMI SEL logs have a fixed size of 0x10000 
	 * bytes.  New log records are thrown away when it gets full.
	 * (OLDEST = 0, NO_MORE = fffffffe , so these are ok.)
	 */
	/* free = (0x10000 - 20) - preventryid; */ 
	printf(	"WARNING: Log free space is very low (%d records)\n"
		"         Clear log with hpisel -c\n", free);
    }

    return SA_OK;
}

int
main(int argc, char **argv)
{
  int c;
  SaErrorT rv;
  SaHpiSessionIdT sessionid;
#ifdef HPI_A
  SaHpiVersionT hpiVer;
  SaHpiRptInfoT rptinfo;
#endif
  SaHpiRptEntryT rptentry;
  SaHpiEntryIdT rptentryid;
  SaHpiEntryIdT nextrptentryid;
  SaHpiResourceIdT resourceid;

  printf("%s: version %s\n",argv[0],progver); 

#ifdef HPI_A
#define ARGSTR		"cx?"
#else
#define ARGSTR		"cdx?"
#endif

  while ( (c = getopt( argc, argv, ARGSTR)) != EOF )
  switch(c) {
        case 'c': fclear  = 1;  break;
#ifndef HPI_A
	case 'd': fdomain = 1; break;
#endif
        case 'x': fdebug  = 1;  break;
        default:
                printf("Usage %s [-cx]\n",argv[0]);
                printf("where -c clears the event log\n");
#ifndef HPI_A
		printf("      -d displays the domain event log\n");
#endif
                printf("      -x displays eXtra debug messages\n");
                exit(1);
  }
#ifdef HPI_A
  rv = saHpiInitialize(&hpiVer);
  if (rv != SA_OK) {
	printf("saHpiInitialize: %s\n",decode_error(rv));
	exit(-1);
	}
  rv = saHpiSessionOpen(SAHPI_DEFAULT_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
	if (rv == SA_ERR_HPI_ERROR) 
	   printf("saHpiSessionOpen: error %d, SpiLibd not running\n",rv);
	else
	   printf("saHpiSessionOpen: %s\n",decode_error(rv));
	exit(-1);
	}
  rv = saHpiResourcesDiscover(sessionid);
  if (fdebug) printf("saHpiResourcesDiscover %s\n",decode_error(rv));
  rv = saHpiRptInfoGet(sessionid,&rptinfo);
  if (fdebug) printf("saHpiRptInfoGet %s\n",decode_error(rv));
  printf("RptInfo: UpdateCount = %x, UpdateTime = %lx\n",
         rptinfo.UpdateCount, (unsigned long)rptinfo.UpdateTimestamp);
#else
  rv = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
	printf("saHpiSessionOpen: %s\n",decode_error(rv));
	exit(-1);
	}
  rv = saHpiDiscover(sessionid);
  if (fdebug) printf("saHpiDiscover %s\n",decode_error(rv));
#endif

#ifndef HPI_A
  if (fdomain != 0)		/* saw option for domain event log */
  {
    /* resourceid = 1; * oh_get_default_domain_id() */
    resourceid = SAHPI_UNSPECIFIED_DOMAIN_ID;
    if (fclear) {
	rv = saHpiEventLogClear(sessionid,resourceid);
	if (rv == SA_OK) printf("Domain EventLog successfully cleared\n");
	else printf("Domain EventLog clear, error = %d\n",rv);
    } else {
	rv = DoEventLogForResource(sessionid, resourceid, "Domain Event Log");
    }
  }
  else				/* walk the RPT list */
#endif
  {
    rptentryid = SAHPI_OLDEST_ENTRY;
    while ((rv == SA_OK) && (rptentryid != SAHPI_LAST_ENTRY))
    {
      rv = saHpiRptEntryGet(sessionid,rptentryid,&nextrptentryid,&rptentry);
      if (fdebug) 
	printf("saHpiRptEntryGet[%d] %s\n",rptentryid,decode_error(rv));
      if (rv != SA_OK)
	goto skip_to_next;
      resourceid = rptentry.ResourceId;
      if (fdebug) printf("RPT %x capabilities = %x\n", resourceid,
				rptentry.ResourceCapabilities);
      if ((rptentry.ResourceCapabilities & SAHPI_CAPABILITY_SEL) == 0)
	goto skip_to_next;
      if (fclear) {
	rv = saHpiEventLogClear(sessionid,resourceid);
	if (rv == SA_OK) printf("EventLog successfully cleared\n");
	else printf("EventLog clear, error = %d\n",rv);
	break;
      }
      // rptentry.ResourceTag.Data[rptentry.ResourceTag.DataLength] = 0; 

      rv = DoEventLogForResource(sessionid, resourceid, 
      					    rptentry.ResourceTag.Data);
    skip_to_next:
      rptentryid = nextrptentryid;
    }
  }
 
  rv = saHpiSessionClose(sessionid);
#ifdef HPI_A
  rv = saHpiFinalize();
#endif

  exit(0);
}
 
/* end hpisel.c */
