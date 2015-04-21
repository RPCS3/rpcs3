#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Utilities/rFile.h"

#include "Emu/SysCalls/lv2/sys_time.h"

#include "SPUInstrTable.h"
#include "SPUDisAsm.h"

#include "SPUThread.h"
#include "SPUInterpreter.h"
#include "SPURecompiler.h"

SPURecompilerCore::SPURecompilerCore(SPUThread& cpu)
	: m_enc(new SPURecompiler(cpu, *this))
	, inter(new SPUInterpreter(cpu))
	, CPU(cpu)
	, first(true)
	, need_check(false)
{
	memset(entry, 0, sizeof(entry));
	X86CpuInfo inf;
	X86CpuUtil::detect(&inf);
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
	//const u64 stamp0 = get_system_time();
	//u64 time0 = 0;

	//SPUDisAsm dis_asm(CPUDisAsm_InterpreterMode);
	//dis_asm.offset = vm::get_ptr<u8>(CPU.offset);

	//StringLogger stringLogger;
	//stringLogger.setOption(kLoggerOptionBinaryForm, true);

	X86Compiler compiler(&runtime);
	m_enc->compiler = &compiler;
	//compiler.setLogger(&stringLogger);

	compiler.addFunc(kFuncConvHost, FuncBuilder4<u32, void*, void*, void*, u32>());
	const u16 start = pos;
	//u32 excess = 0;
	entry[start].count = 0;

	X86GpVar cpu_var(compiler, kVarTypeIntPtr, "cpu");
	compiler.setArg(0, cpu_var);
	compiler.alloc(cpu_var);
	m_enc->cpu_var = &cpu_var;

	X86GpVar ls_var(compiler, kVarTypeIntPtr, "ls");
	compiler.setArg(1, ls_var);
	compiler.alloc(ls_var);
	m_enc->ls_var = &ls_var;

	X86GpVar imm_var(compiler, kVarTypeIntPtr, "imm");
	compiler.setArg(2, imm_var);
	compiler.alloc(imm_var);
	m_enc->imm_var = &imm_var;

	X86GpVar g_imm_var(compiler, kVarTypeIntPtr, "g_imm");
	compiler.setArg(3, g_imm_var);
	compiler.alloc(g_imm_var);
	m_enc->g_imm_var = &g_imm_var;

	X86GpVar pos_var(compiler, kVarTypeUInt32, "pos");
	m_enc->pos_var = &pos_var;
	X86GpVar addr_var(compiler, kVarTypeUInt32, "addr");
	m_enc->addr = &addr_var;
	X86GpVar qw0_var(compiler, kVarTypeUInt64, "qw0");
	m_enc->qw0 = &qw0_var;
	X86GpVar qw1_var(compiler, kVarTypeUInt64, "qw1");
	m_enc->qw1 = &qw1_var;
	X86GpVar qw2_var(compiler, kVarTypeUInt64, "qw2");
	m_enc->qw2 = &qw2_var;

	for (u32 i = 0; i < 16; i++)
	{
		m_enc->xmm_var[i].data = new X86XmmVar(compiler, kX86VarTypeXmm, fmt::Format("reg_%d", i).c_str());
	}

	compiler.xor_(pos_var, pos_var);

	while (true)
	{
		const u32 opcode = vm::read32(CPU.offset + pos * 4);
		m_enc->do_finalize = false;
		if (opcode)
		{
			//const u64 stamp1 = get_system_time();
			// disasm for logging:
			//dis_asm.dump_pc = pos * 4;
			//(*SPU_instr::rrr_list)(&dis_asm, opcode);
			//compiler.addComment(fmt::Format("SPU data: PC=0x%05x %s", pos * 4, dis_asm.last_opcode.c_str()).c_str());
			// compile single opcode:
			(*SPU_instr::rrr_list)(m_enc, opcode);
			// force finalization between every slice using absolute alignment
			/*if ((pos % 128 == 127) && !m_enc->do_finalize)
			{
				compiler.mov(pos_var, pos + 1);
				m_enc->do_finalize = true;
			}*/
			entry[start].count++;
			//time0 += get_system_time() - stamp1;
		}
		else
		{
			m_enc->do_finalize = true;
		}
		bool fin = m_enc->do_finalize;
		//if (entry[pos].valid == re32(opcode))
		//{
		//	excess++;
		//}
		entry[pos].valid = re32(opcode);

		if (fin) break;
		CPU.PC += 4;
		pos++;
	}

	m_enc->XmmRelease();

	for (u32 i = 0; i < 16; i++)
	{
		assert(!m_enc->xmm_var[i].taken);
		delete m_enc->xmm_var[i].data;
		m_enc->xmm_var[i].data = nullptr;
	}

	//const u64 stamp1 = get_system_time();
	compiler.ret(pos_var);
	compiler.endFunc();
	entry[start].pointer = compiler.make();
	compiler.setLogger(nullptr); // crashes without it

	//rFile log;
	//log.Open(fmt::Format("SPUjit_%d.log", GetCurrentSPUThread().GetId()), first ? rFile::write : rFile::write_append);
	//log.Write(fmt::Format("========== START POSITION 0x%x ==========\n\n", start * 4));
	//log.Write(std::string(stringLogger.getString()));
	//if (!entry[start].pointer)
	//{
	//	LOG_ERROR(Log::SPU, "SPURecompilerCore::Compile(pos=0x%x) failed", start * sizeof(u32));
	//	log.Write("========== FAILED ============\n\n");
	//	Emu.Pause();
	//}
	//else
	//{
	//	log.Write(fmt::Format("========== COMPILED %d (excess %d), time: [start=%lld (decoding=%lld), finalize=%lld]\n\n",
	//		entry[start].count, excess, stamp1 - stamp0, time0, get_system_time() - stamp1));
	//}
	//log.Close();
	m_enc->compiler = nullptr;
	first = false;
}

u32 SPURecompilerCore::DecodeMemory(const u32 address)
{
	assert(CPU.offset == address - CPU.PC);
	const u32 m_offset = CPU.offset;
	const u16 pos = (u16)(CPU.PC >> 2);

	//ConLog.Write("DecodeMemory: pos=%d", pos);
	u32* ls = vm::get_ptr<u32>(m_offset);

	if (entry[pos].pointer)
	{
		// check data (hard way)
		bool is_valid = true;
		if (need_check)
		{
			for (u32 i = 0; i < 0x10000; i++)
			{
				if (entry[i].valid && entry[i].valid != ls[i])
				{
					is_valid = false;
					break;
				}
			}
			need_check = false;
		}
		// invalidate if necessary
		if (!is_valid)
		{
			for (u32 i = 0; i < 0x10000; i++)
			{
				if (!entry[i].pointer) continue;

				if (!entry[i].valid || entry[i].valid != ls[i] ||
					i + (u32)entry[i].count > (u32)pos &&
					i < (u32)pos + (u32)entry[pos].count)
				{
					runtime.release(entry[i].pointer);
					entry[i].pointer = nullptr;
					for (u32 j = i; j < i + (u32)entry[i].count; j++)
					{
						entry[j].valid = 0;
					}
					//need_check = true;
				}
			}
			//LOG_ERROR(Log::SPU, "SPURecompilerCore::DecodeMemory(ls=0x%x): code has changed", pos * sizeof(u32));
		}
	}

	bool did_compile = false;
	if (!entry[pos].pointer)
	{
		Compile(pos);
		did_compile = true;
		if (entry[pos].valid == 0)
		{
			LOG_ERROR(Log::SPU, "SPURecompilerCore::Compile(ls=0x%x): branch to 0x0 opcode", pos * sizeof(u32));
			Emu.Pause();
			return 0;
		}
	}

	if (!entry[pos].pointer) return 0;

	typedef u32(*Func)(const void* _cpu, const void* _ls, const void* _imm, const void* _g_imm);

	Func func = asmjit_cast<Func>(entry[pos].pointer);

	void* cpu = (u8*)&CPU.GPR[0] - offsetof(SPUThread, GPR[0]); // ugly cpu base offset detection

	//if (did_compile)
	{
		//LOG2_OPCODE("SPURecompilerCore::DecodeMemory(ls=0x%x): NewPC = 0x%llx", address, (u64)res << 2);
		//if (pos == 0x19c >> 2)
		{
			//Emu.Pause();
			//for (uint i = 0; i < 128; ++i) LOG_NOTICE(Log::SPU, "r%d = 0x%s", i, CPU.GPR[i].ToString().c_str());
		}
	}

	u32 res = pos;
	res = func(cpu, vm::get_ptr<void>(m_offset), imm_table.data(), &g_spu_imm);

	if (res & 0x1000000)
	{
		CPU.halt();
		res &= ~0x1000000;
	}

	if (res & 0x2000000)
	{
		need_check = true;
		res &= ~0x2000000;
	}

	if (did_compile)
	{
		//LOG2_OPCODE("SPURecompilerCore::DecodeMemory(ls=0x%x): NewPC = 0x%llx", address, (u64)res << 2);
		//if (pos == 0x340 >> 2)
		{
			//Emu.Pause();
			//for (uint i = 0; i < 128; ++i) LOG_NOTICE(Log::SPU, "r%d = 0x%s", i, CPU.GPR[i].ToString().c_str());
		}
	}

	if ((res - 1) == (CPU.PC >> 2))
	{
		return 4;
	}
	else
	{
		CPU.SetBranch((u64)res << 2);
		return 0;
	}
}
