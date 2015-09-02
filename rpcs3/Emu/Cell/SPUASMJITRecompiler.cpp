#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/File.h"
#include "Emu/System.h"

#include "SPUDisAsm.h"
#include "SPUThread.h"
#include "SPUInterpreter.h"
#include "SPUASMJITRecompiler.h"

#define ASMJIT_STATIC
#define ASMJIT_DEBUG

#include "asmjit.h"

#define OFFSET_OF(type, x) static_cast<s32>(reinterpret_cast<uintptr_t>(&(((type*)0)->x)))

#define SPU_OFF_128(x) asmjit::host::oword_ptr(*cpu, OFFSET_OF(SPUThread, x))
#define SPU_OFF_64(x) asmjit::host::qword_ptr(*cpu, OFFSET_OF(SPUThread, x))
#define SPU_OFF_32(x) asmjit::host::dword_ptr(*cpu, OFFSET_OF(SPUThread, x))
#define SPU_OFF_16(x) asmjit::host::word_ptr(*cpu, OFFSET_OF(SPUThread, x))
#define SPU_OFF_8(x) asmjit::host::byte_ptr(*cpu, OFFSET_OF(SPUThread, x))

spu_recompiler::spu_recompiler()
	: m_jit(std::make_shared<asmjit::JitRuntime>())
{
	asmjit::X86CpuInfo inf;
	asmjit::X86CpuUtil::detect(&inf);

	LOG_SUCCESS(SPU, "SPU Recompiler (ASMJIT) created...");

	const std::string str = fmt::format("SPU JIT initialization...\n\nTitle: %s\nTitle ID: %s\n\n", Emu.GetTitle().c_str(), Emu.GetTitleID().c_str());

	fs::file("SPUJIT.log", fom::write | fom::create | fom::trunc).write(str.c_str(), str.size());
}

void spu_recompiler::compile(spu_function_t& f)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (f.compiled)
	{
		// return if function already compiled
		return;
	}

	if (f.addr >= 0x40000 || f.addr % 4 || f.size == 0 || f.size >= 0x40000 - f.addr || f.size % 4)
	{
		// function shouldn't touch the greatest possible address (0x3fffc) as well
		throw EXCEPTION("Invalid SPU function (addr=0x%05x, size=0x%x)", f.addr, f.size);
	}

	using namespace asmjit;

	SPUDisAsm dis_asm(CPUDisAsm_InterpreterMode);
	dis_asm.offset = reinterpret_cast<u8*>(f.data.data()) - f.addr;

	StringLogger logger;
	logger.setOption(kLoggerOptionBinaryForm, true);

	std::string log = fmt::format("========== SPU FUNCTION 0x%05x - 0x%05x ==========\n\n", f.addr, f.addr + f.size);

	this->m_func = &f;

	X86Compiler compiler(m_jit.get());
	this->c = &compiler;
	compiler.setLogger(&logger);

	compiler.addFunc(kFuncConvHost, FuncBuilder2<u32, void*, void*>());

	X86GpVar cpu_var(compiler, kVarTypeIntPtr, "cpu");
	compiler.setArg(0, cpu_var);
	compiler.alloc(cpu_var); // ???
	this->cpu = &cpu_var;

	X86GpVar ls_var(compiler, kVarTypeIntPtr, "ls");
	compiler.setArg(1, ls_var);
	compiler.alloc(ls_var); // ???
	this->ls = &ls_var;

	X86GpVar pos_var(compiler, kVarTypeUInt32, "pos");
	this->pos = &pos_var;
	X86GpVar addr_var(compiler, kVarTypeUInt32, "addr");
	this->addr = &addr_var;
	X86GpVar qw0_var(compiler, kVarTypeUInt64, "qw0");
	this->qw0 = &qw0_var;
	X86GpVar qw1_var(compiler, kVarTypeUInt64, "qw1");
	this->qw1 = &qw1_var;
	X86GpVar qw2_var(compiler, kVarTypeUInt64, "qw2");
	this->qw2 = &qw2_var;

	std::array<X86XmmVar, 10> vec_vars;

	for (u32 i = 0; i < vec_vars.size(); i++)
	{
		vec_vars[i] = X86XmmVar{ compiler, kX86VarTypeXmm, fmt::format("vec%d", i).c_str() };
		vec.at(i) = vec_vars.data() + i;
	}

	std::vector<Label> pos_labels{ 0x10000 };

	this->labels = pos_labels.data();

	// Initialize labels
	for (const u32 addr : f.blocks)
	{
		if (addr < f.addr || addr >= f.addr + f.size || addr % 4)
		{
			throw EXCEPTION("Invalid function block entry (0x%05x)", addr);
		}

		pos_labels[addr / 4] = Label{ compiler };
	}

	pos_labels[(f.addr + f.size) / 4] = Label{ compiler };

	Label jt_label{ compiler };
	this->jt = &jt_label;

	m_pos = f.addr;

	for (const u32 op : f.data)
	{
		// Disasm
		dis_asm.dump_pc = m_pos;
		dis_asm.do_disasm(op);

		// Bind label if initialized
		if (pos_labels[m_pos / 4].isInitialized())
		{
			compiler.bind(pos_labels[m_pos / 4]);
			compiler.addComment("Block:");
		}

		compiler.addComment(dis_asm.last_opcode.c_str());
		log += dis_asm.last_opcode.c_str();
		log += '\n';

		(this->*spu_recompiler::opcodes[op])({ op });

		// Set next position
		m_pos += 4;
	}

	log += '\n';

	// Generate default function end (go to the next address)
	compiler.bind(pos_labels.at(m_pos / 4));
	compiler.addComment("Fallthrough:");
	compiler.mov(pos_var, m_pos);
	compiler.ret(pos_var);

	// Generate jump table resolver (uses pos_var)
	compiler.bind(jt_label);

	if (f.jtable.size())
	{
		compiler.addComment("Jump table resolver:");
		compiler.mov(addr_var, pos_var);
		compiler.and_(addr_var, 0x3fffc);
	}

	for (const u32 addr : f.jtable)
	{
		if (pos_labels[addr / 4].isInitialized())
		{
			// It could be binary search or something
			compiler.cmp(addr_var, addr);
			compiler.je(pos_labels[addr / 4]);
		}
		else
		{
			LOG_ERROR(SPU, "Unable to add jump table entry (0x%05x)", addr);
		}
	}

	compiler.ret(pos_var);

	// Finalization
	compiler.endFunc();

	// Append log file
	fs::file log_file("SPUJIT.log", fom::write | fom::append | fom::create);

	// Write log header
	log_file.write(log.c_str(), log.size());

	// Compile and store function address
	f.compiled = asmjit_cast<spu_jit_func_t>(compiler.make());

	logger.logString(kLoggerStyleDefault, "\n\n\n");

	// Write ASMJIT logs
	const auto log_data = logger.getString(); log_file.write(log_data, std::strlen(log_data));
}

spu_recompiler::XmmLink spu_recompiler::XmmAlloc() // get empty xmm register
{
	for (auto& v : vec)
	{
		if (v)
		{
			return XmmLink{ v };
		}
	}

	throw EXCEPTION("Out of Xmm Vars");
}

spu_recompiler::XmmLink spu_recompiler::XmmGet(s8 reg, XmmType type) // get xmm register with specific SPU reg
{
	XmmLink result = XmmAlloc();

	switch (type)
	{
	case XmmType::Int: c->movdqa(result, SPU_OFF_128(gpr[reg])); break;
	case XmmType::Float: c->movaps(result, SPU_OFF_128(gpr[reg])); break;
	case XmmType::Double: c->movapd(result, SPU_OFF_128(gpr[reg])); break;
	default: throw EXCEPTION("Invalid XmmType");
	}

	return result;
}

inline asmjit::X86Mem spu_recompiler::XmmConst(v128 data)
{
	return c->newXmmConst(asmjit::kConstScopeLocal, asmjit::Vec128::fromUq(data._u64[0], data._u64[1]));
}

inline asmjit::X86Mem spu_recompiler::XmmConst(__m128 data)
{
	return XmmConst(v128::fromF(data));
}

inline asmjit::X86Mem spu_recompiler::XmmConst(__m128i data)
{
	return XmmConst(v128::fromV(data));
}

void spu_recompiler::InterpreterCall(spu_opcode_t op)
{
	auto gate = [](SPUThread* _spu, u32 opcode, spu_inter_func_t _func) -> u32
	{
		try
		{
			// TODO: check correctness

			const u32 old_pc = _spu->pc;

			_func(*_spu, { opcode });

			if (old_pc != _spu->pc)
			{
				_spu->pc += 4;
				return 2;
			}

			_spu->pc += 4;
			return 0;
		}
		catch (...)
		{
			_spu->pending_exception = std::current_exception();
			return 1;
		}
	};

	c->mov(SPU_OFF_32(pc), m_pos);
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, u32(SPUThread*, u32, spu_inter_func_t)>(gate)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<u32, void*, u32, void*>());
	call->setArg(0, *cpu);
	call->setArg(1, asmjit::imm_u(op.opcode));
	call->setArg(2, asmjit::imm_ptr(asmjit_cast<void*>(spu_interpreter::fast::g_spu_opcode_table[op.opcode])));
	call->setRet(0, *addr);

	// return immediately if an error occured
	asmjit::Label next(*c);
	c->test(*addr, *addr);
	c->jz(next);
	c->mov(*pos, SPU_OFF_32(pc));
	c->ret(*pos);
	c->bind(next);
}

void spu_recompiler::FunctionCall()
{
	auto gate = [](SPUThread* _spu, u32 link) -> u32
	{
		try
		{
			if (_spu->pc == link)
			{
				LOG_ERROR(SPU, "Branch-to-next (pc=0x%05x)", _spu->pc);
			}
			else if (_spu->pc == link - 4)
			{
				LOG_ERROR(SPU, "Branch-to-self (pc=0x%05x)", _spu->pc);
			}

			// TODO: analyse stack consistency
			// TODO: check correctness

			while (!_spu->m_state.load() || !_spu->check_status())
			{
				// try to use non-virtual call since the type is known
				static_cast<SPURecompilerDecoder&>(*_spu->m_dec).DecodeMemory(_spu->offset + _spu->pc);

				if (_spu->pc == link)
				{
					// returned successfully
					return 0;
				}
			}

			return 2;
		}
		catch (...)
		{
			_spu->pending_exception = std::current_exception();
			return 1;
		}
	};

	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, u32(SPUThread*, u32)>(gate)), asmjit::kFuncConvHost, asmjit::FuncBuilder2<u32, SPUThread*, u32>());
	call->setArg(0, *cpu);
	call->setArg(1, asmjit::imm_u(m_pos + 4));
	call->setRet(0, *addr);

	// return immediately if an error occured
	asmjit::Label next(*c);
	c->test(*addr, *addr);
	c->jz(next);
	c->mov(*pos, SPU_OFF_32(pc));
	c->ret(*pos);
	c->bind(next);

	//asmjit::Label next2(*c);
	//c->cmp(SPU_OFF_32(pc), m_pos + 4); // compare current PC with next position
	//c->je(next2);
	//c->mov(*pos, SPU_OFF_32(pc));
	//c->jmp(*jt); // go to the jump table resolver
	//c->bind(next2);
}

void spu_recompiler::STOP(spu_opcode_t op)
{
	InterpreterCall(op); // TODO
}

void spu_recompiler::LNOP(spu_opcode_t op)
{
}

void spu_recompiler::SYNC(spu_opcode_t op)
{
	// This instruction must be used following a store instruction that modifies the instruction stream.
	c->mfence();
}

void spu_recompiler::DSYNC(spu_opcode_t op)
{
	// This instruction forces all earlier load, store, and channel instructions to complete before proceeding.
	c->mfence();
}

void spu_recompiler::MFSPR(spu_opcode_t op)
{
	InterpreterCall(op);
}

void spu_recompiler::RDCH(spu_opcode_t op)
{
	InterpreterCall(op); // TODO
}

void spu_recompiler::RCHCNT(spu_opcode_t op)
{
	InterpreterCall(op); // TODO
}

void spu_recompiler::SF(spu_opcode_t op)
{
	// sub from
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psubd(vb, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::OR(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->por(vb, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::BG(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from32p(0x80000000)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr[op.rb]));
	c->pcmpgtd(va, vi);
	c->paddd(va, XmmConst(v128::from32p(1)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::SFH(spu_opcode_t op)
{
	// sub from (halfword)
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psubw(vb, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::NOR(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->por(va, SPU_OFF_128(gpr[op.rb]));
	c->pxor(va, XmmConst(v128::from32p(0xffffffff)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ABSDB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vm = XmmAlloc();
	c->movdqa(vm, va);
	c->pmaxub(va, vb);
	c->pminub(vb, vm);
	c->psubb(va, vb);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ROT(spu_opcode_t op)
{
	auto body = [](u32* t, const u32* a, const s32* b) noexcept
	{
		for (u32 i = 0; i < 4; i++)
		{
			t[i] = (a[i] << b[i]) | (a[i] >> (32 - b[i]));
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	c->lea(*qw2, SPU_OFF_128(gpr[op.rb]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(u32*, const u32*, const s32*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<void, void*, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 4; i++) // unrolled loop
	//{
	//	c->mov(qw0->r32(), SPU_OFF_32(gpr[op.ra]._u32[i]));
	//	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[i]));
	//	c->rol(qw0->r32(), *addr);
	//	c->mov(SPU_OFF_32(gpr[op.rt]._u32[i]), qw0->r32());
	//}
}

void spu_recompiler::ROTM(spu_opcode_t op)
{
	auto body = [](u32* t, const u32* a, const u32* b) noexcept
	{
		for (u32 i = 0; i < 4; i++)
		{
			t[i] = static_cast<u32>(static_cast<u64>(a[i]) >> (0 - b[i]));
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	c->lea(*qw2, SPU_OFF_128(gpr[op.rb]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(u32*, const u32*, const u32*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<void, void*, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 4; i++) // unrolled loop
	//{
	//	c->mov(qw0->r32(), SPU_OFF_32(gpr[op.ra]._u32[i]));
	//	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[i]));
	//	c->neg(*addr);
	//	c->shr(*qw0, *addr);
	//	c->mov(SPU_OFF_32(gpr[op.rt]._u32[i]), qw0->r32());
	//}
}

void spu_recompiler::ROTMA(spu_opcode_t op)
{
	auto body = [](s32* t, const s32* a, const u32* b) noexcept
	{
		for (u32 i = 0; i < 4; i++)
		{
			t[i] = static_cast<s32>(static_cast<s64>(a[i]) >> (0 - b[i]));
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	c->lea(*qw2, SPU_OFF_128(gpr[op.rb]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(s32*, const s32*, const u32*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<void, void*, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 4; i++) // unrolled loop
	//{
	//	c->movsxd(*qw0, SPU_OFF_32(gpr[op.ra]._u32[i]));
	//	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[i]));
	//	c->neg(*addr);
	//	c->sar(*qw0, *addr);
	//	c->mov(SPU_OFF_32(gpr[op.rt]._u32[i]), qw0->r32());
	//}
}

void spu_recompiler::SHL(spu_opcode_t op)
{
	auto body = [](u32* t, const u32* a, const u32* b) noexcept
	{
		for (u32 i = 0; i < 4; i++)
		{
			t[i] = static_cast<u32>(static_cast<u64>(a[i]) << b[i]);
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	c->lea(*qw2, SPU_OFF_128(gpr[op.rb]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(u32*, const u32*, const u32*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<void, void*, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 4; i++) // unrolled loop
	//{
	//	c->mov(qw0->r32(), SPU_OFF_32(gpr[op.ra]._u32[i]));
	//	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[i]));
	//	c->shl(*qw0, *addr);
	//	c->mov(SPU_OFF_32(gpr[op.rt]._u32[i]), qw0->r32());
	//}
}

void spu_recompiler::ROTH(spu_opcode_t op) //nf
{
	auto body = [](u16* t, const u16* a, const s16* b) noexcept
	{
		for (u32 i = 0; i < 8; i++)
		{
			t[i] = (a[i] << b[i]) | (a[i] >> (16 - b[i]));
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	c->lea(*qw2, SPU_OFF_128(gpr[op.rb]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(u16*, const u16*, const s16*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<void, void*, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 8; i++) // unrolled loop
	//{
	//	c->movzx(qw0->r32(), SPU_OFF_16(gpr[op.ra]._u16[i]));
	//	c->movzx(*addr, SPU_OFF_16(gpr[op.rb]._u16[i]));
	//	c->rol(qw0->r16(), *addr);
	//	c->mov(SPU_OFF_16(gpr[op.rt]._u16[i]), qw0->r16());
	//}
}

void spu_recompiler::ROTHM(spu_opcode_t op)
{
	auto body = [](u16* t, const u16* a, const u16* b) noexcept
	{
		for (u32 i = 0; i < 8; i++)
		{
			t[i] = static_cast<u16>(static_cast<u32>(a[i]) >> (0 - b[i]));
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	c->lea(*qw2, SPU_OFF_128(gpr[op.rb]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(u16*, const u16*, const u16*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<void, void*, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 8; i++) // unrolled loop
	//{
	//	c->movzx(qw0->r32(), SPU_OFF_16(gpr[op.ra]._u16[i]));
	//	c->movzx(*addr, SPU_OFF_16(gpr[op.rb]._u16[i]));
	//	c->neg(*addr);
	//	c->shr(qw0->r32(), *addr);
	//	c->mov(SPU_OFF_16(gpr[op.rt]._u16[i]), qw0->r16());
	//}
}

void spu_recompiler::ROTMAH(spu_opcode_t op)
{
	auto body = [](s16* t, const s16* a, const u16* b) noexcept
	{
		for (u32 i = 0; i < 8; i++)
		{
			t[i] = static_cast<s16>(static_cast<s32>(a[i]) >> (0 - b[i]));
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	c->lea(*qw2, SPU_OFF_128(gpr[op.rb]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(s16*, const s16*, const u16*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<void, void*, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 8; i++) // unrolled loop
	//{
	//	c->movsx(qw0->r32(), SPU_OFF_16(gpr[op.ra]._u16[i]));
	//	c->movzx(*addr, SPU_OFF_16(gpr[op.rb]._u16[i]));
	//	c->neg(*addr);
	//	c->sar(qw0->r32(), *addr);
	//	c->mov(SPU_OFF_16(gpr[op.rt]._u16[i]), qw0->r16());
	//}
}

void spu_recompiler::SHLH(spu_opcode_t op)
{
	auto body = [](u16* t, const u16* a, const u16* b) noexcept
	{
		for (u32 i = 0; i < 8; i++)
		{
			t[i] = static_cast<u16>(static_cast<u32>(a[i]) << b[i]);
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	c->lea(*qw2, SPU_OFF_128(gpr[op.rb]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(u16*, const u16*, const u16*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<void, void*, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 8; i++) // unrolled loop
	//{
	//	c->movzx(qw0->r32(), SPU_OFF_16(gpr[op.ra]._u16[i]));
	//	c->movzx(*addr, SPU_OFF_16(gpr[op.rb]._u16[i]));
	//	c->shl(qw0->r32(), *addr);
	//	c->mov(SPU_OFF_16(gpr[op.rt]._u16[i]), qw0->r16());
	//}
}

void spu_recompiler::ROTI(spu_opcode_t op)
{
	// rotate left
	const int s = op.si7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	c->movdqa(v1, va);
	c->pslld(va, s);
	c->psrld(v1, 32 - s);
	c->por(va, v1);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ROTMI(spu_opcode_t op)
{
	// shift right logical
	const int s = (0 - op.si7) & 0x3f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrld(va, s);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ROTMAI(spu_opcode_t op)
{
	// shift right arithmetical
	const int s = (0 - op.si7) & 0x3f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrad(va, s);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::SHLI(spu_opcode_t op)
{
	// shift left
	const int s = op.si7 & 0x3f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pslld(va, s);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ROTHI(spu_opcode_t op)
{
	// rotate left (halfword)
	const int s = op.si7 & 0xf;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	c->movdqa(v1, va);
	c->psllw(va, s);
	c->psrlw(v1, 16 - s);
	c->por(va, v1);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ROTHMI(spu_opcode_t op)
{
	// shift right logical
	const int s = (0 - op.si7) & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrlw(va, s);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ROTMAHI(spu_opcode_t op)
{
	// shift right arithmetical (halfword)
	const int s = (0 - op.si7) & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psraw(va, s);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::SHLHI(spu_opcode_t op)
{
	// shift left (halfword)
	const int s = op.si7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psllw(va, s);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::A(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->paddd(vb, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::AND(spu_opcode_t op)
{
	// and
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pand(vb, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::CG(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from32p(0x80000000)));
	c->paddd(vb, va);
	c->pxor(va, vi);
	c->pxor(vb, vi);
	c->pcmpgtd(va, vb);
	c->psrld(va, 31);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::AH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->paddw(va, SPU_OFF_128(gpr[op.rb]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::NAND(spu_opcode_t op)
{
	// nand
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, SPU_OFF_128(gpr[op.rb]));
	c->pxor(va, XmmConst(v128::from32p(0xffffffff)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::AVGB(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pavgb(vb, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::MTSPR(spu_opcode_t op)
{
	InterpreterCall(op);
}

void spu_recompiler::WRCH(spu_opcode_t op)
{
	InterpreterCall(op); // TODO
}

void spu_recompiler::BIZ(spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw EXCEPTION("TODO: enable/disable interrupts");
	}

	asmjit::Label next(*c);

	c->cmp(SPU_OFF_32(gpr[op.rt]._u32[3]), 0);
	c->jne(next);

	c->mov(*pos, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->and_(*pos, 0x3fffc);
	//c->or_(*pos, u32{ op.ra != 0 } << 25 | u32{ op.e } << 26 | u32{ op.d } << 27);
	c->jmp(*jt);
	c->bind(next);
}

void spu_recompiler::BINZ(spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw EXCEPTION("TODO: enable/disable interrupts");
	}

	asmjit::Label next(*c);

	c->cmp(SPU_OFF_32(gpr[op.rt]._u32[3]), 0);
	c->je(next);

	c->mov(*pos, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->and_(*pos, 0x3fffc);
	//c->or_(*pos, u32{ op.ra != 0 } << 25 | u32{ op.e } << 26 | u32{ op.d } << 27);
	c->jmp(*jt);
	c->bind(next);
}

void spu_recompiler::BIHZ(spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw EXCEPTION("TODO: enable/disable interrupts");
	}
	
	asmjit::Label next(*c);

	c->cmp(SPU_OFF_16(gpr[op.rt]._u16[6]), 0);
	c->jne(next);

	c->mov(*pos, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->and_(*pos, 0x3fffc);
	//c->or_(*pos, u32{ op.ra != 0 } << 25 | u32{ op.e } << 26 | u32{ op.d } << 27);
	c->jmp(*jt);
	c->bind(next);
}

void spu_recompiler::BIHNZ(spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw EXCEPTION("TODO: enable/disable interrupts");
	}

	asmjit::Label next(*c);

	c->cmp(SPU_OFF_16(gpr[op.rt]._u16[6]), 0);
	c->je(next);

	c->mov(*pos, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->and_(*pos, 0x3fffc);
	//c->or_(*pos, u32{ op.ra != 0 } << 25 | u32{ op.e } << 26 | u32{ op.d } << 27);
	c->jmp(*jt);
	c->bind(next);
}

void spu_recompiler::STOPD(spu_opcode_t op)
{
	InterpreterCall(op);
}

void spu_recompiler::STQX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->add(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->and_(*addr, 0x3fff0);

	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(asmjit::host::oword_ptr(*ls, *addr), vt);
}

void spu_recompiler::BI(spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw EXCEPTION("TODO: enable/disable interrupts");
	}

	c->mov(*pos, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->and_(*pos, 0x3fffc);
	//c->or_(*pos, u32{ op.ra != 0 } << 25 | u32{ op.e } << 26 | u32{ op.d } << 27);
	c->jmp(*jt);
}

void spu_recompiler::BISL(spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw EXCEPTION("TODO: enable/disable interrupts");
	}

	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->and_(*addr, 0x3fffc);
	c->mov(SPU_OFF_32(pc), *addr);
	//c->or_(*pos, 0x2000000 | u32{ op.e } << 26 | u32{ op.d } << 27);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32(0, 0, 0, m_pos + 4)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	FunctionCall();
}

void spu_recompiler::IRET(spu_opcode_t op)
{
	throw EXCEPTION("Unexpected instruction");
}

void spu_recompiler::BISLED(spu_opcode_t op)
{
	throw EXCEPTION("Unexpected instruction");
}

void spu_recompiler::HBR(spu_opcode_t op)
{
}

void spu_recompiler::GB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pshufb(va, XmmConst(_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0)));
	c->psllq(va, 7);
	c->pmovmskb(*addr, va);
	c->pxor(va, va);
	c->pinsrw(va, *addr, 6);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::GBH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pshufb(va, XmmConst(_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 14, 12, 10, 8, 6, 4, 2, 0)));
	c->psllq(va, 7);
	c->pmovmskb(*addr, va);
	c->pxor(va, va);
	c->pinsrw(va, *addr, 6);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::GBB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psllq(va, 7);
	c->pmovmskb(*addr, va);
	c->pxor(va, va);
	c->pinsrw(va, *addr, 6);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::FSM(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.fsm));
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->and_(*addr, 0xf);
	c->shl(*addr, 4);
	c->movdqa(vr, asmjit::host::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::FSMH(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.fsmh));
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->and_(*addr, 0xff);
	c->shl(*addr, 4);
	c->movdqa(vr, asmjit::host::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::FSMB(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.fsmb));
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->and_(*addr, 0xffff);
	c->shl(*addr, 4);
	c->movdqa(vr, asmjit::host::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::FREST(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->rcpps(va, va);
	c->movaps(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::FRSQEST(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->andps(va, XmmConst(v128::from32p(0x7fffffff))); // abs
	c->rsqrtps(va, va);
	c->movaps(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::LQX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->add(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->and_(*addr, 0x3fff0);

	const XmmLink& vt = XmmAlloc();
	c->movdqa(vt, asmjit::host::oword_ptr(*ls, *addr));
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vt);
}

void spu_recompiler::ROTQBYBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.rldq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->and_(*addr, 0xf << 3);
	c->shl(*addr, 1);
	c->pshufb(va, asmjit::host::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ROTQMBYBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.srdq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->shr(*addr, 3);
	c->neg(*addr);
	c->and_(*addr, 0x1f);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::host::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::SHLQBYBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.sldq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->and_(*addr, 0x1f << 3);
	c->shl(*addr, 1);
	c->pshufb(va, asmjit::host::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CBX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->add(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->not_(*addr);
	c->and_(*addr, 0xf);
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	c->mov(asmjit::host::byte_ptr(*cpu, *addr, 0, OFFSET_OF(SPUThread, gpr[op.rt])), 0x03);
}

void spu_recompiler::CHX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->add(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->not_(*addr);
	c->and_(*addr, 0xe);
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	c->mov(asmjit::host::word_ptr(*cpu, *addr, 0, OFFSET_OF(SPUThread, gpr[op.rt])), 0x0203);
}

void spu_recompiler::CWX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->add(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->not_(*addr);
	c->and_(*addr, 0xc);
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	c->mov(asmjit::host::dword_ptr(*cpu, *addr, 0, OFFSET_OF(SPUThread, gpr[op.rt])), 0x00010203);
}

void spu_recompiler::CDX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->add(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->not_(*addr);
	c->and_(*addr, 0x8);
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	c->mov(*qw0, asmjit::imm_u(0x0001020304050607));
	c->mov(asmjit::host::qword_ptr(*cpu, *addr, 0, OFFSET_OF(SPUThread, gpr[op.rt])), *qw0);
}

void spu_recompiler::ROTQBI(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr[op.ra]._u64[0]));
	c->mov(*qw1, SPU_OFF_64(gpr[op.ra]._u64[1]));
	c->mov(*qw2, *qw0);
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->and_(*addr, 7);
	c->shld(*qw0, *qw1, *addr);
	c->shld(*qw1, *qw2, *addr);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[0]), *qw0);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[1]), *qw1);
}

void spu_recompiler::ROTQMBI(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr[op.ra]._u64[0]));
	c->mov(*qw1, SPU_OFF_64(gpr[op.ra]._u64[1]));
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->neg(*addr);
	c->and_(*addr, 7);
	c->shrd(*qw0, *qw1, *addr);
	c->shr(*qw1, *addr);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[0]), *qw0);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[1]), *qw1);
}

void spu_recompiler::SHLQBI(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr[op.ra]._u64[0]));
	c->mov(*qw1, SPU_OFF_64(gpr[op.ra]._u64[1]));
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->and_(*addr, 7);
	c->shld(*qw1, *qw0, *addr);
	c->shl(*qw0, *addr);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[0]), *qw0);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[1]), *qw1);
}

void spu_recompiler::ROTQBY(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.rldq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->and_(*addr, 0xf);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::host::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ROTQMBY(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.srdq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->neg(*addr);
	c->and_(*addr, 0x1f);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::host::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::SHLQBY(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.sldq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));
	c->and_(*addr, 0x1f);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::host::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ORX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[0]));
	c->or_(*addr, SPU_OFF_32(gpr[op.ra]._u32[1]));
	c->or_(*addr, SPU_OFF_32(gpr[op.ra]._u32[2]));
	c->or_(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->mov(SPU_OFF_32(gpr[op.rt]._u32[3]), *addr);
	c->xor_(*addr, *addr);
	c->mov(SPU_OFF_32(gpr[op.rt]._u32[0]), *addr);
	c->mov(SPU_OFF_32(gpr[op.rt]._u32[1]), *addr);
	c->mov(SPU_OFF_32(gpr[op.rt]._u32[2]), *addr);
}

void spu_recompiler::CBD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u8r[op.si7 & 0xf] = 0x03;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	if (op.si7) c->add(*addr, op.si7);
	c->not_(*addr);
	c->and_(*addr, 0xf);
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	c->mov(asmjit::host::byte_ptr(*cpu, *addr, 0, OFFSET_OF(SPUThread, gpr[op.rt])), 0x03);
}

void spu_recompiler::CHD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u16r[(op.si7 >> 1) & 0x7] = 0x0203;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	if (op.si7) c->add(*addr, op.si7);
	c->not_(*addr);
	c->and_(*addr, 0xe);
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	c->mov(asmjit::host::word_ptr(*cpu, *addr, 0, OFFSET_OF(SPUThread, gpr[op.rt])), 0x0203);
}

void spu_recompiler::CWD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u32r[(op.si7 >> 2) & 0x3] = 0x00010203;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	if (op.si7) c->add(*addr, op.si7);
	c->not_(*addr);
	c->and_(*addr, 0xc);
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	c->mov(asmjit::host::dword_ptr(*cpu, *addr, 0, OFFSET_OF(SPUThread, gpr[op.rt])), 0x00010203);
}

void spu_recompiler::CDD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u64r[(op.si7 >> 3) & 0x1] = 0x0001020304050607ull;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	if (op.si7) c->add(*addr, op.si7);
	c->not_(*addr);
	c->and_(*addr, 0x8);
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	c->mov(*qw0, asmjit::imm_u(0x0001020304050607));
	c->mov(asmjit::host::qword_ptr(*cpu, *addr, 0, OFFSET_OF(SPUThread, gpr[op.rt])), *qw0);
}

void spu_recompiler::ROTQBII(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr[op.ra]._u64[0]));
	c->mov(*qw1, SPU_OFF_64(gpr[op.ra]._u64[1]));
	c->mov(*qw2, *qw0);
	c->shld(*qw0, *qw1, op.si7 & 0x7);
	c->shld(*qw1, *qw2, op.si7 & 0x7);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[0]), *qw0);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[1]), *qw1);
}

void spu_recompiler::ROTQMBII(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr[op.ra]._u64[0]));
	c->mov(*qw1, SPU_OFF_64(gpr[op.ra]._u64[1]));
	c->shrd(*qw0, *qw1, (0 - op.si7) & 0x7);
	c->shr(*qw1, (0 - op.si7) & 0x7);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[0]), *qw0);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[1]), *qw1);
}

void spu_recompiler::SHLQBII(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr[op.ra]._u64[0]));
	c->mov(*qw1, SPU_OFF_64(gpr[op.ra]._u64[1]));
	c->shld(*qw1, *qw0, op.si7 & 0x7);
	c->shl(*qw0, op.si7 & 0x7);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[0]), *qw0);
	c->mov(SPU_OFF_64(gpr[op.rt]._u64[1]), *qw1);
}

void spu_recompiler::ROTQBYI(spu_opcode_t op)
{
	const int s = op.si7 & 0xf;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->palignr(va, va, 16 - s);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ROTQMBYI(spu_opcode_t op)
{
	const int s = (0 - op.si7) & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrldq(va, s);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::SHLQBYI(spu_opcode_t op)
{
	const int s = op.si7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pslldq(va, s);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::NOP(spu_opcode_t op)
{
}

void spu_recompiler::CGT(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtd(va, SPU_OFF_128(gpr[op.rb]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::XOR(spu_opcode_t op)
{
	// xor
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, SPU_OFF_128(gpr[op.rb]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CGTH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtw(va, SPU_OFF_128(gpr[op.rb]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::EQV(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pxor(vb, XmmConst(v128::from32p(0xffffffff)));
	c->pxor(vb, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::CGTB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtb(va, SPU_OFF_128(gpr[op.rb]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::SUMB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from8p(1)));
	c->pmaddubsw(va, vi);
	c->pmaddubsw(vb, vi);
	c->phaddw(va, vb);
	c->pshufb(va, XmmConst(_mm_set_epi8(15, 14, 7, 6, 13, 12, 5, 4, 11, 10, 3, 2, 9, 8, 1, 0)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

//HGT uses signed values.  HLGT uses unsigned values
void spu_recompiler::HGT(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._s32[3]));
	c->cmp(*addr, SPU_OFF_32(gpr[op.rb]._s32[3]));

	asmjit::Label next(*c);
	c->jng(next);
	c->mov(*pos, m_pos | 0x1000000);
	c->ret(*pos);
	c->bind(next);
}

void spu_recompiler::CLZ(spu_opcode_t op)
{
	auto body = [](u32* t, const u32* a) noexcept
	{
		for (u32 i = 0; i < 4; i++)
		{
			t[i] = cntlz32(a[i]);
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(u32*, const u32*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder2<void, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);

	//c->mov(*qw0, 32 + 31);
	//for (u32 i = 0; i < 4; i++) // unrolled loop
	//{
	//	c->bsr(*addr, SPU_OFF_32(gpr[op.ra]._u32[i]));
	//	c->cmovz(*addr, qw0->r32());
	//	c->xor_(*addr, 31);
	//	c->mov(SPU_OFF_32(gpr[op.rt]._u32[i]), *addr);
	//}
}

void spu_recompiler::XSWD(spu_opcode_t op)
{
	c->movsxd(*qw0, SPU_OFF_32(gpr[op.ra]._s32[0]));
	c->movsxd(*qw1, SPU_OFF_32(gpr[op.ra]._s32[2]));
	c->mov(SPU_OFF_64(gpr[op.rt]._s64[0]), *qw0);
	c->mov(SPU_OFF_64(gpr[op.rt]._s64[1]), *qw1);
}

void spu_recompiler::XSHW(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pslld(va, 16);
	c->psrad(va, 16);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CNTB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	const XmmLink& vm = XmmAlloc();
	c->movdqa(v1, va);
	c->psrlq(v1, 4);
	c->movdqa(vm, XmmConst(v128::from8p(0xf)));
	c->pand(va, vm);
	c->pand(v1, vm);
	c->movdqa(vm, XmmConst(_mm_set_epi8(4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0)));
	c->pshufb(vm, va);
	c->movdqa(va, XmmConst(_mm_set_epi8(4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0)));
	c->pshufb(va, v1);
	c->paddb(va, vm);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::XSBH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psllw(va, 8);
	c->psraw(va, 8);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CLGT(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from32p(0x80000000)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr[op.rb]));
	c->pcmpgtd(va, vi);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ANDC(spu_opcode_t op)
{
	// and not
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pandn(vb, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::FCGT(spu_opcode_t op)
{
	// reverted less-than
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	c->cmpps(vb, SPU_OFF_128(gpr[op.ra]), 1);
	c->movaps(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::DFCGT(spu_opcode_t op)
{
	throw EXCEPTION("Unexpected instruction");
}

void spu_recompiler::FA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->addps(va, SPU_OFF_128(gpr[op.rb]));
	c->movaps(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::FS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->subps(va, SPU_OFF_128(gpr[op.rb]));
	c->movaps(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::FM(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->mulps(va, SPU_OFF_128(gpr[op.rb]));
	c->movaps(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CLGTH(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from16p(0x8000)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr[op.rb]));
	c->pcmpgtw(va, vi);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ORC(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pxor(vb, XmmConst(v128::from32p(0xffffffff)));
	c->por(vb, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::FCMGT(spu_opcode_t op)
{
	// reverted less-than
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	const XmmLink& vi = XmmAlloc();
	c->movaps(vi, XmmConst(v128::from32p(0x7fffffff)));
	c->andps(vb, vi); // abs
	c->andps(vi, SPU_OFF_128(gpr[op.ra]));
	c->cmpps(vb, vi, 1);
	c->movaps(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::DFCMGT(spu_opcode_t op)
{
	throw EXCEPTION("Unexpected instruction");
}

void spu_recompiler::DFA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->addpd(va, SPU_OFF_128(gpr[op.rb]));
	c->movapd(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::DFS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->subpd(va, SPU_OFF_128(gpr[op.rb]));
	c->movapd(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::DFM(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr[op.rb]));
	c->movapd(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CLGTB(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from8p(0x80)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr[op.rb]));
	c->pcmpgtb(va, vi);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::HLGT(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->cmp(*addr, SPU_OFF_32(gpr[op.rb]._u32[3]));

	asmjit::Label next(*c);
	c->jna(next);
	c->mov(*pos, m_pos | 0x1000000);
	c->ret(*pos);
	c->bind(next);
}

void spu_recompiler::DFMA(spu_opcode_t op)
{
	const XmmLink& vr = XmmGet(op.rt, XmmType::Double);
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr[op.rb]));
	c->addpd(vr, va);
	c->movapd(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::DFMS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	const XmmLink& vt = XmmGet(op.rt, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr[op.rb]));
	c->subpd(va, vt);
	c->movapd(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::DFNMS(spu_opcode_t op)
{
	const XmmLink& vr = XmmGet(op.rt, XmmType::Double);
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr[op.rb]));
	c->subpd(vr, va);
	c->movapd(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::DFNMA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	const XmmLink& vt = XmmGet(op.rt, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr[op.rb]));
	c->addpd(vt, va);
	c->xorpd(va, va);
	c->subpd(va, vt);
	c->movapd(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CEQ(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqd(va, SPU_OFF_128(gpr[op.rb]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::MPYHHU(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& va2 = XmmAlloc();
	c->movdqa(va2, va);
	c->pmulhuw(va, vb);
	c->pmullw(va2, vb);
	c->pand(va, XmmConst(v128::from32p(0xffff0000)));
	c->psrld(va2, 16);
	c->por(va, va2);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ADDX(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pand(vt, XmmConst(v128::from32p(1)));
	c->paddd(vt, SPU_OFF_128(gpr[op.ra]));
	c->paddd(vt, SPU_OFF_128(gpr[op.rb]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vt);
}

void spu_recompiler::SFX(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pandn(vt, XmmConst(v128::from32p(1)));
	c->psubd(vb, SPU_OFF_128(gpr[op.ra]));
	c->psubd(vb, vt);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::CGX(spu_opcode_t op) //nf
{
	auto body = [](u32* t, const u32* a, const u32* b) noexcept
	{
		for (s32 i = 0; i < 4; i++)
		{
			t[i] = (static_cast<u64>(t[i] & 1) + a[i] + b[i]) >> 32;
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	c->lea(*qw2, SPU_OFF_128(gpr[op.rb]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(u32*, const u32*, const u32*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<void, void*, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);
}

void spu_recompiler::BGX(spu_opcode_t op) //nf
{
	auto body = [](u32* t, const u32* a, const u32* b) noexcept
	{
		for (s32 i = 0; i < 4; i++)
		{
			const s64 result = (u64)b[i] - (u64)a[i] - (u64)(1 - (t[i] & 1));
			t[i] = result >= 0;
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr[op.rt]));
	c->lea(*qw1, SPU_OFF_128(gpr[op.ra]));
	c->lea(*qw2, SPU_OFF_128(gpr[op.rb]));
	asmjit::X86CallNode* call = c->call(asmjit::imm_ptr(asmjit_cast<void*, void(u32*, const u32*, const u32*)>(body)), asmjit::kFuncConvHost, asmjit::FuncBuilder3<void, void*, void*, void*>());
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);
}

void spu_recompiler::MPYHHA(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psrld(va, 16);
	c->psrld(vb, 16);
	c->pmaddwd(va, vb);
	c->paddd(vt, va);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vt);
}

void spu_recompiler::MPYHHAU(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& va2 = XmmAlloc();
	c->movdqa(va2, va);
	c->pmulhuw(va, vb);
	c->pmullw(va2, vb);
	c->pand(va, XmmConst(v128::from32p(0xffff0000)));
	c->psrld(va2, 16);
	c->paddd(vt, va);
	c->paddd(vt, va2);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vt);
}

void spu_recompiler::FSCRRD(spu_opcode_t op)
{
	// zero (hack)
	const XmmLink& v0 = XmmAlloc();
	c->pxor(v0, v0);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), v0);
}

void spu_recompiler::FESD(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->shufps(va, va, 0x8d); // _f[0] = _f[1]; _f[1] = _f[3];
	c->cvtps2pd(va, va);
	c->movapd(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::FRDS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->cvtpd2ps(va, va);
	c->shufps(va, va, 0x72); // _f[1] = _f[0]; _f[3] = _f[1]; _f[0] = _f[2] = 0;
	c->movaps(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::FSCRWR(spu_opcode_t op)
{
	// nop (not implemented)
}

void spu_recompiler::DFTSV(spu_opcode_t op)
{
	throw EXCEPTION("Unexpected instruction");
}

void spu_recompiler::FCEQ(spu_opcode_t op)
{
	// compare equal
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	c->cmpps(vb, SPU_OFF_128(gpr[op.ra]), 0);
	c->movaps(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::DFCEQ(spu_opcode_t op)
{
	throw EXCEPTION("Unexpected instruction");
}

void spu_recompiler::MPY(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from32p(0xffff)));
	c->pand(va, vi);
	c->pand(vb, vi);
	c->pmaddwd(va, vb);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::MPYH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psrld(va, 16);
	c->pmullw(va, vb);
	c->pslld(va, 16);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::MPYHH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psrld(va, 16);
	c->psrld(vb, 16);
	c->pmaddwd(va, vb);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::MPYS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pmulhw(va, vb);
	c->pslld(va, 16);
	c->psrad(va, 16);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CEQH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqw(va, SPU_OFF_128(gpr[op.rb]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::FCMEQ(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	const XmmLink& vi = XmmAlloc();
	c->movaps(vi, XmmConst(v128::from32p(0x7fffffff)));
	c->andps(vb, vi); // abs
	c->andps(vi, SPU_OFF_128(gpr[op.ra]));
	c->cmpps(vb, vi, 0); // ==
	c->movaps(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::DFCMEQ(spu_opcode_t op)
{
	throw EXCEPTION("Unexpected instruction");
}

void spu_recompiler::MPYU(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& va2 = XmmAlloc();
	c->movdqa(va2, va);
	c->pmulhuw(va, vb);
	c->pmullw(va2, vb);
	c->pslld(va, 16);
	c->pand(va2, XmmConst(v128::from32p(0xffff)));
	c->por(va, va2);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CEQB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqb(va, SPU_OFF_128(gpr[op.rb]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::FI(spu_opcode_t op)
{
	// Floating Interpolate
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	c->movaps(SPU_OFF_128(gpr[op.rt]), vb);
}

void spu_recompiler::HEQ(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._s32[3]));
	c->cmp(*addr, SPU_OFF_32(gpr[op.rb]._s32[3]));

	asmjit::Label next(*c);
	c->jne(next);
	c->mov(*pos, m_pos | 0x1000000);
	c->ret(*pos);
	c->bind(next);
}

void spu_recompiler::CFLTS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vi = XmmAlloc();
	if (op.si8 != 173) c->mulps(va, XmmConst(_mm_set1_ps(exp2f(static_cast<float>(173 - (op.si8 & 0xff)))))); // scale
	c->movaps(vi, XmmConst(_mm_set1_ps(exp2f(31))));
	c->cmpps(vi, va, 2);
	c->cvttps2dq(va, va); // convert to ints with truncation
	c->pxor(va, vi); // fix result saturation (0x80000000 -> 0x7fffffff)
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CFLTU(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vs = XmmAlloc();
	const XmmLink& vs2 = XmmAlloc();
	const XmmLink& vs3 = XmmAlloc();
	if (op.si8 != 173) c->mulps(va, XmmConst(_mm_set1_ps(exp2f(static_cast<float>(173 - (op.si8 & 0xff)))))); // scale
	c->maxps(va, XmmConst(_mm_set1_ps(0.0f))); // saturate
	c->movaps(vs, va); // copy scaled value
	c->movaps(vs2, va);
	c->movaps(vs3, XmmConst(_mm_set1_ps(exp2f(31))));
	c->subps(vs2, vs3);
	c->cmpps(vs3, vs, 2);
	c->andps(vs2, vs3);
	c->cvttps2dq(va, va);
	c->cmpps(vs, XmmConst(_mm_set1_ps(exp2f(32))), 5);
	c->cvttps2dq(vs2, vs2);
	c->por(va, vs);
	c->por(va, vs2);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CSFLT(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->cvtdq2ps(va, va); // convert to floats
	if (op.si8 != 155) c->mulps(va, XmmConst(_mm_set1_ps(exp2f(static_cast<float>((op.si8 & 0xff) - 155))))); // scale
	c->movaps(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CUFLT(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	c->movdqa(v1, va);
	c->pand(va, XmmConst(v128::from32p(0x7fffffff)));
	c->cvtdq2ps(va, va); // convert to floats
	c->psrad(v1, 31); // generate mask from sign bit
	c->andps(v1, XmmConst(_mm_set1_ps(exp2f(31)))); // generate correction component
	c->addps(va, v1); // add correction component
	if (op.si8 != 155) c->mulps(va, XmmConst(_mm_set1_ps(exp2f(static_cast<float>((op.si8 & 0xff) - 155))))); // scale
	c->movaps(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::BRZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.si16);

	c->cmp(SPU_OFF_32(gpr[op.rt]._u32[3]), 0);

	if (labels[target / 4].isInitialized())
	{
		c->je(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brz 0x%x)", target);
		}

		asmjit::Label next(*c);
		c->jne(next);
		c->mov(*pos, target);
		c->ret(*pos);
		c->bind(next);
	}
}

void spu_recompiler::STQA(spu_opcode_t op)
{
	const u32 lsa = (op.si16 << 2) & 0x3fff0;

	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(asmjit::host::oword_ptr(*ls, lsa), vt);
}

void spu_recompiler::BRNZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.si16);

	c->cmp(SPU_OFF_32(gpr[op.rt]._u32[3]), 0);

	if (labels[target / 4].isInitialized())
	{
		c->jne(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brnz 0x%x)", target);
		}

		asmjit::Label next(*c);
		c->je(next);
		c->mov(*pos, target);
		c->ret(*pos);
		c->bind(next);
	}
}

void spu_recompiler::BRHZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.si16);

	c->cmp(SPU_OFF_16(gpr[op.rt]._u16[6]), 0);

	if (labels[target / 4].isInitialized())
	{
		c->je(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brhz 0x%x)", target);
		}

		asmjit::Label next(*c);
		c->jne(next);
		c->mov(*pos, target);
		c->ret(*pos);
		c->bind(next);
	}
}

void spu_recompiler::BRHNZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.si16);

	c->cmp(SPU_OFF_16(gpr[op.rt]._u16[6]), 0);

	if (labels[target / 4].isInitialized())
	{
		c->jne(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brhnz 0x%x)", target);
		}

		asmjit::Label next(*c);
		c->je(next);
		c->mov(*pos, target);
		c->ret(*pos);
		c->bind(next);
	}
}

void spu_recompiler::STQR(spu_opcode_t op)
{
	const u32 lsa = spu_branch_target(m_pos, op.si16) & 0x3fff0;

	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(asmjit::host::oword_ptr(*ls, lsa), vt);
}

void spu_recompiler::BRA(spu_opcode_t op)
{
	const u32 target = spu_branch_target(0, op.si16);

	if (labels[target / 4].isInitialized())
	{
		c->jmp(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (bra 0x%x)", target);
		}

		c->mov(*pos, target);
		c->ret(*pos);
	}
}

void spu_recompiler::LQA(spu_opcode_t op)
{
	const u32 lsa = (op.si16 << 2) & 0x3fff0;

	const XmmLink& vt = XmmAlloc();
	c->movdqa(vt, asmjit::host::oword_ptr(*ls, lsa));
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vt);
}

void spu_recompiler::BRASL(spu_opcode_t op)
{
	const u32 target = spu_branch_target(0, op.si16);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32(0, 0, 0, m_pos + 4)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	if (target == m_pos + 4)
	{
		// branch-to-next, probably makes sense only for BRSL
		return;
	}

	c->mov(SPU_OFF_32(pc), target);

	FunctionCall();
}

void spu_recompiler::BR(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.si16);

	if (labels[target / 4].isInitialized())
	{
		c->jmp(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brz 0x%x)", target);
		}

		c->mov(*pos, target);
		c->ret(*pos);
	}
}

void spu_recompiler::FSMBI(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(g_spu_imm.fsmb[op.si16 & 0xffff]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::BRSL(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.si16);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32(0, 0, 0, m_pos + 4)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);

	if (target == m_pos + 4)
	{
		// branch-to-next
		return;
	}

	c->mov(SPU_OFF_32(pc), target);

	FunctionCall();
}

void spu_recompiler::LQR(spu_opcode_t op)
{
	const u32 lsa = spu_branch_target(m_pos, op.si16) & 0x3fff0;

	const XmmLink& vt = XmmAlloc();
	c->movdqa(vt, asmjit::host::oword_ptr(*ls, lsa));
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vt);
}

void spu_recompiler::IL(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32p(op.si16)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::ILHU(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32p(op.si16 << 16)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::ILH(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32p(op.si16)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::IOHL(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->por(vt, XmmConst(v128::from32p(op.si16 & 0xffff)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vt);
}

void spu_recompiler::ORI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	if (op.si10) c->por(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ORHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->por(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ORBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->por(va, XmmConst(v128::from8p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::SFI(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32p(op.si10)));
	c->psubd(vr, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::SFHI(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from16p(op.si10)));
	c->psubw(vr, SPU_OFF_128(gpr[op.ra]));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::ANDI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ANDHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::ANDBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, XmmConst(v128::from8p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::AI(spu_opcode_t op)
{
	// add
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->paddd(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::AHI(spu_opcode_t op)
{
	// add
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->paddw(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::STQD(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	if (op.si10) c->add(*addr, op.si10 << 4);
	c->and_(*addr, 0x3fff0);

	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(asmjit::host::oword_ptr(*ls, *addr), vt);
}

void spu_recompiler::LQD(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	if (op.si10) c->add(*addr, op.si10 << 4);
	c->and_(*addr, 0x3fff0);

	const XmmLink& vt = XmmAlloc();
	c->movdqa(vt, asmjit::host::oword_ptr(*ls, *addr));
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vt);
}

void spu_recompiler::XORI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::XORHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::XORBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(v128::from8p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CGTI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtd(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CGTHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtw(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CGTBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtb(va, XmmConst(v128::from8p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::HGTI(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._s32[3]));
	c->cmp(*addr, op.si10);

	asmjit::Label next(*c);
	c->jng(next);
	c->mov(*pos, m_pos | 0x1000000);
	c->ret(*pos);
	c->bind(next);
}

void spu_recompiler::CLGTI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(v128::from32p(0x80000000)));
	c->pcmpgtd(va, XmmConst(v128::from32p((u32)op.si10 - 0x80000000)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CLGTHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(v128::from16p(0x8000)));
	c->pcmpgtw(va, XmmConst(v128::from16p((u16)op.si10 - 0x8000)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CLGTBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psubb(va, XmmConst(v128::from8p(0x80)));
	c->pcmpgtb(va, XmmConst(v128::from8p((s8)op.si10 - 0x80)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::HLGTI(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->cmp(*addr, op.si10);

	asmjit::Label next(*c);
	c->jna(next);
	c->mov(*pos, m_pos | 0x1000000);
	c->ret(*pos);
	c->bind(next);
}

void spu_recompiler::MPYI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pmaddwd(va, XmmConst(v128::from32p(op.si10 & 0xffff)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::MPYUI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	const XmmLink& va2 = XmmAlloc();
	c->movdqa(va2, va);
	c->movdqa(vi, XmmConst(v128::from32p(op.si10 & 0xffff)));
	c->pmulhuw(va, vi);
	c->pmullw(va2, vi);
	c->pslld(va, 16);
	c->por(va, va2);
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CEQI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqd(va, XmmConst(v128::from32p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CEQHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqw(va, XmmConst(v128::from16p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::CEQBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqb(va, XmmConst(v128::from8p(op.si10)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), va);
}

void spu_recompiler::HEQI(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr[op.ra]._u32[3]));
	c->cmp(*addr, op.si10);

	asmjit::Label next(*c);
	c->jne(next);
	c->mov(*pos, m_pos | 0x1000000);
	c->ret(*pos);
	c->bind(next);
}

void spu_recompiler::HBRA(spu_opcode_t op)
{
}

void spu_recompiler::HBRR(spu_opcode_t op)
{
}

void spu_recompiler::ILA(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(v128::from32p(op.i18 & 0x3ffff)));
	c->movdqa(SPU_OFF_128(gpr[op.rt]), vr);
}

void spu_recompiler::SELB(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vc = XmmGet(op.rt, XmmType::Int);
	c->pand(vb, vc);
	c->pandn(vc, SPU_OFF_128(gpr[op.ra]));
	c->por(vb, vc);
	c->movdqa(SPU_OFF_128(gpr[op.rc]), vb);
}

void spu_recompiler::SHUFB(spu_opcode_t op)
{
	const XmmLink& v0 = XmmGet(op.rt, XmmType::Int); // v0 = mask
	const XmmLink& v1 = XmmAlloc();
	const XmmLink& v2 = XmmAlloc();
	const XmmLink& v3 = XmmAlloc();
	const XmmLink& v4 = XmmAlloc();
	const XmmLink& vFF = XmmAlloc();
	c->movdqa(v2, v0); // v2 = mask
	// generate specific values:
	c->movdqa(v1, XmmConst(v128::from8p(0xe0))); // v1 = 11100000
	c->movdqa(v3, XmmConst(v128::from8p(0x80))); // v3 = 10000000
	c->pand(v2, v1); // filter mask      v2 = mask & 11100000
	c->movdqa(vFF, v2); // and copy      vFF = mask & 11100000
	c->movdqa(v4, XmmConst(v128::from8p(0xc0))); // v4 = 11000000
	c->pcmpeqb(vFF, v4); // gen 0xff     vFF = (mask & 11100000 == 11000000) ? 0xff : 0
	c->movdqa(v4, v2); // copy again     v4 = mask & 11100000
	c->pand(v4, v3); // filter mask      v4 = mask & 10000000
	c->pcmpeqb(v2, v1); //               v2 = (mask & 11100000 == 11100000) ? 0xff : 0
	c->pcmpeqb(v4, v3); //               v4 = (mask & 10000000 == 10000000) ? 0xff : 0
	c->pand(v2, v3); // generate 0x80    v2 = (mask & 11100000 == 11100000) ? 0x80 : 0
	c->por(vFF, v2); // merge 0xff, 0x80 vFF = (mask & 11100000 == 11000000) ? 0xff : (mask & 11100000 == 11100000) ? 0x80 : 0
	c->pandn(v1, v0); // filter mask     v1 = mask & 00011111
	// select bytes from [op.rb]:
	c->movdqa(v2, XmmConst(v128::from8p(0x0f))); //   v2 = 00001111
	c->pxor(v1, XmmConst(v128::from8p(0x10))); //   v1 = (mask & 00011111) ^ 00010000
	c->psubb(v2, v1); //                 v2 = 00001111 - ((mask & 00011111) ^ 00010000)
	c->movdqa(v1, SPU_OFF_128(gpr[op.rb])); //        v1 = op.rb
	c->pshufb(v1, v2); //                v1 = select(op.rb, 00001111 - ((mask & 00011111) ^ 00010000))
	// select bytes from [op.ra]:
	c->pxor(v2, XmmConst(v128::from8p(0xf0))); //   v2 = (00001111 - ((mask & 00011111) ^ 00010000)) ^ 11110000
	c->movdqa(v3, SPU_OFF_128(gpr[op.ra])); //        v3 = op.ra
	c->pshufb(v3, v2); //                v3 = select(op.ra, (00001111 - ((mask & 00011111) ^ 00010000)) ^ 11110000)
	c->por(v1, v3); //                   v1 = select(op.rb, 00001111 - ((mask & 00011111) ^ 00010000)) | (v3)
	c->pandn(v4, v1); // filter result   v4 = v1 & ((mask & 10000000 == 10000000) ? 0 : 0xff)
	c->por(vFF, v4); // final merge      vFF = (mask & 10000000 == 10000000) ? ((mask & 11100000 == 11000000) ? 0xff : (mask & 11100000 == 11100000) ? 0x80 : 0) : (v1)
	c->movdqa(SPU_OFF_128(gpr[op.rc]), vFF);
}

void spu_recompiler::MPYA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(v128::from32p(0xffff)));
	c->pand(va, vi);
	c->pand(vb, vi);
	c->pmaddwd(va, vb);
	c->paddd(va, SPU_OFF_128(gpr[op.rt]));
	c->movdqa(SPU_OFF_128(gpr[op.rc]), va);
}

void spu_recompiler::FNMS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vc = XmmGet(op.rt, XmmType::Float);
	c->mulps(va, SPU_OFF_128(gpr[op.rb]));
	c->subps(vc, va);
	c->movaps(SPU_OFF_128(gpr[op.rc]), vc);
}

void spu_recompiler::FMA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->mulps(va, SPU_OFF_128(gpr[op.rb]));
	c->addps(va, SPU_OFF_128(gpr[op.rt]));
	c->movaps(SPU_OFF_128(gpr[op.rc]), va);
}

void spu_recompiler::FMS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->mulps(va, SPU_OFF_128(gpr[op.rb]));
	c->subps(va, SPU_OFF_128(gpr[op.rt]));
	c->movaps(SPU_OFF_128(gpr[op.rc]), va);
}

void spu_recompiler::UNK(spu_opcode_t op)
{
	throw EXCEPTION("Unknown/Illegal opcode (0x%08x)", op.opcode);
}

const spu_opcode_table_t<void(spu_recompiler::*)(spu_opcode_t)> spu_recompiler::opcodes{ DEFINE_SPU_OPCODES(&spu_recompiler::), &spu_recompiler::UNK };
