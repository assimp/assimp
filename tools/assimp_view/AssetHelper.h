//-------------------------------------------------------------------------------
/**
*	This program is distributed under the terms of the GNU Lesser General
*	Public License (LGPL). 
*
*	ASSIMP Viewer Utility
*
*/
//-------------------------------------------------------------------------------

#if (!defined AV_ASSET_HELPER_H_INCLUDED)
#define AV_ASSET_HELPER_H_INCLUDED


//-------------------------------------------------------------------------------
/**	\brief Class to wrap ASSIMP's asset output structures
*/
//-------------------------------------------------------------------------------
class AssetHelper
	{
	public:

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
					piVBNormals			(NULL),
					piDiffuseTexture	(NULL),
					piSpecularTexture	(NULL),
					piAmbientTexture	(NULL),
					piNormalTexture		(NULL),
					piEmissiveTexture	(NULL),
					piOpacityTexture	(NULL),
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

				// index buffer
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

				// material colors
				D3DXVECTOR4 vDiffuseColor;
				D3DXVECTOR4 vSpecularColor;
				D3DXVECTOR4 vAmbientColor;
				D3DXVECTOR4 vEmissiveColor;

				// opacity for the material
				float fOpacity;

				// shininess for the material
				float fShininess;
			};

		// One instance per aiMesh in the globally loaded asset
		MeshHelper** apcMeshes;

		// Scene wrapper instance
		const aiScene* pcScene;
	};

#endif // !! IG