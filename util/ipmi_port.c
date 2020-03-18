/*
 * ipmi_port.c
 * Allocate the RMCP port (623.) with a bind so that port manager
 * does not try to reuse it.  Only needed for Linux.
 *
 * Note that the Intel dpcproxy service also uses port 623 to listen
 * for incoming telnet/SOL clients, so we should not start ipmi_port
 * if dpcproxy is running.
 *
 * Changes:
 *  05/18/07  Andy Cress - created
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#ifdef TEST
#include "ipmicmd.h" 
#endif

#define RMCP_PORT  623
static char * progver   = "1.4";        /* program version */
static char *progname   = "ipmi_port";  /* program name */
static int sockfd = 0;
static struct sockaddr_in _srcaddr;  
static int interval = 20;  /* num sec to wait, was 60 */
static char fdebug = 0;

static int mkdaemon(int fchdir, int fclose);
static int mkdaemon(int fchdir, int fclose)
{
    int fdlimit = sysconf(_SC_OPEN_MAX); /*fdlimit usu = 1024.*/
    int fd = 0;
    int i;
 
    fdlimit = fileno(stderr);
    switch (fork()) {
        case 0:  break;
        case -1: return -1;
        default: _exit(0);          /* exit the original process */
    }
    if (setsid() < 0) return -1;    /* shouldn't fail */
    switch (fork()) {
        case 0:  break;
        case -1: return -1;
        default: _exit(0);
    }
    if (fchdir) i = chdir("/");
    if (fclose) {
        /* Close stdin,stdout,stderr and replace them with /dev/null */
        for (fd = 0; fd < fdlimit; fd++) close(fd);
        fd = open("/dev/null",O_RDWR);
        i = dup(0); 
	i = dup(0);
    }
    return 0;
}

static int open_rmcp_port(int port);
static int open_rmcp_port(int port)
{
    int rv;

    /* Open a socket binding to the RMCP port */
    rv = socket(AF_INET, SOCK_DGRAM, 0); 
    if (rv < 0) return (rv);
    else sockfd = rv;

    memset(&_srcaddr, 0, sizeof(_srcaddr));
    _srcaddr.sin_family = AF_INET;
    _srcaddr.sin_port   = htons(port);
    _srcaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    rv = bind(sockfd, (struct sockaddr *)&_srcaddr, sizeof(_srcaddr));
    if (rv < 0) {
         printf("bind(%d,%d) error, rv = %d\n",port,INADDR_ANY,rv);
         close(sockfd);
         return (rv);
    }

    return(rv);
}

static void iport_cleanup(int sig)
{
    if (sockfd != 0) close(sockfd);
    exit(EXIT_SUCCESS);
}

/*int ipmi_port(int argc, char **argv) */
int main(int argc, char **argv)
{
   int rv;
   int c; 
   int background = 0;
   struct sigaction sact;

   while ((c = getopt(argc, argv, "bx?")) != EOF) {
        switch (c) {
           case 'x':  fdebug = 1; break;
           case 'b':  background = 1; break;
           default:
              printf("Usage: %s [-xb]   (-b means background)\n",progname);
              exit(1);
        }
   }
   if (!background)
        printf("%s ver %s\n", progname,progver);

   rv = open_rmcp_port(RMCP_PORT);
   if (rv != 0) {
	printf("open_rmcp_port(%d) failed, rv = %d\n",RMCP_PORT,rv);
        exit(1);
   } else if (!background) 
	printf("open_rmcp_port(%d) succeeded, sleeping\n",RMCP_PORT);

   /* convert to a daemon if background */
   if (background) {
        rv = mkdaemon(1,1);
        if (rv != 0) {
           printf("%s: Cannot become daemon, rv = %d\n", progname,rv);
           exit(1);
        }
   }

   /* handle signals for cleanup */
   sact.sa_handler = iport_cleanup;
   sact.sa_flags = 0;
   sigemptyset(&sact.sa_mask);
   sigaction(SIGINT, &sact, NULL);
   sigaction(SIGQUIT, &sact, NULL);
   sigaction(SIGTERM, &sact, NULL);

   if (rv == 0) while(1)
   {
      sleep(interval);  /*wait 60 seconds*/
   }
   if (sockfd != 0) close(sockfd);
   return(rv);
}

/*end ipmi_port.c */
