#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>

#include "Config.h"
#include "resource.h"
#include "DEV9.h"
#include "pcap.h"
#include "pcap_io.h"
#include "net.h"
#include "tap.h"

extern HINSTANCE hInst;
//HANDLE handleDEV9Thread = NULL;
//DWORD dwThreadId, dwThrdParam;

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "Dev9linuz Msg", 0);
}

void OnInitDialog(HWND hW) {
	char *dev;
	//int i;

	LoadConf();

	ComboBox_AddString(GetDlgItem(hW, IDC_BAYTYPE), "Expansion");
	ComboBox_AddString(GetDlgItem(hW, IDC_BAYTYPE), "PC Card");
	for (int j=0;j<2;j++)
	{
	for (int i=0; i<pcap_io_get_dev_num(); i++) {
		dev = pcap_io_get_dev_desc(i,j);
		int itm=ComboBox_AddString(GetDlgItem(hW, IDC_ETHDEV), dev);
		ComboBox_SetItemData(GetDlgItem(hW, IDC_ETHDEV),itm,strdup(pcap_io_get_dev_name(i,j)));
		if (strcmp(pcap_io_get_dev_name(i,j), config.Eth) == 0) {
			ComboBox_SetCurSel(GetDlgItem(hW, IDC_ETHDEV), itm);
		}
	}
	}
	vector<tap_adapter> * al=GetTapAdapters();
	for (int i=0; i<al->size(); i++) {
		
		int itm=ComboBox_AddString(GetDlgItem(hW, IDC_ETHDEV), al[0][i].name.c_str());
		ComboBox_SetItemData(GetDlgItem(hW, IDC_ETHDEV),itm,strdup( al[0][i].guid.c_str()));
		if (strcmp(al[0][i].guid.c_str(), config.Eth) == 0) {
			ComboBox_SetCurSel(GetDlgItem(hW, IDC_ETHDEV), itm);
		}
	}

	Edit_SetText(GetDlgItem(hW, IDC_HDDFILE), config.Hdd);

	Button_SetCheck(GetDlgItem(hW, IDC_ETHENABLED), config.ethEnable);
	Button_SetCheck(GetDlgItem(hW, IDC_HDDENABLED), config.hddEnable);
}

void OnOk(HWND hW) {
	int i = ComboBox_GetCurSel(GetDlgItem(hW, IDC_ETHDEV));
	char* ptr=(char*)ComboBox_GetItemData(GetDlgItem(hW, IDC_ETHDEV),i);
	strcpy(config.Eth, ptr);
	Edit_GetText(GetDlgItem(hW, IDC_HDDFILE), config.Hdd, 256);

	config.ethEnable = Button_GetCheck(GetDlgItem(hW, IDC_ETHENABLED));
	config.hddEnable = Button_GetCheck(GetDlgItem(hW, IDC_HDDENABLED));

	SaveConf();

	EndDialog(hW, TRUE);
}

BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	
	switch(uMsg) {
		case WM_INITDIALOG:
			OnInitDialog(hW);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					EndDialog(hW, FALSE);
					return TRUE;
				case IDOK:
					OnOk(hW);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

void CALLBACK DEV9configure() {
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_CONFIG),
              GetActiveWindow(),  
             (DLGPROC)ConfigureDlgProc); 
		//SysMessage("Nothing to Configure");
}

void CALLBACK DEV9about() {
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_ABOUT),
              GetActiveWindow(),  
              (DLGPROC)AboutDlgProc);
}

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason, 
                      LPVOID lpReserved) {
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}
/*
UINT DEV9ThreadProc() {
	DEV9thread();

	return 0;
}*/
NetAdapter* GetNetAdapter()
{
	if(config.Eth[0]=='p')
	{
		return new PCAPAdapter();
	}
	else if (config.Eth[0]=='t')
	{
		return new TAPAdapter();
	}
	else
		return 0;
}
s32  _DEV9open() 
{
	//handleDEV9Thread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) DEV9ThreadProc, &dwThrdParam, CREATE_SUSPENDED, &dwThreadId);
	//SetThreadPriority(handleDEV9Thread,THREAD_PRIORITY_HIGHEST);
	//ResumeThread (handleDEV9Thread);
	NetAdapter* na=GetNetAdapter();
	if (!na)
		emu_printf("Failed to GetNetAdapter()\n");
	InitNet( na);
	return 0;
}

void _DEV9close() {
	//TerminateThread(handleDEV9Thread,0);
	//handleDEV9Thread = NULL;
	TermNet();
}
