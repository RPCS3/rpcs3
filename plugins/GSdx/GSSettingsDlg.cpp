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
#include "GSdx.h"
#include "GSSettingsDlg.h"
#include "GSUtil.h"
#include "resource.h"

GSSetting GSSettingsDlg::g_renderers[] =
{
	{0, "Direct3D9 (Hardware)", NULL},
	{1, "Direct3D9 (Software)", NULL},
	{2, "Direct3D9 (Null)", NULL},
	{3, "Direct3D10 (Hardware)", NULL},
	{4, "Direct3D10 (Software)", NULL},
	{5, "Direct3D10 (Null)", NULL},
	#if 0
	{6, "Direct3D11 (Hardware)", NULL},
	{7, "Direct3D11 (Software)", NULL},
	{8, "Direct3D11 (Null)", NULL},
	{9, "OpenGL (Hardware)", NULL},
	{10, "OpenGL (Software)", NULL},
	{11, "OpenGL (Null)", NULL},
	#endif
	{12, "Null (Software)", NULL},
	{13, "Null (Null)", NULL},
};

GSSetting GSSettingsDlg::g_interlace[] =
{
	{0, "None", NULL},
	{1, "Weave tff", "saw-tooth"},
	{2, "Weave bff", "saw-tooth"},
	{3, "Bob tff", "use blend if shaking"},
	{4, "Bob bff", "use blend if shaking"},
	{5, "Blend tff", "slight blur, 1/2 fps"},
	{6, "Blend bff", "slight blur, 1/2 fps"},
};

GSSetting GSSettingsDlg::g_aspectratio[] =
{
	{0, "Stretch", NULL},
	{1, "4:3", NULL},
	{2, "16:9", NULL},
};

GSSetting GSSettingsDlg::g_upscale_multiplier[] =
{
	{1, "1x (Use D3D internal Res)", NULL},
	{2, "2x", NULL},
	{3, "3x", NULL},
	{4, "4x", NULL},
};

GSSettingsDlg::GSSettingsDlg()
	: GSDialog(IDD_CONFIG)
{
}

void GSSettingsDlg::OnInit()
{
	__super::OnInit();

	m_modes.clear();

	{
		D3DDISPLAYMODE mode;
		memset(&mode, 0, sizeof(mode));
		m_modes.push_back(mode);

		ComboBoxAppend(IDC_RESOLUTION, "Please select...", (LPARAM)&m_modes.back(), true);

		if(CComPtr<IDirect3D9> d3d = Direct3DCreate9(D3D_SDK_VERSION))
		{
			uint32 w = theApp.GetConfig("ModeWidth", 0);
			uint32 h = theApp.GetConfig("ModeHeight", 0);
			uint32 hz = theApp.GetConfig("ModeRefreshRate", 0);

			uint32 n = d3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_R5G6B5);

			for(uint32 i = 0; i < n; i++)
			{
				if(S_OK == d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_R5G6B5, i, &mode))
				{
					m_modes.push_back(mode);

					string str = format("%dx%d %dHz", mode.Width, mode.Height, mode.RefreshRate);

					ComboBoxAppend(IDC_RESOLUTION, str.c_str(), (LPARAM)&m_modes.back(), w == mode.Width && h == mode.Height && hz == mode.RefreshRate);
				}
			}
		}
	}

	bool isdx10avail = GSUtil::IsDirect3D10Available();
	bool isdx11avail = false; //GSUtil::IsDirect3D11Available();

	vector<GSSetting> renderers;

	for(size_t i = 0; i < countof(g_renderers); i++)
	{
		if(i >= 3 && i <= 5 && !isdx10avail) continue;
		if(i >= 6 && i <= 8 && !isdx11avail) continue;

		renderers.push_back(g_renderers[i]);
	}

	ComboBoxInit(IDC_RENDERER, &renderers[0], renderers.size(), theApp.GetConfig("Renderer", 0));
	ComboBoxInit(IDC_INTERLACE, g_interlace, countof(g_interlace), theApp.GetConfig("Interlace", 0));
	ComboBoxInit(IDC_ASPECTRATIO, g_aspectratio, countof(g_aspectratio), theApp.GetConfig("AspectRatio", 1));
	ComboBoxInit(IDC_UPSCALE_MULTIPLIER, g_upscale_multiplier, countof(g_upscale_multiplier), theApp.GetConfig("upscale_multiplier", 1));

	CheckDlgButton(m_hWnd, IDC_WINDOWED, theApp.GetConfig("windowed", 1));
	CheckDlgButton(m_hWnd, IDC_FILTER, theApp.GetConfig("filter", 2));
	CheckDlgButton(m_hWnd, IDC_PALTEX, theApp.GetConfig("paltex", 0));
	CheckDlgButton(m_hWnd, IDC_VSYNC, theApp.GetConfig("vsync", 0));
	CheckDlgButton(m_hWnd, IDC_LOGZ, theApp.GetConfig("logz", 1));
	CheckDlgButton(m_hWnd, IDC_FBA, theApp.GetConfig("fba", 1));
	CheckDlgButton(m_hWnd, IDC_AA1, theApp.GetConfig("aa1", 0));
	CheckDlgButton(m_hWnd, IDC_BLUR, theApp.GetConfig("blur", 0));
	CheckDlgButton(m_hWnd, IDC_NATIVERES, theApp.GetConfig("nativeres", 0));

	SendMessage(GetDlgItem(m_hWnd, IDC_RESX), UDM_SETRANGE, 0, MAKELPARAM(8192, 256));
	SendMessage(GetDlgItem(m_hWnd, IDC_RESX), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("resx", 1024), 0));

	SendMessage(GetDlgItem(m_hWnd, IDC_RESY), UDM_SETRANGE, 0, MAKELPARAM(8192, 256));
	SendMessage(GetDlgItem(m_hWnd, IDC_RESY), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("resy", 1024), 0));

	SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_SETRANGE, 0, MAKELPARAM(16, 1));
	SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("swthreads", 1), 0));

	UpdateControls();
}

bool GSSettingsDlg::OnCommand(HWND hWnd, UINT id, UINT code)
{
	if(id == IDC_RENDERER && code == CBN_SELCHANGE)
	{
		UpdateControls();
	}
	else if(id == IDC_NATIVERES && code == BN_CLICKED)
	{
		UpdateControls();
	}
	else if(id == IDOK)
	{
		INT_PTR data;

		if(ComboBoxGetSelData(IDC_RESOLUTION, data))
		{
			const D3DDISPLAYMODE* mode = (D3DDISPLAYMODE*)data;

			theApp.SetConfig("ModeWidth", (int)mode->Width);
			theApp.SetConfig("ModeHeight", (int)mode->Height);
			theApp.SetConfig("ModeRefreshRate", (int)mode->RefreshRate);
		}

		if(ComboBoxGetSelData(IDC_RENDERER, data))
		{
			theApp.SetConfig("Renderer", (int)data);
		}

		if(ComboBoxGetSelData(IDC_INTERLACE, data))
		{
			theApp.SetConfig("Interlace", (int)data);
		}

		if(ComboBoxGetSelData(IDC_ASPECTRATIO, data))
		{
			theApp.SetConfig("AspectRatio", (int)data);
		}
		
		if(ComboBoxGetSelData(IDC_UPSCALE_MULTIPLIER, data))
		{
			theApp.SetConfig("upscale_multiplier", (int)data);
		}
		else
		{
			theApp.SetConfig("upscale_multiplier", 1);
		}
		
		theApp.SetConfig("windowed", (int)IsDlgButtonChecked(m_hWnd, IDC_WINDOWED));
		theApp.SetConfig("filter", (int)IsDlgButtonChecked(m_hWnd, IDC_FILTER));
		theApp.SetConfig("paltex", (int)IsDlgButtonChecked(m_hWnd, IDC_PALTEX));
		theApp.SetConfig("vsync", (int)IsDlgButtonChecked(m_hWnd, IDC_VSYNC));
		theApp.SetConfig("logz", (int)IsDlgButtonChecked(m_hWnd, IDC_LOGZ));
		theApp.SetConfig("fba", (int)IsDlgButtonChecked(m_hWnd, IDC_FBA));
		theApp.SetConfig("aa1", (int)IsDlgButtonChecked(m_hWnd, IDC_AA1));
		theApp.SetConfig("blur", (int)IsDlgButtonChecked(m_hWnd, IDC_BLUR));	
		theApp.SetConfig("nativeres", (int)IsDlgButtonChecked(m_hWnd, IDC_NATIVERES));

		theApp.SetConfig("resx", (int)SendMessage(GetDlgItem(m_hWnd, IDC_RESX), UDM_GETPOS, 0, 0));
		theApp.SetConfig("resy", (int)SendMessage(GetDlgItem(m_hWnd, IDC_RESY), UDM_GETPOS, 0, 0));
		theApp.SetConfig("swthreads", (int)SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_GETPOS, 0, 0));
		
		//theApp.SetConfig("gamefix_skipdraw", 0);
	}

	return __super::OnCommand(hWnd, id, code);
}

void GSSettingsDlg::UpdateControls()
{
	INT_PTR i;

	if(ComboBoxGetSelData(IDC_RENDERER, i))
	{
		bool dx9 = i >= 0 && i <= 2;
		bool dx10 = i >= 3 && i <= 5;
		bool dx11 = i >= 6 && i <= 8;
		bool ogl = i >= 9 && i <= 12;
		bool hw = i == 0 || i == 3 || i == 6 || i == 9;
		bool sw = i == 1 || i == 4 || i == 7 || i == 10;
		bool native = !!IsDlgButtonChecked(m_hWnd, IDC_NATIVERES);

		ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO9), dx9 ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO10), dx10 ? SW_SHOW : SW_HIDE);
		// TODO: ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO11), dx11 ? SW_SHOW : SW_HIDE);
		// TODO: ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO_OGL), ogl ? SW_SHOW : SW_HIDE);

		EnableWindow(GetDlgItem(m_hWnd, IDC_WINDOWED), dx9);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESX), hw && !native);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESX_EDIT), hw && !native);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESY), hw && !native);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESY_EDIT), hw && !native);
		EnableWindow(GetDlgItem(m_hWnd, IDC_UPSCALE_MULTIPLIER), hw && !native);
		EnableWindow(GetDlgItem(m_hWnd, IDC_NATIVERES), hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_FILTER), hw && !native);		
		EnableWindow(GetDlgItem(m_hWnd, IDC_PALTEX), hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_LOGZ), dx9 && hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_FBA), dx9 && hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_AA1), sw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_SWTHREADS_EDIT), sw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_SWTHREADS), sw);
	}
}
