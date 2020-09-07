/*---------------------------------------------------------------------------
 * Filename:   ilan.c  (was pefconfig.c)
 *
 * Author:     arcress at users.sourceforge.net
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * Abstract:
 * This tool sets up the custom Platform Event Filter for the panicSEL 
 * record (0x20, OS Critical Stop) to send a PEF SNMP alert for Linux
 * panics.  It also configures the BMC LAN parameters, which are needed 
 * to support PEF alerts.
 *
 * ----------- Change History -----------------------------------------------
 * 10/16/01 Andy Cress - created
 * 10/24/01 Andy Cress - mods for SetPefEntry(0c)
 * 11/02/01 Andy Cress - added option to disable a given PefEntry
 * 11/15/01 Andy Cress - added function to GetLanEntry
 * 11/19/01 Andy Cress - added function to SetLanEntry
 * 01/18/02 Andy Cress - added GetCfgData function
 * 01/31/02 Andy Cress - converted to use ipmi_cmd_ia
 * 02/06/02 Andy Cress - removed GetCfgData
 * 02/06/02 Andy Cress - added ipmi_cmd_va 
 * 02/08/02 Andy Cress - added GetChanAcc
 * 02/14/02 Andy Cress - added Get_IPMac_Addr()
 * 02/21/02 Andy Cress - added Alert IP/MAC logic to Get_IPMac_Addr()
 * 03/21/02 Andy Cress - do SetChanAcc(0x80) with every SetChanAcc(0x40)
 * 04/15/02 Andy Cress v1.2 - fix bug with user-specified Alert IP:
 *		            (ccode=c7 on SetLanEntry(16 & 18), also
 *		            added Get_Mac() for user-specified Alert IP.
 * 04/16/02 Andy Cress v1.3 added SetUser() to set password if specified,
 *                          also changed unset mac[0] to be 0xFF, not 0.
 * 05/09/02 Andy Cress v1.4 fixed bug 504 gwymac[0]=00 (mymac overwrote it),
 *                          also fixed alert scan to pick last trapsink
 * 05/31/02 Andy Cress v1.5 for gwy mac, changed arping -c 1 to -c 2,
 *                          also get com2sec community from snmpd.conf,
 *                          set dest type for no ack, no retry,
 *                          special handling to set preset PEF entries
 * 07/02/02 Andy Cress v1.6 added more Usage text
 * 07/08/02 Andy Cress v1.7 SetUserAccess length change 
 * 08/02/02 Andy Cress v1.8  moved common ipmi_cmd() code to ipmicmd.c
 * 08/27/02 Andy Cress v1.9 fixed 0xc7 on SETUSER_ACCESS with pefconfig -P "",
 *                          show message if alert dest not found
 * 09/10/02 Andy Cress v1.10 make channel nums into defines (16)
 * 09/17/02 Andy Cress v1.11 decode Dest Addr IP (19) in decimal,
 *                           fix enable user padding in SetUser,
 *                           display new community string when setting
 * 12/09/02 Andy Cress v1.12 fix error w -C & snmpd.conf conflict
 * 01/29/03 Andy Cress v1.13 added MV OpenIPMI support
 * 02/05/03 Andy Cress v1.14 show pef entry descriptions,
 *                           added EnablePef routine
 *                           show correct Alert dest mac if -A only
 * 04/04/03 Andy Cress v1.15 add eth interface option (-i) 
 * 05/13/03 Andy Cress v1.16 fix EnablePef if startup delay not supported
 * 06/19/03 Andy Cress v1.17 added errno.h (email from Travers Carter) 
 * 07/25/03 Andy Cress v1.18 add SerialOverLan configuration
 *                           mod to SetUser, added GetBmcEthDevice,
 *                           use 'arping -I' if eth1.
 * 08/18/03 Andy Cress v1.19 Don't abort if IPMI 1.0, just skip PEF,
 *                           SetLanEntry(10 & 11) for bmc grat arp,
 *                           SetLanEntry(2) to 0x17 for auth priv
 * 09/10/03 Andy Cress v1.20 Don't enable a PEF entry if it is empty,
 *                           added -L lan_ch parameter,
 *                           scan for lan_ch in GetBmcEthDevice
 * 09/22/03 Andy Cress v1.21 Add DHCP option (-D), from Jerry Yu.
 * 12/05/03 Andy Cress v1.22 Fix auth type enables for ServerConfig
 * 12/16/03 Andy Cress v1.23 Allow flexible auth types via authmask
 * 03/19/04 Andy Cress v1.24 Change default pefnum for mBMC to 10
 * 04/15/04 Andy Cress v1.25 Init each response for channel info, avoids
 *                           0xcc error with /dev/ipmi0 due to wrong lan_ch
 * 05/05/04 Andy Cress v1.26 call ipmi_close before exit.  Note that
 *                           Get_IPMac_Addr and GetBmcEthDevice
 *                           routines need more work for WIN32.
 * 05/24/04 Andy Cress v1.27 added CHAN_ACC params for ia64
 * 06/28/04 Andy Cress v1.28 added parsing to get community from trapsink
 * 07/23/04 Andy Cress v1.29 use lan_ch variable to set Alert Policy Table
 * 08/23/04 Andy Cress v1.30 fixed decoding of PE Table entries,
 *                           added -e option (same as no params)
 * 08/25/04 Andy Cress v1.31 added some WIN32 logic to Get_Mac, Get_IPMac_Addr
 * 11/01/04 Andy Cress v1.32 add -N / -R for remote nodes   
 *                           added -U for remote username
 * 11/18/04 Andy Cress v1.33 added -u to configure a lan username (user 2)
 * 11/23/04 Andy Cress v1.34 added pef_defaults if first 11 empty
 * 01/11/05 Andy Cress v1.35 allow scan for BMC LAN if fIPMI10
 * 01/20/05 Andy Cress v1.36 fix to allow IPMI 2.0
 * 02/16/05 Andy Cress v1.37 added IPMI 2.0 VLAN parameters, 
 *                           if DHCP, can set DHCP Server via -I param
 * 03/02/05 Andy Cress v1.38 show Serial-Over-Lan params, 
 *                           fix -L with lan_ch_parm. mods to GetBmcEthDevice
 * 03/18/05 Andy Cress v1.39 fix GetBmcEthDevice for invalid MAC compares
 * 06/03/05 Andy Cress v1.40 For my MAC in BMC, check user-specified, then 
 *			  				 check existing BMC MAC, then check OS MAC.
 * 06/10/05 Andy Cress v1.41 Display multiple Alert Destinations, 
 *                           handle fSOL20 commands
 * 07/07/05 Andy Cress v1.42 Fix GetBmcEthDevice for TIGI2U to skip GCM ch 3
 * 07/08/05 Andy Cress v1.43 Mods to handle Intel NSI2U miniBMC,
 * 08/01/05 Andy Cress v1.44 added -t option to test if BMC LAN configured
 * 08/10/05 Andy Cress v1.45 truncate extra string chars,
 *                           decode more PEF params
 * 09/07/05 Andy Cress v1.46 enable mBMC PEF entries 26 thru 30
 * 04/06/06 Andy Cress v1.47 show "gcm" as ifname if -L 3.
 * 06/20/06 Andy Cress v1.48 fix strcmp(gcm), show all 4 alert policies,
 *                           add PefDesc() for misc vendor pefdesc, add -a.
 * 08/08/06 Andy Cress v1.49 add Alcolu to fsharedMAC
 * 09/29/06 Andy Cress v1.52 use bmcmymac if valid, use bmcmyip if ok, 
 *                           added -q for user number, 
 *                           enhanced Get_IPMac_Addr for Windows
 * 10/12/06 Andy Cress v1.53 FindEthNum updates, always use gwy iface for mac
 * 11/02/06 Andy Cress v1.55 add user names, EnablePef mods for non-Intel.
 * 05/02/07 Brian Johnson v1.65 add fpefenable flag to not do SetPefEntry 
 *                           if no Alert Destination.  Previously did 
 *                           SetPefEntry but not EnablePef in this case.
 * 05/04/07 Andy Cress v1.65 Use 0x02 for DHCP source instead of 0x03,
 *                           fix 1714748 missing "X:" in getopt line
 * 05/23/07 Jakub Gorgolewski 
 *                     v1.66 Use iphlpapi for Windows detection
 * 10/31/07 Andy Cress v2.3  Fixed PEF entry for Power Redundancy Lost
 * 11/15/07 Andy Cress v2.4  Move custom PEF to #14, add to usage,
 *                           Allow broadcast MAC for -X
 * 12/17/07 Andy Cress v2.5  Add fSetPEFOks & secondary Gateway
 */
/*M*
 *---------------------------------------------------------------------------
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
 *---------------------------------------------------------------------------
 *M*/
#ifdef WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "getopt.h"
#elif defined(DOS)
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"
#else
/* Linux or similar */
#include <stdio.h>
#include <stdlib.h>
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
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#endif
#ifdef LINUX
#include <unistd.h>
#endif
#ifdef SOLARIS
#include <sys/sockio.h>
#define SIOCGIFHWADDR   SIOCGENADDR
#define ifr_netmask         ifr_ifru.ifru_addr
// #define ifr_hwaddr.sa_data  ifr_ifru.ifru_enaddr
#elif defined(BSD)
#include <sys/sockio.h>
#define SIOCGIFHWADDR   SIOCGIFMAC
#define ifr_netmask         ifr_ifru.ifru_addr
// #define ifr_hwaddr.sa_data  ifr_ifru.ifru_addr 
#elif defined(MACOS)
#include <sys/sockio.h>
#define ifr_netmask         ifr_ifru.ifru_addr
#endif
#include "ipmicmd.h"
#include "oem_intel.h"
#include "oem_supermicro.h"

#define SELprintf          printf
#define RTF_UP          0x0001	/* route usable */

#define SOL_ENABLE_FLAG			0x01
#define SOL_DISABLE_FLAG		0x00
#define SOL_PRIVILEGE_LEVEL_USER	0x02
#define SOL_PRIVILEGE_LEVEL_OPERATOR	0x03
#define SOL_PRIVILEGE_LEVEL_ADMIN 	0x04
#define SOL_PREFERRED_BAUD_RATE		0x0a	/*115.2k */
/* For IPMI 1.5, use Intel SOL commands & subfunctions */
#define SOL_ENABLE_PARAM		0x01
#define SOL_AUTHENTICATION_PARAM 	0x02
#define SOL_ACC_INTERVAL_PARAM	 	0x03
#define SOL_RETRY_PARAM  	 	0x04
#define SOL_BAUD_RATE_PARAM		0x05	/*non-volatile */
#define SOL_VOL_BAUD_RATE_PARAM		0x06	/*volatile */
/* For IPMI 2.0, use IPMI SOL commands & subfunctions */
#define SOL_ENABLE_PARAM2		0x08
#define SOL_AUTHENTICATION_PARAM2 	0x09
#define SOL_BAUD_RATE_PARAM2		0x11

/* IPMI 2.0 SOL PAYLOAD commands */
#define SET_PAYLOAD_ACCESS  0x4C
#define GET_PAYLOAD_ACCESS  0x4D
#define GET_PAYLOAD_SUPPORT 0x4E

/* Channel Access values */
#define CHAN_ACC_DISABLE   0x20	/* PEF off, disabled */
#define CHAN_ACC_PEFON     0x02	/* PEF on, always avail */
#define CHAN_ACC_PEFOFF    0x22	/* PEF off, always avail */
/* special channel access values for ia64 */
#define CHAN_ACC_PEFON64   0x0A	/* PEF on, always avail, UserLevelAuth=off */
#define CHAN_ACC_PEFOFF64  0x2A	/* PEF off, always avail, UserLevelAuth=off */
#define OS_LINUX          1
#define OS_WINDOWS        2
#define OS_SOLARIS        3
#define OS_BSD            4
#define OS_HPUX           5

   /* TSRLT2 Channels: 0=IPMB, 1=Serial/EMP, 6=LAN2, 7=LAN1 */
   /* S5000 Channels: 0=IPMB, 1=LAN1, 2=LAN2, 3=RMM2, 4=Serial, 6=pci, 7=sys */
   /* For TIGPT1U/mBMC: 1=LAN channel, no serial */
#define LAN_CH   1
#define SER_CH   4
#define MAXCHAN  12		/*was 16, reduced for gnu ipmi_lan */
#define NUM_DEVICES_TO_CHECK		32	/*for GetBmcEthDevice() */
#define MAC_LEN  6		/*length of MAC Address */
#define PSW_LEN  16		/* see also PSW_MAX =20  in ipmicmd.h */
/* Note: The optional IPMI 2.0 20-byte passwords are not supported here, 
 * due to back-compatibility issues. */

   /* IP address source values */
#define SRC_STATIC 0x01
#define SRC_DHCP   0x02		/* BMC running DHCP */
#define SRC_BIOS   0x03		/* BIOS, sometimes DHCP */
#define SRC_OTHER  0x04

/* PEF event severities */
#define PEF_SEV_UNSPEC   0x00
#define PEF_SEV_MON      0x01
#define PEF_SEV_INFO     0x02
#define PEF_SEV_OK       0x04
#define PEF_SEV_WARN     0x08
#define PEF_SEV_CRIT     0x10
#define PEF_SEV_NORECOV  0x20
#define FLAG_INIT  99		/*initial value of char flag, beyond scope */
#define PARM_INIT  0xff

typedef struct
{				/* See IPMI Table 15-2 */
  uchar rec_id;
  uchar fconfig;
  uchar action;
  uchar policy;
  uchar severity;
  uchar genid1;
  uchar genid2;
  uchar sensor_type;
  uchar sensor_no;
  uchar event_trigger;
  uchar data1;
  uchar mask1;
  uchar res[9];
} PEF_RECORD;

typedef struct
{				/* See IPMI Table 19-3 */
  uchar data[36];
} LAN_RECORD;			/*LanRecord */

#ifdef METACOMMAND
extern int get_lan_stats (uchar chan);	/*see bmchealth.c */
extern char *get_sensor_type_desc (uchar stype);	/*from ievents.c */
#endif

#define MYIP   0x01
#define GWYIP  0x02
#define DESTIP 0x04
#define MAXPEF 41		/* max pefnum offset = 40 (41 entries) */
/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil lan";
#else
static char * progver   = "3.08";
static char * progname  = "ilan";
#endif
static char fdebug = 0;
static char fipmilan = 0;
static char fIPMI10 = 0;	/* =1 if IPMI v1.0 or less */
static char fIPMI20 = 0;	/* =1 if IPMI v2.0 or greater */
static char fSOL20 = 1;		/* =1 if use Serial-Over-Lan 2.0 w IPMI 2.0 */
static char fsharedMAC = 0;	/* =1 if special shared-MAC BMC LAN port */
static char fAdjustPefNum = 0;	/* =1 adjust pefnum to first empty index */
static char fUserPefNum = 0;	/* =1 if user specified a valid pefnum value */
static char freadonly = 1;	/* =1 to only read LAN & PEF parameters */
static char fcanonical = 0;	/* =1 to show only canonical output */
static char flansecure = 0;	/* =1 set lan security: no null, cipher0 off */
static char bdelim = BCOLON;	/* delimiter ':' or '|' if canonical output */
static char ftestonly = 0;
static char fprivset = 0;
static char flanstats = 0;	/* =1 to show the IPMI LAN statistics */
static char foptmsg = 0;	/* =1 to show the option warning msg */
static char fshowchan = 0;	/* =1 to show the IPMI channels */
static char nopts = 0;		/* number of pefconfig options specified */
static int nerrs = 0;		/* number of errors during processing */
static int ngood = 0;		/* number of good results */
static int lasterr = 0;		/* value of the last error */
static char fCustomPEF = 0;	/* =1 if -j to input a custom PEF record */
static char fSetPEFOks = 0;	/* =1 if -k to set PEF OK rules */
static char fdisable = 0;
static char fenable = 0;	/* =1 to config BMC LAN and PEF */
static char fpefenable = 0;	/* =1 enable PEF if Alert Dest is specified */
static char fdisableSOL = 0;
static char fgetser = 0;
static char fsetifn = 0;	/* =1 if user specified ifname[] with -i */
static char fethfound = 0;	/* =1 if FindEthNum successful */
static char fset_ip = 0;	/* !=0 if options used to specify an IP addr */
static char fpassword = 0;	/* =1 user-specified a password, so set it. */
static uchar fmBMC = 0;		/* =1 mini-BMC, =0 Sahalee BMC */
static uchar fiBMC = 0;		/* =1 Intel iBMC */
static uchar fRomley = 0;	/* =1 Intel Romley BMC */
static uchar fGrantley = 0;
static uchar fipv6 = 0;		/* =1 if BMC supports IPv6 */
static uchar bmcpefctl = 0;	/* existing BMC PEF Control, !0 = enabled */
static char alertnum = 1;	/* alert dest num (usu 1 thru 4) */
static char alertmax = 9;	/* alert dest num max (usu 4, error if >9) */
static char pefnum = 12;	/* 11 pre-defined entries, adding 12th */
static char pefadd = 0;		/* num PEF rules added (usu 2, could be 5 */
static char pefmax = MAXPEF;	/* 20 for Sahalee, 30 for miniBMC */
static char *myuser = NULL;	/* username to set, specified by -u */
static uchar usernum = 0;	/* set non-zero to specify user number */
static uchar rgmyip[4] = { 0, 0, 0, 0 };
static uchar rggwyip[4] = { 0, 0, 0, 0 };
static uchar rggwy2ip[4] = { 0, 0, 0, 0 };
static uchar rgdestip[4] = { 0, 0, 0, 0 };
static uchar rgsubnet[4] = { 0, 0, 0, 0 };
static uchar bmcsubnet[4] = { 255, 255, 255, 0 };	/* default subnet */
static uchar ossubnet[4] = { 0, 0, 0, 0 };
static uchar osmyip[4] = { 0, 0, 0, 0 };
static uchar bmcmyip[4] = { 0, 0, 0, 0 };
static uchar bmcdestip[4] = { 0, 0, 0, 0 };
static uchar bmcdestmac[6] = { 0xff, 0, 0, 0, 0, 0 };
static uchar bmcgwyip[4] = { 0, 0, 0, 0 };
static uchar bmcgwymac[6] = { 0xff, 0, 0, 0, 0, 0 };
static uchar bmcmymac[6] = { 0xff, 0, 0, 0, 0, 0 };
static uchar rgmymac[6] = { 0xff, 0, 0, 0, 0, 0 };
static uchar osmymac[6] = { 0xff, 0, 0, 0, 0, 0 };
static uchar rggwymac[6] = { 0xff, 0, 0, 0, 0, 0 };
static uchar rggwy2mac[6] = { 0xff, 0, 0, 0, 0, 0 };
static uchar rgdestmac[6] = { 0xff, 0, 0, 0, 0, 0 };
static uchar rgdhcpmac[6] = { 0xff, 0, 0, 0, 0, 0 };
static int nciphers = 16;
static int ncipher0 = 0;
static uchar rgciphers[16] =
  { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static char rghostname[32] = { '\0' };
static uchar custPEF[20];	/* max used = 18 bytes */
static char rgcommunity[19] = "public";	/* default community */
static char fsetcommunity = 0;	/* =1 if user-specified community */
static char passwordData[PSW_MAX + 1] =
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static uchar authmask = 0;	/* usu 0x17, mBMC = 0x15 */
static uchar bAuth = 0x16;	/*exclude auth=None for security */
static uchar arp_interval = 0x04;	/* in 500 ms increments, 0-based */
static uchar arp_ctl = 0x01;	/* 01=grat arp, 02=arp resp, 03=both */
static uchar fsetarp = 0;	/* 1=user-specified arp_ctl */
#ifdef WIN32
static uchar ostype = OS_WINDOWS;	/*windows */
static char ifname[64] = "Local Area Connection ";	/* interface name */
static char ifpattn[25] = "Local Area Connection ";
#elif SOLARIS
/* for i86pc use "e1000g0", but for sun4u(sparc) this might be "eri0" */
static uchar ostype = OS_SOLARIS;
#ifdef __SPARC__
static char ifname[16] = "eri0";	/* SPARC interface name */
static char ifname0[16] = "eri0";
static char ifpattn[14] = "eri";
#else
static char ifname[16] = "e1000g0";	/* Solaris x86 interface name */
static char ifname0[16] = "e1000g0";
static char ifpattn[14] = "e1000g";
#endif
#elif defined(BSD)
static uchar ostype = OS_BSD;
static char ifname[16] = "em0";	/* interface name */
static char ifname0[16] = "em0";
static char ifpattn[14] = "em";
#elif defined(HPUX)
static uchar ostype = OS_HPUX;
static char ifname[16] = "lan0";	/* interface name */
static char ifname0[16] = "lan0";
static char ifpattn[14] = "lan";
#else
static uchar ostype = OS_LINUX;
static char ifname[16] = "eth0";	/* interface name */
static char ifname0[16] = "eth0";
static char ifpattn[14] = "eth";
#endif
static char *pspace1 = "\t";	/*used for fcanonical output */
static char *pspace2 = "\t\t";
static char *pspace3 = "\t\t\t";
static char *pspace4 = "\t\t\t\t";
static int vend_id;
static int prod_id;
static int lan_dhcp = 0;	      /*=1 if using DHCP for bmc lan channel*/
static uchar ser_ch = SER_CH;
static uchar gcm_ch = PARM_INIT;
static uchar failover_enable = PARM_INIT;
static uchar vlan_enable = PARM_INIT;
static uchar vlan_prio = 0;	/*default = 0 */
static ushort vlan_id = 0;	/*max 12 bits used */
static uchar lan_access = 0x04;	/* -v usu 4=Admin, 3=Operator, 2=User */
static uchar lan_user = 0x02;	/* -u if specified, default to user 2 */
static uchar lan_ch_parm = PARM_INIT;	/* -L to set, unused if PARM_INIT */
static uchar lan_ch = LAN_CH;	/* default=LAN_CH=1 */
static uchar max_users = 5;	/* set in GetUser(1); */
static uchar enabled_users = 0;	/* set in GetUser(1); */
static uchar show_users = 5;	/* default, adjusted based on DeviceID  */
static uchar fnewbaud = 0;	/* =1 if user specified baud */
static uchar sol_baud = SOL_PREFERRED_BAUD_RATE;	/*115.2k default */
static uchar sol_accum[2] = { 0x04, 0x32 };
static uchar sol_retry[2] = { 0x06, 0x14 };
static uchar sol_bvalid = 0;	/* =1 if SOL baud is valid */
static uchar chan_pefon = CHAN_ACC_PEFON;
static uchar chan_pefoff = CHAN_ACC_PEFOFF;
static uchar SessInfo[18];	/* Session Info data */
// static uchar  bparm7[3] = {0x00, 0x00, 0x00}; /*ipv4 header before*/
static uchar iparm7[3] = { 0x1E, 0x00, 0x00 };	/*intel ipv4 TTL,Flags,Service */
static uchar oparm7[3] = { 0x40, 0x40, 0x10 };	/*other ipv4 TTL,Flags,Service */
static uchar *parm7 = &oparm7[0];
#define MAX_PEFPARAMS  14	/* max pef params = 14 */
uchar peflen[MAX_PEFPARAMS] = { 0, 1, 1, 1, 1, 1, 21, 2, 1, 4, 17, 1, 3, 18 };	/*for ShowPef */
uchar pef_array[MAXPEF][21];	/* array of all PEF entries read, */
								/* sizeof(PEF_RECORD) = 21  */
uchar pef_defaults[11][21] = {	/* array of first 11 default PEF entries */
  {0x01, 0x80, 1, 1, PEF_SEV_CRIT, 0xff, 0xff, 0x01, 0xff, 0x01, 0x95, 0x0a, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/*Temp */
  {0x02, 0x80, 1, 1, PEF_SEV_CRIT, 0xff, 0xff, 0x02, 0xff, 0x01, 0x95, 0x0a, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/*Volt */
  {0x03, 0x80, 1, 1, PEF_SEV_CRIT, 0xff, 0xff, 0x04, 0xff, 0x01, 0x95, 0x0a, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/*Fan */
  {0x04, 0x80, 1, 1, PEF_SEV_WARN, 0xff, 0xff, 0x05, 0x05, 0x03, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0},	/*Chass */
  {0x05, 0x80, 1, 1, PEF_SEV_WARN, 0xff, 0xff, 0x08, 0xff, 0x6f, 0x06, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /*PS*/ 
  {0x06, 0x80, 1, 1, PEF_SEV_WARN, 0xff, 0xff, 0x0c, 0x08, 0x6f, 0x02, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /*ECC*/ 
  {0x07, 0x80, 1, 1, PEF_SEV_CRIT, 0xff, 0xff, 0x0f, 0x06, 0x6f, 0x01, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /*FRB*/
  {0x08, 0x80, 1, 1, PEF_SEV_WARN, 0xff, 0xff, 0x07, 0xff, 0x6f, 0x1c, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /*POST*/
  {0x09, 0x80, 1, 1, PEF_SEV_CRIT, 0xff, 0xff, 0x13, 0xff, 0x6f, 0x3e, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /*NMI*/
  {0x0a, 0x80, 1, 1, PEF_SEV_INFO, 0xff, 0xff, 0x23, 0x03, 0x6f, 0x0e, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /*WDT*/
  {0x0b, 0x80, 1, 1, PEF_SEV_MON, 0xff, 0xff, 0x12, 0xff, 0x6f, 0x02, 0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0} 	/*Restart */
};

char **pefdesc;
char *pefdesc1[MAXPEF] = {	/* for Sahalee BMC */
/* 0 0x00 */ "",
/* 1 0x01 */ "Temperature Sensor",
/* 2 0x02 */ "Voltage Sensor",
/* 3 0x04 */ "Fan Failure",
/* 4 0x05 */ "Chassis Intrusion",
/* 5 0x08 */ "Power Supply Fault",
/* 6 0x0c */ "Memory ECC Error",
/* 7 0x0f */ "BIOS POST Error",
/* 8 0x07 */ "FRB Failure",
/* 9 0x13 */ "Fatal NMI",
/*10 0x23 */ "Watchdog Timer Reset",
/*11 0x12 */ "System Restart",
/*12 0x20 */ "OS Critical Stop",
/*13 0x09 */ "Power Redundancy Lost",
/*14 0x00 */ "reserved",
/*15 0x00 */ "reserved",
/*16 0x00 */ "reserved",
/*17 */ "reserved",
/*18 */ "reserved",
/*19 */ "reserved",
/*20 */ "reserved",
/*21 */ "reserved",
/*22 */ "reserved",
/*23 */ "reserved",
/*24 */ "reserved",
/*25 */ "reserved",
/*26 */ "reserved",
/*27 */ "reserved",
/*28 */ "reserved",
/*29 */ "unused",
/*30 */ "unused"
};

char *pefdesc2[MAXPEF] = {	/* for NSC miniBMC */
/* 0 */ "",
/* 1 0x02*/ "Voltage Sensor Assert",
					/* 2 0x23*/ "Watchdog FRB Timeout",
					/* was "Proc FRB Thermal", */
/* 3 0x02*/ "Voltage Sensor Deassert",
/* 4 0x07*/ "Proc1 IERR",
/* 5 0xff*/ "Digital Sensor OK",
/* 6 0x14*/ "Chassis Identify",
/* 7 0x13*/ "NMI Button",
/* 8 0x14*/ "Clear CMOS via Panel",
/* 9 0x0f*/ "OS Load POST Code",
/*10 0x20*/ "OS Critical Stop",
/*11 0x09 */ "Power Redundancy Lost",
/*12 0x00*/ "reserved",
/*13 */ "reserved",
/*14 */ "reserved",
/*15 */ "reserved",
/*16 */ "reserved",
/*17 */ "reserved",
/*18 */ "reserved",
/*19 */ "reserved",
/*20 */ "reserved",
/*21 */ "reserved",
/*22 */ "reserved",
/*23 */ "reserved",
/*24 */ "reserved",
/*25 */ "reserved",
/*26 0x05*/ "Chassis Intrusion",
/*27 0x0f*/ "POST Code Error",
/*28 0x02*/ "Voltage Failure",
/*29 0x04*/ "Fan Failure",
/*30 0x01*/ "Temperature Failure"
};

#define NLAN  39
char canon_param[NLAN] = { 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };
struct {
  int cmd;
  int sz;
  char desc[28];
} lanparams[NLAN] = {		/* see IPMI Table 19-4 */
    /*  0 */  { 0, 1, "Set in progress"},
    /*  1 */  { 1, 1, "Auth type support"},
    /*  2 */  { 2, 5, "Auth type enables"},
    /*  3 */  { 3, 4, "IP address"},
    /*  4 */  { 4, 1, "IP addr src"},		/* (DHCP/Static) */
    /*  5 */  { 5, 6, "MAC addr"},
    /*  6 */  { 6, 4, "Subnet mask"},
    /*  7 */  { 7, 3, "IPv4 header"},
    /*  8 */  { 8, 2, "Prim RMCP port"},
    /*  9 */  { 9, 2, "Sec RMCP port"},
    /* 10 */  { 10, 1, "BMC grat ARP"},
    /* 11 */  { 11, 1, "grat ARP interval"},
    /* 12 */  { 12, 4, "Def gateway IP"},
    /* 13 */  { 13, 6, "Def gateway MAC"},
    /* 14 */  { 14, 4, "Sec gateway IP"},
    /* 15 */  { 15, 6, "Sec gateway MAC"},
    /* 16 */  { 16, 18, "Community string"},
    /* 17 */  { 17, 1, "Num dest"},
    /* 18 */  { 18, 5, "Dest type"},
    /* 19 */  { 19, 13, "Dest address"},
    /* 20 */  { 20, 2, "VLAN ID"},
    /* 21 */  { 21, 1, "VLAN Priority"},
    /* 22 */  { 22, 1, "Cipher Suite Support"},
    /* 23 */  { 23, 17, "Cipher Suites    "},
    /* 24 */  { 24, 9, "Cipher Suite Priv"},
    /* 25 */  { 25, 4, "VLAN Dest Tag"},
    /* 26 */  { 96, 28, "OEM Alert String"},
    /* 27 */  { 97, 1, "Alert Retry Algorithm"},
    /* 28 */  { 98, 3, "UTC Offset"},
    /* 29 */  { 102, 1, "IPv6 Enable"},
    /* 30 */  { 103, 1, "IPv6 Addr Source"},
    /* 31 */  { 104, 16, "IPv6 Address"},
    /* 32 */  { 105, 1, "IPv6 Prefix Len"},
    /* 33 */  { 106, 16, "IPv6 Default Gateway"},
    /* 34 */  { 108, 17, "IPv6 Dest address"},
    /* 35 */  { 192, 4, "DHCP Server IP"},
    /* 36 */  { 193, 6, "DHCP MAC Address"},
    /* 37 */  { 194, 1, "DHCP Enable"},
    /* 38 */  { 201, 2, "Channel Access Mode(Lan)"}
};

#define NSER  22		/* max=32 */
struct {
  int cmd;
  int sz;
  char desc[28];
} serparams[NSER] = {		/* see IPMI Table 20-4 */
    /*  0 */  { 0, 1, "Set in progress"},
    /*  1 */  { 1, 1, "Auth type support"},
    /*  2 */  { 2, 5, "Auth type enables"},
    /*  3 */  { 3, 1, "Connection Mode"},
    /*  4 */  { 4, 1, "Sess Inactiv Timeout"},
    /*  5 */  { 5, 5, "Channel Callback"},
    /*  6 */  { 6, 1, "Session Termination"},
    /*  7 */  { 7, 2, "IPMI Msg Comm"},
    /*  8 */  { 8, 2, "Mux Switch"},
    /*  9 */  { 9, 2, "Modem Ring Time"},
    /* 10 */  { 10, 17, "Modem Init String"},
    /* 11 */  { 11, 5, "Modem Escape Seq"},
    /* 12 */  { 12, 8, "Modem Hangup Seq"},
    /* 13 */  { 13, 8, "Modem Dial Command"},
    /* 14 */  { 14, 1, "Page Blackout Interval"},
    /* 15 */  { 15, 18, "Community String"},
    /* 16 */  { 16, 1, "Num of Alert Dest"},
    /* 17 */  { 17, 5, "Destination Info"},
    /* 18 */  { 18, 1, "Call Retry Interval"},
    /* 19 */  { 19, 3, "Destination Comm Settings"},
    /* 20 */  { 29, 2, "Terminal Mode Config"},
    /* 21 */  { 201, 2, "Channel Access Mode (Ser)"}
};

static void
getauthstr (uchar auth, char *s)
{
  if (s == NULL)
    return;
  s[0] = 0;
  if (auth & 0x01)
    strcat (s, "None ");
  if (auth & 0x02)
    strcat (s, "MD2 ");
  if (auth & 0x04)
    strcat (s, "MD5 ");
  if (auth & 0x10)
    strcat (s, "Pswd ");
  if (auth & 0x20)
    strcat (s, "OEM ");
  return;
}

static int
GetDeviceID (LAN_RECORD * pLanRecord)
{		/*See also ipmi_getdeviceid( pLanRecord, sizeof(LAN_RECORD),fdebug); */
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  int status;
  uchar inputData[24];
  uchar completionCode;

  if (pLanRecord == NULL)
    return (-1);

  status = ipmi_cmd (GET_DEVICE_ID, inputData, 0, responseData,
		     &responseLength, &completionCode, fdebug);

  if (status == ACCESS_OK) {
    if (completionCode) {
      SELprintf ("GetDeviceID: completion code=%x\n", completionCode);
      status = completionCode;
    }
    else {
      memcpy (pLanRecord, &responseData[0], responseLength);
      set_mfgid (&responseData[0], responseLength);
      return (0);		// successful, done
    }
  }				/* endif */
  /* if get here, error */
  return (status);
}				/*end GetDeviceID() */

static int
GetChanAcc (uchar chan, uchar parm, LAN_RECORD * pLanRecord)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  int status;
  uchar inputData[24];
  uchar completionCode;

  if (pLanRecord == NULL)
    return (-1);
  responseLength = 3;
  inputData[0] = chan;
  inputData[1] = parm;		/* 0x80 = active, 0x40 = non-volatile */
  responseLength = sizeof (responseData);
  status = ipmi_cmd (GET_CHANNEL_ACC, inputData, 2, responseData,
		     &responseLength, &completionCode, fdebug);

  if (status == ACCESS_OK) {
    if (completionCode) {
      SELprintf ("GetChanAcc: completion code=%x\n", completionCode);
      status = completionCode;
    }
    else {
      // dont copy first byte (Parameter revision, usu 0x11)
      memcpy (pLanRecord, &responseData[0], responseLength);
      return (0);		// successful, done
    }
  }				/* endif */
  /* if get here, error */
  return (status);
}				/*GetChanAcc() */

static int
SetChanAcc (uchar chan, uchar parm, uchar val)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  int status;
  uchar inputData[24];
  uchar completionCode;

  if (fmBMC)
    return (0);			/* mBMC doesn't support this */
  /* parm: 0x80 = active, 0x40 = set non-vol */
  responseLength = 1;
  inputData[0] = chan;		/* channel */
  inputData[1] = (parm & 0xc0) | (val & 0x3F);
  inputData[2] = (parm & 0xc0) | lan_access;	/* set priv level to Admin */

  responseLength = sizeof (responseData);
  status = ipmi_cmd (SET_CHANNEL_ACC, inputData, 3, responseData,
		     &responseLength, &completionCode, fdebug);

  if (status == ACCESS_OK) {
    if (completionCode) {
      SELprintf ("SetChanAcc: completion code=%x\n", completionCode);
      status = completionCode;
    }
    else {
      return (0);		// successful, done
    }
  }				/* endif */
  /* if get here, error */
  return (status);
}				/*SetChanAcc() */

int
SetPasswd (int unum, char *uname, char *upswd, uchar chan, uchar priv)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  int status, i, psw_len;
  uchar completionCode;
  uchar inputData[24];
  int ret = 0;

  inputData[0] = (uchar) unum;	/*user 1 = null user */
  responseLength = sizeof (responseData);
  status = ipmi_cmd (GET_USER_NAME, inputData, 1, responseData,
		     &responseLength, &completionCode, fdebug);
  SELprintf ("GET_USERNAME: %x %x %x, status = %x, ccode=%x\n",
	     responseData[0], responseData[1], responseData[2],
	     status, completionCode);
  if (fdebug) {
    char aname[17];
    printf ("User %d: ", unum);
    for (i = 0; i < responseLength; i++) {
      printf ("%02x ", responseData[i]);
      if (responseData[i] < 0x20)
	aname[i] = '.';
      else
	aname[i] = responseData[i];
    }
    aname[16] = 0;
    printf (" %s\n", aname);
  }

  if (unum != 1) {		/* user specified a lan username */
    if (fiBMC && (unum == 2)) {
      /* cannot set user 2 name */
      if (uname != NULL) {
	if (strcmp (uname, "root") != 0)
	  printf ("SETUSERNAME - user2 name %s must be root\n", uname);
      }
    }
    else if (unum == 2 && (vend_id == VENDOR_SUPERMICROX ||
			   vend_id == VENDOR_SUPERMICRO)) {
      /* cannot set user 2 name */
      if (uname != NULL) {
	if (strcmp (uname, "ADMIN") != 0)
	  printf ("SETUSERNAME - user2 name %s must be ADMIN\n", uname);
      }
    }
    else if (uname != NULL) {
      inputData[0] = (uchar) unum;
      memset (&inputData[1], 0, 16);
      memcpy (&inputData[1], uname, strlen (uname));
      status = ipmi_cmd (SET_USER_NAME, inputData, 17, responseData,
			 &responseLength, &completionCode, fdebug);
      if (completionCode == 0xCC)
	status = 0;		/*setting username to previous gives 0xCC, ok */
      else {
	SELprintf ("SETUSERNAME - %x %x %x  status = %x, ccode=%x\n",
		   inputData[0], inputData[1], inputData[2],
		   status, completionCode);
	if (status == 0)
	  status = completionCode;
	if (status != 0)
	  ret = status;
      }
    }
  }

  if ((unum != 1) && (uname == NULL)) {
    ;				/* if no username, do not enable user */
  }
  else {
    inputData[0] = (uchar) unum;
    inputData[1] = 0x01;	/*enable user */
    responseLength = sizeof (responseData);
    status = ipmi_cmd (SET_USER_PASSWORD, inputData, 2, responseData,
		       &responseLength, &completionCode, fdebug);
    printf ("SETUSERENAB - inputData %x %x %x, status = %x, ccode=%x\n",
	    inputData[0], inputData[1], inputData[2], status, completionCode);
    if (status == 0)
      status = completionCode;
    if (status != 0)
      ret = status;
  }

  if (upswd != NULL) {
    inputData[0] = (uchar) unum;
    inputData[1] = 0x02;	/*set password */
    psw_len = PSW_LEN;		      /*=16 change if 20-byte passwords supported */
    memset (&inputData[2], 0, psw_len);
    strcpy ((char *) &inputData[2], upswd);
    if (fdebug) {
      char apsw[PSW_MAX + 1];
      char c;
      printf ("Pswd %d: ", unum);
      for (i = 0; i < psw_len; i++) {
	c = inputData[i + 2];
	printf ("%02x ", (unsigned char) c);
	if (c < 0x20)
	  apsw[i] = '.';
	else
	  apsw[i] = c;
      }
      apsw[psw_len] = 0;
      printf (" %s\n", apsw);
    }
    responseLength = sizeof (responseData);
    status = ipmi_cmd (SET_USER_PASSWORD, inputData, 2 + psw_len,
		       responseData, &responseLength, &completionCode,
		       fdebug);
    SELprintf ("SETUSERPSW - inputData %x %x %x, status = %x, ccode=%x\n",
	       inputData[0], inputData[1], inputData[2], status,
	       completionCode);
    if (status == 0)
      status = completionCode;
    if (status != 0)
      ret = status;

    inputData[0] = (uchar) unum;	/*user 1 = null user */
    inputData[1] = 0x03;	/*test password */
    memset (&inputData[2], 0, psw_len);
    if (upswd != NULL)
      strcpy ((char *) &inputData[2], upswd);
    responseLength = sizeof (responseData);
    status = ipmi_cmd (SET_USER_PASSWORD, inputData, 2 + psw_len,
		       responseData, &responseLength, &completionCode,
		       fdebug);
    SELprintf ("TESTUSERPSW - inputData %x %x %x, status = %x, ccode=%x\n",
	       inputData[0], inputData[1], inputData[2], status,
	       completionCode);
  }

  if (fiBMC && (unum == 2)) {	/*iBMC doesn't support this on user 2 */
    if (fdebug)
      printf ("skipping SETUSER_ACCESS on iBMC for user %d\n", unum);
  }
  else {
    inputData[0] = 0x90 | chan;	/* = 0x97 for chan=lan_ch=7 */
    inputData[1] = (uchar) unum;	/* user num */
    inputData[2] = priv;	/* usu priv=lan_access is admin */
    inputData[3] = 0x00;	/* User Session Limit, 0=not limited */
    responseLength = sizeof (responseData);
    status = ipmi_cmd (SET_USER_ACCESS, inputData, 4, responseData,
		       &responseLength, &completionCode, fdebug);
    printf ("SETUSER_ACCESS - inputData %x %x %x, status = %x ccode=%x\n",
	    (uchar) inputData[0], inputData[1], inputData[2],
	    status, completionCode);
    if (status == 0)
      status = completionCode;
    if (status != 0)
      ret = status;
  }

  return (ret);
}				/*end SetPswd() */

int
SetUser (int unum, char *uname, char *passwd, uchar chan)
{
  int ret = 0;
  /* if the user specified a username or password, set it. */
  if ((fpassword) || (uname != NULL)) {
    /* set username and password */
    ret = SetPasswd (unum, uname, passwd, chan, lan_access);
  }
  return (ret);
}				/*end SetUser() */

int
DisableUser (int unum, uchar chan)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  int status;
  uchar completionCode;
  uchar inputData[24];

  inputData[0] = 0x80 | chan;	/* = 0x87, no IPMI */
  inputData[1] = (uchar) unum;	/* user 1 */
  inputData[2] = 0x0F;		/* No access  */
  inputData[3] = 0x00;		/* User Session Limit, 0=not limited */
  responseLength = sizeof (responseData);
  status = ipmi_cmd (SET_USER_ACCESS, inputData, 4, responseData,
		     &responseLength, &completionCode, fdebug);
  if (status == 0)
    status = completionCode;
  return (status);
}

char * parse_priv (uchar c)
{
  char *p;
  c = (c & 0x0f);
  switch (c) {
  case 1: p = "Callback"; break;
  case 2: p = "User  "; break;
  case 3: p = "Operator"; break;
  case 4: p = "Admin "; break;
  case 5: p = "OEM   "; break;
  case 0x0f: p = "No access"; break;
  default: p = "Reserved";		/*usually =0 */
  }
  return (p);
}

static void
show_priv (uchar c)
{
  char *privstr;
  privstr = parse_priv (c);
  printf ("%s", privstr);
}

static int
valid_priv (int c)
{
  int rv;
  switch (c) {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 0x0f:
    rv = 1;
    break;
  default:
    rv = 0;
    break;
  }
  return rv;
}

/* GetUserINfo - get user configuration info for user subfunction */
int
GetUserInfo (uchar unum, uchar chan, uchar * enab, uchar * priv, char *uname,
	char fdbg)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  int status, rv;
  uchar completionCode;
  uchar inputData[24];
  uchar upriv;

  if (fdbg) fdebug = 1;
  if (enab == NULL || priv == NULL)
    return (-2);
  inputData[0] = chan;		/*lan_ch */
  inputData[1] = unum;		/* user number for IPMI LAN */
  responseLength = sizeof (responseData);
  status = ipmi_cmd (GET_USER_ACCESS, inputData, 2, responseData,
		     &responseLength, &completionCode, fdebug);
  rv = status;
  if (status == 0 && completionCode != 0)
    rv = completionCode;
  if (rv == 0) {
    if (unum == 1) {		/*get max_users and enabled_users */
      max_users = responseData[0] & 0x3f;
      enabled_users = responseData[1] & 0x3f;
    }
    upriv = responseData[3];
    if ((responseData[1] & 0x80) != 0) *enab = 0;
    else *enab = 1;
    inputData[0] = unum;	/* usually = 1 for BMC LAN */
    responseLength = sizeof (responseData);
    status = ipmi_cmd (GET_USER_NAME, inputData, 1, responseData,
		       &responseLength, &completionCode, fdebug);
    if (status != 0 || completionCode != 0)
      responseData[0] = 0;	/*empty user name */
    responseData[PSW_MAX - 1] = 0;	/*for safety */
    *priv = upriv;
    if (uname != NULL)
      strcpy (uname, responseData);
  }
  return (rv);
}

/* GetUser - get and show user configuration */
int
GetUser (uchar user_num, uchar chan)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  int status;
  uchar completionCode;
  uchar inputData[24];

  inputData[0] = chan;		/*lan_ch */
  inputData[1] = user_num;	/* usually = 1 for BMC LAN */
  responseLength = sizeof (responseData);
  status = ipmi_cmd (GET_USER_ACCESS, inputData, 2, responseData,
		     &responseLength, &completionCode, fdebug);
  if (status == 0 && completionCode == 0) {
    uchar c;
    if (user_num == 1) {	/*get max_users and enabled_users */
      max_users = responseData[0] & 0x3f;
      enabled_users = responseData[1] & 0x3f;
      if (enabled_users > show_users)
	show_users = enabled_users;
      if (show_users > max_users)
	show_users = max_users;
      if (!fcanonical)
	SELprintf ("Users:  showing %d of max %d users (%d enabled)\n",
		   show_users, max_users, enabled_users);
    }
    if (fcanonical)
      SELprintf ("Channel %d User %d Access %s%c ", chan, user_num,
		 pspace2, bdelim);
    else
      SELprintf ("User Access(chan%d,user%d): %02x %02x %02x %02x : ",
		 chan, user_num, (uchar) responseData[0],
		 responseData[1], responseData[2], responseData[3]);
    c = responseData[3];
    inputData[0] = user_num;	/* usually = 1 for BMC LAN */
    responseLength = sizeof (responseData);
    status = ipmi_cmd (GET_USER_NAME, inputData, 1, responseData,
		       &responseLength, &completionCode, fdebug);
    if (status != 0 || completionCode != 0)
      responseData[0] = 0;
    if (c & 0x10)
      printf ("IPMI, ");
    show_priv (c);
    printf (" (%s)\n", responseData);	/*show user name */
  }
  else
    SELprintf ("Get User Access(%d,%d), status=%x, ccode=%x\n",
	       chan, user_num, status, completionCode);
  return (status);
}				/*end ShowUser() */

static int
GetSerEntry (uchar subfunc, LAN_RECORD * pLanRecord)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  uchar inputData[24];
  int status;
  uchar completionCode;
  uchar chan;
  uchar bset;

  if (pLanRecord == NULL) {
    if (fdebug)
      printf ("GetSerEntry(%d): error, output buffer is NULL\n", subfunc);
    return (-1);
  }

  chan = ser_ch;		/* 1=EMP, 0=IPMB, 6=LAN2, 7=LAN1 */
  bset = 0;

  inputData[0] = chan;		// flags, channel 3:0 (1=EMP)
  inputData[1] = subfunc;	// Param selector 
  inputData[2] = bset;		// Set selector 
  inputData[3] = 0;		// Block selector
  if (subfunc == 10) {
    inputData[2] = 0;
    inputData[3] = 1;
  }

  status = ipmi_cmd (GET_SER_CONFIG, inputData, 4, responseData,
		     &responseLength, &completionCode, fdebug);

  if (status == ACCESS_OK) {
    if (completionCode) {
      if (fdebug)
	SELprintf ("GetSerEntry(%d,%d): completion code=%x\n",
		   chan, subfunc, completionCode);
    }
    else {
      // dont copy first byte (Parameter revision, usu 0x11)
      memcpy (pLanRecord, &responseData[1], responseLength - 1);
      pLanRecord->data[responseLength - 1] = 0;
      //successful, done
      return (0);
    }
  }

  // we are here because completionCode is not COMPLETION_CODE_OK
  if (fdebug)
    SELprintf ("GetSerEntry(%d,%d): ipmi_cmd status=%x ccode=%x\n",
	       chan, subfunc, status, completionCode);
  return -1;
}

static int
GetLanEntry (uchar subfunc, uchar bset, LAN_RECORD * pLanRecord)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  uchar inputData[24];
  int status, n;
  uchar completionCode;
  uchar chan;

  if (pLanRecord == NULL) {
    if (fdebug)
      printf ("GetLanEntry: error, output buffer is NULL\n");
    return (-1);
  }

  chan = lan_ch;		/* LAN 1 = 7 */

  inputData[0] = chan;		// flags, channel 3:0 (LAN 1)
  inputData[1] = subfunc;	// Param selector (3 = ip addr)
  inputData[2] = bset;		// Set selector 
  inputData[3] = 0;		// Block selector

  status = ipmi_cmd (GET_LAN_CONFIG, inputData, 4, responseData,
		     &responseLength, &completionCode, fdebug);

  if (status == ACCESS_OK) {
    if (completionCode) {
      if (fdebug)
	SELprintf ("GetLanEntry: completion code=%x\n", completionCode);
      status = completionCode;
    }
    else {
      // dont copy first byte (Parameter revision, usu 0x11)
      if (responseLength > 0) {
	n = responseLength - 1;
	memcpy (pLanRecord, &responseData[1], n);
      }
      else
	n = 0;
      pLanRecord->data[n] = 0;
      //successful, done
      return (0);
    }
  }

  // we are here because completionCode is not COMPLETION_CODE_OK
  if (fdebug)
    SELprintf ("GetLanEntry: ipmi_cmd status=%d completionCode=%x\n",
	       status, completionCode);
  return status;
}				/* end GetLanEntry() */

static int
SetLanEntry (uchar subfunc, LAN_RECORD * pLanRecord, int reqlen)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  uchar inputData[24];
  int status;
  uchar completionCode;

  if (pLanRecord == NULL) {
    if (fdebug)
      printf ("SetLanEntry(%d): error, input buffer is NULL\n", subfunc);
    return (-1);
  }
  if (vend_id == VENDOR_SUPERMICROX || vend_id == VENDOR_SUPERMICRO) {
    /* SUPERMICRO cannot set grat arp or grat arp interval */
    if (subfunc == 10 || subfunc == 11)
      return (0);
  }

  inputData[0] = lan_ch;	// flags, channel 3:0 (LAN 1)
  inputData[1] = subfunc;	// Param selector (3 = ip addr)
  memcpy (&inputData[2], pLanRecord, reqlen);

  status = ipmi_cmd (SET_LAN_CONFIG, inputData, (uchar) (reqlen + 2),
		     responseData, &responseLength, &completionCode, fdebug);

  if (status == ACCESS_OK) {
    if (completionCode) {
      if (fdebug)
		SELprintf ("SetLanEntry(%d): completion code=%x\n", subfunc, completionCode);	// responseData[0]);
      return (completionCode);
    }
    else {
      //successful, done
      return (0);
    }
  }

  // we are here because completionCode is not COMPLETION_CODE_OK
  if (fdebug)
    SELprintf ("SetLanEntry(%d): ipmi_cmd status=%d ccode=%x\n",
	       subfunc, status, completionCode);
  return status;
}				/* end SetLanEntry() */

int
GetPefEntry (uchar subfunc, ushort rec_id, PEF_RECORD * pPefRecord)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  uchar inputData[24];		/* only use 3 bytes for input */
  int status, n;
  uchar completionCode;

  if (pPefRecord == NULL) {
    if (fdebug)
      printf ("GetPefEntry(%d): error, output buffer is NULL\n", subfunc);
    return (-1);
  }

  inputData[0] = subfunc;	// Parameter = Evt Filter Table
  inputData[1] = (uchar) rec_id;
  inputData[2] = 0;

  status = ipmi_cmd (GET_PEF_CONFIG, inputData, 3, responseData,
		     &responseLength, &completionCode, fdebug);

  if (status == ACCESS_OK) {
    if (completionCode) {
      if (fdebug)
	SELprintf ("GetPefEntry(%d/%d): completion code=%x\n",
		   subfunc, rec_id, completionCode);
      status = completionCode;
    }
    else {
      /* expect PEF record to be >=21 bytes */
      if (responseLength > 1)
	n = responseLength - 1;
      else
	n = 0;
      if (n > 21)
	n = 21;			/*only use 21 bytes */
      if ((subfunc == 6) && (n < 21)) {
	if (fdebug)
	  printf ("GetPefEntry(%d/%d): length %d too short\n",
		  subfunc, rec_id, responseLength);
      }
      // dont copy first byte (Parameter revision, usu 0x11)
      if (n == 0)
	memset (pPefRecord, 0, 21);
      else
	memcpy (pPefRecord, &responseData[1], n);
      //successful, done
      return (0);
    }
  }

  // we are here because completionCode is not COMPLETION_CODE_OK
  if (fdebug)
    SELprintf ("GetPefEntry: ipmi_cmd status=%x completionCode=%x\n",
	       status, completionCode);
  return status;
}				/* end GetPefEntry() */

int
SetPefEntry (PEF_RECORD * pPefRecord)
{
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  uchar inputData[32];		/* sizeof(PEF_RECORD) = 21 +1=22 */
  int status;
  uchar completionCode;
  uchar subfunc;

  subfunc = 0x06;		// Parameter = Evt Filter Table

  if (pPefRecord == NULL) {
    if (fdebug)
      printf ("SetPefEntry: error, output buffer is NULL\n");
    return (-1);
  }

  //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21
  // 06 0c 80 01 01 00 ff ff 20 ff 6f ff 00 00 00 00 00 00 00 00 00 00
  // memset(&inputData[0],0,requestData.dataLength);
  inputData[0] = subfunc;
  memcpy (&inputData[1], pPefRecord, sizeof (PEF_RECORD));

  status = ipmi_cmd (SET_PEF_CONFIG, inputData, sizeof (PEF_RECORD) + 1,
		     responseData, &responseLength, &completionCode, fdebug);

  if (status == ACCESS_OK) {
    if (completionCode) {
      if (fdebug)
	SELprintf ("SetPefEntry: completion code=%x\n", completionCode);	// responseData[0]);
      status = completionCode;
    }
    else {
      //successful, done
      return (0);
    }

  }

  // we are here because completionCode is not COMPLETION_CODE_OK
  if (fdebug)
    SELprintf ("SetPefEntry: ipmi_cmd status=%d completion code=%x\n",
	       status, completionCode);
  return (status);

}				/* end SetPefEntry() */

int
DisablePef (int anum)
{
  uchar iData[24];		/* sizeof(PEF_RECORD) = 21 +1=22 */
  uchar rData[MAX_BUFFER_SIZE];
  int rLength = MAX_BUFFER_SIZE;
  uchar cc;
  int status;

  if (fmBMC) {
    SELprintf ("mini-BMC does not support disabling BMC LAN\n");
    return (-1);
  }
  else {
    status = SetChanAcc (lan_ch, 0x80, CHAN_ACC_DISABLE);
    if (fdebug)
      SELprintf ("SetChanAcc(lan/active), ret = %d\n", status);
    status = SetChanAcc (lan_ch, 0x40, CHAN_ACC_DISABLE);
    SELprintf ("SetChanAcc(lan), ret = %d\n", status);
    if (status != 0)
      return (status);
  }

  iData[0] = 0x01;		/* PEF Control Param */
  iData[1] = 0x00;		/* PEF disable */
  rLength = MAX_BUFFER_SIZE;
  status = ipmi_cmd (SET_PEF_CONFIG, iData, 2, rData, &rLength, &cc, fdebug);
  if (status != 0)
    return (status);
  if (cc) {
    SELprintf ("DisablePef[%d]: completion code=%x\n", iData[0], cc);
    return (-1);
  }

  if (anum != 0) {
    iData[0] = 0x09;		/* PEF Alert Policy Table */
    iData[1] = (uchar) anum;	/* Policy number (default 0x01) */
    iData[2] = 0x10;		/* PEF LAN, policy disable */
    iData[3] = 0x00;		/* LAN_CH=00, default dest=00 */
    iData[4] = 0x00;		/* No alert string */
    rLength = MAX_BUFFER_SIZE;
    status = ipmi_cmd (SET_PEF_CONFIG, iData, 5, rData, &rLength,
		       &cc, fdebug);
    if (status != 0)
      return (status);
    if (cc) {
      SELprintf ("DisablePef[%d]: completion code=%x\n", iData[0], cc);
      return (-1);
    }
  }
  return (status);
}

int
ShowPef (void)
{
  uchar iData[24];		/* sizeof(PEF_RECORD) = 21 +1=22 */
  uchar rData[MAX_BUFFER_SIZE];
  int rLength = MAX_BUFFER_SIZE;
  uchar cc;
  int status, i, j;

  for (j = 1; j < MAX_PEFPARAMS; j++) {
    if (j == 4 && fmBMC) {
      /* fmBMC gets cc=0x80 for param 4, so skip it. */
      continue;
    }
    iData[0] = (uchar) j;	/* PEF Control Param */
    if (j == 6 || j == 7 || j == 9)
      iData[1] = 1;
    else
      iData[1] = 0x00;		/* PEF Set Selector */
    if (j == 13)
      iData[2] = 1;
    else
      iData[2] = 0x00;		/* PEF Block Selector */
    rLength = MAX_BUFFER_SIZE;
    status = ipmi_cmd (GET_PEF_CONFIG, iData, 3, rData, &rLength,
		       &cc, fdebug);
    if (status == 0 && cc == 0) {
      SELprintf ("PefParam[%d]: ", iData[0]);
      if (rLength > 0)
	for (i = 0; i < peflen[j]; i++)
	  SELprintf ("%02x ", rData[1 + i]);
      SELprintf ("\n");
    }
    else
      SELprintf ("PefParam[%d]: GET_PEF status=%d cc=%x\n",
		 iData[0], status, cc);
  }
  return (status);
}

int
EnablePef (int anum)
{
  uchar iData[24];		/* sizeof(PEF_RECORD) = 21 +1=22 */
  uchar rData[MAX_BUFFER_SIZE];
  int rLength = MAX_BUFFER_SIZE;
  uchar cc;
  int status;
  uchar sdelay;

  status = SetChanAcc (lan_ch, 0x80, chan_pefon);
  if (fdebug)
    SELprintf ("SetChanAcc(lan/active), ret = %d\n", status);
  status = SetChanAcc (lan_ch, 0x40, chan_pefon);
  SELprintf ("SetChanAcc(lan), ret = %d\n", status);
  if (status != 0)
    return (status);

  {
    iData[0] = 0x01;		/* PEF Control Param */
    iData[1] = 0x00;		/* PEF Set Selector */
    iData[2] = 0x00;		/* PEF Block Selector */
    rLength = MAX_BUFFER_SIZE;
    status = ipmi_cmd (GET_PEF_CONFIG, iData, 3, rData, &rLength,
		       &cc, fdebug);
    if (status != 0 || cc != 0)
      sdelay = 0;
    else
      sdelay = rData[1];
    if (fdebug)
      SELprintf ("EnablePef[%d]: get cc=%x, control=%02x\n",
		 iData[0], cc, sdelay);
    iData[0] = 0x01;		/* PEF Control Param (0x01 or 0x05) */
    iData[1] = 0x01;		/* PEF enable, & no startup delay */
    rLength = MAX_BUFFER_SIZE;
    status = ipmi_cmd (SET_PEF_CONFIG, iData, 2, rData, &rLength,
		       &cc, fdebug);
    if (status != 0)
      return (status);
    if (cc) {
      SELprintf ("EnablePef[%d]: completion code=%x\n", iData[0], cc);
      return (-1);
    }

#ifdef TEST
    iData[0] = 0x01;		/* Serial Channel */
    iData[1] = 0x13;		/* Dest Com settings = 19. */
    iData[2] = 0x01;		/* POL Default Dest */
    iData[3] = 0x60;
    iData[4] = 0x07;
    status = ipmi_cmd (SET_SER_CONFIG, iData, 5, rData, &rLength,
		       &cc, fdebug);
#endif

    iData[0] = 0x02;		/* PEF Action Param */
    iData[1] = 0x00;		/* PEF Set Selector */
    iData[2] = 0x00;		/* PEF Block Selector */
    rLength = MAX_BUFFER_SIZE;
    status = ipmi_cmd (GET_PEF_CONFIG, iData, 3, rData, &rLength,
		       &cc, fdebug);
    if (fdebug)
      SELprintf ("EnablePef[%d]: get cc=%x, val=%02x\n",
		 iData[0], cc, rData[1]);
    iData[0] = 0x02;		/* PEF Action Param */
    if (vend_id == VENDOR_INTEL)
      iData[1] = 0x2f;		/* enable alerts, reset, power cycle/down, diag */
    else
      iData[1] = 0x0f;		/* enable alerts, reset, power cycle/down */
    rLength = MAX_BUFFER_SIZE;
    status = ipmi_cmd (SET_PEF_CONFIG, iData, 2, rData, &rLength,
		       &cc, fdebug);
    if (status != 0)
      return (status);
    if (cc) {
      SELprintf ("EnablePef[%d]: completion code=%x\n", iData[0], cc);
      return (-1);
    }

    if ((sdelay & 0x04) != 0) {	/* startup delay is supported */
      iData[0] = 0x03;		/* PEF Startup Delay Param */
      iData[1] = 0x00;		/* 0 seconds, default is 0x3c (60 sec) */
      rLength = MAX_BUFFER_SIZE;
      status = ipmi_cmd (SET_PEF_CONFIG, iData, 2, rData, &rLength,
			 &cc, fdebug);
      if (fdebug)
	SELprintf ("EnablePef[%d]: set val=%02x cc=%x\n",
		   iData[0], iData[1], cc);
      if (status != 0)
	return (status);
      if (cc) {
	SELprintf ("EnablePef[%d]: completion code=%x\n", iData[0], cc);
	// return(-1);
      }
      iData[0] = 0x04;		/* PEF Alert Startup Delay Param */
      iData[1] = 0x00;		/* 0 seconds, default is 0x3c (60 sec) */
      rLength = MAX_BUFFER_SIZE;
      status = ipmi_cmd (SET_PEF_CONFIG, iData, 2, rData, &rLength,
			 &cc, fdebug);
      if (fdebug)
	SELprintf ("EnablePef[%d]: set val=%02x cc=%x\n",
		   iData[0], iData[1], cc);
      if (status != 0)
	return (status);
      if (cc) {
	SELprintf ("EnablePef[%d]: completion code=%x\n", iData[0], cc);
	// return(-1);
      }
    }				/*endif sdelay */

    iData[0] = 0x09;		/* PEF Alert Policy Table */
    iData[1] = (uchar) anum;	/* Policy number (default 0x01) */
    iData[2] = 0x18;		/* PEF LAN, always alert, policy enable */
    iData[3] = (lan_ch << 4) + anum;	/* LAN_CH=70, default dest=01 */
    iData[4] = 0x00;		/* No alert string */
    rLength = MAX_BUFFER_SIZE;
    status = ipmi_cmd (SET_PEF_CONFIG, iData, 5, rData, &rLength,
		       &cc, fdebug);
    if (fdebug)
      SELprintf ("EnablePef[%d]: set val=%02x cc=%x\n",
		 iData[0], iData[1], cc);
    if (status != 0)
      return (status);
    if (cc) {
      SELprintf ("EnablePef[%d]: completion code=%x\n", iData[0], cc);
      return (-1);
    }
  }				/*endif IPMI 1.5 */

  return (status);
}				/* end EnablePef */

#define NBAUDS  10
static struct
{
  unsigned char val;
  char str[8];
} mapbaud[NBAUDS] = {
  {
  6, "9600"}, {
  6, "9.6K"}, {
  7, "19.2K"}, {
  7, "19200"}, {
  8, "38.4K"}, {
  8, "38400"}, {
  9, "57.6K"}, {
  9, "57600"}, {
  10, "115.2K"}, {
  10, "115200"}
};

static unsigned char
Str2Baud (char *str)
{
  unsigned char baud = 0;
  int i, n, len;
  len = strlen_ (str);
  for (i = 0; i < len; i++)	/*toupper */
    if (str[i] >= 'a' && str[i] <= 'z')
      str[i] &= 0x5F;
  for (i = 0; i < NBAUDS; i++) {
    n = strlen_ (mapbaud[i].str);
    if (strncmp (str, mapbaud[i].str, n) == 0) {
      baud = mapbaud[i].val;
      break;
    }
  }
  if (i == NBAUDS || baud == 0) {
    printf ("Invalid -B parameter value (%s), using 19.2K.\n", str);
    i = 1;			/* default is 19.2K */
    baud = mapbaud[i].val;	/* =7 */
  }
  if (fdebug)
    printf ("new baud = %02x (%s)\n", baud, mapbaud[i].str);
  return (baud);
}

static char *
Baud2Str (unsigned char bin)
{
  char *baudstr;
  unsigned char b;
  b = bin & 0x0f;
  switch (b) {
  case 6:
    baudstr = "9600 ";
    break;
  case 7:
    baudstr = "19.2k";
    break;
  case 8:
    baudstr = "38.4k";
    break;
  case 9:
    baudstr = "57.6k";
    break;
  case 10:
    baudstr = "115.2k";
    break;
  default:
    baudstr = "nobaud";
  }
  return (baudstr);
}

static int
BaudValid (unsigned char b)
{
  int val = 0;
  switch (b) {
  case 6:
    val = 1;
    break;
  case 7:
    val = 1;
    break;
  case 8:
    val = 1;
    break;
  case 9:
    val = 1;
    break;
  case 10:
    val = 1;
    break;
  default:
    val = 0;
    break;
  }
  return (val);
}

/*
 * atomac - converts ASCII string to binary MAC address (array).
 * Accepts input formatted as 11:22:33:44:55:66 or 11-22-33-44-55-66.
 */
void
atomac (uchar * array, char *instr)
{
  int i, j, n;
  char *pi;
  j = 0;
  pi = instr;
  n = strlen_ (instr);
  for (i = 0; i <= n; i++) {
    if (instr[i] == ':') {
      array[j++] = htoi (pi);
      pi = &instr[i + 1];
    }
    else if (instr[i] == '-') {
      array[j++] = htoi (pi);
      pi = &instr[i + 1];
    }
    else if (instr[i] == 0) {
      array[j++] = htoi (pi);
    }
    if (j >= MAC_LEN)
      break;			/*safety valve */
  }
  if (fdebug)
    printf ("atomac: %02x %02x %02x %02x %02x %02x\n",
	    array[0], array[1], array[2], array[3], array[4], array[5]);
}				/*end atomac() */

/* extern void atoip(uchar *array,char *instr); *from subs.c*/
void
MacSetInvalid (uchar * mac)
{
  int i;
  if (mac == NULL)
    return;
  for (i = 0; i < MAC_LEN; i++) {
    if (i == 0)
      mac[i] = 0xFF;
    else
      mac[i] = 0x00;
  }
}

int
MacIsValid (uchar * mac)
{
  int fvalid = 0;
  int i;
  /* check for initial invalid value of FF:00:... */
  if (mac[0] == 0xff && mac[1] == 0x00)	/* marked as invalid */
    return (fvalid);
  /* check for all zeros */
  for (i = 0; i < MAC_LEN; i++)
    if (mac[i] != 0) {		/* not all zeros */
      fvalid = 1;
      break;
    }
  return (fvalid);
}

int
IpIsValid (uchar * ipadr)
{
  int fvalid = 1;
  if (ipadr[0] == 0)
    fvalid = 0;
  return (fvalid);
}

int
SubnetIsValid (uchar * subnet)
{
  int fvalid = 0;
  /* if masking off at least one bit, say valid */
  if (subnet[0] != 0)
    fvalid = 1;
  return (fvalid);
}

int
SubnetIsSame (uchar * ip1, uchar * ip2, uchar * subnet)
{
  int i;
  uchar c1, c2;
  for (i = 0; i < 4; i++) {
    c1 = ip1[i] & subnet[i];
    c2 = ip2[i] & subnet[i];
    if (c1 != c2)
      return 0;
  }
  return 1;			/*same, return true */
}

#ifdef WIN32
/*
 * Obtain network adapter information (Windows).
 */
PIP_ADAPTER_ADDRESSES
GetAdapters ()
{
  PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL;
  ULONG OutBufferLength = 0;
  ULONG RetVal = 0, i;

  // The size of the buffer can be different 
  // between consecutive API calls.
  // In most cases, i < 2 is sufficient;
  // One call to get the size and one call to get the actual parameters.
  // But if one more interface is added or addresses are added, 
  // the call again fails with BUFFER_OVERFLOW. 
  // So the number is picked slightly greater than 2. 
  // We use i <5 in the example
  for (i = 0; i < 5; i++) {
    RetVal = GetAdaptersAddresses (AF_INET, 0, NULL,
				   AdapterAddresses, &OutBufferLength);

    if (RetVal != ERROR_BUFFER_OVERFLOW) {
      break;
    }

    if (AdapterAddresses != NULL) {
      free (AdapterAddresses);
    }

    AdapterAddresses = (PIP_ADAPTER_ADDRESSES) malloc (OutBufferLength);
    if (AdapterAddresses == NULL) {
      RetVal = GetLastError ();
      break;
    }
  }
  if (RetVal == NO_ERROR) {
    // If successful, return pointer to structure
    return AdapterAddresses;
  }
  else {
    LPVOID MsgBuf;

    printf ("Call to GetAdaptersAddresses failed.\n");
    if (FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, RetVal, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),	// Default language
		       (LPTSTR) & MsgBuf, 0, NULL)) {
      printf ("\tError: %s", (char *)MsgBuf);
    }
    LocalFree (MsgBuf);
  }
  return NULL;
}

/*
 * Set BMC MAC corresponding to current BMC IP address (Windows).
 */
int
GetLocalMACByIP ()
{
  PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL;
  PIP_ADAPTER_ADDRESSES AdapterList;
  int result = 0;

  struct sockaddr_in *si;

  AdapterAddresses = GetAdapters ();
  AdapterList = AdapterAddresses;

  while (AdapterList) {
    PIP_ADAPTER_UNICAST_ADDRESS addr;
    addr = AdapterList->FirstUnicastAddress;
    if (addr == NULL)
      si = NULL;
    else
      si = (struct sockaddr_in *) addr->Address.lpSockaddr;
    if (si != NULL) {
      if (memcmp (&si->sin_addr.s_addr, rgmyip, 4) == 0) {
	if (!MacIsValid (rgmymac))
	  memcpy (rgmymac, AdapterList->PhysicalAddress, MAC_LEN);
	memcpy (osmyip, &si->sin_addr.s_addr, 4);
	memcpy (osmymac, AdapterList->PhysicalAddress, MAC_LEN);
	wcstombs (ifname, AdapterList->FriendlyName, sizeof (ifname));
	result = 1;
	break;
      }
    }
    AdapterList = AdapterList->Next;
  }

  if (AdapterAddresses != NULL) {
    free (AdapterAddresses);
  }
  return result;
}

/*
 * Get First IP Address in Windows OS
 *   ipaddr is 4 bytes, macaddr is 6 bytes, ipname can be 64 bytes.
 * (called by idiscover.c)
 */
int
GetFirstIP (uchar * ipaddr, uchar * macadr, char *ipname, char fdbg)
{
  PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL;
  PIP_ADAPTER_ADDRESSES AdapterList;
  struct sockaddr_in *si;
  uchar *psaddr;
  int result = -1;

  AdapterAddresses = GetAdapters ();
  AdapterList = AdapterAddresses;

  while (AdapterList) {
    PIP_ADAPTER_UNICAST_ADDRESS addr;

    addr = AdapterList->FirstUnicastAddress;
    si = (struct sockaddr_in *) addr->Address.lpSockaddr;
    psaddr = (uchar *) & si->sin_addr.s_addr;
    if ((psaddr[0] != 0) && (psaddr[0] != 169)) {
      if (fdbg)
	printf ("found IP: s_addr=%d.%d.%d.%d\n",
		psaddr[0], psaddr[1], psaddr[2], psaddr[3]);
      if (ipaddr != NULL)
	memcpy (ipaddr, &si->sin_addr.s_addr, 4);
      if (macadr != NULL) {
	memcpy (macadr, AdapterList->PhysicalAddress, MAC_LEN);
	if (fdbg)
	  printf ("found MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
		  macadr[0], macadr[1], macadr[2], macadr[3],
		  macadr[4], macadr[5]);
      }
      if (ipname != NULL) {
	wcstombs (ipname, AdapterList->FriendlyName, sizeof (ifname));
	if (fdbg)
	  printf ("found Adapter: %s\n", ipname);
      }
      result = 0;
      break;
    }
    AdapterList = AdapterList->Next;
  }

  if (AdapterAddresses != NULL) {
    free (AdapterAddresses);
  }
  return result;
}

/*
 * Get BMC MAC corresponding to current BMC IP address (Windows).
 */
int
GetLocalIPByMAC (uchar * macadr)
{
  PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL;
  PIP_ADAPTER_ADDRESSES AdapterList;
  int result = 0;

  struct sockaddr_in *si;

  AdapterAddresses = GetAdapters ();
  AdapterList = AdapterAddresses;

  while (AdapterList) {
    PIP_ADAPTER_UNICAST_ADDRESS addr;
    if (memcmp (AdapterList->PhysicalAddress, macadr, MAC_LEN) == 0) {
      addr = AdapterList->FirstUnicastAddress;

      si = (struct sockaddr_in *) addr->Address.lpSockaddr;
      if (fdebug) {
	uchar *psaddr;
	psaddr = (uchar *) & si->sin_addr.s_addr;
	printf ("mac match: rgmyip=%d.%d.%d.%d s_addr=%d.%d.%d.%d\n",
		rgmyip[0], rgmyip[1], rgmyip[2], rgmyip[3],
		psaddr[0], psaddr[1], psaddr[2], psaddr[3]);
      }
      if (!IpIsValid (rgmyip) && (fsharedMAC == 1))	/*not specified, shared */
	memcpy (rgmyip, &si->sin_addr.s_addr, 4);
      memcpy (osmyip, &si->sin_addr.s_addr, 4);
      memcpy (osmymac, AdapterList->PhysicalAddress, MAC_LEN);
      wcstombs (ifname, AdapterList->FriendlyName, sizeof (ifname));
      result = 1;
      break;
    }
    AdapterList = AdapterList->Next;
  }

  if (AdapterAddresses != NULL) {
    free (AdapterAddresses);
  }
  return result;
}

/*
 * Set MAC and IP address from given interface name (Windows).
 */
int
GetLocalDataByIface ()
{
  PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL;
  PIP_ADAPTER_ADDRESSES AdapterList;
  int result = 0;

  size_t origsize, newsize, convertedChars;
  wchar_t *wcstring;
  struct sockaddr_in *si;

  AdapterAddresses = GetAdapters ();
  AdapterList = AdapterAddresses;

  origsize = strlen (ifname) + 1;
  newsize = origsize;
  convertedChars = 0;
  wcstring = (wchar_t *) malloc (sizeof (wchar_t) * newsize);
  if (wcstring == NULL)
    AdapterList = NULL;		/*skip loop, do free */
  else
    mbstowcs (wcstring, ifname, origsize);

  while (AdapterList) {
    PIP_ADAPTER_UNICAST_ADDRESS addr;
    if (wcsstr (AdapterList->FriendlyName, wcstring)) {
      printf ("Using interface: %S\n", AdapterList->FriendlyName);
      printf ("\t%S\n", AdapterList->Description);
      addr = AdapterList->FirstUnicastAddress;

      si = (struct sockaddr_in *) addr->Address.lpSockaddr;
      if (fdebug) {
	uchar *psaddr;
	psaddr = (uchar *) & si->sin_addr.s_addr;
	printf ("mac match: rgmyip=%d.%d.%d.%d s_addr=%d.%d.%d.%d "
		"fsharedMAC=%d\n",
		rgmyip[0], rgmyip[1], rgmyip[2], rgmyip[3],
		psaddr[0], psaddr[1], psaddr[2], psaddr[3], fsharedMAC);
      }
      if (!IpIsValid (rgmyip)) {	/*IP not specified */
	memcpy (rgmyip, &si->sin_addr.s_addr, 4);
	memcpy (rgmymac, AdapterList->PhysicalAddress, MAC_LEN);
      }
      memcpy (osmyip, &si->sin_addr.s_addr, 4);
      memcpy (osmymac, AdapterList->PhysicalAddress, MAC_LEN);
      /* FriendlyName == ifname already */
      result = 1;
      break;
    }
    AdapterList = AdapterList->Next;
  }

  if (AdapterAddresses != NULL) {
    free (AdapterAddresses);
  }
  return result;
}

int
FindEthNum (uchar * macadrin)
{
  int i;
  uchar macadr[MAC_LEN];
  memcpy (macadr, macadrin, MAC_LEN);
  if (fsharedMAC == 0 && vend_id == VENDOR_INTEL) {
    /* Intel factory assigns them this way, so use that to compare */
    macadr[MAC_LEN - 1] -= 2;	/*OS MAC = BMC MAC - 2 */
  }
  i = GetLocalIPByMAC (macadr);
  if (i == 1)
    fethfound = 1;
  if (fdebug)			/* show the local OS eth if and MAC */
    printf
      ("FindEth: OS %s IP=%d.%d.%d.%d MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
       ifname, osmyip[0], osmyip[1], osmyip[2], osmyip[3], osmymac[0],
       osmymac[1], osmymac[2], osmymac[3], osmymac[4], osmymac[5]);
  /* The actual Windows ethernet interface is determined 
   * in Get_IPMac_Addr using ipconfig, so
   * init eth interface number as eth0 for Windows. */
  return (0);
}
#elif defined(HPUX)
#define INSAP 22
#define OUTSAP 24

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <sys/stropts.h>
#include <sys/poll.h>
#include <sys/dlpi.h>

#define bcopy(source, destination, length) memcpy(destination, source, length)
#define AREA_SZ 5000 /*=* buffer length in bytes *=*/
#define GOT_CTRL 1
#define GOT_DATA 2
#define GOT_BOTH 3
#define GOT_INTR 4
#define GOT_ERR 128
static u_long ctl_area[AREA_SZ];
static u_long dat_area[AREA_SZ];
static struct strbuf ctl = { AREA_SZ, 0, (char *) ctl_area };
static struct strbuf dat = { AREA_SZ, 0, (char *) dat_area };
static char *dlpi_dev[] = { "/dev/dlpi", "" };

/*=* get a message from a stream; return type of message *=*/
static int
get_msg (int fd)
{
  int flags = 0;
  int res, ret;
  ctl_area[0] = 0;
  dat_area[0] = 0;
  ret = 0;
  res = getmsg (fd, &ctl, &dat, &flags);
  if (res < 0) {
    if (errno == EINTR) {
      return (GOT_INTR);
    }
    else {
      return (GOT_ERR);
    }
  }
  if (ctl.len > 0) {
    ret |= GOT_CTRL;
  }
  if (dat.len > 0) {
    ret |= GOT_DATA;
  }
  return (ret);
}

/*=* verify that dl_primitive in ctl_area = prim *=*/
static int
check_ctrl (int prim)
{
  dl_error_ack_t *err_ack = (dl_error_ack_t *) ctl_area;
  if (err_ack->dl_primitive != prim) {
    return GOT_ERR;
  }
  return 0;
}

/*=* put a control message on a stream *=*/
static int
put_ctrl (int fd, int len, int pri)
{
  ctl.len = len;
  if (putmsg (fd, &ctl, 0, pri) < 0) {
    return GOT_ERR;
  }
  return 0;
}

/*=* put a control + data message on a stream *=*/
static int
put_both (int fd, int clen, int dlen, int pri)
{
  ctl.len = clen;
  dat.len = dlen;
  if (putmsg (fd, &ctl, &dat, pri) < 0) {
    return GOT_ERR;
  }
  return 0;
}

/*=* open file descriptor and attach *=*/
static int
dl_open (const char *dev, int ppa, int *fd)
{
  dl_attach_req_t *attach_req = (dl_attach_req_t *) ctl_area;
  if ((*fd = open (dev, O_RDWR)) == -1) {
    return GOT_ERR;
  }
  attach_req->dl_primitive = DL_ATTACH_REQ;
  attach_req->dl_ppa = ppa;
  put_ctrl (*fd, sizeof (dl_attach_req_t), 0);
  get_msg (*fd);
  return check_ctrl (DL_OK_ACK);
}

/*=* send DL_BIND_REQ *=*/
static int
dl_bind (int fd, int sap, u_char * addr)
{
  dl_bind_req_t *bind_req = (dl_bind_req_t *) ctl_area;
  dl_bind_ack_t *bind_ack = (dl_bind_ack_t *) ctl_area;
  bind_req->dl_primitive = DL_BIND_REQ;
  bind_req->dl_sap = sap;
  bind_req->dl_max_conind = 1;
  bind_req->dl_service_mode = DL_CLDLS;
  bind_req->dl_conn_mgmt = 0;
  bind_req->dl_xidtest_flg = 0;
  put_ctrl (fd, sizeof (dl_bind_req_t), 0);
  get_msg (fd);
  if (GOT_ERR == check_ctrl (DL_BIND_ACK)) {
    return GOT_ERR;
  }
  bcopy ((u_char *) bind_ack + bind_ack->dl_addr_offset, addr,
	 bind_ack->dl_addr_length);
  return 0;
}

int
FindEthNum (uchar * addr)
{				/* Need to use DLPI for HPUX */
  /*See http://cplus.kompf.de/artikel/macaddr.html */
  int fd;
  int ppa;
  u_char mac_addr[25];
  char **dev;
  int i = 0;

  for (dev = dlpi_dev; **dev != ''; ++dev) {
    for (ppa = 0; ppa < 10; ++ppa) {
      if (GOT_ERR != dl_open (*dev, ppa, &fd)) {
	if (GOT_ERR != dl_bind (fd, INSAP, mac_addr)) {
	  // bcopy( mac_addr, addr, 6);
	  i = ppa;
	  if (memcmp (mac_addr, addr, MAC_LEN) == 0) {
	    memcpy (osmymac, addr, MAC_LEN);
	    return (ppa);
	  }
	}
      }
      close (fd);
    }
  }
  return (i);
}
#else

static char *
get_ifreq_mac (struct ifreq *ifrq)
{
  char *ptr;
#ifdef SOLARIS
  ptr = (char *) &ifrq->ifr_ifru.ifru_enaddr[0];
#elif BSD
  ptr = (char *) &ifrq->ifr_ifru.ifru_addr.sa_data[0];
#elif MACOS
  static uchar mactmp[MAC_LEN];
  ptr = &mactmp[0];
  MacSetInvalid (ptr);
#else
  ptr = (char *) &ifrq->ifr_hwaddr.sa_data[0];
#endif
  return (ptr);
}

extern int find_ifname (char *ifname);	/*see idiscover.c */

int
FindEthNum (uchar * macadrin)
{				/*only used for Linux */
  struct ifreq ifr;
  int skfd;
  int nCurDevice;
  int devnum = -1;
  int devos = 0;
  uchar macadr[MAC_LEN];
  uchar macsav[MAC_LEN];
  uchar mactmp[MAC_LEN];
  uchar ipsav[4];
  char szDeviceName[16];	/* MAX_DEVICE_NAME_LENGTH + 1 */
  uchar fipvalid = 0;
  int n;

  memcpy (macadr, macadrin, MAC_LEN);
  if (fsharedMAC == 0 && vend_id == VENDOR_INTEL) {
    /* Intel factory assigns them this way, so use that to compare */
    macadr[MAC_LEN - 1] -= 2;	/*OS MAC = BMC MAC - 2 */
  }
#ifdef DBG
  if (fdebug) {
    uchar *pb;
    pb = macadrin;
    printf ("input mac:%02x:%02x:%02x:%02x:%02x:%02x   "
	    "tmp mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
	    pb[0], pb[1], pb[2], pb[3], pb[4], pb[5],
	    macadr[0], macadr[1], macadr[2], macadr[3], macadr[4], macadr[5]);
  }
#endif

  n = find_ifname (szDeviceName);
  if (n >= 0) {
    n = strlen_ (szDeviceName);
    if (n < sizeof (ifpattn)) {
      strcpy (ifpattn, szDeviceName);
      ifpattn[n - 1] = 0;	/*truncate last digit */
    }
    if (fdebug)
      printf ("found ifname %s, pattern %s\n", szDeviceName, ifpattn);
  }

  if ((skfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
    if (fdebug) {
      perror ("socket");
      return devnum;
    }
  }

  for (nCurDevice = 0;
       (nCurDevice < NUM_DEVICES_TO_CHECK) && (devnum == -1); nCurDevice++) {
    sprintf (szDeviceName, "%s%d", ifpattn, nCurDevice);	/*eth%d */
    memset ((char *) &ifr, 0, sizeof (ifr));
    strcpy (ifr.ifr_name, szDeviceName);
#ifdef SIOCGIFHWADDR
    if (ioctl (skfd, SIOCGIFHWADDR, &ifr) > 0) {
      if (fdebug)
	printf ("FindEthNum: Could not get MAC address for %s\n",
		szDeviceName);
    }
    else
#endif
    {
      uchar *pb;
      pb = (uchar *) get_ifreq_mac (&ifr);
#ifdef DBG
      if (fdebug) {
	printf ("%s mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
		szDeviceName, pb[0], pb[1], pb[2], pb[3], pb[4], pb[5]);
      }
#endif
      memcpy (macsav, pb, MAC_LEN);
      /* check if this device is configured for IP addr */
      memset ((char *) &ifr, 0, sizeof (ifr));
      strcpy (ifr.ifr_name, szDeviceName);
      ifr.ifr_addr.sa_family = AF_INET;
      if (ioctl (skfd, SIOCGIFADDR, &ifr) >= 0) {
	fipvalid = 1;
	memcpy (ipsav, &ifr.ifr_addr.sa_data[2], 4);
      }
      else
	fipvalid = 0;
      if (memcmp (macsav, macadr, MAC_LEN) == 0) {	/*found match */
	devnum = nCurDevice;
	memcpy (osmymac, macsav, MAC_LEN);
	if (fipvalid)
	  memcpy (osmyip, ipsav, 4);
	break;
      }
      if (nCurDevice == 0) {	/*set a default of eth0 */
	devos = nCurDevice;
	memcpy (osmymac, macsav, MAC_LEN);
	if (fipvalid)
	  memcpy (osmyip, ipsav, 4);
      }
      else if (fipvalid) {	/*check if NIC1 is eth1,2,3,... */
	memcpy (mactmp, osmymac, MAC_LEN);
	mactmp[MAC_LEN - 1] -= 1;
	if (memcmp (mactmp, macsav, MAC_LEN) == 0) {
	  devos = nCurDevice;
	  memcpy (osmymac, macsav, MAC_LEN);
	  memcpy (osmyip, ipsav, 4);
	}
      }
    }				/*end else */
  }
  if (!fsetifn) {
    if (devnum == -1) {		/*not found, use devos default */
      devnum = devos;
      sprintf (ifname, "%s%d", ifpattn, devnum);	/*eth%d */
    }
    else {			/* match was found, devnum set */
      fethfound = 1;
      strcpy (ifname, szDeviceName);
    }
  }
  close (skfd);
  if (fdebug)			/* show the local OS eth if and MAC */
    printf
      ("FindEth: OS %s IP=%d.%d.%d.%d MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
       ifname, osmyip[0], osmyip[1], osmyip[2], osmyip[3], osmymac[0],
       osmymac[1], osmymac[2], osmymac[3], osmymac[4], osmymac[5]);
  return (devnum);
}
#endif

int
show_channels (void)
{
  int ret, rlen;
  uchar iData[2];
  uchar rData[10];
  uchar cc, mtype;
  int j;
  int rv = -1;

  for (j = 1; j < MAXCHAN; j++) {
    rlen = sizeof (rData);
    iData[0] = (uchar) j;	/*channel # */
    memset (rData, 0, 9);	/*initialize recv data */
    ret = ipmi_cmd (GET_CHANNEL_INFO, iData, 1, rData, &rlen, &cc, fdebug);
    if (rv != 0)
      rv = ret;
    if (ret == 0xcc || cc == 0xcc)	/* special case for ipmi_lan */
      continue;
    if (ret != 0) {
      if (fdebug)
	printf ("get_chan_info rc = %x\n", ret);
      break;
    }
    mtype = rData[1];		/* channel medium type */
    switch (mtype) {
    case 4:
      printf ("channel[%d] type = lan\n", j);
      break;			/*802.3 LAN type */
    case 5:
      printf ("channel[%d] type = serial\n", j);
      break;
    case 7:
      printf ("channel[%d] type = pci_smbus\n", j);
      break;
    case 12:
      printf ("channel[%d] type = system_interface\n", j);
      break;
    default:
      printf ("channel[%d] type = other %d\n", j, mtype);
      break;
    }
    rv = 0;
  }				/*end for j */
  return (rv);
}

/*
 * GetBmcEthDevice
 * Attempt to auto-detect the BMC LAN channel and matching OS eth port.
 * INPUT:  lan_parm = lan channel from user -L option, 0xFF if not specified
 * OUTPUT: lan_ch is set to BMC LAN channel number
 *         if success, returns index of OS eth port (0, 1, ...). 
 *         if no lan channels found, returns -2.
 *         if other error, returns -1.
 */
int
GetBmcEthDevice (uchar lan_parm, uchar * pchan)
{
  LAN_RECORD LanRecord;
  int devnum = -1;
  int ret;
  uchar bmcMacAddress[MAC_LEN];	/*MAC_LEN = 6 */
  int rlen;
  uchar iData[2];
  uchar rData[10];
  uchar cc;
  int i = 0;
  int j, jstart, jend, jlan;
  uchar mtype;
  uchar *pb;
  int fchgmac;
  // int found = 0;

  /* Find the LAN channel(s) via Channel Info */
  if (lan_parm < MAXCHAN) {	/* try user-specified channel only */
    lan_ch = lan_parm;
    jstart = lan_parm;
    jend = lan_parm + 1;
  }
  else {
    jstart = 1;
    jend = MAXCHAN;
  }
  memset (bmcMacAddress, 0xff, sizeof (bmcMacAddress));	/*initialize to invalid */
  for (j = jstart; j < jend; j++) {
    rlen = sizeof (rData);
    iData[0] = (uchar) j;	/*channel # */
    memset (rData, 0, 9);	/*initialize recv data */
    ret = ipmi_cmd (GET_CHANNEL_INFO, iData, 1, rData, &rlen, &cc, fdebug);
    if (ret == 0xcc || cc == 0xcc)	/* special case for ipmi_lan */
      continue;
    if (ret != 0) {
      if (fdebug)
	printf ("get_chan_info rc = %x\n", ret);
      break;
    }
    mtype = rData[1];		/* channel medium type */
    if (mtype == 4) {		/* 802.3 LAN type */
      if (fdebug)
	printf ("chan[%d] = lan\n", j);
      jlan = lan_ch;		/*save prev lan chan */
      /* Get BMC MAC for this LAN channel. */
      /* Note: BMC MAC may not be valid yet. */
      lan_ch = (uchar) j;	/*set lan channel for GetLanEntry */
      ret = GetLanEntry (5 /*MAC_ADDRESS_LAN_PARAM */ , 0, &LanRecord);
      if (ret != 0) {
         lan_ch = (uchar) jlan;	/*restore lan_ch */
         printf ("GetBmcEthDevice: GetLanEntry(5) failed\n");
         return devnum;
      }
      else {
	pb = &LanRecord.data[0];
	if (fdebug)
	  printf ("chan[%d] BMC MAC %x:%x:%x:%x:%x:%x\n", j,
		  pb[0], pb[1], pb[2], pb[3], pb[4], pb[5]);
	fchgmac = 0;
	/* use the lowest valid lan channel MAC address */
	if (!MacIsValid (bmcMacAddress))	/* old MAC not valid */
	  fchgmac = 1;
	else if (MacIsValid (pb) &&	/* new MAC is valid and */
		 (memcmp (bmcMacAddress, pb, sizeof (bmcMacAddress)) > 0))
	  fchgmac = 1;		/* new MAC lower */
	/* if no -L 3 and this is gcm, do not pick it. */
	if ((j == gcm_ch) && (lan_parm == PARM_INIT))
	  fchgmac = 0;
	if (fchgmac) {		/* pick this channel & MAC */
	  memcpy (bmcMacAddress, pb, sizeof (bmcMacAddress));
	  lan_ch = (uchar) j;
	}
	else
	  lan_ch = (uchar) jlan;	/*restore prev lan chan */
      }
      i++;			/* i = num lan channels found */
    }
    else if (mtype == 5) {	/* serial type */
      if (fdebug)
	printf ("chan[%d] = serial\n", j);
      ser_ch = (uchar) j;	/* set to last serial channel */
    }
    else if (mtype == 7) {	/* PCI SMBus */
      if (fdebug)
	printf ("chan[%d] = pci_smbus\n", j);
    }
    else if (mtype == 12) {	/* system interface */
      if (fdebug)
	printf ("chan[%d] = system_interface\n", j);
    }
    else if (mtype == 1) {	/* IPMB */
      if (fdebug)
	printf ("chan[%d] = IPMB\n", j);
    }
    else /* other channel medium types, see IPMI 1.5 Table 6-3 */ if (fdebug)
      printf ("chan[%d] = %d\n", j, mtype);
  }
  if (i == 0)
    return (-2);		/* no lan channels found */
  if (fdebug)
    printf ("lan_ch detected = %d\n", lan_ch);

  /* This will work if the BMC MAC is shared with the OS */
  /* Otherwise, wait until we get the eth dev from the gateway below */
  devnum = FindEthNum (bmcMacAddress);
  if (fdebug)
    printf ("GetBmcEthDevice: channel %d, %s%d\n", lan_ch, ifpattn, devnum);
  if (pchan != NULL)
    *pchan = lan_ch;
  return devnum;
}

/* file_grep/findmatch - No longer used here, see ievents.c */

/* 
 * Get_Mac
 * This routine finds a MAC address from a given IP address.
 * Usually for the Alert destination.
 * It uses ARP cache to do this.
 */
#if defined(WIN32)
int
Get_Mac (uchar * ipadr, uchar * macadr, char *nodname)
{
  DWORD dwRetVal;
  IPAddr DestIp = 0;
  IPAddr SrcIp = 0;		/* default for src ip */
  ULONG MacAddr[2];		/* for 6-byte hardware addresses */
  ULONG PhysAddrLen = MAC_LEN;	/* default to length of six bytes */
  BYTE *bPhysAddr;

  if (!IpIsValid (ipadr)) {
    if (fdebug)
      printf ("Get_Mac: invalid IP addr\n");
    return 1;			/*error */
  }
  memcpy (&DestIp, ipadr, 4);

  /* invoke system ARP query */
  dwRetVal = SendARP (DestIp, SrcIp, MacAddr, &PhysAddrLen);

  if (dwRetVal == NO_ERROR) {	/* no error - get the MAC */
    bPhysAddr = (BYTE *) & MacAddr;
    if (PhysAddrLen) {
      memcpy (macadr, bPhysAddr, MAC_LEN);
    }
    else
      printf
	("Warning: SendArp completed successfully, but returned length=0\n");
  }
  else if (dwRetVal == ERROR_GEN_FAILURE) {	/* MAC not available in this network - get gateway MAC */
    memcpy (macadr, rggwymac, MAC_LEN);
  }
  else {			/* other errors */
    printf ("Error: SendArp failed with error: %d", dwRetVal);
    switch (dwRetVal) {
    case ERROR_INVALID_PARAMETER:
      printf (" (ERROR_INVALID_PARAMETER)\n");
      break;
    case ERROR_INVALID_USER_BUFFER:
      printf (" (ERROR_INVALID_USER_BUFFER)\n");
      break;
    case ERROR_BAD_NET_NAME:
      printf (" (ERROR_GEN_FAILURE)\n");
      break;
    case ERROR_BUFFER_OVERFLOW:
      printf (" (ERROR_BUFFER_OVERFLOW)\n");
      break;
    case ERROR_NOT_FOUND:
      printf (" (ERROR_NOT_FOUND)\n");
      break;
    default:
      printf ("\n");
      break;
    }
    return 1;
  }
  return 0;
}				/* end Get_Mac() for WIN32 */
#elif defined(SOLARIS)
int
Get_Mac (uchar * ipadr, uchar * macadr, char *nodename)
{
  FILE *fparp;
  char buff[1024];
  /* char arpfile[] = "/proc/net/arp";  */
  char alertfile[] = "/tmp/dest.arping";
  char arping_cmd[128];
  char *pb, *pm;
  int num, i;
  int foundit = 0;
  int ret = 0;

  if (IpIsValid (ipadr)) {	/* if valid IP address */
    sprintf (arping_cmd,
	     "ping %d.%d.%d.%d >/dev/null; arp -a -n |grep %d.%d.%d.%d  >%s\n",
	     ipadr[0], ipadr[1], ipadr[2], ipadr[3],
	     ipadr[0], ipadr[1], ipadr[2], ipadr[3], alertfile);
  }
  else if (nodename != NULL) {	/*if valid nodename */
    sprintf (arping_cmd,
	     "ping %s >/dev/null; arp -a |grep %s  >%s\n",
	     nodename, nodename, alertfile);
  }
  else
    ret = -1;

  if (ret == 0) {		/* if valid IP address */
    /* make sure the destination is in the arp cache */
    if (fdebug)
      printf ("%s", arping_cmd);
    system (arping_cmd);

    fparp = fopen (alertfile, "r");
    if (fparp == NULL) {
      fprintf (stdout, "Get_Mac: Cannot open %s, errno = %d\n",
	       alertfile, get_errno ());
      ret = -1;
    }
    else {
      /* sample output: */
      /* e1000g0 cooper9     255.255.255.255 o   00:07:e9:06:55:c8 */
      while (fgets (buff, 1023, fparp)) {
	/* should only run through loop once */
	num = strcspn (buff, " \t");	/* skip 1st word ("e1000g0") */
	i = strspn (&buff[num], " \t");	/* skip whitespace */
	pb = &buff[num + i];
	num = strcspn (pb, " \t");	/* skip 2nd word (nodename/IP) */
	i = strspn (&pb[num], " \t");	/* skip whitespace */
	pb += (num + i);
	pm = &pb[25];		/* Now pb[25] has the MAC address */
	{			/*validate new address? */
	  if (fdebug)
	    printf ("Get_Mac: mac=%s\n", pm);
	  foundit = 1;
	  if (!MacIsValid (macadr))
	    atomac (macadr, pm);
	  break;
	}
      }				/*end while */
      fclose (fparp);
    }				/*end else file opened */
  }				/*endif valid IP */

  if (foundit == 0) {		/* no errors, but no mac reply */
    if (MacIsValid (rggwymac) && !MacIsValid (macadr))
      /* this is useful if the ipadr is not in the local subnet */
      memcpy (macadr, rggwymac, 6);	/* get to it from the default gateway */
  }
  return (ret);
}				/*end Get_Mac for Solaris */
#elif defined(BSD)
int
Get_Mac (uchar * ipadr, uchar * macadr, char *nodename)
{
  FILE *fparp;
  char buff[1024];
  /* char arpfile[] = "/proc/net/arp";  */
  char alertfile[] = "/tmp/dest.arping";
  char arping_cmd[128];
  char *pb, *pm;
  int num, i, j;
  int foundit = 0;
  int ret = 0;

  if (IpIsValid (ipadr)) {	/* if valid IP address */
    sprintf (arping_cmd,
	     "ping -c2 %d.%d.%d.%d >/dev/null; arp -a -n |grep %d.%d.%d.%d >%s\n",
	     ipadr[0], ipadr[1], ipadr[2], ipadr[3],
	     ipadr[0], ipadr[1], ipadr[2], ipadr[3], alertfile);
  }
  else if (nodename != NULL) {	/*if valid nodename */
    sprintf (arping_cmd,
	     "ping -c2 %s >/dev/null; arp -a |grep %s  >%s\n",
	     nodename, nodename, alertfile);
  }
  else
    ret = -1;

  if (ret == 0) {		/* if valid IP address */
    /* make sure the destination is in the arp cache */
    if (fdebug)
      printf ("%s", arping_cmd);
    system (arping_cmd);

    fparp = fopen (alertfile, "r");
    if (fparp == NULL) {
      fprintf (stdout, "Get_Mac: Cannot open %s, errno = %d\n",
	       alertfile, get_errno ());
      ret = -1;
    }
    else {
      /* sample output of arp -a -n: */
      /* ? (192.168.1.200) at 00:0e:0c:e5:df:65 on em0 [ethernet] */
      /* sample output of arp -a: */
      /* telcoserv (192.168.1.200) at 00:0e:0c:e5:df:65 on em0 [ethernet] */
      while (fgets (buff, 1023, fparp)) {
	/* should only run through loop once */
	pb = &buff[0];
	for (j = 0; j < 3; j++) {	/* skip 3 words */
	  num = strcspn (pb, " \t");	/* skip jth word */
	  i = strspn (&pb[num], " \t");	/* skip whitespace */
	  pb += (num + i);
	}
	pm = &pb[0];		/* Now pb[0] has the MAC address */
	{			/* no need to validate new address */
	  if (fdebug)
	    printf ("Get_Mac: mac=%s\n", pm);
	  foundit = 1;
	  if (!MacIsValid (macadr))
	    atomac (macadr, pm);
	  break;
	}
      }				/*end while */
      fclose (fparp);
    }				/*end else file opened */
  }				/*endif valid IP */

  if (foundit == 0) {		/* no errors, but no mac reply */
    if (MacIsValid (rggwymac) && !MacIsValid (macadr))
      /* this is useful if the ipadr is not in the local subnet */
      memcpy (macadr, rggwymac, 6);	/* get to it from the default gateway */
  }
  return (ret);
}				/*end Get_Mac for BSD */
#else
int
Get_Mac (uchar * ipadr, uchar * macadr, char *nodename)
{				/* Get_Mac for Linux */
  FILE *fparp;
  char buff[1024];
  /* char arpfile[] = "/proc/net/arp";  */
  char alertfile[] = "/tmp/dest.arping";
  char arping_cmd[128];
  char *pb, *pm, *px;
  int num, i;
  int foundit = 0;
  int ret = 0;
  char *_ifname;

  if (strcmp (ifname, "gcm") == 0)
    _ifname = ifname0;		/*see gcm_ch instead */
  else
    _ifname = ifname;

  /* Get a MAC address for a given IP address or nodename */
  if (IpIsValid (ipadr)) {	/* if valid IP address */
    sprintf (arping_cmd,
	     "arping -I %s -c 2 %d.%d.%d.%d |grep reply |tail -n1 >%s\n",
	     _ifname, ipadr[0], ipadr[1], ipadr[2], ipadr[3], alertfile);
  }
  else if (nodename != NULL) {	/*if valid nodename */
    sprintf (arping_cmd,
	     "arping -I %s -c 2 %s |grep reply |tail -n1 >%s\n",
	     _ifname, nodename, alertfile);
  }
  else
    ret = -1;

  if (ret == 0) {		/* if valid IP address */
    /* make sure the destination is in the arp cache */
    if (fdebug)
      printf ("%s", arping_cmd);
    ret = system (arping_cmd);

    fparp = fopen (alertfile, "r");
    if (fparp == NULL) {
      fprintf (stdout, "Get_Mac: Cannot open %s, errno = %d\n",
	       alertfile, get_errno ());
      ret = -1;
    }
    else {
      ret = 0;
      while (fgets (buff, 1023, fparp)) {
	/* should only run through loop once */
	num = strcspn (buff, " \t");	/* skip 1st word ("Unicast") */
	i = strspn (&buff[num], " \t");
	pb = &buff[num + i];
	if (strncmp (pb, "reply", 5) == 0) {	/* valid output */
	  /* Find the ip address */
	  pb += 6 + 5;		/* skip "reply from " */
	  num = strcspn (pb, " \t");
	  pb[num] = 0;
	  if (fdebug)
	    printf ("Get_Mac: ip=%s\n", pb);
	  /* IP address should already match input param */
	  if (!IpIsValid (ipadr))	/* had nodname only */
	    atoip (ipadr, pb);	/* fill in ipadr */
	  /* Now find the mac address */
	  pm = strchr (&pb[num + 1], '[');
	  if (pm == NULL)
	    pm = &pb[num + 2];	/* just in case */
	  pm++;
	  px = strchr (pm, ']');
	  if (px == NULL)
	    px = pm + 17;	/* just in case */
	  px[0] = 0;
	  if (fdebug)
	    printf ("Get_Mac: mac=%s\n", pm);
	  foundit = 1;
	  if (!MacIsValid (macadr))
	    atomac (macadr, pm);
	  break;
	}
      }				/*end while */
      fclose (fparp);
    }				/*end else file opened */
  }				/*endif valid IP */

  if (foundit == 0) {		/* no errors, but no mac reply */
    if (MacIsValid (rggwymac) && !MacIsValid (macadr))
      /* this is useful if the ipadr is not in the local subnet */
      memcpy (macadr, rggwymac, 6);	/* get to it from the default gateway */
  }
  return (ret);
}				/* end Get_Mac() for Linux */
#endif

#ifdef WIN32
/*
 * Set subnet mask based on current IP address (Windows).
 */
int
SetSubnetMask ()
{
  PMIB_IPADDRTABLE pIPAddrTable;
  unsigned int i;
  DWORD dwSize = 0, dwRetVal;
  LPVOID lpMsgBuf;

  pIPAddrTable = (MIB_IPADDRTABLE *) malloc (sizeof (MIB_IPADDRTABLE));

  if (pIPAddrTable) {
    // Make an initial call to GetIpAddrTable to get the
    // necessary size into the dwSize variable
    if (GetIpAddrTable (pIPAddrTable, &dwSize, 0) ==
	ERROR_INSUFFICIENT_BUFFER) {
      free (pIPAddrTable);
      pIPAddrTable = (MIB_IPADDRTABLE *) malloc (dwSize);
    }
  }
  else
    printf ("Memory allocation failed.\n");

  if (pIPAddrTable) {
    // Make a second call to GetIpAddrTable to get the
    // actual data we want
    if ((dwRetVal = GetIpAddrTable (pIPAddrTable, &dwSize, 0)) == NO_ERROR) {
      for (i = 0; i < pIPAddrTable->dwNumEntries; ++i) {
	if (memcmp (&(pIPAddrTable->table[i].dwAddr), rgmyip, 4) == 0) {
	  memcpy (rgsubnet, &(pIPAddrTable->table[i].dwMask), 4);
	  free (pIPAddrTable);
	  return 1;
	}
      }
    }
    else {
      if (FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwRetVal, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),	// Default language
			 (LPTSTR) & lpMsgBuf, 0, NULL)) {
	printf ("\tError: %s", (char *)lpMsgBuf);
      }

      printf ("Call to GetIpAddrTable failed.\n");
    }
  }

  if (pIPAddrTable)
    free (pIPAddrTable);

  return 0;
}

/*
 * Extract gateway address from routing table (Windows).
 */
int
SetDefaultGateway ()
{
  PMIB_IPFORWARDTABLE pIpForwardTable;
  DWORD dwRetVal, dwSize;

  unsigned int nord_mask;
  unsigned int nord_ip;
  unsigned int nord_net;

  unsigned int i;

  nord_mask = *((unsigned int *) rgsubnet);
  nord_ip = *((unsigned int *) rgmyip);

  nord_net = nord_ip & nord_mask;

  pIpForwardTable =
    (MIB_IPFORWARDTABLE *) malloc (sizeof (MIB_IPFORWARDTABLE));
  if (pIpForwardTable == NULL) {
    printf ("Error allocating memory\n");
    return 0;
  }

  dwSize = 0;
  if (GetIpForwardTable (pIpForwardTable, &dwSize, 0) ==
      ERROR_INSUFFICIENT_BUFFER) {
    free (pIpForwardTable);
    pIpForwardTable = (MIB_IPFORWARDTABLE *) malloc (dwSize);
    if (pIpForwardTable == NULL) {
      printf ("Error allocating memory\n");
      return 0;
    }
  }

  /* 
   * Note that the IPv4 addresses returned in 
   * GetIpForwardTable entries are in network byte order 
   */
  if ((dwRetVal =
       GetIpForwardTable (pIpForwardTable, &dwSize, 0)) == NO_ERROR) {
    for (i = 0; i < (int) pIpForwardTable->dwNumEntries; i++) {
      unsigned int gwaddr = pIpForwardTable->table[i].dwForwardNextHop;
      if (nord_net == (gwaddr & nord_mask) && nord_ip != gwaddr) {	/* searching for gateways from our network with different address than ours */
	memcpy (rggwyip, &gwaddr, 4);
	return 0;
      }
    }
    free (pIpForwardTable);
    return 1;
  }
  else {
    printf ("\tGetIpForwardTable failed.\n");
    free (pIpForwardTable);
    return 0;
  }

}

/*endif  WIN32*/
#endif

/* 
 * Get_IPMac_Addr
 * This routine finds the IP and MAC for the local interface, 
 * the default gateway, and SNMP alert destination from the 
 * BMC and OS information.
 *
 * Linux snmpd.conf locations (see ipmiutil.spec):
 * RedHat, MontaVista:  /etc/snmp/snmpd.conf
 * SuSE SLES 8:   /etc/ucdsnmpd.conf
 * SuSE SLES 9:   /etc/snmpd.conf
 */
#ifdef WIN32
int
Get_IPMac_Addr ()
{				/*for Windows */
  char ipstr[] = "IP Address";
  char macstr[] = "Physical Address";
  char gwystr[] = "Default Gateway";
  char substr[] = "Subnet Mask";
  int found = 0;
  char fgetmac;

  if (IpIsValid (rgmyip)) {	/* user-specified ip, get mac */
    if (fdebug)
      printf ("User IP was specified\n");
    if (!MacIsValid (rgmymac)) {	/* no user-specified MAC, get it from IP */
      if (fdebug)
	printf ("No user MAC specified\n");
      if (!GetLocalMACByIP ()) {	/* couldn't get MAC from IP, get old one */
	if (fdebug)
	  printf ("No MAC from IP, use old\n");
	if (IpIsValid (bmcmyip) && MacIsValid (bmcmymac)) {
	  printf ("Using current BMC MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
		  bmcmymac[0], bmcmymac[1],
		  bmcmymac[2], bmcmymac[3], bmcmymac[4], bmcmymac[5]);
	  memcpy (rgmymac, bmcmymac, MAC_LEN);
	}
	else {
	  printf ("Failed to obtain valid MAC\n");
	}
      }
      else {
	printf ("Using adapter MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
		rgmymac[0], rgmymac[1],
		rgmymac[2], rgmymac[3], rgmymac[4], rgmymac[5]);
      }
    }
    else {			/* user MAC available */
      printf ("Using user MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
	      rgmymac[0], rgmymac[1],
	      rgmymac[2], rgmymac[3], rgmymac[4], rgmymac[5]);
    }
  }
  else {			/* no user-specified IP */
    if (fdebug)
      printf ("No user IP specified\n");
    if (!MacIsValid (rgmymac)) {	/* no user-specified MAC, get it from interface */
      if (!GetLocalDataByIface ()) {	/* use existing MAC an IP */
	printf ("Using current BMC IP %d.%d.%d.%d\n",
		bmcmyip[0], bmcmyip[1], bmcmyip[2], bmcmyip[3]);
	memcpy (rgmyip, bmcmyip, 4);
	printf ("Using current BMC MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
		bmcmymac[0], bmcmymac[1],
		bmcmymac[2], bmcmymac[3], bmcmymac[4], bmcmymac[5]);
	memcpy (rgmymac, bmcmymac, MAC_LEN);

      }
    }
    else {			/* user-specified MAC */
      if (!GetLocalIPByMAC (rgmymac)) {	/* use existing MAC and IP */
	printf ("Using current BMC IP %d.%d.%d.%d\n",
		bmcmyip[0], bmcmyip[1], bmcmyip[2], bmcmyip[3]);
	memcpy (rgmyip, bmcmyip, 4);
	printf ("Using current BMC MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
		bmcmymac[0], bmcmymac[1],
		bmcmymac[2], bmcmymac[3], bmcmymac[4], bmcmymac[5]);
	memcpy (rgmymac, bmcmymac, MAC_LEN);
      }
    }
  }

  if (rghostname[0] == 0) {	/*hostname not specified */
    if (!fipmilan)
      gethostname (rghostname, sizeof (rghostname));
  }
  if (fdebug)			/* show the local OS eth if and MAC */
    printf
      ("OS %s IP=%d.%d.%d.%d %s MAC=%02x:%02x:%02x:%02x:%02x:%02x used for arp\n",
       ifname, osmyip[0], osmyip[1], osmyip[2], osmyip[3], rghostname,
       osmymac[0], osmymac[1], osmymac[2], osmymac[3], osmymac[4],
       osmymac[5]);

  if (!SubnetIsValid (rgsubnet)) {
    SetSubnetMask ();
  }
  if (!IpIsValid (rggwyip)) {	/* if gwy ip not user-specified */
    SetDefaultGateway ();
  }

  if (lan_ch == gcm_ch) {
    if (SubnetIsSame (rgmyip, rggwyip, rgsubnet))
      fgetmac = 1;
    else
      fgetmac = 0;
  }
  else
    fgetmac = 1;

  if (fgetmac) {
    if (IpIsValid (rggwyip) && !MacIsValid (rggwymac))	/*gwy mac not specified */
      Get_Mac (rggwyip, rggwymac, NULL);

    if (IpIsValid (rggwy2ip) && !MacIsValid (rggwy2mac))	/*gwy2 mac not valid */
      Get_Mac (rggwy2ip, rggwy2mac, NULL);
  }

  return (0);
}				/* end Get_IPMac_Addr for Windows */

#elif defined(HPUX)
int
Get_IPMac_Addr ()
{				/*for HP-UX */
  return (-1);
}
#else
int
Get_IPMac_Addr ()
{				/*for Linux */
#if defined(SOLARIS)
  char rtfile[] = "/tmp/route";
  char snmpfile[] = "/etc/snmp/conf/snmpd.conf";
#elif defined(BSD)
  char rtfile[] = "/tmp/route";
  char snmpfile[] = "/etc/snmpd.config";
#else
  char rtfile[] = "/proc/net/route";
  char snmpfile[] = "/etc/snmp/snmpd.conf";
#endif
  // char alertfile[] = "/tmp/alert.arping";
  // FILE *fparp;
  FILE *fprt;
  int fd = -1;
  int skfd;
  uchar *pc;
  int rc = 0;
  int i, j;
  uchar bnetadr[4];
  uchar bgateadr[4];
  char gate_addr[128];
  char iface[16];
  char defcommunity[19] = "public";
  char buff[1024];
  char alertname[60];
  int num, nmatch;
  struct ifreq ifr;
  char *_ifname;
  char fgetmac;

  /* Get the IP address and associated MAC address specified. */
  /* Local for ethN; Default Gateway; Alert Destination */

  /* Get the default gateway IP */
#if defined(SOLARIS) || defined(BSD)
  char rtcmd[80];
  sprintf (rtcmd, "netstat -r -n |grep default |awk '{ print $2 }' >%s",
	   rtfile);
  system (rtcmd);
  /* use rtfile output from netstat -r, see also /etc/defaultroute */
  fprt = fopen (rtfile, "r");
  if (fprt == NULL) {
    fprintf (stdout, "netstat: Cannot open %s, errno = %d\n", rtfile,
	     get_errno ());
  }
  else {
    while (fgets (buff, 1023, fprt)) {
      if ((buff[0] > '0') && (buff[0] <= '9')) {	/*valid */
	atoip (bgateadr, buff);
	if (fdebug)
	  printf ("default gateway: %s, %d.%d.%d.%d %s\n", buff,
		  bgateadr[0], bgateadr[1], bgateadr[2], bgateadr[3], ifname);
	if (!IpIsValid (rggwyip))	/* if not user-specified */
	  memcpy (rggwyip, bgateadr, 4);
	break;
      }
    }
    fclose (fprt);
  }				/*end-else good open */
#else
  /* cat /proc/net/route and save Gwy if Dest == 0 and Gateway != 0 */
  fprt = fopen (rtfile, "r");
  if (fprt == NULL) {
    fprintf (stdout, "route: Cannot open %s, errno = %d\n", rtfile,
	     get_errno ());
  }
  else {
    char rtfmt[] = "%16s %128s %128s %X %d %d %d %128s %d %d %d\n";
    int iflags, refcnt, use, metric, mss, window, irtt;
    char mask_addr[128], net_addr[128];
    uint *pnet;
    uint *pgate;

    pnet = (uint *) & bnetadr[0];
    pgate = (uint *) & bgateadr[0];
    while (fgets (buff, 1023, fprt)) {
      num = sscanf (buff, rtfmt,
		    iface, net_addr, gate_addr,
		    &iflags, &refcnt, &use, &metric, mask_addr,
		    &mss, &window, &irtt);
      if (num < 10 || !(iflags & RTF_UP))
	continue;

      j = 6;
      for (i = 0; i < 4; i++) {
	bnetadr[i] = htoi (&net_addr[j]);
	bgateadr[i] = htoi (&gate_addr[j]);
	j -= 2;
      }
      if ((*pnet == 0) && (*pgate != 0)) {	/* found default gateway */
	if (fdebug)
	  printf ("default gateway: %s, %d.%d.%d.%d %s\n", gate_addr,
		  bgateadr[0], bgateadr[1], bgateadr[2], bgateadr[3], iface);
	if (!IpIsValid (rggwyip))	/* if not user-specified */
	  memcpy (rggwyip, bgateadr, 4);
	_ifname = iface;	/*use this iface for gwy mac */
	if (!fsetifn)
	  strncpy (ifname, iface, 16);
	break;
      }
    }				/*end while */
    fclose (fprt);
  }				/*end-else good open */
#endif

  /* Create a channel to the NET kernel. */
  if ((skfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror ("socket");
    return (-3);
  }

  /* Find a valid local BMC MAC Address */
  if (lan_ch == gcm_ch) {
    /* GCM has its own unique mac address */
    if (!MacIsValid (rgmymac)) {
      if (MacIsValid (bmcmymac)) {
	memcpy (rgmymac, bmcmymac, MAC_LEN);	/*use existing */
      }
      else {			/*error */
	printf ("invalid MAC address for gcm\n");
      }
    }				/*else use rgmymac if specified */
    _ifname = iface;		/*use the iface from gwy (e.g. eth0) */
    fsharedMAC = 0;		/*gcm has separate NIC, separate MAC */

    // fd = -1;
    fd = skfd;			// get_socket_for_af(AF_INET) below;
  }
  else {			/*else not gcm */
    if (!fsetifn && !fethfound) {	/*do not have ifname yet */
      i = FindEthNum (bmcmymac);
      if (i >= 0)
	sprintf (ifname, "%s%d", ifpattn, i);
    }
    ifr.ifr_addr.sa_family = AF_INET;
    if (fdebug)
      printf ("getipmac: ifname=%s\n", ifname);
    _ifname = ifname;
    strcpy (ifr.ifr_name, _ifname);

#ifdef SIOCGIFHWADDR
    /* Get the local if HWADDR (MAC Address) from OS */
    if (ioctl (skfd, SIOCGIFHWADDR, &ifr) < 0) {
      if (fdebug)
	printf ("ioctl(SIOCGIFHWADDR,%s) error, errno = %d\n", _ifname,
		get_errno ());
    }
    else
      memcpy (osmymac, get_ifreq_mac (&ifr), MAC_LEN);	/*OS mac */
#endif

    if (memcmp (bmcmymac, osmymac, MAC_LEN) == 0) {
      /* osmymac and ifname were set above by FindEthNum */
      printf ("\tBMC shares IP/MAC with OS NIC %s\n", _ifname);
      fsharedMAC = 1;
    }				/* else fsharedMAC = 0; */

    if (fsharedMAC == 0) {	/* then BMC has separate MAC */
      if (!MacIsValid (rgmymac) && MacIsValid (bmcmymac))
	memcpy (rgmymac, bmcmymac, MAC_LEN);	/*use existing */
    }
    else {			/* else OS & BMC share a MAC */
      /* Use the local if HWADDR (MAC Address) from OS */
      if (!MacIsValid (rgmymac)) {	/* if not user-specified */
	if (MacIsValid (bmcmymac))
	  memcpy (rgmymac, bmcmymac, MAC_LEN);	/*use existing */
	else
	  memcpy (rgmymac, osmymac, MAC_LEN);	/*use OS mac */
      }
    }
    fd = skfd;			// get_socket_for_af(AF_INET) below;
  }

  if (fd >= 0) {		/* if valid fd, find OS IP */
    strcpy (ifr.ifr_name, _ifname);
    ifr.ifr_addr.sa_family = AF_INET;
    /* Get the IFADDR (IP Address) from OS */
    if (ioctl (fd, SIOCGIFADDR, &ifr) < 0) {
      int err;
      err = get_errno ();
      /* errno 99 here means that eth0 is not enabled/up/defined. */
      if (err == 99)
	printf ("ioctl(SIOCGIFADDR) error, %s not enabled\n", _ifname);
      else if (fdebug)
	printf ("ioctl(SIOCGIFADDR,%s) error, errno=%d\n", _ifname, err);
    }
    else {			/* got the local OS IP successfully */
      pc = (uchar *) & ifr.ifr_addr.sa_data[2];
      if (fdebug)
	printf ("%s addr = %d.%d.%d.%d\n", _ifname, pc[0], pc[1], pc[2],
		pc[3]);
      memcpy (osmyip, pc, 4);
      if (!IpIsValid (rgmyip) && (fsharedMAC == 1))	/*not specified, shared */
	memcpy (rgmyip, osmyip, 4);

      /* get the local OS netmask */
      strcpy (ifr.ifr_name, _ifname);
      if (ioctl (fd, SIOCGIFNETMASK, &ifr) < 0) {
	if (fdebug)
	  printf ("ioctl(SIOCGIFNETMASK) error, errno=%d\n", get_errno ());
	/* if leave invalid, will use default rgsubnet */
      }
      else {			// sizeof(struct sockaddr)
	pc = (uchar *) & ifr.ifr_netmask.sa_data[2];
	if (fdebug)
	  printf ("subnet = %d.%d.%d.%d \n", pc[0], pc[1], pc[2], pc[3]);
	memcpy (ossubnet, pc, 4);
	if (!SubnetIsValid (rgsubnet) && fsharedMAC)	/*not specified */
	  memcpy (rgsubnet, pc, 4);
      }

#ifdef SIOCGIFHWADDR
      /* Get the localhost OS HWADDR (MAC Address) */
      if (ioctl (fd, SIOCGIFHWADDR, &ifr) < 0) {
	if (fdebug)
	  printf ("ioctl(SIOCGIFHWADDR,%s) error, errno = %d\n",
		  _ifname, get_errno ());
      }
      else
	memcpy (osmymac, get_ifreq_mac (&ifr), MAC_LEN);	/*OS mac */
#endif
    }
  }
  close (skfd);			/* done, close the socket */

  if (rghostname[0] == 0) {	/*hostname not specified */
    if (!fipmilan)
      gethostname (rghostname, sizeof (rghostname));
  }
  if (fdebug)			/* show the local OS eth if and MAC */
    printf
      ("OS %s IP=%d.%d.%d.%d %s MAC=%02x:%02x:%02x:%02x:%02x:%02x used for arp\n",
       _ifname, osmyip[0], osmyip[1], osmyip[2], osmyip[3], rghostname,
       osmymac[0], osmymac[1], osmymac[2], osmymac[3], osmymac[4],
       osmymac[5]);

  if (!IpIsValid (rgmyip) && IpIsValid (bmcmyip)) {
    /* If no user-specified IP and there is a valid IP already in the 
     * BMC LAN configuration, use the existing BMC LAN IP.  */
    memcpy (rgmyip, bmcmyip, 4);
    if (fdebug)
      printf ("Using current IP %d.%d.%d.%d\n",
	      bmcmyip[0], bmcmyip[1], bmcmyip[2], bmcmyip[3]);
  }

  /* Get the default gateway MAC */
  if (lan_ch == gcm_ch) {
    if (SubnetIsSame (osmyip, rggwyip, ossubnet))
      fgetmac = 1;
    else {			/* gateway is not on the same subnet as RMM/GCM */
      fgetmac = 0;		/*don't try to get mac if not the same subnet */
      if ((fset_ip & GWYIP) == 0)
	memset (rggwyip, 0, 4);
    }
  }
  else
    fgetmac = 1;
  if (fgetmac && IpIsValid (rggwyip) && !MacIsValid (rggwymac))
    Get_Mac (rggwyip, rggwymac, NULL);	/*gwy mac not specified, so get mac */

  /* Get the Alert Destination IP */
  /* By default, attempt to obtain this from /etc/snmp/snmpd.conf. */
  /* cat /etc/snmp/snmpd.conf | grep trapsink |tail -n1 | cut -f2 -d' ' */
  alertname[0] = 0;		/* default to null string */
  fprt = fopen (snmpfile, "r");
  if (fprt == NULL) {
    printf ("snmp: Cannot open %s, errno = %d\n", snmpfile, get_errno ());
  }
  else {
    // char snmpfmt[] = "%20s %60s\n";
    // char *keywd, *value;
    while (fgets (buff, 1023, fprt)) {
      /* parse each line */
      if (buff[0] == '#')
	continue;		/*skip comment lines */
      /* skip leading whitespace here */
      j = strspn (&buff[0], " \t");
      if (strncmp (&buff[j], "com2sec", 7) == 0) {	/* found community line */
	/* usu like this: "com2sec demouser default  public" */
	i = j + 7;
	for (j = 0; j < 3; j++) {
	  num = strspn (&buff[i], " \t");
	  i += num;
	  num = strcspn (&buff[i], " \t\r\n");
	  if (j < 2)
	    i += num;
	}
	buff[i + num] = 0;
	if (fsetcommunity == 0) {	/* if not user-specified */
	  strcpy (rgcommunity, &buff[i]);
	  strcpy (defcommunity, &buff[i]);
	}
      }
#ifdef BSD
      if (strncmp (&buff[j], "traphost :=", 11) == 0)
	nmatch = 11;
      else
	nmatch = 0;
#else
      if (strncmp (&buff[j], "trapsink", 8) == 0)
	nmatch = 8;
      else if (strncmp (&buff[j], "trap2sink", 9) == 0)
	nmatch = 9;
      else
	nmatch = 0;
#endif
      if (nmatch > 0) {		/* found trapsink line match */
	if (fdebug)
	  printf ("%s: %s", snmpfile, &buff[j]);
	num = strspn (&buff[j + nmatch], " \t");
	i = j + nmatch + num;
	if (buff[i] == '`')
	  continue;
	num = strcspn (&buff[i], " \t\r\n");
	strncpy (alertname, &buff[i], num);	/* save alert destination */
	alertname[num] = 0;
	i += num;
	num = strspn (&buff[i], " \t");	/*skip whitespace */
	i += num;
	num = strcspn (&buff[i], " \t\r\n");	/*span next word */
	if (num != 0) {		/* there is another word, that is community */
	  if (fsetcommunity == 0) {	/* if not user-specified */
	    strncpy (rgcommunity, &buff[i], num);	/* save community */
	    rgcommunity[num] = 0;
	  }
	}
	else {			/*have trapsink node with no community */
	  /* use previously discovered default community from above */
	  strcpy (rgcommunity, defcommunity);
	}
	/* dont break, keep looking, use the last one */
      }
    }				/*end while */
    fclose (fprt);
    if (fdebug)
      printf ("snmp alertname=%s community=%s\n", alertname, rgcommunity);
  }				/*end else snmpfile */

  /* Get the Alert Destination MAC from the alertname. */
  if (alertname[0] != 0) {
#ifdef TEST
    char arping_cmd[128];
    char *pb, *pm, *px;
    int num, i;
    if (fdebug)
      printf ("alert %s ip=%d.%d.%d.%d osip=%d.%d.%d.%d "
	      "mac=%02x:%02x:%02x:%02x:%02x:%02x "
	      "osmac=%02x:%02x:%02x:%02x:%02x:%02x\n",
	      alertname,
	      rgdestip[0], rgdestip[1], rgdestip[2], rgdestip[3],
	      osmyip[0], osmyip[1], osmyip[2], osmyip[3],
	      rgdestmac[0], rgdestmac[1], rgdestmac[2], rgdestmac[3],
	      rgdestmac[4], rgdestmac[5],
	      osmymac[0], osmymac[1], osmymac[2], osmymac[3],
	      osmymac[4], osmymac[5]);
#endif
    if (!IpIsValid (rgdestip)) {	/* if not user-specified with -A */
      if (IpIsValid (bmcdestip)) {	/* use existing if valid */
	memcpy (rgdestip, bmcdestip, 4);
	if (MacIsValid (bmcdestmac))
	  memcpy (rgdestmac, bmcdestmac, MAC_LEN);
      }
      else if ((strncmp (alertname, "localhost", 9) == 0)) {	/* snmpd.conf = localhost (self) is the SNMP alert destination */
	if (IpIsValid (osmyip))
	  memcpy (rgdestip, osmyip, 4);
	if (!MacIsValid (rgdestmac)) {	/* if not user-specified */
	  // Get_Mac(rgdestip,rgdestmac,alertname);  (wont work for local)
	  memcpy (rgdestmac, osmymac, MAC_LEN);
	}
      }				/*endif local */
    }
    if (!MacIsValid (rgdestmac)) {	/* if MAC not vaild or user-specified */
      /* Use arping to get MAC from alertname or IP */
      Get_Mac (rgdestip, rgdestmac, alertname);
    }
  }				/*endif have alertname */

  return (rc);
}				/* end Get_IPMac_Addr */
#endif

int
ShowChanAcc (uchar bchan)
{
  LAN_RECORD LanRecord;
  int ret = 0;
  uchar access;
  char *pstr;
  uchar pb0, pb1;

  if (bchan == lan_ch)
    pstr = "lan";
  else if (bchan == ser_ch)
    pstr = "ser";
  else
    pstr = "?";
  ret = GetChanAcc (bchan, 0x40, &LanRecord);
  if (fdebug)
    printf ("  GetChanAcc(%d), ret = %d, data = %02x %02x\n",
	    bchan, ret, LanRecord.data[0], LanRecord.data[1]);
  pb0 = LanRecord.data[0];
  pb1 = LanRecord.data[1];
  if (fcanonical)
    printf ("Channel %d Access Mode %s%c ", bchan, pspace3, bdelim);
  else
    printf ("Channel(%d=%s) Access Mode: %02x %02x : ", bchan, pstr, pb0,
	    pb1);
  access = pb0;
  switch (access & 0x03) {
  case 0:
    printf ("Disabled, ");
    break;
  case 1:
    printf ("Pre-Boot, ");
    break;
  case 2:
    printf ("Always Avail, ");
    break;
  case 3:
    printf ("Shared, ");
    break;
  }
  if (access & 0x20)
    printf ("PEF Alerts Disabled\n");	/*0 */
  else
    printf ("PEF Alerts Enabled\n");	/*1 */
  return (ret);
}

static int
GetSessionInfo (uchar * rData, int sz)
{
  int rv, rlen;
  uchar ccode;
  uchar iData[5];

  iData[0] = 0x00;		/*get data for this session */
  rlen = sz;
  rv =
    ipmi_cmdraw (CMD_GET_SESSION_INFO, NETFN_APP, BMC_SA, PUBLIC_BUS, BMC_LUN,
		 iData, 1, rData, &rlen, &ccode, fdebug);
  if ((rv == 0) && (ccode != 0))
    rv = ccode;
  return (rv);
}

static int
GetPefCapabilities (uchar * bmax)
{
  int rv, rlen;
  uchar ccode;
  uchar rData[MAX_BUFFER_SIZE];

  rlen = sizeof (rData);
  rv = ipmi_cmdraw (0x10, NETFN_SEVT, BMC_SA, PUBLIC_BUS, BMC_LUN,
		    NULL, 0, rData, &rlen, &ccode, fdebug);
  if ((rv == 0) && (ccode != 0))
    rv = ccode;
  if ((rv == 0) && (bmax != NULL))
    *bmax = rData[2];		/*max num PEF table entries */
  return (rv);
}

int
GetSerialOverLan (uchar chan, uchar bset, uchar block)
{
  uchar requestData[24];
  uchar rData[MAX_BUFFER_SIZE];
  int rlen;
  int status, i;
  uchar ccode;
  uchar enable_parm, auth_parm, baud_parm;
  ushort getsolcmd;
  uchar user;

  if (fIPMI20 && fSOL20) {
    getsolcmd = GET_SOL_CONFIG2;
    enable_parm = SOL_ENABLE_PARAM;
    auth_parm = SOL_AUTHENTICATION_PARAM;
    baud_parm = SOL_BAUD_RATE_PARAM;
  }
  else {
    getsolcmd = GET_SOL_CONFIG;
    enable_parm = SOL_ENABLE_PARAM;
    auth_parm = SOL_AUTHENTICATION_PARAM;
    baud_parm = SOL_BAUD_RATE_PARAM;
    chan = 0;			/*override chan for IPMI 1.5 */
  }
  if (!fcanonical)
    printf ("%s, GetSOL for channel %d ...\n", progname, chan);

  requestData[0] = chan;	/*channel */
  requestData[1] = enable_parm;
  requestData[2] = bset;	/*set */
  requestData[3] = block;	/*block */
  rlen = sizeof (rData);
  status = ipmi_cmd (getsolcmd, requestData, 4, rData, &rlen, &ccode, fdebug);
  if (status != 0)
    return (status);
  if (ccode) {
    if (ccode == 0xC1) {	/* unsupported command */
      printf ("Serial-Over-Lan not available on this platform\n");
      return (status);
    }
    else {
      printf ("SOL Enable ccode = %x\n", ccode);
      status = ccode;
    }
  }
  else {			/*success */
    if (fcanonical) {
      printf ("Channel %d SOL Enable %s", chan, pspace3);
    }
    else {
      printf ("SOL Enable: ");
      for (i = 1; i < rlen; i++)
	printf ("%02x ", rData[i]);
    }
    if (rData[1] == 0x01)
      printf ("%c enabled\n", bdelim);
    else
      printf ("%c disabled\n", bdelim);
  }

  if (!fcanonical) {
    requestData[0] = chan;
    requestData[1] = auth_parm;
    requestData[2] = bset;	// selector
    requestData[3] = block;	// block
    rlen = sizeof (rData);
    status =
      ipmi_cmd (getsolcmd, requestData, 4, rData, &rlen, &ccode, fdebug);
    if (status != 0)
      return (status);
    if (ccode) {
      printf ("SOL Auth ccode = %x\n", ccode);
      status = ccode;
    }
    else {			/*success */
      printf ("SOL Auth: ");
      for (i = 1; i < rlen; i++)
	printf ("%02x ", rData[i]);
      printf (": ");
      show_priv (rData[1]);	/* priv level = User,Admin,... */
      printf ("\n");
    }

    requestData[0] = chan;
    requestData[1] = SOL_ACC_INTERVAL_PARAM;
    requestData[2] = bset;
    requestData[3] = block;
    rlen = sizeof (rData);
    status =
      ipmi_cmd (getsolcmd, requestData, 4, rData, &rlen, &ccode, fdebug);
    if (status != 0)
      return (status);
    if (ccode) {
      printf ("SOL Accum Interval ccode = %x\n", ccode);
      status = ccode;
    }
    else {			/*success */
      printf ("SOL Accum Interval: ");
      for (i = 1; i < rlen; i++)
	printf ("%02x ", rData[i]);
      printf (": %d msec\n", (rData[1] * 5));
    }

    requestData[0] = chan;
    requestData[1] = SOL_RETRY_PARAM;
    requestData[2] = bset;
    requestData[3] = block;
    rlen = sizeof (rData);
    status =
      ipmi_cmd (getsolcmd, requestData, 4, rData, &rlen, &ccode, fdebug);
    if (status != 0)
      return (status);
    if (ccode) {
      printf ("SOL Retry ccode = %x\n", ccode);
      status = ccode;
    }
    else {			/*success */
      printf ("SOL Retry Interval: ");
      for (i = 1; i < rlen; i++)
	printf ("%02x ", rData[i]);
      printf (": %d msec\n", (rData[2] * 10));
    }
  }

  if (!fRomley) {
    requestData[0] = chan;
    requestData[1] = baud_parm;
    requestData[2] = bset;
    requestData[3] = block;
    rlen = sizeof (rData);
    status =
      ipmi_cmd (getsolcmd, requestData, 4, rData, &rlen, &ccode, fdebug);
    if (status != 0)
      return (status);
    if (ccode) {
      printf ("SOL nvol Baud ccode = %x\n", ccode);
      status = ccode;
    }
    else {			/*success */
      uchar b;
      if (fcanonical) {
	printf ("Channel %d SOL Baud Rate%s", chan, pspace3);
      }
      else {
	printf ("SOL nvol Baud Rate: ");
	for (i = 1; i < rlen; i++)
	  printf ("%02x ", rData[i]);
      }
      /* if not user-specified and previously enabled, use existing */
      b = (rData[1] & 0x0f);
      if ((fnewbaud == 0) && BaudValid (b)) {
	sol_baud = b;
	sol_bvalid = 1;
      }
      printf ("%c %s\n", bdelim, Baud2Str (b));
    }

    if (!fcanonical) {
      requestData[0] = chan;
      requestData[1] = SOL_VOL_BAUD_RATE_PARAM;	/*0x06 */
      requestData[2] = bset;
      requestData[3] = block;
      rlen = sizeof (rData);
      status =
	ipmi_cmd (getsolcmd, requestData, 4, rData, &rlen, &ccode, fdebug);
      if (status != 0)
	return (status);
      if (ccode) {
	printf ("SOL vol Baud ccode = %x\n", ccode);
	status = ccode;
      }
      else {			/*success */
	printf ("SOL vol Baud Rate: ");
	for (i = 1; i < rlen; i++)
	  printf ("%02x ", rData[i]);
	printf ("%c %s\n", bdelim, Baud2Str (rData[1]));
      }
    }
  }
  if (fIPMI20) {
    if (vend_id != VENDOR_IBM) {
      /* IBM 0x00DC returns invalid cmd for SOL Payload commands. */
      if (!fcanonical) {
	requestData[0] = chan;
	rlen = sizeof (rData);
	status = ipmi_cmdraw (GET_PAYLOAD_SUPPORT, NETFN_APP,
			      BMC_SA, PUBLIC_BUS, BMC_LUN,
			      requestData, 1, rData, &rlen, &ccode, fdebug);
	if ((status != 0) || (ccode != 0)) {
	  printf ("SOL Payload Support error %d, ccode = %x\n", status,
		  ccode);
	  if (status == 0)
	    status = ccode;
	}
	else {			/*success */
	  printf ("SOL Payload Support(%d): ", chan);
	  for (i = 0; i < rlen; i++)
	    printf ("%02x ", rData[i]);
	  printf ("\n");
	}
      }				/*endif not canonical */
      /* get Payload Access for 4 users, not just lan_user */
      for (user = 1; user <= show_users; user++) {
	/* mBMC doesn't support more than 1 user */
	if (fmBMC && (user > 1))
	  break;
	/* IPMI 2.0 has >= 4 users */
	requestData[0] = chan;
	requestData[1] = user;
	rlen = sizeof (rData);
	status = ipmi_cmdraw (GET_PAYLOAD_ACCESS, NETFN_APP,
			      BMC_SA, PUBLIC_BUS, BMC_LUN,
			      requestData, 2, rData, &rlen, &ccode, fdebug);
	if ((status != 0) || (ccode != 0)) {
	  printf ("SOL Payload Access(%d,%d) error %d, ccode = %x\n",
		  chan, user, status, ccode);
	  if (status == 0)
	    status = ccode;
	}
	else {			/*success */
	  if (fcanonical) {
	    printf ("Channel %d SOL Payload Access(user%d)%s", chan, user,
		    pspace1);
	  }
	  else {
	    printf ("SOL Payload Access(%d,%d): ", chan, user);
	    for (i = 0; i < rlen; i++)
	      printf ("%02x ", rData[i]);
	  }
	  if ((rData[0] & 0x02) != 0)
	    printf ("%c enabled\n", bdelim);
	  else
	    printf ("%c disabled\n", bdelim);
	}
      }				/*end user loop */
    }				/*endif not IBM */
  }

  return (status);
}				/*end GetSerialOverLan */

/*
ECHO SOL Config Enable
CMDTOOL 20 30 21 %1 01 01
 
ECHO SOL Authentication (Administrator)
CMDTOOL 20 30 21 %1 02 04
 
ECHO SOL Accumlate Interval and threshold
CMDTOOL 20 30 21 %1 03 06 14
 
ECHO SOL Retry Interval and threshold
CMDTOOL 20 30 21 %1 04 06 14
 
ECHO SOL non-volatile baud rate
CMDTOOL 20 30 21 %1 05 07
 
ECHO SOL volatile baud rate
CMDTOOL 20 30 21 %1 06 07
 
ECHO Set user Payload Access for user 1
CMDTOOL 20 18 4c %1 01 02 00 00 00
 */
int
SetupSerialOverLan (int benable)
{
  uchar requestData[24];
  uchar responseData[MAX_BUFFER_SIZE];
  int responseLength = MAX_BUFFER_SIZE;
  int status;
  uchar completionCode;
  uchar enable_parm, auth_parm, baud_parm;
  ushort setsolcmd;
  ushort getsolcmd;
  uchar bchan, b;

  if (fIPMI20 && fSOL20) {
    setsolcmd = SET_SOL_CONFIG2;
    getsolcmd = GET_SOL_CONFIG2;
    enable_parm = SOL_ENABLE_PARAM;
    auth_parm = SOL_AUTHENTICATION_PARAM;
    baud_parm = SOL_BAUD_RATE_PARAM;
    bchan = lan_ch;
  }
  else {
    setsolcmd = SET_SOL_CONFIG;
    getsolcmd = GET_SOL_CONFIG;
    enable_parm = SOL_ENABLE_PARAM;
    auth_parm = SOL_AUTHENTICATION_PARAM;
    baud_parm = SOL_BAUD_RATE_PARAM;
    bchan = 0x00;		/*override chan for IPMI 1.5 */
  }
  memset (requestData, 0, sizeof (requestData));	/* zero-fill */
  requestData[0] = bchan;
  requestData[1] = enable_parm;
  if (benable == 0)
    requestData[2] = SOL_DISABLE_FLAG;
  else
    requestData[2] = SOL_ENABLE_FLAG;
  responseLength = MAX_BUFFER_SIZE;
  status = ipmi_cmd (setsolcmd, requestData, 3, responseData,
		     &responseLength, &completionCode, fdebug);
  if (status == ACCESS_OK) {
    switch (completionCode) {
    case 0x00:			/* success */
      break;
    case 0xC1:			/* unsupported command */
      SELprintf ("SetupSerialOverLan: SOL not available on this platform\n");
      return 0;
    default:			/* other error */
      SELprintf ("SetupSerialOverLan: SOL_ENABLE_PARAM ccode=%x\n",
		 completionCode);
      return -1;
      break;
    }
  }
  else {
    SELprintf ("SET_SOL_CONFIG, enable SOL failed\n");
    return -1;
  }
  if (benable == 0)
    return 0;

  requestData[0] = bchan;	/* channel */
  requestData[1] = auth_parm;
  requestData[2] = 0x00;	/* set selector */
  requestData[3] = 0x00;	/* block selector */
  responseLength = MAX_BUFFER_SIZE;
  status = ipmi_cmd (getsolcmd, requestData, 4, responseData,
		     &responseLength, &completionCode, fdebug);
  if (status == ACCESS_OK) {
    if (completionCode) {
      SELprintf ("SetupSerialOverLan: GET_SOL_AUTHENTICATION_PARAM code=%x\n",
		 completionCode);

      return -1;
    }
  }
  else {
    SELprintf ("SOL_CONFIG, get SOL authentication failed\n");
    return -1;
  }

  if ((vend_id == VENDOR_SUPERMICROX) || (vend_id == VENDOR_SUPERMICRO))
    b = SOL_PRIVILEGE_LEVEL_OPERATOR;
  else
    b = SOL_PRIVILEGE_LEVEL_USER;
  requestData[0] = bchan;
  requestData[1] = auth_parm;
  requestData[2] = b | (responseData[1] & 0x80);	/* priv | enable */
  responseLength = MAX_BUFFER_SIZE;
  status = ipmi_cmd (setsolcmd, requestData, 3, responseData,
		     &responseLength, &completionCode, fdebug);
  if (status == ACCESS_OK) {
    if (completionCode) {
      SELprintf ("SET_SOL_AUTHENTICATION_PARAM code=%x\n", completionCode);

      return -1;
    }
  }
  else {
    SELprintf ("SET_SOL_CONFIG, set SOL authentication failed\n");
    return -1;
  }

  requestData[0] = bchan;
  requestData[1] = SOL_ACC_INTERVAL_PARAM;
  requestData[2] = sol_accum[0];	//0x04;
  requestData[3] = sol_accum[1];	//0x32;
  responseLength = MAX_BUFFER_SIZE;
  if (fdebug)
    SELprintf ("Setting SOL AccumInterval\n");
  status = ipmi_cmd (setsolcmd, requestData, 4, responseData,
		     &responseLength, &completionCode, fdebug);
  if (status != ACCESS_OK || completionCode) {
    SELprintf ("SET SOL AccumInterval ret=%d ccode=%x\n",
	       status, completionCode);
    return -1;
  }

  /* Some BMCs return sporadic errors for SOL params (e.g. Kontron) */
  // if (vend_id == VENDOR_KONTRON) ; 
  // else
  {
    requestData[0] = bchan;
    requestData[1] = SOL_RETRY_PARAM;
    requestData[2] = sol_retry[0];	//0x06;
    requestData[3] = sol_retry[1];	//0x14;
    responseLength = MAX_BUFFER_SIZE;
    if (fdebug)
      SELprintf ("Setting SOL RetryInterval\n");
    status = ipmi_cmd (setsolcmd, requestData, 4, responseData,
		       &responseLength, &completionCode, fdebug);
    if (status != ACCESS_OK || completionCode) {
      SELprintf ("SET SOL RetryInterval ret=%d ccode=%x\n",
		 status, completionCode);
      return -1;
    }
  }

  if (fRomley);			/* skip SOL BAUD */
  else {			/* else SOL BAUD is used, so set it. */
    if (fnewbaud == 0) {	/* no user-specified SOL baud */
      /* if sol_bvalid, sol_baud was set to existing value above */
      if (!sol_bvalid) {
	status = GetSerEntry (7, (LAN_RECORD *) & responseData);
	if (status == 0) {	/* use Serial baud for SOL */
	  sol_baud = responseData[1];
	  if (fdebug)
	    SELprintf ("Serial Baud is %s\n", Baud2Str (sol_baud));
	}
      }
    }
    requestData[0] = bchan;
    requestData[1] = baud_parm;
    requestData[2] = sol_baud;
    responseLength = MAX_BUFFER_SIZE;
    if (fdebug)
      SELprintf ("Setting SOL BAUD to %s\n", Baud2Str (sol_baud));
    status = ipmi_cmd (setsolcmd, requestData, 3, responseData,
		       &responseLength, &completionCode, fdebug);
    if (status != ACCESS_OK || completionCode) {
      SELprintf ("SET SOL BAUD ret=%d ccode=%x\n", status, completionCode);
      return -1;
    }

    requestData[0] = bchan;
    requestData[1] = SOL_VOL_BAUD_RATE_PARAM;
    requestData[2] = sol_baud;
    responseLength = MAX_BUFFER_SIZE;
    if (fdebug)
      printf ("Setting SOL vol BAUD to %s\n", Baud2Str (sol_baud));
    status = ipmi_cmd (setsolcmd, requestData, 3, responseData,
		       &responseLength, &completionCode, fdebug);
    if (status != ACCESS_OK || completionCode) {
      printf ("SET SOL vol BAUD ret=%d ccode=%x\n", status, completionCode);
      return -1;
    }
  }

  if (fIPMI20 && fSOL20) {
    if (vend_id == VENDOR_KONTRON && lan_user == 1) {
      if (fdebug)
	SELprintf ("Skipping SOL Payload Access for user %d\n", lan_user);
    }
    else if (vend_id == VENDOR_IBM) {	/*non-conformance */
      if (fdebug)
	SELprintf ("Skipping SOL Payload Access for user %d\n", lan_user);
    }
    else {
      if (fdebug)
	SELprintf ("Setting SOL Payload Access for user %d\n", lan_user);
      requestData[0] = bchan;
      requestData[1] = lan_user;	/*enable this user */
      requestData[2] = 0x02;	/*enable std 2.0 SOL */
      requestData[3] = 0;
      requestData[4] = 0;
      requestData[5] = 0;
      responseLength = MAX_BUFFER_SIZE;
      status = ipmi_cmdraw (SET_PAYLOAD_ACCESS, NETFN_APP,
			    BMC_SA, PUBLIC_BUS, BMC_LUN,
			    requestData, 6, responseData, &responseLength,
			    &completionCode, fdebug);
      if (status != ACCESS_OK || completionCode) {
	SELprintf ("SET SOL Payload Access ret=%d ccode=%x\n",
		   status, completionCode);
	return -1;
      }
    }
  }
  return 0;
}				/*end SetupSerialOverLan */

static char *
PefDesc (int idx, uchar stype)
{
  char *pdesc, *p;
  static char mystr[60];
  int mylen = sizeof (mystr);
  pdesc = &mystr[0];
  if (pefdesc != NULL)
    strcpy (pdesc, pefdesc[idx]);	/* if Intel, pre-defined */
  else
    strcpy (pdesc, "reserved");	/* else set default to detect */
  if ((stype != 0) && (strcmp (pdesc, "reserved") == 0)) {
    /* Dynamically set the pef desc string from the sensor type */
    switch (stype) {
    case 0x01:
      strcpy (pdesc, "Temperature");
      break;
    case 0x02:
      strcpy (pdesc, "Voltage");
      break;
    case 0x04:
      strcpy (pdesc, "Fan");
      break;
    case 0x05:
      strcpy (pdesc, "Chassis");
      break;
    case 0x07:
      strcpy (pdesc, "BIOS");
      break;
    case 0x08:
      strcpy (pdesc, "Power Supply");
      break;
    case 0x09:
      strcpy (pdesc, "Power Unit");
      break;
    case 0x0c:
      strcpy (pdesc, "Memory");
      break;
    case 0x0f:
      strcpy (pdesc, "Boot");
      break;
    case 0x12:
      strcpy (pdesc, "System Restart");
      break;
    case 0x13:
      strcpy (pdesc, "NMI");
      break;
    case 0x23:
      strcpy (pdesc, "Watchdog");
      break;
    case 0x20:
      strcpy (pdesc, "OS Critical Stop");
      break;
    default:
#ifdef METACOMMAND
      p = get_sensor_type_desc (stype);
      if (p != NULL) {
	strncpy (pdesc, p, mylen);
	mystr[mylen - 1] = 0;	/*stringify */
      }
#else
      sprintf (pdesc, "Other[%02x]", stype);
#endif
      break;
    }
    if (pef_array[idx - 1][4] == PEF_SEV_OK)
      strcat (pdesc, " OK");
  }
  return (pdesc);
}


#ifdef METACOMMAND
int i_lan(int argc, char **argv)
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
  PEF_RECORD PefRecord;
  LAN_RECORD LanRecord;
  int i, idx, j;
  int c;
  char *pstr;
  uchar bset;
  int ndest = 4;
  int idest;
  char mystr[80];
  char fpefok = 1;
  uchar *pc;
  int sz;
  char *pa;
  char *pb;

  // progname = argv[0];
  printf ("%s ver %s \n", progname, progver);
  j = 0;
  freadonly = FLAG_INIT;
  idx = argc;			/*getopt loop counter */
  /* available opt chars: y O Q + = ~ _ */
  while ((c =
	  getopt (argc, argv,
		  "a:b:cdef:gh:i:j:klm:n:op:q:rstu:v:w:xy:z#::A:B:C:DEF:G:H:I:J:K:L:M:N:OP:Q:R:S:T:U:V:X:YZ:?"))
	 != EOF) 
  {
    switch (c) {
    case 'a':			/* alert dest number (usu 1 thru 4) */
      alertnum = atob (optarg);
      if (alertnum > alertmax)
	alertnum = 1;
      j++;
      break;
    case 'b':
      bAuth = htoi (optarg);
      j++;
      break;			/*undocumented */
    case 'c':
      fcanonical = 1;
      bdelim = BDELIM;
      break;
    case 'd':
      fenable = 0;
      fdisable = 1;
      freadonly = 0;
      break;
    case 'e':
      fenable = 1;
      fdisable = 0;
      freadonly = 0;
      break;
    case 'f':
      i = atoi (optarg);	/*set arp_ctl */
      if (i < 0 || i > 3) printf ("Invalid ARP control %d\n", i);
      else {
        arp_ctl = i;
        fsetarp = 1;
        j++;
      }
      break;
    case 'l':
      fpefenable = 0;
      fenable = 2;
      fdisable = 0;
      freadonly = 0;
      break;
    case 'h':			/* VLAN ID */
      i = atoi (optarg);
      if (i > 4095)
        vlan_enable = 0;
      else {
        vlan_enable = 1;
        vlan_id = (ushort) i;
      }
      j++;
      break;
    case 'y':			/* OEM LAN Failover enable/disable */
      i = atoi (optarg);
      if (i < 0) printf ("Failover(-y) parameter is negative\n");
      else failover_enable = i;
      j++;
      break;
    case 'Q':			/* VLAN Priority */
      i = atoi (optarg);
      if (i > 7 || i < 0) vlan_enable = 0;
      else {
        vlan_enable = 1;
        vlan_prio = (uchar) i;
      }
      j++;
      break;
    case 'i':			/* eth interface (ifname) */
      fsetifn = 1;
      i = sizeof (ifname);
      if (strlen (optarg) > (uint) i) optarg[i] = 0;
      strcpy (ifname, optarg);
      if (fdebug) printf ("ifname = %s\n", ifname);
      j++;
      break;
    case 'j':
      fCustomPEF = 1;		/*custom 10 PEF bytes */
      fpefenable = 1;		/* PEF is implied here */
      freadonly = 0;
      memset (custPEF, 0, sizeof (custPEF));
      custPEF[0] = htoi (&optarg[0]);	/*action  */
      custPEF[1] = htoi (&optarg[2]);	/*policy  */
      custPEF[2] = htoi (&optarg[4]);	/*severity */
      custPEF[3] = htoi (&optarg[6]);	/*genid1  */
      custPEF[4] = htoi (&optarg[8]);	/*genid2  */
      custPEF[5] = htoi (&optarg[10]);	/*sensor_type */
      custPEF[6] = htoi (&optarg[12]);	/*sensor_num */
      custPEF[7] = htoi (&optarg[14]);	/*evt_trigger */
      custPEF[8] = htoi (&optarg[16]);	/*data1offset */
      custPEF[9] = htoi (&optarg[18]);	/*data1mask  */
      if (optarg[20] != 0) {
        /* optionally get 8 extra PEF entry bytes */
        custPEF[10] = htoi (&optarg[20]);	/*data1cmp1 */
        custPEF[11] = htoi (&optarg[22]);	/*data1cmp2 */
        custPEF[12] = htoi (&optarg[24]);	/*data2mask */
        custPEF[13] = htoi (&optarg[26]);	/*data2cmp1 */
        custPEF[14] = htoi (&optarg[28]);	/*data2cmp2 */
        custPEF[15] = htoi (&optarg[30]);	/*data3mask */
        custPEF[16] = htoi (&optarg[32]);	/*data3cmp1 */
        custPEF[17] = htoi (&optarg[34]);	/*data3cmp2 */
      }
      j++;
      break;
    case 'k':
      fSetPEFOks = 1;
      j++;
      break;			/*configure PEF OK rules */
    case 'm':
      set_max_kcs_loops (atoi (optarg));
      break;
    case 'n':			/* number/index in PEF table to insert new entry */
      fpefenable = 1;
      pefnum = atob (optarg);
      if (pefnum >= MAXPEF) {
        pefnum = MAXPEF - 1;
        fAdjustPefNum = 1;
      }
      else
        fUserPefNum = 1;
      j++;
      break;
    case 'o':
      fdisableSOL = 1;		/*disable SOL only */
      fpefenable = 0;		/*no change to PEF */
      freadonly = 0;
      break;
    case 'r':
      freadonly = 1;
      fenable = 0;
      break;
    case 's':
      fgetser = 1;
      break;
    case 't':
      ftestonly = 1;
      freadonly = 1;
      break;
    case 'v':			/* user access privilege level */
      i = atoi (optarg);
      if (valid_priv (i)) lan_access = i & 0x0f;
      else printf ("Invalid privilege -v %d, using Admin\n", i);
      j++;
      break;
    case 'w':
      i = atoi (optarg);	/*set grat arp interval, in #sec */
      if (i >= 0 && i < 256) arp_interval = i * 2;
      else printf ("Invalid arp interval -w %d, skipping\n", i);
      break;
    case 'x':
      fdebug = 1;
      break;
    case 'z':
      flanstats = 1;
      break;
    case 'D':
      lan_dhcp = 1;
      j++;
      break;
    case 'O':
      flansecure = 1;
      j++;
      break;
    case 'I':			/* My BMC IP Address */
      fset_ip |= MYIP;
      atoip (rgmyip, optarg);
      j++;
      break;
    case 'M':			/* My BMC MAC Address */
      atomac (rgmymac, optarg);
      if (!MacIsValid (rgmymac)) printf ("Invalid MAC for -M\n");
      j++;
      break;
    case 'S':			/* Subnet IP Address */
      atoip (rgsubnet, optarg);
      j++;
      break;
    case 'G':			/* Gateway IP Address */
      fset_ip |= GWYIP;
      atoip (rggwyip, optarg);
      j++;
      break;
    case 'g':			/* Secondary Gateway IP Address */
      fset_ip |= GWYIP;
      atoip (rggwy2ip, optarg);
      j++;
      break;
    case 'H':			/* Gateway MAC Address */
      atomac (rggwymac, optarg);
      if (!MacIsValid (rggwymac)) printf ("Invalid MAC for -H\n");
      j++;
      break;
    case 'B':			/* SOL Baud rate */
      fnewbaud = 1;
      sol_baud = Str2Baud (optarg);
      j++;
      break;
    case 'A':			/* Alert Dest IP Address */
      fset_ip |= DESTIP;
      /* allow name or ip here via Get_Mac() ? */
      atoip (rgdestip, optarg);
      fpefenable = 1;		/* PEF is implied here */
      j++;
      break;
    case 'X':			/* Alert Dest MAC Address */
      atomac (rgdestmac, optarg);
      if (!MacIsValid (rgdestmac)) printf ("Invalid MAC for -X\n");
      fpefenable = 1;		/* PEF is implied here */
      j++;
      break;
    case 'K':			/* Kontron IPMI hostname */
      i = sizeof (rghostname);	/*usu 18 */
      if (strlen (optarg) > (uint) i)
	optarg[i] = 0;
      strcpy (rghostname, optarg);
      j++;
      break;
    case 'C':			/* Community String */
      fsetcommunity = 1;
      i = sizeof (rgcommunity);	/*usu 18 */
      if (strlen (optarg) > (uint) i) optarg[i] = 0;
      strcpy (rgcommunity, optarg);
      fpefenable = 1;		/* PEF is implied here */
      j++;
      break;
    case 'u':			/* username to set */
      myuser = strdup_ (optarg);	/*remote username */
      j++;
      break;
    case 'p':			/* password to set */
      fpassword = 1;
      if (strlen (optarg) > PSW_MAX)
	optarg[PSW_MAX] = 0;
      strcpy (passwordData, optarg);
      if (fdebug) printf ("Password = %s\n", passwordData);
      /* Hide password from 'ps' */
      memset (optarg, ' ', strlen (optarg));
      j++;
      break;
    case 'q':
    case '#':
      usernum = atob (optarg);
      if (usernum > 15) usernum = 0;		/*MAX_IPMI_USERS = 15 */
      j++;
      break;
    case 'L':
      if (strcmp (optarg, "list") == 0)
      fshowchan = 1;
      lan_ch_parm = atob (optarg);
      if (lan_ch_parm > MAXCHAN)
      lan_ch_parm = PARM_INIT;	/*invalid */
      break;
    case 'V':			/* priv level */
      fprivset = 1;
    case 'N':			/* nodename */
    case 'U':			/* remote username */
    case 'P':			/* remote password */
    case 'R':			/* remote password */
    case 'E':			/* get password from IPMI_PASSWORD environment var */
    case 'F':			/* force driver type */
    case 'T':			/* auth type */
    case 'J':			/* cipher suite */
    case 'Y':			/* prompt for remote password */
    case 'Z':			/* set local MC address */
      parse_lan_options (c, optarg, fdebug);
      break;
    default:
      printf ("Usage: %s [-abcdefghijklmnopq#rstuvwxyzBDQK]\n", progname);
      printf ("         \t [-a alertnum -i eth1 -n pefnum ]\n");
      printf ("         \t [-I ipadr -M macadr -S subnet ]\n");
      printf ("         \t [-G gwyip -H gwymac -L lan_channel_num]\n");
      printf ("         \t [-A alertip -X alertmac -C community ]\n");
      printf ("         \t [-g 2nd_gwyip  -v priv  -B sol_baud ]\n");
      printf ("         \t [-j 10_bytes_custom_pef -b authmask ]\n");
      printf ("where -c  shows Canonical, simpler output format\n");
      printf ("      -d  Disables BMC LAN & PEF\n");
      printf ("      -e  Enables BMC LAN & PEF\n");
      printf ("      -f  set ARP Control to 1=grat, 2=resp, 3=both\n");
      printf ("      -g  secondary Gateway IP (-G=primary_gwy_ip)\n");
      printf ("      -h  VLAN ID (>=4096 to disable)\n");
      printf ("      -j  specify custom PEF rule (10 or 18 hex bytes)\n");
      printf ("      -k  add PEF oK rules, if PEF enable\n");
      printf ("      -l  Enables BMC LAN only, not PEF\n");
      printf ("      -o  disable Only SOL\n");
      printf ("      -p  password to set \n");
      printf ("   -q/-#  User number of LAN username_to_set\n");
      printf ("      -r  Read-only BMC LAN & PEF settings\n");
      printf ("      -s  Show some Serial settings also \n");
      printf ("      -t  Test if BMC LAN is already configured\n");
      printf ("      -u  username to set \n");
      printf ("      -v  access priVilege: 4=Admin,3=Operator,2=User\n");
      printf ("      -w  set Grat ARP Interval to specified # seconds\n");
      printf ("      -x  Show eXtra debug messages\n");
      printf ("      -y  OEM LAN Failover (1=enable,0=disable if Intel)\n");
      printf ("      -z  Show IPMI LAN statistics\n");
      printf ("      -B  Baud for SerialOverLan (19.2K,115.2K,...)\n");
      printf ("      -D  Use DHCP instead of static IP (-I for server)\n");
      printf ("      -K  (Kontron) IPMI hostname to set\n");
      printf ("      -Q  VLAN Priority (default =0)\n");
      printf ("      -O  Force LAN security: no null user, cipher 0 off\n");
      print_lan_opt_usage (0);
      ret = ERR_USAGE;
      goto do_exit;
    }				/*end switch */
    nopts++;
  }				/*end while */

  if ((freadonly == FLAG_INIT) && (j > 0)) {
    /* got some options implying set, but no -e -l -d option specified. */
    foptmsg = 1;		/*show warning message later */
    freadonly = 1;		/*assume read only */
  }
  fipmilan = is_remote ();
  if (fipmilan && !fprivset)
    parse_lan_options ('V', "4", 0);	/*even if freadonly request admin */
  if ((fsetarp == 0) && ostype == OS_WINDOWS)
    arp_ctl = 0x03;		/*grat arp & arp resp enabled */

  ret = ipmi_getdeviceid((uchar *)&LanRecord,16,fdebug);
  if (ret != 0) {
    goto do_exit;
  }
  else {			/* success */
    uchar ipmi_maj, ipmi_min;
    ipmi_maj = LanRecord.data[4] & 0x0f;
    ipmi_min = LanRecord.data[4] >> 4;
    show_devid (LanRecord.data[2], LanRecord.data[3], ipmi_maj, ipmi_min);
    if (ipmi_maj == 0) fIPMI10 = 1; /* IPMI 1.0 is limited */
    else if (ipmi_maj == 1 && ipmi_min < 5) fIPMI10 = 1;
    else fIPMI10 = 0;		/* >= IPMI 1.5 is ok */
    if (ipmi_maj >= 2) fIPMI20 = 1; /* IPMI 2.0 has more */
    if (fIPMI20) show_users = 5;
    else show_users = 3;
    if (fIPMI10) {
      printf ("This IPMI v%d.%d system does not support PEF records.\n",
	      ipmi_maj, ipmi_min);
      /* Wont handle PEF, but continue and look for BMC LAN anyway */
    }
    prod_id = LanRecord.data[9] + (LanRecord.data[10] << 8);
    vend_id = LanRecord.data[6] + (LanRecord.data[7] << 8)
              + (LanRecord.data[8] << 16);
    /* check Device ID response for Manufacturer ID = 0x0322 (NSC) */
    if (vend_id == VENDOR_NSC) {	/* NSC = 0x000322 */
      fmBMC = 1;		/*NSC miniBMC */
      if (pefnum == 12)
	pefnum = 10;		/* change CritStop pefnum to 0x0a */
      pefdesc = &pefdesc2[0];	/*mini-BMC PEF */
      pefmax = 30;
      fsharedMAC = 1;		/* shared MAC with OS */
    }
    else if (vend_id == VENDOR_LMC) {	/* LMC (on SuperMicro) = 0x000878 */
      pefdesc = NULL;		/* unknown, see PefDesc() */
      if (pefnum == 12)
	pefnum = 15;		/* change CritStop pefnum */
      pefmax = 16;
      fsharedMAC = 0;		/* not-shared BMC LAN port */
    }
    else if (vend_id == VENDOR_INTEL) {	/* Intel = 0x000157 */
      pefdesc = &pefdesc1[0];	/*default Intel PEF */
      pefmax = 20;		/*default Intel PEF */
      switch (prod_id) {
      case 0x4311:		/* Intel NSI2U w SE7520JR23 */
        fmBMC = 1;		/* Intel miniBMC */
        if (pefnum == 12) pefnum = 14;		/* change CritStop pefnum */
        pefdesc = &pefdesc2[0];	/*mini-BMC PEF */
        pefmax = 30;
        fsharedMAC = 1;		/* shared-MAC BMC LAN port, same MAC */
        break;
      case 0x0022:		/* Intel TIGI2U w SE7520JR23 +IMM */
        fsharedMAC = 1;		/* shared-MAC BMC LAN port, same MAC */
        gcm_ch = 3;		/* IMM GCM port, dedicated MAC */
        show_users = 4;
        break;
      case 0x000C:		/*TSRLT2 */
      case 0x001B:		/*TIGPR2U */
        /* fmBMC=0; Intel Sahalee BMC */
        fsharedMAC = 1;		/* shared-MAC BMC LAN port, same MAC */
        break;
      case 0x0026:		/*S5000 Bridgeport */
      case 0x0028:		/*S5000PAL Alcolu */
      case 0x0029:		/*S5000PSL StarLake */
      case 0x0811:		/*S5000PHB TIGW1U */
        /* fmBMC=0;   Intel Sahalee ESB2 BMC */
        fsharedMAC = 0;		/* not-shared BMC LAN port, separate MAC */
        gcm_ch = 3;
        parm7 = &iparm7[0];	/*TTL=30 */
        break;
      case 0x003E:		/*NSN2U or CG2100 Urbanna */
        fiBMC = 1;		/* Intel iBMC */
        fsharedMAC = 0;		/* not-shared BMC LAN port, separate MAC */
        // gcm_ch = 3;
        parm7 = &iparm7[0];	/*TTL=30 */
        if (fsetarp == 0) arp_ctl = 0x02; /*grat arp disabled,arp resp enabled*/
        arp_interval = 0x00;	/*0 sec, since grat arp disabled */
        sol_accum[0] = 0x0c;	/*Intel defaults */
        sol_accum[1] = 0x60;	/*Intel defaults */
        sol_retry[0] = 0x07;	/*Intel defaults */
        sol_retry[1] = 0x32;	/*Intel defaults */
        set_max_kcs_loops (URNLOOPS); /*longer for SetLan cmds (default 300)*/
        break;
      case 0x0107:		/* Intel Caneland */
        fsharedMAC = 0;		/* not-shared BMC LAN port, separate MAC */
        gcm_ch = 3;
        break;
      case 0x0100:		/*Tiger2 ia64 */
        /* for ia64 set chan_pefon, chan_pefoff accordingly */
        chan_pefon = CHAN_ACC_PEFON64;
        chan_pefoff = CHAN_ACC_PEFOFF64;
        /* fall through */
      default:			/* else other Intel */
        /* fmBMC = 0;  * Intel Sahalee BMC */
        if (fIPMI20) fsharedMAC = 0;	/* recent, not-shared BMC MAC */
        else fsharedMAC = 1;	/* usu IPMI 1.x has shared BMC MAC */
        break;
      }				/*end switch */
      if (is_romley(vend_id, prod_id)) fRomley = 1;
      if (is_grantley(vend_id, prod_id)) fGrantley = 1;
      if (fRomley) {
        fiBMC = 1;		/* Intel iBMC */
        fsharedMAC = 0;		/* not-shared BMC LAN port, separate MAC */
        set_max_kcs_loops (URNLOOPS);	/*longer for SetLan (default 300) */
        fipv6 = 1;
        if (fsetarp == 0) arp_ctl = 0x03; /*default to both for Romley */
      }
    }
    else {			/* else other vendors  */
      if (fIPMI20) fsharedMAC = 0;	/* recent, not-shared BMC MAC */
      else fsharedMAC = 1;		/* usu IPMI 1.x has shared BMC MAC */
      pefdesc = NULL;		/* unknown, see PefDesc() */
      if (pefnum == 12) pefnum = 15;	/* change CritStop pefnum to 15? */
      pefmax = 20;
      if (!fUserPefNum) fAdjustPefNum = 1;
    }
    if (fmBMC) show_users = 1;		/* mBMC doesn't support more than 1 user */
  }

  if (fshowchan) {
    ret = show_channels ();
    exit (ret);
  }

  ret = GetPefCapabilities (&bset);
  if ((ret == 0) && (bset <= MAXPEF))
    pefmax = bset;

  /* Get the BMC LAN channel & match it to an OS eth if. */
  i = GetBmcEthDevice (lan_ch_parm, &lan_ch);
  if (i == -2) {		/* no lan channels found (see lan_ch) */
    if (lan_ch_parm == PARM_INIT)
      printf ("This system does not support IPMI LAN channels.\n");
    else			/*specified a LAN channel */
      printf ("BMC channel %d does not support IPMI LAN.\n", lan_ch_parm);
    ret = LAN_ERR_NOTSUPPORT;
    goto do_exit;
  }
  else if (i < 0) {		/* mac not found, use platform defaults */
    i = 0;			/* default to eth0, lan_ch set already. */
    if (vend_id == VENDOR_INTEL) {
      if ((prod_id == 0x001B) || (prod_id == 0x000c)) {
        /* Intel TIGPR2U or TSRLT2 defaults are special */
        if (lan_ch_parm == 6) {
          i = 0;
          lan_ch = 6;
        } else {
          i = 1;
          lan_ch = 7;
        }
        ser_ch = 1;
      }
    }
  }
  if ((i == gcm_ch) && (gcm_ch != PARM_INIT) && (lan_ch_parm == PARM_INIT)) {
    /* Has a GCM, defaulted to it, and user didn't specify -L */
    /* Need this to avoid picking channel 3, the IMM/RMM GCM channel. */
    lan_ch = 1;			/*default BMC LAN channel */
    // i = 0;       /*default eth0 (was eth1) */
  }
  if (fsetifn == 0) {		/*not user specified, use the detected one */
    // if (lan_ch == gcm_ch) strcpy(ifname,"gcm");
    sprintf (ifname, "%s%d", ifpattn, i);	/*eth%d */
  }
  if (fdebug)
    printf ("lan_ch = %d, ifname = %s\n", lan_ch, ifname);

  /* set the lan_user appropriately */
  if (myuser == NULL) {		/* if no -u param */
    if (ipmi_reserved_user (vend_id, 1))
      lan_user = 2;
    else if (flansecure)
      lan_user = 2;
    else
      lan_user = 1;		/*use default null user */
  }
  else if (usernum != 0)
    lan_user = usernum;		/*use -q specified usernum */
  /* else use default lan_user (=2) if -u and not -q */

  if (ftestonly) {		/*test only if BMC LAN is configured or not */
    /* TODO: test gcm also, if present */
    ret = GetLanEntry (4, 0, &LanRecord);	/*ip addr src */
    if (ret == 0) {
      if ((LanRecord.data[0] == SRC_BIOS) || (LanRecord.data[0] == SRC_DHCP))
	ret = 0;		/* DHCP, so ok */
      else {			/*static IP */
	ret = GetLanEntry (3, 0, &LanRecord);	/* ip address */
	if (ret == 0) {
	  if (!IpIsValid (LanRecord.data)) {
	    printf ("invalid BMC IP address\n");
	    ret = 1;		/* invalid ip */
	  }
	  else
	    ret = GetLanEntry (12, 0, &LanRecord);	/*gateway ip */
	  if (ret == 0) {
	    if (!IpIsValid (LanRecord.data)) {
	      printf ("invalid gateway ip\n");
	      ret = 2;		/*invalid gwy ip */
	    }
	    else
	      ret = GetLanEntry (13, 0, &LanRecord);
	    if (ret == 0) {
	      if (!MacIsValid (&LanRecord.data[0])) {
		printf ("invalid gateway mac\n");
		ret = 3;	/*invalid gwy mac */
	      }
	    }
	  }
	}
      }
    }				/*endif GetLanEntry ok */
    if (ret == 0)
      printf ("BMC LAN already configured\n");
    else
      printf ("BMC LAN not configured\n");
    goto do_exit;
  }				/*endif ftestonly */

  memset (SessInfo, 0, sizeof (SessInfo));
  ret = GetSessionInfo (SessInfo, sizeof (SessInfo));
  // rlen = sizeof(SessInfo)); ret = get_session_info(0,0,SessInfo,&rlen); 
  if (fdebug)
    printf ("GetSessionInfo ret=%d, data: %02x %02x %02x %02x \n",
	    ret, SessInfo[0], SessInfo[1], SessInfo[2], SessInfo[3]);
  if (!freadonly && fipmilan) {	/* setting LAN params, and using IPMI LAN */
    if (SessInfo[2] > 1) {	/* another session is active also */
      printf
	("Another session is also active, cannot change IPMI LAN settings now.\n");
      ret = ERR_NOT_ALLOWED;
      goto do_exit;
    }
  }

  if (!fIPMI10) {
    if (fcanonical) {		/* canonical/simple output */
      ret = GetPefEntry (0x01, 0, (PEF_RECORD *) & LanRecord);
      if (ret != 0)
	ndest = 0;
      else {			/*success */
	j = LanRecord.data[0];
	mystr[0] = 0;
	if (j == 0)
	  strcat (mystr, "none ");
	else {
	  if (j & 0x01)
	    strcat (mystr, "PEFenable ");
	  if (j & 0x02)
	    strcat (mystr, "DoEventMsgs ");
	  if (j & 0x04)
	    strcat (mystr, "Delay ");
	  if (j & 0x08)
	    strcat (mystr, "AlertDelay ");
	}
	printf ("PEF Control %s%c %s\n", pspace4, bdelim, mystr);
      }
    }
    else {			/* normal/full output */
      ret = GetPefEntry (0x01, 0, (PEF_RECORD *) & LanRecord);
      if (ret == 0 && (LanRecord.data[0] != 0)) {
	fpefok = 1;
	bmcpefctl = LanRecord.data[0];
      }
      else {			/* skip PEF rules/params if disabled */
	printf ("PEF Control %s%c %s\n", pspace4, bdelim, "none ");
	ndest = 0;
	fpefok = 0;
      }

      if (fpefok) {
	printf ("%s, GetPefEntry ...\n", progname);
	for (idx = 1; idx <= pefmax; idx++) {
	  ret = GetPefEntry (0x06, (ushort) idx, &PefRecord);
	  if (ret == 0) {	// Show the PEF record
	    pc = (uchar *) & PefRecord;
	    sz = 21;		// sizeof(PEF_RECORD) = 21
	    if (PefRecord.sensor_type == 0) {
	      if (idx <= pefnum)
		printf ("PEFilter(%02d): empty\n", idx);
	      memcpy (pef_array[idx - 1], &PefRecord, sz);
	      if (fAdjustPefNum)
		pefnum = (char) idx;
	    }
	    else {
	      memcpy (pef_array[idx - 1], &PefRecord, sz);
	      if (PefRecord.fconfig & 0x80)
		pb = "enabled";
	      else
		pb = "disabled";
	      i = PefRecord.rec_id;
	      switch (PefRecord.action) {
	      case 0x01:
		pa = "alert";
		break;
	      case 0x02:
		pa = "poweroff";
		break;
	      case 0x04:
		pa = "reset";
		break;
	      case 0x08:
		pa = "powercycle";
		break;
	      case 0x10:
		pa = "OEMaction";
		break;
	      case 0x20:
		pa = "NMI";
		break;
	      default:
		pa = "no action";
	      }
	      printf ("PEFilter(%02d): %02x %s event - %s for %s\n",
		      idx, PefRecord.sensor_type,
		      PefDesc (i, PefRecord.sensor_type), pb, pa);
	    }
	    if (fdebug) {	/* show raw PEFilter record */
	      pc = &PefRecord.rec_id;
	      printf ("raw PEF(%.2d):  ", pc[0]);
	      for (i = 0; i < sz; i++)
		printf ("%02x ", pc[i]);
	      printf ("\n");
	    }
	  }
	  else {
	    printf ("GetPefEntry(%d), ret = %d\n", idx, ret);
	    if (ret == 0xC1) {	/*PEF is not supported, so skip the rest. */
	      fpefok = 0;
	      ndest = 0;	/* if no PEF, no alerts & no alert dest */
	      break;
	    }
	  }
	}
      }				/*endif fpefok */
      if (fpefok) {
	if (fdebug)
	  ShowPef ();
	ret = GetPefEntry (0x01, 0, (PEF_RECORD *) & LanRecord);
	if (ret == 0) {
	  j = LanRecord.data[0];
	  mystr[0] = 0;
	  if (j & 0x01)
	    strcat (mystr, "PEFenable ");
	  if (j & 0x02)
	    strcat (mystr, "DoEventMsgs ");
	  if (j & 0x04)
	    strcat (mystr, "Delay ");
	  if (j & 0x08)
	    strcat (mystr, "AlertDelay ");
	  printf ("PEF Control: %02x : %s\n", j, mystr);
	}
	ret = GetPefEntry (0x02, 0, (PEF_RECORD *) & LanRecord);
	if (ret == 0) {
	  j = LanRecord.data[0];
	  mystr[0] = 0;
	  if (j & 0x01)
	    strcat (mystr, "Alert ");
	  if (j & 0x02)
	    strcat (mystr, "PwrDn ");
	  if (j & 0x04)
	    strcat (mystr, "Reset ");
	  if (j & 0x08)
	    strcat (mystr, "PwrCyc ");
	  if (j & 0x10)
	    strcat (mystr, "OEM ");
	  if (j & 0x20)
	    strcat (mystr, "DiagInt ");
	  printf ("PEF Actions: %02x : %s\n", j, mystr);
	}
	ret = GetPefEntry (0x03, 0, (PEF_RECORD *) & LanRecord);
	if (ret == 0)
	  printf ("PEF Startup Delay: %02x : %d sec\n",
		  LanRecord.data[0], LanRecord.data[0]);
	if (!fmBMC) {
	  ret = GetPefEntry (0x04, 0, (PEF_RECORD *) & LanRecord);
	  if (ret == 0)
	    printf ("PEF Alert Startup Delay: %02x: %d sec\n",
		    LanRecord.data[0], LanRecord.data[0]);
	  /* fmBMC gets cc=0x80 here */
	}
	/* note that ndest should be read from lan param 17 below. */
	for (i = 1; i <= ndest; i++) {
	  ret = GetPefEntry (0x09, (ushort) i, (PEF_RECORD *) & LanRecord);
	  if (ret == 0) {
	    mystr[0] = 0;
	    j = LanRecord.data[2];
	    if (LanRecord.data[1] & 0x08) {
	      sprintf (mystr, "Chan[%d] Dest[%d] ", ((j & 0xf0) >> 4),
		       (j & 0x0f));
	      strcat (mystr, "Enabled ");
	    }
	    else
	      strcpy (mystr, "Disabled ");
	    printf ("PEF Alert Policy[%d]: %02x %02x %02x %02x : %s\n", i,
		    LanRecord.data[0], LanRecord.data[1],
		    LanRecord.data[2], LanRecord.data[3], mystr);
	  }
	}			/*endfor ndest */
      }				/*endif fpefok */
    }				/*endif not canonical */

    if (fpefenable && !freadonly) {	/* fenable or fdisable */
      if (fSetPEFOks)
	pefadd = 5;
      else
	pefadd = 2;
      sz = (pefnum - 1) + pefadd + fCustomPEF;
      printf ("\n%s, SetPefEntry(1-%d) ...\n", progname, sz);
      if (fdebug)
	printf ("pefnum = %d, pefmax = %d\n", pefnum, pefmax);
      for (idx = 1; idx <= pefmax; idx++) {
	// Set & Enable all PEF records
	memset (&PefRecord.rec_id, 0, sizeof (PEF_RECORD));
	PefRecord.rec_id = (uchar) idx;	/* next record, or user-specified */
	if (idx < pefnum) {	/* pefnum defaults to 12.(0x0c) */
	  if (pef_array[idx - 1][7] == 0)	/*empty pef record, set to default */
	    memcpy (&PefRecord.rec_id, pef_defaults[idx - 1],
		    sizeof (PEF_RECORD));
	  else {		/* set config however it was previously */
	    memcpy (&PefRecord.rec_id, pef_array[idx - 1],
		    sizeof (PEF_RECORD));
	    if (PefRecord.severity == 0)
	      PefRecord.severity = pef_defaults[idx - 1][4];
	  }
	}
	else if ((idx == pefnum) &&	/* new OS Crit Stop entry */
		 (PefRecord.sensor_type == 0)) {
	  // Set PEF values for 0x20, OS Critical Stop event 
	  PefRecord.severity = PEF_SEV_CRIT;
	  PefRecord.genid1 = 0xff;
	  PefRecord.genid2 = 0xff;
	  PefRecord.sensor_type = 0x20;	/* OS Critical Stop */
	  PefRecord.sensor_no = 0xff;
	  PefRecord.event_trigger = 0x6f;
	  PefRecord.data1 = 0xff;
	  PefRecord.mask1 = 0x00;
	}
	else if ((idx == pefnum + 1) &&	/* new Power Redundancy entry */
		 (PefRecord.sensor_type == 0)) {
	  // Set PEF values for 0x09/0x02/0x0b/0x41, Power Redundancy Lost 
	  PefRecord.severity = PEF_SEV_WARN;
	  PefRecord.genid1 = 0xff;
	  PefRecord.genid2 = 0xff;
	  PefRecord.sensor_type = 0x09;	/* Power Unit */
	  PefRecord.sensor_no = 0xff;	/* usu 01 or 02 */
	  PefRecord.event_trigger = 0x0b;	/* event trigger */
	  PefRecord.data1 = 0x02;	/* 02 -> 41=Redundancy Lost */
	  PefRecord.mask1 = 0x00;
	}
	else if (fSetPEFOks && idx == (pefnum + 2)) {
	  PefRecord.severity = PEF_SEV_OK;
	  PefRecord.genid1 = 0xff;
	  PefRecord.genid2 = 0xff;
	  PefRecord.sensor_type = 0x09;	/* Power Unit, Redund OK */
	  PefRecord.sensor_no = 0xff;	/* usu 01 or 02 */
	  PefRecord.event_trigger = 0x0b;	/* event trigger */
	  PefRecord.data1 = 0x01;	/* 01 -> 40=Redundancy OK */
	  PefRecord.mask1 = 0x00;
	}
	else if (fSetPEFOks && idx == (pefnum + 3)) {
	  PefRecord.severity = PEF_SEV_OK;
	  PefRecord.genid1 = 0xff;
	  PefRecord.genid2 = 0xff;
	  PefRecord.sensor_type = 0x01;	/* Temp OK */
	  PefRecord.sensor_no = 0xff;	/* usu 01 or 02 */
	  PefRecord.event_trigger = 0x81;	/* event trigger */
	  PefRecord.data1 = 0x95;	/* 95 -> 50(NC),52(Crit) match */
	  PefRecord.mask1 = 0x0a;
	}
	else if (fSetPEFOks && idx == (pefnum + 4)) {
	  PefRecord.severity = PEF_SEV_OK;
	  PefRecord.genid1 = 0xff;
	  PefRecord.genid2 = 0xff;
	  PefRecord.sensor_type = 0x02;	/* Voltage OK */
	  PefRecord.sensor_no = 0xff;	/* usu 01 or 02 */
	  PefRecord.event_trigger = 0x81;	/* event trigger */
	  PefRecord.data1 = 0x95;	/* 95 -> 50(NC),52(Crit) match */
	  PefRecord.mask1 = 0x0a;
	}
	else if (fCustomPEF && idx == (pefnum + pefadd)) {
	  /* user entered 10 or 18 PEF entry bytes */
	  PefRecord.action = custPEF[0];
	  PefRecord.policy = custPEF[1];
	  PefRecord.severity = custPEF[2];
	  PefRecord.genid1 = custPEF[3];
	  PefRecord.genid2 = custPEF[4];
	  PefRecord.sensor_type = custPEF[5];
	  PefRecord.sensor_no = custPEF[6];
	  PefRecord.event_trigger = custPEF[7];
	  PefRecord.data1 = custPEF[8];
	  PefRecord.mask1 = custPEF[9];
	  memcpy (&PefRecord.action, custPEF, 18);
	}
	else {
	  memcpy (&PefRecord.rec_id, pef_array[idx - 1], sizeof (PEF_RECORD));
	  if (PefRecord.sensor_type == 0)
	    continue;		/* if reserved, skip it */
	}
	if (fdebug && (PefRecord.rec_id != idx)) {
	  /* memcpy from pef_defaults or pef_array clobbered rec_id */
	  printf ("Warning: SetPef idx=%d, rec_id=%d\n", idx,
		  PefRecord.rec_id);
	  PefRecord.rec_id = (uchar) idx;	/*fix it */
	}
	if (fdisable) {
	  /* Disable all PEF rules */
	  if (idx >= pefnum)
	    PefRecord.fconfig = 0x00;	/*disabled, software */
	  else
	    PefRecord.fconfig = 0x40;	/*disabled, preset */
	  PefRecord.action = 0x00;
	  PefRecord.policy = 0x00;
	}
	else {			/*fenable */
	  if (PefRecord.sensor_type != 0) {	/* not an empty PEF entry */
	    /* Enable all non-empty PEF rules */
	    if (fCustomPEF && (idx == (pefnum + pefadd))) {
	      PefRecord.action = custPEF[0];
	      PefRecord.policy = custPEF[1];
	    }
	    else {
	      PefRecord.action = 0x01;	/*Alert */
	      PefRecord.policy = 0x01;	/*see Alert Policy #1 */
	    }
	    if (idx < pefnum) {	/* special handling for presets, 1 thru 11 */
	      PefRecord.fconfig = 0x80;	/* enabled, software */
	      ret = SetPefEntry (&PefRecord);
	      if (fdebug)
		printf ("SetPefEntry(%d/80) ret=%d\n", PefRecord.rec_id, ret);
	      // if (ret != 0) { nerrs++; lasterr = ret; }
	      // else ngood++;
	      PefRecord.fconfig = 0xC0;	/* enabled, preset */
	    }
	    else {
	      PefRecord.fconfig = 0x80;	/* enabled, software */
	    }
	  }			/*endif not empty */
	}
	{			// Show the new PEF record before setting it.
	  pc = (uchar *) & PefRecord;
	  sz = 21;
	  printf ("PEFilter(%d): ", PefRecord.rec_id);
	  for (i = 0; i < sz; i++)
	    printf ("%02x ", pc[i]);
	  printf ("\n");
	}
	ret = SetPefEntry (&PefRecord);
	if (fdebug)
	  printf ("SetPefEntry(%d) ret = %d\n", PefRecord.rec_id, ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
      }				/*end for */
    }
  }				/*end if not fIPMI10 */

  if (!fcanonical)
    printf ("\n%s, GetLanEntry for channel %d ...\n", progname, lan_ch);
  idest = 1;
  for (idx = 0; idx < NLAN; idx++) {
    int ival;
    if (fcanonical && (canon_param[idx] == 0))
      continue;
    if (idx == 8 || idx == 9)
      continue;			/* not implemented */
    ival = lanparams[idx].cmd;
    if (ival >= 96 && ival <= 98)
      continue;			/* not implemented */
    if (ival >= 102 && ival <= 108) {	/*custom IPv6 parameters */
      if (fipv6 == 0)
	continue;		/*skip these */
    }
    if (ival == 194 && vend_id == VENDOR_KONTRON) {	/*oem hostname parm */
      lanparams[idx].sz = 36;
      strcpy (lanparams[idx].desc, "IPMI Hostname");
    }
    else if (ival >= 192 && ival <= 194) {	/*custom DHCP parameters */
      if (vend_id != VENDOR_INTEL)
	continue;
      if (fmBMC || fiBMC || fcanonical)
	continue;		/*skip these */
    }
    if (ival >= 20 && ival <= 25) {
      if (!fIPMI20)
	continue;		/*VLAN params 20-25, fIPMI20 only/opt */
    }
    if ((ndest == 0) && (ival >= 18 && ival <= 19))
      continue;			/*skip dest */
    if (ival == 11) {		/*grat arp interval */
      if (vend_id == VENDOR_SUPERMICROX)
	continue;
      if (vend_id == VENDOR_SUPERMICRO)
	continue;
    }
    if (ival == 14 || ival == 15) {	/*secondary gateway is optional */
      if (vend_id == VENDOR_KONTRON)
	continue;
    }
    if (ival == 201) {		/*Get Channel Access */
      ret = ShowChanAcc (lan_ch);
    }
    else {
      if (ival == 18 || ival == 19) {	/*dest params */
	if (ndest == 0)
	  continue;		/*skip if ndest==0 */
	bset = (uchar) idest;	/* dest id = 1 thru n */
      }
      else
	bset = 0;
      ret = GetLanEntry ((uchar) ival, bset, &LanRecord);
    }
    if (ret == 0) {		// Show the LAN record
      pc = (uchar *) & LanRecord;
      sz = lanparams[idx].sz;
      if (ival == 18) {		/*skip if invalid dest type param */
	if ((idest > 1) && (pc[2] == 0)) {
	  idest = 1;
	  continue;
	}
      }
      else if (ival == 19) {	/*skip if invalid dest addr param */
	if ((idest > 1) && !IpIsValid (&pc[3])) {
	  idest = 1;
	  continue;
	}
      }
      if (ival == 201);		/* did it above */
      else {
	if (fcanonical) {
	  if ((ival == 19) && (idest > 1));	/*skip it */
	  else {
	    j = strlen_ (lanparams[idx].desc);
	    // (ival < 7) || (ival == 19) || ival == 102)
	    if (j <= 12)
	      pstr = pspace3;
	    else
	      pstr = pspace2;
	    printf ("Channel %d %s %s%c ", lan_ch,
		    lanparams[idx].desc, pstr, bdelim);
	  }
	}
	else
	  printf ("Lan Param(%d) %s: ", ival, lanparams[idx].desc);
      }
      if (ival == 1) {
	authmask = pc[0];	/* auth type support mask */
	/* if (fmBMC) authmask is usually 0x15, else 0x14 */
      }
      else if (ival == 3) {
	if (IpIsValid (pc))
	  memcpy (bmcmyip, pc, 4);
      }
      else if (ival == 5) {
	if (MacIsValid (pc))
	  memcpy (bmcmymac, pc, MAC_LEN);
      }
      else if (ival == 6) {
	if (SubnetIsValid (pc))
	  memcpy (bmcsubnet, pc, 4);
	/* else if invalid, leave default as 255.255.255.0 */
	//} else if (ival == 7) { 
	//   if (pc[0] >= 30) memcpy(bparm7,pc,3);
      }
      else if (ival == 17) {	/* num dest */
	ndest = pc[0];		/* save the number of destinations */
      }
      else if (ival == 12) {	/* gateway addr */
	if (IpIsValid (pc))
	  memcpy (bmcgwyip, pc, 4);
      }
      else if (ival == 13) {	/* gateway mac */
	if (MacIsValid (pc))
	  memcpy (bmcgwymac, pc, MAC_LEN);
      }
      else if (ival == 19) {	/* dest addr */
	if (IpIsValid (&pc[3]))
	  memcpy (bmcdestip, &pc[3], 4);
	if (MacIsValid (&pc[7]))
	  memcpy (bmcdestmac, &pc[7], MAC_LEN);
      }
      else if (ival == 22) {	/*Cipher Suite Support */
	nciphers = pc[0];
      }
      /* now start to display data */
      if (ival == 16) {
	printf ("%s \n", pc);	/* string */
      }
      else if (ival == 194 && vend_id == VENDOR_KONTRON) {
	printf ("%s \n", pc);	/* string */
      }
      else if (ival == 201) {;	/* did it above */
      }
      else {			/* print results for all other ival's */
	pstr = "";		/*interpreted meaning */
	if (fcanonical) {
	  switch (ival) {
	  case 4:		/*param 4, ip src */
	    if (pc[0] == SRC_STATIC)
	      pstr = "Static";	/*0x01 */
	    else if (pc[0] == SRC_DHCP)
	      pstr = "DHCP";	/*0x02 */
	    else if (pc[0] == SRC_BIOS)
	      pstr = "BIOS";	/*0x03 */
	    else
	      pstr = "Other";
	    printf ("%s\n", pstr);
	    break;
	  case 5:		/*param 5, mac addr */
	  case 13:		/*param 6, def gwy mac */
	    printf ("%02x:%02x:%02x:%02x:%02x:%02x\n",
		    pc[0], pc[1], pc[2], pc[3], pc[4], pc[5]);
	    break;
	  case 3:		/*param 4, ip address */
	  case 6:		/*param 6, subnet mask */
	  case 12:		/*param 12, def gwy ip */
	  case 14:		/*param 14, sec gwy ip */
	  case 192:		/*param 192, DHCP svr ip */
	    printf ("%d.%d.%d.%d\n", pc[0], pc[1], pc[2], pc[3]);
	    break;
	  case 19:		/*param 19, dest address */
	    if (idest == 1) {
	      printf ("IP=%d.%d.%d.%d "
		      "MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
		      pc[3], pc[4], pc[5], pc[6],
		      pc[7], pc[8], pc[9], pc[10], pc[11], pc[12]);
	    }
	    break;
	  case 102:		/*param 102, IPv6 enable */
	    if (pc[0] == 0x01)
	      printf ("enabled\n");
	    else
	      printf ("disabled\n");
	    break;
	  default:
	    printf ("%02x \n", pc[0]);
	    break;
	  }
	}
	else {			/*not canonical */
	  if (ival == 3 || ival == 6 || ival == 12 || ival == 14
	      || ival == 192) {
	    printf ("%d.%d.%d.%d", pc[0], pc[1], pc[2], pc[3]);
	  }
	  else if (ival == 23) {	/*Cipher Suites */
	    for (i = 1; i <= nciphers; i++) {
	      if (pc[i] == 0)
		ncipher0 = pc[i];
	      printf ("%2d ", pc[i]);
	    }
	  }
	  else if (ival == 24) {	/*Cipher Suite Privi Levels */
	    j = 0;
	    for (i = 1; i < 9; i++) {
	      char c1, c2;
	      char *p;
	      p = parse_priv ((pc[i] & 0x0f));
	      c1 = p[0];
	      p = parse_priv ((pc[i] & 0xf0) >> 4);
	      c2 = p[0];
	      rgciphers[j++] = (pc[i] & 0x0f);
	      rgciphers[j++] = ((pc[i] & 0xf0) >> 4);
	      if ((i * 2) >= nciphers)
		c2 = ' ';
	      printf (" %c  %c ", c1, c2);
	      if ((i * 2) > nciphers)
		break;
	    }
	  }
	  else
	    for (i = 0; i < sz; i++) {
	      /* print in hex, decimal, or string, based on ival */
	      if (ival == 1) {	/* Auth type support */
		pstr = &mystr[0];
		getauthstr (authmask, pstr);
		printf ("%02x ", authmask);
	      }
	      else if (ival == 4) {	/* IP addr source */
		if (pc[i] == SRC_STATIC)
		  pstr = "Static";	/*0x01 */
		else if (pc[i] == SRC_DHCP)
		  pstr = "DHCP";	/*0x02 */
		else if (pc[i] == SRC_BIOS)
		  pstr = "BIOS";	/*0x03 */
		else
		  pstr = "Other";
		printf ("%02x ", pc[i]);
	      }
	      else if (ival == 10) {	/* grat ARP */
		mystr[0] = 0;
		if (pc[i] == 0)
		  strcat (mystr, "ARP disabled ");
		else if (pc[i] & 0x01)
		  strcat (mystr, "Grat-ARP enabled");
		else
		  strcat (mystr, "Grat-ARP disabled");
		if (pc[i] & 0x02)
		  strcat (mystr, ", ARP-resp enabled");
		pstr = &mystr[0];
		printf ("%02x ", pc[i]);
	      }
	      else if (ival == 11) {	/* grat ARP interval */
		float f;
		f = (float) pc[i] / (float) 2;	/*500msec increments */
		sprintf (mystr, "%.1f sec", f);
		pstr = &mystr[0];
		printf ("%02x ", pc[i]);
	      }
	      else if (ival == 19) {	/* dest addr */
		if (i > 2 && i < 7) {
		  char *sepstr;
		  if (i == 3)
		    printf ("[");
		  if (i == 6)
		    sepstr = "] ";
		  else if (i >= 3 && i < 6)
		    sepstr = ".";
		  else
		    sepstr = " ";
		  printf ("%d%s", pc[i], sepstr);	/* IP address in dec */
		}
		else
		  printf ("%02x ", pc[i]);	/* show mac/etc. in hex */
	      }
	      else
		printf ("%02x ", pc[i]);	/* show in hex */
	    }			/*end for */
	  if (ival == 2) {	/*Auth type enables */
	    pstr = &mystr[0];
	    i = 0;
	    if (lan_ch > 0)
	      i = lan_ch - 1;
	    getauthstr (pc[i], pstr);
	  }
	  if (pstr[0] != 0)
	    printf (": %s\n", pstr);
	  else
	    printf ("\n");
	}			/*end-else not canonical */
      }				/*end-else others */
      if (ival == 18 || ival == 19) {
	if (idest < ndest) {
	  idest++;
	  idx--;		/* repeat this param */
	}
	else
	  idest = 1;
      }
    }
    else {			/* ret != 0 */
      if (ival >= 20 && ival <= 25) {	/*if errors, optional */
	if (fdebug)
	  printf ("GetLanEntry(%d), ret = %d\n", ival, ret);
      }
      else
	printf ("GetLanEntry(%d), ret = %d\n", ival, ret);
      if (ival == 17)
	ndest = 0;		/*error getting num dest */
    }
  }				/*end for */
  if (fRomley || fGrantley) {	/*get LAN Failover param */
    uchar b;
    ret = lan_failover_intel (0xFF, (uchar *) & b);
    if (ret != 0)
      printf ("Intel Lan Failover, ret = %d\n", ret);
    else {
      if (b == 1)
	pstr = "enabled";
      else
	pstr = "disabled";
      if (fcanonical)
	printf ("Intel Lan Failover  %s%c %s\n", pspace3, bdelim, pstr);
      else
	printf ("Intel Lan Failover %s%c %02x %c %s\n",
		pspace2, bdelim, b, bdelim, pstr);
    }
  }
  if (vend_id == VENDOR_SUPERMICROX || vend_id == VENDOR_SUPERMICRO) {
    ret = oem_supermicro_get_lan_port (&bset);
    if (ret == 0) {
      pstr = oem_supermicro_lan_port_string (bset);
      if (fcanonical)
	printf ("SuperMicro Lan Interface  %s%c %s\n", pspace2, bdelim, pstr);
      else
	printf ("SuperMicro Lan Interface  %c %02x    %c %s\n",
		bdelim, bset, bdelim, pstr);
    }
    else {
      if (fdebug)
	printf ("oem_supermicro_get_lan_port error %d\n", ret);
      ret = 0;			/*may not be supported on all smc plaforms */
    }
  }
  // if (fmBMC) lan_access = 0x04;  /*Admin*/
  // else lan_access = 0x04; /*Admin*/
  if (!fIPMI10) {		/* Get SOL params */
    ret = GetSerialOverLan (lan_ch, 0, 0);
    if (ret != 0) {
      printf ("GetSOL error %d, %s\n", ret, decode_rv(ret));
	  ret = 0; /*does not fail entire command*/
	}
  }
  for (i = 1; i <= show_users; i++)
    GetUser ((uchar) i, lan_ch);

  if (fgetser && !fcanonical) {
    printf ("\n%s, GetSerEntry ...\n", progname);
    if (fmBMC)			/* mBMC doesn't support serial */
      printf ("No serial channel support on this platform\n");
    else
      for (idx = 0; idx < NSER; idx++) {
	int ival;
	// if (idx == 9) continue; /* not implemented */
	ival = serparams[idx].cmd;
	if (ival == 201) {
	  ret = GetChanAcc (ser_ch, 0x40, &LanRecord);
	}
	else {
	  ret = GetSerEntry ((uchar) ival, &LanRecord);
	}
	if (ret == 0) {		// Show the SER record
	  pc = (uchar *) & LanRecord;
	  sz = serparams[idx].sz;
	  printf ("Serial Param(%d) %s: ", ival, serparams[idx].desc);
	  if (idx == 10) {	/* modem init string */
	    pc[sz] = 0;
	    printf ("%02x %s\n", pc[0], &pc[1]);
	  }
	  else if ((idx >= 11 && idx <= 13) || idx == 15) {	/* strings */
	    printf ("%s\n", pc);
	  }
	  else {
	    for (i = 0; i < sz; i++) {
	      printf ("%02x ", pc[i]);	/* show in hex */
	    }
	    printf ("\n");
	  }			/*end else */
	}
      }				/*end for */
  }				/*endif fgetser */

  if (!freadonly) {		/* Set IPMI LAN enable/disable params. */
    if (fipmilan)		/* Sets not valid via ipmi_lan if same channel. */
      printf ("\nWarning: Setting LAN %d params while using a LAN channel.\n",
	      lan_ch);

    {
      if (fenable && (fsharedMAC == 0) && !lan_dhcp) {
	/* must have an IP from -I option */
	if (!IpIsValid (rgmyip)) {	/* if not user-specified */
	  if (IpIsValid (bmcmyip)) {
	    memcpy (rgmyip, bmcmyip, 4);
	    if (fdebug)
	      printf ("Using current IP %d.%d.%d.%d\n",
		      bmcmyip[0], bmcmyip[1], bmcmyip[2], bmcmyip[3]);
	  }
	  else {
	    printf ("\nNot shared BMC LAN, must specify a unique "
		    "IP address via -I\n");
	    ret = ERR_BAD_PARAM;
	    goto do_exit;
	  }
	}
      }
      /* Set LAN parameters.  fenable or fdisable */
      printf ("\n%s, SetLanEntry for channel %d ...\n", progname, lan_ch);
      /* use ifname to resolve MAC addresses below */
      if (fdisable) {
	if (!fIPMI10) {
	  ret = DisablePef (alertnum);
	  printf ("DisablePef, ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	}
	if (lan_user != 0) {
	  j = DisableUser (lan_user, lan_ch);	/*disable this lan user */
	  printf ("DisableUser(%d), ret = %d\n", lan_user, j);
	  if (j != 0) {
	    nerrs++;
	    lasterr = j;
	  }
	  else ngood++;
	}
	LanRecord.data[0] = 0x01;	/* static IP address source */
	ret = SetLanEntry (4, &LanRecord, 1);
	printf ("SetLanEntry(4), ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
	/* clear the BMC IP address */
	memset (&LanRecord, 0, 4);
	ret = SetLanEntry (3, &LanRecord, 4);
	printf ("SetLanEntry(3), ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
	/* clear the gateway IP address */
	memset (&LanRecord, 0, 4);
	ret = SetLanEntry (12, &LanRecord, 4);
	printf ("SetLanEntry(12), ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
	/* clear the gateway MAC address */
	memset (&LanRecord, 0, 6);
	ret = SetLanEntry (13, &LanRecord, 6);
	printf ("SetLanEntry(13), ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
      }
      else if (fdisableSOL) {
	ret = SetupSerialOverLan (0);	/*disable */
	SELprintf ("SetupSerialOverLan: ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
      }
      else {			/*fenable */
	uchar chanctl;
	if (bmcpefctl != 0)
	  chanctl = chan_pefon;	/*previously on */
	else
	  chanctl = chan_pefoff;
	ret = SetChanAcc (lan_ch, 0x80, chanctl);
	if (fdebug)
	  printf ("SetChanAcc(lan/active), ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
	ret = SetChanAcc (lan_ch, 0x40, chanctl);
	if (fdebug)
	  printf ("SetChanAcc(lan/nonvol), ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
	if (flansecure) {
	  j = DisableUser (0, lan_ch);	/*disable the default null user */
	  printf ("DisableUser(0), ret = %d\n", j);
	}
	ret = SetUser (lan_user, myuser, passwordData, lan_ch);
	printf ("SetUser(%d), ret = %d\n", lan_user, ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
      }
      if (fdisable && (vend_id == VENDOR_SUPERMICROX
		       || vend_id == VENDOR_SUPERMICRO)) {
	failover_enable = 0;	/*dedicated */
	ret = oem_supermicro_set_lan_port (failover_enable);
	printf ("Set SuperMicro Lan port to %s, ret = %d\n",
		oem_supermicro_lan_port_string (failover_enable), ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
      }

      if (fdisable || fdisableSOL) {
// if (nerrs > 0) printf("Warning: %d errors occurred\n",nerrs);
	goto do_exit;
      }

      if (authmask == 0)
	authmask = 0x17;	/*if none from GetLanEntry(1) */
      LanRecord.data[0] = (bAuth & authmask);	/*Callback level */
      LanRecord.data[1] = (bAuth & authmask);	/*User level    */
      LanRecord.data[2] = (bAuth & authmask);	/*Operator level */
      LanRecord.data[3] = (bAuth & authmask);	/*Admin level   */
      LanRecord.data[4] = 0;	/*OEM level */
      if (fdebug)
	printf ("SetLanEntry(2): %02x %02x %02x %02x %02x\n",
		LanRecord.data[0], LanRecord.data[1], LanRecord.data[2],
		LanRecord.data[3], LanRecord.data[4]);
      ret = SetLanEntry (2, &LanRecord, 5);
      printf ("SetLanEntry(2), ret = %d\n", ret);
      if (ret != 0) {
	nerrs++;
	lasterr = ret;
      }
      else ngood++;

      /* Get the values to use from Linux eth0, etc. */
      ret = Get_IPMac_Addr ();
      if (lan_dhcp) {		/* use DHCP */
	LanRecord.data[0] = SRC_DHCP;	/* BMC running DHCP */
	/* = SRC_BIOS;  * address source = BIOS using DHCP */
	ret = SetLanEntry (4, &LanRecord, 1);
	printf ("SetLanEntry(4), ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
	if (MacIsValid (rgmymac)) {
	  memcpy (&LanRecord, rgmymac, 6);
	  ret = SetLanEntry (5, &LanRecord, 6);
	  if (ret == 0x82) {	/*BMC may not allow the MAC to be set */
	    if (fdebug)
	      printf ("SetLanEntry(5), ret = %x cannot modify MAC\n", ret);
	  }
	  else {
	    printf ("SetLanEntry(5), ret = %d\n", ret);
	    if (ret != 0) {
	      nerrs++;
	      lasterr = ret;
	    }
	    else ngood++;
	  }
	}

	/* DHCP also relates to OEM LAN params 192, 193, 194 */
	if ((vend_id == VENDOR_INTEL) && !fmBMC && !fiBMC) {	/*DHCP params 192-194 are Intel only */
	  if (IpIsValid (rgmyip)) {
	    /* Set DHCP Server IP in param 192 from -I param. */
	    memcpy (&LanRecord, rgmyip, 4);
	    ret = SetLanEntry (192, &LanRecord, 4);
	    printf ("SetLanEntry(192), ret = %d\n", ret);
	    if (!MacIsValid (rgdhcpmac))	/* if MAC not set yet */
	      ret = Get_Mac (rgmyip, rgdhcpmac, NULL);
	    if (ret == 0) {
	      memcpy (&LanRecord, rgdhcpmac, MAC_LEN);
	      ret = SetLanEntry (193, &LanRecord, MAC_LEN);
	      printf ("SetLanEntry(193), ret = %d\n", ret);
	    }
	  }
	  LanRecord.data[0] = 0x01;	/*enable DHCP */
	  ret = SetLanEntry (194, &LanRecord, 1);
	  printf ("SetLanEntry(194), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	}
      }
      else {			/* use static IP */
	printf
	  ("LAN%d (%s)\tip=%d.%d.%d.%d mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
	   lan_ch, ifname, rgmyip[0], rgmyip[1], rgmyip[2], rgmyip[3],
	   rgmymac[0], rgmymac[1], rgmymac[2], rgmymac[3], rgmymac[4],
	   rgmymac[5]);
	if (IpIsValid (rgmyip)) {
	  if (lan_ch != gcm_ch) {	/*skip if gcm */
	    LanRecord.data[0] = 0x00;	/*disable grat arp while setting IP */
	    ret = SetLanEntry (10, &LanRecord, 1);
	    if (fdebug)
	      printf ("SetLanEntry(10,0), ret = %d\n", ret);
	    if (ret != 0) {
	      nerrs++;
	      lasterr = ret;
	    }
	  }
	  LanRecord.data[0] = 0x01;	/* static IP address source */
	  ret = SetLanEntry (4, &LanRecord, 1);
	  printf ("SetLanEntry(4), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	  memcpy (&LanRecord, rgmyip, 4);
	  ret = SetLanEntry (3, &LanRecord, 4);
	  printf ("SetLanEntry(3), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	  if (MacIsValid (rgmymac)) {
	    memcpy (&LanRecord, rgmymac, 6);
	    ret = SetLanEntry (5, &LanRecord, 6);
	    if (ret == 0x82) {

	      /* Do not show anything, not an error if BMC does not
	         allow the BMC MAC to be changed. */
	      if (fdebug)

		printf ("SetLanEntry(5), ret = %x cannot modify MAC\n", ret);
	    }
	    else {

	      printf ("SetLanEntry(5), ret = %d\n", ret);
	      if (ret != 0) {
		nerrs++;
		lasterr = ret;
	      }
	      else ngood++;
	    }
	  }
	  if (!SubnetIsValid (rgsubnet))	/* not specified, use previous */
	    memcpy (rgsubnet, bmcsubnet, 4);
	  memcpy (&LanRecord, rgsubnet, 4);
	  ret = SetLanEntry (6, &LanRecord, 4);
	  printf ("SetLanEntry(6), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	  if (vend_id != VENDOR_PEPPERCON) {
	    /* may want to check bparm7 here */
	    LanRecord.data[0] = parm7[0];	/*IPv4 header, TTL */
	    LanRecord.data[1] = parm7[1];	/*IPv4 header, Flags */
	    LanRecord.data[2] = parm7[2];	/*IPv4 hdr, Precedence/Service */
	    ret = SetLanEntry (7, &LanRecord, 3);
	    printf ("SetLanEntry(7), ret = %d\n", ret);
	    if (ret != 0) {
	      nerrs++;
	      lasterr = ret;
	    }
	    else ngood++;
	  }
	  /* if lan_ch == 3, gcm gets error setting grat arp (ccode=0xCD) */
	  if (lan_ch != gcm_ch) {	/*skip if gcm */
	    /* 01=enable grat arp, 02=enable arp resp, 03=both */
	    LanRecord.data[0] = arp_ctl;	/*grat arp */
	    ret = SetLanEntry (10, &LanRecord, 1);
	    printf ("SetLanEntry(10,%x), ret = %d\n", arp_ctl, ret);
	    if (ret != 0) {
	      nerrs++;
	      lasterr = ret;
	    }
	    else ngood++;
	  }
	  LanRecord.data[0] = arp_interval;	/*grat arp interval */
	  ret = SetLanEntry (11, &LanRecord, 1);
	  printf ("SetLanEntry(11), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  if ((vend_id == VENDOR_INTEL) && !fmBMC && !fiBMC) {
	    LanRecord.data[0] = 0x00;	/*disable DHCP */
	    ret = SetLanEntry (194, &LanRecord, 1);
	    printf ("SetLanEntry(194), ret = %d\n", ret);
	    if (ret != 0) {
	      nerrs++;
	      lasterr = ret;
	    }
	    else ngood++;
	  }
	}
	else {			/* error, don't continue */
	  printf ("Missing IP Address, can't continue. Use -I to specify\n");
	  ret = ERR_BAD_PARAM;
	  goto do_exit;
	}
	if (vend_id == VENDOR_KONTRON && rghostname[0] != 0) {
	  /* set the IPMI Hostname if specified */
	  sz = strlen_ (rghostname);
	  /* LanRecord is larger than rghostname, bounds ok */
	  strncpy ((char *) &LanRecord.data, rghostname, sz);
	  ret = SetLanEntry (194, &LanRecord, sz);
	  printf ("SetLanEntry(194), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else {
	    LanRecord.data[0] = 0x31;
	    ret = SetLanEntry (195, &LanRecord, 1);	/*re-read hostname */
	    // printf("SetLanEntry(195), ret = %d\n",ret);
	    // if (ret != 0) { nerrs++; lasterr = ret; }
	  }
	}
	if (IpIsValid (rggwyip)) {
	  if (!MacIsValid (rggwymac))	/* if gwy MAC not set by user */
	    ret = Get_Mac (rggwyip, rggwymac, NULL);
	  printf
	    ("gateway \tip=%d.%d.%d.%d mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
	     rggwyip[0], rggwyip[1], rggwyip[2], rggwyip[3], rggwymac[0],
	     rggwymac[1], rggwymac[2], rggwymac[3], rggwymac[4], rggwymac[5]);
	  if (!SubnetIsSame (rgmyip, rggwyip, rgsubnet)) {
	    printf
	      ("WARNING: IP Address and Gateway are not on the same subnet,"
	       " setting Gateway to previous value\n");
	    memcpy (rggwyip, bmcgwyip, 4);
	    memcpy (rggwymac, bmcgwymac, 6);
	  }

	  /* Set the Default Gateway IP & MAC */
	  memcpy (&LanRecord, rggwyip, 4);
	  ret = SetLanEntry (12, &LanRecord, 4);
	  printf ("SetLanEntry(12), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	  if (!MacIsValid (rggwymac)) {	/* if gwy MAC not resolved */
	    printf ("  Warning: Gateway MAC address was not resolved! "
		    "Check %s interface, use -i ethN, or use -H gwymac.\n",
		    ifname);
	    memcpy (&LanRecord, bmcgwymac, 6);
	  }
	  else {
	    memcpy (&LanRecord, rggwymac, 6);
	  }
	  ret = SetLanEntry (13, &LanRecord, 6);
	  printf ("SetLanEntry(13), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	}
	if (IpIsValid (rggwy2ip)) {
	  if (!MacIsValid (rggwy2mac))	/* if gwy2 MAC not set by user */
	    ret = Get_Mac (rggwy2ip, rggwy2mac, NULL);
	  /* Set the Secondary Gateway IP & MAC */
	  memcpy (&LanRecord, rggwy2ip, 4);
	  ret = SetLanEntry (14, &LanRecord, 4);
	  printf ("SetLanEntry(14), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	  memcpy (&LanRecord, rggwy2mac, 6);
	  ret = SetLanEntry (15, &LanRecord, 6);
	  printf ("SetLanEntry(15), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	}
      }				/* end-else static IP */
      if (flansecure) {		/* disable cipher 0 */
	char c1, c2;
	memset (&LanRecord, 0, 12);
	j = 1;
	for (i = 0; i < nciphers; i += 2) {
	  c1 = rgciphers[i];
	  c2 = rgciphers[i + 1];
	  /* 0x0f may be vendor-specific, 0x00 = Reserved/Unused */
	  if (i == ncipher0)
	    c1 = 0x00;
	  if ((i + 1) == ncipher0)
	    c2 = 0x00;
	  LanRecord.data[j++] = (c2 << 4) | c1;
	}
	ret = SetLanEntry (24, &LanRecord, 9);
	printf ("SetLanEntry(24) disable cipher0, ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
      }
      ret = SetupSerialOverLan (1);	/*enable */
      SELprintf ("SetupSerialOverLan: ret = %d\n", ret);
      if (ret != 0) {
	nerrs++;
	lasterr = ret;
      }
      else ngood++;
      if (!IpIsValid (rgdestip) && IpIsValid (bmcdestip)) {
	memcpy (rgdestip, bmcdestip, 4);
	if (fdebug)
	  printf ("Using current dest IP %d.%d.%d.%d\n",
		  bmcdestip[0], bmcdestip[1], bmcdestip[2], bmcdestip[3]);
      }
      if (ndest == 0) {
		if (fdebug)
	  	 printf ("ndest==0, anum=%d rgdestip=%d.%d.%d.%d\n",
		  alertnum, rgdestip[0], rgdestip[1], rgdestip[2],
		  rgdestip[3]);
		printf ("alert dest \tnot supported\n");
      }
      else if (!IpIsValid (rgdestip)) {
		printf ("alert dest \taddress not specified\n");
      }
      else {			/* valid alert dest ip */
	  if (!MacIsValid (rgdestmac))	/* if dest MAC not set by user */
	  ret = Get_Mac (rgdestip, rgdestmac, NULL);	/*try to resolve MAC */
	  if (!MacIsValid (rgdestmac)) {	/* if dest MAC not resolved */
	    printf ("  Warning: Alert mac address was not resolved!"
		  " Check %s interface or use -i.\n", ifname);
	    /* use existing BMC alert dest mac (as best guess) */
	    memcpy (rgdestmac, bmcdestmac, 6);
	  }
	/* show destination data */
	printf("alert dest %d\tip=%d.%d.%d.%d mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
	   alertnum, rgdestip[0], rgdestip[1], rgdestip[2], rgdestip[3],
	   rgdestmac[0], rgdestmac[1], rgdestmac[2], rgdestmac[3],
	   rgdestmac[4], rgdestmac[5]);
	printf ("snmp community \t%s\n", rgcommunity);
	/* Only need the SNMP community if there is an Alert Destination */
	memset (&LanRecord.data[0], 0, 18);	/* make sure zero-filled */
	strcpy ((char *) &LanRecord.data[0], rgcommunity);
	ret = SetLanEntry (16, &LanRecord, 18);
	printf ("SetLanEntry(16), ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
	/* Set Alert Destination Type */
	LanRecord.data[0] = alertnum;	/* dest id = 1 */
	LanRecord.data[1] = 0x00;	/* dest type = PET, no ack */
	LanRecord.data[2] = 0x01;	/* ack timeout / retry interval */
	LanRecord.data[3] = 0x00;	/* no retries */
	// LanRecord.data[4] = 0x69; 
	ret = SetLanEntry (18, &LanRecord, 4);
	printf ("SetLanEntry(18), ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
	{
	  /* Set the Alert Destination IP & MAC (param 19) */
	  LanRecord.data[0] = alertnum;	/* dest id = 1 */
	  LanRecord.data[1] = 0x00;
	  LanRecord.data[2] = 0x00;
	  memcpy (&LanRecord.data[3], rgdestip, 4);
	  memcpy (&LanRecord.data[7], rgdestmac, 6);
	  ret = SetLanEntry (19, &LanRecord, 13);
	  printf ("SetLanEntry(19), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	}
   }				/*endif valid alert */

      /* Now enable PEF since we have an Alert destination. */
      if (!fdisable && !fIPMI10 && fpefenable) {	/*fpefenable */
	ret = EnablePef (alertnum);
	printf ("EnablePef, ret = %d\n", ret);
	if (ret != 0) {
	  nerrs++;
	  lasterr = ret;
	}
	else ngood++;
	/* ChanAcc changed, so show it again */
	j = ShowChanAcc (lan_ch);
      }

      if ((vlan_enable != PARM_INIT) && (fIPMI20)) {
	if (vlan_enable == 0) {	/*disable vlan */
	  LanRecord.data[0] = 0x00;
	  LanRecord.data[1] = 0x00;
	  ret = SetLanEntry (20, &LanRecord, 2);
	  printf ("SetLanEntry(20,disable) ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	}
	else {			/*vlan_enable == 1, enable vlan with id */
	  LanRecord.data[0] = (vlan_id & 0x00ff);
	  LanRecord.data[1] = ((vlan_id & 0x0f00) >> 8) | 0x80;
	  ret = SetLanEntry (20, &LanRecord, 2);
	  printf ("SetLanEntry(20,%d), ret = %d\n", vlan_id, ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	  LanRecord.data[0] = vlan_prio;
	  ret = SetLanEntry (21, &LanRecord, 1);
	  printf ("SetLanEntry(21), ret = %d\n", ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	  else ngood++;
	}
      }
      if (failover_enable != PARM_INIT) {
	if (fRomley || fGrantley) {
	  if (failover_enable > 1)
	    failover_enable = 0;	/*default */
	  ret = lan_failover_intel (failover_enable, (uchar *) & i);
	  printf ("Set Intel Lan Failover (%d), ret = %d\n",
		  failover_enable, ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	}
	else if (vend_id == VENDOR_SUPERMICROX
		 || vend_id == VENDOR_SUPERMICRO) {
	  if (failover_enable > 2)
	    failover_enable = 2;	/*default */
	  ret = oem_supermicro_set_lan_port (failover_enable);
	  printf ("Set SuperMicro Lan port to %s, ret = %d\n",
		  oem_supermicro_lan_port_string (failover_enable), ret);
	  if (ret != 0) {
	    nerrs++;
	    lasterr = ret;
	  }
	}
      }				/*endif failover specified */
    }				/*end-else not via ipmi_lan */
  }				/*endif not readonly */

  if (flanstats) {		/* get BMC LAN Statistics */
#ifdef METACOMMAND
    j = get_lan_stats (lan_ch);
#else
    uchar idata[2];
    uchar rdata[20];
    int rlen;
    uchar cc;
    idata[0] = lan_ch;
    idata[1] = 0x00;		/*do not clear stats */
    rlen = sizeof (rdata);
    j = ipmi_cmd (GET_LAN_STATS, idata, 2, rdata, &rlen, &cc, fdebug);
    if (j == 0) {		/*show BMC LAN stats */
      ushort *rw;
      rw = (ushort *) & rdata[0];
      printf ("IPMI LAN channel %d statistics: \n", lan_ch);
      printf (" \tReceived IP Packets      = %d\n", rw[0]);
      printf (" \tRecvd IP Header errors   = %d\n", rw[1]);
      printf (" \tRecvd IP Address errors  = %d\n", rw[2]);
      printf (" \tRecvd IP Fragments       = %d\n", rw[3]);
      printf (" \tTransmitted IP Packets   = %d\n", rw[4]);
      printf (" \tReceived UDP Packets     = %d\n", rw[5]);
      printf (" \tReceived Valid RMCP Pkts = %d\n", rw[6]);
      printf (" \tReceived UDP Proxy Pkts  = %d\n", rw[7]);
      printf (" \tDropped UDP Proxy Pkts   = %d\n", rw[8]);
    }
#endif
  }

do_exit:
  ipmi_close_ ();
  if (foptmsg) {
    if (fset_ip != 0)
      printf
	("WARNING: IP address options were specified, but no -e,-l,-d option.\n");
    else
      printf ("WARNING: %d options were specified, but no -e,-l,-d option.\n",
	      nopts);
    printf ("Read-only usage assumed.\n");
  }
  if (nerrs > 0) {
    printf ("Warning: %d ok, %d errors occurred, last error = %d\n", ngood,
	    nerrs, lasterr);
    ret = lasterr;
  }
  // show_outcome(progname,ret); 
  return (ret);
}				/* end main() */

/* end ilan.c */
