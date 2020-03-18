/*
 * oem_sun.h
 * Handle Sun OEM command functions
 *
 * Change history:
 *  09/02/2010 ARCress - included in source tree
 *
 *---------------------------------------------------------------------
 */
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

#ifndef IPMI_SUNOEM_H
#define IPMI_SUNOEM_H


#define IPMI_NETFN_SUNOEM		0x2e

#define IPMI_SUNOEM_SET_SSH_KEY		0x01
#define IPMI_SUNOEM_DEL_SSH_KEY		0x02
#define IPMI_SUNOEM_GET_HEALTH_STATUS	0x10
#define IPMI_SUNOEM_SET_FAN_SPEED	0x20
#define IPMI_SUNOEM_LED_GET		0x21
#define IPMI_SUNOEM_LED_SET		0x22

#define SDR_RECORD_TYPE_ENTITY_ASSOC		0x08
#define SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR  0x10

//struct valstr {
        //uint16_t val;
        //const char * str;
//};
// const char * val2str(ushort val, const struct valstr *vs);
ushort str2val(const char *str, const struct valstr *vs);

#pragma pack(1)
struct entity_id {
        uint8_t id;                     /* physical entity id */
#if WORDS_BIGENDIAN
        uint8_t logical     : 1;        /* physical/logical */
        uint8_t instance    : 7;        /* instance number */
#else
        uint8_t instance    : 7;        /* instance number */
        uint8_t logical     : 1;        /* physical/logical */
#endif
}; // __attribute__ ((packed));

struct sdr_record_generic_locator {
	uint8_t dev_access_addr;
	uint8_t dev_slave_addr;
#if WORDS_BIGENDIAN
	uint8_t channel_num:3;
	uint8_t lun:2;
	uint8_t bus:3;
#else
	uint8_t bus:3;
	uint8_t lun:2;
	uint8_t channel_num:3;
#endif
#if WORDS_BIGENDIAN
	uint8_t addr_span:3;
	uint8_t __reserved1:5;
#else
	uint8_t __reserved1:5;
	uint8_t addr_span:3;
#endif
	uint8_t __reserved2;
	uint8_t dev_type;
	uint8_t dev_type_modifier;
	struct entity_id entity;
	uint8_t oem;
	uint8_t id_code;
	uint8_t id_string[16];
}; // __attribute__ ((packed));

struct sdr_record_entity_assoc {
	struct entity_id entity;	/* container entity ID and instance */
	struct {
#if WORDS_BIGENDIAN
		uint8_t isrange:1;
		uint8_t islinked:1;
		uint8_t isaccessable:1;
		uint8_t __reserved:5;
#else
		uint8_t __reserved:5;
		uint8_t isaccessable:1;
		uint8_t islinked:1;
		uint8_t isrange:1;
#endif
	} flags;
	uint8_t entity_id_1;	/* entity ID 1    |  range 1 entity */
	uint8_t entity_inst_1;	/* entity inst 1  |  range 1 first instance */
	uint8_t entity_id_2;	/* entity ID 2    |  range 1 entity */
	uint8_t entity_inst_2;	/* entity inst 2  |  range 1 last instance */
	uint8_t entity_id_3;	/* entity ID 3    |  range 2 entity */
	uint8_t entity_inst_3;	/* entity inst 3  |  range 2 first instance */
	uint8_t entity_id_4;	/* entity ID 4    |  range 2 entity */
	uint8_t entity_inst_4;	/* entity inst 4  |  range 2 last instance */
}; // __attribute__ ((packed));
#pragma pack()

int ipmi_sunoem_main(void *, int, char **);
int sunoem_led_get(void * intf, uchar * dev, uchar ledtype, uchar *prsp);
int sunoem_led_set(void * intf, uchar * dev, uchar ledtype, uchar ledmode);
int decode_sensor_sun(uchar *sdr,uchar *reading,char *pstring, int slen);

#endif /*IPMI_SUNOEM_H*/

