/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
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

#ifndef AI_PROCESS_HELPER_H_INCLUDED
#define AI_PROCESS_HELPER_H_INCLUDED

#include "../include/aiPostProcess.h"

#include "SpatialSort.h"
#include "BaseProcess.h"
#include "ParsingUtils.h"

// -------------------------------------------------------------------------------
// Some extensions to std namespace. Mainly std::min and std::max for all
// flat data types in the aiScene. They're used to quickly determine the
// min/max bounds of data arrays.
#ifdef __cplusplus
namespace std {

	// std::min for aiVector3D
	inline ::aiVector3D min (const ::aiVector3D& a, const ::aiVector3D& b)	{
		return ::aiVector3D (min(a.x,b.x),min(a.y,b.y),min(a.z,b.z));
	}

	// std::max for aiVector3D
	inline ::aiVector3D max (const ::aiVector3D& a, const ::aiVector3D& b)	{
		return ::aiVector3D (max(a.x,b.x),max(a.y,b.y),max(a.z,b.z));
	}

	// std::min for aiColor4D
	inline ::aiColor4D min (const ::aiColor4D& a, const ::aiColor4D& b)	{
		return ::aiColor4D (min(a.r,b.r),min(a.g,b.g),min(a.b,b.b),min(a.a,b.a));
	}

	// std::max for aiColor4D
	inline ::aiColor4D max (const ::aiColor4D& a, const ::aiColor4D& b)	{
		return ::aiColor4D (max(a.r,b.r),max(a.g,b.g),max(a.b,b.b),max(a.a,b.a));
	}

	// std::min for aiQuaternion
	inline ::aiQuaternion min (const ::aiQuaternion& a, const ::aiQuaternion& b)	{
		return ::aiQuaternion (min(a.w,b.w),min(a.x,b.x),min(a.y,b.y),min(a.z,b.z));
	}

	// std::max for aiQuaternion
	inline ::aiQuaternion max (const ::aiQuaternion& a, const ::aiQuaternion& b)	{
		return ::aiQuaternion (max(a.w,b.w),max(a.x,b.x),max(a.y,b.y),max(a.z,b.z));
	}

	// std::min for aiVectorKey
	inline ::aiVectorKey min (const ::aiVectorKey& a, const ::aiVectorKey& b)	{
		return ::aiVectorKey (min(a.mTime,b.mTime),min(a.mValue,b.mValue));
	}

	// std::max for aiVectorKey
	inline ::aiVectorKey max (const ::aiVectorKey& a, const ::aiVectorKey& b)	{
		return ::aiVectorKey (max(a.mTime,b.mTime),max(a.mValue,b.mValue));
	}

	// std::min for aiQuatKey
	inline ::aiQuatKey min (const ::aiQuatKey& a, const ::aiQuatKey& b)	{
		return ::aiQuatKey (min(a.mTime,b.mTime),min(a.mValue,b.mValue));
	}

	// std::max for aiQuatKey
	inline ::aiQuatKey max (const ::aiQuatKey& a, const ::aiQuatKey& b)	{
		return ::aiQuatKey (max(a.mTime,b.mTime),max(a.mValue,b.mValue));
	}

	// std::min for aiVertexWeight
	inline ::aiVertexWeight min (const ::aiVertexWeight& a, const ::aiVertexWeight& b)	{
		return ::aiVertexWeight (min(a.mVertexId,b.mVertexId),min(a.mWeight,b.mWeight));
	}

	// std::max for aiVertexWeight
	inline ::aiVertexWeight max (const ::aiVertexWeight& a, const ::aiVertexWeight& b)	{
		return ::aiVertexWeight (max(a.mVertexId,b.mVertexId),max(a.mWeight,b.mWeight));
	}

} // end namespace std
#endif // !! C++

namespace Assimp {

// -------------------------------------------------------------------------------
// Start points for ArrayBounds<T> for all supported Ts
template <typename T>
struct MinMaxChooser;

template <> struct MinMaxChooser<float> {
	void operator ()(float& min,float& max) {
		max = -10e10f;
		min =  10e10f;
}};
template <> struct MinMaxChooser<double> {
	void operator ()(double& min,double& max) {
		max = -10e10;
		min =  10e10;
}};
template <> struct MinMaxChooser<unsigned int> {
	void operator ()(unsigned int& min,unsigned int& max) {
		max = 0;
		min = (1u<<(sizeof(unsigned int)*8-1));
}};

template <> struct MinMaxChooser<aiVector3D> {
	void operator ()(aiVector3D& min,aiVector3D& max) {
		max = aiVector3D(-10e10f,-10e10f,-10e10f);
		min = aiVector3D( 10e10f, 10e10f, 10e10f);
}};
template <> struct MinMaxChooser<aiColor4D> {
	void operator ()(aiColor4D& min,aiColor4D& max) {
		max = aiColor4D(-10e10f,-10e10f,-10e10f,-10e10f);
		min = aiColor4D( 10e10f, 10e10f, 10e10f, 10e10f);
}};

template <> struct MinMaxChooser<aiQuaternion> {
	void operator ()(aiQuaternion& min,aiQuaternion& max) {
		max = aiQuaternion(-10e10f,-10e10f,-10e10f,-10e10f);
		min = aiQuaternion( 10e10f, 10e10f, 10e10f, 10e10f);
}};

template <> struct MinMaxChooser<aiVectorKey> {
	void operator ()(aiVectorKey& min,aiVectorKey& max) {
		MinMaxChooser<double>()(min.mTime,max.mTime);
		MinMaxChooser<aiVector3D>()(min.mValue,max.mValue);
}};
template <> struct MinMaxChooser<aiQuatKey> {
	void operator ()(aiQuatKey& min,aiQuatKey& max) {
		MinMaxChooser<double>()(min.mTime,max.mTime);
		MinMaxChooser<aiQuaternion>()(min.mValue,max.mValue);
}};

template <> struct MinMaxChooser<aiVertexWeight> {
	void operator ()(aiVertexWeight& min,aiVertexWeight& max) {
		MinMaxChooser<unsigned int>()(min.mVertexId,max.mVertexId);
		MinMaxChooser<float>()(min.mWeight,max.mWeight);
}};

// -------------------------------------------------------------------------------
/** @brief Find the min/max values of an array of Ts
 *  @param in Input array
 *  @param size Numebr of elements to process
 *  @param[out] min minimum value
 *  @param[out] max maximum value
 */
template <typename T>
inline void ArrayBounds(const T* in, unsigned int size, T& min, T& max) 
{
	MinMaxChooser<T> ()(min,max);
	for (unsigned int i = 0; i < size;++i) {
		min = std::min(in[i],min);
		max = std::max(in[i],max);
	}
}

// -------------------------------------------------------------------------------
/** @brief Extract single strings from a list of identifiers
 *  @param in Input string list. 
 *  @param out Receives a list of clean output strings
 *  @sdee #AI_CONFIG_PP_OG_EXCLUDE_LIST
 */
inline void ConvertListToStrings(const std::string& in, std::list<std::string>& out)
{
	const char* s = in.c_str();
	while (*s) {
		SkipSpacesAndLineEnd(&s);
		if (*s == '\'') {
			const char* base = ++s;
			while (*s != '\'') {
				++s;
				if (*s == '\0') {
					DefaultLogger::get()->error("ConvertListToString: String list is ill-formatted");
					return;
				}
			}
			out.push_back(std::string(base,(size_t)(s-base)));
			++s;
		}
		else {
			out.push_back(GetNextToken(s));
		}
	}
}

// -------------------------------------------------------------------------------
/** @brief Compute the newell normal of a polygon regardless of its shape
 *
 *  @param out Receives the output normal
 *  @param num Number of input vertices
 *  @param x X data source. x[ofs_x*n] is the n'th element. 
 *  @param y Y data source. y[ofs_y*n] is the y'th element 
 *  @param z Z data source. z[ofs_z*n] is the z'th element 
 *
 *  @note The data arrays must have storage for at least num+2 elements. Using
 *  this method is much faster than the 'other' NewellNormal()
 */
template <int ofs_x, int ofs_y, int ofs_z>
inline void NewellNormal (aiVector3D& out, int num, float* x, float* y, float* z)
{
	// Duplicate the first two vertices at the end
	x[(num+0)*ofs_x] = x[0]; 
	x[(num+1)*ofs_x] = x[ofs_x]; 

	y[(num+0)*ofs_y] = y[0]; 
	y[(num+1)*ofs_y] = y[ofs_y]; 

	z[(num+0)*ofs_z] = z[0]; 
	z[(num+1)*ofs_z] = z[ofs_z]; 

	float sum_xy = 0.0, sum_yz = 0.0, sum_zx = 0.0;

	float *xptr = x +ofs_x, *xlow = x, *xhigh = x + ofs_x*2;
	float *yptr = y +ofs_y, *ylow = y, *yhigh = y + ofs_y*2;
	float *zptr = z +ofs_z, *zlow = z, *zhigh = z + ofs_z*2;

	for (int tmp=0; tmp < num; tmp++) {
		sum_xy += (*xptr) * ( (*yhigh) - (*ylow) );
		sum_yz += (*yptr) * ( (*zhigh) - (*zlow) );
		sum_zx += (*zptr) * ( (*xhigh) - (*xlow) );

		xptr  += ofs_x;
		xlow  += ofs_x;
		xhigh += ofs_x;

		yptr  += ofs_y;
		ylow  += ofs_y;
		yhigh += ofs_y;

		zptr  += ofs_z;
		zlow  += ofs_z;
		zhigh += ofs_z;
	}
	out = aiVector3D(sum_yz,sum_zx,sum_xy);
}

#if 0
// -------------------------------------------------------------------------------
/** @brief Compute newell normal of a polgon regardless of its shape
 *
 *  @param out Receives the output normal
 *  @param data Input vertices
 *  @param idx Index buffer
 *  @param num Number of indices
 */
inline void NewellNormal (aiVector3D& out, const aiVector3D* data, unsigned int* idx, unsigned int num )
{
	// TODO: intended to be used in GenNormals. 
}
#endif

// -------------------------------------------------------------------------------
/** Little helper function to calculate the quadratic difference 
 * of two colours. 
 * @param pColor1 First color
 * @param pColor2 second color
 * @return Quadratic color difference
 */
inline float GetColorDifference( const aiColor4D& pColor1, const aiColor4D& pColor2) 
{
	const aiColor4D c (pColor1.r - pColor2.r, pColor1.g - pColor2.g, 
		pColor1.b - pColor2.b, pColor1.a - pColor2.a);

	return c.r*c.r + c.g*c.g + c.b*c.b + c.a*c.a;
}

// -------------------------------------------------------------------------------
/** @brief Compute the AABB of a mesh after applying a given transform
 *  @param mesh Input mesh
 *  @param[out] min Receives minimum transformed vertex
 *  @param[out] max Receives maximum transformed vertex
 *  @param m Transformation matrix to be applied
 */
inline void FindAABBTransformed (const aiMesh* mesh, aiVector3D& min, aiVector3D& max, 
	const aiMatrix4x4& m)
{
	min = aiVector3D (10e10f,  10e10f, 10e10f);
	max = aiVector3D (-10e10f,-10e10f,-10e10f);
	for (unsigned int i = 0;i < mesh->mNumVertices;++i)
	{
		const aiVector3D v = m * mesh->mVertices[i];
		min = std::min(v,min);
		max = std::max(v,max);
	}
}

// -------------------------------------------------------------------------------
/** @brief Helper function to determine the 'real' center of a mesh
 *
 *  That is the center of its axis-aligned bounding box.
 *  @param mesh Input mesh
 *  @param[out] min Minimum vertex of the mesh
 *  @param[out] max maximum vertex of the mesh
 *  @param[out] out Center point
 */
inline void FindMeshCenter (aiMesh* mesh, aiVector3D& out, aiVector3D& min, aiVector3D& max)
{
	ArrayBounds(mesh->mVertices,mesh->mNumVertices, min,max);
	out = min + (max-min)*0.5f;
}

// -------------------------------------------------------------------------------
// Helper function to determine the 'real' center of a mesh after applying a given transform
inline void FindMeshCenterTransformed (aiMesh* mesh, aiVector3D& out, aiVector3D& min,
	aiVector3D& max, const aiMatrix4x4& m)
{
	FindAABBTransformed(mesh,min,max,m);
	out = min + (max-min)*0.5f;
}

// -------------------------------------------------------------------------------
// Helper function to determine the 'real' center of a mesh
inline void FindMeshCenter (aiMesh* mesh, aiVector3D& out)
{
	aiVector3D min,max;
	FindMeshCenter(mesh,out,min,max);
}

// -------------------------------------------------------------------------------
// Helper function to determine the 'real' center of a mesh after applying a given transform
inline void FindMeshCenterTransformed (aiMesh* mesh, aiVector3D& out,
	const aiMatrix4x4& m)
{
	aiVector3D min,max;
	FindMeshCenterTransformed(mesh,out,min,max,m);
}

// -------------------------------------------------------------------------------
// Compute a good epsilon value for position comparisons on a mesh
inline float ComputePositionEpsilon(const aiMesh* pMesh)
{
	const float epsilon = 1e-4f;

	// calculate the position bounds so we have a reliable epsilon to check position differences against 
	aiVector3D minVec, maxVec;
	ArrayBounds(pMesh->mVertices,pMesh->mNumVertices,minVec,maxVec);
	return (maxVec - minVec).Length() * epsilon;
}

// -------------------------------------------------------------------------------
// Compute a good epsilon value for position comparisons on a array of meshes
inline float ComputePositionEpsilon(const aiMesh* const* pMeshes, size_t num)
{
	const float epsilon = 1e-4f;

	// calculate the position bounds so we have a reliable epsilon to check position differences against 
	aiVector3D minVec, maxVec, mi, ma;
	MinMaxChooser<aiVector3D>()(minVec,maxVec);

	for (size_t a = 0; a < num; ++a) {
		const aiMesh* pMesh = pMeshes[a];
		ArrayBounds(pMesh->mVertices,pMesh->mNumVertices,mi,ma);

		minVec = std::min(minVec,mi);
		maxVec = std::max(maxVec,ma);
	}
	return (maxVec - minVec).Length() * epsilon;
}

// -------------------------------------------------------------------------------
// Compute an unique value for the vertex format of a mesh
inline unsigned int GetMeshVFormatUnique(aiMesh* pcMesh)
{
	ai_assert(NULL != pcMesh);

	// FIX: the hash may never be 0. Otherwise a comparison against
	// nullptr could be successful
	unsigned int iRet = 1;

	// normals
	if (pcMesh->HasNormals())iRet |= 0x2;
	// tangents and bitangents
	if (pcMesh->HasTangentsAndBitangents())iRet |= 0x4;

#ifdef BOOST_STATIC_ASSERT
	BOOST_STATIC_ASSERT(8 >= AI_MAX_NUMBER_OF_COLOR_SETS);
	BOOST_STATIC_ASSERT(8 >= AI_MAX_NUMBER_OF_TEXTURECOORDS);
#endif

	// texture coordinates
	unsigned int p = 0;
	while (pcMesh->HasTextureCoords(p))
	{
		iRet |= (0x100 << p);
		if (3 == pcMesh->mNumUVComponents[p])
			iRet |= (0x10000 << p);

		++p;
	}
	// vertex colors
	p = 0;
	while (pcMesh->HasVertexColors(p))iRet |= (0x1000000 << p++);
	return iRet;
}

typedef std::pair <unsigned int,float> PerVertexWeight;
typedef std::vector	<PerVertexWeight> VertexWeightTable;

// -------------------------------------------------------------------------------
// Compute a per-vertex bone weight table
// please .... delete result with operator delete[] ...
inline VertexWeightTable* ComputeVertexBoneWeightTable(aiMesh* pMesh)
{
	if (!pMesh || !pMesh->mNumVertices || !pMesh->mNumBones)
		return NULL;

	VertexWeightTable* avPerVertexWeights = new VertexWeightTable[pMesh->mNumVertices];
	for (unsigned int i = 0; i < pMesh->mNumBones;++i)
	{
		aiBone* bone = pMesh->mBones[i];
		for (unsigned int a = 0; a < bone->mNumWeights;++a)	{
			const aiVertexWeight& weight = bone->mWeights[a];
			avPerVertexWeights[weight.mVertexId].push_back( 
				std::pair<unsigned int,float>(i,weight.mWeight));
		}
	}
	return avPerVertexWeights;
}

// -------------------------------------------------------------------------------
// Get a string for a given aiTextureType
inline const char* TextureTypeToString(aiTextureType in)
{
	switch (in)
	{
	case aiTextureType_NONE:
		return "n/a";
	case aiTextureType_DIFFUSE:
		return "Diffuse";
	case aiTextureType_SPECULAR:
		return "Specular";
	case aiTextureType_AMBIENT:
		return "Ambient";
	case aiTextureType_EMISSIVE:
		return "Emissive";
	case aiTextureType_OPACITY:
		return "Opacity";
	case aiTextureType_NORMALS:
		return "Normals";
	case aiTextureType_HEIGHT:
		return "Height";
	case aiTextureType_SHININESS:
		return "Shininess";
	case aiTextureType_DISPLACEMENT:
		return "Displacement";
	case aiTextureType_LIGHTMAP:
		return "Lightmap";
	case aiTextureType_REFLECTION:
		return "Reflection";
	case aiTextureType_UNKNOWN:
		return "Unknown";
    default:
        return  "HUGE ERROR. Expect BSOD (linux guys: kernel panic ...).";          
	}
}

// -------------------------------------------------------------------------------
// Get a string for a given aiTextureMapping
inline const char* MappingTypeToString(aiTextureMapping in)
{
	switch (in)
	{
	case aiTextureMapping_UV:
		return "UV";
	case aiTextureMapping_BOX:
		return "Box";
	case aiTextureMapping_SPHERE:
		return "Sphere";
	case aiTextureMapping_CYLINDER:
		return "Cylinder";
	case aiTextureMapping_PLANE:
		return "Plane";
	case aiTextureMapping_OTHER:
		return "Other";
    default:
        return  "HUGE ERROR. Expect BSOD (linux guys: kernel panic ...).";    
	}
}

// -------------------------------------------------------------------------------
// Utility postprocess step to share the spatial sort tree between
// all steps which use it to speedup its computations.
class ComputeSpatialSortProcess : public BaseProcess
{
	bool IsActive( unsigned int pFlags) const
	{
		return NULL != shared && 0 != (pFlags & (aiProcess_CalcTangentSpace | 
			aiProcess_GenNormals | aiProcess_JoinIdenticalVertices));
	}

	void Execute( aiScene* pScene)
	{
		typedef std::pair<SpatialSort, float> _Type; 
		DefaultLogger::get()->debug("Generate spatially-sorted vertex cache");

		std::vector<_Type>* p = new std::vector<_Type>(pScene->mNumMeshes); 
		std::vector<_Type>::iterator it = p->begin();

		for (unsigned int i = 0; i < pScene->mNumMeshes; ++i, ++it)	{
			aiMesh* mesh = pScene->mMeshes[i];
			_Type& blubb = *it;
			blubb.first.Fill(mesh->mVertices,mesh->mNumVertices,sizeof(aiVector3D));
			blubb.second = ComputePositionEpsilon(mesh);
		}

		shared->AddProperty(AI_SPP_SPATIAL_SORT,p);
	}
};

// -------------------------------------------------------------------------------
// ... and the same again to cleanup the whole stuff
class DestroySpatialSortProcess : public BaseProcess
{
	bool IsActive( unsigned int pFlags) const
	{
		return NULL != shared && 0 != (pFlags & (aiProcess_CalcTangentSpace | 
			aiProcess_GenNormals | aiProcess_JoinIdenticalVertices));
	}

	void Execute( aiScene* pScene)
	{
		shared->RemoveProperty(AI_SPP_SPATIAL_SORT);
	}
};

} // ! namespace Assimp
#endif // !! AI_PROCESS_HELPER_H_INCLUDED
