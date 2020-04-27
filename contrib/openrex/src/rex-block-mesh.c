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

#include <stdio.h>

#include "global.h"
#include "rex-block-mesh.h"
#include "rex-block.h"
#include "status.h"
#include "util.h"

uint8_t *rex_block_write_mesh (uint64_t id, struct rex_header *header, struct rex_mesh *mesh, long *sz)
{
    MEM_CHECK (mesh)

    uint32_t nr_normals = (mesh->normals == NULL) ? 0 : mesh->nr_vertices;
    uint32_t nr_texcoords = (mesh->tex_coords == NULL) ? 0 : mesh->nr_vertices;
    uint32_t nr_colors = (mesh->colors == NULL) ? 0 : mesh->nr_vertices;

    // calculate total memory requirement
    *sz = REX_BLOCK_HEADER_SIZE
          + REX_MESH_HEADER_SIZE
          + mesh->nr_vertices * 12
          + nr_normals * 12
          + nr_texcoords * 8
          + nr_colors * 12
          + mesh->nr_triangles * 12;

    uint8_t *ptr = malloc (*sz);
    memset (ptr, 0, *sz);
    uint8_t *addr = ptr;

    struct rex_block block = { .type = Mesh, .version = 1, .sz = *sz - REX_BLOCK_HEADER_SIZE, .id = id };
    ptr = rex_block_header_write (ptr, &block);

    // block data
    rexcpyr (&mesh->lod, ptr, sizeof (uint16_t));
    rexcpyr (&mesh->max_lod, ptr, sizeof (uint16_t));
    rexcpyr (&mesh->nr_vertices, ptr, sizeof (uint32_t));

    rexcpyr (&nr_normals, ptr, sizeof (uint32_t));
    rexcpyr (&nr_texcoords, ptr, sizeof (uint32_t));
    rexcpyr (&nr_colors, ptr, sizeof (uint32_t));

    rexcpyr (&mesh->nr_triangles, ptr, sizeof (uint32_t));

    // offset is relative from the beginning of the block (without the block header)
    uint32_t start_coords = REX_MESH_HEADER_SIZE;
    uint32_t start_normals = REX_MESH_HEADER_SIZE + mesh->nr_vertices * 12;
    uint32_t start_texcoords = start_normals + nr_normals * 12;
    uint32_t start_colors = start_texcoords + nr_texcoords * 8;
    uint32_t start_triangles = start_colors + nr_colors * 12;

    rexcpyr (&start_coords, ptr, sizeof (uint32_t));
    rexcpyr (&start_normals, ptr, sizeof (uint32_t));
    rexcpyr (&start_texcoords, ptr, sizeof (uint32_t));
    rexcpyr (&start_colors, ptr, sizeof (uint32_t));
    rexcpyr (&start_triangles, ptr, sizeof (uint32_t));

    rexcpyr (&mesh->material_id, ptr, sizeof (uint64_t));

    uint16_t name_sz = (uint16_t) strlen (mesh->name);
    rexcpyr (&name_sz, ptr, sizeof (uint16_t));
    rexcpyr (mesh->name, ptr, 74);

    if (mesh->nr_vertices)
        rexcpyr (mesh->positions, ptr, mesh->nr_vertices * 12);
    if (nr_normals)
        rexcpyr (mesh->normals, ptr, nr_normals * 12);
    if (nr_texcoords)
        rexcpyr (mesh->tex_coords, ptr, nr_texcoords * 8);
    if (nr_colors)
        rexcpyr (mesh->colors, ptr, nr_colors * 12);
    if (mesh->nr_triangles)
        rexcpyr (mesh->triangles, ptr, mesh->nr_triangles * 12);

    if (header)
    {
        header->nr_datablocks += 1;
        header->sz_all_datablocks += *sz;
    }

    return addr;
}


uint8_t *rex_block_read_mesh (uint8_t *ptr, struct rex_mesh *mesh)
{
    MEM_CHECK (ptr)
    MEM_CHECK (mesh)

    rex_mesh_init (mesh);

    rexcpy (&mesh->lod, ptr, sizeof (uint16_t));
    rexcpy (&mesh->max_lod, ptr, sizeof (uint16_t));
    rexcpy (&mesh->nr_vertices, ptr, sizeof (uint32_t));

    uint32_t nr_normals;
    uint32_t nr_texcoords;
    uint32_t nr_colors;

    rexcpy (&nr_normals, ptr, sizeof (uint32_t));
    rexcpy (&nr_texcoords, ptr, sizeof (uint32_t));
    rexcpy (&nr_colors, ptr, sizeof (uint32_t));

    rexcpy (&mesh->nr_triangles, ptr, sizeof (uint32_t));

    uint32_t start_coords;
    uint32_t start_normals;
    uint32_t start_texcoords;
    uint32_t start_colors;
    uint32_t start_triangles;

    rexcpy (&start_coords, ptr, sizeof (uint32_t));
    rexcpy (&start_normals, ptr, sizeof (uint32_t));
    rexcpy (&start_texcoords, ptr, sizeof (uint32_t));
    rexcpy (&start_colors, ptr, sizeof (uint32_t));
    rexcpy (&start_triangles, ptr, sizeof (uint32_t));

    rexcpy (&mesh->material_id, ptr, sizeof (uint64_t));

    uint16_t sz; // not used anymore since string is fixed size
    rexcpy (&sz, ptr, sizeof (uint16_t));
    rexcpy (mesh->name, ptr, 74);

    // read positions
    if (mesh->nr_vertices)
    {
        mesh->positions = malloc (mesh->nr_vertices * 12);
        rexcpy (mesh->positions, ptr, mesh->nr_vertices * 12);
    }

    // read normals
    if (nr_normals)
    {
        mesh->normals = malloc (nr_normals * 12);
        rexcpy (mesh->normals, ptr, nr_normals * 12);
    }

    // read texture coords
    if (nr_texcoords)
    {
        mesh->tex_coords = malloc (nr_texcoords * 8);
        rexcpy (mesh->tex_coords, ptr, nr_texcoords * 8);
    }

    // read colors
    if (nr_colors)
    {
        mesh->colors = malloc (nr_colors * 12);
        rexcpy (mesh->colors, ptr, nr_colors * 12);
    }

    // read triangles
    if (mesh->nr_triangles)
    {
        mesh->triangles = malloc (mesh->nr_triangles * 12);
        rexcpy (mesh->triangles, ptr, mesh->nr_triangles * 12);
    }

    return ptr;
}

void rex_mesh_init (struct rex_mesh *mesh)
{
    if (!mesh) return;

    mesh->lod = 0;
    mesh->max_lod = 0;

    mesh->nr_vertices = 0;
    mesh->nr_triangles = 0;

    mesh->positions = 0;
    mesh->normals = 0;
    mesh->tex_coords = 0;
    mesh->colors = 0;
    mesh->triangles = 0;

    memset (mesh->name, 0, 74);
    mesh->material_id = REX_NOT_SET;
}

void rex_mesh_free (struct rex_mesh *mesh)
{
    if (!mesh) return;

    if (mesh->positions)
        FREE (mesh->positions);
    if (mesh->normals)
        FREE (mesh->normals);
    if (mesh->tex_coords)
        FREE (mesh->tex_coords);
    if (mesh->colors)
        FREE (mesh->colors);
    if (mesh->triangles)
        FREE (mesh->triangles);
    rex_mesh_init (mesh);
}

void rex_mesh_dump_obj (struct rex_mesh *mesh)
{
    if (!mesh) return;

    if (mesh->positions)
    {
        float *p = mesh->positions;
        for (unsigned int i = 0; i < mesh->nr_vertices * 3; i += 3)
            printf ("v %f %f %f\n", p[i], p[i + 1], p[i + 2]);
    }

    if (mesh->triangles)
    {
        uint32_t *t = mesh->triangles;
        for (unsigned int i = 0; i < mesh->nr_triangles * 3; i += 3)
            printf ("f %d %d %d\n", t[i] + 1, t[i + 1] + 1, t[i + 2] + 1);
    }
}
