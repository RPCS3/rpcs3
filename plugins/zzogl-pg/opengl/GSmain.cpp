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

GSinternal gs;
char GStitle[256];
extern FILE *gsLog;
GSconf conf;
int ppf;
primInfo *prim;
int g_GSMultiThreaded = 0;
void (*GSirq)();
u8* g_pBasePS2Mem = NULL;
int g_TransferredToGPU = 0;
std::string s_strIniPath("inis/");  	// Air's new ini path (r2361)
std::string s_strLogPath("logs/");

int CurrentSavestate = 0;		// Number of SaveSlot. Default is 0
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

extern GIFRegHandler g_GIFPackedRegHandlers[], g_GIFRegHandlers[];
GIFRegHandler g_GIFTempRegHandlers[16] = {0};
extern int g_nPixelShaderVer, g_nFrameRender, g_nFramesSkipped;

int s_frameskipping = 0;

extern void ProcessMessages();
extern void WriteAA();
extern void WriteBilinear();

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

GLWindow GLWin;

#ifdef _WIN32
HWND GShwnd = NULL;
#endif

u32 THR_KeyEvent = 0; // Value for key event processing between threads
bool THR_bShift = false;

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
	// Get the path to the log directory.
	s_strLogPath = (dir==NULL) ? "logs/" : dir;

	// Reload the log file after updated the path
	if (gsLog != NULL) fclose(gsLog);
    ZZLog::OpenLog();
}

extern int VALIDATE_THRESH;
extern u32 TEXDESTROY_THRESH;

int g_LastCRC = 0;

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

//#define OLD_GS_SET_FRAMESKIP
#ifdef OLD_GS_SET_FRAMESKIP
void CALLBACK GSsetFrameSkip(int frameskip)
{
	FUNCLOG
	s_frameskipping |= frameskip;

	if (frameskip && g_nFrameRender > 1)
	{
		for (int i = 0; i < 16; ++i)
		{
			g_GIFPackedRegHandlers[i] = GIFPackedRegHandlerNOP;
		}

		// still keep certain handlers
		g_GIFPackedRegHandlers[6] = GIFRegHandlerTEX0_1;
		g_GIFPackedRegHandlers[7] = GIFRegHandlerTEX0_2;
		g_GIFPackedRegHandlers[14] = GIFPackedRegHandlerA_D;
		
		g_GIFRegHandlers[0] = GIFRegHandlerNOP;
		g_GIFRegHandlers[1] = GIFRegHandlerNOP;
		g_GIFRegHandlers[2] = GIFRegHandlerNOP;
		g_GIFRegHandlers[3] = GIFRegHandlerNOP;
		g_GIFRegHandlers[4] = GIFRegHandlerNOP;
		g_GIFRegHandlers[5] = GIFRegHandlerNOP;
		g_GIFRegHandlers[12] = GIFRegHandlerNOP;
		g_GIFRegHandlers[13] = GIFRegHandlerNOP;
		g_GIFRegHandlers[26] = GIFRegHandlerNOP;
		g_GIFRegHandlers[27] = GIFRegHandlerNOP;

		g_nFrameRender = 0;
	}
	else if (!frameskip && g_nFrameRender <= 0)
	{
		g_nFrameRender = 1;

		if (g_GIFTempRegHandlers[0] == NULL) return;  // not init yet

		// restore
		memcpy(g_GIFPackedRegHandlers, g_GIFTempRegHandlers, sizeof(g_GIFTempRegHandlers));

		g_GIFRegHandlers[0] = GIFRegHandlerPRIM;
		g_GIFRegHandlers[1] = GIFRegHandlerRGBAQ;
		g_GIFRegHandlers[2] = GIFRegHandlerST;
		g_GIFRegHandlers[3] = GIFRegHandlerUV;
		g_GIFRegHandlers[4] = GIFRegHandlerXYZF2;
		g_GIFRegHandlers[5] = GIFRegHandlerXYZ2;
		g_GIFRegHandlers[12] = GIFRegHandlerXYZF3;
		g_GIFRegHandlers[13] = GIFRegHandlerXYZ2;
		g_GIFRegHandlers[26] = GIFRegHandlerPRMODECONT;
		g_GIFRegHandlers[27] = GIFRegHandlerPRMODE;
	}
}
#else
void CALLBACK GSsetFrameSkip(int frameskip)
{
	FUNCLOG
	s_frameskipping |= frameskip;

	if (frameskip && g_nFrameRender > 1)
	{
		g_GIFPackedRegHandlers[GIF_REG_PRIM] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_RGBA] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_STQ] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_UV] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_XYZF2] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_XYZ2] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_FOG] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_XYZF3] = &GIFPackedRegHandlerNOP;
		g_GIFPackedRegHandlers[GIF_REG_XYZ3] = &GIFPackedRegHandlerNOP;

		g_GIFRegHandlers[GIF_A_D_REG_PRIM] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_ST] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_UV] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_XYZF2] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_XYZ2] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_XYZF3] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_XYZ3] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GIFRegHandlerNOP;
		g_GIFRegHandlers[GIF_A_D_REG_PRMODE] = &GIFRegHandlerNOP;
	}
	else if (!frameskip && g_nFrameRender <= 0)
	{
		g_GIFPackedRegHandlers[GIF_REG_PRIM] = &GIFPackedRegHandlerPRIM;
		g_GIFPackedRegHandlers[GIF_REG_RGBA] = &GIFPackedRegHandlerRGBA;
		g_GIFPackedRegHandlers[GIF_REG_STQ] = &GIFPackedRegHandlerSTQ;
		g_GIFPackedRegHandlers[GIF_REG_UV] = &GIFPackedRegHandlerUV;
		g_GIFPackedRegHandlers[GIF_REG_XYZF2] = &GIFPackedRegHandlerXYZF2;
		g_GIFPackedRegHandlers[GIF_REG_XYZ2] = &GIFPackedRegHandlerXYZ2;
		g_GIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GIFPackedRegHandlerCLAMP_1;
		g_GIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GIFPackedRegHandlerCLAMP_2;
		g_GIFPackedRegHandlers[GIF_REG_FOG] = &GIFPackedRegHandlerFOG;
		g_GIFPackedRegHandlers[GIF_REG_XYZF3] = &GIFPackedRegHandlerXYZF3;
		g_GIFPackedRegHandlers[GIF_REG_XYZ3] = &GIFPackedRegHandlerXYZ3;

		g_GIFRegHandlers[GIF_A_D_REG_PRIM] = &GIFRegHandlerPRIM;
		g_GIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GIFRegHandlerRGBAQ;
		g_GIFRegHandlers[GIF_A_D_REG_ST] = &GIFRegHandlerST;
		g_GIFRegHandlers[GIF_A_D_REG_UV] = &GIFRegHandlerUV;
		g_GIFRegHandlers[GIF_A_D_REG_XYZF2] = &GIFRegHandlerXYZF2;
		g_GIFRegHandlers[GIF_A_D_REG_XYZ2] = &GIFRegHandlerXYZ2;
		g_GIFRegHandlers[GIF_A_D_REG_XYZF3] = &GIFRegHandlerXYZF3;
		g_GIFRegHandlers[GIF_A_D_REG_XYZ3] = &GIFRegHandlerXYZ3;
		g_GIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GIFRegHandlerPRMODECONT;
		g_GIFRegHandlers[GIF_A_D_REG_PRMODE] = &GIFRegHandlerPRMODE;
	}
}
#endif
void CALLBACK GSreset()
{
	FUNCLOG

	memset(&gs, 0, sizeof(gs));

	ZeroGS::GSStateReset();

	gs.prac = 1;
	prim = &gs._prim[0];
	gs.nTriFanVert = -1;
	gs.imageTransfer = -1;
	gs.q = 1;
}

void CALLBACK GSgifSoftReset(u32 mask)
{
	FUNCLOG

	if (mask & 1) memset(&gs.path[0], 0, sizeof(gs.path[0]));
	if (mask & 2) memset(&gs.path[1], 0, sizeof(gs.path[1]));
	if (mask & 4) memset(&gs.path[2], 0, sizeof(gs.path[2]));

	gs.imageTransfer = -1;
	gs.q = 1;
	gs.nTriFanVert = -1;
}

s32 CALLBACK GSinit()
{
	FUNCLOG

	memcpy(g_GIFTempRegHandlers, g_GIFPackedRegHandlers, sizeof(g_GIFTempRegHandlers));

    if (ZZLog::OpenLog() == false)
			return -1;

	ZZLog::WriteLn("Calling GSinit.");

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

extern void ResetRegs();

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

	luPerfFreq = GetCPUTicks();

	gs.path[0].mode = gs.path[1].mode = gs.path[2].mode = 0;
	ResetRegs();
	ZZLog::GS_Log("GSopen finished.");

	return 0;
}

void CALLBACK GSshutdown()
{
	FUNCLOG

	if (gsLog != NULL) fclose(gsLog);
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

int UPDATE_FRAMES = 16;
int g_nFrame = 0;
int g_nRealFrame = 0;

float fFPS = 0;

void CALLBACK GSvsync(int interlace)
{
	FUNCLOG

	//ZZLog::GS_Log("Calling GSvsync.");

	static u32 dwTime = timeGetTime();
	static int nToNextUpdate = 1;
	char strtitle[256];

	GL_REPORT_ERRORD();

	g_nRealFrame++;

	ZeroGS::RenderCRTC(!interlace);

	ProcessMessages();

	if (--nToNextUpdate <= 0)
	{

		u32 d = timeGetTime();
		fFPS = UPDATE_FRAMES * 1000.0f / (float)max(d - dwTime, 1);
		dwTime = d;
		g_nFrame += UPDATE_FRAMES;

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

//		if( g_nFrame > 100 && fFPS > 60.0f ) {
//			ZZLog::Debug_Log("Set profile.");
//			g_bWriteProfile = 1;
//		}
		if (!(conf.fullscreen())) GLWin.SetTitle(strtitle);

		if (fFPS < 16) 
			UPDATE_FRAMES = 4;
		else if (fFPS < 32) 
			UPDATE_FRAMES = 8;
		else 
			UPDATE_FRAMES = 16;

		nToNextUpdate = UPDATE_FRAMES;

		g_TransferredToGPU = 0;
		g_nGenVars = 0;
		g_nTexVars = 0;
		g_nAlphaVars = 0;
		g_nResolve = 0;

		ppf = 0;

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
	{
		if (conf.captureAvi()) return 1;

		ZeroGS::StartCapture();

		conf.setCaptureAvi(true);

		ZZLog::Warn_Log("Started recording zerogs.avi.");
	}
	else
	{
		if (!(conf.captureAvi())) return 1;

		conf.setCaptureAvi(false);

		ZeroGS::StopCapture();

		ZZLog::Warn_Log("Stopped recording.");
	}

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
