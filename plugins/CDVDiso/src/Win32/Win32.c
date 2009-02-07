#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdarg.h>
#include <direct.h>
#include <sys/stat.h>
#include <string.h>

#include "zlib/zlib.h"


#include "Config.h"
#include "CDVDiso.h"
#include "resource.h"

HINSTANCE hInst;
#define MAXFILENAME 256

u8 Zbuf[2352 * 10 * 2];
HWND hDlg;
HWND hProgress;
HWND hIsoFile;
HWND hMethod;
HWND hBlockDump;
int stop;

void SysMessage(char *fmt, ...)
{
	va_list list;
	char tmp[512];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);
	MessageBox(0, tmp, "CDVDiso Msg", 0);
}

int _GetFile(char *out)
{
	OPENFILENAME ofn;
	char szFileName[MAXFILENAME];
	char szFileTitle[MAXFILENAME];

	memset(&szFileName,  0, sizeof(szFileName));
	memset(&szFileTitle, 0, sizeof(szFileTitle));

	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= GetActiveWindow();
	ofn.lpstrFilter			=
	    "Supported Formats\0*.bin;*.iso;*.img;*.nrg;*.mdf;*.Z;*.Z2;*.BZ2;*.dump\0"
	    "Cd Iso Format (*.bin;*.iso;*.img;*.nrg;*.mdf)\0"
	    "*.bin;*.iso;*.img;*.nrg;*.mdf\0"
	    "Compressed Z Iso Format (*.Z;*.Z2)\0"
	    "*.Z;*.Z2\0Compressed BZ Iso Format (*.BZ2)\0"
	    "*.BZ2\0Block Dumps (*.dump)\0*.dump\0All Files\0*.*\0";

	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 1;
	ofn.lpstrFile			= szFileName;
	ofn.nMaxFile			= MAXFILENAME;
	ofn.lpstrInitialDir		= (IsoCWD[0] == 0) ? NULL : IsoCWD;
	ofn.lpstrFileTitle		= szFileTitle;
	ofn.nMaxFileTitle		= MAXFILENAME;
	ofn.lpstrTitle			= NULL;
	ofn.lpstrDefExt			= NULL;
	ofn.Flags				= OFN_HIDEREADONLY;

	if (GetOpenFileName((LPOPENFILENAME)&ofn))
	{
		strcpy(out, szFileName);
		return 1;
	}

	return 0;
}

int _OpenFile(int saveConf)
{

	int retval = 0;

	// for saving the pcsx2 current working directory;
	char* cwd_pcsx2 = _getcwd(NULL, MAXFILENAME);

	if (IsoCWD[0] != 0)
		_chdir(IsoCWD);

	if (_GetFile(IsoFile) == 1)
	{
		// Save the user's new cwd:
		if (_getcwd(IsoCWD, MAXFILENAME) == NULL)
			IsoCWD[0] = 0;

		if (saveConf)
			SaveConf();

		retval = 1;
	}

	// Restore Pcsx2's path.
	if (cwd_pcsx2 != NULL)
	{
		_chdir(cwd_pcsx2);
		free(cwd_pcsx2);
		cwd_pcsx2 = NULL;
	}
	return retval;
}

void CfgOpenFile()
{
	_OpenFile(TRUE);
}

void UpdZmode()
{
	if (ComboBox_GetCurSel(hMethod) == 0)
	{
		Zmode = 1;
	}
	else
	{
		Zmode = 2;
	}
}


void SysUpdate()
{
	MSG msg;

	while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void OnCompress()
{
	u32 lsn;
	u8 cdbuff[10*2352];
	char Zfile[256];
	int ret;
	isoFile *src;
	isoFile *dst;

	Edit_GetText(hIsoFile, IsoFile, 256);
	UpdZmode();

	if (Zmode == 1)
	{
		sprintf(Zfile, "%s.Z2", IsoFile);
	}
	else
	{
		sprintf(Zfile, "%s.BZ2", IsoFile);
	}

	src = isoOpen(IsoFile);
	if (src == NULL) return;
	if (Zmode == 1)
	{
		dst = isoCreate(Zfile, ISOFLAGS_Z2);
	}
	else
	{
		dst = isoCreate(Zfile, ISOFLAGS_BZ2);
	}
	if (dst == NULL) return;

	isoSetFormat(dst, src->blockofs, src->blocksize, src->blocks);
	Button_Enable(GetDlgItem(hDlg, IDC_COMPRESSISO), FALSE);
	Button_Enable(GetDlgItem(hDlg, IDC_DECOMPRESSISO), FALSE);
	stop = 0;

	for (lsn = 0; lsn < src->blocks; lsn++)
	{
		printf("block %d ", lsn);
		putchar(13);
		fflush(stdout);
		ret = isoReadBlock(src, cdbuff, lsn);
		if (ret == -1) break;
		ret = isoWriteBlock(dst, cdbuff, lsn);
		if (ret == -1) break;

		SendMessage(hProgress, PBM_SETPOS, (lsn * 100) / src->blocks, 0);
		SysUpdate();
		if (stop) break;
	}
	isoClose(src);
	isoClose(dst);

	if (!stop) Edit_SetText(hIsoFile, Zfile);

	Button_Enable(GetDlgItem(hDlg, IDC_COMPRESSISO), TRUE);
	Button_Enable(GetDlgItem(hDlg, IDC_DECOMPRESSISO), TRUE);

	if (!stop)
	{
		if (ret == -1)
		{
			SysMessage("Error compressing iso image");
		}
		else
		{
			SysMessage("Iso image compressed OK");
		}
	}
}

void OnDecompress()
{
	char file[256];
	u8 cdbuff[10*2352];
	u32 lsn;
	isoFile *src;
	isoFile *dst;
	int ret;

	Edit_GetText(hIsoFile, IsoFile, 256);

	src = isoOpen(IsoFile);
	if (src == NULL) return;

	strcpy(file, IsoFile);
	if (src->flags & ISOFLAGS_Z)
	{
		file[strlen(file) - 2] = 0;
	}
	else
		if (src->flags & ISOFLAGS_Z2)
		{
			file[strlen(file) - 3] = 0;
		}
		else
			if (src->flags & ISOFLAGS_BZ2)
			{
				file[strlen(file) - 3] = 0;
			}
			else
			{
				SysMessage("%s is not a compressed image", IsoFile);
				return;
			}

	dst = isoCreate(file, 0);
	if (dst == NULL) return;

	isoSetFormat(dst, src->blockofs, src->blocksize, src->blocks);
	Button_Enable(GetDlgItem(hDlg, IDC_COMPRESSISO), FALSE);
	Button_Enable(GetDlgItem(hDlg, IDC_DECOMPRESSISO), FALSE);
	stop = 0;

	for (lsn = 0; lsn < src->blocks; lsn++)
	{
		printf("block %d ", lsn);
		putchar(13);
		fflush(stdout);
		ret = isoReadBlock(src, cdbuff, lsn);
		if (ret == -1) break;
		ret = isoWriteBlock(dst, cdbuff, lsn);
		if (ret == -1) break;

		SendMessage(hProgress, PBM_SETPOS, (lsn * 100) / src->blocks, 0);
		SysUpdate();
		if (stop) break;
	}
	if (!stop) Edit_SetText(hIsoFile, file);

	isoClose(src);
	isoClose(dst);

	Button_Enable(GetDlgItem(hDlg, IDC_COMPRESSISO), TRUE);
	Button_Enable(GetDlgItem(hDlg, IDC_DECOMPRESSISO), TRUE);

	if (!stop)
	{
		if (ret == -1)
		{
			SysMessage("Error decompressing iso image");
		}
		else
		{
			SysMessage("Iso image decompressed OK");
		}
	}
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			hDlg = hW;
			LoadConf();

			hProgress  = GetDlgItem(hW, IDC_PROGRESS);
			hIsoFile   = GetDlgItem(hW, IDC_ISOFILE);
			hMethod    = GetDlgItem(hW, IDC_METHOD);
			hBlockDump = GetDlgItem(hW, IDC_BLOCKDUMP);

			for (i = 0; methods[i] != NULL; i++)
			{
				ComboBox_AddString(hMethod, methods[i]);
			}

			Edit_SetText(hIsoFile, IsoFile);
			ComboBox_SetCurSel(hMethod, 0);
			/*			if (strstr(IsoFile, ".Z") != NULL)
							 ComboBox_SetCurSel(hMethod, 1);
						else ComboBox_SetCurSel(hMethod, 0);*/
			Button_SetCheck(hBlockDump, BlockDump);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_SELECTISO:
					if (_OpenFile(FALSE) == 1)
						Edit_SetText(hIsoFile, IsoFile);
					return TRUE;

				case IDC_COMPRESSISO:
					OnCompress();
					return TRUE;

				case IDC_DECOMPRESSISO:
					OnDecompress();
					return TRUE;

				case IDC_STOP:
					stop = 1;
					return TRUE;

				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;

				case IDOK:
					Edit_GetText(hIsoFile, IsoFile, 256);
					BlockDump = Button_GetCheck(hBlockDump);

					SaveConf();
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

EXPORT_C(void) CDVDconfigure()
{
	DialogBox(hInst,
	          MAKEINTRESOURCE(IDD_CONFIG),
	          GetActiveWindow(),
	          (DLGPROC)ConfigureDlgProc);
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hW, TRUE);
					return FALSE;
			}
	}
	return FALSE;
}

EXPORT_C(void) CDVDabout()
{
	DialogBox(hInst,
	          MAKEINTRESOURCE(IDD_ABOUT),
	          GetActiveWindow(),
	          (DLGPROC)AboutDlgProc);
}

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason,
                      LPVOID lpReserved)
{
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}

