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


/** @file IRRLoader.h
 *  @brief Declaration of the .irrMesh (Irrlight Engine Mesh Format)
 *  importer class.
 */
#ifndef AI_IRRLOADER_H_INCLUDED
#define AI_IRRLOADER_H_INCLUDED

#include "IRRShared.h"
#include "SceneCombiner.h"

namespace Assimp	{


// ---------------------------------------------------------------------------
/** Irr importer class.
 *
 * Irr is the native scene file format of the Irrlight engine and its editor
 * irrEdit. As IrrEdit itself is capable of importing quite many file formats,
 * it might be a good file format for data exchange.
 */
class IRRImporter : public BaseImporter, public IrrlichtBase
{
public:
	IRRImporter();
	~IRRImporter();


public:

	// -------------------------------------------------------------------
	/** Returns whether the class can handle the format of the given file. 
	 *  See BaseImporter::CanRead() for details.	
	 */
	bool CanRead( const std::string& pFile, IOSystem* pIOHandler,
		bool checkSig) const;

protected:

	// -------------------------------------------------------------------
	/**
	 */
	const aiImporterDesc* GetInfo () const;

	// -------------------------------------------------------------------
	/** 
	 */
	void InternReadFile( const std::string& pFile, aiScene* pScene, 
		IOSystem* pIOHandler);

	// -------------------------------------------------------------------
	/** 
	*/
	void SetupProperties(const Importer* pImp);

private:

	/** Data structure for a scenegraph node animator
	 */
	struct Animator
	{
		// Type of the animator
		enum AT
		{
			UNKNOWN		  = 0x0,
			ROTATION      = 0x1,
			FLY_CIRCLE    = 0x2,
			FLY_STRAIGHT  = 0x3,
			FOLLOW_SPLINE = 0x4,
			OTHER         = 0x5

		} type;

		Animator(AT t = UNKNOWN)
			: type				(t)
			, speed				(0.001f)
			, direction			(0.f,1.f,0.f)
			, circleRadius		(1.f)
			, tightness			(0.5f)
			, loop				(true)
			, timeForWay		(100)
		{
		}

		
		// common parameters
		float speed;
		aiVector3D direction;

		// FLY_CIRCLE
		aiVector3D circleCenter;
		float circleRadius;

		// FOLLOW_SPLINE
		float tightness;
		std::vector<aiVectorKey> splineKeys;

		// ROTATION (angles given in direction)

		// FLY STRAIGHT
		// circleCenter = start, direction = end
		bool loop;
		int timeForWay;
	};

	/** Data structure for a scenegraph node in an IRR file
	 */
	struct Node
	{
		// Type of the node
		enum ET
		{
			LIGHT,
			CUBE,
			MESH,
			SKYBOX,
			DUMMY,
			CAMERA,
			TERRAIN,
			SPHERE,
			ANIMMESH
		} type;

		Node(ET t)
			:	type				(t)
			,	scaling				(1.f,1.f,1.f) // assume uniform scaling by default
			,	framesPerSecond		(0.f)
			,	sphereRadius		(1.f)
			,	spherePolyCountX	(100)
			,	spherePolyCountY	(100)
		{
		
			// Generate a default name for the node
			char buffer[128];
			static int cnt;
			::sprintf(buffer,"IrrNode_%i",cnt++);
			name = std::string(buffer);

			// reserve space for up to 5 materials
			materials.reserve(5);

			// reserve space for up to 5 children
			children.reserve(5);
		}

		// Transformation of the node
		aiVector3D position, rotation, scaling;

		// Name of the node
		std::string name;

		// List of all child nodes
		std::vector<Node*> children;

		// Parent node
		Node* parent;

		// Animated meshes: frames per second
		// 0.f if not specified
		float framesPerSecond;

		// Meshes: path to the mesh to be loaded
		std::string meshPath;
		unsigned int id;

		// Meshes: List of materials to be assigned
		// along with their corresponding material flags
		std::vector< std::pair<aiMaterial*, unsigned int> > materials;

		// Spheres: radius of the sphere to be generates
		float sphereRadius;

		// Spheres: Number of polygons in the x,y direction
		unsigned int spherePolyCountX,spherePolyCountY;

		// List of all animators assigned to the node
		std::list<Animator> animators;
	};

	/** Data structure for a vertex in an IRR skybox
	 */
	struct SkyboxVertex
	{
		SkyboxVertex()
		{}

		//! Construction from single vertex components
		SkyboxVertex(float px, float py, float pz,
			float nx, float ny, float nz, 
			float uvx, float uvy)

			:	position	(px,py,pz)
			,	normal		(nx,ny,nz)
			,	uv			(uvx,uvy,0.f)
		{}

		aiVector3D position, normal, uv;
	};


	// -------------------------------------------------------------------
	/** Fill the scenegraph recursively
	 */
	void GenerateGraph(Node* root,aiNode* rootOut ,aiScene* scene,
		BatchLoader& batch,
		std::vector<aiMesh*>& meshes,
		std::vector<aiNodeAnim*>& anims,
		std::vector<AttachmentInfo>& attach,
		std::vector<aiMaterial*>& materials,
		unsigned int& defaultMatIdx);


	// -------------------------------------------------------------------
	/** Generate a mesh that consists of just a single quad
	 */
	aiMesh* BuildSingleQuadMesh(const SkyboxVertex& v1,
		const SkyboxVertex& v2,
		const SkyboxVertex& v3,
		const SkyboxVertex& v4);


	// -------------------------------------------------------------------
	/** Build a skybox 
	 *
	 *  @param meshes Receives 6 output meshes
	 *  @param materials The last 6 materials are assigned to the newly
	 *    created meshes. The names of the materials are adjusted.
	 */
	void BuildSkybox(std::vector<aiMesh*>& meshes, 
		std::vector<aiMaterial*> materials);


	// -------------------------------------------------------------------
	/** Copy a material for a mesh to the output material list 
	 *
	 *  @param materials Receives an output material
	 *  @param inmaterials List of input materials
	 *  @param defMatIdx Default material index - UINT_MAX if not present
	 *  @param mesh Mesh to work on
	 */
	void CopyMaterial(std::vector<aiMaterial*>&	 materials,
		std::vector< std::pair<aiMaterial*, unsigned int> >& inmaterials,
		unsigned int& defMatIdx,
		aiMesh* mesh);


	// -------------------------------------------------------------------
	/** Compute animations for a specific node
	 *
	 *  @param root Node to be processed
	 *  @param anims The list of output animations
	 */
	void ComputeAnimations(Node* root, aiNode* real, 
		std::vector<aiNodeAnim*>& anims);


private:

	/** Configuration option: desired output FPS */
	double fps;

	/** Configuration option: speed flag was set? */
	bool configSpeedFlag;
};

} // end of namespace Assimp

#endif // AI_IRRIMPORTER_H_INC
