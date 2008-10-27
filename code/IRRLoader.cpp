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

	// List of output meshes
	std::vector<aiMesh*> meshes;

	// List of output animation channels
	std::vector<aiNodeAnim*> animations;

	BatchLoader batch(pIOHandler);
	
	cameras.reserve(5);
	lights.reserve(5);
	animations.reserve(5);
	meshes.reserve(5);

	bool inMaterials = false;

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
					meshes.push_back(StandardShapes::MakeMesh(&StandardShapes::MakeHexahedron));
				}
				else if (!ASSIMP_stricmp(sz,"skybox"))
				{
					nd = new Node(Node::SKYBOX);
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
						else if (!ASSIMP_stricmp(reader->getNodeName(),"float"))
						{
							FloatProperty prop;
							ReadFloatProperty(prop);

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
						}
						else if (!ASSIMP_stricmp(reader->getNodeName(),"string"))
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
								else if (prop.name == "Mesh" && Node::MESH == curNode->type ||
									Node::ANIMMESH == curNode->type)
								{
									/*  This is the file name of the mesh - either
									 *  animated or not. We don't need any postprocessing
									 *  steps here. However, it would be useful it we'd 
									 *  be able to use RemoveVC to remove animations
									 *  if this isn't an animated mesh. But that's not
									 *  possible at the moment.
									 */
									batch.AddLoadRequest(prop.value);
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
			else if (!ASSIMP_stricmp(reader->getNodeName(),"materials"))
			{
				inMaterials = false;
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
}
