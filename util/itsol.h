/*
 * Copyright (c) 2005 Tyan Computer Corp.  All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistribution of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * Redistribution in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of Sun Microsystems, Inc. or the names of
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * This software is provided "AS IS," without a warranty of any kind.
 * ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED.
 * SUN MICROSYSTEMS, INC. ("SUN") AND ITS LICENSORS SHALL NOT BE LIABLE
 * FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING
 * OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.  IN NO EVENT WILL
 * SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA,
 * OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
 * PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY OF
 * LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef IPMI_TSOL_H
#define IPMI_TSOL_H

// #include <ipmitool/ipmi.h>

#define IPMI_TSOL_CMD_SENDKEY	0x03
#define IPMI_TSOL_CMD_START	0x06
#define IPMI_TSOL_CMD_STOP	0x02
#define IPMI_NETFN_TSOL         0x30

#define IPMI_TSOL_DEF_PORT	6230

#define IPMI_BUF_SIZE   1024   /* for ipmi_rs in defs.h */

#define MIN(x,y) (((x) < (y)) ? (x) : (y))

#ifdef HAVE_IPV6
#define SOCKADDR_T  struct sockaddr_storage
#else
#define SOCKADDR_T  struct sockaddr_in
#endif
int get_LastError( void );  /* ipmilan.c */
void show_LastError(char *tag, int err);
void lprintf(int level, const char * format, ...); /*subs.c*/
int  lan_keepalive(int type);  /*ipmilan.c*/
void set_loglevel(int level);
void lperror(int level, const char * format, ...);
int ipmi_open(char fdebugcmd);
int open_sockfd(char *node, SockType *sfd, SOCKADDR_T  *daddr,
                       int *daddr_len, int foutput);

int ipmi_tsol_main(void *, int, char **);

#endif /* IPMI_TSOL_H */
