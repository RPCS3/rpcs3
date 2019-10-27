#include "types.h"
#include "JIT.h"
#include "StrFmt.h"
#include "File.h"
#include "Log.h"
#include "mutex.h"
#include "sysinfo.h"
#include "VirtualMemory.h"
#include <immintrin.h>

#ifdef __linux__
#include <sys/mman.h>
#define CAN_OVERCOMMIT
#endif

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
static u8* add_jit_memory(std::size_t size, uint align)
{
	// Select subrange
	u8* pointer = get_jit_memory() + Off;

	if (UNLIKELY(!size && !align))
	{
		// Return subrange info
		return pointer;
	}

	u64 olda, newa;

	// Simple allocation by incrementing pointer to the next free data
	const u64 pos = Ctr.atomic_op([&](u64& ctr) -> u64
	{
		const u64 _pos = ::align(ctr & 0xffff'ffff, align);
		const u64 _new = ::align(_pos + size, align);

		if (UNLIKELY(_new > 0x40000000))
		{
			// Sorry, we failed, and further attempts should fail too.
			ctr |= 0x40000000;
			return -1;
		}

		// Last allocation is stored in highest bits
		olda = ctr >> 32;
		newa = olda;

		// Check the necessity to commit more memory
		if (UNLIKELY(_new > olda))
		{
			newa = ::align(_new, 0x100000);
		}

		ctr += _new - (ctr & 0xffff'ffff);
		return _pos;
	});

	if (UNLIKELY(pos == -1))
	{
		LOG_WARNING(GENERAL, "JIT: Out of memory (size=0x%x, align=0x%x, off=0x%x)", size, align, Off);
		return nullptr;
	}

	if (UNLIKELY(olda != newa))
	{
#ifdef CAN_OVERCOMMIT
		madvise(pointer + olda, newa - olda, MADV_WILLNEED);
#else
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

jit_runtime::jit_runtime()
	: HostRuntime()
{
}

jit_runtime::~jit_runtime()
{
}

asmjit::Error jit_runtime::_add(void** dst, asmjit::CodeHolder* code) noexcept
{
	std::size_t codeSize = code->getCodeSize();
	if (UNLIKELY(!codeSize))
	{
		*dst = nullptr;
		return asmjit::kErrorNoCodeGenerated;
	}

	void* p = jit_runtime::alloc(codeSize, 16);
	if (UNLIKELY(!p))
	{
		*dst = nullptr;
		return asmjit::kErrorNoVirtualMemory;
	}

	std::size_t relocSize = code->relocate(p);
	if (UNLIKELY(!relocSize))
	{
		*dst = nullptr;
		return asmjit::kErrorInvalidState;
	}

	flush(p, relocSize);
	*dst = p;

	return asmjit::kErrorOk;
}

asmjit::Error jit_runtime::_release(void* ptr) noexcept
{
	return asmjit::kErrorOk;
}

u8* jit_runtime::alloc(std::size_t size, uint align, bool exec) noexcept
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
}

asmjit::JitRuntime& asmjit::get_global_runtime()
{
	// Magic static
	static asmjit::JitRuntime g_rt;
	return g_rt;
}

void asmjit::build_transaction_enter(asmjit::X86Assembler& c, asmjit::Label fallback, const asmjit::X86Gp& ctr, uint less_than)
{
	Label fall = c.newLabel();
	Label begin = c.newLabel();
	c.jmp(begin);
	c.bind(fall);

	if (less_than < 65)
	{
		c.add(ctr, 1);
		c.test(x86::eax, _XABORT_RETRY);
		c.jz(fallback);
	}
	else
	{
		// Don't repeat on explicit XABORT instruction (workaround)
		c.test(x86::eax, _XABORT_EXPLICIT);
		c.jnz(fallback);

		// Count an attempt without RETRY flag as 65 normal attempts and continue
		c.push(x86::rax);
		c.not_(x86::eax);
		c.and_(x86::eax, _XABORT_RETRY);
		c.shl(x86::eax, 5);
		c.add(x86::eax, 1); // eax = RETRY ? 1 : 65
		c.add(ctr, x86::rax);
		c.pop(x86::rax);
	}

	c.cmp(ctr, less_than);
	c.jae(fallback);
	c.align(kAlignCode, 16);
	c.bind(begin);
	c.xbegin(fall);
}

void asmjit::build_transaction_abort(asmjit::X86Assembler& c, unsigned char code)
{
	c.db(0xc6);
	c.db(0xf8);
	c.db(code);
}

#ifdef LLVM_AVAILABLE

#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <array>
#include <deque>

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/ExecutionEngine/ObjectCache.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#endif

// Memory manager mutex
shared_mutex s_mutex;

// Size of virtual memory area reserved: 512 MB
static const u64 s_memory_size = 0x20000000;

// Try to reserve a portion of virtual memory in the first 2 GB address space beforehand, if possible.
static void* const s_memory = []() -> void*
{
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();
	LLVMLinkInMCJIT();

#ifdef MAP_32BIT
	auto ptr = ::mmap(nullptr, s_memory_size, PROT_NONE, MAP_ANON | MAP_PRIVATE | MAP_32BIT, -1, 0);
	if (ptr != MAP_FAILED)
		return ptr;
#else
	for (u64 addr = 0x10000000; addr <= 0x80000000 - s_memory_size; addr += 0x1000000)
	{
		if (auto ptr = utils::memory_reserve(s_memory_size, (void*)addr))
		{
			return ptr;
		}
	}
#endif

	return utils::memory_reserve(s_memory_size);
}();

static void* s_next = s_memory;

#ifdef _WIN32
static std::deque<std::vector<RUNTIME_FUNCTION>> s_unwater;
static std::vector<std::vector<RUNTIME_FUNCTION>> s_unwind; // .pdata
#else
static std::deque<std::pair<u8*, std::size_t>> s_unfire;
#endif

// Reset memory manager
extern void jit_finalize()
{
#ifdef _WIN32
	for (auto&& unwind : s_unwind)
	{
		if (!RtlDeleteFunctionTable(unwind.data()))
		{
			LOG_FATAL(GENERAL, "RtlDeleteFunctionTable() failed! Error %u", GetLastError());
		}
	}

	s_unwind.clear();
#else
	for (auto&& t : s_unfire)
	{
		llvm::RTDyldMemoryManager::deregisterEHFramesInProcess(t.first, t.second);
	}

	s_unfire.clear();
#endif

	utils::memory_decommit(s_memory, s_memory_size);

	s_next = s_memory;
}

// Helper class
struct MemoryManager : llvm::RTDyldMemoryManager
{
	std::unordered_map<std::string, u64>& m_link;

	std::array<u8, 16>* m_tramps{};

	u8* m_code_addr{}; // TODO

	MemoryManager(std::unordered_map<std::string, u64>& table)
		: m_link(table)
	{
	}

	[[noreturn]] static void null()
	{
		fmt::throw_exception("Null function" HERE);
	}

	llvm::JITSymbol findSymbol(const std::string& name) override
	{
		auto& addr = m_link[name];

		// Find function address
		if (!addr)
		{
			addr = RTDyldMemoryManager::getSymbolAddress(name);

			if (addr)
			{
				LOG_WARNING(GENERAL, "LLVM: Symbol requested: %s -> 0x%016llx", name, addr);
			}
			else
			{
				LOG_ERROR(GENERAL, "LLVM: Linkage failed: %s", name);
				addr = (u64)null;
			}
		}

		// Verify address for small code model
		if ((u64)s_memory > 0x80000000 - s_memory_size ? (u64)addr - (u64)s_memory >= s_memory_size : addr >= 0x80000000)
		{
			// Lock memory manager
			std::lock_guard lock(s_mutex);

			// Allocate memory for trampolines
			if (!m_tramps)
			{
				m_tramps = reinterpret_cast<decltype(m_tramps)>(s_next);
				utils::memory_commit(s_next, 4096, utils::protection::wx);
				s_next = (u8*)((u64)s_next + 4096);
			}

			// Create a trampoline
			auto& data = *m_tramps++;
			data[0x0] = 0xff; // JMP [rip+2]
			data[0x1] = 0x25;
			data[0x2] = 0x02;
			data[0x3] = 0x00;
			data[0x4] = 0x00;
			data[0x5] = 0x00;
			data[0x6] = 0x48; // MOV rax, imm64 (not executed)
			data[0x7] = 0xb8;
			std::memcpy(data.data() + 8, &addr, 8);
			addr = (u64)&data;

			// Reset pointer (memory page exhausted)
			if (((u64)m_tramps % 4096) == 0)
			{
				m_tramps = nullptr;
			}
		}

		return {addr, llvm::JITSymbolFlags::Exported};
	}

	u8* allocateCodeSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name) override
	{
		// Lock memory manager
		std::lock_guard lock(s_mutex);

		// Simple allocation
		const u64 next = ::align((u64)s_next + size, 4096);

		if (next > (u64)s_memory + s_memory_size)
		{
			LOG_FATAL(GENERAL, "LLVM: Out of memory (size=0x%llx, aligned 0x%x)", size, align);
			return nullptr;
		}

		utils::memory_commit(s_next, size, utils::protection::wx);
		m_code_addr = (u8*)s_next;

		LOG_NOTICE(GENERAL, "LLVM: Code section %u '%s' allocated -> %p (size=0x%llx, aligned 0x%x)", sec_id, sec_name.data(), s_next, size, align);
		return (u8*)std::exchange(s_next, (void*)next);
	}

	u8* allocateDataSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name, bool is_ro) override
	{
		// Lock memory manager
		std::lock_guard lock(s_mutex);

		// Simple allocation
		const u64 next = ::align((u64)s_next + size, 4096);

		if (next > (u64)s_memory + s_memory_size)
		{
			LOG_FATAL(GENERAL, "LLVM: Out of memory (size=0x%llx, aligned 0x%x)", size, align);
			return nullptr;
		}

		if (!is_ro)
		{
		}

		utils::memory_commit(s_next, size);

		LOG_NOTICE(GENERAL, "LLVM: Data section %u '%s' allocated -> %p (size=0x%llx, aligned 0x%x, %s)", sec_id, sec_name.data(), s_next, size, align, is_ro ? "ro" : "rw");
		return (u8*)std::exchange(s_next, (void*)next);
	}

	bool finalizeMemory(std::string* = nullptr) override
	{
		// Lock memory manager
		std::lock_guard lock(s_mutex);

		// TODO: make only read-only sections read-only
//#ifdef _WIN32
//		DWORD op;
//		VirtualProtect(s_memory, (u64)m_next - (u64)s_memory, PAGE_READONLY, &op);
//		VirtualProtect(s_code_addr, s_code_size, PAGE_EXECUTE_READ, &op);
//#else
//		::mprotect(s_memory, (u64)m_next - (u64)s_memory, PROT_READ);
//		::mprotect(s_code_addr, s_code_size, PROT_READ | PROT_EXEC);
//#endif
		return false;
	}

	void registerEHFrames(u8* addr, u64 load_addr, std::size_t size) override
	{
#ifdef _WIN32
		// Lock memory manager
		std::lock_guard lock(s_mutex);

		// Use s_memory as a BASE, compute the difference
		const u64 unwind_diff = (u64)addr - (u64)s_memory;

		// Fix RUNTIME_FUNCTION records (.pdata section)
		auto pdata = std::move(s_unwater.front());
		s_unwater.pop_front();

		for (auto& rf : pdata)
		{
			rf.UnwindData += static_cast<DWORD>(unwind_diff);
		}

		// Register .xdata UNWIND_INFO structs
		if (!RtlAddFunctionTable(pdata.data(), (DWORD)pdata.size(), (u64)s_memory))
		{
			LOG_ERROR(GENERAL, "RtlAddFunctionTable() failed! Error %u", GetLastError());
		}
		else
		{
			s_unwind.emplace_back(std::move(pdata));
		}
#else
		s_unfire.push_front(std::make_pair(addr, size));
#endif

		return RTDyldMemoryManager::registerEHFramesInProcess(addr, size);
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

	u8* allocateCodeSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name) override
	{
		return jit_runtime::alloc(size, align, true);
	}

	u8* allocateDataSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name, bool is_ro) override
	{
		return jit_runtime::alloc(size, align, false);
	}

	bool finalizeMemory(std::string* = nullptr) override
	{
		return false;
	}

	void registerEHFrames(u8* addr, u64 load_addr, std::size_t size) override
	{
#ifndef _WIN32
		RTDyldMemoryManager::registerEHFramesInProcess(addr, size);
		s_unfire.push_front(std::make_pair(addr, size));
#endif
	}

	void deregisterEHFrames() override
	{
	}
};

// Simple memory manager. I promise there will be no MemoryManager4.
struct MemoryManager3 : llvm::RTDyldMemoryManager
{
	std::vector<std::pair<u8*, std::size_t>> allocs;

	MemoryManager3() = default;

	~MemoryManager3() override
	{
		for (auto& a : allocs)
		{
			utils::memory_release(a.first, a.second);
		}
	}

	u8* allocateCodeSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name) override
	{
		u8* r = static_cast<u8*>(utils::memory_reserve(size));
		utils::memory_commit(r, size, utils::protection::wx);
		allocs.emplace_back(r, size);
		return r;
	}

	u8* allocateDataSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name, bool is_ro) override
	{
		u8* r = static_cast<u8*>(utils::memory_reserve(size));
		utils::memory_commit(r, size);
		allocs.emplace_back(r, size);
		return r;
	}

	bool finalizeMemory(std::string* = nullptr) override
	{
		return false;
	}

	void registerEHFrames(u8* addr, u64 load_addr, std::size_t size) override
	{
	}

	void deregisterEHFrames() override
	{
	}
};

// Helper class
struct EventListener : llvm::JITEventListener
{
	MemoryManager& m_mem;

	EventListener(MemoryManager& mem)
		: m_mem(mem)
	{
	}

	void notifyObjectLoaded(ObjectKey K, const llvm::object::ObjectFile& obj, const llvm::RuntimeDyld::LoadedObjectInfo& inf) override
	{
#ifdef _WIN32
		for (auto it = obj.section_begin(), end = obj.section_end(); it != end; ++it)
		{
			llvm::StringRef name;
			name = it->getName().get();

			if (name == ".pdata")
			{
				llvm::StringRef data;
				data = it->getContents().get();

				std::vector<RUNTIME_FUNCTION> rfs(data.size() / sizeof(RUNTIME_FUNCTION));

				auto offsets = reinterpret_cast<DWORD*>(rfs.data());

				// Initialize .pdata section using relocation info
				for (auto ri = it->relocation_begin(), end = it->relocation_end(); ri != end; ++ri)
				{
					if (ri->getType() == 3 /*R_X86_64_GOT32*/)
					{
						const u64 value = *reinterpret_cast<const DWORD*>(data.data() + ri->getOffset());
						offsets[ri->getOffset() / sizeof(DWORD)] = static_cast<DWORD>(value + ri->getSymbol()->getAddress().get());
					}
				}

				// Lock memory manager
				std::lock_guard lock(s_mutex);

				// Use s_memory as a BASE, compute the difference
				const u64 code_diff = (u64)m_mem.m_code_addr - (u64)s_memory;

				// Fix RUNTIME_FUNCTION records (.pdata section)
				for (auto& rf : rfs)
				{
					rf.BeginAddress += static_cast<DWORD>(code_diff);
					rf.EndAddress   += static_cast<DWORD>(code_diff);
				}

				s_unwater.emplace_back(std::move(rfs));
			}
		}
#endif
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

	void notifyObjectCompiled(const llvm::Module* module, llvm::MemoryBufferRef obj) override
	{
		std::string name = m_path;
		name.append(module->getName());
		fs::file(name, fs::rewrite).write(obj.getBufferStart(), obj.getBufferSize());
		LOG_NOTICE(GENERAL, "LLVM: Created module: %s", module->getName().data());
	}

	static std::unique_ptr<llvm::MemoryBuffer> load(const std::string& path)
	{
		if (fs::file cached{path, fs::read})
		{
			auto buf = llvm::WritableMemoryBuffer::getNewUninitMemBuffer(cached.size());
			cached.read(buf->getBufferStart(), buf->getBufferSize());
			return buf;
		}

		return nullptr;
	}

	std::unique_ptr<llvm::MemoryBuffer> getObject(const llvm::Module* module) override
	{
		std::string path = m_path;
		path.append(module->getName());

		if (auto buf = load(path))
		{
			LOG_NOTICE(GENERAL, "LLVM: Loaded module: %s", module->getName().data());
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
		m_cpu = llvm::sys::getHostCPUName();

		if (m_cpu == "sandybridge" ||
			m_cpu == "ivybridge" ||
			m_cpu == "haswell" ||
			m_cpu == "broadwell" ||
			m_cpu == "skylake" ||
			m_cpu == "skylake-avx512" ||
			m_cpu == "cascadelake" ||
			m_cpu == "cannonlake" ||
			m_cpu == "icelake" ||
			m_cpu == "icelake-client" ||
			m_cpu == "icelake-server")
		{
			// Downgrade if AVX is not supported by some chips
			if (!utils::has_avx())
			{
				m_cpu = "nehalem";
			}
		}

		if (m_cpu == "skylake-avx512" ||
			m_cpu == "cascadelake" ||
			m_cpu == "cannonlake" ||
			m_cpu == "icelake" ||
			m_cpu == "icelake-client" ||
			m_cpu == "icelake-server")
		{
			// Downgrade if AVX-512 is disabled or not supported
			if (!utils::has_512())
			{
				m_cpu = "skylake";
			}
		}
	}

	return m_cpu;
}

jit_compiler::jit_compiler(const std::unordered_map<std::string, u64>& _link, const std::string& _cpu, u32 flags)
	: m_link(_link)
	, m_cpu(cpu(_cpu))
{
	std::string result;

	auto null_mod = std::make_unique<llvm::Module> ("null_", m_context);

	if (m_link.empty())
	{
		std::unique_ptr<llvm::RTDyldMemoryManager> mem;

		if (flags & 0x1)
		{
			mem = std::make_unique<MemoryManager3>();
		}
		else
		{
			mem = std::make_unique<MemoryManager2>();
			null_mod->setTargetTriple(llvm::Triple::normalize("x86_64-unknown-linux-gnu"));
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
		auto mem = std::make_unique<MemoryManager>(m_link);
		m_jit_el = std::make_unique<EventListener>(*mem);

		m_engine.reset(llvm::EngineBuilder(std::move(null_mod))
			.setErrorStr(&result)
			.setEngineKind(llvm::EngineKind::JIT)
			.setMCJITMemoryManager(std::move(mem))
			.setOptLevel(llvm::CodeGenOpt::Aggressive)
			.setCodeModel(flags & 0x2 ? llvm::CodeModel::Large : llvm::CodeModel::Small)
			.setMCPU(m_cpu)
			.create());

		if (m_engine)
		{
			m_engine->RegisterJITEventListener(m_jit_el.get());
		}
	}

	if (!m_engine)
	{
		fmt::throw_exception("LLVM: Failed to create ExecutionEngine: %s", result);
	}
}

jit_compiler::~jit_compiler()
{
}

void jit_compiler::add(std::unique_ptr<llvm::Module> module, const std::string& path)
{
	ObjectCache cache{path};
	m_engine->setObjectCache(&cache);

	const auto ptr = module.get();
	m_engine->addModule(std::move(module));
	m_engine->generateCodeForModule(ptr);
	m_engine->setObjectCache(nullptr);

	for (auto& func : ptr->functions())
	{
		// Delete IR to lower memory consumption
		func.deleteBody();
	}
}

void jit_compiler::add(std::unique_ptr<llvm::Module> module)
{
	const auto ptr = module.get();
	m_engine->addModule(std::move(module));
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
		LOG_ERROR(GENERAL, "ObjectCache: Adding failed: %s", path);
	}
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
