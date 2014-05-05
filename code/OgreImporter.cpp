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

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include <vector>
#include <sstream>

#include "OgreImporter.h"
#include "TinyFormatter.h"
#include "irrXMLWrapper.h"

static const aiImporterDesc desc = {
	"Ogre XML Mesh Importer",
	"",
	"",
	"",
	aiImporterFlags_SupportTextFlavour,
	0,
	0,
	0,
	0,
	"mesh.xml"
};

using namespace std;

namespace Assimp
{
namespace Ogre
{

const aiImporterDesc* OgreImporter::GetInfo() const
{
	return &desc;
}

void OgreImporter::SetupProperties(const Importer* pImp)
{
	m_userDefinedMaterialLibFile = pImp->GetPropertyString(AI_CONFIG_IMPORT_OGRE_MATERIAL_FILE, "Scene.material");
	m_detectTextureTypeFromFilename = pImp->GetPropertyBool(AI_CONFIG_IMPORT_OGRE_TEXTURETYPE_FROM_FILENAME, false);
}

bool OgreImporter::CanRead(const std::string &pFile, Assimp::IOSystem *pIOHandler, bool checkSig) const
{
	if (!checkSig) {
		return EndsWith(pFile, ".mesh.xml", false);
	}

	const char* tokens[] = { "<mesh>" };
	return SearchFileHeaderForToken(pIOHandler, pFile, tokens, 1);
}

void OgreImporter::InternReadFile(const std::string &pFile, aiScene *pScene, Assimp::IOSystem *pIOHandler)
{
	// -------------------- Initial file and XML operations --------------------
	
	// Open
	boost::scoped_ptr<IOStream> file(pIOHandler->Open(pFile));
	if (!file.get()) {
		throw DeadlyImportError("Failed to open file " + pFile);
	}

	// Read
	boost::scoped_ptr<CIrrXML_IOStreamReader> xmlStream(new CIrrXML_IOStreamReader(file.get()));
	boost::scoped_ptr<XmlReader> reader(irr::io::createIrrXMLReader(xmlStream.get()));
	if (!reader) {
		throw DeadlyImportError("Failed to create XML Reader for " + pFile);
	}

	DefaultLogger::get()->debug("Opened a XML reader for " + pFile);

	// Read root node
	NextNode(reader.get());
	if (!CurrentNodeNameEquals(reader.get(), "mesh")) {
		throw DeadlyImportError("Root node is not <mesh> but <" + string(reader->getNodeName()) + "> in " + pFile);
	}
	
	// Node names
	const string nnSharedGeometry = "sharedgeometry";
	const string nnVertexBuffer   = "vertexbuffer";
	const string nnSubMeshes      = "submeshes";
	const string nnSubMesh        = "submesh";
	const string nnSubMeshNames   = "submeshnames";
	const string nnSkeletonLink   = "skeletonlink";

	// -------------------- Shared Geometry --------------------
	// This can be used to share geometry between submeshes

	NextNode(reader.get());
	if (CurrentNodeNameEquals(reader.get(), nnSharedGeometry))
	{
		DefaultLogger::get()->debug("Reading shared geometry");
		unsigned int NumVertices = GetAttribute<unsigned int>(reader.get(), "vertexcount");

		NextNode(reader.get());
		while(CurrentNodeNameEquals(reader.get(), nnVertexBuffer)) {
			ReadVertexBuffer(m_SharedGeometry, reader.get(), NumVertices);
		}
	}

	// -------------------- Sub Meshes --------------------

	if (!CurrentNodeNameEquals(reader.get(), nnSubMeshes)) {
		throw DeadlyImportError("Could not find <submeshes> node inside root <mesh> node");
	}

	vector<boost::shared_ptr<SubMesh> > subMeshes;
	vector<aiMaterial*> materials;

	NextNode(reader.get());
	while(CurrentNodeNameEquals(reader.get(), nnSubMesh))
	{
		SubMesh* submesh = new SubMesh();
		ReadSubMesh(subMeshes.size(), *submesh, reader.get());

		// Just a index in a array, we add a mesh in each loop cycle, so we get indicies like 0, 1, 2 ... n;
		// so it is important to do this before pushing the mesh in the vector!
		/// @todo Not sure if this really is needed, refactor out if possible.
		submesh->MaterialIndex = subMeshes.size();

		subMeshes.push_back(boost::shared_ptr<SubMesh>(submesh));

		/** @todo What is the correct way of handling empty ref here.
			Does Assimp require there to be a valid material index for each mesh,
			even if its a dummy material. */
		aiMaterial* material = ReadMaterial(pFile, pIOHandler, submesh->MaterialName);
		materials.push_back(material);
	}

	if (subMeshes.empty()) {
		throw DeadlyImportError("Could not find <submeshes> node inside root <mesh> node");
	}

	// This is really a internal error if we failed to create dummy materials.
	if (subMeshes.size() != materials.size()) {
		throw DeadlyImportError("Internal Error: Material count does not match the submesh count");
	}

	// Skip submesh names.
	/// @todo Should these be read to scene etc. metadata?
	if (CurrentNodeNameEquals(reader.get(), nnSubMeshNames))
	{
		NextNode(reader.get());
		while(CurrentNodeNameEquals(reader.get(), nnSubMesh)) {
			NextNode(reader.get());
		}
	}

	// -------------------- Skeleton --------------------

	vector<Bone> Bones;
	vector<Animation> Animations;

	if (CurrentNodeNameEquals(reader.get(), nnSkeletonLink))
	{
		string skeletonFile = GetAttribute<string>(reader.get(), "name");
		if (!skeletonFile.empty())
		{
			ReadSkeleton(pFile, pIOHandler, pScene, skeletonFile, Bones, Animations);
		}
		else
		{
			DefaultLogger::get()->debug("Found a unusual <" + nnSkeletonLink + "> with a empty file reference");
		}
		NextNode(reader.get());
	}
	else
	{
		DefaultLogger::get()->debug("Mesh has no assigned skeleton with <" + nnSkeletonLink + ">");
	}

	// Now there might be <boneassignments> for the shared geometry
	if (CurrentNodeNameEquals(reader.get(), "boneassignments")) {
		ReadBoneWeights(m_SharedGeometry, reader.get());
	}

	// -------------------- Process Results --------------------
	BOOST_FOREACH(boost::shared_ptr<SubMesh> submesh, subMeshes)
	{
		ProcessSubMesh(*submesh.get(), m_SharedGeometry);
	}

	// -------------------- Apply to aiScene --------------------

	// Materials
	pScene->mMaterials = new aiMaterial*[materials.size()];
	pScene->mNumMaterials = materials.size();

	for(size_t i=0, len=materials.size(); i<len; ++i) {
		pScene->mMaterials[i] = materials[i];
	}

	// Meshes
	pScene->mMeshes = new aiMesh*[subMeshes.size()];
	pScene->mNumMeshes = subMeshes.size();

	for(size_t i=0, len=subMeshes.size(); i<len; ++i)
	{
		boost::shared_ptr<SubMesh> submesh = subMeshes[i];
		pScene->mMeshes[i] = CreateAssimpSubMesh(pScene, *(submesh.get()), Bones);
	}

	// Create the root node
	pScene->mRootNode = new aiNode();
	pScene->mRootNode->mMeshes = new unsigned int[subMeshes.size()];
	pScene->mRootNode->mNumMeshes = subMeshes.size();
	
	for(size_t i=0, len=subMeshes.size(); i<len; ++i) {
		pScene->mRootNode->mMeshes[i] = static_cast<unsigned int>(i);
	}

	// Skeleton and animations
	CreateAssimpSkeleton(pScene, Bones, Animations);
}

} // Ogre
} // Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
