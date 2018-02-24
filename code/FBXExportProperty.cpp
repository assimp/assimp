/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2018, assimp team

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
#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_FBX_EXPORTER

#include "FBXExportProperty.h"

#include <assimp/StreamWriter.h> // StreamWriterLE
#include <assimp/Exceptional.h> // DeadlyExportError

#include <string>
#include <vector>
#include <sstream> // stringstream


// constructors for single element properties

FBX::Property::Property(bool v)
    : type('C'), data(1)
{
    data = {uint8_t(v)};
}

FBX::Property::Property(int16_t v) : type('Y'), data(2)
{
    uint8_t* d = data.data();
    (reinterpret_cast<int16_t*>(d))[0] = v;
}

FBX::Property::Property(int32_t v) : type('I'), data(4)
{
    uint8_t* d = data.data();
    (reinterpret_cast<int32_t*>(d))[0] = v;
}

FBX::Property::Property(float v) : type('F'), data(4)
{
    uint8_t* d = data.data();
    (reinterpret_cast<float*>(d))[0] = v;
}

FBX::Property::Property(double v) : type('D'), data(8)
{
    uint8_t* d = data.data();
    (reinterpret_cast<double*>(d))[0] = v;
}

FBX::Property::Property(int64_t v) : type('L'), data(8)
{
    uint8_t* d = data.data();
    (reinterpret_cast<int64_t*>(d))[0] = v;
}


// constructors for array-type properties

FBX::Property::Property(const char* c, bool raw)
    : Property(std::string(c), raw)
{}

// strings can either be saved as "raw" (R) data, or "string" (S) data
FBX::Property::Property(const std::string& s, bool raw)
    : type(raw ? 'R' : 'S'), data(s.size())
{
    for (size_t i = 0; i < s.size(); ++i) {
        data[i] = uint8_t(s[i]);
    }
}

FBX::Property::Property(const std::vector<uint8_t>& r)
    : type('R'), data(r)
{}

FBX::Property::Property(const std::vector<int32_t>& va)
    : type('i'), data(4*va.size())
{
    int32_t* d = reinterpret_cast<int32_t*>(data.data());
    for (size_t i = 0; i < va.size(); ++i) { d[i] = va[i]; }
}

FBX::Property::Property(const std::vector<double>& va)
    : type('d'), data(8*va.size())
{
    double* d = reinterpret_cast<double*>(data.data());
    for (size_t i = 0; i < va.size(); ++i) { d[i] = va[i]; }
}

FBX::Property::Property(const aiMatrix4x4& vm)
    : type('d'), data(8*16)
{
    double* d = reinterpret_cast<double*>(data.data());
    for (size_t c = 0; c < 4; ++c) {
        for (size_t r = 0; r < 4; ++r) {
            d[4*c+r] = vm[r][c];
        }
    }
}

// public member functions

size_t FBX::Property::size()
{
    switch (type) {
    case 'C': case 'Y': case 'I': case 'F': case 'D': case 'L':
        return data.size() + 1;
    case 'S': case 'R':
        return data.size() + 5;
    case 'i': case 'd':
        return data.size() + 13;
    default:
        throw DeadlyExportError("Requested size on property of unknown type");
    }
}

void FBX::Property::Dump(Assimp::StreamWriterLE &s)
{
    s.PutU1(type);
    uint8_t* d;
    size_t N;
    switch (type) {
    case 'C': s.PutU1(*(reinterpret_cast<uint8_t*>(data.data()))); return;
    case 'Y': s.PutI2(*(reinterpret_cast<int16_t*>(data.data()))); return;
    case 'I': s.PutI4(*(reinterpret_cast<int32_t*>(data.data()))); return;
    case 'F': s.PutF4(*(reinterpret_cast<float*>(data.data()))); return;
    case 'D': s.PutF8(*(reinterpret_cast<double*>(data.data()))); return;
    case 'L': s.PutI8(*(reinterpret_cast<int64_t*>(data.data()))); return;
    case 'S':
    case 'R':
        s.PutU4(data.size());
        for (size_t i = 0; i < data.size(); ++i) { s.PutU1(data[i]); }
        return;
    case 'i':
        N = data.size() / 4;
        s.PutU4(N); // number of elements
        s.PutU4(0); // no encoding (1 would be zip-compressed)
        // TODO: compress if large?
        s.PutU4(data.size()); // data size
        d = data.data();
        for (size_t i = 0; i < N; ++i) {
            s.PutI4((reinterpret_cast<int32_t*>(d))[i]);
        }
        return;
    case 'd':
        N = data.size() / 8;
        s.PutU4(N); // number of elements
        s.PutU4(0); // no encoding (1 would be zip-compressed)
        // TODO: compress if large?
        s.PutU4(data.size()); // data size
        d = data.data();
        for (size_t i = 0; i < N; ++i) {
            s.PutF8((reinterpret_cast<double*>(d))[i]);
        }
        return;
    default:
        std::stringstream err;
        err << "Tried to dump property with invalid type '";
        err << type << "'!";
        throw DeadlyExportError(err.str());
    }
}

#endif // ASSIMP_BUILD_NO_FBX_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
