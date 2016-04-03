/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2016, assimp team
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
/** @file  FBXBinaryTokenizer.cpp
 *  @brief Implementation of a fake lexer for binary fbx files -
 *    we emit tokens so the parser needs almost no special handling
 *    for binary files.
 */

#ifndef ASSIMP_BUILD_NO_FBX_IMPORTER

#include "FBXTokenizer.h"
#include "FBXUtil.h"
#include "../include/assimp/defs.h"
#include <stdint.h>
#include "Exceptional.h"
#include "ByteSwapper.h"

namespace Assimp {
namespace FBX {


// ------------------------------------------------------------------------------------------------
Token::Token(const char* sbegin, const char* send, TokenType type, unsigned int offset)
    :
    #ifdef DEBUG
    contents(sbegin, static_cast<size_t>(send-sbegin)),
    #endif
    sbegin(sbegin)
    , send(send)
    , type(type)
    , line(offset)
    , column(BINARY_MARKER)
{
    ai_assert(sbegin);
    ai_assert(send);

    // binary tokens may have zero length because they are sometimes dummies
    // inserted by TokenizeBinary()
    ai_assert(send >= sbegin);
}


namespace {

// ------------------------------------------------------------------------------------------------
// signal tokenization error, this is always unrecoverable. Throws DeadlyImportError.
AI_WONT_RETURN void TokenizeError(const std::string& message, unsigned int offset) AI_WONT_RETURN_SUFFIX;
AI_WONT_RETURN void TokenizeError(const std::string& message, unsigned int offset)
{
    throw DeadlyImportError(Util::AddOffset("FBX-Tokenize",message,offset));
}


// ------------------------------------------------------------------------------------------------
uint32_t Offset(const char* begin, const char* cursor)
{
    ai_assert(begin <= cursor);
    return static_cast<unsigned int>(cursor - begin);
}


// ------------------------------------------------------------------------------------------------
void TokenizeError(const std::string& message, const char* begin, const char* cursor)
{
    TokenizeError(message, Offset(begin, cursor));
}


// ------------------------------------------------------------------------------------------------
uint32_t ReadWord(const char* input, const char*& cursor, const char* end)
{
    if(Offset(cursor, end) < 4) {
        TokenizeError("cannot ReadWord, out of bounds",input, cursor);
    }

    uint32_t word = *reinterpret_cast<const uint32_t*>(cursor);
    AI_SWAP4(word);

    cursor += 4;

    return word;
}


// ------------------------------------------------------------------------------------------------
uint8_t ReadByte(const char* input, const char*& cursor, const char* end)
{
    if(Offset(cursor, end) < 1) {
        TokenizeError("cannot ReadByte, out of bounds",input, cursor);
    }

    uint8_t word = *reinterpret_cast<const uint8_t*>(cursor);
    ++cursor;

    return word;
}


// ------------------------------------------------------------------------------------------------
unsigned int ReadString(const char*& sbegin_out, const char*& send_out, const char* input, const char*& cursor, const char* end,
    bool long_length = false,
    bool allow_null = false)
{
    const uint32_t len_len = long_length ? 4 : 1;
    if(Offset(cursor, end) < len_len) {
        TokenizeError("cannot ReadString, out of bounds reading length",input, cursor);
    }

    const uint32_t length = long_length ? ReadWord(input, cursor, end) : ReadByte(input, cursor, end);

    if (Offset(cursor, end) < length) {
        TokenizeError("cannot ReadString, length is out of bounds",input, cursor);
    }

    sbegin_out = cursor;
    cursor += length;

    send_out = cursor;

    if(!allow_null) {
        for (unsigned int i = 0; i < length; ++i) {
            if(sbegin_out[i] == '\0') {
                TokenizeError("failed ReadString, unexpected NUL character in string",input, cursor);
            }
        }
    }

    return length;
}



// ------------------------------------------------------------------------------------------------
void ReadData(const char*& sbegin_out, const char*& send_out, const char* input, const char*& cursor, const char* end)
{
    if(Offset(cursor, end) < 1) {
        TokenizeError("cannot ReadData, out of bounds reading length",input, cursor);
    }

    const char type = *cursor;
    sbegin_out = cursor++;

    switch(type)
    {
        // 16 bit int
    case 'Y':
        cursor += 2;
        break;

        // 1 bit bool flag (yes/no)
    case 'C':
        cursor += 1;
        break;

        // 32 bit int
    case 'I':
        // <- fall through

        // float
    case 'F':
        cursor += 4;
        break;

        // double
    case 'D':
        cursor += 8;
        break;

        // 64 bit int
    case 'L':
        cursor += 8;
        break;

        // note: do not write cursor += ReadWord(...cursor) as this would be UB

        // raw binary data
    case 'R':
    {
        const uint32_t length = ReadWord(input, cursor, end);
        cursor += length;
        break;
    }

    case 'b':
        // TODO: what is the 'b' type code? Right now we just skip over it /
        // take the full range we could get
        cursor = end;
        break;

        // array of *
    case 'f':
    case 'd':
    case 'l':
    case 'i':   {

        const uint32_t length = ReadWord(input, cursor, end);
        const uint32_t encoding = ReadWord(input, cursor, end);

        const uint32_t comp_len = ReadWord(input, cursor, end);

        // compute length based on type and check against the stored value
        if(encoding == 0) {
            uint32_t stride = 0;
            switch(type)
            {
            case 'f':
            case 'i':
                stride = 4;
                break;

            case 'd':
            case 'l':
                stride = 8;
                break;

            default:
                ai_assert(false);
            };
            ai_assert(stride > 0);
            if(length * stride != comp_len) {
                TokenizeError("cannot ReadData, calculated data stride differs from what the file claims",input, cursor);
            }
        }
        // zip/deflate algorithm (encoding==1)? take given length. anything else? die
        else if (encoding != 1) {
            TokenizeError("cannot ReadData, unknown encoding",input, cursor);
        }
        cursor += comp_len;
        break;
    }

        // string
    case 'S': {
        const char* sb, *se;
        // 0 characters can legally happen in such strings
        ReadString(sb, se, input, cursor, end, true, true);
        break;
    }
    default:
        TokenizeError("cannot ReadData, unexpected type code: " + std::string(&type, 1),input, cursor);
    }

    if(cursor > end) {
        TokenizeError("cannot ReadData, the remaining size is too small for the data type: " + std::string(&type, 1),input, cursor);
    }

    // the type code is contained in the returned range
    send_out = cursor;
}


// ------------------------------------------------------------------------------------------------
bool ReadScope(TokenList& output_tokens, const char* input, const char*& cursor, const char* end)
{
    // the first word contains the offset at which this block ends
    const uint32_t end_offset = ReadWord(input, cursor, end);

    // we may get 0 if reading reached the end of the file -
    // fbx files have a mysterious extra footer which I don't know
    // how to extract any information from, but at least it always
    // starts with a 0.
    if(!end_offset) {
        return false;
    }

    if(end_offset > Offset(input, end)) {
        TokenizeError("block offset is out of range",input, cursor);
    }
    else if(end_offset < Offset(input, cursor)) {
        TokenizeError("block offset is negative out of range",input, cursor);
    }

    // the second data word contains the number of properties in the scope
    const uint32_t prop_count = ReadWord(input, cursor, end);

    // the third data word contains the length of the property list
    const uint32_t prop_length = ReadWord(input, cursor, end);

    // now comes the name of the scope/key
    const char* sbeg, *send;
    ReadString(sbeg, send, input, cursor, end);

    output_tokens.push_back(new_Token(sbeg, send, TokenType_KEY, Offset(input, cursor) ));

    // now come the individual properties
    const char* begin_cursor = cursor;
    for (unsigned int i = 0; i < prop_count; ++i) {
        ReadData(sbeg, send, input, cursor, begin_cursor + prop_length);

        output_tokens.push_back(new_Token(sbeg, send, TokenType_DATA, Offset(input, cursor) ));

        if(i != prop_count-1) {
            output_tokens.push_back(new_Token(cursor, cursor + 1, TokenType_COMMA, Offset(input, cursor) ));
        }
    }

    if (Offset(begin_cursor, cursor) != prop_length) {
        TokenizeError("property length not reached, something is wrong",input, cursor);
    }

    // at the end of each nested block, there is a NUL record to indicate
    // that the sub-scope exists (i.e. to distinguish between P: and P : {})
    // this NUL record is 13 bytes long.
#define BLOCK_SENTINEL_LENGTH 13

    if (Offset(input, cursor) < end_offset) {

        if (end_offset - Offset(input, cursor) < BLOCK_SENTINEL_LENGTH) {
            TokenizeError("insufficient padding bytes at block end",input, cursor);
        }

        output_tokens.push_back(new_Token(cursor, cursor + 1, TokenType_OPEN_BRACKET, Offset(input, cursor) ));

        // XXX this is vulnerable to stack overflowing ..
        while(Offset(input, cursor) < end_offset - BLOCK_SENTINEL_LENGTH) {
            ReadScope(output_tokens, input, cursor, input + end_offset - BLOCK_SENTINEL_LENGTH);
        }
        output_tokens.push_back(new_Token(cursor, cursor + 1, TokenType_CLOSE_BRACKET, Offset(input, cursor) ));

        for (unsigned int i = 0; i < BLOCK_SENTINEL_LENGTH; ++i) {
            if(cursor[i] != '\0') {
                TokenizeError("failed to read nested block sentinel, expected all bytes to be 0",input, cursor);
            }
        }
        cursor += BLOCK_SENTINEL_LENGTH;
    }

    if (Offset(input, cursor) != end_offset) {
        TokenizeError("scope length not reached, something is wrong",input, cursor);
    }

    return true;
}


}

// ------------------------------------------------------------------------------------------------
void TokenizeBinary(TokenList& output_tokens, const char* input, unsigned int length)
{
    ai_assert(input);

    if(length < 0x1b) {
        TokenizeError("file is too short",0);
    }

    if (strncmp(input,"Kaydara FBX Binary",18)) {
        TokenizeError("magic bytes not found",0);
    }


    //uint32_t offset = 0x1b;

    const char* cursor = input + 0x1b;

    while (cursor < input + length) {
        if(!ReadScope(output_tokens, input, cursor, input + length)) {
            break;
        }
    }
}

} // !FBX
} // !Assimp

#endif
