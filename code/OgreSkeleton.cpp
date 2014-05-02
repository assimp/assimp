/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
All rights reserved.

Redistribution and use of this software in aSource and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of aSource code must retain the above
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

----------------------------------------------------------------------
*/

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include "OgreImporter.h"
#include "TinyFormatter.h"

using namespace std;

namespace Assimp
{
namespace Ogre
{

void OgreImporter::ReadSkeleton(const std::string &pFile, Assimp::IOSystem *pIOHandler, const aiScene *pScene,
							    const std::string &skeletonFile, vector<Bone> &Bones, vector<Animation> &Animations) const
{
	string filename = skeletonFile;
	if (EndsWith(filename, ".skeleton"))
	{
		DefaultLogger::get()->warn("Mesh is referencing a Ogre binary skeleton. Parsing binary Ogre assets is not supported at the moment. Trying to find .skeleton.xml file instead.");
		filename += ".xml";
	}

	if (!pIOHandler->Exists(filename))
	{
		DefaultLogger::get()->error("Failed to find skeleton file '" + filename + "', skeleton will be missing.");
		return;
	}

	boost::scoped_ptr<IOStream> file(pIOHandler->Open(filename));
	if (!file.get()) {
		throw DeadlyImportError("Failed to open skeleton file " + filename);
	}

	boost::scoped_ptr<CIrrXML_IOStreamReader> stream(new CIrrXML_IOStreamReader(file.get()));
	XmlReader* reader = irr::io::createIrrXMLReader(stream.get());
	if (!reader) {
		throw DeadlyImportError("Failed to create XML reader for skeleton file " + filename);
	}

	DefaultLogger::get()->debug("Reading skeleton '" + filename + "'");

	// Root
	NextNode(reader);
	if (!CurrentNodeNameEquals(reader, "skeleton")) {
		throw DeadlyImportError("Root node is not <skeleton> but <" + string(reader->getNodeName()) + "> in " + filename);
	}

	// Bones
	NextNode(reader);
	if (!CurrentNodeNameEquals(reader, "bones")) {
		throw DeadlyImportError("No <bones> node in skeleton " + skeletonFile);
	}

	NextNode(reader);
	while(CurrentNodeNameEquals(reader, "bone"))
	{
		/** @todo Fix this mandatory ordering. Some exporters might just write rotation first etc.
			There is no technical reason this has to be so strict. */

		Bone bone;
		bone.Id = GetAttribute<int>(reader, "id");
		bone.Name = GetAttribute<string>(reader, "name");

		NextNode(reader);
		if (!CurrentNodeNameEquals(reader, "position")) {
			throw DeadlyImportError("Position is not first node in Bone!");
		}

		bone.Position.x = GetAttribute<float>(reader, "x");
		bone.Position.y = GetAttribute<float>(reader, "y");
		bone.Position.z = GetAttribute<float>(reader, "z");

		NextNode(reader);
		if (!CurrentNodeNameEquals(reader, "rotation")) {
			throw DeadlyImportError("Rotation is not the second node in Bone!");
		}
		
		bone.RotationAngle = GetAttribute<float>(reader, "angle");
		
		NextNode(reader);
		if (!CurrentNodeNameEquals(reader, "axis")) {
			throw DeadlyImportError("No axis specified for bone rotation!");
		}
			
		bone.RotationAxis.x = GetAttribute<float>(reader, "x");
		bone.RotationAxis.y = GetAttribute<float>(reader, "y");
		bone.RotationAxis.z = GetAttribute<float>(reader, "z");

		Bones.push_back(bone);

		NextNode(reader);
	}

	// Order bones by Id
	std::sort(Bones.begin(), Bones.end());

	// Validate that bone indexes are not skipped.
	/** @note Left this from original authors code, but not sure if this is strictly necessary
		as per the Ogre skeleton spec. It might be more that other (later) code in this imported does not break. */
	for (size_t i=0, len=Bones.size(); i<len; ++i)
	{
		if (static_cast<int>(Bones[i].Id) != static_cast<int>(i)) {
			throw DeadlyImportError("Bone Ids are not in sequence in " + skeletonFile);
		}
	}

	DefaultLogger::get()->debug(Formatter::format() << "  - Bones " << Bones.size());

	// Bone hierarchy
	if (!CurrentNodeNameEquals(reader, "bonehierarchy")) {
		throw DeadlyImportError("No <bonehierarchy> node found after <bones> in " + skeletonFile);
	}

	NextNode(reader);
	while(CurrentNodeNameEquals(reader, "boneparent"))
	{
		string childName = GetAttribute<string>(reader, "bone");
		string parentName = GetAttribute<string>(reader, "parent");

		vector<Bone>::iterator iterChild = find(Bones.begin(), Bones.end(), childName);
		vector<Bone>::iterator iterParent = find(Bones.begin(), Bones.end(), parentName);
		
		if (iterChild != Bones.end() && iterParent != Bones.end())
		{
			iterChild->ParentId = iterParent->Id;
			iterParent->Children.push_back(iterChild->Id);
		}
		else
		{
			DefaultLogger::get()->warn("Failed to find bones for parenting: Child " + childName + " Parent " + parentName);
		}

		NextNode(reader);
	}

	// Calculate bone matrices for root bones. Recursively does their children.
	BOOST_FOREACH(Bone &theBone, Bones)
	{
		if (!theBone.IsParented()) {
			theBone.CalculateBoneToWorldSpaceMatrix(Bones);
		}
	}
	
	aiVector3D zeroVec(0.f, 0.f, 0.f);

	// Animations
	if (CurrentNodeNameEquals(reader, "animations"))
	{
		DefaultLogger::get()->debug("  - Animations");

		NextNode(reader);
		while(CurrentNodeNameEquals(reader, "animation"))
		{
			Animation animation;
			animation.Name = GetAttribute<string>(reader, "name");
			animation.Length = GetAttribute<float>(reader, "length");
			
			// Tracks
			NextNode(reader);
			if (!CurrentNodeNameEquals(reader, "tracks")) {
				throw DeadlyImportError("No <tracks> node found in animation '" + animation.Name + "' in " + skeletonFile);
			}

			NextNode(reader);
			while(CurrentNodeNameEquals(reader, "track"))
			{
				Track track;
				track.BoneName = GetAttribute<string>(reader, "bone");

				// Keyframes
				NextNode(reader);
				if (!CurrentNodeNameEquals(reader, "keyframes")) {
					throw DeadlyImportError("No <keyframes> node found in a track in animation '" + animation.Name + "' in " + skeletonFile);
				}

				NextNode(reader);
				while(CurrentNodeNameEquals(reader, "keyframe"))
				{
					KeyFrame keyFrame;
					keyFrame.Time = GetAttribute<float>(reader, "time");
				
					NextNode(reader);
					while(CurrentNodeNameEquals(reader, "translate") || CurrentNodeNameEquals(reader, "rotate") || CurrentNodeNameEquals(reader, "scale"))
					{
						if (CurrentNodeNameEquals(reader, "translate"))
						{
							keyFrame.Position.x = GetAttribute<float>(reader, "x");
							keyFrame.Position.y = GetAttribute<float>(reader, "y");
							keyFrame.Position.z = GetAttribute<float>(reader, "z");
						}
						else if (CurrentNodeNameEquals(reader, "rotate"))
						{
							float angle = GetAttribute<float>(reader, "angle");

							NextNode(reader);
							if (!CurrentNodeNameEquals(reader, "axis")) {
								throw DeadlyImportError("No axis for keyframe rotation in animation '" + animation.Name + "'");
							}
								
							aiVector3D axis;
							axis.x = GetAttribute<float>(reader, "x");
							axis.y = GetAttribute<float>(reader, "y");
							axis.z = GetAttribute<float>(reader, "z");

							if (axis.Equal(zeroVec))
							{
								axis.x = 1.0f;
								if (angle != 0) {
									DefaultLogger::get()->warn("Found invalid a key frame with a zero rotation axis in animation '" + animation.Name + "'");
								}
							}
							keyFrame.Rotation = aiQuaternion(axis, angle);
						}
						else if (CurrentNodeNameEquals(reader, "scale"))
						{
							keyFrame.Scaling.x = GetAttribute<float>(reader, "x");
							keyFrame.Scaling.y = GetAttribute<float>(reader, "y");
							keyFrame.Scaling.z = GetAttribute<float>(reader, "z");
						}	
						NextNode(reader);
					}
					track.Keyframes.push_back(keyFrame);
				}
				animation.Tracks.push_back(track);
			}
			Animations.push_back(animation);
			
			DefaultLogger::get()->debug(Formatter::format() << "      " << animation.Name << " (" << animation.Length << " sec, " << animation.Tracks.size() << " tracks)");
		}
	}
}

void OgreImporter::CreateAssimpSkeleton(aiScene *pScene, const std::vector<Bone> &bones, const std::vector<Animation> &animations)
{
	if (bones.empty()) {
		return;
	}

	if (!pScene->mRootNode) {
		throw DeadlyImportError("Creating Assimp skeleton: No root node created!");
	}
	if (pScene->mRootNode->mNumChildren > 0) {
		throw DeadlyImportError("Creating Assimp skeleton: Root node already has children!");
	}

	// Bones
	vector<aiNode*> rootBones;
	BOOST_FOREACH(const Bone &bone, bones)
	{
		if (!bone.IsParented()) {
			rootBones.push_back(CreateNodeFromBone(bone.Id, bones, pScene->mRootNode));
		}
	}
	
	if (!rootBones.empty())
	{
		pScene->mRootNode->mChildren = new aiNode*[rootBones.size()];
		pScene->mRootNode->mNumChildren = rootBones.size();

		for(size_t i=0, len=rootBones.size(); i<len; ++i) {
			pScene->mRootNode->mChildren[i] = rootBones[i];
		}
	}

	// TODO: Auf nicht vorhandene Animationskeys achten!
	// @todo Pay attention to non-existing animation Keys (google translated from above german comment)
	
	// Animations
	if (!animations.empty())
	{
		pScene->mAnimations = new aiAnimation*[animations.size()];
		pScene->mNumAnimations = animations.size();
		
		for(size_t ai=0, alen=animations.size(); ai<alen; ++ai)
		{
			const Animation &aSource = animations[ai];

			aiAnimation *animation = new aiAnimation();
			animation->mName = aSource.Name;
			animation->mDuration = aSource.Length;
			animation->mTicksPerSecond = 1.0f;

			// Tracks
			animation->mChannels = new aiNodeAnim*[aSource.Tracks.size()];
			animation->mNumChannels = aSource.Tracks.size();
			
			for(size_t ti=0, tlen=aSource.Tracks.size(); ti<tlen; ++ti)
			{
				const Track &tSource = aSource.Tracks[ti];

				aiNodeAnim *animationNode = new aiNodeAnim();
				animationNode->mNodeName = tSource.BoneName;

				// We need this, to access the bones default pose. 
				// Which we need to make keys absolute to the default bone pose.
				vector<Bone>::const_iterator boneIter = find(bones.begin(), bones.end(), tSource.BoneName);
				if (boneIter == bones.end())
				{
					for(size_t createdAnimationIndex=0; createdAnimationIndex<ai; createdAnimationIndex++) {
						delete pScene->mAnimations[createdAnimationIndex];
					}
					delete [] pScene->mAnimations;
					pScene->mAnimations = NULL;
					pScene->mNumAnimations = 0;
					
					DefaultLogger::get()->error("Failed to find bone for name " + tSource.BoneName + " when creating animation " + aSource.Name + 
						". This is a serious error, animations wont be imported.");
					return;
				}

				aiMatrix4x4 t0, t1;
				aiMatrix4x4 defaultBonePose = aiMatrix4x4::Translation(boneIter->Position, t1) * aiMatrix4x4::Rotation(boneIter->RotationAngle, boneIter->RotationAxis, t0);

				// Keyframes
				unsigned int numKeyframes = tSource.Keyframes.size();

				animationNode->mPositionKeys = new aiVectorKey[numKeyframes];				
				animationNode->mRotationKeys = new aiQuatKey[numKeyframes];
				animationNode->mScalingKeys = new aiVectorKey[numKeyframes];
				animationNode->mNumPositionKeys = numKeyframes;
				animationNode->mNumRotationKeys = numKeyframes;
				animationNode->mNumScalingKeys  = numKeyframes;

				//...and fill them
				for(size_t kfi=0; kfi<numKeyframes; ++kfi)
				{
					const KeyFrame &kfSource = tSource.Keyframes[kfi];

					// Create a matrix to transform a vector from the bones 
					// default pose to the bone bones in this animation key
					aiMatrix4x4 t2, t3;
					aiMatrix4x4 keyBonePose =
						aiMatrix4x4::Translation(kfSource.Position, t3) *
						aiMatrix4x4(kfSource.Rotation.GetMatrix()) *
						aiMatrix4x4::Scaling(kfSource.Scaling, t2);

					// Calculate the complete transformation from world space to bone space
					aiMatrix4x4 CompleteTransform = defaultBonePose * keyBonePose;

					aiVector3D kfPos; aiQuaternion kfRot; aiVector3D kfScale;
					CompleteTransform.Decompose(kfScale, kfRot, kfPos);

					animationNode->mPositionKeys[kfi].mTime = static_cast<double>(kfSource.Time);
					animationNode->mRotationKeys[kfi].mTime = static_cast<double>(kfSource.Time);
					animationNode->mScalingKeys[kfi].mTime = static_cast<double>(kfSource.Time);

					animationNode->mPositionKeys[kfi].mValue = kfPos;					
					animationNode->mRotationKeys[kfi].mValue = kfRot;
					animationNode->mScalingKeys[kfi].mValue = kfScale;
				}
				animation->mChannels[ti] = animationNode;
			}
			pScene->mAnimations[ai] = animation;
		}
	}
}

aiNode* OgreImporter::CreateNodeFromBone(int boneId, const std::vector<Bone> &bones, aiNode* parent)
{
	aiMatrix4x4 t0,t1;
	const Bone &source = bones[boneId];

	aiNode* boneNode = new aiNode(source.Name);
	boneNode->mParent = parent;
	boneNode->mTransformation = aiMatrix4x4::Translation(source.Position, t0) * aiMatrix4x4::Rotation(source.RotationAngle, source.RotationAxis, t1);

	if (!source.Children.empty())
	{
		boneNode->mChildren = new aiNode*[source.Children.size()];
		boneNode->mNumChildren = source.Children.size();

		for(size_t i=0, len=source.Children.size(); i<len; ++i) {
			boneNode->mChildren[i] = CreateNodeFromBone(source.Children[i], bones, boneNode);
		}
	}

	return boneNode;
}

void Bone::CalculateBoneToWorldSpaceMatrix(vector<Bone> &Bones)
{
	aiMatrix4x4 t0, t1;
	aiMatrix4x4 transform = aiMatrix4x4::Rotation(-RotationAngle, RotationAxis, t1) * aiMatrix4x4::Translation(-Position, t0);

	if (!IsParented())
	{
		BoneToWorldSpace = transform;
	}
	else
	{
		BoneToWorldSpace = transform * Bones[ParentId].BoneToWorldSpace;
	}

	// Recursively for all children now that the parent matrix has been calculated.
	BOOST_FOREACH(int childId, Children)
	{
		Bones[childId].CalculateBoneToWorldSpaceMatrix(Bones);
	}
}

} // Ogre
} // Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
