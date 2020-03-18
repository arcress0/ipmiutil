/*
 * isel.c   (was showsel.c)
 *
 * This tool reads the firmware System Event Log records via IPMI commands.
 * It can use either the Intel /dev/imb driver or VALinux /dev/ipmikcs.
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 *
 * Copyright (c) 2001-2005 Intel Corporation. 
 * Copyright (c) 2009 Kontron America, Inc.
 * Copyright (c) 2013 Andy Cress <arcress at users.sourceforge.net>
 *
 * 10/16/01 Andy Cress - created
 * 10/19/01 Andy Cress - added text for Sensor Types from Table 36-3
 * 10/24/01 Andy Cress - added panic string display for OS Crit Stop
 * 11/01/01 Andy Cress - added logic to write SEL records to syslog
 * 11/15/01 Andy Cress - added logic for more description strings
 * 01/31/02 Andy Cress - isolated dependencies on /dev/imb to ipmi_cmd_ia(),
 *                       added ipmi_cmd_va() for va ipmi driver also,
 *                       added posterrs array for more descriptions.
 * 02/06/02 Andy Cress - fixed bug 279 (extra debug msg at EOF).
 * 03/25/02 Andy Cress - show free space, add -c to clear SEL.
 * 04/11/02 Andy Cress - decode timestamp into readable form
 * 06/14/02 Andy Cress - also show status error in ClearSEL
 * 07/02/02 Andy Cress v1.3 add more Usage text
 * 08/02/02 Andy Cress v1.4 moved common ipmi_cmd() code to ipmicmd.c 
 * 10/07/02 Andy Cress v1.5 added -v option with BMC version too
 * 01/16/03 Andy Cress v1.6 Handle new OS crit stop format with die code
 * 01/29/03 Andy Cress v1.7 added MV OpenIPMI support
 * 02/05/03 Andy Cress v1.8 show better message if empty SEL (cc=0xCB),
 *                          show an additional warning if free space is low.
 * 02/18/03 Andy Cress v1.9 trim out some fields so it fits on 1 line,
 *                          decode Boot Event subcodes
 * 02/27/03 Andy Cress v1.10 change OS Crit Stop decoding to handle new types
 * 03/20/03 Andy Cress v1.11 for -w, save id also, so we don't have to start
 *                           over at the beginning each time.
 * 04/30/03 Andy Cress v1.12 changed display ordering.
 * 06/23/03 Andy Cress v1.13 fix -w if log gets cleared
 * 08/19/03 Andy Cress v1.14 handle OEM/other record types
 * 09/16/03 Andy Cress v1.15 added more sens_desc strings
 * 10/03/03 Andy Cress v1.16 added more sens_desc strings (for boot)
 * 01/19/04 Andy Cress v1.17 added more sens_desc for Fans
 * 01/30/04 Andy Cress v1.18 added WIN32 flags, and sens_desc for Voltage
 * 03/12/04 Andy Cress v1.19 ClockSync description changed
 * 03/29/04 Andy Cress v1.20 change pattern matching for thresholds, 
 *                           added sens_desc for ID Button
 *                           check either time or record_id if fwritesel
 * 			     show warning if <20% free also
 *                           added sens_desc for System Events, HSC, Power, Int
 * 04/13/04 Andy Cress v1.21 added threshold OK descriptions,
 *                           change header (time is local, not GMT)
 * 05/05/04 Andy Cress v1.22 call ipmi_close before exit,
 *                           include ReportEvent code for WIN32
 * 06/10/04 Andy Cress v1.23 use gmtime instead of localtime for WIN32
 * 08/16/04 Andy Cress v1.24 added more decoding for Power events
 * 09/20/04 Andy Cress v1.25 added 2 event descriptors for ia64 platforms
 * 11/01/04 Andy Cress v1.26 add -N / -R for remote nodes   
 * 11/16/04 Andy Cress v1.27 add -U for username,
 *                           added more decoding for mBMC watchdog events
 * 11/19/04 Andy Cress v1.28 added more decoding for crit_int, slots, etc.
 *                           changed firmware error decoding.
 * 02/17/05 Andy Cress v1.29 made decode_sel_entry() a subroutine, 
 *                           added logic for OEM 0xc0 record types.
 * 05/24/05 Andy Cress v1.30 fixed SegFault with StartWriting/fscanf
 * 05/26/05 Andy Cress v1.31 moved decode_sel_entry to events.c
 * 08/01/05 Andy Cress v1.32 updated events.c for PowerUnit & Battery
 * 09/12/05 Andy Cress v1.33 dont check superuser for fipmi_lan
 * 06/29/06 Andy Cress v1.34 added -l option
 * 02/06/08 Andy Cress v2.8  make sure savid for -w is unsigned
 */
/*M*
Copyright (c) 2002-2005, Intel Corporation
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
 *M*/
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"
#elif defined(DOS)
#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"
#else
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#else
#include <getopt.h>
#endif
#endif
#include <time.h>

#include "ipmicmd.h" 

#define  SELprintf  printf
#define  ETYPE_CRITSTOP  0x20
#define  RTYPE_OEM2      0xe0  /* 2nd OEM type (range) */
 
#pragma pack(1)
typedef struct
{
        ushort             record_id;
        uchar              record_type;
        uint               timestamp;
        ushort             generator_id;    /*slave_addr/channel*/
        uchar              evm_rev;         //event message revision
        uchar              sensor_type;
        uchar              sensor_number;
        uchar              event_trigger;
        uchar              event_data1;
        uchar              event_data2;
        uchar              event_data3;
}       SEL_RECORD;
#pragma pack()

#define MIN_FREE       128    /*=8*16, minimal bytes of free SEL space */
#define REC_SIZE        16    /*SEL Record Size, see IPMI 1.5 Table 26-1 */
#define RECORD_BASE     2     //base value to the SEL record in IMB resp data
#define RID_OFFSET      0     //byte offset to the record id
#define RTYPE_OFFSET    2     //byte offset to the record type
#define RTS_OFFSET      3     //byte offset to the record timestamp
#define RGID_OFFSET     7     //byte offset to the record generator id
#define REREV_OFFSET    9     //byte offset to the record event message rev
#define RSTYPE_OFFSET   10    //byte offset to the record sensor type
#define RSN_OFFSET      11    //byte offset to the record sensor number
#define RET_OFFSET      12    //byte offset to the record event trigger
#define RDATA_OFFSET    13    //byte offset to the record event data  

#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil sel";
#else
static char * progver   = "3.08";
static char * progname  = "isel";
#endif
#ifdef WIN32
#define IDXFILE   "sel.idx"
static char idxfile[80] = IDXFILE;
static char idxfile2[80] = "%ipmiutildir%\\sel.idx";
#else
static char idxfile[80]  = "/var/lib/ipmiutil/sel.idx";
static char idxfile2[80] = "/usr/share/ipmiutil/sel.idx"; /*old location*/
#endif
static char fdebug = 0;
static char fall = 1;
static char futc      = 0;
static char fwritesel = 0;
static char fshowraw = 0;
static char fdecoderaw = 0;
static char fdecodebin = 0;
static char fclearsel = 0;
static char faddsel = 0;
static char fonlyver = 0;
static char flastrecs = 0;
static char fremote = 0;
static char fsensdesc = 0;
static char fcanonical = 0;
static char fset_mc = 0;
static uchar min_sev = 0;  /*only show sev >= this value [0,1,2,3]*/
static char *addstr = NULL;
static char *addhex = NULL;
static uint savtime = 0;
static ushort savid = 0;
static int nlast = 20;
static ushort idinc = REC_SIZE;
static char *rawfile = NULL;
static int  vend_id, prod_id;
static uchar *sdrs = NULL;
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;

/*------------------------------------------------------------------------ 
 * decode_sel_entry
 * Parse and decode the SEL record into readable format.
 * See ievents.c
 *------------------------------------------------------------------------*/
extern char *evt_hdr;
extern char *evt_hdr2;
extern int  decode_sel_entry( uchar *psel, char *outbuf, int sz);
extern int  decode_raw_sel( char *raw_file, int mode);
extern void set_sel_opts(int sensdsc, int canon, void *sdrs, char fdbg, char u);
extern int get_sdr_cache( uchar **sdrs);
extern void free_sdr_cache( uchar *sdrs);
extern uchar find_msg_sev(char *msgbuf);  /* subs.c*/
extern int OpenSyslog(char *tag);      /*see subs.c*/
extern void CloseSyslog(void);         /*see subs.c*/
extern void WriteSyslog(char *msgbuf); /*see subs.c*/
extern int write_syslog(char *msg);    /*see subs.c*/

/*F*
////////////////////////////////////////////////////////////////////////////////
//  GetSelEntry
////////////////////////////////////////////////////////////////////////////////
//  Name			:	GetSelEntry
//
//  Purpose			:	This routine gets the next SEL record
//						
//  Parameters		:	
//	pRecordID		:	input/output pointer to next Record ID
//	selRecord		:	output pointer to the sel record
//
//  Returns			:	0 if success
//					-1 if last record.
//					-2 if completion code error.
//					-3 if null buffer on input.
//					-4 if record id mismatch.
//						
//	Notes			:	uses ipmi_cmd()
//
*F*/
int GetSelEntry(ushort *pRecordID, SEL_RECORD *selRecord)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	int status;
	uchar inputData[6];
	uchar completionCode;
	char* cpRecordID;	 //for next record id
	ushort inRecordID;    //save the input record id

	inRecordID = *pRecordID;
	if (!selRecord) 
	{
		if (fdebug)
		    SELprintf("GetSelEntry: error, output buffer is NULL\n");
		return -3;
	}

	//set the reservation id to zero
	inputData[0]            = 0;
	inputData[1]            = 0;

	//set the record id to get
	cpRecordID              = (char*) pRecordID;
	inputData[2]            = cpRecordID[0];
	inputData[3]            = cpRecordID[1];

	//set the offset to the record to value zero
	inputData[4]            = 0;

	//set the number of byte to read.  0xFF means read the entire record
	inputData[5]            = (char) 0xFF;

	status = ipmi_cmd(GET_SEL_ENTRY, inputData, 6, responseData, 
			&responseLength, &completionCode, fdebug);

	if (status == ACCESS_OK) {
		if( completionCode ) {
			if (completionCode == 0xCB && inRecordID == 0)
			  SELprintf("Firmware Log (SEL) is empty\n");
			else 
			  SELprintf("GetSelEntry[%x]: completion code=%x\n", 
				inRecordID,completionCode); // responseData[0]);
		} else {
			//successful, done

			//save the next SEL record id
			cpRecordID[0] = responseData[0];
			cpRecordID[1] = responseData[1];

			//save the SEL record content
			//(note that selRecord structure must be pragma_pack'd)
			memcpy(selRecord,&responseData[RECORD_BASE],16);

			if (inRecordID == 0 || inRecordID == selRecord->record_id)
			{
				/* We return success if the input record is 
				   begin-of-SEL (value 0), or             
                                   input and output record id matches.   
				*/
				return 0;
			}
			else 
			{
				/* If last record, inRecordID will be -1 from
				   response data, so return -1 as normal EOF. 
				   (fix to bug 279)
				*/
				if (inRecordID == 0xFFFF) return(-1);
				/* If not last record, this is an error. */
				if (fdebug)
				SELprintf("GetSelEntry: input id  %d != output id %d \n", inRecordID, selRecord->record_id);
				*pRecordID = inRecordID;  //restore the input record id
				return -4;
			}
		}
	}

	// we are here because after the retry, completionCode is not COMPLETION_CODE_OK

	if (fdebug) 
		SELprintf("GetSelEntry: ipmi_cmd error %d completion code, code=%d\n", status,completionCode);
 	return -2;
 
}  /* end GetSelEntry() */

int AddSelEntry(uchar *selrec, int ilen)
{
	uchar rdata[MAX_BUFFER_SIZE];
	int rlen = MAX_BUFFER_SIZE;
	uchar idata[16];
	uchar ccode;
	int rv;

	/* Do not use AddSelEntry unless there is a legitimate 
	   hardware-related event. */
	memset(idata,0,16);
	if (ilen > 16) ilen = 16;
	memcpy(idata,selrec,ilen);
	rv = ipmi_cmdraw(0x44, NETFN_STOR, g_sa, g_bus, g_lun, 
			idata, 16, rdata, &rlen, &ccode, fdebug);
 	return (rv);
}

int get_sel_time_utc_offset(short *offset)
{
	uchar rdata[MAX_BUFFER_SIZE];
	int rlen = MAX_BUFFER_SIZE;
	uchar idata[16];
	uchar ccode;
	int rv;

	if (offset == NULL) return(-1);
	*offset = 0;
	rv = ipmi_cmdraw(0x5C, NETFN_STOR, g_sa, g_bus, g_lun, 
			idata, 0, rdata, &rlen, &ccode, fdebug);
        if (rv == 0) rv = ccode;
        if (rv == 0) *offset = rdata[0] + (rdata[1] << 8);
 	return (rv);
}

void StartWriting(uint *plasttime, ushort *plastid)
{
	FILE *fd;
	uint lasttime; 
	uint lastid;
	int ret = -1;
	
	lasttime = 0;
	lastid = 0;
	// Open the index file
	fd = fopen(idxfile,"r");
	if (fd == NULL) fd = fopen(idxfile2,"r");
	if (fd != NULL) {
		// Read the file, get savtime & savid
		ret = fscanf(fd,"%x %x",&lasttime,&lastid);
		fclose(fd);
	}
	else printf("StartWriting: cannot open %s\n",idxfile);
	if (fdebug) printf("StartWriting: idx fd=%p, savtime=%x, savid=%x\n",
				fd,lasttime,(ushort)lastid);
	*plasttime = lasttime;
	*plastid = (ushort)lastid;

	ret = OpenSyslog("SEL");
	if (fdebug) printf("StartWriting: ret = %d\n",ret);
	return;
}

void StopWriting(uint lasttime, ushort lastid)
{
	FILE *fd;
	// Rewrite the saved time & record id
	fd = fopen(idxfile,"w");
	if (fd != NULL) {
		fprintf(fd,"%x %x\n",lasttime,lastid);
		fclose(fd);
		}
	else printf("StopWriting: cannot open %s\n",idxfile);
	CloseSyslog();
	return;
}

int ClearSEL(void)
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	int status;
	uchar inputData[6];
	uchar completionCode;
	ushort cmd;

	cmd = RESERVE_SEL;  /*0x0A42*/
	status = ipmi_cmd(cmd, inputData, 0, responseData, 
			&responseLength, &completionCode, fdebug);

	if (status == ACCESS_OK && completionCode == 0) {
	    cmd = CLEAR_SEL;  /*0x0A47*/
	    inputData[0] = responseData[0];  /*Reservation ID [0]*/
	    inputData[1] = responseData[1];  /*Reservation ID [1]*/
	    inputData[2] = 'C';
	    inputData[3] = 'L';
	    inputData[4] = 'R';
	    inputData[5] = 0xAA;   /* initiate erase */
	    status = ipmi_cmd(cmd, inputData, 6, responseData, 
	  		      &responseLength, &completionCode, fdebug);
	    /* The reservation is cancelled by the CLEAR_SEL cmd */
	    }
	if (status == ACCESS_OK) {
		if( completionCode ) {
		    SELprintf("ClearSEL: cmd = %x, completion code=%x\n",
			      cmd,completionCode); 
		    status = completionCode;
		} else {
		    /* Successful, done. */
		    SELprintf("ClearSEL: Log Cleared successfully\n"); 
		}
	} else {
	    SELprintf("ClearSEL: cmd = %x, error status = %d\n",
		      cmd,status); 
	}
	if (fwritesel) {  /* this won't always be enough, but try */
	   StartWriting(&savtime,&savid);
	   savid = 0;  
	   StopWriting(savtime,savid);
	}
	return(status);
}  /* ClearSEL()*/


void ReadSEL(uchar mytype, char fwriteit)
{
	ushort RecordID = 0;  /* 0 = first record, 0xFFFF = end */
	SEL_RECORD selRecord;
	SEL_RECORD *pSelRecord = &selRecord;
	char output[160];
	int rc = 0;
        int ilast = 0;
        short recid0;
	uchar *bsel;
	uchar sev;
	char fskipit = 0;

	if (fwriteit) { 
		StartWriting(&savtime,&savid);
		RecordID = savid;
	}
	memset(pSelRecord, 0, sizeof(SEL_RECORD));
        if (flastrecs) { 
            /* read the first two records to get the record id increment */
            RecordID = 0;
	    rc = GetSelEntry( &RecordID, pSelRecord);
	    recid0 = pSelRecord->record_id;
	    rc = GetSelEntry( &RecordID, pSelRecord);
	    if (rc == 0) idinc = pSelRecord->record_id - recid0;
	    else { idinc = recid0; rc = 0; }
            RecordID = 0xFFFF;  /* -1=0xFFFF; * get last record */
            if (fdebug) printf("recid inc = 0x%02x (%x - %x)\n",idinc,
				pSelRecord->record_id,recid0);
        }
        set_sel_opts(fsensdesc, fcanonical, sdrs, fdebug,futc); 
	if (futc) {  /*Try to get the UTC offset*/
	   short utc_off;
	   printf("Showing SEL Time as UTC\n");
	   rc = get_sel_time_utc_offset(&utc_off);
	   if (rc == 0) {
	      printf("SEL Time UTC Offset = %d\n",utc_off);
	   } else rc = 0; /*may fail if not supported, but ok*/
	}
	/* show header for the SEL records */
	if (fcanonical) 
             printf("%s",evt_hdr2);  /*RecId | Date/Time */
	else printf("%s",evt_hdr);   /*RecId Date/Time_______ */
	while( rc == 0 ) {
		rc = GetSelEntry( &RecordID, pSelRecord);
		if (fwriteit && (rc != 0) && (RecordID == savid)) {
		    /* If here, log was probably cleared, so try
		     * again from the log start. */
		    RecordID = 0;
		    rc = GetSelEntry( &RecordID, pSelRecord);
		}
                if (fdebug) printf("rc = %d, recid = %04x, next = %04x\n",
                                   rc, pSelRecord->record_id, RecordID);
                if (flastrecs && (ilast == 0) && (rc == -1)) rc = 0;
		if (rc != 0) { /* EOF or error */ break; }

		if (fshowraw) {
		   bsel = (uchar *)&selRecord;
		   sprintf(output,"%02x %02x %02x %02x %02x %02x %02x %02x "
		                "%02x %02x %02x %02x %02x %02x %02x %02x\n",
				bsel[0], bsel[1], bsel[2], bsel[3],
				bsel[4], bsel[5], bsel[6], bsel[7],
				bsel[8], bsel[9], bsel[10], bsel[11],
				bsel[12], bsel[13], bsel[14], bsel[15]);
		   printf("%s", output);
		} else {
		 if (mytype == 0xff || pSelRecord->sensor_type == mytype) {
		   /* show all records, or type matches */
		   decode_sel_entry((uchar *)pSelRecord,output, sizeof(output));
		   fskipit = 0;
		   if (min_sev > 0) {
		      sev = find_msg_sev(output);
		      if (fdebug) printf("min_sev=%d, sev=%d\n",min_sev,sev);
		      if (sev < min_sev) fskipit = 1;
		   }
		   if (!fskipit) printf("%s", output);
		 } else if ((mytype == ETYPE_CRITSTOP) && 
                           (pSelRecord->record_type >= RTYPE_OEM2)) {
                   /* if showing panics only, also show its oem records */
		   decode_sel_entry((uchar *)pSelRecord,output,sizeof(output));
		   printf("%s", output);
		 } else {
		   if (fdebug) printf("decoding error, mytype = %d\n",mytype);
		   output[0] = 0;
		 }
		}
	
		if (fwriteit) {
		   /* Only write newer records  to syslog */
		   if (pSelRecord->record_type == 0x02) {
			if ((pSelRecord->timestamp > savtime) ||
			    (pSelRecord->record_id > savid)) {
			   WriteSyslog(output);
			   savid = pSelRecord->record_id;
			   savtime = pSelRecord->timestamp;
			}
		   } else {   /* no timestamp */
			if (pSelRecord->record_id > savid) {
			   WriteSyslog(output);
			   savid = pSelRecord->record_id;
			}
		   }
		}  /*endif writeit*/
		if( pSelRecord->record_id == 0xFFFF )
			break;
		if( RecordID == pSelRecord->record_id )
			break;
                if (flastrecs) { 
                    ++ilast;
		    if (fdebug) printf("ilast = %d, next = %x, id = %x\n",
					ilast,RecordID,pSelRecord->record_id);
                    if (ilast >= nlast) break;
                    RecordID = pSelRecord->record_id - idinc;
                    if (RecordID > pSelRecord->record_id) break;
                    if (RecordID == 0) break;
                }
		memset(pSelRecord, 0, sizeof(SEL_RECORD));
	}  /*endwhile*/
	if (fwriteit) StopWriting(savtime,savid);
}  /* end ReadSEL()*/

static uint vfree = 0;
static uint vused = 0;
static uint vtotal = 0;
static uint vsize = REC_SIZE;

static int ReadSELinfo()
{
	uchar responseData[MAX_BUFFER_SIZE];
	int responseLength = MAX_BUFFER_SIZE;
	int status;
	uchar completionCode;
	uchar inputData[6];
	uchar b;
	char *strb;

	status = ipmi_cmd(GET_SEL_INFO, inputData, 0, responseData, 
			&responseLength, &completionCode, fdebug);
	if (fdebug) printf("GetSelInfo status = %x, cc = %x\n",
			status,completionCode);

	if ((status == ACCESS_OK) && (completionCode == 0)) {
		vfree = responseData[3] + (responseData[4] << 8); // in Bytes
		vused = responseData[1] + (responseData[2] << 8); // in Entries/Allocation Units
		vtotal = vused + (vfree/vsize); // vsize from AllocationInfo
		b = responseData[13];
		if (b & 0x80) strb = " overflow"; /*SEL overflow occurred*/
		else strb = "";
                if (b & 0x1) {         // Get SEL Allocation Info supported
                   status = ipmi_cmd(GET_SEL_ALLOCATION_INFO, inputData, 0,
					responseData, &responseLength,
					&completionCode, fdebug);
		   if (fdebug) printf("GetSelInfo status = %x, cc = %x\n",
					status,completionCode);
		   if ((status == ACCESS_OK) && (completionCode == 0)) {
                      vsize  = responseData[2] + (responseData[3] << 8); 
		      if (vsize == 0) vsize = REC_SIZE;
                      vtotal = responseData[0] + (responseData[1] << 8);
                   }
                }

		if (fcanonical) 
		   SELprintf("SEL %s Size = %d records (Used=%d, Free=%d)\n",
		          strb, vtotal, vused, vfree/vsize );
		else 
		   SELprintf("SEL Ver %x Support %02x%s, Size = %d records (Used=%d, Free=%d)\n",
		          responseData[0],
		          b, strb,
			  vtotal,
			  vused,
			  vfree/vsize );
		//successful, done
		return(0);
	} else {
		vfree = MIN_FREE * 2; /*sane values to avoid SEL full warning*/
		vused = 0;
		vtotal = vused + (vfree/vsize);
		return(1);
	}

}  /*end ReadSELinfo()*/


#ifdef ALONE
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int  argc, char **argv)
#else
/* METACOMMAND */
int i_sel(int argc, char **argv)
#endif
{
   int ret = -1;
   char DevRecord[16];
   int c;
   char *s1;
   char *vend_param = NULL;

   printf("%s version %s\n",progname,progver);
   while ((c = getopt(argc,argv,"a:b:cdef:h:i:l:m:np:rs:uwvxM:T:V:J:EYF:P:N:U:R:Z:?")) != EOF)
      switch(c) {
          case 'a': faddsel = 1; /*undocumented option, to prevent misuse*/
		addstr = optarg; /*text string, max 13 bytes, no date*/
		break;
          case 'h': faddsel = 3; /*undocumented option, to prevent misuse*/
		addhex = optarg; /*string of 16 hex characters, no spaces*/
		break;
          case 'i': faddsel = 2; /*undocumented option, to prevent misuse*/
          addstr = optarg; /*text string, max 9 bytes, with date*/
          break;
          case 'b': fdecodebin = 1;    
		rawfile = optarg;
		break;
          case 'd': fclearsel = 1;   break;  /*delete/clear SEL*/
          case 'e': fsensdesc = 1;   break; /*extended sensor descriptions*/
          case 'f': fdecoderaw = 1;    
		rawfile = optarg;
		break;
          case 'l': flastrecs = 1; 
                nlast = atoi(optarg);
                break;
          case 'm': /* specific MC, 3-byte address, e.g. "409600" */
                    g_bus = htoi(&optarg[0]);  /*bus/channel*/
                    g_sa  = htoi(&optarg[2]);  /*device slave address*/
                    g_lun = htoi(&optarg[4]);  /*LUN*/
		    if (optarg[6] == 's') {
		             g_addrtype = ADDR_SMI;  s1 = "SMI";
		    } else { g_addrtype = ADDR_IPMB; s1 = "IPMB"; }
		    fset_mc = 1;
		    printf("set MC at %s bus=%x sa=%x lun=%x\n",
		            s1,g_bus,g_sa,g_lun);
                    break;
          case 'c':
          case 'n': fcanonical = 1;         /*parse easier, canonical*/
                    fsensdesc = 1;          /*extended sensor descriptions*/
		    /* Note that this option does not show event data bytes */
		    break;
          // case 'p': fall = 0;        break; /*crit stop (panic) only*/
          case 'r': fshowraw = 1;    break;
          case 's': min_sev = atob(optarg); break; /*show sev >= value*/
          case 'u': futc = 1;    break;
          case 'v': fonlyver = 1;    break;
          case 'w': fwritesel = 1;   break;
          case 'x': fdebug = 1;      break;
          case 'M':    /* Manufacturing VendorId */
          			vend_param = optarg;  break;
          case 'p':    /* port */
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
          default:
                printf("Usage: %s [-bcdefmnprsuvwx] [-l 5] [-NUPREFTVYZ]\n",
                       progname);
		printf("   -b  interpret Binary file with raw SEL data\n");
		printf("   -c  Show canonical output with delimiters\n");
		printf("   -d  Delete, Clears all SEL records\n");
		printf("   -e  shows Extended sensor description if run locally\n");
		printf("   -f  interpret File with ascii hex SEL data\n");
		printf("   -l5 Show last 5 SEL records (reverse order)\n");
		printf("   -r  Show uninterpreted raw SEL records in ascii hex\n");
		printf("   -n  Show nominal/canonical output (same as -c)\n");
		//printf("   -p  Show only Panic/Critical Stop records\n");
		printf("   -s1 Show only Severity >= value (0,1,2,3)\n");
                printf("   -u  use raw UTC time\n");
		printf("   -v  Only show version information\n");
		printf("   -w  Writes new SEL records to syslog\n");
		printf("   -x  Display extra debug messages\n");
		print_lan_opt_usage(1);
		ret = ERR_USAGE;
		goto do_exit;
      }               
   if (fwritesel && flastrecs) {
       printf("Error: Options -l and -w are incompatible\n");
       ret = ERR_BAD_PARAM;
       goto do_exit;
   }

   /* Handle simple decoding options -f, -b */
   if (fdecoderaw) {  /* -f */
	ret = decode_raw_sel(rawfile,1);
        goto do_exit;
   } else if (fdecodebin) {  /* -b */
	ret = decode_raw_sel(rawfile,2);
	goto do_exit;
   }

   // if (fwrite) fall = 1;   // would we ever set fwrite without fall??
#ifdef WIN32
   fremote = is_remote();
   if (fwritesel) {  /*resolve path of idxfile*/
      char *ipath;
      ipath = getenv("ipmiutildir");  /*ipmiutil directory path*/
      if (ipath != NULL) {
	  if (strlen(ipath)+8 < sizeof(idxfile)) {
	     sprintf(idxfile,"%s\\%s",ipath,"\\",IDXFILE);
	  }
      }
   }
#elif defined(DOS)
   fremote = 0;
#else
   /* Linux, BSD, Solaris */
   fremote = is_remote();
   if (fremote == 0) {
	//uchar guid[16];
	//char gstr[36];
        /* only run this as superuser */
        ret = geteuid();
        if (ret > 1) {
            printf("Not superuser (%d)\n", ret);
	    /* Show warning, but could be ok if /dev/ipmi0 is accessible */
            //ret = ERR_NOT_ALLOWED;
	    //goto do_exit;
        }
   }
#endif
   if (fremote) {  /*remote, ipmi lan, any OS*/
        char *node;
        node = get_nodename();
        strcat(idxfile,"-");
        strcat(idxfile,node);
        strcat(idxfile2,"-");
        strcat(idxfile2,node);
   } 
#ifdef REMOVABLE
   else { 
	// if removable media, may need to add uniqueness to local file. 
	ret = get_SystemGuid(guid);
	if (ret == 0) {
	   sprintf(gstr,"%02X%02X%02X%02X%02X%02X%02X%02X",
		   guid[8], guid[9], guid[10], guid[11],
		   guid[12], guid[13], guid[14], guid[15]);
           strcat(idxfile,"-");
           strcat(idxfile,gstr);
           strcat(idxfile2,"-");
           strcat(idxfile2,gstr);
	}
   } 
#endif
   if (fremote) {
      if (faddsel || fclearsel)
         parse_lan_options('V',"4",0); /*admin priv to clear*/
   }

   if (faddsel) {  /* -a, Add a custom SEL record */
	/* use this sparingly, only for hardware-related events. */
	char buf[16];
	int i, len = 0;
	memset(&buf[0],0,16);
	if (addstr != NULL) {  /*ASCII text string*/
     if (faddsel == 1) {  /* -a, Add a custom SEL record */
	   buf[2] = 0xf1;  /*use SEL type OEM 0xF1*/
	   len = strlen_(addstr);
	   if (len > 13) len = 13;
	   if (len <= 0) ret = LAN_ERR_TOO_SHORT;
	   else memcpy(&buf[3],addstr,len);
	   len += 3;
	 }
     if (faddsel == 2) {  /* -i, Add a custom SEL record with date*/
	   buf[2] = RT_OEMIU; /*use SEL type OEM 0xDB*/
	   memset(&buf[3],0,4);
	   len = strlen_(addstr);
	   if (len > 9) len = 9;
	   if (len <= 0) ret = LAN_ERR_TOO_SHORT;
	   else memcpy(&buf[7],addstr,len);
	   len += 7;
	 }
	}
	if (addhex != NULL) {  /*string of hex characters, no spaces*/
	   len = strlen_(addhex);
	   if (len < 32) ret = LAN_ERR_TOO_SHORT;
	   else {
	      for (i=2; i<16; i++)
	         buf[i] = htoi(&addhex[i*2]);
	   }
	   len = 16;
	}
        if (fdebug) {
           printf("Ready to AddSelEntry: ");
           for (i=0; i<16; i++) printf("%02x ",buf[i]);
           printf("\n");
        }
	if (ret == 0) 
	   ret = AddSelEntry(buf, len);
	printf("AddSelEntry ret = %d\n",ret);
        goto do_exit;
   } 

   ret = ipmi_getdeviceid( DevRecord, sizeof(DevRecord),fdebug);
   if (ret == 0) {
      uchar ipmi_maj, ipmi_min;
      ipmi_maj = DevRecord[4] & 0x0f;
      ipmi_min = DevRecord[4] >> 4;
      show_devid( DevRecord[2],  DevRecord[3], ipmi_maj, ipmi_min);
      prod_id = DevRecord[9] + (DevRecord[10] << 8);
      vend_id = DevRecord[6] + (DevRecord[7] << 8) + (DevRecord[8] << 16);
      if (vend_id == VENDOR_INTEL) {
         if (prod_id == 0x003E)   /*Urbanna NSN2U or CG2100*/
	           set_max_kcs_loops(URNLOOPS); /*longer KCS timeout*/
      }
   } else {
      goto do_exit;
   }
   if (vend_param != NULL) set_iana(atoi(vend_param));

   ret = ReadSELinfo();
   if (ret == 0 && !fonlyver) {
      if (fclearsel) {
	 ret = ClearSEL();
      } else {
         if (fsensdesc) {
	     if (fdebug) printf("%s: fetching SDRs ...\n",progname);
             ret = get_sdr_cache(&sdrs);
	     if (fdebug) printf("%s: get_sdr_cache ret = %d\n",progname,ret);
	     ret = 0; /*if error, keep going anyway*/
	 }
	 if (fdebug) printf("%s: starting ReadSEL ...\n",progname);
         if (fall) ReadSEL(0xff,fwritesel); /* show all SEL records */
         else ReadSEL(ETYPE_CRITSTOP,fwritesel);  /* only show OS Crit Stops*/
	 /* PEF alerts and other log messages fail if low free space,
	    so show a warning. */
	 if (vfree < MIN_FREE) {
             printf("WARNING: free space is very low (=%d), need to clear with -d\n",
		    vfree);
	 } else if ((vfree/vsize) < ((vtotal * 20)/100)) {
             printf("WARNING: free space is low (=%d), need to clear with -d\n",
		    vfree);
	 }
      }
   }
do_exit:
   free_sdr_cache(sdrs);
   ipmi_close_();
   // show_outcome(progname,ret); 
   return(ret);
}

/* end isel.c */
