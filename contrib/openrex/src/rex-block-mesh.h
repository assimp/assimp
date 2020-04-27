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
 * \brief REX mesh block stored 3D geometry information
 *
 * The offsets in this block refer to the index of the beginning of this data block (this means the position of the lod
 * field, not the general data block header!). If one needs to access from the global REX stream, the offset of the
 * mesh block must be added.
 *
 * | **size [bytes]** | **name**       | **type** | **description**                                                  |
 * |------------------|----------------|----------|------------------------------------------------------------------|
 * | 2                | lod            | uint16_t | level of detail for the given geometry                           |
 * | 2                | maxLod         | uint16_t | maximal level of detail for given geometry                       |
 * | 4                | nrOfVtxCoords  | uint32_t | number of vertex coordinates                                     |
 * | 4                | nrOfNorCoords  | uint32_t | number of normal coordinates (can be zero)                       |
 * | 4                | nrOfTexCoords  | uint32_t | number of texture coordinates (can be zero)                      |
 * | 4                | nrOfVtxColors  | uint32_t | number of vertex colors (can be zero)                            |
 * | 4                | nrTriangles    | uint32_t | number of triangles                                              |
 * | 4                | startVtxCoords | uint32_t | start vertex coordinate block (relative to mesh block start)     |
 * | 4                | startNorCoords | uint32_t | start vertex normals block (relative to mesh block start)        |
 * | 4                | startTexCoords | uint32_t | start of texture coordinate block (relative to mesh block start) |
 * | 4                | startVtxColors | uint32_t | start of colors block (relative to mesh block start)             |
 * | 4                | startTriangles | uint32_t | start triangle block for vertices (relative to mesh block start) |
 * | 8                | materialId     | uint64_t | id which refers to the corresponding material block in this file |
 * | 2                | string size    | uint16_t | size of the following string name                                |
 * | 74               | name           | string   | name of the mesh (this is user-readable)                         |
 *
 * It is assumed that the mesh data is vertex-oriented, so that additional properties such as color, normals, or
 * texture information is equally sized to the nrOfVtxCoords. If not available the number should/can be 0.
 *
 * The mesh references a separate material block which is identified by the materialId (dataId of the material block).
 * Each DataMesh can only have one material block. This is similar to the `usemtl` in the OBJ file format. **If the
 * materialId is `INT64_MAX` (`Long.MAX_VALUE` in Java), then no material is available.**
 *
 * The mesh header size is fixed with **128** bytes.
 */

#include <stdint.h>
#include "rex-header.h"
#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a complete REX mesh. The reference to the according material is done by the materialId.
 * Please make sure that positions, normals, tex_coords, and colors
 * are vertex-aligned. This means that only one index is referring to all information.
 * However, it is valid to have normals, tex_coords, and colors being NULL.
 */
struct rex_mesh
{
    uint16_t lod;            //<! level of detail of the geometry (default 0)
    uint16_t max_lod;        //<! the maximal level of detail for this geometry (default 0)

    uint32_t nr_vertices;    //<! the number of vertices available in this structure
    uint32_t nr_triangles;   //<! the number of triangles stored in this structure

    float *positions;        //<! float array with coordinate information (xyzxyz...)
    float *normals;          //<! float array with normals or NULL
    float *tex_coords;       //<! float array with texture coordinates or NULL
    float *colors;           //<! float array with colors or NULL
    uint32_t *triangles;     //<! indices pointing to the coordinates spanning a triangle

    char name[REX_MESH_NAME_MAX_SIZE]; //<! the mesh name (user-readable)
    uint64_t material_id;              //<! id which refers to the corresponding material block in this file
};

/**
 * Reads a complete mesh data block. The ptr must point to the beginning of
 * the block. The mesh parameter must not be NULL.Please note that memory will
 * be allocated for the mesh data.
 *
 * \param ptr pointer to the block start
 * \param mesh the rex_mesh structure which gets filled
 * \return the pointer to the memory block after the rex_mesh block
 */
uint8_t *rex_block_read_mesh (uint8_t *ptr, struct rex_mesh *mesh);

/**
 * Writes the given rex_mesh block in a buffer. The buffer will be allocated, so the caller
 * must take care of releasing the memory.
 *
 * \param id the data block ID
 * \param header the REX header which gets modified according the the new block, can be NULL
 * \param mesh the image which should get serialized
 * \param sz the total size of the of the data block which is returned
 * \return a pointer to the data block
 */
uint8_t *rex_block_write_mesh (uint64_t id, struct rex_header *header, struct rex_mesh *mesh, long *sz);

/**
 * Sets all properties of the rex_mesh structure to initial values
 */
void rex_mesh_init (struct rex_mesh *mesh);

/**
 * Frees any memory which is allocated for rex_mesh
 */
void rex_mesh_free (struct rex_mesh *mesh);

/**
 * Simply dump an obj file with the stored vertex and triangle information
 */
void rex_mesh_dump_obj (struct rex_mesh *mesh);

#ifdef __cplusplus
}
#endif

