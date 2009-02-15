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
#include "Win32.h"

#include "Common.h"
#include "Counters.h"
#include "VUmicro.h"
#include "Plugins.h"

#include "ix86/ix86.h"

static void EnableDlgItem( HWND hwndDlg, uint itemidx, bool enabled )
{
	EnableWindow( GetDlgItem( hwndDlg, itemidx ), enabled );
}

BOOL CALLBACK CpuDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char cpuspeedc[20];
	char features[256];
	char cfps[20];
	char cFrameskip[20];
	char cConsecutiveFrames[20];
	char cConsecutiveSkip[20];
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
			Static_SetText(GetDlgItem(hW, IDC_FRAMELIMIT), _("Frame Limiting (F4 key switches the mode in-game!)"));
			Static_SetText(GetDlgItem(hW, IDC_CPU_FL_NORMAL), _("Normal - All frames are rendered as fast as possible."));
			Static_SetText(GetDlgItem(hW, IDC_CPU_FL_LIMIT), _("Limit - Force frames to normal speeds if too fast."));
			Static_SetText(GetDlgItem(hW, IDC_CPU_FL_SKIP), _("Frame Skip - In order to achieve normal speeds,\nsome frames are skipped (fast).\nFps displayed counts skipped frames too."));
			Static_SetText(GetDlgItem(hW, IDC_CPU_FL_SKIPVU), _("VU Skip - Same as 'Frame Skip', but tries to skip more.\nArtifacts might be present, but will be faster."));
			Static_SetText(GetDlgItem(hW, IDC_CUSTOM_FPS), _("Custom FPS Limit (0=auto):"));
			Static_SetText(GetDlgItem(hW, IDC_FRAMESKIP_LABEL1), _("Skip Frames when slower than:\n(See Note 1)"));
			Static_SetText(GetDlgItem(hW, IDC_FRAMESKIP_LABEL2), _("Consecutive Frames before skipping:\n(See Note 2)"));
			Static_SetText(GetDlgItem(hW, IDC_FRAMESKIP_LABEL3), _("*Note 1: Will only skip when slower than this fps number.\n (0 = Auto) ; (9999 = Forced-Frameskip regardless of speed.)\n (e.g. If set to 45, will only skip when slower than 45fps.)"));
			Static_SetText(GetDlgItem(hW, IDC_FRAMESKIP_LABEL4), _("*Note 2: Will render this number of consecutive frames before\n  skipping the next frame. (0=default)\n (e.g. If set to 2, will render 2 frames before skipping 1.)"));
			Static_SetText(GetDlgItem(hW, IDC_FRAMESKIP_LABEL5), _("Consecutive Frames to skip:\n(See Note 3)"));
			Static_SetText(GetDlgItem(hW, IDC_FRAMESKIP_LABEL6), _("*Note 3: Will skip this number of frames before\n  rendering the next sequence of frames. (0=default)\n (e.g. If set to 2, will skip 2 consecutive frames whenever its time\n  to skip.)"));

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
//			if(cpucaps.hasAMD64BitArchitecture) strcat(features,",x86-64");
            SetDlgItemText(hW, IDC_FEATURESINPUT, features);

			CheckDlgButton(hW, IDC_CPU_EEREC, !!(Config.Options&PCSX2_EEREC));
			CheckDlgButton(hW, IDC_CPU_VU0REC, !!(Config.Options&PCSX2_VU0REC));
			CheckDlgButton(hW, IDC_CPU_VU1REC, !!(Config.Options&PCSX2_VU1REC));

			EnableDlgItem( hW, IDC_CPU_EEREC, !g_Session.ForceDisableEErec );
			EnableDlgItem( hW, IDC_CPU_VU0REC, !g_Session.ForceDisableVU0rec );
			EnableDlgItem( hW, IDC_CPU_VU1REC, !g_Session.ForceDisableVU1rec );

			CheckDlgButton(hW, IDC_CPU_GSMULTI, !!CHECK_MULTIGS);

			CheckRadioButton(hW,IDC_CPU_FL_NORMAL, IDC_CPU_FL_NORMAL+3, IDC_CPU_FL_NORMAL+(CHECK_FRAMELIMIT>>10));
			
			sprintf(cfps,"%d",Config.CustomFps);
			SetDlgItemText(hW, IDC_CUSTOMFPS, cfps);

			sprintf(cFrameskip,"%d",Config.CustomFrameSkip);
			SetDlgItemText(hW, IDC_CUSTOM_FRAMESKIP, cFrameskip);
			
			sprintf(cConsecutiveFrames,"%d",Config.CustomConsecutiveFrames);
			SetDlgItemText(hW, IDC_CUSTOM_CONSECUTIVE_FRAMES, cConsecutiveFrames);

			sprintf(cConsecutiveSkip,"%d",Config.CustomConsecutiveSkip);
			SetDlgItemText(hW, IDC_CUSTOM_CONSECUTIVE_SKIP, cConsecutiveSkip);

			//EnableWindow( GetDlgItem( hW, IDC_CPU_GSMULTI ), !g_GameInProgress );

			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hW, FALSE);
				return FALSE;

				case IDOK:
					newopts = 0;

					if( SendDlgItemMessage(hW,IDC_CPU_EEREC,BM_GETCHECK,0,0) ) newopts |= PCSX2_EEREC;

					if( SendDlgItemMessage(hW,IDC_CPU_VU0REC,BM_GETCHECK,0,0) ) newopts |= PCSX2_VU0REC;
					if( SendDlgItemMessage(hW,IDC_CPU_VU1REC,BM_GETCHECK,0,0) ) newopts |= PCSX2_VU1REC;

					if( SendDlgItemMessage(hW,IDC_CPU_GSMULTI,BM_GETCHECK,0,0) ) newopts |= PCSX2_GSMULTITHREAD;

					if( SendDlgItemMessage(hW,IDC_CPU_FL_NORMAL,BM_GETCHECK,0,0) ) newopts |= PCSX2_FRAMELIMIT_NORMAL;
					else if( SendDlgItemMessage(hW,IDC_CPU_FL_LIMIT,BM_GETCHECK,0,0) ) newopts |= PCSX2_FRAMELIMIT_LIMIT;
					else if( SendDlgItemMessage(hW,IDC_CPU_FL_SKIP,BM_GETCHECK,0,0) ) newopts |= PCSX2_FRAMELIMIT_SKIP;
					else if( SendDlgItemMessage(hW,IDC_CPU_FL_SKIPVU,BM_GETCHECK,0,0) ) newopts |= PCSX2_FRAMELIMIT_VUSKIP;

					GetDlgItemText(hW, IDC_CUSTOMFPS, cfps, 20);
					Config.CustomFps = atoi(cfps);

					GetDlgItemText(hW, IDC_CUSTOM_FRAMESKIP, cFrameskip, 20);
					Config.CustomFrameSkip = atoi(cFrameskip);

					GetDlgItemText(hW, IDC_CUSTOM_CONSECUTIVE_FRAMES, cConsecutiveFrames, 20);
					Config.CustomConsecutiveFrames = atoi(cConsecutiveFrames);

					GetDlgItemText(hW, IDC_CUSTOM_CONSECUTIVE_SKIP, cConsecutiveSkip, 20);
					Config.CustomConsecutiveSkip = atoi(cConsecutiveSkip);

					EndDialog(hW, TRUE);

					if( Config.Options != newopts )
					{
						SysRestorableReset();

						if( (Config.Options&PCSX2_GSMULTITHREAD) ^ (newopts&PCSX2_GSMULTITHREAD) )
						{
							// Need the MTGS setting to take effect, so close out the plugins:
							PluginsResetGS();
							if( CHECK_MULTIGS )
								Console::Notice( "MTGS mode disabled.\n\tEnjoy the fruits of single-threaded simpicity." );
							else
								Console::Notice( "MTGS mode enabled.\n\tWelcome to multi-threaded awesomeness." );
						}
						Config.Options = newopts;
					}
					else if( Cpu != NULL )
						UpdateVSyncRate();

					SaveConfig();

				return FALSE;
			}
		return TRUE;
	}
	return FALSE;
}
