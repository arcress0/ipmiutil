/*
 * hpifrua.c (hpifru.c for HPI_A)
 *
 * Author:  Bill Barkley & Andy Cress
 * Copyright (c) 2003-2005 Intel Corporation.
 *
 * 04/18/03 
 * 06/09/03 - new CustomField parsing, including SystemGUID
 * 02/19/04 ARCress - generalized BMC tag parsing, created IsTagBmc()
 * 05/05/04 ARCress - added OpenHPI Mgt Ctlr tag to IsTagBmc()
 * 09/22/04 ARCress - inbuff size bigger, check null ptr in fixstr
 * 01/11/05 ARCress - skip INVENTORY RDRs that return errors
 * 03/16/05 ARCress - make sure asset tag is BMC for writes
 */
/*M*
Copyright (c) 2003-2005, Intel Corporation
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "SaHpi.h"

#define NCT 25

char progver[] = "1.4 HPI-A";
char progname[] = "hpifru";
char *chasstypes[NCT] = {
	"Not Defined", "Other", "Unknown", "Desktop", "Low Profile Desktop",
	"Pizza Box", "Mini Tower", "Tower", "Portable", "Laptop",
	"Notebook", "Handheld", "Docking Station", "All in One", "Subnotebook",
	"Space Saving", "Lunch Box", "Main Server", "Expansion", "SubChassis",
	"Buss Expansion Chassis", "Peripheral Chassis", "RAID Chassis", 
	"Rack-Mount Chassis", "New"
};
int fasset = 0;
int fdebug = 0;
int fxdebug = 0;
int i,j,k = 0;
SaHpiUint32T buffersize;
SaHpiUint32T actualsize;
char *asset_tag;
char inbuff[2048];
char outbuff[1024];
SaHpiInventoryDataT *inv;
SaHpiInventChassisTypeT chasstype;
SaHpiInventGeneralDataT *dataptr;
SaHpiTextBufferT *strptr;

#ifdef ANYINVENT
static int 
IsTagBmc(char *dstr, int dlen)
{
   /* if OpenHPI, always return TRUE for any Inventory RDR */
   return(1);
}
#else
char bmctag[]  = "Basbrd Mgmt Ctlr";       /* see IsTagBmc() */
char bmctag2[] = "Management Controller";  /* see IsTagBmc() */
/*
 * findmatch
 * returns offset of the match if found, or -1 if not found.
 */
static int
findmatch(char *buffer, int sbuf, char *pattern, int spattern, char figncase)
{
    int c, j, imatch;
    j = 0;
    imatch = 0;
    for (j = 0; j < sbuf; j++) {
        if (sbuf - j < spattern && imatch == 0) return (-1);
        c = buffer[j];
        if (c == pattern[imatch]) {
            imatch++;
            if (imatch == spattern) return (++j - imatch);
        } else if (pattern[imatch] == '?') {  /*wildcard char*/
            imatch++;
            if (imatch == spattern) return (++j - imatch);
        } else if (figncase == 1) {
            if ((c & 0x5f) == (pattern[imatch] & 0x5f)) {
                imatch++;
                if (imatch == spattern) return (++j - imatch);
            } else
                imatch = 0;
        } else
            imatch = 0;
    }
    return (-1);
}                               /*end findmatch */

static int 
IsTagBmc(char *dstr, int dlen)
{
   int ret = 0;
   if (strncmp(dstr, bmctag, sizeof(bmctag)) == 0)  /* Sahalee */
	ret = 1;
   else if (findmatch(dstr,dlen,"BMC",3,1) >= 0) /* mBMC or other */
	ret = 1;
   else if (findmatch(dstr,dlen,bmctag2,sizeof(bmctag2),1) >= 0)
        ret = 1;    /* BMC tag for OpenHPI with ipmi plugin */
   return(ret);
}
#endif

static void
fixstr(SaHpiTextBufferT *strptr)
{ 
	size_t datalen;

	outbuff[0] = 0;
	if (strptr == NULL) return;
	datalen = strptr->DataLength;
	if (datalen > sizeof(outbuff)) datalen = sizeof(outbuff) - 1;
	if (datalen != 0) {
		strncpy ((char *)outbuff, (char *)strptr->Data, datalen);
		outbuff[datalen] = 0;
		if (fdebug) { 
		  printf("TextBuffer: %s, len=%d, dtype=%x lang=%d\n",
			outbuff, strptr->DataLength, 
			strptr->DataType, strptr->Language);
		}
	}
}

static void
prtchassinfo(void)
{
  chasstype = (SaHpiInventChassisTypeT)inv->DataRecords[i]->RecordData.ChassisInfo.Type;
  for (k=0; k<NCT; k++) {
	  if (k == chasstype)
		  printf( "Chassis Type        : %s\n", chasstypes[k]);
  }	  

  dataptr = (SaHpiInventGeneralDataT *)&inv->DataRecords[i]->RecordData.ChassisInfo.GeneralData;
  strptr=dataptr->Manufacturer;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Chassis Manufacturer: %s\n", outbuff);

  strptr=dataptr->ProductName;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Chassis Name        : %s\n", outbuff);

  strptr=dataptr->ProductVersion;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Chassis Version     : %s\n", outbuff);

  strptr=dataptr->ModelNumber;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Chassis Model Number: %s\n", outbuff);

  strptr=dataptr->SerialNumber;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Chassis Serial #    : %s\n", outbuff);

  strptr=dataptr->PartNumber;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Chassis Part Number : %s\n", outbuff);

  strptr=dataptr->FileId;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Chassis FRU File ID : %s\n", outbuff);

  strptr=dataptr->AssetTag;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Chassis Asset Tag   : %s\n", outbuff);
  if (dataptr->CustomField[0] != 0)
  {
    if (dataptr->CustomField[0]->DataLength != 0)
      strncpy ((char *)outbuff, (char *)dataptr->CustomField[0]->Data,
              dataptr->CustomField[0]->DataLength);
    outbuff[dataptr->CustomField[0]->DataLength] = 0;
    printf( "Chassis OEM Field   : %s\n", outbuff);
  }
}

static void
prtprodtinfo(void)
{
  int j;
  dataptr = (SaHpiInventGeneralDataT *)&inv->DataRecords[i]->RecordData.ProductInfo;
  strptr=dataptr->Manufacturer;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Product Manufacturer: %s\n", outbuff);

  strptr=dataptr->ProductName;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Product Name        : %s\n", outbuff);

  strptr=dataptr->ProductVersion;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Product Version     : %s\n", outbuff);

  strptr=dataptr->ModelNumber;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Product Model Number: %s\n", outbuff);

  strptr=dataptr->SerialNumber;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Product Serial #    : %s\n", outbuff);

  strptr=dataptr->PartNumber;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Product Part Number : %s\n", outbuff);

  strptr=dataptr->FileId;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Product FRU File ID : %s\n", outbuff);

  strptr=dataptr->AssetTag;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Product Asset Tag   : %s\n", outbuff);

  for (j = 0; j < 10; j++) {
     int ii;
     if (dataptr->CustomField[j] != NULL) {
      if ((dataptr->CustomField[j]->DataType == 0) &&
          (dataptr->CustomField[j]->DataLength == 16)) { /*binary GUID*/
         printf( "IPMI SystemGUID     : ");
         for (ii=0; ii< dataptr->CustomField[j]->DataLength; ii++)
              printf("%02x", dataptr->CustomField[j]->Data[ii]);
         printf("\n");
      } else {  /* other text field */
         dataptr->CustomField[j]->Data[
              dataptr->CustomField[j]->DataLength] = 0;
	 if (fdebug) { 
		printf("TextBuffer: %s, len=%d, dtype=%x lang=%d\n",
			dataptr->CustomField[j]->Data,
			dataptr->CustomField[j]->DataLength,
                	dataptr->CustomField[j]->DataType,
                	dataptr->CustomField[j]->Language);
	 }
         printf( "Product OEM Field   : %s\n",
              dataptr->CustomField[j]->Data);
      }
     } else /* NULL pointer */
      break;
  } /*end for*/
}

static void
prtboardinfo(void)
{
  dataptr = (SaHpiInventGeneralDataT *)&inv->DataRecords[i]->RecordData.BoardInfo;
  strptr=dataptr->Manufacturer;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Board Manufacturer  : %s\n", outbuff);

  strptr=dataptr->ProductName;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Board Product Name  : %s\n", outbuff);

  strptr=dataptr->ModelNumber;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Board Model Number  : %s\n", outbuff);

  strptr=dataptr->PartNumber;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Board Part Number   : %s\n", outbuff);

  strptr=dataptr->ProductVersion;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Board Version       : %s\n", outbuff);

  strptr=dataptr->SerialNumber;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Board Serial #      : %s\n", outbuff);

  strptr=dataptr->FileId;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Board FRU File ID   : %s\n", outbuff);

  strptr=dataptr->AssetTag;
  fixstr((SaHpiTextBufferT *)strptr);
  printf( "Board Asset Tag     : %s\n", outbuff);

  if (dataptr->CustomField[0] != 0)
  {
    if (dataptr->CustomField[0]->DataLength != 0)
      strncpy ((char *)outbuff, (char *)dataptr->CustomField[0]->Data,
              dataptr->CustomField[0]->DataLength);
    outbuff[dataptr->CustomField[0]->DataLength] = 0;
    printf( "Board OEM Field     : %s\n", outbuff);
  }
}

int
main(int argc, char **argv)
{
  int prodrecindx=0;
  int asset_len=0;
  int c;
  SaErrorT rv;
  SaHpiVersionT hpiVer;
  SaHpiSessionIdT sessionid;
  SaHpiRptInfoT rptinfo;
  SaHpiRptEntryT rptentry;
  SaHpiEntryIdT rptentryid;
  SaHpiEntryIdT nextrptentryid;
  SaHpiEntryIdT entryid;
  SaHpiEntryIdT nextentryid;
  SaHpiResourceIdT resourceid;
  SaHpiRdrT rdr;
  SaHpiEirIdT eirid;

  printf("%s ver %s\n",progname,progver);

  while ( (c = getopt( argc, argv,"a:xz?")) != EOF )
  switch(c) {
    case 'z': fxdebug = 1; /* fall thru to include next setting */
    case 'x': fdebug = 1; break;
    case 'a':
          fasset = 1;
          if (optarg) 
          {
	    asset_tag = (char *)strdup(optarg);
	    asset_len = strlen(optarg);
	  }
	  /*
	  printf( "String Length = %d\n", asset_len);
	  printf( "String Length = %d\n", strlen(optarg));
	  */
          break;
    default:
          printf("Usage: %s [-x] [-a asset_tag]\n", progname);
          printf("   -a  Sets the asset tag\n");
          printf("   -x  Display debug messages\n");
          printf("   -z  Display extra debug messages\n");
          exit(1);
  }
  inv = (SaHpiInventoryDataT *)&inbuff[0];
  rv = saHpiInitialize(&hpiVer);
  if (rv != SA_OK) {
    printf("saHpiInitialize error %d\n",rv);
    exit(-1);
  }
  rv = saHpiSessionOpen(SAHPI_DEFAULT_DOMAIN_ID,&sessionid,NULL);
  if (rv != SA_OK) {
    printf("saHpiSessionOpen error %d\n",rv);
    exit(-1);
  }
 
  rv = saHpiResourcesDiscover(sessionid);
  if (fxdebug) printf("saHpiResourcesDiscover rv = %d\n",rv);
  rv = saHpiRptInfoGet(sessionid,&rptinfo);
  if (fxdebug) printf("saHpiRptInfoGet rv = %d\n",rv);
  if (fdebug) printf("RptInfo: UpdateCount = %x, UpdateTime = %lx\n",
         rptinfo.UpdateCount, (unsigned long)rptinfo.UpdateTimestamp);
 
  /* walk the RPT list */
  rptentryid = SAHPI_FIRST_ENTRY;
  while ((rv == SA_OK) && (rptentryid != SAHPI_LAST_ENTRY))
  {
    rv = saHpiRptEntryGet(sessionid,rptentryid,&nextrptentryid,&rptentry);
    if (rv != SA_OK) printf("RptEntryGet: rv = %d\n",rv);
    if (rv == SA_OK)
    {
      /* walk the RDR list for this RPT entry */
      entryid = SAHPI_FIRST_ENTRY;
      /* OpenHPI sometimes has bad RPT DataLength here. */
      rptentry.ResourceTag.Data[rptentry.ResourceTag.DataLength] = 0;
      resourceid = rptentry.ResourceId;
      printf("Resource Tag: %s\n", rptentry.ResourceTag.Data);
      if (fdebug) printf("rptentry[%d] resourceid=%d\n", rptentryid,resourceid);
      while ((rv == SA_OK) && (entryid != SAHPI_LAST_ENTRY))
      {
        rv = saHpiRdrGet(sessionid,resourceid, entryid,&nextentryid, &rdr);
  	if (fxdebug) printf("saHpiRdrGet[%d] rv = %d\n",entryid,rv);
	if (rv == SA_OK)
	{
	  if (rdr.RdrType == SAHPI_INVENTORY_RDR)
	  { 
	    /*type 3 includes inventory records*/
	    eirid = rdr.RdrTypeUnion.InventoryRec.EirId;
	    rdr.IdString.Data[rdr.IdString.DataLength] = 0;	    
	    if (fdebug) printf( "RDR[%d]: Inventory, type=%d num=%d %s\n", 
		    rdr.RecordId, rdr.RdrType, eirid, rdr.IdString.Data);
	    else printf("RDR[%d]: %s \n",rdr.RecordId,rdr.IdString.Data);

	    buffersize = sizeof(inbuff);
	    if (fdebug) printf("BufferSize=%d InvenDataRecSize=%d\n",
		    buffersize, sizeof(inbuff));
	    /* Always get inventory data, not just for BMC */
	    /* if ( IsTagBmc(rdr.IdString.Data, rdr.IdString.DataLength) ) */
	    {
	      memset(inv,0,buffersize);
	      if (fdebug) printf("InventoryDataRead (%d, %d, %d, %d, %p, %d)\n",
	       			sessionid, resourceid, eirid, buffersize, inv, 
				actualsize);

	      rv = saHpiEntityInventoryDataRead( sessionid, resourceid,
		  eirid, buffersize, inv, &actualsize);
  	      if (fdebug) {
		 printf("saHpiEntityInventoryDataRead[%d] rv = %d\n",eirid,rv);
		 printf("buffersize = %d, ActualSize=%d\n", 
				buffersize,actualsize);
	      }
	      if (rv == SA_OK || rv == -2000) // (0 - buffersize))
	      {
	 	/* Walk thru the list of inventory data */
		for (i=0; inv->DataRecords[i] != NULL; i++)
		{
		  if (fdebug) printf( "Record index=%d type=%x len=%d\n",
				i, inv->DataRecords[i]->RecordType,
				inv->DataRecords[i]->DataLength);
		  if (inv->Validity == SAHPI_INVENT_DATA_VALID) 
		  {
		    switch (inv->DataRecords[i]->RecordType)
		    {
		      case SAHPI_INVENT_RECTYPE_INTERNAL_USE:
			  if (fdebug) printf( "Internal Use\n");
			  break;
		      case SAHPI_INVENT_RECTYPE_PRODUCT_INFO:
			  if (fdebug) printf( "Product Info\n");
			  prodrecindx = i;
			  prtprodtinfo();
			  break;
		      case SAHPI_INVENT_RECTYPE_CHASSIS_INFO:
			  if (fdebug) printf( "Chassis Info\n");
			  prtchassinfo();
			  break;
		      case SAHPI_INVENT_RECTYPE_BOARD_INFO:
			  if (fdebug) printf( "Board Info\n");
			  prtboardinfo();
			  break;
		      case SAHPI_INVENT_RECTYPE_OEM:
			  if (fdebug) printf( "OEM Record\n");
			  break;
		      default:
			  printf(" Invalid Invent Rec Type =%x\n",  
				  inv->DataRecords[i]->RecordType);
			  break;
		      }
		    }
		  }
		  /* Cannot guarantee writable unless it is the BMC. */
		  if (IsTagBmc(rdr.IdString.Data,rdr.IdString.DataLength)
		      && (fasset == 1))
		  {  /* handle asset tag */
		    SaHpiTextBufferT aTag;
		    if (fdebug) printf( "Inventory = %s\n", rdr.IdString.Data);
		    /* prodrecindx contains index for Prod Rec Type */
		    dataptr = (SaHpiInventGeneralDataT *)
			&inv->DataRecords[prodrecindx]->RecordData.ProductInfo;
		    dataptr->AssetTag = &aTag;
		    strptr=dataptr->AssetTag;
		    strptr->DataType = SAHPI_TL_TYPE_LANGUAGE;
		    strptr->Language = SAHPI_LANG_ENGLISH;
		    strptr->DataLength = (SaHpiUint8T)asset_len;
		    strncpy( (char *)strptr->Data, (char *)asset_tag,asset_len);
		    strptr->Data[asset_len] = 0;

		    printf( "Writing new asset tag: %s\n",(char *)strptr->Data);
                    rv = saHpiEntityInventoryDataWrite( sessionid,
			  resourceid, eirid, inv);
		    printf("Return Write Status = %d\n", rv);
  
		    if (rv == SA_OK)
		    {
		      printf ("Good write - re-reading!\n");
	              rv = saHpiEntityInventoryDataRead( sessionid, resourceid,
		          eirid, buffersize, inv, &actualsize);
  	              if (fxdebug) printf(
		      "saHpiEntityInventoryDataRead[%d] rv = %d\n", eirid, rv);
	              if (fdebug) printf("ActualSize=%d\n", actualsize);
	              if (rv == SA_OK)
	              {
	 	        /* Walk thru the list of inventory data */
		        for (i=0; inv->DataRecords[i] != NULL; i++)
		        {
		          if (inv->Validity == SAHPI_INVENT_DATA_VALID) 
		          {
		            if (fdebug) printf( "Index = %d type=%x len=%d\n",
				i, inv->DataRecords[i]->RecordType, 
			        inv->DataRecords[i]->DataLength);
		            switch (inv->DataRecords[i]->RecordType)
		            {
		              case SAHPI_INVENT_RECTYPE_INTERNAL_USE:
			        if (fdebug) printf( "Internal Use\n");
			        break;
		              case SAHPI_INVENT_RECTYPE_PRODUCT_INFO:
			        if (fdebug) printf( "Product Info\n");
			        prtprodtinfo();
			        break;
		              case SAHPI_INVENT_RECTYPE_CHASSIS_INFO:
			        if (fdebug) printf( "Chassis Info\n");
			        prtchassinfo();
			        break;
		              case SAHPI_INVENT_RECTYPE_BOARD_INFO:
			        if (fdebug) printf( "Board Info\n");
			        prtboardinfo();
			        break;
		              case SAHPI_INVENT_RECTYPE_OEM:
			        if (fdebug) printf( "OEM Record\n");
			        break;
		              default:
			        printf(" Invalid Invent Rec Type =%x\n",  
				  inv->DataRecords[i]->RecordType);
			        break;
		            }
			  }
		        }    
		     } /*re-walk the inventory record list */
		  } /* Good write - re-read */
	        } /* check asset tag flag */
	      } else {
		/* It is an Inventory RDR, but gets error reading FRU. */
		if (fdebug) printf("\tinventory read error, rv=%d\n", rv);
		rv = 0;  /* keep trying another RDR */
		entryid = nextentryid;
		continue;
	      }
	    }
	  } /* Inventory Data Records - Type 3 */
	  entryid = nextentryid;
        }
      }
      rptentryid = nextrptentryid;
    }
  }
  rv = saHpiSessionClose(sessionid);
  rv = saHpiFinalize();
  exit(0);
}
 /* end hpifru.c */
