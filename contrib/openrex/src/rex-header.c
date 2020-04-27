#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "global.h"
#include "rex-header.h"
#include "util.h"

uint8_t *rex_header_write (struct rex_header *header, long *sz)
{
    *sz = 64 + 22; // we also allocate for the CSB which is currently unused
    uint8_t *buf = malloc (*sz);
    memset (buf, 0, *sz);
    uint8_t *addr = buf;

    // fixed due to fixed CSB
    header->start_addr = 86;

    rexcpyr (header->magic, buf, 4);
    rexcpyr (&header->version, buf, sizeof (uint16_t));
    rexcpyr (&header->crc, buf, sizeof (uint32_t));
    rexcpyr (&header->nr_datablocks, buf, sizeof (uint16_t));
    rexcpyr (&header->start_addr, buf, sizeof (uint16_t));
    rexcpyr (&header->sz_all_datablocks, buf, sizeof (uint64_t));
    rexcpyr (header->reserved, buf, 42);

    // write dummy CSB
    uint32_t srid = 3876;
    uint16_t name_sz = 4;
    char name[] = "EPSG";
    float ofs_x = 0.0f;
    float ofs_y = 0.0f;
    float ofs_z = 0.0f;

    rexcpyr (&srid, buf, sizeof (uint32_t));
    rexcpyr (&name_sz, buf, sizeof (uint16_t));
    rexcpyr (name, buf, 4);
    rexcpyr (&ofs_x, buf, sizeof (float));
    rexcpyr (&ofs_y, buf, sizeof (float));
    rexcpyr (&ofs_z, buf, sizeof (float));

    assert ( (buf - *sz) == addr);

    return addr;
}

struct rex_header *rex_header_create ()
{
    struct rex_header *header = malloc (sizeof (struct rex_header));
    header->version = REX_FILE_VERSION;
    header->crc = 0;
    header->nr_datablocks = 0;
    header->start_addr = 0;
    header->sz_all_datablocks = 0;

    memcpy (header->magic, REX_FILE_MAGIC, 4);
    memset (header->reserved, 0, 42);
    return header;
}

uint8_t *rex_header_read (uint8_t *buf, struct rex_header *header)
{
    MEM_CHECK (buf);
    uint8_t *start = buf;

    rexcpy (header->magic, buf, 4);
    rexcpy (&header->version, buf, sizeof (uint16_t));
    rexcpy (&header->crc, buf, sizeof (uint32_t));
    rexcpy (&header->nr_datablocks, buf, sizeof (uint16_t));
    rexcpy (&header->start_addr, buf, sizeof (uint16_t));
    rexcpy (&header->sz_all_datablocks, buf, sizeof (uint64_t));
    rexcpy (header->reserved, buf, 42);

    if (strncmp (header->magic, "REX1", 4) != 0)
        die ("This is not a valid REX file");

    // NOTE: we skip the coordinate system block because it is not used
    return start + header->start_addr;
}
