/***********************************************
 * ipmiutil.h 
 *
 * Definitions for ipmiutil.c
 *
 ***********************************************/
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2006, Intel Corporation
Copyright (c) 2009 Kontron America, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

  a.. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer. 
  b.. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution. 
  c.. Neither the name of Intel Corporation nor the names of its contributors 
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
 *----------------------------------------------------------------------*/
#ifndef IPMIUTIL_H_
#define IPMIUTIL_H_

int i_alarms(int argc, char **argv);
int i_health(int argc, char **argv);
int i_fru(int argc, char **argv);
int i_getevt(int argc, char **argv);
int i_reset(int argc, char **argv);
int i_cmd(int argc, char **argv);
int i_lan(int argc, char **argv);
int i_serial(int argc, char **argv);
int i_sensor(int argc, char **argv);
int i_sel(int argc, char **argv);
int i_wdt(int argc, char **argv);
int i_sol(int argc, char **argv);
int i_discover(int argc, char **argv);
int i_config(int argc, char **argv);
int i_events(int argc, char **argv);
int i_picmg(int argc, char **argv);
int i_firewall(int argc, char **argv);
int i_fwum(int argc, char **argv);
int i_hpm(int argc, char **argv);
int i_sunoem(int argc, char **argv);
int i_delloem(int argc, char **argv);
int i_ekanalyzer(int argc, char **argv);
int i_tsol(int argc, char **argv);
int i_dcmi(int argc, char **argv);
int i_smcoem(int argc, char **argv);
int i_user(int argc, char **argv);

#endif	// IPMIUTIL_H_
