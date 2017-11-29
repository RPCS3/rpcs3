#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "SPUDisAsm.h"
#include "SPUThread.h"
#include "SPUInterpreter.h"
#include "SPUASMJITRecompiler.h"

#include <cmath>

#define ASMJIT_STATIC
#define ASMJIT_DEBUG

#include "asmjit.h"

#define SPU_OFF_128(x, ...) asmjit::x86::oword_ptr(*cpu, offset32(&SPUThread::x, ##__VA_ARGS__))
#define SPU_OFF_64(x, ...) asmjit::x86::qword_ptr(*cpu, offset32(&SPUThread::x, ##__VA_ARGS__))
#define SPU_OFF_32(x, ...) asmjit::x86::dword_ptr(*cpu, offset32(&SPUThread::x, ##__VA_ARGS__))
#define SPU_OFF_16(x, ...) asmjit::x86::word_ptr(*cpu, offset32(&SPUThread::x, ##__VA_ARGS__))
#define SPU_OFF_8(x, ...) asmjit::x86::byte_ptr(*cpu, offset32(&SPUThread::x, ##__VA_ARGS__))

const spu_decoder<spu_interpreter_fast> s_spu_interpreter; // TODO: remove
const spu_decoder<spu_recompiler> s_spu_decoder;

spu_recompiler::spu_recompiler()
	: m_jit(std::make_shared<asmjit::JitRuntime>())
{
	LOG_SUCCESS(SPU, "SPU Recompiler (ASMJIT) created...");

	if (g_cfg.core.spu_debug)
	{
		fs::file log(Emu.GetCachePath() + "SPUJIT.log", fs::rewrite);
		log.write(fmt::format("SPU JIT initialization...\n\nTitle: %s\nTitle ID: %s\n\n", Emu.GetTitle().c_str(), Emu.GetTitleID().c_str()));
	}
}

void spu_recompiler::compile(spu_function_t& f)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (f.compiled)
	{
		// return if function already compiled
		return;
	}

	if (f.addr >= 0x40000 || f.addr % 4 || f.size == 0 || f.size > 0x40000 - f.addr || f.size % 4)
	{
		fmt::throw_exception("Invalid SPU function (addr=0x%05x, size=0x%x)" HERE, f.addr, f.size);
	}

	using namespace asmjit;

	SPUDisAsm dis_asm(CPUDisAsm_InterpreterMode);
	dis_asm.offset = reinterpret_cast<u8*>(f.data.data()) - f.addr;

	StringLogger logger;
	logger.addOptions(Logger::kOptionBinaryForm);

	std::string log;

	if (g_cfg.core.spu_debug)
	{
		fmt::append(log, "========== SPU FUNCTION 0x%05x - 0x%05x ==========\n\n", f.addr, f.addr + f.size);
	}

	this->m_func = &f;

	asmjit::CodeHolder code;
	code.init(m_jit->getCodeInfo());
	this->codeHolder = &code;

	X86Compiler compiler(&code);
	this->c = &compiler;

	if (g_cfg.core.spu_debug)
	{
		// Set logger
		codeHolder->setLogger(&logger);
	}

	compiler.addFunc(FuncSignature2<u32, void*, void*>(asmjit::CallConv::kIdHost));

	// Initialize variables
	X86Gp cpu_var = compiler.newIntPtr("cpu");
	compiler.setArg(0, cpu_var);
	compiler.alloc(cpu_var, asmjit::x86::rbp); // ASMJIT bug workaround
	this->cpu = &cpu_var;

	X86Gp ls_var = compiler.newIntPtr("ls");
	compiler.setArg(1, ls_var);
	compiler.alloc(ls_var, asmjit::x86::rbx); // ASMJIT bug workaround
	this->ls = &ls_var;

	X86Gp addr_var = compiler.newUInt32("addr");
	this->addr = &addr_var;
	X86Gp qw0_var = compiler.newUInt64("qw0");
	this->qw0 = &qw0_var;
	X86Gp qw1_var = compiler.newUInt64("qw1");
	this->qw1 = &qw1_var;
	X86Gp qw2_var = compiler.newUInt64("qw2");
	this->qw2 = &qw2_var;

	std::array<X86Xmm, 6> vec_vars;

	for (u32 i = 0; i < vec_vars.size(); i++)
	{
		vec_vars[i] = compiler.newXmm(fmt::format("vec%d", i).c_str());
		vec.at(i) = vec_vars.data() + i;
	}

	// Initialize labels
	std::vector<Label> pos_labels{ 0x10000 };
	this->labels = pos_labels.data();

	// Register labels for block entries
	for (const u32 addr : f.blocks)
	{
		if (addr < f.addr || addr >= f.addr + f.size || addr % 4)
		{
			fmt::throw_exception("Invalid function block entry (0x%05x)" HERE, addr);
		}

		pos_labels[addr / 4] = compiler.newLabel();
	}

	// Register label for post-the-end address
	pos_labels[(f.addr + f.size) / 4 % 0x10000] = compiler.newLabel();

	// Register label for jump table resolver
	Label jt_label = compiler.newLabel();
	this->jt = &jt_label;

	for (const u32 addr : f.jtable)
	{
		if (addr < f.addr || addr >= f.addr + f.size || addr % 4)
		{
			fmt::throw_exception("Invalid jump table entry (0x%05x)" HERE, addr);
		}
	}

	// Register label for the function return
	Label end_label = compiler.newLabel();
	this->end = &end_label;

	// Start compilation
	m_pos = f.addr;

	for (const u32 op : f.data)
	{
		// Bind label if initialized
		if (pos_labels[m_pos / 4].isValid())
		{
			compiler.bind(pos_labels[m_pos / 4]);

			if (f.blocks.find(m_pos) != f.blocks.end())
			{
				compiler.comment("Block:");
			}
		}

		if (g_cfg.core.spu_debug)
		{
			// Disasm
			dis_asm.dump_pc = m_pos;
			dis_asm.disasm(m_pos);
			compiler.comment(dis_asm.last_opcode.c_str());
			log += dis_asm.last_opcode;
			log += '\n';
		}

		// Recompiler function
		(this->*s_spu_decoder.decode(op))({ op });

		// Collect allocated xmm vars
		for (u32 i = 0; i < vec_vars.size(); i++)
		{
			if (!vec[i])
			{
				compiler.unuse(vec_vars[i]);
				vec[i] = vec_vars.data() + i;
			}
		}

		// Set next position
		m_pos += 4;
	}

	if (g_cfg.core.spu_debug)
	{
		log += '\n';
	}

	// Generate default function end (go to the next address)
	compiler.bind(pos_labels[m_pos / 4 % 0x10000]);
	compiler.comment("Fallthrough:");
	compiler.mov(addr_var, spu_branch_target(m_pos));
	compiler.jmp(end_label);

	// Generate jump table resolver (uses addr_var)
	compiler.bind(jt_label);

	if (f.jtable.size())
	{
		compiler.comment("Jump table resolver:");
	}

	for (const u32 addr : f.jtable)
	{
		if ((addr % 4) == 0 && addr < 0x40000 && pos_labels[addr / 4].isValid())
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

	// Generate function end (returns addr_var)
	compiler.bind(end_label);
	compiler.unuse(cpu_var);
	compiler.unuse(ls_var);
	compiler.ret(addr_var);

	// Finalization
	compiler.endFunc();
	compiler.finalize();

	// Compile and store function address
	typedef u32 (*Func)(void* x, void* y);
	Func fn;
	m_jit->add(&fn, codeHolder);

	f.compiled = asmjit::Internal::ptr_cast<decltype(f.compiled)>(fn);
	
	if (g_cfg.core.spu_debug)
	{
		// Add ASMJIT logs
		log += logger.getString();
		log += "\n\n\n";

		// Append log file
		fs::file(Emu.GetCachePath() + "SPUJIT.log", fs::write + fs::append).write(log);
	}
}

spu_recompiler::XmmLink spu_recompiler::XmmAlloc() // get empty xmm register
{
	for (auto& v : vec)
	{
		if (v) return{ v };
	}

	fmt::throw_exception("Out of Xmm Vars" HERE);
}

spu_recompiler::XmmLink spu_recompiler::XmmGet(s8 reg, XmmType type) // get xmm register with specific SPU reg
{
	XmmLink result = XmmAlloc();

	switch (type)
	{
	case XmmType::Int: c->movdqa(result, SPU_OFF_128(gpr, reg)); break;
	case XmmType::Float: c->movaps(result, SPU_OFF_128(gpr, reg)); break;
	case XmmType::Double: c->movapd(result, SPU_OFF_128(gpr, reg)); break;
	default: fmt::throw_exception("Invalid XmmType" HERE);
	}

	return result;
}

inline asmjit::X86Mem spu_recompiler::XmmConst(v128 data)
{
	return c->newXmmConst(asmjit::kConstScopeLocal, asmjit::Data128::fromU64(data._u64[0], data._u64[1]));
}

inline asmjit::X86Mem spu_recompiler::XmmConst(__m128 data)
{
	return XmmConst(v128::fromF(data));
}

inline asmjit::X86Mem spu_recompiler::XmmConst(__m128i data)
{
	return XmmConst(v128::fromV(data));
}

void spu_recompiler::CheckInterruptStatus(spu_opcode_t op)
{
    if (op.d)
        c->lock().btr(SPU_OFF_8(interrupts_enabled), 0);
    else if (op.e) {
        c->lock().bts(SPU_OFF_8(interrupts_enabled), 0);
        c->mov(*qw0, SPU_OFF_32(ch_event_stat));
        c->and_(*qw0, SPU_OFF_32(ch_event_mask));
        c->and_(*qw0, SPU_EVENT_INTR_TEST);
        c->cmp(*qw0, 0);
        
        asmjit::Label noInterrupt = c->newLabel();
        c->je(noInterrupt);
        c->lock().btr(SPU_OFF_8(interrupts_enabled), 0);
        c->mov(SPU_OFF_32(srr0), *addr);
        c->mov(SPU_OFF_32(pc), 0);

        FunctionCall();

        c->mov(*addr, SPU_OFF_32(srr0));
        c->bind(noInterrupt);
        c->unuse(*qw0);
    }
}

void spu_recompiler::InterpreterCall(spu_opcode_t op)
{
	auto gate = [](SPUThread* _spu, u32 opcode, spu_inter_func_t _func) noexcept -> u32
	{
		try
		{
			// TODO: check correctness

			const u32 old_pc = _spu->pc;

			if (test(_spu->state) && _spu->check_state())
			{
				return 0x2000000 | _spu->pc;
			}

			_func(*_spu, { opcode });

			if (old_pc != _spu->pc)
			{
				_spu->pc += 4;
				return 0x2000000 | _spu->pc;
			}

			_spu->pc += 4;
			return 0;
		}
		catch (...)
		{
			_spu->pending_exception = std::current_exception();
			return 0x1000000 | _spu->pc;
		}
	};

	c->mov(SPU_OFF_32(pc), m_pos);
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, u32(SPUThread*, u32, spu_inter_func_t)>(gate)), asmjit::FuncSignature3<u32, void*, u32, void*>(asmjit::CallConv::kIdHost));
	call->setArg(0, *cpu);
	call->setArg(1, asmjit::imm_u(op.opcode));
	call->setArg(2, asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*>(s_spu_interpreter.decode(op.opcode))));
	call->setRet(0, *addr);

	// return immediately if an error occured
	c->test(*addr, *addr);
	c->jnz(*end);
	c->unuse(*addr);
}

void spu_recompiler::FunctionCall()
{
	auto gate = [](SPUThread* _spu, u32 link) noexcept -> u32
	{
		_spu->recursion_level++;

		try
		{
			// TODO: check correctness

			if (_spu->pc & 0x4000000)
			{
				if (_spu->pc & 0x8000000)
				{
					fmt::throw_exception("Undefined behaviour" HERE);
				}

				_spu->interrupts_enabled = true;
				_spu->pc &= ~0x4000000;
			}
			else if (_spu->pc & 0x8000000)
			{
				_spu->interrupts_enabled = false;
				_spu->pc &= ~0x8000000;
			}

			if (_spu->pc == link)
			{
				LOG_ERROR(SPU, "Branch-to-next");
			}
			else if (_spu->pc == link - 4)
			{
				LOG_ERROR(SPU, "Branch-to-self");
			}

			while (!test(_spu->state) || !_spu->check_state())
			{
				// Proceed recursively
				spu_recompiler_base::enter(*_spu);

				if (test(_spu->state & cpu_flag::ret))
				{
					break;
				}

				if (_spu->pc == link)
				{
					_spu->recursion_level--;
					return 0; // Successfully returned 
				}
			}

			_spu->recursion_level--;
			return 0x2000000 | _spu->pc;
		}
		catch (...)
		{
			_spu->pending_exception = std::current_exception();

			_spu->recursion_level--;
			return 0x1000000 | _spu->pc;
		}
	};

	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, u32(SPUThread*, u32)>(gate)), asmjit::FuncSignature2<u32, SPUThread*, u32>(asmjit::CallConv::kIdHost));
	call->setArg(0, *cpu);
	call->setArg(1, asmjit::imm_u(spu_branch_target(m_pos + 4)));
	call->setRet(0, *addr);

	// return immediately if an error occured
	c->test(*addr, *addr);
	c->jnz(*end);
	c->unuse(*addr);
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
	switch (op.ra)
	{
	case SPU_RdSRR0:
	{
		const XmmLink& vr = XmmAlloc();
		c->movd(vr, SPU_OFF_32(srr0));
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}
	case MFC_RdTagMask:
	{
		const XmmLink& vr = XmmAlloc();
		c->movd(vr, SPU_OFF_32(ch_tag_mask));
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}
	case SPU_RdEventMask:
	{
		const XmmLink& vr = XmmAlloc();
		c->movd(vr, SPU_OFF_32(ch_event_mask));
		c->pslldq(vr, 12);
		c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
		return;
	}
	default:
	{
		InterpreterCall(op); // TODO
	}
	}
}

void spu_recompiler::RCHCNT(spu_opcode_t op)
{
	InterpreterCall(op); // TODO
}

void spu_recompiler::SF(spu_opcode_t op)
{
	// sub from
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psubd(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::OR(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->por(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::BG(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(_mm_set1_epi32(0x80000000)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr, op.rb));
	c->pcmpgtd(va, vi);
	c->paddd(va, XmmConst(_mm_set1_epi32(1)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SFH(spu_opcode_t op)
{
	// sub from (halfword)
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psubw(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::NOR(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->por(va, SPU_OFF_128(gpr, op.rb));
	c->pxor(va, XmmConst(_mm_set1_epi32(0xffffffff)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
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
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROT(spu_opcode_t op)
{
	auto body = [](u32* t, const u32* a, const s32* b) noexcept
	{
		for (u32 i = 0; i < 4; i++)
		{
			t[i] = rol32(a[i], b[i]);
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	c->lea(*qw2, SPU_OFF_128(gpr, op.rb));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(u32*, const u32*, const s32*)>(body)), asmjit::FuncSignature3<void, void*, void*, void*>(asmjit::CallConv::kIdHost));
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 4; i++) // unrolled loop
	//{
	//	c->mov(qw0->r32(), SPU_OFF_32(gpr, op.ra, &v128::_u32, i));
	//	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, i));
	//	c->rol(qw0->r32(), *addr);
	//	c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, i), qw0->r32());
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

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	c->lea(*qw2, SPU_OFF_128(gpr, op.rb));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(u32*, const u32*, const u32*)>(body)), asmjit::FuncSignature3<void, void*, void*, void*>(asmjit::CallConv::kIdHost));
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 4; i++) // unrolled loop
	//{
	//	c->mov(qw0->r32(), SPU_OFF_32(gpr, op.ra, &v128::_u32, i));
	//	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, i));
	//	c->neg(*addr);
	//	c->shr(*qw0, *addr);
	//	c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, i), qw0->r32());
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

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	c->lea(*qw2, SPU_OFF_128(gpr, op.rb));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(s32*, const s32*, const u32*)>(body)), asmjit::FuncSignature3<void, void*, void*, void*>(asmjit::CallConv::kIdHost));
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 4; i++) // unrolled loop
	//{
	//	c->movsxd(*qw0, SPU_OFF_32(gpr, op.ra, &v128::_u32, i));
	//	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, i));
	//	c->neg(*addr);
	//	c->sar(*qw0, *addr);
	//	c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, i), qw0->r32());
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

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	c->lea(*qw2, SPU_OFF_128(gpr, op.rb));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(u32*, const u32*, const u32*)>(body)), asmjit::FuncSignature3<void, void*, void*, void*>(asmjit::CallConv::kIdHost));
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 4; i++) // unrolled loop
	//{
	//	c->mov(qw0->r32(), SPU_OFF_32(gpr, op.ra, &v128::_u32, i));
	//	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, i));
	//	c->shl(*qw0, *addr);
	//	c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, i), qw0->r32());
	//}
}

void spu_recompiler::ROTH(spu_opcode_t op) //nf
{
	auto body = [](u16* t, const u16* a, const s16* b) noexcept
	{
		for (u32 i = 0; i < 8; i++)
		{
			t[i] = rol16(a[i], b[i]);
		}
	};

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	c->lea(*qw2, SPU_OFF_128(gpr, op.rb));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(u16*, const u16*, const s16*)>(body)), asmjit::FuncSignature3<void, void*, void*, void*>(asmjit::CallConv::kIdHost));
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 8; i++) // unrolled loop
	//{
	//	c->movzx(qw0->r32(), SPU_OFF_16(gpr, op.ra, &v128::_u16, i));
	//	c->movzx(*addr, SPU_OFF_16(gpr, op.rb, &v128::_u16, i));
	//	c->rol(qw0->r16(), *addr);
	//	c->mov(SPU_OFF_16(gpr, op.rt, &v128::_u16, i), qw0->r16());
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

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	c->lea(*qw2, SPU_OFF_128(gpr, op.rb));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(u16*, const u16*, const u16*)>(body)), asmjit::FuncSignature3<void, void*, void*, void*>(asmjit::CallConv::kIdHost));
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 8; i++) // unrolled loop
	//{
	//	c->movzx(qw0->r32(), SPU_OFF_16(gpr, op.ra, &v128::_u16, i));
	//	c->movzx(*addr, SPU_OFF_16(gpr, op.rb, &v128::_u16, i));
	//	c->neg(*addr);
	//	c->shr(qw0->r32(), *addr);
	//	c->mov(SPU_OFF_16(gpr, op.rt, &v128::_u16, i), qw0->r16());
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

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	c->lea(*qw2, SPU_OFF_128(gpr, op.rb));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(s16*, const s16*, const u16*)>(body)), asmjit::FuncSignature3<void, void*, void*, void*>(asmjit::CallConv::kIdHost));
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 8; i++) // unrolled loop
	//{
	//	c->movsx(qw0->r32(), SPU_OFF_16(gpr, op.ra, &v128::_u16, i));
	//	c->movzx(*addr, SPU_OFF_16(gpr, op.rb, &v128::_u16, i));
	//	c->neg(*addr);
	//	c->sar(qw0->r32(), *addr);
	//	c->mov(SPU_OFF_16(gpr, op.rt, &v128::_u16, i), qw0->r16());
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

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	c->lea(*qw2, SPU_OFF_128(gpr, op.rb));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(u16*, const u16*, const u16*)>(body)), asmjit::FuncSignature3<void, void*, void*, void*>(asmjit::CallConv::kIdHost));
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);
	call->setArg(2, *qw2);

	//for (u32 i = 0; i < 8; i++) // unrolled loop
	//{
	//	c->movzx(qw0->r32(), SPU_OFF_16(gpr, op.ra, &v128::_u16, i));
	//	c->movzx(*addr, SPU_OFF_16(gpr, op.rb, &v128::_u16, i));
	//	c->shl(qw0->r32(), *addr);
	//	c->mov(SPU_OFF_16(gpr, op.rt, &v128::_u16, i), qw0->r16());
	//}
}

void spu_recompiler::ROTI(spu_opcode_t op)
{
	// rotate left
	const int s = op.i7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	c->movdqa(v1, va);
	c->pslld(va, s);
	c->psrld(v1, 32 - s);
	c->por(va, v1);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTMI(spu_opcode_t op)
{
	// shift right logical
	const int s = 0-op.i7 & 0x3f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrld(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTMAI(spu_opcode_t op)
{
	// shift right arithmetical
	const int s = 0-op.i7 & 0x3f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrad(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SHLI(spu_opcode_t op)
{
	// shift left
	const int s = op.i7 & 0x3f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pslld(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTHI(spu_opcode_t op)
{
	// rotate left (halfword)
	const int s = op.i7 & 0xf;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	c->movdqa(v1, va);
	c->psllw(va, s);
	c->psrlw(v1, 16 - s);
	c->por(va, v1);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTHMI(spu_opcode_t op)
{
	// shift right logical
	const int s = 0-op.i7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrlw(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTMAHI(spu_opcode_t op)
{
	// shift right arithmetical (halfword)
	const int s = 0-op.i7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psraw(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SHLHI(spu_opcode_t op)
{
	// shift left (halfword)
	const int s = op.i7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psllw(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::A(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->paddd(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::AND(spu_opcode_t op)
{
	// and
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pand(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::CG(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(_mm_set1_epi32(0x80000000)));
	c->paddd(vb, va);
	c->pxor(va, vi);
	c->pxor(vb, vi);
	c->pcmpgtd(va, vb);
	c->psrld(va, 31);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::AH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->paddw(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::NAND(spu_opcode_t op)
{
	// nand
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, SPU_OFF_128(gpr, op.rb));
	c->pxor(va, XmmConst(_mm_set1_epi32(0xffffffff)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::AVGB(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pavgb(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::MTSPR(spu_opcode_t op)
{
	InterpreterCall(op);
}

void spu_recompiler::WRCH(spu_opcode_t op)
{
	switch (op.ra)
	{
	case SPU_WrSRR0:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_32(srr0), *addr);
		c->unuse(*addr);
		return;
	}
	case MFC_WrTagMask:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_32(ch_tag_mask), *addr);
		c->unuse(*addr);
		return;
	}
	case MFC_LSA:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_32(ch_mfc_cmd, &spu_mfc_cmd::lsa), *addr);
		c->unuse(*addr);
		return;
	}
	case MFC_EAH:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_32(ch_mfc_cmd, &spu_mfc_cmd::eah), *addr);
		c->unuse(*addr);
		return;
	}
	case MFC_EAL:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_32(ch_mfc_cmd, &spu_mfc_cmd::eal), *addr);
		c->unuse(*addr);
		return;
	}
	case MFC_Size:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_16(ch_mfc_cmd, &spu_mfc_cmd::size), addr->r16());
		c->unuse(*addr);
		return;
	}
	case MFC_TagID:
	{
		c->mov(*addr, SPU_OFF_32(gpr, op.rt, &v128::_u32, 3));
		c->mov(SPU_OFF_8(ch_mfc_cmd, &spu_mfc_cmd::tag), addr->r8());
		c->unuse(*addr);
		return;
	}
	case 69:
	{
		return;
	}
	default:
	{
		InterpreterCall(op); // TODO
	}
	}
}

void spu_recompiler::BIZ(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0x3fffc);
	if (op.d || op.e) c->or_(*addr, op.e << 26 | op.d << 27); // interrupt flags neutralize jump table
	c->cmp(SPU_OFF_32(gpr, op.rt, &v128::_u32, 3), 0);
	c->je(*jt);
	c->unuse(*addr);
}

void spu_recompiler::BINZ(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0x3fffc);
	if (op.d || op.e) c->or_(*addr, op.e << 26 | op.d << 27); // interrupt flags neutralize jump table
	c->cmp(SPU_OFF_32(gpr, op.rt, &v128::_u32, 3), 0);
	c->jne(*jt);
	c->unuse(*addr);
}

void spu_recompiler::BIHZ(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0x3fffc);
	if (op.d || op.e) c->or_(*addr, op.e << 26 | op.d << 27); // interrupt flags neutralize jump table
	c->cmp(SPU_OFF_16(gpr, op.rt, &v128::_u16, 6), 0);
	c->je(*jt);
	c->unuse(*addr);
}

void spu_recompiler::BIHNZ(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0x3fffc);
	if (op.d || op.e) c->or_(*addr, op.e << 26 | op.d << 27); // interrupt flags neutralize jump table
	c->cmp(SPU_OFF_16(gpr, op.rt, &v128::_u16, 6), 0);
	c->jne(*jt);
	c->unuse(*addr);
}

void spu_recompiler::STOPD(spu_opcode_t op)
{
	InterpreterCall(op);
}

void spu_recompiler::STQX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0x3fff0);

	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(asmjit::x86::oword_ptr(*ls, *addr), vt);
	c->unuse(*addr);
}

void spu_recompiler::BI(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0x3fffc);
    CheckInterruptStatus(op);
	c->jmp(*jt);
}

void spu_recompiler::BISL(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0x3fffc);
	if (op.d || op.e) c->or_(*addr, op.e << 26 | op.d << 27); // interrupt flags stored to PC
	c->mov(SPU_OFF_32(pc), *addr);
	c->unuse(*addr);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(spu_branch_target(m_pos + 4), 0, 0, 0)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->unuse(vr);

	FunctionCall();
}

void spu_recompiler::IRET(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(srr0));
	c->and_(*addr, 0x3fffc);
    CheckInterruptStatus(op);
	c->jmp(*jt);
}

void spu_recompiler::BISLED(spu_opcode_t op)
{
	InterpreterCall(op); //TODO
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
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
	c->unuse(*addr);
}

void spu_recompiler::GBH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pshufb(va, XmmConst(_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 14, 12, 10, 8, 6, 4, 2, 0)));
	c->psllq(va, 7);
	c->pmovmskb(*addr, va);
	c->pxor(va, va);
	c->pinsrw(va, *addr, 6);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
	c->unuse(*addr);
}

void spu_recompiler::GBB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psllq(va, 7);
	c->pmovmskb(*addr, va);
	c->pxor(va, va);
	c->pinsrw(va, *addr, 6);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
	c->unuse(*addr);
}

void spu_recompiler::FSM(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.fsm));
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0xf);
	c->shl(*addr, 4);
	c->movdqa(vr, asmjit::x86::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::FSMH(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.fsmh));
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0xff);
	c->shl(*addr, 4);
	c->movdqa(vr, asmjit::x86::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::FSMB(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.fsmb));
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->and_(*addr, 0xffff);
	c->shl(*addr, 4);
	c->movdqa(vr, asmjit::x86::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::FREST(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->rcpps(va, va);
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FRSQEST(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->andps(va, XmmConst(_mm_set1_epi32(0x7fffffff))); // abs
	c->rsqrtps(va, va);
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::LQX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0x3fff0);

	const XmmLink& vt = XmmAlloc();
	c->movdqa(vt, asmjit::x86::oword_ptr(*ls, *addr));
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
	c->unuse(*addr);
}

void spu_recompiler::ROTQBYBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.rldq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0xf << 3);
	c->shl(*addr, 1);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::ROTQMBYBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.srdq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->shr(*addr, 3);
	c->neg(*addr);
	c->and_(*addr, 0x1f);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::SHLQBYBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.sldq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0x1f << 3);
	c->shl(*addr, 1);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::CBX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->not_(*addr);
	c->and_(*addr, 0xf);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::byte_ptr(*cpu, *addr, 0, offset32(&SPUThread::gpr, op.rt)), 0x03);
	c->unuse(*addr);
}

void spu_recompiler::CHX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->not_(*addr);
	c->and_(*addr, 0xe);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::word_ptr(*cpu, *addr, 0, offset32(&SPUThread::gpr, op.rt)), 0x0203);
	c->unuse(*addr);
}

void spu_recompiler::CWX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->not_(*addr);
	c->and_(*addr, 0xc);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::dword_ptr(*cpu, *addr, 0, offset32(&SPUThread::gpr, op.rt)), 0x00010203);
	c->unuse(*addr);
}

void spu_recompiler::CDX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->add(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->not_(*addr);
	c->and_(*addr, 0x8);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(*qw0, asmjit::imm_u(0x0001020304050607));
	c->mov(asmjit::x86::qword_ptr(*cpu, *addr, 0, offset32(&SPUThread::gpr, op.rt)), *qw0);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::ROTQBI(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr, op.ra, &v128::_u64, 0));
	c->mov(*qw1, SPU_OFF_64(gpr, op.ra, &v128::_u64, 1));
	c->mov(*qw2, *qw0);
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 7);
	c->shld(*qw0, *qw1, *addr);
	c->shld(*qw1, *qw2, *addr);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 0), *qw0);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 1), *qw1);
	c->unuse(*addr);
	c->unuse(*qw0);
	c->unuse(*qw1);
	c->unuse(*qw2);
}

void spu_recompiler::ROTQMBI(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr, op.ra, &v128::_u64, 0));
	c->mov(*qw1, SPU_OFF_64(gpr, op.ra, &v128::_u64, 1));
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->neg(*addr);
	c->and_(*addr, 7);
	c->shrd(*qw0, *qw1, *addr);
	c->shr(*qw1, *addr);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 0), *qw0);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 1), *qw1);
	c->unuse(*addr);
	c->unuse(*qw0);
	c->unuse(*qw1);
}

void spu_recompiler::SHLQBI(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr, op.ra, &v128::_u64, 0));
	c->mov(*qw1, SPU_OFF_64(gpr, op.ra, &v128::_u64, 1));
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 7);
	c->shld(*qw1, *qw0, *addr);
	c->shl(*qw0, *addr);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 0), *qw0);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 1), *qw1);
	c->unuse(*addr);
	c->unuse(*qw0);
	c->unuse(*qw1);
}

void spu_recompiler::ROTQBY(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.rldq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0xf);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::ROTQMBY(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.srdq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->neg(*addr);
	c->and_(*addr, 0x1f);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::SHLQBY(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->mov(*qw0, asmjit::imm_ptr((void*)g_spu_imm.sldq_pshufb));
	c->mov(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));
	c->and_(*addr, 0x1f);
	c->shl(*addr, 4);
	c->pshufb(va, asmjit::x86::oword_ptr(*qw0, *addr));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::ORX(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 0));
	c->or_(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 1));
	c->or_(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 2));
	c->or_(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, 3), *addr);
	c->xor_(*addr, *addr);
	c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, 0), *addr);
	c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, 1), *addr);
	c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, 2), *addr);
	c->unuse(*addr);
}

void spu_recompiler::CBD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u8r[op.i7 & 0xf] = 0x03;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.i7) c->add(*addr, op.i7);
	c->not_(*addr);
	c->and_(*addr, 0xf);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::byte_ptr(*cpu, *addr, 0, offset32(&SPUThread::gpr, op.rt)), 0x03);
	c->unuse(*addr);
}

void spu_recompiler::CHD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u16r[(op.i7 >> 1) & 0x7] = 0x0203;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.i7) c->add(*addr, op.i7);
	c->not_(*addr);
	c->and_(*addr, 0xe);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::word_ptr(*cpu, *addr, 0, offset32(&SPUThread::gpr, op.rt)), 0x0203);
	c->unuse(*addr);
}

void spu_recompiler::CWD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u32r[(op.i7 >> 2) & 0x3] = 0x00010203;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.i7) c->add(*addr, op.i7);
	c->not_(*addr);
	c->and_(*addr, 0xc);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(asmjit::x86::dword_ptr(*cpu, *addr, 0, offset32(&SPUThread::gpr, op.rt)), 0x00010203);
	c->unuse(*addr);
}

void spu_recompiler::CDD(spu_opcode_t op)
{
	//if (op.ra == 1)
	//{
	//	// assuming that SP % 16 is always zero
	//	const XmmLink& vr = XmmAlloc();
	//	v128 value = v128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//	value.u64r[(op.i7 >> 3) & 0x1] = 0x0001020304050607ull;
	//	c->movdqa(vr, XmmConst(value));
	//	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	//	return;
	//}

	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.i7) c->add(*addr, op.i7);
	c->not_(*addr);
	c->and_(*addr, 0x8);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->mov(*qw0, asmjit::imm_u(0x0001020304050607));
	c->mov(asmjit::x86::qword_ptr(*cpu, *addr, 0, offset32(&SPUThread::gpr, op.rt)), *qw0);
	c->unuse(*addr);
	c->unuse(*qw0);
}

void spu_recompiler::ROTQBII(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr, op.ra, &v128::_u64, 0));
	c->mov(*qw1, SPU_OFF_64(gpr, op.ra, &v128::_u64, 1));
	c->mov(*qw2, *qw0);
	c->shld(*qw0, *qw1, op.i7 & 0x7);
	c->shld(*qw1, *qw2, op.i7 & 0x7);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 0), *qw0);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 1), *qw1);
	c->unuse(*qw0);
	c->unuse(*qw1);
	c->unuse(*qw2);
}

void spu_recompiler::ROTQMBII(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr, op.ra, &v128::_u64, 0));
	c->mov(*qw1, SPU_OFF_64(gpr, op.ra, &v128::_u64, 1));
	c->shrd(*qw0, *qw1, 0-op.i7 & 0x7);
	c->shr(*qw1, 0-op.i7 & 0x7);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 0), *qw0);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 1), *qw1);
	c->unuse(*qw0);
	c->unuse(*qw1);
}

void spu_recompiler::SHLQBII(spu_opcode_t op)
{
	c->mov(*qw0, SPU_OFF_64(gpr, op.ra, &v128::_u64, 0));
	c->mov(*qw1, SPU_OFF_64(gpr, op.ra, &v128::_u64, 1));
	c->shld(*qw1, *qw0, op.i7 & 0x7);
	c->shl(*qw0, op.i7 & 0x7);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 0), *qw0);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_u64, 1), *qw1);
	c->unuse(*qw0);
	c->unuse(*qw1);
}

void spu_recompiler::ROTQBYI(spu_opcode_t op)
{
	const int s = op.i7 & 0xf;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->palignr(va, va, 16 - s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ROTQMBYI(spu_opcode_t op)
{
	const int s = 0-op.i7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psrldq(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SHLQBYI(spu_opcode_t op)
{
	const int s = op.i7 & 0x1f;
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pslldq(va, s);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::NOP(spu_opcode_t op)
{
}

void spu_recompiler::CGT(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtd(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::XOR(spu_opcode_t op)
{
	// xor
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CGTH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtw(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::EQV(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pxor(vb, XmmConst(_mm_set1_epi32(0xffffffff)));
	c->pxor(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::CGTB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtb(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SUMB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(_mm_set1_epi8(1)));
	c->pmaddubsw(va, vi);
	c->pmaddubsw(vb, vi);
	c->phaddw(va, vb);
	c->pshufb(va, XmmConst(_mm_set_epi8(15, 14, 7, 6, 13, 12, 5, 4, 11, 10, 3, 2, 9, 8, 1, 0)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

//HGT uses signed values.  HLGT uses unsigned values
void spu_recompiler::HGT(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_s32, 3));
	c->cmp(*addr, SPU_OFF_32(gpr, op.rb, &v128::_s32, 3));

	c->mov(*addr, m_pos | 0x1000000);
	c->jg(*end);
	c->unuse(*addr);
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

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(u32*, const u32*)>(body)), asmjit::FuncSignature2<void, void*, void*>(asmjit::CallConv::kIdHost));
	call->setArg(0, *qw0);
	call->setArg(1, *qw1);

	//c->mov(*qw0, 32 + 31);
	//for (u32 i = 0; i < 4; i++) // unrolled loop
	//{
	//	c->bsr(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, i));
	//	c->cmovz(*addr, qw0->r32());
	//	c->xor_(*addr, 31);
	//	c->mov(SPU_OFF_32(gpr, op.rt, &v128::_u32, i), *addr);
	//}
}

void spu_recompiler::XSWD(spu_opcode_t op)
{
	c->movsxd(*qw0, SPU_OFF_32(gpr, op.ra, &v128::_s32, 0));
	c->movsxd(*qw1, SPU_OFF_32(gpr, op.ra, &v128::_s32, 2));
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_s64, 0), *qw0);
	c->mov(SPU_OFF_64(gpr, op.rt, &v128::_s64, 1), *qw1);
	c->unuse(*qw0);
	c->unuse(*qw1);
}

void spu_recompiler::XSHW(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pslld(va, 16);
	c->psrad(va, 16);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CNTB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	const XmmLink& vm = XmmAlloc();
	c->movdqa(v1, va);
	c->psrlq(v1, 4);
	c->movdqa(vm, XmmConst(_mm_set1_epi8(0xf)));
	c->pand(va, vm);
	c->pand(v1, vm);
	c->movdqa(vm, XmmConst(_mm_set_epi8(4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0)));
	c->pshufb(vm, va);
	c->movdqa(va, XmmConst(_mm_set_epi8(4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0)));
	c->pshufb(va, v1);
	c->paddb(va, vm);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::XSBH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psllw(va, 8);
	c->psraw(va, 8);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CLGT(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(_mm_set1_epi32(0x80000000)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr, op.rb));
	c->pcmpgtd(va, vi);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ANDC(spu_opcode_t op)
{
	// and not
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pandn(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::FCGT(spu_opcode_t op)
{
	const auto last_exp_bit = XmmConst(_mm_set1_epi32(0x00800000));
	const auto all_exp_bits = XmmConst(_mm_set1_epi32(0x7f800000));

	const XmmLink& tmp0 = XmmAlloc();
	const XmmLink& tmp1 = XmmAlloc();
	const XmmLink& tmp2 = XmmAlloc();
	const XmmLink& tmp3 = XmmAlloc();
	const XmmLink& tmpv = XmmAlloc();

	c->pxor(tmp0, tmp0);
	c->pxor(tmp1, tmp1);
	c->cmpps(tmp0, SPU_OFF_128(gpr, op.ra), 3);  //tmp0 is true if a is extended (nan/inf)
	c->cmpps(tmp1, SPU_OFF_128(gpr, op.rb), 3);  //tmp1 is true if b is extended (nan/inf)

	//compute lower a and b
	c->movaps(tmp2, last_exp_bit);
	c->movaps(tmp3, last_exp_bit);
	c->pandn(tmp2, SPU_OFF_128(gpr, op.ra));  //tmp2 = lowered_a
	c->pandn(tmp3, SPU_OFF_128(gpr, op.rb));  //tmp3 = lowered_b

	//lower a if extended
	c->movaps(tmpv, tmp0);
	c->pand(tmpv, tmp2);
	c->pandn(tmp0, SPU_OFF_128(gpr, op.ra));
	c->orps(tmp0, tmpv);

	//lower b if extended
	c->movaps(tmpv, tmp1);
	c->pand(tmpv, tmp3);
	c->pandn(tmp1, SPU_OFF_128(gpr, op.rb));
	c->orps(tmp1, tmpv);

	//flush a to 0 if denormalized
	c->pxor(tmpv, tmpv);
	c->movaps(tmp2, SPU_OFF_128(gpr, op.ra));
	c->andps(tmp2, all_exp_bits);
	c->cmpps(tmp2, tmpv, 0);
	c->pandn(tmp2, tmp0);

	//flush b to 0 if denormalized
	c->movaps(tmp3, SPU_OFF_128(gpr, op.rb));
	c->andps(tmp3, all_exp_bits);
	c->cmpps(tmp3, tmpv, 0);
	c->pandn(tmp3, tmp1);

	c->cmpps(tmp3, tmp2, 1);
	c->movaps(SPU_OFF_128(gpr, op.rt), tmp3);
}

void spu_recompiler::DFCGT(spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_recompiler::FA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->addps(va, SPU_OFF_128(gpr, op.rb));
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->subps(va, SPU_OFF_128(gpr, op.rb));
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FM(spu_opcode_t op)
{
	const auto sign_bits = XmmConst(_mm_set1_epi32(0x80000000));
	const auto all_exp_bits = XmmConst(_mm_set1_epi32(0x7f800000));

	const XmmLink& tmp0 = XmmAlloc();
	const XmmLink& tmp1 = XmmAlloc();
	const XmmLink& tmp2 = XmmAlloc();
	const XmmLink& tmp3 = XmmAlloc();
	const XmmLink& tmp4 = XmmGet(op.ra, XmmType::Float);
	const XmmLink& tmp5 = XmmGet(op.rb, XmmType::Float);

	//check denormals
	c->pxor(tmp0, tmp0);
	c->movaps(tmp1, all_exp_bits);
	c->movaps(tmp2, all_exp_bits);
	c->andps(tmp1, tmp4);
	c->andps(tmp2, tmp5);
	c->cmpps(tmp1, tmp0, 0);
	c->cmpps(tmp2, tmp0, 0);
	c->orps(tmp1, tmp2);  //denormal operand mask

	//compute result with flushed denormal inputs
	c->movaps(tmp2, tmp4);
	c->mulps(tmp2, tmp5); //primary result
	c->movaps(tmp3, tmp2);
	c->andps(tmp3, all_exp_bits);
	c->cmpps(tmp3, tmp0, 0); //denom mask from result
	c->orps(tmp3, tmp1);
	c->andnps(tmp3, tmp2); //flushed result

	//compute results for the extended path
	c->andps(tmp2, all_exp_bits);
	c->cmpps(tmp2, all_exp_bits, 0); //extended mask
	c->movaps(tmp4, sign_bits);
	c->movaps(tmp5, sign_bits);
	c->movaps(tmp0, sign_bits);
	c->andps(tmp4, SPU_OFF_128(gpr, op.ra));
	c->andps(tmp5, SPU_OFF_128(gpr, op.rb));
	c->xorps(tmp4, tmp5); //sign mask
	c->pandn(tmp0, tmp2);
	c->orps(tmp4, tmp0); //add result sign back to original extended value
	c->movaps(tmp5, tmp1); //denormal mask (operands)
	c->andnps(tmp5, tmp4); //max_float with sign bit (nan/-nan) where not denormal or zero

	//select result
	c->movaps(tmp0, tmp2);
	c->andnps(tmp0, tmp3);
	c->andps(tmp2, tmp5);
	c->orps(tmp0, tmp2);
	c->movaps(SPU_OFF_128(gpr, op.rt), tmp0);
}

void spu_recompiler::CLGTH(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(_mm_set1_epi16(INT16_MIN)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr, op.rb));
	c->pcmpgtw(va, vi);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ORC(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pxor(vb, XmmConst(_mm_set1_epi32(0xffffffff)));
	c->por(vb, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::FCMGT(spu_opcode_t op)
{
	// reverted less-than
	// since comparison is absoulte, a > b if a is extended and b is not extended
	// flush denormals to zero to make zero == zero work
	const auto last_exp_bit = XmmConst(_mm_set1_epi32(0x00800000));
	const auto all_exp_bits = XmmConst(_mm_set1_epi32(0x7f800000));
	const auto remove_sign_bits = XmmConst(_mm_set1_epi32(0x7fffffff));

	const XmmLink& tmp0 = XmmAlloc();
	const XmmLink& tmp1 = XmmAlloc();
	const XmmLink& tmp2 = XmmAlloc();
	const XmmLink& tmp3 = XmmAlloc();
	const XmmLink& tmpv = XmmAlloc();

	c->pxor(tmp0, tmp0);
	c->pxor(tmp1, tmp1);
	c->cmpps(tmp0, SPU_OFF_128(gpr, op.ra), 3);  //tmp0 is true if a is extended (nan/inf)
	c->cmpps(tmp1, SPU_OFF_128(gpr, op.rb), 3);  //tmp1 is true if b is extended (nan/inf)

	//flush a to 0 if denormalized
	c->pxor(tmpv, tmpv);
	c->movaps(tmp2, SPU_OFF_128(gpr, op.ra));
	c->andps(tmp2, all_exp_bits);
	c->cmpps(tmp2, tmpv, 0);
	c->pandn(tmp2, SPU_OFF_128(gpr, op.ra));

	//flush b to 0 if denormalized
	c->movaps(tmp3, SPU_OFF_128(gpr, op.rb));
	c->andps(tmp3, all_exp_bits);
	c->cmpps(tmp3, tmpv, 0);
	c->pandn(tmp3, SPU_OFF_128(gpr, op.rb));

	//Set tmp1 to true where a is extended but b is not extended
	//This is a simplification since absolute values remove necessity of lowering
	c->xorps(tmp0, tmp1);   //tmp0 is true when either a or b is extended
	c->pandn(tmp1, tmp0);   //tmp1 is true if b is not extended and a is extended

	c->andps(tmp2, remove_sign_bits);
	c->andps(tmp3, remove_sign_bits);
	c->cmpps(tmp3, tmp2, 1);
	c->orps(tmp3, tmp1);    //Force result to all true if a is extended but b is not
	c->movaps(SPU_OFF_128(gpr, op.rt), tmp3);
}

void spu_recompiler::DFCMGT(spu_opcode_t op)
{
	const auto mask = XmmConst(_mm_set1_epi64x(0x7fffffffffffffff));
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Double);

	c->andpd(va, mask);
	c->andpd(vb, mask);
	c->cmppd(vb, va, 1);
	c->movaps(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::DFA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->addpd(va, SPU_OFF_128(gpr, op.rb));
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::DFS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->subpd(va, SPU_OFF_128(gpr, op.rb));
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::DFM(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr, op.rb));
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CLGTB(spu_opcode_t op)
{
	// compare if-greater-than
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(_mm_set1_epi8(INT8_MIN)));
	c->pxor(va, vi);
	c->pxor(vi, SPU_OFF_128(gpr, op.rb));
	c->pcmpgtb(va, vi);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::HLGT(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->cmp(*addr, SPU_OFF_32(gpr, op.rb, &v128::_u32, 3));

	c->mov(*addr, m_pos | 0x1000000);
	c->ja(*end);
	c->unuse(*addr);
}

void spu_recompiler::DFMA(spu_opcode_t op)
{
	const XmmLink& vr = XmmGet(op.rt, XmmType::Double);
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr, op.rb));
	c->addpd(vr, va);
	c->movapd(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::DFMS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	const XmmLink& vt = XmmGet(op.rt, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr, op.rb));
	c->subpd(va, vt);
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::DFNMS(spu_opcode_t op)
{
	const XmmLink& vr = XmmGet(op.rt, XmmType::Double);
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr, op.rb));
	c->subpd(vr, va);
	c->movapd(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::DFNMA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	const XmmLink& vt = XmmGet(op.rt, XmmType::Double);
	c->mulpd(va, SPU_OFF_128(gpr, op.rb));
	c->addpd(vt, va);
	c->xorpd(va, va);
	c->subpd(va, vt);
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQ(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqd(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::MPYHHU(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& va2 = XmmAlloc();
	c->movdqa(va2, va);
	c->pmulhuw(va, vb);
	c->pmullw(va2, vb);
	c->pand(va, XmmConst(_mm_set1_epi32(0xffff0000)));
	c->psrld(va2, 16);
	c->por(va, va2);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ADDX(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pand(vt, XmmConst(_mm_set1_epi32(1)));
	c->paddd(vt, SPU_OFF_128(gpr, op.ra));
	c->paddd(vt, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::SFX(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pandn(vt, XmmConst(_mm_set1_epi32(1)));
	c->psubd(vb, SPU_OFF_128(gpr, op.ra));
	c->psubd(vb, vt);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vb);
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

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	c->lea(*qw2, SPU_OFF_128(gpr, op.rb));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(u32*, const u32*, const u32*)>(body)), asmjit::FuncSignature3<void, void*, void*, void*>(asmjit::CallConv::kIdHost));
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

	c->lea(*qw0, SPU_OFF_128(gpr, op.rt));
	c->lea(*qw1, SPU_OFF_128(gpr, op.ra));
	c->lea(*qw2, SPU_OFF_128(gpr, op.rb));
	asmjit::CCFuncCall* call = c->call(asmjit::imm_ptr(asmjit::Internal::ptr_cast<void*, void(u32*, const u32*, const u32*)>(body)), asmjit::FuncSignature3<void, void*, void*, void*>(asmjit::CallConv::kIdHost));
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
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
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
	c->pand(va, XmmConst(_mm_set1_epi32(0xffff0000)));
	c->psrld(va2, 16);
	c->paddd(vt, va);
	c->paddd(vt, va2);
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::FSCRRD(spu_opcode_t op)
{
	// zero (hack)
	const XmmLink& v0 = XmmAlloc();
	c->pxor(v0, v0);
	c->movdqa(SPU_OFF_128(gpr, op.rt), v0);
}

void spu_recompiler::FESD(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	c->shufps(va, va, 0x8d); // _f[0] = _f[1]; _f[1] = _f[3];
	c->cvtps2pd(va, va);
	c->movapd(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FRDS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Double);
	c->cvtpd2ps(va, va);
	c->shufps(va, va, 0x72); // _f[1] = _f[0]; _f[3] = _f[1]; _f[0] = _f[2] = 0;
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FSCRWR(spu_opcode_t op)
{
	// nop (not implemented)
}

void spu_recompiler::DFTSV(spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_recompiler::FCEQ(spu_opcode_t op)
{
	// compare equal
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	c->cmpps(vb, SPU_OFF_128(gpr, op.ra), 0);
	c->movaps(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::DFCEQ(spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_recompiler::MPY(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(_mm_set1_epi32(0xffff)));
	c->pand(va, vi);
	c->pand(vb, vi);
	c->pmaddwd(va, vb);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::MPYH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psrld(va, 16);
	c->pmullw(va, vb);
	c->pslld(va, 16);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::MPYHH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->psrld(va, 16);
	c->psrld(vb, 16);
	c->pmaddwd(va, vb);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::MPYS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	c->pmulhw(va, vb);
	c->pslld(va, 16);
	c->psrad(va, 16);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQH(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqw(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FCMEQ(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	const XmmLink& vi = XmmAlloc();
	c->movaps(vi, XmmConst(_mm_set1_epi32(0x7fffffff)));
	c->andps(vb, vi); // abs
	c->andps(vi, SPU_OFF_128(gpr, op.ra));
	c->cmpps(vb, vi, 0); // ==
	c->movaps(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::DFCMEQ(spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
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
	c->pand(va2, XmmConst(_mm_set1_epi32(0xffff)));
	c->por(va, va2);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQB(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqb(va, SPU_OFF_128(gpr, op.rb));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::FI(spu_opcode_t op)
{
	// Floating Interpolate
	const XmmLink& vb = XmmGet(op.rb, XmmType::Float);
	c->movaps(SPU_OFF_128(gpr, op.rt), vb);
}

void spu_recompiler::HEQ(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_s32, 3));
	c->cmp(*addr, SPU_OFF_32(gpr, op.rb, &v128::_s32, 3));

	c->mov(*addr, m_pos | 0x1000000);
	c->je(*end);
	c->unuse(*addr);
}

void spu_recompiler::CFLTS(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vi = XmmAlloc();
	if (op.i8 != 173) c->mulps(va, XmmConst(_mm_set1_ps(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))))); // scale
	c->movaps(vi, XmmConst(_mm_set1_ps(std::exp2(31.f))));
	c->cmpps(vi, va, 2);
	c->cvttps2dq(va, va); // convert to ints with truncation
	c->pxor(va, vi); // fix result saturation (0x80000000 -> 0x7fffffff)
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CFLTU(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vs = XmmAlloc();
	const XmmLink& vs2 = XmmAlloc();
	const XmmLink& vs3 = XmmAlloc();
	if (op.i8 != 173) c->mulps(va, XmmConst(_mm_set1_ps(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))))); // scale
	c->maxps(va, XmmConst(_mm_set1_ps(0.0f))); // saturate
	c->movaps(vs, va); // copy scaled value
	c->movaps(vs2, va);
	c->movaps(vs3, XmmConst(_mm_set1_ps(std::exp2(31.f))));
	c->subps(vs2, vs3);
	c->cmpps(vs3, vs, 2);
	c->andps(vs2, vs3);
	c->cvttps2dq(va, va);
	c->cmpps(vs, XmmConst(_mm_set1_ps(std::exp2(32.f))), 5);
	c->cvttps2dq(vs2, vs2);
	c->por(va, vs);
	c->por(va, vs2);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CSFLT(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->cvtdq2ps(va, va); // convert to floats
	if (op.i8 != 155) c->mulps(va, XmmConst(_mm_set1_ps(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))))); // scale
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CUFLT(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	c->movdqa(v1, va);
	c->pand(va, XmmConst(_mm_set1_epi32(0x7fffffff)));
	c->cvtdq2ps(va, va); // convert to floats
	c->psrad(v1, 31); // generate mask from sign bit
	c->andps(v1, XmmConst(_mm_set1_ps(std::exp2(31.f)))); // generate correction component
	c->addps(va, v1); // add correction component
	if (op.i8 != 155) c->mulps(va, XmmConst(_mm_set1_ps(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))))); // scale
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::BRZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	c->cmp(SPU_OFF_32(gpr, op.rt, &v128::_u32, 3), 0);

	if (labels[target / 4].isValid())
	{
		c->je(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brz 0x%x)", target);
		}

		c->mov(*addr, target);
		c->je(*end);
		c->unuse(*addr);
	}
}

void spu_recompiler::STQA(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(asmjit::x86::oword_ptr(*ls, spu_ls_target(0, op.i16)), vt);
}

void spu_recompiler::BRNZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	c->cmp(SPU_OFF_32(gpr, op.rt, &v128::_u32, 3), 0);

	if (labels[target / 4].isValid())
	{
		c->jne(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brnz 0x%x)", target);
		}

		c->mov(*addr, target);
		c->jne(*end);
		c->unuse(*addr);
	}
}

void spu_recompiler::BRHZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	c->cmp(SPU_OFF_16(gpr, op.rt, &v128::_u16, 6), 0);

	if (labels[target / 4].isValid())
	{
		c->je(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brhz 0x%x)", target);
		}

		c->mov(*addr, target);
		c->je(*end);
		c->unuse(*addr);
	}
}

void spu_recompiler::BRHNZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	c->cmp(SPU_OFF_16(gpr, op.rt, &v128::_u16, 6), 0);

	if (labels[target / 4].isValid())
	{
		c->jne(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brhnz 0x%x)", target);
		}

		c->mov(*addr, target);
		c->jne(*end);
		c->unuse(*addr);
	}
}

void spu_recompiler::STQR(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(asmjit::x86::oword_ptr(*ls, spu_ls_target(m_pos, op.i16)), vt);
}

void spu_recompiler::BRA(spu_opcode_t op)
{
	const u32 target = spu_branch_target(0, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	if (labels[target / 4].isValid())
	{
		c->jmp(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (bra 0x%x)", target);
		}

		c->mov(*addr, target);
		c->jmp(*end);
		c->unuse(*addr);
	}
}

void spu_recompiler::LQA(spu_opcode_t op)
{
	const XmmLink& vt = XmmAlloc();
	c->movdqa(vt, asmjit::x86::oword_ptr(*ls, spu_ls_target(0, op.i16)));
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::BRASL(spu_opcode_t op)
{
	const u32 target = spu_branch_target(0, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(spu_branch_target(m_pos + 4), 0, 0, 0)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->unuse(vr);

	c->mov(SPU_OFF_32(pc), target);

	FunctionCall();
}

void spu_recompiler::BR(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos)
	{
		c->mov(*addr, target | 0x2000000);
		//c->cmp(asmjit::x86::dword_ptr(*ls, m_pos), 0x32); // compare instruction opcode with BR-to-self
		//c->je(labels[target / 4]);
		c->lock().or_(SPU_OFF_32(state), static_cast<u32>(cpu_flag::stop + cpu_flag::ret));
		c->jmp(*end);
		c->unuse(*addr);
		return;
	}

	if (labels[target / 4].isValid())
	{
		c->jmp(labels[target / 4]);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brz 0x%x)", target);
		}

		c->mov(*addr, target);
		c->jmp(*end);
		c->unuse(*addr);
	}
}

void spu_recompiler::FSMBI(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(g_spu_imm.fsmb[op.i16]));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::BRSL(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set_epi32(spu_branch_target(m_pos + 4), 0, 0, 0)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
	c->unuse(vr);

	if (target == spu_branch_target(m_pos + 4))
	{
		// branch-to-next
		return;
	}

	c->mov(SPU_OFF_32(pc), target);

	FunctionCall();
}

void spu_recompiler::LQR(spu_opcode_t op)
{
	const XmmLink& vt = XmmAlloc();
	c->movdqa(vt, asmjit::x86::oword_ptr(*ls, spu_ls_target(m_pos, op.i16)));
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::IL(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set1_epi32(op.si16)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::ILHU(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set1_epi32(op.i16 << 16)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::ILH(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set1_epi16(op.i16)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::IOHL(spu_opcode_t op)
{
	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->por(vt, XmmConst(_mm_set1_epi32(op.i16)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
}

void spu_recompiler::ORI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	if (op.si10) c->por(va, XmmConst(_mm_set1_epi32(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ORHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->por(va, XmmConst(_mm_set1_epi16(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ORBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->por(va, XmmConst(_mm_set1_epi8(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::SFI(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set1_epi32(op.si10)));
	c->psubd(vr, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::SFHI(spu_opcode_t op)
{
	const XmmLink& vr = XmmAlloc();
	c->movdqa(vr, XmmConst(_mm_set1_epi16(op.si10)));
	c->psubw(vr, SPU_OFF_128(gpr, op.ra));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::ANDI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, XmmConst(_mm_set1_epi32(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ANDHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, XmmConst(_mm_set1_epi16(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::ANDBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pand(va, XmmConst(_mm_set1_epi8(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::AI(spu_opcode_t op)
{
	// add
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->paddd(va, XmmConst(_mm_set1_epi32(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::AHI(spu_opcode_t op)
{
	// add
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->paddw(va, XmmConst(_mm_set1_epi16(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::STQD(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.si10) c->add(*addr, op.si10 << 4);
	c->and_(*addr, 0x3fff0);

	const XmmLink& vt = XmmGet(op.rt, XmmType::Int);
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(asmjit::x86::oword_ptr(*ls, *addr), vt);
	c->unuse(*addr);
}

void spu_recompiler::LQD(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	if (op.si10) c->add(*addr, op.si10 << 4);
	c->and_(*addr, 0x3fff0);

	const XmmLink& vt = XmmAlloc();
	c->movdqa(vt, asmjit::x86::oword_ptr(*ls, *addr));
	c->pshufb(vt, XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vt);
	c->unuse(*addr);
}

void spu_recompiler::XORI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(_mm_set1_epi32(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::XORHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(_mm_set1_epi16(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::XORBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(_mm_set1_epi8(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CGTI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtd(va, XmmConst(_mm_set1_epi32(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CGTHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtw(va, XmmConst(_mm_set1_epi16(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CGTBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpgtb(va, XmmConst(_mm_set1_epi8(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::HGTI(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_s32, 3));
	c->cmp(*addr, op.si10);

	c->mov(*addr, m_pos | 0x1000000);
	c->jg(*end);
	c->unuse(*addr);
}

void spu_recompiler::CLGTI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(_mm_set1_epi32(0x80000000)));
	c->pcmpgtd(va, XmmConst(_mm_set1_epi32(op.si10 - 0x80000000)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CLGTHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pxor(va, XmmConst(_mm_set1_epi16(INT16_MIN)));
	c->pcmpgtw(va, XmmConst(_mm_set1_epi16(op.si10 - 0x8000)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CLGTBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->psubb(va, XmmConst(_mm_set1_epi8(INT8_MIN)));
	c->pcmpgtb(va, XmmConst(_mm_set1_epi8(op.si10 - 0x80)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::HLGTI(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->cmp(*addr, op.si10);

	c->mov(*addr, m_pos | 0x1000000);
	c->ja(*end);
	c->unuse(*addr);
}

void spu_recompiler::MPYI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pmaddwd(va, XmmConst(_mm_set1_epi32(op.si10 & 0xffff)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::MPYUI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	const XmmLink& va2 = XmmAlloc();
	c->movdqa(va2, va);
	c->movdqa(vi, XmmConst(_mm_set1_epi32(op.si10 & 0xffff)));
	c->pmulhuw(va, vi);
	c->pmullw(va2, vi);
	c->pslld(va, 16);
	c->por(va, va2);
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqd(va, XmmConst(_mm_set1_epi32(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQHI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqw(va, XmmConst(_mm_set1_epi16(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::CEQBI(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->pcmpeqb(va, XmmConst(_mm_set1_epi8(op.si10)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
}

void spu_recompiler::HEQI(spu_opcode_t op)
{
	c->mov(*addr, SPU_OFF_32(gpr, op.ra, &v128::_u32, 3));
	c->cmp(*addr, op.si10);

	c->mov(*addr, m_pos | 0x1000000);
	c->je(*end);
	c->unuse(*addr);
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
	c->movdqa(vr, XmmConst(_mm_set1_epi32(op.i18)));
	c->movdqa(SPU_OFF_128(gpr, op.rt), vr);
}

void spu_recompiler::SELB(spu_opcode_t op)
{
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vc = XmmGet(op.rc, XmmType::Int);
	c->pand(vb, vc);
	c->pandn(vc, SPU_OFF_128(gpr, op.ra));
	c->por(vb, vc);
	c->movdqa(SPU_OFF_128(gpr, op.rt4), vb);
}

void spu_recompiler::SHUFB(spu_opcode_t op)
{
	const XmmLink& v0 = XmmGet(op.rc, XmmType::Int); // v0 = mask
	const XmmLink& v1 = XmmAlloc();
	const XmmLink& v2 = XmmAlloc();
	const XmmLink& v3 = XmmAlloc();
	const XmmLink& v4 = XmmAlloc();
	const XmmLink& vFF = XmmAlloc();
	c->movdqa(v2, v0); // v2 = mask
	// generate specific values:
	c->movdqa(v1, XmmConst(_mm_set1_epi8(-0x20))); // v1 = 11100000
	c->movdqa(v3, XmmConst(_mm_set1_epi8(-0x80))); // v3 = 10000000
	c->pand(v2, v1); // filter mask      v2 = mask & 11100000
	c->movdqa(vFF, v2); // and copy      vFF = mask & 11100000
	c->movdqa(v4, XmmConst(_mm_set1_epi8(-0x40))); // v4 = 11000000
	c->pcmpeqb(vFF, v4); // gen 0xff     vFF = (mask & 11100000 == 11000000) ? 0xff : 0
	c->movdqa(v4, v2); // copy again     v4 = mask & 11100000
	c->pand(v4, v3); // filter mask      v4 = mask & 10000000
	c->pcmpeqb(v2, v1); //               v2 = (mask & 11100000 == 11100000) ? 0xff : 0
	c->pcmpeqb(v4, v3); //               v4 = (mask & 10000000 == 10000000) ? 0xff : 0
	c->pand(v2, v3); // generate 0x80    v2 = (mask & 11100000 == 11100000) ? 0x80 : 0
	c->por(vFF, v2); // merge 0xff, 0x80 vFF = (mask & 11100000 == 11000000) ? 0xff : (mask & 11100000 == 11100000) ? 0x80 : 0
	c->pandn(v1, v0); // filter mask     v1 = mask & 00011111
	// select bytes from [op.rb]:
	c->movdqa(v2, XmmConst(_mm_set1_epi8(0x0f))); //   v2 = 00001111
	c->pxor(v1, XmmConst(_mm_set1_epi8(0x10))); //   v1 = (mask & 00011111) ^ 00010000
	c->psubb(v2, v1); //                 v2 = 00001111 - ((mask & 00011111) ^ 00010000)
	c->movdqa(v1, SPU_OFF_128(gpr, op.rb)); //        v1 = op.rb
	c->pshufb(v1, v2); //                v1 = select(op.rb, 00001111 - ((mask & 00011111) ^ 00010000))
	// select bytes from [op.ra]:
	c->pxor(v2, XmmConst(_mm_set1_epi8(-0x10))); //   v2 = (00001111 - ((mask & 00011111) ^ 00010000)) ^ 11110000
	c->movdqa(v3, SPU_OFF_128(gpr, op.ra)); //        v3 = op.ra
	c->pshufb(v3, v2); //                v3 = select(op.ra, (00001111 - ((mask & 00011111) ^ 00010000)) ^ 11110000)
	c->por(v1, v3); //                   v1 = select(op.rb, 00001111 - ((mask & 00011111) ^ 00010000)) | (v3)
	c->pandn(v4, v1); // filter result   v4 = v1 & ((mask & 10000000 == 10000000) ? 0 : 0xff)
	c->por(vFF, v4); // final merge      vFF = (mask & 10000000 == 10000000) ? ((mask & 11100000 == 11000000) ? 0xff : (mask & 11100000 == 11100000) ? 0x80 : 0) : (v1)
	c->movdqa(SPU_OFF_128(gpr, op.rt4), vFF);
}

void spu_recompiler::MPYA(spu_opcode_t op)
{
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& vb = XmmGet(op.rb, XmmType::Int);
	const XmmLink& vi = XmmAlloc();
	c->movdqa(vi, XmmConst(_mm_set1_epi32(0xffff)));
	c->pand(va, vi);
	c->pand(vb, vi);
	c->pmaddwd(va, vb);
	c->paddd(va, SPU_OFF_128(gpr, op.rc));
	c->movdqa(SPU_OFF_128(gpr, op.rt4), va);
}

void spu_recompiler::FNMS(spu_opcode_t op)
{
	const XmmLink& vc = XmmGet(op.rc, XmmType::Float);
	const auto mask = XmmConst(_mm_set1_epi32(0x7f800000));
	const XmmLink& tmp_a = XmmAlloc();
	const XmmLink& tmp_b = XmmAlloc();

	c->movaps(tmp_a, SPU_OFF_128(gpr, op.ra));
	c->andps(tmp_a, mask);
	c->cmpps(tmp_a, mask, 4);                 //tmp_a = ra == extended
	c->andps(tmp_a, SPU_OFF_128(gpr, op.ra)); //tmp_a = mask_a & ~ra_extended

	c->movaps(tmp_b, SPU_OFF_128(gpr, op.rb));
	c->andps(tmp_b, mask);
	c->cmpps(tmp_b, mask, 4);                 //tmp_b = rb == extended
	c->andps(tmp_b, SPU_OFF_128(gpr, op.rb)); //tmp_b = mask_b & ~rb_extended

	c->mulps(tmp_a, tmp_b);
	c->subps(vc, tmp_a);
	c->movaps(SPU_OFF_128(gpr, op.rt4), vc);
}

void spu_recompiler::FMA(spu_opcode_t op)
{
	const auto mask = XmmConst(_mm_set1_epi32(0x7f800000));
	const XmmLink& tmp_a = XmmAlloc();
	const XmmLink& tmp_b = XmmAlloc();

	c->movaps(tmp_a, SPU_OFF_128(gpr, op.ra));
	c->andps(tmp_a, mask);
	c->cmpps(tmp_a, mask, 4);                 //tmp_a = ra == extended
	c->andps(tmp_a, SPU_OFF_128(gpr, op.ra)); //tmp_a = mask_a & ~ra_extended

	c->movaps(tmp_b, SPU_OFF_128(gpr, op.rb));
	c->andps(tmp_b, mask);
	c->cmpps(tmp_b, mask, 4);                 //tmp_b = rb == extended
	c->andps(tmp_b, SPU_OFF_128(gpr, op.rb)); //tmp_b = mask_b & ~rb_extended

	c->mulps(tmp_a, tmp_b);
	c->addps(tmp_a, SPU_OFF_128(gpr, op.rc));
	c->movaps(SPU_OFF_128(gpr, op.rt4), tmp_a);
}

void spu_recompiler::FMS(spu_opcode_t op)
{
	const auto mask = XmmConst(_mm_set1_epi32(0x7f800000));
	const XmmLink& tmp_a = XmmAlloc();
	const XmmLink& tmp_b = XmmAlloc();

	c->movaps(tmp_a, SPU_OFF_128(gpr, op.ra));
	c->andps(tmp_a, mask);
	c->cmpps(tmp_a, mask, 4);                 //tmp_a = ra == extended
	c->andps(tmp_a, SPU_OFF_128(gpr, op.ra)); //tmp_a = mask_a & ~ra_extended

	c->movaps(tmp_b, SPU_OFF_128(gpr, op.rb));
	c->andps(tmp_b, mask);
	c->cmpps(tmp_b, mask, 4);                 //tmp_b = rb == extended
	c->andps(tmp_b, SPU_OFF_128(gpr, op.rb)); //tmp_b = mask_b & ~rb_extended

	c->mulps(tmp_a, tmp_b);
	c->subps(tmp_a, SPU_OFF_128(gpr, op.rc));
	c->movaps(SPU_OFF_128(gpr, op.rt4), tmp_a);
}

void spu_recompiler::UNK(spu_opcode_t op)
{
	LOG_ERROR(SPU, "0x%05x: Unknown/Illegal opcode (0x%08x)", m_pos, op.opcode);
	c->int3();
}
