/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2023, assimp team


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
DATA, OR PROFITS; OR BUSINESS uint32_tERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

#ifndef AI_S3OHELPER_H_INC
#define AI_S3OHELPER_H_INC

#include <assimp/ByteSwapper.h>

#include <cmath>
#include <vector>

namespace Assimp {

enum {
    S3O_PRIMTYPE_TRIANGLES = 0,
    S3O_PRIMTYPE_TRIANGLE_STRIP = 1,
    S3O_PRIMTYPE_QUADS = 2,
};

struct S3ODataVertex {
    float xpos; // position of vertex relative piece origin
    float ypos;
    float zpos;
    float xnormal; // normal of vertex relative piece rotation
    float ynormal;
    float znormal;
    float texu; // texture offset for vertex
    float texv;

    /**
     * Swap for Little Endian -> Big Endian conversation.
     */
    void swap() {
        ByteSwap::Swap(&xpos);
        ByteSwap::Swap(&ypos);
        ByteSwap::Swap(&zpos);
        ByteSwap::Swap(&xnormal);
        ByteSwap::Swap(&ynormal);
        ByteSwap::Swap(&znormal);
        ByteSwap::Swap(&texu);
        ByteSwap::Swap(&texv);
    }

    inline void FixNormalNanInf() {
        if (
                std::isnan(xnormal) || std::isinf(xnormal) ||
                std::isnan(ynormal) || std::isinf(ynormal) ||
                std::isnan(znormal) || std::isinf(znormal)) {
            xnormal = 0;
            ynormal = 0;
            znormal = 0;
        }
    }
};

const constexpr char *S3OToken = "Spring unit";

struct S3ODataHeader {
    char magic[12]; //"Spring unit\0" - S3OToken
    uint32_t version; // 0 for this version
    float radius; // radius of collision sphere
    float height; // height of whole object
    float midx; // these give the offset from origin(which is supposed to lay in the ground plane) to the middle of the unit collision sphere
    float midy;
    float midz;
    uint32_t rootPiece; // offset in file to root piece
    uint32_t collisionData; // offset in file to collision data, must be 0 for now (no collision data)
    uint32_t texture1; // offset in file to char* filename of first texture
    uint32_t texture2; // offset in file to char* filename of second texture

    /**
     * Swap for Little Endian -> Big Endian conversation.
     */
    void swap() {
        ByteSwap::Swap4(&version);
        ByteSwap::Swap(&radius);
        ByteSwap::Swap(&height);
        ByteSwap::Swap(&midx);
        ByteSwap::Swap(&midy);
        ByteSwap::Swap(&midz);
        ByteSwap::Swap4(&rootPiece);
        ByteSwap::Swap4(&collisionData); // offset in file to collision data, must be 0 for now (no collision data)
        ByteSwap::Swap4(&texture1);
        ByteSwap::Swap4(&texture2);
    }
};

struct S3ODataPiece {
    uint32_t name; // offset in file to char* name of this piece
    uint32_t numChilds; // number of sub pieces this piece has
    uint32_t childs; // file offset to table of dwords containing offsets to child pieces
    uint32_t numVertices; // number of vertices in this piece
    uint32_t vertices; // file offset to vertices in this piece
    uint32_t vertexType; // 0 for now
    uint32_t primitiveType; // type of primitives for this piece, 0=triangles,1 triangle strips,2=quads
    uint32_t vertexTableSize; // number of indexes in vertice table
    uint32_t vertexTable; // file offset to vertice table, vertice table is made up of dwords indicating vertices for this piece, to indicate end of a triangle strip use 0xffffffff
    uint32_t collisionData; // offset in file to collision data, must be 0 for now (no collision data)
    float xoffset; // offset from parent piece
    float yoffset;
    float zoffset;

    /**
     * Swap for Little Endian -> Big Endian conversation.
     */
    void swap() {
        ByteSwap::Swap4(&name);
        ByteSwap::Swap4(&numChilds);
        ByteSwap::Swap4(&childs);
        ByteSwap::Swap4(&numVertices);
        ByteSwap::Swap4(&vertices);
        ByteSwap::Swap4(&vertexType);
        ByteSwap::Swap4(&primitiveType);
        ByteSwap::Swap4(&vertexTableSize);
        ByteSwap::Swap4(&vertexTable);
        ByteSwap::Swap4(&collisionData);
        ByteSwap::Swap(&xoffset);
        ByteSwap::Swap(&yoffset);
        ByteSwap::Swap(&zoffset);
    }
};

struct S3OPiece {
public:
    std::vector<S3ODataVertex> vertices;
    std::vector<uint32_t> indices;

    uint32_t primType = S3O_PRIMTYPE_TRIANGLES;

public:
    aiVector3D offset;
    uint32_t primitiveType;

    std::vector<S3OPiece *> children;
};

class S3OVertex {
public:
    S3OVertex() :
        mPos(),
        mNormal(),
        mTc()
    {};

    ~S3OVertex() = default;

    aiVector3D mPos;
    aiVector3D mNormal;
    std::vector<aiVector3D> mTc;
};

class S3OMesh {
public:
    S3OMesh(S3ODataPiece *pPiece) :
        mVertices(),
        mIndices(),
        mPrimitiveType(pPiece->primitiveType),
        mPiece(pPiece)
    {};

    ~S3OMesh() = default;

    void Load(S3ODataVertex *pVertexList, const uint32_t *pIndexList) {
        // Retrieve vertices.
        for (uint32_t i = 0; i < mPiece->numVertices; ++i) {
            S3ODataVertex *v = pVertexList++;
#ifdef AI_BUILD_BIG_ENDIAN
        v->swap();
#endif

            auto vert = S3OVertex();
            vert.mPos.Set(v->xpos, v->ypos, v->zpos);
            vert.mNormal.Set(v->xnormal, v->ynormal, v->znormal);

            vert.mTc.push_back(aiVector3D(v->texu, v->texv, 0.0f));
            vert.mTc.push_back(aiVector3D(v->texu, v->texv, 0.0f));

            mVertices.push_back(vert);
        };

        // Retrieve indices
        for (uint32_t a = 0; a < mPiece->vertexTableSize; ++a) {
            auto tmp = *(pIndexList++);
#ifdef AI_BUILD_BIG_ENDIAN
        ByteSwap::Swap4(&tmp);
#endif
            mIndices.push_back(tmp);
    }

    };

    void Trianglize() {
        switch (mPrimitiveType) {
            case S3O_PRIMTYPE_TRIANGLES: {
            } break;
            case S3O_PRIMTYPE_TRIANGLE_STRIP: {
			if (mIndices.size() < 3) {
				mPrimitiveType = S3O_PRIMTYPE_TRIANGLES;
				mIndices.clear();
				return;
			}

			decltype(mIndices) newIndices;
			newIndices.resize(mIndices.size() * 3); // each index (can) create a new triangle

			for (size_t i = 0; (i + 2) < mIndices.size(); ++i) {
				// indices can contain end-of-strip markers (-1U)
				if (mIndices[i + 0] == -1 || mIndices[i + 1] == -1 || mIndices[i + 2] == -1)
					continue;

				newIndices.push_back(mIndices[i + 0]);
				newIndices.push_back(mIndices[i + 1]);
				newIndices.push_back(mIndices[i + 2]);
			}

			mPrimitiveType = S3O_PRIMTYPE_TRIANGLES;
			mIndices.swap(newIndices);
            } break;
            		case S3O_PRIMTYPE_QUADS: {
			if (mIndices.size() % 4 != 0) {
				mPrimitiveType = S3O_PRIMTYPE_TRIANGLES;
				mIndices.clear();
				return;
			}

			decltype(mIndices) newIndices;
			const size_t oldCount = mIndices.size();
			newIndices.resize(oldCount + oldCount / 2); // 4 indices become 6

			for (size_t i = 0, j = 0; i < mIndices.size(); i += 4) {
				newIndices[j++] = mIndices[i + 0];
				newIndices[j++] = mIndices[i + 1];
				newIndices[j++] = mIndices[i + 2];

				newIndices[j++] = mIndices[i + 0];
				newIndices[j++] = mIndices[i + 2];
				newIndices[j++] = mIndices[i + 3];
			}

			mPrimitiveType = S3O_PRIMTYPE_TRIANGLES;
			mIndices.swap(newIndices);
		} break;

		default: {
		} break;
        }
    };

public:
    std::vector<S3OVertex> mVertices;
    std::vector<uint32_t> mIndices;
    uint32_t mPrimitiveType;

private:
    S3ODataPiece *mPiece; 
};

} // namespace Assimp

#endif // AI_S3OHELPER_H_INC