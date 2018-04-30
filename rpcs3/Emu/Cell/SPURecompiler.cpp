#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/Memory.h"

#include "SPUThread.h"
#include "SPUAnalyser.h"
#include "SPUInterpreter.h"
#include "SPUDisAsm.h"
#include "SPURecompiler.h"
#include <algorithm>
#include <mutex>
#include <thread>

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
	verify(HERE), spu.jit->compile(block(spu, spu.pc, &spu.jit->m_block_info));
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

std::vector<u32> spu_recompiler_base::block(SPUThread& spu, u32 lsa, std::bitset<0x10000>* out_info)
{
	// Block info (local)
	std::bitset<0x10000> block_info{};

	// Select one to use
	std::bitset<0x10000>& blocks = out_info ? *out_info : block_info;

	if (out_info)
	{
		out_info->reset();
	}

	// Result: addr + raw instruction data
	std::vector<u32> result;
	result.reserve(256);
	result.push_back(lsa);
	blocks.set(lsa / 4);

	// Simple block entry workload list
	std::vector<u32> wl;
	wl.push_back(lsa);

	// Value flags (TODO)
	enum class vf : u32
	{
		is_const,
		is_mask,

		__bitset_enum_max
	};

	// Weak constant propagation context (for guessing branch targets)
	std::array<bs_t<vf>, 128> vflags{};

	// Associated constant values for 32-bit preferred slot
	std::array<u32, 128> values;

	if (spu.pc == lsa && g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		// TODO: use current register values for speculations
		vflags[0] = +vf::is_const;
		values[0] = spu.gpr[0]._u32[3];
	}

	for (u32 wi = 0; wi < wl.size();)
	{
		const auto next_block = [&]
		{
			// Reset value information
			vflags.fill({});
			wi++;
		};

		const auto add_block = [&](u32 target)
		{
			// Verify validity of the new target (TODO)
			if (target > lsa)
			{
				// Check for redundancy
				if (!blocks[target / 4])
				{
					blocks[target / 4] = true;
					wl.push_back(target);
					return;
				}
			}
		};

		const u32 pos = wl[wi];
		const u32 data = spu._ref<u32>(pos);
		const auto op = spu_opcode_t{data};

		wl[wi] += 4;

		// Analyse instruction
		switch (const auto type = s_spu_itype.decode(data))
		{
		case spu_itype::UNK:
		case spu_itype::DFCEQ:
		case spu_itype::DFCMEQ:
		case spu_itype::DFCGT:
		//case spu_itype::DFCMGT:
		case spu_itype::DFTSV:
		{
			// Stop on invalid instructions (TODO)
			blocks[pos / 4] = true;
			next_block();
			continue;
		}

		case spu_itype::SYNC:
		case spu_itype::DSYNC:
		case spu_itype::STOP:
		case spu_itype::STOPD:
		{
			if (data == 0)
			{
				// Stop before null data
				blocks[pos / 4] = true;
				next_block();
				continue;
			}

			if (g_cfg.core.spu_block_size != spu_block_size_type::giga)
			{
				// Stop on special instructions (TODO)
				next_block();
				break;
			}

			break;
		}

		case spu_itype::IRET:
		{
			next_block();
			break;
		}

		case spu_itype::BI:
		case spu_itype::BISL:
		case spu_itype::BIZ:
		case spu_itype::BINZ:
		case spu_itype::BIHZ:
		case spu_itype::BIHNZ:
		{
			const auto af = vflags[op.ra];
			const auto av = values[op.ra];

			if (type == spu_itype::BISL)
			{
				vflags[op.rt] = +vf::is_const;
				values[op.rt] = pos + 4;
			}

			if (test(af, vf::is_const))
			{
				const u32 target = spu_branch_target(av);

				if (target == pos + 4)
				{
					// Nop (unless BISL)
					break;
				}

				if (type != spu_itype::BISL || g_cfg.core.spu_block_size == spu_block_size_type::giga)
				{
					// TODO
					if (g_cfg.core.spu_block_size != spu_block_size_type::safe)
					{
						add_block(target);
					}
				}

				if (type == spu_itype::BISL && target < lsa)
				{
					next_block();
					break;
				}
			}
			else if (type == spu_itype::BI && !op.d && !op.e)
			{
				// Analyse jump table (TODO)
				std::basic_string<u32> jt_abs;
				std::basic_string<u32> jt_rel;
				const u32 start = pos + 4;
				const u32 limit = 0x40000;

				for (u32 i = start; i < limit; i += 4)
				{
					const u32 target = spu._ref<u32>(i);

					if (target % 4)
					{
						// Address cannot be misaligned: abort
						break;
					}

					if (target >= lsa && target < limit)
					{
						// Possible jump table entry (absolute)
						jt_abs.push_back(target);
					}

					if (target + start >= lsa && target + start < limit)
					{
						// Possible jump table entry (relative)
						jt_rel.push_back(target + start);
					}

					if (std::max(jt_abs.size(), jt_rel.size()) * 4 + start <= i)
					{
						// Neither type of jump table completes
						break;
					}
				}

				// Add detected jump table blocks (TODO: avoid adding both)
				if (jt_abs.size() >= 3 || jt_rel.size() >= 3)
				{
					if (jt_abs.size() >= jt_rel.size())
					{
						for (u32 target : jt_abs)
						{
							add_block(target);
						}
					}

					if (jt_rel.size() >= jt_abs.size())
					{
						for (u32 target : jt_rel)
						{
							add_block(target);
						}
					}
				}
			}

			if (type == spu_itype::BI || type == spu_itype::BISL || g_cfg.core.spu_block_size == spu_block_size_type::safe)
			{
				if (type == spu_itype::BI || g_cfg.core.spu_block_size != spu_block_size_type::giga)
				{
					next_block();
					break;
				}
			}

			break;
		}

		case spu_itype::BRSL:
		case spu_itype::BRASL:
		{
			const u32 target = spu_branch_target(type == spu_itype::BRASL ? 0 : pos, op.i16);

			vflags[op.rt] = +vf::is_const;
			values[op.rt] = pos + 4;

			if (target == pos + 4)
			{
				// Get next instruction address idiom
				break;
			}

			if (target < lsa || g_cfg.core.spu_block_size != spu_block_size_type::giga)
			{
				// Stop on direct calls
				next_block();
				break;
			}

			if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
			{
				add_block(target);
			}

			break;
		}

		case spu_itype::BR:
		case spu_itype::BRA:
		case spu_itype::BRZ:
		case spu_itype::BRNZ:
		case spu_itype::BRHZ:
		case spu_itype::BRHNZ:
		{
			const u32 target = spu_branch_target(type == spu_itype::BRA ? 0 : pos, op.i16);

			if (target == pos + 4)
			{
				// Nop
				break;
			}

			add_block(target);

			if (type == spu_itype::BR || type == spu_itype::BRA)
			{
				// Stop on direct branches
				next_block();
				break;
			}

			break;
		}

		case spu_itype::HEQ:
		case spu_itype::HEQI:
		case spu_itype::HGT:
		case spu_itype::HGTI:
		case spu_itype::HLGT:
		case spu_itype::HLGTI:
		case spu_itype::HBR:
		case spu_itype::HBRA:
		case spu_itype::HBRR:
		case spu_itype::LNOP:
		case spu_itype::NOP:
		case spu_itype::MTSPR:
		case spu_itype::WRCH:
		case spu_itype::FSCRWR:
		case spu_itype::STQA:
		case spu_itype::STQD:
		case spu_itype::STQR:
		case spu_itype::STQX:
		{
			// Do nothing
			break;
		}

		case spu_itype::IL:
		{
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.si16;
			break;
		}
		case spu_itype::ILA:
		{
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i18;
			break;
		}
		case spu_itype::ILH:
		{
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i16 << 16 | op.i16;
			break;
		}
		case spu_itype::ILHU:
		{
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i16 << 16;
			break;
		}
		case spu_itype::IOHL:
		{
			values[op.rt] = values[op.rt] | op.i16;
			break;
		}
		case spu_itype::ORI:
		{
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] | op.si10;
			break;
		}
		case spu_itype::OR:
		{
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] | values[op.rb];
			break;
		}
		case spu_itype::AI:
		{
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] + op.si10;
			break;
		}
		case spu_itype::A:
		{
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] + values[op.rb];
			break;
		}
		default:
		{
			// Unconst
			vflags[type & spu_itype::_quadrop ? +op.rt4 : +op.rt] = {};
			break;
		}
		}

		// Insert raw instruction value
		if (result.size() - 1 <= (pos - lsa) / 4)
		{
			if (result.size() - 1 < (pos - lsa) / 4)
			{
				result.resize((pos - lsa) / 4 + 1);
			}

			result.emplace_back(se_storage<u32>::swap(data));
		}
		else if (u32& raw_val = result[(pos - lsa) / 4 + 1])
		{
			verify(HERE), raw_val == se_storage<u32>::swap(data);
		}
		else
		{
			raw_val = se_storage<u32>::swap(data);
		}
	}

	if (g_cfg.core.spu_block_size == spu_block_size_type::safe)
	{
		// Check holes in safe mode (TODO)
		u32 valid_size = 0;

		for (u32 i = 1; i < result.size(); i++)
		{
			if (result[i] == 0)
			{
				const u32 pos = lsa + (i - 1) * 4;
				const u32 data = spu._ref<u32>(pos);
				const auto type = s_spu_itype.decode(data);

				// Allow only NOP or LNOP instructions in holes
				if (type == spu_itype::NOP || type == spu_itype::LNOP)
				{
					if (i + 1 < result.size())
					{
						continue;
					}
				}

				result.resize(valid_size + 1);
				break;
			}
			else
			{
				valid_size = i;
			}
		}
	}

	if (result.size() == 1)
	{
		// Blocks starting from 0x0 or invalid instruction won't be compiled, may need special interpreter fallback
		result.clear();
	}

	return result;
}
