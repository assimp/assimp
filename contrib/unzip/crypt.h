/* crypt.h -- base code for crypt/uncrypt ZIPfile


   Version 1.01e, February 12th, 2005

   Copyright (C) 1998-2005 Gilles Vollant

   This code is a modified version of crypting code in Infozip distribution

   The encryption/decryption parts of this source code (as opposed to the
   non-echoing password parts) were originally written in Europe.  The
   whole source package can be freely distributed, including from the USA.
   (Prior to January 2000, re-export from the US was a violation of US law.)

   This encryption code is a direct transcription of the algorithm from
   Roger Schlafly, described by Phil Katz in the file appnote.txt.  This
   file (appnote.txt) is distributed with the PKZIP program (even in the
   version without encryption capabilities).

   If you don't need crypting in your application, just define symbols
   NOCRYPT and NOUNCRYPT.

   This code support the "Traditional PKWARE Encryption".

   The new AES encryption added on Zip format by Winzip (see the page
   http://www.winzip.com/aes_info.htm ) and PKWare PKZip 5.x Strong
   Encryption is not supported.
*/

#define CRC32(c, b) ((*(pcrc_32_tab+(((int)(c) ^ (b)) & 0xff))) ^ ((c) >> 8))

/***********************************************************************
 * Return the next byte in the pseudo-random sequence
 */
static int decrypt_byte(unsigned long* pkeys, const z_crc_t* pcrc_32_tab) {
    unsigned temp;  /* POTENTIAL BUG:  temp*(temp^1) may overflow in an
                     * unpredictable manner on 16-bit systems; not a problem
                     * with any known compiler so far, though */

    (void)pcrc_32_tab;
    temp = ((unsigned)(*(pkeys+2)) & 0xffff) | 2;
    return (int)(((temp * (temp ^ 1)) >> 8) & 0xff);
}

/***********************************************************************
 * Update the encryption keys with the next byte of plain text
 */
static int update_keys(unsigned long* pkeys, const z_crc_t* pcrc_32_tab, int c) {
    (*(pkeys+0)) = CRC32((*(pkeys+0)), c);
    (*(pkeys+1)) += (*(pkeys+0)) & 0xff;
    (*(pkeys+1)) = (*(pkeys+1)) * 134775813L + 1;
    {
      register int keyshift = (int)((*(pkeys+1)) >> 24);
      (*(pkeys+2)) = CRC32((*(pkeys+2)), keyshift);
    }
    return c;
}


/***********************************************************************
 * Initialize the encryption keys and the random header according to
 * the given password.
 */
static void init_keys(const char* passwd, unsigned long* pkeys, const z_crc_t* pcrc_32_tab) {
    *(pkeys+0) = 305419896L;
    *(pkeys+1) = 591751049L;
    *(pkeys+2) = 878082192L;
    while (*passwd != '\0') {
        update_keys(pkeys,pcrc_32_tab,(int)*passwd);
        passwd++;
    }
}

#define zdecode(pkeys,pcrc_32_tab,c) \
    (update_keys(pkeys,pcrc_32_tab,c ^= decrypt_byte(pkeys,pcrc_32_tab)))

#define zencode(pkeys,pcrc_32_tab,c,t) \
    (t=decrypt_byte(pkeys,pcrc_32_tab), update_keys(pkeys,pcrc_32_tab,c), (Byte)t^(c))

#ifdef INCLUDECRYPTINGCODE_IFCRYPTALLOWED

#define RAND_HEAD_LEN  12

/* Cryptographically secure random source selection:
 * Windows: CryptGenRandom (FIPS 140-2 certified)
 * Unix/macOS: /dev/urandom (non-blocking, cryptographically secure)
 * Fallback: Enhanced rand() with entropy mixing
 */
#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
#  include <wincrypt.h>
#endif
#include <stdlib.h>
#include <time.h>
#if defined(__unix__) || defined(__APPLE__)
#  include <fcntl.h>
#  include <unistd.h>
#  include <sys/types.h>
#endif

static int secure_random_byte(unsigned char* byte) {
#if defined(_WIN32) || defined(_WIN64)
    HCRYPTPROV hCryptProv = 0;
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        goto fallback;
    }
    if (!CryptGenRandom(hCryptProv, 1, byte)) {
        CryptReleaseContext(hCryptProv, 0);
        goto fallback;
    }
    CryptReleaseContext(hCryptProv, 0);
    return 1;
#elif defined(__unix__) || defined(__APPLE__)
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        unsigned char b = 0;
        if (read(fd, &b, 1) == 1) {
            *byte = b;
            close(fd);
            return 1;
        }
        close(fd);
    }
    goto fallback;
#endif
fallback:
    /* Fallback with entropy mixing: time + counter + rand */
    static int initialized = 0;
    static unsigned int counter = 0;
    if (!initialized) {
        srand((unsigned int)(time(NULL) ^ 0xDEADBEEFUL));
        initialized = 1;
    }
    counter++;
    *byte = (unsigned char)(((rand() >> 7) ^ (counter & 0xFF)) & 0xFF);
    return 0;
}

static unsigned crypthead(const char* passwd,       /* password string */
                          unsigned char* buf,       /* where to write header */
                          int bufSize,
                          unsigned long* pkeys,
                          const z_crc_t* pcrc_32_tab,
                          unsigned long crcForCrypting) {
    unsigned n;                  /* index in random header */
    int t;                       /* temporary */
    unsigned char header[RAND_HEAD_LEN-2]; /* random header */

    if (bufSize<RAND_HEAD_LEN)
      return 0;

    /* Generate RAND_HEAD_LEN-2 random bytes using cryptographically secure source */
    init_keys(passwd, pkeys, pcrc_32_tab);
    for (n = 0; n < RAND_HEAD_LEN-2; n++) {
        unsigned char c = 0;
        secure_random_byte(&c);
        header[n] = (unsigned char)zencode(pkeys, pcrc_32_tab, c, t);
    }
    /* Encrypt random header (last two bytes is high word of crc) */
    init_keys(passwd, pkeys, pcrc_32_tab);
    for (n = 0; n < RAND_HEAD_LEN-2; n++) {
        buf[n] = (unsigned char)zencode(pkeys, pcrc_32_tab, header[n], t);
    }
    buf[n++] = (unsigned char)zencode(pkeys, pcrc_32_tab, (int)(crcForCrypting >> 16) & 0xff, t);
    buf[n++] = (unsigned char)zencode(pkeys, pcrc_32_tab, (int)(crcForCrypting >> 24) & 0xff, t);
    return n;
}

#endif