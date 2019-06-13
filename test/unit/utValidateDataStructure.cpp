/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2019, assimp team



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
#include "UnitTestPCH.h"

#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <ValidateDataStructure.h>

using namespace std;
using namespace Assimp;


class ValidateDataStructureTest : public ::testing::Test
{
public:

    virtual void SetUp();
    virtual void TearDown();

protected:


    ValidateDSProcess* vds;
    aiScene* scene;
};

// ------------------------------------------------------------------------------------------------
void ValidateDataStructureTest::SetUp()
{
    // setup a dummy scene with a single node
    scene = new aiScene();
    scene->mRootNode = new aiNode();
    scene->mRootNode->mName.Set("<test>");

    // add some translation
    scene->mRootNode->mTransformation.a4 = 1.f;
    scene->mRootNode->mTransformation.b4 = 2.f;
    scene->mRootNode->mTransformation.c4 = 3.f;

    // and allocate a ScenePreprocessor to operate on the scene
    vds = new ValidateDSProcess();
}

// ------------------------------------------------------------------------------------------------
void ValidateDataStructureTest::TearDown()
{
    delete vds;
    delete scene;
}



// ------------------------------------------------------------------------------------------------
//Template
//TEST_F(ScenePreprocessorTest, test)
//{
//}
// TODO Conditions not yet checked:
//132: ReportError("aiScene::%s is NULL (aiScene::%s is %i)",
//139: ReportError("aiScene::%s[%i] is NULL (aiScene::%s is %i)",
//156: ReportError("aiScene::%s is NULL (aiScene::%s is %i)",
//163: ReportError("aiScene::%s[%i] is NULL (aiScene::%s is %i)",
//173: ReportError("aiScene::%s[%i] has the same name as "
//192: ReportError("aiScene::%s[%i] has no corresponding node in the scene graph (%s)",
//196: ReportError("aiScene::%s[%i]: there are more than one nodes with %s as name",
//217: ReportError("aiScene::mNumMeshes is 0. At least one mesh must be there");
//220: ReportError("aiScene::mMeshes is non-null although there are no meshes");
//229: ReportError("aiScene::mAnimations is non-null although there are no animations");
//238: ReportError("aiScene::mCameras is non-null although there are no cameras");
//247: ReportError("aiScene::mLights is non-null although there are no lights");
//256: ReportError("aiScene::mTextures is non-null although there are no textures");
//266: ReportError("aiScene::mNumMaterials is 0. At least one material must be there");
//270: ReportError("aiScene::mMaterials is non-null although there are no materials");
//281: ReportWarning("aiLight::mType is aiLightSource_UNDEFINED");
//286: ReportWarning("aiLight::mAttenuationXXX - all are zero");
//290: ReportError("aiLight::mAngleInnerCone is larger than aiLight::mAngleOuterCone");
//295: ReportWarning("aiLight::mColorXXX - all are black and won't have any influence");
//303: ReportError("aiCamera::mClipPlaneFar must be >= aiCamera::mClipPlaneNear");
//308: ReportWarning("%f is not a valid value for aiCamera::mHorizontalFOV",pCamera->mHorizontalFOV);
//317: ReportError("aiMesh::mMaterialIndex is invalid (value: %i maximum: %i)",
//332: ReportError("aiMesh::mFaces[%i].mNumIndices is 0",i);
//336: ReportError("aiMesh::mFaces[%i] is a POINT but aiMesh::mPrimitiveTypes "
//337: "does not report the POINT flag",i);
//343: ReportError("aiMesh::mFaces[%i] is a LINE but aiMesh::mPrimitiveTypes "
//344: "does not report the LINE flag",i);
//350: ReportError("aiMesh::mFaces[%i] is a TRIANGLE but aiMesh::mPrimitiveTypes "
//351: "does not report the TRIANGLE flag",i);
//357: this->ReportError("aiMesh::mFaces[%i] is a POLYGON but aiMesh::mPrimitiveTypes "
//358: "does not report the POLYGON flag",i);
//365: ReportError("aiMesh::mFaces[%i].mIndices is NULL",i);
//370: ReportError("The mesh %s contains no vertices", pMesh->mName.C_Str());
//374: ReportError("Mesh has too many vertices: %u, but the limit is %u",pMesh->mNumVertices,AI_MAX_VERTICES);
//377: ReportError("Mesh has too many faces: %u, but the limit is %u",pMesh->mNumFaces,AI_MAX_FACES);
//382: ReportError("If there are tangents, bitangent vectors must be present as well");
//387: ReportError("Mesh %s contains no faces", pMesh->mName.C_Str());
//398: ReportError("Face %u has too many faces: %u, but the limit is %u",i,face.mNumIndices,AI_MAX_FACE_INDICES);
//404: ReportError("aiMesh::mFaces[%i]::mIndices[%i] is out of range",i,a);
//412: ReportError("aiMesh::mVertices[%i] is referenced twice - second "
//426: ReportWarning("There are unreferenced vertices");
//439: ReportError("Texture coordinate channel %i exists "
//453: ReportError("Vertex color channel %i is exists "
//464: ReportError("aiMesh::mBones is NULL (aiMesh::mNumBones is %i)",
//480: ReportError("Bone %u has too many weights: %u, but the limit is %u",i,bone->mNumWeights,AI_MAX_BONE_WEIGHTS);
//485: ReportError("aiMesh::mBones[%i] is NULL (aiMesh::mNumBones is %i)",
//498: ReportError("aiMesh::mBones[%i], name = \"%s\" has the same name as "
//507: ReportWarning("aiMesh::mVertices[%i]: bone weight sum != 1.0 (sum is %f)",i,afSum[i]);
//513: ReportError("aiMesh::mBones is non-null although there are no bones");
//524: ReportError("aiBone::mNumWeights is zero");
//531: ReportError("aiBone::mWeights[%i].mVertexId is out of range",i);
//534: ReportWarning("aiBone::mWeights[%i].mWeight has an invalid value",i);
//549: ReportError("aiAnimation::mChannels is NULL (aiAnimation::mNumChannels is %i)",
//556: ReportError("aiAnimation::mChannels[%i] is NULL (aiAnimation::mNumChannels is %i)",
//563: ReportError("aiAnimation::mNumChannels is 0. At least one node animation channel must be there.");
//567: // if (!pAnimation->mDuration)this->ReportError("aiAnimation::mDuration is zero");
//592: ReportError("Material property %s is expected to be a string",prop->mKey.data);
//596: ReportError("%s #%i is set, but there are only %i %s textures",
//611: ReportError("Found texture property with index %i, although there "
//619: ReportError("Material property %s%i is expected to be an integer (size is %i)",
//627: ReportError("Material property %s%i is expected to be 5 floats large (size is %i)",
//635: ReportError("Material property %s%i is expected to be an integer (size is %i)",
//656: ReportWarning("Invalid UV index: %i (key %s). Mesh %i has only %i UV channels",
//676: ReportWarning("UV-mapped texture, but there are no UV coords");
//690: ReportError("aiMaterial::mProperties[%i] is NULL (aiMaterial::mNumProperties is %i)",
//694: ReportError("aiMaterial::mProperties[%i].mDataLength or "
//702: ReportError("aiMaterial::mProperties[%i].mDataLength is "
//707: ReportError("Missing null-terminator in string material property");
//713: ReportError("aiMaterial::mProperties[%i].mDataLength is "
//720: ReportError("aiMaterial::mProperties[%i].mDataLength is "
//739: ReportWarning("A specular shading model is specified but there is no "
//743: ReportWarning("A specular shading model is specified but the value of the "
//752: ReportWarning("Invalid opacity value (must be 0 < opacity < 1.0)");
//776: ReportError("aiTexture::pcData is NULL");
//781: ReportError("aiTexture::mWidth is zero (aiTexture::mHeight is %i, uncompressed texture)",
//788: ReportError("aiTexture::mWidth is zero (compressed texture)");
//791: ReportWarning("aiTexture::achFormatHint must be zero-terminated");
//794: ReportWarning("aiTexture::achFormatHint should contain a file extension "
//804: ReportError("aiTexture::achFormatHint contains non-lowercase letters");
//815: ReportError("Empty node animation channel");
//822: ReportError("aiNodeAnim::mPositionKeys is NULL (aiNodeAnim::mNumPositionKeys is %i)",
//833: ReportError("aiNodeAnim::mPositionKeys[%i].mTime (%.5f) is larger "
//840: ReportWarning("aiNodeAnim::mPositionKeys[%i].mTime (%.5f) is smaller "
//853: ReportError("aiNodeAnim::mRotationKeys is NULL (aiNodeAnim::mNumRotationKeys is %i)",
//861: ReportError("aiNodeAnim::mRotationKeys[%i].mTime (%.5f) is larger "
//868: ReportWarning("aiNodeAnim::mRotationKeys[%i].mTime (%.5f) is smaller "
//880: ReportError("aiNodeAnim::mScalingKeys is NULL (aiNodeAnim::mNumScalingKeys is %i)",
//888: ReportError("aiNodeAnim::mScalingKeys[%i].mTime (%.5f) is larger "
//895: ReportWarning("aiNodeAnim::mScalingKeys[%i].mTime (%.5f) is smaller "
//907: ReportError("A node animation channel must have at least one subtrack");
//915: ReportError("A node of the scenegraph is NULL");
//920: ReportError("Non-root node %s lacks a valid parent (aiNode::mParent is NULL) ",pNode->mName);
//928: ReportError("aiNode::mMeshes is NULL for node %s (aiNode::mNumMeshes is %i)",
//937: ReportError("aiNode::mMeshes[%i] is out of range for node %s (maximum is %i)",
//942: ReportError("aiNode::mMeshes[%i] is already referenced by this node %s (value: %i)",
//951: ReportError("aiNode::mChildren is NULL for node %s (aiNode::mNumChildren is %i)",
//965: ReportError("aiString::length is too large (%i, maximum is %lu)",
//974: ReportError("aiString::data is invalid: the terminal zero is at a wrong offset");
//979: ReportError("aiString::data is invalid. There is no terminal character");
}

