#pragma once

#include "DefaultIOStream.h"

using namespace ::Assimp;

class TestDefaultIOStream : public DefaultIOStream {
public:
    TestDefaultIOStream()
        : DefaultIOStream() {
        // empty
    }

    TestDefaultIOStream( FILE* pFile, const std::string &strFilename )
        : DefaultIOStream( pFile, strFilename ) {
        // empty
    }

    virtual ~TestDefaultIOStream() {
        // empty
    }
};

