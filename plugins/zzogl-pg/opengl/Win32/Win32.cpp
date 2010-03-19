#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "resrc1.h"

#include "GS.h"
#include "Win32.h"

#include <map>
using namespace std;

extern int g_nPixelShaderVer;
static int prevbilinearfilter;
HINSTANCE hInst=NULL;

void CALLBACK GSkeyEvent(keyEvent *ev) {
//  switch (ev->event) {
//	  case KEYPRESS:
//		  switch (ev->key) {
//			  case VK_PRIOR:
//				  if (conf.fps) fpspos++; break;
//			  case VK_NEXT:
//				  if (conf.fps) fpspos--; break;
//			  case VK_END:
//				  if (conf.fps) fpspress = 1; break;
//			  case VK_DELETE:
//				  conf.fps = 1 - conf.fps;
//				  break;
//		  }
//		  break;
//  }
}

#include "Win32/resource.h"

BOOL CALLBACK LoggingDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			if (conf.log)		CheckDlgButton(hW, IDC_LOG, TRUE);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					EndDialog(hW, FALSE);
					return TRUE;
				case IDOK:
					if (IsDlgButtonChecked(hW, IDC_LOG))
						 conf.log = 1;
					else conf.log = 0;

					SaveConfig();
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

map<int, int> mapConfOpts;
#define PUT_CONF(id) mapConfOpts[IDC_CONFOPT_##id] = 0x##id;

void OnInitDialog(HWND hW)
{
	if( !(conf.options & GSOPTION_LOADED) )
		LoadConfig();

	CheckDlgButton(hW, IDC_CONFIG_INTERLACE, conf.interlace);
	CheckDlgButton(hW, IDC_CONFIG_BILINEAR, conf.bilinear);
	CheckDlgButton(hW, IDC_CONFIG_DEPTHWRITE, conf.mrtdepth);
	CheckRadioButton(hW,IDC_CONFIG_AANONE,IDC_CONFIG_AA4, IDC_CONFIG_AANONE+conf.aa);
	CheckDlgButton(hW, IDC_CONFIG_WIREFRAME, (conf.options&GSOPTION_WIREFRAME)?1:0);
	CheckDlgButton(hW, IDC_CONFIG_CAPTUREAVI, (conf.options&GSOPTION_CAPTUREAVI)?1:0);
	//CheckDlgButton(hW, IDC_CONFIG_CACHEFBP, (conf.options&GSOPTION_ALPHACLAMP)?1:0);
	CheckDlgButton(hW, IDC_CONFIG_FULLSCREEN, (conf.options&GSOPTION_FULLSCREEN)?1:0);
	CheckDlgButton(hW, IDC_CONFIG_WIDESCREEN, (conf.options&GSOPTION_WIDESCREEN)?1:0);
	//CheckDlgButton(hW, IDC_CONFIG_FFX, (conf.options&GSOPTION_FFXHACK)?1:0);
	CheckDlgButton(hW, IDC_CONFIG_BMPSS, (conf.options&GSOPTION_TGASNAP)?1:0);
	CheckRadioButton(hW,IDC_CONF_WIN640, IDC_CONF_WIN1280, IDC_CONF_WIN640+((conf.options&GSOPTION_WINDIMS)>>4));

	prevbilinearfilter = conf.bilinear;

	mapConfOpts.clear();
	PUT_CONF(00000001);
	PUT_CONF(00000002);
	PUT_CONF(00000004);
	PUT_CONF(00000008);
	PUT_CONF(00000010);
	PUT_CONF(00000020);
	PUT_CONF(00000040);
	PUT_CONF(00000080);
	PUT_CONF(00000100);
	PUT_CONF(00000200);
	PUT_CONF(00000400);
	PUT_CONF(00000800);
	PUT_CONF(00001000);
	PUT_CONF(00002000);
	PUT_CONF(00004000);
	PUT_CONF(00008000);
	PUT_CONF(00010000);
	PUT_CONF(00020000);
	PUT_CONF(00040000);
	PUT_CONF(00080000);
	PUT_CONF(00100000);
	PUT_CONF(00200000);
	PUT_CONF(00800000);
	PUT_CONF(01000000);
	PUT_CONF(02000000);
	PUT_CONF(04000000);
	PUT_CONF(10000000);

	for(map<int, int>::iterator it = mapConfOpts.begin(); it != mapConfOpts.end(); ++it) {
		CheckDlgButton(hW, it->first, (conf.gamesettings&it->second)?1:0);
	}
}

void OnOK(HWND hW) {
	
	u32 newinterlace = IsDlgButtonChecked(hW, IDC_CONFIG_INTERLACE);

	if( !conf.interlace ) conf.interlace = newinterlace;
	else if( !newinterlace ) conf.interlace = 2; // off

	conf.bilinear = IsDlgButtonChecked(hW, IDC_CONFIG_BILINEAR);
	// restore
	if( conf.bilinear && prevbilinearfilter )
		conf.bilinear = prevbilinearfilter;

	//conf.mrtdepth = 1;//IsDlgButtonChecked(hW, IDC_CONFIG_DEPTHWRITE);

	if( SendDlgItemMessage(hW,IDC_CONFIG_AANONE,BM_GETCHECK,0,0) ) {
		conf.aa = 0;
	}
	else if( SendDlgItemMessage(hW,IDC_CONFIG_AA2,BM_GETCHECK,0,0) ) {
		conf.aa = 1;
	}
	else if( SendDlgItemMessage(hW,IDC_CONFIG_AA4,BM_GETCHECK,0,0) ) {
		conf.aa = 2;
	}
	else if( SendDlgItemMessage(hW,IDC_CONFIG_AA8,BM_GETCHECK,0,0) ) {
		conf.aa = 3;
	}
	else if( SendDlgItemMessage(hW,IDC_CONFIG_AA16,BM_GETCHECK,0,0) ) {
		conf.aa = 4;
	}
	else conf.aa = 0;
	conf.negaa = 0;

	conf.options = 0;
	conf.options |= IsDlgButtonChecked(hW, IDC_CONFIG_CAPTUREAVI) ? GSOPTION_CAPTUREAVI : 0;
	conf.options |= IsDlgButtonChecked(hW, IDC_CONFIG_WIREFRAME) ? GSOPTION_WIREFRAME : 0;
	conf.options |= IsDlgButtonChecked(hW, IDC_CONFIG_FULLSCREEN) ? GSOPTION_FULLSCREEN : 0;
	conf.options |= IsDlgButtonChecked(hW, IDC_CONFIG_WIDESCREEN) ? GSOPTION_WIDESCREEN : 0;
	//conf.options |= IsDlgButtonChecked(hW, IDC_CONFIG_FFX) ? GSOPTION_FFXHACK : 0;
	conf.options |= IsDlgButtonChecked(hW, IDC_CONFIG_BMPSS) ? GSOPTION_TGASNAP : 0;

	conf.gamesettings = 0;
	for(map<int, int>::iterator it = mapConfOpts.begin(); it != mapConfOpts.end(); ++it) {
		if( IsDlgButtonChecked(hW, it->first) )
			conf.gamesettings |= it->second;
	}
	GSsetGameCRC(g_LastCRC, conf.gamesettings);

	if( SendDlgItemMessage(hW,IDC_CONF_WIN640,BM_GETCHECK,0,0) ) conf.options |= GSOPTION_WIN640;
	else if( SendDlgItemMessage(hW,IDC_CONF_WIN800,BM_GETCHECK,0,0) ) conf.options |= GSOPTION_WIN800;
	else if( SendDlgItemMessage(hW,IDC_CONF_WIN1024,BM_GETCHECK,0,0) ) conf.options |= GSOPTION_WIN1024;
	else if( SendDlgItemMessage(hW,IDC_CONF_WIN1280,BM_GETCHECK,0,0) ) conf.options |= GSOPTION_WIN1280;

	SaveConfig();
	EndDialog(hW, FALSE);
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			OnInitDialog(hW);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;
				case IDOK:
					OnOK(hW);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:

			//ZeroGS uses floating point render targets because A8R8G8B8 format is not sufficient for ps2 blending and this requires alpha blending on floating point render targets
			//There might be a problem with pixel shader precision with older geforce models (textures will look blocky).

			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

s32 CALLBACK GStest() {
	return 0;
}

void CALLBACK GSabout() {
	DialogBox(hInst,
			  MAKEINTRESOURCE(IDD_ABOUT),
			  GetActiveWindow(),  
			  (DLGPROC)AboutDlgProc);
}

BOOL APIENTRY DllMain(HANDLE hModule,				  // DLL INIT
					  DWORD  dwReason, 
					  LPVOID lpReserved) {
	hInst = (HINSTANCE)hModule;
	return TRUE;										  // very quick :)
}

static char *err = "Error Loading Symbol";
static int errval;

void *SysLoadLibrary(char *lib) {
	return LoadLibrary(lib);
}

void *SysLoadSym(void *lib, char *sym) {
	void *tmp = GetProcAddress((HINSTANCE)lib, sym);
	if (tmp == NULL) errval = 1;
	else errval = 0;
	return tmp;
}

char *SysLibError() {
	if (errval) { errval = 0; return err; }
	return NULL;
}

void SysCloseLibrary(void *lib) {
	FreeLibrary((HINSTANCE)lib);
}
