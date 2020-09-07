/*
 * ievents.c
 *
 * This file decodes the IPMI event into a readable string.
 * It is used by showsel.c and getevent.c.
 *
 * ievents.c compile flags:
 * METACOMMAND - defined if building for ipmiutil meta-command binary
 * ALONE       - defined if building for ievents standalone binary
 * else it could be compiled with libipmiutil.a 
 * LINUX, BSD, WIN32, DOS - defined for that OS
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 *
 * Copyright (c) 2009 Kontron America, Inc.
 * Copyright (c) 2006 Intel Corporation. 
 *
 * 05/26/05 Andy Cress - created from showsel.c (see there for history)
 * 06/20/05 Andy Cress - changed PowerSupply present/not to Inserted/Removed
 * 08/01/05 Andy Cress - added more Power Unit & Battery descriptions
 * 03/02/06 Andy Cress - more decoding for Power Unit 0b vs. 6f
 * 04/11/07 Andy Cress - added events -p decoding for PET data
 * 10/03/07 Andy Cress - added file_grep for -p in Windows
 * 03/03/08 Andy Cress - added -f to interpret raw SEL file
 */
/*M*
Copyright (c) 2009 Kontron America, Inc.
Copyright (c) 2006, Intel Corporation
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
#ifdef WIN32
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#elif defined(DOS)
#include <dos.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#endif
#include <time.h>

#include "ipmicmd.h" 
#include "ievents.h" 
 
#define  SELprintf  printf  
#define  SMS_SA   0x41
#define  SMI_SA   0x21
#ifdef METACOMMAND
extern char *progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil events";
#else
static char *progver   = "3.17";
static char *progname  = "ievents";
#endif
static char fsensdesc = 0;   /* 1= get extended sensor descriptions*/
static char fcanonical = 0;  /* 1= show canonical, delimited output*/
static char fgetdevid  = 0;  /* 1= get device ID */
static char fnewevt   = 0;    /* 1= generate New event */
static char futc      = 0;   /* 1= raw UTC time */
static uchar thr_sa   = SMS_SA; /* 0x41 (Sms) used by ipmitool PlarformEvents */
static void *sdrcache = NULL; 
static int  pet_guid  = 8;   /*bytes in the input data for the GUID*/
static int  iopt  = -1;   /*iana option*/
static char bcomma    = ',';
static char bdelim    = BDELIM; /* '|' */
#ifdef ALONE
       char fdebug = 0;      /* 1= debug output, standalone*/
static char fsm_debug = 0;
#else
extern char fdebug;          /* 1= debug output, from ipmicmd.c*/
extern char fsm_debug;       /*mem_if.c*/
#endif
#define SDR_SZ   80

#pragma pack(1)
typedef struct
{
        ushort             record_id;
        uchar              record_type;
        uint               timestamp;
        ushort             generator_id;    /*slave_addr/channel*/
        uchar              evm_rev;         //event message revision
        uchar              sensor_type;
        uchar              sensor_number;
        uchar              event_trigger;
        uchar              event_data1;
        uchar              event_data2;
        uchar              event_data3;
}       SEL_RECORD;
#pragma pack()

#ifdef WIN32
   static char sensfil[80] = "sensor_out.txt";
   static char sensfil2[80] = "%ipmiutildir%\\sensor_out.txt";
   // static char outfil[64] = "stype.tmp";
#else
   static char sensfil[80] = "/var/lib/ipmiutil/sensor_out.txt";
   static char sensfil2[80] = "/usr/share/ipmiutil/sensor_out.txt";
   // static char outfil[] = "/tmp/stype.tmp";
#endif
   static char rawfil[80] = "";

char *evt_hdr  = "RecId Date/Time_______ SEV Src_ Evt_Type___ Sens# Evt_detail - Trig [Evt_data]\n";
char *evt_hdr2 = "RecId | Date/Time  | SEV | Src | Evt_Type | Sensor | Evt_detail \n";
#ifdef MOVED 
/* moved SEV_* to ipmicmd.h */
#define SEV_INFO  0
#define SEV_MIN   1
#define SEV_MAJ   2
#define SEV_CRIT  3
#endif
#define NSEV  4
char *sev_str[NSEV] = {
  /*0*/ "INF",
  /*1*/ "MIN",
  /*2*/ "MAJ",
  /*3*/ "CRT" };

/* sensor_types: See IPMI 1.5 Table 36-3, IPMI 2.0 Table 42-3 */
#define NSTYPES   0x2F
static const char *sensor_types[NSTYPES] = {  
/* 00h */ "reserved",
/* 01h */ "Temperature",
/* 02h */ "Voltage",
/* 03h */ "Current",
/* 04h */ "Fan",
/* 05h */ "Platform Security",
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
/* 10h */ "Event Log",    /*SEL Disabled or Cleared */
/* 11h */ "Watchdog_1",
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
/* 23h */ "Watchdog_2",
/* 24h */ "Platform Alert",
/* 25h */ "Entity Presence",
/* 26h */ "Monitor ASIC",
/* 27h */ "LAN",
/* 28h */ "Management Subsystem Health",
/* 29h */ "Battery",
/* 2Ah */ "SessionAudit",
/* 2Bh */ "Version Change",
/* 2Ch */ "FRU State",
/* 2Dh */ "SMI Timeout",  /* 0xF3 == OEM SMI Timeout */
/* 2Eh */ "ME"    /* 0xDC == ME Node Manager */
};

#define NFWERRS  14
static struct {    /* See Table 36-3, type 0Fh, offset 00h */
  int code; char *msg; 
  } fwerrs[NFWERRS + 1] = {
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

#define NFWSTAT  26
static struct {    /* See Table 36-3, type 0Fh, offset 01h & 02h */
  int code; char *msg; 
  } fwstat[NFWSTAT + 1] = {
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

#define NGDESC   12
static struct {
 ushort g_id;
 const char desc[8];
} gen_desc[NGDESC] = {  /*empirical, format defined in IPMI 1.5 Table 23-5 */
 { 0x0000, "IPMB"},
 { 0x0001, "EFI "},
 { 0x0003, "BIOS"},    /* BIOS POST */
 { BMC_SA, "BMC "},    /* 0x0020==BMC_SA*/
 { SMI_SA, "SMI "},    /* 0x0021==SMI_SA, platform events */
 { 0x0028, "CHAS"},    /* Chassis Bridge Controller */
 { 0x0031, "mSMI"},    /* BIOS SMI/POST errors on miniBMC or ia64 */
 { 0x0033, "Bios"},    /* BIOS SMI (runtime), usually for memory errors */
 { SMS_SA, "Sms "},    /* 0x0041==SmsOs for MS Win2008 Boot, ipmitool event */
 { 0x002C, "ME  "},    /* ME Node Manager for S5500 */
 { 0x602C, "ME  "},    /* ME Node Manager for S5500/i7 bus=0x6, sa=0x2C */
 { HSC_SA, "HSC "}     /* 0x00C0==HSC_SA for SAF-TE Hot-Swap Controller*/
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

#define NBOOTI  8
char * boot_init_str[NBOOTI] = {  /* System Boot Initiated */
 /*00*/ "Power Up ",  
 /*01*/ "Hard Reset",  
 /*02*/ "Warm Reset",  
 /*03*/ "PXE Boot  ",  
 /*04*/ "Diag Boot ",  
 /*05*/ "SW Hard Reset",  
 /*06*/ "SW Warm Reset",  
 /*07*/ "RestartCause" };

#define NOSBOOT  10
char * osboot_str[NOSBOOT] = {  /* OS Boot */
 /*00*/ "A: boot completed",  
 /*01*/ "C: boot completed",  
 /*02*/ "PXE boot completed",  
 /*03*/ "Diag boot completed",  
 /*04*/ "CDROM boot completed",  
 /*05*/ "ROM boot completed",  
 /*06*/ "Other boot completed",
 /*07*/ "USB7 boot completed",
 /*08*/ "USB8 boot completed" };

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

#define NBATT  3
char * batt_str[NBATT] = {  /* Battery assert descriptions */
 /*00*/ "Low",
 /*01*/ "Failed",
 /*02*/ "Present" };
char * batt_clr[NBATT] = {  /* Battery deassert descriptions */
 /*00*/ "Low is OK now",
 /*01*/ "Failed is OK now",
 /*02*/ "Absent" };

#define N_NMH  6
char * nmh_str[N_NMH] = {  /* ME Node Manager Health (73) descriptions */
 /*10*/ "Policy Misconfig", 
 /*11*/ "Power Sensor Err",
 /*12*/ "Inlet Temp Sensor Err",
 /*13*/ "Host Comm Err",
 /*14*/ "RTC Sync Err" ,
 /*15*/ "Plat Shutdown" };   

#define N_NMFW  6
char * nmfw_str[N_NMFW] = {  /* ME Firmware Health (75) descriptions */
 /*00*/ "Forced GPIO Recov", 
 /*01*/ "Image Exec Fail",
 /*02*/ "Flash Erase Error",
 /*03*/ "Flash Corrupt" ,
 /*04*/ "Internal Watchdog Timeout",  // FW Watchdog Timeout, need to flash ME
 /*05*/ "BMC Comm Error" };

#define N_NM  4
char * nm_str[N_NM] = {  /* Node Manager descriptions */
 /*72*/ "NM Exception", /*or NM Alert Threshold*/
 /*73*/ "NM Health",
 /*74*/ "NM Capabilities",
 /*75*/ "FW Health" };  

#define NPROC  11
char * proc_str[NPROC] = {  /* Processor descriptions */
 /*00*/ "IERR",
 /*01*/ "Thermal Trip",
 /*02*/ "FRB1/BIST failure",
 /*03*/ "FRB2 timeout",
 /*04*/ "FRB3 Proc error",
 /*05*/ "Config error",
 /*06*/ "SMBIOS CPU error",
 /*07*/ "Present",
 /*08*/ "Disabled",
 /*09*/ "Terminator",
 /*0A*/ "Throttled" };

#define NACPIP  15
char * acpip_str[NACPIP] = {  /* ACPI Power State descriptions */
 /*00*/ "S0/G0 Working",
 /*01*/ "S1 sleeping, proc/hw context maintained",
 /*02*/ "S2 sleeping, proc context lost",
 /*03*/ "S3 sleeping, proc/hw context lost, mem ok",  /*42 chars*/
 /*04*/ "S4 non-volatile sleep/suspend",
 /*05*/ "S5/G2 soft-off",
 /*06*/ "S4/S5 soft-off, no specific state",
 /*07*/ "G3/Mechanical off",
 /*08*/ "Sleeping in an S1/S2/S3 state",
 /*09*/ "G1 sleeping",
 /*0A*/ "S5 entered by override",
 /*0B*/ "Legacy ON state",
 /*0C*/ "Legacy OFF state",
 /*0D*/ "Unknown",
 /*0E*/ "Unknown" }; 

#define NAUDIT  2
char * audit_str[NAUDIT] = {  /* Session Audit descriptions */
 /*00*/ "Activated",
 /*01*/ "Deactivated"};

#define N_AVAIL  9
char * avail_str[N_AVAIL] = {  /* Discrete Availability, evtype 0x0A */
 /*00*/ "chg to Running",  
 /*01*/ "chg to In Test",  
 /*02*/ "chg to Power Off",
 /*03*/ "chg to On Line",  
 /*04*/ "chg to Off Line", 
 /*05*/ "chg to Off Duty",  
 /*06*/ "chg to Degraded",  
 /*07*/ "chg to Power Save",  
 /*08*/ "Install Error"};


#define NSDESC   88  
struct {
 ushort genid;  /* generator id: BIOS, BMC, etc. (slave_addr/channel) */
 uchar s_typ;   /* 1=temp,2=voltage,4=fan,... */
 uchar s_num;
 uchar evtrg;   /* event trigger/type, see IPMI 1.5 table 36.1 & 36.2 */
 uchar data1;
 uchar data2;
 uchar data3;
 uchar sev;     /* 0=SEV_INFO, 1=SEV_MIN, 2=SEV_MAJ, 3=SEV_CRIT */
 const char desc[40];
} sens_desc[NSDESC] = {  /* if value is 0xff, matches any value */
{0xffff,0x05, 0xff, 0x6f, 0x40,0xff,0xff, 1,"Chassis Intrusion"}, /*chassis*/
{0xffff,0x05, 0xff, 0xef, 0x40,0xff,0xff, 0,"Chassis OK"},       /*chassis*/
{0xffff,0x05, 0xff, 0x6f, 0x44,0xff,0xff, 1,"LAN unplugged"},   /*chassis*/
{0xffff,0x05, 0xff, 0xef, 0x44,0xff,0xff, 0,"LAN restored "},   /*chassis*/
{0xffff,0x06, 0xff, 0xff, 0x45,0xff,0xff, 0,"Password"},        /*security*/
{0xffff,0x07, 0xff, 0xff, 0x41,0xff,0xff, 3,"Thermal trip"},    /*processor*/
{0xffff,0x08, 0xff, 0x6f, 0x40,0xff,0xff, 0,"Inserted"},       /*PowerSupply*/
{0xffff,0x08, 0xff, 0x6f, 0x41,0xff,0xff, 2,"Failure detected"},/*PowerSupply*/
{0xffff,0x08, 0xff, 0x6f, 0x42,0xff,0xff, 1,"Predictive failure"},
{0xffff,0x08, 0xff, 0x6f, 0x43,0xff,0xff, 0,"AC Lost"},
{0xffff,0x08, 0xff, 0x6f, 0x46,0xff,0xff, 2,"Config Error"},
{0xffff,0x08, 0xff, 0xef, 0x40,0xff,0xff, 1,"Removed"},          /*PowerSupply*/
{0xffff,0x08, 0xff, 0xef, 0x41,0xff,0xff, 0,"is OK  "},          /*PowerSupply*/
{0xffff,0x08, 0xff, 0xef, 0x42,0xff,0xff, 0,"Predictive OK"},
{0xffff,0x08, 0xff, 0xef, 0x43,0xff,0xff, 0,"AC Regained"},      /*PowerSupply*/
{0xffff,0x08, 0xff, 0xef, 0x46,0xff,0xff, 0,"Config OK now"},
{0xffff,0x0c, 0xff, 0xff, 0x00,0xff,0xff, 1,"Correctable ECC"},  /*memory*/
{0xffff,0x0f, 0x06, 0xff, 0xff,0xff,0xff, 2,"POST Code"},
{0xffff,0x10, 0x09, 0xff, 0x42,0x0f,0xff, 0,"Log Cleared"},
{0xffff,0x10, 0xff, 0xff, 0x02,0xff,0xff, 0,"Log Cleared"},
{0xffff,0x10, 0xff, 0xff, 0xc0,0x03,0xff, 1,"ECC Memory Errors"},
   /*  Often see these 3 Boot records with reboot:
    * 0003 12 83 6f 05 00 ff = System/SEL ClockSync_1 (start of POST)
    * 0003 12 83 6f 05 80 ff = System/SEL ClockSync_2 (end of POST)
    * 0003 12 83 6f 01 ff ff = OEM System Boot Event  (after POST, loader), or
    * 0003 12 83 6f 41 0f ff = OEM System Boot Event  (same w 01 & 41) 
    * can be either 0003 (BIOS) or 0001 (EFI)
    */
{0xffff,0x12, 0xfe, 0xff, 0xc5,0x00,0xff, 0,"Boot: ClockSync_1"}, 
{0xffff,0x12, 0xfe, 0xff, 0xc5,0x80,0xff, 0,"Boot: ClockSync_2"},   
{0xffff,0x12, 0x83, 0xff, 0x05,0x00,0xff, 0,"Boot: ClockSync_1"}, 
{0xffff,0x12, 0x83, 0xff, 0x05,0x80,0xff, 0,"Boot: ClockSync_2"},   
{0xffff,0x12, 0x83, 0xff, 0x01,0xff,0xff, 0,"OEM System Booted"},   
{0x0001,0x12, 0x08, 0xff, 0x01,0xff,0xff, 0,"OEM System Booted"},    /*ia64*/
{0xffff,0x12, 0x01, 0x6f, 0x01,0xff,0xff, 0,"OEM System Booted"},    /*S5000*/
{0xffff,0x12, 0x38, 0x6f, 0x00,0xff,0xff, 0,"OEM System Booted"},    /*KTC*/
{0xffff,0x12, 0x38, 0xef, 0x00,0xff,0xff, 0,"OEM System Booted"},    /*KTC*/
{0x0031,0x12, 0x00, 0x6f, 0xc3,0xff,0xff, 0,"PEF Action"},           /*ia64*/
{BMC_SA,0x12, 0x83, 0x6f, 0x80,0xff,0xff, 0,"System Reconfigured"},  /*BMC*/
{BMC_SA,0x12, 0x83, 0x0b, 0x80,0xff,0xff, 0,"System Reconfigured"},  /*BMC*/
{BMC_SA,0x12, 0x0b, 0x6f, 0x80,0xff,0xff, 0,"System Reconfigured"},  /*BMC*/
{BMC_SA,0x12, 0x83, 0xff, 0x41,0xff,0xff, 0,"OEM System Boot"},      /*BMC*/
{BMC_SA,0x12, 0x83, 0x6f, 0x42,0xff,0xff, 2,"System HW failure"},    /*BMC*/
{BMC_SA,0x12, 0x83, 0x6f, 0x04,0xff,0xff, 0,"PEF Action"},           /*BMC*/
{HSC_SA,0x0d, 0xff, 0x08, 0x00,0xff,0xff, 1,"Device Removed"},    /*HSC*/
{HSC_SA,0x0d, 0xff, 0x08, 0x01,0xff,0xff, 0,"Device Inserted"},   /*HSC*/
{0xffff,0x0d, 0xff, 0x6f, 0x00,0xff,0xff, 0,"Drive present"},   /*Romley BMC*/
{0xffff,0x0d, 0xff, 0xef, 0x00,0xff,0xff, 0,"Drive removed"},   /*BMC*/
{0xffff,0x0d, 0xff, 0x6f, 0x01,0xff,0xff, 2,"Drive fault"},    /*0x0D Drive*/
{0xffff,0x0d, 0xff, 0xef, 0x01,0xff,0xff, 0,"Drive fault OK"},  
{0xffff,0x0d, 0xff, 0x6f, 0x02,0xff,0xff, 1,"Drive predict fail"},
{0xffff,0x0d, 0xff, 0x6f, 0x05,0xff,0xff, 1,"Drive not redundant"},
{0xffff,0x0d, 0xff, 0x6f, 0x07,0xff,0xff, 0,"Rebuild in progress"},
{0xffff,0x0d, 0xff, 0xef, 0x07,0xff,0xff, 0,"Rebuild complete"},   
{0xffff,0x14, 0xff, 0xff, 0x42,0xff,0xff, 1,"Reset Button pressed"},
{0xffff,0x14, 0xff, 0xff, 0x40,0xff,0xff, 1,"Power Button pressed"},
{0xffff,0x14, 0xff, 0xff, 0x01,0xff,0xff, 0,"ID Button pressed"},
{0xffff,0x23, 0xff, 0xff, 0x40,0xff,0xff, 2,"Expired, no action"},/*watchdog2*/
{0xffff,0x23, 0xff, 0xff, 0x41,0xff,0xff, 3,"Hard Reset action"}, /*watchdog2*/
{0xffff,0x23, 0xff, 0xff, 0x42,0xff,0xff, 3,"Power down action"}, /*watchdog2*/
{0xffff,0x23, 0xff, 0xff, 0x43,0xff,0xff, 3,"Power cycle action"},/*watchdog2*/
{0xffff,0x23, 0xff, 0xff, 0x48,0xff,0xff, 2,"Timer interrupt"},   /*watchdog2*/
{0xffff,0xf3, 0xff, 0x83, 0x41,0xff,0xff, 0,"SMI de-asserted"},
{0xffff,0xf3, 0xff, 0x03, 0x41,0xff,0xff, 3,"SMI asserted"},
{0xffff,0x20, 0x00, 0xff, 0xff,0xff,0xff, 3,"OS Kernel Panic"},
{0xffff,0x01, 0xff, 0x03, 0x01,0xff,0xff, 2,"Temp Asserted"}, /*Discrete temp*/
{0xffff,0x01, 0xff, 0x83, 0x01,0xff,0xff, 0,"Temp OK"},       /*Discrete ok*/
{0xffff,0x01, 0xff, 0x05, 0x01,0xff,0xff, 2,"Temp Asserted"}, /*CPU Hot */
{0xffff,0x01, 0xff, 0x85, 0x01,0xff,0xff, 0,"Temp OK"},       /*CPU Hot ok*/
{0xffff,0x01, 0xff, 0x07, 0x01,0xff,0xff, 2,"Temp Asserted"}, /*Discrete temp*/
{0xffff,0x01, 0xff, 0x87, 0x01,0xff,0xff, 0,"Temp OK"},       /*Discrete ok*/
	/*Thresholds apply to sensor types 1=temp, 2=voltage, 3=current, 4=fan*/
	/*Note that last 2 bytes usu show actual & threshold values*/
	/*evtrg: 0x01=thresh, 0x81=restored/deasserted */
{0xffff,0xff, 0xff, 0x01, 0x50,0xff,0xff, 1,"Lo Noncrit thresh"},
{0xffff,0xff, 0xff, 0x01, 0x51,0xff,0xff, 0,"Lo NoncritH OK now"},
{0xffff,0xff, 0xff, 0x01, 0x52,0xff,0xff, 2,"Lo Crit thresh"},
{0xffff,0xff, 0xff, 0x01, 0x53,0xff,0xff, 0,"Lo CritHi OK now"},
{0xffff,0xff, 0xff, 0x01, 0x54,0xff,0xff, 3,"Lo Unrec thresh"},
{0xffff,0xff, 0xff, 0x01, 0x55,0xff,0xff, 0,"Lo UnrecH OK now"},
{0xffff,0xff, 0xff, 0x01, 0x56,0xff,0xff, 0,"Hi NoncritL OK now"},
{0xffff,0xff, 0xff, 0x01, 0x57,0xff,0xff, 1,"Hi Noncrit thresh"}, 
{0xffff,0xff, 0xff, 0x01, 0x58,0xff,0xff, 0,"Hi CritLo OK now"}, 
{0xffff,0xff, 0xff, 0x01, 0x59,0xff,0xff, 2,"Hi Crit thresh"},
{0xffff,0xff, 0xff, 0x01, 0x5A,0xff,0xff, 0,"Hi UnrecL OK now"},
{0xffff,0xff, 0xff, 0x01, 0x5B,0xff,0xff, 3,"Hi Unrec thresh"},
{0xffff,0xff, 0xff, 0x81, 0x50,0xff,0xff, 0,"LoN thresh OK now"},
{0xffff,0xff, 0xff, 0x81, 0x51,0xff,0xff, 1,"LoNoncrit thresh"},
{0xffff,0xff, 0xff, 0x81, 0x52,0xff,0xff, 0,"LoC thresh OK now"},
{0xffff,0xff, 0xff, 0x81, 0x53,0xff,0xff, 2,"LoCritHi thresh"},
{0xffff,0xff, 0xff, 0x81, 0x54,0xff,0xff, 0,"LoU thresh OK now"},
{0xffff,0xff, 0xff, 0x81, 0x55,0xff,0xff, 3,"LoUnrecH thresh"},
{0xffff,0xff, 0xff, 0x81, 0x56,0xff,0xff, 1,"HiNoncrit thresh"},
{0xffff,0xff, 0xff, 0x81, 0x57,0xff,0xff, 0,"HiN thresh OK now"},
{0xffff,0xff, 0xff, 0x81, 0x58,0xff,0xff, 2,"HiCritLo thresh"},
{0xffff,0xff, 0xff, 0x81, 0x59,0xff,0xff, 0,"HiC thresh OK now"},
{0xffff,0xff, 0xff, 0x81, 0x5A,0xff,0xff, 3,"HiURecLo thresh"},
{0xffff,0xff, 0xff, 0x81, 0x5B,0xff,0xff, 0,"HiU thresh OK now"}
};

#define NENTID  53
static struct { char * str; uchar styp; } entitymap[NENTID] = {
/* 00 */ { "unspecified", 0x00 },
/* 01 */ { "other", 0x00 },
/* 02 */ { "unknown", 0x00 },
/* 03 */ { "Processor", 0x07 },
/* 04 */ { "Disk", 0x00 },
/* 05 */ { "Peripheral bay", 0x0D },
/* 06 */ { "Management module", 0x00 },
/* 07 */ { "System board", 0x00 },
/* 08 */ { "Memory module", 0x00 },
/* 09 */ { "Processor module", 0x00 },
/* 10 */ { "Power supply", 0x08 },
/* 11 */ { "Add-in card", 0x00 },
/* 12 */ { "Front panel bd", 0x00 },
/* 13 */ { "Back panel board", 0x00 },
/* 14 */ { "Power system bd", 0x00 },
/* 15 */ { "Drive backplane", 0x00 },
/* 16 */ { "Expansion board", 0x00 },
/* 17 */ { "Other system board", 0x15 },
/* 18 */ { "Processor board", 0x15 },
/* 19 */ { "Power unit", 0x09 },
/* 20 */ { "Power module", 0x08 },
/* 21 */ { "Power distr board", 0x09 },
/* 22 */ { "Chassis back panel bd", 0x00 },
/* 23 */ { "System Chassis", 0x00 },
/* 24 */ { "Sub-chassis", 0x00 },
/* 25 */ { "Other chassis board", 0x15 },
/* 26 */ { "Disk Drive Bay", 0x0D },
/* 27 */ { "Peripheral Bay", 0x00 },
/* 28 */ { "Device Bay", 0x00 },
/* 29 */ { "Fan", 0x04 },
/* 30 */ { "Cooling unit", 0x0A },
/* 31 */ { "Cable/interconnect", 0x1B },
/* 32 */ { "Memory device ", 0x0C },
/* 33 */ { "System Mgt Software", 0x28 },
/* 34 */ { "BIOS", 0x00 },
/* 35 */ { "Operating System", 0x1F },
/* 36 */ { "System bus", 0x00 },
/* 37 */ { "Group", 0x00 },
/* 38 */ { "Remote Mgt Comm Device", 0x00 },
/* 39 */ { "External Environment", 0x00 },
/* 40 */ { "battery", 0x29 },
/* 41 */ { "Processing blade", 0x00 },
/* 43 */ { "Processor/memory module", 0x0C },
/* 44 */ { "I/O module", 0x15 },
/* 45 */ { "Processor/IO module", 0x00 },
/* 46 */ { "Mgt Controller Firmware", 0x0F },
/* 47 */ { "IPMI Channel", 0x00 },
/* 48 */ { "PCI Bus", 0x00 },
/* 49 */ { "PCI Express Bus", 0x00 },
/* 50 */ { "SCSI Bus", 0x00 },
/* 51 */ { "SATA/SAS bus", 0x00, },
/* 52 */ { "Processor FSB", 0x00 }
};

char *decode_entity_id(int id)
{
   char *str = NULL;
   if (id < 0) id = 0;
   if (id > NENTID) {
      if (id >= 0x90 && id < 0xB0) str = "Chassis-specific";
      else if (id >= 0xB0 && id < 0xD0) str = "Board-specific";
      else if (id >= 0xD0 && id <= 0xFF) str = "OEM-specific";
      else str = "invalid";
   } else  str = entitymap[id].str;
   return(str);
}

uchar entity2sensor_type(uchar ent)
{
   uchar stype = 0x12;
   uchar b;
   if (ent < NENTID) {
	b = entitymap[ent].styp;
	if (fdebug) printf("entity2sensor_type(%x,%s), stype=%x\n",ent,
				entitymap[ent].str,b);
	if (b != 0) stype = b;
   }
   return(stype);
}

static char *mem_str(int off) 
{
   char *pstr;
   switch(off) {
     case 0x00: pstr = "Correctable ECC"; break;
     case 0x01: pstr = "Uncorrectable ECC"; break;
     case 0x02: pstr = "Parity"; break;
     case 0x03: pstr = "Memory Scrub Failed"; break;
     case 0x04: pstr = "Memory Device Disabled"; break;
     case 0x05: pstr = "ECC limit reached" ; break;
     case 0x07: pstr = "ConfigError: SMI Link Lane FailOver"; break; /*Quanta QSSC_S4R*/
     case 0x08: pstr = "Spare";  break;  /*uses data3 */
     case 0x09: pstr = "Memory Automatically Throttled";  break; 
     case 0x0A: pstr = "Critical Overtemperature";  break;  
     default:   pstr = "Other Memory Error"; break;
   }
   return(pstr);
}


#if defined(METACOMMAND)
/* METACOMMAND is defined for ipmiutil meta-command build. */
/* These can only be external if linked with non-core objects. */
extern int GetSDR(int recid, int *recnext, uchar *sdr, int szsdr, int *slen);
extern double RawToFloat(uchar raw, uchar *psdr);
extern char *get_unit_type(uchar iunit, uchar ibase, uchar imod, int flg);
extern int find_sdr_by_snum(uchar *psdr,uchar *pcache, uchar snum, uchar sa);
extern int GetSensorType(int snum, uchar *stype, uchar *rtype);
extern char * get_mfg_str(uchar *rgmfg, int *pmfg);  /*from ihealth.c*/
extern int decode_post_intel(int prod, ushort code, char *outbuf,int szbuf);
extern int decode_sel_kontron(uchar *evt, char *obuf,int sz,char fdesc,char fdbg); /*oem_kontron.c*/
extern int decode_sel_fujitsu(uchar *evt, char *obuf,int sz,char fdesc,char fdbg); /*oem_fujitsu.c*/
extern int decode_sel_intel(uchar *evt, char *obuf,int sz,char fdesc,char fdbg); /*oem_intel.c*/
extern int decode_sel_supermicro(uchar *evt, char *obuf,int sz,char fdesc,char fdbg); /*oem_supermicro.c*/
extern int decode_sel_lenovo(uchar *evt, char *obuf,int sz,char fdesc,char fdbg); /*oem_lenovo.c*/
extern int decode_sel_quanta(uchar *evt, char *obuf,int sz,char fdesc,char fdbg); /*oem_quanta.c*/
extern int decode_sel_newisys(uchar *evt, char *obuf,int sz,char fdesc,char fdbg); /*oem_newisys.c*/
extern int decode_sel_dell(uchar *evt, char *obuf,int sz,char fdesc,char fdbg); /*oem_dell.c*/
extern int decode_mem_intel(int prod, uchar b2, uchar b3, char *desc, int *psz); /*oem_intel.c*/
extern int decode_mem_supermicro(int prod, uchar b2, uchar b3, char *desc, int *psz); /*oem_intel.c*/
#else
/* else it could be ALONE or for a libipmiutil.a build */
static int default_mem_desc(uchar b2, uchar b3, char *desc, int *psz) 
{
    int array, dimm, n;
    uchar bdata;
    if (desc == NULL || psz == NULL) return -1;
    if (b3 == 0xff) bdata = b2;  /*ff is reserved*/
    else  bdata = b3; /* normal case */
    array = (bdata & 0xc0) >> 6;
    dimm = bdata & 0x3f;
    if (bdata == 0xFF) n = sprintf(desc,DIMM_UNKNOWN);
    else n = sprintf(desc,DIMM_NUM,dimm);
    *psz = n;
    return(0);
}
static int decode_mem_intel(int prod, uchar b2, uchar b3, char *desc, int *psz)
{ return(default_mem_desc(b2,b3,desc,psz)); }
static int decode_mem_supermicro(int prod, uchar b2, uchar b3, char *desc, int *psz)
{ return(default_mem_desc(b2,b3,desc,psz)); }
static char * get_mfg_str(uchar *rgmfg, int *pmfg)
{    /* standalone routine here for minimal set of strings*/
    char *mfgstr;
    int mfg;
    mfg  = rgmfg[0] + (rgmfg[1] << 8) + (rgmfg[2] << 16);
    if (pmfg != NULL) *pmfg = mfg;
    if (mfg == VENDOR_INTEL) mfgstr = "Intel";
    else if (mfg == VENDOR_MICROSOFT) mfgstr = "Microsoft";
    else if (mfg == VENDOR_KONTRON) mfgstr = "Kontron";
    else if (mfg == VENDOR_SUPERMICROX) mfgstr = "SuperMicro";
    else if (mfg == VENDOR_SUPERMICRO) mfgstr = "SuperMicro";
    else mfgstr = " ";
    return (mfgstr);
}
#ifdef SENSORS_OK
extern int GetSDR(int recid, int *recnext, uchar *sdr, int szsdr, int *slen);
extern double RawToFloat(uchar raw, uchar *psdr);
extern char *get_unit_type(uchar iunit, uchar ibase, uchar imod, int flg);
extern int find_sdr_by_snum(uchar *psdr,uchar *pcache, uchar snum, uchar sa);
extern int GetSensorType(int snum, uchar *stype, uchar *rtype);
#else
static int GetSDR(int recid, int *recnext, uchar *sdr, int szsdr, int *slen)
{ return(-1); }
static double RawToFloat(uchar raw, uchar *psdr)
{ return((double)raw); }
static char *get_unit_type(uchar iunit, uchar ibase, uchar imod, int flg)
{ return(""); };
static int find_sdr_by_snum(uchar *psdr,uchar *pcache, uchar snum, uchar sa)
{ return(-1); }
static int GetSensorType(int snum, uchar *stype, uchar *rtype)
{ return(-1); }
#endif
#endif

#if defined(ALONE)
/* ALONE is defined for ievents standalone build. */
/* IPMI Spec says that this is an index into the DIMMs.
 * Walking through the SDRs dynamically would be too slow,
 * but isel -e has an SDR cache which could be leveraged,
 * however not all platforms have DIMM slot SDRs. 
 * The BIOS references the DIMM Locator descriptors from 
 * SMBIOS type 17, and the descriptions vary for each baseboard.
 * We'll just show the index here by default.
 * Do the SMBIOS lookup if not standalone build. */
int g_vend_id = VENDOR_INTEL;  /*assume a default of Intel*/

int strlen_(const char *s) { return((int)strlen(s)); }
char *get_iana_str(int vend) { return(""); }  /*from subs.c*/
void set_iana(int vend) { g_vend_id = vend; return; }
char is_remote(void) { return(1); } /* act as if remote with standalone */

int get_MemDesc(int array, int idimm, char *desc, int *psz) 
{
    /* standalone, so use the default method for the DIMM index */
    return(default_mem_desc(array,idimm,desc,psz));
}

void get_mfgid(int *vend, int *prod)
{
    if ((vend == NULL) || (prod == NULL)) return;
    *vend = g_vend_id;  /*assume a default of Intel*/
    *prod = 0;
}
char * decode_rv(int rv) 
{
   static char mystr[30];
   char *pstr;
   switch(rv) {
   case 0:  pstr = "completed successfully"; break;
   case ERR_BAD_PARAM: pstr = "invalid parameter"; break;
   case ERR_USAGE:     pstr = "usage or help requested"; break;
   case ERR_FILE_OPEN: pstr = "cannot open file"; break;
   case ERR_NOT_FOUND: pstr = "item not found"; break;
   default: sprintf(mystr,"error = %d",rv); pstr = mystr; break;
   }
   return(pstr);
}
void dump_buf(char *tag, uchar *pbuf, int sz, char fshowascii)
{
   uchar line[17];
   uchar a;
   int i, j;
   char *stag;
   FILE *fpdbg1;

   fpdbg1 = stdout;
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
int ipmi_cmdraw(uchar cmd, uchar netfn, uchar sa, uchar bus, uchar lun,
		uchar *pdata, int sdata, uchar *presp,
		int *sresp, uchar *pcc, char fdebugcmd)
{ return(-9); }
#else
/* ipmicmd.h has ipmi_cmdraw() */
extern int  get_MemDesc(int array, int dimm, char *desc, int *psz); /*mem_if.c*/
extern void get_mfgid(int *vend, int *prod);  /*from ipmicmd.c*/
extern char is_remote(void);   /*from ipmicmd.c*/
extern char * decode_rv(int rv);  /*from ipmilan.c*/
extern void dump_buf(char *tag, uchar *pbuf, int sz, char fshowascii);
#endif

static int decode_mem_default(uchar b1, uchar b2, uchar b3, uchar etype, char *desc, int *psz)
{
   int cpu, dimm, n;
   int rv = -1;
   int offset = 0;
   uchar bdata;

   if ((desc == NULL) || (psz == NULL)) return -1;
   offset = (b1 & 0x0F);
   if ((b1 & 0x20) != 0) { bdata = b3; }  /*dimm data in byte 3*/
   else if (b2 == 0xff) { bdata = b3; }  /*ff is reserved*/
   else { bdata = b2; }   /*if here, should also have (b1 & 0x80)*/

   if (bdata == 0xFF) n = sprintf(desc,DIMM_UNKNOWN);
   else {
      cpu = (bdata & 0xc0) >> 6;
      dimm = (bdata & 0x3F);  
      n = sprintf(desc,DIMM_NUM,dimm);
      /* Use DMI if we get confirmation about cpu/dimm indices. */
      if (! is_remote()) {
         fsm_debug = fdebug;
         rv = get_MemDesc(cpu,dimm,desc,psz);
         if (rv != 0) n = sprintf(desc,DIMM_NUM,dimm);
      }
   }
   *psz = n;
   if (fdebug) 
	 printf("decode_mem_default: bdata=%02x(%d) %d dimm=%d\n",bdata,bdata,offset,dimm);
   rv = 0;
   return(rv);
} /*end decode_mem_default*/

int new_event(uchar *buf, int len)
{
   int rlen, rv;
   uchar idata[8];
   uchar rdata[4];
   uchar cc;
   /*
   Platform Event Message command, inputs:
   offset  
   0 = GeneratorID, 0x20 for BMC, 0x21 for SMI/kernel, 0x33 for BIOS
   1 = EVM Rev, 3=IPMI10, 4=IPMI15
   2 = Sensor Type
   3 = Sensor Number
   4 = Event Type (0x6f = sensor specific)
   5 = data1
   6 = data2
   7 = data3
   */
   idata[0] = buf[0]; /*GenID on input is two bytes, but one byte here*/
   idata[1] = buf[2];
   idata[2] = buf[3];
   idata[3] = buf[4];
   idata[4] = buf[5];
   idata[5] = buf[6];
   idata[6] = buf[7];
   idata[7] = buf[8];
   rlen = sizeof(rdata);
   rv = ipmi_cmdraw(0x02, NETFN_SEVT, BMC_SA, PUBLIC_BUS, BMC_LUN,
                        idata,8, rdata,&rlen,&cc, fdebug);
   if (fdebug) printf("platform_event: rv = %d, cc = %02x\n",rv,cc);
   if ((rv == 0) && (cc != 0)) rv = cc;
   return(rv);
}

char *get_sensor_type_desc(uchar stype)
{
    int i;
    static char stype_desc[25];
    char *pstr;
    if (stype == 0xF3) i = 0x2D; /*OEM SMI*/
    else if (stype == 0xDC) i = 0x2E; /*NM*/
    else if (stype == SMI_SA) i = BMC_SA; /*SMI -> BMC*/
    else if (stype >= NSTYPES) i = 0;
    else i = stype;
    if (i == 0) {  /* reserved, show the raw sensor_type also */
        if (stype == 0xCF) 
	     strncpy(stype_desc,"OEM Board Reset", sizeof(stype_desc));
	else if (stype >= 0xC0) 
	     sprintf(stype_desc,"OEM(%02x)",stype);
	else sprintf(stype_desc,"%s(%02x)",sensor_types[i],stype); /*reserved*/
	pstr = (char *)stype_desc;
    } else {
	pstr = (char *)sensor_types[i];
    }
    return(pstr);
}

/*------------------------------------------------------------------------ 
 * get_misc_desc
 * Uses the sens_desc array to decode misc entries not otherwise handled.
 * Called by decode_sel_entry
 *------------------------------------------------------------------------*/
char *
get_misc_desc(ushort genid, uchar type, uchar num, uchar trig,
		 uchar data1, uchar data2, uchar data3, uchar *sev)
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
      	          (sens_desc[i].data1 & 0x0f) != (data1 & 0x0f))
				    continue;
	      if (sens_desc[i].data2 != 0xff && 
      	          sens_desc[i].data2 != data2)
				    continue;
	      if (sens_desc[i].data3 != 0xff && 
      	          sens_desc[i].data3 != data3)
				    continue;
	      /* have a match, use description */
	      pstr = (char *)sens_desc[i].desc; 
	      if (sev != NULL) *sev = sens_desc[i].sev; 
	      break;
           }
	} /*end for*/
	return(pstr);
}  /* end get_misc_desc() */

time_t utc2local(time_t t)
{
   struct tm * tm_tmp;
   int gt_year,gt_yday,gt_hour,lt_year,lt_yday,lt_hour;
   int delta_hour;
   time_t lt;
   // convert UTC time to local time 
   // i.e. number of seconds from 1/1/70 0:0:0 1970 GMT
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

void fmt_time(time_t etime, char *buf, int bufsz)
{
	time_t t;
	if (bufsz < 18) printf("fmt_time: buffer size should be >= 18\n");
	if (futc) t = etime;
        else t = utc2local(etime);  /*assume input time is UTC*/
	strncpy(buf,"00/00/00 00:00:00",bufsz);
        strftime(buf,bufsz, "%x %H:%M:%S", gmtime(&t)); /*or "%x %T"*/
	return;
}

/* 
 * findmatch 
 * Finds a matching pattern within a string buffer.
 * returns offset of the match if found, or -1 if not found.  
 */
int
findmatch(char *buffer, int sbuf, char *pattern, int spattern, char figncase)
{
    int c, i, j, imatch;

    j = 0;
    imatch = 0;
    for (j = 0; j < sbuf; j++) {
        if ((sbuf - j) < spattern && imatch == 0) return(-1);
        c = buffer[j];
        if (c == pattern[imatch]) {
            imatch++;
        } else if ((figncase == 1) &&
                   ((c & 0x5f) == (pattern[imatch] & 0x5f))) {
            imatch++;
        } else if (pattern[imatch] == '?') {  /*wildcard char*/
            imatch++;
        } else {
            if (imatch > 0) {
               if (j > 0) j--; /* try again with the first match char */
               imatch = 0;
	    }
        }
        if (imatch == spattern) break;
    }
    if (imatch == spattern) {
        i = (j+1) - imatch;  /*buffer[i] is the match */
        return(i);
    } else return (-1);  /*not found*/
}				/*end findmatch */

/* 
 * file_grep
 * Search (grep) for a pattern within a file.
 * Inputs:  fname  = file name to search
 *          pattn  = pattern string to search for
 *          line   = line buffer 
 *          sline  = size of line buffer
 *          bmode  = 0 to use last match, 
 *                   1 to use first match, 
 *                   2 to specify starting line number & use first match.
 *          nret   = IN: starting char offset, OUT: num chars read
 * Outputs: line   = resulting line (stringified) that matches pattn
 *          nret   = resulting line number within file (0-based)
 *          returns 0 if match, < 0 if error
 */
int file_grep(char *fname, char *pattn, char *line, int sline, 
		     char bmode, int *nret)
{
    FILE *fp;
    char buff[1024];
    int ret = ERR_NOT_FOUND;
    int i, plen, blen;
    int n = 0;
    int nstart = 0;
    int bufsz;

    if ((bmode == 2) && (nret != NULL)) nstart = *nret;
    bufsz = sizeof(buff);
    fp = fopen(fname,"r");
    if (fp == NULL) {
	if (fdebug) printf("file_grep: Cannot open %s\n",fname);
	ret = ERR_FILE_OPEN; /*cannot open file*/
    } else {
	plen = strlen_(pattn);
        fseek(fp, nstart, SEEK_SET);
	n = nstart;
        while (fgets(buff, bufsz, fp) != NULL)
	{
           blen = strlen_(buff);
	   /* check for pattern in this line */
	   i = findmatch(&buff[0],blen,pattn,plen,0);
  	   if (i >= 0) {
		ret = 0;  /* found it, success */
		if ((line != NULL) && sline > 1) {
		   if (blen >= sline) blen = sline - 1;
		   strncpy(line,buff,blen);
		   line[blen] = 0;  /*stringify*/
		}
                if (nret != NULL) *nret = n + i + plen;
		if (bmode > 0) break;  
	 	/* else keep looking, use last one if multiples */
	   }
           n += blen;  /*number of chars*/
        } /*end while*/
        fclose(fp);
    }  /*end else file opened*/
    return(ret); 
}  /*end file_grep*/

char *get_sev_str(int val)
{
   if (val >= NSEV) val = SEV_CRIT;
   return(sev_str[val]);
}

/* The htoi() routine is available in subs.c, but ievents.c also 
 * needs a local copy of it if built with -DALONE. */
static uchar _htoi(char *inhex)
{
   // char rghex[16] = "0123456789ABCDEF";
   uchar val;
   char c;
   c = inhex[0] & 0x5f;  /* force cap */
   if (c > '9') c += 9;  /* c >= 'A' */
   val = (c & 0x0f) << 4;
   c = inhex[1] & 0x5f;  /* force cap */
   if (c > '9') c += 9;  /* c >= 'A' */
   val += (c & 0x0f);
   return(val);
}

/* 
 * set_sel_opts is used to specify options for showing/decoding SEL events.
 * sensdesc : 0 = simple, no sensor descriptions
 *            1 = get sensor descriptions from sdr cache
 *            2 = get sensor descriptions from sensor file (-s)
 * canon    : 0 = normal output
 *            1 = canonical, delimited output
 * sdrs     : NULL = no sdr cache, dynamically get sdr cache if sensdesc==1
 *            ptr  = use this pointer as existing sdr cache if sensdesc==1
 * fdbg     : 0 = normal mode
 *            1 = debug mode
 * futc     : 0 = normal mode
 *            1 = show raw UTC time
 */
void set_sel_opts(int sensdesc, int canon, void *sdrs, char fdbg, char utc)
{
    fsensdesc = (char)sensdesc;  /*get extended sensor descriptions*/
    fcanonical = (char)canon;    /*show canonical, delimited output*/
    if (sdrcache == NULL) sdrcache = sdrs;
    else printf("Warning: attempted to set_sel_opts(sdrcache) twice\n");
    fdebug = fdbg;
    futc = utc;
}

/* get_sensdesc - get the sensor tag/description from the sensor.out file */
int get_sensdesc(uchar sa, int snum, char *sensdesc, int *pstyp, int *pidx)
{
   int rv, i, j, len, idx;
   char pattn[20];
   char sensline[100];
   int nline = 0;
   uchar sa2;
   char *sfil;
   char *p;

   if (sensdesc == NULL) return ERR_BAD_PARAM;
   sensdesc[0] = 0;
   if (fdebug) printf("sensdesc(%02x,%02x) with %s\n",sa,snum,sensfil);
   sprintf(pattn,"snum %02x",snum);
   for (j = 0; j < 3; j++)
   {
      sfil = sensfil;
      /* Use this logic for both Linux and Windows */
      rv = file_grep(sfil,pattn, sensline, sizeof(sensline), 2, &nline);
      if (rv != 0) {
	 if (rv == ERR_FILE_OPEN) {
	    if (fdebug) fprintf(stdout,"Cannot open file %s\n",sfil);
	    sfil = sensfil2;
	    rv = file_grep(sfil, pattn, sensline,sizeof(sensline), 2, &nline);
	    if (fdebug && rv == ERR_FILE_OPEN)
		fprintf(stdout,"Cannot open file %s\n",sfil);
	 }
	 if (rv != ERR_FILE_OPEN) {  
	    if (fdebug) printf("Cannot find snum %02x in file %s\n",snum,sfil);
	    rv = ERR_NOT_FOUND;
	 }
	 break;
      }
      if (rv == 0) {
	 idx = _htoi(&sensline[2]) + (_htoi(&sensline[0]) << 8);
	 sa2 = _htoi(&sensline[20]);
	 if (fdebug) 
	     printf("sensdesc(%02x,%02x) found snum for sa %02x at offset %d\n",				sa,snum,sa2,nline);
	 if (sa == sa2) {
	    /* truncate the sensor line to omit the reading */
	    len = strlen_(sensline);
	    for (i = 0; i < len; i++) 
	       if (sensline[i] == '=') { sensline[i] = 0; break; }
	    if (sensline[i-1] != ' ') {
	       sensline[i] = ' '; sensline[++i] = 0;
	    }
	    /* skip to just the sensor description from the SDR */
	    p = strstr(sensline,"snum");
	    p += 8; /* skip 'snum 11 ' */
	    strcpy(sensdesc,p);
	    if (pstyp != NULL) *pstyp = _htoi(&sensline[25]);
	    if (pidx != NULL) *pidx = idx;
	    break;
         }
      }
    } /*end-for j*/
    if (j >= 3) rv = ERR_NOT_FOUND;  /*not found*/
    return(rv);
}

char *get_genid_str(ushort genid)
{
   static char genstr[10];
   char *gstr;
   int i;
		    
   sprintf(genstr,"%04x",genid);
   gstr = genstr;  /* default */
   for (i = 0; i < NGDESC; i++)
   {
      if (gen_desc[i].g_id == genid) {
         gstr = (char *)gen_desc[i].desc;  
	 break;
      }
   }
   return(gstr);
}

static int is_threshold(uchar evtrg, ushort genid)
{
   int val = 0;  /*false*/
   /* It would be better to check the SDR (if Full supports thresholds),
    * but we do not always have that available. */
   if ( ((genid == BMC_SA) ||   /*from BMC*/
         (genid == SMI_SA) ||   /*from SMI (simulated)*/
	 (genid == thr_sa)) &&  /*from HSC, or other*/
        ((evtrg == 0x01) ||  /*threshold evt*/
         (evtrg == 0x81))) /*threshold ok*/
            val = 1;
   return(val);
}

static void get_sdr_tag(uchar *sdr, char *tagstr)
{
   int i, j, k, len;
   len = sdr[4] + 5;
   switch(sdr[3]) {  
   case 0x01: k = 48; break; /*full sensor*/
   case 0x02: k = 32; break; /*compact sensor*/
   case 0x03: k = 17; break; /*compact sensor*/
   case 0x10: k = 16; break; /*compact sensor*/
   case 0x11: k = 16; break; /*compact sensor*/
   case 0x12: k = 16; break; /*compact sensor*/
   default:   k = 0; break;
   }
   if (k > 0 && k < len) {
	i = len - k;
	for (j = 0; j < i; j++) {
	   if (sdr[k+j] == 0) break;
	   tagstr[j] = sdr[k+j];
	}
	tagstr[j++] = ' ';
	tagstr[j] = 0;
   }
}

/* 
 * get_sensor_tag
 *
 * Get the sensor tag (name) based on the sensor number, etc.
 * Use one of 3 methods to get this:
 *   1) Parse the sensor_out.txt (sensfil) if fsensdesc==2.
 *      Use this method if not METACOMMAND or if user-specified.
 *   2) Find this SDR in the SDR cache, if available.
 *   3) Do a GetSDR command function now (can be slow)
 * Input parameters:
 *  isdr  = index of SDR, use 0 if unknown
 *  genid = genid or sa (slave address) of this sensor
 *  snum  = sensor number of this sensor
 *  tag   = pointer to a buffer for the sensor tag (min 17 bytes)
 *  sdr   = pointer to buffer for the SDR if found (usu <= 65 bytes)
 *  szsdr = size of the SDR buffer
 * Output parameters:
 *  tag   = filled in with sensor tag (name)
 *  sdr   = filled in with SDR, if found. 
 * Returns:
 *   0 if tag and sdr are found.
 *   ERR_NOT_FOUND if SDR is not found (but tag may be found)
 *   other errors, see ipmicmd.h 
 */
int get_sensor_tag(int isdr, int genid, uchar snum, char *tag,
			uchar *sdr, int szsdr) 
{
   int rv, i, j = 0;
   if (tag == NULL) return(ERR_BAD_PARAM);
   if (sdr == NULL) return(ERR_BAD_PARAM);
   if (genid == SMS_SA) genid = BMC_SA;  /*parse Sms as if BMC*/
   if (genid == SMI_SA) genid = BMC_SA;  /*parse SMI as if BMC*/
   tag[0] = 0;
   if (fsensdesc == 2) { /*not connected, so do not try to GetSDR*/
	rv = get_sensdesc((uchar)genid, snum, tag,NULL,&isdr);
	rv = ERR_NOT_FOUND; /*got tag, but did not get SDR*/
   } else if (sdrcache != NULL) { /*valid sdr cache*/
	rv = find_sdr_by_snum(sdr,sdrcache, snum, (uchar)genid);
	if (rv == 0) {
	   get_sdr_tag(sdr,tag);
	}
   } else {  /* try to get this SDR */ 
	rv = GetSDR(isdr, &i,sdr,szsdr,&j);
	if (fdebug) printf("get_sensor_tag GetSDR[%x] rv=%d sz=%d\n",isdr,rv,j);
	if (rv == 0) {
	   get_sdr_tag(sdr,tag);
	} else {  /* use a saved sensor.out file */
	   rv = get_sensdesc((uchar)genid, snum, tag,NULL,&isdr);
	   if (rv != 0) tag[0] = 0;
	   rv = ERR_NOT_FOUND; /*got tag, but did not get SDR*/
	}
   }
   if (rv != 0) strcpy(tag,"na ");
   if (fdebug) printf("get_sensor_tag(%d): find_sdr(%x,%x) rv=%d tag=/%s/\n",
			fsensdesc,snum,genid,rv,tag);
   return(rv);
}

static
int decode_post_oem(int vend, int prod, ushort code, char *outbuf,int szbuf)
{
   int rv = -1;
   if (outbuf == NULL || szbuf == 0) return(rv);
#if defined(METACOMMAND)
   switch(vend) {
   case VENDOR_INTEL:
      rv = decode_post_intel(prod,code,outbuf,szbuf);
      break;
   default:
      break;
   }
#endif
   if (rv != 0) snprintf(outbuf,szbuf,"POST Code %04x",code);
   return(rv);
}

static
int decode_sel_oem(int vend, uchar *pevt, char *outbuf,int szbuf,
			char fdesc, char fdbg)
{
   int rv = -1;
#ifdef METACOMMAND
   switch(vend) {
   case VENDOR_KONTRON:
      rv = decode_sel_kontron(pevt,outbuf,szbuf,fdesc,fdbg);
      break;
   case VENDOR_FUJITSU:
      /* Fujitsu does an OEM IPMI command to return the decoded string. */
      rv = decode_sel_fujitsu(pevt,outbuf,szbuf,fdesc,fdbg);
      break;
   case VENDOR_INTEL:
      thr_sa = HSC_SA; /* HSC_SA(0xC0) by default */
      rv = decode_sel_intel(pevt,outbuf,szbuf,fdesc,fdbg);
      break;
   case VENDOR_PEPPERCON:  /*SuperMicro AOC-SIMSO*/
      if (pevt[7] == 0x40) pevt[7] = BMC_SA;  /*genid broken, fix it*/
      break;
   case VENDOR_MAGNUM:
   case VENDOR_SUPERMICRO:
   case VENDOR_SUPERMICROX:
      rv = decode_sel_supermicro(pevt,outbuf,szbuf,fdesc,fdbg);
      break;
   case VENDOR_LENOVO:
   case VENDOR_LENOVO2:
      rv = decode_sel_lenovo(pevt,outbuf,szbuf,fdesc,fdbg);
      break;
   case VENDOR_QUANTA:
      rv = decode_sel_quanta(pevt,outbuf,szbuf,fdesc,fdbg);
      break;
   case VENDOR_NEWISYS:
      rv = decode_sel_newisys(pevt,outbuf,szbuf,fdesc,fdbg);
      break;
   case VENDOR_DELL:
      rv = decode_sel_dell(pevt,outbuf,szbuf,fdesc,fdbg);
      break;
   default:
      break;
   }
#endif
   if (fdebug) printf("decode_sel_oem(0x%04x) rv=%d\n",vend,rv);
   return(rv);
}

#define N_PWRUNIT  12
static struct {
  uchar trg; uchar data1; uchar sev; char *msg; 
  } pwrunit_evts[N_PWRUNIT] = {
{ 0x6f, 0x00, 0,"Power Off    "},   
{ 0x6f, 0x01, 0,"Power Cycle  "},    
{ 0x6f, 0x02, 0,"240VA power down"},
{ 0x6f, 0x03, 0,"Interlock power down"},
{ 0x6f, 0x04, 1,"AC Lost"},
{ 0x6f, 0x05, 2,"Soft Powerup failure"},
{ 0x6f, 0x06, 2,"Failure detected"},
{ 0x6f, 0x07, 1,"Predictive failure"},
{ 0xef, 0x00, 0,"Power Restored"},
{ 0xef, 0x01, 0,"Power Cycle ok"},
{ 0xef, 0x04, 0,"AC Regained"},
{ 0xef, 0x06, 0,"Failure OK now"}
};
static int decode_pwrunit(uchar trg, uchar data1, char *pstr, uchar *psev)
{
   int rv = -1;
   int i, j;
   j = data1 & 0x0f;
   if (psev == NULL || pstr == NULL) return(rv);
   sprintf(pstr,"_");
   for (i = 0; i < N_PWRUNIT; i++) 
   {
      if ((trg == pwrunit_evts[i].trg) &&
          (j == pwrunit_evts[i].data1)) {
	*psev = pwrunit_evts[i].sev;
	sprintf(pstr,"%s",pwrunit_evts[i].msg);
	rv = 0;
      }
   }
   return(rv);
}

#define N_REDUN   16
static struct {
  uchar trg; uchar data1; uchar sev; char *msg; 
  } redund_evts[N_REDUN] = {
{ 0x0B, 0x00, SEV_INFO,"Redundancy OK  "},
{ 0x0B, 0x01, SEV_MAJ, "Redundancy Lost"}, 
{ 0x0B, 0x02, SEV_MAJ, "Redundancy Degraded"},
{ 0x0B, 0x03, SEV_MAJ, "Not Redundant"},
{ 0x0B, 0x04, SEV_MAJ, "Sufficient Resources"},
{ 0x0B, 0x05, SEV_CRIT,"Insufficient Resources"},
{ 0x0B, 0x06, SEV_MAJ, "Fully-to-Degraded"},
{ 0x0B, 0x07, SEV_MIN, "NonR-to-Degraded"},
{ 0x8B, 0x00, SEV_MAJ, "Redundancy NOT ok"},
{ 0x8B, 0x01, SEV_INFO,"Redundancy Regained"},
{ 0x8B, 0x02, SEV_INFO,"Redundancy Restored"},
{ 0x8B, 0x03, SEV_INFO,"Redundant"},    
{ 0x8B, 0x04, SEV_MAJ, "Not Sufficient"}, 
{ 0x8B, 0x05, SEV_INFO,"Not Insufficient"}, 
{ 0x8B, 0x06, SEV_INFO,"Degraded-to-Fully"}, 
{ 0x8B, 0x07, SEV_MIN, "Degraded-to-NonR"}
};
static int decode_redund(uchar trg, uchar data1, char *pstr, uchar *psev)
{
   int rv = -1;
   int i, j;
   j = data1 & 0x0f;
   if (psev == NULL || pstr == NULL) return(rv);
   sprintf(pstr,"_");
   for (i = 0; i < N_REDUN; i++) 
   {
      if ((trg == redund_evts[i].trg) &&
          (j == redund_evts[i].data1)) {
	*psev = redund_evts[i].sev;
	sprintf(pstr,"%s",redund_evts[i].msg);
	rv = 0;
      }
   }
   return(rv);
}

#define N_PRESENT  2
static char * present_str[N_PRESENT] = {  /* Availability, evtype 0x08 */
 /*00*/ "Absent",   /*Absent/Removed*/
 /*01*/ "Present"}; /*Present/Inserted*/

static int decode_presence(uchar trg, uchar data1, char *pstr, uchar *psev)
{
   int rv = -1;
   int i;
   if (psev == NULL || pstr == NULL) return(rv);
   sprintf(pstr,"_");
   *psev = SEV_INFO;
   i = data1 & 0x0f;
   if (trg == 0x08) {
	if (i >= N_PRESENT) i = N_PRESENT - 1;
	sprintf(pstr,"%s",present_str[i]);
	rv = 0;
   } else if (trg == 0x88) {
	if (data1 & 0x01) i = 0;
	else i = 1;
	sprintf(pstr,"%s",present_str[i]);
   } 
   return(rv);
}

void format_event(ushort id, time_t timestamp, int sevid, ushort genid,
		char *ptype, uchar snum, char *psens, char *pstr, char *more, 
		char *outbuf, int outsz)
{
   char sensdesc[36];
   char timestr[40]; 
   uchar sdr[MAX_BUFFER_SIZE]; /*sdr usu <= 65 bytes*/
   char *gstr; 
   int isdr = 0;
   int rv;

   if (more == NULL) more = "";
   if (psens != NULL) ;  /* use what was passed in */
   else {   /*psens==NULL, may need to get tag*/
        psens = &sensdesc[0];
	sensdesc[0] = 0;
	if (fsensdesc) {
           rv = get_sensor_tag(isdr,genid,snum,psens,sdr,sizeof(sdr));
           if (fdebug) printf("get_sensor_tag(%x) rv = %d\n",snum,rv);
	}
   }
   fmt_time(timestamp, timestr, sizeof(timestr));
   gstr = get_genid_str(genid); /*get Generator ID / Source string*/

   if (fcanonical) {
      snprintf(outbuf,outsz,"%04x %c %s %c %s %c %s %c %s %c %s %c %s %s\n",
			id, bdelim, timestr,  bdelim,
			get_sev_str(sevid), bdelim,
			gstr, bdelim, ptype, bdelim,
                        psens, bdelim, pstr, more );
   } else {
      snprintf(outbuf,outsz,"%04x %s %s %s %s #%02x %s %s %s\n",
			id, timestr, get_sev_str(sevid),
			gstr, ptype, snum, psens, pstr, more);
   }
   return;
}

/*------------------------------------------------------------------------ 
 * decode_sel_entry
 * Parse and decode the SEL record into readable format.
 * This routine is constructed so that it could be used as a library
 * function.
 * Note that this routine outputs 14 of the 16 bytes in either text or
 * raw hex form.  For the two bytes not shown:
 *   . record type:   usually = 02, is shown otherwise (e.g. OEM type)
 *   . event msg revision: =03 if IPMI 1.0, =04 if IPMI 1.5 or IPMI 2.0
 *
 * Input:   psel, a pointer to the IPMI SEL record (16 bytes)
 * Output:  outbuf, a description of the event, max 80 chars.
 * Called by: ReadSEL()
 * Calls:     fmt_time() get_misc_desc()
 *
 * IPMI SEL record format (from IPMI Table 32-1):
 * Offset   Meaning
 *  0-1     Record ID (LSB, MSB)
 *  2       Record Type (usu 0x02)
 *  3-6     Timestamp (LS byte first)
 *  7-8     Generator ID (LS first, usually 20 00)
 *  9       Event message format (=0x04, or =0x03 if IPMI 1.0)
 * 10       Sensor type
 * 11       Sensor number
 * 12       Event Dir | Event Type 
 * 13       Event Data 1
 * 14       Event Data 2
 * 15       Event Data 3
 *------------------------------------------------------------------------*/
int decode_sel_entry( uchar *pevt, char *outbuf, int szbuf)
{
	char mystr[80] = "panic(123)";  /*used for panic string*/
	char *pstr;
	char poststr[80] = "OEM Post Code = %x%x";
	char sensstr[50];
	char datastr[64];
	char cstr[4];
        char *psensstr;
	int i, j, k, n;
	time_t eventTime;
	uchar *evtime;
	char timebuf[40];
	uchar *pc;
	SEL_RECORD *psel;
	uchar fdeassert = 0;
	uchar etype;
	uchar sev = SEV_INFO;
	uchar sdr[MAX_BUFFER_SIZE]; /*sdr usu <= 65 bytes*/
	int isdr = 0;
	int vend, prod;
	int rv = 0;
	char fhave_sdr = 0;
	char mdesc[80];   /*used for oem memory description*/
	int msz;
	char *mfgstr;
	int mfg;

	if (outbuf == NULL) return(ERR_BAD_PARAM);
	if (pevt == NULL) {
		outbuf[0] = 0;
		return(ERR_BAD_PARAM);
	}
	get_mfgid(&vend,&prod); /*saved from ipmi_getdeviceid */
	psel = (SEL_RECORD *)pevt;
	etype = psel->event_trigger;

	j = decode_sel_oem(vend,pevt,outbuf,szbuf,fsensdesc,fdebug);
	if (j == 0) return(0);  /*successful, have the description*/

	if (psel->record_type == RT_OEMIU) { /* 0xDB usu ipmiutil OEM string */
		   /* ipmiutil OEM event with 9-byte string */
		   pc = (uchar *)&psel->generator_id;  /* offset 7 */
		   evtime = (uchar *)&psel->timestamp;
		   eventTime = evtime[0] + (evtime[1] << 8) + 
		   		(evtime[2] << 16) + (evtime[3] << 24);
		   fmt_time(eventTime, timebuf, sizeof(timebuf));
		   if (fcanonical) 
		     sprintf(outbuf,"%04x %c %s %c %s %c %02x %c OEM Event ",
			          psel->record_id, bdelim, timebuf, bdelim, 
			          get_sev_str(sev), bdelim, psel->record_type, bdelim);
		   else
		     sprintf(outbuf,"%04x %s %s %02x OEM Event ", 
			   psel->record_id, timebuf, get_sev_str(sev),
			   psel->record_type);
		   j = strlen_(outbuf);
		   for (i = 0; i < 9; i++) {  /* 7:16 = 9 bytes string data */
			   if (pc[i] == 0) outbuf[j] = ' ';
			   else sprintf(&outbuf[j],"%c",pc[i]);
			   j += 1;
		   }
		   outbuf[j++] = '\n';
		   outbuf[j++] = 0;
	} else if (psel->record_type == 0xDC) {
		   /* OEM Record: these are usually Microsoft */
		   evtime = (uchar *)&psel->timestamp;
		   eventTime = evtime[0] + (evtime[1] << 8) + 
		   		(evtime[2] << 16) + (evtime[3] << 24);
		   fmt_time(eventTime, timebuf, sizeof(timebuf));
		   pc = (uchar *)&psel->generator_id;  /* offset 7 */
		   mfgstr = get_mfg_str(&pc[0],&mfg);
   
		   if (fcanonical) 
                     sprintf(outbuf,"%04x %c %s %c %s %c %02x %c %06x %c %s %c OEM Event ",
			psel->record_id, bdelim, timebuf, bdelim, 
			get_sev_str(sev), bdelim, psel->record_type, bdelim, 
			mfg, bdelim, mfgstr, bdelim);
		   else
		     sprintf(outbuf,"%04x %s %s %02x %06x %s OEM Event ", 
			   psel->record_id, timebuf, get_sev_str(sev),
			   psel->record_type, mfg, mfgstr);
		   j = strlen_(outbuf);
		   for (i = 3; i < 9; i++) {  /* 10:16 = 6 bytes data */
			sprintf(&outbuf[j],"%02x ",pc[i]);
			j += 3;
		   }
		   outbuf[j++] = '\n';
		   outbuf[j++] = 0;
	} else if (psel->record_type == 0xDD) {  /* usu Intel OEM string */
		   int ix = 0;
		   /* Windows reboot reason string from MS ipmidrv.sys */
		   evtime = (uchar *)&psel->timestamp;
		   eventTime = evtime[0] + (evtime[1] << 8) + 
		   		(evtime[2] << 16) + (evtime[3] << 24);
		   fmt_time(eventTime, timebuf, sizeof(timebuf));
		   pc = (uchar *)&psel->generator_id;  /* IANA at offset 7 */
		   mfgstr = get_mfg_str(&pc[0],&mfg);
		   if (fcanonical) 
                     sprintf(outbuf,"%04x %c %s %c %s %c %02x %c %06x %c %s %c OEM Event ",
			psel->record_id, bdelim, timebuf, bdelim, 
			get_sev_str(sev), bdelim, psel->record_type, bdelim, 
			mfg, bdelim, mfgstr, bdelim);
		   else
		     sprintf(outbuf,"%04x %s %s %02x %06x %s OEM Event ", 
			   psel->record_id, timebuf, get_sev_str(sev),
			   psel->record_type, mfg, mfgstr );
		   j = strlen_(outbuf);
		   for (i = 3; i < 9; i++) {  /* 10:16 = 6 bytes data */
			if (i == 3 || ix == 0) {
			   if (i == 3) ix = pc[i];
			   sprintf(&outbuf[j],"%02x ",pc[i]);
			   j += 3;
			} else {
			   if (pc[i] == 0) outbuf[j] = ' ';
			   else sprintf(&outbuf[j],"%c",pc[i]);
			   j += 1;
			} 
		   }
		   outbuf[j++] = '\n';
		   outbuf[j++] = 0;
	} else if (psel->record_type >= 0xe0) { /*OEM Record 26.3*/
		   /* 3 bytes header, 13 bytes data, no timestamp */
		   pc = (uchar *)&psel->timestamp;  /*bytes 4:16*/
		   if (fcanonical) 
                     sprintf(outbuf,"%04x %c %s %c %s %c %02x %c %s %c %s %c OEM Event ",
			psel->record_id, bdelim, "", bdelim, 
			get_sev_str(sev), bdelim, psel->record_type, bdelim, 
			"", bdelim, "", bdelim);
		   else
		     sprintf(outbuf,"%04x %s %02x OEM Event ", 
			psel->record_id,get_sev_str(sev),psel->record_type);
		   j = strlen_(outbuf);
		   for (i = 0; i < 13; i++) {  /* 5:16 = 11 bytes data */
		      if ((psel->record_type == 0xf0) && (i >= 2)) { 
		        /* Linux panic string will be type 0xf0 */
			if (pc[i] == 0) break;
			outbuf[j++] = pc[i];
		      } else if (psel->record_type == 0xf1) {
			/* custom ascii string record, type 0xf1 */
			if (i == 0) {  /*linux panic*/
			   outbuf[j++] = ':';
			   outbuf[j++] = ' ';
			}
			if (pc[i] == 0) break;
			outbuf[j++] = pc[i];
		      } else {
			sprintf(&outbuf[j],"%02x ",pc[i]);
			j += 3;
		      }
		   }
		   outbuf[j++] = '\n';
		   outbuf[j++] = 0;
	} else if (psel->record_type >= 0xc0) { /*OEM Record 26.3*/
		   /* 10 bytes header, 6 bytes data, has timestamp */
		   evtime = (uchar *)&psel->timestamp;
		   eventTime = evtime[0] + (evtime[1] << 8) + 
		   		(evtime[2] << 16) + (evtime[3] << 24);
		   fmt_time(eventTime, timebuf, sizeof(timebuf));
		   pc = (uchar *)&psel->generator_id;  /* IANA at offset 7 */
		   if (fcanonical) 
                     sprintf(outbuf,"%04x %c %s %c %s %c %02x %c %02x%02x%02x %c %s %c OEM Event ",
			psel->record_id, bdelim, timebuf, bdelim, 
			get_sev_str(sev), bdelim, psel->record_type, bdelim, 
			pc[2],pc[1],pc[0], bdelim, "", bdelim);
		   else
		     sprintf(outbuf,"%04x %s %s %02x %02x%02x%02x OEM Event ", 
			   psel->record_id, timebuf, get_sev_str(sev),
			   psel->record_type, pc[2],pc[1],pc[0] );
		   j = strlen_(outbuf);
		   for (i = 3; i < 9; i++) {  /* 10:16 = 6 bytes data */
			sprintf(&outbuf[j],"%02x ",pc[i]);
			j += 3;
		   }
		   outbuf[j++] = '\n';
		   outbuf[j++] = 0;
	} else if (psel->record_type == 0x02) {
		    uchar c = 0;
		    /* most records are record type 2 */
		    /* Interpret the event by sensor type */
		    switch(psel->sensor_type) 
		    {
		     case 0x20:   /*OS Crit Stop*/
			i = psel->event_data1 & 0x0f;
			switch(i) {
			case 0x00: pstr = "Startup Crit Stop";
				   sev = SEV_CRIT; break;
			case 0x02: pstr = "OS Graceful Stop"; break;
			case 0x03: pstr = "OS Graceful Shutdown"; break;
			case 0x04: pstr = "PEF Soft-Shutdown"; break;
			case 0x05: pstr = "Agent Not Responding"; break;
			case 0x01:  /*OS Runtime Critical Stop (panic)*/
			default:
			   sev = SEV_CRIT; 
			   if (psel->sensor_number == 0) { /*Windows*/
			      pstr = "Runtime Crit Stop";
			   } else {  /*Linux panic, get string*/
		              /* Show first 3 chars of panic string */
			      pstr = mystr;
			      strcpy(mystr,"panic(");
			      for (i = 6; i <= 8; i++) {
			        switch(i) {
				   case 6: c = psel->sensor_number; break;
				   case 7: c = psel->event_data2; break;
				   case 8: c = psel->event_data3; break;
			        }
			        c &= 0x7f;
			        if (c < 0x20) c = '.';
			        mystr[i] = c;
			      }
			      mystr[9] = ')';
			      mystr[10] = 0;
			      if (psel->sensor_number & 0x80) 
				   strcat(mystr,"Oops!");
			      if (psel->event_data2 & 0x80) 
				   strcat(mystr,"Int!");
			      if (psel->event_data3 & 0x80) 
				   strcat(mystr,"NullPtr!");
			   }
			   break;
			}  /*end data1 switch*/
		        break;
		     case 0x01:   /*Temperature events*/
			if (is_threshold(psel->event_trigger,
					 psel->generator_id)) {
			   pstr = get_misc_desc( psel->generator_id,
						psel->sensor_type,
						psel->sensor_number,
						psel->event_trigger,
						psel->event_data1,
						psel->event_data2,
						psel->event_data3, &sev);
			} else {  /*else discrete temp event*/
			   /* data1 should usually be 0x01 */
			   if (psel->event_trigger & 0x80) 
				strcpy(mystr,"Temp OK");
			   else strcpy(mystr,"Temp Asserted");
			   pstr = mystr;
			}  /*end-else discrete*/
		        break;
		     /* case 0X04 for Fan events is further below. */
		     case 0x07:   /*Processor (CPU)*/
			i = psel->event_data1 & 0x0f;
			if (psel->event_trigger == 0x6f) {
			  /* Processor status sensor */
			  if (i >= NPROC) i = NPROC - 1;
			  if (i == 7) sev = SEV_INFO;
			  else if (i > 7) sev = SEV_MIN;
			  else sev = SEV_CRIT;
			  sprintf(mystr,"%s",proc_str[i]);
			  pstr = mystr;
			} else if (psel->event_trigger == 0xef) {
			  sev = SEV_CRIT;
			  if (i >= NPROC) i = NPROC - 1;
			  sprintf(mystr,"%s deasserted",proc_str[i]);
			  pstr = mystr;
			} else if (psel->sensor_number == 0x80) { /*CATERR*/
			  char *p1, *p2;
			  sev = SEV_CRIT;
			  switch( psel->event_data2 & 0x0F)
			  {
			  case 1:  p1 = "CATERR"; break;
			  case 2:  p1 = "CPU Core Error"; break;
			  case 3:  p1 = "MSID Mismatch"; break;
			  default: p1 = "Unknown Error"; break;
			  }
			  if (psel->event_data2 & 0x01) p2 = "CPU0";
			  else if (psel->event_data2 & 0x02) p2 = "CPU1";
			  else if (psel->event_data2 & 0x04) p2 = "CPU2";
			  else if (psel->event_data2 & 0x08) p2 = "CPU3";
			  else p2 = "CPU4";
			  sprintf(mystr,"%s on %s",p1,p2);
			  pstr = mystr;
			} else if (psel->event_trigger == 0x03) {
			  if (i) {pstr = "Proc Config Error"; sev = SEV_CRIT;}
			  else   {pstr = "Proc Config OK"; sev = SEV_INFO; }
			} else if (psel->event_trigger == 0x83) {
			  if (i) { pstr = "Proc Config OK"; sev = SEV_INFO; }
			  else   { pstr = "Proc Config Error"; sev = SEV_CRIT; }
                        } else { /* else other processor sensor */
			  i = ((psel->event_trigger & 0x80) >> 7);
			  if (i) { pstr = "ProcErr Deasserted"; sev = SEV_INFO;}
			  else   { pstr = "ProcErr Asserted"; sev = SEV_CRIT; }
			}
		        break;
		     case 0x08:   /*Power Supply*/
		        pstr = NULL; /*falls through to unknown*/
		        if ((psel->event_trigger == 0x0b) ||
		            (psel->event_trigger == 0x8b)) {
		            rv = decode_redund(psel->event_trigger,
							psel->event_data1, mystr, &sev);
		            if (rv == 0) pstr = mystr;
				} else {
			        pstr = get_misc_desc( psel->generator_id,
						psel->sensor_type,
						psel->sensor_number,
						psel->event_trigger,
						psel->event_data1,
						psel->event_data2,
						psel->event_data3, &sev);
				}
		        break;
		     case 0x09:   /*Power Unit*/
			if ((psel->event_trigger == 0x0b) ||
			    (psel->event_trigger == 0x8b)) {
		           rv = decode_redund(psel->event_trigger,
					psel->event_data1, mystr, &sev);
			} else {  /*sensor-specific 0x6f/0xef*/
		           rv = decode_pwrunit(psel->event_trigger,
					psel->event_data1, mystr, &sev);
			}
			if (rv == 0) pstr = mystr;
			else pstr = NULL; /*falls through to unknown*/
		        break;
		     case 0x0C:   /*Memory*/
			i = psel->event_data1 & 0x0f;  /*memstr index*/
			{  /* now get the DIMM index from data2 or data3 */
			   uchar b2, b3, bdata;
			   b2 = psel->event_data2;
			   b3 = psel->event_data3;
			   if ((vend == VENDOR_INTEL && prod == 0x4311) ||
			       (vend == VENDOR_NSC)) {  /*mini-BMC*/
			      bdata = b2;
			   } else if (b3 == 0xff) { /* FF is reserved */
			      bdata = b2;
			   } else {  /* normal case */
			      bdata = b3;
			   }
			   j = bdata & 0x3f;
			   /* Now i==data1(lo nib) for memstr, j==DIMM index */
			   if (i == 0) sev = SEV_MIN; /*correctable ECC*/
		 	   else sev = SEV_MAJ;
			   if (fdebug) printf("DIMM(%d) vend=%x prod=%x\n",j,vend,prod);
			   msz = sizeof(mdesc);
			   /* For Intel S5500/S2600 see decode_mem_intel */
			   if (vend == VENDOR_INTEL) {
			     decode_mem_intel(prod,b2,b3,mdesc,&msz);
			     sprintf(mystr,"%s%c %s",mem_str(i),bcomma,mdesc);
			   } else if ((vend == VENDOR_SUPERMICRO) ||
				      (vend == VENDOR_SUPERMICROX)) {
			     decode_mem_supermicro(prod,b2,b3,mdesc,&msz);
			     sprintf(mystr,"%s%c %s",mem_str(i),bcomma,mdesc);
			   } else {  /*decode_mem_raw*/
			     decode_mem_default(psel->event_data1,b2,b3,etype,mdesc,&msz);
			     sprintf(mystr,"%s%c %s",mem_str(i),bcomma,mdesc);
			     //old: sprintf(mystr,"%s%c DIMM[%d]",mem_str(i),bcomma,j);
			     /* DIMM[2] = 3rd one (zero-based index) */
			   }
			}
			pstr = mystr;
		        break;
		     case 0x0F:    /*System Firmware events, incl POST Errs*/
			sev = SEV_MAJ; /*usu major, but platform-specific*/
			switch (psel->event_data1 & 0x0f)
			{ 
			   case 0x00:  /* System firmware errors */
				i = psel->event_data2;
				if (i > NFWERRS) i = NFWERRS;
        		        pstr = fwerrs[i].msg;
				break;
			   case 0x01:  /* System firmware hang  */
				i = psel->event_data2;
				if (i > NFWSTAT) i = NFWSTAT;
				sprintf(poststr,"hang%c %s",bcomma,
					fwstat[i].msg);
				pstr = poststr;
				break;
			   case 0x02:  /* System firmware progress */
			        sev = SEV_INFO; 
				i = psel->event_data2;
				if (i > NFWSTAT) i = NFWSTAT;
				sprintf(poststr,"prog%c %s",bcomma,
					fwstat[i].msg);
				pstr = poststr;
				break;
			   case 0xa0:  /* OEM post codes */
				/* OEM post codes in bytes 2 & 3 (lo-hi) */
				j = psel->event_data2 |
				    (psel->event_data3 << 8);
				/* interpret some OEM post codes if -e */
				i = decode_post_oem(vend,prod,(ushort)j,
						poststr,sizeof(poststr));
				pstr = poststr;
				break;
			   default:  
			        pstr = get_misc_desc( psel->generator_id,
						psel->sensor_type,
						psel->sensor_number,
						psel->event_trigger,
						psel->event_data1,
						psel->event_data2,
						psel->event_data3, &sev);
				if (pstr == NULL) 
				   pstr = "POST Event";  /*default string*/
			}  /*end switch(data1)*/
		        break;
		     case 0x13:   /*Crit Int*/
			sev = SEV_CRIT;
			i = psel->event_data1 & 0x0f;
			if (i >= NCRITS) i = NCRITS - 1;
			pstr = crit_int_str[i];
			if ((psel->event_trigger == 0x70) ||
			    (psel->event_trigger == 0x71)) { 
			    uchar bus,dev,func;
			    /* Intel AER< decode PCI bus:dev.func data */
			    bus = psel->event_data2;
			    dev = ((psel->event_data3 & 0xf8) >> 3);
			    func = (psel->event_data3 & 0x07);
			    sprintf(mystr,"%s (on %02x:%02x.%d)",
					crit_int_str[i],bus,dev,func);
			    pstr = mystr;
			}
		        break;
		     case 0x15:   /*Board (e.g. IO Module)*/
			if ((psel->event_trigger == 0x08) || 
			    (psel->event_trigger == 0x88)) {
		           rv = decode_presence(psel->event_trigger,
					psel->event_data1, mystr, &sev);
			   pstr = mystr;
			} else pstr = NULL; /*falls through to unknown*/
		        break;
		     case 0x16:   /*Microcontroller (e.g. ME or HDD)*/
			// if (psel->event_trigger == 0x0A)  // Availability
			{
			   i = psel->event_data1 & 0x0f;
			   if (i >= N_AVAIL) i = N_AVAIL - 1;
			   if (i >= 4) sev = SEV_MIN;
			   else sev = SEV_INFO;
			   pstr = avail_str[i];
			}
		        break;
		     case 0x1D:   /*System Boot Initiated*/
			i = psel->event_data1 & 0x0f;
			if (i >= NBOOTI) i = NBOOTI - 1;
			pstr = boot_init_str[i];
		        break;
		     case 0x1F:   /*OS Boot */
			i = psel->event_data1 & 0x0f;
			if (i >= NOSBOOT) i = NOSBOOT - 1;
			pstr = osboot_str[i];
		        break;
		     case 0x21:   /*Slot/Con*/
			i = psel->event_data1 & 0x0f;
			if (i >= NSLOTC) i = NSLOTC - 1;
			if (i == 0) sev = SEV_MAJ; /*Fault*/
			else if (i == 8) sev = SEV_MIN;
			sprintf(mystr,"%s",slot_str[i]);
			/* could also decode data2/data3 here if valid */
			pstr = mystr;
		        break;
		     case 0x22:   /*ACPI Power state*/
			i = psel->event_data1 & 0x0f;
			if (i >= NACPIP) i = NACPIP - 1;
			sprintf(mystr,"%s",acpip_str[i]);
			pstr = mystr;
		        break;
		     case 0x28:   /*Management Subsystem Health*/
			i = psel->event_data1 & 0x0f;
			if (i == 0x04)   /*sensor error*/
			   sprintf(mystr,"Sensor %02x fault",psel->event_data2);
			else 
			   sprintf(mystr,"Other FW HAL error");
			pstr = mystr; 
		        break;
		     case 0x29:   /*Battery*/
			if (is_threshold(psel->event_trigger,
					 psel->generator_id)) 
			{
			   pstr = get_misc_desc( psel->generator_id,
						psel->sensor_type,
						psel->sensor_number,
						psel->event_trigger,
						psel->event_data1,
						psel->event_data2,
						psel->event_data3, &sev);
			} else {
			   i = psel->event_data1 & 0x0f;
			   if (i >= NBATT) i = NBATT - 1;
			   /* sev defaults to SEV_INFO */
			   if (psel->event_trigger & 0x80) { /*deasserted*/
				sprintf(mystr,"%s",batt_clr[i]);
			   } else {  /*asserted*/
				sprintf(mystr,"%s",batt_str[i]);
				if (i == 0) sev = SEV_MIN;
				else if (i == 1) sev = SEV_MAJ;
			   }
			   pstr = mystr;
			}
		        break;
		     case 0x2A:   /*Session Audit, new for IPMI 2.0*/
			i = psel->event_data1 & 0x0f;
			if (i >= NAUDIT) i = NAUDIT - 1;
			sprintf(mystr,"%s User%d",audit_str[i],
					psel->event_data2);
			    /* see also psel->event_data3 for cause/channel*/
			pstr = mystr;
		        break;
		     case 0xDC:   /*ME Node Manager, for S5500 Urbanna*/
			i = psel->event_trigger;
			if (i & 0x80) { fdeassert = 1; i &= 0x7f; }
			if (i >= 0x72) i -= 0x72;
			if (i >= N_NM) i = N_NM - 1;
			sprintf(mystr,"%s",nm_str[i]);
			if (fdeassert) strcat(mystr," OK");
			n = strlen_(mystr);
			sprintf(cstr,"%c ",bcomma);
                        j = psel->event_data2;
			switch(i) {
			   case 0x00: /*0x72 NM Exception*/
				j = psel->event_data1;
				if ((j & 0x08) == 0) {
				   k = j & 0x03;
				   sprintf(&mystr[n],
					   "%sThreshold %d Exceeded",cstr,k);
				} else {  /*Policy event*/
				   sprintf(&mystr[n],
					   "%sPolicy Time Exceeded",cstr);
				}
				sev = SEV_MIN;
				break;
			   case 0x01: /*0x73 NM Health*/
				j &= 0x0f; // if (j >= 0x10) j -= 0x10;
				if (j >= N_NMH) j = N_NMH - 1;
				if (j != 5) sev = SEV_MAJ;
				strcat(mystr,cstr); /*", "*/
				strcat(mystr,nmh_str[j]);
				break;
			   case 0x03: /*0x75 FW Health*/
				if (j >= N_NMFW) j = N_NMFW - 1;
				sev = SEV_MAJ;
				strcat(mystr,cstr); /*", "*/
				strcat(mystr,nmfw_str[j]);
				break;
			   case 0x02: /*0x74 NM Capabilities*/
			   default:
				sev = SEV_MIN;
				break;
			}
			pstr = mystr;
		        break;
		     case 0x04:  /*Fan sensor events */
			if ((psel->event_trigger == 0x0b) ||
			    (psel->event_trigger == 0x8b)) {
		           rv = decode_redund(psel->event_trigger,
					psel->event_data1, mystr, &sev);
			   pstr = mystr;
		           break;
			} else if ((psel->event_trigger == 0x08) || 
			           (psel->event_trigger == 0x88)) {
		           rv = decode_presence(psel->event_trigger,
					psel->event_data1, mystr, &sev);
			   pstr = mystr;
		           break;
			} else if (psel->event_trigger == 0x06) {
			   /* usu psel->event_data1 == 0x01 */
			   sev = SEV_MIN;
			   strcpy(mystr,"Performance Lags");
			   pstr = mystr;
		           break;
			} else if (psel->event_trigger == 0x86) {
			   sev = SEV_INFO;
			   strcpy(mystr,"Performance OK");
			   pstr = mystr;
		           break;
			} 
		  	/* else Fan threshold events handled below, trig=01/81*/
		     default:   /* all other sensor types, see sens_desc */
			pstr = get_misc_desc( psel->generator_id,
				psel->sensor_type,
				psel->sensor_number,
				psel->event_trigger,
				psel->event_data1,
				psel->event_data2,
				psel->event_data3, &sev);
		        break;
		    }  /*end switch(sensor_type)*/

		    if (pstr == NULL) {  /* none found, unknown */
			mystr[0] = '-'; mystr[1] = 0;
			pstr = mystr;
		    }


		    /*firmware timestamp is #seconds since 1/1/1970 localtime*/
		    evtime = (uchar *)&psel->timestamp;
		    eventTime = evtime[0] + (evtime[1] << 8) + 
		   		(evtime[2] << 16) + (evtime[3] << 24);
		    if (fdebug) {  
			char tbuf[40];
			char *tz;  char *lctime;
			strftime(timebuf,sizeof(timebuf), "%x %H:%M:%S %Z",
				localtime(&eventTime));
			strftime(tbuf,sizeof(tbuf), "%x %H:%M:%S %Z",
				gmtime(&eventTime));
			tz = getenv("TZ");
			if (tz == NULL) tz = "";
			lctime = getenv("LC_TIME");
			if (lctime == NULL) lctime = "";
			SELprintf("%s\nTZ=%s, LC_TIME=%s, gmtime=%s\n",
				timebuf,tz,lctime,tbuf);
		    }
		    psensstr = NULL;
		    sensstr[0] = 0;
		    if (fsensdesc) {
			rv = get_sensor_tag(isdr,psel->generator_id,
					psel->sensor_number, sensstr,
					sdr, sizeof(sdr));
			if (rv == 0) {
			    fhave_sdr = 1;
		            psensstr = &sensstr[0]; 
			} else rv = 0;  /*no sdr but not an error*/
		    }   /*endif fsensdesc*/

		    if (is_threshold(psel->event_trigger,psel->generator_id)) {
      		      /* Also usually ((psel->event_data1 & 0x50) == 0x50) */
      		      /* We know that these two MCs should include the 
		       * actual and threshold raw values in data2 & data3 */
		      if (fsensdesc && fhave_sdr) {
			  double v1, v2;
			  char *u;
			  /* if sdrcache, find_sdr_by_snum got the sdr above */
			  /* else, GetSDR tried to get the sdr above */
			  v1 = RawToFloat(psel->event_data2,sdr); 
			  v2 = RawToFloat(psel->event_data3,sdr); 
			  u = get_unit_type(sdr[20],sdr[21],sdr[22],1);
		          sprintf(datastr, "actual=%.2f %s, threshold=%.2f %s",
				 	v1,u, v2,u);
		      } else { // if (fsensdesc == 0 || (rv != 0)) {
		          sprintf(datastr,"act=%02x thr=%02x",
				  psel->event_data2,    /*actual raw reading*/
				  psel->event_data3 );  /*threshold raw value*/
		      }
		    } else { 
		      if (fcanonical) datastr[0] = 0;
		      else sprintf(datastr,"%02x [%02x %02x %02x]",
				psel->event_trigger,
				psel->event_data1,
				psel->event_data2,
				psel->event_data3 );
		    }

		    format_event(psel->record_id, eventTime, sev, 
			psel->generator_id,
			get_sensor_type_desc(psel->sensor_type),
			psel->sensor_number, psensstr,
			pstr, datastr, outbuf,szbuf);

	}  /*endif type 2 */
	else {  /* other misc record type */
		   if (fdebug) printf("Unrecognized record type %02x\n", 
					psel->record_type);
		   rv = ERR_NOT_FOUND;
		   pc = (uchar *)&psel->record_type; 
		   sprintf(outbuf,"%04x Type%02x %s ", 
			psel->record_id,pc[0],get_sev_str(sev));
		   j = strlen_(outbuf);
		   for (i = 1; i < 14; i++) {
			sprintf(mystr,"%02x ",pc[i]);
			strcat(outbuf,mystr);
		   }
		   strcat(outbuf,"\n");
	}  /*endif misc type*/
   return(rv);
}  /*end decode_sel_entry()*/

static void show_usage(void)
{
    printf("Usage: %s [-bdfhprstux] 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10\n",progname);
    printf("where -b = interpret Binary raw SEL file, from ipmitool sel writeraw\n");
    printf("      -d = get DeviceID for vendor/product-specific events\n");
    printf("      -f = interpret File with raw ascii SEL data, from ipmiutil sel -r\n");
    printf("      -h = interpret Hex binary raw SEL file (same as -b)\n");
#ifndef ALONE
    printf("      -o = specify the target vendor IANA number.\n");
#endif
    printf("      -n = generate New platform event, use last 9 event bytes\n");
    printf("      -p = decode PET event bytes, use 34 PET data bytes,\n");
    printf("           skipping the first 8 of the 47-byte PET data (-t=all).\n");
    printf("           If not specified, assumes a 16-byte IPMI event.\n");
    printf("      -r = interpret RAW ascii SEL file (same as -f)\n");
    printf("      -s = sensor file with output of 'ipmiutil sensor', used\n");
    printf("           to get the PET sensor_type from the sensor_num.\n");
    printf("           The default is %s\n",sensfil);
    printf("      -t = decode PET trap bytes, use all 47 PET data bytes (-p=34)\n");
    printf("           If not specified, assumes a 16-byte IPMI event.\n");
    printf("      -u = use raw UTC time\n");
    printf("      -x = show eXtra debug messages\n");
}

/* 
 * decode_raw_sel
 * input parameters:  
 * raw_file : filename of raw SEL file
 * mode     : 1 = ascii raw from ipmiutil sel -r
 *            2 = binary hex from ipmiutil sel writeraw {raw_file}
 */
int decode_raw_sel(char *raw_file, int mode)
{
      FILE *fp;
      char buff[256];
      char msg[132];
      uchar hbuf[50];
      int fvalid = 0;
      int len, i;

      fp = fopen(raw_file,"r");
      if (fp == NULL) {
	 printf("Cannot open file %s\n",raw_file);
	 return(ERR_FILE_OPEN);
      } else {
         printf("%s",evt_hdr); /*"RecId Date/Time_______*/
	 if (mode == 1) { /*ascii raw*/
	    if (fdebug)
                printf("decoding raw ascii file with IPMI event bytes\n");
            while (fgets(buff, 255, fp)) {
   	       len = strlen_(buff);
   	       fvalid = 0;
   	       if (buff[0] >= '0' && (buff[0] <= '9')) fvalid = 1;
   	       else if (buff[0] >= 'a' && (buff[0] <= 'f')) fvalid = 1;
   	       else if (buff[0] >= 'A' && (buff[0] <= 'F')) fvalid = 1;
   	       if (fvalid == 0) continue;
   	       for (i = 0; i < 16; i++) {
                  hbuf[i] = _htoi(&buff[i*3]);
   	       }
               decode_sel_entry(hbuf,msg,sizeof(msg));
               printf("%s", msg);
            } /*end while*/
	 } else {  /*hex raw*/
	    if (fdebug)
		printf("decoding binary hex file with IPMI event bytes\n");
            while (fread(hbuf, 1, 16, fp) == 16) {
               decode_sel_entry(hbuf,msg,sizeof(msg));
               printf("%s", msg);
	    } /*end while*/
	 }
	 fclose(fp);
      }
      return(0);
}

/*
 * The events utility interprets standard 16-byte IPMI events into
 * human-readable form by default. 
 * User must pass raw 16-byte event data from some log text for decoding.
 *
 * Sample IPMI event usage:
 * # ievents fb 07 02 e5 1a b8 44 21 00 03 1d 9a 6f 40 8f ff
 * RecId Date/Time_______ SEV Src_ Evt_Type___ Sens# Evt_detail - Trig [Evt_data]
 * 07fb 07/14/06 18:29:57 INF SMI  System Boot Initiated #9a Power Up  6f [40 8f ff]
 * # ievents 14 04 02 BE 35 13 45 33 40 04 0C 08 6F 20 00 04
 * RecId Date/Time_______ SEV Src_ Evt_Type___ Sens# Evt_detail - Trig [Evt_data]
 * 0414 09/21/06 21:00:46 MIN 4033 Memory #08 Correctable ECC, DIMM[4] 6f [20 00 04]
 *
 * If fPET, interpret the SNMP Platform Event Trap hex varbind data.
 * For interpreting the 47-byte hex data from SNMP Platform Event Traps,
 * specify events -p with the 34 data bytes following the 16-byte GUID.
 * 
 * Sample SNMP PET Data for events:
 * 0000: 3C A7 56 85 08 C5 11 D7 C3 A2 00 04 23 BC AC 12 
 * 0010: 51 14 11 72 38 58 FF FF 20 20 00 10 83 07 01 41 
 * 0020: 0F FF 00 00 00 00 00 19 00 00 01 57 00 22 C1    
 *
 * Sample events -p command from above data:
 * # ievents -p C3 A2 00 04 23 BC AC 12 51 14 11 72 38 58 FF FF 20 20 00 10 83 07 01 41 0F FF 00 00 00 00
 * RecId Date/Time_______ SEV Src_ Evt_Type___ Sens# Evt_detail - Trig [Evt_data]
 * 0014 04/11/07 12:03:20 INF BMC  System Event #83 OEM System Booted 6f [41 0f ff]
 *
 * Platform Event Trap Format
 * Offset Len  Meaning
 *   0    16   System GUID 
 *  16     2   Sequence Number/Cookie 
 *  18     4   Local Timestamp 
 *  22     2   UTC Offset 
 *  24     1   Trap Source Type (usu 0x20)
 *  25     1   Event Source Type (usu 0x20)
 *  26     1   Event Severity
 *  27     1   Sensor Device
 *  28     1   Sensor Number
 *  29     1   Entity
 *  30     1   Entity Instance
 *  31     8   Event Data (8 bytes max, 3 bytes used)
 *  39     1   Language Code (usu 0x19/25. = English)
 *  40     4   Manufacturer IANA number (Intel=0x000157)
 *  44    1-N  OEM data, C1="No more fields"
 * If Intel (0x000157):
 *  44     2   Product ID 
 *  46     1   0xC1  (No more fields)
 */
#if defined(ALONE)
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int  argc, char **argv)
#else 
/* #elif defined(METACOMMAND) or if libipmiutil */
int i_events(int argc, char **argv)
#endif
{
   uchar buf[50];
   uchar msg[132];
   uchar *pmsg;
   int i, j, len;
   char fPET = 0;
   char frawfile = 0;
   char fhexfile = 0;
   int rv = 0;
   char c;
   uchar b = 0;
#ifndef ALONE
   uchar devid[16];
#endif
#if defined(METACOMMAND)
   char *p;
#endif
   
   printf("%s version %s\n",progname,progver);
   if (argc > 0) { argc--; argv++; } /*skip argv[0], program name*/
   /* ievents getopt:  [ -bdfhnoprstux -NPRUEFJTVY */
   while ((argc > 0) && argv[0][0] == '-') 
   { 
      c = argv[0][1];
      switch(c) {
        case 'x':  fdebug = 1; break;
	case 'd': fgetdevid = 1; break; /*get device id (vendor, product)*/
	case 'n': fnewevt = 1; break;  /* generate New event */
        case 'p':  /* PET format, minus first 8 bytes*/
          /* This is important for some SNMP trap receivers that obscure 
           * the first 8 bytes of the trap data */
          fPET = 1; /*incoming data is in PET format*/
	  break;
	case 'u': futc = 1; break;  /*use raw UTC time*/
	case 'M':  /* Set manufacturer IANA */
	case 'o':  /*specify OEM IANA manufacturer id */
          if (argc > 1) { /*next argv is IANA number */
             iopt = atoi(argv[1]);
             printf("setting IANA to %d (%s)\n",iopt,get_iana_str(iopt));
             set_iana(iopt);
             argc--; argv++;
          } else {
             printf("option -%c requires an argument\n",c);
             rv = ERR_BAD_PARAM;
          }
	  break;
	case 't':  /*PET format Trap, use all data*/
          /* This may be helpful if all bytes are available, or if 
           * the GUID is relevant. */
          fPET = 1; /*incoming data is in PET format*/
	  pet_guid = 16; 
	  break;
	case 'f':
	case 'r':
	  /* interpret raw ascii SEL file from optarg */
	  frawfile = 1;
          if (argc > 1) { /*next argv is filename*/
             len = strlen_(argv[1]);
             if (len >= sizeof(rawfil)) 
                len = sizeof(rawfil) - 1;
             strncpy(rawfil,argv[1],len);
             rawfil[len] = 0;  /*stringify*/
             argc--; argv++;
          } else {
	     printf("option -%c requires a filename argument\n",c);
	     rv = ERR_BAD_PARAM;
	  }
	  break;
	case 'b':
	case 'h':
	  /* interpret raw binary/hex SEL file from optarg */
	  fhexfile = 1;
          if (argc > 1) { /*next argv is filename*/
             len = strlen_(argv[1]);
             if (len >= sizeof(rawfil)) 
                len = sizeof(rawfil) - 1;
             strncpy(rawfil,argv[1],len);
             rawfil[len] = 0;  /*stringify*/
             argc--; argv++;
          } else {
	     printf("option -%c requires a filename argument\n",c);
	     rv = ERR_BAD_PARAM;
          }
	  break;
	case 's':  /* get sensor file from optarg */
          if (argc > 1) { /*next argv is filename*/
	     FILE *fp;
             len = strlen_(argv[1]);
             if (len >= sizeof(sensfil)) 
                len = sizeof(sensfil) - 1;
             strncpy(sensfil,argv[1],len);
             sensfil[len] = 0;  /*stringify*/
	     fp = fopen(sensfil,"r");
             if (fp == NULL) {
		printf("cannot open file %s\n",sensfil);
		rv = ERR_FILE_OPEN;
	     } else fclose(fp);
             argc--; argv++;
          } else {
	     printf("option -%c requires a filename argument\n",c);
	     rv = ERR_BAD_PARAM;
          }
	  fsensdesc = 2;
	  break;
#if defined(METACOMMAND)
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
	  if (c == 'Y' || c == 'E') p = NULL;
	  else if (argc <= 1) {
	     printf("option -%c requires an argument\n",c);
	     rv = ERR_BAD_PARAM;
          } else { /* has an optarg */
	     p = argv[1];
             argc--; argv++;
	  }
	  if (rv == 0) parse_lan_options(c,p,fdebug);
	  break;
#endif
	default:  /*unknown option*/
          printf("Unknown option -%c\n",c);
          show_usage();
          rv = ERR_USAGE;
          goto do_exit;
	  break;
      }
      argc--; argv++;   
   }  /*end while options*/

   len = argc;  /*number of data bytes*/
   if (!fPET && len > 16) len = 16;  /* IPMI event max is 16 */
   if (frawfile || fhexfile) len = 0;
   else if (fnewevt) {
      if (len < 9) {
         printf("Need 9 bytes for a New event, got %d bytes input\n",len);
         show_usage();
         rv = ERR_BAD_PARAM;
      } 
   } else if (len < 16) {
      printf("Need 16 bytes for an IPMI event, got %d bytes input\n",len);
      show_usage();
      rv = ERR_BAD_PARAM;
   }
   if (rv != 0) goto do_exit;

#ifndef ALONE
   if (fgetdevid || fsensdesc) 
      rv = ipmi_getdeviceid( devid, sizeof(devid),fdebug); /*sets mfgid*/
#endif

   for (i = 0; i < len; i++)
   {
      if (fPET) msg[i] = _htoi(argv[i]);
      else buf[i] = _htoi(argv[i]);
   }
   if (fPET != 0)   /*PET, reorder bytes to std event format*/
   {
      uchar snum, styp;
      int  timestamp;
      int  yrs, time2; 
      char sensdesc[100];
      int mfg;

      pmsg = &msg[pet_guid];  /*pet_guid=8, skip the GUID*/
      if (fdebug) {
         printf("decoding IPMI PET event bytes\n");
	 dump_buf("PET buffer",msg,len,1);
      }
      /* pmsg[ 9] is event source type (gen id, usu 0x20) */
      /* pmsg[10] is event severity */
      /* pmsg[11] is sensor device */
      /* pmsg[12] is sensor number */
      /* pmsg[13] is Entity */
      snum = pmsg[12];
      styp = entity2sensor_type(pmsg[13]);
      rv = get_sensdesc(pmsg[9],snum,sensdesc,&i,NULL);
      if (rv == 0) {
	  styp = (uchar)i;
          if (fdebug) printf("sensor[%02x]: %s\n",snum,sensdesc);
	  set_sel_opts(2,0, NULL,fdebug,futc);
      } else {
	  if (rv == ERR_NOT_FOUND) {
	      printf("Cannot find snum %02x in %s\n",snum,sensfil);
	      printf("Resolve this by doing 'ipmiutil sensor >sensorX.txt' "
		     "on a system similar\nto the target, then use "
		     "'ipmiutil events -s sensorX.txt ...'\n");
	  }
          /* Try GetSensorType(), which will work if local IPMI. */
          rv = GetSensorType(snum,&b,NULL);
          if (fdebug) printf("sensor[%02x]: GetSensorType rv=%d stype=%x\n",
				snum,rv,b);
          if (rv == 0) styp = b;
      }

      buf[0] = pmsg[1];   /*record id (sequence num)*/
      buf[1] = 0;        /*  was pmsg[0]; */
      buf[2] = 0x02;     /*event type*/
#ifdef RAW
      buf[3] = pmsg[5];   /*timestamp*/
      buf[4] = pmsg[4];   /*timestamp*/
      buf[5] = pmsg[3];   /*timestamp*/
      buf[6] = pmsg[2];   /*timestamp*/
#else
      timestamp = pmsg[5] + (pmsg[4] << 8) + (pmsg[3] << 16) + (pmsg[2] << 24);
      /* add 28 years, includes 7 leap days, less 1 hour TZ fudge */
      // yrs = ((3600 * 24) * 365 * 28) + (7 * (3600 * 24)) - 3600;
      yrs = 0x34aace70;
      time2 = timestamp + yrs;
      if (fdebug) 
          printf("timestamp: %08x + %08x = %08x\n",timestamp,yrs,time2);
      buf[3] = time2  & 0x000000ff;          /*timestamp*/
      buf[4] = (time2 & 0x0000ff00) >> 8;   /*timestamp*/
      buf[5] = (time2 & 0x00ff0000) >> 16;  /*timestamp*/
      buf[6] = (time2 & 0xff000000) >> 24;  /*timestamp*/
#endif
      buf[7] = pmsg[9];   /*generator_id*/
      buf[8] = 0;
      buf[9] = 0x04;     /*evm_rev*/
      buf[10] = styp;    /*derived sensor type, from above */
      buf[11] = snum;    /*sensor number*/
      /* set the event trigger based on context */
      switch(styp) {  /*set the event trigger*/
	case 0x12: buf[12] = 0x6f; break; /*system event (sensor-specific)*/
	case 0x09: buf[12] = 0x0b; break; /*Power Unit*/
	case 0x01:    /*temp*/
	case 0x02:    /*voltage*/
	case 0x03:    /*current*/
	case 0x04:    /*fan*/
	    if (pmsg[10] == 0x04) buf[12] = 0x81; /*info severity, ok*/
	    else buf[12] = 0x01;  /* threshold asserted*/
	    break;
        default: 
	    buf[12] = pmsg[14];  /*event trigger = Entity Instance */
	    /*Note that Entity Instance will not match an event trigger*/
	    buf[12] = 0x6f;   /*set trigger to sensor-specific*/
	    break;
      }
      memcpy(&buf[13],&pmsg[15],3);  /*event data*/
      mfg = pmsg[27] + (pmsg[26] << 8) + (pmsg[25] << 16); 
      if (fdebug) {
	  printf("PET severity=%02x, mfgId=%02x%02x%02x%02x\n",
		 pmsg[10], pmsg[24], pmsg[25], pmsg[26], pmsg[27]);
	  dump_buf("IPMI event",buf,16,0);
      }
      if (mfg == VENDOR_SUN) {  /* Sun = 0x00002A, extra OEM data */
	 j = 28 + pet_guid;  /*offset 28+16=44 (OEM data) */
         pmsg = &msg[j]; 
         for (i = 0; (i+j) < len;  ) {
	    if (pmsg[i] == 0xC1) break;
	    if (i == 0) i += 2;  /* 2-byte header 0x0c 0x01 */
	    else if (pmsg[i] == 0x80) { /* 3-byte header, usu strings */
	        if (pmsg[i+2] == 0x03) printf("  %s\n",&pmsg[i+3]);
		i += (3 + pmsg[i+1]);
	    } else i++;
         }
      }
      decode_sel_entry(buf,(char *)msg,sizeof(msg));
      printf("%s", evt_hdr); /*"RecId Date/Time_______*/
      printf("%s", msg);
   } else if (fnewevt) {
      rv = new_event(buf,len);  /*do new platform event*/
   } else if (frawfile) {
      rv = decode_raw_sel(rawfil,1);  /*ascii raw data from file */
   } else if (fhexfile) {
      rv = decode_raw_sel(rawfil,2);  /*binary/hex raw data from file*/
   } else {
      if (fdebug) printf("decoding standard IPMI event bytes\n");
      if (fdebug) dump_buf("IPMI event",buf,16,0);
      set_sel_opts(2,0, NULL,fdebug,futc);
      rv = decode_sel_entry(buf,(char *)msg,sizeof(msg));
      /* show header for the event record */
      printf("%s", evt_hdr); /*"RecId Date/Time_______*/
      printf("%s", msg);
   }
do_exit:
#ifndef METACOMMAND
   printf("%s, %s\n",progname,decode_rv(rv)); 
#endif
   return(rv);
}
// #endif

/* end ievents.c */
