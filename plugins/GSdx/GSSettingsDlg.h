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

#pragma once

#include "GSSetting.h"
#include "resource.h"

class GSSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(GSSettingsDlg)

private:
	list<D3DDISPLAYMODE> m_modes;

public:
	GSSettingsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~GSSettingsDlg();

	static GSSetting g_renderers[]; 
	static GSSetting g_psversion[];
	static GSSetting g_interlace[];
	static GSSetting g_aspectratio[];

// Dialog Data
	enum { IDD = IDD_CONFIG };
	CComboBox m_resolution;
	CComboBox m_renderer;
	CComboBox m_psversion;
	CComboBox m_interlace;
	CComboBox m_aspectratio;
	int m_filter;
	CSpinButtonCtrl m_resx;
	CSpinButtonCtrl m_resy;
	CSpinButtonCtrl m_swthreads;
	BOOL m_nativeres;
	CEdit m_resxedit;
	CEdit m_resyedit;
	CEdit m_swthreadsedit;
	BOOL m_vsync;
	BOOL m_logz;
	BOOL m_fba;
	BOOL m_aa1;
	BOOL m_blur;

protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnKickIdle();
	afx_msg void OnUpdateResolution(CCmdUI* pCmdUI);
	afx_msg void OnUpdateD3D9Options(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSWOptions(CCmdUI* pCmdUI);
	afx_msg void OnCbnSelchangeCombo1();
};

