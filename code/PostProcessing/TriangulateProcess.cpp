/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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
 * DEBUG SWITCHES - do not enable any of them in release builds:
 *
 * AI_BUILD_TRIANGULATE_COLOR_FACE_WINDING
 *   - generates vertex colors to represent the face winding order.
 *     the first vertex of a polygon becomes red, the last blue.
 * AI_BUILD_TRIANGULATE_DEBUG_POLYS
 *   - dump all polygons and their triangulation sequences to
 *     a file
 */
#ifndef ASSIMP_BUILD_NO_TRIANGULATE_PROCESS

#include "PostProcessing/TriangulateProcess.h"
#include "PostProcessing/ProcessHelper.h"
#include "Common/PolyTools.h"
#include "contrib/earcut-hpp/earcut.hpp"

#include <memory>
#include <cstdint>

//#define AI_BUILD_TRIANGULATE_COLOR_FACE_WINDING
//#define AI_BUILD_TRIANGULATE_DEBUG_POLYS

#define POLY_GRID_Y 40
#define POLY_GRID_X 70
#define POLY_GRID_XPAD 20
#define POLY_OUTPUT_FILE "assimp_polygons_debug.txt"

namespace mapbox::util {

template <>
struct nth<0, aiVector2D> {
    inline static auto get(const aiVector2D& t) {
        return t.x;
    }
};
template <>
struct nth<1, aiVector2D> {
    inline static auto get(const aiVector2D& t) {
        return t.y;
    }
};

} // namespace mapbox::util

using namespace Assimp;

namespace {

    /**
     * @brief Helper struct used to simplify NGON encoding functions.
     */
    struct NGONEncoder {
        NGONEncoder() : mLastNGONFirstIndex((unsigned int)-1) {}

        /**
         * @brief Encode the current triangle, and make sure it is recognized as a triangle.
         *
         * This method will rotate indices in tri if needed in order to avoid tri to be considered
         * part of the previous ngon. This method is to be used whenever you want to emit a real triangle,
         * and make sure it is seen as a triangle.
         *
         * @param tri Triangle to encode.
         */
        void ngonEncodeTriangle(aiFace * tri) {
            ai_assert(tri->mNumIndices == 3);

            // Rotate indices in new triangle to avoid ngon encoding false ngons
            // Otherwise, the new triangle would be considered part of the previous NGON.
            if (isConsideredSameAsLastNgon(tri)) {
                std::swap(tri->mIndices[0], tri->mIndices[2]);
                std::swap(tri->mIndices[1], tri->mIndices[2]);
            }

            mLastNGONFirstIndex = tri->mIndices[0];
        }

        /**
         * @brief Encode a quad (2 triangles) in ngon encoding, and make sure they are seen as a single ngon.
         *
         * @param tri1 First quad triangle
         * @param tri2 Second quad triangle
         *
         * @pre Triangles must be properly fanned from the most appropriate vertex.
         */
        void ngonEncodeQuad(aiFace *tri1, aiFace *tri2) {
            ai_assert(tri1->mNumIndices == 3);
            ai_assert(tri2->mNumIndices == 3);
            ai_assert(tri1->mIndices[0] == tri2->mIndices[0]);

            // If the selected fanning vertex is the same as the previously
            // emitted ngon, we use the opposite vertex which also happens to work
            // for tri-fanning a concave quad.
            // ref: https://github.com/assimp/assimp/pull/3695#issuecomment-805999760
            if (isConsideredSameAsLastNgon(tri1)) {
                // Right-rotate indices for tri1 (index 2 becomes the new fanning vertex)
                std::swap(tri1->mIndices[0], tri1->mIndices[2]);
                std::swap(tri1->mIndices[1], tri1->mIndices[2]);

                // Left-rotate indices for tri2 (index 2 becomes the new fanning vertex)
                std::swap(tri2->mIndices[1], tri2->mIndices[2]);
                std::swap(tri2->mIndices[0], tri2->mIndices[2]);

                ai_assert(tri1->mIndices[0] == tri2->mIndices[0]);
            }

            mLastNGONFirstIndex = tri1->mIndices[0];
        }

        /**
         * @brief Check whether this triangle would be considered part of the lastly emitted ngon or not.
         *
         * @param tri Current triangle.
         * @return true If used as is, this triangle will be part of last ngon.
         * @return false If used as is, this triangle is not considered part of the last ngon.
         */
        bool isConsideredSameAsLastNgon(const aiFace * tri) const {
            ai_assert(tri->mNumIndices == 3);
            return tri->mIndices[0] == mLastNGONFirstIndex;
        }

    private:
        unsigned int mLastNGONFirstIndex;
    };

}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool TriangulateProcess::IsActive( unsigned int pFlags) const {
    return (pFlags & aiProcess_Triangulate) != 0;
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void TriangulateProcess::Execute( aiScene* pScene) {
    ASSIMP_LOG_DEBUG("TriangulateProcess begin");

    bool bHas = false;
    for( unsigned int a = 0; a < pScene->mNumMeshes; a++)
    {
        if (pScene->mMeshes[ a ]) {
            if ( TriangulateMesh( pScene->mMeshes[ a ] ) ) {
                bHas = true;
            }
        }
    }
    if ( bHas ) {
        ASSIMP_LOG_INFO( "TriangulateProcess finished. All polygons have been triangulated." );
    } else {
        ASSIMP_LOG_DEBUG( "TriangulateProcess finished. There was nothing to be done." );
    }
}

// ------------------------------------------------------------------------------------------------
// Triangulates the given mesh.
bool TriangulateProcess::TriangulateMesh( aiMesh* pMesh) {
    // Now we have aiMesh::mPrimitiveTypes, so this is only here for test cases
    if (!pMesh->mPrimitiveTypes)    {
        bool bNeed = false;

        for( unsigned int a = 0; a < pMesh->mNumFaces; a++) {
            const aiFace& face = pMesh->mFaces[a];
            if( face.mNumIndices != 3)  {
                bNeed = true;
            }
        }
        if (!bNeed) {
            return false;
        }
    }
    else if (!(pMesh->mPrimitiveTypes & aiPrimitiveType_POLYGON)) {
        return false;
    }

    // Find out how many output faces we'll get
    uint32_t numOut = 0, max_out = 0;
    bool get_normals = true;
    for( unsigned int a = 0; a < pMesh->mNumFaces; a++) {
        aiFace& face = pMesh->mFaces[a];
        if (face.mNumIndices <= 4) {
            get_normals = false;
        }
        if( face.mNumIndices <= 3) {
            ++numOut;
        } else {
            numOut += face.mNumIndices-2;
            max_out = std::max(max_out,face.mNumIndices);
        }
    }

    // Just another check whether aiMesh::mPrimitiveTypes is correct
    if (numOut == pMesh->mNumFaces) {
        ASSIMP_LOG_ERROR( "Invalidation detected in the number of indices: does not fit to the primitive type." );
        return false;
    }

    aiVector3D *nor_out = nullptr;

    // if we don't have normals yet, but expect them to be a cheap side
    // product of triangulation anyway, allocate storage for them.
    if (!pMesh->mNormals && get_normals) {
        // XXX need a mechanism to inform the GenVertexNormals process to treat these normals as preprocessed per-face normals
    //  nor_out = pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];
    }

    // the output mesh will contain triangles, but no polys anymore
    pMesh->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
    pMesh->mPrimitiveTypes &= ~aiPrimitiveType_POLYGON;

    // The mesh becomes NGON encoded now, during the triangulation process.
    pMesh->mPrimitiveTypes |= aiPrimitiveType_NGONEncodingFlag;

    aiFace* out = new aiFace[numOut](), *curOut = out;
    std::vector<aiVector3D> temp_verts3d(max_out+2); /* temporary storage for vertices */
    std::vector<std::vector<aiVector2D>> temp_poly(1); /* temporary storage for earcut.hpp */
    std::vector<aiVector2D>& temp_verts = temp_poly[0];
    temp_verts.reserve(max_out + 2);

    NGONEncoder ngonEncoder;

    // Apply vertex colors to represent the face winding?
#ifdef AI_BUILD_TRIANGULATE_COLOR_FACE_WINDING
    if (!pMesh->mColors[0])
        pMesh->mColors[0] = new aiColor4D[pMesh->mNumVertices];
    else
        new(pMesh->mColors[0]) aiColor4D[pMesh->mNumVertices];

    aiColor4D* clr = pMesh->mColors[0];
#endif

#ifdef AI_BUILD_TRIANGULATE_DEBUG_POLYS
    FILE* fout = fopen(POLY_OUTPUT_FILE,"a");
#endif

    const aiVector3D* verts = pMesh->mVertices;

    for( unsigned int a = 0; a < pMesh->mNumFaces; a++) {
        aiFace& face = pMesh->mFaces[a];

        unsigned int* idx = face.mIndices;
        unsigned int num = face.mNumIndices;

        // Apply vertex colors to represent the face winding?
#ifdef AI_BUILD_TRIANGULATE_COLOR_FACE_WINDING
        for (unsigned int i = 0; i < face.mNumIndices; ++i) {
            aiColor4D& c = clr[idx[i]];
            c.r = (i+1) / (float)max;
            c.b = 1.f - c.r;
        }
#endif

        aiFace* const last_face = curOut;

        // if it's a simple point,line or triangle: just copy it
        if( face.mNumIndices <= 3)
        {
            aiFace& nface = *curOut++;
            nface.mNumIndices = face.mNumIndices;
            nface.mIndices    = face.mIndices;
            face.mIndices = nullptr;

            // points and lines don't require ngon encoding (and are not supported either!)
            if (nface.mNumIndices == 3) ngonEncoder.ngonEncodeTriangle(&nface);

            continue;
        }
        // optimized code for quadrilaterals
        else if ( face.mNumIndices == 4) {

            // quads can have at maximum one concave vertex. Determine
            // this vertex (if it exists) and start tri-fanning from
            // it.
            unsigned int start_vertex = 0;
            for (unsigned int i = 0; i < 4; ++i) {
                const aiVector3D& v0 = verts[face.mIndices[(i+3) % 4]];
                const aiVector3D& v1 = verts[face.mIndices[(i+2) % 4]];
                const aiVector3D& v2 = verts[face.mIndices[(i+1) % 4]];

                const aiVector3D& v = verts[face.mIndices[i]];

                aiVector3D left = (v0-v);
                aiVector3D diag = (v1-v);
                aiVector3D right = (v2-v);

                left.Normalize();
                diag.Normalize();
                right.Normalize();

                const float angle = std::acos(left*diag) + std::acos(right*diag);
                if (angle > AI_MATH_PI_F) {
                    // this is the concave point
                    start_vertex = i;
                    break;
                }
            }

            const unsigned int temp[] = {face.mIndices[0], face.mIndices[1], face.mIndices[2], face.mIndices[3]};

            aiFace& nface = *curOut++;
            nface.mNumIndices = 3;
            nface.mIndices = face.mIndices;

            nface.mIndices[0] = temp[start_vertex];
            nface.mIndices[1] = temp[(start_vertex + 1) % 4];
            nface.mIndices[2] = temp[(start_vertex + 2) % 4];

            aiFace& sface = *curOut++;
            sface.mNumIndices = 3;
            sface.mIndices = new unsigned int[3];

            sface.mIndices[0] = temp[start_vertex];
            sface.mIndices[1] = temp[(start_vertex + 2) % 4];
            sface.mIndices[2] = temp[(start_vertex + 3) % 4];

            // prevent double deletion of the indices field
            face.mIndices = nullptr;

            ngonEncoder.ngonEncodeQuad(&nface, &sface);

            continue;
        }
        else
        {
            // A polygon with more than 3 vertices can be either concave or convex.
            // Usually everything we're getting is convex and we could easily
            // triangulate by tri-fanning. However, LightWave is probably the only
            // modeling suite to make extensive use of highly concave, monster polygons ...
            // so we need to apply the full 'ear cutting' algorithm to get it right.

            // REQUIREMENT: polygon is expected to be simple and *nearly* planar.
            // We project it onto a plane to get a 2d triangle.

            // Collect all vertices of of the polygon.
            for (unsigned int tmp = 0; tmp < num; ++tmp) {
                temp_verts3d[tmp] = verts[idx[tmp]];
            }

            // Get newell normal of the polygon. Store it for future use if it's a polygon-only mesh
            aiVector3D n;
            NewellNormal<3, 3, 3>(n, num, &temp_verts3d.front().x, &temp_verts3d.front().y, &temp_verts3d.front().z);
            if (nor_out) {
                for (unsigned int tmp = 0; tmp < num; ++tmp)
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

            temp_verts.resize(num);
            for (unsigned int tmp = 0; tmp < num; ++tmp) {
                temp_verts[tmp].x = verts[idx[tmp]][ac];
                temp_verts[tmp].y = verts[idx[tmp]][bc];
            }

            auto indices = mapbox::earcut(temp_poly);
            for (size_t i = 0; i < indices.size(); i += 3) {
                aiFace& nface = *curOut++;
                nface.mIndices = new unsigned int[3];
                nface.mNumIndices = 3;
                nface.mIndices[0] = indices[i];
                nface.mIndices[1] = indices[i + 1];
                nface.mIndices[2] = indices[i + 2];
            }

#ifdef AI_BUILD_TRIANGULATE_DEBUG_POLYS
            // plot the plane onto which we mapped the polygon to a 2D ASCII pic
            aiVector2D bmin,bmax;
            ArrayBounds(&temp_verts[0],max,bmin,bmax);

            char grid[POLY_GRID_Y][POLY_GRID_X+POLY_GRID_XPAD];
            std::fill_n((char*)grid,POLY_GRID_Y*(POLY_GRID_X+POLY_GRID_XPAD),' ');

            for (int i =0; i < max; ++i) {
                const aiVector2D& v = (temp_verts[i] - bmin) / (bmax-bmin);
                const size_t x = static_cast<size_t>(v.x*(POLY_GRID_X-1)), y = static_cast<size_t>(v.y*(POLY_GRID_Y-1));
                char* loc = grid[y]+x;
                if (grid[y][x] != ' ') {
                    for(;*loc != ' '; ++loc);
                    *loc++ = '_';
                }
                *(loc+::ai_snprintf(loc, POLY_GRID_XPAD,"%i",i)) = ' ';
            }


            for(size_t y = 0; y < POLY_GRID_Y; ++y) {
                grid[y][POLY_GRID_X+POLY_GRID_XPAD-1] = '\0';
                fprintf(fout,"%s\n",grid[y]);
            }

            fprintf(fout,"\ntriangulation sequence: ");
#endif
        }

#ifdef AI_BUILD_TRIANGULATE_DEBUG_POLYS

        for(aiFace* f = last_face; f != curOut; ++f) {
            unsigned int* i = f->mIndices;
            fprintf(fout," (%i %i %i)",i[0],i[1],i[2]);
        }

        fprintf(fout,"\n*********************************************************************\n");
        fflush(fout);

#endif

        for(aiFace* f = last_face; f != curOut; ) {
            unsigned int* i = f->mIndices;

            i[0] = idx[i[0]];
            i[1] = idx[i[1]];
            i[2] = idx[i[2]];

            // IMPROVEMENT: Polygons are not supported yet by this ngon encoding + triangulation step.
            //              So we encode polygons as regular triangles. No way to reconstruct the original
            //              polygon in this case.
            ngonEncoder.ngonEncodeTriangle(f);
            ++f;
        }

        delete[] face.mIndices;
        face.mIndices = nullptr;
    }

#ifdef AI_BUILD_TRIANGULATE_DEBUG_POLYS
    fclose(fout);
#endif

    // kill the old faces
    delete [] pMesh->mFaces;

    // ... and store the new ones
    pMesh->mFaces    = out;
    pMesh->mNumFaces = (unsigned int)(curOut-out); /* not necessarily equal to numOut */
    return true;
}

#endif // !! ASSIMP_BUILD_NO_TRIANGULATE_PROCESS
