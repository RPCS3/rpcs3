#pragma once

#include "resource.h"
#include <commctrl.h>

#define SET_CHECK(idc,value) SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,((value)==0)?BST_UNCHECKED:BST_CHECKED,0)
#define HANDLE_CHECK(idc,hvar)	case idc: hvar=hvar?0:1; SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar==1)?BST_CHECKED:BST_UNCHECKED,0); break
#define HANDLE_CHECKNB(idc,hvar)case idc: hvar=hvar?0:1; SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar==1)?BST_CHECKED:BST_UNCHECKED,0)
#define ENABLE_CONTROL(idc,value) EnableWindow(GetDlgItem(hWnd,idc),value)

#define INIT_SLIDER(idc,minrange,maxrange,tickfreq,pagesize,linesize) \
			SendMessage(GetDlgItem(hWnd,idc),TBM_SETRANGEMIN,FALSE,minrange); \
			SendMessage(GetDlgItem(hWnd,idc),TBM_SETRANGEMAX,FALSE,maxrange); \
			SendMessage(GetDlgItem(hWnd,idc),TBM_SETTICFREQ,tickfreq,0); \
			SendMessage(GetDlgItem(hWnd,idc),TBM_SETPAGESIZE,0,pagesize); \
			SendMessage(GetDlgItem(hWnd,idc),TBM_SETLINESIZE,0,linesize)
