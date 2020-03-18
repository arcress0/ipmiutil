/*M*
//      $Workfile:   ipmims.cpp  $
//      $Revision:   1.0  $
//      $Modtime:   08 Sep 2008 13:31:14  $
//      $Author:   arcress at users.sourceforge.net  $  
//
// This implements support for the Microsoft Windows 2003 R2 IPMI driver.
// Note that this should be compiled with /TP (as C++).  
// 
// 09/08/08 ARCress - created.
// 04/27/11 Jay Krell - fixed WIN64-unsafe logic
// 09/30/13 ARCress   - fixed Win2012 x64 SafeArrayDestroy fault
//
 *M*/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2008, Intel Corporation
Copyright (c) 2009 Kontron America, Inc.
Copyright (c) 2013 Andy Cress <arcress at users.sourceforge.net>
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
/* Microsoft IPMI drver is only valid in Windows */
#define _WIN32_DCOM
#pragma once
#include <windows.h>
#include <stdio.h>
#include <iostream>   //deprecated iostream.h
#include <Objbase.h>  //ole32.lib
#include <comutil.h>
#include <Wbemcli.h>

#ifdef ALONE
#define uchar  unsigned char  
#define ushort unsigned short 
#define ulong  unsigned long  
#else
#include "ipmicmd.h"
extern "C" { extern ipmi_cmd_t ipmi_cmds[NCMDS]; }
#endif
static char fmsopen = 0;
static char fdebugms = 0;
static IWbemLocator *pLoc = 0;
static IWbemServices *pSvc = 0;
static IWbemClassObject* pClass = NULL;
static IEnumWbemClassObject* pEnumerator = NULL;
static IWbemClassObject* pInstance = NULL;
static VARIANT varPath; 
#define NVAR 32
/* Wbem error return codes, returned by WMI */
#ifdef RES_ALL
#define NRES   82
#else
#define NRES   10
#endif
static struct { char *desc; int val; } res_list[NRES] = {
{"WBEM_E_FAILED", 0x80041001  },
{"WBEM_E_NOT_FOUND", 0x80041002  },
{"WBEM_E_ACCESS_DENIED", 0x80041003  },
{"WBEM_E_PROVIDER_FAILURE", 0x80041004  },
{"WBEM_E_TYPE_MISMATCH", 0x80041005  },
{"WBEM_E_OUT_OF_MEMORY", 0x80041006  },
{"WBEM_E_INVALID_CONTEXT", 0x80041007  },
{"WBEM_E_INVALID_PARAMETER", 0x80041008  },
{"WBEM_E_NOT_AVAILABLE", 0x80041009  },
{"WBEM_E_CRITICAL_ERROR", 0x8004100A  } 
#ifdef RES_ALL
,
{"WBEM_E_CRITICAL_ERROR", 0x8004100A  } ,
{"WBEM_E_INVALID_STREAM", 0x8004100B  },
{"WBEM_E_NOT_SUPPORTED", 0x8004100C  },
{"WBEM_E_INVALID_SUPERCLASS", 0x8004100D  },
{"WBEM_E_INVALID_NAMESPACE", 0x8004100E  },
{"WBEM_E_INVALID_OBJECT", 0x8004100F  },
{"WBEM_E_INVALID_CLASS", 0x80041010  },
{"WBEM_E_PROVIDER_NOT_FOUND", 0x80041011  },
{"WBEM_E_INVALID_PROVIDER_REGISTRATION", 0x80041012  },
{"WBEM_E_PROVIDER_LOAD_FAILURE", 0x80041013  },
{"WBEM_E_INITIALIZATION_FAILURE", 0x80041014  },
{"WBEM_E_TRANSPORT_FAILURE", 0x80041015  },
{"WBEM_E_INVALID_OPERATION", 0x80041016  },
{"WBEM_E_INVALID_QUERY", 0x80041017  },
{"WBEM_E_INVALID_QUERY_TYPE", 0x80041018  },
{"WBEM_E_ALREADY_EXISTS", 0x80041019  },
{"WBEM_E_OVERRIDE_NOT_ALLOWED", 0x8004101A  },
{"WBEM_E_PROPAGATED_QUALIFIER", 0x8004101B  },
{"WBEM_E_PROPAGATED_PROPERTY", 0x8004101C  },
{"WBEM_E_UNEXPECTED", 0x8004101D  },
{"WBEM_E_ILLEGAL_OPERATION", 0x8004101E  },
{"WBEM_E_CANNOT_BE_KEY", 0x8004101F  },
{"WBEM_E_INCOMPLETE_CLASS", 0x80041020  },
{"WBEM_E_INVALID_SYNTAX", 0x80041021  },
{"WBEM_E_NONDECORATED_OBJECT", 0x80041022  },
{"WBEM_E_READ_ONLY", 0x80041023  },
{"WBEM_E_PROVIDER_NOT_CAPABLE", 0x80041024  },
{"WBEM_E_CLASS_HAS_CHILDREN", 0x80041025  },
{"WBEM_E_CLASS_HAS_INSTANCES", 0x80041026  },
{"WBEM_E_QUERY_NOT_IMPLEMENTED", 0x80041027  },
{"WBEM_E_ILLEGAL_NULL", 0x80041028  },
{"WBEM_E_INVALID_QUALIFIER_TYPE", 0x80041029  },
{"WBEM_E_INVALID_PROPERTY_TYPE", 0x8004102A  },
{"WBEM_E_VALUE_OUT_OF_RANGE", 0x8004102B  },
{"WBEM_E_CANNOT_BE_SINGLETON", 0x8004102C  },
{"WBEM_E_INVALID_CIM_TYPE", 0x8004102D  },
{"WBEM_E_INVALID_METHOD", 0x8004102E  },
{"WBEM_E_INVALID_METHOD_PARAMETERS", 0x8004102F  },
{"WBEM_E_SYSTEM_PROPERTY", 0x80041030  },
{"WBEM_E_INVALID_PROPERTY", 0x80041031  },
{"WBEM_E_CALL_CANCELLED", 0x80041032  },
{"WBEM_E_SHUTTING_DOWN", 0x80041033  },
{"WBEM_E_PROPAGATED_METHOD", 0x80041034  },
{"WBEM_E_UNSUPPORTED_PARAMETER", 0x80041035  },
{"WBEM_E_MISSING_PARAMETER_ID", 0x80041036  },
{"WBEM_E_INVALID_PARAMETER_ID", 0x80041037  },
{"WBEM_E_NONCONSECUTIVE_PARAMETER_IDS", 0x80041038  },
{"WBEM_E_PARAMETER_ID_ON_RETVAL", 0x80041039  },
{"WBEM_E_INVALID_OBJECT_PATH", 0x8004103A  },
{"WBEM_E_OUT_OF_DISK_SPACE", 0x8004103B  },
{"WBEM_E_BUFFER_TOO_SMALL", 0x8004103C  },
{"WBEM_E_UNSUPPORTED_PUT_EXTENSION", 0x8004103D  },
{"WBEM_E_UNKNOWN_OBJECT_TYPE", 0x8004103E  },
{"WBEM_E_UNKNOWN_PACKET_TYPE", 0x8004103F  },
{"WBEM_E_MARSHAL_VERSION_MISMATCH", 0x80041040  },
{"WBEM_E_MARSHAL_INVALID_SIGNATURE", 0x80041041  },
{"WBEM_E_INVALID_QUALIFIER", 0x80041042  },
{"WBEM_E_INVALID_DUPLICATE_PARAMETER", 0x80041043  },
{"WBEM_E_TOO_MUCH_DATA", 0x80041044  },
{"WBEM_E_SERVER_TOO_BUSY", 0x80041045  },
{"WBEM_E_INVALID_FLAVOR", 0x80041046  },
{"WBEM_E_CIRCULAR_REFERENCE", 0x80041047  },
{"WBEM_E_UNSUPPORTED_CLASS_UPDATE", 0x80041048  },
{"WBEM_E_CANNOT_CHANGE_KEY_INHERITANCE", 0x80041049  },
{"WBEM_E_CANNOT_CHANGE_INDEX_INHERITANCE", 0x80041050  },
{"WBEM_E_TOO_MANY_PROPERTIES", 0x80041051  },
{"WBEM_E_UPDATE_TYPE_MISMATCH", 0x80041052  },
{"WBEM_E_UPDATE_OVERRIDE_NOT_ALLOWED", 0x80041053  },
{"WBEM_E_UPDATE_PROPAGATED_METHOD", 0x80041054  },
{"WBEM_E_METHOD_NOT_IMPLEMENTED", 0x80041055  },
{"WBEM_E_METHOD_DISABLED", 0x80041056  },
{"WBEMESS_E_REGISTRATION_TOO_BROAD", 0x80042001  },
{"WBEMESS_E_REGISTRATION_TOO_PRECISE", 0x80042002 }
#endif
};

/* 
 * Microsoft_IPMI methods:
 * void RequestResponse(
 *    [in]   uint8 Command,
 *    [out]  uint8 CompletionCode,
 *    [in]   uint8  Lun,
 *    [in]   uint8 NetworkFunction,
 *    [in]   uint8 RequestData[],
 *    [out]  uint32 ResponseDataSize,
 *    [in]   uint32 RequestDataSize,
 *    [in]   uint8 ResponderAddress,
 *    [out]  uint8 ResponseData
 * );
 * 
 * void SMS_Attention(
 *    [out]  boolean AttentionSet,
 *    [out]  uint8 StatusRegisterValue
 * );
 */

static void cleanup_wmi(void)
{
   VariantClear(&varPath);
   if (pInstance != NULL) pInstance->Release();
   if (pEnumerator != NULL) pEnumerator->Release();
   if (pClass != NULL) pClass->Release();
   if (pSvc != 0) pSvc->Release();
   if (pLoc != 0) pLoc->Release();
   CoUninitialize();
}

static void dumpbuf(uchar *p, int len, char fwrap)
{
   int i;
   for (i = 0; i < len; i++) {
	if (fwrap) {
	    if ((i % 16) == 0) printf("\n%04x: ",i);
	}
	printf(" %02x",p[i]);
   }
   printf("\n");
   return;
}

static char *res_str(HRESULT res)
{
    char *pstr = "";
    int i;
    for (i = 0; i < NRES; i++) {
	if (res_list[i].val == res) {
		pstr = &res_list[i].desc[0];
		break;
	}
    }
    return (pstr);
}

extern "C" {

int ipmi_open_ms(char fdebugcmd)
{
    int bRet = -1;
    HRESULT hres;
    ULONG dwCount = NULL;
 
    fdebugms = fdebugcmd;
    // Initialize COM.
    hres =  CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        if (fdebugcmd) printf("ipmi_open_ms: CoInitializeEx error\n");
        return bRet;
    }
 
    // Obtain the initial locator to Windows Management
    // on a particular host computer.
    hres = CoCreateInstance( CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                IID_IWbemLocator, (LPVOID *) &pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        if (fdebugcmd) printf("ipmi_open_ms: CreateInstance(WbemLoc) error\n");
        return bRet;
    }
 
 
    // Connect to the root\cimv2 namespace with the current user 
    // and obtain pointer pSvc to make IWbemServices calls.
    hres = pLoc->ConnectServer( _bstr_t(L"ROOT\\WMI"), NULL,  NULL, 0,   
				NULL, 0,  0,  &pSvc );
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        if (fdebugcmd) printf("ipmi_open_ms: ConnectServer error\n");
        return bRet;
    }

    // Set the IWbemServices proxy so that impersonation
    // of the user (client) occurs.
    hres = CoSetProxyBlanket( pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
		NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL, EOAC_NONE );
    if (FAILED(hres)) {
        if (fdebugcmd) printf("ipmi_open_ms: Cannot SetProxyBlanket\n");
	cleanup_wmi();
        return bRet;               // Program has failed.
    }

    hres = pSvc->GetObject( L"Microsoft_IPMI", 0, NULL, &pClass, NULL);
    if (FAILED(hres)) {
	cleanup_wmi();
        if (fdebugcmd) 
	     printf("ipmi_open_ms: cannot open microsoft_ipmi driver (ipmidrv.sys)\n");
        return bRet;
    } 

    hres = pSvc->CreateInstanceEnum( L"microsoft_ipmi", 0, NULL, &pEnumerator);
    if (FAILED(hres)) {
	cleanup_wmi();
        if (fdebugcmd) 
	     printf("ipmi_open_ms: cannot open microsoft_ipmi Enum\n");
        return bRet;
    } 
            
    hres = pEnumerator->Next( WBEM_INFINITE, 1, &pInstance, &dwCount);
    if (FAILED(hres)) {
        if (fdebugcmd) 
	     printf("ipmi_open_ms: Cannot get microsoft_ipmi instance\n");
	cleanup_wmi();
        return bRet;
    } 
    VariantInit(&varPath);
    hres = pInstance->Get(_bstr_t(L"__RelPath"), 0, &varPath, NULL, 0);
    if (FAILED(hres)) {
        if (fdebugcmd) 
	     printf("ipmi_open_ms: Cannot get instance Path %s\n","__RelPath");
	cleanup_wmi();
        return bRet;
    } else {   /*success*/
	if (fdebugcmd) 
		printf("ipmi_open_ms: ObjectPath: %ls\n",V_BSTR(&varPath));
	// usually  L"Microsoft_IPMI.InstanceName=\"Root\\SYSTEM\\0003_0\"",
	fmsopen = 1;
	bRet = 0;
    }

    return bRet;
}
 
int ipmi_close_ms(void)
{
   int bRet = -1;
   if (fmsopen) {
	cleanup_wmi();
	fmsopen = 0;
	bRet = 0;
   }
   return bRet;
}
 
int ipmi_cmdraw_ms(uchar cmd, uchar netfn, uchar lun, uchar sa,
		uchar bus, uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd)
{
   int bRet;
   HRESULT hres;
   IWbemClassObject* pInParams = NULL; /*class definition*/
   IWbemClassObject* pInReq = NULL;    /*instance*/
   IWbemClassObject* pOutResp = NULL;
   VARIANT varCmd, varNetfn, varLun, varSa, varSize, varData;
   SAFEARRAY* psa = NULL;
   long i;
   uchar *p;

   fdebugms = fdebugcmd;
   if (!fmsopen) {
      bRet = ipmi_open_ms(fdebugcmd);
      if (bRet != 0) return(bRet);
   }
   bRet = -1;


   hres = pClass->GetMethod(L"RequestResponse",0,&pInParams,NULL);
   if (FAILED(hres)) {
        if (fdebugcmd) 
	     printf("ipmi_cmdraw_ms: Cannot get RequestResponse method\n");
        return (bRet);
   }

#ifdef WDM_FIXED
   /* see http://support.microsoft.com/kb/951242 for WDM bug info */
   hres = pInParams->SpawnInstance(0,&pInReq);
   if (FAILED(hres)) {
        if (fdebugcmd) 
	     printf("ipmi_cmdraw_ms: Cannot get RequestResponse instance\n");
        return (bRet);
   }
   // also substitute pInReq for pInParams below if this gets fixed.
#endif

   VariantInit(&varCmd);
   varCmd.vt = VT_UI1;
   varCmd.bVal = cmd;
   hres = pInParams->Put(_bstr_t(L"Command"), 0, &varCmd, 0);
   // VariantClear(&varCmd);
   if (FAILED(hres)) goto MSRET;

   VariantInit(&varNetfn);
   varNetfn.vt = VT_UI1;
   varNetfn.bVal = netfn;
   hres = pInParams->Put(_bstr_t(L"NetworkFunction"), 0, &varNetfn, 0);
   // VariantClear(&varNetfn);
   if (FAILED(hres)) goto MSRET;

   VariantInit(&varLun);
   varLun.vt = VT_UI1;
   varLun.bVal = lun;
   hres = pInParams->Put(_bstr_t(L"Lun"), 0, &varLun, 0);
   // VariantClear(&varLun);
   if (FAILED(hres)) goto MSRET;

   VariantInit(&varSa);
   varSa.vt = VT_UI1;
   varSa.bVal = sa;
   hres = pInParams->Put(_bstr_t(L"ResponderAddress"), 0, &varSa, 0);
   // VariantClear(&varSa);
   if (FAILED(hres)) goto MSRET;

   VariantInit(&varSize);
   varSize.vt = VT_I4;
   varSize.lVal = sdata;
   hres = pInParams->Put(_bstr_t(L"RequestDataSize"), 0, &varSize, 0);
   // VariantClear(&varSize);
   if (FAILED(hres)) goto MSRET;

   SAFEARRAYBOUND rgsabound[1];
   rgsabound[0].cElements = sdata;
   rgsabound[0].lLbound = 0;
   psa = SafeArrayCreate(VT_UI1,1,rgsabound);
   if(!psa) {
      printf("ipmi_cmdraw_ms: SafeArrayCreate failed\n");
      goto MSRET;
   }
#ifdef SHOULD_WORK_BUT_NO
   /* The SafeArrayPutElement does not put the data in the right 
    * place, so skip this and copy the raw data below. */
   VARIANT tvar;
   if (fdebugcmd && sdata > 0) 
	{ printf("psa1(%p):",psa); dumpbuf((uchar *)psa,42,1); }   

   for(i =0; i< sdata; i++)
   {
      VariantInit(&tvar);
      tvar.vt = VT_UI1;
      tvar.bVal = pdata[i];
      hres = SafeArrayPutElement(psa, &i, &tvar);
      // VariantClear(&tvar);
      if (FAILED(hres)) { 
         printf("ipmi_cmdraw_ms: SafeArrayPutElement(%d) failed\n",i);
         goto MSRET;
      }
   } /*end for*/
   if (fdebugcmd && sdata > 0) 
	{ printf("psa2(%p):",psa); dumpbuf((uchar *)psa,42,1); }  
#endif

   /* Copy the real RequestData into psa */
   memcpy(psa->pvData,pdata,sdata);

   VariantInit(&varData);
   varData.vt = VT_ARRAY | VT_UI1;
   varData.parray = psa;
   hres = pInParams->Put(_bstr_t(L"RequestData"), 0, &varData, 0);
   // VariantClear(&varData);
   if (FAILED(hres)) {
	printf("Put(RequestData) error %x\n",hres);
        goto MSRET;
   }

#ifdef TEST_METHODS
   IWbemClassObject* pOutSms = NULL;
   if (fdebugcmd) printf("ipmi_cmdraw_ms: calling SMS_Attention(%ls)\n",
			  V_BSTR(&varPath)); 
   hres = pSvc->ExecMethod( V_BSTR(&varPath), _bstr_t(L"SMS_Attention"), 
				0, NULL, NULL, &pOutSms, NULL);
   if (FAILED(hres)) {
	printf("ipmi_cmdraw_ms: SMS_Attention method error %x\n",hres);
        goto MSRET;
   }
   if (fdebugcmd) printf("ipmi_cmdraw_ms: SMS_Attention method ok\n"); 
   /* This does work, without input parameters */
   pOutSms->Release();
#endif

   hres = pSvc->ExecMethod( V_BSTR(&varPath), _bstr_t(L"RequestResponse"), 
				0, NULL, pInParams, &pOutResp, NULL);
   if (fdebugcmd) {
       printf("ipmi_cmdraw_ms(cmd=%x,netfn=%x,lun=%x,sa=%x,sdata=%d)"
	      " RequestResponse ret=%x\n", cmd,netfn,lun,sa,sdata,hres); 
       if (sdata > 0) {
	   printf("ipmi_cmdraw_ms: req data(%d):",sdata); 
	   dumpbuf(pdata,sdata,0); 
       }
   }
   if (FAILED(hres)) {
	printf("ipmi_cmdraw_ms: RequestResponse error %x %s\n",
		hres,res_str(hres));
#ifdef EXTRA_DESC
	/* This does not usually add any meaning for IPMI. */
	BSTR desc;
	IErrorInfo *pIErrorInfo;
	GetErrorInfo(0,&pIErrorInfo);
	pIErrorInfo->GetDescription(&desc);
	printf("ipmi_cmdraw_ms: ErrorInfoDescr: %ls\n",desc);
	SysFreeString(desc);
#endif
	bRet = -1; 
	/*fall through for cleanup and return*/
   }
   else {  /*successful, get ccode and response data */
	VARIANT varByte, varRSz, varRData;
        VariantInit(&varByte);
        VariantInit(&varRSz);
        VariantInit(&varRData);
	long rlen;

	hres = pOutResp->Get(_bstr_t(L"CompletionCode"),0, &varByte, NULL, 0);
	if (FAILED(hres)) goto MSRET;
	if (fdebugcmd) printf("ipmi_cmdraw_ms: CompletionCode %x returned\n",
				V_UI1(&varByte) );
	*pcc = V_UI1(&varByte);

	hres = pOutResp->Get(_bstr_t(L"ResponseDataSize"),0, &varRSz, NULL, 0);
	if (FAILED(hres)) goto MSRET;
        rlen = V_I4(&varRSz);
	if (rlen > 1) rlen--;   /*skip cc*/
	if (rlen > *sresp) {
	   if (fdebugcmd) printf("ResponseData truncated from %d to %d\n",
					rlen,*sresp);
	   rlen = *sresp; /*truncate*/
	}
	*sresp = (int)rlen;

	hres = pOutResp->Get(_bstr_t(L"ResponseData"),0, &varRData, NULL,0);
	if (FAILED(hres)) { /*ignore failure */ 
	   if (fdebugcmd) printf("Get ResponseData error %x\n",hres); 
	} else {  /* success */
#ifdef SHOULD_WORK_BUT_NO
	    uchar *pa;
	    p = (uchar*)varRData.parray->pvData;
	    pa = (uchar*)varRData.parray;
	    printf("pa=%p, pa+12=%p p=%p\n",pa,(pa+12),p);
	    if (fdebugcmd) {   
		 printf("Data.vt = %04x, Data.parray(%p):",
			varRData.vt, varRData.parray); 
	         // 0x2011 means VT_ARRAY | VT_UI1
		 dumpbuf((uchar *)varRData.parray,40,1);
	    }
	    /* The SafeArrayGetElement does not get the data from the right 
	     * place, so skip this and copy the raw data below. */
	    VARIANT rgvar[NVAR];
	    if (rlen > NVAR) *pcc = 0xEE; 
	    for (i = 0; i <= rlen; i++)
         	VariantInit(&rgvar[i]);
	    /* copy the response data from varRData to presp */
	    for( i = 0; i <= rlen; i++)
	    {
		hres = SafeArrayGetElement(varRData.parray, &i, &rgvar[i]);
		if (FAILED(hres)) { 
		   if (fdebugcmd)
		      printf("ipmi_cmdraw_ms: SafeArrayGetElement(%d) failed\n",i);
		   break;
		}
		if (fdebugcmd) {   
		     printf("Data[%d] vt=%02x val=%02x, rgvar(%p):",i,
				rgvar[i].vt, V_UI1(&rgvar[i]),&rgvar[i]);
		     dumpbuf((uchar *)&rgvar[i],12,0);
		}
	        /* skip the completion code */
	    	// if (i > 0) presp[i-1] = V_UI1(&rgvar[i]);
	    } /*end for*/
#endif
	    /* 
	     * parray from a GetDeviceId response:
	     * 0015CEE0: 01 00 80 00 01 00 00 00 00 00 00 00 00 cf 15 00
	     * 0015CEF0: 10 00 00 00 00 00 00 00 03 00 06 00 95 01 08 00
             *           ^- datalen=0x10
	     * 0015CF00: 00 20 01 00 19 02 9f 57 01 ...  
             *           ^- start of data (cc=00, ...)
	     */
	    /* Copy the real ResponseData into presp. */
	    p = (uchar*)varRData.parray->pvData;
	    for( i = 0; i <= rlen; i++) {
	        /* skip the completion code */
	    	if (i > 0) presp[i-1] = p[i];
	    }
	    if (fdebugcmd) {
		printf("ipmi_cmdraw_ms: resp data(%d):",rlen+1); 
		dumpbuf(p,rlen+1,0); 
	    }
	}
	bRet = 0;
   }

MSRET:
#define CLEAN_OK  1
#ifdef CLEAN_OK
   /* VariantClear(&var*) should be done by pInParams->Release() */
   if (psa != NULL) SafeArrayDestroy(psa);
   if (pInParams != NULL) pInParams->Release();
   if (pOutResp != NULL) pOutResp->Release();
#endif
   return(bRet);
}

#ifndef ALONE
int ipmi_cmd_ms(ushort cmd, uchar *pdata, int sdata, 
	 	uchar *presp, int *sresp, uchar *pcc, char fdebugcmd)
{
   int bRet = -1;
   int i;

   fdebugms = fdebugcmd;
   for (i = 0; i < NCMDS; i++) {
       if (ipmi_cmds[i].cmdtyp == cmd) break;
   }
   if (i >= NCMDS) {
        printf("ipmi_cmd_ms: Unknown command %04x\n",cmd);
        return(-1);
   }
   if (cmd >= CMDMASK) cmd = cmd & CMDMASK;  /* unmask it */

   bRet = ipmi_cmdraw_ms((uchar)cmd,ipmi_cmds[i].netfn,ipmi_cmds[i].lun,
                        ipmi_cmds[i].sa, ipmi_cmds[i].bus,
                        pdata,sdata,presp,sresp,pcc,fdebugcmd);
   return (bRet);
}
#endif

}  /* end C */

#ifdef TEST_BIN
int main(int argc, char **argv)
{
    uchar  rdata[40];
    int    i, rv;
    int    rlen = 0;
    uchar cc;

    fdebugms = 1;
    rlen = sizeof(rdata);
    rv = ipmi_cmdraw_ms(0x01, 0x06, 0, 0x20, 0,  /*get_device_id*/
			NULL, 0, rdata, &rlen, &cc, fdebugms);
    printf("ipmi_cmdraw_ms ret=%d, cc=%02x\n",rv,cc);
    if (rv == 0) {
       printf(" ** Return Code: %2.2X\n", cc);
       printf(" ** Data[%d]:",rlen);
       for (i=0; i < rlen; i++)
	    printf(" %2.2X", rdata[i]);
       printf("\n");
    }
    ipmi_close_ms();
    return 0;
}
#endif


#endif
/* endif WIN32 */

/* end ipmims.cpp */
