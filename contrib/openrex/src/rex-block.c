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
#include "rex-block-image.h"
#include "rex-block-lineset.h"
#include "rex-block-material.h"
#include "rex-block-mesh.h"
#include "rex-block-pointlist.h"
#include "rex-block-text.h"
#include "rex-block.h"
#include "status.h"
#include "util.h"

uint8_t *rex_block_header_write (uint8_t *ptr, struct rex_block *block)
{
    MEM_CHECK (block)
    rexcpyr (&block->type, ptr, sizeof (uint16_t));
    rexcpyr (&block->version, ptr, sizeof (uint16_t));
    rexcpyr (&block->sz, ptr, sizeof (uint32_t));
    rexcpyr (&block->id, ptr, sizeof (uint64_t));
    return ptr;
}

uint8_t *rex_block_read (uint8_t *ptr, struct rex_block *block)
{
    MEM_CHECK (ptr);
    MEM_CHECK (block);

    rexcpy (&block->type,    ptr, sizeof (uint16_t));
    rexcpy (&block->version, ptr, sizeof (uint16_t));
    rexcpy (&block->sz,      ptr, sizeof (uint32_t));
    rexcpy (&block->id,      ptr, sizeof (uint64_t));

    uint8_t *data_start = ptr;

    switch (block->type)
    {
        case LineSet:
            {
                struct rex_lineset *lineset = malloc (sizeof (struct rex_lineset));
                ptr = rex_block_read_lineset (ptr, lineset);
                block->data = lineset;
                break;
            }
        case Text:
            {
                struct rex_text *text = malloc (sizeof (struct rex_text));
                ptr = rex_block_read_text (ptr, text);
                block->data = text;
                break;
            }
        case PointList:
            {
                struct rex_pointlist *p = malloc (sizeof (struct rex_pointlist));
                ptr = rex_block_read_pointlist (ptr, p);
                block->data = p;
                break;
            }
        case Mesh:
            {
                struct rex_mesh *mesh = malloc (sizeof (struct rex_mesh));
                ptr = rex_block_read_mesh (ptr, mesh);
                block->data = mesh;
                break;
            }
        case Image:
            {
                struct rex_image *img = malloc (sizeof (struct rex_image));
                img->sz = block->sz - sizeof (uint32_t); // subtract compression
                ptr = rex_block_read_image (ptr, img);
                block->data = img;
                break;
            }
        case MaterialStandard:
            {
                struct rex_material_standard *mat = malloc (sizeof (struct rex_material_standard));
                ptr = rex_block_read_material (ptr, mat);
                block->data = mat;
                break;
            }
        case PeopleSimulation:
            warn ("PeopleSimulation is not yet implemented");
            return  data_start + block->sz;
            break;
        default:
            warn ("Not supported REX block, skipping.");
            return  data_start + block->sz;
    }
    return ptr;
}
