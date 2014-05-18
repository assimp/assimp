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

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include "OgreStructs.h"
#include "TinyFormatter.h"

namespace Assimp
{
namespace Ogre
{

// VertexElement

VertexElement::VertexElement() : 
	index(0),
	source(0),
	offset(0),
	type(VET_FLOAT1),
	semantic(VES_POSITION)
{
}

size_t VertexElement::Size() const
{
	return TypeSize(type);
}

size_t VertexElement::ComponentCount() const
{
	return ComponentCount(type);
}

size_t VertexElement::ComponentCount(Type type)
{
	switch(type)
	{
		case VET_COLOUR:
		case VET_COLOUR_ABGR:
		case VET_COLOUR_ARGB:
		case VET_FLOAT1:
		case VET_DOUBLE1:
		case VET_SHORT1:
		case VET_USHORT1:
		case VET_INT1:
		case VET_UINT1:
			return 1;
		case VET_FLOAT2:
		case VET_DOUBLE2:
		case VET_SHORT2:
		case VET_USHORT2:
		case VET_INT2:
		case VET_UINT2:
			return 2;
		case VET_FLOAT3:
		case VET_DOUBLE3:
		case VET_SHORT3:
		case VET_USHORT3:
		case VET_INT3:
		case VET_UINT3:
			return 3;
		case VET_FLOAT4:
		case VET_DOUBLE4:
		case VET_SHORT4:
		case VET_USHORT4:
		case VET_INT4:
		case VET_UINT4:
		case VET_UBYTE4:
			return 4;
	}
	return 0;
}

size_t VertexElement::TypeSize(Type type)
{
	switch(type)
	{
		case VET_COLOUR:
		case VET_COLOUR_ABGR:
		case VET_COLOUR_ARGB:
			return sizeof(unsigned int);
		case VET_FLOAT1:
			return sizeof(float);
		case VET_FLOAT2:
			return sizeof(float)*2;
		case VET_FLOAT3:
			return sizeof(float)*3;
		case VET_FLOAT4:
			return sizeof(float)*4;
		case VET_DOUBLE1:
			return sizeof(double);
		case VET_DOUBLE2:
			return sizeof(double)*2;
		case VET_DOUBLE3:
			return sizeof(double)*3;
		case VET_DOUBLE4:
			return sizeof(double)*4;
		case VET_SHORT1:
			return sizeof(short);
		case VET_SHORT2:
			return sizeof(short)*2;
		case VET_SHORT3:
			return sizeof(short)*3;
		case VET_SHORT4:
			return sizeof(short)*4;
		case VET_USHORT1:
			return sizeof(unsigned short);
		case VET_USHORT2:
			return sizeof(unsigned short)*2;
		case VET_USHORT3:
			return sizeof(unsigned short)*3;
		case VET_USHORT4:
			return sizeof(unsigned short)*4;
		case VET_INT1:
			return sizeof(int);
		case VET_INT2:
			return sizeof(int)*2;
		case VET_INT3:
			return sizeof(int)*3;
		case VET_INT4:
			return sizeof(int)*4;
		case VET_UINT1:
			return sizeof(unsigned int);
		case VET_UINT2:
			return sizeof(unsigned int)*2;
		case VET_UINT3:
			return sizeof(unsigned int)*3;
		case VET_UINT4:
			return sizeof(unsigned int)*4;
		case VET_UBYTE4:
			return sizeof(unsigned char)*4;
	}
	return 0;
}

std::string VertexElement::TypeToString()
{
	return TypeToString(type);
}

std::string VertexElement::TypeToString(Type type)
{
	switch(type)
	{
		case VET_COLOUR:		return "COLOUR";
		case VET_COLOUR_ABGR:	return "COLOUR_ABGR";
		case VET_COLOUR_ARGB:	return "COLOUR_ARGB";
		case VET_FLOAT1:		return "FLOAT1";
		case VET_FLOAT2:		return "FLOAT2";
		case VET_FLOAT3:		return "FLOAT3";
		case VET_FLOAT4:		return "FLOAT4";
		case VET_DOUBLE1:		return "DOUBLE1";
		case VET_DOUBLE2:		return "DOUBLE2";
		case VET_DOUBLE3:		return "DOUBLE3";
		case VET_DOUBLE4:		return "DOUBLE4";
		case VET_SHORT1:		return "SHORT1";
		case VET_SHORT2:		return "SHORT2";
		case VET_SHORT3:		return "SHORT3";
		case VET_SHORT4:		return "SHORT4";
		case VET_USHORT1:		return "USHORT1";
		case VET_USHORT2:		return "USHORT2";
		case VET_USHORT3:		return "USHORT3";
		case VET_USHORT4:		return "USHORT4";
		case VET_INT1:			return "INT1";
		case VET_INT2:			return "INT2";
		case VET_INT3:			return "INT3";
		case VET_INT4:			return "INT4";
		case VET_UINT1:			return "UINT1";
		case VET_UINT2:			return "UINT2";
		case VET_UINT3:			return "UINT3";
		case VET_UINT4:			return "UINT4";
		case VET_UBYTE4:		return "UBYTE4";
	}
	return "Uknown_VertexElement::Type";
}

std::string VertexElement::SemanticToString()
{
	return SemanticToString(semantic);
}

std::string VertexElement::SemanticToString(Semantic semantic)
{
	switch(semantic)
	{
		case VES_POSITION:				return "POSITION";
		case VES_BLEND_WEIGHTS:			return "BLEND_WEIGHTS";
		case VES_BLEND_INDICES:			return "BLEND_INDICES";
		case VES_NORMAL:				return "NORMAL";
		case VES_DIFFUSE:				return "DIFFUSE";
		case VES_SPECULAR:				return "SPECULAR";
		case VES_TEXTURE_COORDINATES:	return "TEXTURE_COORDINATES";
		case VES_BINORMAL:				return "BINORMAL";
		case VES_TANGENT:				return "TANGENT";
	}
	return "Uknown_VertexElement::Semantic";
}

// VertexData

VertexData::VertexData() :
	count(0)
{
}

VertexData::~VertexData()
{
	Reset();
}

void VertexData::Reset()
{
	// Releases shared ptr memory streams.
	vertexBindings.clear();
	vertexElements.clear();
}

uint32_t VertexData::VertexSize(uint16_t source) const
{
	uint32_t size = 0;
	for(VertexElementList::const_iterator iter=vertexElements.begin(), end=vertexElements.end(); iter != end; ++iter)
	{
		if (iter->source == source)
			size += iter->Size();
	}
	return size;
}

MemoryStream *VertexData::VertexBuffer(uint16_t source)
{
	if (vertexBindings.find(source) != vertexBindings.end())
		return vertexBindings[source];
	return 0;
}

VertexElement *VertexData::GetVertexElement(VertexElement::Semantic semantic, uint16_t index)
{
	for(VertexElementList::iterator iter=vertexElements.begin(), end=vertexElements.end(); iter != end; ++iter)
	{
		VertexElement &element = (*iter);
		if (element.semantic == semantic && element.index == index)
			return &element;
	}
	return 0;
}

// IndexData

IndexData::IndexData() :
	count(0),
	faceCount(0),
	is32bit(false)
{
}

IndexData::~IndexData()
{
	Reset();	
}

void IndexData::Reset()
{
	// Release shared ptr memory stream.
	buffer.reset();
}

size_t IndexData::IndexSize() const
{
	return (is32bit ? sizeof(uint32_t) : sizeof(uint16_t));
}

size_t IndexData::FaceSize() const
{
	return IndexSize() * 3;
}

// Mesh

Mesh::Mesh() :
	sharedVertexData(0),
	hasSkeletalAnimations(false)
{
}

Mesh::~Mesh()
{
	Reset();
}

void Mesh::Reset()
{
	OGRE_SAFE_DELETE(sharedVertexData)

	for(size_t i=0, len=subMeshes.size(); i<len; ++i) {
		OGRE_SAFE_DELETE(subMeshes[i])
	}
	subMeshes.clear();
	for(size_t i=0, len=animations.size(); i<len; ++i) {
		OGRE_SAFE_DELETE(animations[i])
	}
	animations.clear();
	for(size_t i=0, len=poses.size(); i<len; ++i) {
		OGRE_SAFE_DELETE(poses[i])
	}
	poses.clear();
}

size_t Mesh::NumSubMeshes() const
{
	return subMeshes.size();
}

SubMesh2 *Mesh::SubMesh(uint16_t index) const
{
	for(size_t i=0; i<subMeshes.size(); ++i)
		if (subMeshes[i]->index == index)
			return subMeshes[i];
	return 0;
}

void Mesh::ConvertToAssimpScene(aiScene* dest)
{
	// Export meshes
	dest->mNumMeshes = NumSubMeshes();
	dest->mMeshes = new aiMesh*[dest->mNumMeshes];

	// Create root node
	dest->mRootNode = new aiNode();
	dest->mRootNode->mNumMeshes = dest->mNumMeshes;
	dest->mRootNode->mMeshes = new unsigned int[dest->mRootNode->mNumMeshes];

	for(size_t i=0; i<dest->mNumMeshes; ++i) {
		dest->mMeshes[i] = subMeshes[i]->ConvertToAssimpMesh(this);
		dest->mRootNode->mMeshes[i] = i;
	}
}

// SubMesh2

SubMesh2::SubMesh2() :
	index(0),
	vertexData(0),
	indexData(new IndexData()),
	usesSharedVertexData(false),
	operationType(OT_POINT_LIST),
	materialIndex(-1)
{
}

SubMesh2::~SubMesh2()
{
	Reset();
}

void SubMesh2::Reset()
{
	OGRE_SAFE_DELETE(vertexData)
	OGRE_SAFE_DELETE(indexData)
}

aiMesh *SubMesh2::ConvertToAssimpMesh(Mesh *parent)
{
	if (operationType != OT_TRIANGLE_LIST) {
		throw DeadlyImportError(Formatter::format() << "Only mesh operation type OT_TRIANGLE_LIST is supported. Found " << operationType);
	}
		
	aiMesh *dest = new aiMesh();
	dest->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

	if (!name.empty())
		dest->mName = name;

	// Material index	
	if (materialIndex != -1)
		dest->mMaterialIndex = materialIndex;

	// Pick source vertex data from shader geometry or from internal geometry.
	VertexData *src = (!usesSharedVertexData ? vertexData : parent->sharedVertexData);

	VertexElement *positionsElement = src->GetVertexElement(VertexElement::VES_POSITION);
	VertexElement *normalsElement   = src->GetVertexElement(VertexElement::VES_NORMAL);
	VertexElement *uv1Element       = src->GetVertexElement(VertexElement::VES_TEXTURE_COORDINATES, 0);
	VertexElement *uv2Element       = src->GetVertexElement(VertexElement::VES_TEXTURE_COORDINATES, 1);

	// Sanity checks
	if (!positionsElement) {
		throw DeadlyImportError("Failed to import Ogre VertexElement::VES_POSITION. Mesh does not have vertex positions!");
	} else if (positionsElement->type != VertexElement::VET_FLOAT3) {
		throw DeadlyImportError("Ogre Mesh position vertex element type != VertexElement::VET_FLOAT3. This is not supported.");
	} else if (normalsElement && normalsElement->type != VertexElement::VET_FLOAT3) {
		throw DeadlyImportError("Ogre Mesh normal vertex element type != VertexElement::VET_FLOAT3. This is not supported.");
	}

	// Faces
	dest->mNumFaces = indexData->faceCount;
	dest->mFaces = new aiFace[dest->mNumFaces];

	// Assimp required unique vertices, we need to convert from Ogres shared indexing.
	size_t uniqueVertexCount = dest->mNumFaces * 3;
	dest->mNumVertices = uniqueVertexCount;
	dest->mVertices = new aiVector3D[dest->mNumVertices];

	// Source streams
	MemoryStream *positions      = src->VertexBuffer(positionsElement->source);
	MemoryStream *normals        = (normalsElement ? src->VertexBuffer(normalsElement->source) : 0);
	MemoryStream *uv1            = (uv1Element ? src->VertexBuffer(uv1Element->source) : 0);
	MemoryStream *uv2            = (uv2Element ? src->VertexBuffer(uv2Element->source) : 0);

	// Element size
	const size_t sizePosition    = positionsElement->Size();
	const size_t sizeNormal      = (normalsElement ? normalsElement->Size() : 0);
	const size_t sizeUv1         = (uv1Element ? uv1Element->Size() : 0);
	const size_t sizeUv2         = (uv2Element ? uv2Element->Size() : 0);

	// Vertex width
	const size_t vWidthPosition  = src->VertexSize(positionsElement->source);
	const size_t vWidthNormal    = (normalsElement ? src->VertexSize(normalsElement->source) : 0);
	const size_t vWidthUv1       = (uv1Element ? src->VertexSize(uv1Element->source) : 0);
	const size_t vWidthUv2       = (uv2Element ? src->VertexSize(uv2Element->source) : 0);
	
	// Prepare normals
	if (normals) 
		dest->mNormals = new aiVector3D[dest->mNumVertices];

	// Prepare UVs, ignoring incompatible UVs.
	if (uv1)
	{
		if (uv1Element->type == VertexElement::VET_FLOAT2 || uv1Element->type == VertexElement::VET_FLOAT3)
		{
			dest->mNumUVComponents[0] = uv1Element->ComponentCount();
			dest->mTextureCoords[0] = new aiVector3D[dest->mNumVertices];	
		}
		else
		{
			DefaultLogger::get()->warn(Formatter::format() << "Ogre imported UV0 type " << uv1Element->TypeToString() << " is not compatible with Assimp. Ignoring UV.");
			uv1 = 0;
		}
	}
	if (uv2)
	{
		if (uv2Element->type == VertexElement::VET_FLOAT2 || uv2Element->type == VertexElement::VET_FLOAT3)
		{
			dest->mNumUVComponents[1] = uv2Element->ComponentCount();
			dest->mTextureCoords[1] = new aiVector3D[dest->mNumVertices];	
		}
		else
		{
			DefaultLogger::get()->warn(Formatter::format() << "Ogre imported UV0 type " << uv2Element->TypeToString() << " is not compatible with Assimp. Ignoring UV.");
			uv2 = 0;
		}
	}

	aiVector3D *uv1Dest = (uv1 ? dest->mTextureCoords[0] : 0);
	aiVector3D *uv2Dest = (uv2 ? dest->mTextureCoords[1] : 0);

	MemoryStream *faces = indexData->buffer.get();
	for (size_t fi=0, isize=indexData->IndexSize(), fsize=indexData->FaceSize(); 
		 fi<dest->mNumFaces; ++fi)
	{
		// Source Ogre face
		aiFace ogreFace;
		ogreFace.mNumIndices = 3;
		ogreFace.mIndices = new unsigned int[3];

		faces->Seek(fi * fsize, aiOrigin_SET);
		if (indexData->is32bit)
		{
			faces->Read(&ogreFace.mIndices[0], isize, 3);
		}
		else
		{
			uint16_t iout = 0;
			for (size_t ii=0; ii<3; ++ii)
			{
				faces->Read(&iout, isize, 1);
				ogreFace.mIndices[ii] = static_cast<unsigned int>(iout);
			}
		}

		// Destination Assimp face
		aiFace &face = dest->mFaces[fi];
		face.mNumIndices = 3;
		face.mIndices = new unsigned int[3];

		const size_t pos = fi * 3;
		for (size_t v=0; v<3; ++v)
		{
			const size_t newIndex = pos + v;

			// Write face index
			face.mIndices[v] = newIndex;

			// Ogres vertex index to ref into the source buffers.
			const size_t ogreVertexIndex = ogreFace.mIndices[v];
			
			// Position
			positions->Seek((vWidthPosition * ogreVertexIndex) + positionsElement->offset, aiOrigin_SET);
			positions->Read(&dest->mVertices[newIndex], sizePosition, 1);

			// Normal
			if (normals)
			{
				normals->Seek((vWidthNormal * ogreVertexIndex) + normalsElement->offset, aiOrigin_SET);
				normals->Read(&dest->mNormals[newIndex], sizeNormal, 1);
			}
			// UV0
			if (uv1 && uv1Dest)
			{
				uv1->Seek((vWidthUv1 * ogreVertexIndex) + uv1Element->offset, aiOrigin_SET);
				uv1->Read(&uv1Dest[newIndex], sizeUv1, 1);
			}
			// UV1
			if (uv2 && uv2Dest)
			{
				uv2->Seek((vWidthUv2 * ogreVertexIndex) + uv2Element->offset, aiOrigin_SET);
				uv2->Read(&uv2Dest[newIndex], sizeUv2, 1);
			}
			
			/// @todo Bones and bone weights.
		}
	} 
	return dest;
}

// Animation2

Animation2::Animation2(Mesh *_parentMesh) : 
	parentMesh(_parentMesh),
	length(0.0f),
	baseTime(-1.0f)
{
}

VertexData *Animation2::AssociatedVertexData(VertexAnimationTrack *track) const
{
	bool sharedGeom = (track->target == 0);
	if (sharedGeom)
		return parentMesh->sharedVertexData;
	else
		return parentMesh->SubMesh(track->target-1)->vertexData;
}

} // Ogre
} // Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
