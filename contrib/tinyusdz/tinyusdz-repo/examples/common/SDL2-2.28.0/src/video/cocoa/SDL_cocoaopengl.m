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

/* NSOpenGL implementation of SDL OpenGL support */

#if SDL_VIDEO_OPENGL_CGL
#include "SDL_cocoavideo.h"
#include "SDL_cocoaopengl.h"
#include "SDL_cocoaopengles.h"

#include <OpenGL/CGLTypes.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLRenderers.h>

#include "SDL_hints.h"
#include "SDL_loadso.h"
#include "SDL_opengl.h"
#include "../../SDL_hints_c.h"

#define DEFAULT_OPENGL  "/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"

/* We still support OpenGL as long as Apple offers it, deprecated or not, so disable deprecation warnings about it. */
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

/* _Nullable is available starting Xcode 7 */
#ifdef __has_feature
#if __has_feature(nullability)
#define HAS_FEATURE_NULLABLE
#endif
#endif
#ifndef HAS_FEATURE_NULLABLE
#define _Nullable
#endif

static SDL_bool SDL_opengl_async_dispatch = SDL_FALSE;

static void SDLCALL
SDL_OpenGLAsyncDispatchChanged(void *userdata, const char *name, const char *oldValue, const char *hint)
{
    SDL_opengl_async_dispatch = SDL_GetStringBoolean(hint, SDL_FALSE);
}

static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    SDLOpenGLContext *nscontext = (__bridge SDLOpenGLContext *) displayLinkContext;

    /*printf("DISPLAY LINK! %u\n", (unsigned int) SDL_GetTicks()); */
    const int setting = SDL_AtomicGet(&nscontext->swapIntervalSetting);
    if (setting != 0) { /* nothing to do if vsync is disabled, don't even lock */
        SDL_LockMutex(nscontext->swapIntervalMutex);
        SDL_AtomicAdd(&nscontext->swapIntervalsPassed, 1);
        SDL_CondSignal(nscontext->swapIntervalCond);
        SDL_UnlockMutex(nscontext->swapIntervalMutex);
    }

    return kCVReturnSuccess;
}

@implementation SDLOpenGLContext : NSOpenGLContext

- (id)initWithFormat:(NSOpenGLPixelFormat *)format
        shareContext:(NSOpenGLContext *)share
{
    self = [super initWithFormat:format shareContext:share];
    if (self) {
        self.openglPixelFormat = format;
        SDL_AtomicSet(&self->dirty, 0);
        self->window = NULL;
        SDL_AtomicSet(&self->swapIntervalSetting, 0);
        SDL_AtomicSet(&self->swapIntervalsPassed, 0);
        self->swapIntervalCond = SDL_CreateCond();
        self->swapIntervalMutex = SDL_CreateMutex();
        if (!self->swapIntervalCond || !self->swapIntervalMutex) {
            return nil;
        }

        /* !!! FIXME: check return values. */
        CVDisplayLinkCreateWithActiveCGDisplays(&self->displayLink);
        CVDisplayLinkSetOutputCallback(self->displayLink, &DisplayLinkCallback, (__bridge void * _Nullable) self);
        CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(self->displayLink, [self CGLContextObj], [format CGLPixelFormatObj]);
        CVDisplayLinkStart(displayLink);
    }

    SDL_AddHintCallback(SDL_HINT_MAC_OPENGL_ASYNC_DISPATCH, SDL_OpenGLAsyncDispatchChanged, NULL);
    return self;
}

- (void)movedToNewScreen
{
    if (self->displayLink) {
        CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(self->displayLink, [self CGLContextObj], [[self openglPixelFormat] CGLPixelFormatObj]);
    }
}

- (void)scheduleUpdate
{
    SDL_AtomicAdd(&self->dirty, 1);
}

/* This should only be called on the thread on which a user is using the context. */
- (void)updateIfNeeded
{
    const int value = SDL_AtomicSet(&self->dirty, 0);
    if (value > 0) {
        /* We call the real underlying update here, since -[SDLOpenGLContext update] just calls us. */
        [self explicitUpdate];
    }
}

/* This should only be called on the thread on which a user is using the context. */
- (void)update
{
    /* This ensures that regular 'update' calls clear the atomic dirty flag. */
    [self scheduleUpdate];
    [self updateIfNeeded];
}

/* Updates the drawable for the contexts and manages related state. */
- (void)setWindow:(SDL_Window *)newWindow
{
    if (self->window) {
        SDL_WindowData *oldwindowdata = (__bridge SDL_WindowData *)self->window->driverdata;

        /* Make sure to remove us from the old window's context list, or we'll get scheduled updates from it too. */
        NSMutableArray *contexts = oldwindowdata.nscontexts;
        @synchronized (contexts) {
            [contexts removeObject:self];
        }
    }

    self->window = newWindow;

    if (newWindow) {
        SDL_WindowData *windowdata = (__bridge SDL_WindowData *)newWindow->driverdata;
        NSView *contentview = windowdata.sdlContentView;

        /* Now sign up for scheduled updates for the new window. */
        NSMutableArray *contexts = windowdata.nscontexts;
        @synchronized (contexts) {
            [contexts addObject:self];
        }

        if ([self view] != contentview) {
            if ([NSThread isMainThread]) {
                [self setView:contentview];
            } else {
                dispatch_sync(dispatch_get_main_queue(), ^{ [self setView:contentview]; });
            }
            if (self == [NSOpenGLContext currentContext]) {
                [self explicitUpdate];
            } else {
                [self scheduleUpdate];
            }
        }
    } else {
        if ([NSThread isMainThread]) {
            [self setView:nil];
        } else {
            dispatch_sync(dispatch_get_main_queue(), ^{ [self setView:nil]; });
        }
    }
}

- (SDL_Window*)window
{
    return self->window;
}

- (void)explicitUpdate
{
    if ([NSThread isMainThread]) {
        [super update];
    } else {
        if (SDL_opengl_async_dispatch) {
            dispatch_async(dispatch_get_main_queue(), ^{ [super update]; });
        } else {
            dispatch_sync(dispatch_get_main_queue(), ^{ [super update]; });
        }
    }
}

- (void)cleanup
{
    [self setWindow:NULL];

    SDL_DelHintCallback(SDL_HINT_MAC_OPENGL_ASYNC_DISPATCH, SDL_OpenGLAsyncDispatchChanged, NULL);
    if (self->displayLink) {
        CVDisplayLinkRelease(self->displayLink);
        self->displayLink = nil;
    }
    if (self->swapIntervalCond) {
        SDL_DestroyCond(self->swapIntervalCond);
        self->swapIntervalCond = NULL;
    }
    if (self->swapIntervalMutex) {
        SDL_DestroyMutex(self->swapIntervalMutex);
        self->swapIntervalMutex = NULL;
    }
}

@end


int Cocoa_GL_LoadLibrary(_THIS, const char *path)
{
    /* Load the OpenGL library */
    if (path == NULL) {
        path = SDL_getenv("SDL_OPENGL_LIBRARY");
    }
    if (path == NULL) {
        path = DEFAULT_OPENGL;
    }
    _this->gl_config.dll_handle = SDL_LoadObject(path);
    if (!_this->gl_config.dll_handle) {
        return -1;
    }
    SDL_strlcpy(_this->gl_config.driver_path, path,
                SDL_arraysize(_this->gl_config.driver_path));
    return 0;
}

void *Cocoa_GL_GetProcAddress(_THIS, const char *proc)
{
    return SDL_LoadFunction(_this->gl_config.dll_handle, proc);
}

void Cocoa_GL_UnloadLibrary(_THIS)
{
    SDL_UnloadObject(_this->gl_config.dll_handle);
    _this->gl_config.dll_handle = NULL;
}

SDL_GLContext Cocoa_GL_CreateContext(_THIS, SDL_Window * window)
{ @autoreleasepool
{
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    SDL_DisplayData *displaydata = (SDL_DisplayData *)display->driverdata;
    NSOpenGLPixelFormatAttribute attr[32];
    NSOpenGLPixelFormat *fmt;
    SDLOpenGLContext *context;
    SDL_GLContext sdlcontext;
    NSOpenGLContext *share_context = nil;
    int i = 0;
    const char *glversion;
    int glversion_major;
    int glversion_minor;
    NSOpenGLPixelFormatAttribute profile;
    int interval;

    if (_this->gl_config.profile_mask == SDL_GL_CONTEXT_PROFILE_ES) {
#if SDL_VIDEO_OPENGL_EGL
        /* Switch to EGL based functions */
        Cocoa_GL_UnloadLibrary(_this);
        _this->GL_LoadLibrary = Cocoa_GLES_LoadLibrary;
        _this->GL_GetProcAddress = Cocoa_GLES_GetProcAddress;
        _this->GL_UnloadLibrary = Cocoa_GLES_UnloadLibrary;
        _this->GL_CreateContext = Cocoa_GLES_CreateContext;
        _this->GL_MakeCurrent = Cocoa_GLES_MakeCurrent;
        _this->GL_SetSwapInterval = Cocoa_GLES_SetSwapInterval;
        _this->GL_GetSwapInterval = Cocoa_GLES_GetSwapInterval;
        _this->GL_SwapWindow = Cocoa_GLES_SwapWindow;
        _this->GL_DeleteContext = Cocoa_GLES_DeleteContext;

        if (Cocoa_GLES_LoadLibrary(_this, NULL) != 0) {
            return NULL;
        }
        return Cocoa_GLES_CreateContext(_this, window);
#else
        SDL_SetError("SDL not configured with EGL support");
        return NULL;
#endif
    }

    attr[i++] = NSOpenGLPFAAllowOfflineRenderers;

    profile = NSOpenGLProfileVersionLegacy;
    if (_this->gl_config.profile_mask == SDL_GL_CONTEXT_PROFILE_CORE) {
        profile = NSOpenGLProfileVersion3_2Core;
    }
    attr[i++] = NSOpenGLPFAOpenGLProfile;
    attr[i++] = profile;

    attr[i++] = NSOpenGLPFAColorSize;
    attr[i++] = SDL_BYTESPERPIXEL(display->current_mode.format)*8;

    attr[i++] = NSOpenGLPFADepthSize;
    attr[i++] = _this->gl_config.depth_size;

    if (_this->gl_config.double_buffer) {
        attr[i++] = NSOpenGLPFADoubleBuffer;
    }

    if (_this->gl_config.stereo) {
        attr[i++] = NSOpenGLPFAStereo;
    }

    if (_this->gl_config.stencil_size) {
        attr[i++] = NSOpenGLPFAStencilSize;
        attr[i++] = _this->gl_config.stencil_size;
    }

    if ((_this->gl_config.accum_red_size +
         _this->gl_config.accum_green_size +
         _this->gl_config.accum_blue_size +
         _this->gl_config.accum_alpha_size) > 0) {
        attr[i++] = NSOpenGLPFAAccumSize;
        attr[i++] = _this->gl_config.accum_red_size + _this->gl_config.accum_green_size + _this->gl_config.accum_blue_size + _this->gl_config.accum_alpha_size;
    }

    if (_this->gl_config.multisamplebuffers) {
        attr[i++] = NSOpenGLPFASampleBuffers;
        attr[i++] = _this->gl_config.multisamplebuffers;
    }

    if (_this->gl_config.multisamplesamples) {
        attr[i++] = NSOpenGLPFASamples;
        attr[i++] = _this->gl_config.multisamplesamples;
        attr[i++] = NSOpenGLPFANoRecovery;
    }
    if (_this->gl_config.floatbuffers) {
        attr[i++] = NSOpenGLPFAColorFloat;
    }

    if (_this->gl_config.accelerated >= 0) {
        if (_this->gl_config.accelerated) {
            attr[i++] = NSOpenGLPFAAccelerated;
        } else {
            attr[i++] = NSOpenGLPFARendererID;
            attr[i++] = kCGLRendererGenericFloatID;
        }
    }

    attr[i++] = NSOpenGLPFAScreenMask;
    attr[i++] = CGDisplayIDToOpenGLDisplayMask(displaydata->display);
    attr[i] = 0;

    fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attr];
    if (fmt == nil) {
        SDL_SetError("Failed creating OpenGL pixel format");
        return NULL;
    }

    if (_this->gl_config.share_with_current_context) {
        share_context = (__bridge NSOpenGLContext*)SDL_GL_GetCurrentContext();
    }

    context = [[SDLOpenGLContext alloc] initWithFormat:fmt shareContext:share_context];

    if (context == nil) {
        SDL_SetError("Failed creating OpenGL context");
        return NULL;
    }

    sdlcontext = (SDL_GLContext)CFBridgingRetain(context);

    /* vsync is handled separately by synchronizing with a display link. */
    interval = 0;
    [context setValues:&interval forParameter:NSOpenGLCPSwapInterval];

    if (Cocoa_GL_MakeCurrent(_this, window, sdlcontext) < 0) {
        SDL_GL_DeleteContext(sdlcontext);
        SDL_SetError("Failed making OpenGL context current");
        return NULL;
    }

    if (_this->gl_config.major_version < 3 &&
        _this->gl_config.profile_mask == 0 &&
        _this->gl_config.flags == 0) {
        /* This is a legacy profile, so to match other backends, we're done. */
    } else {
        const GLubyte *(APIENTRY * glGetStringFunc)(GLenum);

        glGetStringFunc = (const GLubyte *(APIENTRY *)(GLenum)) SDL_GL_GetProcAddress("glGetString");
        if (!glGetStringFunc) {
            SDL_GL_DeleteContext(sdlcontext);
            SDL_SetError ("Failed getting OpenGL glGetString entry point");
            return NULL;
        }

        glversion = (const char *)glGetStringFunc(GL_VERSION);
        if (glversion == NULL) {
            SDL_GL_DeleteContext(sdlcontext);
            SDL_SetError ("Failed getting OpenGL context version");
            return NULL;
        }

        if (SDL_sscanf(glversion, "%d.%d", &glversion_major, &glversion_minor) != 2) {
            SDL_GL_DeleteContext(sdlcontext);
            SDL_SetError ("Failed parsing OpenGL context version");
            return NULL;
        }

        if ((glversion_major < _this->gl_config.major_version) ||
           ((glversion_major == _this->gl_config.major_version) && (glversion_minor < _this->gl_config.minor_version))) {
            SDL_GL_DeleteContext(sdlcontext);
            SDL_SetError ("Failed creating OpenGL context at version requested");
            return NULL;
        }

        /* In the future we'll want to do this, but to match other platforms
           we'll leave the OpenGL version the way it is for now
         */
        /*_this->gl_config.major_version = glversion_major;*/
        /*_this->gl_config.minor_version = glversion_minor;*/
    }
    return sdlcontext;
}}

int Cocoa_GL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{ @autoreleasepool
{
    if (context) {
        SDLOpenGLContext *nscontext = (__bridge SDLOpenGLContext *)context;
        if ([nscontext window] != window) {
            [nscontext setWindow:window];
            [nscontext updateIfNeeded];
        }
        [nscontext makeCurrentContext];
    } else {
        [NSOpenGLContext clearCurrentContext];
    }

    return 0;
}}

int Cocoa_GL_SetSwapInterval(_THIS, int interval)
{ @autoreleasepool
{
    SDLOpenGLContext *nscontext = (__bridge SDLOpenGLContext *) SDL_GL_GetCurrentContext();
    int status;

    if (nscontext == nil) {
        status = SDL_SetError("No current OpenGL context");
    } else {
        SDL_LockMutex(nscontext->swapIntervalMutex);
        SDL_AtomicSet(&nscontext->swapIntervalsPassed, 0);
        SDL_AtomicSet(&nscontext->swapIntervalSetting, interval);
        SDL_UnlockMutex(nscontext->swapIntervalMutex);
        status = 0;
    }

    return status;
}}

int Cocoa_GL_GetSwapInterval(_THIS)
{ @autoreleasepool
{
    SDLOpenGLContext* nscontext = (__bridge SDLOpenGLContext*)SDL_GL_GetCurrentContext();
    return nscontext ? SDL_AtomicGet(&nscontext->swapIntervalSetting) : 0;
}}

int Cocoa_GL_SwapWindow(_THIS, SDL_Window * window)
{ @autoreleasepool
{
    SDLOpenGLContext* nscontext = (__bridge SDLOpenGLContext*)SDL_GL_GetCurrentContext();
    SDL_VideoData *videodata = (__bridge SDL_VideoData *) _this->driverdata;
    const int setting = SDL_AtomicGet(&nscontext->swapIntervalSetting);

    if (setting == 0) {
        /* nothing to do if vsync is disabled, don't even lock */
    } else if (setting < 0) {  /* late swap tearing */
        SDL_LockMutex(nscontext->swapIntervalMutex);
        while (SDL_AtomicGet(&nscontext->swapIntervalsPassed) == 0) {
            SDL_CondWait(nscontext->swapIntervalCond, nscontext->swapIntervalMutex);
        }
        SDL_AtomicSet(&nscontext->swapIntervalsPassed, 0);
        SDL_UnlockMutex(nscontext->swapIntervalMutex);
    } else {
        SDL_LockMutex(nscontext->swapIntervalMutex);
        do {  /* always wait here so we know we just hit a swap interval. */
            SDL_CondWait(nscontext->swapIntervalCond, nscontext->swapIntervalMutex);
        } while ((SDL_AtomicGet(&nscontext->swapIntervalsPassed) % setting) != 0);
        SDL_AtomicSet(&nscontext->swapIntervalsPassed, 0);
        SDL_UnlockMutex(nscontext->swapIntervalMutex);
    }

    /*{ static Uint64 prev = 0; const Uint64 now = SDL_GetTicks64(); const unsigned int diff = (unsigned int) (now - prev); prev = now; printf("GLSWAPBUFFERS TICKS %u\n", diff); }*/

    /* on 10.14 ("Mojave") and later, this deadlocks if two contexts in two
       threads try to swap at the same time, so put a mutex around it. */
    SDL_LockMutex(videodata.swaplock);
    [nscontext flushBuffer];
    [nscontext updateIfNeeded];
    SDL_UnlockMutex(videodata.swaplock);
    return 0;
}}

void Cocoa_GL_DeleteContext(_THIS, SDL_GLContext context)
{ @autoreleasepool
{
    SDLOpenGLContext *nscontext = (__bridge SDLOpenGLContext *)context;
    [nscontext cleanup];
    CFRelease(context);
}}

/* We still support OpenGL as long as Apple offers it, deprecated or not, so disable deprecation warnings about it. */
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif /* SDL_VIDEO_OPENGL_CGL */

/* vi: set ts=4 sw=4 expandtab: */
