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

#ifndef IPMI_KFWUM_H
#define IPMI_KFWUM_H

// #include <inttypes.h>
// #include <ipmitool/ipmi.h>

#define uint8_t   unsigned char
#define uint16_t  unsigned short
#define uint32_t  unsigned int

#define IPMI_NETFN_APP                  0x6
#define IPMI_NETFN_FIRMWARE             0x8

#define IPMI_BMC_SLAVE_ADDR             0x20

#define BMC_GET_DEVICE_ID               0x1 

#define IPM_DEV_MANUFACTURER_ID(x) \
        ((uint32_t) ((x[2] & 0x0F) << 16 | x[1] << 8 | x[0]))

//struct valstr {
 //       uint16_t val;
  //      const char * str;
//};

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
}; // __attribute__ ((packed));
#pragma pack()

/* routines from lib/lanplus/helper.c */
uint16_t buf2short(uint8_t * buf);  
//const char * val2str(uint16_t val, const struct valstr * vs);

#endif /* IPMI_KFWUM_H */
