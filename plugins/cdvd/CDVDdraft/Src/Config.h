#if !defined(AFX_CONFIG_H__A5E8FC61_C0A7_11D7_8E2C_0050DA15DE89__INCLUDED_)
#define AFX_CONFIG_H__A5E8FC61_C0A7_11D7_8E2C_0050DA15DE89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Config.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConfig dialog

class CConfig : public CDialog
{
// Construction
public:
	CConfig(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfig)
	enum { IDD = IDD_CONFIG };
	CComboBox	m_ctrlDrive;
	int		m_nInterface;
	int		m_nBuffMode;
	int		m_nDrive;
	int		m_nReadMode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfig)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL SaveConfig();
	virtual BOOL LoadConfig();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfig)
		// NOTE: the ClassWizard will add member functions here
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIG_H__A5E8FC61_C0A7_11D7_8E2C_0050DA15DE89__INCLUDED_)
