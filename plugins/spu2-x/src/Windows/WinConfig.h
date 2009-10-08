
#pragma once

#ifndef WINVER
#	define WINVER 0x0501
#	define _WIN32_WINNT 0x0501
#endif

#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <initguid.h>
#include <tchar.h>

#include "resource.h"

extern HINSTANCE hInstance;

#define SET_CHECK(idc,value) SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,((value)==0)?BST_UNCHECKED:BST_CHECKED,0)
#define HANDLE_CHECK(idc,hvar)	case idc: (hvar) = !(hvar); SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar)?BST_CHECKED:BST_UNCHECKED,0); break
#define HANDLE_CHECKNB(idc,hvar)case idc: (hvar) = !(hvar); SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar)?BST_CHECKED:BST_UNCHECKED,0)
#define ENABLE_CONTROL(idc,value) EnableWindow(GetDlgItem(hWnd,idc),value)

#define INIT_SLIDER(idc,minrange,maxrange,tickfreq,pagesize,linesize) \
	SendMessage(GetDlgItem(hWnd,idc),TBM_SETRANGEMIN,FALSE,minrange); \
	SendMessage(GetDlgItem(hWnd,idc),TBM_SETRANGEMAX,FALSE,maxrange); \
	SendMessage(GetDlgItem(hWnd,idc),TBM_SETTICFREQ,tickfreq,0); \
	SendMessage(GetDlgItem(hWnd,idc),TBM_SETPAGESIZE,0,pagesize); \
	SendMessage(GetDlgItem(hWnd,idc),TBM_SETLINESIZE,0,linesize)

#define HANDLE_SCROLL_MESSAGE(idc,idcDisplay) \
	if((HWND)lParam == GetDlgItem(hWnd,idc)) return DoHandleScrollMessage( GetDlgItem(hWnd,idcDisplay), wParam, lParam )


// *** BEGIN DRIVER-SPECIFIC CONFIGURATION ***
// -------------------------------------------

struct CONFIG_XAUDIO2
{
	std::wstring Device;
	s8 NumBuffers;

	CONFIG_XAUDIO2() :
		Device(),
		NumBuffers( 2 )
	{
	}
};

struct CONFIG_WAVEOUT
{
	std::wstring Device;
	s8 NumBuffers;

	CONFIG_WAVEOUT() :
		Device(),
		NumBuffers( 4 )
	{
	}
};

extern CONFIG_WAVEOUT Config_WaveOut;
extern CONFIG_XAUDIO2 Config_XAudio2;
