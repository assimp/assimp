/* zip.h -- IO on .zip files using zlib
   Version 1.2.0, September 16th, 2017
   part of the MiniZip project

   Copyright (C) 2012-2017 Nathan Moinvaziri
     https://github.com/nmoinvaz/minizip
   Copyright (C) 2009-2010 Mathias Svensson
     Modifications for Zip64 support
     http://result42.com
   Copyright (C) 1998-2010 Gilles Vollant
     http://www.winimage.com/zLibDll/minizip.html

   This program is distributed under the terms of the same license as zlib.
   See the accompanying LICENSE file for the full text of the license.
*/

#ifndef _ZIP_H
#define _ZIP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ZLIB_H
#  include "zlib.h"
#endif

#ifndef _ZLIBIOAPI_H
#  include "ioapi.h"
#endif

#ifdef HAVE_BZIP2
#  include "bzlib.h"
#endif

#define Z_BZIP2ED 12

#if defined(STRICTZIP) || defined(STRICTZIPUNZIP)
/* like the STRICT of WIN32, we define a pointer that cannot be converted
    from (void*) without cast */
typedef struct TagzipFile__ { int unused; } zip_file__;
typedef zip_file__ *zipFile;
#else
typedef voidp zipFile;
#endif

#define ZIP_OK                          (0)
#define ZIP_EOF                         (0)
#define ZIP_ERRNO                       (Z_ERRNO)
#define ZIP_PARAMERROR                  (-102)
#define ZIP_BADZIPFILE                  (-103)
#define ZIP_INTERNALERROR               (-104)

#ifndef DEF_MEM_LEVEL
#  if MAX_MEM_LEVEL >= 8
#    define DEF_MEM_LEVEL 8
#  else
#    define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#  endif
#endif

typedef struct
{
    uint32_t    dos_date;
    uint16_t    internal_fa;        /* internal file attributes        2 bytes */
    uint32_t    external_fa;        /* external file attributes        4 bytes */
} zip_fileinfo;

#define APPEND_STATUS_CREATE        (0)
#define APPEND_STATUS_CREATEAFTER   (1)
#define APPEND_STATUS_ADDINZIP      (2)

/***************************************************************************/
/* Writing a zip file */

extern zipFile ZEXPORT zipOpen(const char *path, int append);
extern zipFile ZEXPORT zipOpen64(const void *path, int append);
/* Create a zipfile.

   path should contain the full path (by example, on a Windows XP computer 
      "c:\\zlib\\zlib113.zip" or on an Unix computer "zlib/zlib113.zip". 

   return NULL if zipfile cannot be opened
   return zipFile handle if no error

   If the file path exist and append == APPEND_STATUS_CREATEAFTER, the zip
   will be created at the end of the file. (useful if the file contain a self extractor code)
   If the file path exist and append == APPEND_STATUS_ADDINZIP, we will add files in existing 
   zip (be sure you don't add file that doesn't exist)

   NOTE: There is no delete function into a zipfile. If you want delete file into a zipfile, 
   you must open a zipfile, and create another. Of course, you can use RAW reading and writing to copy
   the file you did not want delete. */

extern zipFile ZEXPORT zipOpen2(const char *path, int append, const char **globalcomment, 
    zlib_filefunc_def *pzlib_filefunc_def);

extern zipFile ZEXPORT zipOpen2_64(const void *path, int append, const char **globalcomment, 
    zlib_filefunc64_def *pzlib_filefunc_def);

extern zipFile ZEXPORT zipOpen3(const char *path, int append, uint64_t disk_size, 
    const char **globalcomment, zlib_filefunc_def *pzlib_filefunc_def);
/* Same as zipOpen2 but allows specification of spanned zip size */

extern zipFile ZEXPORT zipOpen3_64(const void *path, int append, uint64_t disk_size, 
    const char **globalcomment, zlib_filefunc64_def *pzlib_filefunc_def);

extern int ZEXPORT zipOpenNewFileInZip(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global, 
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level);
/* Open a file in the ZIP for writing.

   filename : the filename in zip (if NULL, '-' without quote will be used
   *zipfi contain supplemental information
   extrafield_local buffer to store the local header extra field data, can be NULL
   size_extrafield_local size of extrafield_local buffer
   extrafield_global buffer to store the global header extra field data, can be NULL
   size_extrafield_global size of extrafield_local buffer
   comment buffer for comment string
   method contain the compression method (0 for store, Z_DEFLATED for deflate)
   level contain the level of compression (can be Z_DEFAULT_COMPRESSION)
   zip64 is set to 1 if a zip64 extended information block should be added to the local file header.
   this MUST be '1' if the uncompressed size is >= 0xffffffff. */

extern int ZEXPORT zipOpenNewFileInZip64(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int zip64);
/* Same as zipOpenNewFileInZip with zip64 support */

extern int ZEXPORT zipOpenNewFileInZip2(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw);
/* Same as zipOpenNewFileInZip, except if raw=1, we write raw file */

extern int ZEXPORT zipOpenNewFileInZip2_64(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw, int zip64);
/* Same as zipOpenNewFileInZip3 with zip64 support */

extern int ZEXPORT zipOpenNewFileInZip3(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw, int windowBits, int memLevel,
    int strategy, const char *password, ZIP_UNUSED uint32_t crc_for_crypting);
/* Same as zipOpenNewFileInZip2, except
    windowBits, memLevel, strategy : see parameter strategy in deflateInit2
    password : crypting password (NULL for no crypting)
    crc_for_crypting : crc of file to compress (needed for crypting) */

extern int ZEXPORT zipOpenNewFileInZip3_64(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw, int windowBits, int memLevel,
    int strategy, const char *password, ZIP_UNUSED uint32_t crc_for_crypting, int zip64);
/* Same as zipOpenNewFileInZip3 with zip64 support */

extern int ZEXPORT zipOpenNewFileInZip4(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw, int windowBits, int memLevel,
    int strategy, const char *password, ZIP_UNUSED uint32_t crc_for_crypting, uint16_t version_madeby, uint16_t flag_base);
/* Same as zipOpenNewFileInZip3 except versionMadeBy & flag fields */

extern int ZEXPORT zipOpenNewFileInZip4_64(zipFile file, const char *filename, const zip_fileinfo *zipfi,
    const void *extrafield_local, uint16_t size_extrafield_local, const void *extrafield_global,
    uint16_t size_extrafield_global, const char *comment, uint16_t method, int level, int raw, int windowBits, int memLevel,
    int strategy, const char *password, ZIP_UNUSED uint32_t crc_for_crypting, uint16_t version_madeby, uint16_t flag_base, int zip64);
/* Same as zipOpenNewFileInZip4 with zip64 support */

extern int ZEXPORT zipOpenNewFileInZip5(zipFile file,
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
                                        int aes);
/* Allowing optional aes */

extern int ZEXPORT zipWriteInFileInZip(zipFile file, const void *buf, uint32_t len);
/* Write data in the zipfile */

extern int ZEXPORT zipCloseFileInZip(zipFile file);
/* Close the current file in the zipfile */

extern int ZEXPORT zipCloseFileInZipRaw(zipFile file, uint32_t uncompressed_size, uint32_t crc32);
extern int ZEXPORT zipCloseFileInZipRaw64(zipFile file, uint64_t uncompressed_size, uint32_t crc32);
/* Close the current file in the zipfile, for file opened with parameter raw=1 in zipOpenNewFileInZip2
   where raw is compressed data. Parameters uncompressed_size and crc32 are value for the uncompressed data. */

extern int ZEXPORT zipClose(zipFile file, const char *global_comment);
/* Close the zipfile */

extern int ZEXPORT zipClose_64(zipFile file, const char *global_comment);

extern int ZEXPORT zipClose2_64(zipFile file, const char *global_comment, uint16_t version_madeby);
/* Same as zipClose_64 except version_madeby field */

/***************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _ZIP_H */
