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

#if SDL_THREAD_VITA

/* VITA thread management routines for SDL */

#include <stdio.h>
#include <stdlib.h>

#include "SDL_error.h"
#include "SDL_thread.h"
#include "../SDL_systhread.h"
#include "../SDL_thread_c.h"
#include <psp2/types.h>
#include <psp2/kernel/threadmgr.h>

#define VITA_THREAD_STACK_SIZE_MIN     0x1000    // 4KiB
#define VITA_THREAD_STACK_SIZE_MAX     0x2000000 // 32MiB
#define VITA_THREAD_STACK_SIZE_DEFAULT 0x10000   // 64KiB
#define VITA_THREAD_NAME_MAX           32

#define VITA_THREAD_PRIORITY_LOW           191
#define VITA_THREAD_PRIORITY_NORMAL        160
#define VITA_THREAD_PRIORITY_HIGH          112
#define VITA_THREAD_PRIORITY_TIME_CRITICAL 64

static int ThreadEntry(SceSize args, void *argp)
{
    SDL_RunThread(*(SDL_Thread **)argp);
    return 0;
}

int SDL_SYS_CreateThread(SDL_Thread *thread)
{
    char thread_name[VITA_THREAD_NAME_MAX];
    size_t stack_size = VITA_THREAD_STACK_SIZE_DEFAULT;

    SDL_strlcpy(thread_name, "SDL thread", VITA_THREAD_NAME_MAX);
    if (thread->name) {
        SDL_strlcpy(thread_name, thread->name, VITA_THREAD_NAME_MAX);
    }

    if (thread->stacksize) {
        if (thread->stacksize < VITA_THREAD_STACK_SIZE_MIN) {
            thread->stacksize = VITA_THREAD_STACK_SIZE_MIN;
        }
        if (thread->stacksize > VITA_THREAD_STACK_SIZE_MAX) {
            thread->stacksize = VITA_THREAD_STACK_SIZE_MAX;
        }
        stack_size = thread->stacksize;
    }

    /* Create new thread with the same priority as the current thread */
    thread->handle = sceKernelCreateThread(
        thread_name, // name
        ThreadEntry, // function to run
        0,           // priority. 0 means priority of calling thread
        stack_size,  // stack size
        0,           // attibutes. always 0
        0,           // cpu affinity mask. 0 = all CPUs
        NULL         // opt. always NULL
    );

    if (thread->handle < 0) {
        return SDL_SetError("sceKernelCreateThread() failed");
    }

    sceKernelStartThread(thread->handle, 4, &thread);
    return 0;
}

void SDL_SYS_SetupThread(const char *name)
{
    /* Do nothing. */
}

SDL_threadID SDL_ThreadID(void)
{
    return (SDL_threadID)sceKernelGetThreadId();
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
    sceKernelWaitThreadEnd(thread->handle, NULL, NULL);
    sceKernelDeleteThread(thread->handle);
}

void SDL_SYS_DetachThread(SDL_Thread *thread)
{
    /* Do nothing. */
}

int SDL_SYS_SetThreadPriority(SDL_ThreadPriority priority)
{
    int value = VITA_THREAD_PRIORITY_NORMAL;

    switch (priority) {
    case SDL_THREAD_PRIORITY_LOW:
        value = VITA_THREAD_PRIORITY_LOW;
        break;
    case SDL_THREAD_PRIORITY_NORMAL:
        value = VITA_THREAD_PRIORITY_NORMAL;
        break;
    case SDL_THREAD_PRIORITY_HIGH:
        value = VITA_THREAD_PRIORITY_HIGH;
        break;
    case SDL_THREAD_PRIORITY_TIME_CRITICAL:
        value = VITA_THREAD_PRIORITY_TIME_CRITICAL;
        break;
    }

    return sceKernelChangeThreadPriority(0, value);
}

#endif /* SDL_THREAD_VITA */

/* vi: set ts=4 sw=4 expandtab: */
