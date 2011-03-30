
#include "PrecompiledHeader.h"
#include "Utilities/Console.h"
#include "MSWstuff.h"

#include <wx/msw/wrapwin.h>
#include <wx/dynlib.h>

#include <dwmapi.h>

typedef HRESULT WINAPI Fntype_DwmEnableMMCSS(DWORD enable);
typedef HRESULT WINAPI Fntype_DwmSetPresentParameters( HWND hwnd, DWM_PRESENT_PARAMETERS *pPresentParams );

static wxDynamicLibrary lib_dwmapi;

// This could potentially reduce lag while running in Aero,
// by telling the DWM the application requires
// multimedia-class scheduling for smooth display.
void pxDwm_Load()
{
	wxDoNotLogInThisScope please;

	// Version test is not needed since we're using LoadLibrary. --air

	/*OSVERSIONINFOEX info = {0};
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
		mask))*/

	lib_dwmapi.Load( L"dwmapi.dll" );
	if( !lib_dwmapi.IsLoaded() ) return;

	if( Fntype_DwmEnableMMCSS* pDwmEnableMMCSS = (Fntype_DwmEnableMMCSS*)lib_dwmapi.GetSymbol(L"DwmEnableMMCSS") )
	{
		Console.WriteLn( "[Dwm] Desktop Window Manager detected." );

		// Seems to set the Windows timer resolution to 10ms
		if(FAILED(pDwmEnableMMCSS(TRUE)))
			Console.WriteLn("[Dwm] DwmEnableMMCSS returned a failure code.");
	}
}

// wnd - this parameter should be the GS display panel or the top level frame that holds it (not
//       sure if it's supposed to be the actual gsPanel or the top level window/frame that the
//       panel belongs to)
//
void pxDwm_SetPresentParams( WXWidget wnd )
{
	if( !lib_dwmapi.IsLoaded() ) return;
	Fntype_DwmSetPresentParameters* pDwmSetPresentParameters = (Fntype_DwmSetPresentParameters*)lib_dwmapi.GetSymbol(L"DwmSetPresentParameters");

	if( pDwmSetPresentParameters == NULL ) return;

	DWM_PRESENT_PARAMETERS params;

	params.cbSize = sizeof(DWM_PRESENT_PARAMETERS);
	params.fQueue = FALSE;

	if(FAILED(pDwmSetPresentParameters( (HWND)wnd, &params )))
		Console.WriteLn("[Dwm] DwmSetPresentParameters returned a failure code.");

	//DwmSetDxFrameDuration(hMainWindow,1);
}

void pxDwm_Unload()
{
	lib_dwmapi.Unload();
}
