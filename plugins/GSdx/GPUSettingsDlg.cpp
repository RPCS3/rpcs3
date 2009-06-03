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
#include "GSUtil.h"
#include "GPUSettingsDlg.h"
#include "resource.h"

GSSetting GPUSettingsDlg::g_renderers[] =
{
	{0, "Direct3D7 (Software)", NULL},
	{1, "Direct3D9 (Software)", NULL},
	{2, "Direct3D10 (Software)", NULL},
//	{3, "Null (Null)", NULL},
};

GSSetting GPUSettingsDlg::g_psversion[] =
{
	{D3DPS_VERSION(3, 0), "Pixel Shader 3.0", NULL},
	{D3DPS_VERSION(2, 0), "Pixel Shader 2.0", NULL},
	//{D3DPS_VERSION(1, 4), "Pixel Shader 1.4", NULL},
	//{D3DPS_VERSION(1, 1), "Pixel Shader 1.1", NULL},
	//{D3DPS_VERSION(0, 0), "Fixed Pipeline (bogus)", NULL},
};

GSSetting GPUSettingsDlg::g_filter[] =
{
	{0, "Nearest", NULL},
	{1, "Bilinear (polygons only)", NULL},
	{2, "Bilinear", NULL},
};

GSSetting GPUSettingsDlg::g_dithering[] =
{
	{0, "Disabled", NULL},
	{1, "Auto", NULL},
};

GSSetting GPUSettingsDlg::g_aspectratio[] =
{
	{0, "Stretch", NULL},
	{1, "4:3", NULL},
	{2, "16:9", NULL},
};

GSSetting GPUSettingsDlg::g_scale[] =
{
	{0 | (0 << 2), "H x 1 - V x 1", NULL},
	{1 | (0 << 2), "H x 2 - V x 1", NULL},
	{0 | (1 << 2), "H x 1 - V x 2", NULL},
	{1 | (1 << 2), "H x 2 - V x 2", NULL},
	{2 | (1 << 2), "H x 4 - V x 2", NULL},
	{1 | (2 << 2), "H x 2 - V x 4", NULL},
	{2 | (2 << 2), "H x 4 - V x 4", NULL},
};

GPUSettingsDlg::GPUSettingsDlg()
	: GSDialog(IDD_GPUCONFIG)
{
}

void GPUSettingsDlg::OnInit()
{
	__super::OnInit();

	D3DCAPS9 caps;
	memset(&caps, 0, sizeof(caps));
	caps.PixelShaderVersion = D3DPS_VERSION(0, 0);

	m_modes.clear();

	{
		D3DDISPLAYMODE mode;
		memset(&mode, 0, sizeof(mode));
		m_modes.push_back(mode);

		HWND hWnd = GetDlgItem(m_hWnd, IDC_RESOLUTION);

		ComboBoxAppend(hWnd, "Windowed", (LPARAM)&m_modes.back(), true);

		if(CComPtr<IDirect3D9> d3d = Direct3DCreate9(D3D_SDK_VERSION))
		{
			uint32 w = theApp.GetConfig("ModeWidth", 0);
			uint32 h = theApp.GetConfig("ModeHeight", 0);
			uint32 hz = theApp.GetConfig("ModeRefreshRate", 0);

			uint32 n = d3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);

			for(uint32 i = 0; i < n; i++)
			{
				if(S_OK == d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &mode))
				{
					m_modes.push_back(mode);

					string str = format("%dx%d %dHz", mode.Width, mode.Height, mode.RefreshRate);

					ComboBoxAppend(hWnd, str.c_str(), (LPARAM)&m_modes.back(), w == mode.Width && h == mode.Height && hz == mode.RefreshRate);
				}
			}

			d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
		}
	}

	{
		bool isdx10avail = GSUtil::IsDirect3D10Available();

		vector<GSSetting> renderers;

		for(size_t i = 0; i < countof(g_renderers); i++)
		{
			if(i >= 3 && i <= 5 && !isdx10avail) continue;

			renderers.push_back(g_renderers[i]);
		}

		HWND hWnd = GetDlgItem(m_hWnd, IDC_RENDERER);

		ComboBoxInit(hWnd, &renderers[0], renderers.size(), theApp.GetConfig("Renderer", 0));

		OnCommand(hWnd, IDC_RENDERER, CBN_SELCHANGE);
	}

	ComboBoxInit(GetDlgItem(m_hWnd, IDC_SHADER), g_psversion, countof(g_psversion), theApp.GetConfig("PixelShaderVersion2", D3DPS_VERSION(2, 0)), caps.PixelShaderVersion);
	ComboBoxInit(GetDlgItem(m_hWnd, IDC_FILTER), g_filter, countof(g_filter), theApp.GetConfig("filter", 0));
	ComboBoxInit(GetDlgItem(m_hWnd, IDC_DITHERING), g_dithering, countof(g_dithering), theApp.GetConfig("dithering", 1));
	ComboBoxInit(GetDlgItem(m_hWnd, IDC_ASPECTRATIO), g_aspectratio, countof(g_aspectratio), theApp.GetConfig("AspectRatio", 1));
	ComboBoxInit(GetDlgItem(m_hWnd, IDC_SCALE), g_scale, countof(g_scale), theApp.GetConfig("scale_x", 0) | (theApp.GetConfig("scale_y", 0) << 2));

	SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_SETRANGE, 0, MAKELPARAM(16, 1));
	SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("swthreads", 1), 0));
}

bool GPUSettingsDlg::OnCommand(HWND hWnd, UINT id, UINT code)
{
	if(id == IDC_RENDERER && code == CBN_SELCHANGE)
	{
		int item = (int)SendMessage(hWnd, CB_GETCURSEL, 0, 0);

		if(item >= 0)
		{
			int i = (int)SendMessage(hWnd, CB_GETITEMDATA, item, 0);

			ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO9), i == 1 ? SW_SHOW : SW_HIDE);
			ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO10), i == 2 ? SW_SHOW : SW_HIDE);
		}
	}
	else if(id == IDOK)
	{
		INT_PTR data;

		if(ComboBoxGetSelData(GetDlgItem(m_hWnd, IDC_RESOLUTION), data))
		{
			const D3DDISPLAYMODE* mode = (D3DDISPLAYMODE*)data;

			theApp.SetConfig("ModeWidth", (int)mode->Width);
			theApp.SetConfig("ModeHeight", (int)mode->Height);
			theApp.SetConfig("ModeRefreshRate", (int)mode->RefreshRate);
		}

		if(ComboBoxGetSelData(GetDlgItem(m_hWnd, IDC_RENDERER), data))
		{
			theApp.SetConfig("Renderer", (int)data);
		}

		if(ComboBoxGetSelData(GetDlgItem(m_hWnd, IDC_SHADER), data))
		{
			theApp.SetConfig("PixelShaderVersion2", (int)data);
		}

		if(ComboBoxGetSelData(GetDlgItem(m_hWnd, IDC_FILTER), data))
		{
			theApp.SetConfig("filter", (int)data);
		}

		if(ComboBoxGetSelData(GetDlgItem(m_hWnd, IDC_DITHERING), data))
		{
			theApp.SetConfig("dithering", (int)data);
		}

		if(ComboBoxGetSelData(GetDlgItem(m_hWnd, IDC_ASPECTRATIO), data))
		{
			theApp.SetConfig("AspectRatio", (int)data);
		}

		if(ComboBoxGetSelData(GetDlgItem(m_hWnd, IDC_SCALE), data))
		{
			theApp.SetConfig("scale_x", data & 3);
			theApp.SetConfig("scale_y", (data >> 2) & 3);
		}

		theApp.SetConfig("swthreads", (int)SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_GETPOS, 0, 0));
	}

	return __super::OnCommand(hWnd, id, code);
}