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
 * \brief REX text block storing a text element
 *
 * | **size [bytes]** | **name**  | **type** | **description**                   |
 * |------------------|-----------|----------|-----------------------------------|
 * | 4                | red       | float    | red channel                       |
 * | 4                | green     | float    | green channel                     |
 * | 4                | blue      | float    | blue channel                      |
 * | 4                | alpha     | float    | alpha channel                     |
 * | 4                | positionX | float    | x-coordinate of the position      |
 * | 4                | positionY | float    | y-coordinate of the position      |
 * | 4                | positionZ | float    | z-coordinate of the position      |
 * | 4                | fontSize  | float    | font size in font units (e.g. 24) |
 * | 2+sz             | text      | string   | text for the label                |
 */

#include <stdint.h>
#include "linmath.h"
#include "rex-header.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The structure which stores the REX text information
 */
struct rex_text
{
    float red;        //!< the red color value between 0..1
    float green;      //!< the green color value between 0..1
    float blue;       //!< the blue color value between 0..1
    float alpha;      //!< the alpha value between 0..1
    vec3 position;    //!< the position of the text in space (unit meters)
    float font_size;  //!< the font size in font units (e.g. 24)
    char *data;       //!< null terminated string
};

/**
 * Reads a text block from the given pointer. This call will allocate memory
 * for the text. The caller is responsible to free this memory!
 *
 * \param ptr pointer to the block start
 * \param text the rex_text structure which gets filled
 * \return the pointer to the memory block after the rex_text block
 */
uint8_t *rex_block_read_text (uint8_t *ptr, struct rex_text *text);

/**
 * Writes a text block to binary. Memory will be allocated and the caller
 * must take care of releasing the memory.
 *
 * \param id the data block ID
 * \param header the REX header which gets modified according the the new block, can be NULL
 * \param text the text which should get serialized
 * \param sz the total size of the of the data block which is returned
 * \return a pointer to the data block
 */
uint8_t *rex_block_write_text (uint64_t id, struct rex_header *header, struct rex_text *text, long *sz);

#ifdef __cplusplus
}
#endif
