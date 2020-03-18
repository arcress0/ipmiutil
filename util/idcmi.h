/*
 * idcmi.h
 * Data Center Manageability Interface (DCMI) command support 
 *
 * Change history:
 *  11/17/2011 ARCress - created
 *
 *---------------------------------------------------------------------
 */
/*M*
Copyright (c) 2011 Kontron America, Inc.
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

#ifndef IPMI_DCMI_H
#define IPMI_DCMI_H

#define CC_DCMI_INVALID_COMMAND     0xC1
#define CC_DCMI_RECORD_NOT_PRESENT  0xCB
#define NETFN_DCMI  0x2C
/* for DCMI 1.0 and greater */
#define CMD_DCMI_GET_CAPAB     0x01
#define CMD_DCMI_GET_POWREAD   0x02
#define CMD_DCMI_GET_POWLIMIT  0x03
#define CMD_DCMI_SET_POWLIMIT  0x04
#define CMD_DCMI_ACT_POWLIMIT  0x05
#define CMD_DCMI_GET_ASSETTAG  0x06
#define CMD_DCMI_GET_SENSORINF 0x07
#define CMD_DCMI_SET_ASSETTAG  0x08
#define CMD_DCMI_GET_MCIDSTR   0x09
#define CMD_DCMI_SET_MCIDSTR   0x0A
/* for DCMI 1.5 only */
#define CMD_DCMI_SET_THERMLIM  0x0B
#define CMD_DCMI_GET_THERMLIM  0x0C
#define CMD_DCMI_GET_TEMPRDNGS 0x10
#define CMD_DCMI_SET_CONFIG    0x12
#define CMD_DCMI_GET_CONFIG    0x13

/* CMD_DCMI_SET_POWLIMIT options */
#define CORRECTION_TIME         0x01
#define EXCEPTION_ACTION        0x02
#define POWER_LIMIT             0x03
#define SAMPLE_PERIOD           0x04

#endif
/* end idcmi.h */
