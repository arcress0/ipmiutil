/* 
 * md2.c  
 * Used in ipmilan.c
 *
 * 05/26/2006 ARCress - copied from freeipmi ipmi-md2.c, 
 *                      added md2_sum() subroutine, added WIN32 flag
 * 08/14/2008 ARCress - moved md2 routines from md2.c to md2.h 
 */
/*----------------------------------------------------------------------*
The BSD License 

Copyright (c) 2002-2008, Intel Corporation
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#undef MD2_GPL
#undef MD2_LIB
#if defined(ALLOW_GNU)
#define MD2_GPL     1  /*use md2.h GPL code*/
#else
/* if here, ALLOW_GPL is not defined, check for lanplus libcrypto. */
#if defined(HAVE_LANPLUS)
#define MD2_LIB     1  /*use MD2 version from lanplus libcrypto */
#endif
/* if libcrypto does not have EVP_md2, then skip MD2. */
#if defined(SKIP_MD2)
#undef  MD2_LIB
#endif
#endif

/* use GPL md2.h only if "configure --enable-gpl" set ALLOW_GPL */
#ifdef MD2_GPL
/* 
 * The md2.h contains some GPL code that may not be desired in some cases. 
 * If GPL is not desired, the md2.h can be deleted without modifying the 
 * configure/make process.
 * If GPL is ok, then "configure --enable-gpl" should be used.    
 */
#include "md2.h"
#else
/* else MD2_GPL is not set */

#ifdef MD2_LIB
/* Use EVP_md2() from the openssl libcrypto.so */
#include <openssl/evp.h>
void md2_sum(unsigned char *pdata, int sdata, unsigned char *digest)
{
   EVP_MD_CTX ctx;
   unsigned int mdlen;
   static int fmd2added = 0;
   const EVP_MD *md = NULL;
 
   md = EVP_md2();
   if (fmd2added == 0) {
      EVP_add_digest(md);
      fmd2added = 1;
   }
   EVP_MD_CTX_init(&ctx);
   EVP_DigestInit_ex(&ctx, md, NULL);
   EVP_DigestUpdate(&ctx, pdata, sdata);
   mdlen = 16;
   EVP_DigestFinal_ex(&ctx, digest, &mdlen);
   EVP_MD_CTX_cleanup(&ctx);
}
#endif

#endif

/*end md2.c */
