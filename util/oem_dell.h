/****************************************************************************
Copyright (c) 2008, Dell Inc
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution. 
- Neither the name of Dell Inc nor the names of its contributors
may be used to endorse or promote products derived from this software 
without specific prior written permission. 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE. 


*****************************************************************************/
#ifndef IPMI_DELLOEM_H
#define IPMI_DELLOEM_H

#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_ATTRIBUTE_PACKING
/* this attribute is not very portable */
#define ATTRIBUTE_PACKING __attribute__ ((packed))
#else
/* use #pragma pack(1) instead */
#define ATTRIBUTE_PACKING 
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))



#define IPMI_SET_SYS_INFO                  0x58
#define IPMI_GET_SYS_INFO                  0x59

/* Dell selector for LCD control - get and set unless specified */
#define IPMI_DELL_LCD_STRING_SELECTOR       0xC1        /* RW get/set the user string */
#define IPMI_DELL_LCD_CONFIG_SELECTOR       0xC2        /* RW set to user/default/none */
#define IPMI_DELL_LCD_GET_CAPS_SELECTOR     0xCF        /* RO use when available*/
#define IPMI_DELL_LCD_STRINGEX_SELECTOR     0xD0        /* RW get/set the user string use first when available*/
#define IPMI_DELL_LCD_STATUS_SELECTOR       0xE7        /* LCD string when config set to default.*/
#define IPMI_DELL_PLATFORM_MODEL_NAME_SELECTOR 0xD1    /* LCD string when config set to default.*/

/* Dell defines for picking which string to use */
#define IPMI_DELL_LCD_CONFIG_USER_DEFINED   0x00 /* use string set by user*/
#define IPMI_DELL_LCD_CONFIG_DEFAULT        0x01 /* use platform model name*/
#define IPMI_DELL_LCD_CONFIG_NONE           0x02 /* blank*/
#define IPMI_DELL_LCD_iDRAC_IPV4ADRESS      0x04 /* use string set by user*/
#define IPMI_DELL_LCD_IDRAC_MAC_ADDRESS     0x08 /* use platform model name*/
#define IPMI_DELL_LCD_OS_SYSTEM_NAME        0x10 /* blank*/

#define IPMI_DELL_LCD_SERVICE_TAG           0x20  /* use string set by user*/
#define IPMI_DELL_LCD_iDRAC_IPV6ADRESS      0x40  /* use string set by user*/
#define IPMI_DELL_LCD_AMBEINT_TEMP          0x80  /* use platform model name*/
#define IPMI_DELL_LCD_SYSTEM_WATTS          0x100 /* blank*/
#define IPMI_DELL_LCD_ASSET_TAG             0x200

#define IPMI_DELL_LCD_ERROR_DISP_SEL        0x01  /* use platform model name*/
#define IPMI_DELL_LCD_ERROR_DISP_VERBOSE    0x02  /* blank*/

#define IPMI_DELL_IDRAC_VALIDATOR           0xDD    
#define IPMI_DELL_POWER_CAP_STATUS          0xBA   
#define IPMI_DELL_AVG_POWER_CONSMP_HST 	0xEB
#define IPMI_DELL_PEAK_POWER_CONSMP_HST 0xEC
#define SYSTEM_BOARD_SYSTEM_LEVEL_SENSOR_NUM 0x98

#define	IDRAC_11G					1
#define	IDRAC_12G					2
// Return Error code for license
#define	LICENSE_NOT_SUPPORTED		0x6F
#define	VFL_NOT_LICENSED			0x33
#define btuphr              0x01
#define watt                0x00
#define IPMI_DELL_POWER_CAP 0xEA
#define percent             0x03 

/* Not on all Dell servers. If there, use it.*/
#pragma pack(1)
typedef struct _tag_ipmi_dell_lcd_caps
{
       uint8_t parm_rev;                                       /* 0x11 for IPMI 2.0 */
        uint8_t char_set;                                       /* always 1 for printable ASCII 0x20-0x7E */
   uint8_t number_lines;                           /* 0-4, 1 for 9G. 10G tbd */
   uint8_t max_chars[4];                           /* 62 for triathlon, 0 if not present (glacier) */
                                                                             /* [0] is max chars for line 1 */
}IPMI_DELL_LCD_CAPS;

#define IPMI_DELL_LCD_STRING_LENGTH_MAX 62      /* Valid for 9G. Glacier ??. */
#define IPMI_DELL_LCD_STRING1_SIZE      14
#define IPMI_DELL_LCD_STRINGN_SIZE      16

/* vFlash subcommands */
#define IPMI_GET_EXT_SD_CARD_INFO 0xA4


typedef struct _tag_ipmi_dell_lcd_string
{
     uint8_t parm_rev;                       /* 0x11 for IPMI 2.0 */
     uint8_t data_block_selector;            /* 16-byte data block number to access, 0 based.*/
     union 
     {
          struct 
          {
                uint8_t encoding : 4;                     /* 0 is printable ASCII 7-bit */
                uint8_t length;                           /* 0 to max chars from lcd caps */
                uint8_t data[IPMI_DELL_LCD_STRING1_SIZE]; /* not zero terminated.  */
          }selector_0_string;
          uint8_t selector_n_data[IPMI_DELL_LCD_STRINGN_SIZE];
     }lcd_string;
} ATTRIBUTE_PACKING IPMI_DELL_LCD_STRING;

/* Only found on servers with more than 1 line. Use if available. */
typedef struct _tag_ipmi_dell_lcd_stringex
{
      uint8_t parm_rev;                       /* 0x11 for IPMI 2.0 */
      uint8_t line_number;                    /* LCD line number 1 to 4 */
      uint8_t data_block_selector;            /* 16-byte data block number to access, 0 based.*/
      union 
      {
           struct  
           {
                uint8_t encoding : 4;                     /* 0 is printable ASCII 7-bit */
                uint8_t length;                           /* 0 to max chars from lcd caps */
                uint8_t data[IPMI_DELL_LCD_STRING1_SIZE]; /* not zero terminated.  */
           } selector_0_string;
           uint8_t selector_n_data[IPMI_DELL_LCD_STRINGN_SIZE];
   } lcd_string;
} ATTRIBUTE_PACKING IPMI_DELL_LCD_STRINGEX;


typedef struct _lcd_status
{
      char vKVM_status;
      char lock_status;
      char Resv1;
      char Resv;
} ATTRIBUTE_PACKING LCD_STATUS;

typedef struct _lcd_mode
{
    uint8_t parametersel;
    uint32_t lcdmode;
    uint16_t lcdqualifier;
    uint32_t capabilites;
    uint8_t error_display;
    uint8_t Resv;
} ATTRIBUTE_PACKING LCD_MODE;
#pragma pack()

#define PARAM_REV_OFFSET                    (uint8_t)(0x1)

#define LOM_MACTYPE_ETHERNET 0
#define LOM_MACTYPE_ISCSI 1
#define LOM_MACTYPE_RESERVED 3

#define LOM_ETHERNET_ENABLED 0
#define LOM_ETHERNET_DISABLED 1
#define LOM_ETHERNET_PLAYINGDEAD 2
#define LOM_ETHERNET_RESERVED 3

#define LOM_ACTIVE 1
#define LOM_INACTIVE 0

#define MACADDRESSLENGH 6
#define MAX_LOM 8

#define IPMI_NETFN_SE             (uint8_t)(0x04)
#define IPMI_NETFN_APP		  (uint8_t)(0x06) //NETFN_APP
#define IPMI_NETFN_STORAGE 	  (uint8_t)(0x0a) //NETFN_STOR
#define IPMI_CMD_GET_SEL_TIME                0x48
#define GET_FRU_INFO         0x10
#define GET_FRU_DATA         0x11
#define BSWAP_16(x) ((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8))

#define APP_NETFN                   (uint8_t)(0x6)


#define GET_SYSTEM_INFO_CMD         (uint8_t)(0x59)
#define EMB_NIC_MAC_ADDRESS_11G     (uint8_t)(0xDA)
#define EMB_NIC_MAC_ADDRESS_9G_10G  (uint8_t)(0xCB)

#define IMC_IDRAC_10G               (uint8_t) (0x08) 
#define IMC_CMC                     (uint8_t) (0x09)
#define IMC_IDRAC_11G_MONOLITHIC    (uint8_t) (0x0A)
#define IMC_IDRAC_11G_MODULAR       (uint8_t) (0x0B)
#define IMC_UNUSED                  (uint8_t) (0x0C)
#define IMC_MASER_LITE_BMC          (uint8_t) (0x0D)
#define IMC_IDRAC_12G_MONOLITHIC 	(uint8_t) (0x10)
#define IMC_IDRAC_12G_MODULAR 		(uint8_t) (0x11)


#pragma pack(1)
#ifdef LOM_OLD
typedef struct
{
     unsigned int BladSlotNumber : 4;
     unsigned int MacType : 2;
     unsigned int EthernetStatus : 2;
     unsigned int NICNumber : 5;
     unsigned int Reserved : 3;
     uint8_t MacAddressByte[MACADDRESSLENGH];
} LOMMacAddressType;
#else
typedef struct
{
     uint8_t  b0;
     uint8_t  b1;
     uint8_t MacAddressByte[MACADDRESSLENGH];
} LOMMacAddressType;
#endif
#pragma pack()

#pragma pack(1)
typedef struct
{
     LOMMacAddressType LOMMacAddress [MAX_LOM];
} EmbeddedNICMacAddressType;

typedef struct
{
     uint8_t MacAddressByte[MACADDRESSLENGH];
} MacAddressType;

typedef struct
{
   MacAddressType MacAddress [MAX_LOM];
} EmbeddedNICMacAddressType_10G;

struct fru_info {
        uint16_t size;
        uint8_t access:1;
};
struct fru_header {
        uint8_t version;
        struct {
                uint8_t internal;
                uint8_t chassis;
                uint8_t board;
                uint8_t product;
                uint8_t multi;
        } offset;
        uint8_t pad;
        uint8_t checksum;
} ATTRIBUTE_PACKING;

struct entity_id {
        uint8_t id;                     /* physical entity id */
#if WORDS_BIGENDIAN
        uint8_t logical     : 1;        /* physical/logical */
        uint8_t instance    : 7;        /* instance number */
#else
        uint8_t instance    : 7;        /* instance number */
        uint8_t logical     : 1;        /* physical/logical */
#endif
} ATTRIBUTE_PACKING;
struct sdr_record_mask {
        union {
                struct {
                        uint16_t assert_event;  /* assertion event mask */
                        uint16_t deassert_event;        /* de-assertion event ma
sk */
                        uint16_t read;  /* discrete reading mask */
                } ATTRIBUTE_PACKING discrete;
		/*...*/
	} ATTRIBUTE_PACKING type;
} ATTRIBUTE_PACKING ;

struct sdr_record_full_sensor {
	struct {
		uint8_t owner_id;
#if WORDS_BIGENDIAN
		uint8_t channel:4;	/* channel number */
		uint8_t __reserved1:2;
		uint8_t lun:2;	/* sensor owner lun */
#else
		uint8_t lun:2;	/* sensor owner lun */
		uint8_t __reserved2:2;
		uint8_t channel:4;	/* channel number */
#endif
		uint8_t sensor_num;	/* unique sensor number */
	} ATTRIBUTE_PACKING keys;

	struct entity_id entity;

	struct {
		struct {
#if WORDS_BIGENDIAN
			uint8_t __reserved3:1;
			uint8_t scanning:1;
			uint8_t events:1;
			uint8_t thresholds:1;
			uint8_t hysteresis:1;
			uint8_t type:1;
			uint8_t event_gen:1;
			uint8_t sensor_scan:1;
#else
			uint8_t sensor_scan:1;
			uint8_t event_gen:1;
			uint8_t type:1;
			uint8_t hysteresis:1;
			uint8_t thresholds:1;
			uint8_t events:1;
			uint8_t scanning:1;
			uint8_t __reserved4:1;
#endif
		} ATTRIBUTE_PACKING init;
		struct {
#if WORDS_BIGENDIAN
			uint8_t ignore:1;
			uint8_t rearm:1;
			uint8_t hysteresis:2;
			uint8_t threshold:2;
			uint8_t event_msg:2;
#else
			uint8_t event_msg:2;
			uint8_t threshold:2;
			uint8_t hysteresis:2;
			uint8_t rearm:1;
			uint8_t ignore:1;
#endif
		} ATTRIBUTE_PACKING capabilities;
		uint8_t type;
	} ATTRIBUTE_PACKING sensor;

	uint8_t event_type;	/* event/reading type code */

	struct sdr_record_mask mask;

	struct {
#if WORDS_BIGENDIAN
		uint8_t analog:2;
		uint8_t rate:3;
		uint8_t modifier:2;
		uint8_t pct:1;
#else
		uint8_t pct:1;
		uint8_t modifier:2;
		uint8_t rate:3;
		uint8_t analog:2;
#endif
		struct {
			uint8_t base;
			uint8_t modifier;
		} ATTRIBUTE_PACKING type;
	} ATTRIBUTE_PACKING unit;

#define SDR_SENSOR_L_LINEAR     0x00
#define SDR_SENSOR_L_LN         0x01
#define SDR_SENSOR_L_LOG10      0x02
#define SDR_SENSOR_L_LOG2       0x03
#define SDR_SENSOR_L_E          0x04
#define SDR_SENSOR_L_EXP10      0x05
#define SDR_SENSOR_L_EXP2       0x06
#define SDR_SENSOR_L_1_X        0x07
#define SDR_SENSOR_L_SQR        0x08
#define SDR_SENSOR_L_CUBE       0x09
#define SDR_SENSOR_L_SQRT       0x0a
#define SDR_SENSOR_L_CUBERT     0x0b
#define SDR_SENSOR_L_NONLINEAR  0x70
	uint8_t linearization;	/* 70h=non linear, 71h-7Fh=non linear, OEM */
	uint16_t mtol;		/* M, tolerance */
	uint32_t bacc;		/* accuracy, B, Bexp, Rexp */

	struct {
#if WORDS_BIGENDIAN
		uint8_t __reserved5:5;
		uint8_t normal_min:1;	/* normal min field specified */
		uint8_t normal_max:1;	/* normal max field specified */
		uint8_t nominal_read:1;	/* nominal reading field specified */
#else
		uint8_t nominal_read:1;	/* nominal reading field specified */
		uint8_t normal_max:1;	/* normal max field specified */
		uint8_t normal_min:1;	/* normal min field specified */
		uint8_t __reserved5:5;
#endif
	} ATTRIBUTE_PACKING analog_flag;

	uint8_t nominal_read;	/* nominal reading, raw value */
	uint8_t normal_max;	/* normal maximum, raw value */
	uint8_t normal_min;	/* normal minimum, raw value */
	uint8_t sensor_max;	/* sensor maximum, raw value */
	uint8_t sensor_min;	/* sensor minimum, raw value */

	struct {
		struct {
			uint8_t non_recover;
			uint8_t critical;
			uint8_t non_critical;
		} ATTRIBUTE_PACKING upper;
		struct {
			uint8_t non_recover;
			uint8_t critical;
			uint8_t non_critical;
		} ATTRIBUTE_PACKING lower;
		struct {
			uint8_t positive;
			uint8_t negative;
		} ATTRIBUTE_PACKING hysteresis;
	} ATTRIBUTE_PACKING threshold;
	uint8_t __reserved6[2];
	uint8_t oem;		/* reserved for OEM use */
	uint8_t id_code;	/* sensor ID string type/length code */
	uint8_t id_string[16];	/* sensor ID string bytes, only if id_code != 0 */
} ATTRIBUTE_PACKING;
struct sdr_record_list {
        uint16_t id;
        uint8_t version;
        uint8_t type;
        uint8_t length;
        union {
                struct sdr_record_full_sensor *full;
        } ATTRIBUTE_PACKING  record;
        uint8_t *raw;
        struct sdr_record_list *next;
} ATTRIBUTE_PACKING;
#pragma pack()

#define TRANSPORT_NETFN             (uint8_t)(0xc)
#define GET_LAN_PARAM_CMD           (uint8_t)(0x02)
#define MAC_ADDR_PARAM              (uint8_t)(0x05)
#define LAN_CHANNEL_NUMBER          (uint8_t)(0x01)

#define IDRAC_NIC_NUMBER            (uint8_t)(0x8)

#define TOTAL_N0_NICS_INDEX         (uint8_t)(0x1)


// 12g supported 
#define SET_NIC_SELECTION_12G_CMD       (uint8_t)(0x28)
#define GET_NIC_SELECTION_12G_CMD       (uint8_t)(0x29)

// 11g supported 
#define SET_NIC_SELECTION_CMD       (uint8_t)(0x24)
#define GET_NIC_SELECTION_CMD       (uint8_t)(0x25)
#define GET_ACTIVE_NIC_CMD          (uint8_t)(0xc1)
#define POWER_EFFICENCY_CMD     		(uint8_t)(0xc0)
#define SERVER_POWER_CONSUMPTION_CMD   	(uint8_t)(0x8F)

#define POWER_SUPPLY_INFO           (uint8_t)(0xb0)
#define IPMI_ENTITY_ID_POWER_SUPPLY (uint8_t)(0x0a)
#define SENSOR_STATE_STR_SIZE       (uint8_t)(64)
#define SENSOR_NAME_STR_SIZE        (uint8_t)(64)

#define GET_PWRMGMT_INFO_CMD	    (uint8_t)(0x9C)
#define CLEAR_PWRMGMT_INFO_CMD	    (uint8_t)(0x9D)
#define GET_PWR_HEADROOM_CMD	    (uint8_t)(0xBB)
#define GET_PWR_CONSUMPTION_CMD	    (uint8_t)(0xB3)
#define	GET_FRONT_PANEL_INFO_CMD		(uint8_t)0xb5


#pragma pack(1)
typedef struct _ipmi_power_monitor
{
    uint32_t        cumStartTime;
    uint32_t        cumReading;
    uint32_t        maxPeakStartTime;
    uint32_t        ampPeakTime;
    uint16_t        ampReading;
    uint32_t        wattPeakTime;
    uint16_t        wattReading;
} ATTRIBUTE_PACKING IPMI_POWER_MONITOR;


#define MAX_POWER_FW_VERSION 8

typedef struct _ipmi_power_supply_infoo
{
	/*No param_rev it is not a System Information Command */
	uint16_t ratedWatts;
	uint16_t ratedAmps;
	uint16_t ratedVolts;
	uint32_t vendorid;
    uint8_t FrimwareVersion[MAX_POWER_FW_VERSION];
	uint8_t  Powersupplytype;
	uint16_t ratedDCWatts;
	uint16_t Resv;	
                          
} ATTRIBUTE_PACKING IPMI_POWER_SUPPLY_INFO;


typedef struct ipmi_power_consumption_data
{
    uint16_t actualpowerconsumption;
    uint16_t powerthreshold;
    uint16_t warningthreshold;
    uint8_t throttlestate;
    uint16_t maxpowerconsumption;
    uint16_t throttlepowerconsumption;
    uint16_t Resv;
} ATTRIBUTE_PACKING IPMI_POWER_CONSUMPTION_DATA;


typedef struct ipmi_inst_power_consumption_data
{
    uint16_t instanpowerconsumption;
    uint16_t instanApms;
    uint16_t resv1;
    uint8_t resv;
} ATTRIBUTE_PACKING IPMI_INST_POWER_CONSUMPTION_DATA;

typedef struct _ipmi_avgpower_consump_histroy
{
    uint8_t parameterselector;  
    uint16_t lastminutepower;
    uint16_t lasthourpower;
    uint16_t lastdaypower;
    uint16_t lastweakpower;  
                          
} ATTRIBUTE_PACKING IPMI_AVGPOWER_CONSUMP_HISTORY;

typedef struct _ipmi_power_consump_histroy
{
    uint8_t parameterselector;   
    uint16_t lastminutepower;
    uint16_t lasthourpower;
    uint16_t lastdaypower;
    uint16_t lastweakpower; 
    uint32_t lastminutepowertime;
    uint32_t lasthourpowertime;
    uint32_t lastdaypowertime;
    uint32_t lastweekpowertime;
} ATTRIBUTE_PACKING IPMI_POWER_CONSUMP_HISTORY;


typedef struct _ipmi_delloem_power_cap
{     
    uint8_t parameterselector;      
    uint16_t PowerCap;
    uint8_t unit;
    uint16_t MaximumPowerConsmp;
    uint16_t MinimumPowerConsmp;
    uint16_t totalnumpowersupp;
    uint16_t AvailablePower ;
    uint16_t SystemThrottling;
    uint16_t Resv;
} ATTRIBUTE_PACKING IPMI_POWER_CAP;       

typedef struct _power_headroom
{ 
    uint16_t instheadroom;
    uint16_t peakheadroom;
} ATTRIBUTE_PACKING POWER_HEADROOM;

struct vFlashstr {
	uint8_t val;
	const char * str;
};
typedef struct ipmi_vFlash_extended_info
{
	uint8_t  vflashcompcode;
	uint8_t  sdcardstatus;
	uint32_t sdcardsize;
	uint32_t sdcardavailsize;
	uint8_t  bootpartion;
	uint8_t  Resv;
}  IPMI_DELL_SDCARD_INFO;
#pragma pack()


typedef struct _SensorReadingType
{
    uint8_t sensorReading;
    uint8_t sensorFlags;
    uint16_t sensorState;
}SensorReadingType;

struct standard_spec_sel_rec{
        uint32_t        timestamp;
        uint16_t        gen_id;
        uint8_t evm_rev;
        uint8_t sensor_type;
        uint8_t sensor_num;
#if WORDS_BIGENDIAN
        uint8_t event_dir  : 1;
        uint8_t event_type : 7;
#else
        uint8_t event_type : 7;
        uint8_t event_dir  : 1;
#endif
#define DATA_BYTE2_SPECIFIED_MASK 0xc0    /* event_data[0] bit mask */
#define DATA_BYTE3_SPECIFIED_MASK 0x30    /* event_data[0] bit mask */
#define EVENT_OFFSET_MASK         0x0f    /* event_data[0] bit mask */
        uint8_t event_data[3];
};

#define SEL_OEM_TS_DATA_LEN             6
#define SEL_OEM_NOTS_DATA_LEN           13
struct oem_ts_spec_sel_rec{
        uint32_t timestamp;
        uint8_t manf_id[3];
        uint8_t oem_defined[SEL_OEM_TS_DATA_LEN];
};

struct oem_nots_spec_sel_rec{
        uint8_t oem_defined[SEL_OEM_NOTS_DATA_LEN];
};

#pragma pack(1)
struct sel_event_record {
        uint16_t        record_id;
        uint8_t record_type;
        union{
                struct standard_spec_sel_rec standard_type;
                struct oem_ts_spec_sel_rec oem_ts_type;
                struct oem_nots_spec_sel_rec oem_nots_type;
        } sel_type;
}  ATTRIBUTE_PACKING; // __attribute__ ((packed));
#pragma pack()

uint16_t compareinputwattage(IPMI_POWER_SUPPLY_INFO* powersupplyinfo, uint16_t inputwattage);
int ipmi_delloem_main(void * intf, int argc, char ** argv);
int ipmi_delloem_getled_state (void * intf, uint8_t *state);
#endif /*IPMI_DELLOEM_H*/
