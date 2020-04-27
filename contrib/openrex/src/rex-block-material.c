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
#include "rex-block-material.h"
#include "rex-block.h"
#include "util.h"

uint8_t *rex_block_write_material (uint64_t id, struct rex_header *header, struct rex_material_standard *mat, long *sz)
{
    MEM_CHECK (mat)

    *sz = REX_BLOCK_HEADER_SIZE + REX_MATERIAL_STANDARD_SIZE;

    uint8_t *ptr = malloc (*sz);
    memset (ptr, 0, *sz);
    uint8_t *addr = ptr;

    struct rex_block block = { .type = MaterialStandard, .version = 1, .sz = *sz - REX_BLOCK_HEADER_SIZE, .id = id };
    ptr = rex_block_header_write (ptr, &block);

    rexcpyr (&mat->ka_red, ptr, sizeof (float));
    rexcpyr (&mat->ka_green, ptr, sizeof (float));
    rexcpyr (&mat->ka_blue, ptr, sizeof (float));
    rexcpyr (&mat->ka_textureId, ptr, sizeof (uint64_t));
    rexcpyr (&mat->kd_red, ptr, sizeof (float));
    rexcpyr (&mat->kd_green, ptr, sizeof (float));
    rexcpyr (&mat->kd_blue, ptr, sizeof (float));
    rexcpyr (&mat->kd_textureId, ptr, sizeof (uint64_t));
    rexcpyr (&mat->ks_red, ptr, sizeof (float));
    rexcpyr (&mat->ks_green, ptr, sizeof (float));
    rexcpyr (&mat->ks_blue, ptr, sizeof (float));
    rexcpyr (&mat->ks_textureId, ptr, sizeof (uint64_t));
    rexcpyr (&mat->ns, ptr, sizeof (float));
    rexcpyr (&mat->alpha, ptr, sizeof (float));

    if (header)
    {
        header->nr_datablocks += 1;
        header->sz_all_datablocks += *sz;
    }
    return addr;
}

uint8_t *rex_block_read_material (uint8_t *ptr, struct rex_material_standard *mat)
{
    MEM_CHECK (ptr)
    MEM_CHECK (mat)

    rexcpy (&mat->ka_red, ptr, sizeof (float));
    rexcpy (&mat->ka_green, ptr, sizeof (float));
    rexcpy (&mat->ka_blue, ptr, sizeof (float));
    rexcpy (&mat->ka_textureId, ptr, sizeof (uint64_t));
    rexcpy (&mat->kd_red, ptr, sizeof (float));
    rexcpy (&mat->kd_green, ptr, sizeof (float));
    rexcpy (&mat->kd_blue, ptr, sizeof (float));
    rexcpy (&mat->kd_textureId, ptr, sizeof (uint64_t));
    rexcpy (&mat->ks_red, ptr, sizeof (float));
    rexcpy (&mat->ks_green, ptr, sizeof (float));
    rexcpy (&mat->ks_blue, ptr, sizeof (float));
    rexcpy (&mat->ks_textureId, ptr, sizeof (uint64_t));
    rexcpy (&mat->ns, ptr, sizeof (float));
    rexcpy (&mat->alpha, ptr, sizeof (float));
    return ptr;
}
