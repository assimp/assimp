//-------------------------------------------------------------------------------
/**
*	This program is distributed under the terms of the GNU Lesser General
*	Public License (LGPL). 
*
*	ASSIMP Viewer Utility
*
*/
//-------------------------------------------------------------------------------

#if (!defined AV_BACKGROUND_H_INCLUDED)
#define AV_BACKGROUND_H_INCLUDED


class CBackgroundPainter
	{
	CBackgroundPainter()
		: 
		pcTexture(NULL),
		clrColor(D3DCOLOR_ARGB(0xFF,100,100,100)),
		eMode(SIMPLE_COLOR),
		piSkyBoxEffect(NULL)
		{}

public:

	// Supported background draw modi
	enum MODE {SIMPLE_COLOR, TEXTURE_2D, TEXTURE_CUBE};

	// Singleton accessors
	static CBackgroundPainter s_cInstance;
	inline static CBackgroundPainter& Instance ()
		{
		return s_cInstance;
		}

	// set the current background color
	// (this removes any textures loaded)
	void SetColor (D3DCOLOR p_clrNew);

	// Setup a cubemap/a 2d texture as background
	void SetCubeMapBG (const char* p_szPath);
	void SetTextureBG (const char* p_szPath);

	// Called by the render loop
	void OnPreRender();
	void OnPostRender();

	// Release any native resources associated with the instance
	void ReleaseNativeResource();

	// Recreate any native resources associated with the instance
	void RecreateNativeResource();

	// Rotate the skybox
	void RotateSB(const aiMatrix4x4* pm);

	// Reset the state of the skybox
	void ResetSB();

	inline MODE GetMode() const
		{
		return this->eMode;
		}

	inline IDirect3DBaseTexture9* GetTexture()
		{
		return this->pcTexture;
		}

	inline ID3DXBaseEffect* GetEffect()
		{
		return this->piSkyBoxEffect;
		}

private:

	void RemoveSBDeps();

	// current background color
	D3DCOLOR clrColor;

	// current background texture
	IDirect3DBaseTexture9* pcTexture;
	ID3DXEffect* piSkyBoxEffect;

	// current background mode
	MODE eMode;

	// path to the texture
	std::string szPath;

	// transformation matrix for the skybox
	aiMatrix4x4 mMatrix;
	};

#endif // !! AV_BACKGROUND_H_INCLUDED