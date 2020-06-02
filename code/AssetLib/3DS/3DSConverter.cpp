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

/** @file Implementation of the 3ds importer class */

#ifndef ASSIMP_BUILD_NO_3DS_IMPORTER

// internal headers
#include "3DSLoader.h"
#include "Common/TargetAnimation.h"
#include <assimp/StringComparison.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <cctype>
#include <memory>

using namespace Assimp;

static const unsigned int NotSet = 0xcdcdcdcd;

// ------------------------------------------------------------------------------------------------
// Setup final material indices, generae a default material if necessary
void Discreet3DSImporter::ReplaceDefaultMaterial() {
    // Try to find an existing material that matches the
    // typical default material setting:
    // - no textures
    // - diffuse color (in grey!)
    // NOTE: This is here to workaround the fact that some
    // exporters are writing a default material, too.
    unsigned int idx(NotSet);
    for (unsigned int i = 0; i < mScene->mMaterials.size(); ++i) {
        std::string s = mScene->mMaterials[i].mName;
        for (std::string::iterator it = s.begin(); it != s.end(); ++it) {
            *it = static_cast<char>(::tolower(*it));
        }

        if (std::string::npos == s.find("default")) continue;

        if (mScene->mMaterials[i].mDiffuse.r !=
                        mScene->mMaterials[i].mDiffuse.g ||
                mScene->mMaterials[i].mDiffuse.r !=
                        mScene->mMaterials[i].mDiffuse.b) continue;

        if (mScene->mMaterials[i].sTexDiffuse.mMapName.length() != 0 ||
                mScene->mMaterials[i].sTexBump.mMapName.length() != 0 ||
                mScene->mMaterials[i].sTexOpacity.mMapName.length() != 0 ||
                mScene->mMaterials[i].sTexEmissive.mMapName.length() != 0 ||
                mScene->mMaterials[i].sTexSpecular.mMapName.length() != 0 ||
                mScene->mMaterials[i].sTexShininess.mMapName.length() != 0) {
            continue;
        }
        idx = i;
    }
    if (NotSet == idx) {
        idx = (unsigned int)mScene->mMaterials.size();
    }

    // now iterate through all meshes and through all faces and
    // find all faces that are using the default material
    unsigned int cnt = 0;
    for (std::vector<D3DS::Mesh>::iterator
                    i = mScene->mMeshes.begin();
            i != mScene->mMeshes.end(); ++i) {
        for (std::vector<unsigned int>::iterator
                        a = (*i).mFaceMaterials.begin();
                a != (*i).mFaceMaterials.end(); ++a) {
            // NOTE: The additional check seems to be necessary,
            // some exporters seem to generate invalid data here
            if (0xcdcdcdcd == (*a)) {
                (*a) = idx;
                ++cnt;
            } else if ((*a) >= mScene->mMaterials.size()) {
                (*a) = idx;
                ASSIMP_LOG_WARN("Material index overflow in 3DS file. Using default material");
                ++cnt;
            }
        }
    }
    if (cnt && idx == mScene->mMaterials.size()) {
        // We need to create our own default material
        D3DS::Material sMat("%%%DEFAULT");
        sMat.mDiffuse = aiColor3D(0.3f, 0.3f, 0.3f);
        mScene->mMaterials.push_back(sMat);

        ASSIMP_LOG_INFO("3DS: Generating default material");
    }
}

// ------------------------------------------------------------------------------------------------
// Check whether all indices are valid. Otherwise we'd crash before the validation step is reached
void Discreet3DSImporter::CheckIndices(D3DS::Mesh &sMesh) {
    for (std::vector<D3DS::Face>::iterator i = sMesh.mFaces.begin(); i != sMesh.mFaces.end(); ++i) {
        // check whether all indices are in range
        for (unsigned int a = 0; a < 3; ++a) {
            if ((*i).mIndices[a] >= sMesh.mPositions.size()) {
                ASSIMP_LOG_WARN("3DS: Vertex index overflow)");
                (*i).mIndices[a] = (uint32_t)sMesh.mPositions.size() - 1;
            }
            if (!sMesh.mTexCoords.empty() && (*i).mIndices[a] >= sMesh.mTexCoords.size()) {
                ASSIMP_LOG_WARN("3DS: Texture coordinate index overflow)");
                (*i).mIndices[a] = (uint32_t)sMesh.mTexCoords.size() - 1;
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Generate out unique verbose format representation
void Discreet3DSImporter::MakeUnique(D3DS::Mesh &sMesh) {
    // TODO: really necessary? I don't think. Just a waste of memory and time
    // to do it now in a separate buffer.

    // Allocate output storage
    std::vector<aiVector3D> vNew(sMesh.mFaces.size() * 3);
    std::vector<aiVector3D> vNew2;
    if (sMesh.mTexCoords.size())
        vNew2.resize(sMesh.mFaces.size() * 3);

    for (unsigned int i = 0, base = 0; i < sMesh.mFaces.size(); ++i) {
        D3DS::Face &face = sMesh.mFaces[i];

        // Positions
        for (unsigned int a = 0; a < 3; ++a, ++base) {
            vNew[base] = sMesh.mPositions[face.mIndices[a]];
            if (sMesh.mTexCoords.size())
                vNew2[base] = sMesh.mTexCoords[face.mIndices[a]];

            face.mIndices[a] = base;
        }
    }
    sMesh.mPositions = vNew;
    sMesh.mTexCoords = vNew2;
}

// ------------------------------------------------------------------------------------------------
// Convert a 3DS texture to texture keys in an aiMaterial
void CopyTexture(aiMaterial &mat, D3DS::Texture &texture, aiTextureType type) {
    // Setup the texture name
    aiString tex;
    tex.Set(texture.mMapName);
    mat.AddProperty(&tex, AI_MATKEY_TEXTURE(type, 0));

    // Setup the texture blend factor
    if (is_not_qnan(texture.mTextureBlend))
        mat.AddProperty<ai_real>(&texture.mTextureBlend, 1, AI_MATKEY_TEXBLEND(type, 0));

    // Setup the texture mapping mode
    int mapMode = static_cast<int>(texture.mMapMode);
    mat.AddProperty<int>(&mapMode, 1, AI_MATKEY_MAPPINGMODE_U(type, 0));
    mat.AddProperty<int>(&mapMode, 1, AI_MATKEY_MAPPINGMODE_V(type, 0));

    // Mirroring - double the scaling values
    // FIXME: this is not really correct ...
    if (texture.mMapMode == aiTextureMapMode_Mirror) {
        texture.mScaleU *= 2.0;
        texture.mScaleV *= 2.0;
        texture.mOffsetU /= 2.0;
        texture.mOffsetV /= 2.0;
    }

    // Setup texture UV transformations
    mat.AddProperty<ai_real>(&texture.mOffsetU, 5, AI_MATKEY_UVTRANSFORM(type, 0));
}

// ------------------------------------------------------------------------------------------------
// Convert a 3DS material to an aiMaterial
void Discreet3DSImporter::ConvertMaterial(D3DS::Material &p_cMat,
        aiMaterial &p_pcOut) {
    // NOTE: Pass the background image to the viewer by bypassing the
    // material system. This is an evil hack, never do it again!
    if (0 != mBackgroundImage.length() && bHasBG) {
        aiString tex;
        tex.Set(mBackgroundImage);
        p_pcOut.AddProperty(&tex, AI_MATKEY_GLOBAL_BACKGROUND_IMAGE);

        // Be sure this is only done for the first material
        mBackgroundImage = std::string("");
    }

    // At first add the base ambient color of the scene to the material
    p_cMat.mAmbient.r += mClrAmbient.r;
    p_cMat.mAmbient.g += mClrAmbient.g;
    p_cMat.mAmbient.b += mClrAmbient.b;

    aiString name;
    name.Set(p_cMat.mName);
    p_pcOut.AddProperty(&name, AI_MATKEY_NAME);

    // Material colors
    p_pcOut.AddProperty(&p_cMat.mAmbient, 1, AI_MATKEY_COLOR_AMBIENT);
    p_pcOut.AddProperty(&p_cMat.mDiffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
    p_pcOut.AddProperty(&p_cMat.mSpecular, 1, AI_MATKEY_COLOR_SPECULAR);
    p_pcOut.AddProperty(&p_cMat.mEmissive, 1, AI_MATKEY_COLOR_EMISSIVE);

    // Phong shininess and shininess strength
    if (D3DS::Discreet3DS::ShadeType3DS::Phong == p_cMat.mShading ||
            D3DS::Discreet3DS::ShadeType3DS::Metal == p_cMat.mShading) {
        if (!p_cMat.mSpecularExponent || !p_cMat.mShininessStrength) {
            p_cMat.mShading = D3DS::Discreet3DS::ShadeType3DS::Gouraud;
        } else {
            p_pcOut.AddProperty(&p_cMat.mSpecularExponent, 1, AI_MATKEY_SHININESS);
            p_pcOut.AddProperty(&p_cMat.mShininessStrength, 1, AI_MATKEY_SHININESS_STRENGTH);
        }
    }

    // Opacity
    p_pcOut.AddProperty<ai_real>(&p_cMat.mTransparency, 1, AI_MATKEY_OPACITY);

    // Bump height scaling
    p_pcOut.AddProperty<ai_real>(&p_cMat.mBumpHeight, 1, AI_MATKEY_BUMPSCALING);

    // Two sided rendering?
    if (p_cMat.mTwoSided) {
        int i = 1;
        p_pcOut.AddProperty<int>(&i, 1, AI_MATKEY_TWOSIDED);
    }

    // Shading mode
    aiShadingMode eShading = aiShadingMode_NoShading;
    switch (p_cMat.mShading) {
    case D3DS::Discreet3DS::ShadeType3DS::Flat:
        eShading = aiShadingMode_Flat;
        break;

    // I don't know what "Wire" shading should be,
    // assume it is simple lambertian diffuse shading
    case D3DS::Discreet3DS::ShadeType3DS::Wire: {
        // Set the wireframe flag
        unsigned int iWire = 1;
        p_pcOut.AddProperty<int>((int *)&iWire, 1, AI_MATKEY_ENABLE_WIREFRAME);
    }

    case D3DS::Discreet3DS::ShadeType3DS::Gouraud:
        eShading = aiShadingMode_Gouraud;
        break;

    // assume cook-torrance shading for metals.
    case D3DS::Discreet3DS::ShadeType3DS::Phong:
        eShading = aiShadingMode_Phong;
        break;

    case D3DS::Discreet3DS::ShadeType3DS::Metal:
        eShading = aiShadingMode_CookTorrance;
        break;

        // FIX to workaround a warning with GCC 4 who complained
        // about a missing case Blinn: here - Blinn isn't a valid
        // value in the 3DS Loader, it is just needed for ASE
    case D3DS::Discreet3DS::ShadeType3DS::Blinn:
        eShading = aiShadingMode_Blinn;
        break;
    }
    int eShading_ = static_cast<int>(eShading);
    p_pcOut.AddProperty<int>(&eShading_, 1, AI_MATKEY_SHADING_MODEL);

    // DIFFUSE texture
    if (p_cMat.sTexDiffuse.mMapName.length() > 0)
        CopyTexture(p_pcOut, p_cMat.sTexDiffuse, aiTextureType_DIFFUSE);

    // SPECULAR texture
    if (p_cMat.sTexSpecular.mMapName.length() > 0)
        CopyTexture(p_pcOut, p_cMat.sTexSpecular, aiTextureType_SPECULAR);

    // OPACITY texture
    if (p_cMat.sTexOpacity.mMapName.length() > 0)
        CopyTexture(p_pcOut, p_cMat.sTexOpacity, aiTextureType_OPACITY);

    // EMISSIVE texture
    if (p_cMat.sTexEmissive.mMapName.length() > 0)
        CopyTexture(p_pcOut, p_cMat.sTexEmissive, aiTextureType_EMISSIVE);

    // BUMP texture
    if (p_cMat.sTexBump.mMapName.length() > 0)
        CopyTexture(p_pcOut, p_cMat.sTexBump, aiTextureType_HEIGHT);

    // SHININESS texture
    if (p_cMat.sTexShininess.mMapName.length() > 0)
        CopyTexture(p_pcOut, p_cMat.sTexShininess, aiTextureType_SHININESS);

    // REFLECTION texture
    if (p_cMat.sTexReflective.mMapName.length() > 0)
        CopyTexture(p_pcOut, p_cMat.sTexReflective, aiTextureType_REFLECTION);

    // Store the name of the material itself, too
    if (p_cMat.mName.length()) {
        aiString tex;
        tex.Set(p_cMat.mName);
        p_pcOut.AddProperty(&tex, AI_MATKEY_NAME);
    }
}

// ------------------------------------------------------------------------------------------------
// Split meshes by their materials and generate output aiMesh'es
void Discreet3DSImporter::ConvertMeshes(aiScene *p_cOut) {
    std::vector<aiMesh *> avOutMeshes;
    avOutMeshes.reserve(mScene->mMeshes.size() * 2);

    unsigned int iFaceCnt = 0, num = 0;
    aiString name;

    // we need to split all meshes by their materials
    for (std::vector<D3DS::Mesh>::iterator i = mScene->mMeshes.begin(); i != mScene->mMeshes.end(); ++i) {
        std::unique_ptr<std::vector<unsigned int>[]> aiSplit(new std::vector<unsigned int>[mScene->mMaterials.size()]);

        name.length = ASSIMP_itoa10(name.data, num++);

        unsigned int iNum = 0;
        for (std::vector<unsigned int>::const_iterator a = (*i).mFaceMaterials.begin();
                a != (*i).mFaceMaterials.end(); ++a, ++iNum) {
            aiSplit[*a].push_back(iNum);
        }
        // now generate submeshes
        for (unsigned int p = 0; p < mScene->mMaterials.size(); ++p) {
            if (aiSplit[p].empty()) {
                continue;
            }
            aiMesh *meshOut = new aiMesh();
            meshOut->mName = name;
            meshOut->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

            // be sure to setup the correct material index
            meshOut->mMaterialIndex = p;

            // use the color data as temporary storage
            meshOut->mColors[0] = (aiColor4D *)(&*i);
            avOutMeshes.push_back(meshOut);

            // convert vertices
            meshOut->mNumFaces = (unsigned int)aiSplit[p].size();
            meshOut->mNumVertices = meshOut->mNumFaces * 3;

            // allocate enough storage for faces
            meshOut->mFaces = new aiFace[meshOut->mNumFaces];
            iFaceCnt += meshOut->mNumFaces;

            meshOut->mVertices = new aiVector3D[meshOut->mNumVertices];
            meshOut->mNormals = new aiVector3D[meshOut->mNumVertices];
            if ((*i).mTexCoords.size()) {
                meshOut->mTextureCoords[0] = new aiVector3D[meshOut->mNumVertices];
            }
            for (unsigned int q = 0, base = 0; q < aiSplit[p].size(); ++q) {
                unsigned int index = aiSplit[p][q];
                aiFace &face = meshOut->mFaces[q];

                face.mIndices = new unsigned int[3];
                face.mNumIndices = 3;

                for (unsigned int a = 0; a < 3; ++a, ++base) {
                    unsigned int idx = (*i).mFaces[index].mIndices[a];
                    meshOut->mVertices[base] = (*i).mPositions[idx];
                    meshOut->mNormals[base] = (*i).mNormals[idx];

                    if ((*i).mTexCoords.size())
                        meshOut->mTextureCoords[0][base] = (*i).mTexCoords[idx];

                    face.mIndices[a] = base;
                }
            }
        }
    }

    // Copy them to the output array
    p_cOut->mNumMeshes = (unsigned int)avOutMeshes.size();
    p_cOut->mMeshes = new aiMesh *[p_cOut->mNumMeshes]();
    for (unsigned int a = 0; a < p_cOut->mNumMeshes; ++a) {
        p_cOut->mMeshes[a] = avOutMeshes[a];
    }

    // We should have at least one face here
    if (!iFaceCnt) {
        throw DeadlyImportError("No faces loaded. The mesh is empty");
    }
}

// ------------------------------------------------------------------------------------------------
// Add a node to the scenegraph and setup its final transformation
void Discreet3DSImporter::AddNodeToGraph(aiScene *const pcSOut, aiNode *const p_cOut,
        D3DS::Node *const pcIn, aiMatrix4x4 & /*absTrafo*/) {
    std::vector<unsigned int> iArray;
    iArray.reserve(3);

    aiMatrix4x4 abs;

    // Find all meshes with the same name as the node
    for (unsigned int a = 0; a < pcSOut->mNumMeshes; ++a) {
        const D3DS::Mesh *pcMesh = (const D3DS::Mesh *)pcSOut->mMeshes[a]->mColors[0];
        ai_assert(nullptr != pcMesh);

        if (pcIn->mName == pcMesh->mName)
            iArray.push_back(a);
    }

    if (!iArray.empty()) {
        // The matrix should be identical for all meshes with the
        // same name. It HAS to be identical for all meshes .....
        D3DS::Mesh *imesh = ((D3DS::Mesh *)pcSOut->mMeshes[iArray[0]]->mColors[0]);

        // Compute the inverse of the transformation matrix to move the
        // vertices back to their relative and local space
        aiMatrix4x4 mInv = imesh->mMat, mInvTransposed = imesh->mMat;
        mInv.Inverse();
        mInvTransposed.Transpose();
        aiVector3D pivot = pcIn->vPivot;

        p_cOut->mNumMeshes = (unsigned int)iArray.size();
        p_cOut->mMeshes = new unsigned int[iArray.size()];
        for (unsigned int i = 0; i < iArray.size(); ++i) {
            const unsigned int iIndex = iArray[i];
            aiMesh *const mesh = pcSOut->mMeshes[iIndex];

            if (mesh->mColors[1] == nullptr) {
                // Transform the vertices back into their local space
                // fixme: consider computing normals after this, so we don't need to transform them
                const aiVector3D *const pvEnd = mesh->mVertices + mesh->mNumVertices;
                aiVector3D *pvCurrent = mesh->mVertices, *t2 = mesh->mNormals;

                for (; pvCurrent != pvEnd; ++pvCurrent, ++t2) {
                    *pvCurrent = mInv * (*pvCurrent);
                    *t2 = mInvTransposed * (*t2);
                }

                // Handle negative transformation matrix determinant -> invert vertex x
                if (imesh->mMat.Determinant() < 0.0f) {
                    /* we *must* have normals */
                    for (pvCurrent = mesh->mVertices, t2 = mesh->mNormals; pvCurrent != pvEnd; ++pvCurrent, ++t2) {
                        pvCurrent->x *= -1.f;
                        t2->x *= -1.f;
                    }
                    ASSIMP_LOG_INFO("3DS: Flipping mesh X-Axis");
                }

                // Handle pivot point
                if (pivot.x || pivot.y || pivot.z) {
                    for (pvCurrent = mesh->mVertices; pvCurrent != pvEnd; ++pvCurrent) {
                        *pvCurrent -= pivot;
                    }
                }

                mesh->mColors[1] = (aiColor4D *)1;
            } else
                mesh->mColors[1] = (aiColor4D *)1;

            // Setup the mesh index
            p_cOut->mMeshes[i] = iIndex;
        }
    }

    // Setup the name of the node
    // First instance keeps its name otherwise something might break, all others will be postfixed with their instance number
    if (pcIn->mInstanceNumber > 1) {
        char tmp[12];
        ASSIMP_itoa10(tmp, pcIn->mInstanceNumber);
        std::string tempStr = pcIn->mName + "_inst_";
        tempStr += tmp;
        p_cOut->mName.Set(tempStr);
    } else
        p_cOut->mName.Set(pcIn->mName);

    // Now build the transformation matrix of the node
    // ROTATION
    if (pcIn->aRotationKeys.size()) {

        // FIX to get to Assimp's quaternion conventions
        for (std::vector<aiQuatKey>::iterator it = pcIn->aRotationKeys.begin(); it != pcIn->aRotationKeys.end(); ++it) {
            (*it).mValue.w *= -1.f;
        }

        p_cOut->mTransformation = aiMatrix4x4(pcIn->aRotationKeys[0].mValue.GetMatrix());
    } else if (pcIn->aCameraRollKeys.size()) {
        aiMatrix4x4::RotationZ(AI_DEG_TO_RAD(-pcIn->aCameraRollKeys[0].mValue),
                p_cOut->mTransformation);
    }

    // SCALING
    aiMatrix4x4 &m = p_cOut->mTransformation;
    if (pcIn->aScalingKeys.size()) {
        const aiVector3D &v = pcIn->aScalingKeys[0].mValue;
        m.a1 *= v.x;
        m.b1 *= v.x;
        m.c1 *= v.x;
        m.a2 *= v.y;
        m.b2 *= v.y;
        m.c2 *= v.y;
        m.a3 *= v.z;
        m.b3 *= v.z;
        m.c3 *= v.z;
    }

    // TRANSLATION
    if (pcIn->aPositionKeys.size()) {
        const aiVector3D &v = pcIn->aPositionKeys[0].mValue;
        m.a4 += v.x;
        m.b4 += v.y;
        m.c4 += v.z;
    }

    // Generate animation channels for the node
    if (pcIn->aPositionKeys.size() > 1 || pcIn->aRotationKeys.size() > 1 ||
            pcIn->aScalingKeys.size() > 1 || pcIn->aCameraRollKeys.size() > 1 ||
            pcIn->aTargetPositionKeys.size() > 1) {
        aiAnimation *anim = pcSOut->mAnimations[0];
        ai_assert(nullptr != anim);

        if (pcIn->aCameraRollKeys.size() > 1) {
            ASSIMP_LOG_VERBOSE_DEBUG("3DS: Converting camera roll track ...");

            // Camera roll keys - in fact they're just rotations
            // around the camera's z axis. The angles are given
            // in degrees (and they're clockwise).
            pcIn->aRotationKeys.resize(pcIn->aCameraRollKeys.size());
            for (unsigned int i = 0; i < pcIn->aCameraRollKeys.size(); ++i) {
                aiQuatKey &q = pcIn->aRotationKeys[i];
                aiFloatKey &f = pcIn->aCameraRollKeys[i];

                q.mTime = f.mTime;

                // FIX to get to Assimp quaternion conventions
                q.mValue = aiQuaternion(0.f, 0.f, AI_DEG_TO_RAD(/*-*/ f.mValue));
            }
        }
#if 0
        if (pcIn->aTargetPositionKeys.size() > 1)
        {
            ASSIMP_LOG_VERBOSE_DEBUG("3DS: Converting target track ...");

            // Camera or spot light - need to convert the separate
            // target position channel to our representation
            TargetAnimationHelper helper;

            if (pcIn->aPositionKeys.empty())
            {
                // We can just pass zero here ...
                helper.SetFixedMainAnimationChannel(aiVector3D());
            }
            else  helper.SetMainAnimationChannel(&pcIn->aPositionKeys);
            helper.SetTargetAnimationChannel(&pcIn->aTargetPositionKeys);

            // Do the conversion
            std::vector<aiVectorKey> distanceTrack;
            helper.Process(&distanceTrack);

            // Now add a new node as child, name it <ourName>.Target
            // and assign the distance track to it. This is that the
            // information where the target is and how it moves is
            // not lost
            D3DS::Node* nd = new D3DS::Node();
            pcIn->push_back(nd);

            nd->mName = pcIn->mName + ".Target";

            aiNodeAnim* nda = anim->mChannels[anim->mNumChannels++] = new aiNodeAnim();
            nda->mNodeName.Set(nd->mName);

            nda->mNumPositionKeys = (unsigned int)distanceTrack.size();
            nda->mPositionKeys = new aiVectorKey[nda->mNumPositionKeys];
            ::memcpy(nda->mPositionKeys,&distanceTrack[0],
                sizeof(aiVectorKey)*nda->mNumPositionKeys);
        }
#endif

        // Cameras or lights define their transformation in their parent node and in the
        // corresponding light or camera chunks. However, we read and process the latter
        // to to be able to return valid cameras/lights even if no scenegraph is given.
        for (unsigned int n = 0; n < pcSOut->mNumCameras; ++n) {
            if (pcSOut->mCameras[n]->mName == p_cOut->mName) {
                pcSOut->mCameras[n]->mLookAt = aiVector3D(0.f, 0.f, 1.f);
            }
        }
        for (unsigned int n = 0; n < pcSOut->mNumLights; ++n) {
            if (pcSOut->mLights[n]->mName == p_cOut->mName) {
                pcSOut->mLights[n]->mDirection = aiVector3D(0.f, 0.f, 1.f);
            }
        }

        // Allocate a new node anim and setup its name
        aiNodeAnim *nda = anim->mChannels[anim->mNumChannels++] = new aiNodeAnim();
        nda->mNodeName.Set(pcIn->mName);

        // POSITION keys
        if (pcIn->aPositionKeys.size() > 0) {
            nda->mNumPositionKeys = (unsigned int)pcIn->aPositionKeys.size();
            nda->mPositionKeys = new aiVectorKey[nda->mNumPositionKeys];
            ::memcpy(nda->mPositionKeys, &pcIn->aPositionKeys[0],
                    sizeof(aiVectorKey) * nda->mNumPositionKeys);
        }

        // ROTATION keys
        if (pcIn->aRotationKeys.size() > 0) {
            nda->mNumRotationKeys = (unsigned int)pcIn->aRotationKeys.size();
            nda->mRotationKeys = new aiQuatKey[nda->mNumRotationKeys];

            // Rotations are quaternion offsets
            aiQuaternion abs1;
            for (unsigned int n = 0; n < nda->mNumRotationKeys; ++n) {
                const aiQuatKey &q = pcIn->aRotationKeys[n];

                abs1 = (n ? abs1 * q.mValue : q.mValue);
                nda->mRotationKeys[n].mTime = q.mTime;
                nda->mRotationKeys[n].mValue = abs1.Normalize();
            }
        }

        // SCALING keys
        if (pcIn->aScalingKeys.size() > 0) {
            nda->mNumScalingKeys = (unsigned int)pcIn->aScalingKeys.size();
            nda->mScalingKeys = new aiVectorKey[nda->mNumScalingKeys];
            ::memcpy(nda->mScalingKeys, &pcIn->aScalingKeys[0],
                    sizeof(aiVectorKey) * nda->mNumScalingKeys);
        }
    }

    // Allocate storage for children
    p_cOut->mNumChildren = (unsigned int)pcIn->mChildren.size();
    p_cOut->mChildren = new aiNode *[pcIn->mChildren.size()];

    // Recursively process all children
    const unsigned int size = static_cast<unsigned int>(pcIn->mChildren.size());
    for (unsigned int i = 0; i < size; ++i) {
        p_cOut->mChildren[i] = new aiNode();
        p_cOut->mChildren[i]->mParent = p_cOut;
        AddNodeToGraph(pcSOut, p_cOut->mChildren[i], pcIn->mChildren[i], abs);
    }
}

// ------------------------------------------------------------------------------------------------
// Find out how many node animation channels we'll have finally
void CountTracks(const D3DS::Node *const node, unsigned int &cnt) {
    //////////////////////////////////////////////////////////////////////////////
    // We will never generate more than one channel for a node, so
    // this is rather easy here.

    if (node->aPositionKeys.size() > 1 || node->aRotationKeys.size() > 1 ||
            node->aScalingKeys.size() > 1 || node->aCameraRollKeys.size() > 1 ||
            node->aTargetPositionKeys.size() > 1) {
        ++cnt;

        // account for the additional channel for the camera/spotlight target position
        if (node->aTargetPositionKeys.size() > 1) ++cnt;
    }

    // Recursively process all children
    for (unsigned int i = 0; i < node->mChildren.size(); ++i)
        CountTracks(node->mChildren[i], cnt);
}

// ------------------------------------------------------------------------------------------------
// Generate the output node graph
void Discreet3DSImporter::GenerateNodeGraph(aiScene *p_cOut,
        D3DS::Node *p_rootNode) {
    p_cOut->mRootNode = new aiNode();
    if (0 == p_rootNode->mChildren.size()) {
        //////////////////////////////////////////////////////////////////////////////
        // It seems the file is so messed up that it does not even have a hierarchy.
        // Generate a flat hiearachy which looks like this:
        //
        //                ROOT_NODE
        //                   |
        //   ----------------------------------------
        //   |       |       |            |         |
        // MESH_0  MESH_1  MESH_2  ...  MESH_N    CAMERA_0 ....
        //
        ASSIMP_LOG_WARN("No hierarchy information has been found in the file. ");

        p_cOut->mRootNode->mNumChildren = p_cOut->mNumMeshes +
                                          static_cast<unsigned int>(mScene->mCameras.size() + mScene->mLights.size());

        p_cOut->mRootNode->mChildren = new aiNode *[p_cOut->mRootNode->mNumChildren];
        p_cOut->mRootNode->mName.Set("<3DSDummyRoot>");

        // Build dummy nodes for all meshes
        unsigned int a = 0;
        for (unsigned int i = 0; i < p_cOut->mNumMeshes; ++i, ++a) {
            aiNode *pcNode = p_cOut->mRootNode->mChildren[a] = new aiNode();
            pcNode->mParent = p_cOut->mRootNode;
            pcNode->mMeshes = new unsigned int[1];
            pcNode->mMeshes[0] = i;
            pcNode->mNumMeshes = 1;

            // Build a name for the node
            pcNode->mName.length = ai_snprintf(pcNode->mName.data, MAXLEN, "3DSMesh_%u", i);
        }

        // Build dummy nodes for all cameras
        for (unsigned int i = 0; i < (unsigned int)mScene->mCameras.size(); ++i, ++a) {
            aiNode *pcNode = p_cOut->mRootNode->mChildren[a] = new aiNode();
            pcNode->mParent = p_cOut->mRootNode;

            // Build a name for the node
            pcNode->mName = mScene->mCameras[i]->mName;
        }

        // Build dummy nodes for all lights
        for (unsigned int i = 0; i < (unsigned int)mScene->mLights.size(); ++i, ++a) {
            aiNode *pcNode = p_cOut->mRootNode->mChildren[a] = new aiNode();
            pcNode->mParent = p_cOut->mRootNode;

            // Build a name for the node
            pcNode->mName = mScene->mLights[i]->mName;
        }
    } else {
        // First of all: find out how many scaling, rotation and translation
        // animation tracks we'll have afterwards
        unsigned int numChannel = 0;
        CountTracks(p_rootNode, numChannel);

        if (numChannel) {
            // Allocate a primary animation channel
            p_cOut->mNumAnimations = 1;
            p_cOut->mAnimations = new aiAnimation *[1];
            aiAnimation *anim = p_cOut->mAnimations[0] = new aiAnimation();

            anim->mName.Set("3DSMasterAnim");

            // Allocate enough storage for all node animation channels,
            // but don't set the mNumChannels member - we'll use it to
            // index into the array
            anim->mChannels = new aiNodeAnim *[numChannel];
        }

        aiMatrix4x4 m;
        AddNodeToGraph(p_cOut, p_cOut->mRootNode, p_rootNode, m);
    }

    // We used the first and second vertex color set to store some temporary values so we need to cleanup here
    for (unsigned int a = 0; a < p_cOut->mNumMeshes; ++a) {
        p_cOut->mMeshes[a]->mColors[0] = nullptr;
        p_cOut->mMeshes[a]->mColors[1] = nullptr;
    }

    p_cOut->mRootNode->mTransformation = aiMatrix4x4(
                                                 1.f, 0.f, 0.f, 0.f,
                                                 0.f, 0.f, 1.f, 0.f,
                                                 0.f, -1.f, 0.f, 0.f,
                                                 0.f, 0.f, 0.f, 1.f) *
                                         p_cOut->mRootNode->mTransformation;

    // If the root node is unnamed name it "<3DSRoot>"
    if (::strstr(p_cOut->mRootNode->mName.data, "UNNAMED") ||
            (p_cOut->mRootNode->mName.data[0] == '$' && p_cOut->mRootNode->mName.data[1] == '$')) {
        p_cOut->mRootNode->mName.Set("<3DSRoot>");
    }
}

// ------------------------------------------------------------------------------------------------
// Convert all meshes in the scene and generate the final output scene.
void Discreet3DSImporter::ConvertScene(aiScene *p_cOut) {
    // Allocate enough storage for all output materials
    p_cOut->mNumMaterials = (unsigned int)mScene->mMaterials.size();
    p_cOut->mMaterials = new aiMaterial *[p_cOut->mNumMaterials];

    //  ... and convert the 3DS materials to aiMaterial's
    for (unsigned int i = 0; i < p_cOut->mNumMaterials; ++i) {
        aiMaterial *pcNew = new aiMaterial();
        ConvertMaterial(mScene->mMaterials[i], *pcNew);
        p_cOut->mMaterials[i] = pcNew;
    }

    // Generate the output mesh list
    ConvertMeshes(p_cOut);

    // Now copy all light sources to the output scene
    p_cOut->mNumLights = (unsigned int)mScene->mLights.size();
    if (p_cOut->mNumLights) {
        p_cOut->mLights = new aiLight *[p_cOut->mNumLights];
        ::memcpy(p_cOut->mLights, &mScene->mLights[0], sizeof(void *) * p_cOut->mNumLights);
    }

    // Now copy all cameras to the output scene
    p_cOut->mNumCameras = (unsigned int)mScene->mCameras.size();
    if (p_cOut->mNumCameras) {
        p_cOut->mCameras = new aiCamera *[p_cOut->mNumCameras];
        ::memcpy(p_cOut->mCameras, &mScene->mCameras[0], sizeof(void *) * p_cOut->mNumCameras);
    }
}

#endif // !! ASSIMP_BUILD_NO_3DS_IMPORTER
