//-------------------------------------------------------------------------------
/**
*	This program is distributed under the terms of the GNU Lesser General
*	Public License (LGPL). 
*
*	ASSIMP Viewer Utility
*
*/
//-------------------------------------------------------------------------------

#if (!defined AV_MAIN_H_INCLUDED)
#define AV_MAIN_H_INCLUDED

// include resource definitions
#include "resource.h"

// Include ASSIMP headers
#include "aiDefs.h"
#include "aiAnim.h"
#include "aiAssert.h"
#include "aiFileIO.h"
#include "aiMaterial.h"
#include "aiPostProcess.h"
#include "aiMesh.h"
#include "aiScene.h"
#include "aiTypes.h"
#include "IOSystem.h"
#include "IOStream.h"
#include "assimp.h"
#include "assimp.hpp"

// in order for std::min and std::max to behave properly
#ifdef min 
#undef min
#endif // min
#ifdef max 
#undef max
#endif // min

// default movement speed 
#define MOVE_SPEED 10.0f

namespace AssimpView {

#include "AssetHelper.h"
#include "Camera.h"
#include "RenderOptions.h"
#include "Shaders.h"
#include "Background.h"
#include "LogDisplay.h"

//-------------------------------------------------------------------------------
// Function prototypes
//-------------------------------------------------------------------------------
	int InitD3D(void);
	int ShutdownD3D(void);
	int CreateDevice (bool p_bMultiSample,bool p_bSuperSample, bool bHW = true);
	int CreateDevice (void);
	int Render (void);
	int ShutdownDevice(void);
	int GetProjectionMatrix (aiMatrix4x4& p_mOut);
	int LoadAsset(void);
	int CreateAssetData(void);
	int DeleteAssetData(void);
	int ScaleAsset(void);
	int DeleteAsset(void);
	int SetupFPSView();
	aiVector3D GetCameraMatrix (aiMatrix4x4& p_mOut);
	int CreateMaterial(AssetHelper::MeshHelper* pcMesh,const aiMesh* pcSource);

	void HandleMouseInputFPS( void );
	void HandleMouseInputLightRotate( void );
	void HandleMouseInputLocal( void );
	void HandleKeyboardInputFPS( void );
	void HandleMouseInputLightIntensityAndColor( void );
	void HandleMouseInputSkyBox( void );


//-------------------------------------------------------------------------------
//
// Dialog procedure for the progress bar window
//
//-------------------------------------------------------------------------------
INT_PTR CALLBACK ProgressMessageProc(HWND hwndDlg,UINT uMsg,
	WPARAM wParam,LPARAM lParam);

//-------------------------------------------------------------------------------
// Main message procedure of the application
//
// The function handles all incoming messages for the main window.
// However, if does not directly process input commands. 
// NOTE: Due to the impossibility to process WM_CHAR messages in dialogs
// properly the code for all hotkeys has been moved to the WndMain
//-------------------------------------------------------------------------------
INT_PTR CALLBACK MessageProc(HWND hwndDlg,UINT uMsg,
	WPARAM wParam,LPARAM lParam);

//-------------------------------------------------------------------------------
//
// Dialog procedure for the about dialog
//
//-------------------------------------------------------------------------------
INT_PTR CALLBACK AboutMessageProc(HWND hwndDlg,UINT uMsg,
	WPARAM wParam,LPARAM lParam);

//-------------------------------------------------------------------------------
// 
// Dialog prcoedure for the help dialog
//
//-------------------------------------------------------------------------------
INT_PTR CALLBACK HelpDialogProc(HWND hwndDlg,UINT uMsg,
	WPARAM wParam,LPARAM lParam);


//-------------------------------------------------------------------------------
// find a valid path to a texture file
//
// Handle 8.3 syntax correctly, search the environment of the
// executable and the asset for a texture with a name very similar to a given one
//-------------------------------------------------------------------------------
int FindValidPath(aiString* p_szString);


//-------------------------------------------------------------------------------
// Delete all resources of a given material
//
// Must be called before CreateMaterial() to prevent memory leaking
//-------------------------------------------------------------------------------
void DeleteMaterial(AssetHelper::MeshHelper* pcIn);


//-------------------------------------------------------------------------------
// Recreate all specular materials depending on the current specularity settings
//
// Diffuse-only materials are ignored.
// Must be called after specular highlights have been toggled
//-------------------------------------------------------------------------------
void UpdateSpecularMaterials();


//-------------------------------------------------------------------------------
// Handle command line parameters
//
// The function loads an asset specified on the command line as first argument
// Other command line parameters are not handled
//-------------------------------------------------------------------------------
void HandleCommandLine(char* p_szCommand);


//-------------------------------------------------------------------------------
// Position of the cursor relative to the 3ds max' like control circle
//-------------------------------------------------------------------------------
enum EClickPos
	{
	EClickPos_Circle,
	EClickPos_CircleVert,
	EClickPos_CircleHor,
	EClickPos_Outside
	};


//-------------------------------------------------------------------------------
// Evil globals
//-------------------------------------------------------------------------------
	extern HINSTANCE g_hInstance				/*= NULL*/;
	extern HWND g_hDlg							/*= NULL*/;
	extern IDirect3D9* g_piD3D					/*= NULL*/;
	extern IDirect3DDevice9* g_piDevice			/*= NULL*/;
	extern double g_fFPS						/*= 0.0f*/;
	extern char g_szFileName[MAX_PATH];
	extern ID3DXEffect* g_piDefaultEffect		/*= NULL*/;
	extern ID3DXEffect* g_piNormalsEffect		/*= NULL*/;
	extern ID3DXEffect* g_piPassThroughEffect	/*= NULL*/;
	extern bool g_bMousePressed					/*= false*/;
	extern bool g_bMousePressedR				/*= false*/;
	extern bool g_bMousePressedM				/*= false*/;
	extern bool g_bMousePressedBoth				/*= false*/;
	extern float g_fElpasedTime					/*= 0.0f*/;
	extern D3DCAPS9 g_sCaps;
	extern bool g_bLoadingFinished				/*= false*/;
	extern HANDLE g_hThreadHandle				/*= NULL*/;
	extern float g_fWheelPos					/*= -10.0f*/;
	extern bool g_bLoadingCanceled				/*= false*/;
	extern IDirect3DTexture9* g_pcTexture		/*= NULL*/;

	extern aiMatrix4x4 g_mWorld;
	extern aiMatrix4x4 g_mWorldRotate;
	extern aiVector3D g_vRotateSpeed			/*= aiVector3D(0.5f,0.5f,0.5f)*/;

	extern aiVector3D g_avLightDirs[1] /* = 
		{	aiVector3D(-0.5f,0.6f,0.2f) ,
			aiVector3D(-0.5f,0.5f,0.5f)} */;


	extern POINT g_mousePos						/*= {0,0};*/;
	extern POINT g_LastmousePos					/*= {0,0}*/;
	extern bool g_bFPSView						/*= false*/;
	extern bool g_bInvert						/*= false*/;
	extern EClickPos g_eClick;
	extern unsigned int g_iCurrentColor			/*= 0*/;

	extern float g_fLightIntensity				/*=0.0f*/;
	extern float g_fLightColor					/*=0.0f*/;

	extern RenderOptions g_sOptions;
	extern Camera g_sCamera;
	extern AssetHelper *g_pcAsset				/*= NULL*/;


	//
	// Contains the mask image for the HUD 
	// (used to determine the position of a click)
	//
	// The size of the image is identical to the size of the main 
	// HUD texture
	//
	extern unsigned char* g_szImageMask			/*= NULL*/;

	
	//
	// Specifies the number of different shaders generated for
	// the current asset. This number is incremented by CreateMaterial()
	// each time a shader isn't found in cache and needs to be created
	//
	extern unsigned int g_iShaderCount			/* = 0 */;
	};

#endif // !! AV_MAIN_H_INCLUDED