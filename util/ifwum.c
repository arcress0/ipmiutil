/*
 * ifwum.c
 * Handle firmware update manager IPMI command functions
 *
 * Change history:
 *  08/20/2010 ARCress - ported from ipmitool/lib/ipmi_fwum.c
 *
 *---------------------------------------------------------------------
 */
/*
 * Copyright (c) 2004 Kontron Canada, Inc.  All Rights Reserved.
 *
 * Base on code from
 * Copyright (c) 2003 Sun Microsystems, Inc.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistribution of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistribution in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of Sun Microsystems, Inc. or the names of
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * This software is provided "AS IS," without a warranty of any kind.
 * ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED.
 * SUN MICROSYSTEMS, INC. ("SUN") AND ITS LICENSORS SHALL NOT BE LIABLE
 * FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING
 * OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.  IN NO EVENT WILL
 * SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA,
 * OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
 * PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY OF
 * LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

#ifdef WIN32
#include <windows.h>
#include "getopt.h"
#elif defined(HPUX)
/* getopt defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#else
#include <getopt.h>
#endif
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef LINUX
#include <unistd.h>
#endif
#include "ipmicmd.h"
#include "ifwum.h"

/******************************************************************************
* HISTORY
* ===========================================================================
* 2007-01-11 [FI]
*  - Incremented to version 1.3
*  - Added lan packet size reduction mechanism to workaround fact
*    that lan iface will not return C7 on excessive length
*
*****************************************************************************/

extern int verbose;  /*see ipmilanplus.c*/
extern void lprintf(int level, const char * format, ...);  /*ipmilanplus.c*/
// extern int ipmi_sendrecv(struct ipmi_rq * req, uint8_t *rsp, int *rsp_len);
extern char * get_mfg_str(uchar *rgmfg, int *pmfg);  /*ihealth.c*/
#ifndef HAVE_LANPLUS
/* define these routines here if no lanplus/helper.c */
extern uint16_t buf2short(uint8_t * buf);  /*ipmilanplus.c*/
// const char * val2str(uint16_t val, const struct valstr *vs);
#endif

#define VERSION_MAJ        1
#define VERSION_MIN        3

/* global variables */
static char * progname  = "ifwum";
static char * progver   = "1.3";
static char   fdebug    = 0;
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;

typedef enum eKFWUM_Task
{
   KFWUM_TASK_INFO,
   KFWUM_TASK_STATUS,
   KFWUM_TASK_DOWNLOAD,
   KFWUM_TASK_UPGRADE,
   KFWUM_TASK_START_UPGRADE,
   KFWUM_TASK_ROLLBACK,
   KFWUM_TASK_TRACELOG
}tKFWUM_Task;

typedef enum eKFWUM_BoardList
{
   KFWUM_BOARD_KONTRON_UNKNOWN   = 0,
   KFWUM_BOARD_KONTRON_5002      = 5002,
}tKFWUM_BoardList;

typedef enum eKFWUM_IanaList
{
   KFWUM_IANA_KONTRON            = 15000,
}tKFWUM_IanaList;

typedef struct sKFWUM_BoardInfo
{
   tKFWUM_BoardList boardId;
   tKFWUM_IanaList  iana;
}tKFWUM_BoardInfo;


#define KFWUM_STATUS_OK      0
#define KFWUM_STATUS_ERROR   -1
typedef int tKFWUM_Status;
//typedef enum eKFWUM_Status
//{
//   KFWUM_STATUS_OK,
//   KFWUM_STATUS_ERROR
//}tKFWUM_Status;

typedef enum eKFWUM_DownloadType
{
   KFWUM_DOWNLOAD_TYPE_ADDRESS = 0,
	KFWUM_DOWNLOAD_TYPE_SEQUENCE,
}tKFWUM_DownloadType;

typedef enum eKFWUM_DownloadBuffferType
{
   KFWUM_SMALL_BUFFER_TYPE = 0,
	KFUMW_BIG_BUFFER_TYPE
}tKFWUM_DownloadBuffferType;

typedef struct sKFWUM_InFirmwareInfo
{
   unsigned long   fileSize;
   unsigned short  checksum;
   unsigned short  sumToRemoveFromChecksum;
                                  /* Since the checksum is added in the bin
                                  after the checksum is calculated, we
                                  need to remove the each byte value.  This
                                  byte will contain the addition of both bytes*/
   tKFWUM_BoardList boardId;
   unsigned char   deviceId;
   unsigned char   tableVers;
   unsigned char   implRev;
   unsigned char   versMajor;
   unsigned char   versMinor;
   unsigned char   versSubMinor;
   unsigned char   sdrRev;
   tKFWUM_IanaList iana;
}tKFWUM_InFirmwareInfo;

typedef struct sKFWUM_SaveFirmwareInfo
{
	tKFWUM_DownloadType downloadType;
   unsigned char       bufferSize;
   unsigned char       overheadSize;	
}tKFWUM_SaveFirmwareInfo;	
  
#define KFWUM_SMALL_BUFFER     32 /* Minimum size (IPMB/IOL/old protocol) */
#define KFWUM_BIG_BUFFER       32 /* Maximum size on KCS interface        */

#define KFWUM_OLD_CMD_OVERHEAD 6  /*3 address + 1 size + 1 checksum + 1 command*/
#define KFWUM_NEW_CMD_OVERHEAD 4  /*1 sequence+ 1 size + 1 checksum + 1 command*/
#define KFWUM_PAGE_SIZE        256

static unsigned char fileName[512];
static unsigned char firmBuf[1024*512];
static tKFWUM_SaveFirmwareInfo saveFirmwareInfo;

static void KfwumOutputHelp(void);
static int KfwumMain(void * intf, tKFWUM_Task task);
static tKFWUM_Status KfwumGetFileSize(unsigned char * pFileName,
                                                     unsigned long * pFileSize);
static tKFWUM_Status KfwumSetupBuffersFromFile(unsigned char * pFileName,
                                                        unsigned long fileSize);
static void KfwumShowProgress( const unsigned char * task,
                                    unsigned long current, unsigned long total);
static unsigned short KfwumCalculateChecksumPadding(unsigned char * pBuffer,
                                                       unsigned long totalSize);


static tKFWUM_Status KfwumGetInfo(void * intf, unsigned char output,
                                                       unsigned char *pNumBank);
static tKFWUM_Status KfwumGetDeviceInfo(void * intf,
                           unsigned char output, tKFWUM_BoardInfo * pBoardInfo);
static tKFWUM_Status KfwumGetStatus(void * intf);
static tKFWUM_Status KfwumManualRollback(void * intf);
static tKFWUM_Status KfwumStartFirmwareImage(void * intf,
                                   unsigned long length,unsigned short padding);
static tKFWUM_Status KfwumSaveFirmwareImage(void * intf,
     unsigned char sequenceNumber, unsigned long address, 
     unsigned char *pFirmBuf, unsigned char * pInBufLength);
static tKFWUM_Status KfwumFinishFirmwareImage(void * intf,
                                                tKFWUM_InFirmwareInfo firmInfo);
static tKFWUM_Status KfwumUploadFirmware(void * intf,
                              unsigned char * pBuffer, unsigned long totalSize);
static tKFWUM_Status KfwumStartFirmwareUpgrade(void * intf);

static tKFWUM_Status KfwumGetInfoFromFirmware(unsigned char * pBuf,
                         unsigned long bufSize, tKFWUM_InFirmwareInfo * pInfo);
static void KfwumFixTableVersionForOldFirmware(tKFWUM_InFirmwareInfo * pInfo);

static tKFWUM_Status KfwumGetTraceLog(void * intf);

tKFWUM_Status KfwumValidFirmwareForBoard(tKFWUM_BoardInfo boardInfo,
                                                tKFWUM_InFirmwareInfo firmInfo);
static void KfwumOutputInfo(tKFWUM_BoardInfo boardInfo,
                                                tKFWUM_InFirmwareInfo firmInfo);


/* ipmi_fwum_main  -  entry point for this ipmitool mode
 *
 * @intf:	  ipmi interface
 * @arc     : number of arguments
 * @argv    : point to argument array
 *
 * returns 0 on success
 * returns -1 on error
 */
int ipmi_fwum_main(void * intf, int  argc, char ** argv)
{
   int rv = ERR_USAGE; /*1*/
   printf("FWUM extension Version %d.%d\n", VERSION_MAJ, VERSION_MIN);
   if ((!argc) || ( !strncmp(argv[0], "help", 4)))
   {
         KfwumOutputHelp();
   }
   else
   {
      if (!strncmp(argv[0], "info", 4))
      {
         rv = KfwumMain(intf, KFWUM_TASK_INFO);
      }
      else if (!strncmp(argv[0], "status", 6))
      {
         rv = KfwumMain(intf, KFWUM_TASK_STATUS);
      }
      else if (!strncmp(argv[0], "rollback", 8))
      {
         rv = KfwumMain(intf, KFWUM_TASK_ROLLBACK);
      }
      else if (!strncmp(argv[0], "download", 8))
      {
         if((argc >= 2) && (strlen(argv[1]) > 0))
         {
            /* There is a file name in the parameters */
            if(strlen(argv[1]) < 512)
            {
               strcpy((char *)fileName, argv[1]);
               printf("Firmware File Name         : %s\n", fileName);

               rv = KfwumMain(intf, KFWUM_TASK_DOWNLOAD);
            }
            else
            {
               fprintf(stderr,"File name must be smaller than 512 bytes\n");
            }
         }
         else
         {
            fprintf(stderr,"A path and a file name must be specified\n");
         }
      }
      else if (!strncmp(argv[0], "upgrade", 7))
      {
         if((argc >= 2) && (strlen(argv[1]) > 0))
         {
            /* There is a file name in the parameters */
            if(strlen(argv[1]) < 512)
            {
               strcpy((char *)fileName, argv[1]);
               printf("Upgrading using file name %s\n", fileName);
               rv = KfwumMain(intf, KFWUM_TASK_UPGRADE);
            }
            else
            {
               fprintf(stderr,"File name must be smaller than 512 bytes\n");
            }
         }
         else
         {
            rv = KfwumMain(intf, KFWUM_TASK_START_UPGRADE);
         }

      }
      else if (!strncmp(argv[0], "tracelog", 8))
      {
         rv = KfwumMain(intf, KFWUM_TASK_TRACELOG);
      }
      else
      {
         printf("Invalid KFWUM command: %s\n", argv[0]);
      }
   }
   return rv;
}


static void KfwumOutputHelp(void)
{
   printf("KFWUM Commands:  info status download upgrade rollback tracelog\n");
}


/****************************************/
/**  private definitions and macros    **/
/****************************************/
typedef enum eFWUM_CmdId
{
   KFWUM_CMD_ID_GET_FIRMWARE_INFO                         = 0,
   KFWUM_CMD_ID_KICK_IPMC_WATCHDOG                        = 1,
   KFWUM_CMD_ID_GET_LAST_ANSWER                           = 2,
   KFWUM_CMD_ID_BOOT_HANDSHAKE                            = 3,
   KFWUM_CMD_ID_REPORT_STATUS                             = 4,
   KFWUM_CMD_ID_GET_FIRMWARE_STATUS                       = 7,
   KFWUM_CMD_ID_START_FIRMWARE_UPDATE                     = 9,
   KFWUM_CMD_ID_START_FIRMWARE_IMAGE                      = 0x0a,
   KFWUM_CMD_ID_SAVE_FIRMWARE_IMAGE                       = 0x0b,
   KFWUM_CMD_ID_FINISH_FIRMWARE_IMAGE                     = 0x0c,
   KFWUM_CMD_ID_READ_FIRMWARE_IMAGE                       = 0x0d,
   KFWUM_CMD_ID_MANUAL_ROLLBACK                           = 0x0e,
   KFWUM_CMD_ID_GET_TRACE_LOG                             = 0x0f,
   KFWUM_CMD_ID_STD_MAX_CMD,
   KFWUM_CMD_ID_EXTENDED_CMD                              = 0xC0
}  tKFWUM_CmdId;



/****************************************/
/** global/static variables definition **/
/****************************************/

/****************************************/
/**        functions definition        **/
/****************************************/

/*******************************************************************************
*
* Function Name:  KfwumMain
*
* Description:    This function implements the upload of the firware data
*                 received as parameters.
*
* Restriction:    Called only from main
*
* Input:  unsigned char * pBuffer[] : The buffers
*         unsigned long bufSize    : The size of the buffers
*
* Output: None
*
* Global: none
*
* Return: tIFWU_Status (success or failure)
*
*******************************************************************************/
static int KfwumMain(void * intf, tKFWUM_Task task)
{
   tKFWUM_Status status = KFWUM_STATUS_OK;
   tKFWUM_BoardInfo boardInfo;
   tKFWUM_InFirmwareInfo firmInfo = { 0 };
   unsigned long fileSize = 0;
   static unsigned short padding;

   if((status == KFWUM_STATUS_OK) && (task == KFWUM_TASK_INFO))
   {
      unsigned char notUsed;
      if(verbose)
      {
         printf("Getting Kontron FWUM Info\n");
      }
      status = KfwumGetDeviceInfo(intf, 1, &boardInfo);
      status = KfwumGetInfo(intf, 1, &notUsed);

   }


   if((status == KFWUM_STATUS_OK) && (task == KFWUM_TASK_STATUS))
   {
      if(verbose)
      {
         printf("Getting Kontron FWUM Status\n");
      }
      status = KfwumGetStatus(intf);
   }

   if( (status == KFWUM_STATUS_OK) && (task == KFWUM_TASK_ROLLBACK) )
   {
      status = KfwumManualRollback(intf);      
   }

   if(
      (status == KFWUM_STATUS_OK) &&
      (
         (task == KFWUM_TASK_UPGRADE) || (task == KFWUM_TASK_DOWNLOAD)
      )
     )
   {
      status = KfwumGetFileSize(fileName, &fileSize);
      if(status == KFWUM_STATUS_OK)
      {
         status = KfwumSetupBuffersFromFile(fileName, fileSize);
         if(status == KFWUM_STATUS_OK)
         {
            padding = KfwumCalculateChecksumPadding(firmBuf, fileSize);
         }
      }
      if(status == KFWUM_STATUS_OK)
      {
         status = KfwumGetInfoFromFirmware(firmBuf, fileSize, &firmInfo);
      }
      if(status == KFWUM_STATUS_OK)
      {
         status = KfwumGetDeviceInfo(intf, 0, &boardInfo);
      }

      if(status == KFWUM_STATUS_OK)
      {
         status = KfwumValidFirmwareForBoard(boardInfo,firmInfo);
      }
      
      if (status == KFWUM_STATUS_OK)
      {
      	unsigned char notUsed;
      	KfwumGetInfo(intf, 0, &notUsed);
      }

      KfwumOutputInfo(boardInfo,firmInfo);
   }

   if(
      (status == KFWUM_STATUS_OK) &&
      (
         (task == KFWUM_TASK_UPGRADE) || (task == KFWUM_TASK_DOWNLOAD)
      )
     )
   {
      status = KfwumStartFirmwareImage(intf, fileSize, padding);
   }


   if(
      (status == KFWUM_STATUS_OK) &&
      (
         (task == KFWUM_TASK_UPGRADE) || (task == KFWUM_TASK_DOWNLOAD)
      )
     )
   {
      status = KfwumUploadFirmware(intf, firmBuf, fileSize);
   }

   if(
      (status == KFWUM_STATUS_OK) &&
      (
         (task == KFWUM_TASK_UPGRADE) || (task == KFWUM_TASK_DOWNLOAD)
      )
     )
   {
      status = KfwumFinishFirmwareImage(intf, firmInfo);
   }

   if(
      (status == KFWUM_STATUS_OK) &&
      (
         (task == KFWUM_TASK_UPGRADE) || (task == KFWUM_TASK_DOWNLOAD)
      )
     )
   {
      status = KfwumGetStatus(intf);
   }

   if(
      (status == KFWUM_STATUS_OK) &&
      (
         (task == KFWUM_TASK_UPGRADE) || (task == KFWUM_TASK_START_UPGRADE)
      )
     )
   {
      status = KfwumStartFirmwareUpgrade(intf);
   }
   
   if((status == KFWUM_STATUS_OK) && (task == KFWUM_TASK_TRACELOG))
   {
      status = KfwumGetTraceLog(intf);
   }

   return(status);
}

/* KfwumGetFileSize  -  gets the file size
 *
 * @pFileName : filename ptr
 * @pFileSize : output ptr for filesize
 *
 * returns KFWUM_STATUS_OK or KFWUM_STATUS_ERROR
 */
static tKFWUM_Status KfwumGetFileSize(unsigned char * pFileName,
                                                     unsigned long * pFileSize)
{
   tKFWUM_Status status = KFWUM_STATUS_ERROR;
   FILE * pFileHandle;

   pFileHandle = fopen((const char *)pFileName, "rb");

   if(pFileHandle)
   {
      if(fseek(pFileHandle, 0L , SEEK_END) == 0)
      {
         *pFileSize     =    ftell(pFileHandle);

         if( *pFileSize != 0)
         {
            status = KFWUM_STATUS_OK;
         }
      }
      fclose(pFileHandle);
   }

   return(status);
}

/* KfwumSetupBuffersFromFile  -  small buffers are used to store the file data
 *
 * @pFileName : filename ptr
 * unsigned long : filesize
 *
 * returns KFWUM_STATUS_OK or KFWUM_STATUS_ERROR
 */
#define MAX_FW_BUFFER_SIZE          1024*16
static tKFWUM_Status KfwumSetupBuffersFromFile(unsigned char * pFileName,
                                                        unsigned long fileSize)
{
   tKFWUM_Status status = KFWUM_STATUS_OK;
   FILE * pFileHandle;

   pFileHandle = fopen((const char *)pFileName, "rb");

   if(pFileHandle)
   {
      int count   = fileSize / MAX_FW_BUFFER_SIZE;
      int modulus = fileSize % MAX_FW_BUFFER_SIZE;
      int qty     =0;

      rewind(pFileHandle);

      for(qty=0;qty<count;qty++)
      {
         KfwumShowProgress((const unsigned char *)"Reading Firmware from File", qty, count );
         if(fread(&firmBuf[qty*MAX_FW_BUFFER_SIZE], 1, MAX_FW_BUFFER_SIZE ,pFileHandle)
            ==  MAX_FW_BUFFER_SIZE)
         {
            status = KFWUM_STATUS_OK;
         }
      }
      if( modulus )
      {
         if(fread(&firmBuf[qty*MAX_FW_BUFFER_SIZE], 1, modulus, pFileHandle) == modulus)
         {
            status = KFWUM_STATUS_OK;
         }
      }
      if(status == KFWUM_STATUS_OK)
      {
         KfwumShowProgress((const unsigned char *)"Reading Firmware from File", 100, 100);
      }
      fclose(pFileHandle);
   }
   return(status);
}

/* KfwumShowProgress  -  helper routine to display progress bar
 *
 * Converts current/total in percent
 * 
 * *task  : string identifying current operation
 * current: progress
 * total  : limit 
 */
#define PROG_LENGTH 42
void KfwumShowProgress( const unsigned char * task,  unsigned long current ,
                                                           unsigned long total)
{
   static unsigned long staticProgress=0xffffffff;
   unsigned char spaces[PROG_LENGTH + 1];
   unsigned short hash;
   float  percent = ((float)current/total);
   unsigned long progress;

   progress = 100*((unsigned long)percent);
   if(staticProgress == progress)
   {
      /* We displayed the same last time.. so don't do it */
   }
   else
   {
      staticProgress = progress;


      printf("%-25s : ",task);    /* total 20 bytes */

      hash = ( (unsigned short)percent * PROG_LENGTH );
      memset(spaces,'#', hash);
      spaces[ hash ] = '\0';
      printf("%s", spaces );

      memset(spaces,' ',( PROG_LENGTH - hash ) );
      spaces[ ( PROG_LENGTH - hash ) ] = '\0';
      printf("%s", spaces );


      printf(" %3ld %%\r",progress); /* total 7 bytes */

      if( progress == 100 )
      {
         printf("\n");
      }
      fflush(stdout);
   }
}

/* KfwumCalculateChecksumPadding
 *
 * TBD
 * 
 */
static unsigned short KfwumCalculateChecksumPadding(unsigned char * pBuffer,
                                                       unsigned long totalSize)
{
   unsigned short sumOfBytes = 0;
   unsigned short padding;
   unsigned long  counter;

   for(counter = 0; counter < totalSize; counter ++ )
   {
      sumOfBytes += pBuffer[counter];
   }

   padding = 0 - sumOfBytes;
   return padding;
}

/******************************************************************************
******************************* COMMANDS **************************************
******************************************************************************/
#pragma pack(1)
struct KfwumGetInfoResp {
   unsigned char protocolRevision;
   unsigned char controllerDeviceId;
   struct 
   {
      unsigned char mode:1;
      unsigned char seqAdd:1;
      unsigned char res : 6;
   } byte;
   unsigned char firmRev1;
   unsigned char firmRev2;
   unsigned char numBank;
}; // __attribute__ ((packed));
#pragma pack()


/* KfwumGetInfo  -  Get Firmware Update Manager (FWUM) information
 * 
 * * intf  : IPMI interface 
 * output  : when set to non zero, queried information is displayed
 * pNumBank: output ptr for number of banks
 */
static tKFWUM_Status KfwumGetInfo(void * intf, unsigned char output,
                                                       unsigned char *pNumBank)
{
   tKFWUM_Status status = KFWUM_STATUS_OK;
   static struct KfwumGetInfoResp *pGetInfo;
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   int dtype;
   uchar bus, sa, lun, mtype; 

   memset(&req, 0, sizeof(req));
   req.msg.netfn = IPMI_NETFN_FIRMWARE;
   req.msg.cmd = KFWUM_CMD_ID_GET_FIRMWARE_INFO;
   req.msg.data_len = 0;

   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv) {
      printf("FWUM Firmware Get Info returned %d (0x%02x)\n",rv,rv);
      status = rv; // KFWUM_STATUS_ERROR
   }

   if(status == KFWUM_STATUS_OK)
   {
      pGetInfo = (struct KfwumGetInfoResp *) rsp;
      if(output)
      {
         printf("\nFWUM info\n");
         printf("=========\n");
         printf("Protocol Revision         : %02Xh\n",
                                                    pGetInfo->protocolRevision);
         printf("Controller Device Id      : %02Xh\n",
                                                  pGetInfo->controllerDeviceId);
         printf("Firmware Revision         : %u.%u%u",
                                 pGetInfo->firmRev1, pGetInfo->firmRev2 >> 4,
                                                     pGetInfo->firmRev2 & 0x0f);
         if(pGetInfo->byte.mode != 0)
         {
            printf(" - DEBUG BUILD\n");
         }
         else
         {
            printf("\n");
         }
         printf("Number Of Memory Bank     : %u\n",pGetInfo->numBank);
      }
      * pNumBank = pGetInfo->numBank;
      
      /* Determine wich type of download to use: */
      /* Old FWUM or Old IPMC fw (data_len < 7) -->
          Address with small buffer size */
      if ( (pGetInfo->protocolRevision) <= 0x05 || (rsp_len < 7 ) )
      {
      	saveFirmwareInfo.downloadType = KFWUM_DOWNLOAD_TYPE_ADDRESS;
         saveFirmwareInfo.bufferSize   = KFWUM_SMALL_BUFFER;
         saveFirmwareInfo.overheadSize = KFWUM_OLD_CMD_OVERHEAD;

         if(verbose)
         {
            printf("Protocol Revision          :");
            printf(" <= 5 detected, adjusting buffers\n");
         }
      }
      else /* Both fw are using the new protocol */
      {
      	saveFirmwareInfo.downloadType = KFWUM_DOWNLOAD_TYPE_SEQUENCE;
         saveFirmwareInfo.overheadSize = KFWUM_NEW_CMD_OVERHEAD;
         /* Buffer size depending on access type (Local or remote) */
         /* Look if we run remote or locally */

         if(verbose)
         {
            printf("Protocol Revision          :");
            printf(" > 5 optimizing buffers\n");
         }

	 ipmi_get_mc(&bus, &sa, &lun, &mtype);
	 dtype = get_driver_type();
         if(is_remote()) /* covers lan and lanplus */
         {
            saveFirmwareInfo.bufferSize = KFWUM_SMALL_BUFFER;
            if(verbose)
            {
               printf("IOL payload size           : %d\r\n"  ,
                                                  saveFirmwareInfo.bufferSize);
            }
         } else if ( (dtype == DRV_MV) && (sa != IPMI_BMC_SLAVE_ADDR) &&
			(mtype == ADDR_IPMB) )
         {
            saveFirmwareInfo.bufferSize = KFWUM_SMALL_BUFFER;
            if(verbose)
            {
               printf("IPMB payload size          : %d\r\n"  , 
                                                   saveFirmwareInfo.bufferSize);
            }
         }
         else
         {
         	saveFirmwareInfo.bufferSize = KFWUM_BIG_BUFFER;
            if(verbose)
            {
               printf("SMI payload size           : %d\r\n", 
                                                  saveFirmwareInfo.bufferSize);
            }
         }
      }
   }
   return status;
}

/* KfwumGetDeviceInfo  -  Get IPMC/Board information
 * 
 * * intf  : IPMI interface 
 * output  : when set to non zero, queried information is displayed
 * tKFWUM_BoardInfo: output ptr for IPMC/Board information
 */
static tKFWUM_Status KfwumGetDeviceInfo(void * intf,
                            unsigned char output, tKFWUM_BoardInfo * pBoardInfo)
{
   struct ipm_devid_rsp   *pGetDevId;
   tKFWUM_Status status = KFWUM_STATUS_OK;
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   char *mstr;

   /* Send Get Device Id */
   if(status == KFWUM_STATUS_OK)
   {
      memset(&req, 0, sizeof(req));
      req.msg.netfn = IPMI_NETFN_APP;
      req.msg.cmd = BMC_GET_DEVICE_ID;
      req.msg.data_len = 0;

      rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
      if (rv) {
         printf("Error in Get Device Id Command %d (0x%02x)\n",rv,rv);
         status = rv; // KFWUM_STATUS_ERROR
      }
   }

   if(status == KFWUM_STATUS_OK)
   {
      pGetDevId = (struct ipm_devid_rsp *) rsp;
      pBoardInfo->iana = IPM_DEV_MANUFACTURER_ID(pGetDevId->manufacturer_id);
      pBoardInfo->boardId = buf2short(pGetDevId->product_id);
      if(output)
      {
         mstr = get_mfg_str(pGetDevId->manufacturer_id,NULL);
         printf("\nIPMC Info\n");
         printf("=========\n");
         printf("Manufacturer Id           : %u \t%s\n",pBoardInfo->iana,mstr);
         printf("Board Id                  : %u\n",pBoardInfo->boardId);
         printf("Firmware Revision         : %u.%u%u",
                                 pGetDevId->fw_rev1, pGetDevId->fw_rev2 >> 4,
                                                   pGetDevId->fw_rev2 & 0x0f);
         if(
            ( 
               ( pBoardInfo->iana == KFWUM_IANA_KONTRON)
               && 
               (pBoardInfo->boardId == KFWUM_BOARD_KONTRON_5002)
            )
           )
         {
            printf(" SDR %u\n", pGetDevId->aux_fw_rev[0]);
         }
         else
         {
            printf("\n");
         }
      }
   }

   return status;
}

#pragma pack(1)
struct KfwumGetStatusResp {
   unsigned char bankState;
   unsigned char firmLengthLSB;
   unsigned char firmLengthMid;
   unsigned char firmLengthMSB;
   unsigned char firmRev1;
   unsigned char firmRev2;
   unsigned char firmRev3;
}; // __attribute__ ((packed));
#pragma pack()

const struct valstr bankStateValS[] = {
   { 0x00, "Not programmed" },
   { 0x01, "New firmware" },
   { 0x02, "Wait for validation" },
   { 0x03, "Last Known Good" },
   { 0x04, "Previous Good" }
};

/* KfwumGetStatus  -  Get (and prints) FWUM  banks information
 * 
 * * intf  : IPMI interface 
 */
static tKFWUM_Status KfwumGetStatus(void * intf)
{
   tKFWUM_Status status = KFWUM_STATUS_OK;
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   struct KfwumGetStatusResp *pGetStatus;
   unsigned char numBank;
   unsigned char counter;

   if(verbose)
   {
      printf(" Getting Status!\n");
   }

   /* Retreive the number of bank */
   status = KfwumGetInfo(intf, 0, &numBank);

   for(
         counter = 0;
         (counter < numBank) && (status == KFWUM_STATUS_OK);
         counter ++
      )
   {
      /* Retreive the status of each bank */
      memset(&req, 0, sizeof(req));
      req.msg.netfn = IPMI_NETFN_FIRMWARE;
      req.msg.cmd = KFWUM_CMD_ID_GET_FIRMWARE_STATUS;
      req.msg.data = &counter;
      req.msg.data_len = 1;

      rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
      if (rv) {
         printf("FWUM Firmware Get Status Error %d (0x%02x)\n",rv,rv);
         status = rv; // KFWUM_STATUS_ERROR
      }

      if(status == KFWUM_STATUS_OK)
      {
         pGetStatus = (struct KfwumGetStatusResp *) rsp;
         printf("\nBank State %d               : %s\n", counter, val2str(
                                         pGetStatus->bankState, bankStateValS));
         if(pGetStatus->bankState)
         {
            unsigned long firmLength;
            firmLength  = pGetStatus->firmLengthMSB;
            firmLength  = firmLength << 8;
            firmLength |= pGetStatus->firmLengthMid;
            firmLength  = firmLength << 8;
            firmLength |= pGetStatus->firmLengthLSB;

            printf("Firmware Length            : %ld bytes\n", firmLength);
            printf("Firmware Revision          : %u.%u%u SDR %u\n",
                              pGetStatus->firmRev1,  pGetStatus->firmRev2 >> 4,
                           pGetStatus->firmRev2 & 0x0f, pGetStatus->firmRev3);
         }
      }
   }
   printf("\n");
   return status;
}
#pragma pack(1)
struct KfwumManualRollbackReq{
   unsigned char type;
}; // __attribute__ ((packed));
#pragma pack()


/* KfwumManualRollback  -  Ask IPMC to rollback to previous version
 * 
 * * intf  : IPMI interface 
 */
static tKFWUM_Status KfwumManualRollback(void * intf)
{
  tKFWUM_Status status = KFWUM_STATUS_OK;
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   struct KfwumManualRollbackReq thisReq;


   memset(&req, 0, sizeof(req));
   req.msg.netfn = IPMI_NETFN_FIRMWARE;
   req.msg.cmd = KFWUM_CMD_ID_MANUAL_ROLLBACK;

   thisReq.type = 0;  /* Wait BMC shutdown */

   req.msg.data = (unsigned char *) &thisReq;
   req.msg.data_len = 1;

   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv) {
      printf("Error in FWUM Manual Rollback Command %d (0x%02x)\n",rv,rv);
      status = rv; // KFWUM_STATUS_ERROR
   }

   if(status == KFWUM_STATUS_OK)
   {
      printf("FWUM Starting Manual Rollback \n");
   }
   return status;
}

#pragma pack(1)
struct KfwumStartFirmwareDownloadReq{
   unsigned char lengthLSB;
   unsigned char lengthMid;
   unsigned char lengthMSB;
   unsigned char paddingLSB;
   unsigned char paddingMSB;
	unsigned char useSequence;
}; // __attribute__ ((packed));
struct KfwumStartFirmwareDownloadResp {
   unsigned char bank;
}; // __attribute__ ((packed));
#pragma pack()

static tKFWUM_Status KfwumStartFirmwareImage(void * intf,
                                   unsigned long length,unsigned short padding)
{
   tKFWUM_Status status = KFWUM_STATUS_OK;
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   struct KfwumStartFirmwareDownloadResp *pResp;
   struct KfwumStartFirmwareDownloadReq thisReq;

   thisReq.lengthLSB  = (uchar)(length         & 0x000000ff);
   thisReq.lengthMid  = (uchar)((length >>  8) & 0x000000ff);
   thisReq.lengthMSB  = (uchar)((length >> 16) & 0x000000ff);
   thisReq.paddingLSB = padding        & 0x00ff;
   thisReq.paddingMSB = (padding>>  8) & 0x00ff;
	thisReq.useSequence = 0x01;

   memset(&req, 0, sizeof(req));
   req.msg.netfn = IPMI_NETFN_FIRMWARE;
   req.msg.cmd = KFWUM_CMD_ID_START_FIRMWARE_IMAGE;
   req.msg.data = (unsigned char *) &thisReq;
   
   /* Look for download type */
   if ( saveFirmwareInfo.downloadType == KFWUM_DOWNLOAD_TYPE_ADDRESS )
   {
   	req.msg.data_len = 5;
   }
   else
   {
   	req.msg.data_len = 6;
   }
      
   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv) {
      printf("FWUM Firmware Start Firmware Image Download returned %d (0x%02x)\n",rv,rv);
      status = rv; // KFWUM_STATUS_ERROR
   }

   if(status == KFWUM_STATUS_OK)
   {
      pResp = (struct KfwumStartFirmwareDownloadResp *) rsp;
      printf("Bank holding new firmware  : %d\n", pResp->bank);
      os_usleep(5,0);
   }
   return status;
}

#pragma pack(1)
struct KfwumSaveFirmwareAddressReq
{
 	unsigned char addressLSB;
   unsigned char addressMid;
   unsigned char addressMSB;
   unsigned char numBytes;
   unsigned char txBuf[KFWUM_SMALL_BUFFER-KFWUM_OLD_CMD_OVERHEAD];
}; // __attribute__ ((packed));

struct KfwumSaveFirmwareSequenceReq
{
 	unsigned char sequenceNumber;
   unsigned char txBuf[KFWUM_BIG_BUFFER];
}; // __attribute__ ((packed));
#pragma pack()


#define FWUM_SAVE_FIRMWARE_NO_RESPONSE_LIMIT ((unsigned char)6)

static tKFWUM_Status KfwumSaveFirmwareImage(void * intf,
     unsigned char sequenceNumber, unsigned long address, unsigned char *pFirmBuf, 
     unsigned char * pInBufLength)
{
   tKFWUM_Status status = KFWUM_STATUS_OK;
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   unsigned char out = 0;
   unsigned char retry = 0;
   unsigned char noResponse = 0 ;
   
   struct KfwumSaveFirmwareAddressReq  addressReq;
   struct KfwumSaveFirmwareSequenceReq sequenceReq;

   do
   {
   	memset(&req, 0, sizeof(req));
      req.msg.netfn = IPMI_NETFN_FIRMWARE;
      req.msg.cmd = KFWUM_CMD_ID_SAVE_FIRMWARE_IMAGE;
      
   	if (saveFirmwareInfo.downloadType == KFWUM_DOWNLOAD_TYPE_ADDRESS )
   	{
      	 addressReq.addressLSB  = (uchar)(address         & 0x000000ff);
         addressReq.addressMid  = (uchar)((address >>  8) & 0x000000ff);
         addressReq.addressMSB  = (uchar)((address >> 16) & 0x000000ff);
         addressReq.numBytes    = (* pInBufLength);
         memcpy(addressReq.txBuf, pFirmBuf, (* pInBufLength));
         req.msg.data = (unsigned char *) &addressReq;
         req.msg.data_len = (* pInBufLength)+4;
   	}
      else
      {
      	sequenceReq.sequenceNumber = sequenceNumber;
         memcpy(sequenceReq.txBuf, pFirmBuf, (* pInBufLength));
         req.msg.data = (unsigned char *) &sequenceReq;
         req.msg.data_len = (* pInBufLength)+sizeof(unsigned char); /* + 1 => sequenceNumber*/
      }
      						
      rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
      if (rv < 0)
      {
         printf("Error in FWUM Firmware Save Firmware Image Download Command\n");

         out = 0;
         status = KFWUM_STATUS_OK;
         
         /* With IOL, we don't receive "C7" on errors, instead we receive
            nothing */
         if(is_remote())
         {
            noResponse++;

            if(noResponse < FWUM_SAVE_FIRMWARE_NO_RESPONSE_LIMIT )
            {
               (* pInBufLength) -= 1;
               out = 0;
            }
            else
            {
               printf("Error, too many commands without response\n");
               (* pInBufLength) = 0 ;               
               out = 1;
            }
         } /* For other interface keep trying */
      }
      else if (rv > 0)   /*ccode*/
      {
         if(rv == 0xc0)
         {
            status = KFWUM_STATUS_OK;
	    os_usleep(1,0);
         }
         else if(
                  (rv == 0xc7)
                  ||
                  (
                     (rv == 0xC3) &&
                     (sequenceNumber == 0)
                  )
                )
         {
            (* pInBufLength) -= 1;
            status = KFWUM_STATUS_OK;
            retry = 1;
         }
         else if(rv == 0x82)
         {
            /* Double sent, continue */
            status = KFWUM_STATUS_OK;
            out = 1;
         }
         else if(rv == 0x83)
         {
            if(retry == 0)
            {
               retry = 1;
               status = KFWUM_STATUS_OK;
            }
            else
            {
               status = rv; // KFWUM_STATUS_ERROR
               out = 1;
            }
         }
         else if(rv == 0xcf) /* Ok if receive duplicated request */
         {
            retry = 1;
            status = KFWUM_STATUS_OK;
         }
         else if(rv == 0xC3)
         {
            if(retry == 0)
           {
               retry = 1;
               status = KFWUM_STATUS_OK;
            }
            else
            {
               status = rv; // KFWUM_STATUS_ERROR
               out = 1;
            }
         }
         else
         {
            printf("FWUM Firmware Save Firmware Image Download returned %x\n",
                                                                     rv);
            status = rv; // KFWUM_STATUS_ERROR
            out = 1;
         }
      }
      else
      {
         out = 1;
      }
   }while(out == 0);
   return status;
}

#pragma pack(1)
struct KfwumFinishFirmwareDownloadReq{
   unsigned char versionMaj;
   unsigned char versionMinSub;
   unsigned char versionSdr;
   unsigned char reserved;
}; // __attribute__ ((packed));
#pragma pack()
static tKFWUM_Status KfwumFinishFirmwareImage(void * intf,
                                                 tKFWUM_InFirmwareInfo firmInfo)
{
   tKFWUM_Status status = KFWUM_STATUS_OK;
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   struct KfwumFinishFirmwareDownloadReq thisReq;

   thisReq.versionMaj     = firmInfo.versMajor;
   thisReq.versionMinSub  = ((firmInfo.versMinor <<4) | firmInfo.versSubMinor);
   thisReq.versionSdr     = firmInfo.sdrRev;
   thisReq.reserved       = 0;
   /* Byte 4 reserved, write 0 */

   memset(&req, 0, sizeof(req));
   req.msg.netfn = IPMI_NETFN_FIRMWARE;
   req.msg.cmd = KFWUM_CMD_ID_FINISH_FIRMWARE_IMAGE;
   req.msg.data = (unsigned char *) &thisReq;
   req.msg.data_len = 4;
   											       
   do
   {
   	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   }while (rv == 0xc0); 

   if (rv < 0)
   {
      printf("Error in FWUM Firmware Finish Firmware Image Download Command\n");
      status = rv; // KFWUM_STATUS_ERROR
   }
   else if (rv > 0)
   {
      printf("FWUM Firmware Finish Firmware Image Download returned %x\n",
                                                                    rv);
      status = rv; // KFWUM_STATUS_ERROR
   }

   return status;
}


#define FWUM_MAX_UPLOAD_RETRY 6
static tKFWUM_Status KfwumUploadFirmware(void * intf,
                               unsigned char * pBuffer, unsigned long totalSize)
{
   tKFWUM_Status status = KFWUM_STATUS_ERROR;
   unsigned long address    = 0x0;
   unsigned char writeSize;
   unsigned char oldWriteSize;
   unsigned long lastAddress = 0;
   unsigned char sequenceNumber = 0;
   unsigned char retry = FWUM_MAX_UPLOAD_RETRY;
   // unsigned char isLengthValid = 1;

   do
   {
      writeSize = saveFirmwareInfo.bufferSize - saveFirmwareInfo.overheadSize;
           
      /* Reach the end */
      if( address + writeSize > totalSize )
      {
         writeSize = (uchar)(totalSize - address);
      }
      /* Reach boundary end */
      else if(((address % KFWUM_PAGE_SIZE) + writeSize) > KFWUM_PAGE_SIZE)
      {
         writeSize = (KFWUM_PAGE_SIZE - (uchar)(address % KFWUM_PAGE_SIZE));
      }

      oldWriteSize = writeSize;
      status = KfwumSaveFirmwareImage(intf, sequenceNumber, address, 
                                                &pBuffer[address], &writeSize);
 
      if((status != KFWUM_STATUS_OK) && (retry-- != 0))
      {
         address = lastAddress;
         status = KFWUM_STATUS_OK;
      }
      else if( writeSize == 0 )
      {
         status = KFWUM_STATUS_ERROR;
      }
      else
      {
         if(writeSize != oldWriteSize)
         {
            printf("Adjusting length to %d bytes \n", writeSize);
            saveFirmwareInfo.bufferSize -= (oldWriteSize - writeSize);
         }
         
         retry = FWUM_MAX_UPLOAD_RETRY;
         lastAddress = address;
         address+= writeSize;
      }

      if(status == KFWUM_STATUS_OK)
      {
         if((address % 1024) == 0)
         {
            KfwumShowProgress((const unsigned char *)\
                                 "Writing Firmware in Flash",address,totalSize);
         }
         sequenceNumber++;
      }

   }while((status == KFWUM_STATUS_OK) && (address < totalSize  ));

   if(status == KFWUM_STATUS_OK)
   {
      KfwumShowProgress((const unsigned char *)\
                                       "Writing Firmware in Flash", 100 , 100 );
   }

   return(status);
}

static tKFWUM_Status KfwumStartFirmwareUpgrade(void * intf)
{
   tKFWUM_Status status = KFWUM_STATUS_OK;
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   unsigned char upgType = 0 ;  /* Upgrade type, wait BMC shutdown */

   memset(&req, 0, sizeof(req));
   req.msg.netfn = IPMI_NETFN_FIRMWARE;
   req.msg.cmd = KFWUM_CMD_ID_START_FIRMWARE_UPDATE;
   req.msg.data = (unsigned char *) &upgType;
   req.msg.data_len = 1;

   rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
   if (rv < 0)
   {
      printf("Error in FWUM Firmware Start Firmware Upgrade Command\n");
      status = rv; // KFWUM_STATUS_ERROR
   }
   else if (rv > 0)
   {
      if(rv == 0xd5)
      {
         printf("No firmware available for upgrade.  Download Firmware first\n");
      }
      else
      {
         printf("FWUM Firmware Start Firmware Upgrade returned %x\n",
                                                                    rv);
      }
      status = rv; // KFWUM_STATUS_ERROR
   }

   return status;              
}

#define TRACE_LOG_CHUNK_COUNT 7
#define TRACE_LOG_CHUNK_SIZE  7
#define TRACE_LOG_ATT_COUNT   3
/* String table */
/* Must match eFWUM_CmdId */
static const char* CMD_ID_STRING[] = {
                "GetFwInfo",
                "KickWatchdog",
                "GetLastAnswer",
                "BootHandshake",
                "ReportStatus",
                "CtrlIPMBLine",
                "SetFwState",                
                "GetFwStatus",
                "GetSpiMemStatus",
                "StartFwUpdate",
                "StartFwImage",
                "SaveFwImage",
                "FinishFwImage",
                "ReadFwImage",
                "ManualRollback",
                "GetTraceLog" };
                
static const char* EXT_CMD_ID_STRING[] = {
                "FwUpgradeLock",
                "ProcessFwUpg",
                "ProcessFwRb",
                "WaitHSAfterUpg",
                "WaitFirstHSUpg",
                "FwInfoStateChange" };
               
                
static const char* CMD_STATE_STRING[] = {
                "Invalid",
                "Begin",
                "Progress",
                "Completed" };


static tKFWUM_Status KfwumGetTraceLog(void * intf)
{
   tKFWUM_Status status = KFWUM_STATUS_OK;
   uchar rsp[IPMI_RSPBUF_SIZE];
   int rsp_len, rv;
   struct ipmi_rq req;
   unsigned char  chunkIdx;
   unsigned char  cmdIdx;

   if(verbose)
   {
      printf(" Getting Trace Log!\n");
   }
   
   for( chunkIdx = 0; (chunkIdx < TRACE_LOG_CHUNK_COUNT) && (status == KFWUM_STATUS_OK); chunkIdx++ )
   {
      /* Retreive each log chunk and print it */
      memset(&req, 0, sizeof(req));
      req.msg.netfn = IPMI_NETFN_FIRMWARE;
      req.msg.cmd = KFWUM_CMD_ID_GET_TRACE_LOG;
      req.msg.data = &chunkIdx;
      req.msg.data_len = 1;

      rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
      if (rv) {
         printf("FWUM Firmware Get Trace Log returned %d (0x%02x)\n",rv,rv);
         status = rv; // KFWUM_STATUS_ERROR
      } 

      if(status == KFWUM_STATUS_OK)
      {
         for (cmdIdx=0; cmdIdx < TRACE_LOG_CHUNK_SIZE; cmdIdx++)
         {
            /* Don't diplay commands with an invalid state */
            if ( (rsp[TRACE_LOG_ATT_COUNT*cmdIdx+1] != 0) && 
                 (rsp[TRACE_LOG_ATT_COUNT*cmdIdx] < KFWUM_CMD_ID_STD_MAX_CMD))
            {
               printf("  Cmd ID: %17s -- CmdState: %10s -- CompCode: %2x\n", 
                                             CMD_ID_STRING[rsp[TRACE_LOG_ATT_COUNT*cmdIdx]],
                                             CMD_STATE_STRING[rsp[TRACE_LOG_ATT_COUNT*cmdIdx+1]],
                                             rsp[TRACE_LOG_ATT_COUNT*cmdIdx+2]);
            }
            else if ( (rsp[TRACE_LOG_ATT_COUNT*cmdIdx+1] != 0) && 
                      (rsp[TRACE_LOG_ATT_COUNT*cmdIdx] >= KFWUM_CMD_ID_EXTENDED_CMD))
            {
               printf("  Cmd ID: %17s -- CmdState: %10s -- CompCode: %2x\n", 
                                             EXT_CMD_ID_STRING[rsp[TRACE_LOG_ATT_COUNT*cmdIdx] - KFWUM_CMD_ID_EXTENDED_CMD],
                                             CMD_STATE_STRING[rsp[TRACE_LOG_ATT_COUNT*cmdIdx+1]],
                                             rsp[TRACE_LOG_ATT_COUNT*cmdIdx+2]);
            }
         }
      }
   }    
   printf("\n");
   return status;
}


/*******************************************************************************
* Function Name: KfwumGetInfoFromFirmware
*
* Description: This function retreive from the firmare the following info :
*
*              o Checksum
*              o File size (expected)
*              o Board Id
*              o Device Id
*
* Restriction: None
*
* Input: char * fileName - File to get info from
*
* Output: pInfo - container that will hold all the informations gattered.
*                 see structure for all details
*
* Global: None
*
* Return: IFWU_SUCCESS - file ok
*         IFWU_ERROR   - file error
*
*******************************************************************************/
#define IN_FIRMWARE_INFO_OFFSET_LOCATION           0x5a0
#define IN_FIRMWARE_INFO_SIZE                      20
#define IN_FIRMWARE_INFO_OFFSET_FILE_SIZE          0
#define IN_FIRMWARE_INFO_OFFSET_CHECKSUM           4
#define IN_FIRMWARE_INFO_OFFSET_BOARD_ID           6
#define IN_FIRMWARE_INFO_OFFSET_DEVICE_ID          8
#define IN_FIRMWARE_INFO_OFFSET_TABLE_VERSION      9
#define IN_FIRMWARE_INFO_OFFSET_IMPLEMENT_REV      10
#define IN_FIRMWARE_INFO_OFFSET_VERSION_MAJOR      11
#define IN_FIRMWARE_INFO_OFFSET_VERSION_MINSUB     12
#define IN_FIRMWARE_INFO_OFFSET_SDR_REV            13
#define IN_FIRMWARE_INFO_OFFSET_IANA0              14
#define IN_FIRMWARE_INFO_OFFSET_IANA1              15
#define IN_FIRMWARE_INFO_OFFSET_IANA2              16

#define KWUM_GET_BYTE_AT_OFFSET(pBuffer,os)            pBuffer[os]

tKFWUM_Status KfwumGetInfoFromFirmware(unsigned char * pBuf,
                         unsigned long bufSize,  tKFWUM_InFirmwareInfo * pInfo)
{
   tKFWUM_Status status = KFWUM_STATUS_ERROR;

   if(bufSize >= (IN_FIRMWARE_INFO_OFFSET_LOCATION + IN_FIRMWARE_INFO_SIZE))
   {
      unsigned long offset = IN_FIRMWARE_INFO_OFFSET_LOCATION;

      /* Now, fill the structure with read informations */
      pInfo->checksum  =  (unsigned short)KWUM_GET_BYTE_AT_OFFSET(pBuf,
                           offset+0+IN_FIRMWARE_INFO_OFFSET_CHECKSUM ) << 8;
      pInfo->checksum |= (unsigned short)KWUM_GET_BYTE_AT_OFFSET(pBuf,
                           offset+1+IN_FIRMWARE_INFO_OFFSET_CHECKSUM );


      pInfo->sumToRemoveFromChecksum=
         KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_CHECKSUM);

      pInfo->sumToRemoveFromChecksum+=
        KWUM_GET_BYTE_AT_OFFSET(pBuf ,
                             offset+IN_FIRMWARE_INFO_OFFSET_CHECKSUM+1);

      pInfo->fileSize  =
         KWUM_GET_BYTE_AT_OFFSET(pBuf ,
                              offset+IN_FIRMWARE_INFO_OFFSET_FILE_SIZE+0) << 24;
      pInfo->fileSize |=
         (unsigned long)KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_FILE_SIZE+1) << 16;
      pInfo->fileSize |=
         (unsigned long)KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_FILE_SIZE+2) << 8;
      pInfo->fileSize |=
         (unsigned long)KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_FILE_SIZE+3);

      pInfo->boardId   =
         KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_BOARD_ID+0) << 8;
      pInfo->boardId  |=
         KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_BOARD_ID+1);

      pInfo->deviceId  =
         KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_DEVICE_ID);

      pInfo->tableVers     =
         KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_TABLE_VERSION);
      pInfo->implRev       =
         KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_IMPLEMENT_REV);
      pInfo->versMajor     =
         (KWUM_GET_BYTE_AT_OFFSET(pBuf,
                      offset+IN_FIRMWARE_INFO_OFFSET_VERSION_MAJOR)) & 0x0f;
      pInfo->versMinor     =
         (KWUM_GET_BYTE_AT_OFFSET(pBuf,
                 offset+IN_FIRMWARE_INFO_OFFSET_VERSION_MINSUB)>>4) & 0x0f;
      pInfo->versSubMinor  =
         (KWUM_GET_BYTE_AT_OFFSET(pBuf,
                    offset+IN_FIRMWARE_INFO_OFFSET_VERSION_MINSUB)) & 0x0f;
      pInfo->sdrRev        =
         KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_SDR_REV);
      pInfo->iana  =
         KWUM_GET_BYTE_AT_OFFSET(pBuf ,
                              offset+IN_FIRMWARE_INFO_OFFSET_IANA2) << 16;
      pInfo->iana |=
         (unsigned long)KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_IANA1) << 8;
      pInfo->iana |=
         (unsigned long)KWUM_GET_BYTE_AT_OFFSET(pBuf,
                              offset+IN_FIRMWARE_INFO_OFFSET_IANA0);

      KfwumFixTableVersionForOldFirmware(pInfo);

      status = KFWUM_STATUS_OK;
   }
   return(status);
}


void KfwumFixTableVersionForOldFirmware(tKFWUM_InFirmwareInfo * pInfo)
{
   switch(pInfo->boardId)
   {
      case KFWUM_BOARD_KONTRON_UNKNOWN:
         pInfo->tableVers = 0xff;
      break;
      default:
         /* pInfo->tableVers is already set for the right version */
      break;
   }
}


tKFWUM_Status KfwumValidFirmwareForBoard(tKFWUM_BoardInfo boardInfo,
                                                tKFWUM_InFirmwareInfo firmInfo)
{
   tKFWUM_Status status = KFWUM_STATUS_OK;

   if(boardInfo.iana != firmInfo.iana)
   {
      printf("Board IANA does not match firmware IANA\n");
      status = KFWUM_STATUS_ERROR;
   }

   if(boardInfo.boardId != firmInfo.boardId)
   {
      printf("Board IANA does not match firmware IANA\n");
      status = KFWUM_STATUS_ERROR;
   }

   if(status == KFWUM_STATUS_ERROR)
   {
      printf("Firmware invalid for target board.  Download of upgrade aborted\n");
   }
   return status;
}


static void KfwumOutputInfo(tKFWUM_BoardInfo boardInfo,
                                                tKFWUM_InFirmwareInfo firmInfo)
{
   printf("Target Board Id            : %u\n",boardInfo.boardId);
   printf("Target IANA number         : %u\n",boardInfo.iana);
   printf("File Size                  : %lu bytes\n",firmInfo.fileSize);
   printf("Firmware Version           : %d.%d%d SDR %d\n",firmInfo.versMajor,
                   firmInfo.versMinor, firmInfo.versSubMinor, firmInfo.sdrRev);
}

#ifdef METACOMMAND
int i_fwum(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
#endif
{
	void *intf = NULL;
	int rc = 0;
	int c, i;
	char *s1;

	printf("%s ver %s\n", progname,progver);

        while ( (c = getopt( argc, argv,"m:T:V:J:EYF:P:N:R:U:Z:x?")) != EOF )
	switch (c) {
          case 'm': /* specific IPMB MC, 3-byte address, e.g. "409600" */
                    g_bus = htoi(&optarg[0]);  /*bus/channel*/
                    g_sa  = htoi(&optarg[2]);  /*device slave address*/
                    g_lun = htoi(&optarg[4]);  /*LUN*/
                    g_addrtype = ADDR_IPMB;
                    if (optarg[6] == 's') {
                             g_addrtype = ADDR_SMI;  s1 = "SMI";
                    } else { g_addrtype = ADDR_IPMB; s1 = "IPMB"; }
		    ipmi_set_mc(g_bus,g_sa,g_lun,g_addrtype);
                    printf("Use MC at %s bus=%x sa=%x lun=%x\n",
                            s1,g_bus,g_sa,g_lun);
                    break;
          case 'x': fdebug = 1; verbose = 1;
			break;  /* debug messages */
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
          case '?': 
		KfwumOutputHelp();
		return ERR_USAGE;
                break;
	}
        for (i = 0; i < optind; i++) { argv++; argc--; }

	rc = ipmi_fwum_main(intf, argc, argv);

        ipmi_close_();
        // show_outcome(progname,rc);
	return rc;
}
