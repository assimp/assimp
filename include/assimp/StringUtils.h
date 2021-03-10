/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2021, assimp team


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
#ifndef INCLUDED_AI_STRINGUTILS_H
#define INCLUDED_AI_STRINGUTILS_H

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/defs.h>

#include <cstdarg>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <locale>
#include <sstream>

#ifdef _MSC_VER
#define AI_SIZEFMT "%Iu"
#else
#define AI_SIZEFMT "%zu"
#endif

// ---------------------------------------------------------------------------------
///	@fn		ai_snprintf
///	@brief	The portable version of the function snprintf ( C99 standard ), which
///         works on visual studio compilers 2013 and earlier.
///	@param	outBuf		The buffer to write in
///	@param	size		The buffer size
///	@param	format		The format string
///	@param	ap			The additional arguments.
///	@return	The number of written characters if the buffer size was big enough.
///         If an encoding error occurs, a negative number is returned.
// ---------------------------------------------------------------------------------
#if defined(_MSC_VER) && _MSC_VER < 1900

inline int c99_ai_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap) {
    int count(-1);
    if (0 != size) {
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    }
    if (count == -1) {
        count = _vscprintf(format, ap);
    }

    return count;
}

inline int ai_snprintf(char *outBuf, size_t size, const char *format, ...) {
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_ai_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}

#elif defined(__MINGW32__)
#define ai_snprintf __mingw_snprintf
#else
#define ai_snprintf snprintf
#endif

// ---------------------------------------------------------------------------------
///	@fn		to_string
///	@brief	The portable version of to_string ( some gcc-versions on embedded
///         devices are not supporting this).
///	@param	value   The value to write into the std::string.
///	@return	The value as a std::string
// ---------------------------------------------------------------------------------
template <typename T>
AI_FORCE_INLINE std::string ai_to_string(T value) {
    std::ostringstream os;
    os << value;

    return os.str();
}

// ---------------------------------------------------------------------------------
///	@fn		ai_strtof
///	@brief	The portable version of strtof.
///	@param	begin   The first character of the string.
/// @param  end     The last character
///	@return	The float value, 0.0f in case of an error.
// ---------------------------------------------------------------------------------
AI_FORCE_INLINE
float ai_strtof(const char *begin, const char *end) {
    if (nullptr == begin) {
        return 0.0f;
    }
    float val(0.0f);
    if (nullptr == end) {
        val = static_cast<float>(::atof(begin));
    } else {
        std::string::size_type len(end - begin);
        std::string token(begin, len);
        val = static_cast<float>(::atof(token.c_str()));
    }

    return val;
}

// ---------------------------------------------------------------------------------
///	@fn		DecimalToHexa
///	@brief	The portable to convert a decimal value into a hexadecimal string.
///	@param	toConvert   Value to convert
///	@return	The hexadecimal string, is empty in case of an error.
// ---------------------------------------------------------------------------------
template <class T>
AI_FORCE_INLINE std::string ai_decimal_to_hexa(T toConvert) {
    std::string result;
    std::stringstream ss;
    ss << std::hex << toConvert;
    ss >> result;

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = (char)toupper(result[i]);
    }

    return result;
}

// ---------------------------------------------------------------------------------
///	@brief	translate RGBA to String
///	@param	r   aiColor.r
///	@param	g   aiColor.g
///	@param	b   aiColor.b
///	@param	a   aiColor.a
///	@param	with_head   #
///	@return	The hexadecimal string, is empty in case of an error.
// ---------------------------------------------------------------------------------
AI_FORCE_INLINE std::string ai_rgba2hex(int r, int g, int b, int a, bool with_head) {
    std::stringstream ss;
    if (with_head) {
        ss << "#";
    }
    ss << std::hex << (r << 24 | g << 16 | b << 8 | a);

    return ss.str();
}

// ---------------------------------------------------------------------------------
/// @brief   Performs a trim from start (in place)
/// @param  s   string to trim.
AI_FORCE_INLINE void ai_trim_left(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// ---------------------------------------------------------------------------------
/// @brief  Performs a trim from end (in place).
/// @param  s   string to trim.
// ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------
AI_FORCE_INLINE void ai_trim_right(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// ---------------------------------------------------------------------------------
/// @brief  Performs a trim from both ends (in place).
/// @param  s   string to trim.
// ---------------------------------------------------------------------------------
AI_FORCE_INLINE std::string ai_trim(std::string &s) {
    std::string out(s);
    ai_trim_left(out);
    ai_trim_right(out);

    return out;
}

// ---------------------------------------------------------------------------------
template <class char_t>
AI_FORCE_INLINE char_t ai_tolower(char_t in) {
    return (in >= (char_t)'A' && in <= (char_t)'Z') ? (char_t)(in + 0x20) : in;
}

// ---------------------------------------------------------------------------------
/// @brief  Performs a ToLower-operation and return the lower-case string.
/// @param  in  The incoming string.
/// @return The string as lowercase.
// ---------------------------------------------------------------------------------
AI_FORCE_INLINE std::string ai_str_tolower(const std::string &in) {
    std::string out(in);
    ai_trim_left(out);
    ai_trim_right(out);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return ai_tolower(c); });
    return out;
}

// ---------------------------------------------------------------------------------
template <class char_t>
AI_FORCE_INLINE char_t ai_toupper(char_t in) {
    return (in >= (char_t)'a' && in <= (char_t)'z') ? (char_t)(in - 0x20) : in;
}

// ---------------------------------------------------------------------------------
/// @brief  Performs a ToLower-operation and return the upper-case string.
/// @param  in  The incoming string.
/// @return The string as uppercase.
AI_FORCE_INLINE std::string ai_str_toupper(const std::string &in) {
    std::string out(in);
    std::transform(out.begin(), out.end(), out.begin(), [](char c) { return ai_toupper(c); });
    return out;
}

#endif
