/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSUtil.h"
#include "GSRendererDX9.h"
#include "GSRendererDX11.h"
#include "GSRendererSW.h"
#include "GSRendererNull.h"
#include "GSSettingsDlg.h"

#define PS2E_LT_GS 0x01
#define PS2E_GS_VERSION 0x0006
#define PS2E_X86 0x01   // 32 bit
#define PS2E_X86_64 0x02   // 64 bit

static HRESULT s_hr = E_FAIL;
static GSRenderer* s_gs = NULL;
static void (*s_irq)() = NULL;
static uint8* s_basemem = NULL;
static int s_renderer = -1;
static bool s_framelimit = true;
static bool s_vsync = false;
static bool s_exclusive = true;

static bool s_IsGsOpen2 = false;		// boolean to remove some stuff from the config panel in new PCSX2's/

EXPORT_C_(uint32) PS2EgetLibType()
{
	return PS2E_LT_GS;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return GSUtil::GetLibName();
}

EXPORT_C_(uint32) PS2EgetLibVersion2(uint32 type)
{
	const uint32 revision = 0;
	const uint32 build = 1;

	return (build << 0) | (revision << 8) | (PS2E_GS_VERSION << 16) | (PLUGIN_VERSION << 24);
}

EXPORT_C_(void) PS2EsetEmuVersion(const char* emuId, uint32 version)
{
	s_IsGsOpen2 = true;
}

EXPORT_C_(uint32) PS2EgetCpuPlatform()
{
#if _M_AMD64
	return PS2E_X86_64;
#else
	return PS2E_X86;
#endif
}

EXPORT_C GSsetBaseMem(uint8* mem)
{
	s_basemem = mem;
	if( s_gs )
	{
		s_gs->SetRegsMem( s_basemem );
	}
}

EXPORT_C GSsetSettingsDir(const char* dir)
{
	theApp.SetConfigDir(dir);
}

EXPORT_C_(INT32) GSinit()
{
	if(!GSUtil::CheckSSE())
	{
		return -1;
	}

#ifdef _WINDOWS

	//_CrtSetBreakAlloc( 1273 );

	s_hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if(!GSUtil::CheckDirectX())
	{
		return -1;
	}

#endif

	return 0;
}

EXPORT_C GSshutdown()
{
	delete s_gs;

	s_gs = NULL;
	s_renderer = -1;

	GSUtil::UnloadDynamicLibraries();
	
#ifdef _WINDOWS

	if(SUCCEEDED(s_hr))
	{
		::CoUninitialize();

		s_hr = E_FAIL;
	}

#endif
}

EXPORT_C GSclose()
{
	if( !s_gs ) return;

	s_gs->ResetDevice();

	delete s_gs->m_dev;
	s_gs->m_dev = NULL;

	s_gs->m_wnd.Detach();
}

static INT32 _GSopen(void* dsp, char* title, int renderer)
{
	GSDevice* dev = NULL;

	if( renderer == -1 )
	{
		renderer = theApp.GetConfig("renderer", 0);
	}

	try
	{
		if (s_renderer != renderer)
		{
			// Emulator has made a render change request, which requires a completely
			// new s_gs -- if the emu doesn't save/restore the GS state across this
			// GSopen call then they'll get corrupted graphics, but that's not my problem.

			delete s_gs;
			s_gs = NULL;
		}

		switch(renderer)
		{
		default:
		case 0: case 1: case 2: dev = new GSDevice9(); break;
		case 3: case 4: case 5: dev = new GSDevice11(); break;
		case 12: case 13: new GSDeviceNull(); break;
		}

		if( !dev ) return -1;

		if( !s_gs )
		{
			switch(renderer)
			{
			default:
			case 0: s_gs = new GSRendererDX9(); break;
			case 3: s_gs = new GSRendererDX11(); break;
			case 2: case 5: case 8: case 11: case 13:
				s_gs = new GSRendererNull(); break;

			case 1: case 4: case 7: case 10: case 12:
				s_gs = new GSRendererSW(); break;
			}

			s_renderer = renderer;
		}
	}
	catch( std::exception& ex )
	{
		// Allowing std exceptions to escape the scope of the plugin callstack could
		// be problematic, because of differing typeids between DLL and EXE compilations.
		// ('new' could throw std::alloc)

		printf( "GSdx error: Exception caught in GSopen: %s", ex.what() );
		return -1;
	}

	s_gs->SetRegsMem(s_basemem);
	s_gs->SetIrqCallback(s_irq);
	s_gs->SetVsync(s_vsync);
	s_gs->SetFrameLimit(s_framelimit);

	if( *(HWND*)dsp == NULL )
	{
		// old-style API expects us to create and manage our own window:

		int w = theApp.GetConfig("ModeWidth", 0);
		int h = theApp.GetConfig("ModeHeight", 0);

		if(!s_gs->CreateWnd(title, w, h))
		{
			GSclose();
			return -1;
		}

		s_gs->m_wnd.Show();
		*(HWND*)dsp = (HWND)s_gs->m_wnd.GetHandle();
	}
	else
	{
		s_gs->SetMultithreaded( true );
		s_gs->m_wnd.Attach( *(HWND*)dsp, false );
	}

	if( !s_gs->CreateDevice(dev) )
	{
		GSclose();
		return -1;
	}

	// if(mt) _mm_setcsr(MXCSR);

	return 0;
}

EXPORT_C_(INT32) GSopen2( void* dsp, INT32 flags )
{
	int renderer = theApp.GetConfig("renderer", 0);
	if( flags & 4 )
	{
		static bool isdx11avail = GSUtil::IsDirect3D11Available();
		if (isdx11avail)	renderer = 4; //dx11 sw
		else				renderer = 1; //dx9 sw
	}

	INT32 retval = _GSopen( dsp, NULL, renderer );
	s_gs->SetAspectRatio(0);		// PCSX2 manages the aspect ratios

	return retval;
}

EXPORT_C_(INT32) GSopen(void* dsp, char* title, int mt)
{
	int renderer;

	// Legacy GUI expects to acquire vsync from the configuration files.
	s_vsync = !!theApp.GetConfig("vsync", 0);

	if(mt == 2)
	{
		// pcsx2 sent a switch renderer request
		static bool isdx11avail = GSUtil::IsDirect3D11Available();
		if (isdx11avail)	renderer = 4; //dx11 sw
		else				renderer = 1; //dx9 sw
		mt = 1;
	}
	else
	{
		// normal init
		renderer = theApp.GetConfig("renderer", 0);
	}

	*(HWND*)dsp = NULL;

	int retval = _GSopen(dsp, title, renderer);

	if( retval == 0 && s_gs )
	{
		s_gs->SetMultithreaded( !!mt );
	}

	return retval;
}

EXPORT_C GSreset()
{
	s_gs->Reset();
}

EXPORT_C GSgifSoftReset(uint32 mask)
{
	s_gs->SoftReset(mask);
}

EXPORT_C GSwriteCSR(uint32 csr)
{
	s_gs->WriteCSR(csr);
}

EXPORT_C GSreadFIFO(uint8* mem)
{
	s_gs->ReadFIFO(mem, 1);
}

EXPORT_C GSreadFIFO2(uint8* mem, uint32 size)
{
	s_gs->ReadFIFO(mem, size);
}

EXPORT_C GSgifTransfer1(uint8* mem, uint32 addr)
{
	s_gs->Transfer<0>(mem + addr, (0x4000 - addr) / 16);
}

EXPORT_C GSgifTransfer2(uint8* mem, uint32 size)
{
	s_gs->Transfer<1>(mem, size);
}

EXPORT_C GSgifTransfer3(uint8* mem, uint32 size)
{
	s_gs->Transfer<2>(mem, size);
}

EXPORT_C GSvsync(int field)
{
#ifdef _WINDOWS

	if( s_gs->m_wnd.IsManaged() )
	{
		MSG msg;

		memset(&msg, 0, sizeof(msg));

		while(msg.message != WM_QUIT && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
#endif

	s_gs->VSync(field);
}

EXPORT_C_(uint32) GSmakeSnapshot(char* path)
{
	string str = string(path);
	if (str[str.length() - 1] != '\\')
		str = str + "\\";
	return s_gs->MakeSnapshot(str + "gsdx");
}

EXPORT_C GSkeyEvent(GSKeyEventData* e)
{
	s_gs->KeyEvent(e);
}

EXPORT_C_(int) GSfreeze(int mode, GSFreezeData* data)
{
	if(mode == FREEZE_SAVE)
	{
		return s_gs->Freeze(data, false);
	}
	else if(mode == FREEZE_SIZE)
	{
		return s_gs->Freeze(data, true);
	}
	else if(mode == FREEZE_LOAD)
	{
		return s_gs->Defrost(data);
	}

	return 0;
}

EXPORT_C GSconfigure()
{
	if( !GSUtil::CheckSSE() ) return;

	if( GSSettingsDlg( s_IsGsOpen2 ).DoModal() == IDOK )
	{
		if( s_gs != NULL && s_gs->m_wnd.IsManaged() )
		{
			// Legacy apps like gsdxgui expect this...
			GSshutdown();
		}
	}
}

EXPORT_C_(INT32) GStest()
{
	if(!GSUtil::CheckSSE())
	{
		return -1;
	}

#ifdef _WINDOWS

	s_hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if(!GSUtil::CheckDirectX())
	{
		if(SUCCEEDED(s_hr))
			::CoUninitialize();

		s_hr = E_FAIL;

		return -1;
	}

	if(SUCCEEDED(s_hr))
		::CoUninitialize();

	s_hr = E_FAIL;

#endif

	return 0;
}

EXPORT_C GSabout()
{
}

EXPORT_C GSirqCallback(void (*irq)())
{
	s_irq = irq;
	if( s_gs )
	{
		s_gs->SetIrqCallback(s_irq);
	}
}

EXPORT_C_(int) GSsetupRecording(int start, void* data)
{
	GSKeyEventData e;

	e.type = KEYPRESS;
	e.key = VK_F12;

	s_gs->KeyEvent(&e, start & 1);

	return 1;
}

EXPORT_C GSsetGameCRC(uint32 crc, int options)
{
	s_gs->SetGameCRC(crc, options);
}

EXPORT_C GSgetLastTag(uint32* tag)
{
	s_gs->GetLastTag(tag);
}

EXPORT_C GSgetTitleInfo(char dest[128])
{
	//s_gs->GetWindowTitle
}

EXPORT_C GSsetFrameSkip(int frameskip)
{
	s_gs->SetFrameSkip(frameskip);
}

EXPORT_C GSsetVsync(int enabled)
{
	s_vsync = !!enabled;
	if( s_gs )
		s_gs->SetVsync(s_vsync);
}

EXPORT_C GSsetExclusive(int enabled)
{
	s_exclusive = !!enabled;
	if( s_gs )
		s_gs->SetVsync(s_vsync);
}

EXPORT_C GSsetFrameLimit(int limit)
{
	s_framelimit = !!limit;
	if( s_gs )
		s_gs->SetFrameLimit(s_framelimit);
}

#ifdef _WINDOWS

// Returns false if the window's been closed or an invalid packet was encountered.
static __forceinline bool LoopDatPacket_Thingamajig(HWND hWnd, uint8 (&regs)[0x2000], vector<uint8>& buff, FILE* fp, long start)
{
	switch(fgetc(fp))
	{
	case EOF:
		fseek(fp, start, 0);
		return !!IsWindowVisible(hWnd);

	case 0:
	{
		uint32 index = fgetc(fp);
		uint32 size;

		fread(&size, 4, 1, fp);

		switch(index)
		{
		case 0:
		{
			if(buff.size() < 0x4000) buff.resize(0x4000);
			uint32 addr = 0x4000 - size;
			fread(&buff[0] + addr, size, 1, fp);
			GSgifTransfer1(&buff[0], addr);
		}
		break;

		case 1:
			if(buff.size() < size) buff.resize(size);
			fread(&buff[0], size, 1, fp);
			GSgifTransfer2(&buff[0], size / 16);
		break;

		case 2:
			if(buff.size() < size) buff.resize(size);
			fread(&buff[0], size, 1, fp);
			GSgifTransfer3(&buff[0], size / 16);
		break;
		}
	}
	break;

	case 1:
		GSvsync(fgetc(fp));
		return !!IsWindowVisible(hWnd);

	case 2:
	{
		uint32 size;
		fread(&size, 4, 1, fp);
		if(buff.size() < size) buff.resize(size);
		GSreadFIFO2(&buff[0], size / 16);
	}
	break;

	case 3:
		fread(regs, 0x2000, 1, fp);
	break;

	default:
		return false;
	}

	return true;
}

// lpszCmdLine:
//   First parameter is the renderer.
//   Second parameter is the gs file to load and run.
EXPORT_C GSReplay(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	int renderer = -1;

	{
		char* start = lpszCmdLine;
		char* end = NULL;
		long n = strtol(lpszCmdLine, &end, 10);
		if(end > start) {renderer = n; lpszCmdLine = end;}
	}

	while(*lpszCmdLine == ' ') lpszCmdLine++;

	::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	vector<uint8> buff;

	if(FILE* fp = fopen(lpszCmdLine, "rb"))
	{
		GSinit();

		uint8 regs[0x2000];
		GSsetBaseMem(regs);

		s_vsync = !!theApp.GetConfig("vsync", 0);

		HWND hWnd = NULL;
		_GSopen(&hWnd, "", renderer);

		uint32 crc;
		fread(&crc, 4, 1, fp);
		GSsetGameCRC(crc, 0);

		GSFreezeData fd;
		fread(&fd.size, 4, 1, fp);
		fd.data = new uint8[fd.size];
		fread(fd.data, fd.size, 1, fp);
		GSfreeze(FREEZE_LOAD, &fd);
		delete [] fd.data;

		fread(regs, 0x2000, 1, fp);

		long start = ftell(fp);

		GSvsync(1);

		while( LoopDatPacket_Thingamajig(hWnd, regs, buff, fp, start) ) ;

		GSclose();
		GSshutdown();

		fclose(fp);
	}
}

EXPORT_C GSBenchmark(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	FILE* file = fopen("c:\\log.txt", "a");

	fprintf(file, "-------------------------\n\n");

	if(1)
	{
		GSLocalMemory mem;

		static struct {int psm; const char* name;} s_format[] =
		{
			{PSM_PSMCT32, "32"},
			{PSM_PSMCT24, "24"},
			{PSM_PSMCT16, "16"},
			{PSM_PSMCT16S, "16S"},
			{PSM_PSMT8, "8"},
			{PSM_PSMT4, "4"},
			{PSM_PSMT8H, "8H"},
			{PSM_PSMT4HL, "4HL"},
			{PSM_PSMT4HH, "4HH"},
			{PSM_PSMZ32, "32Z"},
			{PSM_PSMZ24, "24Z"},
			{PSM_PSMZ16, "16Z"},
			{PSM_PSMZ16S, "16ZS"},
		};

		uint8* ptr = (uint8*)_aligned_malloc(1024 * 1024 * 4, 16);

		for(int i = 0; i < 1024 * 1024 * 4; i++) ptr[i] = (uint8)i;

		//

		for(int tbw = 5; tbw <= 10; tbw++)
		{
			int n = 256 << ((10 - tbw) * 2);

			int w = 1 << tbw;
			int h = 1 << tbw;

			fprintf(file, "%d x %d\n\n", w, h);

			for(int i = 0; i < countof(s_format); i++)
			{
				const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[s_format[i].psm];

				GSLocalMemory::writeImage wi = psm.wi;
				GSLocalMemory::readImage ri = psm.ri;
				GSLocalMemory::readTexture rtx = psm.rtx;
				GSLocalMemory::readTexture rtxP = psm.rtxP;

				GIFRegBITBLTBUF BITBLTBUF;

				BITBLTBUF.SBP = 0;
				BITBLTBUF.SBW = w / 64;
				BITBLTBUF.SPSM = s_format[i].psm;
				BITBLTBUF.DBP = 0;
				BITBLTBUF.DBW = w / 64;
				BITBLTBUF.DPSM = s_format[i].psm;

				GIFRegTRXPOS TRXPOS;

				TRXPOS.SSAX = 0;
				TRXPOS.SSAY = 0;
				TRXPOS.DSAX = 0;
				TRXPOS.DSAY = 0;

				GIFRegTRXREG TRXREG;

				TRXREG.RRW = w;
				TRXREG.RRH = h;

				GSVector4i r(0, 0, w, h);

				GIFRegTEX0 TEX0;

				TEX0.TBP0 = 0;
				TEX0.TBW = w / 64;

				GIFRegTEXA TEXA;

				TEXA.TA0 = 0;
				TEXA.TA1 = 0x80;
				TEXA.AEM = 0;

				int trlen = w * h * psm.trbpp / 8;
				int len = w * h * psm.bpp / 8;

				clock_t start, end;

				_ftprintf(file, _T("[%4s] "), s_format[i].name);

				start = clock();

				for(int j = 0; j < n; j++)
				{
					int x = 0;
					int y = 0;

					(mem.*wi)(x, y, ptr, trlen, BITBLTBUF, TRXPOS, TRXREG);
				}

				end = clock();

				fprintf(file, "%6d %6d | ", (int)((float)trlen * n / (end - start) / 1000), (int)((float)(w * h) * n / (end - start) / 1000));

				start = clock();

				for(int j = 0; j < n; j++)
				{
					int x = 0;
					int y = 0;

					(mem.*ri)(x, y, ptr, trlen, BITBLTBUF, TRXPOS, TRXREG);
				}

				end = clock();

				fprintf(file, "%6d %6d | ", (int)((float)trlen * n / (end - start) / 1000), (int)((float)(w * h) * n / (end - start) / 1000));

				const GSOffset* o = mem.GetOffset(TEX0.TBP0, TEX0.TBW, TEX0.PSM);

				start = clock();

				for(int j = 0; j < n; j++)
				{
					(mem.*rtx)(o, r, ptr, w * 4, TEXA);
				}

				end = clock();

				fprintf(file, "%6d %6d ", (int)((float)len * n / (end - start) / 1000), (int)((float)(w * h) * n / (end - start) / 1000));

				if(psm.pal > 0)
				{
					start = clock();

					for(int j = 0; j < n; j++)
					{
						(mem.*rtxP)(o, r, ptr, w, TEXA);
					}

					end = clock();

					fprintf(file, "| %6d %6d ", (int)((float)len * n / (end - start) / 1000), (int)((float)(w * h) * n / (end - start) / 1000));
				}

				fprintf(file, "\n");

				fflush(file);
			}

			fprintf(file, "\n");
		}

		_aligned_free(ptr);
	}

	//

	if(0)
	{
		GSLocalMemory mem;

		uint8* ptr = (uint8*)_aligned_malloc(1024 * 1024 * 4, 16);

		for(int i = 0; i < 1024 * 1024 * 4; i++) ptr[i] = (uint8)i;

		const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[PSM_PSMCT32];

		GSLocalMemory::writeImage wi = psm.wi;

		GIFRegBITBLTBUF BITBLTBUF;

		BITBLTBUF.DBP = 0;
		BITBLTBUF.DBW = 32;
		BITBLTBUF.DPSM = PSM_PSMCT32;

		GIFRegTRXPOS TRXPOS;

		TRXPOS.DSAX = 0;
		TRXPOS.DSAY = 1;

		GIFRegTRXREG TRXREG;

		TRXREG.RRW = 256;
		TRXREG.RRH = 256;

		int trlen = 256 * 256 * psm.trbpp / 8;

		int x = 0;
		int y = 0;

		(mem.*wi)(x, y, ptr, trlen, BITBLTBUF, TRXPOS, TRXREG);
	}

	//

	fclose(file);
	PostQuitMessage(0);
}

#endif
