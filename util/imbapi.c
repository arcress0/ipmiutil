/*M*
//  PVCS:
//      $Workfile:   imbapi.c  $
//      $Revision:   1.12  $
//      $Modtime:   06 Aug 2001 13:16:56  $
//
//  Purpose:    This file contains the entry point that opens the IMB device 
//              in order to issue the  IMB driver API related IOCTLs.
//              This file implements the IMB driver API for the Server 
//              Management Agents.
//
// $Log: //epg-scc/sst/projectdatabases/isc/isc3x/LNWORK/SRC/imbapi/imbapi.c.v $
// 
//  04/04/02 ARCress - Mods for open-source & various compile cleanup mods
//  05/28/02 ARCress - fixed buffer size error
//  04/08/03 ARCress - misc cleanup
//  01/20/04 ARCress - added WIN32 flags
//  05/24/05 ARCress - added LINK_LANDESK flags
//  08/24/06 ARCress - mods for addtl DEBUG (dbgmsg)
//  03/19/08 ARCress - fix for SendTimedLan(IpmiVersion) if IPMI 2.0
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

#if defined(SOLARIS) || defined(BSD)
#undef IMB_API 
#else
/* Ok for WIN32, LINUX, SCO_UW, etc., but not Solaris */
#define IMB_API 1
#endif

#ifdef IMB_API

#ifdef WIN32
#define NO_MACRO_ARGS  1
#include <windows.h>
#include <stdio.h>
#define  uint  unsigned int

#else  /* LINUX, SCO_UW, UNIX */
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#endif
#include "imb_api.h"

#ifdef SCO_UW
#define NO_MACRO_ARGS  1
#define __FUNCTION__ "func"
#define IMB_DEVICE "/dev/instru/mismic"
#else
#define IMB_DEVICE "/dev/imb"
#define PAGESIZE EXEC_PAGESIZE
#endif

/*Just to make the DEBUG code cleaner.*/
#if defined(DBG_IPMI) || defined(LINUX_DEBUG) 
#include <stdarg.h>
static void dbgmsg( char *pattn, ... )
{
    va_list arglist;
    FILE *fdlog;

    fdlog = fopen( "imbdbg.log", "a+" );
    if (fdlog == NULL) fdlog = stdout;
    va_start( arglist, pattn );
    vfprintf( fdlog, pattn, arglist );
    va_end( arglist );
    fclose(fdlog);
}
#endif
#ifdef NO_MACRO_ARGS
#define DEBUG(format, args)  /*not_debug*/
#else
#ifdef LINUX_DEBUG
#define DEBUG(format, args...) dbgmsg(format, ##args)
// #define DEBUG(format, args...) printf(format, ##args)
#else
#define DEBUG(format, args...)  /*not_debug*/
#endif
#endif

#define IMB_OPEN_TIMEOUT      400
#define IMB_MAX_RETRIES       2
#define IMB_DEFAULT_TIMEOUT   (1000)   /*timeout for SendTimedImb in msec*/
#define ERR_NO_DRV         -16 /*cannot open IPMI driver*/

#define LOCAL_IPMB  1   /*instead of METACOMMAND_IPMB*/

int ipmi_timeout_ia = IMB_DEFAULT_TIMEOUT;  /* in msec */
#ifdef METACOMMAND_IPMB   /*++++ not used*/
extern int ipmi_cmd_ipmb(BYTE cmd, BYTE netfn, BYTE sa, BYTE bus, BYTE lun,
                BYTE *pdata, int sdata, BYTE *presp,
                int *sresp, BYTE *pcc, char fdebugcmd); /*ipmilan.c*/
#endif
#ifdef METACOMMAND
extern FILE *fpdbg;
extern FILE *fperr;
#else
static FILE *fpdbg = NULL;
static FILE *fperr = NULL;
#endif

/* uncomment out the #define below or use -DLINUX_DEBUG_MAX in the makefile 
// if you want a dump of the memory to debug mmap system call in
// MapPhysicalMemory() below.
// 
//#define LINUX_DEBUG_MAX 1 */
//#define IMB_MEMORY  1  */

/*keep it simple. use global varibles for event objects and handles
//pai 10/8 */

/* UnixWare should eventually have its own source code file. Right now
// new code has been added based on the existing policy of using
// pre-processor directives to separate os-specific code (pai 11/21) */

static int  IpmiVersion;
static HandleType AsyncEventHandle = 0;
// static void *  AsyncEventObject = 0; 

/*////////////////////////////////////////////////////////////////////////////
//  GLOBAL VARIABLES
///////////////////////////////////////////////////////////////////////////// */

#if defined(__i386__) || defined(__i586__) || defined(__i686__)
// static DWORD ioctl_sendmsg = 0x1082;    /* force 0x1082 if 32-bit */
static DWORD ioctl_sendmsg = IOCTL_IMB_SEND_MESSAGE; 
#else
static DWORD ioctl_sendmsg = IOCTL_IMB_SEND_MESSAGE; 
#endif

static IO_STATUS_BLOCK NTstatus; /*dummy place holder. See deviceiocontrol. */
static HandleType hDevice1; /*used in open_imb() */
static HandleType hDevice;  /*used to call DeviceIoControl()*/
/*mutex_t deviceMutex; */
#ifdef LINUX_DEBUG 
static char fdebug = 1;
#else
static char fdebug = 0;
#endif

#ifdef LINK_LANDESK
#define SendTimedImbpRequest ia_SendTimedImbpRequest
#define StartAsyncMesgPoll ia_StartAsyncMesgPoll
#define SendTimedI2cRequest ia_SendTimedI2cRequest
#define SendTimedEmpMessageResponse ia_SendTimedEmpMessageResponse
#define SendTimedEmpMessageResponse_Ex ia_SendTimedEmpMessageResponse_Ex
#define SendTimedLanMessageResponse ia_SendTimedLanMessageResponse
#define SendTimedLanMessageResponse_Ex ia_SendTimedLanMessageResponse_Ex
#define SendAsyncImbpRequest ia_SendAsyncImbpRequest
#define GetAsyncImbpMessage ia_GetAsyncImbpMessage
#define GetAsyncImbpMessage_Ex ia_GetAsyncImbpMessage_Ex
#define IsAsyncMessageAvailable ia_IsAsyncMessageAvailable
#define RegisterForImbAsyncMessageNotification ia_RegisterForImbAsyncMessageNotification
#define UnRegisterForImbAsyncMessageNotification ia_UnRegisterForImbAsyncMessageNotification
#define SetShutDownCode ia_SetShutDownCode
#define GetIpmiVersion ia_GetIpmiVersion
#endif

int open_imb(int fskipcmd);

void set_fps(void)
{
   if (fpdbg == NULL) fpdbg = stdout;
   if (fperr == NULL) fperr = stdout;
}

#ifndef WIN32
DWORD GetLastError(void) { return(NTstatus.Status); }  

/*///////////////////////////////////////////////////////////////////////////
// DeviceIoControl 
///////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       DeviceIoControl 
//  Purpose:    Simulate NT DeviceIoControl using unix calls and structures.
//  Context:    called for every NT DeviceIoControl
//  Returns:    FALSE for fail and TRUE for success. Same as standarad NTOS call
//              as it also sets Ntstatus.status.
//  Parameters: Standard NT call parameters, see below.
//  Notes:      none
*F*/
static BOOL
DeviceIoControl(
	HandleType		dummy_hDevice, /* handle of device */
	DWORD 			dwIoControlCode, /* control code of operation to perform*/
	LPVOID 			lpvInBuffer, /* address of buffer for input data */
	DWORD 			cbInBuffer, /* size of input buffer */
	LPVOID 			lpvOutBuffer, /* address of output buffer */
	DWORD 			cbOutBuffer, /* size of output buffer */
	LPDWORD 		lpcbBytesReturned, /* address of actual bytes of output */
	LPOVERLAPPED 	lpoOverlapped /* address of overlapped struct */
	)
{
	struct smi s;
	int rc, max_i;
	int ioctl_status;

        DEBUG("DeviceIoControl: hDevice1=%x hDevice=%x ioctl=%x\n",
			hDevice1,hDevice,dwIoControlCode);
  	rc = open_imb(1); /*open if needed, set fskipcmd to avoid recursion*/
  	if (rc == 0) {
#ifdef DBG_IPMI
  	  dbgmsg("DeviceIoControl: open_imb failed %d\n", rc);
#endif
    	  return FALSE;
    	  }

	/*-------------dont use locking--------------
	    //lock the mutex, before making the request....
	    if(mutex_lock(&deviceMutex) != 0) {
			return(FALSE);
	    }
	 *-------------------------------------------*/
#ifndef NO_MACRO_ARGS
	DEBUG("%s: ioctl cmd = 0x%lx ", __FUNCTION__,dwIoControlCode);
	DEBUG("cbInBuffer %d cbOutBuffer %d\n", cbInBuffer, cbOutBuffer);
#endif
	/* Test for Intel imb driver max buf size */
	/* Max was 41, too small.  Calculate real max -  ARCress 6/01/05 */
	max_i = MAX_IMB_PACKET_SIZE + MIN_IMB_REQ_BUF_SIZE; /*33 + 13*/
	if (cbInBuffer > max_i) cbInBuffer = max_i;  

  	s.lpvInBuffer = lpvInBuffer;
  	s.cbInBuffer = cbInBuffer;
  	s.lpvOutBuffer = lpvOutBuffer;
  	s.cbOutBuffer = cbOutBuffer;
  	s.lpcbBytesReturned = lpcbBytesReturned;
  	s.lpoOverlapped = lpoOverlapped;
  	s.ntstatus = (LPVOID)&NTstatus; /*dummy place holder. Linux IMB driver
					//doesnt return status or info via it.*/
#ifdef IMB_DEBUG
        if (fdebug) {
          int j, n;
          char *tag;
          unsigned int *bi;
          unsigned char *b;
 
          printf("ioctl %x smi buffer:\n",dwIoControlCode);
          /* show arg/smi structure */
          bi = (unsigned int *)&s;
          n = ( sizeof(struct smi) / sizeof(int) );
          printf("smi(%p), (sz=%d/szint=%d)->%d:\n", bi, sizeof(struct smi),
                                sizeof(int),n);
          for (j = 0; j < n; j++) {
            switch(j) {
                case 0:   tag = "version  "; break;
                case 2:   tag = "reserved1"; break;
                case 4:   tag = "reserved2"; break;
                case 6:   tag = "ntstatus "; break;
                case 8:   tag = "pinbuf "; break;
                case 10:  tag = "sinbuf "; break;
                case 12:  tag = "poutbuf"; break;
                case 14:  tag = "soutbuf"; break;
                case 16:  tag = "cbRet  "; break;
                case 18:  tag = "pOverlap"; break;
                default:  tag = "other  ";
            }
            printf("bi[%d]%s: %08x \n",j,tag,bi[j]);
          } /*end for*/
        } 
#endif

  	if ( (ioctl_status = ioctl(hDevice1, dwIoControlCode,&s) ) <0) {
#ifndef NO_MACRO_ARGS
 	  	DEBUG("%s %s: ioctl cmd = 0x%x failed %d \n", 
			__FILE__,__FUNCTION__,dwIoControlCode,ioctl_status);
#endif
#ifdef DBG_IPMI
 	  	dbgmsg("DeviceIoControl: ioctl cmd = 0x%x failed %d \n", 
			dwIoControlCode,ioctl_status);
#endif
		/*      mutex_unlock(&deviceMutex); */
    		return FALSE;
    	}
	/*-------------dont use locking--------------
	    mutex_unlock(&deviceMutex);
	 *-------------------------------------------*/

#ifndef NO_MACRO_ARGS
	DEBUG("%s: ioctl_status %d  bytes returned =  %d \n",
		 __FUNCTION__, ioctl_status,  *lpcbBytesReturned);
#endif

/*MR commented this just as in Sol1.10. lpcbBytesReturned has the right data
//  	*lpcbBytesReturned = NTstatus.Information; */
  
	if (ioctl_status == STATUS_SUCCESS) {
#ifndef NO_MACRO_ARGS
		DEBUG("%s returning true\n", __FUNCTION__);
#endif
     	return (TRUE);
	}
	else {
#ifndef NO_MACRO_ARGS
		DEBUG("%s returning false\n", __FUNCTION__);
#endif
     	return (FALSE);
     }
}
#endif

static void _dump_buf(char *tag, BYTE *pbuf, int sz, char fshowascii)
{
   BYTE line[17];
   BYTE a;
   int i, j;
   char *stag;
   FILE *fpdbg1;

#ifdef DBG_IPMI
   fpdbg1 = fopen( "imbdbg.log", "a+" );
   if (fpdbg1 == NULL) fpdbg1 = stdout;
#else
   fpdbg1 = stdout;
#endif
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
#ifdef DBG_IPMI
   if (fpdbg1 != stdout) fclose(fpdbg1);
#endif
   return;
}

#ifdef LOCAL_IPMB
#define SMS_MSG_LUN              0x02
extern void os_usleep(int s, int u);  /*subs.c*/
static int sendSeq = 1;
static BYTE cksum(const BYTE *buf, register int len)
{     /* 8-bit 2s compliment checksum */
        register BYTE csum;
        register int i;
        csum = 0;
        for (i = 0; i < len; i++) csum = (csum + buf[i]) % 256;
        csum = -csum;
        return(csum);
}

// SendTimedIpmbpRequest
ACCESN_STATUS
SendTimedIpmbpRequest (
		IMBPREQUESTDATA *reqPtr,         /* request info and data */
		int     timeOut,         /* how long to wait, in mSec units */
		BYTE 	*respDataPtr,    /* where to put response data */
		int 	*respDataLen,    /* how much response data there is */
		BYTE 	*completionCode  /* request status from bmc controller*/
	)
{
	BYTE                    responseData[MAX_IMB_REQ_SIZE];
	ImbResponseBuffer *     resp = (ImbResponseBuffer *) responseData;
	DWORD                   respLength = sizeof( responseData );
	DWORD                   reqLength;
	BYTE                    requestData[MAX_IMB_RESP_SIZE];
	ImbRequestBuffer *      req = (ImbRequestBuffer *) requestData;
	BOOL                    status;
	int i;

	req->req.rsSa           =  BMC_SA;
	req->req.cmd            =  SEND_MESSAGE;	
	req->req.netFn          =  APP_NETFN;
	req->req.rsLun          =  BMC_LUN; /*=0*/
	req->req.data[0] = reqPtr->busType;
	req->req.data[1] = reqPtr->rsSa;
	req->req.data[2] = ((reqPtr->netFn << 2) | (reqPtr->rsLun & 0x03));
	req->req.data[3] = cksum(&req->req.data[1],2);
	req->req.data[4] = BMC_SA;
	req->req.data[5] = ((sendSeq << 2) | (SMS_MSG_LUN & 0x03));
	req->req.data[6] = reqPtr->cmdType;
	for (i = 0; i < reqPtr->dataLength; i++)
		req->req.data[7+i] = reqPtr->data[i];
	req->req.data[7+i] = cksum(&req->req.data[4], reqPtr->dataLength + 3);
	req->req.dataLength = reqPtr->dataLength + 8;

	req->flags              = 0;
	req->timeOut    = timeOut * 1000;       /* convert to uSec units */
	reqLength = req->req.dataLength + MIN_IMB_REQ_BUF_SIZE;
	status = DeviceIoControl(       hDevice,
					ioctl_sendmsg,
					requestData,
					reqLength,
					& responseData,
					sizeof( responseData ),
					& respLength,
					NULL);
	if (fdebug) printf("sendIpmb: send_message status=%d rlen=%lu cc=%x\n",
				status,respLength,resp->cCode);
	if ( status != TRUE ) {
		DWORD error;
		error = GetLastError();
		return ACCESN_ERROR;
	}
	if ( respLength == 0 ) {
		return ACCESN_ERROR;
	}
	sendSeq++;
	if ( resp->cCode != 0) {
		*completionCode = resp->cCode;
		*respDataLen    = 0;
		return ACCESN_OK;
	}

	/*
	 * Sent ok, wait for GetMessage response 
	 */
      for (i=0; i < 10; i++)    /* RETRIES=10 for GetMessage  */
      {
	//req->req.busType        =  PUBLIC_BUS; /*=0*/
	req->req.rsSa           =  BMC_SA;
	req->req.cmd            =  GET_MESSAGE;	
	req->req.netFn          =  APP_NETFN;
	req->req.rsLun          =  BMC_LUN; /*=0*/
	req->req.dataLength = 0;
	reqLength = req->req.dataLength + MIN_IMB_REQ_BUF_SIZE;
	status = DeviceIoControl(       hDevice,
					ioctl_sendmsg,
					requestData,
					reqLength,
					& responseData,
					sizeof( responseData ),
					& respLength,
					NULL);
	if (fdebug) printf("sendIpmb: get_message status=%d rlen=%lu cc=%x\n",
				status,respLength,resp->cCode);
	if ( status != TRUE ) {
		DWORD error;
		error = GetLastError();
		return ACCESN_ERROR;
	}
	if ( respLength == 0 ) {
		return ACCESN_ERROR;
	}
	if ( (resp->cCode != 0x80) && (resp->cCode != 0x83) ) 
	   break;  /*success, exit loop*/
	os_usleep(0,1000);  /* 1 msec*/
      } /*end-for*/

	/*
	 * give the caller his response
	 */
	*completionCode = resp->cCode;
	*respDataLen    = 0;
	if (( respLength > 1 ) && ( respDataPtr)) {
		/* check that resp cmd & netfn match */
		*respDataLen    = respLength - 7;
		memcpy( respDataPtr, &resp->data[7], *respDataLen);
	}
	return ACCESN_OK;
}
#endif

/*///////////////////////////////////////////////////////////////////////////
// SendTimedImbpRequest
///////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       SendTimedImbpRequest 
//  Purpose:    This function sends a request for BMC implemented function
//  Context:    Used by Upper level agents (sis modules) to access BMC implemented functionality. 
//  Returns:    OK  else error status code
//  Parameters: 
//     reqPtr
//     timeOut
//     respDataPtr
//     respLen
//  Notes:      none
*F*/
ACCESN_STATUS
SendTimedImbpRequest (
		IMBPREQUESTDATA *reqPtr,         /* request info and data */
		int     timeOut,         /* how long to wait, in mSec units */
		BYTE 	*respDataPtr,    /* where to put response data */
		int 	*respDataLen,    /* how much response data there is */
		BYTE 	*completionCode  /* request status from bmc controller*/
	)
{
	BYTE                    responseData[MAX_IMB_REQ_SIZE];
	ImbResponseBuffer *     resp = (ImbResponseBuffer *) responseData;
	DWORD                   respLength = sizeof( responseData );
	DWORD                   reqLength;
	BYTE                    requestData[MAX_IMB_RESP_SIZE];
	ImbRequestBuffer *      req = (ImbRequestBuffer *) requestData;
	BOOL                    status;


	req->req.rsSa           = reqPtr->rsSa;
	req->req.cmd            = reqPtr->cmdType;
	req->req.netFn          = reqPtr->netFn;
	req->req.rsLun          = reqPtr->rsLun;
	req->req.dataLength     = (BYTE)reqPtr->dataLength;

#ifndef NO_MACRO_ARGS
	DEBUG("cmd=%02x, pdata=%p, datalen=%d, size=%d\n", req->req.cmd, 
	      reqPtr->data, reqPtr->dataLength, sizeof(requestData));
#endif
	memcpy( req->req.data, reqPtr->data, reqPtr->dataLength );

	req->flags              = 0;
	req->timeOut    = timeOut * 1000;       /* convert to uSec units */

#ifndef NO_MACRO_ARGS
	DEBUG("%s: rsSa 0x%x cmd 0x%x netFn 0x%x rsLun 0x%x\n", __FUNCTION__,
			req->req.rsSa, req->req.cmd, req->req.netFn, req->req.rsLun);
#endif
	reqLength = req->req.dataLength + MIN_IMB_REQ_BUF_SIZE;

	status = DeviceIoControl(       hDevice,
					ioctl_sendmsg,
					requestData,
					reqLength,
					& responseData,
					sizeof( responseData ),
					& respLength,
					NULL
				  );
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl returned status = %d\n",__FUNCTION__, status);
#endif
#ifdef DBG_IPMI
	printf("%s: rsSa %x cmd %x netFn %x lun %x, status=%d, cc=%x, rlen=%d\n", 
            __FUNCTION__, req->req.rsSa, req->req.cmd, req->req.netFn, 
            req->req.rsLun, status,  resp->cCode, respLength );
        dbgmsg("SendTimedImb: rsSa %x cmd %x netFn %x lun %x, status=%d, cc=%x, rlen=%d\n", 
            req->req.rsSa, req->req.cmd, req->req.netFn, 
            req->req.rsLun, status,  resp->cCode, respLength );
	if (req->req.cmd == 0x34) {
          _dump_buf("requestData",requestData,reqLength,0); //if DBG_IPMI
          _dump_buf("responseData",responseData,respLength,0); //if DBG_IPMI
        }
#endif

	if( status != TRUE ) {
		DWORD error;
		error = GetLastError();
		return ACCESN_ERROR;
	}
	if( respLength == 0 ) {
		return ACCESN_ERROR;
	}

	/*
	 * give the caller his response
	 */
	*completionCode = resp->cCode;
	*respDataLen    = 0;

    if(( respLength > 1 ) && ( respDataPtr))
	{
		*respDataLen    = respLength - 1;
		memcpy( respDataPtr, resp->data, *respDataLen);
	}


	return ACCESN_OK;
}


/*////////////////////////////////////////////////////////////////////
// open_imb
////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       open_imb
//  Purpose:    To open imb device
//  Context:    Called from each routine to make sure that open is done.
//  Returns:    returns 0 for Fail and 1 for Success, sets hDevice to open
//              handle.
//  Parameters: none
//  Notes:      none
*F*/
#ifdef WIN32
#define IMB_DEVICE_WIN  "\\\\.\\Imb"
int open_imb(int fskipcmd)
{
/* This routine will be called from all other routines before doing any
   interfacing with imb driver. It will open only once. */
	IMBPREQUESTDATA                requestData;
	BYTE			   respBuffer[MAX_IMB_RESP_SIZE];
	DWORD			   respLength;
	BYTE			   completionCode;
	int ret;

  set_fps();
  if (hDevice1 == 0)  /*INVALID_HANDLE_VALUE*/
  {
        //
        // Open IMB driver device
        //
        hDevice = CreateFile(   IMB_DEVICE_WIN, 
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                            );
        if (hDevice == NULL || hDevice == INVALID_HANDLE_VALUE) {
	    if (fdebug) printf("ipmi_open_ia: error opening %s to imbdrv.sys\n",
				IMB_DEVICE_WIN);
            return (0);  /*FALSE*/
	}

	if (fskipcmd) {
	   IpmiVersion = IPMI_15_VERSION;
	    return (1);  /*TRUE*/
	}
        // Detect the IPMI version for processing requests later.
        // This is a crude but most reliable method to differentiate
        // between old IPMI versions and the 1.0 version. If we had used the
        // version field instead then we would have had to revalidate all the
        // older platforms (pai 4/27/99)
        requestData.cmdType            = GET_DEVICE_ID;
        requestData.rsSa               = BMC_SA;
        requestData.rsLun              = BMC_LUN;
        requestData.netFn              = APP_NETFN ;
        requestData.busType            = PUBLIC_BUS;
        requestData.data               = NULL;
        requestData.dataLength         = 0;
        respLength                     = sizeof(respBuffer);
	ret = SendTimedImbpRequest ( &requestData, IMB_OPEN_TIMEOUT,
                         respBuffer, &respLength, &completionCode);
	if ( (ret != ACCESN_OK ) || ( completionCode != 0) )
        {
	    if (fdebug)   /*GetDeviceId failed*/
	        printf("ipmi_open_ia: imbdrv request error, ret=%d ccode=%x\n",
					ret,completionCode);
            CloseHandle(hDevice);
            return (0);  /*FALSE*/
        }
        hDevice1 = hDevice;

        if (respLength < (IPMI10_GET_DEVICE_ID_RESP_LENGTH-1))
            IpmiVersion = IPMI_09_VERSION;
	else {
			if ( respBuffer[4] == 0x01 )
				IpmiVersion = IPMI_10_VERSION;
			else   /* IPMI 1.5 or 2.0 */
				IpmiVersion = IPMI_15_VERSION;
		}
   }
   return (1);  /*TRUE*/

} /*end open_imb for Win32 */

#else  /* LINUX, SCO_UW, etc. */

int open_imb(int fskipcmd)
{
/* This routine will be called from all other routines before doing any
   interfacing with imb driver. It will open only once. */
	IMBPREQUESTDATA          requestData;
	BYTE			 respBuffer[MAX_IMB_RESP_SIZE];
	int			 respLength;
	BYTE			 completionCode;

	int ret_code;

  set_fps();

  if (hDevice1 == 0)
  {
#ifndef NO_MACRO_ARGS
	DEBUG("%s: opening the driver, ioctl_sendmsg = %x\n", 
		__FUNCTION__, ioctl_sendmsg);
#endif
	/* %%%%*  IOCTL_IMB_SEND_MESSAGE = 0x1082 
      printf("ipmi_open_ia: "
	"IOCTL_IMB_SEND_MESSAGE =%x \n" "IOCTL_IMB_GET_ASYNC_MSG=%x \n"
	"IOCTL_IMB_MAP_MEMORY  = %x \n" "IOCTL_IMB_UNMAP_MEMORY= %x \n"
	"IOCTL_IMB_SHUTDOWN_CODE=%x \n" "IOCTL_IMB_REGISTER_ASYNC_OBJ  =%x \n"
	"IOCTL_IMB_DEREGISTER_ASYNC_OBJ=%x \n"
	"IOCTL_IMB_CHECK_EVENT  =%x \n" "IOCTL_IMB_POLL_ASYNC   =%x \n",
             IOCTL_IMB_SEND_MESSAGE, IOCTL_IMB_GET_ASYNC_MSG,
	IOCTL_IMB_MAP_MEMORY, IOCTL_IMB_UNMAP_MEMORY, IOCTL_IMB_SHUTDOWN_CODE,
	IOCTL_IMB_REGISTER_ASYNC_OBJ, IOCTL_IMB_DEREGISTER_ASYNC_OBJ,
	IOCTL_IMB_CHECK_EVENT , IOCTL_IMB_POLL_ASYNC);  *%%%%*/

		/*O_NDELAY flag will cause problems later when driver makes
		//you wait. Hence removing it. */
		    /*if ((hDevice1 = open(IMB_DEVICE,O_RDWR|O_NDELAY)) <0)  */
		    if ((hDevice1 = open(IMB_DEVICE,O_RDWR)) <0) 
			{
				hDevice1  = 0;
#ifndef WIN32
				/* dont always display open errors if Linux */
				if (fdebug) 
#endif
				{
				  /*debug or not 1st time */
				  printf("imbapi ipmi_open_ia: open(%s) failed, %s\n", 
					 IMB_DEVICE,strerror(errno));
				}
				return (0);
			}	

			if (fskipcmd) {
			   IpmiVersion = IPMI_15_VERSION;
			   return (1);  /*TRUE*/
			}
			/* Detect the IPMI version for processing requests later.
			// This is a crude but most reliable method to differentiate
			// between old IPMI versions and the 1.0 version. If we had used the
			// version field instead then we would have had to revalidate all 
			// the older platforms (pai 4/27/99) */
			requestData.cmdType            = GET_DEVICE_ID;
			requestData.rsSa               = BMC_SA;
			requestData.rsLun              = BMC_LUN;
			requestData.netFn              = APP_NETFN ;
			requestData.busType            = PUBLIC_BUS;
			requestData.data               = NULL; 
			requestData.dataLength	    = 0;
			respLength		    = sizeof(respBuffer);
#ifndef NO_MACRO_ARGS
			DEBUG("%s: opened driver, getting IPMI version\n", __FUNCTION__);
#endif
			ret_code = SendTimedImbpRequest(&requestData,
				 IMB_OPEN_TIMEOUT, respBuffer, &respLength, 
				 &completionCode);
			if ( (ret_code != ACCESN_OK) || (completionCode != 0) )
			{
			   printf("ipmi_open_ia: SendTimedImbpRequest error. Ret = %d CC = 0x%02X\n",
					 ret_code, completionCode);
			   close(hDevice1);
			   hDevice1 = 0;
			   return (0);
			}

			if (respLength < (IPMI10_GET_DEVICE_ID_RESP_LENGTH-1))
				IpmiVersion = IPMI_09_VERSION;
			else {
				if ( respBuffer[4] == 0x01 )
					IpmiVersion = IPMI_10_VERSION;
				else   /* IPMI 1.5 or 2.0 */
					IpmiVersion = IPMI_15_VERSION;
			}
#ifndef NO_MACRO_ARGS
			DEBUG("%s: IPMI version 0x%x\n", __FUNCTION__, IpmiVersion);	
#endif
		
/*
//initialise a mutex
		if(mutex_init(&deviceMutex , USYNC_THREAD, NULL) != 0)
		{
			return(0);
		}
*/

	}
    
	return (1);
}  /*end open_imb()*/

#endif

int close_imb(void)
{
   int rc = 0;
   if (hDevice1 != 0) {
#ifdef WIN32
        CloseHandle(hDevice1);
#else
        rc = close(hDevice1);
#endif
   }
   return(rc);
}

#ifndef LINK_LANDESK
/* If there is not a LANDESK lib, provide these 2 functions for apps 
 * that may be already be coded to that interface.
 */
int initIPMI(void)
{     /* for compatibility with LanDesk programs*/
   int rc;
   rc = open_imb(0);    /*sets hDevice1*/
   if (rc == 1) rc = 0;
   else rc = -1;
   return(rc);
}
int termIPMI(void) 
{     /* for compatibility with LanDesk programs*/
   return(close_imb());
}
#endif

/*---------------------------------------------------------------------*
 * ipmi_open_ia  & ipmi_close_ia 
 *---------------------------------------------------------------------*/
int ipmi_open_ia(char fdebugcmd)
{
   int rc = 0;
   fdebug = fdebugcmd;
   rc = open_imb(0);    /*sets hDevice1*/
   if (rc == 1) rc = 0;
   else rc = -1;
   return(rc);
}

int ipmi_close_ia(void)
{
   int rc = 0;
   rc = close_imb();
   return(rc);
}


int ipmi_cmdraw_ia(BYTE cmd, BYTE netfn, BYTE lun, BYTE sa, BYTE bus, 
                   BYTE *pdata, BYTE sdata, BYTE *presp, int *sresp, 
                   BYTE *pcc, char fdebugcmd)
{
   IMBPREQUESTDATA requestData;
   int status = 0;
   char *imbDev;
   BYTE * pc;
   int sz, i;
#ifndef WIN32
   struct stat     stbuf;
#endif

   if (fdebug) printf("ipmi_cmdraw_ia(%02x,%02x,%02x,%02x,bus=%02x)\n",
			cmd,netfn,lun,sa,bus); 
#ifdef METACOMMAND_IPMB   /*++++ not used*/
   if (bus != PUBLIC_BUS) {
	if (fdebug) printf("ipmi_cmdraw_ia: bus=%x, using ipmb\n",bus);
	status = ipmi_cmd_ipmb(cmd,netfn,lun,sa,bus,pdata,sdata,presp,sresp,
				pcc,fdebugcmd);
	return(status);
   }
#endif
   set_fps();

   requestData.cmdType	= cmd;
   requestData.rsSa	= sa;
   requestData.busType	= bus;
   requestData.netFn	= netfn;
   requestData.rsLun	= lun;
   requestData.dataLength = sdata;
   requestData.data       = pdata; 

   if (fdebugcmd) {
          sz = sizeof(IMBPREQUESTDATA);
          pc = (BYTE *)&requestData.cmdType;
          fprintf(fpdbg,"ipmi_cmdraw_ia: request (len=%d): ",sz);
          for (i = 0; i < sz; i++) fprintf(fpdbg,"%02x ",pc[i]);
          fprintf(fpdbg,"\n");
          pc = requestData.data;
          sz = requestData.dataLength;
          fprintf(fpdbg,"  req.data=%p, dlen=%d: ", pc, sz);
          for (i = 0; i < sz; i++) fprintf(fpdbg,"%02x ",pc[i]);
          fprintf(fpdbg,"\n");
        }

#ifdef WIN32
   imbDev = "[imbdrv]";
   if (1) 
#elif HPUX
   imbDev = "/dev/ipmi";
#else
   imbDev = "/dev/imb";
   if (stat(imbDev, &stbuf) == -1)  {
	   fprintf(fperr,"ipmi_cmdraw_ia: No IMB driver found (%s)\n",imbDev);
	   return(ERR_NO_DRV);
   } else    /* imb device node is there */
#endif
   {
	   sz = *sresp;  /* note that sresp must be pre-set */
	   memset(presp, 0, sz);
           for ( i =0 ; i < IMB_MAX_RETRIES; i++) 
	   {
		*sresp = sz;   /* retries may need to re-init *sresp */
#ifdef LOCAL_IPMB
		if (bus != PUBLIC_BUS)
		   status = SendTimedIpmbpRequest(&requestData, ipmi_timeout_ia, presp, sresp, pcc);
		else 
#endif
		   status = SendTimedImbpRequest(&requestData, ipmi_timeout_ia, presp, sresp, pcc);
		if((status) == 0 ) { break; }
		if (fdebugcmd)   // only gets here if error
	          fprintf(fpdbg,"ipmi_cmdraw_ia: sendImbRequest error status=%x, ccode=%x\n",
                            (uint)status, *pcc);
	   } 
   }

    if (fdebugcmd) {  /* if debug, show both good and bad statuses */
	    fprintf(fpdbg,"ipmi_cmdraw_ia: sendImbRequest status=%x, ccode=%x\n", 
                      (uint)status, *pcc);
            if (status == 0) {  
               BYTE * pc; int sz;
               sz = *sresp;
               pc = (BYTE *)presp;
               fprintf(fpdbg,"ipmi_cmdraw_ia: response (len=%d): ",sz);
               for (i = 0; i < sz; i++) fprintf(fpdbg,"%02x ",pc[i]);
               fprintf(fpdbg,"\n");
            }
        }
    if (status == 1) status = -3;  /*LAN_ERR_RECV_FAIL, a meaningful error*/
    return(status);
} /* end ipmi_cmdraw_ia() */

/*---------------------------------------------------------------------*/
/*Used only by UW. Left here for now. IMB driver will not accept this
//ioctl. */
ACCESN_STATUS
StartAsyncMesgPoll()
{

	DWORD   retLength;
	BOOL    status;
   
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl cmd = %x\n",__FUNCTION__,IOCTL_IMB_POLL_ASYNC);
#endif
	status = DeviceIoControl (      hDevice,
								IOCTL_IMB_POLL_ASYNC,
								NULL,
								0,
								NULL,
								0,
								& retLength,
								0
							);
 
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif

	if( status == TRUE ) {
		return ACCESN_OK;
	} else {
		return ACCESN_ERROR;
	}

}

/*/////////////////////////////////////////////////////////////////////////////
// SendTimedI2cRequest
///////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       SendTimedI2cRequest 
//  Purpose:    This function sends a request to a I2C device
//  Context:    Used by Upper level agents (sis modules) to access dumb I2c devices 
//  Returns:    ACCESN_OK  else error status code
//  Parameters: 
//     reqPtr
//     timeOut
//     respDataPtr
//     respLen
//  Notes:      none
*F*/

ACCESN_STATUS
SendTimedI2cRequest (
		I2CREQUESTDATA 	*reqPtr,         /* I2C request */
		int     timeOut,         /* how long to wait, mSec units */
		BYTE 	*respDataPtr,    /* where to put response data */
		int 	*respDataLen,    /* size of response buffer and */
					 /* size of returned data */
		BYTE 	*completionCode  /* request status from BMC */
	)
{
	BOOL  					status;
    BYTE                  	responseData[MAX_IMB_RESP_SIZE];
	ImbResponseBuffer *     resp = (ImbResponseBuffer *) responseData;
	DWORD                   respLength = sizeof( responseData );
    BYTE                    requestData[MAX_IMB_RESP_SIZE];
	ImbRequestBuffer *      req = (ImbRequestBuffer *) requestData;

	struct WriteReadI2C {   /* format of a write/read I2C request */
		BYTE    busType;
		BYTE    rsSa;
		BYTE    count;
		BYTE    data[1];
	} * wrReq = (struct WriteReadI2C *) req->req.data;

#define MIN_WRI2C_SIZE  3       /* size of write/read request minus any data */


	/*
	// If the Imb driver is not present return AccessFailed
	*/

	req->req.rsSa           = BMC_SA;
	req->req.cmd            = WRITE_READ_I2C;
	req->req.netFn          = APP_NETFN;
	req->req.rsLun          = BMC_LUN;
	req->req.dataLength     = reqPtr->dataLength + MIN_WRI2C_SIZE;

	wrReq->busType          = reqPtr->busType;
	wrReq->rsSa                     = reqPtr->rsSa;
	wrReq->count            = reqPtr->numberOfBytesToRead;
	
	memcpy( wrReq->data, reqPtr->data, reqPtr->dataLength );

	req->flags      = 0;
	req->timeOut    = timeOut * 1000;       /* convert to uSec units */

	status = DeviceIoControl(       hDevice,
					ioctl_sendmsg,
					requestData,
					sizeof( requestData ),
					& responseData,
					sizeof( responseData ),
					& respLength,
					NULL
				  );
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif

	if( status != TRUE ) {
		DWORD error;
		error = GetLastError();
		return ACCESN_ERROR;
	}
	if( respLength == 0 ) {
		return ACCESN_ERROR;
	}

	/*
	// give the caller his response
	*/
	*completionCode = resp->cCode;
	*respDataLen    = respLength - 1;

	if(( *respDataLen ) && (respDataPtr))
		memcpy( respDataPtr, resp->data, *respDataLen);

	return ACCESN_OK;

}

/*This is not a  API exported by the driver in stricter sense. It is 
//added to support EMP functionality. Upper level software could have 
//implemented this function.(pai 5/4/99)  */
/*/////////////////////////////////////////////////////////////////////////////
// SendTimedEmpMessageResponse 
///////////////////////////////////////////////////////////////////////////// */

/*F*
//  Name:       SendTimedEmpMessageResponse 
//  Purpose:    This function sends a response message to the EMP port
//  Context:     
//  Returns:    OK  else error status code
//  Parameters: 
//  
//  Notes:      none
*F*/

ACCESN_STATUS
SendTimedEmpMessageResponse (
		ImbPacket *ptr,       /* pointer to the original request from EMP */
		char      *responseDataBuf,
		int       responseDataLen,
		int       timeOut         /* how long to wait, in mSec units */
	)
{
	BOOL                    status;
    BYTE                    responseData[MAX_IMB_RESP_SIZE];
	/*ImbResponseBuffer *     resp = (ImbResponseBuffer *) responseData; */
	DWORD                   respLength = sizeof( responseData );
    BYTE                    requestData[MAX_IMB_RESP_SIZE];
	ImbRequestBuffer *      req = (ImbRequestBuffer *) requestData;
	int 					i,j;

	/*form the response packet first */
	req->req.rsSa           =  BMC_SA;
	if (IpmiVersion ==	IPMI_09_VERSION)
	req->req.cmd            =  WRITE_EMP_BUFFER;
	else 
	req->req.cmd            =  SEND_MESSAGE;	
	req->req.netFn          =  APP_NETFN;
	req->req.rsLun          =  0;

	i = 0;
	if (IpmiVersion !=	IPMI_09_VERSION)
		req->req.data[i++] 	= EMP_CHANNEL;
		
	req->req.data[i++]    =  ptr->rqSa;
	req->req.data[i++]      =  (((ptr->nfLn & 0xfc) | 0x4) | ((ptr->seqLn) & 0x3));
	if (IpmiVersion ==	IPMI_09_VERSION)
		req->req.data[i++]    = ((~(req->req.data[0] +  req->req.data[1])) +1);
	else
		req->req.data[i++]    = ((~(req->req.data[1] +  req->req.data[2])) +1);

	req->req.data[i++]      =  BMC_SA; /*though software is responding, we have to
					   //provide BMCs slave address as responder
					   //address.  */
	
	req->req.data[i++]      = ( (ptr->seqLn & 0xfc) | (ptr->nfLn & 0x3) );

	req->req.data[i++]      = ptr->cmd;
	for ( j = 0 ; j < responseDataLen ; ++j,++i)
	   req->req.data[i] = responseDataBuf[j];

	 req->req.data[i] = 0;
	 if (IpmiVersion ==	IPMI_09_VERSION)
		 j = 0;
	 else 
		 j = 1;
	for ( ; j < ( i -3); ++j)
		 req->req.data[i] += req->req.data[j+3];
	req->req.data[i]  = ~(req->req.data[i]) +1;
	++i;
	req->req.dataLength     = (BYTE)i;

	req->flags      = 0;
	req->timeOut    = timeOut * 1000;       /* convert to uSec units */


	status = DeviceIoControl(       hDevice,
					ioctl_sendmsg,
					requestData,
					sizeof(requestData),
					responseData,
					sizeof( responseData ),
					& respLength,
					NULL
				  );

#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif


	if ( (status != TRUE) || (respLength != 1) || (responseData[0] != 0) )
	{
		return ACCESN_ERROR;
	}
	return ACCESN_OK;
}


/*This is not a  API exported by the driver in stricter sense. It is added to support
// EMP functionality. Upper level software could have implemented this function.(pai 5/4/99) */
/*///////////////////////////////////////////////////////////////////////////
// SendTimedEmpMessageResponse_Ex
//////////////////////////////////////////////////////////////////////////// */

/*F*
//  Name:       SendTimedEmpMessageResponse_Ex 
//  Purpose:    This function sends a response message to the EMP port
//  Context:     
//  Returns:    OK  else error status code
//  Parameters: 
//  
//  Notes:      none
*F*/

ACCESN_STATUS
SendTimedEmpMessageResponse_Ex (

		ImbPacket *      ptr,       /* pointer to the original request from EMP */
		char      *responseDataBuf,
		int        responseDataLen,
		int         timeOut,        /* how long to wait, in mSec units*/
		BYTE		sessionHandle,	/*This is introduced in IPMI1.5,this is required to be sent in 
			//send message command as a parameter,which is then used by BMC
			//to identify the correct DPC session to send the mesage to. */
		BYTE		channelNumber	/*There are 3 different channels on which DPC communication goes on
			//Emp - 1,Lan channel one - 6,Lan channel two(primary channel) - 7. */
	)
{
	BOOL                    status;
    BYTE                    responseData[MAX_IMB_RESP_SIZE];
	/* ImbResponseBuffer *  resp = (ImbResponseBuffer *) responseData; */
	DWORD                   respLength = sizeof( responseData );
    BYTE                    requestData[MAX_IMB_RESP_SIZE];
	ImbRequestBuffer *      req = (ImbRequestBuffer *) requestData;
	int 					i,j;

	/*form the response packet first */
	req->req.rsSa           =  BMC_SA;
	if (IpmiVersion ==	IPMI_09_VERSION)
	req->req.cmd            =  WRITE_EMP_BUFFER;
	else 
	req->req.cmd            =  SEND_MESSAGE;	
	req->req.netFn          =  APP_NETFN;
	req->req.rsLun          =  0;

	i = 0;

	/*checking for the IPMI version & then assigning the channel number for EMP
	//Actually the channel number is same in both the versions.This is just to 
	//maintain the consistancy with the same method for LAN.
	//This is the 1st byte of the SEND MESSAGE command. */
	if (IpmiVersion ==	IPMI_10_VERSION)
		req->req.data[i++] 	= EMP_CHANNEL;
	else if (IpmiVersion ==	IPMI_15_VERSION)
		req->req.data[i++] 	= channelNumber;

	/*The second byte of data for SEND MESSAGE starts with session handle */
	req->req.data[i++] = sessionHandle;
		
	/*Then it is the response slave address for SEND MESSAGE. */
	req->req.data[i++]    =  ptr->rqSa;

	/*Then the net function + lun for SEND MESSAGE command. */
	req->req.data[i++]      =  (((ptr->nfLn & 0xfc) | 0x4) | ((ptr->seqLn) & 0x3));

	/*Here the checksum is calculated.The checksum calculation starts after the channel number.
	//so for the IPMI 1.5 version its a checksum of 3 bytes that is session handle,response slave 
	//address & netfun+lun. */
	if (IpmiVersion ==	IPMI_09_VERSION)
		req->req.data[i++]    = ((~(req->req.data[0] +  req->req.data[1])) +1);
	else
	{
		if (IpmiVersion == IPMI_10_VERSION)
			req->req.data[i++]    = ((~(req->req.data[1] +  req->req.data[2])) +1);
        else
			req->req.data[i++]    = ((~(req->req.data[2]+  req->req.data[3])) +1);
	}

	/*This is the next byte of the message data for SEND MESSAGE command.It is the request 
	//slave address. */
	req->req.data[i++]      =  BMC_SA; /*though software is responding, we have to
					   //provide BMCs slave address as responder
					   //address. */
	
	/*This is just the sequence number,which is the next byte of data for SEND MESSAGE */
	req->req.data[i++]      = ( (ptr->seqLn & 0xfc) | (ptr->nfLn & 0x3) );

	/*The next byte is the command like get software ID(00).*/
	req->req.data[i++]      = ptr->cmd;

	/*after the cmd the data ,which is sent by DPC & is retrived using the get message earlier
	// is sent back to DPC. */
	for ( j = 0 ; j < responseDataLen ; ++j,++i)
	   req->req.data[i] = responseDataBuf[j];

	 req->req.data[i] = 0;

	 /*The last byte of data for SEND MESSAGE command is the check sum ,which is calculated
	 //from the next byte of the previous checksum that is the request slave address. */
	 if (IpmiVersion ==	IPMI_09_VERSION)
		 j = 0;
	 else 
	 {	
		if (IpmiVersion ==	IPMI_10_VERSION)
			j = 1;
		else
			j = 2;
	 }
	for ( ; j < ( i -3); ++j)
		 req->req.data[i] += req->req.data[j+3];
	req->req.data[i]  = ~(req->req.data[i]) +1;
	++i;
	req->req.dataLength     = (BYTE)i;

	/*The flags & timeouts are used by the driver internally. */
	req->flags      = 0;
	req->timeOut    = timeOut * 1000;       /* convert to uSec units */

	status = DeviceIoControl(       hDevice,
					ioctl_sendmsg,
					requestData,
					sizeof(requestData),
					responseData,
					sizeof( responseData ),
					& respLength,
					NULL
				  );


#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif

	if (fdebug) {
		   printf("SendTimedEmp(%x,%x): status=%d cc=%x rlen=%lu i=%d\n",
				sessionHandle, channelNumber,
				status,responseData[0],respLength,i);
		   _dump_buf("requestData",requestData,sizeof(requestData),0);
	}
	if ( (status != TRUE) || (respLength != 1) || (responseData[0] != 0) )
	{
		return ACCESN_ERROR;
	}
	return ACCESN_OK;


}

/*This is not a  API exported by the driver in stricter sense. It is 
//added to support EMP functionality. Upper level software could have 
//implemented this function.(pai 5/4/99) */
/*///////////////////////////////////////////////////////////////////////////
// SendTimedLanMessageResponse
///////////////////////////////////////////////////////////////////////////// */

/*F*
//  Name:       SendTimedLanMessageResponse
//  Purpose:    This function sends a response message to the DPC Over Lan
//  Context:     
//  Returns:    OK  else error status code
//  Parameters: 
//  
//  Notes:      none
*F*/

ACCESN_STATUS
SendTimedLanMessageResponse(
		ImbPacket *ptr,       /* pointer to the original request from EMP */
		char      *responseDataBuf,
		int       responseDataLen,
		int       timeOut         /* how long to wait, in mSec units */
	)
{
	BOOL                    status;
    BYTE                    responseData[MAX_IMB_RESP_SIZE];
	/* ImbResponseBuffer *     resp = (ImbResponseBuffer *) responseData; */
	DWORD                   respLength = sizeof( responseData );
    BYTE                    requestData[MAX_IMB_RESP_SIZE];
	ImbRequestBuffer *      req = (ImbRequestBuffer *) requestData;
	int 					i,j;

	/*form the response packet first */
	req->req.rsSa           =  BMC_SA;
	if (IpmiVersion ==	IPMI_09_VERSION)
	req->req.cmd            =  WRITE_EMP_BUFFER;
	else 
	req->req.cmd            =  SEND_MESSAGE;	
	req->req.netFn          =  APP_NETFN;

	/* After discussion with firmware team (Shailendra), the lun number needs to stay at 0
	// even though the DPC over Lan firmware EPS states that the lun should be 1 for DPC
	// Over Lan. - Simont (5/17/00) */
	req->req.rsLun          =  0;

	i = 0;
	if (IpmiVersion !=	IPMI_09_VERSION)
		req->req.data[i++] 	= LAN_CHANNEL;
		
	req->req.data[i++]    =  ptr->rqSa;
	req->req.data[i++]      =  (((ptr->nfLn & 0xfc) | 0x4) | ((ptr->seqLn) & 0x3));
	if (IpmiVersion ==	IPMI_09_VERSION)
		req->req.data[i++]    = ((~(req->req.data[0] +  req->req.data[1])) +1);
	else
		req->req.data[i++]    = ((~(req->req.data[1] +  req->req.data[2])) +1);

	req->req.data[i++]      =  BMC_SA; /*though software is responding, we have to
					   //provide BMCs slave address as responder
					   //address. */
	
	req->req.data[i++]      = ( (ptr->seqLn & 0xfc) | (ptr->nfLn & 0x3) );

	req->req.data[i++]      = ptr->cmd;
	for ( j = 0 ; j < responseDataLen ; ++j,++i)
	   req->req.data[i] = responseDataBuf[j];

	 req->req.data[i] = 0;
	 if (IpmiVersion ==	IPMI_09_VERSION)
		 j = 0;
	 else 
		 j = 1;
	for ( ; j < ( i -3); ++j)
		 req->req.data[i] += req->req.data[j+3];
	req->req.data[i]  = ~(req->req.data[i]) +1;
	++i;
	req->req.dataLength     = (BYTE)i;

	req->flags      = 0;
	req->timeOut    = timeOut * 1000;       /* convert to uSec units */

	status = DeviceIoControl(       hDevice,
					ioctl_sendmsg,
					requestData,
					sizeof(requestData),
					responseData,
					sizeof( responseData ),
					& respLength,
					NULL
				  );

	if (fdebug) {
		   printf("SendTimedLan(): status=%d cc=%x rlen=%lu i=%d\n",
				status, responseData[0],respLength,i);
		   _dump_buf("requestData",requestData,sizeof(requestData),0);
	}
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif

	if ( (status != TRUE) || (respLength != 1) || (responseData[0] != 0) )
	{
		return ACCESN_ERROR;
	}
	return ACCESN_OK;
}


/*This is not a  API exported by the driver in stricter sense. It is 
//added to support EMP functionality. Upper level software could have 
//implemented this function.(pai 5/4/99) */
/*///////////////////////////////////////////////////////////////////////////
// SendTimedLanMessageResponse_Ex
///////////////////////////////////////////////////////////////////////////// */

/*F*
//  Name:       SendTimedLanMessageResponse_Ex
//  Purpose:    This function sends a response message to the DPC Over Lan
//  Context:     
//  Returns:    OK  else error status code
//  Parameters: 
//  
//  Notes:      none
*F*/

ACCESN_STATUS
SendTimedLanMessageResponse_Ex(
		ImbPacket *ptr,       /* pointer to the original request from EMP */
		char      *responseDataBuf,
		int       responseDataLen,
		int       timeOut  ,		/* how long to wait, in mSec units */
		BYTE		sessionHandle,	/*This is introduced in IPMI1.5,this is required to be sent in 
			//send message command as a parameter,which is then used by BMC
			//to identify the correct DPC session to send the mesage to. */
		BYTE		channelNumber	/*There are 3 different channels on which DPC communication goes on
			//Emp - 1,Lan channel one - 6,Lan channel two(primary channel) - 7. */
	)
{
	BOOL                    status;
    BYTE                    responseData[MAX_IMB_RESP_SIZE];
	/* ImbResponseBuffer *     resp = (ImbResponseBuffer *) responseData; */
	DWORD                   respLength = sizeof( responseData );
    BYTE                    requestData[MAX_IMB_RESP_SIZE];
	ImbRequestBuffer *      req = (ImbRequestBuffer *) requestData;
	int 					i,j;

	/*form the response packet first */
	req->req.rsSa           =  BMC_SA;
	if (IpmiVersion ==	IPMI_09_VERSION)
	req->req.cmd            =  WRITE_EMP_BUFFER;
	else 
	req->req.cmd            =  SEND_MESSAGE;	
	req->req.netFn          =  APP_NETFN;

	/* After discussion with firmware team (Shailendra), the lun number needs to stay at 0
	// even though the DPC over Lan firmware EPS states that the lun should be 1 for DPC
	// Over Lan. - Simont (5/17/00) */
	req->req.rsLun          =  0;

	i = 0;

	/*checking for the IPMI version & then assigning the channel number for Lan accordingly.
	//This is the 1st byte of the SEND MESSAGE command. */
	if (IpmiVersion ==	IPMI_10_VERSION)
		req->req.data[i++] 	= LAN_CHANNEL;
	else if (IpmiVersion ==	IPMI_15_VERSION)
		req->req.data[i++] 	= channelNumber;

	/*The second byte of data for SEND MESSAGE starts with session handle */
	req->req.data[i++] = sessionHandle;
	
	/*Then it is the response slave address for SEND MESSAGE. */
	req->req.data[i++]    =  ptr->rqSa;

	/*Then the net function + lun for SEND MESSAGE command. */
	req->req.data[i++]      =  (((ptr->nfLn & 0xfc) | 0x4) | ((ptr->seqLn) & 0x3));

	/*Here the checksum is calculated.The checksum calculation starts after the channel number.
	//so for the IPMI 1.5 version its a checksum of 3 bytes that is session handle,response slave 
	//address & netfun+lun. */
	if (IpmiVersion ==	IPMI_09_VERSION)
		req->req.data[i++]    = ((~(req->req.data[0] +  req->req.data[1])) +1);
	else
	{
		if (IpmiVersion == IPMI_10_VERSION)
			req->req.data[i++]    = ((~(req->req.data[1] +  req->req.data[2])) +1);
        else
			req->req.data[i++]    = ((~(req->req.data[2]+  req->req.data[3])) +1);
	}
	
	/*This is the next byte of the message data for SEND MESSAGE command.It is the request 
	//slave address. */
	req->req.data[i++]      =  BMC_SA; /*though software is responding, we have to
					   //provide BMC's slave address as responder
					   //address. */
	
	/*This is just the sequence number,which is the next byte of data for SEND MESSAGE */
	req->req.data[i++]      = ( (ptr->seqLn & 0xfc) | (ptr->nfLn & 0x3) );

	/*The next byte is the command like get software ID(00). */
	req->req.data[i++]      = ptr->cmd;

	/*after the cmd the data ,which is sent by DPC & is retrived using the get message earlier
	// is sent back to DPC. */
	for ( j = 0 ; j < responseDataLen ; ++j,++i)
	   req->req.data[i] = responseDataBuf[j];

	 req->req.data[i] = 0;

	 /*The last byte of data for SEND MESSAGE command is the check sum ,which is calculated
	 //from the next byte of the previous checksum that is the request slave address. */
	 if (IpmiVersion ==	IPMI_09_VERSION)
		 j = 0;
	 else 
	{	
		if (IpmiVersion ==	IPMI_10_VERSION)
			j = 1;
		else
			j = 2;
	 }	
	 for ( ; j < ( i -3); ++j)
		req->req.data[i] += req->req.data[j+3];
	req->req.data[i]  = ~(req->req.data[i]) +1;
	++i;
	req->req.dataLength     = (BYTE)i;

	/*The flags & timeouts are used by the driver internally */
	req->flags      = 0;
	req->timeOut    = timeOut * 1000;       /* convert to uSec units */

	status = DeviceIoControl(       hDevice,
					ioctl_sendmsg,
					requestData,
					sizeof(requestData),
					responseData,
					sizeof( responseData ),
					& respLength,
					NULL
				  );
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif
	if (fdebug) {
	   printf("SendTimedLan(%x,%x): status=%d cc=%x rlen=%lu i=%d\n",
				sessionHandle, channelNumber,
				status,responseData[0],respLength,i);
	   if (responseData[0] != 0)  /*0xcc == invalid request data*/
	   {
	      BYTE *pb;
	      pb = (BYTE *)req->req.data;
	      printf("SendMessage data: %02x %02x %02x %02x %02x %02x %02x %02x\n",
		 pb[0], pb[1], pb[2], pb[3], pb[4], pb[5], pb[6], pb[7]);
	      _dump_buf("requestData",requestData,sizeof(requestData),0);
	   }
	}

	if ( (status != TRUE) || (respLength != 1) || (responseData[0] != 0) )
	{
		return ACCESN_ERROR;
	}
	return ACCESN_OK;
}



/*/////////////////////////////////////////////////////////////////////////
//SendAsyncImbpRequest 
/////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       SendAsyncImbpRequest
//  Purpose:    This function sends a request for Asynchronous IMB implemented function
//  Context:    Used by Upper level agents (sis modules) to access Asynchronous IMB implemented functionality. 
//  Returns:    OK  else error status code
//  Parameters: 
//     reqPtr  Pointer to Async IMB request
//     seqNo   Sequence Munber
//  Notes:      none
*F*/
ACCESN_STATUS
SendAsyncImbpRequest (
		IMBPREQUESTDATA *reqPtr,  /* request info and data */
		BYTE *          seqNo     /* sequence number used in creating IMB msg */
	)
{

	BOOL                    status;
    BYTE                    responseData[MAX_IMB_RESP_SIZE];
	ImbResponseBuffer *     resp = (ImbResponseBuffer *) responseData;
	DWORD                   respLength = sizeof( responseData );
    BYTE                    requestData[MAX_IMB_RESP_SIZE];
	ImbRequestBuffer *      req = (ImbRequestBuffer *) requestData;

	req->req.rsSa           = reqPtr->rsSa;
	req->req.cmd            = reqPtr->cmdType;
	req->req.netFn          = reqPtr->netFn;
	req->req.rsLun          = reqPtr->rsLun;
	req->req.dataLength     = (BYTE)reqPtr->dataLength;

	memcpy( req->req.data, reqPtr->data, reqPtr->dataLength );

	req->flags              = NO_RESPONSE_EXPECTED;
	req->timeOut    = 0;    /* no timeouts for async sends */

	status = DeviceIoControl(       hDevice,
					ioctl_sendmsg,
					requestData,
					sizeof( requestData ),
					& responseData,
					sizeof( responseData ),
					& respLength,
					NULL
				  );
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif

	if( status != TRUE ) {
		DWORD error;
		error = GetLastError();
		return ACCESN_ERROR;
	}
	if( respLength != 2 ) {
		return ACCESN_ERROR;
	}
	/*
	// give the caller his sequence number
	*/
	*seqNo = resp->data[0];

	return ACCESN_OK;

}

/*///////////////////////////////////////////////////////////////////////////
//GetAsyncImbpMessage
///////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       GetAsyncImbpMessage
//  Purpose:    This function gets the next available async message with a message id
//                              greater than SeqNo. The message looks like an IMB packet
//                              and the length and Sequence number is returned
//  Context:    Used by Upper level agents (sis modules) to access Asynchronous IMB implemented functionality. 
//  Returns:    OK  else error status code
//  Parameters: 
//     msgPtr  Pointer to Async IMB request
//     msgLen  Length 
//     timeOut Time to wait 
//     seqNo   Sequence Munber
//  Notes:      none
*F*/

ACCESN_STATUS
GetAsyncImbpMessage (
		ImbPacket *     msgPtr,         /* request info and data */
		DWORD 		*msgLen,        /* IN - length of buffer, OUT - msg len */
		DWORD		timeOut,        /* how long to wait for the message */
		ImbAsyncSeq 	*seqNo,         /* previously returned seq number */
										/* (or ASYNC_SEQ_START) */
		DWORD		channelNumber
	)
{

	BOOL                   status;
        BYTE                   responseData[MAX_ASYNC_RESP_SIZE], lun, b;
	ImbAsyncResponse *     resp = (ImbAsyncResponse *) responseData;
	DWORD                  respLength = sizeof( responseData );
	ImbAsyncRequest        req;
	BYTE   *msgb;

	while(1)
	{


		if( (msgPtr == NULL) || (msgLen == NULL) || ( seqNo == NULL) )
				return ACCESN_ERROR;

		msgb = (BYTE *)msgPtr;
				req.timeOut   = timeOut * 1000;       /* convert to uSec units */
				req.lastSeq   = *seqNo;


			status = DeviceIoControl(       hDevice,
						IOCTL_IMB_GET_ASYNC_MSG,
						& req,
						sizeof( req ),
						& responseData,
						sizeof( responseData ),
						& respLength,
						NULL
					  );

#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif

			if( status != TRUE ) {
				DWORD error = GetLastError();
				/*
				// handle "msg not available" specially. it is 
				// different from a random old error.
				*/
				switch( error ) {
					case IMB_MSG_NOT_AVAILABLE:
												return ACCESN_END_OF_DATA;
					default:
											return ACCESN_ERROR;
				}
				return ACCESN_ERROR;
			}
			if( respLength < MIN_ASYNC_RESP_SIZE ) {
					return ACCESN_ERROR;
			}
			respLength -= MIN_ASYNC_RESP_SIZE;

			if( *msgLen < respLength ) {
					return ACCESN_ERROR;
			}


			/*same code as in NT section */
			if ( IpmiVersion == IPMI_09_VERSION)
			{

				switch( channelNumber) {
					case IPMB_CHANNEL:
								lun = IPMB_LUN;
								 break;

					case  EMP_CHANNEL:
								lun = EMP_LUN;
							  	break;

					default:
								lun = RESERVED_LUN;
								break;
				}

				b = (((ImbPacket *)(resp->data))->nfLn) & 0x3;
				if (channelNumber != ANY_CHANNEL) {
				  if ( (lun == RESERVED_LUN) || 
				       (lun != b) )
				  {
						*seqNo = resp->thisSeq;
						continue;
				  } 
				}

				memcpy( msgPtr, resp->data, respLength );
				*msgLen = respLength;
				/* Hack to return actual lun */
				if (channelNumber == ANY_CHANNEL) 
				   msgb[respLength] = b;
			}	
			else 
			{
				/* it is a 1.0 or  above version 	 */

				b = resp->data[0]; 
				if ((channelNumber != ANY_CHANNEL) &&
				    (channelNumber != b))
				{
					*seqNo = resp->thisSeq;
					continue;
				}

				memcpy(msgPtr, &(resp->data[1]), respLength-1);
				*msgLen = respLength-1;
				/* Hack to return actual channel */
				if (channelNumber == ANY_CHANNEL) 
				   msgb[respLength-1] = b;

			}
	
		/*
		// give the caller his sequence number
		*/
		*seqNo = resp->thisSeq;

		return ACCESN_OK;

	} /*while (1)  */
}

  
/*///////////////////////////////////////////////////////////////////////////
//GetAsyncImbpMessage_Ex
///////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       GetAsyncImbpMessage_Ex
//  Purpose:    This function gets the next available async message with a message id
//                              greater than SeqNo. The message looks like an IMB packet
//                              and the length and Sequence number is returned
//  Context:    Used by Upper level agents (sis modules) to access Asynchronous IMB implemented functionality. 
//  Returns:    OK  else error status code
//  Parameters: 
//     msgPtr  Pointer to Async IMB request
//     msgLen  Length 
//     timeOut Time to wait 
//     seqNo   Sequence Munber
//  Notes:      none
*F*/

ACCESN_STATUS
GetAsyncImbpMessage_Ex (
		ImbPacket *     msgPtr,         /* request info and data */
		DWORD 			*msgLen,        /* IN - length of buffer, OUT - msg len */
		DWORD			timeOut,        /* how long to wait for the message */
		ImbAsyncSeq 	*seqNo,         /* previously returned seq number */
										/* (or ASYNC_SEQ_START) */
		DWORD			channelNumber,
		BYTE *					sessionHandle,
		BYTE *					privilege
	)
{

	BOOL                   status;
    	BYTE                   responseData[MAX_ASYNC_RESP_SIZE], lun, b;
	ImbAsyncResponse *     resp = (ImbAsyncResponse *) responseData;
	DWORD                  respLength = sizeof( responseData );
	ImbAsyncRequest        req;
	BYTE  *msgb;

	while(1)
	{


		if( (msgPtr == NULL) || (msgLen == NULL) || ( seqNo == NULL) )
				return ACCESN_ERROR;

		msgb = (BYTE *)msgPtr;
				req.timeOut   = timeOut * 1000;       /* convert to uSec units */
				req.lastSeq   = *seqNo;


			status = DeviceIoControl(       hDevice,
							IOCTL_IMB_GET_ASYNC_MSG,
							& req,
							sizeof( req ),
							& responseData,
							sizeof( responseData ),
							& respLength,
							NULL
						  );

#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif

			if( status != TRUE ) {
				DWORD error = GetLastError();
				/*
				// handle "msg not available" specially. it is 
				// different from a random old error.
				*/
				switch( error ) {
					case IMB_MSG_NOT_AVAILABLE:
												return ACCESN_END_OF_DATA;
					default:
											return ACCESN_ERROR;
				}
				return ACCESN_ERROR;
			}
			if( respLength < MIN_ASYNC_RESP_SIZE ) {
					return ACCESN_ERROR;
			}
			respLength -= MIN_ASYNC_RESP_SIZE;

			if( *msgLen < respLength ) {
					return ACCESN_ERROR;
			}


			/*same code as in NT section */
			if ( IpmiVersion == IPMI_09_VERSION)
			{

				switch( channelNumber) {
					case IPMB_CHANNEL:
								lun = IPMB_LUN;
								 break;

					case  EMP_CHANNEL:
								lun = EMP_LUN;
							  	break;

					default:
								lun = RESERVED_LUN;
								break;
				}

				b = ((((ImbPacket *)(resp->data))->nfLn) & 0x3);
				if (channelNumber != ANY_CHANNEL) {
				  if ( (lun == RESERVED_LUN) || 
				       (lun != b) )
				  {
						*seqNo = resp->thisSeq;
						continue;
				  }
				}

				memcpy( msgPtr, resp->data, respLength );
				*msgLen = respLength;
				/* Hack to return actual lun */
				if (channelNumber == ANY_CHANNEL) 
				   msgb[respLength] = b;
				
			}	
			else 
			{
				if((sessionHandle ==NULL) || (privilege ==NULL))
					return ACCESN_ERROR;

				/*With the new IPMI version the get message command returns the 
				//channel number along with the privileges.The 1st 4 bits of the
				//second byte of the response data for get message command represent
				//the channel number & the last 4 bits are the privileges. */
				*privilege = (resp->data[0] & 0xf0)>> 4;

#ifndef NO_MACRO_ARGS
				DEBUG("GetAsy: chan=%x chan_parm=%x\n",resp->data[0],channelNumber);
#endif
				b = (resp->data[0] & 0x0f);
				if ((channelNumber != ANY_CHANNEL) && 
				    (channelNumber != b) )
				{
					*seqNo = resp->thisSeq;
					continue;
				}
				
				
				/*The get message command according to IPMI 1.5 spec now even
				//returns the session handle.This is required to be captured
				//as it is required as request data for send message command. */
				*sessionHandle = resp->data[1];
				memcpy( msgPtr, &(resp->data[2]), respLength-1 );
				*msgLen = respLength-1;
						
				/* Hack to return actual channel */
				if (channelNumber == ANY_CHANNEL) 
				   msgb[respLength-1] = b;

			}
	
		/*
		// give the caller his sequence number
		*/
		*seqNo = resp->thisSeq;

		return ACCESN_OK;

	} /*while (1) */
}



/*//////////////////////////////////////////////////////////////////////////////
//IsAsyncMessageAvailable
///////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       IsMessageAvailable
//  Purpose:    This function waits for an Async Message  
//
//  Context:    Used by Upper level agents access Asynchronous IMB based
//              messages   
//  Returns:    OK  else error status code
//  Parameters: 
//               eventId
//    
//  Notes:     This call will block the calling thread if no Async events are 
//                              are available in the queue.
//
*F*/
ACCESN_STATUS
IsAsyncMessageAvailable (HandleType   eventId )
{
    int 	dummy;
    int 	respLength = 0;
    BOOL  	status;

 /* confirm that app is not using a bad Id */


	if ( AsyncEventHandle  != eventId) {
#ifdef LINUX
		printf("Invalid AsyncHandle %lx!=%lx\n",AsyncEventHandle,eventId);
#endif
	  	return ACCESN_ERROR;
	}

	dummy = 0;
	status = DeviceIoControl(hDevice,
				   IOCTL_IMB_CHECK_EVENT,
				   &AsyncEventHandle,
				    sizeof(HandleType),
				    &dummy,
					sizeof(int),
					(LPDWORD) & respLength,
					NULL
				  );
	// if (fdebug) 
	//	printf("IsAsyncMessageAvail: status=%d rlen=%d dummy=%x\n",
	//			status, respLength, dummy);
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif

	if( status != TRUE )
		return  ACCESN_ERROR;

	
	return ACCESN_OK;
}


/*I have retained this commented code because later we may want to use 
//DPC message specific Processing (pai 11/21) */

#ifdef NOT_COMPILED_BUT_LEFT_HERE_FOR_NOW

/*//////////////////////////////////////////////////////////////////////////////
//GetAsyncDpcMessage
///////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       GetAsyncDpcMessage
//  Purpose:    This function gets the next available async message from
//                              the DPC client. 
//
//  Context:    Used by Upper level agents access Asynchronous IMB based
//              messages sent by the DPC client.  
//  Returns:    OK  else error status code
//  Parameters: 
//     msgPtr  Pointer to Async IMB request
//         msgLen  Length 
//         timeOut Time to wait 
//     seqNo   Sequence Munber
//  Notes:     This call will block the calling thread if no Async events are 
//                              are available in the queue.
//
*F*/

ACCESN_STATUS
GetAsyncDpcMessage (
		ImbPacket *             msgPtr,         /* request info and data */
		DWORD *                 msgLen,         /* IN - length of buffer, OUT - msg len */
		DWORD                   timeOut,        /* how long to wait for the message */
		ImbAsyncSeq *   seqNo,          /* previously returned seq number (or ASYNC_SEQ_START) */
	)
{
	BOOL                            status;
    BYTE                                responseData[MAX_ASYNC_RESP_SIZE];
	ImbAsyncResponse *      resp = (ImbAsyncResponse *) responseData;
	DWORD                           respLength = sizeof( responseData );
	ImbAsyncRequest         req;

	if( msgPtr == NULL || msgLen == NULL || seqNo == NULL )
		return ACCESN_ERROR;

	req.lastSeq             = *seqNo;


	hEvt = CreateEvent (NULL, TRUE, FALSE, NULL) ;
	if (!hEvt) {
		return ACCESN_ERROR;
	}

	status = DeviceIoControl(       hDevice,
					IOCTL_IMB_GET_DPC_MSG,
					& req,
					sizeof( req ),
					& responseData,
					sizeof( responseData ),
					& respLength,
					&ovl
				  );

	if( status != TRUE ) {
		DWORD error = GetLastError();
		/*
		// handle "msg not available" specially. it is different from
		// a random old error.
		//
		*/
	if (!status)
	{
			switch (error )
				{
						case ERROR_IO_PENDING:

								WaitForSingleObject (hEvt, INFINITE) ;
								ResetEvent (hEvt) ;
								break;

						case IMB_MSG_NOT_AVAILABLE:

							    CloseHandle(hEvt);
								return ACCESN_END_OF_DATA;

						default:
								CloseHandle(hEvt);
								return ACCESN_ERROR;
								
			}
	}



		if ( 
		( GetOverlappedResult(hDevice,   
							&ovl,    
							(LPDWORD)&respLength, 
							TRUE 
					) == 0 ) || (respLength <= 0)
		)

		{

			CloseHandle(hEvt);
			return ACCESN_ERROR;

		}


	}
	
	if( respLength < MIN_ASYNC_RESP_SIZE ) {
		CloseHandle(hEvt);
		return ACCESN_ERROR;
	}

	respLength -= MIN_ASYNC_RESP_SIZE;

	if( *msgLen < respLength ) {

		/* The following code should have been just return ACCESN_out_of_range */
		CloseHandle(hEvt);
		return ACCESN_ERROR;
	}

	memcpy( msgPtr, resp->data, respLength );

	*msgLen = respLength;
	/*
	// give the caller his sequence number
	*/
	*seqNo = resp->thisSeq;

	CloseHandle(hEvt);


	return ACCESN_OK;

}
#endif /*NOT_COMPILED_BUT_LEFT_HERE_FOR_NOW*/



/*/////////////////////////////////////////////////////////////////////////////
//RegisterForImbAsyncMessageNotification
///////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       RegisterForImbAsyncMessageNotification
//  Purpose:    This function Registers the calling application  
//                              for Asynchronous notification when a sms message
//                              is available with the IMB driver.                       
//
//  Context:    Used by Upper level agents to know that an async
//                              SMS message is available with the driver.  
//  Returns:    OK  else error status code
//  Parameters: 
//    handleId  pointer to the registration handle
//
//  Notes:      The calling application should use the returned handle to 
//              get the Async messages..
*F*/
ACCESN_STATUS
RegisterForImbAsyncMessageNotification (HandleType *handleId)

{
	BOOL      status;
	DWORD     respLength ;
	int       dummy;

	/*allow  only one app to register  */

	if( (handleId  == NULL ) || (AsyncEventHandle) )
		return ACCESN_ERROR;


	status = DeviceIoControl(hDevice,
				IOCTL_IMB_REGISTER_ASYNC_OBJ,
				&dummy,
				sizeof( int ),
				&AsyncEventHandle,
				sizeof(HandleType),
				(LPDWORD) & respLength,
				NULL
			  );
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif
	
	if( (respLength != sizeof(int))  || (status != TRUE )) {
		if (fdebug) {
		   printf("RegisterForImbAsync error status=%d, len=%lu sizeint=%lu\n", status, respLength, sizeof(int));
		  if( respLength != sizeof(int)) printf("Async len err\n");
	 	  if( status != TRUE) printf("Async status err\n");
		}
		return  ACCESN_ERROR;
	}

	*handleId = AsyncEventHandle;
	
#ifndef NO_MACRO_ARGS
	DEBUG("handleId = %x AsyncEventHandle %x\n", *handleId, AsyncEventHandle);
#endif
	return ACCESN_OK;
}





/*/////////////////////////////////////////////////////////////////////////////
//UnRegisterForImbAsyncMessageNotification
///////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       UnRegisterForImbAsyncMessageNotification
//  Purpose:    This function un-registers the calling application  
//                              for Asynchronous notification when a sms message
//                              is available with the IMB driver.                       
//
//  Context:    Used by Upper level agents to un-register
//                              for  async. notification of sms messages  
//  Returns:    OK  else error status code
//  Parameters: 
//    handleId  pointer to the registration handle
//	  iFlag		value used to determine where this function was called from
//				_it is used currently on in NetWare environment_
//
//  Notes:      
*F*/
ACCESN_STATUS
UnRegisterForImbAsyncMessageNotification (HandleType handleId, int iFlag)

{
	BOOL		status;
	DWORD		respLength ;
	int			dummy;

	iFlag = iFlag;	/* to keep compiler happy  We are not using this flag*/

	if ( AsyncEventHandle  != handleId)
	  return ACCESN_ERROR;

	status = DeviceIoControl(hDevice,
					IOCTL_IMB_DEREGISTER_ASYNC_OBJ,
					&AsyncEventHandle,
					sizeof(HandleType),
					&dummy,
					(DWORD)sizeof(int ),
					(LPDWORD) & respLength,
					NULL
				  );
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif

	if( status != TRUE )
		return  ACCESN_ERROR;

	return ACCESN_OK;
}


/*///////////////////////////////////////////////////////////////////////////
// SetShutDownCode
///////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       SetShutDownCode
//  Purpose:    To set the shutdown  action code
//  Context:    Called by the System Control Subsystem
//  Returns:    none
//  Parameters: 
//              code  shutdown action code which can be either
//              SD_NO_ACTION, SD_RESET, SD_POWER_OFF as defined in imb_if.h
*F*/
		
ACCESN_STATUS 
SetShutDownCode (
		int 	delayTime,	 /* time to delay in 100ms units */
		int 	code             /* what to do when time expires */
	)
{   
	DWORD					retLength;
	BOOL                    status;
	ShutdownCmdBuffer       cmd;

	/*
	// If Imb driver is not present return AccessFailed
	*/
	if(hDevice == INVALID_HANDLE_VALUE)
		return ACCESN_ERROR;

	cmd.code        = code;
	cmd.delayTime   = delayTime;

	status = DeviceIoControl( hDevice,
					IOCTL_IMB_SHUTDOWN_CODE,
					& cmd,
					sizeof( cmd ),
					NULL,
					0,
					& retLength,
					NULL
				  );
#ifndef NO_MACRO_ARGS
	DEBUG("%s: DeviceIoControl status = %d\n",__FUNCTION__, status);
#endif

	if(status == TRUE)
		return ACCESN_OK;
	else
		return ACCESN_ERROR;
}

#ifdef IMB_MEMORY
/*/////////////////////////////////////////////////////////////////////////
// MapPhysicalMemory 
/////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       MapPhysicalMemory 
//  Purpose:    This function maps specified range of physical memory in calling
//              pocesse's address space
//  Context:    Used by Upper level agents (sis modules) to access 
//				system physical memory 
//  Returns:    ACCESN_OK  else error status code
//  Parameters: 
//     
//     startAddress   starting physical address of the  memory to be mapped 
//     addressLength  length of the physical memory to be mapped
//     virtualAddress pointer to the mapped virtual address
//  Notes:      none
*F*/
/*///////////////////////////////////////////////////////////////////////////
// UnmapPhysicalMemory 
//////////////////////////////////////////////////////////////////////////// */
/*F*
//  Name:       UnMapPhysicalMemory 
//  Purpose:    This function unmaps the previously mapped physical memory
//  Context:    Used by Upper level agents (sis modules)  
//  Returns:    ACCESN_OK  else error status code
//  Parameters: 
//     
//     addressLength  length of the physical memory to be mapped
//     virtualAddress pointer to the mapped virtual address
//  Notes:      none
*F*/
#ifdef WIN32
ACCESN_STATUS
MapPhysicalMemory (
		int startAddress,       // physical address to map in
		int addressLength,      // how much to map
		int *virtualAddress     // where it got mapped to
	)
{
	DWORD                retLength;
	BOOL                 status;
	PHYSICAL_MEMORY_INFO pmi;
   
	if ( startAddress == (int) NULL || addressLength <= 0 )
		return ACCESN_OUT_OF_RANGE;

	pmi.InterfaceType       = Internal;
	pmi.BusNumber           = 0;
	pmi.BusAddress.HighPart = (LONG)0x0;
	pmi.BusAddress.LowPart  = (LONG)startAddress;
	pmi.AddressSpace        = (LONG) 0;
	pmi.Length              = addressLength;

	status = DeviceIoControl (      hDevice,
								IOCTL_IMB_MAP_MEMORY,
								& pmi,
								sizeof(PHYSICAL_MEMORY_INFO),
								virtualAddress,
								sizeof(PVOID),
								& retLength,
								0
							);
	if( status == TRUE ) {
		return ACCESN_OK;
	} else {
		return ACCESN_ERROR;
	}
}

ACCESN_STATUS
UnmapPhysicalMemory (
		int virtualAddress,     // what memory to unmap
        int Length )
{
	DWORD   retLength;
	BOOL    status;
   
	status = DeviceIoControl (      hDevice,
								IOCTL_IMB_UNMAP_MEMORY,
								& virtualAddress,
								sizeof(PVOID),
								NULL,
								0,
								& retLength,
								0
							);
 
	if( status == TRUE ) {
		return ACCESN_OK;
	} else {
		return ACCESN_ERROR;
	}
}

#else   /*Linux, SCO, UNIX, etc.*/

ACCESN_STATUS
MapPhysicalMemory(int startAddress,int addressLength, int *virtualAddress )
{
	int 				fd, i; 
	unsigned int 		length = addressLength;
	off_t 				startpAddress = (off_t)startAddress;
	unsigned int 		diff;
	caddr_t 			startvAddress;

	if ((startAddress == (int) NULL) || (addressLength <= 0))
		return ACCESN_ERROR;

	if ( (fd = open("/dev/mem", O_RDONLY)) < 0) {
		char buf[128];

		sprintf(buf,"%s %s: open(%s) failed",
                            __FILE__,__FUNCTION__,"/dev/mem");
		perror(buf);
		return ACCESN_ERROR ;
	}

	/* aliging the offset to a page boundary and adjusting the length */
	diff = (int)startpAddress % PAGESIZE;
	startpAddress -= diff;
	length += diff;

	if ( (startvAddress = mmap(	(caddr_t)0, 
								length, 
								PROT_READ, 
								MAP_SHARED, 
								fd, 
								startpAddress
								) ) == (caddr_t)-1)
	{
		char buf[128];

		sprintf(buf,"%s %s: mmap failed", __FILE__,__FUNCTION__);
		perror(buf);
		close(fd);
		return ACCESN_ERROR;
	}
#ifndef NO_MACRO_ARGS
	DEBUG("%s: mmap of 0x%x success\n",__FUNCTION__,startpAddress);
#endif
#ifdef LINUX_DEBUG_MAX
/* dont want this memory dump for normal level of debugging.
// So, I have put it under a stronger debug symbol. mahendra */

	for(i=0; i < length; i++)
	{
		printf("0x%x ", (startvAddress[i]));
		if(isascii(startvAddress[i])) {
			printf("%c ", (startvAddress[i]));
		}
	} 
#endif /*LINUX_DEBUG_MAX */

	*virtualAddress = (int)(startvAddress + diff);
	return ACCESN_OK;
}

ACCESN_STATUS
UnmapPhysicalMemory( int virtualAddress, int Length )
{
	unsigned int diff = 0;

	/* page align the virtual address and adjust length accordingly  */
	diff = 	((unsigned int) virtualAddress) % PAGESIZE;
	virtualAddress -= diff;
	Length += diff;
#ifndef NO_MACRO_ARGS
	DEBUG("%s: calling munmap(0x%x,%d)\n",__FUNCTION__,virtualAddress,Length);
#endif

	if(munmap((caddr_t)virtualAddress, Length) != 0)
	{
		char buf[128];

		sprintf(buf,"%s %s: munmap failed", __FILE__,__FUNCTION__);
		perror(buf);
		return ACCESN_ERROR;

	}
#ifndef NO_MACRO_ARGS
	DEBUG("%s: munmap(0x%x,%d) success\n",__FUNCTION__,virtualAddress,Length);
#endif

	return ACCESN_OK;
}
#endif    /*unix*/

#endif   /*IMB_MEMORY*/


/*/////////////////////////////////////////////////////////////////////////////
// GetIpmiVersion
//////////////////////////////////////////////////////////////////////////// */

/*F*
//  Name:       GetIpmiVersion 
//  Purpose:    This function returns current IPMI version
//  Context:   
//  Returns:    IPMI version
//  Parameters: 
//     reqPtr
//     timeOut
//     respDataPtr
//     respLen
//  Notes:      svuppula
*F*/
BYTE	GetIpmiVersion()
{
	return	((BYTE)IpmiVersion);
}

#ifdef IMBDLL
/* Inlude this routine if building WIN32 imbapi.dll */
BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
   switch(ulReason)
   {
      case DLL_PROCESS_ATTACH:
      case DLL_THREAD_ATTACH:
         break;
      case DLL_PROCESS_DETACH:
      case DLL_THREAD_DETACH:
         break;
      default:
         return FALSE;
   }
   return TRUE;
}
#endif

#endif
/*end of imbapi.c*/
