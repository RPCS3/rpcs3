#include "stdafx.h"
#include "SPUInstrTable.h"
#include "SPUDisAsm.h"
#include "SPUInterpreter.h"
#include "SPURecompiler.h"

#ifdef _WIN32
static const g_imm_table_struct g_imm_table;

SPURecompilerCore::SPURecompilerCore(SPUThread& cpu)
: m_enc(new SPURecompiler(cpu, *this))
, inter(new SPUInterpreter(cpu))
, CPU(cpu)
, first(true)
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
	const u64 stamp0 = get_system_time();
	u64 time0 = 0;

	SPUDisAsm dis_asm(CPUDisAsm_InterpreterMode);

	StringLogger stringLogger;
	stringLogger.setOption(kLoggerOptionBinaryForm, true);

	Compiler compiler(&runtime);
	m_enc->compiler = &compiler;
	compiler.setLogger(&stringLogger);

	compiler.addFunc(kFuncConvHost, FuncBuilder4<u32, void*, void*, void*, u32>());
	const u16 start = pos;
	u32 excess = 0;
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

	GpVar g_imm_var(compiler, kVarTypeIntPtr, "g_imm");
	compiler.setArg(3, g_imm_var);
	compiler.alloc(g_imm_var);
	m_enc->g_imm_var = &g_imm_var;

	GpVar pos_var(compiler, kVarTypeUInt32, "pos");
	m_enc->pos_var = &pos_var;
	GpVar addr_var(compiler, kVarTypeUInt32, "addr");
	m_enc->addr = &addr_var;
	GpVar qw0_var(compiler, kVarTypeUInt64, "qw0");
	m_enc->qw0 = &qw0_var;
	GpVar qw1_var(compiler, kVarTypeUInt64, "qw1");
	m_enc->qw1 = &qw1_var;
	GpVar qw2_var(compiler, kVarTypeUInt64, "qw2");
	m_enc->qw2 = &qw2_var;

	for (u32 i = 0; i < 16; i++)
	{
		m_enc->xmm_var[i].data = new XmmVar(compiler, kVarTypeXmm, fmt::Format("reg_%d", i).c_str());
	}

	compiler.xor_(pos_var, pos_var);

	while (true)
	{
		const u32 opcode = Memory.Read32(CPU.dmac.ls_offset + pos * 4);
		m_enc->do_finalize = false;
		if (opcode)
		{
			const u64 stamp1 = get_system_time();
			// disasm for logging:
			dis_asm.dump_pc = CPU.dmac.ls_offset + pos * 4;
			(*SPU_instr::rrr_list)(&dis_asm, opcode);
			compiler.addComment(fmt::Format("SPU data: PC=0x%05x %s", pos * 4, dis_asm.last_opcode.c_str()).c_str());
			// compile single opcode:
			(*SPU_instr::rrr_list)(m_enc, opcode);
			// force finalization between every slice using absolute alignment
			/*if ((pos % 128 == 127) && !m_enc->do_finalize)
			{
				compiler.mov(pos_var, pos + 1);
				m_enc->do_finalize = true;
			}*/
			entry[start].count++;
			time0 += get_system_time() - stamp1;
		}
		else
		{
			m_enc->do_finalize = true;
		}
		bool fin = m_enc->do_finalize;
		if (entry[pos].valid == re(opcode))
		{
			excess++;
		}
		entry[pos].valid = re(opcode);

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

	const u64 stamp1 = get_system_time();
	compiler.ret(pos_var);
	compiler.endFunc();
	entry[start].pointer = compiler.make();
	compiler.setLogger(nullptr); // crashes without it

	wxFile log;
	log.Open(wxString::Format("SPUjit_%d.log", GetCurrentSPUThread().GetId()), first ? wxFile::write : wxFile::write_append);
	log.Write(wxString::Format("========== START POSITION 0x%x ==========\n\n", start * 4));
	log.Write(wxString(stringLogger.getString()));
	log.Write(wxString::Format("========== COMPILED %d (excess %d), time: [start=%lld (decoding=%lld), finalize=%lld]\n\n",
		entry[start].count, excess, stamp1 - stamp0, time0, get_system_time() - stamp1));
	log.Close();
	m_enc->compiler = nullptr;
	first = false;
}

u8 SPURecompilerCore::DecodeMemory(const u64 address)
{
	assert(CPU.dmac.ls_offset == address - CPU.PC);
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
		/*for (u32 i = pos; i < (u32)(entry[pos].count + pos); i++)
		{
			if (entry[i].valid != ls[i])
			{
				is_valid = false;
				break;
			}
		}*/
		// invalidate if necessary
		if (!is_valid)
		{
			// TODO
			ConLog.Error("SPURecompilerCore::DecodeMemory(ls_addr=0x%x): code has changed", pos * sizeof(u32));
			Emu.Pause();
			return 0;
		}
	}

	bool did_compile = false;
	if (!entry[pos].pointer)
	{
		Compile(pos);
		did_compile = true;
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

	typedef u32(*Func)(const void* _cpu, const void* _ls, const void* _imm, const void* _g_imm);

	Func func = asmjit_cast<Func>(entry[pos].pointer);

	void* cpu = (u8*)&CPU.GPR[0] - offsetof(SPUThread, GPR[0]); // ugly cpu base offset detection

	//if (did_compile)
	{
		//LOG2_OPCODE("SPURecompilerCore::DecodeMemory(ls_addr=0x%x): NewPC = 0x%llx", address, (u64)res << 2);
		//if (pos == 0x19c >> 2)
		{
			//Emu.Pause();
			//for (uint i = 0; i < 128; ++i) ConLog.Write("r%d = 0x%s", i, CPU.GPR[i].ToString().c_str());
		}
	}

	u32 res = pos;
	res = func(cpu, &Memory[m_offset], imm_table.data(), &g_imm_table);

	if (res > 0xffff)
	{
		CPU.Stop();
		res = ~res;
	}

	if (did_compile)
	{
		//LOG2_OPCODE("SPURecompilerCore::DecodeMemory(ls_addr=0x%x): NewPC = 0x%llx", address, (u64)res << 2);
		//if (pos == 0x340 >> 2)
		{
			//Emu.Pause();
			//for (uint i = 0; i < 128; ++i) ConLog.Write("r%d = 0x%s", i, CPU.GPR[i].ToString().c_str());
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
#endif
