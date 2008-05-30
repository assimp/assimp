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


//!
//! @file Definition of MDL importer class
//!

#ifndef AI_MDLLOADER_H_INCLUDED
#define AI_MDLLOADER_H_INCLUDED

#include "BaseImporter.h"
#include "../include/aiTypes.h"
#include "../include/aiTexture.h"
#include "../include/aiMaterial.h"

struct aiNode;
#include "MDLFileData.h"
#include "HalfLifeFileData.h"

namespace Assimp
{
class MaterialHelper;

using namespace MDL;

// ---------------------------------------------------------------------------
/** Used to load MDL files
*/
class MDLImporter : public BaseImporter
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	MDLImporter();

	/** Destructor, private as well */
	~MDLImporter();

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
		append.append("*.mdl");
	}

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. 
	* See BaseImporter::InternReadFile() for details
	*/
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);

protected:

	// -------------------------------------------------------------------
	/** Import a quake 1 MDL file
	*/
	void InternReadFile_Quake1( );

	// -------------------------------------------------------------------
	/** Import a GameStudio A4/A5 file
	*/
	void InternReadFile_GameStudio( );

	// -------------------------------------------------------------------
	/** Import a GameStudio A7 file
	*/
	void InternReadFile_GameStudioA7( );

	// -------------------------------------------------------------------
	/** Import a CS:S/HL2 MDL file
	*/
	void InternReadFile_HL2( );

	// -------------------------------------------------------------------
	/** Load a paletized texture from the file and convert it to 32bpp
	*/
	void CreateTextureARGB8(const unsigned char* szData);


	// -------------------------------------------------------------------
	/** Used to load textures from MDL3/4
	 * \param szData Input data
	 * \param iType Color data type
	 * \param piSkip Receive: Size to skip
	*/
	void CreateTextureARGB8_GS4(const unsigned char* szData, 
		unsigned int iType,
		unsigned int* piSkip);

	// -------------------------------------------------------------------
	/** Used to load textures from MDL5
	 * \param szData Input data
	 * \param iType Color data type
	 * \param piSkip Receive: Size to skip
	*/
	void CreateTextureARGB8_GS5(const unsigned char* szData, 
		unsigned int iType,
		unsigned int* piSkip);

	// -------------------------------------------------------------------
	/** Parse a skin lump in a MDL7 file with all of its features
	 * \param szCurrent Current data pointer
	 * \param szCurrentOut Output data pointer
	 * \param pcMats Material list for this group. To be filled ...
	 */
	void ParseSkinLump_GameStudioA7(
		const unsigned char* szCurrent,
		const unsigned char** szCurrentOut,
		std::vector<MaterialHelper*>& pcMats);

	// -------------------------------------------------------------------
	/** Parse texture color data for MDL5, MDL6 and MDL7 formats
	 * \param szData Current data pointer
	 * \param iType type of the texture data. No DDS or external
	 * \param piSkip Receive the number of bytes to skip
	 * \param pcNew Must point to fully initialized data. Width and 
	 *        height must be set.
	 */
	void ParseTextureColorData(const unsigned char* szData, 
		unsigned int iType,
		unsigned int* piSkip,
		aiTexture* pcNew);

	// -------------------------------------------------------------------
	/** Validate the header data structure of a game studio MDL7 file
	 * \param pcHeader Input header to be validated
	 */
	void ValidateHeader_GameStudioA7(const MDL::Header_MDL7* pcHeader);

	// -------------------------------------------------------------------
	/** Join two materials / skins. Setup UV source ... etc
	 * \param pcMat1 First input material
	 * \param pcMat2 Second input material
	 * \param pcMatOut Output material instance to be filled. Must be empty
	 */
	void JoinSkins_GameStudioA7(MaterialHelper* pcMat1,
		MaterialHelper* pcMat2,
		MaterialHelper* pcMatOut);

	// -------------------------------------------------------------------
	/** Generate the final output meshes for a7 models
	 * \param aiSplit Face-per-material list
	 * \param pcMats List of all materials
	 * \param avOutList Output: List of all meshes
	 * \param pcFaces List of all input faces
	 * \param vPositions List of all input vectors
	 * \param vNormals List of all input normal vectors
	 * \param vTextureCoords1 List of all input UV coords #1
	 * \param vTextureCoords2 List of all input UV coords #2
	 */
	void GenerateOutputMeshes_GameStudioA7(
		const std::vector<unsigned int>** aiSplit,
		const std::vector<MaterialHelper*>& pcMats,
		std::vector<aiMesh*>& avOutList,
		const MDL::IntFace_MDL7* pcFaces,
		const std::vector<aiVector3D>& vPositions,
		const std::vector<aiVector3D>& vNormals, 
		const std::vector<aiVector3D>& vTextureCoords1,
		const std::vector<aiVector3D>& vTextureCoords2);


	// -------------------------------------------------------------------
	/** Try to load a  palette from the current directory (colormap.lmp)
	 *  If it is not found the default palette of Quake1 is returned
	 */
	void SearchPalette(const unsigned char** pszColorMap);

	// -------------------------------------------------------------------
	/** Free a palette created with a previous call to SearchPalette()
	 */
	void FreePalette(const unsigned char* pszColorMap);

	// -------------------------------------------------------------------
	/** Try to determine whether the normals of the model are flipped
	 *  Some MDL7 models seem to have flipped normals (and there is also 
	 *  an option "flip normals" in MED). However, I don't see a proper
	 *  way to read from the file whether all normals are correctly 
	 *  facing outwards ...
	 */
	void FlipNormals(aiMesh* pcMesh);

private:

	/** Header of the MDL file */
	const MDL::Header* m_pcHeader;

	/** Buffer to hold the loaded file */
	unsigned char* mBuffer;

	/** For GameStudio MDL files: The number in the magic 
	word, either 3,4 or 5*/
	unsigned int iGSFileVersion;

	/** Output I/O handler. used to load external lmp files
	*/
	IOSystem* pIOHandler;

	/** Output scene to be filled
	*/
	aiScene* pScene;
};
}; // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC