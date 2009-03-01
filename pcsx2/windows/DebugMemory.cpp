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

#include "Win32.h"
#include "Common.h"

unsigned long memory_addr;
BOOL mem_inupdate = FALSE;
HWND memoryhWnd,hWnd_memscroll,hWnd_memorydump;
unsigned long memory_patch;
unsigned long data_patch;

///MEMORY DUMP 
unsigned char Debug_Read8(unsigned long addr)//just for anycase..
{
#ifdef _WIN32
	__try
	{
#endif
      u8 val8;
      memRead8(addr, &val8);
		return val8;
#ifdef _WIN32
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0;
	}
#endif
}


void RefreshMemory(void)
{
    int x, y;
    unsigned long addr;
    unsigned char b;
	
    char buf[128], text[32], temp[8];



    addr = memory_addr;

    if (!mem_inupdate)
    {
        sprintf(buf, "%08X", addr);
        SetDlgItemText(memoryhWnd, IDC_MEMORY_ADDR, buf);
    }

    SendMessage(hWnd_memorydump, LB_RESETCONTENT, 0, 0);

    for (y = 0; y < 21; y++)
    {
		memzero_obj(text);
        sprintf(buf, "%08X:  ", addr);

        for (x = 0; x < 16; x++)
        {
			b = Debug_Read8(addr++);

                sprintf(temp, "%02X ", b);
				strcat(buf, temp);

	            if (b < 32 || b > 127) b = 32;
				sprintf(temp, "%c", b);
				strcat(text, temp);


            if (x == 7) strcat(buf, " ");
        }

        strcat(buf, " ");
		strcat(buf, text);

        SendMessage(hWnd_memorydump, LB_ADDSTRING, 0, (LPARAM)buf);
    }
}


BOOL APIENTRY DumpMemProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char start[16], end[16], fname[128], buf[128];
    u32 start_pc, end_pc, addr;
	u8 data;

	FILE *fp;

    switch (message)
    {
        case WM_INITDIALOG:
            sprintf(buf, "%08X", cpuRegs.pc);
            SetDlgItemText(hDlg, IDC_DUMPMEM_START, buf);
            SetDlgItemText(hDlg, IDC_DUMPMEM_END,   buf);
			SetDlgItemText(hDlg, IDC_DUMPMEM_FNAME, "dump.raw");
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, IDC_DUMPMEM_START, start, 9);
                start[8] = 0;
                sscanf(start, "%x", &start_pc);
                start_pc &= 0xFFFFFFFC;

                GetDlgItemText(hDlg, IDC_DUMPMEM_END, end, 9);
                end[8] = 0;
                sscanf(end, "%x", &end_pc);
                end_pc &= 0xFFFFFFFC;

				GetDlgItemText(hDlg, IDC_DUMPMEM_FNAME, fname, 128);
				fp = fopen(fname, "wb");
				if (fp == NULL)
				{
					Msgbox::Alert("Can't open file '%s' for writing!", params fname);
				}
				else
				{
					for (addr = start_pc; addr < end_pc; addr ++) {
						memRead8( addr, &data );
						fwrite(&data, 1, 1, fp);
					}

					fclose(fp);
				}

                EndDialog(hDlg, TRUE);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
    }

    return FALSE;
}



BOOL APIENTRY MemoryProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    char buf[16];
    switch (message)
    {
        case WM_INITDIALOG:
            memory_addr = cpuRegs.pc;
            sprintf(buf, "%08X", memory_addr);
            SetDlgItemText(hDlg, IDC_MEMORY_ADDR, buf);
			memory_patch= 0;
            sprintf(buf, "%08X", memory_patch);
            SetDlgItemText(hDlg, IDC_ADDRESS_PATCH, buf);
			data_patch=0;
            sprintf(buf, "%08X", data_patch);
            SetDlgItemText(hDlg, IDC_DATA_PATCH, buf);
            hWnd_memorydump = GetDlgItem(hDlg, IDC_MEMORY_DUMP);
            hWnd_memscroll = GetDlgItem(hDlg, IDC_MEM_SCROLL);

           
          
            SendMessage(hWnd_memorydump, LB_INITSTORAGE, 11, 1280);
            SendMessage(hWnd_memscroll, SBM_SETRANGE, 0, MAXLONG);
            SendMessage(hWnd_memscroll, SBM_SETPOS, MAXLONG / 2, TRUE);

            RefreshMemory();
            return TRUE;
         case WM_VSCROLL:
            switch ((int) LOWORD(wParam))
            {
                case SB_LINEDOWN: memory_addr += 0x00000010; RefreshMemory(); break;
                case SB_LINEUP:   memory_addr -= 0x00000010; RefreshMemory(); break;
                case SB_PAGEDOWN: memory_addr += 0x00000150; RefreshMemory(); break;
                case SB_PAGEUP:   memory_addr -= 0x00000150; RefreshMemory(); break;
            }

            return TRUE;

         case WM_CLOSE:
			EndDialog(hDlg, TRUE );
            return TRUE;


        case WM_COMMAND:
            if (HIWORD(wParam) == EN_UPDATE)
            {
                mem_inupdate = TRUE;
                GetDlgItemText(hDlg, IDC_MEMORY_ADDR, buf, 9);
                buf[8] = 0;

                sscanf(buf, "%x", &memory_addr);
                RefreshMemory();
                mem_inupdate = FALSE;
                return TRUE;
            }

            switch (LOWORD(wParam)) {
				case IDC_PATCH:
		            GetDlgItemText(hDlg, IDC_ADDRESS_PATCH, buf, 9);//32bit address
					buf[8] = 0;
					sscanf(buf, "%x", &memory_patch);
					GetDlgItemText(hDlg, IDC_DATA_PATCH, buf, 9);//32 bit data only for far
					buf[8] = 0;
					sscanf(buf, "%x", &data_patch);
					memWrite32( memory_patch, data_patch );
					sprintf(buf, "%08X", memory_patch);
					SetDlgItemText(hDlg, IDC_MEMORY_ADDR, buf);
					RefreshMemory();
					return TRUE;

				case IDC_DUMPRAW:
                    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_DUMPMEM), hDlg, (DLGPROC)DumpMemProc);

					return TRUE;

                case IDC_MEMORY_CLOSE:
					EndDialog(hDlg, TRUE );                  
                    return TRUE;
            }
            break;
    }

    return FALSE;
}
