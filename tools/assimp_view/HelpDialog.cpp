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

#include "RICHEDIT.H"

namespace AssimpView {


//-------------------------------------------------------------------------------
// Message procedure for the help dialog
//-------------------------------------------------------------------------------
INT_PTR CALLBACK HelpDialogProc(HWND hwndDlg,UINT uMsg,
	WPARAM wParam,LPARAM lParam)
	{
	lParam;
	switch (uMsg)
		{
		case WM_INITDIALOG:
			{
			// load the help file ...
			HRSRC res = FindResource(NULL,MAKEINTRESOURCE(IDR_TEXT1),"TEXT");
			HGLOBAL hg = LoadResource(NULL,res);
			void* pData = LockResource(hg);

			SETTEXTEX sInfo;
			sInfo.flags = ST_DEFAULT;
			sInfo.codepage = CP_ACP;

			SendDlgItemMessage(hwndDlg,IDC_RICHEDIT21,
				EM_SETTEXTEX,(WPARAM)&sInfo,( LPARAM) pData);

			UnlockResource(hg);
			FreeResource(hg);
			return TRUE;
			}

		case WM_CLOSE:
			EndDialog(hwndDlg,0);
			return TRUE;

		case WM_COMMAND:

			if (IDOK == LOWORD(wParam))
				{
				EndDialog(hwndDlg,0);
				return TRUE;
				}

		case WM_PAINT:
			{
			PAINTSTRUCT sPaint;
			HDC hdc = BeginPaint(hwndDlg,&sPaint);

			HBRUSH hBrush = CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

			RECT sRect;
			sRect.left = 0;
			sRect.top = 26;
			sRect.right = 1000;
			sRect.bottom = 507;
			FillRect(hdc, &sRect, hBrush);

			EndPaint(hwndDlg,&sPaint);
			return TRUE;
			}
		};
	return FALSE;
	}

};