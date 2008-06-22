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
//! @file Definition of HMP importer class
//!

#ifndef AI_HMPLOADER_H_INCLUDED
#define AI_HMPLOADER_H_INCLUDED

#include "BaseImporter.h"
#include "../include/aiTypes.h"
#include "../include/aiTexture.h"
#include "../include/aiMaterial.h"

struct aiNode;
#include "MDLLoader.h"

namespace Assimp
{
class MaterialHelper;

#define AI_HMP_MAGIC_NUMBER_BE_4	'HMP4'
#define AI_HMP_MAGIC_NUMBER_LE_4	'4PMH'

#define AI_HMP_MAGIC_NUMBER_BE_5	'HMP5'
#define AI_HMP_MAGIC_NUMBER_LE_5	'5PMH'

#define AI_HMP_MAGIC_NUMBER_BE_7	'HMP7'
#define AI_HMP_MAGIC_NUMBER_LE_7	'7PMH'

namespace HMP
{

// ---------------------------------------------------------------------------
/** Data structure for the header of a HMP5 file.
 *  This is also used by HMP4 and HMP7, but with modifications
*/
struct Header_HMP5
{
	int8_t	ident[4]; // "HMP5"
	int32_t		version;
	
	// ignored
	float	scale[3];
	float	scale_origin[3];
	float	boundingradius;
	
	//! Size of one triangle in x direction
	float	ftrisize_x;		
	//! Size of one triangle in y direction
	float	ftrisize_y;		
	//! Number of vertices in x direction
	float	fnumverts_x;	
							
	//! Number of skins in the file
	int32_t		numskins;

	// can ignore this?
	int32_t		skinwidth;
	int32_t		skinheight;

	//!Number of vertices in the file
	int32_t		numverts;

	// ignored and zero
	int32_t		numtris;

	//! only one supported ...
	int32_t		numframes;		

	//! Always 0 ...
	int32_t		num_stverts;	
	int32_t		flags;
	float	size;
};


// ---------------------------------------------------------------------------
/** Data structure for a terrain vertex in a HMP7 file 
*/
struct Vertex_HMP7
{
	uint16_t	 z;				
	int8_t normal_x,normal_y;
};

}; //! namespace HMP

// ---------------------------------------------------------------------------
/** Used to load 3D GameStudio HMP files (terrains)
*/
class HMPImporter : public MDLImporter
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	HMPImporter();

	/** Destructor, private as well */
	~HMPImporter();

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
		append.append("*.hmp");
	}

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. 
	* See BaseImporter::InternReadFile() for details
	*/
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);

protected:

	// -------------------------------------------------------------------
	/** Import a HMP4 file
	*/
	void InternReadFile_HMP4( );

	// -------------------------------------------------------------------
	/** Import a HMP5 file
	*/
	void InternReadFile_HMP5( );

	// -------------------------------------------------------------------
	/** Import a HMP7 file
	*/
	void InternReadFile_HMP7( );


	// -------------------------------------------------------------------
	/** Generate planar texture coordinates for a terrain
	 * \param width Width of the terrain, in vertices
	 * \param height Height of the terrain, in vertices
	*/
	void GenerateTextureCoords(const unsigned int width, 
		const unsigned int height);

	// -------------------------------------------------------------------
	/** Read the first skin from the file and skip all others ...
	 *  \param iNumSkins Number of skins in the file
	 *  \param szCursor Position of the first skin (offset 84)
	*/
	void ReadFirstSkin(unsigned int iNumSkins, const unsigned char* szCursor,
		const unsigned char** szCursorOut);

private:


};
}; // end of namespace Assimp

#endif // AI_HMPIMPORTER_H_INC