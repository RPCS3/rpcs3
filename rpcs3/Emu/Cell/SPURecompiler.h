#pragma once
#include "Emu/Cell/SPUOpcodes.h"
#include "Emu/Memory/Memory.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/SysCalls/SysCalls.h"

#define ASMJIT_STATIC

#include "asmjit.h"

using namespace asmjit;
using namespace asmjit::host;

#define UNIMPLEMENTED() UNK(__FUNCTION__)

#define mmToU64Ptr(x) ((u64*)(&x))
#define mmToU32Ptr(x) ((u32*)(&x))
#define mmToU16Ptr(x) ((u16*)(&x))
#define mmToU8Ptr(x) ((u8*)(&x))

struct g_imm_table_struct
{
	//u16 cntb_table[65536];

	__m128i fsmb_table[65536];
	__m128i fsmh_table[256];
	__m128i fsm_table[16];

	__m128i sldq_pshufb[32];
	__m128i srdq_pshufb[32];
	__m128i rldq_pshufb[16];

	g_imm_table_struct()
	{
		/*static_assert(offsetof(g_imm_table_struct, cntb_table) == 0, "offsetof(cntb_table) != 0");
		for (u32 i = 0; i < sizeof(cntb_table) / sizeof(cntb_table[0]); i++)
		{
			u32 cnt_low = 0, cnt_high = 0;
			for (u32 j = 0; j < 8; j++)
			{
				cnt_low += (i >> j) & 1;
				cnt_high += (i >> (j + 8)) & 1;
			}
			cntb_table[i] = (cnt_high << 8) | cnt_low;
		}*/
		for (u32 i = 0; i < sizeof(fsm_table) / sizeof(fsm_table[0]); i++)
		{

			for (u32 j = 0; j < 4; j++) mmToU32Ptr(fsm_table[i])[j] = (i & (1 << j)) ? ~0 : 0;
		}
		for (u32 i = 0; i < sizeof(fsmh_table) / sizeof(fsmh_table[0]); i++)
		{
			for (u32 j = 0; j < 8; j++) mmToU16Ptr(fsmh_table[i])[j] = (i & (1 << j)) ? ~0 : 0;
		}
		for (u32 i = 0; i < sizeof(fsmb_table) / sizeof(fsmb_table[0]); i++)
		{
			for (u32 j = 0; j < 16; j++) mmToU8Ptr(fsmb_table[i])[j] = (i & (1 << j)) ? ~0 : 0;
		}
		for (u32 i = 0; i < sizeof(sldq_pshufb) / sizeof(sldq_pshufb[0]); i++)
		{
			for (u32 j = 0; j < 16; j++) mmToU8Ptr(sldq_pshufb[i])[j] = (u8)(j - i);
		}
		for (u32 i = 0; i < sizeof(srdq_pshufb) / sizeof(srdq_pshufb[0]); i++)
		{
			for (u32 j = 0; j < 16; j++) mmToU8Ptr(srdq_pshufb[i])[j] = (j + i > 15) ? 0xff : (u8)(j + i);
		}
		for (u32 i = 0; i < sizeof(rldq_pshufb) / sizeof(rldq_pshufb[0]); i++)
		{
			for (u32 j = 0; j < 16; j++) mmToU8Ptr(rldq_pshufb[i])[j] = (u8)(j - i) & 0xf;
		}
	}
};

class SPURecompiler;

class SPURecompilerCore : public CPUDecoder
{
	SPURecompiler* m_enc;
	SPUThread& CPU;

public:
	SPUInterpreter* inter;
	JitRuntime runtime;
	bool first;

	struct SPURecEntry
	{
		//u16 host; // absolute position of first instruction of current block (not used now)
		u16 count; // count of instructions compiled from current point (and to be checked)
		u32 valid; // copy of valid opcode for validation
		void* pointer; // pointer to executable memory object
	};

	SPURecEntry entry[0x10000];

	std::vector<__m128i> imm_table;

	SPURecompilerCore(SPUThread& cpu);

	~SPURecompilerCore();

	void Compile(u16 pos);

	virtual void Decode(const u32 code);

	virtual u8 DecodeMemory(const u64 address);
};

#define c (*compiler)

#ifdef _WIN32
#define cpu_xmm(x) oword_ptr(*cpu_var, (sizeof((*(SPUThread*)nullptr).x) == 16) ? offsetof(SPUThread, x) : throw "sizeof("#x") != 16")
#define cpu_qword(x) qword_ptr(*cpu_var, (sizeof((*(SPUThread*)nullptr).x) == 8) ? offsetof(SPUThread, x) : throw "sizeof("#x") != 8")
#define cpu_dword(x) dword_ptr(*cpu_var, (sizeof((*(SPUThread*)nullptr).x) == 4) ? offsetof(SPUThread, x) : throw "sizeof("#x") != 4")
#define cpu_word(x) word_ptr(*cpu_var, (sizeof((*(SPUThread*)nullptr).x) == 2) ? offsetof(SPUThread, x) : throw "sizeof("#x") != 2")
#define cpu_byte(x) byte_ptr(*cpu_var, (sizeof((*(SPUThread*)nullptr).x) == 1) ? offsetof(SPUThread, x) : throw "sizeof("#x") != 1")

#define g_imm_xmm(x) oword_ptr(*g_imm_var, offsetof(g_imm_table_struct, x))
#define g_imm2_xmm(x, y) oword_ptr(*g_imm_var, y, 0, offsetof(g_imm_table_struct, x))
#else
#define cpu_xmm(x) oword_ptr(*cpu_var, reinterpret_cast<uintptr_t>(&(((SPUThread*)0)->x)) )
#define cpu_qword(x) qword_ptr(*cpu_var, reinterpret_cast<uintptr_t>(&(((SPUThread*)0)->x)) )
#define cpu_dword(x) dword_ptr(*cpu_var, reinterpret_cast<uintptr_t>(&(((SPUThread*)0)->x)) )
#define cpu_word(x) word_ptr(*cpu_var, reinterpret_cast<uintptr_t>(&(((SPUThread*)0)->x)) )
#define cpu_byte(x) byte_ptr(*cpu_var, reinterpret_cast<uintptr_t>(&(((SPUThread*)0)->x)) )

#define g_imm_xmm(x) oword_ptr(*g_imm_var, reinterpret_cast<uintptr_t>(&(((g_imm_table_struct*)0)->x)))
#define g_imm2_xmm(x, y) oword_ptr(*g_imm_var, y, 0, reinterpret_cast<uintptr_t>(&(((g_imm_table_struct*)0)->x)))
#endif


#define LOG_OPCODE(...) //ConLog.Write("Compiled "__FUNCTION__"(): "__VA_ARGS__)

#define LOG3_OPCODE(...) //ConLog.Write("Linked "__FUNCTION__"(): "__VA_ARGS__)

#define LOG4_OPCODE(...) //c.addComment(fmt::Format("SPU info: "__FUNCTION__"(): "__VA_ARGS__).c_str())

#define WRAPPER_BEGIN(a0, a1, a2, a3) struct opwr_##a0 \
{ \
	static void opcode(u32 a0, u32 a1, u32 a2, u32 a3) \
{ \
	SPUThread& CPU = *(SPUThread*)GetCurrentCPUThread();

#define WRAPPER_END(a0, a1, a2, a3) /*LOG2_OPCODE();*/ } \
}; \
	/*XmmRelease();*/ \
	if (#a0[0] == 'r') XmmInvalidate(a0); \
	if (#a1[0] == 'r') XmmInvalidate(a1); \
	if (#a2[0] == 'r') XmmInvalidate(a2); \
	if (#a3[0] == 'r') XmmInvalidate(a3); \
	X86X64CallNode* call##a0 = c.call(imm_ptr(&opwr_##a0::opcode), kFuncConvHost, FuncBuilder4<FnVoid, u32, u32, u32, u32>()); \
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
	Compiler* compiler;
	bool do_finalize;
	// input:
	GpVar* cpu_var;
	GpVar* ls_var;
	GpVar* imm_var;
	GpVar* g_imm_var;
	// output:
	GpVar* pos_var;
	// temporary:
	GpVar* addr;
	GpVar* qw0;
	GpVar* qw1;
	GpVar* qw2;

	struct XmmLink
	{
		XmmVar* data;
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
				assert(!xmm_var[i].got);
				if (xmm_var[i].got) throw "XmmRead(): wrong reuse";
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
		XmmLink* res = nullptr;
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
			res = &(XmmLink&)XmmAlloc(target);
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
			res->reg = -1; // ???
			LOG4_OPCODE("* cached GPR[%d] not found", reg);
		}
		return *res;
	}

	const XmmLink& XmmCopy(const XmmLink& from, s8 pref = -1) // XmmAlloc + mov
	{
		XmmLink* res = &(XmmLink&)XmmAlloc(pref);
		c.movdqa(*res->data, *from.data);
		res->reg = -1; // ???
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

	Mem XmmConst(const __m128i& data)
	{
		for (u32 i = 0; i < rec.imm_table.size(); i++)
		{
			if (mmToU64Ptr(rec.imm_table[i])[0] == mmToU64Ptr(data)[0] && mmToU64Ptr(rec.imm_table[i])[1] == mmToU64Ptr(data)[1])
			{
				return oword_ptr(*imm_var, i * sizeof(__m128i));
			}
		}
		const int shift = rec.imm_table.size() * sizeof(__m128i);
		rec.imm_table.push_back(data);
		return oword_ptr(*imm_var, shift);
	}

	Mem XmmConst(const __m128& data)
	{
		return XmmConst((__m128i&)data);
	}

private:
	//0 - 10
	void STOP(u32 code)
	{
		struct STOP_wrapper
		{
			static void STOP(u32 code)
			{
				SPUThread& CPU = *(SPUThread*)GetCurrentCPUThread();
				CPU.DoStop(code);
				LOG2_OPCODE();
			}
		};
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		X86X64CallNode* call = c.call(imm_ptr(&STOP_wrapper::STOP), kFuncConvHost, FuncBuilder1<FnVoid, u32>());
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
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		// This instruction must be used following a store instruction that modifies the instruction stream.
		c.mfence();
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
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
		//If register is a dummy register (register labeled 0x0)
		if(sa == 0x0)
		{
			CPU.GPR[rt]._u128.hi = 0x0;
			CPU.GPR[rt]._u128.lo = 0x0;
		}
		else
		{
			CPU.GPR[rt]._u128.hi =  CPU.SPR[sa]._u128.hi;
			CPU.GPR[rt]._u128.lo =  CPU.SPR[sa]._u128.lo;
		}
	}
	void RDCH(u32 rt, u32 ra)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		WRAPPER_BEGIN(rt, ra, yy, zz);
		CPU.ReadChannel(CPU.GPR[rt], ra);
		WRAPPER_END(rt, ra, 0, 0);
		// TODO
	}
	void RCHCNT(u32 rt, u32 ra)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		WRAPPER_BEGIN(rt, ra, yy, zz);
		CPU.GPR[rt].Reset();
		CPU.GPR[rt]._u32[3] = CPU.GetChannelCount(ra);
		WRAPPER_END(rt, ra, 0, 0);
		// TODO
	}
	void SF(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void OR(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			// or
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
		}
		LOG_OPCODE();
	}
	void BG(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			const XmmLink& v1 = XmmAlloc(rt);
			c.movdqa(v1.get(), XmmConst(_mm_set1_epi32(1)));
			XmmFinalize(v1, rt);
		}
		else
		{
			// compare if-greater-than
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& vb = XmmGet(rb);
			c.psubd(va.get(), XmmConst(_mm_set1_epi32(0x80000000)));
			c.psubd(vb.get(), XmmConst(_mm_set1_epi32(0x80000000)));
			c.pcmpgtd(va.get(), vb.get());
			c.paddd(va.get(), XmmConst(_mm_set1_epi32(1)));
			XmmFinalize(va, rt);
			XmmFinalize(vb);
			// sign bits:
			// a b (b-a) -> (result of BG)
			// 0 0 0 -> 1
			// 0 0 1 -> 0
			// 0 1 0 -> 1
			// 0 1 1 -> 1
			// 1 0 0 -> 0
			// 1 0 1 -> 0
			// 1 1 0 -> 0
			// 1 1 1 -> 1
		}
		LOG_OPCODE();
	}
	void SFH(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
		{
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
		}
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
		c.pxor(va.get(), XmmConst(_mm_set1_epi32(-1)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void ABSDB(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
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
		}
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
#ifdef _M_X64
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
#else
		WRAPPER_BEGIN(rt, ra, rb, zz);
		CPU.GPR[rt]._i32[0] = ((0 - CPU.GPR[rb]._u32[0]) & 0x3f) < 32 ? CPU.GPR[ra]._i32[0] >> ((0 - CPU.GPR[rb]._u32[0]) & 0x3f) : CPU.GPR[ra]._i32[0] >> 31;
		CPU.GPR[rt]._i32[1] = ((0 - CPU.GPR[rb]._u32[1]) & 0x3f) < 32 ? CPU.GPR[ra]._i32[1] >> ((0 - CPU.GPR[rb]._u32[1]) & 0x3f) : CPU.GPR[ra]._i32[1] >> 31;
		CPU.GPR[rt]._i32[2] = ((0 - CPU.GPR[rb]._u32[2]) & 0x3f) < 32 ? CPU.GPR[ra]._i32[2] >> ((0 - CPU.GPR[rb]._u32[2]) & 0x3f) : CPU.GPR[ra]._i32[2] >> 31;
		CPU.GPR[rt]._i32[3] = ((0 - CPU.GPR[rb]._u32[3]) & 0x3f) < 32 ? CPU.GPR[ra]._i32[3] >> ((0 - CPU.GPR[rb]._u32[3]) & 0x3f) : CPU.GPR[ra]._i32[3] >> 31;
		WRAPPER_END(rt, ra, rb, 0);
#endif
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
	void ROTH(u32 rt, u32 ra, u32 rb)
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
		const int s = i7 & 0x1f;
		if (s == 0)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& v1 = XmmCopy(va);
			c.pslld(va.get(), s);
			c.psrld(v1.get(), 32 - s);
			c.por(va.get(), v1.get());
			XmmFinalize(va, rt);
			XmmFinalize(v1);
		}
		LOG_OPCODE();
	}
	void ROTMI(u32 rt, u32 ra, s32 i7)
	{
		const int s = (0 - i7) & 0x3f;
		if (s > 31)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else if (s == 0)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			// shift right logical
			const XmmLink& va = XmmGet(ra, rt);
			c.psrld(va.get(), s);
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void ROTMAI(u32 rt, u32 ra, s32 i7)
	{
		const int s = (0 - i7) & 0x3f;
		if (s == 0)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			// shift right arithmetical
			const XmmLink& va = XmmGet(ra, rt);
			c.psrad(va.get(), s);
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void SHLI(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0x3f;
		if (s > 31)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else if (s == 0)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			// shift left
			const XmmLink& va = XmmGet(ra, rt);
			c.pslld(va.get(), s);
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void ROTHI(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0xf;
		if (s == 0)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& v1 = XmmCopy(va);
			c.psllw(va.get(), s);
			c.psrlw(v1.get(), 16 - s);
			c.por(va.get(), v1.get());
			XmmFinalize(va, rt);
			XmmFinalize(v1);
		}
		LOG_OPCODE();
	}
	void ROTHMI(u32 rt, u32 ra, s32 i7)
	{
		const int s = (0 - i7) & 0x1f;
		if (s > 15)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else if (s == 0)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			// shift right logical
			const XmmLink& va = XmmGet(ra, rt);
			c.psrlw(va.get(), s);
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void ROTMAHI(u32 rt, u32 ra, s32 i7)
	{
		const int s = (0 - i7) & 0x1f;
		if (s == 0)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			// shift right arithmetical
			const XmmLink& va = XmmGet(ra, rt);
			c.psraw(va.get(), s);
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void SHLHI(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0x1f;
		if (s > 15)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else if (s == 0)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			// shift left
			const XmmLink& va = XmmGet(ra, rt);
			c.psllw(va.get(), s);
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void A(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			const XmmLink& vb = XmmGet(rb, rt);
			c.paddd(vb.get(), vb.get());
			XmmFinalize(vb, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void AND(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
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
		}
		LOG_OPCODE();
	}
	void CG(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			const XmmLink& va = XmmGet(ra, rt);
			c.psrld(va.get(), 31);
			XmmFinalize(va, rt);
		}
		else
		{
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& vb = XmmGet(rb);
			c.paddd(vb.get(), va.get());
			c.psubd(va.get(), XmmConst(_mm_set1_epi32(0x80000000)));
			c.psubd(vb.get(), XmmConst(_mm_set1_epi32(0x80000000)));
			c.pcmpgtd(va.get(), vb.get());
			c.psrld(va.get(), 31);
			XmmFinalize(va, rt);
			XmmFinalize(vb);
		}
		LOG_OPCODE();
	}
	void AH(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			const XmmLink& va = XmmGet(ra, rt);
			c.paddw(va.get(), va.get());
			XmmFinalize(va, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void NAND(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// not
			const XmmLink& va = XmmGet(ra, rt);
			c.pxor(va.get(), XmmConst(_mm_set1_epi32(-1)));
			XmmFinalize(va, rt);
		}
		else
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
			c.pxor(va.get(), XmmConst(_mm_set1_epi32(-1)));
			XmmFinalize(va, rt);
		}
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
		UNIMPLEMENTED();
		if(sa != 0)
		{
			CPU.SPR[sa]._u128.hi = CPU.GPR[rt]._u128.hi;
			CPU.SPR[sa]._u128.lo = CPU.GPR[rt]._u128.lo;
		}
	}
	void WRCH(u32 ra, u32 rt)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		WRAPPER_BEGIN(ra, rt, yy, zz);
		CPU.WriteChannel(ra, CPU.GPR[rt]);
		WRAPPER_END(ra, rt, 0, 0);
		// TODO

		/*XmmInvalidate(rt);
		
		GpVar v(c, kVarTypeUInt32);
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
			X86X64CallNode* call = c.call(imm_ptr(&WRCH_wrapper::WRCH), kFuncConvHost, FuncBuilder2<FnVoid, u32, u32>());
			call->setArg(0, imm_u(ra));
			call->setArg(1, v);
		}
		}*/
	}
	void BIZ(u32 rt, u32 ra)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.mov(*addr, (u32)CPU.PC + 4);
		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		c.cmp(cpu_dword(GPR[rt]._u32[3]), 0);
		c.cmovne(*pos_var, *addr);
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void BINZ(u32 rt, u32 ra)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.mov(*addr, (u32)CPU.PC + 4);
		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		c.cmp(cpu_dword(GPR[rt]._u32[3]), 0);
		c.cmove(*pos_var, *addr);
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void BIHZ(u32 rt, u32 ra)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.mov(*addr, (u32)CPU.PC + 4);
		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		c.cmp(cpu_word(GPR[rt]._u16[6]), 0);
		c.cmovne(*pos_var, *addr);
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void BIHNZ(u32 rt, u32 ra)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.mov(*addr, (u32)CPU.PC + 4);
		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		c.cmp(cpu_word(GPR[rt]._u16[6]), 0);
		c.cmove(*pos_var, *addr);
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void STOPD(u32 rc, u32 ra, u32 rb)
	{
		UNIMPLEMENTED();
		Emu.Pause();
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
		c.mov(*qw0, cpu_qword(GPR[rt]._u64[0]));
		c.mov(*qw1, cpu_qword(GPR[rt]._u64[1]));
		c.bswap(*qw0);
		c.bswap(*qw1);
		c.mov(qword_ptr(*ls_var, *addr, 0, 0), *qw1);
		c.mov(qword_ptr(*ls_var, *addr, 0, 8), *qw0);
		LOG_OPCODE();
	}
	void BI(u32 ra)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void BISL(u32 rt, u32 ra)
	{
		XmmInvalidate(rt);

		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.xor_(*pos_var, *pos_var);
		c.mov(cpu_dword(GPR[rt]._u32[0]), *pos_var);
		c.mov(cpu_dword(GPR[rt]._u32[1]), *pos_var);
		c.mov(cpu_dword(GPR[rt]._u32[2]), *pos_var);
		c.mov(*pos_var, cpu_dword(GPR[ra]._u32[3]));
		c.mov(cpu_dword(GPR[rt]._u32[3]), (u32)CPU.PC + 4);
		c.shr(*pos_var, 2);
		LOG_OPCODE();
	}
	void IRET(u32 ra)
	{
		UNIMPLEMENTED();
		//SetBranch(SRR0);
	}
	void BISLED(u32 rt, u32 ra)
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
		c.pand(va.get(), XmmConst(_mm_set1_epi32(1)));
		c.pmullw(va.get(), XmmConst(_mm_set_epi32(8, 4, 2, 1)));
		c.phaddd(va.get(), va.get());
		c.phaddd(va.get(), va.get());
		c.pand(va.get(), XmmConst(_mm_set_epi32(0xffffffff, 0, 0, 0)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void GBH(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.pand(va.get(), XmmConst(_mm_set1_epi16(1)));
		c.pmullw(va.get(), XmmConst(_mm_set_epi16(128, 64, 32, 16, 8, 4, 2, 1)));
		c.phaddw(va.get(), va.get());
		c.phaddw(va.get(), va.get());
		c.phaddw(va.get(), va.get());
		c.pand(va.get(), XmmConst(_mm_set_epi32(0xffff, 0, 0, 0)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void GBB(u32 rt, u32 ra)
	{
		const XmmLink& va = XmmGet(ra, rt);
		//c.pand(va.get(), XmmConst(_mm_set1_epi8(1))); // ???
		c.pslld(va.get(), 7);
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
		c.movdqa(vr.get(), g_imm2_xmm(fsm_table[0], *addr));
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void FSMH(u32 rt, u32 ra)
	{
		const XmmLink& vr = XmmAlloc(rt);
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.and_(*addr, 0xff);
		c.shl(*addr, 4);
		c.movdqa(vr.get(), g_imm2_xmm(fsmh_table[0], *addr));
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void FSMB(u32 rt, u32 ra)
	{
		const XmmLink& vr = XmmAlloc(rt);
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.and_(*addr, 0xffff);
		c.shl(*addr, 4);
		c.movdqa(vr.get(), g_imm2_xmm(fsmb_table[0], *addr));
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
		c.andps(va.get(), XmmConst(_mm_set1_epi32(0x7fffffff))); // abs
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
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		if (ra == rb)
		{
			c.add(*addr, *addr);
		}
		else
		{
			c.add(*addr, cpu_dword(GPR[rb]._u32[3]));
		}
		c.and_(*addr, 0xf);
		c.neg(*addr);
		c.add(*addr, 0xf);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(byte_ptr(*cpu_var, *addr, 0, offsetof(SPUThread, GPR[rt]._u8[0])), 0x03);
		LOG_OPCODE();
	}
	void CHX(u32 rt, u32 ra, u32 rb)
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
		c.and_(*addr, 0xe);
		c.neg(*addr);
		c.add(*addr, 0xe);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(word_ptr(*cpu_var, *addr, 0, offsetof(SPUThread, GPR[rt]._u16[0])), 0x0203);
		LOG_OPCODE();
	}
	void CWX(u32 rt, u32 ra, u32 rb)
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
		c.and_(*addr, 0xc);
		c.neg(*addr);
		c.add(*addr, 0xc);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(dword_ptr(*cpu_var, *addr, 0, offsetof(SPUThread, GPR[rt]._u32[0])), 0x00010203);
		LOG_OPCODE();
	}
	void CDX(u32 rt, u32 ra, u32 rb)
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
		c.and_(*addr, 0x8);
		c.neg(*addr);
		c.add(*addr, 0x8);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(dword_ptr(*cpu_var, *addr, 0, offsetof(SPUThread, GPR[rt]._u32[0])), 0x00010203);
		c.mov(dword_ptr(*cpu_var, *addr, 0, offsetof(SPUThread, GPR[rt]._u32[1])), 0x04050607);
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
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.add(*addr, i7);
		c.and_(*addr, 0xf);
		c.neg(*addr);
		c.add(*addr, 0xf);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(byte_ptr(*cpu_var, *addr, 0, offsetof(SPUThread, GPR[rt]._u8[0])), 0x03);
		LOG_OPCODE();
	}
	void CHD(u32 rt, u32 ra, s32 i7)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.add(*addr, i7);
		c.and_(*addr, 0xe);
		c.neg(*addr);
		c.add(*addr, 0xe);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(word_ptr(*cpu_var, *addr, 0, offsetof(SPUThread, GPR[rt]._u16[0])), 0x0203);
		LOG_OPCODE();
	}
	void CWD(u32 rt, u32 ra, s32 i7)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.add(*addr, i7);
		c.and_(*addr, 0xc);
		c.neg(*addr);
		c.add(*addr, 0xc);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(dword_ptr(*cpu_var, *addr, 0, offsetof(SPUThread, GPR[rt]._u32[0])), 0x00010203);
		LOG_OPCODE();
	}
	void CDD(u32 rt, u32 ra, s32 i7)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.add(*addr, i7);
		c.and_(*addr, 0x8);
		c.neg(*addr);
		c.add(*addr, 0x8);
		const XmmLink& vr = XmmAlloc(rt);
		c.movdqa(vr.get(), XmmConst(_mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f)));
		XmmFinalize(vr, rt);
		XmmInvalidate(rt);
		c.mov(dword_ptr(*cpu_var, *addr, 0, offsetof(SPUThread, GPR[rt]._u32[0])), 0x00010203);
		c.mov(dword_ptr(*cpu_var, *addr, 0, offsetof(SPUThread, GPR[rt]._u32[1])), 0x04050607);
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
		if (s == 0)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& v1 = XmmCopy(va);
			c.pslldq(va.get(), s);
			c.psrldq(v1.get(), 16 - s);
			c.por(va.get(), v1.get());
			XmmFinalize(va, rt);
			XmmFinalize(v1);
		}
		LOG_OPCODE();
	}
	void ROTQMBYI(u32 rt, u32 ra, s32 i7)
	{
		const int s = (0 - i7) & 0x1f;
		if (s == 0)
		{
			if (ra != rt)
			{
				// mov
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else if (s > 15)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
		{
			// shift right
			const XmmLink& va = XmmGet(ra, rt);
			c.psrldq(va.get(), s);
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void SHLQBYI(u32 rt, u32 ra, s32 i7)
	{
		const int s = i7 & 0x1f;
		if (s == 0)
		{
			if (ra != rt)
			{
				// mov
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else if (s > 15)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
		{
			// shift left
			const XmmLink& va = XmmGet(ra, rt);
			c.pslldq(va.get(), s);
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void NOP(u32 rt)
	{
		LOG_OPCODE();
	}
	void CGT(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void XOR(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void CGTH(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void EQV(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			const XmmLink& v1 = XmmAlloc(rt);
			c.pcmpeqd(v1.get(), v1.get());
			XmmFinalize(v1, rt);
		}
		else
		{
			const XmmLink& vb = XmmGet(rb, rt);
			c.pxor(vb.get(), XmmConst(_mm_set1_epi32(-1)));
			if (const XmmLink* va = XmmRead(ra))
			{
				c.pxor(vb.get(), va->read());
			}
			else
			{
				c.pxor(vb.get(), cpu_xmm(GPR[ra]));
			}
			XmmFinalize(vb, rt);
		}
		LOG_OPCODE();
	}
	void CGTB(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void SUMB(u32 rt, u32 ra, u32 rb)
	{
		/*WRAPPER_BEGIN(rt, ra, rb, zz);
		const SPU_GPR_hdr _a = CPU.GPR[ra];
		const SPU_GPR_hdr _b = CPU.GPR[rb];
		for (int w = 0; w < 4; w++)
		{
			CPU.GPR[rt]._u16[w*2] = _a._u8[w*4] + _a._u8[w*4 + 1] + _a._u8[w*4 + 2] + _a._u8[w*4 + 3];
			CPU.GPR[rt]._u16[w*2 + 1] = _b._u8[w*4] + _b._u8[w*4 + 1] + _b._u8[w*4 + 2] + _b._u8[w*4 + 3];
		}
		WRAPPER_END(rt, ra, rb, 0);*/

		const XmmLink& va = XmmGet(ra);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		const XmmLink& v1 = XmmCopy(vb, rt);
		const XmmLink& v2 = XmmCopy(vb);
		const XmmLink& vFF = XmmAlloc();
		c.movdqa(vFF.get(), XmmConst(_mm_set1_epi32(0xff)));
		c.pand(v1.get(), vFF.get());
		c.psrld(v2.get(), 8);
		c.pand(v2.get(), vFF.get());
		c.paddd(v1.get(), v2.get());
		c.movdqa(v2.get(), vb.get());
		c.psrld(v2.get(), 16);
		c.pand(v2.get(), vFF.get());
		c.paddd(v1.get(), v2.get());
		c.movdqa(v2.get(), vb.get());
		c.psrld(v2.get(), 24);
		c.paddd(v1.get(), v2.get());
		c.pslld(v1.get(), 16);
		c.movdqa(v2.get(), va.get());
		c.pand(v2.get(), vFF.get());
		c.por(v1.get(), v2.get());
		c.movdqa(v2.get(), va.get());
		c.psrld(v2.get(), 8);
		c.pand(v2.get(), vFF.get());
		c.paddd(v1.get(), v2.get());
		c.movdqa(v2.get(), va.get());
		c.psrld(v2.get(), 16);
		c.pand(v2.get(), vFF.get());
		c.paddd(v1.get(), v2.get());
		c.movdqa(v2.get(), va.get());
		c.psrld(v2.get(), 24);
		c.paddd(v1.get(), v2.get());
		XmmFinalize(vb);
		XmmFinalize(va);
		XmmFinalize(v1, rt);
		XmmFinalize(v2);
		XmmFinalize(vFF);
		LOG_OPCODE();
	}
	//HGT uses signed values.  HLGT uses unsigned values
	void HGT(u32 rt, s32 ra, s32 rb)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._i32[3]));
		c.cmp(*addr, cpu_dword(GPR[rb]._i32[3]));
		c.mov(*addr, 0);
		c.setg(*addr);
		c.neg(*addr);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.xor_(*pos_var, *addr);
		do_finalize = true;
		LOG_OPCODE();
	}
	void CLZ(u32 rt, u32 ra)
	{
		XmmInvalidate(rt);
		for (u32 i = 0; i < 4; i++)
		{
			c.bsr(*addr, cpu_dword(GPR[ra]._u32[i]));
			c.cmovz(*addr, dword_ptr(*g_imm_var, offsetof(g_imm_table_struct, fsmb_table[0xffff]))); // load 0xffffffff
			c.neg(*addr);
			c.add(*addr, 31);
			c.mov(cpu_dword(GPR[rt]._u32[i]), *addr);
		}
		LOG_OPCODE();
	}
	void XSWD(u32 rt, u32 ra)
	{
#ifdef _M_X64
		c.movsxd(*qw0, cpu_dword(GPR[ra]._i32[0]));
		c.movsxd(*qw1, cpu_dword(GPR[ra]._i32[2]));
		c.mov(cpu_qword(GPR[rt]._i64[0]), *qw0);
		c.mov(cpu_qword(GPR[rt]._i64[1]), *qw1);
		XmmInvalidate(rt);
		LOG_OPCODE();
#else
		WRAPPER_BEGIN(rt, ra, yy, zz);
		CPU.GPR[rt]._i64[0] = (s64)CPU.GPR[ra]._i32[0];
		CPU.GPR[rt]._i64[1] = (s64)CPU.GPR[ra]._i32[2];
		WRAPPER_END(rt, ra, 0, 0);
#endif
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
		/*XmmInvalidate(rt);
		for (u32 i = 0; i < 8; i++)
		{
			c.movzx(*addr, cpu_word(GPR[ra]._u16[i]));
			c.movzx(*addr, word_ptr(*g_imm_var, *addr, 1, offsetof(g_imm_table_struct, cntb_table[0])));
			c.mov(cpu_word(GPR[rt]._u16[i]), addr->r16());
		}*/
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& v1 = XmmCopy(va);
		const XmmLink& vm = XmmAlloc();
		c.psrlw(v1.get(), 4);
		c.pand(va.get(), XmmConst(_mm_set1_epi8(0xf)));
		c.pand(v1.get(), XmmConst(_mm_set1_epi8(0xf)));
		c.movdqa(vm.get(), XmmConst(_mm_set_epi8(4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0)));
		c.pshufb(vm.get(), va.get());
		c.movdqa(va.get(), XmmConst(_mm_set_epi8(4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0)));
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
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
		{
			// compare if-greater-than
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& vb = XmmGet(rb);
			c.psubd(va.get(), XmmConst(_mm_set1_epi32(0x80000000)));
			c.psubd(vb.get(), XmmConst(_mm_set1_epi32(0x80000000)));
			c.pcmpgtd(va.get(), vb.get());
			XmmFinalize(va, rt);
			XmmFinalize(vb);
		}
		LOG_OPCODE();
	}
	void ANDC(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
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
		}
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
		// reverted less-than
		const XmmLink& vb = XmmGet(rb, rt);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.cmppd(vb.get(), va->read(), 1);
		}
		else
		{
			c.cmppd(vb.get(), cpu_xmm(GPR[ra]), 1);
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void FA(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (ra == rb)
		{
			c.addps(va.get(), va.get());
		}
		else
		{
			if (const XmmLink* vb = XmmRead(rb))
			{
				c.addps(va.get(), vb->read());
			}
			else
			{
				c.addps(va.get(), cpu_xmm(GPR[rb]));
			}
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void FS(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero (?)
			const XmmLink& v0 = XmmAlloc(rt);
			c.subps(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void FM(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			const XmmLink& va = XmmGet(ra, rt);
			c.mulps(va.get(), va.get());
			XmmFinalize(va, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void CLGTH(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
		{
			// compare if-greater-than
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& vb = XmmGet(rb);
			c.psubw(va.get(), XmmConst(_mm_set1_epi32(0x80008000)));
			c.psubw(vb.get(), XmmConst(_mm_set1_epi32(0x80008000)));
			c.pcmpgtw(va.get(), vb.get());
			XmmFinalize(va, rt);
			XmmFinalize(vb);
		}
		LOG_OPCODE();
	}
	void ORC(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			const XmmLink& v1 = XmmAlloc(rt);
			c.pcmpeqd(v1.get(), v1.get());
			XmmFinalize(v1, rt);
		}
		else
		{
			const XmmLink& vb = XmmGet(rb, rt);
			c.pxor(vb.get(), XmmConst(_mm_set1_epi32(-1)));
			if (const XmmLink* va = XmmRead(ra))
			{
				c.por(vb.get(), va->read());
			}
			else
			{
				c.por(vb.get(), cpu_xmm(GPR[ra]));
			}
			XmmFinalize(vb, rt);
		}
		LOG_OPCODE();
	}
	void FCMGT(u32 rt, u32 ra, u32 rb)
	{
		// reverted less-than
		const XmmLink& vb = XmmGet(rb, rt);
		const XmmLink& va = XmmGet(ra);
		c.andps(vb.get(), XmmConst(_mm_set1_epi32(0x7fffffff))); // abs
		c.andps(va.get(), XmmConst(_mm_set1_epi32(0x7fffffff))); // abs
		c.cmpps(vb.get(), va.get(), 1);
		XmmFinalize(vb, rt);
		XmmFinalize(va);
		LOG_OPCODE();
	}
	void DFCMGT(u32 rt, u32 ra, u32 rb)
	{
		// reverted less-than
		const XmmLink& vb = XmmGet(rb, rt);
		const XmmLink& va = XmmGet(ra);
		c.andpd(vb.get(), XmmConst(_mm_set_epi32(0x7fffffff, 0xffffffff, 0x7fffffff, 0xffffffff))); // abs
		c.andpd(va.get(), XmmConst(_mm_set_epi32(0x7fffffff, 0xffffffff, 0x7fffffff, 0xffffffff))); // abs
		c.cmppd(vb.get(), va.get(), 1);
		XmmFinalize(vb, rt);
		XmmFinalize(va);
		LOG_OPCODE();
	}
	void DFA(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (ra == rb)
		{
			c.addpd(va.get(), va.get());
		}
		else
		{
			if (const XmmLink* vb = XmmRead(rb))
			{
				c.addpd(va.get(), vb->read());
			}
			else
			{
				c.addpd(va.get(), cpu_xmm(GPR[rb]));
			}
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void DFS(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero (?)
			const XmmLink& v0 = XmmAlloc(rt);
			c.subpd(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void DFM(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			const XmmLink& va = XmmGet(ra, rt);
			c.mulpd(va.get(), va.get());
			XmmFinalize(va, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void CLGTB(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
		{
			// compare if-greater-than
			const XmmLink& va = XmmGet(ra, rt);
			const XmmLink& vb = XmmGet(rb);
			c.psubb(va.get(), XmmConst(_mm_set1_epi32(0x80808080)));
			c.psubb(vb.get(), XmmConst(_mm_set1_epi32(0x80808080)));
			c.pcmpgtb(va.get(), vb.get());
			XmmFinalize(va, rt);
			XmmFinalize(vb);
		}
		LOG_OPCODE();
	}
	void HLGT(u32 rt, u32 ra, u32 rb)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.cmp(*addr, cpu_dword(GPR[rb]._u32[3]));
		c.mov(*addr, 0);
		c.seta(*addr);
		c.neg(*addr);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.xor_(*pos_var, *addr);
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
		const XmmLink& vr = XmmGet(rt, rt);
		const XmmLink& va = XmmGet(ra);
		c.mulpd(va.get(), cpu_xmm(GPR[rb]));
		c.xorpd(vr.get(), XmmConst(_mm_set_epi32(0x80000000, 0, 0x80000000, 0))); // neg
		c.addpd(vr.get(), va.get());
		XmmFinalize(vr, rt);
		XmmFinalize(va);
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
		const XmmLink& vr = XmmGet(rt, rt);
		const XmmLink& va = XmmGet(ra);
		c.mulpd(va.get(), cpu_xmm(GPR[rb]));
		c.addpd(vr.get(), va.get());
		c.xorpd(vr.get(), XmmConst(_mm_set_epi32(0x80000000, 0, 0x80000000, 0))); // neg
		XmmFinalize(vr, rt);
		XmmFinalize(va);
		LOG_OPCODE();
	}
	void CEQ(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			const XmmLink& v1 = XmmAlloc(rt);
			c.pcmpeqd(v1.get(), v1.get());
			XmmFinalize(v1, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void MPYHHU(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		c.psrld(va.get(), 16);
		c.psrld(vb.get(), 16);
		c.pmulld(va.get(), vb.get());
		XmmFinalize(va, rt);
		XmmFinalize(vb);
		LOG_OPCODE();
	}
	void ADDX(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vt = XmmGet(rt);
		c.pand(vt.get(), XmmConst(_mm_set1_epi32(1)));
		c.paddd(vt.get(), cpu_xmm(GPR[ra]));
		c.paddd(vt.get(), cpu_xmm(GPR[rb]));
		XmmFinalize(vt, rt);
		LOG_OPCODE();
	}
	void SFX(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vt = XmmGet(rt);
		if (ra == rb)
		{
			// load zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pandn(vt.get(), XmmConst(_mm_set1_epi32(1)));
			c.pxor(v0.get(), v0.get());
			c.psubd(v0.get(), vt.get());
			XmmFinalize(v0, rt);
		}
		else
		{
			// sub
			const XmmLink& vb = XmmGet(rb, rt);
			c.pandn(vt.get(), XmmConst(_mm_set1_epi32(1)));
			c.psubd(vb.get(), cpu_xmm(GPR[ra]));
			c.psubd(vb.get(), vt.get());
			XmmFinalize(vb, rt);
		}
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
		c.psrad(va.get(), 16);
		c.psrad(vb.get(), 16);
		c.pmulld(va.get(), vb.get());
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
		c.psrld(va.get(), 16);
		c.psrld(vb.get(), 16);
		c.pmulld(va.get(), vb.get());
		c.paddd(vt.get(), va.get());
		XmmFinalize(vt, rt);
		XmmFinalize(va);
		XmmFinalize(vb);
		LOG_OPCODE();
	}
	void FSCRRD(u32 rt)
	{
		UNIMPLEMENTED();
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
		UNIMPLEMENTED();
	}
	void DFTSV(u32 rt, u32 ra, s32 i7) //nf
	{
		WRAPPER_BEGIN(rt, ra, i7, zz);
		const u64 DoubleExpMask = 0x7ff0000000000000;
		const u64 DoubleFracMask = 0x000fffffffffffff;
		const u64 DoubleSignMask = 0x8000000000000000;
		const SPU_GPR_hdr temp = CPU.GPR[ra];
		CPU.GPR[rt].Reset();
		if (i7 & 1) //Negative Denorm Check (-, exp is zero, frac is non-zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] & DoubleFracMask)
					if ((temp._u64[i] & (DoubleSignMask | DoubleExpMask)) == DoubleSignMask)
						CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 2) //Positive Denorm Check (+, exp is zero, frac is non-zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] & DoubleFracMask)
					if ((temp._u64[i] & (DoubleSignMask | DoubleExpMask)) == 0)
						CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 4) //Negative Zero Check (-, exp is zero, frac is zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] == DoubleSignMask)
					CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 8) //Positive Zero Check (+, exp is zero, frac is zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] == 0)
					CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 16) //Negative Infinity Check (-, exp is 0x7ff, frac is zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] == (DoubleSignMask | DoubleExpMask))
					CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 32) //Positive Infinity Check (+, exp is 0x7ff, frac is zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] == DoubleExpMask)
					CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		if (i7 & 64) //Not-a-Number Check (any sign, exp is 0x7ff, frac is non-zero)
			for (int i = 0; i < 2; i++)
			{
				if (temp._u64[i] & DoubleFracMask)
					if ((temp._u64[i] & DoubleExpMask) == DoubleExpMask)
						CPU.GPR[rt]._u64[i] = 0xffffffffffffffff;
			}
		WRAPPER_END(rt, ra, i7, 0);
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
		// compare equal
		const XmmLink& vb = XmmGet(rb, rt);
		if (const XmmLink* va = XmmRead(ra))
		{
			c.cmppd(vb.get(), va->read(), 0);
		}
		else
		{
			c.cmppd(vb.get(), cpu_xmm(GPR[ra]), 0);
		}
		XmmFinalize(vb, rt);
		LOG_OPCODE();
	}
	void MPY(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& vb = (ra == rb) ? XmmCopy(va) : XmmGet(rb);
		c.pslld(va.get(), 16);
		c.pslld(vb.get(), 16);
		c.psrad(va.get(), 16);
		c.psrad(vb.get(), 16);
		c.pmulld(va.get(), vb.get());
		XmmFinalize(va, rt);
		XmmFinalize(vb);
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
		c.psrad(va.get(), 16);
		c.psrad(vb.get(), 16);
		c.pmulld(va.get(), vb.get());
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
		if (ra == rb)
		{
			const XmmLink& v1 = XmmAlloc(rt);
			c.pcmpeqw(v1.get(), v1.get());
			XmmFinalize(v1, rt);
		}
		else
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
		}
		LOG_OPCODE();
	}
	void FCMEQ(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vb = XmmGet(rb, rt);
		const XmmLink& va = XmmGet(ra);
		c.andps(vb.get(), XmmConst(_mm_set1_epi32(0x7fffffff))); // abs
		c.andps(va.get(), XmmConst(_mm_set1_epi32(0x7fffffff))); // abs
		c.cmpps(vb.get(), va.get(), 0); // ==
		XmmFinalize(vb, rt);
		XmmFinalize(va);
		LOG_OPCODE();
	}
	void DFCMEQ(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& vb = XmmGet(rb, rt);
		const XmmLink& va = XmmGet(ra);
		c.andpd(vb.get(), XmmConst(_mm_set_epi32(0x7fffffff, 0xffffffff, 0x7fffffff, 0xffffffff))); // abs
		c.andpd(va.get(), XmmConst(_mm_set_epi32(0x7fffffff, 0xffffffff, 0x7fffffff, 0xffffffff))); // abs
		c.cmppd(vb.get(), va.get(), 0); // ==
		XmmFinalize(vb, rt);
		XmmFinalize(va);
		LOG_OPCODE();
	}
	void MPYU(u32 rt, u32 ra, u32 rb)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (ra == rb)
		{
			c.pslld(va.get(), 16);
			c.psrld(va.get(), 16);
			c.pmulld(va.get(), va.get());
		}
		else
		{
			const XmmLink& v1 = XmmAlloc();
			c.movdqa(v1.get(), XmmConst(_mm_set1_epi32(0xffff))); // load mask
			c.pand(va.get(), v1.get()); // clear high words of each dword
			c.pand(v1.get(), cpu_xmm(GPR[rb]));
			c.pmulld(va.get(), v1.get());
			XmmFinalize(v1);
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CEQB(u32 rt, u32 ra, u32 rb)
	{
		if (ra == rb)
		{
			const XmmLink& v1 = XmmAlloc(rt);
			c.pcmpeqb(v1.get(), v1.get());
			XmmFinalize(v1, rt);
		}
		else
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
		}
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
		c.mov(*addr, cpu_dword(GPR[ra]._i32[3]));
		c.cmp(*addr, cpu_dword(GPR[rb]._i32[3]));
		c.mov(*addr, 0);
		c.sete(*addr);
		c.neg(*addr);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.xor_(*pos_var, *addr);
		do_finalize = true;
		LOG_OPCODE();
	}

	//0 - 9
	void CFLTS(u32 rt, u32 ra, s32 i8)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (i8 != 173)
		{
			c.mulps(va.get(), XmmConst(_mm_set1_ps(pow(2, 173 - (i8 & 0xff))))); // scale
		}
		c.maxps(va.get(), XmmConst(_mm_set1_ps(-pow(2, 31)))); // saturate
		c.minps(va.get(), XmmConst(_mm_set1_ps((float)0x7fffffff)));
		c.cvttps2dq(va.get(), va.get()); // convert to ints with truncation
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CFLTU(u32 rt, u32 ra, s32 i8)
	{
		const XmmLink& va = XmmGet(ra, rt);
		if (i8 != 173)
		{
			c.mulps(va.get(), XmmConst(_mm_set1_ps(pow(2, 173 - (i8 & 0xff))))); // scale
		}
		c.maxps(va.get(), XmmConst(_mm_set1_ps(0.0f))); // saturate
		c.minps(va.get(), XmmConst(_mm_set1_ps((float)0xffffffff)));
		const XmmLink& v1 = XmmCopy(va);
		c.cmpps(v1.get(), XmmConst(_mm_set1_ps(pow(2, 31))), 5); // generate mask of big values
		c.andps(v1.get(), XmmConst(_mm_set1_ps(pow(2, 32)))); // generate correction component
		c.subps(va.get(), v1.get()); // subtract correction component
		c.cvttps2dq(va.get(), va.get()); // convert to ints with truncation
		XmmFinalize(va, rt);
		XmmFinalize(v1);
		LOG_OPCODE();
	}
	void CSFLT(u32 rt, u32 ra, s32 i8)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.cvtdq2ps(va.get(), va.get()); // convert to floats
		if (i8 != 155)
		{
			c.mulps(va.get(), XmmConst(_mm_set1_ps(pow(2, (i8 & 0xff) - 155)))); // scale
		}
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CUFLT(u32 rt, u32 ra, s32 i8)
	{
		const XmmLink& va = XmmGet(ra, rt);
		const XmmLink& v1 = XmmCopy(va);
		c.cvtdq2ps(va.get(), va.get()); // convert to floats
		c.psrad(v1.get(), 32); // generate mask from sign bit
		c.andps(v1.get(), XmmConst(_mm_set1_ps(pow(2, 32)))); // generate correction component
		c.addps(va.get(), v1.get()); // add correction component
		if (i8 != 155)
		{
			c.mulps(va.get(), XmmConst(_mm_set1_ps(pow(2, (i8 & 0xff) - 155)))); // scale
		}
		XmmFinalize(va, rt);
		XmmFinalize(v1);
		LOG_OPCODE();
	}

	//0 - 8
	void BRZ(u32 rt, s32 i16)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
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
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.mov(*addr, (CPU.PC >> 2) + 1);
		c.mov(*pos_var, branchTarget(CPU.PC, i16) >> 2);
		c.cmp(cpu_dword(GPR[rt]._u32[3]), 0);
		c.cmove(*pos_var, *addr);
		LOG_OPCODE();
	}
	void BRHZ(u32 rt, s32 i16)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.mov(*addr, (CPU.PC >> 2) + 1);
		c.mov(*pos_var, branchTarget(CPU.PC, i16) >> 2);
		c.cmp(cpu_word(GPR[rt]._u16[6]), 0);
		c.cmovnz(*pos_var, *addr);
		LOG_OPCODE();
	}
	void BRHNZ(u32 rt, s32 i16)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
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
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.mov(*pos_var, branchTarget(0, i16) >> 2);
		LOG_OPCODE();
	}
	void LQA(u32 rt, s32 i16)
	{
		XmmInvalidate(rt);

		const u32 lsa = (i16 << 2) & 0x3fff0;
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

		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.xor_(*addr, *addr); // zero
		c.mov(cpu_dword(GPR[rt]._u32[0]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[1]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[2]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[3]), (u32)CPU.PC + 4);
		c.mov(*pos_var, branchTarget(0, i16) >> 2);
		LOG_OPCODE();
	}
	void BR(s32 i16)
	{
		c.mov(cpu_qword(PC), (u32)CPU.PC);
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
			c.movdqa(vr.get(), g_imm_xmm(fsmb_table[i16 & 0xffff]));
			XmmFinalize(vr, rt);
		}
		LOG_OPCODE();
	}
	void BRSL(u32 rt, s32 i16)
	{
		XmmInvalidate(rt);

		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;

		c.xor_(*addr, *addr); // zero
		c.mov(cpu_dword(GPR[rt]._u32[0]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[1]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[2]), *addr);
		c.mov(cpu_dword(GPR[rt]._u32[3]), (u32)CPU.PC + 4);
		c.mov(*pos_var, branchTarget(CPU.PC, i16) >> 2);
		LOG_OPCODE();
	}
	void LQR(u32 rt, s32 i16)
	{
		XmmInvalidate(rt);

		const u32 lsa = branchTarget(CPU.PC, i16) & 0x3fff0;
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
			c.movdqa(vr.get(), XmmConst(_mm_set1_epi32(i16)));
		}
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void ILHU(u32 rt, s32 i16)
	{
		const XmmLink& vr = XmmAlloc(rt);
		if (i16 == 0)
		{
			c.pxor(vr.get(), vr.get());
		}
		else
		{
			c.movdqa(vr.get(), XmmConst(_mm_set1_epi32(i16 << 16)));
		}
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void ILH(u32 rt, s32 i16)
	{
		const XmmLink& vr = XmmAlloc(rt);
		if (i16 == 0)
		{
			c.pxor(vr.get(), vr.get());
		}
		else
		{
			c.movdqa(vr.get(), XmmConst(_mm_set1_epi16(i16)));
		}
		XmmFinalize(vr, rt);
		LOG_OPCODE();
	}
	void IOHL(u32 rt, s32 i16)
	{
		if (i16 == 0)
		{
			// nop
		}
		else
		{
			const XmmLink& vt = XmmGet(rt, rt);
			c.por(vt.get(), XmmConst(_mm_set1_epi32(i16 & 0xffff)));
			XmmFinalize(vt, rt);
		}
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
			c.por(va.get(), XmmConst(_mm_set1_epi32(i10)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void ORHI(u32 rt, u32 ra, s32 i10)
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
			c.por(va.get(), XmmConst(_mm_set1_epi16(i10)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void ORBI(u32 rt, u32 ra, s32 i10)
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
			c.por(va.get(), XmmConst(_mm_set1_epi8(i10)));
			XmmFinalize(va, rt);
		}
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
			c.movdqa(vr.get(), XmmConst(_mm_set1_epi32(i10)));
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
			c.movdqa(vr.get(), XmmConst(_mm_set1_epi16(i10)));
			c.psubw(vr.get(), cpu_xmm(GPR[ra]));
			XmmFinalize(vr, rt);
		}
		LOG_OPCODE();
	}
	void ANDI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == 0)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else if (i10 == -1)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			const XmmLink& va = XmmGet(ra, rt);
			c.pand(va.get(), XmmConst(_mm_set1_epi32(i10)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void ANDHI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == 0)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else if (i10 == -1)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			const XmmLink& va = XmmGet(ra, rt);
			c.pand(va.get(), XmmConst(_mm_set1_epi16(i10)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void ANDBI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == 0)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else if (i10 == -1)
		{
			// mov
			if (ra != rt)
			{
				const XmmLink& va = XmmGet(ra, rt);
				XmmFinalize(va, rt);
			}
			// else nop
		}
		else
		{
			const XmmLink& va = XmmGet(ra, rt);
			c.pand(va.get(), XmmConst(_mm_set1_epi8(i10)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void AI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == 0)
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
			// add
			const XmmLink& va = XmmGet(ra, rt);
			c.paddd(va.get(), XmmConst(_mm_set1_epi32(i10)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void AHI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == 0)
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
			// add
			const XmmLink& va = XmmGet(ra, rt);
			c.paddw(va.get(), XmmConst(_mm_set1_epi16(i10)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void STQD(u32 rt, s32 i10, u32 ra) // i10 is shifted left by 4 while decoding
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		if (i10) c.add(*addr, i10);
		c.and_(*addr, 0x3fff0);
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
		c.pxor(va.get(), XmmConst(_mm_set1_epi32(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void XORHI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pxor(va.get(), XmmConst(_mm_set1_epi16(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void XORBI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pxor(va.get(), XmmConst(_mm_set1_epi8(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CGTI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpgtd(va.get(), XmmConst(_mm_set1_epi32(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CGTHI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpgtw(va.get(), XmmConst(_mm_set1_epi16(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CGTBI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpgtb(va.get(), XmmConst(_mm_set1_epi8(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void HGTI(u32 rt, u32 ra, s32 i10)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._i32[3]));
		c.cmp(*addr, i10);
		c.mov(*addr, 0);
		c.setg(*addr);
		c.neg(*addr);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.xor_(*pos_var, *addr);
		do_finalize = true;
		LOG_OPCODE();
	}
	void CLGTI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == -1)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
		{
			const XmmLink& va = XmmGet(ra);
			c.psubd(va.get(), XmmConst(_mm_set1_epi32(0x80000000)));
			c.pcmpgtd(va.get(), XmmConst(_mm_set1_epi32((u32)i10 - 0x80000000)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void CLGTHI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == -1)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
		{
			const XmmLink& va = XmmGet(ra);
			c.psubw(va.get(), XmmConst(_mm_set1_epi16((u16)0x8000)));
			c.pcmpgtw(va.get(), XmmConst(_mm_set1_epi16((u16)i10 - 0x8000)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void CLGTBI(u32 rt, u32 ra, s32 i10)
	{
		if (i10 == -1)
		{
			// zero
			const XmmLink& v0 = XmmAlloc(rt);
			c.pxor(v0.get(), v0.get());
			XmmFinalize(v0, rt);
		}
		else
		{
			const XmmLink& va = XmmGet(ra);
			c.psubb(va.get(), XmmConst(_mm_set1_epi8((s8)0x80)));
			c.pcmpgtb(va.get(), XmmConst(_mm_set1_epi8((s8)i10 - 0x80)));
			XmmFinalize(va, rt);
		}
		LOG_OPCODE();
	}
	void HLGTI(u32 rt, u32 ra, s32 i10)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.cmp(*addr, i10);
		c.mov(*addr, 0);
		c.seta(*addr);
		c.neg(*addr);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.xor_(*pos_var, *addr);
		do_finalize = true;
		LOG_OPCODE();
	}
	void MPYI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.pslld(va.get(), 16);
		c.psrad(va.get(), 16);
		c.pmulld(va.get(), XmmConst(_mm_set1_epi32(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void MPYUI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra, rt);
		c.pslld(va.get(), 16);
		c.psrld(va.get(), 16);
		c.pmulld(va.get(), XmmConst(_mm_set1_epi32(i10 & 0xffff)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CEQI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpeqd(va.get(), XmmConst(_mm_set1_epi32(i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CEQHI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpeqw(va.get(), XmmConst(_mm_set1_epi16((s16)i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void CEQBI(u32 rt, u32 ra, s32 i10)
	{
		const XmmLink& va = XmmGet(ra);
		c.pcmpeqb(va.get(), XmmConst(_mm_set1_epi8((s8)i10)));
		XmmFinalize(va, rt);
		LOG_OPCODE();
	}
	void HEQI(u32 rt, u32 ra, s32 i10)
	{
		c.mov(*addr, cpu_dword(GPR[ra]._u32[3]));
		c.cmp(*addr, i10);
		c.mov(*addr, 0);
		c.sete(*addr);
		c.neg(*addr);
		c.mov(*pos_var, (CPU.PC >> 2) + 1);
		c.xor_(*pos_var, *addr);
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
			c.movdqa(vr.get(), XmmConst(_mm_set1_epi32(i18 & 0x3ffff)));
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
		/*WRAPPER_BEGIN(rc, rt, ra, rb);
		const SPU_GPR_hdr _a = CPU.GPR[ra];
		const SPU_GPR_hdr _b = CPU.GPR[rb];
		for (int i = 0; i < 16; i++)
		{
			u8 b = CPU.GPR[rc]._u8[i];
			if (b & 0x80)
			{
				if (b & 0x40)
				{
					if (b & 0x20)
						CPU.GPR[rt]._u8[i] = 0x80;
					else
						CPU.GPR[rt]._u8[i] = 0xFF;
				}
				else
					CPU.GPR[rt]._u8[i] = 0x00;
			}
			else
			{
				if (b & 0x10)
					CPU.GPR[rt]._u8[i] = _b._u8[15 - (b & 0x0F)];
				else
					CPU.GPR[rt]._u8[i] = _a._u8[15 - (b & 0x0F)];
			}
		}
		WRAPPER_END(rc, rt, ra, rb);*/

		const XmmLink& v0 = XmmGet(rc); // v0 = mask
		const XmmLink& v1 = XmmAlloc();
		const XmmLink& v2 = XmmCopy(v0); // v2 = mask
		const XmmLink& v3 = XmmAlloc();
		const XmmLink& v4 = XmmAlloc();
		const XmmLink& vFF = XmmAlloc(rt);
		// generate specific values:
		c.movdqa(v1.get(), XmmConst(_mm_set1_epi32(0xe0e0e0e0))); // v1 = 11100000
		c.movdqa(v3.get(), XmmConst(_mm_set1_epi32(0x80808080))); // v3 = 10000000
		c.pand(v2.get(), v1.get()); // filter mask      v2 = mask & 11100000
		c.movdqa(vFF.get(), v2.get()); // and copy      vFF = mask & 11100000
		c.movdqa(v4.get(), XmmConst(_mm_set1_epi32(0xc0c0c0c0))); // v4 = 11000000
		c.pcmpeqb(vFF.get(), v4.get()); // gen 0xff     vFF = (mask & 11100000 == 11000000) ? 0xff : 0
		c.movdqa(v4.get(), v2.get()); // copy again     v4 = mask & 11100000
		c.pand(v4.get(), v3.get()); // filter mask      v4 = mask & 10000000
		c.pcmpeqb(v2.get(), v1.get()); //               v2 = (mask & 11100000 == 11100000) ? 0xff : 0
		c.pcmpeqb(v4.get(), v3.get()); //               v4 = (mask & 10000000 == 10000000) ? 0xff : 0
		c.pand(v2.get(), v3.get()); // generate 0x80    v2 = (mask & 11100000 == 11100000) ? 0x80 : 0
		c.por(vFF.get(), v2.get()); // merge 0xff, 0x80 vFF = (mask & 11100000 == 11000000) ? 0xff : (mask & 11100000 == 11100000) ? 0x80 : 0
		c.pandn(v1.get(), v0.get()); // filter mask     v1 = mask & 00011111
		// select bytes from [rb]:
		c.movdqa(v2.get(), XmmConst(_mm_set1_epi8(15))); //   v2 = 00001111
		c.pxor(v1.get(), XmmConst(_mm_set1_epi8(0x10))); //   v1 = (mask & 00011111) ^ 00010000
		c.psubb(v2.get(), v1.get()); //                 v2 = 00001111 - ((mask & 00011111) ^ 00010000)
		c.movdqa(v1.get(), cpu_xmm(GPR[rb])); //        v1 = rb
		c.pshufb(v1.get(), v2.get()); //                v1 = select(rb, 00001111 - ((mask & 00011111) ^ 00010000))
		// select bytes from [ra]:
		c.pxor(v2.get(), XmmConst(_mm_set1_epi32(0xf0f0f0f0))); //   v2 = (00001111 - ((mask & 00011111) ^ 00010000)) ^ 11110000
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
		c.pslld(va.get(), 16);
		c.pslld(vb.get(), 16);
		c.psrad(va.get(), 16);
		c.psrad(vb.get(), 16);
		c.pmulld(va.get(), vb.get());
		c.paddd(va.get(), cpu_xmm(GPR[rc]));
		XmmFinalize(va, rt);
		XmmFinalize(vb);
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
			throw (std::string(__FUNCTION__) + std::string("(): invalid case")).c_str();
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
			throw (std::string(__FUNCTION__) + std::string("(): invalid case")).c_str();
		}
		LOG_OPCODE();
	}

	void UNK(u32 code, u32 opcode, u32 gcode)
	{
		UNK(fmt::Format("(SPURecompiler) Unimplemented opcode! (0x%08x, 0x%x, 0x%x)", code, opcode, gcode));
	}

	void UNK(const std::string& err)
	{
		ConLog.Error(err + fmt::Format(" #pc: 0x%x", CPU.PC));
		c.mov(cpu_qword(PC), (u32)CPU.PC);
		do_finalize = true;
		Emu.Pause();
	}

	
};

#undef c
