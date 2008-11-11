/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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

/** @file Implementation of the b3d importer class */

#include "AssimpPCH.h"

// internal headers
#include "B3DImporter.h"
#include "TextureTransform.h"

using namespace Assimp;
using namespace std;

bool B3DImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const{

	int pos=pFile.find_last_of( '.' );
	if( pos==string::npos ) return false;

	string ext=pFile.substr( pos+1 );
	if( ext.size()!=3 ) return false;

	return (ext[0]=='b' || ext[0]=='B') && (ext[1]=='3') && (ext[2]=='d' || ext[2]=='D');
}

void B3DImporter::GetExtensionList( std::string& append ){
	append.append("*.b3d");
}

void B3DImporter::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler){

	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open B3D file " + pFile + ".");

	// check whether the .b3d file is large enough to contain
	// at least one chunk.
	size_t fileSize = file->FileSize();
	if( fileSize < 8) throw new ImportErrorException( "B3D File is too small.");

	_pos=0;
	_buf.resize( fileSize );
	file->Read( &_buf[0],1,fileSize );
	_stack.clear();
	_textures.clear();
	_materials.size();
	_vertices.clear();
	_meshes.clear();

	ReadBB3D();

	//materials
	aiMaterial **mats=new aiMaterial*[_materials.size()];
	for( unsigned i=0;i<_materials.size();++i ){
		mats[i]=_materials[i];
	}
	pScene->mNumMaterials=_materials.size();
	pScene->mMaterials=mats;

	//meshes
	aiMesh **meshes=new aiMesh*[_meshes.size()];
	for( unsigned i=0;i<_meshes.size();++i ){
		meshes[i]=_meshes[i];
	}
	pScene->mNumMeshes=_meshes.size();
	pScene->mMeshes=meshes;

	//nodes - NOTE: Have to create mMeshes array here or crash 'n' burn.
	aiNode *node=new aiNode( "root" );
	node->mNumMeshes=_meshes.size();
	node->mMeshes=new unsigned[_meshes.size()];
	for( unsigned i=0;i<_meshes.size();++i ){
		node->mMeshes[i]=i;
	}
	pScene->mRootNode=node;
}

int B3DImporter::ReadByte(){
	if( _pos<_buf.size() ) return _buf[_pos++];
	throw new ImportErrorException( "B3D EOF Error" );
}

int B3DImporter::ReadInt(){
	if( _pos+4<=_buf.size() ){
		int n=*(int*)&_buf[_pos];
		_pos+=4;
		return n;
	}
	throw new ImportErrorException( "B3D EOF Error" );
}

float B3DImporter::ReadFloat(){
	if( _pos+4<=_buf.size() ){
		float n=*(float*)&_buf[_pos];
		_pos+=4;
		return n;
	}
	throw new ImportErrorException( "B3D EOF Error" );
}

B3DImporter::Vec2 B3DImporter::ReadVec2(){
	Vec2 t;
	t.x=ReadFloat();
	t.y=ReadFloat();
	return t;
}

B3DImporter::Vec3 B3DImporter::ReadVec3(){
	Vec3 t;
	t.x=ReadFloat();
	t.y=ReadFloat();
	t.z=ReadFloat();
	return t;
}

B3DImporter::Vec4 B3DImporter::ReadVec4(){
	Vec4 t;
	t.x=ReadFloat();
	t.y=ReadFloat();
	t.z=ReadFloat();
	t.w=ReadFloat();
	return t;
}

string B3DImporter::ReadString(){
	string str;
	while( _pos<_buf.size() ){
		char c=(char)ReadByte();
		if( !c ) return str;
		str+=c;
	}
	throw new ImportErrorException( "B3D EOF Error" );
}

string B3DImporter::ReadChunk(){
	string tag;
	for( int i=0;i<4;++i ){
		tag+=char( ReadByte() );
	}
//	cout<<"ReadChunk:"<<tag<<endl;
	unsigned sz=(unsigned)ReadInt();
	_stack.push_back( _pos+sz );
	return tag;
}

void B3DImporter::ExitChunk(){
	_pos=_stack.back();
	_stack.pop_back();
}

unsigned B3DImporter::ChunkSize(){
	return _stack.back()-_pos;
}

void B3DImporter::ReadTEXS(){
	while( ChunkSize() ){
		string name=ReadString();
		int flags=ReadInt();
		int blend=ReadInt();
		Vec2 pos=ReadVec2();
		Vec2 scale=ReadVec2();
		float rot=ReadFloat();

		Texture tex;
		tex.name=name;
		_textures.push_back( tex );
	}
}

void B3DImporter::ReadBRUS(){
	int n_texs=ReadInt();
	while( ChunkSize() ){
		string name=ReadString();
		Vec4 color=ReadVec4();
		float shiny=ReadFloat();
		int blend=ReadInt();
		int fx=ReadInt();

		MaterialHelper *mat=new MaterialHelper;
		_materials.push_back( mat );

		for( int i=0;i<n_texs;++i ){
			int texid=ReadInt();
			if( !i ){
				//just use tex 0 for now
				const Texture &tex=_textures[texid];

				aiString texstr;
				texstr.Set( tex.name );
				mat->AddProperty( &texstr,AI_MATKEY_TEXTURE_DIFFUSE(0) );
			}
		}
	}
}

void B3DImporter::ReadVRTS(){
	int vertFlags=ReadInt();
	int tc_sets=ReadInt();
	int tc_size=ReadInt();

	if( tc_sets<0 || tc_sets>4 || tc_size<0 || tc_size>4 ) throw new ImportErrorException( "B3D Param Error" );

	while( ChunkSize() ){
		Vertex vert;

		vert.position=ReadVec3();

		if( vertFlags & 1 ){
			vert.normal=ReadVec3();
		}

		if( vertFlags & 2 ){
			Vec4 color=ReadVec4();
		}

		for( int i=0;i<tc_sets;++i ){
			float texcoords[4]={0,0,0,0};
			for( int j=0;j<tc_size;++j ){
				texcoords[j]=ReadFloat();
			}
			if( !i ) memcpy( &vert.texcoords.x,texcoords,12 );
		}
		_vertices.push_back( vert );
	}
}

void B3DImporter::ReadTRIS(){
	int matid=ReadInt();

	unsigned n_tris=ChunkSize()/12;
	unsigned n_verts=n_tris*3;
	
	aiMesh *mesh=new aiMesh;
	_meshes.push_back( mesh );

	mesh->mMaterialIndex=matid;
	mesh->mNumVertices=n_verts;
	mesh->mNumFaces=n_tris;
	mesh->mPrimitiveTypes=aiPrimitiveType_TRIANGLE;

	aiVector3D *mv=mesh->mVertices=new aiVector3D[n_verts];
	aiVector3D *mn=mesh->mNormals=new aiVector3D[n_verts];
	aiVector3D *mc=mesh->mTextureCoords[0]=new aiVector3D[n_verts];

	aiFace *face=mesh->mFaces=new aiFace[n_tris];

	for( unsigned i=0;i<n_verts;i+=3 ){
		face->mNumIndices=3;
		unsigned *ip=face->mIndices=new unsigned[3];
		for( unsigned j=0;j<3;++j ){
			int k=ReadInt();
			const Vertex &v=_vertices[k];
			memcpy( mv++,&v.position.x,12 );
			memcpy( mn++,&v.normal.x,12 );
			memcpy( mc++,&v.texcoords.x,12 );
			*ip++=i+j;
		}
		++face;
	}
}

void B3DImporter::ReadMESH(){
	int matid=ReadInt();

	_vertices.clear();

	while( ChunkSize() ){
		string t=ReadChunk();
		if( t=="VRTS" ){
			ReadVRTS();
		}else if( t=="TRIS" ){
			ReadTRIS();
		}
		ExitChunk();
	}

	_vertices.clear();
}

void B3DImporter::ReadNODE(){

	string name=ReadString();
	Vec3 trans=ReadVec3();
	Vec3 scale=ReadVec3();
	Vec4 rot=ReadVec4();

	while( ChunkSize() ){
		string t=ReadChunk();
		if( t=="MESH" ){
			ReadMESH();
		}
		ExitChunk();
	}
}

void B3DImporter::ReadBB3D(){
	string t=ReadChunk();
	if( t=="BB3D" ){
		int version=ReadInt();
		while( ChunkSize() ){
			string t=ReadChunk();
			if( t=="TEXS" ){
				ReadTEXS();
			}else if( t=="BRUS" ){
				ReadBRUS();
			}else if( t=="NODE" ){
				ReadNODE();
			}
			ExitChunk();
		}
	}
	ExitChunk();
}
