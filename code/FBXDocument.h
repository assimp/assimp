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

#include "FBXProperties.h"

namespace Assimp {
namespace FBX {

	class Parser;
	class Object;
	struct ImportSettings;

	class PropertyTable;
	class Document;
	class Material;
	class Geometry;

	class AnimationCurve;
	class AnimationCurveNode;
	class AnimationLayer;
	class AnimationStack;

	class Skin;
	class Cluster;


/** Represents a delay-parsed FBX objects. Many objects in the scene
 *  are not needed by assimp, so it makes no sense to parse them
 *  upfront. */
class LazyObject
{
public:

	LazyObject(uint64_t id, const Element& element, const Document& doc);
	~LazyObject();

public:

	const Object* Get(bool dieOnError = false);

	template <typename T> 
	const T* Get(bool dieOnError = false) {
		const Object* const ob = Get(dieOnError);
		return ob ? dynamic_cast<const T*>(ob) : NULL;
	}

	uint64_t ID() const {
		return id;
	}

	bool IsBeingConstructed() const {
		return (flags & BEING_CONSTRUCTED) != 0;
	}

	bool FailedToConstruct() const {
		return (flags & FAILED_TO_CONSTRUCT) != 0;
	}

	const Element& GetElement() const {
		return element;
	}

	const Document& GetDocument() const {
		return doc;
	}

private:

	const Document& doc;
	const Element& element;
	boost::scoped_ptr<const Object> object;

	const uint64_t id;

	enum Flags {
		BEING_CONSTRUCTED = 0x1,
		FAILED_TO_CONSTRUCT = 0x2
	};

	unsigned int flags;
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



/** DOM class for generic FBX NoteAttribute blocks. NoteAttribute's just hold a property table,
 *  fixed members are added by deriving classes. */
class NodeAttribute : public Object
{
public:

	NodeAttribute(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~NodeAttribute();

public:

	const PropertyTable& Props() const {
		ai_assert(props.get());
		return *props.get();
	}

private:

	boost::shared_ptr<const PropertyTable> props;
};


/** DOM base class for FBX camera settings attached to a node */
class CameraSwitcher : public NodeAttribute
{
public:

	CameraSwitcher(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~CameraSwitcher();

public:

	int CameraID() const {
		return cameraId;
	}

	const std::string& CameraName() const {
		return cameraName;
	}


	const std::string& CameraIndexName() const {
		return cameraIndexName;
	}

private:

	int cameraId;
	std::string cameraName;
	std::string cameraIndexName;
};


#define fbx_stringize(a) #a

#define fbx_simple_property(name, type, default_value) \
	type name() const { \
		return PropertyGet<type>(Props(), fbx_stringize(name), (default_value)); \
	}

// XXX improve logging
#define fbx_simple_enum_property(name, type, default_value) \
	type name() const { \
		const int ival = PropertyGet<int>(Props(), fbx_stringize(name), static_cast<int>(default_value)); \
		if (ival < 0 || ival >= AI_CONCAT(type, _MAX)) { \
			ai_assert(static_cast<int>(default_value) >= 0 && static_cast<int>(default_value) < AI_CONCAT(type, _MAX)); \
			return static_cast<type>(default_value); \
		} \
		return static_cast<type>(ival); \
}



/** DOM base class for FBX cameras attached to a node */
class Camera : public NodeAttribute
{
public:

	Camera(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~Camera();

public:

	fbx_simple_property(Position, aiVector3D, aiVector3D(0,0,0));
	fbx_simple_property(UpVector, aiVector3D, aiVector3D(0,1,0));
	fbx_simple_property(InterestPosition, aiVector3D, aiVector3D(0,0,0));

	fbx_simple_property(AspectWidth, float, 1.0f);
	fbx_simple_property(AspectHeight, float, 1.0f);
	fbx_simple_property(FilmWidth, float, 1.0f);
	fbx_simple_property(FilmHeight, float, 1.0f);

	fbx_simple_property(FilmAspectRatio, float, 1.0f);
	fbx_simple_property(ApertureMode, int, 0);

	fbx_simple_property(FieldOfView, float, 1.0f);
	fbx_simple_property(FocalLength, float, 1.0f);

private:
};


/** DOM base class for FBX null markers attached to a node */
class Null : public NodeAttribute
{
public:

	Null(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~Null();
};


/** DOM base class for FBX limb node markers attached to a node */
class LimbNode : public NodeAttribute
{
public:

	LimbNode(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~LimbNode();
};


/** DOM base class for FBX lights attached to a node */
class Light : public NodeAttribute
{
public:

	Light(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~Light();

public:

	enum Type
	{
		Type_Point,
		Type_Directional,
		Type_Spot,
		Type_Area,
		Type_Volume,

		Type_MAX // end-of-enum sentinel
	};

	enum Decay
	{
		Decay_None,
		Decay_Linear,
		Decay_Quadratic,
		Decay_Cubic,

		Decay_MAX // end-of-enum sentinel
	};

public:

	fbx_simple_property(Color, aiVector3D, aiVector3D(1,1,1));
	fbx_simple_enum_property(LightType, Type, 0);
	fbx_simple_property(CastLightOnObject, bool, false);
	fbx_simple_property(DrawVolumetricLight, bool, true);
	fbx_simple_property(DrawGroundProjection, bool, true);
	fbx_simple_property(DrawFrontFacingVolumetricLight, bool, false);
	fbx_simple_property(Intensity, float, 1.0f);
	fbx_simple_property(InnerAngle, float, 0.0f);
	fbx_simple_property(OuterAngle, float, 45.0f);
	fbx_simple_property(Fog, int, 50);
	fbx_simple_enum_property(DecayType, Decay, 0);
	fbx_simple_property(DecayStart, int, 0);
	fbx_simple_property(FileName, std::string, "");

	fbx_simple_property(EnableNearAttenuation, bool, false);
	fbx_simple_property(NearAttenuationStart, float, 0.0f);
	fbx_simple_property(NearAttenuationEnd, float, 0.0f);
	fbx_simple_property(EnableFarAttenuation, bool, false);
	fbx_simple_property(FarAttenuationStart, float, 0.0f);
	fbx_simple_property(FarAttenuationEnd, float, 0.0f);

	fbx_simple_property(CastShadows, bool, true);
	fbx_simple_property(ShadowColor, aiVector3D, aiVector3D(0,0,0));

	fbx_simple_property(AreaLightShape, int, 0);

	fbx_simple_property(LeftBarnDoor, float, 20.0f);
	fbx_simple_property(RightBarnDoor, float, 20.0f);
	fbx_simple_property(TopBarnDoor, float, 20.0f);
	fbx_simple_property(BottomBarnDoor, float, 20.0f);
	fbx_simple_property(EnableBarnDoor, bool, true);


private:
};


/** DOM base class for FBX models (even though its semantics are more "node" than "model" */
class Model : public Object
{
public:

	Model(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~Model();

public:

	enum RotOrder
	{ 
		RotOrder_EulerXYZ = 0, 
		RotOrder_EulerXZY, 
		RotOrder_EulerYZX, 
		RotOrder_EulerYXZ, 
		RotOrder_EulerZXY, 
		RotOrder_EulerZYX,

		RotOrder_SphericXYZ,

		RotOrder_MAX // end-of-enum sentinel
	};


	enum TransformInheritance
	{
		TransformInheritance_RrSs = 0,
		TransformInheritance_RSrs,
		TransformInheritance_Rrs,

		TransformInheritance_MAX // end-of-enum sentinel
	};

public:

	fbx_simple_property(QuaternionInterpolate, int, 0);

	fbx_simple_property(RotationOffset, aiVector3D, aiVector3D());
	fbx_simple_property(RotationPivot, aiVector3D, aiVector3D());
	fbx_simple_property(ScalingOffset, aiVector3D, aiVector3D());
	fbx_simple_property(ScalingPivot, aiVector3D, aiVector3D());
	fbx_simple_property(TranslationActive, bool, false);

	fbx_simple_property(TranslationMin, aiVector3D, aiVector3D());
	fbx_simple_property(TranslationMax, aiVector3D, aiVector3D());

	fbx_simple_property(TranslationMinX, bool, false);
	fbx_simple_property(TranslationMaxX, bool, false);
	fbx_simple_property(TranslationMinY, bool, false);
	fbx_simple_property(TranslationMaxY, bool, false);
	fbx_simple_property(TranslationMinZ, bool, false);
	fbx_simple_property(TranslationMaxZ, bool, false);

	fbx_simple_enum_property(RotationOrder, RotOrder, 0);
	fbx_simple_property(RotationSpaceForLimitOnly, bool, false);
	fbx_simple_property(RotationStiffnessX, float, 0.0f);
	fbx_simple_property(RotationStiffnessY, float, 0.0f);
	fbx_simple_property(RotationStiffnessZ, float, 0.0f);
	fbx_simple_property(AxisLen, float, 0.0f);

	fbx_simple_property(PreRotation, aiVector3D, aiVector3D());
	fbx_simple_property(PostRotation, aiVector3D, aiVector3D());
	fbx_simple_property(RotationActive, bool, false);

	fbx_simple_property(RotationMin, aiVector3D, aiVector3D());
	fbx_simple_property(RotationMax, aiVector3D, aiVector3D());

	fbx_simple_property(RotationMinX, bool, false);
	fbx_simple_property(RotationMaxX, bool, false);
	fbx_simple_property(RotationMinY, bool, false);
	fbx_simple_property(RotationMaxY, bool, false);
	fbx_simple_property(RotationMinZ, bool, false);
	fbx_simple_property(RotationMaxZ, bool, false);
	fbx_simple_enum_property(InheritType, TransformInheritance, 0);

	fbx_simple_property(ScalingActive, bool, false);
	fbx_simple_property(ScalingMin, aiVector3D, aiVector3D());
	fbx_simple_property(ScalingMax, aiVector3D, aiVector3D(1.f,1.f,1.f));
	fbx_simple_property(ScalingMinX, bool, false);
	fbx_simple_property(ScalingMaxX, bool, false);
	fbx_simple_property(ScalingMinY, bool, false);
	fbx_simple_property(ScalingMaxY, bool, false);
	fbx_simple_property(ScalingMinZ, bool, false);
	fbx_simple_property(ScalingMaxZ, bool, false);

	fbx_simple_property(GeometricTranslation, aiVector3D, aiVector3D());
	fbx_simple_property(GeometricRotation, aiVector3D, aiVector3D());
	fbx_simple_property(GeometricScaling, aiVector3D, aiVector3D(1.f, 1.f, 1.f));

	fbx_simple_property(MinDampRangeX, float, 0.0f);
	fbx_simple_property(MinDampRangeY, float, 0.0f);
	fbx_simple_property(MinDampRangeZ, float, 0.0f);
	fbx_simple_property(MaxDampRangeX, float, 0.0f);
	fbx_simple_property(MaxDampRangeY, float, 0.0f);
	fbx_simple_property(MaxDampRangeZ, float, 0.0f);

	fbx_simple_property(MinDampStrengthX, float, 0.0f);
	fbx_simple_property(MinDampStrengthY, float, 0.0f);
	fbx_simple_property(MinDampStrengthZ, float, 0.0f);
	fbx_simple_property(MaxDampStrengthX, float, 0.0f);
	fbx_simple_property(MaxDampStrengthY, float, 0.0f);
	fbx_simple_property(MaxDampStrengthZ, float, 0.0f);

	fbx_simple_property(PreferredAngleX, float, 0.0f);
	fbx_simple_property(PreferredAngleY, float, 0.0f);
	fbx_simple_property(PreferredAngleZ, float, 0.0f);

	fbx_simple_property(Show, bool, true);
	fbx_simple_property(LODBox, bool, false);
	fbx_simple_property(Freeze, bool, false);

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


	/** Get node attachments */
	const std::vector<const NodeAttribute*>& GetAttributes() const {
		return attributes;
	}

public:

	/** convenience method to check if the node has a Null node marker */
	bool IsNull() const;


private:

	void ResolveLinks(const Element& element, const Document& doc);

private:

	std::vector<const Material*> materials;
	std::vector<const Geometry*> geometry;
	std::vector<const NodeAttribute*> attributes;

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

	Geometry(uint64_t id, const Element& element, const std::string& name, const Document& doc);
	~Geometry();

public:

	/** Get the Skin attached to this geometry or NULL */
	const Skin* const DeformerSkin() const {
		return skin;
	}

private:

	const Skin* skin;
};


typedef std::vector<int> MatIndexArray;


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
	const MatIndexArray& GetMaterialIndices() const {
		return materials;
	}


	/** Convert from a fbx file vertex index (for example from a #Cluster weight) or NULL
	  * if the vertex index is not valid. */
	const unsigned int* ToOutputVertexIndex(unsigned int in_index, unsigned int& count) const {
		if(in_index >= mapping_counts.size()) {
			return NULL;
		}

		ai_assert(mapping_counts.size() == mapping_offsets.size());
		count = mapping_counts[in_index];

		ai_assert(count != 0);
		ai_assert(mapping_offsets[in_index] + count <= mappings.size());

		return &mappings[mapping_offsets[in_index]];
	}


	/** Determine the face to which a particular output vertex index belongs.
	 *  This mapping is always unique. */
	unsigned int FaceForVertexIndex(unsigned int in_index) const {
		ai_assert(in_index < vertices.size());
	
		// in the current conversion pattern this will only be needed if
		// weights are present, so no need to always pre-compute this table
		if (facesVertexStartIndices.empty()) {
			facesVertexStartIndices.resize(faces.size() + 1, 0);

			std::partial_sum(faces.begin(), faces.end(), facesVertexStartIndices.begin() + 1);
			facesVertexStartIndices.pop_back();
		}

		ai_assert(facesVertexStartIndices.size() == faces.size());
		const std::vector<unsigned int>::iterator it = std::upper_bound(
			facesVertexStartIndices.begin(),
			facesVertexStartIndices.end(),
			in_index
		);

		return static_cast<unsigned int>(std::distance(facesVertexStartIndices.begin(), it - 1)); 
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

	void ReadVertexDataMaterials(MatIndexArray& materials_out, const Scope& source, 
		const std::string& MappingInformationType,
		const std::string& ReferenceInformationType);

private:

	// cached data arrays
	MatIndexArray materials;
	std::vector<aiVector3D> vertices;
	std::vector<unsigned int> faces;
	mutable std::vector<unsigned int> facesVertexStartIndices;
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

typedef std::vector<uint64_t> KeyTimeList;
typedef std::vector<float> KeyValueList;

/** Represents a FBX animation curve (i.e. a 1-dimensional set of keyframes and values therefor) */
class AnimationCurve : public Object
{
public:

	AnimationCurve(uint64_t id, const Element& element, const std::string& name, const Document& doc);
	~AnimationCurve();

public:

	/** get list of keyframe positions (time).
	 *  Invariant: |GetKeys()| > 0 */
	const KeyTimeList& GetKeys() const {
		return keys;
	}


	/** get list of keyframe values. 
	  * Invariant: |GetKeys()| == |GetValues()| && |GetKeys()| > 0*/
	const KeyValueList& GetValues() const {
		return values;
	}


	const std::vector<float>& GetAttributes() const {
		return attributes;
	}

	const std::vector<unsigned int>& GetFlags() const {
		return flags;
	}

private:

	KeyTimeList keys;
	KeyValueList values;
	std::vector<float> attributes;
	std::vector<unsigned int> flags;
};

// property-name -> animation curve
typedef std::map<std::string, const AnimationCurve*> AnimationCurveMap;


/** Represents a FBX animation curve (i.e. a mapping from single animation curves to nodes) */
class AnimationCurveNode : public Object
{
public:

	/* the optional whitelist specifies a list of property names for which the caller
	wants animations for. If the curve node does not match one of these, std::range_error
	will be thrown. */
	AnimationCurveNode(uint64_t id, const Element& element, const std::string& name, const Document& doc,
		const char* const * target_prop_whitelist = NULL, size_t whitelist_size = 0);

	~AnimationCurveNode();

public:

	const PropertyTable& Props() const {
		ai_assert(props.get());
		return *props.get();
	}


	const AnimationCurveMap& Curves() const;

	/** Object the curve is assigned to, this can be NULL if the
	 *  target object has no DOM representation or could not
	 *  be read for other reasons.*/
	const Object* Target() const {
		return target;
	}

	const Model* TargetAsModel() const {
		return dynamic_cast<const Model*>(target);
	}

	const NodeAttribute* TargetAsNodeAttribute() const {
		return dynamic_cast<const NodeAttribute*>(target);
	}

	/** Property of Target() that is being animated*/
	const std::string& TargetProperty() const {
		return prop;
	}

private:

	const Object* target;
	boost::shared_ptr<const PropertyTable> props;
	mutable AnimationCurveMap curves;

	std::string prop;
	const Document& doc;
};

typedef std::vector<const AnimationCurveNode*> AnimationCurveNodeList;


/** Represents a FBX animation layer (i.e. a list of node animations) */
class AnimationLayer : public Object
{
public:

	
	AnimationLayer(uint64_t id, const Element& element, const std::string& name, const Document& doc);
	~AnimationLayer();

public:

	const PropertyTable& Props() const {
		ai_assert(props.get());
		return *props.get();
	}

	/* the optional whitelist specifies a list of property names for which the caller
	wants animations for. Curves not matching this list will not be added to the
	animation layer. */
	AnimationCurveNodeList Nodes(const char* const * target_prop_whitelist = NULL, size_t whitelist_size = 0) const;

private:

	boost::shared_ptr<const PropertyTable> props;
	const Document& doc;
};


typedef std::vector<const AnimationLayer*> AnimationLayerList;


/** Represents a FBX animation stack (i.e. a list of animation layers) */
class AnimationStack : public Object
{
public:

	AnimationStack(uint64_t id, const Element& element, const std::string& name, const Document& doc);
	~AnimationStack();

public:

	fbx_simple_property(LocalStart, uint64_t, 0L);
	fbx_simple_property(LocalStop, uint64_t, 0L);
	fbx_simple_property(ReferenceStart, uint64_t, 0L);
	fbx_simple_property(ReferenceStop, uint64_t, 0L);



	const PropertyTable& Props() const {
		ai_assert(props.get());
		return *props.get();
	}


	const AnimationLayerList& Layers() const {
		return layers;
	}

private:

	boost::shared_ptr<const PropertyTable> props;
	AnimationLayerList layers;
};


/** DOM class for deformers */
class Deformer : public Object
{
public:

	Deformer(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~Deformer();

public:

	const PropertyTable& Props() const {
		ai_assert(props.get());
		return *props.get();
	}

private:

	boost::shared_ptr<const PropertyTable> props;
};

typedef std::vector<float> WeightArray;
typedef std::vector<unsigned int> WeightIndexArray;


/** DOM class for skin deformer clusters (aka subdeformers) */
class Cluster : public Deformer
{
public:

	Cluster(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~Cluster();

public:

	/** get the list of deformer weights associated with this cluster.
	 *  Use #GetIndices() to get the associated vertices. Both arrays
	 *  have the same size (and may also be empty). */
	const WeightArray& GetWeights() const {
		return weights;
	}

	/** get indices into the vertex data of the geometry associated
	 *  with this cluster. Use #GetWeights() to get the associated weights.
	 *  Both arrays have the same size (and may also be empty). */
	const WeightIndexArray& GetIndices() const {
		return indices;
	}

	/** */
	const aiMatrix4x4& Transform() const {
		return transform;
	}

	const aiMatrix4x4& TransformLink() const {
		return transformLink;
	}

	const Model* const TargetNode() const {
		return node;
	}

private:

	WeightArray weights;
	WeightIndexArray indices;

	aiMatrix4x4 transform;
	aiMatrix4x4 transformLink;

	const Model* node;
};



/** DOM class for skin deformers */
class Skin : public Deformer
{
public:

	Skin(uint64_t id, const Element& element, const Document& doc, const std::string& name);
	~Skin();

public:

	float DeformAccuracy() const {
		return accuracy;
	}


	const std::vector<const Cluster*>& Clusters() const {
		return clusters;
	}

private:

	float accuracy;
	std::vector<const Cluster*> clusters;
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

	// these, however, are always guaranteed to be valid
	LazyObject& LazySourceObject() const;
	LazyObject& LazyDestinationObject() const;


	/** return the name of the property the connection is attached to.
	  * this is an empty string for object to object (OO) connections. */
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


/** DOM class for global document settings, a single instance per document can
 *  be accessed via Document.Globals(). */
class FileGlobalSettings
{
public:

	FileGlobalSettings(const Document& doc, boost::shared_ptr<const PropertyTable> props);
	~FileGlobalSettings();

public:

	const PropertyTable& Props() const {
		ai_assert(props.get());
		return *props.get();
	}

	const Document& GetDocument() const {
		return doc;
	}


	fbx_simple_property(UpAxis, int, 1);
	fbx_simple_property(UpAxisSign, int, 1);
	fbx_simple_property(FrontAxis, int, 2);
	fbx_simple_property(FrontAxisSign, int, 1);
	fbx_simple_property(CoordAxis, int, 0);
	fbx_simple_property(CoordAxisSign, int, 1);
	fbx_simple_property(OriginalUpAxis, int, 0);
	fbx_simple_property(OriginalUpAxisSign, int, 1);
	fbx_simple_property(UnitScaleFactor, double, 1);
	fbx_simple_property(OriginalUnitScaleFactor, double, 1);
	fbx_simple_property(AmbientColor, aiVector3D, aiVector3D(0,0,0));
	fbx_simple_property(DefaultCamera, std::string, "");


	enum FrameRate {
		FrameRate_DEFAULT = 0,
		FrameRate_120 = 1,
		FrameRate_100 = 2,
		FrameRate_60 = 3,
		FrameRate_50 = 4,
		FrameRate_48 = 5,
		FrameRate_30 = 6,
		FrameRate_30_DROP = 7,
		FrameRate_NTSC_DROP_FRAME = 8,
		FrameRate_NTSC_FULL_FRAME = 9,
		FrameRate_PAL = 10,
		FrameRate_CINEMA = 11,
		FrameRate_1000 = 12,
		FrameRate_CINEMA_ND = 13,
		FrameRate_CUSTOM = 14,

		FrameRate_MAX// end-of-enum sentinel
	};

	fbx_simple_enum_property(TimeMode, FrameRate, FrameRate_DEFAULT);
	fbx_simple_property(TimeSpanStart, uint64_t, 0L);
	fbx_simple_property(TimeSpanStop, uint64_t, 0L);
	fbx_simple_property(CustomFrameRate, float, -1.0f);


private:

	boost::shared_ptr<const PropertyTable> props;
	const Document& doc;
};




/** DOM root for a FBX file */
class Document 
{
public:

	Document(const Parser& parser, const ImportSettings& settings);
	~Document();

public:

	LazyObject* GetObject(uint64_t id) const;

	bool IsBinary() const {
		return parser.IsBinary();
	}

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

	const FileGlobalSettings& GlobalSettings() const {
		ai_assert(globals.get());
		return *globals.get();
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

	// note: the implicit rule in all DOM classes is to always resolve
	// from destination to source (since the FBX object hierarchy is,
	// with very few exceptions, a DAG, this avoids cycles). In all
	// cases that may involve back-facing edges in the object graph,
	// use LazyObject::IsBeingConstructed() to check.

	std::vector<const Connection*> GetConnectionsBySourceSequenced(uint64_t source) const;
	std::vector<const Connection*> GetConnectionsByDestinationSequenced(uint64_t dest) const;

	std::vector<const Connection*> GetConnectionsBySourceSequenced(uint64_t source, const char* classname) const;
	std::vector<const Connection*> GetConnectionsByDestinationSequenced(uint64_t dest, const char* classname) const;

	std::vector<const Connection*> GetConnectionsBySourceSequenced(uint64_t source, 
		const char* const* classnames, size_t count) const;
	std::vector<const Connection*> GetConnectionsByDestinationSequenced(uint64_t dest, 
		const char* const* classnames, 
		size_t count) const;

	const std::vector<const AnimationStack*>& AnimationStacks() const;

private:

	std::vector<const Connection*> GetConnectionsSequenced(uint64_t id, const ConnectionMap&) const;
	std::vector<const Connection*> GetConnectionsSequenced(uint64_t id, bool is_src, 
		const ConnectionMap&, 
		const char* const* classnames, 
		size_t count) const;

private:

	void ReadHeader();
	void ReadObjects();
	void ReadPropertyTemplates();
	void ReadConnections();
	void ReadGlobalSettings();

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

	std::vector<uint64_t> animationStacks;
	mutable std::vector<const AnimationStack*> animationStacksResolved;

	boost::scoped_ptr<FileGlobalSettings> globals;
};

}
}

#endif
