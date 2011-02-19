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

GPUSettingsDlg::GPUSettingsDlg()
	: GSDialog(IDD_GPUCONFIG)
{
}

void GPUSettingsDlg::OnInit()
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

			uint32 n = d3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);

			for(uint32 i = 0; i < n; i++)
			{
				if(S_OK == d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &mode))
				{
					m_modes.push_back(mode);

					string str = format("%dx%d %dHz", mode.Width, mode.Height, mode.RefreshRate);

					ComboBoxAppend(IDC_RESOLUTION, str.c_str(), (LPARAM)&m_modes.back(), w == mode.Width && h == mode.Height && hz == mode.RefreshRate);
				}
			}
		}
	}

	ComboBoxInit(IDC_RENDERER, theApp.m_gpu_renderers, theApp.GetConfig("Renderer", 0));
	ComboBoxInit(IDC_FILTER, theApp.m_gpu_filter, theApp.GetConfig("filter", 0));
	ComboBoxInit(IDC_DITHERING, theApp.m_gpu_dithering, theApp.GetConfig("dithering", 1));
	ComboBoxInit(IDC_ASPECTRATIO, theApp.m_gpu_aspectratio, theApp.GetConfig("AspectRatio", 1));
	ComboBoxInit(IDC_SCALE, theApp.m_gpu_scale, theApp.GetConfig("scale_x", 0) | (theApp.GetConfig("scale_y", 0) << 2));

	CheckDlgButton(m_hWnd, IDC_WINDOWED, theApp.GetConfig("windowed", 1));

	SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_SETRANGE, 0, MAKELPARAM(16, 1));
	SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("swthreads", 1), 0));

	UpdateControls();
}

bool GPUSettingsDlg::OnCommand(HWND hWnd, UINT id, UINT code)
{
	if(id == IDC_RENDERER && code == CBN_SELCHANGE)
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

		if(ComboBoxGetSelData(IDC_FILTER, data))
		{
			theApp.SetConfig("filter", (int)data);
		}

		if(ComboBoxGetSelData(IDC_DITHERING, data))
		{
			theApp.SetConfig("dithering", (int)data);
		}

		if(ComboBoxGetSelData(IDC_ASPECTRATIO, data))
		{
			theApp.SetConfig("AspectRatio", (int)data);
		}

		if(ComboBoxGetSelData(IDC_SCALE, data))
		{
			theApp.SetConfig("scale_x", data & 3);
			theApp.SetConfig("scale_y", (data >> 2) & 3);
		}

		theApp.SetConfig("swthreads", (int)SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_GETPOS, 0, 0));
		theApp.SetConfig("windowed", (int)IsDlgButtonChecked(m_hWnd, IDC_WINDOWED));
	}

	return __super::OnCommand(hWnd, id, code);
}

void GPUSettingsDlg::UpdateControls()
{
	INT_PTR i;

	if(ComboBoxGetSelData(IDC_RENDERER, i))
	{
		bool dx9 = i == 0;
		bool dx11 = i == 1;
		bool sw = i >= 0 && i <= 1;

		ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO9), dx9 ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO11), dx11 ? SW_SHOW : SW_HIDE);
		
		EnableWindow(GetDlgItem(m_hWnd, IDC_SCALE), sw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_SWTHREADS_EDIT), sw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_SWTHREADS), sw);
	}
}