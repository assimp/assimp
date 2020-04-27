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
 * \brief REX lineset block storing a list of points which are getting connected to lines
 *
 * | **size [bytes]** | **name**     | **type** | **description**               |
 * |------------------|--------------|----------|-------------------------------|
 * | 4                | red          | float    | red channel                   |
 * | 4                | green        | float    | green channel                 |
 * | 4                | blue         | float    | blue channel                  |
 * | 4                | alpha        | float    | alpha channel                 |
 * | 4                | nrOfVertices | uint32_t | number of vertices            |
 * | 4                | x0           | float    | x-coordinate of first vertex  |
 * | 4                | y0           | float    | y-coordinate of first vertex  |
 * | 4                | z0           | float    | z-coordinate of first vertex  |
 * | 4                | x1           | float    | x-coordinate of second vertex |
 * | ...              |              |          |                               |
 */

#include <stdint.h>
#include "rex-header.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stores all the properties for a REX lineset
 */
struct rex_lineset
{
    float red;              //!< the red color value between 0..1
    float green;            //!< the green color value between 0..1
    float blue;             //!< the blue color value between 0..1
    float alpha;            //!< the alpha value between 0..1
    uint32_t nr_vertices;   //!< the number of vertices stored in vertices
    float *vertices;        //!< the raw data of all vertices (x0y0z0x1y1...)
};

/**
 * Reads a lineset block from the given pointer. This call will allocate memory
 * for the vertices. The caller is responsible to free this memory!
 *
 * \param ptr pointer to the block start
 * \param lineset the rex_lineset structure which gets filled
 * \return the pointer to the memory block after the rex_lineset block
 */
uint8_t *rex_block_read_lineset (uint8_t *ptr, struct rex_lineset *lineset);

/**
 * Writes a lineset block to binary. Memory will be allocated and the caller
 * must take care of releasing the memory.
 *
 * \param id the data block ID
 * \param header the REX header which gets modified according the the new block, can be NULL
 * \param lineset the lineset which should get serialized
 * \param sz the total size of the of the data block which is returned
 * \return a pointer to the data block
 */
uint8_t *rex_block_write_lineset (uint64_t id, struct rex_header *header, struct rex_lineset *lineset, long *sz);

#ifdef __cplusplus
}
#endif
