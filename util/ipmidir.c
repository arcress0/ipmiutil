/************************************************
 *
 * ipmidir.c
 *
 * Supports direct raw KCS and SMBus user-space I/Os.
 * Use this if no other IPMI driver is present.
 * This interface would not be sufficient if more 
 * than one application is using IPMI at a time.
 * This code is currently included for Linux, not for 
 * Windows.  Windows requires imbdrv.sys or ipmidrv.sys.
 * 
 * 08/21/06 Andy Cress - added as a new module
 * 09/22/06 Andy Cress - improved SMBus base address detection
 * 09/27/06 Andy Cress - fixed passing slave addrs other than BMC (0x20)
 * 01/16/08 Andy Cress - more DBG messages
 *
 ************************************************/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2006, Intel Corporation
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

#if defined(__IA64__)
#define STUB_IO   1
#endif
#if defined(STUB_IO)
/* May stub out direct io. For instance, PPC does not support <sys/io.h> */
#define UCHAR  unsigned char
#define UINT16 unsigned short
int ipmi_open_direct(char fdebugcmd)
{ return(-1); }
int ipmi_close_direct(void)
{ return(-1); }
int ipmi_cmdraw_direct(UCHAR cmd, UCHAR netfn, UCHAR lun, UCHAR sa, UCHAR bus,
			UCHAR *pdata, int sdata, UCHAR *presp,
                        int *sresp, UCHAR *pcc, char fdebugcmd)
{ return(-1); }
int ipmi_cmd_direct(UINT16 icmd, UCHAR *pdata, int sdata, UCHAR *presp,
                        int *sresp, UCHAR *pcc, char fdebugcmd)
{ return(-1); }
int ipmi_set_max_kcs_loops(int ms)
{ return(0); }

#elif defined(LINUX) || defined(BSD) || defined(DOS) || defined(MACOS) || defined(HPUX)
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>	// for ProcessSendMessage timeout
#include <string.h>
#include <errno.h>
#ifdef DOS
#include <dos.h>
#else
#include <sys/mman.h>
#endif

#include "imb_api.h"
#include "ipmicmd.h"
#include "ipmidir.h"
/* No special includes for 64-bit are needed. */ 

#if defined(LINUX)
#include <sys/io.h>
#elif defined(BSD) || defined(MACOS) || defined(HPUX)
// #include <machine/cpufunc.h>
int iofd = -1;   

static __inline uchar
inbc(ushort port)
{
        uchar  data;
/* from Linux sys/io.h:
 *	__asm__ __volatile__ ("inb %w1,%0":"=a" (data):"Nd" (port));
 * from BSD machine/cpufunc.h:
 *	__asm __volatile("inb %1,%0" : "=a" (data) : "id" ((ushort)(port)));
 */
	__asm __volatile("inb %1,%0" : "=a" (data) : "d" (port));
        return (data);
}
static __inline void
outbc(ushort port, uchar data)
{
/* from Linux sys/io.h:
 *	__asm__ __volatile__ ("outb %b0,%w1": :"a" (data), "Nd" (port));
 * from BSD machine/cpufunc.h:
 *	__asm __volatile("outb %0,%1" : : "a" (data), "id" ((ushort)(port)));
 */
	__asm __volatile("outb %0,%1" : : "a" (data), "d" (port));
}
static __inline uint
inl(ushort port)
{
        uint   data;

        __asm __volatile("inl %%dx,%0" : "=a" (data) : "d" (port));
        return (data);
}
static __inline void
outl(ushort port, uint data)
{
        __asm __volatile("outl %0,%%dx" : : "a" (data), "d" (port));
}
#endif
/* Linux & BSD define usleep() in unistd.h, but DOS has delay() instead. */
#if defined(DOS)
void usleep(ulong usec)  /*missing from DOS*/
{
   delay(usec);
}
#endif


/* DBGP() is enabled with -x, DBGP2 is enabled if LINUX_DEBUG & -x */
#define DBGP(fmt, args...)   if (fdebugdir) fprintf(stdout,fmt, ##args)
#if defined(DOS)
#define DBGP2()       ;  /*no extra debug output*/
#elif defined(LINUX_DEBUG)
#define DBGP2(fmt, args...)   if (fdebugdir) fprintf(stderr,fmt, ##args)
#else
#define DBGP2(fmt, args...)   ; /*no extra debug output*/
#endif

#ifdef ALONE
static int fjustpass = 0;
#else
// DRV_KCS, DRV_SMB types defined in ipmicmd.h
// extern int fDriverTyp;  // use set_driver_type() instead
extern ipmi_cmd_t ipmi_cmds[NCMDS];  
extern int fjustpass;
#endif

/*****************************************************************************/
/*
 * GLOBAL DATA 
 */
/* KCS base addr:  use 0xca2 for ia32, 0x8a2 for ia64 
 *            some use 0xca8/0xcac w register spacing 
 */
#if defined(__ia64__) 
/* gcc defines __ia64__, or Makefile.am defines __IA64__ */
static UINT16  kcsBaseAddress  = 0x8a2; /* for Itanium2 KCS */
static UINT8   kcs_inc         = 1;     /*register spacing*/
#else
static UINT16  kcsBaseAddress  = 0xca2; /* for ia32 KCS */
static UINT8   kcs_inc         = 1;     /*register spacing*/
#endif
/*
 * SMBus/SSIF: see dmidecode for I2C Slave Address & Base Addr.
 * Slave Addr is constant for all Intel SSIF platforms (=0x84). 
 * 0x0540 base <= PCI ID 24D38086 at 00:1f.3 is JR (ICH5)
 * 0x0400 base <= PCI ID 25A48086 at 00:1f.3 is TP (Hance_Rapids) 
 */
UINT8  mBMCADDR      = 0x84;   /* SMBus I2C slave address = (0x42 << 1) */ 
UINT16 mBMC_baseAddr = 0x540;  /* usu. 0x0540 (JR), or 0x0400 if TP */
UINT16 BMC_base      = 0;      /* resulting BMC base address (KCS or SMBus)*/

static char lock_dir_file[] = "/var/tmp/ipmiutil_dir_lock";

/* SSIF/SMBus defines */
#define STATUS_REQ 0x60
#define WR_START   0x61
#define WR_END     0x62
#define READ_BYTE  0x68
#define COMMAND_REG  (kcsBaseAddress+kcs_inc)
#define STATUS_REG   (kcsBaseAddress+kcs_inc)
#define DATA_IN_REG  (kcsBaseAddress)
#define DATA_OUT_REG (kcsBaseAddress)

#if defined(DOS) 
extern unsigned inp(unsigned _port);
extern unsigned outp(unsigned _port, unsigned _value);
#pragma intrinsic(inp, outp)
#define _INB(addr)  inp((addr))
#define _OUTB(data, addr)  outp((addr),(data))
#define _IOPL(data)  0
#define ReadPortUchar( addr, valp ) (*valp) = inp((addr))
#define WritePortUchar( addr, val ) outp((addr),(val))
#define WritePortUlong( addr, val ) ( outp((addr),(val)) )  
#define ReadPortUlong( addr, valp ) (*(valp) = inp((addr)) )
#elif defined(BSD) || defined(MACOS) || defined(HPUX)
#define _INB(addr)  inbc((addr))
#define _OUTB(data, addr)  outbc((addr),(data))
#define _IOPL(data)  0
#define ReadPortUchar( addr, valp ) (*valp) = inbc((addr))
#define WritePortUchar( addr, val ) outbc((addr),(val))
#define WritePortUlong( addr, val ) ( outl((addr),(val)) )  
#define ReadPortUlong( addr, valp ) (*(valp) = inl((addr)) )
#else
#define _INB(addr)  inb((addr))
#define _OUTB(data, addr)  outb((data),(addr))
#define _IOPL(data)  iopl((data))
#define ReadPortUchar( addr, valp ) (*valp) = inb((addr))
#define WritePortUchar( addr, val ) outb((val),(addr))
#define WritePortUlong( addr, val ) ( outl((val), (addr)) )  
#define ReadPortUlong( addr, valp ) (*(valp) = inl((addr)) )
#endif
/* Static data for Driver type, BMC type, etc. */
static int      g_DriverType    = DRV_KCS;
static UINT16	g_ipmiVersion	= IPMI_VERSION_UNKNOWN;
static UINT16	g_bmcType	= BMC_UNKNOWN;
struct SMBCharStruct SMBChar;
static int  fdebugdir = 0;
static char fDetectedIF = 0;

/*****************************************************************************/
/* Subroutine prototypes */

#ifdef ALONE
int set_driver_type(char * typ) { return; }
int get_IpmiStruct(UCHAR *iftype, UCHAR *ver, UCHAR *sa, int *base, UCHAR *inc) { return(-1); }
#else
/* extern int set_driver_type(char * typ); in ipmicmd.h */
extern char fsm_debug;   /*in mem_if.c*/
extern int get_IpmiStruct(UCHAR *iftype, UCHAR *ver, UCHAR *sa, int *base, UCHAR *inc);
#endif
int SendTimedImbpRequest_kcs( IMBPREQUESTDATA *requestData, 
				unsigned int timeout, UINT8 *resp_data, 
				int *respDataLen, UINT8 *compCode);
int SendTimedImbpRequest_ssif( IMBPREQUESTDATA *requestData, 
				unsigned int timeout, UINT8 *resp_data, 
				int *respDataLen, UINT8 *compCode);
int ImbInit_dir(void);
static int SendmBmcRequest(
           unsigned char*  request,   UINT32     reqLength,
           unsigned char*  response,  UINT32*    respLength,
           UINT32          ipmiOldFlag);
static int ReadResponse( unsigned char* resp, struct SMBCharStruct* smbChar);
static int SendRequest( unsigned char* req, int  length, 
			struct SMBCharStruct* smbChar );
static int GetDeviceId(UINT16 *bmcType, UINT16 *ipmiVersion);
static int send_raw_kcs(UINT8 *rqData, int rqLen, UINT8 *rsData, int *rsLen );
static int ProcessTimedMessage(BMC_MESSAGE *p_reqMsg, BMC_MESSAGE *p_respMsg, const UINT32 timeout);
static int wait_for_SMS_flag(void);

/*****************************************************************************/
/* Subroutines */

static int ProcessMessage(BMC_MESSAGE *p_reqMsg, BMC_MESSAGE *p_respMsg)
{
   int	   status	= STATUS_OK;
   UINT32  timeout	= 1000 * GETMSGTIMEOUT;	// value is in milliseconds
   status = ProcessTimedMessage(p_reqMsg, p_respMsg, timeout);
   return status;
}

static char *BmcDesc(unsigned char drvtype)
{  /* return BMC Driver type description string */
   char *msg; 
   switch(drvtype) {
      case DRV_KCS:  msg = "KCS";   break; /*BMC Sahalee KCS*/
      case DRV_SMB:  msg = "SMBus"; break; /*mBMC SMBus (SSIF)*/
      default: msg = "";
   }
   return(msg);
}

int get_ipmi_if(void)
{
   /* Only care about Linux since ipmidir is not used for Windows. */
   char *if_file = "/var/lib/ipmiutil/ipmi_if.txt";
   char *if_file2 = "/usr/share/ipmiutil/ipmi_if.txt";
   FILE *fp;
   char line[80];
   char *p;
   char *r; 
   int rv = ERR_NO_DRV;
   int i, j;
   ulong mybase = 0;
   int   myinc = 1;
   // uchar *rgbase;

   fp = fopen(if_file,"r");
   if (fp == NULL) fp = fopen(if_file2,"r");
   if (fp == NULL) {  /*error opening ipmi_if file*/
	return -1;
   } else {  /*read the data from the ipmi_if file*/
      while ( (p = fgets(line,sizeof(line),fp)) != NULL) 
      {
         if (strstr(line,"Interface type:") != NULL) {
             if (strstr(line,"KCS") != NULL) {
                 g_DriverType    = DRV_KCS;
             } else { 
                 g_DriverType    = DRV_SMB;
             }
         } else if (strstr(line,"Base Address:") != NULL) {
             r = strchr(line,':'); 
             i = strspn(++r," \t"); /* skip leading spaces */
             r += i;
             j = strcspn(r," \t\r\n");  /* end at next whitespace */
             r[j] = 0;  /*stringify*/
             /* convert the "0x0000000000000401" string to an int */
             if (strncmp(r,"0x",2) == 0) { r += 2; j -= 2; }
             DBGP2("base addr 0x: %s, mybase=%x j=%d\n",r,mybase,j);
             mybase = strtol(r, NULL, 16);  /*hex string is base 16*/
             DBGP2("base addr 0x: %s, mybase=%x \n",r,mybase);
         } else if (strstr(line,"Register Spacing:") != NULL) {
             r = strchr(line,':'); 
             i = strspn(++r," \t"); /* skip leading spaces */
             r += i;
             j = strcspn(r," \t");  /* end at next whitespace */
             r[j] = 0;  /*stringify*/
             myinc = atoi(r);
         }
      }
      fclose(fp);
      DBGP("ipmi_if: Driver = %d (%s), Base = 0x%04lx, Spacing = %d\n",
		g_DriverType,BmcDesc(g_DriverType),mybase,myinc);
      if (g_DriverType == DRV_SMB) {
	   if (mybase & 0x0001) mybase -= 1;  /* 0x0401 -> 0x0400 */
           if (mybase != 0 && (mybase & 0x0F) == 0) { /* valid base address */
               mBMC_baseAddr = mybase;
               BMC_base = mBMC_baseAddr;
	       rv = 0;
	   }
      } else {  /*KCS*/
            if (mybase != 0) { /* valid base address */
               kcsBaseAddress = (mybase & 0x0000ffff);
               BMC_base = kcsBaseAddress;
	       if (myinc > 1) kcs_inc = myinc;
	       rv = 0;
	    }
      }
      DBGP2("ipmi_if: BMC_base = 0x%04x, kcs_inc = %d, ret = %d\n", 
		BMC_base, kcs_inc,rv);
   } 
   return(rv);
}

static int set_lock_dir(void)
{
   int rv = 0;
   FILE *fp;
   fp = fopen(lock_dir_file,"w");
   if (fp == NULL) rv = -1;
   else {
      fclose(fp);
      rv = 0;
   }
   return(rv);
}

static int clear_lock_dir(void)
{
   int rv = 0;
   rv = remove(lock_dir_file);  /*same as unlink() */
   if (fdebugdir) printf("clear_lock rv = %d\n",rv);
   return(rv);
}

static int check_lock_dir(void)
{
   int rv = 0;
#ifdef LOCK_OK
   FILE *fp;
   fp = fopen(lock_dir_file,"r"); 
   if (fp == NULL) rv = 0;
   else {
      fclose(fp);
      rv = -1;  /*error, lock file exists*/
   }
#endif
   return(rv);
}

int ipmi_open_direct(char fdebugcmd)
{
	int status = 0;
        //char *dmsg = "";
	int i;

	DBGP2("open_direct(%d) called\n",fdebugcmd);
        if (fdebugcmd) {
	    fdebugdir = fdebugcmd;
#if defined(LINUX_DEBUG) && !defined(ALONE)
	    fsm_debug = fdebugcmd;
#endif
	}

	/* Read ipmi_if config file data, if present. */
	status = get_ipmi_if();  
	if (status == -1) {
	   uchar iftype, iver, sa, inc;
	   int mybase;
	   /* Read SMBIOS to get IPMI struct */
	   status = get_IpmiStruct(&iftype,&iver,&sa,&mybase,&inc);
	   if (status == 0) {
	      if (iftype == 0x04) { 
		 g_DriverType = DRV_SMB;
		 mBMC_baseAddr  = mybase;
	      } else  { /*0x01==KCS*/ 
	         g_DriverType = DRV_KCS;
	         BMC_base = mybase;
	         if (sa == 0x20 && mybase != 0) { /*valid*/
		    kcsBaseAddress = mybase;
		    kcs_inc = inc;
		 }
	      }
              DBGP("smbios: Driver=%d(%s), sa=%02x, Base=0x%04x, Spacing=%d\n",
		   g_DriverType,BmcDesc(g_DriverType),sa,mybase,inc);
	   } else {
	      return ERR_NO_DRV;  /*no SMBIOS IPMI record*/
	   }
	}
        
#ifndef DOS
	/* superuser/root priv is required for direct I/Os */
	i = geteuid();  /*direct is Linux only anyway*/
	if (i > 1) {
	    fprintf(stdout,"Not superuser (%d)\n", i);
	    return ERR_NO_DRV;
	}
#endif
	/* check lock for driverless interface */
	i = check_lock_dir();
	if (i != 0) {
	    fprintf(stdout,"open_direct interface locked, %s in use\n",
			lock_dir_file);
	    return ERR_NO_DRV;
	}

	/* Find the SMBIOS IPMI driver type, data */
	status = ImbInit_dir();
	DBGP2("open_direct Init status = %d\n",status);
	DBGP2("open_direct base=%x spacing=%d\n",BMC_base,kcs_inc);
	if (status == 0) {
	  fDetectedIF = 1; /*Successfully detected interface */
	  /* Send a command to the IPMI interface */
	  if (!fjustpass) 
             status = GetDeviceId(&g_bmcType,&g_ipmiVersion);
	  if (status == 0) {
              char *typ;
              if (g_DriverType == DRV_SMB) typ = "smb";
              else  typ = "kcs";
              set_driver_type(typ);
	  }
	  /* set lock for driverless interface */
	  i = set_lock_dir();
	}
	DBGP("open_direct: status=%d, %s drv, ipmi=%d\n",
		status,BmcDesc(g_DriverType),g_ipmiVersion);
	return status; 
}

int ipmi_close_direct(void)
{
	int status = 0;
#if defined(BSD) || defined(MACOS) || defined(HPUX)
    if (iofd >= 0) {
		close(iofd);
		iofd = -1;
	}
#endif
	/* clear lock for driverless interface */
        status = clear_lock_dir();
        return status;
}

int ipmi_cmdraw_direct(UCHAR cmd, UCHAR netfn, UCHAR lun, UCHAR sa, UCHAR bus,
		UCHAR *pdata, int sdata, UCHAR *presp,
                        int *sresp, UCHAR *pcc, char fdebugcmd)
{
    BMC_MESSAGE	sendMsg;
    BMC_MESSAGE	respMsg;
    int status;
    int len = 0;

    if (g_bmcType == BMC_UNKNOWN) {
        /* User-specified a driver type, but need open  */
        status = ipmi_open_direct(fdebugcmd);
    }
    fdebugdir = fdebugcmd;
    if (sdata > IPMI_REQBUF_SIZE) return(LAN_ERR_BADLENGTH); 
    if (fjustpass) {
      status = send_raw_kcs(pdata, sdata, presp, sresp);
      *pcc = 0;
      return(status);
    }
    sendMsg.Bus    = bus;
    sendMsg.DevAdd = sa;
    sendMsg.NetFn  = netfn;
    sendMsg.LUN	   = lun;
    sendMsg.Cmd	   = cmd;
    sendMsg.Len	   = sdata;
    if (sdata > 0) memcpy(sendMsg.Data,pdata,sdata);

    status = ProcessMessage(&sendMsg, &respMsg);
    if (status == STATUS_OK) {
	*pcc = respMsg.CompCode;
        if (respMsg.Len > 0) {
            len = respMsg.Len;
            if (len > *sresp) len = *sresp;
        } else len = 0;
        if (len > 0) 
            memcpy(presp,respMsg.Data,len);
        *sresp = len;
    }
    return(status);
}

#ifndef ALONE
int ipmi_cmd_direct(UINT16 icmd, UCHAR *pdata, int sdata, UCHAR *presp,
                        int *sresp, UCHAR *pcc, char fdebugcmd)
{
    UINT8 cmd, netfn, sa, lun, bus;
    int status;
    int i;

    fdebugdir = fdebugcmd;
    for (i = 0; i < NCMDS; i++) {
      if (ipmi_cmds[i].cmdtyp == icmd) {
         cmd = (icmd & 0x00ff);
         sa = ipmi_cmds[i].sa;
         bus = ipmi_cmds[i].bus;
         netfn = ipmi_cmds[i].netfn;
         lun = ipmi_cmds[i].lun;
         break;
      }
    }
    if (i >= NCMDS) {
        DBGP("ipmidir: icmd %04x not found, defaults used\n",icmd);
        cmd = (icmd & 0xFF);
        netfn = (icmd & 0xFF00) >> 8;
        sa = BMC_ADDR;
        lun = BMC_LUN;
        bus = 0;
    }
    status = ipmi_cmdraw_direct(cmd, netfn, lun, sa, bus, pdata, sdata, 
				presp, sresp, pcc, fdebugcmd);
    return(status);
}
#endif

static UINT8 
CalculateChecksum(UINT8* p_buff, int length)
{
	UINT8	sum = 0;
        int i;
	for (i = 0; i < length; i++) sum+= p_buff[i];
	return (~sum) + 1;
}

static int
GetDeviceId(UINT16 *bmcType, UINT16 *ipmiVersion)
{
	int	 status = STATUS_OK;
	UINT16 thisBmcType = BMC_UNKNOWN;
	BMC_MESSAGE	sendMsg;
	BMC_MESSAGE	respMsg;

	DBGP2("open_direct: detecting interface, bmc: %X ver: %X drv=%s\n", 
              g_bmcType, g_ipmiVersion, BmcDesc(g_DriverType));

	// if the BMC type has not yet been detected - detect it
	if( g_bmcType == BMC_UNKNOWN )
	{
		// Try sending the GetDeviceId command to the default interface
		sendMsg.DevAdd	= BMC_ADDR;
		sendMsg.NetFn	= NETFN_APP;
		sendMsg.LUN	= BMC_LUN;
	        sendMsg.Cmd	= CMD_GET_DEVICE_ID;
		sendMsg.Len	= 0;
		DBGP2("open_direct: Try Get_Device_ID with %s driver\n",
				BmcDesc(g_DriverType));
                status = ProcessMessage(&sendMsg, &respMsg);
                if (status == STATUS_OK) {
                   if (g_DriverType == DRV_KCS) thisBmcType = BMC_SAHALEE;
                   else thisBmcType = BMC_MBMC_87431M;
                } else {  /*error, switch interfaces*/
		   DBGP("open_direct: ProcessMessage(%s) error = %d\n",
				BmcDesc(g_DriverType),status);
		   if (!fDetectedIF) {
		      /* if not yet detected, try other IF type */
                      if (g_DriverType == DRV_KCS) {  
   		         DBGP2("open_direct: Not KCS, try SSIF/SMBus\n");
                         g_DriverType = DRV_SMB;
                      } else {
   		         DBGP2("open_direct: Not SSIF, try KCS\n");
                         g_DriverType = DRV_KCS;
                      }
                   }
		}  /*end-else error*/

		// If error, try again with the other interface 
		if (thisBmcType == BMC_UNKNOWN)
		{
		   // send the command via KCS/SMBus to get the IPMI version.
                   status = ProcessMessage(&sendMsg, &respMsg);
                   if (status == STATUS_OK) {
                      if (g_DriverType == DRV_KCS) thisBmcType = BMC_SAHALEE;
                      else thisBmcType = BMC_MBMC_87431M; 
                   } else {
			// If sending message fails
			status = ER_NO_BMC_IF;
		   }
		}
		g_bmcType = thisBmcType;

		// if we contacted the BMC, decode its version
		if( status == STATUS_OK )
		{
		   if( g_bmcType == BMC_MBMC_87431M )
			{	
				// Decode the miniBMC productID
			}
		   if( respMsg.Data[4] == 0x51 ) 
				g_ipmiVersion = (UINT16)IPMI_VERSION_1_5;
		   else if( respMsg.Data[4] == 0x02 )
				g_ipmiVersion = IPMI_VERSION_2_0;
		}
	}  /* endif unknown BMC */
	
	// set the version info and leave
	*ipmiVersion	= g_ipmiVersion;
	*bmcType	= g_bmcType;

	DBGP2("open_direct: AFTER bmc: %X ver: %X\n", 
	 	g_bmcType, g_ipmiVersion);
	return status;
}

//*****************************************************************************
#if defined (WIN64) 
	// Do Nothing for WIN64 as we are not using these functions here
#else
static int ProcessSendMessage(BMC_MESSAGE *p_reqMsg, BMC_MESSAGE *p_respMsg, const UINT8 bus, const UINT8 slave, const UINT32 timeout)
{
	int	status	= STATUS_OK;
	BMC_MESSAGE	sendReq;
	static UINT8	sendSeq	= 1;
	static UINT8  incTestCount=0;
        BMC_MESSAGE     reqMsg,respMsg;
	int	retryCount;
	UINT32	i;
        UINT nRetryCount = 0;
	int testCount;

        // Windows will be using Async Imb request interface to poll for 
        // messages in the BMC SMS message queue 

	// format the send message packet
	sendReq.Cmd	= SENDMESSAGE_CMD;
	sendReq.DevAdd	= BMC_ADDR;
	sendReq.LUN	= 0;
	sendReq.NetFn	= NETFN_APP;

	sendReq.Data[0]	= bus;
	sendReq.Data[1]	= slave;
	// NetFn is the upper 6 bits of the data byte, the LUN is the lower two
	sendReq.Data[2]	= ((p_reqMsg->NetFn << 2) | (p_reqMsg->LUN & 0x03));
	sendReq.Data[3]	= CalculateChecksum(&sendReq.Data[1], 2);
	sendReq.Data[4]	= BMC_ADDR;
	// NetFn is the upper 6 bits of the data byte, the LUN is the lower two
	sendReq.Data[5]	= ((sendSeq << 2) | (SMS_MSG_LUN & 0x03));
             // sequence number = 1 << 2 and BMC message response lun = 0x02
	sendReq.Data[6]	= p_reqMsg->Cmd;

	// loop to copy the command data to the send message data format
        i = 0;
	for(i=0; i < p_reqMsg->Len; i++)
		sendReq.Data[7+i] = p_reqMsg->Data[i];

	// create a checksum
	sendReq.Data[7+i] = CalculateChecksum(&sendReq.Data[4], p_reqMsg->Len + 3);	// send data length plus the values from index 3 - 6

	// send length plus 0 - 7 and checksum byte
	sendReq.Len	= p_reqMsg->Len + 8;
        /* 
         * Send the message with retries 
         * For get Device ID & Read FRU data, retry less, in case
         * HSC/LCP is not present,
         */
        if (p_reqMsg->NetFn != 0x08 ) 
	        retryCount = BMC_MAX_RETRIES+2;
        else
	        retryCount = BMC_MAX_RETRIES+20;
	do
	{	// send the message
	    if( (status = ProcessMessage(&sendReq, p_respMsg)) == STATUS_OK )
	    {
		// some error, maybe because the controller was not ready yet.
		if( p_respMsg->CompCode == 0x83 ) {
				// Sleep for 1 second and try again 
				sleep(1);
				incTestCount = 1; // true
				continue; 	  // try again
			}
		else if( p_respMsg->CompCode == 0x82 ) {
				DBGP("ProcessSendMessage(sa=%02x,%02x,%02x) "
				     "ccode=82 bus error\n", 
				     slave,p_reqMsg->NetFn,p_reqMsg->Cmd);
				status = ERGETTINGIPMIMESSAGE;
				break;  // exit with error
                        }
		else if( p_respMsg->CompCode != 0x00 )
				continue;  //break;	// exit with error

		status = wait_for_SMS_flag();  /* new, added */
		if (status == -1) 
		      DBGP("wait_for_SMS_flag timeout\n");

		// Only for windows we use Imb Asyn interface for polling 
		// For Unix we issue GetMessage command 
            	// issue a getmessage command
			reqMsg.DevAdd	= BMC_ADDR;
			reqMsg.NetFn	= NETFN_APP;
			reqMsg.LUN	= BMC_LUN;
			reqMsg.Cmd	= GETMESSAGE_CMD;
			reqMsg.Len	= 0;

                nRetryCount = 0;
                testCount = 100; /* For HSC  */
		if (slave == 0x22  || incTestCount == 1)   // true
                     testCount=1000;  /*added for LCP */
 		do
		{
                   /* Loop here for the response with the correct 
                    * sequence number.  */
		   if( (status = ProcessMessage( &reqMsg, &respMsg )) != 0 ) {
	              DBGP("Breaking after Getmsg, Status is %d\n",status);
                      break;
                   }
                   if( slave == 0x22 && p_reqMsg->Cmd == 0x04 ) {
                      /* give debug if LCP ever gets here. */
                      DBGP("LCP get: cnt[%d,%d] seq[%02x,%02x] cc=%02x status=%u\n",
                                testCount,retryCount, sendSeq,
				((respMsg.Data[4] & 0xFC)>>2), 
				respMsg.CompCode,status) ;
                      return STATUS_OK;
                   }
                   if (respMsg.CompCode != 0) {
			DBGP2("get, cnt[test=%d] slave=%02x, cc==%02x\n",
				testCount,slave,respMsg.CompCode);
                   /* Used to wait 1 ms via usleep(1000), but not needed. */
			// usleep(1000);
		   }
		} while(((respMsg.CompCode == 0x83 ||(respMsg.CompCode == 0x80))
                      && --testCount > (int) 0) || (respMsg.CompCode == 0x0 
                      && ((respMsg.Data[4] & 0xFC) >> 2) != sendSeq) );

	        DBGP("get cnt[test=%d,retry=%d] seq[Send=0x%02x Recv=0x%02x] "
                     "CompCode=0x%x status=%u\n", testCount,retryCount,
			sendSeq,((respMsg.Data[4] & 0xFC)>>2),
			respMsg.CompCode,status);        

                if( (respMsg.CompCode == 0x00) && (status == STATUS_OK) && 
		    ((respMsg.Data[4] & 0xFC) >> 2 == sendSeq) )
                {
		    // format the GetMessage response 
		    (*p_respMsg)		= (*p_reqMsg);
		    // The first data byte is the channel number the message 
                    // was sent on
		    p_respMsg->DevAdd	= respMsg.Data[3];
		    p_respMsg->NetFn	= respMsg.Data[1] >> 2;
		    p_respMsg->LUN	= respMsg.Data[4] & 0x03;
		    p_respMsg->Cmd	= respMsg.Data[5];
		    p_respMsg->CompCode = respMsg.Data[6];  // comp code 
		    p_respMsg->Len	= respMsg.Len - 8;	
		    // the last data byte is the checksum
		    if ( p_respMsg->CompCode == 0xCC) {
            	       DBGP2(" Cmd=%d Len=%d ccode=CC ",
				p_respMsg->Cmd, p_respMsg->Len);
                    }
		    for(i=0; i < p_respMsg->Len; i++)
			p_respMsg->Data[i]	= respMsg.Data[7+i];
                    break;
                }
   	   }
	   // this will retry the read the number of times in retryCount
	} while( --retryCount > 0 );

	if( retryCount <= 0 ) status = ERGETTINGIPMIMESSAGE;

	/* Increment the sequence number
	 * sequence number can't be greater 63 (6bit number), 
         * so wrap the counter if it is */
	++sendSeq;
	if( sendSeq >= 64 ) sendSeq = 1;
	return status;
}

#endif	/* All other than WIN64 */

/* #define MAX_KCS_LOOP  30000   *was 7000*/
/* Set a failsafe MAX KCS Loops to prevent infinite loop on status reg */
/* max was 7000, but need longer for ClearSEL cmd */
static int  max_kcs_loop = 30000;  /*means 300ms*/
static int  max_sms_loop = 500;    /*means 500ms*/
static int  peak_loops = 0;
int ipmi_set_max_kcs_loops(int ms)
{
   max_kcs_loop = ms * 100;  /*300 ms * 100 = 30000 loops*/
   max_sms_loop = ms;
   return 0;
}

static int wait_for_SMS_flag(void)
{
	int i = 0;
	while((_INB(STATUS_REG) & 0x04) == 0)
	{
		usleep(2 * 1000); /*sleep for 2 msec*/
		i += 2;
		if ( i > max_sms_loop ) return -1;
	}
	return 0;
}

static int  wait_for_IBF_clear(void)
    {
     int i = 0;
     while ((_INB(STATUS_REG) & 0x02) == 0x02) {
        if (i > 0 && (i % 100) == 0) usleep(1000); /*sleep for 1 msec*/
        if (i > max_kcs_loop) {
            DBGP("wait_for_IBF_clear: max loop %d\n",i);
	    return -1;
            // break;
        }
        i++;
     }
     if (i > peak_loops) peak_loops = i;
     return 0;
    }

static int  wait_for_OBF_set(void)
    {
     int i = 0;
     while ((_INB(STATUS_REG) & 0x01) == 0x00) {
        if (i > 0 && (i % 100) == 0) usleep(1000); /*sleep for 1 msec*/
        if (i > max_kcs_loop) {
            DBGP("wait_for_OBF_set: max loop %d\n",i);
	    return -1;
            // break;
        }
        i++;
     }
     if (i > peak_loops) peak_loops = i;
     return 0;
    }

static inline int get_write_state(void)
    {
      if ((_INB(STATUS_REG) >> 6) != 0x02)
        return -1;
      return 0;
    }

static inline int get_read_state(void)
    {
      if ((_INB(STATUS_REG) >> 6) != 0x01)
        return -1;
      return 0;
    }

static inline int get_idle_state(void)
    {
      if ((_INB(STATUS_REG) >> 6) != 0x00)
        return -1;
      return 0;
    }

UINT8 dummy2;
static inline void clear_OBF(void)
{
     dummy2 = _INB(DATA_IN_REG);
}

static int 
send_raw_kcs (UINT8 *rqData, int rqLen, UINT8 *rsData, int *rsLen )
{
   int length;  
   unsigned char dummy;
   unsigned char rx_data[64];
   int rxbuf_len;
   int rv;

    if (fdebugdir) {
        int cnt;
        DBGP("send_raw_kcs: ");
        for ( cnt=0; cnt < rqLen ; cnt++) 
              DBGP(" %02x",rqData[cnt]);
        DBGP("\n");
    }
   wait_for_IBF_clear();
   clear_OBF();
   _OUTB(WR_START,COMMAND_REG);
   rv = wait_for_IBF_clear();
   if (get_write_state() != 0) return LAN_ERR_SEND_FAIL;
   clear_OBF();
   if (rv != 0) return LAN_ERR_SEND_FAIL;

   for(length = 0;length < rqLen-1;length++)
   {
      _OUTB(rqData[length],DATA_OUT_REG);
      wait_for_IBF_clear();
      if (get_write_state() != 0)
         return LAN_ERR_SEND_FAIL;
      clear_OBF();
   }             

   _OUTB(WR_END,COMMAND_REG);
   wait_for_IBF_clear();
   if (get_write_state() != 0)
      return LAN_ERR_SEND_FAIL;
   clear_OBF();
   _OUTB(rqData[length],DATA_OUT_REG);

    /* write phase complete, start read phase */
    rxbuf_len = *rsLen;
    *rsLen = 0;      
    while(*rsLen <= IPMI_RSPBUF_SIZE)
     {
      wait_for_IBF_clear();
      if (get_read_state() != 0)
      {
           if (get_idle_state() != 0) {
	      DBGP2("not idle in rx_data (%02x)\n",_INB(STATUS_REG));
              // clear_lock_dir();
              return LAN_ERR_RECV_FAIL;
           } else {
                rv = wait_for_OBF_set();
	        if (rv != 0) return LAN_ERR_RECV_FAIL;
                dummy = _INB(DATA_IN_REG); 
                /* done, copy the data */
                for (length=0;length < *rsLen;length++)
                   rsData[length] = rx_data[length];           
	 	return ACCESS_OK;
           }
      } else {
	   rv = wait_for_OBF_set();
	   if (rv != 0) return LAN_ERR_RECV_FAIL;
	   rx_data[*rsLen] = _INB(DATA_IN_REG);
	   DBGP2("rx_data[%d] is 0x%x\n",*rsLen,rx_data[*rsLen]);
	   _OUTB(READ_BYTE,DATA_IN_REG); 
	   (*rsLen)++;
       }
       if (*rsLen > rxbuf_len) {
	   DBGP("ipmidir: rx buffer overrun, size = %d\n",rxbuf_len);
	   break;  /*stop if user buffer max*/
       }
     } /*end while*/
     return ACCESS_OK;
}

/* 
 * SendTimedImbpRequest_kcs - write bytes to KCS interface, read response
 * The bytes are written in this order:
 *   1  netfn
 *   2  cmdType
 *   3-N data, if any
 */
int 
SendTimedImbpRequest_kcs (IMBPREQUESTDATA *requestData, 
             unsigned int timeout, UINT8 *resp_data, int *respDataLen, 
             unsigned char *compCode)
{                    /*SendTimedImb for KCS*/
 int length;  
 unsigned char dummy;
 unsigned char rx_data[64];
 int rxbuf_len, rv;

    if (fdebugdir) {
        int cnt;
        DBGP("Send Netfn=%02x Cmd=%02x, raw: %02x %02x %02x %02x",
  		requestData->netFn, requestData->cmdType,
		requestData->busType, requestData->rsSa,
  		(requestData->netFn<<2), requestData->cmdType);
        for ( cnt=0; cnt < requestData->dataLength ; cnt++) 
              DBGP(" %02x",requestData->data[cnt]);
        DBGP("\n");
    }

   rv = wait_for_IBF_clear();
   clear_OBF();
   if (rv != 0) return LAN_ERR_SEND_FAIL;
   _OUTB(WR_START,COMMAND_REG);
   rv = wait_for_IBF_clear();
   if (get_write_state() != 0) return LAN_ERR_SEND_FAIL;
   clear_OBF();
   if (rv != 0) return LAN_ERR_SEND_FAIL;
  
   _OUTB((requestData->netFn << 2),DATA_OUT_REG);
   rv = wait_for_IBF_clear();
   if (get_write_state() != 0) return LAN_ERR_SEND_FAIL;
   clear_OBF();

   if (requestData->dataLength == 0)
   {
      _OUTB(WR_END,COMMAND_REG);
      wait_for_IBF_clear();
      if (get_write_state() != 0)
        return LAN_ERR_SEND_FAIL;
      clear_OBF();
      
      _OUTB(requestData->cmdType,DATA_OUT_REG);
   }
   else  
   {
  
    _OUTB(requestData->cmdType,DATA_OUT_REG);
      wait_for_IBF_clear();
      if (get_write_state() != 0)
         return LAN_ERR_SEND_FAIL;
      clear_OBF();
   
    for(length = 0;length < requestData->dataLength-1;length++)
     {
      _OUTB(requestData->data[length],DATA_OUT_REG);
      wait_for_IBF_clear();
      if (get_write_state() != 0)
         return LAN_ERR_SEND_FAIL;
      clear_OBF();
     }             

      _OUTB(WR_END,COMMAND_REG);
      wait_for_IBF_clear();
      if (get_write_state() != 0)
        return LAN_ERR_SEND_FAIL;
      clear_OBF();
      
      _OUTB(requestData->data[length],DATA_OUT_REG);
   }

/********************************** WRITE PHASE OVER ***********************/

#ifdef TEST_ERROR
   if (fdebugdir == 5) {  /*introduce an error test case*/
	printf("Aborting after KCS write, before read\n");
	return(rv);
   }
#endif
//     length = 0;    
//     usleep(100000);
/********************************** READ PHASE START ***********************/
    rxbuf_len = *respDataLen;
    *respDataLen = 0;      

    while(*respDataLen <= IPMI_RSPBUF_SIZE)
    {     
      wait_for_IBF_clear();
      if (get_read_state() != 0)
       {
           if (get_idle_state() != 0) {
	      DBGP2("not idle in rx_data (%02x)\n",_INB(STATUS_REG)); 
              // clear_lock_dir();
              return LAN_ERR_RECV_FAIL;
           } else {
		rv = wait_for_OBF_set();
		if (rv != 0) { return LAN_ERR_RECV_FAIL; }
		dummy = _INB(DATA_IN_REG); 
		/* done, copy the data, if valid */
		if (*respDataLen < 3) {  /* data not valid, no cc */
		   (*respDataLen) = 0; 
                   *compCode = 0xCA; /*cannot return #bytes*/
                   // clear_lock_dir();
		   return LAN_ERR_TIMEOUT;
		} else {  /*valid*/
		   requestData->netFn = rx_data[0];
		   requestData->cmdType = rx_data[1];
		   *compCode = rx_data[2];
		   (*respDataLen) -= 3;
		   for (length=0;length < *respDataLen;length++)
			resp_data[length] = rx_data[length+3];           
		}
		DBGP2("ipmidir: peak_loops = %d\n",peak_loops);
		return ACCESS_OK;
           }
       } else {
           rv = wait_for_OBF_set();
	   if (rv != 0) { return LAN_ERR_RECV_FAIL; }
           rx_data[*respDataLen] = _INB(DATA_IN_REG);
           DBGP2("rx_data[%d] is 0x%x\n",*respDataLen,rx_data[*respDataLen]);
           _OUTB(READ_BYTE,DATA_IN_REG); 
           (*respDataLen)++;
       }      
       if (*respDataLen > rxbuf_len) {
	   DBGP("ipmidir: rx buffer overrun, size = %d\n",rxbuf_len);
	   break;  /*stop if user buffer max*/
       }
    } /*end while*/
     return ACCESS_OK;
} /* end SendTimedImbpRequest_kcs */


static int ProcessTimedMessage(BMC_MESSAGE *p_reqMsg, BMC_MESSAGE *p_respMsg, const UINT32 timeout)
{
	int		status		= STATUS_OK;
	IMBPREQUESTDATA	requestData	= {0};
	int		respDataLen	= IPMI_RSPBUF_SIZE;
	UINT8		compCode	= 0;
	// static UINT8	sendSeq	= 1;
        // UINT8 buff[DATA_BUF_SIZE] = {0}; 
	ACCESN_STATUS accessn;
        int i, j;

        j = p_reqMsg->Len;
	if (j > IPMI_REQBUF_SIZE) return(LAN_ERR_BADLENGTH);
	// Initialize Response Message Data
	for(i = 0; i < IPMI_RSPBUF_SIZE; i++)
	{
		p_respMsg->Data[i] = 0;
	}

        /* show the request */
        DBGP("ipmidir Cmd=%02x NetFn=%02x Lun=%02x Sa=%02x Data(%d): ", 
		p_reqMsg->Cmd, p_reqMsg->NetFn, 
		p_reqMsg->LUN, p_reqMsg->DevAdd, j);
        for (i=0; i<j; i++) DBGP("%02x ",p_reqMsg->Data[i]);
        DBGP("\n");

        j = _IOPL(3);  
        if (j != 0) { 
	    DBGP("ipmi_direct: iopl errno = %d\n",errno);
	    return(errno); 
        }
	// Initializes Request Message

        // Call into IPMI
        if (p_reqMsg->DevAdd == 0x20) {
  	  requestData.cmdType		= p_reqMsg->Cmd;
	  requestData.rsSa		= 0x20;
	  requestData.busType		= 0;
	  requestData.netFn		= p_reqMsg->NetFn;
	  requestData.rsLun		= p_reqMsg->LUN;
	  requestData.data		= p_reqMsg->Data;
	  requestData.dataLength	= (int)p_reqMsg->Len;

	  if (g_DriverType == DRV_KCS)   /*KCS*/
		accessn = SendTimedImbpRequest_kcs(&requestData, timeout, 
				p_respMsg->Data, &respDataLen, &compCode);
	  else if (g_DriverType == DRV_SMB)  /*SMBus*/
		accessn = SendTimedImbpRequest_ssif(&requestData, timeout, 
				p_respMsg->Data, &respDataLen, &compCode);
	  else {  /* should never happen */
		printf("ipmi_direct: g_DriverType invalid [%d]\n",g_DriverType);
		return(ERR_NO_DRV);
	  }

  	  status = accessn; 
          // Return Response Message
	  p_respMsg->DevAdd	= p_reqMsg->DevAdd;
	  p_respMsg->NetFn	= requestData.netFn; 
	  p_respMsg->LUN	= p_reqMsg->LUN;
	  p_respMsg->Cmd	= requestData.cmdType;
	  p_respMsg->CompCode	= compCode;
	  p_respMsg->Len	= respDataLen;
        } else {  /*DevAdd != 0x20*/
          status = ProcessSendMessage(p_reqMsg, p_respMsg, p_reqMsg->Bus, 
				p_reqMsg->DevAdd,10000);
          DBGP2("ProcessSendMessage(cmd=%02x,rs,sa=%02x,10000) = %d\n",
		p_reqMsg->Cmd,p_reqMsg->DevAdd,status);
        }

	/* validate the response data length */
	if (p_respMsg->Len > IPMI_RSPBUF_SIZE) p_respMsg->Len = IPMI_RSPBUF_SIZE;
        /* show the response */
        j = p_respMsg->Len;
        DBGP("ipmidir Resp(%x,%x): status=%d cc=%02x, Data(%d): ", 
		(p_respMsg->NetFn >> 2), p_respMsg->Cmd,
 		status,p_respMsg->CompCode, j);        
	if (status == 0)
           for (i=0; i<j; i++) DBGP("%02x ",p_respMsg->Data[i]);
        DBGP("\n");
       
        return status;
}

/*
 * ImbInit_dir 
 * Uses SMBIOS to determine the driver type and base address.
 * It also checks the status register if KCS.
 */
int ImbInit_dir(void)
{
     uchar v = 0xff;

     /* Read SMBIOS to get IPMI struct */
     DBGP2("ImbInit: BMC_base = 0x%04x\n",BMC_base);
     if (BMC_base == 0) { /*use get_IpmiStruct routine from mem_if.c */
	   uchar iftype, iver, sa, inc;
	   int mybase, status;
	   char *ifstr;
	   status = get_IpmiStruct(&iftype,&iver,&sa,&mybase,&inc);
	   if (status == 0) {
	      if (iftype == 0x04) {
		  g_DriverType = DRV_SMB;
	 	  ifstr = "SSIF";
		  mBMC_baseAddr  = mybase;
	      } else  /*0x01==KCS*/ {
		  g_DriverType = DRV_KCS;
	      BMC_base = mybase;
	 	  ifstr = "KCS";
	          if (sa == BMC_SA && mybase != 0) { /*valid*/
		     kcsBaseAddress = mybase;
		     kcs_inc = inc;
	          }
	      }
              DBGP("SMBIOS IPMI Record found: type=%s sa=%02x base=0x%04x spacing=%d\n",
		   ifstr, sa, mybase, inc);
	   }
     }

     /* Use KCS here. There are no known SMBus implementations on 64-bit */
     if (BMC_base == 0) {
         DBGP("No IPMI Data Structure Found in SMBIOS Table,\n");
#ifdef TRY_KCS
         g_DriverType    = DRV_KCS;
         BMC_base = kcsBaseAddress;
         DBGP("Continuing with KCS on Default Port 0x%04x\n",kcsBaseAddress);
#else
         printf("No IPMI interface detected...Exiting\n");
         return ERR_NO_DRV;
#endif
     }
#if defined(BSD) || defined(MACOS) || defined(HPUX)
     iofd = open("/dev/io",O_RDWR);
     if (iofd < 0) {
         printf("Cannot open /dev/io...Exiting\n");
         return ERR_NO_DRV;
     }
#endif
     if (g_DriverType == DRV_SMB) {
     /* Perhaps add controller type in ipmi_if.txt (?)*/
     /* Intel SSIF: 0x0540=SJR, 0x0400=STP */
       if (mBMC_baseAddr == 0x540 || mBMC_baseAddr == 0x400) 
	     SMBChar.Controller = INTEL_SMBC;
       else   /*else try ServerWorks*/
	     SMBChar.Controller = SW_SMBC;
	   SMBChar.baseAddr = mBMC_baseAddr;
       DBGP("BMC SSIF/SMBus Interface at i2c=%02x base=0x%04x\n",
		mBMCADDR,mBMC_baseAddr);
     }
     if (g_DriverType == DRV_KCS) {
        v = _IOPL(3);
        v = _INB(STATUS_REG);
        DBGP2("inb(%x) returned %02x\n",STATUS_REG,v);
        if (v == 0xff) {
            printf("No Response from BMC...Exiting\n");
            return ERR_NO_DRV;
        }
        DBGP("BMC KCS Initialized at 0x%04x\n",kcsBaseAddress);
     }
     return STATUS_OK;
}

int 
SendTimedImbpRequest_ssif ( IMBPREQUESTDATA *requestData, 
             unsigned int timeout, UINT8 *resp_data, int *respDataLen, 
             unsigned char *compCode)
{                   /* SendTimedImb for SMBus */
 unsigned char rq[IPMI_REQBUF_SIZE+35] = {0,}; /*SIZE + MAX_ISA_LENGTH=35*/
 unsigned char rp[IPMI_RSPBUF_SIZE+35] = {0,}; /*SIZE + MAX_ISA_LENGTH=35*/
 unsigned int  i, rpl=0;  
 int status;
 int respMax, rlen;

 i = _IOPL(3);
 rq[0] = ((requestData->netFn << 2) | (requestData->rsLun & 0x03));
 rq[1] = requestData->cmdType;
 if (sizeof(rq) < (requestData->dataLength + 2)) return(LAN_ERR_BADLENGTH);
 for (i=0;i<=(unsigned int)requestData->dataLength;i++) 
        rq[i+2] = requestData->data[i];

 respMax = *respDataLen;
 if (respMax == 0) respMax = IPMI_RSPBUF_SIZE;
 status = SendmBmcRequest(rq,requestData->dataLength+2,rp,&rpl,0);
 if (status == IMB_SEND_ERROR) {
     *respDataLen = 0;
     return LAN_ERR_SEND_FAIL;
 }
 
 if (rpl < 3) {
    *respDataLen = 0;
    *compCode = 0xCA; /*cannot return #bytes*/
    return LAN_ERR_TIMEOUT;
 } else {
    rlen = rpl-3;  /* Chop off netfn/LUN , compcode, command */
    if (rlen > respMax) rlen = respMax;
    *respDataLen = rlen;
    *compCode = rp[2];
    for (i = 0; i < rlen; i++) resp_data[i] = rp[i+3];  
 }
 return ACCESS_OK;
}

int SendmBmcRequest (
        unsigned char*    request,
        UINT32           reqLength,
        unsigned char*   response,
        UINT32 *         respLength,
        UINT32           ipmiOldFlag
        )
{
    int retries = 5;

    /* Send Request - Retry 5 times at most */
    do{
        if (SendRequest(request, reqLength, &SMBChar) == 0) 
            break;
    } while (0 < retries--);
    
    if (retries <= 0) { return IMB_SEND_ERROR; }
    
    /* Read Response - Retry 5 times at most */
    retries = 5;
    do{
        if ((*respLength = ReadResponse(response, &SMBChar))!=(-1)) {
            break;  // Success
        }
    }while (0 < retries--);
    
    if (retries <= 0) { return IMB_SEND_ERROR; }

    return IMB_SUCCESS;
}

static int 
SendRequest(unsigned char* req, int length,
                    struct SMBCharStruct* smbChar)
{
    UINT8 data = 0;
    // unsigned char* msgBuf = req;
    int         i; 
    UINT8 status = 0;
    
    // Delay of 50 ms before request is sent
    usleep(100000);
    
        // Handle Intel & ServerWorks Chipsets
        // Clear all status bits, Host Status Register
        switch (smbChar->Controller) {
            // Intel Status Register
            case INTEL_SMBC:
                status = ICH_HST_STA_ALL_ERRS|ICH_HST_STA_INTR|ICH_HST_STA_BYTE_DONE_STS;
                break;
            // ServerWorks Status Register
            case SW_SMBC:
                status = ICH_HST_STA_ALL_ERRS|ICH_HST_STA_INTR;
                break;
        }// End of Switch
        
        // Status Register
        WritePortUchar(  (((smbChar->baseAddr))+ICH_HST_STA), status);

        // Block protocol, Host Control Register
        WritePortUchar(((smbChar->baseAddr)+ICH_HST_CNT), (UINT8)(ICH_HST_CNT_SMB_CMD_BLOCK));
        // IPMI command, Host Command Register
        WritePortUchar(((smbChar->baseAddr)+ICH_HST_CMD), (UINT8)(0x02));
        // Slave address, Host Address Register
        WritePortUchar(((smbChar->baseAddr)+ICH_XMIT_SLVA), (UINT8)(mBMCADDR));
        // Block length, Host DATA0 Register
        WritePortUchar(((smbChar->baseAddr)+ICH_D0), (UINT8)(length));
        // Initialize timer
        
        switch (smbChar->Controller) {
            // Handle Intel Chipset
            case INTEL_SMBC:
                // the first byte
                WritePortUchar(((smbChar->baseAddr)+ICH_BLOCK_DB), *(UINT8 *)((UINT8 *)req ));
                // Block protocol and Start, Host Control Register
                WritePortUchar(((smbChar->baseAddr)+ICH_HST_CNT), ICH_HST_CNT_START | ICH_HST_CNT_SMB_CMD_BLOCK);
                // Send byte by byte
                for ( i = 1; i < length; i++ ) {
                    //   OsSetTimer(&SendReqTimer);  * Start timer *
                    do {
                        // Read host status register
                        ReadPortUchar((smbChar->baseAddr+ICH_HST_STA), &data);
                        //    if (OsTimerTimedout(&SendReqTimer))  return -1;
                        // Any error or Interrupt bit or byte done bit set ?
                    } while ((data & status) == 0);
                    //    OsCancelTimer(&SendReqTimer); * End Timer *
                    // Check for byte completion in block transfer
                    if ((data & ICH_HST_STA_BYTE_DONE_STS) != ICH_HST_STA_BYTE_DONE_STS)
                         break;
                    // Write next byte
                    WritePortUchar(((smbChar->baseAddr)+ICH_BLOCK_DB), *(UINT8 *)((UINT8 *)req + i));
                    // Clear status bits, Host Status Register
                    WritePortUchar((smbChar->baseAddr), (UINT8)(data & status));
                } // End of for - Send byte by byte
            break; // End of Intel Chipset Handling
            // Handle ServerWorks Chipset
            case SW_SMBC:
                // Block length, Host DATA1 Register
                WritePortUchar(((smbChar->baseAddr)+ICH_D1), (UINT8)(length));
                // Reset block inex
                ReadPortUchar(((smbChar->baseAddr)+ICH_HST_CNT), &data);
                // Put all bytes of the message to Block Data Register
                for ( i = 0; i < length; i++ )
                    WritePortUchar(((smbChar->baseAddr)+ICH_BLOCK_DB), *(UINT8 *)((UINT8 *)req + i));
                // Block protocol and Start, Host Control Register
                WritePortUchar(((smbChar->baseAddr)+ICH_HST_CNT), ICH_HST_CNT_START | ICH_HST_CNT_SMB_CMD_BLOCK);
                //   OsSetTimer(&SendReqTimer);  * Start timer *
                do {
                    // Read host status register
                    ReadPortUchar((smbChar->baseAddr+ICH_HST_STA), &data);
                    //    if (OsTimerTimedout(&SendReqTimer))  return -1;
                } while ((data & status) == 0);    // Any error or Interrupt bit set ?
                //   OsCancelTimer(&SendReqTimer); * End Timer *
            break; // End of ServerWorks Chipset Handling
        } // End of Switch Controller
        
        // Clear status bits, Host Status Register
        WritePortUchar((smbChar->baseAddr), (UINT8)(data & status));
        if (data & ICH_HST_STA_ALL_ERRS) return -1;
         
    // Success
    return 0;
}

static int
ReadResponse( unsigned char*  resp,
    struct SMBCharStruct *  smbChar)
{
    UINT8 data, dummy, length, status = 0;
    UINT8 *  msgBuf = (UINT8 *) resp;
    int     i; //, timeout =0;
    //  os_timer_t  SendReqTimer;
    
    // Delay current thread - time to delay in uSecs
    // 100 msec = 100000 microsec
    usleep(100000);
    
        // Handle Intel & ServerWorks Chipsets
        switch (smbChar->Controller) {
            case INTEL_SMBC:
                status = ICH_HST_STA_ALL_ERRS|ICH_HST_STA_INTR|ICH_HST_STA_BYTE_DONE_STS;
                break;
            case SW_SMBC:
                status = ICH_HST_STA_ALL_ERRS|ICH_HST_STA_INTR;
                break;
        } // End of Switch
        
        WritePortUchar(((smbChar->baseAddr)+ICH_HST_STA), status);
        // Block protocol, Host Control Register
        WritePortUchar(((smbChar->baseAddr)+ICH_HST_CNT), ICH_HST_CNT_SMB_CMD_BLOCK);
        // Slave address, Host Address Register
        WritePortUchar(((smbChar->baseAddr)+ICH_XMIT_SLVA), (UINT8)(mBMCADDR |ICH_XMIT_SLVA_READ));
        // IPMI command, Host Command Register
        WritePortUchar(((smbChar->baseAddr)+ICH_HST_CMD), 0x03);
        // Reset block index
        ReadPortUchar(((smbChar->baseAddr)+ICH_HST_CNT), &data);
        // Block protocol and Start, Host Control Register
        WritePortUchar(((smbChar->baseAddr)+ICH_HST_CNT), ICH_HST_CNT_START | ICH_HST_CNT_SMB_CMD_BLOCK);

        // Initialize timer
        do {
            // Read host status register
            ReadPortUchar((smbChar->baseAddr+ICH_HST_STA), &data);
        } while ((data & status) == 0); // Any error or Interrupt bit set ?
        // End Timer
        
        // Clear status bits, Host Status Register
        WritePortUchar((smbChar->baseAddr), (UINT8)(data & status));
        
        if (data & ICH_HST_STA_ALL_ERRS) return -1;
        
        // Block length, Host DATA0 Register
        ReadPortUchar(((smbChar->baseAddr)+ICH_D0), &length);
        /* check recv length > MAX (35) */
        if (length > MAX_ISA_LENGTH) length = MAX_ISA_LENGTH;
        switch (smbChar->Controller) {
            case INTEL_SMBC:
                // Read the first byte
                ReadPortUchar((smbChar->baseAddr+ICH_BLOCK_DB), &(msgBuf[0]));
                // Put all bytes of the message to Block Data Register
                for ( i = 1; i < length; i++ ) {
                    if (i == (length-1)) {
                        // Set Last Byte bit
                        ReadPortUchar(((smbChar->baseAddr)+ICH_HST_CNT),&dummy);
                        WritePortUchar(((smbChar->baseAddr)+ICH_HST_CNT), (UINT8)(dummy | ICH_HST_CNT_LAST_BYTE));
                    }
                    // Clear status bits, Host Status Register
                    WritePortUchar((smbChar->baseAddr), (UINT8)(data & status));

                    do {
                        // Read host status register
                        ReadPortUchar((smbChar->baseAddr+ICH_HST_STA), &data);
                    } while ((data & status) == 0);    // Any error or Interrupt bit set ?

                    if (data & ICH_HST_STA_ALL_ERRS) return -1;
                    ReadPortUchar((smbChar->baseAddr+ICH_BLOCK_DB), &(msgBuf[i]));
                }
                break;
            case SW_SMBC:
                // Put all bytes of the message to Block Data Register
                for ( i = 0; i < length; i++ ) {
                    ReadPortUchar((smbChar->baseAddr+ICH_BLOCK_DB), &(msgBuf[i]));
                }
                break;
        } /* end of switch */
        
        if (data & ICH_HST_STA_ALL_ERRS) return -1;
    
    return length;
}
#endif

/* end ipmidir.c */
