#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/Memory.h"
#include "Crypto/sha1.h"
#include "Utilities/StrUtil.h"

#include "SPUThread.h"
#include "SPUAnalyser.h"
#include "SPUInterpreter.h"
#include "SPUDisAsm.h"
#include "SPURecompiler.h"
#include "PPUAnalyser.h"
#include <algorithm>
#include <mutex>
#include <thread>

extern atomic_t<const char*> g_progr;
extern atomic_t<u64> g_progr_ptotal;
extern atomic_t<u64> g_progr_pdone;

const spu_decoder<spu_itype> s_spu_itype;
const spu_decoder<spu_iname> s_spu_iname;

extern u64 get_timebased_time();

spu_cache::spu_cache(const std::string& loc)
	: m_file(loc, fs::read + fs::write + fs::create)
{
}

spu_cache::~spu_cache()
{
}

std::vector<std::vector<u32>> spu_cache::get()
{
	std::vector<std::vector<u32>> result;

	if (!m_file)
	{
		return result;
	}

	m_file.seek(0);

	// TODO: signal truncated or otherwise broken file
	while (true)
	{
		be_t<u32> size;
		be_t<u32> addr;
		std::vector<u32> func;

		if (!m_file.read(size) || !m_file.read(addr))
		{
			break;
		}

		func.resize(size + 1);
		func[0] = addr;

		if (m_file.read(func.data() + 1, func.size() * 4 - 4) != func.size() * 4 - 4)
		{
			break;
		}

		result.emplace_back(std::move(func));
	}

	return result;
}

void spu_cache::add(const std::vector<u32>& func)
{
	if (!m_file)
	{
		return;
	}

	be_t<u32> size = ::size32(func) - 1;
	be_t<u32> addr = func[0];
	m_file.write(size);
	m_file.write(addr);
	m_file.write(func.data() + 1, func.size() * 4 - 4);
}

void spu_cache::initialize()
{
	const auto _main = fxm::get<ppu_module>();

	if (!_main || !g_cfg.core.spu_shared_runtime)
	{
		return;
	}

	// SPU cache file (version + block size type)
	const std::string loc = _main->cache + "spu-" + fmt::to_lower(g_cfg.core.spu_block_size.to_string()) + "-v5.dat";

	auto cache = std::make_shared<spu_cache>(loc);

	if (!*cache)
	{
		LOG_ERROR(SPU, "Failed to initialize SPU cache at: %s", loc);
		return;
	}

	// Read cache
	auto func_list = cache->get();

	// Recompiler instance for cache initialization
	std::unique_ptr<spu_recompiler_base> compiler;

	if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit)
	{
		compiler = spu_recompiler_base::make_asmjit_recompiler();
	}

	if (g_cfg.core.spu_decoder == spu_decoder_type::llvm)
	{
		compiler = spu_recompiler_base::make_llvm_recompiler();
	}

	if (compiler)
	{
		compiler->init();
	}

	if (compiler && !func_list.empty())
	{
		// Fake LS
		std::vector<be_t<u32>> ls(0x10000);

		// Initialize progress dialog (wait for previous progress done)
		while (g_progr_ptotal)
		{
			std::this_thread::sleep_for(5ms);
		}

		g_progr = "Building SPU cache...";
		g_progr_ptotal += func_list.size();

		// Build functions
		for (auto&& func : func_list)
		{
			if (Emu.IsStopped())
			{
				g_progr_pdone++;
				continue;
			}

			// Get data start
			const u32 start = func[0] * (g_cfg.core.spu_block_size != spu_block_size_type::giga);
			const u32 size0 = ::size32(func);

			// Initialize LS with function data only
			for (u32 i = 1, pos = start; i < size0; i++, pos += 4)
			{
				ls[pos / 4] = se_storage<u32>::swap(func[i]);
			}

			// Call analyser
			std::vector<u32> func2 = compiler->block(ls.data(), func[0]);

			if (func2.size() != size0)
			{
				LOG_ERROR(SPU, "[0x%05x] SPU Analyser failed, %u vs %u", func2[0], func2.size() - 1, size0 - 1);
			}

			compiler->compile(std::move(func));

			// Clear fake LS
			for (u32 i = 1, pos = start; i < func2.size(); i++, pos += 4)
			{
				if (se_storage<u32>::swap(func2[i]) != ls[pos / 4])
				{
					LOG_ERROR(SPU, "[0x%05x] SPU Analyser failed at 0x%x", func2[0], pos);
				}

				ls[pos / 4] = 0;
			}

			if (func2.size() != size0)
			{
				std::memset(ls.data(), 0, 0x40000);
			}

			g_progr_pdone++;
		}

		if (Emu.IsStopped())
		{
			LOG_ERROR(SPU, "SPU Runtime: Cache building aborted.");
			return;
		}

		LOG_SUCCESS(SPU, "SPU Runtime: Built %u functions.", func_list.size());
	}

	// Register cache instance
	fxm::import<spu_cache>([&]() -> std::shared_ptr<spu_cache>&&
	{
		return std::move(cache);
	});
}

spu_recompiler_base::spu_recompiler_base()
{
}

spu_recompiler_base::~spu_recompiler_base()
{
}

void spu_recompiler_base::dispatch(SPUThread& spu, void*, u8* rip)
{
	// If code verification failed from a patched patchpoint, clear it with a single NOP
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
	verify(HERE), spu.jit->compile(spu.jit->block(spu._ptr<u32>(0), spu.pc));
	spu.jit_dispatcher[spu.pc / 4] = spu.jit->get(spu.pc);

	// Diagnostic
	if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		const v128 _info = spu.stack_mirror[(spu.gpr[1]._u32[3] & 0x3fff0) >> 4];

		if (_info._u64[0] != -1)
		{
			LOG_TRACE(SPU, "Called from 0x%x", _info._u32[2] - 4);
		}
	}
}

void spu_recompiler_base::branch(SPUThread& spu, void*, u8* rip)
{
	// Compile (TODO: optimize search of the existing functions)
	const auto func = verify(HERE, spu.jit->compile(spu.jit->block(spu._ptr<u32>(0), spu.pc)));
	spu.jit_dispatcher[spu.pc / 4] = spu.jit->get(spu.pc);

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
		// Far jumps: extremely rare and disabled due to implementation complexity
		bytes[0] = 0x0f; // nop (8-byte form)
		bytes[1] = 0x1f;
		bytes[2] = 0x84;
		std::memset(bytes + 3, 0x00, 5);
	}

#ifdef _MSC_VER
	*(volatile u64*)(rip) = *reinterpret_cast<u64*>(+bytes);
#else
	__atomic_store_n(reinterpret_cast<u64*>(rip), *reinterpret_cast<u64*>(+bytes), __ATOMIC_RELAXED);
#endif
}

std::vector<u32> spu_recompiler_base::block(const be_t<u32>* ls, u32 entry_point)
{
	// Result: addr + raw instruction data
	std::vector<u32> result;
	result.reserve(256);
	result.push_back(entry_point);

	// Initialize block entries
	m_block_info.reset();
	m_block_info.set(entry_point / 4);
	m_entry_info.reset();
	m_entry_info.set(entry_point / 4);

	// Simple block entry workload list
	std::vector<u32> workload;
	workload.push_back(entry_point);

	std::memset(m_regmod.data(), 0xff, sizeof(m_regmod));
	m_targets.clear();
	m_preds.clear();
	m_preds[entry_point];

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

	// SYNC instruction found
	bool sync = false;

	u32 hbr_loc = 0;
	u32 hbr_tg = -1;

	// Result bounds
	u32 lsa = entry_point;
	u32 limit = 0x40000;

	if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		// In Giga mode, all data starts from the address 0
		lsa = 0;
	}

	for (u32 wi = 0, wa = workload[0]; wi < workload.size();)
	{
		const auto next_block = [&]
		{
			// Reset value information
			vflags.fill({});
			sync = false;
			hbr_loc = 0;
			hbr_tg = -1;
			wi++;

			if (wi < workload.size())
			{
				wa = workload[wi];
			}
		};

		const u32 pos = wa;

		const auto add_block = [&](u32 target)
		{
			// Validate new target (TODO)
			if (target > lsa && target < limit)
			{
				// Check for redundancy
				if (!m_block_info[target / 4])
				{
					m_block_info[target / 4] = true;
					workload.push_back(target);
				}

				// Add predecessor
				if (m_preds[target].find_first_of(pos) == -1)
				{
					m_preds[target].push_back(pos);
				}
			}
		};

		if (pos < lsa || pos >= limit)
		{
			// Don't analyse if already beyond the limit
			next_block();
			continue;
		}

		const u32 data = ls[pos / 4];
		const auto op = spu_opcode_t{data};

		wa += 4;

		m_targets.erase(pos);

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
			// Stop before invalid instructions (TODO)
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
				next_block();
				continue;
			}

			if (g_cfg.core.spu_block_size == spu_block_size_type::safe)
			{
				// Stop on special instructions (TODO)
				m_targets[pos];
				next_block();
				break;
			}

			if (type == spu_itype::SYNC)
			{
				// Remember
				sync = true;
			}

			break;
		}

		case spu_itype::IRET:
		{
			if (op.d && op.e)
			{
				LOG_ERROR(SPU, "[0x%x] Invalid interrupt flags (DE)", pos);
			}

			m_targets[pos];
			next_block();
			break;
		}

		case spu_itype::BI:
		case spu_itype::BISL:
		case spu_itype::BISLED:
		case spu_itype::BIZ:
		case spu_itype::BINZ:
		case spu_itype::BIHZ:
		case spu_itype::BIHNZ:
		{
			if (op.d && op.e)
			{
				LOG_ERROR(SPU, "[0x%x] Invalid interrupt flags (DE)", pos);
			}

			const auto af = vflags[op.ra];
			const auto av = values[op.ra];
			const bool sl = type == spu_itype::BISL || type == spu_itype::BISLED;

			if (sl)
			{
				m_regmod[pos / 4] = op.rt;
				vflags[op.rt] = +vf::is_const;
				values[op.rt] = pos + 4;

				if (op.rt == 1 && (pos + 4) % 16)
				{
					LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: BISL", pos);
				}
			}

			if (test(af, vf::is_const))
			{
				const u32 target = spu_branch_target(av);

				if (target == pos + 4)
				{
					LOG_WARNING(SPU, "[0x%x] At 0x%x: indirect branch to next%s", result[0], pos, op.d ? " (D)" : op.e ? " (E)" : "");
				}
				else
				{
					LOG_WARNING(SPU, "[0x%x] At 0x%x: indirect branch to 0x%x", result[0], pos, target);
				}

				m_targets[pos].push_back(target);

				if (!sl)
				{
					if (sync)
					{
						LOG_NOTICE(SPU, "[0x%x] At 0x%x: ignoring branch to 0x%x (SYNC)", result[0], pos, target);

						if (entry_point < target)
						{
							limit = std::min<u32>(limit, target);
						}
					}
					else
					{
						if (op.d || op.e)
						{
							m_entry_info[target / 4] = true;
						}

						add_block(target);
					}
				}

				if (sl && g_cfg.core.spu_block_size == spu_block_size_type::giga)
				{
					if (sync)
					{
						LOG_NOTICE(SPU, "[0x%x] At 0x%x: ignoring call to 0x%x (SYNC)", result[0], pos, target);

						if (target > entry_point)
						{
							limit = std::min<u32>(limit, target);
						}
					}
					else
					{
						m_entry_info[target / 4] = true;
						add_block(target);
					}
				}
				else if (sl && target > entry_point)
				{
					limit = std::min<u32>(limit, target);
				}

				if (sl && g_cfg.core.spu_block_size != spu_block_size_type::safe)
				{
					m_entry_info[pos / 4 + 1] = true;
					m_targets[pos].push_back(pos + 4);
					add_block(pos + 4);
				}
			}
			else if (type == spu_itype::BI && g_cfg.core.spu_block_size != spu_block_size_type::safe && !op.d && !op.e && !sync)
			{
				// Analyse jump table (TODO)
				std::basic_string<u32> jt_abs;
				std::basic_string<u32> jt_rel;
				const u32 start = pos + 4;
				u64 dabs = 0;
				u64 drel = 0;

				for (u32 i = start; i < limit; i += 4)
				{
					const u32 target = ls[i / 4];

					if (target == 0 || target % 4)
					{
						// Address cannot be misaligned: abort
						break;
					}

					if (target >= lsa && target < 0x40000)
					{
						// Possible jump table entry (absolute)
						jt_abs.push_back(target);
					}

					if (target + start >= lsa && target + start < 0x40000)
					{
						// Possible jump table entry (relative)
						jt_rel.push_back(target + start);
					}

					if (std::max(jt_abs.size(), jt_rel.size()) * 4 + start <= i)
					{
						// Neither type of jump table completes
						jt_abs.clear();
						jt_rel.clear();
						break;
					}
				}

				// Choose position after the jt as an anchor and compute the average distance
				for (u32 target : jt_abs)
				{
					dabs += std::abs(static_cast<s32>(target - start - jt_abs.size() * 4));
				}

				for (u32 target : jt_rel)
				{
					drel += std::abs(static_cast<s32>(target - start - jt_rel.size() * 4));
				}

				// Add detected jump table blocks
				if (jt_abs.size() >= 3 || jt_rel.size() >= 3)
				{
					if (jt_abs.size() == jt_rel.size())
					{
						if (dabs < drel)
						{
							jt_rel.clear();
						}

						if (dabs > drel)
						{
							jt_abs.clear();
						}

						verify(HERE), jt_abs.size() != jt_rel.size();
					}

					if (jt_abs.size() >= jt_rel.size())
					{
						const u32 new_size = (start - lsa) / 4 + 1 + jt_abs.size();

						if (result.size() < new_size)
						{
							result.resize(new_size);
						}

						for (u32 i = 0; i < jt_abs.size(); i++)
						{
							add_block(jt_abs[i]);
							result[(start - lsa) / 4 + 1 + i] = se_storage<u32>::swap(jt_abs[i]);
							m_targets[start + i * 4];
						}

						m_targets.emplace(pos, std::move(jt_abs));
					}

					if (jt_rel.size() >= jt_abs.size())
					{
						const u32 new_size = (start - lsa) / 4 + 1 + jt_rel.size();

						if (result.size() < new_size)
						{
							result.resize(new_size);
						}

						for (u32 i = 0; i < jt_rel.size(); i++)
						{
							add_block(jt_rel[i]);
							result[(start - lsa) / 4 + 1 + i] = se_storage<u32>::swap(jt_rel[i] - start);
							m_targets[start + i * 4];
						}

						m_targets.emplace(pos, std::move(jt_rel));
					}
				}
				else if (start + 12 * 4 < limit &&
					ls[start / 4 + 0] == 0x1ce00408 &&
					ls[start / 4 + 1] == 0x24000389 &&
					ls[start / 4 + 2] == 0x24004809 &&
					ls[start / 4 + 3] == 0x24008809 &&
					ls[start / 4 + 4] == 0x2400c809 &&
					ls[start / 4 + 5] == 0x24010809 &&
					ls[start / 4 + 6] == 0x24014809 &&
					ls[start / 4 + 7] == 0x24018809 &&
					ls[start / 4 + 8] == 0x1c200807 &&
					ls[start / 4 + 9] == 0x2401c809)
				{
					LOG_WARNING(SPU, "[0x%x] Pattern 1 detected (hbr=0x%x:0x%x)", pos, hbr_loc, hbr_tg);

					// Add 8 targets (TODO)
					for (u32 addr = start + 4; addr < start + 36; addr += 4)
					{
						m_targets[pos].push_back(addr);
						add_block(addr);
					}
				}
				else if (hbr_loc > start && hbr_loc < limit && hbr_tg == start)
				{
					LOG_WARNING(SPU, "[0x%x] No patterns detected (hbr=0x%x:0x%x)", pos, hbr_loc, hbr_tg);
				}
			}
			else if (type == spu_itype::BI && sync)
			{
				LOG_NOTICE(SPU, "[0x%x] At 0x%x: ignoring indirect branch (SYNC)", result[0], pos);
			}

			if (type == spu_itype::BI || sl)
			{
				if (type == spu_itype::BI || g_cfg.core.spu_block_size == spu_block_size_type::safe)
				{
					m_targets[pos];
				}
				else
				{
					m_entry_info[pos / 4 + 1] = true;
					m_targets[pos].push_back(pos + 4);
					add_block(pos + 4);
				}
			}
			else
			{
				m_targets[pos].push_back(pos + 4);
				add_block(pos + 4);
			}

			next_block();
			break;
		}

		case spu_itype::BRSL:
		case spu_itype::BRASL:
		{
			const u32 target = spu_branch_target(type == spu_itype::BRASL ? 0 : pos, op.i16);

			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = pos + 4;

			if (op.rt == 1 && (pos + 4) % 16)
			{
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: BRSL", pos);
			}

			if (target == pos + 4)
			{
				// Get next instruction address idiom
				break;
			}

			m_targets[pos].push_back(target);

			if (g_cfg.core.spu_block_size != spu_block_size_type::safe)
			{
				m_entry_info[pos / 4 + 1] = true;
				m_targets[pos].push_back(pos + 4);
				add_block(pos + 4);
			}

			if (g_cfg.core.spu_block_size == spu_block_size_type::giga && !sync)
			{
				m_entry_info[target / 4] = true;
				add_block(target);
			}
			else
			{
				if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
				{
					LOG_NOTICE(SPU, "[0x%x] At 0x%x: ignoring fixed call to 0x%x (SYNC)", result[0], pos, target);
				}

				if (target > entry_point)
				{
					limit = std::min<u32>(limit, target);
				}
			}

			next_block();
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

			m_targets[pos].push_back(target);
			add_block(target);

			if (type != spu_itype::BR && type != spu_itype::BRA)
			{
				m_targets[pos].push_back(pos + 4);
				add_block(pos + 4);
			}

			next_block();
			break;
		}

		case spu_itype::HEQ:
		case spu_itype::HEQI:
		case spu_itype::HGT:
		case spu_itype::HGTI:
		case spu_itype::HLGT:
		case spu_itype::HLGTI:
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

		case spu_itype::LQA:
		case spu_itype::LQD:
		case spu_itype::LQR:
		case spu_itype::LQX:
		{
			// Unconst
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = {};
			break;
		}

		case spu_itype::HBR:
		{
			hbr_loc = spu_branch_target(pos, op.roh << 7 | op.rt);
			hbr_tg  = test(vflags[op.ra], vf::is_const) && !op.c ? values[op.ra] & 0x3fffc : -1;
			break;
		}

		case spu_itype::HBRA:
		{
			hbr_loc = spu_branch_target(pos, op.r0h << 7 | op.rt);
			hbr_tg  = spu_branch_target(0x0, op.i16);
			break;
		}

		case spu_itype::HBRR:
		{
			hbr_loc = spu_branch_target(pos, op.r0h << 7 | op.rt);
			hbr_tg  = spu_branch_target(pos, op.i16);
			break;
		}

		case spu_itype::IL:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.si16;

			if (op.rt == 1 && values[1] & ~0x3fff0u)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: IL -> 0x%x", pos, values[1]);
			}

			break;
		}
		case spu_itype::ILA:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i18;

			if (op.rt == 1 && values[1] & ~0x3fff0u)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: ILA -> 0x%x", pos, values[1]);
			}

			break;
		}
		case spu_itype::ILH:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i16 << 16 | op.i16;

			if (op.rt == 1 && values[1] & ~0x3fff0u)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: ILH -> 0x%x", pos, values[1]);
			}

			break;
		}
		case spu_itype::ILHU:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i16 << 16;

			if (op.rt == 1 && values[1] & ~0x3fff0u)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: ILHU -> 0x%x", pos, values[1]);
			}

			break;
		}
		case spu_itype::IOHL:
		{
			m_regmod[pos / 4] = op.rt;
			values[op.rt] = values[op.rt] | op.i16;

			if (op.rt == 1 && op.i16 % 16)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: IOHL, 0x%x", pos, op.i16);
			}

			break;
		}
		case spu_itype::ORI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] | op.si10;

			if (op.rt == 1)
			{
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: ORI", pos);
			}

			break;
		}
		case spu_itype::OR:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] | values[op.rb];

			if (op.rt == 1)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: OR", pos);
			}

			break;
		}
		case spu_itype::ANDI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] & op.si10;

			if (op.rt == 1)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: ANDI", pos);
			}

			break;
		}
		case spu_itype::AND:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] & values[op.rb];

			if (op.rt == 1)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: AND", pos);
			}

			break;
		}
		case spu_itype::AI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] + op.si10;

			if (op.rt == 1 && op.si10 % 16)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: AI, 0x%x", pos, op.si10 + 0u);
			}

			break;
		}
		case spu_itype::A:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] + values[op.rb];

			if (op.rt == 1)
			{
				if (op.ra == 1 || op.rb == 1)
				{
					const u32 r2 = op.ra == 1 ? +op.rb : +op.ra;

					if (test(vflags[r2], vf::is_const) && (values[r2] % 16) == 0)
					{
						break;
					}
				}

				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: A", pos);
			}

			break;
		}
		case spu_itype::SFI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = op.si10 - values[op.ra];

			if (op.rt == 1)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: SFI", pos);
			}

			break;
		}
		case spu_itype::SF:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.rb] - values[op.ra];

			if (op.rt == 1)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: SF", pos);
			}

			break;
		}
		case spu_itype::ROTMI:
		{
			m_regmod[pos / 4] = op.rt;

			if (op.rt == 1)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: ROTMI", pos);
			}

			if (-op.i7 & 0x20)
			{
				vflags[op.rt] = +vf::is_const;
				values[op.rt] = 0;
				break;
			}

			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] >> (-op.i7 & 0x1f);
			break;
		}
		case spu_itype::SHLI:
		{
			m_regmod[pos / 4] = op.rt;

			if (op.rt == 1)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: SHLI", pos);
			}

			if (op.i7 & 0x20)
			{
				vflags[op.rt] = +vf::is_const;
				values[op.rt] = 0;
				break;
			}

			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] << (op.i7 & 0x1f);
			break;
		}

		default:
		{
			// Unconst
			const u32 op_rt = type & spu_itype::_quadrop ? +op.rt4 : +op.rt;
			m_regmod[pos / 4] = op_rt;
			vflags[op_rt] = {};

			if (op_rt == 1)
			{
				LOG_TODO(SPU, "[0x%x] Unexpected instruction on $SP: %s", pos, s_spu_iname.decode(data));
			}

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

	while (g_cfg.core.spu_block_size != spu_block_size_type::giga || limit < 0x40000)
	{
		const u32 initial_size = result.size();

		// Check unreachable blocks
		limit = std::min<u32>(limit, lsa + initial_size * 4 - 4);

		for (auto& pair : m_preds)
		{
			bool reachable = false;

			if (pair.first >= limit)
			{
				continue;
			}

			// All (direct and indirect) predecessors to check
			std::basic_string<u32> workload;

			// Bit array used to deduplicate workload list
			workload.push_back(pair.first);
			m_bits[pair.first / 4] = true;

			for (std::size_t i = 0; !reachable && i < workload.size(); i++)
			{
				for (u32 j = workload[i];; j -= 4)
				{
					// Go backward from an address until the entry point is reached
					if (j == result[0])
					{
						reachable = true;
						break;
					}

					const auto found = m_preds.find(j);

					bool had_fallthrough = false;

					if (found != m_preds.end())
					{
						for (u32 new_pred : found->second)
						{
							// Check whether the predecessor is previous instruction
							if (new_pred == j - 4)
							{
								had_fallthrough = true;
								continue;
							}

							// Check whether in range and not already added
							if (new_pred >= lsa && new_pred < limit && !m_bits[new_pred / 4])
							{
								workload.push_back(new_pred);
								m_bits[new_pred / 4] = true;
							}
						}
					}

					// Check for possible fallthrough predecessor
					if (!had_fallthrough)
					{
						if (result.at((j - lsa) / 4) == 0 || m_targets.count(j - 4))
						{
							break;
						}
					}

					if (i == 0)
					{
						// TODO
					}
				}
			}

			for (u32 pred : workload)
			{
				m_bits[pred / 4] = false;
			}

			if (!reachable && pair.first < limit)
			{
				limit = pair.first;
			}
		}

		result.resize((limit - lsa) / 4 + 1);

		// Check holes in safe mode (TODO)
		u32 valid_size = 0;

		for (u32 i = 1; i < result.size(); i++)
		{
			if (result[i] == 0)
			{
				const u32 pos  = lsa + (i - 1) * 4;
				const u32 data = ls[pos / 4];

				// Allow only NOP or LNOP instructions in holes
				if (data == 0x200000 || (data & 0xffffff80) == 0x40200000)
				{
					continue;
				}

				if (g_cfg.core.spu_block_size != spu_block_size_type::giga)
				{
					result.resize(valid_size + 1);
					break;
				}
			}
			else
			{
				valid_size = i;
			}
		}

		// Even if NOP or LNOP, should be removed at the end
		result.resize(valid_size + 1);

		// Repeat if blocks were removed
		if (result.size() == initial_size)
		{
			break;
		}
	}

	limit = std::min<u32>(limit, lsa + ::size32(result) * 4 - 4);

	// Cleanup block info
	for (u32 i = 0; i < workload.size(); i++)
	{
		const u32 addr = workload[i];

		if (addr < lsa || addr >= limit || !result[(addr - lsa) / 4 + 1])
		{
			m_block_info[addr / 4] = false;
			m_entry_info[addr / 4] = false;
			m_preds.erase(addr);
		}
	}

	// Complete m_preds and associated m_targets for adjacent blocks
	for (auto it = m_preds.begin(); it != m_preds.end();)
	{
		if (it->first < lsa || it->first >= limit)
		{
			it = m_preds.erase(it);
			continue;
		}

		// Erase impossible predecessors
		const auto new_end = std::remove_if(it->second.begin(), it->second.end(), [&](u32 addr)
		{
			return addr < lsa || addr >= limit;
		});

		it->second.erase(new_end, it->second.end());

		// Don't add fallthrough target if all predecessors are removed
		if (it->second.empty() && !m_entry_info[it->first / 4])
		{
			// If not an entry point, remove the block completely
			m_block_info[it->first / 4] = false;
			it = m_preds.erase(it);
			continue;
		}

		// Previous instruction address
		const u32 prev = (it->first - 4) & 0x3fffc;

		// TODO: check the correctness
		if (m_targets.count(prev) == 0 && prev >= lsa && prev < limit && result[(prev - lsa) / 4 + 1])
		{
			// Add target and the predecessor
			m_targets[prev].push_back(it->first);
			it->second.push_back(prev);
		}

		it++;
	}

	// Remove unnecessary target lists
	for (auto it = m_targets.begin(); it != m_targets.end();)
	{
		if (it->first < lsa || it->first >= limit)
		{
			it = m_targets.erase(it);
			continue;
		}

		// Erase unreachable targets
		const auto new_end = std::remove_if(it->second.begin(), it->second.end(), [&](u32 addr)
		{
			return addr < lsa || addr >= limit;
		});

		it->second.erase(new_end, it->second.end());

		it++;
	}

	// Fill holes which contain only NOP and LNOP instructions
	for (u32 i = 1, nnop = 0, vsize = 0; i <= result.size(); i++)
	{
		if (i >= result.size() || result[i])
		{
			if (nnop && nnop == i - vsize - 1)
			{
				// Write only complete NOP sequence
				for (u32 j = vsize + 1; j < i; j++)
				{
					result[j] = se_storage<u32>::swap(ls[lsa / 4 + j - 1]);
				}
			}

			nnop  = 0;
			vsize = i;
		}
		else
		{
			const u32 pos  = lsa + (i - 1) * 4;
			const u32 data = ls[pos / 4];

			if (data == 0x200000 || (data & 0xffffff80) == 0x40200000)
			{
				nnop++;
			}
		}
	}

	// Fill entry map, add entry points
	while (true)
	{
		workload.clear();
		workload.push_back(entry_point);
		std::memset(m_entry_map.data(), 0, sizeof(m_entry_map));

		std::basic_string<u32> new_entries;

		for (u32 wi = 0; wi < workload.size(); wi++)
		{
			const u32 addr = workload[wi];
			const u16 _new = m_entry_map[addr / 4];

			if (!m_entry_info[addr / 4])
			{
				// Check block predecessors
				for (u32 pred : m_preds[addr])
				{
					const u16 _old = m_entry_map[pred / 4];

					if (_old && _old != _new)
					{
						// If block has multiple 'entry' points, it becomes an entry point itself
						new_entries.push_back(addr);
					}
				}
			}

			// Fill value
			const u16 root = m_entry_info[addr / 4] ? ::narrow<u16>(addr / 4) : _new;

			for (u32 wa = addr; wa < limit && result[(wa - lsa) / 4 + 1]; wa += 4)
			{
				// Fill entry address for the instruction
				m_entry_map[wa / 4] = root;

				// Find targets (also means end of the block)
				const auto tfound = m_targets.find(wa);

				if (tfound == m_targets.end())
				{
					continue;
				}

				for (u32 target : tfound->second)
				{
					const u16 value = m_entry_info[target / 4] ? ::narrow<u16>(target / 4) : root;

					if (u16& tval = m_entry_map[target / 4])
					{
						// TODO: fix condition
						if (tval != value && !m_entry_info[target / 4])
						{
							new_entries.push_back(target);
						}
					}
					else
					{
						tval = value;
						workload.emplace_back(target);
					}
				}

				break;
			}
		}

		if (new_entries.empty())
		{
			break;
		}

		for (u32 entry : new_entries)
		{
			m_entry_info[entry / 4] = true;
		}
	}

	if (result.size() == 1)
	{
		// Blocks starting from 0x0 or invalid instruction won't be compiled, may need special interpreter fallback
		result.clear();
	}

	return result;
}

void spu_recompiler_base::dump(std::string& out)
{
	for (u32 i = 0; i < 0x10000; i++)
	{
		if (m_block_info[i])
		{
			fmt::append(out, "A: [0x%05x] %s\n", i * 4, m_entry_info[i] ? "Entry" : "Block");
		}
	}

	for (auto&& pair : std::map<u32, std::basic_string<u32>>(m_targets.begin(), m_targets.end()))
	{
		fmt::append(out, "T: [0x%05x]\n", pair.first);

		for (u32 value : pair.second)
		{
			fmt::append(out, "\t-> 0x%05x\n", value);
		}
	}

	for (auto&& pair : std::map<u32, std::basic_string<u32>>(m_preds.begin(), m_preds.end()))
	{
		fmt::append(out, "P: [0x%05x]\n", pair.first);

		for (u32 value : pair.second)
		{
			fmt::append(out, "\t<- 0x%05x\n", value);
		}
	}

	out += '\n';
}

#ifdef LLVM_AVAILABLE

#include "Emu/CPU/CPUTranslator.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Analysis/Lint.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Vectorize.h"
#include "Utilities/JIT.h"

class spu_llvm_runtime
{
	shared_mutex m_mutex;

	// All functions
	std::map<std::vector<u32>, spu_function_t> m_map;

	// All dispatchers
	std::array<atomic_t<spu_function_t>, 0x10000> m_dispatcher;

	// JIT instance
	jit_compiler m_jit{{}, jit_compiler::cpu(g_cfg.core.llvm_cpu)};

	// Debug module output location
	std::string m_cache_path;

	friend class spu_llvm_recompiler;

public:
	spu_llvm_runtime()
	{
		// Initialize lookup table
		for (auto& v : m_dispatcher)
		{
			v.raw() = &spu_recompiler_base::dispatch;
		}

		// Initialize "empty" block
		m_map[std::vector<u32>()] = &spu_recompiler_base::dispatch;

		// Clear LLVM output
		m_cache_path = fxm::check_unlocked<ppu_module>()->cache;
		fs::create_dir(m_cache_path + "llvm/");
		fs::remove_all(m_cache_path + "llvm/", false);

		if (g_cfg.core.spu_debug)
		{
			fs::file(m_cache_path + "spu.log", fs::rewrite);
		}

		LOG_SUCCESS(SPU, "SPU Recompiler Runtime (LLVM) initialized...");
	}
};

class spu_llvm_recompiler : public spu_recompiler_base, public cpu_translator
{
	std::shared_ptr<spu_llvm_runtime> m_spurt;

	// Current function (chunk)
	llvm::Function* m_function;

	// Current function chunk entry point
	u32 m_entry;

	llvm::Value* m_thread;
	llvm::Value* m_lsptr;

	// Pointers to registers in the thread context
	std::array<llvm::Value*, s_reg_max> m_reg_addr;

	// Global variable (function table)
	llvm::GlobalVariable* m_function_table{};

	struct block_info
	{
		// Current block's entry block
		llvm::BasicBlock* block;

		// Final block (for PHI nodes, set after completion)
		llvm::BasicBlock* block_end{};

		// Regmod compilation (TODO)
		std::bitset<s_reg_max> mod;

		// List of actual predecessors
		std::basic_string<u32> preds;

		// Current register values
		std::array<llvm::Value*, s_reg_max> reg{};

		// PHI nodes created for this block (if any)
		std::array<llvm::PHINode*, s_reg_max> phi{};

		// Store instructions
		std::array<llvm::StoreInst*, s_reg_max> store{};
	};

	struct chunk_info
	{
		// Callable function
		llvm::Function* func;

		// Constants in non-volatile registers at the entry point
		std::array<llvm::Value*, s_reg_max> reg{};

		chunk_info() = default;

		chunk_info(llvm::Function* func)
			: func(func)
		{
		}
	};

	// Current block
	block_info* m_block;

	// Current chunk
	chunk_info* m_finfo;

	// All blocks in the current function chunk
	std::unordered_map<u32, block_info, value_hash<u32, 2>> m_blocks;

	// Block list for processing
	std::vector<u32> m_block_queue;

	// All function chunks in current SPU compile unit
	std::unordered_map<u32, chunk_info, value_hash<u32, 2>> m_functions;

	// Function chunk list for processing
	std::vector<u32> m_function_queue;

	// Helper
	std::vector<u32> m_scan_queue;

	// Add or get the function chunk
	llvm::Function* add_function(u32 addr)
	{
		// Get function chunk name
		const std::string name = fmt::format("spu-chunk-0x%05x", addr);
		llvm::Function* result = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(name, get_type<void>(), get_type<u8*>(), get_type<u8*>(), get_type<u32>()));

		// Set parameters
		result->setLinkage(llvm::GlobalValue::InternalLinkage);
		result->addAttribute(1, llvm::Attribute::NoAlias);
		result->addAttribute(2, llvm::Attribute::NoAlias);

		// Enqueue if necessary
		const auto empl = m_functions.emplace(addr, chunk_info{result});

		if (empl.second)
		{
			m_function_queue.push_back(addr);

			if (m_block && g_cfg.core.spu_block_size != spu_block_size_type::safe)
			{
				// Initialize constants for non-volatile registers (TODO)
				auto& regs = empl.first->second.reg;

				for (u32 i = 80; i <= 127; i++)
				{
					if (auto c = llvm::dyn_cast_or_null<llvm::Constant>(m_block->reg[i]))
					{
						if (!(find_reg_origin(addr, i, false) >> 31))
						{
							regs[i] = c;
						}
					}
				}
			}
		}

		return result;
	}

	void set_function(llvm::Function* func)
	{
		m_function = func;
		m_thread = &*func->arg_begin();
		m_lsptr = &*(func->arg_begin() + 1);

		m_reg_addr.fill(nullptr);
		m_block = nullptr;
		m_finfo = nullptr;
		m_blocks.clear();
		m_block_queue.clear();
		m_ir->SetInsertPoint(llvm::BasicBlock::Create(m_context, "", m_function));
	}

	// Add block with current block as a predecessor
	llvm::BasicBlock* add_block(u32 target)
	{
		// Check the predecessor
		const bool pred_found = m_block_info[target / 4] && m_preds[target].find_first_of(m_pos) != -1;

		if (m_blocks.empty())
		{
			// Special case: first block, proceed normally
		}
		else if (m_block_info[target / 4] && m_entry_info[target / 4] && !(pred_found && m_entry == target))
		{
			// Generate a tail call to the function chunk
			const auto cblock = m_ir->GetInsertBlock();
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);
			m_ir->CreateStore(m_ir->getInt32(target), spu_ptr<u32>(&SPUThread::pc));
			tail(add_function(target));
			m_ir->SetInsertPoint(cblock);
			return result;
		}
		else if (!pred_found || !m_block_info[target / 4])
		{
			if (m_block_info[target / 4])
			{
				LOG_ERROR(SPU, "[0x%x] Predecessor not found for target 0x%x (chunk=0x%x, entry=0x%x, size=%u)", m_pos, target, m_entry, m_function_queue[0], m_size / 4);
			}

			// Generate external indirect tail call
			const auto cblock = m_ir->GetInsertBlock();
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);
			m_ir->CreateStore(m_ir->getInt32(target), spu_ptr<u32>(&SPUThread::pc));
			const auto addr = m_ir->CreateGEP(m_thread, m_ir->getInt64(::offset32(&SPUThread::jit_dispatcher) + target * 2));
			const auto type = llvm::FunctionType::get(get_type<void>(), {get_type<u8*>(), get_type<u8*>(), get_type<u32>()}, false)->getPointerTo()->getPointerTo();
			tail(m_ir->CreateLoad(m_ir->CreateBitCast(addr, type)));
			m_ir->SetInsertPoint(cblock);
			return result;
		}

		auto& result = m_blocks[target].block;

		if (!result)
		{
			result = llvm::BasicBlock::Create(m_context, fmt::format("b-0x%x", target), m_function);

			// Add the block to the queue
			m_block_queue.push_back(target);
		}
		else if (m_block && m_blocks[target].block_end)
		{
			// Connect PHI nodes if necessary
			for (u32 i = 0; i < s_reg_max; i++)
			{
				if (const auto phi = m_blocks[target].phi[i])
				{
					phi->addIncoming(get_vr(i).value, m_block->block_end);
				}
			}
		}

		return result;
	}

	template <typename T>
	llvm::Value* _ptr(llvm::Value* base, u32 offset, std::string name = "")
	{
		const auto off = m_ir->CreateGEP(base, m_ir->getInt64(offset));
		const auto ptr = m_ir->CreateBitCast(off, get_type<T*>(), name);
		return ptr;
	}

	template <typename T, typename... Args>
	llvm::Value* spu_ptr(Args... offset_args)
	{
		return _ptr<T>(m_thread, ::offset32(offset_args...));
	}

	template <typename T, typename... Args>
	llvm::Value* spu_ptr(value_t<u64> add, Args... offset_args)
	{
		const auto off = m_ir->CreateGEP(m_thread, m_ir->getInt64(::offset32(offset_args...)));
		const auto ptr = m_ir->CreateBitCast(m_ir->CreateAdd(off, add.value), get_type<T*>());
		return ptr;
	}

	llvm::Value* init_vr(u32 index)
	{
		auto& ptr = m_reg_addr.at(index);

		if (!ptr)
		{
			// Save and restore current insert point if necessary
			const auto block_cur = m_ir->GetInsertBlock();

			// Emit register pointer at the beginning of the function chunk
			m_ir->SetInsertPoint(m_function->getEntryBlock().getTerminator());
			ptr = _ptr<u32[4]>(m_thread, ::offset32(&SPUThread::gpr, index), fmt::format("Reg$%u", index));
			m_ir->SetInsertPoint(block_cur);
		}

		return ptr;
	}

	llvm::Value* get_vr(u32 index, llvm::Type* type)
	{
		auto& reg = m_block->reg.at(index);

		if (!reg)
		{
			// Load register value if necessary
			reg = m_ir->CreateLoad(init_vr(index), fmt::format("Load$%u", index));
		}

		// Bitcast the constant if necessary
		if (auto c = llvm::dyn_cast<llvm::Constant>(reg))
		{
			// TODO
			if (index < 128)
			{
				return make_const_vector(get_const_vector(c, m_pos, index), type);
			}
		}

		return m_ir->CreateBitCast(reg, type);
	}

	template <typename T = u32[4]>
	value_t<T> get_vr(u32 index)
	{
		value_t<T> r;
		r.value = get_vr(index, get_type<T>());
		return r;
	}

	void set_vr(u32 index, llvm::Value* value)
	{
		// Check
		verify(HERE), m_regmod[m_pos / 4] == index;

		// Set register value
		m_block->reg.at(index) = value;

		// Get register location
		const auto addr = init_vr(index);

		// Erase previous dead store instruction if necessary
		if (m_block->store[index])
		{
			// TODO: better cross-block dead store elimination
			m_block->store[index]->eraseFromParent();
		}

		// Write register to the context
		m_block->store[index] = m_ir->CreateStore(m_ir->CreateBitCast(value, addr->getType()->getPointerElementType()), addr);
	}

	template <typename T>
	void set_vr(u32 index, T expr)
	{
		set_vr(index, expr.eval(m_ir));
	}

	// Return either basic block addr with single dominating value, or negative number of PHI entries
	u32 find_reg_origin(u32 addr, u32 index, bool chunk_only = true)
	{
		u32 result = -1;

		// Handle entry point specially
		if (chunk_only && m_entry_info[addr / 4])
		{
			result = addr;
		}

		// Used for skipping blocks from different chunks
		const u16 root = ::narrow<u16>(m_entry / 4);

		// List of predecessors to check
		m_scan_queue.clear();

		const auto pfound = m_preds.find(addr);

		if (pfound != m_preds.end())
		{
			for (u32 pred : pfound->second)
			{
				if (chunk_only && m_entry_map[pred / 4] != root)
				{
					continue;
				}

				m_scan_queue.push_back(pred);
			}
		}

		// TODO: allow to avoid untouched registers in some cases
		bool regmod_any = result == -1;

		for (u32 i = 0; i < m_scan_queue.size(); i++)
		{
			// Find whether the block modifies the selected register
			bool regmod = false;

			for (addr = m_scan_queue[i];; addr -= 4)
			{
				if (index == m_regmod.at(addr / 4))
				{
					regmod = true;
					regmod_any = true;
				}

				if (LIKELY(!m_block_info[addr / 4]))
				{
					continue;
				}

				const auto pfound = m_preds.find(addr);

				if (pfound == m_preds.end())
				{
					continue;
				}

				if (!regmod)
				{
					// Enqueue predecessors if register is not modified there
					for (u32 pred : pfound->second)
					{
						if (chunk_only && m_entry_map[pred / 4] != root)
						{
							continue;
						}

						// TODO
						if (std::find(m_scan_queue.cbegin(), m_scan_queue.cend(), pred) == m_scan_queue.cend())
						{
							m_scan_queue.push_back(pred);
						}
					}
				}

				break;
			}

			if (regmod || (chunk_only && m_entry_info[addr / 4]))
			{
				if (result == -1)
				{
					result = addr;
				}
				else if (result >> 31)
				{
					result--;
				}
				else
				{
					result = -2;
				}
			}
		}

		if (!regmod_any)
		{
			result = addr;
		}

		return result;
	}

	void update_pc()
	{
		m_ir->CreateStore(m_ir->getInt32(m_pos), spu_ptr<u32>(&SPUThread::pc))->setVolatile(true);
	}

	// Call cpu_thread::check_state if necessary and return or continue (full check)
	void check_state(u32 addr)
	{
		const auto pstate = spu_ptr<u32>(&SPUThread::state);
		const auto _body = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto check = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto stop  = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(m_ir->CreateICmpEQ(m_ir->CreateLoad(pstate), m_ir->getInt32(0)), _body, check);
		m_ir->SetInsertPoint(check);
		m_ir->CreateStore(m_ir->getInt32(addr), spu_ptr<u32>(&SPUThread::pc));
		m_ir->CreateCondBr(call(&exec_check_state, m_thread), stop, _body);
		m_ir->SetInsertPoint(stop);
		m_ir->CreateRetVoid();
		m_ir->SetInsertPoint(_body);
	}

	// Perform external call
	template <typename RT, typename... FArgs, typename... Args>
	llvm::CallInst* call(RT(*_func)(FArgs...), Args... args)
	{
		static_assert(sizeof...(FArgs) == sizeof...(Args), "spu_llvm_recompiler::call(): unexpected arg number");
		const auto iptr = reinterpret_cast<std::uintptr_t>(_func);
		const auto type = llvm::FunctionType::get(get_type<RT>(), {args->getType()...}, false)->getPointerTo();
		return m_ir->CreateCall(m_ir->CreateIntToPtr(m_ir->getInt64(iptr), type), {args...});
	}

	// Perform external call and return
	template <typename RT, typename... FArgs, typename... Args>
	void tail(RT(*_func)(FArgs...), Args... args)
	{
		const auto inst = call(_func, args...);
		inst->setTailCall();

		if (inst->getType() == get_type<void>())
		{
			m_ir->CreateRetVoid();
		}
		else
		{
			m_ir->CreateRet(inst);
		}
	}

	void tail(llvm::Value* func_ptr)
	{
		m_ir->CreateCall(func_ptr, {m_thread, m_lsptr, m_ir->getInt32(0)})->setTailCall();
		m_ir->CreateRetVoid();
	}

public:
	spu_llvm_recompiler()
		: spu_recompiler_base()
		, cpu_translator(nullptr, false)
	{
		if (g_cfg.core.spu_shared_runtime)
		{
			// TODO (local context is unsupported)
			//m_spurt = std::make_shared<spu_llvm_runtime>();
		}
	}

	virtual void init() override
	{
		// Initialize if necessary
		if (!m_spurt)
		{
			m_cache = fxm::get<spu_cache>();
			m_spurt = fxm::get_always<spu_llvm_runtime>();
			m_context = m_spurt->m_jit.get_context();
			m_use_ssse3 = m_spurt->m_jit.has_ssse3();
		}
	}

	virtual spu_function_t get(u32 lsa) override
	{
		init();

		// Simple atomic read
		return m_spurt->m_dispatcher[lsa / 4];
	}

	virtual spu_function_t compile(std::vector<u32>&& func_rv) override
	{
		init();

		// Don't lock without shared runtime
		std::unique_lock<shared_mutex> lock(m_spurt->m_mutex, std::defer_lock);

		if (g_cfg.core.spu_shared_runtime)
		{
			lock.lock();
		}

		// Try to find existing function, register new one if necessary
		const auto fn_info = m_spurt->m_map.emplace(std::move(func_rv), nullptr);

		auto& fn_location = fn_info.first->second;

		if (fn_location)
		{
			return fn_location;
		}

		auto& func = fn_info.first->first;

		std::string hash;
		{
			sha1_context ctx;
			u8 output[20];

			sha1_starts(&ctx);
			sha1_update(&ctx, reinterpret_cast<const u8*>(func.data() + 1), func.size() * 4 - 4);
			sha1_finish(&ctx, output);

			fmt::append(hash, "spu-0x%05x-%s", func[0], fmt::base57(output));
		}

		if (m_cache)
		{
			LOG_SUCCESS(SPU, "LLVM: Building %s (size %u)...", hash, func.size() - 1);
		}
		else
		{
			LOG_NOTICE(SPU, "Building function 0x%x... (size %u, %s)", func[0], func.size() - 1, hash);
		}

		SPUDisAsm dis_asm(CPUDisAsm_InterpreterMode);
		dis_asm.offset = reinterpret_cast<const u8*>(func.data() + 1);

		if (g_cfg.core.spu_block_size != spu_block_size_type::giga)
		{
			dis_asm.offset -= func[0];
		}

		m_pos = func[0];
		m_size = (func.size() - 1) * 4;
		const u32 start = m_pos * (g_cfg.core.spu_block_size != spu_block_size_type::giga);
		const u32 end = start + m_size;

		if (g_cfg.core.spu_debug)
		{
			std::string log;
			fmt::append(log, "========== SPU BLOCK 0x%05x (size %u, %s) ==========\n\n", func[0], func.size() - 1, hash);

			// Disassemble if necessary
			for (u32 i = 1; i < func.size(); i++)
			{
				const u32 pos = start + (i - 1) * 4;

				// Disasm
				dis_asm.dump_pc = pos;
				dis_asm.disasm(pos);

				if (func[i])
				{
					log += '>';
					log += dis_asm.last_opcode;
					log += '\n';
				}
				else
				{
					fmt::append(log, ">[%08x]  xx xx xx xx: <hole>\n", pos);
				}
			}

			log += '\n';
			this->dump(log);
			fs::file(m_spurt->m_cache_path + "spu.log", fs::write + fs::append).write(log);
		}

		using namespace llvm;

		// Create LLVM module
		std::unique_ptr<Module> module = std::make_unique<Module>(hash + ".obj", m_context);
		module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));
		m_module = module.get();

		// Initialize IR Builder
		IRBuilder<> irb(m_context);
		m_ir = &irb;

		// Add entry function (contains only state/code check)
		const auto main_func = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(hash, get_type<void>(), get_type<u8*>(), get_type<u8*>()));
		set_function(main_func);

		// Start compilation

		update_pc();

		const auto label_test = BasicBlock::Create(m_context, "", m_function);
		const auto label_diff = BasicBlock::Create(m_context, "", m_function);
		const auto label_body = BasicBlock::Create(m_context, "", m_function);
		const auto label_stop = BasicBlock::Create(m_context, "", m_function);

		// Emit state check
		const auto pstate = spu_ptr<u32>(&SPUThread::state);
		m_ir->CreateCondBr(m_ir->CreateICmpNE(m_ir->CreateLoad(pstate, true), m_ir->getInt32(0)), label_stop, label_test);

		// Emit code check
		u32 check_iterations = 0;
		m_ir->SetInsertPoint(label_test);

		if (!g_cfg.core.spu_verification)
		{
			// Disable check (unsafe)
			m_ir->CreateBr(label_body);
		}
		else if (func.size() - 1 == 1)
		{
			const auto cond = m_ir->CreateICmpNE(m_ir->CreateLoad(_ptr<u32>(m_lsptr, start)), m_ir->getInt32(func[1]));
			m_ir->CreateCondBr(cond, label_diff, label_body);
		}
		else if (func.size() - 1 == 2)
		{
			const auto cond = m_ir->CreateICmpNE(m_ir->CreateLoad(_ptr<u64>(m_lsptr, start)), m_ir->getInt64(static_cast<u64>(func[2]) << 32 | func[1]));
			m_ir->CreateCondBr(cond, label_diff, label_body);
		}
		else
		{
			const u32 starta = start & -32;
			const u32 enda = ::align(end, 32);
			const u32 sizea = (enda - starta) / 32;
			verify(HERE), sizea;

			llvm::Value* acc = nullptr;

			for (u32 j = starta; j < enda; j += 32)
			{
				u32 indices[8];
				bool holes = false;
				bool data = false;

				for (u32 i = 0; i < 8; i++)
				{
					const u32 k = j + i * 4;

					if (k < start || k >= end || !func[(k - start) / 4 + 1])
					{
						indices[i] = 8;
						holes      = true;
					}
					else
					{
						indices[i] = i;
						data       = true;
					}
				}

				if (!data)
				{
					// Skip aligned holes
					continue;
				}

				// Load aligned code block from LS
				llvm::Value* vls = m_ir->CreateLoad(_ptr<u32[8]>(m_lsptr, j));

				// Mask if necessary
				if (holes)
				{
					vls = m_ir->CreateShuffleVector(vls, ConstantVector::getSplat(8, m_ir->getInt32(0)), indices);
				}

				// Perform bitwise comparison and accumulate
				u32 words[8];

				for (u32 i = 0; i < 8; i++)
				{
					const u32 k = j + i * 4;
					words[i] = k >= start && k < end ? func[(k - start) / 4 + 1] : 0;
				}

				vls = m_ir->CreateXor(vls, ConstantDataVector::get(m_context, words));
				acc = acc ? m_ir->CreateOr(acc, vls) : vls;
				check_iterations++;
			}

			// Pattern for PTEST
			acc = m_ir->CreateBitCast(acc, get_type<u64[4]>());
			llvm::Value* elem = m_ir->CreateExtractElement(acc, u64{0});
			elem = m_ir->CreateOr(elem, m_ir->CreateExtractElement(acc, 1));
			elem = m_ir->CreateOr(elem, m_ir->CreateExtractElement(acc, 2));
			elem = m_ir->CreateOr(elem, m_ir->CreateExtractElement(acc, 3));

			// Compare result with zero
			const auto cond = m_ir->CreateICmpNE(elem, m_ir->getInt64(0));
			m_ir->CreateCondBr(cond, label_diff, label_body);
		}

		// Increase block counter with statistics
		m_ir->SetInsertPoint(label_body);
		const auto pbcount = spu_ptr<u64>(&SPUThread::block_counter);
		m_ir->CreateStore(m_ir->CreateAdd(m_ir->CreateLoad(pbcount), m_ir->getInt64(check_iterations)), pbcount);

		// Call the entry function chunk
		const auto entry_chunk = add_function(m_pos);
		m_ir->CreateCall(entry_chunk, {m_thread, m_lsptr, m_ir->getInt32(0)})->setTailCall();
		m_ir->CreateRetVoid();

		m_ir->SetInsertPoint(label_stop);
		m_ir->CreateRetVoid();

		m_ir->SetInsertPoint(label_diff);

		if (g_cfg.core.spu_verification)
		{
			const auto pbfail = spu_ptr<u64>(&SPUThread::block_failure);
			m_ir->CreateStore(m_ir->CreateAdd(m_ir->CreateLoad(pbfail), m_ir->getInt64(1)), pbfail);
			tail(&spu_recompiler_base::dispatch, m_thread, m_ir->getInt32(0), m_ir->getInt32(0));
		}
		else
		{
			m_ir->CreateUnreachable();
		}

		// Create function table (uninitialized)
		m_function_table = new llvm::GlobalVariable(*m_module, llvm::ArrayType::get(entry_chunk->getType(), m_size / 4), true, llvm::GlobalValue::InternalLinkage, nullptr);

		// Create function chunks
		for (std::size_t fi = 0; fi < m_function_queue.size(); fi++)
		{
			// Initialize function info
			m_entry = m_function_queue[fi];
			set_function(m_functions[m_entry].func);
			m_finfo = &m_functions[m_entry];
			m_ir->CreateBr(add_block(m_entry));

			// Emit instructions for basic blocks
			for (std::size_t bi = 0; bi < m_block_queue.size(); bi++)
			{
				// Initialize basic block info
				const u32 baddr = m_block_queue[bi];
				m_block = &m_blocks[baddr];
				m_ir->SetInsertPoint(m_block->block);

				const auto pfound = m_preds.find(baddr);

				if (pfound != m_preds.end() && !pfound->second.empty())
				{
					// Initialize registers and build PHI nodes if necessary
					for (u32 i = 0; i < s_reg_max; i++)
					{
						// TODO: optimize
						const u32 src = find_reg_origin(baddr, i);

						if (src >> 31)
						{
							// TODO: type
							const auto _phi = m_ir->CreatePHI(get_type<u32[4]>(), 0 - src);
							m_block->phi[i] = _phi;
							m_block->reg[i] = _phi;

							for (u32 pred : pfound->second)
							{
								// TODO: optimize
								while (!m_block_info[pred / 4])
								{
									pred -= 4;
								}

								const auto bfound = m_blocks.find(pred);

								if (bfound != m_blocks.end() && bfound->second.block_end)
								{
									auto& value = bfound->second.reg[i];

									if (!value || value->getType() != _phi->getType())
									{
										const auto regptr = init_vr(i);
										const auto cblock = m_ir->GetInsertBlock();
										m_ir->SetInsertPoint(bfound->second.block_end->getTerminator());

										if (!value)
										{
											// Value hasn't been loaded yet
											value = m_finfo->reg[i] ? m_finfo->reg[i] : m_ir->CreateLoad(regptr);
										}

										if (i < 128 && llvm::isa<llvm::Constant>(value))
										{
											// Bitcast the constant
											value = make_const_vector(get_const_vector(llvm::cast<llvm::Constant>(value), baddr, i), _phi->getType());
										}
										else
										{
											// Ensure correct value type
											value = m_ir->CreateBitCast(value, _phi->getType());
										}

										m_ir->SetInsertPoint(cblock);

										verify(HERE), bfound->second.block_end->getTerminator();
									}

									_phi->addIncoming(value, bfound->second.block_end);
								}
							}

							if (baddr == m_entry)
							{
								// Load value at the function chunk's entry block if necessary
								const auto regptr = init_vr(i);
								const auto cblock = m_ir->GetInsertBlock();
								m_ir->SetInsertPoint(m_function->getEntryBlock().getTerminator());
								const auto value = m_finfo->reg[i] ? m_finfo->reg[i] : m_ir->CreateLoad(regptr);
								m_ir->SetInsertPoint(cblock);
								_phi->addIncoming(value, &m_function->getEntryBlock());
							}
						}
						else if (src != baddr)
						{
							// Passthrough static value or constant
							const auto bfound = m_blocks.find(src);

							// TODO: error
							if (bfound != m_blocks.end())
							{
								m_block->reg[i] = bfound->second.reg[i];
							}
						}
						else if (baddr == m_entry)
						{
							// Passthrough constant from a different chunk
							m_block->reg[i] = m_finfo->reg[i];
						}
					}

					// Emit state check if necessary (TODO: more conditions)
					for (u32 pred : pfound->second)
					{
						if (pred >= baddr && bi > 0)
						{
							// If this block is a target of a backward branch (possibly loop), emit a check
							check_state(baddr);
							break;
						}
					}
				}

				// Emit instructions
				for (m_pos = baddr; m_pos >= start && m_pos < end && !m_ir->GetInsertBlock()->getTerminator(); m_pos += 4)
				{
					if (m_pos != baddr && m_block_info[m_pos / 4])
					{
						break;
					}

					const u32 op = se_storage<u32>::swap(func[(m_pos - start) / 4 + 1]);

					if (!op)
					{
						LOG_ERROR(SPU, "Unexpected fallthrough to 0x%x (chunk=0x%x, entry=0x%x)", m_pos, m_entry, m_function_queue[0]);
						break;
					}

					// Execute recompiler function (TODO)
					(this->*g_decoder.decode(op))({op});
				}

				// Finalize block with fallthrough if necessary
				if (!m_ir->GetInsertBlock()->getTerminator())
				{
					const u32 target = m_pos == baddr ? baddr : m_pos & 0x3fffc;

					if (m_pos != baddr)
					{
						m_pos -= 4;

						if (target >= start && target < end)
						{
							const auto tfound = m_targets.find(m_pos);

							if (tfound == m_targets.end() || tfound->second.find_first_of(target) == -1)
							{
								LOG_ERROR(SPU, "Unregistered fallthrough to 0x%x (chunk=0x%x, entry=0x%x)", target, m_entry, m_function_queue[0]);
							}
						}
					}

					m_block->block_end = m_ir->GetInsertBlock();
					m_ir->CreateBr(add_block(target));
				}

				verify(HERE), m_block->block_end;
			}
		}

		// Create function table if necessary
		if (m_function_table->getNumUses())
		{
			std::vector<llvm::Constant*> chunks;
			chunks.reserve(m_size / 4);

			const auto null = cast<Function>(module->getOrInsertFunction("spu-null", get_type<void>(), get_type<u8*>(), get_type<u8*>(), get_type<u32>()));
			null->setLinkage(llvm::GlobalValue::InternalLinkage);
			set_function(null);
			m_ir->CreateRetVoid();

			for (u32 i = start; i < end; i += 4)
			{
				const auto found = m_functions.find(i);

				if (found == m_functions.end())
				{
					if (m_entry_info[i / 4])
					{
						LOG_ERROR(SPU, "[0x%x] Function chunk not compiled: 0x%x", func[0], i);
					}

					chunks.push_back(null);
					continue;
				}

				chunks.push_back(found->second.func);

				// If a chunk has incoming constants, we can't add it to the function table (TODO)
				for (const auto c : found->second.reg)
				{
					if (c != nullptr)
					{
						chunks.back() = null;
						break;
					}
				}
			}

			m_function_table->setInitializer(llvm::ConstantArray::get(llvm::ArrayType::get(entry_chunk->getType(), m_size / 4), chunks));
		}
		else
		{
			m_function_table->eraseFromParent();
		}

		// Initialize pass manager
		legacy::FunctionPassManager pm(module.get());

		// Basic optimizations
		pm.add(createEarlyCSEPass());
		pm.add(createAggressiveDCEPass());
		pm.add(createCFGSimplificationPass());
		pm.add(createDeadStoreEliminationPass());
		//pm.add(createLintPass()); // Check

		for (const auto& func : m_functions)
		{
			pm.run(*func.second.func);
		}

		// Clear context (TODO)
		m_blocks.clear();
		m_block_queue.clear();
		m_functions.clear();
		m_function_queue.clear();
		m_scan_queue.clear();
		m_function_table = nullptr;

		// Generate a dispatcher (übertrampoline)
		std::vector<u32> addrv{func[0]};
		const auto beg = m_spurt->m_map.lower_bound(addrv);
		addrv[0] += 4;
		const auto _end = m_spurt->m_map.lower_bound(addrv);
		const u32 size0 = std::distance(beg, _end);

		if (size0 > 1)
		{
			const auto trampoline = cast<Function>(module->getOrInsertFunction(fmt::format("spu-0x%05x-trampoline-%03u", func[0], size0), get_type<void>(), get_type<u8*>(), get_type<u8*>()));
			set_function(trampoline);

			struct work
			{
				u32 size;
				u32 level;
				BasicBlock* label;
				std::map<std::vector<u32>, spu_function_t>::iterator beg;
				std::map<std::vector<u32>, spu_function_t>::iterator end;
			};

			std::vector<work> workload;
			workload.reserve(size0);
			workload.emplace_back();
			workload.back().size = size0;
			workload.back().level = 1;
			workload.back().beg = beg;
			workload.back().end = _end;
			workload.back().label = m_ir->GetInsertBlock();

			for (std::size_t i = 0; i < workload.size(); i++)
			{
				// Get copy of the workload info
				work w = workload[i];

				// Switch targets
				std::vector<std::pair<u32, llvm::BasicBlock*>> targets;

				llvm::BasicBlock* def{};

				bool unsorted = false;

				while (w.level < w.beg->first.size())
				{
					const u32 x1 = w.beg->first.at(w.level);

					if (x1 == 0)
					{
						// Cannot split: some functions contain holes at this level
						auto it = w.end;
						it--;

						if (it->first.at(w.level) != 0)
						{
							unsorted = true;
						}

						w.level++;
						continue;
					}

					auto it = w.beg;
					auto it2 = it;
					u32 x = x1;
					bool split = false;

					while (it2 != w.end)
					{
						it2++;

						const u32 x2 = it2 != w.end ? it2->first.at(w.level) : x1;

						if (x2 != x)
						{
							const u32 dist = std::distance(it, it2);

							const auto b = llvm::BasicBlock::Create(m_context, "", m_function);

							if (dist == 1 && x != 0)
							{
								m_ir->SetInsertPoint(b);

								if (const u64 fval = reinterpret_cast<u64>(it->second))
								{
									const auto ptr = m_ir->CreateIntToPtr(m_ir->getInt64(fval), main_func->getType());
									m_ir->CreateCall(ptr, {m_thread, m_lsptr})->setTailCall();
								}
								else
								{
									verify(HERE, &it->second == &fn_location);
									m_ir->CreateCall(main_func, {m_thread, m_lsptr})->setTailCall();
								}

								m_ir->CreateRetVoid();
							}
							else
							{
								workload.emplace_back(w);
								workload.back().beg = it;
								workload.back().end = it2;
								workload.back().label = b;
								workload.back().size = dist;
							}

							if (x == 0)
							{
								def = b;
							}
							else
							{
								targets.emplace_back(std::make_pair(x, b));
							}

							x = x2;
							it = it2;
							split = true;
						}
					}

					if (!split)
					{
						// Cannot split: words are identical within the range at this level
						w.level++;
					}
					else
					{
						break;
					}
				}

				if (!def && targets.empty())
				{
					LOG_ERROR(SPU, "Trampoline simplified at 0x%x (level=%u)", func[0], w.level);
					m_ir->SetInsertPoint(w.label);

					if (const u64 fval = reinterpret_cast<u64>(w.beg->second))
					{
						const auto ptr = m_ir->CreateIntToPtr(m_ir->getInt64(fval), main_func->getType());
						m_ir->CreateCall(ptr, {m_thread, m_lsptr})->setTailCall();
					}
					else
					{
						verify(HERE, &w.beg->second == &fn_location);
						m_ir->CreateCall(main_func, {m_thread, m_lsptr})->setTailCall();
					}

					m_ir->CreateRetVoid();
					continue;
				}

				if (!def)
				{
					def = llvm::BasicBlock::Create(m_context, "", m_function);

					m_ir->SetInsertPoint(def);
					tail(&spu_recompiler_base::dispatch, m_thread, m_ir->getInt32(0), m_ir->getInt32(0));
				}

				m_ir->SetInsertPoint(w.label);
				const auto add = m_ir->CreateGEP(m_lsptr, m_ir->getInt64(start + w.level * 4 - 4));
				const auto ptr = m_ir->CreateBitCast(add, get_type<u32*>());
				const auto val = m_ir->CreateLoad(ptr);
				const auto sw = m_ir->CreateSwitch(val, def, ::size32(targets));

				for (auto& pair : targets)
				{
					sw->addCase(m_ir->getInt32(pair.first), pair.second);
				}
			}
		}

		spu_function_t fn{}, tr{};

		std::string log;

		raw_string_ostream out(log);

		if (g_cfg.core.spu_debug)
		{
			fmt::append(log, "LLVM IR at 0x%x:\n", func[0]);
			out << *module; // print IR
			out << "\n\n";
		}

		if (verifyModule(*module, &out))
		{
			out.flush();
			LOG_ERROR(SPU, "LLVM: Verification failed at 0x%x:\n%s", func[0], log);

			if (g_cfg.core.spu_debug)
			{
				fs::file(m_spurt->m_cache_path + "spu.log", fs::write + fs::append).write(log);
			}

			fmt::raw_error("Compilation failed");
		}

		if (g_cfg.core.spu_debug)
		{
			// Testing only
			m_spurt->m_jit.add(std::move(module), m_spurt->m_cache_path + "llvm/");
		}
		else
		{
			m_spurt->m_jit.add(std::move(module));
		}

		m_spurt->m_jit.fin();
		fn = reinterpret_cast<spu_function_t>(m_spurt->m_jit.get_engine().getPointerToFunction(main_func));
		tr = fn;

		if (size0 > 1)
		{
			tr = reinterpret_cast<spu_function_t>(m_spurt->m_jit.get_engine().getPointerToFunction(m_function));
		}

		// Register function pointer
		fn_location = fn;

		// Trampoline
		m_spurt->m_dispatcher[func[0] / 4] = tr;

		LOG_NOTICE(SPU, "[0x%x] Compiled: %p", func[0], fn);

		if (tr != fn)
			LOG_NOTICE(SPU, "[0x%x] T: %p", func[0], tr);

		if (g_cfg.core.spu_debug)
		{
			out.flush();
			fs::file(m_spurt->m_cache_path + "spu.log", fs::write + fs::append).write(log);
		}

		if (m_cache && g_cfg.core.spu_cache)
		{
			m_cache->add(func);
		}

		return fn;
	}

	static bool exec_check_state(SPUThread* _spu)
	{
		return _spu->check_state();
	}

	template <spu_inter_func_t F>
	static void exec_fall(SPUThread* _spu, spu_opcode_t op)
	{
		if (F(*_spu, op))
		{
			_spu->pc += 4;
		}
	}

	template <spu_inter_func_t F>
	void fall(spu_opcode_t op)
	{
		update_pc();
		call(&exec_fall<F>, m_thread, m_ir->getInt32(op.opcode));
	}

	static void exec_unk(SPUThread* _spu, u32 op)
	{
		fmt::throw_exception("Unknown/Illegal instruction (0x%08x)" HERE, op);
	}

	void UNK(spu_opcode_t op_unk)
	{
		m_block->block_end = m_ir->GetInsertBlock();
		update_pc();
		tail(&exec_unk, m_thread, m_ir->getInt32(op_unk.opcode));
	}

	static bool exec_stop(SPUThread* _spu, u32 code)
	{
		return _spu->stop_and_signal(code);
	}

	void STOP(spu_opcode_t op) //
	{
		update_pc();
		const auto succ = call(&exec_stop, m_thread, m_ir->getInt32(op.opcode & 0x3fff));
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto stop = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(succ, next, stop);
		m_ir->SetInsertPoint(stop);
		m_ir->CreateRetVoid();
		m_ir->SetInsertPoint(next);

		if (g_cfg.core.spu_block_size == spu_block_size_type::safe)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			m_ir->CreateStore(m_ir->getInt32(m_pos + 4), spu_ptr<u32>(&SPUThread::pc));
			m_ir->CreateRetVoid();
		}
	}

	void STOPD(spu_opcode_t op) //
	{
		STOP(spu_opcode_t{0x3fff});
	}

	static s64 exec_rdch(SPUThread* _spu, u32 ch)
	{
		return _spu->get_ch_value(ch);
	}

	static s64 exec_read_in_mbox(SPUThread* _spu)
	{
		// TODO
		return _spu->get_ch_value(SPU_RdInMbox);
	}

	static u32 exec_read_dec(SPUThread* _spu)
	{
		const u32 res = _spu->ch_dec_value - static_cast<u32>(get_timebased_time() - _spu->ch_dec_start_timestamp);

		if (res > 1500 && g_cfg.core.spu_loop_detection)
		{
			std::this_thread::yield();
		}

		return res;
	}

	static s64 exec_read_events(SPUThread* _spu)
	{
		if (const u32 events = _spu->get_events())
		{
			return events;
		}

		// TODO
		return _spu->get_ch_value(SPU_RdEventStat);
	}

	llvm::Value* get_rdch(spu_opcode_t op, u32 off, bool atomic)
	{
		const auto ptr = _ptr<u64>(m_thread, off);
		llvm::Value* val0;

		if (atomic)
		{
			const auto val = m_ir->CreateAtomicRMW(llvm::AtomicRMWInst::Xchg, ptr, m_ir->getInt64(0), llvm::AtomicOrdering::Acquire);
			val0 = val;
		}
		else
		{
			const auto val = m_ir->CreateLoad(ptr);
			m_ir->CreateStore(m_ir->getInt64(0), ptr);
			val0 = val;
		}

		const auto _cur = m_ir->GetInsertBlock();
		const auto done = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto wait = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto stop = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(m_ir->CreateICmpSLT(val0, m_ir->getInt64(0)), done, wait);
		m_ir->SetInsertPoint(wait);
		const auto val1 = call(&exec_rdch, m_thread, m_ir->getInt32(op.ra));
		m_ir->CreateCondBr(m_ir->CreateICmpSLT(val1, m_ir->getInt64(0)), stop, done);
		m_ir->SetInsertPoint(stop);
		m_ir->CreateRetVoid();
		m_ir->SetInsertPoint(done);
		const auto rval = m_ir->CreatePHI(get_type<u64>(), 2);
		rval->addIncoming(val0, _cur);
		rval->addIncoming(val1, wait);
		return m_ir->CreateTrunc(rval, get_type<u32>());
	}

	void RDCH(spu_opcode_t op) //
	{
		value_t<u32> res;

		switch (op.ra)
		{
		case SPU_RdSRR0:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&SPUThread::srr0));
			break;
		}
		case SPU_RdInMbox:
		{
			update_pc();
			res.value = call(&exec_read_in_mbox, m_thread);
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto stop = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpSLT(res.value, m_ir->getInt64(0)), stop, next);
			m_ir->SetInsertPoint(stop);
			m_ir->CreateRetVoid();
			m_ir->SetInsertPoint(next);
			res.value = m_ir->CreateTrunc(res.value, get_type<u32>());
			break;
		}
		case MFC_RdTagStat:
		{
			res.value = get_rdch(op, ::offset32(&SPUThread::ch_tag_stat), false);
			break;
		}
		case MFC_RdTagMask:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&SPUThread::ch_tag_mask));
			break;
		}
		case SPU_RdSigNotify1:
		{
			res.value = get_rdch(op, ::offset32(&SPUThread::ch_snr1), true);
			break;
		}
		case SPU_RdSigNotify2:
		{
			res.value = get_rdch(op, ::offset32(&SPUThread::ch_snr2), true);
			break;
		}
		case MFC_RdAtomicStat:
		{
			res.value = get_rdch(op, ::offset32(&SPUThread::ch_atomic_stat), false);
			break;
		}
		case MFC_RdListStallStat:
		{
			res.value = get_rdch(op, ::offset32(&SPUThread::ch_stall_stat), false);
			break;
		}
		case SPU_RdDec:
		{
			res.value = call(&exec_read_dec, m_thread);
			break;
		}
		case SPU_RdEventMask:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&SPUThread::ch_event_mask));
			break;
		}
		case SPU_RdEventStat:
		{
			update_pc();
			res.value = call(&exec_read_events, m_thread);
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto stop = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpSLT(res.value, m_ir->getInt64(0)), stop, next);
			m_ir->SetInsertPoint(stop);
			m_ir->CreateRetVoid();
			m_ir->SetInsertPoint(next);
			res.value = m_ir->CreateTrunc(res.value, get_type<u32>());
			break;
		}
		case SPU_RdMachStat:
		{
			res.value = m_ir->CreateZExt(m_ir->CreateLoad(spu_ptr<u8>(&SPUThread::interrupts_enabled)), get_type<u32>());
			break;
		}

		default:
		{
			update_pc();
			res.value = call(&exec_rdch, m_thread, m_ir->getInt32(op.ra));
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto stop = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpSLT(res.value, m_ir->getInt64(0)), stop, next);
			m_ir->SetInsertPoint(stop);
			m_ir->CreateRetVoid();
			m_ir->SetInsertPoint(next);
			res.value = m_ir->CreateTrunc(res.value, get_type<u32>());
			break;
		}
		}

		set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
	}

	static u32 exec_rchcnt(SPUThread* _spu, u32 ch)
	{
		return _spu->get_ch_count(ch);
	}

	static u32 exec_get_events(SPUThread* _spu)
	{
		return _spu->get_events();
	}

	llvm::Value* get_rchcnt(u32 off, u64 inv = 0)
	{
		const auto val = m_ir->CreateLoad(_ptr<u64>(m_thread, off), true);
		const auto shv = m_ir->CreateLShr(val, spu_channel::off_count);
		return m_ir->CreateTrunc(m_ir->CreateXor(shv, u64{inv}), get_type<u32>());
	}

	void RCHCNT(spu_opcode_t op) //
	{
		value_t<u32> res;

		switch (op.ra)
		{
		case SPU_WrOutMbox:
		{
			res.value = get_rchcnt(::offset32(&SPUThread::ch_out_mbox), true);
			break;
		}
		case SPU_WrOutIntrMbox:
		{
			res.value = get_rchcnt(::offset32(&SPUThread::ch_out_intr_mbox), true);
			break;
		}
		case MFC_RdTagStat:
		{
			res.value = get_rchcnt(::offset32(&SPUThread::ch_tag_stat));
			break;
		}
		case MFC_RdListStallStat:
		{
			res.value = get_rchcnt(::offset32(&SPUThread::ch_stall_stat));
			break;
		}
		case SPU_RdSigNotify1:
		{
			res.value = get_rchcnt(::offset32(&SPUThread::ch_snr1));
			break;
		}
		case SPU_RdSigNotify2:
		{
			res.value = get_rchcnt(::offset32(&SPUThread::ch_snr2));
			break;
		}
		case MFC_RdAtomicStat:
		{
			res.value = get_rchcnt(::offset32(&SPUThread::ch_atomic_stat));
			break;
		}
		case MFC_WrTagUpdate:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&SPUThread::ch_tag_upd), true);
			res.value = m_ir->CreateICmpEQ(res.value, m_ir->getInt32(0));
			res.value = m_ir->CreateZExt(res.value, get_type<u32>());
			break;
		}
		case MFC_Cmd:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&SPUThread::mfc_size), true);
			res.value = m_ir->CreateSub(m_ir->getInt32(16), res.value);
			break;
		}
		case SPU_RdInMbox:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&SPUThread::ch_in_mbox), true);
			res.value = m_ir->CreateLShr(res.value, 8);
			res.value = m_ir->CreateAnd(res.value, 7);
			break;
		}
		case SPU_RdEventStat:
		{
			res.value = call(&exec_get_events, m_thread);
			res.value = m_ir->CreateICmpNE(res.value, m_ir->getInt32(0));
			res.value = m_ir->CreateZExt(res.value, get_type<u32>());
			break;
		}

		default:
		{
			res.value = call(&exec_rchcnt, m_thread, m_ir->getInt32(op.ra));
			break;
		}
		}

		set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
	}

	static bool exec_wrch(SPUThread* _spu, u32 ch, u32 value)
	{
		return _spu->set_ch_value(ch, value);
	}

	void WRCH(spu_opcode_t op) //
	{
		update_pc();
		const auto succ = call(&exec_wrch, m_thread, m_ir->getInt32(op.ra), extract(get_vr(op.rt), 3).value);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto stop = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(succ, next, stop);
		m_ir->SetInsertPoint(stop);
		m_ir->CreateRetVoid();
		m_ir->SetInsertPoint(next);
	}

	void LNOP(spu_opcode_t op) //
	{
	}

	void NOP(spu_opcode_t op) //
	{
	}

	void SYNC(spu_opcode_t op) //
	{
		// This instruction must be used following a store instruction that modifies the instruction stream.
		m_ir->CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);

		if (g_cfg.core.spu_block_size == spu_block_size_type::safe)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			m_ir->CreateStore(m_ir->getInt32(m_pos + 4), spu_ptr<u32>(&SPUThread::pc));
			m_ir->CreateRetVoid();
		}
	}

	void DSYNC(spu_opcode_t op) //
	{
		// This instruction forces all earlier load, store, and channel instructions to complete before proceeding.
		SYNC(op);
	}

	void MFSPR(spu_opcode_t op) //
	{
		// Check SPUInterpreter for notes.
		set_vr(op.rt, splat<u32[4]>(0));
	}

	void MTSPR(spu_opcode_t op) //
	{
		// Check SPUInterpreter for notes.
	}

	void SF(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr(op.rb) - get_vr(op.ra));
	}

	void OR(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr(op.ra) | get_vr(op.rb));
	}

	void BG(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);
		set_vr(op.rt, zext<u32[4]>(a <= b));
	}

	void SFH(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<u16[8]>(op.rb) - get_vr<u16[8]>(op.ra));
	}

	void NOR(spu_opcode_t op) //
	{
		set_vr(op.rt, ~(get_vr(op.ra) | get_vr(op.rb)));
	}

	void ABSDB(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);
		set_vr(op.rt, max(a, b) - min(a, b));
	}

	void ROT(spu_opcode_t op)
	{
		set_vr(op.rt, rol(get_vr(op.ra), get_vr(op.rb)));
	}

	void ROTM(spu_opcode_t op)
	{
		const auto sh = eval(-get_vr(op.rb) & 0x3f);
		set_vr(op.rt, select(sh < 0x20, eval(get_vr(op.ra) >> sh), splat<u32[4]>(0)));
	}

	void ROTMA(spu_opcode_t op)
	{
		const auto sh = eval(-get_vr(op.rb) & 0x3f);
		set_vr(op.rt, get_vr<s32[4]>(op.ra) >> bitcast<s32[4]>(min(sh, splat<u32[4]>(0x1f))));
	}

	void SHL(spu_opcode_t op)
	{
		const auto sh = eval(get_vr(op.rb) & 0x3f);
		set_vr(op.rt, select(sh < 0x20, eval(get_vr(op.ra) << sh), splat<u32[4]>(0)));
	}

	void ROTH(spu_opcode_t op)
	{
		set_vr(op.rt, rol(get_vr<u16[8]>(op.ra), get_vr<u16[8]>(op.rb)));
	}

	void ROTHM(spu_opcode_t op)
	{
		const auto sh = eval(-get_vr<u16[8]>(op.rb) & 0x1f);
		set_vr(op.rt, select(sh < 0x10, eval(get_vr<u16[8]>(op.ra) >> sh), splat<u16[8]>(0)));
	}

	void ROTMAH(spu_opcode_t op)
	{
		const auto sh = eval(-get_vr<u16[8]>(op.rb) & 0x1f);
		set_vr(op.rt, get_vr<s16[8]>(op.ra) >> bitcast<s16[8]>(min(sh, splat<u16[8]>(0xf))));
	}

	void SHLH(spu_opcode_t op)
	{
		const auto sh = eval(get_vr<u16[8]>(op.rb) & 0x1f);
		set_vr(op.rt, select(sh < 0x10, eval(get_vr<u16[8]>(op.ra) << sh), splat<u16[8]>(0)));
	}

	void ROTI(spu_opcode_t op)
	{
		set_vr(op.rt, rol(get_vr(op.ra), op.i7));
	}

	void ROTMI(spu_opcode_t op)
	{
		if (-op.i7 & 0x20)
		{
			return set_vr(op.rt, splat<u32[4]>(0));
		}

		set_vr(op.rt, get_vr(op.ra) >> (-op.i7 & 0x1f));
	}

	void ROTMAI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) >> (-op.i7 & 0x20 ? 0x1f : -op.i7 & 0x1f));
	}

	void SHLI(spu_opcode_t op)
	{
		if (op.i7 & 0x20)
		{
			return set_vr(op.rt, splat<u32[4]>(0));
		}

		set_vr(op.rt, get_vr(op.ra) << (op.i7 & 0x1f));
	}

	void ROTHI(spu_opcode_t op)
	{
		set_vr(op.rt, rol(get_vr<u16[8]>(op.ra), op.i7));
	}

	void ROTHMI(spu_opcode_t op)
	{
		if (-op.i7 & 0x10)
		{
			return set_vr(op.rt, splat<u16[8]>(0));
		}

		set_vr(op.rt, get_vr<u16[8]>(op.ra) >> (-op.i7 & 0xf));
	}

	void ROTMAHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) >> (-op.i7 & 0x10 ? 0xf : -op.i7 & 0xf));
	}

	void SHLHI(spu_opcode_t op)
	{
		if (op.i7 & 0x10)
		{
			return set_vr(op.rt, splat<u16[8]>(0));
		}

		set_vr(op.rt, get_vr<u16[8]>(op.ra) << (op.i7 & 0xf));
	}

	void A(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr(op.ra) + get_vr(op.rb));
	}

	void AND(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr(op.ra) & get_vr(op.rb));
	}

	void CG(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);
		set_vr(op.rt, zext<u32[4]>(a + b < a));
	}

	void AH(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<u16[8]>(op.ra) + get_vr<u16[8]>(op.rb));
	}

	void NAND(spu_opcode_t op) //
	{
		set_vr(op.rt, ~(get_vr(op.ra) & get_vr(op.rb)));
	}

	void AVGB(spu_opcode_t op)
	{
		set_vr(op.rt, avg(get_vr<u8[16]>(op.ra), get_vr<u8[16]>(op.rb)));
	}

	void GB(spu_opcode_t op)
	{
		const auto a = get_vr<s32[4]>(op.ra);

		if (auto cv = llvm::dyn_cast<llvm::Constant>(a.value))
		{
			v128 data = get_const_vector(cv, m_pos, 680);
			u32 res = 0;
			for (u32 i = 0; i < 4; i++)
				res |= (data._u32[i] & 1) << i;
			set_vr(op.rt, build<u32[4]>(0, 0, 0, res));
			return;
		}

		const auto m = zext<u32>(bitcast<i4>(trunc<bool[4]>(a)));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, m));
	}

	void GBH(spu_opcode_t op)
	{
		const auto a = get_vr<s16[8]>(op.ra);

		if (auto cv = llvm::dyn_cast<llvm::Constant>(a.value))
		{
			v128 data = get_const_vector(cv, m_pos, 684);
			u32 res = 0;
			for (u32 i = 0; i < 8; i++)
				res |= (data._u16[i] & 1) << i;
			set_vr(op.rt, build<u32[4]>(0, 0, 0, res));
			return;
		}

		const auto m = zext<u32>(bitcast<u8>(trunc<bool[8]>(a)));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, m));
	}

	void GBB(spu_opcode_t op)
	{
		const auto a = get_vr<s8[16]>(op.ra);

		if (auto cv = llvm::dyn_cast<llvm::Constant>(a.value))
		{
			v128 data = get_const_vector(cv, m_pos, 688);
			u32 res = 0;
			for (u32 i = 0; i < 16; i++)
				res |= (data._u8[i] & 1) << i;
			set_vr(op.rt, build<u32[4]>(0, 0, 0, res));
			return;
		}

		const auto m = zext<u32>(bitcast<u16>(trunc<bool[16]>(a)));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, m));
	}

	void FSM(spu_opcode_t op)
	{
		const auto v = extract(get_vr(op.ra), 3);

		if (auto cv = llvm::dyn_cast<llvm::ConstantInt>(v.value))
		{
			const u64 v = cv->getZExtValue() & 0xf;
			set_vr(op.rt, -(build<u32[4]>(v >> 0, v >> 1, v >> 2, v >> 3) & 1));
			return;
		}

		const auto m = bitcast<bool[4]>(trunc<i4>(v));
		set_vr(op.rt, sext<u32[4]>(m));
	}

	void FSMH(spu_opcode_t op)
	{
		const auto v = extract(get_vr(op.ra), 3);

		if (auto cv = llvm::dyn_cast<llvm::ConstantInt>(v.value))
		{
			const u64 v = cv->getZExtValue() & 0xff;
			set_vr(op.rt, -(build<u16[8]>(v >> 0, v >> 1, v >> 2, v >> 3, v >> 4, v >> 5, v >> 6, v >> 7) & 1));
			return;
		}

		const auto m = bitcast<bool[8]>(trunc<u8>(v));
		set_vr(op.rt, sext<u16[8]>(m));
	}

	void FSMB(spu_opcode_t op)
	{
		const auto v = extract(get_vr(op.ra), 3);

		if (auto cv = llvm::dyn_cast<llvm::ConstantInt>(v.value))
		{
			const u64 v = cv->getZExtValue() & 0xffff;
			op.i16 = static_cast<u32>(v);
			return FSMBI(op);
		}

		const auto m = bitcast<bool[16]>(trunc<u16>(v));
		set_vr(op.rt, sext<u8[16]>(m));
	}

	void ROTQBYBI(spu_opcode_t op)
	{
		auto sh = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		sh = eval((sh - (zshuffle<u8[16]>(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) >> 3)) & 0xf);
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void ROTQMBYBI(spu_opcode_t op)
	{
		auto sh = build<u8[16]>(112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127);
		sh = eval(sh + (-(zshuffle<u8[16]>(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) >> 3) & 0x1f));
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void SHLQBYBI(spu_opcode_t op)
	{
		auto sh = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		sh = eval(sh - (zshuffle<u8[16]>(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) >> 3));
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void CBX(spu_opcode_t op)
	{
		const auto s = eval(extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3));
		const auto i = eval(~s & 0xf);
		auto r = build<u8[16]>(0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt8(0x3), i.value);
		set_vr(op.rt, r);
	}

	void CHX(spu_opcode_t op)
	{
		const auto s = eval(extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3));
		const auto i = eval(~s >> 1 & 0x7);
		auto r = build<u16[8]>(0x1e1f, 0x1c1d, 0x1a1b, 0x1819, 0x1617, 0x1415, 0x1213, 0x1011);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt16(0x0203), i.value);
		set_vr(op.rt, r);
	}

	void CWX(spu_opcode_t op)
	{
		const auto s = eval(extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3));
		const auto i = eval(~s >> 2 & 0x3);
		auto r = build<u32[4]>(0x1c1d1e1f, 0x18191a1b, 0x14151617, 0x10111213);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt32(0x010203), i.value);
		set_vr(op.rt, r);
	}

	void CDX(spu_opcode_t op)
	{
		const auto s = eval(extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3));
		const auto i = eval(~s >> 3 & 0x1);
		auto r = build<u64[2]>(0x18191a1b1c1d1e1f, 0x1011121314151617);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt64(0x01020304050607), i.value);
		set_vr(op.rt, r);
	}

	void ROTQBI(spu_opcode_t op)
	{
		const auto a = get_vr<u64[2]>(op.ra);
		const auto b = eval((get_vr<u64[2]>(op.rb) >> 32) & 0x7);
		set_vr(op.rt, a << zshuffle<u64[2]>(b, 1, 1) | zshuffle<u64[2]>(a, 1, 0) >> 56 >> zshuffle<u64[2]>(8 - b, 1, 1));
	}

	void ROTQMBI(spu_opcode_t op)
	{
		const auto a = get_vr<u64[2]>(op.ra);
		const auto b = eval(-(get_vr<u64[2]>(op.rb) >> 32) & 0x7);
		set_vr(op.rt, a >> zshuffle<u64[2]>(b, 1, 1) | zshuffle<u64[2]>(a, 1, 2) << 56 << zshuffle<u64[2]>(8 - b, 1, 1));
	}

	void SHLQBI(spu_opcode_t op)
	{
		const auto a = get_vr<u64[2]>(op.ra);
		const auto b = eval((get_vr<u64[2]>(op.rb) >> 32) & 0x7);
		set_vr(op.rt, a << zshuffle<u64[2]>(b, 1, 1) | zshuffle<u64[2]>(a, 2, 0) >> 56 >> zshuffle<u64[2]>(8 - b, 1, 1));
	}

	void ROTQBY(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);
		auto sh = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		sh = eval((sh - zshuffle<u8[16]>(b, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12)) & 0xf);
		set_vr(op.rt, pshufb(a, sh));
	}

	void ROTQMBY(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);
		auto sh = build<u8[16]>(112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127);
		sh = eval(sh + (-zshuffle<u8[16]>(b, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) & 0x1f));
		set_vr(op.rt, pshufb(a, sh));
	}

	void SHLQBY(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);
		auto sh = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		sh = eval(sh - (zshuffle<u8[16]>(b, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) & 0x1f));
		set_vr(op.rt, pshufb(a, sh));
	}

	void ORX(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto x = zshuffle<u32[4]>(a, 2, 3, 0, 1) | a;
		const auto y = zshuffle<u32[4]>(x, 1, 0, 3, 2) | x;
		set_vr(op.rt, zshuffle<u32[4]>(y, 4, 4, 4, 3));
	}

	void CBD(spu_opcode_t op)
	{
		const auto a = eval(extract(get_vr(op.ra), 3) + op.i7);
		const auto i = eval(~a & 0xf);
		auto r = build<u8[16]>(0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt8(0x3), i.value);
		set_vr(op.rt, r);
	}

	void CHD(spu_opcode_t op)
	{
		const auto a = eval(extract(get_vr(op.ra), 3) + op.i7);
		const auto i = eval(~a >> 1 & 0x7);
		auto r = build<u16[8]>(0x1e1f, 0x1c1d, 0x1a1b, 0x1819, 0x1617, 0x1415, 0x1213, 0x1011);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt16(0x0203), i.value);
		set_vr(op.rt, r);
	}

	void CWD(spu_opcode_t op)
	{
		const auto a = eval(extract(get_vr(op.ra), 3) + op.i7);
		const auto i = eval(~a >> 2 & 0x3);
		auto r = build<u32[4]>(0x1c1d1e1f, 0x18191a1b, 0x14151617, 0x10111213);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt32(0x010203), i.value);
		set_vr(op.rt, r);
	}

	void CDD(spu_opcode_t op)
	{
		const auto a = eval(extract(get_vr(op.ra), 3) + op.i7);
		const auto i = eval(~a >> 3 & 0x1);
		auto r = build<u64[2]>(0x18191a1b1c1d1e1f, 0x1011121314151617);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt64(0x01020304050607), i.value);
		set_vr(op.rt, r);
	}

	void ROTQBII(spu_opcode_t op)
	{
		const auto s = op.i7 & 0x7;
		const auto a = get_vr(op.ra);

		if (s == 0)
		{
			return set_vr(op.rt, a);
		}

		set_vr(op.rt, a << s | zshuffle<u32[4]>(a, 3, 0, 1, 2) >> (32 - s));
	}

	void ROTQMBII(spu_opcode_t op)
	{
		const auto s = -op.i7 & 0x7;
		const auto a = get_vr(op.ra);

		if (s == 0)
		{
			return set_vr(op.rt, a);
		}

		set_vr(op.rt, a >> s | zshuffle<u32[4]>(a, 1, 2, 3, 4) << (32 - s));
	}

	void SHLQBII(spu_opcode_t op)
	{
		const auto s = op.i7 & 0x7;
		const auto a = get_vr(op.ra);

		if (s == 0)
		{
			return set_vr(op.rt, a);
		}

		set_vr(op.rt, a << s | zshuffle<u32[4]>(a, 4, 0, 1, 2) >> (32 - s));
	}

	void ROTQBYI(spu_opcode_t op)
	{
		const u32 s = -op.i7 & 0xf;
		set_vr(op.rt, zshuffle<u8[16]>(get_vr<u8[16]>(op.ra),
			s & 15, (s + 1) & 15, (s + 2) & 15, (s + 3) & 15,
			(s + 4) & 15, (s + 5) & 15, (s + 6) & 15, (s + 7) & 15,
			(s + 8) & 15, (s + 9) & 15, (s + 10) & 15, (s + 11) & 15,
			(s + 12) & 15, (s + 13) & 15, (s + 14) & 15, (s + 15) & 15));
	}

	void ROTQMBYI(spu_opcode_t op)
	{
		const u32 s = -op.i7 & 0x1f;

		if (s >= 16)
		{
			return set_vr(op.rt, splat<u32[4]>(0));
		}

		set_vr(op.rt, zshuffle<u8[16]>(get_vr<u8[16]>(op.ra),
			s, s + 1, s + 2, s + 3,
			s + 4, s + 5, s + 6, s + 7,
			s + 8, s + 9, s + 10, s + 11,
			s + 12, s + 13, s + 14, s + 15));
	}

	void SHLQBYI(spu_opcode_t op)
	{
		const u32 s = op.i7 & 0x1f;

		if (s >= 16)
		{
			return set_vr(op.rt, splat<u32[4]>(0));
		}

		const u32 x = -s;

		set_vr(op.rt, zshuffle<u8[16]>(get_vr<u8[16]>(op.ra),
			x & 31, (x + 1) & 31, (x + 2) & 31, (x + 3) & 31,
			(x + 4) & 31, (x + 5) & 31, (x + 6) & 31, (x + 7) & 31,
			(x + 8) & 31, (x + 9) & 31, (x + 10) & 31, (x + 11) & 31,
			(x + 12) & 31, (x + 13) & 31, (x + 14) & 31, (x + 15) & 31));
	}

	void CGT(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr<s32[4]>(op.ra) > get_vr<s32[4]>(op.rb)));
	}

	void XOR(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr(op.ra) ^ get_vr(op.rb));
	}

	void CGTH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<s16[8]>(op.ra) > get_vr<s16[8]>(op.rb)));
	}

	void EQV(spu_opcode_t op) //
	{
		set_vr(op.rt, ~(get_vr(op.ra) ^ get_vr(op.rb)));
	}

	void CGTB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<s8[16]>(op.ra) > get_vr<s8[16]>(op.rb)));
	}

	void SUMB(spu_opcode_t op)
	{
		const auto a = get_vr<u16[8]>(op.ra);
		const auto b = get_vr<u16[8]>(op.rb);
		const auto ahs = eval((a >> 8) + (a & 0xff));
		const auto bhs = eval((b >> 8) + (b & 0xff));
		const auto lsh = shuffle2<u16[8]>(ahs, bhs, 0, 9, 2, 11, 4, 13, 6, 15);
		const auto hsh = shuffle2<u16[8]>(ahs, bhs, 1, 8, 3, 10, 5, 12, 7, 14);
		set_vr(op.rt, lsh + hsh);
	}

	void CLZ(spu_opcode_t op)
	{
		set_vr(op.rt, ctlz(get_vr(op.ra)));
	}

	void XSWD(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s64[2]>(op.ra) << 32 >> 32);
	}

	void XSHW(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) << 16 >> 16);
	}

	void CNTB(spu_opcode_t op)
	{
		set_vr(op.rt, ctpop(get_vr<u8[16]>(op.ra)));
	}

	void XSBH(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) << 8 >> 8);
	}

	void CLGT(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr(op.ra) > get_vr(op.rb)));
	}

	void ANDC(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr(op.ra) & ~get_vr(op.rb));
	}

	void CLGTH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) > get_vr<u16[8]>(op.rb)));
	}

	void ORC(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr(op.ra) | ~get_vr(op.rb));
	}

	void CLGTB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<u8[16]>(op.ra) > get_vr<u8[16]>(op.rb)));
	}

	void CEQ(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr(op.ra) == get_vr(op.rb)));
	}

	void MPYHHU(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) >> 16) * (get_vr(op.rb) >> 16));
	}

	void ADDX(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) + get_vr(op.rb) + (get_vr(op.rt) & 1));
	}

	void SFX(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.rb) - get_vr(op.ra) - (~get_vr(op.rt) & 1));
	}

	void CGX(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);
		const auto c = eval(get_vr<s32[4]>(op.rt) << 31);
		const auto s = eval(a + b);
		set_vr(op.rt, zext<u32[4]>(s < a | (s == -1 & c < 0)));
	}

	void BGX(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);
		const auto c = eval(get_vr<s32[4]>(op.rt) << 31);
		set_vr(op.rt, zext<u32[4]>(a <= b & ~(a == b & c >= 0)));
	}

	void MPYHHA(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) >> 16) * (get_vr<s32[4]>(op.rb) >> 16) + get_vr<s32[4]>(op.rt));
	}

	void MPYHHAU(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) >> 16) * (get_vr(op.rb) >> 16) + get_vr(op.rt));
	}

	void MPY(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) << 16 >> 16) * (get_vr<s32[4]>(op.rb) << 16 >> 16));
	}

	void MPYH(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) >> 16) * (get_vr(op.rb) << 16));
	}

	void MPYHH(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) >> 16) * (get_vr<s32[4]>(op.rb) >> 16));
	}

	void MPYS(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) << 16 >> 16) * (get_vr<s32[4]>(op.rb) << 16 >> 16) >> 16);
	}

	void CEQH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) == get_vr<u16[8]>(op.rb)));
	}

	void MPYU(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) << 16 >> 16) * (get_vr(op.rb) << 16 >> 16));
	}

	void CEQB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<u8[16]>(op.ra) == get_vr<u8[16]>(op.rb)));
	}

	void FSMBI(spu_opcode_t op)
	{
		v128 data;
		for (u32 i = 0; i < 16; i++)
			data._bytes[i] = op.i16 & (1u << i) ? -1 : 0;
		value_t<u8[16]> r;
		r.value = make_const_vector<v128>(data, get_type<u8[16]>());
		set_vr(op.rt, r);
	}

	void IL(spu_opcode_t op)
	{
		set_vr(op.rt, splat<s32[4]>(op.si16));
	}

	void ILHU(spu_opcode_t op)
	{
		set_vr(op.rt, splat<u32[4]>(op.i16 << 16));
	}

	void ILH(spu_opcode_t op)
	{
		set_vr(op.rt, splat<u16[8]>(op.i16));
	}

	void IOHL(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.rt) | op.i16);
	}

	void ORI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) | op.si10);
	}

	void ORHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) | op.si10);
	}

	void ORBI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s8[16]>(op.ra) | op.si10);
	}

	void SFI(spu_opcode_t op)
	{
		set_vr(op.rt, op.si10 - get_vr<s32[4]>(op.ra));
	}

	void SFHI(spu_opcode_t op)
	{
		set_vr(op.rt, op.si10 - get_vr<s16[8]>(op.ra));
	}

	void ANDI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) & op.si10);
	}

	void ANDHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) & op.si10);
	}

	void ANDBI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s8[16]>(op.ra) & op.si10);
	}

	void AI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) + op.si10);
	}

	void AHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) + op.si10);
	}

	void XORI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) ^ op.si10);
	}

	void XORHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) ^ op.si10);
	}

	void XORBI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s8[16]>(op.ra) ^ op.si10);
	}

	void CGTI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr<s32[4]>(op.ra) > op.si10));
	}

	void CGTHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<s16[8]>(op.ra) > op.si10));
	}

	void CGTBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<s8[16]>(op.ra) > op.si10));
	}

	void CLGTI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr(op.ra) > op.si10));
	}

	void CLGTHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) > op.si10));
	}

	void CLGTBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<u8[16]>(op.ra) > op.si10));
	}

	void MPYI(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) << 16 >> 16) * splat<s32[4]>(op.si10));
	}

	void MPYUI(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) << 16 >> 16) * splat<u32[4]>(op.si10 & 0xffff));
	}

	void CEQI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr(op.ra) == op.si10));
	}

	void CEQHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) == op.si10));
	}

	void CEQBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<u8[16]>(op.ra) == op.si10));
	}

	void ILA(spu_opcode_t op)
	{
		set_vr(op.rt, splat<u32[4]>(op.i18));
	}

	void SELB(spu_opcode_t op)
	{
		if (auto ei = llvm::dyn_cast_or_null<llvm::CastInst>(m_block->reg[op.rc]))
		{
			// Detect if the mask comes from a comparison instruction
			if (ei->getOpcode() == llvm::Instruction::SExt && ei->getSrcTy()->isIntOrIntVectorTy(1))
			{
				auto op0 = ei->getOperand(0);
				auto typ = ei->getDestTy();
				auto op1 = m_block->reg[op.rb];
				auto op2 = m_block->reg[op.ra];

				if (typ == get_type<u64[2]>())
				{
					if (op1 && op1->getType() == get_type<f64[2]>() || op2 && op2->getType() == get_type<f64[2]>())
					{
						op1 = get_vr<f64[2]>(op.rb).value;
						op2 = get_vr<f64[2]>(op.ra).value;
					}
					else
					{
						op1 = get_vr<u64[2]>(op.rb).value;
						op2 = get_vr<u64[2]>(op.ra).value;
					}
				}
				else if (typ == get_type<u32[4]>())
				{
					if (op1 && op1->getType() == get_type<f32[4]>() || op2 && op2->getType() == get_type<f32[4]>())
					{
						op1 = get_vr<f32[4]>(op.rb).value;
						op2 = get_vr<f32[4]>(op.ra).value;
					}
					else
					{
						op1 = get_vr<u32[4]>(op.rb).value;
						op2 = get_vr<u32[4]>(op.ra).value;
					}
				}
				else if (typ == get_type<u16[8]>())
				{
					op1 = get_vr<u16[8]>(op.rb).value;
					op2 = get_vr<u16[8]>(op.ra).value;
				}
				else if (typ == get_type<u8[16]>())
				{
					op1 = get_vr<u8[16]>(op.rb).value;
					op2 = get_vr<u8[16]>(op.ra).value;
				}
				else
				{
					LOG_ERROR(SPU, "[0x%x] SELB: unknown cast destination type", m_pos);
					op0 = nullptr;
				}

				if (op0 && op1 && op2)
				{
					set_vr(op.rt4, m_ir->CreateSelect(op0, op1, op2));
					return;
				}
			}
		}

		set_vr(op.rt4, merge(get_vr(op.rc), get_vr(op.rb), get_vr(op.ra)));
	}

	void SHUFB(spu_opcode_t op)
	{
		if (auto ii = llvm::dyn_cast_or_null<llvm::InsertElementInst>(m_block->reg[op.rc]))
		{
			// Detect if the mask comes from a CWD-like constant generation instruction
			auto c0 = llvm::dyn_cast<llvm::Constant>(ii->getOperand(0));

			if (c0 && get_const_vector(c0, m_pos, op.rc) != v128::from64(0x18191a1b1c1d1e1f, 0x1011121314151617))
			{
				c0 = nullptr;
			}

			auto c1 = llvm::dyn_cast<llvm::ConstantInt>(ii->getOperand(1));

			llvm::Type* vtype = nullptr;
			llvm::Value* _new = nullptr;

			// Optimization: emit SHUFB as simple vector insert
			if (c0 && c1 && c1->getType() == get_type<u64>() && c1->getZExtValue() == 0x01020304050607)
			{
				vtype = get_type<u64[2]>();
				_new  = extract(get_vr<u64[2]>(op.ra), 1).value;
			}
			else if (c0 && c1 && c1->getType() == get_type<u32>() && c1->getZExtValue() == 0x010203)
			{
				vtype = get_type<u32[4]>();
				_new  = extract(get_vr<u32[4]>(op.ra), 3).value;
			}
			else if (c0 && c1 && c1->getType() == get_type<u16>() && c1->getZExtValue() == 0x0203)
			{
				vtype = get_type<u16[8]>();
				_new  = extract(get_vr<u16[8]>(op.ra), 6).value;
			}
			else if (c0 && c1 && c1->getType() == get_type<u8>() && c1->getZExtValue() == 0x03)
			{
				vtype = get_type<u8[16]>();
				_new  = extract(get_vr<u8[16]>(op.ra), 12).value;
			}

			if (vtype && _new)
			{
				set_vr(op.rt4, m_ir->CreateInsertElement(get_vr(op.rb, vtype), _new, ii->getOperand(2)));
				return;
			}
		}

		const auto c = get_vr<u8[16]>(op.rc);

		if (auto ci = llvm::dyn_cast<llvm::Constant>(c.value))
		{
			// Optimization: SHUFB with constant mask
			v128 mask = get_const_vector(ci, m_pos, 57216);

			if (((mask._u64[0] | mask._u64[1]) & 0xe0e0e0e0e0e0e0e0) == 0)
			{
				// Trivial insert or constant shuffle (TODO)
				static constexpr struct mask_info
				{
					u64 i1;
					u64 i0;
					decltype(&cpu_translator::get_type<void>) type;
					u64 extract_from;
					u64 insert_to;
				} s_masks[30]
				{
					{ 0x0311121314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 15 },
					{ 0x1003121314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 14 },
					{ 0x1011031314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 13 },
					{ 0x1011120314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 12 },
					{ 0x1011121303151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 11 },
					{ 0x1011121314031617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 10 },
					{ 0x1011121314150317, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 9 },
					{ 0x1011121314151603, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 8 },
					{ 0x1011121314151617, 0x03191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 7 },
					{ 0x1011121314151617, 0x18031a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 6 },
					{ 0x1011121314151617, 0x1819031b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 5 },
					{ 0x1011121314151617, 0x18191a031c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 4 },
					{ 0x1011121314151617, 0x18191a1b031d1e1f, &cpu_translator::get_type<u8[16]>, 12, 3 },
					{ 0x1011121314151617, 0x18191a1b1c031e1f, &cpu_translator::get_type<u8[16]>, 12, 2 },
					{ 0x1011121314151617, 0x18191a1b1c1d031f, &cpu_translator::get_type<u8[16]>, 12, 1 },
					{ 0x1011121314151617, 0x18191a1b1c1d1e03, &cpu_translator::get_type<u8[16]>, 12, 0 },
					{ 0x0203121314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 7 },
					{ 0x1011020314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 6 },
					{ 0x1011121302031617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 5 },
					{ 0x1011121314150203, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 4 },
					{ 0x1011121314151617, 0x02031a1b1c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 3 },
					{ 0x1011121314151617, 0x181902031c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 2 },
					{ 0x1011121314151617, 0x18191a1b02031e1f, &cpu_translator::get_type<u16[8]>, 6, 1 },
					{ 0x1011121314151617, 0x18191a1b1c1d0203, &cpu_translator::get_type<u16[8]>, 6, 0 },
					{ 0x0001020314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u32[4]>, 3, 3 },
					{ 0x1011121300010203, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u32[4]>, 3, 2 },
					{ 0x1011121314151617, 0x000102031c1d1e1f, &cpu_translator::get_type<u32[4]>, 3, 1 },
					{ 0x1011121314151617, 0x18191a1b00010203, &cpu_translator::get_type<u32[4]>, 3, 0 },
					{ 0x0001020304050607, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u64[2]>, 1, 1 },
					{ 0x1011121303151617, 0x0001020304050607, &cpu_translator::get_type<u64[2]>, 1, 0 },
				};

				// Check important constants from CWD-like constant generation instructions
				for (const auto& cm : s_masks)
				{
					if (mask._u64[0] == cm.i0 && mask._u64[1] == cm.i1)
					{
						const auto t = (this->*cm.type)();
						const auto a = get_vr(op.ra, t);
						const auto b = get_vr(op.rb, t);
						const auto e = m_ir->CreateExtractElement(a, cm.extract_from);
						set_vr(op.rt4, m_ir->CreateInsertElement(b, e, cm.insert_to));
						return;
					}
				}

				// Generic constant shuffle without constant-generation bits
				mask = mask ^ v128::from8p(0x1f);

				if (op.ra == op.rb)
				{
					mask = mask & v128::from8p(0xf);
				}

				const auto a = get_vr<u8[16]>(op.ra);
				const auto b = get_vr<u8[16]>(op.rb);
				const auto c = make_const_vector(mask, get_type<u8[16]>());
				set_vr(op.rt4, m_ir->CreateShuffleVector(b.value, op.ra == op.rb ? b.value : a.value, m_ir->CreateZExt(c, get_type<u32[16]>())));
				return;
			}

			LOG_TODO(SPU, "[0x%x] Const SHUFB mask: %s", m_pos, mask);
		}

		const auto x = avg(sext<u8[16]>((c & 0xc0) == 0xc0), sext<u8[16]>((c & 0xe0) == 0xc0));
		const auto cr = c ^ 0xf;
		const auto a = pshufb(get_vr<u8[16]>(op.ra), cr);
		const auto b = pshufb(get_vr<u8[16]>(op.rb), cr);
		set_vr(op.rt4, select(bitcast<s8[16]>(cr << 3) >= 0, a, b) | x);
	}

	void MPYA(spu_opcode_t op)
	{
		set_vr(op.rt4, (get_vr<s32[4]>(op.ra) << 16 >> 16) * (get_vr<s32[4]>(op.rb) << 16 >> 16) + get_vr<s32[4]>(op.rc));
	}

	void FSCRRD(spu_opcode_t op) //
	{
		// Hack
		set_vr(op.rt, splat<u32[4]>(0));
	}

	void FSCRWR(spu_opcode_t op) //
	{
		// Hack
	}

	void DFCGT(spu_opcode_t op) //
	{
		return UNK(op);
	}

	void DFCEQ(spu_opcode_t op) //
	{
		return UNK(op);
	}

	void DFCMGT(spu_opcode_t op) //
	{
		set_vr(op.rt, sext<u64[2]>(fcmp<llvm::FCmpInst::FCMP_OGT>(fabs(get_vr<f64[2]>(op.ra)), fabs(get_vr<f64[2]>(op.rb)))));
	}

	void DFCMEQ(spu_opcode_t op) //
	{
		return UNK(op);
	}

	void DFTSV(spu_opcode_t op) //
	{
		return UNK(op);
	}

	void DFA(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) + get_vr<f64[2]>(op.rb));
	}

	void DFS(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) - get_vr<f64[2]>(op.rb));
	}

	void DFM(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb));
	}

	void DFMA(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb) + get_vr<f64[2]>(op.rt));
	}

	void DFMS(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb) - get_vr<f64[2]>(op.rt));
	}

	void DFNMS(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<f64[2]>(op.rt) - get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb));
	}

	void DFNMA(spu_opcode_t op) //
	{
		set_vr(op.rt, -(get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb) + get_vr<f64[2]>(op.rt)));
	}

	void FREST(spu_opcode_t op) //
	{
		set_vr(op.rt, fsplat<f32[4]>(1.0) / get_vr<f32[4]>(op.ra));
	}

	void FRSQEST(spu_opcode_t op) //
	{
		set_vr(op.rt, fsplat<f32[4]>(1.0) / sqrt(fabs(get_vr<f32[4]>(op.ra))));
	}

	void FCGT(spu_opcode_t op) //
	{
		set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OGT>(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb))));
	}

	void FCMGT(spu_opcode_t op) //
	{
		set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OGT>(fabs(get_vr<f32[4]>(op.ra)), fabs(get_vr<f32[4]>(op.rb)))));
	}

	void FA(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<f32[4]>(op.ra) + get_vr<f32[4]>(op.rb));
	}

	void FS(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<f32[4]>(op.ra) - get_vr<f32[4]>(op.rb));
	}

	void FM(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb));
	}

	void FESD(spu_opcode_t op) //
	{
		value_t<f64[2]> r;
		r.value = m_ir->CreateFPExt(shuffle2<f32[2]>(get_vr<f32[4]>(op.ra), fsplat<f32[4]>(0.), 1, 3).value, get_type<f64[2]>());
		set_vr(op.rt, r);
	}

	void FRDS(spu_opcode_t op) //
	{
		value_t<f32[2]> r;
		r.value = m_ir->CreateFPTrunc(get_vr<f64[2]>(op.ra).value, get_type<f32[2]>());
		set_vr(op.rt, shuffle2<f32[4]>(r, fsplat<f32[2]>(0.), 2, 0, 3, 1));
	}

	void FCEQ(spu_opcode_t op) //
	{
		set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OEQ>(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb))));
	}

	void FCMEQ(spu_opcode_t op) //
	{
		set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OEQ>(fabs(get_vr<f32[4]>(op.ra)), fabs(get_vr<f32[4]>(op.rb)))));
	}

	void FNMS(spu_opcode_t op) //
	{
		set_vr(op.rt4, get_vr<f32[4]>(op.rc) - get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb));
	}

	void FMA(spu_opcode_t op) //
	{
		set_vr(op.rt4, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb) + get_vr<f32[4]>(op.rc));
	}

	void FMS(spu_opcode_t op) //
	{
		set_vr(op.rt4, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb) - get_vr<f32[4]>(op.rc));
	}

	void FI(spu_opcode_t op) //
	{
		set_vr(op.rt, get_vr<f32[4]>(op.rb));
	}

	void CFLTS(spu_opcode_t op) //
	{
		value_t<f32[4]> a = get_vr<f32[4]>(op.ra);
		if (op.i8 != 173)
			a = eval(a * fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))));

		value_t<s32[4]> r;
		r.value = m_ir->CreateFPToSI(a.value, get_type<s32[4]>());
		set_vr(op.rt, r ^ sext<s32[4]>(fcmp<llvm::FCmpInst::FCMP_OGE>(a, fsplat<f32[4]>(std::exp2(31.f)))));
	}

	void CFLTU(spu_opcode_t op) //
	{
		value_t<f32[4]> a = get_vr<f32[4]>(op.ra);
		if (op.i8 != 173)
			a = eval(a * fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))));

		value_t<s32[4]> r;
		r.value = m_ir->CreateFPToUI(a.value, get_type<s32[4]>());
		set_vr(op.rt, r & ~(bitcast<s32[4]>(a) >> 31));
	}

	void CSFLT(spu_opcode_t op) //
	{
		value_t<f32[4]> r;
		r.value = m_ir->CreateSIToFP(get_vr<s32[4]>(op.ra).value, get_type<f32[4]>());
		if (op.i8 != 155)
			r = eval(r * fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))));
		set_vr(op.rt, r);
	}

	void CUFLT(spu_opcode_t op) //
	{
		value_t<f32[4]> r;
		r.value = m_ir->CreateUIToFP(get_vr<s32[4]>(op.ra).value, get_type<f32[4]>());
		if (op.i8 != 155)
			r = eval(r * fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))));
		set_vr(op.rt, r);
	}

	void STQX(spu_opcode_t op) //
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) & 0x3fff0);
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
	}

	void LQX(spu_opcode_t op) //
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) & 0x3fff0);
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void STQA(spu_opcode_t op) //
	{
		value_t<u64> addr = splat<u64>(spu_ls_target(0, op.i16));
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
	}

	void LQA(spu_opcode_t op) //
	{
		value_t<u64> addr = splat<u64>(spu_ls_target(0, op.i16));
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void STQR(spu_opcode_t op) //
	{
		value_t<u64> addr = splat<u64>(spu_ls_target(m_pos, op.i16));
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
	}

	void LQR(spu_opcode_t op) //
	{
		value_t<u64> addr = splat<u64>(spu_ls_target(m_pos, op.i16));
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void STQD(spu_opcode_t op) //
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + (op.si10 << 4)) & 0x3fff0);
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
	}

	void LQD(spu_opcode_t op) //
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + (op.si10 << 4)) & 0x3fff0);
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void make_halt(value_t<bool> cond)
	{
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto halt = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, halt, next);
		m_ir->SetInsertPoint(halt);
		const auto pstatus = spu_ptr<u32>(&SPUThread::status);
		const auto chalt = m_ir->getInt32(SPU_STATUS_STOPPED_BY_HALT);
		m_ir->CreateAtomicRMW(llvm::AtomicRMWInst::Or, pstatus, chalt, llvm::AtomicOrdering::Release)->setVolatile(true);
		const auto ptr = m_ir->CreateIntToPtr(m_ir->getInt64(reinterpret_cast<u64>(vm::base(0xffdead00))), get_type<u32*>());
		m_ir->CreateStore(m_ir->getInt32("HALT"_u32), ptr)->setVolatile(true);
		m_ir->CreateBr(next);
		m_ir->SetInsertPoint(next);
	}

	void HGT(spu_opcode_t op) //
	{
		const auto cond = eval(extract(get_vr<s32[4]>(op.ra), 3) > extract(get_vr<s32[4]>(op.rb), 3));
		make_halt(cond);
	}

	void HEQ(spu_opcode_t op) //
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) == extract(get_vr(op.rb), 3));
		make_halt(cond);
	}

	void HLGT(spu_opcode_t op) //
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) > extract(get_vr(op.rb), 3));
		make_halt(cond);
	}

	void HGTI(spu_opcode_t op) //
	{
		const auto cond = eval(extract(get_vr<s32[4]>(op.ra), 3) > op.si10);
		make_halt(cond);
	}

	void HEQI(spu_opcode_t op) //
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) == op.si10);
		make_halt(cond);
	}

	void HLGTI(spu_opcode_t op) //
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) > op.si10);
		make_halt(cond);
	}

	void HBR(spu_opcode_t op) //
	{
		// TODO: use the hint.
	}

	void HBRA(spu_opcode_t op) //
	{
		// TODO: use the hint.
	}

	void HBRR(spu_opcode_t op) //
	{
		// TODO: use the hint.
	}

	// TODO
	static u32 exec_check_interrupts(SPUThread* _spu, u32 addr)
	{
		_spu->set_interrupt_status(true);

		if ((_spu->ch_event_mask & _spu->ch_event_stat & SPU_EVENT_INTR_IMPLEMENTED) > 0)
		{
			_spu->interrupts_enabled = false;
			_spu->srr0 = addr;

			// Test for BR/BRA instructions (they are equivalent at zero pc)
			const u32 br = _spu->_ref<const u32>(0);

			if ((br & 0xfd80007f) == 0x30000000)
			{
				return (br >> 5) & 0x3fffc;
			}

			return 0;
		}

		return addr;
	}

	llvm::BasicBlock* add_block_indirect(spu_opcode_t op, value_t<u32> addr, bool ret = true)
	{
		// Convert an indirect branch into a static one if possible
		if (const auto _int = llvm::dyn_cast<llvm::ConstantInt>(addr.value))
		{
			const u32 target = ::narrow<u32>(_int->getZExtValue(), HERE);

			LOG_WARNING(SPU, "[0x%x] Fixed branch to 0x%x", m_pos, target);

			if (!op.e && !op.d)
			{
				return add_block(target);
			}

			if (!m_entry_info[target / 4])
			{
				LOG_ERROR(SPU, "[0x%x] Fixed branch to 0x%x", m_pos, target);
			}
			else
			{
				add_function(target);
			}

			// Fixed branch excludes the possibility it's a function return (TODO)
			ret = false;
		}
		else if (llvm::isa<llvm::Constant>(addr.value))
		{
			LOG_ERROR(SPU, "[0x%x] Unexpected constant (add_block_indirect)", m_pos);
		}

		// Load stack addr if necessary
		value_t<u32> sp;

		if (ret && g_cfg.core.spu_block_size != spu_block_size_type::safe)
		{
			sp = eval(extract(get_vr(1), 3) & 0x3fff0);
		}

		const auto cblock = m_ir->GetInsertBlock();
		const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->SetInsertPoint(result);

		if (op.e)
		{
			addr.value = call(&exec_check_interrupts, m_thread, addr.value);
		}

		if (op.d)
		{
			m_ir->CreateStore(m_ir->getFalse(), spu_ptr<bool>(&SPUThread::interrupts_enabled))->setVolatile(true);
		}

		m_ir->CreateStore(addr.value, spu_ptr<u32>(&SPUThread::pc));
		const auto type = llvm::FunctionType::get(get_type<void>(), {get_type<u8*>(), get_type<u8*>(), get_type<u32>()}, false)->getPointerTo()->getPointerTo();
		const auto disp = m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, m_ir->getInt64(::offset32(&SPUThread::jit_dispatcher))), type);
		const auto ad64 = m_ir->CreateZExt(addr.value, get_type<u64>());

		if (ret && g_cfg.core.spu_block_size != spu_block_size_type::safe)
		{
			// Compare address stored in stack mirror with addr
			const auto stack0 = eval(zext<u64>(sp) + ::offset32(&SPUThread::stack_mirror));
			const auto stack1 = eval(stack0 + 8);
			const auto _ret = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack0.value), type));
			const auto link = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack1.value), get_type<u64*>()));
			const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto done = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpEQ(ad64, link), done, fail);
			m_ir->SetInsertPoint(done);

			// Clear stack mirror and return by tail call to the provided return address
			m_ir->CreateStore(splat<u64[2]>(-1).value, m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack0.value), get_type<u64(*)[2]>()));
			tail(_ret);
			m_ir->SetInsertPoint(fail);
		}

		llvm::Value* ptr = m_ir->CreateGEP(disp, m_ir->CreateLShr(ad64, 2, "", true));

		if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
		{
			// Try to load chunk address from the function table
			const auto use_ftable = m_ir->CreateICmpULT(ad64, m_ir->getInt64(m_size));
			ptr = m_ir->CreateSelect(use_ftable, m_ir->CreateGEP(m_function_table, {m_ir->getInt64(0), m_ir->CreateLShr(ad64, 2, "", true)}), ptr);
		}

		tail(m_ir->CreateLoad(ptr));
		m_ir->SetInsertPoint(cblock);
		return result;
	}

	void BIZ(spu_opcode_t op) //
	{
		m_block->block_end = m_ir->GetInsertBlock();
		const auto cond = eval(extract(get_vr(op.rt), 3) == 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block(m_pos + 4));
	}

	void BINZ(spu_opcode_t op) //
	{
		m_block->block_end = m_ir->GetInsertBlock();
		const auto cond = eval(extract(get_vr(op.rt), 3) != 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block(m_pos + 4));
	}

	void BIHZ(spu_opcode_t op) //
	{
		m_block->block_end = m_ir->GetInsertBlock();
		const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) == 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block(m_pos + 4));
	}

	void BIHNZ(spu_opcode_t op) //
	{
		m_block->block_end = m_ir->GetInsertBlock();
		const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) != 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block(m_pos + 4));
	}

	void BI(spu_opcode_t op) //
	{
		m_block->block_end = m_ir->GetInsertBlock();
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);

		// Create jump table if necessary (TODO)
		const auto tfound = m_targets.find(m_pos);

		if (!op.d && !op.e && tfound != m_targets.end() && tfound->second.size())
		{
			// Shift aligned address for switch
			const auto sw_arg = m_ir->CreateLShr(addr.value, 2, "", true);

			// Initialize jump table targets
			std::map<u32, llvm::BasicBlock*> targets;

			for (u32 target : tfound->second)
			{
				if (m_block_info[target / 4])
				{
					targets.emplace(target, nullptr);
				}
			}

			// Initialize target basic blocks
			for (auto& pair : targets)
			{
				pair.second = add_block(pair.first);
			}

			// Get jump table bounds (optimization)
			const u32 start = targets.begin()->first;
			const u32 end = targets.rbegin()->first + 4;

			// Emit switch instruction aiming for a jumptable in the end (indirectbr could guarantee it)
			const auto sw = m_ir->CreateSwitch(sw_arg, llvm::BasicBlock::Create(m_context, "", m_function), (end - start) / 4);

			for (u32 pos = start; pos < end; pos += 4)
			{
				if (m_block_info[pos / 4] && targets.count(pos))
				{
					const auto found = targets.find(pos);

					if (found != targets.end())
					{
						sw->addCase(m_ir->getInt32(pos / 4), found->second);
						continue;
					}
				}

				sw->addCase(m_ir->getInt32(pos / 4), sw->getDefaultDest());
			}

			// Exit function on unexpected target
			m_ir->SetInsertPoint(sw->getDefaultDest());
			m_ir->CreateStore(addr.value, spu_ptr<u32>(&SPUThread::pc));
			m_ir->CreateRetVoid();
		}
		else
		{
			// Simple indirect branch
			m_ir->CreateBr(add_block_indirect(op, addr));
		}
	}

	void BISL(spu_opcode_t op) //
	{
		m_block->block_end = m_ir->GetInsertBlock();
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		set_link(op);
		m_ir->CreateBr(add_block_indirect(op, addr, false));
	}

	void IRET(spu_opcode_t op) //
	{
		m_block->block_end = m_ir->GetInsertBlock();
		value_t<u32> srr0;
		srr0.value = m_ir->CreateLoad(spu_ptr<u32>(&SPUThread::srr0));
		m_ir->CreateBr(add_block_indirect(op, srr0));
	}

	void BISLED(spu_opcode_t op) //
	{
		UNK(op);
	}

	void BRZ(spu_opcode_t op) //
	{
		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto cond = eval(extract(get_vr(op.rt), 3) == 0);
			m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
		}
	}

	void BRNZ(spu_opcode_t op) //
	{
		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto cond = eval(extract(get_vr(op.rt), 3) != 0);
			m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
		}
	}

	void BRHZ(spu_opcode_t op) //
	{
		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) == 0);
			m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
		}
	}

	void BRHNZ(spu_opcode_t op) //
	{
		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) != 0);
			m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
		}
	}

	void BRA(spu_opcode_t op) //
	{
		const u32 target = spu_branch_target(0, op.i16);

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			m_ir->CreateBr(add_block(target));
		}
	}

	void BRASL(spu_opcode_t op) //
	{
		set_link(op);
		BRA(op);
	}

	void BR(spu_opcode_t op) //
	{
		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			m_ir->CreateBr(add_block(target));
		}
	}

	void BRSL(spu_opcode_t op) //
	{
		set_link(op);
		BR(op);
	}

	void set_link(spu_opcode_t op)
	{
		set_vr(op.rt, build<u32[4]>(0, 0, 0, spu_branch_target(m_pos + 4)));

		if (g_cfg.core.spu_block_size != spu_block_size_type::safe && m_block_info[m_pos / 4 + 1] && m_entry_info[m_pos / 4 + 1])
		{
			// Store the return function chunk address at the stack mirror
			const auto func = add_function(m_pos + 4);
			const auto stack0 = eval(zext<u64>(extract(get_vr(1), 3) & 0x3fff0) + ::offset32(&SPUThread::stack_mirror));
			const auto stack1 = eval(stack0 + 8);
			m_ir->CreateStore(func, m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack0.value), func->getType()->getPointerTo()));
			m_ir->CreateStore(m_ir->getInt64(m_pos + 4), m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack1.value), get_type<u64*>()));
		}
	}

	static const spu_decoder<spu_llvm_recompiler> g_decoder;
};

std::unique_ptr<spu_recompiler_base> spu_recompiler_base::make_llvm_recompiler()
{
	return std::make_unique<spu_llvm_recompiler>();
}

DECLARE(spu_llvm_recompiler::g_decoder);

#else

std::unique_ptr<spu_recompiler_base> spu_recompiler_base::make_llvm_recompiler()
{
	fmt::throw_exception("LLVM is not available in this build.");
}

#endif
