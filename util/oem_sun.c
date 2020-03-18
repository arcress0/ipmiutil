/*
 * oem_sun.c
 * Handle Sun OEM command functions
 *
 * Change history:
 *  09/02/2010 ARCress - included in source tree
 *
 *---------------------------------------------------------------------
 */
/*
 * Copyright (c) 2005 Sun Microsystems, Inc.  All Rights Reserved.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#define uint8_t  unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#include "getopt.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <limits.h>
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#include <stdint.h>
#else
#include <getopt.h>
#endif
#endif

#ifndef ULONG_MAX
/*needed for WIN32*/
#define ULONG_MAX    4294967295UL
#define UCHAR_MAX     255
#endif

#include "ipmicmd.h"
#include "isensor.h"
#include "oem_sun.h"

extern int  verbose; /*ipmilanplus.c*/
extern void lprintf(int level, const char * format, ...); /*ipmilanplus.c*/
extern void set_loglevel(int level);

static const struct valstr sunoem_led_type_vals[] = {
	{ 0, "OK2RM" },
	{ 1, "SERVICE" },
	{ 2, "ACT" },
	{ 3, "LOCATE" },
	{ 0xFF, NULL }
};

static const struct valstr sunoem_led_mode_vals[] = {
	{ 0, "OFF" },
	{ 1, "ON" },
	{ 2, "STANDBY" },
	{ 3, "SLOW" },
	{ 4, "FAST" },
	{ 0xFF, NULL }
};
static const struct valstr sunoem_led_mode_optvals[] = {
	{ 0, "STEADY_OFF" },
	{ 1, "STEADY_ON" },
	{ 2, "STANDBY_BLINK" },
	{ 3, "SLOW_BLINK" },
	{ 4, "FAST_BLINK" },
	{ 0xFF, NULL }
};

/* global variables */
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil sunoem";
#else
static char * progver   = "3.08";
static char * progname  = "isunoem";
#endif
static char   fdebug    = 0;
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;
static int is_sbcmd = 0;
static int csv_output = 0; 
static uchar *sdrcache = NULL;
static int vend_id = 0;
static int prod_id = 0;

static void
ipmi_sunoem_usage(void)
{
	lprintf(LOG_NOTICE, "usage: sunoem <command> [option...]");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "   fan speed <0-100>");
	lprintf(LOG_NOTICE, "      Set system fan speed (PWM duty cycle)");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "   sshkey set <userid> <id_rsa.pub>");
	lprintf(LOG_NOTICE, "      Set ssh key for a userid into authorized_keys,");
	lprintf(LOG_NOTICE, "      view users with 'user list' command.");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "   sshkey del <userid>");
	lprintf(LOG_NOTICE, "      Delete ssh key for userid from authorized_keys,");
	lprintf(LOG_NOTICE, "      view users with 'user list' command.");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "   led get <sensorid> [ledtype]");
	lprintf(LOG_NOTICE, "      Read status of LED found in Generic Device Locator.");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "   led set <sensorid> <ledmode> [ledtype]");
	lprintf(LOG_NOTICE, "      Set mode of LED found in Generic Device Locator.");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "   sbled get <sensorid> [ledtype]");
	lprintf(LOG_NOTICE, "      Read status of LED found in Generic Device Locator");
	lprintf(LOG_NOTICE, "      for Sun Blade Modular Systems.");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "   sbled set <sensorid> <ledmode> [ledtype]");
	lprintf(LOG_NOTICE, "      Set mode of LED found in Generic Device Locator");
	lprintf(LOG_NOTICE, "      for Sun Blade Modular Systems.");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "      Use 'sdr list generic' command to get list of Generic");
	lprintf(LOG_NOTICE, "      Devices that are controllable LEDs.");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "      Required SIS LED Mode:");
	lprintf(LOG_NOTICE, "         OFF          Off");
	lprintf(LOG_NOTICE, "         ON           Steady On");
	lprintf(LOG_NOTICE, "         STANDBY      100ms on 2900ms off blink rate");
	lprintf(LOG_NOTICE, "         SLOW         1HZ blink rate");
	lprintf(LOG_NOTICE, "         FAST         4HZ blink rate");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "      Optional SIS LED Type:");
	lprintf(LOG_NOTICE, "         OK2RM        OK to Remove");
	lprintf(LOG_NOTICE, "         SERVICE      Service Required");
	lprintf(LOG_NOTICE, "         ACT          Activity");
	lprintf(LOG_NOTICE, "         LOCATE       Locate");
	lprintf(LOG_NOTICE, "");
}

/* 
 * IPMI Request Data: 1 byte
 *
 * [byte 0]  FanSpeed    Fan speed as percentage
 */
static int
ipmi_sunoem_fan_speed(void * intf, uint8_t speed)
{
	uchar rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	/* 
	 * sunoem fan speed <percent>
	 */

	if (speed > 100) {
		lprintf(LOG_NOTICE, "Invalid fan speed: %d", speed);
		return -1;
	}

	req.msg.netfn = IPMI_NETFN_SUNOEM;
	req.msg.cmd = IPMI_SUNOEM_SET_FAN_SPEED;
	req.msg.data = &speed;
	req.msg.data_len = 1;
		
	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv < 0) {
		lprintf(LOG_ERR, "Sun OEM Set Fan Speed command failed");
		return (rv);
	} else if (rv > 0) {
		lprintf(LOG_ERR, "Sun OEM Set Fan Speed command failed: %s",
			decode_cc(0,rv));
		return (rv);
	}

	printf("Set Fan speed to %d%%\n", speed);

	return rv;
}


static void
led_print(const char * name, uint8_t state)
{
	if (csv_output)
		printf("%s,%s\n", name, val2str(state, sunoem_led_mode_vals));
	else
		printf("%-16s | %s\n", name, val2str(state, sunoem_led_mode_vals));
}

int
sunoem_led_get(void * intf, uchar *sdr, uchar ledtype, uchar *prsp)
{
	uchar rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;
	uint8_t rqdata[7];
	int rqdata_len = 5;
	struct sdr_record_generic_locator * dev;

	if (sdr == NULL) return -1;
	dev = (struct sdr_record_generic_locator *)&sdr[5];

	rqdata[0] = dev->dev_slave_addr;
	if (ledtype == 0xFF)
		rqdata[1] = dev->oem;
	else
		rqdata[1] = ledtype;
	rqdata[2] = dev->dev_access_addr;
	rqdata[3] = dev->oem;
	if (is_sbcmd) {
		rqdata[4] = dev->entity.id;
		rqdata[5] = dev->entity.instance;
		rqdata[6] = 0;
		rqdata_len = 7;
	}
	else {
		rqdata[4] = 0;
	}

	req.msg.netfn = IPMI_NETFN_SUNOEM;
	req.msg.cmd = IPMI_SUNOEM_LED_GET;
	req.msg.lun = dev->lun;
	req.msg.data = rqdata;
	req.msg.data_len = (uchar)rqdata_len;
		
	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv < 0) {
		lprintf(LOG_ERR, "Sun OEM Get LED command failed");
		return rv;
	}
	else if (rv > 0) {
		lprintf(LOG_ERR, "Sun OEM Get LED command failed: %s",
			decode_cc(0,rv));
		return rv;
	}
	if (prsp != NULL) memcpy(prsp,rsp,rsp_len);
	if (rsp_len != 1) {
		lprintf(LOG_ERR, "Sun OEM Get LED command error len=%d",
			rsp_len);
		return -1;
	}
	return rv;
}

int
sunoem_led_set(void * intf, uchar *sdr, uchar ledtype, uchar ledmode)
{
	uchar rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;
	uint8_t rqdata[9];
	int rqdata_len = 7;
	struct sdr_record_generic_locator * dev;

	if (sdr == NULL) return -1;
	dev = (struct sdr_record_generic_locator *)&sdr[5];

	rqdata[0] = dev->dev_slave_addr;
	if (ledtype == 0xFF)
		rqdata[1] = dev->oem;
	else
		rqdata[1] = ledtype;
	rqdata[2] = dev->dev_access_addr;
	rqdata[3] = dev->oem;
	rqdata[4] = ledmode;
	if (is_sbcmd) {
		rqdata[5] = dev->entity.id;
		rqdata[6] = dev->entity.instance;
		rqdata[7] = 0;
		rqdata[8] = 0;
		rqdata_len = 9;
	}
	else {
		rqdata[5] = 0;
		rqdata[6] = 0;
	}

	req.msg.netfn = IPMI_NETFN_SUNOEM;
	req.msg.cmd = IPMI_SUNOEM_LED_SET;
	req.msg.lun = dev->lun;
	req.msg.data = rqdata;
	req.msg.data_len = (uchar)rqdata_len;
		
	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv < 0) {
		lprintf(LOG_ERR, "Sun OEM Set LED command failed");
	} else if (rv > 0) {
		lprintf(LOG_ERR, "Sun OEM Set LED command failed: %s",
			decode_cc(0,rv));
	}
	return rv;
}

static void
sunoem_led_get_byentity(void * intf, uchar entity_id,
			  uchar entity_inst, uchar ledtype)
{
	uchar rsp[IPMI_RSPBUF_SIZE];
	int  rv;
	uchar *elist;
	struct entity_id entity;
	struct sdr_record_generic_locator *dev;
	uchar sdr[IPMI_RSPBUF_SIZE];
	ushort id;

	if (entity_id == 0)
		return;

	/* lookup sdrs with this entity */
	memset(&entity, 0, sizeof(struct entity_id));
	entity.id = entity_id;
	entity.instance = entity_inst;

	if (sdrcache == NULL) rv = get_sdr_cache(&elist);
	else elist = sdrcache;
	id = 0;
	/* for each generic sensor set its led state */
	while(find_sdr_next(sdr,elist,id) == 0) {
		id = sdr[0] + (sdr[1] << 8);
		if (sdr[3] != SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR)
			continue;
		dev = (struct sdr_record_generic_locator *)&sdr[5];
		if ((dev->entity.id == entity_id) &&
		    (dev->entity.instance == entity_inst)) {
		  rv = sunoem_led_get(intf, sdr, ledtype, rsp);
		  if (rv == 0) {
			led_print((const char *)dev->id_string, rsp[0]);
		  }
		} 
	}

	if (sdrcache == NULL) free_sdr_cache(elist);
}

static void
sunoem_led_set_byentity(void * intf, uchar entity_id,
			  uchar entity_inst, uchar ledtype, uchar ledmode)
{
	int  rv;
	uchar *elist;
	struct entity_id entity;
	struct sdr_record_generic_locator * dev;
	uchar sdr[IPMI_RSPBUF_SIZE];
	ushort id;

	if (entity_id == 0)
		return;

	/* lookup sdrs with this entity */
	memset(&entity, 0, sizeof(struct entity_id));
	entity.id = entity_id;
	entity.instance = entity_inst;

	if (sdrcache == NULL) rv = get_sdr_cache(&elist);
	else elist = sdrcache;
	id = 0;
	/* for each generic sensor set its led state */
	while(find_sdr_next(sdr,elist,id) == 0) {
		id = sdr[0] + (sdr[1] << 8);
		if (sdr[3] != SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR)
			continue;
		/* match entity_id and entity_inst */
		dev = (struct sdr_record_generic_locator *)&sdr[5];
		if ((dev->entity.id == entity_id) &&
		    (dev->entity.instance == entity_inst)) {
		   rv = sunoem_led_set(intf, sdr, ledtype, ledmode);
		   if (rv == 0) {
			led_print((const char *)dev->id_string, ledmode);
		   }
		}
	}

	if (sdrcache == NULL) free_sdr_cache(elist);
}

/* 
 * IPMI Request Data: 5 bytes
 *
 * [byte 0]  devAddr     Value from the "Device Slave Address" field in
 *                       LED's Generic Device Locator record in the SDR
 * [byte 1]  led         LED Type: OK2RM, ACT, LOCATE, SERVICE
 * [byte 2]  ctrlrAddr   Controller address; value from the "Device
 *                       Access Address" field, 0x20 if the LED is local
 * [byte 3]  hwInfo      The OEM field from the SDR record
 * [byte 4]  force       1 = directly access the device
 *                       0 = go thru its controller
 *                       Ignored if LED is local
 * 
 * The format below is for Sun Blade Modular systems only
 * [byte 4]  entityID    The entityID field from the SDR record
 * [byte 5]  entityIns   The entityIns field from the SDR record
 * [byte 6]  force       1 = directly access the device
 *                       0 = go thru its controller
 *                       Ignored if LED is local
 */
static int
ipmi_sunoem_led_get(void * intf,  int  argc, char ** argv)
{
	uchar rsp[IPMI_RSPBUF_SIZE];
	int  rv;
	uchar *alist;
	struct sdr_record_entity_assoc *assoc;
	struct sdr_record_generic_locator * dev;
	uchar sdr[IPMI_RSPBUF_SIZE];
	uchar sdr1[IPMI_RSPBUF_SIZE];
	ushort id;
	uchar ledtype = 0xFF;
	int i;

	/* 
	 * sunoem led/sbled get <id> [type]
	 */

	if (argc < 1 || strncmp(argv[0], "help", 4) == 0) {
		ipmi_sunoem_usage();
		return 0;
	}

	if (argc > 1) {
		ledtype = (uchar)str2val(argv[1], sunoem_led_type_vals);
		if (ledtype == 0xFF) 
			lprintf(LOG_ERR, "Unknown ledtype, will use data from the SDR oem field");
	}

	if (strncasecmp(argv[0], "all", 3) == 0) {
		/* do all generic sensors */
		lprintf(LOG_NOTICE, "Fetching SDRs ...");
		rv = get_sdr_cache(&alist);
		if (fdebug) lprintf(LOG_NOTICE, "get_sdr_cache rv = %d",rv);
		id = 0;
		rv = ERR_NOT_FOUND;
		while(find_sdr_next(sdr,alist,id) == 0) {
			id = sdr[0] + (sdr[1] << 8);
			if (sdr[3] != SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR)
				continue;
			dev = (struct sdr_record_generic_locator *)&sdr[5];
			if (dev->entity.logical) // (sdr[13] & 0x80 != 0) 
				continue;
			rv = sunoem_led_get(intf, sdr, ledtype, rsp);
			if (rv == 0) {
				led_print((const char *)dev->id_string, rsp[0]);
			}
		}
		free_sdr_cache(alist);
		return rv;
	}

	/* look up generic device locator record in SDR */
	id = (ushort)atoi(argv[0]);
	lprintf(LOG_NOTICE, "Fetching SDRs ...");
	rv = get_sdr_cache(&alist);
	if (fdebug) lprintf(LOG_NOTICE, "get_sdr_cache rv = %d",rv);
	if (rv == 0) {
	   sdrcache = alist;
	   rv = find_sdr_next(sdr1,alist,id);
	}
	if (rv != 0) {
		lprintf(LOG_ERR, "No Sensor Data Record found for %s", argv[0]);
		free_sdr_cache(alist);
		return (rv);
	}

	if (sdr1[3] != SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR) {
		lprintf(LOG_ERR, "Invalid SDR type %d", sdr1[3]);
		free_sdr_cache(alist);
		return (-1);
	}

	dev = (struct sdr_record_generic_locator *)&sdr1[5];
	if (!dev->entity.logical) {
		/*
		 * handle physical entity
		 */
		rv = sunoem_led_get(intf, sdr1, ledtype, rsp);
		if (rv == 0) {
			led_print((const char *)dev->id_string, rsp[0]);
		}
		free_sdr_cache(alist);
		return rv;
	}

	/* 
	 * handle logical entity for LED grouping
	 */

	lprintf(LOG_INFO, "LED %s is logical device", argv[0]);

	/* get entity assoc records */
	// rv = get_sdr_cache(&alist);
	id = 0;
	while(find_sdr_next(sdr,alist,id) == 0) {
		id = sdr[0] + (sdr[1] << 8);
		if (sdr[3] != SDR_RECORD_TYPE_ENTITY_ASSOC)
			continue;
		assoc = (struct sdr_record_entity_assoc *)&sdr[5];
		if (assoc == NULL) continue;
		/* check that the entity id/instance matches our generic record */
		if (assoc->entity.id != dev->entity.id ||
		    assoc->entity.instance != dev->entity.instance)
			continue;

		if (assoc->flags.isrange) {
			/*
			 * handle ranged entity associations
			 *
			 * the test for non-zero entity id is handled in
			 * sunoem_led_get_byentity()
			 */

			/* first range set - id 1 and 2 must be equal */
			if (assoc->entity_id_1 == assoc->entity_id_2)
				for (i = assoc->entity_inst_1; i <= assoc->entity_inst_2; i++)
					sunoem_led_get_byentity(intf, assoc->entity_id_1, (uchar)i, ledtype);

			/* second range set - id 3 and 4 must be equal */
			if (assoc->entity_id_3 == assoc->entity_id_4)
				for (i = assoc->entity_inst_3; i <= assoc->entity_inst_4; i++)
					sunoem_led_get_byentity(intf, assoc->entity_id_3, (uchar)i, ledtype);
		}
		else {
			/*
			 * handle entity list
			 */
			sunoem_led_get_byentity(intf, assoc->entity_id_1,
						assoc->entity_inst_1, ledtype);
			sunoem_led_get_byentity(intf, assoc->entity_id_2,
						assoc->entity_inst_2, ledtype);
			sunoem_led_get_byentity(intf, assoc->entity_id_3,
						assoc->entity_inst_3, ledtype);
			sunoem_led_get_byentity(intf, assoc->entity_id_4,
						assoc->entity_inst_4, ledtype);
		}
	}

	free_sdr_cache(alist);
	sdrcache = NULL;

	return rv;
}

/* 
 * IPMI Request Data: 7 bytes
 *
 * [byte 0]  devAddr     Value from the "Device Slave Address" field in
 *                       LED's Generic Device Locator record in the SDR
 * [byte 1]  led         LED Type: OK2RM, ACT, LOCATE, SERVICE
 * [byte 2]  ctrlrAddr   Controller address; value from the "Device
 *                       Access Address" field, 0x20 if the LED is local
 * [byte 3]  hwInfo      The OEM field from the SDR record
 * [byte 4]  mode        LED Mode: OFF, ON, STANDBY, SLOW, FAST
 * [byte 5]  force       TRUE - directly access the device
 *                       FALSE - go thru its controller
 *                       Ignored if LED is local
 * [byte 6]  role        Used by BMC for authorization purposes
 *
 * The format below is for Sun Blade Modular systems only
 * [byte 5]  entityID    The entityID field from the SDR record
 * [byte 6]  entityIns   The entityIns field from the SDR record
 * [byte 7]  force       TRUE - directly access the device
 *                       FALSE - go thru its controller
 *                       Ignored if LED is local
 * [byte 8]  role        Used by BMC for authorization purposes
 *
 *
 * IPMI Response Data: 1 byte
 * 
 * [byte 0]  mode     LED Mode: OFF, ON, STANDBY, SLOW, FAST
 */

static int
ipmi_sunoem_led_set(void * intf,  int  argc, char ** argv)
{
	int  rv;
	uchar *alist;
	struct sdr_record_entity_assoc *assoc;
	struct sdr_record_generic_locator * dev;
	uchar sdr[IPMI_RSPBUF_SIZE];
	uchar sdr1[IPMI_RSPBUF_SIZE];
	ushort id;
	uchar ledmode;
	uchar ledtype = 0xFF;
	int i;

	/* 
	 * sunoem led/sbled set <id> <mode> [type]
	 */

	if (argc < 2 || strncmp(argv[0], "help", 4) == 0) {
		ipmi_sunoem_usage();
		return 0;
	}

	i = str2val(argv[1], sunoem_led_mode_vals);
	if (i == 0xFF) {
		i = str2val(argv[1], sunoem_led_mode_optvals);
		if (i == 0xFF) {
			lprintf(LOG_NOTICE, "Invalid LED Mode: %s", argv[1]);
			return -1;
		}
	}
	ledmode = (uchar)i;

	if (argc > 3) {
		ledtype = (uchar)str2val(argv[2], sunoem_led_type_vals);
		if (ledtype == 0xFF) 
			lprintf(LOG_ERR, "Unknown ledtype, will use data from the SDR oem field");
	}

	if (strncasecmp(argv[0], "all", 3) == 0) {
		/* do all generic sensors */
		lprintf(LOG_NOTICE, "Fetching SDRs ...");
		rv = get_sdr_cache(&alist);
		if (fdebug) lprintf(LOG_NOTICE, "get_sdr_cache rv = %d",rv);
		id = 0;
		rv = ERR_NOT_FOUND;
		while(find_sdr_next(sdr,alist,id) == 0) {
			id = sdr[0] + (sdr[1] << 8);
			if (sdr[3] != SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR)
				continue;
			dev = (struct sdr_record_generic_locator *)&sdr[5];
			if ((sdr[13] & 0x80) != 0) /*logical entity*/
				continue;
			rv = sunoem_led_set(intf, sdr, ledtype, ledmode);
			if (rv == 0) {
				led_print((const char *)dev->id_string, ledmode);
			}
		}
		free_sdr_cache(alist);
		return rv;
	}

	/* look up generic device locator records in SDR */
	id = (ushort)atoi(argv[0]);
	lprintf(LOG_NOTICE, "Fetching SDRs ...");
	rv = get_sdr_cache(&alist);
	if (fdebug) lprintf(LOG_NOTICE, "get_sdr_cache rv = %d",rv);
	if (rv == 0) {
		sdrcache = alist;
		rv = find_sdr_next(sdr1,alist,id);
	}
	if (rv != 0) {
		lprintf(LOG_ERR, "No Sensor Data Record found for %s", argv[0]);
		free_sdr_cache(alist);
		return (rv);
	}

	if (sdr1[3] != SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR) {
		lprintf(LOG_ERR, "Invalid SDR type %d", sdr1[3]);
		free_sdr_cache(alist);
		return -1;
	}
	dev = (struct sdr_record_generic_locator *)&sdr1[5];

	if (!dev->entity.logical) {
		/*
		 * handle physical entity
		 */
		rv = sunoem_led_set(intf, sdr, ledtype, ledmode);
		if (rv == 0) {
			led_print(argv[0], ledmode);
		}
		free_sdr_cache(alist);
		return rv;
	}

	/* 
	 * handle logical entity for LED grouping
	 */

	lprintf(LOG_INFO, "LED %s is logical device", argv[0]);

	/* get entity assoc records */
	id = 0;
	while(find_sdr_next(sdr,alist,id) == 0) {
		id = sdr[0] + (sdr[1] << 8);
		if (sdr[3] != SDR_RECORD_TYPE_ENTITY_ASSOC)
			continue;
		assoc = (struct sdr_record_entity_assoc *)&sdr[5];
		if (assoc == NULL) continue;

		/* check that the entity id/instance matches our generic record */
		if (assoc->entity.id != dev->entity.id ||
		    assoc->entity.instance != dev->entity.instance)
			continue;

		if (assoc->flags.isrange) {
			/*
			 * handle ranged entity associations
			 *
			 * the test for non-zero entity id is handled in
			 * sunoem_led_get_byentity()
			 */

			/* first range set - id 1 and 2 must be equal */
			if (assoc->entity_id_1 == assoc->entity_id_2)
				for (i = assoc->entity_inst_1; i <= assoc->entity_inst_2; i++)
					sunoem_led_set_byentity(intf, assoc->entity_id_1, (uchar)i, ledtype, ledmode);

			/* second range set - id 3 and 4 must be equal */
			if (assoc->entity_id_3 == assoc->entity_id_4)
				for (i = assoc->entity_inst_3; i <= assoc->entity_inst_4; i++)
					sunoem_led_set_byentity(intf, assoc->entity_id_3, (uchar)i, ledtype, ledmode);
		}
		else {
			/*
			 * handle entity list
			 */
			sunoem_led_set_byentity(intf, assoc->entity_id_1,
						assoc->entity_inst_1, ledtype, ledmode);
			sunoem_led_set_byentity(intf, assoc->entity_id_2,
						assoc->entity_inst_2, ledtype, ledmode);
			sunoem_led_set_byentity(intf, assoc->entity_id_3,
						assoc->entity_inst_3, ledtype, ledmode);
			sunoem_led_set_byentity(intf, assoc->entity_id_4,
						assoc->entity_inst_4, ledtype, ledmode);
		}
	}

	free_sdr_cache(alist);
	sdrcache = NULL;

	return rv;
}

static int
ipmi_sunoem_sshkey_del(void * intf, uint8_t uid)
{
	uchar rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
	struct ipmi_rq req;

	memset(&req, 0, sizeof(struct ipmi_rq));
	req.msg.netfn = IPMI_NETFN_SUNOEM;
	req.msg.cmd = IPMI_SUNOEM_DEL_SSH_KEY;
	req.msg.data = &uid;
	req.msg.data_len = 1;

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv < 0) {
		lprintf(LOG_ERR, "Unable to delete ssh key for UID %d", uid);
		return rv;
	}
	else if (rv > 0) {
		lprintf(LOG_ERR, "Unable to delete ssh key for UID %d: %s", uid,
			decode_cc(0,rv));
		return rv;
	}

	printf("Deleted SSH key for user id %d\n", uid);
	return rv;
}

#define SSHKEY_BLOCK_SIZE	64
static int
ipmi_sunoem_sshkey_set(void * intf, uint8_t uid, char * ifile)
{
	uchar rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv = -1;
	struct ipmi_rq req;
	FILE * fp;
	size_t count;
	uint16_t i_size, r, f_size;
	uint8_t wbuf[SSHKEY_BLOCK_SIZE + 3];

	if (ifile == NULL) {
		lprintf(LOG_ERR, "Invalid or missing input filename");
		return -1;
	}

	fp = fopen(ifile, "r");
	if (fp == NULL) {
		lprintf(LOG_ERR, "Unable to open file %s for reading", ifile);
		return -1;
	}

	printf("Setting SSH key for user id %d...", uid);

	memset(&req, 0, sizeof(struct ipmi_rq));
	req.msg.netfn = IPMI_NETFN_SUNOEM;
	req.msg.cmd = IPMI_SUNOEM_SET_SSH_KEY;
	req.msg.data = wbuf;

	fseek(fp, 0, SEEK_END);
	f_size = (uint16_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);

	for (r = 0; r < f_size; r += i_size) {
		i_size = f_size - r;
		if (i_size > SSHKEY_BLOCK_SIZE)
			i_size = SSHKEY_BLOCK_SIZE;

		memset(wbuf, 0, SSHKEY_BLOCK_SIZE);
		fseek(fp, r, SEEK_SET);
		count = fread(wbuf+3, 1, i_size, fp);
		if (count != i_size) {
			lprintf(LOG_ERR, "Unable to read %d bytes from file %s", i_size, ifile);
			fclose(fp);
			return -1;
		}

		printf(".");
		fflush(stdout);

		wbuf[0] = uid;
		if ((r + SSHKEY_BLOCK_SIZE) >= f_size)
			wbuf[1] = 0xff;
		else
			wbuf[1] = (uint8_t)(r / SSHKEY_BLOCK_SIZE);
		wbuf[2] = (uint8_t)i_size;
		req.msg.data_len = i_size + 3;

		rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
		if (rv < 0) {
			lprintf(LOG_ERR, "Unable to set ssh key for UID %d", uid);
			break;
		}
	}

	printf("done\n");

	fclose(fp);
	return rv;
}


int
ipmi_sunoem_main(void * intf, int  argc, char ** argv)
{
	int rc = 0;

	if (argc == 0 || strncmp(argv[0], "help", 4) == 0) {
		ipmi_sunoem_usage();
		return 0;
	}

	if (strncmp(argv[0], "fan", 3) == 0) {
		uint8_t pct;
		if (argc < 2) {
			ipmi_sunoem_usage();
			return -1;
		}
		else if (strncmp(argv[1], "speed", 5) == 0) {
			if (argc < 3) {
				ipmi_sunoem_usage();
				return -1;
			}
			pct = atob(argv[2]);
			rc = ipmi_sunoem_fan_speed(intf, pct);
		}
		else {
			ipmi_sunoem_usage();
			return -1;
		}
	}

	if ((strncmp(argv[0], "led", 3) == 0) || (strncmp(argv[0], "sbled", 5) == 0)) {
		if (argc < 2) {
			ipmi_sunoem_usage();
			return -1;
		}
		if (strncmp(argv[0], "sbled", 5) == 0) {
			is_sbcmd = 1;
		}
		if (strncmp(argv[1], "get", 3) == 0) {
			if (argc < 3) {
				char * arg[] = { "all" };
				rc = ipmi_sunoem_led_get(intf, 1, arg);
			} else {
				rc = ipmi_sunoem_led_get(intf, argc-2, &(argv[2]));
			}
		}
		else if (strncmp(argv[1], "set", 3) == 0) {
			if (argc < 4) {
				ipmi_sunoem_usage();
				return -1;
			}
			rc = ipmi_sunoem_led_set(intf, argc-2, &(argv[2]));
		}
		else {
			ipmi_sunoem_usage();
			return -1;
		}
	}

	if (strncmp(argv[0], "sshkey", 6) == 0) {
		uint8_t uid = 0;
		unsigned long l;
		if (argc < 2) {
			ipmi_sunoem_usage();
			return -1;
		}
		else if (strncmp(argv[1], "del", 3) == 0) {
			if (argc < 3) {
				ipmi_sunoem_usage();
				return -1;
			}
			l = strtoul(argv[2], NULL, 0);
			if ((l == ULONG_MAX) || (l > UCHAR_MAX)) {
				printf("param %s is out of bounds\n",argv[2]);
				return -17; /*ERR_BAD_PARAM*/
			}
			uid = (uint8_t)l;
			rc = ipmi_sunoem_sshkey_del(intf, uid);
		}
		else if (strncmp(argv[1], "set", 3) == 0) {
			if (argc < 4) {
				ipmi_sunoem_usage();
				return -1;
			}
			l = strtoul(argv[2], NULL, 0);
			if ((l == ULONG_MAX) || (l > UCHAR_MAX)) {
				printf("param %s is out of bounds\n",argv[2]);
				return -17; /*ERR_BAD_PARAM*/
			}
			uid = (uint8_t)l;
			rc = ipmi_sunoem_sshkey_set(intf, uid, argv[3]);
		}
	}

	return rc;
}

int decode_sensor_sun(uchar *sdr,uchar *reading,char *pstring, int slen)
{
   int rv = -1;
   uchar stype;
   char *pstr = NULL;

   if (sdr == NULL || reading == NULL) return(rv);
   if (pstring == NULL || slen == 0) return(rv);
   /* sdr[3] is the SDR type: 02=Compact, 01=Full)  */
   /* Usually compact sensors here, but type 0xC0 is a full sensor */
   stype = sdr[12];
   switch(stype) {
	case 0x15:	/* Module/Board State sensor (e.g. BL0/STATE) */
	   if ((reading[1] + reading[2]) == 0) pstr = "NotAvailable"; 
	   else if (reading[2] & 0x01) pstr = "OK";   /*deasserted/OK*/
	   else pstr = "Asserted"; /*Asserted, error*/
	   rv = 0;
	   break;
	case 0xC0:	/* Blade Error sensor (e.g. BL0/ERR, a Full SDR) */
	   if ((reading[1] + reading[2]) == 0) pstr = "NotAvailable"; 
	   else if (reading[2] & 0x01) pstr = "OK";   /*deasserted/OK*/
	   else pstr = "Asserted"; /*Asserted, error*/
	   rv = 0;
	   break;
	default:
	   break;
   }
   if (rv == 0) strncpy(pstring, pstr, slen);
   return(rv);
}

#ifdef METACOMMAND
int i_sunoem(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
#endif
{
	void *intf = NULL;
	int rc = 0;
	int c, i;
	char *s1;
	uchar devrec[16];

	printf("%s ver %s\n", progname,progver);
 	set_loglevel(LOG_NOTICE);

        while ( (c = getopt( argc, argv,"m:T:V:J:EYF:P:N:R:U:Z:x?")) != EOF )
	switch (c) {
          case 'm': /* specific IPMB MC, 3-byte address, e.g. "409600" */
                    g_bus = htoi(&optarg[0]);  /*bus/channel*/
                    g_sa  = htoi(&optarg[2]);  /*device slave address*/
                    g_lun = htoi(&optarg[4]);  /*LUN*/
                    g_addrtype = ADDR_IPMB;
                    if (optarg[6] == 's') {
                             g_addrtype = ADDR_SMI;  s1 = "SMI";
                    } else { g_addrtype = ADDR_IPMB; s1 = "IPMB"; }
		    ipmi_set_mc(g_bus,g_sa,g_lun,g_addrtype);
                    printf("Use MC at %s bus=%x sa=%x lun=%x\n",
                            s1,g_bus,g_sa,g_lun);
                    break;
          case 'x': fdebug = 1; verbose = 1;
		    set_debug();
		    break;  /* debug messages */
          case 'N':    /* nodename */
          case 'U':    /* remote username */
          case 'P':    /* remote password */
          case 'R':    /* remote password */
          case 'E':    /* get password from IPMI_PASSWORD environment var */
          case 'F':    /* force driver type */
          case 'T':    /* auth type */
          case 'J':    /* cipher suite */
          case 'V':    /* priv level */
          case 'Y':    /* prompt for remote password */
          case 'Z':    /* set local MC address */
                parse_lan_options(c,optarg,fdebug);
                break;
          case '?': 
		ipmi_sunoem_usage();
		return 0;
                break;
	}
        for (i = 0; i < optind; i++) { argv++; argc--; }

        rc = ipmi_getdeviceid( devrec, sizeof(devrec),fdebug);
	if (rc == 0) {
		char ipmi_maj, ipmi_min;
		ipmi_maj = devrec[4] & 0x0f;
		ipmi_min = devrec[4] >> 4;
		vend_id = devrec[6] + (devrec[7] << 8) + (devrec[8] << 16);
		prod_id = devrec[9] + (devrec[10] << 8);
		show_devid( devrec[2],  devrec[3], ipmi_maj, ipmi_min);
	
		rc = ipmi_sunoem_main(intf, argc, argv);
	} 
        ipmi_close_();
        // show_outcome(progname,rc);
	return rc;
}
