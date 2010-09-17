#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "resrc1.h"

#include "GS.h"
#include "ZZoglShaders.h"
#include "Win32.h"

#include <map>

using namespace std;

extern int g_nPixelShaderVer;
static int prevbilinearfilter;
HINSTANCE hInst = NULL;

void CALLBACK GSkeyEvent(keyEvent *ev)
{
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

map<int, int> mapConfOpts;
#define PUT_CONF(id) mapConfOpts[IDC_CONFOPT_##id] = 0x##id;

void OnAdvOK(HWND hW)
{
	conf.hacks._u32 = 0;

	for (map<int, int>::iterator it = mapConfOpts.begin(); it != mapConfOpts.end(); ++it)
	{
		if (IsDlgButtonChecked(hW, it->first)) conf.hacks._u32 |= it->second;
	}

	GSsetGameCRC(g_LastCRC, conf.hacks._u32);

	SaveConfig();

	EndDialog(hW, false);
}

void OnInitAdvDialog(HWND hW)
{
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

	for (map<int, int>::iterator it = mapConfOpts.begin(); it != mapConfOpts.end(); ++it)
	{
		CheckDlgButton(hW, it->first, (conf.settings()._u32 & it->second) ? 1 : 0);
	}
}

BOOL CALLBACK AdvancedDialogProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			OnInitAdvDialog(hW);
			return true;

		case WM_COMMAND:

			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hW, true);
					return true;

				case IDOK:
					OnAdvOK(hW);
					return true;
			}
	}

	return false;
}

void CALLBACK AdvancedDialog()
{
	DialogBox(hInst,
			  MAKEINTRESOURCE(IDD_ADV_OPTIONS),
			  GetActiveWindow(),
			  (DLGPROC)AdvancedDialogProc);
}

void OnInitConfDialog(HWND hW)
{
	if (!(conf.zz_options.loaded)) LoadConfig();

     TCHAR *aaName[] = {"None", "x2", "x4", "x8", "x16"};
 
     for(int i=0; i<5; i++)
	 {
		 ComboBox_AddString(GetDlgItem(hW, IDC_AA_COMBO), (LPARAM)aaName[i]);
	 }
	ComboBox_SelectString(GetDlgItem(hW, IDC_AA_COMBO), -1, (LPARAM)aaName[conf.aa]);

    TCHAR *sizeName[] = {"640 x 480", "800 x 600", "1024 x 768", "1280 x 960"};
 
    for(int i=0; i<4; i++)
	{
		ComboBox_AddString(GetDlgItem(hW, IDC_WIN_SIZE_COMBO), (LPARAM)sizeName[i]);
	}
	ComboBox_SelectString(GetDlgItem(hW, IDC_WIN_SIZE_COMBO), -1, (LPARAM)sizeName[conf.zz_options.dimensions]);

	CheckDlgButton(hW, IDC_CONFIG_INTERLACE, conf.interlace);
	CheckDlgButton(hW, IDC_CONFIG_BILINEAR, conf.bilinear);
	CheckDlgButton(hW, IDC_CONFIG_DEPTHWRITE, conf.mrtdepth);
	CheckDlgButton(hW, IDC_CONFIG_WIREFRAME, (conf.wireframe()) ? 1 : 0);
	CheckDlgButton(hW, IDC_CONFIG_CAPTUREAVI, (conf.captureAvi()) ? 1 : 0);
	CheckDlgButton(hW, IDC_CONFIG_FULLSCREEN, (conf.fullscreen()) ? 1 : 0);
	CheckDlgButton(hW, IDC_CONFIG_WIDESCREEN, (conf.widescreen()) ? 1 : 0);
	CheckDlgButton(hW, IDC_CONFIG_BMPSS, (conf.zz_options.tga_snap) ? 1 : 0);

	prevbilinearfilter = conf.bilinear;
}

void OnConfOK(HWND hW)
{
	u32 newinterlace = IsDlgButtonChecked(hW, IDC_CONFIG_INTERLACE);

	if (!conf.interlace) 
		conf.interlace = newinterlace;
	else if (!newinterlace) 
		conf.interlace = 2;  // off

	conf.bilinear = IsDlgButtonChecked(hW, IDC_CONFIG_BILINEAR);

	// restore
	if (conf.bilinear && prevbilinearfilter) conf.bilinear = prevbilinearfilter;

	if (ComboBox_GetCurSel(GetDlgItem(hW, IDC_AA_COMBO)) != -1)
		conf.aa = ComboBox_GetCurSel(GetDlgItem(hW, IDC_AA_COMBO));

	conf.zz_options._u32 = 0;

	conf.zz_options.capture_avi = IsDlgButtonChecked(hW, IDC_CONFIG_CAPTUREAVI) ? 1 : 0;
	conf.zz_options.wireframe = IsDlgButtonChecked(hW, IDC_CONFIG_WIREFRAME) ? 1 : 0;
	conf.zz_options.fullscreen = IsDlgButtonChecked(hW, IDC_CONFIG_FULLSCREEN) ? 1 : 0;
	conf.zz_options.widescreen = IsDlgButtonChecked(hW, IDC_CONFIG_WIDESCREEN) ? 1 : 0;
	conf.zz_options.tga_snap = IsDlgButtonChecked(hW, IDC_CONFIG_BMPSS) ? 1 : 0;

	if (ComboBox_GetCurSel(GetDlgItem(hW, IDC_WIN_SIZE_COMBO)) == 0) 
		conf.zz_options.dimensions = GSDim_640;
	else if (ComboBox_GetCurSel(GetDlgItem(hW, IDC_WIN_SIZE_COMBO)) == 1) 
		conf.zz_options.dimensions = GSDim_800;
	else if (ComboBox_GetCurSel(GetDlgItem(hW, IDC_WIN_SIZE_COMBO)) == 2) 
		conf.zz_options.dimensions = GSDim_1024;
	else if (ComboBox_GetCurSel(GetDlgItem(hW, IDC_WIN_SIZE_COMBO)) == 3) 
		conf.zz_options.dimensions = GSDim_1280;

	SaveConfig();

	EndDialog(hW, false);
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			OnInitConfDialog(hW);
			return true;

		case WM_COMMAND:

			switch (LOWORD(wParam))
			{
                case IDC_AA_COMBO: 
					break; 

				case IDC_ADV_BTN:
					AdvancedDialog();
					return true;

				case IDCANCEL:
					EndDialog(hW, true);
					return true;

				case IDOK:
					OnConfOK(hW);
					return true;
			}
	}

	return false;
}

void CALLBACK GSconfigure()
{
	DialogBox(hInst,
			  MAKEINTRESOURCE(IDD_CONFIG2),
			  GetActiveWindow(),
			  (DLGPROC)ConfigureDlgProc);

	if (g_nPixelShaderVer == SHADER_REDUCED) conf.bilinear = 0;
}

s32 CALLBACK GStest()
{
	return 0;
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hW, false);
					return true;
			}
	}

	return false;
}

void CALLBACK GSabout()
{
	DialogBox(hInst,
			  MAKEINTRESOURCE(IDD_ABOUT),
			  GetActiveWindow(),
			  (DLGPROC)AboutDlgProc);
}

bool APIENTRY DllMain(HANDLE hModule,				  // DLL INIT
					  DWORD  dwReason,
					  LPVOID lpReserved)
{
	hInst = (HINSTANCE)hModule;
	return true;										  // very quick :)
}

static char *err = "Error Loading Symbol";
static int errval;

void *SysLoadLibrary(char *lib)
{
	return LoadLibrary(lib);
}

void *SysLoadSym(void *lib, char *sym)
{
	void *tmp = GetProcAddress((HINSTANCE)lib, sym);

	if (tmp == NULL)
		errval = 1;
	else 
		errval = 0;

	return tmp;
}

char *SysLibError()
{
	if (errval) { errval = 0; return err; }

	return NULL;
}

void SysCloseLibrary(void *lib)
{
	FreeLibrary((HINSTANCE)lib);
}

void SysMessage(const char *fmt, ...)
{
	va_list list;
	char tmp[512];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);
	MessageBox(0, tmp, "ZZOgl-PG Msg", 0);
}
