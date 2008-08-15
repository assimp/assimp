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

/** @file Declaration of the LWO importer class. */
#ifndef AI_LWOLOADER_H_INCLUDED
#define AI_LWOLOADER_H_INCLUDED

#include "../include/aiTypes.h"

#include "BaseImporter.h"
#include "LWOFileData.h"
#include "MaterialSystem.h"

struct aiTexture;
struct aiNode;

namespace Assimp	{
using namespace LWO;

// ---------------------------------------------------------------------------
/** Clas to load LWO files
*/
class LWOImporter : public BaseImporter
{
	friend class Importer;


protected:
	/** Constructor to be privately used by Importer */
	LWOImporter();

	/** Destructor, private as well */
	~LWOImporter();

public:

	// -------------------------------------------------------------------
	/** Returns whether the class can handle the format of the given file. 
	* See BaseImporter::CanRead() for details.	*/
	bool CanRead( const std::string& pFile, IOSystem* pIOHandler) const;


	// -------------------------------------------------------------------
	/** Called prior to ReadFile().
	* The function is a request to the importer to update its configuration
	* basing on the Importer's configuration property list.
	*/
	void SetupProperties(const Importer* pImp);

protected:

	// -------------------------------------------------------------------
	/** Called by Importer::GetExtensionList() for each loaded importer.
	 * See BaseImporter::GetExtensionList() for details
	 */
	void GetExtensionList(std::string& append)
	{
		append.append("*.lwo");
	}

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. 
	* See BaseImporter::InternReadFile() for details
	*/
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);

private:

	// -------------------------------------------------------------------
	/** Loads a LWO file in the older LWOB format (LW < 6)
	 */
	void LoadLWOBFile();

	// -------------------------------------------------------------------
	/** Loads a LWO file in the newer LWO2 format (LW >= 6)
	 */
	void LoadLWO2File();

	// -------------------------------------------------------------------
	/** Loads a surface chunk from an LWOB file
	 *  @param size Maximum size to be read, in bytes.  
	 */
	void LoadLWOBSurface(unsigned int size);

	// -------------------------------------------------------------------
	/** Loads a surface chunk from an LWO2 file
	 *  @param size Maximum size to be read, in bytes.  
	 */
	void LoadLWO2Surface(unsigned int size);

	// -------------------------------------------------------------------
	/** Loads a texture block from a LWO2 file.
	 *  @param size Maximum size to be read, in bytes.  
	 *  @param type Type of the texture block - PROC, GRAD or IMAP
	 */
	void LoadLWO2TextureBlock(uint32_t type, unsigned int size );

	// -------------------------------------------------------------------
	/** Loads an image map from a LWO2 file
	 *  @param size Maximum size to be read, in bytes.  
	 *  @param tex Texture object to be filled
	 */
	void LoadLWO2ImageMap(unsigned int size, LWO::Texture& tex );
	void LoadLWO2Gradient(unsigned int size, LWO::Texture& tex );
	void LoadLWO2Procedural(unsigned int size, LWO::Texture& tex );

	// loads the header - used by thethree functions above
	void LoadLWO2TextureHeader(unsigned int size, LWO::Texture& tex );

	// -------------------------------------------------------------------
	/** Loads the LWO tag list from the file
	 *  @param size Maximum size to be read, in bytes.  
	 */
	void LoadLWOTags(unsigned int size);

	// -------------------------------------------------------------------
	/** Load polygons from a POLS chunk
	 *  @param length Size of the chunk
	*/
	void LoadLWOPolygons(unsigned int length);

	// -------------------------------------------------------------------
	/** Load polygons from a PNTS chunk
	 *  @param length Size of the chunk
	*/
	void LoadLWOPoints(unsigned int length);


	// -------------------------------------------------------------------
	/** Count vertices and faces in a LWOB/LWO2 file
	*/
	void CountVertsAndFaces(unsigned int& verts, 
		unsigned int& faces,
		LE_NCONST uint16_t*& cursor, 
		const uint16_t* const end,
		unsigned int max = 0xffffffff);

	// -------------------------------------------------------------------
	/** Read vertices and faces in a LWOB/LWO2 file
	*/
	void CopyFaceIndices(LWO::FaceList::iterator& it,
		LE_NCONST uint16_t*& cursor, 
		const uint16_t* const end, 
		unsigned int max = 0xffffffff);

	// -------------------------------------------------------------------
	/** Resolve the tag and surface lists that have been loaded.
	* Generates the mMapping table.
	*/
	void ResolveTags();

	// -------------------------------------------------------------------
	/** Computes a proper texture form a procedural gradient
	 *  description.
	 *  @param grad Gradient description
     *  @param out List of output textures. The new texture should
	 *    be added to the list, if the conversion was successful.
	 *  @return true if successful
	*/
	bool ComputeGradientTexture(LWO::GradientInfo& grad,
		std::vector<aiTexture*>& out);

	// -------------------------------------------------------------------
	/** Parse a string from the current file position
	*/
	void ParseString(std::string& out,unsigned int max);

	// -------------------------------------------------------------------
	/** Adjust a texture path
	*/
	void AdjustTexturePath(std::string& out);

	// -------------------------------------------------------------------
	/** Convert a LWO surface description to an ASSIMP material
	*/
	void ConvertMaterial(const LWO::Surface& surf,MaterialHelper* pcMat);

	// -------------------------------------------------------------------
	/** Generate the final node graph
	 *  Unused nodes are deleted.
	 *  @param apcNodes Flat list of nodes
	*/
	void GenerateNodeGraph(std::vector<aiNode*>& apcNodes);

protected:

	/** true if the file is a LWO2 file*/
	bool mIsLWO2;

	/** Temporary list of layers from the file */
	LayerList* mLayers;

	/** Pointer to the current layer */
	LWO::Layer* mCurLayer;

	/** Temporary tag list from the file */
	TagList* mTags;

	/** Mapping table to convert from tag to surface indices.
	    0xffffffff indicates that a no corresponding surface is available */
	TagMappingTable* mMapping;

	/** Temporary surface list from the file */
	SurfaceList* mSurfaces;

	/** file buffer */
	LE_NCONST uint8_t* mFileBuffer;

	/** Size of the file, in bytes */
	unsigned int fileSize;

	/** Output scene */
	aiScene* pScene;

	/** Configuration option: X and Y size of gradient maps */
	unsigned int configGradientResX,configGradientResY;
};

} // end of namespace Assimp

#endif // AI_LWOIMPORTER_H_INCLUDED