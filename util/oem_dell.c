/*
 * oem_dell.c
 * Handle Dell OEM command functions
 *
 * Change history:
 *  08/17/2011 ARCress - included in ipmiutil source tree
 *  09/18/2024 ARCress - fix macos compile error with vFlashstr typedef
 *
 */
/******************************************************************
Copyright (c) 2008, Dell Inc
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution. 
- Neither the name of Dell Inc nor the names of its contributors
may be used to endorse or promote products derived from this software 
without specific prior written permission. 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE. 


******************************************************************/
/*
* Thursday Oct 7 17:30:12 2009
* <deepaganesh_paulraj@dell.com>
*
* This code implements a dell OEM proprietary commands.
* This Code is edited and Implemented the License feature for Delloem
* Author Harsha S <Harsha_S1@dell.com>
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#define int8_t   char
#define uint8_t  unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t unsigned long
#include "getopt.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <limits.h>
#if defined(HPUX)
/* getopt is defined in stdio.h */
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#include <stdint.h>
#else
#include <getopt.h>
#endif
#endif

#include <time.h>
#include "ipmicmd.h"
#include "isensor.h"
#include "ievents.h"
#include "oem_dell.h"

// #ifdef METACOMMAND is assumed 
extern int i_sol(int  argc, char **argv);  /*isol.c*/
extern void lprintf(int level, const char * format, ...); /*ipmilanplus.c*/
extern void set_loglevel(int level);
extern void printbuf(const uint8_t * buf, int len, const char * desc);

#define IPMI_DELL_OEM_NETFN        (uint8_t)(0x30)
#define GET_CHASSIS_LED_STATE      (uint8_t)(0x32)
#define GET_IDRAC_VIRTUAL_MAC      (uint8_t)(0xC9)
// 11g Support Macros
#define INVALID 		 	-1
#define SHARED 			  	0
#define SHARED_WITH_FAILOVER_LOM2 	1
#define DEDICATED 		 	2
#define SHARED_WITH_FAILOVER_ALL_LOMS 	3
char ActiveLOM_String[6][10] = {"None","LOM1","LOM2","LOM3","LOM4","dedicated"};
#define INVAILD_FAILOVER_MODE   	-2
#define INVAILD_FAILOVER_MODE_SETTINGS  -3
#define INVAILD_SHARED_MODE             -4

#define        INVAILD_FAILOVER_MODE_STRING    "ERROR: Cannot set shared with failover lom same as current shared lom.\n"
#define        INVAILD_FAILOVER_MODE_SET       "ERROR: Cannot set shared with failover loms when NIC is set to dedicated Mode.\n"
#define        INVAILD_SHARED_MODE_SET_STRING  "ERROR: Cannot set shared Mode for Blades.\n"

// 11g Support Strings for nic selection
char NIC_Selection_Mode_String [4] [50] =	{	"shared",  
						"shared with failover lom2",
						"dedicated",
						"shared with Failover all loms"
						};

// 11g Support Macros (dups)
//#define SHARED 0
//#define SHARED_WITH_FAILOVER_LOM2 1
//#define DEDICATED 2
//#define SHARED_WITH_FAILOVER_ALL_LOMS 3

// 12g Support Strings for nic selection
char NIC_Selection_Mode_String_12g[] [50] =	{	
						"dedicated",
						"shared with lom1",  
						"shared with lom2",
						"shared with lom3",
						"shared with lom4",
						"shared with failover lom1",
						"shared with failover lom2",
						"shared with failover lom3",
						"shared with failover lom4",
						"shared with failover all loms"
						};

#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil delloem";
#else
static char * progver   = "3.08";
static char * progname  = "idelloem";
#endif
static int verbose = 0;
static char fdebug = 0;
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;
static uchar *sdrcache = NULL;
static char  *sdrfile = NULL;
static int    argc_sav; 
static char **argv_sav;
static int current_arg =0;
uint8_t iDRAC_FLAG=0;
LCD_MODE lcd_mode;
static uint8_t LcdSupported=0;
static uint8_t SetLEDSupported=0;

volatile uint8_t IMC_Type = IMC_IDRAC_10G;

typedef struct
{
    int val;
    char *str;
} vFlashstr;

// const struct vFlashstr vFlash_completion_code_vals[] = {
const vFlashstr  vFlash_completion_code_vals[] = {
	{0x00, "SUCCESS"},
	{0x01, "NO_SD_CARD"},
	{0x63, "UNKNOWN_ERROR"},
	{0x00, NULL}
};


POWER_HEADROOM powerheadroom;

uint8_t PowercapSetable_flag=0;
uint8_t PowercapstatusFlag=0;

static void usage(void);

/* LCD Function prototypes */
static int ipmi_delloem_lcd_main (void * intf, int  argc, char ** argv);
static int ipmi_lcd_get_platform_model_name (void * intf,char* lcdstring,
                        uint8_t max_length,uint8_t field_type);
static int ipmi_idracvalidator_command (void * intf);
static int ipmi_lcd_get_configure_command_wh (void * intf);
static int ipmi_lcd_get_configure_command (void * intf,uint8_t *command);
static int ipmi_lcd_set_configure_command (void * intf, int command);
static int ipmi_lcd_set_configure_command_wh (void * intf, uint32_t  mode,
                        uint16_t lcdqualifier,uint8_t errordisp);
static int ipmi_lcd_get_single_line_text (void * intf, char* lcdstring, uint8_t max_length);
static int ipmi_lcd_get_info_wh(void * intf);
static int ipmi_lcd_get_info(void * intf);
static int ipmi_lcd_get_status_val(void * intf, LCD_STATUS* lcdstatus);
static int IsLCDSupported ();
static void CheckLCDSupport(void * intf);
static void ipmi_lcd_status_print( LCD_STATUS lcdstatus);
static int ipmi_lcd_get_status(void * intf );
static int ipmi_lcd_set_kvm(void * intf, char status);
static int ipmi_lcd_set_lock(void * intf,  char lock);
static int ipmi_lcd_set_single_line_text (void * intf, char * text);
static int ipmi_lcd_set_text(void * intf, char * text, int line_number);
static int ipmi_lcd_configure_wh (void * intf, uint32_t  mode ,
                       uint16_t lcdqualifier, uint8_t errordisp, 
                       int8_t line_number, char * text);
static int ipmi_lcd_configure (void * intf, int command, 
                    int8_t line_number, char * text);
static void ipmi_lcd_usage(void);

/* MAC Function prototypes */
static int ipmi_delloem_mac_main (void * intf, int  argc, char ** argv);
static int make_int(const char *str, int *value);
static void InitEmbeddedNICMacAddressValues ();
static int ipmi_macinfo_drac_idrac_virtual_mac(void* intf,uint8_t NicNum);
static int ipmi_macinfo_drac_idrac_mac(void* intf,uint8_t NicNum);
static int ipmi_macinfo_10g (void* intf, uint8_t NicNum);
static int ipmi_macinfo_11g (void* intf, uint8_t NicNum);
static int ipmi_macinfo (void* intf, uint8_t NicNum);
static void ipmi_mac_usage(void);

/* LAN Function prototypes */
static int ipmi_delloem_lan_main (void * intf, int  argc, char ** argv);
static int IsLANSupported ();
static int get_nic_selection_mode (int current_arg, char ** argv);
static int ipmi_lan_set_nic_selection (void* intf, uint8_t nic_selection);
static int ipmi_lan_get_nic_selection (void* intf);
static int get_nic_selection_mode_12g (void* intf,int iarg, char **argv, char *nic_set);
static int ipmi_lan_get_active_nic (void* intf);
static void ipmi_lan_usage(void);
static int ipmi_lan_set_nic_selection_12g (void* intf, uint8_t* nic_selection);

/* POwer monitor Function prototypes */
static int ipmi_delloem_powermonitor_main (void * intf, int  argc, char **argv);
static void ipmi_time_to_str(time_t rawTime, char* strTime);
static int ipmi_get_power_capstatus_command (void * intf);
static int ipmi_set_power_capstatus_command (void * intf,uint8_t val);
static int ipmi_powermgmt(void* intf);
static int ipmi_powermgmt_clear(void* intf,uint8_t clearValue);
static uint64_t watt_to_btuphr_conversion(uint32_t powerinwatt);
static uint32_t btuphr_to_watt_conversion(uint64_t powerinbtuphr);
static int ipmi_get_power_headroom_command (void * intf,uint8_t unit);
static int ipmi_get_power_consumption_data(void* intf,uint8_t unit);
static int ipmi_get_instan_power_consmpt_data(void* intf,
                        IPMI_INST_POWER_CONSUMPTION_DATA* instpowerconsumptiondata);
static void ipmi_print_get_instan_power_Amps_data(IPMI_INST_POWER_CONSUMPTION_DATA instpowerconsumptiondata);
static int ipmi_print_get_power_consmpt_data(void* intf,uint8_t  unit);
static int ipmi_get_avgpower_consmpt_history(void* intf,IPMI_AVGPOWER_CONSUMP_HISTORY* pavgpower );
static int ipmi_get_peakpower_consmpt_history(void* intf,IPMI_POWER_CONSUMP_HISTORY * pstPeakpower);
static int ipmi_get_minpower_consmpt_history(void* intf,IPMI_POWER_CONSUMP_HISTORY * pstMinpower);
static int ipmi_print_power_consmpt_history(void* intf,int unit );
static int ipmi_get_power_cap(void* intf,IPMI_POWER_CAP* ipmipowercap );
static int ipmi_print_power_cap(void* intf,uint8_t unit );
static int ipmi_set_power_cap(void* intf,int unit,int val );
static void ipmi_powermonitor_usage(void);

/* vFlash Function prototypes */
static int ipmi_delloem_vFlash_main(void * intf, int  argc, char ** argv);
// const char * get_vFlash_compcode_str(uint8_t vflashcompcode, const struct vFlashstr *vs);
const char * get_vFlash_compcode_str(uint8_t vflashcompcode, const vFlashstr *vs);
static int ipmi_get_sd_card_info(void* intf);
static int ipmi_delloem_vFlash_process(void* intf, int current_arg, char ** argv);
static void ipmi_vFlash_usage(void);


/* windbg Function prototypes */
volatile uint8_t windbgsession = 0;
static int ipmi_delloem_windbg_main (void * intf, int  argc, char ** argv);
static int ipmi_windbg_start (void * intf);
static int ipmi_windbg_end (void * intf);
static void ipmi_windbg_usage (void);



/* LED Function prototypes */

static int ipmi_getsesmask(int, char **);
static int CheckSetLEDSupport(void * intf);
static int IsSetLEDSupported(void);
static void ipmi_setled_usage(void);
static int ipmi_delloem_setled_main(void *intf, int  argc, char ** argv);
static int ipmi_delloem_getled_main(void *intf, int  argc, char ** argv);
static int ipmi_setled_state (void * intf, int bayId, int slotId, int state);
static int ipmi_getdrivemap (void * intf, int b, int d, int f, int *bayId, int *slotId);
//extern int optind;  /*from getopt*/

static int ipmi_sol_activate(void *intf, int j, int k) 
{ 
   char **args;
   int rv, i, n;
   int x = 0;
   n = argc_sav;
   args = argv_sav;
   /* use the user-specified args, but switch to sol */
   for (i = 0; i < n; i++) {
      if (strcmp(args[i],"ipmiutil") == 0) { x = 1; } 
      else if (strcmp(args[i],"delloem") == 0) args[i] = "sol";
      else if (strcmp(args[i],"windbg") == 0) args[i] = "-a";
      else if (strcmp(args[i],"start") == 0) args[i] = "-a";
   }
   ipmi_close_();
   if (x == 1) { args++; n--; }
   optind = 0;
   rv = i_sol(n, args);
   return rv; 
}

static int ipmi_sol_deactivate(void *intf) 
{
   char **args;
   int rv, i, n;
   int x = 0;
   n = argc_sav;
   args = argv_sav;
   /* use the user-specified args, but switch to sol */
   for (i = 0; i < n; i++) {
      if (strcmp(args[i],"ipmiutil") == 0) { x = 1; } 
      else if (strcmp(args[i],"delloem") == 0) args[i] = "sol";
      else if (strcmp(args[i],"windbg") == 0) args[i] = "-d";
      else if (strcmp(args[i],"end") == 0) args[i] = "-d";
   }
   ipmi_close_();
   if (x == 1) { args++; n--; }
   optind = 0;
   rv = i_sol(n, args);
   return rv;
}

static int sysinfo_param(const char *str) {
    // SysInfo Parameters
    //   1  = system firmware version
    //   2  = system hostname
    //   3  = primary operating system name (non-volatile)
    //   4  = operating system name (volatile)
    // 0xe3 = dell: os version
    // 0xde = dell: url
    if (!strcmp(str, "system_name")) return 0x02;
    if (!strcmp(str, "primary_os_name")) return 0x03;
    if (!strcmp(str, "os_name")) return 0x04;
    if (!strcmp(str, "dell_os_version")) return 0xe4;
    if (!strcmp(str, "dell_url")) return 0xde;
    return strtoul(str, 0, 16);
}

static void ipmi_sysinfo_usage()
{
       lprintf(LOG_NOTICE, "");
       lprintf(LOG_NOTICE, "   getsysinfo (os_name|primary_os_name|dell_os_version|dell_url)");
       lprintf(LOG_NOTICE, "      Retrieves system info from bmc for given argument");
       lprintf(LOG_NOTICE, "   setsysinfo (os_name|primary_os_name|dell_os_version|dell_url)");
       lprintf(LOG_NOTICE, "      Stores system Info for given argument to bmc");
       lprintf(LOG_NOTICE, "");
       lprintf(LOG_NOTICE, "   primary_os_name = primary operating system name");
       lprintf(LOG_NOTICE, "   os_name = secondary operating system name");
       lprintf(LOG_NOTICE, "   system_name = system hostname of server");
       lprintf(LOG_NOTICE, "   dell_os_version = running version of operating system (Dell)");
       lprintf(LOG_NOTICE, "   dell_url = url of bmc webserver (Dell)");

       lprintf(LOG_NOTICE, "");
}

static int
ipmi_delloem_sysinfo_main(void * intf, int  argc, char ** argv)
{
    int param, isset;
    char *str;
    unsigned char  infostr[256], *pos;
    int   j, ret;

    /* Is this a setsysinfo or getsysinfo */
    isset = !strncmp(argv[current_arg], "setsysinfo\0",10);

    current_arg++;
    if (argc < current_arg) {
       usage();
       return -1;
    }
    if (argc == 1 || strcmp(argv[current_arg], "help") == 0 ||
       argc < (isset ? current_arg+2 : current_arg+1)) {
       ipmi_sysinfo_usage();
       return 0;
    }
    memset(infostr, 0, sizeof(infostr));

    param = sysinfo_param(argv[current_arg++]);

    if (isset) {
	str = argv[current_arg];
	j = strlen_(str);
	ret = set_system_info(param,str,j);
    } else {
	pos = infostr;
	j = sizeof(infostr);
        ret = get_system_info(param,infostr,&j);
	printf("%s\n", infostr);
    }
    return ret;
}

static void ipmi_password_policy_usage(void)
{
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   passwordpolicy <on | off>");
    lprintf(LOG_NOTICE, "      Set the OEM Password Policy Check on or off");
    lprintf(LOG_NOTICE, "      This parameter is write-only");
    lprintf(LOG_NOTICE, "");
}

static int
ipmi_delloem_password_policy(void * intf, int  argc, char ** argv)
{
    int rv = 0;
    int rsp_len;
    struct ipmi_rq req;
    uint8_t data[4];
    uint8_t rsp[IPMI_RSPBUF_SIZE];
    uint8_t bval;
    

    current_arg++;
    if (argc < current_arg) {
        usage();
        return -1;
    }

    if (strcmp(argv[current_arg], "on") == 0) {
	bval = 0x01;
    } else if (strcmp(argv[current_arg], "off") == 0) {
	bval = 0x00;
    } else {   /* other or "help" */
        ipmi_password_policy_usage();
	return 0;
    }

    printf("Setting OEM Password Policy Check to %s ...\n", argv[current_arg]);
        req.msg.netfn = 0x30;  /*Dell OEM netfn*/
        req.msg.lun = 0;
        req.msg.cmd = 0x51;
        req.msg.data_len = 1;
        req.msg.data = data;
        data[0] = bval; 
	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
	   printf("Error setting OEM password policy: ");
	   if (rv < 0) printf("no response\n");
	   else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	}
    if (rv == 0) printf("successful\n");
    /* This works for DELL C6220 with firmware >= 1.23 */

    return(rv);
}

/*****************************************************************
* Function Name:       ipmi_delloem_main
*
* Description:         This function processes the delloem command
* Input:               intf    - ipmi interface
                       argc    - no of arguments
                       argv    - argument string array
* Output:        
*
* Return:              return code     0 - success
*                                      -1 - failure
*
******************************************************************/

int
ipmi_delloem_main(void * intf, int  argc, char ** argv)
{
    int rc = 0;

    // ipmi_idracvalidator_command(intf);
    // CheckLCDSupport (intf);
    // CheckSetLEDSupport (intf);

    if (argc == 0 || strncmp(argv[0], "help\0", 5) == 0) 
    {
        usage();
        return 0;
    }

    if (0 ==strncmp(argv[current_arg], "lcd\0", 4) ) 
    {
        rc = ipmi_delloem_lcd_main (intf,argc,argv);
    }
    /* mac address*/
    else if (strncmp(argv[current_arg], "mac\0", 4) == 0) 
    {
        rc = ipmi_delloem_mac_main (intf,argc,argv);
    }
    /* lan address*/
    else if (strncmp(argv[current_arg], "lan\0", 4) == 0) 
    {
        rc = ipmi_delloem_lan_main (intf,argc,argv);
    }
    /* SetLED support */
    else if (strncmp(argv[current_arg], "setled\0", 7) == 0)
    {
        rc = ipmi_delloem_setled_main (intf,argc,argv);
    }
    else if (strncmp(argv[current_arg], "getled\0", 13) == 0) 
    {
        rc = ipmi_delloem_getled_main (intf,argc,argv);
    }
    else if (strncmp(argv[current_arg], "passwordpolicy\0", 14) == 0) 
    {
        rc = ipmi_delloem_password_policy (intf,argc,argv);
    }
    /*Powermanagement report processing*/
    else if (strncmp(argv[current_arg], "powermonitor\0", 13) == 0) 
    {
        rc = ipmi_delloem_powermonitor_main (intf,argc,argv);
    }
	/* vFlash Support */	
	else if (strncmp(argv[current_arg], "vFlash\0", 7) == 0)
	{
        rc = ipmi_delloem_vFlash_main (intf,argc,argv);	
	}
	else if (strncmp(argv[current_arg], "windbg\0", 7) == 0) 
	{
        rc = ipmi_delloem_windbg_main (intf,argc,argv);		
	}
	else if ((strncmp(argv[current_arg], "getsysinfo\0", 10) == 0) ||
	         (strncmp(argv[current_arg], "setsysinfo\0", 10) == 0))
	{
        rc = ipmi_delloem_sysinfo_main (intf,argc,argv);		
	}
    else
    {
        usage();
        return ERR_USAGE;
    }
    return rc;
}

/*****************************************************************
* Function Name:     usage
*
* Description:       This function prints help message for delloem command
* Input:           
* Output:       
*
* Return:              
*
******************************************************************/

static void usage(void)
{
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "usage: delloem <command> [option...]");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "commands:");
    lprintf(LOG_NOTICE, "    lcd"); 
    lprintf(LOG_NOTICE, "    mac");         
    lprintf(LOG_NOTICE, "    lan");
    lprintf(LOG_NOTICE, "    setled");         
    lprintf(LOG_NOTICE, "    getled");         
    lprintf(LOG_NOTICE, "    powermonitor");        
    lprintf(LOG_NOTICE, "    windbg");	
    lprintf(LOG_NOTICE, "    vFlash");
    lprintf(LOG_NOTICE, "    getsysinfo");
    lprintf(LOG_NOTICE, "    setsysinfo");
    lprintf(LOG_NOTICE, "    passwordpolicy");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "For help on individual commands type:");
    lprintf(LOG_NOTICE, "delloem <command> help");

}

/*****************************************************************
* Function Name:       ipmi_delloem_lcd_main
*
* Description:         This function processes the delloem lcd command
* Input:               intf    - ipmi interface
                       argc    - no of arguments
                       argv    - argument string array
* Output:        
*
* Return:              return code     0 - success
*                         -1 - failure
*
******************************************************************/

static int ipmi_delloem_lcd_main (void * intf, int  argc, char ** argv)
{
    int rc = 0;

    current_arg++;
    if (argc < current_arg) 
    {
        usage();
        return -1;
    }


    /* ipmitool delloem lcd info*/
    if (argc == 1 || strcmp(argv[current_arg], "help") == 0)
    {
        ipmi_lcd_usage();
	return 0;
    }
    CheckLCDSupport (intf);
    ipmi_idracvalidator_command(intf);
    if (!IsLCDSupported()) {
        printf("lcd is not supported on this system.\n");
        return -1;
    }
    else if (strncmp(argv[current_arg], "info\0", 5) == 0) 
    {
	if((iDRAC_FLAG==IDRAC_11G) || (iDRAC_FLAG==IDRAC_12G) )            
            rc = ipmi_lcd_get_info_wh(intf);
        else
            rc = ipmi_lcd_get_info(intf);
    }
    else if (strncmp(argv[current_arg], "status\0", 7) == 0)
    {
        rc = ipmi_lcd_get_status(intf);
    }
    /* ipmitool delloem lcd set*/
    else if (strncmp(argv[current_arg], "set\0", 4) == 0) 
    {
        uint8_t line_number = 0;
        current_arg++;
        if (argc <= current_arg) 
        {
            ipmi_lcd_usage();
            return -1;
        }
        if (strncmp(argv[current_arg], "line\0", 5) == 0) 
        {
            current_arg++;
            if (argc <= current_arg) {usage();return -1;}
            line_number = (uint8_t)strtoul(argv[current_arg], NULL, 0);
            current_arg++;
            if (argc <= current_arg) {usage();return -1;}
        }


		if ((strncmp(argv[current_arg], "mode\0", 5) == 0)&&((iDRAC_FLAG==IDRAC_11G) || (iDRAC_FLAG==IDRAC_12G) )) 
        {
            current_arg++;
            if (argc <= current_arg) 
            {
                ipmi_lcd_usage();
                return -1;
            }    
            if (argv[current_arg] == NULL)  
            {
                ipmi_lcd_usage();
                return -1;
            }
            if (strncmp(argv[current_arg], "none\0", 5) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_CONFIG_NONE,0xFF,0XFF, 0, NULL);
            }
            else if (strncmp(argv[current_arg], "modelname\0", 10) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_CONFIG_DEFAULT,0xFF,0XFF, 0, NULL);
            }
            else if (strncmp(argv[current_arg], "userdefined\0", 12) == 0) 
            {
                current_arg++;
                if (argc <= current_arg) 
                {
                    ipmi_lcd_usage();return -1;
                }
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_CONFIG_USER_DEFINED,0xFF,0XFF, line_number, argv[current_arg]);
            }
            else if (strncmp(argv[current_arg], "ipv4address\0", 12) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_iDRAC_IPV4ADRESS  ,0xFF,0XFF, 0, NULL);
            }
            else if (strncmp(argv[current_arg], "macaddress\0", 11) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_IDRAC_MAC_ADDRESS,0xFF,0XFF, 0, NULL);
            }
            else if (strncmp(argv[current_arg], "systemname\0", 11) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_OS_SYSTEM_NAME,0xFF,0XFF, 0, NULL);
            }
            else if (strncmp(argv[current_arg], "servicetag\0", 11) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_SERVICE_TAG, 0xFF,0XFF,0, NULL);
            }
            else if (strncmp(argv[current_arg], "ipv6address\0", 12) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_iDRAC_IPV6ADRESS  ,0xFF,0XFF, 0, NULL);
            }
            else if (strncmp(argv[current_arg], "ambienttemp\0", 12) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_AMBEINT_TEMP, 0xFF,0XFF,0, NULL);

            }
            else if (strncmp(argv[current_arg], "systemwatt\0", 11) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_SYSTEM_WATTS , 0xFF,0XFF,0, NULL);

            }
            else if (strncmp(argv[current_arg], "assettag\0", 9) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, IPMI_DELL_LCD_ASSET_TAG , 0xFF,0XFF,0, NULL);

            }
            else if (strncmp(argv[current_arg], "help\0", 5) == 0) 
            {
                ipmi_lcd_usage();
            }
            else
            {       
                ipmi_lcd_usage();
            }
        }
		else if ((strncmp(argv[current_arg], "lcdqualifier\0", 13)== 0) &&((iDRAC_FLAG==IDRAC_11G) || (iDRAC_FLAG==IDRAC_12G) ) )
        {

            current_arg++;
            if (argc <= current_arg) 
            {
                ipmi_lcd_usage();
                return -1;
            }
            if (argv[current_arg] == NULL) 
            {
                ipmi_lcd_usage();
                return -1;
            } 

            if (strncmp(argv[current_arg], "watt\0", 5) == 0) {


                rc = ipmi_lcd_configure_wh (intf, 0xFF,0x00,0XFF, 0, NULL);
            }
            else if (strncmp(argv[current_arg], "btuphr\0",7) == 0) {
                rc = ipmi_lcd_configure_wh (intf, 0xFF,0x01,0XFF, 0, NULL);

            } else if (strncmp(argv[current_arg], "celsius\0", 8) == 0) {
                rc = ipmi_lcd_configure_wh (intf, 0xFF,0x02,0xFF, 0, NULL);
            } else if (strncmp(argv[current_arg], "fahrenheit", 11) == 0) {
                rc = ipmi_lcd_configure_wh (intf, 0xFF,0x03,0xFF, 0, NULL);

            }else if (strncmp(argv[current_arg], "help\0", 5) == 0) {
                ipmi_lcd_usage();
            }
            else {  
                ipmi_lcd_usage();
            }
        }
		else if( (strncmp(argv[current_arg], "errordisplay\0", 13) == 0)&&((iDRAC_FLAG==IDRAC_11G) || (iDRAC_FLAG==IDRAC_12G) )) 
        {

            current_arg++;
            if (argc <= current_arg)
            {
                ipmi_lcd_usage();
                return -1;
            }
            if (argv[current_arg] == NULL) 
            { 
                ipmi_lcd_usage();
                return -1;
            } 

            if (strncmp(argv[current_arg], "sel\0", 4) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, 0xFF,0xFF,IPMI_DELL_LCD_ERROR_DISP_SEL , 0, NULL);
            }
            else if (strncmp(argv[current_arg], "simple\0", 7) == 0) 
            {
                rc = ipmi_lcd_configure_wh (intf, 0xFF,0xFF,IPMI_DELL_LCD_ERROR_DISP_VERBOSE , 0, NULL);

            }
            else if (strncmp(argv[current_arg], "help\0", 5) == 0) 
            {
                ipmi_lcd_usage();
            }
            else 
            {       
                ipmi_lcd_usage();
            }
        }

        else if ((strncmp(argv[current_arg], "none\0", 5) == 0)&&(iDRAC_FLAG==0)) 
        {
            rc = ipmi_lcd_configure (intf, IPMI_DELL_LCD_CONFIG_NONE, 0, NULL);
        }
        else if ((strncmp(argv[current_arg], "default\0", 8) == 0)&&(iDRAC_FLAG==0)) 
        {
            rc = ipmi_lcd_configure (intf, IPMI_DELL_LCD_CONFIG_DEFAULT, 0, NULL);

        } 
        else if ((strncmp(argv[current_arg], "custom\0", 7) == 0)&&(iDRAC_FLAG==0))  
        {
            current_arg++;
            if (argc <= current_arg) 
            {
                ipmi_lcd_usage();
                return -1;
            }
            rc = ipmi_lcd_configure (intf, IPMI_DELL_LCD_CONFIG_USER_DEFINED, line_number, argv[current_arg]);
        } 

        else if (strncmp(argv[current_arg], "vkvm\0", 5) == 0) 
        {
            current_arg++;
            if (argc <= current_arg) 
            {
                ipmi_lcd_usage();
                return -1;
            }

            if (strncmp(argv[current_arg], "active\0", 7) == 0) 
            {
                rc = ipmi_lcd_set_kvm (intf, 1);
            }
            else if (strncmp(argv[current_arg], "inactive\0", 9)==0)
            {
                rc = ipmi_lcd_set_kvm (intf, 0);

            }
            else if (strncmp(argv[current_arg], "help\0", 5) == 0) 
            {
                ipmi_lcd_usage();
            }
            else 
            {       
                ipmi_lcd_usage();
            }

        }
        else if (strncmp(argv[current_arg], "frontpanelaccess\0", 17) == 0) 
        {
            current_arg++;
            if (argc <= current_arg) 
            {
                ipmi_lcd_usage();
                return -1;
            }
            if (strncmp(argv[current_arg], "viewandmodify\0", 14) == 0) 
            {
                rc = ipmi_lcd_set_lock (intf, 0);
            }
            else if (strncmp(argv[current_arg], "viewonly\0", 9)==0)
            {
                rc =  ipmi_lcd_set_lock (intf, 1);

            }
            else if (strncmp(argv[current_arg], "disabled\0", 9)==0)
            {
                rc =  ipmi_lcd_set_lock (intf, 2);

            }
            else if (strncmp(argv[current_arg], "help\0", 5) == 0) 
            {
                ipmi_lcd_usage();
            }
            else 
            {       
                ipmi_lcd_usage();
            }

        }
        else if( (strncmp(argv[current_arg], "help\0", 5) == 0)&&(iDRAC_FLAG==0))  
        {
            ipmi_lcd_usage();
        }
        else 
        {
            ipmi_lcd_usage();
            return -1;
        }
    } 
    else 
    {
        ipmi_lcd_usage();
        return -1;
    }
    return(rc);
}



/*****************************************************************
* Function Name:      ipmi_lcd_get_platform_model_name
*
* Description: This function retrieves the platform model name, or any other parameter 
*              which stores  data in the same format
* Input:       intf         - pointer to interface
*              max_length   - length of the platform model string
*              field_type   - either hostname / platform model
* Output:      lcdstring    - hostname / platform model string
*
* Return:            
*
******************************************************************/ 
static int
ipmi_lcd_get_platform_model_name (void * intf, 
                          char* lcdstring, 
                                  uint8_t max_length, 
                                  uint8_t field_type)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE];
    int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];
    IPMI_DELL_LCD_STRING * lcdstringblock;
    int lcdstring_len = 0;
    int bytes_copied = 0;

    int ii;

    for (ii = 0; ii < 4; ii++)
    {
        int bytes_to_copy;
		memset (&req,0,sizeof(req));
        req.msg.netfn = IPMI_NETFN_APP;
        req.msg.lun = 0;
        req.msg.cmd = IPMI_GET_SYS_INFO;
        req.msg.data_len = 4;
        req.msg.data = data;
        data[0] = 0;                            /* get parameter*/
        data[1] = field_type;
        data[2] = ii;
        data[3] = 0;


	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
	   printf("Error getting platform model name: ");
	   if (rv < 0) printf("no response\n");
	   else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	   return rv;
        }

        lcdstringblock = (IPMI_DELL_LCD_STRING *) (void *) rsp;

        /* first block is different - 14 bytes*/
        if (0 == ii) {
            lcdstring_len = lcdstringblock->lcd_string.selector_0_string.length;

            lcdstring_len = MIN (lcdstring_len,max_length);

            bytes_to_copy = MIN(lcdstring_len, IPMI_DELL_LCD_STRING1_SIZE);
            memcpy (lcdstring, lcdstringblock->lcd_string.selector_0_string.data, bytes_to_copy);
        } else {
            int string_offset;

            bytes_to_copy = MIN(lcdstring_len - bytes_copied, IPMI_DELL_LCD_STRINGN_SIZE);
            if (bytes_to_copy < 1)
                break;
            string_offset = IPMI_DELL_LCD_STRING1_SIZE + IPMI_DELL_LCD_STRINGN_SIZE * (ii-1);
            memcpy (lcdstring+string_offset, lcdstringblock->lcd_string.selector_n_data, bytes_to_copy);
        }


        bytes_copied += bytes_to_copy;

        if (bytes_copied >= lcdstring_len)

            break;
    }
    return rv;
}

/*****************************************************************
* Function Name:    ipmi_idracvalidator_command
*
* Description:      This function returns the iDRAC6 type
* Input:            intf            - ipmi interface
* Output:       
*
* Return:           iDRAC6 type     1 - whoville 
*                                   0 - others
*
******************************************************************/

static int
ipmi_idracvalidator_command (void * intf)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE];
    int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];

    memset (&req,0,sizeof(req));
    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    req.msg.data = data;
    data[0] = 0;
    data[1] = IPMI_DELL_IDRAC_VALIDATOR;
    data[2] = 2;
    data[3] = 0;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv && fdebug) {
        printf(" Error getting IMC type");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }
    IMC_Type = rsp[10];
    if( (IMC_Type == IMC_IDRAC_11G_MONOLITHIC) || (IMC_Type == IMC_IDRAC_11G_MODULAR) )
    {
		iDRAC_FLAG=IDRAC_11G;
    }
    else if( (IMC_Type == IMC_IDRAC_12G_MONOLITHIC) || (IMC_Type == IMC_IDRAC_12G_MODULAR) )
    {
		iDRAC_FLAG=IDRAC_12G;
    }
    else
    {
        iDRAC_FLAG=0;
    }       
    
    return 0;
}

/*****************************************************************
* Function Name:    ipmi_lcd_get_configure_command_wh
*
* Description:      This function returns current lcd configuration for Dell OEM LCD command
* Input:            intf            - ipmi interface
* Global:           lcd_mode - lcd mode setting
* Output:    
*
* Return:           returns the current lcd configuration
*                   0 = User defined
*                   1 = Default
*                   2 = None
*
******************************************************************/ 
static int
ipmi_lcd_get_configure_command_wh (void * intf)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE];
    int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];

    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    req.msg.data = data;
    data[0] = 0;
    data[1] = IPMI_DELL_LCD_CONFIG_SELECTOR;
    data[2] = 0;
    data[3] = 0;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error getting LCD configuration: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }

    lcd_mode= *((LCD_MODE*)(&rsp[0]));
    return 0;
}


/*****************************************************************
* Function Name:    ipmi_lcd_get_configure_command
*
* Description:   This function returns current lcd configuration for Dell OEM LCD command
* Input:         intf            - ipmi interface
* Output:        command         - user defined / default / none / ipv4 / mac address / 
                 system name / service tag / ipv6 / temp / system watt / asset tag
*
* Return:             
*
******************************************************************/

static int
ipmi_lcd_get_configure_command (void * intf,
                                uint8_t *command)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE];
    int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];

    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    req.msg.data = data;
    data[0] = 0;
    data[1] = IPMI_DELL_LCD_CONFIG_SELECTOR;
    data[2] = 0;
    data[3] = 0;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error getting LCD configuration: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }

    /* rsp->data[0] is the rev */
    *command = rsp[1];

    return 0;
}

/*****************************************************************
* Function Name:    ipmi_lcd_set_configure_command
*
* Description:      This function updates current lcd configuration 
* Input:            intf            - ipmi interface
*                   command         - user defined / default / none / ipv4 / mac address / 
*                        system name / service tag / ipv6 / temp / system watt / asset tag
* Output:
* Return:             
*
******************************************************************/

static int
ipmi_lcd_set_configure_command (void * intf, int command)
{
#define LSCC_DATA_LEN 2

    uint8_t rsp[IPMI_RSPBUF_SIZE];
    int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[2];

    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_SET_SYS_INFO;
    req.msg.data_len = 2;
    req.msg.data = data;
    data[0] = IPMI_DELL_LCD_CONFIG_SELECTOR;
    data[1] = command;                      /* command - custom, default, none */

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error setting LCD configuration: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }

    return 0;
}


/*****************************************************************
* Function Name:    ipmi_lcd_set_configure_command
*
* Description:      This function updates current lcd configuration 
* Input:            intf            - ipmi interface
*                   mode            - user defined / default / none 
*                   lcdqualifier   - lcd quallifier id
*                   errordisp       - error number
* Output:
* Return:                
*
******************************************************************/ 
static int
ipmi_lcd_set_configure_command_wh (void * intf, 
                                   uint32_t  mode,
                                   uint16_t lcdqualifier,
                                   uint8_t errordisp)
{
#define LSCC_DATA_LEN 2

    uint8_t rsp[IPMI_RSPBUF_SIZE];
    int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[13];

    ipmi_lcd_get_configure_command_wh(intf);
    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_SET_SYS_INFO;
    req.msg.data_len = 13;
    req.msg.data = data;
    data[0] = IPMI_DELL_LCD_CONFIG_SELECTOR;

    if(mode!=0xFF)
    {

        data[1] = mode&0xFF;                    /* command - custom, default, none*/
        data[2]=(mode&0xFF00)>>8;
        data[3]=(mode&0xFF0000)>>16;
        data[4]=(mode&0xFF000000)>>24;
    }
    else
    {
        data[1] = (lcd_mode.lcdmode)&0xFF;                      /* command - custom, default, none*/
        data[2]=((lcd_mode.lcdmode)&0xFF00)>>8;
        data[3]=((lcd_mode.lcdmode)&0xFF0000)>>16;
        data[4]=((lcd_mode.lcdmode)&0xFF000000)>>24;
    }

    if(lcdqualifier!=0xFF)
    {
        if(lcdqualifier==0x01)
        {
            data[5] =(lcd_mode.lcdqualifier)|0x01;                 /* command - custom, default, none*/

        }
        else  if(lcdqualifier==0x00)
        {
            data[5] =(lcd_mode.lcdqualifier)&0xFE;                 /* command - custom, default, none*/
        }
        else if (lcdqualifier==0x03)
        {
            data[5] =(lcd_mode.lcdqualifier)|0x02;                 /* command - custom, default, none*/
        }
        else if (lcdqualifier==0x02)
        {
            data[5] =(lcd_mode.lcdqualifier)&0xFD; 
        }
    }
    else
    {
        data[5]=(uchar)lcd_mode.lcdqualifier;
    }   
    if(errordisp!=0xFF)
    {
        data[11]=errordisp;
    }
    else
    {
        data[11]=lcd_mode.error_display;
    }
    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error setting LCD configuration: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }

    return 0;
}



/*****************************************************************
* Function Name:    ipmi_lcd_get_single_line_text
*
* Description:    This function updates current lcd configuration 
* Input:          intf            - ipmi interface
*                 lcdstring       - new string to be updated 
*                 max_length      - length of the string
* Output:
* Return:              
*
******************************************************************/

static int
ipmi_lcd_get_single_line_text (void * intf, char* lcdstring, uint8_t max_length)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE];
    int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];
    IPMI_DELL_LCD_STRING * lcdstringblock;
    int lcdstring_len = 0;
    int bytes_copied = 0;
    int ii;

    for (ii = 0; ii < 4; ii++) {
        int bytes_to_copy;

        req.msg.netfn = IPMI_NETFN_APP;
        req.msg.lun = 0;
        req.msg.cmd = IPMI_GET_SYS_INFO;
        req.msg.data_len = 4;
        req.msg.data = data;
        data[0] = 0;                            /* get parameter*/
        data[1] = IPMI_DELL_LCD_STRING_SELECTOR;
        data[2] = ii;                           /* block selector*/
        data[3] = 0;                            /* set selector (n/a)*/

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
	   printf("Error getting text data: ");
	   if (rv < 0) printf("no response\n");
	   else printf("Completion Code 0x%02x\n", rv);
	   return rv;
	}

        lcdstringblock = (IPMI_DELL_LCD_STRING *) (void *) rsp;

        /* first block is different - 14 bytes*/
        if (0 == ii)
        {
            lcdstring_len = lcdstringblock->lcd_string.selector_0_string.length;

            if (lcdstring_len < 1 || lcdstring_len > max_length)
                break;

            bytes_to_copy = MIN(lcdstring_len, IPMI_DELL_LCD_STRING1_SIZE);
            memcpy (lcdstring, lcdstringblock->lcd_string.selector_0_string.data, bytes_to_copy);
        }
        else
        {
            int string_offset;

            bytes_to_copy = MIN(lcdstring_len - bytes_copied, IPMI_DELL_LCD_STRINGN_SIZE);
            if (bytes_to_copy < 1)
                break;
            string_offset = IPMI_DELL_LCD_STRING1_SIZE + IPMI_DELL_LCD_STRINGN_SIZE * (ii-1);
            memcpy (lcdstring+string_offset, lcdstringblock->lcd_string.selector_n_data, bytes_to_copy);
        }

        bytes_copied += bytes_to_copy;
        if (bytes_copied >= lcdstring_len)
            break;
    }
    return 0;
}

/*****************************************************************
* Function Name:    ipmi_lcd_get_info_wh
*
* Description:     This function prints current lcd configuration for whoville platform
* Input:           intf            - ipmi interface
* Output:
* Return:              
*
******************************************************************/

static int
ipmi_lcd_get_info_wh(void * intf)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];
    IPMI_DELL_LCD_CAPS* lcd_caps;
    char lcdstring[IPMI_DELL_LCD_STRING_LENGTH_MAX+1] = {0};

    printf("LCD info\n");

    if (ipmi_lcd_get_configure_command_wh (intf) != 0) 
    {
        return -1;
    }
    else 
    {
        if (lcd_mode.lcdmode== IPMI_DELL_LCD_CONFIG_DEFAULT) 
        {
            char text[IPMI_DELL_LCD_STRING_LENGTH_MAX+1] = {0};

            ipmi_lcd_get_platform_model_name(intf, text, 
                IPMI_DELL_LCD_STRING_LENGTH_MAX,
                IPMI_DELL_PLATFORM_MODEL_NAME_SELECTOR);

            if (text == NULL)
                return -1;
            printf("    Setting:Model name\n");
            printf("    Line 1:  %s\n", text);
        }
        else if (lcd_mode.lcdmode == IPMI_DELL_LCD_CONFIG_NONE) 
        {
            printf("    Setting:   none\n");
        }
        else if (lcd_mode.lcdmode == IPMI_DELL_LCD_CONFIG_USER_DEFINED) 
        {
            req.msg.netfn = IPMI_NETFN_APP;
            req.msg.lun = 0;
            req.msg.cmd = IPMI_GET_SYS_INFO;
            req.msg.data_len = 4;
            req.msg.data = data;
            data[0] = 0;                            /* get parameter*/
            data[1] = IPMI_DELL_LCD_GET_CAPS_SELECTOR;
            data[2] = 0;                            /* set selector (n/a)*/
            data[3] = 0;                            /* block selector (n/a)*/

            printf("    Setting: User defined\n");

            rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error getting LCD capabilities: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }

            lcd_caps = (IPMI_DELL_LCD_CAPS *)rsp;
            if (lcd_caps->number_lines > 0) 
            {
                memset(lcdstring, 0, IPMI_DELL_LCD_STRING_LENGTH_MAX+1);

                rv = ipmi_lcd_get_single_line_text (intf, lcdstring, lcd_caps->max_chars[0]);
                printf("    Text:    %s\n", lcdstring);
            }
            else 
            {
                printf("    No lines to show\n");
            }
        }
        else if (lcd_mode.lcdmode == IPMI_DELL_LCD_iDRAC_IPV4ADRESS) 
        {
            printf("    Setting:   IPV4 Address\n");
        }
        else if (lcd_mode.lcdmode == IPMI_DELL_LCD_IDRAC_MAC_ADDRESS) 
        {
            printf("    Setting:   MAC Address\n");
        }
        else if (lcd_mode.lcdmode == IPMI_DELL_LCD_OS_SYSTEM_NAME) 
        {
            printf("    Setting:   OS System Name\n");
        }
        else if (lcd_mode.lcdmode == IPMI_DELL_LCD_SERVICE_TAG) 
        {
            printf("    Setting:   System Tag\n");
        }
        else if (lcd_mode.lcdmode == IPMI_DELL_LCD_iDRAC_IPV6ADRESS) 
        {
            printf("    Setting:  IPV6 Address\n");
        }
        else if (lcd_mode.lcdmode == IPMI_DELL_LCD_AMBEINT_TEMP) 
        {
            printf("    Setting:  Ambient Temp\n");
            if(lcd_mode.lcdqualifier&0x02)
                printf("    Unit:  F\n");  
            else
                printf("    Unit:  C\n");  
        }
        else if (lcd_mode.lcdmode == IPMI_DELL_LCD_SYSTEM_WATTS)
        {
            printf("    Setting:  System Watts\n");

            if(lcd_mode.lcdqualifier&0x01)
                printf("    Unit:  BTU/hr\n");  
            else
                printf("    Unit:  Watt\n"); 

        }
        if(lcd_mode.error_display==IPMI_DELL_LCD_ERROR_DISP_SEL)
            printf("    Error Display:  SEL\n");
        else if(lcd_mode.error_display==IPMI_DELL_LCD_ERROR_DISP_VERBOSE)
            printf("    Error Display:  Simple\n");
    }

    return 0;
}

/*****************************************************************
* Function Name:    ipmi_lcd_get_info
*
* Description:      This function prints current lcd configuration for platform other than whoville
* Input:            intf            - ipmi interface
* Output:
* Return:              
*
******************************************************************/
static int ipmi_lcd_get_info(void * intf)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];
    IPMI_DELL_LCD_CAPS * lcd_caps;
    uint8_t command = 0;
    char lcdstring[IPMI_DELL_LCD_STRING_LENGTH_MAX+1] = {0};

    printf("LCD info\n");

    if (ipmi_lcd_get_configure_command (intf, &command) != 0)
    {
        return -1;
    }
    else
    {
        if (command == IPMI_DELL_LCD_CONFIG_DEFAULT)
        {
            memset (lcdstring,0,IPMI_DELL_LCD_STRING_LENGTH_MAX+1);

            ipmi_lcd_get_platform_model_name(intf, lcdstring, IPMI_DELL_LCD_STRING_LENGTH_MAX,
                IPMI_DELL_PLATFORM_MODEL_NAME_SELECTOR);

            printf("    Setting: default\n");
            printf("    Line 1:  %s\n", lcdstring);
        }
        else if (command == IPMI_DELL_LCD_CONFIG_NONE)
        {
            printf("    Setting:   none\n");
        }
        else if (command == IPMI_DELL_LCD_CONFIG_USER_DEFINED)
        {
            req.msg.netfn = IPMI_NETFN_APP;
            req.msg.lun = 0;
            req.msg.cmd = IPMI_GET_SYS_INFO;
            req.msg.data_len = 4;
            req.msg.data = data;
            data[0] = 0;                            /* get parameter */
            data[1] = IPMI_DELL_LCD_GET_CAPS_SELECTOR;
            data[2] = 0;                            /* set selector (n/a) */
            data[3] = 0;                            /* block selector (n/a) */

            printf("    Setting: custom\n");

            rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error getting LCD capabilities: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }

            lcd_caps = (IPMI_DELL_LCD_CAPS *)(void *)rsp;
            if (lcd_caps->number_lines > 0)
            {
                memset (lcdstring,0,IPMI_DELL_LCD_STRING_LENGTH_MAX+1);
                rv = ipmi_lcd_get_single_line_text (intf, lcdstring, lcd_caps->max_chars[0]);
                printf("    Text:    %s\n", lcdstring);
            }
            else
            {
                printf("    No lines to show\n");
            }
        }
    }

    return 0;
}

/*****************************************************************
* Function Name:    ipmi_lcd_get_status_val
*
* Description:      This function gets current lcd configuration 
* Input:            intf            - ipmi interface
* Output:           lcdstatus       - KVM Status & Lock Status
* Return:           
*
******************************************************************/

static int
ipmi_lcd_get_status_val(void * intf, LCD_STATUS* lcdstatus)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];


    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    req.msg.data = data;
    data[0] = 0;                            /* get parameter */
    data[1] = IPMI_DELL_LCD_STATUS_SELECTOR;
    data[2] = 0;                            /* block selector */
    data[3] = 0;            
    /* set selector (n/a) */
    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error getting LCD status: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }

    /*lcdstatus= (LCD_STATUS* ) rsp->data; */

    lcdstatus->vKVM_status=rsp[1];
    lcdstatus->lock_status=rsp[2];

    return 0;
}


/*****************************************************************
* Function Name:    IsLCDSupported
*
* Description:   This function returns whether lcd supported or not
* Input:              
* Output:       
* Return:               
*
******************************************************************/
static int IsLCDSupported ()
{
    return LcdSupported;
}

/*****************************************************************
* Function Name:         CheckLCDSupport
*
* Description:  This function checks whether lcd supported or not
* Input:        intf            - ipmi interface
* Output:       
* Return:               
*
******************************************************************/
static void CheckLCDSupport(void * intf)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];

    LcdSupported = 0;

    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    req.msg.data = data;
    data[0] = 0;                            /* get parameter */
    data[1] = IPMI_DELL_LCD_STATUS_SELECTOR;
    data[2] = 0;                            /* block selector */
    data[3] = 0;            
    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) { return; }
    LcdSupported = 1;       

}

/*****************************************************************
* Function Name:     ipmi_lcd_status_print
*
* Description:    This function prints current lcd configuration KVM Status & Lock Status
* Input:          lcdstatus - KVM Status & Lock Status
* Output:   
* Return:               
*
******************************************************************/

static void ipmi_lcd_status_print( LCD_STATUS lcdstatus)
{
    switch (lcdstatus.vKVM_status)
    {
    case 0x00: 
        printf("LCD KVM Status :Inactive\n");
        break;
    case 0x01: 
        printf("LCD KVM Status :Active\n");
        break;
    default:
        printf("LCD KVM Status :Invalid Status\n");

        break;
    }                       

    switch (lcdstatus.lock_status)
    {
    case 0x00: 
        printf("LCD lock Status :View and modify\n");
        break;
    case 0x01: 
        printf("LCD lock Status :View only\n");
        break;
    case 0x02:
        printf("LCD lock Status :disabled\n");
        break;
    default:
        printf("LCD lock Status :Invalid\n");
        break;
    }

}

/*****************************************************************
* Function Name:     ipmi_lcd_get_status
*
* Description:      This function gets current lcd KVM active status & lcd access mode
* Input:            intf            - ipmi interface
* Output:       
* Return:           -1 on error
*                   0 if successful
*
******************************************************************/
static int
ipmi_lcd_get_status(void * intf )
{
    int rc=0;
    LCD_STATUS  lcdstatus;

    rc =ipmi_lcd_get_status_val( intf, &lcdstatus);
    if (rc <0)
        return -1;
    ipmi_lcd_status_print(lcdstatus);

    return rc;

}

/*****************************************************************
* Function Name:     ipmi_lcd_set_kvm
*
* Description:       This function sets lcd KVM active status 
* Input:             intf            - ipmi interface
*                    status  - Inactive / Active
* Output:    
* Return:            -1 on error
*                    0 if successful
*
******************************************************************/ 
static int
ipmi_lcd_set_kvm(void * intf, char status)
{
#define LSCC_DATA_LEN 2
    LCD_STATUS lcdstatus;
    int rc=0;
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[5];
    rc=ipmi_lcd_get_status_val(intf,&lcdstatus);
    if (rc < 0)
        return -1;
    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_SET_SYS_INFO;
    req.msg.data_len = 5;
    req.msg.data = data;
    data[0] = IPMI_DELL_LCD_STATUS_SELECTOR;
    data[1] = status;                       /* active- incative*/
    data[2] = lcdstatus.lock_status;        /* full-veiw-locked */
    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error setting LCD status: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }

    return rc;
}

/*****************************************************************
* Function Name:   ipmi_lcd_set_lock
*
* Description:     This function sets lcd access mode 
* Input:           intf            - ipmi interface
*                  lock    - View and modify / View only / Diabled
* Output:        
* Return:          -1 on error
*                  0 if successful
*
******************************************************************/ 
static int
ipmi_lcd_set_lock(void * intf,  char lock)
{
#define LSCC_DATA_LEN 2
    LCD_STATUS lcdstatus;
    int rc =0;
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[5];
    rc=ipmi_lcd_get_status_val(intf,&lcdstatus);
    if (rc < 0)
        return -1;
    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_SET_SYS_INFO;
    req.msg.data_len = 5;
    req.msg.data = data;
    data[0] = IPMI_DELL_LCD_STATUS_SELECTOR;
    data[1] = lcdstatus.vKVM_status;                        /* active- incative */
    data[2] = lock;                 /* full- veiw-locked */
    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error setting LCD status: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }

    return rc;
}

/*****************************************************************
* Function Name:   ipmi_lcd_set_single_line_text
*
* Description:    This function sets lcd line text
* Input:          intf            - ipmi interface
*                 text    - lcd string
* Output:   
* Return:         -1 on error
*                 0 if successful
*
******************************************************************/

static int 
ipmi_lcd_set_single_line_text (void * intf, char * text)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[18];
    int bytes_to_store = strlen_(text);
    int bytes_stored = 0;
    int ii;
    int rc = 0;
    if (bytes_to_store>IPMI_DELL_LCD_STRING_LENGTH_MAX)
    {
        lprintf(LOG_ERR, " Out of range Max limit is 62 characters");
        return 1;

    }
    else
    {
        bytes_to_store = MIN(bytes_to_store, IPMI_DELL_LCD_STRING_LENGTH_MAX);
        for (ii = 0; ii < 4; ii++) {
            /*first block, 2 bytes parms and 14 bytes data*/
            if (0 == ii) {
                int size_of_copy =  
                    MIN((bytes_to_store - bytes_stored), IPMI_DELL_LCD_STRING1_SIZE);
                if (size_of_copy < 0)           /* allow 0 string length*/
                    break;
                req.msg.netfn = IPMI_NETFN_APP;
                req.msg.lun = 0;
                req.msg.cmd = IPMI_SET_SYS_INFO;
                req.msg.data_len = size_of_copy + 4; /* chars, selectors and sizes*/
                req.msg.data = data;
                data[0] = IPMI_DELL_LCD_STRING_SELECTOR;
                data[1] = ii;                           /* block number to use (0)*/
                data[2] = 0;                            /*string encoding*/
                data[3] = bytes_to_store;       /* total string length*/
                memcpy (data+4, text+bytes_stored, size_of_copy);
                bytes_stored += size_of_copy;
            } else {
                int size_of_copy =  
                    MIN((bytes_to_store - bytes_stored), IPMI_DELL_LCD_STRINGN_SIZE);
                if (size_of_copy <= 0)
                    break;
                req.msg.netfn = IPMI_NETFN_APP;
                req.msg.lun = 0;
                req.msg.cmd = IPMI_SET_SYS_INFO;
                req.msg.data_len = size_of_copy + 2;
                req.msg.data = data;
                data[0] = IPMI_DELL_LCD_STRING_SELECTOR;
                data[1] = ii;                           /* block number to use (1,2,3)*/
                memcpy (data+2, text+bytes_stored, size_of_copy);
                bytes_stored += size_of_copy;
            }

            rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error setting text data: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }
        }
    }
    return rc;
}

/*****************************************************************
* Function Name:   ipmi_lcd_set_text
*
* Description:     This function sets lcd line text
* Input:           intf            - ipmi interface
*                  text    - lcd string
*                  line_number- line number

* Output:       
* Return:          -1 on error
*                  0 if successful
*
******************************************************************/

static int
ipmi_lcd_set_text(void * intf, char * text, int line_number)
{
    int rc = 0;

    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];
    IPMI_DELL_LCD_CAPS * lcd_caps;

    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    req.msg.data = data;
    data[0] = 0;                            /* get parameter*/
    data[1] = IPMI_DELL_LCD_GET_CAPS_SELECTOR;
    data[2] = 0;                            /* set selector (n/a)*/
    data[3] = 0;                            /* block selector (n/a)*/

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf("Error getting LCD capabilities: ");
        if (rv < 0) printf("no response\n");
        else printf("Completion Code 0x%02x\n", rv);
        return rv;
    }

    lcd_caps = (IPMI_DELL_LCD_CAPS *)(void *)rsp;

    if (lcd_caps->number_lines > 0) {
        rc = ipmi_lcd_set_single_line_text (intf, text);
    } else {
        lprintf(LOG_ERR, "LCD does not have any lines that can be set");
        rc = -1;
    }


    return rc;
}

/*****************************************************************
* Function Name:   ipmi_lcd_configure_wh
*
* Description:     This function updates the current lcd configuration
* Input:           intf            - ipmi interface
*                  lcdqualifier- lcd quallifier
*                  errordisp       - error number
*                  line_number-line number
*                  text            - lcd string
* Output:   
* Return:          -1 on error
*                  0 if successful
*
******************************************************************/

static int
ipmi_lcd_configure_wh (void * intf, uint32_t  mode ,
                       uint16_t lcdqualifier, uint8_t errordisp, 
                       int8_t line_number, char * text)
{
    int rc = 0;


    if (IPMI_DELL_LCD_CONFIG_USER_DEFINED == mode)
        /* Any error was reported earlier. */
        rc = ipmi_lcd_set_text(intf, text, line_number);


    if (rc == 0)

        rc = ipmi_lcd_set_configure_command_wh (intf, mode ,lcdqualifier,errordisp);

    return rc;
}


/*****************************************************************
* Function Name:   ipmi_lcd_configure
*
* Description:     This function updates the current lcd configuration
* Input:           intf            - ipmi interface
*                  command- lcd command
*                  line_number-line number
*                  text            - lcd string
* Output:   
* Return:          -1 on error
*                  0 if successful
*
******************************************************************/

static int
ipmi_lcd_configure (void * intf, int command, 
                    int8_t line_number, char * text)
{
    int rc = 0;

    if (IPMI_DELL_LCD_CONFIG_USER_DEFINED == command)
        rc = ipmi_lcd_set_text(intf, text, line_number);

    if (rc == 0)
        rc = ipmi_lcd_set_configure_command (intf, command);

    return rc;
}


/*****************************************************************
* Function Name:   ipmi_lcd_usage
*
* Description:   This function prints help message for lcd command
* Input:               
* Output:       
*
* Return:              
*
******************************************************************/

static void
ipmi_lcd_usage(void)
{
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "Generic DELL HW:");
    lprintf(LOG_NOTICE, "   lcd set {none}|{default}|{custom <text>}");
    lprintf(LOG_NOTICE, "      Set LCD text displayed during non-fault conditions");

    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "iDRAC 11g or iDRAC 12g:");
    lprintf(LOG_NOTICE, "   lcd set {mode}|{lcdqualifier}|{errordisplay}");
    lprintf(LOG_NOTICE, "      Allows you to set the LCD mode and user-definedstring.");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   lcd set mode {none}|{modelname}|{ipv4address}|{macaddress}|");
    lprintf(LOG_NOTICE, "   {systemname}|{servicetag}|{ipv6address}|{ambienttemp}");
    lprintf(LOG_NOTICE, "   {systemwatt }|{assettag}|{userdefined}<text>");
    lprintf(LOG_NOTICE, "         Allows you to set the LCD display mode to any of the preceding parameters");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   lcd set lcdqualifier {watt}|{btuphr}|{celsius}|{fahrenheit}");
    lprintf(LOG_NOTICE, "      Allows you to set the unit for the system ambient temperature mode.");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   lcd set errordisplay {sel}|{simple}");
    lprintf(LOG_NOTICE, "      Allows you to set the error display.");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   lcd info");
    lprintf(LOG_NOTICE, "      Show LCD text that is displayed during non-fault conditions");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   lcd set vkvm{active}|{inactive}");
    lprintf(LOG_NOTICE, "           Set vKVM active and inactive, message will be displayed on lcd"); 
    lprintf(LOG_NOTICE, " when vKVM is active and vKVM session is in progress");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   lcd set frontpanelaccess {viewandmodify}|{viewonly}|{disabled}");
    lprintf(LOG_NOTICE, "      Set LCD mode to view and modify, view only or disabled ");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   lcd status");
    lprintf(LOG_NOTICE, "           Show LCD Status for vKVM display<active|inactive>");
    lprintf(LOG_NOTICE, "   and Front Panel access mode {viewandmodify}|{viewonly}|{disabled} ");
    lprintf(LOG_NOTICE, "");
}

/*****************************************************************
* Function Name:       ipmi_delloem_mac_main
*
* Description:         This function processes the delloem mac command
* Input:               intf    - ipmi interface
                       argc    - no of arguments
                       argv    - argument string array
* Output:        
*
* Return:              return code     0 - success
*                         -1 - failure
*
******************************************************************/


static int ipmi_delloem_mac_main (void * intf, int  argc, char ** argv)
{
    int rc = 0;

    current_arg++;
    if (argc > 1 && strcmp(argv[current_arg], "help") == 0)
    {
        ipmi_mac_usage();
        return 0;
    }
    ipmi_idracvalidator_command(intf);
    if (argc == 1) /*( || (strncmp(argv[current_arg], "list\0", 5) == 0) )*/ 
    {
        rc = ipmi_macinfo(intf, 0xff);
    }
    else if (strncmp(argv[current_arg], "get\0", 4) == 0)
    {
        int currIdInt;
        current_arg++;
        if (argv[current_arg] == NULL)
        {
            ipmi_mac_usage();
            return -1;
        }
        if(make_int(argv[current_arg],&currIdInt) < 0) {
            lprintf(LOG_ERR, "Invalid NIC number. The NIC number should be between 0-8\n");
            return -1;
        }
        if( (currIdInt > 8) || (currIdInt < 0) )
        {
            lprintf(LOG_ERR, "Invalid NIC number. The NIC number should be between 0-8\n");
            return -1;
        }
        rc = ipmi_macinfo(intf, currIdInt);
    }
    else
    {
        ipmi_mac_usage();
    }
    return(rc);
}


/*****************************************************************
* Function Name:     make_int
*
* Description:   This function convert string into integer
* Input:         str     - decimal number string
* Output:        value   - integer value
* Return:                
*
******************************************************************/
static int make_int(const char *str, int *value)
{
    char *tmp=NULL;
    *value = (int)strtol(str,&tmp,0);
    if ( tmp-str != strlen(str) )
    {
        return -1;
    }
    return 0;
}





EmbeddedNICMacAddressType EmbeddedNICMacAddress;

EmbeddedNICMacAddressType_10G EmbeddedNICMacAddress_10G;

static void InitEmbeddedNICMacAddressValues ()
{
    uint8_t i;
    uint8_t j;


    for (i=0;i<MAX_LOM;i++)
    {
#ifdef LOM_OLD
        EmbeddedNICMacAddress.LOMMacAddress[i].BladSlotNumber = 0;
        EmbeddedNICMacAddress.LOMMacAddress[i].MacType = LOM_MACTYPE_RESERVED;
        EmbeddedNICMacAddress.LOMMacAddress[i].EthernetStatus = LOM_ETHERNET_RESERVED;
        EmbeddedNICMacAddress.LOMMacAddress[i].NICNumber = 0;
        EmbeddedNICMacAddress.LOMMacAddress[i].Reserved = 0;
#else
	EmbeddedNICMacAddress.LOMMacAddress[i].b0 = 0xF0;
	EmbeddedNICMacAddress.LOMMacAddress[i].b1 = 0x00;
#endif
        for (j=0;j<MACADDRESSLENGH;j++)
        {
            EmbeddedNICMacAddress.LOMMacAddress[i].MacAddressByte[j] = 0;
            EmbeddedNICMacAddress_10G.MacAddress[i].MacAddressByte[j] = 0;
        }
    }
}

uint8_t UseVirtualMacAddress = 0;
#define VIRTUAL_MAC_OFFSET (2)
static int ipmi_macinfo_drac_idrac_virtual_mac(void* intf,uint8_t NicNum)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;

    uint8_t msg_data[30];
    uint8_t VirtualMacAddress [MACADDRESSLENGH];
    uint8_t input_length=0;
    uint8_t j;
    //uint8_t length;
    uint8_t i;


    if (0xff==NicNum || IDRAC_NIC_NUMBER==NicNum )
    {
        UseVirtualMacAddress = 0;

        input_length = 0;
        msg_data[input_length++] = 1; /*Get*/

      	req.msg.netfn = IPMI_DELL_OEM_NETFN;
        req.msg.lun = 0;                
        req.msg.cmd = GET_IDRAC_VIRTUAL_MAC;
        req.msg.data = msg_data;
        req.msg.data_len = input_length;

        rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) { return rv; }

        if( (IMC_IDRAC_12G_MODULAR == IMC_Type) || (IMC_IDRAC_12G_MONOLITHIC== IMC_Type) ) {
           // Get the Chasiss Assigned MAC Addresss        for 12g Only
           memcpy(VirtualMacAddress,&rsp[1],MACADDRESSLENGH);

           for (i=0;i<MACADDRESSLENGH;i++)
           {
              if (0 != VirtualMacAddress [i])
              {
                 UseVirtualMacAddress = 1;
              }
           }
           // Get the Server Assigned MAC Addresss for 12g Only
           if(!UseVirtualMacAddress) {
              memcpy(VirtualMacAddress,&rsp[1+MACADDRESSLENGH],MACADDRESSLENGH);

              for (i=0;i<MACADDRESSLENGH;i++)
              {
                 if (0 != VirtualMacAddress [i])
                 {
                    UseVirtualMacAddress = 1;
                 }
              }
           }
        } else {
           memcpy(VirtualMacAddress,&rsp[VIRTUAL_MAC_OFFSET],MACADDRESSLENGH);

           for (i=0;i<MACADDRESSLENGH;i++)
           {
              if (0 != VirtualMacAddress [i])
              {
                UseVirtualMacAddress = 1;
              }       
           }
	}
        if (0 == UseVirtualMacAddress)
            return -1;              
        if (IMC_IDRAC_10G == IMC_Type)
            printf ("\nDRAC MAC Address ");
        else if ( (IMC_IDRAC_11G_MODULAR == IMC_Type) || (IMC_IDRAC_11G_MONOLITHIC== IMC_Type) )
            printf ("\niDRAC6 MAC Address ");
        else if ( (IMC_IDRAC_12G_MODULAR == IMC_Type) || (IMC_IDRAC_12G_MONOLITHIC== IMC_Type) )
            printf ("\niDRAC7 MAC Address ");

        for (j=0;j<5;j++)
            printf("%02x:",VirtualMacAddress[j]);
        printf("%02x",VirtualMacAddress[j]); /*5*/

        printf ("\n\r");        

    }  
    return 0;
}


/*****************************************************************
* Function Name:    ipmi_macinfo_drac_idrac_mac
*
* Description:      This function retrieves the mac address of DRAC or iDRAC
* Input:            NicNum
* Output:                 
* Return:               
*
******************************************************************/

static int ipmi_macinfo_drac_idrac_mac(void* intf,uint8_t NicNum)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;

    uint8_t msg_data[30];
    uint8_t input_length=0;
    uint8_t iDRAC6MacAddressByte[MACADDRESSLENGH];
    uint8_t j;

    ipmi_macinfo_drac_idrac_virtual_mac (intf,NicNum);


    if ((0xff==NicNum || IDRAC_NIC_NUMBER==NicNum) && 0 == UseVirtualMacAddress)
    {

        input_length = 0;

        msg_data[input_length++] = LAN_CHANNEL_NUMBER; 
        msg_data[input_length++] = MAC_ADDR_PARAM;  
        msg_data[input_length++] = 0x00;                        
        msg_data[input_length++] = 0x00;                        

        req.msg.netfn = TRANSPORT_NETFN;
        req.msg.lun = 0;                
        req.msg.cmd = GET_LAN_PARAM_CMD;
        req.msg.data = msg_data;
        req.msg.data_len = input_length;

        rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
            printf(" Error in getting MAC Address: ");
	    if (rv < 0) printf("no response\n");
	    else printf("Completion Code 0x%02x\n", rv);
	    return rv;
        }

        memcpy(iDRAC6MacAddressByte,&rsp[PARAM_REV_OFFSET],MACADDRESSLENGH);

        if (IMC_IDRAC_10G == IMC_Type)
            printf ("\n\rDRAC MAC Address ");
		else if ((IMC_IDRAC_11G_MODULAR == IMC_Type) || (IMC_IDRAC_11G_MONOLITHIC== IMC_Type))
			printf ("\n\riDRAC6 MAC Address ");
		else if ((IMC_IDRAC_12G_MODULAR == IMC_Type) || (IMC_IDRAC_12G_MONOLITHIC== IMC_Type))		
			printf ("\n\riDRAC7 MAC Address ");
        else
            printf ("\n\riDRAC6 MAC Address ");

        for (j=0;j<5;j++)
            printf("%02x:",iDRAC6MacAddressByte[j]);
        printf("%02x",iDRAC6MacAddressByte[j]);

        printf ("\n\r");        
    }
    return 0;
}


/*****************************************************************
* Function Name:    ipmi_macinfo_10g
*
* Description:      This function retrieves the mac address of LOMs
* Input:            intf      - ipmi interface
                    NicNum    - NIC number
* Output:               
* Return:               
*
******************************************************************/

static int ipmi_macinfo_10g (void* intf, uint8_t NicNum)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;

    uint8_t msg_data[30];
    uint8_t input_length=0;

    uint8_t j;
    uint8_t i;

    uint8_t Total_No_NICs = 0;


    InitEmbeddedNICMacAddressValues ();

    memset(msg_data, 0, sizeof(msg_data));
    input_length = 0;
    msg_data[input_length++] = 0x00; /* Get Parameter Command */
    msg_data[input_length++] = EMB_NIC_MAC_ADDRESS_9G_10G;  /* OEM Param */

    msg_data[input_length++] = 0x00;         
    msg_data[input_length++] = 0x00;         

    memset(&req, 0, sizeof(req));

    req.msg.netfn = APP_NETFN;
    req.msg.lun = 0;                
    req.msg.cmd = GET_SYSTEM_INFO_CMD;
    req.msg.data = msg_data;


    req.msg.data_len = input_length;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
            printf(" Error in getting MAC Address: ");
	    if (rv < 0) printf("no response\n");
	    else printf("Completion Code 0x%02x\n", rv);
	    return rv;
    }
    if (fdebug) dump_buf("GetMacResp_10G",rsp,rsp_len,0);

    Total_No_NICs = (uint8_t) rsp[PARAM_REV_OFFSET]; /* Byte 1: Total Number of Embedded NICs */

    if (IDRAC_NIC_NUMBER != NicNum)
    {
        if (0xff == NicNum)
        {
            printf ("\n\rSystem LOMs");
        }       
        printf("\n\rNIC Number\tMAC Address\n\r");

        memcpy(&EmbeddedNICMacAddress_10G,&rsp[PARAM_REV_OFFSET+TOTAL_N0_NICS_INDEX],Total_No_NICs* MACADDRESSLENGH);


        /*Read the LOM type and Mac Addresses */

        for (i=0;i<Total_No_NICs;i++)
        {
            if ((0xff==NicNum) || (i == NicNum)     )
            {       
                printf ("\n\r%d",i);
                printf ("\t\t");
                for (j=0;j<5;j++)
                {
                    printf("%02x:",EmbeddedNICMacAddress_10G.MacAddress[i].MacAddressByte[j]);
                }       
                printf("%02x",EmbeddedNICMacAddress_10G.MacAddress[i].MacAddressByte[j]);
            }               
        }
        printf ("\n\r");

    }

    ipmi_macinfo_drac_idrac_mac(intf,NicNum);


    return 0;
}


/*****************************************************************
* Function Name:      ipmi_macinfo_11g
*
* Description:        This function retrieves the mac address of LOMs
* Input:              intf - ipmi interface
* Output:               
* Return:               
*
******************************************************************/

static int ipmi_macinfo_11g (void* intf, uint8_t NicNum)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t msg_data[30];
    uint8_t input_length=0;
    uint8_t len;
    uint8_t j;
    uint8_t offset;
    uint8_t maxlen;
    uint8_t loop_count;
    uint8_t i;
    uint8_t lom_mactype;
    uint8_t lom_nicnum;
    uint8_t lom_ethstat;
    uint8_t *lom_mac;
    // uint8_t LOMStatus = 0;
    // uint8_t PlayingDead = 0;

    offset = 0;
    len = 8; /*eigher 8 or 16 */
    maxlen = 64;
    loop_count = maxlen / len;

    InitEmbeddedNICMacAddressValues ();

    memset(msg_data, 0, sizeof(msg_data));
    input_length = 0;
    msg_data[input_length++] = 0x00; /* Get Parameter Command */
    msg_data[input_length++] = EMB_NIC_MAC_ADDRESS_11G;      /* OEM Param */

    msg_data[input_length++] = 0x00;      
    msg_data[input_length++] = 0x00;      
    msg_data[input_length++] = 0x00;      
    msg_data[input_length++] = 0x00;      

    memset(&req, 0, sizeof(req));

    req.msg.netfn = APP_NETFN;
    req.msg.lun = 0;              
    req.msg.cmd = GET_SYSTEM_INFO_CMD;
    req.msg.data = msg_data;


    req.msg.data_len = input_length;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
            printf(" Error in getting MAC Address: ");
	    if (rv < 0) printf("no response\n");
	    else printf("Completion Code 0x%02x\n", rv);
	    return rv;
    }

    len = 8; /*eigher 8 or 16 */
    maxlen = (uint8_t) rsp[0+PARAM_REV_OFFSET];
    loop_count = maxlen / len;

    if (IDRAC_NIC_NUMBER != NicNum)
    {
        if (0xff == NicNum)
        {
            printf ("\n\rSystem LOMs");
        }       
        printf("\n\rNIC Number\tMAC Address\t\tStatus\n\r");


        /*Read the LOM type and Mac Addresses */
        offset=0;
        for (i=0;i<loop_count;i++,offset=offset+len)
        {
            input_length = 4;
            msg_data[input_length++] = offset;
            msg_data[input_length++] = len;   

            req.msg.netfn = APP_NETFN;
            req.msg.lun = 0;
            req.msg.cmd = GET_SYSTEM_INFO_CMD;
            req.msg.data = msg_data;
            req.msg.data_len = input_length;

            rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	    if (rv) {
		printf(" Error in getting MAC Address: ");
		if (rv < 0) printf("no response\n");
		else printf("Completion Code 0x%02x\n", rv);
		return rv;
	    }
	    if (fdebug) {
		printf("ipmi_macinfo_11g(%d) i=%d offset=%d\n",NicNum,i,offset);
		dump_buf("GetMacResp",rsp,rsp_len,0);
	    }

            memcpy(&(EmbeddedNICMacAddress.LOMMacAddress[i]),&rsp[PARAM_REV_OFFSET],len);

#ifdef LOM_OLD
	    lom_ethstat = EmbeddedNICMacAddress.LOMMacAddress[i].EthernetStatus;
	    lom_mactype = EmbeddedNICMacAddress.LOMMacAddress[i].MacType;
	    lom_nicnum  = EmbeddedNICMacAddress.LOMMacAddress[i].NICNumber;
	    lom_mac     = &EmbeddedNICMacAddress.LOMMacAddress[i].MacAddressByte[0];
#else
	    lom_ethstat = ((EmbeddedNICMacAddress.LOMMacAddress[i].b0 & 0xc0) >> 6);
	    lom_mactype = ((EmbeddedNICMacAddress.LOMMacAddress[i].b0 & 0x30) >> 4);
	    /* lom_bladslot = (b0 & 0x0f); */
	    lom_nicnum  = (EmbeddedNICMacAddress.LOMMacAddress[i].b1 & 0x1f);
	    lom_mac     = &EmbeddedNICMacAddress.LOMMacAddress[i].MacAddressByte[0];
	    if (fdebug) {
		printf("\n\rlom_ethstat=%x lom_mactype=%x lom_nicnum=%x\n",
			lom_ethstat,lom_mactype,lom_nicnum);
		printf("MacAdrB=%02x:%02x:%02x:%02x:%02x:%02x\n",
		     lom_mac[0], lom_mac[1], lom_mac[2],
		     lom_mac[3], lom_mac[4], lom_mac[5]);
		printf("\n\rrsp_mac=%02x:%02x:%02x:%02x:%02x:%02x\n",
		     rsp[3], rsp[4], rsp[5], rsp[6], rsp[7], rsp[8]);
	    }
	    lom_mac     = &rsp[3];
#endif

            if (LOM_MACTYPE_ETHERNET == lom_mactype)
            {

                if (  (0xff==NicNum) || (NicNum == lom_nicnum)  )
                {
                    printf ("\n\r%d",lom_nicnum);
                    printf ("\t\t");
                    for (j=0;j<5;j++)
                        printf("%02x:",lom_mac[j]);
                    printf("%02x",lom_mac[j]);

                    if (LOM_ETHERNET_ENABLED == lom_ethstat)
                        printf ("\tEnabled");
                    else
                        printf ("\tDisabled");
                }
            }

        }         
        printf ("\n\r");

    }

    ipmi_macinfo_drac_idrac_mac(intf,NicNum);

    return 0;

}     



/*****************************************************************
* Function Name:      ipmi_macinfo
*
* Description:     This function retrieves the mac address of LOMs
* Input:           intf   - ipmi interface
* Output:               
* Return:               
*
******************************************************************/

static int ipmi_macinfo (void* intf, uint8_t NicNum)
{
    if (IMC_IDRAC_10G == IMC_Type)
    {
        return ipmi_macinfo_10g (intf,NicNum);
    }
	else if ((IMC_IDRAC_11G_MODULAR == IMC_Type || IMC_IDRAC_11G_MONOLITHIC== IMC_Type )  ||
			(IMC_IDRAC_12G_MODULAR == IMC_Type || IMC_IDRAC_12G_MONOLITHIC== IMC_Type ) )
    {
        return ipmi_macinfo_11g (intf,NicNum);
    }
    else
    {
        lprintf(LOG_ERR, " Error in getting MAC Address : Not supported platform");
        return 0;
    }       
}


/*****************************************************************
* Function Name:     ipmi_mac_usage
*
* Description:   This function prints help message for mac command
* Input:               
* Output:       
*
* Return:              
*
******************************************************************/

static void
ipmi_mac_usage(void)
{
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   mac list");
    lprintf(LOG_NOTICE, "      Lists the MAC address of LOMs");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   mac get <NIC number>");
    lprintf(LOG_NOTICE, "      Shows the MAC address of specified LOM. 0-7 System LOM, 8- DRAC/iDRAC.");
    lprintf(LOG_NOTICE, "");
}

/*****************************************************************
* Function Name:       ipmi_delloem_lan_main
*
* Description:         This function processes the delloem lan command
* Input:               intf    - ipmi interface
                       argc    - no of arguments
                       argv    - argument string array
* Output:        
*
* Return:              return code     0 - success
*                         -1 - failure
*
******************************************************************/

static int ipmi_delloem_lan_main (void * intf, int  argc, char ** argv)
{
    int rc = 0;

    int nic_selection = 0;
    char nic_set[2] = {0};
    current_arg++;
    if (argv[current_arg] == NULL || strcmp(argv[current_arg], "help") == 0)
    {
        ipmi_lan_usage();
        return 0;
    }
    ipmi_idracvalidator_command(intf);
    if (!IsLANSupported())
    {
        printf("lan is not supported on this system.\n");
        return -1;
    }               
    else if (strncmp(argv[current_arg], "set\0", 4) == 0)
    {
        current_arg++;
        if (argv[current_arg] == NULL)
        {
            ipmi_lan_usage();
            return -1;
        }
		if(iDRAC_FLAG == IDRAC_12G) {
			nic_selection = get_nic_selection_mode_12g(intf,current_arg,argv,nic_set);
			if (INVALID == nic_selection)
			{
				ipmi_lan_usage();
				return -1;
                        } else if(INVAILD_FAILOVER_MODE == nic_selection) {
                                printf(INVAILD_FAILOVER_MODE_STRING);
                                return 0;
                        } else if(INVAILD_FAILOVER_MODE_SETTINGS == nic_selection){
                                printf(INVAILD_FAILOVER_MODE_SET);
                                return 0;
                        } else if(INVAILD_SHARED_MODE == nic_selection){
                                printf(INVAILD_SHARED_MODE_SET_STRING);
                                return 0;
			}				
		
			rc = ipmi_lan_set_nic_selection_12g(intf,nic_set);
		}
		else
		{
        	  nic_selection = get_nic_selection_mode(current_arg,argv);

        	  if (INVALID == nic_selection)
        	  {
        	    ipmi_lan_usage();
        	    return -1;
        	  }
        	  if(IMC_IDRAC_11G_MODULAR == IMC_Type) {
        	    printf(INVAILD_SHARED_MODE_SET_STRING);
        	    return 0;
        	  }
        	  rc = ipmi_lan_set_nic_selection(intf,nic_selection);
		}		
        return 0;                       
    }
    else if (strncmp(argv[current_arg], "get\0", 4) == 0)
    {
        current_arg++;
        if (argv[current_arg] == NULL)
        {
            rc = ipmi_lan_get_nic_selection(intf);
            return rc;
        }
        else if (strncmp(argv[current_arg], "active\0", 7) == 0)                
        {
            rc = ipmi_lan_get_active_nic(intf);
            return rc;
        }
        else
        {
            ipmi_lan_usage();
        }

    }
    else
    {
        ipmi_lan_usage();
        return -1;
    }
    return(rc);
}


static int IsLANSupported ()
{
    if (IMC_IDRAC_11G_MODULAR == IMC_Type)
        return 0;
    return 1;
}


int get_nic_selection_mode_12g (void* intf,int current_arg, char ** argv, char *nic_set)
{
	int failover = 0;

	// First get the current settings.
	uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
	struct ipmi_rq req;

	uint8_t msg_data[30];
	uint8_t input_length=0;
	
	input_length = 0;
		
   	req.msg.netfn = IPMI_DELL_OEM_NETFN;
   	req.msg.lun = 0;		
	
  	req.msg.cmd = GET_NIC_SELECTION_12G_CMD;

  	req.msg.data = msg_data;
  	req.msg.data_len = input_length;
  
	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
            printf(" Error in getting NIC selection: ");
	    if (rv < 0) printf("no response\n");
	    else printf("Completion Code 0x%02x %s\n",
				rv,decode_cc(0,rv));
	    return rv;
	}
	
	nic_set[0] = rsp[0];
	nic_set[1] = rsp[1];

	
	if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "dedicated\0", 10)) 
	{
		nic_set[0] = 1;
		nic_set[1] = 0;
		return 0;
	}
	if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "shared\0", 7)) 
	{
		
	}
	else
		return INVALID;
	
	current_arg++;	
	if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "with\0", 5)) 
	{
	}	
	else
		return INVALID;		
	
	current_arg++;	
	if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "failover\0", 9)) 
	{
		failover = 1;
	}	
	if(failover)
		current_arg++;	
	if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "lom1\0", 5)) 
	{
                if(IMC_IDRAC_12G_MODULAR == IMC_Type)
                {
                        return INVAILD_SHARED_MODE;
                }
		if(failover) {
                        if(nic_set[0] == 2)
                        {
                                return INVAILD_FAILOVER_MODE;
                        } else if(nic_set[0] == 1) {
                                return INVAILD_FAILOVER_MODE_SETTINGS;
                        }
			nic_set[1] = 2;
		}	
		else {
			nic_set[0] = 2;
                        if(nic_set[1] == 2)
                                nic_set[1] = 0;
		}	
		return 0;
	}
	else if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "lom2\0", 5)) 
	{
                if(IMC_IDRAC_12G_MODULAR == IMC_Type)
                {
                        return INVAILD_SHARED_MODE;
                }
		if(failover) {		
			if(nic_set[0] == 3)
                        {
                                return INVAILD_FAILOVER_MODE;
                        } else if(nic_set[0] == 1) {
                                return INVAILD_FAILOVER_MODE_SETTINGS;
                        }
			nic_set[1] = 3;
		}	
		else {
			nic_set[0] = 3;
			if(nic_set[1] == 3)
			        nic_set[1] = 0;

		}	
		return 0;
	}
	else if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "lom3\0", 5)) 
	{
                if(IMC_IDRAC_12G_MODULAR == IMC_Type)
                {
                        return INVAILD_SHARED_MODE;
                }
		if(failover) {	
                        if(nic_set[0] == 4)
                        {
                                return INVAILD_FAILOVER_MODE;
                        } else if(nic_set[0] == 1) {
                                return INVAILD_FAILOVER_MODE_SETTINGS;
                        }
			nic_set[1] = 4;
		}	
		else {
			nic_set[0] = 4;
                        if(nic_set[1] == 4)
                                nic_set[1] = 0;
 	}	
		return 0;
	} 
	else if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "lom4\0", 5)) 
	{
                if(IMC_IDRAC_12G_MODULAR == IMC_Type)
                {
                        return INVAILD_SHARED_MODE;
                }
		if(failover) {	
                        if(nic_set[0] == 5)
                        {
                                return INVAILD_FAILOVER_MODE;
                        } else if(nic_set[0] == 1) {
                                return INVAILD_FAILOVER_MODE_SETTINGS;
                        }
			nic_set[1] = 5;
		}	
		else {
			nic_set[0] = 5;
                        if(nic_set[1] == 5)
                                nic_set[1] = 0;
		}	
		return 0;
	}	
	else if (failover && NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "none\0", 5)) 
	{
		if(IMC_IDRAC_12G_MODULAR == IMC_Type)
		{
		        return INVAILD_SHARED_MODE;
		}
		if(failover) {
			if(nic_set[0] == 1) {
				return INVAILD_FAILOVER_MODE_SETTINGS;
			}
			nic_set[1] = 0;
		}	
		return 0;
	}	
	else if (failover && NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "all\0", 4)) 
	{
	}	
	else
		return INVALID;	
	
	current_arg++;	
	if (failover && NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "loms\0", 5)) 
	{
		if(IMC_IDRAC_12G_MODULAR == IMC_Type)
		{
			return INVAILD_SHARED_MODE;
		}
		if(nic_set[0] == 1) {
			return INVAILD_FAILOVER_MODE_SETTINGS;
		}
		nic_set[1] = 6;
		return 0;
	}	

	return INVALID;
	
}


static int get_nic_selection_mode (int current_arg, char ** argv)
{
    if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "dedicated\0", 10)) 
    {
        return DEDICATED;
    }
    if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "shared\0", 7)) 
    {
        if (NULL == argv[current_arg+1] )
            return SHARED;          
    }
    current_arg++;  
    if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "with\0", 5)) 
    {
    }       
    else
        return INVALID;         

    current_arg++;  
    if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "failover\0", 9)) 
    {
    }       
    else
        return INVALID;         

    current_arg++;  
    if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "lom2\0", 5)) 
    {
        return SHARED_WITH_FAILOVER_LOM2;
    }       
    else if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "all\0", 4)) 
    {
    }       
    else
        return INVALID; 

    current_arg++;  
    if (NULL!= argv[current_arg] && 0 == strncmp(argv[current_arg], "loms\0", 5)) 
    {
        return SHARED_WITH_FAILOVER_ALL_LOMS;
    }       

    return INVALID;
	
}


static int ipmi_lan_set_nic_selection_12g (void* intf, uint8_t* nic_selection)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
	struct ipmi_rq req;

	uint8_t msg_data[30];
	uint8_t input_length=0;

	input_length = 0;
		
	msg_data[input_length++] = nic_selection[0]; 
	msg_data[input_length++] = nic_selection[1]; 

	req.msg.netfn = IPMI_DELL_OEM_NETFN;
	req.msg.lun = 0;		
	req.msg.cmd = SET_NIC_SELECTION_12G_CMD;
	req.msg.data = msg_data;
	req.msg.data_len = input_length;
  
	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
            printf(" Error in setting NIC selection: ");
	    if (rv < 0) printf("no response\n");
	    else printf("Completion Code 0x%02x %s\n",
				rv,decode_cc(0,rv));
	    return rv;
	}
	printf("configured successfully");

	return 0;
}


static int ipmi_lan_set_nic_selection (void* intf, uint8_t nic_selection)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;

    uint8_t msg_data[30];
    uint8_t input_length=0;
    //uint8_t j;

    input_length = 0;

    msg_data[input_length++] = nic_selection; 

   	req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;                
    req.msg.cmd = SET_NIC_SELECTION_CMD;
    req.msg.data = msg_data;
    req.msg.data_len = input_length;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
            printf(" Error in setting NIC selection: ");
	    if (rv < 0) printf("no response\n");
	    else printf("Completion Code 0x%02x %s\n",
				rv,decode_cc(0,rv));
	    return rv;
    }
    printf("configured successfully");

    return 0;
}      

static int ipmi_lan_get_nic_selection (void* intf)
{
    uint8_t nic_selection=-1;
	uint8_t nic_selection_failover = 0;

    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;

    uint8_t msg_data[30];
    uint8_t input_length=0;
    //uint8_t j;

    input_length = 0;

    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;                
    if(iDRAC_FLAG == IDRAC_12G)
  	req.msg.cmd = GET_NIC_SELECTION_12G_CMD;
    else  	
        req.msg.cmd = GET_NIC_SELECTION_CMD;
    req.msg.data = msg_data;
    req.msg.data_len = input_length;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
            printf(" Error in getting NIC selection: ");
	    if (rv < 0) printf("no response\n");
	    else printf("Completion Code 0x%02x %s\n",
				rv,decode_cc(0,rv));
	    return rv;
    }
    nic_selection = rsp[0];

	if(iDRAC_FLAG == IDRAC_12G)
	{		
		
		nic_selection_failover = rsp[1];
		if ((nic_selection < 6) && (nic_selection > 0) && (nic_selection_failover < 7))
		{
			if(nic_selection == 1) {
				printf ("%s\n",NIC_Selection_Mode_String_12g[nic_selection-1]);
			} else if(nic_selection) {
				printf ("Shared LOM   :  %s\n",NIC_Selection_Mode_String_12g[nic_selection-1]);
				if(nic_selection_failover  == 0)
					printf ("Failover LOM :  None\n");
				else if(nic_selection_failover   >= 2 && nic_selection_failover   <= 6)
					printf ("Failover LOM :  %s\n",NIC_Selection_Mode_String_12g[nic_selection_failover + 3]);
			}
				
		} 
		else
		{
			lprintf(LOG_ERR, " Error Outof bond Value received (%d) (%d) \n",nic_selection,nic_selection_failover);
		  	return -1;	  	
		}
	}
	else
	{
		printf ("%s\n",NIC_Selection_Mode_String[nic_selection]);
	}

    return 0;
}      

static int ipmi_lan_get_active_nic (void* intf)
{
    uint8_t active_nic=0;
    uint8_t current_lom =0;

    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;

    uint8_t msg_data[30];
    uint8_t input_length=0;

    input_length = 0;

    msg_data[input_length++] = 0; /*Get current LOM*/
    msg_data[input_length++] = 0; /*Reserved*/
    msg_data[input_length++] = 0; /*Reserved*/        

    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;                
    req.msg.cmd = GET_ACTIVE_NIC_CMD;
    req.msg.data = msg_data;
    req.msg.data_len = input_length;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
            printf(" Error in getting Current LOM: ");
	    if (rv < 0) printf("no response\n");
	    else printf("Completion Code 0x%02x %s\n",
				rv,decode_cc(0,rv));
	    return rv;
    }
    current_lom = rsp[0];

    input_length = 0;

    msg_data[input_length++] = 1; //Get Link status
    msg_data[input_length++] = 0; //Reserved
    msg_data[input_length++] = 0; //Reserved

    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;
    req.msg.cmd = GET_ACTIVE_NIC_CMD;
    req.msg.data = msg_data;
    req.msg.data_len = input_length;
    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
            printf(" Error in getting Active LOM Status: ");
	    if (rv < 0) printf("no response\n");
	    else printf("Completion Code 0x%02x %s\n",
				rv,decode_cc(0,rv));
	    return rv;
    }

    active_nic = rsp[1];
    if (current_lom < 5 && active_nic)
        printf ("\n%s\n",ActiveLOM_String[current_lom]);
    else
        printf ("\n%s\n",ActiveLOM_String[5]);

    return 0;
}


static void
ipmi_lan_usage(void)
{
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   lan set <Mode> ");
    lprintf(LOG_NOTICE, "      sets the NIC Selection Mode :");
    lprintf(LOG_NOTICE, "          on iDRAC12g :");

    lprintf(LOG_NOTICE, "              dedicated, shared with lom1, shared with lom2,shared with lom3,shared ");
    lprintf(LOG_NOTICE, "              with lom4,shared with failover lom1,shared with failover lom2,shared ");
    lprintf(LOG_NOTICE, "              with failover lom3,shared with failoverlom4,shared with Failover all ");
    lprintf(LOG_NOTICE, "              loms, shared with Failover None).");
    lprintf(LOG_NOTICE, "          on other systems :");
    lprintf(LOG_NOTICE, "              dedicated, shared, shared with failoverlom2,");
    lprintf(LOG_NOTICE, "              shared with Failover all loms.");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   lan get ");
    lprintf(LOG_NOTICE, "          on iDRAC12g :");
    lprintf(LOG_NOTICE, "              returns the current NIC Selection Mode (dedicated, shared with lom1, shared ");
    lprintf(LOG_NOTICE, "              with lom2, shared with lom3, shared with lom4,shared with failover lom1,");
    lprintf(LOG_NOTICE, "              shared with failover lom2,shared with failover lom3,shared with failover ");
    lprintf(LOG_NOTICE, "              lom4,shared with Failover all loms,shared with Failover None).");
    lprintf(LOG_NOTICE, "          on other systems :");
    lprintf(LOG_NOTICE, "              dedicated, shared, shared with failover,");
    lprintf(LOG_NOTICE, "              lom2, shared with Failover all loms.");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   lan get active");
    lprintf(LOG_NOTICE, "      Get the current active LOMs (LOM1, LOM2, LOM3, LOM4, NONE).");
    lprintf(LOG_NOTICE, "");

}

/*****************************************************************
* Function Name:       ipmi_delloem_powermonitor_main
*
* Description:         This function processes the delloem powermonitor command
* Input:               intf    - ipmi interface
                       argc    - no of arguments
                       argv    - argument string array
* Output:        
*
* Return:              return code     0 - success
*                         -1 - failure
*
******************************************************************/

static int ipmi_delloem_powermonitor_main (void * intf, int  argc, char ** argv)
{
    int rc = 0;

    current_arg++;
    if (argc > 1 && strcmp(argv[current_arg], "help") == 0)
    {
        ipmi_powermonitor_usage();
        return 0;
    }
    ipmi_idracvalidator_command(intf);
    if (argc == 1)
    {
        rc = ipmi_powermgmt(intf);
    }
    else if (strncmp(argv[current_arg], "status\0", 7) == 0) 
    {
        rc = ipmi_powermgmt(intf);
    }

    else if (strncmp(argv[current_arg], "clear\0", 6) == 0) 
    {
        current_arg++;
        if (argv[current_arg] == NULL) 
        {
            ipmi_powermonitor_usage();
            return -1;
        }
        else if (strncmp(argv[current_arg], "peakpower\0", 10) == 0) 
        {
            rc = ipmi_powermgmt_clear(intf, 1);
        }
        else if (strncmp(argv[current_arg], "cumulativepower\0", 16) == 0) 
        {
            rc = ipmi_powermgmt_clear(intf, 0);
        }
        else 
        {
            ipmi_powermonitor_usage();
            return -1;
        }

    }


    else if (strncmp(argv[current_arg], "powerconsumption\0", 17) == 0) 
    {
        current_arg++;

        if (argv[current_arg] == NULL)
        {

            rc=ipmi_print_get_power_consmpt_data(intf,watt);

        }
        else if (strncmp(argv[current_arg], "watt\0", 5) == 0) 
        {

            rc = ipmi_print_get_power_consmpt_data(intf, watt);
        }
        else if (strncmp(argv[current_arg], "btuphr\0", 7) == 0) 
        {
            rc = ipmi_print_get_power_consmpt_data(intf, btuphr);
        }
        else
        {
            ipmi_powermonitor_usage();
            return -1;
        }
    }
    else if (strncmp(argv[current_arg], "powerconsumptionhistory\0", 23) == 0) 
    {
        current_arg++;
        if (argv[current_arg] == NULL)
        {
            rc=ipmi_print_power_consmpt_history(intf,watt);

        }
        else if (strncmp(argv[current_arg], "watt\0", 5) == 0) 
        {
            rc = ipmi_print_power_consmpt_history(intf, watt);
        }
        else if (strncmp(argv[current_arg], "btuphr\0", 7) == 0)
        {
            rc = ipmi_print_power_consmpt_history(intf, btuphr);
        }
        else
        {
            ipmi_powermonitor_usage();
            return -1;
        }

    }

    else if (strncmp(argv[current_arg], "getpowerbudget\0", 15) == 0)
    {
        current_arg++;
        if (argv[current_arg] == NULL)
        {
            rc=ipmi_print_power_cap(intf,watt);

        }
        else if (strncmp(argv[current_arg], "watt\0", 5) == 0)
        {
            rc = ipmi_print_power_cap(intf, watt);
        }
        else if (strncmp(argv[current_arg], "btuphr\0", 7) == 0)
        {
            rc = ipmi_print_power_cap(intf, btuphr);
        }
        else
        {
            ipmi_powermonitor_usage();
            return -1;
        }

    }

    else if (strncmp(argv[current_arg], "setpowerbudget\0", 15) == 0)
    {
        int val;
        current_arg++;
        if (argv[current_arg] == NULL)
        { 
            ipmi_powermonitor_usage();
            return -1;
        }
		if (strchr(argv[current_arg], '.'))
		{
			lprintf(LOG_ERR, " Cap value in Watts, Btu/hr or percent should be whole number");
			return -1;
		}
        make_int(argv[current_arg],&val);
        current_arg++;
        if (argv[current_arg] == NULL)
        {       
            ipmi_powermonitor_usage();
        }
        else if (strncmp(argv[current_arg], "watt\0", 5) == 0)
        {
            rc=ipmi_set_power_cap(intf,watt,val);
        }
        else if (strncmp(argv[current_arg], "btuphr\0", 7) == 0)
        {
            rc=ipmi_set_power_cap(intf, btuphr,val);
        }
        else if (strncmp(argv[current_arg], "percent\0", 8) == 0)
        {
            rc=ipmi_set_power_cap(intf,percent,val);
        }
        else
        {
            ipmi_powermonitor_usage();
            return -1;
        }

    }

    else if (strncmp(argv[current_arg], "enablepowercap\0", 15) == 0)
    {
        rc = ipmi_set_power_capstatus_command(intf,1);
    }

    else if (strncmp(argv[current_arg], "disablepowercap\0", 16) == 0)
    {
        rc = ipmi_set_power_capstatus_command(intf,0);
    }
    else
    {
        ipmi_powermonitor_usage();
        return -1;
    }
    if (sdrcache != NULL) free_sdr_cache(sdrcache); 
    return(rc);
}


/*****************************************************************
* Function Name:     ipmi_time_to_str
*
* Description:       This function converts ipmi time format into gmtime format
* Input:             rawTime  - ipmi time format 
* Output:            strTime  - gmtime format
*
* Return:              
*
******************************************************************/

static void
ipmi_time_to_str(time_t rawTime, char* strTime)
{
    struct tm * tm;
    char *temp;
    tm = gmtime(&rawTime);

    temp = asctime(tm);

    strcpy(strTime,temp);
}

#ifdef NOT_USED
static int ipmi_get_sensor_reading(void *intf ,
            unsigned char sensorNumber, 
                        SensorReadingType* pSensorReadingData);
/*****************************************************************
* Function Name:      ipmi_get_sensor_reading
*
* Description:        This function retrieves a raw sensor reading
* Input:              sensorOwner       - sensor owner id
*                     sensorNumber      - sensor id
*                     intf              - ipmi interface
* Output:             sensorReadingData - ipmi response structure
* Return:             1 on error
*                     0 if successful
*
******************************************************************/
static int
ipmi_get_sensor_reading(void *intf ,
                unsigned char sensorNumber,
                        SensorReadingType* pSensorReadingData)
{
    struct ipmi_rq req;
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len;
    int rc = 0;
    // uint8_t save_addr;

    memset(&req, 0, sizeof (req));
    req.msg.netfn = IPMI_NETFN_SE;
    req.msg.lun = 0;
    req.msg.cmd = (uint8_t)(GET_SENSOR_READING | 0x0ff);
    req.msg.data = &sensorNumber;
    req.msg.data_len = 1;

    if (NULL == pSensorReadingData)
        return -1;
    memset(pSensorReadingData,0, sizeof(SensorReadingType));        

    rc = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rc) return 1; 

    memcpy(pSensorReadingData, rsp, sizeof(SensorReadingType));

    /* if sensor messages are disabled, return error*/
    if ((!(rsp[1]& 0xC0)) || ((rsp[1] & 0x20))) {
        rc =1;
    }
    return rc;
}
#endif


/*****************************************************************
* Function Name:   ipmi_get_power_capstatus_command
*
* Description:     This function gets the power cap status
* Input:           intf                 - ipmi interface
* Global:          PowercapSetable_flag - power cap status
* Output:                
*
* Return:              
*
******************************************************************/
static int
ipmi_get_power_capstatus_command (void * intf)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[2];

    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_DELL_POWER_CAP_STATUS;
    req.msg.data_len = 2;
    req.msg.data = data;
    data[0] = 01;
    data[1] = 0xFF;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
            printf(" Error getting powercap status: ");
	    if (rv < 0) printf("no response\n");
	    else printf("Completion Code 0x%02x %s\n",
				rv,decode_cc(0,rv));
	    return rv;
    }
    if (rsp[0]&0x02)
        PowercapSetable_flag=1;
    if(rsp[0]&0x01)
        PowercapstatusFlag=1;
    return 0;
}

/*****************************************************************
* Function Name:    ipmi_set_power_capstatus_command
*
* Description:      This function sets the power cap status
* Input:            intf     - ipmi interface
*                   val      - power cap status
* Output:            
*
* Return:              
*
******************************************************************/

static int
ipmi_set_power_capstatus_command (void * intf,uint8_t val)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[2];
	if(ipmi_get_power_capstatus_command(intf) < 0)
		return -1;

    if (PowercapSetable_flag!=1)
    {
        lprintf(LOG_ERR, " Can not set powercap on this system");
        return -1;
    }
    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_DELL_POWER_CAP_STATUS;
    req.msg.data_len = 2;
    req.msg.data = data;

    data[0] = 00;
    data[1] = val;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
         printf(" Error setting powercap status: ");
	 if (rv < 0) printf("no response\n");
	 else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	//return unlicensed Error code
	 } 
	 else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	 return rv;
    }

    return 0;
}



/*****************************************************************
* Function Name:    ipmi_powermgmt
*
* Description:      This function print the powermonitor details
* Input:            intf     - ipmi interface
* Output:              
*
* Return:              
*
******************************************************************/
static int ipmi_powermgmt(void* intf)
{
    time_t now;
    struct tm* tm;
    char* dte;

    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t msg_data[2];
    uint32_t cumStartTimeConv;
    uint32_t cumReadingConv;
    uint32_t maxPeakStartTimeConv;
    uint32_t ampPeakTimeConv;
    uint16_t ampReadingConv;
    uint32_t wattPeakTimeConv;
    uint32_t wattReadingConv;
    uint32_t bmctimeconv;
    uint32_t * bmctimeconvval;

    IPMI_POWER_MONITOR* pwrMonitorInfo;


    char cumStartTime[26];
    char maxPeakStartTime[26];
    char ampPeakTime[26];
    char wattPeakTime[26];
    char bmctime[26];

    // float cumReading;
    int ampReading;
    int wattReading;
    int ampReadingRemainder;
    // int round;
    // int round2;
    int remainder;

    now = time(0);
    tm = gmtime(&now);
    dte = asctime(tm);

    memset(&req, 0, sizeof(req));
    req.msg.netfn = IPMI_NETFN_STORAGE;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_CMD_GET_SEL_TIME;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
        printf(" Error getting BMC time info ");
	if (rv < 0) printf("no response\n");
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }
    bmctimeconvval=(uint32_t*)rsp;
#if WORDS_BIGENDIAN
    bmctimeconv=BSWAP_32(*bmctimeconvval);
#else
    bmctimeconv=*bmctimeconvval;
#endif

    /* get powermanagement info*/
    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0x0;
    req.msg.cmd = GET_PWRMGMT_INFO_CMD;
    req.msg.data = msg_data;
    req.msg.data_len = 2;

    memset(msg_data, 0, 2);
    msg_data[0] = 0x07;     
    msg_data[1] = 0x01;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error getting power management info ");
	if (rv < 0) printf("no response\n");
	else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	
	} 
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }

    pwrMonitorInfo = (IPMI_POWER_MONITOR*)rsp;

#if WORDS_BIGENDIAN
    cumStartTimeConv = BSWAP_32(pwrMonitorInfo->cumStartTime);
    cumReadingConv = BSWAP_32(pwrMonitorInfo->cumReading);
    maxPeakStartTimeConv = BSWAP_32(pwrMonitorInfo->maxPeakStartTime);
    ampPeakTimeConv = BSWAP_32(pwrMonitorInfo->ampPeakTime);
    ampReadingConv = BSWAP_16(pwrMonitorInfo->ampReading);
    wattPeakTimeConv = BSWAP_32(pwrMonitorInfo->wattPeakTime);
    wattReadingConv = BSWAP_16(pwrMonitorInfo->wattReading);
#else
    cumStartTimeConv = pwrMonitorInfo->cumStartTime;
    cumReadingConv = pwrMonitorInfo->cumReading;
    maxPeakStartTimeConv = pwrMonitorInfo->maxPeakStartTime;
    ampPeakTimeConv = pwrMonitorInfo->ampPeakTime;
    ampReadingConv = pwrMonitorInfo->ampReading;
    wattPeakTimeConv = pwrMonitorInfo->wattPeakTime;
    wattReadingConv = pwrMonitorInfo->wattReading;
#endif

    ipmi_time_to_str(cumStartTimeConv, cumStartTime);

    ipmi_time_to_str(maxPeakStartTimeConv, maxPeakStartTime);
    ipmi_time_to_str(ampPeakTimeConv, ampPeakTime);
    ipmi_time_to_str(wattPeakTimeConv, wattPeakTime);
    ipmi_time_to_str(bmctimeconv, bmctime);

    now = time(0);


    remainder = (cumReadingConv % 1000);
    cumReadingConv = cumReadingConv / 1000;
    remainder = (remainder + 50) / 100;

    ampReading = ampReadingConv;
    ampReadingRemainder = ampReading%10;
    ampReading = ampReading/10;

    wattReading = wattReadingConv;

    printf("Power Tracking Statistics\n");
    printf("Statistic      : Cumulative Energy Consumption\n");
    printf("Start Time     : %s", cumStartTime);
    printf("Finish Time    : %s", bmctime);
    printf("Reading        : %d.%d kWh\n\n", cumReadingConv, remainder);

    printf("Statistic      : System Peak Power\n");
    printf("Start Time     : %s", maxPeakStartTime);
    printf("Peak Time      : %s", wattPeakTime);
    printf("Peak Reading   : %d W\n\n", wattReading);

    printf("Statistic      : System Peak Amperage\n");
    printf("Start Time     : %s", maxPeakStartTime);
    printf("Peak Time      : %s", ampPeakTime);
    printf("Peak Reading   : %d.%d A\n", ampReading, ampReadingRemainder);


    return 0;

}
/*****************************************************************
* Function Name:    ipmi_powermgmt_clear
*
* Description:     This function clears peakpower / cumulativepower value
* Input:           intf           - ipmi interface
*                  clearValue     - peakpower / cumulativepower
* Output:          
*
* Return:              
*
******************************************************************/
static int
ipmi_powermgmt_clear(void* intf,uint8_t clearValue)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t clearType;
    uint8_t msg_data[3];

    if (clearValue) {
        clearType = 2;
    } else {
        clearType = 1;
    }

    /* clear powermanagement info*/
    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;
    req.msg.cmd = CLEAR_PWRMGMT_INFO_CMD;
    req.msg.data = msg_data;
    req.msg.data_len = 3;


    memset(msg_data, 0, 3);
    msg_data[0] = 0x07;
    msg_data[1] = 0x01;
    msg_data[2] = clearType;


    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error clearing power values: ");
	if (rv < 0) printf("no response\n");
	else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	
	} 
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }
	return 0;

}

/*****************************************************************
* Function Name:    watt_to_btuphr_conversion
*
* Description:      This function converts the power value in watt to btuphr
* Input:            powerinwatt     - power in watt
*                               
* Output:           power in btuphr
*
* Return:              
*
******************************************************************/
static uint64_t watt_to_btuphr_conversion(uint32_t powerinwatt)
{
    uint64_t powerinbtuphr;
    powerinbtuphr=(uint64_t)(3.413*powerinwatt);

    return(powerinbtuphr);
}

/*****************************************************************
* Function Name:    btuphr_to_watt_conversion
*
* Description:      This function converts the power value in  btuphr to watt
* Input:            powerinbtuphr   - power in btuphr
*                              
* Output:           power in watt
*
* Return:                
*
******************************************************************/
static uint32_t btuphr_to_watt_conversion(uint64_t powerinbtuphr)
{
    uint32_t powerinwatt;
    /*returning the floor value*/
    powerinwatt= (uint32_t)(powerinbtuphr/3.413);
    return (powerinwatt);
}

/*****************************************************************
* Function Name:        ipmi_get_power_headroom_command
*
* Description:          This function prints the Power consumption information
* Input:                intf    - ipmi interface
*                       unit    - watt / btuphr
* Output:           
*
* Return:              
*
******************************************************************/
static int ipmi_get_power_headroom_command (void * intf,uint8_t unit)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint64_t peakpowerheadroombtuphr;
    uint64_t instantpowerhearoom;

    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;
    req.msg.cmd = GET_PWR_HEADROOM_CMD;
    req.msg.data_len = 0;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error getting power headroom status: ");
	if (rv < 0) printf("no response\n");
	else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	
	}
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }
    if(verbose>1)
        printf("power headroom  Data               : %x %x %x %x ",
        /*need to look into */                                                                  rsp[0], rsp[1], rsp[2], rsp[3]);
    powerheadroom= *(( POWER_HEADROOM *)rsp);
#if WORDS_BIGENDIAN
    powerheadroom.instheadroom = BSWAP_16(powerheadroom.instheadroom);
    powerheadroom.peakheadroom = BSWAP_16(powerheadroom.peakheadroom);
#endif

    printf ("Headroom\n\r");
    printf ("Statistic                     Reading\n\r");   

    if(unit == btuphr)
    {
        peakpowerheadroombtuphr=watt_to_btuphr_conversion(powerheadroom.peakheadroom);
        instantpowerhearoom= watt_to_btuphr_conversion(powerheadroom.instheadroom);

        printf ("System Instantaneous Headroom : %ld BTU/hr\n",instantpowerhearoom);
		printf ("System Peak Headroom          : %ld BTU/hr\n",peakpowerheadroombtuphr);
	}
	else
	{
        printf ("System Instantaneous Headroom : %d W\n",powerheadroom.instheadroom);
		printf ("System Peak Headroom          : %d W\n",powerheadroom.peakheadroom);
	}

    return 0;
}

/*****************************************************************
* Function Name:       ipmi_get_power_consumption_data
*
* Description:         This function updates the instant Power consumption information
* Input:               intf - ipmi interface
* Output:              power consumption current reading 
*                      Assumption value will be in Watt.
*
* Return:               
*
******************************************************************/
static int ipmi_get_power_consumption_data(void* intf,uint8_t unit)
{
    int rc = 0;
    SensorReadingType sensorReadingData;
    uint8_t rsp[IPMI_RSPBUF_SIZE]; 
    struct sdr_record_list *sdr = NULL;
    uchar sdrbuf[SDR_SZ];
    double readingf, warningf, failuref;
    int readingbtuphr=0;
    int warning_threshbtuphr=0;
    int failure_thresbtuphr=0;
    int status=0;
    int sensor_number = 0;

    if (sdrfile != NULL) {
        rc = get_sdr_file(sdrfile,&sdrcache);
        if (rc) printf ("Error 0x%02x: Cannot get SDRs from %s\n",rc,sdrfile);
    } else if (sdrcache == NULL) {
	rc = get_sdr_cache(&sdrcache);
        if (rc) printf ("Error 0x%02x: Cannot get SDRs\n",rc);
    }

    rc = find_sdr_by_tag(sdrbuf, sdrcache, "System Level", fdebug);
    if (rc != 0)
    {
        printf ("Error %d: Cannot access the System Level sensor data\n",rc);
        return rc;
    }
    sdr = (struct sdr_record_list *)sdrbuf;

    sensor_number = sdrbuf[7]; // sdr->record.full->keys.sensor_num;
    if (fdebug) printf("calling GetSensorReading(%x)\n",sensor_number);
    rc = GetSensorReading(sensor_number, sdrbuf, 
			  (uchar *)&sensorReadingData.sensorReading);
    if (rc != 0) 
	printf("Error %d getting sensor %x reading\n",rc,sensor_number);

    rc = GetSensorThresholds( sensor_number, rsp);
    if (fdebug) printf("GetSensorThresholds(%x) rc = %d\n",sensor_number,rc);
    if (rc == 0)
    {   
        readingf = RawToFloat(sensorReadingData.sensorReading,sdrbuf);
        warningf = RawToFloat(rsp[4], sdrbuf);
        failuref = RawToFloat(rsp[5], sdrbuf);
        readingbtuphr = (int)readingf;
        warning_threshbtuphr = (int)warningf;
        failure_thresbtuphr = (int)failuref;

	if (fdebug) {
	   printf("Reading 0x%02x = %.2f, Warning 0x%02x = %.2f, Failure 0x%02x = %.2f\n",
		sensorReadingData.sensorReading, readingf,
		rsp[4], warningf, rsp[5], failuref);
	}

        printf ("System Board System Level\n\r");
        if (unit==btuphr)
        {
            readingbtuphr= watt_to_btuphr_conversion(readingbtuphr);
            warning_threshbtuphr= watt_to_btuphr_conversion(warning_threshbtuphr);
            failure_thresbtuphr= watt_to_btuphr_conversion( failure_thresbtuphr);

            printf ("Reading                : %d BTU/hr\n",readingbtuphr);
            printf ("Warning threshold      : %d BTU/hr\n",warning_threshbtuphr);
            printf ("Failure threshold      : %d BTU/hr\n",failure_thresbtuphr);
        }
        else
        {
            printf ("Reading                : %d W \n",readingbtuphr);              
            printf ("Warning threshold      : %d W \n",(warning_threshbtuphr));
            printf ("Failure threshold      : %d W \n",(failure_thresbtuphr));
        }
    }
    else
    {
        printf ("Error %d: Cannot access the System Level threshold data\n",rc);
        return -1;              
    }       
    return status;
}




/*****************************************************************
* Function Name:      ipmi_get_instan_power_consmpt_data
*
* Description:        This function updates the instant Power consumption information
* Input:              intf - ipmi interface
* Output:             instpowerconsumptiondata - instant Power consumption information
*
* Return:              
*
******************************************************************/

static int ipmi_get_instan_power_consmpt_data(void* intf,
                                              IPMI_INST_POWER_CONSUMPTION_DATA* instpowerconsumptiondata)
{

    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;

    uint8_t msg_data[2];


    /*get instantaneous power consumption command*/
    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;
    req.msg.cmd = GET_PWR_CONSUMPTION_CMD;

    req.msg.data = msg_data;
    req.msg.data_len = 2;



    memset(msg_data, 0, 2);

    msg_data[0] = 0x0A;     
    msg_data[1] = 0x00;


    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error getting power consumption data: ");
	if (rv < 0) printf("no response\n");
	else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	
	} 
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }

    * instpowerconsumptiondata = * ( (IPMI_INST_POWER_CONSUMPTION_DATA*) (rsp));
#if WORDS_BIGENDIAN
    instpowerconsumptiondata->instanpowerconsumption = BSWAP_16(instpowerconsumptiondata->instanpowerconsumption);
    instpowerconsumptiondata->instanApms = BSWAP_16(instpowerconsumptiondata->instanApms);
    instpowerconsumptiondata->resv1 = BSWAP_16(instpowerconsumptiondata->resv1);
#endif

    return 0;


}


/*****************************************************************
* Function Name:      ipmi_print_get_instan_power_Amps_data
*
* Description:        This function prints the instant Power consumption information
* Input:              instpowerconsumptiondata - instant Power consumption information
* Output:               
*
* Return:              
*
******************************************************************/
static void ipmi_print_get_instan_power_Amps_data(IPMI_INST_POWER_CONSUMPTION_DATA instpowerconsumptiondata)
{
    uint16_t intampsval=0;
    uint16_t decimalampsval=0;


    if (instpowerconsumptiondata.instanApms>0)
    {
        decimalampsval=(instpowerconsumptiondata.instanApms%10);
        intampsval=instpowerconsumptiondata.instanApms/10;
    }
    printf("\nAmperage value: %d.%d A \n",intampsval,decimalampsval);
}
/*****************************************************************
* Function Name:     ipmi_print_get_power_consmpt_data
*
* Description:       This function prints the Power consumption information
* Input:             intf            - ipmi interface
*                    unit            - watt / btuphr 
* Output:               
*
* Return:              
*
******************************************************************/
static int ipmi_print_get_power_consmpt_data(void* intf,uint8_t  unit)
{

    int rc = 0;
    IPMI_INST_POWER_CONSUMPTION_DATA instpowerconsumptiondata = {0,0,0,0};
    // int i;
    //uint16_t inputwattageL=0;
    //int sensorIndex = 0;
    //uint32_t readingbtuphr;
    //uint32_t warning_threshbtuphr;
    //uint32_t failure_thresbtuphr;

    printf ("\nPower consumption information\n");


    rc=ipmi_get_power_consumption_data(intf,unit);
    if (-1 == rc)
        return rc;

    rc=ipmi_get_instan_power_consmpt_data(intf,&instpowerconsumptiondata);
    if (-1 == rc)
        return rc;

    ipmi_print_get_instan_power_Amps_data(instpowerconsumptiondata);


    rc=ipmi_get_power_headroom_command(intf,unit);      

    if (-1 == rc)
        return rc;

    return rc;


}


/*****************************************************************
* Function Name:   ipmi_get_avgpower_consmpt_history
*
* Description:     This function updates the average power consumption information
* Input:           intf            - ipmi interface
* Output:          pavgpower- average power consumption information
*
* Return:              
*
******************************************************************/
static int ipmi_get_avgpower_consmpt_history(void* intf,IPMI_AVGPOWER_CONSUMP_HISTORY* pavgpower )
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];

    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    req.msg.data = data;
    data[0] = 0;
    data[1] = 0xeb;
    data[2] = 0;
    data[3] = 0;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error getting average power consumption data: ");
	if (rv < 0) printf("no response\n");
	else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	
	}
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }

    if (verbose > 1)
    {
        printf("Average power consumption history  Data               :%x %x %x %x %x %x %x %x\n\n",
            rsp[0], rsp[1], rsp[2], rsp[3], 
            rsp[4], rsp[5], rsp[6], rsp[7]);

    }

    *pavgpower = *( (IPMI_AVGPOWER_CONSUMP_HISTORY*) rsp);
#if WORDS_BIGENDIAN
    pavgpower->lastminutepower = BSWAP_16(pavgpower->lastminutepower);
    pavgpower->lasthourpower = BSWAP_16(pavgpower->lasthourpower);
    pavgpower->lastdaypower = BSWAP_16(pavgpower->lastdaypower);
    pavgpower->lastweakpower = BSWAP_16(pavgpower->lastweakpower);
#endif

    return 0;
}

/*****************************************************************
* Function Name:    ipmi_get_peakpower_consmpt_history
*
* Description:      This function updates the peak power consumption information
* Input:            intf            - ipmi interface
* Output:           pavgpower- peak power consumption information
*
* Return:         
*
******************************************************************/
static int ipmi_get_peakpower_consmpt_history(void* intf,IPMI_POWER_CONSUMP_HISTORY * pstPeakpower)
{

    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];

    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    req.msg.data = data;
    data[0] = 0;
    data[1] = 0xec;
    data[2] = 0;
    data[3] = 0;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error getting peak power consumption history: ");
	if (rv < 0) printf("no response\n");
	else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	
	} 
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }

    if (verbose > 1)
    {
        printf("Peak power consmhistory  Data               : %x %x %x %x %x %x %x %x %x %x\n   %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n\n",
            rsp[0], rsp[1], rsp[2], rsp[3], 
            rsp[4], rsp[5], rsp[6], rsp[7], 
            rsp[8], rsp[9], rsp[10], rsp[11], 
            rsp[12], rsp[13], rsp[14], rsp[15], 
            rsp[16], rsp[17], rsp[18], rsp[19],  
            rsp[20], rsp[21], rsp[22], rsp[23]
        );

    }
    *pstPeakpower =* ((IPMI_POWER_CONSUMP_HISTORY*)rsp);
#if WORDS_BIGENDIAN
    pstPeakpower->lastminutepower = BSWAP_16(pstPeakpower->lastminutepower);
    pstPeakpower->lasthourpower = BSWAP_16(pstPeakpower->lasthourpower);
    pstPeakpower->lastdaypower = BSWAP_16(pstPeakpower->lastdaypower);
    pstPeakpower->lastweakpower = BSWAP_16(pstPeakpower->lastweakpower);
    pstPeakpower->lastminutepowertime = BSWAP_32(pstPeakpower->lastminutepowertime);
    pstPeakpower->lasthourpowertime = BSWAP_32(pstPeakpower->lasthourpowertime);
    pstPeakpower->lastdaypowertime = BSWAP_32(pstPeakpower->lastdaypowertime);
    pstPeakpower->lastweekpowertime = BSWAP_32(pstPeakpower->lastweekpowertime);
#endif
    return 0;
}


/*****************************************************************
* Function Name:    ipmi_get_minpower_consmpt_history
*
* Description:      This function updates the peak power consumption information
* Input:            intf            - ipmi interface
* Output:           pavgpower- peak power consumption information
*
* Return:         
*
******************************************************************/
static int ipmi_get_minpower_consmpt_history(void* intf,IPMI_POWER_CONSUMP_HISTORY * pstMinpower)
{

    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[4];

    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    req.msg.data = data;
    data[0] = 0;
    data[1] = 0xed;
    data[2] = 0;
    data[3] = 0;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error getting min power consumption history: ");
	if (rv < 0) printf("no response\n");
	else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	
	}
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }

    if (verbose > 1)
    {
        printf("Peak power consmhistory  Data               : %x %x %x %x %x %x %x %x %x %x\n   %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n\n",
            rsp[0], rsp[1], rsp[2], rsp[3], 
            rsp[4], rsp[5], rsp[6], rsp[7], 
            rsp[8], rsp[9], rsp[10], rsp[11], 
            rsp[12], rsp[13], rsp[14], rsp[15], 
            rsp[16], rsp[17], rsp[18], rsp[19],  
            rsp[20], rsp[21], rsp[22], rsp[23]
        );

    }
    *pstMinpower =* ((IPMI_POWER_CONSUMP_HISTORY*)rsp);
#if WORDS_BIGENDIAN
    pstMinpower->lastminutepower = BSWAP_16(pstMinpower->lastminutepower);
    pstMinpower->lasthourpower = BSWAP_16(pstMinpower->lasthourpower);
    pstMinpower->lastdaypower = BSWAP_16(pstMinpower->lastdaypower);
    pstMinpower->lastweakpower = BSWAP_16(pstMinpower->lastweakpower);
    pstMinpower->lastminutepowertime = BSWAP_32(pstMinpower->lastminutepowertime);
    pstMinpower->lasthourpowertime = BSWAP_32(pstMinpower->lasthourpowertime);
    pstMinpower->lastdaypowertime = BSWAP_32(pstMinpower->lastdaypowertime);
    pstMinpower->lastweekpowertime = BSWAP_32(pstMinpower->lastweekpowertime);
#endif
    return 0;
}



/*****************************************************************
* Function Name:    ipmi_print_power_consmpt_history
*
* Description:      This function print the average and peak power consumption information
* Input:            intf      - ipmi interface
*                   unit      - watt / btuphr
* Output:                
*
* Return:              
*
******************************************************************/
static int ipmi_print_power_consmpt_history(void* intf,int unit )
{

    char timestr[30];

    uint32_t lastminutepeakpower;
    uint32_t lasthourpeakpower;
    uint32_t lastdaypeakpower;
    uint32_t lastweekpeakpower;

    IPMI_AVGPOWER_CONSUMP_HISTORY avgpower;
    IPMI_POWER_CONSUMP_HISTORY stMinpower;
    IPMI_POWER_CONSUMP_HISTORY stPeakpower;
    int rc=0;

    uint64_t tempbtuphrconv;
    //uint16_t temp;


    rc= ipmi_get_avgpower_consmpt_history(intf,&avgpower);    
    if (-1 == rc)
        return rc;

    rc= ipmi_get_peakpower_consmpt_history(intf,&stPeakpower);
    if (-1 == rc)
        return rc;

    rc= ipmi_get_minpower_consmpt_history(intf,&stMinpower);
    if (-1 == rc)
        return rc;


    if(rc==0) 
    {
        printf ("Power Consumption History\n\r\n\r");
        /* The fields are alligned manually changing the spaces will alter the alignment*/
        printf ("Statistic                   Last Minute     Last Hour     Last Day     Last Week\n\r\n\r");

        if (unit ==btuphr)
        {
            printf ("Average Power Consumption  ");         
            tempbtuphrconv=watt_to_btuphr_conversion(avgpower.lastminutepower);
            printf ("%4d BTU/hr     ",tempbtuphrconv);
            tempbtuphrconv=watt_to_btuphr_conversion(avgpower.lasthourpower);
            printf ("%4d BTU/hr   ",tempbtuphrconv);
            tempbtuphrconv=watt_to_btuphr_conversion(avgpower.lastdaypower);
            printf ("%4d BTU/hr  ",tempbtuphrconv);
            tempbtuphrconv=watt_to_btuphr_conversion(avgpower.lastweakpower);
            printf ("%4d BTU/hr\n\r",tempbtuphrconv);

            printf ("Max Power Consumption      ");         
            tempbtuphrconv=watt_to_btuphr_conversion(stPeakpower.lastminutepower);
            printf ("%4d BTU/hr     ",tempbtuphrconv);
            tempbtuphrconv=watt_to_btuphr_conversion(stPeakpower.lasthourpower);
            printf ("%4d BTU/hr   ",tempbtuphrconv);
            tempbtuphrconv=watt_to_btuphr_conversion(stPeakpower.lastdaypower);
            printf ("%4d BTU/hr  ",tempbtuphrconv);
            tempbtuphrconv=watt_to_btuphr_conversion(stPeakpower.lastweakpower);
            printf ("%4d BTU/hr\n\r",tempbtuphrconv);

            printf ("Min Power Consumption      ");         
            tempbtuphrconv=watt_to_btuphr_conversion(stMinpower.lastminutepower);
            printf ("%4d BTU/hr     ",tempbtuphrconv);
            tempbtuphrconv=watt_to_btuphr_conversion(stMinpower.lasthourpower);
            printf ("%4d BTU/hr   ",tempbtuphrconv);
            tempbtuphrconv=watt_to_btuphr_conversion(stMinpower.lastdaypower);
            printf ("%4d BTU/hr  ",tempbtuphrconv);
            tempbtuphrconv=watt_to_btuphr_conversion(stMinpower.lastweakpower);
            printf ("%4d BTU/hr\n\r\n\r",tempbtuphrconv);

        }
        else
        {

            printf ("Average Power Consumption  ");         
            tempbtuphrconv=(avgpower.lastminutepower);
			printf ("%4ld W          ",tempbtuphrconv);
			tempbtuphrconv=(avgpower.lasthourpower);
			printf ("%4ld W        ",tempbtuphrconv);
			tempbtuphrconv=(avgpower.lastdaypower);
			printf ("%4ld W       ",tempbtuphrconv);
			tempbtuphrconv=(avgpower.lastweakpower);
			printf ("%4ld W   \n\r",tempbtuphrconv);

		printf ("Max Power Consumption      ");		
			tempbtuphrconv=(stPeakpower.lastminutepower);
			printf ("%4ld W          ",tempbtuphrconv);
			tempbtuphrconv=(stPeakpower.lasthourpower);
			printf ("%4ld W        ",tempbtuphrconv);
			tempbtuphrconv=(stPeakpower.lastdaypower);
			printf ("%4ld W       ",tempbtuphrconv);
			tempbtuphrconv=(stPeakpower.lastweakpower);
			printf ("%4ld W   \n\r",tempbtuphrconv);

		printf ("Min Power Consumption      ");		
			tempbtuphrconv=(stMinpower.lastminutepower);
			printf ("%4ld W          ",tempbtuphrconv);
			tempbtuphrconv=(stMinpower.lasthourpower);
			printf ("%4ld W        ",tempbtuphrconv);
			tempbtuphrconv=(stMinpower.lastdaypower);
			printf ("%4ld W       ",tempbtuphrconv);
			tempbtuphrconv=(stMinpower.lastweakpower);
		   	printf ("%4ld W   \n\r\n\r",tempbtuphrconv);
		}		
		
        lastminutepeakpower=stPeakpower.lastminutepowertime;
        lasthourpeakpower=stPeakpower.lasthourpowertime;
        lastdaypeakpower=stPeakpower.lastdaypowertime;
        lastweekpeakpower=stPeakpower.lastweekpowertime;

        printf ("Max Power Time\n\r");
        ipmi_time_to_str(lastminutepeakpower, timestr);         
        printf ("Last Minute     : %s",timestr);
        ipmi_time_to_str(lasthourpeakpower, timestr);           
        printf ("Last Hour       : %s",timestr);
        ipmi_time_to_str(lastdaypeakpower, timestr);            
        printf ("Last Day        : %s",timestr);
        ipmi_time_to_str(lastweekpeakpower, timestr);           
        printf ("Last Week       : %s",timestr);                


        lastminutepeakpower=stMinpower.lastminutepowertime;
        lasthourpeakpower=stMinpower.lasthourpowertime;
        lastdaypeakpower=stMinpower.lastdaypowertime;
        lastweekpeakpower=stMinpower.lastweekpowertime; 

        printf ("Min Power Time\n\r");
        ipmi_time_to_str(lastminutepeakpower, timestr);         
        printf ("Last Minute     : %s",timestr);
        ipmi_time_to_str(lasthourpeakpower, timestr);           
        printf ("Last Hour       : %s",timestr);
        ipmi_time_to_str(lastdaypeakpower, timestr);            
        printf ("Last Day        : %s",timestr);
        ipmi_time_to_str(lastweekpeakpower, timestr);           
        printf ("Last Week       : %s",timestr);        

    }
	return rc;

}



/*****************************************************************
* Function Name:    ipmi_get_power_cap
*
* Description:      This function updates the power cap information
* Input:            intf         - ipmi interface
* Output:           ipmipowercap - power cap information
*
* Return:          
*
******************************************************************/

static int ipmi_get_power_cap(void* intf,IPMI_POWER_CAP* ipmipowercap )
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    //uint64_t tempbtuphrconv;
    uint8_t data[4];

    /* power supply rating command*/
    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    req.msg.data = data;

    data[0] = 0;
    data[1] = IPMI_DELL_POWER_CAP;
    data[2] = 0;
    data[3] = 0;


    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error getting power cap: ");
	if (rv < 0) printf("no response\n");
	else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	
	} 
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }
    if (verbose > 1){
        printf("power cap  Data               :%x %x %x %x %x %x %x %x %x %x %x",
            rsp[1], rsp[2], rsp[3], 
            rsp[4], rsp[5], rsp[6], rsp[7], 
            rsp[8], rsp[9], rsp[10],rsp[11]);

    }

    * ipmipowercap = *((IPMI_POWER_CAP*)(rsp));
#if WORDS_BIGENDIAN
    ipmipowercap->PowerCap = BSWAP_16(ipmipowercap->PowerCap);
    ipmipowercap->MaximumPowerConsmp = BSWAP_16(ipmipowercap->MaximumPowerConsmp);
    ipmipowercap->MinimumPowerConsmp = BSWAP_16(ipmipowercap->MinimumPowerConsmp);
    ipmipowercap->totalnumpowersupp = BSWAP_16(ipmipowercap->totalnumpowersupp);
    ipmipowercap->AvailablePower = BSWAP_16(ipmipowercap->AvailablePower);
    ipmipowercap->SystemThrottling = BSWAP_16(ipmipowercap->SystemThrottling);
    ipmipowercap->Resv = BSWAP_16(ipmipowercap->Resv);
#endif

    return 0;
}

/*****************************************************************
* Function Name:    ipmi_print_power_cap
*
* Description:      This function print the power cap information
* Input:            intf            - ipmi interface
*                   unit            - watt / btuphr
* Output:                
* Return:               
*
******************************************************************/
static int ipmi_print_power_cap(void* intf,uint8_t unit )
{
    uint64_t tempbtuphrconv;
    int rc;
    IPMI_POWER_CAP ipmipowercap;

	memset(&ipmipowercap,0,sizeof(ipmipowercap));
    rc=ipmi_get_power_cap(intf,&ipmipowercap);


    if (rc==0) 
    {
        if (unit ==btuphr){
            tempbtuphrconv=watt_to_btuphr_conversion(ipmipowercap.MaximumPowerConsmp);
			printf ("Maximum power: %ld  BTU/hr\n",tempbtuphrconv);
			tempbtuphrconv=watt_to_btuphr_conversion(ipmipowercap.MinimumPowerConsmp);
			printf ("Minimum power: %ld  BTU/hr\n",tempbtuphrconv);
			tempbtuphrconv=watt_to_btuphr_conversion(ipmipowercap.PowerCap);
			printf ("Power cap    : %ld  BTU/hr\n",tempbtuphrconv);
		}else{
			printf ("Maximum power: %d Watt\n",ipmipowercap.MaximumPowerConsmp);
			printf ("Minimum power: %d Watt\n",ipmipowercap.MinimumPowerConsmp);
			printf ("Power cap    : %d Watt\n",ipmipowercap.PowerCap);
        }
    }
    return rc;

}  

/*****************************************************************
* Function Name:     ipmi_set_power_cap
*
* Description:       This function updates the power cap information
* Input:             intf            - ipmi interface
*                    unit            - watt / btuphr
*                    val             - new power cap value
* Output:          
* Return:               
*
******************************************************************/
static int ipmi_set_power_cap(void* intf,int unit,int val )
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[13];
    uint16_t powercapval;
    uint64_t maxpowerbtuphr;
    uint64_t maxpowerbtuphr1;
    uint64_t minpowerbtuphr;
    IPMI_POWER_CAP ipmipowercap;

	if(ipmi_get_power_capstatus_command(intf) < 0)
		return -1;	// Adding the failed condition check

    if (PowercapSetable_flag!=1)
    {
        lprintf(LOG_ERR, " Can not set powercap on this system");
        return -1;
    }
    else if(PowercapstatusFlag!=1)
    {
        lprintf(LOG_ERR, " Power cap set feature is not enabled");
        return -1;
    }

    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_GET_SYS_INFO;
    req.msg.data_len = 4;
    memset(data, 0, 4);
    req.msg.data = data;

    data[0] = 0;
    data[1] = IPMI_DELL_POWER_CAP;
    data[2] = 0;
    data[3] = 0;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error getting power cap: ");
	if (rv < 0) printf("no response\n");
	else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	
	}
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }
    if (verbose > 1)
    {
        printf("power cap  Data               :%x %x %x %x %x %x %x %x %x %x %x",
            rsp[1], rsp[2], rsp[3], 
            rsp[4], rsp[5], rsp[6], rsp[7], 
            rsp[8], rsp[9], rsp[10],rsp[11]);

    }

    ipmipowercap.PowerCap=((rsp[1]<<8)+rsp[2]);
    ipmipowercap.unit=rsp[3];
    ipmipowercap.MaximumPowerConsmp=((rsp[4]<<8)+rsp[5]);
    ipmipowercap.MinimumPowerConsmp=((rsp[6]<<8)+rsp[7]);
    /* ARC: need Dell to verify these 3 values */
    ipmipowercap.totalnumpowersupp = rsp[8];
    ipmipowercap.AvailablePower = ((rsp[9]<<8)+rsp[10]);
    ipmipowercap.SystemThrottling = rsp[11];

    memset(data, 0, 13);
    req.msg.netfn = IPMI_NETFN_APP;
    req.msg.lun = 0;
    req.msg.cmd = IPMI_SET_SYS_INFO;
    req.msg.data_len = 13;
    req.msg.data = data;
    data[0] = IPMI_DELL_POWER_CAP;
    powercapval=val;


    data[1] = (powercapval&0XFF);                   
    data[2] = ((powercapval&0XFF00)>>8);
    data[3] = unit;

    data[4]=((ipmipowercap.MaximumPowerConsmp&0xFF));
    data[5]=((ipmipowercap.MaximumPowerConsmp&0xFF00)>>8);
    data[6]=((ipmipowercap.MinimumPowerConsmp&0xFF));
    data[7]=((ipmipowercap.MinimumPowerConsmp&0xFF00)>>8);
    data[8]=(uint8_t)(ipmipowercap.totalnumpowersupp);  
    data[9]=((ipmipowercap.AvailablePower&0xFF));
    data[10]=((ipmipowercap.AvailablePower&0xFF00)>>8);
    data[11]=(uint8_t)(ipmipowercap.SystemThrottling);
    data[12]=0x00;

    ipmipowercap.MaximumPowerConsmp = BSWAP_16(ipmipowercap.MaximumPowerConsmp);
    ipmipowercap.MinimumPowerConsmp = BSWAP_16(ipmipowercap.MinimumPowerConsmp);
    ipmipowercap.PowerCap = BSWAP_16(ipmipowercap.PowerCap);
    if(unit==btuphr)
    {
        val = btuphr_to_watt_conversion(val);

    }
    else if(unit ==percent)
    {
       	if((val <0)||(val>100))
        {
            lprintf(LOG_ERR, " Cap value is out of boundary conditon it should be between 0  - 100");
            return -1;
        }
        val =( (val*(ipmipowercap.MaximumPowerConsmp -ipmipowercap.MinimumPowerConsmp))/100)+ipmipowercap.MinimumPowerConsmp;
        lprintf(LOG_ERR, " Cap value in percentage is  %d ",val);
        data[1] = (val&0XFF);                   
        data[2] = ((val&0XFF00)>>8);
        data[3] = watt;
    }
    if(((val<ipmipowercap.MinimumPowerConsmp)||(val>ipmipowercap.MaximumPowerConsmp))&&(unit==watt))
    {
        lprintf(LOG_ERR, " Cap value is out of boundary conditon it should be between %d  - %d",
            ipmipowercap.MinimumPowerConsmp,ipmipowercap.MaximumPowerConsmp);
        return -1;
    }
    else if(((val<ipmipowercap.MinimumPowerConsmp)||(val>ipmipowercap.MaximumPowerConsmp))&&(unit==btuphr))
    {
        minpowerbtuphr= watt_to_btuphr_conversion(ipmipowercap.MinimumPowerConsmp);
        maxpowerbtuphr=watt_to_btuphr_conversion(ipmipowercap.MaximumPowerConsmp);
        maxpowerbtuphr1= watt_to_btuphr_conversion(ipmipowercap.MaximumPowerConsmp);
        lprintf(LOG_ERR, " Cap value is out of boundary conditon it should be between %d",
            minpowerbtuphr);
        lprintf(LOG_ERR, " -%d",
            maxpowerbtuphr1);

        return -1;
    }
    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error setting power cap: ");
	if (rv < 0) printf("no response\n");
	else if((iDRAC_FLAG == IDRAC_12G) && (rv == LICENSE_NOT_SUPPORTED)) {
		printf("FM001 : A required license is missing or expired\n");
		return rv;	
	} 
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }
    if (verbose > 1)
    {
        printf("CC for setpowercap :%d ",rv);
    }
    return 0;
}

#ifdef NOT_USED
static int getpowersupplyfruinfo(void *intf, uint8_t id, 
                       struct fru_header header, struct fru_info fru);
/*****************************************************************
* Function Name:    getpowersupplyfruinfo
*
* Description:      This function retrieves the FRU header
* Input:            intf    - ipmi interface
*                   header  - watt / btuphr
*                   fru     - FRU information
* Output:           header  - FRU header
* Return:           
*
******************************************************************/
static int getpowersupplyfruinfo(void *intf, uint8_t id, 
                         struct fru_header header, struct fru_info fru)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;

    uint8_t msg_data[4];

    memset(&fru, 0, sizeof(struct fru_info));
    memset(&header, 0, sizeof(struct fru_header));

    /*
    * get info about this FRU
    */
    memset(msg_data, 0, 4);
    msg_data[0] = id;

    memset(&req, 0, sizeof(req));
    req.msg.netfn = IPMI_NETFN_STORAGE;
    req.msg.lun = 0;
    req.msg.cmd = GET_FRU_INFO;
    req.msg.data = msg_data;
    req.msg.data_len = 1;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Device not present, ");
	if (rv < 0) printf("no response\n");
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }

    fru.size = (rsp[1] << 8) | rsp[0];
    fru.access = rsp[2] & 0x1;

    lprintf(LOG_DEBUG, "fru.size = %d bytes (accessed by %s)",
        fru.size, fru.access ? "words" : "bytes");

    if (fru.size < 1) {
        printf(" Invalid FRU size %d", fru.size);
        return -1;
    }

    /*
    * retrieve the FRU header
    */
    msg_data[0] = id;
    msg_data[1] = 0;
    msg_data[2] = 0;
    msg_data[3] = 8;

    memset(&req, 0, sizeof(req));
    req.msg.netfn = IPMI_NETFN_STORAGE;
    req.msg.lun = 0;
    req.msg.cmd = GET_FRU_DATA;
    req.msg.data = msg_data;
    req.msg.data_len = 4;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Device not present, ");
	if (rv < 0) printf("no response\n");
	else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	return rv;
    }

    if (verbose > 1)
        printbuf(rsp, rsp_len, "FRU DATA");

    memcpy(&header, &rsp[1], 8);

	return 0;


}
#endif

/*****************************************************************
* Function Name:   ipmi_powermonitor_usage
*
* Description:     This function prints help message for powermonitor command
* Input:              
* Output:       
*
* Return:              
*
******************************************************************/
static void
ipmi_powermonitor_usage(void)
{
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   powermonitor");
    lprintf(LOG_NOTICE, "      Shows power tracking statistics ");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   powermonitor clear cumulativepower");
    lprintf(LOG_NOTICE, "      Reset cumulative power reading");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   powermonitor clear peakpower");
    lprintf(LOG_NOTICE, "      Reset peak power reading");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   powermonitor powerconsumption");
    lprintf(LOG_NOTICE, "      Displays power consumption in <watt|btuphr>");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   powermonitor powerconsumptionhistory <watt|btuphr>");
    lprintf(LOG_NOTICE, "      Displays power consumption history ");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   powermonitor getpowerbudget");
    lprintf(LOG_NOTICE, "      Displays power cap in <watt|btuphr>");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   powermonitor setpowerbudget <val><watt|btuphr|percent>");
    lprintf(LOG_NOTICE, "      Allows user to set the  power cap in <watt|BTU/hr|percentage>");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   powermonitor enablepowercap ");
    lprintf(LOG_NOTICE, "      To enable set power cap");
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   powermonitor disablepowercap ");
    lprintf(LOG_NOTICE, "      To disable set power cap");
    lprintf(LOG_NOTICE, "");

}
/*****************************************************************
* Function Name:	   ipmi_delloem_vFlash_main
*
* Description:		   This function processes the delloem vFlash command
* Input:			   intf    - ipmi interface
					   argc    - no of arguments
					   argv    - argument string array
* Output:		 
*
* Return:			   return code	   0 - success
*						  -1 - failure
*
******************************************************************/

static int ipmi_delloem_vFlash_main (void * intf, int  argc, char ** argv)
{
	int rc = 0;

	current_arg++;
	rc = ipmi_delloem_vFlash_process(intf, current_arg, argv);
	return(rc);
}



/*****************************************************************
* Function Name: 	get_vFlash_compcode_str
*
* Description: 	This function maps the vFlash completion code
* 		to a string
* Input : vFlash completion code and static array of codes vs strings
* Output: - 		
* Return: returns the mapped string		
*
******************************************************************/
const char * 
// get_vFlash_compcode_str(uint8_t vflashcompcode, const struct vFlashstr *vs)
get_vFlash_compcode_str(uint8_t vflashcompcode, const vFlashstr *vs)
{
	static char un_str[32];
	int i;

	for (i = 0; vs[i].str != NULL; i++) {
		if (vs[i].val == vflashcompcode)
			return vs[i].str;
	}

	memset(un_str, 0, 32);
	snprintf(un_str, 32, "Unknown (0x%02X)", vflashcompcode);

	return un_str;
}

/*****************************************************************
* Function Name: 	ipmi_get_sd_card_info
*
* Description: This function prints the vFlash Extended SD card info
* Input : ipmi interface
* Output: prints the sd card extended info		
* Return: 0 - success -1 - failure
*
******************************************************************/
static int
ipmi_get_sd_card_info(void* intf) {
	uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
	struct ipmi_rq req;

	uint8_t msg_data[2];
	uint8_t input_length=0;
	uint8_t cardstatus=0x00;

	IPMI_DELL_SDCARD_INFO * sdcardinfoblock;

	input_length = 2;
	msg_data[0] = msg_data[1] = 0x00;

	req.msg.netfn = IPMI_DELL_OEM_NETFN;
	req.msg.lun = 0;
	req.msg.cmd = IPMI_GET_EXT_SD_CARD_INFO;
	req.msg.data = msg_data;
	req.msg.data_len = input_length;

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (rv) {
	   printf(" Error getting SD Card Extended info, ");
	   if (rv < 0) printf("no response\n");
	   else printf("Completion Code 0x%02x %s\n",rv,decode_cc(0,rv));
	   return rv;
	}

	sdcardinfoblock = (IPMI_DELL_SDCARD_INFO *) (void *) rsp;

	if( (iDRAC_FLAG == IDRAC_12G) && (sdcardinfoblock->vflashcompcode == VFL_NOT_LICENSED))
	{
		printf("FM001 : A required license is missing or expired\n");
		return -1;	
	}
	else if (sdcardinfoblock->vflashcompcode != 0x00)
	{
		lprintf(LOG_ERR, " Error in getting SD Card Extended Information (%s) \n", get_vFlash_compcode_str(sdcardinfoblock->vflashcompcode,
					vFlash_completion_code_vals));
		return -1;
	}

	if (!(sdcardinfoblock->sdcardstatus & 0x04))
	{
		lprintf(LOG_ERR, " vFlash SD card is unavailable, please insert the card\n of size 256MB or greater\n");
		return 0;
	}

	printf("vFlash SD Card Properties\n");
	printf("SD Card size       : %8dMB\n",sdcardinfoblock->sdcardsize);
	printf("Available size     : %8dMB\n",sdcardinfoblock->sdcardavailsize);
	printf("Initialized        : %10s\n", (sdcardinfoblock->sdcardstatus & 0x80) ?
			"Yes" : "No");
	printf("Licensed           : %10s\n", (sdcardinfoblock->sdcardstatus & 0x40) ?
			"Yes" : "No");
	printf("Attached           : %10s\n", (sdcardinfoblock->sdcardstatus & 0x20) ?
			"Yes" : "No");
	printf("Enabled            : %10s\n", (sdcardinfoblock->sdcardstatus & 0x10) ?
			"Yes" : "No");
	printf("Write Protected    : %10s\n", (sdcardinfoblock->sdcardstatus & 0x08) ?
			"Yes" : "No");
	cardstatus = sdcardinfoblock->sdcardstatus & 0x03;
	printf("Health             : %10s\n", ((0x00 == cardstatus
		) ? "OK" : ((cardstatus == 0x03) ? 
			"Undefined" : ((cardstatus == 0x02) ? 
				"Critical" : "Warning"))));
	printf("Bootable partition : %10d\n",sdcardinfoblock->bootpartion);
	return 0;
}

/*****************************************************************
* Function Name: 	ipmi_delloem_vFlash_process
*
* Description: 	This function processes the args for vFlash subcmd
* Input : intf - ipmi interface, arg index, argv array
* Output: prints help or error with help
* Return: 0 - Success -1 - failure
*
******************************************************************/
static int
ipmi_delloem_vFlash_process(void* intf, int current_arg, char ** argv) 
{
	int rc = 0;
	int drv;

	drv = get_driver_type();
	if (drv != DRV_MV) /* MV open driver */
	{
		lprintf(LOG_ERR, " vFlash support is enabled only for wmi and open interface.\n Its not enabled for lan and lanplus interface.");
		return -1;
	}

	if (argv[current_arg] == NULL || strcmp(argv[current_arg], "help") == 0)
	{
		ipmi_vFlash_usage();
		return 0;
	}
	ipmi_idracvalidator_command(intf);
	if (!strncmp(argv[current_arg], "info\0", 5))
	{
		current_arg++;
		if (argv[current_arg] == NULL)
		{
			ipmi_vFlash_usage();
			return -1;
		}
		else if (strncmp(argv[current_arg], "Card\0", 5) == 0)
		{
			current_arg++;
			if (argv[current_arg] != NULL)
			{
				ipmi_vFlash_usage();
				return -1;
			}
			rc = ipmi_get_sd_card_info(intf);
			return rc;
		}
		else /* TBD: many sub commands are present */
		{
			ipmi_vFlash_usage();
			return -1;
		}
	}
	/* TBD other vFlash subcommands */
	else
	{
		ipmi_vFlash_usage();
		return -1;
	}
	return(rc);
}

/*****************************************************************
* Function Name: 	ipmi_vFlash_usage
*
* Description: 	This function displays the usage for using vFlash
* Input : void
* Output: prints help		
* Return: void	
*
******************************************************************/
static void
ipmi_vFlash_usage(void)
{
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "   vFlash info Card");
	lprintf(LOG_NOTICE, "      Shows Extended SD Card information");
	lprintf(LOG_NOTICE, "");
}
/*****************************************************************
* Function Name:       ipmi_delloem_windbg_main
*
* Description:         This function processes the delloem windbg command
* Input:               intf    - ipmi interface
                       argc    - no of arguments
                       argv    - argument string array
* Output:        
*
* Return:              return code     0 - success
*                         -1 - failure
*
******************************************************************/

static int ipmi_delloem_windbg_main (void * intf, int  argc, char ** argv)
{
    int rc = 0;

	current_arg++;
	if (argv[current_arg] == NULL)
	{
		ipmi_windbg_usage();
		return -1;
	}		
	if (strncmp(argv[current_arg], "start\0", 6) == 0)
	{
		rc = ipmi_windbg_start(intf);
	}
	else if (strncmp(argv[current_arg], "end\0", 4) == 0)
	{
		rc = ipmi_windbg_end(intf);
	}
	else
	{
		ipmi_windbg_usage();
	}
	return(rc);
}

/*****************************************************************
* Function Name: 	ipmi_windbg_start 
*
* Description: 	This function Starts the windbg
* Input : void
* Output: Start the debug		
* Return: void	
*
******************************************************************/
static int
ipmi_windbg_start (void * intf)
{
	int rc;
	lprintf(LOG_NOTICE, "Issuing sol activate");
	lprintf(LOG_NOTICE, "");
	
	rc = ipmi_sol_activate(intf,0,0);
	if (rc) lprintf(LOG_NOTICE, "Can not issue sol activate");
	else windbgsession = 1;
	return(rc);
}

/*****************************************************************
* Function Name: 	ipmi_windbg_end
*
* Description: 	This function ends the windbg
* Input : void
* Output: End the debug		
* Return: void	
*
******************************************************************/

static int
ipmi_windbg_end(void * intf)
{
	int rc;
	lprintf(LOG_NOTICE, "Issuing sol deactivate");
	lprintf(LOG_NOTICE, "");
	rc = ipmi_sol_deactivate(intf);
	if (rc) lprintf(LOG_NOTICE, "Can not issue sol deactivate");
	else windbgsession = 0;
	return(rc);	
}


/*****************************************************************
* Function Name: 	ipmi_windbg_usage
*
* Description: 	This function displays the usage for using windbg
* Input : void
* Output: prints help		
* Return: void	
*
******************************************************************/

static void
ipmi_windbg_usage(void)
{
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "   windbg start");
	lprintf(LOG_NOTICE, "      Starts the windbg session (Cold Reset & SOL Activation)");
	lprintf(LOG_NOTICE, "");
	lprintf(LOG_NOTICE, "   windbg end");
	lprintf(LOG_NOTICE, "      Ends the windbg session (SOL Deactivation");
	lprintf(LOG_NOTICE, "");
}



/**********************************************************************
* Function Name: ipmi_setled_usage
*
* Description:  This function prints help message for setled command
* Input:
* Output:
*
* Return:
*
***********************************************************************/
static void
ipmi_setled_usage(void)
{
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   setled <b:d.f> <state..>");
    lprintf(LOG_NOTICE, "      Set backplane LED state");
    lprintf(LOG_NOTICE, "      b:d.f = PCI Bus:Device.Function of drive (lspci format)");
    lprintf(LOG_NOTICE, "      state = present|online|hotspare|identify|rebuilding|");
    lprintf(LOG_NOTICE, "              fault|predict|critical|failed");
    lprintf(LOG_NOTICE, "");
}

static void
ipmi_delloem_getled_usage(void)
{
    lprintf(LOG_NOTICE, "");
    lprintf(LOG_NOTICE, "   getled ");
    lprintf(LOG_NOTICE, "      Get Chassis ID LED state");
    lprintf(LOG_NOTICE, "");
}

static int
IsSetLEDSupported(void)
{
    return SetLEDSupported;
}

static int
CheckSetLEDSupport(void * intf)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[10];

    SetLEDSupported = 0;
    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;
    req.msg.cmd = 0xD5; 		/* Storage */
    req.msg.data_len = sizeof(data);    /*10*/
    req.msg.data = data;

    memset(data, 0, sizeof(data));
    data[0] = 0x01;                        // get
    data[1] = 0x00;			   // subcmd:get firmware version
    data[2] = 0x08;                        // length lsb
    data[3] = 0x00;			   // length msb
    data[4] = 0x00;			   // offset lsb
    data[5] = 0x00;			   // offset msb
    data[6] = 0x00;			   // bay id
    data[7] = 0x00;		
    data[8] = 0x00;
    data[9] = 0x00;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv == 0) SetLEDSupported = 1;
    return(rv);
}

/*****************************************************************
* Function Name:    ipmi_getdrivemap
*
* Description:      This function returns mapping of BDF to Bay:Slot
* Input:            intf         - ipmi interface
*		    bdf	 	 - PCI Address of drive
*		    *bay	 - Returns bay ID
		    *slot	 - Returns slot ID
* Output:           
*
* Return:          
*
******************************************************************/
static int
ipmi_getdrivemap(void * intf, int b, int d, int f, int *bay, int *slot)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[8];

    /* Get mapping of BDF to bay:slot */
    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;
    req.msg.cmd = 0xD5;
    req.msg.data_len = 8;
    req.msg.data = data;

    memset(data, 0, sizeof(data));
    data[0] = 0x01;		// get
    data[1] = 0x07;		// storage map
    data[2] = 0x06;		// length lsb
    data[3] = 0x00;		// length msb
    data[4] = 0x00;		// offset lsb
    data[5] = 0x00;		// offset msb
    data[6] = b;		// bus
    data[7] = (d << 3) + f;	// devfn

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error issuing getdrivemap command, ");
	if (rv < 0) printf("no response\n");
	else printf("Completion Code 0x%02x %s\n", rv, decode_cc(0,rv));
	return rv;
    }

    *bay = rsp[7];
    *slot = rsp[8];
    if (*bay == 0xFF || *slot == 0xFF)
    {
	lprintf(LOG_ERR, "Error could not get drive bay:slot mapping");
	return -1;
    }
    return 0;
}

/*****************************************************************
* Function Name:    ipmi_setled_state
*
* Description:      This function updates the LED on the backplane
* Input:            intf         - ipmi interface
*		    bdf	 	 - PCI Address of drive
*		    state	 - SES Flags state of drive
* Output:           
*
* Return:          
*
******************************************************************/
static int
ipmi_setled_state (void * intf, int bayId, int slotId, int state)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[20];

    /* Issue Drive Status Update to bay:slot */
    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;
    req.msg.cmd = 0xD5;
    req.msg.data_len = 20;
    req.msg.data = data;

    memset(data, 0, sizeof(data));
    data[0] = 0x00;		// set
    data[1] = 0x04;		// set drive status
    data[2] = 0x0e;		// length lsb
    data[3] = 0x00;		// length msb
    data[4] = 0x00;		// offset lsb
    data[5] = 0x00;		// offset msb
    data[6] = 0x0e;		// length lsb
    data[7] = 0x00;		// length msb
    data[8] = bayId;		// bayid
    data[9] = slotId;		// slotid
    data[10] = state & 0xff;	// state LSB
    data[11] = state >> 8;	// state MSB;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error issuing setled command, ");
	if (rv < 0) printf("no response\n");
	else printf("Completion Code 0x%02x %s\n", rv, decode_cc(0,rv));
	return rv;
    }

    return 0;
}

int ipmi_delloem_getled_state (void * intf, uint8_t *state)
{
    uint8_t rsp[IPMI_RSPBUF_SIZE]; int rsp_len, rv;
    struct ipmi_rq req;
    uint8_t data[2];
    uint8_t led_state = 0;

    req.msg.netfn = IPMI_DELL_OEM_NETFN;
    req.msg.lun = 0;
    req.msg.cmd = GET_CHASSIS_LED_STATE;
    req.msg.data_len = 0;
    req.msg.data = data;

    rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
    if (rv) {
	printf(" Error issuing getled command, ");
	if (rv < 0) printf("no response\n");
	else printf("Completion Code 0x%02x %s\n", rv, decode_cc(0,rv));
    } else {
        led_state = rsp[0];  
    }
    *state = led_state;
    return rv;
}

/*****************************************************************
* Function Name:    ipmi_getsesmask
*
* Description:      This function calculates bits in SES drive update
* Return:           Mask set with bits for SES backplane update
*
******************************************************************/
static int ipmi_getsesmask(int  argc, char **argv)
{
	int mask = 0;
	//int idx;
	
	while (current_arg < argc) {
		if (!strcmp(argv[current_arg], "present"))
			mask |= (1L << 0);
		if (!strcmp(argv[current_arg], "online"))
			mask |= (1L << 1);
		if (!strcmp(argv[current_arg], "hotspare"))
			mask |= (1L << 2);
		if (!strcmp(argv[current_arg], "identify"))
			mask |= (1L << 3);
		if (!strcmp(argv[current_arg], "rebuilding"))
			mask |= (1L << 4);
		if (!strcmp(argv[current_arg], "fault"))
			mask |= (1L << 5);
		if (!strcmp(argv[current_arg], "predict"))
			mask |= (1L << 6);
		if (!strcmp(argv[current_arg], "critical"))
			mask |= (1L << 9);
		if (!strcmp(argv[current_arg], "failed"))
			mask |= (1L << 10);
		current_arg++;
	}
	return mask;
}

/*****************************************************************
* Function Name:       ipmi_delloem_setled_main
*
* Description:         This function processes the delloem setled command
* Input:               intf    - ipmi interface
                       argc    - no of arguments
                       argv    - argument string array
* Output:        
*
* Return:              return code     0 - success
*                         -1 - failure
*
******************************************************************/
static int
ipmi_delloem_setled_main(void * intf, int  argc, char ** argv)
{
    int rc = -1;
    int b,d,f, mask;
    int bayId, slotId;

    bayId = 0xFF;
    slotId = 0xFF;

    current_arg++;
    if (argc < current_arg) 
    {
        usage();
        return rc;
    }

    /* ipmitool delloem setled info*/
    if (argc == 1 || strcmp(argv[current_arg], "help") == 0)
    {
        ipmi_setled_usage();
	return 0;
    }
    CheckSetLEDSupport (intf);
    if (!IsSetLEDSupported())
    {
        printf("'setled' is not supported on this system.\n");
        return rc;
    }
    else if (sscanf(argv[current_arg], "%*x:%x:%x.%x", &b,&d,&f) == 3) {
        /* We have bus/dev/function of drive */
	current_arg++;
    	ipmi_getdrivemap (intf, b, d, f, &bayId, &slotId);
    }
    else if (sscanf(argv[current_arg], "%x:%x.%x", &b,&d,&f) == 3) {
        /* We have bus/dev/function of drive */
	current_arg++;
    }
    else {
	ipmi_setled_usage();
	return -1;
    }
    /* Get mask of SES flags */	
    mask = ipmi_getsesmask(argc, argv);

    /* Get drive mapping */
    if (ipmi_getdrivemap (intf, b, d, f, &bayId, &slotId))
	return -1;

    /* Set drive LEDs */
    return ipmi_setled_state (intf, bayId, slotId, mask);
}

static int
ipmi_delloem_getled_main(void * intf, int  argc, char ** argv)
{
    int rc = 0;
    uint8_t state;

    if (argc == 1 || strncmp(argv[0], "help\0", 5) == 0) 
    {
        ipmi_delloem_getled_usage();
	return(0);
    } else {
	rc = ipmi_delloem_getled_state (intf, &state);
	if (rc != 0) printf("getled_state error %d\n",rc);
	else {
	   if (state == 0x01) 
	        printf("Chassis ID LED Status = ON\n");
	   else printf("Chassis ID LED Status = off\n");
	}
    }
    return (rc);
}

/*
 * decode_sensor_dell
 * inputs:
 *    sdr     = the SDR buffer
 *    reading = the 3 or 4 bytes of data from GetSensorReading
 *    pstring = points to the output string buffer
 *    slen    = size of the output buffer
 * outputs:
 *    rv     = 0 if this sensor was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    pstring = contains the sensor reading interpretation string (if rv==0)
 */
int decode_sensor_dell(uchar *sdr,uchar *reading,char *pstring, int slen)
{
   int rv = -1;
   uchar stype, evtype;

   if (sdr == NULL || reading == NULL) return(rv);
   if (pstring == NULL || slen == 0) return(rv);
   /* sdr[3] is SDR type: 1=full, 2=compact, 0xC0=oem */
   if (sdr[3] != 0x02) return(rv);  /*return if not compact SDR */
   stype = sdr[12];  /*sensor type*/
   evtype = sdr[13];  /*event type */
   if (stype == 0x02) {  /* Discrete Voltage */
      /* Dell Discrete Voltage is opposite from normal */
      if (evtype == 0x03) {   /*oem interpretation */
         if (reading[2] & 0x01) strncpy(pstring,"OK",slen); 
         else      strncpy(pstring,"Exceeded",slen); /*LimitExceeded*/
         rv = 0;
      }
   }
   return(rv);
}

#define BIT(x)		(1 << x)
#define	SIZE_OF_DESC	128

char * get_dell_evt_desc(uchar *sel_rec, int *psev)
{
	struct sel_event_record * rec = (struct sel_event_record *)sel_rec;
	int data1, data2, data3;
	int code;
	char *desc = NULL;
	
	unsigned char count;
	unsigned char node;
	//unsigned char num;
	unsigned char dimmNum;
	unsigned char dimmsPerNode;
	char          dimmStr[32];
	//char          cardStr[32];
	//char          numStr[32];
	char          tmpdesc[SIZE_OF_DESC];
	static char   rgdesc[SIZE_OF_DESC];
	char*         str;
	unsigned char incr = 0;
	unsigned char i    = 0;
	//unsigned char postCode;
	// struct ipmi_rs *rsp;
	// struct ipmi_rq req;
	char tmpData;
	int version;
        // uint8_t devid[20]; /*usually 16 bytes*/
        uint8_t iver;
	// int rv;

	data1 = rec->sel_type.standard_type.event_data[0];
	data2 = rec->sel_type.standard_type.event_data[1];
	data3 = rec->sel_type.standard_type.event_data[2];
	if ( (rec->sel_type.standard_type.event_type == 0x0B) || 
		(rec->sel_type.standard_type.event_type == 0x6F)  || 
		(rec->sel_type.standard_type.event_type == 0x07)) 
		{
		code = rec->sel_type.standard_type.sensor_type;
		/* BDF or Slot */
		desc = rgdesc;
		memset(desc,0,SIZE_OF_DESC);
		switch (code) {
			case 0x07:
				if( ((data1 & DATA_BYTE2_SPECIFIED_MASK) == 0x80))
				{
					*psev = SEV_CRIT;
					if((data1 & 0x0f) == 0x00) 
						snprintf(desc,SIZE_OF_DESC,"CPU Internal Err | ");
					if((data1 & 0x0f) == 0x06)
					{
						snprintf(desc,SIZE_OF_DESC,"CPU Protocol Err | ");
					}
					/* change bit location to a number */
					for (count= 0; count < 8; count++)
					{
					  if (BIT(count)& data2)
					  {
					    count++;
						if( ((data1 & 0x0f) == 0x06) && (rec->sel_type.standard_type.sensor_num == 0x0A)) 
						    snprintf(desc,SIZE_OF_DESC,"FSB %d ",count);
						else
						    snprintf(desc,SIZE_OF_DESC,"CPU %d | APIC ID %d ",count,data3);
					    break;
					  }
					}
				}
			break;
			case 0x0C:
				if ( (rec->sel_type.standard_type.event_type == 0x0B) && 
						!(data1 & 0x03) ) 
					{
						*psev = SEV_INFO;
						if(data2 & 0x04)
							strcpy(desc,"Memory is in Spare Mode");
						else if(data2 & 0x02)
							strcpy(desc,"Memory is in Raid Mode ");
						else if(data2 & 0x01)
							strcpy(desc,"Memory is in Mirror Mode ");
						break;
					}
			case 0x10:
				get_devid_ver(NULL,NULL,&iver);
		//		rv = ipmi_getdeviceid(devid,sizeof(devid),fdbg);
		//		if (rv != 0) return NULL;
				version = iver;
				/* Memory DIMMS */
				*psev = SEV_MAJ; /*default severity for  DIMM events*/
				if( (data1 &  0x80) || (data1 & 0x20 ) )
				{
					if( (code == 0x0c) && (rec->sel_type.standard_type.event_type == 0x0B) )
					{
						if((data1 & 0x0f) == 0x00) 
						{
							snprintf(desc,SIZE_OF_DESC," Redundancy Regained | ");
							*psev = SEV_INFO;
						}
						else if((data1 & 0x0f) == 0x01)
						{
							snprintf(desc,SIZE_OF_DESC,"Redundancy Lost | ");
							*psev = SEV_MAJ;
						}
					}	
					else if(code == 0x0c) 
					{
						if((data1 & 0x0f) == 0x00)
						{
							if(rec->sel_type.standard_type.sensor_num == 0x1C)
							{
								if((data1 &  0x80) && (data1 & 0x20 ))
								{
									count = 0;
									snprintf(desc,SIZE_OF_DESC,"CRC Error on:");
									for(i=0;i<4;i++)
									{
										if((BIT(i))&(data2))
										{
											if(count)
											{
												str = desc+strlen(desc);
												*str++ = ',';
												 str = '\0';
						              			count = 0;
											}
											switch(i)
											{
												case 0: snprintf(tmpdesc,SIZE_OF_DESC,"South Bound Memory");
														strcat(desc,tmpdesc);
														count++;
														break;
												case 1:	snprintf(tmpdesc,SIZE_OF_DESC,"South Bound Config");
														strcat(desc,tmpdesc);
														count++;
														break;
												case 2: snprintf(tmpdesc,SIZE_OF_DESC,"North Bound memory");
														strcat(desc,tmpdesc);
														count++;
														break;
												case 3:	snprintf(tmpdesc,SIZE_OF_DESC,"North Bound memory-corr");
														strcat(desc,tmpdesc);
														count++;
														break;
												default:
														break;
											}
										}
									}
									if(data3>=0x00 && data3<0xFF)
									{
										snprintf(tmpdesc,SIZE_OF_DESC,"|Failing_Channel:%d",data3);
										strcat(desc,tmpdesc);
									}
								}
								break;
							}
							snprintf(desc,SIZE_OF_DESC,"Correctable ECC | ");
							*psev = SEV_MAJ;
						}
						else if((data1 & 0x0f) == 0x01) 
						{
							snprintf(desc,SIZE_OF_DESC,"UnCorrectable ECC | ");
							*psev = SEV_CRIT;
						}
					} 
					else if(code == 0x10)
					{
						if((data1 & 0x0f) == 0x00) {
							snprintf(desc,SIZE_OF_DESC,"Corr Memory Log Disabled | ");
							*psev = SEV_INFO;
						}
					}
				} 
				else
				{
					if(code == 0x12) 
					{
						if((data1 & 0x0f) == 0x02) {
							snprintf(desc,SIZE_OF_DESC,"Unknown System Hardware Failure ");
							*psev = SEV_MAJ;
						}
					}
					if(code == 0x10)
					{
						if((data1 & 0x0f) == 0x03) {
							snprintf(desc,SIZE_OF_DESC,"All Even Logging Dissabled");
							*psev = SEV_INFO;
						}
					}
				}
				if(data1 & 0x80 ) 
				{
					if(((data2 >> 4) != 0x0f) && ((data2 >> 4) < 0x08))
					{
						tmpData = 	('A'+ (data2 >> 4));
						if( (code == 0x0c) && (rec->sel_type.standard_type.event_type == 0x0B) )
						{
							snprintf(tmpdesc, SIZE_OF_DESC, "Bad Card %c", tmpData);								
						}
						else
						{
							snprintf(tmpdesc, SIZE_OF_DESC, "Card %c", tmpData);
						}
						strcat(desc, tmpdesc);
					}
					if (0x0F != (data2 & 0x0f)) 
					{
						if(0x51  == version)
						{
							snprintf(tmpdesc, SIZE_OF_DESC, "Bank %d", ((data2 & 0x0f)+1));	
							strcat(desc, tmpdesc);
						}
						else 
						{
							incr = (data2 & 0x0f) << 3;
						}
					}
					
				}
				if(data1 & 0x20 )
				{
					strcat(desc, "ECC Error,");
					if(0x51  == version) 
					{
						snprintf(tmpdesc, SIZE_OF_DESC, "DIMM %c", (char)('A'+ data3));
						strcat(desc, tmpdesc);
					} 
					else if( ((data2 >> 4) > 0x07) && ((data2 >> 4) != 0x0F)) 
					{
						strcpy(dimmStr, " DIMM_");
						str = desc+strlen(desc);
						dimmsPerNode = 4;
						if( (data2 >> 4) == 0x09) dimmsPerNode = 6;
						else if( (data2 >> 4) == 0x0A) dimmsPerNode = 8;
						else if( (data2 >> 4) == 0x0B) dimmsPerNode = 9;
						else if( (data2 >> 4) == 0x0C) dimmsPerNode = 12;
						else if( (data2 >> 4) == 0x0D) dimmsPerNode = 24;	
						else if( (data2 >> 4) == 0x0E) dimmsPerNode = 3;							
						count = 0;
				        for (i = 0; i < 8; i++)
				        {
				          if (BIT(i) & data3)
				          {
				            if (count)
				            {
				              *str++ = ',';
				              count = 0;
				            }
				            node = (incr + i)/dimmsPerNode;
				            dimmNum = ((incr + i)%dimmsPerNode)+1;
				            dimmStr[6] = node + 'A';
				            sprintf(tmpdesc,"%d",dimmNum);
				            dimmStr[7] = tmpdesc[0];
							dimmStr[8] = '\0'; 
							strcat(str,dimmStr);
				            count++;
				          }
				        }
					} 
					else 
					{
				        strcpy(dimmStr, " DIMM");
						str = desc+strlen(desc);
				        count = 0;
				        for (i = 0; i < 8; i++)
				        {
				          if (BIT(i) & data3)
				          {
				            // check if more than one DIMM, if so add a comma to the string.
				            if (count)
				            {
				              *str++ = ',';
				              count = 0;
				            }
				            sprintf(tmpdesc,"%d",(i + incr + 1));
							dimmStr[5] = tmpdesc[0];
							dimmStr[6] = '\0'; 
				            strcat(str, dimmStr);
				            count++;
				          }
				        }
			        }
				}
			break;
			case 0x20:
				if(((data1 & 0x0f)== 0x00)&&((data1 & 0x80) && (data1 & 0x20)))
				{
					*psev = SEV_MAJ;
					if((data2 > 0x00)&&(data2<0xFF))
					{
						//Add the code to display 194 entries.This sensor present only in ORCA
						
					}	
					switch(data3)
					{
						case 0x01:
							snprintf(desc,SIZE_OF_DESC,"BIOS TXT Error");
							break;
						case 0x02:
							snprintf(desc,SIZE_OF_DESC,"Processor/FIT TXT");
							break;
						case 0x03:
							snprintf(desc,SIZE_OF_DESC,"BIOS ACM TXT Error");
							break;
						case 0x04:
							snprintf(desc,SIZE_OF_DESC,"SINIT ACM TXT Error");
							break;
						case 0xff:
							snprintf(desc,SIZE_OF_DESC,"Unrecognized TT Error12");
							break;
						default:
							break;						
					}
				}
			break;	
			case 0x23:
				
				if(data1 == 0xC1)
				{
					if(data2 == 0x04)
					{
						*psev = SEV_CRIT;
						snprintf(desc,SIZE_OF_DESC,"Hard Reset|Interrupt type None,SMS/OS Timer used at expiration");
					}
				}	
				
			break;
			case 0x2B:
				if(((data1 & 0x0f)== 0x02)&&((data1 & 0x80) && (data1 & 0x20)))
				{
					if(data2 == 0x02)
					{
						*psev = SEV_MAJ;
						if(data3 == 0x00)
						{
							snprintf(desc, SIZE_OF_DESC, "between BMC/iDRAC Firmware and other hardware");
						}
						else if(data3 == 0x01)
						{
							snprintf(desc, SIZE_OF_DESC, "between BMC/iDRAC Firmware and CPU");
						}
					}
				}
			break;	
			
			case 0xC1:
				if(rec->sel_type.standard_type.sensor_num == 0x25)
				{
					*psev = SEV_MAJ;
					if((data1 & 0x0f) == 0x01)
					{
						snprintf(desc, SIZE_OF_DESC, "Failed to program Virtual Mac Address");
						if((data1 & 0x80)&&(data1 & 0x20))
						{
							snprintf(tmpdesc, SIZE_OF_DESC, "PCI %.2x:%.2x.%x",
							data3 &0x7f, (data2 >> 3) & 0x1F, 
							data2 & 0x7);
						}
					}
					else if((data1 & 0x0f) == 0x02)
					{
						snprintf(desc, SIZE_OF_DESC, "Device option ROM failed to support link tuning or flex address");
						if((data1 & 0x80)&&(data1 & 0x20))
						{
							//Add Mezzanine code here.DELLOEM SEL displayed unknown event
						}
					}
					else if((data1 & 0x0f) == 0x03)
					{
						snprintf(desc, SIZE_OF_DESC, "Failed to get link tuning or flex address data from BMC/iDRAC");
					}
					strcat(desc,tmpdesc);
				}
			break;	
			case 0x13:
			case 0xC2:
			case 0xC3:	
				if(rec->sel_type.standard_type.sensor_num == 0x29)
				{
					*psev = SEV_MAJ;
					if(((data1 & 0x0f)== 0x02)&&((data1 & 0x80) && (data1 & 0x20)))
					{
					#if 1 /*This sensor is not implemented in iDRAC code*/
						snprintf(tmpdesc, SIZE_OF_DESC, "Partner-(LinkId:%d,AgentId:%d)|",(data2 & 0xC0),(data2 & 0x30));
						strcat(desc,tmpdesc);
						snprintf(tmpdesc, SIZE_OF_DESC, "ReportingAgent(LinkId:%d,AgentId:%d)|",(data2 & 0x0C),(data2 & 0x03));
						strcat(desc,tmpdesc);
						if((data3 & 0xFC) == 0x00)
						{
							snprintf(tmpdesc, SIZE_OF_DESC, "LinkWidthDegraded|");
							strcat(desc,tmpdesc);
						}
						if(BIT(1)& data3)
						{
							snprintf(tmpdesc,SIZE_OF_DESC,"PA_Type:IOH|");
						}
						else
						{
							snprintf(tmpdesc,SIZE_OF_DESC,"PA-Type:CPU|");
						}
						strcat(desc,tmpdesc);
						if(BIT(0)& data3)
						{
							snprintf(tmpdesc,SIZE_OF_DESC,"RA-Type:IOH");
						}
						else
						{
							snprintf(tmpdesc,SIZE_OF_DESC,"RA-Type:CPU");
						}
						strcat(desc,tmpdesc);
					#endif	
						*psev = SEV_MAJ;
					}
				}
				else
				{
					*psev = SEV_MAJ;
					if((data1 & 0x0f) == 0x02) 
					{
						sprintf(desc,"%s","IO channel Check NMI");
					}
					else 
					{
						if((data1 & 0x0f) == 0x00)
						{
							snprintf(desc, SIZE_OF_DESC, "%s","PCIe Error |"); 
						}
						else if((data1 & 0x0f) == 0x01) 
						{
							snprintf(desc, SIZE_OF_DESC, "%s","I/O Error |"); 
						}
						else if((data1 & 0x0f) == 0x04) 
						{
							snprintf(desc, SIZE_OF_DESC, "%s","PCI PERR |"); 
						} 
						else if((data1 & 0x0f) == 0x05)
						{
							snprintf(desc, SIZE_OF_DESC, "%s","PCI SERR |");
						} 
						else 
						{
							snprintf(desc, SIZE_OF_DESC, "%s"," ");
						}
						if (data3 & 0x80)
							snprintf(tmpdesc, SIZE_OF_DESC, "Slot %d", data3 & 0x7f);
						else
							snprintf(tmpdesc, SIZE_OF_DESC, "PCI %.2x:%.2x.%x",
							data3 &0x7f, (data2 >> 3) & 0x1F, 
							data2 & 0x7);
						strcat(desc,tmpdesc);
					}
					*psev = SEV_CRIT;
				}
			break;	
			case 0x0F:	
				if(((data1 & 0x0f)== 0x0F)&&(data1 & 0x80))
				{
					*psev = SEV_CRIT;
					switch(data2)
					{
						case 0x80:
							snprintf(desc, SIZE_OF_DESC, "No memory is detected.");break;
						case 0x81:	
							snprintf(desc,SIZE_OF_DESC, "Memory is detected but is not configurable.");break;
						case 0x82:
							snprintf(desc, SIZE_OF_DESC, "Memory is configured but not usable.");break;
						case 0x83:
							snprintf(desc, SIZE_OF_DESC, "System BIOS shadow failed.");break;
						case 0x84:	
							snprintf(desc, SIZE_OF_DESC, "CMOS failed.");break;
						case 0x85:	
							snprintf(desc, SIZE_OF_DESC, "DMA controller failed.");break;
						case 0x86:	
							snprintf(desc, SIZE_OF_DESC, "Interrupt controller failed.");break;
						case 0x87:
							snprintf(desc, SIZE_OF_DESC, "Timer refresh failed.");break;
						case 0x88:
							snprintf(desc, SIZE_OF_DESC, "Programmable interval timer error.");break;
						case 0x89:
							snprintf(desc, SIZE_OF_DESC, "Parity error.");break;
						case 0x8A:
							snprintf(desc, SIZE_OF_DESC, "SIO failed.");break;
						case 0x8B:	
							snprintf(desc, SIZE_OF_DESC, "Keyboard controller failed.");break;
						case 0x8C:	
							snprintf(desc, SIZE_OF_DESC, "System management interrupt initialization failed.");break;
						case 0x8D:	
							snprintf(desc, SIZE_OF_DESC, "TXT-SX Error.");break;
						case 0xC0:
							snprintf(desc, SIZE_OF_DESC, "Shutdown test failed.");break;
						case 0xC1:	
							snprintf(desc, SIZE_OF_DESC, "BIOS POST memory test failed.");break;
						case 0xC2:	
							snprintf(desc, SIZE_OF_DESC, "RAC configuration failed.");break;
						case 0xC3:	
							snprintf(desc, SIZE_OF_DESC, "CPU configuration failed.");break;
						case 0xC4:	
							snprintf(desc, SIZE_OF_DESC, "Incorrect memory configuration.");break;
						case 0xFE:	
							snprintf(desc, SIZE_OF_DESC, "General failure after video.");
							break;
					}
				}
			break;
			
			default:
			break;				
		} 
		if (desc[0] == 0) { /* if no description specified above */
			/* snprintf(desc,SIZE_OF_DESC,"%02x [%02x %02x %02x]",
					code,data1,date2,data3); // show raw data */
			desc = NULL;  /*if empty, handle with default logic*/
		}
	}
	else
	{
		code = rec->sel_type.standard_type.event_type;
	}
	return desc;
}

/* 
 * decode_sel_dell
 * inputs:
 *    evt    = the 16-byte IPMI SEL event
 *    outbuf = points to the output string buffer
 *    outsz  = size of the output buffer
 * outputs: 
 *    rv     = 0 if this event was successfully interpreted here,
 *             non-zero otherwise, to use default interpretations.
 *    outbuf = will contain the interpreted event text string (if rv==0)
 */
int decode_sel_dell(uint8_t *evt, char *outbuf, int outsz, char fdesc, 
			char fdbg)
{
   int rv = -1;
   uint16_t id, genid;
   uint8_t rectype;
   uint32_t timestamp;
   char *type_str = NULL;
   char *gstr = NULL;
   char *pstr = NULL;
   int sevid;
   uchar stype, snum;

   fdebug = fdbg;
   sevid = SEV_INFO;
   id = evt[0] + (evt[1] << 8);
   rectype = evt[2];
   timestamp = evt[3] + (evt[4] << 8) + (evt[5] << 16) + (evt[6] << 24);
   genid = evt[7] | (evt[8] << 8);
   stype = evt[10];
   snum = evt[11];
   gstr = "BMC ";
   if (genid == 0x0033) gstr = "Bios";

#ifdef OTHER
   /* evt[13] is data1/offset*/
   if ( ((evt[13] & DATA_BYTE2_SPECIFIED_MASK) == 0x80) ||
        ((evt[13] & DATA_BYTE3_SPECIFIED_MASK) == 0x20) ) {
	      // if (evt[13] & DATA_BYTE2_SPECIFIED_MASK)
	 	 // evt->data = rec->sel_type.standard_type.event_data[1];
              pstr = get_dell_evt_desc(evt,&sevid);
    } else if (evt[13] == 0xC1) {
        if (snum == 0x23) {
              //     evt->data = rec->sel_type.standard_type.event_data[1];
              pstr = get_dell_evt_desc(evt,&sevid);
	}
    }    
#endif
   pstr = get_dell_evt_desc(evt,&sevid);
   if (pstr != NULL) rv = 0;

   if (rv == 0) {
	type_str = "";
	if (rectype == 0x02) type_str = get_sensor_type_desc(stype);
	format_event(id,timestamp, sevid, genid, type_str,
			snum,NULL,pstr,NULL,outbuf,outsz);
   }
   return rv;
}

#ifdef METACOMMAND
int i_delloem(int argc, char **argv)
#else
#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char **argv)
#endif
{
   int rv = 0;
   uchar devrec[16];
   int c, i;
   char *s1;

   printf("%s ver %s\n", progname,progver);
   set_loglevel(LOG_NOTICE);
   argc_sav = argc;
   argv_sav = argv;
   parse_lan_options('V',"4",0);  /*default to admin priv*/

   while ( (c = getopt( argc, argv,"m:s:xzEF:J:N:P:R:T:U:V:YZ:?")) != EOF )
      switch(c) {
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
          case 's': sdrfile = optarg; break;
          case 'x': fdebug = 2; /* normal (dbglog if isol) */
                    verbose = 1;
                    break;
          case 'z': fdebug = 3; /*full debug (for isol)*/
                    verbose = 1;
                    break;
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
		usage();
                rv = ERR_USAGE;
                goto do_exit;
                break;
      }
   rv = ipmi_getdeviceid(devrec,16,fdebug);
   if (rv == 0) {
      char ipmi_maj, ipmi_min;
      ipmi_maj = devrec[4] & 0x0f;
      ipmi_min = devrec[4] >> 4;
      // vend_id = devrec[6] + (devrec[7] << 8) + (devrec[8] << 16);
      // prod_id = devrec[9] + (devrec[10] << 8);
      show_devid( devrec[2],  devrec[3], ipmi_maj, ipmi_min);
   }
   for (i = 0; i < optind; i++) { argv++; argc--; }

   rv = ipmi_delloem_main(NULL, argc, argv);

do_exit:
   ipmi_close_();
   return(rv);
}
/* end oem_dell.c */
