/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team



All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
*/
#include "SceneDiffer.h"
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include <sstream>

namespace Assimp {

SceneDiffer::SceneDiffer() 
: m_diffs() {
    // empty
}

SceneDiffer::~SceneDiffer() {
    // empty
}

bool SceneDiffer::isEqual( const aiScene *expected, const aiScene *toCompare ) {
    if ( expected == toCompare ) {
        return true;
    }

    if ( nullptr == expected ) {
        return false;
    }

    if ( nullptr == toCompare ) {
        return false;
    }

    // meshes
    if ( expected->mNumMeshes != toCompare->mNumMeshes ) {
        std::stringstream stream;
        stream << "Number of meshes not equal ( expected: " << expected->mNumMeshes << ", found : " << toCompare->mNumMeshes << " )\n";
        addDiff( stream.str() );
        return false;
    }

    for ( unsigned int i = 0; i < expected->mNumMeshes; i++ ) {
        aiMesh *expMesh( expected->mMeshes[ i ] );
        aiMesh *toCompMesh( toCompare->mMeshes[ i ] );
        if ( !compareMesh( expMesh, toCompMesh ) ) {
            std::stringstream stream;
            stream << "Meshes are not equal, index : " << i << "\n";
            addDiff( stream.str() );
        }
    }

    // ToDo!
    return true;
    // materials
    if ( expected->mNumMaterials != toCompare->mNumMaterials ) {
        std::stringstream stream;
        stream << "Number of materials not equal ( expected: " << expected->mNumMaterials << ", found : " << toCompare->mNumMaterials << " )\n";
        addDiff( stream.str() );
        return false;
    }

    if ( expected->mNumMaterials > 0 ) {
        if ( nullptr == expected->mMaterials || nullptr == toCompare->mMaterials ) {
            addDiff( "Number of materials > 0 and mat pointer is nullptr" );
            return false;
        }
    }

    for ( unsigned int i = 0; i < expected->mNumMaterials; i++ ) {
        aiMaterial *expectedMat( expected->mMaterials[ i ] );
        aiMaterial *toCompareMat( expected->mMaterials[ i ] );
        if ( !compareMaterial( expectedMat, toCompareMat ) ) {
            std::stringstream stream;
            stream << "Materials are not equal, index : " << i << "\n";
            addDiff( stream.str() );
        }
    }

    return true;
}

void SceneDiffer::showReport() {
    if ( m_diffs.empty() ) {
        return;
    }

    for ( std::vector<std::string>::iterator it = m_diffs.begin(); it != m_diffs.end(); it++ ) {
        std::cout << *it << "\n";
    }

    std::cout << std::endl;
}

void SceneDiffer::reset() {
    m_diffs.resize( 0 );
}

void SceneDiffer::addDiff( const std::string &diff ) {
    if ( diff.empty() ) {
        return;
    }
    m_diffs.push_back( diff );
}

static std::string dumpVector3( const aiVector3D &toDump ) {
    std::stringstream stream;
    stream << "( " << toDump.x << ", " << toDump.y << ", " << toDump.z << ")";
    return stream.str();
}

/*static std::string dumpColor4D( const aiColor4D &toDump ) {
    std::stringstream stream;
    stream << "( " << toDump.r << ", " << toDump.g << ", " << toDump.b << ", " << toDump.a << ")";
    return stream.str();
}*/

static std::string dumpFace( const aiFace &face ) {
    std::stringstream stream;
    for ( unsigned int i = 0; i < face.mNumIndices; i++ ) {
        stream << face.mIndices[ i ];
        if ( i < face.mNumIndices - 1 ) {
            stream << ", ";
        }
        else {
            stream << "\n";
        }
    }
    return stream.str();
}

bool SceneDiffer::compareMesh( aiMesh *expected, aiMesh *toCompare ) {
    if ( expected == toCompare ) {
        return true;
    }

    if ( nullptr == expected || nullptr == toCompare ) {
        return false;
    }

    if ( expected->mName != toCompare->mName ) {
        std::stringstream stream;
        stream << "Mesh name not equal ( expected: " << expected->mName.C_Str() << ", found : " << toCompare->mName.C_Str() << " )\n";
        addDiff( stream.str() );
    }

    if ( expected->mNumVertices != toCompare->mNumVertices ) {
        std::stringstream stream;
        stream << "Number of vertices not equal ( expected: " << expected->mNumVertices << ", found : " << toCompare->mNumVertices << " )\n";
        addDiff( stream.str() );
        return false;
    }

    // positions
    if ( expected->HasPositions() != toCompare->HasPositions() ) {
        addDiff( "Expected are vertices, toCompare does not have any." );
        return false;
    }

    bool vertEqual( true );
    for ( unsigned int i = 0; i < expected->mNumVertices; i++ ) {
        aiVector3D &expVert( expected->mVertices[ i ] );
        aiVector3D &toCompVert( toCompare->mVertices[ i ] );
        if ( !expVert.Equal( toCompVert ) ) {
            std::cout << "index = " << i << dumpVector3( toCompVert ) << "\n";
            std::stringstream stream;
            stream << "Vertex not equal ( expected: " << dumpVector3( toCompVert ) << ", found: " << dumpVector3( toCompVert ) << "\n";
            addDiff( stream.str() );
            vertEqual = false;
        }
    }
    if ( !vertEqual ) {
        return false;
    }

    // normals
    if ( expected->HasNormals() != toCompare->HasNormals() ) {
        addDiff( "Expected are normals, toCompare does not have any." );
        return false;
    }

    //    return true;

        //ToDo!
    /*bool normalEqual( true );
        for ( unsigned int i = 0; i < expected->mNumVertices; i++ ) {
            aiVector3D &expNormal( expected->mNormals[ i ] );
            aiVector3D &toCompNormal( toCompare->mNormals[ i ] );
            if ( expNormal.Equal( toCompNormal ) ) {
                std::stringstream stream;
                stream << "Normal not equal ( expected: " << dumpVector3( expNormal ) << ", found: " << dumpVector3( toCompNormal ) << "\n";
                addDiff( stream.str() );
                normalEqual = false;
            }
        }
        if ( !normalEqual ) {
            return false;
        }

        // vertex colors
        bool vertColEqual( true );
        for ( unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++ ) {
            if ( expected->HasVertexColors(a) != toCompare->HasVertexColors(a) ) {
                addDiff( "Expected are normals, toCompare does not have any." );
                return false;
            }
            for ( unsigned int i = 0; i < expected->mNumVertices; i++ ) {
                aiColor4D &expColor4D( expected->mColors[ a ][ i ] );
                aiColor4D &toCompColor4D( toCompare->mColors[ a ][ i ] );
                if ( expColor4D != toCompColor4D ) {
                    std::stringstream stream;
                    stream << "Color4D not equal ( expected: " << dumpColor4D( expColor4D ) << ", found: " << dumpColor4D( toCompColor4D ) << "\n";
                    addDiff( stream.str() );
                    vertColEqual = false;
                }
            }
            if ( !vertColEqual ) {
                return false;
            }
        }

        // texture coords
        bool texCoordsEqual( true );
        for ( unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++ ) {
            if ( expected->HasTextureCoords( a ) != toCompare->HasTextureCoords( a ) ) {
                addDiff( "Expected are texture coords, toCompare does not have any." );
                return false;
            }
            for ( unsigned int i = 0; i < expected->mNumVertices; i++ ) {
                aiVector3D &expTexCoord( expected->mTextureCoords[ a ][ i ] );
                aiVector3D &toCompTexCoord( toCompare->mTextureCoords[ a ][ i ] );
                if ( expTexCoord.Equal( toCompTexCoord ) ) {
                    std::stringstream stream;
                    stream << "Texture coords not equal ( expected: " << dumpVector3( expTexCoord ) << ", found: " << dumpVector3( toCompTexCoord ) << "\n";
                    addDiff( stream.str() );
                    vertColEqual = false;
                }
            }
            if ( !vertColEqual ) {
                return false;
            }
        }

        // tangents and bi-tangents
        if ( expected->HasTangentsAndBitangents() != toCompare->HasTangentsAndBitangents() ) {
            addDiff( "Expected are tangents and bi-tangents, toCompare does not have any." );
            return false;
        }
        bool tangentsEqual( true );
        for ( unsigned int i = 0; i < expected->mNumVertices; i++ ) {
            aiVector3D &expTangents( expected->mTangents[ i ] );
            aiVector3D &toCompTangents( toCompare->mTangents[ i ] );
            if ( expTangents.Equal( toCompTangents ) ) {
                std::stringstream stream;
                stream << "Tangents not equal ( expected: " << dumpVector3( expTangents ) << ", found: " << dumpVector3( toCompTangents ) << "\n";
                addDiff( stream.str() );
                tangentsEqual = false;
            }

            aiVector3D &expBiTangents( expected->mBitangents[ i ] );
            aiVector3D &toCompBiTangents( toCompare->mBitangents[ i ] );
            if ( expBiTangents.Equal( toCompBiTangents ) ) {
                std::stringstream stream;
                stream << "Tangents not equal ( expected: " << dumpVector3( expBiTangents ) << ", found: " << dumpVector3( toCompBiTangents ) << " )\n";
                addDiff( stream.str() );
                tangentsEqual = false;
            }
        }
        if ( !tangentsEqual ) {
            return false;
        }*/

        // faces
    if ( expected->mNumFaces != toCompare->mNumFaces ) {
        std::stringstream stream;
        stream << "Number of faces are not equal, ( expected: " << expected->mNumFaces << ", found: " << toCompare->mNumFaces << ")\n";
        addDiff( stream.str() );
        return false;
    }
    bool facesEqual( true );
    for ( unsigned int i = 0; i < expected->mNumFaces; i++ ) {
        aiFace &expFace( expected->mFaces[ i ] );
        aiFace &toCompareFace( toCompare->mFaces[ i ] );
        if ( !compareFace( &expFace, &toCompareFace ) ) {
            addDiff( "Faces are not equal\n" );
            addDiff( dumpFace( expFace ) );
            addDiff( dumpFace( toCompareFace ) );
            facesEqual = false;
        }
    }
    if ( !facesEqual ) {
        return false;
    }

    return true;
}

bool SceneDiffer::compareFace( aiFace *expected, aiFace *toCompare ) {
    if ( nullptr == expected ) {
        return false;
    }
    if ( nullptr == toCompare ) {
        return false;
    }

    // same instance
    if ( expected == toCompare ) {
        return true;
    }

    // using compare operator
    if ( *expected == *toCompare ) {
        return true;
    }

    return false;
}

bool SceneDiffer::compareMaterial( aiMaterial *expected, aiMaterial *toCompare ) {
    if ( nullptr == expected ) {
        return false;
    }
    if ( nullptr == toCompare ) {
        return false;
    }

    // same instance
    if ( expected == toCompare ) {
        return true;
    }

    // todo!

    return true;
}

}
