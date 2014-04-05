#include "stdafx.h"
#include "SPUInstrTable.h"
#include "SPUInterpreter.h"
#include "SPURecompiler.h"

static const SPUImmTable g_spu_imm;

SPURecompilerCore::SPURecompilerCore(SPUThread& cpu)
: m_enc(new SPURecompiler(cpu, *this))
, m_inter(new SPUInterpreter(cpu))
, CPU(cpu)
, compiler(&runtime)
{
	memset(entry, 0, sizeof(entry));
}

SPURecompilerCore::~SPURecompilerCore()
{
	delete m_enc;
	delete m_inter;
}

void SPURecompilerCore::Decode(const u32 code) // decode instruction and run with interpreter
{
	(*SPU_instr::rrr_list)(m_inter, code);
}

void SPURecompilerCore::Compile(u16 pos)
{
	compiler.addFunc(kFuncConvHost, FuncBuilder4<u16, void*, void*, void*, u16>());
	entry[pos].host = pos;

	GpVar cpu_var(compiler, kVarTypeIntPtr, "cpu");
	compiler.setArg(0, cpu_var);
	compiler.alloc(cpu_var);
	m_enc->cpu_var = &cpu_var;

	GpVar ls_var(compiler, kVarTypeIntPtr, "ls");
	compiler.setArg(1, ls_var);
	compiler.alloc(ls_var);
	m_enc->ls_var = &ls_var;

	GpVar imm_var(compiler, kVarTypeIntPtr, "imm");
	compiler.setArg(2, imm_var);
	compiler.alloc(imm_var);
	m_enc->imm_var = &imm_var;

	GpVar pos_var(compiler, kVarTypeUInt16, "pos");
	compiler.setArg(3, pos_var);
	compiler.alloc(pos_var);

	while (true)
	{
		const u32 opcode = Memory.Read32(CPU.dmac.ls_offset + pos * 4);
		m_enc->do_finalize = false;
		(*SPU_instr::rrr_list)(m_enc, opcode); // compile single opcode
		bool fin = m_enc->do_finalize;
		entry[pos].valid = opcode;

		if (fin) break;
		CPU.PC += 4;
		pos++;
		entry[pos].host = entry[pos - 1].host;
	}

	compiler.xor_(pos_var, pos_var);
	compiler.ret(pos_var);
	compiler.endFunc();
	entry[entry[pos].host].pointer = compiler.make();
}

u8 SPURecompilerCore::DecodeMemory(const u64 address)
{
	const u64 m_offset = address - CPU.PC;
	const u16 pos = (CPU.PC >> 2);

	u32* ls = (u32*)Memory.VirtualToRealAddr(m_offset);

	if (!pos)
	{
		ConLog.Error("SPURecompilerCore::DecodeMemory(): ls_addr = 0");
		Emu.Pause();
		return 0;
	}

	if (entry[pos].pointer)
	{
		// check data (hard way)
		bool is_valid = true;
		for (u32 i = pos; i < entry[pos].count + pos; i++)
		{
			if (entry[i].valid != ls[i])
			{
				is_valid = false;
				break;
			}
		}
		// invalidate if necessary
		if (!is_valid)
		{
			// TODO
		}
	}

	if (!entry[pos].pointer)
	{
		// compile from current position to nearest dynamic or statically unresolved branch, zero data or something other
		Compile(pos);
	}

	if (!entry[pos].pointer)
	{
		ConLog.Error("SPURecompilerCore::DecodeMemory(ls_addr=0x%x): compilation failed", pos * sizeof(u32));
		Emu.Pause();
		return 0;
	}
	// jump
	typedef u16(*Func)(void* _cpu, void* _ls, const SPUImmTable* _imm, u16 _pos);

	Func func = asmjit_cast<Func>(entry[entry[pos].host].pointer);

	void* cpu = (u8*)&CPU.GPR[0] - offsetof(SPUThread, GPR[0]); // ugly cpu base offset detection

	u16 res = pos == entry[pos].host ? 0 : pos;
	res = func(cpu, ls, &g_spu_imm, res);

	ConLog.Write("func -> %d", res);

	return 0;
	/*Decode(Memory.Read32(address));
	return 4;*/
}