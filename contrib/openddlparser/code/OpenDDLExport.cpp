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
#include <openddlparser/OpenDDLExport.h>
#include <openddlparser/DDLNode.h>

BEGIN_ODDLPARSER_NS

struct DDLNodeIterator {
    const DDLNode::DllNodeList &m_childs;
    size_t m_idx;
    DDLNodeIterator( const DDLNode::DllNodeList &childs ) 
        : m_childs( childs )
        , m_idx( 0 ) {

    }

    bool getNext( DDLNode **node ) {
        if( m_childs.size() > (m_idx+1) ) {
            m_idx++;
            *node = m_childs[ m_idx ];
            return true;
        }

        return false;
    }
};

OpenDDLExport::OpenDDLExport() 
:m_file( nullptr ) {

}

OpenDDLExport::~OpenDDLExport() {
    if( nullptr != m_file ) {
        ::fclose( m_file );
        m_file = nullptr;
    }
}

bool OpenDDLExport::exportContext( Context *ctx, const std::string &filename ) {
    if( filename.empty() ) {
        return false;
    }

    if( ddl_nullptr == ctx ) {
        return false;
    }

    DDLNode *root( ctx->m_root );
    if( nullptr == root ) {
        return true;
    }

    return handleNode( root );
}

bool OpenDDLExport::handleNode( DDLNode *node ) {
    if( ddl_nullptr == node ) {
        return true;
    }

    const DDLNode::DllNodeList &childs = node->getChildNodeList();
    if( childs.empty() ) {
        return true;
    }
    DDLNode *current( ddl_nullptr );
    DDLNodeIterator it( childs );
    bool success( true );
    while( it.getNext( &current ) ) {
        if( ddl_nullptr != current ) {
            success |= writeNode( current );
            if( !handleNode( current ) ) {
                success != false;
            }
        }
    }

    return success;
}

bool OpenDDLExport::writeNode( DDLNode *node ) {
    bool success( true );
    if( node->hasProperties() ) {
        success |= writeProperties( node );
    }

    return true;
}

bool OpenDDLExport::writeProperties( DDLNode *node ) {
    Property *prop( node->getProperties() );

    return true;
}

END_ODDLPARSER_NS
