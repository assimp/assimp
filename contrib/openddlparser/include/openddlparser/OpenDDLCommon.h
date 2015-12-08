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

#include <cstddef>
#include <vector>
#include <string>

#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#  include <inttypes.h>
#endif 

#ifdef _MSC_VER
#   define TAG_DLL_EXPORT __declspec(dllexport)
#   define TAG_DLL_IMPORT __declspec(dllimport )
#   ifdef OPENDDLPARSER_BUILD
#       define DLL_ODDLPARSER_EXPORT TAG_DLL_EXPORT
#   else
#       define DLL_ODDLPARSER_EXPORT TAG_DLL_IMPORT
#   endif // OPENDDLPARSER_BUILD
#   pragma warning( disable : 4251 )
#else
#   define DLL_ODDLPARSER_EXPORT
#endif // _WIN32

// Namespace declarations, override this to avoid any conflicts
#define BEGIN_ODDLPARSER_NS namespace ODDLParser {
#define END_ODDLPARSER_NS   } // namespace ODDLParser
#define USE_ODDLPARSER_NS   using namespace ODDLParser;

BEGIN_ODDLPARSER_NS

// We will use C++11 optional
#ifndef OPENDDL_NO_USE_CPP11
    // All C++11 constructs
#   define ddl_nullptr nullptr
#else
    // Fall-back for older compilers
#   define ddl_nullptr NULL
#endif // OPENDDL_NO_USE_CPP11

// Forward declarations
class DDLNode;
class Value;

struct Name;
struct Identifier;
struct Reference;
struct Property;
struct DataArrayList;

// Platform-specific typedefs
#ifdef _WIN32
typedef signed __int64    int64_impl;
typedef unsigned __int64  uint64_impl;
#else
typedef int64_t           int64_impl;
typedef uint64_t          uint64_impl;
#endif

// OpenDDL-specific data typedefs
typedef signed char       int8;    ///< Signed integer, 1 byte
typedef signed short      int16;   ///< Signed integer, 2 byte
typedef signed int        int32;   ///< Signed integer, 4 byte
typedef int64_impl        int64;   ///< Signed integer, 8 byte
typedef unsigned char     uint8;   ///< Unsigned integer, 1 byte
typedef unsigned short    uint16;  ///< Unsigned integer, 2 byte
typedef unsigned int      uint32;  ///< Unsigned integer, 4 byte
typedef uint64_impl       uint64;  ///< Unsigned integer, 8 byte

///	@brief  Stores a text.
///
/// A text is stored in a simple character buffer. Texts buffer can be 
/// greater than the number of stored characters in them.
struct DLL_ODDLPARSER_EXPORT Text {
    size_t m_capacity;  ///< The capacity of the text.
    size_t m_len;       ///< The length of the text.
    char *m_buffer;     ///< The buffer with the text.

    ///	@brief  The constructor with a given text buffer.
    /// @param  buffer      [in] The buffer.
    /// @param  numChars    [in] The number of characters in the buffer.
    Text( const char *buffer, size_t numChars );

    ///	@brief  The destructor.
    ~Text();

    ///	@brief  Clears the text.
    void clear();

    ///	@brief  Set a new text.
    /// @param  buffer      [in] The buffer.
    /// @param  numChars    [in] The number of characters in the buffer.
    void set( const char *buffer, size_t numChars );

    ///	@brief  The compare operator for std::strings.
    bool operator == ( const std::string &name ) const;

    ///	@brief  The compare operator for Texts.
    bool operator == ( const Text &rhs ) const;

private:
    Text( const Text & );
    Text &operator = ( const Text & );
};

///	@brief  Stores an OpenDDL-specific identifier type.
struct DLL_ODDLPARSER_EXPORT Identifier {
    Text m_text;    ///< The text element.

    ///	@brief  The constructor with a sized buffer full of characters.
    ///	@param  buffer  [in] The identifier buffer.
    ///	@param  len     [in] The length of the buffer
    Identifier( const char buffer[], size_t len );

    ///	@brief  The constructor with a buffer full of characters.
    ///	@param  buffer  [in] The identifier buffer.
    /// @remark Buffer must be null-terminated.
    Identifier( const char buffer[] );

    ///	@brief  The destructor.
    ~Identifier();
    
    ///	@brief  The compare operator.
    bool operator == ( const Identifier &rhs ) const;

private:
    Identifier( const Identifier & );
    Identifier &operator = ( const Identifier & );
};

///	@brief  Description of the type of a name.
enum NameType {
    GlobalName, ///< Name is global.
    LocalName   ///< Name is local.
};

///	@brief  Stores an OpenDDL-specific name
struct DLL_ODDLPARSER_EXPORT Name {
    NameType    m_type; ///< The type of the name ( @see NameType ).
    Identifier *m_id;   ///< The id.

    ///	@brief  The constructor with the type and the id.
    ///	@param  type    [in] The name type.
    ///	@param  id      [in] The id.
    Name( NameType type, Identifier *id );

    ///	@brief  The destructor.
    ~Name();

private:
    Name( const Name & );
    Name &operator = ( const Name& );
};

///	@brief  Stores a bundle of references.
struct DLL_ODDLPARSER_EXPORT Reference {
    size_t   m_numRefs;         ///< The number of stored references.
    Name   **m_referencedName;  ///< The reference names.

    ///	@brief  The default constructor.
    Reference();
     
    ///	@brief  The constructor with an array of ref names.
    /// @param  numrefs     [in] The number of ref names.
    /// @param  names       [in] The ref names.
    Reference( size_t numrefs, Name **names );

    ///	@brief  The destructor.
    ~Reference();

private:
    Reference( const Reference & );
    Reference &operator = ( const Reference & );
};

///	@brief  Stores a property list.
struct DLL_ODDLPARSER_EXPORT Property {
    Identifier *m_key;      ///< The identifier / key of the property.
    Value      *m_value;    ///< The value assigned to its key / id ( ddl_nullptr if none ).
    Reference  *m_ref;      ///< References assigned to its key / id ( ddl_nullptr if none ).
    Property   *m_next;     ///< The next property ( ddl_nullptr if none ).

    ///	@brief  The default constructor.
    Property();

    ///	@brief  The constructor for initialization.
    /// @param  id      [in] The identifier
    Property( Identifier *id );

    ///	@brief  The destructor.
    ~Property();

private:
    Property( const Property & );
    Property &operator = ( const Property & );
};

///	@brief  Stores a data array list.
struct DLL_ODDLPARSER_EXPORT DataArrayList {
    size_t         m_numItems;  ///< The number of items in the list.
    Value         *m_dataList;  ///< The data list ( ee Value ).
    DataArrayList *m_next;      ///< The next data array list ( ddl_nullptr if last ).

    ///	@brief  The default constructor for initialization.
    DataArrayList();

    ///	@brief  The destructor.
    ~DataArrayList();

private:
    DataArrayList( const DataArrayList & ); 
    DataArrayList &operator = ( const DataArrayList & );
};

///	@brief  Stores the context of a parsed OpenDDL declaration.
struct DLL_ODDLPARSER_EXPORT Context {
    DDLNode *m_root;    ///< The root node of the OpenDDL node tree.

    ///	@brief  Constructor for initialization.
    Context();

    ///	@brief  Destructor.
    ~Context();

    ///	@brief  Clears the whole node tree.
    void clear();

private:
    Context( const Context & );
    Context &operator = ( const Context & );
};

END_ODDLPARSER_NS
