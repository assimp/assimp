/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
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
	if (!file.get())
		throw DeadlyImportError("Failed to open skeleton file " + filename);

	boost::scoped_ptr<CIrrXML_IOStreamReader> stream(new CIrrXML_IOStreamReader(file.get()));
	XmlReader* reader = irr::io::createIrrXMLReader(stream.get());
	if (!reader)
		throw DeadlyImportError("Failed to create XML reader for skeleton file " + filename);

	DefaultLogger::get()->debug("Reading skeleton '" + filename + "'");

	// Root
	NextNode(reader);
	if (!CurrentNodeNameEquals(reader, "skeleton"))
		throw DeadlyImportError("Root node is not <skeleton> but <" + string(reader->getNodeName()) + "> in " + filename);

	// Bones
	NextNode(reader);
	if (!CurrentNodeNameEquals(reader, "bones"))
		throw DeadlyImportError("No <bones> node in skeleton " + skeletonFile);

	NextNode(reader);
	while(CurrentNodeNameEquals(reader, "bone"))
	{
		//TODO: Maybe we can have bone ids for the errrors, but normaly, they should never appear, so what....
		/// @todo What does the above mean?

		Bone bone;
		bone.Id = GetAttribute<int>(reader, "id");
		bone.Name = GetAttribute<string>(reader, "name");

		NextNode(reader);
		if (!CurrentNodeNameEquals(reader, "position"))
			throw DeadlyImportError("Position is not first node in Bone!");

		bone.Position.x = GetAttribute<float>(reader, "x");
		bone.Position.y = GetAttribute<float>(reader, "y");
		bone.Position.z = GetAttribute<float>(reader, "z");

		NextNode(reader);
		if (!CurrentNodeNameEquals(reader, "rotation"))
			throw DeadlyImportError("Rotation is not the second node in Bone!");
		
		bone.RotationAngle = GetAttribute<float>(reader, "angle");
		
		NextNode(reader);
		if (!CurrentNodeNameEquals(reader, "axis"))
			throw DeadlyImportError("No axis specified for bone rotation!");
			
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
		if (static_cast<int>(Bones[i].Id) != static_cast<int>(i))
			throw DeadlyImportError("Bone Ids are not in sequence in " + skeletonFile);

	DefaultLogger::get()->debug(Formatter::format() << "  - Bones " << Bones.size());

	// Bone hierarchy
	if (!CurrentNodeNameEquals(reader, "bonehierarchy"))
		throw DeadlyImportError("No <bonehierarchy> node found after <bones> in " + skeletonFile);

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
			DefaultLogger::get()->warn("Failed to find bones for parenting: Child " + childName + " Parent " + parentName);

		NextNode(reader);
	}

	// Calculate bone matrices for root bones. Recursively does their children.
	BOOST_FOREACH(Bone &theBone, Bones)
	{
		if (!theBone.IsParented())
			theBone.CalculateBoneToWorldSpaceMatrix(Bones);
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
			if (!CurrentNodeNameEquals(reader, "tracks"))
				throw DeadlyImportError("No <tracks> node found in animation '" + animation.Name + "' in " + skeletonFile);

			NextNode(reader);
			while(CurrentNodeNameEquals(reader, "track"))
			{
				Track track;
				track.BoneName = GetAttribute<string>(reader, "bone");

				// Keyframes
				NextNode(reader);
				if (!CurrentNodeNameEquals(reader, "keyframes"))
					throw DeadlyImportError("No <keyframes> node found in a track in animation '" + animation.Name + "' in " + skeletonFile);

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
							if(string("axis")!=reader->getNodeName())
								throw DeadlyImportError("No axis for keyframe rotation!");
								
							aiVector3D axis;
							axis.x = GetAttribute<float>(reader, "x");
							axis.y = GetAttribute<float>(reader, "y");
							axis.z = GetAttribute<float>(reader, "z");

							if (axis.Equal(zeroVec))
							{
								axis.x = 1.0f;
								if (angle != 0)
									DefaultLogger::get()->warn("Found invalid a key frame with a zero rotation axis in animation '" + animation.Name + "'");
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

void OgreImporter::CreateAssimpSkeleton(aiScene *pScene, const std::vector<Bone> &Bones, const std::vector<Animation> &/*Animations*/)
{
	if(!pScene->mRootNode)
		throw DeadlyImportError("No root node exists!!");
	if(0!=pScene->mRootNode->mNumChildren)
		throw DeadlyImportError("Root Node already has childnodes!");


	//Createt the assimp bone hierarchy
	vector<aiNode*> RootBoneNodes;
	BOOST_FOREACH(const Bone &theBone, Bones)
	{
		if(-1==theBone.ParentId) //the bone is a root bone
		{
			//which will recursily add all other nodes
			RootBoneNodes.push_back(CreateAiNodeFromBone(theBone.Id, Bones, pScene->mRootNode));
		}
	}
	
	if(RootBoneNodes.size() > 0)
	{
		pScene->mRootNode->mNumChildren=RootBoneNodes.size();	
		pScene->mRootNode->mChildren=new aiNode*[RootBoneNodes.size()];
		memcpy(pScene->mRootNode->mChildren, &RootBoneNodes[0], sizeof(aiNode*)*RootBoneNodes.size());
	}
}

void OgreImporter::PutAnimationsInScene(aiScene *pScene, const std::vector<Bone> &Bones, const std::vector<Animation> &Animations)
{
	// TODO: Auf nicht vorhandene Animationskeys achten!
	// @todo Pay attention to non-existing animation Keys (google translated from above german comment)

	if(Animations.size()>0)//Maybe the model had only a skeleton and no animations. (If it also has no skeleton, this function would'nt have been called
	{
		pScene->mNumAnimations=Animations.size();
		pScene->mAnimations=new aiAnimation*[Animations.size()];
		for(unsigned int i=0; i<Animations.size(); ++i)//create all animations
		{
			aiAnimation* NewAnimation=new aiAnimation();
			NewAnimation->mName=Animations[i].Name;
			NewAnimation->mDuration=Animations[i].Length;
			NewAnimation->mTicksPerSecond=1.0f;

			//Create all tracks in this animation
			NewAnimation->mNumChannels=Animations[i].Tracks.size();
			NewAnimation->mChannels=new aiNodeAnim*[Animations[i].Tracks.size()];
			for(unsigned int j=0; j<Animations[i].Tracks.size(); ++j)
			{
				aiNodeAnim* NewNodeAnim=new aiNodeAnim();
				NewNodeAnim->mNodeName=Animations[i].Tracks[j].BoneName;

				//we need this, to acces the bones default pose, which we need to make keys absolute to the default bone pose
				vector<Bone>::const_iterator CurBone=find(Bones.begin(), Bones.end(), NewNodeAnim->mNodeName);
				aiMatrix4x4 t0, t1;
				aiMatrix4x4 DefBonePose=aiMatrix4x4::Translation(CurBone->Position, t1)
									 *	aiMatrix4x4::Rotation(CurBone->RotationAngle, CurBone->RotationAxis, t0);
				

				//Create the keyframe arrays...
				unsigned int KeyframeCount=Animations[i].Tracks[j].Keyframes.size();
				NewNodeAnim->mNumPositionKeys=KeyframeCount;
				NewNodeAnim->mNumRotationKeys=KeyframeCount;
				NewNodeAnim->mNumScalingKeys =KeyframeCount;
				NewNodeAnim->mPositionKeys=new aiVectorKey[KeyframeCount];
				NewNodeAnim->mRotationKeys=new aiQuatKey[KeyframeCount];
				NewNodeAnim->mScalingKeys =new aiVectorKey[KeyframeCount];
				
				//...and fill them
				for(unsigned int k=0; k<KeyframeCount; ++k)
				{
					aiMatrix4x4 t2, t3;

					//Create a matrix to transfrom a vector from the bones default pose to the bone bones in this animation key
					aiMatrix4x4 PoseToKey=
									  aiMatrix4x4::Translation(Animations[i].Tracks[j].Keyframes[k].Position, t3)	//pos
									* aiMatrix4x4(Animations[i].Tracks[j].Keyframes[k].Rotation.GetMatrix())		//rot
									* aiMatrix4x4::Scaling(Animations[i].Tracks[j].Keyframes[k].Scaling, t2);		//scale
									

					//calculate the complete transformation from world space to bone space
					aiMatrix4x4 CompleteTransform=DefBonePose * PoseToKey;
					
					aiVector3D Pos;
					aiQuaternion Rot;
					aiVector3D Scale;

					CompleteTransform.Decompose(Scale, Rot, Pos);

					double Time=Animations[i].Tracks[j].Keyframes[k].Time;

					NewNodeAnim->mPositionKeys[k].mTime=Time;
					NewNodeAnim->mPositionKeys[k].mValue=Pos;
					
					NewNodeAnim->mRotationKeys[k].mTime=Time;
					NewNodeAnim->mRotationKeys[k].mValue=Rot;

					NewNodeAnim->mScalingKeys[k].mTime=Time;
					NewNodeAnim->mScalingKeys[k].mValue=Scale;
				}
				
				NewAnimation->mChannels[j]=NewNodeAnim;
			}

			pScene->mAnimations[i]=NewAnimation;
		}
	}
}


aiNode* OgreImporter::CreateAiNodeFromBone(int BoneId, const std::vector<Bone> &Bones, aiNode* ParentNode)
{
	//----Create the node for this bone and set its values-----
	aiNode* NewNode=new aiNode(Bones[BoneId].Name);
	NewNode->mParent=ParentNode;

	aiMatrix4x4 t0,t1;
	NewNode->mTransformation=
		aiMatrix4x4::Translation(Bones[BoneId].Position, t0)
		*aiMatrix4x4::Rotation(Bones[BoneId].RotationAngle, Bones[BoneId].RotationAxis, t1)
	;
	//__________________________________________________________


	//---------- recursivly create all children Nodes: ----------
	NewNode->mNumChildren=Bones[BoneId].Children.size();
	NewNode->mChildren=new aiNode*[Bones[BoneId].Children.size()];
	for(unsigned int i=0; i<Bones[BoneId].Children.size(); ++i)
	{
		NewNode->mChildren[i]=CreateAiNodeFromBone(Bones[BoneId].Children[i], Bones, NewNode);
	}
	//____________________________________________________


	return NewNode;
}


void Bone::CalculateBoneToWorldSpaceMatrix(vector<Bone> &Bones)
{
	aiMatrix4x4 t0, t1;
	aiMatrix4x4 transform = aiMatrix4x4::Rotation(-RotationAngle, RotationAxis, t1) * aiMatrix4x4::Translation(-Position, t0);

	if (!IsParented())
		BoneToWorldSpace = transform;
	else
		BoneToWorldSpace = transform * Bones[ParentId].BoneToWorldSpace;

	// Recursively for all children now that the parent matrix has been calculated.
	BOOST_FOREACH(int childId, Children)
	{
		Bones[childId].CalculateBoneToWorldSpaceMatrix(Bones);
	}
}

} // Ogre
} // Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
