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
GSconf conf;
int ppf;
primInfo *prim;
FILE *gsLog;
int g_GSMultiThreaded = 0;
void (*GSirq)();
u8* g_pBasePS2Mem = NULL;
int g_TransferredToGPU = 0;
std::string s_strIniPath("inis/");  	// Air's new ini path (r2361)
std::string s_strLogPath("logs/");

int CurrentSavestate = 0;		// Number of SaveSlot. Default is 0
bool SaveStateExists = true;		// We could not know save slot status before first change occured
const char* SaveStateFile = NULL;	// Name of SaveFile for access check.

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

static const char* s_aa[5] = { "AA none |", "AA 2x |", "AA 4x |", "AA 8x |", "AA 16x |" };
static const char* s_naa[3] = { "native res |", "res /2 |", "res /4 |" };
static const char* pbilinear[] = { "off", "normal", "forced" };

extern GIFRegHandler g_GIFPackedRegHandlers[], g_GIFRegHandlers[];
GIFRegHandler g_GIFTempRegHandlers[16] = {0};
extern int g_nPixelShaderVer, g_nFrameRender, g_nFramesSkipped;

int s_frameskipping = 0;

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

namespace ZZLog
{
bool IsLogging()
{
	// gsLog can be null if the config dialog is used prior to Pcsx2 starting an emulation session.
	// (GSinit won't have been called then)
	return (gsLog != NULL && conf.log);
}

bool OpenLog() {
    bool result = true;
    const std::string LogFile(s_strLogPath + "GSzzogl.log");

    gsLog = fopen(LogFile.c_str(), "w");
    if (gsLog != NULL)
        setvbuf(gsLog, NULL,  _IONBF, 0);
    else {
        SysMessage("Can't create log file %s\n", LogFile.c_str());
        result = false;
    }

    return result;
}

void WriteToScreen(const char* pstr, u32 ms)
{
	ZeroGS::AddMessage(pstr, ms);
}

void _Message(const char *str)
{
	SysMessage(str);
}

void _Log(const char *str)
{
	if (IsLogging()) fprintf(gsLog, str);
}

void _WriteToConsole(const char *str)
{
	fprintf(stderr,"ZZogl-PG: %s", str);
}

void _Print(const char *str)
{
	fprintf(stderr,"ZZogl-PG: %s", str);

	if (IsLogging()) fprintf(gsLog, str);
}

void Message(const char *fmt, ...)
{
	va_list list;
	char tmp[512];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);

	SysMessage(tmp);
}

void Log(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (IsLogging()) vfprintf(gsLog, fmt, list);

	va_end(list);
}

void WriteToConsole(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	fprintf(stderr, "ZZogl-PG: ");
	vfprintf(stderr, fmt, list);
	va_end(list);
}

void Print(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (IsLogging()) vfprintf(gsLog, fmt, list);
	
	fprintf(stderr, "ZZogl-PG: ");
	vfprintf(stderr, fmt, list);

	va_end(list);
}


void WriteLn(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (IsLogging()) vfprintf(gsLog, fmt, list);
	
	fprintf(stderr, "ZZogl-PG: ");
	vfprintf(stderr, fmt, list);
	va_end(list);
	fprintf(stderr,"\n");
}

void Greg_Log(const char *fmt, ...)
{
	// Not currently used
#if 0
	va_list list;
	char tmp[512];

	va_start(list, fmt);

	if (IsLogging()) vfprintf(gsLog, fmt, list);

	va_end(list);

#endif
}

void Prim_Log(const char *fmt, ...)
{
#if defined(ZEROGS_DEVBUILD) && defined(WRITE_PRIM_LOGS)
	va_list list;
	char tmp[512];

	va_start(list, fmt);

	if (conf.log /*& 0x00000010*/)
	{
		if (IsLogging()) vfprintf(gsLog, fmt, list);

		fprintf(stderr, "ZZogl-PG(PRIM): ");
		vfprintf(stderr, fmt, list);

		vprintf(fmt, list);
	}

	va_end(list);
	fprintf(stderr,"\n");

#endif
}

void GS_Log(const char *fmt, ...)
{
#ifdef ZEROGS_DEVBUILD
	va_list list;

	va_start(list, fmt);

	if (IsLogging())
	{
		vfprintf(gsLog, fmt, list);
		fprintf(gsLog, "\n");
	}
	
	fprintf(stderr, "ZZogl-PG: ");
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
#endif
}

void Warn_Log(const char *fmt, ...)
{
#ifdef ZEROGS_DEVBUILD
	va_list list;

	va_start(list, fmt);

	if (IsLogging())
	{
		vfprintf(gsLog, fmt, list);
		fprintf(gsLog, "\n");
	}

	fprintf(stderr, "ZZogl-PG:  ");
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
#endif
}

void Debug_Log(const char *fmt, ...)
{
#if _DEBUG
	va_list list;

	va_start(list, fmt);

	if (IsLogging())
	{
		vfprintf(gsLog, fmt, list);
		fprintf(gsLog, "\n");
	}

	fprintf(stderr, "ZZogl-PG:  ");
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
#endif
}

void Error_Log(const char *fmt, ...)
{
	va_list list;

	va_start(list, fmt);

	if (IsLogging())
	{
		vfprintf(gsLog, fmt, list);
		fprintf(gsLog, "\n");
	}

	fprintf(stderr, "ZZogl-PG:  ");
	vfprintf(stderr, fmt, list);
	fprintf(stderr, "\n");
	
	va_end(list);
}
};

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

void CALLBACK GSshutdown()
{
	FUNCLOG

	if (gsLog != NULL) fclose(gsLog);
}

// keyboard functions
void OnKeyboardF5(int shift)
{
	FUNCLOG

	char strtitle[256];

	if (shift)
	{
		if (g_nPixelShaderVer == SHADER_REDUCED)
		{
			conf.bilinear = 0;
			sprintf(strtitle, "reduced shaders don't support bilinear filtering");
		}
		else
		{
			conf.bilinear = (conf.bilinear + 1) % 3;
			sprintf(strtitle, "bilinear filtering - %s", pbilinear[conf.bilinear]);
		}
	}
	else
	{
		conf.interlace++;

		if (conf.interlace > 2) conf.interlace = 0;

		if (conf.interlace < 2) 
			sprintf(strtitle, "interlace on - mode %d", conf.interlace);
		else 
			sprintf(strtitle, "interlace off");
	}

	ZeroGS::AddMessage(strtitle);

	SaveConfig();
}

void OnKeyboardF6(int shift)
{
	FUNCLOG

	char strtitle[256];

	if (shift)
	{
		conf.decAA();
		sprintf(strtitle, "anti-aliasing - %s", s_aa[conf.aa]);
		ZeroGS::SetAA(conf.aa);
	}
	else
	{
		conf.incAA();
		sprintf(strtitle, "anti-aliasing - %s", s_aa[conf.aa]);
		ZeroGS::SetAA(conf.aa);
	}

	ZeroGS::AddMessage(strtitle);

	SaveConfig();
}

void OnKeyboardF7(int shift)
{
	FUNCLOG

	char strtitle[256];

	if (!shift)
	{
		extern bool g_bDisplayFPS;
		g_bDisplayFPS ^= 1;
	}
	else
	{
		conf.zz_options.wireframe = !conf.zz_options.wireframe;
		glPolygonMode(GL_FRONT_AND_BACK, (conf.wireframe()) ? GL_LINE : GL_FILL);
		sprintf(strtitle, "wireframe rendering - %s", (conf.wireframe()) ? "on" : "off");
	}
}

void OnKeyboardF61(int shift)
{
	FUNCLOG

	char strtitle[256];

	if (shift)
	{
		conf.negaa--; // -1
		if (conf.negaa > 2) conf.negaa = 2;					// u8 in unsigned, so negative value is 255.
		sprintf(strtitle, "down resolution - %s", s_naa[conf.negaa]);
		ZeroGS::SetNegAA(conf.negaa);
	}
	else
	{
		conf.negaa++;
		if (conf.negaa > 2) conf.negaa = 0;
		sprintf(strtitle, "down resolution - %s", s_naa[conf.negaa]);
		ZeroGS::SetNegAA(conf.negaa);
	}

	ZeroGS::AddMessage(strtitle);
	SaveConfig();
}

typedef struct GameHackStruct
{
	const char HackName[40];
	u32 HackMask;
} GameHack;

#define HACK_NUMBER 30

GameHack HackinshTable[HACK_NUMBER] =
{
	{"*** 0 No Hack", 0},
	{"*** 1 TexTargets Check", GAME_TEXTURETARGS},
	{"*** 2 Autoreset Targets", GAME_AUTORESET},
	{"*** 3 Interlace 2x", GAME_INTERLACE2X},
	{"*** 4 TexA hack", GAME_TEXAHACK},
	{"*** 5 No Target Resolve", GAME_NOTARGETRESOLVE},
	{"*** 6 Exact color", GAME_EXACTCOLOR},
	{"*** 7 No color clamp", GAME_NOCOLORCLAMP},
	{"*** 8 FFX hack", GAME_FFXHACK},
	{"*** 9 No Alpha Fail", GAME_NOALPHAFAIL},
	{"***10 No Depth Update", GAME_NODEPTHUPDATE},
	{"***11 Quick Resolve 1", GAME_QUICKRESOLVE1},
	{"***12 No quick resolve", GAME_NOQUICKRESOLVE},
	{"***13 Notaget clut", GAME_NOTARGETCLUT},
	{"***14 No Stencil", GAME_NOSTENCIL},
	{"***15 No Depth resolve", GAME_NODEPTHRESOLVE},
	{"***16 Full 16 bit", GAME_FULL16BITRES},
	{"***17 Resolve promoted", GAME_RESOLVEPROMOTED},
	{"***18 Fast Update", GAME_FASTUPDATE},
	{"***19 No Alpha Test", GAME_NOALPHATEST},
	{"***20 Disable MRT deprh", GAME_DISABLEMRTDEPTH},
	{"***21 32 bit targes", GAME_32BITTARGS},
	{"***22 path 3 hack", GAME_PATH3HACK},
	{"***23 parallelise calls", GAME_DOPARALLELCTX},
	{"***24 specular highligths", GAME_XENOSPECHACK},
	{"***25 partial pointers", GAME_PARTIALPOINTERS},
	{"***26 partial depth", GAME_PARTIALDEPTH},
	{"***27 reget hack", GAME_REGETHACK},

	{"***28 gust hack", GAME_GUSTHACK},
	{"***29 log-Z", GAME_NOLOGZ}
};

int CurrentHackSetting = 0;

void OnKeyboardF9(int shift)
{
	FUNCLOG

//	printf ("A %d\n", HackinshTable[CurrentHackSetting].HackMask);
	conf.hacks._u32 &= !(HackinshTable[CurrentHackSetting].HackMask);

	if (shift)
	{
		CurrentHackSetting--;

		if (CurrentHackSetting == -1) CurrentHackSetting = HACK_NUMBER - 1;
	}
	else
	{
		CurrentHackSetting++;

		if (CurrentHackSetting == HACK_NUMBER) CurrentHackSetting = 0;
	}

	conf.hacks._u32 |= HackinshTable[CurrentHackSetting].HackMask;

	ZeroGS::AddMessage(HackinshTable[CurrentHackSetting].HackName);
	SaveConfig();
}

void OnKeyboardF1(int shift)
{
	FUNCLOG
	char strtitle[256];
	sprintf(strtitle, "Saving in savestate %d", CurrentSavestate);
	SaveStateExists = true;
	ZeroGS::AddMessage(HackinshTable[CurrentHackSetting].HackName);
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

	switch (conf.bilinear)
	{
		case 2:
			ZeroGS::AddMessage("bilinear filtering - forced", 1000);
			break;

		case 1:
			ZeroGS::AddMessage("bilinear filtering - normal", 1000);
			break;

		default:
			break;
	}

	if (conf.aa != 0)
	{
		char strtitle[64];
		sprintf(strtitle, "anti-aliasing - %s", s_aa[conf.aa]);
		ZeroGS::AddMessage(strtitle, 1000);
	}

	luPerfFreq = GetCPUTicks();

	gs.path[0].mode = gs.path[1].mode = gs.path[2].mode = 0;
	ResetRegs();
	ZZLog::GS_Log("GSopen finished.");

	return 0;
}

#ifdef _WIN32
void ProcessMessages()
{
	MSG msg;

	ZeroMemory(&msg, sizeof(msg));

	while (1)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			switch (msg.message)
			{

				case WM_KEYDOWN :
					int my_KeyEvent = msg.wParam;
					bool my_bShift = !!(GetKeyState(VK_SHIFT) & 0x8000);

					switch (msg.wParam)
					{

						case VK_F5:
							OnKeyboardF5(my_bShift);
							break;

						case VK_F6:
							OnKeyboardF6(my_bShift);
							break;

						case VK_F7:
							OnKeyboardF7(my_bShift);
							break;

						case VK_F9:
							OnKeyboardF9(my_bShift);
							break;

						case VK_ESCAPE:

							if (conf.fullscreen())
							{
								// destroy that msg
								conf.setFullscreen(false);
								ZeroGS::ChangeDeviceSize(conf.width, conf.height);
								UpdateWindow(GShwnd);
								continue; // so that msg doesn't get sent
							}
							else
							{
								SendMessage(GShwnd, WM_DESTROY, 0, 0);
								return;
							}

							break;
					}

					break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			break;
		}
	}

	if ((GetKeyState(VK_MENU) & 0x8000) && (GetKeyState(VK_RETURN) & 0x8000))
	{
		conf.zz_options.fullscreen = !conf.zz_options.fullscreen;

		ZeroGS::SetChangeDeviceSize(
			(conf.fullscreen()) ? 1280 : conf.width,
			(conf.fullscreen()) ? 960 : conf.height);
	}
}

#else // linux

void ProcessMessages()
{
	FUNCLOG

	// check resizing
	GLWin.ResizeCheck();

	if (THR_KeyEvent)     // This values was passed from GSKeyEvents which could be in another thread
	{
		int my_KeyEvent = THR_KeyEvent;
		bool my_bShift = THR_bShift;
		THR_KeyEvent = 0;

		switch (my_KeyEvent)
		{
			case XK_F5:
				OnKeyboardF5(my_bShift);
				break;

			case XK_F6:
				OnKeyboardF6(my_bShift);
				break;

			case XK_F7:
				OnKeyboardF7(my_bShift);
				break;

			case XK_F9:
				OnKeyboardF9(my_bShift);
				break;
		}
	}
}

#endif // linux

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

		sprintf(filename, "%ssnap%03ld.%s", path, snapshotnr, (conf.zz_options.tga_snap) ? "bmp" : "jpg");

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
