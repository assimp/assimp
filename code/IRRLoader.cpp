/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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
---------------------------------------------------------------------------
*/

/** @file Implementation of the Irr importer class */

#include "AssimpPCH.h"

#include "IRRLoader.h"
#include "ParsingUtils.h"
#include "fast_atof.h"
#include "GenericProperty.h"

#include "SceneCombiner.h"
#include "StandardShapes.h"

using namespace Assimp;


// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
IRRImporter::IRRImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
IRRImporter::~IRRImporter()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool IRRImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler) const
{
	/* NOTE: A simple check for the file extension is not enough
	 * here. Irrmesh and irr are easy, but xml is too generic
	 * and could be collada, too. So we need to open the file and
	 * search for typical tokens.
	 */

	std::string::size_type pos = pFile.find_last_of('.');

	// no file extension - can't read
	if( pos == std::string::npos)
		return false;

	std::string extension = pFile.substr( pos);
	for (std::string::iterator i = extension.begin(); i != extension.end();++i)
		*i = ::tolower(*i);

	if (extension == ".irr")return true;
	else if (extension == ".xml")
	{
		/*  If CanRead() is called to check whether the loader
		 *  supports a specific file extension in general we
		 *  must return true here.
		 */
		if (!pIOHandler)return true;
		const char* tokens[] = {"irr_scene"};
		return SearchFileHeaderForToken(pIOHandler,pFile,tokens,1);
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
void IRRImporter::GenerateGraph(Node* root,aiNode* rootOut ,aiScene* scene,
	BatchLoader& batch,
	std::vector<aiMesh*>&        meshes,
	std::vector<aiNodeAnim*>&    anims,
	std::vector<AttachmentInfo>& attach)
{
	// Setup the name of this node
	rootOut->mName.Set(root->name);

	unsigned int oldMeshSize = (unsigned int)meshes.size();

	// Now determine the type of the node 
	switch (root->type)
	{
	case Node::ANIMMESH:
	case Node::MESH:
		{
			// get the loaded mesh from the scene and add it to
			// the list of all scenes to be attached to the 
			// graph we're currently building
			aiScene* scene = batch.GetImport(root->meshPath);
			if (!scene)
			{
				DefaultLogger::get()->error("IRR: Unable to load external file: " + root->meshPath);
				break;
			}
			attach.push_back(AttachmentInfo(scene,rootOut));

			// now combine the material we've loaded for this mesh
			// with the real meshes we got from the file. As we
			// don't execute any pp-steps on the file, the numbers
			// should be equal. If they are not, we can impossibly
			// do this  ...
			if (root->materials.size() != (unsigned int)scene->mNumMaterials)
			{
				DefaultLogger::get()->warn("IRR: Failed to match imported materials "
					"with the materials found in the IRR scene file");

				break;
			}
			for (unsigned int i = 0; i < scene->mNumMaterials;++i)
			{
				// delete the old material
				delete scene->mMaterials[i];

				std::pair<aiMaterial*, unsigned int>& src = root->materials[i];
				scene->mMaterials[i] = src.first;

				// Process material flags (e.g. lightmapping)
			}
		}
		break;
	
	case Node::LIGHT:
	case Node::CAMERA:

		// We're already finished with lights and cameras
		break;


	case Node::SPHERE:
		{
			// generate the sphere model. Our input parameter to
			// the sphere generation algorithm is the number of
			// subdivisions of each triangle - but here we have
			// the number of poylgons on a specific axis. Just
			// use some limits ...
			unsigned int mul = root->spherePolyCountX*root->spherePolyCountY;
			if      (mul < 100)mul = 2;
			else if (mul < 300)mul = 3;
			else               mul = 4;

			meshes.push_back(StandardShapes::MakeMesh(mul,&StandardShapes::MakeSphere));

			// Adjust scaling
			root->scaling *= root->sphereRadius;
		}
		break;

	case Node::CUBE:
	case Node::SKYBOX:
		{
			// Skyboxes and normal cubes - generate the cube first
			meshes.push_back(StandardShapes::MakeMesh(&StandardShapes::MakeHexahedron));

			// Adjust scaling
			root->scaling *= root->sphereRadius;
		}
		break;

	case Node::TERRAIN:
		{
		}
		break;
	};

	// Check whether we added a mesh. In this case we'll also
	// need to attach it to the node
	if (oldMeshSize != (unsigned int) meshes.size())
	{
		rootOut->mNumMeshes = 1;
		rootOut->mMeshes    = new unsigned int[1];
		rootOut->mMeshes[0] = oldMeshSize;
	}

	// Now compute the final local transformation matrix of the
	// node from the given translation, rotation and scaling values.
	// (the rotation is given in Euler angles, XYZ order)
	aiMatrix4x4 m;
	rootOut->mTransformation = aiMatrix4x4::RotationX(AI_DEG_TO_RAD(root->rotation.x),m)
		* aiMatrix4x4::RotationY(AI_DEG_TO_RAD(root->rotation.y),m)
		* aiMatrix4x4::RotationZ(AI_DEG_TO_RAD(root->rotation.z),m);

	// apply scaling
	aiMatrix4x4& mat = rootOut->mTransformation;
	mat.a1 *= root->scaling.x;
	mat.b1 *= root->scaling.x; 
	mat.c1 *= root->scaling.x;
	mat.a2 *= root->scaling.y; 
	mat.b2 *= root->scaling.y; 
	mat.c2 *= root->scaling.y;
	mat.a3 *= root->scaling.z;
	mat.b3 *= root->scaling.z; 
	mat.c3 *= root->scaling.z;

	// apply translation
	mat.a4 = root->position.x; 
	mat.b4 = root->position.y; 
	mat.c4 = root->position.z;

	// Add all children recursively. First allocate enough storage
	// for them, then call us again
	rootOut->mNumChildren = (unsigned int)root->children.size();
	if (rootOut->mNumChildren)
	{
		rootOut->mChildren = new aiNode*[rootOut->mNumChildren];
		for (unsigned int i = 0; i < rootOut->mNumChildren;++i)
		{
			aiNode* node = rootOut->mChildren[i] =  new aiNode();
			node->mParent = rootOut;
			GenerateGraph(root->children[i],node,scene,batch,meshes,anims,attach);
		}
	}
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void IRRImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::scoped_ptr<IOStream> file( pIOHandler->Open( pFile));

	// Check whether we can read from the file
	if( file.get() == NULL)
		throw new ImportErrorException( "Failed to open IRR file " + pFile + "");

	// Construct the irrXML parser
	CIrrXML_IOStreamReader st(file.get());
	reader = createIrrXMLReader((IFileReadCallBack*) &st);

	// The root node of the scene
	Node* root = new Node(Node::DUMMY);
	root->parent = NULL;

	// Current node parent
	Node* curParent = root;

	// Scenegraph node we're currently working on
	Node* curNode = NULL;

	// List of output cameras
	std::vector<aiCamera*> cameras;

	// List of output lights
	std::vector<aiLight*> lights;

	// Batch loader used to load external models
	BatchLoader batch(pIOHandler);
	
	cameras.reserve(5);
	lights.reserve(5);

	bool inMaterials = false, inAnimator = false;
	unsigned int guessedAnimCnt = 0, guessedMeshCnt = 0;

	// Parse the XML file
	while (reader->read())
	{
		switch (reader->getNodeType())
		{
		case EXN_ELEMENT:
			
			if (!ASSIMP_stricmp(reader->getNodeName(),"node"))
			{
				/*  What we're going to do with the node depends
				 *  on its type:
				 *
				 *  "mesh" - Load a mesh from an external file
				 *  "cube" - Generate a cube 
				 *  "skybox" - Generate a skybox
				 *  "light" - A light source
				 *  "sphere" - Generate a sphere mesh
				 *  "animatedMesh" - Load an animated mesh from an external file
				 *    and join its animation channels with ours.
				 *  "empty" - A dummy node
				 *  "camera" - A camera
				 *
				 *  Each of these nodes can be animated.
				 */
				const char* sz = reader->getAttributeValueSafe("type");
				Node* nd;
				if (!ASSIMP_stricmp(sz,"mesh"))
				{
					nd = new Node(Node::MESH);
				}
				else if (!ASSIMP_stricmp(sz,"cube"))
				{
					nd = new Node(Node::CUBE);
					++guessedMeshCnt;
					// meshes.push_back(StandardShapes::MakeMesh(&StandardShapes::MakeHexahedron));
				}
				else if (!ASSIMP_stricmp(sz,"skybox"))
				{
					nd = new Node(Node::SKYBOX);
					++guessedMeshCnt;
				}
				else if (!ASSIMP_stricmp(sz,"camera"))
				{
					nd = new Node(Node::CAMERA);

					// Setup a temporary name for the camera
					aiCamera* cam = new aiCamera();
					cam->mName.Set( nd->name );
					cameras.push_back(cam);
				}
				else if (!ASSIMP_stricmp(sz,"light"))
				{
					nd = new Node(Node::LIGHT);

					// Setup a temporary name for the light
					aiLight* cam = new aiLight();
					cam->mName.Set( nd->name );
					lights.push_back(cam);
				}
				else if (!ASSIMP_stricmp(sz,"sphere"))
				{
					nd = new Node(Node::SPHERE);
					++guessedMeshCnt;
				}
				else if (!ASSIMP_stricmp(sz,"animatedMesh"))
				{
					nd = new Node(Node::ANIMMESH);
				}
				else if (!ASSIMP_stricmp(sz,"empty"))
				{
					nd = new Node(Node::DUMMY);
				}
				else
				{
					DefaultLogger::get()->warn("IRR: Found unknown node: " + std::string(sz));

					/*  We skip the contents of nodes we don't know.
					 *  We parse the transformation and all animators 
					 *  and skip the rest.
					 */
					nd = new Node(Node::DUMMY);
				}

				/* Attach the newly created node to the scenegraph
				 */
				curNode = nd;
				nd->parent = curParent;
				curParent->children.push_back(nd);
			}
			else if (!ASSIMP_stricmp(reader->getNodeName(),"materials"))
			{
				inMaterials = true;
			}
			else if (!ASSIMP_stricmp(reader->getNodeName(),"animators"))
			{
				inAnimator = true;
			}
			else if (!ASSIMP_stricmp(reader->getNodeName(),"attributes"))
			{
				/*  We should have a valid node here
				 */
				if (!curNode)
				{
					DefaultLogger::get()->error("IRR: Encountered <attributes> element, but "
						"there is no node active");
					continue;
				}

				Animator* curAnim = NULL;

				if (inMaterials && curNode->type == Node::ANIMMESH ||
					curNode->type == Node::MESH )
				{
					/*  This is a material description - parse it!
					 */
					curNode->materials.push_back(std::pair< aiMaterial*, unsigned int > () );
					std::pair< aiMaterial*, unsigned int >& p = curNode->materials.back();

					p.first = ParseMaterial(p.second);
					continue;
				}
				else if (inAnimator)
				{
					/*  This is an animation path - add a new animator
					 *  to the list.
					 */
					curNode->animators.push_back(Animator());
					curAnim = & curNode->animators.back();

					++guessedAnimCnt;
				}

				/*  Parse all elements in the attributes block 
				 *  and process them.
				 */
				while (reader->read())
				{
					if (reader->getNodeType() == EXN_ELEMENT)
					{
						if (!ASSIMP_stricmp(reader->getNodeName(),"vector3d"))
						{
							VectorProperty prop;
							ReadVectorProperty(prop);

							// Convert to our coordinate system
							std::swap( (float&)prop.value.z, (float&)prop.value.y );
							prop.value.y *= -1.f;

							if (inAnimator)
							{
								if (curAnim->type == Animator::ROTATION && prop.name == "Rotation")
								{
									// We store the rotation euler angles in 'direction'
									curAnim->direction = prop.value;
								}
								else if (curAnim->type == Animator::FOLLOW_SPLINE)
								{
									// Check whether the vector follows the PointN naming scheme,
									// here N is the ONE-based index of the point
									if (prop.name.length() >= 6 && prop.name.substr(0,5) == "Point")
									{
										// Add a new key to the list
										curAnim->splineKeys.push_back(aiVectorKey());
										aiVectorKey& key = curAnim->splineKeys.back();

										// and parse its properties
										key.mValue = prop.value;
										key.mTime  = strtol10(&prop.name.c_str()[5]);
									}
								}
								else if (curAnim->type == Animator::FLY_CIRCLE)
								{
									if (prop.name == "Center")
									{
										curAnim->circleCenter = prop.value;
									}
									else if (prop.name == "Direction")
									{
										curAnim->direction = prop.value;
									}
								}
								else if (curAnim->type == Animator::FLY_STRAIGHT)
								{
									if (prop.name == "Start")
									{
										// We reuse the field here
										curAnim->circleCenter = prop.value;
									}
									else if (prop.name == "End")
									{
										// We reuse the field here
										curAnim->direction = prop.value;
									}
								}
							}
							else
							{
								if (prop.name == "Position")
								{
									curNode->position = prop.value;
								}
								else if (prop.name == "Rotation")
								{
									curNode->rotation = prop.value;
								}
								else if (prop.name == "Scale")
								{
									curNode->scaling = prop.value;
								}
								else if (Node::CAMERA == curNode->type)
								{
									aiCamera* cam = cameras.back();
									if (prop.name == "Target")
									{
										cam->mLookAt = prop.value;
									}
									else if (prop.name == "UpVector")
									{
										cam->mUp = prop.value;
									}
								}
							}
						}
						else if (!ASSIMP_stricmp(reader->getNodeName(),"bool"))
						{
							BoolProperty prop;
							ReadBoolProperty(prop);

							if (inAnimator && curAnim->type == Animator::FLY_CIRCLE && prop.name == "Loop")
							{
								curAnim->loop = prop.value;
							}
						}
						else if (!ASSIMP_stricmp(reader->getNodeName(),"float"))
						{
							FloatProperty prop;
							ReadFloatProperty(prop);

							if (inAnimator)
							{
								// The speed property exists for several animators
								if (prop.name == "Speed")
								{
									curAnim->speed = prop.value;
								}
								else if (curAnim->type == Animator::FLY_CIRCLE && prop.name == "Radius")
								{
									curAnim->circleRadius = prop.value;
								}
								else if (curAnim->type == Animator::FOLLOW_SPLINE && prop.name == "Tightness")
								{
									curAnim->tightness = prop.value;
								}
							}
							else
							{
								if (prop.name == "FramesPerSecond" &&
									Node::ANIMMESH == curNode->type)
								{
									curNode->framesPerSecond = prop.value;
								}
								else if (Node::CAMERA == curNode->type)
								{	
									/*  This is the vertical, not the horizontal FOV.
									*  We need to compute the right FOV from the
									*  screen aspect which we don't know yet.
									*/
									if (prop.name == "Fovy")
									{
										cameras.back()->mHorizontalFOV  = prop.value;
									}
									else if (prop.name == "Aspect")
									{
										cameras.back()->mAspect = prop.value;
									}
									else if (prop.name == "ZNear")
									{
										cameras.back()->mClipPlaneNear = prop.value;
									}
									else if (prop.name == "ZFar")
									{
										cameras.back()->mClipPlaneFar = prop.value;
									}
								}
								else if (Node::LIGHT == curNode->type)
								{	
									/*  Additional light information
								 */
									if (prop.name == "Attenuation")
									{
										lights.back()->mAttenuationLinear  = prop.value;
									}
									else if (prop.name == "OuterCone")
									{
										lights.back()->mAngleOuterCone =  AI_DEG_TO_RAD( prop.value );
									}
									else if (prop.name == "InnerCone")
									{
										lights.back()->mAngleInnerCone =  AI_DEG_TO_RAD( prop.value );
									}
								}
								// radius of the sphere to be generated -
								// or alternatively, size of the cube
								else if (Node::SPHERE == curNode->type && prop.name == "Radius" ||
										 Node::CUBE == curNode->type   && prop.name == "Size" )
								{
									curNode->sphereRadius = prop.value;
								}
							}
						}
						else if (!ASSIMP_stricmp(reader->getNodeName(),"int"))
						{
							IntProperty prop;
							ReadIntProperty(prop);

							if (inAnimator)
							{
								if (curAnim->type == Animator::FLY_STRAIGHT && prop.name == "TimeForWay")
								{
									curAnim->timeForWay = prop.value;
								}
							}
							else
							{
								// sphere polgon numbers in each direction
								if (Node::SPHERE == curNode->type)
								{
									if (prop.name == "PolyCountX")
									{
										curNode->spherePolyCountX = prop.value;
									}
									else if (prop.name == "PolyCountY")
									{
										curNode->spherePolyCountY = prop.value;
									}
								}
							}
						}
						else if (!ASSIMP_stricmp(reader->getNodeName(),"string") ||
							!ASSIMP_stricmp(reader->getNodeName(),"enum"))
						{
							StringProperty prop;
							ReadStringProperty(prop);
							if (prop.value.length())
							{
								if (prop.name == "Name")
								{
									curNode->name = prop.value;

									/*  If we're either a camera or a light source
									 *  we need to update the name in the aiLight/
									 *  aiCamera structure, too.
									 */
									if (Node::CAMERA == curNode->type)
									{
										cameras.back()->mName.Set(prop.value);
									}
									else if (Node::LIGHT == curNode->type)
									{
										lights.back()->mName.Set(prop.value);
									}
								}
								else if (Node::LIGHT == curNode->type && "LightType" == prop.name)
								{
								}
								else if (prop.name == "Mesh" && Node::MESH == curNode->type ||
									Node::ANIMMESH == curNode->type)
								{
									/*  This is the file name of the mesh - either
									 *  animated or not. We need to make sure we setup
									 *  the correct postprocessing settings here.
									 */
									unsigned int pp = 0;
									BatchLoader::PropertyMap map;

									/* If the mesh is a static one remove all animations
									 */
									if (Node::ANIMMESH != curNode->type)
									{
										pp |= aiProcess_RemoveComponent;
										SetGenericProperty<int>(map.ints,AI_CONFIG_PP_RVC_FLAGS,
											aiComponent_ANIMATIONS | aiComponent_BONEWEIGHTS);
									}

									batch.AddLoadRequest(prop.value,pp,&map);
									curNode->meshPath = prop.value;
								}
								else if (inAnimator && prop.name == "Type")
								{
									// type of the animator
									if (prop.value == "rotation")
									{
										curAnim->type = Animator::ROTATION;
									}
									else if (prop.value == "flyCircle")
									{
										curAnim->type = Animator::FLY_CIRCLE;
									}
									else if (prop.value == "flyStraight")
									{
										curAnim->type = Animator::FLY_CIRCLE;
									}
									else if (prop.value == "followSpline")
									{
										curAnim->type = Animator::FOLLOW_SPLINE;
									}
									else
									{
										DefaultLogger::get()->warn("IRR: Ignoring unknown animator: "
											+ prop.value);

										curAnim->type = Animator::UNKNOWN;
									}
								}
							}
						}
					}
					else if (reader->getNodeType() == EXN_ELEMENT_END &&
							 !ASSIMP_stricmp(reader->getNodeName(),"attributes"))
					{
						break;
					}
				}
			}
			break;

		case EXN_ELEMENT_END:
		
			// If we reached the end of a node, we need to continue processing its parent
			if (!ASSIMP_stricmp(reader->getNodeName(),"node"))
			{
				if (!curNode)
				{
					// currently is no node set. We need to go
					// back in the node hierarchy
					curParent = curParent->parent;
					if (!curParent)
					{
						curParent = root;
						DefaultLogger::get()->error("IRR: Too many closing <node> elements");
					}
				}
				else curNode = NULL;
			}
			// clear all flags
			else if (!ASSIMP_stricmp(reader->getNodeName(),"materials"))
			{
				inMaterials = false;
			}
			else if (!ASSIMP_stricmp(reader->getNodeName(),"animators"))
			{
				inAnimator = false;
			}
			break;

		default:
			// GCC complains that not all enumeration values are handled
			break;
		}
	}

	/*  Now iterate through all cameras and compute their final (horizontal) FOV
	 */
	for (std::vector<aiCamera*>::iterator it = cameras.begin(), end = cameras.end();
		 it != end; ++it)
	{
		aiCamera* cam = *it;
		if (cam->mAspect) // screen aspect could be missing
		{
			cam->mHorizontalFOV *= cam->mAspect;
		}
		else DefaultLogger::get()->warn("IRR: Camera aspect is not given, can't compute horizontal FOV");
	}

	/* Allocate a tempoary scene data structure
	 */
	aiScene* tempScene = new aiScene();
	tempScene->mRootNode = new aiNode();
	tempScene->mRootNode->mName.Set("<IRRRoot>");

	/* Copy the cameras to the output array
	 */
	tempScene->mNumCameras = (unsigned int)cameras.size();
	tempScene->mCameras = new aiCamera*[tempScene->mNumCameras];
	::memcpy(tempScene->mCameras,&cameras[0],sizeof(void*)*tempScene->mNumCameras);

	/* Copy the light sources to the output array
	 */
	tempScene->mNumLights = (unsigned int)lights.size();
	tempScene->mLights = new aiLight*[tempScene->mNumLights];
	::memcpy(tempScene->mLights,&lights[0],sizeof(void*)*tempScene->mNumLights);

	// temporary data
	std::vector< aiNodeAnim*> anims;
	std::vector< AttachmentInfo > attach;
	std::vector<aiMesh*> meshes;

	anims.reserve(guessedAnimCnt + (guessedAnimCnt >> 2));
	meshes.reserve(guessedMeshCnt + (guessedMeshCnt >> 2));

	/* Now process our scenegraph recursively: generate final
	 * meshes and generate animation channels for all nodes.
	 */
	GenerateGraph(root,tempScene->mRootNode, tempScene,
		batch, meshes, anims, attach);

	if (!anims.empty())
	{
		tempScene->mNumAnimations = 1;
		tempScene->mAnimations = new aiAnimation*[tempScene->mNumAnimations];
		aiAnimation* an = tempScene->mAnimations[0] = new aiAnimation();

		// ***********************************************************
		// This is only the global animation channel of the scene.
		// If there are animated models, they will have separate 
		// animation channels in the scene. To display IRR scenes
		// correctly, users will need to combine the global anim
		// channel with all the local animations they want to play
		// ***********************************************************
		an->mName.Set("Irr_GlobalAnimChannel");

		// copy all node animation channels to the global channel
		an->mNumChannels = (unsigned int)anims.size();
		an->mChannels = new aiNodeAnim*[an->mNumChannels];
		::memcpy(an->mChannels, & anims [0], sizeof(void*)*an->mNumChannels);
	}
	if (meshes.empty())
	{
		// There are no meshes in the scene - the scene is incomplete
		pScene->mFlags |= AI_SCENE_FLAGS_INCOMPLETE;
		DefaultLogger::get()->info("IRR: No Meshes loaded, setting AI_SCENE_FLAGS_INCOMPLETE flag");
	}
	else
	{
		// copy all meshes to the temporary scene
		tempScene->mNumMeshes = (unsigned int)meshes.size();
		tempScene->mMeshes = new aiMesh*[tempScene->mNumMeshes];
		::memcpy(tempScene->mMeshes,&meshes[0],tempScene->mNumMeshes);
	}

	/*  Now merge all sub scenes and attach them to the correct
	 *  attachment points in the scenegraph.
	 */
	SceneCombiner::MergeScenes(pScene,tempScene,attach);


	/* Finished ... everything destructs automatically and all 
	 * temporary scenes have already been deleted by MergeScenes()
	 */
}
