/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2009, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

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
---------------------------------------------------------------------------
*/

/** @file  TriangulateProcess.cpp
 *  @brief Implementation of the post processing step to split up
 *    all faces with more than three indices into triangles.
 *
 *
 *  The triangulation algorithm will handle concave or convex polygons.
 *  Self-intersecting or non-planar polygons are not rejected, but
 *  they're probably not triangulated correctly.
 *
 * AI_BUILD_TRIANGULATE_COLOR_FACE_WINDING
 *   - generates vertex colors to represent the face winding order.
 *     the first vertex of a polygon becomes red, the last blue.
 */

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_TRIANGULATE_PROCESS
#include "TriangulateProcess.h"
#include "ProcessHelper.h"

//#define AI_BUILD_TRIANGULATE_COLOR_FACE_WINDING
using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
TriangulateProcess::TriangulateProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
TriangulateProcess::~TriangulateProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool TriangulateProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_Triangulate) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void TriangulateProcess::Execute( aiScene* pScene)
{
	DefaultLogger::get()->debug("TriangulateProcess begin");

	bool bHas = false;
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
	{
		if(	TriangulateMesh( pScene->mMeshes[a]))
			bHas = true;
	}
	if (bHas)DefaultLogger::get()->info ("TriangulateProcess finished. All polygons have been triangulated.");
	else     DefaultLogger::get()->debug("TriangulateProcess finished. There was nothing to be done.");
}

// ------------------------------------------------------------------------------------------------
// Test whether a point p2 is on the left side of the line formed by p0-p1 
inline bool OnLeftSideOfLine(const aiVector2D& p0, const aiVector2D& p1,const aiVector2D& p2)
{
	return ( (p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y) ) > 0;
}

// ------------------------------------------------------------------------------------------------
// Test whether a point is inside a given triangle in R2
inline bool PointInTriangle2D(const aiVector2D& p0, const aiVector2D& p1,const aiVector2D& p2, const aiVector2D& pp)
{
	// Point in triangle test using baryzentric coordinates
	const aiVector2D v0 = p1 - p0;
	const aiVector2D v1 = p2 - p0;
	const aiVector2D v2 = pp - p0;

	float dot00 = v0 * v0;
	float dot01 = v0 * v1;
	float dot02 = v0 * v2;
	float dot11 = v1 * v1;
	float dot12 = v1 * v2;

	const float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	dot11 = (dot11 * dot02 - dot01 * dot12) * invDenom;
	dot00 = (dot00 * dot12 - dot01 * dot02) * invDenom;

	return (dot11 > 0) && (dot00 > 0) && (dot11 + dot00 < 1);
}

// ------------------------------------------------------------------------------------------------
// Triangulates the given mesh.
bool TriangulateProcess::TriangulateMesh( aiMesh* pMesh)
{
	// Now we have aiMesh::mPrimitiveTypes, so this is only here for test cases
	if (!pMesh->mPrimitiveTypes)	{
		bool bNeed = false;

		for( unsigned int a = 0; a < pMesh->mNumFaces; a++)	{
			const aiFace& face = pMesh->mFaces[a];

			if( face.mNumIndices != 3)	{
				bNeed = true;
			}
		}
		if (!bNeed)
			return false;
	}
	else if (!(pMesh->mPrimitiveTypes & aiPrimitiveType_POLYGON)) {
		return false;
	}

	// the output mesh will contain triangles, but no polys anymore
	pMesh->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
	pMesh->mPrimitiveTypes &= ~aiPrimitiveType_POLYGON;

	// Find out how many output faces we'll get
	unsigned int numOut = 0, max_out = 0;
	for( unsigned int a = 0; a < pMesh->mNumFaces; a++)	{
		aiFace& face = pMesh->mFaces[a];
		if( face.mNumIndices <= 3)
			numOut++;

		else {
			numOut += face.mNumIndices-2;
			max_out = std::max(max_out,face.mNumIndices);
		}
	}

	// Just another check whether aiMesh::mPrimitiveTypes is correct
	assert(numOut != pMesh->mNumFaces);

	aiVector3D* nor_out = NULL;
	if (!pMesh->mNormals && pMesh->mPrimitiveTypes == aiPrimitiveType_POLYGON) {
		nor_out = pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];
	}

	aiFace* out = new aiFace[numOut], *curOut = out;
	std::vector<aiVector3D> temp_verts(max_out+2); /* temporary storage for vertices */

	// Apply vertex colors to represent the face winding?
#ifdef AI_BUILD_TRIANGULATE_COLOR_FACE_WINDING
	if (!pMesh->mColors[0])
		pMesh->mColors[0] = new aiColor4D[pMesh->mNumVertices];
	else
		new(pMesh->mColors[0]) aiColor4D[pMesh->mNumVertices];

	aiColor4D* clr = pMesh->mColors[0];
#endif

	// use boost::scoped_array to avoid slow std::vector<bool> specialiations
	boost::scoped_array<bool> done(new bool[max_out]); 
	for( unsigned int a = 0; a < pMesh->mNumFaces; a++)	{
		aiFace& face = pMesh->mFaces[a];

		unsigned int* idx = face.mIndices;
		int num = (int)face.mNumIndices, ear = 0, tmp, prev = num-1, next = 0, max = num;

		// Apply vertex colors to represent the face winding?
#ifdef AI_BUILD_TRIANGULATE_COLOR_FACE_WINDING
		for (unsigned int i = 0; i < face.mNumIndices; ++i) {
			aiColor4D& c = clr[idx[i]];
			c.r = (i+1) / (float)max;
			c.b = 1.f - c.r;
		}
#endif

		// if it's a simple point,line or triangle: just copy it
		if( face.mNumIndices <= 3)
		{
			aiFace& nface = *curOut++;
			nface.mNumIndices = face.mNumIndices;
			nface.mIndices    = face.mIndices;
		} 
		// quadrilaterals can't have ears. trifanning will always work
		else if ( face.mNumIndices == 4) {
			aiFace& nface = *curOut++;
			nface.mNumIndices = 3;
			nface.mIndices = face.mIndices;

			aiFace& sface = *curOut++;
			sface.mNumIndices = 3;
			sface.mIndices = new unsigned int[3];

			sface.mIndices[0] = face.mIndices[0];
			sface.mIndices[1] = face.mIndices[2];
			sface.mIndices[2] = face.mIndices[3];
		}
		else
		{
			// A polygon with more than 3 vertices can be either concave or convex.
			// Usually everything we're getting is convex and we could easily
			// triangulate by trifanning. However, LightWave is probably the only
			// modeller to make extensive use of highly concave monster polygons ...
			// so we need to apply the full 'ear cutting' algorithm.

			// RERQUIREMENT: polygon is expected to be simple and *nearly* planar.
			// We project it onto a plane to get 2d data. Working in R3 would
			// also be possible but it's more difficult to implement. 

			// Collect all vertices of of the polygon.
			aiVector3D* verts = pMesh->mVertices;
			for (tmp = 0; tmp < max; ++tmp) {
				temp_verts[tmp] = verts[idx[tmp]];
			}

			// Get newell normal of the polygon. Store it for future use if it's a polygon-only mesh
			aiVector3D n;
			NewellNormal<3,3,3>(n,max,&temp_verts.front().x,&temp_verts.front().y,&temp_verts.front().z);
			if (nor_out) {
				 for (tmp = 0; tmp < max; ++tmp)
					 nor_out[idx[tmp]] = n;
			}

			// Select largest normal coordinate to ignore for projection
			const float ax = (n.x>0 ? n.x : -n.x);    
			const float ay = (n.y>0 ? n.y : -n.y);   
			const float az = (n.z>0 ? n.z : -n.z);    

			unsigned int ac = 0, bc = 1; /* no z coord. projection to xy */
			float inv = n.z;
			if (ax > ay) {
				if (ax > az) { /* no x coord. projection to yz */
					ac = 1; bc = 2;
					inv = n.x;
				}
			}
			else if (ay > az) { /* no y coord. projection to zy */
				ac = 2; bc = 0;
				inv = n.y;
			}

			// Swap projection axes to take the negated projection vector into account
			if (inv < 0.f) {
				std::swap(ac,bc);
			}

			for (tmp =0; tmp < max; ++tmp) {
				temp_verts[tmp].x = verts[idx[tmp]][ac];
				temp_verts[tmp].y = verts[idx[tmp]][bc];
				done[tmp] = false;	
			}

			//
			// FIXME: currently this is the slow O(kn) variant with a worst case
			// complexity of O(n^2) (I think). Can be done in O(n).
			while (num > 3)	{

				// Find the next ear of the polygon
				int num_found = 0;
				for (ear = next;;prev = ear,ear = next) {
				
					// break after we looped two times without a positive match
					for (next=ear+1;done[(next>max-1?next=0:next)];++next);
					if (next < ear) {
						if (++num_found == 2) {
							break;
						}
					}
					const aiVector2D* pnt1 = (const aiVector2D*)&temp_verts[ear], 
						*pnt0 = (const aiVector2D*)&temp_verts[prev], 
						*pnt2 = (const aiVector2D*)&temp_verts[next];
			
					// Must be a convex point. Assuming ccw winding, it must be on the right of the line between p-1 and p+1.
					if (OnLeftSideOfLine (*pnt0,*pnt2,*pnt1)) {
						continue;
					}

					// and no other point may be contained in this triangle
					for ( tmp = 0; tmp < max; ++tmp) {

						// We need to compare the actual values because it's possible that multiple indexes in 
						// the polygon are refering to the same position. concave_polygon.obj is a sample
						//
						// FIXME: Use 'epsiloned' comparisons instead? Due to numeric inaccuracies in
						// PointInTriangle() I'm guessing that it's actually possible to construct
						// input data that would cause us to end up with no ears. The problem is,
						// which epsilon? If we chose a too large value, we'd get wrong results
						const aiVector2D& vtmp = * ((aiVector2D*) &temp_verts[tmp] ); 
						if ( vtmp != *pnt1 && vtmp != *pnt2 && vtmp != *pnt0 && PointInTriangle2D(*pnt0,*pnt1,*pnt2,vtmp))
							break;		

					}
					if (tmp != max) {
						continue;
					}
							
					// this vertex is an ear
					break;
				}
				if (num_found == 2) {

					// Due to the 'two ear theorem', every simple polygon with more than three points must
					// have 2 'ears'. Here's definitely someting wrong ... but we don't give up yet.
					//

					// Instead we're continuting with the standard trifanning algorithm which we'd
					// use if we had only convex polygons. That's life.
					DefaultLogger::get()->error("Failed to triangulate polygon (no ear found). Probably not a simple polygon?");

					curOut -= (max-num); /* undo all previous work */
					for (tmp = 0; tmp < max-2; ++tmp) {
						aiFace& nface = *curOut++;

						nface.mNumIndices = 3;
						if (!nface.mIndices)
							nface.mIndices = new unsigned int[3];

						nface.mIndices[0] = idx[0];
						nface.mIndices[1] = idx[tmp+1];
						nface.mIndices[2] = idx[tmp+2];
					}
					num = 0;
					break;
				}

				aiFace& nface = *curOut++;
				nface.mNumIndices = 3;

				if (!nface.mIndices) {
					nface.mIndices = new unsigned int[3];
				}

				// setup indices for the new triangle ...
				nface.mIndices[0] = idx[prev];
				nface.mIndices[1] = idx[ear];
				nface.mIndices[2] = idx[next];

				// exclude the ear from most further processing
				done[ear] = true;
				--num;
			}
			if (num > 0) {
				// We have three indices forming the last 'ear' remaining. Collect them.
				aiFace& nface = *curOut++;
				nface.mNumIndices = 3;
				nface.mIndices = face.mIndices;

				for (tmp = 0; done[tmp]; ++tmp);
				idx[0] = idx[tmp];

				for (++tmp; done[tmp]; ++tmp);
				idx[1] = idx[tmp];

				for (++tmp; done[tmp]; ++tmp);
				idx[2] = idx[tmp];
			}
		}
		face.mIndices = NULL; /* prevent unintended deletion of our awesome results. would be a pity */
	}

	// kill the old faces
	delete [] pMesh->mFaces;

	// ... and store the new ones
	pMesh->mFaces    = out;
	pMesh->mNumFaces = (unsigned int)(curOut-out); /* not necessarily equal to numOut */
	return true;
}

#endif // !! ASSIMP_BUILD_NO_TRIANGULATE_PROCESS
