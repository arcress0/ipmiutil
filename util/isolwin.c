/*
 * isolwin.c
 *      IPMI Serial-Over-LAN console Windows terminal emulation
 *
 * Author:  Andy Cress  arcress at users.sourceforge.net
 * Copyright (c) 2006 Intel Corporation. 
 * Copyright (c) 2009 Kontron America, Inc.
 *
 * 08/20/07 Andy Cress - extracted from isolconsole.c/WIN32,
 *                       added os_usleep to input_thread
 * 09/24/07 Andy Cress - various termio fixes, e.g. scroll(), BUF_SZ
 */
/*M*
Copyright (c) 2006-2007, Intel Corporation
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
#include <windows.h>
#include <stdio.h>
#include <winsock.h>
#include <io.h>
#include <conio.h>
#include <string.h>
#include <time.h>

#define SockType SOCKET
typedef struct {
  int type;
  int len;
  char *data;
  } SOL_RSP_PKT;

extern void dbglog( char *pattn, ... ); /*from isolconsole.c*/
extern void os_usleep(int s, int u);    /*from ipmilan.c*/

typedef unsigned char uchar;

/*
 * Global variables 
 */
extern int   sol_done;
extern char  fUseWinCon;
extern uchar fRaw;
extern uchar fCRLF;    /* =1 to use legacy CR+LF translation for BIOS */
extern uchar bTextMode;
extern char fdebug;      /*from isolconsole.c*/
extern int   wait_time;  /*from isolconsole.c*/

/*======== start Windows =============================*/
    /* Windows os_read, os_select and supporting code */
#define INBUF_SZ    128
static DWORD rg_stdin[INBUF_SZ];
static int   n_stdin = 0;
#define NSPECIAL 33
static struct { uchar c; uchar code[4]; } special_keys[NSPECIAL] = {
 ' ', {0x20,0x20,0x01,0x01},
 '!', {0x21,0x31,0x01,0x21},
 '"', {0x22,0xde,0x01,0x21},
 '#', {0x23,0x33,0x01,0x21},
 '$', {0x24,0x34,0x01,0x21},
 '%', {0x25,0x35,0x01,0x21},
 '&', {0x26,0x37,0x01,0x21},
 0x27, {0x27,0xde,0x01,0x01},  /*'''*/
 '(', {0x28,0x39,0x01,0x21},
 ')', {0x29,0x30,0x01,0x21},
 '*', {0x2a,0x38,0x01,0x21},
 '+', {0x2b,0xbb,0x01,0x21},
 ',', {0x2c,0xbc,0x01,0x01},
 '-', {0x2d,0xbd,0x01,0x01},
 '.', {0x2e,0xbe,0x01,0x01},
 '/', {0x2f,0xbf,0x01,0x01},
 ':', {0x3a,0xba,0x01,0x21},
 ';', {0x3b,0xba,0x01,0x01},
 '<', {0x3c,0xbc,0x01,0x21},
 '=', {0x3d,0xbb,0x01,0x01},
 '>', {0x3e,0xbe,0x01,0x21},
 '?', {0x3f,0xbf,0x01,0x21},
 '@', {0x40,0x32,0x01,0x21},
 '[', {0x5b,0xdb,0x01,0x01},
 0x5c, {0x5c,0xdc,0x01,0x01},  /*'\'*/
 ']', {0x5d,0xdd,0x01,0x01},
 '^', {0x5e,0x36,0x01,0x21},
 '_', {0x5f,0xbd,0x01,0x21},
 '`', {0x60,0xc0,0x01,0x01},
 '{', {0x7b,0xdb,0x01,0x21},
 '|', {0x7c,0xdc,0x01,0x21},
 '}', {0x7d,0xdd,0x01,0x21},
 '~', {0x7e,0xc0,0x01,0x21}
};

void Ascii2KeyCode(uchar c, uchar *buf)
{
   int i; 
   int j = 0;
   /* Convert ascii chars from script file into KeyCodes*/
   for (i = 0; i < NSPECIAL; i++) 
        if (c == special_keys[i].c) break;
   if (i < NSPECIAL) {  /* special chars */
        buf[j++] = c;
        buf[j++] = special_keys[i].code[1];
        buf[j++] = special_keys[i].code[2];
        buf[j++] = special_keys[i].code[3];
   } else if (c < 0x0D) {  /* control chars, like Ctl-c */
        buf[j++] = c;
        buf[j++] = (c | 0x40);
        buf[j++] = 0x01;
        buf[j++] = 0x41;
   } else {  /* alphanumeric chars */
        buf[j++] = c;
	if (c > 0x60) 
	     buf[j++] = c - 0x20; /*drop 0x20 bit if upper case*/
        else buf[j++] = c;
        buf[j++] = 0x01;
        buf[j++] = 0x01;
   }
}

DWORD WINAPI input_thread( LPVOID lpParam) 
{
   HANDLE hstdin;
   DWORD rv = 0;
   INPUT_RECORD inrecords[10];
   DWORD nread;
   DWORD oldmode;

   hstdin = GetStdHandle(STD_INPUT_HANDLE); 

   GetConsoleMode(hstdin, &oldmode);
   SetConsoleMode(hstdin, ENABLE_WINDOW_INPUT);

   while (!sol_done)
   {
      DWORD dwResult = WaitForSingleObject(hstdin, wait_time * 2); /*1000 msec*/

      if (dwResult != WAIT_OBJECT_0)
      {
	 if (dwResult != WAIT_TIMEOUT) 
	    dbglog("input_thread: Wait result = %x\n",dwResult);
         continue;
      }
      
      if (ReadConsoleInput(hstdin, inrecords, 10, &nread))
      {
         DWORD index;

         for (index = 0; index < nread; index++)
         {
            /* Pack the char info into a DWORD */
            INPUT_RECORD *precord = &inrecords[index];
            DWORD dwChar;

            if (precord->EventType == KEY_EVENT)
            {
               if (precord->Event.KeyEvent.wVirtualKeyCode == VK_SHIFT ||
                  precord->Event.KeyEvent.wVirtualKeyCode == VK_CONTROL ||
                  precord->Event.KeyEvent.wVirtualKeyCode == VK_MENU ||
                  precord->Event.KeyEvent.wVirtualKeyCode == VK_CAPITAL)
               {
                  continue;
               }

               dwChar = (((precord->Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0) << 31) | 
                        (((precord->Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0) << 30) | 
                        (((precord->Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) != 0) << 29) | 
                        (((precord->Event.KeyEvent.dwControlKeyState & NUMLOCK_ON) != 0) << 28) | 
                        (((precord->Event.KeyEvent.dwControlKeyState & SCROLLLOCK_ON) != 0) << 27) | 
                        (((precord->Event.KeyEvent.dwControlKeyState & CAPSLOCK_ON) != 0) << 26) | 
                        (((precord->Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) != 0) << 25) | 
                        ((precord->Event.KeyEvent.bKeyDown != 0) << 24) | 
                        ((precord->Event.KeyEvent.wRepeatCount & 0xFF) << 16) | 
                        ((precord->Event.KeyEvent.wVirtualKeyCode & 0xFF) << 8) | 
                         (precord->Event.KeyEvent.uChar.AsciiChar & 0xFF);
            }
            else if (precord->EventType == WINDOW_BUFFER_SIZE_EVENT)
            {
               dwChar = ~0;
            }
            else
            {
               continue;
            }
	    if (precord->Event.KeyEvent.bKeyDown == 0) { 
		if (fdebug > 2) 
		    dbglog("input_thread: key up event (%08x)\n",dwChar);
		continue;
	    }

            if (n_stdin < INBUF_SZ) {  /* rg_stdin[INBUF_SZ] */
		if (fdebug > 2) 
		    dbglog("input_thread: n=%d dwChar=%08x keystate=%x\n",
			n_stdin, dwChar, 
			precord->Event.KeyEvent.dwControlKeyState);
		rg_stdin[n_stdin++] = dwChar;
            } else
		dbglog("input_thread: overflow n=%d size=%d\n",
			n_stdin,INBUF_SZ);
         } /*end-for nread*/
      } /*endif ReadConsoleInput*/
   } /*end-while not sol_done*/

   SetConsoleMode(hstdin, oldmode);

   CloseHandle(hstdin);

   return(rv);
}

int os_read(int fd, uchar *buf, int sz)
{
    int len = 0;

    if (fd == 0) {
       if (n_stdin > 0) {
          /* get pending chars from stdin */
          int cnt = n_stdin * sizeof(rg_stdin[0]);
	  if (fdebug > 2) 
	       dbglog("os_read: stdin n=%d  %04x %04x \n",
			n_stdin,rg_stdin[0],rg_stdin[1]);
          memcpy(buf, rg_stdin, cnt);
          len = cnt;
          n_stdin = 0;
	  memset((uchar *)&rg_stdin,0,cnt);
       }
    } else {
       // ReadFile(fd, buf, sz, &len, NULL);
       // len = _read(fd, buf, sz);
       len = recv(fd, buf, sz, 0);
    }
    return(len);
}

int os_select(int infd, SockType sfd, fd_set *read_fds, fd_set *error_fds)
{
   int rv = 0;
   struct timeval tv;

   /* check the socket for new data via select */
   /* Windows select only works on WSA sockets */
   {
      FD_ZERO(read_fds);
      FD_SET(sfd, read_fds);
      FD_ZERO(error_fds);
      FD_SET(sfd, error_fds);
      tv.tv_sec =  0;
      tv.tv_usec = wait_time * 100; /* 50000 usec, 50 msec, 0.05 sec */
      // rv = select(sfd + 1, read_fds, NULL, error_fds, &tv);
      rv = select((int)(sfd+1), read_fds, NULL, error_fds, &tv);
      if (rv < 0) {
         rv = -(WSAGetLastError());
         if (fdebug) dbglog("select(%d) error %d\r\n",sfd,rv);
      }
      else if (FD_ISSET(sfd, error_fds)) {
         if (fdebug) dbglog("select(%d) error_fds rv=%d err=%d\r\n",sfd, rv, 
			 WSAGetLastError());
      }
      if (rv > 0 && fdebug) 
         dbglog("select(%d) got socket data %d\r\n",sfd,rv);
   }
   if (infd == 0) {  /* check stdin */
      if (n_stdin > 0) {
	FD_SET(infd, read_fds);
	if (rv < 0) rv = 1;
	else rv++;
      }
   }
   return rv;
}
/*======== end Windows ===============================*/


/*end solwin.c*/
