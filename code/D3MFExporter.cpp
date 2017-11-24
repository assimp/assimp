/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2017, assimp team

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
#include "D3MFExporter.h"

#include <assimp/scene.h>
#include <assimp/IOSystem.hpp>
#include <assimp//IOStream.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/DefaultLogger.hpp>

#include "Exceptional.h"
#include "3MFXmlTags.h"
#include "D3MFOpcPackage.h"

namespace Assimp {

void ExportScene3MF( const char* pFile, IOSystem* pIOSystem, const aiScene* pScene, const ExportProperties* /*pProperties*/ ) {
    D3MF::D3MFExporter myExporter( pFile, pIOSystem, pScene );
    if ( myExporter.validate() ) {
        bool ok = myExporter.exportArchive(pFile);
    }
}

namespace D3MF {

#ifndef ASSIMP_BUILD_NO3MF_EXPORTER

D3MFExporter::D3MFExporter( const char* pFile, IOSystem* pIOSystem, const aiScene* pScene )
: mIOSystem( pIOSystem )
, mArchiveName( pFile )
, mScene( pScene )
, mBuildItems()
, mRelations() {
    // empty
}

D3MFExporter::~D3MFExporter() {
    for ( size_t i = 0; i < mRelations.size(); ++i ) {
        delete mRelations[ i ];
    }
    mRelations.clear();
}

bool D3MFExporter::createFileStructure( const char *file ) {
    if ( !mIOSystem->CreateDirectory( file ) ) {
        return false;
    }

    if ( !mIOSystem->ChangeDirectory( file ) ) {
        return false;
    }

    if ( !mIOSystem->CreateDirectory( "3D" ) ) {
        return false;
    }

    if ( !mIOSystem->CreateDirectory( "_rels" ) ) {
        return false;
    }

    return true;
}

bool D3MFExporter::validate() {
    if ( mArchiveName.empty() ) {
        return false;
    }

    if ( nullptr == mScene ) {
        return false;
    }

    return true;
}

bool D3MFExporter::exportArchive( const char *file ) {
    bool ok( true );
    ok |= createFileStructure( file );
    ok |= exportRelations();
    ok |= export3DModel();

    if ( ok ) {
        createZipArchiveFromeFileStructure();
    }
    return ok;
}

bool D3MFExporter::exportRelations() {
    mOutput.clear();

    mOutput << "<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n";
    mOutput << "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n";

    for ( size_t i = 0; i < mRelations.size(); ++i ) {
        mOutput << "<Relationship Target =\"/3D/" << mRelations[ i ]->target << " ";
        mOutput << "id=\"" << mRelations[i]->id << " ";
        mOutput << "Type=\"" << mRelations[ i ]->type << "/>\n";
    }
    mOutput << "</Relationships>\n";

    writeRelInfoToFile( "_rels", ".rels" );

    return true;
}

bool D3MFExporter::export3DModel() {
    mOutput.clear();

    writeHeader();
    mOutput << "<" << XmlTag::model << " " << XmlTag::model_unit << "=\"millimeter\""
            << "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">"
            << "\n";
    mOutput << "<" << XmlTag::resources << ">\n";

    writeObjects();


    mOutput << "</" << XmlTag::resources << ">\n";
    writeBuild();

    mOutput << "</" << XmlTag::model << ">\n";

    OpcPackageRelationship *info = new OpcPackageRelationship;
    info->id = mArchiveName;
    info->target = "rel0";
    info->type = "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel";
    mRelations.push_back( info );

    writeModelToArchive( "3D", mArchiveName );

    mOutput.clear();

    return true;
}

void D3MFExporter::writeHeader() {
    mOutput << "<?xml version=\"1.0\" encoding=\"UTF - 8\"?>" << "\n";
}

void D3MFExporter::writeObjects() {
    if ( nullptr == mScene->mRootNode ) {
        return;
    }

    aiNode *root = mScene->mRootNode;
    for ( unsigned int i = 0; i < root->mNumChildren; ++i ) {
        aiNode *currentNode( root->mChildren[ i ] );
        if ( nullptr == currentNode ) {
            continue;
        }
        mOutput << "<" << XmlTag::object << " id=\"" << currentNode->mName.C_Str() << "\" type=\"model\">\n";
        for ( unsigned int j = 0; j < currentNode->mNumMeshes; ++j ) {
            aiMesh *currentMesh = mScene->mMeshes[ currentNode->mMeshes[ j ] ];
            if ( nullptr == currentMesh ) {
                continue;
            }
            writeMesh( currentMesh );
        }
        mBuildItems.push_back( i );

        mOutput << "</" << XmlTag::object << ">\n";
    }
}

void D3MFExporter::writeMesh( aiMesh *mesh ) {
    if ( nullptr == mesh ) {
        return;
    }

    mOutput << "<" << XmlTag::mesh << ">\n";
    mOutput << "<" << XmlTag::vertices << ">\n";
    for ( unsigned int i = 0; i < mesh->mNumVertices; ++i ) {
        writeVertex( mesh->mVertices[ i ] );
    }
    mOutput << "</" << XmlTag::vertices << ">\n";
    mOutput << "</" << XmlTag::mesh << ">\n";

    writeFaces( mesh );
}

void D3MFExporter::writeVertex( const aiVector3D &pos ) {
    mOutput << "<" << XmlTag::vertex << " x=\"" << pos.x << "\" y=\"" << pos.y << "\" z=\"" << pos.z << "\">\n";
}

void D3MFExporter::writeFaces( aiMesh *mesh ) {
    if ( nullptr == mesh ) {
        return;
    }

    if ( !mesh->HasFaces() ) {
        return;
    }
    mOutput << "<" << XmlTag::triangles << ">\n";
    for ( unsigned int i = 0; i < mesh->mNumFaces; ++i ) {
        aiFace &currentFace = mesh->mFaces[ i ];
        mOutput << "<" << XmlTag::triangle << " v1=\"" << currentFace.mIndices[ 0 ] << "\" v2=\""
                << currentFace.mIndices[ 1 ] << "\" v3=\"" << currentFace.mIndices[ 2 ] << "\"/>\n";
    }
    mOutput << "</" << XmlTag::triangles << ">\n";
}

void D3MFExporter::writeBuild() {
    mOutput << "<" << XmlTag::build << ">\n";

    for ( size_t i = 0; i < mBuildItems.size(); ++i ) {
        mOutput << "<" << XmlTag::item << " objectid=\"" << i + 1 << "\"/>\n";
    }
    mOutput << "</" << XmlTag::build << ">\n";
}

void D3MFExporter::writeModelToArchive( const std::string &folder, const std::string &modelName ) {
    const std::string &oldFolder( mIOSystem->CurrentDirectory() );
    if ( folder != oldFolder ) {
        mIOSystem->PushDirectory( oldFolder );
        mIOSystem->ChangeDirectory( folder );

        const std::string &exportTxt( mOutput.str() );
        std::shared_ptr<IOStream> outfile( mIOSystem->Open( modelName, "wb" ) );
        if ( !outfile ) {
            throw DeadlyExportError( "Could not open output model file: " + std::string( modelName ) );
        }

        const size_t writtenBytes( outfile->Write( exportTxt.c_str(), sizeof( char ), exportTxt.size() ) );

        mIOSystem->ChangeDirectory( ".." );
        mIOSystem->PopDirectory();
    }
}

void D3MFExporter::writeRelInfoToFile( const std::string &folder, const std::string &relName ) {
    const std::string &oldFolder( mIOSystem->CurrentDirectory() );
    if ( folder != oldFolder ) {
        mIOSystem->PushDirectory( oldFolder );
        mIOSystem->ChangeDirectory( folder );

        const std::string &exportTxt( mOutput.str() );
        std::shared_ptr<IOStream> outfile( mIOSystem->Open( relName, "wb" ) );
        if ( !outfile ) {
            throw DeadlyExportError( "Could not open output model file: " + std::string( relName ) );
        }

        const size_t writtenBytes( outfile->Write( exportTxt.c_str(), sizeof( char ), exportTxt.size() ) );

        mIOSystem->ChangeDirectory( ".." );
        mIOSystem->PopDirectory();
    }
}

void D3MFExporter::createZipArchiveFromeFileStructure() {

}

#endif // ASSIMP_BUILD_NO3MF_EXPORTER

} // Namespace D3MF
} // Namespace Assimp
