/* crypt.h -- base code for traditional PKWARE encryption
   Version 1.01e, February 12th, 2005

   Copyright (C) 1998-2005 Gilles Vollant
   Modifications for Info-ZIP crypting
     Copyright (C) 2003 Terry Thorsen

   This code is a modified version of crypting code in Info-ZIP distribution

   Copyright (C) 1990-2000 Info-ZIP.  All rights reserved.

   This program is distributed under the terms of the same license as zlib.
   See the accompanying LICENSE file for the full text of the license.
*/

#ifndef _MINICRYPT_H
#define _MINICRYPT_H

#if ZLIB_VERNUM < 0x1270
typedef unsigned long z_crc_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define RAND_HEAD_LEN  12

/***************************************************************************/

#define zdecode(pkeys,pcrc_32_tab,c) \
    (update_keys(pkeys,pcrc_32_tab, c ^= decrypt_byte(pkeys)))

#define zencode(pkeys,pcrc_32_tab,c,t) \
    (t = decrypt_byte(pkeys), update_keys(pkeys,pcrc_32_tab,c), t^(c))

/***************************************************************************/

/* Return the next byte in the pseudo-random sequence */
uint8_t decrypt_byte(uint32_t *pkeys);

/* Update the encryption keys with the next byte of plain text */
uint8_t update_keys(uint32_t *pkeys, const z_crc_t *pcrc_32_tab, int32_t c);

/* Initialize the encryption keys and the random header according to the given password. */
void init_keys(const char *passwd, uint32_t *pkeys, const z_crc_t *pcrc_32_tab);

/* Generate cryptographically secure random numbers */
int cryptrand(unsigned char *buf, unsigned int len);

/* Create encryption header */
int crypthead(const char *passwd, uint8_t *buf, int buf_size, uint32_t *pkeys, 
    const z_crc_t *pcrc_32_tab, uint8_t verify1, uint8_t verify2);

/***************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
