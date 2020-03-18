/*----------------------------------------------------------------------*
 * mem_if.c
 *    Get SMBIOS Tables and associated data. 
 *    This code is 64-bit safe.
 * Note that for Windows, this should be compiled with /TP (as C++).
 *
 * 12/17/07 ARCress - created
 * 02/26/08 ARCress - decode type 15 log structure
 * 07/21/08 ARCress - fixed for 64-bit memory model
 * 08/12/08 ARCress - trim out extra stuff, consolidated
 *----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2002-2008, Intel Corporation
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
#define _WIN32_DCOM
#pragma once
#include <windows.h>
#include <stdio.h>
#include <iostream>   //deprecated iostream.h
#include <Objbase.h>  //ole32.lib
#include <comutil.h>
#include <Wbemcli.h>  //Wbemcli.h,Wbemidl.h  Wbemuuid.lib

#elif defined(DOS)
#include <dos.h>
#include <stdlib.h>
#include <string.h>

#else // Linux or Solaris
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <linux/param.h>
#endif
#endif
#if defined(SOLARIS) || defined(BSD) || defined(HPUX)
#define EXEC_PAGESIZE     4096
#endif

#define	DOS_MEM_RANGE_BASE   0xF0000  //starting memory range for smbios tables
#define	DOS_NUM_BYTES_TO_MAP 0x0FFFE  //number bytes to map for smbios tables

//smbios defines
#define	SMBIOS_STRING		"_SM_"		//server management bios string
#define	SMBIOS_MAJOR_REV		2	//major rev for smbios spec 2.3
#define	SMBIOS_MINOR_REV		1	//minor rev for smbios spce 2.3
#define	SMBIOS_STRUCTURE_TYPE_OFFSET	0	//offset in structure for type
#define	SMBIOS_STRUCTURE_LENGTH_OFFSET	1	//offset in structure for length
#define	SMBIOS_TABLE_LENGTH_OFFSET	0x5	//offset for length of table
#define SMBIOS_MAJOR_REV_OFFSET		0x6	//offset for major rev of smbios
#define SMBIOS_MINOR_REV_OFFSET		0x7	//offset for minor rev of smbios
#define	SMBIOS_TABLE_ENTRY_POINT_OFFSET	0x18	//offset for smbios structure table entry point
#define	SMBIOS_TABLE_SIZE_OFFSET	0x16	//offset for smbios structure table size
#define NIBBLE_SIZE			4	//bit size of a nibble

#ifdef	WIN32
extern "C" { char fsm_debug = 0; }  /*=1 to show smbios debug messages*/
#else
char fsm_debug = 0;   /*=1 to show smbios debug messages*/
#endif
static int   s_iTableRev = 0;   //static globabl var to store table revision

#ifndef WIN32
typedef int           BOOL;
typedef unsigned long ULONG;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;

BOOL MapPhysicalMemory(ULONG	tdStartAddress, ULONG	ulSize,
		   ULONG*	ptdVirtualAddress);
BOOL UnMapPhysicalMemory(ULONG tdVirtualAddress, ULONG	ulSize);
int  OpenIMemoryInterface(void);
int  CloseIMemoryInterface(void);
#endif // Linux, not __WINDOWS__

#ifdef	WIN32
       /* Windows Memory routines */
static BYTE m_byMajorVersion = 0;
static BYTE m_byMinorVersion = 0;
static BYTE * m_pbBIOSData = NULL;
static DWORD  m_dwLen = 0;

extern "C" {

int  getSmBiosTables(UCHAR **ptableAddress)
{
    int bRet = 0;
    HRESULT hres;
    IWbemLocator *pLoc = 0;
    IWbemServices *pSvc = 0;
    IEnumWbemClassObject* pEnumerator = NULL;

    // Initialize COM.
    hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
    if (FAILED(hres)) {
        return bRet;  
    }

    // Obtain the initial locator to Windows Management
    // on a particular host computer.
    hres = CoCreateInstance( CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, 
                IID_IWbemLocator, (LPVOID *) &pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        return bRet;  
    }


    // Connect to the root\cimv2 namespace with the
    // current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\WMI"), // WMI namespace
        NULL,                    // User name
        NULL,                    // User password
        0,                       // Locale
        NULL,                    // Security flags                 
        0,                       // Authority       
        0,                       // Context object
        &pSvc                    // IWbemServices proxy
        );                              
    if (FAILED(hres)) {
        pLoc->Release();     
        CoUninitialize();
        return bRet;  
    }

    // Set the IWbemServices proxy so that impersonation
    // of the user (client) occurs.
    hres = CoSetProxyBlanket(
       pSvc,                         // the proxy to set
       RPC_C_AUTHN_WINNT,            // authentication service
       RPC_C_AUTHZ_NONE,             // authorization service
       NULL,                         // Server principal name
       RPC_C_AUTHN_LEVEL_CALL,       // authentication level
       RPC_C_IMP_LEVEL_IMPERSONATE,  // impersonation level
       NULL,                         // client identity 
       EOAC_NONE                     // proxy capabilities     
       );
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return bRet;               // Program has failed.
    }

    hres = pSvc->CreateInstanceEnum( L"MSSMBios_RawSMBiosTables", 0, 
                                    NULL, &pEnumerator);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return bRet;               // Program has failed.
    } else {
        do {
            IWbemClassObject* pInstance = NULL;
            ULONG dwCount = NULL;

            hres = pEnumerator->Next(WBEM_INFINITE, 1, &pInstance, &dwCount); 
			if (SUCCEEDED(hres))
			{
				VARIANT varBIOSData;
				VariantInit(&varBIOSData);
				CIMTYPE             type;

				hres = pInstance->Get(bstr_t("SmbiosMajorVersion"),0,&varBIOSData,&type,NULL);
				if (FAILED(hres)) {
					VariantClear(&varBIOSData);
				} else {
					m_byMajorVersion = (BYTE)varBIOSData.iVal;
					VariantInit(&varBIOSData);
					hres = pInstance->Get(bstr_t("SmbiosMinorVersion"),0,&varBIOSData,&type,NULL);
					if(FAILED(hres)) {
						VariantClear(&varBIOSData);
					} else {
						m_byMinorVersion = (BYTE)varBIOSData.iVal;
						s_iTableRev = (m_byMajorVersion << 4) + m_byMinorVersion;
						
						VariantInit(&varBIOSData);
						hres = pInstance->Get(bstr_t("SMBiosData"),0,&varBIOSData,&type,NULL);
						if(SUCCEEDED(hres)) {
							if ( ( VT_UI1 | VT_ARRAY ) != varBIOSData.vt ) {
							} else {
								SAFEARRAY *parray = NULL;
								parray = V_ARRAY(&varBIOSData);
								BYTE* pbData = (BYTE*)parray->pvData;

								m_dwLen = parray->rgsabound[0].cElements;

								m_pbBIOSData = (BYTE *)malloc(m_dwLen);
								memcpy(m_pbBIOSData,pbData,m_dwLen);
								*ptableAddress = m_pbBIOSData;
								bRet = m_dwLen;
							}
						}
						VariantClear(&varBIOSData);
					}
				}
				break;
			}
        } while (hres == WBEM_S_NO_ERROR);
    }
	
    // Cleanup
    // ========
    pSvc->Release();
    pLoc->Release();     
    CoUninitialize();
    return(bRet);
} /*end getSmBiosTables*/

int  closeSmBios(UCHAR *ptableAddress, ULONG ulSmBiosLen)
{
	int rv = 0;
	/* memory interface already closed */
	free(ptableAddress);
        return(rv);
}

} /*end extern C*/

#else
       /* Linux Memory routines */
#define	MEM_DRIVER		"/dev/mem"
#define FALSE			0
#define TRUE			1
//intialize static handle amd counter
static int	m_iDriver = 0;
static int	m_iCount = 0;

//////////////////////////////////////////////////////////////////////////////
// OpenIMemoryInterface
//////////////////////////////////////////////////////////////////////////////
//  Name:       OpenIMemoryInterface 
//  Purpose:    create handle to driver if first time
//  Context:    static m_iDriver & m_iCount
//  Returns:    int rv (0 = success)
//  Parameters: none
//  Notes:      
///////////////////////////////////////////////////////////////////////////////
int OpenIMemoryInterface(void)
{
     int rv = -1;
/* ARM64 does not handle /dev/mem the same.  
 * It exposes SMBIOS at /sys/firmware/dmi/tables/, but because 
 * IO memory is memory mapped, cannot use legacy /dev/mem. */
#ifndef __arm__
#ifndef __aarch64__
	//check to see if driver has been previously defined
	if (!m_iDriver) { //open the driver
		m_iDriver = open(MEM_DRIVER, O_RDONLY);
		if (m_iDriver > 0) {  //increment instance counter
			m_iCount++;
		} else {  //couldn't open the driver...so make it zero again
			m_iDriver = 0;
		}
	} else {  //handle exists...so inc instance counter.
		m_iCount++;
	}
	if (m_iDriver > 0) rv = 0;
#endif
#endif
	return(rv);
}

///////////////////////////////////////////////////////////////////////////////
// CloseIMemoryInterface
//  Name:       destructor
//  Purpose:    close driver handle and null out field
//  Context:    
//  Returns:    
//  Parameters: none
//  Notes:      
///////////////////////////////////////////////////////////////////////////////
int CloseIMemoryInterface(void)
{
     int rv = 0;
	//check if an instance has been created by looking at the counter
	if (!m_iCount) {
		m_iCount--;  //decrement instance counter
		if (m_iCount == 0) {
			close(m_iDriver);
			m_iDriver = 0; //and null it out
		}
	}
     return(rv);
}

///////////////////////////////////////////////////////////////////////////////
// MapPhysicalMemory
///////////////////////////////////////////////////////////////////////////////
//  Name:       MapPhysicalMemory 
//  Purpose:    given the starting address and size, the virtualize 
//				memory is mapped.
//  Returns:    True if success
//  Parameters: [IN] tdStartAddress = physical address to map
//				[IN] ulSize = number of bytes to map
//				[OUT] ptdVirtualAddress = ptr to virtual address
///////////////////////////////////////////////////////////////////////////////
BOOL MapPhysicalMemory(ULONG	tdStartAddress, 
			ULONG	ulSize,
			ULONG*	ptdVirtualAddress)
{
	long	tdVirtualAddr = 0;
	ULONG	ulDiff;
	//check if we have a valid driver handle
	if (!m_iDriver) return FALSE;	//error
	//check for valid input params
	if (!tdStartAddress || !ulSize) return FALSE;	//error
	//align the offset to a valid page boundary and adust its length
	ulDiff = (ULONG)(tdStartAddress % EXEC_PAGESIZE);
        if (fsm_debug) printf("MapPhys: tdStart=%lx, page=%x, diff=%lx\n",
			tdStartAddress,EXEC_PAGESIZE,ulDiff);
	tdStartAddress -= ulDiff;
	ulSize += ulDiff;
	tdVirtualAddr = (long)mmap((caddr_t)0,
					(size_t)ulSize,
					PROT_READ,
					MAP_SHARED,
					m_iDriver,
					(off_t)tdStartAddress);
        if (fsm_debug) printf("MapPhys: mmap(tdStart=%lx,size=%lx) = %lx\n",
			tdStartAddress,ulSize,tdVirtualAddr);
	if (tdVirtualAddr == -1)
		return FALSE;
	*ptdVirtualAddress = tdVirtualAddr + ulDiff;
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// UnMapPhysicalMemory
///////////////////////////////////////////////////////////////////////////////
//  Name:       UnMapPhysicalMemory 
//  Purpose:    given the virtual address, unmaps it...frees memory.
//  Returns:    True if success
//  Parameters: [IN] tdVirtualAddress = virtual address to unmap
//		[IN] ulSize = not needed for windows
//  Notes:      none
///////////////////////////////////////////////////////////////////////////////
BOOL UnMapPhysicalMemory(ULONG	tdVirtualAddress,
			  ULONG		ulSize)
{
	ULONG	ulDiff = 0;
	ulDiff = (ULONG)(tdVirtualAddress % EXEC_PAGESIZE);
	tdVirtualAddress -= ulDiff;
	ulSize += ulDiff;
	if (munmap((void *)tdVirtualAddress,ulSize) != 0) return FALSE;
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// getSmBiosTables
// Purpose:    returns smbios tables pointer
// Returns:    pointer to the virtual address where SMBIOS tables begins
//////////////////////////////////////////////////////////////////////////////
int  getSmBiosTables(UCHAR **ptableAddress)
{
	ULONG		tdStartAddress = DOS_MEM_RANGE_BASE;
	ULONG		ulSize = DOS_NUM_BYTES_TO_MAP;
	ULONG 		ulSmBiosLen = 0;
	ULONG		tdVirtualAddress = 0;
	int		iInc;
	UCHAR		*pSmBios = NULL;
	UCHAR		ucCheckSum = 0;
	UCHAR		ucVal, ucVal1;
	int rv;

	if (fsm_debug) printf("getSmBiosTables start\n");
	//get interface to Memory
	rv = OpenIMemoryInterface();
	if (fsm_debug) printf("OpenIMemoryInterface rv = %d\n",rv);

	//map memory into user space
	if ((rv != 0) || 
		!MapPhysicalMemory(tdStartAddress,ulSize,&tdVirtualAddress))
	{
#ifdef SOLARIS
	   /* mmap always fails on Solaris, so skip the error message,
	    * unless in debug mode. */
	   if (fsm_debug) 
#endif
		fprintf(stderr, "Cannot map memory.\n");
	        return ulSmBiosLen; /*==0*/
	}

	//now find the entry point for smbios
	for(iInc=0; iInc < (long)ulSize; iInc+=sizeof(int) )
	{
		if (!memcmp((UCHAR *)(tdVirtualAddress+iInc),SMBIOS_STRING,sizeof(int)))
		{
			//cast entry point to a byte pointer
			pSmBios = (UCHAR *)(tdVirtualAddress+iInc);
			break;
		}
	}

	if (pSmBios == NULL)
	{
		fprintf(stderr, "Can't find SMBIOS address entry point.\n");
		UnMapPhysicalMemory(tdVirtualAddress,ulSize);
		return ulSmBiosLen;
	}

	if (fsm_debug) {  
		printf("Found pSmBios=%p tdV=%lx, inc=%x\n",pSmBios,
		 	tdVirtualAddress,iInc); 
		// dump_buf("SMBIOS",pSmBios,64,1);
	}
	//now sum up the bytes in the table for checksum checking
	for(iInc=0; iInc<pSmBios[SMBIOS_TABLE_LENGTH_OFFSET]; iInc++)	
		ucCheckSum += pSmBios[iInc];

	if (ucCheckSum)
	{
		UnMapPhysicalMemory(tdVirtualAddress,ulSize);
		fprintf(stderr, "_SM_ Checksum != 0.\n");
		return ulSmBiosLen;
	}

	//save smbios revision for future calls...bcd format
	s_iTableRev = (pSmBios[SMBIOS_MAJOR_REV_OFFSET]<<NIBBLE_SIZE) | pSmBios[SMBIOS_MINOR_REV_OFFSET];

	//find smbios table entry point and size of tables
	memcpy(&tdStartAddress,&pSmBios[SMBIOS_TABLE_ENTRY_POINT_OFFSET],4);
			//sizeof(ULONG));
	memcpy(&ulSmBiosLen,&pSmBios[SMBIOS_TABLE_SIZE_OFFSET],sizeof(USHORT));
	memcpy(&ucVal,&pSmBios[SMBIOS_TABLE_SIZE_OFFSET],sizeof(UCHAR));
	memcpy(&ucVal1,&pSmBios[SMBIOS_TABLE_SIZE_OFFSET+1],sizeof(UCHAR));


	//unmap physical memory (f0000-ffffe)
	UnMapPhysicalMemory(tdVirtualAddress,ulSize);

	if (!MapPhysicalMemory(tdStartAddress,ulSmBiosLen,&tdVirtualAddress)) {
#ifdef SOLARIS
	   /* mmap always fails on Solaris, so skip the error message,
	    * unless in debug mode. */
	   if (fsm_debug) 
#endif
		fprintf(stderr, "Cannot map memory.\n");
		return 0;
        }

	if (fsm_debug) {  
	     printf("MapMemory(%lx,%lx) ok = %lx\n",tdStartAddress,
			ulSmBiosLen,tdVirtualAddress); 
	     // dump_buf("Table",(UCHAR *)tdVirtualAddress,256,1);
        }
	//set size_t smbios tables starting address to ptr
	*ptableAddress = (UCHAR*)tdVirtualAddress;

	//Not doing this since the map is needed
	//	UnMapPhysicalMemory(tdVirtualAddress,ulSmBiosLen);
	// rv = CloseIMemoryInterface();
	return ulSmBiosLen;
}

int  closeSmBios(UCHAR *ptableAddress, ULONG ulSmBiosLen)
{
	int rv;
	UnMapPhysicalMemory((size_t)ptableAddress,ulSmBiosLen);
	rv = CloseIMemoryInterface();
        return(rv);
}
#endif		// Linux (not WIN32)

#ifdef WIN32
extern "C"  {
#endif
///////////////////////////////////////////////////////////////////////////////
// getSmBiosRev
//     return the revision of smbios...in bcd format
//     Integer
///////////////////////////////////////////////////////////////////////////////
int getSmBiosRev()
{
        return s_iTableRev;
}


///////////////////////////////////////////////////////////////////////////////
// get_IpmiStruct
//////////////////////////////////////////////////////////////////////////////
//  Name:       get_IpmiStruct
//  Purpose:    find Type 38 record to get the IPMI Version.
//  Returns:    0 for success, or -1 if not found.
//              if success, all 4 parameters will have valid data
//////////////////////////////////////////////////////////////////////////////
int get_IpmiStruct(UCHAR *iftype, UCHAR *ver, UCHAR *sa, int *base, UCHAR *inc)
{
	UCHAR *VirtualAddress;
	ULONG SMBiosLen = 0;
	int length, j,  i;
	UCHAR Type;
	int rv = -1;

	SMBiosLen = getSmBiosTables( &VirtualAddress);
	if ((SMBiosLen == 0) || !VirtualAddress) return rv;

	for(i=0; i<(int)SMBiosLen; )
	{
		Type = VirtualAddress[i];
		length = (int)VirtualAddress[i+1];

		if (Type == 127) //end of table record
			break;
		else if (Type == 38) //Found the IPMI Device Information record
		{
			if (fsm_debug) {
			    int j;
			    printf("IPMI record: ");
			    for (j = i; j < i+length; j++)
			       printf("%02x ",VirtualAddress[j]);
			    printf("\n");
			}
			/*
			 * Byte 05h is the IPMI version as X.Y 
			 * where X is bits 7:4 and Y bits 3:0
			 *
 			 *            KCS Iv sa nv base_addr
 			 * 26 12 01 00 01 20 20 ff a3 0c 00 00 00 00 00 00 00 00
 			 *         IPMI 2.0 KCS, sa=0x20 Base=0x0ca2, spacing=1
 			 * 26 12 73 00 01 20 20 ff a9 0c 00 00 00 00 00 00 40 00
 			 *         IPMI 2.0 KCS, sa=0x20 Base=0x0ca8, spacing=4
 			 * 26 10 44 00 04 15 84 01 01 04 00 00 00 00 00 00 
 			 *         IPMI v1.5 SMBus sa=0x42, base=0x0000000401
 		 	 */
			*iftype = VirtualAddress[i+4];
			*ver    = VirtualAddress[i+5];
			*sa     = VirtualAddress[i+6];
			j       = VirtualAddress[i+8] + 
			  	  (VirtualAddress[i+9] << 8) +
				  (VirtualAddress[i+10] << 16) +
				  (VirtualAddress[i+11] << 24);
			/*if odd, then subtract 1 from base addr*/
			if (j & 0x01) j -= 1;
			*base = j;  
			/* detect Register Spacing */
			*inc = 1;  /*default*/
			if ((*iftype != 0x04) && (length >= 18)) {
		           switch(VirtualAddress[i+16] >>6) {
			    case 0x00: *inc = 1; break;   /* 1-byte bound*/
			    case 0x01: *inc = 4; break;   /* 4-byte bound*/
			    case 0x02: *inc = 16; break;  /*16-byte bound*/
			    default: break;  /**inc = 1; above*/
			  }
			}
			rv = 0;
			break;
		}
		
		//Skip this record and go to the end of it.
		for (j = i+length; j < (int)SMBiosLen; j++) {
			if (VirtualAddress[j] == 0x00 && 
			    VirtualAddress[j+1] == 0x00) {
				i = j+2;
				break;
			}
		}
	}
	closeSmBios(VirtualAddress,SMBiosLen);
	return rv;
} //getIPMI_struct

///////////////////////////////////////////////////////////////////////////////
// get_MemDesc
//////////////////////////////////////////////////////////////////////////////
//  Name:       get_MemDesc
//  Purpose:    find Type 17 record to get the Memory Locator Description
//  Returns:    0 for success, or -1 if not found.
//              if success, the desc string will have valid data
//////////////////////////////////////////////////////////////////////////////
int get_MemDesc(UCHAR array, UCHAR dimm, char *desc, int *psize)
{
	UCHAR *VirtualAddress;
	ULONG SMBiosLen = 0;
	int length, j,  i;
	int iarray, idimm;
	UCHAR Type;
	int rv = -1;
	char dimmstr[32];
	char bankstr[32];

	SMBiosLen = getSmBiosTables( &VirtualAddress);
	if ((SMBiosLen == 0) || !VirtualAddress) return rv;
	if (desc == NULL) return(-1);
	bankstr[0] = 0;
	dimmstr[0] = 0;

	if (fsm_debug) printf("get_MemDesc(%d,%d)\n",array,dimm);
	iarray = 0;
	idimm = 0;
	for(i=0; i<(int)SMBiosLen; )
	{
		Type = VirtualAddress[i];
		length = (int)VirtualAddress[i+1];

		if (Type == 127) //end of table record
			break;
		else if (Type == 16) //Found a Memory Array */
		{
		    /* usually only one onboard memory array */
		    if (array == iarray) ; /*match*/
		    else iarray++;
		}
		else if (Type == 17) //Found a Memory Device */
		{
		   int bank, k, n, nStr, nBLoc, sz;
		   if (dimm == idimm) 
		   {
			if (fsm_debug) {
			      printf("Memory record %d.%d: ",iarray,idimm);
			      for (j = i; j < (i+length+16); j++) {
				 if (((j-i) % 16) == 0) printf("\n");
			         printf("%02x ",VirtualAddress[j]);
			      }
			      printf("\n");
		   	}
			/*
			 * Memory Device record
			 * walk through strings to find Locator
 		 	 */
			sz = VirtualAddress[i+12] + (VirtualAddress[i+13] << 8); /*Size*/
			bank = VirtualAddress[i+15]; /*Set*/
			nStr = VirtualAddress[i+16]; /*Locator string num*/
			nBLoc = VirtualAddress[i+17]; /*BankLocator string num*/
			if (fsm_debug) 
				printf("bank=%d nStr=%d sz=%x\n",bank,nStr,sz);
			k = i + length;  /*hdr len (usu 0x1B)*/
			n = 1;  /* string number index */
			for (j = k; j < (int)SMBiosLen; j++) {
			   if (VirtualAddress[j] == 0x00 && 
			       VirtualAddress[j-1] == 0x00) break;
			   else if (VirtualAddress[j] == 0x00) {
			      if (fsm_debug) printf("str[%d] = %s\n",
						    n,&VirtualAddress[k]);
			      if (n == nBLoc) { /*bank string*/
			          strcpy(bankstr,(char *)&VirtualAddress[k]);
				  break; /*string number*/
			      }
			      if (n == nStr) {
			          strcpy(dimmstr,(char *)&VirtualAddress[k]);
			      }
			      n++; k = j + 1; 
			   }
			}
			if ((desc != NULL) && (j < (int)SMBiosLen)) {
			   sprintf(desc,"%s/%s",bankstr,dimmstr);
			   rv = 0;
			} else {  /* have header, but not string */
			   char b;
			   if ((idimm % 2) == 0) b = 'A';
			   else b = 'B';
			   sprintf(desc,"DIMM%d%c",bank,b);
			   rv = 0;
			}
			*psize = sz;
			break;
		   } else idimm++; /*else go to next dimm*/
		}
		
		//Skip this record and go to the end of it.
		for (j = i+length; j < (int)SMBiosLen; j++) {
			if (VirtualAddress[j] == 0x00 && 
			    VirtualAddress[j+1] == 0x00) {
				i = j+2;
				break;
			}
		}
	}
	closeSmBios(VirtualAddress,SMBiosLen);
	/* fill in a default if error */
	if ((rv != 0) && (desc != NULL)) sprintf(desc,"DIMM(%d)",dimm);
	return rv;
} //get_MemDesc

///////////////////////////////////////////////////////////////////////////////
// get_BiosVersion
//////////////////////////////////////////////////////////////////////////////
//  Name:       get_BiosVersion
//  Purpose:    finds Type 0 record and gets the BIOS ID string
//  Returns:    -1 if not found and 0 if found. Param contains the string
//////////////////////////////////////////////////////////////////////////////
int get_BiosVersion(char *bios_str)
{
#define BIOS_VERSION	0x05	//Specifies string number of BIOS Ver string
	UCHAR *VirtualAddress;
	ULONG SMBiosLen = 0;
	int recLength, j,  i, k = 0, str_num;
	UCHAR recType;
	int rv = -1;

	SMBiosLen = getSmBiosTables( &VirtualAddress);
	if ((SMBiosLen == 0) || !VirtualAddress) return rv;
	i=0; 
	while (i<(int)SMBiosLen)
	{
		recType = VirtualAddress[i];
		recLength = (int)VirtualAddress[i+1];

		// end of table record, error 
		if (recType == 127) return -1;
		else if (recType == 0) 
		{ 	// BIOS Information (Type 0) record found
			//Set index j to the end of the formated part of record.
			j = i + recLength;

			//First grab string number of the BIOS Version string
			//This non-zero number specifies which string at the end
			//of the record is the BIOS Version string. 
			str_num = VirtualAddress[i + BIOS_VERSION];

			//Now skip over strings at the end of the record till we
			//get to the BIOS Version string 
			for (str_num--;str_num > 0; str_num--)
			{
				//Skipping over one string at a time
				for (; VirtualAddress[j] != 0x00; j++);
				j++; //Advance index past EOS.
			}				

			//Copy the BIOS Version string into buffer from caller.
			for (; VirtualAddress[j] != 0x00; j++)
			{
				bios_str[k++] = VirtualAddress[j];
			}				
			bios_str[k] = '\0';
			rv = 0;
			break;  /*exit while loop*/
		} else {
			//Not a Type 0, so skip to the end of this structure
			for (j = i+recLength; j < (int)SMBiosLen; j++)
			{
				if (VirtualAddress[j] == 0x00 && 
				    VirtualAddress[j+1] == 0x00)
				{
					i = j+2;
					break;
				}
			}
		}	
	} /*end while*/
	closeSmBios(VirtualAddress,SMBiosLen);
	return rv;
}

int get_ChassisSernum(char *chs_str, char fdbg)
{
#define CHASSIS_SERNUM	0x07	//Specifies string number of BIOS Ver string
	UCHAR *VirtualAddress;
	ULONG SMBiosLen = 0;
	int recLength, j,  i, k = 0, str_num;
	UCHAR recType;
	int rv = -1;

	SMBiosLen = getSmBiosTables( &VirtualAddress);
	if ((SMBiosLen == 0) || !VirtualAddress) return rv;
	i=0; 
	while (i<(int)SMBiosLen)
	{
		recType = VirtualAddress[i];
		recLength = (int)VirtualAddress[i+1];
		if (recType == 127) return -1; // end of table record, error 
		else if (recType == 3) 
		{ 	// Chassis Information (Type 3) record found
			j = i + recLength;
			str_num = VirtualAddress[i + CHASSIS_SERNUM];
			for (str_num--;str_num > 0; str_num--)
			{
				for (; VirtualAddress[j] != 0x00; j++);
				j++; 
			}
			for (; VirtualAddress[j] != 0x00; j++)
			{ chs_str[k++] = VirtualAddress[j]; }
			chs_str[k] = '\0';
			/* also copy Chassis Manuf */
			j = i + recLength;
			memcpy(&chs_str[k+1],&VirtualAddress[j],20);
			rv = 0;
			break;  /*exit while loop*/
		} else {
			//Not a Type 0, so skip to the end of this structure
			for (j = i+recLength; j < (int)SMBiosLen; j++)
			{
				if (VirtualAddress[j] == 0x00 && 
				    VirtualAddress[j+1] == 0x00)
				{
					i = j+2;
					break;
				}
			}
		}
	} /*end while*/
	closeSmBios(VirtualAddress,SMBiosLen);
	return rv;
}

int get_SystemGuid(UCHAR *guid)
{
	UCHAR *VirtualAddress;
	ULONG SMBiosLen = 0;
	int recLength, i, j,  n, k = 0;
	UCHAR recType;
	int rv = -1;

	SMBiosLen = getSmBiosTables( &VirtualAddress);
	if ((SMBiosLen == 0) || !VirtualAddress) return rv;
	i=0; 
	while (i<(int)SMBiosLen)
	{
		recType = VirtualAddress[i];
		recLength = (int)VirtualAddress[i+1];

		// end of table record, error 
		if (recType == 127) return -1;
		else if (recType == 1) 
		{ 	// System Board Information (Type 1) record found
			//Set index j to the end of the formated part of record.
			j = i + recLength;
			n  = i + 8;  /*UUID offset 8*/
			//Copy the UUID string into buffer from caller.
			for (k = 0; k < 16; k++)
			{
				guid[k] = VirtualAddress[n + k];
			}				
			rv = 0;
			break;  /*exit while loop*/
		} else {
			//else skip to the end of this structure
			for (j = i+recLength; j < (int)SMBiosLen; j++)
			{
				if (VirtualAddress[j] == 0x00 && 
				    VirtualAddress[j+1] == 0x00)
				{
					i = j+2;
					break;
				}
			}
		}	
	} /*end while*/
	closeSmBios(VirtualAddress,SMBiosLen);
	return rv;
}
#ifdef WIN32
}
#endif


#ifdef COMP_BIN
int
main(int argc, char **argv)
{
   char biosver[80];
   UCHAR ifs,ver,sa,inc;
   int rv;
   int base;

   biosver[0] = 0;
   rv = get_BiosVersion(biosver);
   printf("get_BiosVersion rv=%d Ver=%s\n",rv,biosver);

   rv = get_IpmiStruct( &ifs, &ver, &sa, &base, &inc);
   printf("get_IpmiStruct rv=%d if=%02x ver=%02x sa=%02x base=%x, spacing=%d\n",
	  rv, ifs,ver,sa,base,inc);

   rv = get_SystemGuid(biosver);
   printf("get_SystemGuid rv=%d Ver=%s\n",rv,biosver);
   // exit(rv);
   return(rv);
}
#endif

/* end mem_if.c */
