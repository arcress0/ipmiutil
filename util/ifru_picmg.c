/*
 * ifru_picmg.c
 * These are helper routines for FRU PICMG decoding.
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 *
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 09/08/10 Andy Cress - created
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
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#include "getopt.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#endif
#include <time.h>

#include "ipmicmd.h" 
#include "ipicmg.h" 

extern int verbose;
extern void show_guid(uchar *pguid);  /*from ifru.c*/

static char *picmg_link_type(uchar b)
{
   char *s;
   if (b >= 0xf0 && b <= 0xf3) s = "OEM GUID Definition";
   else switch(b) {
   case 0x01: s = "PICMG 3.0 Base Interface 10/100/1000"; break;
   case 0x02: s = "PICMG 3.1 Ethernet Fabric Interface"; break;
   case 0x03: s = "PICMG 3.2 Infiniband Fabric Interface"; break;
   case 0x04: s = "PICMG 3.3 Star Fabric Interface"; break;
   case 0x05: s = "PICMG 3.4 PCI Express Fabric Interface"; break;
   default:   s = "Reserved"; break;
   }
   return(s);
}

static char *picmg_if_type(uchar b)
{
   char *s;
   switch(b) {
   case 0x00: s = "Base Interface"; break; 
   case 0x01: s = "Fabric Interface"; break;
   case 0x02: s = "Update Channel"; break; 
   case 0x03: 
   default:   s = "Reserved"; break;
   }
   return(s);
}

static char *picmg_chan_type(uchar b)
{
   char *s;
   switch(b) {
   case 0x00:
   case 0x07: s = "PICMG 2.9"; break; 
   case 0x08: s = "Single Port Fabric IF"; break;
   case 0x09: s = "Double Port Fabric IF"; break;
   case 0x0a: s = "Full Port Fabric IF"; break;
   case 0x0b: s = "Base IF"; break;
   case 0x0c: s = "Update Channel IF"; break; 
   default:   s = "Unknown IF"; break;
   }
   return(s);
}

void show_fru_picmg(uchar *pdata, int dlen)
{
   int id, i, j, k, m, n;
   float f;
   uchar *p;
   uchar b0, b1, b2, b3, b4, b5, b6, b7, b8;
   char *s1, *s2;
   long l1, l2, l3;

   id = pdata[3];
   i = 5;
   switch(id) {
	case FRU_PICMG_BACKPLANE_P2P:
		printf("\tFRU_PICMG_BACKPLANE_P2P\n");
		while (i <= dlen) {
		   b1 = pdata[i]; 
		   b2 = pdata[i+1]; 
		   b3 = pdata[i+2]; 
		   printf("\t  Channel Type     : %02x - %s\n",b1,
				picmg_chan_type(b1));
		   printf("\t  Slot Address     : %02x\n",b2);
		   printf("\t  Channel Count    : %02x\n",b3);
		   i += 3;
		   for (j = 0; j < b3; j++) {
		      if (i > dlen) break;
		      if (verbose) 
			 printf("\t  Channel[%d] : %02x -> %02x in slot %02x\n",
					j,pdata[i],pdata[i+1],pdata[i+2]);
		      i += 3;
		   }
		}
		break;
	case FRU_PICMG_ADDRESS_TABLE:
		printf("\tFRU_PICMG_ADDRESS_TABLE\n");
		printf("\t  Type/Len         : %02x\n", pdata[i++]);
		printf("\t  Shelf Addr       : ");
		for (j = 0; j < 20; j++) 
		   printf("%02x ",pdata[i++]);
		printf("\n");
		n = pdata[i++];
		printf("\t  AddrTable Entries: %02x\n",n);
		for (j = 0; j < n; j++) {
		   if (i >= dlen) break;
		   printf("\t    HWAddr %02x, SiteNum %02x, SiteType %02x\n",
			  pdata[i], pdata[i+1], pdata[i+2]);
		   i += 3;
		}
		break;
	case FRU_PICMG_SHELF_POWER_DIST:
		printf("\tFRU_PICMG_SHELF_POWER_DIST\n");
		n = pdata[i++];
		printf("\t  Num Power Feeds  : %02x\n",n);
		for (j = 0; j < n; j++) {
		   if (i >= dlen) break;
		   printf("\t    Max Ext Current  : %04x\n", 
				(pdata[i] | (pdata[i+1] << 8)));
		   i += 2;
		   printf("\t    Max Int Current  : %04x\n", 
				(pdata[i] | (pdata[i+1] << 8)));
		   i += 2;
		   printf("\t    Min Exp Voltage  : %02x\n", pdata[i++]);
		   m = pdata[i++];  /*num entries*/
		   printf("\t    Feed to FRU count: %02x\n", m);
		   for (k = 0; k < m; k++) {
		      if (i >= dlen) break;
		      printf("\t      HW: %02x",pdata[i++]);
		      printf("   FRU ID: %02x\n",pdata[i++]);
		   }
		}
		break;
	case FRU_PICMG_SHELF_ACTIVATION:
		printf("\tFRU_PICMG_SHELF_ACTIVATION\n");
		printf("\t  Allowance for FRU Act Readiness  : %02x\n",
			pdata[i++]);
		n = pdata[i++];
		printf("\t  FRU activation and Power desc Cnt: %02x\n",n);
		for (j = 0; j < n; j++) {
		   if (i >= dlen) break;
		   printf("\t    HW Addr: %02x, ", pdata[i++]);
		   printf(" FRU ID: %02x, ", pdata[i++]);
		   printf(" Max FRU Power: %04x, ", pdata[i] | (pdata[i+1]<<8));
		   i += 2;
		   printf(" Config: %02x\n", pdata[i++]);
		}
		break;
	case FRU_PICMG_SHMC_IP_CONN:
		printf("\tFRU_PICMG_SHMC_IP_CONN\n");
		printf("\t  Conn Data: ");
		for (  ; i < dlen; i++) {
		   printf("%02x ",pdata[i]);
		}
		printf("\n");
		break;
	case FRU_PICMG_BOARD_P2P:  /*0x14*/
		printf("\tFRU_PICMG_BOARD_P2P\n");
		n = pdata[i++];  /*guid count*/
		printf("\t  GUID count      : %d\n",n);
		for (j = 0; j < n; j++) {
		   printf("\t  GUID[%d]         : ",j);
		   show_guid(&pdata[i]);
		   printf("\n");
		   i += 16;
		}
		for (j = 1; i < dlen; i += 4) {
		   p = &pdata[i];
		   b1 = (p[0] & 0x3f);     /*chan*/
		   b2 = (p[0] & 0xc0) >> 6;  /*if*/
		   b3 = (p[1] & 0x0f);     /*port*/
		   b4 = ((p[1] & 0xf0) >> 4) + (p[2] & 0xf0);  /*type*/ 
		   b5 = (p[2] & 0x0f);  /*ext*/
		   b6 = p[3];  /*grouping*/
		   printf("\t  Link%d Grouping  : %02x\n",j,b6);
		   printf("\t  Link%d Extension : %02x\n",j,b5);
		   printf("\t  Link%d Type      : %02x - %s\n",
				j,b4,picmg_link_type(b4));
		   printf("\t  Link%d Port      : %02x\n",j,b3);
		   printf("\t  Link%d Interface : %02x - %s\n",
				j,b2,picmg_if_type(b2));
		   printf("\t  Link%d Channel   : %02x\n",j,b1);
		   j++;
		}
		break;
	case FRU_AMC_CURRENT:
		printf("\tFRU_AMC_CURRENT\n");
		b1 = pdata[i];  /*current*/
		f = (float)(b1/10.0);
		printf("\t  Current draw: %.1f A @ 12V => %.2f Watt\n",
			f, (f*12.0));
		break;
	case FRU_AMC_ACTIVATION:
		printf("\tFRU_AMC_ACTIVATION\n");
		b1 = pdata[i] | (pdata[i+1]<<8); /*max current*/
		i += 2;
		f = (float)b1 / 10;
		printf("\t  Max Internal Current(@12V) : %.2f A [ %.2f Watt ]\n",
			f, f*12);
		printf("\t  Module Activation Readiness: %i sec.\n",pdata[i++]);
		n = pdata[i++];
		printf("\t  Descriptor Count: %i\n",n);
		for (j = 0; i < dlen; i += 3) {
		   if (j >= n) break;
		   printf("\t    IPMB Address      : %02x\n",pdata[i]);
		   printf("\t    Max Module Current: %.2f A\n",
			  (float)(pdata[i+1]/10));
		   j++;
		}
		break;
	case FRU_AMC_CARRIER_P2P:
		printf("\tFRU_AMC_CARRIER_P2P\n");
		for ( ; i < dlen; ) {
		   b1 = pdata[i];  /*resource id*/
		   n  = pdata[i+1]; /*desc count*/
		   i += 2;
		   b2 = (b1 >> 7);
		   printf("\t  Resource ID: %i, Type: %s\n",
			(b1 & 0x07), (b2 == 1 ? "AMC" : "Local"));
		   printf("\t  Descriptor Count: %i\n",n);
		   for (j = 0; j < n; j++) {
		       if (i >= dlen) break;
		       p = &pdata[i];
		       b3 = p[0];          /*remote resource id*/
		       b4 = (p[1] & 0x1f); /*remote port*/
		       b5 = (p[1] & 0xe0 >> 5) + (p[2] & 0x03 << 3); /*local*/
		       printf("\t  Port %02d -> Remote Port %02d " 
			      "[ %s ID: %02d ]\n", b5, b4, 
			      ((b3 >> 7) == 1)? "AMC  " : "local", b3 & 0x0f);
		       i += 3;
		   }
		}
		break;
	case FRU_AMC_P2P:
		printf("\tFRU_AMC_P2P\n");
		n = pdata[i++];  /*guid count*/
		printf("\t  GUID count      : %d\n",n);
		for (j = 0; j < n; j++) {
		   printf("\t  GUID[%d]         : ",j);
		   show_guid(&pdata[i]);
		   printf("\n");
		   i += 16;
		}
		b1 = pdata[i] & 0x0f;        /*resource id*/
		b2 = (pdata[i] & 0x80) >> 7; /*resource type*/
		i++;
		printf("\t  Resource ID: %i - %s\n",b1, 
			b2 ? "AMC Module" : "On-Carrier Device");
		n = pdata[i++];        /*descriptor count*/
		printf("\t  Descriptor Count: %i\n",n);
		for (j = 0; j < n; j++) {  /*channel desc loop*/
		   if (i >= dlen) break;
		   p = &pdata[i];
		   b0 = p[0] & 0x1f;
		   b1 = ((p[0] & 0xe0) >> 5) + ((p[1] & 0x03) << 3);
		   b2 = ((p[1] & 0x7c) >> 2);
		   b3 = ((p[1] & 0x80) >> 7) + ((p[2] & 0x0f) << 1);
		   printf("\t    Lane 0 Port: %i\n",b0);
		   printf("\t    Lane 1 Port: %i\n",b1);
		   printf("\t    Lane 2 Port: %i\n",b2);
		   printf("\t    Lane 3 Port: %i\n",b3);
		   i += 3;
		}
		for ( ; i < dlen; i += 5) {  /*ext descriptor loop*/
		   p = &pdata[i];
		   b0 = p[0];                /*channel id*/
		   b1 = p[1] & 0x01;         /*port flag 0*/
		   b2 = (p[1] & 0x02) >> 1;  /*port flag 1*/
		   b3 = (p[1] & 0x04) >> 2;  /*port flag 2*/
		   b4 = (p[1] & 0x08) >> 3;  /*port flag 3*/
		   b5 = ((p[1] & 0xf0) >> 4) + ((p[2] & 0x0f) << 4); /*type*/
		   b6 = (p[2] & 0xf0) >> 4;  /*type ext*/
		   b7 = p[3];                /*group id*/
		   b8 = (p[4] & 0x03);       /*asym match*/
		   printf("\t  Link Designator: Channel ID: %i, "
			  "Port Flag 0: %s%s%s%s\n",b0,
			  b1 ? "o" : "-", 
			  b2 ? "o" : "-", 
			  b3 ? "o" : "-", 
			  b4 ? "o" : "-" );
		   switch(b5) {  /*link type*/
		   case FRU_PICMGEXT_AMC_LINK_TYPE_PCIE:   
			printf("\t  Link Type:     %02x - %s\n", b5,
			       "AMC.1 PCI Express");
			switch(b6) {  /*link type ext*/
			case AMC_LINK_TYPE_EXT_PCIE_G1_NSSC: 
			   s2 = "Gen 1 capable - non SSC"; break;
			case AMC_LINK_TYPE_EXT_PCIE_G1_SSC:
			   s2 = "Gen 1 capable - SSC"; break;
			case AMC_LINK_TYPE_EXT_PCIE_G2_NSSC:
			   s2 = "Gen 2 capable - non SSC"; break;
                        case AMC_LINK_TYPE_EXT_PCIE_G2_SSC:
			   s2 = "Gen 2 capable - SSC"; break;
			default:
			   s2 = "Invalid"; break;
			}
			printf("\t  Link Type Ext: %02x - %s\n", b6,s2);
			break;
		   case FRU_PICMGEXT_AMC_LINK_TYPE_PCIE_AS1:
		   case FRU_PICMGEXT_AMC_LINK_TYPE_PCIE_AS2:
			printf("\t  Link Type:     %02x - %s\n", b5,
			       "AMC.1 PCI Express Advanced Switching");
			printf("\t  Link Type Ext: %02x\n", b6);
			break;
		   case FRU_PICMGEXT_AMC_LINK_TYPE_ETHERNET:
			s1 = "AMC.2 Ethernet";
			printf("\t  Link Type:     %02x - %s\n", b5,s1);
			switch(b6) {  /*link type ext*/
			case AMC_LINK_TYPE_EXT_ETH_1000_BX:
			   s2 = "1000Base-Bx (SerDES Gigabit) Ethernet Link"; 
			   break;
			case AMC_LINK_TYPE_EXT_ETH_10G_XAUI:
			   s2 = "10Gbit XAUI Ethernet Link"; break;
			default: 
			   s2 = "Invalid"; break;
			}
			printf("\t  Link Type Ext: %02x - %s\n", b6,s2);
			break;
		   case FRU_PICMGEXT_AMC_LINK_TYPE_STORAGE:
			printf("\t  Link Type:     %02x - %s\n", b5,
			       "AMC.3 Storage");
			switch(b6) {  /*link type ext*/
			case AMC_LINK_TYPE_EXT_STORAGE_FC:
			   s2 = "Fibre Channel"; break;
			case AMC_LINK_TYPE_EXT_STORAGE_SATA:
			   s2 = "Serial ATA"; break;
			case AMC_LINK_TYPE_EXT_STORAGE_SAS:
			   s2 = "Serial Attached SCSI"; break;
			default: 
			   s2 = "Invalid"; break;
			}
			printf("\t  Link Type Ext: %02x - %s\n", b6,s2);
			break;
		   case FRU_PICMGEXT_AMC_LINK_TYPE_RAPIDIO:
			printf("\t  Link Type:     %02x - %s\n", b5,
				"AMC.4 Serial Rapid IO");
			printf("\t  Link Type Ext: %02x\n", b6);
			break;
		   default:
			printf("\t  Link Type:     %02x - %s\n", b5,
				"reserved or OEM GUID");
			printf("\t  Link Type Ext: %02x\n", b6);
			break;
		   } /*end switch(link_type)*/
		   printf("\t  Link group Id:   %i\n", b7);
		   printf("\t  Link Asym Match: %i\n", b8);
		} /*end ext descriptor loop*/
		break;
	case FRU_AMC_CARRIER_INFO:
		printf("\tFRU_AMC_CARRIER_INFO\n");
		b1 = pdata[i++];  /*extVersion*/
		n  = pdata[i++];  /*siteCount*/
		printf("\t  AMC.0 extension version: R%d.%d\n",
			b1 & 0x0f, (b1 >> 4) & 0x0f);
		printf("\t  Carrier Site Count: %d\n",n);
		for (j = 0; j < n; j++) {
		   if (i >= dlen) break;
		   printf("\t    Site ID: %i\n", pdata[i++]);
		}
		break;
	case FRU_PICMG_CLK_CARRIER_P2P:
		printf("\tFRU_PICMG_CLK_CARRIER_P2P\n");
		b0 = pdata[i++];
		n  = pdata[i++];
		k = (b0 & 0xC0) >> 6;
		switch(k) {
		case 0: s1 = "On-Carrier-Device"; break;
		case 1: s1 = "AMC slot"; break;
		case 2: s1 = "Backplane"; break;
		default: s1 = "reserved"; break;
		}
		printf("\t  Clock Resource ID: %02x, Type: %s\n",b0,s1 );
		printf("\t  Channel Count    : %02x\n",n);
		for (j = 0; j < n; j++) {
		   b1 = pdata[i++]; /*local channel*/
		   b2 = pdata[i++]; /*remote channel*/
		   b3 = pdata[i++]; /*remote resource*/
		   k = (b3 & 0xC0) >> 6;
		   switch(k) {
		   case 0: s1 = "Carrier-Dev"; break;
		   case 1: s1 = "AMC slot   "; break;
		   case 2: s1 = "Backplane  "; break;
		   default: s1 = "reserved   "; break;
		   }
		   printf("\t  CLK-ID: %02x -> %02x [ %s %02x ]\n",b1,b2,s1,b3);
		}
		break;
	case FRU_PICMG_CLK_CONFIG:
		printf("\tFRU_PICMG_CLK_CONFIG\n");
		b0 = pdata[i++];
		n  = pdata[i++];
		printf("\t  Clock Resource ID: %02x\n",b0);
		printf("\t  Descriptor Count : %02x\n",n);
		for (j = 0; j < n; j++) {
		   b1 = pdata[i++]; /*channel id*/
		   b2 = pdata[i++]; /*control*/
		   printf("\t  CLK-ID: %02x - CTRL %02x [ %12s ]\n", b1, b2, 
			  (b2 & 0x01) == 0 ? "Carrier IPMC":"Application");
		   b3 = pdata[i++]; /*indirect_cnt*/
		   b4 = pdata[i++]; /*direct_cnt*/
		   printf("\t  Cnt: Indirect %02x / Direct %02x\n",b3,b4);
		   for (k = 0; k < b3; k++) {
		      b5 = pdata[i++]; /*feature*/
		      b6 = pdata[i++]; /*dep_chn_id*/
		      printf("\t    Feature: %02x [%8s] - ",
			    b5, (b5 & 0x01)==1 ? "Source":"Receiver");
		      printf("Dep. CLK-ID: %02x\n",b6);
		   }
		   for (k = 0; k < b4; k++) {
		      b5 = pdata[i++]; /*feature*/
		      b6 = pdata[i++]; /*family*/
		      b7 = pdata[i++]; /*accuracy*/
		      l1 = pdata[i] | (pdata[i+1] << 8) | /*frequency*/
		           (pdata[i+2] << 8) | (pdata[i+3] << 8);
		      i += 4;
		      l2 = pdata[i] | (pdata[i+1] << 8) | /*min frequency*/
		           (pdata[i+2] << 8) | (pdata[i+3] << 8);
		      i += 4;
		      l3 = pdata[i] | (pdata[i+1] << 8) | /*max frequency*/
		           (pdata[i+2] << 8) | (pdata[i+3] << 8);
		      i += 4;
		      printf("\t    Feature: %02x - PLL: %x / Asym: %s\n",
			b5, (b5 >> 1) & 0x01, (b5 & 1) ? "Source":"Receiver");
		      printf("\t    Family : %02x - AccLVL: %02x\n", b6, b7);
		      printf("\t    FRQ    : %-9d, min: %-9d, max: %-9d\n",
				(int)l1, (int)l2, (int)l3);
		   }
		}
		break;
	case FRU_UTCA_FRU_INFO_TABLE:
	case FRU_UTCA_CARRIER_MNG_IP:
	case FRU_UTCA_CARRIER_INFO:
	case FRU_UTCA_CARRIER_LOCATION:
	case FRU_UTCA_SHMC_IP_LINK:
	case FRU_UTCA_POWER_POLICY:
	case FRU_UTCA_ACTIVATION:
	case FRU_UTCA_PM_CAPABILTY:
	case FRU_UTCA_FAN_GEOGRAPHY:
	case FRU_UTCA_CLOCK_MAPPING:
	case FRU_UTCA_MSG_BRIDGE_POLICY:
	case FRU_UTCA_OEM_MODULE_DESC:
		printf("\tNot yet implemented uTCA record %x\n",id);
		break;
	default:
		printf("\tUnknown PICMG Extension %x\n",id);
		break;
   }
} /*end show_fru_picmg*/

/*end ifru_picmg.c */
