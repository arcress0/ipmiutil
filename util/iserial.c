/*
 * tmconfig.c
 *
 * This tool sets up the serial EMP port for the Terminal Mode settings.
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2002-2006 Intel Corporation. 
 * 
 * Options: 
 *  -b   Set serial port up for Basic Mode (fBMode)
 *  -B   Set baud rate for serial port
 *  -d   Disable serial port for IPMI use
 *  -f   Flow Control setting, default=1 (RTS/CTS)
 *  -r   Read-only, view serial port parameters
 *  -s   configure serial port for Shared Basic Mode & Remote Console
 *  -t   Configure serial port for shared Terminal Mode (TMode) & Console
 *  -u username  Set username for EMP TMode login (default is null user 1)
 *  -p password  Set password for EMP TMode login 
 *  -l   show LAN parameters also
 *  -m0  switch MUX to Normal baseboard operation
 *  -m1  switch MUX to BMC operation (BMode or TMode)
 *  -x   eXtra debug messages
 *  -f   special boot Flags
 *
 * Change Log:
 * 02/12/02 Andy Cress - created from pefconfig.c
 * 03/08/02 Andy Cress - added GET_USER_ACCESS to serial parameters shown
 * 03/11/02 Andy Cress - changed MUX param 8 value for -b
 * 03/13/02 Andy Cress - changed default behavior to be like -r,
 *                       added -c option to configure TMode
 * 03/15/02 Andy Cress v1.3 added bootflags
 * 04/12/02 Andy Cress v1.4 use always avail instead of shared mode
 *                          also, don't do boot flags if not specified
 * 07/02/02 Andy Cress v1.5 add more Usage, add -d option.
 * 07/25/02 Andy Cress v1.6 fixed passwd offset for fSetTMode (-c)
 * 08/02/02 Andy Cress v1.7 moved common ipmi_cmd() code to ipmicmd.c
 * 12/09/02 Andy Cress v1.8 make sure -d sets user1 to admin again,
 *                          combine -n/-t into -m for MUX,
 *                          fix -c to drop thru to SetTMode logic,
 *                          param 8 values changed,
 * 		            Dont SetUser for TMode unless -u,
 *                          use DCD & RTS/CTS for TMode. 
 * 12/12/02 Andy Cress v1.9  change access from 0x23 to 0x2b
 * 01/23/03 Andy Cress v1.10 decode UserAccess meanings
 * 01/29/03 Andy Cress v1.11 added MV OpenIPMI support
 * 07/31/03 Andy Cress v1.12 added -F to try force it
 * 09/10/03 Andy Cress v1.13 isolate ser_ch, add -n option to specify chan#,
 *                           abort if no serial channels
 * 05/05/04 Andy Cress v1.14 call ipmi_close before exit, added WIN32
 * 06/29/04 Andy Cress v1.15 added fSetPsw even if not fSetUser3
 * 11/01/04 Andy Cress v1.16 add -N / -R for remote nodes   
 * 11/23/04 Andy Cress v1.17 recompile with ipmignu.c changes
 * 01/31/05 Andy Cress v1.18 allow IPMI 2.0 versions
 * 03/18/05 Andy Cress v1.19 fix for -n, show the ser_ch it is using.
 * 05/19/05 Andy Cress v1.20 show 4 users if IPMI20
 * 06/13/05 Andy Cress v1.21 show multiple alert destinations
 * 08/10/05 Andy Cress v1.22 truncate extra string chars
 * 05/02/06 Andy Cress v1.23 add -B option for baud rate
 * 09/12/06 Andy Cress v1.26 use authmask to set Serial Param(2)
 * 09/29/06 Andy Cress v1.27 add -q for user number, add -f for Flow Control,
 *                           adjust Mode if known, clear SerialParam(3) if -d,
 *                           added ShowAccess if -r.
 */
/*M*
Copyright (c) 2002-2006, Intel Corporation
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
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#else
#include <getopt.h>
#endif
#include <string.h>
#endif
#include "ipmicmd.h"
 
#define SELprintf          printf
#define SER_CH    4
#define LAN_CH    1
#define MAXCHAN   16
#define PSW_LEN  16   /*see also PSW_MAX=20 in ipmicmd.h*/
/* Note: The optional IPMI 2.0 20-byte passwords are not supported here,
 * due to back-compatibility issues. */

/* serial channel access values */
#define ALWAYS_AUTH   0x22
#define ALWAYS_NOAUTH 0x2A
#define SHARED_AUTH   0x23
#define SHARED_NOAUTH 0x2B

typedef struct
{
        unsigned short     record_id;
        uchar              record_type;
        int                timestamp;
        unsigned short     generator_id;
        uchar              evm_rev;         //event message revision
        uchar              sensor_type;
        uchar              sensor_number;
        uchar              event_trigger;
        uchar              event_data1;
        uchar              event_data2;
        uchar              event_data3;
}       SEL_RECORD;

typedef struct
{            /* See IPMI Table 19-3 */
        uchar              data[30];
}       SER_RECORD;

/*
 * Global variables 
 */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil serial";
#else
static char * progver   = "3.08";
static char * progname  = "iserial";
#endif
static int    vend_id = 0;
static int    prod_id = 0;
static char   fdebug    = 0;
static char   fpicmg    = 0;
static char   freadonly = 0;
static char   fdoanyway = 0;
static char   fSetUser3 = 0;     /* =1 set User3->admin and User1->user access*/
static char   fSetPsw   = 0;     /* =1 set Password to user parameter */
static char   fSetTMode = 0;     /* =1 configure for TMode */
static char   fBasebdOn = 0;     /* =1 only do MUX switch BMC -> Baseboard */
static char   fTModeOn  = 0;     /* =1 only do MUX switch Baseboard -> BMC */
static char   fBMode    = 0;     /* =1 do Basic mode config stuff */
static char   fShared   = 0;     /* =1 shared Basic mode & Remote console */
static char   fDisable  = 0;     /* =1 to disable serial port IPMI */
static char   fKnownMode = 0;    /* =0 if unknown, =1 if TMode, =2 if BMode */
static char   fgetlan    = 0;
static char   fnotshared = 0;    /* =1 if IPMI serial shared with OS/BIOS*/
static char   fconsoleonly = 0;  /* =1 BIOS console only, no serial mux */
static char   fIPMI20   = 0;	 /* =1 IPMI v2.0 or greater */
static char   fcanonical = 0;	 /* =1 IPMI v2.0 or greater */
static char   fuser1admin = 0;	 /* =1 if default user 1 should be admin */
static char   bdelim    = ':';
static char   bFlow     = 1;     /* Flow Control: 1=RTS/CTS, 0=none */
static char   inactivity = 0;    /* Inactivity Timeout (30 sec increments) */
static char   newbaud    = 0x07;  /* default = 19.2K */
static uchar  bootflags  = 16;    /* if < 16 use these bootflags */
static uchar  ser_ch     = SER_CH;
static uchar  lan_ch     = LAN_CH;  /* usu LAN 1 = 7 */
static uchar  ser_user   = 0x03;    /* if -u specified, configure user 3 */
static uchar  ser_access = 0x04;    /* user priv 4=Admin, 3=Operator, 2=User */
static uchar  usernum    = 0;       /* set non-zero to specify user number */
static uchar  useridx    = 0;       /* user number index for get/show*/
static uchar  max_users     = 5;    /* set in GetUserAccess() if unum==1 */
static uchar  enabled_users = 0;    /* set in GetUserAccess() if unum==1 */
static uchar  show_users    = 5;    /* set in GetUserAccess() if unum==1 */
static char rguser[16]   = "root";     /* default, settable via user param */
static char rgpasswd[PSW_LEN+1] = "password"; /* default, set via user param */

#define NLAN  27
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
 /*  8 */ { 8, 2, "Prim RMCP port "},
 /*  9 */ { 9, 2, "Sec RMCP port "},
 /* 10 */ {10, 1, "BMC grat ARP "},
 /* 11 */ {11, 1, "grat ARP interval"},
 /* 12 */ {12, 4, "Def gateway IP"},
 /* 13 */ {13, 6, "Def gateway MAC"},
 /* 14 */ {14, 4, "Sec gateway IP"},
 /* 15 */ {15, 6, "Sec gateway MAC"},
 /* 16 */ {16,18, "Community string"},
 /* 17 */ {17, 1, "Num dest"},
 /* 18 */ {18, 5, "Dest type"},
 /* 19 */ {19, 13, "Dest address"},
 /* 20 */ {96, 28, "OEM Alert String"},
 /* 21 */ {97,  1, "Alert Retry Algorithm"},
 /* 22 */ {98,  3, "UTC Offset"},
 /* 23 */ {192, 4, "DHCP Server IP"},
 /* 24 */ {193, 6, "DHCP MAC Address"},
 /* 25 */ {194, 1, "DHCP Enable"},
 /* 26 */ {201, 2, "Channel Access Mode (Lan)"}
};

/* Special temp ids for other serial commands that aren't serial params */
#define CMDID_CA1   201  /*this is the first temp id*/
#define CMDID_UA1   202
#define CMDID_UA2   203
#define CMDID_UA3   204
#define CMDID_UA4   205
#define CMDID_MUX   210
#define CMDID_BOOT  211
#define NSER  28   /* was 32 */
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
 /* 20 */ {20, 1, "Number Dial Strings"},
 /* 21 */ {21,33, "Dest Dial String"},
 /* 22 */ {22, 1, "Number Dest IP Addrs"},
 /* 23 */ {23, 5, "Dest IP Address"},
 /* 24 */ {29, 2, "Terminal Mode Config"},
 /* 25 */ {CMDID_MUX, 1,"Get Serial MUX Status"},
 /* 26 */ {CMDID_BOOT, 3,"Get Boot Options(3)"},
 /* 27 */ {CMDID_CA1, 2,"Channel Access Mode (Ser)"}
 /* 28 *% {CMDID_UA1, 4,""},  //"Get User Access (1)", */
 /* 29 *% {CMDID_UA2, 4,""},  //"Get User Access (2)", */
 /* 30 *% {CMDID_UA3, 4,""},  //"Get User Access (3)", */
 /* 31 *% {CMDID_UA4, 4,""}   //"Get User Access (4)"  */
};

static int GetDeviceID(SER_RECORD *pSerRecord)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	int status;
	uchar inputData[24];
	uchar completionCode;

	if (pSerRecord == NULL) return(-1);

        status = ipmi_cmd(GET_DEVICE_ID, inputData, 0, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
			SELprintf("GetDeviceID: completion code=%x\n", 
				completionCode); 
		} else {
			// dont copy first byte (Parameter revision, usu 0x11)
			memcpy(pSerRecord,&responseData[0],responseLength);
			set_mfgid(&responseData[0],responseLength);
			return(0);  // successful, done
		}
	}  /* endif */
	/* if get here, error */
 	return(-1);
}  /*end GetDeviceID() */

static int GetChanAcc(uchar chan, uchar parm, SER_RECORD *pSerRecord)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	int status;
	uchar inputData[24];
	uchar completionCode;

	if (pSerRecord == NULL) return(-1);
	responseLength = 3;
	inputData[0] = chan;
	inputData[1] = parm;  /* 0x80 = active, 0x40 = non-volatile */

        status = ipmi_cmd(GET_CHANNEL_ACC, inputData, 2, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
			SELprintf("GetChanAcc: completion code=%x\n", 
				completionCode); 
		} else {
			memcpy(pSerRecord,&responseData[0],responseLength);
			return(0);  // successful, done
		}
	}  /* endif */
	/* if get here, error */
 	return(-1);
}  /*GetChanAcc()*/

static int SetChanAcc(uchar chan, uchar parm, uchar val)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	int status;
	uchar inputData[24];
	uchar completionCode;

        /* parm: 0x80 = active, 0x40 = set non-vol*/
	responseLength = 1;
	inputData[0] = chan;  /* channel */
	inputData[1] = (parm & 0xc0) | (val & 0x3F); 
	if (chan == lan_ch) inputData[2] = 0x04;  /* LAN, don't set priv level*/
	else inputData[2] = (parm & 0xc0) | 0x04;  /* set Admin priv level */
                     /* serial defaults to 0x02 = User priv level */

        status = ipmi_cmd(SET_CHANNEL_ACC, inputData, 3, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
			SELprintf("SetChanAcc: completion code=%x\n", 
				completionCode); 
		} else {
			return(0);  // successful, done
		}
	}  /* endif */
	/* if get here, error */
 	return(-1);
}  /*SetChanAcc()*/

static int GetSerEntry(uchar subfunc, uchar bset, SER_RECORD *pSerRecord)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24];
	int status;
	uchar completionCode;
	uchar chan; 

	if (pSerRecord == NULL)
	{
	   if (fdebug)
	       printf("GetSerEntry: error, output buffer is NULL\n");
 	   return (-1);
	}

        chan = ser_ch;  /* sample: 0=IPMB, 1=EMP/serial, 6=LAN2, 7=LAN1 */

	inputData[0]            = chan;  // flags, channel 3:0 (1=EMP)
	inputData[1]            = subfunc;  // Param selector 
	inputData[2]            = bset;  // Set selector 
	inputData[3]            = 0;  // Block selector
	if (subfunc == 10)  {  /*modem init string*/
	   inputData[3] = 1;  /*blocks start with 1*/
 	} else if (subfunc == 21) {
	   inputData[3] = 1;  /*blocks start with 1*/
	}

        status = ipmi_cmd(GET_SER_CONFIG, inputData, 4, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
		   if (completionCode == 0x80) 
			SELprintf("GetSerEntry(%d): Parameter not supported\n", 
				  subfunc); 
		   else
			SELprintf("GetSerEntry(%d): completion code=%x\n", 
				subfunc,completionCode); // responseData[0]);
		} else {
			// dont copy first byte (Parameter revision, usu 0x11)
			memcpy(pSerRecord,&responseData[1],responseLength-1);
			pSerRecord->data[responseLength-1] = 0;
			//successful, done
			return(0);
		}
	}

	// we are here because completionCode is not COMPLETION_CODE_OK
	if (fdebug) 
	     printf("GetSerEntry: ipmi_cmd status=%d, completion code=%d\n",
                          status,completionCode);
 	return -1;
}

static int SetSerEntry(uchar subfunc, SER_RECORD *pSerRecord, int reqlen)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24];
	int status;
	uchar completionCode;

	if (pSerRecord == NULL)
	{
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

	if (status == ACCESS_OK) {
		if( completionCode ) {
		   if (completionCode == 0x80) 
			SELprintf("SetSerEntry(%d): Parameter not supported\n", 
				  subfunc); 
		   else
			SELprintf("SetSerEntry(%d): completion code=%x\n", 
				subfunc, completionCode);
		} else {
			//successful, done
			return(0);
		}
	}

	// we are here because completionCode is not COMPLETION_CODE_OK
	if (fdebug) 
	     printf("SetSerEntry: ipmi_cmd status=%d, completion code=%d\n",
                          status,completionCode);
 	return -1;
}  /* end SetSerEntry() */

#ifdef NEEDED
int SerialIsOptional(uchar bparam)
{
   /* These Serial Parameters are for optional Modem/Callback functions. */
   uchar optvals[9] = { 5, 9, 10, 11, 12, 13, 14, 20, 21 };
   int rv = 0;
   int i;
   for (i = 0; i < sizeof(optvals); i++) {
      if (optvals[i] == bparam) { rv = 1; break; }
   }
   return(rv);
}
#endif

int SetMiscEntry(ushort icmd, SER_RECORD *pSerRecord, int reqlen)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24];
	int status;
	uchar completionCode;

	if (pSerRecord == NULL) {
	   if (fdebug) printf("SetMiscEntry: error, input buffer is NULL\n");
 	   return (-1);
	}
	memcpy(&inputData[0],pSerRecord,reqlen);

        status = ipmi_cmd(icmd, inputData, (uchar)reqlen, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
			printf("SetMiscEntry(%04x): completion code=%x\n", 
				icmd,completionCode); 
		} else {  //successful
			if (responseLength > 1) {
			  // skip first byte (Parameter revision, usu 0x11)
			  memcpy(pSerRecord,&responseData[1],responseLength-1);
			  }
			return(0);
		}
	}

	// we are here because completionCode is not COMPLETION_CODE_OK
	if (fdebug) 
	     printf("SetMiscEntry: ipmi_cmd status=%d, completion code=%d\n",
                          status,completionCode);
 	return -1;
}  /* end SetMiscEntry() */

int GetMiscEntry(ushort icmd, SER_RECORD *pSerRecord, int reqlen)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24];
	int status;
	uchar completionCode;

	if (pSerRecord == NULL) {
	   if (fdebug) printf("GetMiscEntry: error, output buffer is NULL\n");
 	   return (-1);
	}

	memcpy(&inputData[0],pSerRecord,reqlen);
        status = ipmi_cmd(icmd, inputData, (uchar)reqlen, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
		   if (completionCode == 0x80) 
			printf("GetMiscEntry(%d): Parameter not supported\n", 
				 icmd); 
		   else
			printf("GetMiscEntry(%d): completion code=%x\n", 
				icmd,completionCode); 
		} else {     //successful
			if (responseLength > 1) 
			  memcpy(pSerRecord,&responseData[0],responseLength);
			return(0);
		}
	}

	if (fdebug) 
	    printf("GetMiscEntry(%d): ipmi_cmd status=%d, completion code=%x\n",
                        icmd,status,completionCode);
 	return -1;
}  /* end GetMiscEntry() */

void ShowUserAccess(uchar unum, uchar c0, uchar c1, uchar c2, uchar c, 
			char *name)
{ 
	/* channel is always ser_ch */
	if (unum == 1 && !fcanonical) {
	   printf("Users: showing %d of max %d users (%d enabled)\n",
		show_users,max_users,enabled_users);
	}
	printf("Get User Access(%d)%c %02x %02x %02x %02x%c ",
			unum,bdelim,c0,c1,c2,c,bdelim);
	if (c & 0x10) printf("IPMI, ");
	c = (c & 0x0f);
	switch(c) {
			case 1:    printf("Callback"); break;
			case 2:    printf("User "); break;
			case 3:    printf("Operator"); break;
			case 4:    printf("Admin"); break;
			case 5:    printf("OEM "); break;
			case 0x0f: printf("No access"); break;
			default:   printf("Reserved"); 
	}
        printf(" (%s)\n",name);  
}

int GetUserAccess(uchar unum, uchar *rdata, char *name)
{
    int rv;
    /* rdata must be at least 2 bytes, and name must be at least 16 bytes */
    rdata[0] = ser_ch;  /* channel# */
    rdata[1] = unum;  /* user# */
    rv = GetMiscEntry(GET_USER_ACCESS, (SER_RECORD *)rdata, 2);
    if (rv == 0 && name != NULL) {
       name[0] = unum;  /* user# */
       rv = GetMiscEntry(GET_USER_NAME, (SER_RECORD *)name, 1);
       if (rv != 0) name[0] = 0;
       rv = 0;
    }
    if (unum == 1) { /*get max_users and enabled_users*/
        max_users = rdata[0] & 0x3f;
        enabled_users = rdata[1] & 0x3f;
        // if (enabled_users > show_users) show_users = enabled_users;
	if (show_users > max_users) show_users = max_users;
    }
    if (rv == 0) 
         ShowUserAccess(unum,rdata[0],rdata[1],rdata[2],rdata[3],name);
    else printf("Get User Access(%d): error %d\n",unum,rv);
    return(rv);
}

static int GetLanEntry(uchar subfunc, SER_RECORD *pSerRecord)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24];
	int status;
	uchar completionCode;
	uchar chan; uchar bset;

	if (pSerRecord == NULL)
	{
	   if (fdebug)
	       printf("GetLanEntry: error, output buffer is NULL\n");
 	   return (-1);
	}

        chan = lan_ch;
	if (subfunc == 18 || subfunc == 19) bset = 1;  /* dest id = 1 */
	else bset = 0;

	inputData[0]            = chan;  // flags, channel 3:0 (LAN 1)
	inputData[1]            = subfunc;  // Param selector (3 = ip addr)
	inputData[2]            = bset;  // Set selector 
	inputData[3]            = 0;  // Block selector

        status = ipmi_cmd(GET_LAN_CONFIG, inputData, 4, responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
			SELprintf("GetLanEntry: completion code=%x\n", 
				completionCode); // responseData[0]);
		} else {
			// dont copy first byte (Parameter revision, usu 0x11)
			memcpy(pSerRecord,&responseData[1],responseLength-1);
			pSerRecord->data[responseLength-1] = 0;
			//successful, done
			return(0);
		}
	}

	// we are here because completionCode is not COMPLETION_CODE_OK
	if (fdebug) 
	     printf("GetLanEntry: ipmi_cmd status=%d, completion code=%d\n",
                          status,completionCode);
 	return -1;
}  /* end GetLanEntry() */

#ifdef NOT
static int SetLanEntry(uchar subfunc, SER_RECORD *pSerRecord, int reqlen)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar inputData[24];
	int status;
	uchar completionCode;

	if (pSerRecord == NULL)
	{
	   if (fdebug)
	       printf("SetLanEntry: error, input buffer is NULL\n");
 	   return (-1);
	}

	inputData[0]            = lan_ch;  // flags, channel 3:0 (LAN 1)
	inputData[1]            = subfunc;  // Param selector (3 = ip addr)
	memcpy(&inputData[2],pSerRecord,reqlen);

        status = ipmi_cmd(SET_LAN_CONFIG,inputData,(uchar)(reqlen+2),responseData,
                        &responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK) {
		if( completionCode ) {
			SELprintf("SetLanEntry: completion code=%x\n", 
				completionCode); // responseData[0]);
		} else {
			//successful, done
			return(0);
		}
	}

	// we are here because completionCode is not COMPLETION_CODE_OK
	if (fdebug) 
	     printf("SetLanEntry: ipmi_cmd status=%d, completion code=%d\n",
                          status,completionCode);
 	return -1;
}  /* end SetLanEntry() */
#endif

static int ReadSELinfo()
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	uchar completionCode;
	uchar inputData[6];
	int status;

        status = ipmi_cmd(GET_SEL_INFO, inputData, 0, responseData, 
			&responseLength, &completionCode, fdebug); 

	if (status == ACCESS_OK)
	{
		if (fdebug)
                  SELprintf("Code %d SEL Ver %d Support %d\n",
                          completionCode,
                          responseData[0],
                          responseData[13]
                          );
		//successful, done
		return(0);
	}
        else    return(1);

}  /*end ReadSELinfo()*/

#define NBAUDS  10
static struct {
       unsigned char val;
       char str[8];
    } mapbaud[NBAUDS] = {
       { 6,  "9600" },
       { 6,  "9.6K" },
       { 7,  "19.2K" },
       { 7,  "19200" },
       { 8,  "38.4K" },
       { 8,  "38400" },
       { 9,  "57.6K" },
       { 9,  "57600" },
       { 10, "115.2K" },
       { 10, "115200" }
    };

static unsigned char Str2Baud(char * str)
{
   unsigned char baud = 0;
   int i, n, len;
   len = (int)strlen(str);
   for (i = 0; i < len; i++)  /*toupper*/
      if (str[i] >= 'a' && str[i] <= 'z') str[i] &= 0x5F;
   for (i = 0; i < NBAUDS; i++) {
       n = (int)strlen(mapbaud[i].str);
       if (strncmp(str,mapbaud[i].str,n) == 0) {
           baud = mapbaud[i].val;
           break;
       }
   }
   if (i == NBAUDS || baud == 0) {
       printf("Invalid -B parameter value (%s), using 19.2K.\n",str);
       i = 1; /* default is 19.2K */
       baud = mapbaud[i].val; /* =7 */
   }
   if (fdebug) printf("new baud = %02x (%s)\n",baud,mapbaud[i].str);
   return(baud);
}

static char *Baud2Str(unsigned char bin)
{
    char *baudstr;
    unsigned char b;
    b = bin & 0x0f;
    switch(b) {
	case 6:  baudstr = "9600 "; break;
	case 7:  baudstr = "19.2k"; break;
	case 8:  baudstr = "38.4k"; break;
	case 9:  baudstr = "57.6k"; break;
	case 10: baudstr = "115.2k"; break;
	default: baudstr = "nobaud";
    }
    return(baudstr);
}

void ShowChanAccess(uchar chan, char *tag, uchar access, uchar access2)
{
    printf("Channel Access Mode(%d=%s)%c %02x %02x %c ",
		chan,tag,bdelim,access,access2,bdelim);
    switch (access & 0x03) {
        case 0: printf("Access = Disabled, ");     break;
        case 1: printf("Access = Pre-Boot, ");     break;
        case 2: printf("Access = Always Avail, "); break;
        case 3: printf("Access = Shared, ");       break;
        default:  break;
    }
    if (access & 0x20) printf("PEF Alerts Disabled\n"); /*0*/
    else printf("PEF Alerts Enabled\n");   /*1*/
}

static int valid_priv(int c)
{
	int rv;
	switch(c) {
		case 1:  
		case 2:  
		case 3:   
		case 4:   
		case 5:   
		case 0x0f:
			rv = 1; break;
		default:   
			rv = 0; break;
	}
	return(rv);
}

#ifdef METACOMMAND
int i_serial(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int  argc, char **argv)
#endif
{
   int ret;
   SER_RECORD SerRecord;
   char uname[16];
   int i, idx;
   int c;
   int rlen;
   uchar iData[2];
   uchar rData[10];
   uchar cc;
   int j;
   uchar bset;
   uchar nalert, ndial, ndest;
   uchar ialert, idial, idest;
   uchar authmask = 0;
   char mystr[80];

   // progname = argv[0];
   printf("%s ver %s \n",progname,progver);

   if (argc <= 1) freadonly = 1;  /* default to readonly if no options */

   while ( (c = getopt( argc, argv,"abcdef:gi:lm:n:p:#:q:rstu:v:xB:F:T:V:J:EYP:N:R:U:X:Z:?")) != EOF ) 
      switch(c) {
          case 'x': fdebug = 1;     break;
          case 'r': freadonly = 1;  break;
          case 'l': fgetlan = 1;    break;
          case 'd': fDisable = 1; freadonly = 0;  break;
          case 'a': fcanonical = 1;  /* canonical mode with delimeter */
		    bdelim = BDELIM; break;
          case 'c':                  /* configure Terminal Mode & Console */
          case 't': fSetTMode = 1;         /* set Terminal Mode & Console */
                    freadonly = 0; break; 
          case 'e': fSetTMode = 1;         /* set Terminal Mode not shared*/
		    fnotshared = 1;        /* no BIOS console, not shared */
                    freadonly = 0; break; 
          case 'g': fSetTMode = 1;         /* set Terminal Mode, but BCR only */
		    fconsoleonly = 1;      /* BIOS console only, disable mux */
                    freadonly = 0; break; 
          case 'b': fBMode = 1;            /* set Basic Mode only */
                    freadonly = 0; break; 
          case 's': fBMode = 1;            /* set Basic Mode & Console */
		    fShared = 1; freadonly = 0;
		    break;
          case 'f': i = atoi(optarg);  /*flow control*/
                    if (i >= 0 && i < 2) bFlow = (uchar)i;
                    break;
          case 'i': i = atoi(optarg);    /*inactivity timeout*/
                    if (i > 0 && i < 30) inactivity = 1;
                    else inactivity = i / 30;
                    break;
          case 'B': newbaud = Str2Baud(optarg);  break;
          case 'm': 		    /* set MUX to specific value */
		    i = atoi(optarg);
		    if (fdebug) printf("MUX Mode %d\n",i);
		    switch(i) {
			case 0: fBasebdOn = 1; break;
			case 1: fTModeOn = 1;  break;
			default: fBasebdOn = 1;
		    }
		    break;
          case 'u': 
		    strncpy(rguser,optarg,16);
		    rguser[15] = 0;
		    /* Use specified user #3, and set user 1 to not admin */
		    fSetUser3 = 1; 
		    break;
          case 'p': 
		    strncpy(rgpasswd,optarg,PSW_LEN);
		    rgpasswd[PSW_LEN] = 0;
		    fSetPsw = 1;
                    /* Hide password from 'ps' */
                    memset(optarg, ' ', strlen(optarg));
		    break;
          case 'q': 
          case '#': 
		    usernum = atob(optarg);
                    if (usernum > 15) usernum = 0;  /*MAX_IPMI_USERS = 4*/
		    break;
          case 'n': 
		    ser_ch = atob(optarg);
		    if (ser_ch > MAXCHAN) ser_ch = SER_CH; /*default=1*/
		    break;
          case 'v':    /*user privilege level access */
		    i = atoi(optarg);
		    if (valid_priv(i)) ser_access = i & 0x0f;
		    else printf("Invalid privilege -v %d, using Admin\n",i);
		    break;
          case 'X': 
		    bootflags = atob(optarg);
		    fdoanyway = 1;   /* undocumented, for test only */
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
                printf("Usage: %s [-bcdefglmnq#rstxB -u user -p passwd -NUPREFTVY]\n",progname);
		printf("   -b   configure serial port for Basic Mode\n");
		printf("   -c   Configure serial port for shared Terminal Mode & Console\n");
		printf("   -d   Disable serial port for IPMI use.\n");
		printf("   -e   configure serial port for Terminal Mode only\n");
		printf("   -f0  Flow control:  0=none, 1=RTS/CTS(default)\n");
		printf("   -g   configure serial port for console only, with BMode CTS\n");
		printf("   -s   configure serial port for Shared Basic Mode & Remote Console\n");
		printf("   -l   show some LAN parameters also\n");
		printf("   -m0  switch MUX to Normal baseboard operation\n");
		printf("   -m1  switch MUX to BMC operation (BMode or TMode)\n");
		printf("   -B bval      set Baud to bval for serial port\n");
		printf("   -n ser_chan  serial channel Number (default=1)\n");
		printf("   -p password  set Password for EMP TMode login\n");
		printf("   -u username  set Username for EMP TMode login\n");
                printf("   -q/-#        User number of Username for EMP\n"); 
		printf("   -r   Read-only, view serial port parameters (default)\n");
		printf("   -t   Configure serial port for shared Terminal Mode & Console (same as -c)\n");
		printf("   -v4  access priVilege: 4=Admin, 3=Operator, 2=User\n");
		printf("   -x   eXtra debug messages\n");
		print_lan_opt_usage(0);
                ret = ERR_USAGE;
		goto do_exit;
      }

   if (is_remote() && !freadonly) parse_lan_options('V',"4",0);

   ret = GetDeviceID( &SerRecord);
   if (ret != 0) {
      goto do_exit;
   } else {
      uchar ipmi_maj, ipmi_min;
      uchar *devrec;
 
      devrec = &SerRecord.data[0];
      ipmi_maj = devrec[4] & 0x0f;
      ipmi_min = devrec[4] >> 4;
      vend_id = devrec[6] + (devrec[7] << 8) + (int)(devrec[8] << 16);
      prod_id = devrec[9] + (devrec[10] << 8);
      show_devid( devrec[2],  devrec[3], ipmi_maj, ipmi_min);
      if ((ipmi_maj == 0) || (ipmi_maj == 1 && ipmi_min < 5)) {
	 /* IPMI 0.9 and 1.0 dont support this. */
         printf("This system does not support EMP Terminal Mode.\n");
         if (!fdoanyway) {
            ret = LAN_ERR_NOTSUPPORT;
            goto do_exit;
         }
      }
      if (ipmi_maj >= 2) fIPMI20 = 1;
      /* Determine if Basic or Terminal Mode is supported by product id */
      if (vend_id == VENDOR_INTEL) { /*Intel*/
         switch(prod_id) {
            case 0x4311: fKnownMode = 1; break;  /*NSI2U,   BMode*/
            case 0x000C: fKnownMode = 1; break;  /*TSRLT2,  BMode*/
            case 0x0100: fKnownMode = 1; break;  /*Tiger2,  BMode*/
            case 0x001B: fKnownMode = 2; break;  /*TIGPR2U, TMode*/
            case 0x0022: fKnownMode = 2; break;  /*TIGI2U,  TMode*/
            case 0x0026: fKnownMode = 2; break;  /*Alcolu,  TMode*/
            case 0x0028: fKnownMode = 2; break;  /*Alcolu,  TMode*/
            case 0x0811: fKnownMode = 2; break;  /*TIGW1U,  TMode*/
            case 0x003E: fKnownMode = 2;         /*NSN2U/CG2100, TMode*/
	        set_max_kcs_loops(URNLOOPS); /*longer KCS timeout*/
		break; 
            default:   break;
         }
      } else if (vend_id == VENDOR_NSC) { /*NSC*/
         if (prod_id == 0x4311) fKnownMode = 1;  /*TIGPT1U, No EMP*/
      } /* else   * other vend_id, leave defaults */
      if (fKnownMode > 0) {  
         /* Adjust if we know that the user set the wrong one. */
         if (fKnownMode == 1 && fSetTMode == 1) {
            printf("Adjusting from Terminal Mode to Basic Mode\n");
            fSetTMode = 0;  fBMode = 1; fShared = 1;
         } else if (fKnownMode == 2 && fBMode == 1) {
            printf("Adjusting from Basic Mode to Terminal Mode\n");
            fSetTMode = 1;  fBMode = 0;
         } 
      }
   }  /*end-else have device_id */

   ret = ipmi_getpicmg( &SerRecord.data[0], 16, fdebug);
   if (ret == 0) fpicmg = 1;

   ret = ReadSELinfo();
   if (ret == 0) {   /* talking to BMC ok */

      /* find the first Serial channel via Channel Info */
      for (j = 1; j < MAXCHAN; j++) {
        iData[0] = (uchar)j;  /*channel #*/
        rlen = sizeof(rData);
        ret = ipmi_cmd(GET_CHANNEL_INFO, iData, 1, rData, &rlen, &cc, 0); 
	if (ret != 0) break;
	if (rData[1] == 4) { 		/* 4 = LAN type*/
		if (fdebug) printf("chan[%d] = lan\n",j);
		lan_ch = (uchar)j;
	} else if (rData[1] == 5) {     /* 5 = Serial type*/
		if (fdebug) printf("chan[%d] = serial\n",j);
		if (ser_ch != SER_CH) { /* user set it */
		   if (j == ser_ch) break;
		} else {   /* find the first serial channel */
		   ser_ch = (uchar)j;
		   break;
		}
	} else  /* 7 = SMBus, 12 = System Interface */
		if (fdebug) printf("chan[%d] = %d\n",j,rData[1]);
      }
      if (fdebug) printf("ser_ch = %d\n",ser_ch);
      if (j >= MAXCHAN) {
	  printf("No serial channel support found (channel %d)\n",ser_ch);
          ret = LAN_ERR_NOTSUPPORT;
          goto do_exit;
      }

      if (fdebug && fSetUser3 == 1)  
         printf("user %d/%d: username=%s, password=%p\n",
			usernum,ser_user, rguser,rgpasswd);

      if (fgetlan && !fcanonical) {
       int ival;
       printf("%s, GetLanEntry for channel %d ...\n",progname,lan_ch);
       for (idx = 0; idx < NLAN; idx++)
       {
	 ival = lanparams[idx].cmd;
         if (ival == 8 || ival == 9) continue;	  /* not implemented */
         if (ival >= 96 && ival <= 98) continue;  /* not implemented */
         if (ival == CMDID_CA1) {
             ret = GetChanAcc(lan_ch, 0x40, &SerRecord);
	 } else {
             ret = GetLanEntry((uchar)ival, &SerRecord);
	 }
         if (ret == 0) {    // Show the LAN record
            uchar * pc; int sz;
            pc = (uchar *)&SerRecord;
            sz = lanparams[idx].sz; 
            printf("Lan Param(%d) %s%c ",ival,lanparams[idx].desc,bdelim);
	    if (ival == 16) { printf("%s \n",pc);      /* string */
	    } else {  /* print results for others */
              for (i = 0; i < sz; i++) {
	  	if (ival == 3 || ival == 6 || ival == 12 || ival == 14 ||
		    ival == 192)
		   printf("%d ",pc[i]);      /* IP addresses in dec */
		else printf("%02x ",pc[i]);  /* show in hex */
		}
	      /* if (ival == 4) { pc[0]: 01 = static, 02 = DHCP } */
              printf("\n");
	    }
         } else  /* ret != 0 */
            printf("GetLanEntry(%d), ret = %d\n",ival,ret);
        }  /*end for*/
       } 

       printf("%s, GetSerEntry for channel %d ...\n",progname,ser_ch);
       nalert = 1; ialert = 1;
       ndial = 1;  idial = 1;
       ndest = 1;  idest = 1;
       for (idx = 0; idx < NSER; idx++) {
   	 int ival;
	 ival = serparams[idx].cmd;
         /* 
          * ival is either the Serial Param# or a temp id.
          * Do the appropriate Get command, based on ival.
          */
         if (ival == CMDID_CA1) {
             ret = GetChanAcc(ser_ch, 0x40, &SerRecord);
         } else if (ival == CMDID_UA1) {
	     useridx = 1;
             ret = GetUserAccess(useridx,(uchar *)&SerRecord,uname);
         } else if (ival == CMDID_UA2) {
	     useridx = 2;
             ret = GetUserAccess(useridx,(uchar *)&SerRecord,uname);
         } else if (ival == CMDID_UA3) {
	     useridx = 3;
             ret = GetUserAccess(useridx,(uchar *)&SerRecord,uname);
         } else if (ival == CMDID_UA4) {
	     useridx = 4;
	     if (fIPMI20 == 0) continue;  /* IPMI 1.5 only has 3 users */
             ret = GetUserAccess(useridx,(uchar *)&SerRecord,uname);
         } else if (ival == CMDID_MUX) {
	     SerRecord.data[0] = ser_ch;   /* channel 1 = EMP */
	     SerRecord.data[1] = 0x00;   /* just get MUX status */
             ret = SetMiscEntry(SET_SER_MUX, &SerRecord, 2);
         } else if (ival == CMDID_BOOT) {
	     SerRecord.data[0] = 0x03;   /* Boot Param 3 */
	     SerRecord.data[1] = 0x00;   /* Set Selector */
	     SerRecord.data[2] = 0x00;   /* %%%% */
             ret = GetMiscEntry(GET_BOOT_OPTIONS, &SerRecord, 3);
	 } else {
	     if (fcanonical) {
		switch(ival) {  /*only show certain parameters*/
		    case 3:
		    case 4:
		    case 6:
		    case 7:
		    case 8:
		    case 29:
			break;
		    default:
			continue; break;
		}
	     }
             if (vend_id == VENDOR_SUPERMICROX && ival == 8) continue;
             if (vend_id == VENDOR_SUPERMICRO && ival == 8) continue;
	     if (ival == 17 || ival == 19) bset = ialert;
	     else if (ival == 21)          bset = idial;
	     else if (ival == 23)          bset = idest;
	     else bset = 0;  /*default*/
             ret = GetSerEntry((uchar)ival, bset, &SerRecord);
	 }
         /* 
          * Now the Get command is done, check the status, and
          * show the raw output.
          */
         if (ret == 0) {    // Show the SER record
           uchar * pc; int sz;
           pc = (uchar *)&SerRecord;
           sz = serparams[idx].sz; 
           /* save some input data for later reuse */
	   if (ival == 16) nalert = pc[0];
	   else if (ival == 20) ndial = pc[0];
	   else if (ival == 22) ndest = pc[0];
           if (ival == CMDID_CA1)  ;
           else if (ival > CMDID_CA1) 
		printf("%s%c ",serparams[idx].desc,bdelim);
           else
	       printf("Serial Param(%d) %s%c ",ival,serparams[idx].desc,bdelim);
	   if (ival == 1) {  /* Auth Type support*/
                authmask = pc[0]; /* auth type support mask */
                mystr[0] = 0;
                if (authmask & 0x01) strcat(mystr,"None ");
                if (authmask & 0x02) strcat(mystr,"MD2 ");
                if (authmask & 0x04) strcat(mystr,"MD5 ");
                if (authmask & 0x10) strcat(mystr,"Pswd ");
                if (authmask & 0x20) strcat(mystr,"OEM ");
                printf("%02x %c %s\n",authmask,bdelim,mystr);
	        } 
           else if (ival == 10) {  /* modem init string */
		pc[sz] = 0;
		printf("%02x %s\n",pc[0],&pc[1]); 
		}
	   else if (ival == 21) {  /* modem dial string */
		pc[sz] = 0;
		printf("%02x %02x %s\n",pc[0],pc[1],&pc[2]); 
		}
	   else if ((ival >= 11 && ival <= 13) || ival == 15) {  /* strings */
		pc[sz] = 0;
		printf("%s\n",pc); 
		}
	   else if (ival == 23) {  /* Dest IP Address */
		printf("%02x %d %d %d %d\n", pc[0], /*dest index*/
			pc[1],pc[2],pc[3],pc[4]);
		}
	   else if (ival == CMDID_CA1) {  /* Channel Access */
                ShowChanAccess(ser_ch,"Ser",pc[0],pc[1]);
	        } 
	   else if (ival == CMDID_UA1 || ival == CMDID_UA2 || 
                     ival == CMDID_UA3 || ival == CMDID_UA4) {
                // ShowUserAccess(useridx,pc[0],pc[1],pc[2],pc[3],uname);
	        } /*endif UA*/
	   else   /* hex data */
           {
            for (i = 0; i < sz; i++) {
		printf("%02x ",pc[i]);  /* show in hex */
		}
            /* 
             * Raw hex data display is complete, now do post-processing. 
             * Interpret some hex data.
             */
            if (ival == 4) {  /* Session Inactivity Timeout */
		c = pc[0];
		if (c == 0) strcpy(mystr,"infinite");
		else sprintf(mystr,"%d sec",(c * 30));
                printf(" %c %s",bdelim,mystr);
	        } 
	    else if (ival == 7) {  /*IPMI Msg Comm*/
	      uchar v;
	      char *flow; char *dtr; char *baud;
	      v = pc[0] & 0xc0;
	      switch(v) {
		case 0x40: flow = "RTS/CTS"; break;
		case 0x80: flow = "XON/XOFF"; break;
		case 0x00: 
		default:   flow = "no_flow";
	      }
	      if ((pc[0] & 0x20) == 0) dtr="no_DTR";
	      else dtr = "DTR";
              baud = Baud2Str(pc[1]);
	      printf("%c %s, %s, %s",bdelim,flow,dtr,baud);
	    } 
	    else if (ival == 19) {  /*Dest Comm Settings*/
	      uchar v;
	      char *flow; char *baud;
	      uchar chsz,parity,stopb;
	      v = pc[1] & 0xc0;
	      switch(v) {
		case 0x40: flow = "RTS/CTS"; break;
		case 0x80: flow = "XON/XOFF"; break;
		case 0x00: 
		default:   flow = "no_flow";
	      }
	      if ((pc[1] & 0x10) == 0) stopb = 1;
	      else stopb = 2;
	      if ((pc[1] & 0x08) == 0) chsz = 8;
	      else chsz = 7;
	      v = pc[1] & 0x07; 
	      switch(v) {
		case 1:  parity = 'O'; break;
		case 2:  parity = 'E'; break;
		case 0:  
		default: parity = 'N';
	      }
              baud = Baud2Str(pc[2]);
	      printf("%c %s, %d%c%d, %s",bdelim,flow,chsz,parity,stopb,baud);
	    } 
#ifdef TEST
	    /* This MUX reading is volatile and may not be accurate */
            else if (ival == CMDID_MUX) {
		    if ((pc[0] & 0x01) == 1) printf("%c BMC",bdelim);
		    else printf("%c System",bdelim); /*BIOS*/
	    }
#endif
            printf("\n");
	   } /*end else hex*/
	 }  /*endif show*/
	 if (ival ==  17 || ival == 19) {   /* alert params */
            if (ialert < nalert) {
               ialert++;
               idx--;  /* repeat this param*/
            } else ialert = 1;
	 }
	 if (ival ==  21) {        /* dial param */
            if (idial < ndial) {
               idial++;
               idx--;  /* repeat this param*/
            } else idial = 1;
	 }
	 if (ival ==  23) {     /* dest param */
            if (idest < ndest) {
               idest++;
               idx--;  /* repeat this param*/
            } else idest = 1;
	 }
       }  /*end for(idx) serial params*/

       for (i = 1; i <= show_users; i++)
           ret = GetUserAccess((uchar)i,(uchar *)&SerRecord,uname);

       if (!freadonly) {    /* set serial channel parameters */
	uchar access;
	if (fBasebdOn) {  /* -m0 */
	 SerRecord.data[0] = ser_ch;   /* channel 1 = EMP */
	 SerRecord.data[1] = 0x03;   /* force switch to Baseboard */
         ret = SetMiscEntry(SET_SER_MUX, &SerRecord, 2);
         printf("SetSerialMux(System), ret = %d, value = %02x\n",
		ret,SerRecord.data[0]);
	} else if (fTModeOn) {  /* -m1 */
	 SerRecord.data[0] = ser_ch;   /* channel 1 = EMP */
	 SerRecord.data[1] = 0x04;   /* force switch to BMC for TMode */
         ret = SetMiscEntry(SET_SER_MUX, &SerRecord, 2);
         printf("SetSerialMux(BMC), ret = %d, value = %02x\n",
		ret,SerRecord.data[0]);

	} else if (fDisable) {  /* -d */
	 /*
          * DISABLE SERIAL CHANNEL ACCESS 
          */
         if (vend_id == VENDOR_INTEL && fuser1admin) {
	    /* First, make sure null user (1) has admin privileges again */
	    SerRecord.data[0] = 0x90 | ser_ch;   /*0x91*/
	    SerRecord.data[1] = 0x01; /*user 1*/
	    SerRecord.data[2] = ser_access;  /* usu 0x04 admin access*/
	    SerRecord.data[3] = 0x00; 
            ret = SetMiscEntry(SET_USER_ACCESS, &SerRecord, 4);
            printf("SetUserAccess(1/%02x), ret = %d\n",ser_access,ret);
	 }
         if (usernum != 0) {   /*disable usernum if specified*/
	    SerRecord.data[0] = 0x80 | ser_ch;   /*0x81*/
	    SerRecord.data[1] = usernum;
	    SerRecord.data[2] = 0x0F; /*No access*/
	    SerRecord.data[3] = 0x00; 
            ret = SetMiscEntry(SET_USER_ACCESS, &SerRecord, 4);
            printf("SetUserAccess(%d/none), ret = %d\n",usernum,ret);
         }
         ret = SetChanAcc(ser_ch, 0x80, 0x20); /*vol  access=disabled*/
         if (fdebug) printf("SetChanAcc(ser/disable), ret = %d\n",ret);
         ret = SetChanAcc(ser_ch, 0x40, 0x20); /*nvol access=disabled*/
         printf("SetChanAcc(ser/disable), ret = %d\n",ret);
         ret = GetChanAcc(ser_ch, 0x40, &SerRecord);
	 access = SerRecord.data[0];
         printf("GetChanAcc(ser/disable), ret = %d, new value = %02x\n",
		ret,access);
	 if ((vend_id != VENDOR_SUPERMICROX) &&
	     (vend_id != VENDOR_SUPERMICRO)) {
            /* Also disable the Serial Parameter 3: Connection Mode */
	    SerRecord.data[0] = 0x00; /* Connection Mode = none */
            ret = SetSerEntry(3, &SerRecord,1);
            printf("SetSerEntry(3/disable), ret = %d\n",ret);
	 }

	} else if (fSetTMode || fBMode) {
	 /*
	  * ENABLE SERIAL CHANNEL ACCESS
	  * Enable: configure everything for TMode and/or BMode 
          *
	  * Set Serial/EMP Channel Access
	  * Use serial channel in shared mode 
	  * 0x20 means alerting=off, disabled (system only)
	  * 0x22 means alerting=off, always avail
	  *      which would only show BMC messages, not OS messages
	  * 0x23 means alerting=off, always avail + shared + user-auth on
	  * 0x2A means alerting=off, always avail + user-auth off
	  * 0x2B means alerting=off, always avail + shared + user-auth off
	  * 0x0B means alerting=on,  always avail + shared + user-auth off
	  * SSU won't set 0x2B, but we need shared access mode.
	  */
	 if (fconsoleonly) {
	   access = ALWAYS_NOAUTH;  /*0x2A*/
	 } else if (fnotshared) {  /* serial IPMI, but no console (-e only) */
	   if (fSetTMode) access = ALWAYS_NOAUTH;  /*0x2A*/
	   else access = ALWAYS_AUTH;  /*0x22*/
	 } else {     /* shared with BIOS console, normal */
	   if (fSetTMode) access = SHARED_NOAUTH;  /*0x2B*/
	   else access = SHARED_AUTH;  /*0x23*/
	 }
         ret = SetChanAcc(ser_ch, 0x80, access);  
         if (fdebug) printf("SetChanAcc(ser/active), ret = %d\n",ret);
         ret = SetChanAcc(ser_ch, 0x40, access);
         printf("SetChanAcc(ser), ret = %d\n",ret);
         ret = GetChanAcc(ser_ch, 0x80, &SerRecord);
	 if (fdebug) {
	    access = SerRecord.data[0];
            printf("GetChanAcc(ser/active), ret = %d, new value = %02x\n",
		   ret,access);
	    }
         ret = GetChanAcc(ser_ch, 0x40, &SerRecord);
	 access = SerRecord.data[0];
         printf("GetChanAcc(ser), ret = %d, new value = %02x\n",
		ret,access);
         ShowChanAccess(ser_ch,"ser",access,SerRecord.data[1]);

	 /* Needed if (fSetTMode), otherwise extra. */
	 {
            /* auth types: pswd, MD5, MD2, none */
            if (authmask == 0) authmask = 0x17;
            SerRecord.data[0] = (0x16 & authmask); /*Callback level*/
            SerRecord.data[1] = (0x16 & authmask); /*User level    */
            SerRecord.data[2] = (0x16 & authmask); /*Operator level*/
            SerRecord.data[3] = (0x16 & authmask); /*Admin level   */
            SerRecord.data[4] = 0x00;         /*OEM level*/
	    ret = SetSerEntry(2, &SerRecord,5);
            printf("SetSerEntry(2), ret = %d\n",ret);
	 }
	 if (fconsoleonly) SerRecord.data[0] = 0x81; /*direct, Basic only */
	 else if (fBMode)  SerRecord.data[0] = 0x83; /*direct; PPP & Basic */
	 else        SerRecord.data[0] = 0x87; /*direct; TMode, PPP, Basic*/
	 if ((vend_id != VENDOR_SUPERMICROX) &&
	     (vend_id != VENDOR_SUPERMICRO)) {
            ret = SetSerEntry(3, &SerRecord,1);
            printf("SetSerEntry(3), ret = %d\n",ret);
         }
	 /* 
	  * BMode inactivity timeout = 2 (* 30 sec = 60 sec)
	  * TMode inactivity timeout = 0 (infinite)
	  */
	 if (fBMode) SerRecord.data[0] = 0x02;  
	 else        SerRecord.data[0] = inactivity;  /*usu 0, infinite*/
         ret = SetSerEntry(4, &SerRecord,1);
         printf("SetSerEntry(4), ret = %d\n",ret);
	 if ((vend_id != VENDOR_SUPERMICROX) &&
	     (vend_id != VENDOR_SUPERMICRO)) {
	   // SerRecord.data[0] = 0x02;  /* enable inactivity, ignore DCD */
	   SerRecord.data[0] = 0x03;  /* enable inactivity, enable DCD */
           ret = SetSerEntry(6, &SerRecord,1);
           printf("SetSerEntry(6), ret = %d\n",ret);
           /* flow control: 60=RTS/CTS+DTR, 20=none+DTR */
	   SerRecord.data[0] = (bFlow << 6) | 0x20;
	   SerRecord.data[1] = newbaud; /* baud default = 19.2K bps */
           ret = SetSerEntry(7, &SerRecord,2);
           printf("SetSerEntry(7) baud %s, ret = %d\n",Baud2Str(newbaud),ret);
	   if (fBMode) {
	     SerRecord.data[0] = 0x1e; /* mux 2way control, GetChanAuth, PPP */
	   } else {  /*TMode*/
	     // SerRecord.data[0] = 0x1e; /* mux 2way control,GetChanAuth,PPP */
	     SerRecord.data[0] = 0x16; /* mux 2way control, GetChanAuth,no PPP*/
				       /* 2way: <ESC>( and <ESC>Q */
	   }
	   if (fconsoleonly)
	     SerRecord.data[1] = 0x09; /* enable serial port sharing, hb=not */
	   else if (fBMode && !fShared)  /* Basic Mode and NOT shared */
	     SerRecord.data[1] = 0x0a; /* enable serial port sharing,heartbeat*/
	   else 
	     SerRecord.data[1] = 0x08; /* enable serial port sharing, hb=off */
           ret = SetSerEntry(8, &SerRecord,2);
           printf("SetSerEntry(8), ret = %d\n",ret);
         }

	 if (fSetUser3) {
           if (usernum == 0) usernum = ser_user;  /* user 3 */
	   /* 
	    * Set up admin user for Serial User (usu User# 3)
	    * Username: root
	    * Password: password
            *
	    * cmdtool 20 18 45 3 72 6F 6F 74 0 0 0 0 0 0 0 0 0 0 0 0
	    * cmdtool 20 18 47 3 2 70 61 73 73 77 6F 72 64 0 0 0 0 0 0 0 0
	    * cmdtool 20 18 47 3 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
	    */
	   SerRecord.data[0] = usernum;  /* user 3 */
	   memset(&SerRecord.data[1],0,16);
	   memcpy(&SerRecord.data[1],rguser,strlen(rguser));
           ret = SetMiscEntry(SET_USER_NAME, &SerRecord, 17);
           printf("SetUserName(%d), ret = %d\n",usernum,ret);
	   /* set password for user 3 */
	   SerRecord.data[0] = usernum; 
	   SerRecord.data[1] = 0x02; 
	   memset(&SerRecord.data[2],0,PSW_LEN);
	   memcpy(&SerRecord.data[2],rgpasswd,strlen(rgpasswd));
           ret = SetMiscEntry(SET_USER_PASSWORD, &SerRecord, 2+PSW_LEN);
             printf("SetUserPassword(%d,2), ret = %d\n",usernum,ret);
	   /* enable user 3 */
	   SerRecord.data[0] = usernum; 
	   SerRecord.data[1] = 0x01; 
	   memset(&SerRecord.data[2],0,PSW_LEN);
           ret = SetMiscEntry(SET_USER_PASSWORD, &SerRecord, 2+PSW_LEN);
           printf("SetUserPassword(%d,1), ret = %d\n",usernum,ret);
	   
	   /* Testing User #3 password */
	   // cmdtool 20 18 47 3 3 70 61 73 73 77 6F 72 64 0 0 0 0 0 0 0 0
	   SerRecord.data[0] = usernum; 
	   SerRecord.data[1] = 0x03; 
	   memset(&SerRecord.data[2],0,PSW_LEN);
	   memcpy(&SerRecord.data[2],rgpasswd,strlen(rgpasswd));
           ret = SetMiscEntry(SET_USER_PASSWORD, &SerRecord, 2+PSW_LEN);
           printf("SetUserPassword(%d,3), ret = %d\n",usernum,ret);
	   
	   /* Set User #3 access up w/ ADMIN privilege for serial channel */
	   // cmdtool 20 18 43 91 3 4 0
	   SerRecord.data[0] = 0x90 | ser_ch;   /*0x91*/
	   SerRecord.data[1] = usernum;  /* user 3 */
	   SerRecord.data[2] = ser_access; /*usu 0x04; Admin*/
	   SerRecord.data[3] = 0x00; 
           ret = SetMiscEntry(SET_USER_ACCESS, &SerRecord, 4);
           printf("SetUserAccess(%d/%02x), ret = %d\n",usernum,ser_access,ret);
	  
	  if (ipmi_reserved_user(vend_id,0x01) == 0) {
	   /* Set NULL user access up w/ USER privilege for serial channel */
	   // cmdtool 20 18 43 91 1 2 0    
	   SerRecord.data[0] = 0x90 | ser_ch;   /*0x91*/
	   SerRecord.data[1] = 0x01;  /* user 1 (default) */
	   SerRecord.data[2] = 0x02;  /* user privilege */
	   SerRecord.data[3] = 0x00; 
           ret = SetMiscEntry(SET_USER_ACCESS, &SerRecord, 4);
           printf("SetUserAccess(1/user), ret = %d\n",ret);
          }
	 } else {   
          /* No username specified, use NULL user 1 for Serial */
	  if (ipmi_reserved_user(vend_id,0x01) == 0) {
	   /* Set NULL user password, if specified */
	   if (fSetPsw) {
	     SerRecord.data[0] = 0x01;  /*user 1*/
	     SerRecord.data[1] = 0x02; 
	     memset(&SerRecord.data[2],0,PSW_LEN);
	     memcpy(&SerRecord.data[2],rgpasswd,strlen(rgpasswd));
             ret = SetMiscEntry(SET_USER_PASSWORD, &SerRecord, 2+PSW_LEN);
             printf("SetUserPassword(1,2), ret = %d\n",ret);
	     /* make sure user 1 is enabled */
	     SerRecord.data[0] = 0x01;  
	     SerRecord.data[1] = 0x01; 
	     memset(&SerRecord.data[2],0,PSW_LEN);
             ret = SetMiscEntry(SET_USER_PASSWORD, &SerRecord, 2+PSW_LEN);
             printf("SetUserPassword(1,1), ret = %d\n",ret);
	   }
	   /* Set NULL user access up w/ ADMIN privilege for serial channel */
	   // cmdtool 20 18 43 91 1 4 0    
	   SerRecord.data[0] = 0x90 | ser_ch;   /*0x91*/
	   SerRecord.data[1] = 0x01;   /* user 1 */
	   SerRecord.data[2] = ser_access; /* admin */
	   SerRecord.data[3] = 0x00; 
           ret = SetMiscEntry(SET_USER_ACCESS, &SerRecord, 4);
           printf("SetUserAccess(1/%02x), ret = %d\n",ser_access,ret);
	  }
	 }

	 if (fSetTMode) {
	   /*
	    * Set the TMode configuration:
	    *    Enable line editing
	    *    Delete seq = bksp-sp-bksp 
	    *    Enable echo 
	    *    Handshake (hb) = off
	    *    Output newline seq = CR-LF
	    *    Input newline seq = CR
	    */
	   if ((vend_id != VENDOR_SUPERMICROX) &&
	       (vend_id != VENDOR_SUPERMICRO)) {
#ifdef TEST
	     SerRecord.data[0] = 0xA6;    /* sets bit for volatile - wrong */
	     SerRecord.data[1] = 0x11;
             ret = SetSerEntry(29, &SerRecord,2);
#endif
	     SerRecord.data[0] = 0x66;
	     SerRecord.data[1] = 0x11;
             ret = SetSerEntry(29, &SerRecord,2);
             printf("SetSerEntry(29), ret = %d\n",ret);
           }

	   /* Force mux switch to the BMC and start talking... */
	   /* Set Serial/Modem Mux */
	   /* cmdtool 20 30 12 1 4 */
	   SerRecord.data[0] = ser_ch;   /* channel 1 = EMP */
	   SerRecord.data[1] = 0x02;   /* was 0x04 to force switch to BMC */
           i = SetMiscEntry(SET_SER_MUX, &SerRecord, 2);
           printf("SetSerialMux(BMC), ret = %d, value = %02x\n",
		i,SerRecord.data[0]);
	 } /*endif fSetTMode*/

	 else {   /* fBMode */ 
	   if (fShared) {
	     uchar bval;
	     /* workaround for BMC bug w BIOS Console Redirect */
	     SerRecord.data[0] = 0x03;   /* Boot Param 3 */
	     if (bootflags < 16) {   /* user specified a boot flag */
	        SerRecord.data[1] = bootflags; /* Flags */
	        bval = SerRecord.data[1];
                ret = SetMiscEntry(SET_BOOT_OPTIONS, &SerRecord, 2);
                printf("SetBootOptions(3), new value = %02x, ret = %d\n",
		    bval,ret);
	     }

	     /* Force mux switch to the Baseboard initially */
	     SerRecord.data[0] = ser_ch;   /* channel 1 = EMP */
	     SerRecord.data[1] = 0x03;   /* force switch to Baseboard */
             i = SetMiscEntry(SET_SER_MUX, &SerRecord, 2);
             printf("SetSerialMux(System), ret = %d, value = %02x\n",
	    	    i,SerRecord.data[0]);
	   } else {  /* not shared, BMode only */
	     /* Force mux switch to the BMC and start talking... */
	     SerRecord.data[0] = ser_ch;   /* channel 1 = EMP */
	     SerRecord.data[1] = 0x04;   /* 0x04 to force switch to BMC */
             i = SetMiscEntry(SET_SER_MUX, &SerRecord, 2);
             printf("SetSerialMux(BMC), ret = %d, value = %02x\n",
	 	    i,SerRecord.data[0]);
	   }
	 }  /*end-else BMode*/
        }  /*end-else Enable (not Disable) */
	else {  /*not enable or disable BMode/TMode */
	  if (!fcanonical)
	     printf("Please specify a configuration mode (b,d,e,m,s,t)\n");
	}
       }  /*endif not readonly */
   }  /* endif ok */
do_exit:
   ipmi_close_();
   // show_outcome(progname,ret); 
   return(ret);
}  /* end main()*/

/************************
DPC configuration stuff on serial.
16:25:08.165  outgoing: set system boot options[08] 20 00 E0 81 6C 08 02 02 07
16:25:08.165  outgoing: set system boot options[08] 20 00 E0 81 70 08 06 01 0B 28 11 02 45 1F 96 3C 84
16:25:08.165  outgoing: set system boot options[08] 20 00 E0 81 74 08 04 FF FF 01
16:25:08.165  outgoing: set system boot options[08] 20 00 E0 81 78 08 03 00 FC
16:25:08.245  INCOMING: SET SYSTEM BOOT OPTIONS[08] 81 04 7B 20 6C 08 -[00]- 6C
16:25:08.245  INCOMING: SET SYSTEM BOOT OPTIONS[08] 81 04 7B 20 70 08 -[00]- 68
16:25:08.245  outgoing: set system boot options[08] 20 00 E0 81 7C 08 05 80 00 4A 00 00 2C
16:25:08.255  outgoing: chassis control[02] 20 00 E0 81 80 02 03 FA
 ************************/
/* end iserial.c */
