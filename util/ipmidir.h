/***********************************************
 * ipmidir.h 
 *
 * Definitions and data structures for direct
 * user-space IPMI I/Os.
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
#ifndef IPMIDIR_H_
#define IPMIDIR_H_


#define SMB_DATA_ADDRESS = 0x000

//
// State bits based on S1 & S0 below
//
#define ISA_STATE_MASK        0xC0
#define ISA_IDLE_STATE        0x00
#define ISA_READ_STATE        0x40
#define ISA_WRITE_STATE       0x80
#define ISA_ERROR_STATE       0xC0
//
// Status Register Bits
//
#define ISA_S1_FLAG           0x80
#define ISA_S0_FLAG           0x40
// RESERVED                   0x20
// RESERVED                   0x10
#define ISA_CD_FLAG           0x08
#define ISA_SMS_MSG_FLAG      0x04
#define ISA_IBF_FLAG          0x02
#define ISA_OBF_FLAG          0x01
//
// ISA interface register access defines
//

#define BMC_SLAVE_ADDR         0x20
#define MAX_ISA_LENGTH         35

#define ISA_SMS_TIMEOUT        5000    //    in miliseconds - 1 second
#define ISA_SMM_TIMEOUT        100     //    in microseconds
//
// mBMC Address
//
#define IMB_SEND_ERROR 1
#define IMB_SUCCESS    0

// typedef unsigned int DWORD;
// typedef unsigned char BYTE;
// typedef unsigned char  UCHAR;  * defined in imb_api.h *
// typedef unsigned long  ULONG;  * defined in imb_api.h *
// typedef unsigned int   UINT32;
typedef unsigned short UINT16;
typedef unsigned char  UINT8;

struct SMBCharStruct
{
unsigned int Controller;
unsigned int baseAddr;
};

/*---------------------------------------------------------*
 *  PCI defines for SMBus 
 *---------------------------------------------------------*/

// PCI related definitions
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

// PCI unique identifiers
#define ID_8111             0x746A1022
#define ID_81801            0x24128086
#define ID_81801AA          0x24138086
#define ID_81801AB          0x24238086
#define ID_82801BA          0x24438086
#define ID_82801CA          0x24838086
#define ID_ICH4             0x24C38086
#define ID_ICH5             0x24D38086
#define ID_ICH6             0x266A8086 //srini added ICH6 support
#define ID_CSB5             0x02011166
#define ID_CSB6             0x02031166
#define ID_OSB4             0x02001166
#define Hance_Rapids        0x25A48086   //lester 0627

// PCI configuration registers
#define  ICH_SMB_BASE               0x20 // base address register
#define  ICH_HOSTC                  0x40 // host config register
#define  ICH_HOSTC_I2C_EN           0x04 // enable i2c mode 
#define  ICH_HOSTC_SMB_SMI_EN       0x02 // SMI# instead of irq
#define  ICH_HOSTC_HST_EN           0x01 // enable host cntrlr 
#define  AMD_SMB_BASE_2             0x10 // base address register of SMBus 2.0
#define  AMD_SMB_BASE_2_EN          0x04 // SMBus 2.0 space enable
#define  AMD_SMB_BASE_1             0x58 // base address register  of SMBus 1.0
#define  AMD_SMB_BASE_1_EN          0x41 // SMBus 1.0 space enable
#define  AMD_SMBUS_1                0xE0 // SMBus 1.0 registers

// I/O registers
#define ICH_HST_STA                     0x00 // host status
#define ICH_HST_STA_BYTE_DONE_STS       0x80 // byte send/rec'd
#define ICH_HST_STA_INUSE_STS           0x40 // device access mutex
#define ICH_HST_STA_SMBALERT_STS        0x20 // SMBALERT# signal
#define ICH_HST_STA_FAILED              0x10 // failed bus transaction
#define ICH_HST_STA_BUS_ERR             0x08 // transaction collision 
#define ICH_HST_STA_DEV_ERR             0x04 // misc. smb device error 
#define ICH_HST_STA_ALL_ERRS            (ICH_HST_STA_FAILED|ICH_HST_STA_BUS_ERR|ICH_HST_STA_DEV_ERR)
#define ICH_HST_STA_INTR                0x02 // command completed ok 
#define ICH_HST_STA_HOST_BUSY           0x01 // command is running 
#define ICH_HST_CNT                     0x02 // host control 
#define ICH_HST_CNT_START               0x40 // start command 
#define ICH_HST_CNT_LAST_BYTE           0x20 // indicate last byte 
#define ICH_HST_CNT_SMB_CMD_QUICK       0x00 // command: quick
#define ICH_HST_CNT_SMB_CMD_BYTE        0x04 // command: byte 
#define ICH_HST_CNT_SMB_CMD_BYTE_DATA   0x08 // command: byte data
#define ICH_HST_CNT_SMB_CMD_WORD_DATA   0x0c // command: word data 
#define ICH_HST_CNT_SMB_CMD_PROC_CALL   0x10 // command: process call 
#define ICH_HST_CNT_SMB_CMD_BLOCK       0x14 // command: block
#define ICH_HST_CNT_SMB_CMD_I2C_READ    0x18 // command: i2c read 
#define ICH_HST_CNT_KILL                0x02 // kill current transaction 
#define ICH_HST_CNT_INTREN              0x01 // enable interrupt 
#define ICH_HST_CMD                     0x03 // host command
#define ICH_XMIT_SLVA                   0x04 // transmit slave address
#define ICH_XMIT_SLVA_READ              0x01 // direction: read
#define ICH_XMIT_SLVA_WRITE             0x00 // direction: write
#define ICH_D0                          0x05 // host data 0 
#define ICH_D1                          0x06 // host data 1
#define ICH_BLOCK_DB                    0x07 // block data byte

// SMBus 1.0
#define AMD_SMB_1_STATUS                0x00 // SMBus global status
#define AMD_SMB_1_STATUS_SMBS_BSY       (1<11)
#define AMD_SMB_1_STATUS_SMBA_STS       (1<10)
#define AMD_SMB_1_STATUS_HSLV_STS       (1<9)
#define AMD_SMB_1_STATUS_SNP_STS        (1<8)
#define AMD_SMB_1_STATUS_TO_STS         (1<5)
#define AMD_SMB_1_STATUS_HCYC_STS       (1<4)
#define AMD_SMB_1_STATUS_HST_BSY        (1<3)
#define AMD_SMB_1_STATUS_PERR_STS       (1<2)
#define AMD_SMB_1_STATUS_COL_STS        (1<1)
#define AMD_SMB_1_STATUS_ABRT_STS       (1<0)
#define AMD_SMB_1_STATUS_ALL_ERRS        AMD_SMB_1_STATUS_TO_STS|AMD_SMB_1_STATUS_HST_BSY|AMD_SMB_1_STATUS_PERR_STS|AMD_SMB_1_STATUS_COL_STS
#define AMD_SMB_1_CTL                   0x02 // SMBus global control
#define AMD_SMB_1_CTL_SMBA_EN           (1<<10)
#define AMD_SMB_1_CTL_HSLV_EN           (1<<9)
#define AMD_SMB_1_CTL_SNP_EN            (1<<8)
#define AMD_SMB_1_CTL_ABORT             (1<<5)
#define AMD_SMB_HCYC_EN                 (1<<4)
#define AMD_SMB_HOSTSTS                 (1<<3)
#define AMD_SMB_CYCTYPE_QC              0x0 // quick command
#define AMD_SMB_CYCTYPE_RSB             0x1 // receive/send byte
#define AMD_SMB_CYCTYPE_RWB             0x2 // read/write byte
#define AMD_SMB_CYCTYPE_RWW             0x3 // read/write word
#define AMD_SMB_CYCTYPE_PC              0x4 // process call
#define AMD_SMB_CYCTYPE_RWBL            0x5 // read/write block
#define AMD_SMB_1_ADDR                  0x04 // SMBus host address
#define AMD_SMB_1_ADDR_READCYC          (1<<0)
#define AMD_SMB_1_DATA                  0x06 // SMBus host data
#define AMD_SMB_1_CMD                   0x08 // SMBus host command
#define AMD_SMB_1_BLOCK_DATA            0x09 // SMBus block data FIFO access port

// For SMBus 2.0, see ACPI 2.0 chapter 13 PCI interface definitions

#define AMD_SMB_BLOCK_WRITE  0xa
#define AMD_SMB_BLOCK_READ   0xb

// AMD PCI control registers definitions.
#define AMD_PCI_MISC            0x48
#define AMD_PCI_MISC_SCI        0x04
#define AMD_PCI_MISC_INT        0x02
#define AMD_PCI_MISC_SPEEDUP    0x01


// SMBus Controllers
#define INTEL_SMBC           1       // Intel
#define SW_SMBC              2       // ServerWorks
#define VIA_SMBC             3       // VIA
#define AMD_SMBC             4       // AMD

/************************************************
 * bmc.h
 ************************************************/

#define STATUS_OK 	0
#define BMC_MAX_RETRIES	3	// Max number of retries if BMC is busy
#define DATA_BUF_SIZE	255	// Max data buffer, see IPMI_REQBUF_SIZE
#define GETMSGTIMEOUT	5	// Time out in sec for send/get message command

// Product IDs of systems with PC87431 chips (have both Sahalee and mBMC)
#define ID_PC87431_M		0x4311
#define ID_PC87431_S		0x4312
#define ID_PC87431_C		0x4315

// Available BMC chips that are supported by this library.
#define	BMC_UNKNOWN	0
#define	BMC_SAHALEE	1		// Intel Sahalee BMC
#define	BMC_MBMC_87431M 2		// mBMC, chip = PC87431M
#define	BMC_MBMC_87431S 3		// mBMC, chip = PC87431S
#define	BMC_MBMC_87431C 4		// mBMC, chip = PC87431C
#define	BMC_BOTH_SAHALEE_MBMC 5	
#define	BMC_DUAL_NW	6		// currently not detected
// #define BMC_AMT10       7

// Available IPMI versions.  There may be full and partial IPMI implementations.
// To figure out what implementation is supported, look at the BMC_TYPE.
#define IPMI_VERSION_UNKNOWN  0
#define IPMI_VERSION_1_5      1
#define IPMI_VERSION_2_0      2
#define IPMI_VERSION_1_0      3

//*****************************************************************************
// Message struct for sending and receiving messages from the BMC.  Requests
// must have a DevAdd, NetFn, LUN, Cmd, Data, and Len.  The response will have
// all fields filled in.  In both cases 'Len' is the length of the data buffer.
typedef struct {
	UINT8	Bus;
	UINT8	DevAdd;
	UINT8	NetFn;
	UINT8	LUN;
	UINT8	Cmd;
	UINT8	Data[DATA_BUF_SIZE];
	UINT32	Len;
	UINT8	CompCode;
} BMC_MESSAGE;

/* Internal error codes from BMC or IPMI driver:
 * ERR_NO_DRV is defined in ipmicmd.h, and the other
 * values below are not used.
 */
#define ER_NO_BMC_IF		 -400	// could not find the BMC interface
#define ERCANTLOCATEIPMIHANDLE	 -500	// Unable to locate IPMI handle
#define ERCANTALLOCMEMORYFORIPMI -501	// Unable to allocate memory for IPMI services
#define ERCANTLOADIPMIDRIVER    -502	// Unable to dynamically load IPMI driver
#define ERSENDINGIPMIMESSAGE	-503	// Error sending request to BMC
#define ERGETTINGIPMIMESSAGE	-504	// Error getting request from BMC
#define ERCMDRETURNEDDONTMATCH	-505	// Returned command # doesn't match what was sent
#define ERLUNRETURNEDDONTMATCH	-506	// Returned LUN # doesn't match what was sent
#define ERCOMPCODEERR		-507	// Comp code is not COMPCODE_NORMAL
#define ERSEQNUMBER		-508	// Sequence number error.
#define ERCANTCLEARMSGFLAGS	-509	// Can not clear message flags
#define	ERINVALIDRESPONSELEN	-510	// Response length not as expected.
#define ER_NO_PCI_BIOS		-511	// No PCI BIOS record exist
#define ER_SMBUSBUSERROR	-512
#define ER_SMBUSTIMEOUT		-513
#define ER_SMBUSOWNERSHIP	-514
#define ER_SMBUSDEVERROR	-515
#define ER_NOTKCS		-516	// Not KCS, probably SMBus

// Device Addresses
#define BMC_ADDR		0x20	// Baseboard Management Controller
#define HSC0_ADDR		0xC0	// Hot Swap Controller 0
#define HSC1_ADDR		0xC2	// Hot Swap Controller 1
#define PSC_ADDR		0xC8	// Power Supply Controller
#define CBC_ADDR		0x28	// Chassis Bridge Controller
#define LCP_ADDR		0x22	// LCP(Local Control Panel) Address
#define MBMC_ADDR		0x84    // MINI BMC address

#define SMBUS_SLAVE_ADDRESS	0x00
#define ID_VITESSE   0x01    // for HSC
#define ID_GEM       0x02    // for HSC
#define ISA_BUS			0xFF	// ISA Bus address

#define BMC_DEV_ADDR		0x20	// BMC Device Addr for Get Device ID
// #define BMC_LUN		0x00	// BMC commands & event request messages
#define OEM_LUN1		0x01	// OEM LUN 1
#define SMS_MSG_LUN		0x02	// SMS message LUN
#define OEM_LUN2		0x03	// OEM LUN 2

#define NETFN_APP       0x06 
#define GETMESSAGE_CMD		0x33	// IPMI Get Message command
#define SENDMESSAGE_CMD		0x34	// IPMI Send Message command

#endif	// IPMIDIR_H_
