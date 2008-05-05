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


/* extern */ CLogDisplay CLogDisplay::s_cInstance;

//-------------------------------------------------------------------------------
void CLogDisplay::AddEntry(const std::string& szText,
	const D3DCOLOR clrColor)
	{
	SEntry sNew;
	sNew.clrColor = clrColor;
	sNew.szText = szText;
	sNew.dwStartTicks = (DWORD)GetTickCount();

	this->asEntries.push_back(sNew);
	}

//-------------------------------------------------------------------------------
void CLogDisplay::ReleaseNativeResource()
	{
	if (this->piFont)
		{
		this->piFont->Release();
		this->piFont = NULL;
		}
	}

//-------------------------------------------------------------------------------
void CLogDisplay::RecreateNativeResource()
	{
	if (!this->piFont)
		{
		if (FAILED(D3DXCreateFont(g_piDevice,     
                     16,					//Font height
                     0,						//Font width
                     FW_BOLD,				//Font Weight
                     1,						//MipLevels
                     false,					//Italic
                     DEFAULT_CHARSET,		//CharSet
                     OUT_DEFAULT_PRECIS,	//OutputPrecision
					 CLEARTYPE_QUALITY,	//Quality
                     DEFAULT_PITCH|FF_DONTCARE,	//PitchAndFamily
                     "Verdana",					//pFacename,
					 &this->piFont)))
			{
			CLogDisplay::Instance().AddEntry("Unable to load font",D3DCOLOR_ARGB(0xFF,0xFF,0,0));

			this->piFont = NULL;
			return;
			}
		}
	return;
	}

//-------------------------------------------------------------------------------
void CLogDisplay::OnRender()
	{
	DWORD dwTick = (DWORD) GetTickCount();
	DWORD dwLimit = dwTick - 8000;
	DWORD dwLimit2 = dwLimit + 3000;

	unsigned int iCnt = 0;
	RECT sRect;
	sRect.left = 0;
	sRect.top = 10;
	
	RECT sWndRect;
	GetWindowRect(GetDlgItem(g_hDlg,IDC_RT),&sWndRect);
	sWndRect.right -= sWndRect.left;
	sWndRect.bottom -= sWndRect.top;
	sWndRect.left = sWndRect.top = 0;

	// if no asset is loaded draw a "no asset loaded" text in the center
	if (!g_pcAsset)
		{
		const char* szText = "No asset loaded\r\nUse [Viewer | Open asset] to load one";

		// shadow
		RECT sCopy;
		sCopy.left		= sWndRect.left+1;
		sCopy.top		= sWndRect.top+1;
		sCopy.bottom	= sWndRect.bottom+1;
		sCopy.right		= sWndRect.right+1;
		this->piFont->DrawText(NULL,szText ,
			-1,&sCopy,DT_CENTER | DT_VCENTER,D3DCOLOR_ARGB(100,0x0,0x0,0x0));
		sCopy.left		= sWndRect.left+1;
		sCopy.top		= sWndRect.top+1;
		sCopy.bottom	= sWndRect.bottom-1;
		sCopy.right		= sWndRect.right-1;
		this->piFont->DrawText(NULL,szText ,
			-1,&sCopy,DT_CENTER | DT_VCENTER,D3DCOLOR_ARGB(100,0x0,0x0,0x0));
		sCopy.left		= sWndRect.left-1;
		sCopy.top		= sWndRect.top-1;
		sCopy.bottom	= sWndRect.bottom+1;
		sCopy.right		= sWndRect.right+1;
		this->piFont->DrawText(NULL,szText ,
			-1,&sCopy,DT_CENTER | DT_VCENTER,D3DCOLOR_ARGB(100,0x0,0x0,0x0));
		sCopy.left		= sWndRect.left-1;
		sCopy.top		= sWndRect.top-1;
		sCopy.bottom	= sWndRect.bottom-1;
		sCopy.right		= sWndRect.right-1;
		this->piFont->DrawText(NULL,szText ,
			-1,&sCopy,DT_CENTER | DT_VCENTER,D3DCOLOR_ARGB(100,0x0,0x0,0x0));

		// text
		this->piFont->DrawText(NULL,szText ,
			-1,&sWndRect,DT_CENTER | DT_VCENTER,D3DCOLOR_ARGB(0xFF,0xFF,0xFF,0xFF));
		}

	sRect.right = sWndRect.right - 30;
	sRect.bottom = sWndRect.bottom;

	// update all elements in the queue and render them
	for (std::list<SEntry>::iterator
		i =  this->asEntries.begin();
		i != this->asEntries.end();++i,++iCnt)
		{
		if ((*i).dwStartTicks < dwLimit)
			{
			i = this->asEntries.erase(i);

			if(i == this->asEntries.end())break;
			}
		else if (NULL != this->piFont)
			{
			float fAlpha = 1.0f;
			if ((*i).dwStartTicks <= dwLimit2)
				{
				// linearly interpolate to create the fade out effect
				fAlpha = 1.0f - (float)(dwLimit2 - (*i).dwStartTicks) / 3000.0f;
				}
			D3DCOLOR& clrColor = (*i).clrColor;
			clrColor &= ~(0xFFu << 24);
			clrColor |= (((unsigned char)(fAlpha * 255.0f)) & 0xFFu) << 24;

			const char* szText = (*i).szText.c_str();
			if (sRect.top + 30 > sWndRect.bottom)
				{
				// end of window. send a special message
				szText = "... too many errors";
				clrColor = D3DCOLOR_ARGB(0xFF,0xFF,100,0x0);
				}

			// draw the black shadow
			RECT sCopy;
			sCopy.left		= sRect.left+1;
			sCopy.top		= sRect.top+1;
			sCopy.bottom	= sRect.bottom+1;
			sCopy.right		= sRect.right+1;
			this->piFont->DrawText(NULL,szText,
				-1,&sCopy,DT_RIGHT | DT_TOP,D3DCOLOR_ARGB(
				(unsigned char)(fAlpha * 100.0f),0x0,0x0,0x0));

			sCopy.left		= sRect.left-1;
			sCopy.top		= sRect.top-1;
			sCopy.bottom	= sRect.bottom-1;
			sCopy.right		= sRect.right-1;
			this->piFont->DrawText(NULL,szText,
				-1,&sCopy,DT_RIGHT | DT_TOP,D3DCOLOR_ARGB(
				(unsigned char)(fAlpha * 100.0f),0x0,0x0,0x0));

			sCopy.left		= sRect.left-1;
			sCopy.top		= sRect.top-1;
			sCopy.bottom	= sRect.bottom+1;
			sCopy.right		= sRect.right+1;
			this->piFont->DrawText(NULL,szText,
				-1,&sCopy,DT_RIGHT | DT_TOP,D3DCOLOR_ARGB(
				(unsigned char)(fAlpha * 100.0f),0x0,0x0,0x0));

			sCopy.left		= sRect.left+1;
			sCopy.top		= sRect.top+1;
			sCopy.bottom	= sRect.bottom-1;
			sCopy.right		= sRect.right-1;
			this->piFont->DrawText(NULL,szText,
				-1,&sCopy,DT_RIGHT | DT_TOP,D3DCOLOR_ARGB(
				(unsigned char)(fAlpha * 100.0f),0x0,0x0,0x0));

			// draw the text itself
			int iPX = this->piFont->DrawText(NULL,szText,
				-1,&sRect,DT_RIGHT | DT_TOP,clrColor);

			sRect.top += iPX;
			sRect.bottom += iPX;

			if (szText != (*i).szText.c_str())break;
			}
		}
	return;
	}
};