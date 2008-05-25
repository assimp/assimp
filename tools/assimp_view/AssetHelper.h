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


#if (!defined AV_ASSET_HELPER_H_INCLUDED)
#define AV_ASSET_HELPER_H_INCLUDED


//-------------------------------------------------------------------------------
/**	\brief Class to wrap ASSIMP's asset output structures
*/
//-------------------------------------------------------------------------------
class AssetHelper
	{
	public:
		enum 
		{
			// the original normal set will be used
			ORIGINAL = 0x0u,

			// a smoothed normal set will be used
			SMOOTH = 0x1u,

			// a hard normal set will be used
			HARD = 0x2u,
		};

		// default constructor
		AssetHelper()
			: iNormalSet(ORIGINAL)
		{}

		//---------------------------------------------------------------
		// default vertex data structure
		// (even if tangents, bitangents or normals aren't
		// required by the shader they will be committed to the GPU)
		//---------------------------------------------------------------
		struct Vertex
			{
			aiVector3D vPosition;
			aiVector3D vNormal;

			D3DCOLOR dColorDiffuse;
			aiVector3D vTangent;
			aiVector3D vBitangent;
			aiVector2D vTextureUV;

			// retrieves the FVF code of the vertex type
			static DWORD GetFVF()
				{
				return D3DFVF_DIFFUSE | D3DFVF_XYZ | D3DFVF_NORMAL |
					D3DFVF_TEX1 | D3DFVF_TEX2 | D3DFVF_TEX3 |
					D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEXCOORDSIZE3(1);
				}
			};

		//---------------------------------------------------------------
		// FVF vertex structure used for normals
		//---------------------------------------------------------------
		struct LineVertex
			{
			aiVector3D vPosition;
			DWORD dColorDiffuse;

			// retrieves the FVF code of the vertex type
			static DWORD GetFVF()
				{
				return D3DFVF_DIFFUSE | D3DFVF_XYZ;
				}
			};

		//---------------------------------------------------------------
		// Helper class to store GPU related resources created for
		// a given aiMesh
		//---------------------------------------------------------------
		class MeshHelper
			{
			public:

				MeshHelper ()
					: 
					piVB				(NULL),
					piIB				(NULL),
					piEffect			(NULL),
					piVBNormals			(NULL),
					piDiffuseTexture	(NULL),
					piSpecularTexture	(NULL),
					piAmbientTexture	(NULL),
					piNormalTexture		(NULL),
					piEmissiveTexture	(NULL),
					piOpacityTexture	(NULL),
					piShininessTexture	(NULL),
					pvOriginalNormals	(NULL),
					bSharedFX(false) {}

				~MeshHelper ()
					{
					// NOTE: This is done in DeleteAssetData()
					// TODO: Make this a proper d'tor
					}

				// shading mode to use. Either Lambert or otherwise phong
				// will be used in every case
				aiShadingMode eShadingMode;

				// vertex buffer
				IDirect3DVertexBuffer9* piVB;

				// index buffer. For partially transparent meshes
				// created with dynamic usage to be able to update
				// the buffer contents quickly
				IDirect3DIndexBuffer9* piIB;

				// vertex buffer to be used to draw vertex normals
				// (vertex normals are generated in every case)
				IDirect3DVertexBuffer9* piVBNormals;

				// shader to be used
				ID3DXEffect* piEffect;
				bool bSharedFX;

				// material textures
				IDirect3DTexture9* piDiffuseTexture;
				IDirect3DTexture9* piSpecularTexture;
				IDirect3DTexture9* piAmbientTexture;
				IDirect3DTexture9* piEmissiveTexture;
				IDirect3DTexture9* piNormalTexture;
				IDirect3DTexture9* piOpacityTexture;
				IDirect3DTexture9* piShininessTexture;

				// material colors
				D3DXVECTOR4 vDiffuseColor;
				D3DXVECTOR4 vSpecularColor;
				D3DXVECTOR4 vAmbientColor;
				D3DXVECTOR4 vEmissiveColor;

				// opacity for the material
				float fOpacity;

				// shininess for the material
				float fShininess;

				// strength of the specular highlight
				float fSpecularStrength;

				// Stores a pointer to the original normal set of the asset
				aiVector3D* pvOriginalNormals;
			};

		// One instance per aiMesh in the globally loaded asset
		MeshHelper** apcMeshes;

		// Scene wrapper instance
		aiScene* pcScene;

		// Specifies the normal set to be used
		unsigned int iNormalSet;

		// ------------------------------------------------------------------
		// set the normal set to be used
		void SetNormalSet(unsigned int iSet);

		// ------------------------------------------------------------------
		// flip all normal vectors
		void FlipNormals();
	};

#endif // !! IG