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

/** @file GenUVCoords step */


#include "AssimpPCH.h"
#include "ComputeUVMappingProcess.h"
#include "ProcessHelper.h"

using namespace Assimp;


// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ComputeUVMappingProcess::ComputeUVMappingProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
ComputeUVMappingProcess::~ComputeUVMappingProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool ComputeUVMappingProcess::IsActive( unsigned int pFlags) const
{
	return	(pFlags & aiProcess_GenUVCoords) != 0;
}

// ------------------------------------------------------------------------------------------------
// Compute the AABB of a mesh
inline void FindAABB (aiMesh* mesh, aiVector3D& min, aiVector3D& max)
{
	min = aiVector3D (10e10f,  10e10f, 10e10f);
	max = aiVector3D (-10e10f,-10e10f,-10e10f);
	for (unsigned int i = 0;i < mesh->mNumVertices;++i)
	{
		const aiVector3D& v = mesh->mVertices[i];
		min.x = ::std::min(v.x,min.x);
		min.y = ::std::min(v.y,min.y);
		min.z = ::std::min(v.z,min.z);
		max.x = ::std::max(v.x,max.x);
		max.y = ::std::max(v.y,max.y);
		max.z = ::std::max(v.z,max.z);
	}
}

// ------------------------------------------------------------------------------------------------
// Helper function to determine the 'real' center of a mesh
inline void FindMeshCenter (aiMesh* mesh, aiVector3D& out, aiVector3D& min, aiVector3D& max)
{
	FindAABB(mesh,min,max);
	out = min + (max-min)*0.5f;
}

// ------------------------------------------------------------------------------------------------
// Helper function to determine the 'real' center of a mesh
inline void FindMeshCenter (aiMesh* mesh, aiVector3D& out)
{
	aiVector3D min,max;
	FindMeshCenter(mesh,out,min,max);
}

// ------------------------------------------------------------------------------------------------
// Check whether a ray intersects a plane and find the intersection point
inline bool PlaneIntersect(const aiRay& ray, const aiVector3D& planePos,
	const aiVector3D& planeNormal, aiVector3D& pos)
{
	const float b = planeNormal * (planePos - ray.pos);
	float h = ray.dir * planeNormal;
    if (h < 10e-5f && h > -10e-5f || (h = b/h) < 0)
		return false;

    pos = ray.pos + (ray.dir * h);
    return true;
}

// ------------------------------------------------------------------------------------------------
// Find the first empty UV channel in a mesh
inline unsigned int FindEmptyUVChannel (aiMesh* mesh)
{
	for (unsigned int m = 0; m < AI_MAX_NUMBER_OF_TEXTURECOORDS;++m)
		if (!mesh->mTextureCoords[m])return m;
	
	DefaultLogger::get()->error("Unable to compute UV coordinates, no free UV slot found");
	return 0xffffffff;
}

// ------------------------------------------------------------------------------------------------
// Try to remove UV seams
void RemoveUVSeams (aiMesh* mesh, aiVector3D* out)
{
	// TODO: just a very rough algorithm. I think it could be done
	// much easier, but I don't know how and am currently too tired to 
	// to think about a better solution. 

	const static float LOWER_LIMIT = 0.1f;
	const static float UPPER_LIMIT = 0.9f;

	const static float LOWER_EPSILON = 10e-3f;
	const static float UPPER_EPSILON = 1.f-10e-3f;

	for (unsigned int fidx = 0; fidx < mesh->mNumFaces;++fidx)
	{
		const aiFace& face = mesh->mFaces[fidx];
		if (face.mNumIndices < 3) continue; // triangles and polygons only, please

		unsigned int small = face.mNumIndices, large = small;
		bool zero = false, one = false, round_to_zero = false;

		// Check whether this face lies on a UV seam. We can just guess,
		// but the assumption that a face with at least one very small
		// on the one side and one very large U coord on the other side 
		// lies on a UV seam should work for most cases.
		for (unsigned int n = 0; n < face.mNumIndices;++n)
		{
			if (out[face.mIndices[n]].x < LOWER_LIMIT)
			{
				small = n;

				// If we have a U value very close to 0 we can't
				// round the others to 0, too.
				if (out[face.mIndices[n]].x <= LOWER_EPSILON)
					zero = true;
				else round_to_zero = true;
			}
			if (out[face.mIndices[n]].x > UPPER_LIMIT)
			{
				large = n;

				// If we have a U value very close to 1 we can't
				// round the others to 1, too.
				if (out[face.mIndices[n]].x >= UPPER_EPSILON)
					one = true;
			}
		}
		if (small != face.mNumIndices && large != face.mNumIndices)
		{
			for (unsigned int n = 0; n < face.mNumIndices;++n)
			{
				// If the u value is over the upper limit and no other u 
				// value of that face is 0, round it to 0
				if (out[face.mIndices[n]].x > UPPER_LIMIT && !zero)
					out[face.mIndices[n]].x = 0.f;

				// If the u value is below the lower limit and no other u 
				// value of that face is 1, round it to 1
				else if (out[face.mIndices[n]].x < LOWER_LIMIT && !one)
					out[face.mIndices[n]].x = 1.f;

				// The face contains both 0 and 1 as UV coords. This can occur
				// for faces which have an edge that lies directly on the seam.
				// Due to numerical inaccuracies one U coord becomes 0, the
				// other 1. But we do still have a third UV coord to determine 
				// to which side we must round to.
				else if (one && zero)
				{
					if (round_to_zero && out[face.mIndices[n]].x >=  UPPER_EPSILON)
						out[face.mIndices[n]].x = 0.f;
					else if (!round_to_zero && out[face.mIndices[n]].x <= LOWER_EPSILON)
						out[face.mIndices[n]].x = 1.f;
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ComputeUVMappingProcess::ComputeSphereMapping(aiMesh* mesh,aiAxis axis, aiVector3D* out)
{
	aiVector3D center;
	FindMeshCenter (mesh, center);
	
	// For each point get a normalized projection vector in the sphere,
	// get its longitude and latitude and map them to their respective
	// UV axes. Problems occur around the poles ... unsolvable.
	//
	// The spherical coordinate system looks like this:
	// x = cos(lon)*cos(lat)
	// y = sin(lon)*cos(lat)
	// z = sin(lat)
	// 
	// Thus we can derive:
	// lat  = arcsin (z)
	// lon  = arctan (y/x)

	for (unsigned int pnt = 0; pnt < mesh->mNumVertices;++pnt)
	{
		const aiVector3D diff = (mesh->mVertices[pnt]-center).Normalize();
		float lat, lon;

		switch (axis)
		{
		case aiAxis_X:
			lat = asin  (diff.x);
			lon = atan2 (diff.z, diff.y);
			break;
		case aiAxis_Y:
			lat = asin  (diff.y);
			lon = atan2 (diff.x, diff.z);
			break;
		case aiAxis_Z:
			lat = asin  (diff.z);
			lon = atan2 (diff.y, diff.x);
			break;
		}
		out[pnt] = aiVector3D((lon + (float)AI_MATH_PI ) / (float)AI_MATH_TWO_PI,
			(lat + (float)AI_MATH_HALF_PI) / (float)AI_MATH_PI, 0.f);
	}

	// Now find and remove UV seams. A seam occurs if a face has a tcoord
	// close to zero on the one side, and a tcoord close to one on the
	// other side.
	RemoveUVSeams(mesh,out);
}

// ------------------------------------------------------------------------------------------------
void ComputeUVMappingProcess::ComputeCylinderMapping(aiMesh* mesh,aiAxis axis, aiVector3D* out)
{
	aiVector3D center, min, max;
	FindMeshCenter(mesh, center, min, max);

	ai_assert(0 == aiAxis_X);
	const float diff = max[axis] - min[axis];
	if (!diff)
	{
		DefaultLogger::get()->error("Can't compute cylindrical mapping, the mesh is "
			"flat in the requested axis");

		return;
	}

	// If the main axis is 'z', the z coordinate of a point 'p' is mapped 
	// directly to the texture V axis. The other axis is derived from
	// the angle between ( p.x - c.x, p.y - c.y ) and (1,0), where
	// 'c' is the center point of the mesh.
	for (unsigned int pnt = 0; pnt < mesh->mNumVertices;++pnt)
	{
		const aiVector3D& pos = mesh->mVertices[pnt];
		aiVector3D& uv  = out[pnt];

		switch (axis)
		{
		case aiAxis_X:
			uv.y = (pos.x - min.x) / diff;
			uv.x = atan2 ( pos.z - center.z, pos.y - center.y);
			break;
		case aiAxis_Y:
			uv.y = (pos.y - min.y) / diff;
			uv.x = atan2 ( pos.x - center.x, pos.z - center.z);
			break;
		case aiAxis_Z:
			uv.y = (pos.z - min.z) / diff;
			uv.x = atan2 ( pos.y - center.y, pos.x - center.x);
			break;
		}
		uv.x = (uv.x +(float)AI_MATH_PI ) / (float)AI_MATH_TWO_PI;
		uv.z = 0.f;
	}

	// Now find and remove UV seams. A seam occurs if a face has a tcoord
	// close to zero on the one side, and a tcoord close to one on the
	// other side.
	RemoveUVSeams(mesh,out);
}

// ------------------------------------------------------------------------------------------------
void ComputeUVMappingProcess::ComputePlaneMapping(aiMesh* mesh,aiAxis axis, aiVector3D* out)
{
	aiVector3D center, min, max;
	FindMeshCenter(mesh, center, min, max);

	float diffu,diffv;

	switch (axis)
	{
		case aiAxis_X:
			diffu = max.z - min.z;
			diffv = max.y - min.y;
			break;
		case aiAxis_Y:
			diffu = max.x - min.x;
			diffv = max.z - min.z;
			break;
		case aiAxis_Z:
			diffu = max.y - min.y;
			diffv = max.z - min.z;
			break;
	}
	
	if (!diffu || !diffv)
	{
		DefaultLogger::get()->error("Can't compute plane mapping, the mesh is "
			"flat in the requested axis");
		return;
	}

	// That's rather simple. We just project the vertices onto a plane
	// that lies on the two coordinate aces orthogonal to the main axis
	for (unsigned int pnt = 0; pnt < mesh->mNumVertices;++pnt)
	{
		const aiVector3D& pos = mesh->mVertices[pnt];
		aiVector3D& uv  = out[pnt];

		switch (axis)
		{
		case aiAxis_X:
			uv.x = (pos.z - min.z) / diffu;
			uv.y = (pos.y - min.y) / diffv;
			break;
		case aiAxis_Y:
			uv.x = (pos.x - min.x) / diffu;
			uv.y = (pos.z - min.z) / diffv;
			break;
		case aiAxis_Z:
			uv.x = (pos.y - min.y) / diffu;
			uv.y = (pos.x - min.x) / diffv;
			break;
		}
		uv.z = 0.f;
	}

}

// ------------------------------------------------------------------------------------------------
void ComputeUVMappingProcess::ComputeBoxMapping(aiMesh* mesh, aiVector3D* out)
{
	DefaultLogger::get()->error("Mapping type currently not implemented");
}

// ------------------------------------------------------------------------------------------------
void ComputeUVMappingProcess::Execute( aiScene* pScene) 
{
	DefaultLogger::get()->debug("GenUVCoordsProcess begin");
	char buffer[1024];

	if (pScene->mFlags & AI_SCENE_FLAGS_NON_VERBOSE_FORMAT)
		throw new ImportErrorException("Post-processing order mismatch: expecting pseudo-indexed (\"verbose\") vertices here");

	std::list<MappingInfo> mappingStack;

	/*  Iterate through all materials and search for non-UV mapped textures
	 */
	for (unsigned int i = 0; i < pScene->mNumMaterials;++i)
	{
		mappingStack.clear();
		aiMaterial* mat = pScene->mMaterials[i];
		for (unsigned int a = 0; a < mat->mNumProperties;++a)
		{
			aiMaterialProperty* prop = mat->mProperties[a];
			if (!::strcmp( prop->mKey.data, "$tex.mapping"))
			{
				aiTextureMapping& mapping = *((aiTextureMapping*)prop->mData);
				if (aiTextureMapping_UV != mapping)
				{
					if (!DefaultLogger::isNullLogger())
					{
						sprintf(buffer, "Found non-UV mapped texture (%s,%i). Mapping type: %s",
							TextureTypeToString((aiTextureType)prop->mSemantic),prop->mIndex,
							MappingTypeToString(mapping));

						DefaultLogger::get()->info(buffer);
					}

					if (aiTextureMapping_OTHER == mapping)
						continue;

					MappingInfo info (mapping);

					// Get further properties - currently only the major axis
					for (unsigned int a2 = 0; a2 < mat->mNumProperties;++a2)
					{
						aiMaterialProperty* prop2 = mat->mProperties[a2];
						if (prop2->mSemantic != prop->mSemantic || prop2->mIndex != prop->mIndex)
							continue;

						if ( !::strcmp( prop2->mKey.data, "$tex.mapaxis"))
						{
							info.axis = *((aiAxis*)prop2->mData);
							break;
						}
					}

					unsigned int idx;

					// Check whether we have this mapping mode already
					std::list<MappingInfo>::iterator it = std::find (mappingStack.begin(),mappingStack.end(), info);
					if (mappingStack.end() != it)
					{
						idx = (*it).uv;
					}
					else
					{
						/*  We have found a non-UV mapped texture. Now
						*   we need to find all meshes using this material
						*   that we can compute UV channels for them.
						*/
						for (unsigned int m = 0; m < pScene->mNumMeshes;++m)
						{
							aiMesh* mesh = pScene->mMeshes[m];
							unsigned int outIdx;
							if ( mesh->mMaterialIndex != i || ( outIdx = FindEmptyUVChannel(mesh) ) == 0xffffffff ||
								!mesh->mNumVertices)
							{
								continue;
							}

							// Allocate output storage
							aiVector3D* p = mesh->mTextureCoords[outIdx] = new aiVector3D[mesh->mNumVertices];

							switch (mapping)
							{
							case aiTextureMapping_SPHERE:
								ComputeSphereMapping(mesh,info.axis,p);
								break;
							case aiTextureMapping_CYLINDER:
								ComputeCylinderMapping(mesh,info.axis,p);
								break;
							case aiTextureMapping_PLANE:
								ComputePlaneMapping(mesh,info.axis,p);
								break;
							case aiTextureMapping_BOX:
								ComputeBoxMapping(mesh,p);
								break;
							default:
								ai_assert(false);
							}
							if (m && idx != outIdx)
							{
								DefaultLogger::get()->warn("UV index mismatch. Not all meshes assigned to "
									"this material have equal numbers of UV channels. The UV index stored in  "
									"the material structure does therefore not apply for all meshes. ");
							}
							idx = outIdx;
						}
						info.uv = idx;
						mappingStack.push_back(info);
					}

					// Update the material property list
					mapping = aiTextureMapping_UV;
					((MaterialHelper*)mat)->AddProperty(&idx,1,AI_MATKEY_UVWSRC(prop->mSemantic,prop->mIndex));
				}
			}
		}
	}
	DefaultLogger::get()->debug("GenUVCoordsProcess finished");
}
