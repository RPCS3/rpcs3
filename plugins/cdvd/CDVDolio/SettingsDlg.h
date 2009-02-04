#pragma once
#include "afxwin.h"
#include "resource.h"

// CSettingsDlg dialog

class CSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CSettingsDlg)

protected:
	void InitDrive();
	void InitISO();

public:
	CSettingsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSettingsDlg();

	virtual BOOL OnInitDialog();

	enum { IDD = IDD_CONFIG };

	CComboBox m_drive;
	CString m_iso;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	afx_msg void OnBrowse();
	afx_msg void OnBnClickedOk();
};
