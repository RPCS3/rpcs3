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
#include "GSSettingsDlg.h"
#include <shlobj.h>

static struct {int id; const TCHAR* name;} s_renderers[] =
{
	{RENDERER_D3D_HW, "Direct3D"},
	// {RENDERER_D3D_SW_FX, "Software (fixed)"},
	{RENDERER_D3D_SW_FP, "Software (float)"},
	{RENDERER_D3D_NULL, "Do not render"},
};

static struct {DWORD id; const TCHAR* name;} s_psversions[] =
{
	{D3DPS_VERSION(3, 0), _T("Pixel Shader 3.0")},
	{D3DPS_VERSION(2, 0), _T("Pixel Shader 2.0")},
	{D3DPS_VERSION(1, 4), _T("Pixel Shader 1.4")},
	{D3DPS_VERSION(1, 1), _T("Pixel Shader 1.1")},
	{D3DPS_VERSION(0, 0), _T("Fixed Pipeline (bogus)")},
};

IMPLEMENT_DYNAMIC(CGSSettingsDlg, CDialog)
CGSSettingsDlg::CGSSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGSSettingsDlg::IDD, pParent)
	, m_fEnablePalettizedTextures(FALSE)
	, m_fEnableTvOut(FALSE)
	, m_fNloopHack(FALSE)
	, m_fRecordState(FALSE)
	, m_fLinearTexFilter(TRUE)
{
}

CGSSettingsDlg::~CGSSettingsDlg()
{
}

void CGSSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO3, m_resolution);
	DDX_Control(pDX, IDC_COMBO1, m_renderer);
	DDX_Control(pDX, IDC_COMBO4, m_psversion);
	DDX_Check(pDX, IDC_CHECK1, m_fEnablePalettizedTextures);
	DDX_Check(pDX, IDC_CHECK3, m_fEnableTvOut);
	DDX_Check(pDX, IDC_CHECK2, m_fRecordState);
	DDX_Check(pDX, IDC_CHECK5, m_fNloopHack);
	DDX_Text(pDX, IDC_EDIT1, m_strRecordState);
	DDX_Check(pDX, IDC_CHECK4, m_fLinearTexFilter);
}

BEGIN_MESSAGE_MAP(CGSSettingsDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()

// CGSSettingsDlg message handlers

BOOL CGSSettingsDlg::OnInitDialog()
{
	__super::OnInitDialog();

    CWinApp* pApp = AfxGetApp();

	D3DCAPS9 caps;
	ZeroMemory(&caps, sizeof(caps));
	caps.PixelShaderVersion = D3DPS_VERSION(0, 0);

	m_modes.RemoveAll();

	// windowed

	{
		D3DDISPLAYMODE mode;
		memset(&mode, 0, sizeof(mode));
		m_modes.AddTail(mode);

		int iItem = m_resolution.AddString(_T("Windowed"));
		m_resolution.SetItemDataPtr(iItem, m_modes.GetTailPosition());
		m_resolution.SetCurSel(iItem);
	}

	// fullscreen

	if(CComPtr<IDirect3D9> pD3D = Direct3DCreate9(D3D_SDK_VERSION))
	{
		int ModeWidth = pApp->GetProfileInt(_T("Settings"), _T("ModeWidth"), 0);
		int ModeHeight = pApp->GetProfileInt(_T("Settings"), _T("ModeHeight"), 0);
		int ModeRefreshRate = pApp->GetProfileInt(_T("Settings"), _T("ModeRefreshRate"), 0);

		UINT nModes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
		for(UINT i = 0; i < nModes; i++)
		{
			D3DDISPLAYMODE mode;
			if(S_OK == pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &mode))
			{
				CString str;
				str.Format(_T("%dx%d %dHz"), mode.Width, mode.Height, mode.RefreshRate);
				int iItem = m_resolution.AddString(str);

				m_modes.AddTail(mode);
				m_resolution.SetItemDataPtr(iItem, m_modes.GetTailPosition());

				if(ModeWidth == mode.Width && ModeHeight == mode.Height && ModeRefreshRate == mode.RefreshRate)
					m_resolution.SetCurSel(iItem);
			}
		}

		pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, CGSdx9App::D3DDEVTYPE_X, &caps);
	}

	// renderer

	int renderer_id = pApp->GetProfileInt(_T("Settings"), _T("Renderer"), RENDERER_D3D_HW);

	for(int i = 0; i < countof(s_renderers); i++)
	{
		int iItem = m_renderer.AddString(s_renderers[i].name);
		m_renderer.SetItemData(iItem, s_renderers[i].id);
		if(s_renderers[i].id == renderer_id) m_renderer.SetCurSel(iItem);
	}

	// shader

	DWORD psversion_id = pApp->GetProfileInt(_T("Settings"), _T("PixelShaderVersion2"), D3DPS_VERSION(2, 0));

	for(int i = 0; i < countof(s_psversions); i++)
	{
		if(s_psversions[i].id > caps.PixelShaderVersion) continue;
		int iItem = m_psversion.AddString(s_psversions[i].name);
		m_psversion.SetItemData(iItem, s_psversions[i].id);
		if(s_psversions[i].id == psversion_id) m_psversion.SetCurSel(iItem);
	}

	//

	m_fEnablePalettizedTextures = pApp->GetProfileInt(_T("Settings"), _T("fEnablePalettizedTextures"), FALSE);

	//

	m_fLinearTexFilter = (D3DTEXTUREFILTERTYPE)pApp->GetProfileInt(_T("Settings"), _T("TexFilter"), D3DTEXF_LINEAR) == D3DTEXF_LINEAR;

	//

	m_fEnableTvOut = pApp->GetProfileInt(_T("Settings"), _T("fEnableTvOut"), FALSE);

	//
	m_fNloopHack = pApp->GetProfileInt(_T("Settings"), _T("fNloopHack"), FALSE);

	//
	m_fRecordState = pApp->GetProfileInt(_T("Settings"), _T("RecordState"), FALSE);
	m_strRecordState = pApp->GetProfileString(_T("Settings"), _T("RecordStatePath"), _T(""));

	//

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CGSSettingsDlg::OnOK()
{
	CWinApp* pApp = AfxGetApp();

	UpdateData();

	if(m_resolution.GetCurSel() >= 0)
	{
        D3DDISPLAYMODE& mode = m_modes.GetAt((POSITION)m_resolution.GetItemData(m_resolution.GetCurSel()));
		pApp->WriteProfileInt(_T("Settings"), _T("ModeWidth"), mode.Width);
		pApp->WriteProfileInt(_T("Settings"), _T("ModeHeight"), mode.Height);
		pApp->WriteProfileInt(_T("Settings"), _T("ModeRefreshRate"), mode.RefreshRate);
	}

	if(m_renderer.GetCurSel() >= 0)
	{
		pApp->WriteProfileInt(_T("Settings"), _T("Renderer"), m_renderer.GetItemData(m_renderer.GetCurSel()));
	}

	if(m_psversion.GetCurSel() >= 0)
	{
		pApp->WriteProfileInt(_T("Settings"), _T("PixelShaderVersion2"), m_psversion.GetItemData(m_psversion.GetCurSel()));
	}

	pApp->WriteProfileInt(_T("Settings"), _T("fEnablePalettizedTextures"), m_fEnablePalettizedTextures);

	pApp->WriteProfileInt(_T("Settings"), _T("TexFilter"), m_fLinearTexFilter ? D3DTEXF_LINEAR : D3DTEXF_POINT);

	pApp->WriteProfileInt(_T("Settings"), _T("fEnableTvOut"), m_fEnableTvOut);
	pApp->WriteProfileInt(_T("Settings"), _T("fNloopHack"), m_fNloopHack);
	pApp->WriteProfileInt(_T("Settings"), _T("RecordState"), m_fRecordState);
	pApp->WriteProfileString(_T("Settings"), _T("RecordStatePath"), m_strRecordState);

	__super::OnOK();
}

void CGSSettingsDlg::OnBnClickedButton1()
{
	TCHAR path[MAX_PATH];

	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = path;
	bi.lpszTitle = _T("Select path for the state file");
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_VALIDATE | BIF_USENEWUI;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0; 

	LPITEMIDLIST iil;
	if(iil = SHBrowseForFolder(&bi))
	{
		SHGetPathFromIDList(iil, path);
		m_strRecordState = path;
		UpdateData(FALSE);
	}
}
