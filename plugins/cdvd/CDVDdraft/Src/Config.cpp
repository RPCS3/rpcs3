// Config.cpp : implementation file
//

#include "stdafx.h"
#include "cdvddraft.h"
#include "Config.h"
#include <atlbase.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfig dialog


CConfig::CConfig(CWnd* pParent /*=NULL*/)
	: CDialog(CConfig::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfig)
	m_nInterface = 0;
	m_nBuffMode  = 0;
	m_nDrive     = 0;
	m_nReadMode  = 0;
	//}}AFX_DATA_INIT
}

void CConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfig)
	DDX_Control(pDX, IDC_DRIVE, m_ctrlDrive);
	DDX_CBIndex(pDX, IDC_INTERFACE, m_nInterface);
	DDX_CBIndex(pDX, IDC_PREFETCH, m_nBuffMode);
	DDX_CBIndex(pDX, IDC_DRIVE, m_nDrive);
	DDX_CBIndex(pDX, IDC_READMODE, m_nReadMode);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CConfig, CDialog)
	//{{AFX_MSG_MAP(CConfig)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfig message handlers
BOOL CConfig::OnInitDialog()
{
	int nResult = CDialog::OnInitDialog();

    TRACE("on init dialog.\n");
    int  curr_intf = (CDVD_INTERFACE_TYPE) 
        cdvd_init(CDVD_INTF_UNKNOWN);

    int init_okay = CDVD_ERROR_FAIL;

    if(curr_intf == CDVD_INTF_UNKNOWN)
    {
        if(CCdvd::Aspi_CheckValidInstallation() 
            == CDVD_ERROR_SUCCESS)
        {
            init_okay = cdvd_init(CDVD_INTF_ASPI);
            TRACE("config: init_aspi: %ld\n", init_okay);
        }
        else if(CCdvd::GetWin32OSType() == WINNTNEW)
        {
            TRACE("config: init_ioctl: %ld\n", init_okay);
            init_okay = cdvd_init(CDVD_INTF_IOCTL);
        }
        else 
        {
            curr_intf = CDVD_INTF_UNKNOWN;
            TRACE("config: init_unknown: %ld\n", init_okay);
        }
    }
    // else already either init to ioctl or aspi
    else
    {
        TRACE("config: already previously init.\n");
        init_okay = CDVD_ERROR_SUCCESS;
    }

    if(init_okay == CDVD_ERROR_SUCCESS)
    {
        ADAPTERINFO info;
        int ndrives = cdvd_getnumdrives();
        
        if(ndrives > 0)
        {
            for(int i = 0; i < ndrives; i++)
            {
                CString str;
                info = cdvd_getadapterdetail(i);
		        str.Format(_T(" %d:%d:%d           %s"), 
                           info.ha, info.id, info.lun, info.name
                );			
                TRACE("str: %s\n", str);
		        m_ctrlDrive.InsertString(i, str);
            }
        }
        else
        {
		    MessageBox(_T("Unable to locate c/dvd drives on this computer\n"), 
                       _T("Drive Error"), 
                       MB_OK|MB_ICONERROR);
        }

        if(curr_intf == CDVD_INTF_UNKNOWN && 
           init_okay == CDVD_ERROR_SUCCESS)
            cdvd_shutdown();
    }

    LoadConfig();

	return TRUE;
}

void CConfig::OnOK() 
{
	SaveConfig();
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// save/load stuff
BOOL CConfig::SaveConfig()
{
	CRegKey rKey;
	
	if(rKey.Create(HKEY_CURRENT_USER, PLUGIN_REG_PATH) == ERROR_SUCCESS)
	{
        UpdateData(TRUE);
        if(m_nDrive != -1)
        {
            rKey.SetValue(m_nDrive, _T("DriveNum"));
            rKey.SetValue(m_nBuffMode, _T("BuffMode"));
            rKey.SetValue(m_nInterface, _T("Interface"));
            rKey.SetValue(m_nReadMode, _T("ReadMode"));
        }
        rKey.Close();
	}

	return TRUE;
}

BOOL CConfig::LoadConfig()
{
	CRegKey rKey;
	
	if(rKey.Create(HKEY_CURRENT_USER, PLUGIN_REG_PATH) == ERROR_SUCCESS)
	{
        DWORD val;

        if(rKey.QueryValue(val, _T("DriveNum")) == ERROR_SUCCESS)
            m_nDrive = val;
        if(rKey.QueryValue(val, _T("BuffMode")) == ERROR_SUCCESS)
            m_nBuffMode = val;
        if(rKey.QueryValue(val, _T("Interface")) == ERROR_SUCCESS)
            m_nInterface = val;
        if(rKey.QueryValue(val, _T("ReadMode")) == ERROR_SUCCESS)
            m_nReadMode = val;

		UpdateData(FALSE);
		rKey.Close();
	}

	return TRUE;
}
