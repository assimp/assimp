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

#include "OgreImporter.hpp"
#include "TinyFormatter.h"

using namespace std;

namespace Assimp
{
namespace Ogre
{



void OgreImporter::LoadSkeleton(std::string FileName, vector<Bone> &Bones, vector<Animation> &Animations) const
{
	const aiScene* const m_CurrentScene=this->m_CurrentScene;//make sure, that we can access but not change the scene
	(void)m_CurrentScene;


	//most likely the skeleton file will only end with .skeleton
	//But this is a xml reader, so we need: .skeleton.xml
	FileName+=".xml";

	DefaultLogger::get()->debug(string("Loading Skeleton: ")+FileName);

	//Open the File:
	boost::scoped_ptr<IOStream> File(m_CurrentIOHandler->Open(FileName));
	if(NULL==File.get())
		throw DeadlyImportError("Failed to open skeleton file "+FileName+".");

	//Read the Mesh File:
	boost::scoped_ptr<CIrrXML_IOStreamReader> mIOWrapper(new CIrrXML_IOStreamReader(File.get()));
	XmlReader* SkeletonFile = irr::io::createIrrXMLReader(mIOWrapper.get());
	if(!SkeletonFile)
		throw DeadlyImportError(string("Failed to create XML Reader for ")+FileName);

	//Quick note: Whoever read this should know this one thing: irrXml fucking sucks!!!

	XmlRead(SkeletonFile);
	if(string("skeleton")!=SkeletonFile->getNodeName())
		throw DeadlyImportError("No <skeleton> node in SkeletonFile: "+FileName);



	//------------------------------------load bones-----------------------------------------
	XmlRead(SkeletonFile);
	if(string("bones")!=SkeletonFile->getNodeName())
		throw DeadlyImportError("No bones node in skeleton "+FileName);

	XmlRead(SkeletonFile);

	while(string("bone")==SkeletonFile->getNodeName())
	{
		//TODO: Maybe we can have bone ids for the errrors, but normaly, they should never appear, so what....

		//read a new bone:
		Bone NewBone;
		NewBone.Id=GetAttribute<int>(SkeletonFile, "id");
		NewBone.Name=GetAttribute<string>(SkeletonFile, "name");

		//load the position:
		XmlRead(SkeletonFile);
		if(string("position")!=SkeletonFile->getNodeName())
			throw DeadlyImportError("Position is not first node in Bone!");
		NewBone.Position.x=GetAttribute<float>(SkeletonFile, "x");
		NewBone.Position.y=GetAttribute<float>(SkeletonFile, "y");
		NewBone.Position.z=GetAttribute<float>(SkeletonFile, "z");

		//Rotation:
		XmlRead(SkeletonFile);
		if(string("rotation")!=SkeletonFile->getNodeName())
			throw DeadlyImportError("Rotation is not the second node in Bone!");
		NewBone.RotationAngle=GetAttribute<float>(SkeletonFile, "angle");
		XmlRead(SkeletonFile);
		if(string("axis")!=SkeletonFile->getNodeName())
			throw DeadlyImportError("No axis specified for bone rotation!");
		NewBone.RotationAxis.x=GetAttribute<float>(SkeletonFile, "x");
		NewBone.RotationAxis.y=GetAttribute<float>(SkeletonFile, "y");
		NewBone.RotationAxis.z=GetAttribute<float>(SkeletonFile, "z");

		//append the newly loaded bone to the bone list
		Bones.push_back(NewBone);

		//Proceed to the next bone:
		XmlRead(SkeletonFile);
	}
	//The bones in the file a not neccesarly ordered by there id's so we do it now:
	std::sort(Bones.begin(), Bones.end());

	//now the id of each bone should be equal to its position in the vector:
	//so we do a simple check:
	{
		bool IdsOk=true;
		for(int i=0; i<static_cast<signed int>(Bones.size()); ++i)//i is signed, because all Id's are also signed!
		{
			if(Bones[i].Id!=i)
				IdsOk=false;
		}
		if(!IdsOk)
			throw DeadlyImportError("Bone Ids are not valid!"+FileName);
	}
	DefaultLogger::get()->debug((Formatter::format(),"Number of bones: ",Bones.size()));
	//________________________________________________________________________________






	//----------------------------load bonehierarchy--------------------------------
	if(string("bonehierarchy")!=SkeletonFile->getNodeName())
		throw DeadlyImportError("no bonehierarchy node in "+FileName);

	DefaultLogger::get()->debug("loading bonehierarchy...");
	XmlRead(SkeletonFile);
	while(string("boneparent")==SkeletonFile->getNodeName())
	{
		string Child, Parent;
		Child=GetAttribute<string>(SkeletonFile, "bone");
		Parent=GetAttribute<string>(SkeletonFile, "parent");

		unsigned int ChildId, ParentId;
		ChildId=find(Bones.begin(), Bones.end(), Child)->Id;
		ParentId=find(Bones.begin(), Bones.end(), Parent)->Id;

		Bones[ChildId].ParentId=ParentId;
		Bones[ParentId].Children.push_back(ChildId);

		XmlRead(SkeletonFile);//i once forget this line, which led to an endless loop, did i mentioned, that irrxml sucks??
	}
	//_____________________________________________________________________________


	//--------- Calculate the WorldToBoneSpace Matrix recursivly for all bones: ------------------
	BOOST_FOREACH(Bone &theBone, Bones)
	{
		if(-1==theBone.ParentId) //the bone is a root bone
		{
			theBone.CalculateBoneToWorldSpaceMatrix(Bones);
		}
	}
	//_______________________________________________________________________
	

	//---------------------------load animations-----------------------------
	if(string("animations")==SkeletonFile->getNodeName())//animations are optional values
	{
		DefaultLogger::get()->debug("Loading Animations");
		XmlRead(SkeletonFile);
		while(string("animation")==SkeletonFile->getNodeName())
		{
			Animation NewAnimation;
			NewAnimation.Name=GetAttribute<string>(SkeletonFile, "name");
			NewAnimation.Length=GetAttribute<float>(SkeletonFile, "length");
			
			//Load all Tracks
			XmlRead(SkeletonFile);
			if(string("tracks")!=SkeletonFile->getNodeName())
				throw DeadlyImportError("no tracks node in animation");
			XmlRead(SkeletonFile);
			while(string("track")==SkeletonFile->getNodeName())
			{
				Track NewTrack;
				NewTrack.BoneName=GetAttribute<string>(SkeletonFile, "bone");

				//Load all keyframes;
				XmlRead(SkeletonFile);
				if(string("keyframes")!=SkeletonFile->getNodeName())
					throw DeadlyImportError("no keyframes node!");
				XmlRead(SkeletonFile);
				while(string("keyframe")==SkeletonFile->getNodeName())
				{
					Keyframe NewKeyframe;
					NewKeyframe.Time=GetAttribute<float>(SkeletonFile, "time");

					//loop over the attributes:
					
					while(true)
					{
						XmlRead(SkeletonFile);

						//If any property doesn't show up, it will keep its initialization value

						//Position:
						if(string("translate")==SkeletonFile->getNodeName())
						{
							NewKeyframe.Position.x=GetAttribute<float>(SkeletonFile, "x");
							NewKeyframe.Position.y=GetAttribute<float>(SkeletonFile, "y");
							NewKeyframe.Position.z=GetAttribute<float>(SkeletonFile, "z");
						}

						//Rotation:
						else if(string("rotate")==SkeletonFile->getNodeName())
						{
							float RotationAngle=GetAttribute<float>(SkeletonFile, "angle");
							aiVector3D RotationAxis;
							XmlRead(SkeletonFile);
							if(string("axis")!=SkeletonFile->getNodeName())
								throw DeadlyImportError("No axis for keyframe rotation!");
							RotationAxis.x=GetAttribute<float>(SkeletonFile, "x");
							RotationAxis.y=GetAttribute<float>(SkeletonFile, "y");
							RotationAxis.z=GetAttribute<float>(SkeletonFile, "z");

							if(0==RotationAxis.x && 0==RotationAxis.y && 0==RotationAxis.z)//we have an invalid rotation axis
							{
								RotationAxis.x=1.0f;
								if(0!=RotationAngle)//if we don't rotate at all, the axis does not matter
								{
									DefaultLogger::get()->warn("Invalid Rotation Axis in Keyframe!");
								}
							}
							NewKeyframe.Rotation=aiQuaternion(RotationAxis, RotationAngle);
						}

						//Scaling:
						else if(string("scale")==SkeletonFile->getNodeName())
						{
							NewKeyframe.Scaling.x=GetAttribute<float>(SkeletonFile, "x");
							NewKeyframe.Scaling.y=GetAttribute<float>(SkeletonFile, "y");
							NewKeyframe.Scaling.z=GetAttribute<float>(SkeletonFile, "z");
						}

						//we suppose, that we read all attributes and this is a new keyframe or the end of the animation
						else
							break;
					}


					NewTrack.Keyframes.push_back(NewKeyframe);
					//XmlRead(SkeletonFile);
				}


				NewAnimation.Tracks.push_back(NewTrack);
			}

			Animations.push_back(NewAnimation);
		}
	}
	//_____________________________________________________________________________

}


void OgreImporter::CreateAssimpSkeleton(const std::vector<Bone> &Bones, const std::vector<Animation> &/*Animations*/)
{
	if(!m_CurrentScene->mRootNode)
		throw DeadlyImportError("No root node exists!!");
	if(0!=m_CurrentScene->mRootNode->mNumChildren)
		throw DeadlyImportError("Root Node already has childnodes!");


	//Createt the assimp bone hierarchy
	vector<aiNode*> RootBoneNodes;
	BOOST_FOREACH(Bone theBone, Bones)
	{
		if(-1==theBone.ParentId) //the bone is a root bone
		{
			//which will recursily add all other nodes
			RootBoneNodes.push_back(CreateAiNodeFromBone(theBone.Id, Bones, m_CurrentScene->mRootNode));
		}
	}
	
	if (RootBoneNodes.size()) {
		m_CurrentScene->mRootNode->mNumChildren=RootBoneNodes.size();	
		m_CurrentScene->mRootNode->mChildren=new aiNode*[RootBoneNodes.size()];
		memcpy(m_CurrentScene->mRootNode->mChildren, &RootBoneNodes[0], sizeof(aiNode*)*RootBoneNodes.size());
	}
}


void OgreImporter::PutAnimationsInScene(const std::vector<Bone> &Bones, const std::vector<Animation> &Animations)
{
	//-----------------Create the Assimp Animations --------------------
	if(Animations.size()>0)//Maybe the model had only a skeleton and no animations. (If it also has no skeleton, this function would'nt have been called
	{
		m_CurrentScene->mNumAnimations=Animations.size();
		m_CurrentScene->mAnimations=new aiAnimation*[Animations.size()];
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
									* aiMatrix4x4::Scaling(Animations[i].Tracks[j].Keyframes[k].Scaling, t2);	//scale
									

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

			m_CurrentScene->mAnimations[i]=NewAnimation;
		}
	}
//TODO: Auf nicht vorhandene Animationskeys achten!
//#pragma warning (s.o.)
	//__________________________________________________________________
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
	//Calculate the matrix for this bone:

	aiMatrix4x4 t0,t1;
	aiMatrix4x4 Transf=	aiMatrix4x4::Rotation(-RotationAngle, RotationAxis, t1)
					*	aiMatrix4x4::Translation(-Position, t0);

	if(-1==ParentId)
	{
		BoneToWorldSpace=Transf;
	}
	else
	{
		BoneToWorldSpace=Transf*Bones[ParentId].BoneToWorldSpace;
	}
	

	//and recursivly for all children:
	BOOST_FOREACH(int theChildren, Children)
	{
		Bones[theChildren].CalculateBoneToWorldSpaceMatrix(Bones);
	}
}


}//namespace Ogre
}//namespace Assimp

#endif  // !! ASSIMP_BUILD_NO_OGRE_IMPORTER
