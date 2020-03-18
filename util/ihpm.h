/*
 * Copyright (c) 2003 Sun Microsystems, Inc.  All Rights Reserved.
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

#ifndef IPMI_HPMFWUPG_H
#define IPMI_HPMFWUPG_H

#define IPMI_BUF_SIZE  1024
#ifdef HAVE_LANPLUS
#include "../lib/lanplus/lanplus_defs.h"
#else
#define _IPMI_RS_
struct ipmi_rs {
        uint8_t ccode;
        uint8_t data[IPMI_BUF_SIZE];

        /*
         * Looks like this is the length of the entire packet, including the RMC
P
         * stuff, then modified to be the length of the extra IPMI message data
         */
        int data_len;

        struct {
                uint8_t netfn;
                uint8_t cmd;
                uint8_t seq;
                uint8_t lun;
        } msg;
        struct {
                uint8_t authtype;
                uint32_t seq;
                uint32_t id;
                uint8_t bEncrypted;     /* IPMI v2 only */
                uint8_t bAuthenticated; /* IPMI v2 only */
                uint8_t payloadtype;    /* IPMI v2 only */
                uint16_t msglen;
        } session;
};
#endif

#define TRUE   1
#define FALSE  0

#define IPMI_NETFN_APP           0x06
#define IPMI_NETFN_PICMG         0x2C

#define IPMI_BMC_SLAVE_ADDR             0x20

#define BMC_GET_DEVICE_ID               0x1

#define IPMI_CC_OK                0x00
#define IPMI_CC_REQ_DATA_INV_LENGTH                0xc7

#define ATTRIBUTE_PACKING /* */
#define PRAGMA_PACK  1

#pragma pack(1)
struct ipm_devid_rsp {
        uint8_t device_id;
        uint8_t device_revision;
        uint8_t fw_rev1;
        uint8_t fw_rev2;
        uint8_t ipmi_version;
        uint8_t adtl_device_support;
        uint8_t manufacturer_id[3];
        uint8_t product_id[2];
        uint8_t aux_fw_rev[4];
};
#pragma pack()

typedef struct md5_state_s {
    uint  count[2];        /* message length in bits, lsw first */
    uint  abcd[4];         /* digest buffer */
    uchar buf[64];         /* accumulate block */
} md5_state_t;

void md5_init(md5_state_t *pms);  /*md5.c*/
void md5_append(md5_state_t *pms, const uchar *data, int nbytes); /*md5.c*/
void md5_finish(md5_state_t *pms, uchar digest[16]); /*md5.c*/

void lprintf(int level, const char * format, ...); /*ipmilanplus.c*/
void set_loglevel(int level);
char * get_mfg_str(uchar *rgmfg, int *pmfg);
ushort buf2short(uchar * buf);

int ipmi_hpmfwupg_main(void *, int, char **);

#endif /* IPMI_KFWUM_H */
