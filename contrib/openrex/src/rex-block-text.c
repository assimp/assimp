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
#include "rex-block-text.h"
#include "rex-block.h"
#include "util.h"

uint8_t *rex_block_write_text (uint64_t id, struct rex_header *header, struct rex_text *text, long *sz)
{
    MEM_CHECK (text)
    MEM_CHECK (text->data)

    uint16_t text_len = (uint16_t) strlen (text->data);

    *sz = REX_BLOCK_HEADER_SIZE
          + sizeof (float) * 4 // RGBA
          + sizeof (float) * 3 // position
          + sizeof (float)     // font size
          + sizeof (uint16_t)  // text size
          + text_len;

    uint8_t *ptr = malloc (*sz);
    memset (ptr, 0, *sz);
    uint8_t *addr = ptr;

    struct rex_block block = { .type = Text, .version = 1, .sz = *sz - REX_BLOCK_HEADER_SIZE, .id = id };
    ptr = rex_block_header_write (ptr, &block);

    rexcpyr (&text->red, ptr, sizeof (float));
    rexcpyr (&text->green, ptr, sizeof (float));
    rexcpyr (&text->blue, ptr, sizeof (float));
    rexcpyr (&text->alpha, ptr, sizeof (float));
    rexcpyr (text->position, ptr, sizeof (float) * 3);
    rexcpyr (&text->font_size, ptr, sizeof (float));
    rexcpyr (&text_len, ptr, sizeof (uint16_t));
    rexcpyr (text->data, ptr, text_len);

    if (header)
    {
        header->nr_datablocks += 1;
        header->sz_all_datablocks += *sz;
    }
    return addr;
}


uint8_t *rex_block_read_text (uint8_t *ptr, struct rex_text *text)
{
    MEM_CHECK (ptr)
    MEM_CHECK (text)

    uint16_t text_len;
    rexcpy (&text->red, ptr, sizeof (float));
    rexcpy (&text->green, ptr, sizeof (float));
    rexcpy (&text->blue, ptr, sizeof (float));
    rexcpy (&text->alpha, ptr, sizeof (float));
    rexcpy (text->position, ptr, sizeof (float) * 3);
    rexcpy (&text->font_size, ptr, sizeof (float));
    rexcpy (&text_len, ptr, sizeof (uint16_t));

    text->data = malloc (text_len + 1);

    rexcpy (text->data, ptr, text_len);
    text->data[text_len] = '\0';
    return ptr;
}
