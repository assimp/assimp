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
 * \brief The general REX block definition and IO functions
 *
 * | **size [bytes]** | **name** | **type** | **description**                  |
 * |------------------|----------|----------|----------------------------------|
 * | 2                | type     | uint16_t | data type                        |
 * | 2                | version  | uint16_t | version for this data block      |
 * | 4                | size     | uint32_t | data block size (without header) |
 * | 8                | dataId   | uint64_t | id which is used in the database |
 *
 * The currently supported data block types are as follows. Please make sure that the IDs are not reordered.
 * Total size of the header is **16 bytes**.
 *
 * Please note that some of the data types offer a LOD (level-of-detail) information. This value
 * can be interpreted as 0 being the highest level. As data type we use 32bit for better memory alignment.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * List of currently supported REX data blocks
 */
enum rex_block_type
{
    LineSet          = 0,
    Text             = 1,
    PointList        = 2,
    Mesh             = 3,
    Image            = 4,
    MaterialStandard = 5,
    PeopleSimulation = 6,
    UnityPackage     = 7
};

/**
 * Structure which stores the REX block. The block has the actual
 * playload stored as a void* in data.
 */
struct rex_block
{
    uint16_t type;    //<! identifies the block and therefore can be used to map *data
    uint16_t version; //<! block version
    uint32_t sz;      //<! data block size w/o header
    uint64_t id;      //<! a unique identifier for this block
    void     *data;   //<! stores the actual data
};

/*
 * Read the complete data block from the given data block pointer.
 * After successful read, the new pointer location is return, else NULL.
 *
 * Memory is allocated for the data block. The caller must make sure that this
 * memory is then deallocated.
 *
 * \param ptr the pointer which points to the beginning of a block
 * \param the actual REX block which contains the block payload data
 * \return the pointer to the end of this block
 */
uint8_t *rex_block_read (uint8_t *ptr, struct rex_block *block);

/**
 * Writes the block header to the given pointer and returns the pointer to the
 * data after the block. The ptr must point to allocated memory. The data pointer
 * will not be written!
 *
 * \param ptr pointing to a pre-allocated memory block.
 * \param block the block which contains the header information
 * \return the pointer to the end of the serialized data blob
 */
uint8_t *rex_block_header_write (uint8_t *ptr, struct rex_block *block);


#ifdef __cplusplus
}
#endif
