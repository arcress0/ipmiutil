/*---------------------------------------------------------------------------
 * Filename:   iconfig.c (was bmcconfig.c)
 *
 * Author:     arcress at users.sourceforge.net
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * Abstract:
 * This tool saves and restores the BMC Configuration parameters.
 * This includes BMC PEF, LAN, Serial, User, Channel, and SOL parameters.
 *
 * ----------- Change History -----------------------------------------------
 * 08/18/08 Andy Cress - created from pefconfig.c
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
#ifdef SOLARIS
#include <sys/sockio.h>
#define SIOCGIFHWADDR   SIOCGENADDR
#define ifr_netmask         ifr_ifru.ifru_addr
#elif defined(BSD)
#include <sys/sockio.h>
#define SIOCGIFHWADDR   SIOCGIFMAC
#define ifr_netmask         ifr_ifru.ifru_addr
#elif defined(MACOS)
#include <sys/sockio.h>
// #define SIOCGIFHWADDR   SIOCGIFMAC
#define ifr_netmask         ifr_ifru.ifru_addr
#endif
#include "ipmicmd.h" 
#include "oem_intel.h" 
#include "oem_supermicro.h" 

extern int find_ifname(char *ifname);   /*see idiscover.c*/
/* atoip was moved to subs.c, defined in ipmicmd.h */

#define SELprintf          printf
#define RTF_UP          0x0001   /* route usable */

#define SOL_ENABLE_FLAG			0x01
#define SOL_DISABLE_FLAG		0x00
#define SOL_PRIVILEGE_LEVEL_USER	0x02
#define SOL_PREFERRED_BAUD_RATE		0x07  /*19.2k*/
/* For IPMI 1.5, use Intel SOL commands & subfunctions */
#define SOL_ENABLE_PARAM		0x01
#define SOL_AUTHENTICATION_PARAM 	0x02
#define SOL_ACC_INTERVAL_PARAM	 	0x03
#define SOL_RETRY_PARAM  	 	0x04
#define SOL_BAUD_RATE_PARAM		0x05  /*non-volatile*/
#define SOL_VOL_BAUD_RATE_PARAM		0x06  /*volatile*/
/* For IPMI 2.0, use IPMI SOL commands & subfunctions */
#define SOL_ENABLE_PARAM2		0x08
#define SOL_AUTHENTICATION_PARAM2 	0x09
#define SOL_BAUD_RATE_PARAM2		0x11

/* IPMI 2.0 SOL PAYLOAD commands */
#define SET_PAYLOAD_ACCESS  0x4C
#define GET_PAYLOAD_ACCESS  0x4D
#define GET_PAYLOAD_SUPPORT 0x4E

/* Channel Access values */
#define CHAN_ACC_DISABLE   0x20   /* PEF off, disabled*/
#define CHAN_ACC_PEFON     0x02   /* PEF on, always avail */
#define CHAN_ACC_PEFOFF    0x22   /* PEF off, always avail*/
/* special channel access values for ia64 */
#define CHAN_ACC_PEFON64   0x0A   /* PEF on, always avail, UserLevelAuth=off */
#define CHAN_ACC_PEFOFF64  0x2A   /* PEF off, always avail, UserLevelAuth=off */

   /* TSRLT2/TIGPR2U Channels: 0=IPMB, 1=Serial/EMP, 6=LAN2, 7=LAN1 */
   /* S5000/other Channels: 1=LAN1, 2=LAN2, 3=LAN3, 4=Serial, 6=pci, 7=sys */
#define LAN_CH   1  
#define SER_CH   4
#define MAXCHAN  12  /*was 16, reduced for gnu ipmi_lan*/
#define NUM_DEVICES_TO_CHECK		32   /*for GetBmcEthDevice()*/
#define MAC_LEN  6   /*length of MAC Address*/
#define PSW_LEN  16   /*see also PSW_MAX=20 in ipmicmd.h*/
#define MAXPEF 41	 	/* max pefnum offset = 40 (41 entries) */

   /* IP address source values */
#define SRC_STATIC 0x01
#define SRC_DHCP   0x02  /* BMC running DHCP */
#define SRC_BIOS   0x03  /* BIOS, sometimes DHCP */
#define SRC_OTHER  0x04

/* PEF event severities */
#define PEF_SEV_UNSPEC   0x00
#define PEF_SEV_MON      0x01
#define PEF_SEV_INFO     0x02
#define PEF_SEV_OK       0x04
#define PEF_SEV_WARN     0x08
#define PEF_SEV_CRIT     0x10
#define PEF_SEV_NORECOV  0x20

typedef struct
{            /* See IPMI Table 15-2 */
        uchar              rec_id;
        uchar              fconfig;
        uchar              action;
        uchar              policy;
        uchar              severity;
        uchar              genid1;
        uchar              genid2;
        uchar              sensor_type;
        uchar              sensor_no;
        uchar              event_trigger;
        uchar              data1;
        uchar              mask1;
        uchar              res[9];
}       PEF_RECORD;

/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil config";
#else
static char * progver   = "3.08";
static char * progname  = "iconfig";
#endif
static char   fdebug    = 0;
static char   fipmilan  = 0;
static FILE * fd_bmc    = NULL;
static char   fIPMI10   = 0;      /* =1 if IPMI v1.0 or less */
static char   fIPMI20   = 0;      /* =1 if IPMI v2.0 or greater */
static char   fSOL20    = 1;      /* =1 if SOL v2.0 is ok */
static char   freadonly = 1;      /* =1 to only read LAN & PEF parameters */
static char   func      = 'd';    /* function: display, save, restore */
static char   fdomac    = 0;      /* =1 to restore MAC also */
static char   fpassword = 0;	  /* =1 user-specified a password, so set it. */
static uchar  fmBMC     = 0;  
static uchar  fiBMC     = 0;  
static uchar  fRomley   = 0;
static uchar  fGrantley = 0;
static char   fipv6     = 0;
static char   fcanonical = 0;
static char   fchan2wart = 0;     /* =1 if need wart to skip channel 2 */
static char   bdelim    = BCOLON;  /*':' as default*/
static char   bcomment  = BCOMMENT; /* '#' */
static char   pefmax    = MAXPEF;   /* 20 for Sahalee, 30 for miniBMC */
static int    nerrs     = 0;   
static int    ngood     = 0;   
static int    lasterr   = 0;   
static uchar  nusers    = 5;   
static uchar  max_users = 5;   
static uchar  enabled_users = 0;   
static uchar  last_user_enable = 0; /* last user enabled */
static uchar  passwordData[16];
static uchar  fsetifn   = 0;   
static ushort setsolcmd;
static ushort getsolcmd;
static uchar  sol_bchan = 0; 
static uchar  authmask = 0;      
static uchar  lan_access = 0x04;  /* see SetPassword*/

static uchar pefnum     = 12;
static uchar fsharedMAC = 0;
//static uchar alertnum   = 1;
//static uchar rgdestip[4];
static uchar  rgdestmac[6];
static uchar  rggwymac[6];
#ifdef WIN32
static uchar  rggwyip[4]   = {0,0,0,0};
static uchar  rgmyip [4]   = {0,0,0,0};  /*WIN32*/
static uchar  rgmymac[6]  = {0xFF,0,0,0,0,0};  /*WIN32*/
static uchar  rgsubnet[4]  = {0,0,0,0};  /*WIN32*/
static uchar  osmyip[4]    = {0,0,0,0};
static uchar  osmymac[6]   = {0xff,0,0,0,0,0};
#endif
static uchar  bmcmyip[4]   = {0,0,0,0};
static uchar  bmcdestip[4] = {0,0,0,0};
static uchar  bmcmymac[6]  = {0xFF,0,0,0,0,0};
static char   ifname[16]    = "eth0"; /* interface name */
static char   ifname0[16]   = "eth0"; /* first interface name */
static char   ifpattn[14]   = "eth";  /* default, discovered via find_ifname */
static int    vend_id;
static int    prod_id;
static uchar  ser_ch        = 0;  /*was SER_CH==4*/
static uchar  lan_ch        = LAN_CH; 
static uchar  lan_ch_parm   = 0xff;
static uchar  lan_ch_sav    = 0xff; 
static uchar  gcm_ch        = 0;
static uchar  SessInfo[16];
static uchar  chan_type[MAXCHAN]; 
static int    nlans = 0;
#define MAX_PEFPARAMS  14	/* max pef params = 14 */
static char **pefdesc;
static char *pefdesc1[MAXPEF] = {    /* for Sahalee BMC */
/* 0 0x00 */ "",
/* 1 0x01 */ "Temperature Sensor",
/* 2 0x02 */ "Voltage Sensor",
/* 3 0x04 */ "Fan Failure",
/* 4 0x05 */ "Chassis Intrusion",
/* 5 0x08 */ "Power Supply Fault",
/* 6 0x0c */ "Memory ECC Error",
/* 7 0x0f */ "FRB Failure",
/* 8 0x07 */ "BIOS POST Error",
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
/*30 */ "unused" };

static char *pefdesc2[MAXPEF] = {    /* for NSC miniBMC */
/* 0 */ "",
/* 1 0x02*/ "Voltage Sensor Assert",
/* 2 0x23*/ "Watchdog FRB Timeout",  /* was "Proc FRB Thermal", */
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
/*30 0x01*/ "Temperature Failure"};

#define NLAN  39
static struct {
  int cmd;
  int sz;
  char desc[28];
} lanparams[NLAN] = {   /* see IPMI Table 19-4 */
 /*  0 */ { 0, 1, "Set in progress"},
 /*  1 */ { 1, 1, "Auth type support"},
 /*  2 */ { 2, 5, "Auth type enables"},
 /*  3 */ { 3, 4, "IP address"},
 /*  4 */ { 4, 1, "IP addr src"},  /* (DHCP/Static) */
 /*  5 */ { 5, 6, "MAC addr"},
 /*  6 */ { 6, 4, "Subnet mask"},
 /*  7 */ { 7, 3, "IPv4 header"},
 /*  8 */ { 8, 2, "Prim RMCP port"},
 /*  9 */ { 9, 2, "Sec RMCP port"},
 /* 10 */ {10, 1, "BMC grat ARP"},
 /* 11 */ {11, 1, "grat ARP interval"},
 /* 12 */ {12, 4, "Def gateway IP"},
 /* 13 */ {13, 6, "Def gateway MAC"},
 /* 14 */ {14, 4, "Sec gateway IP"},
 /* 15 */ {15, 6, "Sec gateway MAC"},
 /* 16 */ {16,18, "Community string"},
 /* 17 */ {17, 1, "Num dest"},
 /* 18 */ {18, 5, "Dest type"},
 /* 19 */ {19, 13, "Dest address"},
 /* 20 */ {20, 2,  "VLAN ID"},
 /* 21 */ {21, 1,  "VLAN Priority"},
 /* 22 */ {22, 1,  "Cipher Suite Support"},
 /* 23 */ {23,17,  "Cipher Suites"},
 /* 24 */ {24, 9,  "Cipher Suite Privilege"},
 /* 25 */ {25, 4,  "VLAN Dest Tag"},
 /* 26 */ {96, 28, "OEM Alert String"},
 /* 27 */ {97,  1, "Alert Retry Algorithm"},
 /* 28 */ {98,  3, "UTC Offset"},
 /* 29 */ {102, 1, "IPv6 Enable"},
 /* 30 */ {103, 1, "IPv6 Addr Source"},
 /* 31 */ {104,16, "IPv6 Address"},
 /* 32 */ {105, 1, "IPv6 Prefix Len"},
 /* 33 */ {106,16, "IPv6 Default Gateway"},
 /* 34 */ {108,17, "IPv6 Dest address"},
 /* 35 */ {192, 4, "DHCP Server IP"},
 /* 36 */ {193, 6, "DHCP MAC Address"},
 /* 37 */ {194, 1, "DHCP Enable"},
 /* 38 */ {201, 2, "Channel Access Mode(Lan)"}
};

#define NSER  22   /* max=32 */
static struct {
  int cmd;
  int sz;
  char desc[28];
} serparams[NSER] = {   /* see IPMI Table 20-4 */
 /*  0 */ { 0, 1, "Set in progress"},
 /*  1 */ { 1, 1, "Auth type support"},
 /*  2 */ { 2, 5, "Auth type enables"},
 /*  3 */ { 3, 1, "Connection Mode"},
 /*  4 */ { 4, 1, "Sess Inactiv Timeout"},
 /*  5 */ { 5, 5, "Channel Callback"},
 /*  6 */ { 6, 1, "Session Termination"},
 /*  7 */ { 7, 2, "IPMI Msg Comm"},
 /*  8 */ { 8, 2, "Mux Switch"},
 /*  9 */ { 9, 2, "Modem Ring Time"},
 /* 10 */ {10,17, "Modem Init String"},
 /* 11 */ {11, 5, "Modem Escape Seq"},
 /* 12 */ {12, 8, "Modem Hangup Seq"},
 /* 13 */ {13, 8, "Modem Dial Command"},
 /* 14 */ {14, 1, "Page Blackout Interval"},
 /* 15 */ {15,18, "Community String"},
 /* 16 */ {16, 1, "Num of Alert Dest"},
 /* 17 */ {17, 5, "Destination Info"},
 /* 18 */ {18, 1, "Call Retry Interval"},
 /* 19 */ {19, 3, "Destination Comm Settings"},
 /* 20 */ {29, 2, "Terminal Mode Config"},
 /* 21 */ {201, 2,"Channel Access Mode (Ser)"}
};

#define NSYS  6   /* num SystemParam */
#define CHAS_RESTORE   0x00  /*chassis power restore policy*/
#define SYS_INFO       0x01  /*System Info (1,2,3,4) */
#define LAN_FAILOVER   0x02  /*Intel LAN Failover*/
/* TODO: Future DCMI 1.5 params */
#define DCMI_POWER     0x03  /*DCMI Power limit*/
#define DCMI_THERMAL   0x04  /*DCMI Thermal params*/
#define DCMI_CONFIG    0x05  /*DCMI Config params*/

static int GetDeviceID(uchar *pLanRecord)
{  /*See also ipmi_getdeviceid( pLanRecord, sizeof(LAN_RECORD),fdebug); */
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	int status;
	uchar inputData[24];
	uchar completionCode;

	if (pLanRecord == NULL) return(-1);

        status = ipmi_cmd(GET_DEVICE_ID, inputData, 0, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
			SELprintf("GetDeviceID: completion code=%x\n", 
				completionCode); 
			status = completionCode;
		} else {
			memcpy(pLanRecord,&responseData[0],responseLength);
			set_mfgid(&responseData[0],responseLength);
			return(0);  // successful, done
		}
	}  /* endif */
	/* if get here, error */
 	return(status);
}  /*end GetDeviceID() */

static int GetChanAcc(uchar chan, uchar parm, uchar *pLanRecord)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	int status;
	uchar inputData[24];
	uchar completionCode;

	if (pLanRecord == NULL) return(-1);
	responseLength = 3;
	inputData[0] = chan;
	inputData[1] = parm;  /* 0x80 = active, 0x40 = non-volatile */
	responseLength = sizeof(responseData);
        status = ipmi_cmd(GET_CHANNEL_ACC, inputData, 2, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
			SELprintf("%c GetChanAcc: completion code=%x\n", 
				bcomment,completionCode); 
			status = completionCode;
		} else {
			// dont copy first byte (Parameter revision, usu 0x11)
			memcpy(pLanRecord,&responseData[0],responseLength);
			return(0);  // successful, done
		}
	}  /* endif */
	/* if get here, error */
 	return(status);
}  /*GetChanAcc()*/

static int SetChanAcc(uchar chan, uchar parm, uchar val, uchar access)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	int status;
	uchar inputData[24];
	uchar completionCode;

	if (fmBMC) return(0);  /* mBMC doesn't support this */
        /* parm: 0x80 = active, 0x40 = set non-vol*/
	responseLength = 1;
	inputData[0] = chan;  /* channel */
	inputData[1] = (parm & 0xc0) | (val & 0x3F); 
	inputData[2] = (parm & 0xc0) | access; /* set priv level */

	responseLength = sizeof(responseData);
        status = ipmi_cmd(SET_CHANNEL_ACC, inputData, 3, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
			SELprintf("SetChanAcc: completion code=%x\n", 
				completionCode); 
			status = completionCode;
		} else {
			return(0);  // successful, done
		}
	}  /* endif */
	/* if get here, error */
 	return(status);
}  /*SetChanAcc()*/

static int GetUser(int user_num)
{
      uchar responseData[MAX_BUFFER_SIZE];
      int responseLength = MAX_BUFFER_SIZE;
      int status, i;
      uchar completionCode;
      char  inputData[24];

         inputData[0] = lan_ch;
         inputData[1] = (uchar)user_num;  /* usually = 1 for BMC LAN */
	 responseLength = sizeof(responseData);
         status = ipmi_cmd(GET_USER_ACCESS, inputData, 2, responseData,
                        &responseLength, &completionCode, fdebug);
	 if (status == 0 && completionCode == 0) {
	    uchar c;
	    if (user_num == 1) {
                max_users = responseData[0] & 0x3f;
                enabled_users = responseData[1] & 0x3f;
                if (enabled_users > nusers) nusers = enabled_users;
	    }
            fprintf(fd_bmc,"UserAccess %d,%d%c %02x %02x %02x %02x \n",
			lan_ch,user_num,bdelim,responseData[0],responseData[1],
			responseData[2], responseData[3]);
	    c = responseData[3];
            inputData[0] = (uchar)user_num;  /* usually = 1 for BMC LAN */
	    responseLength = sizeof(responseData);
            status = ipmi_cmd(GET_USER_NAME, inputData, 1, responseData, 
	 	        &responseLength, &completionCode, fdebug);
	    if (status != 0 || completionCode != 0) 
               responseData[0] = 0;
	    else {
               fprintf(fd_bmc,"UserName     %d%c",user_num,bdelim);
	       for (i=0; i< responseLength; i++) 
                   fprintf(fd_bmc," %02x",responseData[i]);
               fprintf(fd_bmc,"\n");
               fprintf(fd_bmc,"%c UserPassword %d%c",bcomment,user_num,bdelim);
	       for (i=0; i< PSW_LEN; i++) 
                   fprintf(fd_bmc," 00");
               fprintf(fd_bmc,"\n");
	    }
	 } else 
            printf("%c GetUserAccess(%d,%d), status=%x, ccode=%x\n",
			bcomment,lan_ch,user_num, status, completionCode);
	return(status);
}  /*end GetUser()*/

static int GetSerEntry(uchar subfunc, uchar bset, uchar *pLanRecord)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24];
	int status;
	uchar completionCode;
	uchar chan; 

	if (pLanRecord == NULL)
	{
	   if (fdebug)
	      printf("GetSerEntry(%d): error, output buffer is NULL\n",subfunc);
 	   return (-1);
	}

        chan = ser_ch;  /* 1=EMP, 0=IPMB, 6=LAN2, 7=LAN1 */

	inputData[0]            = chan;  // flags, channel 3:0 (1=EMP)
	inputData[1]            = subfunc;  // Param selector 
	inputData[2]            = bset;  // Set selector 
	inputData[3]            = 0;  // Block selector
	if (subfunc == 10)  {
	   inputData[2] = 0;
	   inputData[3] = 1;
 	}

        status = ipmi_cmd(GET_SER_CONFIG, inputData, 4, responseData,
                        &responseLength, &completionCode, fdebug); 
	if (status == ACCESS_OK) {
		if( completionCode ) {
			if (fdebug) 
			   SELprintf("GetSerEntry(%d,%d): completion code=%x\n",
				chan,subfunc,completionCode); 
			status = completionCode;
		} else {
			// dont copy first byte (Parameter revision, usu 0x11)
			memcpy(pLanRecord,&responseData[1],responseLength-1);
			pLanRecord[responseLength-1] = 0;
			//successful, done
			return(0);
		}
	}

	// we are here because completionCode is not COMPLETION_CODE_OK
	if (fdebug) 
		SELprintf("GetSerEntry(%d,%d): ipmi_cmd status=%x ccode=%x\n",
                          chan,subfunc,status,completionCode);
 	return status;
}

static int SetSerEntry(uchar subfunc, uchar *pSerRecord, int reqlen)
{
        uchar responseData[MAX_BUFFER_SIZE];
        int responseLength = MAX_BUFFER_SIZE;
        uchar inputData[24];
        int status;
        uchar completionCode;
 
        if (pSerRecord == NULL) {
           if (fdebug)
               printf("SetSerEntry(%d): error, input buffer is NULL\n",
                      subfunc);
           return (-1);
        }
        inputData[0]            = ser_ch;  // flags, channel 3:0 (EMP)
        inputData[1]            = subfunc;  // Param selector
        memcpy(&inputData[2],pSerRecord,reqlen);
        status = ipmi_cmd(SET_SER_CONFIG, inputData, (uchar)(reqlen+2),
                        responseData, &responseLength, &completionCode, fdebug);

        if (fdebug)
             printf("SetSerEntry: ipmi_cmd status=%d, completion code=%d\n",
                          status,completionCode);
        if (status == ACCESS_OK) {
                if( completionCode ) {
		   status = completionCode; 
                   if (completionCode == 0x80)
                        printf("SetSerEntry(%d): Parameter not supported\n",
                                  subfunc);
                   else
                        printf("SetSerEntry(%d): completion code=%x\n",
                                subfunc, completionCode);
                } 
		// else  successful, done
        }
        return status;
}  /* end SetSerEntry() */  

static int GetLanEntry(uchar subfunc, uchar bset, uchar *pLanRecord)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24];
	int status, n;
	uchar completionCode;
	uchar chan; 

	if (pLanRecord == NULL) {
	   if (fdebug) printf("GetLanEntry: error, output buffer is NULL\n");
 	   return (-1);
	}

        chan = lan_ch;  /* LAN 1 = 7 */

	inputData[0]            = chan;  // flags, channel 3:0 (LAN 1)
	inputData[1]            = subfunc;  // Param selector (3 = ip addr)
	inputData[2]            = bset;  // Set selector 
	inputData[3]            = 0;  // Block selector

        status = ipmi_cmd(GET_LAN_CONFIG, inputData, 4, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
		    if (fdebug)
			SELprintf("GetLanEntry(%d,%d): completion code=%x\n", 
				chan,subfunc,completionCode);
		    status = completionCode;
		} else {
			// dont copy first byte (Parameter revision, usu 0x11)
                        if (responseLength > 0) {
                           n = responseLength-1;
                           memcpy(pLanRecord,&responseData[1],n);
                        } else n = 0;
			pLanRecord[n] = 0;
			//successful, done
			return(0);
		}
	}

	// we are here because completionCode is not COMPLETION_CODE_OK
	if (fdebug) 
		SELprintf("GetLanEntry(%d,%d): status=%d completionCode=%x\n",
                          chan,subfunc,status,completionCode);
 	return (status);
}  /* end GetLanEntry() */

static int SetLanEntry(uchar subfunc, uchar *pLanRecord, int reqlen)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24];
	int status;
	uchar completionCode;

	if (pLanRecord == NULL)
	{
	   if (fdebug)
	       printf("SetLanEntry(%d): error, input buffer is NULL\n",subfunc);
 	   return (-1);
	}

        if ((vend_id == VENDOR_SUPERMICROX) ||
            (vend_id == VENDOR_SUPERMICRO)) {
	   /* SUPERMICRO cannot set grat arp or grat arp interval */
	   if (subfunc == 10 || subfunc == 11) return(0);
	}
	inputData[0]      = lan_ch;  // flags, channel 3:0 (LAN 1)
	inputData[1]      = subfunc;  // Param selector (3 = ip addr)
	memcpy(&inputData[2],pLanRecord,reqlen);

        status = ipmi_cmd(SET_LAN_CONFIG, inputData, (uchar)(reqlen+2), 
			responseData, &responseLength,&completionCode,fdebug);

	if (status == ACCESS_OK) {
		if( completionCode ) {
		    if (fdebug)
			SELprintf("SetLanEntry(%d,%d): completion code=%x\n", 
				lan_ch,subfunc,completionCode); 
                        return(completionCode);
		} else {
			//successful, done
			return(0);
		}
	}

	// we are here because completionCode is not COMPLETION_CODE_OK
	if (fdebug) 
	    SELprintf("SetLanEntry(%d,%d): ipmi_cmd status=%d ccode=%x\n",
                          lan_ch,subfunc,status,completionCode);
 	return (status);
}  /* end SetLanEntry() */

static int GetPefEntry(uchar subfunc, ushort rec_id, PEF_RECORD *pPefRecord)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24];
	int status, n;
	uchar completionCode;

	if (pPefRecord == NULL)
	{
	   if (fdebug)
	      printf("GetPefEntry(%d): error, output buffer is NULL\n",subfunc);
 	   return (-1);
	}

	inputData[0]            = subfunc; // Parameter = Evt Filter Table
	inputData[1]            = (uchar)rec_id; 
	inputData[2]            = 0; 

        status = ipmi_cmd(GET_PEF_CONFIG, inputData, 3, responseData,
                        &responseLength, &completionCode, fdebug); 

        if (status == ACCESS_OK) {
                if( completionCode ) {
		    if (fdebug)
                        SELprintf("GetPefEntry(%d/%d): completion code=%x\n",
                                subfunc,rec_id,completionCode); 
		    status = completionCode;
                } else {
			/* expect PEF record to be >=21 bytes */
			if (responseLength > 1) n = responseLength-1;
			else n = 0;
			if (n > 21) n = 21;
                        // dont copy first byte (Parameter revision, usu 0x11)
			if (n == 0) memset(pPefRecord,0,21); 
			else memcpy(pPefRecord,&responseData[1],n);
                        //successful, done
                        return(0);
                }
	}

	// we are here because completionCode is not COMPLETION_CODE_OK
	if (fdebug) 
	      SELprintf("GetPefEntry: ipmi_cmd status=%x completionCode=%x\n",
                         status, completionCode);
 	return status;
}  /* end GetPefEntry() */

static int SetPefEntry(PEF_RECORD *pPefRecord)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24]; /* sizeof(PEF_RECORD) = 21 +1=22 */
	int status;
	uchar completionCode;
	uchar subfunc;

	subfunc = 0x06; // Parameter = Evt Filter Table

	if (pPefRecord == NULL) {
	   if (fdebug)
	       printf("SetPefEntry: error, output buffer is NULL\n");
 	   return (-1);
	}

        //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21
        // 06 0c 80 01 01 00 ff ff 20 ff 6f ff 00 00 00 00 00 00 00 00 00 00
	// memset(&inputData[0],0,requestData.dataLength);
	inputData[0]            = subfunc; 
	memcpy(&inputData[1],pPefRecord,sizeof(PEF_RECORD));

        status = ipmi_cmd(SET_PEF_CONFIG, inputData, sizeof(PEF_RECORD)+1, 
		       responseData, &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
		    if (fdebug)
			SELprintf("SetPefEntry: completion code=%x\n", 
				completionCode); // responseData[0]);
		    status = completionCode;
		} else {
			//successful, done
			return(0);
		}

	}

	// we are here because completionCode is not COMPLETION_CODE_OK
	if (fdebug) 
		SELprintf("SetPefEntry: ipmi_cmd status=%d ccode=%x\n",
                          status,completionCode);
 	return(status);
 
}  /* end SetPefEntry() */

static int MacIsValid(uchar *mac)
{
   int fvalid = 0;
   int i;
   /* check for initial invalid value of FF:00:... */
   if (mac[0] == 0xFF && mac[1] == 0x00)  /* marked as invalid */
	return(fvalid);
   /* check for all zeros */
   for (i = 0; i < MAC_LEN; i++)
	if (mac[i] != 0) {  /* not all zeros */
	    fvalid = 1;
	    break;
	}
   return(fvalid);
}

static int IpIsValid(uchar *ipadr)
{
   int fvalid = 1;
   if (ipadr[0] == 0) fvalid = 0;
   return(fvalid);
}

static int SubnetIsValid(uchar *subnet)
{
   int fvalid = 0;
   /* if masking off at least one bit, say valid */
   if ((subnet[0] & 0x80) == 0x80) fvalid = 1;
   return(fvalid);
}

#ifdef WIN32
/*
 * Obtain network adapter information (Windows).
 */
static PIP_ADAPTER_ADDRESSES GetAdapters() {
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
        RetVal = GetAdaptersAddresses(
                AF_INET, 0, NULL, 
                AdapterAddresses, 
                &OutBufferLength);
        
        if (RetVal != ERROR_BUFFER_OVERFLOW) {
            break;
        }

        if (AdapterAddresses != NULL) {
            free(AdapterAddresses);
        }
        
        AdapterAddresses = (PIP_ADAPTER_ADDRESSES) malloc(OutBufferLength);
        if (AdapterAddresses == NULL) {
            RetVal = GetLastError();
            break;
        }
    }
    if (RetVal == NO_ERROR) {
      // If successful, return pointer to structure
        return AdapterAddresses;
    }
    else { 
      LPVOID MsgBuf;
      
      printf("Call to GetAdaptersAddresses failed.\n");
      if (FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        RetVal,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &MsgBuf,
        0,
        NULL )) {
        printf("\tError: %s", MsgBuf);
      }
      LocalFree(MsgBuf);
    }  
    return NULL;
}

/*
 * Set BMC MAC corresponding to current BMC IP address (Windows).
 */
static int GetLocalMACByIP() {
    PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL;
    PIP_ADAPTER_ADDRESSES AdapterList;
    int result = 0;

    struct sockaddr_in *si;
    
    AdapterAddresses = GetAdapters();
    AdapterList = AdapterAddresses;
    
    while (AdapterList) {
        PIP_ADAPTER_UNICAST_ADDRESS addr;
        addr = AdapterList->FirstUnicastAddress;
	if (addr == NULL) si = NULL;
        else si = (struct sockaddr_in*)addr->Address.lpSockaddr;
	if (si != NULL) {
           if(memcmp(&si->sin_addr.s_addr, rgmyip, 4) == 0) {
              memcpy(rgmymac, AdapterList->PhysicalAddress, MAC_LEN);
              result = 1;
              break;
           }
        }
        AdapterList = AdapterList->Next;
    }   

    if (AdapterAddresses != NULL) {
        free(AdapterAddresses);
    }
    return result;
}

/*
 * Set BMC MAC corresponding to current BMC IP address (Windows).
 */
static int GetLocalIPByMAC(uchar *macadr) {
    PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL;
    PIP_ADAPTER_ADDRESSES AdapterList;
    int result = 0;

    struct sockaddr_in *si;
    
    AdapterAddresses = GetAdapters();
    AdapterList = AdapterAddresses;
    
    while (AdapterList) {
        PIP_ADAPTER_UNICAST_ADDRESS addr;
        if(memcmp(AdapterList->PhysicalAddress, macadr, MAC_LEN) == 0) {
            addr = AdapterList->FirstUnicastAddress;
            
            si = (struct sockaddr_in*)addr->Address.lpSockaddr;
	    if (fdebug) {
		uchar *psaddr;
		psaddr = (uchar *)&si->sin_addr.s_addr;
		printf("mac match: rgmyip=%d.%d.%d.%d s_addr=%d.%d.%d.%d\n",
			rgmyip[0], rgmyip[1], rgmyip[2], rgmyip[3],
			psaddr[0], psaddr[1], psaddr[2], psaddr[3]);
	    }
 	    if (!IpIsValid(rgmyip) && (fsharedMAC==1)) /*not specified, shared*/
		memcpy(rgmyip, &si->sin_addr.s_addr, 4);
            memcpy(osmyip, &si->sin_addr.s_addr, 4);
	    memcpy(osmymac, AdapterList->PhysicalAddress, MAC_LEN);
            wcstombs(ifname,AdapterList->FriendlyName, sizeof(ifname));
            result = 1;
            break;
        }
        AdapterList = AdapterList->Next;
    }   

    if (AdapterAddresses != NULL) {
        free(AdapterAddresses);
    }
    return result;
}

/*
 * Set MAC and IP address from given interface name (Windows).
 */
static int GetLocalDataByIface() {
    PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL;
    PIP_ADAPTER_ADDRESSES AdapterList;
    int result = 0;

    size_t origsize, newsize, convertedChars;
    wchar_t* wcstring;
    struct sockaddr_in *si;
    
    AdapterAddresses = GetAdapters();
    AdapterList = AdapterAddresses;
    
    origsize = strlen(ifname) + 1;
    newsize = origsize;
    convertedChars = 0;
    wcstring = (wchar_t*) malloc(sizeof(wchar_t) * newsize)  ;
    if (wcstring == NULL) AdapterList = NULL;  /*skip loop, do free*/
    else mbstowcs(wcstring, ifname, origsize );
    
    while (AdapterList) {
        PIP_ADAPTER_UNICAST_ADDRESS addr;
        if(wcsstr(AdapterList->FriendlyName, wcstring)) {
            printf("Using interface: %S\n", AdapterList->FriendlyName);
            printf("\t%S\n", AdapterList->Description);
            addr = AdapterList->FirstUnicastAddress;
            
            si = (struct sockaddr_in*)addr->Address.lpSockaddr;
            memcpy(rgmyip, &si->sin_addr.s_addr, 4);
            memcpy(rgmymac, AdapterList->PhysicalAddress, MAC_LEN);
            
            result = 1;
            break;
        }
        AdapterList = AdapterList->Next;
    }   

    if (AdapterAddresses != NULL) {
        free(AdapterAddresses);
    }
    return result;
}

static int FindEthNum(uchar *macadrin)
{
   int i;
   uchar macadr[MAC_LEN];
   memcpy(macadr,macadrin,MAC_LEN);
   if (fsharedMAC == 0 && vend_id == VENDOR_INTEL)  {
        /* Intel factory assigns them this way, so use that to compare */
        macadr[MAC_LEN-1] -= 2; /*OS MAC = BMC MAC - 2*/
   }
   i = GetLocalIPByMAC(macadr);
   if (fdebug)  /* show the local OS eth if and MAC */
        printf("FindEth: OS %s IP=%d.%d.%d.%d MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
                ifname, rgmyip[0], rgmyip[1], rgmyip[2], rgmyip[3],
                rgmymac[0], rgmymac[1], rgmymac[2], rgmymac[3],
                rgmymac[4], rgmymac[5]);
   /* The actual Windows ethernet interface is determined
    * in Get_IPMac_Addr using ipconfig, so
    * init eth interface number as eth0 for Windows. */
   return(i);
}
#elif defined(HPUX)
static int FindEthNum(uchar *macadrin)
{
   return(0);
}
#else
/* Linux, BSD, Solaris */
static char *get_ifreq_mac(struct ifreq *ifrq)
{
    char *ptr;
#ifdef SOLARIS
    ptr = (char *)&ifrq->ifr_ifru.ifru_enaddr[0];
#elif BSD
    ptr = (char *)&ifrq->ifr_ifru.ifru_addr.sa_data[0];
#elif MACOS
    static uchar mactmp[MAC_LEN];
    ptr = &mactmp[0];
    MacSetInvalid(ptr);
#else
    ptr = (char *)&ifrq->ifr_hwaddr.sa_data[0];
#endif
    return (ptr);
}


static int FindEthNum(uchar *macadrin)
{                     /*only used for Linux*/
    struct ifreq ifr;
    int skfd;
    int nCurDevice;
    uchar macadr[MAC_LEN];
    char szDeviceName[ 16 ];  /* sizeof(ifpattn), MAX_DEVICE_NAME_LENGTH + 1 */
    int devnum = -1;
    int n;

    memcpy(macadr,macadrin,MAC_LEN);
    if (fsharedMAC == 0 && vend_id == VENDOR_INTEL)  {
	/* Intel factory assigns them this way, so use that to compare */
	macadr[MAC_LEN-1] -= 2; /*OS MAC = BMC MAC - 2*/
    }
    n = find_ifname(szDeviceName);
    if (n >= 0) {
	n = strlen_(szDeviceName);
	if (n < sizeof(ifpattn)) {
           strcpy(ifname0,szDeviceName);
           strcpy(ifpattn,szDeviceName);
           ifpattn[n - 1] = 0;  /*truncate last digit*/
	}
	if (fdebug) 
	   printf("found ifname %s, pattern %s\n",szDeviceName,ifpattn);
    }

    if ( ( skfd = socket(AF_INET, SOCK_DGRAM, 0 ) ) < 0) {
       if ( fdebug ) {
          perror("socket");
          return devnum;
       }
    }
	    
    for( nCurDevice = 0 ;
    	 (nCurDevice < NUM_DEVICES_TO_CHECK) && (devnum == -1);
	 nCurDevice++ )
    {
        sprintf( szDeviceName, "%s%d", ifpattn, nCurDevice );
        strcpy(ifr.ifr_name, szDeviceName );
#ifdef SIOCGIFHWADDR
        if (ioctl(skfd, SIOCGIFHWADDR, &ifr) > 0) {
            if ( fdebug ) 
                printf("FindEthNum: Could not get MAC address for %s\n",
			 szDeviceName);
        } else 
#endif
	{
            if (memcmp(get_ifreq_mac(&ifr), macadr, MAC_LEN) == 0) {
                devnum = nCurDevice;
                break;
            }
        }
    }
    close(skfd);
    return(devnum);
}
#endif

/*
 * GetBmcEthDevice
 * Attempt to auto-detect the BMC LAN channel and matching OS eth port.
 * INPUT:  lan_parm = lan channel from user -L option, 0xFF if not specified
 * OUTPUT: lan_ch is set to BMC LAN channel number
 *         if success, returns index of OS eth port (0, 1, ...). 
 *         if no lan channels found, returns -2.
 *         if other error, returns -1.
 */
static int GetBmcEthDevice(uchar lan_parm)
{
    uchar LanRecord[30];
    int devnum = -1;
    int ret;
    uchar bmcMacAddress[ MAC_LEN ];  /*MAC_LEN = 6*/
    int rlen;
    uchar iData[2];
    uchar rData[10];
    uchar cc;
    int i = 0;
    int j, jstart, jend, jlan;
    uchar mtype;
    uchar *pb;
    int fchgmac;

    /* Find the LAN channel(s) via Channel Info */
    if (lan_parm < MAXCHAN) {  /* try user-specified channel only */
        lan_ch = lan_parm;
	jstart = lan_parm;
	jend = lan_parm+1; 
    } else {
	jstart = 1;
	jend = MAXCHAN; 
        for (j = 0; j < MAXCHAN; j++) chan_type[j] = 0;
    }
    memset(bmcMacAddress,0xff,sizeof(bmcMacAddress)); /*initialize to invalid*/
    for (j = jstart; j < jend; j++) {
        rlen = sizeof(rData);
        iData[0] = (uchar)j; /*channel #*/
	memset(rData,0,9); /*initialize recv data*/
        ret = ipmi_cmd(GET_CHANNEL_INFO, iData, 1, rData, &rlen, &cc, fdebug); 
	if (ret == 0xcc || cc == 0xcc) /* special case for ipmi_lan */
		continue;
	if (ret != 0) {
    		if (fdebug) printf("get_chan_info rc = %x\n",ret);
		break;
	}
	mtype = rData[1];  /* channel medium type */
	chan_type[j] = mtype;
	if (mtype == 4) {  /* 802.3 LAN type*/
		if (fdebug) printf("chan[%d] = lan\n",j);
		jlan = lan_ch;  /*save prev lan chan */
		/* Get BMC MAC for this LAN channel. */
		/* Note: BMC MAC may not be valid yet. */
		lan_ch = (uchar)j;   /*set lan channel for GetLanEntry()*/
		ret = GetLanEntry( 5 /*MAC_ADDRESS_LAN_PARAM*/,0, LanRecord);
		if ( ret < 0 ) {
		   lan_ch = (uchar)jlan;  /*restore lan_ch*/
		   printf( "GetBmcEthDevice: GetLanEntry failed\n" );
		   return devnum;
		}
		pb = &LanRecord[0];
		if (fdebug) printf("chan[%d] BMC MAC %x:%x:%x:%x:%x:%x\n",j,
				   pb[0], pb[1], pb[2], pb[3], pb[4], pb[5] );
		fchgmac = 0;
		if (!MacIsValid(bmcMacAddress)) /* old MAC not valid */
			fchgmac = 1; 
		else if (MacIsValid(pb) &&   /* new MAC is valid and */
		      (memcmp(bmcMacAddress,pb, sizeof(bmcMacAddress)) > 0))
			fchgmac = 1;   /* new MAC lower */
		if (fchgmac) {    /* use lowest valid MAC */
		   memcpy(bmcMacAddress,pb,sizeof(bmcMacAddress));
		   lan_ch = (uchar)j;
		} else lan_ch = (uchar)jlan;  /* restore prev lan chan */
		i++;  /* i = num lan channels found */
	} else if (mtype == 5) { /* serial type*/
		if (fdebug) printf("chan[%d] = serial\n",j);
		ser_ch = (uchar)j;    /* set to last serial channel */
	} else if (mtype == 7) { /* PCI SMBus */
		if (fdebug) printf("chan[%d] = pci_smbus\n",j);
	} else if (mtype == 12) { /* system interface */
		if (fdebug) printf("chan[%d] = system_interface\n",j);
	} else  /* other channel medium types, see IPMI 1.5 Table 6-3 */
		if (fdebug) printf("chan[%d] = %d\n",j,mtype);
    }
    nlans = i;
    if (i == 0) return(-2);  /* no lan channels found */
    if (fdebug) printf("lan_ch detected = %d\n",lan_ch);

    devnum = FindEthNum(bmcMacAddress);
    if ( fdebug ) 
	printf("GetBmcEthDevice: channel %d, %s%d\n",lan_ch,ifpattn,devnum);
    return devnum;
}

/*
 * atomac - converts ASCII string to binary MAC address (array).
 * Accepts input formatted as 11:22:33:44:55:66 or 11-22-33-44-55-66.
 */
static void atomac(uchar *array, char *instr)
{
   int i,j,n;
   char *pi;
   j = 0;
   pi = instr;
   n = strlen_(instr);
   for (i = 0; i <= n; i++) {
      if (instr[i] == ':') {
	array[j++] = htoi(pi);
	pi = &instr[i+1];
      } else if (instr[i] == '-') {
	array[j++] = htoi(pi);
	pi = &instr[i+1];
      } else if (instr[i] == 0) {
	array[j++] = htoi(pi);
	}
      if (j >= MAC_LEN) break; /*safety valve*/
   }
   if (fdebug) 
      printf("atomac: %02x %02x %02x %02x %02x %02x\n", 
         array[0],array[1],array[2],array[3], array[4],array[5]);
}  /*end atomac()*/

/* file_grep/findmatch no longer used here, see ievents.c */

/* 
 * Get_Mac
 * This routine finds a MAC address from a given IP address.
 * Usually for the Alert destination.
 * It uses ARP cache to do this.
 */
#ifdef WIN32
static int Get_Mac(uchar *ipadr,uchar *macadr)
{
    DWORD dwRetVal;
    IPAddr DestIp = 0;
    IPAddr SrcIp = 0;       /* default for src ip */
    ULONG MacAddr[2];       /* for 6-byte hardware addresses */
    ULONG PhysAddrLen = MAC_LEN;  /* default to length of six bytes */
    BYTE *bPhysAddr;


    memcpy(&DestIp, ipadr, 4);
    
    /* invoke system ARP query */
    dwRetVal = SendARP(DestIp, SrcIp, MacAddr, &PhysAddrLen);

    if (dwRetVal == NO_ERROR) 
    { /* no error - get the MAC */
        bPhysAddr = (BYTE *) & MacAddr;
        if (PhysAddrLen) {
            memcpy(macadr, bPhysAddr, MAC_LEN);
        } else
            printf("Warning: SendArp completed successfully, but returned length=0\n");
    } else if (dwRetVal == ERROR_GEN_FAILURE) 
    { /* MAC not available in this netowork - get gateway MAC */ 
        memcpy(macadr, rggwymac, MAC_LEN);
    } else 
    { /* other errors */
        printf("Error: SendArp failed with error: %d", dwRetVal);
        switch (dwRetVal) {
        case ERROR_INVALID_PARAMETER:
            printf(" (ERROR_INVALID_PARAMETER)\n");
            break;
        case ERROR_INVALID_USER_BUFFER:
            printf(" (ERROR_INVALID_USER_BUFFER)\n");
            break;
        case ERROR_BAD_NET_NAME:
            printf(" (ERROR_GEN_FAILURE)\n");
            break;
        case ERROR_BUFFER_OVERFLOW:
            printf(" (ERROR_BUFFER_OVERFLOW)\n");
            break;
        case ERROR_NOT_FOUND:
            printf(" (ERROR_NOT_FOUND)\n");
            break;
        default:
            printf("\n");
            break;
        }
        return 1;
    }
    return 0;
}  /* end Get_Mac()*/
/*endif WIN32 */
#else
/*else Linux */
static int Get_Mac(uchar *ipadr,uchar *macadr)
{
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

   if (strcmp(ifname,"gcm") == 0) _ifname = ifname0;
   else _ifname = ifname;

   /* Get a MAC address for a given IP address */
   if (ipadr[0] != 0) {   /* if valid IP address */

      /* make sure the destination is in the arp cache */
      sprintf(arping_cmd,
	      "arping -I %s -c 2 %d.%d.%d.%d |grep reply |tail -n1 >%s\n",
              _ifname,ipadr[0],ipadr[1],ipadr[2],ipadr[3],alertfile);
      if (fdebug) printf("%s", arping_cmd);
      i = system(arping_cmd);

      fparp = fopen(alertfile,"r");
      if (fparp == NULL) {
          fprintf(stdout,"Get_Mac: Cannot open %s, errno = %d\n",
			alertfile,get_errno());
          ret = -1;
       } else {
         while (fgets(buff, 1023, fparp)) {
	   /* should only run through loop once */
	   num = strcspn(buff," \t");    /* skip 1st word ("Unicast") */
	   i = strspn(&buff[num]," \t");
	   pb = &buff[num+i];
  	   if (strncmp(pb,"reply",5) == 0) {  /* valid output */
	      /* Find the ip address */
	      pb += 6 + 5;         /* skip "reply from " */
	      num = strcspn(pb," \t");
	      pb[num] = 0;
	      if (fdebug) printf("Alert ip=%s\n",pb);
	      /* IP address should already match input param */
 	      /* if (rgdestip[0] == 0) atoip(rgdestip,pb);  */
	      /* Now find the mac address */
	      pm = strchr(&pb[num+1],'[');
	      if (pm == NULL) pm = &pb[num+2];  /* just in case */
	      pm++;
	      px = strchr(pm,']');
	      if (px == NULL) px = pm + 17;    /* just in case */
	      px[0] = 0;
	      if (fdebug) printf("Alert mac=%s\n",pm);
	      foundit = 1;
 	      if (!MacIsValid(macadr)) atomac(macadr,pm);
	      break;
	   }
         } /*end while*/
         fclose(fparp);
       }  /*end else file opened*/
   }  /*endif valid IP */
   else ret = -1;

   if (ret == -1 || foundit == 0) {  /* couldn't get it */
      if (MacIsValid(rggwymac) && !MacIsValid(rgdestmac))  
         memcpy(rgdestmac,rggwymac,6);  /* get to it from the default gateway */
      }
   return(ret);
}  /* end Get_Mac()*/
/*end else Linux*/
#endif

#ifdef WIN32

/*
 * Set subnet mask based on current IP address (Windows).
 */
static int SetSubnetMask() {
    PMIB_IPADDRTABLE pIPAddrTable;
    unsigned int i;
    DWORD dwSize = 0, dwRetVal;
    LPVOID lpMsgBuf;

    pIPAddrTable = (MIB_IPADDRTABLE*) malloc( sizeof( MIB_IPADDRTABLE) );

    if ( pIPAddrTable ) {
        // Make an initial call to GetIpAddrTable to get the
        // necessary size into the dwSize variable
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
            free( pIPAddrTable );
            pIPAddrTable = (MIB_IPADDRTABLE *) malloc ( dwSize );
        }
    } else
        printf("Memory allocation failed.\n");

    if ( pIPAddrTable ) {
        // Make a second call to GetIpAddrTable to get the
        // actual data we want
        if ( (dwRetVal = GetIpAddrTable( pIPAddrTable, &dwSize, 0 )) == NO_ERROR ) { 
	        for(i = 0; i < pIPAddrTable->dwNumEntries; ++i) {
                if(memcmp(&(pIPAddrTable->table[i].dwAddr), rgmyip, 4) == 0) {
                    memcpy(rgsubnet, &(pIPAddrTable->table[i].dwMask), 4);
                    free( pIPAddrTable );
                    return 1;     
                }
            }
        } else {
            if (FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dwRetVal,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &lpMsgBuf,
                0,
                NULL )) {
                printf("\tError: %s", lpMsgBuf);
            }

            printf("Call to GetIpAddrTable failed.\n");
        }
    }

    if ( pIPAddrTable )
        free( pIPAddrTable );

    return 0;
}

/*
 * Extract gateway address from routing table (Windows).
 */
static int SetDefaultGateway() {
    PMIB_IPFORWARDTABLE pIpForwardTable;
    DWORD dwRetVal, dwSize;

    unsigned int nord_mask;
    unsigned int nord_ip;
    unsigned int nord_net;

    unsigned int i;

    nord_mask = *((unsigned int *)rgsubnet);
    nord_ip = *((unsigned int *)rgmyip);

    nord_net = nord_ip & nord_mask;

    pIpForwardTable = (MIB_IPFORWARDTABLE*) malloc(sizeof(MIB_IPFORWARDTABLE));
    if (pIpForwardTable == NULL) {
        printf("Error allocating memory\n");
        return 0;
    }

    dwSize = 0;
    if (GetIpForwardTable(pIpForwardTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
        free(pIpForwardTable);
        pIpForwardTable = (MIB_IPFORWARDTABLE*) malloc(dwSize);
        if (pIpForwardTable == NULL) {
            printf("Error allocating memory\n");
            return 0;
        }
    }

    /* 
     * Note that the IPv4 addresses returned in 
     * GetIpForwardTable entries are in network byte order 
     */
    if ((dwRetVal = GetIpForwardTable(pIpForwardTable, &dwSize, 0)) == NO_ERROR) {
        for (i = 0; i < (int) pIpForwardTable->dwNumEntries; i++) {
            unsigned int gwaddr = pIpForwardTable->table[i].dwForwardNextHop;
            if(nord_net == (gwaddr & nord_mask) && nord_ip != gwaddr) 
            { /* searching for gateways from our network with different address than ours */
                memcpy(rggwyip, &gwaddr, 4);
                return 0;
            }
        }
        free(pIpForwardTable);
        return 1;
    }
    else {
        printf("\tGetIpForwardTable failed.\n");
        free(pIpForwardTable);
        return 0;
    }
    
}
/*endif  WIN32*/
#endif

static int ShowChanAcc(uchar bchan)
{
    uchar LanRecord[30];
    int ret = 0;

    ret = GetChanAcc(bchan, 0x40, LanRecord);
    if (fdebug)
         printf("  GetChanAcc(%d) ret = %d, data = %02x %02x\n",
 	        bchan,ret, LanRecord[0], LanRecord[1]);
    if (ret == 0) 
       fprintf(fd_bmc,"ChannelAccess %d%c %02x %02x \n",
		   bchan,bdelim,LanRecord[0],LanRecord[1]);
    return(ret);
}

static int GetSerialOverLan( uchar chan, uchar bset, uchar block )
{
	uchar requestData[24];
	uchar rData[MAX_BUFFER_SIZE];
	int rlen;
	int status, i;
	uchar ccode;
	uchar enable_parm, auth_parm, baud_parm;
	uchar user;

        if (fIPMI20 && fSOL20) {
	   getsolcmd = GET_SOL_CONFIG2;
	   enable_parm = SOL_ENABLE_PARAM;
	   auth_parm   = SOL_AUTHENTICATION_PARAM;
	   baud_parm   = SOL_BAUD_RATE_PARAM;
	} else {
	   getsolcmd = GET_SOL_CONFIG;
	   enable_parm = SOL_ENABLE_PARAM;
	   auth_parm   = SOL_AUTHENTICATION_PARAM;
	   baud_parm   = SOL_BAUD_RATE_PARAM;
	   chan = 0;  /*override chan for IPMI 1.5*/
	}
	printf("%c## %s, GetSOL for channel %d ...\n",bcomment,progname,chan);

	requestData[0] = chan;  /*channel*/
	requestData[1] = enable_parm;
	requestData[2] = bset;    /*set*/
	requestData[3] = block;   /*block*/
	rlen = sizeof(rData);
        status = ipmi_cmd(getsolcmd, requestData,4, rData, &rlen,&ccode,fdebug);
	if (status != 0) return(status);
	if (ccode) {
		if (ccode == 0xC1) { /* unsupported command */
		   printf("%c Serial-Over-Lan not available on this platform\n",
			  bcomment);
		   return(status);
		} else {
	     	   printf("%c SOL Enable ccode = %x\n",bcomment,ccode);
	     	   status = ccode;
		}
	} else {  /*success*/
	     fprintf(fd_bmc,"SOLParam %d,%d,%d%c",chan,enable_parm,bset,bdelim);
	     for (i = 1; i < rlen; i++) fprintf(fd_bmc," %02x",rData[i]);
             fprintf(fd_bmc,"\n");
	}

	requestData[0] = chan;
	requestData[1] = auth_parm;
	requestData[2] = bset;   // selector
	requestData[3] = block;  // block
	rlen = sizeof(rData);
        status = ipmi_cmd(getsolcmd, requestData,4,rData, &rlen,&ccode,fdebug); 
	if (status != 0) return(status);
	if (ccode) {
	     printf("%c SOL Auth ccode = %x\n",bcomment,ccode);
	     status = ccode;
	} else {  /*success*/
	     fprintf(fd_bmc,"SOLParam %d,%d,%d%c",chan,auth_parm,bset,bdelim);
	     for (i = 1; i < rlen; i++) fprintf(fd_bmc," %02x",rData[i]);
             fprintf(fd_bmc,"\n");
	}

	requestData[0] = chan;
	requestData[1] = SOL_ACC_INTERVAL_PARAM;
	requestData[2] = bset;
	requestData[3] = block;
	rlen = sizeof(rData);
        status = ipmi_cmd(getsolcmd, requestData,4,rData, &rlen,&ccode,fdebug); 
	if (status != 0) return(status);
	if (ccode) {
	     printf("%c SOL Accum Interval ccode = %x\n",bcomment,ccode);
	     status = ccode;
	} else {  /*success*/
	     fprintf(fd_bmc,"SOLParam %d,%d,%d%c",chan,SOL_ACC_INTERVAL_PARAM,bset,bdelim);
	     for (i = 1; i < rlen; i++) fprintf(fd_bmc," %02x",rData[i]);
             fprintf(fd_bmc,"\n");
        }

	requestData[0] = chan;
	requestData[1] = SOL_RETRY_PARAM;
	requestData[2] = bset;
	requestData[3] = block;
	rlen = sizeof(rData);
        status = ipmi_cmd(getsolcmd, requestData,4,rData, &rlen,&ccode,fdebug); 
	if (status != 0) return(status);
	if (ccode) {
	     printf("%c SOL Retry ccode = %x\n",bcomment,ccode);
	     status = ccode;
	} else {  /*success*/
	     fprintf(fd_bmc,"SOLParam %d,%d,%d%c",chan,SOL_RETRY_PARAM,bset,bdelim);
	     for (i = 1; i < rlen; i++) fprintf(fd_bmc," %02x",rData[i]);
             fprintf(fd_bmc,"\n");
        }

	if (!fRomley) {
	  requestData[0] = chan;
	  requestData[1] = baud_parm;
	  requestData[2] = bset;
	  requestData[3] = block;
	  rlen = sizeof(rData);
          status = ipmi_cmd(getsolcmd,requestData,4,rData,&rlen,&ccode,fdebug); 
	  if (status != 0) return(status);
	  if (ccode) {
	     printf("%c SOL nvol Baud ccode = %x\n",bcomment,ccode);
	     status = ccode;
	  } else {  /*success*/
	     fprintf(fd_bmc,"SOLParam %d,%d,%d%c",chan,baud_parm,bset,bdelim);
	     for (i = 1; i < rlen; i++) fprintf(fd_bmc," %02x",rData[i]);
             fprintf(fd_bmc,"\n");
          }

	  requestData[0] = chan;
	  requestData[1] = SOL_VOL_BAUD_RATE_PARAM;  /*0x06*/
	  requestData[2] = bset;
	  requestData[3] = block;
	  rlen = sizeof(rData);
          status = ipmi_cmd(getsolcmd,requestData,4,rData,&rlen,&ccode,fdebug); 
	  if (status != 0) return(status);
	  if (ccode) {
	     printf("%c SOL vol Baud ccode = %x\n",bcomment,ccode);
	     status = ccode;
	  } else {  /*success*/
	     fprintf(fd_bmc,"SOLParam %d,%d,%d%c",chan,SOL_VOL_BAUD_RATE_PARAM,bset,bdelim);
	     for (i = 1; i < rlen; i++) fprintf(fd_bmc," %02x",rData[i]);
             fprintf(fd_bmc,"\n");
	  }
	}

        if (fIPMI20) {
	   requestData[0] = chan;  
	   rlen = sizeof(rData);
	   status = ipmi_cmdraw(GET_PAYLOAD_SUPPORT, NETFN_APP, 
			BMC_SA,PUBLIC_BUS,BMC_LUN,
			requestData,1,rData, &rlen, &ccode, fdebug);
           if ((status != 0) || (ccode != 0)) {
	     printf("%c SOL Payload Support(%d) error %d, ccode = %x\n",
			bcomment,chan,status,ccode);
	     if (status == 0) status = ccode;
           } else {  /*success*/
	     fprintf(fd_bmc,"SOLPayloadSupport %d%c",chan,bdelim);
	     for (i = 1; i < rlen; i++) fprintf(fd_bmc," %02x",rData[i]);
             fprintf(fd_bmc,"\n");
           }
	   /* get Payload Access for nusers, not just lan_user */
	   for (user = 1; user <= nusers; user++)
	   {
	     /* IPMI 2.0 nusers >= 4 users */
	     requestData[0] = chan;  
	     requestData[1] = user;  
	     rlen = sizeof(rData);
	     status = ipmi_cmdraw(GET_PAYLOAD_ACCESS, NETFN_APP, 
			BMC_SA,PUBLIC_BUS,BMC_LUN,
			requestData,2,rData, &rlen, &ccode, fdebug);
             if ((status != 0) || (ccode != 0)) {
	       printf("%c SOL Payload Access(%d,%d) error %d, ccode = %x\n",
			bcomment,chan,user,status,ccode);
	       if (status == 0) status = ccode;
             } else {  /*success*/
	       fprintf(fd_bmc,"SOLPayloadAccess %d,%d%c",chan,user,bdelim);
	       for (i = 0; i < rlen; i++) fprintf(fd_bmc," %02x",rData[i]);
               fprintf(fd_bmc,"\n");
             }
           }  /*end user loop*/
        }

	return(status);
} /*end GetSerialOverLan */

static char *PefDesc(int idx, uchar stype)
{
   char *pdesc;
   if (pefdesc == NULL) pdesc = "reserved";
   else pdesc = pefdesc[idx];
   if ((stype != 0) && (strcmp(pdesc,"reserved") == 0)) {
      /* pefdesc may not match on some non-Intel systems. */
      /* so use sensor type */
      switch(stype) {  
         case 0x01: pdesc = "Temperature";    break;
         case 0x02: pdesc = "Voltage";  break;
         case 0x04: pdesc = "Fan";      break;
         case 0x05: pdesc = "Chassis";  break;
         case 0x07: pdesc = "BIOS";     break;
         case 0x08: pdesc = "Power Supply";   break;
         case 0x09: pdesc = "Power Unit";   break;
         case 0x0c: pdesc = "Memory";   break;
         case 0x0f: pdesc = "Boot";     break;
         case 0x12: pdesc = "System Restart"; break;
         case 0x13: pdesc = "NMI"; break;
         case 0x23: pdesc = "Watchdog"; break;
         case 0x20: pdesc = "OS Critical Stop"; break;
         default:   pdesc = "Other";    break;
      }
   }
   return(pdesc);
}

static int GetSessionInfo(uchar *rData, int sz)
{
   int rv, rlen;
   uchar ccode;
   uchar iData[5];

   iData[0] = 0x00;  /*get data for this session*/
   rlen = sz;
   rv = ipmi_cmdraw(CMD_GET_SESSION_INFO,NETFN_APP, BMC_SA,PUBLIC_BUS,BMC_LUN,
			iData,1,rData, &rlen, &ccode, fdebug);
   if ((rv == 0) && (ccode != 0)) rv = ccode;
   return(rv);
}

static int GetPefCapabilities(uchar *bmax)
{
   int rv, rlen;
   uchar ccode;
   uchar rData[MAX_BUFFER_SIZE];

   rlen = sizeof(rData);
   rv = ipmi_cmdraw(0x10, NETFN_SEVT, BMC_SA,PUBLIC_BUS,BMC_LUN,
			NULL,0,rData, &rlen, &ccode, fdebug);
   if ((rv == 0) && (ccode != 0)) rv = ccode;
   if ((rv == 0) && (bmax != NULL)) 
      *bmax = rData[2]; /*max num PEF table entries*/
   return(rv);
}

int WaitForSetComplete(int limit)
{
   int rv = 0;
   int i;
   uchar bdata[4];
             
   for (i = 0; i < limit; i++) {
      rv = GetLanEntry(0, 0, bdata);
      if (fdebug) printf("WaitForSetComplete(%d): i=%d rv=%d val=%x\n",
			limit,i,rv,bdata[1]);
      if ((rv == 0) && (bdata[1] == 0))  break; 
      else os_usleep(0,100);
   }
   return(rv);
}

int SerialIsOptional(int bparam)
{
   /* These Serial Parameters are for optional Modem/Callback functions. */
   int optvals[9] = { 5, 9, 10, 11, 12, 13, 14, 20, 21 };
   int rv = 0;
   int i;
   for (i = 0; i < (sizeof(optvals) / sizeof(int)); i++) {
      if (optvals[i] == bparam) { rv = 1; break; }
   }
   return(rv);
}

static int parse_line(char *line, char *keyret, char *value)
{
	char *eol;
	char *key;
	char *val;
        int i, n;

        key = &line[0];
        eol = &line[strlen(line)];
        while( *key < 0x21 && key < eol )
            key++;              /*remove leading whitespace */
        if( key[0] == bcomment ) return 2;        /*skip comments */
        /*
         * find the value set, delimited by bdelim (':' or '|')
         */
        val = strchr( line, bdelim ); 
        if( val == NULL ) return 1;  /* skip if empty or no delimeter */
        val[0] = 0;   /*stringify the key*/
        val++;
        while( val[0] < 0x21 && val < eol )
            val++;              /*remove leading whitespace */
        /*
         * truncate trailing newline/whitespace after last word
         */
        n = strlen_( val );
        for( i = n; i >= 0; i-- )        /*decrease from end*/
            if( val[i] >= 0x21 ) break;  /*found last valid char */
        if (i < n) val[i + 1] = 0;

	strcpy(keyret, key);  /*has keyword and params*/
	strcpy(value, val);   /*has list of hex values*/
	return 0;
}

#ifdef METACOMMAND
int i_config(int argc, char **argv)
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
   PEF_RECORD *pPefRecord;
   PEF_RECORD PefRecord;
   uchar LanRecord[64];
   uchar bParams[5];
   char filename[80] = "";
   int i, j, c, n;
   uchar idx;
   // char *pstr;
   uchar bset = 0;
   int ndest = 4;
   int idest;
   // char mystr[80];
   uchar * pc; int sz;
   char line[240]; /* hdr(18) + data(192 = 64 * 3) + '\0' = 211 */
   char key[40];
   char value[100];
   uchar rData[50];
   int rlen;
   uchar cc;
   uchar chan;
   char *pk;
   char fpefok = 1;
   char fignore_err;

   // progname = argv[0];
   printf("%s ver %s \n",progname,progver);
   func = 'l'; freadonly = 1;  /*list is default*/

   while ((c = getopt(argc, argv,"cdmlr:s:xL:T:V:J:EYF:P:N:R:U:Z:?")) != EOF)
      switch(c) {
          case 'c': fcanonical = 1; bdelim = BDELIM; break; 
          case 'd': func = 'd'; freadonly = 0; break;  /*set Defaults*/
          case 'l': func = 'l'; freadonly = 1; break;  /*list*/
          case 'm': fdomac = 1; break; /*restore mac*/
          case 'r': func = 'r'; freadonly = 0;   /*restore*/
		sz = strlen_(optarg);
		if (sz > sizeof(filename)) sz = sizeof(filename);
		strncpy(filename,optarg,sz);
		break;
          case 's': func = 's'; freadonly = 1;   /*save*/
		sz = strlen_(optarg);
		if (sz > sizeof(filename)) sz = sizeof(filename);
		strncpy(filename,optarg,sz);
		break;
          case 'x': fdebug = 1;     break;
	  case 'p':      /* password to set */
		fpassword = 1;
		if (strlen_(optarg) > 16) optarg[16] = 0;
		strcpy(passwordData,optarg);
                if (fdebug) printf("Password = %s\n",passwordData);
		/* Hide password from 'ps' */
	        memset(optarg, ' ', strlen_(optarg));	
		break;
          case 'L': 
		lan_ch_parm = atob(optarg);
		if (lan_ch_parm >= MAXCHAN) lan_ch_parm = 0xff; /*invalid*/
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
          case 'Z':    /* set local MC address */
                parse_lan_options(c,optarg,fdebug);
                break;
	  default:
             printf("Usage: %s [-clmpxLNUPREFTJVY -r <file> -s <file>]\n",
			 progname);
             printf("where -l  Lists BMC configuration parameters\n");
             printf("      -r  Restores BMC configuration from <file>\n");
             printf("      -s  Saves BMC configuration to <file>\n");
             printf("      -c  canonical output with delimiter '%c'\n",BDELIM);
             printf("      -m  Set BMC MAC during restore\n");
             printf("      -x  show eXtra debug messages\n");
             printf("      -p <psw> specify a user password to set\n");
             printf("      -L 3     specify lan channel number 3\n");
	     print_lan_opt_usage(0);
	     ret = ERR_USAGE;
	     goto do_exit;
      }

   switch(func) {
	case 'l':  fd_bmc = stdout; break;
	case 'r':  fd_bmc = fopen(filename,"r"); break;
	case 's':  fd_bmc = fopen(filename,"w"); break;
	default: break;
   }
   if (fd_bmc == NULL) {
	printf("Error: cannot open %s\n",filename);
	ret = ERR_FILE_OPEN;
	fd_bmc = stdout;
	goto do_exit;
   }
   fipmilan = is_remote();

   if (fipmilan) parse_lan_options('V',"4",0);

   ret = GetDeviceID( LanRecord);
   if (ret != 0) {
	goto do_exit;
   } else {  /* success */
      uchar ipmi_maj, ipmi_min;
      ipmi_maj = LanRecord[4] & 0x0f;
      ipmi_min = LanRecord[4] >> 4;
      show_devid( LanRecord[2],  LanRecord[3], ipmi_maj, ipmi_min);
      if (ipmi_maj == 0)  fIPMI10 = 1;
      else if (ipmi_maj == 1 && ipmi_min < 5) fIPMI10 = 1; 
      else  fIPMI10 = 0;    /* >= IPMI 1.5 is ok */
      if (ipmi_maj >= 2) fIPMI20 = 1;
      /* nusers can be up to 15 max */
      if (fIPMI20) nusers = 5;  
      else nusers = 3;
      if (fIPMI10) {
         printf("%c This IPMI v%d.%d system does not support PEF records.\n",
			 bcomment,ipmi_maj,ipmi_min);
	 /* Wont handle PEF, but continue and look for BMC LAN anyway */
	 // fIPMI10 = 1;
	 // ipmi_close_();
	 // exit(1);  
         }
      prod_id = LanRecord[9] + (LanRecord[10] << 8);
      vend_id = LanRecord[6] + (LanRecord[7] << 8) 
			 	  + (LanRecord[8] << 16);
      /* check Device ID response for Manufacturer ID = 0x0322 (NSC) */
      if (vend_id == VENDOR_NSC) {         /* NSC = 0x000322 */
	 fmBMC = 1;  /*NSC miniBMC*/
	 if (pefnum == 12) pefnum = 10;  /* change CritStop pefnum to 0x0a */
	 pefdesc = &pefdesc2[0];
	 pefmax = 30;
	 fsharedMAC = 1;  /*LAN1 shares MAC with OS*/
      } else if (vend_id == VENDOR_LMC) {  /* LMC (on SuperMicro) = 0x000878 */
	 fmBMC = 0;  
	 pefdesc = NULL;  /* unknown, see PefDesc() */
	 if (pefnum == 12) pefnum = 15;  /* change CritStop pefnum */
	 pefmax  = 16;
         fsharedMAC = 0;  /* not-shared BMC LAN port */
      } else if (vend_id == VENDOR_INTEL) {  /* Intel = 0x000157 */
	 pefdesc = &pefdesc1[0];  /*Intel defaults*/
	 pefmax  = 20;            /*Intel default pefmax = 20*/
         switch(prod_id) {
         case 0x4311:  /* Intel NSI2U*/
 	   fmBMC = 1;  /* Intel miniBMC*/
	   if (pefnum == 12) pefnum = 14;  /* change CritStop pefnum */
	   pefdesc = &pefdesc2[0];
	   pefmax = 30;
	   fsharedMAC = 1;  /*LAN1 shares MAC with OS*/
           break;
         case 0x0026:
         case 0x0028:
         case 0x0811:  /* Alcolu & TIGW1U */
	   fmBMC = 0;  /* Intel Sahalee BMC*/
           fsharedMAC = 0;  /* not-shared BMC LAN port, separate MAC */
           gcm_ch = 3;
           break;
         case 0x003E:  /*NSN2U or CG2100 Urbanna*/
	   fiBMC = 1;  /* Intel iBMC */
           fsharedMAC = 0;  /* not-shared BMC LAN port, separate MAC */
           set_max_kcs_loops(URNLOOPS); /*longer for SetLan cmds (default 300)*/
           break;
         case 0x0107:  /* Intel Caneland*/
           fsharedMAC = 0;  /* not-shared BMC LAN port, separate MAC */
           gcm_ch = 3;
           break;
         case 0x0022:  /* Intel TIGI2U*/
	   fsharedMAC = 1;  /*LAN1 shares MAC with OS*/
           gcm_ch = 3;
	   nusers = 4;
           break;
         default:      /* else other Intel */
           if (fIPMI20) fsharedMAC = 0;  /* recent, not-shared BMC MAC */
           else fsharedMAC = 1;  /* usu IPMI 1.x has shared BMC MAC */
#ifdef TEST
	   /* also check for ia64, and set chan_pefon, chan_pefoff accordingly*/
	   if (prod_id == 0x0100) { /* Intel Tiger2, Itanium2 */
	     chan_pefon  = CHAN_ACC_PEFON64;
	     chan_pefoff = CHAN_ACC_PEFOFF64;
           }
#endif
           break;
         } /*end switch*/
         if (is_romley(vend_id,prod_id)) fRomley = 1;
         if (is_grantley(vend_id,prod_id)) fGrantley = 1;
         if (fRomley) {
            fiBMC = 1;  /* Intel iBMC */
            fsharedMAC = 0;  /* not-shared BMC LAN port, separate MAC */
            set_max_kcs_loops(URNLOOPS); /*longer for SetLan cmds */
            fipv6 = 1;
         }
      } else if (vend_id == VENDOR_KONTRON) { 
	 //if (prod_id == 0x1590) fchan2wart = 1; /* KTC5520 chan2 wart */
         fsharedMAC = 0;     /* not-shared BMC MAC */
	 pefdesc = NULL;     /* unknown, see PefDesc() */
	 if (pefnum == 12) pefnum = 15;  /* change CritStop pefnum to 15 */
      } else {  /* else other vendors  */
         if (fIPMI20) fsharedMAC = 0;  /* recent, not-shared BMC MAC */
         else fsharedMAC = 1;  /* usu IPMI 1.x has shared BMC MAC */
	 fmBMC = 0;  
	 pefdesc = NULL;  /* unknown, see PefDesc() */
	 if (pefnum == 12) pefnum = 15;  /* change CritStop pefnum to 15? */
	 pefmax  = 20;
      }
      if (fmBMC) nusers = 1;
   }

   ret = GetPefCapabilities(&bset);
   if ((ret == 0) && (bset <= MAXPEF)) pefmax = bset;

   /* Get the BMC LAN channel & match it to an OS eth if. */
   i = GetBmcEthDevice(lan_ch_parm);
   if (i == -2) {  /* no lan channels */
	printf("This system does not support BMC LAN channels.\n");
	ret = ERR_NOT_ALLOWED;
	goto do_exit;
   } else if (i < 0) {  /* mac not found, use platform defaults */
	i = 0;   /* default to eth0, lan_ch set already. */
	if (vend_id == VENDOR_INTEL) {
	   if ((prod_id == 0x001B) || (prod_id == 0x000c)) { 
	      /* Intel TIGPR2U or TSRLT2 defaults are special */ 
              if (lan_ch_parm == 6) 
                   { i = 0; lan_ch = 6; }
              else { i = 1; lan_ch = 7; }
	      ser_ch = 1;
	   }
        }
   }
   if ((gcm_ch != 0) && (lan_ch_parm == 0xff)) {
	/* Has a GCM, and user didn't specify -L */
	/* Need this to avoid picking channel 3, the IMM GCM channel. */
	lan_ch = 1;  /*default BMC LAN channel*/
	// i = 1;       /*default eth1*/
   }
   if (fsetifn == 0) {
       if (lan_ch == gcm_ch) strcpy(ifname,"gcm");
       else sprintf(ifname,"%s%d",ifpattn,i);
   }
   if (fdebug) printf("lan_ch = %d, ifname = %s\n",lan_ch,ifname);

   /* initialize the correct SOL command values */
   if (fIPMI20 && fSOL20) {
	   setsolcmd = SET_SOL_CONFIG2;
	   getsolcmd = GET_SOL_CONFIG2;
           sol_bchan = lan_ch;
   } else {
	   setsolcmd = SET_SOL_CONFIG;
	   getsolcmd = GET_SOL_CONFIG;
           sol_bchan = 0x00;  /*override chan for IPMI 1.5*/
   }

   memset(SessInfo,0,sizeof(SessInfo));
   ret = GetSessionInfo(SessInfo,sizeof(SessInfo));
   // rlen = sizeof(SessInfo)); ret = get_session_info(0,0,SessInfo,&rlen); 
   if (fdebug) printf("GetSessionInfo ret=%d, data: %02x %02x %02x %02x \n",
                        ret,SessInfo[0],SessInfo[1],SessInfo[2],SessInfo[3]);
   if (!freadonly && fipmilan) {  /* setting LAN params, and using IPMI LAN */
      if (SessInfo[2] > 1) { /* another session is active also */
         printf("Another session is also active, cannot change IPMI LAN settings now.\n");
	 ret = ERR_NOT_ALLOWED;
	 goto do_exit;
      }
   }

   /* set the lan_user appropriately */
   if (freadonly) 
   {
     if (!fIPMI10) {
      printf("%c## %s, GetPefEntry ...\n",bcomment,progname);
      for (idx = 1; idx <= pefmax; idx++)
      {
         ret = GetPefEntry( 0x06, (ushort)idx, &PefRecord);
         if (ret == 0) {    // Show the PEF record
            pc = (uchar *)&PefRecord;
            sz = 21;        // sizeof(PEF_RECORD) = 21
	    printf("%c PefParam(%d): %s\n",bcomment,idx,PefDesc(idx,pc[7]));
	    fprintf(fd_bmc,"PEFParam %d,%02d%c",6,idx,bdelim);
	    for (i = 0; i < sz; i++) fprintf(fd_bmc," %02x",pc[i]);
	    fprintf(fd_bmc,"\n");
         } else {
	    char *pstr;
	    if (ret > 0) pstr = decode_cc(0,(uchar)ret);
	    else pstr = "";
	    printf("%c GetPefEntry(%d): ret = %d %s\n",bcomment,idx,ret,pstr);
	    if (ret == 0xC1) {
		fpefok = 0;
		ndest = 0;
		break;
	    }
	 }
      }
      if (fpefok) {
       ret = GetPefEntry(0x01, 0,(PEF_RECORD *)&LanRecord);  
       if (ret == 0) {
	 fprintf(fd_bmc,"PEFParam %d%c %02x\n",1,bdelim,LanRecord[0]);
       }
       ret = GetPefEntry(0x02, 0,(PEF_RECORD *)&LanRecord);
       if (ret == 0) {
	 fprintf(fd_bmc,"PEFParam %d%c %02x\n",2,bdelim,LanRecord[0]);
       }
       ret = GetPefEntry(0x03, 0,(PEF_RECORD *)&LanRecord);
       if (ret == 0)
	   fprintf(fd_bmc,"PEFParam %d%c %02x\n", 3,bdelim,LanRecord[0]);
       if (!fmBMC) {
	 ret = GetPefEntry(0x04, 0,(PEF_RECORD *)&LanRecord);
	 if (ret == 0) 
	   fprintf(fd_bmc,"PEFParam %d%c %02x\n",4,bdelim,LanRecord[0]);
	 /* fmBMC gets cc=0x80 here */
       }
       /* note that ndest should be read from lan param 17 below. */
       for (i = 1; i <= ndest; i++)
       {
         ret = GetPefEntry(0x09, (ushort)i,(PEF_RECORD *)&LanRecord);
         if (ret == 0) {
	   fprintf(fd_bmc,"PEFParam %d,%d%c %02x %02x %02x %02x \n",9,i,bdelim,
			LanRecord[0], LanRecord[1],LanRecord[2], LanRecord[3]);
         }
       } /*endfor ndest*/
      } /*endif fpefok*/
     }  /*endif not fIPMI10*/

     for (chan = lan_ch; chan < MAXCHAN; chan++ )
     {
      if (chan_type[chan] != 4) continue;  /*chan != LAN, skip it*/
      lan_ch = chan;
      printf("%c## %s, GetLanEntry for channel %d ...\n",bcomment,progname,lan_ch);
      idest = 1;
      for (idx = 0; idx < NLAN; idx++)
      {
	 int ival;
         if (idx == 8 || idx == 9) continue;	  /* not implemented */
	 ival = lanparams[idx].cmd;
         if (ival >= 96 && ival <= 98) continue;  /* not implemented */
	 if (ival >= 102 && ival <= 108) { /*custom IPv6 parameters*/
	     if (fipv6 == 0) continue;  /*skip these*/
	 }
	 if (ival == 194 && vend_id == VENDOR_KONTRON) { /*oem hostname parm*/
	     lanparams[idx].sz = 36;
	     strcpy(lanparams[idx].desc,"IPMI Hostname");
	 } else if (ival >= 192 && ival <= 194) { /*custom DHCP parameters*/
             if (vend_id != VENDOR_INTEL) continue;
             if (fmBMC || fiBMC || fRomley || fcanonical) continue;  /*skip*/
         }
         /* VLAN params 20-25, fIPMI20 only*/
	 if (ival >= 20 && ival <= 25) { if (!fIPMI20) continue; }
         if (ival == 11) {  /*grat arp interval*/
             if (vend_id == VENDOR_SUPERMICROX) continue;
             if (vend_id == VENDOR_SUPERMICRO) continue;
         }
         if (ival == 14 || ival == 15) {  /*secondary gateway is optional*/
             if (vend_id == VENDOR_KONTRON) continue;
         }
         if (ival == 201) {  /*Get Channel Access*/
             ret = ShowChanAcc(lan_ch);
	 } else {
	     if (ival == 18 || ival == 19) {  /*dest params*/
		if (ndest == 0) continue;  /*skip if none*/
		bset = (uchar)idest;  /* dest id = 1 thru n */
	     } else bset = 0;
             ret = GetLanEntry((uchar)ival, bset, LanRecord);
	 }
         if (ret == 0) {    // Show the LAN record
            pc = (uchar *)&LanRecord;
            sz = lanparams[idx].sz; 
	    if (ival == 201) ;  /* ShowChanAcc(lan_ch) above */
            else {
		fprintf(fd_bmc,"LanParam %d,%d,%d%c ",lan_ch,ival,bset,bdelim);
	        for (i = 0; i < sz; i++) fprintf(fd_bmc," %02x",pc[i]);
	        fprintf(fd_bmc,"\n");
		if (ival == 3) 
		   printf("%c LanParam(%d,%d,%d) IP address: %d.%d.%d.%d\n", 
				bcomment, lan_ch,ival,bset,
				pc[0], pc[1], pc[2], pc[3]);
	    }

	    if (ival == 1) {
		authmask = pc[0]; /* auth type support mask */
		/* if (fmBMC) authmask is usually 0x15, else 0x14 */
            } else if (ival == 3) {
                if (IpIsValid(pc)) memcpy(bmcmyip,pc,4);
	    } else if (ival == 5) {
		if (MacIsValid(pc)) memcpy(bmcmymac,pc,MAC_LEN);
	    } else if (ival == 17)  {  /* num dest */
		ndest = pc[0];  /* save the number of destinations */
	    } else if (ival == 19)  {  /* dest addr */
                if (IpIsValid(&pc[3])) memcpy(bmcdestip,&pc[3],4);
	    }

	    if (ival == 18 || ival == 19) {
		if (idest < ndest) {
		   idest++;
		   idx--;  /* repeat this param*/
		} else idest = 1;
	    }
         } else { /* ret != 0 */
	    if (ival >= 20 && ival <= 25) ; 
	    else 
               printf("%c GetLanEntry(%d,%d,%d), ret = %d\n",bcomment,lan_ch,ival,bset,ret);
	    if (ival == 17) ndest = 0;  /*error getting num dest*/
	 }
      }  /*end for NLAN*/
      if (!fIPMI10) {  /* Get SOL params */
	 ret = GetSerialOverLan(lan_ch,0,0);
	 if (ret != 0) printf("%c GetSOL error %d\n",bcomment,ret);
      }
      for (idx = 1; idx <= nusers; idx++) 
	 GetUser(idx);
      if (lan_ch_parm != 0xff) chan = MAXCHAN;
     } /*end-for chan*/

     printf("%c## %s, GetSerEntry for channel %d ...\n",bcomment,progname,ser_ch);
     if (fmBMC || (ser_ch == 0)) {   /* mBMC doesn't support serial */
	  printf("%cNo serial channel support on this platform\n",
		 bcomment);
     } else {
       idest = 1;
       for (idx = 0; idx < NSER; idx++) {
   	 int ival;
         // if (idx == 9) continue; /* not implemented */
	 ival = serparams[idx].cmd;
         if (vend_id == VENDOR_SUPERMICRO && ival == 8) continue;
         if (ival == 201) {
             j = ShowChanAcc(ser_ch);
	 } else {
             if (ival == 17 || ival == 19 || ival == 21 || ival == 23)         
		  bset = (uchar)idest; 
             else bset = 0;  /*default*/
             ret = GetSerEntry((uchar)ival, bset, LanRecord);
             if (ret == 0) {    // Show the SER record
                pc = (uchar *)&LanRecord;
                sz = serparams[idx].sz; 
                fprintf(fd_bmc,"SerialParam %d,%d,%d%c",
			ser_ch,ival,bset,bdelim);
                for (i = 0; i < sz; i++) 
		    fprintf(fd_bmc," %02x",pc[i]);  /* show in hex */
                fprintf(fd_bmc,"\n");
	        if (ival == 16) ndest = pc[0];
	     } else {  /*ret != 0, error*/
		char *pstr;
		char *tag;
		if (SerialIsOptional(ival)) tag = "Optional";
		else tag = "";
		if (ret > 0) pstr = decode_cc(0,ret);
		else pstr = "";
		printf("%c GetSerEntry(%d,%d): %s ret = %d %s\n",
			bcomment,ser_ch,ival,tag,ret,pstr);
	        if (ival == 16) ndest = 0;
		if (ret == 0xC1) {
		   ret = 0;
		   break; /*not supported, exit for loop*/
		}
	     }
	 }
         if (ival == 17 || ival == 19 || ival == 21 || ival == 23) {
		if (idest < ndest) {
		   idest++;
		   idx--;  /* repeat this param*/
		} else idest = 1;
	 }
       }  /*end for NSER*/
       lan_ch_sav = lan_ch;
       lan_ch = ser_ch;  /* use ser_ch for User functions now */
       for (idx = 1; idx <= nusers; idx++) 
	  GetUser(idx);
       lan_ch = lan_ch_sav;
     }  /*endif serial*/

     printf("%c## %s, GetSystemParams ...\n",bcomment,progname);
     for (idx = 0; idx < NSYS; idx++) { /*Get System Params*/
	switch(idx) {
	   case 0:  j = CHAS_RESTORE; bset = 0; break;
	   case 1:  j = SYS_INFO; bset = 1; break;
	   case 2:  j = SYS_INFO; bset = 2; break;
	   case 3:  j = SYS_INFO; bset = 3; break;
	   case 4:  j = SYS_INFO; bset = 4; break;
	   case 5: 
	   default: j = LAN_FAILOVER; bset = 0; break; 
	}
	fignore_err = 0;
	pc = (uchar *)&LanRecord;
	switch(j) {
	   case CHAS_RESTORE: /* Chassis Status, Power Restore Policy */
		sz = 0;
		rlen = sizeof(rData);
		ret = ipmi_cmdraw(CHASSIS_STATUS, NETFN_CHAS,
			BMC_SA,PUBLIC_BUS,BMC_LUN,
			pc,0,rData,&rlen,&cc,fdebug);
		if (ret == 0 && cc != 0) ret = cc;
		if (ret == 0) {
		   sz = rlen;
		   memcpy(pc,&rData,sz);   /*should be 3 bytes*/
		}
		break;
	   case SYS_INFO: /* System Info */
		if (! fIPMI20) continue;  /*skip if not IPMI 2.0*/
		rlen = sizeof(LanRecord); /* param from read */
		ret = get_system_info(bset,LanRecord,&rlen);
		/* find actual string size (ends at first 0x00) */
		for (i=0; i<rlen; i++) if (LanRecord[i] == 0) break;
		if (i < rlen) rlen = i;
		sz = rlen;
		fignore_err = 1;
		break;
	   case LAN_FAILOVER: /* Intel LAN Failover */
		if (is_romley(vend_id,prod_id)) 
		     ret = lan_failover_intel(0xFF,&LanRecord[0]);
        else if ((vend_id == VENDOR_SUPERMICROX) || (vend_id == VENDOR_SUPERMICRO)) {
		     ret = oem_supermicro_get_lan_port(&LanRecord[0]);
		     if (fdebug) printf("SMC get_lan_port ret=%d val=%d\n",ret,LanRecord[0]);
		} else continue;  /*skip if not Intel Romley */
		sz = 1;
		fignore_err = 1;
		break;
	   default:   /*do nothing*/
		sz = 0;
		ret = LAN_ERR_NOTSUPPORT;
		break;
	}  /*end switch*/
	if (ret == 0) {
		fprintf(fd_bmc,"SystemParam %d,%d%c ",j,bset,bdelim);
	        for (i = 0; i < sz; i++) fprintf(fd_bmc," %02x",pc[i]);
	        fprintf(fd_bmc,"\n");
	} else {
		char *pstr;
		if (ret > 0) pstr = decode_cc(0,ret);
		else pstr = "";
		if (fdebug || !fignore_err)
		   printf("%c GetSystemParam(%d,%d): ret = %d %s\n",
			  bcomment,j,bset,ret,pstr);
		if (fignore_err) ret = 0;
	}
     }  /*end-for System Params*/

    } /*endif readonly*/

    if (!freadonly)  /* Set parameters via Restore */
    {
       if (fipmilan) { /* Sets not valid via ipmi_lan if same channel. */
         printf("\nWarning: Setting LAN %d params while using a LAN channel.\n",		lan_ch);
       }
       GetUser(1);  /*sets num enabled_users */

       /* Set BMC parameters.  (restore)  */
       /* read each record from the file */
       while ( fgets( line, sizeof(line), fd_bmc) != NULL )    
       {
	 ret = parse_line(line, key, value);
	 if (ret != 0) {
		if (ret == 2) ret = 0; /*just skip comment*/
		else if (fdebug) printf("parse error on line: %s\n",line);
		continue;
	 }

	 /* get parameters from key */
	 sz = sizeof(bParams);
	 memset(bParams,0,sz);
	 pk = strchr(key,' ');  /*skip keyword, to first param*/
	 if (pk == NULL) {
	    pk = &key[0];
	 }
	 for (n=0; n<sz; n++)
	 {
	    pc = strchr(pk,',');
            if (pc != NULL) {
		*pc = 0;
		bParams[n] = atob(pk);
	    	pk = (char *)++pc;
	    } else { 
		bParams[n] = atob(pk);
		break;
	    }
	 }
	 /* n == number of params, usually 3 */
	 chan = bParams[0];
	 idx  = bParams[1];
	 bset = bParams[2];
	 /* get data from value */
	 pc = value;
	 sz = strlen_(value);
	 for (j=0,i=0; i<sz; i+=3)
	    LanRecord[j++] = htoi(&pc[i]);
	 if (fdebug) {
		printf("Record(%d,%d,%d):",chan,idx,bset);
		for (i=0; i<j; i++) printf(" %02x",LanRecord[i]);
		printf("\n");
	 }

         if (strncasecmp(key,"LanParam",8) == 0) {
	    if (idx == 0) continue;  /*skip Set in progress*/
	    if (idx == 1 || idx == 17) continue;  /*read-only params*/
	    if (idx == 5 && fIPMI20) /*BMC MAC address & IPMI 2.0(not shared)*/
		if (fdomac == 0) continue;  /*skip BMC MAC unless -m */
            if ((idx == 10) && (chan == gcm_ch)) continue; /*skip Lan3 arp*/
            if ((vend_id == VENDOR_PEPPERCON) && (idx == 7)) continue;
	    if ((idx == 18 || idx == 19) && (ndest == 0)) continue;  
	    if (idx >= 22 && idx <= 24) continue; /*read-only Cipher*/
	    else if (idx >= 20 && idx <= 25) {  /*VLAN*/
	       if (!fIPMI20) continue; 
	    }
	    if (idx == 18) j--;  /*one less byte for Set than from Get*/
	    lan_ch = chan;
	    if (idx == 3) {  /* 3 = IP address */
	       uchar bdata[2];
	       bdata[0] = 0x00;  /*disable grat arp while setting IP*/
               ret = SetLanEntry(10, &bdata[0], 1);
               if (fdebug) printf("SetLanEntry(%d,10,0), ret = %d\n",chan,ret);
	       WaitForSetComplete(4); /*wait if it is a slow MC */
	       bdata[0] = SRC_STATIC;  /*set src to static before setting IP*/
               ret = SetLanEntry(4, &bdata[0], 1);
               if (fdebug) printf("SetLanEntry(%d,4,0), ret = %d\n",chan,ret);
	       WaitForSetComplete(4); /*wait if it is a slow MC */
	    }
	    else if ((idx == 6) || (idx == 12)) WaitForSetComplete(4);
            ret = SetLanEntry(idx, LanRecord, j);
	    if ((ret != 0)  && (idx >= 20 && idx <= 25)) ;  /*VLAN optional*/
	    else {
	       printf("SetLanEntry(%d,%d), ret = %d\n",chan,idx,ret);
	       if (ret != 0) { nerrs++; lasterr = ret; }
	       else ngood++;
	    }

	 } else if (strncasecmp(key,"PEFParam",8) == 0) {
	    if (fpefok == 0) continue;
	    idx  = bParams[0];
	    bset = bParams[1];
	    if (idx == 6) {  /*PEF table rules*/
		pPefRecord = (PEF_RECORD *)&LanRecord[0];
		if (pPefRecord->fconfig == 0xC0) {
		  pPefRecord->fconfig  = 0x80;    /* enabled, software */
                  ret = SetPefEntry(pPefRecord);
                  if (fdebug)
                    printf("SetPefEntry(%d,%d/80) ret = %d\n",idx,bset,ret);
		  pPefRecord->fconfig  = 0xC0;
		}
                ret = SetPefEntry(pPefRecord);
                printf("SetPefEntry(%d,%d) ret = %d\n",idx,bset,ret);
	        if (ret != 0) { nerrs++; lasterr = ret; }
	        else ngood++;
	    } else { 
		pc = (uchar *)&PefRecord;
		pc[0] = idx;
	        for (i=0; i<j; i++)
	           pc[i+1] = LanRecord[i]; 
		rlen = sizeof(rData);
		ret = ipmi_cmd(SET_PEF_CONFIG, pc,j+1, rData,&rlen, &cc,fdebug);
		if ((ret == 0) && (cc != 0)) ret = cc;
	        if ((idx == 9) && (bset > 1) && (ret != 0)); /*9=PEF Policy*/
		else {
                  printf("SetPefEntry(%d,%d) ret = %d\n",idx,bset,ret);
	          if (ret != 0) { nerrs++; lasterr = ret; }
	          else ngood++;
		}
	    }
	    if (ret == 0xC1) { fpefok = 0; ndest = 0; }

	 } else if (strncasecmp(key,"SerialParam",11) == 0) {
	    if (idx == 0) continue;  /*skip Set in progress*/
	    if (idx == 1 || idx == 16) continue;    /*read-only param*/
            if (vend_id == VENDOR_PEPPERCON) {
		if ((idx >= 3) && (idx <= 6)) continue;
	    }
            if ((vend_id == VENDOR_SUPERMICROX) ||
                (vend_id == VENDOR_SUPERMICRO)) {
		if (idx == 3) continue;
		if ((idx >= 6) && (idx <= 8)) continue;
		if (idx == 29) continue;
	    }
            if (fmBMC || (ser_ch == 0)) continue; /*doesn't support serial*/
            ser_ch = chan;
            ret = SetSerEntry(idx, LanRecord, j); 
	    if ((ret != 0) && SerialIsOptional(idx)) ; /*ignore errors if opt*/
	    else {
		printf("SetSerEntry(%d,%d,%d), ret = %d\n",chan,idx,bset,ret);
		if (ret != 0) { nerrs++; lasterr = ret; }
	        else ngood++;
	    }
	 } else if (strncasecmp(key,"ChannelAccess",13) == 0) {
            if (((vend_id == VENDOR_SUPERMICROX) ||
                 (vend_id == VENDOR_SUPERMICRO)) && chan == 3) ; /*skip serial*/
	    else {
              ret = SetChanAcc(chan, 0x80, LanRecord[0], LanRecord[1]);
              if (fdebug) printf("SetChanAcc(%d/active), ret = %d\n",chan,ret);
              ret = SetChanAcc(chan, 0x40, LanRecord[0], LanRecord[1]);
              printf("SetChanAcc(%d), ret = %d\n",chan,ret);
	      if (ret != 0) { nerrs++; lasterr = ret; }
	      else ngood++;
	    }

	 } else if (strncasecmp(key,"UserName",8) == 0) {
	    if (fchan2wart && (lan_ch == 2)) continue;
	    idx  = bParams[0];
	    if (idx <= 1) ;  /*skip if anonymous user 1*/
            else if (idx == 2 && vend_id == VENDOR_SUPERMICROX) ; /*skip user2*/
            else if (idx == 2 && vend_id == VENDOR_SUPERMICRO) ; /*skip user2*/
	    else {
		pc = (uchar *)&PefRecord;
		pc[0] = idx;  /*user num*/
		memcpy(&pc[1],&LanRecord[0],16);
		rlen = sizeof(rData);
		ret = ipmi_cmd(SET_USER_NAME,pc,17, rData,&rlen, &cc,fdebug);
		if (ret == 0 && cc != 0) ret = cc;
		if (ret == 0xCC) ; /*SetUserName matching previous gives this*/
		else {
		  printf("SetUserName(%d) ret = %d\n",idx,ret); 
	          if (ret != 0) { nerrs++; lasterr = ret; }
	          else ngood++;
		}
	    }
	    if (fpassword) { 
		pc = (uchar *)&PefRecord;
		pc[0] = idx;  /*user num*/
		pc[1] = 0x02;  /*set password*/ 
		memset(&pc[2],0, PSW_LEN);
		strcpy(&pc[2],passwordData);
		rlen = sizeof(rData);
		ret = ipmi_cmd(SET_USER_PASSWORD,pc,2+PSW_LEN,rData,&rlen, 
				&cc,fdebug);
		printf("SetUserPassword(%d) ret = %d\n",idx,ret); 
	        if (ret != 0) { nerrs++; lasterr = ret; }
	        else ngood++;
	    }
	 } else if (strncasecmp(key,"UserPassword",12) == 0) {
	    if (fchan2wart && (lan_ch == 2)) continue;
	        idx  = bParams[0];
		pc = (uchar *)&PefRecord;
		pc[0] = idx;  /*user num*/
		pc[1] = 0x02;  /*set password*/ 
		memset(&pc[2],0,PSW_LEN);
		strcpy(&pc[2],passwordData);
		rlen = sizeof(rData);
		ret = ipmi_cmd(SET_USER_PASSWORD,pc,2+PSW_LEN,rData,&rlen, 
				&cc,fdebug);
		printf("SetUserPassword(%d) ret = %d\n",idx,ret); 
	        if (ret != 0) { nerrs++; lasterr = ret; }
	        else ngood++;
	 } else if (strncasecmp(key,"UserAccess",10) == 0) {
            if ((idx > enabled_users) && ((LanRecord[3] & 0x10) == 0)) continue;
	    if (vend_id == VENDOR_KONTRON) {
		if (idx == 1) continue;
		if (idx > enabled_users) continue;
	    }
	    if (ipmi_reserved_user(vend_id,idx) == 1) continue;
	    if (fchan2wart && (lan_ch == 2)) continue;
	    pc = (uchar *)&PefRecord;;
            pc[0] = 0x80 | (LanRecord[3] & 0x70) | chan; /*User Channel Access*/
	    pc[1] = idx;  /*user id*/
	    pc[2] = (LanRecord[3] & 0x0F); /*User Privilege (Admin,User,Oper)*/
	    pc[3] = 0x00;    /* User Session Limit, 0=not limited*/
	    rlen = sizeof(rData);
	    ret = ipmi_cmd(SET_USER_ACCESS,pc,4, rData,&rlen, &cc,fdebug);
	    if (ret == 0 && cc != 0) ret = cc;
	    if (ret != 0xCC) {  /*if invalid user, ignore errors*/
	        printf("SetUserAccess (%x %x %x %x) ret = %d\n", 
			pc[0],pc[1],pc[2],pc[3],ret);
	        if (ret != 0)  { nerrs++; lasterr = ret; }
	        else ngood++;
	    }
	    if ((LanRecord[3] & 0x0f) != 0x0F) { /*not NoAccess, enable user*/
		if (idx > last_user_enable) last_user_enable = idx;
		pc[0] = idx;  /*user number, 1=null_user */
		pc[1] = 0x01;  /*enable user*/
		rlen = sizeof(rData);
		ret = ipmi_cmd(SET_USER_PASSWORD,pc,2, rData,&rlen, &cc,fdebug);
		if (ret == 0 && cc != 0) ret = cc;
		printf("SetUserEnable (%x %x) ret = %d\n",pc[0],pc[1],ret);
	        if (ret != 0) { nerrs++; lasterr = ret; }
	        else ngood++;
	    }
	 } else if (strncasecmp(key,"SOLParam",8) == 0) {
	    if (fchan2wart && (chan == 2)) continue;
	    pc = (uchar *)&PefRecord;;
	    pc[0] = chan;
	    pc[1] = idx;  /*sol parameter number*/
	    memcpy(&pc[2],&LanRecord,j); 
	    rlen = sizeof(rData);
	    ret = ipmi_cmd(setsolcmd, pc, j+2, rData,&rlen, &cc,fdebug);
	    if (ret == 0 && cc != 0) ret = cc;
	    printf("SetSOLParam (%d,%d) ret = %d\n",chan,idx,ret);
	    if (ret != 0) { nerrs++; lasterr = ret; }
	    else ngood++;

	 } else if (strncasecmp(key,"SOLPayloadSupport",17) == 0) {
	    ; /* Nothing to do, this is a read-only parameter */

	 } else if (strncasecmp(key,"SOLPayloadAccess",16) == 0) {
	    if (fIPMI20) {
	       if (ipmi_reserved_user(vend_id,idx) == 1) continue;
	       pc = (uchar *)&PefRecord;;
	       pc[0] = chan;
	       pc[1] = idx;   // lan_user 
	       memcpy(&pc[2],&LanRecord,j); 
	       rlen = sizeof(rData);
	       ret = ipmi_cmdraw(SET_PAYLOAD_ACCESS,NETFN_APP,
			BMC_SA,PUBLIC_BUS,BMC_LUN,
			pc,j+2,rData,&rlen,&cc,fdebug);
	       if (ret == 0 && cc != 0) ret = cc;
	       printf("SetSOLPayloadAccess (%d,%d) ret = %d\n",
			chan,idx,ret);
	       if (ret != 0) { nerrs++; lasterr = ret; }
	       else ngood++;
	    }
	 } else if (strncasecmp(key,"SystemParam",11) == 0) {
	    idx  = bParams[0];
	    bset = bParams[1];
	    switch(idx) {
              case CHAS_RESTORE:  /* Chassis Power Restore Policy*/
	        if (vend_id == VENDOR_KONTRON) continue; /*N/A, cannot set it*/
		pc = (uchar *)&PefRecord;;
		i = (LanRecord[0] & 0x60); /*restore policy bits*/
		if (i & 0x20) pc[0] = 0x01;  /*last_state*/
		else if (i & 0x40) pc[0] = 0x02;  /*turn_on*/
		else pc[0] = 0x00;  /*stay_off*/
		rlen = sizeof(rData);
		ret = ipmi_cmdraw(0x06 , NETFN_CHAS,
			BMC_SA,PUBLIC_BUS,BMC_LUN,
			pc,1,rData,&rlen,&cc,fdebug);
		if (ret == 0 && cc != 0) ret = cc;
		break;
              case SYS_INFO:  /* System Info */
		if (! fIPMI20) continue;  /*skip if not IPMI 2.0*/
		/* j = #bytes read into LanRecord */
		ret = set_system_info(bset,(char *)LanRecord,j);
		break;
              case LAN_FAILOVER:  /* Intel LAN Failover */
		if (is_romley(vend_id,prod_id)) 
		     ret = lan_failover_intel(LanRecord[0],(uchar *)&i);
        else if ((vend_id == VENDOR_SUPERMICROX) || (vend_id == VENDOR_SUPERMICRO)) {
		   ret = oem_supermicro_set_lan_port(LanRecord[0]);
		   if (fdebug) printf("SMC set_lan_port(%d) = %d\n",LanRecord[0],ret);
		} else continue;  /*skip if not Intel Romley*/
		break;
	      default:
		ret = LAN_ERR_NOTSUPPORT;
	    }
	    printf("SetSystemParam(%d,%d) ret = %d\n",idx,bset,ret);
	    if (ret != 0) { nerrs++; lasterr = ret; }
	    else ngood++;
	 } /*end-else*/

       } /*end-while*/

       /* Disable any users not enabled above */
       for (i = last_user_enable+1; i < max_users; i++) {
		pc[0] = (uchar)i;  /*user number, 1=null_user */
		pc[1] = 0x00;  /*disable user*/
		rlen = sizeof(rData);
		ret = ipmi_cmd(SET_USER_PASSWORD,pc,2, rData,&rlen, &cc,fdebug);
		if (ret == 0 && cc != 0) ret = cc;
		printf("SetUserEnable (%x %x) ret = %d\n",pc[0],pc[1],ret);
	        if (ret != 0) { nerrs++; lasterr = ret; }
	        else ngood++;
       }
   }  /*endif not readonly*/

do_exit:
   if (fd_bmc != NULL && fd_bmc != stdout) fclose(fd_bmc);
   ipmi_close_();
   if (nerrs > 0) {
	printf("Warning: %d ok, %d errors occurred, last error = %d\n",ngood,nerrs,lasterr);
        ret = lasterr;
   }
   // show_outcome(progname,ret); 
   return(ret);
}  /* end main()*/

/* end iconfig.c */
