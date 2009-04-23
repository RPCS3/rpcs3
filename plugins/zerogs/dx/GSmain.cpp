/*  ZeroGS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#if defined(_WIN32) || defined(_WIN32)
#include <d3dx9.h>
#include <dxerr9.h>
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

#include "Win32.h"

#include "GS.h"
#include "Mem.h"
#include "Regs.h"

#include "zerogs.h"
#include "targets.h"
#include "zerogsshaders/zerogsshaders.h"

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

#ifdef _DEBUG
HANDLE g_hCurrentThread = NULL;
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
int g_GameSettings = 0;

static LARGE_INTEGER luPerfFreq;

// statistics
u32 g_nGenVars = 0, g_nTexVars = 0, g_nAlphaVars = 0, g_nResolve = 0;

#define VER 97
const unsigned char version  = PS2E_GS_VERSION;
unsigned char revision = 0;	// revision and build gives plugin version
unsigned char build	= VER;
unsigned char minor = 1;

#ifdef _DEBUG
char *libraryName	= "ZeroGS (Debug) ";
#elif defined(RELEASE_TO_PUBLIC)

#ifdef ZEROGS_SSE2
char *libraryName	= "ZeroGS";
#else
char *libraryName	= "ZeroGS (no SSE2)";
#endif

#else
char *libraryName	= "ZeroGS (Dev) ";
#endif

static const char* s_aa[5] = { "AA none |", "AA 2x |", "AA 4x |", "AA 8x", "AA 16x" };

extern GIFRegHandler g_GIFPackedRegHandlers[];
extern GIFRegHandler g_GIFRegHandlers[];
GIFRegHandler g_GIFTempRegHandlers[16] = {0};
extern int g_nPixelShaderVer;
extern int g_nFrameRender;
extern int g_nFramesSkipped;

const char* pbilinear[] = { "off", "normal", "forced" };

int s_frameskipping = 0;
u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_GS;
}

char* CALLBACK PS2EgetLibName() {
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (version<<16) | (revision<<8) | build | (minor << 24);
}

#ifdef _WIN32

HWND GShwnd;

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "GSsoftdx Msg", 0);
}

#endif

void __Log(const char *fmt, ...) {
	va_list list;

	// gsLog can be null if the config dialog is used prior to Pcsx2 an emulation session.
	// (GSinit won't have been called then)

	if (gsLog == NULL || !conf.log) return;

	va_start(list, fmt);
	vfprintf(gsLog, fmt, list);
	va_end(list);
}

void __LogToConsole(const char *fmt, ...) {
	va_list list;

	printf("ZeroGS: ");

	// gsLog can be null if the config dialog is used prior to Pcsx2 an emulation session.
	// (GSinit won't have been called then)

	va_start(list, fmt);
	if( gsLog != NULL )
		vfprintf(gsLog, fmt, list);
	vprintf(fmt, list);
	va_end(list);
}

void CALLBACK GSsetBaseMem(void* pmem) {
	g_pBasePS2Mem = (u8*)pmem;
}

extern int VALIDATE_THRESH;
extern u32 TEXDESTROY_THRESH;
int g_LastCRC = 0;
void CALLBACK GSsetGameCRC(int crc, int options)
{
	VALIDATE_THRESH = 8;
	g_GameSettings = conf.gamesettings|options;
	conf.mrtdepth = !(conf.gamesettings&GAME_DISABLEMRTDEPTH);

	g_GameSettings |= GAME_PATH3HACK;
	g_LastCRC = crc;

	//g_GameSettings |= GAME_PARTIALDEPTH;

//	g_GameSettings |= GAME_DOPARALLELCTX|GAME_XENOSPECHACK;
//	VALIDATE_THRESH = 64;
//	TEXDESTROY_THRESH = 32;

	switch(crc) {
		case 0x54A548B4: // crash n burn
			// overbright
			if( pd3dDevice != NULL ) {
				pd3dDevice->SetPixelShaderConstantF(27, DXVEC4(0.5f, 0.9f/256.0f, 0,1/255.0f), 1); // g_fExactColor
			}
			break;

		case 0xA3D63039: // xenosaga(j)
		case 0x0E7807B2: // xenosaga(u)
			g_GameSettings |= GAME_DOPARALLELCTX;
			VALIDATE_THRESH = 64;
			TEXDESTROY_THRESH = 32;
			break;

		case 0x7D2FE035: // espgaluda (j)
			VALIDATE_THRESH = 24;
			//g_GameSettings |= GAME_BIGVALIDATE;
			break;
	}

	DEBUG_LOG("ZeroGS: Set game options: 0x%8.8x\n", g_GameSettings);
}

void CALLBACK GSsetFrameSkip(int frameskip)
{
	s_frameskipping |= frameskip;
	if( frameskip && g_nFrameRender > 1 ) {
		
		for(int i = 0; i < 16; ++i) {
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
	else if( !frameskip && g_nFrameRender <= 0 ) {
		g_nFrameRender = 1;

		if( g_GIFTempRegHandlers[0] == NULL ) return; // not init yet

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

void CALLBACK GSreset() {

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
	if( mask & 1 ) memset(&gs.path1, 0, sizeof(gs.path1));
	if( mask & 2 ) memset(&gs.path2, 0, sizeof(gs.path2));
	if( mask & 4 ) memset(&gs.path3, 0, sizeof(gs.path3));
	gs.imageTransfer = -1;
	gs.q = 1;
	gs.nTriFanVert = -1;
}

s32 CALLBACK GSinit()
{
	memcpy(g_GIFTempRegHandlers, g_GIFPackedRegHandlers, sizeof(g_GIFTempRegHandlers));

#ifdef GS_LOG
	gsLog = fopen("logs/gsLog.txt", "w");
	if (gsLog == NULL) {
		gsLog = fopen("gsLog.txt", "w");
		if (gsLog == NULL) {
			SysMessage("Can't create gsLog.txt"); return -1;
		}
	}
	setvbuf(gsLog, NULL,  _IONBF, 0);
	GS_LOG("GSinit\n");
#endif

	GSreset();
	GS_LOG("GSinit ok\n");

	return 0;
}

void CALLBACK GSshutdown()
{
#ifdef GS_LOG
	if (gsLog != NULL) fclose(gsLog);
#endif
}

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static int nWindowWidth = 0, nWindowHeight = 0;

	switch( msg ) {
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;

		case WM_KEYDOWN:
//			switch(wParam) {
//				case VK_ESCAPE:
//					SendMessage(hWnd, WM_DESTROY, 0L, 0L);
//					break;
//			}
			break;

		case WM_ACTIVATE:

			if( wParam != WA_INACTIVE ) {
				//DEBUG_LOG("restoring device\n");
				ZeroGS::Restore();
			}

			break;

		case WM_SIZE:
			nWindowWidth = lParam&0xffff;
			nWindowHeight = lParam>>16;
			ZeroGS::ChangeWindowSize(nWindowWidth, nWindowHeight);

			break;

		case WM_SIZING:
			// if button is 0, then just released so can resize
			if( GetSystemMetrics(SM_SWAPBUTTON) ? !GetAsyncKeyState(VK_RBUTTON) : !GetAsyncKeyState(VK_LBUTTON) ) {
				ZeroGS::SetChangeDeviceSize(nWindowWidth, nWindowHeight);
			}
			break;

		case WM_SETCURSOR:
			SetCursor(NULL);
			break;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}

s32 CALLBACK GSopen(void *pDsp, char *Title, int multithread) {

	g_GSMultiThreaded = multithread;

	GS_LOG("GSopen\n");

#ifdef _DEBUG
	g_hCurrentThread = GetCurrentThread();
#endif

	assert( GSirq != NULL );
	LoadConfig();
	g_GameSettings = 0;

	strcpy(GStitle, Title);
	
	RECT rc, rcdesktop;
	rc.left = 0; rc.top = 0;
	rc.right = conf.width; rc.bottom = conf.height;

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, 
					GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
					"PS2EMU_ZEROGS", NULL };
	RegisterClassEx( &wc );

	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	GetWindowRect(GetDesktopWindow(), &rcdesktop);

	GShwnd = CreateWindow( "PS2EMU_ZEROGS", "ZeroGS", WS_OVERLAPPEDWINDOW,
		(rcdesktop.right-(rc.right-rc.left))/2, (rcdesktop.bottom-(rc.bottom-rc.top))/2,
		rc.right-rc.left, rc.bottom-rc.top, NULL, NULL, wc.hInstance, NULL );

	if(GShwnd == NULL) {
		GS_LOG("Failed to create window. Exiting...");
		return -1;
	}

	if( pDsp != NULL )
		*(int*)pDsp = (int)GShwnd;

	DEBUG_LOG("creating zerogs\n");
	//if (conf.record) recOpen();
	if( FAILED(ZeroGS::Create(conf.width, conf.height)) )
		return -1;

	DEBUG_LOG("initialization successful\n");

	if( FAILED(ZeroGS::InitDeviceObjects()) )
		return -1;

	if( conf.bilinear == 2 ) {
		ZeroGS::AddMessage("forced bilinear filtering - on", 1000);
	}
	else if( conf.bilinear == 1 ) {
		ZeroGS::AddMessage("normal bilinear filtering - on", 1000);
	}
	if( conf.aa ) {
		char strtitle[64];
		sprintf(strtitle, "anti-aliasing - %s", s_aa[conf.aa], 1000);
		ZeroGS::AddMessage(strtitle);
	}
	if( conf.options & GSOPTION_WIDESCREEN ) {
		ZeroGS::AddMessage("16:9 widescreen - on", 1000);
	}

	// set just in case
	SetWindowLongPtr(GShwnd, GWLP_WNDPROC, (LPARAM)(WNDPROC)MsgProc);
	
	ShowWindow( GShwnd, SW_SHOWDEFAULT );
	UpdateWindow( GShwnd );
	SetFocus(GShwnd);

	conf.winstyle = GetWindowLong( GShwnd, GWL_STYLE );
	conf.winstyle &= ~WS_MAXIMIZE & ~WS_MINIMIZE; // remove minimize/maximize style

#ifdef _WIN32
	QueryPerformanceFrequency(&luPerfFreq);
#else
	luPerfFreq.QuadPart = 1;
#endif

	GS_LOG("GSopen ok\n");

	gs.path1.mode = 0;
	gs.path2.mode = 0;
	gs.path3.mode = 0;

	return 0;	
}

void CALLBACK GSclose() {
	ZeroGS::Destroy(1);

	//if (conf.record) recClose();

#ifdef _WIN32
	DestroyWindow(GShwnd);
#endif
}

void CALLBACK GSirqCallback(void (*callback)()) {
	GSirq = callback;
}

void CALLBACK GSwriteCSR(u32 write)
{
	gs.CSRw = write;
}

void CALLBACK GSchangeSaveState(int newstate, const char* filename)
{
	char str[255];
	sprintf(str, "save state %d", newstate);
	ZeroGS::AddMessage(str);
}

void CALLBACK GSmakeSnapshot(char *path)
{
	FILE *bmpfile;
	char filename[256];	
	u32 snapshotnr = 0;
	
	// increment snapshot value & try to get filename
	for (;;) {
		snapshotnr++;

		sprintf(filename,"%ssnap%03ld.%s", path, snapshotnr, (conf.options&GSOPTION_BMPSNAP)?"bmp":"jpg");

		bmpfile=fopen(filename,"rb");
		if (bmpfile == NULL) break;
		fclose(bmpfile);
	}

	// try opening new snapshot file
	if((bmpfile=fopen(filename,"wb"))==NULL) {
		char strdir[255];
		sprintf(strdir, "%s", path);
		CreateDirectory(strdir, NULL);

		if((bmpfile=fopen(filename,"wb"))==NULL) return;
	}

	fclose(bmpfile);

	// get the bits
	ZeroGS::SaveSnapshot(filename);
}

int UPDATE_FRAMES = 16;
int g_nFrame = 0;
int g_nRealFrame = 0;
static BOOL g_bHidden = 0;

float fFPS = 0;

void CALLBACK GSvsync(int interlace)
{
	GS_LOG("\nGSvsync\n\n");

	static u32 dwTime = timeGetTime();
	static int nToNextUpdate = 1;

	char strtitle[512];
	
	g_nRealFrame++;

	MSG msg; 
	ZeroMemory( &msg, sizeof(msg) );
	while( 1 ) {
		if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
		{
			switch( msg.message ) {
				case WM_KEYDOWN :
					if( msg.wParam == VK_F5 ) {

						if( (GetKeyState(VK_SHIFT)&0x8000) ) {
							if( g_nPixelShaderVer == SHADER_20 ) {
								conf.bilinear = 0;
								sprintf(strtitle, "ps 2.0 doesn't support bilinear filtering");
							}
							else {
								conf.bilinear = (conf.bilinear+1)%3;
								sprintf(strtitle, "bilinear filtering - %s", pbilinear[conf.bilinear]);
							}
						}
						else {
							conf.interlace++;
							if( conf.interlace > 2 ) conf.interlace = 0;
							if( conf.interlace < 2 ) sprintf(strtitle, "interlace on - mode %d", conf.interlace);
							else sprintf(strtitle, "interlace off");
						}

						ZeroGS::AddMessage(strtitle);
						SaveConfig();
					}
					else if( msg.wParam == VK_F6 ) {
						
						if( (GetKeyState(VK_SHIFT)&0x8000) ) {
							conf.aa--; // -1
							if( conf.aa > 4) conf.aa = 4;
							sprintf(strtitle, "anti-aliasing - %s", s_aa[conf.aa]);
							ZeroGS::SetAA(conf.aa);
						}
						else {
							conf.aa++;
							if( conf.aa > 4 ) conf.aa = 0;
							sprintf(strtitle, "anti-aliasing - %s", s_aa[conf.aa]);
							ZeroGS::SetAA(conf.aa);
						}

						ZeroGS::AddMessage(strtitle);
						SaveConfig();
					}
					else if( msg.wParam == VK_F7 ) {

						if( (GetKeyState(VK_SHIFT)&0x8000) ) {
							extern BOOL g_bDisplayFPS;
							g_bDisplayFPS ^= 1;
						}
						else {
							conf.options ^= GSOPTION_WIREFRAME;
							SETRS(D3DRS_FILLMODE, (conf.options&GSOPTION_WIREFRAME)?D3DFILL_WIREFRAME:D3DFILL_SOLID);
							sprintf(strtitle, "wireframe rendering - %s", (conf.options&GSOPTION_WIREFRAME)?"on":"off");
//							conf.options ^= GSOPTION_CAPTUREAVI;
//							if( conf.options & GSOPTION_CAPTUREAVI ) ZeroGS::StartCapture();
//							else ZeroGS::StopCapture();
//
//							sprintf(strtitle, "capture avi (zerogs_dump.avi) - %s", (conf.options&GSOPTION_CAPTUREAVI) ? "on" : "off");
//							ZeroGS::AddMessage(strtitle);
//							SaveConfig();
						}
					}
					else if( msg.wParam == VK_F9 ) {

						if( (GetKeyState(VK_SHIFT)&0x8000) ) {
							conf.options ^= GSOPTION_WIDESCREEN;
							sprintf(strtitle, "16:9 widescreen - %s", (conf.options&GSOPTION_WIDESCREEN)?"on":"off");
						}
						else {
							g_GameSettings ^= GAME_PATH3HACK;
							sprintf(strtitle, "path3 hack - %s", (g_GameSettings&GAME_PATH3HACK) ? "on" : "off");
						}
						ZeroGS::AddMessage(strtitle);
					}
					else if( msg.wParam == VK_ESCAPE ) {

						if( conf.options & GSOPTION_FULLSCREEN ) {
							// destroy that msg					
							conf.options &= ~GSOPTION_FULLSCREEN;
							conf.winstyle = GetWindowLong( GShwnd, GWL_STYLE );
							conf.winstyle &= ~WS_MAXIMIZE & ~WS_MINIMIZE; // remove minimize/maximize style
							ZeroGS::ChangeDeviceSize(conf.width, conf.height);
							UpdateWindow(GShwnd);
							continue; // so that msg doesn't get sent
						}
						else {
							SendMessage(GShwnd, WM_DESTROY, 0, 0);
							g_bHidden = 1;
							return;
						}
					}
			
					break;
			}

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
			break;
	}

	if( (GetKeyState(VK_MENU)&0x8000) && (GetKeyState(VK_RETURN)&0x8000) ) {
		conf.options ^= GSOPTION_FULLSCREEN;

		if( conf.options & GSOPTION_FULLSCREEN ) {
			conf.winstyle = GetWindowLong( GShwnd, GWL_STYLE );
			conf.winstyle &= ~WS_MAXIMIZE & ~WS_MINIMIZE; // remove minimize/maximize style
		}

		ZeroGS::SetChangeDeviceSize(
			(conf.options&GSOPTION_FULLSCREEN) ? 1280 : conf.width,
			(conf.options&GSOPTION_FULLSCREEN) ? 960 : conf.height);
	}

//	if( conf.fullscreen && (GetKeyState(VK_ESCAPE)&0x8000)) {
//		conf.fullscreen &= ~GSOPTION_FULLSCREEN;
//		ZeroGS::SetChangeDeviceSize(conf.width, conf.height);
//	}

 	//if( conf.interlace && g_nGenVars + g_nTexVars + g_nAlphaVars + g_nResolve == 0 )
	//	CSR->FIELD = 0; // 0 should always be the repeating at 0

	ZeroGS::RenderCRTC(!interlace);

	if( --nToNextUpdate <= 0 ) {

		u32 d = timeGetTime();
		fFPS = UPDATE_FRAMES * 1000.0f / (float)max(d-dwTime,1);
		dwTime = d;
		g_nFrame += UPDATE_FRAMES;

#ifdef RELEASE_TO_PUBLIC
		const char* g_pShaders[4] = { "ps 2.0", "ps 2.0a", "ps 2.0b", "ps 3.0" };

		_snprintf(strtitle, 512, "%s 0.%d.%d %.1f fps | %s%s%s%s %s (%.1f)", libraryName, build, minor, fFPS,
			(conf.interlace < 2) ? "interlace | " : "",
			conf.bilinear ? (conf.bilinear==2?"forced bilinear | ":"bilinear | ") : "",
			conf.aa ? s_aa[conf.aa] : "",
			(g_GameSettings&GAME_FFXHACK) ? "ffxhack | " : "",
			g_pShaders[g_nPixelShaderVer], (ppf&0xfffff)/(float)UPDATE_FRAMES);
#else
		_snprintf(strtitle, 512, "%d | %.1f fps (sk:%d%%) | g: %.1f, t: %.1f, a: %.1f, r: %.1f | p: %.1f | tex: %d %d (%d kbpf)", g_nFrame, fFPS, 
			100*g_nFramesSkipped/g_nFrame,
			g_nGenVars/(float)UPDATE_FRAMES, g_nTexVars/(float)UPDATE_FRAMES, g_nAlphaVars/(float)UPDATE_FRAMES,
			g_nResolve/(float)UPDATE_FRAMES, (ppf&0xfffff)/(float)UPDATE_FRAMES,
			ZeroGS::g_MemTargs.listTargets.size(), ZeroGS::g_MemTargs.listClearedTargets.size(), g_TransferredToGPU>>10);
		//_snprintf(strtitle, 512, "%x %x", *(int*)(ZeroGS::g_pbyGSMemory + 256 * 0x3e0c + 4), *(int*)(ZeroGS::g_pbyGSMemory + 256 * 0x3e04 + 4));

#endif

		if( !(conf.options&GSOPTION_FULLSCREEN) )
			SetWindowText(GShwnd, strtitle);

		if( fFPS < 16 ) UPDATE_FRAMES = 4;
		else if( fFPS < 32 ) UPDATE_FRAMES = 8;
		else UPDATE_FRAMES = 16;

		nToNextUpdate = UPDATE_FRAMES;

		g_TransferredToGPU = 0;
		g_nGenVars = 0;
		g_nTexVars = 0;
		g_nAlphaVars = 0;
		g_nResolve = 0;
		ppf = 0;
		g_nFramesSkipped = 0;
	}
}

void GIFtag(pathInfo *path, u32 *data) {
	
	path->tag.nloop	= data[0] & 0x7fff;
	path->tag.eop	= (data[0] >> 15) & 0x1;
	u32 tagpre		= (data[1] >> 14) & 0x1;
	u32 tagprim		= (data[1] >> 15) & 0x7ff;
	u32 tagflg		= (data[1] >> 26) & 0x3;
	path->tag.nreg	= (data[1] >> 28)<<2;
	
	if (path->tag.nreg == 0) path->tag.nreg = 64;

	gs.q = 1;

//	GS_LOG("GIFtag: %8.8lx_%8.8lx_%8.8lx_%8.8lx: EOP=%d, NLOOP=%x, FLG=%x, NREG=%d, PRE=%d\n",
//			data[3], data[2], data[1], data[0],
//			path->tag.eop, path->tag.nloop, tagflg, path->tag.nreg, tagpre);

	path->mode = tagflg+1;

	switch (tagflg) {
		case 0x0:
			path->regs = *(u64 *)(data+2);
			path->regn = 0;
			if (tagpre)
				GIFRegHandlerPRIM((u32*)&tagprim);

			break;

		case 0x1:
			path->regs = *(u64 *)(data+2);
			path->regn = 0;
			break;
	}
}

void _GSgifPacket(pathInfo *path, u32 *pMem) { // 128bit

	int reg = (int)((path->regs >> path->regn) & 0xf);
	g_GIFPackedRegHandlers[reg](pMem);

	path->regn += 4;
	if (path->tag.nreg == path->regn) {
		path->regn = 0;
		path->tag.nloop--;
	}
}

void _GSgifRegList(pathInfo *path, u32 *pMem) { // 64bit
	int reg;

	reg = (int)((path->regs >> path->regn) & 0xf);

	g_GIFRegHandlers[reg](pMem);
	path->regn += 4;
	if (path->tag.nreg == path->regn) {
		path->regn = 0;
		path->tag.nloop--;
	}
}

//static pathInfo* s_pLastPath = &gs.path3;
static int nPath3Hack = 0;

void CALLBACK GSgetLastTag(u64* ptag)
{
	*(u32*)ptag = nPath3Hack;
	nPath3Hack = 0;
}

void _GSgifTransfer(pathInfo *path, u32 *pMem, u32 size)
{
#ifdef _DEBUG
	assert( g_hCurrentThread == GetCurrentThread() );
#endif

	//s_pLastPath = path;
	//BOOL bAfter0Tag = 0;

//	if( conf.log & 0x20 ) {
//		__Log("%d: %x:%x\n", (path==&gs.path3)?3:(path==&gs.path2?2:1), pMem, size);
//		for(int i = 0; i < size; i++) {
//			__Log("%x %x %x %x\n", pMem[4*i+0], pMem[4*i+1], pMem[4*i+2], pMem[4*i+3]);
//		}
//	}

	while(size > 0)
	{
		//LOG(_T("Transfer(%08x, %d) START\n"), pMem, size);
		if (path->tag.nloop == 0) 
		{
 			GIFtag(path, pMem);
			pMem+= 4;
			size--;

			if ((g_GameSettings & GAME_PATH3HACK) && path == &gs.path3 && gs.path3.tag.eop)
				nPath3Hack = 1;
				
			if (path == &gs.path1) 
			{
				// if too much data, just ignore
				if (path->tag.nloop * (path->tag.nreg / 4) > (int)size * (path->mode==2?2:1)) 
				{
					static int lasttime = 0;
					if( timeGetTime() - lasttime > 5000 ) 
					{
						ERROR_LOG("VU1 too much data, ignore if gfx are fine\n");
						lasttime = timeGetTime();
					}
					path->tag.nloop = 0;
					return;
				}

				if (path->mode == 1) 
				{
					// check if 0xb is in any reg, if yes, exit (kh2)
					for(int i = 0; i < path->tag.nreg; i += 4) 
					{
						if (((path->regs >> i)&0xf) == 11) 
						{
							static int lasttime = 0;
							if( timeGetTime() - lasttime > 5000 ) 
							{
								ERROR_LOG("Invalid unpack type\n");
								lasttime = timeGetTime();
							}
							path->tag.nloop = 0;
							return;
						}
					}
				}
			}

			if(path->tag.nloop == 0 ) 
			{
				if( path == &gs.path1 ) 
				{
					// ffx hack
					if( g_GameSettings & GAME_FFXHACK ) 
					{
						if( path->tag.eop )
							return;
						continue;
					}

					return;
				}
				
				if( !path->tag.eop ) 
				{
					//DEBUG_LOG("continuing from eop\n");
					continue;
				}
				
				continue;
			}
		}

  		switch(path->mode) {
		case 1: // PACKED
		{
			assert( path->tag.nloop > 0 );
			for(; size > 0; size--, pMem += 4)
			{
				int reg = (int)((path->regs >> path->regn) & 0xf);

				g_GIFPackedRegHandlers[reg](pMem);

				path->regn += 4;
				if (path->tag.nreg == path->regn) 
				{
					path->regn = 0;
					if( path->tag.nloop-- <= 1 ) 
					{
						size--;
						pMem += 4;
						break;
					}
				}
			}
			break;
		}
		case 2: // REGLIST
		{
			//GS_LOG("%8.8x%8.8x %d L\n", ((u32*)&gs.regs)[1], *(u32*)&gs.regs, path->tag.nreg/4);
			assert( path->tag.nloop > 0 );
			size *= 2;
			for(; size > 0; pMem+= 2, size--)
			{
				int reg = (int)((path->regs >> path->regn) & 0xf);
				g_GIFRegHandlers[reg](pMem);
				path->regn += 4;
				if (path->tag.nreg == path->regn) 
				{
					path->regn = 0;
					if( path->tag.nloop-- <= 1 ) 
					{
						size--;
						pMem += 2;
						break;
					}
				}
			}

			if( size & 1 ) pMem += 2;
			size /= 2;
			break;
		}
		case 3: // GIF_IMAGE (FROM_VFRAM)
		case 4:
		{
			if(gs.imageTransfer >= 0 && gs.imageTransfer <= 1)
			{
				int process = min((int)size, path->tag.nloop);

				if( process > 0 ) 
				{
					if (gs.imageTransfer) 
						ZeroGS::TransferLocalHost(pMem, process);
					else 
						ZeroGS::TransferHostLocal(pMem, process*4);

					path->tag.nloop -= process;
					pMem += process*4; size -= process;

					assert( size == 0 || path->tag.nloop == 0 );
				}
				break;
			}
			else 
			{
				// simulate
				int process = min((int)size, path->tag.nloop);
				path->tag.nloop -= process;
				pMem += process*4; size -= process;
			}

			break;
		}	
		default: // GIF_IMAGE
			GS_LOG("*** WARNING **** Unexpected GIFTag flag\n");
			assert(0);
			path->tag.nloop = 0;
			break;

		}

		if( path == &gs.path1 && path->tag.eop )
			return;
	}
}

void CALLBACK GSgifTransfer2(u32 *pMem, u32 size)
{
	//GS_LOG("GSgifTransfer2 size = %lx (mode %d, gs.path2.tag.nloop = %d)\n", size, gs.path2.mode, gs.path2.tag.nloop);

	_GSgifTransfer(&gs.path2, pMem, size);
}

void CALLBACK GSgifTransfer3(u32 *pMem, u32 size)
{
	//GS_LOG("GSgifTransfer3 size = %lx (mode %d, gs.path3.tag.nloop = %d)\n", size, gs.path3.mode, gs.path3.tag.nloop);

	nPath3Hack = 0;
	_GSgifTransfer(&gs.path3, pMem, size);
}

static int count = 0;
void CALLBACK GSgifTransfer1(u32 *pMem, u32 addr)
{
	pathInfo *path = &gs.path1;

	//GS_LOG("GSgifTransfer1 0x%x (mode %d)\n", addr, path->mode);

	addr &= 0x3fff;

#ifdef _DEBUG
	PRIM_LOG("count: %d\n", count);
	count++;
#endif

	gs.path1.tag.nloop = 0;
	gs.path1.tag.eop = 0;
	_GSgifTransfer(&gs.path1, (u32*)((u8*)pMem+addr), (0x4000-addr)/16);

	if( !gs.path1.tag.eop && gs.path1.tag.nloop > 0 ) {
		assert( (addr&0xf) == 0 ); //BUG
		gs.path1.tag.nloop = 0;
		ERROR_LOG("Transfer1 - 2\n");
		return;
	}
}

void CALLBACK GSreadFIFO(u64 *pMem)
{
	//GS_LOG("GSreadFIFO\n");

	ZeroGS::TransferLocalHost((u32*)pMem, 1);
}

void CALLBACK GSreadFIFO2(u64 *pMem, int qwc)
{
	//GS_LOG("GSreadFIFO2\n");

	ZeroGS::TransferLocalHost((u32*)pMem, qwc);
}

int CALLBACK GSsetupRecording(int start, void* pData)
{
	if( start ) {
		if( conf.options & GSOPTION_CAPTUREAVI )
			return 1;
		if( !ZeroGS::StartCapture() )
			return 0;
		conf.options |= GSOPTION_CAPTUREAVI;
		DEBUG_LOG("ZeroGS: started recording at zerogs.avi\n");
	}
	else {
		if( !(conf.options & GSOPTION_CAPTUREAVI) )
			return 1;
		conf.options &= ~GSOPTION_CAPTUREAVI;
		ZeroGS::StopCapture();
		DEBUG_LOG("ZeroGS: stopped recording\n");
	}

	return 1;
}

s32 CALLBACK GSfreeze(int mode, freezeData *data) 
{
	switch (mode)
	{
		case FREEZE_LOAD:
			if (!ZeroGS::Load(data->data)) DEBUG_LOG("GS: Bad load format!");
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

extern HINSTANCE hInst;
void CALLBACK GSconfigure() {
	DialogBox(hInst,
				MAKEINTRESOURCE(IDD_CONFIG),
				GetActiveWindow(),  
				(DLGPROC)ConfigureDlgProc);

	if( g_nPixelShaderVer == SHADER_20 )
		conf.bilinear = 0;
}
