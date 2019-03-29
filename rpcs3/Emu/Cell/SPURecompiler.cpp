#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/vm.h"
#include "Crypto/sha1.h"
#include "Utilities/StrUtil.h"

#include "SPUThread.h"
#include "SPUAnalyser.h"
#include "SPUInterpreter.h"
#include "SPUDisAsm.h"
#include "SPURecompiler.h"
#include <algorithm>
#include <mutex>
#include <thread>

extern atomic_t<const char*> g_progr;
extern atomic_t<u64> g_progr_ptotal;
extern atomic_t<u64> g_progr_pdone;

const spu_decoder<spu_itype> s_spu_itype;
const spu_decoder<spu_iname> s_spu_iname;

extern u64 get_timebased_time();

thread_local DECLARE(spu_runtime::workload){};

thread_local DECLARE(spu_runtime::addrv){u32{0}};

DECLARE(spu_runtime::tr_dispatch) = []
{
	// Generate a special trampoline to spu_recompiler_base::dispatch with pause instruction
	u8* const trptr = jit_runtime::alloc(16, 16);
	trptr[0] = 0xf3; // pause
	trptr[1] = 0x90;
	trptr[2] = 0xff; // jmp [rip]
	trptr[3] = 0x25;
	std::memset(trptr + 4, 0, 4);
	const u64 target = reinterpret_cast<u64>(&spu_recompiler_base::dispatch);
	std::memcpy(trptr + 8, &target, 8);
	return reinterpret_cast<spu_function_t>(trptr);
}();

DECLARE(spu_runtime::tr_branch) = []
{
	// Generate a trampoline to spu_recompiler_base::branch
	u8* const trptr = jit_runtime::alloc(16, 16);
	trptr[0] = 0xff; // jmp [rip]
	trptr[1] = 0x25;
	std::memset(trptr + 2, 0, 4);
	const u64 target = reinterpret_cast<u64>(&spu_recompiler_base::branch);
	std::memcpy(trptr + 6, &target, 8);
	return reinterpret_cast<spu_function_t>(trptr);
}();

DECLARE(spu_runtime::g_dispatcher) = []
{
	const auto ptr = reinterpret_cast<decltype(spu_runtime::g_dispatcher)>(jit_runtime::alloc(0x10000 * sizeof(void*), 8, false));

	// Initialize lookup table
	for (u32 i = 0; i < 0x10000; i++)
	{
		ptr[i].raw() = &spu_recompiler_base::dispatch;
	}

	return ptr;
}();

DECLARE(spu_runtime::g_interpreter) = nullptr;

spu_cache::spu_cache(const std::string& loc)
	: m_file(loc, fs::read + fs::write + fs::create + fs::append)
{
}

spu_cache::~spu_cache()
{
}

std::deque<std::vector<u32>> spu_cache::get()
{
	std::deque<std::vector<u32>> result;

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

		result.emplace_front(std::move(func));
	}

	return result;
}

void spu_cache::add(const std::vector<u32>& func)
{
	if (!m_file)
	{
		return;
	}

	// Allocate buffer
	const auto buf = std::make_unique<be_t<u32>[]>(func.size() + 1);

	buf[0] = ::size32(func) - 1;
	buf[1] = func[0];
	std::memcpy(buf.get() + 2, func.data() + 1, func.size() * 4 - 4);

	// Append data
	m_file.write(buf.get(), func.size() * 4 + 4);
}

void spu_cache::initialize()
{
	spu_runtime::g_interpreter = nullptr;

	const std::string ppu_cache = Emu.PPUCache();

	if (ppu_cache.empty())
	{
		return;
	}

	// SPU cache file (version + block size type)
	const std::string loc = ppu_cache + "spu-" + fmt::to_lower(g_cfg.core.spu_block_size.to_string()) + "-v1-tane.dat";

	auto cache = std::make_shared<spu_cache>(loc);

	if (!*cache)
	{
		LOG_ERROR(SPU, "Failed to initialize SPU cache at: %s", loc);
		return;
	}

	// Read cache
	auto func_list = cache->get();
	atomic_t<std::size_t> fnext{};
	atomic_t<u8> fail_flag{0};

	// Initialize compiler instances for parallel compilation
	u32 max_threads = static_cast<u32>(g_cfg.core.llvm_threads);
	u32 thread_count = max_threads > 0 ? std::min(max_threads, std::thread::hardware_concurrency()) : std::thread::hardware_concurrency();
	std::vector<std::unique_ptr<spu_recompiler_base>> compilers{thread_count};

	if (g_cfg.core.spu_decoder == spu_decoder_type::fast)
	{
		if (auto compiler = spu_recompiler_base::make_llvm_recompiler(11))
		{
			compiler->init();

			if (compiler->compile(0, {}) && spu_runtime::g_interpreter)
			{
				LOG_SUCCESS(SPU, "SPU Runtime: built interpreter.");
				return;
			}
		}
	}

	for (auto& compiler : compilers)
	{
		if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit)
		{
			compiler = spu_recompiler_base::make_asmjit_recompiler();
		}
		else if (g_cfg.core.spu_decoder == spu_decoder_type::llvm)
		{
			compiler = spu_recompiler_base::make_llvm_recompiler();
		}
		else
		{
			compilers.clear();
			break;
		}

		compiler->init();
	}

	if (compilers.size() && !func_list.empty())
	{
		// Initialize progress dialog (wait for previous progress done)
		while (g_progr_ptotal)
		{
			std::this_thread::sleep_for(5ms);
		}

		g_progr = "Building SPU cache...";
		g_progr_ptotal += func_list.size();
	}

	std::deque<named_thread<std::function<void()>>> thread_queue;

	for (std::size_t i = 0; i < compilers.size(); i++) thread_queue.emplace_back("Worker " + std::to_string(i), [&, compiler = compilers[i].get()]()
	{
		// Register SPU runtime user
		spu_runtime::passive_lock _passive_lock(compiler->get_runtime());

		// Fake LS
		std::vector<be_t<u32>> ls(0x10000);

		// Build functions
		for (std::size_t func_i = fnext++; func_i < func_list.size(); func_i = fnext++)
		{
			std::vector<u32>& func = func_list[func_i];

			if (Emu.IsStopped() || fail_flag)
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
			const std::vector<u32>& func2 = compiler->analyse(ls.data(), func[0]);

			if (func2.size() != size0)
			{
				LOG_ERROR(SPU, "[0x%05x] SPU Analyser failed, %u vs %u", func2[0], func2.size() - 1, size0 - 1);
			}

			if (!compiler->compile(0, func))
			{
				// Likely, out of JIT memory. Signal to prevent further building.
				fail_flag |= 1;
			}

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
	});

	// Join all threads
	while (!thread_queue.empty())
	{
		thread_queue.pop_front();
	}

	if (Emu.IsStopped())
	{
		LOG_ERROR(SPU, "SPU Runtime: Cache building aborted.");
		return;
	}

	if (fail_flag)
	{
		LOG_ERROR(SPU, "SPU Runtime: Cache building failed (too much data). SPU Cache will be disabled.");
		spu_runtime::passive_lock _passive_lock(compilers[0]->get_runtime());
		compilers[0]->get_runtime().reset(0);
		return;
	}

	if (compilers.size() && !func_list.empty())
	{
		LOG_SUCCESS(SPU, "SPU Runtime: Built %u functions.", func_list.size());
	}

	// Register cache instance
	fxm::import<spu_cache>([&]() -> std::shared_ptr<spu_cache>&&
	{
		return std::move(cache);
	});
}

spu_runtime::spu_runtime()
{
	// Initialize "empty" block
	m_map[std::vector<u32>()] = &spu_recompiler_base::dispatch;

	// Clear LLVM output
	m_cache_path = Emu.PPUCache();
	fs::create_dir(m_cache_path + "llvm/");
	fs::remove_all(m_cache_path + "llvm/", false);

	if (g_cfg.core.spu_debug)
	{
		fs::file(m_cache_path + "spu.log", fs::rewrite);
	}

	workload.reserve(250);

	LOG_SUCCESS(SPU, "SPU Recompiler Runtime initialized...");
}

bool spu_runtime::add(u64 last_reset_count, void* _where, spu_function_t compiled)
{
	writer_lock lock(*this);

	// Check reset count (makes where invalid)
	if (!_where || last_reset_count != m_reset_count)
	{
		return false;
	}

	// Use opaque pointer
	auto& where = *static_cast<decltype(m_map)::value_type*>(_where);

	// Function info
	const std::vector<u32>& func = where.first;

	//
	const u32 start = func[0] * (g_cfg.core.spu_block_size != spu_block_size_type::giga);

	// Set pointer to the compiled function
	where.second = compiled;

	// Generate a dispatcher (übertrampoline)
	addrv[0] = func[0];
	const auto beg = m_map.lower_bound(addrv);
	addrv[0] += 4;
	const auto _end = m_map.lower_bound(addrv);
	const u32 size0 = std::distance(beg, _end);

	if (size0 == 1)
	{
		g_dispatcher[func[0] / 4] = compiled;
	}
	else
	{
		// Allocate some writable executable memory
		u8* const wxptr = jit_runtime::alloc(size0 * 20, 16);

		if (!wxptr)
		{
			return false;
		}

		// Raw assembly pointer
		u8* raw = wxptr;

		// Write jump instruction with rel32 immediate
		auto make_jump = [&](u8 op, auto target)
		{
			verify("Asm overflow" HERE), raw + 6 <= wxptr + size0 * 20;

			// Fallback to dispatch if no target
			const u64 taddr = target ? reinterpret_cast<u64>(target) : reinterpret_cast<u64>(tr_dispatch);

			// Compute the distance
			const s64 rel = taddr - reinterpret_cast<u64>(raw) - (op != 0xe9 ? 6 : 5);

			verify(HERE), rel >= INT32_MIN, rel <= INT32_MAX;

			if (op != 0xe9)
			{
				// First jcc byte
				*raw++ = 0x0f;
				verify(HERE), (op >> 4) == 0x8;
			}

			*raw++ = op;

			const s32 r32 = static_cast<s32>(rel);

			std::memcpy(raw, &r32, 4);
			raw += 4;
		};

		workload.clear();
		workload.reserve(size0);
		workload.emplace_back();
		workload.back().size  = size0;
		workload.back().level = 1;
		workload.back().from  = 0;
		workload.back().rel32 = 0;
		workload.back().beg   = beg;
		workload.back().end   = _end;

		for (std::size_t i = 0; i < workload.size(); i++)
		{
			// Get copy of the workload info
			auto w = workload[i];

			// Split range in two parts
			auto it = w.beg;
			auto it2 = w.beg;
			u32 size1 = w.size / 2;
			u32 size2 = w.size - size1;
			std::advance(it2, w.size / 2);

			while (verify("spu_runtime::work::level overflow" HERE, w.level))
			{
				it = it2;
				size1 = w.size - size2;

				if (w.level >= w.beg->first.size())
				{
					// Cannot split: smallest function is a prefix of bigger ones (TODO)
					break;
				}

				const u32 x1 = w.beg->first.at(w.level);

				if (!x1)
				{
					// Cannot split: some functions contain holes at this level
					w.level++;
					continue;
				}

				// Adjust ranges (forward)
				while (it != w.end && x1 == it->first.at(w.level))
				{
					it++;
					size1++;
				}

				if (it == w.end)
				{
					// Cannot split: words are identical within the range at this level
					w.level++;
				}
				else
				{
					size2 = w.size - size1;
					break;
				}
			}

			if (w.rel32)
			{
				// Patch rel32 linking it to the current location if necessary
				const s32 r32 = ::narrow<s32>(raw - w.rel32, HERE);
				std::memcpy(w.rel32 - 4, &r32, 4);
			}

			if (w.level >= w.beg->first.size())
			{
				// If functions cannot be compared, assume smallest function
				LOG_ERROR(SPU, "Trampoline simplified at 0x%x (level=%u)", func[0], w.level);
				make_jump(0xe9, w.beg->second); // jmp rel32
				continue;
			}

			// Value for comparison
			const u32 x = it->first.at(w.level);

			// Adjust ranges (backward)
			while (true)
			{
				it--;

				if (it->first.at(w.level) != x)
				{
					it++;
					break;
				}

				verify(HERE), it != w.beg;
				size1--;
				size2++;
			}

			// Emit 32-bit comparison: cmp [ls+addr], imm32
			verify("Asm overflow" HERE), raw + 11 <= wxptr + size0 * 20;

			if (w.from != w.level)
			{
				// If necessary (level has advanced), emit load: mov eax, [ls + addr]
#ifdef _WIN32
				*raw++ = 0x8b;
				*raw++ = 0x82; // ls = rdx
#else
				*raw++ = 0x8b;
				*raw++ = 0x86; // ls = rsi
#endif
				const u32 cmp_lsa = start + (w.level - 1) * 4;
				std::memcpy(raw, &cmp_lsa, 4);
				raw += 4;
			}

			// Emit comparison: cmp eax, imm32
			*raw++ = 0x3d;
			std::memcpy(raw, &x, 4);
			raw += 4;

			// Low subrange target
			if (size1 == 1)
			{
				make_jump(0x82, w.beg->second); // jb rel32
			}
			else
			{
				make_jump(0x82, raw); // jb rel32 (stub)
				auto& to = workload.emplace_back(w);
				to.end   = it;
				to.size  = size1;
				to.rel32 = raw;
				to.from  = w.level;
			}

			// Second subrange target
			if (size2 == 1)
			{
				make_jump(0xe9, it->second); // jmp rel32
			}
			else
			{
				it2 = it;

				// Select additional midrange for equality comparison
				while (it2 != w.end && it2->first.at(w.level) == x)
				{
					size2--;
					it2++;
				}

				if (it2 != w.end)
				{
					// High subrange target
					if (size2 == 1)
					{
						make_jump(0x87, it2->second); // ja rel32
					}
					else
					{
						make_jump(0x87, raw); // ja rel32 (stub)
						auto& to = workload.emplace_back(w);
						to.beg   = it2;
						to.size  = size2;
						to.rel32 = raw;
						to.from  = w.level;
					}

					const u32 size3 = w.size - size1 - size2;

					if (size3 == 1)
					{
						make_jump(0xe9, it->second); // jmp rel32
					}
					else
					{
						make_jump(0xe9, raw); // jmp rel32 (stub)
						auto& to = workload.emplace_back(w);
						to.beg   = it;
						to.end   = it2;
						to.size  = size3;
						to.rel32 = raw;
						to.from  = w.level;
					}
				}
				else
				{
					make_jump(0xe9, raw); // jmp rel32 (stub)
					auto& to = workload.emplace_back(w);
					to.beg   = it;
					to.size  = w.size - size1;
					to.rel32 = raw;
					to.from  = w.level;
				}
			}
		}

		workload.clear();
		g_dispatcher[func[0] / 4] = reinterpret_cast<spu_function_t>(reinterpret_cast<u64>(wxptr));
	}

	// Notify in lock destructor
	lock.notify = true;
	return true;
}

void* spu_runtime::find(u64 last_reset_count, const std::vector<u32>& func)
{
	writer_lock lock(*this);

	// Check reset count
	if (last_reset_count != m_reset_count)
	{
		return nullptr;
	}

	// Try to find existing function, register new one if necessary
	const auto result = m_map.try_emplace(func, nullptr);

	// Pointer to the value in the map (pair)
	const auto fn_location = &*result.first;

	if (fn_location->second)
	{
		// Already compiled
		return g_dispatcher;
	}
	else if (!result.second)
	{
		// Wait if already in progress
		while (!fn_location->second)
		{
			m_cond.wait(m_mutex);

			// If reset count changed, fn_location is invalidated; also requires return
			if (last_reset_count != m_reset_count)
			{
				return nullptr;
			}
		}

		return g_dispatcher;
	}

	// Return location to compile and use in add()
	return fn_location;
}

spu_function_t spu_runtime::find(const se_t<u32, false>* ls, u32 addr) const
{
	const u64 reset_count = m_reset_count;

	reader_lock lock(*this);

	if (reset_count != m_reset_count)
	{
		return nullptr;
	}

	const u32 start = addr * (g_cfg.core.spu_block_size != spu_block_size_type::giga);

	addrv[0] = addr;
	const auto beg = m_map.lower_bound(addrv);
	addrv[0] += 4;
	const auto _end = m_map.lower_bound(addrv);

	for (auto it = beg; it != _end; ++it)
	{
		bool bad = false;

		for (u32 i = 1; i < it->first.size(); ++i)
		{
			const u32 x = it->first[i];
			const u32 y = ls[start / 4 + i - 1];

			if (x && x != y)
			{
				bad = true;
				break;
			}
		}

		if (!bad)
		{
			return it->second;
		}
	}

	return nullptr;
}

spu_function_t spu_runtime::make_branch_patchpoint(u32 target) const
{
	u8* const raw = jit_runtime::alloc(16, 16);

	if (!raw)
	{
		return nullptr;
	}

	// Save address of the following jmp
#ifdef _WIN32
	raw[0] = 0x4c; // lea r8, [rip+1]
	raw[1] = 0x8d;
	raw[2] = 0x05;
#else
	raw[0] = 0x48; // lea rdx, [rip+1]
	raw[1] = 0x8d;
	raw[2] = 0x15;
#endif
	raw[3] = 0x01;
	raw[4] = 0x00;
	raw[5] = 0x00;
	raw[6] = 0x00;
	raw[7] = 0x90; // nop

	// Jump to spu_recompiler_base::branch
	raw[8] = 0xe9;
	// Compute the distance
	const s64 rel = reinterpret_cast<u64>(tr_branch) - reinterpret_cast<u64>(raw + 8) - 5;
	std::memcpy(raw + 9, &rel, 4);
	raw[13] = 0xcc;

	// Write compressed target address
	raw[14] = target >> 2;
	raw[15] = target >> 10;

	return reinterpret_cast<spu_function_t>(raw);
}

u64 spu_runtime::reset(std::size_t last_reset_count)
{
	writer_lock lock(*this);

	if (last_reset_count != m_reset_count || !m_reset_count.compare_and_swap_test(last_reset_count, last_reset_count + 1))
	{
		// Probably already reset
		return m_reset_count;
	}

	// Notify SPU threads
	idm::select<named_thread<spu_thread>>([](u32, cpu_thread& cpu)
	{
		if (!cpu.state.test_and_set(cpu_flag::jit_return))
		{
			cpu.notify();
		}
	});

	// Reset function map (may take some time)
	m_map.clear();

	// Wait for threads to catch on jit_return flag
	while (m_passive_locks)
	{
		busy_wait();
	}

	// Reinitialize (TODO)
	jit_runtime::finalize();
	jit_runtime::initialize();
	return ++m_reset_count;
}

void spu_runtime::handle_return(spu_thread* _spu)
{
	// Wait until the runtime becomes available
	writer_lock lock(*this);

	// Reset stack mirror
	std::memset(_spu->stack_mirror.data(), 0xff, sizeof(spu_thread::stack_mirror));

	// Reset the flag
	_spu->state -= cpu_flag::jit_return;
}

spu_recompiler_base::spu_recompiler_base()
{
	result.reserve(8192);
}

spu_recompiler_base::~spu_recompiler_base()
{
}

void spu_recompiler_base::make_function(const std::vector<u32>& data)
{
	for (u64 reset_count = m_spurt->get_reset_count();;)
	{
		if (LIKELY(compile(reset_count, data)))
		{
			break;
		}

		reset_count = m_spurt->reset(reset_count);
	}
}

void spu_recompiler_base::dispatch(spu_thread& spu, void*, u8* rip)
{
	// If code verification failed from a patched patchpoint, clear it with a dispatcher jump
	if (rip)
	{
		const u32 target = *(u16*)(rip + 6) * 4;
		const s64 rel = reinterpret_cast<u64>(spu_runtime::g_dispatcher) + 2 * target - reinterpret_cast<u64>(rip - 8) - 6;

		union
		{
			u8 bytes[8];
			u64 result;
		};

		bytes[0] = 0xff; // jmp [rip + 0x...]
		bytes[1] = 0x25;
		std::memcpy(bytes + 2, &rel, 4);
		bytes[6] = 0x90;
		bytes[7] = 0x90;

		atomic_storage<u64>::release(*reinterpret_cast<u64*>(rip - 8), result);
	}

	// Second attempt (recover from the recursion after repeated unsuccessful trampoline call)
	if (spu.block_counter != spu.block_recover && &dispatch != spu_runtime::g_dispatcher[spu.pc / 4])
	{
		spu.block_recover = spu.block_counter;
		return;
	}

	// Compile
	spu.jit->make_function(spu.jit->analyse(spu._ptr<u32>(0), spu.pc));

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

void spu_recompiler_base::branch(spu_thread& spu, void*, u8* rip)
{
	// Find function
	const auto func = spu.jit->get_runtime().find(spu._ptr<se_t<u32, false>>(0), *(u16*)(rip + 6) * 4);

	if (!func)
	{
		return;
	}

	// Overwrite jump to this function with jump to the compiled function
	const s64 rel = reinterpret_cast<u64>(func) - reinterpret_cast<u64>(rip) - 5;

	union
	{
		u8 bytes[8];
		u64 result;
	};

	if (rel >= INT32_MIN && rel <= INT32_MAX)
	{
		const s64 rel8 = (rel + 5) - 2;

		if (rel8 >= INT8_MIN && rel8 <= INT8_MAX)
		{
			bytes[0] = 0xeb; // jmp rel8
			bytes[1] = static_cast<s8>(rel8);
			std::memset(bytes + 2, 0xcc, 4);
		}
		else
		{
			bytes[0] = 0xe9; // jmp rel32
			std::memcpy(bytes + 1, &rel, 4);
			bytes[5] = 0xcc;
		}

		// Preserve target address
		bytes[6] = rip[6];
		bytes[7] = rip[7];
	}
	else
	{
		fmt::throw_exception("Impossible far jump: %p -> %p", rip, func);
	}

	atomic_storage<u64>::release(*reinterpret_cast<u64*>(rip), result);
}

const std::vector<u32>& spu_recompiler_base::analyse(const be_t<u32>* ls, u32 entry_point)
{
	// Result: addr + raw instruction data
	result.clear();
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
			if (target >= lsa && target < limit)
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
		case spu_itype::DFCMGT:
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

			if (af & vf::is_const)
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
		case spu_itype::FSCRWR:
		case spu_itype::STQA:
		case spu_itype::STQD:
		case spu_itype::STQR:
		case spu_itype::STQX:
		{
			// Do nothing
			break;
		}

		case spu_itype::WRCH:
		{
			switch (op.ra)
			{
			case MFC_EAL:
			{
				m_regmod[pos / 4] = s_reg_mfc_eal;
				break;
			}
			case MFC_LSA:
			{
				m_regmod[pos / 4] = s_reg_mfc_lsa;
				break;
			}
			case MFC_TagID:
			{
				m_regmod[pos / 4] = s_reg_mfc_tag;
				break;
			}
			case MFC_Size:
			{
				m_regmod[pos / 4] = s_reg_mfc_size;
				break;
			}
			}

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
			hbr_tg  = vflags[op.ra] & vf::is_const && !op.c ? values[op.ra] & 0x3fffc : -1;
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: IL -> 0x%x", pos, values[1]);
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: ILA -> 0x%x", pos, values[1]);
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: ILH -> 0x%x", pos, values[1]);
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: ILHU -> 0x%x", pos, values[1]);
			}

			break;
		}
		case spu_itype::IOHL:
		{
			m_regmod[pos / 4] = op.rt;
			values[op.rt] = values[op.rt] | op.i16;

			if (op.rt == 1 && op.i16 % 16)
			{
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: IOHL, 0x%x", pos, op.i16);
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: OR", pos);
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: ANDI", pos);
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: AND", pos);
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: AI, 0x%x", pos, op.si10 + 0u);
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

					if (vflags[r2] & vf::is_const && (values[r2] % 16) == 0)
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: SFI", pos);
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: SF", pos);
			}

			break;
		}
		case spu_itype::ROTMI:
		{
			m_regmod[pos / 4] = op.rt;

			if (op.rt == 1)
			{
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: ROTMI", pos);
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: SHLI", pos);
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
				LOG_WARNING(SPU, "[0x%x] Unexpected instruction on $SP: %s", pos, s_spu_iname.decode(data));
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

class spu_llvm_recompiler : public spu_recompiler_base, public cpu_translator
{
	// JIT Instance
	jit_compiler m_jit{{}, jit_compiler::cpu(g_cfg.core.llvm_cpu)};

	// Interpreter table size power
	const u8 m_interp_magn;

	// Constant opcode bits
	u32 m_op_const_mask = -1;

	// Current function chunk entry point
	u32 m_entry;

	// Current function (chunk)
	llvm::Function* m_function;

	llvm::Value* m_thread;
	llvm::Value* m_lsptr;
	llvm::Value* m_interp_op;
	llvm::Value* m_interp_pc;
	llvm::Value* m_interp_table;
	llvm::Value* m_interp_7f0;
	llvm::Value* m_interp_regs;

	// Helpers
	llvm::Value* m_interp_pc_next;
	llvm::BasicBlock* m_interp_bblock;

	// i8*, contains constant vm::g_base_addr value
	llvm::Value* m_memptr;

	// Pointers to registers in the thread context
	std::array<llvm::Value*, s_reg_max> m_reg_addr;

	// Global variable (function table)
	llvm::GlobalVariable* m_function_table{};

	// Helpers (interpreter)
	llvm::GlobalVariable* m_scale_float_to{};
	llvm::GlobalVariable* m_scale_to_float{};

	llvm::MDNode* m_md_unlikely;
	llvm::MDNode* m_md_likely;

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
		llvm::Function* result = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(name, get_ftype<void, u8*, u8*, u32>()).getCallee());

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
		m_memptr = m_ir->CreateIntToPtr(m_ir->getInt64((u64)vm::g_base_addr), get_type<u8*>());
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
			m_ir->CreateStore(m_ir->getInt32(target), spu_ptr<u32>(&spu_thread::pc));
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

			// Generate a patchpoint for fixed location
			const auto cblock = m_ir->GetInsertBlock();
			const auto ppptr  = m_spurt->make_branch_patchpoint(target);
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);
			m_ir->CreateStore(m_ir->getInt32(target), spu_ptr<u32>(&spu_thread::pc));
			const auto type = llvm::FunctionType::get(get_type<void>(), {get_type<u8*>(), get_type<u8*>(), get_type<u32>()}, false)->getPointerTo();
			tail(m_ir->CreateIntToPtr(m_ir->getInt64(reinterpret_cast<u64>(ppptr ? ppptr : &spu_recompiler_base::dispatch)), type));
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
					const auto typ = phi->getType() == get_type<f64[4]>() ? get_type<f64[4]>() : get_reg_type(i);
					phi->addIncoming(get_reg_fixed(i, typ), m_block->block_end);
				}
			}
		}

		return result;
	}

	template <typename T = u8>
	llvm::Value* _ptr(llvm::Value* base, u32 offset)
	{
		const auto off = m_ir->CreateGEP(base, m_ir->getInt64(offset));
		const auto ptr = m_ir->CreateBitCast(off, get_type<T*>());
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

	// Return default register type
	llvm::Type* get_reg_type(u32 index)
	{
		if (index < 128)
		{
			return get_type<u32[4]>();
		}

		switch (index)
		{
		case s_reg_mfc_eal:
		case s_reg_mfc_lsa:
			return get_type<u32>();
		case s_reg_mfc_tag:
			return get_type<u8>();
		case s_reg_mfc_size:
			return get_type<u16>();
		default:
			fmt::throw_exception("get_reg_type(%u): invalid register index" HERE, index);
		}
	}

	u32 get_reg_offset(u32 index)
	{
		if (index < 128)
		{
			return ::offset32(&spu_thread::gpr, index);
		}

		switch (index)
		{
		case s_reg_mfc_eal: return ::offset32(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::eal);
		case s_reg_mfc_lsa: return ::offset32(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::lsa);
		case s_reg_mfc_tag: return ::offset32(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::tag);
		case s_reg_mfc_size: return ::offset32(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::size);
		default:
			fmt::throw_exception("get_reg_offset(%u): invalid register index" HERE, index);
		}
	}

	llvm::Value* init_reg_fixed(u32 index)
	{
		if (!m_block)
		{
			return m_ir->CreateBitCast(_ptr<u8>(m_thread, get_reg_offset(index)), get_reg_type(index)->getPointerTo());
		}

		auto& ptr = m_reg_addr.at(index);

		if (!ptr)
		{
			// Save and restore current insert point if necessary
			const auto block_cur = m_ir->GetInsertBlock();

			// Emit register pointer at the beginning of the function chunk
			m_ir->SetInsertPoint(m_function->getEntryBlock().getTerminator());
			ptr = _ptr<u8>(m_thread, get_reg_offset(index));
			ptr = m_ir->CreateBitCast(ptr, get_reg_type(index)->getPointerTo());
			m_ir->SetInsertPoint(block_cur);
		}

		return ptr;
	}

	// Get pointer to the vector register (interpreter only)
	template <typename T, uint I>
	llvm::Value* init_vr(const bf_t<u32, I, 7>& index)
	{
		if (!m_interp_magn)
		{
			m_interp_7f0 = m_ir->getInt32(0x7f0);
			m_interp_regs = _ptr(m_thread, get_reg_offset(0));
		}

		// Extract reg index
		const auto isl = I >= 4 ? m_interp_op : m_ir->CreateShl(m_interp_op, u64{4 - I});
		const auto isr = I <= 4 ? m_interp_op : m_ir->CreateLShr(m_interp_op, u64{I - 4});
		const auto idx = m_ir->CreateAnd(I > 4 ? isr : isl, m_interp_7f0);

		// Pointer to the register
		return m_ir->CreateBitCast(m_ir->CreateGEP(m_interp_regs, m_ir->CreateZExt(idx, get_type<u64>())), get_type<T*>());
	}

	llvm::Value* double_as_uint64(llvm::Value* val)
	{
		if (llvm::isa<llvm::ConstantAggregateZero>(val))
		{
			return splat<u64[4]>(0).value;
		}

		if (auto cv = llvm::dyn_cast<llvm::ConstantDataVector>(val))
		{
			const f64 data[4]
			{
				cv->getElementAsDouble(0),
				cv->getElementAsDouble(1),
				cv->getElementAsDouble(2),
				cv->getElementAsDouble(3)
			};

			return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef((const u64*)(const u8*)+data, 4));
		}

		if (llvm::isa<llvm::Constant>(val))
		{
			fmt::throw_exception("[0x%x] double_as_uint64: bad constant type", m_pos);
		}

		return m_ir->CreateBitCast(val, get_type<u64[4]>());
	}

	llvm::Value* uint64_as_double(llvm::Value* val)
	{
		if (llvm::isa<llvm::ConstantAggregateZero>(val))
		{
			return fsplat<f64[4]>(0.).value;
		}

		if (auto cv = llvm::dyn_cast<llvm::ConstantDataVector>(val))
		{
			const u64 data[4]
			{
				cv->getElementAsInteger(0),
				cv->getElementAsInteger(1),
				cv->getElementAsInteger(2),
				cv->getElementAsInteger(3)
			};

			return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef((const f64*)(const u8*)+data, 4));
		}

		if (llvm::isa<llvm::Constant>(val))
		{
			fmt::throw_exception("[0x%x] uint64_as_double: bad constant type", m_pos);
		}

		return m_ir->CreateBitCast(val, get_type<f64[4]>());
	}

	llvm::Value* double_to_xfloat(llvm::Value* val)
	{
		verify("double_to_xfloat" HERE), val, val->getType() == get_type<f64[4]>();

		// Detect xfloat_to_double to avoid unnecessary ops and prevent zeroed denormals
		if (auto _bitcast = llvm::dyn_cast<llvm::CastInst>(val))
		{
			if (_bitcast->getOpcode() == llvm::Instruction::BitCast)
			{
				if (auto _select = llvm::dyn_cast<llvm::SelectInst>(_bitcast->getOperand(0)))
				{
					if (auto _icmp = llvm::dyn_cast<llvm::ICmpInst>(_select->getOperand(0)))
					{
						if (auto _and = llvm::dyn_cast<llvm::BinaryOperator>(_icmp->getOperand(0)))
						{
							if (auto _zext = llvm::dyn_cast<llvm::CastInst>(_and->getOperand(0)))
							{
								// TODO: check all details and return xfloat_to_double() arg
							}
						}
					}
				}
			}
		}

		const auto d = double_as_uint64(val);
		const auto s = m_ir->CreateAnd(m_ir->CreateLShr(d, 32), 0x80000000);
		const auto m = m_ir->CreateXor(m_ir->CreateLShr(d, 29), 0x40000000);
		const auto r = m_ir->CreateOr(m_ir->CreateAnd(m, 0x7fffffff), s);
		return m_ir->CreateTrunc(m_ir->CreateSelect(m_ir->CreateIsNotNull(d), r, splat<u64[4]>(0).value), get_type<u32[4]>());
	}

	llvm::Value* xfloat_to_double(llvm::Value* val)
	{
		verify("xfloat_to_double" HERE), val, val->getType() == get_type<u32[4]>();

		const auto x = m_ir->CreateZExt(val, get_type<u64[4]>());
		const auto s = m_ir->CreateShl(m_ir->CreateAnd(x, 0x80000000), 32);
		const auto a = m_ir->CreateAnd(x, 0x7fffffff);
		const auto m = m_ir->CreateShl(m_ir->CreateAdd(a, splat<u64[4]>(0x1c0000000).value), 29);
		const auto r = m_ir->CreateSelect(m_ir->CreateICmpSGT(a, splat<u64[4]>(0x7fffff).value), m, splat<u64[4]>(0).value);
		const auto f = m_ir->CreateOr(s, r);
		return uint64_as_double(f);
	}

	// Clamp double values to ±Smax, flush values smaller than ±Smin to positive zero
	llvm::Value* xfloat_in_double(llvm::Value* val)
	{
		verify("xfloat_in_double" HERE), val, val->getType() == get_type<f64[4]>();

		const auto smax = uint64_as_double(splat<u64[4]>(0x47ffffffe0000000).value);
		const auto smin = uint64_as_double(splat<u64[4]>(0x3810000000000000).value);

		const auto d = double_as_uint64(val);
		const auto s = m_ir->CreateAnd(d, 0x8000000000000000);
		const auto a = uint64_as_double(m_ir->CreateAnd(d, 0x7fffffffe0000000));
		const auto n = m_ir->CreateFCmpOLT(a, smax);
		const auto z = m_ir->CreateFCmpOLT(a, smin);
		const auto c = double_as_uint64(m_ir->CreateSelect(n, a, smax));
		return m_ir->CreateSelect(z, fsplat<f64[4]>(0.).value, uint64_as_double(m_ir->CreateOr(c, s)));
	}

	// Expand 32-bit mask for xfloat values to 64-bit, 29 least significant bits are always zero
	llvm::Value* conv_xfloat_mask(llvm::Value* val)
	{
		const auto d = m_ir->CreateZExt(val, get_type<u64[4]>());
		const auto s = m_ir->CreateShl(m_ir->CreateAnd(d, 0x80000000), 32);
		const auto e = m_ir->CreateLShr(m_ir->CreateAShr(m_ir->CreateShl(d, 33), 4), 1);
		return m_ir->CreateOr(s, e);
	}

	llvm::Value* get_reg_raw(u32 index)
	{
		if (!m_block || index >= m_block->reg.size())
		{
			return nullptr;
		}

		return m_block->reg[index];
	}

	llvm::Value* get_reg_fixed(u32 index, llvm::Type* type)
	{
		llvm::Value* dummy{};

		auto& reg = *(m_block ? &m_block->reg.at(index) : &dummy);

		if (!reg)
		{
			// Load register value if necessary
			reg = m_ir->CreateLoad(init_reg_fixed(index));
		}

		if (reg->getType() == get_type<f64[4]>())
		{
			if (type == reg->getType())
			{
				return reg;
			}

			const auto res = double_to_xfloat(reg);

			if (auto c = llvm::dyn_cast<llvm::Constant>(res))
			{
				return make_const_vector(get_const_vector(c, m_pos, 1000 + index), type);
			}

			return m_ir->CreateBitCast(res, type);
		}

		if (type == get_type<f64[4]>())
		{
			if (const auto phi = llvm::dyn_cast<llvm::PHINode>(reg))
			{
				if (phi->getNumUses())
				{
					LOG_WARNING(SPU, "[0x%x] $%u: Phi has uses :(", m_pos, index);
				}
				else
				{
					const auto cblock = m_ir->GetInsertBlock();
					m_ir->SetInsertPoint(phi);

					const auto newphi = m_ir->CreatePHI(get_type<f64[4]>(), phi->getNumIncomingValues());

					for (u32 i = 0; i < phi->getNumIncomingValues(); i++)
					{
						const auto iblock = phi->getIncomingBlock(i);
						m_ir->SetInsertPoint(iblock->getTerminator());
						const auto ivalue = phi->getIncomingValue(i);
						newphi->addIncoming(xfloat_to_double(ivalue), iblock);
					}

					if (phi->getParent() == m_block->block)
					{
						m_block->phi[index] = newphi;
					}

					reg = newphi;

					m_ir->SetInsertPoint(cblock);
					phi->eraseFromParent();
					return reg;
				}
			}

			if (auto c = llvm::dyn_cast<llvm::Constant>(reg))
			{
				return xfloat_to_double(make_const_vector(get_const_vector(c, m_pos, 2000 + index), get_type<u32[4]>()));
			}

			return xfloat_to_double(m_ir->CreateBitCast(reg, get_type<u32[4]>()));
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
	value_t<T> get_reg_fixed(u32 index)
	{
		value_t<T> r;
		r.value = get_reg_fixed(index, get_type<T>());
		return r;
	}

	template <typename T = u32[4], uint I>
	value_t<T> get_vr(const bf_t<u32, I, 7>& index)
	{
		value_t<T> r;

		if ((m_op_const_mask & index.data_mask()) != index.data_mask())
		{
			// Update const mask if necessary
			if (I >= (32 - m_interp_magn))
			{
				m_op_const_mask |= index.data_mask();
			}

			// Load reg
			if (get_type<T>() == get_type<f64[4]>())
			{
				r.value = xfloat_to_double(m_ir->CreateLoad(init_vr<u32[4]>(index)));
			}
			else
			{
				r.value = m_ir->CreateLoad(init_vr<T>(index));
			}
		}
		else
		{
			r.value = get_reg_fixed(index, get_type<T>());
		}

		return r;
	}

	void set_reg_fixed(u32 index, llvm::Value* value, bool fixup = true)
	{
		llvm::StoreInst* dummy{};

		// Check
		verify(HERE), !m_block || m_regmod[m_pos / 4] == index;

		// Test for special case
		const bool is_xfloat = value->getType() == get_type<f64[4]>();

		// Clamp value if necessary
		const auto saved_value = is_xfloat && fixup ? xfloat_in_double(value) : value;

		// Set register value
		if (m_block)
		{
			m_block->reg.at(index) = saved_value;
		}

		// Get register location
		const auto addr = init_reg_fixed(index);

		auto& _store = *(m_block ? &m_block->store[index] : &dummy);

		// Erase previous dead store instruction if necessary
		if (_store)
		{
			// TODO: better cross-block dead store elimination
			_store->eraseFromParent();
		}

		// Write register to the context
		_store = m_ir->CreateStore(is_xfloat ? double_to_xfloat(saved_value) : m_ir->CreateBitCast(value, addr->getType()->getPointerElementType()), addr);
	}

	template <typename T, uint I>
	void set_vr(const bf_t<u32, I, 7>& index, T expr, bool fixup = true)
	{
		// Process expression
		const auto value = expr.eval(m_ir);

		// Test for special case
		const bool is_xfloat = value->getType() == get_type<f64[4]>();

		if ((m_op_const_mask & index.data_mask()) != index.data_mask())
		{
			// Update const mask if necessary
			if (I >= (32 - m_interp_magn))
			{
				m_op_const_mask |= index.data_mask();
			}

			// Clamp value if necessary
			const auto saved_value = is_xfloat && fixup ? xfloat_in_double(value) : value;

			// Store value
			m_ir->CreateStore(is_xfloat ? double_to_xfloat(saved_value) : m_ir->CreateBitCast(value, get_type<u32[4]>()), init_vr<u32[4]>(index));
			return;
		}

		set_reg_fixed(index, value, fixup);
	}

	template <typename T = u32[4], uint I, uint N>
	value_t<T> get_imm(const bf_t<u32, I, N>& imm, bool mask = true)
	{
		if ((m_op_const_mask & imm.data_mask()) != imm.data_mask())
		{
			// Update const mask if necessary
			if (I >= (32 - m_interp_magn))
			{
				m_op_const_mask |= imm.data_mask();
			}

			// Extract unsigned immediate (skip AND if mask == false or truncated anyway)
			value_t<T> r;
			r.value = m_interp_op;
			r.value = I == 0 ? r.value : m_ir->CreateLShr(r.value, u64{I});
			r.value = !mask || N >= r.esize ? r.value : m_ir->CreateAnd(r.value, imm.data_mask() >> I);

			if (r.esize != 32)
			{
				r.value = m_ir->CreateZExtOrTrunc(r.value, get_type<T>()->getScalarType());
			}

			if (r.is_vector)
			{
				r.value = m_ir->CreateVectorSplat(r.is_vector, r.value);
			}

			return r;
		}

		return splat<T>(imm);
	}

	template <typename T = u32[4], uint I, uint N>
	value_t<T> get_imm(const bf_t<s32, I, N>& imm)
	{
		if ((m_op_const_mask & imm.data_mask()) != imm.data_mask())
		{
			// Update const mask if necessary
			if (I >= (32 - m_interp_magn))
			{
				m_op_const_mask |= imm.data_mask();
			}

			// Extract signed immediate (skip sign ext if truncated anyway)
			value_t<T> r;
			r.value = m_interp_op;
			r.value = I + N == 32 || N >= r.esize ? r.value : m_ir->CreateShl(r.value, u64{32 - I - N});
			r.value = N == 32 || N >= r.esize ? r.value : m_ir->CreateAShr(r.value, u64{32 - N});
			r.value = I == 0 || N < r.esize ? r.value : m_ir->CreateLShr(r.value, u64{I});

			if (r.esize != 32)
			{
				r.value = m_ir->CreateSExtOrTrunc(r.value, get_type<T>()->getScalarType());
			}

			if (r.is_vector)
			{
				r.value = m_ir->CreateVectorSplat(r.is_vector, r.value);
			}

			return r;
		}

		return splat<T>(imm);
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
		m_ir->CreateStore(m_ir->getInt32(m_pos), spu_ptr<u32>(&spu_thread::pc))->setVolatile(true);
	}

	// Call cpu_thread::check_state if necessary and return or continue (full check)
	void check_state(u32 addr)
	{
		const auto pstate = spu_ptr<u32>(&spu_thread::state);
		const auto _body = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto check = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto stop  = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(m_ir->CreateICmpEQ(m_ir->CreateLoad(pstate), m_ir->getInt32(0)), _body, check, m_md_likely);
		m_ir->SetInsertPoint(check);
		m_ir->CreateStore(m_ir->getInt32(addr), spu_ptr<u32>(&spu_thread::pc));
		m_ir->CreateCondBr(call(&exec_check_state, m_thread), stop, _body, m_md_unlikely);
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
	spu_llvm_recompiler(u8 interp_magn = 0)
		: spu_recompiler_base()
		, cpu_translator(nullptr, false)
		, m_interp_magn(interp_magn)
	{
	}

	virtual void init() override
	{
		// Initialize if necessary
		if (!m_spurt)
		{
			m_cache = fxm::get<spu_cache>();
			m_spurt = fxm::get_always<spu_runtime>();
			m_context = m_jit.get_context();
			m_use_ssse3 = m_jit.has_ssse3();

			const auto md_name = llvm::MDString::get(m_context, "branch_weights");
			const auto md_low = llvm::ValueAsMetadata::get(llvm::ConstantInt::get(GetType<u32>(), 1));
			const auto md_high = llvm::ValueAsMetadata::get(llvm::ConstantInt::get(GetType<u32>(), 999));

			// Metadata for branch weights
			m_md_likely = llvm::MDTuple::get(m_context, {md_name, md_high, md_low});
			m_md_unlikely = llvm::MDTuple::get(m_context, {md_name, md_low, md_high});
		}
	}

	virtual bool compile(u64 last_reset_count, const std::vector<u32>& func) override
	{
		if (func.empty() && last_reset_count == 0 && m_interp_magn)
		{
			return compile_interpreter();
		}

		const auto fn_location = m_spurt->find(last_reset_count, func);

		if (fn_location == spu_runtime::g_dispatcher)
		{
			return true;
		}

		if (!fn_location)
		{
			return false;
		}

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
			fs::file(m_spurt->get_cache_path() + "spu.log", fs::write + fs::append).write(log);
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
		const auto main_func = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(hash, get_ftype<void, u8*, u8*, u8*>()).getCallee());
		const auto main_arg2 = &*(main_func->arg_begin() + 2);
		set_function(main_func);

		// Start compilation

		update_pc();

		const auto label_test = BasicBlock::Create(m_context, "", m_function);
		const auto label_diff = BasicBlock::Create(m_context, "", m_function);
		const auto label_body = BasicBlock::Create(m_context, "", m_function);
		const auto label_stop = BasicBlock::Create(m_context, "", m_function);

		// Emit state check
		const auto pstate = spu_ptr<u32>(&spu_thread::state);
		m_ir->CreateCondBr(m_ir->CreateICmpNE(m_ir->CreateLoad(pstate, true), m_ir->getInt32(0)), label_stop, label_test, m_md_unlikely);

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
			m_ir->CreateCondBr(cond, label_diff, label_body, m_md_unlikely);
		}
		else if (func.size() - 1 == 2)
		{
			const auto cond = m_ir->CreateICmpNE(m_ir->CreateLoad(_ptr<u64>(m_lsptr, start)), m_ir->getInt64(static_cast<u64>(func[2]) << 32 | func[1]));
			m_ir->CreateCondBr(cond, label_diff, label_body, m_md_unlikely);
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
			m_ir->CreateCondBr(cond, label_diff, label_body, m_md_unlikely);
		}

		// Increase block counter with statistics
		m_ir->SetInsertPoint(label_body);
		const auto pbcount = spu_ptr<u64>(&spu_thread::block_counter);
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
			const auto pbfail = spu_ptr<u64>(&spu_thread::block_failure);
			m_ir->CreateStore(m_ir->CreateAdd(m_ir->CreateLoad(pbfail), m_ir->getInt64(1)), pbfail);
			tail(&spu_recompiler_base::dispatch, m_thread, m_ir->getInt32(0), main_arg2);
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
							const auto _phi = m_ir->CreatePHI(get_reg_type(i), 0 - src);
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
										const auto regptr = init_reg_fixed(i);
										const auto cblock = m_ir->GetInsertBlock();
										m_ir->SetInsertPoint(bfound->second.block_end->getTerminator());

										if (!value)
										{
											// Value hasn't been loaded yet
											value = m_finfo->reg[i] ? m_finfo->reg[i] : m_ir->CreateLoad(regptr);
										}

										if (value->getType() == get_type<f64[4]>())
										{
											value = double_to_xfloat(value);
										}
										else if (i < 128 && llvm::isa<llvm::Constant>(value))
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
								const auto regptr = init_reg_fixed(i);
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

				// State check at the beginning of the chunk
				if (bi == 0 && g_cfg.core.spu_block_size != spu_block_size_type::safe)
				{
					check_state(baddr);
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
					try
					{
						(this->*g_decoder.decode(op))({op});
					}
					catch (const std::exception& e)
					{
						std::string dump;
						raw_string_ostream out(dump);
						out << *module; // print IR
						out.flush();
						LOG_ERROR(SPU, "[0x%x] LLVM dump:\n%s", m_pos, dump);
						throw;
					}
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

			const auto null = cast<Function>(module->getOrInsertFunction("spu-null", get_ftype<void, u8*, u8*, u32>()).getCallee());
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
		pm.add(createCFGSimplificationPass());
		pm.add(createNewGVNPass());
		pm.add(createDeadStoreEliminationPass());
		pm.add(createLoopVersioningLICMPass());
		pm.add(createAggressiveDCEPass());
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
				fs::file(m_spurt->get_cache_path() + "spu.log", fs::write + fs::append).write(log);
			}

			fmt::raw_error("Compilation failed");
		}

		if (g_cfg.core.spu_debug)
		{
			// Testing only
			m_jit.add(std::move(module), m_spurt->get_cache_path() + "llvm/");
		}
		else
		{
			m_jit.add(std::move(module));
		}

		m_jit.fin();

		// Register function pointer
		const spu_function_t fn = reinterpret_cast<spu_function_t>(m_jit.get_engine().getPointerToFunction(main_func));

		if (!m_spurt->add(last_reset_count, fn_location, fn))
		{
			return false;
		}

		if (g_cfg.core.spu_debug)
		{
			out.flush();
			fs::file(m_spurt->get_cache_path() + "spu.log", fs::write + fs::append).write(log);
		}

		if (m_cache && g_cfg.core.spu_cache)
		{
			m_cache->add(func);
		}

		return true;
	}

	static void interp_check(spu_thread* _spu, bool after)
	{
		static const spu_decoder<spu_interpreter_fast> s_dec;

		static thread_local std::array<v128, 128> s_gpr;

		if (!after)
		{
			// Preserve reg state
			s_gpr = _spu->gpr;

			// Execute interpreter instruction
			const u32 op = *reinterpret_cast<const be_t<u32>*>(_spu->_ptr<u8>(0) + _spu->pc);
			if (!s_dec.decode(op)(*_spu, {op}))
				LOG_FATAL(SPU, "Bad instruction" HERE);

			// Swap state
			for (u32 i = 0; i < s_gpr.size(); ++i)
				std::swap(_spu->gpr[i], s_gpr[i]);
		}
		else
		{
			// Check saved state
			for (u32 i = 0; i < s_gpr.size(); ++i)
			{
				if (_spu->gpr[i] != s_gpr[i])
				{
					LOG_FATAL(SPU, "Register mismatch: $%u\n%s\n%s", i, _spu->gpr[i], s_gpr[i]);
					_spu->state += cpu_flag::dbg_pause;
				}
			}
		}
	}

	bool compile_interpreter()
	{
		using namespace llvm;

		// Create LLVM module
		std::unique_ptr<Module> module = std::make_unique<Module>("spu_interpreter.obj", m_context);
		module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));
		m_module = module.get();

		// Initialize IR Builder
		IRBuilder<> irb(m_context);
		m_ir = &irb;

		// Create interpreter table
		const auto if_type = get_ftype<void, u8*, u8*, u32, u32, u8*, u32, u8*>();
		const auto if_pptr = if_type->getPointerTo()->getPointerTo();
		m_function_table = new GlobalVariable(*m_module, ArrayType::get(if_type->getPointerTo(), 1u << m_interp_magn), true, GlobalValue::InternalLinkage, nullptr);

		// Add return function
		const auto ret_func = cast<Function>(module->getOrInsertFunction("spu_ret", if_type).getCallee());
		ret_func->setCallingConv(CallingConv::GHC);
		ret_func->setLinkage(GlobalValue::InternalLinkage);
		m_ir->SetInsertPoint(BasicBlock::Create(m_context, "", ret_func));
		m_thread = &*(ret_func->arg_begin() + 1);
		m_interp_pc = &*(ret_func->arg_begin() + 2);
		m_ir->CreateStore(m_interp_pc, spu_ptr<u32>(&spu_thread::pc));
		m_ir->CreateRetVoid();

		// Add entry function, serves as a trampoline
		const auto main_func = llvm::cast<Function>(m_module->getOrInsertFunction("spu_interpreter", get_ftype<void, u8*, u8*, u8*>()).getCallee());
		set_function(main_func);

		// Load pc and opcode
		m_interp_pc = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::pc));
		m_interp_op = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, m_ir->CreateZExt(m_interp_pc, get_type<u64>())), get_type<u32*>()));
		m_interp_op = m_ir->CreateCall(get_intrinsic<u32>(Intrinsic::bswap), {m_interp_op});

		// Pinned constant, address of interpreter table
		m_interp_table = m_ir->CreateBitCast(m_ir->CreateGEP(m_function_table, {m_ir->getInt64(0), m_ir->getInt64(0)}), get_type<u8*>());

		// Pinned constant, mask for shifted register index
		m_interp_7f0 = m_ir->getInt32(0x7f0);

		// Pinned constant, address of first register
		m_interp_regs = _ptr(m_thread, get_reg_offset(0));

		// Decode (shift) and load function pointer
		const auto first = m_ir->CreateLoad(m_ir->CreateGEP(m_ir->CreateBitCast(m_interp_table, if_pptr), m_ir->CreateLShr(m_interp_op, 32 - m_interp_magn)));
		const auto call0 = m_ir->CreateCall(first, {m_lsptr, m_thread, m_interp_pc, m_interp_op, m_interp_table, m_interp_7f0, m_interp_regs});
		call0->setCallingConv(CallingConv::GHC);
		m_ir->CreateRetVoid();

		// Create helper globals
		{
			std::vector<llvm::Constant*> float_to;
			std::vector<llvm::Constant*> to_float;
			float_to.reserve(256);
			to_float.reserve(256);

			for (int i = 0; i < 256; ++i)
			{
				float_to.push_back(ConstantFP::get(get_type<f32>(), std::exp2(173 - i)));
				to_float.push_back(ConstantFP::get(get_type<f32>(), std::exp2(i - 155)));
			}

			const auto atype = ArrayType::get(get_type<f32>(), 256);
			m_scale_float_to = new GlobalVariable(*m_module, atype, true, GlobalValue::InternalLinkage, ConstantArray::get(atype, float_to));
			m_scale_to_float = new GlobalVariable(*m_module, atype, true, GlobalValue::InternalLinkage, ConstantArray::get(atype, to_float));
		}

		// Fill interpreter table
		std::vector<llvm::Constant*> iptrs;
		iptrs.reserve(1u << m_interp_magn);

		m_block = nullptr;

		auto last_itype = spu_itype::UNK;

		for (u32 i = 0; i < 1u << m_interp_magn;)
		{
			// Fake opcode
			const u32 op = i << (32 - m_interp_magn);

			// Instruction type
			const auto itype = s_spu_itype.decode(op);

			// Function name
			std::string fname = fmt::format("spu_%s", s_spu_iname.decode(op));

			if (last_itype != itype)
			{
				// Trigger automatic information collection (probing)
				m_op_const_mask = 0;
			}
			else
			{
				// Inject const mask into function name
				fmt::append(fname, "_%X", (i & (m_op_const_mask >> (32 - m_interp_magn))) | (1u << m_interp_magn));
			}

			// Decode instruction name, access function
			const auto f = cast<Function>(module->getOrInsertFunction(fname, if_type).getCallee());

			// Build if necessary
			if (f->empty())
			{
				f->setCallingConv(CallingConv::GHC);
				f->setLinkage(GlobalValue::InternalLinkage);

				m_function = f;
				m_lsptr  = &*(f->arg_begin() + 0);
				m_thread = &*(f->arg_begin() + 1);
				m_interp_pc = &*(f->arg_begin() + 2);
				m_interp_op = &*(f->arg_begin() + 3);
				m_interp_table = &*(f->arg_begin() + 4);
				m_interp_7f0 = &*(f->arg_begin() + 5);
				m_interp_regs = &*(f->arg_begin() + 6);

				m_ir->SetInsertPoint(BasicBlock::Create(m_context, "", f));

				switch (itype)
				{
				case spu_itype::UNK:
				case spu_itype::DFCEQ:
				case spu_itype::DFCMEQ:
				case spu_itype::DFCGT:
				case spu_itype::DFCMGT:
				case spu_itype::DFTSV:
				case spu_itype::STOP:
				case spu_itype::STOPD:
				case spu_itype::RDCH:
				case spu_itype::WRCH:
				{
					// Invalid or abortable instruction. Save current address.
					m_ir->CreateStore(m_interp_pc, spu_ptr<u32>(&spu_thread::pc));
					[[fallthrough]];
				}
				default:
				{
					break;
				}
				}

				try
				{
					m_interp_bblock = nullptr;

					// Next instruction (no wraparound at the end of LS)
					m_interp_pc_next = m_ir->CreateAdd(m_interp_pc, m_ir->getInt32(4));

					bool check = false;

					if (itype == spu_itype::WRCH ||
						itype == spu_itype::RDCH ||
						itype == spu_itype::RCHCNT ||
						itype == spu_itype::STOP ||
						itype == spu_itype::STOPD ||
						itype & spu_itype::floating ||
						itype & spu_itype::branch)
					{
						check = false;
					}

					if (itype & spu_itype::branch)
					{
						// Instruction changes pc - change order.
						(this->*g_decoder.decode(op))({op});

						if (m_interp_bblock)
						{
							m_ir->SetInsertPoint(m_interp_bblock);
							m_interp_bblock = nullptr;
						}
					}

					if (!m_ir->GetInsertBlock()->getTerminator())
					{
						if (check)
						{
							m_ir->CreateStore(m_interp_pc, spu_ptr<u32>(&spu_thread::pc));
						}

						// Decode next instruction.
						const auto next_pc = itype & spu_itype::branch ? m_interp_pc : m_interp_pc_next;
						const auto be32_op = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, m_ir->CreateZExt(next_pc, get_type<u64>())), get_type<u32*>()));
						const auto next_op = m_ir->CreateCall(get_intrinsic<u32>(Intrinsic::bswap), {be32_op});
						const auto next_if = m_ir->CreateLoad(m_ir->CreateGEP(m_ir->CreateBitCast(m_interp_table, if_pptr), m_ir->CreateLShr(next_op, 32 - m_interp_magn)));
						llvm::cast<LoadInst>(next_if)->setVolatile(true);

						if (!(itype & spu_itype::branch))
						{
							if (check)
							{
								call(&interp_check, m_thread, m_ir->getFalse());
							}

							// Normal instruction.
							(this->*g_decoder.decode(op))({op});

							if (check && !m_ir->GetInsertBlock()->getTerminator())
							{
								call(&interp_check, m_thread, m_ir->getTrue());
							}

							m_interp_pc = m_interp_pc_next;
						}

						if (!m_ir->GetInsertBlock()->getTerminator())
						{
							// Call next instruction.
							const auto _stop = BasicBlock::Create(m_context, "", f);
							const auto _next = BasicBlock::Create(m_context, "", f);
							m_ir->CreateCondBr(m_ir->CreateIsNotNull(m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::state))), _stop, _next, m_md_unlikely);
							m_ir->SetInsertPoint(_next);

							if (itype == spu_itype::WRCH ||
								itype == spu_itype::RDCH ||
								itype == spu_itype::RCHCNT ||
								itype == spu_itype::STOP ||
								itype == spu_itype::STOPD)
							{
								m_interp_7f0  = m_ir->getInt32(0x7f0);
								m_interp_regs = _ptr(m_thread, get_reg_offset(0));
							}

							const auto ncall = m_ir->CreateCall(next_if, {m_lsptr, m_thread, m_interp_pc, next_op, m_interp_table, m_interp_7f0, m_interp_regs});
							ncall->setCallingConv(CallingConv::GHC);
							ncall->setTailCall();
							m_ir->CreateRetVoid();
							m_ir->SetInsertPoint(_stop);
							m_ir->CreateStore(m_interp_pc, spu_ptr<u32>(&spu_thread::pc));
							m_ir->CreateRetVoid();
						}
					}
				}
				catch (const std::exception& e)
				{
					std::string dump;
					raw_string_ostream out(dump);
					out << *module; // print IR
					out.flush();
					LOG_ERROR(SPU, "[0x%x] LLVM dump:\n%s", m_pos, dump);
					throw;
				}
			}

			if (last_itype != itype)
			{
				// Repeat after probing
				last_itype = itype;
			}
			else
			{
				// Add to the table
				iptrs.push_back(f);
				i++;
			}
		}

		m_function_table->setInitializer(ConstantArray::get(ArrayType::get(if_type->getPointerTo(), 1u << m_interp_magn), iptrs));
		m_function_table = nullptr;

		// Initialize pass manager
		legacy::FunctionPassManager pm(module.get());

		// Basic optimizations
		pm.add(createEarlyCSEPass());
		pm.add(createCFGSimplificationPass());
		pm.add(createDeadStoreEliminationPass());
		pm.add(createAggressiveDCEPass());
		//pm.add(createLintPass());

		std::string log;

		raw_string_ostream out(log);

		if (g_cfg.core.spu_debug)
		{
			fmt::append(log, "LLVM IR (interpreter):\n");
			out << *module; // print IR
			out << "\n\n";
		}

		if (verifyModule(*module, &out))
		{
			out.flush();
			LOG_ERROR(SPU, "LLVM: Verification failed:\n%s", log);

			if (g_cfg.core.spu_debug)
			{
				fs::file(m_spurt->get_cache_path() + "spu.log", fs::write + fs::append).write(log);
			}

			fmt::raw_error("Compilation failed");
		}

		if (g_cfg.core.spu_debug)
		{
			// Testing only
			m_jit.add(std::move(module), m_spurt->get_cache_path() + "llvm/");
		}
		else
		{
			m_jit.add(std::move(module));
		}

		m_jit.fin();

		// Register interpreter entry point
		spu_runtime::g_interpreter = reinterpret_cast<spu_function_t>(m_jit.get_engine().getPointerToFunction(main_func));

		if (!spu_runtime::g_interpreter)
		{
			return false;
		}

		if (g_cfg.core.spu_debug)
		{
			out.flush();
			fs::file(m_spurt->get_cache_path() + "spu.log", fs::write + fs::append).write(log);
		}

		return true;
	}

	static bool exec_check_state(spu_thread* _spu)
	{
		return _spu->check_state();
	}

	template <spu_inter_func_t F>
	static void exec_fall(spu_thread* _spu, spu_opcode_t op)
	{
		if (F(*_spu, op))
		{
			_spu->pc += 4;
		}
	}

	template <spu_inter_func_t F>
	void fall(spu_opcode_t op)
	{
		if (m_interp_magn)
		{
			call(F, m_thread, m_interp_op);
			return;
		}

		update_pc();
		call(&exec_fall<F>, m_thread, m_ir->getInt32(op.opcode));
	}

	static void exec_unk(spu_thread* _spu, u32 op)
	{
		fmt::throw_exception("Unknown/Illegal instruction (0x%08x)" HERE, op);
	}

	void UNK(spu_opcode_t op_unk)
	{
		if (m_interp_magn)
		{
			m_ir->CreateStore(m_interp_pc, spu_ptr<u32>(&spu_thread::pc));
			call(&exec_unk, m_thread, m_ir->getInt32(op_unk.opcode));
			return;
		}

		m_block->block_end = m_ir->GetInsertBlock();
		update_pc();
		tail(&exec_unk, m_thread, m_ir->getInt32(op_unk.opcode));
	}

	static bool exec_stop(spu_thread* _spu, u32 code)
	{
		return _spu->stop_and_signal(code);
	}

	void STOP(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			const auto succ = call(&exec_stop, m_thread, m_ir->CreateAnd(m_interp_op, m_ir->getInt32(0x3fff)));
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto stop = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(succ, next, stop);
			m_ir->SetInsertPoint(stop);
			m_ir->CreateRetVoid();
			m_ir->SetInsertPoint(next);
			return;
		}

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
			m_ir->CreateStore(m_ir->getInt32(m_pos + 4), spu_ptr<u32>(&spu_thread::pc));
			m_ir->CreateRetVoid();
		}
		else
		{
			check_state(m_pos + 4);
		}
	}

	void STOPD(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			const auto succ = call(&exec_stop, m_thread, m_ir->getInt32(0x3fff));
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto stop = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(succ, next, stop);
			m_ir->SetInsertPoint(stop);
			m_ir->CreateRetVoid();
			m_ir->SetInsertPoint(next);
			return;
		}

		STOP(spu_opcode_t{0x3fff});
	}

	static s64 exec_rdch(spu_thread* _spu, u32 ch)
	{
		return _spu->get_ch_value(ch);
	}

	static s64 exec_read_in_mbox(spu_thread* _spu)
	{
		// TODO
		return _spu->get_ch_value(SPU_RdInMbox);
	}

	static u32 exec_read_dec(spu_thread* _spu)
	{
		const u32 res = _spu->ch_dec_value - static_cast<u32>(get_timebased_time() - _spu->ch_dec_start_timestamp);

		if (res > 1500 && g_cfg.core.spu_loop_detection)
		{
			std::this_thread::yield();
		}

		return res;
	}

	static s64 exec_read_events(spu_thread* _spu)
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

		if (m_interp_magn)
		{
			res.value = call(&exec_rdch, m_thread, get_imm<u32>(op.ra).value);
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto stop = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpSLT(res.value, m_ir->getInt64(0)), stop, next);
			m_ir->SetInsertPoint(stop);
			m_ir->CreateRetVoid();
			m_ir->SetInsertPoint(next);
			res.value = m_ir->CreateTrunc(res.value, get_type<u32>());
			set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
			return;
		}

		switch (op.ra)
		{
		case SPU_RdSRR0:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::srr0));
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
			res.value = get_rdch(op, ::offset32(&spu_thread::ch_tag_stat), false);
			break;
		}
		case MFC_RdTagMask:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::ch_tag_mask));
			break;
		}
		case SPU_RdSigNotify1:
		{
			res.value = get_rdch(op, ::offset32(&spu_thread::ch_snr1), true);
			break;
		}
		case SPU_RdSigNotify2:
		{
			res.value = get_rdch(op, ::offset32(&spu_thread::ch_snr2), true);
			break;
		}
		case MFC_RdAtomicStat:
		{
			res.value = get_rdch(op, ::offset32(&spu_thread::ch_atomic_stat), false);
			break;
		}
		case MFC_RdListStallStat:
		{
			res.value = get_rdch(op, ::offset32(&spu_thread::ch_stall_stat), false);
			break;
		}
		case SPU_RdDec:
		{
			res.value = call(&exec_read_dec, m_thread);
			break;
		}
		case SPU_RdEventMask:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::ch_event_mask));
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
			res.value = m_ir->CreateZExt(m_ir->CreateLoad(spu_ptr<u8>(&spu_thread::interrupts_enabled)), get_type<u32>());
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

	static u32 exec_rchcnt(spu_thread* _spu, u32 ch)
	{
		return _spu->get_ch_count(ch);
	}

	static u32 exec_get_events(spu_thread* _spu)
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

		if (m_interp_magn)
		{
			res.value = call(&exec_rchcnt, m_thread, get_imm<u32>(op.ra).value);
			set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
			return;
		}

		switch (op.ra)
		{
		case SPU_WrOutMbox:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_out_mbox), true);
			break;
		}
		case SPU_WrOutIntrMbox:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_out_intr_mbox), true);
			break;
		}
		case MFC_RdTagStat:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_tag_stat));
			break;
		}
		case MFC_RdListStallStat:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_stall_stat));
			break;
		}
		case SPU_RdSigNotify1:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_snr1));
			break;
		}
		case SPU_RdSigNotify2:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_snr2));
			break;
		}
		case MFC_RdAtomicStat:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_atomic_stat));
			break;
		}
		case MFC_WrTagUpdate:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::ch_tag_upd), true);
			res.value = m_ir->CreateICmpEQ(res.value, m_ir->getInt32(0));
			res.value = m_ir->CreateZExt(res.value, get_type<u32>());
			break;
		}
		case MFC_Cmd:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::mfc_size), true);
			res.value = m_ir->CreateSub(m_ir->getInt32(16), res.value);
			break;
		}
		case SPU_RdInMbox:
		{
			res.value = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::ch_in_mbox), true);
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

	static bool exec_wrch(spu_thread* _spu, u32 ch, u32 value)
	{
		return _spu->set_ch_value(ch, value);
	}

	static void exec_mfc(spu_thread* _spu)
	{
		return _spu->do_mfc();
	}

	static void exec_list_unstall(spu_thread* _spu, u32 tag)
	{
		for (u32 i = 0; i < _spu->mfc_size; i++)
		{
			if (_spu->mfc_queue[i].tag == (tag | 0x80))
			{
				_spu->mfc_queue[i].tag &= 0x7f;
			}
		}

		return exec_mfc(_spu);
	}

	static bool exec_mfc_cmd(spu_thread* _spu)
	{
		return _spu->process_mfc_cmd();
	}

	void WRCH(spu_opcode_t op) //
	{
		const auto val = extract(get_vr(op.rt), 3);

		if (m_interp_magn)
		{
			const auto succ = call(&exec_wrch, m_thread, get_imm<u32>(op.ra).value, val.value);
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto stop = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(succ, next, stop);
			m_ir->SetInsertPoint(stop);
			m_ir->CreateRetVoid();
			m_ir->SetInsertPoint(next);
			return;
		}

		switch (op.ra)
		{
		case SPU_WrSRR0:
		{
			m_ir->CreateStore(val.value, spu_ptr<u32>(&spu_thread::srr0));
			return;
		}
		case SPU_WrOutIntrMbox:
		{
			// TODO
			break;
		}
		case SPU_WrOutMbox:
		{
			// TODO
			break;
		}
		case MFC_WrTagMask:
		{
			// TODO
			m_ir->CreateStore(val.value, spu_ptr<u32>(&spu_thread::ch_tag_mask));
			return;
		}
		case MFC_WrTagUpdate:
		{
			if (auto ci = llvm::dyn_cast<llvm::ConstantInt>(val.value))
			{
				const u64 upd = ci->getZExtValue();

				const auto tag_mask  = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::ch_tag_mask));
				const auto mfc_fence = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::mfc_fence));
				const auto completed = m_ir->CreateAnd(tag_mask, m_ir->CreateNot(mfc_fence));
				const auto upd_ptr   = spu_ptr<u32>(&spu_thread::ch_tag_upd);
				const auto stat_ptr  = spu_ptr<u64>(&spu_thread::ch_tag_stat);
				const auto stat_val  = m_ir->CreateOr(m_ir->CreateZExt(completed, get_type<u64>()), INT64_MIN);

				if (upd == 0)
				{
					m_ir->CreateStore(m_ir->getInt32(0), upd_ptr);
					m_ir->CreateStore(stat_val, stat_ptr);
					return;
				}
				else if (upd == 1)
				{
					const auto cond = m_ir->CreateICmpNE(completed, m_ir->getInt32(0));
					m_ir->CreateStore(m_ir->CreateSelect(cond, m_ir->getInt32(1), m_ir->getInt32(0)), upd_ptr);
					m_ir->CreateStore(m_ir->CreateSelect(cond, stat_val, m_ir->getInt64(0)), stat_ptr);
					return;
				}
				else if (upd == 2)
				{
					const auto cond = m_ir->CreateICmpEQ(completed, tag_mask);
					m_ir->CreateStore(m_ir->CreateSelect(cond, m_ir->getInt32(2), m_ir->getInt32(0)), upd_ptr);
					m_ir->CreateStore(m_ir->CreateSelect(cond, stat_val, m_ir->getInt64(0)), stat_ptr);
					return;
				}
			}

			LOG_WARNING(SPU, "[0x%x] MFC_WrTagUpdate: $%u is not a good constant", m_pos, +op.rt);
			break;
		}
		case MFC_LSA:
		{
			set_reg_fixed(s_reg_mfc_lsa, val.value);
			return;
		}
		case MFC_EAH:
		{
			if (auto ci = llvm::dyn_cast<llvm::ConstantInt>(val.value))
			{
				if (ci->getZExtValue() == 0)
				{
					return;
				}
			}

			LOG_WARNING(SPU, "[0x%x] MFC_EAH: $%u is not a zero constant", m_pos, +op.rt);
			//m_ir->CreateStore(val.value, spu_ptr<u32>(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::eah));
			return;
		}
		case MFC_EAL:
		{
			set_reg_fixed(s_reg_mfc_eal, val.value);
			return;
		}
		case MFC_Size:
		{
			set_reg_fixed(s_reg_mfc_size, trunc<u16>(val & 0x7fff).value);
			return;
		}
		case MFC_TagID:
		{
			set_reg_fixed(s_reg_mfc_tag, trunc<u8>(val & 0x1f).value);
			return;
		}
		case MFC_Cmd:
		{
			// Prevent store elimination (TODO)
			m_block->store[s_reg_mfc_eal] = nullptr;
			m_block->store[s_reg_mfc_lsa] = nullptr;
			m_block->store[s_reg_mfc_tag] = nullptr;
			m_block->store[s_reg_mfc_size] = nullptr;

			if (!g_use_rtm)
			{
				// TODO: don't require TSX (current implementation is TSX-only)
				break;
			}

			if (auto ci = llvm::dyn_cast<llvm::ConstantInt>(trunc<u8>(val).value))
			{
				const auto eal = get_reg_fixed<u32>(s_reg_mfc_eal);
				const auto lsa = get_reg_fixed<u32>(s_reg_mfc_lsa);
				const auto tag = get_reg_fixed<u8>(s_reg_mfc_tag);

				const auto size = get_reg_fixed<u16>(s_reg_mfc_size);
				const auto mask = m_ir->CreateShl(m_ir->getInt32(1), zext<u32>(tag).value);
				const auto exec = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto next = llvm::BasicBlock::Create(m_context, "", m_function);

				const auto pf = spu_ptr<u32>(&spu_thread::mfc_fence);
				const auto pb = spu_ptr<u32>(&spu_thread::mfc_barrier);

				switch (u64 cmd = ci->getZExtValue())
				{
				case MFC_GETLLAR_CMD:
				case MFC_PUTLLC_CMD:
				case MFC_PUTLLUC_CMD:
				case MFC_PUTQLLUC_CMD:
				case MFC_PUTL_CMD:
				case MFC_PUTLB_CMD:
				case MFC_PUTLF_CMD:
				case MFC_PUTRL_CMD:
				case MFC_PUTRLB_CMD:
				case MFC_PUTRLF_CMD:
				case MFC_GETL_CMD:
				case MFC_GETLB_CMD:
				case MFC_GETLF_CMD:
				{
					// TODO
					m_ir->CreateBr(next);
					m_ir->SetInsertPoint(exec);
					m_ir->CreateUnreachable();
					m_ir->SetInsertPoint(fail);
					m_ir->CreateUnreachable();
					m_ir->SetInsertPoint(next);
					m_ir->CreateStore(ci, spu_ptr<u8>(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::cmd));
					call(&exec_mfc_cmd, m_thread);
					return;
				}
				case MFC_SNDSIG_CMD:
				case MFC_SNDSIGB_CMD:
				case MFC_SNDSIGF_CMD:
				case MFC_PUT_CMD:
				case MFC_PUTB_CMD:
				case MFC_PUTF_CMD:
				case MFC_PUTR_CMD:
				case MFC_PUTRB_CMD:
				case MFC_PUTRF_CMD:
				case MFC_GET_CMD:
				case MFC_GETB_CMD:
				case MFC_GETF_CMD:
				{
					// Try to obtain constant size
					u64 csize = -1;

					if (auto ci = llvm::dyn_cast<llvm::ConstantInt>(size.value))
					{
						csize = ci->getZExtValue();
					}

					if (cmd >= MFC_SNDSIG_CMD && csize != 4)
					{
						csize = -1;
					}

					llvm::Value* src = m_ir->CreateGEP(m_lsptr, zext<u64>(lsa).value);
					llvm::Value* dst = m_ir->CreateGEP(m_memptr, zext<u64>(eal).value);

					if (cmd & MFC_GET_CMD)
					{
						std::swap(src, dst);
					}

					llvm::Value* barrier = m_ir->CreateLoad(pb);

					if (cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK))
					{
						barrier = m_ir->CreateOr(barrier, m_ir->CreateLoad(pf));
					}

					const auto cond = m_ir->CreateIsNull(m_ir->CreateAnd(mask, barrier));
					m_ir->CreateCondBr(cond, exec, fail, m_md_likely);
					m_ir->SetInsertPoint(exec);

					const auto mmio = llvm::BasicBlock::Create(m_context, "", m_function);
					const auto copy = llvm::BasicBlock::Create(m_context, "", m_function);
					m_ir->CreateCondBr(m_ir->CreateICmpUGE(eal.value, m_ir->getInt32(0xe0000000)), mmio, copy, m_md_unlikely);
					m_ir->SetInsertPoint(mmio);
					m_ir->CreateStore(ci, spu_ptr<u8>(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::cmd));
					call(&exec_mfc_cmd, m_thread);
					m_ir->CreateBr(next);
					m_ir->SetInsertPoint(copy);

					llvm::Type* vtype = get_type<u8(*)[16]>();

					switch (csize)
					{
					case 0:
					case UINT64_MAX:
					{
						break;
					}
					case 1:
					{
						vtype = get_type<u8*>();
						break;
					}
					case 2:
					{
						vtype = get_type<u16*>();
						break;
					}
					case 4:
					{
						vtype = get_type<u32*>();
						break;
					}
					case 8:
					{
						vtype = get_type<u64*>();
						break;
					}
					default:
					{
						if (csize % 16 || csize > 0x4000)
						{
							LOG_ERROR(SPU, "[0x%x] MFC_Cmd: invalid size %u", m_pos, csize);
						}
					}
					}

					if (csize > 0 && csize <= 16)
					{
						// Generate single copy operation
						m_ir->CreateStore(m_ir->CreateLoad(m_ir->CreateBitCast(src, vtype), true), m_ir->CreateBitCast(dst, vtype), true);
					}
					else if (csize <= 256)
					{
						// Generate fixed sequence of copy operations
						for (u32 i = 0; i < csize; i += 16)
						{
							const auto _src = m_ir->CreateGEP(src, m_ir->getInt32(i));
							const auto _dst = m_ir->CreateGEP(dst, m_ir->getInt32(i));
							m_ir->CreateStore(m_ir->CreateLoad(m_ir->CreateBitCast(_src, vtype), true), m_ir->CreateBitCast(_dst, vtype), true);
						}
					}
					else
					{
						// TODO
						m_ir->CreateCall(get_intrinsic<u8*, u8*, u32>(llvm::Intrinsic::memcpy), {dst, src, zext<u32>(size).value, m_ir->getTrue()});
					}

					m_ir->CreateBr(next);
					break;
				}
				case MFC_BARRIER_CMD:
				case MFC_EIEIO_CMD:
				case MFC_SYNC_CMD:
				{
					const auto cond = m_ir->CreateIsNull(m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::mfc_size)));
					m_ir->CreateCondBr(cond, exec, fail, m_md_likely);
					m_ir->SetInsertPoint(exec);
					m_ir->CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);
					m_ir->CreateBr(next);
					break;
				}
				default:
				{
					// TODO
					LOG_ERROR(SPU, "[0x%x] MFC_Cmd: unknown command (0x%x)", m_pos, cmd);
					m_ir->CreateBr(next);
					m_ir->SetInsertPoint(exec);
					m_ir->CreateUnreachable();
					break;
				}
				}

				// Fallback: enqueue the command
				m_ir->SetInsertPoint(fail);

				// Get MFC slot, redirect to invalid memory address
				const auto slot = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::mfc_size));
				const auto off0 = m_ir->CreateAdd(m_ir->CreateMul(slot, m_ir->getInt32(sizeof(spu_mfc_cmd))), m_ir->getInt32(::offset32(&spu_thread::mfc_queue)));
				const auto ptr0 = m_ir->CreateGEP(m_thread, m_ir->CreateZExt(off0, get_type<u64>()));
				const auto ptr1 = m_ir->CreateGEP(m_memptr, m_ir->getInt64(0xffdeadf0));
				const auto pmfc = m_ir->CreateSelect(m_ir->CreateICmpULT(slot, m_ir->getInt32(16)), ptr0, ptr1);
				m_ir->CreateStore(ci, _ptr<u8>(pmfc, ::offset32(&spu_mfc_cmd::cmd)));

				switch (u64 cmd = ci->getZExtValue())
				{
				case MFC_GETLLAR_CMD:
				case MFC_PUTLLC_CMD:
				case MFC_PUTLLUC_CMD:
				case MFC_PUTQLLUC_CMD:
				{
					break;
				}
				case MFC_PUTL_CMD:
				case MFC_PUTLB_CMD:
				case MFC_PUTLF_CMD:
				case MFC_PUTRL_CMD:
				case MFC_PUTRLB_CMD:
				case MFC_PUTRLF_CMD:
				case MFC_GETL_CMD:
				case MFC_GETLB_CMD:
				case MFC_GETLF_CMD:
				{
					break;
				}
				case MFC_SNDSIG_CMD:
				case MFC_SNDSIGB_CMD:
				case MFC_SNDSIGF_CMD:
				case MFC_PUT_CMD:
				case MFC_PUTB_CMD:
				case MFC_PUTF_CMD:
				case MFC_PUTR_CMD:
				case MFC_PUTRB_CMD:
				case MFC_PUTRF_CMD:
				case MFC_GET_CMD:
				case MFC_GETB_CMD:
				case MFC_GETF_CMD:
				{
					m_ir->CreateStore(tag.value, _ptr<u8>(pmfc, ::offset32(&spu_mfc_cmd::tag)));
					m_ir->CreateStore(size.value, _ptr<u16>(pmfc, ::offset32(&spu_mfc_cmd::size)));
					m_ir->CreateStore(lsa.value, _ptr<u32>(pmfc, ::offset32(&spu_mfc_cmd::lsa)));
					m_ir->CreateStore(eal.value, _ptr<u32>(pmfc, ::offset32(&spu_mfc_cmd::eal)));
					m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(pb), mask), pb);
					if (cmd & MFC_FENCE_MASK)
						m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(pf), mask), pf);
					break;
				}
				case MFC_BARRIER_CMD:
				case MFC_EIEIO_CMD:
				case MFC_SYNC_CMD:
				{
					m_ir->CreateStore(m_ir->getInt32(-1), pb);
					break;
				}
				default:
				{
					m_ir->CreateUnreachable();
					break;
				}
				}

				m_ir->CreateStore(m_ir->CreateAdd(slot, m_ir->getInt32(1)), spu_ptr<u32>(&spu_thread::mfc_size));
				m_ir->CreateBr(next);
				m_ir->SetInsertPoint(next);
				return;
			}

			// Fallback to unoptimized WRCH implementation (TODO)
			LOG_WARNING(SPU, "[0x%x] MFC_Cmd: $%u is not a constant", m_pos, +op.rt);
			break;
		}
		case MFC_WrListStallAck:
		{
			const auto mask = eval(splat<u32>(1) << (val & 0x1f));
			const auto _ptr = spu_ptr<u32>(&spu_thread::ch_stall_mask);
			const auto _old = m_ir->CreateLoad(_ptr);
			const auto _new = m_ir->CreateAnd(_old, m_ir->CreateNot(mask.value));
			m_ir->CreateStore(_new, _ptr);
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto _mfc = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpNE(_old, _new), _mfc, next);
			m_ir->SetInsertPoint(_mfc);
			call(&exec_list_unstall, m_thread, eval(val & 0x1f).value);
			m_ir->CreateBr(next);
			m_ir->SetInsertPoint(next);
			return;
		}
		case SPU_WrDec:
		{
			m_ir->CreateStore(call(&get_timebased_time), spu_ptr<u64>(&spu_thread::ch_dec_start_timestamp));
			m_ir->CreateStore(val.value, spu_ptr<u32>(&spu_thread::ch_dec_value));
			return;
		}
		case SPU_WrEventMask:
		{
			m_ir->CreateStore(val.value, spu_ptr<u32>(&spu_thread::ch_event_mask))->setVolatile(true);
			return;
		}
		case SPU_WrEventAck:
		{
			m_ir->CreateAtomicRMW(llvm::AtomicRMWInst::And, spu_ptr<u32>(&spu_thread::ch_event_stat), eval(~val).value, llvm::AtomicOrdering::Release);
			return;
		}
		case 69:
		{
			return;
		}
		}

		update_pc();
		const auto succ = call(&exec_wrch, m_thread, m_ir->getInt32(op.ra), val.value);
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

		if (g_cfg.core.spu_block_size == spu_block_size_type::safe && !m_interp_magn)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			m_ir->CreateStore(m_ir->getInt32(m_pos + 4), spu_ptr<u32>(&spu_thread::pc));
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

	void SF(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.rb) - get_vr(op.ra));
	}

	void OR(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) | get_vr(op.rb));
	}

	void BG(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);
		set_vr(op.rt, zext<u32[4]>(a <= b));
	}

	void SFH(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<u16[8]>(op.rb) - get_vr<u16[8]>(op.ra));
	}

	void NOR(spu_opcode_t op)
	{
		set_vr(op.rt, ~(get_vr(op.ra) | get_vr(op.rb)));
	}

	void ABSDB(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);
		set_vr(op.rt, max(a, b) - min(a, b));
	}

	template <typename T>
	void make_spu_rol(spu_opcode_t op, value_t<T> by)
	{
		set_vr(op.rt, rol(get_vr<T>(op.ra), by));
	}

	template <typename R, typename T>
	void make_spu_rotate_mask(spu_opcode_t op, value_t<T> by)
	{
		value_t<R> sh;
		static_assert(sh.esize == by.esize);
		sh.value = m_ir->CreateAnd(m_ir->CreateNeg(by.value), by.esize * 2 - 1);
		if constexpr (!by.is_vector)
			sh.value = m_ir->CreateVectorSplat(sh.is_vector, sh.value);

		set_vr(op.rt, select(sh < by.esize, eval(get_vr<R>(op.ra) >> sh), splat<R>(0)));
	}

	template <typename R, typename T>
	void make_spu_rotate_sext(spu_opcode_t op, value_t<T> by)
	{
		value_t<R> sh;
		static_assert(sh.esize == by.esize);
		sh.value = m_ir->CreateAnd(m_ir->CreateNeg(by.value), by.esize * 2 - 1);
		if constexpr (!by.is_vector)
			sh.value = m_ir->CreateVectorSplat(sh.is_vector, sh.value);

		value_t<R> max_sh = splat<R>(by.esize - 1);
		sh.value = m_ir->CreateSelect(m_ir->CreateICmpUGT(max_sh.value, sh.value), sh.value, max_sh.value);
		set_vr(op.rt, get_vr<R>(op.ra) >> sh);
	}

	template <typename R, typename T>
	void make_spu_shift_left(spu_opcode_t op, value_t<T> by)
	{
		value_t<R> sh;
		static_assert(sh.esize == by.esize);
		sh.value = m_ir->CreateAnd(by.value, by.esize * 2 - 1);
		if constexpr (!by.is_vector)
			sh.value = m_ir->CreateVectorSplat(sh.is_vector, sh.value);

		set_vr(op.rt, select(sh < by.esize, eval(get_vr<R>(op.ra) << sh), splat<R>(0)));
	}

	void ROT(spu_opcode_t op)
	{
		make_spu_rol(op, get_vr<u32[4]>(op.rb));
	}

	void ROTM(spu_opcode_t op)
	{
		make_spu_rotate_mask<u32[4]>(op, get_vr(op.rb));
	}

	void ROTMA(spu_opcode_t op)
	{
		make_spu_rotate_sext<s32[4]>(op, get_vr(op.rb));
	}

	void SHL(spu_opcode_t op)
	{
		make_spu_shift_left<u32[4]>(op, get_vr(op.rb));
	}

	void ROTH(spu_opcode_t op)
	{
		make_spu_rol(op, get_vr<u16[8]>(op.rb));
	}

	void ROTHM(spu_opcode_t op)
	{
		make_spu_rotate_mask<u16[8]>(op, get_vr<u16[8]>(op.rb));
	}

	void ROTMAH(spu_opcode_t op)
	{
		make_spu_rotate_sext<s16[8]>(op, get_vr<s16[8]>(op.rb));
	}

	void SHLH(spu_opcode_t op)
	{
		make_spu_shift_left<u16[8]>(op, get_vr<u16[8]>(op.rb));
	}

	void ROTI(spu_opcode_t op)
	{
		make_spu_rol(op, get_imm<u32[4]>(op.i7, false));
	}

	void ROTMI(spu_opcode_t op)
	{
		make_spu_rotate_mask<u32[4]>(op, get_imm<u32>(op.i7, false));
	}

	void ROTMAI(spu_opcode_t op)
	{
		make_spu_rotate_sext<s32[4]>(op, get_imm<u32>(op.i7, false));
	}

	void SHLI(spu_opcode_t op)
	{
		make_spu_shift_left<u32[4]>(op, get_imm<u32>(op.i7, false));
	}

	void ROTHI(spu_opcode_t op)
	{
		make_spu_rol(op, get_imm<u16[8]>(op.i7, false));
	}

	void ROTHMI(spu_opcode_t op)
	{
		make_spu_rotate_mask<u16[8]>(op, get_imm<u16>(op.i7, false));
	}

	void ROTMAHI(spu_opcode_t op)
	{
		make_spu_rotate_sext<s16[8]>(op, get_imm<u16>(op.i7, false));
	}

	void SHLHI(spu_opcode_t op)
	{
		make_spu_shift_left<u16[8]>(op, get_imm<u16>(op.i7, false));
	}

	void A(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) + get_vr(op.rb));
	}

	void AND(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) & get_vr(op.rb));
	}

	void CG(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);
		set_vr(op.rt, zext<u32[4]>(a + b < a));
	}

	void AH(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<u16[8]>(op.ra) + get_vr<u16[8]>(op.rb));
	}

	void NAND(spu_opcode_t op)
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
		const auto a = eval(extract(get_vr(op.ra), 3) + get_imm<u32>(op.i7));
		const auto i = eval(~a & 0xf);
		auto r = build<u8[16]>(0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt8(0x3), i.value);
		set_vr(op.rt, r);
	}

	void CHD(spu_opcode_t op)
	{
		const auto a = eval(extract(get_vr(op.ra), 3) + get_imm<u32>(op.i7));
		const auto i = eval(~a >> 1 & 0x7);
		auto r = build<u16[8]>(0x1e1f, 0x1c1d, 0x1a1b, 0x1819, 0x1617, 0x1415, 0x1213, 0x1011);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt16(0x0203), i.value);
		set_vr(op.rt, r);
	}

	void CWD(spu_opcode_t op)
	{
		const auto a = eval(extract(get_vr(op.ra), 3) + get_imm<u32>(op.i7));
		const auto i = eval(~a >> 2 & 0x3);
		auto r = build<u32[4]>(0x1c1d1e1f, 0x18191a1b, 0x14151617, 0x10111213);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt32(0x010203), i.value);
		set_vr(op.rt, r);
	}

	void CDD(spu_opcode_t op)
	{
		const auto a = eval(extract(get_vr(op.ra), 3) + get_imm<u32>(op.i7));
		const auto i = eval(~a >> 3 & 0x1);
		auto r = build<u64[2]>(0x18191a1b1c1d1e1f, 0x1011121314151617);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt64(0x01020304050607), i.value);
		set_vr(op.rt, r);
	}

	void ROTQBII(spu_opcode_t op)
	{
		const auto a = get_vr<u64[2]>(op.ra);
		const auto b = eval(get_imm<u64[2]>(op.i7, false) & 0x7);
		set_vr(op.rt, a << b | zshuffle<u64[2]>(a, 1, 0) >> 56 >> (8 - b));
	}

	void ROTQMBII(spu_opcode_t op)
	{
		const auto a = get_vr<u64[2]>(op.ra);
		const auto b = eval(-get_imm<u64[2]>(op.i7, false) & 0x7);
		set_vr(op.rt, a >> b | zshuffle<u64[2]>(a, 1, 2) << 56 << (8 - b));
	}

	void SHLQBII(spu_opcode_t op)
	{
		const auto a = get_vr<u64[2]>(op.ra);
		const auto b = eval(get_imm<u64[2]>(op.i7, false) & 0x7);
		set_vr(op.rt, a << b | zshuffle<u64[2]>(a, 2, 0) >> 56 >> (8 - b));
	}

	void ROTQBYI(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		auto sh = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		sh = eval((sh - get_imm<u8[16]>(op.i7, false)) & 0xf);
		set_vr(op.rt, pshufb(a, sh));
	}

	void ROTQMBYI(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		auto sh = build<u8[16]>(112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127);
		sh = eval(sh + (-get_imm<u8[16]>(op.i7, false) & 0x1f));
		set_vr(op.rt, pshufb(a, sh));
	}

	void SHLQBYI(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		auto sh = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		sh = eval(sh - (get_imm<u8[16]>(op.i7, false) & 0x1f));
		set_vr(op.rt, pshufb(a, sh));
	}

	void CGT(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr<s32[4]>(op.ra) > get_vr<s32[4]>(op.rb)));
	}

	void XOR(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) ^ get_vr(op.rb));
	}

	void CGTH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<s16[8]>(op.ra) > get_vr<s16[8]>(op.rb)));
	}

	void EQV(spu_opcode_t op)
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

	void ANDC(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) & ~get_vr(op.rb));
	}

	void CLGTH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) > get_vr<u16[8]>(op.rb)));
	}

	void ORC(spu_opcode_t op)
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
		const auto x = eval(~get_vr<u32[4]>(op.rt) & 1);
		const auto s = eval(a + b);
		set_vr(op.rt, zext<u32[4]>((sext<u32[4]>(s < a) | (s & ~x)) == -1));
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
		if (m_interp_magn)
		{
			const auto m = bitcast<bool[16]>(get_imm<u16>(op.i16));
			set_vr(op.rt, sext<u8[16]>(m));
			return;
		}

		v128 data;
		for (u32 i = 0; i < 16; i++)
			data._bytes[i] = op.i16 & (1u << i) ? -1 : 0;
		value_t<u8[16]> r;
		r.value = make_const_vector<v128>(data, get_type<u8[16]>());
		set_vr(op.rt, r);
	}

	void IL(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm<s32[4]>(op.si16));
	}

	void ILHU(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm<u32[4]>(op.i16) << 16);
	}

	void ILH(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm<u16[8]>(op.i16));
	}

	void IOHL(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.rt) | get_imm(op.i16));
	}

	void ORI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) | get_imm<s32[4]>(op.si10));
	}

	void ORHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) | get_imm<s16[8]>(op.si10));
	}

	void ORBI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s8[16]>(op.ra) | get_imm<s8[16]>(op.si10));
	}

	void SFI(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm<s32[4]>(op.si10) - get_vr<s32[4]>(op.ra));
	}

	void SFHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm<s16[8]>(op.si10) - get_vr<s16[8]>(op.ra));
	}

	void ANDI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) & get_imm<s32[4]>(op.si10));
	}

	void ANDHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) & get_imm<s16[8]>(op.si10));
	}

	void ANDBI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s8[16]>(op.ra) & get_imm<s8[16]>(op.si10));
	}

	void AI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) + get_imm<s32[4]>(op.si10));
	}

	void AHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) + get_imm<s16[8]>(op.si10));
	}

	void XORI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) ^ get_imm<s32[4]>(op.si10));
	}

	void XORHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) ^ get_imm<s16[8]>(op.si10));
	}

	void XORBI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s8[16]>(op.ra) ^ get_imm<s8[16]>(op.si10));
	}

	void CGTI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr<s32[4]>(op.ra) > get_imm<s32[4]>(op.si10)));
	}

	void CGTHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<s16[8]>(op.ra) > get_imm<s16[8]>(op.si10)));
	}

	void CGTBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<s8[16]>(op.ra) > get_imm<s8[16]>(op.si10)));
	}

	void CLGTI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr(op.ra) > get_imm(op.si10)));
	}

	void CLGTHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) > get_imm<u16[8]>(op.si10)));
	}

	void CLGTBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<u8[16]>(op.ra) > get_imm<u8[16]>(op.si10)));
	}

	void MPYI(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) << 16 >> 16) * get_imm<s32[4]>(op.si10));
	}

	void MPYUI(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) << 16 >> 16) * (get_imm(op.si10) & 0xffff));
	}

	void CEQI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr(op.ra) == get_imm(op.si10)));
	}

	void CEQHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) == get_imm<u16[8]>(op.si10)));
	}

	void CEQBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<u8[16]>(op.ra) == get_imm<u8[16]>(op.si10)));
	}

	void ILA(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm(op.i18));
	}

	void SELB(spu_opcode_t op)
	{
		if (auto ei = llvm::dyn_cast_or_null<llvm::CastInst>(get_reg_raw(op.rc)))
		{
			// Detect if the mask comes from a comparison instruction
			if (ei->getOpcode() == llvm::Instruction::SExt && ei->getSrcTy()->isIntOrIntVectorTy(1))
			{
				auto op0 = ei->getOperand(0);
				auto typ = ei->getDestTy();
				auto op1 = get_reg_raw(op.rb);
				auto op2 = get_reg_raw(op.ra);

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
					else if (op1 && op1->getType() == get_type<f64[4]>() || op2 && op2->getType() == get_type<f64[4]>())
					{
						op1 = get_vr<f64[4]>(op.rb).value;
						op2 = get_vr<f64[4]>(op.ra).value;
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
					set_reg_fixed(op.rt4, m_ir->CreateSelect(op0, op1, op2));
					return;
				}
			}
		}

		const auto op1 = get_reg_raw(op.rb);
		const auto op2 = get_reg_raw(op.ra);

		if (op1 && op1->getType() == get_type<f64[4]>() || op2 && op2->getType() == get_type<f64[4]>())
		{
			// Optimization: keep xfloat values in doubles even if the mask is unpredictable (hard way)
			const auto c = get_vr<u32[4]>(op.rc);
			const auto b = get_vr<f64[4]>(op.rb);
			const auto a = get_vr<f64[4]>(op.ra);
			const auto m = conv_xfloat_mask(c.value);
			const auto x = m_ir->CreateAnd(double_as_uint64(b.value), m);
			const auto y = m_ir->CreateAnd(double_as_uint64(a.value), m_ir->CreateNot(m));
			set_reg_fixed(op.rt4, uint64_as_double(m_ir->CreateOr(x, y)));
			return;
		}

		set_vr(op.rt4, merge(get_vr(op.rc), get_vr(op.rb), get_vr(op.ra)));
	}

	void SHUFB(spu_opcode_t op) //
	{
		if (auto ii = llvm::dyn_cast_or_null<llvm::InsertElementInst>(get_reg_raw(op.rc)))
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
				set_reg_fixed(op.rt4, m_ir->CreateInsertElement(get_reg_fixed(op.rb, vtype), _new, ii->getOperand(2)));
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
						const auto a = get_reg_fixed(op.ra, t);
						const auto b = get_reg_fixed(op.rb, t);
						const auto e = m_ir->CreateExtractElement(a, cm.extract_from);
						set_reg_fixed(op.rt4, m_ir->CreateInsertElement(b, e, cm.insert_to));
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
				set_reg_fixed(op.rt4, m_ir->CreateShuffleVector(b.value, op.ra == op.rb ? b.value : a.value, m_ir->CreateZExt(c, get_type<u32[16]>())));
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
		return UNK(op);
	}

	void DFCMEQ(spu_opcode_t op) //
	{
		return UNK(op);
	}

	void DFTSV(spu_opcode_t op) //
	{
		return UNK(op);
	}

	void DFA(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) + get_vr<f64[2]>(op.rb));
	}

	void DFS(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) - get_vr<f64[2]>(op.rb));
	}

	void DFM(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb));
	}

	void DFMA(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb) + get_vr<f64[2]>(op.rt));
	}

	void DFMS(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb) - get_vr<f64[2]>(op.rt));
	}

	void DFNMS(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.rt) - get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb));
	}

	void DFNMA(spu_opcode_t op)
	{
		set_vr(op.rt, -(get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb) + get_vr<f64[2]>(op.rt)));
	}

	void FREST(spu_opcode_t op)
	{
		// TODO
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt, fsplat<f64[4]>(1.0) / get_vr<f64[4]>(op.ra));
		else
			set_vr(op.rt, fsplat<f32[4]>(1.0) / get_vr<f32[4]>(op.ra));
	}

	void FRSQEST(spu_opcode_t op)
	{
		// TODO
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt, fsplat<f64[4]>(1.0) / sqrt(fabs(get_vr<f64[4]>(op.ra))));
		else
			set_vr(op.rt, fsplat<f32[4]>(1.0) / sqrt(fabs(get_vr<f32[4]>(op.ra))));
	}

	void FCGT(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OGT>(get_vr<f64[4]>(op.ra), get_vr<f64[4]>(op.rb))));
			return;
		}

		const auto a = get_vr<f32[4]>(op.ra);
		const auto b = get_vr<f32[4]>(op.rb);

		// See FCMGT.
		if (g_cfg.core.spu_approx_xfloat)
		{
			const auto ia = bitcast<s32[4]>(fabs(a));
			const auto ib = bitcast<s32[4]>(fabs(b));
			const auto nz = eval((ia > 0x7fffff) | (ib > 0x7fffff));

			// Use sign bits to invert abs values before comparison.
			const auto ca = eval(ia ^ (bitcast<s32[4]>(a) >> 31));
			const auto cb = eval(ib ^ (bitcast<s32[4]>(b) >> 31));
			set_vr(op.rt, sext<u32[4]>((ca > cb) & nz));
		}
		else
		{
			set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OGT>(a, b)));
		}
	}

	void FCMGT(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OGT>(fabs(get_vr<f64[4]>(op.ra)), fabs(get_vr<f64[4]>(op.rb)))));
			return;
		}

		const auto a = get_vr<f32[4]>(op.ra);
		const auto b = get_vr<f32[4]>(op.rb);
		const auto abs_a = fabs(a);
		const auto abs_b = fabs(b);

		// Actually, it's accurate and can be used as an alternative path for accurate xfloat.
		if (g_cfg.core.spu_approx_xfloat)
		{
			// Compare abs values as integers, but return false if both are denormals or zeros.
			const auto ia = bitcast<s32[4]>(abs_a);
			const auto ib = bitcast<s32[4]>(abs_b);
			const auto nz = eval((ia > 0x7fffff) | (ib > 0x7fffff));
			set_vr(op.rt, sext<u32[4]>((ia > ib) & nz));
		}
		else
		{
			set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OGT>(abs_a, abs_b)));
		}
	}

	void FA(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt, get_vr<f64[4]>(op.ra) + get_vr<f64[4]>(op.rb));
		else
			set_vr(op.rt, get_vr<f32[4]>(op.ra) + get_vr<f32[4]>(op.rb));
	}

	void FS(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt, get_vr<f64[4]>(op.ra) - get_vr<f64[4]>(op.rb));
		else
			set_vr(op.rt, get_vr<f32[4]>(op.ra) - get_vr<f32[4]>(op.rb));
	}

	void FM(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt, get_vr<f64[4]>(op.ra) * get_vr<f64[4]>(op.rb));
		else if (g_cfg.core.spu_approx_xfloat)
		{
			const auto a = get_vr<f32[4]>(op.ra);
			const auto b = get_vr<f32[4]>(op.rb);
			const auto m = eval(a * b);
			const auto abs_a = bitcast<s32[4]>(fabs(a));
			const auto abs_b = bitcast<s32[4]>(fabs(b));
			const auto abs_m = bitcast<s32[4]>(fabs(m));
			const auto sign_a = eval(bitcast<s32[4]>(a) & 0x80000000);
			const auto sign_b = eval(bitcast<s32[4]>(b) & 0x80000000);
			const auto smod_m = eval(bitcast<s32[4]>(m) & 0x7fffffff);
			const auto fmax_m = eval((sign_a ^ sign_b) | 0x7fffffff);
			const auto nzero = eval((abs_a > 0x7fffff) & (abs_b > 0x7fffff) & (abs_m > 0x7fffff));

			// If m produces Inf or NaN, flush it to max xfloat with appropriate sign
			const auto clamp = select(smod_m > 0x7f7fffff, bitcast<f32[4]>(fmax_m), m);

			// If a, b, or a * b is a denorm or zero, return zero
			set_vr(op.rt, select(nzero, clamp, fsplat<f32[4]>(0.)));
		}
		else
			set_vr(op.rt, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb));
	}

	void FESD(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			const auto r = shuffle2<f64[2]>(get_vr<f64[4]>(op.ra), fsplat<f64[4]>(0.), 1, 3);
			const auto d = bitcast<s64[2]>(r);
			const auto a = eval(d & 0x7fffffffffffffff);
			const auto s = eval(d & 0x8000000000000000);
			const auto i = select(a == 0x47f0000000000000, eval(s | 0x7ff0000000000000), d);
			const auto n = select(a > 0x47f0000000000000, splat<s64[2]>(0x7ff8000000000000), i);
			set_vr(op.rt, bitcast<f64[2]>(n));
		}
		else
		{
			value_t<f64[2]> r;
			r.value = m_ir->CreateFPExt(shuffle2<f32[2]>(get_vr<f32[4]>(op.ra), fsplat<f32[4]>(0.), 1, 3).value, get_type<f64[2]>());
			set_vr(op.rt, r);
		}
	}

	void FRDS(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			const auto r = get_vr<f64[2]>(op.ra);
			const auto d = bitcast<s64[2]>(r);
			const auto a = eval(d & 0x7fffffffffffffff);
			const auto s = eval(d & 0x8000000000000000);
			const auto i = select(a > 0x47f0000000000000, eval(s | 0x47f0000000000000), d);
			const auto n = select(a > 0x7ff0000000000000, splat<s64[2]>(0x47f8000000000000), i);
			const auto z = select(a < 0x3810000000000000, s, n);
			set_vr(op.rt, shuffle2<f64[4]>(bitcast<f64[2]>(z), fsplat<f64[2]>(0.), 2, 0, 3, 1), false);
		}
		else
		{
			value_t<f32[2]> r;
			r.value = m_ir->CreateFPTrunc(get_vr<f64[2]>(op.ra).value, get_type<f32[2]>());
			set_vr(op.rt, shuffle2<f32[4]>(r, fsplat<f32[2]>(0.), 2, 0, 3, 1));
		}
	}

	void FCEQ(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OEQ>(get_vr<f64[4]>(op.ra), get_vr<f64[4]>(op.rb))));
		else
			set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OEQ>(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb))));
	}

	void FCMEQ(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OEQ>(fabs(get_vr<f64[4]>(op.ra)), fabs(get_vr<f64[4]>(op.rb)))));
		else
			set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OEQ>(fabs(get_vr<f32[4]>(op.ra)), fabs(get_vr<f32[4]>(op.rb)))));
	}

	// Multiply and return zero if any of the arguments is in the xfloat range.
	value_t<f32[4]> mzero_if_xtended(value_t<f32[4]> a, value_t<f32[4]> b)
	{
		// Compare absolute values with max positive float in normal range.
		const auto aa = bitcast<s32[4]>(fabs(a));
		const auto ab = bitcast<s32[4]>(fabs(b));
		return select(eval(max(aa, ab) > 0x7f7fffff), fsplat<f32[4]>(0.), eval(a * b));
	}

	void FNMS(spu_opcode_t op)
	{
		// See FMA.
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt4, -fmuladd(get_vr<f64[4]>(op.ra), get_vr<f64[4]>(op.rb), eval(-get_vr<f64[4]>(op.rc))));
		else if (g_cfg.core.spu_approx_xfloat)
			set_vr(op.rt4, get_vr<f32[4]>(op.rc) - mzero_if_xtended(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb)));
		else
			set_vr(op.rt4, get_vr<f32[4]>(op.rc) - get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb));
	}

	void FMA(spu_opcode_t op)
	{
		// Hardware FMA produces the same result as multiple + add on the limited double range (xfloat).
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt4, fmuladd(get_vr<f64[4]>(op.ra), get_vr<f64[4]>(op.rb), get_vr<f64[4]>(op.rc)));
		else if (g_cfg.core.spu_approx_xfloat)
			set_vr(op.rt4, mzero_if_xtended(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb)) + get_vr<f32[4]>(op.rc));
		else
			set_vr(op.rt4, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb) + get_vr<f32[4]>(op.rc));
	}

	void FMS(spu_opcode_t op)
	{
		// See FMA.
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt4, fmuladd(get_vr<f64[4]>(op.ra), get_vr<f64[4]>(op.rb), eval(-get_vr<f64[4]>(op.rc))));
		else if (g_cfg.core.spu_approx_xfloat)
			set_vr(op.rt4, mzero_if_xtended(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb)) - get_vr<f32[4]>(op.rc));
		else
			set_vr(op.rt4, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb) - get_vr<f32[4]>(op.rc));
	}

	void FI(spu_opcode_t op)
	{
		// TODO
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt, get_vr<f64[4]>(op.rb));
		else
			set_vr(op.rt, get_vr<f32[4]>(op.rb));
	}

	void CFLTS(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			value_t<f64[4]> a = get_vr<f64[4]>(op.ra);
			value_t<f64[4]> s;
			if (m_interp_magn)
				s = vsplat<f64[4]>(bitcast<f64>(((1023 + 173) - get_imm<u64>(op.i8)) << 52));
			else
				s = fsplat<f64[4]>(std::exp2(static_cast<int>(173 - op.i8)));
			if (op.i8 != 173 || m_interp_magn)
				a = eval(a * s);

			value_t<s32[4]> r;

			if (auto ca = llvm::dyn_cast<llvm::ConstantDataVector>(a.value))
			{
				const f64 data[4]
				{
					ca->getElementAsDouble(0),
					ca->getElementAsDouble(1),
					ca->getElementAsDouble(2),
					ca->getElementAsDouble(3)
				};

				v128 result;

				for (u32 i = 0; i < 4; i++)
				{
					if (data[i] >= std::exp2(31.f))
					{
						result._s32[i] = INT32_MAX;
					}
					else if (data[i] < std::exp2(-31.f))
					{
						result._s32[i] = INT32_MIN;
					}
					else
					{
						result._s32[i] = static_cast<s32>(data[i]);
					}
				}

				r.value = make_const_vector(result, get_type<s32[4]>());
				set_vr(op.rt, r);
				return;
			}

			if (llvm::isa<llvm::ConstantAggregateZero>(a.value))
			{
				set_vr(op.rt, splat<u32[4]>(0));
				return;
			}

			r.value = m_ir->CreateFPToSI(a.value, get_type<s32[4]>());
			set_vr(op.rt, r ^ sext<s32[4]>(fcmp<llvm::FCmpInst::FCMP_OGE>(a, fsplat<f64[4]>(std::exp2(31.f)))));
		}
		else
		{
			value_t<f32[4]> a = get_vr<f32[4]>(op.ra);
			value_t<f32[4]> s;
			if (m_interp_magn)
				s = vsplat<f32[4]>(load_const<f32>(m_scale_float_to, get_imm<u8>(op.i8)));
			else
				s = fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8))));
			if (op.i8 != 173 || m_interp_magn)
				a = eval(a * s);

			value_t<s32[4]> r;
			r.value = m_ir->CreateFPToSI(a.value, get_type<s32[4]>());
			set_vr(op.rt, r ^ sext<s32[4]>(fcmp<llvm::FCmpInst::FCMP_OGE>(a, fsplat<f32[4]>(std::exp2(31.f)))));
		}
	}

	void CFLTU(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			value_t<f64[4]> a = get_vr<f64[4]>(op.ra);
			value_t<f64[4]> s;
			if (m_interp_magn)
				s = vsplat<f64[4]>(bitcast<f64>(((1023 + 173) - get_imm<u64>(op.i8)) << 52));
			else
				s = fsplat<f64[4]>(std::exp2(static_cast<int>(173 - op.i8)));
			if (op.i8 != 173 || m_interp_magn)
				a = eval(a * s);

			value_t<s32[4]> r;

			if (auto ca = llvm::dyn_cast<llvm::ConstantDataVector>(a.value))
			{
				const f64 data[4]
				{
					ca->getElementAsDouble(0),
					ca->getElementAsDouble(1),
					ca->getElementAsDouble(2),
					ca->getElementAsDouble(3)
				};

				v128 result;

				for (u32 i = 0; i < 4; i++)
				{
					if (data[i] >= std::exp2(32.f))
					{
						result._u32[i] = UINT32_MAX;
					}
					else if (data[i] < 0.)
					{
						result._u32[i] = 0;
					}
					else
					{
						result._u32[i] = static_cast<u32>(data[i]);
					}
				}

				r.value = make_const_vector(result, get_type<s32[4]>());
				set_vr(op.rt, r);
				return;
			}

			if (llvm::isa<llvm::ConstantAggregateZero>(a.value))
			{
				set_vr(op.rt, splat<u32[4]>(0));
				return;
			}

			r.value = m_ir->CreateFPToUI(a.value, get_type<s32[4]>());
			set_vr(op.rt, r & sext<s32[4]>(fcmp<llvm::FCmpInst::FCMP_OGE>(a, fsplat<f64[4]>(0.))));
		}
		else
		{
			value_t<f32[4]> a = get_vr<f32[4]>(op.ra);
			value_t<f32[4]> s;
			if (m_interp_magn)
				s = vsplat<f32[4]>(load_const<f32>(m_scale_float_to, get_imm<u8>(op.i8)));
			else
				s = fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8))));
			if (op.i8 != 173 || m_interp_magn)
				a = eval(a * s);

			value_t<s32[4]> r;
			r.value = m_ir->CreateFPToUI(a.value, get_type<s32[4]>());
			set_vr(op.rt, r & ~(bitcast<s32[4]>(a) >> 31));
		}
	}

	void CSFLT(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			value_t<s32[4]> a = get_vr<s32[4]>(op.ra);
			value_t<f64[4]> r;

			if (auto ca = llvm::dyn_cast<llvm::Constant>(a.value))
			{
				v128 data = get_const_vector(ca, m_pos, 25971);
				r = build<f64[4]>(data._s32[0], data._s32[1], data._s32[2], data._s32[3]);
			}
			else
			{
				r.value = m_ir->CreateSIToFP(a.value, get_type<f64[4]>());
			}

			value_t<f64[4]> s;
			if (m_interp_magn)
				s = vsplat<f64[4]>(bitcast<f64>((get_imm<u64>(op.i8) + (1023 - 155)) << 52));
			else
				s = fsplat<f64[4]>(std::exp2(static_cast<int>(op.i8 - 155)));
			if (op.i8 != 155 || m_interp_magn)
				r = eval(r * s);
			set_vr(op.rt, r);
		}
		else
		{
			value_t<f32[4]> r;
			r.value = m_ir->CreateSIToFP(get_vr<s32[4]>(op.ra).value, get_type<f32[4]>());
			value_t<f32[4]> s;
			if (m_interp_magn)
				s = vsplat<f32[4]>(load_const<f32>(m_scale_to_float, get_imm<u8>(op.i8)));
			else
				s = fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155))));
			if (op.i8 != 155 || m_interp_magn)
				r = eval(r * s);
			set_vr(op.rt, r);
		}
	}

	void CUFLT(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			value_t<s32[4]> a = get_vr<s32[4]>(op.ra);
			value_t<f64[4]> r;

			if (auto ca = llvm::dyn_cast<llvm::Constant>(a.value))
			{
				v128 data = get_const_vector(ca, m_pos, 20971);
				r = build<f64[4]>(data._u32[0], data._u32[1], data._u32[2], data._u32[3]);
			}
			else
			{
				r.value = m_ir->CreateUIToFP(a.value, get_type<f64[4]>());
			}

			value_t<f64[4]> s;
			if (m_interp_magn)
				s = vsplat<f64[4]>(bitcast<f64>((get_imm<u64>(op.i8) + (1023 - 155)) << 52));
			else
				s = fsplat<f64[4]>(std::exp2(static_cast<int>(op.i8 - 155)));
			if (op.i8 != 155 || m_interp_magn)
				r = eval(r * s);
			set_vr(op.rt, r);
		}
		else
		{
			value_t<f32[4]> r;
			r.value = m_ir->CreateUIToFP(get_vr<s32[4]>(op.ra).value, get_type<f32[4]>());
			value_t<f32[4]> s;
			if (m_interp_magn)
				s = vsplat<f32[4]>(load_const<f32>(m_scale_to_float, get_imm<u8>(op.i8)));
			else
				s = fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155))));
			if (op.i8 != 155 || m_interp_magn)
				r = eval(r * s);
			set_vr(op.rt, r);
		}
	}

	void STQX(spu_opcode_t op)
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) & 0x3fff0);
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
	}

	void LQX(spu_opcode_t op)
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) & 0x3fff0);
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void STQA(spu_opcode_t op)
	{
		value_t<u64> addr = eval((get_imm<u64>(op.i16, false) << 2) & 0x3fff0);
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
	}

	void LQA(spu_opcode_t op)
	{
		value_t<u64> addr = eval((get_imm<u64>(op.i16, false) << 2) & 0x3fff0);
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void STQR(spu_opcode_t op) //
	{
		value_t<u64> addr;
		addr.value = m_interp_magn ? m_interp_pc : m_ir->getInt32(m_pos);
		addr = eval(((get_imm<u64>(op.i16, false) << 2) + zext<u64>(addr)) & 0x3fff0);
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
	}

	void LQR(spu_opcode_t op) //
	{
		value_t<u64> addr;
		addr.value = m_interp_magn ? m_interp_pc : m_ir->getInt32(m_pos);
		addr = eval(((get_imm<u64>(op.i16, false) << 2) + zext<u64>(addr)) & 0x3fff0);
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void STQD(spu_opcode_t op)
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + (get_imm<u32>(op.si10) << 4)) & 0x3fff0);
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
	}

	void LQD(spu_opcode_t op)
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + (get_imm<u32>(op.si10) << 4)) & 0x3fff0);
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void make_halt(value_t<bool> cond)
	{
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto halt = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, halt, next, m_md_unlikely);
		m_ir->SetInsertPoint(halt);
		if (m_interp_magn)
			m_ir->CreateStore(&*(m_function->arg_begin() + 2), spu_ptr<u32>(&spu_thread::pc))->setVolatile(true);
		const auto pstatus = spu_ptr<u32>(&spu_thread::status);
		const auto chalt = m_ir->getInt32(SPU_STATUS_STOPPED_BY_HALT);
		m_ir->CreateAtomicRMW(llvm::AtomicRMWInst::Or, pstatus, chalt, llvm::AtomicOrdering::Release)->setVolatile(true);
		const auto ptr = _ptr<u32>(m_memptr, 0xffdead00);
		m_ir->CreateStore(m_ir->getInt32("HALT"_u32), ptr)->setVolatile(true);
		m_ir->CreateBr(next);
		m_ir->SetInsertPoint(next);
	}

	void HGT(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr<s32[4]>(op.ra), 3) > extract(get_vr<s32[4]>(op.rb), 3));
		make_halt(cond);
	}

	void HEQ(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) == extract(get_vr(op.rb), 3));
		make_halt(cond);
	}

	void HLGT(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) > extract(get_vr(op.rb), 3));
		make_halt(cond);
	}

	void HGTI(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr<s32[4]>(op.ra), 3) > get_imm<s32>(op.si10));
		make_halt(cond);
	}

	void HEQI(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) == get_imm<u32>(op.si10));
		make_halt(cond);
	}

	void HLGTI(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) > get_imm<u32>(op.si10));
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
	static u32 exec_check_interrupts(spu_thread* _spu, u32 addr)
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
		if (m_interp_magn)
		{
			m_interp_bblock = llvm::BasicBlock::Create(m_context, "", m_function);

			const auto cblock = m_ir->GetInsertBlock();
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto e_exec = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto d_test = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto d_exec = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto d_done = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);
			m_ir->CreateCondBr(get_imm<bool>(op.e).value, e_exec, d_test, m_md_unlikely);
			m_ir->SetInsertPoint(e_exec);
			const auto e_addr = call(&exec_check_interrupts, m_thread, addr.value);
			m_ir->CreateBr(d_test);
			m_ir->SetInsertPoint(d_test);
			const auto target = m_ir->CreatePHI(get_type<u32>(), 2);
			target->addIncoming(addr.value, result);
			target->addIncoming(e_addr, e_exec);
			m_ir->CreateCondBr(get_imm<bool>(op.d).value, d_exec, d_done, m_md_unlikely);
			m_ir->SetInsertPoint(d_exec);
			m_ir->CreateStore(m_ir->getFalse(), spu_ptr<bool>(&spu_thread::interrupts_enabled))->setVolatile(true);
			m_ir->CreateBr(d_done);
			m_ir->SetInsertPoint(d_done);
			m_ir->CreateBr(m_interp_bblock);
			m_ir->SetInsertPoint(cblock);
			m_interp_pc = target;
			return result;
		}

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
			sp = eval(extract(get_reg_fixed(1), 3) & 0x3fff0);
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
			m_ir->CreateStore(m_ir->getFalse(), spu_ptr<bool>(&spu_thread::interrupts_enabled))->setVolatile(true);
		}

		m_ir->CreateStore(addr.value, spu_ptr<u32>(&spu_thread::pc));
		const auto type = llvm::FunctionType::get(get_type<void>(), {get_type<u8*>(), get_type<u8*>(), get_type<u32>()}, false)->getPointerTo()->getPointerTo();
		const auto disp = m_ir->CreateIntToPtr(m_ir->getInt64((u64)spu_runtime::g_dispatcher), type);
		const auto ad64 = m_ir->CreateZExt(addr.value, get_type<u64>());

		if (ret && g_cfg.core.spu_block_size != spu_block_size_type::safe)
		{
			// Compare address stored in stack mirror with addr
			const auto stack0 = eval(zext<u64>(sp) + ::offset32(&spu_thread::stack_mirror));
			const auto stack1 = eval(stack0 + 8);
			const auto _ret = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack0.value), type));
			const auto link = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack1.value), get_type<u64*>()));
			const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto done = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpEQ(ad64, link), done, fail, m_md_likely);
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

	llvm::BasicBlock* add_block_next()
	{
		if (m_interp_magn)
		{
			const auto cblock = m_ir->GetInsertBlock();
			m_ir->SetInsertPoint(m_interp_bblock);
			const auto target = m_ir->CreatePHI(get_type<u32>(), 2);
			target->addIncoming(m_interp_pc_next, cblock);
			target->addIncoming(m_interp_pc, m_interp_bblock->getSinglePredecessor());
			m_ir->SetInsertPoint(cblock);
			m_interp_pc = target;
			return m_interp_bblock;
		}

		return add_block(m_pos + 4);
	}

	void BIZ(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		const auto cond = eval(extract(get_vr(op.rt), 3) == 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block_next());
	}

	void BINZ(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		const auto cond = eval(extract(get_vr(op.rt), 3) != 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block_next());
	}

	void BIHZ(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) == 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block_next());
	}

	void BIHNZ(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) != 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block_next());
	}

	void BI(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);

		if (m_interp_magn)
		{
			m_ir->CreateBr(add_block_indirect(op, addr));
			return;
		}

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
			m_ir->CreateStore(addr.value, spu_ptr<u32>(&spu_thread::pc));
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
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		set_link(op);
		m_ir->CreateBr(add_block_indirect(op, addr, false));
	}

	void IRET(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		value_t<u32> srr0;
		srr0.value = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::srr0));
		m_ir->CreateBr(add_block_indirect(op, srr0));
	}

	void BISLED(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		set_link(op);
		value_t<u32> res;
		res.value = call(&exec_get_events, m_thread);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(m_ir->CreateICmpNE(res.value, m_ir->getInt32(0)), target, add_block_next());
	}

	void BRZ(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			value_t<u32> target;
			target.value = m_interp_pc;
			target = eval((target + (get_imm<u32>(op.i16, false) << 2)) & 0x3fffc);
			m_interp_pc = m_ir->CreateSelect(eval(extract(get_vr(op.rt), 3) == 0).value, target.value, m_interp_pc_next);
			return;
		}

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
		if (m_interp_magn)
		{
			value_t<u32> target;
			target.value = m_interp_pc;
			target = eval((target + (get_imm<u32>(op.i16, false) << 2)) & 0x3fffc);
			m_interp_pc = m_ir->CreateSelect(eval(extract(get_vr(op.rt), 3) != 0).value, target.value, m_interp_pc_next);
			return;
		}

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
		if (m_interp_magn)
		{
			value_t<u32> target;
			target.value = m_interp_pc;
			target = eval((target + (get_imm<u32>(op.i16, false) << 2)) & 0x3fffc);
			m_interp_pc = m_ir->CreateSelect(eval(extract(get_vr<u16[8]>(op.rt), 6) == 0).value, target.value, m_interp_pc_next);
			return;
		}

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
		if (m_interp_magn)
		{
			value_t<u32> target;
			target.value = m_interp_pc;
			target = eval((target + (get_imm<u32>(op.i16, false) << 2)) & 0x3fffc);
			m_interp_pc = m_ir->CreateSelect(eval(extract(get_vr<u16[8]>(op.rt), 6) != 0).value, target.value, m_interp_pc_next);
			return;
		}

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
		if (m_interp_magn)
		{
			m_interp_pc = eval((get_imm<u32>(op.i16, false) << 2) & 0x3fffc).value;
			return;
		}

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
		if (m_interp_magn)
		{
			value_t<u32> target;
			target.value = m_interp_pc;
			target = eval((target + (get_imm<u32>(op.i16, false) << 2)) & 0x3fffc);
			m_interp_pc = target.value;
			return;
		}

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
		if (m_interp_magn)
		{
			value_t<u32> next;
			next.value = m_interp_pc_next;
			set_vr(op.rt, insert(splat<u32[4]>(0), 3, next));
			return;
		}

		set_vr(op.rt, build<u32[4]>(0, 0, 0, spu_branch_target(m_pos + 4)));

		if (g_cfg.core.spu_block_size != spu_block_size_type::safe && m_block_info[m_pos / 4 + 1] && m_entry_info[m_pos / 4 + 1])
		{
			// Store the return function chunk address at the stack mirror
			const auto func = add_function(m_pos + 4);
			const auto stack0 = eval(zext<u64>(extract(get_reg_fixed(1), 3) & 0x3fff0) + ::offset32(&spu_thread::stack_mirror));
			const auto stack1 = eval(stack0 + 8);
			m_ir->CreateStore(func, m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack0.value), func->getType()->getPointerTo()));
			m_ir->CreateStore(m_ir->getInt64(m_pos + 4), m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack1.value), get_type<u64*>()));
		}
	}

	static const spu_decoder<spu_llvm_recompiler> g_decoder;
};

std::unique_ptr<spu_recompiler_base> spu_recompiler_base::make_llvm_recompiler(u8 magn)
{
	return std::make_unique<spu_llvm_recompiler>(magn);
}

DECLARE(spu_llvm_recompiler::g_decoder);

#else

std::unique_ptr<spu_recompiler_base> spu_recompiler_base::make_llvm_recompiler(u8 magn)
{
	if (magn)
	{
		return nullptr;
	}

	fmt::throw_exception("LLVM is not available in this build.");
}

#endif
