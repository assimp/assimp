/* zip.c -- IO on .zip files using zlib
   Version 1.2.0, September 16th, 2017
   part of the MiniZip project

   Copyright (C) 2010-2017 Nathan Moinvaziri
     Modifications for AES, PKWARE disk spanning
     https://github.com/nmoinvaz/minizip
   Copyright (C) 2009-2010 Mathias Svensson
     Modifications for Zip64 support
     http://result42.com
   Copyright (C) 1998-2010 Gilles Vollant
     http://www.winimage.com/zLibDll/minizip.html

   This program is distributed under the terms of the same license as zlib.
   See the accompanying LICENSE file for the full text of the license.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "zlib.h"
#include "zip.h"

#ifdef HAVE_AES
#  define AES_METHOD          (99)
#  define AES_PWVERIFYSIZE    (2)
#  define AES_AUTHCODESIZE    (10)
#  define AES_MAXSALTLENGTH   (16)
#  define AES_VERSION         (0x0001)
#  define AES_ENCRYPTIONMODE  (0x03)

#  include "aes/aes.h"
#  include "aes/fileenc.h"
#  include "aes/prng.h"
#endif
#ifdef HAVE_APPLE_COMPRESSION
#  include <compression.h>
#endif

#ifndef NOCRYPT
#  include "crypt.h"
#endif

#define SIZEDATA_INDATABLOCK        (4096-(4*4))

#define DISKHEADERMAGIC             (0x08074b50)
#define LOCALHEADERMAGIC            (0x04034b50)
#define CENTRALHEADERMAGIC          (0x02014b50)
#define ENDHEADERMAGIC              (0x06054b50)
#define ZIP64ENDHEADERMAGIC         (0x06064b50)
#define ZIP64ENDLOCHEADERMAGIC      (0x07064b50)
#define DATADESCRIPTORMAGIC         (0x08074b50)

#define FLAG_LOCALHEADER_OFFSET     (0x06)
#define CRC_LOCALHEADER_OFFSET      (0x0e)

#define SIZECENTRALHEADER           (0x2e) /* 46 */
#define SIZECENTRALHEADERLOCATOR    (0x14) /* 20 */
#define SIZECENTRALDIRITEM          (0x2e)
#define SIZEZIPLOCALHEADER          (0x1e)

#ifndef BUFREADCOMMENT
#  define BUFREADCOMMENT            (0x400)
#endif
#ifndef VERSIONMADEBY
#  define VERSIONMADEBY             (0x0) /* platform dependent */
#endif

#ifndef Z_BUFSIZE
#  define Z_BUFSIZE                 (UINT16_MAX)
#endif

#ifndef ALLOC
#  define ALLOC(size) (malloc(size))
#endif
#ifndef TRYFREE
#  define TRYFREE(p) {if (p) free(p);}
#endif

/* NOT sure that this work on ALL platform */
#define MAKEULONG64(a, b) ((uint64_t)(((unsigned long)(a)) | ((uint64_t)((unsigned long)(b))) << 32))

#ifndef DEF_MEM_LEVEL
#  if MAX_MEM_LEVEL >= 8
#    define DEF_MEM_LEVEL 8
#  else
#    define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#  endif
#endif

const char zip_copyright[] = " zip 1.01 Copyright 1998-2004 Gilles Vollant - http://www.winimage.com/zLibDll";

typedef struct linkedlist_datablock_internal_s
{
    struct linkedlist_datablock_internal_s *next_datablock;
    uint32_t    avail_in_this_block;
    uint32_t    filled_in_this_block;
    uint32_t    unused; /* for future use and alignment */
    uint8_t     data[SIZEDATA_INDATABLOCK];
} linkedlist_datablock_internal;

typedef struct linkedlist_data_s
{
    linkedlist_datablock_internal *first_block;
    linkedlist_datablock_internal *last_block;
} linkedlist_data;

typedef struct
{
    z_stream stream;                /* zLib stream structure for inflate */
#ifdef HAVE_BZIP2
    bz_stream bstream;              /* bzLib stream structure for bziped */
#endif
#ifdef HAVE_APPLE_COMPRESSION
    compression_stream astream;     /* libcompression stream structure */
#endif
#ifdef HAVE_AES
    fcrypt_ctx aes_ctx;
    prng_ctx aes_rng[1];
#endif
    int      stream_initialised;    /* 1 is stream is initialized */
    uint32_t pos_in_buffered_data;  /* last written byte in buffered_data */

    uint64_t pos_local_header;      /* offset of the local header of the file currently writing */
    char    *central_header;        /* central header data for the current file */
    uint16_t size_centralextra;
    uint16_t size_centralheader;    /* size of the central header for cur file */
    uint16_t size_centralextrafree; /* Extra bytes allocated to the central header but that are not used */
    uint16_t size_comment;
    uint16_t flag;                  /* flag of the file currently writing */

    uint16_t method;                /* compression method written to file.*/
    uint16_t compression_method;    /* compression method to use */
    int      raw;                   /* 1 for directly writing raw data */
    uint8_t  buffered_data[Z_BUFSIZE];  /* buffer contain compressed data to be writ*/
    uint32_t dos_date;
    uint32_t crc32;
    int      zip64;                 /* add ZIP64 extended information in the extra field */
    uint32_t number_disk;           /* number of current disk used for spanning ZIP */
    uint64_t total_compressed;
    uint64_t total_uncompressed;
#ifndef NOCRYPT
    uint32_t keys[3];          /* keys defining the pseudo-random sequence */
    const z_crc_t *pcrc_32_tab;
#endif
} curfile64_info;

typedef struct
{
    zlib_filefunc64_32_def z_filefunc;
    voidpf filestream;              /* io structure of the zipfile */
    voidpf filestream_with_CD;      /* io structure of the zipfile with the central dir */
    linkedlist_data central_dir;    /* datablock with central dir in construction*/
    int in_opened_file_inzip;       /* 1 if a file in the zip is currently writ.*/
    int append;                     /* append mode */
    curfile64_info ci;              /* info on the file currently writing */

    uint64_t add_position_when_writting_offset;
    uint64_t number_entry;
    uint64_t disk_size;             /* size of each disk */
    uint32_t number_disk;           /* number of the current disk, used for spanning ZIP */
    uint32_t number_disk_with_CD;   /* number the the disk with central dir, used for spanning ZIP */
#ifndef NO_ADDFILEINEXISTINGZIP
    char *globalcomment;
#endif
} zip64_internal;

/* Allocate a new data block */
static linkedlist_datablock_internal *allocate_new_datablock(void)
{
    linkedlist_datablock_internal *ldi = NULL;

    ldi = (linkedlist_datablock_internal*)ALLOC(sizeof(linkedlist_datablock_internal));

    if (ldi != NULL)
    {
        ldi->next_datablock = NULL;
        ldi->filled_in_this_block = 0;
        ldi->avail_in_this_block = SIZEDATA_INDATABLOCK;
    }
    return ldi;
}

/* Free data block in linked list */
static void free_datablock(linkedlist_datablock_internal *ldi)
{
    while (ldi != NULL)
    {
        linkedlist_datablock_internal *ldinext = ldi->next_datablock;
        TRYFREE(ldi);
        ldi = ldinext;
    }
}

/* Initialize linked list */
static void init_linkedlist(linkedlist_data *ll)
{
    ll->first_block = ll->last_block = NULL;
}

/* Free entire linked list and all data blocks */
static void free_linkedlist(linkedlist_data *ll)
{
    free_datablock(ll->first_block);
    ll->first_block = ll->last_block = NULL;
}

/* Add data to linked list data block */
static int add_data_in_datablock(linkedlist_data *ll, const void *buf, uint32_t len)
{
    linkedlist_datablock_internal *ldi = NULL;
    const unsigned char *from_copy = NULL;

    if (ll == NULL)
        return ZIP_INTERNALERROR;

    if (ll->last_block == NULL)
    {
        ll->first_block = ll->last_block = allocate_new_datablock();
        if (ll->first_block == NULL)
            return ZIP_INTERNALERROR;
    }

    ldi = ll->last_block;
    from_copy = (unsigned char*)buf;

    while (len > 0)
    {
        uint32_t copy_this = 0;
        uint32_t i = 0;
        unsigned char *to_copy = NULL;

        if (ldi->avail_in_this_block == 0)
        {
            ldi->next_datablock = allocate_new_datablock();
            if (ldi->next_datablock == NULL)
                return ZIP_INTERNALERROR;
            ldi = ldi->next_datablock ;
            ll->last_block = ldi;
        }

        if (ldi->avail_in_this_block < len)
            copy_this = ldi->avail_in_this_block;
        else
            copy_this = len;

        to_copy = &(ldi->data[ldi->filled_in_this_block]);

        for (i = 0; i < copy_this; i++)
            *(to_copy+i) = *(from_copy+i);

        ldi->filled_in_this_block += copy_this;
        ldi->avail_in_this_block -= copy_this;
        from_copy += copy_this;
        len -= copy_this;
    }
    return ZIP_OK;
}

/* Inputs a long in LSB order to the given file: nbByte == 1, 2 ,4 or 8 (byte, short or long, uint64_t) */
static int zipWriteValue(const zlib_filefunc64_32_def *pzlib_filefunc_def, voidpf filestream,
    uint64_t x, uint32_t len)
{
    unsigned char buf[8];
    uint32_t n = 0;

    for (n = 0; n < len; n++)
    {
        buf[n] = (unsigned char)(x & 0xff);
        x >>= 8;
    }

    if (x != 0)
    {
        /* Data overflow - hack for ZIP64 (X Roche) */
        for (n = 0; n < len; n++)
        {
            buf[n] = 0xff;
        }
    }

    if (ZWRITE64(*pzlib_filefunc_def, filestream, buf, len) != len)
        return ZIP_ERRNO;

    return ZIP_OK;
}

static void zipWriteValueToMemory(void* dest, uint64_t x, uint32_t len)
{
    unsigned char *buf = (unsigned char*)dest;
    uint32_t n = 0;

    for (n = 0; n < len; n++)
    {
        buf[n] = (unsigned char)(x & 0xff);
        x >>= 8;
    }

    if (x != 0)
    {
       /* data overflow - hack for ZIP64 */
       for (n = 0; n < len; n++)
       {
          buf[n] = 0xff;
       }
    }
}

static void zipWriteValueToMemoryAndMove(unsigned char **dest_ptr, uint64_t x, uint32_t len)
{
    zipWriteValueToMemory(*dest_ptr, x, len);
    *dest_ptr += len;
}

static int zipReadUInt8(const zlib_filefunc64_32_def *pzlib_filefunc_def, voidpf filestream, uint8_t *value)
{
    uint8_t c = 0;
    if (ZREAD64(*pzlib_filefunc_def, filestream, &c, 1) == 1)
    {
        *value = (uint8_t)c;
        return ZIP_OK;
    }
    if (ZERROR64(*pzlib_filefunc_def, filestream))
        return ZIP_ERRNO;
    return ZIP_EOF;
}

static int zipReadUInt16(const zlib_filefunc64_32_def *pzlib_filefunc_def, voidpf filestream, uint16_t *value)
{
    uint16_t x = 0;
    uint8_t c = 0;
    int err = ZIP_OK;

    err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x = (uint16_t)c;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint16_t)c) << 8;

    if (err == ZIP_OK)
        *value = x;
    else
        *value = 0;
    return err;
}

static int zipReadUInt32(const zlib_filefunc64_32_def *pzlib_filefunc_def, voidpf filestream, uint32_t *value)
{
    uint32_t x = 0;
    uint8_t c = 0;
    int err = ZIP_OK;

    err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x = (uint32_t)c;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint32_t)c) << 8;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint32_t)c) << 16;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint32_t)c) << 24;

    if (err == ZIP_OK)
        *value = x;
    else
        *value = 0;
    return err;
}

static int zipReadUInt64(const zlib_filefunc64_32_def *pzlib_filefunc_def, voidpf filestream, uint64_t *value)
{
    uint64_t x = 0;
    uint8_t c = 0;
    int err = ZIP_OK;

    err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x = (uint64_t)c;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint64_t)c) << 8;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint64_t)c) << 16;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint64_t)c) << 24;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint64_t)c) << 32;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint64_t)c) << 40;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint64_t)c) << 48;
    if (err == ZIP_OK)
        err = zipReadUInt8(pzlib_filefunc_def, filestream, &c);
    x += ((uint64_t)c) << 56;

    if (err == ZIP_OK)
        *value = x;
    else
        *value = 0;

    return err;
}

/* Gets the amount of bytes left to write to the current disk for spanning archives */
static void zipGetDiskSizeAvailable(zipFile file, uint64_t *size_available)
{
    zip64_internal *zi = NULL;
    uint64_t current_disk_size = 0;

    zi = (zip64_internal*)file;
    ZSEEK64(zi->z_filefunc, zi->filestream, 0, ZLIB_FILEFUNC_SEEK_END);
    current_disk_size = ZTELL64(zi->z_filefunc, zi->filestream);
    *size_available = zi->disk_size - current_disk_size;
}

/* Goes to a specific disk number for spanning archives */
static int zipGoToSpecificDisk(zipFile file, uint32_t number_disk, int open_existing)
{
    zip64_internal *zi = NULL;
    int err = ZIP_OK;

    zi = (zip64_internal*)file;
    if (zi->disk_size == 0)
        return err;

    if ((zi->filestream != NULL) && (zi->filestream != zi->filestream_with_CD))
        ZCLOSE64(zi->z_filefunc, zi->filestream);

    zi->filestream = ZOPENDISK64(zi->z_filefunc, zi->filestream_with_CD, number_disk, (open_existing == 1) ?
            (ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_EXISTING) :
            (ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_CREATE));

    if (zi->filestream == NULL)
        err = ZIP_ERRNO;

    return err;
}

/* Goes to the first disk in a spanned archive */
static int zipGoToFirstDisk(zipFile file)
{
    zip64_internal *zi = NULL;
    uint32_t number_disk_next = 0;
    int err = ZIP_OK;

    zi = (zip64_internal*)file;

    if (zi->disk_size == 0)
        return err;
    number_disk_next = 0;
    if (zi->number_disk_with_CD > 0)
        number_disk_next = zi->number_disk_with_CD - 1;
    err = zipGoToSpecificDisk(file, number_disk_next, (zi->append == APPEND_STATUS_ADDINZIP));
    if ((err == ZIP_ERRNO) && (zi->append == APPEND_STATUS_ADDINZIP))
        err = zipGoToSpecificDisk(file, number_disk_next, 0);
    if (err == ZIP_OK)
        zi->number_disk = number_disk_next;
    ZSEEK64(zi->z_filefunc, zi->filestream, 0, ZLIB_FILEFUNC_SEEK_END);
    return err;
}

/* Goes to the next disk in a spanned archive */
static int zipGoToNextDisk(zipFile file)
{
    zip64_internal *zi = NULL;
    uint64_t size_available_in_disk = 0;
    uint32_t number_disk_next = 0;
    int err = ZIP_OK;

    zi = (zip64_internal*)file;
    if (zi->disk_size == 0)
        return err;

    number_disk_next = zi->number_disk + 1;

    do
    {
        err = zipGoToSpecificDisk(file, number_disk_next, (zi->append == APPEND_STATUS_ADDINZIP));
        if ((err == ZIP_ERRNO) && (zi->append == APPEND_STATUS_ADDINZIP))
            err = zipGoToSpecificDisk(file, number_disk_next, 0);
        if (err != ZIP_OK)
            break;
        zipGetDiskSizeAvailable(file, &size_available_in_disk);
        zi->number_disk = number_disk_next;
        zi->number_disk_with_CD = zi->number_disk + 1;

        number_disk_next += 1;
    }
    while (size_available_in_disk <= 0);

    return err;
}

/* Locate the Central directory of a zipfile (at the end, just before the global comment) */
static uint64_t zipSearchCentralDir(const zlib_filefunc64_32_def *pzlib_filefunc_def, voidpf filestream)
{
    unsigned char *buf = NULL;
    uint64_t file_size = 0;
    uint64_t back_read = 4;
    uint64_t max_back = UINT16_MAX; /* maximum size of global comment */
    uint64_t pos_found = 0;
    uint32_t read_size = 0;
    uint64_t read_pos = 0;
    uint32_t i = 0;

    buf = (unsigned char*)ALLOC(BUFREADCOMMENT+4);
    if (buf == NULL)
        return 0;

    if (ZSEEK64(*pzlib_filefunc_def, filestream, 0, ZLIB_FILEFUNC_SEEK_END) != 0)
    {
        TRYFREE(buf);
        return 0;
    }

    file_size = ZTELL64(*pzlib_filefunc_def, filestream);

    if (max_back > file_size)
        max_back = file_size;

    while (back_read < max_back)
    {
        if (back_read + BUFREADCOMMENT > max_back)
            back_read = max_back;
        else
            back_read += BUFREADCOMMENT;

        read_pos = file_size-back_read;
        read_size = ((BUFREADCOMMENT+4) < (file_size - read_pos)) ?
                     (BUFREADCOMMENT+4) : (uint32_t)(file_size - read_pos);

        if (ZSEEK64(*pzlib_filefunc_def, filestream, read_pos, ZLIB_FILEFUNC_SEEK_SET) != 0)
            break;
        if (ZREAD64(*pzlib_filefunc_def, filestream, buf, read_size) != read_size)
            break;

        for (i = read_size-3; (i--) > 0;)
            if ((*(buf+i)) == (ENDHEADERMAGIC & 0xff) &&
                (*(buf+i+1)) == (ENDHEADERMAGIC >> 8 & 0xff) &&
                (*(buf+i+2)) == (ENDHEADERMAGIC >> 16 & 0xff) &&
                (*(buf+i+3)) == (ENDHEADERMAGIC >> 24 & 0xff))
            {
                pos_found = read_pos+i;
                break;
            }

        if (pos_found != 0)
            break;
    }
    TRYFREE(buf);
    return pos_found;
}

/* Locate the Central directory 64 of a zipfile (at the end, just before the global comment) */
static uint64_t zipSearchCentralDir64(const zlib_filefunc64_32_def *pzlib_filefunc_def, voidpf filestream,
    const uint64_t endcentraloffset)
{
    uint64_t offset = 0;
    uint32_t value32 = 0;

    /* Zip64 end of central directory locator */
    if (ZSEEK64(*pzlib_filefunc_def, filestream, endcentraloffset - SIZECENTRALHEADERLOCATOR, ZLIB_FILEFUNC_SEEK_SET) != 0)
        return 0;

    /* Read locator signature */
    if (zipReadUInt32(pzlib_filefunc_def, filestream, &value32) != ZIP_OK)
        return 0;
    if (value32 != ZIP64ENDLOCHEADERMAGIC)
        return 0;
    /* Number of the disk with the start of the zip64 end of  central directory */
    if (zipReadUInt32(pzlib_filefunc_def, filestream, &value32) != ZIP_OK)
        return 0;
    /* Relative offset of the zip64 end of central directory record */
    if (zipReadUInt64(pzlib_filefunc_def, filestream, &offset) != ZIP_OK)
        return 0;
    /* Total number of disks */
    if (zipReadUInt32(pzlib_filefunc_def, filestream, &value32) != ZIP_OK)
        return 0;
    /* Goto end of central directory record */
    if (ZSEEK64(*pzlib_filefunc_def,filestream, offset, ZLIB_FILEFUNC_SEEK_SET) != 0)
        return 0;
    /* The signature */
    if (zipReadUInt32(pzlib_filefunc_def, filestream, &value32) != ZIP_OK)
        return 0;
    if (value32 != ZIP64ENDHEADERMAGIC)
        return 0;

    return offset;
}

extern zipFile ZEXPORT zipOpen4(const void *path, int append, uint64_t disk_size, const char **globalcomment,
    zlib_filefunc64_32_def *pzlib_filefunc64_32_def)
{
    zip64_internal ziinit;
    zip64_internal *zi = NULL;
#ifndef NO_ADDFILEINEXISTINGZIP
    uint64_t byte_before_the_zipfile = 0;   /* byte before the zipfile, (>0 for sfx)*/
    uint64_t size_central_dir = 0;          /* size of the central directory  */
    uint64_t offset_central_dir = 0;        /* offset of start of central directory */
    uint64_t number_entry_CD = 0;           /* total number of entries in the central dir */
    uint64_t number_entry = 0;
    uint64_t central_pos = 0;
    uint64_t size_central_dir_to_read = 0;
    uint16_t value16 = 0;
    uint32_t value32 = 0;
    uint16_t size_comment = 0;
    size_t buf_size = SIZEDATA_INDATABLOCK;
    void *buf_read = NULL;
#endif
    int err = ZIP_OK;
    int mode = 0;

    ziinit.z_filefunc.zseek32_file = NULL;
    ziinit.z_filefunc.ztell32_file = NULL;

    if (pzlib_filefunc64_32_def == NULL)
        fill_fopen64_filefunc(&ziinit.z_filefunc.zfile_func64);
    else
        ziinit.z_filefunc = *pzlib_filefunc64_32_def;

    if (append == APPEND_STATUS_CREATE)
        mode = (ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_CREATE);
    else
        mode = (ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_EXISTING);

    ziinit.filestream = ZOPEN64(ziinit.z_filefunc, path, mode);
    if (ziinit.filestream == NULL)
        return NULL;

    if (append == APPEND_STATUS_CREATEAFTER)
    {
        /* Don't support spanning ZIP with APPEND_STATUS_CREATEAFTER */
        if (disk_size > 0)
            return NULL;

        ZSEEK64(ziinit.z_filefunc,ziinit.filestream,0,SEEK_END);
    }

    ziinit.filestream_with_CD = ziinit.filestream;
    ziinit.append = append;
    ziinit.number_disk = 0;
    ziinit.number_disk_with_CD = 0;
    ziinit.disk_size = disk_size;
    ziinit.in_opened_file_inzip = 0;
    ziinit.ci.stream_initialised = 0;
    ziinit.number_entry = 0;
    ziinit.add_position_when_writting_offset = 0;
    init_linkedlist(&(ziinit.central_dir));

    zi = (zip64_internal*)ALLOC(sizeof(zip64_internal));
    if (zi == NULL)
    {
        ZCLOSE64(ziinit.z_filefunc,ziinit.filestream);
        return NULL;
    }

#ifndef NO_ADDFILEINEXISTINGZIP
    /* Add file in a zipfile */
    ziinit.globalcomment = NULL;
    if (append == APPEND_STATUS_ADDINZIP)
    {
        /* Read and Cache Central Directory Records */
        central_pos = zipSearchCentralDir(&ziinit.z_filefunc,ziinit.filestream);
        /* Disable to allow appending to empty ZIP archive (must be standard zip, not zip64)
            if (central_pos == 0)
                err = ZIP_ERRNO;
        */

        if (err == ZIP_OK)
        {
            /* Read end of central directory info */
            if (ZSEEK64(ziinit.z_filefunc, ziinit.filestream, central_pos,ZLIB_FILEFUNC_SEEK_SET) != 0)
                err = ZIP_ERRNO;

            /* The signature, already checked */
            if (zipReadUInt32(&ziinit.z_filefunc, ziinit.filestream, &value32) != ZIP_OK)
                err = ZIP_ERRNO;
            /* Number of this disk */
            if (zipReadUInt16(&ziinit.z_filefunc, ziinit.filestream, &value16) != ZIP_OK)
                err = ZIP_ERRNO;
            ziinit.number_disk = value16;
            /* Number of the disk with the start of the central directory */
            if (zipReadUInt16(&ziinit.z_filefunc, ziinit.filestream, &value16) != ZIP_OK)
                err = ZIP_ERRNO;
            ziinit.number_disk_with_CD = value16;
            /* Total number of entries in the central dir on this disk */
            number_entry = 0;
            if (zipReadUInt16(&ziinit.z_filefunc, ziinit.filestream, &value16) != ZIP_OK)
                err = ZIP_ERRNO;
            else
                number_entry = value16;
            /* Total number of entries in the central dir */
            number_entry_CD = 0;
            if (zipReadUInt16(&ziinit.z_filefunc, ziinit.filestream, &value16) != ZIP_OK)
                err = ZIP_ERRNO;
            else
                number_entry_CD = value16;
            if (number_entry_CD!=number_entry)
                err = ZIP_BADZIPFILE;
            /* Size of the central directory */
            size_central_dir = 0;
            if (zipReadUInt32(&ziinit.z_filefunc, ziinit.filestream, &value32) != ZIP_OK)
                err = ZIP_ERRNO;
            else
                size_central_dir = value32;
            /* Offset of start of central directory with respect to the starting disk number */
            offset_central_dir = 0;
            if (zipReadUInt32(&ziinit.z_filefunc, ziinit.filestream, &value32) != ZIP_OK)
                err = ZIP_ERRNO;
            else
                offset_central_dir = value32;
            /* Zipfile global comment length */
            if (zipReadUInt16(&ziinit.z_filefunc, ziinit.filestream, &size_comment) != ZIP_OK)
                err = ZIP_ERRNO;

            if ((err == ZIP_OK) && ((number_entry_CD == UINT16_MAX) || (offset_central_dir == UINT32_MAX)))
            {
                /* Format should be Zip64, as the central directory or file size is too large */
                central_pos = zipSearchCentralDir64(&ziinit.z_filefunc, ziinit.filestream, central_pos);

                if (central_pos)
                {
                    uint64_t sizeEndOfCentralDirectory;

                    if (ZSEEK64(ziinit.z_filefunc, ziinit.filestream, central_pos, ZLIB_FILEFUNC_SEEK_SET) != 0)
                        err = ZIP_ERRNO;

                    /* The signature, already checked */
                    if (zipReadUInt32(&ziinit.z_filefunc, ziinit.filestream, &value32) != ZIP_OK)
                        err = ZIP_ERRNO;
                    /* Size of zip64 end of central directory record */
                    if (zipReadUInt64(&ziinit.z_filefunc, ziinit.filestream, &sizeEndOfCentralDirectory) != ZIP_OK)
                        err = ZIP_ERRNO;
                    /* Version made by */
                    if (zipReadUInt16(&ziinit.z_filefunc, ziinit.filestream, &value16) != ZIP_OK)
                        err = ZIP_ERRNO;
                    /* Version needed to extract */
                    if (zipReadUInt16(&ziinit.z_filefunc, ziinit.filestream, &value16) != ZIP_OK)
                        err = ZIP_ERRNO;
                    /* Number of this disk */
                    if (zipReadUInt32(&ziinit.z_filefunc, ziinit.filestream, &ziinit.number_disk) != ZIP_OK)
                        err = ZIP_ERRNO;
                    /* Number of the disk with the start of the central directory */
                    if (zipReadUInt32(&ziinit.z_filefunc, ziinit.filestream, &ziinit.number_disk_with_CD) != ZIP_OK)
                        err = ZIP_ERRNO;
                    /* Total number of entries in the central directory on this disk */
                    if (zipReadUInt64(&ziinit.z_filefunc, ziinit.filestream, &number_entry) != ZIP_OK)
                        err = ZIP_ERRNO;
                    /* Total number of entries in the central directory */
                    if (zipReadUInt64(&ziinit.z_filefunc, ziinit.filestream, &number_entry_CD) != ZIP_OK)
                        err = ZIP_ERRNO;
                    if (number_entry_CD!=number_entry)
                        err = ZIP_BADZIPFILE;
                    /* Size of the central directory */
                    if (zipReadUInt64(&ziinit.z_filefunc, ziinit.filestream, &size_central_dir) != ZIP_OK)
                        err = ZIP_ERRNO;
                    /* Offset of start of central directory with respect to the starting disk number */
                    if (zipReadUInt64(&ziinit.z_filefunc, ziinit.filestream, &offset_central_dir) != ZIP_OK)
                        err = ZIP_ERRNO;
                }
                else
                    err = ZIP_BADZIPFILE;
             }
        }

        if ((err == ZIP_OK) && (central_pos < offset_central_dir + size_central_dir))
            err = ZIP_BADZIPFILE;

        if ((err == ZIP_OK) && (size_comment > 0))
        {
            ziinit.globalcomment = (char*)ALLOC(size_comment+1);
            if (ziinit.globalcomment)
            {
                if (ZREAD64(ziinit.z_filefunc, ziinit.filestream, ziinit.globalcomment, size_comment) != size_comment)
                    err = ZIP_ERRNO;
                else
                    ziinit.globalcomment[size_comment] = 0;
            }
        }

        if (err != ZIP_OK)
        {
            ZCLOSE64(ziinit.z_filefunc, ziinit.filestream);
            TRYFREE(ziinit.globalcomment);
            TRYFREE(zi);
            return NULL;
        }

        byte_before_the_zipfile = central_pos - (offset_central_dir+size_central_dir);
        ziinit.add_position_when_writting_offset = byte_before_the_zipfile;

        /* Store central directory in memory */
        size_central_dir_to_read = size_central_dir;
        buf_size = SIZEDATA_INDATABLOCK;
        buf_read = (void*)ALLOC(buf_size);

        if (ZSEEK64(ziinit.z_filefunc, ziinit.filestream,
                offset_central_dir + byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET) != 0)
            err = ZIP_ERRNO;

        while ((size_central_dir_to_read > 0) && (err == ZIP_OK))
        {
            uint64_t read_this = SIZEDATA_INDATABLOCK;
            if (read_this > size_central_dir_to_read)
                read_this = size_central_dir_to_read;

            if (ZREAD64(ziinit.z_filefunc, ziinit.filestream, buf_read, (uint32_t)read_this) != read_this)
                err = ZIP_ERRNO;

            if (err == ZIP_OK)
                err = add_data_in_datablock(&ziinit.central_dir, buf_read, (uint32_t)read_this);

            size_central_dir_to_read -= read_this;
        }
        TRYFREE(buf_read);

        ziinit.number_entry = number_entry_CD;

        if (ZSEEK64(ziinit.z_filefunc, ziinit.filestream,
                offset_central_dir+byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET) != 0)
            err = ZIP_ERRNO;
    }

    if (globalcomment)
        *globalcomment = ziinit.globalcomment;
#endif

    if (err != ZIP_OK)
    {
#ifndef NO_ADDFILEINEXISTINGZIP
        TRYFREE(ziinit.globalcomment);
#endif
        TRYFREE(zi);
        return NULL;
    }

    *zi = ziinit;
    zipGoToFirstDisk((zipFile)zi);
    return(zipFile)zi;
}

extern zipFile ZEXPORT zipOpen2(const char *path, int append, const char **globalcomment,
    zlib_filefunc_def *pzlib_filefunc32_def)
{
    if (pzlib_filefunc32_def != NULL)
    {
        zlib_filefunc64_32_def zlib_filefunc64_32_def_fill;
        fill_zlib_filefunc64_32_def_from_filefunc32(&zlib_filefunc64_32_def_fill,pzlib_filefunc32_def);
        return zipOpen4(path, append, 0, globalcomment, &zlib_filefunc64_32_def_fill);
    }
    return zipOpen4(path, append, 0, globalcomment, NULL);
}

extern zipFile ZEXPORT zipOpen2_64(const void *path, int append, const char **globalcomment,
    zlib_filefunc64_def *pzlib_filefunc_def)
{
    if (pzlib_filefunc_def != NULL)
    {
        zlib_filefunc64_32_def zlib_filefunc64_32_def_fill;
        zlib_filefunc64_32_def_fill.zfile_func64 = *pzlib_filefunc_def;
        zlib_filefunc64_32_def_fill.ztell32_file = NULL;
        zlib_filefunc64_32_def_fill.zseek32_file = NULL;
        return zipOpen4(path, append, 0, globalcomment, &zlib_filefunc64_32_def_fill);
    }
    return zipOpen4(path, append, 0, globalcomment, NULL);
}

extern zipFile ZEXPORT zipOpen3(const char *path, int append, uint64_t disk_size, const char **globalcomment,
    zlib_filefunc_def *pzlib_filefunc32_def)
{
    if (pzlib_filefunc32_def != NULL)
    {
        zlib_filefunc64_32_def zlib_filefunc64_32_def_fill;
        fill_zlib_filefunc64_32_def_from_filefunc32(&zlib_filefunc64_32_def_fill,pzlib_filefunc32_def);
        return zipOpen4(path, append, disk_size, globalcomment, &zlib_filefunc64_32_def_fill);
    }
    return zipOpen4(path, append, disk_size, globalcomment, NULL);
}

extern zipFile ZEXPORT zipOpen3_64(const void *path, int append, uint64_t disk_size, const char **globalcomment,
    zlib_filefunc64_def *pzlib_filefunc_def)
{
    if (pzlib_filefunc_def != NULL)
    {
        zlib_filefunc64_32_def zlib_filefunc64_32_def_fill;
        zlib_filefunc64_32_def_fill.zfile_func64 = *pzlib_filefunc_def;
        zlib_filefunc64_32_def_fill.ztell32_file = NULL;
        zlib_filefunc64_32_def_fill.zseek32_file = NULL;
        return zipOpen4(path, append, disk_size, globalcomment, &zlib_filefunc64_32_def_fill);
    }
    return zipOpen4(path, append, disk_size, globalcomment, NULL);
}

extern zipFile ZEXPORT zipOpen(const char *path, int append)
{
    return zipOpen3(path, append, 0, NULL, NULL);
}

extern zipFile ZEXPORT zipOpen64(const void *path, int append)
{
    return zipOpen3_64(path, append, 0, NULL, NULL);
}

extern int ZEXPORT zipOpenNewFileInZip_internal(zipFile file,
                                                const char *filename,
                                                const zip_fileinfo *zipfi,
                                                const void *extrafield_local,
                                                uint16_t size_extrafield_local,
                                                const void *extrafield_global,
                                                uint16_t size_extrafield_global,
                                                const char *comment,
                                                uint16_t flag_base,
                                                int zip64,
                                                uint16_t method,
                                                int level,
                                                int raw,
                                                int windowBits,
                                                int memLevel,
                                                int strategy,
                                                const char *password,
                                                int aes,
                                                uint16_t version_madeby)
{
    zip64_internal *zi = NULL;
    uint64_t size_available = 0;
    uint64_t size_needed = 0;
    uint16_t size_filename = 0;
    uint16_t size_comment = 0;
    uint16_t i = 0;
    unsigned char *central_dir = NULL;
    int err = ZIP_OK;

#ifdef NOCRYPT
    if (password != NULL)
        return ZIP_PARAMERROR;
#endif

    if (file == NULL)
        return ZIP_PARAMERROR;

    if ((method != 0) &&
#ifdef HAVE_BZIP2
        (method != Z_BZIP2ED) &&
#endif
        (method != Z_DEFLATED))
        return ZIP_PARAMERROR;

    zi = (zip64_internal*)file;

    if (zi->in_opened_file_inzip == 1)
    {
        err = zipCloseFileInZip (file);
        if (err != ZIP_OK)
            return err;
    }

    if (filename == NULL)
        filename = "-";
    if (comment != NULL)
        size_comment = (uint16_t)strlen(comment);

    size_filename = (uint16_t)strlen(filename);

    if (zipfi == NULL)
        zi->ci.dos_date = 0;
    else
    {
        if (zipfi->dos_date != 0)
            zi->ci.dos_date = zipfi->dos_date;
    }

    zi->ci.method = method;
    zi->ci.compression_method = method;
    zi->ci.raw = raw;
    zi->ci.flag = flag_base | 8;
    if ((level == 8) || (level == 9))
        zi->ci.flag |= 2;
    if (level == 2)
        zi->ci.flag |= 4;
    if (level == 1)
        zi->ci.flag |= 6;

    if (password != NULL)
    {
        zi->ci.flag |= 1;
#ifdef HAVE_AES
        if (aes)
            zi->ci.method = AES_METHOD;
#endif
    }
    else
    {
        zi->ci.flag &= ~1;
    }

    if (zi->disk_size > 0)
    {
        if ((zi->number_disk == 0) && (zi->number_entry == 0))
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)DISKHEADERMAGIC, 4);

        /* Make sure enough space available on current disk for local header */
        zipGetDiskSizeAvailable((zipFile)zi, &size_available);
        size_needed = 30 + size_filename + size_extrafield_local;
#ifdef HAVE_AES
        if (zi->ci.method == AES_METHOD)
            size_needed += 11;
#endif
        if (size_available < size_needed)
            zipGoToNextDisk((zipFile)zi);
    }

    zi->ci.zip64 = zip64;

    zi->ci.pos_local_header = ZTELL64(zi->z_filefunc, zi->filestream);
    if (zi->ci.pos_local_header >= UINT32_MAX)
        zi->ci.zip64 = 1;

    zi->ci.size_comment = size_comment;
    zi->ci.size_centralheader = SIZECENTRALHEADER + size_filename + size_extrafield_global;
    zi->ci.size_centralextra = size_extrafield_global;
    zi->ci.size_centralextrafree = 32; /* Extra space reserved for ZIP64 extra info */
#ifdef HAVE_AES
    if (zi->ci.method == AES_METHOD)
        zi->ci.size_centralextrafree += 11; /* Extra space reserved for AES extra info */
#endif
    zi->ci.central_header = (char*)ALLOC((uint32_t)zi->ci.size_centralheader + zi->ci.size_centralextrafree + size_comment);
    zi->ci.number_disk = zi->number_disk;

    /* Write central directory header */
    central_dir = (unsigned char*)zi->ci.central_header;
    zipWriteValueToMemoryAndMove(&central_dir, (uint32_t)CENTRALHEADERMAGIC, 4);
    zipWriteValueToMemoryAndMove(&central_dir, version_madeby, 2);
    if (zi->ci.zip64)
        zipWriteValueToMemoryAndMove(&central_dir, (uint16_t)45, 2);
    else
        zipWriteValueToMemoryAndMove(&central_dir, (uint16_t)20, 2);
    zipWriteValueToMemoryAndMove(&central_dir, zi->ci.flag, 2);
    zipWriteValueToMemoryAndMove(&central_dir, zi->ci.method, 2);
    zipWriteValueToMemoryAndMove(&central_dir, zi->ci.dos_date, 4);
    zipWriteValueToMemoryAndMove(&central_dir, (uint32_t)0, 4); /*crc*/
    zipWriteValueToMemoryAndMove(&central_dir, (uint32_t)0, 4); /*compr size*/
    zipWriteValueToMemoryAndMove(&central_dir, (uint32_t)0, 4); /*uncompr size*/
    zipWriteValueToMemoryAndMove(&central_dir, size_filename, 2);
    zipWriteValueToMemoryAndMove(&central_dir, size_extrafield_global, 2);
    zipWriteValueToMemoryAndMove(&central_dir, size_comment, 2);
    zipWriteValueToMemoryAndMove(&central_dir, (uint16_t)zi->ci.number_disk, 2); /*disk nm start*/

    if (zipfi == NULL)
        zipWriteValueToMemoryAndMove(&central_dir, (uint16_t)0, 2);
    else
        zipWriteValueToMemoryAndMove(&central_dir, zipfi->internal_fa, 2);
    if (zipfi == NULL)
        zipWriteValueToMemoryAndMove(&central_dir, (uint32_t)0, 4);
    else
        zipWriteValueToMemoryAndMove(&central_dir, zipfi->external_fa, 4);
    if (zi->ci.pos_local_header >= UINT32_MAX)
        zipWriteValueToMemoryAndMove(&central_dir, UINT32_MAX, 4);
    else
        zipWriteValueToMemoryAndMove(&central_dir,
            (uint32_t)(zi->ci.pos_local_header - zi->add_position_when_writting_offset), 4);

    for (i = 0; i < size_filename; i++)
        zi->ci.central_header[SIZECENTRALHEADER+i] = filename[i];
    for (i = 0; i < size_extrafield_global; i++)
        zi->ci.central_header[SIZECENTRALHEADER+size_filename+i] =
            ((const char*)extrafield_global)[i];

    /* Store comment at the end for later repositioning */
    for (i = 0; i < size_comment; i++)
        zi->ci.central_header[zi->ci.size_centralheader+
            zi->ci.size_centralextrafree+i] = comment[i];

    if (zi->ci.central_header == NULL)
        return ZIP_INTERNALERROR;

    /* Write the local header */
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)LOCALHEADERMAGIC, 4);

    if (err == ZIP_OK)
    {
        if (zi->ci.zip64)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint16_t)45, 2); /* version needed to extract */
        else
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint16_t)20, 2); /* version needed to extract */
    }
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->ci.flag, 2);
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->ci.method, 2);
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->ci.dos_date, 4);

    /* CRC & compressed size & uncompressed size is in data descriptor */
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)0, 4); /* crc 32, unknown */
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)0, 4); /* compressed size, unknown */
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)0, 4); /* uncompressed size, unknown */
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, size_filename, 2);
    if (err == ZIP_OK)
    {
        uint64_t size_extrafield = size_extrafield_local;
#ifdef HAVE_AES
        if (zi->ci.method == AES_METHOD)
            size_extrafield += 11;
#endif
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint16_t)size_extrafield, 2);
    }
    if ((err == ZIP_OK) && (size_filename > 0))
    {
        if (ZWRITE64(zi->z_filefunc, zi->filestream, filename, size_filename) != size_filename)
            err = ZIP_ERRNO;
    }
    if ((err == ZIP_OK) && (size_extrafield_local > 0))
    {
        if (ZWRITE64(zi->z_filefunc, zi->filestream, extrafield_local, size_extrafield_local) != size_extrafield_local)
            err = ZIP_ERRNO;
    }

#ifdef HAVE_AES
    /* Write the AES extended info */
    if ((err == ZIP_OK) && (zi->ci.method == AES_METHOD))
    {
        int headerid = 0x9901;
        short datasize = 7;

        err = zipWriteValue(&zi->z_filefunc, zi->filestream, headerid, 2);
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, datasize, 2);
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, AES_VERSION, 2);
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, 'A', 1);
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, 'E', 1);
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, AES_ENCRYPTIONMODE, 1);
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->ci.compression_method, 2);
    }
#endif

    zi->ci.crc32 = 0;
    zi->ci.stream_initialised = 0;
    zi->ci.pos_in_buffered_data = 0;
    zi->ci.total_compressed = 0;
    zi->ci.total_uncompressed = 0;

#ifdef HAVE_BZIP2
    zi->ci.bstream.avail_in = (uint16_t)0;
    zi->ci.bstream.avail_out = (uint16_t)Z_BUFSIZE;
    zi->ci.bstream.next_out = (char*)zi->ci.buffered_data;
    zi->ci.bstream.total_in_hi32 = 0;
    zi->ci.bstream.total_in_lo32 = 0;
    zi->ci.bstream.total_out_hi32 = 0;
    zi->ci.bstream.total_out_lo32 = 0;
#endif

    zi->ci.stream.avail_in = (uint16_t)0;
    zi->ci.stream.avail_out = Z_BUFSIZE;
    zi->ci.stream.next_out = zi->ci.buffered_data;
    zi->ci.stream.total_in = 0;
    zi->ci.stream.total_out = 0;
    zi->ci.stream.data_type = Z_BINARY;

    if ((err == ZIP_OK) && (!zi->ci.raw))
    {
        if (method == Z_DEFLATED)
        {
            zi->ci.stream.zalloc = (alloc_func)0;
            zi->ci.stream.zfree = (free_func)0;
            zi->ci.stream.opaque = (voidpf)zi;

            if (windowBits > 0)
                windowBits = -windowBits;

#ifdef HAVE_APPLE_COMPRESSION
            err = compression_stream_init(&zi->ci.astream, COMPRESSION_STREAM_ENCODE, COMPRESSION_ZLIB);
            if (err == COMPRESSION_STATUS_ERROR)
                err = Z_ERRNO;
            else
                err = Z_OK;
#else
            err = deflateInit2(&zi->ci.stream, level, Z_DEFLATED, windowBits, memLevel, strategy);
#endif
            if (err == Z_OK)
                zi->ci.stream_initialised = Z_DEFLATED;
        }
        else if (method == Z_BZIP2ED)
        {
#ifdef HAVE_BZIP2
            zi->ci.bstream.bzalloc = 0;
            zi->ci.bstream.bzfree = 0;
            zi->ci.bstream.opaque = (voidpf)0;

            err = BZ2_bzCompressInit(&zi->ci.bstream, level, 0, 35);
            if (err == BZ_OK)
                zi->ci.stream_initialised = Z_BZIP2ED;
#endif
        }
    }

#ifndef NOCRYPT
    if ((err == Z_OK) && (password != NULL))
    {
#ifdef HAVE_AES
        if (zi->ci.method == AES_METHOD)
        {
            unsigned char passverify[AES_PWVERIFYSIZE];
            unsigned char saltvalue[AES_MAXSALTLENGTH];
            uint16_t saltlength = 0;

            if ((AES_ENCRYPTIONMODE < 1) || (AES_ENCRYPTIONMODE > 3))
                return Z_ERRNO;

            saltlength = SALT_LENGTH(AES_ENCRYPTIONMODE);

            prng_init(cryptrand, zi->ci.aes_rng);
            prng_rand(saltvalue, saltlength, zi->ci.aes_rng);
            prng_end(zi->ci.aes_rng);

            fcrypt_init(AES_ENCRYPTIONMODE, (uint8_t *)password, (uint32_t)strlen(password), saltvalue, passverify, &zi->ci.aes_ctx);

            if (ZWRITE64(zi->z_filefunc, zi->filestream, saltvalue, saltlength) != saltlength)
                err = ZIP_ERRNO;
            if (ZWRITE64(zi->z_filefunc, zi->filestream, passverify, AES_PWVERIFYSIZE) != AES_PWVERIFYSIZE)
                err = ZIP_ERRNO;

            zi->ci.total_compressed += saltlength + AES_PWVERIFYSIZE + AES_AUTHCODESIZE;
        }
        else
#endif
        {
            unsigned char buf_head[RAND_HEAD_LEN];
            uint32_t size_head = 0;
            uint8_t verify1 = 0;
            uint8_t verify2 = 0;

            zi->ci.pcrc_32_tab = get_crc_table();

            /*
            Info-ZIP modification to ZipCrypto format:
            If bit 3 of the general purpose bit flag is set, it uses high byte of 16-bit File Time. 
            */
            verify1 = (uint8_t)((zi->ci.dos_date >> 16) & 0xff);
            verify2 = (uint8_t)((zi->ci.dos_date >> 8) & 0xff);

            size_head = crypthead(password, buf_head, RAND_HEAD_LEN, zi->ci.keys, zi->ci.pcrc_32_tab, verify1, verify2);
            zi->ci.total_compressed += size_head;

            if (ZWRITE64(zi->z_filefunc, zi->filestream, buf_head, size_head) != size_head)
                err = ZIP_ERRNO;
        }
    }
#endif

    if (err == Z_OK)
        zi->in_opened_file_inzip = 1;
    return err;
}

extern int ZEXPORT zipOpenNewFileInZip5(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t flag_base, int zip64, uint16_t method, int level, int raw,
    int windowBits, int memLevel, int strategy, const char *password, int aes)
{
    return zipOpenNewFileInZip_internal(file, filename, zipfi, extrafield_local, size_extrafield_local, extrafield_global,
        size_extrafield_global, comment, flag_base, zip64, method, level, raw, windowBits, memLevel, strategy, password, aes,
        VERSIONMADEBY);
}

extern int ZEXPORT zipOpenNewFileInZip4_64(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw, int windowBits, int memLevel,
    int strategy, const char *password, ZIP_UNUSED uint32_t crc_for_crypting, uint16_t version_madeby, uint16_t flag_base, int zip64)
{
    uint8_t aes = 0;
#ifdef HAVE_AES
    aes = 1;
#endif
    return zipOpenNewFileInZip_internal(file, filename, zipfi, extrafield_local, size_extrafield_local, extrafield_global,
        size_extrafield_global, comment, flag_base, zip64, method, level, raw, windowBits, memLevel, strategy, password, aes,
        version_madeby);
}

extern int ZEXPORT zipOpenNewFileInZip4(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw, int windowBits,
    int memLevel, int strategy, const char *password, ZIP_UNUSED uint32_t crc_for_crypting, uint16_t version_madeby, uint16_t flag_base)
{
    return zipOpenNewFileInZip4_64(file, filename, zipfi, extrafield_local, size_extrafield_local,
        extrafield_global, size_extrafield_global, comment, method, level, raw, windowBits, memLevel,
        strategy, password, crc_for_crypting, version_madeby, flag_base, 0);
}

extern int ZEXPORT zipOpenNewFileInZip3(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw, int windowBits,
    int memLevel, int strategy, const char *password, ZIP_UNUSED uint32_t crc_for_crypting)
{
    return zipOpenNewFileInZip4_64(file, filename, zipfi, extrafield_local, size_extrafield_local,
        extrafield_global, size_extrafield_global, comment, method, level, raw, windowBits, memLevel,
        strategy, password, crc_for_crypting, VERSIONMADEBY, 0, 0);
}

extern int ZEXPORT zipOpenNewFileInZip3_64(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw, int windowBits,
    int memLevel, int strategy, const char *password, ZIP_UNUSED uint32_t crc_for_crypting, int zip64)
{
    return zipOpenNewFileInZip4_64(file, filename, zipfi, extrafield_local, size_extrafield_local,
        extrafield_global, size_extrafield_global, comment, method, level, raw, windowBits, memLevel, strategy,
        password, crc_for_crypting, VERSIONMADEBY, 0, zip64);
}

extern int ZEXPORT zipOpenNewFileInZip2(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw)
{
    return zipOpenNewFileInZip4_64(file, filename, zipfi, extrafield_local, size_extrafield_local,
        extrafield_global, size_extrafield_global, comment, method, level, raw, -MAX_WBITS, DEF_MEM_LEVEL,
        Z_DEFAULT_STRATEGY, NULL, 0, VERSIONMADEBY, 0, 0);
}

extern int ZEXPORT zipOpenNewFileInZip2_64(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw, int zip64)
{
    return zipOpenNewFileInZip4_64(file, filename, zipfi, extrafield_local, size_extrafield_local,
        extrafield_global, size_extrafield_global, comment, method, level, raw, -MAX_WBITS, DEF_MEM_LEVEL,
        Z_DEFAULT_STRATEGY, NULL, 0, VERSIONMADEBY, 0, zip64);
}

extern int ZEXPORT zipOpenNewFileInZip64(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int zip64)
{
    return zipOpenNewFileInZip4_64(file, filename, zipfi, extrafield_local, size_extrafield_local,
        extrafield_global, size_extrafield_global, comment, method, level, 0, -MAX_WBITS, DEF_MEM_LEVEL,
        Z_DEFAULT_STRATEGY, NULL, 0, VERSIONMADEBY, 0, zip64);
}

extern int ZEXPORT zipOpenNewFileInZip(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level)
{
    return zipOpenNewFileInZip4_64(file, filename, zipfi, extrafield_local, size_extrafield_local,
        extrafield_global, size_extrafield_global, comment, method, level, 0, -MAX_WBITS, DEF_MEM_LEVEL,
        Z_DEFAULT_STRATEGY, NULL, 0, VERSIONMADEBY, 0, 0);
}

/* Flushes the write buffer to disk */
static int zipFlushWriteBuffer(zip64_internal *zi)
{
    uint64_t size_available = 0;
    uint32_t written = 0;
    uint32_t total_written = 0;
    uint32_t write = 0;
    uint32_t max_write = 0;
    int err = ZIP_OK;

    if ((zi->ci.flag & 1) != 0)
    {
#ifndef NOCRYPT
#ifdef HAVE_AES
        if (zi->ci.method == AES_METHOD)
        {
            fcrypt_encrypt(zi->ci.buffered_data, zi->ci.pos_in_buffered_data, &zi->ci.aes_ctx);
        }
        else
#endif
        {
            uint32_t i = 0;
            uint8_t t = 0;

            for (i = 0; i < zi->ci.pos_in_buffered_data; i++)
                zi->ci.buffered_data[i] = (uint8_t)zencode(zi->ci.keys, zi->ci.pcrc_32_tab, zi->ci.buffered_data[i], t);
        }
#endif
    }

    write = zi->ci.pos_in_buffered_data;

    do
    {
        max_write = write;

        if (zi->disk_size > 0)
        {
            zipGetDiskSizeAvailable((zipFile)zi, &size_available);

            if (size_available == 0)
            {
                err = zipGoToNextDisk((zipFile)zi);
                if (err != ZIP_OK)
                    return err;
            }

            if (size_available < (uint64_t)max_write)
                max_write = (uint32_t)size_available;
        }

        written = ZWRITE64(zi->z_filefunc, zi->filestream, zi->ci.buffered_data + total_written, max_write);
        if (written != max_write)
        {
            err = ZIP_ERRNO;
            break;
        }

        total_written += written;
        write -= written;
    }
    while (write > 0);

    zi->ci.total_compressed += zi->ci.pos_in_buffered_data;

#ifdef HAVE_BZIP2
    if (zi->ci.compression_method == Z_BZIP2ED)
    {
        zi->ci.total_uncompressed += zi->ci.bstream.total_in_lo32;
        zi->ci.bstream.total_in_lo32 = 0;
        zi->ci.bstream.total_in_hi32 = 0;
    }
    else
#endif
    {
        zi->ci.total_uncompressed += zi->ci.stream.total_in;
        zi->ci.stream.total_in = 0;
    }

    zi->ci.pos_in_buffered_data = 0;

    return err;
}

extern int ZEXPORT zipWriteInFileInZip(zipFile file, const void *buf, uint32_t len)
{
    zip64_internal *zi = NULL;
    int err = ZIP_OK;

    if (file == NULL)
        return ZIP_PARAMERROR;
    zi = (zip64_internal*)file;

    if (zi->in_opened_file_inzip == 0)
        return ZIP_PARAMERROR;

    zi->ci.crc32 = (uint32_t)crc32(zi->ci.crc32, buf, len);

#ifdef HAVE_BZIP2
    if ((zi->ci.compression_method == Z_BZIP2ED) && (!zi->ci.raw))
    {
        zi->ci.bstream.next_in = (void*)buf;
        zi->ci.bstream.avail_in = len;
        err = BZ_RUN_OK;

        while ((err == BZ_RUN_OK) && (zi->ci.bstream.avail_in > 0))
        {
            if (zi->ci.bstream.avail_out == 0)
            {
                err = zipFlushWriteBuffer(zi);
                
                zi->ci.bstream.avail_out = (uint16_t)Z_BUFSIZE;
                zi->ci.bstream.next_out = (char*)zi->ci.buffered_data;
            }
            else
            {
                uint32_t total_out_before_lo = zi->ci.bstream.total_out_lo32;
                uint32_t total_out_before_hi = zi->ci.bstream.total_out_hi32;

                err = BZ2_bzCompress(&zi->ci.bstream, BZ_RUN);

                zi->ci.pos_in_buffered_data += (uint16_t)(zi->ci.bstream.total_out_lo32 - total_out_before_lo);
            }
        }

        if (err == BZ_RUN_OK)
            err = ZIP_OK;
    }
    else
#endif
    {
        zi->ci.stream.next_in = (uint8_t*)buf;
        zi->ci.stream.avail_in = len;

        while ((err == ZIP_OK) && (zi->ci.stream.avail_in > 0))
        {
            if (zi->ci.stream.avail_out == 0)
            {
                err = zipFlushWriteBuffer(zi);
                
                zi->ci.stream.avail_out = Z_BUFSIZE;
                zi->ci.stream.next_out = zi->ci.buffered_data;
            }

            if (err != ZIP_OK)
                break;

            if ((zi->ci.compression_method == Z_DEFLATED) && (!zi->ci.raw))
            {
#ifdef HAVE_APPLE_COMPRESSION
                uint32_t total_out_before = (uint32_t)zi->ci.stream.total_out;

                zi->ci.astream.src_ptr = zi->ci.stream.next_in;
                zi->ci.astream.src_size = zi->ci.stream.avail_in;
                zi->ci.astream.dst_ptr = zi->ci.stream.next_out;
                zi->ci.astream.dst_size = zi->ci.stream.avail_out;

                compression_status status = 0;
                compression_stream_flags flags = 0;

                status = compression_stream_process(&zi->ci.astream, flags);

                uint32_t total_out_after = len - zi->ci.astream.src_size;

                zi->ci.stream.next_in = zi->ci.astream.src_ptr;
                zi->ci.stream.avail_in = zi->ci.astream.src_size;
                zi->ci.stream.next_out = zi->ci.astream.dst_ptr;
                zi->ci.stream.avail_out = zi->ci.astream.dst_size;
                zi->ci.stream.total_in += total_out_after;
                //zi->ci.stream.total_out += copy_this;
                zi->ci.pos_in_buffered_data += total_out_after;

                if (status == COMPRESSION_STATUS_ERROR)
                    err = ZIP_INTERNALERROR;
#else
                uint32_t total_out_before = (uint32_t)zi->ci.stream.total_out;
                err = deflate(&zi->ci.stream, Z_NO_FLUSH);
                zi->ci.pos_in_buffered_data += (uint32_t)(zi->ci.stream.total_out - total_out_before);
#endif
            }
            else
            {
                uint32_t copy_this = 0;
                uint32_t i = 0;
                if (zi->ci.stream.avail_in < zi->ci.stream.avail_out)
                    copy_this = zi->ci.stream.avail_in;
                else
                    copy_this = zi->ci.stream.avail_out;

                for (i = 0; i < copy_this; i++)
                    *(((char*)zi->ci.stream.next_out)+i) =
                        *(((const char*)zi->ci.stream.next_in)+i);

                zi->ci.stream.avail_in -= copy_this;
                zi->ci.stream.avail_out -= copy_this;
                zi->ci.stream.next_in += copy_this;
                zi->ci.stream.next_out += copy_this;
                zi->ci.stream.total_in += copy_this;
                zi->ci.stream.total_out += copy_this;
                zi->ci.pos_in_buffered_data += copy_this;
            }
        }
    }

    return err;
}

extern int ZEXPORT zipCloseFileInZipRaw64(zipFile file, uint64_t uncompressed_size, uint32_t crc32)
{
    zip64_internal *zi = NULL;
    uint16_t extra_data_size = 0;
    uint32_t i = 0;
    unsigned char *extra_info = NULL;
    int err = ZIP_OK;

    if (file == NULL)
        return ZIP_PARAMERROR;
    zi = (zip64_internal*)file;

    if (zi->in_opened_file_inzip == 0)
        return ZIP_PARAMERROR;
    zi->ci.stream.avail_in = 0;

    if (!zi->ci.raw)
    {
        if (zi->ci.compression_method == Z_DEFLATED)
        {
            while (err == ZIP_OK)
            {
                uint32_t total_out_before = 0;
                
                if (zi->ci.stream.avail_out == 0)
                {
                    err = zipFlushWriteBuffer(zi);

                    zi->ci.stream.avail_out = Z_BUFSIZE;
                    zi->ci.stream.next_out = zi->ci.buffered_data;
                }
                
                if (err != ZIP_OK)
                    break;
                
#ifdef HAVE_APPLE_COMPRESSION
                total_out_before = zi->ci.stream.total_out;

                zi->ci.astream.src_ptr = zi->ci.stream.next_in;
                zi->ci.astream.src_size = zi->ci.stream.avail_in;
                zi->ci.astream.dst_ptr = zi->ci.stream.next_out;
                zi->ci.astream.dst_size = zi->ci.stream.avail_out;

                compression_status status = 0;
                status = compression_stream_process(&zi->ci.astream, COMPRESSION_STREAM_FINALIZE);

                uint32_t total_out_after = Z_BUFSIZE - zi->ci.astream.dst_size;

                zi->ci.stream.next_in = zi->ci.astream.src_ptr;
                zi->ci.stream.avail_in = zi->ci.astream.src_size;
                zi->ci.stream.next_out = zi->ci.astream.dst_ptr;
                zi->ci.stream.avail_out = zi->ci.astream.dst_size;
                //zi->ci.stream.total_in += total_out_after;
                //zi->ci.stream.total_out += copy_this;
                zi->ci.pos_in_buffered_data += total_out_after;

                if (status == COMPRESSION_STATUS_ERROR)
                {
                    err = ZIP_INTERNALERROR;
                }
                else if (status == COMPRESSION_STATUS_END)
                {
                    err = Z_STREAM_END;
                }
#else
                total_out_before = (uint32_t)zi->ci.stream.total_out;
                err = deflate(&zi->ci.stream, Z_FINISH);
                zi->ci.pos_in_buffered_data += (uint16_t)(zi->ci.stream.total_out - total_out_before);
#endif
            }
        }
        else if (zi->ci.compression_method == Z_BZIP2ED)
        {
#ifdef HAVE_BZIP2
            err = BZ_FINISH_OK;
            while (err == BZ_FINISH_OK)
            {
                uint32_t total_out_before = 0;
                
                if (zi->ci.bstream.avail_out == 0)
                {
                    err = zipFlushWriteBuffer(zi);
                    
                    zi->ci.bstream.avail_out = (uint16_t)Z_BUFSIZE;
                    zi->ci.bstream.next_out = (char*)zi->ci.buffered_data;
                }
                
                total_out_before = zi->ci.bstream.total_out_lo32;
                err = BZ2_bzCompress(&zi->ci.bstream, BZ_FINISH);
                if (err == BZ_STREAM_END)
                    err = Z_STREAM_END;
                zi->ci.pos_in_buffered_data += (uint16_t)(zi->ci.bstream.total_out_lo32 - total_out_before);
            }

            if (err == BZ_FINISH_OK)
                err = ZIP_OK;
#endif
        }
    }

    if (err == Z_STREAM_END)
        err = ZIP_OK; /* this is normal */

    if ((zi->ci.pos_in_buffered_data > 0) && (err == ZIP_OK))
    {
        err = zipFlushWriteBuffer(zi);
    }

#ifdef HAVE_AES
    if (zi->ci.method == AES_METHOD)
    {
        unsigned char authcode[AES_AUTHCODESIZE];

        fcrypt_end(authcode, &zi->ci.aes_ctx);

        if (ZWRITE64(zi->z_filefunc, zi->filestream, authcode, AES_AUTHCODESIZE) != AES_AUTHCODESIZE)
            err = ZIP_ERRNO;
    }
#endif

    if (!zi->ci.raw)
    {
        if (zi->ci.compression_method == Z_DEFLATED)
        {
            int tmp_err = 0;
#ifdef HAVE_APPLE_COMPRESSION
            tmp_err = compression_stream_destroy(&zi->ci.astream);
#else
            tmp_err = deflateEnd(&zi->ci.stream);
#endif
            if (err == ZIP_OK)
                err = tmp_err;
            zi->ci.stream_initialised = 0;
        }
#ifdef HAVE_BZIP2
        else if (zi->ci.compression_method == Z_BZIP2ED)
        {
            int tmperr = BZ2_bzCompressEnd(&zi->ci.bstream);
            if (err == ZIP_OK)
                err = tmperr;
            zi->ci.stream_initialised = 0;
        }
#endif

        crc32 = zi->ci.crc32;
        uncompressed_size = zi->ci.total_uncompressed;
    }

    /* Write data descriptor */
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)DATADESCRIPTORMAGIC, 4);
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, crc32, 4);
    if (err == ZIP_OK)
    {
        if (zi->ci.zip64)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->ci.total_compressed, 8);
        else
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)zi->ci.total_compressed, 4);
    }
    if (err == ZIP_OK)
    {
        if (zi->ci.zip64)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, uncompressed_size, 8);
        else
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)uncompressed_size, 4);
    }

    /* Update crc and sizes to central directory */
    zipWriteValueToMemory(zi->ci.central_header + 16, crc32, 4); /* crc */
    if (zi->ci.total_compressed >= UINT32_MAX)
        zipWriteValueToMemory(zi->ci.central_header + 20, UINT32_MAX, 4); /* compr size */
    else
        zipWriteValueToMemory(zi->ci.central_header + 20, zi->ci.total_compressed, 4); /* compr size */
    if (uncompressed_size >= UINT32_MAX)
        zipWriteValueToMemory(zi->ci.central_header + 24, UINT32_MAX, 4); /* uncompr size */
    else
        zipWriteValueToMemory(zi->ci.central_header + 24, uncompressed_size, 4); /* uncompr size */
    if (zi->ci.stream.data_type == Z_ASCII)
        zipWriteValueToMemory(zi->ci.central_header + 36, (uint16_t)Z_ASCII, 2); /* internal file attrib */

    /* Add ZIP64 extra info field for uncompressed size */
    if (uncompressed_size >= UINT32_MAX)
        extra_data_size += 8;
    /* Add ZIP64 extra info field for compressed size */
    if (zi->ci.total_compressed >= UINT32_MAX)
        extra_data_size += 8;
    /* Add ZIP64 extra info field for relative offset to local file header of current file */
    if (zi->ci.pos_local_header >= UINT32_MAX)
        extra_data_size += 8;

    /* Add ZIP64 extra info header to central directory */
    if (extra_data_size > 0)
    {
        if ((uint32_t)(extra_data_size + 4) > zi->ci.size_centralextrafree)
            return ZIP_BADZIPFILE;

        extra_info = (unsigned char*)zi->ci.central_header + zi->ci.size_centralheader;

        zipWriteValueToMemoryAndMove(&extra_info, 0x0001, 2);
        zipWriteValueToMemoryAndMove(&extra_info, extra_data_size, 2);

        if (uncompressed_size >= UINT32_MAX)
            zipWriteValueToMemoryAndMove(&extra_info, uncompressed_size, 8);
        if (zi->ci.total_compressed >= UINT32_MAX)
            zipWriteValueToMemoryAndMove(&extra_info, zi->ci.total_compressed, 8);
        if (zi->ci.pos_local_header >= UINT32_MAX)
            zipWriteValueToMemoryAndMove(&extra_info, zi->ci.pos_local_header, 8);

        zi->ci.size_centralextrafree -= extra_data_size + 4;
        zi->ci.size_centralheader += extra_data_size + 4;
        zi->ci.size_centralextra += extra_data_size + 4;

        zipWriteValueToMemory(zi->ci.central_header + 30, zi->ci.size_centralextra, 2);
    }

#ifdef HAVE_AES
    /* Write AES extra info header to central directory */
    if (zi->ci.method == AES_METHOD)
    {
        extra_info = (unsigned char*)zi->ci.central_header + zi->ci.size_centralheader;
        extra_data_size = 7;

        if ((uint32_t)(extra_data_size + 4) > zi->ci.size_centralextrafree)
            return ZIP_BADZIPFILE;

        zipWriteValueToMemoryAndMove(&extra_info, 0x9901, 2);
        zipWriteValueToMemoryAndMove(&extra_info, extra_data_size, 2);
        zipWriteValueToMemoryAndMove(&extra_info, AES_VERSION, 2);
        zipWriteValueToMemoryAndMove(&extra_info, 'A', 1);
        zipWriteValueToMemoryAndMove(&extra_info, 'E', 1);
        zipWriteValueToMemoryAndMove(&extra_info, AES_ENCRYPTIONMODE, 1);
        zipWriteValueToMemoryAndMove(&extra_info, zi->ci.compression_method, 2);

        zi->ci.size_centralextrafree -= extra_data_size + 4;
        zi->ci.size_centralheader += extra_data_size + 4;
        zi->ci.size_centralextra += extra_data_size + 4;

        zipWriteValueToMemory(zi->ci.central_header + 30, zi->ci.size_centralextra, 2);
    }
#endif
    /* Restore comment to correct position */
    for (i = 0; i < zi->ci.size_comment; i++)
        zi->ci.central_header[zi->ci.size_centralheader+i] =
            zi->ci.central_header[zi->ci.size_centralheader+zi->ci.size_centralextrafree+i];
    zi->ci.size_centralheader += zi->ci.size_comment;

    if (err == ZIP_OK)
        err = add_data_in_datablock(&zi->central_dir, zi->ci.central_header, zi->ci.size_centralheader);

    free(zi->ci.central_header);

    zi->number_entry++;
    zi->in_opened_file_inzip = 0;

    return err;
}

extern int ZEXPORT zipCloseFileInZipRaw(zipFile file, uint32_t uncompressed_size, uint32_t crc32)
{
    return zipCloseFileInZipRaw64(file, uncompressed_size, crc32);
}

extern int ZEXPORT zipCloseFileInZip(zipFile file)
{
    return zipCloseFileInZipRaw(file, 0, 0);
}

extern int ZEXPORT zipClose(zipFile file, const char *global_comment)
{
    return zipClose_64(file, global_comment);
}

extern int ZEXPORT zipClose_64(zipFile file, const char *global_comment)
{
    return zipClose2_64(file, global_comment, VERSIONMADEBY);
}

extern int ZEXPORT zipClose2_64(zipFile file, const char *global_comment, uint16_t version_madeby)
{
    zip64_internal *zi = NULL;
    uint32_t size_centraldir = 0;
    uint16_t size_global_comment = 0;
    uint64_t centraldir_pos_inzip = 0;
    uint64_t pos = 0;
    uint64_t cd_pos = 0;
    uint32_t write = 0;
    int err = ZIP_OK;

    if (file == NULL)
        return ZIP_PARAMERROR;
    zi = (zip64_internal*)file;

    if (zi->in_opened_file_inzip == 1)
        err = zipCloseFileInZip(file);

#ifndef NO_ADDFILEINEXISTINGZIP
    if (global_comment == NULL)
        global_comment = zi->globalcomment;
#endif

    if (zi->filestream != zi->filestream_with_CD)
    {
        if (ZCLOSE64(zi->z_filefunc, zi->filestream) != 0)
            if (err == ZIP_OK)
                err = ZIP_ERRNO;
        if (zi->disk_size > 0)
            zi->number_disk_with_CD = zi->number_disk + 1;
        zi->filestream = zi->filestream_with_CD;
    }

    centraldir_pos_inzip = ZTELL64(zi->z_filefunc, zi->filestream);

    if (err == ZIP_OK)
    {
        linkedlist_datablock_internal *ldi = zi->central_dir.first_block;
        while (ldi != NULL)
        {
            if ((err == ZIP_OK) && (ldi->filled_in_this_block > 0))
            {
                write = ZWRITE64(zi->z_filefunc, zi->filestream, ldi->data, ldi->filled_in_this_block);
                if (write != ldi->filled_in_this_block)
                    err = ZIP_ERRNO;
            }

            size_centraldir += ldi->filled_in_this_block;
            ldi = ldi->next_datablock;
        }
    }

    free_linkedlist(&(zi->central_dir));

    pos = centraldir_pos_inzip - zi->add_position_when_writting_offset;

    /* Write the ZIP64 central directory header */
    if (pos >= UINT32_MAX || zi->number_entry > UINT16_MAX)
    {
        uint64_t zip64_eocd_pos_inzip = ZTELL64(zi->z_filefunc, zi->filestream);
        uint32_t zip64_datasize = 44;

        err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)ZIP64ENDHEADERMAGIC, 4);

        /* Size of this 'zip64 end of central directory' */
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint64_t)zip64_datasize, 8);
        /* Version made by */
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, version_madeby, 2);
        /* version needed */
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint16_t)45, 2);
        /* Number of this disk */
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->number_disk_with_CD, 4);
        /* Number of the disk with the start of the central directory */
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->number_disk_with_CD, 4);
        /* Total number of entries in the central dir on this disk */
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->number_entry, 8);
        /* Total number of entries in the central dir */
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->number_entry, 8);
        /* Size of the central directory */
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint64_t)size_centraldir, 8);

        if (err == ZIP_OK)
        {
            /* Offset of start of central directory with respect to the starting disk number */
            cd_pos = centraldir_pos_inzip - zi->add_position_when_writting_offset;
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, cd_pos, 8);
        }
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)ZIP64ENDLOCHEADERMAGIC, 4);

        /* Number of the disk with the start of the central directory */
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->number_disk_with_CD, 4);
        /* Relative offset to the Zip64EndOfCentralDirectory */
        if (err == ZIP_OK)
        {
            cd_pos = zip64_eocd_pos_inzip - zi->add_position_when_writting_offset;
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, cd_pos, 8);
        }
        /* Number of the disk with the start of the central directory */
        if (err == ZIP_OK)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, zi->number_disk_with_CD + 1, 4);
    }

    /* Write the central directory header */

    /* Signature */
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)ENDHEADERMAGIC, 4);
    /* Number of this disk */
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint16_t)zi->number_disk_with_CD, 2);
    /* Number of the disk with the start of the central directory */
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint16_t)zi->number_disk_with_CD, 2);
    /* Total number of entries in the central dir on this disk */
    if (err == ZIP_OK)
    {
        if (zi->number_entry >= UINT16_MAX)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, UINT16_MAX, 2); /* use value in ZIP64 record */
        else
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint16_t)zi->number_entry, 2);
    }
    /* Total number of entries in the central dir */
    if (err == ZIP_OK)
    {
        if (zi->number_entry >= UINT16_MAX)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, UINT16_MAX, 2); /* use value in ZIP64 record */
        else
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint16_t)zi->number_entry, 2);
    }
    /* Size of the central directory */
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, size_centraldir, 4);
    /* Offset of start of central directory with respect to the starting disk number */
    if (err == ZIP_OK)
    {
        cd_pos = centraldir_pos_inzip - zi->add_position_when_writting_offset;
        if (pos >= UINT32_MAX)
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, UINT32_MAX, 4);
        else
            err = zipWriteValue(&zi->z_filefunc, zi->filestream, (uint32_t)cd_pos, 4);
    }

    /* Write global comment */

    if (global_comment != NULL)
        size_global_comment = (uint16_t)strlen(global_comment);
    if (err == ZIP_OK)
        err = zipWriteValue(&zi->z_filefunc, zi->filestream, size_global_comment, 2);
    if (err == ZIP_OK && size_global_comment > 0)
    {
        if (ZWRITE64(zi->z_filefunc, zi->filestream, global_comment, size_global_comment) != size_global_comment)
            err = ZIP_ERRNO;
    }

    if ((ZCLOSE64(zi->z_filefunc, zi->filestream) != 0) && (err == ZIP_OK))
        err = ZIP_ERRNO;

#ifndef NO_ADDFILEINEXISTINGZIP
    TRYFREE(zi->globalcomment);
#endif
    TRYFREE(zi);

    return err;
}
