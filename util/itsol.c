/*
 * Copyright (c) 2005 Tyan Computer Corp.  All Rights Reserved.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#include <io.h>
#include <time.h>
#include "getopt.h"
#define uint8_t   unsigned char
#define uint16_t  unsigned short
#define uint32_t  unsigned int
typedef uint32_t    socklen_t;
#define inet_ntop   InetNtop
#define inet_pton   InetPton
int   InetPton(int af, char *pstr, void *addr); /*see ws2_32.dll*/
char *InetNtop(int af, void *addr, char *pstr, int sz);
char *InetNtop(int af, void *addr, char *pstr, int sz) { return NULL; };
int   gettimeofday(struct timeval *tv, struct timezone *tz);
#undef POLL_OK

#else
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if defined(HPUX)
/* getopt is defined in stdio.h */
#undef POLL_OK
#elif defined(MACOS)
/* getopt is defined in unistd.h */
#include <unistd.h>
#include <stdint.h>
#undef POLL_OK
#else
#include <getopt.h>
#include <sys/poll.h>
#define POLL_OK 1
#endif

#if defined (HAVE_SYS_TERMIOS_H)
#include <sys/termios.h>
#else
// #if defined(HAVE_TERMIOS_H)
#include <termios.h>
#endif

#endif

#include "ipmicmd.h"
#include "itsol.h"

extern int verbose;
extern char   fdebug;  /*from ipmicmd.c*/
#ifdef METACOMMAND
extern char * progver;  /*from ipmiutil.c*/
static char * progname  = "ipmiutil tsol";
#else
static char * progver   = "3.08";
static char * progname  = "itsol";
#endif
static uchar  g_bus  = PUBLIC_BUS;
static uchar  g_sa   = BMC_SA;
static uchar  g_lun  = BMC_LUN;
static uchar  g_addrtype = ADDR_SMI;
static char hostname[SZGNODE]; 
static SockType sockfd = -1;
static char  sol_escape_char = '~';
static struct timeval _start_keepalive;
static int _in_raw_mode = 0;
static int _altterm = 0;
static SOCKADDR_T haddr;
static int haddrlen = 0;
static int hauth, hpriv, hcipher;
#ifdef WIN32
// #define NI_MAXHOST 80
// #define NI_MAXSERV 80
// struct pollfd { int fd; short events; short revents; };
struct winsize { int x; int y; };
#else
static struct termios _saved_tio;
static struct winsize _saved_winsize;
#endif


static int
ipmi_tsol_command(void * intf, char *recvip, int port, unsigned char cmd)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
        struct ipmi_rq	req;
        unsigned char	data[6];
	unsigned	ip1, ip2, ip3, ip4;

        if (sscanf(recvip, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4) != 4) {
                lprintf(LOG_ERR, "Invalid IP address: %s", recvip);
                return -1;
        }

	memset(&req, 0, sizeof(struct ipmi_rq));
        req.msg.netfn    = IPMI_NETFN_TSOL;
        req.msg.cmd      = cmd;
        req.msg.data_len = 6;
        req.msg.data     = data;

	memset(data, 0, sizeof(data));
        data[0] = ip1;
        data[1] = ip2;
        data[2] = ip3;
        data[3] = ip4;
        data[4] = (port & 0xff00) >> 8;
        data[5] = (port & 0xff);

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
        if (rv < 0) {
		lprintf(LOG_ERR, "Unable to perform TSOL command");
                return rv;
	}
	if (rv > 0) {
		lprintf(LOG_ERR, "Unable to perform TSOL command: %s",
			decode_cc(0,rv));
		return -1;
        }

        return 0;
}

static int
ipmi_tsol_start(void * intf, char *recvip, int port)
{
	return ipmi_tsol_command(intf, recvip, port, IPMI_TSOL_CMD_START);
}

static int
ipmi_tsol_stop(void * intf, char *recvip, int port)
{
        return ipmi_tsol_command(intf, recvip, port, IPMI_TSOL_CMD_STOP);
}

static int
ipmi_tsol_send_keystroke(void * intf, char *buff, int length)
{
	uint8_t rsp[IPMI_RSPBUF_SIZE];
	int rsp_len, rv;
        struct ipmi_rq   req;
        unsigned char    data[16];
	static unsigned char keyseq = 0;

	memset(&req, 0, sizeof(struct ipmi_rq));
        req.msg.netfn    = IPMI_NETFN_TSOL;
        req.msg.cmd      = IPMI_TSOL_CMD_SENDKEY;
        req.msg.data_len = length + 2;
        req.msg.data     = data;

	memset(data, 0, sizeof(data));
        data[0] = length + 1;
	memcpy(data + 1, buff, length);
	data[length + 1] = keyseq++;

	rv = ipmi_sendrecv(&req, &rsp[0], &rsp_len);
	if (verbose) {
		if (rv < 0) {
			lprintf(LOG_ERR, "Unable to send keystroke");
			return rv;
		}
		if (rv > 0) {
			lprintf(LOG_ERR, "Unable to send keystroke: %s",
				decode_cc(0,rv));
			return -1;
		}
	}

        return length;
}

static int
tsol_keepalive(void * intf)
{
        struct timeval end;
	int rv;

        gettimeofday(&end, 0);

        if (end.tv_sec - _start_keepalive.tv_sec <= 30)
                return 0;

        // intf->keepalive(intf);  
	rv = lan_keepalive(1);  /*1=getdevid, 2=empty sol*/

	gettimeofday(&_start_keepalive, 0);

        return rv;
}

static void
print_escape_seq(void *intf)
{
	lprintf(LOG_NOTICE,
		"       %c.  - terminate connection\r\n"
		"       %c^Z - suspend ipmiutil\r\n"
		"       %c^X - suspend ipmiutil, but don't restore tty on restart\r\n"
		"       %c?  - this message\r\n"
		"       %c%c  - send the escape character by typing it twice\r\n"
		"       (Note that escapes are only recognized immediately after newline.)\r",
		sol_escape_char,
		sol_escape_char,
		sol_escape_char,
		sol_escape_char,
		sol_escape_char,
		sol_escape_char);
}

#ifdef WIN32
static int leave_raw_mode(void) { return(0); }
static int enter_raw_mode(void) { return(0); }
static void set_terminal_size(int rows, int cols) { return; }
static void do_terminal_cleanup(void) { 
	int err;
	err =  get_LastError();
	lprintf(LOG_ERR, "Exiting due to error %d", err);
	show_LastError("",err);
}
static void suspend_self(int restore_tty) { return; }
#else
static int
leave_raw_mode(void)
{
	if (!_in_raw_mode)
		return -1;
	else if (tcsetattr(fileno(stdin), TCSADRAIN, &_saved_tio) == -1)
		lperror(LOG_ERR, "tcsetattr(stdin)");
	else if (tcsetattr(fileno(stdout), TCSADRAIN, &_saved_tio) == -1)
		lperror(LOG_ERR, "tcsetattr(stdout)");
	else
		_in_raw_mode = 0;

	return 0;
}

static int
enter_raw_mode(void)
{
	struct termios tio;

	if (tcgetattr(fileno(stdout), &_saved_tio) < 0) {
		lperror(LOG_ERR, "tcgetattr failed");
		return -1;
	}

	tio = _saved_tio;

	if (_altterm) {
		tio.c_iflag &= (ISTRIP | IGNBRK );
		tio.c_cflag &= ~(CSIZE | PARENB | IXON | IXOFF | IXANY);
		tio.c_cflag |= (CS8 |CREAD) | (IXON|IXOFF|IXANY);
		tio.c_lflag &= 0;
		tio.c_cc[VMIN] = 1;
		tio.c_cc[VTIME] = 0;
	} else {
		tio.c_iflag |= IGNPAR;
		tio.c_iflag &= ~(ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXANY | IXOFF);
		tio.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL | IEXTEN);
		tio.c_oflag &= ~OPOST;
		tio.c_cc[VMIN] = 1;
		tio.c_cc[VTIME] = 0;
	}

	if (tcsetattr(fileno(stdin), TCSADRAIN, &tio) < 0)
		lperror(LOG_ERR, "tcsetattr(stdin)");
	else if (tcsetattr(fileno(stdout), TCSADRAIN, &tio) < 0)
		lperror(LOG_ERR, "tcsetattr(stdout)");
	else
		_in_raw_mode = 1;

	return 0;
}

static void
do_terminal_cleanup(void)
{
	if (_saved_winsize.ws_row > 0 && _saved_winsize.ws_col > 0)
		ioctl(fileno(stdout), TIOCSWINSZ, &_saved_winsize);

	leave_raw_mode();

	if (errno)
		lprintf(LOG_ERR, "Exiting due to error %d -> %s",
			errno, strerror(errno));
}

static void
set_terminal_size(int rows, int cols)
{
	struct winsize winsize;

	if (rows <= 0 || cols <= 0)
		return;

	/* save initial winsize */
	ioctl(fileno(stdout), TIOCGWINSZ, &_saved_winsize);

	/* set new winsize */
	winsize.ws_row = rows;
	winsize.ws_col = cols;
	ioctl(fileno(stdout), TIOCSWINSZ, &winsize);
}

static void
suspend_self(int restore_tty)
{
	leave_raw_mode();

	kill(getpid(), SIGTSTP);

	if (restore_tty)
		enter_raw_mode();
}
#endif

static int
do_inbuf_actions(void *intf, char *in_buff, int len)
{
	static int in_esc = 0;
	static int last_was_cr = 1;
	int i;

	for(i = 0; i < len ;) {
		if (!in_esc) {
			if (last_was_cr &&
			    (in_buff[i] == sol_escape_char)) {
				in_esc = 1;
				memmove(in_buff, in_buff + 1, len - i - 1);
				len--;
				continue;
			}
		}
		if (in_esc) {
			if (in_buff[i] == sol_escape_char) {
				in_esc = 0;
				i++;
				continue;
			}

			switch (in_buff[i]) {
			case '.':
				printf("%c. [terminated ipmiutil]\r\n",
				       sol_escape_char);
				return -1;

			case 'Z' - 64:
				printf("%c^Z [suspend ipmiutil]\r\n",
				       sol_escape_char);
				suspend_self(1); /* Restore tty back to raw */
				break;

			case 'X' - 64:
				printf("%c^X [suspend ipmiutil]\r\n",
				       sol_escape_char);
				suspend_self(0); /* Don't restore to raw mode */
				break;

			case '?':
				printf("%c? [ipmiutil help]\r\n",
				       sol_escape_char);
				print_escape_seq(intf);
				break;
			}

			memmove(in_buff, in_buff + 1, len - i - 1);
			len--;
			in_esc = 0;

			continue;
		}

		last_was_cr = (in_buff[i] == '\r' || in_buff[i] == '\n');

		i++;
	}

	return len;
}


static void
print_tsol_usage(void)
{
	lprintf(LOG_NOTICE, "Usage: tsol [recvip] [port=NUM] [ro|rw] [rows=NUM] [cols=NUM] [altterm]");
	lprintf(LOG_NOTICE, "       recvip       Receiver IP Address             [default=local]");
	lprintf(LOG_NOTICE, "       port=NUM     Receiver UDP Port               [default=%d]",
		IPMI_TSOL_DEF_PORT);
	lprintf(LOG_NOTICE, "       ro|rw        Set Read-Only or Read-Write     [default=rw]");
#ifdef POLL_OK
	{
	  struct winsize winsize;
	  ioctl(fileno(stdout), TIOCGWINSZ, &winsize);
	  lprintf(LOG_NOTICE, "       rows=NUM     Set terminal rows               [default=%d]",
		winsize.ws_row);
	  lprintf(LOG_NOTICE, "       cols=NUM     Set terminal columns            [default=%d]",
		winsize.ws_col);
	}
#endif
	lprintf(LOG_NOTICE, "       altterm      Alternate terminal setup        [default=off]");
}


int
ipmi_tsol_main(void * intf, int  argc, char ** argv)
{
	char *recvip = NULL;
	int result, i;
	int ip1, ip2, ip3, ip4;
	int read_only = 0, rows = 0, cols = 0;
	int port = IPMI_TSOL_DEF_PORT;
#ifdef POLL_OK
	SOCKADDR_T sin, myaddr;
	socklen_t mylen;
	struct pollfd fds_wait[3], fds_data_wait[3], *fds;
	int fd_socket; 
	int out_buff_fill, in_buff_fill;
	char out_buff[IPMI_BUF_SIZE * 8], in_buff[IPMI_BUF_SIZE];
	char buff[IPMI_BUF_SIZE + 4];
	char mystr[NI_MAXHOST];
#endif

	if (! is_remote()) {
		lprintf(LOG_ERR, "Error: Tyan SOL is only available over lan interface");
		return -1;
	}

	for (i = 0; i<argc; i++) {
		if (sscanf(argv[i], "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4) == 4)
			recvip = strdup_(argv[i]);
		else if (sscanf(argv[i], "port=%d", &ip1) == 1)
			port = ip1;
		else if (sscanf(argv[i], "rows=%d", &ip1) == 1)
			rows = ip1;
		else if (sscanf(argv[i], "cols=%d", &ip1) == 1)
			cols = ip1;
		else if (strlen(argv[i]) == 2 && strncmp(argv[i], "ro", 2) == 0)
			read_only = 1;
		else if (strlen(argv[i]) == 2 && strncmp(argv[i], "rw", 2) == 0)
			read_only = 0;
		else if (strlen(argv[i]) == 7 && strncmp(argv[i], "altterm", 7) == 0)
			_altterm = 1;
		else if (strlen(argv[i]) == 4 && strncmp(argv[i], "help", 4) == 0) {
			print_tsol_usage();
			return 0;
		}
		else {
			print_tsol_usage();
			return 0;
		}
	}

	get_lan_options(hostname,NULL,NULL,&hauth, &hpriv, &hcipher,NULL,NULL);
        result = open_sockfd(hostname, port, &sockfd, &haddr, &haddrlen, 1);
	if (result) {
		lperror(LOG_ERR, "Connect to %s failed",hostname);
		return result;
	}

#ifdef WIN32
	/* TODO: implement terminal handling for Windows here. */
	printf("Windows TSOL terminal handling not yet implemented\n");
	if (recvip != NULL)
		result = ipmi_tsol_stop(intf, recvip, port);
	return LAN_ERR_NOTSUPPORT;
#elif defined(MACOS)
	printf("MacOS TSOL terminal handling not yet implemented\n");
	if (recvip != NULL)
		result = ipmi_tsol_stop(intf, recvip, port);
	return LAN_ERR_NOTSUPPORT;
#elif defined(HPUX)
	printf("HPUX TSOL terminal handling not yet implemented\n");
	if (recvip != NULL)
		result = ipmi_tsol_stop(intf, recvip, port);
	return LAN_ERR_NOTSUPPORT;
#else
	/*
	 * retrieve local IP address if not supplied on command line
	 */
	if (recvip == NULL) {
	        set_lan_options(hostname,NULL,NULL,hauth, hpriv, hcipher,
				(void *)&haddr,haddrlen);
		result = ipmi_open(fdebug); /* must connect first */
		if (result < 0)
			return -1;

		memset(&myaddr, 0, sizeof(myaddr));
		memset(mystr, 0, sizeof(mystr));
		mylen = sizeof(myaddr);
		if (getsockname(sockfd, (struct sockaddr *)&myaddr, &mylen) < 0) {
			lperror(LOG_ERR, "getsockname failed");
			return -1;
		}

		recvip = (char *)inet_ntop(((struct sockaddr *)&myaddr)->sa_family, &myaddr, mystr, sizeof(mystr)-1);
		if (recvip == NULL) {
			lprintf(LOG_ERR, "Unable to find local IP address");
			return -1;
		}
	}

	printf("[Starting %sSOL with receiving address %s:%d]\r\n",
	       read_only ? "Read-only " : "", recvip, port);

	set_terminal_size(rows, cols);
	enter_raw_mode();

	/*
	 * talk to smdc to start Console redirect - IP address and port as parameter
	 * ipmiutil -I lan -H 192.168.168.227 -U Administrator raw 0x30 0x06 0xC0 0xA8 0xA8 0x78 0x1A 0x0A
	 */
	result = ipmi_tsol_start(intf, recvip, port);
        if (result < 0) {
		leave_raw_mode();
		lprintf(LOG_ERR, "Error starting SOL");
                return -1;
        }

	printf("[SOL Session operational.  Use %c? for help]\r\n",
	       sol_escape_char);

	gettimeofday(&_start_keepalive, 0);

	fd_socket = sockfd;
	fds_wait[0].fd = fd_socket;
	fds_wait[0].events = POLLIN;
	fds_wait[0].revents = 0;
	fds_wait[1].fd = fileno(stdin);
	fds_wait[1].events = POLLIN;
	fds_wait[1].revents = 0;
	fds_wait[2].fd = -1;
	fds_wait[2].events = 0;
	fds_wait[2].revents = 0;

	fds_data_wait[0].fd = fd_socket;
	fds_data_wait[0].events = POLLIN | POLLOUT;
	fds_data_wait[0].revents = 0;
	fds_data_wait[1].fd = fileno(stdin);
	fds_data_wait[1].events = POLLIN;
	fds_data_wait[1].revents = 0;
	fds_data_wait[2].fd = fileno(stdout);
	fds_data_wait[2].events = POLLOUT;
	fds_data_wait[2].revents = 0;

	out_buff_fill = 0;
	in_buff_fill = 0;
	fds = fds_wait;

	for (;;) {
		result = poll(fds, 3, 15*1000);
		if (result < 0)
			break;

		/* send keepalive packet */
		tsol_keepalive(intf);

		if ((fds[0].revents & POLLIN) && (sizeof(out_buff) > out_buff_fill)){
			socklen_t sin_len = sizeof(sin);
			result = recvfrom(fd_socket, buff, sizeof(out_buff) - out_buff_fill + 4, 0,
					  (struct sockaddr *)&sin, &sin_len);

			/* read the data from udp socket, skip some bytes in the head */
			if((result - 4) > 0 ){
				int length = result - 4;
#if 1
		 		length = (unsigned char)buff[2] & 0xff;
			       	length *= 256;
				length += ((unsigned char)buff[3] & 0xff);
				if ((length <= 0) || (length > (result - 4)))
			              length = result - 4;
#endif
				memcpy(out_buff + out_buff_fill, buff + 4, length);
				out_buff_fill += length;
			}
		}
		if ((fds[1].revents & POLLIN) && (sizeof(in_buff) > in_buff_fill)) {
			result = read(fileno(stdin), in_buff + in_buff_fill,
				      sizeof(in_buff) - in_buff_fill); // read from keyboard
			if (result > 0) {
				int bytes;
				bytes = do_inbuf_actions(intf, in_buff + in_buff_fill, result);
				if(bytes < 0) {
					result = ipmi_tsol_stop(intf, recvip, port);
					do_terminal_cleanup();
					return result;
				}
				if (read_only)
					bytes = 0;
				in_buff_fill += bytes;
			}
		}
		if ((fds[2].revents & POLLOUT) && out_buff_fill) {
			result = write(fileno(stdout), out_buff, out_buff_fill); // to screen
			if (result > 0) {
				out_buff_fill -= result;
				if (out_buff_fill) {
					memmove(out_buff, out_buff + result, out_buff_fill);
				}
			}
		}
		if ((fds[0].revents & POLLOUT) && in_buff_fill) {
			/*
			 * translate key and send that to SMDC using IPMI
			 * ipmiutil cmd -N 192.168.168.227 -U Administrator 0x30 0x03 0x04 0x1B 0x5B 0x43
			 */
			result = ipmi_tsol_send_keystroke(intf, in_buff, 
					MIN(in_buff_fill,14));
			if (result > 0) {
				gettimeofday(&_start_keepalive, 0);
				in_buff_fill -= result;
				if (in_buff_fill) {
					memmove(in_buff, in_buff + result, in_buff_fill);
				}
			}
		}
		fds = (in_buff_fill || out_buff_fill )?
			fds_data_wait : fds_wait;
	}
#endif

	return 0;
}

#ifdef METACOMMAND
int i_tsol(int argc, char **argv)
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
	set_loglevel(LOG_NOTICE);

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
		    set_loglevel(LOG_DEBUG);
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
		print_tsol_usage();
		return ERR_USAGE;
                break;
	}
        for (i = 0; i < optind; i++) { argv++; argc--; }

	rc = ipmi_tsol_main(intf, argc, argv);

        ipmi_close_();
        // show_outcome(progname,rc);
	return rc;
}
/* end itsol.c */

