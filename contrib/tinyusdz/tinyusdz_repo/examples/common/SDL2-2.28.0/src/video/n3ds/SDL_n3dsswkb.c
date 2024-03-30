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

#ifdef SDL_VIDEO_DRIVER_N3DS

#include <3ds.h>

#include "SDL_n3dsswkb.h"

static SwkbdState sw_keyboard;
const static size_t BUFFER_SIZE = 256;

void N3DS_SwkbInit()
{
    swkbdInit(&sw_keyboard, SWKBD_TYPE_NORMAL, 2, -1);
}

void N3DS_SwkbPoll()
{
    return;
}

void N3DS_SwkbQuit()
{
    return;
}

SDL_bool N3DS_HasScreenKeyboardSupport(_THIS)
{
    return SDL_TRUE;
}

void N3DS_StartTextInput(_THIS)
{
    char buffer[BUFFER_SIZE];
    SwkbdButton button_pressed;
    button_pressed = swkbdInputText(&sw_keyboard, buffer, BUFFER_SIZE);
    if (button_pressed == SWKBD_BUTTON_CONFIRM) {
        SDL_SendKeyboardText(buffer);
    }
}

void N3DS_StopTextInput(_THIS)
{
    return;
}

#endif /* SDL_VIDEO_DRIVER_N3DS */

/* vi: set sts=4 ts=4 sw=4 expandtab: */
