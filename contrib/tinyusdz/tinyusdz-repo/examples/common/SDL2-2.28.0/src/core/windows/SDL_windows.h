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

/* This is an include file for windows.h with the SDL build settings */

#ifndef _INCLUDED_WINDOWS_H
#define _INCLUDED_WINDOWS_H

#if defined(__WIN32__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef STRICT
#define STRICT 1
#endif
#ifndef UNICODE
#define UNICODE 1
#endif
#undef WINVER
#undef _WIN32_WINNT
#if defined(SDL_VIDEO_RENDER_D3D12)
#define _WIN32_WINNT 0xA00 /* For D3D12, 0xA00 is required */
#elif defined(HAVE_SHELLSCALINGAPI_H)
#define _WIN32_WINNT 0x603 /* For DPI support */
#else
#define _WIN32_WINNT 0x501 /* Need 0x410 for AlphaBlend() and 0x500 for EnumDisplayDevices(), 0x501 for raw input */
#endif
#define WINVER _WIN32_WINNT

#elif defined(__WINGDK__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef STRICT
#define STRICT 1
#endif
#ifndef UNICODE
#define UNICODE 1
#endif
#undef WINVER
#undef _WIN32_WINNT
#define _WIN32_WINNT 0xA00
#define WINVER       _WIN32_WINNT

#elif defined(__XBOXONE__) || defined(__XBOXSERIES__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef STRICT
#define STRICT 1
#endif
#ifndef UNICODE
#define UNICODE 1
#endif
#undef WINVER
#undef _WIN32_WINNT
#define _WIN32_WINNT 0xA00
#define WINVER       _WIN32_WINNT
#endif

#include <windows.h>
#include <basetyps.h> /* for REFIID with broken mingw.org headers */

/* Older Visual C++ headers don't have the Win64-compatible typedefs... */
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
#ifndef DWORD_PTR
#define DWORD_PTR DWORD
#endif
#ifndef LONG_PTR
#define LONG_PTR LONG
#endif
#endif

#include "SDL_rect.h"

/* Routines to convert from UTF8 to native Windows text */
#define WIN_StringToUTF8W(S) SDL_iconv_string("UTF-8", "UTF-16LE", (const char *)(S), (SDL_wcslen(S) + 1) * sizeof(WCHAR))
#define WIN_UTF8ToStringW(S) (WCHAR *)SDL_iconv_string("UTF-16LE", "UTF-8", (const char *)(S), SDL_strlen(S) + 1)
/* !!! FIXME: UTF8ToString() can just be a SDL_strdup() here. */
#define WIN_StringToUTF8A(S) SDL_iconv_string("UTF-8", "ASCII", (const char *)(S), (SDL_strlen(S) + 1))
#define WIN_UTF8ToStringA(S) SDL_iconv_string("ASCII", "UTF-8", (const char *)(S), SDL_strlen(S) + 1)
#if UNICODE
#define WIN_StringToUTF8 WIN_StringToUTF8W
#define WIN_UTF8ToString WIN_UTF8ToStringW
#define SDL_tcslen       SDL_wcslen
#define SDL_tcsstr       SDL_wcsstr
#else
#define WIN_StringToUTF8 WIN_StringToUTF8A
#define WIN_UTF8ToString WIN_UTF8ToStringA
#define SDL_tcslen       SDL_strlen
#define SDL_tcsstr       SDL_strstr
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Sets an error message based on a given HRESULT */
extern int WIN_SetErrorFromHRESULT(const char *prefix, HRESULT hr);

/* Sets an error message based on GetLastError(). Always return -1. */
extern int WIN_SetError(const char *prefix);

#if !defined(__WINRT__)
/* Load a function from combase.dll */
void *WIN_LoadComBaseFunction(const char *name);
#endif

/* Wrap up the oddities of CoInitialize() into a common function. */
extern HRESULT WIN_CoInitialize(void);
extern void WIN_CoUninitialize(void);

/* Wrap up the oddities of RoInitialize() into a common function. */
extern HRESULT WIN_RoInitialize(void);
extern void WIN_RoUninitialize(void);

/* Returns SDL_TRUE if we're running on Windows Vista and newer */
extern BOOL WIN_IsWindowsVistaOrGreater(void);

/* Returns SDL_TRUE if we're running on Windows 7 and newer */
extern BOOL WIN_IsWindows7OrGreater(void);

/* Returns SDL_TRUE if we're running on Windows 8 and newer */
extern BOOL WIN_IsWindows8OrGreater(void);

/* You need to SDL_free() the result of this call. */
extern char *WIN_LookupAudioDeviceName(const WCHAR *name, const GUID *guid);

/* Checks to see if two GUID are the same. */
extern BOOL WIN_IsEqualGUID(const GUID *a, const GUID *b);
extern BOOL WIN_IsEqualIID(REFIID a, REFIID b);

/* Convert between SDL_rect and RECT */
extern void WIN_RECTToRect(const RECT *winrect, SDL_Rect *sdlrect);
extern void WIN_RectToRECT(const SDL_Rect *sdlrect, RECT *winrect);

/* Returns SDL_TRUE if the rect is empty */
extern BOOL WIN_IsRectEmpty(const RECT *rect);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_WINDOWS_H */

/* vi: set ts=4 sw=4 expandtab: */
