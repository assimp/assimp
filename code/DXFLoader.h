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

/** @file Declaration of the .dxf importer class. */
#ifndef AI_DXFLOADER_H_INCLUDED
#define AI_DXFLOADER_H_INCLUDED

#include "BaseImporter.h"

namespace Assimp	{

// ---------------------------------------------------------------------------
/** DXF importer class
*/
class DXFImporter : public BaseImporter
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	DXFImporter();

	/** Destructor, private as well */
	~DXFImporter();


	// describes a single layer in the DXF file
	struct LayerInfo
	{
		LayerInfo()
		{
			name[0] = '\0';
		}

		char name[4096];

		// face buffer - order is x,y,z v1,v2,v3,v4 
		// if v2 = v3:    line
		// elsif v3 = v2: triangle
		// else: polygon
		std::vector<aiVector3D> vPositions;
		std::vector<aiColor4D>  vColors;
	};


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
		append.append("*.dxf");
	}

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. 
	 * See BaseImporter::InternReadFile() for details
	 */
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);

	// -------------------------------------------------------------------
	/** Get the next line from the file.
	 *  @return false if the end of the file was reached
	 */
	bool GetNextLine();

	// -------------------------------------------------------------------
	/** Get the next token (group code + data line) from the file.
	 *  @return false if the end of the file was reached
	 */
	bool GetNextToken();

	// -------------------------------------------------------------------
	/** Parses the ENTITIES section in the file
	 *  @return false if the end of the file was reached
	 */
	bool ParseEntities();

	// -------------------------------------------------------------------
	/** Parses a 3DFACE section in the file
	 *  @return false if the end of the file was reached
	 */
	bool Parse3DFace();

	// -------------------------------------------------------------------
	/** Parses a POLYLINE section in the file
	 *  @return false if the end of the file was reached
	 */
	bool ParsePolyLine();

	// -------------------------------------------------------------------
	/** Sets the current layer - cursor must point to the name of it.
	 *  @param out Receives a handle to the layer
	 */
	void SetLayer(LayerInfo*& out);

	// -------------------------------------------------------------------
	/** Creates a default layer.
	 *  @param out Receives a handle to the default layer
	 */
	void SetDefaultLayer(LayerInfo*& out);

	// -------------------------------------------------------------------
	/** Parses a VERTEX element in a POLYLINE/POLYFACE
	 *  @param out Receives the output vertex. 
	 *  @param clr Receives the output vertex color - won't be modified
	 *    if it is not existing. 
	 *  @param outIdx Receives the output vertex indices, if present.
	 *    Wont't be modified otherwise. Size must be at least 4.
	 *  @return false if the end of the file was reached
	 */
	bool ParsePolyLineVertex(aiVector3D& out, aiColor4D& clr, 
		unsigned int* outIdx);

private:

	// points to the next section 
	const char* buffer;

	// specifies the current group code
	int groupCode;

	// contains the current data line
	char cursor[4096];

	// specifies whether the next call to GetNextToken()
	// should return the current token a second time
	bool bRepeat;

	// list of all loaded layers
	std::vector<LayerInfo> mLayers;
	LayerInfo* mDefaultLayer;
};

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC
