#pragma once
#include "CDVDDialog.h"
#include "resource.h"

class CDVDSettingsDlg : public CDVDDialog
{
	void UpdateDrives();

protected:
	void OnInit();
	bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
	bool OnCommand(HWND hWnd, UINT id, UINT code);

public:
	CDVDSettingsDlg();
};
