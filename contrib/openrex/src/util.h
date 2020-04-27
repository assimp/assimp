/*
 * Copyright 2018 Robotic Eyes GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.*
 */
#pragma once

/**
 * \file
 * \brief Various utility functions
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LEN(x) (sizeof (x) / sizeof *(x))

/**
 * Dumps a warning to stderr
 */
void warn (const char *, ...);

/**
 * Dumps the error to stderr and exits the program
 * This should only be called if some severe errors occur
 */
void die (const char *, ...);

/**
 * Reads the content of a file as ASCII text. The caller must make sure
 * that the memory gets freed!
 *
 * \param filename the absolute path to the file
 */
char *read_file_ascii (const char *filename);

/**
 * Reads the content of a file as binary blob. The caller must make sure
 * that the memory gets freed! The total size of allocated memory is returned
 * as sz.
 *
 * \param filename the absolute path to the file
 * \param sz the size of the allocated memory
 */
uint8_t *read_file_binary (const char *filename, long *sz);

/**
 * Checks if a directory exists, if so, return != 0.
 * If it does not exist, return 0
 */
int dir_exists (const char *);

#define FP_CHECK(fp) \
if (!fp) \
{ \
    warn ("File pointer is not valid"); \
    return REX_ERROR_FILE_OPEN; \
}

#define MEM_CHECK(buf) \
if (buf == NULL) \
{ \
    warn ("Memory is corrupt"); \
    return NULL; \
}

#define FREE(m) do { free(m); m = NULL; } while(0);

#define rex_write(p,s,n,fp) \
{ \
    size_t ret = fwrite (p, s, n, fp); \
    if (ret != n) \
        return REX_ERROR_FILE_READ; \
}

#define rex_read(p,s,n,fp) \
{ \
    size_t ret = fread (p, s, n, fp); \
    if (ret != n) \
        return REX_ERROR_FILE_WRITE; \
}

/**
 * Copies the src into the destination with the given size sz
 * and advances the src pointer.
 */
#define rexcpy(dest, src, sz) \
{ \
    memcpy(dest, src, sz); \
    src += sz; \
}

/**
 * Copies the src into the destination with the given size sz
 * and advances the dest pointer.
 */
#define rexcpyr(src, dest, sz) \
{ \
    memcpy(dest, src, sz); \
    dest += sz; \
}

/**
 * Path separator which handles the platform
 */
char separator();

#ifdef __cplusplus
}
#endif
