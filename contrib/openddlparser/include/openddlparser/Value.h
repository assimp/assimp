/*-----------------------------------------------------------------------------------------------
The MIT License (MIT)

Copyright (c) 2014-2015 Kim Kulling

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

#include <openddlparser/OpenDDLCommon.h>

#include <string>

BEGIN_ODDLPARSER_NS

struct ValueAllocator;

///------------------------------------------------------------------------------------------------
///	@brief  This class implements a value.
///
///	Values are used to store data types like boolean, integer, floats, double and many mode. To get
///	an overview please check the enum VylueType ( @see Value::ValueType ).
/// Values can be single items or lists of items. They are implemented as linked lists.
///------------------------------------------------------------------------------------------------
class DLL_ODDLPARSER_EXPORT Value {
    friend struct ValueAllocator;

public:
    ///	@brief  This class implements an iterator through a Value list.
    ///	
    /// When getting a new value you need to know how to iterate through it. The Value::Iterator 
    /// will help you here:
    ///	@code
    /// Value *val = node->getValue();
    /// Value::Iterator it( val );
    /// while( it.hasNext() ) {
    ///     Value v( it.getNext );
    /// }
    /// @endcode
    class DLL_ODDLPARSER_EXPORT Iterator {
    public:
        ///	@brief  The default class constructor.
        Iterator();

        ///	@brief  The class constructor with the start value.
        /// @param  start   [in] The first value for iteration,
        Iterator( Value *start );

        ///	@brief  The class destructor.
        ~Iterator();

        ///	@brief  Will return true, if another value is in the list.
        /// @return true if another value is there.
        bool hasNext() const;

        ///	@brief  Returns the next item and moves the iterator to it.
        ///	@return The next value, is ddl_nullptr in case of being the last item.
        Value *getNext();

    private:
        Value *m_start;
        Value *m_current;

    private:
        Iterator( const Iterator & );
        Iterator &operator = ( const Iterator & );
    };

    ///	@brief  This enum describes the data type stored in the value.
    enum ValueType {
        ddl_none = -1,          ///< Nothing specified
        ddl_bool = 0,           ///< A boolean type
        ddl_int8,               ///< Integer type, 8 bytes
        ddl_int16,              ///< Integer type, 16 bytes
        ddl_int32,              ///< Integer type, 32 bytes
        ddl_int64,              ///< Integer type, 64 bytes
        ddl_unsigned_int8,      ///< Unsigned integer type, 8 bytes
        ddl_unsigned_int16,     ///< Unsigned integer type, 16 bytes
        ddl_unsigned_int32,     ///< Unsigned integer type, 32 bytes
        ddl_unsigned_int64,     ///< Unsigned integer type, 64 bytes
        ddl_half,               
        ddl_float,
        ddl_double,
        ddl_string,
        ddl_ref,
        ddl_types_max
    };

    Value( ValueType type );
    ~Value();
    void setBool( bool value );
    bool getBool();
    void setInt8( int8 value );
    int8 getInt8();
    void setInt16( int16 value );
    int16 getInt16();
    void setInt32( int32 value );
    int32 getInt32();
    void setInt64( int64 value );
    int64 getInt64();
    void setUnsignedInt8( uint8 value );
    uint8 getUnsignedInt8() const;
    void setUnsignedInt16( uint16 value );
    uint16 getUnsignedInt16() const;
    void setUnsignedInt32( uint32 value );
    uint32 getUnsignedInt32() const;
    void setUnsignedInt64( uint64 value );
    uint64 getUnsignedInt64() const;
    void setFloat( float value );
    float getFloat() const;
    void setDouble( double value );
    double getDouble() const;
    void setString( const std::string &str );
    const char *getString() const;
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
