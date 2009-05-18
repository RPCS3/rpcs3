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
#include <shlobj.h>
#include <afxpriv.h>

GSSetting GSSettingsDlg::g_renderers[] =
{
	{0, _T("Direct3D9 (Hardware)"), NULL},
	{1, _T("Direct3D9 (Software)"), NULL},
	{2, _T("Direct3D9 (Null)"), NULL},
	{3, _T("Direct3D10 (Hardware)"), NULL},
	{4, _T("Direct3D10 (Software)"), NULL},
	{5, _T("Direct3D10 (Null)"), NULL},
	{6, _T("Null (Software)"), NULL},
	{7, _T("Null (Null)"), NULL},
};

GSSetting GSSettingsDlg::g_psversion[] =
{
	{D3DPS_VERSION(3, 0), _T("Pixel Shader 3.0"), NULL},
	{D3DPS_VERSION(2, 0), _T("Pixel Shader 2.0"), NULL},
	//{D3DPS_VERSION(1, 4), _T("Pixel Shader 1.4"), NULL},
	//{D3DPS_VERSION(1, 1), _T("Pixel Shader 1.1"), NULL},
	//{D3DPS_VERSION(0, 0), _T("Fixed Pipeline (bogus)"), NULL},
};

GSSetting GSSettingsDlg::g_interlace[] =
{
	{0, _T("None"), NULL},
	{1, _T("Weave tff"), _T("saw-tooth")},
	{2, _T("Weave bff"), _T("saw-tooth")},
	{3, _T("Bob tff"), _T("use blend if shaking")},
	{4, _T("Bob bff"), _T("use blend if shaking")},
	{5, _T("Blend tff"), _T("slight blur, 1/2 fps")},
	{6, _T("Blend bff"), _T("slight blur, 1/2 fps")},
};

GSSetting GSSettingsDlg::g_aspectratio[] =
{
	{0, _T("Stretch"), NULL},
	{1, _T("4:3"), NULL},
	{2, _T("16:9"), NULL},
};

IMPLEMENT_DYNAMIC(GSSettingsDlg, CDialog)
GSSettingsDlg::GSSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(GSSettingsDlg::IDD, pParent)
	, m_filter(1)
	, m_nativeres(FALSE)
	, m_vsync(FALSE)
	, m_logz(FALSE)
	, m_fba(TRUE)
	, m_aa1(FALSE)
	, m_blur(FALSE)
{
}

GSSettingsDlg::~GSSettingsDlg()
{
}

LRESULT GSSettingsDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = __super::DefWindowProc(message, wParam, lParam);

	if(message == WM_INITDIALOG)
	{
		SendMessage(WM_KICKIDLE);
	}

	return ret;
}

void GSSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO3, m_resolution);
	DDX_Control(pDX, IDC_COMBO1, m_renderer);
	DDX_Control(pDX, IDC_COMBO4, m_psversion);
	DDX_Control(pDX, IDC_COMBO2, m_interlace);
	DDX_Control(pDX, IDC_COMBO5, m_aspectratio);
	DDX_Check(pDX, IDC_CHECK4, m_filter);
	DDX_Control(pDX, IDC_SPIN1, m_resx);
	DDX_Control(pDX, IDC_SPIN2, m_resy);
	DDX_Control(pDX, IDC_SPIN3, m_swthreads);
	DDX_Check(pDX, IDC_CHECK1, m_nativeres);
	DDX_Control(pDX, IDC_EDIT1, m_resxedit);
	DDX_Control(pDX, IDC_EDIT2, m_resyedit);
	DDX_Control(pDX, IDC_EDIT3, m_swthreadsedit);
	DDX_Check(pDX, IDC_CHECK2, m_vsync);
	DDX_Check(pDX, IDC_CHECK5, m_logz);
	DDX_Check(pDX, IDC_CHECK7, m_fba);
	DDX_Check(pDX, IDC_CHECK8, m_aa1);
	DDX_Check(pDX, IDC_CHECK9, m_blur);
}

BEGIN_MESSAGE_MAP(GSSettingsDlg, CDialog)
	ON_MESSAGE_VOID(WM_KICKIDLE, OnKickIdle)
	ON_UPDATE_COMMAND_UI(IDC_SPIN1, OnUpdateResolution)
	ON_UPDATE_COMMAND_UI(IDC_SPIN2, OnUpdateResolution)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateResolution)
	ON_UPDATE_COMMAND_UI(IDC_EDIT2, OnUpdateResolution)
	ON_UPDATE_COMMAND_UI(IDC_COMBO4, OnUpdateD3D9Options)
	ON_UPDATE_COMMAND_UI(IDC_CHECK3, OnUpdateD3D9Options)
	ON_UPDATE_COMMAND_UI(IDC_CHECK5, OnUpdateD3D9Options)
	ON_UPDATE_COMMAND_UI(IDC_CHECK7, OnUpdateD3D9Options)
	ON_UPDATE_COMMAND_UI(IDC_SPIN3, OnUpdateSWOptions)
	ON_UPDATE_COMMAND_UI(IDC_EDIT3, OnUpdateSWOptions)
	ON_CBN_SELCHANGE(IDC_COMBO1, &GSSettingsDlg::OnCbnSelchangeCombo1)
END_MESSAGE_MAP()

void GSSettingsDlg::OnKickIdle()
{
	UpdateDialogControls(this, false);
}

BOOL GSSettingsDlg::OnInitDialog()
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
				string str = format("%dx%d %dHz", mode.Width, mode.Height, mode.RefreshRate);
				int iItem = m_resolution.AddString(str.c_str());

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
		if(i >= 3 && i <= 5 && !isdx10avail) continue;

		renderers.push_back(g_renderers[i]);
	}

	GSSetting::InitComboBox(&renderers[0], renderers.size(), m_renderer, theApp.GetConfig("Renderer", 0));
	GSSetting::InitComboBox(g_psversion, countof(g_psversion), m_psversion, theApp.GetConfig("PixelShaderVersion2", D3DPS_VERSION(2, 0)), caps.PixelShaderVersion);
	GSSetting::InitComboBox(g_interlace, countof(g_interlace), m_interlace, theApp.GetConfig("Interlace", 0));
	GSSetting::InitComboBox(g_aspectratio, countof(g_aspectratio), m_aspectratio, theApp.GetConfig("AspectRatio", 1));

	OnCbnSelchangeCombo1();

	//

	m_filter = theApp.GetConfig("filter", 1);
	m_vsync = !!theApp.GetConfig("vsync", 0);
	m_logz = !!theApp.GetConfig("logz", 0);
	m_fba = !!theApp.GetConfig("fba", 1);
	m_aa1 = !!theApp.GetConfig("aa1", 0);
	m_blur = !!theApp.GetConfig("blur", 0);

	m_resx.SetRange(512, 4096);
	m_resy.SetRange(512, 4096);
	m_resx.SetPos(theApp.GetConfig("resx", 1024));
	m_resy.SetPos(theApp.GetConfig("resy", 1024));
	m_nativeres = !!theApp.GetConfig("nativeres", 0);

	m_resx.EnableWindow(!m_nativeres);
	m_resy.EnableWindow(!m_nativeres);
	m_resxedit.EnableWindow(!m_nativeres);
	m_resyedit.EnableWindow(!m_nativeres);

	m_swthreads.SetRange(1, 16);
	m_swthreads.SetPos(theApp.GetConfig("swthreads", 1));

	//

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void GSSettingsDlg::OnOK()
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
		theApp.SetConfig("Renderer", (uint32)m_renderer.GetItemData(m_renderer.GetCurSel()));
	}

	if(m_psversion.GetCurSel() >= 0)
	{
		theApp.SetConfig("PixelShaderVersion2", (uint32)m_psversion.GetItemData(m_psversion.GetCurSel()));
	}

	if(m_interlace.GetCurSel() >= 0)
	{
		theApp.SetConfig("Interlace", (uint32)m_interlace.GetItemData(m_interlace.GetCurSel()));
	}

	if(m_aspectratio.GetCurSel() >= 0)
	{
		theApp.SetConfig("AspectRatio", (uint32)m_aspectratio.GetItemData(m_aspectratio.GetCurSel()));
	}

	theApp.SetConfig("filter", m_filter);
	theApp.SetConfig("vsync", m_vsync);
	theApp.SetConfig("logz", m_logz);
	theApp.SetConfig("fba", m_fba);
	theApp.SetConfig("aa1", m_aa1);
	theApp.SetConfig("blur", m_blur);	

	theApp.SetConfig("resx", m_resx.GetPos());
	theApp.SetConfig("resy", m_resy.GetPos());
	theApp.SetConfig("swthreads", m_swthreads.GetPos());
	theApp.SetConfig("nativeres", m_nativeres);

	__super::OnOK();
}

void GSSettingsDlg::OnUpdateResolution(CCmdUI* pCmdUI)
{
	UpdateData();

	int i = (int)m_renderer.GetItemData(m_renderer.GetCurSel());

	pCmdUI->Enable(!m_nativeres && (i == 0 || i == 3));
}

void GSSettingsDlg::OnUpdateD3D9Options(CCmdUI* pCmdUI)
{
	int i = (int)m_renderer.GetItemData(m_renderer.GetCurSel());

	pCmdUI->Enable(i >= 0 && i <= 2);
}

void GSSettingsDlg::OnUpdateSWOptions(CCmdUI* pCmdUI)
{
	int i = (int)m_renderer.GetItemData(m_renderer.GetCurSel());

	pCmdUI->Enable(i == 1 || i == 4 || i == 6);
}

void GSSettingsDlg::OnCbnSelchangeCombo1()
{
	int i = (int)m_renderer.GetItemData(m_renderer.GetCurSel());

	GetDlgItem(IDC_LOGO9)->ShowWindow(i >= 0 && i <= 2 ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_LOGO10)->ShowWindow(i >= 3 && i <= 5 ? SW_SHOW : SW_HIDE);
}
