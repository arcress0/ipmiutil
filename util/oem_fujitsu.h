/*
 * oem_fujitsu.h
 *
 * Authors: Andy Cress  arcress at users.sourceforge.net, and
 *          Dan Lukes   dan at obluda.cz
 *
 * 08/27/10 Andy Cress - added with source input from Dan Lukes
 */
/*M*
The BSD 2.0 License 

Copyright (c) 2009 Kontron America, Inc.  All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 
  c.. Neither the name of Kontron, nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission. 

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
 *M*/

#define uint8_t  unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int

#define IPMI_OEM_MAX_BYTES      100
#define IPMI_IANA_ENTERPRISE_ID_FUJITSU               10368
#define IPMI_NET_FN_OEM_GROUP_RQ           0x2E
#define IPMI_CMD_OEM_FUJITSU_SYSTEM        0xF5
#define IPMI_OEM_FUJITSU_COMMAND_SPECIFIER_GET_SEL_ENTRY_LONG_TEXT   0x43

#define IPMI_FUJITSU_PRODUCT_ID_MIN                 0x0200
#define IPMI_FUJITSU_PRODUCT_ID_MAX                 0x03FF
// iRMC-S1 based systems
#define IPMI_FUJITSU_PRODUCT_ID_TX200S3             0x0200
#define IPMI_FUJITSU_PRODUCT_ID_TX300S3             0x0201
#define IPMI_FUJITSU_PRODUCT_ID_RX200S3             0x0202
#define IPMI_FUJITSU_PRODUCT_ID_RX300S3             0x0203
#define IPMI_FUJITSU_PRODUCT_ID_UNUSEDS3            0x0204
#define IPMI_FUJITSU_PRODUCT_ID_RX100S4             0x0205
#define IPMI_FUJITSU_PRODUCT_ID_TX150S5             0x0206
#define IPMI_FUJITSU_PRODUCT_ID_TX120S1             0x0207
#define IPMI_FUJITSU_PRODUCT_ID_BX630S2             0x0208
#define IPMI_FUJITSU_PRODUCT_ID_RX330S1             0x0209
#define IPMI_FUJITSU_PRODUCT_ID_E230RN1             0x0210
#define IPMI_FUJITSU_PRODUCT_ID_E230RSL             0x0211
#define IPMI_FUJITSU_PRODUCT_ID_RX330S1_SHA         0x0212
#define IPMI_FUJITSU_PRODUCT_ID_BX630S2_SHA         0x0213

#define FUJITSU_PRODUCT_IS_iRMC_S1(_product_id_)                   \
  ((_product_id_) == IPMI_FUJITSU_PRODUCT_ID_TX200S3               \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_TX300S3           \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_RX200S3           \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_RX300S3           \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_UNUSEDS3          \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_RX100S4           \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_TX150S5           \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_TX120S1           \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_BX630S2           \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_RX330S1           \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_E230RN1           \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_E230RSL           \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_RX330S1_SHA       \
    || (_product_id_) == IPMI_FUJITSU_PRODUCT_ID_BX630S2_SHA)

// iRMC-S2 based systems        
#define IPMI_FUJITSU_PRODUCT_ID_RX600S4             0x0218
#define IPMI_FUJITSU_PRODUCT_ID_TX200S4             0x0220
#define IPMI_FUJITSU_PRODUCT_ID_TX300S4             0x0221
#define IPMI_FUJITSU_PRODUCT_ID_RX200S4             0x0222
#define IPMI_FUJITSU_PRODUCT_ID_RX300S4             0x0223
#define IPMI_FUJITSU_PRODUCT_ID_UNUSEDS4            0x0224
#define IPMI_FUJITSU_PRODUCT_ID_RX100S5             0x0225
#define IPMI_FUJITSU_PRODUCT_ID_TX150S6             0x0226
#define IPMI_FUJITSU_PRODUCT_ID_TX120S2             0x0227
#define IPMI_FUJITSU_PRODUCT_ID_TX150S6_64K         0x0233
#define IPMI_FUJITSU_PRODUCT_ID_TX200S4_64K         0x0234
#define IPMI_FUJITSU_PRODUCT_ID_TX300S4_64K         0x0235
#define IPMI_FUJITSU_PRODUCT_ID_TX200S5             0x0240
#define IPMI_FUJITSU_PRODUCT_ID_TX300S5             0x0241
#define IPMI_FUJITSU_PRODUCT_ID_RX200S5             0x0242
#define IPMI_FUJITSU_PRODUCT_ID_RX300S5             0x0243
#define IPMI_FUJITSU_PRODUCT_ID_BX620S5             0x0244
#define IPMI_FUJITSU_PRODUCT_ID_RX100S6             0x0245
#define IPMI_FUJITSU_PRODUCT_ID_TX150S7             0x0246
#define IPMI_FUJITSU_PRODUCT_ID_BX960S1             0x0254
#define IPMI_FUJITSU_PRODUCT_ID_BX924S1             0x0255
#define IPMI_FUJITSU_PRODUCT_ID_BX920S1             0x0256
#define IPMI_FUJITSU_PRODUCT_ID_BX922S1             0x0257
#define IPMI_FUJITSU_PRODUCT_ID_RX600S5             0x0258
#define IPMI_FUJITSU_PRODUCT_ID_TX200S6             0x0260
#define IPMI_FUJITSU_PRODUCT_ID_TX300S6             0x0261
#define IPMI_FUJITSU_PRODUCT_ID_RX200S6             0x0262
#define IPMI_FUJITSU_PRODUCT_ID_RX300S6             0x0263

/*******************************************
+ * Fujitsu Siemens Computers               *
+ * Fujitsu Technology Solutions            *
+ * iRMC S1 / iRMC S2                       *
+ *******************************************/
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_I2C_BUS                0xC0
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_SYSTEM_POWER_CONSUMPTION 0xDD //Events only
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_MEMORY_STATUS          0xDE
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_MEMORY_CONFIG          0xDF
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_MEMORY                 0xE1 // Events only
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_HW_ERROR               0xE3 // Events only
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_SYS_ERROR              0xE4 // Events only
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_FAN_STATUS             0xE6
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_PSU_STATUS             0xE8
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_PSU_REDUNDANCY         0xE9
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_COMMUNICATION          0xEA // Reserved
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_FLASH                  0xEC // Events only
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_EVENT                  0xEE // Reserved
#define IPMI_SENSOR_TYPE_OEM_FUJITSU_CONFIG_BACKUP          0xEF
