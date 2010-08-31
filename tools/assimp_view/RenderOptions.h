//-------------------------------------------------------------------------------
/**
*	This program is distributed under the terms of the GNU Lesser General
*	Public License (LGPL). 
*
*	ASSIMP Viewer Utility
*
*/
//-------------------------------------------------------------------------------

#if (!defined AV_RO_H_INCLUDED)
#define AV_RO_H_INCLUDED


//-------------------------------------------------------------------------------
/**	\brief Class to manage render options. One global instance
*/
//-------------------------------------------------------------------------------
class RenderOptions
	{
	public:

		// enumerates different drawing modi. POINT is currently
		// not supported and probably will never be.
		enum DrawMode {NORMAL, WIREFRAME, POINT};

		inline RenderOptions	(void) :
			bMultiSample	(true),
			bSuperSample	(false),
			bRenderMats		(true),
			bRenderNormals	(false),
			eDrawMode		(NORMAL),
			b3Lights		(false),
			bLightRotate	(false),
			bRotate			(true),
			bLowQuality		(false),
			bNoSpecular		(false),
			bStereoView		(false),
			bCulling		(false),
			bSkeleton		(false),
			bNoAlphaBlending(false)
			
			{}

		bool bMultiSample;

		// SuperSampling has not yet been implemented
		bool bSuperSample;

		// Display the real material of the object
		bool bRenderMats;

		// Render the normals
		bool bRenderNormals;

		// Use 2 directional light sources
		bool b3Lights;

		// Automatically rotate the light source(s)
		bool bLightRotate;

		// Automatically rotate the asset around its origin
		bool bRotate;

		// use standard lambertian lighting
		bool bLowQuality;

		// disable specular lighting got all elements in the scene
		bool bNoSpecular;

		// enable stereo view
		bool bStereoView;

		bool bNoAlphaBlending;

		// wireframe or solid rendering?
		DrawMode eDrawMode;

		bool bCulling,bSkeleton;
	};

#endif // !! IG