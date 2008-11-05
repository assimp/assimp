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

/** @file Declaration of the .irrMesh (Irrlight Engine Mesh Format)
    importer class. */
#ifndef AI_IRRMESHLOADER_H_INCLUDED
#define AI_IRRMESHLOADER_H_INCLUDED

#include "./irrXML/irrXMLWrapper.h"
#include "BaseImporter.h"

namespace Assimp	{

// Default: 0 = solid, one texture
#define AI_IRRMESH_MAT_solid_2layer			0x10000

// Transparency flags
#define AI_IRRMESH_MAT_trans_vertex_alpha	0x1
#define AI_IRRMESH_MAT_trans_add			0x2

// Lightmapping flags
#define AI_IRRMESH_MAT_lightmap				0x2 
#define AI_IRRMESH_MAT_lightmap_m2			(AI_IRRMESH_MAT_lightmap|0x4)
#define AI_IRRMESH_MAT_lightmap_m4			(AI_IRRMESH_MAT_lightmap|0x8)
#define AI_IRRMESH_MAT_lightmap_light		(AI_IRRMESH_MAT_lightmap|0x10)
#define AI_IRRMESH_MAT_lightmap_light_m2	(AI_IRRMESH_MAT_lightmap|0x20)
#define AI_IRRMESH_MAT_lightmap_light_m4	(AI_IRRMESH_MAT_lightmap|0x40)
#define AI_IRRMESH_MAT_lightmap_add			(AI_IRRMESH_MAT_lightmap|0x80)

// Standard NormalMap (or Parallax map, they're treated equally)
#define AI_IRRMESH_MAT_normalmap_solid		(0x100)

// Normal map combined with vertex alpha
#define AI_IRRMESH_MAT_normalmap_tva	\
	(AI_IRRMESH_MAT_normalmap_solid | AI_IRRMESH_MAT_trans_vertex_alpha)

// Normal map combined with additive transparency
#define AI_IRRMESH_MAT_normalmap_ta		\
	(AI_IRRMESH_MAT_normalmap_solid | AI_IRRMESH_MAT_trans_add)

// Special flag. It indicates a second texture has been found
// Its type depends ... either a normal textue or a normal map
#define AI_IRRMESH_EXTRA_2ND_TEXTURE		0x100000


// ---------------------------------------------------------------------------
/** Base class for the Irr and IrrMesh importers
 */
class IrrlichtBase
{
protected:

	template <class T>
	struct Property
	{
		std::string name;
		T value;
	};

	typedef Property<uint32_t>		HexProperty;
	typedef Property<std::string>	StringProperty;
	typedef Property<bool>			BoolProperty;
	typedef Property<float>			FloatProperty;
	typedef Property<aiVector3D>	VectorProperty;
	typedef Property<int>			IntProperty;

	/** XML reader instance
	 */
	IrrXMLReader* reader;


	// -------------------------------------------------------------------
	/** Parse a material description from the XML
	 *  @return The created material
	 *  @param matFlags Receives AI_IRRMESH_MAT_XX flags
	 */
	aiMaterial* ParseMaterial(unsigned int& matFlags);


	// -------------------------------------------------------------------
	/** Read a property of the specified type from the current XML element.
	 *  @param out Recives output data
	 */
	void ReadHexProperty    (HexProperty&    out);
	void ReadStringProperty (StringProperty& out);
	void ReadBoolProperty   (BoolProperty&   out);
	void ReadFloatProperty  (FloatProperty&  out);
	void ReadVectorProperty (VectorProperty&  out);
	void ReadIntProperty    (IntProperty&    out);
};

// ---------------------------------------------------------------------------
/** IrrMesh importer class.
 *
 * IrrMesh is the native file format of the Irrlight engine and its editor
 * irrEdit. As IrrEdit itself is capable of importing quite many file formats,
 * it might be a good file format for data exchange.
 */
class IRRMeshImporter : public BaseImporter, public IrrlichtBase
{
	friend class Importer;

protected:
	/** Constructor to be privately used by Importer */
	IRRMeshImporter();

	/** Destructor, private as well */
	~IRRMeshImporter();

public:

	// -------------------------------------------------------------------
	/** Returns whether the class can handle the format of the given file. 
	 *  See BaseImporter::CanRead() for details.	
	 */
	bool CanRead( const std::string& pFile, IOSystem* pIOHandler) const;

protected:

	// -------------------------------------------------------------------
	/** Called by Importer::GetExtensionList() for each loaded importer.
	 * See BaseImporter::GetExtensionList() for details
	 */
	void GetExtensionList(std::string& append)
	{

		/*  NOTE: The file extenxsion .xml is too generic. We'll 
		 *  need to open the file in CanRead() and check whether it is 
		 *  a real irrlicht file
		 */

		append.append("*.xml;*.irrmesh");
	}

	// -------------------------------------------------------------------
	/** Imports the given file into the given scene structure. 
	 * See BaseImporter::InternReadFile() for details
	 */
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);

};

} // end of namespace Assimp

#endif // AI_IRRMESHIMPORTER_H_INC
