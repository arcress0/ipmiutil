/*
 * ievents.h
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2011 Kontron America, Inc.
 *
 * 12/12/11 Andy Cress - created
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
 
/* Public routines from ievents.c */
int  decode_sel_entry( uchar *psel, char *outbuf, int sz);
char *decode_entity_id(int id);
char *get_sensor_type_desc(uchar stype); 
char *get_sev_str(int val); 
void  fmt_time(time_t etime, char *buf, int bufsz);
int   get_sensor_tag(int isdr, int genid, uchar snum, char *tag,
                        uchar *sdr, int szsdr);
void format_event(ushort id, time_t timestamp, int sevid, ushort genid,
		  char *ptype, uchar snum, char *psens, char *pstr, char *more, 
		  char *outbuf, int outsz);

#define DIMM_UNKNOWN  "DIMM_unknown"
#define DIMM_NUM  "DIMM(%d)"
/*
 * set_sel_opts is used to specify options for showing/decoding SEL events.
 * sensdesc : 0 = simple, no sensor descriptions
 *            1 = get sensor descriptions from sdr cache
 *            2 = get sensor descriptions from sensor file (-s)
 * canon    : 0 = normal output
 *            1 = canonical, delimited output
 * sdrs     : NULL = no sdr cache, dynamically get sdr cache if sensdesc==1
 *            ptr  = use this pointer as existing sdr cache if sensdesc==1
 * fdbg     : 0 = normal mode
 *            1 = debug mode
 * futc     : 0 = normal mode
 *            1 = show raw UTC time
 */
void set_sel_opts(int sensdsc, int canon, void *sdrs, char fdbg, char futc);

/*end ievents.h*/
