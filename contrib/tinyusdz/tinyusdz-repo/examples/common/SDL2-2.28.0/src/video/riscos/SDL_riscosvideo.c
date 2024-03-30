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

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_riscosvideo.h"
#include "SDL_riscosevents_c.h"
#include "SDL_riscosframebuffer_c.h"
#include "SDL_riscosmouse.h"
#include "SDL_riscosmodes.h"
#include "SDL_riscoswindow.h"

#define RISCOSVID_DRIVER_NAME "riscos"

/* Initialization/Query functions */
static int RISCOS_VideoInit(_THIS);
static void RISCOS_VideoQuit(_THIS);

/* RISC OS driver bootstrap functions */

static void RISCOS_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device->driverdata);
    SDL_free(device);
}

static SDL_VideoDevice *RISCOS_CreateDevice(void)
{
    SDL_VideoDevice *device;
    SDL_VideoData *phdata;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (device == NULL) {
        SDL_OutOfMemory();
        return 0;
    }

    /* Initialize internal data */
    phdata = (SDL_VideoData *)SDL_calloc(1, sizeof(SDL_VideoData));
    if (phdata == NULL) {
        SDL_OutOfMemory();
        SDL_free(device);
        return NULL;
    }

    device->driverdata = phdata;

    /* Set the function pointers */
    device->VideoInit = RISCOS_VideoInit;
    device->VideoQuit = RISCOS_VideoQuit;
    device->PumpEvents = RISCOS_PumpEvents;

    device->GetDisplayModes = RISCOS_GetDisplayModes;
    device->SetDisplayMode = RISCOS_SetDisplayMode;

    device->CreateSDLWindow = RISCOS_CreateWindow;
    device->DestroyWindow = RISCOS_DestroyWindow;
    device->GetWindowWMInfo = RISCOS_GetWindowWMInfo;

    device->CreateWindowFramebuffer = RISCOS_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = RISCOS_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = RISCOS_DestroyWindowFramebuffer;

    device->free = RISCOS_DeleteDevice;

    return device;
}

VideoBootStrap RISCOS_bootstrap = {
    RISCOSVID_DRIVER_NAME, "SDL RISC OS video driver",
    RISCOS_CreateDevice
};

static int RISCOS_VideoInit(_THIS)
{
    if (RISCOS_InitEvents(_this) < 0) {
        return -1;
    }

    if (RISCOS_InitMouse(_this) < 0) {
        return -1;
    }

    if (RISCOS_InitModes(_this) < 0) {
        return -1;
    }

    /* We're done! */
    return 0;
}

static void RISCOS_VideoQuit(_THIS)
{
    RISCOS_QuitEvents(_this);
}

#endif /* SDL_VIDEO_DRIVER_RISCOS */

/* vi: set ts=4 sw=4 expandtab: */
