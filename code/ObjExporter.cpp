/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
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

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_OBJ_EXPORTER

#include "ObjExporter.h"
#include "../include/assimp/version.h"

using namespace Assimp;
namespace Assimp	{

// ------------------------------------------------------------------------------------------------
// Worker function for exporting a scene to Wavefront OBJ. Prototyped and registered in Exporter.cpp
void ExportSceneObj(const char* pFile,IOSystem* pIOSystem, const aiScene* pScene)
{
	// invoke the exporter 
	ObjExporter exporter(pFile, pScene);

	// we're still here - export successfully completed. Write both the main OBJ file and the material script
	{
		boost::scoped_ptr<IOStream> outfile (pIOSystem->Open(pFile,"wt"));
		outfile->Write( exporter.mOutput.str().c_str(), static_cast<size_t>(exporter.mOutput.tellp()),1);
	}
	{
		boost::scoped_ptr<IOStream> outfile (pIOSystem->Open(exporter.GetMaterialLibFileName(),"wt"));
		outfile->Write( exporter.mOutputMat.str().c_str(), static_cast<size_t>(exporter.mOutputMat.tellp()),1);
	}
}

} // end of namespace Assimp


// ------------------------------------------------------------------------------------------------
ObjExporter :: ObjExporter(const char* _filename, const aiScene* pScene)
: filename(_filename)
, pScene(pScene)
, endl("\n") 
{
	// make sure that all formatting happens using the standard, C locale and not the user's current locale
	const std::locale& l = std::locale("C");
	mOutput.imbue(l);
	mOutputMat.imbue(l);

	WriteGeometryFile();
	WriteMaterialFile();
}

// ------------------------------------------------------------------------------------------------
std::string ObjExporter :: GetMaterialLibName()
{	
	// within the Obj file, we use just the relative file name with the path stripped
	const std::string& s = GetMaterialLibFileName();
	std::string::size_type il = s.find_last_of("/\\");
	if (il != std::string::npos) {
		return s.substr(il + 1);
	}

	return s;
}

// ------------------------------------------------------------------------------------------------
std::string ObjExporter :: GetMaterialLibFileName()
{	
	return filename + ".mtl";
}

// ------------------------------------------------------------------------------------------------
void ObjExporter :: WriteHeader(std::ostringstream& out)
{
	out << "# File produced by Open Asset Import Library (http://www.assimp.sf.net)" << endl;
	out << "# (assimp v" << aiGetVersionMajor() << '.' << aiGetVersionMinor() << '.' << aiGetVersionRevision() << ")" << endl  << endl;
}

// ------------------------------------------------------------------------------------------------
std::string ObjExporter :: GetMaterialName(unsigned int index)
{
	const aiMaterial* const mat = pScene->mMaterials[index];
	aiString s;
	if(AI_SUCCESS == mat->Get(AI_MATKEY_NAME,s)) {
		return std::string(s.data,s.length);
	}

	char number[ sizeof(unsigned int) * 3 + 1 ];
	ASSIMP_itoa10(number,index);
	return "$Material_" + std::string(number);
}

// ------------------------------------------------------------------------------------------------
void ObjExporter :: WriteMaterialFile()
{
	WriteHeader(mOutputMat);

	for(unsigned int i = 0; i < pScene->mNumMaterials; ++i) {
		const aiMaterial* const mat = pScene->mMaterials[i];

		int illum = 1;
		mOutputMat << "newmtl " << GetMaterialName(i)  << endl;

		aiColor4D c;
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE,c)) {
			mOutputMat << "kd " << c.r << " " << c.g << " " << c.b << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_AMBIENT,c)) {
			mOutputMat << "ka " << c.r << " " << c.g << " " << c.b << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_COLOR_SPECULAR,c)) {
			mOutputMat << "ks " << c.r << " " << c.g << " " << c.b << endl;
		}

		float o;
		if(AI_SUCCESS == mat->Get(AI_MATKEY_OPACITY,o)) {
			mOutputMat << "d " << o << endl;
		}

		if(AI_SUCCESS == mat->Get(AI_MATKEY_SHININESS,o) && o) {
			mOutputMat << "Ns " << o << endl;
			illum = 2;
		}

		mOutputMat << "illum " << illum << endl;

		aiString s;
		if(AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_DIFFUSE(0),s)) {
			mOutputMat << "map_kd " << s.data << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_AMBIENT(0),s)) {
			mOutputMat << "map_ka " << s.data << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_SPECULAR(0),s)) {
			mOutputMat << "map_ks " << s.data << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_SHININESS(0),s)) {
			mOutputMat << "map_ns " << s.data << endl;
		}
		if(AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_HEIGHT(0),s) || AI_SUCCESS == mat->Get(AI_MATKEY_TEXTURE_NORMALS(0),s)) {
			// implementations seem to vary here, so write both variants
			mOutputMat << "bump " << s.data << endl;
			mOutputMat << "map_bump " << s.data << endl;
		}

		mOutputMat << endl;
	}
}

// ------------------------------------------------------------------------------------------------
void ObjExporter :: WriteGeometryFile()
{
	WriteHeader(mOutput);
	mOutput << "mtllib "  << GetMaterialLibName() << endl << endl;

	// collect mesh geometry
	aiMatrix4x4 mBase;
	AddNode(pScene->mRootNode,mBase);

	// write vertex positions
	mOutput << "# " << vp.size() << " vertex positions" << endl;
	BOOST_FOREACH(const aiVector3D& v, vp) {
		mOutput << "v  " << v.x << " " << v.y << " " << v.z << endl;
	}
	mOutput << endl;

	// write uv coordinates
	mOutput << "# " << vt.size() << " UV coordinates" << endl;
	BOOST_FOREACH(const aiVector3D& v, vt) {
		mOutput << "vt " << v.x << " " << v.y << " " << v.z << endl;
	}
	mOutput << endl;

	// write vertex normals
	mOutput << "# " << vn.size() << " vertex normals" << endl;
	BOOST_FOREACH(const aiVector3D& v, vn) {
		mOutput << "vn " << v.x << " " << v.y << " " << v.z << endl;
	}
	mOutput << endl;

	// now write all mesh instances
	BOOST_FOREACH(const MeshInstance& m, meshes) {
		mOutput << "# Mesh \'" << m.name << "\' with " << m.faces.size() << " faces" << endl;
		mOutput << "g " << m.name << endl;
		mOutput << "usemtl " << m.matname << endl;

		BOOST_FOREACH(const Face& f, m.faces) {
			mOutput << f.kind << ' ';
			BOOST_FOREACH(const FaceVertex& fv, f.indices) {
				mOutput << ' ' << fv.vp;

				if (f.kind != 'p') {
					if (fv.vt || f.kind == 'f') {
						mOutput << '/';
					}
					if (fv.vt) {
						mOutput << fv.vt;
					}
					if (f.kind == 'f') {
						mOutput << '/';
						if (fv.vn) {
							mOutput << fv.vn;
						}
					}
				}
			}

			mOutput << endl;
		}
		mOutput << endl;
	}
}

// ------------------------------------------------------------------------------------------------
void ObjExporter :: AddMesh(const aiString& name, const aiMesh* m, const aiMatrix4x4& mat)
{
	meshes.push_back(MeshInstance());
	MeshInstance& mesh = meshes.back();

	mesh.name = std::string(name.data,name.length) + (m->mName.length ? "_"+std::string(m->mName.data,m->mName.length) : "");
	mesh.matname = GetMaterialName(m->mMaterialIndex);

	mesh.faces.resize(m->mNumFaces);
	for(unsigned int i = 0; i < m->mNumFaces; ++i) {
		const aiFace& f = m->mFaces[i];

		Face& face = mesh.faces[i];
		switch (f.mNumIndices) {
			case 1: 
				face.kind = 'p';
				break;
			case 2: 
				face.kind = 'l';
				break;
			default: 
				face.kind = 'f';
		}
		face.indices.resize(f.mNumIndices);

		for(unsigned int a = 0; a < f.mNumIndices; ++a) {
			const unsigned int idx = f.mIndices[a];

			// XXX need a way to check if this is an unique vertex or if we had it already, 
			// in which case we should instead reference the previous occurrence.
			ai_assert(m->mVertices);
			vp.push_back( mat * m->mVertices[idx] );
			face.indices[a].vp = vp.size();

			if (m->mNormals) {
				vn.push_back( m->mNormals[idx] );
			}
			face.indices[a].vn = vn.size();

			if (m->mTextureCoords[0]) {
				vt.push_back( m->mTextureCoords[0][idx] );
			}
			face.indices[a].vt = vt.size();
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ObjExporter :: AddNode(const aiNode* nd, const aiMatrix4x4& mParent)
{
	const aiMatrix4x4& mAbs = mParent * nd->mTransformation;

	for(unsigned int i = 0; i < nd->mNumMeshes; ++i) {
		AddMesh(nd->mName, pScene->mMeshes[nd->mMeshes[i]],mAbs);
	}

	for(unsigned int i = 0; i < nd->mNumChildren; ++i) {
		AddNode(nd->mChildren[i],mAbs);
	}
}

#endif
#endif
