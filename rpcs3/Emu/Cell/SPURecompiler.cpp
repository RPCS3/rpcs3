#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/Memory.h"

#include "SPUThread.h"
#include "SPUAnalyser.h"
#include "SPURecompiler.h"
#include <algorithm>

extern u64 get_system_time();

const spu_decoder<spu_itype> s_spu_itype;

spu_recompiler_base::spu_recompiler_base(SPUThread& spu)
	: m_spu(spu)
{
	// Initialize lookup table
	spu.jit_dispatcher.fill(&dispatch);
}

spu_recompiler_base::~spu_recompiler_base()
{
}

void spu_recompiler_base::dispatch(SPUThread& spu, void*, u8* rip)
{
	// If check failed after direct branch, patch it with single NOP
	if (rip)
	{
#ifdef _MSC_VER
		*(volatile u64*)(rip) = 0x841f0f;
#else
		__atomic_store_n(reinterpret_cast<u64*>(rip), 0x841f0f, __ATOMIC_RELAXED);
#endif
	}

	const auto func = spu.jit->get(spu.pc);

	// First attempt (load new trampoline and retry)
	if (func != spu.jit_dispatcher[spu.pc / 4])
	{
		spu.jit_dispatcher[spu.pc / 4] = func;
		return;
	}

	// Second attempt (recover from the recursion after repeated unsuccessful trampoline call)
	if (spu.block_counter != spu.block_recover && func != &dispatch)
	{
		spu.block_recover = spu.block_counter;
		return;
	}

	// Compile
	verify(HERE), spu.jit->compile(block(spu, spu.pc));
	spu.jit_dispatcher[spu.pc / 4] = spu.jit->get(spu.pc);
}

void spu_recompiler_base::branch(SPUThread& spu, void*, u8* rip)
{
	const auto pair = *reinterpret_cast<std::pair<const std::vector<u32>, spu_function_t>**>(rip + 24);

	spu.pc = pair->first[0];

	const auto func = pair->second ? pair->second : spu.jit->compile(pair->first);

	verify(HERE), func, pair->second == func;

	// Overwrite function address
	reinterpret_cast<atomic_t<spu_function_t>*>(rip + 32)->store(func);

	// Overwrite jump to this function with jump to the compiled function
	const s64 rel = reinterpret_cast<u64>(func) - reinterpret_cast<u64>(rip) - 5;

	alignas(8) u8 bytes[8];

	if (rel >= INT32_MIN && rel <= INT32_MAX)
	{
		const s64 rel8 = (rel + 5) - 2;

		if (rel8 >= INT8_MIN && rel8 <= INT8_MAX)
		{
			bytes[0] = 0xeb; // jmp rel8
			bytes[1] = static_cast<s8>(rel8);
			std::memset(bytes + 2, 0x90, 6);
		}
		else
		{
			bytes[0] = 0xe9; // jmp rel32
			std::memcpy(bytes + 1, &rel, 4);
			std::memset(bytes + 5, 0x90, 3);
		}
	}
	else
	{
		bytes[0] = 0xff; // jmp [rip+26]
		bytes[1] = 0x25;
		bytes[2] = 0x1a;
		bytes[3] = 0x00;
		bytes[4] = 0x00;
		bytes[5] = 0x00;
		bytes[6] = 0x90;
		bytes[7] = 0x90;
	}

#ifdef _MSC_VER
	*(volatile u64*)(rip) = *reinterpret_cast<u64*>(+bytes);
#else
	__atomic_store_n(reinterpret_cast<u64*>(rip), *reinterpret_cast<u64*>(+bytes), __ATOMIC_RELAXED);
#endif
}

std::vector<u32> spu_recompiler_base::block(SPUThread& spu, u32 lsa)
{
	u32 addr = lsa;

	std::vector<u32> result;

	while (addr < 0x40000)
	{
		const u32 data = spu._ref<u32>(addr);

		if (data == 0 && addr == lsa)
		{
			break;
		}

		addr += 4;

		if (result.empty())
		{
			result.emplace_back(lsa);
		}

		result.emplace_back(se_storage<u32>::swap(data));

		const auto type = s_spu_itype.decode(data);

		switch (type)
		{
		case spu_itype::UNK:
		case spu_itype::STOP:
		case spu_itype::STOPD:
		case spu_itype::SYNC:
		case spu_itype::DSYNC:
		case spu_itype::DFCEQ:
		case spu_itype::DFCMEQ:
		case spu_itype::DFCGT:
		//case spu_itype::DFCMGT:
		case spu_itype::DFTSV:
		case spu_itype::BI:
		case spu_itype::IRET:
		case spu_itype::BISL:
		{
			break;
		}
		case spu_itype::BRA:
		case spu_itype::BRASL:
		{
			if (spu_branch_target(0, spu_opcode_t{data}.i16) == addr)
			{
				continue;
			}

			break;
		}
		case spu_itype::BR:
		case spu_itype::BRSL:
		{
			if (spu_branch_target(addr - 4, spu_opcode_t{data}.i16) == addr)
			{
				continue;
			}

			break;
		}
		case spu_itype::BRZ:
		case spu_itype::BRNZ:
		case spu_itype::BRHZ:
		case spu_itype::BRHNZ:
		{
			if (spu_branch_target(addr - 4, spu_opcode_t{data}.i16) >= addr)
			{
				continue;
			}

			break;
		}
		default:
		{
			continue;
		}
		}

		break;
	}

	return result;
}
