/*
 * oem_intel.h
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2010 Kontron America, Inc.
 *
 * 09/02/10 Andy Cress - separated from ialaems.c
 */
/*M*
Copyright (c) 2010 Kontron America, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 
  c.. Neither the name of Kontron nor the names of its contributors 
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
 *M*/
 

#define PRIVATE_BUS_ID      0x03 // w Sahalee,  the 8574 is on Private Bus 1
#define PRIVATE_BUS_ID5     0x05 // for Intel TIGI2U
#define PRIVATE_BUS_ID7     0x07 // for Intel S5000
#define PERIPHERAL_BUS_ID   0x24 // w mBMC, the 8574 is on the Peripheral Bus
#define ALARMS_PANEL_WRITE  0x40 
#define ALARMS_PANEL_READ   0x41
#define DISK_LED_WRITE      0x44 // only used for Chesnee mBMC
#define DISK_LED_READ       0x45 // only used for Chesnee mBMC

#define HAS_ALARMS_MASK   0x0001
#define HAS_BMCTAM_MASK   0x0002
#define HAS_ENCL_MASK     0x0004
#define HAS_PICMG_MASK    0x0008
#define HAS_NSC_MASK      0x0010
#define HAS_ROMLEY_MASK   0x0020
#define HAS_GRANTLEY_MASK 0x0040

uchar get_nsc_diskleds(uchar busid);
int set_nsc_diskleds(uchar val, uchar busid);
void show_nsc_diskleds(uchar val);

uchar get_alarms_intel(uchar busid);
int set_alarms_intel(uchar val, uchar busid);
void show_alarms_intel(uchar val);
int check_bmctam_intel(void);
int detect_capab_intel(int vend_id,int prod_id, int *cap, int *ndsk,char fdbg);
int get_led_status_intel(uchar *pstate);
int is_lan2intel(int vend, int prod);
int decode_sensor_intel(uchar *sdr,uchar *reading,char *pstring, int slen);
int decode_sensor_intel_nm(uchar *sdr,uchar *reading,char *pstring, int slen);
void show_oemsdr_intel(uchar *sdr);
void show_oemsdr_nm(uchar *sdr);
int is_romley(int vend, int prod); 
int is_thurley(int vend, int prod); 
int get_enc_leds_intel(uchar *val);
int set_enc_leds_intel(uchar val);
void show_enc_leds_intel(uchar val, int numd);
int lan_failover_intel(uchar func, uchar *mode);
int intel_romley_desc(int vend, int prod, char **pdesc);
int get_power_restore_delay_intel(int *delay);
int get_hsbp_version_intel(uchar *maj, uchar *min);
int is_grantley(int vend, int prod);
int intel_grantley_desc(int vend, int prod, char **pdesc);

/* end oem_intel.h */
