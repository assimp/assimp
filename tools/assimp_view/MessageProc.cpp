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

// Static array to keep custom color values
COLORREF g_aclCustomColors[16] =
	{0};


//-------------------------------------------------------------------------------
// Setup file associations for all formats supported by the library
//
// File associations are registered in HKCU\Software\Classes. They might
// be overwritten by global file associations.
//-------------------------------------------------------------------------------
void MakeFileAssociations()
	{
	/*
	; .wscript
	root: HKCR; Flags: deletekey; Subkey: ".uscript";  ValueType: string; ValueData: "UE_WSCRIPT_CLASS"; Components: rt
	root: HKCR; Flags: deletekey; Subkey: "UE_WSCRIPT_CLASS";  ValueName:; ValueType: string; ValueData: "UtopicEngine Console Script"; Components: rt
	root: HKCR; Flags: deletekey; Subkey: "UE_WSCRIPT_CLASS\shell\open\command";   ValueName:; ValueType: string; ValueData: "notepad.exe %1"; Components: rt
	*/
	char szTemp2[MAX_PATH];
	char szTemp[MAX_PATH + 10];

	GetModuleFileName(NULL,szTemp2,MAX_PATH);
	sprintf(szTemp,"%s %%1",szTemp2);

	HKEY hTemp;

	// ------------------------------------------------- 
	// .3ds
	// -------------------------------------------------
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\.3ds",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)"AV3DSCLASS",(DWORD)strlen("AV3DSCLASS")+1);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AV3DSCLASS",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AV3DSCLASS\\shell\\open\\command",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)szTemp,(DWORD)strlen(szTemp)+1);
	RegCloseKey(hTemp);

	// ------------------------------------------------- 
	// .x
	// -------------------------------------------------
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\.x",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)"AVXCLASS",(DWORD)strlen("AVXCLASS")+1);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVXCLASS",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVXCLASS\\shell\\open\\command",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)szTemp,(DWORD)strlen(szTemp)+1);
	RegCloseKey(hTemp);

	// ------------------------------------------------- 
	// .obj
	// -------------------------------------------------
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\.obj",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)"AVOBJCLASS",(DWORD)strlen("AVOBJCLASS")+1);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVOBJCLASS",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVOBJCLASS\\shell\\open\\command",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)szTemp,(DWORD)strlen(szTemp)+1);
	RegCloseKey(hTemp);

	// ------------------------------------------------- 
	// .ms3d
	// -------------------------------------------------
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\.ms3d",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)"AVMS3DCLASS",(DWORD)strlen("AVMS3DCLASS")+1);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVMS3DCLASS",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVMS3DCLASS\\shell\\open\\command",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)szTemp,(DWORD)strlen(szTemp)+1);
	RegCloseKey(hTemp);

	// ------------------------------------------------- 
	// .md3
	// -------------------------------------------------
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\.md3",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)"AVMD3CLASS",(DWORD)strlen("AVMD3CLASS")+1);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVMD3CLASS",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVMD3CLASS\\shell\\open\\command",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)szTemp,(DWORD)strlen(szTemp)+1);
	RegCloseKey(hTemp);

	// ------------------------------------------------- 
	// .md2
	// -------------------------------------------------
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\.md2",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)"AVMD2CLASS",(DWORD)strlen("AVMD2CLASS")+1);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVMD3CLASS",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVMD2CLASS\\shell\\open\\command",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)szTemp,(DWORD)strlen(szTemp)+1);
	RegCloseKey(hTemp);

	// ------------------------------------------------- 
	// .md4
	// -------------------------------------------------
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\.md4",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)"AVMD4CLASS",(DWORD)strlen("AVMD4CLASS")+1);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVMD4CLASS",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVMD4CLASS\\shell\\open\\command",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)szTemp,(DWORD)strlen(szTemp)+1);
	RegCloseKey(hTemp);

	// ------------------------------------------------- 
	// .md5
	// -------------------------------------------------
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\.md5",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)"AVMD5CLASS",(DWORD)strlen("AVMD5CLASS")+1);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVMD5CLASS",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVMD5CLASS\\shell\\open\\command",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)szTemp,(DWORD)strlen(szTemp)+1);
	RegCloseKey(hTemp);

	// ------------------------------------------------- 
	// .ply
	// -------------------------------------------------
	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\.ply",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)"AVPLYCLASS",(DWORD)strlen("AVPLYCLASS")+1);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVPLYCLASS",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegCloseKey(hTemp);

	RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Classes\\AVPLYCLASS\\shell\\open\\command",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	RegSetValueEx(hTemp,"",0,REG_SZ,(const BYTE*)szTemp,(DWORD)strlen(szTemp)+1);
	RegCloseKey(hTemp);

	CLogDisplay::Instance().AddEntry("[OK] File assocations have been registered",
		D3DCOLOR_ARGB(0xFF,0,0xFF,0));
	}



//-------------------------------------------------------------------------------
// Recreate all specular materials depending on the current specularity settings
//
// Diffuse-only materials are ignored.
// Must be called after specular highlights have been toggled
//-------------------------------------------------------------------------------
void UpdateSpecularMaterials()
	{
	if (g_pcAsset && g_pcAsset->pcScene)
		{
		for (unsigned int i = 0; i < g_pcAsset->pcScene->mNumMeshes;++i)
			{
			if (aiShadingMode_Phong == g_pcAsset->apcMeshes[i]->eShadingMode)
				{
				DeleteMaterial(g_pcAsset->apcMeshes[i]);
				CreateMaterial(g_pcAsset->apcMeshes[i],g_pcAsset->pcScene->mMeshes[i]);
				}
			}
		}
	}

//-------------------------------------------------------------------------------
// Handle command line parameters
//
// The function loads an asset specified on the command line as first argument
// Other command line parameters are not handled
//-------------------------------------------------------------------------------
void HandleCommandLine(char* p_szCommand)
	{
	char* sz = p_szCommand;
	//bool bQuak = false;

	if (strlen(sz) < 2)return;

	if (*sz == '\"')
		{
		char* sz2 = strrchr(sz,'\"');
		if (sz2)*sz2 = 0;
		}
	strcpy( g_szFileName, sz );
	LoadAsset();
	}

//-------------------------------------------------------------------------------
// Main message procedure of the application
//
// The function handles all incoming messages for the main window.
// However, if does not directly process input commands. 
// NOTE: Due to the impossibility to process WM_CHAR messages in dialogs
// properly the code for all hotkeys has been moved to the WndMain
//-------------------------------------------------------------------------------
INT_PTR CALLBACK MessageProc(HWND hwndDlg,UINT uMsg,
	WPARAM wParam,LPARAM lParam)
	{
	UNREFERENCED_PARAMETER(lParam);
	UNREFERENCED_PARAMETER(wParam);

	int xPos,yPos;
	int xPos2,yPos2;
	int fHalfX;
	int fHalfY;

	TRACKMOUSEEVENT sEvent;
	switch (uMsg)
		{
		case WM_INITDIALOG:

			CheckDlgButton(hwndDlg,IDC_TOGGLEMS,BST_CHECKED);
			CheckDlgButton(hwndDlg,IDC_ZOOM,BST_CHECKED);
			CheckDlgButton(hwndDlg,IDC_AUTOROTATE,BST_CHECKED);

			SetDlgItemText(hwndDlg,IDC_EVERT,"0");
			SetDlgItemText(hwndDlg,IDC_EFACE,"0");
			SetDlgItemText(hwndDlg,IDC_EMAT,"0");
			SetDlgItemText(hwndDlg,IDC_ESHADER,"0");
			return TRUE;

		case WM_MOUSEWHEEL:

			if (!g_bFPSView)
				{
				g_sCamera.vPos.z += GET_WHEEL_DELTA_WPARAM(wParam) / 50.0f;
				}
			else
				{
				g_sCamera.vPos += (GET_WHEEL_DELTA_WPARAM(wParam) / 50.0f) *
					g_sCamera.vLookAt.Normalize();
				}
			return TRUE;

		case WM_MOUSELEAVE:

			g_bMousePressed = false;
			g_bMousePressedR = false;
			g_bMousePressedM = false;
			g_bMousePressedBoth = false;
			return TRUE;

		case WM_LBUTTONDBLCLK:

			CheckDlgButton(hwndDlg,IDC_AUTOROTATE,
				IsDlgButtonChecked(hwndDlg,IDC_AUTOROTATE) == BST_CHECKED 
				? BST_UNCHECKED : BST_CHECKED);

			g_sOptions.bRotate = !g_sOptions.bRotate;
			return TRUE;


		case WM_CLOSE:
			PostQuitMessage(0);
			DestroyWindow(hwndDlg);
			return TRUE;

		case WM_LBUTTONDOWN:
			g_bMousePressed = true;

			// register a mouse track handler to be sure we'll know
			// when the mouse leaves the display view again
			sEvent.cbSize = sizeof(TRACKMOUSEEVENT);
			sEvent.dwFlags = TME_LEAVE;
			sEvent.hwndTrack = g_hDlg;
			sEvent.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&sEvent);

			if (g_bMousePressedR)
				{
				g_bMousePressed = false;
				g_bMousePressedR = false;
				g_bMousePressedBoth = true;
				return TRUE;
				}

			// need to determine the position of the mouse and the 
			// distance from the center
			xPos = (int)(short)LOWORD(lParam); 
			yPos = (int)(short)HIWORD(lParam); 
			xPos -= 10;
			yPos -= 10;
			xPos2 = xPos-3;
			yPos2 = yPos-5;

			RECT sRect;
			GetWindowRect(GetDlgItem(g_hDlg,IDC_RT),&sRect);
			sRect.right -= sRect.left;
			sRect.bottom -= sRect.top;

			// if the mouse klick was inside the viewer panel
			// give the focus to it
			if (xPos > 0 && xPos < sRect.right && yPos > 0 && yPos < sRect.bottom)
				{
				SetFocus(GetDlgItem(g_hDlg,IDC_RT));
				}

			// g_bInvert stores whether the mouse has started on the negative
			// x or on the positive x axis of the imaginary coordinate system
			// with origin p at the center of the HUD texture
			xPos -= sRect.right/2;
			yPos -= sRect.bottom/2;

			if (xPos > 0)g_bInvert = true;
			else g_bInvert = false;

			D3DSURFACE_DESC sDesc;
			g_pcTexture->GetLevelDesc(0,&sDesc);
	
			fHalfX = (int)(((float)sRect.right-(float)sDesc.Width) / 2.0f);
			fHalfY = (int)(((float)sRect.bottom-(float)sDesc.Height) / 2.0f);

			// Determine the input operation to perform for this position
			g_eClick = EClickPos_Outside;
 			if (xPos2 >= fHalfX && xPos2 < fHalfX + (int)sDesc.Width &&
				yPos2 >= fHalfY && yPos2 < fHalfY + (int)sDesc.Height &&
				NULL != g_szImageMask)
				{
				// inside the texture. Lookup the grayscale value from it
				xPos2 -= fHalfX;
				yPos2 -= fHalfY;

				unsigned char chValue = g_szImageMask[xPos2 + yPos2 * sDesc.Width];
				if (chValue > 0xFF-20)
					{
					g_eClick = EClickPos_Circle;
					}
				else if (chValue < 0xFF-20 && chValue > 185)
					{
					g_eClick = EClickPos_CircleHor;
					}
				else if (chValue > 0x10 && chValue < 185)
					{
					g_eClick = EClickPos_CircleVert;
					}
				}

			// OLD version of this code. Not using a texture lookup to
			// determine the exact position, but using maths and the fact
			// that we have a circle f(x) = m +rx ;-)
#if 0
			g_eClick = EClickPos_Circle;
			if (yPos < 10 && yPos > -10)
				{
				if ((xPos2 > fHalfX-5 && xPos2 < fHalfX+15) ||
					(xPos2 > fHalfX+(int)sDesc.Width-10 && xPos2 < fHalfX+(int)sDesc.Width+10))
					{
					g_eClick = EClickPos_CircleHor;
					}
				}
			else if (xPos < 10 && xPos > -10)
				{
				if ((yPos2 > fHalfY-5 && yPos2 < fHalfY+15) ||
					(yPos2 > fHalfY+(int)sDesc.Height-10 && yPos2 < fHalfY+(int)sDesc.Height+10))
					{
					g_eClick = EClickPos_CircleVert;
					}
				}
			else if (sqrtf((float)(xPos * xPos  + yPos * yPos)) > (float)(sDesc.Width/2))
				{
				g_eClick = EClickPos_Outside;
				}
#endif

			return TRUE;

		case WM_RBUTTONDOWN:
			g_bMousePressedR = true;

			sEvent.cbSize = sizeof(TRACKMOUSEEVENT);
			sEvent.dwFlags = TME_LEAVE;
			sEvent.hwndTrack = g_hDlg;
			sEvent.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&sEvent);

			if (g_bMousePressed)
				{
				g_bMousePressedR = false;
				g_bMousePressed = false;
				g_bMousePressedBoth = true;
				}

			return TRUE;

		case WM_MBUTTONDOWN:

			g_bMousePressedM = true;

			sEvent.cbSize = sizeof(TRACKMOUSEEVENT);
			sEvent.dwFlags = TME_LEAVE;
			sEvent.hwndTrack = g_hDlg;
			sEvent.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&sEvent);
			return TRUE;

		case WM_LBUTTONUP:
			g_bMousePressed = false;
			g_bMousePressedBoth = false;
			return TRUE;

		case WM_RBUTTONUP:
			g_bMousePressedR = false;
			g_bMousePressedBoth = false;
			return TRUE;

		case WM_MBUTTONUP:
			g_bMousePressedM = false;
			return TRUE;

		case WM_COMMAND:

			if (ID_VIEWER_QUIT == LOWORD(wParam))
				{
				PostQuitMessage(0);
				DestroyWindow(hwndDlg);
				}
			else if (ID_VIEWER_RESETVIEW == LOWORD(wParam))
				{
				g_sCamera.vPos = aiVector3D(0.0f,0.0f,-10.0f);
				g_sCamera.vLookAt = aiVector3D(0.0f,0.0f,1.0f);
				g_sCamera.vUp = aiVector3D(0.0f,1.0f,0.0f);
				g_sCamera.vRight = aiVector3D(0.0f,1.0f,0.0f);
				g_mWorldRotate = aiMatrix4x4();
				g_mWorld = aiMatrix4x4();

				// don't forget to reset the st
				CBackgroundPainter::Instance().ResetSB();
				}
			else if (ID__HELP == LOWORD(wParam))
				{
				DialogBox(g_hInstance,MAKEINTRESOURCE(IDD_AVHELP),
					hwndDlg,&HelpDialogProc);
				}
			else if (ID__ABOUT == LOWORD(wParam))
				{
				DialogBox(g_hInstance,MAKEINTRESOURCE(IDD_ABOUTBOX),
					hwndDlg,&AboutMessageProc);
				}
			else if (ID_VIEWER_H == LOWORD(wParam))
				{
				MakeFileAssociations();
				}
			else if (ID_BACKGROUND_CLEAR == LOWORD(wParam))
				{
				D3DCOLOR clrColor = D3DCOLOR_ARGB(0xFF,100,100,100);
				CBackgroundPainter::Instance().SetColor(clrColor);

				HKEY hTemp;
				RegCreateKeyEx(HKEY_CURRENT_USER,
					"Software\\ASSIMP\\Viewer",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
				RegSetValueExA(hTemp,"LastSkyBoxSrc",0,REG_SZ,(const BYTE*)"",MAX_PATH);
				RegSetValueExA(hTemp,"LastTextureSrc",0,REG_SZ,(const BYTE*)"",MAX_PATH);

				RegSetValueExA(hTemp,"Color",0,REG_DWORD,(const BYTE*)&clrColor,4);
				RegCloseKey(hTemp);
				}
			else if (ID_BACKGROUND_SETCOLOR == LOWORD(wParam))
				{
				HKEY hTemp;
				RegCreateKeyEx(HKEY_CURRENT_USER,
					"Software\\ASSIMP\\Viewer",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
				RegSetValueExA(hTemp,"LastSkyBoxSrc",0,REG_SZ,(const BYTE*)"",MAX_PATH);
				RegSetValueExA(hTemp,"LastTextureSrc",0,REG_SZ,(const BYTE*)"",MAX_PATH);

				CHOOSECOLOR clr;
				clr.lStructSize = sizeof(CHOOSECOLOR);
				clr.hwndOwner = hwndDlg;
				clr.Flags = CC_RGBINIT | CC_FULLOPEN;
				clr.rgbResult = RGB(100,100,100);
				clr.lpCustColors = g_aclCustomColors;
				clr.lpfnHook = NULL;
				clr.lpTemplateName = NULL;
				clr.lCustData = NULL;

				ChooseColor(&clr);

				D3DCOLOR clrColor = D3DCOLOR_ARGB(0xFF,
					GetRValue(clr.rgbResult),
					GetGValue(clr.rgbResult),
					GetBValue(clr.rgbResult));
				CBackgroundPainter::Instance().SetColor(clrColor);

				RegSetValueExA(hTemp,"Color",0,REG_DWORD,(const BYTE*)&clrColor,4);
				RegCloseKey(hTemp);
				}
			else if (ID_BACKGROUND_LOADTEXTURE == LOWORD(wParam))
				{
				char szFileName[MAX_PATH];

				DWORD dwTemp = MAX_PATH;
				HKEY hTemp;
				RegCreateKeyEx(HKEY_CURRENT_USER,
					"Software\\ASSIMP\\Viewer",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
				if(ERROR_SUCCESS != RegQueryValueEx(hTemp,"TextureSrc",NULL,NULL,
					(BYTE*)szFileName,&dwTemp))
					{
					// Key was not found. Use C:
					strcpy(szFileName,"");
					}
				else
					{
					// need to remove the file name
					char* sz = strrchr(szFileName,'\\');
					if (!sz)sz = strrchr(szFileName,'/');
					if (!sz)*sz = 0;
					}
				OPENFILENAME sFilename1 = {
					sizeof(OPENFILENAME),
					g_hDlg,GetModuleHandle(NULL), 
					"Textures\0*.png;*.dds;*.tga;*.bmp;*.tif;*.ppm;*.ppx;*.jpg;*.jpeg;*.exr\0*.*\0", NULL, 0, 1, 
					szFileName, MAX_PATH, NULL, 0, NULL, 
					"Open texture as background",
					OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_NOCHANGEDIR, 
					0, 1, ".jpg", 0, NULL, NULL
					};
				if(GetOpenFileName(&sFilename1) == 0) return TRUE;

				// Now store the file in the registry
				RegSetValueExA(hTemp,"TextureSrc",0,REG_SZ,(const BYTE*)szFileName,MAX_PATH);
				RegSetValueExA(hTemp,"LastTextureSrc",0,REG_SZ,(const BYTE*)szFileName,MAX_PATH);
				RegSetValueExA(hTemp,"LastSkyBoxSrc",0,REG_SZ,(const BYTE*)"",MAX_PATH);
				RegCloseKey(hTemp);

				CBackgroundPainter::Instance().SetTextureBG(szFileName);
				}
			else if (ID_BACKGROUND_LOADSKYBOX == LOWORD(wParam))
				{
				char szFileName[MAX_PATH];

				DWORD dwTemp = MAX_PATH;
				HKEY hTemp;
				RegCreateKeyEx(HKEY_CURRENT_USER,
					"Software\\ASSIMP\\Viewer",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
				if(ERROR_SUCCESS != RegQueryValueEx(hTemp,"SkyBoxSrc",NULL,NULL,
					(BYTE*)szFileName,&dwTemp))
					{
					// Key was not found. Use C:
					strcpy(szFileName,"");
					}
				else
					{
					// need to remove the file name
					char* sz = strrchr(szFileName,'\\');
					if (!sz)sz = strrchr(szFileName,'/');
					if (!sz)*sz = 0;
					}
				OPENFILENAME sFilename1 = {
					sizeof(OPENFILENAME),
					g_hDlg,GetModuleHandle(NULL), 
					"Skyboxes\0*.dds\0*.*\0", NULL, 0, 1, 
					szFileName, MAX_PATH, NULL, 0, NULL, 
					"Open skybox as background",
					OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_NOCHANGEDIR, 
					0, 1, ".dds", 0, NULL, NULL
					};
				if(GetOpenFileName(&sFilename1) == 0) return TRUE;

				// Now store the file in the registry
				RegSetValueExA(hTemp,"SkyBoxSrc",0,REG_SZ,(const BYTE*)szFileName,MAX_PATH);
				RegSetValueExA(hTemp,"LastSkyBoxSrc",0,REG_SZ,(const BYTE*)szFileName,MAX_PATH);
				RegSetValueExA(hTemp,"LastTextureSrc",0,REG_SZ,(const BYTE*)"",MAX_PATH);
				RegCloseKey(hTemp);

				CBackgroundPainter::Instance().SetCubeMapBG(szFileName);
				}
			else if (ID_VIEWER_SAVESCREENSHOTTOFILE == LOWORD(wParam))
				{
				char szFileName[MAX_PATH];

				DWORD dwTemp = MAX_PATH;
				HKEY hTemp;
				RegCreateKeyEx(HKEY_CURRENT_USER,
					"Software\\ASSIMP\\Viewer",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
				if(ERROR_SUCCESS != RegQueryValueEx(hTemp,"ScreenShot",NULL,NULL,
					(BYTE*)szFileName,&dwTemp))
					{
					// Key was not found. Use C:
					strcpy(szFileName,"");
					}
				else
					{
					// need to remove the file name
					char* sz = strrchr(szFileName,'\\');
					if (!sz)sz = strrchr(szFileName,'/');
					if (!sz)*sz = 0;
					}
				OPENFILENAME sFilename1 = {
					sizeof(OPENFILENAME),
					g_hDlg,GetModuleHandle(NULL), 
					"PNG Images\0*.png", NULL, 0, 1, 
					szFileName, MAX_PATH, NULL, 0, NULL, 
					"Save Screenshot to file",
					OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_NOCHANGEDIR, 
					0, 1, ".png", 0, NULL, NULL
					};
				if(GetSaveFileName(&sFilename1) == 0) return TRUE;

				// Now store the file in the registry
				RegSetValueExA(hTemp,"ScreenShot",0,REG_SZ,(const BYTE*)szFileName,MAX_PATH);
				RegCloseKey(hTemp);

				IDirect3DSurface9* pi;
				g_piDevice->GetRenderTarget(0,&pi);
				D3DXSaveSurfaceToFile(szFileName,D3DXIFF_PNG,
					pi,NULL,NULL);
				pi->Release();
				}
			else if (ID_VIEWER_OPEN == LOWORD(wParam))
				{
				char szFileName[MAX_PATH];

				DWORD dwTemp = MAX_PATH;
				HKEY hTemp;
				RegCreateKeyEx(HKEY_CURRENT_USER,
					"Software\\ASSIMP\\Viewer",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
				if(ERROR_SUCCESS != RegQueryValueEx(hTemp,"CurrentApp",NULL,NULL,
					(BYTE*)szFileName,&dwTemp))
					{
					// Key was not found. Use C:
					strcpy(szFileName,"");
					}
				else
					{
					// need to remove the file name
					char* sz = strrchr(szFileName,'\\');
					if (!sz)sz = strrchr(szFileName,'/');
					if (!sz)*sz = 0;
					}
				OPENFILENAME sFilename1 = {
					sizeof(OPENFILENAME),
					g_hDlg,GetModuleHandle(NULL), 
					"ASSIMP assets\0*.x;*.obj;*.ms3d;*.3ds;*.md3;*.md1;*.md2;*.md4;*.md5;*.ply\0All files\0*.*", NULL, 0, 1, 
					szFileName, MAX_PATH, NULL, 0, NULL, 
					"Import Asset into ASSIMP",
					OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_NOCHANGEDIR, 
					0, 1, ".x", 0, NULL, NULL
					};
				if(GetOpenFileName(&sFilename1) == 0) return TRUE;

				// Now store the file in the registry
				RegSetValueExA(hTemp,"CurrentApp",0,REG_SZ,(const BYTE*)szFileName,MAX_PATH);
				RegCloseKey(hTemp);

				if (0 != strcmp(g_szFileName,szFileName))
					{
					strcpy(g_szFileName, szFileName);
					DeleteAssetData();
					DeleteAsset();
					LoadAsset();
					}

				}
			else if (ID_VIEWER_CLOSEASSET == LOWORD(wParam))
				{
				DeleteAssetData();
				DeleteAsset();
				}
			else if (BN_CLICKED == HIWORD(wParam))
				{
				if (IDC_TOGGLEMS == LOWORD(wParam))
					{
					g_sOptions.bMultiSample = !g_sOptions.bMultiSample; 
					DeleteAssetData();
					ShutdownDevice();
					if (0 == CreateDevice())
						{
						CLogDisplay::Instance().AddEntry(
							"[ERROR] Failed to toggle MultiSampling mode");
						g_sOptions.bMultiSample = !g_sOptions.bMultiSample;
						CreateDevice();
						}
					CreateAssetData();

					if (g_sOptions.bMultiSample)
						{
						CLogDisplay::Instance().AddEntry(
							"[OK] Changed MultiSampling mode to the maximum value for this device");
						}
					else
						{
						CLogDisplay::Instance().AddEntry(
							"[OK] MultiSampling has been disabled");
						}
					}
				else if (IDC_TOGGLEMAT == LOWORD(wParam))
					{
					g_sOptions.bRenderMats = !g_sOptions.bRenderMats; 
					}
				else if (IDC_NOSPECULAR == LOWORD(wParam))
					{
					g_sOptions.bNoSpecular = !g_sOptions.bNoSpecular;
					UpdateSpecularMaterials();
					}
				else if (IDC_ZOOM == LOWORD(wParam))
					{
					g_bFPSView = !g_bFPSView;

					SetupFPSView();
					}
				else if (IDC_TOGGLENORMALS == LOWORD(wParam))
					{
					g_sOptions.bRenderNormals = !g_sOptions.bRenderNormals; 
					}
				else if (IDC_LOWQUALITY == LOWORD(wParam))
					{
					g_sOptions.bLowQuality = !g_sOptions.bLowQuality; 
					}
				else if (IDC_3LIGHTS == LOWORD(wParam))
					{
					g_sOptions.b3Lights = !g_sOptions.b3Lights; 
					}
				else if (IDC_LIGHTROTATE == LOWORD(wParam))
					{
					g_sOptions.bLightRotate = !g_sOptions.bLightRotate; 
					}
				else if (IDC_AUTOROTATE == LOWORD(wParam))
					{
					g_sOptions.bRotate = !g_sOptions.bRotate; 
					}
				else if (IDC_TOGGLEWIRE == LOWORD(wParam))
					{
					if (g_sOptions.eDrawMode == RenderOptions::WIREFRAME)
						g_sOptions.eDrawMode = RenderOptions::NORMAL;
					else g_sOptions.eDrawMode = RenderOptions::WIREFRAME;
					}
				}

			return TRUE;
		};
	return FALSE;
	}


//-------------------------------------------------------------------------------
// Message prcoedure for the progress dialog
//-------------------------------------------------------------------------------
INT_PTR CALLBACK ProgressMessageProc(HWND hwndDlg,UINT uMsg,
	 WPARAM wParam,LPARAM lParam)
	{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
		{
		case WM_INITDIALOG:

			SendDlgItemMessage(hwndDlg,IDC_PROGRESS,PBM_SETRANGE,0,
				MAKELPARAM(0,500));

			SetTimer(hwndDlg,0,40,NULL);
			return TRUE;

		case WM_CLOSE:
			EndDialog(hwndDlg,0);
			return TRUE;

		case WM_COMMAND:

			if (IDOK == LOWORD(wParam))
				{
#if 0
				g_bLoadingCanceled = true;
				TerminateThread(g_hThreadHandle,5);
				g_pcAsset = NULL;

				EndDialog(hwndDlg,0);
#endif

				// PROBLEM: If we terminate the loader thread, ASSIMP's state
				// is undefined. Any further attempts to load assets will
				// fail.
				exit(5);
//				return TRUE;
				}
		case WM_TIMER:

			UINT iPos = (UINT)SendDlgItemMessage(hwndDlg,IDC_PROGRESS,PBM_GETPOS,0,0);
			iPos += 10;
			if (iPos > 490)iPos = 0;
			SendDlgItemMessage(hwndDlg,IDC_PROGRESS,PBM_SETPOS,iPos,0);

			if (g_bLoadingFinished)
				{
				EndDialog(hwndDlg,0);
				return TRUE;
				}

			return TRUE;
		}
	return FALSE;
	}


//-------------------------------------------------------------------------------
// Message procedure for the about dialog
//-------------------------------------------------------------------------------
INT_PTR CALLBACK AboutMessageProc(HWND hwndDlg,UINT uMsg,
    WPARAM wParam,LPARAM lParam)
	{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
		{
		case WM_CLOSE:
			EndDialog(hwndDlg,0);
			return TRUE;

		case WM_COMMAND:

			if (IDOK == LOWORD(wParam))
				{
				EndDialog(hwndDlg,0);
				return TRUE;
				}
		}
	return FALSE;
	}
};

using namespace AssimpView;

//-------------------------------------------------------------------------------
// Entry point to the application
//-------------------------------------------------------------------------------
int APIENTRY _tWinMain(HINSTANCE hInstance,
					   HINSTANCE hPrevInstance,
					   LPTSTR    lpCmdLine,
					   int       nCmdShow)
	{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// needed for the RichEdit control in the about/help dialog
	LoadLibrary( "riched20.dll" );

	InitCommonControls();

	g_hInstance = hInstance;
	if (0 == InitD3D())
		{
		MessageBox(NULL,"Failed to initialize Direct3D 9",
			"ASSIMP ModelViewer",MB_OK);
		return -6;
		}

	HWND hDlg = CreateDialog(hInstance,MAKEINTRESOURCE(IDD_DIALOGMAIN),
		NULL,&MessageProc);

	if (NULL == hDlg)
		{
		MessageBox(NULL,"Failed to create dialog from resource",
			"ASSIMP ModelViewer",MB_OK);
		return -5;
		}

	g_hDlg = hDlg;

	MSG uMsg;
	memset(&uMsg,0,sizeof( MSG));

	ShowWindow( hDlg, nCmdShow );
	UpdateWindow( hDlg );

	if (0 == CreateDevice(true,false,true))
		{
		MessageBox(NULL,"Failed to initialize Direct3D 9 (2)",
			"ASSIMP ModelViewer",MB_OK);
		return -4;
		}

	CLogDisplay::Instance().AddEntry("[OK] The viewer has been initialized successfully");

	// recover background skyboxes/textures from the last session
	HKEY hTemp;
	union
		{
		char szFileName[MAX_PATH];
		D3DCOLOR clrColor;
		};
	DWORD dwTemp = MAX_PATH;
	RegCreateKeyEx(HKEY_CURRENT_USER,
		"Software\\ASSIMP\\Viewer",NULL,NULL,0,KEY_ALL_ACCESS, NULL, &hTemp,NULL);
	if(ERROR_SUCCESS == RegQueryValueEx(hTemp,"LastSkyBoxSrc",NULL,NULL,
		(BYTE*)szFileName,&dwTemp) && '\0' != szFileName[0])
		{
		CBackgroundPainter::Instance().SetCubeMapBG(szFileName);
		}
	else if(ERROR_SUCCESS == RegQueryValueEx(hTemp,"LastTextureSrc",NULL,NULL,
		(BYTE*)szFileName,&dwTemp) && '\0' != szFileName[0])
		{
		CBackgroundPainter::Instance().SetTextureBG(szFileName);
		}
	else if(ERROR_SUCCESS == RegQueryValueEx(hTemp,"Color",NULL,NULL,
		(BYTE*)&clrColor,&dwTemp))
		{
		CBackgroundPainter::Instance().SetColor(clrColor);
		}
	RegCloseKey(hTemp);

	// now handle command line arguments
	HandleCommandLine(lpCmdLine);


	double adLast[30];
	for (int i = 0; i < 30;++i)adLast[i] = 0.0f;
	int iCurrent = 0;

	double g_dCurTime = 0;
	double g_dLastTime = 0;
	while( uMsg.message != WM_QUIT )
		{
		if( PeekMessage( &uMsg, NULL, 0, 0, PM_REMOVE ) )
			{ 
			TranslateMessage( &uMsg );
			DispatchMessage( &uMsg );

			if (WM_CHAR == uMsg.message)
				{

				switch ((char)uMsg.wParam)
					{
					case 'M':
					case 'm':

						CheckDlgButton(g_hDlg,IDC_TOGGLEMS,
							IsDlgButtonChecked(g_hDlg,IDC_TOGGLEMS) == BST_CHECKED 
							? BST_UNCHECKED : BST_CHECKED);

						g_sOptions.bMultiSample = !g_sOptions.bMultiSample; 
						DeleteAssetData();
						ShutdownDevice();
						if (0 == CreateDevice())
							{
							CLogDisplay::Instance().AddEntry(
								"[ERROR] Failed to toggle MultiSampling mode");
							g_sOptions.bMultiSample = !g_sOptions.bMultiSample;
							CreateDevice();
							}
						CreateAssetData();

						if (g_sOptions.bMultiSample)
							{
							CLogDisplay::Instance().AddEntry(
								"[OK] Changed MultiSampling mode to the maximum value for this device");
							}
						else
							{
							CLogDisplay::Instance().AddEntry(
								"[OK] MultiSampling has been disabled");
							}
						break;

					case 'L':
					case 'l':

						CheckDlgButton(g_hDlg,IDC_3LIGHTS,
							IsDlgButtonChecked(g_hDlg,IDC_3LIGHTS) == BST_CHECKED 
							? BST_UNCHECKED : BST_CHECKED);

						g_sOptions.b3Lights = !g_sOptions.b3Lights;

						break;

					case 'P':
					case 'p':

						CheckDlgButton(g_hDlg,IDC_LOWQUALITY,
							IsDlgButtonChecked(g_hDlg,IDC_LOWQUALITY) == BST_CHECKED 
							? BST_UNCHECKED : BST_CHECKED);

						g_sOptions.bLowQuality = !g_sOptions.bLowQuality;

						break;

					case 'D':
					case 'd':

						CheckDlgButton(g_hDlg,IDC_TOGGLEMAT,
							IsDlgButtonChecked(g_hDlg,IDC_TOGGLEMAT) == BST_CHECKED 
							? BST_UNCHECKED : BST_CHECKED);

						g_sOptions.bRenderMats = !g_sOptions.bRenderMats;

						break;


					case 'N':
					case 'n':

						CheckDlgButton(g_hDlg,IDC_TOGGLENORMALS,
							IsDlgButtonChecked(g_hDlg,IDC_TOGGLENORMALS) == BST_CHECKED 
							? BST_UNCHECKED : BST_CHECKED);

						g_sOptions.bRenderNormals = !g_sOptions.bRenderNormals;

						break;


					case 'S':
					case 's':

						CheckDlgButton(g_hDlg,IDC_NOSPECULAR,
							IsDlgButtonChecked(g_hDlg,IDC_NOSPECULAR) == BST_CHECKED 
							? BST_UNCHECKED : BST_CHECKED);

						g_sOptions.bNoSpecular = !g_sOptions.bNoSpecular;
						UpdateSpecularMaterials();

						break;

					case 'A':
					case 'a':

						CheckDlgButton(g_hDlg,IDC_AUTOROTATE,
							IsDlgButtonChecked(g_hDlg,IDC_AUTOROTATE) == BST_CHECKED 
							? BST_UNCHECKED : BST_CHECKED);

						g_sOptions.bRotate = !g_sOptions.bRotate;

						break;


					case 'R':
					case 'r':

						CheckDlgButton(g_hDlg,IDC_LIGHTROTATE,
							IsDlgButtonChecked(g_hDlg,IDC_LIGHTROTATE) == BST_CHECKED 
							? BST_UNCHECKED : BST_CHECKED);

						g_sOptions.bLightRotate = !g_sOptions.bLightRotate;

						break;

					case 'Z':
					case 'z':

						CheckDlgButton(g_hDlg,IDC_ZOOM,
							IsDlgButtonChecked(g_hDlg,IDC_ZOOM) == BST_CHECKED 
							? BST_UNCHECKED : BST_CHECKED);

						g_bFPSView = !g_bFPSView;
						SetupFPSView();
						break;


					case 'W':
					case 'w':

						CheckDlgButton(g_hDlg,IDC_TOGGLEWIRE,
							IsDlgButtonChecked(g_hDlg,IDC_TOGGLEWIRE) == BST_CHECKED 
							? BST_UNCHECKED : BST_CHECKED);

						if (g_sOptions.eDrawMode == RenderOptions::NORMAL)
							g_sOptions.eDrawMode = RenderOptions::WIREFRAME;
						else g_sOptions.eDrawMode = RenderOptions::NORMAL;

						break;
					}
				}
			}

		// render the scene
		Render();

		g_dCurTime     = timeGetTime();
		g_fElpasedTime = (float)((g_dCurTime - g_dLastTime) * 0.001);
		g_dLastTime    = g_dCurTime;

		adLast[iCurrent++] = 1.0f / g_fElpasedTime;

		double dFPS = 0.0;
		for (int i = 0;i < 30;++i)
			dFPS += adLast[i];
		dFPS /= 30.0;

		if (30 == iCurrent)
			{
			iCurrent = 0;
			if (dFPS != g_fFPS)
				{
				g_fFPS = dFPS;
				char szOut[256];

				sprintf(szOut,"%i",(int)floorf((float)dFPS+0.5f));
				SetDlgItemText(g_hDlg,IDC_EFPS,szOut);
				}
			}
		}
	DeleteAsset();
	ShutdownDevice();
	ShutdownD3D();
	return 0;
	}