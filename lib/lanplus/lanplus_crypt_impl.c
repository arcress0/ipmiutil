/*
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
/* ARCress, TODO: improve error handling and remove all assert() calls here. */

#include "ipmitool/log.h"
#include "ipmitool/ipmi_constants.h"
#include "lanplus.h"
#include "lanplus_crypt_impl.h"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <assert.h>

#ifdef WIN32
/* win_rand_filename 
   Custom RAND_file_name routine to use better path than C:\ 
   Use: %USERPROFILE%, %HOME%, %HOMEPATH%, 
   USERPROFILE=C:\Users\acress
   HOMEDRIVE=C:
   HOMEPATH=\Users\acress
 */
char *win_rand_filename(char *buffer, int bufsz)
{
   char *root = "C:\\";
   char *envpath = NULL;
   
   envpath = getenv("USERPROFILE");
   if ((envpath == NULL) || (envpath[0] == '\0')) {
		envpath = root;
   }
   snprintf(buffer,bufsz,"%s\\.rnd",envpath);
   return(buffer);
}
#endif

/*
 * lanplus_seed_prng
 *
 * Seed our PRNG with the specified number of bytes from /dev/random
 * 
 * param bytes specifies the number of bytes to read from /dev/random
 * 
 * returns 0 on success
 *         1 on failure
 */
int lanplus_seed_prng(uint32_t bytes)
{
#ifdef WIN32
        const char *randfile;
        char buffer[200];
        static FILE *fp = NULL;
        size_t i;
        // randfile = RAND_file_name(buffer, sizeof buffer); /* usu C:\.rnd */
        randfile = win_rand_filename(buffer, sizeof buffer); 
        if ((randfile != NULL) && (fp == NULL)) {  
		   fp = fopen(randfile,"r");  /*check the randfile*/
           if (fp == NULL) {   /*does not exist, create it*/
	   	      /*first time, so open/create file*/
              fp = fopen(randfile,"w");  /*create the randfile*/
              if (fp != NULL) {
				i = fwrite(" \n",2,1,fp);
				fclose(fp);
              } else {
                printf("seed_prng: cannot create %s file\n",randfile);
			  }
           } else { /*file opened, so close it*/
		      fclose(fp);
		   }
        }
        if (verbose > 0)
            printf("seed_prng: RAND_file_name = %s, fp=%p\n",randfile,fp);
#else
        const char *randfile = "/dev/urandom";
#endif

        if (RAND_load_file(randfile, bytes) == 0) {
                printf("seed_prng: RAND_load_file(%s) failed\n",randfile);
		return 1;
	} else {  /*success*/
		return 0;
	}
}

/*
 * lanplus_rand
 *
 * Generate a random number of the specified size
 * 
 * param num_bytes [in]  is the size of the random number to be
 *       generated
 * param buffer [out] is where we will place our random number
 * 
 * return 0 on success
 *        1 on failure
 */
int
lanplus_rand(uint8_t * buffer, uint32_t num_bytes)
{
#undef IPMI_LANPLUS_FAKE_RAND
#ifdef IPMI_LANPLUS_FAKE_RAND
	/*
	 * This code exists so that we can easily find the generated random
	 * number in the hex dumps.
	 */
 	int i;
	for (i = 0; i < num_bytes; ++i)
		buffer[i] = 0x70 | i;
	return 0;
#else
	return (! RAND_bytes(buffer, num_bytes));
#endif
}

/*
 * lanplus_HMAC
 *
 * param mac specifies the algorithm to be used, currently only SHA1 is supported
 * param key is the key used for HMAC generation
 * param key_len is the lenght of key
 * param d is the data to be MAC'd
 * param n is the length of the data at d
 * param md is the result of the HMAC algorithm
 * param md_len is the length of md
 *
 * returns a pointer to md
 */
uint8_t *
lanplus_HMAC(uint8_t        mac,
			 const void    *key,
			 int            key_len,
			 const uint8_t *d,
			 int            n,
			 uint8_t       *md,
			 uint32_t      *md_len)
{
	const EVP_MD *evp_md = NULL;
	unsigned int mlen;
	uint8_t *pnew;

	*md_len = 0;  /*if return NULL, also return zero length*/
	if (verbose > 2) {
		printf("lanplus_HMAC start mac=%x\n",mac);
	}
	if ((mac == IPMI_AUTH_RAKP_HMAC_SHA1) ||
		(mac == IPMI_INTEGRITY_HMAC_SHA1_96))
		evp_md = EVP_sha1();
	else if ((mac == IPMI_AUTH_RAKP_HMAC_MD5) ||
		     (mac == IPMI_INTEGRITY_HMAC_MD5_128))
		evp_md = EVP_md5();
	else if ((mac == IPMI_AUTH_RAKP_HMAC_SHA256) ||
		     (mac == IPMI_INTEGRITY_HMAC_SHA256_128)) {
#ifdef HAVE_SHA256
		evp_md = EVP_sha256();
#else
		printf("lanplus_HMAC: Invalid EVP_sha256 in lanplus_HMAC\n");
		lprintf(LOG_ERR, "Invalid EVP_sha256 in lanplus_HMAC");
		return NULL; // assert(0);
#endif
	} else {
		printf("lanplus_HMAC: Invalid mac type 0x%x in lanplus_HMAC\n",mac);
		lprintf(LOG_ERR,"Invalid mac type 0x%x in lanplus_HMAC",mac);
		return NULL; // assert(0);
	}
	mlen = 20; /* md_len is usually not initialized, default IPMI_AUTHCODE_BUFFER_SIZE=20 */
	pnew = HMAC(evp_md, key, key_len, d, n, md, &mlen);
	if (verbose > 2) {
		printf("lanplus_HMAC mac=%x, pnew=%p, mlen=%d",mac,pnew,mlen);
	}
	*md_len = (uint32_t)mlen;
	return(pnew);
}


/*
 * lanplus_encrypt_aes_cbc_128
 *
 * Encrypt with the AES CBC 128 algorithm
 *
 * param iv is the 16 byte initialization vector
 * param key is the 16 byte key used by the AES algorithm
 * param input is the data to be encrypted
 * param input_length is the number of bytes to be encrypted.  This MUST
 *       be a multiple of the block size, 16.
 * param output is the encrypted output
 * param bytes_written is the number of bytes written.  This param is set
 *       to 0 on failure, or if 0 bytes were input.
 */
void
lanplus_encrypt_aes_cbc_128(const uint8_t * iv,
				const uint8_t * key,
				const uint8_t * input,
				uint32_t        input_length,
				uint8_t       * output,
				uint32_t      * bytes_written)
{
	int nwritten = 0;
	int inlen = 0;
	EVP_CIPHER_CTX *pctx;
#ifdef SSL11
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	pctx = ctx;
#else
	EVP_CIPHER_CTX ctx;
	pctx = &ctx;
#endif
	EVP_CIPHER_CTX_init(pctx);
	EVP_EncryptInit_ex(pctx, EVP_aes_128_cbc(), NULL, key, iv);
	EVP_CIPHER_CTX_set_padding(pctx, 0);

	*bytes_written = 0;
	if (input_length == 0) return;

	if (verbose >= 5)
	{
		printbuf(iv,  16, "encrypting with this IV");
		printbuf(key, 16, "encrypting with this key");
		printbuf(input, input_length, "encrypting this data");
	}

	/*
	 * The default implementation adds a whole block of padding if the input
	 * data is perfectly aligned. We would like to keep that from happening.
	 * We have made a point to have our input perfectly padded.
	 */
	// assert((input_length % IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE) == 0);
	if ((input_length % IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE) != 0) {
		 os_assert("lanplus_encrypt_aes_cbc_128"); /**/
	}
	inlen = input_length;

	if(!EVP_EncryptUpdate(pctx, output, &nwritten, input, inlen))
	{
		*bytes_written = 0; /* Error */
	}
	else
	{
		int tmplen;

		if(!EVP_EncryptFinal_ex(pctx, output + nwritten, &tmplen))
		{
			*bytes_written = 0; /* Error */
		}
		else
		{
			/* Success */
			*bytes_written = nwritten + tmplen;
			EVP_CIPHER_CTX_cleanup(pctx);
		}
	}
#ifdef SSL11
    EVP_CIPHER_CTX_free(ctx);
#endif
    return;
}


/*
 * lanplus_decrypt_aes_cbc_128
 *
 * Decrypt with the AES CBC 128 algorithm
 *
 * param iv is the 16 byte initialization vector
 * param key is the 16 byte key used by the AES algorithm
 * param input is the data to be decrypted
 * param input_length is the number of bytes to be decrypted.  This MUST
 *       be a multiple of the block size, 16.
 * param output is the decrypted output
 * param bytes_written is the number of bytes written.  This param is set
 *       to 0 on failure, or if 0 bytes were input.
 */
void
lanplus_decrypt_aes_cbc_128(const uint8_t * iv,
					const uint8_t * key,
					const uint8_t * input,
					uint32_t        input_length,
					uint8_t       * output,
					uint32_t      * bytes_written)
{
	int nwritten = 0;
	int inlen = 0;
	EVP_CIPHER_CTX *pctx;
#ifdef SSL11
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	pctx = ctx;
#else
	EVP_CIPHER_CTX ctx;
	pctx = &ctx;
#endif
	EVP_CIPHER_CTX_init(pctx);
	EVP_DecryptInit_ex(pctx, EVP_aes_128_cbc(), NULL, key, iv);
	EVP_CIPHER_CTX_set_padding(pctx, 0);

	if (verbose >= 5)
	{
		printbuf(iv,  16, "decrypting with this IV");
		printbuf(key, 16, "decrypting with this key");
		printbuf(input, input_length, "decrypting this data");
	}

	*bytes_written = 0;
	if (input_length == 0) return;

	/*
	 * The default implementation adds a whole block of padding if the input
	 * data is perfectly aligned. We would like to keep that from happening.
	 * We have made a point to have our input perfectly padded.
	 */
	// assert((input_length % IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE) == 0);
	if ((input_length % IPMI_CRYPT_AES_CBC_128_BLOCK_SIZE) != 0) {
		 os_assert("lanplus_decrypt_aes_cbc_128"); /**/
	}
	inlen = input_length;

	if (!EVP_DecryptUpdate(pctx, output, &nwritten, input, inlen))
	{
		/* Error */
		lprintf(LOG_DEBUG, "ERROR: decrypt update failed");
		*bytes_written = 0;
		return;
	}
	else
	{
		int tmplen;

		if (!EVP_DecryptFinal_ex(pctx, output + nwritten, &tmplen))
		{
			char buffer[1000];
			ERR_error_string(ERR_get_error(), buffer);
			lprintf(LOG_DEBUG, "the ERR error %s", buffer);
			lprintf(LOG_DEBUG, "ERROR: decrypt final failed");
			*bytes_written = 0;
			goto evpfin2;
		}
		else
		{
			/* Success */
			*bytes_written = nwritten + tmplen;
			EVP_CIPHER_CTX_cleanup(pctx);
		}
	}

	if (verbose >= 5)
	{
		lprintf(LOG_DEBUG, "Decrypted %d encrypted bytes",input_length);
		printbuf(output, *bytes_written, "Decrypted this data");
	}
evpfin2:
#ifdef SSL11
    EVP_CIPHER_CTX_free(ctx);
#endif
    return;
}
