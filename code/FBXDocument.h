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
	struct ImportSettings;

	class PropertyTable;
	class Document;
	class Material;
	class Geometry;


/** Represents a delay-parsed FBX objects. Many objects in the scene
 *  are not needed by assimp, so it makes no sense to parse them
 *  upfront. */
class LazyObject
{
public:

	LazyObject(uint64_t id, const Element& element, const Document& doc);
	~LazyObject();

public:

	const Object* Get();

	template <typename T> 
	T* Get() {
		const Object* const ob = Get();
		return ob ? dynamic_cast<T*>(ob) : NULL;
	}

	uint64_t ID() const {
		return id;
	}

private:

	const Document& doc;
	const Element& element;
	boost::scoped_ptr<const Object> object;

	const uint64_t id;
};



/** Base class for in-memory (DOM) representations of FBX objects */
class Object
{
public:

	Object(uint64_t id, const Element& element, const std::string& name);
	virtual ~Object();

public:

	const Element& SourceElement() const {
		return element;
	}

	const std::string& Name() const {
		return name;
	}

	uint64_t ID() const {
		return id;
	}

protected:
	const Element& element;
	const std::string name;
	const uint64_t id;
};


/** DOM base class for FBX models */
class Model : public Object
{
public:

	Model(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~Model();

public:

	const std::string& Shading() const {
		return shading;
	}

	const std::string& Culling() const {
		return culling;
	}

	const PropertyTable& Props() const {
		ai_assert(props.get());
		return *props.get();
	}

	/** Get material links */
	const std::vector<const Material*>& GetMaterials() const {
		return materials;
	}


	/** Get geometry links */
	const std::vector<const Geometry*>& GetGeometry() const {
		return geometry;
	}

private:

	void ResolveLinks(const Element& element, const Document& doc);

private:

	std::vector<const Material*> materials;
	std::vector<const Geometry*> geometry;

	std::string shading;
	std::string culling;
	boost::shared_ptr<const PropertyTable> props;
};



/** DOM class for generic FBX textures */
class Texture : public Object
{
public:

	Texture(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~Texture();

public:

	const std::string& Type() const {
		return type;
	}

	const std::string& FileName() const {
		return fileName;
	}

	const std::string& RelativeFilename() const {
		return relativeFileName;
	}

	const std::string& AlphaSource() const {
		return alphaSource;
	}

	const aiVector2D& UVTranslation() const {
		return uvTrans;
	}

	const aiVector2D& UVScaling() const {
		return uvScaling;
	}

	const PropertyTable& Props() const {
		ai_assert(props.get());
		return *props.get();
	}

	// return a 4-tuple 
	const unsigned int* Crop() const {
		return crop;
	}

private:

	aiVector2D uvTrans;
	aiVector2D uvScaling;

	std::string type;
	std::string relativeFileName;
	std::string fileName;
	std::string alphaSource;
	boost::shared_ptr<const PropertyTable> props;

	unsigned int crop[4];
};


typedef std::fbx_unordered_map<std::string, const Texture*> TextureMap;


/** DOM class for generic FBX materials */
class Material : public Object
{
public:

	Material(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~Material();

public:

	const std::string& GetShadingModel() const {
		return shading;
	}

	bool IsMultilayer() const {
		return multilayer;
	}

	const PropertyTable& Props() const {
		ai_assert(props.get());
		return *props.get();
	}

	const TextureMap& Textures() const {
		return textures;
	}

private:

	std::string shading;
	bool multilayer;
	boost::shared_ptr<const PropertyTable> props;

	TextureMap textures;
};


/** DOM base class for all kinds of FBX geometry */
class Geometry : public Object
{
public:

	Geometry(uint64_t id, const Element& element, const std::string& name);
	~Geometry();
};


/** DOM class for FBX geometry of type "Mesh"*/
class MeshGeometry : public Geometry
{

public:

	MeshGeometry(uint64_t id, const Element& element, const std::string& name, const Document& doc);
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


	/** Get a UV coordinate slot, returns an empty array if
	 *  the requested slot does not exist. */
	std::string GetTextureCoordChannelName(unsigned int index) const {
		return index >= AI_MAX_NUMBER_OF_TEXTURECOORDS ? "" : uvNames[index];
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

	std::string uvNames[AI_MAX_NUMBER_OF_TEXTURECOORDS];
	std::vector<aiVector2D> uvs[AI_MAX_NUMBER_OF_TEXTURECOORDS];
	std::vector<aiColor4D> colors[AI_MAX_NUMBER_OF_COLOR_SETS];

	std::vector<unsigned int> mapping_counts;
	std::vector<unsigned int> mapping_offsets;
	std::vector<unsigned int> mappings;
};


/** Represents a link between two FBX objects. */
class Connection
{
public:

	Connection(uint64_t insertionOrder,  uint64_t src, uint64_t dest, const std::string& prop, const Document& doc);
	~Connection();

	// note: a connection ensures that the source and dest objects exist, but
	// not that they have DOM representations, so the return value of one of
	// these functions can still be NULL.
	const Object* SourceObject() const;
	const Object* DestinationObject() const;

	// return the name of the property the connection is attached to.
	// this is an empty string for object to object (OO) connections.
	const std::string& PropertyName() const {
		return prop;
	}

	uint64_t InsertionOrder() const {
		return insertionOrder;
	}

	int CompareTo(const Connection* c) const {
		// note: can't subtract because this would overflow uint64_t
		if(InsertionOrder() > c->InsertionOrder()) {
			return 1;
		}
		else if(InsertionOrder() < c->InsertionOrder()) {
			return -1;
		}
		return 0;
	}

	bool Compare(const Connection* c) const {
		return InsertionOrder() < c->InsertionOrder();
	}

public:

	uint64_t insertionOrder;
	const std::string prop;

	uint64_t src, dest;
	const Document& doc;
};


	// XXX again, unique_ptr would be useful. shared_ptr is too
	// bloated since the objects have a well-defined single owner
	// during their entire lifetime (Document). FBX files have
	// up to many thousands of objects (most of which we never use),
	// so the memory overhead for them should be kept at a minimum.
	typedef std::map<uint64_t, LazyObject*> ObjectMap; 
	typedef std::fbx_unordered_map<std::string, boost::shared_ptr<const PropertyTable> > PropertyTemplateMap;


	typedef std::multimap<uint64_t, const Connection*> ConnectionMap;


/** DOM root for a FBX file */
class Document 
{
public:

	Document(const Parser& parser, const ImportSettings& settings);
	~Document();

public:

	LazyObject* GetObject(uint64_t id) const;


	unsigned int FBXVersion() const {
		return fbxVersion;
	}

	const std::string& Creator() const {
		return creator;
	}

	// elements (in this order): Uear, Month, Day, Hour, Second, Millisecond
	const unsigned int* CreationTimeStamp() const {
		return creationTimeStamp;
	}

	const PropertyTemplateMap& Templates() const {
		return templates;
	}

	const ObjectMap& Objects() const {
		return objects;
	}

	const ImportSettings& Settings() const {
		return settings;
	}

	const ConnectionMap& ConnectionsBySource() const {
		return src_connections;
	}

	const ConnectionMap& ConnectionsByDestination() const {
		return dest_connections;
	}

	std::vector<const Connection*> GetConnectionsBySourceSequenced(uint64_t source) const;
	std::vector<const Connection*> GetConnectionsByDestinationSequenced(uint64_t dest) const;

private:

	void ReadHeader();
	void ReadObjects();
	void ReadPropertyTemplates();
	void ReadConnections();

private:

	const ImportSettings& settings;

	ObjectMap objects;
	const Parser& parser;

	PropertyTemplateMap templates;
	ConnectionMap src_connections;
	ConnectionMap dest_connections;

	unsigned int fbxVersion;
	std::string creator;
	unsigned int creationTimeStamp[7];

};

}
}

#endif
