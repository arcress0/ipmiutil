/*M*
//  PVCS:
//      $Workfile:   ipmicmd.c  $
//      $Revision:   1.12  $
//      $Modtime:   23 Feb 2005 11:24:14  $
//      $Author:   arcress at users.sourceforge.net  $  
//
//  Define the ipmi_cmd routine and supporting logic to execute IPMI 
//  commands via one of the supported IPMI drivers:
//     /dev/ipmi0  /dev/ipmi/0    = MontaVista OpenIPMI driver
//     /dev/imb                   = Intel IMB ipmidrvr (comes with ISM)
//     /dev/ipmikcs /dev/ipmi/kcs = valinux driver by San Mehat
//     libfreeipmi.so             = GNU FreeIPMI user-space library
//     ldipmidaemon               = LanDesk IPMI daemon (user-space process) 
// 
//  08/05/02 ARC - created 
//  08/15/02 ARC - added decode_cc
//  10/24/02 ARC - made cmd param ushort to be more unique
//  01/29/03 ARC - added MontaVista OpenIPMI driver support
//  07/25/03 ARC - added serial-over-lan commands
//  07/30/03 ARC - added GetThresholds, fix for ipmi_cmd_raw,
//                 changed some error messages
//  09/04/03 ARC - added debug messages for fDriverTyp first time
//  05/05/04 ARC - leave _mv device open, rely on each app calling ipmi_close,
//		   helps performance.
//  08/10/04 ARC - fix typo in ipmi_cmd_raw/mv: cmd->icmd (thanks Kevin Gao)
//  08/26/04 ARC - fix out-of-bounds error in decode_cc
//  10/27/04 ARC - added gnu FreeIPMI library support
//  11/11/04 ARC - added fdebug to ipmi_getdeviceid & ipmi_open_gnu 
//  02/23/05 ARC - added routines for LanDesk, fDriverTyp=5
//  07/15/05 ARC - test for ldipmi first, since it hangs KCS if another 
//                 driver tries to coexist.
//  07/06/06 ARC - better separate driver implementations, cleaner now
 *M*/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2002-2006, Intel Corporation
Copyright (c) 2009 Kontron America, Inc.
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
 *----------------------------------------------------------------------*/
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#elif defined(EFI)
// defined (EFI32) || defined (EFI64) || defined(EFIX64)
#include <bmc.h>
#include <libdbg.h>
#else
/* Linux, Solaris, BSD */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#ifndef DOS
#include <sys/ioctl.h>
#include <termios.h>
#endif
#include <stdarg.h>
#include <errno.h>
#endif

#include "ipmicmd.h"    /* has NCMDS, ipmi_cmd_t */
#include "ipmilan2.h"  /*includes ipmilan.h also*/

ipmi_cmd_t ipmi_cmds[NCMDS] = { /*if add here, also change NCMDS in ipmicmd.h*/
 {/*empty,temp*/ 0, BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 0, 20}, 
 {GET_SEL_INFO,     BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 0, 14},
 {GET_SEL_ALLOCATION_INFO,BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 0, 9},
 {GET_SEL_ENTRY,    BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 6, 18},
 {RESERVE_SEL,      BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 0, 2},
 {CLEAR_SEL,        BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 6, 1},
 {GET_SEL_TIME,     BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 6, 4},
 {GET_LAN_CONFIG,   BMC_SA, PUBLIC_BUS, NETFN_TRANS, BMC_LUN, 4, 19},
 {SET_LAN_CONFIG,   BMC_SA, PUBLIC_BUS, NETFN_TRANS, BMC_LUN, 21, 0},
 {GET_LAN_STATS,    BMC_SA, PUBLIC_BUS, NETFN_TRANS, BMC_LUN, 2, 18},
 {GET_SER_CONFIG,   BMC_SA, PUBLIC_BUS, NETFN_TRANS, BMC_LUN, 4, 19},
 {SET_SER_CONFIG,   BMC_SA, PUBLIC_BUS, NETFN_TRANS, BMC_LUN, 21, 0},
 {SET_SER_MUX,      BMC_SA, PUBLIC_BUS, NETFN_TRANS, BMC_LUN, 2, 0},
 {GET_PEF_CONFIG,   BMC_SA, PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 3, 22},
 {SET_PEF_CONFIG,   BMC_SA, PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 22, 0},
// {SET_PEF_ENABLE,  BMC_SA, PUBLIC_BUS, NETFN_APP, BMC_LUN, 4, 0}, /*old*/
 {GET_DEVSDR_INFO,  BMC_SA, PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 0, 6},
 {GET_DEVICE_SDR,   BMC_SA, PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 6, 18},
 {RESERVE_DEVSDR_REP,BMC_SA,PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 0, 2},
 {GET_SENSOR_READING,BMC_SA,PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 1, 4},
 {GET_SENSOR_READING_FACTORS,BMC_SA,PUBLIC_BUS, NETFN_SEVT, BMC_LUN, 2, 7},
 {GET_SENSOR_TYPE,  BMC_SA, PUBLIC_BUS, NETFN_SEVT,    BMC_LUN, 1, 2},
 {GET_SENSOR_THRESHOLD,BMC_SA,PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 1, 7},
 {SET_SENSOR_THRESHOLD,BMC_SA,PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 8, 0},
 {GET_SENSOR_HYSTERESIS,BMC_SA,PUBLIC_BUS, NETFN_SEVT, BMC_LUN, 2, 2},
 {SET_SENSOR_HYSTERESIS,BMC_SA,PUBLIC_BUS, NETFN_SEVT, BMC_LUN, 4, 0},
 {GET_SDR,          BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 6, 18},/*full=63*/
 {GET_SDR_REPINFO,  BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 0, 14},
 {RESERVE_SDR_REP,  BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 0, 2},
 {GET_FRU_INV_AREA, BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 1, 3},
 {READ_FRU_DATA,    BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN, 4, 18},
 {WRITE_FRU_DATA,   BMC_SA, PUBLIC_BUS, NETFN_STOR,  BMC_LUN,20 /*3+N(17)*/, 1},
 {GET_DEVICE_ID,    BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 0, 15},
 {SET_USER_ACCESS,  BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 4, 0},
 {GET_USER_ACCESS,  BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 2, 4},
 {GET_USER_NAME,    BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 1, 16},
 {SET_USER_NAME,    BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 17, 0},
 {SET_USER_PASSWORD,BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 18, 0},
 {MASTER_WRITE_READ,BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 4 /*or 3*/, 1},
 {GET_SYSTEM_GUID,  BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 0, 16},
 {WATCHDOG_GET,     BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 0, 8},
 {WATCHDOG_SET,     BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 6, 0},
 {WATCHDOG_RESET,   BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 0, 0},
 {CHASSIS_STATUS,   BMC_SA, PUBLIC_BUS, NETFN_CHAS,  BMC_LUN, 0, 2},
 {CHASSIS_CTL,      BMC_SA, PUBLIC_BUS, NETFN_CHAS,  BMC_LUN, 1, 0},
 {CHASSIS_IDENTIFY, BMC_SA, PUBLIC_BUS, NETFN_CHAS,  BMC_LUN, 1, 0},
 {GET_POWERON_HOURS,BMC_SA, PUBLIC_BUS, NETFN_CHAS,  BMC_LUN, 0, 0},
 {SET_BOOT_OPTIONS, BMC_SA, PUBLIC_BUS, NETFN_CHAS,  BMC_LUN, 19, 0},
 {GET_BOOT_OPTIONS, BMC_SA, PUBLIC_BUS, NETFN_CHAS,  BMC_LUN, 3, 18},
 {ACTIVATE_SOL1,    BMC_SA, PUBLIC_BUS, NETFN_SOL,   BMC_LUN, 0, 0},
 {SET_SOL_CONFIG,   BMC_SA, PUBLIC_BUS, NETFN_SOL,   BMC_LUN, 3, 0},
 {GET_SOL_CONFIG,   BMC_SA, PUBLIC_BUS, NETFN_SOL,   BMC_LUN, 4, 2},
 {ACTIVATE_SOL2,    BMC_SA, PUBLIC_BUS, NETFN_TRANS, BMC_LUN, 0, 0},
 {SET_SOL_CONFIG2,  BMC_SA, PUBLIC_BUS, NETFN_TRANS, BMC_LUN, 3, 0},
 {GET_SOL_CONFIG2,  BMC_SA, PUBLIC_BUS, NETFN_TRANS, BMC_LUN, 4, 2},
 {GET_SEVT_ENABLE,  BMC_SA, PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 1, 5},
 {SET_SEVT_ENABLE,  BMC_SA, PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 6, 0},
 {REARM_SENSOR,     BMC_SA, PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 6, 0},
 {READ_EVENT_MSGBUF,BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 0, 16},
 {GET_EVENT_RECEIVER,BMC_SA,PUBLIC_BUS, NETFN_SEVT,  BMC_LUN, 0, 2},
 {GET_CHANNEL_INFO, BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 1, 9},
 {SET_CHANNEL_ACC,  BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 3, 0},
 {GET_CHANNEL_ACC,  BMC_SA, PUBLIC_BUS, NETFN_APP,   BMC_LUN, 2, 1} };

/* Subroutine definitions for each driver */
#ifdef EFI
int ipmi_open_efi(char fdebug);
int ipmi_cmdraw_efi( uchar cmd, uchar netfn, uchar lun, uchar sa, 
		uchar bus, uchar *pdata, int sdata, 
	 	uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
#else

#ifdef WIN32
extern int ipmi_cmdraw_ia( uchar cmd, uchar netfn, uchar lun, uchar sa, 
		uchar bus, uchar *pdata, int sdata, 
	 	uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_open_ia(char fdebug);
extern int ipmi_close_ia(void);
extern int ipmi_cmdraw_ms(uchar cmd, uchar netfn, uchar lun, uchar sa, 
			uchar bus, uchar *pdata, int sdata, 
			uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_cmd_ms(ushort cmd, uchar *pdata, int sdata, uchar *presp, 
			int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_open_ms(char fdebug);
extern int ipmi_close_ms(void);
#elif defined(SOLARIS)
extern int ipmi_cmdraw_bmc(uchar cmd, uchar netfn, uchar lun, uchar sa, 
			uchar bus, uchar *pdata, int sdata, 
			uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_cmd_bmc(ushort cmd, uchar *pdata, int sdata, uchar *presp, 
			int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_open_bmc(char fdebug);
extern int ipmi_close_bmc(void);
extern int ipmi_cmdraw_lipmi(uchar cmd, uchar netfn, uchar lun, uchar sa, 
			uchar bus, uchar *pdata, int sdata, 
			uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_cmd_lipmi(ushort cmd, uchar *pdata, int sdata, uchar *presp, 
			int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_open_lipmi(char fdebug);
extern int ipmi_close_lipmi(void);
#elif defined(LINUX)
extern int ipmi_cmdraw_ia( uchar cmd, uchar netfn, uchar lun, uchar sa, 
		uchar bus, uchar *pdata, int sdata, 
	 	uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_open_ia(char fdebug);
extern int ipmi_close_ia(void);
extern int ipmi_cmdraw_mv(uchar cmd, uchar netfn, uchar lun, uchar sa, 
			uchar bus, uchar *pdata, int sdata, 
			uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_cmd_mv(ushort cmd, uchar *pdata, int sdata, uchar *presp, 
			int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_open_mv(char fdebug);
extern int ipmi_close_mv(void);
extern int ipmi_open_ld(char fdebug);
extern int ipmi_close_ld(void);
extern int ipmi_cmdraw_ld(uchar cmd, uchar netfn, uchar lun, uchar sa, 
			uchar bus, uchar *pdata, int sdata, 
			uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_cmd_ld(ushort cmd, uchar *pdata, int sdata, uchar *presp, 
			int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_open_direct(char fdebug);
extern int ipmi_close_direct(void);
extern int ipmi_cmdraw_direct( uchar cmd, uchar netfn, uchar lun, 
			uchar sa, uchar bus, uchar *pdata, int sdata, 
			uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_cmd_direct(ushort cmd, uchar *pdata, int sdata, uchar *presp, 
			int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_set_max_kcs_loops(int ms);
#elif defined(DOS)
extern int ipmi_open_direct(char fdebug);
extern int ipmi_close_direct(void);
extern int ipmi_cmdraw_direct( uchar cmd, uchar netfn, uchar lun, 
			uchar sa, uchar bus, uchar *pdata, int sdata, 
			uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_cmd_direct(ushort cmd, uchar *pdata, int sdata, uchar *presp, 
			int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_set_max_kcs_loops(int ms);
#else
/* BSD */
extern int ipmi_open_direct(char fdebug);
extern int ipmi_close_direct(void);
extern int ipmi_cmdraw_direct( uchar cmd, uchar netfn, uchar lun, 
			uchar sa, uchar bus, uchar *pdata, int sdata, 
			uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_cmd_direct(ushort cmd, uchar *pdata, int sdata, uchar *presp, 
			int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_set_max_kcs_loops(int ms);
extern int ipmi_cmdraw_mv(uchar cmd, uchar netfn, uchar lun, uchar sa, 
			uchar bus, uchar *pdata, int sdata, 
			uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_cmd_mv(ushort cmd, uchar *pdata, int sdata, uchar *presp, 
			int *sresp, uchar *pcc, char fdebugcmd);
extern int ipmi_open_mv(char fdebug);
extern int ipmi_close_mv(void);
#endif
extern int fd_wait(int fd, int nsec, int usec);
#endif

/* Global Data */
int fDriverTyp = DRV_UNKNOWN;  /* 1=IMB driver, 2=VA driver, 3=MV open driver */
                      /* 4= GNU FreeIPMI, 5= LanDesk,    6= builtin IPMI LAN */
		      /* 7= direct KCS,   8= direct SMB, 9= IPMI LAN v2.0 */
int fipmi_lan = 0;    
int fjustpass = 0;
FILE *fperr = NULL;
FILE *fpdbg = NULL;
FILE *fplog = NULL;
int  fauth_type_set = 0;
char fdebug = 0;    
char log_name[60] = {'\0'};  /*log_name global*/
uchar my_devid[20] = {0,0,0,0,0,0,0,0}; /*saved devid, only needs 16 bytes*/
int  gshutdown  = 0;    /* see ipmilan.c ipmilanplus.c */

LAN_OPT lanp = { "localhost","","", IPMI_SESSION_AUTHTYPE_MD5, 
			IPMI_PRIV_LEVEL_USER, 3, "", 0, RMCP_PRI_RMCP_PORT };
static char *gnode = lanp.node;
//char *guser = lanp.user;
//char *gpswd = lanp.pswd;
//uchar *gaddr = lanp.addr; 
//int   gaddr_len = 0;
//int gcipher_suite = 3;          /*used in ipmilanplus.c*/
//int gauth_type  = IPMI_SESSION_AUTHTYPE_MD5; /*if 0, use any: MD5, MD2, etc.*/
//int gpriv_level = IPMI_PRIV_LEVEL_USER;

typedef struct {
  uchar adrtype;
  uchar sa;
  uchar bus;
  uchar lun;
  uchar capab;
} mc_info;
mc_info  bmc = { ADDR_SMI,  BMC_SA, PUBLIC_BUS, BMC_LUN, 0x8F }; /*BMC via SMI*/
mc_info  mc2 = { ADDR_IPMB, BMC_SA, PUBLIC_BUS, BMC_LUN, 0x4F }; /*IPMB target*/
mc_info  mymc = { ADDR_IPMB, BMC_SA, PUBLIC_BUS, BMC_LUN, 0x4F }; /*IPMB */
static char bcomma = ',';
static mc_info  *mc  = &bmc;
#ifdef WIN32
static char msg_no_drv[] = {   /*no Windows driver*/
		"Cannot open an IPMI driver: imbdrv.sys or ipmidrv.sys\n"};
#elif defined(SOLARIS)
static char msg_no_drv[] = {   /*no Solaris driver*/
		"Cannot open an IPMI driver: /dev/bmc or /dev/lipmi\n"};
#elif defined(LINUX)
static char msg_no_drv[] = {   /*no Linux driver*/
		"Cannot open an IPMI driver: /dev/imb, /dev/ipmi0, "
		"/dev/ipmi/0, \n\t "
/* "/dev/ipmikcs, /dev/ipmi/kcs, "  *no longer support valinux */
#ifdef LINK_LANDESK
		"ldipmi, "
#endif
		"or direct driverless.\n" };

#elif defined(DOS)
static char msg_no_drv[] = {   /*no DOS IPMI driver*/
		"Cannot open an IPMI direct KCS interface.\n"};
#else
static char msg_no_drv[] = {   /*no BSD IPMI driver*/
		"Cannot open an IPMI driver: /dev/ipmi0 or direct.\n"};
#endif

/* From IPMI v1.5/v2.0 spec, Table 5-2 Completion Codes */
#define NUMCC    32
struct {
	uchar code;
	char *mesg;
	}  cc_mesg[NUMCC] = { 
/* Note: completion codes 0x80-0x9f may vary depending on the command.
 * 0x80 = Invalid Session Handle or Empty Buffer or Unsupported Feature 
 */
{0x00, "Command completed successfully"},
{0x80, "Invalid Session Handle or Empty Buffer"},
{0x81, "Lost Arbitration"},
{0x82, "Bus Error"},
{0x83, "NAK on Write - busy"},
{0x84, "Truncated Read"},
{0x85, "Invalid session ID in request"},            /*for ActivateSession*/
{0x86, "Requested privilege level exceeds limit"},  /*for ActivateSession*/
{0xC0, "Node Busy"},
{0xC1, "Invalid Command"},
{0xC2, "Command invalid for given LUN"},
{0xC3, "Timeout while processing command"},
{0xC4, "Out of space"},
{0xC5, "Reservation ID cancelled or invalid"},
{0xC6, "Request data truncated"},
{0xC7, "Request data length invalid"},
{0xC8, "Request data field length limit exceeded"},
{0xC9, "Parameter out of range"},
{0xCA, "Cannot return requested number of data bytes"},
{0xCB, "Requested sensor, data, or record not present"},
{0xCC, "Invalid data field in request"},
{0xCD, "Command illegal for this sensor/record type"},
{0xCE, "Command response could not be provided"},
{0xCF, "Cannot execute duplicated request"},
{0xD0, "SDR Repository in update mode, no response"},
{0xD1, "Device in firmware update mode, no response"},
{0xD2, "BMC initialization in progress, no response"},
{0xD3, "Destination unavailable"},
{0xD4, "Cannot execute command. Insufficient privilege level"},
{0xD5, "Cannot execute command. Request parameters not supported"},
{0xD6, "Cannot execute command. Subfunction unavailable"}, 
{0xFF, "Unspecified error"}
};

char * decode_cc(ushort icmd, int cc)
{
   static char other_msg[25];
   char *pmsg;
   int i;
   for (i = 0; i < NUMCC; i++) {
	if (cc == cc_mesg[i].code) break;
   }
   if (i == NUMCC) { /* if not found, show other_msg */
      sprintf(other_msg,"Other error 0x%02x",cc);
      pmsg = other_msg;
   } else {
      if ((icmd == READ_EVENT_MSGBUF) && (cc == 0x80))
           pmsg = "no data available (queue/buffer empty)";
      else pmsg = cc_mesg[i].mesg;
   }
   return(pmsg);
}

char *decode_rv(int rv)
{
   char *msg;
   static char msgbuf[80];
   if (rv == 0x6F) msg = "License not supported"; /*for Dell*/
   else if (rv > 0) msg = decode_cc((ushort)0,rv);
   else switch(rv) {
       case 0:                 msg = "completed successfully";  break;
       case -1:                msg = "error -1";    break;
       case LAN_ERR_SEND_FAIL: msg = "send to BMC failed";    break;
       case LAN_ERR_RECV_FAIL: msg = "receive from BMC failed"; break;
       case LAN_ERR_CONNECT:   msg = "cannot connect to BMC"; break;
       case LAN_ERR_ABORT:     msg = "abort signal caught"; break;
       case LAN_ERR_TIMEOUT:   msg = "timeout occurred"; break;
       case LAN_ERR_BADLENGTH: msg = "length greater than max"; break;
       case LAN_ERR_INVPARAM:   msg = "invalid lan parameter"; break;
       case LAN_ERR_NOTSUPPORT: msg = "request not supported"; break;
       case LAN_ERR_TOO_SHORT:  msg = "receive too short"; break;
       case LAN_ERR_HOSTNAME: msg = "error resolving hostname"; break;
       case LAN_ERR_PING:     msg = "error during ping"; break;
       case LAN_ERR_V1:       msg = "BMC only supports lan v1"; break;
       case LAN_ERR_V2:       msg = "BMC only supports lan v2"; break;
       case LAN_ERR_OTHER:    msg = "other error"; break; 
       case ERR_NO_DRV:       msg = "cannot open IPMI driver"; break; 
       case ERR_BAD_PARAM:    msg = "invalid parameter"; break;
       case ERR_NOT_ALLOWED:  msg = "access not allowed"; break;
       case ERR_USAGE:        msg = "usage or help requested"; break;
       case LAN_ERR_DROPPED:  msg = "session dropped by BMC"; break; 
       case ERR_FILE_OPEN:    msg = "cannot open file"; break;
       case ERR_NOT_FOUND:    msg = "item not found"; break;
       case ERR_BMC_MSG:      msg = "error getting msg from BMC"; break;
		/* ipmidir.h: ERGETTINGIPMIMESSAGE -504 */
       case ERR_BAD_FORMAT:   msg = "bad format"; break;
       case ERR_BAD_LENGTH:   msg = "length less than min"; break;
       case ERR_SDR_MALFORMED: msg = "an SDR is malformed"; break;
       default:           
           sprintf(msgbuf,"error %d",rv);
           msg = msgbuf;
           break;
   }
   return(msg);
}

int get_cmd_rslen(uchar cmd, uchar netfn)
{			/* used by ipmicmd_gnu */
   int rslen = 0;
   int i;
   ushort cmdkey;
   cmdkey = cmd | (netfn << 8);
   for (i = 0; i < NCMDS; i++) {
      if (ipmi_cmds[i].cmdtyp == cmdkey) {
	rslen = ipmi_cmds[i].rslen;
	break;
	}
   }
   return(rslen);
} /*end get_cmd_rslen()*/

static int ndrivers = NDRIVERS;
static struct {
     int idx;
     char *tag;
   } drv_types[NDRIVERS] = {
  { DRV_IMB,  "imb" },
  { DRV_MV,   "open" },
  { DRV_LD,   "landesk" },
  { DRV_LAN2, "lan2" },
  { DRV_LAN2I,"lan2i" },
  { DRV_LAN,  "lan" },
  { DRV_KCS,  "kcs" },
  { DRV_SMB,  "smb" },
  { DRV_MS,   "ms" },
  { DRV_BMC,  "sun_bmc" },
  { DRV_LIPMI,"sun_lipmi" },
  { DRV_SMC,  "supermicro" },
  { DRV_IBM,  "ibm" },
  { DRV_HP,   "hp" },  /*++++*/
#ifdef EFI
  { DRV_EFI,  "efi" }
#else
  { 0, "" } /*DRV_UNKNOWN*/
#endif
};
  // { DRV_VA,   "va" },
  // { DRV_GNU,  "free" },

void set_iana(int iana)
{
      my_devid[6] = (iana & 0x0000ff);
      my_devid[7] = ((iana & 0x00ff00) >> 8);
      my_devid[8] = ((iana & 0xff0000) >> 16);
}

void set_mfgid(uchar *devid, int len)
{
    if (devid == NULL) return;
    if (len > sizeof(my_devid)) len = sizeof(my_devid);
    memcpy(my_devid,devid,len);
}

void get_mfgid(int *pvend, int *pprod)
{
    if (pvend != NULL) 
      *pvend  = my_devid[6] + (my_devid[7] << 8) + (my_devid[8] << 16); 
    if (pprod != NULL) 
      *pprod  = my_devid[9] + (my_devid[10] << 8);
}

char *show_driver_type(int idx)
{
   int i;
   char *tag;
   for (i = 0; i < ndrivers; i++)
   {
      if (drv_types[i].idx == idx) {
         tag = drv_types[i].tag;
         break;
      }
   }
   if (i >= ndrivers) {  /*not found*/
      tag = "unknown";
   }
   return(tag);
}

int get_driver_type(void)
{
    return(fDriverTyp);
}

int set_driver_type(char *tag)
{
   int rv = 0;
   int i;
   /* else if (str_icmp(tag,"lan2") == 0) * leave vendor id as is. */
   for (i = 0; i < ndrivers; i++)
   {
      if (str_icmp(drv_types[i].tag, tag) == 0) {
         fDriverTyp = drv_types[i].idx;
	 if (fDriverTyp == DRV_LAN2I) {  /*LAN2 Intel*/
		set_iana(VENDOR_INTEL); /*VENDOR_INTEL = 0x000157*/
	 } else if (fDriverTyp == DRV_SMC) {  /*supermicro*/
	        set_iana(VENDOR_SUPERMICRO); /*VENDOR_SUPERMICRO = 0x002A7C*/
		fDriverTyp = DRV_LAN;
	 }
	 if (fDriverTyp == DRV_IBM) {  /*LAN IBM*/
	        set_iana(VENDOR_IBM); 
		fDriverTyp = DRV_LAN;
	 }
	 if (fDriverTyp == DRV_HP) {  /*LAN2 HP ++++*/
	        set_iana(VENDOR_HP); 
		fDriverTyp = DRV_LAN2;
		lanp.auth_type = IPMI_SESSION_AUTHTYPE_NONE; /*HP default*/
	 }
         break;
      }
   }
   if (i >= ndrivers) {  /*not found*/
      fDriverTyp = DRV_UNKNOWN;  /*not set yet, so detect*/
      rv = 1;
      // if (fdebugcmd) 
      {
         printf("Invalid -F argument (%s), valid driver types are:\n",tag);
         for (i = 0; i < ndrivers; i++)
            printf("\t%s\n",drv_types[i].tag);
      }
   }
   return(rv);
}

/* 
 * use_devsdrs
 * detect whether to use SDR repository or Device SDRs from
 * the saved GetDeviceID response.
 * ipmi_getdeviceid saves it into my_devid.
 */
int use_devsdrs(int picmg)
{
    int fdev, vend, prod;
    /* set Device SDRs flag as specified in the GetDeviceID */
    if ((my_devid[1] & 0x80) == 0x80) fdev = 1;
    else fdev = 0;
    if (picmg) return(fdev);
    /* check for vendor/products that can report the flag wrong */
    vend  = my_devid[6] + (my_devid[7] << 8) + (my_devid[8] << 16); 
    prod  = my_devid[9] + (my_devid[10] << 8);
    switch(vend) {
      case VENDOR_INTEL:
	if ((prod != 0x800) && (prod != 0x808) && (prod != 0x841)) 
	    fdev = 0;
	break;
      case VENDOR_NSC:  fdev = 0; break;
      case VENDOR_NEC:  fdev = 0; break;
      case VENDOR_DELL: fdev = 0; break;
      case VENDOR_HP:   fdev = 0; break;
      case VENDOR_SUN:  fdev = 0; break;
      default: break;
    }
    return(fdev);
}

/* get_lan_channel returns the next lan channel starting with chfirst. */
int get_lan_channel(uchar chfirst, uchar *chan)
{
   int ret, j;
   uchar iData[4]; 
   uchar rData[9]; 
   int rlen;
   uchar cc;
   int found = 0;

   for (j = chfirst; j < 12; j++) 
   {
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
	/* rData[1] == channel medium type */
        if (rData[1] == 4) {  /* medium type 4 = 802.3 LAN type*/
            if (fdebug) printf("chan[%d] = lan\n",j);
	    found = 1;
	    *chan = (uchar)j;
	    break;
	}
   }
   if (found == 0) ret = -1;
   return(ret);
}

int nodeislocal(char *nodename)
{
   if (nodename == NULL) return 1;
   if (nodename[0] == 0) return 1;
   if (strcmp(nodename,"localhost") == 0) return 1;
   return 0;
}

int set_max_kcs_loops(int ms)
{
   int rv = 0;
#if defined(LINUX)
   rv = ipmi_set_max_kcs_loops(ms);
#elif defined(BSD)
   rv = ipmi_set_max_kcs_loops(ms);
#elif defined(DOS)
   rv = ipmi_set_max_kcs_loops(ms);
#endif
   return(rv);
}

/* 
 * ipmi_open
 * This is called by ipmi_cmd and ipmicmd_raw if a session has not
 * yet been opened (fDriverTyp == DRV_UNKNOWN).
 * The order to try these drivers could be customized for specific
 * environments by modifying this routine.
 *
 * ipmi_cmd[raw] would call a specific open routine if set_driver_type().
 */
int ipmi_open(char fdebugcmd)
{
   int rc = 0;
   fperr = stderr;
   fpdbg = stdout;

   fdebug = fdebugcmd;
#ifdef EFI
   rc = ipmi_open_efi(fdebugcmd);
#else
   if (!nodeislocal(gnode)) { fipmi_lan = 1; }
   if (fdebugcmd) printf("ipmi_open: driver type = %s\n",
			show_driver_type(fDriverTyp));
   /* first time, so try each */
   if (fipmi_lan) {
      /* Try IPMI LAN 1.5 first */
      rc = ipmi_open_lan(gnode,lanp.port,lanp.user,lanp.pswd,fdebugcmd); 
      fDriverTyp = DRV_LAN;
      if (rc == LAN_ERR_V2) { 
         /* Use IPMI LAN 2.0 if BMC said it only supports LAN2 */
	 /* This is a violation of IPMI 2.0 Spec section 13.4,
	  * but some HP firmware behaves this way, so handle it. */
         fDriverTyp = DRV_LAN2;
         rc = ipmi_open_lan2(gnode,lanp.user,lanp.pswd,fdebugcmd); 
         if (rc != 0) fDriverTyp = DRV_UNKNOWN;
      }
   } else {  /* local, not lan */
#ifdef WIN32
	rc = ipmi_open_ia(fdebugcmd);
	if (rc == ACCESS_OK) 
		fDriverTyp = DRV_IMB; 
	else if ((rc = ipmi_open_ms(fdebugcmd)) == ACCESS_OK) 
		fDriverTyp = DRV_MS; 
	else rc = ERR_NO_DRV;
#elif defined(SOLARIS)
	rc = ipmi_open_bmc(fdebugcmd);
	if (rc == ACCESS_OK) 
		fDriverTyp = DRV_BMC; 
	else if ((rc = ipmi_open_lipmi(fdebugcmd)) == ACCESS_OK) 
		fDriverTyp = DRV_LIPMI; 
	else rc = ERR_NO_DRV;
#elif defined(LINUX)
	if ((rc = ipmi_open_ld(fdebugcmd)) == ACCESS_OK) {
		fDriverTyp = DRV_LD;
		ipmi_close_ld();
	} else if ((rc = ipmi_open_mv(fdebugcmd)) == ACCESS_OK) {
		fDriverTyp = DRV_MV;
		/* ipmi_close_mv(); * leave it open until explicit close */
	} else if ((rc = ipmi_open_ia(fdebugcmd)) == ACCESS_OK) {
		fDriverTyp = DRV_IMB;
	} else if ((rc = ipmi_open_direct(fdebugcmd)) == ACCESS_OK) {
		/* set to either DRV_KCS or DRV_SMB */
	} else rc = ERR_NO_DRV;
#elif defined(DOS)
        rc = ipmi_open_direct(fdebugcmd);
	/* sets fDriverTyp to either DRV_KCS or DRV_SMB */
	if (rc != ACCESS_OK) rc = ERR_NO_DRV;
#else
	/* BSD or MACOS */
	if ((rc = ipmi_open_mv(fdebugcmd)) == ACCESS_OK) {
		/* FreeBSD "kldload ipmi" has /dev/ipmi0 */
		fDriverTyp = DRV_MV;
		/* ipmi_close_mv(); * leave it open until explicit close */
	} else if ((rc = ipmi_open_direct(fdebugcmd)) == ACCESS_OK) {
		/* sets fDriverTyp to either DRV_KCS or DRV_SMB */
	} else rc = ERR_NO_DRV;
#endif

   }  /*endelse local, not lan*/
#endif
   if (fdebugcmd) printf("ipmi_open rc = %d type = %s\n",rc,
			show_driver_type(fDriverTyp));
   return (rc);
}

int ipmi_close_(void)
{
   int rc = 0;
#ifndef EFI
   switch (fDriverTyp)
   {
#ifdef WIN32
	case DRV_IMB: rc = ipmi_close_ia(); break;
	case DRV_MS: rc = ipmi_close_ms(); break;
#elif defined(SOLARIS)
	case DRV_BMC: rc   = ipmi_close_bmc(); break;
	case DRV_LIPMI: rc = ipmi_close_lipmi(); break;
#elif defined(LINUX)
	case DRV_IMB: rc = ipmi_close_ia(); break;
	case DRV_MV:  rc = ipmi_close_mv(); break;
	case DRV_LD:  rc = ipmi_close_ld(); break;
	case DRV_SMB: 
	case DRV_KCS: rc = ipmi_close_direct(); break;
#elif defined(DOS)
	case DRV_SMB: 
	case DRV_KCS: rc = ipmi_close_direct(); break;
#else
	/* BSD or MACOS */
	case DRV_MV:  rc = ipmi_close_mv(); break;
	case DRV_SMB: 
	case DRV_KCS: rc = ipmi_close_direct(); break;
#endif
	case DRV_LAN:  rc = ipmi_close_lan(gnode); break;
	case DRV_LAN2I:
	case DRV_LAN2: rc = ipmi_close_lan2(gnode); break;
	default:  break;
   }  /*end switch*/
#endif
   fDriverTyp = DRV_UNKNOWN;
   return (rc);
}

/* don't worry about conflict with other ipmi libs any longer */
int ipmi_close(void) {  return(ipmi_close_()); }

#if defined(EFI)
int ipmi_open_efi(int fdebugcmd)
{
    int rc = 0;
    static bool BmcLibInitialized = false;
 
    if (BmcLibInitialized == false ) {
          rc = BmcLibInitialize();
          if (rc == 0) {
                BmcLibInitialized = true;
                fDriverTyp = DRV_EFI;
          } 
    }
    return rc;
}

#define TIMEOUT_EFI  (1000*1000)  /*see ipmi_timeout_ia*/
int ipmi_cmdraw_efi(uchar cmd, uchar netfn, uchar sa, uchar bus, uchar lun,
                uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd)
{
    BMC_MESSAGE ReqMsg;
    BMC_MESSAGE RespMsg;
    int status = 0;
    uchar * pc;
    int sz, i;
 
    ReqMsg.DevAdd   = sa;
    ReqMsg.NetFn    = netfn;
    ReqMsg.LUN      = lun;
    ReqMsg.Cmd      = cmd;
    ReqMsg.Len      = sdata;
    for( sz=0; (sz<sdata && sz< IPMI_REQBUF_SIZE); sz++ )
            ReqMsg.Data[sz] = pdata[sz];
    sz = *sresp;  /* note that sresp must be pre-set */
    memset(presp, 0, sz);
    for ( i =0 ; i < BMC_MAX_RETRIES; i++)
    {
        *sresp = sz;   /* retries may need to re-init *sresp */
        if((status =ProcessTimedMessage(&ReqMsg, &RespMsg,TIMEOUT_EFI)) == 0) {
            *sresp = RespMsg.Len;
            for( sz=0 ; sz<RespMsg.Len && sz<IPMI_RSPBUF_SIZE ; sz++ )
                            presp[sz] = RespMsg.Data[sz];
            *pcc = RespMsg.CompCode;
            break;
        }
        if (fdebugcmd)   // only gets here if error
            fprintf(fpdbg,"ipmi_cmd_efi: ProcessTimedMessage error status=%x\n",
                    (uint)status);
    }
    return(status);
}
#endif

/* 
 * ipmi_cmdraw()
 *
 * This routine can be used to invoke IPMI commands that are not
 * already pre-defined in the ipmi_cmds array.
 * It invokes whichever driver-specific routine is needed (ia, mv, etc.).
 * Parameters:
 * uchar cmd     (input): IPMI Command
 * uchar netfn   (input): IPMI NetFunction
 * uchar sa      (input): IPMI Slave Address of the MC
 * uchar bus     (input): BUS  of the MC
 * uchar lun     (input): IPMI LUN
 * uchar *pdata  (input): pointer to ipmi data
 * int sdata   (input): size of ipmi data
 * uchar *presp (output): pointer to response data buffer
 * int *sresp   (input/output): on input, size of response buffer,
 *                              on output, length of response data
 * uchar *cc    (output): completion code
 * char fdebugcmd(input): flag =1 if debug output desired
 */
int ipmi_cmdraw(uchar cmd, uchar netfn, uchar sa, uchar bus, uchar lun,
		uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd)
{
    int rc = 0;
    ushort icmd;

    fperr = stderr;
    fpdbg = stdout;

    if (sdata > 255) return(LAN_ERR_BADLENGTH);
    if (fDriverTyp == DRV_UNKNOWN) {   /*first time, so find which one */
        rc = ipmi_open(fdebugcmd);
	if (fdebugcmd) 
		fprintf(fpdbg,"Driver type %s, open rc = %d\n",
			show_driver_type(fDriverTyp),rc);
        if (rc == ERR_NO_DRV && !fipmi_lan) fprintf(fperr, "%s", msg_no_drv);
        else if (rc != 0) fprintf(fperr,"ipmi_open error = %d %s\n", rc,decode_rv(rc));
	if (rc != 0) return(rc);
    }  /*endif first time*/

    icmd = (cmd & 0x00ff) | (netfn << 8);
    *pcc = 0;
    /* Check for the size of the response buffer being zero. */
    /* This may be valid for some commands, but print a debug warning. */
    if (fdebugcmd && (*sresp == 0)) printf("ipmi_cmdraw: warning, sresp==0\n");

#ifdef EFI
    rc = ipmi_cmdraw_efi(cmd, netfn, lun, sa, bus, pdata,sdata,
                                presp,sresp, pcc, fdebugcmd);
#else
    switch (fDriverTyp)
    {
#ifdef WIN32
	case DRV_IMB: 
	   rc = ipmi_cmdraw_ia(cmd, netfn, lun, sa, bus, pdata,sdata, 
				presp,sresp, pcc, fdebugcmd);
	   break;
	case DRV_MS: 
           rc = ipmi_cmdraw_ms(cmd,  netfn, lun, sa, bus, pdata,sdata, 
			 	presp,sresp, pcc, fdebugcmd);
	   break;
#elif defined(SOLARIS)
	case DRV_BMC: 
           rc = ipmi_cmdraw_bmc(cmd,  netfn, lun, sa, bus, pdata,sdata, 
			 	presp,sresp, pcc, fdebugcmd);
	   break;
	case DRV_LIPMI: 
           rc = ipmi_cmdraw_lipmi(cmd,  netfn, lun, sa, bus, pdata,sdata, 
			 	presp,sresp, pcc, fdebugcmd);
	   break;
#elif defined(LINUX)
	case DRV_IMB: 
	   rc = ipmi_cmdraw_ia(cmd, netfn, lun, sa, bus, pdata,sdata, 
				presp,sresp, pcc, fdebugcmd);
	   break;
	case DRV_MV: 
           rc = ipmi_cmdraw_mv(cmd,  netfn, lun, sa, bus, pdata,sdata, 
			 	presp,sresp, pcc, fdebugcmd);
	   break;
	case DRV_LD: 
	   rc = ipmi_cmdraw_ld( cmd, netfn, lun, sa, bus, 
				pdata,sdata, presp,sresp, pcc, fdebugcmd);
	   break;
	case DRV_SMB: 
	case DRV_KCS: 
           rc = ipmi_cmdraw_direct(cmd,  netfn, lun, sa, bus, pdata,sdata, 
				presp,sresp, pcc, fdebugcmd);
	   break;
#elif defined(DOS)
	case DRV_SMB: 
	case DRV_KCS: 
           rc = ipmi_cmdraw_direct(cmd,  netfn, lun, sa, bus, pdata,sdata, 
				presp,sresp, pcc, fdebugcmd);
	   break;
#else
	/* BSD or MACOS */
	case DRV_MV: 
           rc = ipmi_cmdraw_mv(cmd,  netfn, lun, sa, bus, pdata,sdata, 
			 	presp,sresp, pcc, fdebugcmd);
	   break;
	case DRV_SMB: 
	case DRV_KCS: 
           rc = ipmi_cmdraw_direct(cmd,  netfn, lun, sa, bus, pdata,sdata, 
				presp,sresp, pcc, fdebugcmd);
	   break;
#endif
	case DRV_LAN: 
	   rc = ipmi_cmdraw_lan(gnode, cmd, netfn, lun, sa, bus, 
				pdata,sdata, presp,sresp, pcc, fdebugcmd);
	   break;
	case DRV_LAN2I: 
	case DRV_LAN2: 
	   rc = ipmi_cmdraw_lan2(gnode, cmd, netfn, lun, sa, bus, 
				pdata,sdata, presp,sresp, pcc, fdebugcmd);
	   break;
	default:    /* no ipmi driver */
	   rc = ERR_NO_DRV;
	   break;
    }  /*end switch*/
#endif

    if ((rc >= 0) && (*pcc != 0) && fdebugcmd) {
          fprintf(fpdbg,"ccode %x: %s\n",*pcc,decode_cc(icmd,(int)*pcc));
    }
    /* clear the temp cmd (OLD) */
    // ipmi_cmds[0].cmdtyp = 0;    
    // ipmi_cmds[0].sa = BMC_SA;

    return(rc);
}

/* 
 * ipmi_cmd_mc()
 * This uses the mc pointer to route commands via either the SMI or
 * IPMB method to the designated mc.
 * See also ipmi_set_mc and ipmi_restore_mc.
 */
int ipmi_cmd_mc(ushort icmd, uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd)
{
   uchar cmd, netfn;
   int rv;
 
   cmd = icmd & CMDMASK;
   netfn = (icmd & 0xFF00) >> 8;
   if (sdata > 255) return(LAN_ERR_BADLENGTH);
   if ((fDriverTyp != DRV_MV) && (mc->adrtype == ADDR_IPMB) && !fipmi_lan) {
      rv = ipmi_cmd_ipmb(cmd, netfn, mc->sa, mc->bus, mc->lun,
                       pdata, sdata, presp, sresp, pcc, fdebugcmd);
   } else {    /* use ADDR_SMI */
      rv = ipmi_cmdraw(cmd, netfn, mc->sa, mc->bus, mc->lun,
                      pdata, sdata, presp, sresp, pcc, fdebugcmd);
   }
   return(rv);
}

int ipmi_cmdraw_mc(uchar cmd, uchar netfn, 
		uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd)
{
   int rv;
   if (sdata > 255) return(LAN_ERR_BADLENGTH);
   if ((fDriverTyp != DRV_MV) && (mc->adrtype == ADDR_IPMB) && !fipmi_lan) {
      rv = ipmi_cmd_ipmb(cmd, netfn, mc->sa, mc->bus, mc->lun,
                       pdata, sdata, presp, sresp, pcc, fdebugcmd);
   } else {    /* use ADDR_SMI */
      rv = ipmi_cmdraw(cmd, netfn, mc->sa, mc->bus, mc->lun,
                      pdata, sdata, presp, sresp, pcc, fdebugcmd);
   }
   return(rv);
}

/* 
 * ipmi_cmd()
 *
 * This is the externally exposed subroutine for commands that
 * are defined in the ipmi_cmds array above.
 * It calls the ipmi_cmdraw routine for further processing.
 */
int ipmi_cmd(ushort icmd, uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd)
{
    int rc, i;
    uchar bcmd;
    uchar netfn, sa, bus, lun;

    fperr = stderr;
    fpdbg = stdout;
    if (sdata > 255) return(LAN_ERR_BADLENGTH);
    if (fDriverTyp == DRV_UNKNOWN) {   /*first time, so find which one */
        rc = ipmi_open(fdebugcmd);
	if (fdebugcmd) 
		fprintf(fpdbg,"Driver type %s, open rc = %d\n",
			show_driver_type(fDriverTyp),rc);
        if (rc != 0) {
           if (rc == ERR_NO_DRV && !fipmi_lan) fprintf(fperr, "%s", msg_no_drv);
           else fprintf(fperr,"ipmi_open error = %d %s\n", rc,decode_rv(rc));
           return(rc);
        }
    }  /*endif first time*/

    for (i = 0; i < NCMDS; i++) {
       if (ipmi_cmds[i].cmdtyp == icmd) break;
    }
    if (i >= NCMDS) {
        fprintf(fperr, "ipmi_cmd: Unknown command %x\n",icmd);
        return(-1);
        }
    bcmd  = icmd & CMDMASK;  /* unmask it */
    netfn = ipmi_cmds[i].netfn;
    lun   = ipmi_cmds[i].lun;
    sa    = ipmi_cmds[i].sa;
    bus   = ipmi_cmds[i].bus;

    rc = ipmi_cmdraw(bcmd, netfn, sa, bus, lun, 
			pdata,sdata, presp,sresp, pcc, fdebugcmd);
    return(rc);
}

/* MOVED ipmi_cmd_ipmb() to ipmilan.c */

int ipmi_getpicmg(uchar *presp, int sresp, char fdebug)
{
   uchar idata[2];
   int rc; uchar cc;

   /* check that sresp is big enough */
   if (sresp < 4) return(-3);
   idata[0] = PICMG_ID;
   rc = ipmi_cmdraw(PICMG_GET_PROPERTIES, NETFN_PICMG,
		    BMC_SA, PUBLIC_BUS, BMC_LUN, 
		    idata, 1, presp,&sresp, &cc, fdebug);
   if (rc != ACCESS_OK) return(rc);
   if (cc != 0) return(cc);
   return(ACCESS_OK);   /* success */
}

int ipmi_getdeviceid(uchar *presp, int sresp, char fdebug)
{
   int rc, i; uchar cc;

   /* check that sresp is big enough (default is 15 bytes for Langley)*/
   if (sresp < 15) return(ERR_BAD_LENGTH);
   rc = ipmi_cmd_mc(GET_DEVICE_ID, NULL, 0, presp,&sresp, &cc, fdebug);
   if (rc != ACCESS_OK) return(rc);
   if (cc != 0) return(cc);
   i = sresp;
   if (i > sizeof(my_devid)) i = sizeof(my_devid);
   memcpy(my_devid,presp,i); /* save device id for later use */
   if (fdebug) {
	  uchar maj,min,iver;
      int vend, prod;
	  get_devid_ver(&maj,&min,&iver);
      get_mfgid(&vend, &prod);
      printf("devid: firmware ver %x.%02x, IPMI v%02x, vendor=%d prod=%d\n",
			 maj,min,iver,vend,prod);
   }
   return(ACCESS_OK);   /* success */
}

void get_devid_ver(uchar *bmaj, uchar *bmin, uchar *iver)
{
   if (bmaj != NULL) *bmaj = my_devid[2];
   if (bmin != NULL) *bmin = my_devid[3];
   if (iver != NULL) *iver = my_devid[4];
}

void show_devid(uchar b1, uchar b2, uchar i1, uchar i2)
{
   /* b1 = devid[2]; b2 = devid[3]; i2|i1 = devid[4]; */
   printf("-- BMC version %x.%02x%c IPMI version %d.%d \n",b1,b2,bcomma,i1,i2);
}

void ipmi_set_mc(uchar bus, uchar sa, uchar lun, uchar atype) 
{
   mc = &mc2;
   mc->bus  = bus;
   mc->sa   = sa;
   mc->lun  = lun;
   mc->adrtype = atype; /* ADDR_SMI or ADDR_IPMB */
   if (fdebug) printf("ipmi_set_mc(%02x,%02x,%02x,%02x)\n",bus,sa,lun,atype);
   return;
}

void ipmi_restore_mc(void)
{
   mc = &bmc;
   return;
}

void ipmi_get_mc(uchar *bus, uchar *sa, uchar *lun, uchar *type) 
{
   /* mc = &bmc or &mc2; */
   if (bus != NULL)  *bus = mc->bus;
   if (sa != NULL)   *sa = mc->sa;
   if (lun != NULL)  *lun = mc->lun;
   if (type != NULL) *type = mc->adrtype; /* ADDR_SMI or ADDR_IPMB */
   return;
}

void ipmi_set_mymc(uchar bus, uchar sa, uchar lun, uchar type) 
{
   mymc.bus  = bus;
   mymc.sa   = sa;
   mymc.lun  = lun;
   mymc.adrtype = type; /* ADDR_SMI or ADDR_IPMB */
   return;
}

void ipmi_get_mymc(uchar *bus, uchar *sa, uchar *lun, uchar *type) 
{
   if (bus != NULL)  *bus = mymc.bus;
   if (sa != NULL)   *sa = mymc.sa;
   if (lun != NULL)  *lun = mymc.lun;
   if (type != NULL) *type = mymc.adrtype; /* ADDR_SMI or ADDR_IPMB */
   return;
}

int ipmi_sendrecv(struct ipmi_rq * req, uchar *rsp, int *rsp_len)
{      /* compatible with intf->sendrecv() */
   int rv;
   uchar ccode;
   int rlen;

   rlen = IPMI_RSPBUF_SIZE;
   *rsp_len = 0;
   if ((fDriverTyp != DRV_MV) && (mc->adrtype == ADDR_IPMB) && !fipmi_lan) {
      rv = ipmi_cmd_ipmb( req->msg.cmd, req->msg.netfn, mc->sa,mc->bus,
		       req->msg.lun, req->msg.data, (uchar)req->msg.data_len,
                       rsp, &rlen, &ccode, fdebug);
   } else {    /* use ADDR_SMI */
      rv = ipmi_cmdraw(req->msg.cmd, req->msg.netfn, mc->sa,mc->bus,
		       req->msg.lun, req->msg.data, (uchar)req->msg.data_len,
                       rsp, &rlen, &ccode, fdebug);
   }
   if (rv == 0 && ccode != 0) rv = ccode;
   if (rv == 0) { /*success*/
        *rsp_len = rlen;
   }
   return (rv);
}

#ifdef WIN32
static HANDLE con_in  = INVALID_HANDLE_VALUE; 
static DWORD cmodein;
static DWORD cmodeold;

void tty_setraw(int mode)
{
    // system("@echo off");
    con_in  = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(con_in,  &cmodein);
    cmodeold = cmodein;
    if (mode == 2) {
       cmodein &= ~(ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | 
    		   ENABLE_ECHO_INPUT);
    } else {  /* (mode==1) just suppress ECHO */
       cmodein &= ~ENABLE_ECHO_INPUT;
    }
    SetConsoleMode(con_in,  cmodein);
}
void tty_setnormal(int mode)
{
    // system("echo on");
    if (mode == 1) 
       cmodein  |= ENABLE_ECHO_INPUT;
    else 
       cmodein = cmodeold;
    SetConsoleMode(con_in,  cmodein);
}
int tty_getattr(int *lflag, int *oflag, int *iflag)
{
    *lflag = (int)cmodein;
    *oflag = 0;
    *iflag = 0;
    return(0);
}
#elif defined(DOS)
void tty_setraw(int mode)
{ return; }
void tty_setnormal(int mode)
{ return; }
int tty_getattr(int *lflag, int *oflag, int *iflag)
{ return(-1); }
#else
          /*LINUX, SOLARIS, BSD*/
// #include <curses.h>
static struct termios mytty;
static struct termios ttyold;
static ulong  tty_oldflags;
int tty_getattr(int *lflag, int *oflag, int *iflag)
{
    int rv;
    static struct termios outtty;
    rv = tcgetattr(STDOUT_FILENO, &outtty);
    if (rv == 0) {
         *lflag = outtty.c_lflag;
         *oflag = outtty.c_oflag;
         *iflag = outtty.c_iflag; 
    }
    return(rv);
}

void tty_setraw(int mode)
{
    int i;
    // system("stty -echo");
    i = tcgetattr(STDIN_FILENO, &mytty);
    if (i == 0) {
        tty_oldflags = mytty.c_lflag;
        ttyold = mytty;
#ifdef SOLARIS
        mytty.c_iflag |= IGNPAR;
        mytty.c_iflag &= ~(ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXANY | IXOFF);
        mytty.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL);
        mytty.c_lflag &= ~IEXTEN;
        /* mytty.c_oflag &= ~OPOST;  // this causes NL but no CR on output */
        mytty.c_cc[VMIN] = 1;
        mytty.c_cc[VTIME] = 0;
	i = tcsetattr(STDIN_FILENO, TCSADRAIN, &mytty);
#else
        if (mode == 2) {  /*raw mode*/
           mytty.c_lflag &= ~(ICANON | ISIG | ECHO);
           // mytty.c_oflag &= ~ONLCR;  /* do not map NL to CR-NL on output */
        } else   /* (mode==1) just suppress ECHO */
           mytty.c_lflag &= ~ECHO;
        i = tcsetattr(STDIN_FILENO, TCSANOW, &mytty);
#endif
    }
}
void tty_setnormal(int mode)
{
    // system("stty echo");
    if (mode == 1) 
       mytty.c_lflag |= ECHO;
    else { /*(mode==2)*/
       mytty.c_lflag = tty_oldflags;
       mytty = ttyold;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &mytty);
#ifdef SOLARIS
	tcsetattr(fileno(stdin), TCSADRAIN, &mytty);
#endif
}
#endif

static char *my_getline(char *prompt, char fwipe)
{
   /* getline is the same format as readline, but much simpler, and portable. */
   static char linebuf[128];
   int c, i;

   if (prompt != NULL) printf("%s\n",prompt);
   if (fwipe) tty_setraw(1); 
   for (i = 0; i < (sizeof(linebuf)-1); i++)
   {
       c = getc(stdin);
       if (c == EOF)  break;
       if (c == '\n') break;
       if ((c < 0x20) || (c > 0x7F)) break;  /*out of bounds for ASCII */
       linebuf[i] = c & 0xff;
   }
   linebuf[i] = 0;
   if (fwipe) {
       for (c = 0; c < i; c++) putc('*',stdout);
       putc('\n',stdout);
       tty_setnormal(1);
   }
   if (i == 0) return NULL;
   return(linebuf);
}

static char *getline_wipe(char *prompt)
{
   return(my_getline(prompt,1));
}

void set_debug(void)
{
   fdebug = 1;
}

char is_remote(void)
{
   return((char)fipmi_lan);
}

int get_lan_options(char *node, char *user, char *pswd, int *auth, int *priv,
		int *cipher, void *addr, int *addr_len)
{
   if (fipmi_lan == 0) return(-1);
   if (node != NULL) strcpy(node,lanp.node);
   if (user != NULL) strcpy(user,lanp.user);
   if (pswd != NULL) strcpy(pswd,lanp.pswd);
   if (auth != NULL) *auth = lanp.auth_type;
   if (priv != NULL) *priv = lanp.priv;
   if (cipher != NULL) *cipher = lanp.cipher;
   if (addr != NULL && lanp.addr_len != 0) memcpy(addr,lanp.addr,lanp.addr_len);
   if (addr_len != NULL) *addr_len = lanp.addr_len;
   return(0);
}

int set_lan_options(char *node, char *user, char *pswd, int auth, int priv,
		int cipher, void *addr, int addr_len)
{
   int rv = 0;
   if (node != NULL) {
	strncpy(lanp.node,node,SZGNODE); 
	lanp.node[SZGNODE] = '\0';
	gnode = lanp.node;
	fipmi_lan = 1; 
   }
   if (user != NULL) {
	strncpy(lanp.user,user,SZGNODE); 
	lanp.user[SZGNODE] = '\0';
   }
   if (pswd != NULL) {
	strncpy(lanp.pswd,pswd,PSW_MAX); 
	lanp.pswd[PSW_MAX] = '\0';
   }
   if (auth > 0 && auth <= 5) { lanp.auth_type = auth; }
   else rv = ERR_BAD_PARAM;
   if (priv > 0 && priv <= 5) { lanp.priv = priv;  }
   else rv = ERR_BAD_PARAM;
   if (cipher >= 0 && cipher <= 17) { lanp.cipher = cipher; }
   else rv = ERR_BAD_PARAM;
   if ((addr != NULL) && (addr_len > 15) && (addr_len <= sizeof(lanp.addr))) {
	memcpy(lanp.addr,addr,addr_len);
	lanp.addr_len = addr_len;
   }
   ipmi_flush_lan(gnode);
   return(rv);
}

void parse_lan_options(int c, char *popt, char fdebugcmd)
{
#if defined(EFI) | defined(DOS)
   return;
#else
   int i;
   static int fset_dtype = 0;
   uchar sa;
   char *p = NULL;

   switch(c) 
   {
	  case 'p':
                i = atoi(popt);
                if (i > 0) lanp.port = i;
				else printf("-p port %d < 0, defaults to %d\n",
				i,RMCP_PRI_RMCP_PORT);
                break;
          case 'F':      /* force driver type */
                i = set_driver_type(popt);
				if (i == 0) fset_dtype = 1;
                break;
          case 'T':      /* auth type */
                i = atoi(popt);
                if (i >= 0 && i <= 5) lanp.auth_type = i;
		fauth_type_set = 1;
                break;
          case 'V':      /* priv level */
                i = atoi(popt);
                if (i > 0 && i <= 5) lanp.priv = i;
                break;
          case 'J':
                i = atoi(popt);
                if (i >= 0 && i <= 17) lanp.cipher = i;
		else printf("-J cipher suite %d > 17, defaults to %d\n",
				i,lanp.cipher);
		if (fset_dtype == 0) i = set_driver_type("lan2");
                break;
          case 'N': 
                strncpy(lanp.node,popt,SZGNODE);  /*remote nodename */
		lanp.node[SZGNODE] = '\0';
                fipmi_lan = 1;
                break;
          case 'U':
                strncpy(lanp.user,popt,SZGNODE);  /*remote username */
		lanp.user[SZGNODE] = '\0';
                /* Hide username from 'ps' */
                memset(popt, ' ', strlen(popt));
                break;
          case 'R':
          case 'P':
                strncpy(lanp.pswd,popt,PSW_MAX);  /*remote password */
		lanp.pswd[PSW_MAX] = '\0';
                /* Hide password from 'ps' */
                memset(popt, ' ', strlen(popt));
                break;
          case 'E':      /* get password from IPMI_PASSWORD environment var */
                p = getenv("IPMI_PASSWORD");
                if (p == NULL) perror("getenv(IPMI_PASSWORD)");
                else {
                   strncpy(lanp.pswd,p,PSW_MAX);  /*remote password */
                   if (strlen(p) > PSW_MAX) lanp.pswd[PSW_MAX] = '\0';
                   if (fdebugcmd) printf("using IPMI_PASSWORD\n");
                }
                break;
          case 'Y':      /* prompt for remote password */
		p = getline_wipe("Enter IPMI LAN Password: ");
                if (p != NULL) {
                   strncpy(lanp.pswd,p,PSW_MAX);  /*remote password */
                   if (strlen(p) > PSW_MAX) lanp.pswd[PSW_MAX] = '\0';
		}
                break;
          case 'Z':    /* set local MC address */
		sa  = htoi(&popt[0]);  /*device slave address*/
		ipmi_set_mymc(mc->bus, sa, mc->lun, ADDR_IPMB);
		break;
          default:
                if (fdebugcmd) printf("unrecognized option %c\n",c);
                break;
   }
   ipmi_flush_lan(gnode);
#endif
}  /*end parse_lan_options*/

void print_lan_opt_usage(int opt)
{
#if defined(EFI) | defined(DOS)
    return;
#else
    if (opt == 1) /*port ok*/
	printf("       -p port  UDP Port of target system\n");
    printf("       -N node  Nodename or IP address of target system\n");
    printf("       -U user  Username for remote node\n");
    printf("       -P/-R pswd  Remote Password\n");
    printf("       -E   use password from Environment IPMI_PASSWORD\n");
    printf("       -F   force driver type (e.g. imb, lan2)\n");
    printf("       -J 0 use lanplus cipher suite 0: 0 thru 14, 3=default\n");
    printf("       -T 1 use auth Type: 1=MD2, 2=MD5(default), 4=Pswd\n");
    printf("       -V 2 use priVilege level: 2=user(default), 4=admin\n");
    printf("       -Y   prompt for remote password\n");
    printf("       -Z   set slave address of local MC\n");
#endif
}  /*end parse_lan_options*/

char *get_nodename(void)
{
   return(gnode);
}

void show_outcome(char *prog, int ret)
{
   int err = 0;
   if (prog == NULL) prog = "";
   err = get_LastError();
   if (ret == -1 && err != 0) show_LastError(prog,err);
   printf("%s%c %s\n",prog,bcomma,decode_rv(ret));
}

/* end ipmicmd.c */
