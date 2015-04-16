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
#ifndef OPENDDLPARSER_DDLNODE_H_INC
#define OPENDDLPARSER_DDLNODE_H_INC

#include <openddlparser/OpenDDLCommon.h>

#include <vector>
#include <string>

BEGIN_ODDLPARSER_NS

class Value;
class OpenDDLParser;

struct Identifier;
struct Reference;
struct Property;
struct DataArrayList;

class DLL_ODDLPARSER_EXPORT DDLNode {
public:
    friend class OpenDDLParser;

    typedef std::vector<DDLNode*> DllNodeList;

public:
    ~DDLNode();
    void attachParent( DDLNode *parent );
    void detachParent();
    DDLNode *getParent() const;
    const DllNodeList &getChildNodeList() const;
    void setType( const std::string &name );
    const std::string &getType() const;
    void setName( const std::string &name );
    const std::string &getName() const;
    void setProperties( Property *prop );
    Property *getProperties() const;
    bool hasProperty( const std::string &name );
    Property *findPropertyByName( const std::string &name );
    void setValue( Value *val );
    Value *getValue() const;
    void setDataArrayList( DataArrayList  *dtArrayList );
    DataArrayList *getDataArrayList() const;
    void setReferences( Reference  *refs );
    Reference *getReferences() const;
    static DDLNode *create( const std::string &type, const std::string &name, DDLNode *parent = ddl_nullptr );

private:
    DDLNode( const std::string &type, const std::string &name, size_t idx, DDLNode *parent = ddl_nullptr );
    DDLNode();
    DDLNode( const DDLNode & );
    DDLNode &operator = ( const DDLNode & );
    static void releaseNodes();

private:
    std::string m_type;
    std::string m_name;
    DDLNode *m_parent;
    std::vector<DDLNode*> m_children;
    Property *m_properties;
    Value *m_value;
    DataArrayList *m_dtArrayList;
    Reference *m_references;
    size_t m_idx;
    static DllNodeList s_allocatedNodes;
};

END_ODDLPARSER_NS

#endif // OPENDDLPARSER_DDLNODE_H_INC
