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
 * \brief REX pointlist block stored 3D point clouds
 *
 * This data type can be used to store a set of points (e.g. colored point clouds).
 * The number of vertices  The number of colors can be zero. If this is the case the
 * points are rendered with a pre-defined color (e.g. white). If color is provided
 * the number of color entries must fit the number of vertices (i.e. every point needs
 * to have an RGB color). The number of points which can currently be handled properly
 * is 200k.
 *
 * | **size [bytes]** | **name**     | **type** | **description**                              |
 * |------------------|--------------|----------|----------------------------------------------|
 * | 4                | nrOfVertices | uint32_t | number of vertices                           |
 * | 4                | nrOfColors   | uint32_t | number of colors                             |
 * | 4                | x            | float    | x-coordinate of first vertex                 |
 * | 4                | y            | float    | y-coordinate of first vertex                 |
 * | 4                | z            | float    | z-coordinate of first vertex                 |
 * | 4                | x            | float    | x-coordinate of second vertex                |
 * | ...              |              |          |                                              |
 * | 4                | red          | float    | red component of the first vertex            |
 * | 4                | green        | float    | green component of the first vertex          |
 * | 4                | blue         | float    | blue component of the first vertex           |
 * | 4                | red          | float    | red component of the second vertex           |
 * | ...              |              |          |                                              |
 */

#include <stdint.h>
#include "rex-header.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The REX pointlist structure storing the block data
 */
struct rex_pointlist
{
    uint32_t nr_vertices; //<! the number of vertices
    uint32_t nr_colors;   //<! the number of colors, can either be 0 or match nr_vertices

    float *positions;     //<! the byte array storing the coordinates (xyzxyzxyz...)
    float *colors;        //<! the byte array storing the color information (rgbrgbrgb...)
};

/**
 * Reads a pointlist block from the data pointer. NULL is returned in case of error,
 * else the pointer after the block is returned.
 *
 * \param ptr pointer to the block start
 * \param plist the rex_pointlist structure which gets filled
 * \return the pointer to the memory block after the rex_pointlist block
 */
uint8_t *rex_block_read_pointlist (uint8_t *ptr, struct rex_pointlist *plist);

/**
 * Writes a pointlist block to a binary stream. Memory will be allocated and the caller
 * must take care of releasing the memory.
 *
 * \param id the data block ID
 * \param header the REX header which gets modified according the the new block, can be NULL
 * \param plist the image which should get serialized
 * \param sz the total size of the of the data block which is returned
 * \return a pointer to the data block
 */
uint8_t *rex_block_write_pointlist (uint64_t id, struct rex_header *header, struct rex_pointlist *plist, long *sz);

/**
 * Sets all properties of the rex_lineset structure to initial values
 */
void rex_pointlist_init (struct rex_pointlist *plist);

/**
 * Frees any memory which is allocated for rex_pointlist
 */
void rex_pointlist_free (struct rex_pointlist *plist);

#ifdef __cplusplus
}
#endif
