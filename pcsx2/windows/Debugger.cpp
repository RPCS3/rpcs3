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

#include "win32.h"

#include "resource.h"
#include "R5900OpcodeTables.h"
#include "Debugger.h"
#include "Common.h"
#include "IopMem.h"
#include "R3000A.h"

#ifdef _MSC_VER
#pragma warning(disable:4996) //ignore the stricmp deprecated warning
#endif

void RefreshIOPDebugger(void);
extern int ISR3000A;//for disasm
HWND hWnd_debugdisasm, hWnd_debugscroll,hWnd_IOP_debugdisasm, hWnd_IOP_debugscroll;
unsigned long DebuggerPC = 0;
HWND hRegDlg;//for debug registers..
HWND debughWnd;
unsigned long DebuggerIOPPC=0;
HWND hIOPDlg;//IOP debugger

breakpoints bkpt_regv[NUM_BREAKPOINTS];


void RefreshDebugAll()//refresh disasm and register window
{
  RefreshDebugger();
  RefreshIOPDebugger();
  UpdateRegs();
}

void MakeDebugOpcode(void)
{
	memRead32( opcode_addr, &cpuRegs.code );
}

void MakeIOPDebugOpcode(void)
{
	psxRegs.code = iopMemRead32( opcode_addr );
}

BOOL HasBreakpoint()
{
	int t;

	for (t = 0; t < NUM_BREAKPOINTS; t++)
	{
		switch (bkpt_regv[t].type) {
			case 1: // exec
				if (cpuRegs.pc == bkpt_regv[t].value) return TRUE;
				break;

			case 2: // count
				if ((cpuRegs.cycle - 10) <= bkpt_regv[t].value &&
					(cpuRegs.cycle + 10) >= bkpt_regv[t].value) return TRUE;
				break;
		}
	}
	return FALSE;

}
BOOL APIENTRY JumpProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buf[16];
    unsigned long temp;

    switch (message)
    {
        case WM_INITDIALOG:
            sprintf(buf, "%08X", cpuRegs.pc);
            SetDlgItemText(hDlg, IDC_JUMP_PC, buf);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, IDC_JUMP_PC, buf, 9);

                buf[8] = 0;
                sscanf(buf, "%x", &temp);

                temp      &= 0xFFFFFFFC;
                DebuggerPC = temp - 0x00000038;

                EndDialog(hDlg, TRUE);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
    }

    return FALSE;
}

extern void EEDumpRegs(FILE * fp);
extern void IOPDumpRegs(FILE * fp);
BOOL APIENTRY DumpProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char start[16], end[16], fname[128], tmp[128];
    unsigned long start_pc, end_pc, temp;

	FILE *fp;

    switch (message)
    {
        case WM_INITDIALOG:
            sprintf(tmp, "%08X", cpuRegs.pc);
            SetDlgItemText(hDlg, IDC_DUMP_START, tmp);
            SetDlgItemText(hDlg, IDC_DUMP_END,   tmp);
			SetDlgItemText(hDlg, IDC_DUMP_FNAME, "EEdisasm.txt");

			sprintf(tmp, "%08X", psxRegs.pc);
            SetDlgItemText(hDlg, IDC_DUMP_STARTIOP, tmp);
            SetDlgItemText(hDlg, IDC_DUMP_ENDIOP,   tmp);
			SetDlgItemText(hDlg, IDC_DUMP_FNAMEIOP, "IOPdisasm.txt");
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, IDC_DUMP_START, start, 9);
                start[8] = 0;
                sscanf(start, "%x", &start_pc);
                start_pc &= 0xFFFFFFFC;

                GetDlgItemText(hDlg, IDC_DUMP_END, end, 9);
                end[8] = 0;
                sscanf(end, "%x", &end_pc);
                end_pc &= 0xFFFFFFFC;

				GetDlgItemText(hDlg, IDC_DUMP_FNAME, fname, 128);
				fp = fopen(fname, "wt");
				if (fp == NULL)
				{
					//MessageBox(MainhWnd, "Can't open file for writing!", NULL, MB_OK);
				}
				else
				{
					std::string output;

					fprintf(fp,"----------------------------------\n");
					fprintf(fp,"EE DISASM TEXT DOCUMENT BY PCSX2  \n");
					fprintf(fp,"----------------------------------\n");
					for (temp = start_pc; temp <= end_pc; temp += 4)
					{
						opcode_addr=temp;
						MakeDebugOpcode();

						output.assign( HasBreakpoint() ? "*" : "" );
						R5900::GetCurrentInstruction().disasm( output );

						fprintf(fp, "%08X %08X: %s\n", temp, cpuRegs.code, output.c_str());
					}


					fprintf(fp,"\n\n\n----------------------------------\n");
					fprintf(fp,"EE REGISTER DISASM TEXT DOCUMENT BY PCSX2  \n");
					fprintf(fp,"----------------------------------\n");
					EEDumpRegs(fp);
					fclose(fp);
				}



				GetDlgItemText(hDlg, IDC_DUMP_STARTIOP, start, 9);
                start[8] = 0;
                sscanf(start, "%x", &start_pc);
                start_pc &= 0xFFFFFFFC;

                GetDlgItemText(hDlg, IDC_DUMP_ENDIOP, end, 9);
                end[8] = 0;
                sscanf(end, "%x", &end_pc);
                end_pc &= 0xFFFFFFFC;

				GetDlgItemText(hDlg, IDC_DUMP_FNAMEIOP, fname, 128);
				fp = fopen(fname, "wt");
				if (fp == NULL)
				{
					//MessageBox(MainhWnd, "Can't open file for writing!", NULL, MB_OK);
				}
				else
				{
					fprintf(fp,"----------------------------------\n");
					fprintf(fp,"IOP DISASM TEXT DOCUMENT BY PCSX2 \n");
					fprintf(fp,"----------------------------------\n");
					for (temp = start_pc; temp <= end_pc; temp += 4)
					{
						opcode_addr=temp;
						MakeIOPDebugOpcode();											
						R3000A::IOP_DEBUG_BSC[(psxRegs.code) >> 26](tmp);
						fprintf(fp, "%08X %08X: %s\n", temp, psxRegs.code, tmp);
					}

					fprintf(fp,"\n\n\n----------------------------------\n");
					fprintf(fp,"IOP REGISTER DISASM TEXT DOCUMENT BY PCSX2  \n");
					fprintf(fp,"----------------------------------\n");
					IOPDumpRegs(fp);
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

BOOL APIENTRY BpexecProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buf[16];

    switch (message)
    {
        case WM_INITDIALOG:
            sprintf(buf, "%08X", bkpt_regv[0].value);
            SetDlgItemText(hDlg, IDC_EXECBP, buf);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, IDC_EXECBP, buf, 9);

                buf[8] = 0;
                sscanf(buf, "%x", &bkpt_regv[0].value);
				bkpt_regv[0].type = 1;

                EndDialog(hDlg, TRUE);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
    }

    return FALSE;
}

BOOL APIENTRY BpcntProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char buf[16];

    switch (message)
    {
        case WM_INITDIALOG:
            sprintf(buf, "%08X", bkpt_regv[1].value);
            SetDlgItemText(hDlg, IDC_CNTBP, buf);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                GetDlgItemText(hDlg, IDC_CNTBP, buf, 9);

                buf[8] = 0;
                sscanf(buf, "%x", &bkpt_regv[1].value);
				bkpt_regv[1].type = 2;

				EndDialog(hDlg, TRUE);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
            }
            return TRUE;
    }

    return FALSE;
}
HINSTANCE m2_hInst;
HWND m2_hWnd;
HWND hIopDlg;

LRESULT CALLBACK IOP_DISASM(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
            hWnd_IOP_debugdisasm = GetDlgItem(hDlg, IDC_DEBUG_DISASM_IOP);
            hWnd_IOP_debugscroll = GetDlgItem(hDlg, IDC_DEBUG_SCROLL_IOP);

            SendMessage(hWnd_IOP_debugdisasm, LB_INITSTORAGE, 29, 1131);
            SendMessage(hWnd_IOP_debugscroll, SBM_SETRANGE, 0, MAXLONG);
            SendMessage(hWnd_IOP_debugscroll, SBM_SETPOS, MAXLONG / 2, TRUE);
            RefreshIOPDebugger();
	    return (TRUE);
     case WM_VSCROLL:
		   switch ((int) LOWORD(wParam))
           {
              case SB_LINEDOWN: DebuggerIOPPC += 0x00000004; RefreshIOPDebugger(); break;
              case SB_LINEUP:   DebuggerIOPPC -= 0x00000004; RefreshIOPDebugger(); break;
              case SB_PAGEDOWN: DebuggerIOPPC += 0x00000029; RefreshIOPDebugger(); break;
              case SB_PAGEUP:   DebuggerIOPPC -= 0x00000029; RefreshIOPDebugger(); break;
			}
            return TRUE;
		break;
	case WM_COMMAND:
 
		switch(LOWORD(wParam))
		{
		case (IDOK || IDCANCEL):
			EndDialog(hDlg,TRUE);
			return(TRUE);
			break;
		}
		break;
	}

	return(FALSE);
}
int CreatePropertySheet2(HWND hwndOwner)
{
	PROPSHEETPAGE psp[1];
	PROPSHEETHEADER psh;

	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_USETITLE;
	psp[0].hInstance = m2_hInst;
	psp[0].pszTemplate = MAKEINTRESOURCE( IDD_IOP_DEBUG);
	psp[0].pszIcon = NULL;
	psp[0].pfnDlgProc =(DLGPROC)IOP_DISASM;
	psp[0].pszTitle = "Iop Disasm";
	psp[0].lParam = 0;

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS;
	psh.hwndParent =hwndOwner;
	psh.hInstance = m2_hInst;
	psh.pszIcon = NULL;
	psh.pszCaption = (LPSTR) "IOP Debugger";
	psh.nStartPage = 0;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.ppsp = (LPCPROPSHEETPAGE) &psp;
   
	return (PropertySheet(&psh)); 
}

/** non-zero if the dialog is currently executing instructions. */
static int isRunning = 0;

/** non-zero if the user has requested a break in the execution of instructions. */
static int breakRequested = 0;

static
void EnterRunningState(HWND hDlg)
{
	isRunning = 1;
	breakRequested = 0;
	EnableWindow(GetDlgItem(hDlg, IDC_DEBUG_STEP_OVER), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_DEBUG_STEP_EE), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_DEBUG_STEP), FALSE);
	EnableWindow(GetDlgItem(hDlg, IDC_DEBUG_SKIP), FALSE);
	OpenPlugins(NULL);
}

static
void EnterHaltedState(HWND hDlg)
{
	isRunning = 0;
	breakRequested = 0;
	EnableWindow(GetDlgItem(hDlg, IDC_DEBUG_STEP_OVER), TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_DEBUG_STEP_EE), TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_DEBUG_STEP), TRUE);
	EnableWindow(GetDlgItem(hDlg, IDC_DEBUG_SKIP), TRUE);
}

BOOL APIENTRY DebuggerProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    FARPROC jmpproc, dumpproc;
    FARPROC bpexecproc, bpcntproc;
	u32 oldpc = 0;

    switch (message)
    {
        case WM_INITDIALOG:
			ShowCursor(TRUE);
			isRunning = 0;
			breakRequested = 0;

			SetWindowText(hDlg, "R5900 Debugger");
            debughWnd=hDlg;
            DebuggerPC = 0;
            // Clear all breakpoints.
			memzero_obj(bkpt_regv);

			hWnd_debugdisasm = GetDlgItem(hDlg, IDC_DEBUG_DISASM);
            hWnd_debugscroll = GetDlgItem(hDlg, IDC_DEBUG_SCROLL);

            SendMessage(hWnd_debugdisasm, LB_INITSTORAGE, 29, 1131);
            SendMessage(hWnd_debugscroll, SBM_SETRANGE, 0, MAXLONG);
            SendMessage(hWnd_debugscroll, SBM_SETPOS, MAXLONG / 2, TRUE);

            hRegDlg = (HWND)CreatePropertySheet(hDlg);
			hIopDlg = (HWND)CreatePropertySheet2(hDlg);
	        UpdateRegs();   
            SetWindowPos(hRegDlg, NULL, 525, 0, 600, 515,0 );
			SetWindowPos(hIopDlg, NULL, 0  ,515,600,230,0);
            RefreshDebugger();
			RefreshIOPDebugger();
            return TRUE;

        case WM_VSCROLL:
				
             switch ((int) LOWORD(wParam))
             {
				 case SB_LINEDOWN: DebuggerPC += 0x00000004; RefreshDebugAll(); break;
				 case SB_LINEUP:   DebuggerPC -= 0x00000004; RefreshDebugAll(); break;
				 case SB_PAGEDOWN: DebuggerPC += 0x00000074; RefreshDebugAll(); break;
				 case SB_PAGEUP:   DebuggerPC -= 0x00000074; RefreshDebugAll(); break;
			 }
            return TRUE;
	

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_DEBUG_STEP:
					oldpc = psxRegs.pc;
					EnterRunningState(hDlg);
                    Cpu->Step();
					while(oldpc == psxRegs.pc) Cpu->Step();
                    DebuggerPC = 0;
					DebuggerIOPPC=0;
					EnterHaltedState(hDlg);
                    RefreshDebugAll();
                    return TRUE;

                case IDC_DEBUG_STEP_EE:
					EnterRunningState(hDlg);
                    Cpu->Step();
					EnterHaltedState(hDlg);
                    DebuggerPC = 0;
					DebuggerIOPPC=0;
                    RefreshDebugAll();
                    return TRUE;

				case IDC_DEBUG_STEP_OVER:
					/* Step over a subroutine call. */
					/* Note that this may take some time to execute and
					 * because Cpu->Step() pumps the message loop, we need
					 * to guard against re-entry. We do that by disabling the step buttons.
					 */
					EnterRunningState(hDlg);

					memRead32(cpuRegs.pc, &cpuRegs.code);

					{
						u32 target_pc = 0;
						if (3 == (cpuRegs.code >> 26)){
							/* it's a JAL instruction. */
							target_pc = cpuRegs.pc + 8;
						} else if (0x0c == (cpuRegs.code & 0xFF)){
							/* it's a syscall. */
							target_pc = cpuRegs.pc + 4;
						}
						if (0 != target_pc){
							while(target_pc != cpuRegs.pc && !breakRequested) { 		
								Cpu->Step();
							}
						} else {
							Cpu->Step();
						}
					}

					DebuggerPC = 0;
					DebuggerIOPPC=0;
					EnterHaltedState(hDlg);
                    RefreshDebugAll();

                    return TRUE;
					
                case IDC_DEBUG_SKIP:
					cpuRegs.pc+= 4;
                    DebuggerPC = 0;
                    RefreshDebugAll();
                    return TRUE;

				case IDC_DEBUG_BREAK:
					breakRequested = 1;
					return TRUE;

                case IDC_DEBUG_GO:
					EnterRunningState(hDlg);
					for (;;) {
						if (breakRequested || HasBreakpoint()) {
							Cpu->Step();
							break;
						}
						Cpu->Step();
					}
                    DebuggerPC = 0;
					DebuggerIOPPC=0;
					EnterHaltedState(hDlg);
                    RefreshDebugAll();
                    return TRUE;

                case IDC_DEBUG_RUN_TO_CURSOR:
					{
					/* Run to the cursor without checking for breakpoints. */
					int sel = SendMessage(hWnd_debugdisasm, LB_GETCURSEL,0,0);  
					if (sel != LB_ERR){
						const u32 target_pc = DebuggerPC + sel*4;
						EnterRunningState(hDlg);
						while(target_pc != cpuRegs.pc && !breakRequested) {
							Cpu->Step();
						}
						DebuggerPC = 0;
						DebuggerIOPPC=0;
						EnterHaltedState(hDlg);
						RefreshDebugAll();
					}
					return TRUE;					
					}

                case IDC_DEBUG_STEP_TO_CURSOR:
					{
					int sel = SendMessage(hWnd_debugdisasm, LB_GETCURSEL,0,0);  
					if (sel != LB_ERR){
						const u32 target_pc = DebuggerPC + sel*4;
						EnterRunningState(hDlg);
						while(target_pc != cpuRegs.pc && !breakRequested) {
							if (HasBreakpoint()) {
								Cpu->Step();
								break;
							}
							Cpu->Step();
						}
						DebuggerPC = 0;
						DebuggerIOPPC=0;
						EnterHaltedState(hDlg);
						RefreshDebugAll();
					}
					return TRUE;					
					}

				#ifdef PCSX2_DEVBUILD
                case IDC_DEBUG_LOG:
					if( varLog )
						varLog &= ~0x80000000;
					else
						varLog |= 0x80000000;
                    return TRUE;
				#endif

                case IDC_DEBUG_RESETTOPC:
                    DebuggerPC = 0;
					DebuggerIOPPC=0;
                    RefreshDebugAll();
                    return TRUE;

                case IDC_DEBUG_JUMP:
                    jmpproc = MakeProcInstance((FARPROC)JumpProc, MainhInst);
                    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_JUMP), debughWnd, (DLGPROC)jmpproc);
                    FreeProcInstance(jmpproc);
                    
                    RefreshDebugAll();
                    return TRUE;

				case IDC_CPUOP:
					// This updated a global opcode counter.
                    //UpdateR5900op();
					return TRUE;

                case IDC_DEBUG_BP_EXEC:
                    bpexecproc = MakeProcInstance((FARPROC)BpexecProc, MainhInst);
                    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_BPEXEC), debughWnd, (DLGPROC)bpexecproc);
                    FreeProcInstance(bpexecproc);
                    return TRUE;

				case IDC_DEBUG_BP_COUNT:
                    bpcntproc = MakeProcInstance((FARPROC)BpcntProc, MainhInst);
                    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_BPCNT), debughWnd, (DLGPROC)bpcntproc);
                    FreeProcInstance(bpcntproc);
					return TRUE;

                case IDC_DEBUG_BP_CLEAR:
					memzero_obj(bkpt_regv);
					return TRUE;

				case IDC_DEBUG_DUMP:
                    dumpproc = MakeProcInstance((FARPROC)DumpProc, MainhInst);
                    DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_DUMP), debughWnd, (DLGPROC)dumpproc);
                    FreeProcInstance(dumpproc);
					return TRUE;

				case IDC_DEBUG_MEMORY:
					DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_MEMORY), debughWnd, (DLGPROC)MemoryProc);
					return TRUE;

                case IDC_DEBUG_CLOSE:

                    EndDialog(hRegDlg ,TRUE);                  
					EndDialog(hDlg,TRUE);
					EndDialog(hIopDlg,TRUE);

                    ClosePlugins( true );
					isRunning = 0;
                    return TRUE;

            }
            break;
    }

    return FALSE;
}

void RefreshDebugger(void)
{
    unsigned long t;
    int cnt;
    
    if (DebuggerPC == 0) 
       DebuggerPC = cpuRegs.pc; //- 0x00000038;

    SendMessage(hWnd_debugdisasm, LB_RESETCONTENT, 0, 0);

    for (t = DebuggerPC, cnt = 0; t < (DebuggerPC + 0x00000074); t += 0x00000004, cnt++)
    {
		char syscall_str[256];
		// Make the opcode.
		u32 *mem = (u32*)PSM(t);
		if (mem == NULL) {
			sprintf(syscall_str, "%8.8lx 00000000: NULL MEMORY", t);
		} else {
			/* special procesing for syscall. This should probably be moved into the disR5900Fasm() call in the future. */
			if (0x0c == *mem && 0x24030000 == (*(mem-1) & 0xFFFFFF00)){
				/* it's a syscall preceeded by a li v1,$data instruction. */
				u8 bios_call = *(mem-1) & 0xFF;
				sprintf(syscall_str, "%08X:\tsyscall\t%s", t, R5900::bios[bios_call]);
			} else {
				std::string str;
				R5900::disR5900Fasm(str, *mem, t);
				str.copy( syscall_str, 256 );
				syscall_str[str.length()] = 0;
			}
		}
        SendMessage(hWnd_debugdisasm, LB_ADDSTRING, 0, (LPARAM)syscall_str );
	}
}

void RefreshIOPDebugger(void)
{
    unsigned long t;
    int cnt;
	
	if (DebuggerIOPPC == 0){
		DebuggerIOPPC = psxRegs.pc; //- 0x00000038;
	}
    SendMessage(hWnd_IOP_debugdisasm, LB_RESETCONTENT, 0, 0);

    for (t = DebuggerIOPPC, cnt = 0; t < (DebuggerIOPPC + 0x00000029); t += 0x00000004, cnt++)
    {
		// Make the opcode.
		u32 mem = iopMemRead32( t );
		char *str = R3000A::disR3000Fasm(mem, t);
        SendMessage(hWnd_IOP_debugdisasm, LB_ADDSTRING, 0, (LPARAM)str);
	}
    
}
