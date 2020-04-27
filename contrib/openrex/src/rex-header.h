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
 * \brief the REX file header information
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * General structure which stores the REX file header information
 */
struct rex_header
{
    char       magic[4];           //<! identifier for a valid REX file
    uint16_t   version;            //<! REX file version
    uint32_t   crc;                //<! a CRC check number (can be 0)
    uint16_t   nr_datablocks;      //<! number of data blocks contained in this file/stream
    uint16_t   start_addr;         //<! address of the first block in the file/stream
    uint64_t   sz_all_datablocks;  //<! size of all data blocks
    char       reserved[42];       //<! for future fields
};

/**
 * Create an empty valid REX header structure
 */
struct rex_header *rex_header_create ();

/**
 * Reads a REX header from the given pointer location
 *
 * \param buf pointer to the start of the content
 * \param the header which should get filled
 * \return the pointer position to the first data block
 */
uint8_t *rex_header_read (uint8_t *buf, struct rex_header *header);

/**
 * Writes a given REX header to a buffer. Memory is getting allocated
 * and must be freed by the caller.
 *
 * \param header the header which should get written
 * \param sz the size of the memory which got allocated
 * \return the pointer to the allocated memory containing the REX header information
 */
uint8_t *rex_header_write (struct rex_header *header, long *sz);

#ifdef __cplusplus
}
#endif
