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

#pragma once

#include "resource.h"
#include "afxwin.h"

class CGSSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CGSSettingsDlg)

private:
	CList<D3DDISPLAYMODE> m_modes;

public:
	CGSSettingsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGSSettingsDlg();

// Dialog Data
	enum { IDD = IDD_CONFIG };
	CComboBox m_resolution;
	CComboBox m_renderer;
	CComboBox m_psversion;
	BOOL m_fEnablePalettizedTextures;
	BOOL m_fEnableTvOut;
	BOOL m_fRecordState;
	BOOL m_fNloopHack;
	CString m_strRecordState;
	BOOL m_fLinearTexFilter;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
};
