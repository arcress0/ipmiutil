/*
 * oem_supermicro.h
 * SuperMicro OEM command functions 
 *
 *---------------------------------------------------------------------
 */
/*M*
Copyright (c) 2013 Kontron America, Inc.
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

#define SUPER_NETFN_OEM           0x30
#define SUPER_CMD_BMCSTATUS       0x70
#define SUPER_CMD_RESET_INTRUSION 0x03
#define SUPER_NETFN_OEMFW     0x3C  /*for SuperMicro/Peppercon*/
#define SUPER_CMD_OEMFWINFO   0x20

int oem_supermicro_get_bmc_status(uchar *sts);
int oem_supermicro_set_bmc_status(uchar sts);
int oem_supermicro_get_health(char *pstr, int sz);
int oem_supermicro_get_firmware_info(uchar *info);
int oem_supermicro_get_firmware_str(char *pstr, int sz);
int oem_supermicro_reset_intrusion(void);
int oem_supermicro_get_lan_port(uchar *val);
int oem_supermicro_set_lan_port(uchar val);
char *oem_supermicro_lan_port_string(uchar val);

int decode_sensor_supermicro(uchar *sdr,uchar *reading,char *pstring, int slen, int fsimple, char fdbg);
int decode_mem_supermicro(int prod, uchar b2, uchar b3, char *desc, int *psz);
int decode_sel_supermicro(uchar *evt, char *outbuf, int outsz, char fdesc,
                        char fdebug);

/* end oem_supermicro.h */
