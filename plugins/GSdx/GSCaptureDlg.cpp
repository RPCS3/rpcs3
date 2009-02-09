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
#include <afxpriv.h>
#include "GSCaptureDlg.h"

// GSCaptureDlg dialog

IMPLEMENT_DYNAMIC(GSCaptureDlg, CDialog)
GSCaptureDlg::GSCaptureDlg(CWnd* pParent /*=NULL*/)
	: CDialog(GSCaptureDlg::IDD, pParent)
{
	m_width = AfxGetApp()->GetProfileInt(_T("Capture"), _T("Width"), 640);
	m_height = AfxGetApp()->GetProfileInt(_T("Capture"), _T("Height"), 480);
	m_filename = AfxGetApp()->GetProfileString(_T("Capture"), _T("FileName"));
}

GSCaptureDlg::~GSCaptureDlg()
{
}

int GSCaptureDlg::GetSelCodec(Codec& c)
{
	int iSel = m_codeclist.GetCurSel();

	if(iSel < 0) return 0;

	POSITION pos = (POSITION)m_codeclist.GetItemDataPtr(iSel);

	if(pos == NULL) return 2;

	c = m_codecs.GetAt(pos);

	if(!c.filter)
	{
		c.moniker->BindToObject(NULL, NULL, __uuidof(IBaseFilter), (void**)&c.filter);

		if(!c.filter) return 0;
	}

	return 1;
}

LRESULT GSCaptureDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret = __super::DefWindowProc(message, wParam, lParam);

	if(message == WM_INITDIALOG) SendMessage(WM_KICKIDLE);

	return(ret);
}

void GSCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_EDIT1, m_filename);
	DDX_Control(pDX, IDC_COMBO1, m_codeclist);
	DDX_Text(pDX, IDC_EDIT2, m_width);
	DDX_Text(pDX, IDC_EDIT4, m_height);
}

BOOL GSCaptureDlg::OnInitDialog()
{
	__super::OnInitDialog();

	m_codecs.RemoveAll();

	m_codeclist.ResetContent();
	m_codeclist.SetItemDataPtr(m_codeclist.AddString(_T("Uncompressed")), NULL);

	BeginEnumSysDev(CLSID_VideoCompressorCategory, moniker)
	{
		Codec c;
		c.moniker = moniker;

		LPOLESTR strName = NULL;
		if(FAILED(moniker->GetDisplayName(NULL, NULL, &strName)))
			continue;

		c.DisplayName = strName;
		CoTaskMemFree(strName);

		CComPtr<IPropertyBag> pPB;
		moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		if(FAILED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
			continue;
		
		c.FriendlyName = var.bstrVal;

		CStringW str = CStringW(c.DisplayName).MakeLower();
		CString prefix;
		if(str.Find(L"@device:dmo:") == 0) prefix = _T("(DMO) ");
		else if(str.Find(L"@device:sw:") == 0) prefix = _T("(DS) ");
		else if(str.Find(L"@device:cm:") == 0) prefix = _T("(VfW) ");
		c.FriendlyName = prefix + c.FriendlyName;

		m_codeclist.SetItemDataPtr(m_codeclist.AddString(c.FriendlyName), m_codecs.AddTail(c));
	}
	EndEnumSysDev

	//

	CString DisplayNameToFind = AfxGetApp()->GetProfileString(_T("Capture"), _T("VideoCodecDisplayName"));

	for(int i = 0; i < m_codeclist.GetCount(); i++)
	{
		CString DisplayName;

		POSITION pos = (POSITION)m_codeclist.GetItemDataPtr(i);

		if(pos) DisplayName = m_codecs.GetAt(pos).DisplayName;

		if(DisplayName == DisplayNameToFind)
		{
			m_codeclist.SetCurSel(i);
			break;
		}
	}

	//

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(GSCaptureDlg, CDialog)
	ON_MESSAGE_VOID(WM_KICKIDLE, OnKickIdle)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateButton2)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOK)
END_MESSAGE_MAP()

// GSCaptureDlg message handlers

void GSCaptureDlg::OnKickIdle()
{
	UpdateDialogControls(this, false);
}

void GSCaptureDlg::OnBnClickedButton1()
{
	UpdateData();

	DWORD flags = OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;

	CFileDialog fd(FALSE, _T("avi"), m_filename, flags, _T("Avi files (*.avi)|*.avi||"), this, 0);

	if(fd.DoModal() == IDOK)
	{
		m_filename = fd.GetPathName();

		UpdateData(FALSE);
	}
}

void GSCaptureDlg::OnBnClickedButton2()
{
	Codec c;

	if(GetSelCodec(c) != 1) return;

	if(CComQIPtr<ISpecifyPropertyPages> pSPP = c.filter)
	{
		CAUUID caGUID;

		memset(&caGUID, 0, sizeof(caGUID));

		if(SUCCEEDED(pSPP->GetPages(&caGUID)))
		{
			IUnknown* lpUnk = NULL;
			pSPP.QueryInterface(&lpUnk);
			OleCreatePropertyFrame(m_hWnd, 0, 0, CStringW(c.FriendlyName), 1, (IUnknown**)&lpUnk, caGUID.cElems, caGUID.pElems, 0, 0, NULL);
			lpUnk->Release();

			if(caGUID.pElems) CoTaskMemFree(caGUID.pElems);
		}
	}
	else if(CComQIPtr<IAMVfwCompressDialogs> pAMVfWCD = c.filter)
	{
		if(pAMVfWCD->ShowDialog(VfwCompressDialog_QueryConfig, NULL) == S_OK)
		{
			pAMVfWCD->ShowDialog(VfwCompressDialog_Config, m_hWnd);
		}
	}
}

void GSCaptureDlg::OnUpdateButton2(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_codeclist.GetCurSel() >= 0 && m_codeclist.GetItemDataPtr(m_codeclist.GetCurSel()) != NULL);
}

void GSCaptureDlg::OnBnClickedOk()
{
	UpdateData();

	Codec c;

	if(GetSelCodec(c) == 0) return;

	m_enc = c.filter;

	AfxGetApp()->WriteProfileInt(_T("Capture"), _T("Width"), m_width);
	AfxGetApp()->WriteProfileInt(_T("Capture"), _T("Height"), m_height);
	AfxGetApp()->WriteProfileString(_T("Capture"), _T("FileName"), m_filename);
	AfxGetApp()->WriteProfileString(_T("Capture"), _T("VideoCodecDisplayName"), CString(c.DisplayName));

	OnOK();
}

void GSCaptureDlg::OnUpdateOK(CCmdUI* pCmdUI)
{
	CString str;

	GetDlgItem(IDC_EDIT1)->GetWindowText(str);

	pCmdUI->Enable(!str.IsEmpty() && m_codeclist.GetCurSel() >= 0);
}
