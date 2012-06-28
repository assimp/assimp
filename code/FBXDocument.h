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

/** @file  FBXDocument.h
 *  @brief FBX DOM
 */
#ifndef INCLUDED_AI_FBX_DOCUMENT_H
#define INCLUDED_AI_FBX_DOCUMENT_H

#include <vector>
#include <map>
#include <string>

namespace Assimp {
namespace FBX {

	class Parser;
	class Object;


/** Represents a delay-parsed FBX objects. Many objects in the scene
 *  are not needed by assimp, so it makes no sense to parse them
 *  upfront. */
class LazyObject
{
public:

	LazyObject(const Element& element);
	~LazyObject();

public:

	const Object* Get();

	template <typename T> 
	T* Get() {
		const Object* const ob = Get();
		return ob ? dynamic_cast<T*>(ob) : NULL;
	}

private:

	const Element& element;
	boost::scoped_ptr<const Object> object;
};



/** Base class for in-memory (DOM) representations of FBX objects */
class Object
{
public:

	Object(const Element& element, const std::string& name);
	~Object();

public:

protected:
	const Element& element;
	const std::string name;
};


/** DOM base class for all kinds of FBX geometry */
class Geometry : public Object
{
public:

	Geometry(const Element& element, const std::string& name);
	~Geometry();
};


/** DOM class for FBX geometry of type "Mesh"*/
class MeshGeometry : public Geometry
{

public:

	MeshGeometry(const Element& element, const std::string& name);
	~MeshGeometry();

public:

	/** Get a list of all vertex points, non-unique*/
	const std::vector<aiVector3D>& GetVertices() const {
		return vertices;
	}

	/** Get a list of all vertex normals or an empty array if
	 *  no normals are specified. */
	const std::vector<aiVector3D>& GetNormals() const {
		return normals;
	}

	/** Get a list of all vertex tangents or an empty array
	 *  if no tangents are specified */
	const std::vector<aiVector3D>& GetTangents() const {
		return tangents;
	}

	/** Get a list of all vertex binormals or an empty array
	 *  if no binormals are specified */
	const std::vector<aiVector3D>& GetBinormals() const {
		return binormals;
	}
	
	/** Return list of faces - each entry denotes a face and specifies
	 *  how many vertices it has. Vertices are taken from the 
	 *  vertex data arrays in sequential order. */
	const std::vector<unsigned int>& GetFaceIndexCounts() const {
		return faces;
	}

	/** Get a UV coordinate slot, returns an empty array if
	 *  the requested slot does not exist. */
	const std::vector<aiVector2D>& GetTextureCoords(unsigned int index) const {
		static const std::vector<aiVector2D> empty;
		return index >= AI_MAX_NUMBER_OF_TEXTURECOORDS ? empty : uvs[index];
	}

	/** Get a vertex color coordinate slot, returns an empty array if
	 *  the requested slot does not exist. */
	const std::vector<aiColor4D>& GetVertexColors(unsigned int index) const {
		static const std::vector<aiColor4D> empty;
		return index >= AI_MAX_NUMBER_OF_COLOR_SETS ? empty : colors[index];
	}
	
	
	/** Get per-face-vertex material assignments */
	const std::vector<unsigned int>& GetMaterialIndices() const {
		return materials;
	}

public:

private:

	void ReadLayer(const Scope& layer);
	void ReadLayerElement(const Scope& layerElement);
	void ReadVertexData(const std::string& type, int index, const Scope& source);

	void ReadVertexDataUV(std::vector<aiVector2D>& uv_out, const Scope& source, 
		const std::string& MappingInformationType,
		const std::string& ReferenceInformationType);

	void ReadVertexDataNormals(std::vector<aiVector3D>& normals_out, const Scope& source, 
		const std::string& MappingInformationType,
		const std::string& ReferenceInformationType);

	void ReadVertexDataColors(std::vector<aiColor4D>& colors_out, const Scope& source, 
		const std::string& MappingInformationType,
		const std::string& ReferenceInformationType);

	void ReadVertexDataTangents(std::vector<aiVector3D>& tangents_out, const Scope& source, 
		const std::string& MappingInformationType,
		const std::string& ReferenceInformationType);

	void ReadVertexDataBinormals(std::vector<aiVector3D>& binormals_out, const Scope& source, 
		const std::string& MappingInformationType,
		const std::string& ReferenceInformationType);

	void ReadVertexDataMaterials(std::vector<unsigned int>& materials_out, const Scope& source, 
		const std::string& MappingInformationType,
		const std::string& ReferenceInformationType);

private:

	// cached data arrays
	std::vector<unsigned int> materials;
	std::vector<aiVector3D> vertices;
	std::vector<unsigned int> faces;
	std::vector<aiVector3D> tangents;
	std::vector<aiVector3D> binormals;
	std::vector<aiVector3D> normals;
	std::vector<aiVector2D> uvs[AI_MAX_NUMBER_OF_TEXTURECOORDS];
	std::vector<aiColor4D> colors[AI_MAX_NUMBER_OF_COLOR_SETS];

	std::vector<unsigned int> mapping_counts;
	std::vector<unsigned int> mapping_offsets;
	std::vector<unsigned int> mappings;
};

	// XXX again, unique_ptr would be useful. shared_ptr is too
	// bloated since the objects have a well-defined single owner
	// during their entire lifetime (Document). FBX files have
	// up to many thousands of objects (most of which we never use),
	// so the memory overhead for them should be kept at a minimum.
	typedef std::map<uint64_t, LazyObject*> ObjectMap; 


/** DOM root for a FBX file */
class Document 
{
public:

	Document(const Parser& parser);
	~Document();

public:

	const ObjectMap& Objects() const {
		return objects;
	}

private:

	ObjectMap objects;
	const Parser& parser;
};

}
}

#endif
