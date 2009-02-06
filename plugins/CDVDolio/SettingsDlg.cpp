// SettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "cdvd.h"
#include "SettingsDlg.h"
#include <dbt.h>
#include <afxdlgs.h>

// CSettingsDlg dialog

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialog)

CSettingsDlg::CSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSettingsDlg::IDD, pParent)
	, m_iso(_T(""))
{

}

CSettingsDlg::~CSettingsDlg()
{
}

BOOL CSettingsDlg::OnInitDialog()
{
	__super::OnInitDialog();

	InitDrive();
	InitISO();

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSettingsDlg::InitDrive()
{
	int drive = AfxGetApp()->GetProfileInt(_T("Settings"), _T("drive"), -1);

	int sel = m_drive.GetCurSel();

	if(sel >= 0)
	{
		drive = m_drive.GetItemData(sel);
	}

	while(m_drive.GetCount() > 0)
	{
		m_drive.DeleteString(0);
	}

	for(int i = 'A'; i <= 'Z'; i++)
	{
		CString path;

		path.Format(_T("%c:"), i);

		if(GetDriveType(path) == DRIVE_CDROM)
		{
			CString label = path;

			path.Format(_T("\\\\.\\%c:"), i);

			CDVD cdvd;
			
			if(cdvd.Open(path))
			{
				CString str = cdvd.GetLabel();

				if(str.IsEmpty())
				{
					str = _T("(no label)");
				}

				label.Format(_T("[%s] %s"), CString(label), str);
			}
			else
			{
				label.Format(_T("[%s] (not detected)"), CString(label));
			}

			m_drive.SetItemData(m_drive.AddString(label), (DWORD_PTR)i);
		}
	}

	m_drive.SetItemData(m_drive.AddString(_T("Other...")), (DWORD_PTR)-1);

	for(int i = 0, j = m_drive.GetCount(); i < j; i++)
	{
		if((int)m_drive.GetItemData(i) == drive)
		{
			m_drive.SetCurSel(i);

			return;
		}
	}

	m_drive.SetCurSel(-1);
}

void CSettingsDlg::InitISO()
{
	m_iso = AfxGetApp()->GetProfileString(_T("Settings"), _T("iso"), _T(""));
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_drive);
	DDX_Text(pDX, IDC_EDIT1, m_iso);
}

BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, &CSettingsDlg::OnBrowse)
	ON_BN_CLICKED(IDOK, &CSettingsDlg::OnBnClickedOk)
END_MESSAGE_MAP()

// CSettingsDlg message handlers

LRESULT CSettingsDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_DEVICECHANGE)
	{
		if(wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE)
		{
			InitDrive();

			DEV_BROADCAST_HDR* p = (DEV_BROADCAST_HDR*)lParam;

			if(p->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				DEV_BROADCAST_VOLUME* v = (DEV_BROADCAST_VOLUME*)p;

				for(int i = 0; i < 32; i++)
				{
					if(v->dbcv_unitmask & (1 << i))
					{
						TRACE(_T("%c:\n"), 'A' + i);

						// TODO
					}
				}
			}
		}
	}

	return __super::WindowProc(message, wParam, lParam);
}

void CSettingsDlg::OnBrowse()
{
	UpdateData();

	CFileDialog fd(TRUE, NULL, m_iso, 
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY, 
		_T("ISO file|*.iso|All files|*.*|"), this);

	if(fd.DoModal() == IDOK)
	{
		m_iso = fd.GetPathName();

		UpdateData(FALSE);

		for(int i = 0, j = m_drive.GetCount(); i < j; i++)
		{
			if((int)m_drive.GetItemData(i) < 0)
			{
				m_drive.SetCurSel(i);

				break;
			}
		}
	}
}

void CSettingsDlg::OnBnClickedOk()
{
	UpdateData();

	int i = m_drive.GetCurSel();

	if(i >= 0)
	{
		i = (int)m_drive.GetItemData(i);
	}

	AfxGetApp()->WriteProfileInt(_T("Settings"), _T("drive"), i);

	AfxGetApp()->WriteProfileString(_T("Settings"), _T("iso"), m_iso);

	OnOK();
}
