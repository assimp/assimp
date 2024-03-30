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

#include <sys/stat.h>
#include <unistd.h>

#if defined(SDL_FILESYSTEM_PSP)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent filesystem routines                                */

#include "SDL_error.h"
#include "SDL_filesystem.h"

char *SDL_GetBasePath(void)
{
    char *retval = NULL;
    size_t len;
    char cwd[FILENAME_MAX];

    getcwd(cwd, sizeof(cwd));
    len = SDL_strlen(cwd) + 2;
    retval = (char *)SDL_malloc(len);
    SDL_snprintf(retval, len, "%s/", cwd);

    return retval;
}

char *SDL_GetPrefPath(const char *org, const char *app)
{
    char *retval = NULL;
    size_t len;
    char *base = SDL_GetBasePath();
    if (app == NULL) {
        SDL_InvalidParamError("app");
        return NULL;
    }
    if (org == NULL) {
        org = "";
    }

    len = SDL_strlen(base) + SDL_strlen(org) + SDL_strlen(app) + 4;
    retval = (char *)SDL_malloc(len);

    if (*org) {
        SDL_snprintf(retval, len, "%s%s/%s/", base, org, app);
    } else {
        SDL_snprintf(retval, len, "%s%s/", base, app);
    }
    free(base);

    mkdir(retval, 0755);
    return retval;
}

#endif /* SDL_FILESYSTEM_PSP */

/* vi: set ts=4 sw=4 expandtab: */
