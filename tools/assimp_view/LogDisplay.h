//-------------------------------------------------------------------------------
/**
*	This program is distributed under the terms of the GNU Lesser General
*	Public License (LGPL). 
*
*	ASSIMP Viewer Utility
*
*/
//-------------------------------------------------------------------------------

#if (!defined AV_LOG_DISPLAY_H_INCLUDED)
#define AV_LOG_DISPLAY_H_INCLUDE

//-------------------------------------------------------------------------------
/**	\brief Class to display log strings in the upper right corner of the view
*/
//-------------------------------------------------------------------------------
class CLogDisplay
	{
private:

	CLogDisplay()  {}

public:

	// data structure for an entry in the log queue
	struct SEntry
		{
		SEntry ()
			:
			clrColor(D3DCOLOR_ARGB(0xFF,0xFF,0xFF,0x00)), dwStartTicks(0)
				{}

		std::string szText;
		D3DCOLOR clrColor;
		DWORD dwStartTicks;
		};

	// Singleton accessors
	static CLogDisplay s_cInstance;
	inline static CLogDisplay& Instance ()
		{
		return s_cInstance;
		}

	// Add an entry to the log queue
	void AddEntry(const std::string& szText,
		const D3DCOLOR clrColor = D3DCOLOR_ARGB(0xFF,0xFF,0xFF,0x00));

	// Release any native resources associated with the instance
	void ReleaseNativeResource();

	// Recreate any native resources associated with the instance
	void RecreateNativeResource();

	// Called during the render loop
	void OnRender();

private:

	std::list<SEntry> asEntries;
	ID3DXFont* piFont;

	};

#endif // AV_LOG_DISPLAY_H_INCLUDE