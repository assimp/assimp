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
 * \brief REX image block storing any image or texture data
 *
 * The Image data block can either contain an arbitrary image or a texture for a given 3D mesh. If a texture is stored,
 * the 3D mesh will refer to it by the `dataId`.  The data block size in the header refers to the total size of
 * this block (compression + data_size).
 *
 * | **size [bytes]** | **name**    | **type** | **description**                        |
 * |------------------|-------------|----------|----------------------------------------|
 * | 4                | compression | uint32_t | id for supported compression algorithm |
 * |                  | data        | bytes    | data of the file content               |
 *
 */

#include <stdint.h>
#include "rex-header.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is a list of supported image compressions. The compression can be used
 * to get the correct encoding format of the image.
 */
enum rex_image_compression
{
    Raw24 = 0,
    Jpeg = 1,
    Png = 2
};

/**
 * Stores all the properties for a REX image
 */
struct rex_image
{
    uint32_t compression; //!< stores the rex_image_compression
    uint8_t *data;        //!< the binary data of the image
    uint64_t sz;          //!< the size of the image data stored in data
};

/**
 * Reads an image block from the given pointer. This call will allocate memory
 * for the image. The caller is responsible to free this memory! The sz parameter
 * is required for the number of bytes to read.
 *
 * \param ptr pointer to the block start
 * \param img the rex_image structure which gets filled
 * \return the pointer to the memory block after the rex_image block
 */
uint8_t *rex_block_read_image (uint8_t *ptr, struct rex_image *img);

/**
 * Writes an image block to a binary stream. Memory will be allocated and the caller
 * must take care of releasing the memory.
 *
 * \param id the data block ID
 * \param header the REX header which gets modified according the the new block, can be NULL
 * \param img the image which should get serialized
 * \param sz the total size of the of the data block which is returned
 * \return a pointer to the data block
 */
uint8_t *rex_block_write_image (uint64_t id, struct rex_header *header, struct rex_image *img, long *sz);

#ifdef __cplusplus
}
#endif

