#include "JIT.h"

#ifndef _XABORT_RETRY
#define _XABORT_RETRY (1 << 1)
#endif

asmjit::JitRuntime& asmjit::get_global_runtime()
{
	// Magic static
	static asmjit::JitRuntime g_rt;
	return g_rt;
}

asmjit::Label asmjit::build_transaction_enter(asmjit::X86Assembler& c, asmjit::Label fallback)
{
	Label fall = c.newLabel();
	Label begin = c.newLabel();
	c.jmp(begin);
	c.bind(fall);
	c.test(x86::eax, _XABORT_RETRY);
	c.jz(fallback);
	c.align(kAlignCode, 16);
	c.bind(begin);
	c.xbegin(fall);
	return begin;
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

#include "types.h"
#include "StrFmt.h"
#include "File.h"
#include "Log.h"
#include "mutex.h"
#include "sysinfo.h"
#include "VirtualMemory.h"

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
			writer_lock lock(s_mutex);

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
		writer_lock lock(s_mutex);

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
		writer_lock lock(s_mutex);

		// Simple allocation
		const u64 next = ::align((u64)s_next + size, 4096);

		if (next > (u64)s_memory + s_memory_size)
		{
			LOG_FATAL(GENERAL, "LLVM: Out of memory (size=0x%llx, aligned 0x%x)", size, align);
			return nullptr;
		}

		if (!is_ro)
		{
			LOG_ERROR(GENERAL, "LLVM: Writeable data section not supported!");
		}

		utils::memory_commit(s_next, size);

		LOG_NOTICE(GENERAL, "LLVM: Data section %u '%s' allocated -> %p (size=0x%llx, aligned 0x%x, %s)", sec_id, sec_name.data(), s_next, size, align, is_ro ? "ro" : "rw");
		return (u8*)std::exchange(s_next, (void*)next);
	}

	bool finalizeMemory(std::string* = nullptr) override
	{
		// Lock memory manager
		writer_lock lock(s_mutex);

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
		writer_lock lock(s_mutex);

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

		return RTDyldMemoryManager::registerEHFrames(addr, load_addr, size);
	}

	void deregisterEHFrames() override
	{
	}
};

// Simple memory manager
struct MemoryManager2 : llvm::RTDyldMemoryManager
{
	// Reserve 2 GiB
	void* const m_memory = utils::memory_reserve(0x80000000);

	u8* const m_code = static_cast<u8*>(m_memory) + 0x00000000;
	u8* const m_data = static_cast<u8*>(m_memory) + 0x40000000;

	u64 m_code_pos = 0;
	u64 m_data_pos = 0;

	MemoryManager2() = default;

	~MemoryManager2() override
	{
		utils::memory_release(m_memory, 0x80000000);
	}

	u8* allocateCodeSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name) override
	{
		// Simple allocation
		const u64 old = m_code_pos;
		const u64 pos = ::align(m_code_pos, align);
		m_code_pos = ::align(pos + size, align);

		if (m_code_pos > 0x40000000)
		{
			LOG_FATAL(GENERAL, "LLVM: Out of code memory (size=0x%x, align=0x%x)", size, align);
			return nullptr;
		}

		const u64 olda = ::align(old, 0x10000);
		const u64 newa = ::align(m_code_pos, 0x10000);

		if (olda != newa)
		{
			// Commit more memory
			utils::memory_commit(m_code + olda, newa - olda, utils::protection::wx);
		}

		LOG_NOTICE(GENERAL, "LLVM: Code section %u '%s' allocated -> %p (size=0x%x, align=0x%x)", sec_id, sec_name.data(), m_code + pos, size, align);
		return m_code + pos;
	}

	u8* allocateDataSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name, bool is_ro) override
	{
		// Simple allocation
		const u64 old = m_data_pos;
		const u64 pos = ::align(m_data_pos, align);
		m_data_pos = ::align(pos + size, align);

		if (m_data_pos > 0x40000000)
		{
			LOG_FATAL(GENERAL, "LLVM: Out of data memory (size=0x%x, align=0x%x)", size, align);
			return nullptr;
		}

		const u64 olda = ::align(old, 0x10000);
		const u64 newa = ::align(m_data_pos, 0x10000);

		if (olda != newa)
		{
			// Commit more memory
			utils::memory_commit(m_data + olda, newa - olda);
		}

		LOG_NOTICE(GENERAL, "LLVM: Data section %u '%s' allocated -> %p (size=0x%x, align=0x%x, %s)", sec_id, sec_name.data(), m_data + pos, size, align, is_ro ? "ro" : "rw");
		return m_data + pos;
	}

	bool finalizeMemory(std::string* = nullptr) override
	{
		return false;
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

	void NotifyObjectEmitted(const llvm::object::ObjectFile& obj, const llvm::RuntimeDyld::LoadedObjectInfo& inf) override
	{
#ifdef _WIN32
		for (auto it = obj.section_begin(), end = obj.section_end(); it != end; ++it)
		{
			llvm::StringRef name;
			it->getName(name);

			if (name == ".pdata")
			{
				llvm::StringRef data;
				it->getContents(data);

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
				writer_lock lock(s_mutex);

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
			return std::move(buf);
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
			m_cpu == "cannonlake" ||
			m_cpu == "icelake")
		{
			// Downgrade if AVX is not supported by some chips
			if (!utils::has_avx())
			{
				m_cpu = "nehalem";
			}
		}

		if (m_cpu == "skylake-avx512" ||
			m_cpu == "cannonlake" ||
			m_cpu == "icelake")
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

jit_compiler::jit_compiler(const std::unordered_map<std::string, u64>& _link, const std::string& _cpu, bool large)
	: m_link(_link)
	, m_cpu(cpu(_cpu))
{
	std::string result;

	if (m_link.empty())
	{
		// Auxiliary JIT (does not use custom memory manager, only writes the objects)
		m_engine.reset(llvm::EngineBuilder(std::make_unique<llvm::Module>("null_", m_context))
			.setErrorStr(&result)
			.setEngineKind(llvm::EngineKind::JIT)
			.setMCJITMemoryManager(std::make_unique<MemoryManager2>())
			.setOptLevel(llvm::CodeGenOpt::Aggressive)
			.setCodeModel(large ? llvm::CodeModel::Large : llvm::CodeModel::Small)
			.setMCPU(m_cpu)
			.create());
	}
	else
	{
		// Primary JIT
		auto mem = std::make_unique<MemoryManager>(m_link);
		m_jit_el = std::make_unique<EventListener>(*mem);

		m_engine.reset(llvm::EngineBuilder(std::make_unique<llvm::Module>("null", m_context))
			.setErrorStr(&result)
			.setEngineKind(llvm::EngineKind::JIT)
			.setMCJITMemoryManager(std::move(mem))
			.setOptLevel(llvm::CodeGenOpt::Aggressive)
			.setCodeModel(large ? llvm::CodeModel::Large : llvm::CodeModel::Small)
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

bool jit_compiler::has_ssse3() const
{
	if (m_cpu == "generic" ||
		m_cpu == "k8" ||
		m_cpu == "opteron" ||
		m_cpu == "athlon64" ||
		m_cpu == "athlon-fx" ||
		m_cpu == "k8-sse3" ||
		m_cpu == "opteron-sse3" ||
		m_cpu == "athlon64-sse3" ||
		m_cpu == "amdfam10" ||
		m_cpu == "barcelona")
	{
		return false;
	}

	return true;
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
	m_engine->addObjectFile(std::move(llvm::object::ObjectFile::createObjectFile(*ObjectCache::load(path)).get()));
}

void jit_compiler::fin()
{
	m_engine->finalizeObject();
}

u64 jit_compiler::get(const std::string& name)
{
	return m_engine->getGlobalValueAddress(name);
}

std::unordered_map<std::string, u64> jit_compiler::add(std::unordered_map<std::string, std::string> data)
{
	// Lock memory manager
	writer_lock lock(s_mutex);

	std::unordered_map<std::string, u64> result;

	std::size_t size = 0;

	for (auto&& pair : data)
	{
		size += ::align(pair.second.size(), 16);
	}

	utils::memory_commit(s_next, size, utils::protection::wx);
	std::memset(s_next, 0xc3, ::align(size, 4096));

	for (auto&& pair : data)
	{
		std::memcpy(s_next, pair.second.data(), pair.second.size());
		result.emplace(pair.first, (u64)s_next);
		s_next = (void*)::align((u64)s_next + pair.second.size(), 16);
	}

	s_next = (void*)::align((u64)s_next, 4096);

	return result;
}

#endif
