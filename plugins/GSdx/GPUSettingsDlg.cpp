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
#include <shlobj.h>
#include <afxpriv.h>

GSSetting GPUSettingsDlg::g_renderers[] =
{
	{0, _T("Direct3D7 (Software)"), NULL},
	{1, _T("Direct3D9 (Software)"), NULL},
	{2, _T("Direct3D10 (Software)"), NULL},
//	{3, _T("Null (Null)"), NULL},
};

GSSetting GPUSettingsDlg::g_psversion[] =
{
	{D3DPS_VERSION(3, 0), _T("Pixel Shader 3.0"), NULL},
	{D3DPS_VERSION(2, 0), _T("Pixel Shader 2.0"), NULL},
	//{D3DPS_VERSION(1, 4), _T("Pixel Shader 1.4"), NULL},
	//{D3DPS_VERSION(1, 1), _T("Pixel Shader 1.1"), NULL},
	//{D3DPS_VERSION(0, 0), _T("Fixed Pipeline (bogus)"), NULL},
};

GSSetting GPUSettingsDlg::g_filter[] =
{
	{0, _T("Nearest"), NULL},
	{1, _T("Bilinear (polygons only)"), NULL},
	{2, _T("Bilinear"), NULL},
};

GSSetting GPUSettingsDlg::g_dithering[] =
{
	{0, _T("Disabled"), NULL},
	{1, _T("Auto"), NULL},
};

GSSetting GPUSettingsDlg::g_aspectratio[] =
{
	{0, _T("Stretch"), NULL},
	{1, _T("4:3"), NULL},
	{2, _T("16:9"), NULL},
};

GSSetting GPUSettingsDlg::g_internalresolution[] =
{
	{0 | (0 << 2), _T("H x 1 - V x 1"), NULL},
	{1 | (0 << 2), _T("H x 2 - V x 1"), NULL},
	{0 | (1 << 2), _T("H x 1 - V x 2"), NULL},
	{1 | (1 << 2), _T("H x 2 - V x 2"), NULL},
	{2 | (1 << 2), _T("H x 4 - V x 2"), NULL},
	{1 | (2 << 2), _T("H x 2 - V x 4"), NULL},
	{2 | (2 << 2), _T("H x 4 - V x 4"), NULL},
};

IMPLEMENT_DYNAMIC(GPUSettingsDlg, CDialog)

GPUSettingsDlg::GPUSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(GPUSettingsDlg::IDD, pParent)
{

}

GPUSettingsDlg::~GPUSettingsDlg()
{
}

LRESULT GPUSettingsDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = __super::DefWindowProc(message, wParam, lParam);

	if(message == WM_INITDIALOG)
	{
		SendMessage(WM_KICKIDLE);
	}

	return ret;
}

void GPUSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO3, m_resolution);
	DDX_Control(pDX, IDC_COMBO1, m_renderer);
	DDX_Control(pDX, IDC_COMBO4, m_psversion);
	DDX_Control(pDX, IDC_COMBO2, m_filter);
	DDX_Control(pDX, IDC_COMBO5, m_dithering);
	DDX_Control(pDX, IDC_COMBO6, m_aspectratio);
	DDX_Control(pDX, IDC_COMBO7, m_internalresolution);
	DDX_Control(pDX, IDC_SPIN3, m_swthreads);
	DDX_Control(pDX, IDC_EDIT3, m_swthreadsedit);
}

BEGIN_MESSAGE_MAP(GPUSettingsDlg, CDialog)
	ON_MESSAGE_VOID(WM_KICKIDLE, OnKickIdle)
	ON_UPDATE_COMMAND_UI(IDC_COMBO4, OnUpdateD3D9Options)
	ON_UPDATE_COMMAND_UI(IDC_COMBO7, OnUpdateSWOptions)
	ON_UPDATE_COMMAND_UI(IDC_SPIN3, OnUpdateSWOptions)
	ON_UPDATE_COMMAND_UI(IDC_EDIT3, OnUpdateSWOptions)
	ON_CBN_SELCHANGE(IDC_COMBO1, &GPUSettingsDlg::OnCbnSelchangeCombo1)
END_MESSAGE_MAP()

void GPUSettingsDlg::OnKickIdle()
{
	UpdateDialogControls(this, false);
}

BOOL GPUSettingsDlg::OnInitDialog()
{
	__super::OnInitDialog();

	D3DCAPS9 caps;
	memset(&caps, 0, sizeof(caps));
	caps.PixelShaderVersion = D3DPS_VERSION(0, 0);

	m_modes.clear();

	// windowed

	{
		D3DDISPLAYMODE mode;
		memset(&mode, 0, sizeof(mode));
		m_modes.push_back(mode);

		int iItem = m_resolution.AddString(_T("Windowed"));
		m_resolution.SetItemDataPtr(iItem, &m_modes.back());
		m_resolution.SetCurSel(iItem);
	}

	// fullscreen

	if(CComPtr<IDirect3D9> d3d = Direct3DCreate9(D3D_SDK_VERSION))
	{
		uint32 ModeWidth = theApp.GetConfig("ModeWidth", 0);
		uint32 ModeHeight = theApp.GetConfig("ModeHeight", 0);
		uint32 ModeRefreshRate = theApp.GetConfig("ModeRefreshRate", 0);

		uint32 nModes = d3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);

		for(uint32 i = 0; i < nModes; i++)
		{
			D3DDISPLAYMODE mode;

			if(S_OK == d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &mode))
			{
				CString str;
				str.Format(_T("%dx%d %dHz"), mode.Width, mode.Height, mode.RefreshRate);
				int iItem = m_resolution.AddString(str);

				m_modes.push_back(mode);
				m_resolution.SetItemDataPtr(iItem, &m_modes.back());

				if(ModeWidth == mode.Width && ModeHeight == mode.Height && ModeRefreshRate == mode.RefreshRate)
				{
					m_resolution.SetCurSel(iItem);
				}
			}
		}

		d3d->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
	}

	bool isdx10avail = GSUtil::IsDirect3D10Available();

	vector<GSSetting> renderers;

	for(size_t i = 0; i < countof(g_renderers); i++)
	{
		if(i == 2 && !isdx10avail) continue;

		renderers.push_back(g_renderers[i]);
	}

	GSSetting::InitComboBox(&renderers[0], renderers.size(), m_renderer, theApp.GetConfig("Renderer", 1));
	GSSetting::InitComboBox(g_psversion, countof(g_psversion), m_psversion, theApp.GetConfig("PixelShaderVersion2", D3DPS_VERSION(2, 0)), caps.PixelShaderVersion);
	GSSetting::InitComboBox(g_filter, countof(g_filter), m_filter, theApp.GetConfig("filter", 0));
	GSSetting::InitComboBox(g_dithering, countof(g_dithering), m_dithering, theApp.GetConfig("dithering", 1));
	GSSetting::InitComboBox(g_aspectratio, countof(g_aspectratio), m_aspectratio, theApp.GetConfig("AspectRatio", 1));
	GSSetting::InitComboBox(g_internalresolution, countof(g_internalresolution), m_internalresolution, theApp.GetConfig("scale_x", 0) | (theApp.GetConfig("scale_y", 0) << 2));
	

	OnCbnSelchangeCombo1();

	//

	m_swthreads.SetRange(1, 16);
	m_swthreads.SetPos(theApp.GetConfig("swthreads", 1));

	//

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void GPUSettingsDlg::OnOK()
{
	UpdateData();

	if(m_resolution.GetCurSel() >= 0)
	{
		D3DDISPLAYMODE& mode = *(D3DDISPLAYMODE*)m_resolution.GetItemData(m_resolution.GetCurSel());

		theApp.SetConfig("ModeWidth", (int)mode.Width);
		theApp.SetConfig("ModeHeight", (int)mode.Height);
		theApp.SetConfig("ModeRefreshRate", (int)mode.RefreshRate);
	}

	if(m_renderer.GetCurSel() >= 0)
	{
		theApp.SetConfig("Renderer", (int)m_renderer.GetItemData(m_renderer.GetCurSel()));
	}

	if(m_psversion.GetCurSel() >= 0)
	{
		theApp.SetConfig("PixelShaderVersion2", (int)m_psversion.GetItemData(m_psversion.GetCurSel()));
	}

	if(m_filter.GetCurSel() >= 0)
	{
		theApp.SetConfig("filter", (int)m_filter.GetItemData(m_filter.GetCurSel()));
	}

	if(m_dithering.GetCurSel() >= 0)
	{
		theApp.SetConfig("dithering", (int)m_dithering.GetItemData(m_dithering.GetCurSel()));
	}

	if(m_aspectratio.GetCurSel() >= 0)
	{
		theApp.SetConfig("AspectRatio", (int)m_aspectratio.GetItemData(m_aspectratio.GetCurSel()));
	}

	if(m_internalresolution.GetCurSel() >= 0)
	{
		int value = (int)m_internalresolution.GetItemData(m_internalresolution.GetCurSel());

		theApp.SetConfig("scale_x", value & 3);
		theApp.SetConfig("scale_y", (value >> 2) & 3);
	}

	theApp.SetConfig("swthreads", m_swthreads.GetPos());

	__super::OnOK();
}

void GPUSettingsDlg::OnUpdateResolution(CCmdUI* pCmdUI)
{
	UpdateData();

	int i = (int)m_renderer.GetItemData(m_renderer.GetCurSel());

	pCmdUI->Enable(i == 1);
}

void GPUSettingsDlg::OnUpdateD3D9Options(CCmdUI* pCmdUI)
{
	int i = (int)m_renderer.GetItemData(m_renderer.GetCurSel());

	pCmdUI->Enable(i == 1);
}

void GPUSettingsDlg::OnUpdateSWOptions(CCmdUI* pCmdUI)
{
	int i = (int)m_renderer.GetItemData(m_renderer.GetCurSel());

	pCmdUI->Enable(i >= 0 && i <= 2);
}

void GPUSettingsDlg::OnCbnSelchangeCombo1()
{
	int i = (int)m_renderer.GetItemData(m_renderer.GetCurSel());

	GetDlgItem(IDC_LOGO9)->ShowWindow(i == 1 ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_LOGO10)->ShowWindow(i == 2 ? SW_SHOW : SW_HIDE);
}
