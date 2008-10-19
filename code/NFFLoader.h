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

/** @file Declaration of the NFF importer class. */
#ifndef AI_NFFLOADER_H_INCLUDED
#define AI_NFFLOADER_H_INCLUDED

#include "BaseImporter.h"
#include <vector>

#include "../include/aiTypes.h"

namespace Assimp	{

// ---------------------------------------------------------------------------
/** NFF (Neutral File Format) Importer class
*/
class NFFImporter : public BaseImporter
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	NFFImporter();

	/** Destructor, private as well */
	~NFFImporter();

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
		append.append("*.nff;*.enff");
	}

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. 
	* See BaseImporter::InternReadFile() for details
	*/
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);

private:


	// describes face material properties
	struct ShadingInfo
	{
		ShadingInfo()
			: color(0.6f,0.6f,0.6f)
			, diffuse   (1.f)
			, specular  (1.f)
			, ambient   (0.1f)
			, refracti  (1.f)
		{}

		aiColor3D color;
		float diffuse, specular, ambient, refracti;

		std::string texFile;

		// shininess is ignored for the moment
		bool operator == (const ShadingInfo& other) const
		{
			return color == other.color		&& 
				diffuse  == other.diffuse	&&
				specular == other.specular	&&
				ambient  == other.ambient	&&
				refracti == other.refracti  &&
				texFile  == other.texFile;
		}
	};

	// describes a NFF light source
	struct Light
	{
		Light()
			: color		(1.f,1.f,1.f)
			, intensity	(1.f)
		{}

		aiVector3D position;
		float intensity;
		aiColor3D color;
	};

	enum PatchType
	{
		PatchType_Simple = 0x0,
		PatchType_Normals = 0x1,
		PatchType_UVAndNormals = 0x2
	};

	// describes a NFF mesh
	struct MeshInfo
	{
		MeshInfo(PatchType _pType, bool bL = false)
			: pType(_pType)
			, bLocked(bL)
		{
			name[0] = '\0'; // by default meshes are unnamed
		}

		ShadingInfo shader;
		PatchType pType;
		bool bLocked;

		// for spheres, cones and cylinders: center point of the object
		aiVector3D center, radius;

		char name[128];

		std::vector<aiVector3D> vertices, normals, uvs;
		std::vector<aiColor4D>  colors; // for NFF2
		std::vector<unsigned int> faces;
	};
};

} // end of namespace Assimp

#endif // AI_NFFIMPORTER_H_IN
