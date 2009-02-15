//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "spu2.h"

int crazy_debug=0;

char s[4096];

FILE *spu2Log;

void FileLog(const char *fmt, ...) {
#ifdef SPU2_LOG
	int n;
	va_list list;

	if(!AccessLog()) return;
	if(!spu2Log) return;

	va_start(list, fmt);
	n=vsprintf(s,fmt, list);
	va_end(list);

	fputs(s,spu2Log);
	fflush(spu2Log);

#if 0
	if(crazy_debug)
	{
		fputs(s,stderr);
		fflush(stderr);
	}
#endif
#endif
}

void ConLog(const char *fmt, ...) {
#ifdef SPU2_LOG
	int n;
	va_list list;

	if(!MsgToConsole()) return;

	va_start(list, fmt);
	n=vsprintf(s,fmt, list);
	va_end(list);

	fputs(s,stderr);
	fflush(stderr);

	if(spu2Log)
	{
		fputs(s,spu2Log);
		fflush(spu2Log);
	}
#endif
}

void DoFullDump()
{
#ifdef SPU2_LOG
	FILE *dump;
	u8 c=0, v=0;

	if(MemDump())
	{
		dump = _wfopen( MemDumpFileName, _T("wb") );
		if (dump)
		{
			fwrite(_spu2mem,0x200000,1,dump);
			fclose(dump);
		}
	}
	if(RegDump())
	{
		dump = _wfopen( RegDumpFileName, _T("wb") );
		if (dump)
		{
			fwrite(spu2regs,0x2000,1,dump);
			fclose(dump);
		}
	}

	if(!CoresDump()) return;
	dump = _wfopen( CoresDumpFileName, _T("wt") );
	if (dump) {
		for(c=0;c<2;c++)
		{
			fprintf(dump,"#### CORE %d DUMP.\n",c);
			fprintf(dump,"Master Volume for Left Channel: %x\n"
						 "  - Value:     %x\n"
						 "  - Mode:      %x\n"
						 "  - Increment: %x\n",
						 Cores[c].MasterL.Reg_VOL,
						 Cores[c].MasterL.Value,
						 Cores[c].MasterL.Mode,
						 Cores[c].MasterL.Increment);
			fprintf(dump,"Master Volume for Right Channel: %x\n"
						 "  - Value:     %x\n"
						 "  - Mode:      %x\n"
						 "  - Increment: %x\n",
						 Cores[c].MasterR.Reg_VOL,
						 Cores[c].MasterR.Value,
						 Cores[c].MasterR.Mode,
						 Cores[c].MasterR.Increment);
			fprintf(dump,"Volume for External Data Input (Left Channel):  %x\n",Cores[c].ExtL);
			fprintf(dump,"Volume for External Data Input (Right Channel): %x\n",Cores[c].ExtR);
			fprintf(dump,"Volume for Sound Data Input (Left Channel):     %x\n",Cores[c].InpL);
			fprintf(dump,"Volume for Sound Data Input (Right Channel):    %x\n",Cores[c].InpR);
			fprintf(dump,"Volume for Output from Effects (Left Channel):  %x\n",Cores[c].FxL);
			fprintf(dump,"Volume for Output from Effects (Right Channel): %x\n",Cores[c].FxR);
			fprintf(dump,"Interrupt Address:          %x\n",Cores[c].IRQA);
			fprintf(dump,"DMA Transfer Start Address: %x\n",Cores[c].TSA);
			fprintf(dump,"External Input to Direct Output (Left):    %s\n",Cores[c].ExtDryL?"Yes":"No");
			fprintf(dump,"External Input to Direct Output (Right):   %s\n",Cores[c].ExtDryR?"Yes":"No");
			fprintf(dump,"External Input to Effects (Left):          %s\n",Cores[c].ExtWetL?"Yes":"No");
			fprintf(dump,"External Input to Effects (Right):         %s\n",Cores[c].ExtWetR?"Yes":"No");
			fprintf(dump,"Sound Data Input to Direct Output (Left):  %s\n",Cores[c].SndDryL?"Yes":"No");
			fprintf(dump,"Sound Data Input to Direct Output (Right): %s\n",Cores[c].SndDryR?"Yes":"No");
			fprintf(dump,"Sound Data Input to Effects (Left):        %s\n",Cores[c].SndWetL?"Yes":"No");
			fprintf(dump,"Sound Data Input to Effects (Right):       %s\n",Cores[c].SndWetR?"Yes":"No");
			fprintf(dump,"Voice Data Input to Direct Output (Left):  %s\n",Cores[c].InpDryL?"Yes":"No");
			fprintf(dump,"Voice Data Input to Direct Output (Right): %s\n",Cores[c].InpDryR?"Yes":"No");
			fprintf(dump,"Voice Data Input to Effects (Left):        %s\n",Cores[c].InpWetL?"Yes":"No");
			fprintf(dump,"Voice Data Input to Effects (Right):       %s\n",Cores[c].InpWetR?"Yes":"No");
			fprintf(dump,"IRQ Enabled:     %s\n",Cores[c].IRQEnable?"Yes":"No");
			fprintf(dump,"Effects Enabled: %s\n",Cores[c].FxEnable?"Yes":"No");
			fprintf(dump,"Mute Enabled:    %s\n",Cores[c].Mute?"Yes":"No");
			fprintf(dump,"Noise Clock:     %d\n",Cores[c].NoiseClk);
			fprintf(dump,"DMA Bits:        %d\n",Cores[c].DMABits);
			fprintf(dump,"Effects Start:   %x\n",Cores[c].EffectsStartA);
			fprintf(dump,"Effects End:     %x\n",Cores[c].EffectsEndA);
			fprintf(dump,"Registers:\n");
			fprintf(dump,"  - PMON:   %x\n",Cores[c].Regs.PMON);
			fprintf(dump,"  - NON:    %x\n",Cores[c].Regs.NON);
			fprintf(dump,"  - VMIXL:  %x\n",Cores[c].Regs.VMIXL);
			fprintf(dump,"  - VMIXR:  %x\n",Cores[c].Regs.VMIXR);
			fprintf(dump,"  - VMIXEL: %x\n",Cores[c].Regs.VMIXEL);
			fprintf(dump,"  - VMIXER: %x\n",Cores[c].Regs.VMIXER);
			fprintf(dump,"  - MMIX:   %x\n",Cores[c].Regs.VMIXEL);
			fprintf(dump,"  - ENDX:   %x\n",Cores[c].Regs.VMIXER);
			fprintf(dump,"  - STATX:  %x\n",Cores[c].Regs.VMIXEL);
			fprintf(dump,"  - ATTR:   %x\n",Cores[c].Regs.VMIXER);
			for(v=0;v<24;v++) {
				fprintf(dump,"Voice %d:\n",v);
				fprintf(dump,"  - Volume for Left Channel: %x\n"
							 "     - Value:     %x\n"
							 "     - Mode:      %x\n"
							 "     - Increment: %x\n",
							 Cores[c].Voices[v].VolumeL.Reg_VOL,
							 Cores[c].Voices[v].VolumeL.Value,
							 Cores[c].Voices[v].VolumeL.Mode,
							 Cores[c].Voices[v].VolumeL.Increment);
				fprintf(dump,"  - Volume for Right Channel: %x\n"
							 "     - Value:     %x\n"
							 "     - Mode:      %x\n"
							 "     - Increment: %x\n",
							 Cores[c].Voices[v].VolumeR.Reg_VOL,
							 Cores[c].Voices[v].VolumeR.Value,
							 Cores[c].Voices[v].VolumeR.Mode,
							 Cores[c].Voices[v].VolumeR.Increment);
				fprintf(dump,"  - ADSR Envelope: %x & %x\n"
							 "     - Ar: %x\n"
							 "     - Am: %x\n"
							 "     - Dr: %x\n"
							 "     - Sl: %x\n"
							 "     - Sr: %x\n"
							 "     - Sm: %x\n"
							 "     - Rr: %x\n"
							 "     - Rm: %x\n"
							 "     - Phase: %x\n"
							 "     - Value: %x\n",
							 Cores[c].Voices[v].ADSR.Reg_ADSR1,
							 Cores[c].Voices[v].ADSR.Reg_ADSR2,
							 Cores[c].Voices[v].ADSR.Ar,
							 Cores[c].Voices[v].ADSR.Am,
							 Cores[c].Voices[v].ADSR.Dr,
							 Cores[c].Voices[v].ADSR.Sl,
							 Cores[c].Voices[v].ADSR.Sr,
							 Cores[c].Voices[v].ADSR.Sm,
							 Cores[c].Voices[v].ADSR.Rr,
							 Cores[c].Voices[v].ADSR.Rm,
							 Cores[c].Voices[v].ADSR.Phase,
							 Cores[c].Voices[v].ADSR.Value);
				fprintf(dump,"  - Pitch:     %x\n",Cores[c].Voices[v].Pitch);
				fprintf(dump,"  - Modulated: %s\n",Cores[c].Voices[v].Modulated?"Yes":"No");
				fprintf(dump,"  - Source:    %s\n",Cores[c].Voices[v].Noise?"Noise":"Wave");
				fprintf(dump,"  - Direct Output for Left Channel:   %s\n",Cores[c].Voices[v].DryL?"Yes":"No");
				fprintf(dump,"  - Direct Output for Right Channel:  %s\n",Cores[c].Voices[v].DryR?"Yes":"No");
				fprintf(dump,"  - Effects Output for Left Channel:  %s\n",Cores[c].Voices[v].WetL?"Yes":"No");
				fprintf(dump,"  - Effects Output for Right Channel: %s\n",Cores[c].Voices[v].WetR?"Yes":"No");
				fprintf(dump,"  - Loop Start Adress:  %x\n",Cores[c].Voices[v].LoopStartA);
				fprintf(dump,"  - Sound Start Adress: %x\n",Cores[c].Voices[v].StartA);
				fprintf(dump,"  - Next Data Adress:   %x\n",Cores[c].Voices[v].NextA);
				fprintf(dump,"  - Play Start Cycle:   %d\n",Cores[c].Voices[v].PlayCycle);
				fprintf(dump,"  - Play Status:        %s\n",(Cores[c].Voices[v].ADSR.Phase>0)?"Playing":"Not Playing");
				fprintf(dump,"  - Block Sample:       %d\n",Cores[c].Voices[v].SCurrent);
			}
			fprintf(dump,"#### END OF DUMP.\n\n");
		}
	}
	fclose(dump);
	dump = fopen( "logs/effects.txt", "wt" );
	if (dump)
	{
		for(c=0;c<2;c++)
		{
			fprintf(dump,"#### CORE %d EFFECTS PROCESSOR DUMP.\n",c);

			fprintf(dump,"  - IN_COEF_L:   %x\n",Cores[c].Revb.IN_COEF_R);
			fprintf(dump,"  - IN_COEF_R:   %x\n",Cores[c].Revb.IN_COEF_L);

			fprintf(dump,"  - FB_ALPHA:    %x\n",Cores[c].Revb.FB_ALPHA);
			fprintf(dump,"  - FB_X:        %x\n",Cores[c].Revb.FB_X);
			fprintf(dump,"  - FB_SRC_A:    %x\n",Cores[c].Revb.FB_SRC_A);
			fprintf(dump,"  - FB_SRC_B:    %x\n",Cores[c].Revb.FB_SRC_B);

			fprintf(dump,"  - IIR_ALPHA:   %x\n",Cores[c].Revb.IIR_ALPHA);
			fprintf(dump,"  - IIR_COEF:    %x\n",Cores[c].Revb.IIR_COEF);
			fprintf(dump,"  - IIR_SRC_A0:  %x\n",Cores[c].Revb.IIR_SRC_A0);
			fprintf(dump,"  - IIR_SRC_A1:  %x\n",Cores[c].Revb.IIR_SRC_A1);
			fprintf(dump,"  - IIR_SRC_B1:  %x\n",Cores[c].Revb.IIR_SRC_B0);
			fprintf(dump,"  - IIR_SRC_B0:  %x\n",Cores[c].Revb.IIR_SRC_B1);
			fprintf(dump,"  - IIR_DEST_A0: %x\n",Cores[c].Revb.IIR_DEST_A0);
			fprintf(dump,"  - IIR_DEST_A1: %x\n",Cores[c].Revb.IIR_DEST_A1);
			fprintf(dump,"  - IIR_DEST_B0: %x\n",Cores[c].Revb.IIR_DEST_B0);
			fprintf(dump,"  - IIR_DEST_B1: %x\n",Cores[c].Revb.IIR_DEST_B1);

			fprintf(dump,"  - ACC_COEF_A:  %x\n",Cores[c].Revb.ACC_COEF_A);
			fprintf(dump,"  - ACC_COEF_B:  %x\n",Cores[c].Revb.ACC_COEF_B);
			fprintf(dump,"  - ACC_COEF_C:  %x\n",Cores[c].Revb.ACC_COEF_C);
			fprintf(dump,"  - ACC_COEF_D:  %x\n",Cores[c].Revb.ACC_COEF_D);
			fprintf(dump,"  - ACC_SRC_A0:  %x\n",Cores[c].Revb.ACC_SRC_A0);
			fprintf(dump,"  - ACC_SRC_A1:  %x\n",Cores[c].Revb.ACC_SRC_A1);
			fprintf(dump,"  - ACC_SRC_B0:  %x\n",Cores[c].Revb.ACC_SRC_B0);
			fprintf(dump,"  - ACC_SRC_B1:  %x\n",Cores[c].Revb.ACC_SRC_B1);
			fprintf(dump,"  - ACC_SRC_C0:  %x\n",Cores[c].Revb.ACC_SRC_C0);
			fprintf(dump,"  - ACC_SRC_C1:  %x\n",Cores[c].Revb.ACC_SRC_C1);
			fprintf(dump,"  - ACC_SRC_D0:  %x\n",Cores[c].Revb.ACC_SRC_D0);
			fprintf(dump,"  - ACC_SRC_D1:  %x\n",Cores[c].Revb.ACC_SRC_D1);

			fprintf(dump,"  - MIX_DEST_A0: %x\n",Cores[c].Revb.MIX_DEST_A0);
			fprintf(dump,"  - MIX_DEST_A1: %x\n",Cores[c].Revb.MIX_DEST_A1);
			fprintf(dump,"  - MIX_DEST_B0: %x\n",Cores[c].Revb.MIX_DEST_B0);
			fprintf(dump,"  - MIX_DEST_B1: %x\n",Cores[c].Revb.MIX_DEST_B1);
			fprintf(dump,"#### END OF DUMP.\n\n");
		}
		fclose(dump);
	}
#endif
}
