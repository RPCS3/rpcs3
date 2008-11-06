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

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>

#include "Common.h"
#include "VUmicro.h"
#include "PsxCommon.h"
#include "plugins.h"
#include "resource.h"
#include "Win32.h"

#include "ix86/ix86.h"

BOOL CALLBACK CpuDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char cpuspeedc[20];
	char features[256];
	char cfps[20];
	u32 newopts;

	switch(uMsg) {
		case WM_INITDIALOG:
            SetWindowText(hW, _("Cpu Config"));
			SetDlgItemText(hW, IDC_VENDORINPUT,cpuinfo.x86ID );
            SetDlgItemText(hW, IDC_FAMILYINPUT, cpuinfo.x86Fam);
			sprintf(cpuspeedc,"%d MHZ",cpuinfo.cpuspeed);
			SetDlgItemText(hW, IDC_CPUSPEEDINPUT, cpuspeedc);
			Static_SetText(GetDlgItem(hW, IDC_VENDORNAME), _("CPU Vendor"));
			Static_SetText(GetDlgItem(hW, IDC_FAMILYNAME), _("Family"));
			Static_SetText(GetDlgItem(hW, IDC_CPUSPEEDNAME), _("CPU Speed"));
			Static_SetText(GetDlgItem(hW, IDC_FEATURESNAME), _("Features"));
			Static_SetText(GetDlgItem(hW, IDC_CPU_EEREC), _("EERec -  EE/IOP recompiler (need MMX/SSE)"));
			Static_SetText(GetDlgItem(hW, IDC_CPU_VUGROUP), _("VU Recompilers - All options are set by default"));
			Static_SetText(GetDlgItem(hW, IDC_CPU_VU0REC), _("VU0rec - enable recompiler for VU0 unit"));
			Static_SetText(GetDlgItem(hW, IDC_CPU_VU1REC), _("VU1rec - enable recompiler for VU1 unit"));
			Static_SetText(GetDlgItem(hW, IDC_CPU_GSMULTI), _("Multi threaded GS mode (MTGS)\n(faster on dual core/HT procs, requires pcsx2 restart)"));
			Static_SetText(GetDlgItem(hW, IDC_CPU_MULTI), _("Dual Core Mode (DC) - Much faster but only valid with MTGS"));
			Static_SetText(GetDlgItem(hW, IDC_FRAMELIMIT), _("Frame Limiting"));
			Static_SetText(GetDlgItem(hW, IDC_CPU_FL_NORMAL), _("Normal - All frames are rendered as fast as possible."));
			Static_SetText(GetDlgItem(hW, IDC_CPU_FL_LIMIT), _("Limit - Force frames to normal speeds if too fast."));
			Static_SetText(GetDlgItem(hW, IDC_CPU_FL_SKIP), _("Frame Skip - In order to achieve normal speeds,\nsome frames are skipped (fast).\nFps displayed counts skipped frames too."));
			Static_SetText(GetDlgItem(hW, IDC_CPU_FL_SKIPVU), _("VU Skip - Same as 'Frame Skip', but tries to skip more.\nArtifacts might be present, but will be faster."));
			Static_SetText(GetDlgItem(hW, IDC_CUSTOM_FPS), _("Custom FPS (0=auto)"));

			Button_SetText(GetDlgItem(hW, IDOK), _("OK"));
			Button_SetText(GetDlgItem(hW, IDCANCEL), _("Cancel"));

			//features[0]=':';
			//strcat(features,"");
			strcpy(features,"");
            if(cpucaps.hasMultimediaExtensions) strcat(features,"MMX");
            if(cpucaps.hasStreamingSIMDExtensions) strcat(features,",SSE");
            if(cpucaps.hasStreamingSIMD2Extensions) strcat(features,",SSE2");
			if(cpucaps.hasStreamingSIMD3Extensions) strcat(features,",SSE3");
			if(cpucaps.hasStreamingSIMD4Extensions) strcat(features,",SSE4.1");
//            if(cpucaps.has3DNOWInstructionExtensions) strcat(features,",3DNOW");
//            if(cpucaps.has3DNOWInstructionExtensionsExt)strcat(features,",3DNOW+");
			if(cpucaps.hasAMD64BitArchitecture) strcat(features,",x86-64");
            SetDlgItemText(hW, IDC_FEATURESINPUT, features);
			if(!cpucaps.hasStreamingSIMDExtensions) 
			{
				EnableWindow(GetDlgItem(hW,IDC_RADIORECOMPILERVU),FALSE);//disable checkbox if no SSE2 found
				Config.Options &= (PCSX2_VU0REC|PCSX2_VU1REC);//disable the config just in case
			}
			if(!cpucaps.hasMultimediaExtensions)
			{
                  EnableWindow(GetDlgItem(hW,IDC_RADIORECOMPILER),FALSE);
				  Config.Options &= ~(PCSX2_EEREC|PCSX2_VU0REC|PCSX2_VU1REC|PCSX2_COP2REC);//return to interpreter mode

			}
			SetDlgItemText(hW, IDC_FEATURESINPUT, features);

			CheckDlgButton(hW, IDC_CPU_EEREC, !!CHECK_EEREC);

//#ifdef PCSX2_DEVBUILD
			CheckDlgButton(hW, IDC_CPU_VU0REC, !!CHECK_VU0REC);
			CheckDlgButton(hW, IDC_CPU_VU1REC, !!CHECK_VU1REC);
//#else
//			// don't show
//			ShowWindow(GetDlgItem(hW, IDC_CPU_VUGROUP), SW_HIDE);
//			ShowWindow(GetDlgItem(hW, IDC_CPU_VU0REC), SW_HIDE);
//			ShowWindow(GetDlgItem(hW, IDC_CPU_VU1REC), SW_HIDE);
//#endif

			CheckDlgButton(hW, IDC_CPU_GSMULTI, !!CHECK_MULTIGS);
			CheckDlgButton(hW, IDC_CPU_MULTI, !!CHECK_DUALCORE);

			CheckRadioButton(hW,IDC_CPU_FL_NORMAL, IDC_CPU_FL_NORMAL+3, IDC_CPU_FL_NORMAL+(CHECK_FRAMELIMIT>>10));
			
			sprintf(cfps,"%d",Config.CustomFps);
			SetDlgItemText(hW, IDC_CUSTOMFPS, cfps);

			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					EndDialog(hW, FALSE);
					return TRUE;

				case IDOK:
                    Cpu->Shutdown();
					vu0Shutdown();
					vu1Shutdown();
                    
					newopts = 0;

					if( SendDlgItemMessage(hW,IDC_CPU_EEREC,BM_GETCHECK,0,0) ) newopts |= PCSX2_EEREC;

//#ifdef PCSX2_DEVBUILD
					if( SendDlgItemMessage(hW,IDC_CPU_VU0REC,BM_GETCHECK,0,0) ) newopts |= PCSX2_VU0REC;
					if( SendDlgItemMessage(hW,IDC_CPU_VU1REC,BM_GETCHECK,0,0) ) newopts |= PCSX2_VU1REC;
//#else  //Why oh why were we forcing this in release to public builds?
//					newopts |= PCSX2_VU0REC|PCSX2_VU1REC;
//#endif

					if( SendDlgItemMessage(hW,IDC_CPU_GSMULTI,BM_GETCHECK,0,0) ) newopts |= PCSX2_GSMULTITHREAD;
					if( SendDlgItemMessage(hW,IDC_CPU_MULTI,BM_GETCHECK,0,0) ) newopts |= PCSX2_DUALCORE;
					if( SendDlgItemMessage(hW,IDC_CPU_FRAMELIMIT,BM_GETCHECK,0,0) ) newopts |= PCSX2_FRAMELIMIT;

					if( SendDlgItemMessage(hW,IDC_CPU_FL_NORMAL,BM_GETCHECK,0,0) ) newopts |= PCSX2_FRAMELIMIT_NORMAL;
					else if( SendDlgItemMessage(hW,IDC_CPU_FL_LIMIT,BM_GETCHECK,0,0) ) newopts |= PCSX2_FRAMELIMIT_LIMIT;
					else if( SendDlgItemMessage(hW,IDC_CPU_FL_SKIP,BM_GETCHECK,0,0) ) newopts |= PCSX2_FRAMELIMIT_SKIP;
					else if( SendDlgItemMessage(hW,IDC_CPU_FL_SKIPVU,BM_GETCHECK,0,0) ) newopts |= PCSX2_FRAMELIMIT_VUSKIP;

					if( (Config.Options&PCSX2_GSMULTITHREAD) ^ (newopts&PCSX2_GSMULTITHREAD) ) {
						Config.Options = newopts;
						SaveConfig();
						MessageBox(NULL, "Restart Pcsx2", "Query", MB_OK);
						exit(0);
					}

					if( newopts & PCSX2_EEREC ) newopts |= PCSX2_COP2REC;

					Config.Options = newopts;

					GetDlgItemText(hW, IDC_CUSTOMFPS, cfps, 20);
					Config.CustomFps = atoi(cfps);

					UpdateVSyncRate();
					SaveConfig();

					cpuRestartCPU();
					EndDialog(hW, TRUE);
					return TRUE;
			}
	}
	return FALSE;
}
