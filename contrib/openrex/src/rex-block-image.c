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
#include "rex-block-image.h"
#include "rex-block.h"
#include "util.h"

uint8_t *rex_block_write_image (uint64_t id, struct rex_header *header, struct rex_image *img, long *sz)
{
    MEM_CHECK (img)
    MEM_CHECK (img->data)

    *sz = REX_BLOCK_HEADER_SIZE + sizeof (uint32_t) + img->sz;

    uint8_t *ptr = malloc (*sz);
    memset (ptr, 0, *sz);
    uint8_t *addr = ptr;

    struct rex_block block = { .type = Image, .version = 1, .sz = *sz - REX_BLOCK_HEADER_SIZE, .id = id };
    ptr = rex_block_header_write (ptr, &block);

    rexcpyr (&img->compression, ptr, sizeof (uint32_t));
    rexcpyr (img->data, ptr, img->sz);

    if (header)
    {
        header->nr_datablocks += 1;
        header->sz_all_datablocks += *sz;
    }
    return addr;
}


uint8_t *rex_block_read_image (uint8_t *ptr, struct rex_image *img)
{
    MEM_CHECK (ptr)
    MEM_CHECK (img)

    rexcpy (&img->compression, ptr, sizeof (uint32_t));
    img->data = malloc (img->sz);

    rexcpy (img->data, ptr, img->sz);
    return ptr;
}
