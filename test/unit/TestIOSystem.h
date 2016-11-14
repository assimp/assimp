#pragma once

#include "UnitTestPCH.h"

#include <assimp/IOSystem.hpp>

using namespace std;
using namespace Assimp;

static const string Sep = "/";
class TestIOSystem : public IOSystem {
public:
    TestIOSystem() : IOSystem() {}
    virtual ~TestIOSystem() {}
    virtual bool Exists( const char* ) const {
        return true;
    }
    virtual char getOsSeparator() const {
        return Sep[ 0 ];
    }

    virtual IOStream* Open( const char* pFile, const char* pMode = "rb" ) {
        return NULL;
    }

    virtual void Close( IOStream* pFile ) {
        // empty
    }
};
