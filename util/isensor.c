/*
 * isensor.c
 *
 * This tool reads the SDR records to return sensor information. 
 * It can use either the Intel /dev/imb driver or VALinux /dev/ipmikcs.
 *
 * Author:  arcress at users.sourceforge.net
 * Copyright (c) 2002-2006 Intel Corporation. 
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 07/25/02 Andy Cress created
 * 10/09/02 Andy Cress v1.1 added decodeValue(RawToFloat) routine 
 * 10/11/02 Andy Cress v1.2 added expon routine 
 * 10/30/02 Andy Cress v1.3 added SDR types 08 & 14
 * 12/04/02 Andy Cress v1.4 changed dstatus descriptions
 * 01/29/03 Andy Cress v1.5 added MV OpenIPMI driver support
 *          Hannes Schulz <schulz@schwaar.com>
 *                1) correct raw readings to floatval
 *                2) allow extra SDR bytes returned from HP netserver 1000r
 *          Guo Min <guo.min@intel.com>
 *                add -l option for simpler list display
 * 02/25/03 Andy Cress v1.6 misc cleanup
 * 05/02/03 Andy Cress v1.7 add PowerOnHours
 * 07/28/03 Andy Cress v1.8 added -t option for threshold values,
 *                          added sample Discovery routine (unfinished),
 *                          added ipmi_getdeviceid for completeness.
 * 09/05/03 Andy Cress v1.9 show SDR OEM subtypes, 
 *                          fix GetSDR multi-part get for OEM SDRs
 *                          stop if SDR Repository is empty
 * 09/23/03 Andy Cress v1.10 Add options to set thresholds
 * 10/14/03 Andy Cress v1.11 Fixed sdr offset for ShowThreshold values
 * 01/15/04 Andy Cress v1.12 Fixed SetThreshold to set hysteresis, 
 *                           Fixed sens_cap testing in ShowThresh(Full)
 * 01/30/04 Andy Cress v1.13 Changed field display order, added header,
 *                           check for sdr sz below min, added WIN32.
 * 02/19/04 Andy Cress v1.14 Added SDR type 3 parsing for mBMC
 * 02/27/04 Andy Cress v1.15 Added check for superuser, more mBMC logic
 * 03/11/04 Andy Cress v1.16 Added & removed private mBMC code for set 
 *                           thresholds due to licensing issues
 * 04/13/04 Andy Cress v1.17 Added -r to show raw SDRs also
 * 05/05/04 Andy Cress v1.18 call ipmi_close before exit,
 *                           fix sresp in GetSDR for WIN32.
 * 07/07/04 Andy Cress v1.19 Added -a to reArm sensor,
 *                           show debug raw reading values only in hex
 * 08/18/04 Andy Cress v1.20 Added decoding for DIMM status
 * 11/01/04 Andy Cress v1.21 add -N / -R for remote nodes,  
 *                           added -U for remote username
 * 11/19/04 Andy Cress v1.22 added more decoding for compact reading types,
 *                           added -w option to wrap thresholds
 * 11/24/04 ARCress  v1.23   added sens_type to display output
 * 12/10/04 ARCress  v1.24   added support for device sdrs also,
 *                           fixed sens_cap byte,
 * 01/10/05 ARCress  v1.25   change ShowThresh order, highest to lowest,
 *                           change signed exponent type in RawToFloat
 * 01/13/05 ARCress  v1.26   added time display if fwrap
 * 01/28/05 ARCress  v1.27   mod for Power Redundancy SDR status
 * 02/15/05 ARCress  v1.28   added FloatToRaw for -h/-l threshold set funcs, 
 *                           always take -n sensor_num as hex (like displayed)
 * 03/07/05 ARCress  v1.29   added "LAN Leash Lost" decoding in decode_comp_
 *                           added -v to show max/min & hysteresis.
 * 03/22/05 ARCress  v1.30   added OEM subtype 0x60 for BMC TAM
 * 03/26/05 ARCress  v1.31   added battery type to decode_comp_reading
 * 04/21/05 ARCress  v1.32   added error message if -n sensor_num not found,
 *                           added more decoding for Power Redund sensor
 * 06/20/05 ARCress  v1.33   if GetSDRRepository cc=0xc1 switch fdevsdrs mode,
 *                           also detect fdevsdrs better for ATCA.
 * 07/28/05 ARCress  v1.34   check for Reading init state, 
 *                           add extra byte to decode_comp_reading()
 * 09/12/05 ARCress  v1.35   don't check superuser for fipmi_lan
 * 01/26/06 ARCress  v1.36   added -i option to only show one sensor index
 * 03/14/06 ARCress  v1.37   added -p option to save persistent thresholds
 * 04/06/06 ARCress  v1.38   show auto/manual rearm
 * 07/17/06 ARCress  v1.39   add -V, add -L, handle RepInfo rc=0xc1 
 * 11/28/06 ARCress  v1.46   added -c -m for ATCA child MCs
 * 08/15/07 ARCress  v1.58   filter display if -n sensor_num
 * 08/29/07 ARCress  v1.59   fixed Battery sensor interpretation
 * 10/31/07 ARCress  v2.3    retry GetSDR if cc=0xC5 (lost reservationID)
 * 01/14/08 ARCress  v2.6    add -u param for setting unique thresholds, 
 *                           always show float when setting thresholds,
 *                           fixup in decoding Proc,PS Comp readings
 * 01/25/08 ARCress  v2.7    allow float input with -u thresholds, 
 *                           add -p persist logic for -u thresholds.
 * 09/20/19 ARCress  v3.15   workaround for Pigeon Point bad sa in SDR
 * 07/08/22 ARCress  v3.19   fix -i get_idx_range to show last idx in range
 */
/*M*
Copyright (c) 2002-2006 Intel Corporation. 
Copyright (c) 2009 Kontron America, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 
  c.. Neither the name of Intel nor the names of its contributors 
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
#include <math.h>   // for:  double pow(double x, double y);
#include <string.h>
#include <time.h>
#ifdef WIN32
#include <windows.h>
#include "getopt.h"
#elif defined(DOS)
#include <dos.h>
#include "getopt.h"
#else
#include <sys/stat.h>
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#else
#include <getopt.h>
#endif
#endif
#if defined(LINUX)
#include <unistd.h>
#include <sys/types.h>
#endif
#include "ipmicmd.h"
#include "isensor.h"

#define PICMG_CHILD  1 /* show child MCs if -b */
#define MIN_SDR_SZ  8 
#define SZCHUNK 16    /* SDR chunksize was 8, now 16 */
#define INIT_SNUM  0xff 
#define N_SGRP   16
#define THR_EMPTY  999

/* prototypes */
int get_LastError( void );  /* ipmilan.c */
extern int use_devsdrs(int picmg);  /* ipmicmd.c */
extern int get_MemDesc(int array, int dimm, char *desc, int *psz); /*mem_if.c*/
extern char *get_sensor_type_desc(uchar stype); /*ievents.c*/
#ifdef METACOMMAND
#include "oem_intel.h"
/*     void show_oemsdr_intel(uchar *sdr); in oem_intel.h */
/*     int decode_sensor_intel();          in oem_intel.h */
/*     int is_romley(int vend, int prod);  in oem_intel.h */
extern int decode_sensor_kontron(uchar *sdr,uchar *reading,char *pstring,
				int slen);  /*see oem_kontron.c*/
extern int decode_sensor_fujitsu(uchar *sdr,uchar *reading,char *pstring,
				int slen);  /*see oem_fujitsu.c*/
extern int decode_sensor_sun(uchar *sdr,uchar *reading,char *pstring,
				int slen);  /*see oem_sun.c*/
extern int decode_sensor_supermicro(uchar *sdr,uchar *reading,char *pstring,
				int slen, int fsimple, char fdbg);  /*see oem_supermicro.c*/
extern int decode_sensor_quanta(uchar *sdr,uchar *reading,char *pstring,
				int slen);  /*see oem_quanta.c*/
extern int decode_sensor_dell(uchar *sdr,uchar *reading,char *pstring,
				int slen);  /*see oem_dell.c*/
extern int decode_sensor_lenovo(uchar *sdr,uchar *reading,char *pstring,
				int slen);  /*see oem_lenovo.c*/
extern int decode_sensor_asus(uchar *sdr,uchar *reading,char *pstring,int slen);
extern int decode_sensor_hp(uchar *sdr,uchar *reading,char *pstring,
				int slen);  /*see oem_hp.c*/
extern void show_oemsdr_hp(uchar *sdr); 
#else
int is_romley(int vend, int prod) { 
   if ((vend == VENDOR_INTEL) && ((prod >= 0x0048) && (prod <= 0x005e)))
	return(1);
   return(0); 
}
int is_grantley(int vend, int prod) { 
   if ((vend == VENDOR_INTEL) && (prod == 0x0071))
	return(1);
   return(0); 
}
int is_thurley(int vend, int prod) { 
   if ((vend == VENDOR_INTEL) && ((prod >= 0x003A) && (prod <= 0x0040)))
	return(1);
   return(0); 
}
#endif
#ifdef ALONE
#define NENTID  53
static char *entity_id_str[NENTID] = {
/* 00 */ "unspecified",
/* 01 */ "other",
/* 02 */ "unknown",
/* 03 */ "processor",
/* 04 */ "disk",
/* 05 */ "peripheral bay",
/* 06 */ "management module",
/* 07 */ "system board",
/* 08 */ "memory module",
/* 09 */ "processor module",
/* 10 */ "power supply",
/* 11 */ "add-in card",
/* 12 */ "front panel bd",
/* 13 */ "back panel board",
/* 14 */ "power system bd",
/* 15 */ "drive backplane",
/* 16 */ "expansion board",
/* 17 */ "Other system board",
/* 18 */ "processor board",
/* 19 */ "power unit",
/* 20 */ "power module",
/* 21 */ "power distr board",
/* 22 */ "chassis back panel bd",
/* 23 */ "system chassis",
/* 24 */ "sub-chassis",
/* 25 */ "Other chassis board",
/* 26 */ "Disk Drive Bay",
/* 27 */ "Peripheral Bay",
/* 28 */ "Device Bay",
/* 29 */ "fan",
/* 30 */ "cooling unit",
/* 31 */ "cable/interconnect",
/* 32 */ "memory device ",
/* 33 */ "System Mgt Software",
/* 34 */ "BIOS",
/* 35 */ "Operating System",
/* 36 */ "system bus",
/* 37 */ "Group",
/* 38 */ "Remote Mgt Comm Device",
/* 39 */ "External Environment",
/* 40 */ "battery",
/* 41 */ "Processing blade",
/* 43 */ "Processor/memory module",
/* 44 */ "I/O module",
/* 45 */ "Processor/IO module",
/* 46 */ "Mgt Controller Firmware",
/* 47 */ "IPMI Channel",
/* 48 */ "PCI Bus",
/* 49 */ "PCI Express Bus",
/* 50 */ "SCSI Bus",
/* 51 */ "SATA/SAS bus",
/* 52 */ "Processor FSB"
};
char *decode_entity_id(int id) { 
   if (id < NENTID) return (""); 
   else return(entity_id_str[id]); }
#else
/* char *decode_entity_id(int id);  *isensor.h, from ievents.c*/
#endif
/************************
 *  Global Data
 ************************/
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil sensor";
#else
static char * progver   = "3.15";
static char * progname  = "isensor";
#endif
#ifdef WIN32
static char savefile[] = "%ipmiutildir%\\thresholds.cmd";
#else
static char savefile[] = "/var/lib/ipmiutil/thresholds.sh";
// static char savefile[] = "/usr/share/ipmiutil/thresholds.sh";
#endif
extern char fdebug;      /*from ipmicmd.c*/
int sens_verbose    = 0; /* =1 show max/min & hysteresis also */
static int fdevsdrs = 0;
static int fReserveOK = 1;
static int fDoReserve = 1;
static int fsimple     = 0; /*=1 simple, canonical output*/
static int fshowthr = 0;   /* =1 show thresholds, =2 show thr in ::: fmt */
static int fwrap    = 0;
static int frawsdr  = 0;
static int frearm   = 0;
static int fshowidx = 0;   /* only show a specific SDR index/range */
static int fshowgrp = 0;   /* =1 show group of sensors by sensor type */
static int fdoloop  = 0;   /* =1 if user specified number of loops */
static int fpicmg   = 0;
static int fchild   = 0;   /* =1 show child SDRs */
static int fset_mc  = 0;   /* =1 -m to set_mc   */
static int fbadsdr  = 0;   /* =1 to ignore bad SDR mc  */
static int fdump    = 0;  
static int frestore = 0;  
static int fjumpstart = 0;  
static int fgetmem  = 0;  
static int fprivset = 0;  
static char fremote = 0;  
static int  nloops  = 1;   /* num times to show repeated sensor readings */
static int  loopsec  = 1;  /* wait N sec between loops, default 1 */
static char bdelim = BDELIM;  /* delimiter for canonical output */
static char tmpstr[20];    /* temp string */
static char *binfile = NULL;
static int   fsetthresh = 0;
static int   fsavethresh = 0;
static uchar  sensor_grps[N_SGRP] = {0, 0};  /*sensor type groups*/
static ushort sensor_idx1 = 0xffff;
static ushort sensor_idxN = 0xffff;
static uchar  sensor_num = INIT_SNUM;
static uchar  sensor_hi = 0xff;
static uchar  sensor_lo = 0xff;
static uchar  sensor_thr[6]  = {0,0,0,0,0,0};
static double sensor_thrf[6] = {0,0,0,0,0,0};
static double sensor_hi_f = 0;
static double sensor_lo_f = 0;
static int fmBMC = 0;
static int fRomley = 0;
static int fGrantley = 0;
static char chEol = '\n';  /* newline by default, space if option -w */
static uchar resid[2] = {0,0};
static uchar g_bus = PUBLIC_BUS;
static uchar g_sa  = 0;
static uchar g_lun = BMC_LUN;
static uchar g_addrtype = ADDR_SMI;
static int vend_id = 0;
static int prod_id;

/* sensor_dstatus
 * This is used to decode the sensor reading types and meanings.
 * Use IPMI Table 36-1 and 36-2 for this.
 */
#define N_DSTATUS 101
#define STR_CUSTOM     58
#define STR_OEM        71
#define STR_AC_LOST    76
#define STR_PS_FAIL    77
#define STR_PS_CONFIG  78
#define STR_HSC_OFF    79
#define STR_REBUILDING 80
#define STR_OTHER      81
static char oem_string[50] = "OEM";
static char *sensor_dstatus[N_DSTATUS] = {
/* 0 00h */ "OK  ",
/* Threshold event states */
/* 1 01h */ "Warn-lo",   // "Warning-lo",
/* 2 02h */ "Crit-lo",   // "Critical-lo",
/* 3 04h */ "BelowCrit", // "BelowCrit-lo",
/* 4 08h */ "Warn-hi",   // "Warning-hi",
/* 5 10h */ "Crit-hi",   // "Critical-hi",
/* 6 20h */ "AboveCrit", // "AboveCrit-hi",
/* 7 40h */ "Init ",  /*in init state, no reading*/
/* 8 80h */ "OK* ",
/*  Hotswap Controller event states, also Availability */
/* 9 HSC */ "Present", /*present,inserted*/
/*10 HSC */ "Absent", /*absent,removed,empty,missing*/
/*11 HSC */ "Ready",
/*12 HSC */ "Faulty",
/*  Digital/Discrete event states */
/*13 D-D */ "Asserted",
/*14 D-D */ "Deassert",
/*15 D-D */ "Predict ",
/*  Availability event states */
/*16 Avl */ "Disabled",
/*17 Avl */ "Enabled ",
/*18 Avl */ "Redundant",
/*19 Avl */ "RedunLost",
/*20 Avl */ "RedunDegr",
/* ACPI Device Power States */
/*21 ACPI*/ "Off    ", /*D3*/
/*22 ACPI*/ "Working", /*D1*/
/*23 ACPI*/ "Sleeping", /*D2/S2*/
/*24 ACPI*/ "On",      /*D0*/
/* Critical Interrupt event states */
/*25 CrI */ "FP_NMI  ",
/*26 CrI */ "Bus_TimOut",
/*27 CrI */ "IOch_NMI",
/*28 CrI */ "SW_NMI  ",
/*29 CrI */ "PCI_PERR",
/*30 CrI */ "PCI_SERR",
/*31 CrI */ "EISA_TimOut",
/*32 CrI */ "Bus_Warn ", /*Correctable*/
/*33 CrI */ "Bus_Error", /*Uncorrectable*/
/*34 CrI */ "Fatal_NMI",
/*35 CrI */ "Bus_Fatal", /*0x0A*/
/*36 CrI */ "Bus_Degraded", /*0x0B*/
/* Physical Security event states */
/*37 Phys*/ "LanLeashLost",
/*38 Phys*/ "ChassisIntrus",
/* Memory states  */
/*39 Mem */ "ECCerror",
/*40 Mem */ "ParityErr",
/* Discrete sensor invalid readings (error or init state) */
/*41 D-D */ "Unknown",
/*42 D-D */ "NotAvailable",
/* Discrete sensor OEM reading states */
/*43 OEM */ "Enabled ",
/*44 OEM */ "Disabled",
/* Session Audit states */
/*45 OEM */ "Activated ",
/*46 OEM */ "Deactivated",
/*47 HSC */ "Unused  ", 
/* Processor event states */
/*48 Proc*/ "IERR",
/*49 Proc*/ "ThermalTrip",
/*50 Proc*/ "FRB1Failure",
/*51 Proc*/ "FRB2Failure",
/*52 Proc*/ "FRB3Failure",
/*53 Proc*/ "ConfigError",
/*54 Proc*/ "SMBIOSError",
/*55 Proc*/ "ProcPresent",
/*56 Proc*/ "ProcDisabled",
/*57 Proc*/ "TermPresent",
/* Custom data string, 15 bytes */
/*58 Custom*/ "CustomData12345", 
/* Event Log */ 
/*59 EvLog*/ "MemLogDisab",
/*60 EvLog*/ "TypLogDisab",
/*61 EvLog*/ "LogCleared",
/*62 EvLog*/ "AllLogDisab",
/*63 EvLog*/ "SelFull",
/*64 EvLog*/ "SelNearFull",
/* more Digital Discrete */ 
/*65 D-D */  "Exceeded",
/*66 Alert*/ "AlertPage",
/*67 Alert*/ "AlertLAN",
/*68 Alert*/ "AlertPET",
/*69 Alert*/ "AlertSNMP",
/*70 Alert*/ "None",
/*71 OEM str*/ &oem_string[0],
/* Version Change */
/*72 Change*/ "HW Changed",
/*73 Change*/ "SW Changed",
/*74 Change*/ "HW incompatibility",
/*75 Change*/ "Change Error",
/* Power Supply event states */
/*76 PS  */   "AC_Lost  ",
/*77 PS  */   "PS_Failed",
/* Power Supply event states */
/*78 PS  */   "Config_Err",
/*79 HSC */   "Offline",
/*80 HSC */   "Rebuilding",
/*81 other*/  " _ ",
/*82 Avl */ "NonRedund_Sufficient",
/*83 Avl */ "NonRedund_Insufficient",
/*84 Usage */ "Idle",
/*85 Usage */ "Active",
/*86 Usage */ "Busy",
/*87 Trans */ "Non-Critical",
/*88 Trans */ "Critical",
/*89 Trans */ "Non-Recoverable",
/*90 Trans */ "Monitor",
/*91 Trans */ "Informational",
/*92 State */ "Running",
/*93 State */ "In_Test",
/*94 State */ "Power_Off",
/*95 State */ "Online",
/*96 State */ "Offline",
/*97 State */ "Off_Duty",
/*98 State */ "Degraded",
/*99 State */ "PowerSave",
/*100 State */ "InstallError"
};

static char *raid_states[9] = {  /*for sensor type 0x0d drive status */
  "Faulty",
  "Rebuilding",
  "InFailedArray",  
  "InCriticalArray",  
  "ParityCheck",  
  "PredictedFault",  
  "Un-configured",  
  "HotSpare",
  "NoRaid" };

#define NSENSTYPES   0x2a
#ifdef OLD
/* see ievents.c */
static const char *sensor_types[NSENSTYPES] = {  /*IPMI 2.0 Table 42-3*/
/* 00h */ "reserved",
/* 01h */ "Temperature",
/* 02h */ "Voltage",
/* 03h */ "Current",
/* 04h */ "Fan",
/* 05h */ "Platform Chassis Intrusion",
/* 06h */ "Platform Security Violation",
/* 07h */ "Processor",
/* 08h */ "Power Supply",
/* 09h */ "Power Unit",
/* 0Ah */ "Cooling Device",
/* 0Bh */ "FRU Sensor",
/* 0Ch */ "Memory",
/* 0Dh */ "Drive Slot",
/* 0Eh */ "POST Memory Resize",
/* 0Fh */ "System Firmware",
/* 10h */ "SEL Disabled",
/* 11h */ "Watchdog 1",
/* 12h */ "System Event",          /* offset 0,1,2 */
/* 13h */ "Critical Interrupt",    /* offset 0,1,2 */
/* 14h */ "Button",                /* offset 0,1,2 */
/* 15h */ "Board",
/* 16h */ "Microcontroller",
/* 17h */ "Add-in Card",
/* 18h */ "Chassis",
/* 19h */ "Chip Set",
/* 1Ah */ "Other FRU",
/* 1Bh */ "Cable / Interconnect",
/* 1Ch */ "Terminator",
/* 1Dh */ "System Boot Initiated",
/* 1Eh */ "Boot Error",
/* 1Fh */ "OS Boot",
/* 20h */ "OS Critical Stop",
/* 21h */ "Slot / Connector",
/* 22h */ "ACPI Power State",
/* 23h */ "Watchdog 2",
/* 24h */ "Platform Alert",
/* 25h */ "Entity Presence",
/* 26h */ "Monitor ASIC",
/* 27h */ "LAN",
/* 28h */ "Management Subsystem Health",
/* 29h */ "Battery",
};
#endif

#define NUNITS  30
static char *unit_types[] = {
/* 00 */ "unspecified",
/* 01 */ "degrees C",
/* 02 */ "degrees F",
/* 03 */ "degrees K",
/* 04 */ "Volts",
/* 05 */ "Amps",
/* 06 */ "Watts",
/* 07 */ "Joules",
/* 08 */ "Coulombs",
/* 09 */ "VA",
/* 10 */ "Nits",
/* 11 */ "lumen",
/* 12 */ "lux",
/* 13 */ "Candela",
/* 14 */ "kPa",
/* 15 */ "PSI",
/* 16 */ "Newton",
/* 17 */ "CFM",
/* 18 */ "RPM",
/* 19 */ "Hz",
/* 20 */ "microseconds",
/* 21 */ "milliseconds",
/* 22 */ "seconds",
/* 23 */ "minutes",
/* 24 */ "hours",
/* 25 */ "days",
/* 26 */ "weeks",
/* 27 */ "mil",
/* 28 */ "inches",
/* 29 */ "feet",
/* 42 */ "cycles"  
};
/* 68 * "megabit", */
/* 72 * "megabyte", */
/* 90 * "uncorrectable error" (last defined)*/
static char *unit_types_short[] = {
/* 00 */ "?", /*unknown, not specified*/
/* 01 */ "C",
/* 02 */ "F",
/* 03 */ "K",
/* 04 */ "V",
/* 05 */ "A",
/* 06 */ "W",
/* 07 */ "J",
/* 08 */ "Coul",
/* 09 */ "VA",
/* 10 */ "Nits",
/* 11 */ "lumen",
/* 12 */ "lux",
/* 13 */ "Cand",
/* 14 */ "kPa",
/* 15 */ "PSI",
/* 16 */ "Newton",
/* 17 */ "CFM",
/* 18 */ "RPM",
/* 19 */ "Hz",
/* 20 */ "usec",
/* 21 */ "msec",
/* 22 */ "sec",
/* 23 */ "min",
/* 24 */ "hrs",
/* 25 */ "days",
/* 26 */ "wks",
/* 27 */ "mil",
/* 28 */ "in",
/* 29 */ "ft",
/* 42 */ "cyc"  
};

ushort parse_idx(char *str)
{
    int i, n;
    char istr[5];
    if (strncmp(str,"0x",2) == 0) str += 2;
    n = strlen_(str);
    if (n == 4) {
        i = (htoi(str) << 8) + htoi(&str[2]);
    } else if (n == 3) {
        istr[0] = '0';
        memcpy(&istr[1],str,3);
        i = (htoi(istr) << 8) + htoi(&istr[2]);
    } else i = htoi(str);      /*was atoi()*/
    printf("idx = 0x%x\n",i);
    return((ushort)i);
}

int get_idx_range(char *str)
{
    // int i = 0;
    char *p;
	char *p2;
    p = strchr(str,'-');
    if (p == NULL) p = strchr(str,',');
    if (p != NULL) {  /*range*/
       *p = 0;
       p++;
       sensor_idx1 = parse_idx(str);
       sensor_idxN = parse_idx(p) + 1; /*end if >=, so + 1*/
    } else {  /*single sensor*/
       sensor_idx1 = parse_idx(str);
       sensor_idxN = sensor_idx1; /*end if >=*/
    }
    return(0);
}

char *get_unit_type(uchar iunits, uchar ibase, uchar imod, int fshort)
{
    char *pstr = NULL;
    char **punittypes;
    static char unitstr[32];
    int jbase, jmod;
    uchar umod;

    punittypes = unit_types;
    if (fshort) punittypes = unit_types_short;
    if (fdebug) printf("get_unit_type(%x,%d,%d,%d)\n",iunits,ibase,imod,fshort);
    umod = (iunits & 0x06) >> 1;
    if (ibase < NUNITS) jbase = ibase;
    else {
	if (fdebug) printf("units base %02x > %d\n",ibase,NUNITS);
        if (ibase == 42) jbase = NUNITS;  /*"cycles"*/
	else jbase = 0; 
    }
    if (imod < NUNITS) jmod = imod;
    else {
	if (fdebug) printf("units mod %02x > %d\n",imod,NUNITS);
	jmod = 0; 
    }
    switch (umod) {
	case 2:
	   snprintf(unitstr,sizeof(unitstr),"%s * %s",
			punittypes[jbase],punittypes[jmod]);
	   pstr = unitstr;
	   break;
	case 1:
	   snprintf(unitstr,sizeof(unitstr),"%s/%s",
			punittypes[jbase],punittypes[jmod]);
	   pstr = unitstr;
	   break;
	case 0:
	default:
           pstr = punittypes[jbase];
	   break;
    }
    if ((umod == 0) && (iunits > 0)) {
	   /* special cases for other SensorUnits1 bits */
	   if ((iunits & 0x01) != 0) {  /*percentage*/
	      if (fshort) pstr = "%";
	      else pstr = "percent";
	   } else if (iunits == 0xC0) {  /*no analog reading*/
	      pstr = "na";
	   } else if (iunits == 0x18) {
	      /* For Tyan fans:  base=42, units=24.(0x18) -> cycles/hour */
	      snprintf(unitstr,sizeof(unitstr),"%s/hour",punittypes[jbase]);
	      pstr = unitstr;
	   } 
    }
    return(pstr);
}

char *decode_capab(uchar c)
{
   static char cstr[50];
   char *arm;
   char *thr;
   char *evt;
   // char *hys;
   uchar b;
   /* decode sens_capab bits */
   if ((c & 0x40) == 0) arm = "man"; /*manual rearm*/
   else arm = "auto";  /*automatic rearm*/
   /* skip hysteresis bits (0x30) */
   b = ((c & 0x0c) >> 2);
   switch(b) {
   case 0x00: thr = "none"; break;  /*no thresholds*/
   case 0x01: thr = "read"; break;
   case 0x02: thr = "write"; break; /*read & write*/
   case 0x03: 
   default:   thr = "fixed"; break; 
   }
   b = (c & 0x03) ;
   switch(b) {
   case 0x00: evt = "state"; break;  /*threshold or discrete state*/
   case 0x01: evt = "entire"; break; /*entire sensor only*/
   case 0x02: evt = "disab"; break;  /*global disable only*/
   case 0x03:
   default:   evt = "none"; break;   /*no events*/
   }
   sprintf(cstr,"arm=%s thr=%s evts=%s",arm,thr,evt);
   return(cstr);
}


int get_group_id(char *pstr)
{
   int i, j, n, sz, len;
   char *p;
   int rv = -1;
		   
   sz = strlen_(pstr);
   p = &pstr[0];
   n = 0;
   for (i = 0; i <= sz; i++) {
      if (n >= N_SGRP) break;
      switch(pstr[i]) {
	 case ',':  /*delimiter*/
	 case '\n':
	 case '\0':
	    pstr[i] = 0;  /*stringify this word*/
            len = strlen_(p);
	    for (j = 0; j < NSENSTYPES; j++) {
		if (strncasecmp(get_sensor_type_desc(j),p,len) ==  0) {
		   sensor_grps[n++] = (uchar)j;
		   rv = 0;
		   break; 
		}
	    } /*endfor(j)*/
	    if (i+1 < sz) p = &pstr[i+1];  /*set p for next word*/
	    if (j >= NSENSTYPES) { /* sensor type not found */
		rv = -1; 
		i = sz;  /*exit loop*/
	    }
	    break;
	 default:
	    break;
      } /*end switch*/
   }  /*end for(i)*/
   if (rv == 0) rv = n;
   else rv = -1;
   return(rv);
}

static int validate_thresholds(void *pthrs, char flag, uchar *sdr)
{
   double *thrf;
   uchar *thr;
   int rv = 0;
   uchar bits;

   if (sdr == NULL) bits = 0xff; /*assume all are used*/
   else bits = sdr[18];  /*18=indicates which are readable/used */

   if (bits == 0) {
      printf("No threshold values can be set for this sensor.\n");
      return(3);
   }
   if (flag == 1) { /*float*/
      thrf = (double *)pthrs;
      if (fdebug) 
         printf("validate_thresh: bits=%02x, values: %f>=%f>=%f, %f<=%f<=%f\n",
		bits, thrf[0],thrf[1],thrf[2], thrf[3],thrf[4],thrf[5]);
      if ((bits & 0x02) != 0) { /*thrf[1] lo-crit is valid*/
        if ((thrf[1] > thrf[0]) && ((bits & 0x01) != 0)) rv = 1;
        if ((thrf[2] > thrf[1]) && ((bits & 0x04) != 0)) rv = 1; /*lo is wrong*/
      } 
      if ((bits & 0x10) != 0) { /*thrf[4] hi-crit is valid*/
        if ((thrf[4] < thrf[3]) && ((bits & 0x08) != 0)) rv = 2;
        if ((thrf[5] < thrf[4]) && ((bits & 0x20) != 0)) rv = 2; /*hi is wrong*/
      }
      if (rv != 0) {
	  printf("Threshold values: %f>=%f>=%f, %f<=%f<=%f\n",
		thrf[0], thrf[1], thrf[2], thrf[3], thrf[4], thrf[5]);
          printf("Invalid threshold order in %s range.\n",
				((rv == 1)? "lo": "hi"));
      }
   } else {
      thr = (uchar *)pthrs;
      if ((bits & 0x02) != 0) { /*thr[1] lo-crit is valid*/
        if ((thr[1] > thr[0]) && ((bits & 0x01) != 0)) rv = 1;
        if ((thr[2] > thr[1]) && ((bits & 0x04) != 0)) rv = 1; /*lo is wrong*/
      } 
      if ((bits & 0x10) != 0) { /*thr[4] hi-crit is valid*/
        if ((thr[4] < thr[3]) && ((bits & 0x08) != 0)) rv = 2;
        if ((thr[5] < thr[4]) && ((bits & 0x20) != 0)) rv = 2; /*hi is wrong*/
      }
      if (rv != 0) {
         printf("Threshold values: %02x>=%02x>=%02x %02x<=%02x<=%02x\n",
		thr[0], thr[1], thr[2], thr[3], thr[4], thr[5]);
         printf("Invalid threshold order within -u (%s)\n",
				((rv == 1)? "lo": "hi"));
      }
   }
   return(rv);
}

int 
GetSDRRepositoryInfo(int *nret, int *fdev)
{
   uchar resp[MAX_BUFFER_SIZE];
   int sresp = MAX_BUFFER_SIZE;
   int rc;
   int nSDR;
   int freespace;
   ushort cmd;
   uchar cc = 0;
   int   i;

   memset(resp,0,6);  /* init first part of buffer */
   if (nret != NULL) *nret = 0;
   if (fdev != NULL) fdevsdrs = *fdev;
   if (fdevsdrs) cmd = GET_DEVSDR_INFO;
   else cmd = GET_SDR_REPINFO;
   rc = ipmi_cmd_mc(cmd, NULL, 0, resp,&sresp, &cc, fdebug);
   if (fdebug) printf("ipmi_cmd[%04x] repinf(%d) status=%d cc=%x\n",
			cmd, fdevsdrs,rc,cc);
   /* some drivers return cc in rc */
   if ((rc == 0xc1) || (rc == 0xd4)) cc = rc; 
   else if (rc != 0) return(rc);
   if (cc != 0) {
      if ((cc == 0xc1) ||  /*0xC1 (193.) means unsupported command */
          (cc == 0xd4))    /*0xD4 means insufficient privilege (Sun/HP)*/
      {
         /* Must be reporting wrong bit for fdevsdrs, 
          * so switch & retry */
         if (fdevsdrs) { 
            fdevsdrs = 0;
            cmd = GET_SDR_REPINFO;
         } else {  
            fdevsdrs = 1;
            cmd = GET_DEVSDR_INFO;
         }
         sresp = MAX_BUFFER_SIZE;
         rc = ipmi_cmd_mc(cmd, NULL, 0, resp,&sresp, &cc, fdebug);
         if (fdebug) 
             printf("ipmi_cmd[%04x] repinf status=%d cc=%x\n",cmd,rc,cc);
         if (rc != ACCESS_OK) return(rc);
         if (cc != 0) return(cc);
      } else return(cc);
   }

   if (fdevsdrs) {
      nSDR = resp[0];
      freespace = 1;  
      fReserveOK = 1;
   } else {
      nSDR = resp[1] + (resp[2] << 8);
      freespace = resp[3] + (resp[4] << 8);
      if ((resp[13] & 0x02) == 0) fReserveOK = 0;
      else fReserveOK = 1;
   }
   if (nret != NULL) *nret = nSDR;
   if (fdev != NULL) *fdev = fdevsdrs;
   if (fdebug) {
      //successful, show data
      printf("SDR Repository (len=%d): ",sresp);
      for (i = 0; i < sresp; i++) printf("%02x ",resp[i]);
      printf("\n");
      printf("SDR Info: fdevsdrs=%d nSDRs=%d free space = %x ReserveOK=%d\n",
		fdevsdrs,nSDR,freespace,fReserveOK);
   }

   return(0);
}  /*end GetSDRRepositoryInfo()*/


int 
GetSensorThresholds(uchar sens_num, uchar *thr_data)
{
	uchar resp[MAX_BUFFER_SIZE];
	int sresp = MAX_BUFFER_SIZE;
	uchar inputData[6];
	int rc;
	uchar cc = 0;

	inputData[0] = sens_num;
	rc = ipmi_cmd_mc(GET_SENSOR_THRESHOLD, inputData,1, resp,&sresp, &cc,fdebug);
	if (fdebug)
		printf("GetSensorThreshold[%02x] rc = %d, resp(%d) %02x %02x %02x %02x %02x %02x %02x\n",
			sens_num,rc, sresp,resp[0],resp[1],resp[2],resp[3],
			resp[4],resp[5],resp[6]);
	if (rc != ACCESS_OK) return(rc);
	if (cc != 0) return(cc);
	if (sresp == 0) return(-2);
	memcpy(thr_data,resp,sresp);
	return(0);
}

int
RearmSensor(uchar sens_num)
{
	uchar resp[MAX_BUFFER_SIZE];
	int sresp = MAX_BUFFER_SIZE;
	uchar inputData[8];
	int rc;
	uchar cc = 0;

	memset(inputData,0,6);
	memset(resp,0,6);
	inputData[0] = sens_num;
	rc = ipmi_cmd_mc(GET_SEVT_ENABLE, inputData, 1, resp,&sresp, &cc, fdebug);
	if (rc == 0 && cc != 0) rc = cc;
	if (rc != 0 || fdebug)
		printf("GetSensorEventEnable(%02x) rc = %d, cc = %x %02x %02x %02x\n",
			sens_num,rc,cc,resp[0],resp[1],resp[3]);
	if (rc == 0 && resp[0] != 0xc0) {
		printf("EventEnable(%02x) = %02x, is not 0xc0\n",
			sens_num,resp[0]);
		memset(inputData,0,6);
		inputData[0] = sens_num;
		inputData[1] = resp[0] | 0xc0;
		inputData[2] = resp[1];
		inputData[3] = resp[2];
		inputData[4] = resp[3];
		inputData[5] = resp[4];
		rc = ipmi_cmd_mc(SET_SEVT_ENABLE, inputData, 6, resp,&sresp,
				&cc, fdebug);
		if (rc == 0 && cc != 0) rc = cc;
		if (rc != 0 || fdebug)
		printf("SetSensorEventEnable(%02x) rc = %d, cc = %x\n",
			sens_num,rc,cc);
	}

	memset(inputData,0,6);
	inputData[0] = sens_num;
	inputData[1] = 0;  /* rearm all events for this sensor */
	rc = ipmi_cmd_mc(REARM_SENSOR, inputData, 6, resp,&sresp, &cc, fdebug);
	if (fdebug)
		printf("RearmSensor(%02x) rc = %d, cc = %x %02x %02x\n",
			sens_num,rc,cc,resp[0],resp[1]);
	if (rc == 0 && cc != 0) rc = cc;

	/* Could also do a global rearm via SetEventReceiver. */

	return(rc);
}  /*end RearmSensor*/

int 
SetSensorThresholds(uchar sens_num, uchar hi, uchar lo, 
			uchar *thr_data, uchar *thr_set)
{
	uchar resp[MAX_BUFFER_SIZE];
	int sresp = MAX_BUFFER_SIZE;
	uchar inputData[8];
	int rc;
	uchar cc = 0;
	uchar sets = 0;
	int i;

	/* 
	 * Set the sensor Hysteresis before setting the threshold.
	 */
	memset(inputData,0,8);
	inputData[0] = sens_num;
	inputData[1] = 0xff;
	rc = ipmi_cmd_mc(GET_SENSOR_HYSTERESIS,inputData,2, resp,&sresp, &cc,fdebug);
	if (fdebug)
		printf("GetSensorHysteresis(%02x) rc = %d, cc = %x %02x %02x\n",
			sens_num,rc,cc,resp[0],resp[1]);
	if (rc != ACCESS_OK) return(rc);
	inputData[0] = sens_num;
	inputData[1] = 0xff;
	inputData[2] = resp[0];
	inputData[3] = resp[1];
	rc = ipmi_cmd_mc(SET_SENSOR_HYSTERESIS,inputData,4, resp,&sresp, &cc,fdebug);
	if (fdebug)
		printf("SetSensorHysteresis(%02x) rc = %d, cc = %x\n",
			sens_num,rc,cc);
	if (rc != ACCESS_OK) return(rc);
	
	/* 
	 * The application should validate that values are ordered, 
	 * e.g.  upper critical should be greater than upper 
	 * non-critical.
	 * Due to the limited command line parameter interface, 
	 * use the hi & lo values to set each of the thresholds.
	 * For a full implemenation, these thresholds should be set
	 * individually.
	 */
	memset(inputData,0,8);
	inputData[0] = sens_num;
	sets = thr_data[0];
        if (thr_set != NULL) {  /* use specific thr_set values */
           memcpy(&inputData[2],thr_set,6);
        } else {  /*default, use hi/lo params */
	   if (lo == 0xff) sets &= 0x38;  /* don't set lowers */
	   else {
	      inputData[2] = lo;      /* lower non-crit  (& 0x01)  */
	      inputData[3] = lo - 1;  /* lower critical  (& 0x02) */
	      inputData[4] = lo - 2;  /* lower non-recov (& 0x04) */
	   }
	   if (hi == 0xff) sets &= 0x07;  /* don't set uppers */
	   else {
	      inputData[5] = hi;      /* upper non-crit  (& 0x08) */
	      inputData[6] = hi + 1;  /* upper critical  (& 0x10) */
	      inputData[7] = hi + 2;  /* upper non-recov (& 0x20) */
	   }
        } 
	inputData[1] = sets;  /* which ones to set */
	{   /* show from/to changes */
		printf("GetThreshold[%02x]: %02x ",sens_num,sens_num);
		for (i = 0; i < 7; i++) printf("%02x ",thr_data[i]);
		printf("\n");
		printf("SetThreshold[%02x]: ",sens_num);
		for (i = 0; i < 8; i++) printf("%02x ",inputData[i]);
		printf("\n");
	}
	rc = ipmi_cmd_mc(SET_SENSOR_THRESHOLD, inputData, 8, resp,&sresp, &cc, fdebug);
	if (fdebug)
		printf("SetSensorThreshold(%02x) rc = %d, cc = %x\n",
			sens_num,rc,cc);
	if (rc == 0 && cc != 0) rc = cc;
	/* mBMC gets cc = 0xD5 (213.) here, setting thresholds disabled. */
	return(rc);
}

int 
GetSensorReading(uchar sens_num, void *psdr, uchar *sens_data)
{
	uchar resp[MAX_BUFFER_SIZE];
	int sresp = MAX_BUFFER_SIZE;
	uchar inputData[6];
        SDR02REC *sdr = NULL;
        int mc;
	int rc;
	uchar cc = 0;
	uchar lun = 0;
	uchar chan = 0;

        if (psdr != NULL && fbadsdr == 0) {
           sdr = (SDR02REC *)psdr;
           mc = sdr->sens_ownid;
	   if (mc != BMC_SA) {  /* not BMC, e.g. HSC or ME sensor */
	      uchar a = ADDR_IPMB;
	      if (mc == HSC_SA) a = ADDR_SMI;
	      chan = (sdr->sens_ownlun & 0xf0) >> 4;
	      lun = (sdr->sens_ownlun & 0x03);
              ipmi_set_mc(chan,(uchar)mc, lun,a);
           }
        } else mc = BMC_SA;
	inputData[0] = sens_num;
	rc = ipmi_cmd_mc(GET_SENSOR_READING,inputData,1, resp,&sresp,&cc,fdebug);
	if (fdebug) 
	    printf("GetSensorReading mc=%x,%x,%x status=%d cc=%x sz=%d resp: %02x %02x %02x %02x\n",
		     chan,mc,lun,rc,cc,sresp,resp[0],resp[1],resp[2],resp[3]);
        if (mc != BMC_SA) ipmi_restore_mc();
	if ((rc == 0) && (cc != 0)) {
	    if (fdebug) printf("GetSensorReading error %x %s\n",cc,
				decode_cc((ushort)0,(uchar)cc));
            rc = cc;
        }
        if (rc != 0) return(rc);

	if (resp[1] & 0x20) { /* init state, reading invalid */
	   if (fdebug) 
		printf("sensor[%x] in init state, no reading\n", sens_num);
           sens_data[1] = resp[1];
           sens_data[2] = 0x40;  /*set bit num for init state*/
        } else {     /*valid reading, copy it*/
	   /* only returns 4 bytes, no matter what type */
	   memcpy(sens_data,resp,4);
	}
	return(0);
}  /*end GetSensorReading()*/

int 
GetSensorReadingFactors(uchar snum, uchar raw, int *m, int *b, int *  b_exp, 
			int *r, int *a)
{
	uchar resp[MAX_BUFFER_SIZE];
	int sresp = MAX_BUFFER_SIZE;
	uchar inputData[6];
	int rc;
	uchar cc = 0;
	int  toler, a_exp;

	inputData[0] = snum;
	inputData[1] = raw;
	rc = ipmi_cmd_mc(GET_SENSOR_READING_FACTORS, inputData, 2, 
                      resp,&sresp, &cc, fdebug);
	if (fdebug) printf("GetSensorReadingFactors status = %d\n",rc);
	if (rc != ACCESS_OK) return(rc);
	if (cc != 0) return(cc);

        /* successful, copy values */
	*m = resp[1] + ((resp[2] & 0xc0) << 2);
	toler = resp[2] & 0x3f;
	*b = resp[3] + ((resp[4] & 0xc0) << 2);
	*a = (resp[4] & 0x3f) + ((resp[5] & 0xf0) << 4);
	a_exp = (resp[5] & 0xc0) >> 2;
	*r = (resp[6] &0xf0) >> 4;
	*b_exp = resp[6] & 0x0f;
	if (fdebug) {
	      printf("factors: next=%x m=%d b=%d b_exp=%d a=%d a_exp=%d r=%d\n",
			resp[0],*m,*b,*b_exp,*a,a_exp,*r);
	}
	return(0);
}

int GetSensorType(uchar snum, uchar *stype, uchar *rtype)
{
	uchar resp[MAX_BUFFER_SIZE];
	int sresp = MAX_BUFFER_SIZE;
	uchar inputData[6];
	int rc;
	uchar cc = 0;

	inputData[0] = snum;
	rc = ipmi_cmd_mc(GET_SENSOR_TYPE, inputData, 1, 
                      resp,&sresp, &cc, fdebug);
	if (fdebug) printf("GetSensorType: ipmi_cmd rv = %d, cc = %x\n",rc,cc);
	if (rc != ACCESS_OK) return(rc);
	if (cc != 0) return(cc);
        /* successful, copy values */
	if (stype != NULL) *stype = resp[0];
	if (rtype != NULL) *rtype = resp[1] & 0x7f;
	return(rc);
}

void set_reserve(int val)
{
   fDoReserve = val;
}

int sdr_get_reservation(uchar *res_id, int fdev)
{
   int sresp;
   uchar resp[MAX_BUFFER_SIZE];
   uchar cc = 0;
   ushort cmd;
   int rc = -1;

   if (fDoReserve == 1) {
	fDoReserve = 0; /* only reserve SDR the first time */
        sresp = sizeof(resp);;
	if (fdev) cmd = RESERVE_DEVSDR_REP;
	else cmd = RESERVE_SDR_REP;
	rc = ipmi_cmd_mc(cmd, NULL, 0, resp, &sresp, &cc, fdebug);
	if (rc == 0 && cc != 0) rc = cc;
	if (rc == 0) {  /* ok, so set the reservation id */
	   resid[0] = resp[0]; 
	   resid[1] = resp[1];
        }
	/* A reservation is cancelled by the next reserve request. */
        if (fdebug)
	   printf("ipmi_cmd RESERVE status=%d cc=%x id=%02x%02x\n",
		  rc,cc,resid[0],resid[1]);
   } else rc = 0;
   /* if not first time, or if error, return existing resid. */
   res_id[0] = resid[0];
   res_id[1] = resid[1];
   return(rc);
} /*end sdr_get_reservation*/

int sdr_clear_repo(int fdev)
{
   int sresp;
   uchar resp[MAX_BUFFER_SIZE];
   uchar inputData[6];
   uchar cc = 0;
   int rc = -1;
   ushort cmd;
   uchar resv[2] = {0,0};

   if (fReserveOK) 
	rc = sdr_get_reservation(resv,fdev);

   cmd = 0x27 + (NETFN_STOR << 8);  /*Clear SDR Repository*/
   inputData[0] = resv[0];     /*res id LSB*/
   inputData[1] = resv[1];     /*res id MSB*/
   inputData[2] = 'C';
   inputData[3] = 'L'; 
   inputData[4] = 'R';
   inputData[5] = 0xAA;  
   sresp = sizeof(resp);;
   rc = ipmi_cmd_mc(cmd, inputData, 6, resp, &sresp,&cc, fdebug);
   if (fdebug) printf("sdr_clear_repo: rc = %d, cc = %x, sz=%d\n",rc,cc,sresp);
   if (rc == 0 && cc != 0) rc = cc;

   if (rc == 0 && (resp[0] & 1) != 1) {
	if (fdebug) printf("Wait for sdr_clear_repo to complete\n");
	os_usleep(1,0);
   }
   return(rc);
}

int sdr_add_record(uchar *sdr, int fdev)
{
   int sresp;
   uchar resp[MAX_BUFFER_SIZE];
   uchar inputData[6+SZCHUNK];
   uchar cc = 0;
   int rc = -1;
   ushort cmd;
   uchar resv[2] = {0,0};
   int reclen, len, i;
   int recid, chunksz;
   uchar prog;

   reclen = sdr[4] + 5;
   recid = sdr[0] + (sdr[1] << 8);
   /* OEM SDRs can be min 8 bytes, less is an error. */
   if (reclen < 8) return(LAN_ERR_BADLENGTH);  
   if (fReserveOK) 
	rc = sdr_get_reservation(resv,fdev);
   if (fdebug) printf("sdr_add_record[%x]: reclen = %d, reserve rc = %d\n",
			recid,reclen,rc);

   cmd = 0x25 + (NETFN_STOR << 8); /*PartialAddSdr*/
   recid = 0;  /*first chunk must be added as 0000*/
   chunksz = SZCHUNK;
   for (i = 0; i < reclen; )
   {
      prog = 0;
      len = chunksz;
      if ((i+chunksz) >= reclen) { /*last record*/
	  len = reclen - i;
	  prog = 1;
      }
      inputData[0] = resv[0]; /*res id LSB*/
      inputData[1] = resv[1]; /*res id MSB*/
      inputData[2] = recid & 0x00ff;         /*record id LSB*/
      inputData[3] = (recid >> 8) & 0x00ff;  /*record id MSB*/
      inputData[4] = (uchar)i;       /*offset */
      inputData[5] = prog;    /*progress: 1=last record*/
      memcpy(&inputData[6],&sdr[i],len);
      sresp = sizeof(resp);
      rc = ipmi_cmd_mc(cmd, inputData, 6+len, resp, &sresp,&cc, fdebug);
      if (fdebug) 
         printf("sdr_add_record[%x,%x]: rc = %d, cc = %x, sz=%d\n",
			recid,i,rc,cc,sresp);
      if (rc == 0 && cc != 0) rc = cc;
      if (rc != 0) break;
      if (recid == 0 && rc == 0)   /*first time, so set recid*/
         recid = resp[0] + (resp[1] << 8);
      i += len;
   }
   return(rc);
}

int GetSDR(int r_id, int *r_next, uchar *recdata, int srecdata, int *rlen)
{
	int sresp;
	uchar resp[MAX_BUFFER_SIZE+SZCHUNK];
	uchar respchunk[SZCHUNK+10];
	uchar inputData[6];
	uchar cc = 0;
	int rc = -1;
	int  i, chunksz, thislen, off;
	int reclen;
	ushort cmd;
	uchar resv[2] = {0,0};
  
	chunksz = SZCHUNK;
        reclen = srecdata; /*max size of SDR record*/
	off = 0;
        *rlen = 0;
        *r_next = 0xffff;  /*default*/
	if (fReserveOK) 
	   rc = sdr_get_reservation(resv,fdevsdrs);
	if (fdevsdrs) cmd = GET_DEVICE_SDR;
	else cmd = GET_SDR;
	if (reclen == 0xFFFF) {  /* get it all in one call */
	   inputData[0] = resv[0];     /*res id LSB*/
	   inputData[1] = resv[1];     /*res id MSB*/
	   inputData[2] = r_id & 0x00ff;        /*record id LSB*/
	   inputData[3] = (r_id & 0xff00) >> 8;	/*record id MSB*/
	   inputData[4] = 0;      /*offset */
	   inputData[5] = 0xFF;  /*bytes to read, ff=all*/
	   sresp = sizeof(resp);;
	   if (fdebug) printf("ipmi_cmd SDR id=%d read_all, len=%d\n",
				r_id,sresp);
	   rc = ipmi_cmd_mc(cmd, inputData, 6, recdata, &sresp,&cc, fdebug);
	   /* This will usually return cc = 0xCA (invalid length). */
	   if (fdebug) printf("ipmi_cmd SDR data status = %d, cc = %x, sz=%d\n",
				rc,cc,sresp);
	   reclen = sresp;
	   *r_next = recdata[0] + (recdata[1] << 8);
	} else    /* if (reclen > chunksz) do multi-part chunks */
	for (off = 0; off < reclen; )
	{
	   thislen = chunksz;
	   if ((off+chunksz) > reclen) thislen = reclen - off;
	   inputData[0] = resv[0];     /*res id LSB*/
	   inputData[1] = resv[1];     /*res id MSB*/
	   inputData[2] = r_id & 0x00ff; 	/*record id LSB*/
	   inputData[3] = (r_id & 0xff00) >> 8;	/*record id MSB*/
	   inputData[4] = (uchar)off;      /*offset */
	   inputData[5] = (uchar)thislen;  /*bytes to read, ff=all*/
	   sresp = sizeof(respchunk);
	   rc = ipmi_cmd_mc(cmd, inputData, 6, respchunk, &sresp,&cc, fdebug);
	   if (fdebug) 
               printf("ipmi_cmd SDR[%x] off=%d ilen=%d status=%d cc=%x sz=%d\n",
			r_id,off,thislen,rc,cc,sresp);
	   if (off == 0 && cc == 0xCA && thislen == SZCHUNK) { 
		/* maybe shorter than SZCHUNK, try again */
	        chunksz = 0x06;
	        if (fdebug) printf("sdr[%x] try again with chunksz=%d\n",
					r_id,chunksz);
	        continue;
	   }
           if (off > chunksz) {
              /* already have first part of the SDR, ok to truncate */
              if (rc == -3) { /* if LAN_ERR_RECV_FAIL */
                  if (fdebug) printf("sdr[%x] error rc=%d len=%d truncated\n",
                                       r_id,rc,sresp);
                  sresp = 0;
                  rc = 0;
              }
              if (cc == 0xC8 || cc == 0xCA) { /* length errors */
                  /* handle certain MCs that report wrong length,
                   * at least use what we already have (sresp==0) */
                  if (fdebug) printf("sdr[%x] error cc=%02x len=%d truncated\n",
                                       r_id,cc,sresp);
                  cc = 0;
              }
           }
	   if (rc != ACCESS_OK) return(rc);
	   if (cc != 0) return(cc);
	   /* if here, successful, chunk was read */
           if (sresp < (thislen+2)) {
              /* There are some SDRs that may report the wrong length, and
               * return less bytes than they reported, so just truncate. */
              fprintf(stderr,"SDR record %x is malformed, length %d is less than minimum %d\n",r_id,sresp,thislen+2);
              if (fdebug) printf("sdr[%x] off=%d, expected %d, got %d\n",
                                r_id,off,thislen+2,sresp);
              if (sresp >= 2) thislen = sresp - 2;
              else thislen = 0;
              reclen = off + thislen;  /* truncate, stop reading */
	      /* auto-corrected, so not a fatal error */
	      // rc = ERR_SDR_MALFORMED;
           }
	   /* successful */
	   memcpy(&resp[off],&respchunk[2],thislen);
	   if (off == 0 && sresp >= 5) {
		*r_next = respchunk[0] + (respchunk[1] << 8);
                reclen = respchunk[6] + 5;  /*get actual record size*/
                if (reclen > srecdata) {
                  if (fdebug) printf("sdr[%x] chunk0, reclen=%d srecdata=%d\n",
                                        r_id, reclen, srecdata);
                  reclen = srecdata; /*truncate*/
                }
           }
	   off += thislen;
           *rlen = off;
	}
	if (fdebug) {
           printf("GetSDR[%04x] next=%x (len=%d): ",r_id,*r_next,reclen); 
	   for (i = 0; i < reclen; i++) printf("%02x ",resp[i]);
	   printf("\n");
	   }
	memcpy(recdata,&resp[0],reclen);
        *rlen = reclen;
	return(rc);
}  /*end GetSDR*/

static int nsdrs = 0;    /*number of sdrs*/
static int sz_sdrs = 0;  /*actual size used with sdrs*/
static uchar *psdrcache = NULL;

void free_sdr_cache(uchar *ptr)
{
   if (ptr != NULL) free(ptr);
   if ((ptr != psdrcache) && (psdrcache != NULL)) 
		    free(psdrcache);
   psdrcache = NULL;
}

int get_sdr_file(char *sdrfile, uchar **sdrlist)
{
   int rv = -1;
   FILE *fp = NULL;
   int i, n, num, nsdr, isdr, len;
   uchar *sdrbuf;
   char buff[255];
   uchar hbuf[85];
   char fvalid;

   fp = fopen(sdrfile,"r");
   if (fp == NULL) {
         printf("Cannot open file %s\n",sdrfile);
         return(ERR_FILE_OPEN);
   }
   /* determine number of SDRs by number of lines in the file */
   num = 0;
   while (fgets(buff, 255, fp)) { num++; }
   if (fdebug) {
     printf("Reading %d SDRs from file %s\n",num,sdrfile);
     if ((psdrcache != NULL) && (nsdrs > 0)) {  /*already have sdrcache*/
	printf("get_sdr_file: Already have cache\n"); /*fdebug*/
	free_sdr_cache(psdrcache); /*free previous sdrcache*/
     }
   }
   sdrbuf = malloc(num * SDR_SZ); 
   if (sdrbuf == NULL) {
	fclose(fp);
	return(rv);
   }
   fseek(fp, 0L, SEEK_SET); 
   *sdrlist = sdrbuf; 
   psdrcache = sdrbuf;
   nsdrs = num;
   isdr = 0;
   nsdr = 0;
   while (fgets(buff, 255, fp)) {
   	len = strlen_(buff);
   	fvalid = 0;
   	if (buff[0] >= '0' && (buff[0] <= '9')) fvalid = 1;
   	else if (buff[0] >= 'a' && (buff[0] <= 'f')) fvalid = 1;
   	else if (buff[0] >= 'A' && (buff[0] <= 'F')) fvalid = 1;
   	if (fvalid == 0) continue;
	i = 0;
   	for (n = 0; n < len; ) {
           if (buff[n] < 0x20) break; /* '\n', etc. */
           hbuf[i++] = htoi(&buff[n]);
	   n += 3;
   	}
	memcpy(&sdrbuf[isdr],hbuf,i);
	isdr += i;
	nsdr++;
   } /*end while*/
   if (fdebug) printf("Read %d SDRs, %d bytes\n",nsdr,isdr);
   fclose(fp);
   rv = 0;
   return(rv);
}

int get_sdr_cache(uchar **pret)
{
   int rv = -1;
   int i, n, sz, len, asz;
   int recid, recnext;
   uchar *pcache;
   uchar *psdr;

   if (pret == NULL) return(rv);
   fdevsdrs = use_devsdrs(fpicmg);

   if ((psdrcache != NULL) && (nsdrs > 0)) {  /*already have sdrcache*/
        *pret = psdrcache;
	if (fdebug) printf("get_sdr_cache: already have cache (%p)\n",*pret);
	return(0);
   }
   else if (fdebug) printf("get_sdr_cache: Allocating cache\n");

   rv = GetSDRRepositoryInfo(&n,&fdevsdrs);
   if (rv != 0) return(rv);
   if (n == 0) {  
	/* this is an error, probably because fdevsdrs is wrong.*/
	if (fdebug) printf("get_sdr_cache: nsdrs=0, retrying\n");
	fdevsdrs = (fdevsdrs ^ 1);
	n = 150; /*try some default num SDRs*/
   }

   sz = n * SDR_SZ;  /*estimate max size for n sdrs*/
   pcache = malloc(sz);
   if (pcache == NULL) return(rv);
   psdrcache = pcache;
   *pret = pcache;
   memset(pcache,0,sz);
   recid = 0;
   asz = 0;
   for (i = 0; i <= n; i++)
   {
	if (recid == 0xffff) break;
	// psdr = &pcache[i * SDR_SZ];	
        psdr = &pcache[asz];
	rv = GetSDR(recid,&recnext,psdr,SDR_SZ,&len);
	if (fdebug) 
	   printf("GetSDR[%x] rv = %d len=%d next=%x\n",recid,rv,len,recnext);
	if (rv != 0) {
	   if (rv == 0xC5) { set_reserve(1); i--; } /*retry*/
	   else break;
	} else {  /*success*/
	   /* if sdrlen!=len, adjust */
	   if ((len > 5) && (len != (psdr[4] + 5)) ) {
		if (fdebug) printf("SDR[%x] adjust len from %d to %d\n",
					recid,psdr[4]+5,len);
		psdr[4] = len - 5;
	   }
	   asz += len;
	   if (recnext == recid) recid = 0xffff;
	   else recid = recnext;
	}
   }
   nsdrs = n;
   sz_sdrs = asz;  /* save the size for later*/
   if (fdebug) {
	printf("get_sdr_cache, n=%d sz=%d asz=%d\n",n,sz,asz);
	if (i < n) printf("get_sdr_cache error, i=%d < n=%d, rv=%d\n",i,n,rv);
   }
   return(rv);
}

int find_nsdrs(uchar *pcache)
{
   int num = 0;
   ulong asz = 0;
   int i, len;
   uchar *sdr;
   ushort recid = 0;

   if (pcache == NULL) return(num);
   for (i = 0; (int)asz < sz_sdrs; i++)
   {
	sdr = &pcache[asz];
	if (sdr[2] != 0x51) {  /* Dell SDR length error */
	    printf("SDR[%x] length error at %ld\n",recid,asz);
	    sdr = &pcache[++asz]; /*try it if off-by-one*/
	}
	len = sdr[4] + 5;
	recid = sdr[0] + (sdr[1] << 8);
	if (fdebug) printf("SDR[%x] len=%d i=%d offset=%lx\n",recid,len,i,asz);
	asz += len;
   }
   num = i;
   return(num);
}

int find_sdr_by_snum(uchar *psdr, uchar *pcache, uchar snum, uchar sa)
{
   int rv = -1;
   uchar *sdr;
   int i, k, len;
   int asz = 0;
   if (psdr == NULL) return(rv);
   if (pcache == NULL) return(rv);
   for (i = 0; i <= nsdrs; i++)
   {
	// sdr = &pcache[i * SDR_SZ];	
	sdr = &pcache[asz];	
	len = sdr[4] + 5;
	asz += len;
	switch(sdr[3]) {
	case 0x01: k = 7; break; /*full sensor*/
	case 0x02: k = 7; break; /*compact sensor*/
	case 0x03: k = 7; break;/*compact sensor*/
	default:   k = 0; break;
	}
	if (k == 0) continue;
	else {
	    if ((sdr[5] == sa) && (sdr[k] == snum)) {
		memcpy(psdr,sdr,len);
		return(0);
	    }
	}
   }
   return(rv);
}

int find_sdr_by_tag(uchar *psdr, uchar *pcache, char *tag, uchar dbg)
{
   int rv = -1;
   uchar *sdr;
   int i, k, n, len;
   int asz = 0;
   if (psdr == NULL) return(rv);
   if (pcache == NULL) return(rv);
   if (tag == NULL) return(rv);
   if (dbg) fdebug = 1;
   n = strlen_(tag);
   if (fdebug) printf("find_sdr_by_tag(%s) nsdrs=%d\n",tag,nsdrs);
   for (i = 0; i <= nsdrs; i++)
   {
	// sdr = &pcache[i * SDR_SZ];	
	sdr = &pcache[asz];
	len = sdr[4] + 5;
	asz += len;
	switch(sdr[3]) { /* set tag offset by SDR type */
	case 0x01: k = 48; break; /*full SDR*/
	case 0x02: k = 32; break; /*compact SDR*/
	case 0x03: k = 17; break; /*event-only SDR*/
	case 0x10: k = 16; break; /*device locator SDR*/
	case 0x11: k = 16; break; /*FRU device locator SDR*/
	case 0x12: k = 16; break; /*IPMB device locator SDR*/
	default:   k = 0; break;  /*else do not have an ID string/tag*/
	}
	if (k == 0) {
	   if (fdebug) printf("sdr[%d] idx=%02x%02x num=%x type=%x skip\n",
				i,sdr[1],sdr[0],sdr[7],sdr[3]);
	   continue;
	} else {
	    if (len > SDR_SZ) len = SDR_SZ;
	    if (fdebug) {
		char tmp[17];
		memset(tmp,0,sizeof(tmp));
		memcpy(tmp,&sdr[k],(len - k));
		tmp[16] = 0;  /*force stringify*/
		printf("sdr[%d] idx=%02x%02x num=%x tag: %s\n",i,sdr[1],sdr[0],
			sdr[7],tmp);
	    }
	    if (strncmp(tag,(char *)&sdr[k],n) == 0) {
		memcpy(psdr,sdr,len);
		return(0);
	    }
	}
   }
   return(rv);
}


int find_sdr_next(uchar *psdr, uchar *pcache, ushort id)
{
   int rv = -1;
   uchar *sdr;
   int i, imatch, len;
   ushort recid;
   int asz = 0;
   if (psdr == NULL) return(rv);
   if (pcache == NULL) return(rv);
   imatch = nsdrs;
   for (i = 0; i < nsdrs; i++)
   {
	// sdr = &pcache[i * SDR_SZ];
	sdr = &pcache[asz];
	if (sdr[2] != 0x51)   /* Dell SDR off-by-one error */
	    sdr = &pcache[++asz]; 
	len = sdr[4] + 5;
	recid = sdr[0] + (sdr[1] << 8);
	asz += len;
	// if (fdebug) printf("SDR[%x] len=%d id=%x i=%d imatch=%d\n",
	//			recid,len,id,i,imatch);
	if (recid == id) imatch = i + 1; /*matches prev, return next one*/
	else if (id == 0) { rv = 0; break; } /* 0000 = first one */
	if (i == imatch) { rv = 0; break; }
   }
   if (rv == 0) memcpy(psdr,sdr,len);
   return(rv);
}

int find_sdr_by_id(uchar *psdr, uchar *pcache, ushort id)
{
   int rv = -1;
   uchar *sdr;
   int i, imatch, len;
   ushort recid;
   int asz = 0;
   if (psdr == NULL) return(rv);
   if (pcache == NULL) return(rv);
   imatch = nsdrs;
   for (i = 0; i < nsdrs; i++)
   {
	sdr = &pcache[asz];
	len = sdr[4] + 5;
	recid = sdr[0] + (sdr[1] << 8);
	asz += len;
	if (recid == id) { rv = 0; break; }
	else if (id == 0) { rv = 0; break; } /* 0000 = first one */
   }
   if (rv == 0) memcpy(psdr,sdr,len);
   return(rv);
}

uchar
bitnum(ushort value)
{
   uchar ret = 0;
   int i;
   /* returns the highest bit number number set in this word. */
   /* Bit numbers are 1-based in this routine, 0 means no bits set. */
   /* scan 15 bits (0x7FFF). */
   for (i = 0; i < 15; i++) {
	if (value & 0x01) ret = i+1;  /*was ret++;*/
	value = (value >> 1);
   }
   return(ret);
}

static double
expon(int x, int y)
{
   double res;
   int i;
   /* compute exponent: x to the power y */
   res = 1;
   if (y > 0) {
	for (i = 0; i < y; i++) res = res * x;
   } else if (y < 0) {
	for (i = 0; i > y; i--) res = res / x;
   } /* else if if (y == 0) do nothing, res=1 */
   return(res);
}

double
RawToFloat(uchar raw, uchar *psdr)
{
   double floatval;
   int m, b, a;
   uchar  ax; 
   int  rx, b_exp;
   SDR01REC *sdr;
   int signval;

   sdr = (SDR01REC *)psdr;
   floatval = (double)raw;  /*default*/

   // if (raw == 0xff) floatval = 0; else 
   if (sdr->rectype == 0x01) {  /* SDR rectype == full */
	if (fdebug)
	   printf("units=%x base=%d mod=%d (raw=%x, nom_rd=%x)\n",
		sdr->sens_units,sdr->sens_base,sdr->sens_mod,
		raw, sdr->nom_reading);
	m = sdr->m + ((sdr->m_t & 0xc0) << 2);
	b = sdr->b + ((sdr->b_a & 0xc0) << 2);
	if (b & 0x0200) b = (b - 0x0400);  /*negative*/
	if (m & 0x0200) m = (m - 0x0400);  /*negative*/
	rx = (sdr->rx_bx & 0xf0) >> 4;
	if (rx & 0x08) rx = (rx - 0x10); /*negative, fix sign w ARM compilers*/
	a = (sdr->b_a & 0x3f) + ((sdr->a_ax & 0xf0) << 2);
	ax = (sdr->a_ax & 0x0c) >> 2;
	b_exp = (sdr->rx_bx & 0x0f);
	if (b_exp & 0x08) b_exp = (b_exp - 0x10);  /*negative*/
		//b_exp |= 0xf0;        /* negative 8-bit */
	if ((sdr->sens_units & 0xc0) == 0) {  /*unsigned*/
		floatval = (double)raw;
	} else {  /*signed*/
		if (raw & 0x80) signval = (raw - 0x100);
		else signval = raw;
		floatval = (double)signval;
	}
	floatval *= (double) m;
#ifdef MATH_OK
	floatval += (b * pow (10,b_exp));
	floatval *= pow (10,rx);
#else
	floatval += (b * expon (10,b_exp));
	floatval *= expon (10,rx);
#endif
	if (fdebug)
	   printf("decode1: m=%d b=%d b_exp=%x rx=%d, a=%d ax=%d l=%x, floatval=%f\n",
			m,b,b_exp,rx,a,ax,sdr->linear,floatval);
	switch(sdr->linear) {
	case 0:   /*linear*/
	   break;
	case 7:   /*invert 1/x*/
	   /* skip if zero to avoid dividing by zero */
	   if (raw != 0) floatval = 1 / floatval;
	   break;
	case 1:   /*ln*/
	case 2:   /*log10, log2, e, exp10, exp2, */
	case 3:   /*log2*/
	case 4:   /*e*/
	case 5:   /*exp10*/
	case 6:   /*exp2*/
	case 8:   /*sqr(x)*/
	case 9:   /*cube(x)*/
	case 10:  /*sqrt(x)*/
	case 11:  /*cube-1(x)*/
        default:
	   if (fdebug) printf("linear mode %x not implemented\n",sdr->linear);
	   break;
	}  /*end-switch linear*/
   }

#ifdef NOT_LINEAR
   /* else if (sdr->linear != 7) */
   {
     double be, re;
     rc = GetSensorType(sdr->sens_num,&stype,&rtype);
     if (fdebug)
	printf("decode: rc=%x, stype=%x, rtype=%x\n",rc,stype,rtype);
     if (rc != 0) return(floatval);

     /* GetSensorReadingFactors */
     rc = GetSensorReadingFactors(sdr->sens_num,raw,&m,&b,&b_exp,&r,&a);
     if (rc == 0) {
	// floatval = ((m * raw) + (b * be)) * re;
     } 
     if (fdebug) printf("decode: rc=%x, floatval=%f\n",rc,floatval);
   }
#endif

   return(floatval);
}

#define IpmiAnalogDataFormatUnsigned 0
#define IpmiAnalogDataFormat1Compl   1
#define IpmiAnalogDataFormat2Compl   2

uchar
FloatToRaw(double val, uchar *psdr, int rounding)
{
   double      cval;
   int         lowraw, highraw, raw, maxraw, minraw, next_raw;
   int         analog_dfmt;

   analog_dfmt = (psdr[20] >> 6) & 0x03;
   switch( analog_dfmt )
   {
       case IpmiAnalogDataFormat1Compl:
            lowraw   = -127;
            highraw  = 127;
            minraw   = -127;
            maxraw   = 127;
            next_raw = 0;
            break;
       case IpmiAnalogDataFormat2Compl:
            lowraw   = -128;
            highraw  = 127;
            minraw   = -128;
            maxraw   = 127;
            next_raw = 0;
            break;
       case IpmiAnalogDataFormatUnsigned:
       default:
            lowraw   = 0;
            highraw  = 255;
            minraw   = 0;
            maxraw   = 255;
            next_raw = 128;
            break;
    }

    /* do a binary search for the right nth root value */
    do {
        raw = next_raw;
        cval = RawToFloat( (uchar)raw, psdr );
        if ( cval < val ) {
            next_raw = ((highraw - raw) / 2) + raw;
            lowraw = raw;
        } else {
            next_raw = ((raw - lowraw) / 2) + lowraw;
            highraw = raw;
        }
    } while ( raw != next_raw );
   
    /* Do rounding to get the final value */
    switch( rounding ) {
       case 0:   /* Round Normal = Round to nearest value */
            if ( val > cval ) {
                 if ( raw < maxraw ) {
                      double nval;
		      nval = RawToFloat((uchar)(raw+1),psdr);
                      nval = cval + ((nval - cval) / 2.0);
                      if ( val >= nval ) raw++;
                 }
            } else {
                 if ( raw > minraw ) {
                      double pval;
		      pval = RawToFloat((uchar)(raw-1),psdr);
                      pval = pval + ((cval - pval) / 2.0);
                      if ( val < pval ) raw--;
                 }
            }
            break;
       case 1:  /*Round Down*/
            if ((val < cval) && (raw > minraw )) raw--;
            break;
       case 2:  /*Round Up*/
            if ((val > cval) && (raw < maxraw)) raw++;
            break;
     }
     if ( analog_dfmt == IpmiAnalogDataFormat1Compl )
        if ( raw < 0 ) raw -= 1;
    return((uchar)raw);
}  /*end FloatToRaw()*/

static int fill_thresholds(double *thrf, uchar *sdr)
{
   int rv = 0;
   uchar *vals;
   uchar bits;

   // snum = sdr[7];
   bits = sdr[19]; /*which are settable*/
   vals = &sdr[36];
   if (fdebug) 
      printf("fill_thresholds: bits=%02x, values: %f>=%f>=%f, %f<=%f<=%f\n",
		bits, thrf[0],thrf[1],thrf[2], thrf[3],thrf[4],thrf[5]);
   if (thrf[0] == THR_EMPTY) {
	if ((bits & 0x01) != 0) { /*lo-noncrit*/
	   thrf[0] = RawToFloat(vals[5],sdr);
           rv++;
        } else thrf[0] = 0;
   }
   if (thrf[1] == THR_EMPTY) { 
	if ((bits & 0x02) != 0) { /*lo-crit*/
	   thrf[1] = RawToFloat(vals[4],sdr);
           rv++;
	} else thrf[1] = 0;
   }
   if (thrf[2] == THR_EMPTY) { 
	if ((bits & 0x04) != 0) { /*lo-unrec*/
	   thrf[2] = RawToFloat(vals[3],sdr);
           rv++;
	} else thrf[2] = 0;
   }
   if (thrf[3] == THR_EMPTY) { 
	if ((bits & 0x08) != 0) { /*hi-noncrit*/
	   thrf[3] = RawToFloat(vals[2],sdr);
           rv++;
	} else thrf[3] = 0;
   }
   if (thrf[4] == THR_EMPTY) { 
	if ((bits & 0x10) != 0) { /*hi-crit*/
	   thrf[4] = RawToFloat(vals[1],sdr);
           rv++;
	} else thrf[4] = 0;
   }
   if (thrf[5] == THR_EMPTY) { 
	if ((bits & 0x20) != 0) { /*hi-unrec*/
	   thrf[5] = RawToFloat(vals[0],sdr);
           rv++;
	} else thrf[5] = 0;
   }
   if (fdebug)
      printf("fill_thresholds: after rv=%d values: %f>=%f>=%f, %f<=%f<=%f\n",
	      rv,thrf[0],thrf[1],thrf[2], thrf[3],thrf[4],thrf[5]);
   return(rv); 
}

char *
decode_itype(uchar itype)
{
   char *retstr;
   int i;
   /* Decode the Interrupt Type from Entity Assoc records */

   retstr = tmpstr;
   if (itype <= 0x0f) sprintf(retstr,"IRQ_%d",itype);
   else if (itype <= 0x13) {
	strcpy(retstr,"PCI-A");
	for (i=0x10;i<itype;i++) retstr[4]++;
	}
   else if (itype == 0x14) strcpy(retstr,"SMI");
   else if (itype == 0x15) strcpy(retstr,"SCI");
   else if (itype >= 0x20 && itype <= 0x5f) 
	sprintf(retstr,"SysInt_%d",itype-0x20);
   else if (itype == 0x60) strcpy(retstr,"ACPI/PnP");
   else if (itype == 0xFF) strcpy(retstr,"NoInt");
   else strcpy(retstr,"Invalid");
   return(retstr);
}

int decode_oem_sensor(uchar *sdr,uchar *reading,char *pstring,int slen)
{
   int rv = -1;
#ifdef METACOMMAND
   switch(vend_id) {
      case VENDOR_INTEL: 
          rv = decode_sensor_intel(sdr, reading, pstring, slen);
          break;
      case VENDOR_KONTRON: 
          rv = decode_sensor_kontron(sdr, reading, pstring, slen);
          break;
      case VENDOR_FUJITSU:
          rv = decode_sensor_fujitsu(sdr,reading,pstring,slen);
          break;
      case VENDOR_SUN: 
          rv = decode_sensor_sun(sdr, reading, pstring, slen);
          break;
      case VENDOR_MAGNUM: 
      case VENDOR_SUPERMICRO: 
      case VENDOR_SUPERMICROX: 
          rv = decode_sensor_supermicro(sdr,reading,pstring,slen,fsimple,fdebug);
          break;
      case VENDOR_QUANTA: 
          rv = decode_sensor_quanta(sdr, reading, pstring, slen);
          break;
      case VENDOR_HP: 
          rv = decode_sensor_hp(sdr, reading, pstring, slen);
          break;
      case VENDOR_DELL: 
          rv = decode_sensor_dell(sdr, reading, pstring, slen);
          break;
      case VENDOR_IBM: 
      case VENDOR_LENOVO: 
      case VENDOR_LENOVO2: 
          rv = decode_sensor_lenovo(sdr, reading, pstring, slen);
          break;
      case VENDOR_ASUS: 
          rv = decode_sensor_asus(sdr, reading, pstring, slen);
          break;
      default:
	  break;
   } /*end-switch vend_id*/
   if (fdebug) // && rv == 0)
      printf("decode_oem_sensor rv=%d vend=%x string=%s\n",rv,vend_id,pstring);
#endif
   return (rv);
}

int show_oemsdr(int vend, uchar *sdr)
{
   int rv = -1;
   int i, len;

#ifdef METACOMMAND
   if (vend == VENDOR_INTEL) {
      show_oemsdr_intel(sdr); /*show subtypes for Intel BMC_TAM*/
      rv = 0;
   } else if (vend == 4156) {  /*special HP/NewAccess OEM SDR*/
      show_oemsdr_hp(sdr);    
      rv = 0;
   } else if (vend == VENDOR_QUANTA) {
      printf("Quanta: ");
      show_oemsdr_nm(sdr); 
      rv = 0;
   }
#endif
   if (rv != 0) {
      len = sdr[4] + 5;
      if (vend == VENDOR_FUJITSU) printf("Fujitsu: ");
      else if (vend == VENDOR_INTEL) printf("Intel: ");
      else printf("manuf=%d: ",vend);
      for (i = 8; i < len; i++) printf("%02x ",sdr[i]);
      printf("\n");
   }
   return(rv);
}

void
ShowThresh(int flg, uchar bits, uchar *vals, uchar *sdr)
{
   char str[128] = "";
   char part[24]; /* ~15 bytes used */
   double ival;
   char sep[4];
   char *tag;
   tag = "";
   if (fsimple) {
	sprintf(sep,"%c ",bdelim);
	tag = "Thresholds";
   } else sep[0] = 0; /*null string*/
   if (fshowthr == 2) {
	double i0, i1, i2, i3, i4, i5;
        i0 = RawToFloat(vals[0],sdr);
        i1 = RawToFloat(vals[1],sdr);
        i2 = RawToFloat(vals[2],sdr);
        i3 = RawToFloat(vals[3],sdr);
        i4 = RawToFloat(vals[4],sdr);
        i5 = RawToFloat(vals[5],sdr);
	sprintf(str,"%.2f:%.2f:%.2f:%.2f:%.2f:%.2f",i0,i1,i2,i3,i4,i5);
	printf("\t%s%s%s%c",sep,"Thresh ",str,chEol);
   } else if (flg != 0) {  /* Compact, did GetThresholds, reverse order */
	if (bits & 0x20) {
		ival = RawToFloat(vals[5],sdr);
		sprintf(part,"%shi-unrec %.2f ",sep,ival);
		strcat(str,part);
	}
	if (bits & 0x10) {
		ival = RawToFloat(vals[4],sdr);
		sprintf(part,"%shi-crit %.2f ", sep,ival);
		strcat(str,part);
	}
	if (bits & 0x08) {
		ival = RawToFloat(vals[3],sdr);
		sprintf(part,"%shi-noncr %.2f ",sep,ival);
		strcat(str,part);
	}
	if (bits & 0x01) {
		ival = RawToFloat(vals[0],sdr);
		sprintf(part,"%slo-noncr %.2f ",sep,ival);
		strcat(str,part);
	}
	if (bits & 0x02) {
		ival = RawToFloat(vals[1],sdr);
		sprintf(part,"%slo-crit %.2f ", sep,ival);
		strcat(str,part);
	}
	if (bits & 0x04) {
		ival = RawToFloat(vals[2],sdr);
		sprintf(part,"%slo-unrec %.2f ",sep,ival);
		strcat(str,part);
	}
	if (flg == 2) {
	   if (sens_verbose) tag = "Volatile ";
	   printf("\t%s%s%s%c",sep,tag,str,chEol);
	} else 
	   printf("\t%s%s%s%c",sep,tag,str,chEol);
   } else {  /*Full SDR*/
	if (fdebug) printf("ShowThresh[%x]: bits=%02x, sdr18=%02x %02x\n",
				sdr[7],bits,sdr[18],sdr[19]);
	if (bits & 0x20) {
		ival = RawToFloat(vals[0],sdr);
		sprintf(part,"%shi-unrec %.2f ",sep,ival);
		strcat(str,part);
	}
	if (bits & 0x10) {
		ival = RawToFloat(vals[1],sdr);
		sprintf(part,"%shi-crit %.2f ",sep,ival);
		strcat(str,part);
	}
	if (bits & 0x08) {
		ival = RawToFloat(vals[2],sdr);
		sprintf(part,"%shi-noncr %.2f ",sep,ival);
		strcat(str,part);
	}
	if (bits & 0x01) {
		ival = RawToFloat(vals[5],sdr);
		sprintf(part,"%slo-noncr %.2f ",sep,ival);
		strcat(str,part);
	}
	if (bits & 0x02) {
		ival = RawToFloat(vals[4],sdr);
		sprintf(part,"%slo-crit %.2f ",sep,ival);
		strcat(str,part);
	}
	if (bits & 0x04) {
		ival = RawToFloat(vals[3],sdr);
		sprintf(part,"%slo-unrec %.2f ",sep,ival);
		strcat(str,part);
	}
	if (sens_verbose) tag = "SdrThres ";
	printf("\t%s%s%s%c",sep,tag,str,chEol);
	if (sens_verbose)
	{ 	/* show max/min  & hysteresis from full sdr */
		str[0] = 0;
		ival = RawToFloat(sdr[31],sdr);
		sprintf(part,"%snom %.2f ",sep,ival);
		strcat(str,part);
		ival = RawToFloat(sdr[32],sdr);
		sprintf(part,"%snmax %.2f ",sep,ival);
		strcat(str,part);
		ival = RawToFloat(sdr[33],sdr);
		sprintf(part,"%snmin %.2f ",sep,ival);
		strcat(str,part);

		ival = RawToFloat(sdr[34],sdr);
		sprintf(part,"%ssmax %.2f ",sep,ival);
		strcat(str,part);
	 
		ival = RawToFloat(sdr[35],sdr);
		sprintf(part,"%ssmin %.2f ",sep,ival);
		strcat(str,part);
	   
#ifdef OLD
		ival = RawToFloat(sdr[42],sdr);
		sprintf(part,"%s+hyst %.2f ",sep,ival);
		strcat(str,part);

		ival = RawToFloat(sdr[43],sdr);
		sprintf(part,"%s-hyst %.2f ",sep,ival);
		strcat(str,part);
#endif
	
		printf("\t%s%c",str,chEol);
	}
   }  /*endif full sdr*/
}

int decode_comp_generic(uchar type, uchar evtype, uchar num, ushort reading)
{
	int istr = 0;
    uchar b;
	   /* decode via evtype */
       switch(evtype)
	   {
	   case 0x02:	
		  if (reading & 0x01) istr = 85; /* Active */
		  if (reading & 0x02) istr = 86; /* Busy */
		  else istr = 84;  /* Idle (OK)) */
	   case 0x03:
		  if (reading & 0x01) istr = 13; /* Asserted */
		  else istr = 0;  /* "OK" Deasserted */
	      break;
	   case 0x04:
		  if (reading & 0x01) istr = 15; /* Predictive Failure */
		  else istr = 0;  /* "OK" Deasserted */
	      break;
	   case 0x05:
		  if (reading & 0x01) istr = 65; /* Limit Exceeded*/
		  else istr = 0;  /* "OK" LimitNotExceeded*/
	      break;
	   case 0x07:	/* transition */
		  b = bitnum(reading);
		  switch(b) {
		  case 0: istr = 0; break; /*no bits set, OK*/
		  case 1: istr = 87; break; /*transition up to Non-Critical*/
		  case 2: istr = 88; break; /*transition up to Critical */
		  case 3: istr = 89; break; /*transition up to Non-recoverable */
		  case 4: istr = 87; break; /*transition down to Non-Critical */
		  case 5: istr = 88; break; /*transition down to Critical */
		  case 6: istr = 89; break; /*transition to Non-recoverable */
		  case 7: istr = 90; break; /*Monitor*/
		  case 8: istr = 91; break; /*Informational*/
		  default: istr = 8; break; /*'OK*'*/
		  }
	      break;
	   case 0x08:
		  if (reading & 0x01) istr = 9; /* Present */
		  else istr = 10;  /*Absent*/
	      break;
	   case 0x09:	/*  Availability event states */
		  if (reading & 0x01) istr = 17; /* Enabled */
		  else istr = 16;  /*Disabled*/
	      break;
	   case 0x0A:	/* */
		  b = (reading & 0x7f);
		  switch(b) {
		  case 0x00: istr = 8;  break; /* 00 'OK*'*/
		  case 0x01: istr = 92; break; /* transition to Running */
		  case 0x02: istr = 93; break; /* transition to In Test */
		  case 0x04: istr = 94; break; /* transition to Power Off */
		  case 0x08: istr = 95; break; /* transition to On Line */
		  case 0x10: istr = 96; break; /* transition to Off Line */
		  case 0x20: istr = 97; break; /* transition to Off Duty */
		  case 0x40: istr = 98; break; /* transition to Degraded */
		  case 0x80: istr = 99; break; /* transition to Power Save */
		  default: istr = 100; break; /* Install Error */
		  }
	      break;
	   case 0x0B:	/* Redundancy */
		  b = (reading & 0x7f);
		  switch(b) {
		  case 0x00: istr = 8;  break; /* 00 'OK*'*/
		  case 0x01: istr = 18; break; /* 01 Fully Redundant */
		  case 0x02: istr = 19; break; /* 02 Redundancy Lost */
		  case 0x04: istr = 20; break; /* 04 Redundancy Degraded */
		  case 0x08: istr = 82; break; /* 08 Non-Redundant/Sufficient down */
		  case 0x10: istr = 82; break; /* 10 Non-Redundant/Sufficient up*/
		  case 0x20: istr = 83; break; /* 20 Non-Redundant/Insufficient */
		  case 0x40: istr = 20; break; /* 40 Redundancy Degraded down */
		  default: istr = 20; break; /* Redundancy Degraded up */
		  }
	      break;
	   case 0x0C:	/* ACPI Power States */
		  if (reading & 0x04)      istr = 21; /* D3, Off */
		  else if (reading & 0x02) istr = 23; /* D2, Sleeping */
		  else if (reading & 0x01) istr = 22; /* D1, Working */
		  else istr = 24;  /*D0, On*/
	      break;
	   default:
	      if (fdebug) 
		    printf("sensor[%x] et %02x type %02x not decoded, reading = %04x\n",
				   num,evtype,type,reading);
	      istr = STR_OTHER;  /* other " - " */
	      break;
	   }
	return(istr);
}

/* 
 * decode_comp_reading
 * 
 * Decodes the readings from compact SDR sensors.
 * Use sensor_dstatus array for sensor reading types and meaning strings.
 * Refer to IPMI Table 36-1 and 36-2 for this.
 * 
 * Note that decoding should be based on sensor type and ev_type only,
 * except for end cases.
 * 
 * Note reading1 = sens_reading[2], reading2 = sens_reading[3]
 */
int 
decode_comp_reading(uchar type, uchar evtype, uchar num, 
		    uchar reading1, uchar reading2)
{
   int istr = 0;  /*string index into sensor_dstatus[] */
   uchar b;
   ushort reading;
   static char customstr[35];

   /* usually reading2 has h.o. bit set (0x80). */
   reading = reading1 | ((reading2 & 0x7f) << 8);

   switch(type)
   {
	case 0x01:	/*Discrete Thermal, Temperature*/
	   if (fdebug) 
	      printf("Discrete Thermal snum %x evtype=%x reading=%x\n",
				num,evtype,reading);
	   if (evtype == 0x07) {
		   b = bitnum(reading);
		   if (b == 0) istr = 8; /*no bits set, "OK*"*/
		   else if (b < 3) istr = 0; /*bits 1,2 "OK"*/
		   else istr = 65; /*transition to error, Limit Exceeded*/
	   } else if (evtype == 0x05) {
		   /* see CPU1 VRD Temp on S5000, snum 0xc0 thru 0xcf */
		   if (reading & 0x01) istr = 65; /* Limit Exceeded*/
		   else istr = 0;  /* "OK" LimitNotExceeded*/
	   } else if (evtype == 0x03) {
		   if (reading & 0x01) istr = 13; /* Asserted */
		   else istr = 0;  /* "OK" Deasserted */
	   } else {   /* evtype == other 0x05 */
		   if (reading & 0x01) istr = 0; /* 8="OK*", 0="OK" */
		   else istr = 5; /*state asserted, Crit-hi */
	   } 
	   break;
	case 0x02:	/*Discrete Voltage*/
	   { /* evtype == 0x05 for VBat, 0x03 for VR Watchdog */
	      if (reading & 0x01) istr = 65; /*LimitExceeded, was Crit-hi*/
              else istr = 0; /* "OK" LimitNotExceeded*/ 
	   }
	   break;
	case 0x04:	/*Fan*/
	   if (evtype == 0x0b) {  /*redundancy*/
	      b = reading & 0x3f;
	      if (b == 0x00) istr = 16;   /*sensor disabled*/
	      else if (b == 0x01) istr = 18;  /*fully redundant*/
	      else if (b == 0x02) istr = 19; /*redundancy lost*/
	      else if (b == 0x0b) istr = STR_AC_LOST; /*ac lost*/
	      else istr = 20; /*redundancy degraded*/
	   } else if (evtype == 0x08) {  /*presence*/
              if (reading & 0x02) istr = 9;   /*Present/Inserted*/
              else if (reading & 0x01) istr = 10; /*Absent/Removed*/
              else /*reading==00*/ istr = 47; /*Unused*/
	   } else if (evtype == 0x03) {  /*PS Fan Fail*/
	      if (reading == 0) istr = 0; /*OK*/
	      else istr = 12;  /*faulty*/
	   } else {  /*other evtype*/
	      b = reading & 0x0f;
	      if (b == 0) istr = 12;  /*faulty*/
              else if (b & 0x01) istr = 11;  /*ready*/
	      else istr = 41;   /*Unknown*/
	   }
	   break;
	case 0x05:	/*Physical Security, Chassis */
	   if (reading == 0) istr = 0; /*OK*/
	   else if (reading & 0x01) istr = 38; /*chassis intrusion*/
	   /* 0x02 Drive bay intrusion */
	   /* 0x04 IO area intrusion */
	   /* 0x08 Processor area intrusion */
	   else if (reading & 0x10) istr = 37; /*lan leash lost*/
	   /* 0x20 Dock/Undock */
	   /* 0x40 Fan area intrusion */
	   else istr = 41; /* Unknown, was bitnum(reading); */
	   break;
	case 0x07:	/*Processor Status - 0x80 is OK/Present */
	   b = bitnum(reading);
	   if (evtype == 0x03) {
              if (b <= 1) istr = 0; /*bit1 Deasserted, OK* */
	      else istr = 13; /*bit2 Asserted*/  
	   } else {   /*usu 0x6f*/
              if (b > 10) istr = 41; /*Unknown*/
              else if (b == 0) istr = 0;  /*OK*/
              else istr = 47 + b;  /*Proc strings 48 thru 57*/
	   }
	   break;
	case 0x08:	/*Power Supply*/
	   b = reading & 0x7f;
	   if (b == 0) istr = 10;  /*absent*/
	   else if (b & 0x40) istr = STR_PS_CONFIG; /*Config Err*/
	   else if (b & 0x08) istr = STR_AC_LOST; /*AC Lost*/
	   else if (b & 0x04) istr = 15; /*Predictive Fail*/
	   else if (b & 0x02) istr = STR_PS_FAIL; /*PS Fail*/
	   else if (b & 0x01) istr =  9; /*present*/
	   break;
	case 0x09:	/*Power Unit*/
	   b = reading & 0x3f;
	   if (evtype == 0x0b) {  /*Power Redundancy*/
	      if (b == 0x00) istr = 16;   /*sensor disabled*/
	      else if (b == 0x01) istr = 18;  /*fully redundant*/
	      else if (b == 0x02) istr = 19; /*redundancy lost*/
	      else if (b == 0x0b) istr = STR_AC_LOST; /*ac lost*/
	      else istr = 20; /*redundancy degraded*/
	   } else {  /* Power Unit (evtype==0x6f or 0xef) */
	      if (b == 0) istr = 17;   /*enabled*/
	      else if ((b & 0x01) == 1) istr = 16;  /*disabled*/
	   }
	   break;
	case 0x0C:	/* Memory */
	   b = reading & 0x3f;
           if (b == 0) istr = 0;  /*OK*/
           else if (b & 0x01) istr = 8;  /*Correctable ECC (OK*)*/
           else if ((b & 0x02) || (b & 0x20)) istr = 39;  /*ECC Error*/
           else if (b & 0x04) istr = 40;  /*Parity Error*/
	   else istr = bitnum(b);  /* ECC or other error */
	   break;
	case 0x0D:	/* drive slot - usually HSC sens_ownid == 0xc0 */
	   if (fRomley || fGrantley) {  /* evtype==0x6f, has both status and presence */
	      if (reading & 0x02) istr = 12; /*Faulty*/
	      else if (reading & 0x80) istr = STR_REBUILDING; /*Rebuilding*/
	      else if (reading & 0x01) istr = 9; /*Present (OK)*/
	      else istr = 10; /*Absent*/
	   } else {
	      if (evtype == 8) {  /* HSC slot presence sensors (num > 8)*/
                 if (reading1 & 0x02) istr = 9;   /*Present/Inserted*/
                 else if (reading1 & 0x01) istr = 10; /*Absent/Removed*/
                 else /*reading1==00*/ istr = 47; /*Unused*/
	      } else { /* HSC slot status sensors (evtype==0x6f)*/
                 /* usually reading2 == 0x82 or 0x8E if healthy */
                 if (reading2 & 0x01) istr = 12; /*state8=Rebuild stopped*/
                 else if (reading2 & 0x02) istr = 11; /*state9=Inserted/Ready */
                 else if (reading2 & 0x04) istr = 11; /*state10=Safe_to_Remove*/
                 else if (reading2 & 0x08) istr = 11; /*state11=Ready */
                 else if (reading2 == 0x80) istr = 47; /*no states, Unused*/
                 else istr = 12;  /*faulty*/
                 b = 8;  /*if no bits set, no raid state */
                 if (reading1 & 0x01) { b = 0; } /*state0=Faulty*/
                 else if (reading1 & 0x02) b = 1; /*state1=Rebuilding*/
                 else if (reading1 & 0x04) b = 2; /*state2=InFailedArray*/
                 else if (reading1 & 0x08) b = 3; /*state3=InCriticalArray*/
                 else if (reading1 & 0x10) b = 4; /*state4=ParityCheck*/
                 else if (reading1 & 0x20) b = 5; /*state5=PredictedFault*/
                 else if (reading1 & 0x40) b = 6; /*state6=Un-configured*/
                 else if (reading1 & 0x80) b = 7; /*state7=HotSpare*/
                 if (b < 8) { 
		     /* also include a raid_state, via custom string */
		     sprintf(customstr,"%s %s",
				sensor_dstatus[istr], raid_states[b]);
		     istr = STR_CUSTOM;
		     sensor_dstatus[istr] = customstr;
		     if (fdebug) printf("dstatus=%s\n",sensor_dstatus[istr]);
                 }
              } /*end-else-if HSC slot status (0x6f)*/
           } 
	   break;
	case 0x10:	/*Event Logging*/
	   /* usu evtype==0x6f*/
	   b = bitnum(reading & 0x3f);
	   switch (b) {
	      case 0x00:  istr = 0; break; /*OK*/
	      case 0x01:  istr = 59; break; /*MemLogDisabled*/
	      case 0x02:  istr = 60; break; /*TypLogDisabled*/
	      case 0x03:  istr = 61; break; /*LogCleared*/
	      case 0x04:  istr = 62; break; /*AllLogDisabled*/
	      case 0x05:  istr = 63; break; /*SelFull*/
	      case 0x06:  istr = 64; break; /*SelNearFull*/
	      default:  istr = 41; break;  /*Unknown*/
	   }
	   break;
	case 0x12:	/*System Event*/
	   if (reading == 0) istr = 0;
	   else istr = 13; /*Asserted*/  
	   break;
	case 0x13:	/*Critical Interrupt*/
	   /* valid bits:  0x03FF */
	   if (reading == 0) istr = 0;  /*OK*/
	   else {   
	        b = bitnum(reading);  /* ECC or other error */
		if (b > 10) b = 10;
		istr = 24 + b;
	   }
	   break;
	case 0x14:	/*Button*/
	   if (reading == 0) istr = 0; /*OK*/
	   else istr = 13; /*Asserted*/  
	   break;
	case 0x15:	/*Module/ Board */
	   if (evtype == 0x08) {  /*presence*/
              if (reading & 0x02) istr = 9;   /*Present/Inserted*/
              else if (reading & 0x01) istr = 10; /*Absent/Removed*/
              else /*reading==00*/ istr = 47; /*Unused*/
	   }
	   break;
	case 0x16:	/* HSBP Status (esp. Romley) */
	   if (reading & 0x010) istr = STR_HSC_OFF; /*Offline*/
	   else istr = 0;  /*OK*/
	   break;
	case 0x17:	/* ATCA CDM, Air Filter, Filter Tray */
	   if (reading == 0) istr = 0; /*OK*/
	   else if (reading & 0x01) istr = 10; /*Absent*/
	   else istr = bitnum(reading);  /* other error, TODO: fix this */
	   break;
	case 0x1C:	/*Terminator (usu SCSI)*/
	   if (reading & 0x01) istr = 9; /*present*/
	   else istr = 10;        /*missing,absent*/
	   break;
	case 0x21:	/*DIMM memory slot*/
	   if ((reading & 0x04) != 0) istr = 9; /*present*/
	   else istr = 10; /*absent*/
	   sprintf(customstr,"%s", sensor_dstatus[istr]);
	   if ((reading & 0x01) != 0) strcat(customstr,",Fault");
	   if ((reading & 0x0100) != 0) strcat(customstr,",Disabled");
	   istr = 58; /*use a custom string*/
	   sensor_dstatus[istr] = customstr;
	   if (fdebug) printf("dstatus=%s\n",sensor_dstatus[istr]);
	   break;
	case 0x22:	/*ACPI Power State*/
           b = bitnum(reading); 
           switch(b) {
	     case 0: istr = 0;  break; /*OK*/
	     case 1: istr = 22;  break; /*Working*/
	     case 2:
	     case 3:
	     case 4:
	     case 5:
	     case 9:
	     case 10: istr = 23;  break; /*Sleeping*/
	     case 6: 
	     case 7: 
	     case 8: 
	     case 11: 
	     case 12: istr = 24;  break; /*On*/
	     case 13: istr = 21;  break; /*Off*/
	     default: istr = 41;   /*unknown*/
           }
	   break;
	case 0x23:	/*Watchdog*/
	   if (reading == 0) istr = 0;
	   else istr = 13; /*Asserted*/  
	   break;
	case 0x24:	/*Platform Alert*/
           b = bitnum(reading); 
           switch(b) {
	     case 0: istr = 0;   break; /*OK, no bits set*/
	     case 1: istr = 66;  break; /*Page, bit 0 set*/
	     case 2: istr = 67;  break; /*LAN,  bit 1 set*/
	     case 3: istr = 68;  break; /*PET*/
	     case 4: istr = 69;  break; /*SNMP OEM*/
	     default: istr = 70;   /*None*/
           }
	   break;
	case 0x25:	/* Entity Presence */
	   if (reading & 0x01) istr = 8;  /*Present*/
	   else if (reading & 0x02) istr = 9;  /*Absent*/
	   else if (reading & 0x04) istr = 16;  /*Disabled*/
	   else istr = 42;  /* NotAvailable */
	   break;
	case 0x28:	/* BMC FW Health */
	   if (evtype == 0x6F) {  /*Sensor-specific*/
	     if (reading == 0) istr = 0;  /*OK*/
	     else istr = 12; /*Faulty*/
	   } else {  /*use event/reading type*/
	     istr = decode_comp_generic(type, evtype, num, reading);
	   }
	   break;
	case 0x29:	/* Battery */
           switch(reading & 0x7f) {
	     case 0x00: istr = 0;  break; /*OK*/
	     case 0x01: istr = 15; break; /*Predict Fail*/
	     case 0x04: istr = 9;  break; /*Present*/
	     case 0x02: 
	     default:   istr = 12; break; /*Failed/Faulty*/
           }
	   break;
	case 0x2A:	/* Session Audit (IPMI 2.0) */
	   if (reading == 0x00) istr = 45; /*Activated*/
	   else istr = 46;                 /*Deactivated*/
	   break;
	case 0x2B:	/* Version Change */
	   b = bitnum(reading1); 
	   switch(b) {
	     case 0: istr = 0;   break; /*OK, no bits set*/
	     case 1: istr = 72;  break; /*HW Changed, bit 0 set*/
	     case 2: istr = 73;  break; /*SW Changed,  bit 1 set*/
	     case 3: istr = 74;  break; /*HW incompatibility*/
	     default: istr = 75; break; /*Change error*/
	   }
	   break;

        /* sensor types 0xC0 - 0xFF are OEM RESERVED */
	case 0xF1:	/* ATCA IPMB-0 Sensor */
	   if ((reading & 0x7fff) == 0x0008) istr = 0; /*OK*/
	   else istr = bitnum(reading1);  /* other error, TODO: refine this */
	   break;
	case 0xC0:	/* SMI State, NMI State */
	case 0xD8:	/* BIST */
	case 0xF0:	/* ATCA FRU HotSwap, TODO: refine this */
	case 0xF2:	/* ATCA Module HotSwap, TODO: refine this */
	case 0xF3:	/* SMI Timeout, etc. */
	   if (reading & 0x01) istr = 13; /* Asserted */
	   else istr = 0;  /* "OK", Deasserted */
	   break;

	case 0x60:	/* SCSI 1 Term Flt */
	case 0x61:	/* SCSI 2 Term Flt */
   	default:
	   istr = decode_comp_generic(type, evtype, num, reading);
	   break;
   }
   return(istr);
}  /* end decode_comp_reading */

#define STYPSZ  15
static char *get_stype_str(uchar stype)
{   /*return sensor type string, with fixed length */
    static char stype_str[STYPSZ+1];
    char *tmpstr;
    int i, n;
    tmpstr = get_sensor_type_desc(stype); 
    n = strlen_(tmpstr);
    if (n > STYPSZ) n = STYPSZ;
    strncpy(stype_str,tmpstr,n);
    for (i = n; i < STYPSZ; i++) stype_str[i] = ' ';
    stype_str[i] = 0;
    tmpstr = stype_str;
    return(tmpstr);
}

void
ShowSDR(char *tag, uchar *sdr)
{
  SDR01REC *sdr01;
  SDR02REC *sdr02;
  SDR08REC *sdr08;
  SDR11REC *sdr11;
  SDR12REC *sdr12;
  SDR14REC *sdr14;
  SDRc0REC *sdrc0;
  char idstr[32];
  char *typestr = NULL;
  int vend;
  int len, ilen, i, j;
  int ioff;
  uchar sens[4];
  uchar sens_cap;
  uchar shar_cnt;
  int rc; 
  double val;
  char brearm;
  uchar sep[4];
  char rdgstr[50];

  len = sdr[4] + 5;
  sens_cap = 0x80;  /*ignore*/
  if (fdebug) printf("ShowSDR: len=%d, type=%x\n",len,sdr[3]);
  memset(sens,0,4);
  if (frawsdr || fdebug) {
	  /* raw is different than dump_buf */
	  printf("raw SDR: ");
	  for (i = 0; i < len; i++)
		  printf("%02x ",sdr[i]);
	  printf("\n");
  }
  strcpy(idstr,"INIT");  /*always set idstr to some initial string*/
  switch(sdr[3])
  {
    case 0x01:   /* Full sensor record */
	sdr01 = (SDR01REC *)sdr;
	ioff = 48;
	if (ioff > len) {
		if (fdebug) printf("bad length: type=%x, len=%d, ioff=%d\n",
					sdr[3],len,ioff);
		fprintf(stderr,"Bad SDR Length %d, please apply the correct FRU/SDR diskette\n",len);
		return;
	}
	sens_cap = sdr[11];  /*sdr01->sens_capab*/
	// ilen = (sdr[ioff] & 0x1f);  /*sdr01->id_typelen*/
	ilen = len - ioff;
	if (fdebug) printf("SDR[%x] Full ioff=%d idTypLen=0x%02x ilen=%d\n", 
			sdr01->recid, ioff,sdr[ioff] ,ilen);
	if (ilen >= sizeof(idstr)) ilen = sizeof(idstr) - 1;
	if (ilen <= 0) {  /*bug if true*/
	   fprintf(stderr,"Bad SDR Length %d, omits ID string\n",len);
	   ilen = 16;  /*less than sizeof(idstr)*/
	}
	memcpy(idstr,&sdr[ioff],ilen);
	for (i=ilen; i<16; i++) { idstr[i] = ' '; ilen++; }
	idstr[ilen] = 0;  /* stringify */
	if ((sdr01->sens_capab & 0x40) == 0) brearm = 'm'; /*manual rearm*/
	else brearm = 'a';  /*automatic rearm*/
	if (fdebug) printf("entity %d.%d, idlen=%d sizeof=%lu idstr0=%c s0=%x\n",
					sdr01->entity_id, sdr01->entity_inst, 
					ilen,sizeof(SDR01REC),idstr[0],sdr[ioff]);
	rc = GetSensorReading(sdr01->sens_num,sdr01,sens);
	if (rc != 0) { /* if rc != 0, leave sens values zero */
	   i = 41;  /* Unknown */
	   val = 0;
	   if (rc == 0xCB) {  /*sensor not present*/
	        i = 10;  /* Absent */
	        typestr = "na";
	   } else typestr = decode_rv(rc);
	} else {
	   j = (sens[2] & 0x3f);   /*sensor reading state*/
	   i = bitnum((ushort)j);  /*sensor_dstatus index*/
	   if (fdebug) 
		printf("bitnum(%02x)=%d raw=%02x init=%x base/units=%x/%x\n",
			sens[2],i,sens[0],sens[1],sdr01->sens_base,
			sdr01->sens_units);
	   if ((sens[1] & 0x20) != 0) { i = 7; val = 0; } /* Init state */
       else if (sdr01->sens_units == 0xC0) i = 42; /*reading NotAvailable*/
	   else if (sens[2] == 0xc7) { i = 10; val = 0;  /* Absent (Intel) */
			if (fdebug) printf("sensor[%x] is absent (c7), no reading\n", 
							sdr01->sens_num);
	   }
	   else val = RawToFloat(sens[0],sdr);
	   typestr = get_unit_type(sdr01->sens_units, sdr01->sens_base, 
					sdr01->sens_mod, fsimple);
#ifdef WRONG
	   if (is_romley(vend_id,prod_id) && 
		(sdr01->sens_type == 0x0C) && (sdr01->sens_units & 0x01))
	   { /* Intel Memory Thermal Throttling %, raw 0x01 == 0.5 % */
               val = (val / 2);  /* handle MTT SDR errata */
	   } /*correct solution is to fix the SDR m-value instead */
#endif
	}
	rc = decode_oem_sensor(sdr,sens,oem_string,sizeof(oem_string));
	if (rc == 0) {
	   if (fsimple) strncpy(rdgstr,oem_string,sizeof(rdgstr));
	   else snprintf(rdgstr,sizeof(rdgstr),"%02x %s",sens[0],oem_string);
	} else {
	   if (fsimple) 
	      snprintf(rdgstr,sizeof(rdgstr),"%s %c %.2f %s",
			sensor_dstatus[i],bdelim,val,typestr);
	   else 
	      snprintf(rdgstr,sizeof(rdgstr),"%02x %s %.2f %s",
			sens[0], sensor_dstatus[i],val,typestr);
	}
	sep[0] = 0; /*null string*/
    printf("%s", tag);
	if (fsimple) {
	   sprintf(sep,"%c ",bdelim);
	   printf("%04x %c Full    %c %s %c %02x %c %s %c %s%c",
			sdr01->recid, bdelim, bdelim,
			get_stype_str(sdr01->sens_type), 
			bdelim, sdr01->sens_num,bdelim, idstr,
			bdelim,rdgstr,chEol);
	} else
	   printf("%04x SDR Full %02x %02x %02x %c %02x snum %02x %s = %s%c",
		sdr01->recid, sdr01->rectype, sdr01->ev_type,
		sdr01->sens_ownid, brearm, sdr01->sens_type, sdr01->sens_num, 
		idstr, rdgstr, chEol);
	if (fdebug && fshowthr)
		   printf("cap=%02x settable=%02x, readable=%02x\n",
			  sens_cap,sdr[19],sdr[18]);
	if (sens_verbose)   /* if -v, also show Entity ID */
	    printf("\t%sEntity ID %d.%d (%s), Capab: %s%c",
		   sep, sdr01->entity_id, sdr01->entity_inst, 
		   decode_entity_id(sdr01->entity_id), // sens_cap, 
		   decode_capab(sens_cap),chEol);
	if (fshowthr && (sens_cap & 0x0f) != 0x03) {
		uchar thresh[7];
		/* Thresholds, so show them */
		/* Use settable bits to show thresholds, since the 
		 * readable values will be off for Full SDRs.  
		 * If cant set any thresholds, only show SDR thresholds */
		if (sdr[19] == 0) rc = 1; 
		else {
		   /* Show volatile thresholds. */
		   rc = GetSensorThresholds(sdr01->sens_num,&thresh[0]);
		   if (rc == 0) ShowThresh(2,thresh[0],&thresh[1],sdr);
		}
		/* Show SDR non-volatile thresholds. */
		if (sens_verbose || rc !=0) ShowThresh(0,sdr[18],&sdr[36],sdr); 
		// ShowThresh(0,0x3f,&sdr[36],sdr); /* to show all %%%% */
	} 
	if (fwrap) {  /* (chEol != '\n') include time */
		time_t ltime;
		time(&ltime);
		if (fsimple) 
	 	   printf("%c %s",bdelim,ctime(&ltime)); /*ctime has '\n' */
		else
	 	   printf("at %s",ctime(&ltime)); /*ctime has '\n' */
	} 
	break;
    case 0x02:   /* Compact sensor record */
	sdr02 = (SDR02REC *)sdr;
	ioff = 32;
	if (ioff > len) {
		if (fdebug) printf("bad length: type=%x, len=%d, ioff=%d\n",
					sdr[3],len,ioff);
		fprintf(stderr,"Bad SDR Length, please apply the correct FRU/SDR diskette\n");
		return;
	}
	sens_cap = sdr[11];  /*sdr02->sens_capab*/
	shar_cnt = sdr02->shar_cnt & 0x0f;  /*sdr[23]*/
	ilen = len - ioff;
	if ((ilen+1) >= sizeof(idstr)) ilen = sizeof(idstr) - 2;
	memcpy(idstr,&sdr[ioff],ilen);
	if ((shar_cnt > 1) && (sdr02->shar_off & 0x80) != 0) { /*do shared SDR*/
	   j = (sdr02->shar_off & 0x7f);  /*sdr[24] = modifier offset*/
	   if (fdebug) printf("share count = %d, mod_offset = %d\n",shar_cnt,j);
	   if ((sdr02->shar_cnt & 0x10) != 0) { /*alpha*/ 
	      idstr[ilen++] = 0x40 + j;  /* j=1 -> 'A' */
	      idstr[ilen] = 0;  /* stringify */
	   } else { /*numeric*/
	      sprintf(&idstr[ilen],"%d",j);
	      ilen = strlen_(idstr);
	   }
	} /* else normal idstr */
	for (i=ilen; i<16; i++) { idstr[i] = ' '; ilen++; }
	idstr[ilen] = 0;  /* stringify */
        if ((sdr02->sens_capab & 0x40) == 0) brearm = 'm'; /*manual rearm*/
        else brearm = 'a';  /*automatic rearm*/
	if (fdebug) printf("ilen=%d, istr0=%c, sizeof=%zu, s0=%x\n",
				ilen,idstr[0],sizeof(SDR02REC),sdr[ioff]);
	memset(sens,0,sizeof(sens));
	rc = GetSensorReading(sdr02->sens_num,sdr02,sens);
	if (rc != 0) { /* if rc != 0, leave sens values zero */
	  i = 41;  /* Unknown */
	  val = 0;
	  if (rc == 0xCB) {  /*sensor not present*/
	        i = 10;  /* Absent */
		typestr = "na";
	  } else typestr = decode_rv(rc);
	} else {
          if ((sens[1] & 0x20) != 0) i = 42; /*init state, NotAvailable*/
	  else {
	    rc = decode_oem_sensor(sdr,sens,oem_string,sizeof(oem_string));
	    if (rc == 0) i = STR_OEM;
	    else i = decode_comp_reading(sdr02->sens_type,sdr02->ev_type,
					sdr02->sens_num,sens[2],sens[3]);
          }
        }
	if (fdebug) 
            printf("snum %x type %x evt %x reading %02x%02x i=%d rc=%d %s\n",
			sdr02->sens_num,sdr02->sens_type,sdr02->ev_type,
			sens[3],sens[2],i,rc, decode_rv(rc));
	j = sens[2] | ((sens[3] & 0x7f) << 8); /*full reading, less h.o. bit*/
	sep[0] = 0; /*null string*/
        printf("%s", tag);
	if (fsimple) {
		sprintf(sep,"%c ",bdelim);
	        printf("%04x %c Compact %c %s %c %02x %c %s %c %s %c%c",
			sdr02->recid, bdelim, bdelim,
			get_stype_str(sdr02->sens_type), 
			bdelim, sdr02->sens_num, bdelim, idstr,
			bdelim, sensor_dstatus[i],bdelim,chEol);
	} else if (i == STR_OEM) {
	   // idstr[ilen] = 0; /*cut out padding in idstr*/
	   printf("%04x SDR Comp %02x %02x %02x %c %02x snum %02x %s = %04x %s%c",
		sdr02->recid, sdr02->rectype, sdr02->ev_type,
		sdr02->sens_ownid, brearm, sdr02->sens_type, sdr02->sens_num, 
                idstr, j, sensor_dstatus[i],chEol);
                // sensor_dstatus[i] == oem_string
	} else {
	   printf("%04x SDR Comp %02x %02x %02x %c %02x snum %02x %s = %04x %s%c",
		sdr02->recid, sdr02->rectype, sdr02->ev_type,
		sdr02->sens_ownid, brearm, sdr02->sens_type, sdr02->sens_num, 
		idstr, j, sensor_dstatus[i],chEol);
                /* idstr, sens[0], sens[1], sens[2], sens[3], */ 
	}
	if (fdebug && fshowthr)
		   printf("cap=%02x settable=%02x, readable=%02x\n",
			  sens_cap,sdr[19],sdr[18]);
	if (fshowthr)  /*also show Entity ID */
	    printf("\t%sEntity ID %d.%d (%s), Capab: %s%c",
		   sep, sdr02->entity_id, sdr02->entity_inst, 
		   decode_entity_id(sdr02->entity_id), // sens_cap, 
		   decode_capab(sens_cap),chEol);
	if (fshowthr &&
	    ((sens_cap & 0x80) == 0) && (sens_cap & 0x0C) != 0) {
		uchar thresh[7];
		/* Thresholds, show them */
		/* Use readable bits to get & show thresholds */
		if (sdr[20] != 0) {
		   rc = GetSensorThresholds(sdr02->sens_num,&thresh[0]);
		   if (rc == 0) ShowThresh(1,thresh[0],&thresh[1],sdr);
		}
	}
	if (fwrap) {  /*include time and \n */
		time_t ltime;
		time(&ltime);
		if (fsimple) 
	 	   printf("%c %s",bdelim,ctime(&ltime)); /*ctime has '\n' */
		else
	 	   printf("at %s",ctime(&ltime)); /*ctime has '\n' */
	} 
	break;
    case 0x03:   /* Event-only sensor record, treat like Compact SDR */
	sdr02 = (SDR02REC *)sdr;
	ioff = 17;
	if (ioff > len) {
		fprintf(stderr,"Bad SDR %x Length %d. Please apply the correct FRU/SDR diskette\n",
			sdr02->recid, len);
		return;
	}
	if (!fsimple)
	{
	ilen = len - ioff;
	if (ilen >= sizeof(idstr)) ilen = sizeof(idstr) - 1;
	memcpy(idstr,&sdr[ioff],ilen);
	for (i=ilen; i<16; i++) { idstr[i] = ' '; ilen++; }
	idstr[ilen] = 0;  /* stringify */
	sens_cap = sdr[11];
	memset(sens,0,sizeof(sens));
        if ((sdr02->sens_capab & 0x40) == 0) brearm = 'm'; /*manual rearm*/
        else brearm = 'a';  /*automatic rearm*/
	// rc = GetSensorReading(sdr02->sens_num,sdr02,sens);
	/* EvtOnly SDRs do not support readings. 
	 * GetSensorReading would return ccode=0xCB (not present), 
	 * but this skips error msg */
	rc = 0xCB;
	i = bitnum((ushort)sens[2]);
	j = sens[2] | ((sens[3] & 0x7f) << 8);
        printf("%s",tag);
	printf("%04x SDR EvtO %02x %02x %02x %c %02x snum %02x %s = %04x %s\n",
		sdr02->recid, sdr02->rectype, sdr02->reclen, 
		sdr02->sens_ownid, 'a', sdr[10], sdr02->sens_num, 
		idstr, j,  sensor_dstatus[i]);
		// sens[0], sens[1], sens[2], sens[3], sensor_dstatus[i]
	}
	break;
    case 0x08:   /* Entity Association record */
	sdr08 = (SDR08REC *)sdr;
	if (!fsimple)
	{
        printf("%s",tag);
	printf("%04x SDR EntA %02x %02x %02x %02x %02x: ",
		sdr08->recid, sdr08->rectype, sdr08->reclen, 
		sdr08->contid, sdr08->continst, sdr08->flags);
	for (i = 0; i < 8; i++) printf("%02x ",sdr08->edata[i]);
	printf("\n");
	}
	break;
    case 0x09:   /* Device-relative Entity Association record */
        sdr08 = (SDR08REC *)sdr;  /*but SDR09 is 26 bytes*/
        if (!fsimple)
        {
        printf("%s",tag);
        printf("%04x SDR DEnt %02x %02x %02x %02x %02x %02x %02x: ",
                sdr08->recid, sdr08->rectype, sdr08->reclen,
                sdr08->contid, sdr08->continst, sdr08->flags,
                sdr08->edata[0], sdr08->edata[1]);
        /*display 2 of 4 contained entity devices edata[2-10] */
        for (i = 2; i < 8; i++) printf("%02x ",sdr08->edata[i]);
        printf("\n");
        }
        break;
    case 0x10:   /* Generic Device Locator record */
	sdr11 = (SDR11REC *)sdr;
	ioff = 16;
	if (ioff > len) { 
	    if (fdebug) printf("SDR %x bad length: type=%x, len=%d, ioff=%d\n",
				sdr11->recid, sdr[3],len,ioff);
            return;
        }
	if (!fsimple)
	{
	ilen = len - ioff;
	if (ilen >= sizeof(idstr)) ilen = sizeof(idstr) - 1;
	memcpy(idstr,&sdr[ioff],ilen);
	idstr[ilen] = 0;  /* stringify */
        printf("%s", tag);
	if (fsimple)
		printf("DevLocator record[%x]%c device %02x %c %s\n",
			sdr11->recid, bdelim,sdr11->dev_slave_adr,bdelim,idstr);
	else
	printf("%04x SDR DLoc %02x %02x dev: %02x %02x %02x %02x %02x %02x %s\n",
		sdr11->recid, sdr11->rectype, sdr11->reclen,
		sdr11->dev_access_adr, sdr11->dev_slave_adr, 
		sdr11->access_lun, sdr[8], sdr[10], sdr[11],
		idstr);
        }
	break;
    case 0x11:   /* FRU record */
	sdr11 = (SDR11REC *)sdr;
	ioff = 16;
	if (ioff > len) {
	    if (fdebug) printf("SDR %x bad length: type=%x len=%d ioff=%d\n",
				sdr11->recid, sdr[3],len,ioff);
		printf("Please apply the correct FRU/SDR diskette\n");
		return;
	}
	if (!fsimple)
	{
	ilen = len - ioff;
	if (ilen >= sizeof(idstr)) ilen = sizeof(idstr) - 1;
	memcpy(idstr,&sdr[ioff],ilen);
	idstr[ilen] = 0;  /* stringify */
	if (fdebug) printf("ilen=%d, istr0=%c, sizeof=%zu, s0=%x\n",
				ilen,idstr[0],sizeof(SDR11REC),sdr[ioff]);
        printf("%s", tag);
	if (fsimple)
		printf("FRU record[%x]: device %02x : %s\n",
			sdr11->recid, sdr11->dev_slave_adr,idstr);
	else
	printf("%04x SDR FRU  %02x %02x dev: %02x %02x %02x %02x %02x %02x %s\n",
		sdr11->recid, sdr11->rectype, sdr11->reclen,
		sdr11->dev_access_adr, sdr11->dev_slave_adr /*fru_id*/, 
		sdr11->access_lun, sdr11->chan_num,
		sdr11->entity_id, sdr11->entity_inst, 
		idstr);
	}
	break;
    case 0x12:   /* IPMB record */
	sdr12 = (SDR12REC *)sdr;
	ioff = 16;
	if (ioff > len) {
		if (fdebug) printf("bad length: type=%x, len=%d, ioff=%d\n",
					sdr[3],len,ioff);
		printf("Please apply the correct FRU/SDR diskette\n");
		return;
	}
	if (!fsimple)
	{
	ilen = len - ioff;
	if (ilen >= sizeof(idstr)) ilen = sizeof(idstr) - 1;
	memcpy(idstr,&sdr[ioff],ilen);
	idstr[ilen] = 0;  /* stringify */
	if (fdebug) printf("ilen=%d, istr0=%c, sizeof=%zu, s0=%x\n",
				ilen,idstr[0],sizeof(SDR12REC),sdr[ioff]);
        printf("%s", tag);
	if (fsimple)
		printf("IPMB record[%x]%c addr %02x %02x %c %s\n",
			sdr12->recid, bdelim,sdr12->dev_slave_adr, 
			sdr12->chan_num,bdelim,idstr);
	else
	printf("%04x SDR IPMB %02x %02x dev: %02x %02x %02x %02x %02x %s\n",
		sdr12->recid, sdr12->rectype, sdr12->reclen,
		sdr12->dev_slave_adr, sdr12->chan_num, sdr12->dev_capab,
		sdr12->entity_id, sdr12->entity_inst, 
		idstr);
	}
	break;
    case 0x14:   /* BMC Message Channel Info record */
	sdr14 = (SDR14REC *)sdr;
	if(!fsimple){
        printf("%s", tag);
	printf("%04x SDR BMsg %02x %02x: ",
		sdr14->recid, sdr14->rectype, sdr14->reclen );
	for (i = 0; i < 8; i++) printf("%02x ",sdr14->mdata[i]);
	printf("%s %s %02x\n",decode_itype(sdr14->mint),
		decode_itype(sdr14->eint), sdr14->rsvd);
	}
	break;
    case 0xc0:   /* OEM SDR record (manuf_id 343. = Intel) */
	sdrc0 = (SDRc0REC *)sdr;
	if(!fsimple)
	{
	  vend = sdrc0->manuf_id[0] + (sdrc0->manuf_id[1] << 8) 
		+ (sdrc0->manuf_id[2] << 16);
          printf("%s",tag);
	  printf("%04x SDR OEM  %02x %02x ", 
		sdrc0->recid, sdrc0->rectype, sdrc0->reclen);
	  show_oemsdr(vend,sdr);
	}
	break;
    default:
	sdrc0 = (SDRc0REC *)sdr;
	/* also saw type = 0x08 & 0x14 on STL2s */
	if (!fsimple){
          printf("%s", tag);
	  printf("%04x SDR type=%02x ", sdrc0->recid, sdr[3]);
	  for (i = 0; i < len; i++) printf("%02x ",sdr[i]);
	  printf("\n");
	}
  }
  return;
}

static int
ShowPowerOnHours(void)
{
	uchar resp[MAX_BUFFER_SIZE];
	int sresp = MAX_BUFFER_SIZE;
	uchar cc;
	int rc = -1;
	int i;
	unsigned int hrs;
  
	if (fmBMC) return(0);
	if (fsimple) return(0);
	sresp = MAX_BUFFER_SIZE;
	memset(resp,0,6);  /* default response size is 5 */
	rc = ipmi_cmd_mc(GET_POWERON_HOURS, NULL, 0, resp, &sresp, &cc, fdebug);
	if (rc == 0 && cc == 0) {
	   /* show the hours (32-bits) */
	   hrs = resp[1] | (resp[2] << 8) | (resp[3] << 16) | (resp[4] << 24);
	   /*60=normal, more is OOB, so avoid div-by-zero*/ 
	   if ((resp[0] <= 0) || (resp[0] >= 60)) i = 1; 
	   else {
		i = 60 / resp[0];
		hrs = hrs / i;
	   }
	   printf("     SDR IPMI       sensor: Power On Hours \t   = %d hours\n",
				hrs);
	}
	if (fdebug) {
		   printf("PowerOnHours (rc=%d cc=%x len=%d): ",rc,cc,sresp);
		   if (rc == 0) 
		   for (i = 0; i < sresp; i++) printf("%02x ",resp[i]);
		   printf("\n");
	}
	return(rc);
}

int SaveThreshold(int id, int sensor_num, int sensor_lo, int sensor_hi, 
		 uchar *thr_set)
{
    int rv = 0;
    char lostr[20];
    char histr[20];
    FILE *fd;

    /* persist the thresholds by re-applying with ipmiutil sensor commands.*/
    if (thr_set != NULL) {
      sprintf(lostr,"-u 0x%02x%02x%02x%02x%02x%02x",
		sensor_thr[0], sensor_thr[1], sensor_thr[2], 
		sensor_thr[3], sensor_thr[4], sensor_thr[5]);
      histr[0] = 0;  /*empty string*/
    } else {
      if (sensor_lo != 0xff) {
	  sprintf(lostr,"-l 0x%02x",sensor_lo);
      } else      lostr[0] = 0;
      if (sensor_hi != 0xff) {
	  sprintf(histr,"-h 0x%02x",sensor_hi);
      } else      histr[0] = 0;
    }
    fd = fopen(savefile,"a+");
    if (fd == NULL) return(-1);
    fprintf(fd, "ipmiutil sensor -i 0x%04x -n 0x%02x %s %s\n", id, sensor_num,
		lostr,histr);
    fclose(fd);
    return(rv);
}

#ifdef NOT_USED
#define PICMG_GET_ADDR_INFO  0x01
static int get_picmg_addrinfo(uchar a1, uchar a2, uchar *addrdata)
{
   uchar idata[5];
   uchar rdata[16];
   int rlen;
   ushort icmd;
   uchar ilen, cc;
   int rv;
   
   idata[0] = 0x00;
   idata[1] = 0x00;
   idata[2] = 0x03;
   idata[3] = a1;  /* 01 thru 0f */
   idata[4] = a2;  /* 00, 01 thru 09 */
   ilen = 5;
   rlen = sizeof(rdata);
   icmd = PICMG_GET_ADDR_INFO | (NETFN_PICMG << 8);
   rv = ipmi_cmd_mc(icmd, idata, ilen, rdata, &rlen, &cc, fdebug);
   if (rv == 0 && cc != 0) rv = cc;
   if (rv == 0) {
       if (fdebug) {
	  printf("picmg_addr(%02x,%02x)",a1,a2);
	  dump_buf("picmg_addr",rdata,rlen,0);
       }
       memcpy(addrdata,rdata,rlen); 
   }
   return(rv);
}
#endif

#ifdef WIN32
static int get_filesize(char *fileName, ulong *psize)
{
    int rv;
    WIN32_FILE_ATTRIBUTE_DATA   fileInfo;

    if (fileName == NULL) return -1;
    if (psize == NULL) return -1;
    rv = GetFileAttributesEx(fileName, GetFileExInfoStandard, (void*)&fileInfo);
    if (!rv) return -1;
    *psize = (long)fileInfo.nFileSizeLow;
    return 0;
}
#endif

int write_sdr_binfile(char *binfile)
{
      uchar *pbuf = NULL;
      FILE *fp;
      int len, ret;
      ret = get_sdr_cache(&pbuf); /* sets nsdrs, sz_sdrs */
      if (ret == 0) {
		fp = fopen(binfile,"wb");
    	if (fp == NULL) {
    	   ret = get_LastError();
    	   printf("Cannot open file %s for writing, error %d\n",binfile,ret);
    	} else {
    	   printf("Writing SDR size %d to %s  ...\n",sz_sdrs,binfile);
    	   len = (int)fwrite(pbuf, 1, sz_sdrs, fp);
    	   fclose(fp);
    	   if (len <= 0) {
    	      ret = get_LastError();
    	      printf("Error %d writing file %s\n",ret,binfile);
    	   } else ret = 0;
    	}
    	free_sdr_cache(pbuf);
      }
      return(ret);
}

int read_sdr_binfile(char *binfile, uchar **pbufret, int *buflen)
{
      uchar *pbuf = NULL;
      FILE *fp;
      int len;
      int ret;
#ifdef WIN32
      {
	 ulong flen;
	 ret = get_filesize(binfile, &flen);
	 if (ret == 0) len = flen;
	 else {
	   ret = get_LastError();
	   printf("Cannot get file size for %s, error %d\n",binfile,ret);
	   return(ret);
	 } 
      }
#endif
      fp = fopen(binfile,"rb");
      if (fp == NULL) {
    	 ret = get_LastError();
    	 printf("Cannot open file %s, error %d\n",binfile,ret);
    	 return(ret);
      } 
      fseek(fp, 0L, SEEK_SET);
#ifndef WIN32
      { /*not windows but Linux, etc.*/
	 struct stat st;
	 /* use fstat to get file size and allocate buffer */
	 ret = fstat(fileno(fp), &st);
	 len = st.st_size;  /*file size in bytes*/
	 if (ret != 0) {
	    ret = get_LastError();
	    printf("Cannot stat file %s, error %d\n",binfile,ret);
	    return(ret);
	 }
      }
#endif
      /* Could estimate size for nsdrs*SDR_SZ, but we don't yet know nsdrs.
       * It is better to use the real file size detected above. */
      sz_sdrs = len;
      pbuf = malloc(len);
      if (fdebug) printf("sdr_binfile: malloc(%d) pbuf=%p\n",len,pbuf);
      if (pbuf == NULL) {
    	  ret = -2;
    	  fclose(fp);
    	  return(ret);
      }
      psdrcache = pbuf;
      /*ok, so proceed with restore*/
      ret = 0;
      len = (int)fread(pbuf, 1, sz_sdrs, fp);
      if (len <= 0) {
    	 ret = get_LastError();
    	 printf("Error %d reading file %s\n",ret,binfile);
    	 sz_sdrs = 0;  /*for safety*/
      } else if (len < sz_sdrs) {
    	 /* Show error if this happens in Windows */
    	 ret = get_LastError();
    	 printf("truncated fread(%s): attempted %d, got %d, error %d\n",
    		binfile,sz_sdrs,len,ret);
         ret = 0; /*try to keep going*/
      }
      fclose(fp);
      if (fdebug) {
		 printf("SDR buffer from file (len=%d,sz=%d)\n",len,sz_sdrs);
		 dump_buf("SDR buffer",pbuf,len,1);
      }
    *pbufret = pbuf;
    *buflen = len;
    return(ret);
}

#ifdef ALONE
#ifdef WIN32
int __cdecl 
#else
int 
#endif
main(int  argc, char **argv)
#else
/* METACOMMAND or libipmiutil */
int i_sensor(int argc, char **argv)
#endif
{
   int ret, rv;
   int c;
   int recid, recnext;
   uchar sdrdata[MAX_BUFFER_SIZE];
   uchar devrec[16];
   int sz, i, j;
   int fsetfound = 0;
   int iloop, irec;
   int ipass, npass;
   uchar *pset;
   char *p;
   char *s1;

   printf("%s version %s\n",progname,progver);

   while ( (c = getopt( argc, argv,"a:bcd:ef:g:h:i:j:k:l:m:n:opqrstu:vwxT:V:J:L:EYF:P:N:R:U:Z:?")) != EOF )
      switch(c) {
	  case 'a':   /* reArm sensor number N */
		if (strncmp(optarg,"0x",2) == 0) frearm = htoi(&optarg[2]);
		else frearm = htoi(optarg);  /*was atoi()*/
		break;
	  case 'c': fsimple = 1;   break;  /* Canonical/simple output*/
	  case 'd': fdump = 1;      /* Dump SDRs to a file*/
		    binfile = optarg; break;
	  case 'b': fchild = 1;    break;  /* Bladed, so get child SDRs */   
	  case 'e': fchild = 1;    break;  /* Extra bladed child SDRs */   
	  case 'f': frestore = 1;      /* Restore SDRs from a file*/
		    binfile = optarg; break;
	  case 's': fsimple = 1;   break;  /* Simple/canonical output */
		    /*fcanonical==fsimple*/
	  case 'g': 
		rv = get_group_id(optarg);
		if (rv < 0) { 
		    printf("Unrecognized sensor type group (%s)\n",optarg);
		    ret = ERR_BAD_PARAM;
		    goto do_exit;
		} else fshowgrp = rv;
		if (fdebug) printf("num sensor type groups = %d\n",fshowgrp);
		break;
	  case 'i': 
		fshowidx = 1;  
		get_idx_range(optarg);
		break;
	  case 'j': fjumpstart = 1;      /* Load SDR cache from a file*/
		    binfile = optarg; break;
	  case 'k': loopsec = atoi(optarg); break;  /*N sec between loops*/
	  case 'm': /* specific MC, 3-byte address, e.g. "409600" */
		    g_bus = htoi(&optarg[0]);  /*bus/channel*/
		    g_sa  = htoi(&optarg[2]);  /*device slave address*/
		    g_lun = htoi(&optarg[4]);  /*LUN*/
		    if (optarg[6] == 's') {
			     g_addrtype = ADDR_SMI;  s1 = "SMI";
		    } else { g_addrtype = ADDR_IPMB; s1 = "IPMB"; }
		    fset_mc = 1;
		    printf("set MC at %s bus=%x sa=%x lun=%x\n",
			    s1,g_bus,g_sa,g_lun);
		    break;
	  case 'o': 
		fgetmem = 1;
		break;
	  case 'n': 
		if (strncmp(optarg,"0x",2) == 0) i = htoi(&optarg[2]);
		else i = htoi(optarg);      /*was atoi()*/
		sensor_num = (uchar)i;
		printf("sensor_num = 0x%x\n",sensor_num);
		break;
	  case 'h':   /* highest threshold */
		if (strncmp(optarg,"0x",2) == 0) {
			i = htoi(&optarg[2]);
			sensor_hi = (uchar)i;
			fsetthresh = 1;
		} else {
			sensor_hi_f = atof(optarg);
			fsetthresh = 2;  /*indicates float conversion*/
		}
		break;
	  case 'l':    /* lowest threshold */
		if (strncmp(optarg,"0x",2) == 0) {
			i = htoi(&optarg[2]);
			sensor_lo = (uchar)i;
			fsetthresh = 1;
		} else {
			sensor_lo_f = atof(optarg);
			fsetthresh = 2;  /*indicates float conversion*/
		}
		break;
	  case 'p': fsavethresh = 1;  break;
	  case 'q': fshowthr = 2; fwrap = 1; break;
	  case 'r': frawsdr = 1;   break;
	  case 't': fshowthr = 1;  break;
	  case 'v': fshowthr = 1; sens_verbose = 1; break;
	  case 'u':    /* specify unique thresholds in hex or float */
		/* raw hex format: 0xLNLCLUHNHCHU, all 6 required */
		if (strncmp(optarg,"0x",2) == 0) {  /*raw hex thresholds*/
		   sensor_thr[0] = htoi(&optarg[2]);  /*lo noncrit*/
		   sensor_thr[1] = htoi(&optarg[4]);  /*lo crit*/
		   sensor_thr[2] = htoi(&optarg[6]);  /*lo unrec*/
		   sensor_thr[3] = htoi(&optarg[8]);  /*hi noncrit*/
		   sensor_thr[4] = htoi(&optarg[10]); /*hi crit*/
		   sensor_thr[5] = htoi(&optarg[12]); /*hi unrec*/
		   /* validate sensor threshold ordering */
		   rv = validate_thresholds(&sensor_thr[0],0,NULL);
		   if (rv == 0) {
		      sensor_lo = sensor_thr[0];
		      sensor_hi = sensor_thr[3];
		      fsetthresh = 3;  /*indicates unique raw thresholds */
		   } else {
		      ret = ERR_BAD_PARAM;
		      goto do_exit;
		   }
		} else {  
		   /* assume float input thresholds, with ':' separator*/
		   /* format LN:LC:LU:HN:HC:HU */
		   sz = strlen_(optarg);
		   p = &optarg[0];
		   for (i = 0; i < 6; i++) sensor_thrf[i] = THR_EMPTY;
		   j = 0;
		   for (i = 0; i <= sz; i++) {
		      if (j >= 6) break;
		      switch(optarg[i]) {
			case ':':
			case '\n':
			case '\0':
			   optarg[i] = 0;
			   if (p[0] == 0) sensor_thrf[j] = THR_EMPTY;
			   else sensor_thrf[j] = atof(p);
			   if (i+1 < sz) p = &optarg[i+1];
			   j++;
			   break;
			default:
			   break;
		      }
		   }
		   /* validate sensor threshold ordering later */
		   // rv = validate_thresholds(&sensor_thrf[0],1,NULL);
		   // if (rv == 0) {
		      sensor_lo_f = sensor_thrf[0];
		      sensor_hi_f = sensor_thrf[3];
		      fsetthresh = 4;  /*indicates unique float thresholds */
		   // } else {
		      // ret = ERR_BAD_PARAM;
		      // goto do_exit;
		   // }
		} /*end-else -u float thresholds*/
		break;
	  case 'w': fwrap = 1;   break;
	  case 'x': fdebug = 1;  break;
	  case 'L':      /* Loop */
		nloops = atoi(optarg);
		fdoloop = 1;
		break;
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
	  default:   /*usage*/
	     printf("Usage: %s [-abcdefghijlmnprstuvwxL -NUPREFTVYZ]\n",progname);
	     printf("where -x      shows eXtra debug messages\n");
	     printf("      -a snum reArms the sensor (snum) for events\n");
	     printf("      -b      show Bladed child MCs for PICMG (same as -e)\n");
	     printf("      -c      displays a simpler, Canonical output fmt\n");
	     printf("      -d file Dump SDRs to a binary file\n");
	     printf("      -e      show Every bladed child MC for PICMG\n");
	//   printf("      -f file Restore SDRs from a binary dump file\n");
	     printf("      -g fan  show only this sensor type group\n");
	     printf("      -h tval specifies the Highest threshold to set\n");
	     printf("      -i id   only show these sensor id numbers\n");
	     printf("      -j file Jump-start SDR cache from a binary file\n");
	     printf("      -k K    If -L, wait K sec between loops (default=1)\n");
	     printf("      -l tval specifies the Lowest threshold to set\n");
	     printf("      -m002000 specific MC (bus 00,sa 20,lun 00)\n");
	     printf("      -n snum specifies the sensor Number to set hi/lo\n");
	     printf("      -o      output memory DIMM information\n");
	     printf("      -p      persist the threshold being set\n");
	     printf("      -q      shows threshold values in d:d:d format\n");
	     printf("      -r      show Raw SDR bytes\n");
	     printf("      -s      displays a Simpler output format\n");
	     printf("      -t      shows Threshold values in text format\n");
	     printf("      -u thr  set Unique threshold values (e.g. 3:2:1:48:49:50)\n");
	     printf("      -v      Verbose: thresholds, max/min, hysteresis\n");
	     printf("      -w      Wrap thresholds on sensor line\n");
	     printf("      -L n    Loop n times every k seconds (default k=1)\n");
	     print_lan_opt_usage(0);
	     ret = ERR_USAGE;
	     goto do_exit;
      }
    if (fjumpstart && fchild) {
       printf("Cannot use -j jumpstart cache with -c child SDRs\n");
       ret = ERR_BAD_PARAM;
       goto do_exit;
    }
    fremote = is_remote();
#ifndef WIN32
    if (fremote == 0) {  
	/* only run this as superuser for accessing IPMI devices. */
	i = geteuid();
	if (i > 1) {
	    printf("Not superuser (%d)\n", i);
	    /* Show warning, but could be ok if /dev/ipmi0 is accessible */
	    //ret = ERR_NOT_ALLOWED;
	    //goto do_exit;
	}
    } 
#endif
    if (fremote) {
	if (!fprivset) {
	  /* on many systems, getting the SDR Reservation ID requires admin */
	  /* if ((fsetthresh != 0) || (frearm != 0))  also require admin */
	  parse_lan_options('V',"4",0);
	}
    }

   ret = ipmi_getdeviceid(devrec,sizeof(devrec),fdebug);
   if (ret == 0) {
	uchar ipmi_maj;
	uchar ipmi_min;
	char *pstr;
	ipmi_maj = devrec[4] & 0x0f;
	ipmi_min = devrec[4] >> 4;
	if ((devrec[1] & 0x80) == 0x80) fdevsdrs = 1;
	vend_id = devrec[6] + (devrec[7] << 8) + (devrec[8] << 16);
	prod_id = devrec[9] + (devrec[10] << 8);
	if (vend_id == VENDOR_NSC) { /*NSC mBMC*/ 
		pstr = "mBMC";
		fmBMC = 1;
		fdevsdrs = 0;
	} else if (vend_id == VENDOR_INTEL) { /*Intel BMC*/ 
		/*Intel Sahalee BMC */
		pstr = "BMC";
		fmBMC = 0;
		if (is_romley(vend_id,prod_id)) fRomley = 1;
		if (is_grantley(vend_id,prod_id)) fGrantley = 1;
		if (prod_id == 0x003E || fRomley || fGrantley)  /*Urbanna,CG2100*/
		   set_max_kcs_loops(URNLOOPS); /*longer KCS timeout*/
	} else if ((vend_id == VENDOR_SUPERMICRO)
                || (vend_id == VENDOR_SUPERMICROX)) {
                   set_max_kcs_loops(URNLOOPS); /*longer KCS timeout*/
	} else if (vend_id == 16394) { /*Pigeon Point*/ 
		fbadsdr = 1;   /* SDR has bad sa/mc value, so ignore it */
	} else {   /* Other products */
		pstr = "BMC";
		fmBMC = 0;
		if (vend_id == VENDOR_NEC) fdevsdrs = 0;
	}
	show_devid( devrec[2],  devrec[3], ipmi_maj, ipmi_min);
		// "-- %s version %x.%x, IPMI version %d.%d \n", pstr,
   } else {
	goto do_exit;
   }

   ret = ipmi_getpicmg( devrec, sizeof(devrec),fdebug);
   if (ret == 0) fpicmg = 1;
   /*if not PICMG, some vendors override to SDR Rep*/
   fdevsdrs = use_devsdrs(fpicmg);
   if (fdevsdrs) printf("supports device sdrs\n");
   npass = 1;
   if (g_sa != 0) {
      /* target a specific MC via IPMB (usu a picmg blade) */
      ipmi_set_mc(g_bus,g_sa,g_lun,g_addrtype);
      fchild = 0;  /* no children, only the specified address */
   } else { 
#ifdef PICMG_CHILD
      /* fchild set above if -b is specified to get Blade child SDRs */
      /* npass = 2 will get both SdrRep & DevSdr passes on CMM */
      if (fpicmg && fdevsdrs) {
	 npass = 2;
	 g_addrtype = ADDR_IPMB;
      }
#endif
      g_sa = BMC_SA;
   }

   if (fgetmem) {
      if (fremote) printf("Cannot get memory DIMM information remotely.\n");
      else {
	 int msz;
	 char desc[80];
	 char szstr[25];
	 ret = -1;
	 for (j = 0; j < 1; j++) {
	    for (i = 0; i < 16; i++) {
	       rv = get_MemDesc(j, i, desc,&msz);
	       if (rv == 0) {
		  if (msz == 0) strcpy(szstr,"not present");
		  else if (msz & 0x8000)
		       sprintf(szstr,"size=%dKB",(msz & 0x7FFF));
		  else sprintf(szstr,"size=%dMB",msz);
		  printf("Memory Device (%d,%d): %s : %s\n",
			 j,i,desc,szstr);
		  ret = 0;
	       }
	    }
	 }
      } /*end-else*/
      goto do_exit;
   }

   if (fdump) {
	  ret = write_sdr_binfile(binfile);
      goto do_exit;
   } /*endif fdump*/

   if (frestore) {
      uchar sdr[MAX_BUFFER_SIZE];
      ushort id;
      int slen;
      uchar *pbuf = NULL;

      ret = read_sdr_binfile(binfile,&pbuf,&slen);
      if (ret == 0) {   /*successful, so write SDRs */
		 nsdrs = find_nsdrs(pbuf);
		 printf("Ready to restore %d SDRs\n",nsdrs);
		 set_reserve(1);
		 ret = sdr_clear_repo(fdevsdrs);
		 if (ret != 0) {
		    printf("SDR Clear Repository error %d\n",ret);
		    goto do_exit;
		 }
		 id = 0;
		 while(find_sdr_next(sdr,pbuf,id) == 0) {
		    id = sdr[0] + (sdr[1] << 8);
		    if (fdebug) printf("adding SDR[%x]\n",id);
		    set_reserve(1);
		    ret = sdr_add_record(sdr,fdevsdrs);
		    if (ret != 0) {
			printf("SDR[%x] add error %d\n",id,ret);
			break;
		    }
		 } /*end while sdr*/
      }
      if (ret == 0) printf("Restored %d SDRs successfully.\n",nsdrs);
      free_sdr_cache(pbuf); /* does nothing if (pbuf == NULL) */
      goto do_exit;
   } /*endif frestore*/

   if (fjumpstart) {
      uchar *pbuf = NULL;
      int slen;
      ret = read_sdr_binfile(binfile,&pbuf,&slen);
      if (ret != 0) { 
    	 /* Try to dump sdrs to this file if not there */
	     ret = write_sdr_binfile(binfile);
         if (ret == 0) 
      		ret = read_sdr_binfile(binfile,&pbuf,&slen);
         if (ret != 0) {
    		fjumpstart = 0; /*cannot do jumpstart*/
		 }
      } else {  /* set this as the SDR cache */
    	 psdrcache = pbuf;
    	 sz_sdrs = slen;
    	 nsdrs = find_nsdrs(pbuf);
    	 if (fdebug) printf("jumpstart cache: nsdrs=%d size=%d\n",nsdrs,slen);
      }
   } /*endif fjumpstart*/

   for (ipass = 0; ipass < npass; ipass++)
   {
     if (fjumpstart) ; /*already got this above*/
     else {
	ret = GetSDRRepositoryInfo(&j,&fdevsdrs);
	if (fdebug) printf("GetSDRRepositoryInfo: ret=%x nSDRs=%d\n",ret,j);
	if (ret == 0 && j == 0) {
	   printf("SDR Repository is empty\n");
	   goto do_exit;
	}
	nsdrs = j;
     }

     /* show header for SDR records */
     if (fsimple) 
	printf(" ID  | SDRType | Type            |SNum| Name             |Status| Reading\n");
     else 
	printf("_ID_ SDR_Type_xx ET Own Typ S_Num   Sens_Description   Hex & Interp Reading\n");

     if (fwrap) chEol = ' ';
     if (!fdoloop) nloops = 1;
     for (iloop = 0; iloop < nloops; iloop++)
     {
       if (fshowidx) recid = sensor_idx1;
       else recid = 0;
	   irec = 0; /*first sdr record*/
       while (recid != 0xffff) 
       {
	 if (fjumpstart) {
	   if (irec == 0)  /*need sdr_by_id if fshowid recid>0*/
	        ret = find_sdr_by_id(sdrdata,psdrcache,recid);
	   else ret = find_sdr_next(sdrdata,psdrcache,recid);
	   if (ret != 0) {  /*end of sdrs*/
		if (fdebug) printf("find_sdr_next(%04x): ret = %d\n", recid,ret);
		ret = 0; break; 
	   }
	   recnext = sdrdata[0] + (sdrdata[1] << 8);  /*same as recid*/
	   if (fdebug) printf("find_sdr_next(%04x): ret = %d, next=%04x\n",
				recid,ret,recnext);
	   if (recid > 0 && recnext == 0) {
		if (fdebug) printf("Error recid=%04x recnext=%04x\n",recid,recnext);
		ret = 0; break; 
	   }
	   sz = sdrdata[4] + 5;
	 } else {
	   int try;
           for (try = 0; try < 10; try++) {
             ret = GetSDR(recid,&recnext,sdrdata,sizeof(sdrdata),&sz);
             if (fdebug)
               printf("GetSDR[%04x]: ret = %x, next=%x\n",recid,ret,recnext);
             if (ret != 0) {
               if (ret == 0xC5) {  /* lost Reservation ID, retry */
                 /*fprintf(stderr,"%04x lost reservation retrying to get, try: %d,  %d, rlen = %d\n", recid,try,ret,sz);*/
                 os_usleep((rand() & 3000),0);
                 fDoReserve = 1;
               }
               else {
                 fprintf(stderr,"%04x GetSDR error %d, rlen = %d\n", recid,ret,sz);
                 break;
               }
             }
             else {
               break;
             }
             if (try==9){
               sz=0;
              fprintf(stderr,"%04x GetSDR error %d, rlen = %d\n", recid,ret,sz);
             }
           }
           if (sz < MIN_SDR_SZ) {  /* don't have recnext, so abort */
             break;
           } /* else fall through & continue */
	 } /*end-else*/
	 if (ret == 0) {  /* (ret == 0) OK, got full SDR */
	   if (fdebug) {
		dump_buf("got SDR",sdrdata,sz,0);
	   }
	   if (sz < MIN_SDR_SZ) goto NextSdr;
	   /* if recid == 0, get real record id */
	   if (recid == 0) recid = sdrdata[0] + (sdrdata[1] << 8);
	   if (fshowgrp > 0) {
	      for (i = 0; i < fshowgrp; i++) {
		uchar styp;
		if (sdrdata[3] == 0x03) styp = sdrdata[10]; /*EvtOnly*/
		else styp = sdrdata[12];
		if (fdebug) printf("sdrt=%02x styp=%02x sgrp[%d]=%02x\n",
				sdrdata[3],styp,i,sensor_grps[i]);
		if (sdrdata[3] == 0xc0) continue; /*skip OEM SDRs*/
		if (styp == sensor_grps[i]) break;
	      }
	      if (i >= fshowgrp) goto NextSdr;
	   }

	   if ((sensor_num == INIT_SNUM) || (sdrdata[7] == sensor_num) 
	      || fsetthresh) {
	       /* if -n not set or if -n matches, parse and show the SDR */
	       ShowSDR("",sdrdata);
	   }  /* else filter SDRs if not matching -n sensor_num */

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
	   if (fpicmg && fchild && (sdrdata[3] == 0x12)) { /* PICMG MC DLR */
	      int   _recid, _recnext, _sz;
	      uchar _sdrdata[MAX_SDR_SIZE];
	      int   devsdrs_save;
	      uchar cc;

	      /* save the BMC globals, use IPMB MC */
	      devsdrs_save  = fdevsdrs;
	      fdevsdrs = 1;   /* use Device SDRs for the children*/
	      if (fdebug)
		printf(" --- IPMB MC (sa=%02x cap=%02x id=%02x devsdrs=%d):\n",
		       sdrdata[5],sdrdata[8],sdrdata[12],fdevsdrs);
	      fDoReserve = 1;  /* get a new SDR Reservation ID */
	      ipmi_set_mc(PICMG_SLAVE_BUS,sdrdata[5],sdrdata[6],g_addrtype);

	      _sz = 16;
	      ret = ipmi_cmd_mc(GET_DEVICE_ID,NULL,0,_sdrdata,&_sz,&cc,fdebug);
	      if (ret == 0 && cc == 0) {
		/* Get the SDRs from the IPMB MC */
		_recid = 0;
		while (_recid != 0xffff) 
		{
		  ret = GetSDR(_recid,&_recnext,_sdrdata,sizeof(_sdrdata),&_sz);
		  if (ret != 0) {
		     fprintf(stderr,"%04x GetSDR error %d, rlen = %d\n",_recid,ret,_sz);
		     break;
		  }
		  else if (_sz >= MIN_SDR_SZ) 
		     ShowSDR(" ",_sdrdata);

		  if (_recnext == _recid) _recid = 0xffff;
		  else _recid = _recnext;
		} /*end while*/
	      } /*endif ret==0*/

	      /* restore BMC globals */
	      fdevsdrs = devsdrs_save;
	      ipmi_restore_mc();
	      fDoReserve = 1;  /* get a new SDR Reservation ID */
	   }  /*endif fpicmg && fchild*/
#endif

	   if (fdebug) printf("fsetthresh=%d snum=%02x(%02x) sa=%02x(%02x)\n",
			    fsetthresh,sdrdata[7],sensor_num,sdrdata[5],g_sa);
	   if (fsetthresh && (sdrdata[7] == sensor_num) 
		&& (sdrdata[5] == g_sa))  /*g_sa usu is BMC_SA*/
	   {
	     /* setting threshold, compute threshold raw values */
	     if (fsetthresh == 2) {   /*set from float*/
		if (fdebug) 
		   printf("lof=%.2f hif=%.2f\n", sensor_lo_f,sensor_hi_f);
		if (sensor_lo_f != 0) 
		   sensor_lo = FloatToRaw(sensor_lo_f,sdrdata,0);
		if (sensor_hi_f != 0) 
		   sensor_hi = FloatToRaw(sensor_hi_f,sdrdata,0);
	     } else if (fsetthresh == 1) {  /*raw thresholds*/
		if (sensor_hi != 0xff)
		   sensor_hi_f = RawToFloat(sensor_hi,sdrdata);
		if (sensor_lo != 0xff)
		   sensor_lo_f = RawToFloat(sensor_lo,sdrdata);
	     } else if (fsetthresh == 3) {  /*unique raw thresholds*/
		if (sensor_hi != 0xff)
		   sensor_hi_f = RawToFloat(sensor_hi,sdrdata);
		if (sensor_lo != 0xff)
		   sensor_lo_f = RawToFloat(sensor_lo,sdrdata);
	     } else if (fsetthresh == 4) {   /*set unique from float*/
		i = fill_thresholds(&sensor_thrf[0], sdrdata); 
		/* if (i > 0) ;  * filled in some thresholds */
		{  /* always set lo/hi if any are non-zero */
		   for (j = 0; j < 3; j++) {
		      if (sensor_thrf[j] != 0) {
			  sensor_lo_f = sensor_thrf[j]; break; }
		   }
		   for (j = 3; j < 6; j++) {
		      if (sensor_thrf[j] != 0) {
			  sensor_hi_f = sensor_thrf[j]; break; }
		   }
		}
		if (fdebug) 
		   printf("lof=%.2f hif=%.2f\n", sensor_lo_f,sensor_hi_f);
		/* convert thrf (float) to thr (raw) */
		if (sensor_lo_f != 0) {
		   sensor_lo = FloatToRaw(sensor_lo_f,sdrdata,0);
		   sensor_thr[0] = FloatToRaw(sensor_thrf[0],sdrdata,0);
		   sensor_thr[1] = FloatToRaw(sensor_thrf[1],sdrdata,0);
		   sensor_thr[2] = FloatToRaw(sensor_thrf[2],sdrdata,0);
		}
		if (sensor_hi_f != 0) {
		   sensor_hi = FloatToRaw(sensor_hi_f,sdrdata,0);
		   sensor_thr[3] = FloatToRaw(sensor_thrf[3],sdrdata,0);
		   sensor_thr[4] = FloatToRaw(sensor_thrf[4],sdrdata,0);
		   sensor_thr[5] = FloatToRaw(sensor_thrf[5],sdrdata,0);
		}
		/* validate threshold ordering */
		if (validate_thresholds(sensor_thrf,1,sdrdata) != 0) {
		   ret = ERR_BAD_PARAM;
		   goto do_exit; 
		}
	     }
	     {
		printf("\tSetting SDR %04x sensor %02x to lo=%02x hi=%02x\n",
			    recid,sensor_num,sensor_lo,sensor_hi);
		if (recid == 0) fsetfound = 1;
		else fsetfound = recid;
	     }
	   } /*endif fsetthresh */
	 }  /*endif ok, got full SDR */

NextSdr:
	 if (ret == ERR_SDR_MALFORMED) break;
	 if (fjumpstart) recid = recnext;
	 else {
		if (recnext == recid) recid = 0xffff; /*break;*/
		else recid = recnext;
	 }
	 if (fshowidx) {
		/* if we have already read the last in the range, done. */
		if (recid >= sensor_idxN) break; // recnext = 0xffff; // break;
	 }
	     irec++;
       } /*end while recid*/
       if (fdoloop && (nloops > 1)) {
     	 printf("\n"); /* output an empty separator line */
     	 os_usleep(loopsec,0); /*delay 1 sec between loops*/
       }
     } /*end for nloops*/

     if (npass > 1) {   /* npass==2 for PICMG */
	/* Switch fdevsdrs from Device to Repository */
	if (fdevsdrs == 0) fdevsdrs = 1;
	else fdevsdrs = 0;
	fDoReserve = 1;  /* get a new SDR Reservation ID */
     }
   } /*end for npass*/

   if ((fshowidx == 0) && (fshowgrp == 0)) {
      /* use local rv, errors are ignored for POH */
      rv = ShowPowerOnHours();
   }

   if (frearm != 0) {
	ret = RearmSensor((uchar)frearm);
	printf("RearmSensor(0x%02x) ret = %d\n",frearm,ret);
   }

   if (fsetthresh != 0) {
	uchar tdata[7];
	if (fsetfound == 0) {
	   printf("Did not find sensor number %02x.\nPlease enter the sensor number parameter in hex, as it is displayed above.\n",sensor_num);
	}
	ret = GetSensorThresholds(sensor_num,tdata);
	if (ret != 0) goto do_exit; 
#ifdef TEST
	printf("thresh(%02x): %02x %02x %02x %02x %02x %02x %02x %02x\n",
		sensor_num, sensor_num, tdata[0], tdata[1], tdata[2],
		tdata[3], tdata[4], tdata[5], tdata[6]);
	printf("   set(%02x):          %02x       %02x \n",
		sensor_num,sensor_lo,sensor_hi);
#endif
	if (fsetthresh == 3 || fsetthresh == 4) {  
	   /* apply unique sensor thresholds */
	   pset = &sensor_thr[0];
	} else pset = NULL;  /* use just hi/lo */
	ret = SetSensorThresholds(sensor_num,sensor_hi,sensor_lo,tdata,pset);
	printf("SetSensorThreshold[%02x] to lo=%02x(%4.3f) hi=%02x(%4.3f), ret = %d\n",
		sensor_num,sensor_lo,sensor_lo_f,sensor_hi,sensor_hi_f,ret);
	if (fsavethresh && ret == 0) {
	    recid = fsetfound;
	    rv = SaveThreshold(recid,sensor_num,sensor_lo,sensor_hi,pset);
	    if (rv == 0) 
		printf("Saved thresholds for sensor %02x\n",sensor_num);
	}
	fsetthresh = 0;  /*only set threshold once*/
   }

do_exit:
   // if (fjumpstart) 
   free_sdr_cache(psdrcache); /* does nothing if ==NULL*/
   /* show_outcome(progname,ret); *handled in ipmiutil.c*/
   ipmi_close_();
   return(ret);  
}

/* end isensor.c */
