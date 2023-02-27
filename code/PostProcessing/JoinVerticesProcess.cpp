/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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

/** @file Implementation of the post processing step to join identical vertices
 * for all imported meshes
 */

#ifndef ASSIMP_BUILD_NO_JOINVERTICES_PROCESS

#include "JoinVerticesProcess.h"
#include "ProcessHelper.h"
#include <assimp/Vertex.h>
#include <assimp/TinyFormatter.h>

#include <stdio.h>
#include <unordered_set>
#include <unordered_map>

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool JoinVerticesProcess::IsActive( unsigned int pFlags) const {
    return (pFlags & aiProcess_JoinIdenticalVertices) != 0;
}
// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void JoinVerticesProcess::Execute( aiScene* pScene) {
    ASSIMP_LOG_DEBUG("JoinVerticesProcess begin");

    // get the total number of vertices BEFORE the step is executed
    int iNumOldVertices = 0;
    if (!DefaultLogger::isNullLogger()) {
        for( unsigned int a = 0; a < pScene->mNumMeshes; a++)   {
            iNumOldVertices +=  pScene->mMeshes[a]->mNumVertices;
        }
    }

    // execute the step
    int iNumVertices = 0;
    for( unsigned int a = 0; a < pScene->mNumMeshes; a++) {
        iNumVertices += ProcessMesh( pScene->mMeshes[a],a);
    }

    pScene->mFlags |= AI_SCENE_FLAGS_NON_VERBOSE_FORMAT;

    // if logging is active, print detailed statistics
    if (!DefaultLogger::isNullLogger()) {
        if (iNumOldVertices == iNumVertices) {
            ASSIMP_LOG_DEBUG("JoinVerticesProcess finished ");
            return;
        }

        // Show statistics
        ASSIMP_LOG_INFO("JoinVerticesProcess finished | Verts in: ", iNumOldVertices,
            " out: ", iNumVertices, " | ~",
            ((iNumOldVertices - iNumVertices) / (float)iNumOldVertices) * 100.f );
    }
}

namespace {

bool areVerticesEqual(
    const Vertex &lhs,
    const Vertex &rhs,
    unsigned numUVChannels,
    unsigned numColorChannels) {
    // A little helper to find locally close vertices faster.
    // Try to reuse the lookup table from the last step.
    const static float epsilon = 1e-5f;
    // Squared because we check against squared length of the vector difference
    static const float squareEpsilon = epsilon * epsilon;

    // Square compare is useful for animeshes vertices compare
    if ((lhs.position - rhs.position).SquareLength() > squareEpsilon) {
        return false;
    }

    // We just test the other attributes even if they're not present in the mesh.
    // In this case they're initialized to 0 so the comparison succeeds.
    // By this method the non-present attributes are effectively ignored in the comparison.
    if ((lhs.normal - rhs.normal).SquareLength() > squareEpsilon) {
        return false;
    }

    if ((lhs.tangent - rhs.tangent).SquareLength() > squareEpsilon) {
        return false;
    }

    if ((lhs.bitangent - rhs.bitangent).SquareLength() > squareEpsilon) {
        return false;
    }

    for (unsigned i = 0; i < numUVChannels; i++) {
        if ((lhs.texcoords[i] - rhs.texcoords[i]).SquareLength() > squareEpsilon) {
            return false;
        }
    }

    for (unsigned i = 0; i < numColorChannels; i++) {
        if (GetColorDifference(lhs.colors[i], rhs.colors[i]) > squareEpsilon) {
            return false;
        }
    }

    return true;
}

template<class XMesh>
void updateXMeshVertices(XMesh *pMesh, std::vector<Vertex> &uniqueVertices) {
    // replace vertex data with the unique data sets
    pMesh->mNumVertices = (unsigned int)uniqueVertices.size();

    // ----------------------------------------------------------------------------
    // NOTE - we're *not* calling Vertex::SortBack() because it would check for
    // presence of every single vertex component once PER VERTEX. And our CPU
    // dislikes branches, even if they're easily predictable.
    // ----------------------------------------------------------------------------

    // Position, if present (check made for aiAnimMesh)
    if (pMesh->mVertices) {
        delete [] pMesh->mVertices;
        pMesh->mVertices = new aiVector3D[pMesh->mNumVertices];
        for (unsigned int a = 0; a < pMesh->mNumVertices; a++) {
            pMesh->mVertices[a] = uniqueVertices[a].position;
        }
    }

    // Normals, if present
    if (pMesh->mNormals) {
        delete [] pMesh->mNormals;
        pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];
        for( unsigned int a = 0; a < pMesh->mNumVertices; a++) {
            pMesh->mNormals[a] = uniqueVertices[a].normal;
        }
    }
    // Tangents, if present
    if (pMesh->mTangents) {
        delete [] pMesh->mTangents;
        pMesh->mTangents = new aiVector3D[pMesh->mNumVertices];
        for (unsigned int a = 0; a < pMesh->mNumVertices; a++) {
            pMesh->mTangents[a] = uniqueVertices[a].tangent;
        }
    }
    // Bitangents as well
    if (pMesh->mBitangents) {
        delete [] pMesh->mBitangents;
        pMesh->mBitangents = new aiVector3D[pMesh->mNumVertices];
        for (unsigned int a = 0; a < pMesh->mNumVertices; a++) {
            pMesh->mBitangents[a] = uniqueVertices[a].bitangent;
        }
    }
    // Vertex colors
    for (unsigned int a = 0; pMesh->HasVertexColors(a); a++) {
        delete [] pMesh->mColors[a];
        pMesh->mColors[a] = new aiColor4D[pMesh->mNumVertices];
        for( unsigned int b = 0; b < pMesh->mNumVertices; b++) {
            pMesh->mColors[a][b] = uniqueVertices[b].colors[a];
        }
    }
    // Texture coords
    for (unsigned int a = 0; pMesh->HasTextureCoords(a); a++) {
        delete [] pMesh->mTextureCoords[a];
        pMesh->mTextureCoords[a] = new aiVector3D[pMesh->mNumVertices];
        for (unsigned int b = 0; b < pMesh->mNumVertices; b++) {
            pMesh->mTextureCoords[a][b] = uniqueVertices[b].texcoords[a];
        }
    }
}

} // namespace

// ------------------------------------------------------------------------------------------------
// Unites identical vertices in the given mesh
// combine hashes
inline void hash_combine(std::size_t &) {
    // empty
}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    hash_combine(seed, rest...);
}
//template specialization for std::hash for Vertex
template<>
struct std::hash<Vertex> {
    std::size_t operator()(Vertex const& v) const noexcept {
        size_t seed = 0;
        hash_combine(seed, v.position.x ,v.position.y,v.position.z);
        return seed;
    }
};
//template specialization for std::equal_to for Vertex
template<>
struct std::equal_to<Vertex> {
    equal_to(unsigned numUVChannels, unsigned numColorChannels) :
            mNumUVChannels(numUVChannels),
            mNumColorChannels(numColorChannels) {}
    bool operator()(const Vertex &lhs, const Vertex &rhs) const {
        return areVerticesEqual(lhs, rhs, mNumUVChannels, mNumColorChannels);
    }

private:
    unsigned mNumUVChannels;
    unsigned mNumColorChannels;
};

static constexpr size_t JOINED_VERTICES_MARK = 0x80000000u;

// now start the JoinVerticesProcess
int JoinVerticesProcess::ProcessMesh( aiMesh* pMesh, unsigned int meshIndex) {
    static_assert( AI_MAX_NUMBER_OF_COLOR_SETS    == 8, "AI_MAX_NUMBER_OF_COLOR_SETS    == 8");
	static_assert( AI_MAX_NUMBER_OF_TEXTURECOORDS == 8, "AI_MAX_NUMBER_OF_TEXTURECOORDS == 8");

    // Return early if we don't have any positions
    if (!pMesh->HasPositions() || !pMesh->HasFaces()) {
        return 0;
    }

    // We should care only about used vertices, not all of them
    // (this can happen due to original file vertices buffer being used by
    // multiple meshes)
    std::vector<bool> usedVertexIndicesMask;
    usedVertexIndicesMask.resize(pMesh->mNumVertices, false);
    for (unsigned int a = 0; a < pMesh->mNumFaces; a++) {
        aiFace& face = pMesh->mFaces[a];
        for (unsigned int b = 0; b < face.mNumIndices; b++) {
            usedVertexIndicesMask[face.mIndices[b]] = true;
        }
    }

    // We'll never have more vertices afterwards.
    std::vector<Vertex> uniqueVertices;
    uniqueVertices.reserve( pMesh->mNumVertices);

    // For each vertex the index of the vertex it was replaced by.
    // Since the maximal number of vertices is 2^31-1, the most significand bit can be used to mark
    //  whether a new vertex was created for the index (true) or if it was replaced by an existing
    //  unique vertex (false). This saves an additional std::vector<bool> and greatly enhances
    //  branching performance.
    static_assert(AI_MAX_VERTICES == 0x7fffffff, "AI_MAX_VERTICES == 0x7fffffff");
    std::vector<unsigned int> replaceIndex( pMesh->mNumVertices, 0xffffffff);

    // float posEpsilonSqr;
    SpatialSort *vertexFinder = nullptr;
    SpatialSort _vertexFinder;

    typedef std::pair<SpatialSort,float> SpatPair;
    if (shared) {
        std::vector<SpatPair >* avf;
        shared->GetProperty(AI_SPP_SPATIAL_SORT,avf);
        if (avf)    {
            SpatPair& blubb = (*avf)[meshIndex];
            vertexFinder  = &blubb.first;
            // posEpsilonSqr = blubb.second;
        }
    }
    if (!vertexFinder)  {
        // bad, need to compute it.
        _vertexFinder.Fill(pMesh->mVertices, pMesh->mNumVertices, sizeof( aiVector3D));
        vertexFinder = &_vertexFinder;
        // posEpsilonSqr = ComputePositionEpsilon(pMesh);
    }

    // Again, better waste some bytes than a realloc ...
    std::vector<unsigned int> verticesFound;
    verticesFound.reserve(10);

    // Run an optimized code path if we don't have multiple UVs or vertex colors.
    // This should yield false in more than 99% of all imports ...
    const bool hasAnimMeshes = pMesh->mNumAnimMeshes > 0;

    // We'll never have more vertices afterwards.
    std::vector<std::vector<Vertex>> uniqueAnimatedVertices;
    if (hasAnimMeshes) {
        uniqueAnimatedVertices.resize(pMesh->mNumAnimMeshes);
        for (unsigned int animMeshIndex = 0; animMeshIndex < pMesh->mNumAnimMeshes; animMeshIndex++) {
            uniqueAnimatedVertices[animMeshIndex].reserve(pMesh->mNumVertices);
        }
    }
    // a map that maps a vertex to its new index
    const auto numBuckets = pMesh->mNumVertices;
    const auto hasher = std::hash<Vertex>();
    const auto comparator = std::equal_to<Vertex>(
            pMesh->GetNumUVChannels(),
            pMesh->GetNumColorChannels());
    std::unordered_map<Vertex, int> vertex2Index(numBuckets, hasher, comparator);
    // we can not end up with more vertices than we started with
    vertex2Index.reserve(pMesh->mNumVertices);
    // Now check each vertex if it brings something new to the table
    int newIndex = 0;
    for( unsigned int a = 0; a < pMesh->mNumVertices; a++)  {
        // if the vertex is unused Do nothing
        if (!usedVertexIndicesMask[a]) {
            continue;
        }
        // collect the vertex data
        Vertex v(pMesh,a);
        // is the vertex already in the map?
        auto it = vertex2Index.find(v);
        // if the vertex is not in the map then it is a new vertex add it.
        if (it == vertex2Index.end()) {
            // this is a new vertex give it a new index
            vertex2Index[v] = newIndex;
            //keep track of its index and increment 1
            replaceIndex[a] = newIndex++;
            // add the vertex to the unique vertices
            uniqueVertices.push_back(v);
            if (hasAnimMeshes) {
                for (unsigned int animMeshIndex = 0; animMeshIndex < pMesh->mNumAnimMeshes; animMeshIndex++) {
                    uniqueAnimatedVertices[animMeshIndex].emplace_back(pMesh->mAnimMeshes[animMeshIndex], a);
                }
            }
        } else{
            // if the vertex is already there just find the replace index that is appropriate to it
			// mark it with JOINED_VERTICES_MARK
            replaceIndex[a] = it->second | JOINED_VERTICES_MARK;
        }
    }

    if (!DefaultLogger::isNullLogger() && DefaultLogger::get()->getLogSeverity() == Logger::VERBOSE)    {
        ASSIMP_LOG_VERBOSE_DEBUG(
            "Mesh ",meshIndex,
            " (",
            (pMesh->mName.length ? pMesh->mName.data : "unnamed"),
            ") | Verts in: ",pMesh->mNumVertices,
            " out: ",
            uniqueVertices.size(),
            " | ~",
            ((pMesh->mNumVertices - uniqueVertices.size()) / (float)pMesh->mNumVertices) * 100.f,
            "%"
        );
    }

    updateXMeshVertices(pMesh, uniqueVertices);
    if (hasAnimMeshes) {
        for (unsigned int animMeshIndex = 0; animMeshIndex < pMesh->mNumAnimMeshes; animMeshIndex++) {
            updateXMeshVertices(pMesh->mAnimMeshes[animMeshIndex], uniqueAnimatedVertices[animMeshIndex]);
        }
    }

    // adjust the indices in all faces
    for( unsigned int a = 0; a < pMesh->mNumFaces; a++) {
        aiFace& face = pMesh->mFaces[a];
        for( unsigned int b = 0; b < face.mNumIndices; b++) {
            face.mIndices[b] = replaceIndex[face.mIndices[b]] & ~JOINED_VERTICES_MARK;
        }
    }

    // adjust bone vertex weights.
    for( int a = 0; a < (int)pMesh->mNumBones; a++) {
        aiBone* bone = pMesh->mBones[a];
        std::vector<aiVertexWeight> newWeights;
        newWeights.reserve( bone->mNumWeights);

        if (nullptr != bone->mWeights) {
            for ( unsigned int b = 0; b < bone->mNumWeights; b++ ) {
                const aiVertexWeight& ow = bone->mWeights[ b ];
                // if the vertex is a unique one, translate it
				// filter out joined vertices by JOINED_VERTICES_MARK.
                if ( !( replaceIndex[ ow.mVertexId ] & JOINED_VERTICES_MARK ) ) {
                    aiVertexWeight nw;
                    nw.mVertexId = replaceIndex[ ow.mVertexId ];
                    nw.mWeight = ow.mWeight;
                    newWeights.push_back( nw );
                }
            }
        } else {
            ASSIMP_LOG_ERROR( "X-Export: aiBone shall contain weights, but pointer to them is nullptr." );
        }

        if (newWeights.size() > 0) {
            // kill the old and replace them with the translated weights
            delete [] bone->mWeights;
            bone->mNumWeights = (unsigned int)newWeights.size();

            bone->mWeights = new aiVertexWeight[bone->mNumWeights];
            memcpy( bone->mWeights, &newWeights[0], bone->mNumWeights * sizeof( aiVertexWeight));
        }
    }
    return pMesh->mNumVertices;
}

#endif // !! ASSIMP_BUILD_NO_JOINVERTICES_PROCESS
