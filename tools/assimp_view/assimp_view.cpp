
//-------------------------------------------------------------------------------
/**
 *	This program is distributed under the terms of the GNU Lesser General
 *	Public License (LGPL). 
 *
 *	ASSIMP Viewer Utility
 *
 */
//-------------------------------------------------------------------------------

#include "stdafx.h"
#include "assimp_view.h"


namespace AssimpView {


//-------------------------------------------------------------------------------
// evil globals
//-------------------------------------------------------------------------------
HINSTANCE g_hInstance				= NULL;
HWND g_hDlg							= NULL;
IDirect3D9* g_piD3D					= NULL;
IDirect3DDevice9* g_piDevice		= NULL;
double g_fFPS						= 0.0f;
char g_szFileName[MAX_PATH];
ID3DXEffect* g_piDefaultEffect		= NULL;
ID3DXEffect* g_piNormalsEffect		= NULL;
ID3DXEffect* g_piPassThroughEffect	= NULL;
bool g_bMousePressed				= false;
bool g_bMousePressedR				= false;
bool g_bMousePressedM				= false;
bool g_bMousePressedBoth			= false;
float g_fElpasedTime				= 0.0f;
D3DCAPS9 g_sCaps;
bool g_bLoadingFinished				= false;
HANDLE g_hThreadHandle				= NULL;
float g_fWheelPos					= -10.0f;
bool g_bLoadingCanceled				= false;
IDirect3DTexture9* g_pcTexture		= NULL;

aiMatrix4x4 g_mWorld;
aiMatrix4x4 g_mWorldRotate;
aiVector3D g_vRotateSpeed			= aiVector3D(0.5f,0.5f,0.5f);

aiVector3D g_avLightDirs[1] = {	aiVector3D(-0.5f,0.6f,0.2f) /*,
								aiVector3D(-0.5f,0.5f,0.5f)*/};
POINT g_mousePos;
POINT g_LastmousePos;
bool g_bFPSView						= false;
bool g_bInvert						= false;
EClickPos g_eClick					= EClickPos_Circle;
unsigned int g_iCurrentColor		= 0;

float g_fLightIntensity				= 1.0f;
float g_fLightColor					= 1.0f;

RenderOptions g_sOptions;
Camera g_sCamera;
AssetHelper *g_pcAsset				= NULL;

//
// Contains the mask image for the HUD 
// (used to determine the position of a click)
//
unsigned char* g_szImageMask		= NULL;

//-------------------------------------------------------------------------------
// Table of colors used for normal vectors. 
//-------------------------------------------------------------------------------
D3DXVECTOR4 g_aclNormalColors[14] = 
	{
	D3DXVECTOR4(0xFF / 255.0f,0xFF / 255.0f,0xFF / 255.0f, 1.0f), // white

	D3DXVECTOR4(0xFF / 255.0f,0x00 / 255.0f,0x00 / 255.0f,1.0f), // red
	D3DXVECTOR4(0x00 / 255.0f,0xFF / 255.0f,0x00 / 255.0f,1.0f), // green
	D3DXVECTOR4(0x00 / 255.0f,0x00 / 255.0f,0xFF / 255.0f,1.0f), // blue

	D3DXVECTOR4(0xFF / 255.0f,0xFF / 255.0f,0x00 / 255.0f,1.0f), // yellow
	D3DXVECTOR4(0xFF / 255.0f,0x00 / 255.0f,0xFF / 255.0f,1.0f), // magenta
	D3DXVECTOR4(0x00 / 255.0f,0xFF / 255.0f,0xFF / 255.0f,1.0f), // wtf

	D3DXVECTOR4(0xFF / 255.0f,0x60 / 255.0f,0x60 / 255.0f,1.0f), // light red
	D3DXVECTOR4(0x60 / 255.0f,0xFF / 255.0f,0x60 / 255.0f,1.0f), // light green
	D3DXVECTOR4(0x60 / 255.0f,0x60 / 255.0f,0xFF / 255.0f,1.0f), // light blue

	D3DXVECTOR4(0xA0 / 255.0f,0x00 / 255.0f,0x00 / 255.0f,1.0f), // dark red
	D3DXVECTOR4(0x00 / 255.0f,0xA0 / 255.0f,0x00 / 255.0f,1.0f), // dark green
	D3DXVECTOR4(0x00 / 255.0f,0x00 / 255.0f,0xA0 / 255.0f,1.0f), // dark blue

	D3DXVECTOR4(0x88 / 255.0f,0x88 / 255.0f,0x88 / 255.0f, 1.0f) // gray
	};


//-------------------------------------------------------------------------------
// Entry point for the loader thread
// The laoder thread loads the asset while the progress dialog displays the
// smart progress bar
//-------------------------------------------------------------------------------
DWORD WINAPI LoadThreadProc(LPVOID lpParameter)
	{
	UNREFERENCED_PARAMETER(lpParameter);

	// get current time
	double fCur = (double)timeGetTime();

	// call ASSIMPs C-API to load the file
	g_pcAsset->pcScene = aiImportFile(g_szFileName,
		aiProcess_CalcTangentSpace		| // calculate tangents and bitangents
		aiProcess_JoinIdenticalVertices | // join identical vertices
		aiProcess_Triangulate			| // triangulate n-polygons
		aiProcess_GenSmoothNormals		| // generate smooth normal vectors if not existing
		aiProcess_ConvertToLeftHanded	| // convert everything to D3D left handed space
		aiProcess_SplitLargeMeshes);      // split large, unrenderable meshes into submeshes

	// get the end time of zje operation, calculate delta t
	double fEnd = (double)timeGetTime();
	double dTime = (fEnd - fCur) / 1000;
	char szTemp[128];
	sprintf(szTemp,"%.5f",(float)dTime);
	SetDlgItemText(g_hDlg,IDC_ELOAD,szTemp);
	g_bLoadingFinished = true;

	// check whether the loading process has failed ...
	if (NULL == g_pcAsset->pcScene)
		{
		CLogDisplay::Instance().AddEntry("[ERROR] Unable to load this asset:",
			D3DCOLOR_ARGB(0xFF,0xFF,0,0));

		// print ASSIMPs error string to the log display
		CLogDisplay::Instance().AddEntry(aiGetErrorString(),
			D3DCOLOR_ARGB(0xFF,0xFF,0,0));
		return 1;
		}
	return 0;
	}

//-------------------------------------------------------------------------------
// Recursivly count the number of nodes in an asset's node graph
// Used by LoadAsset()
//-------------------------------------------------------------------------------
void GetNodeCount(aiNode* pcNode, unsigned int* piCnt)
{
	*piCnt = *piCnt+1;
	for (unsigned int i = 0; i < pcNode->mNumChildren;++i)
		GetNodeCount(pcNode->mChildren[i],piCnt);
}

//-------------------------------------------------------------------------------
// load the current asset
// THe path to the asset is specified in the global path variable
//-------------------------------------------------------------------------------
int LoadAsset(void)
{
	// set the world and world rotation matrices to the identuty
	g_mWorldRotate = aiMatrix4x4();
	g_mWorld = aiMatrix4x4();

	// create a helper thread to load the asset
	DWORD dwID;
	g_bLoadingCanceled = false;
	g_pcAsset = new AssetHelper();
	g_hThreadHandle = CreateThread(NULL,0,&LoadThreadProc,NULL,0,&dwID);

	if (!g_hThreadHandle)
	{
		CLogDisplay::Instance().AddEntry(
			"[ERROR] Unable to create helper thread for loading",
			D3DCOLOR_ARGB(0xFF,0xFF,0,0));
		return 0;
	}

	// show the progress bar dialog
	DialogBox(g_hInstance,MAKEINTRESOURCE(IDD_LOADDIALOG),
		g_hDlg,&ProgressMessageProc);

	// now we should have loaded the asset. Check this ...
	g_bLoadingFinished = false;
	if (!g_pcAsset || !g_pcAsset->pcScene)
	{
		if (g_pcAsset)
		{
			delete g_pcAsset;
			g_pcAsset = NULL;
		}
		return 0;
	}

	// allocate a new MeshHelper array and build a new instance
	// for each mesh in the original asset
	g_pcAsset->apcMeshes = new AssetHelper::MeshHelper*[
		g_pcAsset->pcScene->mNumMeshes]();

	// get the number of vertices/faces in the model
	unsigned int iNumVert = 0;
	unsigned int iNumFaces = 0;
	for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumMeshes;++i)
	{
		iNumVert += g_pcAsset->pcScene->mMeshes[i]->mNumVertices;
		iNumFaces += g_pcAsset->pcScene->mMeshes[i]->mNumFaces;
		g_pcAsset->apcMeshes[i] = new AssetHelper::MeshHelper();
	}
	// and fill the statistic edit controls
	char szOut[1024];
	sprintf(szOut,"%i",(int)iNumVert);
	SetDlgItemText(g_hDlg,IDC_EVERT,szOut);
	sprintf(szOut,"%i",(int)iNumFaces);
	SetDlgItemText(g_hDlg,IDC_EFACE,szOut);
	sprintf(szOut,"%i",(int)g_pcAsset->pcScene->mNumMaterials);
	SetDlgItemText(g_hDlg,IDC_EMAT,szOut);

	// need to get the number of nodes
	iNumVert = 0;
	GetNodeCount(g_pcAsset->pcScene->mRootNode,&iNumVert);
	sprintf(szOut,"%i",(int)iNumVert);
	SetDlgItemText(g_hDlg,IDC_ENODE,szOut);

	// build a new caption string for the viewer
	sprintf(szOut,AI_VIEW_CAPTION_BASE " [%s]",g_szFileName);
	SetWindowText(g_hDlg,szOut);

	// scale the asset vertices to fit into the viewer window
	ScaleAsset();

	// reset the camera view to the default position
	g_sCamera.vPos = aiVector3D(0.0f,0.0f,-10.0f);
	g_sCamera.vLookAt = aiVector3D(0.0f,0.0f,1.0f);
	g_sCamera.vUp = aiVector3D(0.0f,1.0f,0.0f);
	g_sCamera.vRight = aiVector3D(0.0f,1.0f,0.0f);

	// build native D3D vertex/index buffers, textures, materials
	return CreateAssetData();
}


//-------------------------------------------------------------------------------
// Delete the loaded asset
//-------------------------------------------------------------------------------
int DeleteAsset(void)
	{
	if (!g_pcAsset)return 0;

	// don't anymore know why this was necessary ...
	Render();

	// delete everything
	DeleteAssetData();
	for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumMeshes;++i)
		{
		delete g_pcAsset->apcMeshes[i];
		}
	aiReleaseImport(g_pcAsset->pcScene);
	delete[] g_pcAsset->apcMeshes;
	delete g_pcAsset;
	g_pcAsset = NULL;

	// clear all stats edit controls
	SetDlgItemText(g_hDlg,IDC_EVERT,"0");
	SetDlgItemText(g_hDlg,IDC_EFACE,"0");
	SetDlgItemText(g_hDlg,IDC_EMAT,"0");
	SetDlgItemText(g_hDlg,IDC_ENODE,"0");
	SetDlgItemText(g_hDlg,IDC_ESHADER,"0");
	SetDlgItemText(g_hDlg,IDC_ETEX,"0");

	// reset the caption of the viewer window
	SetWindowText(g_hDlg,AI_VIEW_CAPTION_BASE);
	return 1;
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int CalculateBounds(aiNode* piNode, aiVector3D* p_avOut, 
	const aiMatrix4x4& piMatrix)
	{
	aiMatrix4x4 mTemp = piNode->mTransformation;
	mTemp.Transpose();
	aiMatrix4x4 aiMe = mTemp * piMatrix;

	for (unsigned int i = 0; i < piNode->mNumMeshes;++i)
		{
		for( unsigned int a = 0; a < g_pcAsset->pcScene->mMeshes[
			piNode->mMeshes[i]]->mNumVertices;++a)
			{
			aiVector3D pc =g_pcAsset->pcScene->mMeshes[
				piNode->mMeshes[i]]->mVertices[a];

			aiVector3D pc1;
			D3DXVec3TransformCoord((D3DXVECTOR3*)&pc1,(D3DXVECTOR3*)&pc,
				(D3DXMATRIX*)&aiMe);

			p_avOut[0].x = std::min( p_avOut[0].x, pc1.x);
			p_avOut[0].y = std::min( p_avOut[0].y, pc1.y);
			p_avOut[0].z = std::min( p_avOut[0].z, pc1.z);
			p_avOut[1].x = std::max( p_avOut[1].x, pc1.x);
			p_avOut[1].y = std::max( p_avOut[1].y, pc1.y);
			p_avOut[1].z = std::max( p_avOut[1].z, pc1.z);
			}
		}
	for (unsigned int i = 0; i < piNode->mNumChildren;++i)
		{
		CalculateBounds( piNode->mChildren[i], p_avOut, aiMe );
		}
	return 1;
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int ScaleAsset(void)
	{
	aiVector3D aiVecs[2] = {aiVector3D( 1e10f, 1e10f, 1e10f),
		aiVector3D( -1e10f, -1e10f, -1e10f) };

	if (g_pcAsset->pcScene->mRootNode)
		{
		aiMatrix4x4 m;
		CalculateBounds(g_pcAsset->pcScene->mRootNode,aiVecs,m);
		}
	
	aiVector3D vDelta = aiVecs[1]-aiVecs[0];
	aiVector3D vHalf =  aiVecs[0] + (vDelta / 2.0f);
	float fScale = 10.0f / vDelta.Length();
	
	g_mWorld =  aiMatrix4x4(
			1.0f,0.0f,0.0f,0.0f,
			0.0f,1.0f,0.0f,0.0f,
			0.0f,0.0f,1.0f,0.0f,
			-vHalf.x,-vHalf.y,-vHalf.z,1.0f) *
		aiMatrix4x4(
			fScale,0.0f,0.0f,0.0f,
			0.0f,fScale,0.0f,0.0f,
			0.0f,0.0f,fScale,0.0f,
			0.0f,0.0f,0.0f,1.0f);
#if 0
	// now handle the fact that the asset might have its 
	// own transformation matrix (handle scaling and translation)
	if (NULL != g_pcAsset->pcScene->mRootNode)
		{
		if (0.0f != g_pcAsset->pcScene->mRootNode->mTransformation[0][0] &&
			0.0f != g_pcAsset->pcScene->mRootNode->mTransformation[1][1] &&
			0.0f != g_pcAsset->pcScene->mRootNode->mTransformation[2][2] &&
			0.0f != g_pcAsset->pcScene->mRootNode->mTransformation[3][3])
			{
			g_mWorld[0][0] /= g_pcAsset->pcScene->mRootNode->mTransformation[0][0];
			g_mWorld[1][1] /= g_pcAsset->pcScene->mRootNode->mTransformation[1][1];
			g_mWorld[2][2] /= g_pcAsset->pcScene->mRootNode->mTransformation[2][2];
			g_mWorld[3][3] /= g_pcAsset->pcScene->mRootNode->mTransformation[3][3];
			}
		g_mWorld[3][0] -= g_pcAsset->pcScene->mRootNode->mTransformation[3][0];
		g_mWorld[3][1] -= g_pcAsset->pcScene->mRootNode->mTransformation[3][1];
		g_mWorld[3][2] -= g_pcAsset->pcScene->mRootNode->mTransformation[3][2];
		
		aiMatrix4x4 m;
		if ( 0 == memcmp(&m,&g_pcAsset->pcScene->mRootNode->mTransformation,sizeof(aiMatrix4x4)) &&
			 1 <= g_pcAsset->pcScene->mRootNode->mNumChildren)
			{
			if (0.0f != g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[0][0] &&
				0.0f != g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[1][1] &&
				0.0f != g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[2][2] &&
				0.0f != g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[3][3])
				{
				g_mWorld[0][0] /= g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[0][0];
				g_mWorld[1][1] /= g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[1][1];
				g_mWorld[2][2] /= g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[2][2];
				g_mWorld[3][3] /= g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[3][3];
				}
			g_mWorld[3][0] -= g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[3][0];
			g_mWorld[3][1] -= g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[3][1];
			g_mWorld[3][2] -= g_pcAsset->pcScene->mRootNode->mChildren[0]->mTransformation[3][2];
			}
		}
#endif
	return 1;
	}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int GenerateNormalsAsLineList(AssetHelper::MeshHelper* pcMesh,const aiMesh* pcSource)
	{
	if (!pcSource->mNormals)return 0;

	// create vertex buffer
	if(FAILED( g_piDevice->CreateVertexBuffer(sizeof(AssetHelper::LineVertex) *
		pcSource->mNumVertices * 2,
		D3DUSAGE_WRITEONLY,
		AssetHelper::LineVertex::GetFVF(),
		D3DPOOL_DEFAULT, &pcMesh->piVBNormals,NULL)))
		{
		MessageBox(g_hDlg,"Failed to create vertex buffer for the normal list",
			"ASSIMP Viewer Utility",MB_OK);
		return 2;
		}

	// now fill the vertex buffer
	AssetHelper::LineVertex* pbData2;
	pcMesh->piVBNormals->Lock(0,0,(void**)&pbData2,0);
	for (unsigned int x = 0; x < pcSource->mNumVertices;++x)
		{
		pbData2->vPosition = pcSource->mVertices[x];

		++pbData2;

		aiVector3D vNormal = pcSource->mNormals[x];
		vNormal.Normalize();

		vNormal.x /= g_mWorld.a1*4;
		vNormal.y /= g_mWorld.b2*4;
		vNormal.z /= g_mWorld.c3*4;

		pbData2->vPosition = pcSource->mVertices[x] + vNormal;
			
		++pbData2;
		}
	pcMesh->piVBNormals->Unlock();
	return 1;
	}


//-------------------------------------------------------------------------------
// Fill the UI combobox with a list of all supported animations
//
// The animations are added in order
//-------------------------------------------------------------------------------
int FillAnimList(void)
{
	// clear the combo box
	SendDlgItemMessage(g_hDlg,IDC_COMBO1,CB_RESETCONTENT,0,0);

	if (0 == g_pcAsset->pcScene->mNumAnimations)
	{
		// disable all UI components related to animations
		EnableWindow(GetDlgItem(g_hDlg,IDC_PLAYANIM),FALSE);
		EnableWindow(GetDlgItem(g_hDlg,IDC_SPEED),FALSE);
		EnableWindow(GetDlgItem(g_hDlg,IDC_PINORDER),FALSE);

		EnableWindow(GetDlgItem(g_hDlg,IDC_SSPEED),FALSE);
		EnableWindow(GetDlgItem(g_hDlg,IDC_SANIMGB),FALSE);
		EnableWindow(GetDlgItem(g_hDlg,IDC_SANIM),FALSE);
		EnableWindow(GetDlgItem(g_hDlg,IDC_COMBO1),FALSE);
		}
	else
	{
		// reenable all animation components if they have been
		// disabled for a previous mesh
		EnableWindow(GetDlgItem(g_hDlg,IDC_PLAYANIM),TRUE);
		EnableWindow(GetDlgItem(g_hDlg,IDC_SPEED),TRUE);
		EnableWindow(GetDlgItem(g_hDlg,IDC_PINORDER),TRUE);

		EnableWindow(GetDlgItem(g_hDlg,IDC_SSPEED),TRUE);
		EnableWindow(GetDlgItem(g_hDlg,IDC_SANIMGB),TRUE);
		EnableWindow(GetDlgItem(g_hDlg,IDC_SANIM),TRUE);
		EnableWindow(GetDlgItem(g_hDlg,IDC_COMBO1),TRUE);

		// now fill in all animation names
		for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumAnimations;++i)
		{
			SendDlgItemMessage(g_hDlg,IDC_COMBO1,CB_ADDSTRING,0,
				( LPARAM ) g_pcAsset->pcScene->mAnimations[i]->mName.data);
		}
	}
	return 1;
}


//-------------------------------------------------------------------------------
// Add a node to the display list
// Recusrivly add all subnodes
// iNode - Index of the node image in the tree view's image lust
// iIndex - Index of the node in the parent's child list
// iDepth - Current depth of the node
// pcNode - Node object
// hRoot - Parent tree view node
//-------------------------------------------------------------------------------
int AddNodeToDisplayList(unsigned int iNode,
	unsigned int iIndex, 
	unsigned int iDepth,
	const aiNode* pcNode,
	HTREEITEM hRoot)
{
	ai_assert(NULL != pcNode);

	char chTemp[512];

	if(0 == pcNode->mName.length)
	{
		if (iNode >= 100)
		{
			iNode += iDepth  * 1000;
		}
		else if (iNode >= 10)
		{
			iNode += iDepth  * 100;
		}
		else iNode += iDepth  * 10;
		sprintf(chTemp,"Node %i",iNode);
	}
	else strcpy(chTemp,pcNode->mName.data);

	TVITEMEX tvi; 
	TVINSERTSTRUCT sNew;
	tvi.pszText = chTemp;
	tvi.cchTextMax = (int)strlen(chTemp);
	tvi.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_IMAGE | TVIF_HANDLE;
	tvi.iImage = iNode;
	tvi.iSelectedImage = iNode;
	tvi.lParam = (LPARAM)0; 

	sNew.itemex = tvi; 
	sNew.hInsertAfter = TVI_LAST; 
	sNew.hParent = hRoot;

	// add the item to the list
	HTREEITEM hTexture = (HTREEITEM)SendMessage(GetDlgItem(g_hDlg,IDC_TREE1), 
		TVM_INSERTITEM, 
		0,
		(LPARAM)(LPTVINSERTSTRUCT)&sNew);

	// recursively add all child nodes
	++iDepth;
	for (unsigned int i = 0; i< pcNode->mNumChildren;++i)
	{
		AddNodeToDisplayList(iNode,i,iDepth,pcNode->mChildren[i],hTexture);
	}
	return 1;
}


//-------------------------------------------------------------------------------
// Add a texture to the display list
// pcMat - material containing the texture
// hTexture - Handle to the material tree item
// iTexture - Index of the texture image in the image list of the tree view
// szPath - Path to the texture
// iUVIndex - UV index to be used for the texture
// fBlendFactor - Blend factor to be used for the texture
// eTextureOp - texture operation to be used for the texture
//-------------------------------------------------------------------------------
int AddTextureToDisplayList(unsigned int iType,
	unsigned int iIndex,
	const aiString* szPath,
	HTREEITEM hFX, 
	const aiMaterial* pcMat,
	unsigned int iTexture = 0,
	unsigned int iUVIndex = 0,
	const float fBlendFactor = 0.0f,
	aiTextureOp eTextureOp = aiTextureOp_Multiply)
{
	char chTemp[512];
	const char* sz = strrchr(szPath->data,'\\');
	if (!sz)sz = strrchr(szPath->data,'/');
	if (!sz)sz = szPath->data;

	const char* szType;
	switch (iType)
	{
	case AI_TEXTYPE_DIFFUSE:
		szType = "Diffuse";break;
	case AI_TEXTYPE_SPECULAR:
		szType = "Specular";break;
	case AI_TEXTYPE_AMBIENT:
		szType = "Ambient";break;
	case AI_TEXTYPE_EMISSIVE:
		szType = "Emissive";break;
	case AI_TEXTYPE_HEIGHT:
		szType = "HeightMap";break;
	case AI_TEXTYPE_NORMALS:
		szType = "NormalMap";break;
	case AI_TEXTYPE_SHININESS:
		szType = "Shininess";break;
	};
	sprintf(chTemp,"%s %i (%s)",szType,iIndex+1,sz);

	TVITEMEX tvi; 
	TVINSERTSTRUCT sNew;
	tvi.pszText = chTemp;
	tvi.cchTextMax = (int)strlen(chTemp);
	tvi.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_IMAGE | TVIF_HANDLE;
	tvi.iImage = iTexture;
	tvi.iSelectedImage = iTexture;
	tvi.lParam = (LPARAM)0; 

	sNew.itemex = tvi; 
	sNew.hInsertAfter = TVI_LAST; 
	sNew.hParent = hFX;

	// add the item to the list
	HTREEITEM hTexture = (HTREEITEM)SendMessage(GetDlgItem(g_hDlg,IDC_TREE1), 
		TVM_INSERTITEM, 
		0,
		(LPARAM)(LPTVINSERTSTRUCT)&sNew);
	return 1;
}


//-------------------------------------------------------------------------------
// Add a material and all sub textures to the display mode list
// pcMat - material to be added
// hRoot - Handle to the root of the tree view
// iFX - Index of the material image in the image list of the tree view
// iTexture - Index of the texture image in the image list of the tree view
// iIndex - Material index
//-------------------------------------------------------------------------------
int AddMaterialToDisplayList(HTREEITEM hRoot, const aiMaterial* pcMat,
	unsigned int iFX, unsigned int iTexture, unsigned int iIndex)
{
	// use the name of the material, if possible
	char chTemp[512];
	aiString szOut;
	if (AI_SUCCESS != aiGetMaterialString(pcMat,AI_MATKEY_NAME,&szOut))
	{
		sprintf(chTemp,"Material %i",iIndex+1);
	}
	else
	{
		sprintf(chTemp,"%s (%i)",szOut.data,iIndex+1);
	}
	TVITEMEX tvi; 
	TVINSERTSTRUCT sNew;
	tvi.pszText = chTemp;
	tvi.cchTextMax = (int)strlen(chTemp);
	tvi.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_IMAGE | TVIF_HANDLE | TVIF_STATE;
	tvi.iImage = iFX;
	tvi.iSelectedImage = iFX;
	tvi.lParam = (LPARAM)0; 
	tvi.state = TVIS_EXPANDED | TVIS_EXPANDEDONCE ;

	sNew.itemex = tvi; 
	sNew.hInsertAfter = TVI_LAST; 
	sNew.hParent = hRoot;

	// add the item to the list
	HTREEITEM hTexture = (HTREEITEM)SendMessage(GetDlgItem(g_hDlg,IDC_TREE1), 
		TVM_INSERTITEM, 
		0,
		(LPARAM)(LPTVINSERTSTRUCT)&sNew);

	// for each texture in the list ... add it
	unsigned int iUV;
	float fBlend;
	aiTextureOp eOp;
	aiString szPath;
	for (unsigned int i = 0; i < 7;++i)
	{
		unsigned int iNum = 0;
		while (true)
		{
			if (AI_SUCCESS != aiGetMaterialTexture(pcMat,iNum,i,
				&szPath,&iUV,&fBlend,&eOp))
			{
				break;
			}
			AddTextureToDisplayList(i,iNum,&szPath,hTexture,
				pcMat,iTexture,iUV,fBlend,eOp);
			++iNum;
		}
	}
	return 1;
}

//-------------------------------------------------------------------------------
// Fill the UI combobox with a list of all supported view modi
//
// The display modes are added in order
//-------------------------------------------------------------------------------
int FillDisplayList(void)
{
	// Initialize the tree view window.

	// First, create the image list we will need.
#define NUM_BITMAPS 4
	HIMAGELIST hIml = ImageList_Create( 16,16,ILC_COLOR24, NUM_BITMAPS, 0 );


	// Load the bitmaps and add them to the image lists.
	HBITMAP hBmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_BFX));
	int iFX = ImageList_Add(hIml, hBmp, NULL);
	DeleteObject(hBmp);

	hBmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_BNODE));
	int iNode = ImageList_Add(hIml, hBmp, NULL);
	DeleteObject(hBmp);

	hBmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_BTX));
	int iTexture = ImageList_Add(hIml, hBmp, NULL);
	DeleteObject(hBmp);

	hBmp = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_BROOT));
	int iRoot = ImageList_Add(hIml, hBmp, NULL);
	DeleteObject(hBmp);

	// Associate the image list with the tree.
	TreeView_SetImageList(GetDlgItem(g_hDlg,IDC_TREE1), hIml, TVSIL_NORMAL);

	// fill in the first entry
	TVITEMEX tvi; 
	TVINSERTSTRUCT sNew;
	tvi.pszText = "Model";
	tvi.cchTextMax = (int)strlen(tvi.pszText);
	tvi.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_IMAGE | TVIF_HANDLE | TVIF_STATE;
	tvi.state = TVIS_EXPANDED ;
	tvi.iImage = iRoot;
	tvi.iSelectedImage = iRoot;
	tvi.lParam = (LPARAM)0; 

	sNew.itemex = tvi; 
    sNew.hInsertAfter = TVI_ROOT; 
	sNew.hParent = 0;

	HTREEITEM hRoot = (HTREEITEM)SendMessage(GetDlgItem(g_hDlg,IDC_TREE1), 
		TVM_INSERTITEM, 
		0,
		(LPARAM)(LPTVINSERTSTRUCT)&sNew);


	// add each loaded material
	for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumMaterials;++i)
	{
		AddMaterialToDisplayList(hRoot,g_pcAsset->pcScene->mMaterials[i],
			iFX,iTexture,i);
	}

	// now add all loaded nodes recursively
	AddNodeToDisplayList(iNode,0,0,g_pcAsset->pcScene->mRootNode,hRoot);
	return 1;
}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int CreateAssetData(void)
	{
	if (!g_pcAsset)return 0;

	g_iShaderCount = 0;

	for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumMeshes;++i)
		{
		// create vertex buffer
		if(FAILED( g_piDevice->CreateVertexBuffer(sizeof(AssetHelper::Vertex) *
			g_pcAsset->pcScene->mMeshes[i]->mNumVertices,
			D3DUSAGE_WRITEONLY,
			AssetHelper::Vertex::GetFVF(),
			D3DPOOL_DEFAULT, &g_pcAsset->apcMeshes[i]->piVB,NULL)))
			{
			MessageBox(g_hDlg,"Failed to create vertex buffer",
				"ASSIMP Viewer Utility",MB_OK);
			return 2;
		}

		// check whether we can use 16 bit indices
		if (g_pcAsset->pcScene->mMeshes[i]->mNumFaces * 3 >= 65536)
		{
			// create 32 bit index buffer
			if(FAILED( g_piDevice->CreateIndexBuffer( 4 *
				g_pcAsset->pcScene->mMeshes[i]->mNumFaces * 3,
				D3DUSAGE_WRITEONLY,
				D3DFMT_INDEX32,
				D3DPOOL_DEFAULT, &g_pcAsset->apcMeshes[i]->piIB,NULL)))
			{
				MessageBox(g_hDlg,"Failed to create 32 Bit index buffer",
					"ASSIMP Viewer Utility",MB_OK);
				return 2;
			}

			// now fill the index buffer
			unsigned int* pbData;
			g_pcAsset->apcMeshes[i]->piIB->Lock(0,0,(void**)&pbData,0);
			for (unsigned int x = 0; x < g_pcAsset->pcScene->mMeshes[i]->mNumFaces;++x)
			{
				for (unsigned int a = 0; a < 3;++a)
				{
					*pbData++ = g_pcAsset->pcScene->mMeshes[i]->mFaces[x].mIndices[a];
				}
			}
		}
		else
		{
			// create 16 bit index buffer
			if(FAILED( g_piDevice->CreateIndexBuffer( 2 *
				g_pcAsset->pcScene->mMeshes[i]->mNumFaces * 3,
				D3DUSAGE_WRITEONLY,
				D3DFMT_INDEX16,
				D3DPOOL_DEFAULT, &g_pcAsset->apcMeshes[i]->piIB,NULL)))
			{
				MessageBox(g_hDlg,"Failed to create 16 Bit index buffer",
					"ASSIMP Viewer Utility",MB_OK);
				return 2;
			}

			// now fill the index buffer
			uint16_t* pbData;
			g_pcAsset->apcMeshes[i]->piIB->Lock(0,0,(void**)&pbData,0);
			for (unsigned int x = 0; x < g_pcAsset->pcScene->mMeshes[i]->mNumFaces;++x)
			{
				for (unsigned int a = 0; a < 3;++a)
				{
					*pbData++ = (uint16_t)g_pcAsset->pcScene->mMeshes[i]->mFaces[x].mIndices[a];
				}
			}
		}
		g_pcAsset->apcMeshes[i]->piIB->Unlock();

		// now fill the vertex buffer
		AssetHelper::Vertex* pbData2;
		g_pcAsset->apcMeshes[i]->piVB->Lock(0,0,(void**)&pbData2,0);
		for (unsigned int x = 0; x < g_pcAsset->pcScene->mMeshes[i]->mNumVertices;++x)
			{
			pbData2->vPosition = g_pcAsset->pcScene->mMeshes[i]->mVertices[x];

			if (NULL == g_pcAsset->pcScene->mMeshes[i]->mNormals)
				pbData2->vNormal = aiVector3D(0.0f,0.0f,0.0f);
			else pbData2->vNormal = g_pcAsset->pcScene->mMeshes[i]->mNormals[x];

			if (NULL == g_pcAsset->pcScene->mMeshes[i]->mTangents)
				{
				pbData2->vTangent = aiVector3D(0.0f,0.0f,0.0f);
				pbData2->vBitangent = aiVector3D(0.0f,0.0f,0.0f);
				}
			else 
				{
				pbData2->vTangent = g_pcAsset->pcScene->mMeshes[i]->mTangents[x];
				pbData2->vBitangent = g_pcAsset->pcScene->mMeshes[i]->mBitangents[x];
				}

			if (g_pcAsset->pcScene->mMeshes[i]->HasVertexColors( 0))
				{
				pbData2->dColorDiffuse = D3DCOLOR_ARGB(
					((unsigned char)std::max( std::min( g_pcAsset->pcScene->	
					mMeshes[i]->mColors[0][x].a * 255.0f, 255.0f),0.0f)),
					((unsigned char)std::max( std::min( g_pcAsset->pcScene->
					mMeshes[i]->mColors[0][x].r * 255.0f, 255.0f),0.0f)),
					((unsigned char)std::max( std::min( g_pcAsset->pcScene->
					mMeshes[i]->mColors[0][x].g * 255.0f, 255.0f),0.0f)),
					((unsigned char)std::max( std::min( g_pcAsset->pcScene->
					mMeshes[i]->mColors[0][x].b * 255.0f, 255.0f),0.0f)));
				}
			else pbData2->dColorDiffuse = D3DCOLOR_ARGB(0xFF,0,0,0);

			// ignore a third texture coordinate component
			if (g_pcAsset->pcScene->mMeshes[i]->HasTextureCoords( 0))
				{
				pbData2->vTextureUV = aiVector2D(
					g_pcAsset->pcScene->mMeshes[i]->mTextureCoords[0][x].x,
					g_pcAsset->pcScene->mMeshes[i]->mTextureCoords[0][x].y);
				}
			else pbData2->vTextureUV = aiVector2D(0.0f,0.0f);
			++pbData2;
			}
		g_pcAsset->apcMeshes[i]->piVB->Unlock();

		// now generate the second vertex buffer, holding all normals
		GenerateNormalsAsLineList(g_pcAsset->apcMeshes[i],g_pcAsset->pcScene->mMeshes[i]);

		// create the material for the mesh
		CreateMaterial(g_pcAsset->apcMeshes[i],g_pcAsset->pcScene->mMeshes[i]);
		}
	CLogDisplay::Instance().AddEntry("[OK] The asset has been loaded successfully");


	// now get the number of unique shaders generated for the asset
	// (even if the environment changes this number won't change)
	char szTemp[32];
	sprintf(szTemp,"%i", g_iShaderCount);
	SetDlgItemText(g_hDlg,IDC_ESHADER,szTemp);

	FillDisplayList();
	return FillAnimList();
	}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int DeleteAssetData(void)
	{
	if (!g_pcAsset)return 0;

	// TODO: Move this to a proper destructor
	for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumMeshes;++i)
		{
		if(g_pcAsset->apcMeshes[i]->piVB)
			{
			g_pcAsset->apcMeshes[i]->piVB->Release();
			g_pcAsset->apcMeshes[i]->piVB = NULL;
			}
		if(g_pcAsset->apcMeshes[i]->piVBNormals)
			{
			g_pcAsset->apcMeshes[i]->piVBNormals->Release();
			g_pcAsset->apcMeshes[i]->piVBNormals = NULL;
			}
		if(g_pcAsset->apcMeshes[i]->piIB)
			{
			g_pcAsset->apcMeshes[i]->piIB->Release();
			g_pcAsset->apcMeshes[i]->piIB = NULL;
			}
		if(g_pcAsset->apcMeshes[i]->piEffect)
			{
			g_pcAsset->apcMeshes[i]->piEffect->Release();
			g_pcAsset->apcMeshes[i]->piEffect = NULL;
			}
		if(g_pcAsset->apcMeshes[i]->piDiffuseTexture)
			{
			g_pcAsset->apcMeshes[i]->piDiffuseTexture->Release();
			g_pcAsset->apcMeshes[i]->piDiffuseTexture = NULL;
			}
		if(g_pcAsset->apcMeshes[i]->piNormalTexture)
			{
			g_pcAsset->apcMeshes[i]->piNormalTexture->Release();
			g_pcAsset->apcMeshes[i]->piNormalTexture = NULL;
			}
		if(g_pcAsset->apcMeshes[i]->piSpecularTexture)
			{
			g_pcAsset->apcMeshes[i]->piSpecularTexture->Release();
			g_pcAsset->apcMeshes[i]->piSpecularTexture = NULL;
			}
		if(g_pcAsset->apcMeshes[i]->piAmbientTexture)
			{
			g_pcAsset->apcMeshes[i]->piAmbientTexture->Release();
			g_pcAsset->apcMeshes[i]->piAmbientTexture = NULL;
			}
		if(g_pcAsset->apcMeshes[i]->piEmissiveTexture)
			{
			g_pcAsset->apcMeshes[i]->piEmissiveTexture->Release();
			g_pcAsset->apcMeshes[i]->piEmissiveTexture = NULL;
			}
		}
	return 1;
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int SetupFPSView()
	{
	if (!g_bFPSView)
		{
		g_sCamera.vPos = aiVector3D(0.0f,0.0f,g_fWheelPos);
		g_sCamera.vLookAt = aiVector3D(0.0f,0.0f,1.0f);
		g_sCamera.vUp = aiVector3D(0.0f,1.0f,0.0f);
		g_sCamera.vRight = aiVector3D(0.0f,1.0f,0.0f);
		}
	else
		{
		g_fWheelPos = g_sCamera.vPos.z;
		g_sCamera.vPos = aiVector3D(0.0f,0.0f,-10.0f);
		g_sCamera.vLookAt = aiVector3D(0.0f,0.0f,1.0f);
		g_sCamera.vUp = aiVector3D(0.0f,1.0f,0.0f);
		g_sCamera.vRight = aiVector3D(0.0f,1.0f,0.0f);
		}
	return 1;
	}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int InitD3D(void)
	{
	if (NULL == g_piD3D)
		{
		g_piD3D = Direct3DCreate9(D3D_SDK_VERSION);
		if (NULL == g_piD3D)return 0;
		}
	return 1;
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int ShutdownD3D(void)
	{
	ShutdownDevice();
	if (NULL != g_piD3D)
		{
		g_piD3D->Release();
		g_piD3D = NULL;
		}
	return 1;
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int ShutdownDevice(void)
	{
	if (NULL != g_piDevice)
		{
		g_piDevice->Release();
		g_piDevice = NULL;
		}
	if (NULL != g_piDefaultEffect)
		{
		g_piDefaultEffect->Release();
		g_piDefaultEffect = NULL;
		}
	if (NULL != g_piNormalsEffect)
		{
		g_piNormalsEffect->Release();
		g_piNormalsEffect = NULL;
		}
	if (NULL != g_piPassThroughEffect)
		{
		g_piPassThroughEffect->Release();
		g_piPassThroughEffect = NULL;
		}
	if (NULL != g_pcTexture)
		{
		g_pcTexture->Release();
		g_pcTexture = NULL;
		}
	delete[] g_szImageMask;
	g_szImageMask = NULL;
	CBackgroundPainter::Instance().ReleaseNativeResource();
	CLogDisplay::Instance().ReleaseNativeResource();
	return 1;
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int CreateHUDTexture()
	{
	// lock the memory resource ourselves
	HRSRC res = FindResource(NULL,MAKEINTRESOURCE(IDR_HUD),RT_RCDATA);
	HGLOBAL hg = LoadResource(NULL,res);
	void* pData = LockResource(hg);

	if(FAILED(D3DXCreateTextureFromFileInMemoryEx(g_piDevice,
		pData,SizeofResource(NULL,res),
		D3DX_DEFAULT_NONPOW2,
		D3DX_DEFAULT_NONPOW2,
		1,
		0,
		D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED,
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		0,
		NULL,
		NULL,
		&g_pcTexture)))
		{
		CLogDisplay::Instance().AddEntry("[ERROR] Unable to load HUD texture",
			D3DCOLOR_ARGB(0xFF,0xFF,0,0));

		g_pcTexture  = NULL;
		g_szImageMask = NULL;

		UnlockResource(hg);
		FreeResource(hg);
		return 0;
		}

	UnlockResource(hg);
	FreeResource(hg);

	D3DSURFACE_DESC sDesc;
	g_pcTexture->GetLevelDesc(0,&sDesc);


	// lock the memory resource ourselves
	res = FindResource(NULL,MAKEINTRESOURCE(IDR_HUDMASK),RT_RCDATA);
	hg = LoadResource(NULL,res);
	pData = LockResource(hg);

	IDirect3DTexture9* pcTex;
	if(FAILED(D3DXCreateTextureFromFileInMemoryEx(g_piDevice,
		pData,SizeofResource(NULL,res),
		sDesc.Width,
		sDesc.Height,
		1,
		0,
		D3DFMT_L8,
		D3DPOOL_MANAGED, // unnecessary
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		0,
		NULL,
		NULL,
		&pcTex)))
		{
		CLogDisplay::Instance().AddEntry("[ERROR] Unable to load HUD mask texture",
			D3DCOLOR_ARGB(0xFF,0xFF,0,0));
		g_szImageMask = NULL;

		UnlockResource(hg);
		FreeResource(hg);
		return 0;
		}

	UnlockResource(hg);
	FreeResource(hg);

	// lock the texture and copy it to get a pointer
	D3DLOCKED_RECT sRect;
	pcTex->LockRect(0,&sRect,NULL,D3DLOCK_READONLY);

	unsigned char* szOut = new unsigned char[sDesc.Width * sDesc.Height];
	unsigned char* _szOut = szOut;

	unsigned char* szCur = (unsigned char*) sRect.pBits;
	for (unsigned int y = 0; y < sDesc.Height;++y)
		{
		memcpy(_szOut,szCur,sDesc.Width);
		
		szCur += sRect.Pitch;
		_szOut += sDesc.Width;
		}
	pcTex->UnlockRect(0);
	pcTex->Release();

	g_szImageMask = szOut;
	return 1;
	}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int CreateDevice (bool p_bMultiSample,bool p_bSuperSample,bool bHW /*= true*/)
	{
	D3DDEVTYPE eType = bHW ? D3DDEVTYPE_HAL : D3DDEVTYPE_REF;

	// get the client rectangle of the window.
	RECT sRect;
	GetWindowRect(GetDlgItem(g_hDlg,IDC_RT),&sRect);
	sRect.right -= sRect.left;
	sRect.bottom -= sRect.top;

	D3DPRESENT_PARAMETERS sParams;
	memset(&sParams,0,sizeof(D3DPRESENT_PARAMETERS));

	// get the current display mode
	D3DDISPLAYMODE sMode;
	g_piD3D->GetAdapterDisplayMode(0,&sMode);

	// fill the presentation parameter structure
	sParams.Windowed				= TRUE;
	sParams.hDeviceWindow			= GetDlgItem( g_hDlg, IDC_RT );
	sParams.EnableAutoDepthStencil	= TRUE;
	sParams.PresentationInterval	= D3DPRESENT_INTERVAL_ONE;
	sParams.BackBufferWidth			= (UINT)sRect.right;
	sParams.BackBufferHeight		= (UINT)sRect.bottom;
	sParams.SwapEffect				= D3DSWAPEFFECT_DISCARD;

	// check whether we can use a D32 depth buffer format
	if (SUCCEEDED ( g_piD3D->CheckDepthStencilMatch(0,eType,
		D3DFMT_X8R8G8B8,D3DFMT_X8R8G8B8,D3DFMT_D32)))
	{
		sParams.AutoDepthStencilFormat = D3DFMT_D32;
	}
	else sParams.AutoDepthStencilFormat = D3DFMT_D24X8;

	// find the highest multisample type available on this device
	D3DMULTISAMPLE_TYPE sMS = D3DMULTISAMPLE_2_SAMPLES;
	D3DMULTISAMPLE_TYPE sMSOut = D3DMULTISAMPLE_NONE;
	DWORD dwQuality = 0;
	if (p_bMultiSample)
		{
		while ((D3DMULTISAMPLE_TYPE)(D3DMULTISAMPLE_16_SAMPLES + 1)  != 
			  (sMS = (D3DMULTISAMPLE_TYPE)(sMS + 1)))
			{
			if(SUCCEEDED( g_piD3D->CheckDeviceMultiSampleType(0,eType,
				sMode.Format,TRUE,sMS,&dwQuality)))
				{
				sMSOut = sMS;
				}
			}
		if (0 != dwQuality)dwQuality -= 1;

	
		sParams.MultiSampleQuality = dwQuality;
		sParams.MultiSampleType = sMSOut;
		}

	// create the D3D9 device object
	if(FAILED(g_piD3D->CreateDevice(0,eType,
		g_hDlg,D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,&sParams,&g_piDevice)))
		{
		if(FAILED(g_piD3D->CreateDevice(0,eType,
			g_hDlg,D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,&sParams,&g_piDevice)))
			{
			// if hardware fails use software rendering instead
			if (bHW)return CreateDevice(p_bMultiSample,p_bSuperSample,false);
			return 0;
			}
		}
	g_piDevice->SetFVF(AssetHelper::Vertex::GetFVF());

	// compile the default material shader (gray gouraud/phong)
	ID3DXBuffer* piBuffer = NULL;
	if(FAILED( D3DXCreateEffect(g_piDevice,
		g_szDefaultShader.c_str(),
		(UINT)g_szDefaultShader.length(),
		NULL,
		NULL,
		D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,
		NULL,
		&g_piDefaultEffect,&piBuffer)))
		{
		if( piBuffer) 
			{
			MessageBox(g_hDlg,(LPCSTR)piBuffer->GetBufferPointer(),"HLSL",MB_OK);
			piBuffer->Release();
			}
		return 0;
		}
	if( piBuffer) 
		{
		piBuffer->Release();
		piBuffer = NULL;
		}

	// create the shader used to draw the HUD
	if(FAILED( D3DXCreateEffect(g_piDevice,
		g_szPassThroughShader.c_str(),(UINT)g_szPassThroughShader.length(),
		NULL,NULL,D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,NULL,&g_piPassThroughEffect,&piBuffer)))
		{
		if( piBuffer) 
			{
			MessageBox(g_hDlg,(LPCSTR
				)piBuffer->GetBufferPointer(),"HLSL",MB_OK);
			piBuffer->Release();
			}
		return 0;
		}
	if( piBuffer) 
		{
		piBuffer->Release();
		piBuffer = NULL;
		}

	// create the shader used to visualize normal vectors
	if(FAILED( D3DXCreateEffect(g_piDevice,
		g_szNormalsShader.c_str(),(UINT)g_szNormalsShader.length(),
		NULL,NULL,D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,NULL,&g_piNormalsEffect, &piBuffer)))
		{
		if( piBuffer) 
			{
			MessageBox(g_hDlg,(LPCSTR
				)piBuffer->GetBufferPointer(),"HLSL",MB_OK);
			piBuffer->Release();
			}
		return 0;
		}
	if( piBuffer) 
		{
		piBuffer->Release();
		piBuffer = NULL;
		}

	// get the capabilities of the device object
	g_piDevice->GetDeviceCaps(&g_sCaps);
	if(g_sCaps.PixelShaderVersion < D3DPS_VERSION(3,0))
		{
		EnableWindow(GetDlgItem(g_hDlg,IDC_LOWQUALITY),FALSE);
		}

	// create the texture for the HUD
	CreateHUDTexture();
	CBackgroundPainter::Instance().RecreateNativeResource();
	CLogDisplay::Instance().RecreateNativeResource();

	g_piPassThroughEffect->SetTexture("TEXTURE_2D",g_pcTexture);
	return 1;
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int CreateDevice (void)
	{
	return CreateDevice(g_sOptions.bMultiSample,
		g_sOptions.bSuperSample);
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int GetProjectionMatrix (aiMatrix4x4& p_mOut)
	{
	const float fFarPlane = 100.0f;
	const float fNearPlane = 0.1f;
	const float fFOV = (float)(45.0 * 0.0174532925);

	const float s = 1.0f / tanf(fFOV * 0.5f);
    const float Q = fFarPlane / (fFarPlane - fNearPlane);
	
	RECT sRect;
	GetWindowRect(GetDlgItem(g_hDlg,IDC_RT),&sRect);
	sRect.right -= sRect.left;
	sRect.bottom -= sRect.top;
	const float fAspect = (float)sRect.right / (float)sRect.bottom;

    p_mOut = aiMatrix4x4(
		s / fAspect, 0.0f, 0.0f, 0.0f,
        0.0f, s, 0.0f, 0.0f,
        0.0f, 0.0f, Q, 1.0f,
        0.0f, 0.0f, -Q * fNearPlane, 0.0f);
	return 1;
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
aiVector3D GetCameraMatrix (aiMatrix4x4& p_mOut)
	{
	D3DXMATRIX view;
	D3DXMatrixIdentity( &view );

	D3DXVec3Normalize( (D3DXVECTOR3*)&g_sCamera.vLookAt, (D3DXVECTOR3*)&g_sCamera.vLookAt );
	D3DXVec3Cross( (D3DXVECTOR3*)&g_sCamera.vRight, (D3DXVECTOR3*)&g_sCamera.vUp, (D3DXVECTOR3*)&g_sCamera.vLookAt );
	D3DXVec3Normalize( (D3DXVECTOR3*)&g_sCamera.vRight, (D3DXVECTOR3*)&g_sCamera.vRight );
	D3DXVec3Cross( (D3DXVECTOR3*)&g_sCamera.vUp, (D3DXVECTOR3*)&g_sCamera.vLookAt, (D3DXVECTOR3*)&g_sCamera.vRight );
	D3DXVec3Normalize( (D3DXVECTOR3*)&g_sCamera.vUp, (D3DXVECTOR3*)&g_sCamera.vUp );

	view._11 = g_sCamera.vRight.x;
	view._12 = g_sCamera.vUp.x;
	view._13 = g_sCamera.vLookAt.x;
	view._14 = 0.0f;

	view._21 = g_sCamera.vRight.y;
	view._22 = g_sCamera.vUp.y;
	view._23 = g_sCamera.vLookAt.y;
	view._24 = 0.0f;

	view._31 = g_sCamera.vRight.z;
	view._32 = g_sCamera.vUp.z;
	view._33 = g_sCamera.vLookAt.z;
	view._34 = 0.0f;

	view._41 = -D3DXVec3Dot( (D3DXVECTOR3*)&g_sCamera.vPos, (D3DXVECTOR3*)&g_sCamera.vRight );
	view._42 = -D3DXVec3Dot( (D3DXVECTOR3*)&g_sCamera.vPos, (D3DXVECTOR3*)&g_sCamera.vUp );
	view._43 = -D3DXVec3Dot( (D3DXVECTOR3*)&g_sCamera.vPos, (D3DXVECTOR3*)&g_sCamera.vLookAt );
	view._44 =  1.0f;

	memcpy(&p_mOut,&view,sizeof(aiMatrix4x4));

	return g_sCamera.vPos;
	}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int SetupMaterial (AssetHelper::MeshHelper* pcMesh,
	const aiMatrix4x4& pcProj,
	const aiMatrix4x4& aiMe,
	const aiMatrix4x4& pcCam,
	const aiVector3D& vPos)
	{
	if (!pcMesh->piEffect)return 0;

	ID3DXEffect* piEnd = pcMesh->piEffect;

	piEnd->SetMatrix("WorldViewProjection",
		(const D3DXMATRIX*)&pcProj);

	piEnd->SetMatrix("World",(const D3DXMATRIX*)&aiMe);
	piEnd->SetMatrix("WorldInverseTranspose",
		(const D3DXMATRIX*)&pcCam);

	D3DXVECTOR4 apcVec[5];
	memset(apcVec,0,sizeof(apcVec));
	apcVec[0].x = g_avLightDirs[0].x;
	apcVec[0].y = g_avLightDirs[0].y;
	apcVec[0].z = g_avLightDirs[0].z;
	apcVec[1].x = g_avLightDirs[0].x * -1.0f;
	apcVec[1].y = g_avLightDirs[0].y * -1.0f;
	apcVec[1].z = g_avLightDirs[0].z * -1.0f;
	D3DXVec4Normalize(&apcVec[0],&apcVec[0]);
	D3DXVec4Normalize(&apcVec[1],&apcVec[1]);
	piEnd->SetVectorArray("afLightDir",apcVec,5);

	if(g_sOptions.b3Lights)
		{
		apcVec[0].x = 1.0f;
		apcVec[0].y = 1.0f;
		apcVec[0].z = 1.0f;
		apcVec[0].w = 1.0f;

		apcVec[1].x = 0.1f;
		apcVec[1].y = 1.0f;
		apcVec[1].z = 0.1f;
		apcVec[1].w = 1.0f;
		}
	else
		{
		apcVec[0].x = 1.0f;
		apcVec[0].y = 1.0f;
		apcVec[0].z = 1.0f;
		apcVec[0].w = 1.0f;

		apcVec[1].x = 0.0f;
		apcVec[1].y = 0.0f;
		apcVec[1].z = 0.0f;
		apcVec[1].w = 0.0f;
		}
	apcVec[0] *= g_fLightIntensity;
	apcVec[1] *= g_fLightIntensity;
	piEnd->SetVectorArray("afLightColor",apcVec,5);

	if(g_sOptions.b3Lights)
		{
		apcVec[0].x = 0.05f;
		apcVec[0].y = 0.05f;
		apcVec[0].z = 0.05f;
		apcVec[0].w = 1.0f;

		apcVec[1].x = 0.05f;
		apcVec[1].y = 0.05f;
		apcVec[1].z = 0.05f;
		apcVec[1].w = 1.0f;
		}
	else
		{
		apcVec[0].x = 0.05f;
		apcVec[0].y = 0.05f;
		apcVec[0].z = 0.05f;
		apcVec[0].w = 1.0f;

		apcVec[1].x = 0.0f;
		apcVec[1].y = 0.0f;
		apcVec[1].z = 0.0f;
		apcVec[1].w = 0.0f;
		}
	apcVec[0] *= g_fLightIntensity;
	apcVec[1] *= g_fLightIntensity;
	piEnd->SetVectorArray("afLightColorAmbient",apcVec,5);


	apcVec[0].x = vPos.x;
	apcVec[0].y = vPos.y;
	apcVec[0].z = vPos.z;
	piEnd->SetVector( "vCameraPos",&apcVec[0]);

	if (pcMesh->bSharedFX)
		{
		// now commit all constants to the shader
		if (1.0f != pcMesh->fOpacity)
			pcMesh->piEffect->SetFloat("TRANSPARENCY",pcMesh->fOpacity);
		if (pcMesh->eShadingMode  != aiShadingMode_Gouraud)
			pcMesh->piEffect->SetFloat("SPECULARITY",pcMesh->fShininess);

		pcMesh->piEffect->SetVector("DIFFUSE_COLOR",&pcMesh->vDiffuseColor);
		pcMesh->piEffect->SetVector("SPECULAR_COLOR",&pcMesh->vSpecularColor);
		pcMesh->piEffect->SetVector("AMBIENT_COLOR",&pcMesh->vAmbientColor);
		pcMesh->piEffect->SetVector("EMISSIVE_COLOR",&pcMesh->vEmissiveColor);

		if (pcMesh->piOpacityTexture)
			pcMesh->piEffect->SetTexture("OPACITY_TEXTURE",pcMesh->piOpacityTexture);
		if (pcMesh->piDiffuseTexture)
			pcMesh->piEffect->SetTexture("DIFFUSE_TEXTURE",pcMesh->piDiffuseTexture);
		if (pcMesh->piSpecularTexture)
			pcMesh->piEffect->SetTexture("SPECULAR_TEXTURE",pcMesh->piSpecularTexture);
		if (pcMesh->piAmbientTexture)
			pcMesh->piEffect->SetTexture("AMBIENT_TEXTURE",pcMesh->piAmbientTexture);
		if (pcMesh->piEmissiveTexture)
			pcMesh->piEffect->SetTexture("EMISSIVE_TEXTURE",pcMesh->piEmissiveTexture);
		if (pcMesh->piNormalTexture)
			pcMesh->piEffect->SetTexture("NORMAL_TEXTURE",pcMesh->piNormalTexture);

		if (CBackgroundPainter::TEXTURE_CUBE == CBackgroundPainter::Instance().GetMode())
			{
			piEnd->SetTexture("lw_tex_envmap",CBackgroundPainter::Instance().GetTexture());
			}
		}

	if (g_sCaps.PixelShaderVersion < D3DPS_VERSION(3,0) || g_sOptions.bLowQuality)
		{
		if (g_sOptions.b3Lights)
			piEnd->SetTechnique("MaterialFXSpecular_PS20_D2");
		else piEnd->SetTechnique("MaterialFXSpecular_PS20_D1");
		}
	else
		{
		if (g_sOptions.b3Lights)
			piEnd->SetTechnique("MaterialFXSpecular_D2");
		else piEnd->SetTechnique("MaterialFXSpecular_D1");
		}

	UINT dwPasses = 0;
	piEnd->Begin(&dwPasses,0);
	piEnd->BeginPass(0);
	return 1;
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int EndMaterial (AssetHelper::MeshHelper* pcMesh)
	{
	if (!pcMesh->piEffect)return 0;

	pcMesh->piEffect->EndPass();
	pcMesh->piEffect->End();

	return 1;
	}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int RenderNode (aiNode* piNode,const aiMatrix4x4& piMatrix, bool bAlpha = false)
	{
	aiMatrix4x4 mTemp = piNode->mTransformation;
	mTemp.Transpose();
	aiMatrix4x4 aiMe = mTemp * piMatrix;

	aiMatrix4x4 pcProj;
	GetProjectionMatrix(pcProj);

	aiMatrix4x4 pcCam;
	aiVector3D vPos = GetCameraMatrix(pcCam);
	pcProj = (aiMe * pcCam) * pcProj;

	pcCam = aiMe;
	pcCam.Inverse().Transpose();

	// VERY UNOPTIMIZED, much stuff is redundant. Who cares?
	if (!g_sOptions.bRenderMats && !bAlpha)
		{
		// this is very similar to the code in SetupMaterial()
		ID3DXEffect* piEnd = g_piDefaultEffect;

		piEnd->SetMatrix("WorldViewProjection",
			(const D3DXMATRIX*)&pcProj);

		piEnd->SetMatrix("World",(const D3DXMATRIX*)&aiMe);
		piEnd->SetMatrix("WorldInverseTranspose",
			(const D3DXMATRIX*)&pcCam);

		if ( CBackgroundPainter::TEXTURE_CUBE == CBackgroundPainter::Instance().GetMode())
			{
			pcCam = pcCam * pcProj;
			piEnd->SetMatrix("ViewProj",(const D3DXMATRIX*)&pcCam);
			pcCam.Inverse();
			piEnd->SetMatrix("InvViewProj",
				(const D3DXMATRIX*)&pcCam);
			}

		D3DXVECTOR4 apcVec[5];
		apcVec[0].x = g_avLightDirs[0].x;
		apcVec[0].y = g_avLightDirs[0].y;
		apcVec[0].z = g_avLightDirs[0].z;
		apcVec[1].x = g_avLightDirs[0].x * -1.0f;
		apcVec[1].y = g_avLightDirs[0].y * -1.0f;
		apcVec[1].z = g_avLightDirs[0].z * -1.0f;

		D3DXVec4Normalize(&apcVec[0],&apcVec[0]);
		D3DXVec4Normalize(&apcVec[1],&apcVec[1]);
		piEnd->SetVectorArray("afLightDir",apcVec,5);

		if(g_sOptions.b3Lights)
			{
			apcVec[0].x = 0.6f;
			apcVec[0].y = 0.6f;
			apcVec[0].z = 0.6f;
			apcVec[0].w = 1.0f;

			apcVec[1].x = 0.3f;
			apcVec[1].y = 0.0f;
			apcVec[1].z = 0.0f;
			apcVec[1].w = 1.0f;
			}
		else
			{
			apcVec[0].x = 1.0f;
			apcVec[0].y = 1.0f;
			apcVec[0].z = 1.0f;
			apcVec[0].w = 1.0f;

			apcVec[1].x = 0.0f;
			apcVec[1].y = 0.0f;
			apcVec[1].z = 0.0f;
			apcVec[1].w = 0.0f;
			}
		apcVec[0] *= g_fLightIntensity;
		apcVec[1] *= g_fLightIntensity;
		piEnd->SetVectorArray("afLightColor",apcVec,5);

		apcVec[0].x = vPos.x;
		apcVec[0].y = vPos.y;
		apcVec[0].z = vPos.z;
		piEnd->SetVector( "vCameraPos",&apcVec[0]);

		if (g_sCaps.PixelShaderVersion < D3DPS_VERSION(3,0) || g_sOptions.bLowQuality)
			{
			if (g_sOptions.b3Lights)
				piEnd->SetTechnique("DefaultFXSpecular_PS20_D2");
			else piEnd->SetTechnique("DefaultFXSpecular_PS20_D1");
			}
		else
			{
			if (g_sOptions.b3Lights)
				piEnd->SetTechnique("DefaultFXSpecular_D2");
			else piEnd->SetTechnique("DefaultFXSpecular_D1");
			}

		UINT dwPasses = 0;
		piEnd->Begin(&dwPasses,0);
		piEnd->BeginPass(0);
		}
	D3DXVECTOR4 vVector = g_aclNormalColors[g_iCurrentColor];
	if (++g_iCurrentColor == 14)
		{
		g_iCurrentColor = 0;
		}
	if (! (!g_sOptions.bRenderMats && bAlpha))
		{
		for (unsigned int i = 0; i < piNode->mNumMeshes;++i)
			{
			// don't render the mesh if the render pass is incorrect
			if (g_sOptions.bRenderMats && (
				g_pcAsset->apcMeshes[piNode->mMeshes[i]]->piOpacityTexture || 
				g_pcAsset->apcMeshes[piNode->mMeshes[i]]->fOpacity != 1.0f))
				{
				if (!bAlpha)continue;
				}
			else if (bAlpha)continue;

			// set vertex and index buffer and the material ...
			g_piDevice->SetStreamSource(0,
				g_pcAsset->apcMeshes[piNode->mMeshes[i]]->piVB,0,
				sizeof(AssetHelper::Vertex));

			// now setup the material
			if (g_sOptions.bRenderMats)
				SetupMaterial(g_pcAsset->apcMeshes[piNode->mMeshes[i]],pcProj,aiMe,pcCam,vPos);

			g_piDevice->SetIndices(g_pcAsset->apcMeshes[piNode->mMeshes[i]]->piIB);
			g_piDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
				0,0,
				g_pcAsset->pcScene->mMeshes[piNode->mMeshes[i]]->mNumVertices,0,
				g_pcAsset->pcScene->mMeshes[piNode->mMeshes[i]]->mNumFaces);

			// now end the material
			if (g_sOptions.bRenderMats)
				EndMaterial(g_pcAsset->apcMeshes[piNode->mMeshes[i]]);

			// render normal vectors?
			if (g_sOptions.bRenderNormals && g_pcAsset->apcMeshes[piNode->mMeshes[i]]->piVBNormals)
				{
				// this is very similar to the code in SetupMaterial()
				ID3DXEffect* piEnd = g_piNormalsEffect;

				piEnd->SetVector("OUTPUT_COLOR",&vVector);

				piEnd->SetMatrix("WorldViewProjection",
					(const D3DXMATRIX*)&pcProj);

				UINT dwPasses = 0;
				piEnd->Begin(&dwPasses,0);
				piEnd->BeginPass(0);

				g_piDevice->SetStreamSource(0,
					g_pcAsset->apcMeshes[piNode->mMeshes[i]]->piVBNormals,0,
					sizeof(AssetHelper::LineVertex));

				g_piDevice->DrawPrimitive(D3DPT_LINELIST,0,
					g_pcAsset->pcScene->mMeshes[piNode->mMeshes[i]]->mNumVertices);

				piEnd->EndPass();
				piEnd->End();
				}
			}
		if (!g_sOptions.bRenderMats)
			{
			g_piDefaultEffect->EndPass();
			g_piDefaultEffect->End();
			}
		}
	for (unsigned int i = 0; i < piNode->mNumChildren;++i)
		{
		RenderNode(piNode->mChildren[i],aiMe,bAlpha );
		}
	return 1;
	}


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
int Render (void)
	{
	g_iCurrentColor = 0;

	// setup wireframe/solid rendering mode
	if (g_sOptions.eDrawMode == RenderOptions::WIREFRAME)
		{
		g_piDevice->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);
		}
	else g_piDevice->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);

	g_piDevice->BeginScene();

	// draw the scene background (clear and texture 2d)
	CBackgroundPainter::Instance().OnPreRender();

	// draw all opaque objects in the scene
	aiMatrix4x4 m;
	if (NULL != g_pcAsset && NULL != g_pcAsset->pcScene->mRootNode)
		{
		if(CBackgroundPainter::TEXTURE_CUBE == CBackgroundPainter::Instance().GetMode())
			HandleMouseInputSkyBox();

		// handle input commands
		HandleMouseInputLightRotate();
		HandleMouseInputLightIntensityAndColor();
		if(g_bFPSView)
			{
			HandleMouseInputFPS();
			HandleKeyboardInputFPS();
			}
		else
			{
			HandleMouseInputLocal();
			}

		// compute auto rotation depending on the time passed
		if (g_sOptions.bRotate)
			{
			aiMatrix4x4 mMat;
			D3DXMatrixRotationYawPitchRoll((D3DXMATRIX*)&mMat,
				g_vRotateSpeed.x * g_fElpasedTime,
				g_vRotateSpeed.y * g_fElpasedTime,
				g_vRotateSpeed.z * g_fElpasedTime);
			g_mWorldRotate = g_mWorldRotate * mMat;
			}

		// Handle rotations of light source(s)
		if (g_sOptions.bLightRotate)
			{
			aiMatrix4x4 mMat;
			D3DXMatrixRotationYawPitchRoll((D3DXMATRIX*)&mMat,
				g_vRotateSpeed.x * g_fElpasedTime * 0.5f,
				g_vRotateSpeed.y * g_fElpasedTime * 0.5f,
				g_vRotateSpeed.z * g_fElpasedTime * 0.5f);
			
			D3DXVec3TransformNormal((D3DXVECTOR3*)&g_avLightDirs[0],
				(D3DXVECTOR3*)&g_avLightDirs[0],(D3DXMATRIX*)&mMat);

			// 2 lights to rotate?
			if (g_sOptions.b3Lights)
				{
				D3DXVec3TransformNormal((D3DXVECTOR3*)&g_avLightDirs[1],
					(D3DXVECTOR3*)&g_avLightDirs[1],(D3DXMATRIX*)&mMat);

				g_avLightDirs[1].Normalize();
				}
			g_avLightDirs[0].Normalize();
			}

		m =  g_mWorld * g_mWorldRotate ;
		RenderNode(g_pcAsset->pcScene->mRootNode,m,false);
		}

	// if a cube texture is loaded as background image, the user
	// should be able to rotate it even if no asset is loaded
	else if(CBackgroundPainter::TEXTURE_CUBE == CBackgroundPainter::Instance().GetMode())
		{
		if (g_bFPSView)
			{
			HandleMouseInputFPS();
			HandleKeyboardInputFPS();
			}
		HandleMouseInputSkyBox();

		// need to store the last mouse position in the global variable
		// HandleMouseInputFPS() is doing this internally
		if (!g_bFPSView)
			{
			g_LastmousePos.x = g_mousePos.x;
			g_LastmousePos.y = g_mousePos.y;
			}
		}

	// draw the scene background
	CBackgroundPainter::Instance().OnPostRender();

	// draw all non-opaque objects in the scene
	if (NULL != g_pcAsset && NULL != g_pcAsset->pcScene->mRootNode)
		{
		RenderNode(g_pcAsset->pcScene->mRootNode,m,true);
		}

	// draw the HUD texture on top of the rendered scene using
	// pre-projected vertices
	if (!g_bFPSView && g_pcAsset && g_pcTexture)
		{
		RECT sRect;
		GetWindowRect(GetDlgItem(g_hDlg,IDC_RT),&sRect);
		sRect.right -= sRect.left;
		sRect.bottom -= sRect.top;

		struct SVertex
			{
			float x,y,z,w,u,v;
			};

		UINT dw;
		g_piPassThroughEffect->Begin(&dw,0);
		g_piPassThroughEffect->BeginPass(0);

		D3DSURFACE_DESC sDesc;
		g_pcTexture->GetLevelDesc(0,&sDesc);
		SVertex as[4];
		float fHalfX = ((float)sRect.right-(float)sDesc.Width) / 2.0f;
		float fHalfY = ((float)sRect.bottom-(float)sDesc.Height) / 2.0f;
		as[1].x = fHalfX;
		as[1].y = fHalfY;
		as[1].z = 0.2f;
		as[1].w = 1.0f;
		as[1].u = 0.0f;
		as[1].v = 0.0f;

		as[3].x = (float)sRect.right-fHalfX;
		as[3].y = fHalfY;
		as[3].z = 0.2f;
		as[3].w = 1.0f;
		as[3].u = 1.0f;
		as[3].v = 0.0f;

		as[0].x = fHalfX;
		as[0].y = (float)sRect.bottom-fHalfY;
		as[0].z = 0.2f;
		as[0].w = 1.0f;
		as[0].u = 0.0f;
		as[0].v = 1.0f;

		as[2].x = (float)sRect.right-fHalfX;
		as[2].y = (float)sRect.bottom-fHalfY;
		as[2].z = 0.2f;
		as[2].w = 1.0f;
		as[2].u = 1.0f;
		as[2].v = 1.0f;

		as[0].x -= 0.5f;as[1].x -= 0.5f;as[2].x -= 0.5f;as[3].x -= 0.5f;
		as[0].y -= 0.5f;as[1].y -= 0.5f;as[2].y -= 0.5f;as[3].y -= 0.5f;

		DWORD dw2;g_piDevice->GetFVF(&dw2);
		g_piDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
		g_piDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,
			&as,sizeof(SVertex));

		g_piPassThroughEffect->EndPass();
		g_piPassThroughEffect->End();

		g_piDevice->SetFVF(dw2);
		}

	// Now render the log display in the upper right corner of the window
	CLogDisplay::Instance().OnRender();

	// present the backbuffer
	g_piDevice->EndScene();
	g_piDevice->Present(NULL,NULL,NULL,NULL);

	// don't remove this, problems on some older machines (AMD timing bug)
	Sleep(10);
	return 1;
	}
};






