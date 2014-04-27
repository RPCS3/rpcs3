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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSdx.h"
#include "GSSettingsDlg.h"
#include "GSUtil.h"
#include "GSDevice9.h"
#include "GSDevice11.h"
#include "resource.h"

GSSettingsDlg::GSSettingsDlg(bool isOpen2)
	: GSDialog(isOpen2 ? IDD_CONFIG2 : IDD_CONFIG)
	, m_IsOpen2(isOpen2)
{
}

void GSSettingsDlg::OnInit()
{
	__super::OnInit();

	m_modes.clear();

	CComPtr<IDirect3D9> d3d9;
	d3d9.Attach(Direct3DCreate9(D3D_SDK_VERSION));

	CComPtr<IDXGIFactory1> dxgi_factory;
	if (GSUtil::CheckDXGI())
		CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&dxgi_factory);

	if(!m_IsOpen2)
	{
		D3DDISPLAYMODE mode;
		memset(&mode, 0, sizeof(mode));
		m_modes.push_back(mode);

		ComboBoxAppend(IDC_RESOLUTION, "Please select...", (LPARAM)&m_modes.back(), true);

		if(d3d9)
		{
			uint32 w = theApp.GetConfig("ModeWidth", 0);
			uint32 h = theApp.GetConfig("ModeHeight", 0);
			uint32 hz = theApp.GetConfig("ModeRefreshRate", 0);

			uint32 n = d3d9->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_R5G6B5);

			for(uint32 i = 0; i < n; i++)
			{
				if(S_OK == d3d9->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_R5G6B5, i, &mode))
				{
					m_modes.push_back(mode);

					string str = format("%dx%d %dHz", mode.Width, mode.Height, mode.RefreshRate);

					ComboBoxAppend(IDC_RESOLUTION, str.c_str(), (LPARAM)&m_modes.back(), w == mode.Width && h == mode.Height && hz == mode.RefreshRate);
				}
			}
		}
	}

	adapters.clear();

	adapters.push_back(Adapter("Default Hardware Device", "default", GSUtil::CheckDirect3D11Level(NULL, D3D_DRIVER_TYPE_HARDWARE)));
	adapters.push_back(Adapter("Reference Device", "ref", GSUtil::CheckDirect3D11Level(NULL, D3D_DRIVER_TYPE_REFERENCE)));

	if (dxgi_factory)
	{
		for (int i = 0;; i++)
		{
			CComPtr<IDXGIAdapter1> adapter;
			if (S_OK != dxgi_factory->EnumAdapters1(i, &adapter))
				break;
			DXGI_ADAPTER_DESC1 desc;
			HRESULT hr = adapter->GetDesc1(&desc);
			if (S_OK == hr)
			{
				D3D_FEATURE_LEVEL level = GSUtil::CheckDirect3D11Level(adapter, D3D_DRIVER_TYPE_UNKNOWN);
// GSDX isn't unicode!?
#if 1
				int size = WideCharToMultiByte(CP_ACP, 0,
					desc.Description, sizeof(desc.Description),
					NULL, 0,
					NULL, NULL);
				char *buf = new char[size];
				WideCharToMultiByte(CP_ACP, 0,
					desc.Description, sizeof(desc.Description),
					buf, size,
					NULL, NULL);
				adapters.push_back(Adapter(buf, GSAdapter(desc), level));
				delete [] buf;
#else
				adapters.push_back(Adapter(desc.Description, GSAdapter(desc), level));
#endif
			}
		}
	}
	else if (d3d9)
	{
		int n = d3d9->GetAdapterCount();
		for (int i = 0; i < n; i++)
		{
			D3DADAPTER_IDENTIFIER9 desc;
			if (D3D_OK != d3d9->GetAdapterIdentifier(i, 0, &desc))
				break;
// GSDX isn't unicode!?
#if 0
			wchar_t buf[sizeof desc.Description * sizeof(WCHAR)];
			MultiByteToWideChar(CP_ACP /* I have no idea if this is right */, 0,
				desc.Description, sizeof(desc.Description),
				buf, sizeof buf / sizeof *buf);
			adapters.push_back(Adapter(buf, GSAdapter(desc), (D3D_FEATURE_LEVEL)0));
#else
			adapters.push_back(Adapter(desc.Description, GSAdapter(desc), (D3D_FEATURE_LEVEL)0));
#endif
		}
	}

	std::string adapter_setting = theApp.GetConfig("Adapter", "default");
	vector<GSSetting> adapter_settings;
	unsigned adapter_sel = 0;

	for (unsigned i = 0; i < adapters.size(); i++)
	{
		if (adapters[i].id == adapter_setting)
			adapter_sel = i;
		adapter_settings.push_back(GSSetting(i, adapters[i].name.c_str(), ""));
	}

	ComboBoxInit(IDC_ADAPTER, adapter_settings, adapter_sel);
	UpdateRenderers();
	ComboBoxInit(IDC_INTERLACE, theApp.m_gs_interlace, theApp.GetConfig("Interlace", 7)); // 7 = "auto", detects interlace based on SMODE2 register
	ComboBoxInit(IDC_ASPECTRATIO, theApp.m_gs_aspectratio, theApp.GetConfig("AspectRatio", 1));
	ComboBoxInit(IDC_UPSCALE_MULTIPLIER, theApp.m_gs_upscale_multiplier, theApp.GetConfig("upscale_multiplier", 1));
	ComboBoxInit(IDC_AFCOMBO, theApp.m_gs_max_anisotropy, theApp.GetConfig("MaxAnisotropy", 0));

	CheckDlgButton(m_hWnd, IDC_WINDOWED, theApp.GetConfig("windowed", 1));
	CheckDlgButton(m_hWnd, IDC_FILTER, theApp.GetConfig("filter", 2));
	CheckDlgButton(m_hWnd, IDC_PALTEX, theApp.GetConfig("paltex", 0));
	CheckDlgButton(m_hWnd, IDC_LOGZ, theApp.GetConfig("logz", 1));
	CheckDlgButton(m_hWnd, IDC_FBA, theApp.GetConfig("fba", 1));
	CheckDlgButton(m_hWnd, IDC_AA1, theApp.GetConfig("aa1", 0));
	CheckDlgButton(m_hWnd, IDC_NATIVERES, theApp.GetConfig("nativeres", 1));
	CheckDlgButton(m_hWnd, IDC_ANISOTROPIC, theApp.GetConfig("AnisotropicFiltering", 0));

	// Shade Boost
	CheckDlgButton(m_hWnd, IDC_SHADEBOOST, theApp.GetConfig("ShadeBoost", 0));

	// FXAA shader
	CheckDlgButton(m_hWnd, IDC_FXAA, theApp.GetConfig("Fxaa", 0));

	// External FX shader
	CheckDlgButton(m_hWnd, IDC_SHADER_FX, theApp.GetConfig("shaderfx", 0));
	
	// Hacks
	CheckDlgButton(m_hWnd, IDC_HACKS_ENABLED, theApp.GetConfig("UserHacks", 0));
	

	SendMessage(GetDlgItem(m_hWnd, IDC_RESX), UDM_SETRANGE, 0, MAKELPARAM(8192, 256));
	SendMessage(GetDlgItem(m_hWnd, IDC_RESX), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("resx", 1024), 0));

	SendMessage(GetDlgItem(m_hWnd, IDC_RESY), UDM_SETRANGE, 0, MAKELPARAM(8192, 256));
	SendMessage(GetDlgItem(m_hWnd, IDC_RESY), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("resy", 1024), 0));


	SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_SETRANGE, 0, MAKELPARAM(16, 0));
	SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("extrathreads", 0), 0));

	UpdateControls();
}

bool GSSettingsDlg::OnCommand(HWND hWnd, UINT id, UINT code)
{
	switch (id)
	{
		case IDC_ADAPTER:
			if (code == CBN_SELCHANGE)
			{
				UpdateRenderers();
				UpdateControls();
			}
			break;
		case IDC_RENDERER:
		case IDC_UPSCALE_MULTIPLIER:
			if (code == CBN_SELCHANGE)
				UpdateControls();
			break;
		case IDC_NATIVERES:
		case IDC_SHADEBOOST:
		case IDC_FILTER:
			if (code == BN_CLICKED)
				UpdateControls();
			break;
		case IDC_ANISOTROPIC:
			if (code == BN_CLICKED)
				UpdateControls();
			break;
		case IDC_HACKS_ENABLED:
			if (code == BN_CLICKED)
				UpdateControls();
			break;
		case IDC_SHADEBUTTON:
			if (code == BN_CLICKED)
				ShadeBoostDlg.DoModal();
			break;
		case IDC_HACKSBUTTON:
			if (code == BN_CLICKED)
				HacksDlg.DoModal();
			break;
		case IDOK:
		{
			INT_PTR data;

			if(ComboBoxGetSelData(IDC_ADAPTER, data))
			{
				theApp.SetConfig("Adapter", adapters[(int)data].id.c_str());
			}

			if(!m_IsOpen2 && ComboBoxGetSelData(IDC_RESOLUTION, data))
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

			if (ComboBoxGetSelData(IDC_AFCOMBO, data))
			{
				theApp.SetConfig("MaxAnisotropy", (int)data);
			}

			if(GetId() == IDD_CONFIG) // TODO: other options may not be present in IDD_CONFIG2 as well
			{
				theApp.SetConfig("windowed", (int)IsDlgButtonChecked(m_hWnd, IDC_WINDOWED));			
			}

			theApp.SetConfig("filter", (int)IsDlgButtonChecked(m_hWnd, IDC_FILTER));
			theApp.SetConfig("paltex", (int)IsDlgButtonChecked(m_hWnd, IDC_PALTEX));
			theApp.SetConfig("logz", (int)IsDlgButtonChecked(m_hWnd, IDC_LOGZ));
			theApp.SetConfig("fba", (int)IsDlgButtonChecked(m_hWnd, IDC_FBA));
			theApp.SetConfig("aa1", (int)IsDlgButtonChecked(m_hWnd, IDC_AA1));
			theApp.SetConfig("nativeres", (int)IsDlgButtonChecked(m_hWnd, IDC_NATIVERES));
			theApp.SetConfig("resx", (int)SendMessage(GetDlgItem(m_hWnd, IDC_RESX), UDM_GETPOS, 0, 0));
			theApp.SetConfig("resy", (int)SendMessage(GetDlgItem(m_hWnd, IDC_RESY), UDM_GETPOS, 0, 0));
			theApp.SetConfig("extrathreads", (int)SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_GETPOS, 0, 0));
			theApp.SetConfig("AnisotropicFiltering", (int)IsDlgButtonChecked(m_hWnd, IDC_ANISOTROPIC));

			// Shade Boost
			theApp.SetConfig("ShadeBoost", (int)IsDlgButtonChecked(m_hWnd, IDC_SHADEBOOST));

			// FXAA shader
			theApp.SetConfig("Fxaa", (int)IsDlgButtonChecked(m_hWnd, IDC_FXAA));

			// External FX Shader
			theApp.SetConfig("shaderfx", (int)IsDlgButtonChecked(m_hWnd, IDC_SHADER_FX));

			theApp.SetConfig("UserHacks", (int)IsDlgButtonChecked(m_hWnd, IDC_HACKS_ENABLED));
		}
		break;
	}

	return __super::OnCommand(hWnd, id, code);
}

void GSSettingsDlg::UpdateRenderers()
{
	INT_PTR i;

	if (!ComboBoxGetSelData(IDC_ADAPTER, i))
		return;

	// Ugggh
	HacksDlg.SetAdapter(adapters[(int)i].id);

	D3D_FEATURE_LEVEL level = adapters[(int)i].level;

	vector<GSSetting> renderers;

	unsigned renderer_setting = theApp.GetConfig("Renderer", 0);
	unsigned renderer_sel = 0;

	for(size_t i = 0; i < theApp.m_gs_renderers.size(); i++)
	{
		GSSetting r = theApp.m_gs_renderers[i];

		if(i >= 3 && i <= 5)
		{
			if(level < D3D_FEATURE_LEVEL_10_0) continue;

			r.name = std::string("Direct3D") + (level >= D3D_FEATURE_LEVEL_11_0 ? "11" : "10");
		}

		renderers.push_back(r);
		if (r.id == renderer_setting)
			renderer_sel = renderer_setting;
	}

	ComboBoxInit(IDC_RENDERER, renderers, renderer_sel);
}

void GSSettingsDlg::UpdateControls()
{
	INT_PTR i;

	int scaling = 1; // in case reading the combo doesn't work, enable the custom res control anyway

	if(ComboBoxGetSelData(IDC_UPSCALE_MULTIPLIER, i))
	{
		scaling = (int)i;
	}

	if(ComboBoxGetSelData(IDC_RENDERER, i))
	{
		bool dx9 = (i / 3) == 0;
		bool dx11 = (i / 3) == 1;
		bool ogl = (i / 3) == 4;
		bool hw = (i % 3) == 0;
		//bool sw = (i % 3) == 1;
		bool native = !!IsDlgButtonChecked(m_hWnd, IDC_NATIVERES);

		ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO9), dx9 ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO11), dx11 ? SW_SHOW : SW_HIDE);

		EnableWindow(GetDlgItem(m_hWnd, IDC_WINDOWED), dx9);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESX), hw && !native && scaling == 1);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESX_EDIT), hw && !native && scaling == 1);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESY), hw && !native && scaling == 1);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESY_EDIT), hw && !native && scaling == 1);
		EnableWindow(GetDlgItem(m_hWnd, IDC_UPSCALE_MULTIPLIER), hw && !native);
		EnableWindow(GetDlgItem(m_hWnd, IDC_NATIVERES), hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_FILTER), hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_PALTEX), hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_LOGZ), dx9 && hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_FBA), dx9 && hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_ANISOTROPIC), (int)IsDlgButtonChecked(m_hWnd, IDC_FILTER) && hw && !ogl);
		EnableWindow(GetDlgItem(m_hWnd, IDC_AFCOMBO), (int)IsDlgButtonChecked(m_hWnd, IDC_FILTER) && (int)IsDlgButtonChecked(m_hWnd, IDC_ANISOTROPIC) && hw && !ogl);
		//EnableWindow(GetDlgItem(m_hWnd, IDC_AA1), sw); // Let uers set software params regardless of renderer used 
		//EnableWindow(GetDlgItem(m_hWnd, IDC_SWTHREADS_EDIT), sw);
		//EnableWindow(GetDlgItem(m_hWnd, IDC_SWTHREADS), sw);


		// Shade Boost
		EnableWindow(GetDlgItem(m_hWnd, IDC_SHADEBUTTON), IsDlgButtonChecked(m_hWnd, IDC_SHADEBOOST) == BST_CHECKED);

		// Hacks
		EnableWindow(GetDlgItem(m_hWnd, IDC_HACKS_ENABLED), hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_HACKSBUTTON), hw /*&& IsDlgButtonChecked(m_hWnd, IDC_HACKS_ENABLED) == BST_CHECKED*/);
	}
}

// Shade Boost Dialog

GSShadeBostDlg::GSShadeBostDlg() : 
	GSDialog(IDD_SHADEBOOST)
{}

void GSShadeBostDlg::OnInit()
{
	contrast = theApp.GetConfig("ShadeBoost_Contrast", 50);
	brightness = theApp.GetConfig("ShadeBoost_Brightness", 50);
	saturation = theApp.GetConfig("ShadeBoost_Saturation", 50);

	UpdateControls();
}

void GSShadeBostDlg::UpdateControls()
{
	SendMessage(GetDlgItem(m_hWnd, IDC_SATURATION_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(0, 100));
	SendMessage(GetDlgItem(m_hWnd, IDC_BRIGHTNESS_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(0, 100));
	SendMessage(GetDlgItem(m_hWnd, IDC_CONTRAST_SLIDER), TBM_SETRANGE, TRUE, MAKELONG(0, 100));

	SendMessage(GetDlgItem(m_hWnd, IDC_SATURATION_SLIDER), TBM_SETPOS, TRUE, saturation);
	SendMessage(GetDlgItem(m_hWnd, IDC_BRIGHTNESS_SLIDER), TBM_SETPOS, TRUE, brightness);
	SendMessage(GetDlgItem(m_hWnd, IDC_CONTRAST_SLIDER), TBM_SETPOS, TRUE, contrast);

	char text[8] = {0};

	sprintf(text, "%d", saturation);
	SetDlgItemText(m_hWnd, IDC_SATURATION_TEXT, text);
	sprintf(text, "%d", brightness);
	SetDlgItemText(m_hWnd, IDC_BRIGHTNESS_TEXT, text);
	sprintf(text, "%d", contrast);
	SetDlgItemText(m_hWnd, IDC_CONTRAST_TEXT, text);
}

bool GSShadeBostDlg::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_HSCROLL:	
	{											
		if((HWND)lParam == GetDlgItem(m_hWnd, IDC_SATURATION_SLIDER)) 
		{	
			char text[8] = {0};

			saturation = SendMessage(GetDlgItem(m_hWnd, IDC_SATURATION_SLIDER),TBM_GETPOS,0,0);			
				
			sprintf(text, "%d", saturation);
			SetDlgItemText(m_hWnd, IDC_SATURATION_TEXT, text);
		}
		else if((HWND)lParam == GetDlgItem(m_hWnd, IDC_BRIGHTNESS_SLIDER)) 
		{	
			char text[8] = {0};

			brightness = SendMessage(GetDlgItem(m_hWnd, IDC_BRIGHTNESS_SLIDER),TBM_GETPOS,0,0);			
				
			sprintf(text, "%d", brightness);
			SetDlgItemText(m_hWnd, IDC_BRIGHTNESS_TEXT, text);
		}
		else if((HWND)lParam == GetDlgItem(m_hWnd, IDC_CONTRAST_SLIDER)) 
		{	
			char text[8] = {0};

			contrast = SendMessage(GetDlgItem(m_hWnd, IDC_CONTRAST_SLIDER),TBM_GETPOS,0,0);
							
			sprintf(text, "%d", contrast);
			SetDlgItemText(m_hWnd, IDC_CONTRAST_TEXT, text);
		}
	} break;

	case WM_COMMAND:
	{
		int id = LOWORD(wParam);

		switch(id)
		{
		case IDOK: 
		{
			theApp.SetConfig("ShadeBoost_Contrast", contrast);
			theApp.SetConfig("ShadeBoost_Brightness", brightness);
			theApp.SetConfig("ShadeBoost_Saturation", saturation);
			EndDialog(m_hWnd, id);		
		} break;

		case IDRESET:
		{
			contrast = 50;
			brightness = 50;
			saturation = 50;

			UpdateControls();
		} break;
		}

	} break;

	case WM_CLOSE:EndDialog(m_hWnd, IDCANCEL); break;

	default: return false;
	}
	

	return true;
}

// Hacks Dialog

GSHacksDlg::GSHacksDlg() : 
	GSDialog(IDD_HACKS)
{
	memset(msaa2cb, 0, sizeof(msaa2cb));
	memset(cb2msaa, 0, sizeof(cb2msaa));
}

void GSHacksDlg::OnInit()
{					
	bool dx9 = (int)SendMessage(GetDlgItem(GetParent(m_hWnd), IDC_RENDERER), CB_GETCURSEL, 0, 0) / 3 == 0;
	unsigned short cb = 0;

	if(dx9) for(unsigned short i = 0; i < 17; i++)
	{
		if( i == 1) continue;

		int depth = GSDevice9::GetMaxDepth(i, adapter_id);

		if(depth)
		{
			char text[32] = {0};
			sprintf(text, depth == 32 ? "%dx Z-32" : "%dx Z-24", i);
			SendMessage(GetDlgItem(m_hWnd, IDC_MSAACB), CB_ADDSTRING, 0, (LPARAM)text);

			msaa2cb[i] = cb;
			cb2msaa[cb] = i;
			cb++;
		}
	}
	else for(unsigned short j = 0; j < 5; j++) // TODO: Make the same kind of check for d3d11, eventually....
	{
		unsigned short i = j == 0 ? 0 : 1 << j;
		
		msaa2cb[i] = j;
		cb2msaa[j] = i;
		
		char text[32] = {0};
		sprintf(text, "%dx ", i);

		SendMessage(GetDlgItem(m_hWnd, IDC_MSAACB), CB_ADDSTRING, 0, (LPARAM)text);
	}

	SendMessage(GetDlgItem(m_hWnd, IDC_MSAACB), CB_SETCURSEL, msaa2cb[min(theApp.GetConfig("UserHacks_MSAA", 0), 16)], 0);

	CheckDlgButton(m_hWnd, IDC_ALPHAHACK, theApp.GetConfig("UserHacks_AlphaHack", 0));
	CheckDlgButton(m_hWnd, IDC_OFFSETHACK, theApp.GetConfig("UserHacks_HalfPixelOffset", 0));
	CheckDlgButton(m_hWnd, IDC_SPRITEHACK, theApp.GetConfig("UserHacks_SpriteHack", 0));
	CheckDlgButton(m_hWnd, IDC_WILDHACK, theApp.GetConfig("UserHacks_WildHack", 0));
	CheckDlgButton(m_hWnd, IDC_AGGRESSIVECRC, theApp.GetConfig("UserHacks_AggressiveCRC", 0));
	CheckDlgButton(m_hWnd, IDC_ALPHASTENCIL, theApp.GetConfig("UserHacks_AlphaStencil", 0));
	CheckDlgButton(m_hWnd, IDC_CHECK_NVIDIA_HACK, theApp.GetConfig("UserHacks_NVIDIAHack", 0));
	CheckDlgButton(m_hWnd, IDC_CHECK_DISABLE_ALL_HACKS, theApp.GetConfig("UserHacks_DisableCrcHacks", 0));

	SendMessage(GetDlgItem(m_hWnd, IDC_SKIPDRAWHACK), UDM_SETRANGE, 0, MAKELPARAM(1000, 0));
	SendMessage(GetDlgItem(m_hWnd, IDC_SKIPDRAWHACK), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("UserHacks_SkipDraw", 0), 0));

	SendMessage(GetDlgItem(m_hWnd, IDC_TCOFFSETX), UDM_SETRANGE, 0, MAKELPARAM(10000, 0));
	SendMessage(GetDlgItem(m_hWnd, IDC_TCOFFSETX), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("UserHacks_TCOffset", 0) & 0xFFFF, 0));

	SendMessage(GetDlgItem(m_hWnd, IDC_TCOFFSETY), UDM_SETRANGE, 0, MAKELPARAM(10000, 0));
	SendMessage(GetDlgItem(m_hWnd, IDC_TCOFFSETY), UDM_SETPOS, 0, MAKELPARAM((theApp.GetConfig("UserHacks_TCOffset", 0) >> 16) & 0xFFFF, 0));

	// Hacks descriptions
	SetWindowText(GetDlgItem(m_hWnd, IDC_HACK_DESCRIPTION), "Hover over an item to get a description.");
}

void GSHacksDlg::UpdateControls()
{}

bool GSHacksDlg::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{	    
	switch(message)
	{
	case WM_SETCURSOR:
	{
		const char *helpstr = "";
		bool updateText = true;

		POINT pos;
		GetCursorPos(&pos);
		ScreenToClient(m_hWnd, &pos);

		HWND hoveredwnd = ChildWindowFromPointEx(m_hWnd, pos, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);

		if (hoveredwnd != hovered_window)
			hovered_window = hoveredwnd;
		else
			break;

		switch (GetDlgCtrlID(hoveredwnd))
		{
			case IDC_SKIPDRAWHACK:
			case IDC_SKIPDRAWHACKEDIT:
			case IDC_STATIC_SKIPDRAW:
				helpstr = "Skipdraw\n\nSkips drawing n surfaces completely. "
						  "Use it, for example, to try and get rid of bad post processing effects."
						  " Try values between 1 and 100.";
				break;
			case IDC_ALPHAHACK:
				helpstr = "Alpha Hack\n\nDifferent alpha handling. Can work around some shadow problems.";
				break;
			case IDC_OFFSETHACK:
				helpstr = "Halfpixel\n\nMight fix some misaligned fog, bloom, or blend effect.";
				break;
			case IDC_SPRITEHACK:
				helpstr = "Sprite Hack\n\nHelps getting rid of black inner lines in some filtered sprites."
						  " Half option is the preferred one. Use it for Mana Khemia or ArTonelico for example."
						  " Full can be used for Tales of Destiny.";
				break;
			case IDC_WILDHACK:
				helpstr = "WildArms\n\nLowers the GS precision to avoid gaps between pixels when"
						  " upscaling. Full option fixes the text on WildArms games, while Half option might improve portraits"
						  " in ArTonelico.";
				break;
			case IDC_MSAACB:
			case IDC_STATIC_MSAA:
				helpstr = "Multisample Anti-Aliasing\n\nEnables hardware Anti-Aliasing. Needs lots of memory."
						  " The Z-24 modes might need to have LogarithmicZ to compensate for the bits lost (only in DX9 mode).";
				break;
			case IDC_AGGRESSIVECRC:
				helpstr = "Use more aggressive CRC hacks on some games\n\n"
						  "Only affects few games, removing some effects which might make the image sharper/clearer.\n"
						  "Affected games: FFX, FFX2, FFXII, GOW2, ICO, SoTC, SSX3.\n"
						  "Works as a speedhack for: Steambot Chronicles.";
				break;
			case IDC_ALPHASTENCIL:
				helpstr = "Extend stencil based emulation of destination alpha to perform stencil operations while drawing.\n\n"
						  "Improves many shadows which are normally overdrawn in parts, may affect other effects.\n"
						  "Will disable partial transparency in some games or even prevent drawing some elements altogether.";
				break;
			case IDC_CHECK_NVIDIA_HACK:
				helpstr = "This is a hack to work around problems with recent NVIDIA drivers causing odd stretching problems in DirectX 11 only "
						  "when using Upscaling.\n\n"
						  "Try not to use this unless your game Videos or 2D screens are stretching outside the frame.\n\n"
						  "If you have an AMD/ATi graphics card you should not need this.";
				break;
			case IDC_CHECK_DISABLE_ALL_HACKS:
				helpstr = "FOR TESTING ONLY!!\n\n"
						  "Disable all CRC hacks - will break many games. Overrides CrcHacksExclusion at gsdx.ini\n"
						  "\n"
						  "It's possible to exclude CRC hacks also via the gsdx.ini. E.g.:\n"
						  "CrcHacksExclusions=all\n"
						  "CrcHacksExclusions=0x0F0C4A9C, 0x0EE5646B, 0x7ACF7E03";
				break;

			case IDC_TCOFFSETX:
			case IDC_TCOFFSETX2:
			case IDC_STATIC_TCOFFSETX:
			case IDC_TCOFFSETY:
			case IDC_TCOFFSETY2:
			case IDC_STATIC_TCOFFSETY:
				helpstr = "Texture Coordinates Offset Hack\n\n"
						  "Offset for the ST/UV texture coordinates. Fixes some odd texture issues and might fix some post processing alignment too.\n\n"
						  "  0500 0500, fixes Persona 3 minimap, helps Haunting Ground.\n"
						  "  0000 1000, fixes Xenosaga hair edges (DX10+ Issue)\n";
				break;

			default:
				updateText = false;
				break;
		}

		if(updateText)
			SetWindowText(GetDlgItem(m_hWnd, IDC_HACK_DESCRIPTION), helpstr);

	} break;

	case WM_COMMAND:
	{
		int id = LOWORD(wParam);

		switch(id)
		{
		case IDOK: 
		{
			theApp.SetConfig("UserHacks_MSAA", cb2msaa[(int)SendMessage(GetDlgItem(m_hWnd, IDC_MSAACB), CB_GETCURSEL, 0, 0)]);
			theApp.SetConfig("UserHacks_AlphaHack", (int)IsDlgButtonChecked(m_hWnd, IDC_ALPHAHACK));
			theApp.SetConfig("UserHacks_HalfPixelOffset", (int)IsDlgButtonChecked(m_hWnd, IDC_OFFSETHACK));
			theApp.SetConfig("UserHacks_SpriteHack", (int)IsDlgButtonChecked(m_hWnd, IDC_SPRITEHACK));
			theApp.SetConfig("UserHacks_SkipDraw", (int)SendMessage(GetDlgItem(m_hWnd, IDC_SKIPDRAWHACK), UDM_GETPOS, 0, 0));
			theApp.SetConfig("UserHacks_WildHack", (int)IsDlgButtonChecked(m_hWnd, IDC_WILDHACK));
			theApp.SetConfig("UserHacks_AggressiveCRC", (int)IsDlgButtonChecked(m_hWnd, IDC_AGGRESSIVECRC));
			theApp.SetConfig("UserHacks_AlphaStencil", (int)IsDlgButtonChecked(m_hWnd, IDC_ALPHASTENCIL));
			theApp.SetConfig("UserHacks_NVIDIAHack", (int)IsDlgButtonChecked(m_hWnd, IDC_CHECK_NVIDIA_HACK));
			theApp.SetConfig("UserHacks_DisableCrcHacks", (int)IsDlgButtonChecked(m_hWnd, IDC_CHECK_DISABLE_ALL_HACKS));

			unsigned int TCOFFSET  =  SendMessage(GetDlgItem(m_hWnd, IDC_TCOFFSETX), UDM_GETPOS, 0, 0) & 0xFFFF;
						 TCOFFSET |= (SendMessage(GetDlgItem(m_hWnd, IDC_TCOFFSETY), UDM_GETPOS, 0, 0) & 0xFFFF) << 16;

			theApp.SetConfig("UserHacks_TCOffset", TCOFFSET);

			EndDialog(m_hWnd, id);
		} break;
		}

	} break;

	case WM_CLOSE:EndDialog(m_hWnd, IDCANCEL); break;

	default: return false;
	}

	return true;
}
