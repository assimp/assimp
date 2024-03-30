/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_RISCOS

#include "../SDL_sysvideo.h"
#include "../../events/SDL_mouse_c.h"

#include "SDL_riscosvideo.h"
#include "SDL_riscosmodes.h"

#include <kernel.h>
#include <swis.h>

enum
{
    MODE_FLAG_565 = 1 << 7,

    MODE_FLAG_COLOUR_SPACE = 0xF << 12,

    MODE_FLAG_TBGR = 0,
    MODE_FLAG_TRGB = 1 << 14,
    MODE_FLAG_ABGR = 1 << 15,
    MODE_FLAG_ARGB = MODE_FLAG_TRGB | MODE_FLAG_ABGR
};

static const struct
{
    SDL_PixelFormatEnum pixel_format;
    int modeflags, ncolour, log2bpp;
} mode_to_pixelformat[] = {
    /* { SDL_PIXELFORMAT_INDEX1LSB, 0, 1, 0 }, */
    /* { SDL_PIXELFORMAT_INDEX2LSB, 0, 3, 1 }, */
    /* { SDL_PIXELFORMAT_INDEX4LSB, 0, 15, 2 }, */
    /* { SDL_PIXELFORMAT_INDEX8,    MODE_FLAG_565, 255, 3 }, */
    { SDL_PIXELFORMAT_XBGR1555, MODE_FLAG_TBGR, 65535, 4 },
    { SDL_PIXELFORMAT_XRGB1555, MODE_FLAG_TRGB, 65535, 4 },
    { SDL_PIXELFORMAT_ABGR1555, MODE_FLAG_ABGR, 65535, 4 },
    { SDL_PIXELFORMAT_ARGB1555, MODE_FLAG_ARGB, 65535, 4 },
    { SDL_PIXELFORMAT_XBGR4444, MODE_FLAG_TBGR, 4095, 4 },
    { SDL_PIXELFORMAT_XRGB4444, MODE_FLAG_TRGB, 4095, 4 },
    { SDL_PIXELFORMAT_ABGR4444, MODE_FLAG_ABGR, 4095, 4 },
    { SDL_PIXELFORMAT_ARGB4444, MODE_FLAG_ARGB, 4095, 4 },
    { SDL_PIXELFORMAT_BGR565, MODE_FLAG_TBGR | MODE_FLAG_565, 65535, 4 },
    { SDL_PIXELFORMAT_RGB565, MODE_FLAG_TRGB | MODE_FLAG_565, 65535, 4 },
    { SDL_PIXELFORMAT_BGR24, MODE_FLAG_TBGR, 16777215, 6 },
    { SDL_PIXELFORMAT_RGB24, MODE_FLAG_TRGB, 16777215, 6 },
    { SDL_PIXELFORMAT_XBGR8888, MODE_FLAG_TBGR, -1, 5 },
    { SDL_PIXELFORMAT_XRGB8888, MODE_FLAG_TRGB, -1, 5 },
    { SDL_PIXELFORMAT_ABGR8888, MODE_FLAG_ABGR, -1, 5 },
    { SDL_PIXELFORMAT_ARGB8888, MODE_FLAG_ARGB, -1, 5 }
};

static SDL_PixelFormatEnum RISCOS_ModeToPixelFormat(int ncolour, int modeflags, int log2bpp)
{
    int i;

    for (i = 0; i < SDL_arraysize(mode_to_pixelformat); i++) {
        if (log2bpp == mode_to_pixelformat[i].log2bpp &&
            (ncolour == mode_to_pixelformat[i].ncolour || ncolour == 0) &&
            (modeflags & (MODE_FLAG_565 | MODE_FLAG_COLOUR_SPACE)) == mode_to_pixelformat[i].modeflags) {
            return mode_to_pixelformat[i].pixel_format;
        }
    }

    return SDL_PIXELFORMAT_UNKNOWN;
}

static size_t measure_mode_block(const int *block)
{
    size_t blockSize = ((block[0] & 0xFF) == 3) ? 7 : 5;
    while (block[blockSize] != -1) {
        blockSize += 2;
    }
    blockSize++;

    return blockSize * 4;
}

static int read_mode_variable(int *block, int var)
{
    _kernel_swi_regs regs;
    regs.r[0] = (int)block;
    regs.r[1] = var;
    _kernel_swi(OS_ReadModeVariable, &regs, &regs);
    return regs.r[2];
}

static SDL_bool read_mode_block(int *block, SDL_DisplayMode *mode, SDL_bool extended)
{
    int xres, yres, ncolour, modeflags, log2bpp, rate;

    if ((block[0] & 0xFF) == 1) {
        xres = block[1];
        yres = block[2];
        log2bpp = block[3];
        rate = block[4];
        ncolour = (1 << (1 << log2bpp)) - 1;
        modeflags = MODE_FLAG_TBGR;
    } else if ((block[0] & 0xFF) == 3) {
        xres = block[1];
        yres = block[2];
        ncolour = block[3];
        modeflags = block[4];
        log2bpp = block[5];
        rate = block[6];
    } else {
        return SDL_FALSE;
    }

    if (extended) {
        xres = read_mode_variable(block, 11) + 1;
        yres = read_mode_variable(block, 12) + 1;
        log2bpp = read_mode_variable(block, 9);
        ncolour = read_mode_variable(block, 3);
        modeflags = read_mode_variable(block, 0);
    }

    mode->w = xres;
    mode->h = yres;
    mode->format = RISCOS_ModeToPixelFormat(ncolour, modeflags, log2bpp);
    mode->refresh_rate = rate;

    return SDL_TRUE;
}

static void *convert_mode_block(const int *block)
{
    int xres, yres, log2bpp, rate, ncolour = 0, modeflags = 0;
    size_t pos = 0;
    int *dst;

    if ((block[0] & 0xFF) == 1) {
        xres = block[1];
        yres = block[2];
        log2bpp = block[3];
        rate = block[4];
    } else if ((block[0] & 0xFF) == 3) {
        xres = block[1];
        yres = block[2];
        ncolour = block[3];
        modeflags = block[4];
        log2bpp = block[5];
        rate = block[6];
    } else {
        return NULL;
    }

    dst = SDL_malloc(40);
    if (dst == NULL) {
        return NULL;
    }

    dst[pos++] = 1;
    dst[pos++] = xres;
    dst[pos++] = yres;
    dst[pos++] = log2bpp;
    dst[pos++] = rate;
    if (ncolour != 0) {
        dst[pos++] = 3;
        dst[pos++] = ncolour;
    }
    if (modeflags != 0) {
        dst[pos++] = 0;
        dst[pos++] = modeflags;
    }
    dst[pos++] = -1;

    return dst;
}

static void *copy_memory(const void *src, size_t size, size_t alloc)
{
    void *dst = SDL_malloc(alloc);
    if (dst) {
        SDL_memcpy(dst, src, size);
    }
    return dst;
}

int RISCOS_InitModes(_THIS)
{
    SDL_DisplayMode mode;
    int *current_mode;
    _kernel_swi_regs regs;
    _kernel_oserror *error;
    size_t size;

    regs.r[0] = 1;
    error = _kernel_swi(OS_ScreenMode, &regs, &regs);
    if (error != NULL) {
        return SDL_SetError("Unable to retrieve the current screen mode: %s (%i)", error->errmess, error->errnum);
    }

    current_mode = (int *)regs.r[1];
    if (!read_mode_block(current_mode, &mode, SDL_TRUE)) {
        return SDL_SetError("Unsupported mode block format %d", current_mode[0]);
    }

    size = measure_mode_block(current_mode);
    mode.driverdata = copy_memory(current_mode, size, size);
    if (!mode.driverdata) {
        return SDL_OutOfMemory();
    }

    return SDL_AddBasicVideoDisplay(&mode);
}

void RISCOS_GetDisplayModes(_THIS, SDL_VideoDisplay *display)
{
    SDL_DisplayMode mode;
    _kernel_swi_regs regs;
    _kernel_oserror *error;
    void *block, *pos;

    regs.r[0] = 2;
    regs.r[2] = 0;
    regs.r[6] = 0;
    regs.r[7] = 0;
    error = _kernel_swi(OS_ScreenMode, &regs, &regs);
    if (error != NULL) {
        SDL_SetError("Unable to enumerate screen modes: %s (%i)", error->errmess, error->errnum);
        return;
    }

    block = SDL_malloc(-regs.r[7]);
    if (block == NULL) {
        SDL_OutOfMemory();
        return;
    }

    regs.r[6] = (int)block;
    regs.r[7] = -regs.r[7];
    error = _kernel_swi(OS_ScreenMode, &regs, &regs);
    if (error != NULL) {
        SDL_free(block);
        SDL_SetError("Unable to enumerate screen modes: %s (%i)", error->errmess, error->errnum);
        return;
    }

    for (pos = block; pos < (void *)regs.r[6]; pos += *((int *)pos)) {
        if (!read_mode_block(pos + 4, &mode, SDL_FALSE)) {
            continue;
        }

        if (mode.format == SDL_PIXELFORMAT_UNKNOWN) {
            continue;
        }

        mode.driverdata = convert_mode_block(pos + 4);
        if (!mode.driverdata) {
            SDL_OutOfMemory();
            break;
        }

        if (!SDL_AddDisplayMode(display, &mode)) {
            SDL_free(mode.driverdata);
        }
    }

    SDL_free(block);
}

int RISCOS_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    const char disable_cursor[] = { 23, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    _kernel_swi_regs regs;
    _kernel_oserror *error;
    int i;

    regs.r[0] = 0;
    regs.r[1] = (int)mode->driverdata;
    error = _kernel_swi(OS_ScreenMode, &regs, &regs);
    if (error != NULL) {
        return SDL_SetError("Unable to set the current screen mode: %s (%i)", error->errmess, error->errnum);
    }

    /* Turn the text cursor off */
    for (i = 0; i < SDL_arraysize(disable_cursor); i++) {
        _kernel_oswrch(disable_cursor[i]);
    }

    /* Update cursor visibility, since it may have been disabled by the mode change. */
    SDL_SetCursor(NULL);

    return 0;
}

#endif /* SDL_VIDEO_DRIVER_RISCOS */

/* vi: set ts=4 sw=4 expandtab: */
