/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
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

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

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
#include "ScenePreprocessor.h"

using namespace Assimp;

// ---------------------------------------------------------------------------
void ScenePreprocessor::ProcessScene (aiScene* _scene)
{
	scene = _scene;

	// - nothing to do for meshes for the moment
	// - nothing to do for materials for the moment
	// - nothing to do for nodes for the moment
	// - nothing to do for textures for the moment
	// - nothing to do for lights for the moment
	// - nothing to do for cameras for the moment

	// Process all animations
	for (unsigned int i = 0; i < scene->mNumAnimations;++i)
		ProcessAnimation(scene->mAnimations[i]);
}


// ---------------------------------------------------------------------------
void ScenePreprocessor::ProcessAnimation (aiAnimation* anim)
{
	for (unsigned int i = 0; i < anim->mNumChannels;++i)
	{
		aiNodeAnim* channel = anim->mChannels[i];

		/*  Check whether the animation channel has no rotation
		 *  or position tracks. In this case we generate a dummy
		 *  track from the information we have in the transformation
		 *  matrix of the corresponding node.
		 */
		if (!channel->mNumRotationKeys || !channel->mNumPositionKeys || !channel->mNumScalingKeys)
		{
			// Find the node that belongs to this animation
			aiNode* node = scene->mRootNode->FindNode(channel->mNodeName);
			if (node) // ValidateDS will complain later if 'node' is NULL
			{
				// Decompose the transformation matrix of the node
				aiVector3D scaling, position;
				aiQuaternion rotation;

				node->mTransformation.Decompose(scaling, rotation,position);

				// No rotation keys? Generate a dummy track
				if (!channel->mNumRotationKeys)
				{
					channel->mNumRotationKeys = 1;
					channel->mRotationKeys = new aiQuatKey[1];
					aiQuatKey& q = channel->mRotationKeys[0];

					q.mTime  = 0.;
					q.mValue = rotation;

					DefaultLogger::get()->debug("ScenePreprocessor: Dummy rotation track has been generated");
				}

				// No scaling keys? Generate a dummy track
				if (!channel->mNumScalingKeys)
				{
					channel->mNumScalingKeys = 1;
					channel->mScalingKeys = new aiVectorKey[1];
					aiVectorKey& q = channel->mScalingKeys[0];

					q.mTime  = 0.;
					q.mValue = scaling;

					DefaultLogger::get()->debug("ScenePreprocessor: Dummy scaling track has been generated");
				}

				// No position keys? Generate a dummy track
				if (!channel->mNumPositionKeys)
				{
					channel->mNumPositionKeys = 1;
					channel->mPositionKeys = new aiVectorKey[1];
					aiVectorKey& q = channel->mPositionKeys[0];

					q.mTime  = 0.;
					q.mValue = position;

					DefaultLogger::get()->debug("ScenePreprocessor: Dummy position track has been generated");
				}
			}
		}
	}
}