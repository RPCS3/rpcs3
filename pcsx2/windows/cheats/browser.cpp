/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

using namespace std;

#include "../cheatscpp.h"

#include "PS2Edefs.h"
#include "Memory.h"
#include "Elfheader.h"
#include "cheats.h"
#include "../../patch.h"

HWND hWndBrowser;


/// ADDED CODE ///
void CreateListBox(HWND hWnd)
{
	// Setting the ListView style
	LRESULT lStyle = SendMessage(GetDlgItem(hWnd,IDC_PATCHES), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	SendMessage(GetDlgItem(hWnd,IDC_PATCHES), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, lStyle | LVS_EX_FULLROWSELECT);

	// Adding colummns to the ListView
	LVCOLUMN cols[7]={
		{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,70,"Address",0,0,0},
		{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,35,"CPU",1,0,0},
		{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,70,"Data",2,0,0},
		{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,50,"Enabled",3,0,0},
		{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,45,"Group",4,0,0},
		{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,80,"PlaceToPatch",5,0,0},
		{LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM,0,50,"Type",6,0,0}
	};

	ListView_InsertColumn(GetDlgItem(hWnd,IDC_PATCHES),0,&cols[0]);
	ListView_InsertColumn(GetDlgItem(hWnd,IDC_PATCHES),1,&cols[1]);
	ListView_InsertColumn(GetDlgItem(hWnd,IDC_PATCHES),2,&cols[2]);
	ListView_InsertColumn(GetDlgItem(hWnd,IDC_PATCHES),3,&cols[3]);
	ListView_InsertColumn(GetDlgItem(hWnd,IDC_PATCHES),4,&cols[4]);
	ListView_InsertColumn(GetDlgItem(hWnd,IDC_PATCHES),5,&cols[5]);
	ListView_InsertColumn(GetDlgItem(hWnd,IDC_PATCHES),6,&cols[6]);
}

void RefreshListBox(HWND hWnd)
{
	// First delete all items
	ListView_DeleteAllItems(GetDlgItem(hWnd,IDC_PATCHES));

	char Address[100], CPU[100], Data[100], Enabled[100], Group[100], PlaceToPatch[100], Type[100];
	
	LVITEM   item[7]={
		{LVIF_TEXT|LVIF_STATE,0,0,0,0,Address,0,0,0,0},
		{LVIF_TEXT|LVIF_STATE,0,1,0,0,CPU,0,0,0,0},
		{LVIF_TEXT|LVIF_STATE,0,2,0,0,Data,0,0,0,0},
		{LVIF_TEXT|LVIF_STATE,0,3,0,0,Enabled,0,0,0,0},
		{LVIF_TEXT|LVIF_STATE,0,4,0,0,Group,0,0,0,0},
		{LVIF_TEXT|LVIF_STATE,0,5,0,0,PlaceToPatch,0,0,0,0},
		{LVIF_TEXT|LVIF_STATE,0,6,0,0,Type,0,0,0,0}
	};

	char datatype[4][10]={"byte", "short", "word", "double"};
	char cpucore[2][10]={"EE", "IOP"};
	
	// Adding items
	for (int i = patchnumber-1; i >= 0; i--)
	{
		sprintf(Address, "0x%.8x", patch[i].addr);
		sprintf(CPU, "%s", cpucore[patch[i].cpu-1]);
		sprintf(Data, "0x%.8x", patch[i].data);
		sprintf(Enabled, "%s", patch[i].enabled?"Yes":"No");
		sprintf(Group, "%d", patch[i].group);
		sprintf(PlaceToPatch, "%d", patch[i].placetopatch);
		sprintf(Type, "%s", datatype[patch[i].type-1]);

		ListView_InsertItem(GetDlgItem(hWnd,IDC_PATCHES),&item[0]);
		ListView_SetItem(GetDlgItem(hWnd,IDC_PATCHES),&item[1]);
		ListView_SetItem(GetDlgItem(hWnd,IDC_PATCHES),&item[2]);
		ListView_SetItem(GetDlgItem(hWnd,IDC_PATCHES),&item[3]);
		ListView_SetItem(GetDlgItem(hWnd,IDC_PATCHES),&item[4]);
		ListView_SetItem(GetDlgItem(hWnd,IDC_PATCHES),&item[5]);
		ListView_SetItem(GetDlgItem(hWnd,IDC_PATCHES),&item[6]);
	}	
}


// Decryption code for GS2v3-4/GS5 from maxconvert 0.71
unsigned char gsv3_bseeds1[256] = {
0x6E, 0x3C, 0x01, 0x30, 0x45, 0xDA, 0xA1, 0x77,	0x6A, 0x41, 0xFC, 0xC6, 0x00, 0xEA, 0x2F, 0x23,
0xD9, 0x6F, 0x79, 0x39, 0x7E, 0xCC, 0x9A, 0x4E, 0x1E, 0xF4, 0xA7, 0x3D, 0x05, 0x75, 0xD0, 0x36,
0x9E, 0x8D, 0xF8, 0x8B, 0x96, 0x7A, 0xBF, 0x49, 0x62, 0x4F, 0x2A, 0xB8, 0xAF, 0x60, 0x80, 0x0D,
0x99, 0x89, 0xA9, 0xED, 0xB6, 0xD5, 0x7B, 0x54, 0x56, 0x65, 0x5C, 0xB3, 0xD3, 0x11, 0xDF, 0x27,
0x10, 0x71, 0x91, 0x3E, 0x25, 0x52, 0x46, 0x15, 0x3A, 0x31, 0x51, 0x81, 0xC5, 0xB1, 0xBE, 0x43, 
0x95, 0x7C, 0x83, 0x53, 0x38, 0x88, 0x21, 0x4C, 0x9B, 0x7F, 0x90, 0xB9, 0xDB, 0x55, 0x33, 0xB7,
0xAD, 0x61, 0xD8, 0xE1, 0xE4, 0x20, 0xDC, 0x5F, 0xA4, 0x67, 0x26, 0x97, 0x2E, 0xAC, 0x7D, 0x24,
0xBD, 0x22, 0x04, 0x6C, 0xC0, 0x73, 0xE0, 0x32, 0x0C, 0xA3, 0xEE, 0x2B, 0xA0, 0x8A, 0x5B, 0xF3, 
0xA6, 0xD1, 0x68, 0x8E, 0xAE, 0xC7, 0x9C, 0x82, 0xB4, 0xF9, 0xF6, 0xC4, 0x1B, 0xAB, 0x57, 0xCE,
0xEF, 0x69, 0xF7, 0x74, 0xFF, 0xA2, 0x6D, 0xF5, 0xB2, 0x0A, 0x37, 0x1F, 0xEC, 0x06, 0x5D, 0x0F,
0xB0, 0x08, 0xD7, 0xE3,	0x85, 0x58, 0x1A, 0x9F, 0x1D, 0x84, 0xE9, 0xEB, 0x0E, 0x66, 0x64, 0x40,
0x4A, 0x44, 0x35, 0x92, 0x3B, 0x86, 0xF0, 0xF2, 0xAA, 0x47, 0xCB, 0x02,	0xB5, 0xDD, 0xD2, 0x13,
0x16, 0x07, 0xC2, 0xE5, 0x17, 0xA5, 0x5E, 0xCA,	0xD6, 0xC9, 0x87, 0x2D, 0xC1, 0xCD, 0x76, 0x50,
0x1C, 0xE2, 0x8F, 0x29,	0xD4, 0xDE, 0xA8, 0xE7, 0x14, 0xFB, 0xC3, 0xE6, 0xFD, 0x5A, 0x48, 0xCF,
0x98, 0x42, 0x63, 0x4D, 0xBB, 0xE8, 0x70, 0xF1, 0x12, 0x78, 0x0B, 0xFA,	0xBA, 0x18, 0x93, 0x9D,
0x6B, 0x28, 0x2C, 0x09, 0x59, 0xFE, 0xC8, 0x34,	0x03, 0x94, 0x72, 0x8C, 0x3F, 0x4B, 0x19, 0xBC
};

unsigned char gsv3_bseeds2[25] = { 0x12, 0x18, 0x07, 0x0A, 0x17, 0x16, 0x10, 0x00,
0x05, 0x0D, 0x04, 0x02,	0x09, 0x08, 0x0E, 0x0F,	0x13, 0x11, 0x01, 0x15,
0x03, 0x06, 0x0C, 0x0B,	0x14
};

unsigned char gsv3_bseeds3[64] = {0x1D, 0x01, 0x33, 0x0B, 0x23, 0x22, 0x2C, 0x24,
0x0E, 0x2A, 0x2D, 0x0F, 0x2B, 0x20, 0x08, 0x34, 0x17, 0x1B, 0x36, 0x0C,
0x12, 0x1F, 0x35, 0x27, 0x0A, 0x31, 0x11, 0x05, 0x1E, 0x32, 0x14, 0x02,
0x21, 0x03, 0x04, 0x0D, 0x09, 0x16, 0x2E, 0x1A, 0x26, 0x25, 0x2F, 0x07,
0x1C, 0x29, 0x18, 0x30, 0x10, 0x15, 0x13, 0x19, 0x06, 0x38, 0x28, 0x00,
0x37, 0x48, 0x48, 0x7F, 0x45, 0x45, 0x45, 0x7F
};

unsigned char gsv3_bseeds4[64] = { 0x10, 0x01, 0x18, 0x3C, 0x33, 0x29, 0x00, 0x06,
0x07, 0x02, 0x04, 0x13, 0x0B, 0x28, 0x15, 0x08, 0x1F, 0x3E, 0x0C, 0x05,
0x11, 0x1D, 0x1E, 0x27, 0x1A, 0x17, 0x38, 0x23, 0x2D, 0x37, 0x03, 0x2C,
0x2A, 0x1B, 0x22, 0x39, 0x25, 0x14, 0x24, 0x34, 0x20, 0x2F, 0x36, 0x3F,
0x3A, 0x0E, 0x0F, 0x2B, 0x32, 0x31, 0x21, 0x12, 0x26, 0x35, 0x30, 0x3B,
0x09, 0x19, 0x16, 0x0D, 0x0A, 0x2E, 0x3D, 0x1C
};

unsigned short gsv3_hseeds[4] = { 0x6C27, 0x1D38, 0x7FE1, 0x0000};

unsigned char gsv3_bseeds101[256] = {
0x0C, 0x02, 0xBB, 0xF8, 0x72, 0x1C, 0x9D, 0xC1, 0xA1, 0xF3, 0x99, 0xEA, 0x78, 0x2F, 0xAC, 0x9F,
0x40, 0x3D, 0xE8, 0xBF, 0xD8, 0x47, 0xC0, 0xC4, 0xED, 0xFE, 0xA6, 0x8C, 0xD0, 0xA8, 0x18, 0x9B,
0x65, 0x56, 0x71, 0x0F, 0x6F, 0x44, 0x6A, 0x3F, 0xF1, 0xD3, 0x2A, 0x7B, 0xF2, 0xCB, 0x6C, 0x0E,
0x03, 0x49, 0x77, 0x5E, 0xF7, 0xB2, 0x1F, 0x9A, 0x54, 0x13, 0x48, 0xB4, 0x01, 0x1B, 0x43, 0xFC,
0xAF, 0x09, 0xE1, 0x4F, 0xB1, 0x04, 0x46, 0xB9, 0xDE, 0x27, 0xB0, 0xFD, 0x57, 0xE3, 0x17, 0x29,
0xCF, 0x4A, 0x45, 0x53, 0x37, 0x5D, 0x38, 0x8E, 0xA5, 0xF4, 0xDD, 0x7E, 0x3A, 0x9E, 0xC6, 0x67,
0x2D, 0x61, 0x28, 0xE2, 0xAE, 0x39, 0xAD, 0x69, 0x82, 0x91, 0x08, 0xF0, 0x73, 0x96, 0x00, 0x11,
0xE6, 0x41, 0xFA, 0x75, 0x93, 0x1D, 0xCE, 0x07, 0xE9, 0x12, 0x25, 0x36, 0x51, 0x6E, 0x14, 0x59,
0x2E, 0x4B, 0x87, 0x52, 0xA9, 0xA4, 0xB5, 0xCA, 0x55, 0x31, 0x7D, 0x23, 0xFB, 0x21, 0x83, 0xD2,
0x5A, 0x42, 0xB3, 0xEE, 0xF9, 0x50, 0x24, 0x6B, 0xE0, 0x30, 0x16, 0x58, 0x86, 0xEF, 0x20, 0xA7,
0x7C, 0x06, 0x95, 0x79, 0x68, 0xC5, 0x80, 0x1A, 0xD6, 0x32, 0xB8, 0x8D, 0x6D, 0x60, 0x84, 0x2C,
0xA0, 0x4D, 0x98, 0x3B, 0x88, 0xBC, 0x34, 0x5F, 0x2B, 0x5B, 0xEC, 0xE4, 0xFF, 0x70, 0x4E, 0x26,
0x74, 0xCC, 0xC2, 0xDA, 0x8B, 0x4C, 0x0B, 0x85, 0xF6, 0xC9, 0xC7, 0xBA, 0x15, 0xCD, 0x8F, 0xDF,
0x1E, 0x81, 0xBE, 0x3C, 0xD4, 0x35, 0xC8, 0xA2, 0x62, 0x10, 0x05, 0x5C, 0x66, 0xBD, 0xD5, 0x3E,
0x76, 0x63, 0xD1, 0xA3, 0x64, 0xC3, 0xDB, 0xD7, 0xE5, 0xAA, 0x0D, 0xAB, 0x9C, 0x33, 0x7A, 0x90,
0xB6, 0xE7, 0xB7, 0x7F, 0x19, 0x97, 0x8A, 0x92, 0x22, 0x89, 0xEB, 0xD9, 0x0A, 0xDC, 0xF5, 0x94
};


u8 DecryptGS2v3(u32* address, u32* value, u8 ctrl)
{
	u32 command = *address & 0xFE000000;
	u32 tmp = 0, tmp2 = 0, mask = 0, i = 0;
	u8 bmask[4], flag;
	u8 encFlag = (*address >> 25) & 0x7;

	if(ctrl > 0)
	{
		switch(ctrl)
		{
			case 2:
			{
				*address ^= (gsv3_hseeds[1] << 2);
				*value ^= (gsv3_hseeds[1] << 13);
				for(i = 0; i < 0x40; i++)
				{
					if(i < 32)
					{
						flag = (*value >> i) & 1;
					}
					else
					{
						flag = (*address >> (i - 32)) & 1;
					}

					if(flag > 0)
					{
						if(gsv3_bseeds4[i] < 32)
						{
							tmp2 |= (1 << gsv3_bseeds4[i]);
						}
						else
						{
							tmp |= (1 << (gsv3_bseeds4[i] - 32));
						}
					}
				}
				
				*address = tmp ^ (gsv3_hseeds[2] << 3);
				*value = tmp2 ^ gsv3_hseeds[2];
				break;
			}

			case 4:
			{
				tmp = *address ^ (gsv3_hseeds[1] << 3);
				bmask[0] = gsv3_bseeds1[(tmp >> 24) & 0xFF];
				bmask[1] = gsv3_bseeds1[(tmp >> 16) & 0xFF];
				bmask[2] = gsv3_bseeds1[(tmp >> 8) & 0xFF];
				bmask[3] = gsv3_bseeds1[tmp & 0xFF];
				tmp = (bmask[0] << 24) | (bmask[1] << 16) | (bmask[2] << 8) | bmask[3];
				*address = tmp ^ (gsv3_hseeds[2] << 16);

				tmp = *value ^ (gsv3_hseeds[1] << 9);
				bmask[0] = gsv3_bseeds1[(tmp >> 24) & 0xFF];
				bmask[1] = gsv3_bseeds1[(tmp >> 16) & 0xFF];
				bmask[2] = gsv3_bseeds1[(tmp >> 8) & 0xFF];
				bmask[3] = gsv3_bseeds1[tmp & 0xFF];
				tmp = (bmask[0] << 24) | (bmask[1] << 16) | (bmask[2] << 8) | bmask[3];
				*value = tmp ^ (gsv3_hseeds[2] << 5);
				break;
			}
		}
		return 0;
	}

	if(command >= 0x30000000 && command <= 0x6FFFFFFF)
	{
		if(encFlag == 1 || encFlag == 2)
		{
			ctrl = 2;
		}
		else if (encFlag == 3 || encFlag == 4)
		{
			ctrl = 4;
		}
	}

	switch(encFlag)
	{
		case 0:
		{
			break;
		}

		case 1:
		{
			tmp = *address & 0x01FFFFFF;
			tmp = tmp ^ (gsv3_hseeds[1] << 8);
			mask = 0;
			for(i = 0; i < 25; i++)
			{
				mask |= ((tmp & (1 << i)) >> i) << gsv3_bseeds2[i];
			}
			tmp = mask ^ gsv3_hseeds[2];
			tmp |= command;
			*address =  tmp & 0xF1FFFFFF;
			break;
		}
	
		case 2:
		{
			if(encFlag == 2)
			{
				tmp = *address & 0x01FFFFFF;
				*address = tmp ^ (gsv3_hseeds[1] << 1);
				*value ^= (gsv3_hseeds[1] << 16);
				tmp = 0;
				tmp2 = 0;
				for(i = 0; i < 0x39; i++)
				{
					if(i < 32)
					{
						flag = (*value >> i) & 1;
					}
					else
					{
						flag = (*address >> (i - 32)) & 1;
					}

					if(flag > 0)
					{
						if(gsv3_bseeds3[i] < 32)
						{
							tmp2 |= (1 << gsv3_bseeds3[i]);
						}
						else
						{
							tmp |= (1 << (gsv3_bseeds3[i] - 32));
						}
					}
				}
				*address = ((tmp ^ (gsv3_hseeds[2] << 8)) | command) & 0xF1FFFFFF;
				*value = tmp2 ^ gsv3_hseeds[2];
			}
			break;
		}

		case 3:
		{
			tmp = *address ^ (gsv3_hseeds[1] << 8);
			bmask[0] = gsv3_bseeds1[(tmp >> 8) & 0xFF];
			bmask[1] = gsv3_bseeds1[(tmp & 0xFF)];
			tmp = (tmp & 0xFFFF0000) ;
			tmp |= (bmask[0] ^ (bmask[1] << 8)) ;
			tmp ^= (gsv3_hseeds[2] << 4);
			*address = tmp & 0xF1FFFFFF;
			break;
		}

		case 4:
		{
			tmp = *address ^ (gsv3_hseeds[1] << 8);
			bmask[0] = gsv3_bseeds1[(tmp >> 8) & 0xFF];
			bmask[1] = gsv3_bseeds1[(tmp & 0xFF)];
			tmp = (tmp & 0xFFFF0000) ;
			tmp |= (bmask[1] | (bmask[0] << 8)) ;
			tmp ^= (gsv3_hseeds[2] << 4);
			tmp2 = *address ^ (gsv3_hseeds[1] << 3);
			*address = tmp & 0xF1FFFFFF;
			tmp = *value ^ (gsv3_hseeds[1] << 9);
			bmask[0] = gsv3_bseeds1[(tmp >> 24) & 0xFF];
			bmask[1] = gsv3_bseeds1[(tmp >> 16) & 0xFF];
			bmask[2] = gsv3_bseeds1[(tmp >> 8) & 0xFF];
			bmask[3] = gsv3_bseeds1[tmp & 0xFF];
			tmp = (bmask[0] << 24) | (bmask[1] << 16) | (bmask[2] << 8) | bmask[3];
			*value = tmp ^ (gsv3_hseeds[2] << 5);
			break;
		}

		case 5:
		case 6:
		case 7:
		default:
			break;
	}

	return ctrl;

}

struct Cheat
{
	unsigned int address, value;
};

int ParseCheats(char *cur, std::vector<Cheat> &Cheats, HWND hWnd)
{
	// Parse the text and put the cheats to the vector
	while(*cur!=NULL){
		Cheat C;

		// if there is endline advance 2 chars
		if(*cur==0x0d && *(cur+1)==0x0a)
			cur+=2;
		// check if new start is null
		if(*cur==NULL)
			break;

		// get address and value and insert in the vector
		int ret = sscanf(cur, "%X %X", &C.address, &C.value);
		if(ret != 2)
		{
			MessageBox(hWnd,"Code conversion is probably wrong.","Error",0);
			return 0;
		}
		Cheats.insert(Cheats.end(), C);
		cur+=17;
	}
	return 1;
}


// Callback for the GameShark2 v3-4 cheat dialog
BOOL CALLBACK AddGS(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	static HWND hParent;

	switch(uMsg)
	{

		case WM_PAINT:
			return FALSE;
		case WM_INITDIALOG:
			hParent=(HWND)lParam;
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

				case IDC_CONVERT: // Convert the code
				{
					std::vector<Cheat> Cheats;
					char chepa[256];

					// Set ready text to false
					SetWindowText(GetDlgItem(hWnd,IDC_READY), "0");

					// Get the text from the edit control and load it
					GetWindowText(GetDlgItem(hWnd,IDC_ADDR),chepa,256);

					// Parsing the cheats
					ParseCheats(chepa, Cheats, hWnd);

					int control = 0;
					*chepa=0;
					// Decrypt multiline code
					for(unsigned int i=0;i<Cheats.size();i++)
					{
						control = DecryptGS2v3(&Cheats[i].address, &Cheats[i].value, control);

						// Add decrypted code to final string
						char temp[128];
						sprintf(temp, "%.8X %.8X", Cheats[i].address, Cheats[i].value);
						strcat(chepa, temp);
						strcat(chepa, "\x0d\x0a");
					}

					// Set output textbox with decoded lines
					SetWindowText(GetDlgItem(hWnd,IDC_CONVERTEDCODE),chepa);
					SetWindowText(GetDlgItem(hWnd,IDC_READY), "1");
					break;

				}
					
				
				case IDOK: // Add the code
				{
					std::vector<Cheat> Cheats;
					char ready[2], chepa[256];

					// Check if we are ready to add first
					GetWindowText(GetDlgItem(hWnd,IDC_READY),ready,2);
					if(ready[0] == '0')
					{
						MessageBox(hWnd, "First convert the code.", "Error", 0);
						break;
					}

					// Get the text from the edit control and load it
					GetWindowText(GetDlgItem(hWnd,IDC_CONVERTEDCODE),chepa,256);

					// Parsing the cheats
					int ret = ParseCheats(chepa, Cheats, hWnd);

					if(ret == 1)
					{
						// Adding cheats
						for(unsigned int i=0;i<Cheats.size();i++)
							AddPatch(1,1, Cheats[i].address, 3, Cheats[i].value);

						RefreshListBox(hParent);

						// Close the dialog
						EndDialog(hWnd,1);
					}
					else
					{
						MessageBox(hWnd, "Error adding cheats.", "Error", 0);
					}

					break;
				}


				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	
	return TRUE;
}



// Callback for the RAW cheat dialog
BOOL CALLBACK AddRAW(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	static HWND hParent;

	switch(uMsg)
	{

		case WM_PAINT:
			return FALSE;
		case WM_INITDIALOG:
			hParent=(HWND)lParam;
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
					
				case IDOK: // Add the code
				{
					std::vector<Cheat> Cheats;
					char chepa[256];

					// Get the text from the edit control and load it
					GetWindowText(GetDlgItem(hWnd,IDC_ADDR),chepa,256);

					if(strlen(chepa) == 0)
					{
						MessageBox(hWnd, "Add some cheats.", "Error", 0);
						break;
					}

					// Parsing the cheats
					int ret = ParseCheats(chepa, Cheats, hWnd);

					if(ret == 1)
					{
						// Adding cheats
						for(unsigned int i=0;i<Cheats.size();i++)
							AddPatch(1,1, Cheats[i].address, 3, Cheats[i].value);

						RefreshListBox(hParent);

						// Close the dialog
						EndDialog(hWnd,1);
					}
					else
					{
						MessageBox(hWnd, "Error adding cheats.", "Error", 0);
					}

					break;
				}


				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	
	return TRUE;
}


// Reads patch data from Edit/Add patch dialog
int ReadPatch(HWND hWnd, IniPatch &temp)
{
	char chepa[256];
	int ret;

	// Address
	GetWindowText(GetDlgItem(hWnd, IDC_ADDRESS), chepa, 256);
	ret = sscanf(chepa, "0x%X", &temp.addr);
	if(ret != 1)
	{
		MessageBox(hWnd, "Incorrect Address.\nCorrect format is: 0x1234ABCD.", "Error", 0);
		return 0;
	}

	// Data
	GetWindowText(GetDlgItem(hWnd, IDC_DATA), chepa, 256);
	ret = sscanf(chepa, "0x%X", &temp.data);
	if(ret != 1)
	{
		MessageBox(hWnd, "Incorrect Data.\nCorrect format is: 0x1234ABCD.", "Error", 0);
		return 0;
	}

	// Group
	GetWindowText(GetDlgItem(hWnd, IDC_GROUP), chepa, 256);
	ret = sscanf(chepa, "%d", &temp.group);
	if(ret != 1)
	{
		MessageBox(hWnd, "Incorrect group.\n", "Error", 0);
		return 0;
	}

	// CPU
	temp.cpu = (patch_cpu_type)(SendMessage(GetDlgItem(hWnd, IDC_CPU), CB_GETCURSEL, 0, 0) + 1);
	if(temp.cpu < 1)
	{
		MessageBox(hWnd, "CPU is not selected.", "Error", 0);
		return 0;
	}

	// Enabled
	temp.enabled = SendMessage(GetDlgItem(hWnd, IDC_ENABLED), CB_GETCURSEL, 0, 0);
	if(temp.cpu < 0)
	{
		MessageBox(hWnd, "Enabled is not selected.", "Error", 0);
		return 0;
	}

	// PlaceToPatch
	temp.placetopatch = SendMessage(GetDlgItem(hWnd, IDC_PLACETOPATCH), CB_GETCURSEL, 0, 0);
	if(temp.placetopatch < 0)
	{
		MessageBox(hWnd, "PlaceToPatch is not selected.", "Error", 0);
		return 0;
	}

	// Type
	temp.type = (patch_data_type)(SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_GETCURSEL, 0, 0) + 1);
	if(temp.type < 1)
	{
		MessageBox(hWnd, "Type is not selected.", "Error", 0);
		return 0;
	}

	return 1;
}

// Callback for Add Patch
BOOL CALLBACK AddPatchProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	static HWND hParent;

	switch(uMsg)
	{

		case WM_PAINT:
			return FALSE;
		case WM_INITDIALOG:
			hParent=(HWND)lParam;

			// Window title
			SetWindowText(hWnd, "Add Patch");

			// Address
			SetWindowText(GetDlgItem(hWnd, IDC_ADDRESS), "0x00000000");
			
			// Data
			SetWindowText(GetDlgItem(hWnd, IDC_DATA), "0x00000000");
			
			// Group
			SetWindowText(GetDlgItem(hWnd, IDC_GROUP), "0");

			// Cpu ComboBox
			SendMessage(GetDlgItem(hWnd, IDC_CPU), CB_ADDSTRING, 0, (LPARAM)"EE");
			SendMessage(GetDlgItem(hWnd, IDC_CPU), CB_ADDSTRING, 0, (LPARAM)"IOP");
			SendMessage(GetDlgItem(hWnd, IDC_CPU), CB_SETCURSEL, (WPARAM)0, 0);

			// Enabled ComboBox
			SendMessage(GetDlgItem(hWnd, IDC_ENABLED), CB_ADDSTRING, 0, (LPARAM)"No");
			SendMessage(GetDlgItem(hWnd, IDC_ENABLED), CB_ADDSTRING, 0, (LPARAM)"Yes");
			SendMessage(GetDlgItem(hWnd, IDC_ENABLED), CB_SETCURSEL, (WPARAM)0, 0);
			
			// PlaceToPatch ComboBox
			SendMessage(GetDlgItem(hWnd, IDC_PLACETOPATCH), CB_ADDSTRING, 0, (LPARAM)"0");
			SendMessage(GetDlgItem(hWnd, IDC_PLACETOPATCH), CB_ADDSTRING, 0, (LPARAM)"1");
			SendMessage(GetDlgItem(hWnd, IDC_PLACETOPATCH), CB_SETCURSEL, (WPARAM)0, 0);
			
			// Type ComboBox
			SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_ADDSTRING, 0, (LPARAM)"byte");
			SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_ADDSTRING, 0, (LPARAM)"short");
			SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_ADDSTRING, 0, (LPARAM)"word");
			SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_ADDSTRING, 0, (LPARAM)"double");
			SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_SETCURSEL, (WPARAM)0, 0);
			
			// Spin Control
			SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_SETBUDDY, (WPARAM)GetDlgItem(hWnd, IDC_GROUP), 0);
			SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_SETRANGE32, (WPARAM)(0-0x80000000), (LPARAM)0x7FFFFFFF);
			
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
				
				case IDC_SAVE: // Add the code
				{
					IniPatch temp;
					
					if(ReadPatch(hWnd, temp) == 0)
						break;

					// Add new patch, refresh and exit
					memcpy((void *)&patch[patchnumber], (void *)&temp, sizeof(IniPatch));
					patchnumber++;
					RefreshListBox(hParent);
					EndDialog(hWnd,1);
					break;
				}


				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	
	return TRUE;
}


// Callback for Edit Patch
BOOL CALLBACK EditPatch(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	static HWND hParent;
	static int selected;
	char chepa[256];

	switch(uMsg)
	{

		case WM_PAINT:
			return FALSE;
		case WM_INITDIALOG:
		{	
			hParent=(HWND)lParam;
			
			// Find the selected patch
			UINT mresults = ListView_GetItemCount(GetDlgItem(hParent, IDC_PATCHES));

			unsigned int i;
			for(i=0;i<mresults;i++)
			{
				UINT state = ListView_GetItemState(GetDlgItem(hParent,IDC_PATCHES), i, LVIS_SELECTED);
				if(state==LVIS_SELECTED)
				{
					selected = i;
					// Fill data on gui controls from the patch

					// Address
					sprintf(chepa, "0x%.8X", patch[i].addr);
					SetWindowText(GetDlgItem(hWnd, IDC_ADDRESS), chepa);
					
					// Data
					sprintf(chepa, "0x%.8X", patch[i].data);
					SetWindowText(GetDlgItem(hWnd, IDC_DATA), chepa);
					
					// Group
					sprintf(chepa, "%d", patch[i].group);
					SetWindowText(GetDlgItem(hWnd, IDC_GROUP), chepa);

					// Cpu ComboBox
					SendMessage(GetDlgItem(hWnd, IDC_CPU), CB_ADDSTRING, 0, (LPARAM)"EE");
					SendMessage(GetDlgItem(hWnd, IDC_CPU), CB_ADDSTRING, 0, (LPARAM)"IOP");
					if(patch[i].cpu == 1)
						SendMessage(GetDlgItem(hWnd, IDC_CPU), CB_SETCURSEL, (WPARAM)0, 0);
					else
						SendMessage(GetDlgItem(hWnd, IDC_CPU), CB_SETCURSEL, (WPARAM)1, 0);

					// Enabled ComboBox
					SendMessage(GetDlgItem(hWnd, IDC_ENABLED), CB_ADDSTRING, 0, (LPARAM)"No");
					SendMessage(GetDlgItem(hWnd, IDC_ENABLED), CB_ADDSTRING, 0, (LPARAM)"Yes");
					SendMessage(GetDlgItem(hWnd, IDC_ENABLED), CB_SETCURSEL, (WPARAM)patch[i].enabled, 0);
					
					// PlaceToPatch ComboBox
					SendMessage(GetDlgItem(hWnd, IDC_PLACETOPATCH), CB_ADDSTRING, 0, (LPARAM)"0");
					SendMessage(GetDlgItem(hWnd, IDC_PLACETOPATCH), CB_ADDSTRING, 0, (LPARAM)"1");
					SendMessage(GetDlgItem(hWnd, IDC_PLACETOPATCH), CB_SETCURSEL, (WPARAM)patch[i].placetopatch, 0);
					
					// Type ComboBox
					SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_ADDSTRING, 0, (LPARAM)"byte");
					SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_ADDSTRING, 0, (LPARAM)"short");
					SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_ADDSTRING, 0, (LPARAM)"word");
					SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_ADDSTRING, 0, (LPARAM)"double");
					SendMessage(GetDlgItem(hWnd, IDC_TYPE), CB_SETCURSEL, (WPARAM)patch[i].type-1, 0);
					
					// Spin Control
					SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_SETBUDDY, (WPARAM)GetDlgItem(hWnd, IDC_GROUP), 0);
					SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_SETRANGE32, (WPARAM)(0-0x80000000), (LPARAM)0x7FFFFFFF);
					break;
				}
			}

			// If nothing is selected close the window
			if(i == mresults)
				EndDialog(hWnd,1);
			
			break;
		}
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDCANCEL:
					EndDialog(hWnd,1);
					break;
				
				case IDC_SAVE: // Add the code
				{
					IniPatch temp;
					
					if(ReadPatch(hWnd, temp) == 0)
						break;

					// Copying temporal patch to selected patch, refreshing parent's listbox and exit
					memcpy((void *)&patch[selected], (void *)&temp, sizeof(IniPatch));
					RefreshListBox(hParent);
					EndDialog(hWnd,1);
					break;
				}


				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	
	return TRUE;
}


BOOL CALLBACK pnachWriterProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	static HWND hParent;
	char chepa[256];

	switch(uMsg)
	{

		case WM_PAINT:
			return FALSE;
		case WM_INITDIALOG:
			hParent=(HWND)lParam;

			// CRC
			sprintf(chepa, "%.8x", ElfCRC);
			SetWindowText(GetDlgItem(hWnd, IDC_CRC), chepa);

			// try to load value
			if(ElfCRC != 0){}
			
			// Comment
			SetWindowText(GetDlgItem(hWnd, IDC_COMMENT), "Insert comment.");
			
			// Gametitle
			SetWindowText(GetDlgItem(hWnd, IDC_GAMETITLE), "Insert Game Title.");

			// ZeroGS
			SetWindowText(GetDlgItem(hWnd, IDC_ZEROGS), "00000000");

			// Round mode 1
			SendMessage(GetDlgItem(hWnd, IDC_ROUND1), CB_ADDSTRING, 0, (LPARAM)"NEAR");
			SendMessage(GetDlgItem(hWnd, IDC_ROUND1), CB_ADDSTRING, 0, (LPARAM)"DOWN");
			SendMessage(GetDlgItem(hWnd, IDC_ROUND1), CB_ADDSTRING, 0, (LPARAM)"UP");
			SendMessage(GetDlgItem(hWnd, IDC_ROUND1), CB_ADDSTRING, 0, (LPARAM)"CHOP");
			SendMessage(GetDlgItem(hWnd, IDC_ROUND1), CB_SETCURSEL, (WPARAM)0, 0);

			// Round mode 2
			SendMessage(GetDlgItem(hWnd, IDC_ROUND2), CB_ADDSTRING, 0, (LPARAM)"NEAR");
			SendMessage(GetDlgItem(hWnd, IDC_ROUND2), CB_ADDSTRING, 0, (LPARAM)"DOWN");
			SendMessage(GetDlgItem(hWnd, IDC_ROUND2), CB_ADDSTRING, 0, (LPARAM)"UP");
			SendMessage(GetDlgItem(hWnd, IDC_ROUND2), CB_ADDSTRING, 0, (LPARAM)"CHOP");
			SendMessage(GetDlgItem(hWnd, IDC_ROUND2), CB_SETCURSEL, (WPARAM)3, 0);



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
				
				case IDC_SAVE: // Add the code
				{
					char filename[1024];

					// Obtain the crc, build the path and open the file
					GetWindowText(GetDlgItem(hWnd,IDC_CRC),chepa,256);
					strcpy(filename, "./patches/");
					strcat(filename, chepa);
					strcat(filename, ".pnach");
					FILE *fp = fopen(filename, "w");

					// comment
					GetWindowText(GetDlgItem(hWnd,IDC_COMMENT),chepa,256);
					fprintf(fp, "comment=%s\n", chepa);

					// gametitle
					GetWindowText(GetDlgItem(hWnd,IDC_GAMETITLE),chepa,256);
					fprintf(fp, "gametitle=%s\n", chepa);

					// zerogs
					GetWindowText(GetDlgItem(hWnd,IDC_ZEROGS),chepa,256);
					int zgs;
					sscanf(chepa, "%x", &zgs);
					if(zgs != 0)
					{
						fprintf(fp, "zerogs=%s\n", chepa);
					}

					// round mode
					int round1, round2;
					round1 = SendMessage(GetDlgItem(hWnd, IDC_ROUND1), CB_GETCURSEL, 0, 0);
					round2 = SendMessage(GetDlgItem(hWnd, IDC_ROUND2), CB_GETCURSEL, 0, 0);
					if(round1 != 0 || round2 !=3) // if not default roundings write it
					{
						char r1[10], r2[10];
						GetWindowText(GetDlgItem(hWnd, IDC_ROUND1), r1, 10);
						GetWindowText(GetDlgItem(hWnd, IDC_ROUND2), r2, 10);
						fprintf(fp, "roundmode=%s,%s\n", r1, r2);
					}

					// fastmemory
					UINT s = IsDlgButtonChecked(hWnd, IDC_FASTMEMORY);
					if(s == 1)
					{
						fprintf(fp, "fastmemory\n");
					}

					// path3hack
					s = IsDlgButtonChecked(hWnd, IDC_PATH3HACK);
					if(s == 1)
					{
						fprintf(fp, "path3hack\n");
					}

					// vunanmode
					s = IsDlgButtonChecked(hWnd, IDC_VUNANMODE);
					if(s == 1)
					{
						fprintf(fp, "vunanmode\n");
					}
					
					// write patches
					for(int i=0;i<patchnumber;i++)
					{
						char cpucore[10], type[10];
						switch(patch[i].cpu)
						{
							case 1:
								strcpy(cpucore, "EE");
								break;
							case 2:
								strcpy(cpucore, "IOP");
								break;
						}

						switch(patch[i].type)
						{
							case 1:
								strcpy(type, "byte");
								break;
							case 2:
								strcpy(type, "short");
								break;
							case 3:
								strcpy(type, "word");
								break;
							case 4:
								strcpy(type, "double");
								break;
						}
						//patch=placetopatch,cpucore,address,type,data 
						fprintf(fp, "patch=%d,%s,%.8x,%s,%.8x\n", patch[i].placetopatch, cpucore, patch[i].addr, type, patch[i].data);
					}

					fclose(fp);

					EndDialog(hWnd,1);
					break;
				}


				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	
	return TRUE;
}



BOOL CALLBACK BrowserProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;

	switch(uMsg)
	{

		case WM_PAINT:
			return FALSE;

		case WM_INITDIALOG:

			CreateListBox(hWnd);
			RefreshListBox(hWnd);

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
					EndDialog(hWnd,1);
					break;

				case IDC_ENABLEBUTTON:
				{
					UINT mresults = ListView_GetItemCount(GetDlgItem(hWnd, IDC_PATCHES));
					for(unsigned int i=0;i<mresults;i++)
					{
						UINT state = ListView_GetItemState(GetDlgItem(hWnd,IDC_PATCHES), i, LVIS_SELECTED);
						if(state==LVIS_SELECTED)
						{
							patch[i].enabled = !patch[i].enabled;
						}
					}
					RefreshListBox(hWnd);
					break;
				}

				case IDC_ADDGS:
				{
					INT_PTR retret=DialogBoxParam(0,MAKEINTRESOURCE(IDD_ADDGS),hWnd,(DLGPROC)AddGS,(LPARAM)hWnd);
					break;
				}

				case IDC_EDITPATCH:
				{
					INT_PTR retret=DialogBoxParam(0,MAKEINTRESOURCE(IDD_EDITPATCH),hWnd,(DLGPROC)EditPatch,(LPARAM)hWnd);
					break;
				}

				case IDC_ADDPATCH:
				{
					INT_PTR retret=DialogBoxParam(0,MAKEINTRESOURCE(IDD_EDITPATCH),hWnd,(DLGPROC)AddPatchProc,(LPARAM)hWnd);
					break;
				}

				case IDC_ADDRAW:
				{
					INT_PTR retret=DialogBoxParam(0,MAKEINTRESOURCE(IDD_ADDRAW),hWnd,(DLGPROC)AddRAW,(LPARAM)hWnd);
					break;
				}

				case IDC_PNACHWRITER:
				{
					INT_PTR retret=DialogBoxParam(0,MAKEINTRESOURCE(IDD_PNACHWRITER),hWnd,(DLGPROC)pnachWriterProc,(LPARAM)hWnd);
					break;
				}

				case IDC_SKIPMPEG:
				{
					u8 *p = PS2MEM_BASE;
					u8 *d = p + Ps2MemSize::Base;
					d -= 16;
					u32 *u;

					while(p<d)
					{
						u = (u32 *)p;
						//if( (u[0] == 0x4000838c) && (u[1] == 0x0800e003) && (u[2] == 0x0000628c) )
						if( (u[0] == 0x8c830040) && (u[1] == 0x03e00008) && (u[2] == 0x8c620000) )
						{
							AddPatch(1,1, (int)(p+8-PS2MEM_BASE), 3, 0x24020001);
							MessageBox(hWnd, "Patch Found.", "Patch", 0);
							RefreshListBox(hWnd);
							return FALSE;
						}

						p++;
					}
					MessageBox(hWnd, "Patch not found.", "Patch", 0);
					break;
				}

				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

void ShowCheats(HINSTANCE hInstance, HWND hParent)
{
	INT_PTR ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_CHEATS),hParent,(DLGPROC)BrowserProc,1);
}
