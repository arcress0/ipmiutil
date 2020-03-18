/*
 * ipmilan2.c
 * 
 * Interface to call libintf_lanplus from ipmitool to do RMCP+ protocol.
 *
 * 01/09/07 Andy Cress - created
 * 02/22/07 Andy Cress - initialize cipher_suite to 3 (was 0)
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#if defined(LINUX) || defined(BSD)
#include <sys/time.h>
#endif

#undef HAVE_LANPLUS 

// #define DEBUG  1
#ifndef HAVE_LANPLUS
/* No lanplus, so stub these functions returning errors. */
#define uchar  unsigned char
#define ushort  unsigned short
#define LAN_ERR_INVPARAM  -8
#define LOG_WARN  4
#define LOG_MSG_LENGTH 1024  /*usu. ipmicmd.h*/
int   verbose = 0;  
char  fdbglog = 0;
void set_loglevel(int level);
void lprintf(int level, const char * format, ...);

int ipmi_open_lan2(char *node, char *user, char *pswd, int fdebugcmd)
{ if (fdebugcmd) verbose = 1; return(LAN_ERR_INVPARAM); }

int ipmi_close_lan2(char *node)
{ return(LAN_ERR_INVPARAM); }

int ipmi_cmdraw_lan2(char *node, uchar cmd, uchar netfn, uchar lun, 
		uchar sa, uchar bus, uchar *pdata, int sdata,
                uchar *presp, int *sresp, uchar *pcc, char fdebugcmd)
{ return(LAN_ERR_INVPARAM); }

int ipmi_cmd_lan2(char *node, ushort cmd, uchar *pdata, int sdata,
                uchar *presp, int *sresp, uchar *pcc, char fdebugcmd)
{ return(LAN_ERR_INVPARAM); }

int lan2_send_sol( uchar *payload, int len, void *rsp)
{ return(LAN_ERR_INVPARAM); }
int lan2_recv_sol( void *rsp )
{ return(LAN_ERR_INVPARAM); }
int lan2_keepalive(int type, void *rsp)
{ return(LAN_ERR_INVPARAM); }
void lan2_recv_handler( void *rs )
{ return; }
void lan2_set_sol_data(int insize, int outsize, int port, void *handler,
			char esc_char)
{ return; }
int lan2_get_fd(void) { return(1); }
void lanplus_set_recvdelay( int delay ) { return; }
long lan2_get_latency( void ) { return(1); }
int lan2_send_break( void *rsp) { return(LAN_ERR_INVPARAM); }
int lan2_send_ctlaltdel( void *rsp) { return(LAN_ERR_INVPARAM); }

#else /* else HAVE_LANPLUS is defined */

#endif

/* end ipmilan2.c */
