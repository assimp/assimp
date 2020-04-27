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
#include "rex-block-pointlist.h"
#include "rex-block.h"
#include "util.h"

uint8_t *rex_block_write_pointlist (uint64_t id, struct rex_header *header, struct rex_pointlist *plist, long *sz)
{
    MEM_CHECK (plist)

    *sz = REX_BLOCK_HEADER_SIZE
          + sizeof (uint32_t)
          + sizeof (uint32_t)
          + plist->nr_vertices * 12
          + plist->nr_colors * 12;

    uint8_t *ptr = malloc (*sz);
    memset (ptr, 0, *sz);
    uint8_t *addr = ptr;

    struct rex_block block = { .type = PointList, .version = 1, .sz = *sz - REX_BLOCK_HEADER_SIZE, .id = id };
    ptr = rex_block_header_write (ptr, &block);

    rexcpyr (&plist->nr_vertices, ptr, sizeof (uint32_t));
    rexcpyr (&plist->nr_colors, ptr, sizeof (uint32_t));

    if (plist->nr_vertices)
        rexcpyr (plist->positions, ptr, plist->nr_vertices * 12);

    // check if length are matching
    if (plist->nr_colors && plist->nr_colors != plist->nr_vertices)
    {
        warn ("Number of colors does not match number of vertices");
        FREE (addr);
        return NULL;
    }

    if (plist->nr_colors)
        rexcpyr (plist->colors, ptr, plist->nr_colors * 12);

    if (header)
    {
        header->nr_datablocks += 1;
        header->sz_all_datablocks += *sz;
    }
    return addr;
}

uint8_t *rex_block_read_pointlist (uint8_t *ptr, struct rex_pointlist *plist)
{
    MEM_CHECK (ptr)
    MEM_CHECK (plist)

    rexcpy (&plist->nr_vertices, ptr, sizeof (uint32_t));
    rexcpy (&plist->nr_colors, ptr, sizeof (uint32_t));

    // read positions
    if (plist->nr_vertices)
    {
        plist->positions = malloc (plist->nr_vertices * 12);
        rexcpy (plist->positions, ptr, plist->nr_vertices * 12);
    }

    // read colors
    if (plist->nr_colors)
    {
        plist->colors = malloc (plist->nr_colors * 12);
        rexcpy (plist->colors, ptr, plist->nr_colors * 12);
    }

    return ptr;
}

void rex_pointlist_init (struct rex_pointlist *plist)
{
    if (!plist) return;

    plist->nr_vertices = 0;
    plist->nr_colors = 0;

    plist->positions = 0;
    plist->colors = 0;
}

void rex_pointlist_free (struct rex_pointlist *plist)
{
    if (!plist) return;

    if (plist->positions)
        FREE (plist->positions);
    if (plist->colors)
        FREE (plist->colors);
    rex_pointlist_init (plist);
}
