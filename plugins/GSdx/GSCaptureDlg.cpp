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
#include "GSCaptureDlg.h"

GSCaptureDlg::GSCaptureDlg()
	: GSDialog(IDD_CAPTURE)
{
	m_width = theApp.GetConfig("CaptureWidth", 640);
	m_height = theApp.GetConfig("CaptureHeight", 480);
	m_filename = theApp.GetConfig("CaptureFileName", "");
}

int GSCaptureDlg::GetSelCodec(Codec& c)
{
	INT_PTR data = 0;

	if(ComboBoxGetSelData(IDC_CODECS, data))
	{
		if(data == 0) return 2;

		c = *(Codec*)data;

		if(!c.filter)
		{
			c.moniker->BindToObject(NULL, NULL, __uuidof(IBaseFilter), (void**)&c.filter);

			if(!c.filter) return 0;
		}

		return 1;
	}

	return 0;
}

void GSCaptureDlg::OnInit()
{
	__super::OnInit();

	SetTextAsInt(IDC_WIDTH, m_width);
	SetTextAsInt(IDC_HEIGHT, m_height);
	SetText(IDC_FILENAME, m_filename.c_str());

	m_codecs.clear();

	string selected = theApp.GetConfig("CaptureVideoCodecDisplayName", "");

	ComboBoxAppend(IDC_CODECS, "Uncompressed", 0, true);

	BeginEnumSysDev(CLSID_VideoCompressorCategory, moniker)
	{
		Codec c;

		c.moniker = moniker;

		wstring prefix;

		LPOLESTR str = NULL;

		if(FAILED(moniker->GetDisplayName(NULL, NULL, &str)))
			continue;

		if(wcsstr(str, L"@device:dmo:")) prefix = L"(DMO) ";
		else if(wcsstr(str, L"@device:sw:")) prefix = L"(DS) ";
		else if(wcsstr(str, L"@device:cm:")) prefix = L"(VfW) ";

		c.DisplayName = str;

		CoTaskMemFree(str);

		CComPtr<IPropertyBag> pPB;

		if(FAILED(moniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
			continue;

		CComVariant var;

		if(FAILED(pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL)))
			continue;
		
		c.FriendlyName = prefix + var.bstrVal;

		m_codecs.push_back(c);

		string s(c.FriendlyName.begin(), c.FriendlyName.end());

		ComboBoxAppend(IDC_CODECS, s.c_str(), (LPARAM)&m_codecs.back(), s == selected);
	}
	EndEnumSysDev
}

bool GSCaptureDlg::OnCommand(HWND hWnd, UINT id, UINT code)
{
	if(id == IDC_BROWSE && code == BN_CLICKED)
	{
		char buff[MAX_PATH] = {0};

		OPENFILENAME ofn;

		memset(&ofn, 0, sizeof(ofn));

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = m_hWnd;
		ofn.lpstrFile = buff;
		ofn.nMaxFile = countof(buff);
		ofn.lpstrFilter = "Avi files (*.avi)\0*.avi\0";
		ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

		strcpy(ofn.lpstrFile, m_filename.c_str());

		if(GetSaveFileName(&ofn))
		{
			m_filename = ofn.lpstrFile;

			SetText(IDC_FILENAME, m_filename.c_str());
		}

		return true;
	}
	else if(id == IDC_CONFIGURE && code == BN_CLICKED)
	{
		Codec c;

		if(GetSelCodec(c) == 1)
		{
			if(CComQIPtr<ISpecifyPropertyPages> pSPP = c.filter)
			{
				CAUUID caGUID;

				memset(&caGUID, 0, sizeof(caGUID));

				if(SUCCEEDED(pSPP->GetPages(&caGUID)))
				{
					IUnknown* lpUnk = NULL;
					pSPP.QueryInterface(&lpUnk);
					OleCreatePropertyFrame(m_hWnd, 0, 0, c.FriendlyName.c_str(), 1, (IUnknown**)&lpUnk, caGUID.cElems, caGUID.pElems, 0, 0, NULL);
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

		return true;
	}
	else if(id == IDOK)
	{
		m_width = GetTextAsInt(IDC_WIDTH);
		m_height = GetTextAsInt(IDC_HEIGHT);
		m_filename = GetText(IDC_FILENAME);

		Codec c;

		if(GetSelCodec(c) == 0)
		{
			return false;
		}

		m_enc = c.filter;

		theApp.SetConfig("CaptureWidth", m_width);
		theApp.SetConfig("CaptureHeight", m_height);
		theApp.SetConfig("CaptureFileName", m_filename.c_str());

		wstring s = wstring(c.DisplayName.m_str);

		theApp.SetConfig("CaptureVideoCodecDisplayName", string(s.begin(), s.end()).c_str());
	}

	return __super::OnCommand(hWnd, id, code);
}