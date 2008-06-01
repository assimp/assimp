
/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
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

----------------------------------------------------------------------
*/

/** @file Definition of the .3ds importer class. */
#ifndef AI_3DSIMPORTER_H_INC
#define AI_3DSIMPORTER_H_INC

#include <map>

#include "BaseImporter.h"
#include "../include/aiTypes.h"

struct aiNode;
#include "3DSHelper.h"

namespace Assimp
{
class MaterialHelper;

using namespace Dot3DS;

// ---------------------------------------------------------------------------
/** The Dot3DSImporter is a worker class capable of importing a scene from a
* 3ds Max 4/5 Files (.3ds)
*/
class Dot3DSImporter : public BaseImporter
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	Dot3DSImporter();

	/** Destructor, private as well */
	~Dot3DSImporter();

public:
	// -------------------------------------------------------------------
	/** Returns whether the class can handle the format of the given file. 
	* See BaseImporter::CanRead() for details.	*/
	bool CanRead( const std::string& pFile, IOSystem* pIOHandler) const;

protected:

	// -------------------------------------------------------------------
	/** Called by Importer::GetExtensionList() for each loaded importer.
	 * See BaseImporter::GetExtensionList() for details
	 */
	void GetExtensionList(std::string& append)
	{
		append.append("*.3ds");
	}

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. 
	* See BaseImporter::InternReadFile() for details
	*/
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);

	// -------------------------------------------------------------------
	/** Converts a temporary material to the outer representation 
	*/
	void ConvertMaterial(Dot3DS::Material& p_cMat,
		MaterialHelper& p_pcOut);

	// -------------------------------------------------------------------
	/** Read a chunk, get a pointer to it
	*  The mCurrent pointer will be increased by sizeof(Dot3DSFile::Chunk),
	* thus pointing directly to the data of the chunk
	*/
	void ReadChunk(const Dot3DSFile::Chunk** p_ppcOut);

	// -------------------------------------------------------------------
	/** Parse a percentage chunk. mCurrent will point to the next
	* chunk behind afterwards. If no percentage chunk is found
	* QNAN is returned.
	*/
	float ParsePercentageChunk();

	// -------------------------------------------------------------------
	/** Parse a color chunk. mCurrent will point to the next
	* chunk behind afterwards. If no color chunk is found
	* QNAN is returned in all members.
	*/
	void ParseColorChunk(aiColor3D* p_pcOut,
		bool p_bAcceptPercent = true);


	// -------------------------------------------------------------------
	/** Skip a chunk in the file
	*/
	void SkipChunk();

	// -------------------------------------------------------------------
	/** Generate the nodegraph
	*/
	void GenerateNodeGraph(aiScene* pcOut);

	// -------------------------------------------------------------------
	/** Parse a main top-level chunk in the file
	*/
	void ParseMainChunk(int* piRemaining);

	// -------------------------------------------------------------------
	/** Parse a top-level chunk in the file
	*/
	void ParseChunk(int* piRemaining);

	// -------------------------------------------------------------------
	/** Parse a top-level editor chunk in the file
	*/
	void ParseEditorChunk(int* piRemaining);

	// -------------------------------------------------------------------
	/** Parse a top-level object chunk in the file
	*/
	void ParseObjectChunk(int* piRemaining);

	// -------------------------------------------------------------------
	/** Parse a material chunk in the file
	*/
	void ParseMaterialChunk(int* piRemaining);

	// -------------------------------------------------------------------
	/** Parse a mesh chunk in the file
	*/
	void ParseMeshChunk(int* piRemaining);

	// -------------------------------------------------------------------
	/** Parse a face list chunk in the file
	*/
	void ParseFaceChunk(int* piRemaining);

	// -------------------------------------------------------------------
	/** Parse a keyframe chunk in the file
	*/
	void ParseKeyframeChunk(int* piRemaining);

	// -------------------------------------------------------------------
	/** Parse a hierarchy chunk in the file
	*/
	void ParseHierarchyChunk(int* piRemaining);

	// -------------------------------------------------------------------
	/** Parse a texture chunk in the file
	*/
	void ParseTextureChunk(int* piRemaining,Dot3DS::Texture* pcOut);

	// -------------------------------------------------------------------
	/** Convert the meshes in the file
	*/
	void ConvertMeshes(aiScene* pcOut);

	// -------------------------------------------------------------------
	/** Replace the default material in the scene
	*/
	void ReplaceDefaultMaterial();

	// -------------------------------------------------------------------
	/** Convert the whole scene
	*/
	void ConvertScene(aiScene* pcOut);

	// -------------------------------------------------------------------
	/** generate normal vectors for a given mesh
	*/
	void GenNormals(Dot3DS::Mesh* sMesh);

	// -------------------------------------------------------------------
	/** generate unique vertices for a mesh
	*/
	void MakeUnique(Dot3DS::Mesh* sMesh);

	// -------------------------------------------------------------------
	/** Add a node to the node graph
	*/
	void AddNodeToGraph(aiScene* pcSOut,aiNode* pcOut,Dot3DS::Node* pcIn);

	// -------------------------------------------------------------------
	/** Search for a node in the graph.
	* Called recursively
	*/
	void InverseNodeSearch(Dot3DS::Node* pcNode,Dot3DS::Node* pcCurrent);

	// -------------------------------------------------------------------
	/** Apply the master scaling factor to the mesh
	*/
	void ApplyMasterScale(aiScene* pScene);

	// -------------------------------------------------------------------
	/** Clamp all indices in the file to a valid range
	*/
	void CheckIndices(Dot3DS::Mesh* sMesh);


protected:
	/** Buffer to hold the loaded file */
	unsigned char* mBuffer;

	/** Pointer to the current read position */
	const unsigned char* mCurrent;

	/** Used to store old chunk addresses to jump back in the file*/
	const unsigned char* mLast;

	/** Last touched node index */
	short mLastNodeIndex;

	/** Current node, root node */
	Dot3DS::Node* mCurrentNode, *mRootNode;

	/** Scene under construction */
	Dot3DS::Scene* mScene;

	/** Ambient base color of the scene */
	aiColor3D mClrAmbient;

	/** Master scaling factor of the scene */
	float mMasterScale;

	/** Path to the background image of the scene */
	std::string mBackgroundImage;
	bool bHasBG;
};

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC