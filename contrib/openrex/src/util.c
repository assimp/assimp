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
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(WIN32) || defined(WIN64) || defined(_WINDOWS)
// Copied from linux libc sys/stat.h:
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif


#include "util.h"

static void verr (const char *fmt, va_list ap)
{
    vfprintf (stderr, fmt, ap);

    if (fmt[0] && fmt[strlen (fmt) - 1] == ':')
    {
        fputc (' ', stderr);
        perror (NULL);
    }
    else
        fputc ('\n', stderr);
}

void warn (const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    verr (fmt, ap);
    va_end (ap);
}

void die (const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    verr (fmt, ap);
    va_end (ap);

    exit (1);
}

int dir_exists (const char *folder)
{
    struct stat sb;
    if (stat (folder, &sb) == 0 && S_ISDIR (sb.st_mode))
        return 1;
    return 0;
}

char *read_file_ascii (const char *filename)
{
    FILE *f = fopen (filename, "rt");
    if (f == NULL) return NULL;
    fseek (f, 0, SEEK_END);
    long length = ftell (f);
    fseek (f, 0, SEEK_SET);
    char *buffer = (char *) malloc (length + 1);
    buffer[length] = '\0';
    fread (buffer, 1, length, f);
    fclose (f);
    return buffer;
}

uint8_t *read_file_binary (const char *filename, long *sz)
{
    FILE *f = fopen (filename, "rb");
    if (f == NULL) return NULL;
    fseek (f, 0, SEEK_END);
    *sz = ftell (f);
    fseek (f, 0, SEEK_SET);
    uint8_t *buffer = (uint8_t *) malloc (*sz);
    size_t ret = fread (buffer, 1, *sz, f);
    if (ret != *sz)
    {
        FREE (buffer);
        fclose (f);
        return NULL;
    }

    fclose (f);
    return buffer;
}

inline char separator()
{
#ifdef WIN32
    return '\\';
#else
    return '/';
#endif
}

