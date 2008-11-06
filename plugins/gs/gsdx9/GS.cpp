/* 
 *	Copyright (C) 2003-2005 Gabest
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
#include "GSdx9.h"
#include "GS.h"
#include "GSRendererHW.h"
#include "GSRendererSoft.h"
#include "GSRendererNull.h"
#include "GSWnd.h"
#include "GSSettingsDlg.h"

#define PS2E_LT_GS 0x01
#define PS2E_GS_VERSION 0x0006
#define PS2E_DLL_VERSION 0x09
#define PS2E_X86 0x01   // 32 bit
#define PS2E_X86_64 0x02   // 64 bit

EXPORT_C_(UINT32) PS2EgetLibType()
{
	return PS2E_LT_GS;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	CString str = _T("GSdx9");

#if _M_AMD64
	str += _T(" 64-bit");
#endif

	CList<CString> sl;

#ifdef __INTEL_COMPILER
	CString s;
	s.Format(_T("Intel C++ %d.%02d"), __INTEL_COMPILER/100, __INTEL_COMPILER%100);
	sl.AddTail(s);
#elif _MSC_VER
	CString s;
	s.Format(_T("MSVC %d.%02d"), _MSC_VER/100, _MSC_VER%100);
	sl.AddTail(s);
#endif

#if _M_IX86_FP >= 2
	sl.AddTail(_T("SSE2"));
#elif _M_IX86_FP >= 1
	sl.AddTail(_T("SSE"));
#endif

#ifdef _OPENMP
	sl.AddTail(_T("OpenMP"));
#endif

	POSITION pos = sl.GetHeadPosition();
	while(pos)
	{
		if(pos == sl.GetHeadPosition()) str += _T(" (");
		str += sl.GetNext(pos);
		str += pos ? _T(", ") : _T(")");
	}

	static char buff[256];
	strncpy(buff, CStringA(str), min(countof(buff)-1, str.GetLength()));
	return buff;
}

EXPORT_C_(UINT32) PS2EgetLibVersion2(UINT32 type)
{
	return (PS2E_GS_VERSION<<16)|(0x00<<8)|PS2E_DLL_VERSION;
}

EXPORT_C_(UINT32) PS2EgetCpuPlatform()
{
#if _M_AMD64
	return PS2E_X86_64;
#else
	return PS2E_X86;
#endif
}

//////////////////

#define REPLAY_TITLE "Replay"

static HRESULT s_hrCoInit = E_FAIL;
static CGSWnd s_hWnd;
static CAutoPtr<GSState> s_gs;
static void (*s_fpGSirq)() = NULL;

BYTE* g_pBasePS2Mem = NULL;

EXPORT_C GSsetBaseMem(BYTE* pBasePS2Mem)
{
	g_pBasePS2Mem = pBasePS2Mem - 0x12000000;
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

EXPORT_C_(INT32) GSopen(void* pDsp, char* Title, int multithread)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	//

	s_hrCoInit = ::CoInitialize(0);

	ASSERT(!s_gs);
	s_gs.Free();

	if(!s_hWnd.Create(_T("PCSX2")))
		return -1;

	HRESULT hr;

	switch(AfxGetApp()->GetProfileInt(_T("Settings"), _T("Renderer"), RENDERER_D3D_HW))
	{
	case RENDERER_D3D_HW: s_gs.Attach(new GSRendererHW(s_hWnd, hr)); break;
	// case RENDERER_D3D_SW_FX: s_gs.Attach(new GSRendererSoftFX(s_hWnd, hr)); break;
	case RENDERER_D3D_SW_FP: s_gs.Attach(new GSRendererSoftFP(s_hWnd, hr)); break;
	case RENDERER_D3D_NULL: s_gs.Attach(new GSRendererNull(s_hWnd, hr)); break;
	}

	if(!s_gs || FAILED(hr))
	{
		s_gs.Free();
		s_hWnd.DestroyWindow();
		return -1;
	}

	//

	if(!IsWindow(s_hWnd))
		return -1;

	*(HWND*)pDsp = s_hWnd;

	s_gs->ResetDevice();

	s_gs->GSirq(s_fpGSirq);

	s_gs->m_fMultiThreaded = !!multithread;

	if((!Title || strcmp(Title, REPLAY_TITLE) != 0) 
	&& AfxGetApp()->GetProfileInt(_T("Settings"), _T("RecordState"), FALSE))
	{
		CPath spath = AfxGetApp()->GetProfileString(_T("Settings"), _T("RecordStatePath"), _T(""));
		CString fn;
		fn.Format(_T("gsdx9_%s.gs"), CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S")));
		spath.Append(fn);
		s_gs->CaptureState(spath);
	}

	s_hWnd.SetWindowText(CString(Title));

	s_hWnd.Show();

	return 0;
}

EXPORT_C GSclose()
{
	s_hWnd.Show(false);

	ASSERT(s_gs);
	s_gs.Free();

	s_hWnd.DestroyWindow();

	if(SUCCEEDED(s_hrCoInit)) ::CoUninitialize();
}

EXPORT_C GSreset()
{
	s_gs->Reset();
}

EXPORT_C GSwriteCSR(UINT32 csr)
{
	s_gs->WriteCSR(csr);
}

EXPORT_C GSreadFIFO(BYTE* pMem)
{
	s_gs->ReadFIFO(pMem);
}

EXPORT_C GSgifTransfer1(BYTE* pMem, UINT32 addr)
{
	s_gs->Transfer1(pMem, addr);
}

EXPORT_C GSgifTransfer2(BYTE* pMem, UINT32 size)
{
	s_gs->Transfer2(pMem, size);
}

EXPORT_C GSgifTransfer3(BYTE* pMem, UINT32 size)
{
	s_gs->Transfer3(pMem, size);
}

extern int Path3hack;

EXPORT_C GSgetLastTag(UINT64* ptag)
{
	*(UINT32*)ptag = Path3hack;
	Path3hack = 0;
}

EXPORT_C GSvsync(int field)
{
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while(msg.message != WM_QUIT)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// TODO
			// Sleep(40);
			s_gs->VSync(field);
			break;
		}
	}
}

////////

EXPORT_C_(UINT32) GSmakeSnapshot(char* path)
{
	return s_gs->MakeSnapshot(path);
}



EXPORT_C GSkeyEvent(keyEvent* ev)
{
	if(ev->event != KEYPRESS) return;

	switch(ev->key)
	{
		case VK_INSERT:
			s_gs->Capture();
			break;

		case VK_DELETE:
			s_gs->ToggleOSD();
			break;

		default:
			break;
	}
}

EXPORT_C_(INT32) GSfreeze(int mode, freezeData* data)
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

	if(IDOK == CGSSettingsDlg().DoModal())
	{
		GSshutdown();
		GSinit();
	}
}

EXPORT_C_(INT32) GStest()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	int ret = 0;

	D3DCAPS9 caps;
	ZeroMemory(&caps, sizeof(caps));
	caps.PixelShaderVersion = D3DPS_VERSION(0, 0);

	if(CComPtr<IDirect3D9> pD3D = Direct3DCreate9(D3D_SDK_VERSION))
	{
		pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, CGSdx9App::D3DDEVTYPE_X, &caps);

		LPCTSTR yep = _T("^_^"), nope = _T(":'(");

		CString str, tmp;

		if(caps.PixelShaderVersion < D3DPS_VERSION(1, 4))
			ret = -1;

		tmp.Format(_T("%s Pixel Shader version %d.%d\n"), 
			caps.PixelShaderVersion >= D3DPS_VERSION(1, 4) ? yep : nope,
			D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion),
			D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion));
		str += tmp;

		if(!(caps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND))
			ret = -1;

		tmp.Format(_T("%s Separate Alpha Blend\n"), 
			!!(caps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND) ? yep : nope);
		str += tmp;

		if(!(caps.SrcBlendCaps & D3DPBLENDCAPS_BLENDFACTOR)
		|| !(caps.DestBlendCaps & D3DPBLENDCAPS_BLENDFACTOR))
			ret = -1;

		tmp.Format(_T("%s Source Blend Factor\n"), 
			!!(caps.SrcBlendCaps & D3DPBLENDCAPS_BLENDFACTOR) ? yep : nope);
		str += tmp;

		tmp.Format(_T("%s Destination Blend Factor\n"), 
			!!(caps.DestBlendCaps & D3DPBLENDCAPS_BLENDFACTOR) ? yep : nope);
		str += tmp;

		AfxMessageBox(str);
	}
	else
	{
		ret = -1;
	}

	return ret;
}

EXPORT_C GSabout()
{
}

EXPORT_C GSirqCallback(void (*fpGSirq)())
{
	s_fpGSirq = fpGSirq;
	// if(s_gs) s_gs->GSirq(fpGSirq);
}

/////////////////
/*
EXPORT_C GSReplay(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	if(!GSinit())
	{
		HWND hWnd = NULL;
		if(!GSopen((void*)&hWnd, REPLAY_TITLE))
		{
			if(FILE* sfp = _tfopen(lpszCmdLine, _T("rb")))
			{
				BYTE* buff = (BYTE*)_aligned_malloc(4*1024*1024, 16);

				while(!feof(sfp))
				{
					switch(fgetc(sfp))
					{
					case ST_WRITE:
						{
						GS_REG mem;
						UINT64 value, mask;
						fread(&mem, 4, 1, sfp);
						fread(&value, 8, 1, sfp);
						fread(&mask, 8, 1, sfp);
						switch(mask)
						{
						case 0xff: GSwrite8(mem, (UINT8)value); break;
						case 0xffff: GSwrite16(mem, (UINT16)value); break;
						case 0xffffffff: GSwrite32(mem, (UINT32)value); break;
						case 0xffffffffffffffff: GSwrite64(mem, value); break;
						}
						break;
						}
					case ST_TRANSFER:
						{
						UINT32 size = 0;
						fread(&size, 4, 1, sfp);
						UINT32 len = 0;
						fread(&len, 4, 1, sfp);
						if(len > 4*1024*1024) {ASSERT(0); break;}
						fread(buff, len, 1, sfp);
						GSgifTransfer3(buff, size);
						break;
						}
					case ST_VSYNC:
						GSvsync();
						break;
					}
				}

				_aligned_free(buff);
			}

			GSclose();
		}
		
		GSshutdown();
	}
}
*/