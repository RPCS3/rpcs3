#include "PrecompiledHeader.h"
#include <windows.h>
#include "Utilities/Console.h"

// This could potentially reduce lag while running in Aero,
// by telling the DWM the application requires
// multimedia-class scheduling for smooth display.
void SetupDwmStuff(WXHWND hMainWindow)
{
	HRESULT (WINAPI * pDwmEnableMMCSS)(DWORD);

	OSVERSIONINFOEX info = {0};
	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	info.dwMajorVersion = 6;

	DWORDLONG mask=0;
	VER_SET_CONDITION(mask,VER_MAJORVERSION,    VER_GREATER_EQUAL);
	VER_SET_CONDITION(mask,VER_MINORVERSION,    VER_GREATER_EQUAL);
	VER_SET_CONDITION(mask,VER_SERVICEPACKMAJOR,VER_GREATER_EQUAL);
	VER_SET_CONDITION(mask,VER_SERVICEPACKMINOR,VER_GREATER_EQUAL);

	//info
	if(VerifyVersionInfo(&info,
		VER_MAJORVERSION|VER_MINORVERSION|VER_SERVICEPACKMAJOR|VER_SERVICEPACKMINOR,
		mask))
	{
		HMODULE hDwmapi = LoadLibrary(wxT("dwmapi.dll"));
		if(hDwmapi)
		{
			pDwmEnableMMCSS = (HRESULT (WINAPI *)(DWORD))GetProcAddress(hDwmapi,"DwmEnableMMCSS");
			if(pDwmEnableMMCSS)
			{
				if(FAILED(pDwmEnableMMCSS(TRUE)))
				{
					Console.WriteLn("Warning: DwmEnableMMCSS returned a failure code (not an error).");
				}
				else
				{
					Console.WriteLn("DwmEnableMMCSS successful.");
				}
			}

			//DwmSetDxFrameDuration(hMainWindow,1);

			FreeLibrary(hDwmapi);
		}
	}
}