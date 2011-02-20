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

	if(!m_IsOpen2)
	{
		D3DDISPLAYMODE mode;
		memset(&mode, 0, sizeof(mode));
		m_modes.push_back(mode);

		ComboBoxAppend(IDC_RESOLUTION, "Please select...", (LPARAM)&m_modes.back(), true);

		CComPtr<IDirect3D9> d3d;

		d3d.Attach(Direct3DCreate9(D3D_SDK_VERSION));
		
		if(d3d)
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

	D3D_FEATURE_LEVEL level;

	GSUtil::CheckDirect3D11Level(level);

	vector<GSSetting> renderers;

	for(size_t i = 0; i < theApp.m_gs_renderers.size(); i++)
	{
		GSSetting r = theApp.m_gs_renderers[i];

		if(i >= 3 && i <= 5)
		{
			if(level < D3D_FEATURE_LEVEL_10_0) continue;

			r.name = std::string("Direct3D") + (level >= D3D_FEATURE_LEVEL_11_0 ? "11" : "10");
		}

		renderers.push_back(r);
	}

	ComboBoxInit(IDC_RENDERER, renderers, theApp.GetConfig("Renderer", 0));
	ComboBoxInit(IDC_INTERLACE, theApp.m_gs_interlace, theApp.GetConfig("Interlace", 0));
	ComboBoxInit(IDC_ASPECTRATIO, theApp.m_gs_aspectratio, theApp.GetConfig("AspectRatio", 1));
	ComboBoxInit(IDC_UPSCALE_MULTIPLIER, theApp.m_gs_upscale_multiplier, theApp.GetConfig("upscale_multiplier", 1));

	CheckDlgButton(m_hWnd, IDC_WINDOWED, theApp.GetConfig("windowed", 1));
	CheckDlgButton(m_hWnd, IDC_FILTER, theApp.GetConfig("filter", 2));
	CheckDlgButton(m_hWnd, IDC_PALTEX, theApp.GetConfig("paltex", 0));
	CheckDlgButton(m_hWnd, IDC_VSYNC, theApp.GetConfig("vsync", 0));
	CheckDlgButton(m_hWnd, IDC_LOGZ, theApp.GetConfig("logz", 1));
	CheckDlgButton(m_hWnd, IDC_FBA, theApp.GetConfig("fba", 1));
	CheckDlgButton(m_hWnd, IDC_AA1, theApp.GetConfig("aa1", 0));
	CheckDlgButton(m_hWnd, IDC_NATIVERES, theApp.GetConfig("nativeres", 0));
	// Hacks
	CheckDlgButton(m_hWnd, IDC_ALPHAHACK, theApp.GetConfig("UserHacks_AlphaHack", 0));
	CheckDlgButton(m_hWnd, IDC_OFFSETHACK, theApp.GetConfig("UserHacks_HalfPixelOffset", 0));
	SendMessage(GetDlgItem(m_hWnd, IDC_SKIPDRAWHACK), UDM_SETRANGE, 0, MAKELPARAM(1000, 0));
	SendMessage(GetDlgItem(m_hWnd, IDC_SKIPDRAWHACK), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("UserHacks_SkipDraw", 0), 0));

	SendMessage(GetDlgItem(m_hWnd, IDC_RESX), UDM_SETRANGE, 0, MAKELPARAM(8192, 256));
	SendMessage(GetDlgItem(m_hWnd, IDC_RESX), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("resx", 1024), 0));

	SendMessage(GetDlgItem(m_hWnd, IDC_RESY), UDM_SETRANGE, 0, MAKELPARAM(8192, 256));
	SendMessage(GetDlgItem(m_hWnd, IDC_RESY), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("resy", 1024), 0));

	int r = theApp.GetConfig("Renderer", 0);

	if(r >= 0 && r <= 2) // DX9
	{
		GSDevice9::ForceValidMsaaConfig();

		m_lastValidMsaa = theApp.GetConfig("msaa", 0);
	}

	SendMessage(GetDlgItem(m_hWnd, IDC_MSAA), UDM_SETRANGE, 0, MAKELPARAM(16, 0));
	SendMessage(GetDlgItem(m_hWnd, IDC_MSAA), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("msaa", 0), 0));

	SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_SETRANGE, 0, MAKELPARAM(16, 1));
	SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_SETPOS, 0, MAKELPARAM(theApp.GetConfig("swthreads", 1), 0));

	UpdateControls();
}

bool GSSettingsDlg::OnCommand(HWND hWnd, UINT id, UINT code)
{
	if(id == IDC_MSAAEDIT && code == EN_CHANGE) // validate and possibly warn user when changing msaa
	{
		//post change

		bool dx9 = false;

		INT_PTR i;

		if(ComboBoxGetSelData(IDC_RENDERER, i))
		{
			dx9 = i >= 0 && i <= 2;
		}

		if(dx9)
		{
			uint32 requestedMsaa = (int)SendMessage(GetDlgItem(m_hWnd, IDC_MSAA), UDM_GETPOS, 0, 0); // valid from OnCommand?
			uint32 derivedDepth = GSDevice9::GetMaxDepth(requestedMsaa);

			if(derivedDepth == 0)
			{
				//FIXME: Ugly UI: HW AA is currently a uint spinbox but usually only some values are supported (e.g. only 2/4/8 or a similar set).
				//       Better solution would be to use a drop-down with only valid msaa values such that we don't need this. Maybe some day.
				//       Known bad behavior: When manually deleting a HW AA value to put another instead (e.g. 2 -> delete -> 4)
				//						     it's registered as 0 after the deletion (with possible higher derived z bits), and might issue
				//							 a warning when the new value is registered (i.e. 4 in our example) since it might result in fewer
				//							 z bits than 0, even if it's not different than the previous value (i.e. 2 in our example) z bits.
				
				//Find valid msaa values, regardless of derived z buffer bits

				string supportedAa = "";
				
				for(int i = 2; i <= 16; i++)
				{
					if(GSDevice9::GetMaxDepth(i))
					{
						if(supportedAa.length()) supportedAa += "/";

						supportedAa += format("%d", i);
					}
				}
				
				if(!supportedAa.length())
				{
					supportedAa = "None";
				}

				string s = format("AA=%d is not supported.\nSupported AA values: %s.", (int)requestedMsaa, supportedAa.c_str());

				MessageBox(hWnd, s.c_str(),"Warning", MB_OK | MB_ICONWARNING);

				SendMessage(GetDlgItem(m_hWnd, IDC_MSAA), UDM_SETPOS, 0, requestedMsaa = m_lastValidMsaa); // revert value from inside OnCommand? is this OK?
			}
			else if(derivedDepth < GSDevice9::GetMaxDepth(m_lastValidMsaa))
			{
				string s = format("AA=%d will force GSdx to degrade Z buffer\nfrom 32 to 24 bit, which will probably cause glitches\n(changing 'Logarithmic Z' might help some).\n\nContinue?", (int)requestedMsaa);

				//s+= format("\nlastMsaa=%d, lastDepth=%d, newMsaa=%d, newDepth=%d", (int)m_lastValidMsaa, (int)GSDevice9::GetMaxDepth(m_lastValidMsaa), (int)requestedMsaa, (int)GSDevice9::GetMaxDepth(requestedMsaa));

				if(IDOK != MessageBox(hWnd, s.c_str(), "Warning", MB_OKCANCEL|MB_ICONWARNING))
				{
					SendMessage(GetDlgItem(m_hWnd, IDC_MSAA), UDM_SETPOS, 0, requestedMsaa=m_lastValidMsaa); // revert value from inside OnCommand? is this OK?
				}
			}

			m_lastValidMsaa = requestedMsaa;

			UpdateControls();
		}
	}
	else if(id == IDC_UPSCALE_MULTIPLIER && code == CBN_SELCHANGE)
	{
		UpdateControls();
	}
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

		theApp.SetConfig("windowed", (int)IsDlgButtonChecked(m_hWnd, IDC_WINDOWED));
		theApp.SetConfig("filter", (int)IsDlgButtonChecked(m_hWnd, IDC_FILTER));
		theApp.SetConfig("paltex", (int)IsDlgButtonChecked(m_hWnd, IDC_PALTEX));
		theApp.SetConfig("vsync", (int)IsDlgButtonChecked(m_hWnd, IDC_VSYNC));
		theApp.SetConfig("logz", (int)IsDlgButtonChecked(m_hWnd, IDC_LOGZ));
		theApp.SetConfig("fba", (int)IsDlgButtonChecked(m_hWnd, IDC_FBA));
		theApp.SetConfig("aa1", (int)IsDlgButtonChecked(m_hWnd, IDC_AA1));
		theApp.SetConfig("nativeres", (int)IsDlgButtonChecked(m_hWnd, IDC_NATIVERES));
		theApp.SetConfig("resx", (int)SendMessage(GetDlgItem(m_hWnd, IDC_RESX), UDM_GETPOS, 0, 0));
		theApp.SetConfig("resy", (int)SendMessage(GetDlgItem(m_hWnd, IDC_RESY), UDM_GETPOS, 0, 0));
		theApp.SetConfig("swthreads", (int)SendMessage(GetDlgItem(m_hWnd, IDC_SWTHREADS), UDM_GETPOS, 0, 0));
		theApp.SetConfig("msaa", (int)SendMessage(GetDlgItem(m_hWnd, IDC_MSAA), UDM_GETPOS, 0, 0));
		// Hacks
		theApp.SetConfig("UserHacks_AlphaHack", (int)IsDlgButtonChecked(m_hWnd, IDC_ALPHAHACK));
		theApp.SetConfig("UserHacks_HalfPixelOffset", (int)IsDlgButtonChecked(m_hWnd, IDC_OFFSETHACK));
		theApp.SetConfig("UserHacks_SkipDraw", (int)SendMessage(GetDlgItem(m_hWnd, IDC_SKIPDRAWHACK), UDM_GETPOS, 0, 0));
	}

	return __super::OnCommand(hWnd, id, code);
}


void GSSettingsDlg::UpdateControls()
{
	INT_PTR i;

	bool allowHacks = !!theApp.GetConfig("allowHacks", 0);

	int scaling = 1; // in case reading the combo doesn't work, enable the custom res control anyway

	if(ComboBoxGetSelData(IDC_UPSCALE_MULTIPLIER, i))
	{
		scaling = (int)i;
	}

	if(ComboBoxGetSelData(IDC_RENDERER, i))
	{
		bool dx9 = i >= 0 && i <= 2;
		bool dx10 = i >= 3 && i <= 5;
		bool dx11 = i >= 6 && i <= 8;
		bool hw = i == 0 || i == 3 || i == 6;
		bool sw = i == 1 || i == 4 || i == 7;
		bool native = !!IsDlgButtonChecked(m_hWnd, IDC_NATIVERES);

		ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO9), dx9 ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(m_hWnd, IDC_LOGO11), dx10 ? SW_SHOW : SW_HIDE);

		EnableWindow(GetDlgItem(m_hWnd, IDC_WINDOWED), dx9);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESX), hw && !native && scaling == 1);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESX_EDIT), hw && !native && scaling == 1);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESY), hw && !native && scaling == 1);
		EnableWindow(GetDlgItem(m_hWnd, IDC_RESY_EDIT), hw && !native && scaling == 1);
		EnableWindow(GetDlgItem(m_hWnd, IDC_UPSCALE_MULTIPLIER), hw && !native);
		EnableWindow(GetDlgItem(m_hWnd, IDC_NATIVERES), hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_FILTER), hw && !native);
		EnableWindow(GetDlgItem(m_hWnd, IDC_PALTEX), hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_LOGZ), dx9 && hw && GSDevice9::GetMaxDepth(m_lastValidMsaa) < 32);
		EnableWindow(GetDlgItem(m_hWnd, IDC_FBA), dx9 && hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_AA1), sw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_SWTHREADS_EDIT), sw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_SWTHREADS), sw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_MSAAEDIT), hw);
		EnableWindow(GetDlgItem(m_hWnd, IDC_MSAA), hw);

		//ShowWindow(GetDlgItem(m_hWnd, IDC_USERHACKS), allowHacks && hw) ? SW_SHOW : SW_HIDE;  //Don't disable the "Hacks" frame
		ShowWindow(GetDlgItem(m_hWnd, IDC_MSAAEDIT), allowHacks && hw) ? SW_SHOW : SW_HIDE;
		ShowWindow(GetDlgItem(m_hWnd, IDC_MSAA), allowHacks && hw) ? SW_SHOW : SW_HIDE;
		ShowWindow(GetDlgItem(m_hWnd, IDC_STATIC_TEXT_HWAA), allowHacks && hw) ? SW_SHOW : SW_HIDE;
		
		ShowWindow(GetDlgItem(m_hWnd, IDC_ALPHAHACK), allowHacks && hw) ? SW_SHOW : SW_HIDE;
		ShowWindow(GetDlgItem(m_hWnd, IDC_OFFSETHACK), allowHacks && hw) ? SW_SHOW : SW_HIDE;
		
		ShowWindow(GetDlgItem(m_hWnd, IDC_SKIPDRAWHACKEDIT), allowHacks && hw) ? SW_SHOW : SW_HIDE;
		ShowWindow(GetDlgItem(m_hWnd, IDC_SKIPDRAWHACK), allowHacks && hw) ? SW_SHOW : SW_HIDE;
		ShowWindow(GetDlgItem(m_hWnd, IDC_STATIC_TEXT_SKIPDRAW), allowHacks && hw) ? SW_SHOW : SW_HIDE;
	}
}
