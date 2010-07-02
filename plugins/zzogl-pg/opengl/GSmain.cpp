/*	ZeroGS KOSMOS
 *	Copyright (C) 2005-2006 zerofrog@gmail.com
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA
 */
#if defined(_WIN32)
#include <windows.h>
#include "Win32.h"
#include <io.h>
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <list>
#include <vector>
#include <map>
#include <string>

using namespace std;

#include "GS.h"
#include "Mem.h"
#include "Regs.h"
#include "Profile.h"

#include "zerogs.h"
#include "targets.h"
#include "ZeroGSShaders/zerogsshaders.h"

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

GLWindow GLWin;
GSinternal gs;
char GStitle[256];
GSconf conf;

int ppf, g_GSMultiThreaded, CurrentSavestate = 0;
int g_LastCRC = 0, g_TransferredToGPU = 0, s_frameskipping = 0;

int UPDATE_FRAMES = 16, g_nFrame = 0, g_nRealFrame = 0;
float fFPS = 0;

void (*GSirq)();
u8* g_pBasePS2Mem = NULL;
std::string s_strIniPath("inis/");  	// Air's new ini path (r2361)

bool SaveStateExists = true;		// We could not know save slot status before first change occured
const char* SaveStateFile = NULL;	// Name of SaveFile for access check.

extern const char* s_aa[5];
extern const char* s_naa[3];
extern const char* pbilinear[];
// statistics
u32 g_nGenVars = 0, g_nTexVars = 0, g_nAlphaVars = 0, g_nResolve = 0;

#define VER 1
const unsigned char zgsversion	= PS2E_GS_VERSION;
unsigned char zgsrevision = 0; // revision and build gives plugin version
unsigned char zgsbuild	= VER;
unsigned char zgsminor = 0;

#ifdef _DEBUG
char *libraryName	 = "ZZ Ogl PG (Debug) ";
#elif defined(ZEROGS_DEVBUILD)
char *libraryName	 = "ZZ Ogl PG (Dev)";
#else
char *libraryName	 = "ZZ Ogl PG ";
#endif

extern int g_nPixelShaderVer, g_nFrameRender, g_nFramesSkipped;

extern void ProcessMessages();
extern void WriteAA();
extern void WriteBilinear();

extern int VALIDATE_THRESH;
extern u32 TEXDESTROY_THRESH;

#ifdef _WIN32
HWND GShwnd = NULL;
#endif

u32 THR_KeyEvent = 0; // Value for key event processing between threads
bool THR_bShift = false;


u32 CALLBACK PS2EgetLibType()
{
	return PS2E_LT_GS;
}

char* CALLBACK PS2EgetLibName()
{
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type)
{
	return (zgsversion << 16) | (zgsrevision << 8) | zgsbuild | (zgsminor << 24);
}

void CALLBACK GSsetBaseMem(void* pmem)
{
	g_pBasePS2Mem = (u8*)pmem;
}

void CALLBACK GSsetSettingsDir(const char* dir)
{
	s_strIniPath = (dir == NULL) ? "inis/" : dir;
}

void CALLBACK GSsetLogDir(const char* dir)
{
	ZZLog::SetDir(dir);
}

void CALLBACK GSsetGameCRC(int crc, int options)
{
	// TEXDESTROY_THRESH starts out at 16.
	VALIDATE_THRESH = 8;
	conf.mrtdepth = (conf.settings().disable_mrt_depth != 0);

	if (!conf.mrtdepth)
		ZZLog::Error_Log("Disabling MRT depth writing.");
	else
		ZZLog::Error_Log("Enabling MRT depth writing.");

	bool CRCValueChanged = (g_LastCRC != crc);

	g_LastCRC = crc;

	ZZLog::Error_Log("CRC = %x", crc);

	if (CRCValueChanged && (crc != 0))
	{
		for (int i = 0; i < GAME_INFO_INDEX; i++)
		{
			if (crc_game_list[i].crc == crc)
			{
				if (crc_game_list[i].v_thresh > 0) VALIDATE_THRESH = crc_game_list[i].v_thresh;
				if (crc_game_list[i].t_thresh > 0) TEXDESTROY_THRESH = crc_game_list[i].t_thresh;

				conf.def_hacks._u32 |= crc_game_list[i].flags;

				ZZLog::Error_Log("Found CRC[%x] in crc game list.", crc);

				return;
			}
		}
	}
}

void CALLBACK GSsetFrameSkip(int frameskip)
{
	FUNCLOG
	s_frameskipping |= frameskip;

	if (frameskip && g_nFrameRender > 1)
	{
		SetFrameSkip(true);
	}
	else if (!frameskip && g_nFrameRender <= 0)
	{
		SetFrameSkip(false);
	}
}

void CALLBACK GSreset()
{
	ZeroGS::GSReset();
}

void CALLBACK GSgifSoftReset(u32 mask)
{
	ZeroGS::GSSoftReset(mask);
}

s32 CALLBACK GSinit()
{
	FUNCLOG

    if (ZZLog::Open() == false) return -1;
	ZZLog::WriteLn("Calling GSinit.");

	WriteTempRegs();
	GSreset();
	
	ZZLog::WriteLn("GSinit finished.");
	return 0;
}

#ifdef _WIN32

#ifdef _DEBUG
HANDLE g_hCurrentThread = NULL;
#endif


extern LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern HINSTANCE hInst;
#endif


s32 CALLBACK GSopen(void *pDsp, char *Title, int multithread)
{
	FUNCLOG

	bool err;

	g_GSMultiThreaded = multithread;

	ZZLog::WriteLn("Calling GSopen.");

#ifdef _WIN32
#ifdef _DEBUG
	g_hCurrentThread = GetCurrentThread();
#endif
#endif

	LoadConfig();
	strcpy(GStitle, Title);
	
	err = GLWin.CreateWindow(pDsp);
	if (!err)
	{
		ZZLog::Error_Log("Failed to create window. Exiting...");
		return -1;
	}

	ZZLog::GS_Log("Using %s:%d.%d.%d.", libraryName, zgsrevision, zgsbuild, zgsminor);
	ZZLog::WriteLn("Creating ZZOgl window.");

	if (!ZeroGS::Create(conf.width, conf.height)) return -1;

	ZZLog::WriteLn("Initialization successful.");

	WriteBilinear();
	WriteAA();
	InitProfile();
	InitPath();
	ResetRegs();
	ZZLog::GS_Log("GSopen finished.");
	return 0;
}

void CALLBACK GSshutdown()
{
	FUNCLOG

	ZZLog::Close();
}

void CALLBACK GSclose()
{
	FUNCLOG

	ZeroGS::Destroy(1);

	GLWin.CloseWindow();

	SaveStateFile = NULL;
	SaveStateExists = true; // default value
}

void CALLBACK GSirqCallback(void (*callback)())
{
	FUNCLOG

	GSirq = callback;
}

void CALLBACK GSwriteCSR(u32 write)
{
	FUNCLOG

	gs.CSRw = write;
}

void CALLBACK GSchangeSaveState(int newstate, const char* filename)
{
	FUNCLOG

	char str[255];
	sprintf(str, "save state %d", newstate);
	ZeroGS::AddMessage(str);
	CurrentSavestate = newstate;

	SaveStateFile = filename;
	SaveStateExists = (access(SaveStateFile, 0) == 0);
}

void CALLBACK GSmakeSnapshot(char *path)
{
	FUNCLOG

	FILE *bmpfile;
	char filename[256];
	u32 snapshotnr = 0;

	// increment snapshot value & try to get filename

	for (;;)
	{
		snapshotnr++;

		sprintf(filename, "%s/snap%03ld.%s", path, snapshotnr, (conf.zz_options.tga_snap) ? "bmp" : "jpg");

		bmpfile = fopen(filename, "rb");

		if (bmpfile == NULL) break;

		fclose(bmpfile);
	}

	// try opening new snapshot file
	if ((bmpfile = fopen(filename, "wb")) == NULL)
	{
		char strdir[255];

#ifdef _WIN32
		sprintf(strdir, "%s", path);
		CreateDirectory(strdir, NULL);
#else
		sprintf(strdir, "mkdir %s", path);
		system(strdir);
#endif

		if ((bmpfile = fopen(filename, "wb")) == NULL) return;
	}

	fclose(bmpfile);

	// get the bits
	ZeroGS::SaveSnapshot(filename);
}

// I'll probably move this somewhere else later, but it's got a ton of dependencies.
static __forceinline void SetGSTitle()
{
	char strtitle[256];

#if !defined(ZEROGS_DEVBUILD)
	const char* g_pShaders[4] = { "full", "reduced", "accurate", "accurate-reduced" };
	const char* g_pInterlace[3] = { "interlace 0 |", "interlace 1 |", "" };
	const char* g_pBilinear[3] = { "", "bilinear |", "forced bilinear |" };

	if (SaveStateFile != NULL && !SaveStateExists)
		SaveStateExists = (access(SaveStateFile, 0) == 0);
	else
		SaveStateExists = true;

	sprintf(strtitle, "ZZ Open GL 0.%d.%d | %.1f fps | %s%s%s savestate %d%s | shaders %s | (%.1f)", zgsbuild, zgsminor, fFPS,
			g_pInterlace[conf.interlace], g_pBilinear[conf.bilinear],
			(conf.aa >= conf.negaa) ? (conf.aa ? s_aa[conf.aa - conf.negaa] : "") : (conf.negaa ? s_naa[conf.negaa - conf.aa] : ""),
					CurrentSavestate, (SaveStateExists ? "" :  "*"),
					g_pShaders[g_nPixelShaderVer], (ppf&0xfffff) / (float)UPDATE_FRAMES);

#else
	sprintf(strtitle, "%d | %.1f fps (sk:%d%%) | g: %.1f, t: %.1f, a: %.1f, r: %.1f | p: %.1f | tex: %d %d (%d kbpf)", g_nFrame, fFPS,
			100*g_nFramesSkipped / g_nFrame,
			g_nGenVars / (float)UPDATE_FRAMES, g_nTexVars / (float)UPDATE_FRAMES, g_nAlphaVars / (float)UPDATE_FRAMES,
			g_nResolve / (float)UPDATE_FRAMES, (ppf&0xfffff) / (float)UPDATE_FRAMES,
			ZeroGS::g_MemTargs.listTargets.size(), ZeroGS::g_MemTargs.listClearedTargets.size(), g_TransferredToGPU >> 10);

	//_snprintf(strtitle, 512, "%x %x", *(int*)(g_pbyGSMemory + 256 * 0x3e0c + 4), *(int*)(g_pbyGSMemory + 256 * 0x3e04 + 4));
#endif

//	if( g_nFrame > 100 && fFPS > 60.0f ) {
//		ZZLog::Debug_Log("Set profile.");
//		g_bWriteProfile = 1;
//	}
	if (!(conf.fullscreen())) GLWin.SetTitle(strtitle);
}

void CALLBACK GSvsync(int interlace)
{
	FUNCLOG

	//ZZLog::GS_Log("Calling GSvsync.");

	static u32 dwTime = timeGetTime();
	static int nToNextUpdate = 1;

	GL_REPORT_ERRORD();

	g_nRealFrame++;

	// !interlace? Hmmm... Fixme.
	ZeroGS::RenderCRTC(!interlace);

	ProcessMessages();

	if (--nToNextUpdate <= 0)
	{
		u32 d = timeGetTime();
		fFPS = UPDATE_FRAMES * 1000.0f / (float)max(d - dwTime, 1);
		dwTime = d;
		g_nFrame += UPDATE_FRAMES;
		SetGSTitle();

//		if( g_nFrame > 100 && fFPS > 60.0f ) {
//			ZZLog::Debug_Log("Set profile.");
//			g_bWriteProfile = 1;
//		}

		if (fFPS < 16) 
			UPDATE_FRAMES = 4;
		else if (fFPS < 32) 
			UPDATE_FRAMES = 8;
		else 
			UPDATE_FRAMES = 16;

		nToNextUpdate = UPDATE_FRAMES;

		ppf = 0;
		g_TransferredToGPU = 0;
		g_nGenVars = 0;
		g_nTexVars = 0;
		g_nAlphaVars = 0;
		g_nResolve = 0;
		g_nFramesSkipped = 0;
	}

#if defined(ZEROGS_DEVBUILD)
	if (g_bWriteProfile)
	{
		//g_bWriteProfile = 0;
		DVProfWrite("prof.txt", UPDATE_FRAMES);
		DVProfClear();
	}

#endif
	GL_REPORT_ERRORD();
}

void CALLBACK GSreadFIFO(u64 *pMem)
{
	FUNCLOG

	//ZZLog::GS_Log("Calling GSreadFIFO.");

	ZeroGS::TransferLocalHost((u32*)pMem, 1);
}

void CALLBACK GSreadFIFO2(u64 *pMem, int qwc)
{
	FUNCLOG

	//ZZLog::GS_Log("Calling GSreadFIFO2.");

	ZeroGS::TransferLocalHost((u32*)pMem, qwc);
}

int CALLBACK GSsetupRecording(int start, void* pData)
{
	FUNCLOG

	if (start)
		ZeroGS::StartCapture();
	else
		ZeroGS::StopCapture();

	return 1;
}

s32 CALLBACK GSfreeze(int mode, freezeData *data)
{
	FUNCLOG

	switch (mode)
	{
		case FREEZE_LOAD:
			if (!ZeroGS::Load(data->data)) ZZLog::Error_Log("GS: Bad load format!");
			g_nRealFrame += 100;
			break;

		case FREEZE_SAVE:
			ZeroGS::Save(data->data);
			break;

		case FREEZE_SIZE:
			data->size = ZeroGS::Save(NULL);
			break;

		default:
			break;
	}

	return 0;
}
