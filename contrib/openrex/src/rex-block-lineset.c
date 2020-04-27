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

#include "global.h"
#include "rex-block-lineset.h"
#include "rex-block.h"
#include "util.h"


uint8_t *rex_block_write_lineset (uint64_t id, struct rex_header *header, struct rex_lineset *lineset, long *sz)
{
    MEM_CHECK (lineset)
    MEM_CHECK (lineset->vertices)

    *sz = REX_BLOCK_HEADER_SIZE
          + sizeof (uint32_t)  // nr_vertices
          + sizeof (float) * 4 // RGBA
          + lineset->nr_vertices * 3 * sizeof (float);

    uint8_t *ptr = malloc (*sz);
    memset (ptr, 0, *sz);
    uint8_t *addr = ptr;

    struct rex_block block = { .type = LineSet, .version = 1, .sz = *sz - REX_BLOCK_HEADER_SIZE, .id = id };
    ptr = rex_block_header_write (ptr, &block);

    rexcpyr (&lineset->red, ptr, sizeof (float));
    rexcpyr (&lineset->green, ptr, sizeof (float));
    rexcpyr (&lineset->blue, ptr, sizeof (float));
    rexcpyr (&lineset->alpha, ptr, sizeof (float));
    rexcpyr (&lineset->nr_vertices, ptr, sizeof (uint32_t));
    rexcpyr (lineset->vertices, ptr, sizeof (float) * lineset->nr_vertices * 3);

    if (header)
    {
        header->nr_datablocks += 1;
        header->sz_all_datablocks += *sz;
    }
    return addr;
}


uint8_t *rex_block_read_lineset (uint8_t *ptr, struct rex_lineset *lineset)
{
    MEM_CHECK (ptr)
    MEM_CHECK (lineset)

    rexcpy (&lineset->red, ptr, sizeof (float));
    rexcpy (&lineset->green, ptr, sizeof (float));
    rexcpy (&lineset->blue, ptr, sizeof (float));
    rexcpy (&lineset->alpha, ptr, sizeof (float));
    rexcpy (&lineset->nr_vertices, ptr, sizeof (uint32_t));

    lineset->vertices = malloc (lineset->nr_vertices * sizeof (float) * 3);

    rexcpy (lineset->vertices, ptr, sizeof (float) * lineset->nr_vertices * 3);
    return ptr;
}
