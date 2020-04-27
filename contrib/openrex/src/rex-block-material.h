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
 * \brief REX standard material used for rex_mesh
 *
 * The standard material block is used to set the material for the geometry specified in the mesh data block.
 *
 * | **size [bytes]** | **name**     | **type** | **description**                                         |
 * |------------------|--------------|----------|---------------------------------------------------------|
 * | 4                | Ka red       | float    | RED component for ambient color                         |
 * | 4                | Ka green     | float    | GREEN component for ambient color                       |
 * | 4                | Ka blue      | float    | BLUE component for ambient color                        |
 * | 8                | Ka textureId | uint64_t | dataId of the referenced texture for ambient component  |
 * | 4                | Kd red       | float    | RED component for diffuse color                         |
 * | 4                | Kd green     | float    | GREEN component for diffuse color                       |
 * | 4                | Kd blue      | float    | BLUE component for diffuse color                        |
 * | 8                | Kd textureId | uint64_t | dataId of the referenced texture for diffuse component  |
 * | 4                | Ks red       | float    | RED component for specular color                        |
 * | 4                | Ks green     | float    | GREEN component for specular color                      |
 * | 4                | Ks blue      | float    | BLUE component for specular color                       |
 * | 8                | Ks textureId | uint64_t | dataId of the referenced texture for specular component |
 * | 4                | Ns           | float    | specular exponent                                       |
 * | 4                | alpha        | float    | alpha between 0..1, 1 means full opaque                 |
 *
 * If no texture is available/set, then the `textureId` is set to `INT64_MAX` (`Long.MAX_VALUE` in Java) value.
 */

#include <stdint.h>
#include "rex-header.h"

#ifdef __cplusplus
extern "C" {
#endif

struct rex_material_standard
{
    float    ka_red;       //!< RED component for ambient color (0..1)
    float    ka_green;     //!< GREEN component for ambient color (0..1)
    float    ka_blue;      //!< BLUE component for ambient color (0..1)
    uint64_t ka_textureId; //!< dataId of the referenced texture for ambient component
    float    kd_red;       //!< RED component for diffuse color (0..1)
    float    kd_green;     //!< GREEN component for diffuse color (0..1)
    float    kd_blue;      //!< BLUE component for diffuse color (0..1)
    uint64_t kd_textureId; //!< dataId of the referenced texture for diffuse component
    float    ks_red;       //!< RED component for specular color (0..1)
    float    ks_green;     //!< GREEN component for specular color (0..1)
    float    ks_blue;      //!< BLUE component for specular color (0..1)
    uint64_t ks_textureId; //!< dataId of the referenced texture for specular component
    float    ns;           //!< specular exponent
    float    alpha;        //!< alpha between 0..1, 1 means full opaque
};

/**
 * Reads a material block from the data pointer. NULL is returned in case of error,
 * else the pointer after the block is returned.
 *
 * \param ptr pointer to the block start
 * \param mat the rex_material_standard structure which gets filled
 * \return the pointer to the memory block after the rex_material_standard block
 */
uint8_t *rex_block_read_material (uint8_t *ptr, struct rex_material_standard *mat);

/**
 * Writes a material block to a binary stream. Memory will be allocated and the caller
 * must take care of releasing the memory.
 *
 * \param id the data block ID
 * \param header the REX header which gets modified according the the new block, can be NULL
 * \param mat the material which should get serialized
 * \param sz the total size of the of the data block which is returned
 * \return a pointer to the data block
 */
uint8_t *rex_block_write_material (uint64_t id, struct rex_header *header, struct rex_material_standard *mat, long *sz);

#ifdef __cplusplus
}
#endif
