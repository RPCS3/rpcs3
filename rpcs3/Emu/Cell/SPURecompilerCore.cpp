#include "stdafx.h"
#include "SPUInstrTable.h"
#include "SPUInterpreter.h"
#include "SPURecompiler.h"

static const SPUImmTable g_spu_imm;

SPURecompilerCore::SPURecompilerCore(SPUThread& cpu)
: m_enc(new SPURecompiler(cpu, *this))
, inter(new SPUInterpreter(cpu))
, CPU(cpu)
, compiler(&runtime)
{
	memset(entry, 0, sizeof(entry));
}

SPURecompilerCore::~SPURecompilerCore()
{
	delete m_enc;
	delete inter;
}

void SPURecompilerCore::Decode(const u32 code) // decode instruction and run with interpreter
{
	(*SPU_instr::rrr_list)(inter, code);
}

void SPURecompilerCore::Compile(u16 pos)
{
	compiler.addFunc(kFuncConvHost, FuncBuilder4<u32, void*, void*, void*, u32>());
	const u16 start = pos;
	entry[start].count = 0;

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

	GpVar pos_var(compiler, kVarTypeUInt32, "pos");
	compiler.setArg(3, pos_var);
	compiler.alloc(pos_var);

	m_enc->pos_var = &pos_var;

	compiler.xor_(pos_var, pos_var);

	while (true)
	{
		const u32 opcode = Memory.Read32(CPU.dmac.ls_offset + pos * 4);
		m_enc->do_finalize = false;
		if (opcode)
		{
			(*SPU_instr::rrr_list)(m_enc, opcode); // compile single opcode
			entry[start].count++;
		}
		else
		{
			m_enc->do_finalize = true;
		}
		bool fin = m_enc->do_finalize;
		entry[pos].valid = re(opcode);

		if (fin) break;
		CPU.PC += 4;
		pos++;
	}

	compiler.ret(pos_var);
	compiler.endFunc();
	entry[start].pointer = compiler.make();
}

u8 SPURecompilerCore::DecodeMemory(const u64 address)
{
	const u64 m_offset = CPU.dmac.ls_offset;
	const u16 pos = (CPU.PC >> 2);

	//ConLog.Write("DecodeMemory: pos=%d", pos);
	u32* ls = (u32*)&Memory[m_offset];

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
		for (u32 i = pos; i < (u32)(entry[pos].count + pos); i++)
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
			ConLog.Error("SPURecompilerCore::DecodeMemory(ls_addr=0x%x): code has changed", pos * sizeof(u32));
			Emu.Pause();
			return 0;
		}
	}

	if (!entry[pos].pointer)
	{
		// compile from current position to nearest dynamic or statically unresolved branch, zero data or something other
		Compile(pos);
		if (entry[pos].valid == 0)
		{
			ConLog.Error("SPURecompilerCore::Compile(ls_addr=0x%x): branch to 0x0 opcode", pos * sizeof(u32));
			Emu.Pause();
			return 0;
		}
	}

	if (!entry[pos].pointer)
	{
		ConLog.Error("SPURecompilerCore::DecodeMemory(ls_addr=0x%x): compilation failed", pos * sizeof(u32));
		Emu.Pause();
		return 0;
	}
	// jump
	typedef u32(*Func)(void* _cpu, void* _ls, const SPUImmTable* _imm, u32 _pos);

	Func func = asmjit_cast<Func>(entry[pos].pointer);

	void* cpu = (u8*)&CPU.GPR[0] - offsetof(SPUThread, GPR[0]); // ugly cpu base offset detection

	u16 res = pos;
	res = (u16)func(cpu, &Memory[m_offset], &g_spu_imm, res);

	LOG2_OPCODE("SPURecompilerCore::DecodeMemory(ls_addr=0x%x): NewPC = 0x%llx", address, (u64)res << 2);
	if ((res - 1) == (CPU.PC >> 2))
	{
		return 4;
	}
	else
	{
		CPU.SetBranch((u64)res << 2);
		return 0;
	}
	/*Decode(Memory.Read32(address));
	return 4;*/
}