/***********************************************
 * ipmilan.h 
 *
 * Definitions and data structures for the
 * IPMI LAN interface
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
#ifndef IPMILAN_H_
#define IPMILAN_H_

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN    64
#endif
#define SOL_HDR_LEN   	30  /*SOL 1.5 HDR: 4(rmcp) + 9 + 1 + 5(sol) */
#define RQ_HDR_LEN   	30
#define RQ_LEN_MAX   	200  /*see IPMI_REQBUF_SIZE, was 25 */
#define RS_LEN_MAX   	200  /*see IPMI_RSPBUF_SIZE */
/* Note that the send buffer is [RQ_LEN_MAX+RQ_HDR_LEN+7]    =  62 */
/* Note that the receive buffer is [RS_LEN_MAX+RQ_HDR_LEN+7] = 237 */
#define SEND_BUF_SZ     (RQ_LEN_MAX+RQ_HDR_LEN+7+8+8)  /*RQ_LEN+53= 78*/
#define RECV_BUF_SZ     (RS_LEN_MAX+RQ_HDR_LEN+7+8+8)  /*RS_LEN+53=253*/
/* #define IPMI_LAN_SEQ_NUM_MAX    0x3F  * only for gnulan */
#define RMCP_PRI_RMCP_PORT     0x26F

#define PAYLOAD_TYPE_SOL    0x01 

/* IPMI commands used for LAN sessions */
#define CMD_GET_CHAN_AUTH_CAP      0x38
#define CMD_GET_SESSION_CHALLENGE  0x39
#define CMD_ACTIVATE_SESSION       0x3A
#define CMD_SET_SESSION_PRIV       0x3B
#define CMD_CLOSE_SESSION          0x3C
#define CMD_GET_MESSAGE 	   0x33
#define CMD_SEND_MESSAGE	   0x34

/* see ipmicmd.h for LAN_ERR definitions */

int get_LastError( void );
void show_LastError(char *tag, int err);
void ipmi_lan_set_timeout(int ipmito, int tries, int pingto);
int ipmi_open_lan(char *node, int port, char *user, char *pswd, int fdebugcmd);
int ipmi_close_lan(char *node);
int ipmi_flush_lan(char *node);
int ipmi_cmd_lan(char *node, ushort cmd, uchar *pdata, int sdata,
                uchar *presp, int *sresp, uchar *pcc, char fdebugcmd);
int ipmi_cmdraw_lan(char *node, uchar cmd, uchar netfn, uchar lun, uchar sa,
		uchar bus, uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd);
int ipmicmd_lan(char *node, uchar cmd, uchar netfn, uchar lun, uchar sa, 
		uchar bus, uchar *pdata, int sdata, uchar *presp, int *sresp, 
		uchar *pcc, char fdebugcmd);
int ipmi_cmd_ipmb(uchar cmd, uchar netfn, uchar sa, uchar bus, uchar lun,
                uchar *pdata, int sdata, uchar *presp,
                int *sresp, uchar *pcc, char fdebugcmd);

#endif	// IPMILAN_H_
