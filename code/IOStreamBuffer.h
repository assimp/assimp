#pragma once

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

#include <assimp/types.h>
#include <assimp/IOStream.hpp>

#include <iostream>

namespace Assimp {

template<class T>
class IOStreamBuffer {
public:
    IOStreamBuffer( size_t cache = 4096 * 4096 );
    ~IOStreamBuffer();
    bool open( IOStream *stream );
    bool close();
    size_t size() const;
    bool readNextBlock();
    bool getNextLine( std::vector<T> &buffer );

private:
    IOStream *m_stream;
    size_t m_filesize;
    size_t m_cacheSize;
    std::vector<T> m_cache;
    size_t m_cachePos;
    size_t m_filePos;
};

template<class T>
inline
IOStreamBuffer<T>::IOStreamBuffer( size_t cache )
    : m_stream( nullptr )
    , m_filesize( 0 )
    , m_cacheSize( cache )
    , m_cachePos( 0 )
    , m_filePos( 0 ) {
    m_cache.resize( cache );
    std::fill( m_cache.begin(), m_cache.end(), '\0' );
}

template<class T>
inline
IOStreamBuffer<T>::~IOStreamBuffer() {
    // empty
}

template<class T>
inline
bool IOStreamBuffer<T>::open( IOStream *stream ) {
    if ( nullptr == stream ) {
        return false;
    }

    m_stream = stream;
    m_filesize = m_stream->FileSize();
    if ( m_filesize == 0 ) {
        return false;
    }
    if ( m_filesize < m_cacheSize ) {
        m_cacheSize = m_filesize;
    }

    return true;
}

template<class T>
inline
bool IOStreamBuffer<T>::close() {
    if ( nullptr == m_stream ) {
        return false;
    }

    m_stream = nullptr;
    m_filesize = 0;

    return true;
}

template<class T>
inline
size_t IOStreamBuffer<T>::size() const {
    return m_filesize;
}

template<class T>
inline
bool IOStreamBuffer<T>::readNextBlock() {
    m_stream->Seek( m_filePos, aiOrigin_SET );
    size_t readLen = m_stream->Read( &m_cache[ 0 ], sizeof( T ), m_cacheSize );
    if ( readLen == 0 ) {
        return false;
    }
    if ( readLen < m_cacheSize ) {
        m_cacheSize = readLen;
    }
    m_filePos += m_cacheSize;
    m_cachePos = 0;

    return true;
}

template<class T>
inline
bool IOStreamBuffer<T>::getNextLine( std::vector<T> &buffer ) {
    buffer.resize( m_cacheSize );
    ::memset( &buffer[ 0 ], '\n', m_cacheSize );

    if ( m_cachePos == m_cacheSize || 0 == m_filePos ) {
        if ( !readNextBlock() ) {
            return false;
        }
    }
    size_t i = 0;
    while ( !IsLineEnd( m_cache[ m_cachePos ] ) ) {
        buffer[ i ] = m_cache[ m_cachePos ];
        m_cachePos++;
        i++;
        if ( m_cachePos >= m_cacheSize ) {
            if ( !readNextBlock() ) {
                return false;
            }
        }
    }
    m_cachePos++;

    return true;
}

} // !ns Assimp
