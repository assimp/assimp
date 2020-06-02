/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

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

/** @file  3DSLoader.cpp
 *  @brief Implementation of the 3ds importer class
 *
 *  http://www.the-labs.com/Blender/3DS-details.html
 */

#ifndef ASSIMP_BUILD_NO_3DS_IMPORTER

#include "3DSLoader.h"
#include <assimp/StringComparison.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/IOSystem.hpp>

using namespace Assimp;

static const aiImporterDesc desc = {
    "Discreet 3DS Importer",
    "",
    "",
    "Limited animation support",
    aiImporterFlags_SupportBinaryFlavour,
    0,
    0,
    0,
    0,
    "3ds prj"
};

// ------------------------------------------------------------------------------------------------
// Begins a new parsing block
// - Reads the current chunk and validates it
// - computes its length
#define ASSIMP_3DS_BEGIN_CHUNK(p_stream)                                        \
    while (true) {                                                              \
        if (p_stream->GetRemainingSizeToLimit() < sizeof(Discreet3DS::Chunk)) { \
            return;                                                             \
        }                                                                       \
        Discreet3DS::Chunk chunk;                                               \
        ReadChunk(chunk, p_stream);                                             \
        int chunkSize = chunk.Size - sizeof(Discreet3DS::Chunk);                \
        if (chunkSize <= 0)                                                     \
            continue;                                                           \
        const unsigned int oldReadLimit = p_stream->SetReadLimit(               \
                p_stream->GetCurrentPos() + chunkSize);

// ------------------------------------------------------------------------------------------------
// End a parsing block
// Must follow at the end of each parsing block, reset chunk end marker to previous value
#define ASSIMP_3DS_END_CHUNK(p_stream)            \
    p_stream->SkipToReadLimit();                  \
    p_stream->SetReadLimit(oldReadLimit);         \
    if (p_stream->GetRemainingSizeToLimit() == 0) \
        return;                                   \
    }

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
Discreet3DSImporter::Discreet3DSImporter() :
        mLastNodeIndex(), mScene(), mMasterScale(), bHasBG(), bIsPrj() {
    // empty
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
Discreet3DSImporter::~Discreet3DSImporter() {
    // empty
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file.
bool Discreet3DSImporter::CanRead(const std::string &p_File, IOSystem *const p_IOHandler, const bool p_checkSig) const {
    std::string extension = GetExtension(p_File);
    if (extension == "3ds" || extension == "prj") {
        return true;
    }

    if (!extension.length() || p_checkSig) {
        uint16_t token[3];
        token[0] = 0x4d4d;
        token[1] = 0x3dc2;
        //token[2] = 0x3daa;
        return CheckMagicToken(p_IOHandler, p_File, token, 2, 0, 2);
    }
    return false;
}

// ------------------------------------------------------------------------------------------------
// Loader registry entry
const aiImporterDesc *Discreet3DSImporter::GetInfo() const {
    return &desc;
}

// ------------------------------------------------------------------------------------------------
// Setup configuration properties
void Discreet3DSImporter::SetupProperties(const Importer *const /*pImp*/) {
    // nothing to be done for the moment
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure.
void Discreet3DSImporter::InternReadFile(const std::string &p_File,
        aiScene *p_Scene, IOSystem *p_IOHandler) {
    StreamReaderLE theStream(p_IOHandler->Open(p_File, "rb"));

    // We should have at least one chunk
    if (theStream.GetRemainingSize() < 16) {
        throw DeadlyImportError("3DS file is either empty or corrupt: " + p_File);
    }
    // this->p_stream = &theStream;

    // Allocate our temporary 3DS representation
    D3DS::Scene _scene;
    mScene = &_scene;

    // Initialize members
    D3DS::Node rootNode("UNNAMED");
    mLastNodeIndex = -1;
    D3DS::Node *p_currentNode = &rootNode;
    D3DS::Node *p_rootNode = &rootNode;
    p_rootNode->mHierarchyPos = -1;
    p_rootNode->mHierarchyIndex = -1;
    p_rootNode->mParent = nullptr;
    mMasterScale = 1.0f;
    mBackgroundImage = "";
    bHasBG = false;
    bIsPrj = false;

    // Parse the file
    ParseMainChunk(p_rootNode, p_currentNode, &theStream);

    // Process all meshes in the file. First check whether all
    // face indices have valid values. The generate our
    // internal verbose representation. Finally compute normal
    // vectors from the smoothing groups we read from the
    // file.
    for (auto &mesh : mScene->mMeshes) {
        if (mesh.mFaces.size() > 0 && mesh.mPositions.size() == 0) {
            throw DeadlyImportError("3DS file contains faces but no vertices: " + p_File);
        }
        CheckIndices(mesh);
        MakeUnique(mesh);
        ComputeNormalsWithSmoothingsGroups<D3DS::Face>(mesh);
    }

    // Replace all occurrences of the default material with a
    // valid material. Generate it if no material containing
    // DEFAULT in its name has been found in the file
    ReplaceDefaultMaterial();

    // Convert the scene from our internal representation to an
    // aiScene object. This involves copying all meshes, lights
    // and cameras to the scene
    ConvertScene(p_Scene);

    // Generate the node graph for the scene. This is a little bit
    // tricky since we'll need to split some meshes into sub-meshes
    GenerateNodeGraph(p_Scene, p_rootNode);

    // Now apply the master scaling factor to the scene
    ApplyMasterScale(p_Scene);

    // Our internal scene representation and the root
    // node will be automatically deleted, so the whole hierarchy will follow

    AI_DEBUG_INVALIDATE_PTR(mScene);
}

// ------------------------------------------------------------------------------------------------
// Applies a master-scaling factor to the imported scene
void Discreet3DSImporter::ApplyMasterScale(aiScene *const p_Scene) {
    // There are some 3DS files with a zero scaling factor
    if (!mMasterScale)
        mMasterScale = 1.0f;
    else
        mMasterScale = 1.0f / mMasterScale;

    // Construct an uniform scaling matrix and multiply with it
    p_Scene->mRootNode->mTransformation *= aiMatrix4x4(
            mMasterScale, 0.0f, 0.0f, 0.0f,
            0.0f, mMasterScale, 0.0f, 0.0f,
            0.0f, 0.0f, mMasterScale, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

    // Check whether a scaling track is assigned to the root node.
}

// ------------------------------------------------------------------------------------------------
// Reads a new chunk from the file
void Discreet3DSImporter::ReadChunk(Discreet3DS::Chunk &p_cOut,
        StreamReaderLE *const p_stream) const {

    p_cOut.Flag = static_cast<Discreet3DS::ChunkEnum>(p_stream->GetI2());
    p_cOut.Size = p_stream->GetI4();

    if (p_cOut.Size - sizeof(Discreet3DS::Chunk) > p_stream->GetRemainingSize()) {
        throw DeadlyImportError("Chunk is too large");
    }

    if (p_cOut.Size - sizeof(Discreet3DS::Chunk) > p_stream->GetRemainingSizeToLimit()) {
        ASSIMP_LOG_ERROR("3DS: Chunk overflow");
    }
}

// ------------------------------------------------------------------------------------------------
// Process the primary chunk of the file
void Discreet3DSImporter::ParseMainChunk(D3DS::Node *const p_rootNode,
        D3DS::Node *p_currentNode,
        StreamReaderLE *p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);

    // get chunk type
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_PRJ:
        bIsPrj = true;
    case Discreet3DS::ChunkEnum::CHUNK_MAIN:
        ParseEditorChunk(p_rootNode, p_currentNode, p_stream);
        break;
    default: break;
    };

    ASSIMP_3DS_END_CHUNK(p_stream);
    // recursively continue processing this hierarchy level
    return ParseMainChunk(p_rootNode, p_currentNode, p_stream);
}

// ------------------------------------------------------------------------------------------------
void Discreet3DSImporter::ParseEditorChunk(D3DS::Node *const p_rootNode, D3DS::Node *const p_currentNode,
        StreamReaderLE *const p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);

    // get chunk type
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_OBJMESH:
        ParseObjectChunk(p_stream);
        break;

    // NOTE: In several documentations in the internet this
    // chunk appears at different locations
    case Discreet3DS::ChunkEnum::CHUNK_KEYFRAMER:
        ParseKeyframeChunk(p_rootNode, p_currentNode, p_stream);
        break;

    case Discreet3DS::ChunkEnum::CHUNK_VERSION: {
        // print the version number
        char buff[10];
        ASSIMP_itoa10(buff, p_stream->GetI2());
        ASSIMP_LOG_INFO_F(std::string("3DS file format version: "), buff);
    } break;
    default: break;
    };
    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
void Discreet3DSImporter::ParseObjectChunk(StreamReaderLE *p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);

    // get chunk type
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_OBJBLOCK: {
        unsigned int cnt = 0;
        const char *sz = (const char *)p_stream->GetPtr();

        // Get the name of the geometry object
        while (p_stream->GetI1())
            ++cnt;
        ParseChunk(sz, cnt, p_stream);
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_MATERIAL:

        // Add a new material to the list
        mScene->mMaterials.push_back(D3DS::Material(std::string("UNNAMED_" + to_string(mScene->mMaterials.size()))));
        ParseMaterialChunk(p_stream);
        break;

    case Discreet3DS::ChunkEnum::CHUNK_AMBCOLOR:

        // This is the ambient base color of the scene.
        // We add it to the ambient color of all materials
        ParseColorChunk(&mClrAmbient, p_stream, true);
        if (is_qnan(mClrAmbient.r)) {
            // We failed to read the ambient base color.
            ASSIMP_LOG_ERROR("3DS: Failed to read ambient base color");
            mClrAmbient.r = mClrAmbient.g = mClrAmbient.b = 0.0f;
        }
        break;

    case Discreet3DS::ChunkEnum::CHUNK_BIT_MAP: {
        // Specifies the background image. The string should already be
        // properly 0 terminated but we need to be sure
        unsigned int cnt = 0;
        const char *sz = (const char *)p_stream->GetPtr();
        while (p_stream->GetI1())
            ++cnt;
        mBackgroundImage = std::string(sz, cnt);
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_BIT_MAP_EXISTS:
        bHasBG = true;
        break;

    case Discreet3DS::ChunkEnum::CHUNK_MASTER_SCALE:
        // Scene master scaling factor
        mMasterScale = p_stream->GetF4();
        break;
    default: break;
    };

    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
void Discreet3DSImporter::ParseChunk(const char *name, unsigned int num,
        StreamReaderLE *p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);

    // IMPLEMENTATION NOTE;
    // Cameras or lights define their transformation in their parent node and in the
    // corresponding light or camera chunks. However, we read and process the latter
    // to to be able to return valid cameras/lights even if no scenegraph is given.

    // get chunk type
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_TRIMESH: {
        // this starts a new triangle mesh
        mScene->mMeshes.push_back(D3DS::Mesh(std::string(name, num)));

        // Read mesh chunks
        ParseMeshChunk(p_stream);
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_LIGHT: {
        // This starts a new light
        aiLight *light = new aiLight();
        mScene->mLights.push_back(light);

        light->mName.Set(std::string(name, num));

        // First read the position of the light
        light->mPosition.x = p_stream->GetF4();
        light->mPosition.y = p_stream->GetF4();
        light->mPosition.z = p_stream->GetF4();

        light->mColorDiffuse = aiColor3D(1.f, 1.f, 1.f);

        // Now check for further subchunks
        if (!bIsPrj) /* fixme */
            ParseLightChunk(p_stream);

        // The specular light color is identical the the diffuse light color. The ambient light color
        // is equal to the ambient base color of the whole scene.
        light->mColorSpecular = light->mColorDiffuse;
        light->mColorAmbient = mClrAmbient;

        if (light->mType == aiLightSource_UNDEFINED) {
            // It must be a point light
            light->mType = aiLightSource_POINT;
        }
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_CAMERA: {
        // This starts a new camera
        aiCamera *camera = new aiCamera();
        mScene->mCameras.push_back(camera);
        camera->mName.Set(std::string(name, num));

        // First read the position of the camera
        camera->mPosition.x = p_stream->GetF4();
        camera->mPosition.y = p_stream->GetF4();
        camera->mPosition.z = p_stream->GetF4();

        // Then the camera target
        camera->mLookAt.x = p_stream->GetF4() - camera->mPosition.x;
        camera->mLookAt.y = p_stream->GetF4() - camera->mPosition.y;
        camera->mLookAt.z = p_stream->GetF4() - camera->mPosition.z;
        ai_real len = camera->mLookAt.Length();
        if (len < 1e-5) {

            // There are some files with lookat == position. Don't know why or whether it's ok or not.
            ASSIMP_LOG_ERROR("3DS: Unable to read proper camera look-at vector");
            camera->mLookAt = aiVector3D(0.0, 1.0, 0.0);

        } else
            camera->mLookAt /= len;

        // And finally - the camera rotation angle, in counter clockwise direction
        const ai_real angle = AI_DEG_TO_RAD(p_stream->GetF4());
        aiQuaternion quat(camera->mLookAt, angle);
        camera->mUp = quat.GetMatrix() * aiVector3D(0.0, 1.0, 0.0);

        // Read the lense angle
        camera->mHorizontalFOV = AI_DEG_TO_RAD(p_stream->GetF4());
        if (camera->mHorizontalFOV < 0.001f) {
            camera->mHorizontalFOV = AI_DEG_TO_RAD(45.f);
        }

        // Now check for further subchunks
        if (!bIsPrj) /* fixme */ {
            ParseCameraChunk(p_stream);
        }
    } break;
    default: break;
    };
    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
void Discreet3DSImporter::ParseLightChunk(StreamReaderLE *p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);
    aiLight *light = mScene->mLights.back();

    // get chunk type
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_DL_SPOTLIGHT:
        // Now we can be sure that the light is a spot light
        light->mType = aiLightSource_SPOT;

        // We wouldn't need to normalize here, but we do it
        light->mDirection.x = p_stream->GetF4() - light->mPosition.x;
        light->mDirection.y = p_stream->GetF4() - light->mPosition.y;
        light->mDirection.z = p_stream->GetF4() - light->mPosition.z;
        light->mDirection.Normalize();

        // Now the hotspot and falloff angles - in degrees
        light->mAngleInnerCone = AI_DEG_TO_RAD(p_stream->GetF4());

        // FIX: the falloff angle is just an offset
        light->mAngleOuterCone = light->mAngleInnerCone + AI_DEG_TO_RAD(p_stream->GetF4());
        break;

        // intensity multiplier
    case Discreet3DS::ChunkEnum::CHUNK_DL_MULTIPLIER:
        light->mColorDiffuse = light->mColorDiffuse * p_stream->GetF4();
        break;

        // light color
    case Discreet3DS::ChunkEnum::CHUNK_RGBF:
    case Discreet3DS::ChunkEnum::CHUNK_LINRGBF:
        light->mColorDiffuse.r *= p_stream->GetF4();
        light->mColorDiffuse.g *= p_stream->GetF4();
        light->mColorDiffuse.b *= p_stream->GetF4();
        break;

        // light attenuation
    case Discreet3DS::ChunkEnum::CHUNK_DL_ATTENUATE:
        light->mAttenuationLinear = p_stream->GetF4();
        break;
    default: break;
    };

    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
void Discreet3DSImporter::ParseCameraChunk(StreamReaderLE *p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);
    aiCamera *camera = mScene->mCameras.back();

    // get chunk type
    // if chunk flag related to camera range, obtain near and far clip planes.
    if (chunk.Flag == Discreet3DS::ChunkEnum::CHUNK_CAM_RANGES) {
        camera->mClipPlaneNear = p_stream->GetF4();
        camera->mClipPlaneFar = p_stream->GetF4();
    }

    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
void Discreet3DSImporter::ParseKeyframeChunk(D3DS::Node *const p_rootNode, D3DS::Node *const p_currentNode,
        StreamReaderLE *const p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);

    // get chunk type
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_TRACKCAMTGT:
    case Discreet3DS::ChunkEnum::CHUNK_TRACKSPOTL:
    case Discreet3DS::ChunkEnum::CHUNK_TRACKCAMERA:
    case Discreet3DS::ChunkEnum::CHUNK_TRACKINFO:
    case Discreet3DS::ChunkEnum::CHUNK_TRACKLIGHT:
    case Discreet3DS::ChunkEnum::CHUNK_TRACKLIGTGT:
        // this starts a new mesh hierarchy chunk
        ParseHierarchyChunk(chunk.Flag, p_rootNode, p_currentNode, p_stream);
        break;
    default: break;
    };

    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
// Little helper function for ParseHierarchyChunk
void Discreet3DSImporter::InverseNodeSearch(D3DS::Node *const pcNode, D3DS::Node *const p_rootNode, D3DS::Node *const pcCurrent) {
    if (!pcCurrent) {
        p_rootNode->push_back(pcNode);
        return;
    }

    if (pcCurrent->mHierarchyPos == pcNode->mHierarchyPos) {
        if (pcCurrent->mParent) {
            pcCurrent->mParent->push_back(pcNode);
        } else
            pcCurrent->push_back(pcNode);
        return;
    }
    return InverseNodeSearch(pcNode, p_rootNode, pcCurrent->mParent);
}

// ------------------------------------------------------------------------------------------------
// Find a node with a specific name in the import hierarchy
D3DS::Node *FindNode(D3DS::Node *const root, const std::string &name) {
    if (root->mName == name) {
        return root;
    }

    for (Assimp::D3DS::Node *const it : root->mChildren) {
        D3DS::Node *nd = FindNode(it, name);
        if (nullptr != nd) {
            return nd;
        }
    }

    return nullptr;
}

// ------------------------------------------------------------------------------------------------
// Binary predicate for std::unique()
template <class T>
bool KeyUniqueCompare(const T &first, const T &second) {
    return first.mTime == second.mTime;
}

// ------------------------------------------------------------------------------------------------
// Skip some additional import data.
void Discreet3DSImporter::SkipTCBInfo(StreamReaderLE *p_stream) {
    Discreet3DS::AnimatedKey flags = static_cast<Discreet3DS::AnimatedKey>(p_stream->GetI2());

    if (static_cast<uint16_t>(flags) == 0) {
        // Currently we can't do anything with these values. They occur
        // quite rare, so it wouldn't be worth the effort implementing
        // them. 3DS is not really suitable for complex animations,
        // so full support is not required.
        ASSIMP_LOG_WARN("3DS: Skipping TCB animation info");
    }

    if (flags & Discreet3DS::AnimatedKey::KEY_USE_TENS) {
        p_stream->IncPtr(4);
    }
    if (flags & Discreet3DS::AnimatedKey::KEY_USE_BIAS) {
        p_stream->IncPtr(4);
    }
    if (flags & Discreet3DS::AnimatedKey::KEY_USE_CONT) {
        p_stream->IncPtr(4);
    }
    if (flags & Discreet3DS::AnimatedKey::KEY_USE_EASE_FROM) {
        p_stream->IncPtr(4);
    }
    if (flags & Discreet3DS::AnimatedKey::KEY_USE_EASE_TO) {
        p_stream->IncPtr(4);
    }
}

// ------------------------------------------------------------------------------------------------
// Read hierarchy and keyframe info
void Discreet3DSImporter::ParseHierarchyChunk(const Discreet3DS::ChunkEnum parent,
        D3DS::Node *const p_rootNode,
        D3DS::Node *p_currentNode,
        StreamReaderLE *const p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);

    // get chunk type
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_TRACKOBJNAME:

        // This is the name of the object to which the track applies. The chunk also
        // defines the position of this object in the hierarchy.
        {

            // First of all: get the name of the object
            unsigned int cnt = 0;
            const char *sz = (const char *)p_stream->GetPtr();

            while (p_stream->GetI1())
                ++cnt;
            std::string name = std::string(sz, cnt);

            // Now find out whether we have this node already (target animation channels
            // are stored with a separate object ID)
            D3DS::Node *pcNode = FindNode(p_rootNode, name);
            int instanceNumber = 1;

            if (pcNode) {
                // if the source is not a CHUNK_TRACKINFO block it won't be an object instance
                if (parent != Discreet3DS::ChunkEnum::CHUNK_TRACKINFO) {
                    p_currentNode = pcNode;
                    break;
                }
                pcNode->mInstanceCount++;
                instanceNumber = pcNode->mInstanceCount;
            }
            pcNode = new D3DS::Node(name);
            pcNode->mInstanceNumber = instanceNumber;

            // There are two unknown values which we can safely ignore
            p_stream->IncPtr(4);

            // Now read the hierarchy position of the object
            uint16_t hierarchy = p_stream->GetI2() + 1;
            pcNode->mHierarchyPos = hierarchy;
            pcNode->mHierarchyIndex = mLastNodeIndex;

            // And find a proper position in the graph for it
            if (p_currentNode && p_currentNode->mHierarchyPos == hierarchy) {

                // add to the parent of the last touched node
                p_currentNode->mParent->push_back(pcNode);
                mLastNodeIndex++;
            } else if (hierarchy >= mLastNodeIndex) {

                // place it at the current position in the hierarchy
                p_currentNode->push_back(pcNode);
                mLastNodeIndex = hierarchy;
            } else {
                // need to go back to the specified position in the hierarchy.
                InverseNodeSearch(pcNode, p_rootNode, p_currentNode);
                mLastNodeIndex++;
            }
            // Make this node the current node
            p_currentNode = pcNode;
        }
        break;

    case Discreet3DS::ChunkEnum::CHUNK_TRACKDUMMYOBJNAME:

        // This is the "real" name of a $$$DUMMY object
        {
            const char *sz = (const char *)p_stream->GetPtr();
            while (p_stream->GetI1())
                ;

            // If object name is DUMMY, take this one instead
            if (p_currentNode->mName == "$$$DUMMY") {
                p_currentNode->mName = std::string(sz);
                break;
            }
        }
        break;

    case Discreet3DS::ChunkEnum::CHUNK_TRACKPIVOT:

        if (Discreet3DS::ChunkEnum::CHUNK_TRACKINFO != parent) {
            ASSIMP_LOG_WARN("3DS: Skipping pivot subchunk for non usual object");
            break;
        }

        // Pivot = origin of rotation and scaling
        p_currentNode->vPivot.x = p_stream->GetF4();
        p_currentNode->vPivot.y = p_stream->GetF4();
        p_currentNode->vPivot.z = p_stream->GetF4();
        break;

        // ////////////////////////////////////////////////////////////////////
        // POSITION KEYFRAME
    case Discreet3DS::ChunkEnum::CHUNK_TRACKPOS: {
        p_stream->IncPtr(10);
        const unsigned int numFrames = p_stream->GetI4();
        bool sortKeys = false;

        // This could also be meant as the target position for
        // (targeted) lights and cameras
        std::vector<aiVectorKey> *l;
        if (Discreet3DS::ChunkEnum::CHUNK_TRACKCAMTGT == parent || Discreet3DS::ChunkEnum::CHUNK_TRACKLIGTGT == parent) {
            l = &p_currentNode->aTargetPositionKeys;
        } else
            l = &p_currentNode->aPositionKeys;

        l->reserve(numFrames);
        for (unsigned int i = 0; i < numFrames; ++i) {
            const unsigned int fidx = p_stream->GetI4();

            // Setup a new position key
            aiVectorKey v;
            v.mTime = (double)fidx;

            SkipTCBInfo(p_stream);
            v.mValue.x = p_stream->GetF4();
            v.mValue.y = p_stream->GetF4();
            v.mValue.z = p_stream->GetF4();

            // check whether we'll need to sort the keys
            if (!l->empty() && v.mTime <= l->back().mTime)
                sortKeys = true;

            // Add the new keyframe to the list
            l->push_back(v);
        }

        // Sort all keys with ascending time values and remove duplicates?
        if (sortKeys) {
            std::stable_sort(l->begin(), l->end());
            l->erase(std::unique(l->begin(), l->end(), &KeyUniqueCompare<aiVectorKey>), l->end());
        }
    }

    break;

        // ////////////////////////////////////////////////////////////////////
        // CAMERA ROLL KEYFRAME
    case Discreet3DS::ChunkEnum::CHUNK_TRACKROLL: {
        // roll keys are accepted for cameras only
        if (parent != Discreet3DS::ChunkEnum::CHUNK_TRACKCAMERA) {
            ASSIMP_LOG_WARN("3DS: Ignoring roll track for non-camera object");
            break;
        }
        bool sortKeys = false;
        std::vector<aiFloatKey> *l = &p_currentNode->aCameraRollKeys;

        p_stream->IncPtr(10);
        const unsigned int numFrames = p_stream->GetI4();
        l->reserve(numFrames);
        for (unsigned int i = 0; i < numFrames; ++i) {
            const unsigned int fidx = p_stream->GetI4();

            // Setup a new position key
            aiFloatKey v;
            v.mTime = (double)fidx;

            // This is just a single float
            SkipTCBInfo(p_stream);
            v.mValue = p_stream->GetF4();

            // Check whether we'll need to sort the keys
            if (!l->empty() && v.mTime <= l->back().mTime)
                sortKeys = true;

            // Add the new keyframe to the list
            l->push_back(v);
        }

        // Sort all keys with ascending time values and remove duplicates?
        if (sortKeys) {
            std::stable_sort(l->begin(), l->end());
            l->erase(std::unique(l->begin(), l->end(), &KeyUniqueCompare<aiFloatKey>), l->end());
        }
    } break;

        // ////////////////////////////////////////////////////////////////////
        // CAMERA FOV KEYFRAME
    case Discreet3DS::ChunkEnum::CHUNK_TRACKFOV: {
        ASSIMP_LOG_ERROR("3DS: Skipping FOV animation track. "
                         "This is not supported");
    } break;

        // ////////////////////////////////////////////////////////////////////
        // ROTATION KEYFRAME
    case Discreet3DS::ChunkEnum::CHUNK_TRACKROTATE: {
        p_stream->IncPtr(10);
        const unsigned int numFrames = p_stream->GetI4();

        bool sortKeys = false;
        std::vector<aiQuatKey> *l = &p_currentNode->aRotationKeys;
        l->reserve(numFrames);

        for (unsigned int i = 0; i < numFrames; ++i) {
            const unsigned int fidx = p_stream->GetI4();
            SkipTCBInfo(p_stream);

            aiQuatKey v;
            v.mTime = (double)fidx;

            // The rotation keyframe is given as an axis-angle pair
            const float rad = p_stream->GetF4();
            aiVector3D axis;
            axis.x = p_stream->GetF4();
            axis.y = p_stream->GetF4();
            axis.z = p_stream->GetF4();

            if (!axis.x && !axis.y && !axis.z)
                axis.y = 1.f;

            // Construct a rotation quaternion from the axis-angle pair
            v.mValue = aiQuaternion(axis, rad);

            // Check whether we'll need to sort the keys
            if (!l->empty() && v.mTime <= l->back().mTime)
                sortKeys = true;

            // add the new keyframe to the list
            l->push_back(v);
        }
        // Sort all keys with ascending time values and remove duplicates?
        if (sortKeys) {
            std::stable_sort(l->begin(), l->end());
            l->erase(std::unique(l->begin(), l->end(), &KeyUniqueCompare<aiQuatKey>), l->end());
        }
    } break;

        // ////////////////////////////////////////////////////////////////////
        // SCALING KEYFRAME
    case Discreet3DS::ChunkEnum::CHUNK_TRACKSCALE: {
        p_stream->IncPtr(10);
        const unsigned int numFrames = p_stream->GetI2();
        p_stream->IncPtr(2);

        bool sortKeys = false;
        std::vector<aiVectorKey> *l = &p_currentNode->aScalingKeys;
        l->reserve(numFrames);

        for (unsigned int i = 0; i < numFrames; ++i) {
            const unsigned int fidx = p_stream->GetI4();
            SkipTCBInfo(p_stream);

            // Setup a new key
            aiVectorKey v;
            v.mTime = (double)fidx;

            // ... and read its value
            v.mValue.x = p_stream->GetF4();
            v.mValue.y = p_stream->GetF4();
            v.mValue.z = p_stream->GetF4();

            // check whether we'll need to sort the keys
            if (!l->empty() && v.mTime <= l->back().mTime)
                sortKeys = true;

            // Remove zero-scalings on singular axes - they've been reported to be there erroneously in some strange files
            if (!v.mValue.x) v.mValue.x = 1.f;
            if (!v.mValue.y) v.mValue.y = 1.f;
            if (!v.mValue.z) v.mValue.z = 1.f;

            l->push_back(v);
        }
        // Sort all keys with ascending time values and remove duplicates?
        if (sortKeys) {
            std::stable_sort(l->begin(), l->end());
            l->erase(std::unique(l->begin(), l->end(), &KeyUniqueCompare<aiVectorKey>), l->end());
        }
    } break;
    default: break;
    };

    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
// Read a face chunk - it contains smoothing groups and material assignments
void Discreet3DSImporter::ParseFaceChunk(StreamReaderLE *p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);

    // Get the mesh we're currently working on
    D3DS::Mesh &mMesh = mScene->mMeshes.back();

    // Get chunk type
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_SMOOLIST: {
        // This is the list of smoothing groups - a bitfield for every face.
        // Up to 32 smoothing groups assigned to a single face.
        unsigned int num = chunkSize / 4, m = 0;
        if (num > mMesh.mFaces.size()) {
            throw DeadlyImportError("3DS: More smoothing groups than faces");
        }
        for (std::vector<D3DS::Face>::iterator i = mMesh.mFaces.begin(); m != num; ++i, ++m) {
            // nth bit is set for nth smoothing group
            (*i).iSmoothGroup = p_stream->GetI4();
        }
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_FACEMAT: {
        // at fist an asciiz with the material name
        const char *sz = (const char *)p_stream->GetPtr();
        while (p_stream->GetI1())
            ;

        // find the index of the material
        unsigned int idx = 0xcdcdcdcd, cnt = 0;
        for (std::vector<D3DS::Material>::const_iterator i = mScene->mMaterials.begin(); i != mScene->mMaterials.end(); ++i, ++cnt) {
            // use case independent comparisons. hopefully it will work.
            if ((*i).mName.length() && !ASSIMP_stricmp(sz, (*i).mName.c_str())) {
                idx = cnt;
                break;
            }
        }
        if (0xcdcdcdcd == idx) {
            ASSIMP_LOG_ERROR_F("3DS: Unknown material: ", sz);
        }

        // Now continue and read all material indices
        cnt = (uint16_t)p_stream->GetI2();
        for (unsigned int i = 0; i < cnt; ++i) {
            unsigned int fidx = (uint16_t)p_stream->GetI2();

            // check range
            if (fidx >= mMesh.mFaceMaterials.size()) {
                ASSIMP_LOG_ERROR("3DS: Invalid face index in face material list");
            } else
                mMesh.mFaceMaterials[fidx] = idx;
        }
    } break;
    default: break;
    };
    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
// Read a mesh chunk. Here's the actual mesh data
void Discreet3DSImporter::ParseMeshChunk(StreamReaderLE *p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);

    // Get the mesh we're currently working on
    D3DS::Mesh &mMesh = mScene->mMeshes.back();

    // get chunk type
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_VERTLIST: {
        // This is the list of all vertices in the current mesh
        int num = (int)(uint16_t)p_stream->GetI2();
        mMesh.mPositions.reserve(num);
        while (num-- > 0) {
            aiVector3D v;
            v.x = p_stream->GetF4();
            v.y = p_stream->GetF4();
            v.z = p_stream->GetF4();
            mMesh.mPositions.push_back(v);
        }
    } break;
    case Discreet3DS::ChunkEnum::CHUNK_TRMATRIX: {
        // This is the RLEATIVE transformation matrix of the current mesh. Vertices are
        // pretransformed by this matrix wonder.
        mMesh.mMat.a1 = p_stream->GetF4();
        mMesh.mMat.b1 = p_stream->GetF4();
        mMesh.mMat.c1 = p_stream->GetF4();
        mMesh.mMat.a2 = p_stream->GetF4();
        mMesh.mMat.b2 = p_stream->GetF4();
        mMesh.mMat.c2 = p_stream->GetF4();
        mMesh.mMat.a3 = p_stream->GetF4();
        mMesh.mMat.b3 = p_stream->GetF4();
        mMesh.mMat.c3 = p_stream->GetF4();
        mMesh.mMat.a4 = p_stream->GetF4();
        mMesh.mMat.b4 = p_stream->GetF4();
        mMesh.mMat.c4 = p_stream->GetF4();
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_MAPLIST: {
        // This is the list of all UV coords in the current mesh
        int num = (int)(uint16_t)p_stream->GetI2();
        mMesh.mTexCoords.reserve(num);
        while (num-- > 0) {
            aiVector3D v;
            v.x = p_stream->GetF4();
            v.y = p_stream->GetF4();
            mMesh.mTexCoords.push_back(v);
        }
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_FACELIST: {
        // This is the list of all faces in the current mesh
        int num = (int)(uint16_t)p_stream->GetI2();
        mMesh.mFaces.reserve(num);
        while (num-- > 0) {
            // 3DS faces are ALWAYS triangles
            mMesh.mFaces.push_back(D3DS::Face());
            D3DS::Face &sFace = mMesh.mFaces.back();

            sFace.mIndices[0] = (uint16_t)p_stream->GetI2();
            sFace.mIndices[1] = (uint16_t)p_stream->GetI2();
            sFace.mIndices[2] = (uint16_t)p_stream->GetI2();

            p_stream->IncPtr(2); // skip edge visibility flag
        }

        // Resize the material array (0xcdcdcdcd marks the default material; so if a face is
        // not referenced by a material, $$DEFAULT will be assigned to it)
        mMesh.mFaceMaterials.resize(mMesh.mFaces.size(), 0xcdcdcdcd);

        // Larger 3DS files could have multiple FACE chunks here
        chunkSize = (int)p_stream->GetRemainingSizeToLimit();
        if (chunkSize > (int)sizeof(Discreet3DS::Chunk))
            ParseFaceChunk(p_stream);
    } break;
    default: break;
    };
    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
// Read a 3DS material chunk
void Discreet3DSImporter::ParseMaterialChunk(StreamReaderLE *p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_MAT_MATNAME:

    {
        // The material name string is already zero-terminated, but we need to be sure ...
        const char *sz = (const char *)p_stream->GetPtr();
        unsigned int cnt = 0;
        while (p_stream->GetI1())
            ++cnt;

        if (!cnt) {
            // This may not be, we use the default name instead
            ASSIMP_LOG_ERROR("3DS: Empty material name");
        } else
            mScene->mMaterials.back().mName = std::string(sz, cnt);
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_DIFFUSE: {
        // This is the diffuse material color
        aiColor3D *pc = &mScene->mMaterials.back().mDiffuse;
        ParseColorChunk(pc, p_stream);
        if (is_qnan(pc->r)) {
            // color chunk is invalid. Simply ignore it
            ASSIMP_LOG_ERROR("3DS: Unable to read DIFFUSE chunk");
            pc->r = pc->g = pc->b = 1.0f;
        }
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_SPECULAR: {
        // This is the specular material color
        aiColor3D *pc = &mScene->mMaterials.back().mSpecular;
        ParseColorChunk(pc, p_stream);
        if (is_qnan(pc->r)) {
            // color chunk is invalid. Simply ignore it
            ASSIMP_LOG_ERROR("3DS: Unable to read SPECULAR chunk");
            pc->r = pc->g = pc->b = 1.0f;
        }
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_AMBIENT: {
        // This is the ambient material color
        aiColor3D *pc = &mScene->mMaterials.back().mAmbient;
        ParseColorChunk(pc, p_stream);
        if (is_qnan(pc->r)) {
            // color chunk is invalid. Simply ignore it
            ASSIMP_LOG_ERROR("3DS: Unable to read AMBIENT chunk");
            pc->r = pc->g = pc->b = 0.0f;
        }
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_SELF_ILLUM: {
        // This is the emissive material color
        aiColor3D *pc = &mScene->mMaterials.back().mEmissive;
        ParseColorChunk(pc, p_stream);
        if (is_qnan(pc->r)) {
            // color chunk is invalid. Simply ignore it
            ASSIMP_LOG_ERROR("3DS: Unable to read EMISSIVE chunk");
            pc->r = pc->g = pc->b = 0.0f;
        }
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_TRANSPARENCY: {
        // This is the material's transparency
        ai_real *pcf = &mScene->mMaterials.back().mTransparency;
        *pcf = ParsePercentageChunk(p_stream);

        // NOTE: transparency, not opacity
        if (is_qnan(*pcf))
            *pcf = ai_real(1.0);
        else
            *pcf = ai_real(1.0) - *pcf * (ai_real)0xFFFF / ai_real(100.0);
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_SHADING:
        // This is the material shading mode
        mScene->mMaterials.back().mShading = static_cast<D3DS::Discreet3DS::ShadeType3DS>(p_stream->GetI2());
        break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_TWO_SIDE:
        // This is the two-sided flag
        mScene->mMaterials.back().mTwoSided = true;
        break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_SHININESS: { // This is the shininess of the material
        ai_real *pcf = &mScene->mMaterials.back().mSpecularExponent;
        *pcf = ParsePercentageChunk(p_stream);
        if (is_qnan(*pcf))
            *pcf = 0.0;
        else
            *pcf *= (ai_real)0xFFFF;
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_SHININESS_PERCENT: { // This is the shininess strength of the material
        ai_real *pcf = &mScene->mMaterials.back().mShininessStrength;
        *pcf = ParsePercentageChunk(p_stream);
        if (is_qnan(*pcf))
            *pcf = ai_real(0.0);
        else
            *pcf *= (ai_real)0xffff / ai_real(100.0);
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_SELF_ILPCT: { // This is the self illumination strength of the material
        ai_real f = ParsePercentageChunk(p_stream);
        if (is_qnan(f))
            f = ai_real(0.0);
        else
            f *= (ai_real)0xFFFF / ai_real(100.0);
        mScene->mMaterials.back().mEmissive = aiColor3D(f, f, f);
    } break;

    // Parse texture chunks
    case Discreet3DS::ChunkEnum::CHUNK_MAT_TEXTURE:
        // Diffuse texture
        ParseTextureChunk(&mScene->mMaterials.back().sTexDiffuse, p_stream);
        break;
    case Discreet3DS::ChunkEnum::CHUNK_MAT_BUMPMAP:
        // Height map
        ParseTextureChunk(&mScene->mMaterials.back().sTexBump, p_stream);
        break;
    case Discreet3DS::ChunkEnum::CHUNK_MAT_OPACMAP:
        // Opacity texture
        ParseTextureChunk(&mScene->mMaterials.back().sTexOpacity, p_stream);
        break;
    case Discreet3DS::ChunkEnum::CHUNK_MAT_MAT_SHINMAP:
        // Shininess map
        ParseTextureChunk(&mScene->mMaterials.back().sTexShininess, p_stream);
        break;
    case Discreet3DS::ChunkEnum::CHUNK_MAT_SPECMAP:
        // Specular map
        ParseTextureChunk(&mScene->mMaterials.back().sTexSpecular, p_stream);
        break;
    case Discreet3DS::ChunkEnum::CHUNK_MAT_SELFIMAP:
        // Self-illumination (emissive) map
        ParseTextureChunk(&mScene->mMaterials.back().sTexEmissive, p_stream);
        break;
    case Discreet3DS::ChunkEnum::CHUNK_MAT_REFLMAP:
        // Reflection map
        ParseTextureChunk(&mScene->mMaterials.back().sTexReflective, p_stream);
        break;
    default: break;
    };
    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
void Discreet3DSImporter::ParseTextureChunk(D3DS::Texture *p_cOut,
        StreamReaderLE *p_stream) {
    ASSIMP_3DS_BEGIN_CHUNK(p_stream);

    // get chunk type
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_MAPFILE: {
        // The material name string is already zero-terminated, but we need to be sure ...
        const char *sz = (const char *)p_stream->GetPtr();
        unsigned int cnt = 0;
        while (p_stream->GetI1())
            ++cnt;
        p_cOut->mMapName = std::string(sz, cnt);
    } break;

    case Discreet3DS::ChunkEnum::CHUNK_PERCENTD:
        // Manually parse the blend factor
        p_cOut->mTextureBlend = ai_real(p_stream->GetF8());
        break;

    case Discreet3DS::ChunkEnum::CHUNK_PERCENTF:
        // Manually parse the blend factor
        p_cOut->mTextureBlend = p_stream->GetF4();
        break;

    case Discreet3DS::ChunkEnum::CHUNK_PERCENTW:
        // Manually parse the blend factor
        p_cOut->mTextureBlend = (ai_real)((uint16_t)p_stream->GetI2()) / ai_real(100.0);
        break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_MAP_USCALE:
        // Texture coordinate scaling in the U direction
        p_cOut->mScaleU = p_stream->GetF4();
        if (0.0f == p_cOut->mScaleU) {
            ASSIMP_LOG_WARN("Texture coordinate scaling in the x direction is zero. Assuming 1.");
            p_cOut->mScaleU = 1.0f;
        }
        break;
    case Discreet3DS::ChunkEnum::CHUNK_MAT_MAP_VSCALE:
        // Texture coordinate scaling in the V direction
        p_cOut->mScaleV = p_stream->GetF4();
        if (0.0f == p_cOut->mScaleV) {
            ASSIMP_LOG_WARN("Texture coordinate scaling in the y direction is zero. Assuming 1.");
            p_cOut->mScaleV = 1.0f;
        }
        break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_MAP_UOFFSET:
        // Texture coordinate offset in the U direction
        p_cOut->mOffsetU = -p_stream->GetF4();
        break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_MAP_VOFFSET:
        // Texture coordinate offset in the V direction
        p_cOut->mOffsetV = p_stream->GetF4();
        break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_MAP_ANG:
        // Texture coordinate rotation, CCW in DEGREES
        p_cOut->mRotation = -AI_DEG_TO_RAD(p_stream->GetF4());
        break;

    case Discreet3DS::ChunkEnum::CHUNK_MAT_MAP_TILING: {
        const uint16_t iFlags = p_stream->GetI2();

        // Get the mapping mode (for both axes)
        if (iFlags & 0x2u)
            p_cOut->mMapMode = aiTextureMapMode_Mirror;

        else if (iFlags & 0x10u)
            p_cOut->mMapMode = aiTextureMapMode_Decal;

        // wrapping in all remaining cases
        else
            p_cOut->mMapMode = aiTextureMapMode_Wrap;
    } break;
    default: break;
    };

    ASSIMP_3DS_END_CHUNK(p_stream);
}

// ------------------------------------------------------------------------------------------------
// Read a percentage chunk
ai_real Discreet3DSImporter::ParsePercentageChunk(StreamReaderLE *p_stream) {
    Discreet3DS::Chunk chunk;
    ReadChunk(chunk, p_stream);

    if (Discreet3DS::ChunkEnum::CHUNK_PERCENTF == chunk.Flag) {
        return p_stream->GetF4() * ai_real(100) / ai_real(0xFFFF);
    } else if (Discreet3DS::ChunkEnum::CHUNK_PERCENTW == chunk.Flag) {
        return (ai_real)((uint16_t)p_stream->GetI2()) / (ai_real)0xFFFF;
    }

    return get_qnan();
}

// ------------------------------------------------------------------------------------------------
// Read a color chunk. If a percentage chunk is found instead it is read as a grayscale color
void Discreet3DSImporter::ParseColorChunk(aiColor3D *p_cOut, StreamReaderLE *p_stream, bool p_acceptPercent) {
    ai_assert(p_cOut != nullptr);

    // error return value
    const ai_real qnan = get_qnan();
    static const aiColor3D clrError = aiColor3D(qnan, qnan, qnan);

    Discreet3DS::Chunk chunk;
    ReadChunk(chunk, p_stream);
    const unsigned int diff = chunk.Size - sizeof(Discreet3DS::Chunk);

    bool bGamma = false;

    // Get the type of the chunk
    switch (chunk.Flag) {
    case Discreet3DS::ChunkEnum::CHUNK_LINRGBF:
        bGamma = true;

    case Discreet3DS::ChunkEnum::CHUNK_RGBF:
        if (sizeof(float) * 3 > diff) {
            *p_cOut = clrError;
            return;
        }
        p_cOut->r = p_stream->GetF4();
        p_cOut->g = p_stream->GetF4();
        p_cOut->b = p_stream->GetF4();
        break;

    case Discreet3DS::ChunkEnum::CHUNK_LINRGBB:
        bGamma = true;
    case Discreet3DS::ChunkEnum::CHUNK_RGBB: {
        if (sizeof(char) * 3 > diff) {
            *p_cOut = clrError;
            return;
        }
        const ai_real invVal = ai_real(1.0) / ai_real(255.0);
        p_cOut->r = (ai_real)(uint8_t)p_stream->GetI1() * invVal;
        p_cOut->g = (ai_real)(uint8_t)p_stream->GetI1() * invVal;
        p_cOut->b = (ai_real)(uint8_t)p_stream->GetI1() * invVal;
    } break;

    // Percentage chunks are accepted, too.
    case Discreet3DS::ChunkEnum::CHUNK_PERCENTF:
        if (p_acceptPercent && 4 <= diff) {
            p_cOut->g = p_cOut->b = p_cOut->r = p_stream->GetF4();
            break;
        }
        *p_cOut = clrError;
        return;

    case Discreet3DS::ChunkEnum::CHUNK_PERCENTW:
        if (p_acceptPercent && 1 <= diff) {
            p_cOut->g = p_cOut->b = p_cOut->r = (ai_real)(uint8_t)p_stream->GetI1() / ai_real(255.0);
            break;
        }
        *p_cOut = clrError;
        return;

    default:
        p_stream->IncPtr(diff);
        // Skip unknown chunks, hope this won't cause any problems.
        return ParseColorChunk(p_cOut, p_stream, p_acceptPercent);
    };
    (void)bGamma;
}

#endif // !! ASSIMP_BUILD_NO_3DS_IMPORTER
