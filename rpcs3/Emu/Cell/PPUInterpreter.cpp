#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/Cell/PPUDecoder.h"
#include "PPUInstrTable.h"
#include "PPUInterpreter.h"
#include "PPUInterpreter2.h"
#include "Emu/CPU/CPUThreadManager.h"

void ppu_interpreter::NULL_OP(PPUThread& CPU, ppu_opcode_t op)
{
	throw __FUNCTION__;
}

void ppu_interpreter::NOP(PPUThread& CPU, ppu_opcode_t op)
{
}


void ppu_interpreter::TDI(PPUThread& CPU, ppu_opcode_t op)
{
	throw __FUNCTION__;
}

void ppu_interpreter::TWI(PPUThread& CPU, ppu_opcode_t op)
{
	throw __FUNCTION__;
}


void ppu_interpreter::MFVSCR(PPUThread& CPU, ppu_opcode_t op)
{
	throw __FUNCTION__;
}

void ppu_interpreter::MTVSCR(PPUThread& CPU, ppu_opcode_t op)
{
}

void ppu_interpreter::VADDCUW(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._u32[w] = ~CPU.VPR[op.va]._u32[w] < CPU.VPR[op.vb]._u32[w];
	}
}

void ppu_interpreter::VADDFP(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._f[w] = CPU.VPR[op.va]._f[w] + CPU.VPR[op.vb]._f[w];
	}
}

void ppu_interpreter::VADDSBS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VADDSHS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VADDSWS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VADDUBM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VADDUBS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VADDUHM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VADDUHS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VADDUWM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VADDUWS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VAND(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VANDC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VAVGSB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VAVGSH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VAVGSW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VAVGUB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VAVGUH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VAVGUW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCFSX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCFUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPBFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPBFP_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPEQFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPEQFP_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPEQUB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPEQUB_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPEQUH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPEQUH_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPEQUW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPEQUW_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGEFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGEFP_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTFP_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTSB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTSB_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTSH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTSH_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTSW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTSW_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTUB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTUB_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTUH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTUH_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTUW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCMPGTUW_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCTSXS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VCTUXS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VEXPTEFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VLOGEFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMADDFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMAXFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMAXSB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMAXSH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMAXSW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMAXUB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMAXUH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMAXUW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMHADDSHS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMHRADDSHS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMINFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMINSB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMINSH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMINSW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMINUB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMINUH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMINUW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMLADDUHM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMRGHB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMRGHH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMRGHW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMRGLB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMRGLH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMRGLW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMSUMMBM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMSUMSHM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMSUMSHS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMSUMUBM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMSUMUHM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMSUMUHS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMULESB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMULESH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMULEUB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMULEUH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMULOSB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMULOSH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMULOUB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VMULOUH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VNMSUBFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VNOR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VOR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VPERM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VPKPX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VPKSHSS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VPKSHUS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VPKSWSS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VPKSWUS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VPKUHUM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VPKUHUS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VPKUWUM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VPKUWUS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VREFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VRFIM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VRFIN(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VRFIP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VRFIZ(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VRLB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VRLH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VRLW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VRSQRTEFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSEL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSLB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSLDOI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSLH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSLO(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSLW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSPLTB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSPLTH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSPLTISB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSPLTISH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSPLTISW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSPLTW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSRAB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSRAH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSRAW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSRB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSRH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSRO(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSRW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBCUW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBFP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBSBS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBSHS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBSWS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBUBM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBUBS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBUHM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBUHS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBUWM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUBUWS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUMSWS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUM2SWS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUM4SBS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUM4SHS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VSUM4UBS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VUPKHPX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VUPKHSB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VUPKHSH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VUPKLPX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VUPKLSB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VUPKLSH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::VXOR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MULLI(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.rd] = (s64)CPU.GPR[op.ra] * op.simm16;
}

void ppu_interpreter::SUBFIC(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	const u64 IMM = (s64)op.simm16;
	CPU.GPR[op.rd] = ~RA + IMM + 1;

	CPU.XER.CA = CPU.IsCarry(~RA, IMM, 1);
}

void ppu_interpreter::CMPLI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CMPI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ADDIC(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	CPU.GPR[op.rd] = RA + op.simm16;
	CPU.XER.CA = CPU.IsCarry(RA, op.simm16);
}

void ppu_interpreter::ADDIC_(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	CPU.GPR[op.rd] = RA + op.simm16;
	CPU.XER.CA = CPU.IsCarry(RA, op.simm16);
	CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::ADDI(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.rd] = op.ra ? ((s64)CPU.GPR[op.ra] + op.simm16) : op.simm16;
}

void ppu_interpreter::ADDIS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.rd] = op.ra ? ((s64)CPU.GPR[op.ra] + (op.simm16 << 16)) : (op.simm16 << 16);
}

void ppu_interpreter::BC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::HACK(PPUThread& CPU, ppu_opcode_t op)
{
	execute_ppu_func_by_index(CPU, op.opcode & 0x3ffffff);
}

void ppu_interpreter::SC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::B(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MCRF(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::BCLR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CRNOR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CRANDC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ISYNC(PPUThread& CPU, ppu_opcode_t op)
{
	_mm_mfence();
}

void ppu_interpreter::CRXOR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CRNAND(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CRAND(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CREQV(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CRORC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CROR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::BCCTR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::RLWIMI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::RLWINM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::RLWNM(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ORI(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] | op.uimm16;
}

void ppu_interpreter::ORIS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] | ((u64)op.uimm16 << 16);
}

void ppu_interpreter::XORI(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] ^ op.uimm16;
}

void ppu_interpreter::XORIS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] ^ ((u64)op.uimm16 << 16);
}

void ppu_interpreter::ANDI_(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] & op.uimm16;
	CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::ANDIS_(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] & ((u64)op.uimm16 << 16);
	CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::RLDICL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::RLDICR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::RLDIC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::RLDIMI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::RLDC_LR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CMP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::TW(PPUThread& CPU, ppu_opcode_t op)
{
	throw __FUNCTION__;
}

void ppu_interpreter::LVSL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LVEBX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SUBFC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MULHDU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ADDC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MULHWU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MFOCRF(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LWARX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];

	be_t<u32> value;
	vm::reservation_acquire(&value, vm::cast(addr), sizeof(value));

	CPU.GPR[op.rd] = value;
}

void ppu_interpreter::LDX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::read64(vm::cast(addr));
}

void ppu_interpreter::LWZX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::read32(vm::cast(addr));
}

void ppu_interpreter::SLW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CNTLZW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SLD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::AND(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CMPL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LVSR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LVEHX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SUBF(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LDUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DCBST(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LWZUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::CNTLZD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ANDC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::TD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LVEWX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MULHD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MULHW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LDARX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DCBF(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LBZX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LVX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::NEG(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LBZUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::NOR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STVEBX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SUBFE(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ADDE(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MTOCRF(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STDX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STWCX_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STWX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STVEHX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STDUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STWUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STVEWX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SUBFZE(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ADDZE(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STDCX_(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STBX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STVX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MULLD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SUBFME(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ADDME(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MULLW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DCBTST(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STBUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ADD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DCBT(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LHZX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::EQV(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ECIWX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LHZUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::XOR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MFSPR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LWAX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DST(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LHAX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LVXL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MFTB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LWAUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DSTST(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LHAUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STHX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ORC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ECOWX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STHUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::OR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DIVDU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DIVWU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MTSPR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DCBI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::NAND(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STVXL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DIVD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DIVW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LVLX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LDBRX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LSWX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LWBRX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LFSX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SRW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SRD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LVRX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LSWI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LFSUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SYNC(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LFDX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LFDUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STVLX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STDBRX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STSWX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STWBRX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STFSX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STVRX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STFSUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STSWI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STFDX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STFDUX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LVLXL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LHBRX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SRAW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SRAD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LVRXL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DSS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SRAWI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SRADI1(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::SRADI2(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::EIEIO(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STVLXL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STHBRX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::EXTSH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STVRXL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::EXTSB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STFIWX(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::EXTSW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::ICBI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::DCBZ(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LWZ(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LWZU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LBZ(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LBZU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STWU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STBU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LHZ(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LHZU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LHA(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LHAU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STH(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STHU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LMW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STMW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LFS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LFSU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LFD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LFDU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STFS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STFSU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STFD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STFDU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LDU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::LWA(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FDIVS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FSUBS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FADDS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FSQRTS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FRES(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FMULS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FMADDS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FMSUBS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FNMSUBS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FNMADDS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::STDU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MTFSB1(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MCRFS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MTFSB0(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MTFSFI(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MFFS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::MTFSF(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}


void ppu_interpreter::FCMPU(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FRSP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FCTIW(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FCTIWZ(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FDIV(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FSUB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FADD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FSQRT(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FSEL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FMUL(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FRSQRTE(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FMSUB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FMADD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FNMSUB(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FNMADD(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FCMPO(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FNEG(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FMR(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FNABS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FABS(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FCTID(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FCTIDZ(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::FCFID(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}


void ppu_interpreter::UNK(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}
