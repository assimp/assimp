/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2014, assimp team
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
#ifndef ASSIMP_BUILD_NO_OPENGEX_IMPORTER

#include "OpenGEXImporter.h"
#include "DefaultIOSystem.h"
#include "MakeVerboseFormat.h"

#include <openddlparser/OpenDDLParser.h>
#include <assimp/scene.h>
#include <assimp/ai_assert.h>

#include <vector>

static const aiImporterDesc desc = {
    "Open Game Engine Exchange",
    "",
    "",
    "",
    aiImporterFlags_SupportTextFlavour,
    0,
    0,
    0,
    0,
    "ogex"
};

namespace Grammar {
    static const std::string MetricType = "Metric";
    static const std::string Metric_DistanceType = "distance";
    static const std::string Metric_AngleType = "angle";
    static const std::string Metric_TimeType = "time";
    static const std::string Metric_UpType = "up";
    static const std::string NameType = "Name";
    static const std::string ObjectRefType = "ObjectRef";
    static const std::string MaterialRefType = "MaterialRef";
    static const std::string MetricKeyType = "key";
    static const std::string GeometryNodeType = "GeometryNode";
    static const std::string GeometryObjectType = "GeometryObject";
    static const std::string TransformType = "Transform";
    static const std::string MeshType = "Mesh";
    static const std::string VertexArrayType = "VertexArray";
    static const std::string IndexArrayType = "IndexArray";
    static const std::string MaterialType = "Material";
    static const std::string ColorType = "Color";
    static const std::string DiffuseColorToken = "diffuse";
    static const std::string SpecularColorToken = "specular";
    static const std::string EmissionColorToken = "emission";

    static const std::string DiffuseTextureToken = "diffuse";
    static const std::string DiffuseSpecularTextureToken = "specular";
    static const std::string SpecularPowerTextureToken = "specular_power";
    static const std::string EmissionTextureToken = "emission";
    static const std::string OpacyTextureToken = "opacity";
    static const std::string TransparencyTextureToken = "transparency";
    static const std::string NormalTextureToken = "normal";

    static const char *TextureType         = "Texture";

    enum TokenType {
        NoneType = -1,
        MetricToken,
        NameToken,
        ObjectRefToken,
        MaterialRefToken,
        MetricKeyToken,
        GeometryNodeToken,
        GeometryObjectToken,
        TransformToken,
        MeshToken,
        VertexArrayToken,
        IndexArrayToken,
        MaterialToken,
        ColorToken,
        TextureToken
    };

    static const std::string ValidMetricToken[ 4 ] = {
        Metric_DistanceType,
        Metric_AngleType,
        Metric_TimeType,
        Metric_UpType
    };

    static int isValidMetricType( const char *token ) {
        if( NULL == token ) {
            return false;
        }

        int idx( -1 );
        for( size_t i = 0; i < 4; i++ ) {
            if( ValidMetricToken[ i ] == token ) {
                idx = (int) i;
                break;
            }
        }

        return idx;
    }

    static TokenType matchTokenType( const char *tokenType ) {
        if( MetricType == tokenType ) {
            return MetricToken;
        } else if(  NameType == tokenType ) {
            return NameToken;
        } else if( ObjectRefType == tokenType ) {
            return ObjectRefToken;
        } else if( MaterialRefType == tokenType ) {
            return MaterialRefToken;
        } else if( MetricKeyType == tokenType ) {
            return MetricKeyToken;
        } else if( GeometryNodeType == tokenType ) {
            return GeometryNodeToken;
        } else if( GeometryObjectType == tokenType ) {
            return GeometryObjectToken;
        } else if( TransformType == tokenType ) {
            return TransformToken;
        } else if( MeshType == tokenType ) {
            return MeshToken;
        } else if( VertexArrayType == tokenType ) {
            return VertexArrayToken;
        } else if( IndexArrayType == tokenType ) {
            return IndexArrayToken;
        } else if(  MaterialType == tokenType ) {
            return MaterialToken;
        } else if( ColorType == tokenType ) {
            return ColorToken;
        } else if(  TextureType == tokenType ) {
            return TextureToken;
        }

        return NoneType;
    }

} // Namespace Grammar

namespace Assimp {
namespace OpenGEX {

USE_ODDLPARSER_NS

//------------------------------------------------------------------------------------------------
OpenGEXImporter::VertexContainer::VertexContainer()
: m_numVerts( 0 )
, m_vertices(NULL)
, m_numNormals( 0 )
, m_normals(NULL)
, m_numUVComps()
, m_textureCoords() {
}

//------------------------------------------------------------------------------------------------
OpenGEXImporter::VertexContainer::~VertexContainer() {
    delete[] m_vertices;
    delete[] m_normals;
    for( size_t i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; i++ ) {
        delete [] m_textureCoords[ i ];
    }
}

//------------------------------------------------------------------------------------------------
OpenGEXImporter::RefInfo::RefInfo( aiNode *node, Type type, std::vector<std::string> &names )
: m_node( node )
, m_type( type )
, m_Names( names ) {
    // empty
}

//------------------------------------------------------------------------------------------------
OpenGEXImporter::RefInfo::~RefInfo() {
    // empty
}

//------------------------------------------------------------------------------------------------
OpenGEXImporter::OpenGEXImporter()
: m_root( NULL )
, m_nodeChildMap()
, m_meshCache()
, m_mesh2refMap()
, m_ctx( NULL )
, m_metrics()
, m_currentNode( NULL )
, m_currentMesh( NULL )
, m_currentMaterial( NULL )
, m_tokenType( Grammar::NoneType )
, m_nodeStack()
, m_unresolvedRefStack() {
}

//------------------------------------------------------------------------------------------------
OpenGEXImporter::~OpenGEXImporter() {
    m_ctx = NULL;
}

//------------------------------------------------------------------------------------------------
bool OpenGEXImporter::CanRead( const std::string &file, IOSystem *pIOHandler, bool checkSig ) const {
    bool canRead( false );
    if( !checkSig ) {
        canRead = SimpleExtensionCheck( file, "ogex" );
    } else {
        static const char *token[] = { "Metric", "GeometryNode", "VertexArray (attrib", "IndexArray" };
        canRead = BaseImporter::SearchFileHeaderForToken( pIOHandler, file, token, 4 );
    }

    return canRead;
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::InternReadFile( const std::string &filename, aiScene *pScene, IOSystem *pIOHandler ) {
    // open source file
    IOStream *file = pIOHandler->Open( filename, "rb" );
    if( !file ) {
        throw DeadlyImportError( "Failed to open file " + filename );
    }

    std::vector<char> buffer;
    TextFileToBuffer( file, buffer );

    OpenDDLParser myParser;
    myParser.setBuffer( &buffer[ 0 ], buffer.size() );
    bool success( myParser.parse() );
    if( success ) {
        m_ctx = myParser.getContext();
        pScene->mRootNode = new aiNode;
        pScene->mRootNode->mName.Set( filename );
        handleNodes( m_ctx->m_root, pScene );
    }

    copyMeshes( pScene );
    resolveReferences();
    createNodeTree( pScene );
}

//------------------------------------------------------------------------------------------------
const aiImporterDesc *OpenGEXImporter::GetInfo() const {
    return &desc;
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::SetupProperties( const Importer *pImp ) {
    if( NULL == pImp ) {
        return;
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleNodes( DDLNode *node, aiScene *pScene ) {
    if( NULL == node ) {
        return;
    }

    DDLNode::DllNodeList childs = node->getChildNodeList();
    for( DDLNode::DllNodeList::iterator it = childs.begin(); it != childs.end(); ++it ) {
        Grammar::TokenType tokenType( Grammar::matchTokenType( ( *it )->getType().c_str() ) );
        switch( tokenType ) {
            case Grammar::MetricToken:
                handleMetricNode( *it, pScene );
                break;

            case Grammar::NameToken:
                handleNameNode( *it, pScene );
                break;

            case Grammar::ObjectRefToken:
                handleObjectRefNode( *it, pScene );
                break;

            case Grammar::MaterialRefToken:
                handleMaterialRefNode( *it, pScene );
                break;

            case Grammar::MetricKeyToken:
                break;

            case Grammar::GeometryNodeToken:
                handleGeometryNode( *it, pScene );
                break;

            case Grammar::GeometryObjectToken:
                handleGeometryObject( *it, pScene );
                break;

            case Grammar::TransformToken:
                handleTransformNode( *it, pScene );
                break;

            case Grammar::MeshToken:
                handleMeshNode( *it, pScene );
                break;

            case Grammar::VertexArrayToken:
                handleVertexArrayNode( *it, pScene );
                break;

            case Grammar::IndexArrayToken:
                handleIndexArrayNode( *it, pScene );
                break;

            case Grammar::MaterialToken:
                handleMaterialNode( *it, pScene );
                break;

            case Grammar::ColorToken:
                handleColorNode( *it, pScene );
                break;

            case Grammar::TextureToken:
                handleTextureNode( *it, pScene );
                break;

            default:
                break;
        }
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleMetricNode( DDLNode *node, aiScene *pScene ) {
    if( NULL == node || NULL == m_ctx ) {
        return;
    }

    if( m_ctx->m_root != node->getParent() ) {
        return;
    }

    Property *prop( node->getProperties() );
    while( NULL != prop ) {
        if( NULL != prop->m_key ) {
            if( Value::ddl_string == prop->m_value->m_type ) {
                std::string valName( ( char* ) prop->m_value->m_data );
                int type( Grammar::isValidMetricType( valName.c_str() ) );
                if( Grammar::NoneType != type ) {
                    Value *val( node->getValue() );
                    if( NULL != val ) {
                        if( Value::ddl_float == val->m_type ) {
                            m_metrics[ type ].m_floatValue = val->getFloat();
                        } else if( Value::ddl_int32 == val->m_type ) {
                            m_metrics[ type ].m_intValue = val->getInt32();
                        } else if( Value::ddl_string == val->m_type ) {
                            m_metrics[type].m_stringValue = std::string( val->getString() );
                        } else {
                            throw DeadlyImportError( "OpenGEX: invalid data type for Metric node." );
                        }
                    }
                }
            }
        }
        prop = prop->m_next;
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleNameNode( DDLNode *node, aiScene *pScene ) {
    if( NULL == m_currentNode ) {
        throw DeadlyImportError( "No parent node for name." );
        return;
    }

    Value *val( node->getValue() );
    if( NULL != val ) {
        if( Value::ddl_string != val->m_type ) {
            throw DeadlyImportError( "OpenGEX: invalid data type for value in node name." );
            return;
        }

        const std::string name( val->getString() );
        if( m_tokenType == Grammar::GeometryNodeToken ) {
            m_currentNode->mName.Set( name.c_str() );
        } else if( m_tokenType == Grammar::MaterialToken ) {

        }

    }
}

//------------------------------------------------------------------------------------------------
static void getRefNames( DDLNode *node, std::vector<std::string> &names ) {
    ai_assert( NULL != node );

    Reference *ref = node->getReferences();
    if( NULL != ref ) {
        for( size_t i = 0; i < ref->m_numRefs; i++ )  {
            Name *currentName( ref->m_referencedName[ i ] );
            if( NULL != currentName && NULL != currentName->m_id ) {
                const std::string name( currentName->m_id->m_text.m_buffer );
                if( !name.empty() ) {
                    names.push_back( name );
                }
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleObjectRefNode( DDLNode *node, aiScene *pScene ) {
    if( NULL == m_currentNode ) {
        throw DeadlyImportError( "No parent node for name." );
        return;
    }

    std::vector<std::string> objRefNames;
    getRefNames( node, objRefNames );
    m_currentNode->mNumMeshes = objRefNames.size();
    m_currentNode->mMeshes = new unsigned int[ objRefNames.size() ];
    if( !objRefNames.empty() ) {
        m_unresolvedRefStack.push_back( new RefInfo( m_currentNode, RefInfo::MeshRef, objRefNames ) );
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleMaterialRefNode( ODDLParser::DDLNode *node, aiScene *pScene ) {
    if( NULL == m_currentNode ) {
        throw DeadlyImportError( "No parent node for name." );
        return;
    }

    std::vector<std::string> matRefNames;
    getRefNames( node, matRefNames );
    if( !matRefNames.empty() ) {
        m_unresolvedRefStack.push_back( new RefInfo( m_currentNode, RefInfo::MaterialRef, matRefNames ) );
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleGeometryNode( DDLNode *node, aiScene *pScene ) {
    aiNode *newNode = new aiNode;
    pushNode( newNode, pScene );
    m_tokenType = Grammar::GeometryNodeToken;
    m_currentNode = newNode;
    handleNodes( node, pScene );

    popNode();
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleGeometryObject( DDLNode *node, aiScene *pScene ) {
    handleNodes( node, pScene );
}

//------------------------------------------------------------------------------------------------
static void setMatrix( aiNode *node, DataArrayList *transformData ) {
    ai_assert( NULL != node );
    ai_assert( NULL != transformData );

    float m[ 16 ];
    size_t i( 1 );
    Value *next( transformData->m_dataList->m_next );
    m[ 0 ] = transformData->m_dataList->getFloat();
    while(  next != NULL ) {
        m[ i ] = next->getFloat();
        next = next->m_next;
        i++;
    }

    node->mTransformation.a1 = m[ 0 ];
    node->mTransformation.a2 = m[ 4 ];
    node->mTransformation.a3 = m[ 8 ];
    node->mTransformation.a4 = m[ 12 ];

    node->mTransformation.b1 = m[ 1 ];
    node->mTransformation.b2 = m[ 5 ];
    node->mTransformation.b3 = m[ 9 ];
    node->mTransformation.b4 = m[ 13 ];

    node->mTransformation.c1 = m[ 2 ];
    node->mTransformation.c2 = m[ 6 ];
    node->mTransformation.c3 = m[ 10 ];
    node->mTransformation.c4 = m[ 14 ];

    node->mTransformation.d1 = m[ 3 ];
    node->mTransformation.d2 = m[ 7 ];
    node->mTransformation.d3 = m[ 11 ];
    node->mTransformation.d4 = m[ 15 ];

}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleTransformNode( ODDLParser::DDLNode *node, aiScene *pScene ) {
    if( NULL == m_currentNode ) {
        throw DeadlyImportError( "No parent node for name." );
        return;
    }


    DataArrayList *transformData( node->getDataArrayList() );
    if( NULL != transformData ) {
        if( transformData->m_numItems != 16 ) {
            throw DeadlyImportError( "Invalid number of data for transform matrix." );
            return;
        }
        setMatrix( m_currentNode, transformData );
    }
}

//------------------------------------------------------------------------------------------------
static void propId2StdString( Property *prop, std::string &name, std::string &key ) {
    name = key = "";
    if( NULL == prop ) {
        return;
    }

    if( NULL != prop->m_key ) {
        name = prop->m_key->m_text.m_buffer;
        if( Value::ddl_string == prop->m_value->m_type ) {
            key = prop->m_value->getString();
        }
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleMeshNode( ODDLParser::DDLNode *node, aiScene *pScene ) {
    m_currentMesh = new aiMesh;
    const size_t meshidx( m_meshCache.size() );
    m_meshCache.push_back( m_currentMesh );

    Property *prop = node->getProperties();
    if( NULL != prop ) {
        std::string propName, propKey;
        propId2StdString( prop, propName, propKey );
        if( "primitive" == propName ) {
            if( "triangles" == propKey ) {
                m_currentMesh->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
            }
        }
    }

    handleNodes( node, pScene );

    DDLNode *parent( node->getParent() );
    if( NULL != parent ) {
        const std::string &name = parent->getName();
        m_mesh2refMap[ name ] = meshidx;
    }
}

//------------------------------------------------------------------------------------------------
enum MeshAttribute {
    None,
    Position,
    Normal,
    TexCoord
};

//------------------------------------------------------------------------------------------------
static MeshAttribute getAttributeByName( const char *attribName ) {
    ai_assert( NULL != attribName  );

    if( 0 == strncmp( "position", attribName, strlen( "position" ) ) ) {
        return Position;
    } else if( 0 == strncmp( "normal", attribName, strlen( "normal" ) ) ) {
        return Normal;
    } else if( 0 == strncmp( "texcoord", attribName, strlen( "texcoord" ) ) ) {
        return TexCoord;
    }

    return None;
}

//------------------------------------------------------------------------------------------------
static void fillVector3( aiVector3D *vec3, Value *vals ) {
    ai_assert( NULL != vec3 );
    ai_assert( NULL != vals );

    float x( 0.0f ), y( 0.0f ), z( 0.0f );
    Value *next( vals );
    x = next->getFloat();
    next = next->m_next;
    y = next->getFloat();
    next = next->m_next;
    if( NULL != next ) {
        z = next->getFloat();
    }

    vec3->Set( x, y, z );
}

//------------------------------------------------------------------------------------------------
static size_t countDataArrayListItems( DataArrayList *vaList ) {
    size_t numItems( 0 );
    if( NULL == vaList ) {
        return numItems;
    }

    DataArrayList *next( vaList );
    while( NULL != next ) {
        if( NULL != vaList->m_dataList ) {
            numItems++;
        }
        next = next->m_next;
    }

    return numItems;
}

//------------------------------------------------------------------------------------------------
static void copyVectorArray( size_t numItems, DataArrayList *vaList, aiVector3D *vectorArray ) {
    for( size_t i = 0; i < numItems; i++ ) {
        Value *next( vaList->m_dataList );
        fillVector3( &vectorArray[ i ], next );
        vaList = vaList->m_next;
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleVertexArrayNode( ODDLParser::DDLNode *node, aiScene *pScene ) {
    if( NULL == node ) {
        throw DeadlyImportError( "No parent node for name." );
        return;
    }

    Property *prop( node->getProperties() );
    if( NULL != prop ) {
        std::string propName, propKey;
        propId2StdString( prop, propName, propKey );
        MeshAttribute attribType( getAttributeByName( propKey.c_str() ) );
        if( None == attribType ) {
            return;
        }

        DataArrayList *vaList = node->getDataArrayList();
        if( NULL == vaList ) {
            return;
        }

        const size_t numItems( countDataArrayListItems( vaList ) );
        if( Position == attribType ) {
            m_currentVertices.m_numVerts = numItems;
            m_currentVertices.m_vertices = new aiVector3D[ numItems ];
            copyVectorArray( numItems, vaList, m_currentVertices.m_vertices );
        } else if( Normal == attribType ) {
            m_currentVertices.m_numNormals = numItems;
            m_currentVertices.m_normals = new aiVector3D[ numItems ];
            copyVectorArray( numItems, vaList, m_currentVertices.m_normals );
        } else if( TexCoord == attribType ) {
            m_currentVertices.m_numUVComps[ 0 ] = numItems;
            m_currentVertices.m_textureCoords[ 0 ] = new aiVector3D[ numItems ];
            copyVectorArray( numItems, vaList, m_currentVertices.m_textureCoords[ 0 ] );
        }
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleIndexArrayNode( ODDLParser::DDLNode *node, aiScene *pScene ) {
    if( NULL == node ) {
        throw DeadlyImportError( "No parent node for name." );
        return;
    }

    if( NULL == m_currentMesh ) {
        throw DeadlyImportError( "No current mesh for index data found." );
        return;
    }

    DataArrayList *vaList = node->getDataArrayList();
    if( NULL == vaList ) {
        return;
    }

    const size_t numItems( countDataArrayListItems( vaList ) );
    m_currentMesh->mNumFaces = numItems;
    m_currentMesh->mFaces = new aiFace[ numItems ];
    m_currentMesh->mNumVertices = numItems * 3;
    m_currentMesh->mVertices = new aiVector3D[ m_currentMesh->mNumVertices ];
    m_currentMesh->mNormals = new aiVector3D[ m_currentMesh->mNumVertices ];
    m_currentMesh->mNumUVComponents[ 0 ] = numItems * 3;
    m_currentMesh->mTextureCoords[ 0 ] = new aiVector3D[ m_currentMesh->mNumUVComponents[ 0 ] ];

    unsigned int index( 0 );
    for( size_t i = 0; i < m_currentMesh->mNumFaces; i++ ) {
        aiFace &current(  m_currentMesh->mFaces[ i ] );
        current.mNumIndices = 3;
        current.mIndices = new unsigned int[ current.mNumIndices ];
        Value *next( vaList->m_dataList );
        for( size_t indices = 0; indices < current.mNumIndices; indices++ ) {
            const int idx = next->getInt32();
            ai_assert( static_cast<size_t>( idx ) <= m_currentVertices.m_numVerts );

            aiVector3D &pos = ( m_currentVertices.m_vertices[ idx ] );
            aiVector3D &normal = ( m_currentVertices.m_normals[ idx ] );
            aiVector3D &tex = ( m_currentVertices.m_textureCoords[ 0 ][ idx ] );

            ai_assert( index < m_currentMesh->mNumVertices );
            m_currentMesh->mVertices[ index ].Set( pos.x, pos.y, pos.z );
            m_currentMesh->mNormals[ index ].Set( normal.x, normal.y, normal.z );
            m_currentMesh->mTextureCoords[0][ index ].Set( tex.x, tex.y, tex.z );
            current.mIndices[ indices ] = index;
            index++;

            next = next->m_next;
        }
        vaList = vaList->m_next;
    }
}

//------------------------------------------------------------------------------------------------
static void getColorRGB( aiColor3D *pColor, DataArrayList *colList ) {
    if( NULL == pColor || NULL == colList ) {
        return;
    }

    ai_assert( 3 == colList->m_numItems );
    Value *val( colList->m_dataList );
    pColor->r = val->getFloat();
    val = val->getNext();
    pColor->g = val->getFloat();
    val = val->getNext();
    pColor->b = val->getFloat();
}

//------------------------------------------------------------------------------------------------
enum ColorType {
    NoneColor = 0,
    DiffuseColor,
    SpecularColor,
    EmissionColor
};

//------------------------------------------------------------------------------------------------
static ColorType getColorType( Identifier *id ) {
    if( id->m_text == Grammar::DiffuseColorToken ) {
        return DiffuseColor;
    } else if( id->m_text == Grammar::SpecularColorToken ) {
        return SpecularColor;
    } else if( id->m_text == Grammar::EmissionColorToken ) {
        return EmissionColor;
    }

    return NoneColor;
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleMaterialNode( ODDLParser::DDLNode *node, aiScene *pScene ) {
    m_currentMaterial = new aiMaterial;
    m_materialCache.push_back( m_currentMaterial );
    m_tokenType = Grammar::MaterialToken;
    handleNodes( node, pScene );
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleColorNode( ODDLParser::DDLNode *node, aiScene *pScene ) {
    if( NULL == node ) {
        return;
    }

    Property *prop = node->findPropertyByName( "attrib" );
    if( NULL != prop ) {
        if( NULL != prop->m_value ) {
            DataArrayList *colList( node->getDataArrayList() );
            if( NULL == colList ) {
                return;
            }
            aiColor3D col;
            getColorRGB( &col, colList );
            const ColorType colType( getColorType( prop->m_key ) );
            if( DiffuseColor == colType ) {
                m_currentMaterial->AddProperty( &col, 1, AI_MATKEY_COLOR_DIFFUSE );
            } else if( SpecularColor == colType ) {
                m_currentMaterial->AddProperty( &col, 1, AI_MATKEY_COLOR_SPECULAR );
            } else if( EmissionColor == colType ) {
                m_currentMaterial->AddProperty( &col, 1, AI_MATKEY_COLOR_EMISSIVE );
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::handleTextureNode( ODDLParser::DDLNode *node, aiScene *pScene ) {
    if( NULL == node ) {
        return;
    }

    Property *prop = node->findPropertyByName( "attrib" );
    if( NULL != prop ) {
        if( NULL != prop->m_value ) {
            Value *val( node->getValue() );
            if( NULL != val ) {
                aiString tex;
                tex.Set( val->getString() );
                if( prop->m_value->getString() == Grammar::DiffuseTextureToken ) {
                    m_currentMaterial->AddProperty( &tex, AI_MATKEY_TEXTURE_DIFFUSE( 0 ) );
                } else if( prop->m_value->getString() == Grammar::SpecularPowerTextureToken ) {
                    m_currentMaterial->AddProperty( &tex, AI_MATKEY_TEXTURE_SPECULAR( 0 ) );

                } else if( prop->m_value->getString() == Grammar::EmissionTextureToken ) {
                    m_currentMaterial->AddProperty( &tex, AI_MATKEY_TEXTURE_EMISSIVE( 0 ) );

                } else if( prop->m_value->getString() == Grammar::OpacyTextureToken ) {
                    m_currentMaterial->AddProperty( &tex, AI_MATKEY_TEXTURE_OPACITY( 0 ) );

                } else if( prop->m_value->getString() == Grammar::TransparencyTextureToken ) {
                    // ToDo!
                    // m_currentMaterial->AddProperty( &tex, AI_MATKEY_TEXTURE_DIFFUSE( 0 ) );
                } else if( prop->m_value->getString() == Grammar::NormalTextureToken ) {
                    m_currentMaterial->AddProperty( &tex, AI_MATKEY_TEXTURE_NORMALS( 0 ) );

                }
                else {
                    ai_assert( false );
                }
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::copyMeshes( aiScene *pScene ) {
    if( m_meshCache.empty() ) {
        return;
    }

    pScene->mNumMeshes = m_meshCache.size();
    pScene->mMeshes = new aiMesh*[ pScene->mNumMeshes ];
    std::copy( m_meshCache.begin(), m_meshCache.end(), pScene->mMeshes );
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::resolveReferences() {
    if( m_unresolvedRefStack.empty() ) {
        return;
    }

    RefInfo *currentRefInfo( NULL );
    for( std::vector<RefInfo*>::iterator it = m_unresolvedRefStack.begin(); it != m_unresolvedRefStack.end(); ++it ) {
        currentRefInfo = *it;
        if( NULL != currentRefInfo ) {
            aiNode *node( currentRefInfo->m_node );
            if( RefInfo::MeshRef == currentRefInfo->m_type ) {
                for( size_t i = 0; i < currentRefInfo->m_Names.size(); i++ ) {
                    const std::string &name(currentRefInfo->m_Names[ i ] );
                    ReferenceMap::const_iterator it( m_mesh2refMap.find( name ) );
                    if( m_mesh2refMap.end() != it ) {
                        unsigned int meshIdx = m_mesh2refMap[ name ];
                        node->mMeshes[ i ] = meshIdx;
                    }
                }
            } else if( RefInfo::MaterialRef == currentRefInfo->m_type ) {
                // ToDo!
            } else {
                throw DeadlyImportError( "Unknown reference info to resolve." );
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::createNodeTree( aiScene *pScene ) {
    if( NULL == m_root ) {
        return;
    }

    if( m_root->m_children.empty() ) {
        return;
    }

    pScene->mRootNode->mNumChildren = m_root->m_children.size();
    pScene->mRootNode->mChildren = new aiNode*[ pScene->mRootNode->mNumChildren ];
    std::copy( m_root->m_children.begin(), m_root->m_children.end(), pScene->mRootNode->mChildren );
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::pushNode( aiNode *node, aiScene *pScene ) {
    ai_assert( NULL != pScene );

    if( NULL != node ) {
        ChildInfo *info( NULL );
        if( m_nodeStack.empty() ) {
            node->mParent = pScene->mRootNode;
            NodeChildMap::iterator it( m_nodeChildMap.find( node->mParent ) );
            if( m_nodeChildMap.end() == it ) {
                info = new ChildInfo;
                m_root = info;
                m_nodeChildMap[ node->mParent ] = info;
            } else {
                info = it->second;
            }
            info->m_children.push_back( node );
        } else {
            aiNode *parent( m_nodeStack.back() );
            ai_assert( NULL != parent );
            node->mParent = parent;
            NodeChildMap::iterator it( m_nodeChildMap.find( node->mParent ) );
            if( m_nodeChildMap.end() == it ) {
                info = new ChildInfo;
                m_nodeChildMap[ node->mParent ] = info;
            } else {
                info = it->second;
            }
            info->m_children.push_back( node );
        }
        m_nodeStack.push_back( node );
    }
}

//------------------------------------------------------------------------------------------------
aiNode *OpenGEXImporter::popNode() {
    if( m_nodeStack.empty() ) {
        return NULL;
    }

    aiNode *node( top() );
    m_nodeStack.pop_back();

    return node;
}

//------------------------------------------------------------------------------------------------
aiNode *OpenGEXImporter::top() const {
    if( m_nodeStack.empty() ) {
        return NULL;
    }

    return m_nodeStack.back();
}

//------------------------------------------------------------------------------------------------
void OpenGEXImporter::clearNodeStack() {
    m_nodeStack.clear();
}

//------------------------------------------------------------------------------------------------

} // Namespace OpenGEX
} // Namespace Assimp

#endif // ASSIMP_BUILD_NO_OPENGEX_IMPORTER
