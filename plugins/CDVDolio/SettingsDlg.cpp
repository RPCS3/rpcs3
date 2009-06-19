// SettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "CDVD.h"
#include "SettingsDlg.h"
#include <dbt.h>

CDVDSettingsDlg::CDVDSettingsDlg()
	: CDVDDialog(IDD_CONFIG)
{
}

void CDVDSettingsDlg::OnInit()
{
	__super::OnInit();

	UpdateDrives();

	SetText(IDC_EDIT1, theApp.GetConfig("iso", "").c_str());
}

bool CDVDSettingsDlg::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_DEVICECHANGE && (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE))
	{
		UpdateDrives();

		DEV_BROADCAST_HDR* p = (DEV_BROADCAST_HDR*)lParam;

		if(p->dbch_devicetype == DBT_DEVTYP_VOLUME)
		{
			DEV_BROADCAST_VOLUME* v = (DEV_BROADCAST_VOLUME*)p;

			for(int i = 0; i < 32; i++)
			{
				if(v->dbcv_unitmask & (1 << i))
				{
					// printf("%c:\n", 'A' + i);

					// TODO
				}
			}
		}
	}

	return __super::OnMessage(message, wParam, lParam);
}

bool CDVDSettingsDlg::OnCommand(HWND hWnd, UINT id, UINT code)
{
	if(id == IDOK)
	{
		INT_PTR data = 0;

		if(!ComboBoxGetSelData(IDC_COMBO1, data))
		{
			data = -1;
		}

		theApp.SetConfig("drive", (int)data);

		theApp.SetConfig("iso", GetText(IDC_EDIT1).c_str());
	}
	else if(id == IDC_BUTTON1 && code == BN_CLICKED)
	{
		char buff[MAX_PATH] = {0};

		OPENFILENAME ofn;

		memset(&ofn, 0, sizeof(ofn));

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = m_hWnd;
		ofn.lpstrFile = buff;
		ofn.nMaxFile = countof(buff);
		ofn.lpstrFilter = "ISO file\0*.iso\0All files\0*.*\0";
		ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		strcpy(ofn.lpstrFile, GetText(IDC_EDIT1).c_str());

		if(GetOpenFileName(&ofn))
		{
			SetText(IDC_EDIT1, ofn.lpstrFile);

			HWND hWnd = GetDlgItem(m_hWnd, IDC_COMBO1);

			SendMessage(hWnd, CB_SETCURSEL, SendMessage(hWnd, CB_GETCOUNT, 0, 0) - 1, 0);
		}

		return true;
	}

	return __super::OnCommand(hWnd, id, code);
}

void CDVDSettingsDlg::UpdateDrives()
{
	int drive = theApp.GetConfig("drive", -1);

	INT_PTR data = 0;

	if(ComboBoxGetSelData(IDC_COMBO1, data))
	{
		drive = (int)data;
	}

	vector<CDVDSetting> drives;

	for(int i = 'A'; i <= 'Z'; i++)
	{
		string path = format("%c:", i);

		if(GetDriveType(path.c_str()) == DRIVE_CDROM)
		{
			string label = path;

			path = format("\\\\.\\%c:", i);

			CDVD cdvd;
			
			if(cdvd.Open(path.c_str()))
			{
				string str = cdvd.GetLabel();

				if(str.empty())
				{
					str = "(no label)";
				}

				label = "[" + label + "] " + str;
			}
			else
			{
				label = "[" + label + "] (not detected)";
			}

			CDVDSetting s;

			s.id = i;
			s.name = label;

			drives.push_back(s);
		}
	}

	{
		CDVDSetting s;

		s.id = -1;
		s.name = "Other...";

		drives.push_back(s);
	}

	ComboBoxInit(IDC_COMBO1, &drives[0], drives.size(), drive);
}
