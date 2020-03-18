/* 
 * md2.h  
 * Used in ipmilan.c
 *
 * 05/26/2006 ARCress - copied from freeipmi ipmi-md2.c, 
 *                      added md2_sum() subroutine, added WIN32 flag
 * 08/14/2008 ARCress - moved md2 routines from md2.c to md2.h
 */
/* 
 * (c) (C) 2003 FreeIPMI Core Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */


/* ====================================================================
 * Copyright (c) 1998-2001 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

/* only include this file if MD2OK was defined */

#ifdef WIN32
#define  u_int32_t  unsigned int
#define  u_int8_t   unsigned char
#endif
/* start ipmi-md2.h */
#define IPMI_MD2_BLOCK_LEN   16
#define IPMI_MD2_BUFFER_LEN  48
#define IPMI_MD2_CHKSUM_LEN  16
#define IPMI_MD2_DIGEST_LEN  16
#define IPMI_MD2_PADDING_LEN 16
#define IPMI_MD2_ROUNDS_LEN  18

typedef struct __md2 {
  u_int32_t magic;
  u_int8_t l;
  unsigned int mlen;
  u_int8_t x[IPMI_MD2_BUFFER_LEN];
  u_int8_t c[IPMI_MD2_CHKSUM_LEN];
  u_int8_t m[IPMI_MD2_BLOCK_LEN];
} ipmi_md2_t;

int ipmi_md2_init(ipmi_md2_t *ctx);
int ipmi_md2_update_data(ipmi_md2_t *ctx, u_int8_t *buf, unsigned int buflen);
int ipmi_md2_finish(ipmi_md2_t *ctx, u_int8_t *digest, unsigned int digestlen);
/* end ipmi-md2.h */

static char padding[16][16] = 
  {
    {0x01},
    {0x02,0x02},
    {0x03,0x03,0x03},
    {0x04,0x04,0x04,0x04},
    {0x05,0x05,0x05,0x05,0x05},
    {0x06,0x06,0x06,0x06,0x06,0x06},
    {0x07,0x07,0x07,0x07,0x07,0x07,0x07},
    {0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08},
    {0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09},
    {0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A},
    {0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B,0x0B},
    {0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C},
    {0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D,0x0D},
    {0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E},
    {0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F},
    {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10}
  };

static unsigned char S[256] = 
  {
    0x29, 0x2E, 0x43, 0xC9, 0xA2, 0xD8, 0x7C, 0x01, 
    0x3D, 0x36, 0x54, 0xA1, 0xEC, 0xF0, 0x06, 0x13, 
    0x62, 0xA7, 0x05, 0xF3, 0xC0, 0xC7, 0x73, 0x8C, 
    0x98, 0x93, 0x2B, 0xD9, 0xBC, 0x4C, 0x82, 0xCA, 
    0x1E, 0x9B, 0x57, 0x3C, 0xFD, 0xD4, 0xE0, 0x16, 
    0x67, 0x42, 0x6F, 0x18, 0x8A, 0x17, 0xE5, 0x12, 
    0xBE, 0x4E, 0xC4, 0xD6, 0xDA, 0x9E, 0xDE, 0x49, 
    0xA0, 0xFB, 0xF5, 0x8E, 0xBB, 0x2F, 0xEE, 0x7A, 
    0xA9, 0x68, 0x79, 0x91, 0x15, 0xB2, 0x07, 0x3F, 
    0x94, 0xC2, 0x10, 0x89, 0x0B, 0x22, 0x5F, 0x21, 
    0x80, 0x7F, 0x5D, 0x9A, 0x5A, 0x90, 0x32, 0x27, 
    0x35, 0x3E, 0xCC, 0xE7, 0xBF, 0xF7, 0x97, 0x03, 
    0xFF, 0x19, 0x30, 0xB3, 0x48, 0xA5, 0xB5, 0xD1, 
    0xD7, 0x5E, 0x92, 0x2A, 0xAC, 0x56, 0xAA, 0xC6, 
    0x4F, 0xB8, 0x38, 0xD2, 0x96, 0xA4, 0x7D, 0xB6, 
    0x76, 0xFC, 0x6B, 0xE2, 0x9C, 0x74, 0x04, 0xF1, 
    0x45, 0x9D, 0x70, 0x59, 0x64, 0x71, 0x87, 0x20, 
    0x86, 0x5B, 0xCF, 0x65, 0xE6, 0x2D, 0xA8, 0x02, 
    0x1B, 0x60, 0x25, 0xAD, 0xAE, 0xB0, 0xB9, 0xF6, 
    0x1C, 0x46, 0x61, 0x69, 0x34, 0x40, 0x7E, 0x0F, 
    0x55, 0x47, 0xA3, 0x23, 0xDD, 0x51, 0xAF, 0x3A, 
    0xC3, 0x5C, 0xF9, 0xCE, 0xBA, 0xC5, 0xEA, 0x26, 
    0x2C, 0x53, 0x0D, 0x6E, 0x85, 0x28, 0x84, 0x09, 
    0xD3, 0xDF, 0xCD, 0xF4, 0x41, 0x81, 0x4D, 0x52, 
    0x6A, 0xDC, 0x37, 0xC8, 0x6C, 0xC1, 0xAB, 0xFA, 
    0x24, 0xE1, 0x7B, 0x08, 0x0C, 0xBD, 0xB1, 0x4A, 
    0x78, 0x88, 0x95, 0x8B, 0xE3, 0x63, 0xE8, 0x6D, 
    0xE9, 0xCB, 0xD5, 0xFE, 0x3B, 0x00, 0x1D, 0x39, 
    0xF2, 0xEF, 0xB7, 0x0E, 0x66, 0x58, 0xD0, 0xE4, 
    0xA6, 0x77, 0x72, 0xF8, 0xEB, 0x75, 0x4B, 0x0A, 
    0x31, 0x44, 0x50, 0xB4, 0x8F, 0xED, 0x1F, 0x1A, 
    0xDB, 0x99, 0x8D, 0x33, 0x9F, 0x11, 0x83, 0x14
  };

#define L               ctx->l  
#define X               ctx->x 
#define C               ctx->c
#define M               ctx->m
#define Mlen            ctx->mlen
#define IPMI_MD2_MAGIC  0xf00fd00d 

int 
ipmi_md2_init(ipmi_md2_t *ctx) 
{

  if (ctx == NULL) 
    {
      errno = EINVAL;
      return -1;
    }

  ctx->magic = IPMI_MD2_MAGIC;

  L = 0;
  Mlen = 0;
  memset(X, '\0', IPMI_MD2_BUFFER_LEN);
  memset(C, '\0', IPMI_MD2_CHKSUM_LEN);
  memset(M, '\0', IPMI_MD2_BLOCK_LEN);

  return 0;
}

static void 
_ipmi_md2_update_digest_and_checksum(ipmi_md2_t *ctx) 
{
  int j, k;
  u_int8_t c, t;

  /* Update X */

  for (j = 0; j < IPMI_MD2_BLOCK_LEN; j++) 
    {
      X[16+j] = M[j];
      X[32+j] = (X[16+j] ^ X[j]);
    }
  
  t = 0;

  for (j = 0; j < IPMI_MD2_ROUNDS_LEN; j++) 
    {
      for (k = 0; k < IPMI_MD2_BUFFER_LEN; k++) 
        {
          t = X[k] = (X[k] ^ S[t]);
        }
      t = (t + j) % 256;
    }

  /* Update C and L */
  
  /* achu: Note that there is a typo in the RFC 1319 specification.
   * In section 3.2, the line:
   *
   * Set C[j] to S[c xor L]
   *
   * should read:
   *
   * Set C[j] to C[j] xor S[c xor L].
   */
  
  for (j = 0; j < IPMI_MD2_BLOCK_LEN; j++) 
    {
      c = M[j];
      C[j] = C[j] ^ S[c ^ L];
      L = C[j];
    }
}

int 
ipmi_md2_update_data(ipmi_md2_t *ctx, u_int8_t *buf, unsigned int buflen) 
{

  if (ctx == NULL || ctx->magic != IPMI_MD2_MAGIC || buf == NULL) 
    {
      errno = EINVAL;
      return -1;
    }

  if (buflen == 0)
    return 0;

  if ((Mlen + buflen) >= IPMI_MD2_BLOCK_LEN) 
    {
      unsigned int bufcount;
      
      bufcount = (IPMI_MD2_BLOCK_LEN - Mlen);
      memcpy(M + Mlen, buf, bufcount);
      _ipmi_md2_update_digest_and_checksum(ctx);
    
      while ((buflen - bufcount) >= IPMI_MD2_BLOCK_LEN) 
        {
          memcpy(M, buf + bufcount, IPMI_MD2_BLOCK_LEN);
          bufcount += IPMI_MD2_BLOCK_LEN;
          _ipmi_md2_update_digest_and_checksum(ctx);
        }
      
      Mlen = buflen - bufcount;
      if (Mlen > 0)
        memcpy(M, buf + bufcount, Mlen);
    }
  else 
    {
      /* Not enough data to update X and C, just copy in data */ 
      memcpy(M + Mlen, buf, buflen); 
      Mlen += buflen;
    }

  return buflen;
}

static void 
_ipmi_md2_append_padding_and_checksum(ipmi_md2_t *ctx) 
{
  unsigned int padlen;
  int padindex;

  padlen = IPMI_MD2_PADDING_LEN - Mlen;
  padindex = padlen - 1;

  ipmi_md2_update_data(ctx, padding[padindex], padlen);
  
  ipmi_md2_update_data(ctx, C, IPMI_MD2_CHKSUM_LEN);
}

int 
ipmi_md2_finish(ipmi_md2_t *ctx, u_int8_t *digest, unsigned int digestlen) 
{
  
  if (ctx == NULL || ctx->magic != IPMI_MD2_MAGIC 
      || digest == NULL || digestlen < IPMI_MD2_DIGEST_LEN) 
    {
      errno = EINVAL;
      return -1;
    }
  
  _ipmi_md2_append_padding_and_checksum(ctx);
  memcpy(digest, X, IPMI_MD2_DIGEST_LEN);
  
  return IPMI_MD2_DIGEST_LEN;
}

/* added - ARCress */
void md2_sum(unsigned char *pdata, int sdata, unsigned char *digest)
{
   ipmi_md2_t ctx;
 
   ipmi_md2_init(&ctx);
   ipmi_md2_update_data(&ctx, pdata, sdata);
   ipmi_md2_finish(&ctx, digest, 16);
}
/*end md2.h */
