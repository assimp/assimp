/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
copyright notice, this list of conditions and the
following disclaimer.

* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other
materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
contributors may be used to endorse or promote products
derived from this software without specific prior
written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/
#pragma once

#if _MSC_VER // "unreferenced function has been removed" (SSE2 detection routine in x64 builds)
#pragma warning(push)
#pragma warning(disable : 4505)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#ifndef STB_USE_HUNTER
/*  Use prefixed names for the symbols from stb_image as it is a very commonly embedded library.
    Including vanilla stb_image symbols causes duplicate symbol problems if assimp is linked
    statically together with another library or executable that also embeds stb_image.

    Symbols are not prefixed if using Hunter because in such case there exists a single true
    stb_image library on the system that is used by assimp and can be used by all the other
    libraries and executables.

    The list can be regenerated using the following:

    cat "path/to/stb/stb_image.h" | fgrep STBIDEF | fgrep '(' | sed -E 's/\*|\(.+//g' | \
        awk '{print "#define " $(NF) " assimp_" $(NF) }' | sort | uniq
*/
#define stbi_convert_iphone_png_to_rgb assimp_stbi_convert_iphone_png_to_rgb
#define stbi_convert_iphone_png_to_rgb_thread assimp_stbi_convert_iphone_png_to_rgb_thread
#define stbi_convert_wchar_to_utf8 assimp_stbi_convert_wchar_to_utf8
#define stbi_failure_reason assimp_stbi_failure_reason
#define stbi_hdr_to_ldr_gamma assimp_stbi_hdr_to_ldr_gamma
#define stbi_hdr_to_ldr_scale assimp_stbi_hdr_to_ldr_scale
#define stbi_image_free assimp_stbi_image_free
#define stbi_info assimp_stbi_info
#define stbi_info_from_callbacks assimp_stbi_info_from_callbacks
#define stbi_info_from_file assimp_stbi_info_from_file
#define stbi_info_from_memory assimp_stbi_info_from_memory
#define stbi_is_16_bit assimp_stbi_is_16_bit
#define stbi_is_16_bit_from_callbacks assimp_stbi_is_16_bit_from_callbacks
#define stbi_is_16_bit_from_file assimp_stbi_is_16_bit_from_file
#define stbi_is_16_bit_from_memory assimp_stbi_is_16_bit_from_memory
#define stbi_is_hdr assimp_stbi_is_hdr
#define stbi_is_hdr_from_callbacks assimp_stbi_is_hdr_from_callbacks
#define stbi_is_hdr_from_file assimp_stbi_is_hdr_from_file
#define stbi_is_hdr_from_memory assimp_stbi_is_hdr_from_memory
#define stbi_ldr_to_hdr_gamma assimp_stbi_ldr_to_hdr_gamma
#define stbi_ldr_to_hdr_scale assimp_stbi_ldr_to_hdr_scale
#define stbi_load assimp_stbi_load
#define stbi_load_16 assimp_stbi_load_16
#define stbi_load_16_from_callbacks assimp_stbi_load_16_from_callbacks
#define stbi_load_16_from_memory assimp_stbi_load_16_from_memory
#define stbi_load_from_callbacks assimp_stbi_load_from_callbacks
#define stbi_load_from_file assimp_stbi_load_from_file
#define stbi_load_from_file_16 assimp_stbi_load_from_file_16
#define stbi_load_from_memory assimp_stbi_load_from_memory
#define stbi_load_gif_from_memory assimp_stbi_load_gif_from_memory
#define stbi_loadf assimp_stbi_loadf
#define stbi_loadf_from_callbacks assimp_stbi_loadf_from_callbacks
#define stbi_loadf_from_file assimp_stbi_loadf_from_file
#define stbi_loadf_from_memory assimp_stbi_loadf_from_memory
#define stbi_set_flip_vertically_on_load assimp_stbi_set_flip_vertically_on_load
#define stbi_set_flip_vertically_on_load_thread assimp_stbi_set_flip_vertically_on_load_thread
#define stbi_set_unpremultiply_on_load assimp_stbi_set_unpremultiply_on_load
#define stbi_set_unpremultiply_on_load_thread assimp_stbi_set_unpremultiply_on_load_thread
#define stbi_zlib_decode_buffer assimp_stbi_zlib_decode_buffer
#define stbi_zlib_decode_malloc assimp_stbi_zlib_decode_malloc
#define stbi_zlib_decode_malloc_guesssize assimp_stbi_zlib_decode_malloc_guesssize
#define stbi_zlib_decode_malloc_guesssize_headerflag assimp_stbi_zlib_decode_malloc_guesssize_headerflag
#define stbi_zlib_decode_noheader_buffer assimp_stbi_zlib_decode_noheader_buffer
#define stbi_zlib_decode_noheader_malloc assimp_stbi_zlib_decode_noheader_malloc
#endif

#include "stb/stb_image.h"

#if _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
