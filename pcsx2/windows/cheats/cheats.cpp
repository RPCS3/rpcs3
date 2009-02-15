/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include "../Win32.h"

#include <commctrl.h>
#include <vector>

#include "PS2Edefs.h"
#include "Memory.h"
#include "IopMem.h"

#include "cheats.h"
#include "../../patch.h"

class result
{
public:
	u32 address;
	result(u32 addr):
		address(addr)
	{
	}
};

char *sizenames[4]={"char","short","word","double"};

char mtext[100];

HINSTANCE pInstance;

int TSelect;

bool Unsigned;
int Source;
int Compare;
int Size;
int CompareTo;

u64 CompareValue;

std::vector<result> results;

//int mresults;

bool FirstSearch;

bool FirstShow;

char olds[Ps2MemSize::Base];

char tn[100];
char to[100];
char tv[100];

u8 *mptr[2];

int  msize[2]={0x02000000,0x00200000};

int lticks;

HWND hWndFinder;

LVCOLUMN cols[3]={
	{LVCF_TEXT|LVCF_WIDTH,0,60,"Address",0,0,0},
	{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,60,"Old V",1,0,0},
	{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,60,"New V",2,0,0}
};

LVITEM   item[3]={
	{LVIF_TEXT|LVIF_STATE,0,0,0,0,tn,0,0,0,0},
	{LVIF_TEXT|LVIF_STATE,0,1,0,0,to,0,0,0,0},
	{LVIF_TEXT|LVIF_STATE,0,2,0,0,tv,0,0,0,0}
};

void DoEvents()
{
	MSG msg;
	while(PeekMessage(&msg,0,0,0,PM_REMOVE)!=0)
		DispatchMessage(&msg);
}

void UpdateStatus()
{
	int nticks=GetTickCount();
	if((nticks-lticks)>250)
	{
		int nshown=ListView_GetItemCount(GetDlgItem(hWndFinder,IDC_RESULTS));
		sprintf(mtext,"%d matches found (%d shown).",results.size(),nshown);
		SetWindowText(GetDlgItem(hWndFinder,IDC_MATCHES),mtext);
		lticks=nticks;
		DoEvents();
	}
}

void SearchReset()
{
	memcpy(olds,mptr[Source],msize[Source]);
	FirstSearch=true;

	results.clear();
	
}

int AddResult(u32 addr)
{
	result nr=result(addr);
	results.push_back(nr);
	return 1;
}

bool CompareAny(u64 val,u64 cto)
{

	if(Unsigned) 
	{
		switch(Size)
		{
			case 0:
				val=(u8)val;
				cto=(u8)cto;
				break;
			case 1:
				val=(u16)val;
				cto=(u16)cto;
				break;
			case 2:
				val=(u32)val;
				cto=(u32)cto;
				break;
			case 3:
				break;
			default:return false;
		}
		switch(Compare)
		{
			case 0: /* EQ */
				return val==cto;
			case 1: /* GT */
				return val> cto;
			case 2: /* LT */
				return val< cto;
			case 3: /* GE */
				return val>=cto;
			case 4: /* LE */
				return val<=cto;
			default:/* NE */
				return val!=cto;
		}
	}
	else
	{
		switch(Size)
		{
			case 0:
				val=(s8)val;
				cto=(s8)cto;
				break;
			case 1:
				val=(s16)val;
				cto=(s16)cto;
				break;
			case 2:
				val=(s32)val;
				cto=(s32)cto;
				break;
			case 3:
				break;
			default:return false;
		}
		switch(Compare)
		{
			case 0: /* EQ */
				return (s64)val==(s64)cto;
			case 1: /* GT */
				return (s64)val> (s64)cto;
			case 2: /* LT */
				return (s64)val< (s64)cto;
			case 3: /* GE */
				return (s64)val>=(s64)cto;
			case 4: /* LE */
				return (s64)val<=(s64)cto;
			default:/* NE */
				return (s64)val!=(s64)cto;
		}
	}
}

void SearchFirst()
{
	int MSize=1<<Size;
	u64*cur=(u64*)mptr[Source];
	u64 cto=CompareValue;
	u64 val;
	u64 old;
	int addr;

	addr=0;
	while((addr+MSize)<msize[Source])
	{
		(u64)val=0;
		memcpy(&val,cur,MSize);			//update the buffer
		memcpy(&old,olds+addr,MSize);	//
		if(CompareTo==0)
		{
			cto=old;
		}

		if(CompareAny(val,cto)) {
			AddResult(addr);
			cur=(u64*)(((char*)cur)+MSize);
			addr+=MSize;
			UpdateStatus();
		}
		else {
			cur=(u64*)(((char*)cur)+1);
			addr+=1;
		}
	}
	FirstSearch=false;
}

void SearchMore()
{
	u32 i;
	int MSize=1<<Size;
	u64 cto=CompareValue;
	u64 val;
	u64 old;
	int addr;

	std::vector<result> oldr=std::vector<result>(results);

	results.clear();

	for(i=0;i<oldr.size();i++)
	{
		val =0;
		addr=oldr[i].address;
		memcpy(&val,mptr[Source]+addr,MSize);
		memcpy(&old,olds+addr,MSize);
		if(CompareTo==0)
		{
			cto=old;
		}
		
		if(CompareAny(val,cto)) {
			AddResult(addr);
			UpdateStatus();
		}
	}
}

#define INIT_CHECK(idc,value) SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,value?BST_UNCHECKED:BST_CHECKED,0)
#define HANDLE_CHECK(idc,hvar)	case idc: hvar=hvar?0:1; SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar==1)?BST_CHECKED:BST_UNCHECKED,0); break
#define HANDLE_CHECKNB(idc,hvar)case idc: hvar=hvar?0:1; SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar==1)?BST_CHECKED:BST_UNCHECKED,0)
#define ENABLE_CONTROL(idc,value) EnableWindow(GetDlgItem(hWnd,idc),value)

#define HANDLE_GROUP_ITEM(idc)	case idc: 
#define BEGIN_GROUP_HANDLER(first,hvar) TSelect=wmId;hvar=TSelect-first
#define GROUP_SELECT(idc)	SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(TSelect==idc)?BST_CHECKED:BST_UNCHECKED,0)
#define GROUP_INIT(first,hvar) TSelect=first+hvar

BOOL CALLBACK AddCheatProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent,i,mresults;
	static HWND hParent;
	UINT state;
	u64 value;
	static int Selected;

	switch(uMsg)
	{

		case WM_PAINT:
			INIT_CHECK(IDC_UNSIGNED,Unsigned);
			return FALSE;
		case WM_INITDIALOG:
			hParent=(HWND)lParam;

			mresults=ListView_GetItemCount(GetDlgItem(hParent,IDC_RESULTS));
			for(i=0;i<mresults;i++)
			{
				state=ListView_GetItemState(GetDlgItem(hParent,IDC_RESULTS),i,LVIS_SELECTED);
				if(state==LVIS_SELECTED)
				{
					Selected=i;
					ListView_GetItemText(GetDlgItem(hParent,IDC_RESULTS),i,0,tn,100);
					ListView_GetItemText(GetDlgItem(hParent,IDC_RESULTS),i,0,tv,100);

					sprintf(to,"patch=0,%s,%s,%s,<value>",
						Source?"IOP":"EE",
						tn,
						sizenames[Size]);

					SetWindowText(GetDlgItem(hWnd,IDC_ADDR),tn);
					SetWindowText(GetDlgItem(hWnd,IDC_VALUE),tv);
					SetWindowText(GetDlgItem(hWnd,IDC_NAME),to);

					break;
				}
			}


			break;
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDCANCEL:
					EndDialog(hWnd,1);
					break;
				
				case IDOK:
					GetWindowText(GetDlgItem(hWnd,IDC_VALUE),tv,100);
					value=_atoi64(tv);
					AddPatch(1,Source+1,results[Selected].address,Size+1,value);

					EndDialog(hWnd,1);
					break;


				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

void AddCheat(HINSTANCE hInstance, HWND hParent)
{
	INT_PTR retret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_ADD),hParent,(DLGPROC)AddCheatProc,(LPARAM)hParent);
}

void AddResults(HWND hWnd)
{
	int i,mresults;
	u64 sizemask=(1<<(1<<Size))-1;

	mresults=results.size();
	if(mresults>32768) {
		mresults=32768;
	}

	ListView_DeleteAllItems(GetDlgItem(hWnd,IDC_RESULTS));

	for(i=0;i<mresults;i++)
	{
		u64 o=0;
		memcpy(&o,olds+results[i].address,(1<<Size));

		u64 v=*(u64*)(mptr[Source]+results[i].address);

		sprintf(tn,"%08x",results[i].address);

		if(Unsigned) 
		{
			sprintf(to,"%I64u",(u64)o&sizemask);
			sprintf(tv,"%I64u",(u64)v&sizemask);
		}
		else
		{
			switch(Size)
			{
				case 0:
					o=(s64)(s8)o;
					v=(s64)(s8)v;
					break;
				case 1:
					o=(s64)(s16)o;
					v=(s64)(s16)v;
					break;
				case 2:
					o=(s64)(s32)o;
					v=(s64)(s32)v;
					break;
			}
			sprintf(to,"%I64d",(s64)o);
			sprintf(tv,"%I64d",(s64)v);
		}

		item[0].iItem=i;
		item[1].iItem=i;
		item[2].iItem=i;

		//Listview Sample Data
		ListView_InsertItem(GetDlgItem(hWnd,IDC_RESULTS),&item[0]);
		ListView_SetItem(GetDlgItem(hWnd,IDC_RESULTS),&item[1]);
		ListView_SetItem(GetDlgItem(hWnd,IDC_RESULTS),&item[2]);
		UpdateStatus();
	}

	if(results.size()>32768) {
		sprintf(mtext,"%d matches found (32768 shown).",results.size());
		SetWindowText(GetDlgItem(hWnd,IDC_MATCHES),mtext);
	}
	else {
		sprintf(mtext,"%d matches found.",results.size());
		SetWindowText(GetDlgItem(hWnd,IDC_MATCHES),mtext);
	}

}

BOOL CALLBACK FinderProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	LRESULT lStyle;

	switch(uMsg)
	{

		case WM_PAINT:
			INIT_CHECK(IDC_UNSIGNED,Unsigned);
			return FALSE;

		case WM_INITDIALOG:
			mptr[0]=psM;
			mptr[1]=psxM;

			hWndFinder=hWnd;

			ENABLE_CONTROL(IDC_VALUE,false);

			GROUP_INIT(IDC_EE,Source);
				GROUP_SELECT(IDC_EE);
				GROUP_SELECT(IDC_IOP);

			GROUP_INIT(IDC_OLD,CompareTo);
				GROUP_SELECT(IDC_OLD);
				GROUP_SELECT(IDC_SET);
				ENABLE_CONTROL(IDC_VALUE,(CompareTo!=0));

			GROUP_INIT(IDC_EQ,Compare);
				GROUP_SELECT(IDC_EQ);
				GROUP_SELECT(IDC_GT);
				GROUP_SELECT(IDC_LT);
				GROUP_SELECT(IDC_GE);
				GROUP_SELECT(IDC_LE);
				GROUP_SELECT(IDC_NE);

			GROUP_INIT(IDC_8B,Size);
				GROUP_SELECT(IDC_8B);
				GROUP_SELECT(IDC_16B);
				GROUP_SELECT(IDC_32B);
				GROUP_SELECT(IDC_64B);

			INIT_CHECK(IDC_UNSIGNED,Unsigned);

			//Listview Init
			lStyle = SendMessage(GetDlgItem(hWnd,IDC_RESULTS), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
			SendMessage(GetDlgItem(hWnd,IDC_RESULTS), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lStyle | LVS_EX_FULLROWSELECT);

			ListView_InsertColumn(GetDlgItem(hWnd,IDC_RESULTS),0,&cols[0]);
			ListView_InsertColumn(GetDlgItem(hWnd,IDC_RESULTS),1,&cols[1]);
			ListView_InsertColumn(GetDlgItem(hWnd,IDC_RESULTS),2,&cols[2]);

			if(FirstShow)
			{
				SearchReset();
				SetWindowText(GetDlgItem(hWnd,IDC_MATCHES),"ready to search.");
				FirstShow=false;
			}
			else {
				AddResults(hWnd);
			}

			break;
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDCANCEL:
					EndDialog(hWnd,1);
					break;
				
				case IDC_ADD:
					AddCheat(pInstance,hWnd);
					break;

				case IDC_RESET:
					ENABLE_CONTROL(IDC_EE,		true);
					ENABLE_CONTROL(IDC_IOP,		true);
					ENABLE_CONTROL(IDC_STATUS,	true);
					ENABLE_CONTROL(IDC_UNSIGNED,true);
					ENABLE_CONTROL(IDC_8B,		true);
					ENABLE_CONTROL(IDC_16B,		true);
					ENABLE_CONTROL(IDC_32B,		true);
					ENABLE_CONTROL(IDC_64B,		true);
					SetWindowText(GetDlgItem(hWnd,IDC_MATCHES),"ready to search.");
					SearchReset();
					ListView_DeleteAllItems(GetDlgItem(hWnd,IDC_RESULTS));
					break;

				case IDC_SEARCH:
					GetWindowText(GetDlgItem(hWnd,IDC_VALUE),mtext,100);
					CompareValue=atoi(mtext);
					ENABLE_CONTROL(IDC_SEARCH,	false);
					ENABLE_CONTROL(IDC_RESET,	false);
					ENABLE_CONTROL(IDC_ADD,		false);
					ENABLE_CONTROL(IDCANCEL,	false);
					if(FirstSearch) {
						ENABLE_CONTROL(IDC_EE,		false);
						ENABLE_CONTROL(IDC_IOP,		false);
						ENABLE_CONTROL(IDC_STATUS,	false);
						ENABLE_CONTROL(IDC_UNSIGNED,false);
						ENABLE_CONTROL(IDC_8B,		false);
						ENABLE_CONTROL(IDC_16B,		false);
						ENABLE_CONTROL(IDC_32B,		false);
						ENABLE_CONTROL(IDC_64B,		false);
						SearchFirst();
					}
					else			SearchMore();
					
					AddResults(hWnd);

					memcpy(olds,mptr[Source],msize[Source]);

					ENABLE_CONTROL(IDC_SEARCH,	true);
					ENABLE_CONTROL(IDC_RESET,	true);
					ENABLE_CONTROL(IDC_ADD,		true);
					ENABLE_CONTROL(IDCANCEL,	true);

					break;

				HANDLE_CHECK(IDC_UNSIGNED,Unsigned);

				HANDLE_GROUP_ITEM(IDC_EE);
				HANDLE_GROUP_ITEM(IDC_IOP);
				BEGIN_GROUP_HANDLER(IDC_EE,Source);
					GROUP_SELECT(IDC_EE);
					GROUP_SELECT(IDC_IOP);
					break;

				HANDLE_GROUP_ITEM(IDC_OLD);
				HANDLE_GROUP_ITEM(IDC_SET);
				BEGIN_GROUP_HANDLER(IDC_OLD,CompareTo);
					GROUP_SELECT(IDC_OLD);
					GROUP_SELECT(IDC_SET);
					ENABLE_CONTROL(IDC_VALUE,(CompareTo!=0));
					break;

				HANDLE_GROUP_ITEM(IDC_EQ);
				HANDLE_GROUP_ITEM(IDC_GT);
				HANDLE_GROUP_ITEM(IDC_LT);
				HANDLE_GROUP_ITEM(IDC_GE);
				HANDLE_GROUP_ITEM(IDC_LE);
				HANDLE_GROUP_ITEM(IDC_NE);
				BEGIN_GROUP_HANDLER(IDC_EQ,Compare);
					GROUP_SELECT(IDC_EQ);
					GROUP_SELECT(IDC_GT);
					GROUP_SELECT(IDC_LT);
					GROUP_SELECT(IDC_GE);
					GROUP_SELECT(IDC_LE);
					GROUP_SELECT(IDC_NE);
					break;

				HANDLE_GROUP_ITEM(IDC_8B);
				HANDLE_GROUP_ITEM(IDC_16B);
				HANDLE_GROUP_ITEM(IDC_32B);
				HANDLE_GROUP_ITEM(IDC_64B);
				BEGIN_GROUP_HANDLER(IDC_8B,Size);
					GROUP_SELECT(IDC_8B);
					GROUP_SELECT(IDC_16B);
					GROUP_SELECT(IDC_32B);
					GROUP_SELECT(IDC_64B);
					break;

				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

void ShowFinder(HINSTANCE hInstance, HWND hParent)
{
	INT_PTR ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_FINDER),hParent,(DLGPROC)FinderProc,1);
}
