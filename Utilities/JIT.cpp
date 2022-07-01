#include "util/types.hpp"
#include "util/sysinfo.hpp"
#include "JIT.h"
#include "StrFmt.h"
#include "File.h"
#include "util/logs.hpp"
#include "mutex.h"
#include "util/vm.hpp"
#include "util/asm.hpp"
#include <charconv>
#include <zlib.h>

#ifdef __linux__
#define CAN_OVERCOMMIT
#endif

LOG_CHANNEL(jit_log, "JIT");

void jit_announce(uptr func, usz size, std::string_view name)
{
	if (!size)
	{
		jit_log.error("Empty function announced: %s (%p)", name, func);
		return;
	}

	// If directory ASMJIT doesn't exist, nothing will be written
	static constexpr u64 c_dump_size = 0x1'0000'0000;
	static constexpr u64 c_index_size = c_dump_size / 16;
	static atomic_t<u64> g_index_off = 0;
	static atomic_t<u64> g_data_off = c_index_size;

	static void* g_asm = []() -> void*
	{
		fs::remove_all(fs::get_cache_dir() + "/ASMJIT/", false);

		fs::file objs(fmt::format("%s/ASMJIT/.objects", fs::get_cache_dir()), fs::read + fs::rewrite);

		if (!objs || !objs.trunc(c_dump_size))
		{
			return nullptr;
		}

		return utils::memory_map_fd(objs.get_handle(), c_dump_size, utils::protection::rw);
	}();

	if (g_asm && size < c_index_size)
	{
		struct entry
		{
			u64 addr; // RPCS3 process address
			u32 size; // Function size
			u32 off; // Function offset
		};

		// Write index entry at the beginning of file, and data + NTS name at fixed offset
		const u64 index_off = g_index_off.fetch_add(1);
		const u64 size_all = size + name.size() + 1;
		const u64 data_off = g_data_off.fetch_add(size_all);

		// If either index or data area is exhausted, nothing will be written
		if (index_off < c_index_size / sizeof(entry) && data_off + size_all < c_dump_size)
		{
			entry& index = static_cast<entry*>(g_asm)[index_off];

			std::memcpy(static_cast<char*>(g_asm) + data_off, reinterpret_cast<char*>(func), size);
			std::memcpy(static_cast<char*>(g_asm) + data_off + size, name.data(), name.size());
			index.size = static_cast<u32>(size);
			index.off = static_cast<u32>(data_off);
			atomic_storage<u64>::store(index.addr, func);
		}
	}

	if (g_asm && !name.empty() && name[0] != '_')
	{
		// Save some objects separately
		fs::file dump(fmt::format("%s/ASMJIT/%s", fs::get_cache_dir(), name), fs::rewrite);

		if (dump)
		{
			dump.write(reinterpret_cast<uchar*>(func), size);
		}
	}

#ifdef __linux__
	static const fs::file s_map(fmt::format("/tmp/perf-%d.map", getpid()), fs::rewrite + fs::append);

	s_map.write(fmt::format("%x %x %s\n", func, size, name));
#endif
}

static u8* get_jit_memory()
{
	// Reserve 2G memory (magic static)
	static void* const s_memory2 = []() -> void*
	{
		void* ptr = utils::memory_reserve(0x80000000);
#ifdef CAN_OVERCOMMIT
		utils::memory_commit(ptr, 0x80000000);
		utils::memory_protect(ptr, 0x40000000, utils::protection::wx);
#endif
		return ptr;
	}();

	return static_cast<u8*>(s_memory2);
}

// Allocation counters (1G code, 1G data subranges)
static atomic_t<u64> s_code_pos{0}, s_data_pos{0};

// Snapshot of code generated before main()
static std::vector<u8> s_code_init, s_data_init;

template <atomic_t<u64>& Ctr, uint Off, utils::protection Prot>
static u8* add_jit_memory(usz size, uint align)
{
	// Select subrange
	u8* pointer = get_jit_memory() + Off;

	if (!size && !align) [[unlikely]]
	{
		// Return subrange info
		return pointer;
	}

	u64 olda, newa;

	// Simple allocation by incrementing pointer to the next free data
	const u64 pos = Ctr.atomic_op([&](u64& ctr) -> u64
	{
		const u64 _pos = utils::align(ctr & 0xffff'ffff, align);
		const u64 _new = utils::align(_pos + size, align);

		if (_new > 0x40000000) [[unlikely]]
		{
			// Sorry, we failed, and further attempts should fail too.
			ctr |= 0x40000000;
			return -1;
		}

		// Last allocation is stored in highest bits
		olda = ctr >> 32;
		newa = olda;

		// Check the necessity to commit more memory
		if (_new > olda) [[unlikely]]
		{
			newa = utils::align(_new, 0x200000);
		}

		ctr += _new - (ctr & 0xffff'ffff);
		return _pos;
	});

	if (pos == umax) [[unlikely]]
	{
		jit_log.error("Out of memory (size=0x%x, align=0x%x, off=0x%x)", size, align, Off);
		return nullptr;
	}

	if (olda != newa) [[unlikely]]
	{
#ifndef CAN_OVERCOMMIT
		// Commit more memory
		utils::memory_commit(pointer + olda, newa - olda, Prot);
#endif
		// Acknowledge committed memory
		Ctr.atomic_op([&](u64& ctr)
		{
			if ((ctr >> 32) < newa)
			{
				ctr += (newa - (ctr >> 32)) << 32;
			}
		});
	}

	return pointer + pos;
}

const asmjit::Environment& jit_runtime_base::environment() const noexcept
{
	static const asmjit::Environment g_env = asmjit::Environment::host();

	return g_env;
}

void* jit_runtime_base::_add(asmjit::CodeHolder* code) noexcept
{
	ensure(!code->flatten());
	ensure(!code->resolveUnresolvedLinks());
	usz codeSize = code->codeSize();
	if (!codeSize)
		return nullptr;

	auto p = ensure(this->_alloc(codeSize, 64));
	ensure(!code->relocateToBase(uptr(p)));

	{
		// We manage rw <-> rx transitions manually on Apple
		// because it's easier to keep track of when and where we need to toggle W^X
#if !(defined(ARCH_ARM64) && defined(__APPLE__))
		asmjit::VirtMem::ProtectJitReadWriteScope rwScope(p, codeSize);
#endif

		for (asmjit::Section* section : code->_sections)
		{
			std::memcpy(p + section->offset(), section->data(), section->bufferSize());
		}
	}

	return p;
}

jit_runtime::jit_runtime()
{
}

jit_runtime::~jit_runtime()
{
}

uchar* jit_runtime::_alloc(usz size, usz align) noexcept
{
	return jit_runtime::alloc(size, align, true);
}

u8* jit_runtime::alloc(usz size, uint align, bool exec) noexcept
{
	if (exec)
	{
		return add_jit_memory<s_code_pos, 0x0, utils::protection::wx>(size, align);
	}
	else
	{
		return add_jit_memory<s_data_pos, 0x40000000, utils::protection::rw>(size, align);
	}
}

void jit_runtime::initialize()
{
	if (!s_code_init.empty() || !s_data_init.empty())
	{
		return;
	}

	// Create code/data snapshot
	s_code_init.resize(s_code_pos & 0xffff'ffff);
	std::memcpy(s_code_init.data(), alloc(0, 0, true), s_code_init.size());
	s_data_init.resize(s_data_pos & 0xffff'ffff);
	std::memcpy(s_data_init.data(), alloc(0, 0, false), s_data_init.size());
}

void jit_runtime::finalize() noexcept
{
#ifdef __APPLE__
	pthread_jit_write_protect_np(false);
#endif
	// Reset JIT memory
#ifdef CAN_OVERCOMMIT
	utils::memory_reset(get_jit_memory(), 0x80000000);
	utils::memory_protect(get_jit_memory(), 0x40000000, utils::protection::wx);
#else
	utils::memory_decommit(get_jit_memory(), 0x80000000);
#endif

	s_code_pos = 0;
	s_data_pos = 0;

	// Restore code/data snapshot
	std::memcpy(alloc(s_code_init.size(), 1, true), s_code_init.data(), s_code_init.size());
	std::memcpy(alloc(s_data_init.size(), 1, false), s_data_init.data(), s_data_init.size());

#ifdef __APPLE__
	pthread_jit_write_protect_np(true);
#endif
#ifdef ARCH_ARM64
	// Flush all cache lines after potentially writing executable code
	asm("ISB");
	asm("DSB ISH");
#endif
}

jit_runtime_base& asmjit::get_global_runtime()
{
	// 16 MiB for internal needs
	static constexpr u64 size = 1024 * 1024 * 16;

	struct custom_runtime final : jit_runtime_base
	{
		custom_runtime() noexcept
		{
			// Search starting in first 2 GiB of memory
			for (u64 addr = size;; addr += size)
			{
				if (auto ptr = utils::memory_reserve(size, reinterpret_cast<void*>(addr)))
				{
					m_pos.raw() = static_cast<uchar*>(ptr);
					break;
				}
			}

			// Initialize "end" pointer
			m_max = m_pos + size;

			// Make memory writable + executable
			utils::memory_commit(m_pos, size, utils::protection::wx);
		}

		uchar* _alloc(usz size, usz align) noexcept override
		{
			return m_pos.atomic_op([&](uchar*& pos) -> uchar*
			{
				const auto r = reinterpret_cast<uchar*>(utils::align(uptr(pos), align));

				if (r >= pos && r + size > pos && r + size <= m_max)
				{
					pos = r + size;
					return r;
				}

				return nullptr;
			});
		}

	private:
		atomic_t<uchar*> m_pos{};

		uchar* m_max{};
	};

	// Magic static
	static custom_runtime g_rt;
	return g_rt;
}

asmjit::inline_runtime::inline_runtime(uchar* data, usz size)
	: m_data(data)
	, m_size(size)
{
}

uchar* asmjit::inline_runtime::_alloc(usz size, usz align) noexcept
{
	ensure(align <= 4096);

	return size <= m_size ? m_data : nullptr;
}

asmjit::inline_runtime::~inline_runtime()
{
	utils::memory_protect(m_data, m_size, utils::protection::rx);
}

#ifdef LLVM_AVAILABLE

#include <unordered_map>
#include <unordered_set>
#include <deque>

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Host.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/ObjectCache.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/SymbolSize.h"
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

const bool jit_initialize = []() -> bool
{
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();
	LLVMLinkInMCJIT();
	return true;
}();

[[noreturn]] static void null(const char* name)
{
	fmt::throw_exception("Null function: %s", name);
}

namespace vm
{
	extern u8* const g_sudo_addr;
}

static shared_mutex null_mtx;

static std::unordered_map<std::string, u64> null_funcs;

static u64 make_null_function(const std::string& name)
{
	if (name.starts_with("__0x"))
	{
		u32 addr = -1;
		auto res = std::from_chars(name.c_str() + 4, name.c_str() + name.size(), addr, 16);

		if (res.ec == std::errc() && res.ptr == name.c_str() + name.size() && addr < 0x8000'0000)
		{
			// Point the garbage to reserved, non-executable memory
			return reinterpret_cast<u64>(vm::g_sudo_addr + addr);
		}
	}

	std::lock_guard lock(null_mtx);

	if (u64& func_ptr = null_funcs[name]) [[likely]]
	{
		// Already exists
		return func_ptr;
	}
	else
	{
		using namespace asmjit;

		// Build a "null" function that contains its name
		const auto func = build_function_asm<void (*)()>("NULL", [&](native_asm& c, auto& args)
		{
#if defined(ARCH_X64)
			Label data = c.newLabel();
			c.lea(args[0], x86::qword_ptr(data, 0));
			c.jmp(Imm(&null));
			c.align(AlignMode::kCode, 16);
			c.bind(data);

			// Copy function name bytes
			for (char ch : name)
				c.db(ch);
			c.db(0);
			c.align(AlignMode::kData, 16);
#else
			// AArch64 implementation
			Label jmp_address = c.newLabel();
			Label data = c.newLabel();
			// Force absolute jump to prevent out of bounds PC-rel jmp
			c.ldr(args[0], arm::ptr(jmp_address));
			c.br(args[0]);
			c.align(AlignMode::kCode, 16);

			c.bind(data);
			c.embed(name.c_str(), name.size());
			c.embedUInt8(0U);
			c.bind(jmp_address);
			c.embedUInt64(reinterpret_cast<u64>(&null));
			c.align(AlignMode::kData, 16);
#endif
		});

		func_ptr = reinterpret_cast<u64>(func);
		return func_ptr;
	}
}

struct JITAnnouncer : llvm::JITEventListener
{
	void notifyObjectLoaded(u64, const llvm::object::ObjectFile& obj, const llvm::RuntimeDyld::LoadedObjectInfo& info) override
	{
		using namespace llvm;

		object::OwningBinary<object::ObjectFile> debug_obj_ = info.getObjectForDebug(obj);
		if (!debug_obj_.getBinary())
		{
#ifdef __linux__
			jit_log.error("LLVM: Failed to announce JIT events (no debug object)");
#endif
			return;
		}

		const object::ObjectFile& debug_obj = *debug_obj_.getBinary();

		for (const auto& [sym, size] : computeSymbolSizes(debug_obj))
		{
			Expected<object::SymbolRef::Type> type_ = sym.getType();
			if (!type_ || *type_ != object::SymbolRef::ST_Function)
				continue;

			Expected<StringRef> name = sym.getName();
			if (!name)
				continue;

			Expected<u64> addr = sym.getAddress();
			if (!addr)
				continue;

			jit_announce(*addr, size, {name->data(), name->size()});
		}
	}
};

// Simple memory manager
struct MemoryManager1 : llvm::RTDyldMemoryManager
{
	// 256 MiB for code or data
	static constexpr u64 c_max_size = 0x20000000 / 2;

	// Allocation unit (2M)
	static constexpr u64 c_page_size = 2 * 1024 * 1024;

	// Reserve 512 MiB
	u8* const ptr = static_cast<u8*>(utils::memory_reserve(c_max_size * 2));

	u64 code_ptr = 0;
	u64 data_ptr = c_max_size;

	MemoryManager1() = default;

	MemoryManager1(const MemoryManager1&) = delete;

	MemoryManager1& operator=(const MemoryManager1&) = delete;

	~MemoryManager1() override
	{
		// Hack: don't release to prevent reuse of address space, see jit_announce
		utils::memory_decommit(ptr, c_max_size * 2);
	}

	llvm::JITSymbol findSymbol(const std::string& name) override
	{
		u64 addr = RTDyldMemoryManager::getSymbolAddress(name);

		if (!addr)
		{
			addr = make_null_function(name);

			if (!addr)
			{
				fmt::throw_exception("Failed to link '%s'", name);
			}
		}

		return {addr, llvm::JITSymbolFlags::Exported};
	}

	u8* allocate(u64& oldp, uptr size, uint align, utils::protection prot)
	{
		if (align > c_page_size)
		{
			jit_log.fatal("Unsupported alignment (size=0x%x, align=0x%x)", size, align);
			return nullptr;
		}

		const u64 olda = utils::align(oldp, align);
		const u64 newp = utils::align(olda + size, align);

		if ((newp - 1) / c_max_size != oldp / c_max_size)
		{
			jit_log.fatal("Out of memory (size=0x%x, align=0x%x)", size, align);
			return nullptr;
		}

		if ((oldp - 1) / c_page_size != (newp - 1) / c_page_size)
		{
			// Allocate pages on demand
			const u64 pagea = utils::align(oldp, c_page_size);
			const u64 psize = utils::align(newp - pagea, c_page_size);
			utils::memory_commit(this->ptr + pagea, psize, prot);
		}

		// Update allocation counter
		oldp = newp;

		return this->ptr + olda;
	}

	u8* allocateCodeSection(uptr size, uint align, uint /*sec_id*/, llvm::StringRef /*sec_name*/) override
	{
		return allocate(code_ptr, size, align, utils::protection::wx);
	}

	u8* allocateDataSection(uptr size, uint align, uint /*sec_id*/, llvm::StringRef /*sec_name*/, bool /*is_ro*/) override
	{
		return allocate(data_ptr, size, align, utils::protection::rw);
	}

	bool finalizeMemory(std::string* = nullptr) override
	{
		return false;
	}

	void registerEHFrames(u8*, u64, usz) override
	{
	}

	void deregisterEHFrames() override
	{
	}
};

// Simple memory manager
struct MemoryManager2 : llvm::RTDyldMemoryManager
{
	MemoryManager2() = default;

	~MemoryManager2() override
	{
	}

	llvm::JITSymbol findSymbol(const std::string& name) override
	{
		u64 addr = RTDyldMemoryManager::getSymbolAddress(name);

		if (!addr)
		{
			addr = make_null_function(name);

			if (!addr)
			{
				fmt::throw_exception("Failed to link '%s' (MM2)", name);
			}
		}

		return {addr, llvm::JITSymbolFlags::Exported};
	}

	u8* allocateCodeSection(uptr size, uint align, uint /*sec_id*/, llvm::StringRef /*sec_name*/) override
	{
		return jit_runtime::alloc(size, align, true);
	}

	u8* allocateDataSection(uptr size, uint align, uint /*sec_id*/, llvm::StringRef /*sec_name*/, bool /*is_ro*/) override
	{
		return jit_runtime::alloc(size, align, false);
	}

	bool finalizeMemory(std::string* = nullptr) override
	{
		return false;
	}

	void registerEHFrames(u8*, u64, usz) override
	{
	}

	void deregisterEHFrames() override
	{
	}
};

// Helper class
class ObjectCache final : public llvm::ObjectCache
{
	const std::string& m_path;

public:
	ObjectCache(const std::string& path)
		: m_path(path)
	{
	}

	~ObjectCache() override = default;

	void notifyObjectCompiled(const llvm::Module* _module, llvm::MemoryBufferRef obj) override
	{
		std::string name = m_path;
		name.append(_module->getName().data());
		//fs::file(name, fs::rewrite).write(obj.getBufferStart(), obj.getBufferSize());
		name.append(".gz");

		z_stream zs{};
		uLong zsz = compressBound(::narrow<u32>(obj.getBufferSize())) + 256;
		auto zbuf = std::make_unique<uchar[]>(zsz);
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
		deflateInit2(&zs, 9, Z_DEFLATED, 16 + 15, 9, Z_DEFAULT_STRATEGY);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
		zs.avail_in  = static_cast<uInt>(obj.getBufferSize());
		zs.next_in   = reinterpret_cast<uchar*>(const_cast<char*>(obj.getBufferStart()));
		zs.avail_out = static_cast<uInt>(zsz);
		zs.next_out  = zbuf.get();

		switch (deflate(&zs, Z_FINISH))
		{
		case Z_OK:
		case Z_STREAM_END:
		{
			deflateEnd(&zs);
			break;
		}
		default:
		{
			jit_log.error("LLVM: Failed to compress module: %s", _module->getName().data());
			deflateEnd(&zs);
			return;
		}
		}

		if (!fs::write_file(name, fs::rewrite, zbuf.get(), zsz - zs.avail_out))
		{
				jit_log.error("LLVM: Failed to create module file: %s (%s)", name, fs::g_tls_error);
				return;
		}

		jit_log.notice("LLVM: Created module: %s", _module->getName().data());
	}

	static std::unique_ptr<llvm::MemoryBuffer> load(const std::string& path)
	{
		if (fs::file cached{path + ".gz", fs::read})
		{
			std::vector<uchar> gz = cached.to_vector<uchar>();
			std::vector<uchar> out;
			z_stream zs{};

			if (gz.empty()) [[unlikely]]
			{
				return nullptr;
			}
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
			inflateInit2(&zs, 16 + 15);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
			zs.avail_in = static_cast<uInt>(gz.size());
			zs.next_in  = gz.data();
			out.resize(gz.size() * 6);
			zs.avail_out = static_cast<uInt>(out.size());
			zs.next_out  = out.data();

			while (zs.avail_in)
			{
				switch (inflate(&zs, Z_FINISH))
				{
				case Z_OK: break;
				case Z_STREAM_END: break;
				case Z_BUF_ERROR:
				{
					if (zs.avail_in)
						break;
					[[fallthrough]];
				}
				default:
					inflateEnd(&zs);
					return nullptr;
				}

				if (zs.avail_in)
				{
					auto cur_size = zs.next_out - out.data();
					out.resize(out.size() + 65536);
					zs.avail_out = static_cast<uInt>(out.size() - cur_size);
					zs.next_out = out.data() + cur_size;
				}
			}

			out.resize(zs.next_out - out.data());
			inflateEnd(&zs);

			auto buf = llvm::WritableMemoryBuffer::getNewUninitMemBuffer(out.size());
			std::memcpy(buf->getBufferStart(), out.data(), out.size());
			return buf;
		}

		if (fs::file cached{path, fs::read})
		{
			if (cached.size() == 0) [[unlikely]]
			{
				return nullptr;
			}

			auto buf = llvm::WritableMemoryBuffer::getNewUninitMemBuffer(cached.size());
			cached.read(buf->getBufferStart(), buf->getBufferSize());
			return buf;
		}

		return nullptr;
	}

	std::unique_ptr<llvm::MemoryBuffer> getObject(const llvm::Module* _module) override
	{
		std::string path = m_path;
		path.append(_module->getName().data());

		if (auto buf = load(path))
		{
			jit_log.notice("LLVM: Loaded module: %s", _module->getName().data());
			return buf;
		}

		return nullptr;
	}
};

std::string jit_compiler::cpu(const std::string& _cpu)
{
	std::string m_cpu = _cpu;

	if (m_cpu.empty())
	{
		m_cpu = llvm::sys::getHostCPUName().operator std::string();

		if (m_cpu == "sandybridge" ||
			m_cpu == "ivybridge" ||
			m_cpu == "haswell" ||
			m_cpu == "broadwell" ||
			m_cpu == "skylake" ||
			m_cpu == "skylake-avx512" ||
			m_cpu == "cascadelake" ||
			m_cpu == "cooperlake" ||
			m_cpu == "cannonlake" ||
			m_cpu == "icelake" ||
			m_cpu == "icelake-client" ||
			m_cpu == "icelake-server" ||
			m_cpu == "tigerlake" ||
			m_cpu == "rocketlake")
		{
			// Downgrade if AVX is not supported by some chips
			if (!utils::has_avx())
			{
				m_cpu = "nehalem";
			}
		}

		if (m_cpu == "skylake-avx512" ||
			m_cpu == "cascadelake" ||
			m_cpu == "cooperlake" ||
			m_cpu == "cannonlake" ||
			m_cpu == "icelake" ||
			m_cpu == "icelake-client" ||
			m_cpu == "icelake-server" ||
			m_cpu == "tigerlake" ||
			m_cpu == "rocketlake")
		{
			// Downgrade if AVX-512 is disabled or not supported
			if (!utils::has_avx512())
			{
				m_cpu = "skylake";
			}
		}

		if (m_cpu == "znver1" && utils::has_clwb())
		{
			// Upgrade
			m_cpu = "znver2";
		}
	}

	return m_cpu;
}

jit_compiler::jit_compiler(const std::unordered_map<std::string, u64>& _link, const std::string& _cpu, u32 flags)
	: m_context(new llvm::LLVMContext)
	, m_cpu(cpu(_cpu))
{
	std::string result;

	auto null_mod = std::make_unique<llvm::Module> ("null_", *m_context);
#if defined(__APPLE__) && defined(ARCH_ARM64)
	// Force override triple on Apple arm64 or we'll get linking errors.
	null_mod->setTargetTriple(llvm::Triple::normalize(utils::c_llvm_default_triple));
#endif

	if (_link.empty())
	{
		std::unique_ptr<llvm::RTDyldMemoryManager> mem;

		if (flags & 0x1)
		{
			mem = std::make_unique<MemoryManager1>();
		}
		else
		{
			mem = std::make_unique<MemoryManager2>();
			null_mod->setTargetTriple(llvm::Triple::normalize(utils::c_llvm_default_triple));
		}

		// Auxiliary JIT (does not use custom memory manager, only writes the objects)
		m_engine.reset(llvm::EngineBuilder(std::move(null_mod))
			.setErrorStr(&result)
			.setEngineKind(llvm::EngineKind::JIT)
			.setMCJITMemoryManager(std::move(mem))
			.setOptLevel(llvm::CodeGenOpt::Aggressive)
			.setCodeModel(flags & 0x2 ? llvm::CodeModel::Large : llvm::CodeModel::Small)
			.setMCPU(m_cpu)
			.create());
	}
	else
	{
		// Primary JIT
		m_engine.reset(llvm::EngineBuilder(std::move(null_mod))
			.setErrorStr(&result)
			.setEngineKind(llvm::EngineKind::JIT)
			.setMCJITMemoryManager(std::make_unique<MemoryManager1>())
			.setOptLevel(llvm::CodeGenOpt::Aggressive)
			.setCodeModel(flags & 0x2 ? llvm::CodeModel::Large : llvm::CodeModel::Small)
			.setMCPU(m_cpu)
			.create());

		for (auto&& [name, addr] : _link)
		{
			m_engine->updateGlobalMapping(name, addr);
		}
	}

	if (!_link.empty() || !(flags & 0x1))
	{
		m_engine->RegisterJITEventListener(llvm::JITEventListener::createIntelJITEventListener());
		m_engine->RegisterJITEventListener(new JITAnnouncer);
	}

	if (!m_engine)
	{
		fmt::throw_exception("LLVM: Failed to create ExecutionEngine: %s", result);
	}
}

jit_compiler::~jit_compiler()
{
}

void jit_compiler::add(std::unique_ptr<llvm::Module> _module, const std::string& path)
{
	ObjectCache cache{path};
	m_engine->setObjectCache(&cache);

	const auto ptr = _module.get();
	m_engine->addModule(std::move(_module));
	m_engine->generateCodeForModule(ptr);
	m_engine->setObjectCache(nullptr);

	for (auto& func : ptr->functions())
	{
		// Delete IR to lower memory consumption
		func.deleteBody();
	}
}

void jit_compiler::add(std::unique_ptr<llvm::Module> _module)
{
	const auto ptr = _module.get();
	m_engine->addModule(std::move(_module));
	m_engine->generateCodeForModule(ptr);

	for (auto& func : ptr->functions())
	{
		// Delete IR to lower memory consumption
		func.deleteBody();
	}
}

void jit_compiler::add(const std::string& path)
{
	auto cache = ObjectCache::load(path);

	if (auto object_file = llvm::object::ObjectFile::createObjectFile(*cache))
	{
		m_engine->addObjectFile( std::move(*object_file) );
	}
	else
	{
		jit_log.error("ObjectCache: Adding failed: %s", path);
	}
}

bool jit_compiler::check(const std::string& path)
{
	if (auto cache = ObjectCache::load(path))
	{
		if (auto object_file = llvm::object::ObjectFile::createObjectFile(*cache))
		{
			return true;
		}

		if (fs::remove_file(path))
		{
			jit_log.error("ObjectCache: Removed damaged file: %s", path);
		}
	}

	return false;
}

void jit_compiler::fin()
{
	m_engine->finalizeObject();
}

u64 jit_compiler::get(const std::string& name)
{
	return m_engine->getGlobalValueAddress(name);
}

#endif
