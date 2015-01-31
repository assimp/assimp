/*-----------------------------------------------------------------------------------------------
The MIT License (MIT)

Copyright (c) 2014 Kim Kulling

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-----------------------------------------------------------------------------------------------*/
#pragma once
#ifndef OPENDDLPARSER_VALUE_H_INC
#define OPENDDLPARSER_VALUE_H_INC

#include <openddlparser/OpenDDLCommon.h>

BEGIN_ODDLPARSER_NS

class DLL_ODDLPARSER_EXPORT Value {
public:
    enum ValueType {
        ddl_none = -1,
        ddl_bool = 0,
        ddl_int8,
        ddl_int16,
        ddl_int32,
        ddl_int64,
        ddl_unsigned_int8,
        ddl_unsigned_int16,
        ddl_unsigned_int32,
        ddl_unsigned_int64,
        ddl_half,
        ddl_float,
        ddl_double,
        ddl_string,
        ddl_ref,
        ddl_types_max
    };

    Value();
    ~Value();
    void setBool( bool value );
    bool getBool();
    void setInt8( int8 value );
    int8 getInt8();
    void setInt16( int16 value );
    int16  getInt16();
    void setInt32( int32 value );
    int32  getInt32();
    void setInt64( int64 value );
    int64  getInt64();
    void setFloat( float value );
    float getFloat() const;
    void setDouble( double value );
    double getDouble() const;
    void dump();
    void setNext( Value *next );
    Value *getNext() const;

    ValueType m_type;
    size_t m_size;
    unsigned char *m_data;
    Value *m_next;
};

struct DLL_ODDLPARSER_EXPORT ValueAllocator {
    static Value *allocPrimData( Value::ValueType type, size_t len = 1 );
    static void releasePrimData( Value **data );
};

END_ODDLPARSER_NS

#endif // OPENDDLPARSER_VALUE_H_INC
