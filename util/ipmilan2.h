/***********************************************
 * ipmilan2.h 
 *
 * Definitions and data structures for the
 * IPMI 2.0 LAN interface
 *
 ***********************************************/
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
#ifndef IPMILAN2_H_
#define IPMILAN2_H_

#include "ipmilan.h"

#define IPMI_AUTH_RAKP_NONE  0x00
#define IPMI_INTEGRITY_NONE  0x00
#define IPMI_CRYPT_NONE      0x00

int ipmi_open_lan2(char *node, char *user, char *pswd, int fdebugcmd);
int ipmi_close_lan2(char *node);
int ipmi_cmd_lan2(char *node, ushort cmd, uchar *pdata, int sdata,
                uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
int ipmi_cmdraw_lan2(char *node, uchar cmd, uchar netfn, uchar lun, uchar sa,
		uchar bus, uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd);
int ipmicmd_lan2(char *node, uchar cmd, uchar netfn, uchar lun, uchar sa,
		uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd);
int ipmi_cmd_ipmb2(uchar cmd, uchar netfn, uchar sa, uchar bus, uchar lun,
                uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd);


#endif	// IPMILAN2_H_
