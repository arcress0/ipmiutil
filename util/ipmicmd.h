/*M*
//  PVCS:
//      $Workfile:   ipmicmd.h  $
//      $Revision:   1.0  $
//      $Modtime:   22 Jul 2002 08:51:14  $
//      $Author:   arcress  $  
//
//  10/24/02 arcress - made cmd param ushort to be more unique
//
 *M*/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2002, Intel Corporation
Copyright (c) 2009 Kontron America, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 
  c.. Neither the name of the copyright holder nor the names of its contributors
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

#define uchar   unsigned char
#define uint32  unsigned int
#define uint64  unsigned long
// #ifdef __USE_MISC
/* Can use compatibility names for C types. (from sys/types.h) */
// typedef unsigned long int  ulong;
// typedef unsigned short int ushort;
// typedef unsigned int       uint;
// #else
#ifndef ushort
#define ushort  unsigned short
#define ulong   unsigned long
#define uint    unsigned int
#endif

#ifdef WIN32
#define snprintf _snprintf
#define SockType   SOCKET
#define SockInvalid INVALID_SOCKET
#else
#define SockType   int
#define SockInvalid -1
#endif

// Other IPMI values
#define PUBLIC_BUS      0
#define PRIVATE_BUS  0x03
#define BMC_SA       0x20
#define BMC_LUN         0
#define SMS_LUN         2
#define HSC_SA       0xC0
#define ME_SA        0x2C
#define ME_BUS       0x06

#define ADDR_SMI     1
#define ADDR_IPMB    2

#define ACCESS_OK    0
#define IPMI_REQBUF_SIZE       255
#define IPMI_RSPBUF_SIZE       250  // was 80, then 1024, see MAX_BUFFER_SIZE 
#define IPMI_LANBUF_SIZE       200  // see RS_LEN_MAX for IPMI LAN

// IPMI NetFn types, see Table 5-1
#define NETFN_CHAS   0x00  // chassis
#define NETFN_BRIDGE 0x02  // bridge
#define NETFN_SEVT   0x04  // sensor/event
#define NETFN_APP    0x06  // application
#define NETFN_FW     0x08  // firmware
#define NETFN_STOR   0x0a  // storage
#define NETFN_TRANS  0x0c  // transport
#define NETFN_SOL    0x34  // serial-over-lan (in IPMI 2.0, use TRANS)
#define NETFN_PICMG  0x2c  // for ATCA PICMG systems

#ifndef IMBAPI_H__ 
// special IMB defines, duplicates if imb_api.h
#define MAX_BUFFER_SIZE        255
#define MAX_SDR_SIZE           128
#define GET_DEVICE_ID           (0x01 | (NETFN_APP << 8))
#define WRITE_READ_I2C          (0x52 | (NETFN_APP << 8)) /*=MASTER_WRITE_READ*/
#endif

#define PSW_MAX                 20  /* IPMI Passwords max = 16 or 20 bytes*/
/* IPMI Commands, see Table 38-8, combined CMD and NETFN in unsigned short */
#define CMDMASK   0xff      /* mask to leave only the command part */
#define WATCHDOG_RESET  	(0x22 | (NETFN_APP << 8))
#define WATCHDOG_SET    	(0x24 | (NETFN_APP << 8))
#define WATCHDOG_GET    	(0x25 | (NETFN_APP << 8))
#define GET_SYSTEM_GUID 	(0x37 | (NETFN_APP << 8))
#define SET_CHANNEL_ACC 	(0x40 | (NETFN_APP << 8))
#define GET_CHANNEL_ACC	        (0x41 | (NETFN_APP << 8))
#define GET_CHANNEL_INFO        (0x42 | (NETFN_APP << 8))
#define SET_USER_ACCESS         (0x43 | (NETFN_APP << 8))
#define GET_USER_ACCESS         (0x44 | (NETFN_APP << 8))
#define SET_USER_NAME           (0x45 | (NETFN_APP << 8))
#define GET_USER_NAME           (0x46 | (NETFN_APP << 8))
#define SET_USER_PASSWORD       (0x47 | (NETFN_APP << 8))
#define MASTER_WRITE_READ       (0x52 | (NETFN_APP << 8))
// #define SET_PEF_ENABLE       0xA1  /* NETFN_APP (old) */
#define CHASSIS_STATUS          0x01  /* NETFN_CHAS (=00) */
#define CHASSIS_CTL             0x02  /* NETFN_CHAS (=00) */
#define CHASSIS_IDENTIFY        0x04  /* NETFN_CHAS (=00) */
#define SET_BOOT_OPTIONS        0x08  /* NETFN_CHAS (=00) */
#define GET_BOOT_OPTIONS        0x09  /* NETFN_CHAS (=00) */
#define GET_POWERON_HOURS       0x0F  /* NETFN_CHAS (=00) */
#define GET_PEF_CONFIG          (0x13 | (NETFN_SEVT << 8))
#define SET_PEF_CONFIG          (0x12 | (NETFN_SEVT << 8))
#define GET_DEVSDR_INFO		(0x20 | (NETFN_SEVT << 8))
#define GET_DEVICE_SDR		(0x21 | (NETFN_SEVT << 8))
#define RESERVE_DEVSDR_REP	(0x22 | (NETFN_SEVT << 8))
#define SET_SEVT_ENABLE		(0x28 | (NETFN_SEVT << 8))
#define GET_SEVT_ENABLE		(0x29 | (NETFN_SEVT << 8))
#define REARM_SENSOR		(0x2A | (NETFN_SEVT << 8))
#define GET_FRU_INV_AREA        (0x10 | (NETFN_STOR << 8))
#define READ_FRU_DATA           (0x11 | (NETFN_STOR << 8))
#define WRITE_FRU_DATA          (0x12 | (NETFN_STOR << 8))

#define GET_SENSOR_READING_FACTORS (0x23 | (NETFN_SEVT << 8))
#define SET_SENSOR_HYSTERESIS	(0x24 | (NETFN_SEVT << 8))
#define GET_SENSOR_HYSTERESIS	(0x25 | (NETFN_SEVT << 8))
#define SET_SENSOR_THRESHOLD	(0x26 | (NETFN_SEVT << 8))
#define GET_SENSOR_THRESHOLD	(0x27 | (NETFN_SEVT << 8))
#define GET_SENSOR_EVT_ENABLE	(0x29 | (NETFN_SEVT << 8))
#define REARM_SENSOR_EVENTS	(0x2A | (NETFN_SEVT << 8))
#define GET_SENSOR_EVT_STATUS	(0x2B | (NETFN_SEVT << 8))
#define GET_SENSOR_READING	(0x2D | (NETFN_SEVT << 8))
#define GET_SENSOR_TYPE 	(0x2F | (NETFN_SEVT << 8))

#define SET_LAN_CONFIG          (0x01 | (NETFN_TRANS << 8))
#define GET_LAN_CONFIG          (0x02 | (NETFN_TRANS << 8))
#define GET_LAN_STATS           (0x04 | (NETFN_TRANS << 8))
#define SET_SER_CONFIG          (0x10 | (NETFN_TRANS << 8))
#define GET_SER_CONFIG          (0x11 | (NETFN_TRANS << 8))
#define SET_SER_MUX             (0x12 | (NETFN_TRANS << 8))
#define GET_SEL_INFO            (0x40 | (NETFN_STOR << 8))
#define GET_SEL_ALLOCATION_INFO (0x41 | (NETFN_STOR << 8))
#define RESERVE_SEL             (0x42 | (NETFN_STOR << 8))
#define GET_SEL_ENTRY           (0x43 | (NETFN_STOR << 8))
#define CLEAR_SEL               (0x47 | (NETFN_STOR << 8))
#define GET_SEL_TIME     	(0x48 | (NETFN_STOR << 8))
#define GET_SDR_REPINFO		(0x20 | (NETFN_STOR << 8))
#define RESERVE_SDR_REP         (0x22 | (NETFN_STOR << 8))
#define GET_SDR			(0x23 | (NETFN_STOR << 8))
#define ACTIVATE_SOL1		(0x01 | (NETFN_SOL << 8))
#define SET_SOL_CONFIG		(0x03 | (NETFN_SOL << 8))
#define GET_SOL_CONFIG		(0x04 | (NETFN_SOL << 8))
#define ACTIVATE_SOL2		(0x20 | (NETFN_TRANS << 8))
#define SET_SOL_CONFIG2		(0x21 | (NETFN_TRANS << 8))
#define GET_SOL_CONFIG2		(0x22 | (NETFN_TRANS << 8))
#define READ_EVENT_MSGBUF	(0x35 | (NETFN_APP << 8))
#define GET_EVENT_RECEIVER	(0x01 | (NETFN_SEVT << 8))
#define SMS_OS_REQUEST 		0x10 /*(0x10 | (NETFN_APP << 8)) */
#define CMD_GET_SESSION_INFO     0x3D /* NETFN_APP */
#define CMD_SET_SYSTEM_INFO      0x58 /* NETFN_APP */
#define CMD_GET_SYSTEM_INFO      0x59 /* NETFN_APP */
/*
 Other commands used for IPMI LAN:
    GET_CHAN_AUTH  (0x38 | (NETFN_APP << 8))
    GET_SESS_CHAL  (0x39 | (NETFN_APP << 8))
    ACT_SESSION    (0x3A | (NETFN_APP << 8))
    SET_SESS_PRIV  (0x3B | (NETFN_APP << 8))
    CLOSE_SESSION  (0x3C | (NETFN_APP << 8))
 */

#define IPMB_CLEAR_MSGF           0x30
#define IPMB_GET_MESSAGE          0x33
#define IPMB_SEND_MESSAGE         0x34

#define PICMG_SLAVE_BUS           0x40 
/* commands under NETFN_PICMG */
#define PICMG_GET_PROPERTIES      0x00   
#define PICMG_GET_LED_PROPERTIES  0x05
#define PICMG_SET_LED_STATE       0x07
#define PICMG_GET_LED_STATE       0x08
#define PICMG_ID               0x00
 
/* structure used by ipmi_cmd(), not used by ipmi_cmdraw */
#define NCMDS   62  
typedef struct {
 ushort cmdtyp;
 uchar sa;
 uchar bus;
 uchar netfn;
 uchar lun;
 uchar len;  /*length of request data (FYI, but not used here) */
 uchar rslen;  /*length of response data expected (not including ccode) */
} ipmi_cmd_t;

struct valstr {
        ushort val;
        const char * str;
};

struct oemvalstr {
        uint   oem;
	ushort val;
        const char * str;
};

/* IPMI driver types returned by get_driver_type() */
#define NDRIVERS   15
#define DRV_UNKNOWN 0
#define DRV_IMB   1
#define DRV_VA    2
#define DRV_MV    3
#define DRV_GNU   4
#define DRV_LD    5  /*LANDesk*/
#define DRV_LAN   6  /*IPMI LAN 1.5*/
#define DRV_KCS   7  /*direct KCS*/
#define DRV_SMB   8  /*direct SMBus/SSIF*/
#define DRV_LAN2  9  /*LANplus, IPMI LAN 2.0*/
#define DRV_MS    10 /*Microsoft ipmidrv.sys*/
#define DRV_BMC   11 /*Solaris 10 bmc */
#define DRV_SMC   12 /*SuperMicro Computer LAN mode*/
#define DRV_LIPMI 13 /*Solaris 8/9 lipmi */
#define DRV_LAN2I 14 /*LANplus with Intel OEM */
#define DRV_EFI   15 /*Intel EFI, ipmi.efi*/
#define DRV_IBM   16 /*LAN with IBM OEM mode*/
#define DRV_HP    17 /*LANplus with HP OEM mode*/

/* Event severity codes, used in ievents.c and oem*.c */
#define SEV_INFO  0
#define SEV_MIN   1
#define SEV_MAJ   2
#define SEV_CRIT  3

/* Errors returned by ipmiutil functions, lan, etc, see decode_rv() */
#define ERR_SDR_MALFORMED  -25 /*SDR is malformed */
#define ERR_BAD_LENGTH     -24 /*length < MIN */
#define ERR_BAD_FORMAT     -23 /*bad format*/
#define ERR_USAGE          -22 /*usage/help requested*/
#define ERR_NOT_FOUND      -21 /*requested item not found*/
#define ERR_FILE_OPEN      -20 /*cannot open file*/
#define LAN_ERR_DROPPED    -19 /*Remote BMC dropped the connection*/
#define ERR_NOT_ALLOWED    -18 /*access not allowed*/
#define ERR_BAD_PARAM      -17 /*invalid parameter*/
#define ERR_NO_DRV         -16 /*cannot open IPMI driver*/
#define LAN_ERR_V2         -15 /*BMC only supports IPMI 2.0*/
#define LAN_ERR_V1         -14 /*BMC only supports IPMI 1.x*/
#define LAN_ERR_OTHER      -13
#define LAN_ERR_PING       -12 /*error with ping*/
#define LAN_ERR_HOSTNAME   -11 /*error resolving hostname*/
#define LAN_ERR_TOO_SHORT  -10 /*recv data too short */
#define LAN_ERR_NOTSUPPORT -9  /*slave address != 0x20, not supported now */
#define LAN_ERR_INVPARAM   -8  /*null pointers, etc. */
#define LAN_ERR_BADLENGTH  -7  /*length > MAX */
#define LAN_ERR_TIMEOUT    -6  /*timeout signal(SIGALRM) recvd */
#define LAN_ERR_ABORT      -5  /*abort signal(SIGINT) recvd */
#define LAN_ERR_CONNECT    -4  /*problem connecting to BMC*/
#define LAN_ERR_RECV_FAIL  -3  /*receive failed, usually no response*/
#define LAN_ERR_SEND_FAIL  -2  /*send failed */
#define ERR_BMC_MSG      -504  /*error getting message from BMC*/
                   /* see ipmidir.h: ERGETTINGIPMIMESSAGE -504 */

/* values used to request AUTHTYPE */
#define IPMI_SESSION_AUTHTYPE_NONE      0x00
#define IPMI_SESSION_AUTHTYPE_MD2       0x01
#define IPMI_SESSION_AUTHTYPE_MD5       0x02
#define IPMI_SESSION_AUTHTYPE_PASSWORD  0x04
#define IPMI_SESSION_AUTHTYPE_OEM       0x05
#define AUTHTYPE_INIT      0xFF     /*initial value, not set*/
/* mask values used for AUTHTYPE support */
#define IPMI_MASK_AUTHTYPE_NONE      0x01
#define IPMI_MASK_AUTHTYPE_MD2       0x02
#define IPMI_MASK_AUTHTYPE_MD5       0x04
#define IPMI_MASK_AUTHTYPE_PASSWORD  0x10
#define IPMI_MASK_AUTHTYPE_OEM       0x20

#define IPMI_PRIV_LEVEL_OEM      0x05
#define IPMI_PRIV_LEVEL_ADMIN    0x04
#define IPMI_PRIV_LEVEL_OPERATOR 0x03
#define IPMI_PRIV_LEVEL_USER     0x02
#define IPMI_PRIV_LEVEL_CALLBACK 0x01

#define VENDOR_INTEL    0x000157   /*=343.*/
#define VENDOR_KONTRON  0x003A98   /*=15000*/
#define VENDOR_NSC      0x000322
#define VENDOR_LMC      0x000878
#define VENDOR_TYAN     0x0019FD
#define VENDOR_NEC      0x000077
#define VENDOR_SUPERMICRO 0x002A7C /*=10876.*/
#define VENDOR_PEPPERCON 0x0028C5  /*used in SuperMicro AOC-SIMSO*/
#define VENDOR_FUJITSU   0x002880  /*Fujitsu-Siemens*/
#define VENDOR_MICROSOFT 0x000137  /* 311. */
#define VENDOR_SUN       0x00002A
#define VENDOR_DELL      0x0002A2 
#define VENDOR_HP        0x00000B
#define VENDOR_IBM       0x000002
#define VENDOR_SUPERMICROX 0x00B980 /*=47488. used for Winbond/SuperMicro */
#define VENDOR_MAGNUM    5593      /* Magnum Technologies, also SuperMicro */
#define VENDOR_QUANTA    7244
#define VENDOR_XYRATEX   1993
#define VENDOR_NEWISYS   9237
#define VENDOR_CISCO     5771      /*=0x168B*/
#define VENDOR_LENOVO    0x004A66
#define VENDOR_LENOVO2   0x004F4D
#define VENDOR_ASUS      0x000A3F

#define PRODUCT_QUANTA_S99Q   21401
#define PRODUCT_QUANTA_QSSC_S4R  64  /*0x0040*/

#define URNLOOPS   1000  /* default is 300 ms, Urbanna needs 1000 ms */
#define LOG_MSG_LENGTH   1024   /*max len of log message*/
#define SZGNODE  80  /* max len of a nodename */

#define BDELIM  '|'   /*delimeter for canonical output*/
#define BCOMMA  ','   /*delimeter for CSV output*/
#define BCOLON  ':'   /*delimeter some output with colons*/
#define BCOMMENT '#'  /*delimeter '#' used for comments */

#define RT_OEMIU  0xDB   /*record type for OEM ipmiutil events*/

#ifndef LOG_WARN
#define LOG_EMERG       0     //  system is unusable
#define LOG_ALERT       1     //  action must be taken immediately
#define LOG_CRIT        2     //  critical conditions
#define LOG_ERR         3     //  error conditions
#define LOG_WARN        4     //  warning conditions
#define LOG_NOTICE      5     //  normal but significant condition
#define LOG_INFO        6     //  informational
#define LOG_DEBUG       7     //  debug-level messages
#endif

typedef struct { 
	char node[SZGNODE+1]; 
	char user[SZGNODE+1]; 
	char pswd[PSW_MAX+1]; 
	int auth_type;  /* if 0, use any: MD5, MD2, etc.*/
	int priv;  /* IPMI_PRIV_LEVEL_USER or IPMI_PRIV_LEVEL_ADMIN */
	int cipher; 
	unsigned char addr[128]; /* sizeof(struct sockaddr_storage) = 128 */
	int addr_len; /* struct sockaddr_in/_in6 gaddr; _in6=28, _in=16 bytes*/
	int port;
} LAN_OPT; /* used for IPMI LAN, specified with option -NUP, etc. */

#ifndef _IPMI_RQ_
#define _IPMI_RQ_  1
/* structure used in ipmi_sendrecv, maps to ipmitool syntax. */
struct ipmi_rq {
        struct {
                uchar netfn:6; 
                uchar lun:2; 
                uchar cmd;
                uchar target_cmd;
                ushort data_len;
                uchar *data;
        } msg;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif 
/* ------------------------ SUBROUTINES ------------------------- */

/*
 * ipmi_cmd
 * ushort cmd    (input): (netfn << 8) + command
 * uchar *pdata  (input): pointer to ipmi data
 * int   sdata   (input): size of ipmi data
 * uchar *presp (output): pointer to response data buffer
 * int *sresp   (input/output): on input, size of response buffer,
 *                              on output, length of response data
 * uchar *cc    (output): completion code
 * char fdebugcmd(input): flag =1 if debug output desired
 * returns 0 if successful, <0 if error
 */
int ipmi_cmd(ushort cmd, uchar *pdata,  int sdata, uchar *presp,
		int *sresp, uchar *pcc, char fdebugcmd);   
/*
 * ipmi_cmdraw
 * uchar cmd     (input): IPMI Command
 * uchar netfn   (input): IPMI NetFunction
 * uchar sa      (input): IPMI Slave Address of the MC
 * uchar bus     (input): BUS  of the MC
 * uchar lun     (input): IPMI LUN
 * uchar *pdata  (input): pointer to ipmi data
 * int   sdata   (input): size of ipmi data
 * uchar *presp (output): pointer to response data buffer
 * int *sresp   (input/output): on input, size of response buffer,
 *                              on output, length of response data
 * uchar *cc    (output): completion code
 * char fdebugcmd(input): flag =1 if debug output desired
 * returns 0 if successful, <0 if error
 */
int ipmi_cmdraw(uchar cmd, uchar netfn, uchar sa, uchar bus, uchar lun,
		uchar *pdata, int sdata, uchar *presp,
		int *sresp, uchar *pcc, char fdebugcmd);
/*
 * ipmi_close_
 * Called to close an IPMI session.
 * returns 0 if successful, <0 if error
 */
int ipmi_close_(void);
int ipmi_close(void);  /*ditto*/
/*-----------------------------------------------------------------*
 * These externals are conditionally compiled in ipmicmd.c 
   ipmi_cmdraw_ia()    Intel IMB driver, /dev/imb 
   ipmi_cmdraw_mv()    MontaVista OpenIPMI driver
   ipmi_cmdraw_va()    VALinux driver
   ipmi_cmdraw_ld()    LANDesk driver
   ipmi_cmdraw_direct() Direct/Driverless KCS or SSIF
   ipmi_cmdraw_lan()   IPMI LAN
   ipmi_cmdraw_lan2()  IPMI LANplus (RMCP+ in IPMI 2.0)
 *-----------------------------------------------------------------*/

/*
 * parse_lan_options
 * Parse the IPMI LAN options from the command-line getopt.
 * int  c        (input): command-line option from getopt, one of:
	  case 'p':  UDP port
	  case 'F':  force driver type 
	  case 'T':  auth type 
	  case 'V':  priv level 
	  case 'J':  cipher suite
	  case 'N':  nodename 
	  case 'U':  username
	  case 'R':  remote password 
	  case 'P':  remote password 
	  case 'E':  get password from IPMI_PASSWORD environment var 
	  case 'Y':  prompt for remote password 
	  case 'Z':  set local MC address 
 * char *optarg  (input): command-line argument from getopt
 * char fdebug   (input): show debug messages if =1, default=0
 */
void parse_lan_options(int c, char *optarg, char fdebug);
/*
 * set_lan_options
 * Use this routine to set the lan options 'gnode','guser','gpswd', etc.
 * This would only be required before opening a new session.
 * char *node    (input): IP address or nodename of remote node's IPMI LAN
 * char *user    (input): IPMI LAN username
 * char *pswd    (input): IPMI LAN password
 * int  auth     (input): IPMI LAN authentication type (1 - 5)
 *			  IPMI_SESSION_AUTHTYPE_NONE      0x00
 * 			  IPMI_SESSION_AUTHTYPE_MD2       0x01
 * 			  IPMI_SESSION_AUTHTYPE_MD5       0x02
 * 			  IPMI_SESSION_AUTHTYPE_PASSWORD  0x04
 * 			  IPMI_SESSION_AUTHTYPE_OEM       0x05
 * int  priv     (input): IPMI LAN privilege level (1 - 5)
 * 			  IPMI_PRIV_LEVEL_CALLBACK 0x01
 * 			  IPMI_PRIV_LEVEL_USER     0x02
 * 			  IPMI_PRIV_LEVEL_OPERATOR 0x03
 * 			  IPMI_PRIV_LEVEL_ADMIN    0x04
 * 		          IPMI_PRIV_LEVEL_OEM      0x05
 * int  cipher   (input): IPMI LAN cipher suite (0 thru 17, default is 3)
 * 			  See table 22-19 in the IPMIv2 spec.
 * void *addr    (input): Socket Address to use (SOCKADDR_T *) if not NULL
 *                        This is only used in itsol.c because it has an
 *                        existing socket open.  Default is NULL for this.
 * int  addr_len (input): length of Address buffer (128 if ipv6, 16 if ipv4)
 * returns 0 if successful, <0 if error
 */
int set_lan_options(char *node, char *user, char *pswd, int auth, int priv,
                int cipher, void *addr, int addr_len);
int get_lan_options(char *node, char *user, char *pswd, int *auth, int *priv,
                int *cipher, void *addr, int *addr_len);
void  print_lan_opt_usage(int opt);
int   ipmi_getdeviceid(uchar *presp, int sresp, char fdebugcmd);
/* int ipmi_open(void);  * embedded in ipmi_cmd() */
int   ipmi_getpicmg(uchar *presp, int sresp, char fdebug);
char *show_driver_type(int idx);
int   set_driver_type(char *tag);  
int   set_driver_options(int fdir);  
int   get_driver_type(void);
int   nodeislocal(char *nodename);
/* These *_mc routines are used to manage changing the mc. 
 * The local mc (mymc) may be changed via -Z, and 
 * the remote mc (mc) may be changed with -m. */
void ipmi_set_mc(uchar bus, uchar sa, uchar lun, uchar type);
void ipmi_get_mc(uchar *bus, uchar *sa, uchar *lun, uchar *type);
void ipmi_restore_mc(void);
void ipmi_set_mymc(uchar bus, uchar sa, uchar lun, uchar type);
void ipmi_get_mymc(uchar *bus, uchar *sa, uchar *lun, uchar *type);
/* ipmi_cmdraw_mc and ipmi_cmd_mc are used in cases where the mc may
 * have been changed via ipmi_set_mc.  */
int ipmi_cmdraw_mc(uchar cmd, uchar netfn, 
		uchar *pdata, int sdata, uchar *presp,
		int *sresp, uchar *pcc, char fdebugcmd);
int ipmi_cmd_mc(ushort icmd, uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd);
/* ipmi_sendrecv is a wrapper for ipmi_cmdraw which maps to ipmitool syntax */
int ipmi_sendrecv(struct ipmi_rq * req, uchar *rsp, int *rsp_len);

/* other common subroutines */
char * decode_rv(int rv);  /*ipmicmd.c*/
char * decode_cc(ushort icmd, int cc);
void dump_buf(char *tag,uchar *pbuf,int sz, char fshowascii);
int  get_lan_channel(uchar chstart, uchar *chan);
void show_fru_picmg(uchar *pdata, int dlen); /* ifru_picmg.c*/
/* show_outcome outputs the meaning of the return code. */
void show_outcome(char *prog, int ret);
/* these log routines are primarily for the isol debug log */
FILE *open_log(char *mname);
void close_log(void);
void flush_log(void);
void print_log( char *pattn, ... );
void dump_log(FILE *fp,char *tag,uchar *pbuf,int sz, char fshowascii);
void logmsg( char *pname, char *pattn, ... );

#ifdef WIN32
/* Implement the Linux strncasecmp for Windows. */
int strncasecmp(const char *s1, const char *s2, int n);
#endif
const char *val2str(ushort val, const struct valstr *vs); /*ipmilanplus.c*/
const char * oemval2str(ushort oem, uchar val, const struct oemvalstr *vs);
void  set_debug(void);  /*used only by oem_sun.c*/
void  set_iana(int iana);  /*ipmicmd.c*/
void  set_mfgid(uchar *devid, int len);
void  get_mfgid(int *pvend, int *pprod);
void  get_devid_ver(uchar *bmaj, uchar *bmin, uchar *iver);

char *get_nodename(void);
char  is_remote(void);
void  show_devid(uchar b1, uchar b2, uchar i1, uchar i2);
int   set_max_kcs_loops(int ms);  /* ipmicmd.c, calls ipmidir.c if ok */

/* These common subroutines are in subs.c */
int    str_icmp(char *s1, char *s2); /*used internally in ipmicmd.c*/
char * strdup_(const char *instr);  /*wrapper for strdup, supports WIN32*/
int   strlen_(const char *s);
uchar  htoi(char *inhex);
void  os_usleep(int s, int u);  
char *get_iana_str(int mfg);   /*subs.c*/
int   get_errno(void);   /*subs.c*/
const char * buf2str(uchar * buf, int len); /*subs.c*/
int   str2uchar(char *str_in, uchar *uchr_out);
uchar atob(char *str_in);    /* calls str2uchar*/
void  atoip(uchar *array,char *instr);
int   get_system_info(uchar parm, char *pbuf, int *szbuf); /*subs.c*/
int   set_system_info(uchar parm, char *pbuf, int szbuf); /*subs.c*/
int   ipmi_reserved_user(int vend, int userid);  /*subs.c*/
	
/* from mem_if.c */
int get_BiosVersion(char *str);

/* see isensor.h for SDR cache routines */
/* see ievents.h for sensor_type_desc, sel_opts, decode_sel routines */

#ifdef __cplusplus
}
#endif 
/* end ipmicmd.h */
