#include "stdafx.h"
#include "SPURecompiler.h"

#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Crypto/sha1.h"
#include "Utilities/StrUtil.h"
#include "Utilities/JIT.h"
#include "Utilities/sysinfo.h"
#include "util/init_mutex.hpp"

#include "SPUThread.h"
#include "SPUAnalyser.h"
#include "SPUInterpreter.h"
#include "SPUDisAsm.h"
#include <algorithm>
#include <mutex>
#include <thread>

extern atomic_t<const char*> g_progr;
extern atomic_t<u32> g_progr_ptotal;
extern atomic_t<u32> g_progr_pdone;

const spu_decoder<spu_itype> s_spu_itype;
const spu_decoder<spu_iname> s_spu_iname;
const spu_decoder<spu_iflag> s_spu_iflag;

extern const spu_decoder<spu_interpreter_precise> g_spu_interpreter_precise;
extern const spu_decoder<spu_interpreter_fast> g_spu_interpreter_fast;

extern u64 get_timebased_time();

// Move 4 args for calling native function from a GHC calling convention function
static u8* move_args_ghc_to_native(u8* raw)
{
#ifdef _WIN32
	// mov  rcx, r13
	// mov  rdx, rbp
	// mov  r8,  r12
	// mov  r9,  rbx
	std::memcpy(raw, "\x4C\x89\xE9\x48\x89\xEA\x4D\x89\xE0\x49\x89\xD9", 12);
#else
	// mov  rdi, r13
	// mov  rsi, rbp
	// mov  rdx, r12
	// mov  rcx, rbx
	std::memcpy(raw, "\x4C\x89\xEF\x48\x89\xEE\x4C\x89\xE2\x48\x89\xD9", 12);
#endif

	return raw + 12;
}

DECLARE(spu_runtime::tr_dispatch) = []
{
	// Generate a special trampoline to spu_recompiler_base::dispatch with pause instruction
	u8* const trptr = jit_runtime::alloc(32, 16);
	u8* raw = move_args_ghc_to_native(trptr);
	*raw++ = 0xf3; // pause
	*raw++ = 0x90;
	*raw++ = 0xff; // jmp [rip]
	*raw++ = 0x25;
	std::memset(raw, 0, 4);
	const u64 target = reinterpret_cast<u64>(&spu_recompiler_base::dispatch);
	std::memcpy(raw + 4, &target, 8);
	return reinterpret_cast<spu_function_t>(trptr);
}();

DECLARE(spu_runtime::tr_branch) = []
{
	// Generate a trampoline to spu_recompiler_base::branch
	u8* const trptr = jit_runtime::alloc(32, 16);
	u8* raw = move_args_ghc_to_native(trptr);
	*raw++ = 0xff; // jmp [rip]
	*raw++ = 0x25;
	std::memset(raw, 0, 4);
	const u64 target = reinterpret_cast<u64>(&spu_recompiler_base::branch);
	std::memcpy(raw + 4, &target, 8);
	return reinterpret_cast<spu_function_t>(trptr);
}();

DECLARE(spu_runtime::tr_interpreter) = []
{
	u8* const trptr = jit_runtime::alloc(32, 16);
	u8* raw = move_args_ghc_to_native(trptr);
	*raw++ = 0xff; // jmp [rip]
	*raw++ = 0x25;
	std::memset(raw, 0, 4);
	const u64 target = reinterpret_cast<u64>(&spu_recompiler_base::old_interpreter);
	std::memcpy(raw + 4, &target, 8);
	return reinterpret_cast<spu_function_t>(trptr);
}();

DECLARE(spu_runtime::g_dispatcher) = []
{
	// Allocate 2^20 positions in data area
	const auto ptr = reinterpret_cast<decltype(g_dispatcher)>(jit_runtime::alloc(sizeof(*g_dispatcher), 64, false));

	for (auto& x : *ptr)
	{
		x.raw() = tr_dispatch;
	}

	return ptr;
}();

DECLARE(spu_runtime::tr_all) = []
{
	u8* const trptr = jit_runtime::alloc(32, 16);
	u8* raw = trptr;

	// Load PC: mov eax, [r13 + spu_thread::pc]
	*raw++ = 0x41;
	*raw++ = 0x8b;
	*raw++ = 0x45;
	*raw++ = ::narrow<s8>(::offset32(&spu_thread::pc));

	// Get LS address starting from PC: lea rcx, [rbp + rax]
	*raw++ = 0x48;
	*raw++ = 0x8d;
	*raw++ = 0x4c;
	*raw++ = 0x05;
	*raw++ = 0x00;

	// mov eax, [rcx]
	*raw++ = 0x8b;
	*raw++ = 0x01;

	// shr eax, (32 - 20)
	*raw++ = 0xc1;
	*raw++ = 0xe8;
	*raw++ = 0x0c;

	// Load g_dispatcher to rdx
	*raw++ = 0x48;
	*raw++ = 0x8d;
	*raw++ = 0x15;
	const s32 r32 = ::narrow<s32>(reinterpret_cast<u64>(g_dispatcher) - reinterpret_cast<u64>(raw) - 4, HERE);
	std::memcpy(raw, &r32, 4);
	raw += 4;

	// Update block_hash (set zero): mov [r13 + spu_thread::m_block_hash], 0
	*raw++ = 0x49;
	*raw++ = 0xc7;
	*raw++ = 0x45;
	*raw++ = ::narrow<s8>(::offset32(&spu_thread::block_hash));
	*raw++ = 0x00;
	*raw++ = 0x00;
	*raw++ = 0x00;
	*raw++ = 0x00;

	// jmp [rdx + rax * 8]
	*raw++ = 0xff;
	*raw++ = 0x24;
	*raw++ = 0xc2;

	return reinterpret_cast<spu_function_t>(trptr);
}();

DECLARE(spu_runtime::g_gateway) = build_function_asm<spu_function_t>([](asmjit::X86Assembler& c, auto& args)
{
	// Gateway for SPU dispatcher, converts from native to GHC calling convention, also saves RSP value for spu_escape
	using namespace asmjit;

#ifdef _WIN32
	c.push(x86::r15);
	c.push(x86::r14);
	c.push(x86::r13);
	c.push(x86::r12);
	c.push(x86::rsi);
	c.push(x86::rdi);
	c.push(x86::rbp);
	c.push(x86::rbx);
	c.sub(x86::rsp, 0xa8);
	c.movaps(x86::oword_ptr(x86::rsp, 0x90), x86::xmm15);
	c.movaps(x86::oword_ptr(x86::rsp, 0x80), x86::xmm14);
	c.movaps(x86::oword_ptr(x86::rsp, 0x70), x86::xmm13);
	c.movaps(x86::oword_ptr(x86::rsp, 0x60), x86::xmm12);
	c.movaps(x86::oword_ptr(x86::rsp, 0x50), x86::xmm11);
	c.movaps(x86::oword_ptr(x86::rsp, 0x40), x86::xmm10);
	c.movaps(x86::oword_ptr(x86::rsp, 0x30), x86::xmm9);
	c.movaps(x86::oword_ptr(x86::rsp, 0x20), x86::xmm8);
	c.movaps(x86::oword_ptr(x86::rsp, 0x10), x86::xmm7);
	c.movaps(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
#else
	c.push(x86::rbp);
	c.push(x86::r15);
	c.push(x86::r14);
	c.push(x86::r13);
	c.push(x86::r12);
	c.push(x86::rbx);
	c.push(x86::rax);
#endif

	// Load tr_all function pointer to call actual compiled function
	c.mov(x86::rax, asmjit::imm_ptr(spu_runtime::tr_all));

	// Save native stack pointer for longjmp emulation
	c.mov(x86::qword_ptr(args[0], ::offset32(&spu_thread::saved_native_sp)), x86::rsp);

	// Move 4 args (despite spu_function_t def)
	c.mov(x86::r13, args[0]);
	c.mov(x86::rbp, args[1]);
	c.mov(x86::r12, args[2]);
	c.mov(x86::rbx, args[3]);

	if (utils::has_avx())
	{
		c.vzeroupper();
	}

	c.call(x86::rax);

	if (utils::has_avx())
	{
		c.vzeroupper();
	}

#ifdef _WIN32
	c.movaps(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
	c.movaps(x86::xmm7, x86::oword_ptr(x86::rsp, 0x10));
	c.movaps(x86::xmm8, x86::oword_ptr(x86::rsp, 0x20));
	c.movaps(x86::xmm9, x86::oword_ptr(x86::rsp, 0x30));
	c.movaps(x86::xmm10, x86::oword_ptr(x86::rsp, 0x40));
	c.movaps(x86::xmm11, x86::oword_ptr(x86::rsp, 0x50));
	c.movaps(x86::xmm12, x86::oword_ptr(x86::rsp, 0x60));
	c.movaps(x86::xmm13, x86::oword_ptr(x86::rsp, 0x70));
	c.movaps(x86::xmm14, x86::oword_ptr(x86::rsp, 0x80));
	c.movaps(x86::xmm15, x86::oword_ptr(x86::rsp, 0x90));
	c.add(x86::rsp, 0xa8);
	c.pop(x86::rbx);
	c.pop(x86::rbp);
	c.pop(x86::rdi);
	c.pop(x86::rsi);
	c.pop(x86::r12);
	c.pop(x86::r13);
	c.pop(x86::r14);
	c.pop(x86::r15);
#else
	c.add(x86::rsp, +8);
	c.pop(x86::rbx);
	c.pop(x86::r12);
	c.pop(x86::r13);
	c.pop(x86::r14);
	c.pop(x86::r15);
	c.pop(x86::rbp);
#endif

	c.ret();
});

DECLARE(spu_runtime::g_escape) = build_function_asm<void(*)(spu_thread*)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	// Restore native stack pointer (longjmp emulation)
	c.mov(x86::rsp, x86::qword_ptr(args[0], ::offset32(&spu_thread::saved_native_sp)));

	// Return to the return location
	c.jmp(x86::qword_ptr(x86::rsp, -8));
});

DECLARE(spu_runtime::g_tail_escape) = build_function_asm<void(*)(spu_thread*, spu_function_t, u8*)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	// Restore native stack pointer (longjmp emulation)
	c.mov(x86::rsp, x86::qword_ptr(args[0], ::offset32(&spu_thread::saved_native_sp)));

	// Adjust stack for initial call instruction in the gateway
	c.sub(x86::rsp, 8);

	// Tail call, GHC CC (second arg)
	c.mov(x86::r13, args[0]);
	c.mov(x86::ebp, x86::dword_ptr(args[0], ::offset32(&spu_thread::offset)));
	c.add(x86::rbp, x86::qword_ptr(args[0], ::offset32(&spu_thread::memory_base_addr)));
	c.mov(x86::r12, args[2]);
	c.xor_(x86::ebx, x86::ebx);
	c.jmp(args[1]);
});

DECLARE(spu_runtime::g_interpreter_table) = {};

DECLARE(spu_runtime::g_interpreter) = nullptr;

spu_cache::spu_cache(const std::string& loc)
	: m_file(loc, fs::read + fs::write + fs::create + fs::append)
{
}

spu_cache::~spu_cache()
{
}

std::deque<spu_program> spu_cache::get()
{
	std::deque<spu_program> result;

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

		func.resize(size);

		if (m_file.read(func.data(), func.size() * 4) != func.size() * 4)
		{
			break;
		}

		if (!size || !func[0])
		{
			// Skip old format Giga entries
			continue;
		}

		spu_program res;
		res.entry_point = addr;
		res.lower_bound = addr;
		res.data = std::move(func);
		result.emplace_front(std::move(res));
	}

	return result;
}

void spu_cache::add(const spu_program& func)
{
	if (!m_file)
	{
		return;
	}

	be_t<u32> size = ::size32(func.data);
	be_t<u32> addr = func.entry_point;

	const fs::iovec_clone gather[3]
	{
		{&size, sizeof(size)},
		{&addr, sizeof(addr)},
		{func.data.data(), func.data.size() * 4}
	};

	// Append data
	m_file.write_gather(gather, 3);
}

void spu_cache::initialize()
{
	spu_runtime::g_interpreter = spu_runtime::g_gateway;

	if (g_cfg.core.spu_decoder == spu_decoder_type::precise || g_cfg.core.spu_decoder == spu_decoder_type::fast)
	{
		for (auto& x : *spu_runtime::g_dispatcher)
		{
			x.raw() = spu_runtime::tr_interpreter;
		}
	}

	const std::string ppu_cache = Emu.PPUCache();

	if (ppu_cache.empty())
	{
		return;
	}

	// SPU cache file (version + block size type)
	const std::string loc = ppu_cache + "spu-" + fmt::to_lower(g_cfg.core.spu_block_size.to_string()) + "-v1-tane.dat";

	spu_cache cache(loc);

	if (!cache)
	{
		spu_log.error("Failed to initialize SPU cache at: %s", loc);
		return;
	}

	// Read cache
	auto func_list = cache.get();
	atomic_t<std::size_t> fnext{};
	atomic_t<u8> fail_flag{0};

	// Initialize compiler instances for parallel compilation
	u32 max_threads = static_cast<u32>(g_cfg.core.llvm_threads);
	u32 thread_count = max_threads > 0 ? std::min(max_threads, std::thread::hardware_concurrency()) : std::thread::hardware_concurrency();
	std::vector<std::unique_ptr<spu_recompiler_base>> compilers{thread_count};

	if (g_cfg.core.spu_decoder == spu_decoder_type::fast || g_cfg.core.spu_decoder == spu_decoder_type::llvm)
	{
		if (auto compiler = spu_recompiler_base::make_llvm_recompiler(11))
		{
			compiler->init();

			if (compiler->compile({}) && spu_runtime::g_interpreter)
			{
				spu_log.success("SPU Runtime: Built the interpreter.");

				if (g_cfg.core.spu_decoder != spu_decoder_type::llvm)
				{
					return;
				}
			}
			else
			{
				spu_log.fatal("SPU Runtime: Failed to build the interpreter.");
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
		// Fake LS
		std::vector<be_t<u32>> ls(0x10000);

		// Build functions
		for (std::size_t func_i = fnext++; func_i < func_list.size(); func_i = fnext++)
		{
			const spu_program& func = std::as_const(func_list)[func_i];

			if (Emu.IsStopped() || fail_flag)
			{
				g_progr_pdone++;
				continue;
			}

			// Get data start
			const u32 start = func.lower_bound;
			const u32 size0 = ::size32(func.data);

			// Initialize LS with function data only
			for (u32 i = 0, pos = start; i < size0; i++, pos += 4)
			{
				ls[pos / 4] = std::bit_cast<be_t<u32>>(func.data[i]);
			}

			// Call analyser
			spu_program func2 = compiler->analyse(ls.data(), func.entry_point);

			if (func2 != func)
			{
				spu_log.error("[0x%05x] SPU Analyser failed, %u vs %u", func2.entry_point, func2.data.size(), size0);
			}
			else if (!compiler->compile(std::move(func2)))
			{
				// Likely, out of JIT memory. Signal to prevent further building.
				fail_flag |= 1;
			}

			// Clear fake LS
			std::memset(ls.data() + start / 4, 0, 4 * (size0 - 1));

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
		spu_log.error("SPU Runtime: Cache building aborted.");
		return;
	}

	if (fail_flag)
	{
		spu_log.fatal("SPU Runtime: Cache building failed (out of memory).");
		return;
	}

	if (compilers.size() && !func_list.empty())
	{
		spu_log.success("SPU Runtime: Built %u functions.", func_list.size());
	}

	// Initialize global cache instance
	g_fxo->init<spu_cache>(std::move(cache));
}

bool spu_program::operator==(const spu_program& rhs) const noexcept
{
	// TODO
	return entry_point - lower_bound == rhs.entry_point - rhs.lower_bound && data == rhs.data;
}

bool spu_program::operator<(const spu_program& rhs) const noexcept
{
	const u32 lhs_offs = (entry_point - lower_bound) / 4;
	const u32 rhs_offs = (rhs.entry_point - rhs.lower_bound) / 4;

	// Select range for comparison
	std::basic_string_view<u32> lhs_data(data.data() + lhs_offs, data.size() - lhs_offs);
	std::basic_string_view<u32> rhs_data(rhs.data.data() + rhs_offs, rhs.data.size() - rhs_offs);
	const auto cmp0 = lhs_data.compare(rhs_data);

	if (cmp0 < 0)
		return true;
	else if (cmp0 > 0)
		return false;

	// Compare from address 0 to the point before the entry point (TODO: undesirable)
	lhs_data = {data.data(), lhs_offs};
	rhs_data = {rhs.data.data(), rhs_offs};
	const auto cmp1 = lhs_data.compare(rhs_data);

	if (cmp1 < 0)
		return true;
	else if (cmp1 > 0)
		return false;

	// TODO
	return lhs_offs < rhs_offs;
}

spu_runtime::spu_runtime()
{
	// Clear LLVM output
	m_cache_path = Emu.PPUCache();

	if (m_cache_path.empty())
	{
		return;
	}

	fs::create_dir(m_cache_path + "llvm/");
	fs::remove_all(m_cache_path + "llvm/", false);

	if (g_cfg.core.spu_debug)
	{
		fs::file(m_cache_path + "spu.log", fs::rewrite);
		fs::file(m_cache_path + "spu-ir.log", fs::rewrite);
	}
}

spu_item* spu_runtime::add_empty(spu_program&& data)
{
	if (data.data.empty())
	{
		return nullptr;
	}

	// Store previous item if already added
	spu_item* prev = nullptr;

	//Try to add item that doesn't exist yet
	const auto ret = m_stuff[data.data[0] >> 12].push_if([&](spu_item& _new, spu_item& _old)
	{
		if (_new.data == _old.data)
		{
			prev = &_old;
			return false;
		}

		return true;
	}, std::move(data));

	if (ret)
	{
		return ret;
	}

	return prev;
}

spu_function_t spu_runtime::rebuild_ubertrampoline(u32 id_inst)
{
	// Prepare sorted list
	static thread_local std::vector<std::pair<std::basic_string_view<u32>, spu_function_t>> m_flat_list;

	// Remember top position
	auto stuff_it = m_stuff.at(id_inst >> 12).begin();
	auto stuff_end = m_stuff.at(id_inst >> 12).end();
	{
		if (stuff_it->trampoline)
		{
			return stuff_it->trampoline;
		}

		m_flat_list.clear();

		for (auto it = stuff_it; it != stuff_end; ++it)
		{
			if (const auto ptr = it->compiled.load())
			{
				std::basic_string_view<u32> range{it->data.data.data(), it->data.data.size()};
				range.remove_prefix((it->data.entry_point - it->data.lower_bound) / 4);
				m_flat_list.emplace_back(range, ptr);
			}
			else
			{
				// Pull oneself deeper (TODO)
				++stuff_it;
			}
		}
	}

	std::sort(m_flat_list.begin(), m_flat_list.end(), [&](const auto& a, const auto& b)
	{
		std::basic_string_view<u32> lhs = a.first;
		std::basic_string_view<u32> rhs = b.first;
		return lhs < rhs;
	});

	struct work
	{
		u32 size;
		u16 from;
		u16 level;
		u8* rel32;
		decltype(m_flat_list)::iterator beg;
		decltype(m_flat_list)::iterator end;
	};

	// Scratch vector
	static thread_local std::vector<work> workload;

	// Generate a dispatcher (übertrampoline)
	const auto beg = m_flat_list.begin();
	const auto _end = m_flat_list.end();
	const u32 size0 = ::size32(m_flat_list);

	auto result = beg->second;

	if (size0 != 1)
	{
		// Allocate some writable executable memory
		u8* const wxptr = jit_runtime::alloc(size0 * 22 + 14, 16);

		if (!wxptr)
		{
			return nullptr;
		}

		// Raw assembly pointer
		u8* raw = wxptr;

		// Write jump instruction with rel32 immediate
		auto make_jump = [&](u8 op, auto target)
		{
			verify("Asm overflow" HERE), raw + 8 <= wxptr + size0 * 22 + 16;

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
		workload.back().level = 0;
		workload.back().from  = -1;
		workload.back().rel32 = 0;
		workload.back().beg   = beg;
		workload.back().end   = _end;

		// LS address starting from PC is already loaded into rcx (see spu_runtime::tr_all)

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

			while (verify("spu_runtime::work::level overflow" HERE, w.level != 0xffff))
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

					// Resort subrange starting from the new level
					std::stable_sort(w.beg, w.end, [&](const auto& a, const auto& b)
					{
						std::basic_string_view<u32> lhs = a.first;
						std::basic_string_view<u32> rhs = b.first;

						lhs.remove_prefix(w.level);
						rhs.remove_prefix(w.level);

						return lhs < rhs;
					});

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

			if (w.level >= w.beg->first.size() || w.level >= it->first.size())
			{
				// If functions cannot be compared, assume smallest function
				spu_log.error("Trampoline simplified at ??? (level=%u)", w.level);
				make_jump(0xe9, w.beg->second); // jmp rel32
				continue;
			}

			// Value for comparison
			const u32 x = it->first.at(w.level);

			// Adjust ranges (backward)
			while (it != m_flat_list.begin())
			{
				it--;

				if (w.level >= it->first.size())
				{
					it = m_flat_list.end();
					break;
				}

				if (it->first.at(w.level) != x)
				{
					it++;
					break;
				}

				verify(HERE), it != w.beg;
				size1--;
				size2++;
			}

			if (it == m_flat_list.end())
			{
				spu_log.error("Trampoline simplified (II) at ??? (level=%u)", w.level);
				make_jump(0xe9, w.beg->second); // jmp rel32
				continue;
			}

			// Emit 32-bit comparison
			verify("Asm overflow" HERE), raw + 12 <= wxptr + size0 * 22 + 16;

			if (w.from != w.level)
			{
				// If necessary (level has advanced), emit load: mov eax, [rcx + addr]
				const u32 cmp_lsa = w.level * 4u;

				if (cmp_lsa < 0x80)
				{
					*raw++ = 0x8b;
					*raw++ = 0x41;
					*raw++ = ::narrow<s8>(cmp_lsa);
				}
				else
				{
					*raw++ = 0x8b;
					*raw++ = 0x81;
					std::memcpy(raw, &cmp_lsa, 4);
					raw += 4;
				}
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
		result = reinterpret_cast<spu_function_t>(reinterpret_cast<u64>(wxptr));
	}

	if (auto _old = stuff_it->trampoline.compare_and_swap(nullptr, result))
	{
		return _old;
	}

	// Install ubertrampoline
	auto& insert_to = spu_runtime::g_dispatcher->at(id_inst >> 12);

	auto _old = insert_to.load();

	do
	{
		// Make sure we are replacing an older ubertrampoline but not newer one
		if (_old != tr_dispatch)
		{
			bool ok = false;

			for (auto it = stuff_it; it != stuff_end; ++it)
			{
				if (it->trampoline == _old)
				{
					ok = true;
					break;
				}
			}

			if (!ok)
			{
				return result;
			}
		}
	}
	while (!insert_to.compare_exchange(_old, result));

	return result;
}

spu_function_t spu_runtime::find(const u32* ls, u32 addr) const
{
	for (auto& item : m_stuff.at(ls[addr / 4] >> 12))
	{
		if (const auto ptr = item.compiled.load())
		{
			std::basic_string_view<u32> range{item.data.data.data(), item.data.data.size()};
			range.remove_prefix((item.data.entry_point - item.data.lower_bound) / 4);

			if (addr / 4 + range.size() > 0x10000)
			{
				continue;
			}

			if (range.compare(0, range.size(), ls + addr / 4, range.size()) == 0)
			{
				return ptr;
			}
		}
	}

	return nullptr;
}

spu_function_t spu_runtime::make_branch_patchpoint(u16 data) const
{
	u8* const raw = jit_runtime::alloc(16, 16);

	if (!raw)
	{
		return nullptr;
	}

	// Save address of the following jmp (GHC CC 3rd argument)
	raw[0] = 0x4c; // lea r12, [rip+1]
	raw[1] = 0x8d;
	raw[2] = 0x25;
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
	raw[14] = data >> 8;
	raw[15] = data & 0xff;

	return reinterpret_cast<spu_function_t>(raw);
}

spu_recompiler_base::spu_recompiler_base()
{
}

spu_recompiler_base::~spu_recompiler_base()
{
}

void spu_recompiler_base::dispatch(spu_thread& spu, void*, u8* rip)
{
	// If code verification failed from a patched patchpoint, clear it with a dispatcher jump
	if (rip)
	{
		const s64 rel = reinterpret_cast<u64>(spu_runtime::tr_all) - reinterpret_cast<u64>(rip - 8) - 5;

		union
		{
			u8 bytes[8];
			u64 result;
		};

		bytes[0] = 0xe9; // jmp rel32
		std::memcpy(bytes + 1, &rel, 4);
		bytes[5] = 0x66; // lnop (2 bytes)
		bytes[6] = 0x90;
		bytes[7] = 0x90;

		atomic_storage<u64>::release(*reinterpret_cast<u64*>(rip - 8), result);
	}

	// Second attempt (recover from the recursion after repeated unsuccessful trampoline call)
	if (spu.block_counter != spu.block_recover && &dispatch != spu_runtime::g_dispatcher->at(spu._ref<nse_t<u32>>(spu.pc) >> 12))
	{
		spu.block_recover = spu.block_counter;
		return;
	}

	spu.jit->init();

	// Compile
	if (spu._ref<u32>(spu.pc) == 0)
	{
		spu_runtime::g_escape(&spu);
		return;
	}

	const auto func = spu.jit->compile(spu.jit->analyse(spu._ptr<u32>(0), spu.pc));

	if (!func)
	{
		spu_log.fatal("[0x%05x] Compilation failed.", spu.pc);
		Emu.Pause();
		return;
	}

	// Diagnostic
	if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		const v128 _info = spu.stack_mirror[(spu.gpr[1]._u32[3] & 0x3fff0) >> 4];

		if (_info._u64[0] + 1)
		{
			spu_log.trace("Called from 0x%x", _info._u32[2] - 4);
		}
	}

	spu_runtime::g_tail_escape(&spu, func, nullptr);
}

void spu_recompiler_base::branch(spu_thread& spu, void*, u8* rip)
{
	if (const u32 ls_off = ((rip[6] << 8) | rip[7]) * 4)
	{
		spu_log.todo("Special branch patchpoint hit.\nPlease report to the developer (0x%05x).", ls_off);
	}

	// Find function
	const auto func = spu.jit->get_runtime().find(static_cast<u32*>(vm::base(spu.offset)), spu.pc);

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

		bytes[6] = rip[6];
		bytes[7] = rip[7];
	}
	else
	{
		fmt::throw_exception("Impossible far jump: %p -> %p", rip, func);
	}

	atomic_storage<u64>::release(*reinterpret_cast<u64*>(rip), result);

	spu_runtime::g_tail_escape(&spu, func, rip);
}

void spu_recompiler_base::old_interpreter(spu_thread& spu, void* ls, u8* rip) try
{
	// Select opcode table
	const auto& table = *(
		g_cfg.core.spu_decoder == spu_decoder_type::precise ? &g_spu_interpreter_precise.get_table() :
		g_cfg.core.spu_decoder == spu_decoder_type::fast ? &g_spu_interpreter_fast.get_table() :
		(fmt::throw_exception("Invalid SPU decoder"), nullptr));

	// LS pointer
	const auto base = static_cast<const u8*>(ls);

	while (true)
	{
		if (spu.state) [[unlikely]]
		{
			if (spu.check_state())
				break;
		}

		const u32 op = *reinterpret_cast<const be_t<u32>*>(base + spu.pc);
		if (table[spu_decode(op)](spu, {op}))
			spu.pc += 4;
	}
}
catch (const std::exception& e)
{
	Emu.Pause();
	spu_log.fatal("%s thrown: %s", typeid(e).name(), e.what());
	spu_log.notice("\n%s", spu.dump());
}

spu_program spu_recompiler_base::analyse(const be_t<u32>* ls, u32 entry_point)
{
	// Result: addr + raw instruction data
	spu_program result;
	result.data.reserve(10000);
	result.entry_point = entry_point;
	result.lower_bound = entry_point;

	// Initialize block entries
	m_block_info.reset();
	m_block_info.set(entry_point / 4);
	m_entry_info.reset();
	m_entry_info.set(entry_point / 4);
	m_ret_info.reset();

	// Simple block entry workload list
	workload.clear();
	workload.push_back(entry_point);

	std::memset(m_regmod.data(), 0xff, sizeof(m_regmod));
	std::memset(m_use_ra.data(), 0xff, sizeof(m_use_ra));
	std::memset(m_use_rb.data(), 0xff, sizeof(m_use_rb));
	std::memset(m_use_rc.data(), 0xff, sizeof(m_use_rc));
	m_targets.clear();
	m_preds.clear();
	m_preds[entry_point];
	m_bbs.clear();
	m_chunks.clear();
	m_funcs.clear();

	// Value flags (TODO: only is_const is implemented)
	enum class vf : u32
	{
		is_const,
		is_mask,
		is_rel,

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
				if (m_preds[target].find_first_of(pos) + 1 == 0)
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

		// Fill register access info
		if (auto iflags = s_spu_iflag.decode(data))
		{
			if (iflags & spu_iflag::use_ra)
				m_use_ra[pos / 4] = op.ra;
			if (iflags & spu_iflag::use_rb)
				m_use_rb[pos / 4] = op.rb;
			if (iflags & spu_iflag::use_rc)
				m_use_rc[pos / 4] = op.rc;
		}

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
				spu_log.error("[0x%x] Invalid interrupt flags (DE)", pos);
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
				spu_log.error("[0x%x] Invalid interrupt flags (DE)", pos);
			}

			const auto af = vflags[op.ra];
			const auto av = values[op.ra];
			const bool sl = type == spu_itype::BISL || type == spu_itype::BISLED;

			if (sl)
			{
				m_regmod[pos / 4] = op.rt;
				vflags[op.rt] = +vf::is_const;
				values[op.rt] = pos + 4;
			}

			if (af & vf::is_const)
			{
				const u32 target = spu_branch_target(av);

				spu_log.warning("[0x%x] At 0x%x: indirect branch to 0x%x%s", entry_point, pos, target, op.d ? " (D)" : op.e ? " (E)" : "");

				m_targets[pos].push_back(target);

				if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
				{
					if (sync)
					{
						spu_log.notice("[0x%x] At 0x%x: ignoring %scall to 0x%x (SYNC)", entry_point, pos, sl ? "" : "tail ", target);

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
				else if (target > entry_point)
				{
					limit = std::min<u32>(limit, target);
				}

				if (sl && g_cfg.core.spu_block_size != spu_block_size_type::safe)
				{
					m_ret_info[pos / 4 + 1] = true;
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
						const u32 new_size = (start - lsa) / 4 + jt_abs.size();

						if (result.data.size() < new_size)
						{
							result.data.resize(new_size);
						}

						for (u32 i = 0; i < jt_abs.size(); i++)
						{
							add_block(jt_abs[i]);
							result.data[(start - lsa) / 4 + i] = std::bit_cast<u32, be_t<u32>>(jt_abs[i]);
							m_targets[start + i * 4];
						}

						m_targets.emplace(pos, std::move(jt_abs));
					}

					if (jt_rel.size() >= jt_abs.size())
					{
						const u32 new_size = (start - lsa) / 4 + jt_rel.size();

						if (result.data.size() < new_size)
						{
							result.data.resize(new_size);
						}

						for (u32 i = 0; i < jt_rel.size(); i++)
						{
							add_block(jt_rel[i]);
							result.data[(start - lsa) / 4 + i] = std::bit_cast<u32, be_t<u32>>(jt_rel[i] - start);
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
					spu_log.warning("[0x%x] Pattern 1 detected (hbr=0x%x:0x%x)", pos, hbr_loc, hbr_tg);

					// Add 8 targets (TODO)
					for (u32 addr = start + 4; addr < start + 36; addr += 4)
					{
						m_targets[pos].push_back(addr);
						add_block(addr);
					}
				}
				else if (hbr_loc > start && hbr_loc < limit && hbr_tg == start)
				{
					spu_log.warning("[0x%x] No patterns detected (hbr=0x%x:0x%x)", pos, hbr_loc, hbr_tg);
				}
			}
			else if (type == spu_itype::BI && sync)
			{
				spu_log.notice("[0x%x] At 0x%x: ignoring indirect branch (SYNC)", entry_point, pos);
			}

			if (type == spu_itype::BI || sl)
			{
				if (type == spu_itype::BI || g_cfg.core.spu_block_size == spu_block_size_type::safe)
				{
					m_targets[pos];
				}
				else
				{
					m_ret_info[pos / 4 + 1] = true;
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

			if (type == spu_itype::BRSL && target == pos + 4)
			{
				// Get next instruction address idiom
				break;
			}

			m_targets[pos].push_back(target);

			if (g_cfg.core.spu_block_size != spu_block_size_type::safe)
			{
				m_ret_info[pos / 4 + 1] = true;
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
					spu_log.notice("[0x%x] At 0x%x: ignoring fixed call to 0x%x (SYNC)", entry_point, pos, target);
				}

				if (target > entry_point)
				{
					limit = std::min<u32>(limit, target);
				}
			}

			next_block();
			break;
		}

		case spu_itype::BRA:
		{
			const u32 target = spu_branch_target(0, op.i16);

			if (g_cfg.core.spu_block_size == spu_block_size_type::giga && !sync)
			{
				m_entry_info[target / 4] = true;
				add_block(target);
			}
			else
			{
				if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
				{
					spu_log.notice("[0x%x] At 0x%x: ignoring fixed tail call to 0x%x (SYNC)", entry_point, pos, target);
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
		case spu_itype::BRZ:
		case spu_itype::BRNZ:
		case spu_itype::BRHZ:
		case spu_itype::BRHNZ:
		{
			const u32 target = spu_branch_target(pos, op.i16);

			if (target == pos + 4)
			{
				// Nop
				break;
			}

			m_targets[pos].push_back(target);
			add_block(target);

			if (type != spu_itype::BR)
			{
				m_targets[pos].push_back(pos + 4);
				add_block(pos + 4);
			}

			next_block();
			break;
		}

		case spu_itype::DSYNC:
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
			case MFC_Cmd:
			{
				m_use_rb[pos / 4] = s_reg_mfc_eal;
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
			break;
		}
		case spu_itype::ILA:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i18;
			break;
		}
		case spu_itype::ILH:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i16 << 16 | op.i16;
			break;
		}
		case spu_itype::ILHU:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i16 << 16;
			break;
		}
		case spu_itype::IOHL:
		{
			m_regmod[pos / 4] = op.rt;
			values[op.rt] = values[op.rt] | op.i16;
			break;
		}
		case spu_itype::ORI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] | op.si10;
			break;
		}
		case spu_itype::OR:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] | values[op.rb];
			break;
		}
		case spu_itype::ANDI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] & op.si10;
			break;
		}
		case spu_itype::AND:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] & values[op.rb];
			break;
		}
		case spu_itype::AI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] + op.si10;
			break;
		}
		case spu_itype::A:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] + values[op.rb];
			break;
		}
		case spu_itype::SFI:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = op.si10 - values[op.ra];
			break;
		}
		case spu_itype::SF:
		{
			m_regmod[pos / 4] = op.rt;
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.rb] - values[op.ra];
			break;
		}
		case spu_itype::ROTMI:
		{
			m_regmod[pos / 4] = op.rt;

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
			break;
		}
		}

		// Insert raw instruction value
		const u32 new_size = (pos - lsa) / 4;

		if (result.data.size() <= new_size)
		{
			if (result.data.size() < new_size)
			{
				result.data.resize(new_size);
			}

			result.data.emplace_back(std::bit_cast<u32, be_t<u32>>(data));
		}
		else if (u32& raw_val = result.data[new_size])
		{
			verify(HERE), raw_val == std::bit_cast<u32, be_t<u32>>(data);
		}
		else
		{
			raw_val = std::bit_cast<u32, be_t<u32>>(data);
		}
	}

	while (lsa > 0 || limit < 0x40000)
	{
		const u32 initial_size = result.data.size();

		// Check unreachable blocks
		limit = std::min<u32>(limit, lsa + initial_size * 4);

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
					if (j == entry_point)
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
						if (result.data.at((j - lsa) / 4 - 1) == 0 || m_targets.count(j - 4))
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

		result.data.resize((limit - lsa) / 4);

		// Check holes in safe mode (TODO)
		u32 valid_size = 0;

		for (u32 i = 0; i < result.data.size(); i++)
		{
			if (result.data[i] == 0)
			{
				const u32 pos  = lsa + i * 4;
				const u32 data = ls[pos / 4];

				// Allow only NOP or LNOP instructions in holes
				if (data == 0x200000 || (data & 0xffffff80) == 0x40200000)
				{
					continue;
				}

				if (g_cfg.core.spu_block_size != spu_block_size_type::giga)
				{
					result.data.resize(valid_size);
					break;
				}
			}
			else
			{
				valid_size = i + 1;
			}
		}

		// Even if NOP or LNOP, should be removed at the end
		result.data.resize(valid_size);

		// Repeat if blocks were removed
		if (result.data.size() == initial_size)
		{
			break;
		}
	}

	limit = std::min<u32>(limit, lsa + ::size32(result.data) * 4);

	// Cleanup block info
	for (u32 i = 0; i < workload.size(); i++)
	{
		const u32 addr = workload[i];

		if (addr < lsa || addr >= limit || !result.data[(addr - lsa) / 4])
		{
			m_block_info[addr / 4] = false;
			m_entry_info[addr / 4] = false;
			m_ret_info[addr / 4] = false;
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
		if (m_targets.count(prev) == 0 && prev >= lsa && prev < limit && result.data[(prev - lsa) / 4])
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

		it++;
	}

	// Fill holes which contain only NOP and LNOP instructions (TODO: compile)
	for (u32 i = 0, nnop = 0, vsize = 0; i <= result.data.size(); i++)
	{
		if (i >= result.data.size() || result.data[i])
		{
			if (nnop && nnop == i - vsize)
			{
				// Write only complete NOP sequence
				for (u32 j = vsize; j < i; j++)
				{
					result.data[j] = std::bit_cast<u32, be_t<u32>>(ls[lsa / 4 + j]);
				}
			}

			nnop  = 0;
			vsize = i + 1;
		}
		else
		{
			const u32 pos  = lsa + i * 4;
			const u32 data = ls[pos / 4];

			if (data == 0x200000 || (data & 0xffffff80) == 0x40200000)
			{
				nnop++;
			}
		}
	}

	// Fill block info
	for (auto& pred : m_preds)
	{
		auto& block = m_bbs[pred.first];

		// Copy predeccessors (wrong at this point, needs a fixup later)
		block.preds = pred.second;

		// Fill register usage info
		for (u32 ia = pred.first; ia < limit; ia += 4)
		{
			block.size++;

			// Decode instruction
			const spu_opcode_t op{std::bit_cast<be_t<u32>>(result.data[(ia - lsa) / 4])};

			const auto type = s_spu_itype.decode(op.opcode);

			u8 reg_save = 255;

			if (type == spu_itype::STQD && op.ra == s_reg_sp && !block.reg_mod[op.rt] && !block.reg_use[op.rt])
			{
				// Register saved onto the stack before use
				block.reg_save_dom[op.rt] = true;

				reg_save = op.rt;
			}

			for (auto* _use : {&m_use_ra, &m_use_rb, &m_use_rc})
			{
				if (u8 reg = (*_use)[ia / 4]; reg < s_reg_max)
				{
					// Register reg use only if it happens before reg mod
					if (!block.reg_mod[reg])
					{
						block.reg_use.set(reg);

						if (reg_save != reg && block.reg_save_dom[reg])
						{
							// Register is still used after saving; probably not eligible for optimization
							block.reg_save_dom[reg] = false;
						}
					}
				}
			}

			if (m_use_rb[ia / 4] == s_reg_mfc_eal)
			{
				// Expand MFC_Cmd reg use
				for (u8 reg : {s_reg_mfc_lsa, s_reg_mfc_tag, s_reg_mfc_size})
				{
					if (!block.reg_mod[reg])
						block.reg_use.set(reg);
				}
			}

			// Register reg modification
			if (u8 reg = m_regmod[ia / 4]; reg < s_reg_max)
			{
				block.reg_mod.set(reg);
				block.reg_mod_xf.set(reg, type & spu_itype::xfloat);

				if (type == spu_itype::SELB && (block.reg_mod_xf[op.ra] || block.reg_mod_xf[op.rb]))
					block.reg_mod_xf.set(reg);

				// Possible post-dominating register load
				if (type == spu_itype::LQD && op.ra == s_reg_sp)
					block.reg_load_mod[reg] = ia + 1;
				else
					block.reg_load_mod[reg] = 0;
			}

			// Find targets (also means end of the block)
			const auto tfound = m_targets.find(ia);

			if (tfound != m_targets.end())
			{
				// Copy targets
				block.targets = tfound->second;

				// Assume that the call reads and modifies all volatile registers (TODO)
				bool is_call = false;
				bool is_tail = false;
				switch (type)
				{
				case spu_itype::BRSL:
					is_call = spu_branch_target(ia, op.i16) != ia + 4;
					break;
				case spu_itype::BRASL:
					is_call = spu_branch_target(0, op.i16) != ia + 4;
					break;
				case spu_itype::BRA:
					is_call = true;
					is_tail = true;
					break;
				case spu_itype::BISL:
				case spu_itype::BISLED:
					is_call = true;
					break;
				default:
					break;
				}

				if (is_call)
				{
					for (u32 i = 0; i < s_reg_max; ++i)
					{
						if (i == s_reg_lr || (i >= 2 && i < s_reg_80) || i > s_reg_127)
						{
							if (!block.reg_mod[i])
								block.reg_use.set(i);

							if (!is_tail)
							{
								block.reg_mod.set(i);
								block.reg_mod_xf[i] = false;
							}
						}
					}
				}

				break;
			}
		}
	}

	// Fixup block predeccessors to point to basic blocks, not last instructions
	for (auto& bb : m_bbs)
	{
		const u32 addr = bb.first;

		for (u32& pred : bb.second.preds)
		{
			pred = std::prev(m_bbs.upper_bound(pred))->first;
		}

		if (m_entry_info[addr / 4] && g_cfg.core.spu_block_size == spu_block_size_type::giga)
		{
			// Register empty chunk
			m_chunks.push_back(addr);

			// Register function if necessary
			if (!m_ret_info[addr / 4])
			{
				m_funcs[addr];
			}
		}
	}

	// Ensure there is a function at the lowest address
	if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		if (auto emp = m_funcs.try_emplace(m_bbs.begin()->first); emp.second)
		{
			const u32 addr = emp.first->first;
			spu_log.error("[0x%05x] Fixed first function at 0x%05x", entry_point, addr);
			m_entry_info[addr / 4] = true;
			m_ret_info[addr / 4] = false;
		}
	}

	// Split functions
	while (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		bool need_repeat = false;

		u32 start = 0;
		u32 limit = 0x40000;

		// Walk block list in ascending order
		for (auto& block : m_bbs)
		{
			const u32 addr = block.first;

			if (m_entry_info[addr / 4] && !m_ret_info[addr / 4])
			{
				const auto upper = m_funcs.upper_bound(addr);
				start = addr;
				limit = upper == m_funcs.end() ? 0x40000 : upper->first;
			}

			// Find targets that exceed [start; limit) range and make new functions from them
			for (u32 target : block.second.targets)
			{
				const auto tfound = m_bbs.find(target);

				if (tfound == m_bbs.end())
				{
					continue;
				}

				if (target < start || target >= limit)
				{
					if (!m_entry_info[target / 4] || m_ret_info[target / 4])
					{
						// Create new function entry (likely a tail call)
						m_entry_info[target / 4] = true;

						m_ret_info[target / 4] = false;

						m_funcs.try_emplace(target);

						if (target < limit)
						{
							need_repeat = true;
						}
					}
				}
			}

			block.second.func = start;
		}

		if (!need_repeat)
		{
			break;
		}
	}

	// Fill entry map
	while (true)
	{
		workload.clear();
		workload.push_back(entry_point);

		std::basic_string<u32> new_entries;

		for (u32 wi = 0; wi < workload.size(); wi++)
		{
			const u32 addr = workload[wi];
			auto& block    = m_bbs.at(addr);
			const u32 _new = block.chunk;

			if (!m_entry_info[addr / 4])
			{
				// Check block predecessors
				for (u32 pred : block.preds)
				{
					const u32 _old = m_bbs.at(pred).chunk;

					if (_old < 0x40000 && _old != _new)
					{
						// If block has multiple 'entry' points, it becomes an entry point itself
						new_entries.push_back(addr);
					}
				}
			}

			// Update chunk address
			block.chunk = m_entry_info[addr / 4] ? addr : _new;

			// Process block targets
			for (u32 target : block.targets)
			{
				const auto tfound = m_bbs.find(target);

				if (tfound == m_bbs.end())
				{
					continue;
				}

				auto& tb = tfound->second;

				const u32 value = m_entry_info[target / 4] ? target : block.chunk;

				if (u32& tval = tb.chunk; tval < 0x40000)
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
		}

		if (new_entries.empty())
		{
			break;
		}

		for (u32 entry : new_entries)
		{
			m_entry_info[entry / 4] = true;

			// Acknowledge artificial (reversible) chunk entry point
			m_ret_info[entry / 4] = true;
		}

		for (auto& bb : m_bbs)
		{
			// Reset chunk info
			bb.second.chunk = 0x40000;
		}
	}

	workload.clear();
	workload.push_back(entry_point);

	// Fill workload adding targets
	for (u32 wi = 0; wi < workload.size(); wi++)
	{
		const u32 addr  = workload[wi];
		auto& block     = m_bbs.at(addr);
		block.analysed  = true;

		for (u32 target : block.targets)
		{
			const auto tfound = m_bbs.find(target);

			if (tfound == m_bbs.end())
			{
				continue;
			}

			auto& tb = tfound->second;

			if (!tb.analysed)
			{
				workload.push_back(target);
				tb.analysed = true;
			}

			// Limited xfloat hint propagation (possibly TODO)
			if (tb.chunk == block.chunk)
			{
				tb.reg_maybe_xf &= block.reg_mod_xf;
			}
			else
			{
				tb.reg_maybe_xf.reset();
			}
		}

		block.reg_origin.fill(0x80000000);
		block.reg_origin_abs.fill(0x80000000);
	}

	// Fill register origin info
	while (true)
	{
		bool must_repeat = false;

		for (u32 wi = 0; wi < workload.size(); wi++)
		{
			const u32 addr = workload[wi];
			auto& block    = m_bbs.at(addr);

			// Initialize entry point with default value: unknown origin (requires load)
			if (m_entry_info[addr / 4])
			{
				for (u32 i = 0; i < s_reg_max; i++)
				{
					if (block.reg_origin[i] == 0x80000000)
						block.reg_origin[i] = 0x40000;
				}
			}

			if (g_cfg.core.spu_block_size == spu_block_size_type::giga && m_entry_info[addr / 4] && !m_ret_info[addr / 4])
			{
				for (u32 i = 0; i < s_reg_max; i++)
				{
					if (block.reg_origin_abs[i] == 0x80000000)
						block.reg_origin_abs[i] = 0x40000;
					else if (block.reg_origin_abs[i] + 1 == 0)
						block.reg_origin_abs[i] = -2;
				}
			}

			for (u32 target : block.targets)
			{
				const auto tfound = m_bbs.find(target);

				if (tfound == m_bbs.end())
				{
					continue;
				}

				auto& tb = tfound->second;

				for (u32 i = 0; i < s_reg_max; i++)
				{
					if (tb.chunk == block.chunk && tb.reg_origin[i] + 1)
					{
						const u32 expected = block.reg_mod[i] ? addr : block.reg_origin[i];

						if (tb.reg_origin[i] == 0x80000000)
						{
							tb.reg_origin[i] = expected;
						}
						else if (tb.reg_origin[i] != expected)
						{
							// Set -1 if multiple origins merged (requires PHI node)
							tb.reg_origin[i] = -1;

							must_repeat |= !tb.targets.empty();
						}
					}

					if (g_cfg.core.spu_block_size == spu_block_size_type::giga && tb.func == block.func && tb.reg_origin_abs[i] + 2)
					{
						const u32 expected = block.reg_mod[i] ? addr : block.reg_origin_abs[i];

						if (tb.reg_origin_abs[i] == 0x80000000)
						{
							tb.reg_origin_abs[i] = expected;
						}
						else if (tb.reg_origin_abs[i] != expected)
						{
							if (tb.reg_origin_abs[i] == 0x40000 || expected + 2 == 0 || expected == 0x40000)
							{
								// Set -2: sticky value indicating possible external reg origin (0x40000)
								tb.reg_origin_abs[i] = -2;

								must_repeat |= !tb.targets.empty();
							}
							else if (tb.reg_origin_abs[i] + 1)
							{
								tb.reg_origin_abs[i] = -1;

								must_repeat |= !tb.targets.empty();
							}
						}
					}
				}
			}
		}

		if (!must_repeat)
		{
			break;
		}

		for (u32 wi = 0; wi < workload.size(); wi++)
		{
			const u32 addr = workload[wi];
			auto& block    = m_bbs.at(addr);

			// Reset values for the next attempt (keep negative values)
			for (u32 i = 0; i < s_reg_max; i++)
			{
				if (block.reg_origin[i] <= 0x40000)
					block.reg_origin[i] = 0x80000000;
				if (block.reg_origin_abs[i] <= 0x40000)
					block.reg_origin_abs[i] = 0x80000000;
			}
		}
	}

	// Fill more block info
	for (u32 wi = 0; wi < workload.size(); wi++)
	{
		if (g_cfg.core.spu_block_size != spu_block_size_type::giga)
		{
			break;
		}

		const u32 addr = workload[wi];
		auto& bb       = m_bbs.at(addr);
		auto& func     = m_funcs.at(bb.func);

		// Update function size
		func.size = std::max<u16>(func.size, bb.size + (addr - bb.func) / 4);

		// Copy constants according to reg origin info
		for (u32 i = 0; i < s_reg_max; i++)
		{
			const u32 orig = bb.reg_origin_abs[i];

			if (orig < 0x40000)
			{
				auto& src = m_bbs.at(orig);
				bb.reg_const[i] = src.reg_const[i];
				bb.reg_val32[i] = src.reg_val32[i];
			}

			if (!bb.reg_save_dom[i] && bb.reg_use[i] && (orig == 0x40000 || orig + 2 == 0))
			{
				// Destroy offset if external reg value is used
				func.reg_save_off[i] = -1;
			}
		}

		if (u32 orig = bb.reg_origin_abs[s_reg_sp]; orig < 0x40000)
		{
			auto& prologue = m_bbs.at(orig);

			// Copy stack offset (from the assumed prologue)
			bb.stack_sub = prologue.stack_sub;
		}
		else if (orig > 0x40000)
		{
			// Unpredictable stack
			bb.stack_sub = 0x80000000;
		}

		spu_opcode_t op;

		auto last_inst = spu_itype::UNK;

		for (u32 ia = addr; ia < addr + bb.size * 4; ia += 4)
		{
			// Decode instruction again
			op.opcode = std::bit_cast<be_t<u32>>(result.data[(ia - lsa) / 41]);
			last_inst = s_spu_itype.decode(op.opcode);

			// Propagate some constants
			switch (last_inst)
			{
			case spu_itype::IL:
			{
				bb.reg_const[op.rt] = true;
				bb.reg_val32[op.rt] = op.si16;
				break;
			}
			case spu_itype::ILA:
			{
				bb.reg_const[op.rt] = true;
				bb.reg_val32[op.rt] = op.i18;
				break;
			}
			case spu_itype::ILHU:
			{
				bb.reg_const[op.rt] = true;
				bb.reg_val32[op.rt] = op.i16 << 16;
				break;
			}
			case spu_itype::ILH:
			{
				bb.reg_const[op.rt] = true;
				bb.reg_val32[op.rt] = op.i16 << 16 | op.i16;
				break;
			}
			case spu_itype::IOHL:
			{
				bb.reg_val32[op.rt] = bb.reg_val32[op.rt] | op.i16;
				break;
			}
			case spu_itype::ORI:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra];
				bb.reg_val32[op.rt] = bb.reg_val32[op.ra] | op.si10;
				break;
			}
			case spu_itype::OR:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra] & bb.reg_const[op.rb];
				bb.reg_val32[op.rt] = bb.reg_val32[op.ra] | bb.reg_val32[op.rb];
				break;
			}
			case spu_itype::AI:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra];
				bb.reg_val32[op.rt] = bb.reg_val32[op.ra] + op.si10;
				break;
			}
			case spu_itype::A:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra] & bb.reg_const[op.rb];
				bb.reg_val32[op.rt] = bb.reg_val32[op.ra] + bb.reg_val32[op.rb];
				break;
			}
			case spu_itype::SFI:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra];
				bb.reg_val32[op.rt] = op.si10 - bb.reg_val32[op.ra];
				break;
			}
			case spu_itype::SF:
			{
				bb.reg_const[op.rt] = bb.reg_const[op.ra] & bb.reg_const[op.rb];
				bb.reg_val32[op.rt] = bb.reg_val32[op.rb] - bb.reg_val32[op.ra];
				break;
			}
			case spu_itype::STQD:
			{
				if (op.ra == s_reg_sp && bb.stack_sub != 0x80000000 && bb.reg_save_dom[op.rt])
				{
					const u32 offset = 0x80000000 + op.si10 * 16 - bb.stack_sub;

					if (func.reg_save_off[op.rt] == 0)
					{
						// Store reg save offset
						func.reg_save_off[op.rt] = offset;
					}
					else if (func.reg_save_off[op.rt] != offset)
					{
						// Conflict of different offsets
						func.reg_save_off[op.rt] = -1;
					}
				}

				break;
			}
			case spu_itype::LQD:
			{
				if (op.ra == s_reg_sp && bb.stack_sub != 0x80000000 && bb.reg_load_mod[op.rt] == ia + 1)
				{
					// Adjust reg load offset
					bb.reg_load_mod[op.rt] = 0x80000000 + op.si10 * 16 - bb.stack_sub;
				}

				// Clear const
				bb.reg_const[op.rt] = false;
				break;
			}
			default:
			{
				// Clear const if reg is modified here
				if (u8 reg = m_regmod[ia / 4]; reg < s_reg_max)
					bb.reg_const[reg] = false;
				break;
			}
			}

			// $SP is modified
			if (m_regmod[ia / 4] == s_reg_sp)
			{
				if (bb.reg_const[s_reg_sp])
				{
					// Making $SP a constant is a funny thing too.
					bb.stack_sub = 0x80000000;
				}

				if (bb.stack_sub != 0x80000000)
				{
					switch (last_inst)
					{
					case spu_itype::AI:
					{
						if (op.ra == s_reg_sp)
							bb.stack_sub -= op.si10;
						else
							bb.stack_sub = 0x80000000;
						break;
					}
					case spu_itype::A:
					{
						if (op.ra == s_reg_sp && bb.reg_const[op.rb])
							bb.stack_sub -= bb.reg_val32[op.rb];
						else if (op.rb == s_reg_sp && bb.reg_const[op.ra])
							bb.stack_sub -= bb.reg_val32[op.ra];
						else
							bb.stack_sub = 0x80000000;
						break;
					}
					case spu_itype::SF:
					{
						if (op.rb == s_reg_sp && bb.reg_const[op.ra])
							bb.stack_sub += bb.reg_val32[op.ra];
						else
							bb.stack_sub = 0x80000000;
						break;
					}
					default:
					{
						bb.stack_sub = 0x80000000;
						break;
					}
					}
				}

				// Check for funny values.
				if (bb.stack_sub >= 0x40000 || bb.stack_sub % 16)
				{
					bb.stack_sub = 0x80000000;
				}
			}
		}

		// Analyse terminator instruction
		const u32 tia = addr + bb.size * 4 - 4;

		switch (last_inst)
		{
		case spu_itype::BR:
		case spu_itype::BRNZ:
		case spu_itype::BRZ:
		case spu_itype::BRHNZ:
		case spu_itype::BRHZ:
		case spu_itype::BRSL:
		{
			const u32 target = spu_branch_target(tia, op.i16);

			if (target == tia + 4)
			{
				bb.terminator = term_type::fallthrough;
			}
			else if (last_inst != spu_itype::BRSL)
			{
				// No-op terminator or simple branch instruction
				bb.terminator = term_type::br;

				if (target == bb.func)
				{
					// Recursive tail call
					bb.terminator = term_type::ret;
				}
			}
			else if (op.rt == s_reg_lr)
			{
				bb.terminator = term_type::call;
			}
			else
			{
				bb.terminator = term_type::interrupt_call;
			}

			break;
		}
		case spu_itype::BRA:
		case spu_itype::BRASL:
		{
			bb.terminator = term_type::indirect_call;
			break;
		}
		case spu_itype::BI:
		{
			if (op.d || op.e || bb.targets.size() == 1)
			{
				bb.terminator = term_type::interrupt_call;
			}
			else if (bb.targets.size() > 1)
			{
				// Jump table
				bb.terminator = term_type::br;
			}
			else if (op.ra == s_reg_lr)
			{
				// Return (TODO)
				bb.terminator = term_type::ret;
			}
			else
			{
				// Indirect tail call (TODO)
				bb.terminator = term_type::interrupt_call;
			}

			break;
		}
		case spu_itype::BISLED:
		case spu_itype::IRET:
		{
			bb.terminator = term_type::interrupt_call;
			break;
		}
		case spu_itype::BISL:
		case spu_itype::BIZ:
		case spu_itype::BINZ:
		case spu_itype::BIHZ:
		case spu_itype::BIHNZ:
		{
			if (op.d || op.e || bb.targets.size() != 1)
			{
				bb.terminator = term_type::interrupt_call;
			}
			else if (last_inst != spu_itype::BISL && bb.targets[0] == tia + 4 && op.ra == s_reg_lr)
			{
				// Conditional return (TODO)
				bb.terminator = term_type::ret;
			}
			else if (last_inst == spu_itype::BISL)
			{
				// Indirect call
				bb.terminator = term_type::indirect_call;
			}
			else
			{
				// TODO
				bb.terminator = term_type::interrupt_call;
			}

			break;
		}
		default:
		{
			// Normal instruction
			bb.terminator = term_type::fallthrough;
			break;
		}
		}
	}

	// Check function blocks, verify and print some reasons
	for (auto& f : m_funcs)
	{
		if (g_cfg.core.spu_block_size != spu_block_size_type::giga)
		{
			break;
		}

		bool is_ok = true;

		u32 used_stack = 0;

		for (auto it = m_bbs.lower_bound(f.first); it != m_bbs.end() && it->second.func == f.first; ++it)
		{
			auto& bb       = it->second;
			auto& func     = m_funcs.at(bb.func);
			const u32 addr = it->first;
			const u32 flim = bb.func + func.size * 4;

			used_stack |= bb.stack_sub;

			if (is_ok && bb.terminator >= term_type::indirect_call)
			{
				is_ok = false;
			}

			if (is_ok && bb.terminator == term_type::ret)
			{
				// Check $LR (alternative return registers are currently not supported)
				if (u32 lr_orig = bb.reg_mod[s_reg_lr] ? addr : bb.reg_origin_abs[s_reg_lr]; lr_orig < 0x40000)
				{
					auto& src = m_bbs.at(lr_orig);

					if (src.reg_load_mod[s_reg_lr] != func.reg_save_off[s_reg_lr])
					{
						spu_log.error("Function 0x%05x: [0x%05x] $LR mismatch (src=0x%x; 0x%x vs 0x%x)", f.first, addr, lr_orig, src.reg_load_mod[0], func.reg_save_off[0]);
						is_ok = false;
					}
					else if (src.reg_load_mod[s_reg_lr] == 0)
					{
						spu_log.error("Function 0x%05x: [0x%05x] $LR modified (src=0x%x)", f.first, addr, lr_orig);
						is_ok = false;
					}
				}
				else if (lr_orig > 0x40000)
				{
					spu_log.todo("Function 0x%05x: [0x%05x] $LR unpredictable (src=0x%x)", f.first, addr, lr_orig);
					is_ok = false;
				}

				// Check $80..$127 (should be restored or unmodified)
				for (u32 i = s_reg_80; is_ok && i <= s_reg_127; i++)
				{
					if (u32 orig = bb.reg_mod[i] ? addr : bb.reg_origin_abs[i]; orig < 0x40000)
					{
						auto& src = m_bbs.at(orig);

						if (src.reg_load_mod[i] != func.reg_save_off[i])
						{
							spu_log.error("Function 0x%05x: [0x%05x] $%u mismatch (src=0x%x; 0x%x vs 0x%x)", f.first, addr, i, orig, src.reg_load_mod[i], func.reg_save_off[i]);
							is_ok = false;
						}
					}
					else if (orig > 0x40000)
					{
						spu_log.todo("Function 0x%05x: [0x%05x] $%u unpredictable (src=0x%x)", f.first, addr, i, orig);
						is_ok = false;
					}

					if (func.reg_save_off[i] + 1 == 0)
					{
						spu_log.error("Function 0x%05x: [0x%05x] $%u used incorrectly", f.first, addr, i);
						is_ok = false;
					}
				}

				// Check $SP (should be restored or unmodified)
				if (bb.stack_sub != 0 && bb.stack_sub != 0x80000000)
				{
					spu_log.error("Function 0x%05x: [0x%05x] return with stack frame 0x%x", f.first, addr, bb.stack_sub);
					is_ok = false;
				}
			}

			if (is_ok && bb.terminator == term_type::call)
			{
				// Check call instruction (TODO)
				if (bb.stack_sub == 0)
				{
					// Call without a stack frame
					spu_log.error("Function 0x%05x: [0x%05x] frameless call", f.first, addr);
					is_ok = false;
				}
			}

			if (is_ok && bb.terminator == term_type::fallthrough)
			{
				// Can't just fall out of the function
				if (bb.targets.size() != 1 || bb.targets[0] >= flim)
				{
					spu_log.error("Function 0x%05x: [0x%05x] bad fallthrough to 0x%x", f.first, addr, bb.targets[0]);
					is_ok = false;
				}
			}

			if (is_ok && bb.stack_sub == 0x80000000)
			{
				spu_log.error("Function 0x%05x: [0x%05x] bad stack frame", f.first, addr);
				is_ok = false;
			}

			// Fill external function targets (calls, possibly tail calls)
			for (u32 target : bb.targets)
			{
				if (target < bb.func || target >= flim || (bb.terminator == term_type::call && target == bb.func))
				{
					if (func.calls.find_first_of(target) + 1 == 0)
					{
						func.calls.push_back(target);
					}
				}
			}
		}

		if (is_ok && used_stack && f.first == entry_point)
		{
			spu_log.error("Function 0x%05x: considered possible chunk", f.first);
			is_ok = false;
		}

		// if (is_ok && f.first > 0x1d240 && f.first < 0x1e000)
		// {
		// 	spu_log.error("Function 0x%05x: manually disabled", f.first);
		// 	is_ok = false;
		// }

		f.second.good = is_ok;
	}

	// Check function call graph
	while (g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		bool need_repeat = false;

		for (auto& f : m_funcs)
		{
			if (!f.second.good)
			{
				continue;
			}

			for (u32 call : f.second.calls)
			{
				const auto ffound = std::as_const(m_funcs).find(call);

				if (ffound == m_funcs.cend() || ffound->second.good == false)
				{
					need_repeat = true;

					if (f.second.good)
					{
						spu_log.error("Function 0x%05x: calls bad function (0x%05x)", f.first, call);
						f.second.good = false;
					}
				}
			}
		}

		if (!need_repeat)
		{
			break;
		}
	}

	if (result.data.empty())
	{
		// Blocks starting from 0x0 or invalid instruction won't be compiled, may need special interpreter fallback
	}

	return result;
}

void spu_recompiler_base::dump(const spu_program& result, std::string& out)
{
	SPUDisAsm dis_asm(CPUDisAsm_InterpreterMode);
	dis_asm.offset = reinterpret_cast<const u8*>(result.data.data()) - result.lower_bound;

	std::string hash;
	{
		sha1_context ctx;
		u8 output[20];

		sha1_starts(&ctx);
		sha1_update(&ctx, reinterpret_cast<const u8*>(result.data.data()), result.data.size() * 4);
		sha1_finish(&ctx, output);
		fmt::append(hash, "%s", fmt::base57(output));
	}

	fmt::append(out, "========== SPU BLOCK 0x%05x (size %u, %s) ==========\n", result.entry_point, result.data.size(), hash);

	for (auto& bb : m_bbs)
	{
		for (u32 pos = bb.first, end = bb.first + bb.second.size * 4; pos < end; pos += 4)
		{
			dis_asm.dump_pc = pos;
			dis_asm.disasm(pos);
			fmt::append(out, ">%s\n", dis_asm.last_opcode);
		}

		if (m_block_info[bb.first / 4])
		{
			fmt::append(out, "A: [0x%05x] %s\n", bb.first, m_entry_info[bb.first / 4] ? (m_ret_info[bb.first / 4] ? "Chunk" : "Entry") : "Block");

			fmt::append(out, "\tF: 0x%05x\n", bb.second.func);

			for (u32 pred : bb.second.preds)
			{
				fmt::append(out, "\t<- 0x%05x\n", pred);
			}

			for (u32 target : bb.second.targets)
			{
				fmt::append(out, "\t-> 0x%05x%s\n", target, m_bbs.count(target) ? "" : " (null)");
			}
		}
		else
		{
			fmt::append(out, "A: [0x%05x] ?\n", bb.first);
		}

		out += '\n';
	}

	for (auto& f : m_funcs)
	{
		fmt::append(out, "F: [0x%05x]%s\n", f.first, f.second.good ? " (good)" : " (bad)");

		fmt::append(out, "\tN: 0x%05x\n", f.second.size * 4 + f.first);

		for (u32 call : f.second.calls)
		{
			fmt::append(out, "\t>> 0x%05x%s\n", call, m_funcs.count(call) ? "" : " (null)");
		}
	}

	out += '\n';
}

#ifdef LLVM_AVAILABLE

#include "Emu/CPU/CPUTranslator.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "llvm/ADT/Triple.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/Analysis/Lint.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Vectorize.h"
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

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

	// Main entry point offset
	u32 m_base;

	// Module name
	std::string m_hash;

	// Patchpoint unique id
	u32 m_pp_id = 0;

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
	llvm::Value* m_base_pc;
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

	// Helper for check_state
	llvm::GlobalVariable* m_fake_global1{};

	// Function for check_state execution
	llvm::Function* m_test_state{};

	// Chunk for external tail call (dispatch)
	llvm::Function* m_dispatch{};

	llvm::MDNode* m_md_unlikely;
	llvm::MDNode* m_md_likely;

	struct block_info
	{
		// Pointer to the analyser
		spu_recompiler_base::block_info* bb{};

		// Current block's entry block
		llvm::BasicBlock* block;

		// Final block (for PHI nodes, set after completion)
		llvm::BasicBlock* block_end{};

		// Current register values
		std::array<llvm::Value*, s_reg_max> reg{};

		// PHI nodes created for this block (if any)
		std::array<llvm::PHINode*, s_reg_max> phi{};

		// Store instructions
		std::array<llvm::StoreInst*, s_reg_max> store{};
	};

	struct function_info
	{
		// Standard callable chunk
		llvm::Function* chunk{};

		// Callable function
		llvm::Function* fn{};

		// Registers possibly loaded in the entry block
		std::array<llvm::Value*, s_reg_max> load{};
	};

	// Current block
	block_info* m_block;

	// Current function or chunk
	function_info* m_finfo;

	// All blocks in the current function chunk
	std::unordered_map<u32, block_info, value_hash<u32, 2>> m_blocks;

	// Block list for processing
	std::vector<u32> m_block_queue;

	// All function chunks in current SPU compile unit
	std::unordered_map<u32, function_info, value_hash<u32, 2>> m_functions;

	// Function chunk list for processing
	std::vector<u32> m_function_queue;

	// Add or get the function chunk
	function_info* add_function(u32 addr)
	{
		// Enqueue if necessary
		const auto empl = m_functions.try_emplace(addr);

		if (!empl.second)
		{
			return &empl.first->second;
		}

		// Chunk function type
		// 0. Result (tail call target)
		// 1. Thread context
		// 2. Local storage pointer
		// 3.
#if 0
		const auto chunk_type = get_ftype<u8*, u8*, u8*, u32>();
#else
		const auto chunk_type = get_ftype<void, u8*, u8*, u32>();
#endif

		// Get function chunk name
		const std::string name = fmt::format("spu-chunk-0x%05x", addr);
		llvm::Function* result = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(name, chunk_type).getCallee());

		// Set parameters
		result->setLinkage(llvm::GlobalValue::InternalLinkage);
		result->addAttribute(1, llvm::Attribute::NoAlias);
		result->addAttribute(2, llvm::Attribute::NoAlias);
#if 1
		result->setCallingConv(llvm::CallingConv::GHC);
#endif

		empl.first->second.chunk = result;

		if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
		{
			// Find good real function
			const auto ffound = m_funcs.find(addr);

			if (ffound != m_funcs.end() && ffound->second.good)
			{
				// Real function type (not equal to chunk type)
				// 4. $SP
				// 5. $3
				const auto func_type = get_ftype<u32[4], u8*, u8*, u32, u32[4], u32[4]>();

				const std::string fname = fmt::format("spu-function-0x%05x", addr);
				llvm::Function* fn = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(fname, func_type).getCallee());

				fn->setLinkage(llvm::GlobalValue::InternalLinkage);
				fn->addAttribute(1, llvm::Attribute::NoAlias);
				fn->addAttribute(2, llvm::Attribute::NoAlias);
#if 1
				fn->setCallingConv(llvm::CallingConv::GHC);
#endif
				empl.first->second.fn = fn;
			}
		}

		// Enqueue
		m_function_queue.push_back(addr);

		return &empl.first->second;
	}

	// Create tail call to the function chunk (non-tail calls are just out of question)
	void tail_chunk(llvm::Value* chunk, llvm::Value* base_pc = nullptr)
	{
		if (!chunk && !g_cfg.core.spu_verification)
		{
			// Disable patchpoints if verification is disabled
			chunk = m_dispatch;
		}
		else if (!chunk)
		{
			// Create branch patchpoint if chunk == nullptr
			verify(HERE), m_finfo, !m_finfo->fn || m_function == m_finfo->chunk;

			// Register under a unique linkable name
			const std::string ppname = fmt::format("%s-pp-%u", m_hash, m_pp_id++);
			m_engine->updateGlobalMapping(ppname, reinterpret_cast<u64>(m_spurt->make_branch_patchpoint()));

			// Create function with not exactly correct type
			const auto ppfunc = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(ppname, m_finfo->chunk->getFunctionType()).getCallee());
			ppfunc->setCallingConv(m_finfo->chunk->getCallingConv());

			if (m_finfo->chunk->getReturnType() != get_type<void>())
			{
				m_ir->CreateRet(m_ir->CreateBitCast(ppfunc, get_type<u8*>()));
				return;
			}

			chunk = ppfunc;
			base_pc = m_ir->getInt32(0);
		}

		auto call = m_ir->CreateCall(chunk, {m_thread, m_lsptr, base_pc ? base_pc : m_base_pc});
		auto func = m_finfo ? m_finfo->chunk : llvm::cast<llvm::Function>(chunk);
		call->setCallingConv(func->getCallingConv());
		call->setTailCall();

		if (func->getReturnType() == get_type<void>())
		{
			m_ir->CreateRetVoid();
		}
		else
		{
			m_ir->CreateRet(call);
		}
	}

	// Call the real function
	void call_function(llvm::Function* fn, bool tail = false)
	{
		llvm::Value* lr{};
		llvm::Value* sp{};
		llvm::Value* r3{};

		if (!m_finfo->fn && !m_block)
		{
			lr = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::gpr, +s_reg_lr, &v128::_u32, 3));
			sp = m_ir->CreateLoad(spu_ptr<u32[4]>(&spu_thread::gpr, +s_reg_sp));
			r3 = m_ir->CreateLoad(spu_ptr<u32[4]>(&spu_thread::gpr, 3));
		}
		else
		{
			lr = m_ir->CreateExtractElement(get_reg_fixed<u32[4]>(s_reg_lr).value, 3);
			sp = get_reg_fixed<u32[4]>(s_reg_sp).value;
			r3 = get_reg_fixed<u32[4]>(3).value;
		}

		const auto _call = m_ir->CreateCall(verify(HERE, fn), {m_thread, m_lsptr, m_base_pc, sp, r3});

		_call->setCallingConv(fn->getCallingConv());

		// Tail call using loaded LR value (gateway from a chunk)
		if (!m_finfo->fn)
		{
			lr = m_ir->CreateAnd(lr, 0x3fffc);
			m_ir->CreateStore(lr, spu_ptr<u32>(&spu_thread::pc));
			m_ir->CreateStore(_call, spu_ptr<u32[4]>(&spu_thread::gpr, 3));
			m_ir->CreateBr(add_block_indirect({}, value<u32>(lr)));
		}
		else if (tail)
		{
			_call->setTailCall();
			m_ir->CreateRet(_call);
		}
		else
		{
			// TODO: initialize $LR with a constant
			for (u32 i = 0; i < s_reg_max; i++)
			{
				if (i != s_reg_lr && i != s_reg_sp && (i < s_reg_80 || i > s_reg_127))
				{
					m_block->reg[i] = m_ir->CreateLoad(init_reg_fixed(i));
				}
			}

			// Set result
			m_block->reg[3] = _call;
		}
	}

	// Emit return from the real function
	void ret_function()
	{
		m_ir->CreateRet(get_reg_fixed<u32[4]>(3).value);
	}

	void set_function(llvm::Function* func)
	{
		m_function = func;
		m_thread = &*func->arg_begin();
		m_lsptr = &*(func->arg_begin() + 1);
		m_base_pc = &*(func->arg_begin() + 2);

		m_reg_addr.fill(nullptr);
		m_block = nullptr;
		m_finfo = nullptr;
		m_blocks.clear();
		m_block_queue.clear();
		m_ir->SetInsertPoint(llvm::BasicBlock::Create(m_context, "", m_function));
		m_memptr = m_ir->CreateLoad(spu_ptr<u8*>(&spu_thread::memory_base_addr));
	}

	// Add block with current block as a predecessor
	llvm::BasicBlock* add_block(u32 target, bool absolute = false)
	{
		// Check the predecessor
		const bool pred_found = m_block_info[target / 4] && m_preds[target].find_first_of(m_pos) + 1;

		if (m_blocks.empty())
		{
			// Special case: first block, proceed normally
			if (auto fn = std::exchange(m_finfo->fn, nullptr))
			{
				// Create a gateway
				call_function(fn, true);

				m_finfo->fn = fn;
				m_function = fn;
				m_thread = &*fn->arg_begin();
				m_lsptr = &*(fn->arg_begin() + 1);
				m_base_pc = &*(fn->arg_begin() + 2);
				m_ir->SetInsertPoint(llvm::BasicBlock::Create(m_context, "", fn));
				m_memptr = m_ir->CreateLoad(spu_ptr<u8*>(&spu_thread::memory_base_addr));

				// Load registers at the entry chunk
				for (u32 i = 0; i < s_reg_max; i++)
				{
					if (i >= s_reg_80 && i <= s_reg_127)
					{
						// TODO
						//m_finfo->load[i] = llvm::UndefValue::get(get_reg_type(i));
					}

					m_finfo->load[i] = m_ir->CreateLoad(init_reg_fixed(i));
				}

				// Load $SP
				m_finfo->load[s_reg_sp] = &*(fn->arg_begin() + 3);

				// Load first args
				m_finfo->load[3] = &*(fn->arg_begin() + 4);
			}
		}
		else if (m_block_info[target / 4] && m_entry_info[target / 4] && !(pred_found && m_entry == target) && (!m_finfo->fn || !m_ret_info[target / 4]))
		{
			// Generate a tail call to the function chunk
			const auto cblock = m_ir->GetInsertBlock();
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);
			const auto pfinfo = add_function(target);

			if (absolute)
			{
				verify(HERE), !m_finfo->fn;

				const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
				m_ir->CreateCondBr(m_ir->CreateICmpEQ(m_base_pc, m_ir->getInt32(m_base)), next, fail);
				m_ir->SetInsertPoint(fail);
				m_ir->CreateStore(m_ir->getInt32(target), spu_ptr<u32>(&spu_thread::pc), true);
				tail_chunk(nullptr);
				m_ir->SetInsertPoint(next);
			}

			if (pfinfo->fn)
			{
				// Tail call to the real function
				call_function(pfinfo->fn, true);

				if (!result->getTerminator())
					ret_function();
			}
			else
			{
				// Just a boring tail call to another chunk
				update_pc(target);
				tail_chunk(pfinfo->chunk);
			}

			m_ir->SetInsertPoint(cblock);
			return result;
		}
		else if (!pred_found || !m_block_info[target / 4])
		{
			if (m_block_info[target / 4])
			{
				spu_log.error("[0x%x] Predecessor not found for target 0x%x (chunk=0x%x, entry=0x%x, size=%u)", m_pos, target, m_entry, m_function_queue[0], m_size / 4);
			}

			const auto cblock = m_ir->GetInsertBlock();
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);

			if (absolute)
			{
				verify(HERE), !m_finfo->fn;

				m_ir->CreateStore(m_ir->getInt32(target), spu_ptr<u32>(&spu_thread::pc), true);
			}
			else
			{
				update_pc(target);
			}

			tail_chunk(nullptr);
			m_ir->SetInsertPoint(cblock);
			return result;
		}

		verify(HERE), !absolute;

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
		return bitcast<u64[4]>(val);
	}

	llvm::Value* uint64_as_double(llvm::Value* val)
	{
		return bitcast<f64[4]>(val);
	}

	llvm::Value* double_to_xfloat(llvm::Value* val)
	{
		verify("double_to_xfloat" HERE), val, val->getType() == get_type<f64[4]>();

		const auto d = double_as_uint64(val);
		const auto s = m_ir->CreateAnd(m_ir->CreateLShr(d, 32), 0x80000000);
		const auto m = m_ir->CreateXor(m_ir->CreateLShr(d, 29), 0x40000000);
		const auto r = m_ir->CreateOr(m_ir->CreateAnd(m, 0x7fffffff), s);
		return m_ir->CreateTrunc(m_ir->CreateSelect(m_ir->CreateIsNotNull(d), r, splat<u64[4]>(0).eval(m_ir)), get_type<u32[4]>());
	}

	llvm::Value* xfloat_to_double(llvm::Value* val)
	{
		verify("xfloat_to_double" HERE), val, val->getType() == get_type<u32[4]>();

		const auto x = m_ir->CreateZExt(val, get_type<u64[4]>());
		const auto s = m_ir->CreateShl(m_ir->CreateAnd(x, 0x80000000), 32);
		const auto a = m_ir->CreateAnd(x, 0x7fffffff);
		const auto m = m_ir->CreateShl(m_ir->CreateAdd(a, splat<u64[4]>(0x1c0000000).eval(m_ir)), 29);
		const auto r = m_ir->CreateSelect(m_ir->CreateICmpSGT(a, splat<u64[4]>(0x7fffff).eval(m_ir)), m, splat<u64[4]>(0).eval(m_ir));
		const auto f = m_ir->CreateOr(s, r);
		return uint64_as_double(f);
	}

	// Clamp double values to ±Smax, flush values smaller than ±Smin to positive zero
	llvm::Value* xfloat_in_double(llvm::Value* val)
	{
		verify("xfloat_in_double" HERE), val, val->getType() == get_type<f64[4]>();

		const auto smax = uint64_as_double(splat<u64[4]>(0x47ffffffe0000000).eval(m_ir));
		const auto smin = uint64_as_double(splat<u64[4]>(0x3810000000000000).eval(m_ir));

		const auto d = double_as_uint64(val);
		const auto s = m_ir->CreateAnd(d, 0x8000000000000000);
		const auto a = uint64_as_double(m_ir->CreateAnd(d, 0x7fffffffe0000000));
		const auto n = m_ir->CreateFCmpOLT(a, smax);
		const auto z = m_ir->CreateFCmpOLT(a, smin);
		const auto c = double_as_uint64(m_ir->CreateSelect(n, a, smax));
		return m_ir->CreateSelect(z, fsplat<f64[4]>(0.).eval(m_ir), uint64_as_double(m_ir->CreateOr(c, s)));
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
			reg = m_finfo && m_finfo->load[index] ? m_finfo->load[index] : m_ir->CreateLoad(init_reg_fixed(index));
		}

		if (reg->getType() == get_type<f64[4]>())
		{
			if (type == reg->getType())
			{
				return reg;
			}

			return bitcast(double_to_xfloat(reg), type);
		}

		if (type == get_type<f64[4]>())
		{
			return xfloat_to_double(bitcast<u32[4]>(reg));
		}

		return bitcast(reg, type);
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
			if (I >= (32u - m_interp_magn))
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

	template <typename U, uint I>
	auto get_vr_as(U&&, const bf_t<u32, I, 7>& index)
	{
		return get_vr<typename llvm_expr_t<U>::type>(index);
	}

	template <typename T = u32[4], typename... Args>
	std::tuple<std::conditional_t<false, Args, value_t<T>>...> get_vrs(const Args&... args)
	{
		return {get_vr<T>(args)...};
	}

	template <typename T = u32[4], uint I>
	llvm_match_t<T> match_vr(const bf_t<u32, I, 7>& index)
	{
		llvm_match_t<T> r;

		if (m_block)
		{
			auto v = m_block->reg.at(index);

			if (v && v->getType() == get_type<T>())
			{
				r.value = v;
				return r;
			}
		}

		return r;
	}

	template <typename U, uint I>
	auto match_vr_as(U&&, const bf_t<u32, I, 7>& index)
	{
		return match_vr<typename llvm_expr_t<U>::type>(index);
	}

	template <typename... Types, uint I, typename F>
	bool match_vr(const bf_t<u32, I, 7>& index, F&& pred)
	{
		return (( match_vr<Types>(index) ? pred(match_vr<Types>(index), match<Types>()) : false ) || ...);
	}

	template <typename T = u32[4], typename... Args>
	std::tuple<std::conditional_t<false, Args, llvm_match_t<T>>...> match_vrs(const Args&... args)
	{
		return {match_vr<T>(args)...};
	}

	// Extract scalar value from the preferred slot
	template <typename T>
	auto get_scalar(value_t<T> value)
	{
		using e_type = std::remove_extent_t<T>;

		static_assert(sizeof(T) == 16 || std::is_same_v<f64[4], T>, "Unknown vector type");

		if (auto [ok, v] = match_expr(value, vsplat<T>(match<e_type>())); ok)
		{
			return eval(v);
		}

		if constexpr (sizeof(e_type) == 1)
		{
			return eval(extract(value, 12));
		}
		else if constexpr (sizeof(e_type) == 2)
		{
			return eval(extract(value, 6));
		}
		else if constexpr (sizeof(e_type) == 4 || sizeof(T) == 32)
		{
			return eval(extract(value, 3));
		}
		else
		{
			return eval(extract(value, 1));
		}
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

		if (m_finfo && m_finfo->fn)
		{
			if (index <= 3 || (index >= s_reg_80 && index <= s_reg_127))
			{
				// Don't save some registers in true functions
				return;
			}
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
			if (I >= (32u - m_interp_magn))
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
			if (I >= (32u - m_interp_magn))
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

		return eval(splat<T>(imm));
	}

	template <typename T = u32[4], uint I, uint N>
	value_t<T> get_imm(const bf_t<s32, I, N>& imm)
	{
		if ((m_op_const_mask & imm.data_mask()) != imm.data_mask())
		{
			// Update const mask if necessary
			if (I >= (32u - m_interp_magn))
			{
				m_op_const_mask |= imm.data_mask();
			}

			// Extract signed immediate (skip sign ext if truncated anyway)
			value_t<T> r;
			r.value = m_interp_op;
			r.value = I + N == 32 || N >= r.esize ? r.value : m_ir->CreateShl(r.value, u64{32u - I - N});
			r.value = N == 32 || N >= r.esize ? r.value : m_ir->CreateAShr(r.value, u64{32u - N});
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

		return eval(splat<T>(imm));
	}

	// Get PC for given instruction address
	llvm::Value* get_pc(u32 addr)
	{
		return m_ir->CreateAdd(m_base_pc, m_ir->getInt32(addr - m_base));
	}

	// Update PC for current or explicitly specified instruction address
	void update_pc(u32 target = -1)
	{
		m_ir->CreateStore(m_ir->CreateAnd(get_pc(target + 1 ? target : m_pos), 0x3fffc), spu_ptr<u32>(&spu_thread::pc), true);
	}

	// Call cpu_thread::check_state if necessary and return or continue (full check)
	void check_state(u32 addr)
	{
		const auto pstate = spu_ptr<u32>(&spu_thread::state);
		const auto _body = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto check = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(m_ir->CreateICmpEQ(m_ir->CreateLoad(pstate, true), m_ir->getInt32(0)), _body, check, m_md_likely);
		m_ir->SetInsertPoint(check);
		update_pc(addr);
		m_ir->CreateStore(m_ir->getFalse(), m_fake_global1, true);
		m_ir->CreateBr(_body);
		m_ir->SetInsertPoint(_body);
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
			m_spurt = g_fxo->get<spu_runtime>();
			cpu_translator::initialize(m_jit.get_context(), m_jit.get_engine());

			const auto md_name = llvm::MDString::get(m_context, "branch_weights");
			const auto md_low = llvm::ValueAsMetadata::get(llvm::ConstantInt::get(GetType<u32>(), 1));
			const auto md_high = llvm::ValueAsMetadata::get(llvm::ConstantInt::get(GetType<u32>(), 999));

			// Metadata for branch weights
			m_md_likely = llvm::MDTuple::get(m_context, {md_name, md_high, md_low});
			m_md_unlikely = llvm::MDTuple::get(m_context, {md_name, md_low, md_high});
		}
	}

	virtual spu_function_t compile(spu_program&& _func) override
	{
		if (_func.data.empty() && m_interp_magn)
		{
			return compile_interpreter();
		}

		const u32 start0 = _func.entry_point;

		const auto add_loc = m_spurt->add_empty(std::move(_func));

		if (!add_loc)
		{
			return nullptr;
		}

		const spu_program& func = add_loc->data;

		if (func.entry_point != start0)
		{
			// Wait for the duplicate
			while (!add_loc->compiled)
			{
				add_loc->compiled.wait(nullptr);
			}

			return add_loc->compiled;
		}

		std::string log;

		if (auto cache = g_fxo->get<spu_cache>(); cache && g_cfg.core.spu_cache && !add_loc->cached.exchange(1))
		{
			cache->add(func);
		}

		{
			sha1_context ctx;
			u8 output[20];

			sha1_starts(&ctx);
			sha1_update(&ctx, reinterpret_cast<const u8*>(func.data.data()), func.data.size() * 4);
			sha1_finish(&ctx, output);

			m_hash.clear();
			fmt::append(m_hash, "spu-0x%05x-%s", func.entry_point, fmt::base57(output));

			be_t<u64> hash_start;
			std::memcpy(&hash_start, output, sizeof(hash_start));
			m_hash_start = hash_start;
		}

		spu_log.notice("Building function 0x%x... (size %u, %s)", func.entry_point, func.data.size(), m_hash);

		m_pos = func.lower_bound;
		m_base = func.entry_point;
		m_size = ::size32(func.data) * 4;
		const u32 start = m_pos;
		const u32 end = start + m_size;

		m_pp_id = 0;

		if (g_cfg.core.spu_debug && !add_loc->logged.exchange(1))
		{
			this->dump(func, log);
			fs::file(m_spurt->get_cache_path() + "spu.log", fs::write + fs::append).write(log);
		}

		using namespace llvm;

		m_engine->clearAllGlobalMappings();

		// Create LLVM module
		std::unique_ptr<Module> module = std::make_unique<Module>(m_hash + ".obj", m_context);
		module->setTargetTriple(Triple::normalize("x86_64-unknown-linux-gnu"));
		module->setDataLayout(m_jit.get_engine().getTargetMachine()->createDataLayout());
		m_module = module.get();

		// Initialize IR Builder
		IRBuilder<> irb(m_context);
		m_ir = &irb;

		// Helper for check_state. Used to not interfere with LICM pass.
		m_fake_global1 = new llvm::GlobalVariable(*m_module, get_type<bool>(), false, llvm::GlobalValue::InternalLinkage, m_ir->getFalse());

		// Add entry function (contains only state/code check)
		const auto main_func = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(m_hash, get_ftype<void, u8*, u8*, u64>()).getCallee());
		const auto main_arg2 = &*(main_func->arg_begin() + 2);
		main_func->setCallingConv(CallingConv::GHC);
		set_function(main_func);

		// Start compilation
		const auto label_test = BasicBlock::Create(m_context, "", m_function);
		const auto label_diff = BasicBlock::Create(m_context, "", m_function);
		const auto label_body = BasicBlock::Create(m_context, "", m_function);
		const auto label_stop = BasicBlock::Create(m_context, "", m_function);

		// Load PC, which will be the actual value of 'm_base'
		m_base_pc = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::pc));

		// Emit state check
		const auto pstate = spu_ptr<u32>(&spu_thread::state);
		m_ir->CreateCondBr(m_ir->CreateICmpNE(m_ir->CreateLoad(pstate, true), m_ir->getInt32(0)), label_stop, label_test, m_md_unlikely);

		// Emit code check
		u32 check_iterations = 0;
		m_ir->SetInsertPoint(label_test);

		// Set block hash for profiling (if enabled)
		if (g_cfg.core.spu_prof && g_cfg.core.spu_verification)
			m_ir->CreateStore(m_ir->getInt64((m_hash_start & -65536)), spu_ptr<u64>(&spu_thread::block_hash), true);

		if (!g_cfg.core.spu_verification)
		{
			// Disable check (unsafe)
			m_ir->CreateBr(label_body);
		}
		else if (func.data.size() == 1)
		{
			const auto pu32 = m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, m_base_pc), get_type<u32*>());
			const auto cond = m_ir->CreateICmpNE(m_ir->CreateLoad(pu32), m_ir->getInt32(func.data[0]));
			m_ir->CreateCondBr(cond, label_diff, label_body, m_md_unlikely);
		}
		else if (func.data.size() == 2)
		{
			const auto pu64 = m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, m_base_pc), get_type<u64*>());
			const auto cond = m_ir->CreateICmpNE(m_ir->CreateLoad(pu64), m_ir->getInt64(static_cast<u64>(func.data[1]) << 32 | func.data[0]));
			m_ir->CreateCondBr(cond, label_diff, label_body, m_md_unlikely);
		}
		else
		{
			u32 starta = start;

			// Skip holes at the beginning (giga only)
			for (u32 j = start; j < end; j += 4)
			{
				if (!func.data[(j - start) / 4])
				{
					starta += 4;
				}
				else
				{
					break;
				}
			}

			// Get actual pc corresponding to the found beginning of the data
			llvm::Value* starta_pc = m_ir->CreateAnd(get_pc(starta), 0x3fffc);
			llvm::Value* data_addr = m_ir->CreateGEP(m_lsptr, starta_pc);

			llvm::Value* acc = nullptr;

			for (u32 j = starta; j < end; j += 32)
			{
				u32 indices[8];
				bool holes = false;
				bool data = false;

				for (u32 i = 0; i < 8; i++)
				{
					const u32 k = j + i * 4;

					if (k < start || k >= end || !func.data[(k - start) / 4])
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
					// Skip full-sized holes
					continue;
				}

				// Load unaligned code block from LS
				llvm::Value* vls = m_ir->CreateAlignedLoad(_ptr<u32[8]>(data_addr, j - starta), 4);

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
					words[i] = k >= start && k < end ? func.data[(k - start) / 4] : 0;
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
		const auto entry_call = m_ir->CreateCall(entry_chunk->chunk, {m_thread, m_lsptr, m_base_pc});
		entry_call->setCallingConv(entry_chunk->chunk->getCallingConv());

		const auto dispatcher = llvm::cast<llvm::Function>(m_module->getOrInsertFunction("spu_dispatcher", main_func->getType()).getCallee());
		m_engine->updateGlobalMapping("spu_dispatcher", reinterpret_cast<u64>(spu_runtime::tr_all));
		dispatcher->setCallingConv(main_func->getCallingConv());

		// Proceed to the next code
		if (entry_chunk->chunk->getReturnType() != get_type<void>())
		{
			const auto next_call = m_ir->CreateCall(m_ir->CreateBitCast(entry_call, main_func->getType()), {m_thread, m_lsptr, m_ir->getInt64(0)});
			next_call->setCallingConv(main_func->getCallingConv());
			next_call->setTailCall();
		}
		else
		{
			entry_call->setTailCall();
		}

		m_ir->CreateRetVoid();

		m_ir->SetInsertPoint(label_stop);
		m_ir->CreateRetVoid();

		m_ir->SetInsertPoint(label_diff);

		if (g_cfg.core.spu_verification)
		{
			const auto pbfail = spu_ptr<u64>(&spu_thread::block_failure);
			m_ir->CreateStore(m_ir->CreateAdd(m_ir->CreateLoad(pbfail), m_ir->getInt64(1)), pbfail);
			const auto dispci = call("spu_dispatch", spu_runtime::tr_dispatch, m_thread, m_lsptr, main_arg2);
			dispci->setCallingConv(CallingConv::GHC);
			dispci->setTailCall();
			m_ir->CreateRetVoid();
		}
		else
		{
			m_ir->CreateUnreachable();
		}

		m_dispatch = cast<Function>(module->getOrInsertFunction("spu-null", entry_chunk->chunk->getFunctionType()).getCallee());
		m_dispatch->setLinkage(llvm::GlobalValue::InternalLinkage);
		m_dispatch->setCallingConv(entry_chunk->chunk->getCallingConv());
		set_function(m_dispatch);

		if (entry_chunk->chunk->getReturnType() == get_type<void>())
		{
			const auto next_call = m_ir->CreateCall(m_ir->CreateBitCast(dispatcher, main_func->getType()), {m_thread, m_lsptr, m_ir->getInt64(0)});
			next_call->setCallingConv(main_func->getCallingConv());
			next_call->setTailCall();
			m_ir->CreateRetVoid();
		}
		else
		{
			m_ir->CreateRet(m_ir->CreateBitCast(dispatcher, get_type<u8*>()));
		}

		// Function that executes check_state and escapes if necessary
		m_test_state = llvm::cast<llvm::Function>(m_module->getOrInsertFunction("spu_test_state", get_ftype<void, u8*>()).getCallee());
		m_test_state->setLinkage(GlobalValue::InternalLinkage);
		m_test_state->setCallingConv(CallingConv::PreserveAll);
		m_ir->SetInsertPoint(BasicBlock::Create(m_context, "", m_test_state));
		const auto escape_yes = BasicBlock::Create(m_context, "", m_test_state);
		const auto escape_no = BasicBlock::Create(m_context, "", m_test_state);
		m_ir->CreateCondBr(call("spu_exec_check_state", &exec_check_state, &*m_test_state->arg_begin()), escape_yes, escape_no);
		m_ir->SetInsertPoint(escape_yes);
		call("spu_escape", spu_runtime::g_escape, &*m_test_state->arg_begin());
		m_ir->CreateRetVoid();
		m_ir->SetInsertPoint(escape_no);
		m_ir->CreateRetVoid();

		// Create function table (uninitialized)
		m_function_table = new llvm::GlobalVariable(*m_module, llvm::ArrayType::get(entry_chunk->chunk->getType(), m_size / 4), true, llvm::GlobalValue::InternalLinkage, nullptr);

		// Create function chunks
		for (std::size_t fi = 0; fi < m_function_queue.size(); fi++)
		{
			// Initialize function info
			m_entry = m_function_queue[fi];
			set_function(m_functions[m_entry].chunk);

			// Set block hash for profiling (if enabled)
			if (g_cfg.core.spu_prof)
				m_ir->CreateStore(m_ir->getInt64((m_hash_start & -65536) | (m_entry >> 2)), spu_ptr<u64>(&spu_thread::block_hash), true);

			m_finfo = &m_functions[m_entry];
			m_ir->CreateBr(add_block(m_entry));

			// Emit instructions for basic blocks
			for (std::size_t bi = 0; bi < m_block_queue.size(); bi++)
			{
				// Initialize basic block info
				const u32 baddr = m_block_queue[bi];
				m_block = &m_blocks[baddr];
				m_ir->SetInsertPoint(m_block->block);
				auto& bb = m_bbs.at(baddr);
				bool need_check = false;
				m_block->bb = &bb;

				if (bb.preds.size())
				{
					// Initialize registers and build PHI nodes if necessary
					for (u32 i = 0; i < s_reg_max; i++)
					{
						const u32 src = m_finfo->fn ? bb.reg_origin_abs[i] : bb.reg_origin[i];

						if (src > 0x40000)
						{
							// Use the xfloat hint to create 256-bit (4x double) PHI
							llvm::Type* type = g_cfg.core.spu_accurate_xfloat && bb.reg_maybe_xf[i] ? get_type<f64[4]>() : get_reg_type(i);

							const auto _phi = m_ir->CreatePHI(type, ::size32(bb.preds), fmt::format("phi0x%05x_r%u", baddr, i));
							m_block->phi[i] = _phi;
							m_block->reg[i] = _phi;

							for (u32 pred : bb.preds)
							{
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
											value = m_finfo && m_finfo->load[i] ? m_finfo->load[i] : m_ir->CreateLoad(regptr);
										}

										if (value->getType() == get_type<f64[4]>() && type != get_type<f64[4]>())
										{
											value = double_to_xfloat(value);
										}
										else if (value->getType() != get_type<f64[4]>() && type == get_type<f64[4]>())
										{
											value = xfloat_to_double(bitcast<u32[4]>(value));
										}
										else
										{
											value = bitcast(value, _phi->getType());
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
								const auto value = m_finfo && m_finfo->load[i] ? m_finfo->load[i] : m_ir->CreateLoad(regptr);
								m_ir->SetInsertPoint(cblock);
								_phi->addIncoming(value, &m_function->getEntryBlock());
							}
						}
						else if (src < 0x40000)
						{
							// Passthrough register value
							const auto bfound = m_blocks.find(src);

							if (bfound != m_blocks.end())
							{
								m_block->reg[i] = bfound->second.reg[i];
							}
							else
							{
								spu_log.error("[0x%05x] Value not found ($%u from 0x%05x)", baddr, i, src);
							}
						}
						else
						{
							m_block->reg[i] = m_finfo->load[i];
						}
					}

					// Emit state check if necessary (TODO: more conditions)
					for (u32 pred : bb.preds)
					{
						if (pred >= baddr)
						{
							// If this block is a target of a backward branch (possibly loop), emit a check
							need_check = true;
							break;
						}
					}
				}

				// State check at the beginning of the chunk
				if (need_check || (bi == 0 && g_cfg.core.spu_block_size != spu_block_size_type::safe))
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

					const u32 op = std::bit_cast<be_t<u32>>(func.data[(m_pos - start) / 4]);

					if (!op)
					{
						spu_log.error("Unexpected fallthrough to 0x%x (chunk=0x%x, entry=0x%x)", m_pos, m_entry, m_function_queue[0]);
						break;
					}

					// Execute recompiler function (TODO)
					try
					{
						(this->*g_decoder.decode(op))({op});
					}
					catch (const std::exception&)
					{
						std::string dump;
						raw_string_ostream out(dump);
						out << *module; // print IR
						out.flush();
						spu_log.error("[0x%x] LLVM dump:\n%s", m_pos, dump);
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

							if (tfound == m_targets.end() || tfound->second.find_first_of(target) + 1 == 0)
							{
								spu_log.error("Unregistered fallthrough to 0x%x (chunk=0x%x, entry=0x%x)", target, m_entry, m_function_queue[0]);
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

			for (u32 i = start; i < end; i += 4)
			{
				const auto found = m_functions.find(i);

				if (found == m_functions.end())
				{
					if (false && g_cfg.core.spu_verification)
					{
						const std::string ppname = fmt::format("%s-chunkpp-0x%05x", m_hash, i);
						m_engine->updateGlobalMapping(ppname, reinterpret_cast<u64>(m_spurt->make_branch_patchpoint(i / 4)));

						const auto ppfunc = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(ppname, m_finfo->chunk->getFunctionType()).getCallee());
						ppfunc->setCallingConv(m_finfo->chunk->getCallingConv());

						chunks.push_back(ppfunc);
						continue;
					}

					chunks.push_back(m_dispatch);
					continue;
				}

				chunks.push_back(found->second.chunk);
			}

			m_function_table->setInitializer(llvm::ConstantArray::get(llvm::ArrayType::get(entry_chunk->chunk->getType(), m_size / 4), chunks));
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
		//pm.add(createNewGVNPass());
		pm.add(createDeadStoreEliminationPass());
		pm.add(createLICMPass());
		pm.add(createAggressiveDCEPass());
		//pm.add(createLintPass()); // Check

		for (const auto& func : m_functions)
		{
			const auto f = func.second.fn ? func.second.fn : func.second.chunk;
			pm.run(*f);

			for (auto& bb : *f)
			{
				for (auto& i : bb)
				{
					// Replace volatile fake store with spu_test_state call
					if (auto si = dyn_cast<StoreInst>(&i); si && si->getOperand(1) == m_fake_global1)
					{
						m_ir->SetInsertPoint(si);

						CallInst* ci{};
						if (si->getOperand(0) == m_ir->getFalse())
						{
							ci = m_ir->CreateCall(m_test_state, {&*f->arg_begin()});
							ci->setCallingConv(m_test_state->getCallingConv());
						}
						else
						{
							continue;
						}

						si->replaceAllUsesWith(ci);
						si->eraseFromParent();
						break;
					}
				}
			}
		}

		// Clear context (TODO)
		m_blocks.clear();
		m_block_queue.clear();
		m_functions.clear();
		m_function_queue.clear();
		m_function_table = nullptr;

		raw_string_ostream out(log);

		if (g_cfg.core.spu_debug)
		{
			fmt::append(log, "LLVM IR at 0x%x:\n", func.entry_point);
			out << *module; // print IR
			out << "\n\n";
		}

		if (verifyModule(*module, &out))
		{
			out.flush();
			spu_log.error("LLVM: Verification failed at 0x%x:\n%s", func.entry_point, log);

			if (g_cfg.core.spu_debug)
			{
				fs::file(m_spurt->get_cache_path() + "spu-ir.log", fs::write + fs::append).write(log);
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

		// Install unconditionally, possibly replacing existing one from spu_fast
		add_loc->compiled = fn;

		// Rebuild trampoline if necessary
		if (!m_spurt->rebuild_ubertrampoline(func.data[0]))
		{
			return nullptr;
		}

		add_loc->compiled.notify_all();

		if (g_cfg.core.spu_debug)
		{
			out.flush();
			fs::file(m_spurt->get_cache_path() + "spu-ir.log", fs::write + fs::append).write(log);
		}

		if (g_fxo->get<spu_cache>())
		{
			spu_log.success("New block compiled successfully");
		}

		return fn;
	}

	static void interp_check(spu_thread* _spu, bool after)
	{
		static thread_local std::array<v128, 128> s_gpr;

		if (!after)
		{
			// Preserve reg state
			s_gpr = _spu->gpr;

			// Execute interpreter instruction
			const u32 op = *reinterpret_cast<const be_t<u32>*>(_spu->_ptr<u8>(0) + _spu->pc);
			if (!g_spu_interpreter_fast.decode(op)(*_spu, {op}))
				spu_log.fatal("Bad instruction" HERE);

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
					spu_log.fatal("Register mismatch: $%u\n%s\n%s", i, _spu->gpr[i], s_gpr[i]);
					_spu->state += cpu_flag::dbg_pause;
				}
			}
		}
	}

	spu_function_t compile_interpreter()
	{
		using namespace llvm;

		m_engine->clearAllGlobalMappings();

		// Create LLVM module
		std::unique_ptr<Module> module = std::make_unique<Module>("spu_interpreter.obj", m_context);
		module->setTargetTriple(Triple::normalize("x86_64-unknown-linux-gnu"));
		module->setDataLayout(m_jit.get_engine().getTargetMachine()->createDataLayout());
		m_module = module.get();

		// Initialize IR Builder
		IRBuilder<> irb(m_context);
		m_ir = &irb;

		// Create interpreter table
		const auto if_type = get_ftype<void, u8*, u8*, u32, u32, u8*, u32, u8*>();
		const auto if_pptr = if_type->getPointerTo()->getPointerTo();
		m_function_table = new GlobalVariable(*m_module, ArrayType::get(if_type->getPointerTo(), 1ull << m_interp_magn), true, GlobalValue::InternalLinkage, nullptr);

		// Add return function
		const auto ret_func = cast<Function>(module->getOrInsertFunction("spu_ret", if_type).getCallee());
		ret_func->setCallingConv(CallingConv::GHC);
		ret_func->setLinkage(GlobalValue::InternalLinkage);
		m_ir->SetInsertPoint(BasicBlock::Create(m_context, "", ret_func));
		m_thread = &*(ret_func->arg_begin() + 1);
		m_interp_pc = &*(ret_func->arg_begin() + 2);
		m_ir->CreateRetVoid();

		// Add entry function, serves as a trampoline
		const auto main_func = llvm::cast<Function>(m_module->getOrInsertFunction("spu_interpreter", get_ftype<void, u8*, u8*, u8*>()).getCallee());
#ifdef _WIN32
		main_func->setCallingConv(CallingConv::Win64);
#endif
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

		// Save host thread's stack pointer
		const auto native_sp = spu_ptr<u64>(&spu_thread::saved_native_sp);
		const auto rsp_name = MetadataAsValue::get(m_context, MDNode::get(m_context, {MDString::get(m_context, "rsp")}));
		m_ir->CreateStore(m_ir->CreateCall(get_intrinsic<u64>(Intrinsic::read_register), {rsp_name}), native_sp);

		// Decode (shift) and load function pointer
		const auto first = m_ir->CreateLoad(m_ir->CreateGEP(m_ir->CreateBitCast(m_interp_table, if_pptr), m_ir->CreateLShr(m_interp_op, 32u - m_interp_magn)));
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
		std::array<llvm::Function*, 256> ifuncs{};
		std::vector<llvm::Constant*> iptrs;
		iptrs.reserve(1ull << m_interp_magn);

		m_block = nullptr;

		auto last_itype = spu_itype::type{255};

		for (u32 i = 0; i < 1u << m_interp_magn;)
		{
			// Fake opcode
			const u32 op = i << (32u - m_interp_magn);

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
				fmt::append(fname, "_%X", (i & (m_op_const_mask >> (32u - m_interp_magn))) | (1u << m_interp_magn));
			}

			// Decode instruction name, access function
			const auto f = cast<Function>(module->getOrInsertFunction(fname, if_type).getCallee());

			// Build if necessary
			if (f->empty())
			{
				if (last_itype != itype)
				{
					ifuncs[itype] = f;
				}

				f->setCallingConv(CallingConv::GHC);

				m_function = f;
				m_lsptr  = &*(f->arg_begin() + 0);
				m_thread = &*(f->arg_begin() + 1);
				m_interp_pc = &*(f->arg_begin() + 2);
				m_interp_op = &*(f->arg_begin() + 3);
				m_interp_table = &*(f->arg_begin() + 4);
				m_interp_7f0 = &*(f->arg_begin() + 5);
				m_interp_regs = &*(f->arg_begin() + 6);

				m_ir->SetInsertPoint(BasicBlock::Create(m_context, "", f));
				m_memptr = m_ir->CreateLoad(spu_ptr<u8*>(&spu_thread::memory_base_addr));

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
						const auto next_if = m_ir->CreateLoad(m_ir->CreateGEP(m_ir->CreateBitCast(m_interp_table, if_pptr), m_ir->CreateLShr(next_op, 32u - m_interp_magn)));
						llvm::cast<LoadInst>(next_if)->setVolatile(true);

						if (!(itype & spu_itype::branch))
						{
							if (check)
							{
								call("spu_interp_check", &interp_check, m_thread, m_ir->getFalse());
							}

							// Normal instruction.
							(this->*g_decoder.decode(op))({op});

							if (check && !m_ir->GetInsertBlock()->getTerminator())
							{
								call("spu_interp_check", &interp_check, m_thread, m_ir->getTrue());
							}

							m_interp_pc = m_interp_pc_next;
						}

						if (last_itype != itype)
						{
							// Reset to discard dead code
							llvm::cast<LoadInst>(next_if)->setVolatile(false);

							if (itype & spu_itype::branch)
							{
								const auto _stop = BasicBlock::Create(m_context, "", f);
								const auto _next = BasicBlock::Create(m_context, "", f);
								m_ir->CreateCondBr(m_ir->CreateIsNotNull(m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::state))), _stop, _next, m_md_unlikely);
								m_ir->SetInsertPoint(_stop);
								m_ir->CreateStore(m_interp_pc, spu_ptr<u32>(&spu_thread::pc));

								const auto escape_yes = BasicBlock::Create(m_context, "", f);
								const auto escape_no = BasicBlock::Create(m_context, "", f);
								m_ir->CreateCondBr(call("spu_exec_check_state", &exec_check_state, m_thread), escape_yes, escape_no);
								m_ir->SetInsertPoint(escape_yes);
								call("spu_escape", spu_runtime::g_escape, m_thread);
								m_ir->CreateBr(_next);
								m_ir->SetInsertPoint(escape_no);
								m_ir->CreateBr(_next);
								m_ir->SetInsertPoint(_next);
							}

							llvm::Value* fret = m_ir->CreateBitCast(m_interp_table, if_type->getPointerTo());

							if (itype == spu_itype::WRCH ||
								itype == spu_itype::RDCH ||
								itype == spu_itype::RCHCNT ||
								itype == spu_itype::STOP ||
								itype == spu_itype::STOPD ||
								itype == spu_itype::UNK ||
								itype == spu_itype::DFCMEQ ||
								itype == spu_itype::DFCMGT ||
								itype == spu_itype::DFCGT ||
								itype == spu_itype::DFCEQ ||
								itype == spu_itype::DFTSV)
							{
								m_interp_7f0  = m_ir->getInt32(0x7f0);
								m_interp_regs = _ptr(m_thread, get_reg_offset(0));
								fret = ret_func;
							}
							else if (!(itype & spu_itype::branch))
							{
								// Hack: inline ret instruction before final jmp; this is not reliable.
								m_ir->CreateCall(InlineAsm::get(get_ftype<void>(), "ret", "", true, false, InlineAsm::AD_Intel));
								fret = ret_func;
							}

							const auto arg3 = UndefValue::get(get_type<u32>());
							const auto _ret = m_ir->CreateCall(fret, {m_lsptr, m_thread, m_interp_pc, arg3, m_interp_table, m_interp_7f0, m_interp_regs});
							_ret->setCallingConv(CallingConv::GHC);
							_ret->setTailCall();
							m_ir->CreateRetVoid();
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
				catch (const std::exception&)
				{
					std::string dump;
					raw_string_ostream out(dump);
					out << *module; // print IR
					out.flush();
					spu_log.error("[0x%x] LLVM dump:\n%s", m_pos, dump);
					throw;
				}
			}

			if (last_itype != itype && g_cfg.core.spu_decoder != spu_decoder_type::llvm)
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

		m_function_table->setInitializer(ConstantArray::get(ArrayType::get(if_type->getPointerTo(), 1ull << m_interp_magn), iptrs));
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
			spu_log.error("LLVM: Verification failed:\n%s", log);

			if (g_cfg.core.spu_debug)
			{
				fs::file(m_spurt->get_cache_path() + "spu-ir.log", fs::write + fs::append).write(log);
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

		for (u32 i = 0; i < spu_runtime::g_interpreter_table.size(); i++)
		{
			// Fill exported interpreter table
			spu_runtime::g_interpreter_table[i] = ifuncs[i] ? reinterpret_cast<u64>(m_jit.get_engine().getPointerToFunction(ifuncs[i])) : 0;
		}

		if (!spu_runtime::g_interpreter)
		{
			return nullptr;
		}

		if (g_cfg.core.spu_debug)
		{
			out.flush();
			fs::file(m_spurt->get_cache_path() + "spu-ir.log", fs::write + fs::append).write(log);
		}

		return spu_runtime::g_interpreter;
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
		std::string name = fmt::format("spu_%s", s_spu_iname.decode(op.opcode));

		if (m_interp_magn)
		{
			call(name, F, m_thread, m_interp_op);
			return;
		}

		update_pc();
		call(name, &exec_fall<F>, m_thread, m_ir->getInt32(op.opcode));
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
			call("spu_unknown", &exec_unk, m_thread, m_ir->getInt32(op_unk.opcode));
			return;
		}

		m_block->block_end = m_ir->GetInsertBlock();
		update_pc();
		call("spu_unknown", &exec_unk, m_thread, m_ir->getInt32(op_unk.opcode));
	}

	static void exec_stop(spu_thread* _spu, u32 code)
	{
		if (!_spu->stop_and_signal(code))
		{
			spu_runtime::g_escape(_spu);
		}

		if (_spu->test_stopped())
		{
			_spu->pc += 4;
			spu_runtime::g_escape(_spu);
		}
	}

	void STOP(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			call("spu_syscall", &exec_stop, m_thread, m_ir->CreateAnd(m_interp_op, m_ir->getInt32(0x3fff)));
			return;
		}

		update_pc();
		call("spu_syscall", &exec_stop, m_thread, m_ir->getInt32(op.opcode & 0x3fff));

		if (g_cfg.core.spu_block_size == spu_block_size_type::safe)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			update_pc(m_pos + 4);
			tail_chunk(m_dispatch);
			return;
		}
	}

	void STOPD(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			call("spu_syscall", &exec_stop, m_thread, m_ir->getInt32(0x3fff));
			return;
		}

		STOP(spu_opcode_t{0x3fff});
	}

	static u32 exec_rdch(spu_thread* _spu, u32 ch)
	{
		const s64 result = _spu->get_ch_value(ch);

		if (result < 0)
		{
			spu_runtime::g_escape(_spu);
		}

		if (_spu->test_stopped())
		{
			_spu->pc += 4;
			spu_runtime::g_escape(_spu);
		}

		return static_cast<u32>(result & 0xffffffff);
	}

	static u32 exec_read_in_mbox(spu_thread* _spu)
	{
		// TODO
		return exec_rdch(_spu, SPU_RdInMbox);
	}

	static u32 exec_read_dec(spu_thread* _spu)
	{
		const u32 res = _spu->ch_dec_value - static_cast<u32>(get_timebased_time() - _spu->ch_dec_start_timestamp);

		if (res > 1500 && g_cfg.core.spu_loop_detection)
		{
			_spu->state += cpu_flag::wait;
			std::this_thread::yield();

			if (_spu->test_stopped())
			{
				_spu->pc += 4;
				spu_runtime::g_escape(_spu);
			}
		}

		return res;
	}

	static u32 exec_read_events(spu_thread* _spu)
	{
		if (const u32 events = _spu->get_events())
		{
			return events;
		}

		// TODO
		return exec_rdch(_spu, SPU_RdEventStat);
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
			const auto val = m_ir->CreateLoad(ptr, true);
			m_ir->CreateStore(m_ir->getInt64(0), ptr, true);
			val0 = val;
		}

		const auto _cur = m_ir->GetInsertBlock();
		const auto done = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto wait = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto cond = m_ir->CreateICmpSLT(val0, m_ir->getInt64(0));
		val0 = m_ir->CreateTrunc(val0, get_type<u32>());
		m_ir->CreateCondBr(cond, done, wait);
		m_ir->SetInsertPoint(wait);
		const auto val1 = call("spu_read_channel", &exec_rdch, m_thread, m_ir->getInt32(op.ra));
		m_ir->CreateBr(done);
		m_ir->SetInsertPoint(done);
		const auto rval = m_ir->CreatePHI(get_type<u32>(), 2);
		rval->addIncoming(val0, _cur);
		rval->addIncoming(val1, wait);
		return rval;
	}

	void RDCH(spu_opcode_t op) //
	{
		value_t<u32> res;

		if (m_interp_magn)
		{
			res.value = call("spu_read_channel", &exec_rdch, m_thread, get_imm<u32>(op.ra).value);
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
			res.value = call("spu_read_in_mbox", &exec_read_in_mbox, m_thread);
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
			res.value = call("spu_read_decrementer", &exec_read_dec, m_thread);
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
			res.value = call("spu_read_events", &exec_read_events, m_thread);
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
			res.value = call("spu_read_channel", &exec_rdch, m_thread, m_ir->getInt32(op.ra));
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
			res.value = call("spu_read_channel_count", &exec_rchcnt, m_thread, get_imm<u32>(op.ra).value);
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
			res.value = call("spu_get_events", &exec_get_events, m_thread);
			res.value = m_ir->CreateICmpNE(res.value, m_ir->getInt32(0));
			res.value = m_ir->CreateZExt(res.value, get_type<u32>());
			break;
		}

		default:
		{
			res.value = call("spu_read_channel_count", &exec_rchcnt, m_thread, m_ir->getInt32(op.ra));
			break;
		}
		}

		set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
	}

	static void exec_wrch(spu_thread* _spu, u32 ch, u32 value)
	{
		if (!_spu->set_ch_value(ch, value))
		{
			spu_runtime::g_escape(_spu);
		}

		if (_spu->test_stopped())
		{
			_spu->pc += 4;
			spu_runtime::g_escape(_spu);
		}
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

		_spu->do_mfc();
	}

	static void exec_mfc_cmd(spu_thread* _spu)
	{
		if (!_spu->process_mfc_cmd())
		{
			spu_runtime::g_escape(_spu);
		}

		if (_spu->test_stopped())
		{
			_spu->pc += 4;
			spu_runtime::g_escape(_spu);
		}
	}

	void WRCH(spu_opcode_t op) //
	{
		const auto val = eval(extract(get_vr(op.rt), 3));

		if (m_interp_magn)
		{
			call("spu_write_channel", &exec_wrch, m_thread, get_imm<u32>(op.ra).value, val.value);
			return;
		}

		switch (op.ra)
		{
		case SPU_WrSRR0:
		{
			m_ir->CreateStore(eval(val & 0x3fffc).value, spu_ptr<u32>(&spu_thread::srr0));
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

			spu_log.warning("[0x%x] MFC_WrTagUpdate: $%u is not a good constant", m_pos, +op.rt);
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

			spu_log.warning("[0x%x] MFC_EAH: $%u is not a zero constant", m_pos, +op.rt);
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
			set_reg_fixed(s_reg_mfc_size, trunc<u16>(val & 0x7fff).eval(m_ir));
			return;
		}
		case MFC_TagID:
		{
			set_reg_fixed(s_reg_mfc_tag, trunc<u8>(val & 0x1f).eval(m_ir));
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

			if (auto ci = llvm::dyn_cast<llvm::ConstantInt>(trunc<u8>(val).eval(m_ir)))
			{
				const auto eal = get_reg_fixed<u32>(s_reg_mfc_eal);
				const auto lsa = get_reg_fixed<u32>(s_reg_mfc_lsa);
				const auto tag = get_reg_fixed<u8>(s_reg_mfc_tag);

				const auto size = get_reg_fixed<u16>(s_reg_mfc_size);
				const auto mask = m_ir->CreateShl(m_ir->getInt32(1), zext<u32>(tag).eval(m_ir));
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
					update_pc();
					call("spu_exec_mfc_cmd", &exec_mfc_cmd, m_thread);
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

					llvm::Value* src = m_ir->CreateGEP(m_lsptr, zext<u64>(lsa).eval(m_ir));
					llvm::Value* dst = m_ir->CreateGEP(m_memptr, zext<u64>(eal).eval(m_ir));

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
					call("spu_exec_mfc_cmd", &exec_mfc_cmd, m_thread);
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
							spu_log.error("[0x%x] MFC_Cmd: invalid size %u", m_pos, csize);
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
						auto spu_memcpy = [](u8* dst, const u8* src, u32 size)
						{
							std::memcpy(dst, src, size);
						};
						call("spu_memcpy", +spu_memcpy, dst, src, zext<u32>(size).eval(m_ir));
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
					spu_log.error("[0x%x] MFC_Cmd: unknown command (0x%x)", m_pos, cmd);
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
					m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(pf), mask), pf);
					if (cmd & MFC_BARRIER_MASK)
						m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(pb), mask), pb);
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
			spu_log.warning("[0x%x] MFC_Cmd: $%u is not a constant", m_pos, +op.rt);
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
			call("spu_list_unstall", &exec_list_unstall, m_thread, eval(val & 0x1f).value);
			m_ir->CreateBr(next);
			m_ir->SetInsertPoint(next);
			return;
		}
		case SPU_WrDec:
		{
			m_ir->CreateStore(call("get_timebased_time", &get_timebased_time), spu_ptr<u64>(&spu_thread::ch_dec_start_timestamp));
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
		call("spu_write_channel", &exec_wrch, m_thread, m_ir->getInt32(op.ra), val.value);
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
			update_pc(m_pos + 4);
			tail_chunk(m_dispatch);
		}
	}

	void DSYNC(spu_opcode_t op) //
	{
		// This instruction forces all earlier load, store, and channel instructions to complete before proceeding.
		m_ir->CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);
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

	template <typename TA, typename TB>
	static auto mpyh(TA&& a, TB&& b)
	{
		return (std::forward<TA>(a) >> 16) * (std::forward<TB>(b) << 16);
	}

	template <typename TA, typename TB>
	static auto mpyu(TA&& a, TB&& b)
	{
		return (std::forward<TA>(a) << 16 >> 16) * (std::forward<TB>(b) << 16 >> 16);
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
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);
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
		const auto [a, b] = get_vrs<u8[16]>(op.ra, op.rb);
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

		set_vr(op.rt, select(sh < by.esize, get_vr<R>(op.ra) >> sh, splat<R>(0)));
	}

	template <typename R, typename T>
	void make_spu_rotate_sext(spu_opcode_t op, value_t<T> by)
	{
		value_t<R> sh;
		static_assert(sh.esize == by.esize);
		sh.value = m_ir->CreateAnd(m_ir->CreateNeg(by.value), by.esize * 2 - 1);
		if constexpr (!by.is_vector)
			sh.value = m_ir->CreateVectorSplat(sh.is_vector, sh.value);

		value_t<R> max_sh = eval(splat<R>(by.esize - 1));
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

		set_vr(op.rt, select(sh < by.esize, get_vr<R>(op.ra) << sh, splat<R>(0)));
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
		if (auto [a, b] = match_vrs<u32[4]>(op.ra, op.rb); a && b)
		{
			static const auto MP = match<u32[4]>();

			if (auto [ok, a0, b0, b1, a1] = match_expr(a, mpyh(MP, MP) + mpyh(MP, MP)); ok)
			{
				if (auto [ok, a2, b2] = match_expr(b, mpyu(MP, MP)); ok && a2.eq(a0, a1) && b2.eq(b0, b1))
				{
					// 32-bit multiplication
					spu_log.notice("mpy32 in %s at 0x%05x", m_hash, m_pos);
					set_vr(op.rt, a0 * b0);
					return;
				}
			}
		}

		set_vr(op.rt, get_vr(op.ra) + get_vr(op.rb));
	}

	void AND(spu_opcode_t op)
	{
		if (match_vr<u8[16], u16[8], u64[2]>(op.ra, [&](auto a, auto MP1)
		{
			if (auto b = match_vr_as(a, op.rb))
			{
				set_vr(op.rt, a & b);
				return true;
			}

			return match_vr<u8[16], u16[8], u64[2]>(op.rb, [&](auto b, auto MP2)
			{
				set_vr(op.rt, a & get_vr_as(a, op.rb));
				return true;
			});
		}))
		{
			return;
		}

		set_vr(op.rt, get_vr(op.ra) & get_vr(op.rb));
	}

	void CG(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);
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
		const auto m = zext<u32>(bitcast<i4>(trunc<bool[4]>(a)));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, eval(m)));
	}

	void GBH(spu_opcode_t op)
	{
		const auto a = get_vr<s16[8]>(op.ra);
		const auto m = zext<u32>(bitcast<u8>(trunc<bool[8]>(a)));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, eval(m)));
	}

	void GBB(spu_opcode_t op)
	{
		const auto a = get_vr<s8[16]>(op.ra);
		const auto m = zext<u32>(bitcast<u16>(trunc<bool[16]>(a)));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, eval(m)));
	}

	void FSM(spu_opcode_t op)
	{
		const auto v = extract(get_vr(op.ra), 3);
		const auto m = bitcast<bool[4]>(trunc<i4>(v));
		set_vr(op.rt, sext<s32[4]>(m));
	}

	void FSMH(spu_opcode_t op)
	{
		const auto v = extract(get_vr(op.ra), 3);
		const auto m = bitcast<bool[8]>(trunc<u8>(v));
		set_vr(op.rt, sext<s16[8]>(m));
	}

	void FSMB(spu_opcode_t op)
	{
		const auto v = extract(get_vr(op.ra), 3);
		const auto m = bitcast<bool[16]>(trunc<u16>(v));
		set_vr(op.rt, sext<s8[16]>(m));
	}

	void ROTQBYBI(spu_opcode_t op)
	{
		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = (sc - (zshuffle(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) >> 3)) & 0xf;
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void ROTQMBYBI(spu_opcode_t op)
	{
		const auto sc = build<u8[16]>(112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127);
		const auto sh = sc + (-(zshuffle(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) >> 3) & 0x1f);
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void SHLQBYBI(spu_opcode_t op)
	{
		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = sc - (zshuffle(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) >> 3);
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	template <typename RT, typename T>
	auto spu_get_insertion_shuffle_mask(T&& index)
	{
		const auto c = bitcast<RT>(build<u8[16]>(0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10));
		using e_type = std::remove_extent_t<RT>;
		const auto v = splat<e_type>(static_cast<e_type>(sizeof(e_type) == 8 ? 0x01020304050607ull : 0x010203ull));
		return insert(c, std::forward<T>(index), v);
	}

	void CBX(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// Optimization with aligned stack assumption. Strange because SPU code could use CBD instead, but encountered in wild.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u8[16]>(~get_scalar(get_vr(op.rb)) & 0xf));
			return;
		}

		const auto s = get_scalar(get_vr(op.ra)) + get_scalar(get_vr(op.rb));
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u8[16]>(~s & 0xf));
	}

	void CHX(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBX.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u16[8]>(~get_scalar(get_vr(op.rb)) >> 1 & 0x7));
			return;
		}

		const auto s = get_scalar(get_vr(op.ra)) + get_scalar(get_vr(op.rb));
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u16[8]>(~s >> 1 & 0x7));
	}

	void CWX(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBX.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u32[4]>(~get_scalar(get_vr(op.rb)) >> 2 & 0x3));
			return;
		}

		const auto s = get_scalar(get_vr(op.ra)) + get_scalar(get_vr(op.rb));
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u32[4]>(~s >> 2 & 0x3));
	}

	void CDX(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBX.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u64[2]>(~get_scalar(get_vr(op.rb)) >> 3 & 0x1));
			return;
		}

		const auto s = get_scalar(get_vr(op.ra)) + get_scalar(get_vr(op.rb));
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u64[2]>(~s >> 3 & 0x1));
	}

	void ROTQBI(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = zshuffle(get_vr(op.rb) & 0x7, 3, 3, 3, 3);
		set_vr(op.rt, fshl(a, zshuffle(a, 3, 0, 1, 2), b));
	}

	void ROTQMBI(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = zshuffle(-get_vr(op.rb) & 0x7, 3, 3, 3, 3);
		set_vr(op.rt, fshr(zshuffle(a, 1, 2, 3, 4), a, b));
	}

	void SHLQBI(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = zshuffle(get_vr(op.rb) & 0x7, 3, 3, 3, 3);
		set_vr(op.rt, fshl(a, zshuffle(a, 4, 0, 1, 2), b));
	}

	void ROTQBY(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);
		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = eval((sc - zshuffle(b, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12)) & 0xf);
		set_vr(op.rt, pshufb(a, sh));
	}

	void ROTQMBY(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);
		const auto sc = build<u8[16]>(112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127);
		const auto sh = sc + (-zshuffle(b, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) & 0x1f);
		set_vr(op.rt, pshufb(a, sh));
	}

	void SHLQBY(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);
		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = sc - (zshuffle(b, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) & 0x1f);
		set_vr(op.rt, pshufb(a, sh));
	}

	void ORX(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto x = zshuffle(a, 2, 3, 0, 1) | a;
		const auto y = zshuffle(x, 1, 0, 3, 2) | x;
		set_vr(op.rt, zshuffle(y, 4, 4, 4, 3));
	}

	void CBD(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// Known constant with aligned stack assumption (optimization).
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u8[16]>(~get_imm<u32>(op.i7) & 0xf));
			return;
		}

		const auto a = get_scalar(get_vr(op.ra)) + get_imm<u32>(op.i7);
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u8[16]>(~a & 0xf));
	}

	void CHD(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBD.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u16[8]>(~get_imm<u32>(op.i7) >> 1 & 0x7));
			return;
		}

		const auto a = get_scalar(get_vr(op.ra)) + get_imm<u32>(op.i7);
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u16[8]>(~a >> 1 & 0x7));
	}

	void CWD(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBD.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u32[4]>(~get_imm<u32>(op.i7) >> 2 & 0x3));
			return;
		}

		const auto a = get_scalar(get_vr(op.ra)) + get_imm<u32>(op.i7);
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u32[4]>(~a >> 2 & 0x3));
	}

	void CDD(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBD.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u64[2]>(~get_imm<u32>(op.i7) >> 3 & 0x1));
			return;
		}

		const auto a = get_scalar(get_vr(op.ra)) + get_imm<u32>(op.i7);
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u64[2]>(~a >> 3 & 0x1));
	}

	void ROTQBII(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = eval(get_imm(op.i7, false) & 0x7);
		set_vr(op.rt, fshl(a, zshuffle(a, 3, 0, 1, 2), b));
	}

	void ROTQMBII(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = eval(-get_imm(op.i7, false) & 0x7);
		set_vr(op.rt, fshr(zshuffle(a, 1, 2, 3, 4), a, b));
	}

	void SHLQBII(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = eval(get_imm(op.i7, false) & 0x7);
		set_vr(op.rt, fshl(a, zshuffle(a, 4, 0, 1, 2), b));
	}

	void ROTQBYI(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = (sc - get_imm<u8[16]>(op.i7, false)) & 0xf;
		set_vr(op.rt, pshufb(a, sh));
	}

	void ROTQMBYI(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto sc = build<u8[16]>(112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127);
		const auto sh = sc + (-get_imm<u8[16]>(op.i7, false) & 0x1f);
		set_vr(op.rt, pshufb(a, sh));
	}

	void SHLQBYI(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = sc - (get_imm<u8[16]>(op.i7, false) & 0x1f);
		set_vr(op.rt, pshufb(a, sh));
	}

	void CGT(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s32[4]>(get_vr<s32[4]>(op.ra) > get_vr<s32[4]>(op.rb)));
	}

	void XOR(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) ^ get_vr(op.rb));
	}

	void CGTH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s16[8]>(get_vr<s16[8]>(op.ra) > get_vr<s16[8]>(op.rb)));
	}

	void EQV(spu_opcode_t op)
	{
		set_vr(op.rt, ~(get_vr(op.ra) ^ get_vr(op.rb)));
	}

	void CGTB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<s8[16]>(op.ra) > get_vr<s8[16]>(op.rb)));
	}

	void SUMB(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u16[8]>(op.ra, op.rb);
		const auto ahs = eval((a >> 8) + (a & 0xff));
		const auto bhs = eval((b >> 8) + (b & 0xff));
		const auto lsh = shuffle2(ahs, bhs, 0, 9, 2, 11, 4, 13, 6, 15);
		const auto hsh = shuffle2(ahs, bhs, 1, 8, 3, 10, 5, 12, 7, 14);
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
		set_vr(op.rt, sext<s32[4]>(get_vr(op.ra) > get_vr(op.rb)));
	}

	void ANDC(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) & ~get_vr(op.rb));
	}

	void CLGTH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s16[8]>(get_vr<u16[8]>(op.ra) > get_vr<u16[8]>(op.rb)));
	}

	void ORC(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) | ~get_vr(op.rb));
	}

	void CLGTB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<u8[16]>(op.ra) > get_vr<u8[16]>(op.rb)));
	}

	void CEQ(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s32[4]>(get_vr(op.ra) == get_vr(op.rb)));
	}

	void MPYHHU(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) >> 16) * (get_vr(op.rb) >> 16));
	}

	void ADDX(spu_opcode_t op)
	{
		set_vr(op.rt, llvm_sum{get_vr(op.ra), get_vr(op.rb), get_vr(op.rt) & 1});
	}

	void SFX(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.rb) - get_vr(op.ra) - (~get_vr(op.rt) & 1));
	}

	void CGX(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);
		const auto x = (get_vr<s32[4]>(op.rt) << 31) >> 31;
		const auto s = eval(a + b);
		set_vr(op.rt, noncast<u32[4]>(sext<s32[4]>(s < a) | (sext<s32[4]>(s == noncast<u32[4]>(x)) & x)) >> 31);
	}

	void BGX(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);
		const auto c = get_vr<s32[4]>(op.rt) << 31;
		set_vr(op.rt, noncast<u32[4]>(sext<s32[4]>(b > a) | (sext<s32[4]>(a == b) & c)) >> 31);
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
		set_vr(op.rt, mpyh(get_vr(op.ra), get_vr(op.rb)));
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
		set_vr(op.rt, sext<s16[8]>(get_vr<u16[8]>(op.ra) == get_vr<u16[8]>(op.rb)));
	}

	void MPYU(spu_opcode_t op)
	{
		set_vr(op.rt, mpyu(get_vr(op.ra), get_vr(op.rb)));
	}

	void CEQB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<u8[16]>(op.ra) == get_vr<u8[16]>(op.rb)));
	}

	void FSMBI(spu_opcode_t op)
	{
		const auto m = bitcast<bool[16]>(get_imm<u16>(op.i16));
		set_vr(op.rt, sext<s8[16]>(m));
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
		set_vr(op.rt, sext<s32[4]>(get_vr<s32[4]>(op.ra) > get_imm<s32[4]>(op.si10)));
	}

	void CGTHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s16[8]>(get_vr<s16[8]>(op.ra) > get_imm<s16[8]>(op.si10)));
	}

	void CGTBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<s8[16]>(op.ra) > get_imm<s8[16]>(op.si10)));
	}

	void CLGTI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s32[4]>(get_vr(op.ra) > get_imm(op.si10)));
	}

	void CLGTHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s16[8]>(get_vr<u16[8]>(op.ra) > get_imm<u16[8]>(op.si10)));
	}

	void CLGTBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<u8[16]>(op.ra) > get_imm<u8[16]>(op.si10)));
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
		set_vr(op.rt, sext<s32[4]>(get_vr(op.ra) == get_imm(op.si10)));
	}

	void CEQHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s16[8]>(get_vr<u16[8]>(op.ra) == get_imm<u16[8]>(op.si10)));
	}

	void CEQBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<u8[16]>(op.ra) == get_imm<u8[16]>(op.si10)));
	}

	void ILA(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm(op.i18));
	}

	void SELB(spu_opcode_t op)
	{
		if (match_vr<s8[16], s16[8], s32[4], s64[2]>(op.rc, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			// If the control mask comes from a comparison instruction, replace SELB with select
			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
				if constexpr (std::extent_v<VT> == 2) // u64[2]
				{
					// Try to select floats as floats if a OR b is typed as f64[2]
					if (auto [a, b] = match_vrs<f64[2]>(op.ra, op.rb); a || b)
					{
						set_vr(op.rt4, select(x, get_vr<f64[2]>(op.rb), get_vr<f64[2]>(op.ra)));
						return true;
					}
				}

				if constexpr (std::extent_v<VT> == 4) // u32[4]
				{
					if (auto [a, b] = match_vrs<f64[4]>(op.ra, op.rb); a || b)
					{
						set_vr(op.rt4, select(x, get_vr<f64[4]>(op.rb), get_vr<f64[4]>(op.ra)));
						return true;
					}

					if (auto [a, b] = match_vrs<f32[4]>(op.ra, op.rb); a || b)
					{
						set_vr(op.rt4, select(x, get_vr<f32[4]>(op.rb), get_vr<f32[4]>(op.ra)));
						return true;
					}
				}

				set_vr(op.rt4, select(x, get_vr<VT>(op.rb), get_vr<VT>(op.ra)));
				return true;
			}

			return false;
		}))
		{
			return;
		}

		const auto op1 = get_reg_raw(op.rb);
		const auto op2 = get_reg_raw(op.ra);

		if ((op1 && op1->getType() == get_type<f64[4]>()) || (op2 && op2->getType() == get_type<f64[4]>()))
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

		const auto c = get_vr(op.rc);
		set_vr(op.rt4, (get_vr(op.rb) & c) | (get_vr(op.ra) & ~c));
	}

	void SHUFB(spu_opcode_t op) //
	{
		if (match_vr<u8[16], u16[8], u32[4], u64[2]>(op.rc, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			// If the mask comes from a constant generation instruction, replace SHUFB with insert
			if (auto [ok, i] = match_expr(c, spu_get_insertion_shuffle_mask<VT>(match<u32>())); ok)
			{
				set_vr(op.rt4, insert(get_vr<VT>(op.rb), i, get_scalar(get_vr<VT>(op.ra))));
				return true;
			}

			return false;
		}))
		{
			return;
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

			spu_log.todo("[0x%x] Const SHUFB mask: %s", m_pos, mask);
		}

		const auto x = avg(noncast<u8[16]>(sext<s8[16]>((c & 0xc0) == 0xc0)), noncast<u8[16]>(sext<s8[16]>((c & 0xe0) == 0xc0)));
		const auto cr = eval(c ^ 0xf);
		const auto a = pshufb(get_vr<u8[16]>(op.ra), cr);
		const auto b = pshufb(get_vr<u8[16]>(op.rb), cr);
		set_vr(op.rt4, select(noncast<s8[16]>(cr << 3) >= 0, a, b) | x);
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

	// clamping helpers
	value_t<f32[4]> clamp_positive_smax(value_t<f32[4]> v)
	{
		return eval(bitcast<f32[4]>(min(bitcast<s32[4]>(v),splat<s32[4]>(0x7f7fffff))));
	}

	value_t<f32[4]> clamp_negative_smax(value_t<f32[4]> v)
	{
		return eval(bitcast<f32[4]>(min(bitcast<u32[4]>(v),splat<u32[4]>(0xff7fffff))));
	}

	value_t<f32[4]> clamp_smax(value_t<f32[4]> v)
	{
		return eval(clamp_negative_smax(clamp_positive_smax(v)));
	}

	// FMA favouring zeros
	value_t<f32[4]> xmuladd(value_t<f32[4]> a, value_t<f32[4]> b, value_t<f32[4]> c)
	{
		const auto ma = eval(sext<s32[4]>(fcmp_uno(a != fsplat<f32[4]>(0.))));
		const auto mb = eval(sext<s32[4]>(fcmp_uno(b != fsplat<f32[4]>(0.))));
		const auto ca = eval(bitcast<f32[4]>(bitcast<s32[4]>(a) & mb));
		const auto cb = eval(bitcast<f32[4]>(bitcast<s32[4]>(b) & ma));
		return eval(fmuladd(ca, cb, c));
	}

	void FREST(spu_opcode_t op)
	{
		// TODO
		if (g_cfg.core.spu_accurate_xfloat)
		{
			const auto a = get_vr<f32[4]>(op.ra);
			const auto mask_ov = sext<s32[4]>(bitcast<s32[4]>(fabs(a)) > splat<s32[4]>(0x7e7fffff));
			const auto mask_de = eval(noncast<u32[4]>(sext<s32[4]>(fcmp_ord(a == fsplat<f32[4]>(0.)))) >> 1);
			set_vr(op.rt, (bitcast<s32[4]>(fre(a)) & ~mask_ov) | noncast<s32[4]>(mask_de));
		}
		else
		{
			set_vr(op.rt, fre(get_vr<f32[4]>(op.ra)));
		}
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
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(get_vr<f64[4]>(op.ra) > get_vr<f64[4]>(op.rb))));
			return;
		}

		const auto a = get_vr<f32[4]>(op.ra);
		const auto b = get_vr<f32[4]>(op.rb);

		if (g_cfg.core.spu_approx_xfloat)
		{
			const auto ca = eval(clamp_positive_smax(a));
			const auto cb = eval(clamp_negative_smax(b));
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(ca > cb)));
		}
		else
		{
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(a > b)));
		}
	}

	void FCMGT(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(fabs(get_vr<f64[4]>(op.ra)) > fabs(get_vr<f64[4]>(op.rb)))));
			return;
		}

		const auto a = eval(fabs(get_vr<f32[4]>(op.ra)));
		const auto b = eval(fabs(get_vr<f32[4]>(op.rb)));

		if (g_cfg.core.spu_approx_xfloat)
		{
			set_vr(op.rt, sext<s32[4]>(fcmp_uno(a > b) & (bitcast<s32[4]>(a) > bitcast<s32[4]>(b))));
		}
		else
		{
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(a > b)));
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
		else if (g_cfg.core.spu_approx_xfloat)
		{
			const auto b = eval(clamp_smax(get_vr<f32[4]>(op.rb))); // for #4478
			set_vr(op.rt, get_vr<f32[4]>(op.ra) - b);
		}
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
			const auto ma = eval(sext<s32[4]>(fcmp_uno(a != fsplat<f32[4]>(0.))));
			const auto mb = eval(sext<s32[4]>(fcmp_uno(b != fsplat<f32[4]>(0.))));
			const auto ca = eval(bitcast<f32[4]>(bitcast<s32[4]>(a) & mb));
			const auto cb = eval(bitcast<f32[4]>(bitcast<s32[4]>(b) & ma));
			set_vr(op.rt, ca * cb);
		}
		else
			set_vr(op.rt, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb));
	}

	void FESD(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			const auto r = shuffle2(get_vr<f64[4]>(op.ra), fsplat<f64[4]>(0.), 1, 3);
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
			r.value = m_ir->CreateFPExt(shuffle2(get_vr<f32[4]>(op.ra), fsplat<f32[4]>(0.), 1, 3).eval(m_ir), get_type<f64[2]>());
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
			set_vr(op.rt, shuffle2(bitcast<f64[2]>(z), fsplat<f64[2]>(0.), 2, 0, 3, 1), false);
		}
		else
		{
			value_t<f32[2]> r;
			r.value = m_ir->CreateFPTrunc(get_vr<f64[2]>(op.ra).value, get_type<f32[2]>());
			set_vr(op.rt, shuffle2(r, fsplat<f32[2]>(0.), 2, 0, 3, 1));
		}
	}

	void FCEQ(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(get_vr<f64[4]>(op.ra) == get_vr<f64[4]>(op.rb))));
		else
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(get_vr<f32[4]>(op.ra) == get_vr<f32[4]>(op.rb))));
	}

	void FCMEQ(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(fabs(get_vr<f64[4]>(op.ra)) == fabs(get_vr<f64[4]>(op.rb)))));
		else
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(fabs(get_vr<f32[4]>(op.ra)) == fabs(get_vr<f32[4]>(op.rb)))));
	}

	value_t<f32[4]> fma32x4(value_t<f32[4]> a, value_t<f32[4]> b, value_t<f32[4]> c)
	{
		// Compare absolute values with max positive float in normal range.
		const auto aa = bitcast<s32[4]>(fabs(a));
		const auto ab = bitcast<s32[4]>(fabs(b));
		const auto sc = eval(max(aa, ab) > 0x7f7fffff);

		if (m_use_fma)
		{
			value_t<f32[4]> r;
			r.value = m_ir->CreateCall(get_intrinsic<f32[4]>(llvm::Intrinsic::fma), {a.value, b.value, c.value});
			r.value = m_ir->CreateSelect(sc.value, c.value, r.value);
			return r;
		}

		// Convert to doubles
		const auto xa = m_ir->CreateFPExt(a.value, get_type<f64[4]>());
		const auto xb = m_ir->CreateFPExt(b.value, get_type<f64[4]>());
		const auto xc = m_ir->CreateFPExt(c.value, get_type<f64[4]>());
		const auto xr = m_ir->CreateCall(get_intrinsic<f64[4]>(llvm::Intrinsic::fmuladd), {xa, xb, xc});
		value_t<f32[4]> r;
		r.value = m_ir->CreateFPTrunc(xr, get_type<f32[4]>());
		r.value = m_ir->CreateSelect(sc.value, c.value, r.value);
		return r;
	}

	void FNMS(spu_opcode_t op)
	{
		// See FMA.
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt4, -fmuladd(get_vr<f64[4]>(op.ra), get_vr<f64[4]>(op.rb), eval(-get_vr<f64[4]>(op.rc))));
		else if (g_cfg.core.spu_approx_xfloat)
			set_vr(op.rt4, -fma32x4(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb), eval(-get_vr<f32[4]>(op.rc))));
		else
			set_vr(op.rt4, get_vr<f32[4]>(op.rc) - get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb));
	}

	void FMA(spu_opcode_t op)
	{
		// Hardware FMA produces the same result as multiple + add on the limited double range (xfloat).
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt4, fmuladd(get_vr<f64[4]>(op.ra), get_vr<f64[4]>(op.rb), get_vr<f64[4]>(op.rc)));
		else if (g_cfg.core.spu_approx_xfloat)
			set_vr(op.rt4, fma32x4(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb), get_vr<f32[4]>(op.rc)));
		else
			set_vr(op.rt4, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb) + get_vr<f32[4]>(op.rc));
	}

	void FMS(spu_opcode_t op)
	{
		// See FMA.
		if (g_cfg.core.spu_accurate_xfloat)
			set_vr(op.rt4, fmuladd(get_vr<f64[4]>(op.ra), get_vr<f64[4]>(op.rb), eval(-get_vr<f64[4]>(op.rc))));
		else if (g_cfg.core.spu_approx_xfloat)
			set_vr(op.rt4, fma32x4(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb), eval(-get_vr<f32[4]>(op.rc))));
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
				s = eval(vsplat<f64[4]>(bitcast<f64>(((1023 + 173) - get_imm<u64>(op.i8)) << 52)));
			else
				s = eval(fsplat<f64[4]>(std::exp2(static_cast<int>(173 - op.i8))));
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
			set_vr(op.rt, r ^ sext<s32[4]>(fcmp_ord(a >= fsplat<f64[4]>(std::exp2(31.f)))));
		}
		else
		{
			value_t<f32[4]> a = get_vr<f32[4]>(op.ra);
			value_t<f32[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f32[4]>(load_const<f32>(m_scale_float_to, get_imm<u8>(op.i8))));
			else
				s = eval(fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))));
			if (op.i8 != 173 || m_interp_magn)
				a = eval(a * s);

			value_t<s32[4]> r;
			r.value = m_ir->CreateFPToSI(a.value, get_type<s32[4]>());
			set_vr(op.rt, r ^ sext<s32[4]>(bitcast<s32[4]>(a) > splat<s32[4]>(((31 + 127) << 23) - 1)));
		}
	}

	void CFLTU(spu_opcode_t op)
	{
		if (g_cfg.core.spu_accurate_xfloat)
		{
			value_t<f64[4]> a = get_vr<f64[4]>(op.ra);
			value_t<f64[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f64[4]>(bitcast<f64>(((1023 + 173) - get_imm<u64>(op.i8)) << 52)));
			else
				s = eval(fsplat<f64[4]>(std::exp2(static_cast<int>(173 - op.i8))));
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
			set_vr(op.rt, select(fcmp_ord(a >= fsplat<f64[4]>(std::exp2(32.f))), splat<s32[4]>(-1), r & sext<s32[4]>(fcmp_ord(a >= fsplat<f64[4]>(0.)))));
		}
		else
		{
			value_t<f32[4]> a = get_vr<f32[4]>(op.ra);
			value_t<f32[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f32[4]>(load_const<f32>(m_scale_float_to, get_imm<u8>(op.i8))));
			else
				s = eval(fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))));
			if (op.i8 != 173 || m_interp_magn)
				a = eval(a * s);

			value_t<s32[4]> r;
			r.value = m_ir->CreateFPToUI(a.value, get_type<s32[4]>());
			set_vr(op.rt, select(bitcast<s32[4]>(a) > splat<s32[4]>(((32 + 127) << 23) - 1), splat<s32[4]>(-1), r & ~(bitcast<s32[4]>(a) >> 31)));
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
				r.value = build<f64[4]>(data._s32[0], data._s32[1], data._s32[2], data._s32[3]).eval(m_ir);
			}
			else
			{
				r.value = m_ir->CreateSIToFP(a.value, get_type<f64[4]>());
			}

			value_t<f64[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f64[4]>(bitcast<f64>((get_imm<u64>(op.i8) + (1023 - 155)) << 52)));
			else
				s = eval(fsplat<f64[4]>(std::exp2(static_cast<int>(op.i8 - 155))));
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
				s = eval(vsplat<f32[4]>(load_const<f32>(m_scale_to_float, get_imm<u8>(op.i8))));
			else
				s = eval(fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))));
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
				r.value = build<f64[4]>(data._u32[0], data._u32[1], data._u32[2], data._u32[3]).eval(m_ir);
			}
			else
			{
				r.value = m_ir->CreateUIToFP(a.value, get_type<f64[4]>());
			}

			value_t<f64[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f64[4]>(bitcast<f64>((get_imm<u64>(op.i8) + (1023 - 155)) << 52)));
			else
				s = eval(fsplat<f64[4]>(std::exp2(static_cast<int>(op.i8 - 155))));
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
				s = eval(vsplat<f32[4]>(load_const<f32>(m_scale_to_float, get_imm<u8>(op.i8))));
			else
				s = eval(fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))));
			if (op.i8 != 155 || m_interp_magn)
				r = eval(r * s);
			set_vr(op.rt, r);
		}
	}

	void make_store_ls(value_t<u64> addr, value_t<u8[16]> data)
	{
		const auto bswapped = zshuffle(data, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
		m_ir->CreateStore(bswapped.eval(m_ir), m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
	}

	auto make_load_ls(value_t<u64> addr)
	{
		value_t<u8[16]> data;
		data.value = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_lsptr, addr.value), get_type<u8(*)[16]>()));
		return zshuffle(data, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
	}

	void STQX(spu_opcode_t op)
	{
		value_t<u64> addr = eval(zext<u64>((extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) & 0x3fff0));
		make_store_ls(addr, get_vr<u8[16]>(op.rt));
	}

	void LQX(spu_opcode_t op)
	{
		value_t<u64> addr = eval(zext<u64>((extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) & 0x3fff0));
		set_vr(op.rt, make_load_ls(addr));
	}

	void STQA(spu_opcode_t op)
	{
		value_t<u64> addr = eval((get_imm<u64>(op.i16, false) << 2) & 0x3fff0);
		make_store_ls(addr, get_vr<u8[16]>(op.rt));
	}

	void LQA(spu_opcode_t op)
	{
		value_t<u64> addr = eval((get_imm<u64>(op.i16, false) << 2) & 0x3fff0);
		set_vr(op.rt, make_load_ls(addr));
	}

	void STQR(spu_opcode_t op) //
	{
		value_t<u64> addr;
		addr.value = m_ir->CreateZExt(m_interp_magn ? m_interp_pc : get_pc(m_pos), get_type<u64>());
		addr = eval(((get_imm<u64>(op.i16, false) << 2) + addr) & 0x3fff0);
		make_store_ls(addr, get_vr<u8[16]>(op.rt));
	}

	void LQR(spu_opcode_t op) //
	{
		value_t<u64> addr;
		addr.value = m_ir->CreateZExt(m_interp_magn ? m_interp_pc : get_pc(m_pos), get_type<u64>());
		addr = eval(((get_imm<u64>(op.i16, false) << 2) + addr) & 0x3fff0);
		set_vr(op.rt, make_load_ls(addr));
	}

	void STQD(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn)
		{
			if (op.rt <= s_reg_sp || (op.rt >= s_reg_80 && op.rt <= s_reg_127))
			{
				if (m_block->bb->reg_save_dom[op.rt] && get_reg_raw(op.rt) == m_finfo->load[op.rt])
				{
					return;
				}
			}
		}

		value_t<u64> addr = eval(zext<u64>((extract(get_vr(op.ra), 3) + (get_imm<u32>(op.si10) << 4)) & 0x3fff0));
		make_store_ls(addr, get_vr<u8[16]>(op.rt));
	}

	void LQD(spu_opcode_t op)
	{
		value_t<u64> addr = eval(zext<u64>((extract(get_vr(op.ra), 3) + (get_imm<u32>(op.si10) << 4)) & 0x3fff0));
		set_vr(op.rt, make_load_ls(addr));
	}

	void make_halt(value_t<bool> cond)
	{
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto halt = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, halt, next, m_md_unlikely);
		m_ir->SetInsertPoint(halt);
		if (m_interp_magn)
			m_ir->CreateStore(&*(m_function->arg_begin() + 2), spu_ptr<u32>(&spu_thread::pc))->setVolatile(true);
		else
			update_pc();
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
			const auto e_addr = call("spu_check_interrupts", &exec_check_interrupts, m_thread, addr.value);
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

		if (llvm::isa<llvm::Constant>(addr.value))
		{
			// Fixed branch excludes the possibility it's a function return (TODO)
			ret = false;
		}

		if (m_finfo && m_finfo->fn && op.opcode)
		{
			const auto cblock = m_ir->GetInsertBlock();
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);
			ret_function();
			m_ir->SetInsertPoint(cblock);
			return result;
		}

		// Load stack addr if necessary
		value_t<u32> sp;

		if (ret && g_cfg.core.spu_block_size != spu_block_size_type::safe)
		{
			if (op.opcode)
			{
				sp = eval(extract(get_reg_fixed(1), 3) & 0x3fff0);
			}
			else
			{
				sp.value = m_ir->CreateLoad(spu_ptr<u32>(&spu_thread::gpr, 1, &v128::_u32, 3));
			}
		}

		const auto cblock = m_ir->GetInsertBlock();
		const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->SetInsertPoint(result);

		if (op.e)
		{
			addr.value = call("spu_check_interrupts", &exec_check_interrupts, m_thread, addr.value);
		}

		if (op.d)
		{
			m_ir->CreateStore(m_ir->getFalse(), spu_ptr<bool>(&spu_thread::interrupts_enabled))->setVolatile(true);
		}

		m_ir->CreateStore(addr.value, spu_ptr<u32>(&spu_thread::pc));
		const auto type = m_finfo->chunk->getFunctionType()->getPointerTo()->getPointerTo();

		if (ret && g_cfg.core.spu_block_size >= spu_block_size_type::mega)
		{
			// Compare address stored in stack mirror with addr
			const auto stack0 = eval(zext<u64>(sp) + ::offset32(&spu_thread::stack_mirror));
			const auto stack1 = eval(stack0 + 8);
			const auto _ret = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack0.value), type));
			const auto link = m_ir->CreateLoad(m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack1.value), get_type<u64*>()));
			const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto done = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpEQ(addr.value, m_ir->CreateTrunc(link, get_type<u32>())), done, fail, m_md_likely);
			m_ir->SetInsertPoint(done);

			// Clear stack mirror and return by tail call to the provided return address
			m_ir->CreateStore(splat<u64[2]>(-1).eval(m_ir), m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack0.value), get_type<u64(*)[2]>()));
			tail_chunk(_ret, m_ir->CreateTrunc(m_ir->CreateLShr(link, 32), get_type<u32>()));
			m_ir->SetInsertPoint(fail);
		}

		if (g_cfg.core.spu_block_size >= spu_block_size_type::mega)
		{
			// Try to load chunk address from the function table
			const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto done = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto ad32 = m_ir->CreateSub(addr.value, m_base_pc);
			m_ir->CreateCondBr(m_ir->CreateICmpULT(ad32, m_ir->getInt32(m_size)), done, fail, m_md_likely);
			m_ir->SetInsertPoint(done);

			const auto ad64 = m_ir->CreateZExt(ad32, get_type<u64>());
			const auto pptr = m_ir->CreateGEP(m_function_table, {m_ir->getInt64(0), m_ir->CreateLShr(ad64, 2, "", true)});
			tail_chunk(m_ir->CreateLoad(pptr));
			m_ir->SetInsertPoint(fail);
		}

		tail_chunk(nullptr);
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

		if (!op.d && !op.e && tfound != m_targets.end() && tfound->second.size() > 1)
		{
			// Shift aligned address for switch
			const auto addrfx = m_ir->CreateSub(addr.value, m_base_pc);
			const auto sw_arg = m_ir->CreateLShr(addrfx, 2, "", true);

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

			if (targets.empty())
			{
				// Emergency exit
				spu_log.error("[0x%05x] No jump table targets at 0x%05x (%u)", m_entry, m_pos, tfound->second.size());
				m_ir->CreateBr(add_block_indirect(op, addr));
				return;
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
						sw->addCase(m_ir->getInt32(pos / 4 - m_base / 4), found->second);
						continue;
					}
				}

				sw->addCase(m_ir->getInt32(pos / 4 - m_base / 4), sw->getDefaultDest());
			}

			// Exit function on unexpected target
			m_ir->SetInsertPoint(sw->getDefaultDest());
			m_ir->CreateStore(addr.value, spu_ptr<u32>(&spu_thread::pc), true);

			if (m_finfo && m_finfo->fn)
			{
				// Can't afford external tail call in true functions
				m_ir->CreateStore(m_ir->getInt32("BIJT"_u32), _ptr<u32>(m_memptr, 0xffdead20))->setVolatile(true);
				m_ir->CreateStore(m_ir->getFalse(), m_fake_global1, true);
				m_ir->CreateBr(sw->getDefaultDest());
			}
			else
			{
				tail_chunk(nullptr);
			}
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
		const auto res = call("spu_get_events", &exec_get_events, m_thread);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(m_ir->CreateICmpNE(res, m_ir->getInt32(0)), target, add_block_next());
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

		m_block->block_end = m_ir->GetInsertBlock();
		m_ir->CreateBr(add_block(target, true));
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

		const u32 target = spu_branch_target(m_pos, op.i16);

		if (m_finfo && m_finfo->fn && target != m_pos + 4)
		{
			if (auto fn = add_function(target)->fn)
			{
				call_function(fn);
				return;
			}
			else
			{
				spu_log.fatal("[0x%x] Can't add function 0x%x", m_pos, target);
				return;
			}
		}

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

		set_vr(op.rt, insert(splat<u32[4]>(0), 3, value<u32>(get_pc(m_pos + 4)) & 0x3fffc));

		if (m_finfo && m_finfo->fn)
		{
			return;
		}

		if (g_cfg.core.spu_block_size >= spu_block_size_type::mega && m_block_info[m_pos / 4 + 1] && m_entry_info[m_pos / 4 + 1])
		{
			// Store the return function chunk address at the stack mirror
			const auto pfunc = add_function(m_pos + 4);
			const auto stack0 = eval(zext<u64>(extract(get_reg_fixed(1), 3) & 0x3fff0) + ::offset32(&spu_thread::stack_mirror));
			const auto stack1 = eval(stack0 + 8);
			const auto base_plus_pc = m_ir->CreateOr(m_ir->CreateShl(m_ir->CreateZExt(m_base_pc, get_type<u64>()), 32), m_ir->getInt64(m_pos + 4));
			m_ir->CreateStore(pfunc->chunk, m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack0.value), pfunc->chunk->getType()->getPointerTo()));
			m_ir->CreateStore(base_plus_pc, m_ir->CreateBitCast(m_ir->CreateGEP(m_thread, stack1.value), get_type<u64*>()));
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

// SPU LLVM recompiler thread context
struct spu_llvm
{
	// Workload
	lf_queue<std::pair<const u64, spu_item*>> registered;

	void operator()()
	{
		// SPU LLVM Recompiler instance
		const auto compiler = spu_recompiler_base::make_llvm_recompiler();
		compiler->init();

		// Fake LS
		std::vector<be_t<u32>> ls(0x10000);

		// To compile (hash -> item)
		std::unordered_multimap<u64, spu_item*, value_hash<u64>> enqueued;

		// Mini-profiler (hash -> number of occurrences)
		std::unordered_map<u64, atomic_t<u64>, value_hash<u64>> samples;

		// For synchronization with profiler thread
		stx::init_mutex prof_mutex;

		named_thread profiler("SPU LLVM Profiler"sv, [&]()
		{
			while (thread_ctrl::state() != thread_state::aborting)
			{
				{
					// Lock if enabled
					const auto lock = prof_mutex.access();

					if (!lock)
					{
						// Wait when the profiler is disabled
						prof_mutex.wait_for_initialized();
						continue;
					}

					// Collect profiling samples
					idm::select<named_thread<spu_thread>>([&](u32 id, spu_thread& spu)
					{
						const u64 name = atomic_storage<u64>::load(spu.block_hash);

						if (!(spu.state.load() & (cpu_flag::wait + cpu_flag::stop + cpu_flag::dbg_global_pause)))
						{
							const auto found = std::as_const(samples).find(spu.block_hash);

							if (found != std::as_const(samples).end())
							{
								const_cast<atomic_t<u64>&>(found->second)++;
							}
						}
					});
				}

				// Sleep for a short period if enabled
				thread_ctrl::wait_for(20, false);
			}
		});

		while (thread_ctrl::state() != thread_state::aborting)
		{
			for (const auto& pair : registered.pop_all())
			{
				enqueued.emplace(pair);

				// Interrupt and kick profiler thread
				const auto lock = prof_mutex.init_always([&]{});

				// Register new blocks to collect samples
				samples.emplace(pair.first, 0);
			}

			if (enqueued.empty())
			{
				// Interrupt profiler thread and put it to sleep
				static_cast<void>(prof_mutex.reset());
				registered.wait();
				continue;
			}

			// Find the most used enqueued item
			u64 sample_max = 0;
			auto found_it  = enqueued.begin();

			for (auto it = enqueued.begin(), end = enqueued.end(); it != end; ++it)
			{
				const u64 cur = std::as_const(samples).at(it->first);

				if (cur > sample_max)
				{
					sample_max = cur;
					found_it = it;
				}
			}

			// Start compiling
			const spu_program& func = found_it->second->data;

			// Old function pointer (pre-recompiled)
			const spu_function_t _old = found_it->second->compiled;

			// Remove item from the queue
			enqueued.erase(found_it);

			// Get data start
			const u32 start = func.lower_bound;
			const u32 size0 = ::size32(func.data);

			// Initialize LS with function data only
			for (u32 i = 0, pos = start; i < size0; i++, pos += 4)
			{
				ls[pos / 4] = std::bit_cast<be_t<u32>>(func.data[i]);
			}

			// Call analyser
			spu_program func2 = compiler->analyse(ls.data(), func.entry_point);

			if (func2 != func)
			{
				spu_log.error("[0x%05x] SPU Analyser failed, %u vs %u", func2.entry_point, func2.data.size(), size0);
			}
			else if (const auto target = compiler->compile(std::move(func2)))
			{
				// Redirect old function (TODO: patch in multiple places)
				const s64 rel = reinterpret_cast<u64>(target) - reinterpret_cast<u64>(_old) - 5;

				union
				{
					u8 bytes[8];
					u64 result;
				};

				bytes[0] = 0xe9; // jmp rel32
				std::memcpy(bytes + 1, &rel, 4);
				bytes[5] = 0x90;
				bytes[6] = 0x90;
				bytes[7] = 0x90;

				atomic_storage<u64>::release(*reinterpret_cast<u64*>(_old), result);
			}
			else
			{
				spu_log.fatal("[0x%05x] Compilation failed.", func.entry_point);
				Emu.Pause();
				return;
			}

			// Clear fake LS
			std::memset(ls.data() + start / 4, 0, 4 * (size0 - 1));
		}
	}

	static constexpr auto thread_name = "SPU LLVM"sv;
};

using spu_llvm_thread = named_thread<spu_llvm>;

struct spu_fast : public spu_recompiler_base
{
	virtual void init() override
	{
		if (!m_spurt)
		{
			m_spurt = g_fxo->get<spu_runtime>();
		}
	}

	virtual spu_function_t compile(spu_program&& _func) override
	{
		const auto add_loc = m_spurt->add_empty(std::move(_func));

		if (!add_loc)
		{
			return nullptr;
		}

		if (add_loc->compiled)
		{
			return add_loc->compiled;
		}

		const spu_program& func = add_loc->data;

		if (g_cfg.core.spu_debug && !add_loc->logged.exchange(1))
		{
			std::string log;
			this->dump(func, log);
			fs::file(m_spurt->get_cache_path() + "spu.log", fs::write + fs::append).write(log);
		}

		// Allocate executable area with necessary size
		const auto result = jit_runtime::alloc(22 + 1 + 9 + ::size32(func.data) * (16 + 16) + 36 + 47, 16);

		if (!result)
		{
			return nullptr;
		}

		m_pos = func.lower_bound;
		m_size = ::size32(func.data) * 4;

		{
			sha1_context ctx;
			u8 output[20];

			sha1_starts(&ctx);
			sha1_update(&ctx, reinterpret_cast<const u8*>(func.data.data()), func.data.size() * 4);
			sha1_finish(&ctx, output);

			be_t<u64> hash_start;
			std::memcpy(&hash_start, output, sizeof(hash_start));
			m_hash_start = hash_start;
		}

		u8* raw = result;

		// 8-byte intruction for patching (long NOP)
		*raw++ = 0x0f;
		*raw++ = 0x1f;
		*raw++ = 0x84;
		*raw++ = 0;
		*raw++ = 0;
		*raw++ = 0;
		*raw++ = 0;
		*raw++ = 0;

		// mov rax, m_hash_start
		*raw++ = 0x48;
		*raw++ = 0xb8;
		std::memcpy(raw, &m_hash_start, sizeof(m_hash_start));
		raw += 8;

		// Update block_hash: mov [r13 + spu_thread::m_block_hash], rax
		*raw++ = 0x49;
		*raw++ = 0x89;
		*raw++ = 0x45;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::block_hash));

		// Load PC: mov eax, [r13 + spu_thread::pc]
		*raw++ = 0x41;
		*raw++ = 0x8b;
		*raw++ = 0x45;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::pc));

		// Get LS address starting from PC: lea rcx, [rbp + rax]
		*raw++ = 0x48;
		*raw++ = 0x8d;
		*raw++ = 0x4c;
		*raw++ = 0x05;
		*raw++ = 0x00;

		// Verification (slow)
		for (u32 i = 0; i < func.data.size(); i++)
		{
			if (!func.data[i])
			{
				continue;
			}

			// cmp dword ptr [rcx + off], opc
			*raw++ = 0x81;
			*raw++ = 0xb9;
			const u32 off = i * 4;
			const u32 opc = func.data[i];
			std::memcpy(raw + 0, &off, 4);
			std::memcpy(raw + 4, &opc, 4);
			raw += 8;

			// jne tr_dispatch
			const s64 rel = reinterpret_cast<u64>(spu_runtime::tr_dispatch) - reinterpret_cast<u64>(raw) - 6;
			*raw++ = 0x0f;
			*raw++ = 0x85;
			std::memcpy(raw + 0, &rel, 4);
			raw += 4;
		}

		// trap
		//*raw++ = 0xcc;

		// Secondary prologue: sub rsp,0x28
		*raw++ = 0x48;
		*raw++ = 0x83;
		*raw++ = 0xec;
		*raw++ = 0x28;

		// Fix args: xchg r13,rbp
		*raw++ = 0x49;
		*raw++ = 0x87;
		*raw++ = 0xed;

		// mov r12d, eax
		*raw++ = 0x41;
		*raw++ = 0x89;
		*raw++ = 0xc4;

		// mov esi, 0x7f0
		*raw++ = 0xbe;
		*raw++ = 0xf0;
		*raw++ = 0x07;
		*raw++ = 0x00;
		*raw++ = 0x00;

		// lea rdi, [rbp + spu_thread::gpr]
		*raw++ = 0x48;
		*raw++ = 0x8d;
		*raw++ = 0x7d;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::gpr));

		// Save base pc: mov [rbp + spu_thread::base_pc], eax
		*raw++ = 0x89;
		*raw++ = 0x45;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::base_pc));

		// inc block_counter
		*raw++ = 0x48;
		*raw++ = 0xff;
		*raw++ = 0x85;
		const u32 blc_off = ::offset32(&spu_thread::block_counter);
		std::memcpy(raw, &blc_off, 4);
		raw += 4;

		// lea r14, [local epilogue]
		*raw++ = 0x4c;
		*raw++ = 0x8d;
		*raw++ = 0x35;
		const u32 epi_off = ::size32(func.data) * 16;
		std::memcpy(raw, &epi_off, 4);
		raw += 4;

		// Instructions (each instruction occupies fixed number of bytes)
		for (u32 i = 0; i < func.data.size(); i++)
		{
			const u32 pos = m_pos + i * 4;

			if (!func.data[i])
			{
				// Save pc: mov [rbp + spu_thread::pc], r12d
				*raw++ = 0x44;
				*raw++ = 0x89;
				*raw++ = 0x65;
				*raw++ = ::narrow<s8>(::offset32(&spu_thread::pc));

				// Epilogue: add rsp,0x28
				*raw++ = 0x48;
				*raw++ = 0x83;
				*raw++ = 0xc4;
				*raw++ = 0x28;

				// ret (TODO)
				*raw++ = 0xc3;
				std::memset(raw, 0xcc, 16 - 9);
				raw += 16 - 9;
				continue;
			}

			// Fix endianness
			const spu_opcode_t op{std::bit_cast<be_t<u32>>(func.data[i])};

			switch (auto type = s_spu_itype.decode(op.opcode))
			{
			case spu_itype::BRZ:
			case spu_itype::BRHZ:
			case spu_itype::BRNZ:
			case spu_itype::BRHNZ:
			{
				const u32 target = spu_branch_target(pos, op.i16);

				if (0 && target >= m_pos && target < m_pos + m_size)
				{
					*raw++ = type == spu_itype::BRHZ || type == spu_itype::BRHNZ ? 0x66 : 0x90;
					*raw++ = 0x83;
					*raw++ = 0xbd;
					const u32 off = ::offset32(&spu_thread::gpr, op.rt) + 12;
					std::memcpy(raw, &off, 4);
					raw += 4;
					*raw++ = 0x00;

					*raw++ = 0x0f;
					*raw++ = type == spu_itype::BRZ || type == spu_itype::BRHZ ? 0x84 : 0x85;
					const u32 dif = (target - (pos + 4)) / 4 * 16 + 2;
					std::memcpy(raw, &dif, 4);
					raw += 4;

					*raw++ = 0x66;
					*raw++ = 0x90;
					break;
				}

				[[fallthrough]];
			}
			default:
			{
				// Ballast: mov r15d, pos
				*raw++ = 0x41;
				*raw++ = 0xbf;
				std::memcpy(raw, &pos, 4);
				raw += 4;

				// mov ebx, opc
				*raw++ = 0xbb;
				std::memcpy(raw, &op, 4);
				raw += 4;

				// call spu_* (specially built interpreter function)
				const s64 rel = spu_runtime::g_interpreter_table[type] - reinterpret_cast<u64>(raw) - 5;
				*raw++ = 0xe8;
				std::memcpy(raw, &rel, 4);
				raw += 4;
				break;
			}
			}
		}

		// Local dispatcher/epilogue: fix stack after branch instruction, then dispatch or return

		// add rsp, 8
		*raw++ = 0x48;
		*raw++ = 0x83;
		*raw++ = 0xc4;
		*raw++ = 0x08;

		// and rsp, -16
		*raw++ = 0x48;
		*raw++ = 0x83;
		*raw++ = 0xe4;
		*raw++ = 0xf0;

		// lea rax, [r12 - size]
		*raw++ = 0x49;
		*raw++ = 0x8d;
		*raw++ = 0x84;
		*raw++ = 0x24;
		const u32 msz = 0u - m_size;
		std::memcpy(raw, &msz, 4);
		raw += 4;

		// sub eax, [rbp + spu_thread::base_pc]
		*raw++ = 0x2b;
		*raw++ = 0x45;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::base_pc));

		// cmp eax, (0 - size)
		*raw++ = 0x3d;
		std::memcpy(raw, &msz, 4);
		raw += 4;

		// jb epilogue
		*raw++ = 0x72;
		*raw++ = +12;

		// movsxd rax, eax
		*raw++ = 0x48;
		*raw++ = 0x63;
		*raw++ = 0xc0;

		// shl rax, 2
		*raw++ = 0x48;
		*raw++ = 0xc1;
		*raw++ = 0xe0;
		*raw++ = 0x02;

		// add rax, r14
		*raw++ = 0x4c;
		*raw++ = 0x01;
		*raw++ = 0xf0;

		// jmp rax
		*raw++ = 0xff;
		*raw++ = 0xe0;

		// Save pc: mov [rbp + spu_thread::pc], r12d
		*raw++ = 0x44;
		*raw++ = 0x89;
		*raw++ = 0x65;
		*raw++ = ::narrow<s8>(::offset32(&spu_thread::pc));

		// Epilogue: add rsp,0x28 ; ret
		*raw++ = 0x48;
		*raw++ = 0x83;
		*raw++ = 0xc4;
		*raw++ = 0x28;
		*raw++ = 0xc3;

		const auto fn = reinterpret_cast<spu_function_t>(result);

		// Install pointer carefully
		const bool added = !add_loc->compiled && add_loc->compiled.compare_and_swap_test(nullptr, fn);

		if (added)
		{
			// Send work to LLVM compiler thread
			g_fxo->get<spu_llvm_thread>()->registered.push(m_hash_start, add_loc);
		}

		// Rebuild trampoline if necessary
		if (!m_spurt->rebuild_ubertrampoline(func.data[0]))
		{
			return nullptr;
		}

		if (added)
		{
			add_loc->compiled.notify_all();
		}

		return fn;
	}
};

std::unique_ptr<spu_recompiler_base> spu_recompiler_base::make_fast_llvm_recompiler()
{
	return std::make_unique<spu_fast>();
}
