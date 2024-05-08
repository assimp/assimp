/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

/** @file mesh.h
 *  @brief Declares the data structures in which the imported geometry is
    returned by ASSIMP: aiMesh, aiFace and aiBone data structures.
 */
#pragma once
#ifndef AI_MESH_H_INC
#define AI_MESH_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4351)
#endif // _MSC_VER

#include <assimp/aabb.h>
#include <assimp/types.h>

#ifdef __cplusplus
#include <unordered_set>

extern "C" {
#endif

// ---------------------------------------------------------------------------
// Limits. These values are required to match the settings Assimp was
// compiled against. Therefore, do not redefine them unless you build the
// library from source using the same definitions.
// ---------------------------------------------------------------------------

/** @def AI_MAX_FACE_INDICES
 *  Maximum number of indices per face (polygon). */

#ifndef AI_MAX_FACE_INDICES
#define AI_MAX_FACE_INDICES 0x7fff
#endif

/** @def AI_MAX_BONE_WEIGHTS
 *  Maximum number of indices per face (polygon). */

#ifndef AI_MAX_BONE_WEIGHTS
#define AI_MAX_BONE_WEIGHTS 0x7fffffff
#endif

/** @def AI_MAX_VERTICES
 *  Maximum number of vertices per mesh.  */

#ifndef AI_MAX_VERTICES
#define AI_MAX_VERTICES 0x7fffffff
#endif

/** @def AI_MAX_FACES
 *  Maximum number of faces per mesh. */

#ifndef AI_MAX_FACES
#define AI_MAX_FACES 0x7fffffff
#endif

/** @def AI_MAX_NUMBER_OF_COLOR_SETS
 *  Supported number of vertex color sets per mesh. */

#ifndef AI_MAX_NUMBER_OF_COLOR_SETS
#define AI_MAX_NUMBER_OF_COLOR_SETS 0x8
#endif // !! AI_MAX_NUMBER_OF_COLOR_SETS

/** @def AI_MAX_NUMBER_OF_TEXTURECOORDS
 *  Supported number of texture coord sets (UV(W) channels) per mesh */

#ifndef AI_MAX_NUMBER_OF_TEXTURECOORDS
#define AI_MAX_NUMBER_OF_TEXTURECOORDS 0x8
#endif // !! AI_MAX_NUMBER_OF_TEXTURECOORDS

// ---------------------------------------------------------------------------
/**
 * @brief A single face in a mesh, referring to multiple vertices.
 *
 * If mNumIndices is 3, we call the face 'triangle', for mNumIndices > 3
 * it's called 'polygon' (hey, that's just a definition!).
 * <br>
 * aiMesh::mPrimitiveTypes can be queried to quickly examine which types of
 * primitive are actually present in a mesh. The #aiProcess_SortByPType flag
 * executes a special post-processing algorithm which splits meshes with
 * *different* primitive types mixed up (e.g. lines and triangles) in several
 * 'clean' sub-meshes. Furthermore there is a configuration option (
 * #AI_CONFIG_PP_SBP_REMOVE) to force #aiProcess_SortByPType to remove
 * specific kinds of primitives from the imported scene, completely and forever.
 * In many cases you'll probably want to set this setting to
 * @code
 * aiPrimitiveType_LINE|aiPrimitiveType_POINT
 * @endcode
 * Together with the #aiProcess_Triangulate flag you can then be sure that
 * #aiFace::mNumIndices is always 3.
 * @note Take a look at the @link data Data Structures page @endlink for
 * more information on the layout and winding order of a face.
 */
struct aiFace {
    //! Number of indices defining this face.
    //! The maximum value for this member is #AI_MAX_FACE_INDICES.
    unsigned int mNumIndices;

    //! Pointer to the indices array. Size of the array is given in numIndices.
    unsigned int *mIndices;

#ifdef __cplusplus

    //! @brief Default constructor.
    aiFace() AI_NO_EXCEPT
            : mNumIndices(0),
              mIndices(nullptr) {
        // empty
    }

    //! @brief Default destructor. Delete the index array
    ~aiFace() {
        delete[] mIndices;
    }

    //! @brief Copy constructor. Copy the index array
    aiFace(const aiFace &o) :
            mNumIndices(0), mIndices(nullptr) {
        *this = o;
    }

    //! @brief Assignment operator. Copy the index array
    aiFace &operator=(const aiFace &o) {
        if (&o == this) {
            return *this;
        }

        delete[] mIndices;
        mNumIndices = o.mNumIndices;
        if (mNumIndices) {
            mIndices = new unsigned int[mNumIndices];
            ::memcpy(mIndices, o.mIndices, mNumIndices * sizeof(unsigned int));
        } else {
            mIndices = nullptr;
        }

        return *this;
    }

    //! @brief Comparison operator. Checks whether the index array of two faces is identical.
    bool operator==(const aiFace &o) const {
        if (mIndices == o.mIndices) {
            return true;
        }

        if (nullptr != mIndices && mNumIndices != o.mNumIndices) {
            return false;
        }

        if (nullptr == mIndices) {
            return false;
        }

        for (unsigned int i = 0; i < this->mNumIndices; ++i) {
            if (mIndices[i] != o.mIndices[i]) {
                return false;
            }
        }

        return true;
    }

    //! @brief Inverse comparison operator. Checks whether the index
    //! array of two faces is NOT identical
    bool operator!=(const aiFace &o) const {
        return !(*this == o);
    }
#endif // __cplusplus
}; // struct aiFace

// ---------------------------------------------------------------------------
/** @brief A single influence of a bone on a vertex.
 */
struct aiVertexWeight {
    //! Index of the vertex which is influenced by the bone.
    unsigned int mVertexId;

    //! The strength of the influence in the range (0...1).
    //! The influence from all bones at one vertex amounts to 1.
    ai_real mWeight;

#ifdef __cplusplus

    //! @brief Default constructor
    aiVertexWeight() AI_NO_EXCEPT
            : mVertexId(0),
              mWeight(0.0f) {
        // empty
    }

    //! @brief Initialization from a given index and vertex weight factor
    //! \param pID ID
    //! \param pWeight Vertex weight factor
    aiVertexWeight(unsigned int pID, float pWeight) :
            mVertexId(pID), mWeight(pWeight) {
        // empty
    }

    bool operator==(const aiVertexWeight &rhs) const {
        return (mVertexId == rhs.mVertexId && mWeight == rhs.mWeight);
    }

    bool operator!=(const aiVertexWeight &rhs) const {
        return (*this == rhs);
    }

#endif // __cplusplus
};

// Forward declare aiNode (pointer use only)
struct aiNode;

// ---------------------------------------------------------------------------
/** @brief A single bone of a mesh.
 *
 *  A bone has a name by which it can be found in the frame hierarchy and by
 *  which it can be addressed by animations. In addition it has a number of
 *  influences on vertices, and a matrix relating the mesh position to the
 *  position of the bone at the time of binding.
 */
struct aiBone {
    /**
     * The name of the bone.
     */
    C_STRUCT aiString mName;

    /**
     * The number of vertices affected by this bone.
     * The maximum value for this member is #AI_MAX_BONE_WEIGHTS.
     */
    unsigned int mNumWeights;

#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
    /**
     * The bone armature node - used for skeleton conversion
     * you must enable aiProcess_PopulateArmatureData to populate this
     */
    C_STRUCT aiNode *mArmature;

    /**
     * The bone node in the scene - used for skeleton conversion
     * you must enable aiProcess_PopulateArmatureData to populate this
     */
    C_STRUCT aiNode *mNode;

#endif
    /**
     * The influence weights of this bone, by vertex index.
     */
    C_STRUCT aiVertexWeight *mWeights;

    /**
     * Matrix that transforms from mesh space to bone space in bind pose.
     *
     * This matrix describes the position of the mesh
     * in the local space of this bone when the skeleton was bound.
     * Thus it can be used directly to determine a desired vertex position,
     * given the world-space transform of the bone when animated,
     * and the position of the vertex in mesh space.
     *
     * It is sometimes called an inverse-bind matrix,
     * or inverse bind pose matrix.
     */
    C_STRUCT aiMatrix4x4 mOffsetMatrix;

#ifdef __cplusplus

    ///	@brief  Default constructor
    aiBone() AI_NO_EXCEPT
            : mName(),
              mNumWeights(0),
#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
              mArmature(nullptr),
              mNode(nullptr),
#endif
              mWeights(nullptr),
              mOffsetMatrix() {
        // empty
    }

    /// @brief  Copy constructor
    aiBone(const aiBone &other) :
            mName(other.mName),
            mNumWeights(other.mNumWeights),
#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
              mArmature(nullptr),
              mNode(nullptr),
#endif
            mWeights(nullptr),
            mOffsetMatrix(other.mOffsetMatrix) {
        copyVertexWeights(other);
    }

    void copyVertexWeights( const aiBone &other ) {
        if (other.mWeights == nullptr || other.mNumWeights == 0) {
            mWeights = nullptr;
            mNumWeights = 0;
            return;
        }

        mNumWeights = other.mNumWeights;
        if (mWeights) {
            delete[] mWeights;
        }

        mWeights = new aiVertexWeight[mNumWeights];
        ::memcpy(mWeights, other.mWeights, mNumWeights * sizeof(aiVertexWeight));
    }

    //! @brief Assignment operator
    aiBone &operator = (const aiBone &other) {
        if (this == &other) {
            return *this;
        }

        mName = other.mName;
        mNumWeights = other.mNumWeights;
        mOffsetMatrix = other.mOffsetMatrix;
        copyVertexWeights(other);

        return *this;
    }

    /// @brief Compare operator.
    bool operator==(const aiBone &rhs) const {
        if (mName != rhs.mName || mNumWeights != rhs.mNumWeights ) {
            return false;
        }

        for (size_t i = 0; i < mNumWeights; ++i) {
            if (mWeights[i] != rhs.mWeights[i]) {
                return false;
            }
        }

        return true;
    }
    //! @brief Destructor - deletes the array of vertex weights
    ~aiBone() {
        delete[] mWeights;
    }
#endif // __cplusplus
};

// ---------------------------------------------------------------------------
/** @brief Enumerates the types of geometric primitives supported by Assimp.
 *
 *  @see aiFace Face data structure
 *  @see aiProcess_SortByPType Per-primitive sorting of meshes
 *  @see aiProcess_Triangulate Automatic triangulation
 *  @see AI_CONFIG_PP_SBP_REMOVE Removal of specific primitive types.
 */
enum aiPrimitiveType {
    /**
     * @brief A point primitive.
     *
     * This is just a single vertex in the virtual world,
     * #aiFace contains just one index for such a primitive.
     */
    aiPrimitiveType_POINT = 0x1,

    /**
     * @brief A line primitive.
     *
     * This is a line defined through a start and an end position.
     * #aiFace contains exactly two indices for such a primitive.
     */
    aiPrimitiveType_LINE = 0x2,

    /**
     * @brief A triangular primitive.
     *
     * A triangle consists of three indices.
     */
    aiPrimitiveType_TRIANGLE = 0x4,

    /**
     * @brief A higher-level polygon with more than 3 edges.
     *
     * A triangle is a polygon, but polygon in this context means
     * "all polygons that are not triangles". The "Triangulate"-Step
     * is provided for your convenience, it splits all polygons in
     * triangles (which are much easier to handle).
     */
    aiPrimitiveType_POLYGON = 0x8,

    /**
     * @brief A flag to determine whether this triangles only mesh is NGON encoded.
     *
     * NGON encoding is a special encoding that tells whether 2 or more consecutive triangles
     * should be considered as a triangle fan. This is identified by looking at the first vertex index.
     * 2 consecutive triangles with the same 1st vertex index are part of the same
     * NGON.
     *
     * At the moment, only quads (concave or convex) are supported, meaning that polygons are 'seen' as
     * triangles, as usual after a triangulation pass.
     *
     * To get an NGON encoded mesh, please use the aiProcess_Triangulate post process.
     *
     * @see aiProcess_Triangulate
     * @link https://github.com/KhronosGroup/glTF/pull/1620
     */
    aiPrimitiveType_NGONEncodingFlag = 0x10,

    /**
     * This value is not used. It is just here to force the
     * compiler to map this enum to a 32 Bit integer.
     */
#ifndef SWIG
    _aiPrimitiveType_Force32Bit = INT_MAX
#endif
}; //! enum aiPrimitiveType

// Get the #aiPrimitiveType flag for a specific number of face indices
#define AI_PRIMITIVE_TYPE_FOR_N_INDICES(n) \
    ((n) > 3 ? aiPrimitiveType_POLYGON : (aiPrimitiveType)(1u << ((n)-1)))

// ---------------------------------------------------------------------------
/** @brief An AnimMesh is an attachment to an #aiMesh stores per-vertex
 *  animations for a particular frame.
 *
 *  You may think of an #aiAnimMesh as a `patch` for the host mesh, which
 *  replaces only certain vertex data streams at a particular time.
 *  Each mesh stores n attached attached meshes (#aiMesh::mAnimMeshes).
 *  The actual relationship between the time line and anim meshes is
 *  established by #aiMeshAnim, which references singular mesh attachments
 *  by their ID and binds them to a time offset.
*/
struct aiAnimMesh {
    /**Anim Mesh name */
    C_STRUCT aiString mName;

    /** Replacement for aiMesh::mVertices. If this array is non-nullptr,
     *  it *must* contain mNumVertices entries. The corresponding
     *  array in the host mesh must be non-nullptr as well - animation
     *  meshes may neither add or nor remove vertex components (if
     *  a replacement array is nullptr and the corresponding source
     *  array is not, the source data is taken instead)*/
    C_STRUCT aiVector3D *mVertices;

    /** Replacement for aiMesh::mNormals.  */
    C_STRUCT aiVector3D *mNormals;

    /** Replacement for aiMesh::mTangents. */
    C_STRUCT aiVector3D *mTangents;

    /** Replacement for aiMesh::mBitangents. */
    C_STRUCT aiVector3D *mBitangents;

    /** Replacement for aiMesh::mColors */
    C_STRUCT aiColor4D *mColors[AI_MAX_NUMBER_OF_COLOR_SETS];

    /** Replacement for aiMesh::mTextureCoords */
    C_STRUCT aiVector3D *mTextureCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];

    /** The number of vertices in the aiAnimMesh, and thus the length of all
     * the member arrays.
     *
     * This has always the same value as the mNumVertices property in the
     * corresponding aiMesh. It is duplicated here merely to make the length
     * of the member arrays accessible even if the aiMesh is not known, e.g.
     * from language bindings.
     */
    unsigned int mNumVertices;

    /**
     * Weight of the AnimMesh.
     */
    float mWeight;

#ifdef __cplusplus
    /// @brief  The class constructor.
    aiAnimMesh() AI_NO_EXCEPT :
            mVertices(nullptr),
            mNormals(nullptr),
            mTangents(nullptr),
            mBitangents(nullptr),
            mColors {nullptr},
            mTextureCoords{nullptr},
            mNumVertices(0),
            mWeight(0.0f) {
        // empty
    }

    /// @brief The class destructor.
    ~aiAnimMesh() {
        delete[] mVertices;
        delete[] mNormals;
        delete[] mTangents;
        delete[] mBitangents;
        for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++) {
            delete[] mTextureCoords[a];
        }
        for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++) {
            delete[] mColors[a];
        }
    }

    /**
     *  @brief Check whether the anim-mesh overrides the vertex positions
     *         of its host mesh.
     *  @return true if positions are stored, false if not.
     */
    bool HasPositions() const {
        return mVertices != nullptr;
    }

    /**
     *  @brief Check whether the anim-mesh overrides the vertex normals
     *         of its host mesh
     *  @return true if normals are stored, false if not.
     */
    bool HasNormals() const {
        return mNormals != nullptr;
    }

    /**
     *  @brief Check whether the anim-mesh overrides the vertex tangents
     *         and bitangents of its host mesh. As for aiMesh,
     *         tangents and bitangents always go together.
     *  @return true if tangents and bi-tangents are stored, false if not.
     */
    bool HasTangentsAndBitangents() const {
        return mTangents != nullptr;
    }

    /**
     *  @brief Check whether the anim mesh overrides a particular
     *         set of vertex colors on his host mesh.
     *  @param pIndex 0<index<AI_MAX_NUMBER_OF_COLOR_SETS
     *  @return true if vertex colors are stored, false if not.
     */

    bool HasVertexColors(unsigned int pIndex) const {
        return pIndex >= AI_MAX_NUMBER_OF_COLOR_SETS ? false : mColors[pIndex] != nullptr;
    }

    /**
     *  @brief Check whether the anim mesh overrides a particular
     *        set of texture coordinates on his host mesh.
     *  @param pIndex 0<index<AI_MAX_NUMBER_OF_TEXTURECOORDS
     *  @return true if texture coordinates are stored, false if not.
     */
    bool HasTextureCoords(unsigned int pIndex) const {
        return pIndex >= AI_MAX_NUMBER_OF_TEXTURECOORDS ? false : mTextureCoords[pIndex] != nullptr;
    }

#endif
};

// ---------------------------------------------------------------------------
/** @brief Enumerates the methods of mesh morphing supported by Assimp.
 */
enum aiMorphingMethod {
    /** Morphing method to be determined */
    aiMorphingMethod_UNKNOWN = 0x0,

    /** Interpolation between morph targets */
    aiMorphingMethod_VERTEX_BLEND = 0x1,

    /** Normalized morphing between morph targets  */
    aiMorphingMethod_MORPH_NORMALIZED = 0x2,

    /** Relative morphing between morph targets  */
    aiMorphingMethod_MORPH_RELATIVE = 0x3,

/** This value is not used. It is just here to force the
     *  compiler to map this enum to a 32 Bit integer.
     */
#ifndef SWIG
    _aiMorphingMethod_Force32Bit = INT_MAX
#endif
}; //! enum aiMorphingMethod

// ---------------------------------------------------------------------------
/** @brief A mesh represents a geometry or model with a single material.
 *
 * It usually consists of a number of vertices and a series of primitives/faces
 * referencing the vertices. In addition there might be a series of bones, each
 * of them addressing a number of vertices with a certain weight. Vertex data
 * is presented in channels with each channel containing a single per-vertex
 * information such as a set of texture coordinates or a normal vector.
 * If a data pointer is non-null, the corresponding data stream is present.
 * From C++-programs you can also use the comfort functions Has*() to
 * test for the presence of various data streams.
 *
 * A Mesh uses only a single material which is referenced by a material ID.
 * @note The mPositions member is usually not optional. However, vertex positions
 * *could* be missing if the #AI_SCENE_FLAGS_INCOMPLETE flag is set in
 * @code
 * aiScene::mFlags
 * @endcode
 */
struct aiMesh {
    /**
     * Bitwise combination of the members of the #aiPrimitiveType enum.
     * This specifies which types of primitives are present in the mesh.
     * The "SortByPrimitiveType"-Step can be used to make sure the
     * output meshes consist of one primitive type each.
     */
    unsigned int mPrimitiveTypes;

    /**
     * The number of vertices in this mesh.
     * This is also the size of all of the per-vertex data arrays.
     * The maximum value for this member is #AI_MAX_VERTICES.
     */
    unsigned int mNumVertices;

    /**
     * The number of primitives (triangles, polygons, lines) in this  mesh.
     * This is also the size of the mFaces array.
     * The maximum value for this member is #AI_MAX_FACES.
     */
    unsigned int mNumFaces;

    /**
     * @brief Vertex positions.
     * 
     * This array is always present in a mesh. The array is
     * mNumVertices in size.
     */
    C_STRUCT aiVector3D *mVertices;

    /**
     * @brief Vertex normals.
     * 
     * The array contains normalized vectors, nullptr if not present.
     * The array is mNumVertices in size. Normals are undefined for
     * point and line primitives. A mesh consisting of points and
     * lines only may not have normal vectors. Meshes with mixed
     * primitive types (i.e. lines and triangles) may have normals,
     * but the normals for vertices that are only referenced by
     * point or line primitives are undefined and set to QNaN (WARN:
     * qNaN compares to inequal to *everything*, even to qNaN itself.
     * Using code like this to check whether a field is qnan is:
     * @code
     * #define IS_QNAN(f) (f != f)
     * @endcode
     * still dangerous because even 1.f == 1.f could evaluate to false! (
     * remember the subtleties of IEEE754 artithmetics). Use stuff like
     * @c fpclassify instead.
     * @note Normal vectors computed by Assimp are always unit-length.
     * However, this needn't apply for normals that have been taken
     * directly from the model file.
     */
    C_STRUCT aiVector3D *mNormals;

    /**
     * @brief Vertex tangents.
     * 
     * The tangent of a vertex points in the direction of the positive
     * X texture axis. The array contains normalized vectors, nullptr if
     * not present. The array is mNumVertices in size. A mesh consisting
     * of points and lines only may not have normal vectors. Meshes with
     * mixed primitive types (i.e. lines and triangles) may have
     * normals, but the normals for vertices that are only referenced by
     * point or line primitives are undefined and set to qNaN.  See
     * the #mNormals member for a detailed discussion of qNaNs.
     * @note If the mesh contains tangents, it automatically also
     * contains bitangents.
     */
    C_STRUCT aiVector3D *mTangents;

    /**
     * @brief Vertex bitangents.
     * 
     * The bitangent of a vertex points in the direction of the positive
     * Y texture axis. The array contains normalized vectors, nullptr if not
     * present. The array is mNumVertices in size.
     * @note If the mesh contains tangents, it automatically also contains
     * bitangents.
     */
    C_STRUCT aiVector3D *mBitangents;

    /**
     * @brief Vertex color sets.
     * 
     * A mesh may contain 0 to #AI_MAX_NUMBER_OF_COLOR_SETS vertex
     * colors per vertex. nullptr if not present. Each array is
     * mNumVertices in size if present.
     */
    C_STRUCT aiColor4D *mColors[AI_MAX_NUMBER_OF_COLOR_SETS];

    /**
     * @brief Vertex texture coordinates, also known as UV channels.
     * 
     * A mesh may contain 0 to AI_MAX_NUMBER_OF_TEXTURECOORDS per
     * vertex. nullptr if not present. The array is mNumVertices in size.
     */
    C_STRUCT aiVector3D *mTextureCoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];

    /**
     * @brief Specifies the number of components for a given UV channel.
     * 
     * Up to three channels are supported (UVW, for accessing volume
     * or cube maps). If the value is 2 for a given channel n, the
     * component p.z of mTextureCoords[n][p] is set to 0.0f.
     * If the value is 1 for a given channel, p.y is set to 0.0f, too.
     * @note 4D coordinates are not supported
     */
    unsigned int mNumUVComponents[AI_MAX_NUMBER_OF_TEXTURECOORDS];

    /**
     * @brief The faces the mesh is constructed from.
     * 
     * Each face refers to a number of vertices by their indices.
     * This array is always present in a mesh, its size is given
     *  in mNumFaces. If the #AI_SCENE_FLAGS_NON_VERBOSE_FORMAT
     * is NOT set each face references an unique set of vertices.
     */
    C_STRUCT aiFace *mFaces;

    /**
    * The number of bones this mesh contains. Can be 0, in which case the mBones array is nullptr.
    */
    unsigned int mNumBones;

    /**
     * @brief The bones of this mesh.
     * 
     * A bone consists of a name by which it can be found in the
     * frame hierarchy and a set of vertex weights.
     */
    C_STRUCT aiBone **mBones;

    /**
     * @brief The material used by this mesh.
     * 
     * A mesh uses only a single material. If an imported model uses
     * multiple materials, the import splits up the mesh. Use this value
     * as index into the scene's material list.
     */
    unsigned int mMaterialIndex;

    /**
     *  Name of the mesh. Meshes can be named, but this is not a
     *  requirement and leaving this field empty is totally fine.
     *  There are mainly three uses for mesh names:
     *   - some formats name nodes and meshes independently.
     *   - importers tend to split meshes up to meet the
     *      one-material-per-mesh requirement. Assigning
     *      the same (dummy) name to each of the result meshes
     *      aids the caller at recovering the original mesh
     *      partitioning.
     *   - Vertex animations refer to meshes by their names.
     */
    C_STRUCT aiString mName;

    /**
     * The number of attachment meshes.
     * Currently known to work with loaders:
     * - Collada
     * - gltf
     */
    unsigned int mNumAnimMeshes;

    /**
     * Attachment meshes for this mesh, for vertex-based animation.
     * Attachment meshes carry replacement data for some of the
     * mesh'es vertex components (usually positions, normals).
     * Currently known to work with loaders:
     * - Collada
     * - gltf
     */
    C_STRUCT aiAnimMesh **mAnimMeshes;

    /**
     *  Method of morphing when anim-meshes are specified.
     *  @see aiMorphingMethod to learn more about the provided morphing targets.
     */
    enum aiMorphingMethod mMethod;

    /**
     *  The bounding box.
     */
    C_STRUCT aiAABB mAABB;

    /**
     * Vertex UV stream names. Pointer to array of size AI_MAX_NUMBER_OF_TEXTURECOORDS
     */
    C_STRUCT aiString **mTextureCoordsNames;

#ifdef __cplusplus

    //! The default class constructor.
    aiMesh() AI_NO_EXCEPT
            : mPrimitiveTypes(0),
              mNumVertices(0),
              mNumFaces(0),
              mVertices(nullptr),
              mNormals(nullptr),
              mTangents(nullptr),
              mBitangents(nullptr),
              mColors{nullptr},
              mTextureCoords{nullptr},
              mNumUVComponents{0},
              mFaces(nullptr),
              mNumBones(0),
              mBones(nullptr),
              mMaterialIndex(0),
              mNumAnimMeshes(0),
              mAnimMeshes(nullptr),
              mMethod(aiMorphingMethod_UNKNOWN),
              mAABB(),
              mTextureCoordsNames(nullptr) {
        // empty
    }

    //! @brief The class destructor.
    ~aiMesh() {
        delete[] mVertices;
        delete[] mNormals;
        delete[] mTangents;
        delete[] mBitangents;
        for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++) {
            delete[] mTextureCoords[a];
        }

        if (mTextureCoordsNames) {
            for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; a++) {
                delete mTextureCoordsNames[a];
            }
            delete[] mTextureCoordsNames;
        }

        for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; a++) {
            delete[] mColors[a];
        }

        // DO NOT REMOVE THIS ADDITIONAL CHECK
        if (mNumBones && mBones) {
            std::unordered_set<const aiBone *> bones;
            for (unsigned int a = 0; a < mNumBones; a++) {
                if (mBones[a]) {
                    bones.insert(mBones[a]);
                }
            }
            for (const aiBone *bone: bones) {
                delete bone;
            }
            delete[] mBones;
        }

        if (mNumAnimMeshes && mAnimMeshes) {
            for (unsigned int a = 0; a < mNumAnimMeshes; a++) {
                delete mAnimMeshes[a];
            }
            delete[] mAnimMeshes;
        }

        delete[] mFaces;
    }

    //! @brief Check whether the mesh contains positions. Provided no special
    //!        scene flags are set, this will always be true
    //! @return true, if positions are stored, false if not.
    bool HasPositions() const {
        return mVertices != nullptr && mNumVertices > 0;
    }

    //! @brief Check whether the mesh contains faces. If no special scene flags
    //!        are set this should always return true
    //! @return true, if faces are stored, false if not.
    bool HasFaces() const {
        return mFaces != nullptr && mNumFaces > 0;
    }

    //! @brief Check whether the mesh contains normal vectors
    //! @return true, if normals are stored, false if not.
    bool HasNormals() const {
        return mNormals != nullptr && mNumVertices > 0;
    }

    //! @brief Check whether the mesh contains tangent and bitangent vectors.
    //! 
    //! It is not possible that it contains tangents and no bitangents
    //! (or the other way round). The existence of one of them
    //! implies that the second is there, too.
    //! @return true, if tangents and bi-tangents are stored, false if not.
    bool HasTangentsAndBitangents() const {
        return mTangents != nullptr && mBitangents != nullptr && mNumVertices > 0;
    }

    //! @brief Check whether the mesh contains a vertex color set
    //! @param index    Index of the vertex color set
    //! @return true, if vertex colors are stored, false if not.
    bool HasVertexColors(unsigned int index) const {
        if (index >= AI_MAX_NUMBER_OF_COLOR_SETS) {
            return false;
        }
        return mColors[index] != nullptr && mNumVertices > 0;        
    }

    //! @brief Check whether the mesh contains a texture coordinate set
    //! @param index    Index of the texture coordinates set
    //! @return true, if texture coordinates are stored, false if not.
    bool HasTextureCoords(unsigned int index) const {
        if (index >= AI_MAX_NUMBER_OF_TEXTURECOORDS) {
            return false;
        }
        return (mTextureCoords[index] != nullptr && mNumVertices > 0);
    }

    //! @brief Get the number of UV channels the mesh contains.
    //! @return the number of stored uv-channels.
    unsigned int GetNumUVChannels() const {
        unsigned int n(0);
        while (n < AI_MAX_NUMBER_OF_TEXTURECOORDS && mTextureCoords[n]) {
            ++n;
        }

        return n;
    }

    //! @brief Get the number of vertex color channels the mesh contains.
    //! @return The number of stored color channels.
    unsigned int GetNumColorChannels() const {
        unsigned int n(0);
        while (n < AI_MAX_NUMBER_OF_COLOR_SETS && mColors[n]) {
            ++n;
        }
        return n;
    }

    //! @brief Check whether the mesh contains bones.
    //! @return true, if bones are stored.
    bool HasBones() const {
        return mBones != nullptr && mNumBones > 0;
    }

    //! @brief  Check whether the mesh contains a texture coordinate set name
    //! @param pIndex Index of the texture coordinates set
    //! @return true, if texture coordinates for the index exists.
    bool HasTextureCoordsName(unsigned int pIndex) const {
        if (mTextureCoordsNames == nullptr || pIndex >= AI_MAX_NUMBER_OF_TEXTURECOORDS) {
            return false;
        }
        return mTextureCoordsNames[pIndex] != nullptr;
    }

    //! @brief  Set a texture coordinate set name
    //! @param pIndex Index of the texture coordinates set
    //! @param texCoordsName name of the texture coordinate set
    void SetTextureCoordsName(unsigned int pIndex, const aiString &texCoordsName) {
        if (pIndex >= AI_MAX_NUMBER_OF_TEXTURECOORDS) {
            return;
        }

        if (mTextureCoordsNames == nullptr) {
            // Construct and null-init array
            mTextureCoordsNames = new aiString *[AI_MAX_NUMBER_OF_TEXTURECOORDS];
            for (size_t i=0; i<AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
                mTextureCoordsNames[i] = nullptr;
            }
        }

        if (texCoordsName.length == 0) {
            delete mTextureCoordsNames[pIndex];
            mTextureCoordsNames[pIndex] = nullptr;
            return;
        }

        if (mTextureCoordsNames[pIndex] == nullptr) {
            mTextureCoordsNames[pIndex] = new aiString(texCoordsName);
            return;
        }

        *mTextureCoordsNames[pIndex] = texCoordsName;
    }

    //! @brief  Get a texture coordinate set name
    //! @param  pIndex Index of the texture coordinates set
    //! @return The texture coordinate name.
    const aiString *GetTextureCoordsName(unsigned int index) const {
        if (mTextureCoordsNames == nullptr || index >= AI_MAX_NUMBER_OF_TEXTURECOORDS) {
            return nullptr;
        }

        return mTextureCoordsNames[index];
    }

#endif // __cplusplus
};

/**
 * @brief  A skeleton bone represents a single bone is a skeleton structure.
 *
 * Skeleton-Animations can be represented via a skeleton struct, which describes
 * a hierarchical tree assembled from skeleton bones. A bone is linked to a mesh.
 * The bone knows its parent bone. If there is no parent bone the parent id is
 * marked with -1.
 * The skeleton-bone stores a pointer to its used armature. If there is no
 * armature this value if set to nullptr.
 * A skeleton bone stores its offset-matrix, which is the absolute transformation
 * for the bone. The bone stores the locale transformation to its parent as well.
 * You can compute the offset matrix by multiplying the hierarchy like:
 * Tree: s1 -> s2 -> s3
 * Offset-Matrix s3 = locale-s3 * locale-s2 * locale-s1
 */
struct aiSkeletonBone {
    /// The parent bone index, is -1 one if this bone represents the root bone.
    int mParent;


#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
    /// @brief The bone armature node - used for skeleton conversion
    /// you must enable aiProcess_PopulateArmatureData to populate this
    C_STRUCT aiNode *mArmature;

    /// @brief The bone node in the scene - used for skeleton conversion
    /// you must enable aiProcess_PopulateArmatureData to populate this
    C_STRUCT aiNode *mNode;

#endif
    /// @brief The number of weights
    unsigned int mNumnWeights;

    /// The mesh index, which will get influenced by the weight.
    C_STRUCT aiMesh *mMeshId;

    /// The influence weights of this bone, by vertex index.
    C_STRUCT aiVertexWeight *mWeights;

    /** Matrix that transforms from bone space to mesh space in bind pose.
     *
     * This matrix describes the position of the mesh
     * in the local space of this bone when the skeleton was bound.
     * Thus it can be used directly to determine a desired vertex position,
     * given the world-space transform of the bone when animated,
     * and the position of the vertex in mesh space.
     *
     * It is sometimes called an inverse-bind matrix,
     * or inverse bind pose matrix.
     */
    C_STRUCT aiMatrix4x4 mOffsetMatrix;

    /// Matrix that transforms the locale bone in bind pose.
    C_STRUCT aiMatrix4x4 mLocalMatrix;

#ifdef __cplusplus
    ///	@brief The class constructor.
    aiSkeletonBone() :
            mParent(-1),
#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
            mArmature(nullptr),
            mNode(nullptr),
#endif
            mNumnWeights(0),
            mMeshId(nullptr),
            mWeights(nullptr),
            mOffsetMatrix(),
            mLocalMatrix() {
        // empty
    }

    /// @brief The class constructor with its parent
    /// @param  parent      The parent node index.
    aiSkeletonBone(unsigned int parent) :
            mParent(parent),
#ifndef ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS
            mArmature(nullptr),
            mNode(nullptr),
#endif
            mNumnWeights(0),
            mMeshId(nullptr),
            mWeights(nullptr),
            mOffsetMatrix(),
            mLocalMatrix() {
        // empty
    }
    /// @brief The class destructor.
    ~aiSkeletonBone() {
        delete[] mWeights;
        mWeights = nullptr;
    }
#endif // __cplusplus
};
/**
 * @brief A skeleton represents the bone hierarchy of an animation.
 *
 * Skeleton animations can be described as a tree of bones:
 *                  root
 *                    |
 *                  node1
 *                  /   \
 *               node3  node4
 * If you want to calculate the transformation of node three you need to compute the
 * transformation hierarchy for the transformation chain of node3:
 * root->node1->node3
 * Each node is represented as a skeleton instance.
 */
struct aiSkeleton {
    /**
     *  @brief The name of the skeleton instance.
     */
    C_STRUCT aiString mName;

    /**
     *  @brief  The number of bones in the skeleton.
     */
    unsigned int mNumBones;

    /**
     *  @brief The bone instance in the skeleton.
     */
    C_STRUCT aiSkeletonBone **mBones;

#ifdef __cplusplus
    /**
     *  @brief The class constructor.
     */
    aiSkeleton() AI_NO_EXCEPT : mName(), mNumBones(0), mBones(nullptr) {
        // empty
    }

    /**
     *  @brief  The class destructor.
     */
    ~aiSkeleton() {
        delete[] mBones;
    }
#endif // __cplusplus
};
#ifdef __cplusplus
}
#endif //! extern "C"

#endif // AI_MESH_H_INC

