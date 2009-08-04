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

////////////////////////////////////////////////////
#include "PrecompiledHeader.h"
#include "IopCommon.h"
#include "Counters.h"
#include "iCore.h"
#include "iR5900.h"
#include "IPU/IPU.h"

#include "AppConfig.h"

using namespace R5900;
// fixme: currently should not be uncommented.
//#define TEST_BROKEN_DUMP_ROUTINES

#ifdef TEST_BROKEN_DUMP_ROUTINES
//extern u32 psxdump;
//extern int rdram_devices;	// put 8 for TOOL and 2 for PS2 and PSX
//extern int rdram_sdevid;
extern tIPU_BP g_BP;

#define VF_VAL(x) ((x==0x80000000)?0:(x))
#endif


// iR5900-32.cpp
extern EEINST* s_pInstCache;
extern u32 s_nEndBlock; // what pc the current block ends	


void iDumpPsxRegisters(u32 startpc, u32 temp)
{
// [TODO] fixme : thie code is broken and has no labels.  Needs a rewrite to be useful.

#ifdef TEST_BROKEN_DUMP_ROUTINES
	int i;
	const char* pstr = temp ? "t" : "";

	// fixme: PSXM doesn't exist any more.
	//__Log("%spsxreg: %x %x ra:%x k0: %x %x", pstr, startpc, psxRegs.cycle, psxRegs.GPR.n.ra, psxRegs.GPR.n.k0, *(int*)PSXM(0x13c128));
	
	for(i = 0; i < 34; i+=2) __Log("%spsx%s: %x %x", pstr, disRNameGPR[i], psxRegs.GPR.r[i], psxRegs.GPR.r[i+1]);
	
	__Log("%scycle: %x %x %x; counters %x %x", pstr, psxRegs.cycle, g_psxNextBranchCycle, EEsCycle, 
		psxNextsCounter, psxNextCounter);

	__Log("psxdma%d c%x b%x m%x t%x", 2, HW_DMA2_CHCR, HW_DMA2_BCR, HW_DMA2_MADR, HW_DMA2_TADR);
	__Log("psxdma%d c%x b%x m%x", 3, HW_DMA3_CHCR, HW_DMA3_BCR, HW_DMA3_MADR);
	__Log("psxdma%d c%x b%x m%x t%x", 4, HW_DMA4_CHCR, HW_DMA4_BCR, HW_DMA4_MADR, HW_DMA4_TADR);
	__Log("psxdma%d c%x b%x m%x", 6, HW_DMA6_CHCR, HW_DMA6_BCR, HW_DMA6_MADR);
	__Log("psxdma%d c%x b%x m%x", 7, HW_DMA7_CHCR, HW_DMA7_BCR, HW_DMA7_MADR);
	__Log("psxdma%d c%x b%x m%x", 8, HW_DMA8_CHCR, HW_DMA8_BCR, HW_DMA8_MADR);
	__Log("psxdma%d c%x b%x m%x t%x", 9, HW_DMA9_CHCR, HW_DMA9_BCR, HW_DMA9_MADR, HW_DMA9_TADR);
	__Log("psxdma%d c%x b%x m%x", 10, HW_DMA10_CHCR, HW_DMA10_BCR, HW_DMA10_MADR);
	__Log("psxdma%d c%x b%x m%x", 11, HW_DMA11_CHCR, HW_DMA11_BCR, HW_DMA11_MADR);
	__Log("psxdma%d c%x b%x m%x", 12, HW_DMA12_CHCR, HW_DMA12_BCR, HW_DMA12_MADR);
	
	for(i = 0; i < 7; ++i)
		__Log("%scounter%d: mode %x count %I64x rate %x scycle %x target %I64x", pstr, i, psxCounters[i].mode, psxCounters[i].count, psxCounters[i].rate, psxCounters[i].sCycleT, psxCounters[i].target);
#endif
}

void iDumpRegisters(u32 startpc, u32 temp)
{
// [TODO] fixme : this code is broken and has no labels.  Needs a rewrite to be useful.

#ifdef TEST_BROKEN_DUMP_ROUTINES

	int i;
	const char* pstr;// = temp ? "t" : "";
	const u32 dmacs[] = {0x8000, 0x9000, 0xa000, 0xb000, 0xb400, 0xc000, 0xc400, 0xc800, 0xd000, 0xd400 };
	const char* psymb;
	
	if (temp)
		pstr = "t";
	else
		pstr = "";
	
	psymb = disR5900GetSym(startpc);

	if( psymb != NULL )
		__Log("%sreg(%s): %x %x c:%x", pstr, psymb, startpc, cpuRegs.interrupt, cpuRegs.cycle);
	else
		__Log("%sreg: %x %x c:%x", pstr, startpc, cpuRegs.interrupt, cpuRegs.cycle);
	
	for(i = 1; i < 32; ++i) __Log("%s: %x_%x_%x_%x", disRNameGPR[i], cpuRegs.GPR.r[i].UL[3], cpuRegs.GPR.r[i].UL[2], cpuRegs.GPR.r[i].UL[1], cpuRegs.GPR.r[i].UL[0]);
	
	//for(i = 0; i < 32; i+=4) __Log("cp%d: %x_%x_%x_%x", i, cpuRegs.CP0.r[i], cpuRegs.CP0.r[i+1], cpuRegs.CP0.r[i+2], cpuRegs.CP0.r[i+3]);
	//for(i = 0; i < 32; ++i) __Log("%sf%d: %f %x", pstr, i, fpuRegs.fpr[i].f, fpuRegs.fprc[i]);
	//for(i = 1; i < 32; ++i) __Log("%svf%d: %f %f %f %f, vi: %x", pstr, i, VU0.VF[i].F[3], VU0.VF[i].F[2], VU0.VF[i].F[1], VU0.VF[i].F[0], VU0.VI[i].UL);
	
	for(i = 0; i < 32; ++i) __Log("%sf%d: %x %x", pstr, i, fpuRegs.fpr[i].UL, fpuRegs.fprc[i]);
	for(i = 1; i < 32; ++i) __Log("%svf%d: %x %x %x %x, vi: %x", pstr, i, VU0.VF[i].UL[3], VU0.VF[i].UL[2], VU0.VF[i].UL[1], VU0.VF[i].UL[0], VU0.VI[i].UL);
	
	__Log("%svfACC: %x %x %x %x", pstr, VU0.ACC.UL[3], VU0.ACC.UL[2], VU0.ACC.UL[1], VU0.ACC.UL[0]);
	__Log("%sLO: %x_%x_%x_%x, HI: %x_%x_%x_%x", pstr, cpuRegs.LO.UL[3], cpuRegs.LO.UL[2], cpuRegs.LO.UL[1], cpuRegs.LO.UL[0],
	cpuRegs.HI.UL[3], cpuRegs.HI.UL[2], cpuRegs.HI.UL[1], cpuRegs.HI.UL[0]);
	__Log("%sCycle: %x %x, Count: %x", pstr, cpuRegs.cycle, g_nextBranchCycle, cpuRegs.CP0.n.Count);
	
	iDumpPsxRegisters(psxRegs.pc, temp);

	__Log("f410,30,40: %x %x %x, %d %d", psHu32(0xf410), psHu32(0xf430), psHu32(0xf440), rdram_sdevid, rdram_devices);
	__Log("cyc11: %x %x; vu0: %x, vu1: %x", cpuRegs.sCycle[1], cpuRegs.eCycle[1], VU0.cycle, VU1.cycle);

	__Log("%scounters: %x %x; psx: %x %x", pstr, nextsCounter, nextCounter, psxNextsCounter, psxNextCounter);
	
	// fixme: The members of the counters[i] struct are wrong here.
	/*for(i = 0; i < 4; ++i) {
		__Log("eetimer%d: count: %x mode: %x target: %x %x; %x %x; %x %x %x %x", i,
			counters[i].count, counters[i].mode, counters[i].target, counters[i].hold, counters[i].rate,
			counters[i].interrupt, counters[i].Cycle, counters[i].sCycle, counters[i].CycleT, counters[i].sCycleT);
	}*/
	__Log("VIF0_STAT = %x, VIF1_STAT = %x", psHu32(0x3800), psHu32(0x3C00));
	__Log("ipu %x %x %x %x; bp: %x %x %x %x", psHu32(0x2000), psHu32(0x2010), psHu32(0x2020), psHu32(0x2030), g_BP.BP, g_BP.bufferhasnew, g_BP.FP, g_BP.IFC);
	__Log("gif: %x %x %x", psHu32(0x3000), psHu32(0x3010), psHu32(0x3020));
	
	for(i = 0; i < ArraySize(dmacs); ++i) {
		DMACh* p = (DMACh*)(PS2MEM_HW+dmacs[i]);
		__Log("dma%d c%x m%x q%x t%x s%x", i, p->chcr, p->madr, p->qwc, p->tadr, p->sadr);
	}
	__Log("dmac %x %x %x %x", psHu32(DMAC_CTRL), psHu32(DMAC_STAT), psHu32(DMAC_RBSR), psHu32(DMAC_RBOR));
	__Log("intc %x %x", psHu32(INTC_STAT), psHu32(INTC_MASK));
	__Log("sif: %x %x %x %x %x", psHu32(0xf200), psHu32(0xf220), psHu32(0xf230), psHu32(0xf240), psHu32(0xf260));
#endif
}

void iDumpVU0Registers()
{
	// fixme: This code is outdated, broken, and lacks printed labels.
	// Needs heavy mods to be useful.
#ifdef TEST_BROKEN_DUMP_ROUTINES
	int i;
	
	for(i = 1; i < 32; ++i) {
		__Log("v%d: %x %x %x %x, vi: ", i, VF_VAL(VU0.VF[i].UL[3]), VF_VAL(VU0.VF[i].UL[2]),
			VF_VAL(VU0.VF[i].UL[1]), VF_VAL(VU0.VF[i].UL[0]));
		if( i == REG_Q || i == REG_P ) 
			__Log("%f\n", VU0.VI[i].F);
		else if( i == REG_MAC_FLAG ) 
			__Log("%x\n", 0);//VU0.VI[i].UL&0xff);
		else if( i == REG_STATUS_FLAG )
			__Log("%x\n", 0);//VU0.VI[i].UL&0x03);
		else if( i == REG_CLIP_FLAG ) 
			__Log("0\n");
		else
			__Log("%x\n", VU0.VI[i].UL);
	}
	__Log("vfACC: %f %f %f %f\n", VU0.ACC.F[3], VU0.ACC.F[2], VU0.ACC.F[1], VU0.ACC.F[0]);
#endif
}

void iDumpVU1Registers()
{
	// fixme: This code is outdated, broken, and lacks printed labels.
	// Needs heavy mods to be useful.
#ifdef TEST_BROKEN_DUMP_ROUTINES
	int i;
	
//	static int icount = 0;
//	__Log("%x\n", icount);
	
	for(i = 1; i < 32; ++i) {
		
//		__Log("v%d: w%f(%x) z%f(%x) y%f(%x) x%f(%x), vi: ", i, VU1.VF[i].F[3], VU1.VF[i].UL[3], VU1.VF[i].F[2], VU1.VF[i].UL[2],
//			VU1.VF[i].F[1], VU1.VF[i].UL[1], VU1.VF[i].F[0], VU1.VF[i].UL[0]);
		//__Log("v%d: %f %f %f %f, vi: ", i, VU1.VF[i].F[3], VU1.VF[i].F[2], VU1.VF[i].F[1], VU1.VF[i].F[0]);
		
		__Log("v%d: %x %x %x %x, vi: ", i, VF_VAL(VU1.VF[i].UL[3]), VF_VAL(VU1.VF[i].UL[2]), VF_VAL(VU1.VF[i].UL[1]), VF_VAL(VU1.VF[i].UL[0]));
		
		if( i == REG_Q || i == REG_P ) __Log("%f\n", VU1.VI[i].F);
		//else __Log("%x\n", VU1.VI[i].UL);
		else __Log("%x\n", (i==REG_STATUS_FLAG||i==REG_MAC_FLAG||i==REG_CLIP_FLAG)?0:VU1.VI[i].UL);
	}
	__Log("vfACC: %f %f %f %f\n", VU1.ACC.F[3], VU1.ACC.F[2], VU1.ACC.F[1], VU1.ACC.F[0]);
#endif
}


#ifdef PCSX2_DEVBUILD
// and not sure what these might have once been used for... (air)
//static const char *txt0 = "EAX = %x : ECX = %x : EDX = %x\n";
//static const char *txt0RC = "EAX = %x : EBX = %x : ECX = %x : EDX = %x : ESI = %x : EDI = %x\n";
//static const char *txt1 = "REG[%d] = %x_%x\n";
//static const char *txt2 = "M32 = %x\n";
#endif
////////////////////////////////////////////////////

#include "Utilities/AsciiFile.h"

// Originally from iR5900-32.cpp
void iDumpBlock( int startpc, u8 * ptr )
{
	u8 used[34];
	u8 fpuused[33];
	int numused, fpunumused;

	Console::Status( "dump1 %x:%x, %x", params startpc, pc, cpuRegs.cycle );

	g_Conf->Folders.Logs.Mkdir();
	AsciiFile eff(
		Path::Combine( g_Conf->Folders.Logs, wxsFormat(L"R5900dump%.8X.txt", startpc) ),
		wxFile::write
	);

	if( disR5900GetSym(startpc) != NULL )
	{
		eff.Printf( disR5900GetSym( startpc ) );
		eff.Printf( "\n" );
	}
	
	for ( uint i = startpc; i < s_nEndBlock; i += 4 )
	{
		string output;
		disR5900Fasm( output, memRead32( i ), i );
		eff.Printf( output.c_str() );
	}

	// write the instruction info

	eff.Printf( "\n\nlive0 - %x, live1 - %x, live2 - %x, lastuse - %x\nmmx - %x, xmm - %x, used - %x\n",
		EEINST_LIVE0, EEINST_LIVE1, EEINST_LIVE2, EEINST_LASTUSE, EEINST_MMX, EEINST_XMM, EEINST_USED
	);

	memzero_obj(used);
	numused = 0;
	for(uint i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( s_pInstCache->regs[i] & EEINST_USED ) {
			used[i] = 1;
			numused++;
		}
	}

	memzero_obj(fpuused);
	fpunumused = 0;
	for(uint i = 0; i < ArraySize(s_pInstCache->fpuregs); ++i) {
		if( s_pInstCache->fpuregs[i] & EEINST_USED ) {
			fpuused[i] = 1;
			fpunumused++;
		}
	}

	eff.Printf( "       " );
	for(uint i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( used[i] ) eff.Printf( "%2d ", i );
	}
	eff.Printf( "\n" );
	for(uint i = 0; i < ArraySize(s_pInstCache->fpuregs); ++i) {
		if( fpuused[i] ) eff.Printf( "%2d ", i );
	}

	eff.Printf( "\n" );
	eff.Printf( "       " );

	// TODO : Finish converting this over to wxWidgers wxFile stuff...
	/*
	int count;
	EEINST* pcur;

	for(uint i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( used[i] ) fprintf(f, "%s ", disRNameGPR[i]);
	}
	for(uint i = 0; i < ArraySize(s_pInstCache->fpuregs); ++i) {
		if( fpuused[i] ) fprintf(f, "%s ", i<32?"FR":"FA");
	}
	fprintf(f, "\n");

	pcur = s_pInstCache+1;
	for( uint i = 0; i < (s_nEndBlock-startpc)/4; ++i, ++pcur) {
		fprintf(f, "%2d: %2.2x ", i+1, pcur->info);
		
		count = 1;
		for(uint j = 0; j < ArraySize(s_pInstCache->regs); j++) {
			if( used[j] ) {
				fprintf(f, "%2.2x%s", pcur->regs[j], ((count%8)&&count<numused)?"_":" ");
				++count;
			}
		}
		count = 1;
		for(uint j = 0; j < ArraySize(s_pInstCache->fpuregs); j++) {
			if( fpuused[j] ) {
				fprintf(f, "%2.2x%s", pcur->fpuregs[j], ((count%8)&&count<fpunumused)?"_":" ");
				++count;
			}
		}
		fprintf(f, "\n");
	}
	fclose( f );*/
}
