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
#include "GSRendererHW9.h"
#include "GSRendererHW10.h"
#include "GSRendererSW.h"
#include "GSRendererNull.h"
#include "GSSettingsDlg.h"

#define PS2E_LT_GS 0x01
#define PS2E_GS_VERSION 0x0006
#define PS2E_X86 0x01   // 32 bit
#define PS2E_X86_64 0x02   // 64 bit

static HRESULT s_hr = E_FAIL;
static GSRendererBase* s_gs = NULL;
static void (*s_irq)() = NULL;
static BYTE* s_basemem = NULL;

EXPORT_C_(UINT32) PS2EgetLibType()
{
	return PS2E_LT_GS;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return GSUtil::GetLibName();
}

EXPORT_C_(UINT32) PS2EgetLibVersion2(UINT32 type)
{
	const UINT32 revision = 0;
	const UINT32 build = 1;

	return (build << 0) | (revision << 8) | (PS2E_GS_VERSION << 16) | (PLUGIN_VERSION << 24);
}

EXPORT_C_(UINT32) PS2EgetCpuPlatform()
{
#if _M_AMD64
	return PS2E_X86_64;
#else
	return PS2E_X86;
#endif
}

EXPORT_C GSsetBaseMem(BYTE* mem)
{
	s_basemem = mem - 0x12000000;
}

EXPORT_C_(INT32) GSinit()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	return 0;
}

EXPORT_C GSshutdown()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
}

EXPORT_C GSclose()
{
	delete s_gs; 
	
	s_gs = NULL;

	if(SUCCEEDED(s_hr))
	{
		::CoUninitialize();

		s_hr = E_FAIL;
	}
}

static INT32 GSopen(void* dsp, char* title, int mt, int renderer)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if(!GSUtil::CheckDirectX() || !GSUtil::CheckSSE())
	{
		return -1;
	}

	GSclose();

	GSRendererSettings rs;

	rs.m_interlace = AfxGetApp()->GetProfileInt(_T("Settings"), _T("interlace"), 0);
	rs.m_aspectratio = AfxGetApp()->GetProfileInt(_T("Settings"), _T("aspectratio"), 1);
	rs.m_filter = AfxGetApp()->GetProfileInt(_T("Settings"), _T("filter"), 1);
	rs.m_vsync = !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("vsync"), FALSE);
	rs.m_nativeres = !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("nativeres"), FALSE);
	rs.m_aa1 = !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("aa1"), FALSE);
	rs.m_blur = !!AfxGetApp()->GetProfileInt(_T("Settings"), _T("blur"), FALSE);

	int threads = AfxGetApp()->GetProfileInt(_T("Settings"), _T("swthreads"), 1);

	switch(renderer)
	{
	default: 
	case 0: s_gs = new GSRendererHW9(s_basemem, !!mt, s_irq, rs); break;
	case 1: s_gs = new GSRendererSW<GSDevice9>(s_basemem, !!mt, s_irq, rs, threads); break;
	case 2: s_gs = new GSRendererNull<GSDevice9>(s_basemem, !!mt, s_irq, rs); break;
	case 3: s_gs = new GSRendererHW10(s_basemem, !!mt, s_irq, rs); break;
	case 4: s_gs = new GSRendererSW<GSDevice10>(s_basemem, !!mt, s_irq, rs, threads); break;
	case 5: s_gs = new GSRendererNull<GSDevice10>(s_basemem, !!mt, s_irq, rs); break;
	case 6: s_gs = new GSRendererSW<GSDeviceNull>(s_basemem, !!mt, s_irq, rs, threads); break;
	case 7: s_gs = new GSRendererNull<GSDeviceNull>(s_basemem, !!mt, s_irq, rs); break;
	}

	s_hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if(!s_gs->Create(CString(title)))
	{
		GSclose();

		return -1;
	}

	s_gs->m_wnd.Show();

	*(HWND*)dsp = s_gs->m_wnd;

	// if(mt) _mm_setcsr(MXCSR);

	return 0;
}

EXPORT_C_(INT32) GSopen(void* dsp, char* title, int mt)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	int renderer;
	
	if (mt == 2){ //pcsx2 sent a switch renderer request
		renderer = 1; //DX9 sw
		mt = 1;	
	}
	else { //normal init
		renderer = AfxGetApp()->GetProfileInt(_T("Settings"), _T("renderer"), 0);
	}
	return GSopen(dsp, title, mt, renderer);
}

EXPORT_C GSreset()
{
	s_gs->Reset();
}

EXPORT_C GSgifSoftReset(int mask)
{
	s_gs->SoftReset((BYTE)mask);
}

EXPORT_C GSwriteCSR(UINT32 csr)
{
	s_gs->WriteCSR(csr);
}

EXPORT_C GSreadFIFO(BYTE* mem)
{
	s_gs->ReadFIFO(mem, 1);
}

EXPORT_C GSreadFIFO2(BYTE* mem, UINT32 size)
{
	s_gs->ReadFIFO(mem, size);
}

EXPORT_C GSgifTransfer1(BYTE* mem, UINT32 addr)
{
	s_gs->Transfer<0>(mem + addr, (0x4000 - addr) / 16);
}

EXPORT_C GSgifTransfer2(BYTE* mem, UINT32 size)
{
	s_gs->Transfer<1>(mem, size);
}

EXPORT_C GSgifTransfer3(BYTE* mem, UINT32 size)
{
	s_gs->Transfer<2>(mem, size);
}

EXPORT_C GSvsync(int field)
{
	s_gs->VSync(field);
}

EXPORT_C_(UINT32) GSmakeSnapshot(char* path)
{
	return s_gs->MakeSnapshot(CString(path) + _T("gsdx"));
}

EXPORT_C GSkeyEvent(keyEvent* ev)
{
}

EXPORT_C_(INT32) GSfreeze(int mode, GSFreezeData* data)
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
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	GSSettingsDlg dlg;

	if(IDOK == dlg.DoModal())
	{
		GSshutdown();
		GSinit();
	}
}

EXPORT_C_(INT32) GStest()
{
	return 0;

	// TODO

	/*
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CComPtr<ID3D10Device> dev;

	return SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &dev)) ? 0 : -1;
	*/
}

EXPORT_C GSabout()
{
}

EXPORT_C GSirqCallback(void (*irq)())
{
	s_irq = irq;
}

EXPORT_C GSsetGameCRC(DWORD crc, int options)
{
	s_gs->SetGameCRC(crc, options);
}

EXPORT_C GSgetLastTag(UINT32* tag) 
{
	s_gs->GetLastTag(tag);
}

EXPORT_C GSsetFrameSkip(int frameskip)
{
	s_gs->SetFrameSkip(frameskip);
}

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

	CAtlArray<BYTE> buff;

	if(FILE* fp = fopen(lpszCmdLine, "rb"))
	{
		GSinit();

		BYTE regs[0x2000];
		GSsetBaseMem(regs);

		HWND hWnd = NULL;
		GSopen(&hWnd, _T(""), true, renderer);

		DWORD crc;
		fread(&crc, 4, 1, fp);
		GSsetGameCRC(crc, 0);

		GSFreezeData fd;
		fread(&fd.size, 4, 1, fp);
		fd.data = new BYTE[fd.size];
		fread(fd.data, fd.size, 1, fp);
		GSfreeze(FREEZE_LOAD, &fd);
		delete [] fd.data;

		fread(regs, 0x2000, 1, fp);

		long start = ftell(fp);

		unsigned int index, size, addr;

		GSvsync(1);

		while(1)
		{
			switch(fgetc(fp))
			{
			case EOF:
				fseek(fp, start, 0);
				if(!IsWindowVisible(hWnd)) return;
				break;
			case 0:
				index = fgetc(fp);
				fread(&size, 4, 1, fp);
				switch(index)
				{
				case 0:
					if(buff.GetCount() < 0x4000) buff.SetCount(0x4000);
					addr = 0x4000 - size;
					fread(buff.GetData() + addr, size, 1, fp);
					GSgifTransfer1(buff.GetData(), addr);
					break;
				case 1:
					if(buff.GetCount() < size) buff.SetCount(size);
					fread(buff.GetData(), size, 1, fp);
					GSgifTransfer2(buff.GetData(), size / 16);
					break;
				case 2:
					if(buff.GetCount() < size) buff.SetCount(size);
					fread(buff.GetData(), size, 1, fp);
					GSgifTransfer3(buff.GetData(), size / 16);
					break;
				}
				break;
			case 1:
				GSvsync(fgetc(fp));
				if(!IsWindowVisible(hWnd)) return;
				break;
			case 2:
				fread(&size, 4, 1, fp);
				if(buff.GetCount() < size) buff.SetCount(size);
				GSreadFIFO2(buff.GetData(), size / 16);
				break;
			case 3:
				fread(regs, 0x2000, 1, fp);
				break;
			default:
				return;
			}
		}

		GSclose();

		GSshutdown();

		fclose(fp);
	}
}

EXPORT_C GSBenchmark(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	FILE* file = _tfopen(_T("c:\\log.txt"), _T("a"));

	_ftprintf(file, _T("-------------------------\n\n"));

	if(1)
	{
		GSLocalMemory mem;

		static struct {int psm; LPCSTR name;} s_format[] = 
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

		BYTE* ptr = (BYTE*)_aligned_malloc(1024 * 1024 * 4, 16);

		for(int i = 0; i < 1024 * 1024 * 4; i++) ptr[i] = (BYTE)i;

		// 

		for(int tbw = 5; tbw <= 10; tbw++)
		{
			int n = 256 << ((10 - tbw) * 2);

			int w = 1 << tbw;
			int h = 1 << tbw;

			_ftprintf(file, _T("%d x %d\n\n"), w, h);

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

				CRect r(0, 0, w, h);

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

				_ftprintf(file, _T("%6d %6d | "), (int)((float)trlen * n / (end - start) / 1000), (int)((float)(w * h) * n / (end - start) / 1000));

				start = clock();

				for(int j = 0; j < n; j++)
				{
					int x = 0;
					int y = 0;

					(mem.*ri)(x, y, ptr, trlen, BITBLTBUF, TRXPOS, TRXREG);
				}

				end = clock();

				_ftprintf(file, _T("%6d %6d | "), (int)((float)trlen * n / (end - start) / 1000), (int)((float)(w * h) * n / (end - start) / 1000));

				start = clock();

				for(int j = 0; j < n; j++)
				{
					(mem.*rtx)(r, ptr, w * 4, TEX0, TEXA);
				}

				end = clock();

				_ftprintf(file, _T("%6d %6d "), (int)((float)len * n / (end - start) / 1000), (int)((float)(w * h) * n / (end - start) / 1000));

				if(psm.pal > 0)
				{
					start = clock();

					for(int j = 0; j < n; j++)
					{
						(mem.*rtxP)(r, ptr, w, TEX0, TEXA);
					}

					end = clock();

					_ftprintf(file, _T("| %6d %6d "), (int)((float)len * n / (end - start) / 1000), (int)((float)(w * h) * n / (end - start) / 1000));
				}

				_ftprintf(file, _T("\n"));

				fflush(file);
			}

			_ftprintf(file, _T("\n"));
		}

		_aligned_free(ptr);
	}

	//

	if(0)
	{
		GSLocalMemory mem;

		BYTE* ptr = (BYTE*)_aligned_malloc(1024 * 1024 * 4, 16);

		for(int i = 0; i < 1024 * 1024 * 4; i++) ptr[i] = (BYTE)i;

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
}

