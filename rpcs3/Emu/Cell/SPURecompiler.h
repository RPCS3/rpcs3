#pragma once

#define ASMJIT_STATIC

#include "asmjit.h"

using namespace asmjit;
using namespace asmjit::host;

#define UNIMPLEMENTED() UNK(__FUNCTION__)

class SPURecompiler;

class SPURecompilerCore : public CPUDecoder
{
	SPURecompiler* m_enc;
	SPUThread& CPU;

public:
	SPUInterpreter* inter;
	JitRuntime runtime;
	bool first;
	bool need_check;

	struct SPURecEntry
	{
		//u16 host; // absolute position of first instruction of current block (not used now)
		u16 count; // count of instructions compiled from current point (and to be checked)
		u32 valid; // copy of valid opcode for validation
		void* pointer; // pointer to executable memory object
	};

	SPURecEntry entry[0x10000];

	std::vector<u128> imm_table;

	SPURecompilerCore(SPUThread& cpu);

	~SPURecompilerCore();

	void Compile(u16 pos);

	virtual void Decode(const u32 code);

	virtual u32 DecodeMemory(const u32 address);
};

#define c (*compiler)

#define cpu_offsetof(x) static_cast<int32_t>(reinterpret_cast<uintptr_t>(&(((SPUThread*)0)->x)))
#define cpu_xmm(x) oword_ptr(*cpu_var, cpu_offsetof(x))
#define cpu_qword(x) qword_ptr(*cpu_var, cpu_offsetof(x))
#define cpu_dword(x) dword_ptr(*cpu_var, cpu_offsetof(x))
#define cpu_word(x) word_ptr(*cpu_var, cpu_offsetof(x))
#define cpu_byte(x) byte_ptr(*cpu_var, cpu_offsetof(x))

#define g_imm_offsetof(x) static_cast<int32_t>(reinterpret_cast<uintptr_t>(&(((g_spu_imm_table_t*)0)->x)))
#define g_imm_xmm(x) oword_ptr(*g_imm_var, g_imm_offsetof(x))
#define g_imm2_xmm(x, y) oword_ptr(*g_imm_var, y, 0, g_imm_offsetof(x))


#define LOG_OPCODE(...) //ConLog.Write("Compiled "__FUNCTION__"(): "__VA_ARGS__)

#define LOG3_OPCODE(...) //ConLog.Write("Linked "__FUNCTION__"(): "__VA_ARGS__)

#define LOG4_OPCODE(...) //c.addComment(fmt::Format("SPU info: "__FUNCTION__"(): "__VA_ARGS__).c_str())

#define WRAPPER_BEGIN(a0, a1, a2, a3) struct opwr_##a0 \
{ \
	static void opcode(u32 a0, u32 a1, u32 a2, u32 a3) \
{ \
	SPUThread& CPU = *(SPUThread*)GetCurrentNamedThread();

#define WRAPPER_END(a0, a1, a2, a3) /*LOG2_OPCODE();*/ } \
}; \
	/*XmmRelease();*/ \
	if (#a0[0] == 'r') XmmInvalidate(a0); \
	if (#a1[0] == 'r') XmmInvalidate(a1); \
	if (#a2[0] == 'r') XmmInvalidate(a2); \
	if (#a3[0] == 'r') XmmInvalidate(a3); \
	X86CallNode* call##a0 = c.call(imm_ptr(reinterpret_cast<void*>(&opwr_##a0::opcode)), kFuncConvHost, FuncBuilder4<void, u32, u32, u32, u32>()); \
	call##a0->setArg(0, imm_u(a0)); \
	call##a0->setArg(1, imm_u(a1)); \
	call##a0->setArg(2, imm_u(a2)); \
	call##a0->setArg(3, imm_u(a3)); \
	LOG3_OPCODE(/*#a0"=%d, "#a1"=%d, "#a2"=%d, "#a3"=%d", a0, a1, a2, a3*/);


class SPURecompiler : public SPUOpcodes
{
private:
	SPUThread& CPU;
	SPURecompilerCore& rec;

public:
	X86Compiler* compiler;
	bool do_finalize;
	// input:
	X86GpVar* cpu_var;
	X86GpVar* ls_var;
	X86GpVar* imm_var;
	X86GpVar* g_imm_var;
	// output:
	X86GpVar* pos_var;
	// temporary:
	X86GpVar* addr;
	X86GpVar* qw0;
	X86GpVar* qw1;
	X86GpVar* qw2;

	struct XmmLink
	{
		X86XmmVar* data;
		s8 reg;
		bool taken;
		mutable bool got;
		mutable u32 access;

		XmmLink()
			: data(nullptr)
			, reg(-1)
			, taken(false)
			, got(false)
			, access(0)
		{
		}

		const XmmVar& get() const
		{
			assert(data);
			assert(taken);
			if (!taken) throw "XmmLink::get(): wrong use";
			got = true;
			return *data;
		}

		const XmmVar& read() const
		{
			assert(data);
			return *data;
		}
	} xmm_var[16];

	SPURecompiler(SPUThread& cpu, SPURecompilerCore& rec)
		: CPU(cpu)
		, rec(rec)
		, compiler(nullptr)
	{
	}

	const XmmLink& XmmAlloc(s8 pref = -1) // get empty xmm register
	{
		if (pref >= 0) for (u32 i = 0; i < 16; i++)
		{
			if ((xmm_var[i].reg == pref) && !xmm_var[i].taken)
			{
				xmm_var[i].taken = true;
				xmm_var[i].got = false;
				xmm_var[i].access = 0;
				LOG4_OPCODE("pref(%d) reg taken (i=%d)", pref, i);
				return xmm_var[i];
			}
		}
		for (u32 i = 0; i < 16; i++)
		{
			if ((xmm_var[i].reg == -1) && !xmm_var[i].taken)
			{
				xmm_var[i].taken = true;
				xmm_var[i].got = false;
				xmm_var[i].access = 0;
				LOG4_OPCODE("free reg taken (i=%d)", i);
				return xmm_var[i];
			}
		}
		int last = -1, max = -1;
		for (u32 i = 0; i < 16; i++)
		{
			if (!xmm_var[i].taken)
			{
				if ((int)xmm_var[i].access > max)
				{
					last = i;
					max = xmm_var[i].access;
				}
			}
		}
		if (last >= 0)
		{
			// (saving cached data?)
			//c.movdqa(cpu_xmm(GPR[xmm_var[last].reg]), *xmm_var[last].data);
			xmm_var[last].taken = true;
			xmm_var[last].got = false;
			LOG4_OPCODE("cached reg taken (i=%d): GPR[%d] lost", last, xmm_var[last].reg);
			xmm_var[last].reg = -1; // ???
			xmm_var[last].access = 0;
			return xmm_var[last];
		}
		throw "XmmAlloc() failed";
	}

	const XmmLink* XmmRead(const s8 reg) const // get xmm register with specific SPU reg or nullptr
	{
		assert(reg >= 0);
		for (u32 i = 0; i < 16; i++)
		{
			if (xmm_var[i].reg == reg)
			{
				//assert(!xmm_var[i].got);
				//if (xmm_var[i].got) throw "XmmRead(): wrong reuse";
				LOG4_OPCODE("GPR[%d] has been read (i=%d)", reg, i);
				xmm_var[i].access++;
				return &xmm_var[i];
			}
		}
		LOG4_OPCODE("GPR[%d] not found", reg);
		return nullptr;
	}

	const XmmLink& XmmGet(s8 reg, s8 target = -1) // get xmm register with specific SPU reg
	{
		assert(reg >= 0);
		const XmmLink* res = nullptr;
		if (reg == target)
		{
			for (u32 i = 0; i < 16; i++)
			{
				if (xmm_var[i].reg == reg)
				{
					res = &xmm_var[i];
					if (xmm_var[i].taken) throw "XmmGet(): xmm_var is taken";
					xmm_var[i].taken = true;
					xmm_var[i].got = false;
					//xmm_var[i].reg = -1;
					for (u32 j = i + 1; j < 16; j++)
					{
						if (xmm_var[j].reg == reg) throw "XmmGet(): xmm_var duplicate";
					}
					LOG4_OPCODE("cached GPR[%d] used (i=%d)", reg, i);
					break;
				}
			}
		}
		if (!res)
		{
			res = &XmmAlloc(target);
			/*if (target != res->reg)
			{
				c.movdqa(*res->data, cpu_xmm(GPR[reg]));
			}
			else*/
			{
				if (const XmmLink* source = XmmRead(reg))
				{
					c.movdqa(*res->data, source->read());
				}
				else
				{
					c.movdqa(*res->data, cpu_xmm(GPR[reg]));
				}
			}
			const_cast<XmmLink*>(res)->reg = -1; // ???
			LOG4_OPCODE("* cached GPR[%d] not found", reg);
		}
		return *res;
	}

	const XmmLink& XmmCopy(const XmmLink& from, s8 pref = -1) // XmmAlloc + mov
	{
		const XmmLink* res = &XmmAlloc(pref);
		c.movdqa(*res->data, *from.data);
		const_cast<XmmLink*>(res)->reg = -1; // ???
		LOG4_OPCODE("*");
		return *res;
	}

	void XmmInvalidate(const s8 reg) // invalidate cached register
	{
		assert(reg >= 0);
		for (u32 i = 0; i < 16; i++)
		{
			if (xmm_var[i].reg == reg)
			{
				if (xmm_var[i].taken) throw "XmmInvalidate(): xmm_var is taken";
				LOG4_OPCODE("GPR[%d] invalidated (i=%d)", reg, i);
				xmm_var[i].reg = -1;
				xmm_var[i].access = 0;
			}
		}
	}

	void XmmFinalize(const XmmLink& var, s8 reg = -1)
	{
		// invalidation
		if (reg >= 0) for (u32 i = 0; i < 16; i++)
		{
			if (xmm_var[i].reg == reg)
			{
				LOG4_OPCODE("GPR[%d] invalidated (i=%d)", reg, i);
				xmm_var[i].reg = -1;
				xmm_var[i].access = 0;
			}
		}
		for (u32 i = 0; i < 16; i++)
		{
			if (xmm_var[i].data == var.data)
			{
				assert(xmm_var[i].taken);
				// save immediately:
				if (reg >= 0)
				{
					c.movdqa(cpu_xmm(GPR[reg]), *xmm_var[i].data);
				}
				else
				{
				}
				LOG4_OPCODE("GPR[%d] finalized (i=%d), GPR[%d] replaced", reg, i, xmm_var[i].reg);
				// (to disable caching:)
				//reg = -1;
				xmm_var[i].reg = reg;
				xmm_var[i].taken = false;
				xmm_var[i].got = false;
				xmm_var[i].access = 0;
				return;
			}
		}
		assert(false);
	}

	void XmmRelease()
	{
		for (u32 i = 0; i < 16; i++)
		{
			if (xmm_var[i].reg >= 0)
			{
				//c.movdqa(cpu_xmm(GPR[xmm_var[i].reg]), *xmm_var[i].data);
				LOG4_OPCODE("GPR[%d] released (i=%d)", xmm_var[i].reg, i);
				xmm_var[i].reg = -1;
				xmm_var[i].access = 0;
			}
		}
	}

	Mem XmmConst(u128 data)
	{
		s32 shift = 0;

		for (; shift < rec.imm_table.size(); shift++)
		{
			if (rec.imm_table[shift] == data)
			{
				return oword_ptr(*imm_var, shift * sizeof(u128));
			}
		}

		rec.imm_table.push_back(data);
		return oword_ptr(*imm_var, shift * sizeof(u128));
	}

private:
	//0 - 10
	void STOP(u32 code)
	{
		struct STOP_wrapper
		{
			static void STOP(u32 code)
			{
				SPUThread& CPU = *(SPUThread*)GetCurrentNamedThread();
				CPU.stop_and_signal(code);
				LOG2_OPCODE();
			}
		};
		c.mov(cpu_dword(PC), CPU.PC);
		X86CallNode* call = c.call(imm_ptr(reinterpret_cast<void*>(&STOP_wrapper::STOP)), kFuncConvHost, FuncBuilder1<void, u32>());
		call->setArg(0, imm_u(code));
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		do_finalize = true;
		LOG_OPCODE();
	}
	void LNOP()
	{
		LOG_OPCODE();
	}
	void SYNC(u32 Cbit)
	{
		c.mov(cpu_dword(PC), CPU.PC);
		// This instruction must be used following a store instruction that modifies the instruction stream.
		c.mfence();
		c.mov(*pos_var, (CPU.PC >> 2) + 1 + 0x2000000);
		do_finalize = true;
		LOG_OPCODE();
	}
	void DSYNC()
	{
		// This instruction forces all earlier load, store, and channel instructions to complete before proceeding.
		c.mfence();
		LOG_OPCODE();
	}
	void MFSPR(u32 rt, u32 sa)
	{
		UNIMPLEMENTED();
	}
	void RDCH(u32 rt, u32 ra)
	{
		c.mov(cpu_dword(PC), CPU.PC);
		WRAPPER_BEGIN(rt, ra, yy, zz);
		CPU.GPR[rt] = u128::from32r(CPU.get_ch_value(ra));
		WRAPPER_END(rt, ra, 0, 0);
		// TODO
	}
	void RCHCNT(u32 rt, u32 ra)
	{
		c.mov(cpu_dword(PC), CPU.PC);
		WRAPPER_BEGIN(rt, ra, yy, zz);
		CPU.GPR[rt] = u128::from32r(CPU.get_ch_count(ra));
		WRAPPER_END(rt, ra, 0, 0);
		// TODO
	}
	void SF(u32 rt, u32 ra, u32 rb)
	{
		// sub from
		const XmmLink& vb = XmmGet(rb, rt);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.psubd(vb.get(), va->read());
		}
		else
		{
			c.psubd(vb.get(), cpu_xmm(GPR[ra]));
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void OR(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vb = XmmGet(rb, rt);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.por(vb.get(), va->read());
		}
		else
		{
			c.por(vb.get(), cpu_xmm(GPR[ra]));
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void BG(u32 rt, u32 ra, u32 rb)
	{
		// compare if-greater-than
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vi = XmmAlloc();
		c.movdqa(vi.get(), XmmConst(u128::from32p(0x80000000)));
		c.pxor(va.get(), vi.get());
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pxor(vi.get(), vb->read());
		}
		else
		{
			c.pxor(vi.get(), cpu_xmm(GPR[rb]));
		}
		c.pcmpgtd(va.get(), vi.get());
		c.paddd(va.get(), XmmConst(u128::from32p(1)));
		XmmFinalize(va, rt);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	void SFH(u32 rt, u32 ra, u32 rb)
	{
		// sub from (halfword)
		const XmmLink& vb = XmmGet(rb, rt);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.psubw(vb.get(), va->read());
		}
		else
		{
			c.psubw(vb.get(), cpu_xmm(GPR[ra]));
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void NOR(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (ra != rb)
		{
			if (const XmmLink* vb = XmmRead(rb))
			{
				c.por(va.get(), vb->read());
			}
			else
			{
				c.por(va.get(), cpu_xmm(GPR[rb]));
			}
		}
		c.pxor(va.get(), XmmConst(u128::from32p(0xffffffff)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ABSDB(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = XmmGet(rb);
		const XmmLink& vm = XmmCopy(va);
		c.pmaxub(va.get(), vb.get());
		c.pminub(vb.get(), vm.get());
		c.psubb(va.get(), vb.get());
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		XmmFinalize(vm);
		LOG_OPCODE();
	}
	void ROT(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);
		for (u32 i = 0; i < 4; i++)
		{
			c.mov(qw0->r32(), cpu_dword(GPR[ra]._u32[i]));
			c.mov(*addr, cpu_dword(GPR[rb]._u32[i]));
			c.rol(qw0->r32(), *addr);
			c.mov(cpu_dword(GPR[rt]._u32[i]), qw0->r32());
		}
		LOG_OPCODE();
	}
	void ROTM(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);
		for (u32 i = 0; i < 4; i++)
		{
			c.mov(qw0->r32(), cpu_dword(GPR[ra]._u32[i]));
			c.mov(*addr, cpu_dword(GPR[rb]._u32[i]));
			c.neg(*addr);
			c.shr(*qw0, *addr);
			c.mov(cpu_dword(GPR[rt]._u32[i]), qw0->r32());
		}
		LOG_OPCODE();
	}
	void ROTMA(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);
		for (u32 i = 0; i < 4; i++)
		{
			c.movsxd(*qw0, cpu_dword(GPR[ra]._u32[i]));
			c.mov(*addr, cpu_dword(GPR[rb]._u32[i]));
			c.neg(*addr);
			c.sar(*qw0, *addr);
			c.mov(cpu_dword(GPR[rt]._u32[i]), qw0->r32());
		}
		LOG_OPCODE();
	}
	void SHL(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);
		for (u32 i = 0; i < 4; i++)
		{
			c.mov(qw0->r32(), cpu_dword(GPR[ra]._u32[i]));
			c.mov(*addr, cpu_dword(GPR[rb]._u32[i]));
			c.shl(*qw0, *addr);
			c.mov(cpu_dword(GPR[rt]._u32[i]), qw0->r32());
		}
		LOG_OPCODE();
	}
	void ROTH(u32 rt, u32 ra, u32 rb) //nf
	{
		XmmInvalidate(rt);
		for (u32 i = 0; i < 8; i++)
		{
			c.movzx(qw0->r32(), cpu_word(GPR[ra]._u16[i]));
			c.movzx(*addr, cpu_word(GPR[rb]._u16[i]));
			c.rol(qw0->r16(), *addr);
			c.mov(cpu_word(GPR[rt]._u16[i]), qw0->r16());
		}
		LOG_OPCODE();
	}
	void ROTHM(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);
		for (u32 i = 0; i < 8; i++)
		{
			c.movzx(qw0->r32(), cpu_word(GPR[ra]._u16[i]));
			c.movzx(*addr, cpu_word(GPR[rb]._u16[i]));
			c.neg(*addr);
			c.shr(qw0->r32(), *addr);
			c.mov(cpu_word(GPR[rt]._u16[i]), qw0->r16());
		}
		LOG_OPCODE();
	}
	void ROTMAH(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);
		for (u32 i = 0; i < 8; i++)
		{
			c.movsx(qw0->r32(), cpu_word(GPR[ra]._u16[i]));
			c.movzx(*addr, cpu_word(GPR[rb]._u16[i]));
			c.neg(*addr);
			c.sar(qw0->r32(), *addr);
			c.mov(cpu_word(GPR[rt]._u16[i]), qw0->r16());
		}
		LOG_OPCODE();
	}
	void SHLH(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);
		for (u32 i = 0; i < 8; i++)
		{
			c.movzx(qw0->r32(), cpu_word(GPR[ra]._u16[i]));
			c.movzx(*addr, cpu_word(GPR[rb]._u16[i]));
			c.shl(qw0->r32(), *addr);
			c.mov(cpu_word(GPR[rt]._u16[i]), qw0->r16());
		}
		LOG_OPCODE();
	}
	void ROTI(u32 rt, u32 ra, s32 i7)
	{
		// rotate left
		const int s = i7 & 0x1f;
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& v1 = XmmCopy(va);
		c.pslld(va.get(), s);
		c.psrld(v1.get(), 32 - s);
		c.por(va.get(), v1.get());
		XmmFinalize(va, rt);
		XmmFinalize(v1);
		LOG_OPCODE();
	}
	void ROTMI(u32 rt, u32 ra, s32 i7)
	{
		// shift right logical
		const int s = (0 - i7) & 0x3f;
		const XmmLink& va = XmmGet(ra, rt);
		c.psrld(va.get(), s);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ROTMAI(u32 rt, u32 ra, s32 i7)
	{
		// shift right arithmetical
		const int s = (0 - i7) & 0x3f;
		const XmmLink& va = XmmGet(ra, rt);
		c.psrad(va.get(), s);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void SHLI(u32 rt, u32 ra, s32 i7)
	{
		// shift left
		const int s = i7 & 0x3f;
		const XmmLink& va = XmmGet(ra, rt);
		c.pslld(va.get(), s);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ROTHI(u32 rt, u32 ra, s32 i7)
	{
		// rotate left (halfword)
		const int s = i7 & 0xf;
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& v1 = XmmCopy(va);
		c.psllw(va.get(), s);
		c.psrlw(v1.get(), 16 - s);
		c.por(va.get(), v1.get());
		XmmFinalize(va, rt);
		XmmFinalize(v1);
		LOG_OPCODE();
	}
	void ROTHMI(u32 rt, u32 ra, s32 i7)
	{
		// shift right logical
		const int s = (0 - i7) & 0x1f;
		const XmmLink& va = XmmGet(ra, rt);
		c.psrlw(va.get(), s);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ROTMAHI(u32 rt, u32 ra, s32 i7)
	{
		// shift right arithmetical (halfword)
		const int s = (0 - i7) & 0x1f;
		const XmmLink& va = XmmGet(ra, rt);
		c.psraw(va.get(), s);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void SHLHI(u32 rt, u32 ra, s32 i7)
	{
		// shift left (halfword)
		const int s = i7 & 0x1f;
		const XmmLink& va = XmmGet(ra, rt);
		c.psllw(va.get(), s);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void A(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vb = XmmGet(rb, rt);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.paddd(vb.get(), va->read());
		}
		else
		{
			c.paddd(vb.get(), cpu_xmm(GPR[ra]));
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void AND(u32 rt, u32 ra, u32 rb)
	{
		// and
		const XmmLink& vb = XmmGet(rb, rt);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.pand(vb.get(), va->read());
		}
		else
		{
			c.pand(vb.get(), cpu_xmm(GPR[ra]));
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void CG(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = XmmGet(rb);
		const XmmLink& vi = XmmAlloc();
		c.movdqa(vi.get(), XmmConst(u128::from32p(0x80000000)));
		c.paddd(vb.get(), va.get());
		c.pxor(va.get(), vi.get());
		c.pxor(vb.get(), vi.get());
		c.pcmpgtd(va.get(), vb.get());
		c.psrld(va.get(), 31);
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	void AH(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.paddw(va.get(), vb->read());
		}
		else
		{
			c.paddw(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void NAND(u32 rt, u32 ra, u32 rb)
	{
		// nand
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pand(va.get(), vb->read());
		}
		else
		{
			c.pand(va.get(), cpu_xmm(GPR[rb]));
		}
		c.pxor(va.get(), XmmConst(u128::from32p(0xffffffff)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void AVGB(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vb = XmmGet(rb);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.pavgb(vb.get(), va->read());
		}
		else
		{
			c.pavgb(vb.get(), cpu_xmm(GPR[ra]));
		}
		XmmFinalize(vb, rt);
	}
	void MTSPR(u32 rt, u32 sa)
	{
	}
	void WRCH(u32 ra, u32 rt)
	{
		c.mov(cpu_dword(PC), CPU.PC);
		WRAPPER_BEGIN(ra, rt, yy, zz);
		CPU.set_ch_value(ra, CPU.GPR[rt]._u32[3]);
		WRAPPER_END(ra, rt, 0, 0);
		// TODO

		/*XmmInvalidate(rt);
		
		X86GpVar v(c, kVarTypeUInt32);
		c.mov(v, cpu_dword(GPR[rt]._u32[3]));
		switch (ra)
		{
		case MFC_LSA:
			c.mov(cpu_dword(MFC1.LSA.m_value[0]), v);
			break;
		
		case MFC_EAH:
			c.mov(cpu_dword(MFC1.EAH.m_value[0]), v);
			break;

		case MFC_EAL:
			c.mov(cpu_dword(MFC1.EAL.m_value[0]), v);
			break;

		case MFC_Size:
			c.mov(cpu_word(MFC1.Size_Tag.m_val16[1]), v);
			break;

		case MFC_TagID:
			c.mov(cpu_word(MFC1.Size_Tag.m_val16[0]), v);
			break;

		default:
		{
			X86X64CallNode* call = c.call(imm_ptr(reinterpret_cast<void*>(&WRCH_wrapper::WRCH)), kFuncConvHost, FuncBuilder2<FnVoid, u32, u32>());
			call->setArg(0, imm_u(ra));
			call->setArg(1, v);
		}
		}*/
	}
	void BIZ(u32 intr, u32 rt, u32 ra)
	{
		switch (intr & 0x30)
		{
		case 0: break;
		default: UNIMPLEMENTED(); return;
		}

		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*addr, CPU.PC + 4);
		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		if (ra) c.or_(*pos_var, 0x2000000 << 2); // rude (check if not LR)
		c.cmp(cpu_dword(GPR[rt]._u32[3]), 0);
		c.cmovne(*pos_var, *addr);
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void BINZ(u32 intr, u32 rt, u32 ra)
	{
		switch (intr & 0x30)
		{
		case 0: break;
		default: UNIMPLEMENTED(); return;
		}

		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*addr, CPU.PC + 4);
		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		if (ra) c.or_(*pos_var, 0x2000000 << 2); // rude (check if not LR)
		c.cmp(cpu_dword(GPR[rt]._u32[3]), 0);
		c.cmove(*pos_var, *addr);
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void BIHZ(u32 intr, u32 rt, u32 ra)
	{
		switch (intr & 0x30)
		{
		case 0: break;
		default: UNIMPLEMENTED(); return;
		}

		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*addr, CPU.PC + 4);
		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		if (ra) c.or_(*pos_var, 0x2000000 << 2); // rude (check if not LR)
		c.cmp(cpu_word(GPR[rt]._u16[6]), 0);
		c.cmovne(*pos_var, *addr);
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void BIHNZ(u32 intr, u32 rt, u32 ra)
	{
		switch (intr & 0x30)
		{
		case 0: break;
		default: UNIMPLEMENTED(); return;
		}

		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*addr, CPU.PC + 4);
		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		if (ra) c.or_(*pos_var, 0x2000000 << 2); // rude (check if not LR)
		c.cmp(cpu_word(GPR[rt]._u16[6]), 0);
		c.cmove(*pos_var, *addr);
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void STOPD(u32 rc, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void STQX(u32 rt, u32 ra, u32 rb)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		if (ra == rb)
		{
			c.add(*addr, *addr);
		}
		else
		{
			c.add(*addr, cpu_dword(GPR[rb]._u32[3]));
		}
		c.and_(*addr, 0x3fff0);

		/*const XmmLink& vt = XmmGet(rt);
		c.pshufb(vt.get(), XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c.movdqa(oword_ptr(*ls_var, *addr), vt.get());
		XmmFinalize(vt);*/

		c.mov(*qw0, cpu_qword(GPR[rt]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[rt]._u64[1]));
		c.bswap(*qw0);
		c.bswap(*qw1);
		c.mov(qword_ptr(*ls_var, *addr, 0, 0), *qw1);
		c.mov(qword_ptr(*ls_var, *addr, 0, 8), *qw0);

		LOG_OPCODE();
	}
	void BI(u32 intr, u32 ra)
	{
		switch (intr & 0x30)
		{
		case 0: break;
		default: UNIMPLEMENTED(); return;
		}

		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		if (ra) c.or_(*pos_var, 0x2000000 << 2); // rude (check if not LR)
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void BISL(u32 intr, u32 rt, u32 ra)
	{
		switch (intr & 0x30)
		{
		case 0: break;
		default: UNIMPLEMENTED(); return;
		}

		XmmInvalidate(rt);

		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.xor_(*pos_var, *pos_var);
		c.mov(cpu_dword(GPR[rt]._u32[0]), *pos_var);
		c.mov(cpu_dword(GPR[rt]._u32[1]), *pos_var);
		c.mov(cpu_dword(GPR[rt]._u32[2]), *pos_var);
		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		c.mov(cpu_dword(GPR[rt]._u32[3]), CPU.PC + 4);
		c.shr(*pos_var, 2);
		c.or_(*pos_var, 0x2000000);
		LOG_OPCODE();
	}
	void IRET(u32 ra)
	{
		UNIMPLEMENTED();
	}
	void BISLED(u32 intr, u32 rt, u32 ra)
	{
		UNIMPLEMENTED();
	}
	void HBR(u32 p, u32 ro, u32 ra)
	{
		LOG_OPCODE();
	}
	void GB(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.pshufb(va.get(), XmmConst(u128::fromV(_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0))));
		c.psllq(va.get(), 7);
		c.pmovmskb(*addr, va.get());
		c.pxor(va.get(), va.get());
		c.pinsrw(va.get(), *addr, 6);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void GBH(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.pshufb(va.get(), XmmConst(u128::fromV(_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 14, 12, 10, 8, 6, 4, 2, 0))));
		c.psllq(va.get(), 7);
		c.pmovmskb(*addr, va.get());
		c.pxor(va.get(), va.get());
		c.pinsrw(va.get(), *addr, 6);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void GBB(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.psllq(va.get(), 7);
		c.pmovmskb(*addr, va.get());
		c.pxor(va.get(), va.get());
		c.pinsrw(va.get(), *addr, 6);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void FSM(u32 rt, u32 ra)
	{
		const XmmLink& vr = XmmAlloc(rt);
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.and_(*addr, 0xf);
		c.shl(*addr, 4);
		c.movdqa(vr.get(), g_imm2_xmm(fsm[0], *addr));
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void FSMH(u32 rt, u32 ra)
	{
		const XmmLink& vr = XmmAlloc(rt);
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.and_(*addr, 0xff);
		c.shl(*addr, 4);
		c.movdqa(vr.get(), g_imm2_xmm(fsmh[0], *addr));
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void FSMB(u32 rt, u32 ra)
	{
		const XmmLink& vr = XmmAlloc(rt);
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.and_(*addr, 0xffff);
		c.shl(*addr, 4);
		c.movdqa(vr.get(), g_imm2_xmm(fsmb[0], *addr));
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void FREST(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.rcpps(va.get(), va.get());
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void FRSQEST(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.andps(va.get(), XmmConst(u128::from32p(0x7fffffff))); // abs
		c.rsqrtps(va.get(), va.get());
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void LQX(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);

		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		if (ra == rb)
		{
			c.add(*addr, *addr);
		}
		else
		{
			c.add(*addr, cpu_dword(GPR[rb]._u32[3]));
		}
		c.and_(*addr, 0x3fff0);

		/*const XmmLink& vt = XmmAlloc(rt);
		c.movdqa(vt.get(), oword_ptr(*ls_var, *addr));
		c.pshufb(vt.get(), XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		XmmFinalize(vt, rt);*/

		c.mov(*qw0, qword_ptr(*ls_var, *addr, 0, 0));
		c.mov(*qw1, qword_ptr(*ls_var, *addr, 0, 8));
		c.bswap(*qw0);
		c.bswap(*qw1);
		c.mov(cpu_qword(GPR[rt]._u64[0]), *qw1);
		c.mov(cpu_qword(GPR[rt]._u64[1]), *qw0);

		LOG_OPCODE();
	}
	void ROTQBYBI(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.and_(*addr, 0xf << 3);
		c.shl(*addr, 1);
		c.pshufb(va.get(), g_imm2_xmm(rldq_pshufb[0], *addr));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ROTQMBYBI(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.shr(*addr, 3);
		c.neg(*addr);
		c.and_(*addr, 0x1f);
		c.shl(*addr, 4);
		c.pshufb(va.get(), g_imm2_xmm(srdq_pshufb[0], *addr));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void SHLQBYBI(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.and_(*addr, 0x1f << 3);
		c.shl(*addr, 1);
		c.pshufb(va.get(), g_imm2_xmm(sldq_pshufb[0], *addr));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CBX(u32 rt, u32 ra, u32 rb)
	{
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		if (ra == 1)
		{
			// assuming that SP % 16 is always zero
		}
		else if (ra == rb)
		{
			c.add(*addr, *addr);
		}
		else
		{
			c.add(*addr, cpu_dword(GPR[ra]._u32[3]));
		}
		c.not_(*addr);
		c.and_(*addr, 0xf);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f))));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(byte_ptr(*cpu_var, *addr, 0, (s32)offsetof(SPUThread, GPR[rt]._u8[0])), 0x03);
		LOG_OPCODE();
	}
	void CHX(u32 rt, u32 ra, u32 rb)
	{
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		if (ra == 1)
		{
			// assuming that SP % 16 is always zero
		}
		else if (ra == rb)
		{
			c.add(*addr, *addr);
		}
		else
		{
			c.add(*addr, cpu_dword(GPR[ra]._u32[3]));
		}
		c.not_(*addr);
		c.and_(*addr, 0xe);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f))));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(word_ptr(*cpu_var, *addr, 0, (s32)offsetof(SPUThread, GPR[rt]._u16[0])), 0x0203);
		LOG_OPCODE();
	}
	void CWX(u32 rt, u32 ra, u32 rb)
	{
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		if (ra == 1)
		{
			// assuming that SP % 16 is always zero
		}
		else if (ra == rb)
		{
			c.add(*addr, *addr);
		}
		else
		{
			c.add(*addr, cpu_dword(GPR[ra]._u32[3]));
		}
		c.not_(*addr);
		c.and_(*addr, 0xc);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f))));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(dword_ptr(*cpu_var, *addr, 0, (s32)offsetof(SPUThread, GPR[rt]._u32[0])), 0x00010203);
		LOG_OPCODE();
	}
	void CDX(u32 rt, u32 ra, u32 rb)
	{
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		if (ra == 1)
		{
			// assuming that SP % 16 is always zero
		}
		else if (ra == rb)
		{
			c.add(*addr, *addr);
		}
		else
		{
			c.add(*addr, cpu_dword(GPR[ra]._u32[3]));
		}
		c.not_(*addr);
		c.and_(*addr, 0x8);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f))));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(dword_ptr(*cpu_var, *addr, 0, (s32)offsetof(SPUThread, GPR[rt]._u32[0])), 0x04050607);
		c.mov(dword_ptr(*cpu_var, *addr, 0, (s32)offsetof(SPUThread, GPR[rt]._u32[1])), 0x00010203);
		LOG_OPCODE();
	}
	void ROTQBI(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);
		c.mov(*qw0, cpu_qword(GPR[ra]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[ra]._u64[1]));
		c.mov(*qw2, *qw0);
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.and_(*addr, 7);
		c.shld(*qw0, *qw1, *addr);
		c.shld(*qw1, *qw2, *addr);
		c.mov(cpu_qword(GPR[rt]._u64[0]), *qw0);
		c.mov(cpu_qword(GPR[rt]._u64[1]), *qw1);
		LOG_OPCODE();
	}
	void ROTQMBI(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);
		c.mov(*qw0, cpu_qword(GPR[ra]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[ra]._u64[1]));
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.neg(*addr);
		c.and_(*addr, 7);
		c.shrd(*qw0, *qw1, *addr);
		c.shr(*qw1, *addr);
		c.mov(cpu_qword(GPR[rt]._u64[0]), *qw0);
		c.mov(cpu_qword(GPR[rt]._u64[1]), *qw1);
		LOG_OPCODE();
	}
	void SHLQBI(u32 rt, u32 ra, u32 rb)
	{
		XmmInvalidate(rt);
		c.mov(*qw0, cpu_qword(GPR[ra]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[ra]._u64[1]));
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.and_(*addr, 7);
		c.shld(*qw1, *qw0, *addr);
		c.shl(*qw0, *addr);
		c.mov(cpu_qword(GPR[rt]._u64[0]), *qw0);
		c.mov(cpu_qword(GPR[rt]._u64[1]), *qw1);
		LOG_OPCODE();
	}
	void ROTQBY(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.and_(*addr, 0xf);
		c.shl(*addr, 4);
		c.pshufb(va.get(), g_imm2_xmm(rldq_pshufb[0], *addr));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ROTQMBY(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.neg(*addr);
		c.and_(*addr, 0x1f);
		c.shl(*addr, 4);
		c.pshufb(va.get(), g_imm2_xmm(srdq_pshufb[0], *addr));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void SHLQBY(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.mov(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.and_(*addr, 0x1f);
		c.shl(*addr, 4);
		c.pshufb(va.get(), g_imm2_xmm(sldq_pshufb[0], *addr));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ORX(u32 rt, u32 ra)
	{
		XmmInvalidate(rt);
		c.mov(*addr, cpu_dword(GPR[ra]._u32[0]));
		c.or_(*addr, cpu_dword(GPR[ra]._u32[1]));
		c.or_(*addr, cpu_dword(GPR[ra]._u32[2]));
		c.or_(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.mov(cpu_dword(GPR[rt]._u32[3]), *addr);
		c.xor_(*addr, *addr);
		c.mov(cpu_dword(GPR[rt]._u32[0]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[1]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[2]), *addr);
		LOG_OPCODE();
	}
	void CBD(u32 rt, u32 ra, s32 i7)
	{
		if (ra == 1)
		{
			// assuming that SP % 16 is always zero
			const XmmLink& vr = XmmAlloc(rt);
			u128 value = u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
			value.u8r[i7 & 0xf] = 0x03;
			c.movdqa(vr.get(), XmmConst(value));
			XmmFinalize(vr, rt);
		}
		else
		{
			c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
			c.add(*addr, i7);
			c.not_(*addr);
			c.and_(*addr, 0xf);
			const XmmLink& vr = XmmAlloc(rt);
			c.movdqa(vr.get(), XmmConst(u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f))));
			XmmFinalize(vr, rt);
			XmmInvalidate(rt);
			c.mov(byte_ptr(*cpu_var, *addr, 0, (s32)offsetof(SPUThread, GPR[rt]._u8[0])), 0x03);
		}
		LOG_OPCODE();
	}
	void CHD(u32 rt, u32 ra, s32 i7)
	{
		if (ra == 1)
		{
			// assuming that SP % 16 is always zero
			const XmmLink& vr = XmmAlloc(rt);
			u128 value = u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
			value.u16r[(i7 >> 1) & 0x7] = 0x0203;
			c.movdqa(vr.get(), XmmConst(value));
			XmmFinalize(vr, rt);
		}
		else
		{
			c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
			c.add(*addr, i7);
			c.not_(*addr);
			c.and_(*addr, 0xe);
			const XmmLink& vr = XmmAlloc(rt);
			c.movdqa(vr.get(), XmmConst(u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f))));
			XmmFinalize(vr, rt);
			XmmInvalidate(rt);
			c.mov(word_ptr(*cpu_var, *addr, 0, (s32)offsetof(SPUThread, GPR[rt]._u16[0])), 0x0203);
		}
		LOG_OPCODE();
	}
	void CWD(u32 rt, u32 ra, s32 i7)
	{
		if (ra == 1)
		{
			// assuming that SP % 16 is always zero
			const XmmLink& vr = XmmAlloc(rt);
			u128 value = u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
			value.u32r[(i7 >> 2) & 0x3] = 0x00010203;
			c.movdqa(vr.get(), XmmConst(value));
			XmmFinalize(vr, rt);
		}
		else
		{
			c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
			c.add(*addr, i7);
			c.not_(*addr);
			c.and_(*addr, 0xc);
			const XmmLink& vr = XmmAlloc(rt);
			c.movdqa(vr.get(), XmmConst(u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f))));
			XmmFinalize(vr, rt);
			XmmInvalidate(rt);
			c.mov(dword_ptr(*cpu_var, *addr, 0, (s32)offsetof(SPUThread, GPR[rt]._u32[0])), 0x00010203);
		}
		LOG_OPCODE();
	}
	void CDD(u32 rt, u32 ra, s32 i7)
	{
		if (ra == 1)
		{
			// assuming that SP % 16 is always zero
			const XmmLink& vr = XmmAlloc(rt);
			u128 value = u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
			value.u64r[(i7 >> 3) & 0x1] = 0x0001020304050607ull;
			c.movdqa(vr.get(), XmmConst(value));
			XmmFinalize(vr, rt);
		}
		else
		{
			c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
			c.add(*addr, i7);
			c.not_(*addr);
			c.and_(*addr, 0x8);
			const XmmLink& vr = XmmAlloc(rt);
			c.movdqa(vr.get(), XmmConst(u128::fromV(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f))));
			XmmFinalize(vr, rt);
			XmmInvalidate(rt);
			c.mov(dword_ptr(*cpu_var, *addr, 0, (s32)offsetof(SPUThread, GPR[rt]._u32[0])), 0x04050607);
			c.mov(dword_ptr(*cpu_var, *addr, 0, (s32)offsetof(SPUThread, GPR[rt]._u32[1])), 0x00010203);
		}
		LOG_OPCODE();
	}
	void ROTQBII(u32 rt, u32 ra, s32 i7)
	{
		XmmInvalidate(rt);
		c.mov(*qw0, cpu_qword(GPR[ra]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[ra]._u64[1]));
		c.mov(*qw2, *qw0);
		c.shld(*qw0, *qw1, i7 & 0x7);
		c.shld(*qw1, *qw2, i7 & 0x7);
		c.mov(cpu_qword(GPR[rt]._u64[0]), *qw0);
		c.mov(cpu_qword(GPR[rt]._u64[1]), *qw1);
		LOG_OPCODE();
	}
	void ROTQMBII(u32 rt, u32 ra, s32 i7)
	{
		XmmInvalidate(rt);
		c.mov(*qw0, cpu_qword(GPR[ra]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[ra]._u64[1]));
		c.shrd(*qw0, *qw1, (0 - i7) & 0x7);
		c.shr(*qw1, (0 - i7) & 0x7);
		c.mov(cpu_qword(GPR[rt]._u64[0]), *qw0);
		c.mov(cpu_qword(GPR[rt]._u64[1]), *qw1);
		LOG_OPCODE();
	}
	void SHLQBII(u32 rt, u32 ra, s32 i7)
	{
		XmmInvalidate(rt);
		c.mov(*qw0, cpu_qword(GPR[ra]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[ra]._u64[1]));
		c.shld(*qw1, *qw0, i7 & 0x7);
		c.shl(*qw0, i7 & 0x7);
		c.mov(cpu_qword(GPR[rt]._u64[0]), *qw0);
		c.mov(cpu_qword(GPR[rt]._u64[1]), *qw1);
		LOG_OPCODE();
	}
	void ROTQBYI(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0xf;
		const XmmLink& va = XmmGet(ra, rt);
		c.palignr(va.get(), va.get(), 16 - s);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ROTQMBYI(u32 rt, u32 ra, s32 i7)
	{
		const int s = (0 - i7) & 0x1f;
		const XmmLink& va = XmmGet(ra, rt);
		c.psrldq(va.get(), s);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void SHLQBYI(u32 rt, u32 ra, s32 i7)
	{		
		const int s = i7 & 0x1f;
		const XmmLink& va = XmmGet(ra, rt);
		c.pslldq(va.get(), s);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void NOP(u32 rt)
	{
		LOG_OPCODE();
	}
	void CGT(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pcmpgtd(va.get(), vb->read());
		}
		else
		{
			c.pcmpgtd(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void XOR(u32 rt, u32 ra, u32 rb)
	{
		// xor
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pxor(va.get(), vb->read());
		}
		else
		{
			c.pxor(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CGTH(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pcmpgtw(va.get(), vb->read());
		}
		else
		{
			c.pcmpgtw(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void EQV(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vb = XmmGet(rb, rt);
		c.pxor(vb.get(), XmmConst(u128::from32p(0xffffffff)));
		if (const XmmLink* va = XmmRead(ra))
		{
			c.pxor(vb.get(), va->read());
		}
		else
		{
			c.pxor(vb.get(), cpu_xmm(GPR[ra]));
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void CGTB(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pcmpgtb(va.get(), vb->read());
		}
		else
		{
			c.pcmpgtb(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void SUMB(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		const XmmLink& vi = XmmAlloc();
		c.movdqa(vi.get(), XmmConst(u128::from8p(1)));
		c.pmaddubsw(va.get(), vi.get());
		c.pmaddubsw(vb.get(), vi.get());
		c.phaddw(va.get(), vb.get());
		c.pshufb(va.get(), XmmConst(u128::fromV(_mm_set_epi8(15, 14, 7, 6, 13, 12, 5, 4, 11, 10, 3, 2, 9, 8, 1, 0))));
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	//HGT uses signed values.  HLGT uses unsigned values
	void HGT(u32 rt, s32 ra, s32 rb)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._s32[3]));
		c.cmp(*addr, cpu_dword(GPR[rb]._s32[3]));
		c.setg(addr->r8());
		c.shl(*addr, 24);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.or_(*pos_var, *addr);
		do_finalize = true;
		LOG_OPCODE();
	}
	void CLZ(u32 rt, u32 ra)
	{
		XmmInvalidate(rt);
		c.mov(*qw0, 32 + 31);
		for (u32 i = 0; i < 4; i++)
		{
			c.bsr(*addr, cpu_dword(GPR[ra]._u32[i]));
			c.cmovz(*addr, qw0->r32());
			c.xor_(*addr, 31);
			c.mov(cpu_dword(GPR[rt]._u32[i]), *addr);
		}
		LOG_OPCODE();
	}
	void XSWD(u32 rt, u32 ra)
	{
		c.movsxd(*qw0, cpu_dword(GPR[ra]._s32[0]));
		c.movsxd(*qw1, cpu_dword(GPR[ra]._s32[2]));
		c.mov(cpu_qword(GPR[rt]._s64[0]), *qw0);
		c.mov(cpu_qword(GPR[rt]._s64[1]), *qw1);
		XmmInvalidate(rt);
		LOG_OPCODE();
	}
	void XSHW(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.pslld(va.get(), 16);
		c.psrad(va.get(), 16);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CNTB(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& v1 = XmmCopy(va);
		const XmmLink& vm = XmmAlloc();
		c.psrlq(v1.get(), 4);
		c.movdqa(vm.get(), XmmConst(u128::from8p(0xf)));
		c.pand(va.get(), vm.get());
		c.pand(v1.get(), vm.get());
		c.movdqa(vm.get(), XmmConst(u128::fromV(_mm_set_epi8(4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0))));
		c.pshufb(vm.get(), va.get());
		c.movdqa(va.get(), XmmConst(u128::fromV(_mm_set_epi8(4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0))));
		c.pshufb(va.get(), v1.get());
		c.paddb(va.get(), vm.get());
		XmmFinalize(va, rt);
		XmmFinalize(v1);
		XmmFinalize(vm);
		LOG_OPCODE();
	}
	void XSBH(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.psllw(va.get(), 8);
		c.psraw(va.get(), 8);
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CLGT(u32 rt, u32 ra, u32 rb)
	{
		// compare if-greater-than
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vi = XmmAlloc();
		c.movdqa(vi.get(), XmmConst(u128::from32p(0x80000000)));
		c.pxor(va.get(), vi.get());
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pxor(vi.get(), vb->read());
		}
		else
		{
			c.pxor(vi.get(), cpu_xmm(GPR[rb]));
		}
		c.pcmpgtd(va.get(), vi.get());
		XmmFinalize(va, rt);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	void ANDC(u32 rt, u32 ra, u32 rb)
	{
		// and not
		const XmmLink& vb = XmmGet(rb, rt);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.pandn(vb.get(), va->read());
		}
		else
		{
			c.pandn(vb.get(), cpu_xmm(GPR[ra]));
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void FCGT(u32 rt, u32 ra, u32 rb)
	{
		// reverted less-than
		const XmmLink& vb = XmmGet(rb, rt);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.cmpps(vb.get(), va->read(), 1);
		}
		else
		{
			c.cmpps(vb.get(), cpu_xmm(GPR[ra]), 1);
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void DFCGT(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void FA(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.addps(va.get(), vb->read());
		}
		else
		{
			c.addps(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void FS(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.subps(va.get(), vb->read());
		}
		else
		{
			c.subps(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void FM(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.mulps(va.get(), vb->read());
		}
		else
		{
			c.mulps(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CLGTH(u32 rt, u32 ra, u32 rb)
	{
		// compare if-greater-than
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vi = XmmAlloc();
		c.movdqa(vi.get(), XmmConst(u128::from16p(0x8000)));
		c.pxor(va.get(), vi.get());
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pxor(vi.get(), vb->read());
		}
		else
		{
			c.pxor(vi.get(), cpu_xmm(GPR[rb]));
		}
		c.pcmpgtw(va.get(), vi.get());
		XmmFinalize(va, rt);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	void ORC(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vb = XmmGet(rb, rt);
		c.pxor(vb.get(), XmmConst(u128::from32p(0xffffffff)));
		if (const XmmLink* va = XmmRead(ra))
		{
			c.por(vb.get(), va->read());
		}
		else
		{
			c.por(vb.get(), cpu_xmm(GPR[ra]));
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void FCMGT(u32 rt, u32 ra, u32 rb)
	{
		// reverted less-than
		const XmmLink& vb = XmmGet(rb, rt);
		const XmmLink& vi = XmmAlloc();
		c.movaps(vi.get(), XmmConst(u128::from32p(0x7fffffff)));
		c.andps(vb.get(), vi.get()); // abs
		if (const XmmLink* va = XmmRead(ra))
		{
			c.andps(vi.get(), va->read());
		}
		else
		{
			c.andps(vi.get(), cpu_xmm(GPR[ra]));
		}
		c.cmpps(vb.get(), vi.get(), 1);
		XmmFinalize(vb, rt);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	void DFCMGT(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void DFA(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.addpd(va.get(), vb->read());
		}
		else
		{
			c.addpd(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void DFS(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.subpd(va.get(), vb->read());
		}
		else
		{
			c.subpd(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void DFM(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.mulpd(va.get(), vb->read());
		}
		else
		{
			c.mulpd(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CLGTB(u32 rt, u32 ra, u32 rb)
	{
		// compare if-greater-than
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vi = XmmAlloc();
		c.movdqa(vi.get(), XmmConst(u128::from8p(0x80)));
		c.pxor(va.get(), vi.get());
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pxor(vi.get(), vb->read());
		}
		else
		{
			c.pxor(vi.get(), cpu_xmm(GPR[rb]));
		}
		c.pcmpgtb(va.get(), vi.get());
		XmmFinalize(va, rt);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	void HLGT(u32 rt, u32 ra, u32 rb)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.cmp(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.seta(addr->r8());
		c.shl(*addr, 24);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.or_(*pos_var, *addr);
		do_finalize = true;
		LOG_OPCODE();
	}
	void DFMA(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vr = XmmGet(rt, rt);
		const XmmLink& va = XmmGet(ra);
		c.mulpd(va.get(), cpu_xmm(GPR[rb]));
		c.addpd(vr.get(), va.get());
		XmmFinalize(vr, rt);
		XmmFinalize(va);
		LOG_OPCODE();
	}
	void DFMS(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vt = (ra == rt) ? XmmCopy(va) : XmmGet(rt);
		c.mulpd(va.get(), cpu_xmm(GPR[rb]));
		c.subpd(va.get(), vt.get());
		XmmFinalize(va, rt);
		XmmFinalize(vt);
		LOG_OPCODE();
	}
	void DFNMS(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vr = XmmGet(rt, rt);
		const XmmLink& va = XmmGet(ra);
		c.mulpd(va.get(), cpu_xmm(GPR[rb]));
		c.subpd(vr.get(), va.get());
		XmmFinalize(vr, rt);
		XmmFinalize(va);
		LOG_OPCODE();
	}
	void DFNMA(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vt = (ra == rt) ? XmmCopy(va) : XmmGet(rt);
		c.mulpd(va.get(), cpu_xmm(GPR[rb]));
		c.addpd(vt.get(), va.get());
		c.xorpd(va.get(), va.get());
		c.subpd(va.get(), vt.get());
		XmmFinalize(va, rt);
		XmmFinalize(vt);
		LOG_OPCODE();
	}
	void CEQ(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pcmpeqd(va.get(), vb->read());
		}
		else
		{
			c.pcmpeqd(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void MPYHHU(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		const XmmLink& va2 = XmmCopy(va);
		c.pmulhuw(va.get(), vb.get());
		c.pmullw(va2.get(), vb.get());
		c.pand(va.get(), XmmConst(u128::from32p(0xffff0000)));
		c.psrld(va2.get(), 16);
		c.por(va.get(), va2.get());
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		XmmFinalize(va2);
		LOG_OPCODE();
	}
	void ADDX(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vt = XmmGet(rt);
		c.pand(vt.get(), XmmConst(u128::from32p(1)));
		c.paddd(vt.get(), cpu_xmm(GPR[ra]));
		c.paddd(vt.get(), cpu_xmm(GPR[rb]));
		XmmFinalize(vt, rt);
		LOG_OPCODE();
	}
	void SFX(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vt = XmmGet(rt);
		const XmmLink& vb = XmmGet(rb, rt);
		c.pandn(vt.get(), XmmConst(u128::from32p(1)));
		c.psubd(vb.get(), cpu_xmm(GPR[ra]));
		c.psubd(vb.get(), vt.get());
		XmmFinalize(vb, rt);
		XmmFinalize(vt);
		LOG_OPCODE();
	}
	void CGX(u32 rt, u32 ra, u32 rb) //nf
	{
		WRAPPER_BEGIN(rt, ra, rb, zz);
		for (int w = 0; w < 4; w++)
			CPU.GPR[rt]._u32[w] = ((u64)CPU.GPR[ra]._u32[w] + (u64)CPU.GPR[rb]._u32[w] + (u64)(CPU.GPR[rt]._u32[w] & 1)) >> 32;
		WRAPPER_END(rt, ra, rb, 0);
	}
	void BGX(u32 rt, u32 ra, u32 rb) //nf
	{
		WRAPPER_BEGIN(rt, ra, rb, zz);
		s64 nResult;
		
		for (int w = 0; w < 4; w++)
		{
			nResult = (u64)CPU.GPR[rb]._u32[w] - (u64)CPU.GPR[ra]._u32[w] - (u64)(1 - (CPU.GPR[rt]._u32[w] & 1));
			CPU.GPR[rt]._u32[w] = nResult < 0 ? 0 : 1;
		}
		WRAPPER_END(rt, ra, rb, 0);
	}
	void MPYHHA(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vt = XmmGet(rt, rt);
		const XmmLink& va = XmmGet(ra);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		c.psrld(va.get(), 16);
		c.psrld(vb.get(), 16);
		c.pmaddwd(va.get(), vb.get());
		c.paddd(vt.get(), va.get());
		XmmFinalize(vt, rt);
		XmmFinalize(va);
		XmmFinalize(vb);
		LOG_OPCODE();
	}
	void MPYHHAU(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vt = XmmGet(rt, rt);
		const XmmLink& va = XmmGet(ra);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		const XmmLink& va2 = XmmCopy(va);
		c.pmulhuw(va.get(), vb.get());
		c.pmullw(va2.get(), vb.get());
		c.pand(va.get(), XmmConst(u128::from32p(0xffff0000)));
		c.psrld(va2.get(), 16);
		c.paddd(vt.get(), va.get());
		c.paddd(vt.get(), va2.get());
		XmmFinalize(vt, rt);
		XmmFinalize(va);
		XmmFinalize(vb);
		XmmFinalize(va2);
		LOG_OPCODE();
	}
	void FSCRRD(u32 rt)
	{
		// zero (hack)
		const XmmLink& v0 = XmmAlloc(rt);
		c.pxor(v0.get(), v0.get());
		XmmFinalize(v0, rt);
		LOG_OPCODE();
	}
	void FESD(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.shufps(va.get(), va.get(), 0x8d); // _f[0] = _f[1]; _f[1] = _f[3];
		c.cvtps2pd(va.get(), va.get());
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void FRDS(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.cvtpd2ps(va.get(), va.get());
		c.shufps(va.get(), va.get(), 0x72); // _f[1] = _f[0]; _f[3] = _f[1]; _f[0] = _f[2] = 0;
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void FSCRWR(u32 rt, u32 ra)
	{
		// nop (not implemented)
		LOG_OPCODE();
	}
	void DFTSV(u32 rt, u32 ra, s32 i7)
	{
		UNIMPLEMENTED();
	}
	void FCEQ(u32 rt, u32 ra, u32 rb)
	{
		// compare equal
		const XmmLink& vb = XmmGet(rb, rt);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.cmpps(vb.get(), va->read(), 0);
		}
		else
		{
			c.cmpps(vb.get(), cpu_xmm(GPR[ra]), 0);
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void DFCEQ(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void MPY(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		const XmmLink& vi = XmmAlloc();
		c.movdqa(vi.get(), XmmConst(u128::from32p(0xffff)));
		c.pand(va.get(), vi.get());
		c.pand(vb.get(), vi.get());
		c.pmaddwd(va.get(), vb.get());
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	void MPYH(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		c.psrld(va.get(), 16);
		c.pmullw(va.get(), vb.get());
		c.pslld(va.get(), 16);
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		LOG_OPCODE();
	}
	void MPYHH(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		c.psrld(va.get(), 16);
		c.psrld(vb.get(), 16);
		c.pmaddwd(va.get(), vb.get());
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		LOG_OPCODE();
	}
	void MPYS(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		c.pmulhw(va.get(), vb.get());
		c.pslld(va.get(), 16);
		c.psrad(va.get(), 16);
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		LOG_OPCODE();
	}
	void CEQH(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pcmpeqw(va.get(), vb->read());
		}
		else
		{
			c.pcmpeqw(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void FCMEQ(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vb = XmmGet(rb, rt);
		const XmmLink& vi = XmmAlloc();
		c.movaps(vi.get(), XmmConst(u128::from32p(0x7fffffff)));
		c.andps(vb.get(), vi.get()); // abs
		if (const XmmLink* va = XmmRead(ra))
		{
			c.andps(vi.get(), va->read());
		}
		else
		{
			c.andps(vi.get(), cpu_xmm(GPR[ra]));
		}
		c.cmpps(vb.get(), vi.get(), 0); // ==
		XmmFinalize(vb, rt);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	void DFCMEQ(u32 rt, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
	}
	void MPYU(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		const XmmLink& va2 = XmmCopy(va);
		c.pmulhuw(va.get(), vb.get());
		c.pmullw(va2.get(), vb.get());
		c.pslld(va.get(), 16);
		c.pand(va2.get(), XmmConst(u128::from32p(0xffff)));
		c.por(va.get(), va2.get());
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		XmmFinalize(va2);
		LOG_OPCODE();
	}
	void CEQB(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (const XmmLink* vb = XmmRead(rb))
		{
			c.pcmpeqb(va.get(), vb->read());
		}
		else
		{
			c.pcmpeqb(va.get(), cpu_xmm(GPR[rb]));
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void FI(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vb = XmmGet(rb);
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void HEQ(u32 rt, u32 ra, u32 rb)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._s32[3]));
		c.cmp(*addr, cpu_dword(GPR[rb]._s32[3]));
		c.sete(addr->r8());
		c.shl(*addr, 24);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.or_(*pos_var, *addr);
		do_finalize = true;
		LOG_OPCODE();
	}

	//0 - 9
	void CFLTS(u32 rt, u32 ra, s32 i8)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (i8 != 173)
		{
			c.mulps(va.get(), XmmConst(u128::fromF(_mm_set1_ps(exp2f(static_cast<float>(173 - (i8 & 0xff))))))); // scale
		}
		const XmmLink& vi = XmmAlloc();
		c.movaps(vi.get(), XmmConst(u128::fromF(_mm_set1_ps(exp2f(31)))));
		c.cmpps(vi.get(), va.get(), 2);
		c.cvttps2dq(va.get(), va.get()); // convert to ints with truncation
		c.pxor(va.get(), vi.get()); // fix result saturation (0x80000000 -> 0x7fffffff)
		XmmFinalize(va, rt);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	void CFLTU(u32 rt, u32 ra, s32 i8)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (i8 != 173)
		{
			c.mulps(va.get(), XmmConst(u128::fromF(_mm_set1_ps(exp2f(static_cast<float>(173 - (i8 & 0xff))))))); // scale
		}
		c.maxps(va.get(), XmmConst({})); // saturate
		const XmmLink& vs = XmmCopy(va); // copy scaled value
		const XmmLink& vs2 = XmmCopy(va);
		const XmmLink& vs3 = XmmAlloc();
		c.movaps(vs3.get(), XmmConst(u128::fromF(_mm_set1_ps(exp2f(31)))));
		c.subps(vs2.get(), vs3.get());
		c.cmpps(vs3.get(), vs.get(), 2);
		c.andps(vs2.get(), vs3.get());
		c.cvttps2dq(va.get(), va.get());
		c.cmpps(vs.get(), XmmConst(u128::fromF(_mm_set1_ps(exp2f(32)))), 5);
		c.cvttps2dq(vs2.get(), vs2.get());
		c.por(va.get(), vs.get());
		c.por(va.get(), vs2.get());
		XmmFinalize(va, rt);
		XmmFinalize(vs);
		XmmFinalize(vs2);
		XmmFinalize(vs3);
		LOG_OPCODE();
	}
	void CSFLT(u32 rt, u32 ra, s32 i8)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.cvtdq2ps(va.get(), va.get()); // convert to floats
		if (i8 != 155)
		{
			c.mulps(va.get(), XmmConst(u128::fromF(_mm_set1_ps(exp2f(static_cast<float>((i8 & 0xff) - 155)))))); // scale
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CUFLT(u32 rt, u32 ra, s32 i8)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& v1 = XmmCopy(va);
		c.pand(va.get(), XmmConst(u128::from32p(0x7fffffff)));
		c.cvtdq2ps(va.get(), va.get()); // convert to floats
		c.psrad(v1.get(), 31); // generate mask from sign bit
		c.andps(v1.get(), XmmConst(u128::fromF(_mm_set1_ps(exp2f(31))))); // generate correction component
		c.addps(va.get(), v1.get()); // add correction component
		if (i8 != 155)
		{
			c.mulps(va.get(), XmmConst(u128::fromF(_mm_set1_ps(exp2f(static_cast<float>((i8 & 0xff) - 155)))))); // scale
		}
		XmmFinalize(va, rt);
		XmmFinalize(v1);
		LOG_OPCODE();
	}

	//0 - 8
	void BRZ(u32 rt, s32 i16)
	{
		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*addr, (CPU.PC >> 2) + 1);
		c.mov(*pos_var, branchTarget(CPU.PC, i16) >> 2);
		c.cmp(cpu_dword(GPR[rt]._u32[3]), 0);
		c.cmovne(*pos_var, *addr);
		LOG_OPCODE();
	}
	void STQA(u32 rt, s32 i16)
	{
		const u32 lsa = (i16 << 2) & 0x3fff0;

		/*const XmmLink& vt = XmmGet(rt);
		c.pshufb(vt.get(), XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c.movdqa(oword_ptr(*ls_var, lsa), vt.get());
		XmmFinalize(vt);*/

		c.mov(*qw0, cpu_qword(GPR[rt]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[rt]._u64[1]));
		c.bswap(*qw0);
		c.bswap(*qw1);
		c.mov(qword_ptr(*ls_var, lsa), *qw1);
		c.mov(qword_ptr(*ls_var, lsa + 8), *qw0);

		LOG_OPCODE();
	}
	void BRNZ(u32 rt, s32 i16)
	{
		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*addr, (CPU.PC >> 2) + 1);
		c.mov(*pos_var, branchTarget(CPU.PC, i16) >> 2);
		c.cmp(cpu_dword(GPR[rt]._u32[3]), 0);
		c.cmove(*pos_var, *addr);
		LOG_OPCODE();
	}
	void BRHZ(u32 rt, s32 i16)
	{
		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*addr, (CPU.PC >> 2) + 1);
		c.mov(*pos_var, branchTarget(CPU.PC, i16) >> 2);
		c.cmp(cpu_word(GPR[rt]._u16[6]), 0);
		c.cmovnz(*pos_var, *addr);
		LOG_OPCODE();
	}
	void BRHNZ(u32 rt, s32 i16)
	{
		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*addr, (CPU.PC >> 2) + 1);
		c.mov(*pos_var, branchTarget(CPU.PC, i16) >> 2);
		c.cmp(cpu_word(GPR[rt]._u16[6]), 0);
		c.cmovz(*pos_var, *addr);
		LOG_OPCODE();
	}
	void STQR(u32 rt, s32 i16)
	{
		const u32 lsa = branchTarget(CPU.PC, i16) & 0x3fff0;

		/*const XmmLink& vt = XmmGet(rt);
		c.pshufb(vt.get(), XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c.movdqa(oword_ptr(*ls_var, lsa), vt.get());
		XmmFinalize(vt);*/

		c.mov(*qw0, cpu_qword(GPR[rt]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[rt]._u64[1]));
		c.bswap(*qw0);
		c.bswap(*qw1);
		c.mov(qword_ptr(*ls_var, lsa), *qw1);
		c.mov(qword_ptr(*ls_var, lsa + 8), *qw0);

		LOG_OPCODE();
	}
	void BRA(s32 i16)
	{
		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*pos_var, branchTarget(0, i16) >> 2);
		LOG_OPCODE();
	}
	void LQA(u32 rt, s32 i16)
	{
		XmmInvalidate(rt);

		const u32 lsa = (i16 << 2) & 0x3fff0;

		/*const XmmLink& vt = XmmAlloc(rt);
		c.movdqa(vt.get(), oword_ptr(*ls_var, lsa));
		c.pshufb(vt.get(), XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		XmmFinalize(vt, rt);*/

		c.mov(*qw0, qword_ptr(*ls_var, lsa));
		c.mov(*qw1, qword_ptr(*ls_var, lsa + 8));
		c.bswap(*qw0);
		c.bswap(*qw1);
		c.mov(cpu_qword(GPR[rt]._u64[0]), *qw1);
		c.mov(cpu_qword(GPR[rt]._u64[1]), *qw0);

		LOG_OPCODE();
	}
	void BRASL(u32 rt, s32 i16)
	{
		XmmInvalidate(rt);

		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.xor_(*addr, *addr); // zero
		c.mov(cpu_dword(GPR[rt]._u32[0]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[1]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[2]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[3]), CPU.PC + 4);
		c.mov(*pos_var, branchTarget(0, i16) >> 2);
		LOG_OPCODE();
	}
	void BR(s32 i16)
	{
		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.mov(*pos_var, branchTarget(CPU.PC, i16) >> 2);
		LOG_OPCODE();
	}
	void FSMBI(u32 rt, s32 i16)
	{
		if (i16 == 0)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
		{
			const XmmLink& vr = XmmAlloc(rt);
			c.movdqa(vr.get(), g_imm_xmm(fsmb[i16 & 0xffff]));
			XmmFinalize(vr, rt);
		}
		LOG_OPCODE();
	}
	void BRSL(u32 rt, s32 i16)
	{
		XmmInvalidate(rt);

		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;

		c.xor_(*addr, *addr); // zero
		c.mov(cpu_dword(GPR[rt]._u32[0]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[1]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[2]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[3]), CPU.PC + 4);
		c.mov(*pos_var, branchTarget(CPU.PC, i16) >> 2);
		LOG_OPCODE();
	}
	void LQR(u32 rt, s32 i16)
	{
		XmmInvalidate(rt);

		const u32 lsa = branchTarget(CPU.PC, i16) & 0x3fff0;

		/*const XmmLink& vt = XmmAlloc(rt);
		c.movdqa(vt.get(), oword_ptr(*ls_var, lsa));
		c.pshufb(vt.get(), XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		XmmFinalize(vt, rt);*/

		c.mov(*qw0, qword_ptr(*ls_var, lsa));
		c.mov(*qw1, qword_ptr(*ls_var, lsa + 8));
		c.bswap(*qw0);
		c.bswap(*qw1);
		c.mov(cpu_qword(GPR[rt]._u64[0]), *qw1);
		c.mov(cpu_qword(GPR[rt]._u64[1]), *qw0);

		LOG_OPCODE();
	}
	void IL(u32 rt, s32 i16)
	{
		const XmmLink& vr = XmmAlloc(rt);
		if (i16 == 0)
		{
			c.pxor(vr.get(), vr.get());
		}
		else if (i16 == -1)
		{
			c.pcmpeqd(vr.get(), vr.get());
		}
		else
		{
			c.movdqa(vr.get(), XmmConst(u128::from32p(i16)));
		}
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void ILHU(u32 rt, s32 i16)
	{
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(u128::from32p(i16 << 16)));
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void ILH(u32 rt, s32 i16)
	{
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(u128::from32p(i16)));
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void IOHL(u32 rt, s32 i16)
	{
		const XmmLink& vt = XmmGet(rt, rt);
		c.por(vt.get(), XmmConst(u128::from32p(i16 & 0xffff)));
		XmmFinalize(vt, rt);
		LOG_OPCODE();
	}
	

	//0 - 7
	void ORI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == -1)
		{
			// fill with 1
			const XmmLink& v1 = XmmAlloc(rt);
			c.pcmpeqd(v1.get(), v1.get());
			XmmFinalize(v1, rt);
		}
		else if (i10 == 0)
		{
			if (rt != ra)
			{
				// mov
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			const XmmLink& va = XmmGet(ra, rt);
			c.por(va.get(), XmmConst(u128::from32p(i10)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void ORHI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.por(va.get(), XmmConst(u128::from16p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ORBI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.por(va.get(), XmmConst(u128::from8p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void SFI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == 0)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			c.psubd(v0.get(), cpu_xmm(GPR[ra]));
			XmmFinalize(v0, rt);
		}
		else if (i10 == -1)
		{
			// fill with 1
			const XmmLink& v1 = XmmAlloc(rt);
			c.pcmpeqd(v1.get(), v1.get());
			c.psubd(v1.get(), cpu_xmm(GPR[ra]));
			XmmFinalize(v1, rt);
		}
		else
		{
			const XmmLink& vr = XmmAlloc(rt);
			c.movdqa(vr.get(), XmmConst(u128::from32p(i10)));
			c.psubd(vr.get(), cpu_xmm(GPR[ra]));
			XmmFinalize(vr, rt);
		}
		LOG_OPCODE();
	}
	void SFHI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == 0)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			c.psubw(v0.get(), cpu_xmm(GPR[ra]));
			XmmFinalize(v0, rt);
		}
		else if (i10 == -1)
		{
			// fill with 1
			const XmmLink& v1 = XmmAlloc(rt);
			c.pcmpeqw(v1.get(), v1.get());
			c.psubw(v1.get(), cpu_xmm(GPR[ra]));
			XmmFinalize(v1, rt);
		}
		else
		{
			const XmmLink& vr = XmmAlloc(rt);
			c.movdqa(vr.get(), XmmConst(u128::from16p(i10)));
			c.psubw(vr.get(), cpu_xmm(GPR[ra]));
			XmmFinalize(vr, rt);
		}
		LOG_OPCODE();
	}
	void ANDI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.pand(va.get(), XmmConst(u128::from32p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ANDHI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.pand(va.get(), XmmConst(u128::from16p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ANDBI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.pand(va.get(), XmmConst(u128::from8p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void AI(u32 rt, u32 ra, s32 i10)
	{
		// add
		const XmmLink& va = XmmGet(ra, rt);
		c.paddd(va.get(), XmmConst(u128::from32p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void AHI(u32 rt, u32 ra, s32 i10)
	{
		// add
		const XmmLink& va = XmmGet(ra, rt);
		c.paddw(va.get(), XmmConst(u128::from16p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void STQD(u32 rt, s32 i10, u32 ra) // i10 is shifted left by 4 while decoding
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		if (i10) c.add(*addr, i10);
		c.and_(*addr, 0x3fff0);

		/*const XmmLink& vt = XmmGet(rt);
		c.pshufb(vt.get(), XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		c.movdqa(oword_ptr(*ls_var, *addr), vt.get());
		XmmFinalize(vt);*/

		c.mov(*qw0, cpu_qword(GPR[rt]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[rt]._u64[1]));
		c.bswap(*qw0);
		c.bswap(*qw1);
		c.mov(qword_ptr(*ls_var, *addr, 0, 0), *qw1);
		c.mov(qword_ptr(*ls_var, *addr, 0, 8), *qw0);

		LOG_OPCODE();
	}
	void LQD(u32 rt, s32 i10, u32 ra) // i10 is shifted left by 4 while decoding
	{
		XmmInvalidate(rt);

		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		if (i10) c.add(*addr, i10);
		c.and_(*addr, 0x3fff0);

		/*const XmmLink& vt = XmmAlloc(rt);
		c.movdqa(vt.get(), oword_ptr(*ls_var, *addr));
		c.pshufb(vt.get(), XmmConst(_mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f)));
		XmmFinalize(vt, rt);*/

		c.mov(*qw0, qword_ptr(*ls_var, *addr, 0, 0));
		c.mov(*qw1, qword_ptr(*ls_var, *addr, 0, 8));
		c.bswap(*qw0);
		c.bswap(*qw1);
		c.mov(cpu_qword(GPR[rt]._u64[0]), *qw1);
		c.mov(cpu_qword(GPR[rt]._u64[1]), *qw0);

		LOG_OPCODE();
	}
	void XORI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pxor(va.get(), XmmConst(u128::from32p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void XORHI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pxor(va.get(), XmmConst(u128::from16p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void XORBI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pxor(va.get(), XmmConst(u128::from8p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CGTI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpgtd(va.get(), XmmConst(u128::from32p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CGTHI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpgtw(va.get(), XmmConst(u128::from16p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CGTBI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpgtb(va.get(), XmmConst(u128::from8p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void HGTI(u32 rt, u32 ra, s32 i10)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._s32[3]));
		c.cmp(*addr, i10);
		c.setg(addr->r8());
		c.shl(*addr, 24);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.or_(*pos_var, *addr);
		do_finalize = true;
		LOG_OPCODE();
	}
	void CLGTI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pxor(va.get(), XmmConst(u128::from32p(0x80000000)));
		c.pcmpgtd(va.get(), XmmConst(u128::from32p((u32)i10 - 0x80000000)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CLGTHI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pxor(va.get(), XmmConst(u128::from16p(0x8000)));
		c.pcmpgtw(va.get(), XmmConst(u128::from16p((u16)i10 - 0x8000)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CLGTBI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.psubb(va.get(), XmmConst(u128::from8p(0x80)));
		c.pcmpgtb(va.get(), XmmConst(u128::from8p((s8)i10 - 0x80)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void HLGTI(u32 rt, u32 ra, s32 i10)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.cmp(*addr, i10);
		c.seta(addr->r8());
		c.shl(*addr, 24);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.or_(*pos_var, *addr);
		do_finalize = true;
		LOG_OPCODE();
	}
	void MPYI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.pmaddwd(va.get(), XmmConst(u128::from32p(i10 & 0xffff)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void MPYUI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vi = XmmAlloc();
		const XmmLink& va2 = XmmCopy(va);
		c.movdqa(vi.get(), XmmConst(u128::from32p(i10 & 0xffff)));
		c.pmulhuw(va.get(), vi.get());
		c.pmullw(va2.get(), vi.get());
		c.pslld(va.get(), 16);
		c.por(va.get(), va2.get());
		XmmFinalize(va, rt);
		XmmFinalize(vi);
		XmmFinalize(va2);
		LOG_OPCODE();
	}
	void CEQI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpeqd(va.get(), XmmConst(u128::from32p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CEQHI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpeqw(va.get(), XmmConst(u128::from16p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CEQBI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpeqb(va.get(), XmmConst(u128::from8p(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void HEQI(u32 rt, u32 ra, s32 i10)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.cmp(*addr, i10);
		c.sete(addr->r8());
		c.shl(*addr, 24);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.or_(*pos_var, *addr);
		do_finalize = true;
		LOG_OPCODE();
	}


	//0 - 6
	void HBRA(s32 ro, s32 i16)
	{ //i16 is shifted left by 2 while decoding
		LOG_OPCODE();
	}
	void HBRR(s32 ro, s32 i16)
	{
		LOG_OPCODE();
	}
	void ILA(u32 rt, u32 i18)
	{
		const XmmLink& vr = XmmAlloc(rt);
		if (i18 == 0)
		{
			c.pxor(vr.get(), vr.get());
		}
		else
		{
			c.movdqa(vr.get(), XmmConst(u128::from32p(i18 & 0x3ffff)));
		}
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}

	//0 - 3
	void SELB(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		const XmmLink& vb = XmmGet(rb);
		const XmmLink& vc = XmmGet(rc);
		c.pand(vb.get(), vc.get());
		c.pandn(vc.get(), cpu_xmm(GPR[ra]));
		c.por(vb.get(), vc.get());
		XmmFinalize(vb, rt);
		XmmFinalize(vc);
		LOG_OPCODE();
	}
	void SHUFB(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		const XmmLink& v0 = XmmGet(rc); // v0 = mask
		const XmmLink& v1 = XmmAlloc();
		const XmmLink& v2 = XmmCopy(v0); // v2 = mask
		const XmmLink& v3 = XmmAlloc();
		const XmmLink& v4 = XmmAlloc();
		const XmmLink& vFF = XmmAlloc(rt);
		// generate specific values:
		c.movdqa(v1.get(), XmmConst(u128::from8p(0xe0))); // v1 = 11100000
		c.movdqa(v3.get(), XmmConst(u128::from8p(0x80))); // v3 = 10000000
		c.pand(v2.get(), v1.get()); // filter mask      v2 = mask & 11100000
		c.movdqa(vFF.get(), v2.get()); // and copy      vFF = mask & 11100000
		c.movdqa(v4.get(), XmmConst(u128::from8p(0xc0))); // v4 = 11000000
		c.pcmpeqb(vFF.get(), v4.get()); // gen 0xff     vFF = (mask & 11100000 == 11000000) ? 0xff : 0
		c.movdqa(v4.get(), v2.get()); // copy again     v4 = mask & 11100000
		c.pand(v4.get(), v3.get()); // filter mask      v4 = mask & 10000000
		c.pcmpeqb(v2.get(), v1.get()); //               v2 = (mask & 11100000 == 11100000) ? 0xff : 0
		c.pcmpeqb(v4.get(), v3.get()); //               v4 = (mask & 10000000 == 10000000) ? 0xff : 0
		c.pand(v2.get(), v3.get()); // generate 0x80    v2 = (mask & 11100000 == 11100000) ? 0x80 : 0
		c.por(vFF.get(), v2.get()); // merge 0xff, 0x80 vFF = (mask & 11100000 == 11000000) ? 0xff : (mask & 11100000 == 11100000) ? 0x80 : 0
		c.pandn(v1.get(), v0.get()); // filter mask     v1 = mask & 00011111
		// select bytes from [rb]:
		c.movdqa(v2.get(), XmmConst(u128::from8p(0x0f))); //   v2 = 00001111
		c.pxor(v1.get(), XmmConst(u128::from8p(0x10))); //   v1 = (mask & 00011111) ^ 00010000
		c.psubb(v2.get(), v1.get()); //                 v2 = 00001111 - ((mask & 00011111) ^ 00010000)
		c.movdqa(v1.get(), cpu_xmm(GPR[rb])); //        v1 = rb
		c.pshufb(v1.get(), v2.get()); //                v1 = select(rb, 00001111 - ((mask & 00011111) ^ 00010000))
		// select bytes from [ra]:
		c.pxor(v2.get(), XmmConst(u128::from8p(0xf0))); //   v2 = (00001111 - ((mask & 00011111) ^ 00010000)) ^ 11110000
		c.movdqa(v3.get(), cpu_xmm(GPR[ra])); //        v3 = ra
		c.pshufb(v3.get(), v2.get()); //                v3 = select(ra, (00001111 - ((mask & 00011111) ^ 00010000)) ^ 11110000)
		c.por(v1.get(), v3.get()); //                   v1 = select(rb, 00001111 - ((mask & 00011111) ^ 00010000)) | (v3)
		c.pandn(v4.get(), v1.get()); // filter result   v4 = v1 & ((mask & 10000000 == 10000000) ? 0 : 0xff)
		c.por(vFF.get(), v4.get()); // final merge      vFF = (mask & 10000000 == 10000000) ? ((mask & 11100000 == 11000000) ? 0xff : (mask & 11100000 == 11100000) ? 0x80 : 0) : (v1)
		XmmFinalize(vFF, rt);
		XmmFinalize(v4);
		XmmFinalize(v3);
		XmmFinalize(v2);
		XmmFinalize(v1);
		XmmFinalize(v0);
		LOG_OPCODE();
	}
	void MPYA(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = XmmGet(rb);
		const XmmLink& vi = XmmAlloc();
		c.movdqa(vi.get(), XmmConst(u128::from32p(0xffff)));
		c.pand(va.get(), vi.get());
		c.pand(vb.get(), vi.get());
		c.pmaddwd(va.get(), vb.get());
		c.paddd(va.get(), cpu_xmm(GPR[rc]));
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		XmmFinalize(vi);
		LOG_OPCODE();
	}
	void FNMS(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		const XmmLink& va = XmmGet(ra);
		const XmmLink& vc = (ra == rc) ? XmmCopy(va, rt) : XmmGet(rc, rt);

		if (ra == rb)
		{
			c.mulps(va.get(), va.get());
		}
		else if (rb == rc)
		{
			c.mulps(va.get(), vc.get());
		}
		else
		{
			if (const XmmLink* vb = XmmRead(rb))
			{
				c.mulps(va.get(), vb->read());
			}
			else
			{
				c.mulps(va.get(), cpu_xmm(GPR[rb]));
			}
		}
		c.subps(vc.get(), va.get());
		XmmFinalize(vc, rt);
		XmmFinalize(va);
		LOG_OPCODE();
	}
	void FMA(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		if (ra != rb && rb != rc && rc != ra)
		{
			const XmmLink& va = XmmGet(ra, rt);
			if (const XmmLink* vb = XmmRead(rb))
			{
				c.mulps(va.get(), vb->read());
			}
			else
			{
				c.mulps(va.get(), cpu_xmm(GPR[rb]));
			}
			if (const XmmLink* vc = XmmRead(rc))
			{
				c.addps(va.get(), vc->read());
			}
			else
			{
				c.addps(va.get(), cpu_xmm(GPR[rc]));
			}
			XmmFinalize(va, rt);
		}
		else if (ra == rb && rb == rc)
		{
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& vc = XmmCopy(va);
			c.mulps(va.get(), va.get());
			c.addps(va.get(), vc.get());
			XmmFinalize(va, rt);
			XmmFinalize(vc);
		}
		else if (ra == rb)
		{
			const XmmLink& va = XmmGet(ra, rt);
			c.mulps(va.get(), va.get());
			if (const XmmLink* vc = XmmRead(rc))
			{
				c.addps(va.get(), vc->read());
			}
			else
			{
				c.addps(va.get(), cpu_xmm(GPR[rc]));
			}
			XmmFinalize(va, rt);
		}
		else if (rb == rc)
		{
			const XmmLink& va = XmmGet(ra, rt);
			if (const XmmLink* vc = XmmRead(rc))
			{
				c.mulps(va.get(), vc->read());
				c.addps(va.get(), vc->read());
			}
			else
			{
				const XmmLink& vb = XmmGet(rb, rb);
				c.mulps(va.get(), vb.get());
				c.addps(va.get(), vb.get());
				XmmFinalize(vb, rb);
			}
			XmmFinalize(va, rt);
		}
		else if (ra == rc)
		{
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& vc = XmmCopy(va);
			if (const XmmLink* vb = XmmRead(rb))
			{
				c.mulps(va.get(), vb->read());
			}
			else
			{
				c.mulps(va.get(), cpu_xmm(GPR[rb]));
			}
			c.addps(va.get(), vc.get());
			XmmFinalize(va, rt);
			XmmFinalize(vc);
		}
		else
		{
			throw "FMA: invalid case"; // should never happen
		}
		LOG_OPCODE();
	}
	void FMS(u32 rt, u32 ra, u32 rb, u32 rc)
	{
		if (ra != rb && rb != rc && rc != ra)
		{
			const XmmLink& va = XmmGet(ra, rt);
			if (const XmmLink* vb = XmmRead(rb))
			{
				c.mulps(va.get(), vb->read());
			}
			else
			{
				c.mulps(va.get(), cpu_xmm(GPR[rb]));
			}
			if (const XmmLink* vc = XmmRead(rc))
			{
				c.subps(va.get(), vc->read());
			}
			else
			{
				c.subps(va.get(), cpu_xmm(GPR[rc]));
			}
			XmmFinalize(va, rt);
		}
		else if (ra == rb && rb == rc)
		{
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& vc = XmmCopy(va);
			c.mulps(va.get(), va.get());
			c.subps(va.get(), vc.get());
			XmmFinalize(va, rt);
			XmmFinalize(vc);
		}
		else if (ra == rb)
		{
			const XmmLink& va = XmmGet(ra, rt);
			c.mulps(va.get(), va.get());
			if (const XmmLink* vc = XmmRead(rc))
			{
				c.subps(va.get(), vc->read());
			}
			else
			{
				c.subps(va.get(), cpu_xmm(GPR[rc]));
			}
			XmmFinalize(va, rt);
		}
		else if (rb == rc)
		{
			const XmmLink& va = XmmGet(ra, rt);
			if (const XmmLink* vc = XmmRead(rc))
			{
				c.mulps(va.get(), vc->read());
				c.subps(va.get(), vc->read());
			}
			else
			{
				const XmmLink& vb = XmmGet(rb, rb);
				c.mulps(va.get(), vb.get());
				c.subps(va.get(), vb.get());
				XmmFinalize(vb, rb);
			}
			XmmFinalize(va, rt);
		}
		else if (ra == rc)
		{
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& vc = XmmCopy(va);
			if (const XmmLink* vb = XmmRead(rb))
			{
				c.mulps(va.get(), vb->read());
			}
			else
			{
				c.mulps(va.get(), cpu_xmm(GPR[rb]));
			}
			c.subps(va.get(), vc.get());
			XmmFinalize(va, rt);
			XmmFinalize(vc);
		}
		else
		{
			throw "FMS: invalid case"; // should never happen
		}
		LOG_OPCODE();
	}

	void UNK(u32 code, u32 opcode, u32 gcode)
	{
		UNK(fmt::Format("Unimplemented opcode! (0x%08x, 0x%x, 0x%x)", code, opcode, gcode));
	}

	void UNK(const std::string& err)
	{
		LOG_ERROR(Log::SPU, "%s #pc: 0x%x", err.c_str(), CPU.PC);
		c.mov(cpu_dword(PC), CPU.PC);
		do_finalize = true;
		Emu.Pause();
	}

	
};

#undef c
