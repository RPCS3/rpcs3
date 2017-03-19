#ifdef LLVM_AVAILABLE

#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <array>

#include "types.h"
#include "StrFmt.h"
#include "File.h"
#include "Log.h"
#include "VirtualMemory.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

#include "JIT.h"

// Global LLVM context (thread-unsafe)
llvm::LLVMContext g_llvm_ctx;

// Size of virtual memory area reserved: 512 MB
static const u64 s_memory_size = 0x20000000;

// Try to reserve a portion of virtual memory in the first 2 GB address space beforehand, if possible.
static void* const s_memory = []() -> void*
{
	for (u64 addr = 0x10000000; addr <= 0x80000000 - s_memory_size; addr += 0x1000000)
	{
		if (auto ptr = utils::memory_reserve(s_memory_size, (void*)addr))
		{
			return ptr;
		}
	}

	return utils::memory_reserve(s_memory_size);
}();

// Code section
static u8* s_code_addr;
static u64 s_code_size;

// EH frames
static u8* s_unwind_info;
static u64 s_unwind_size;

#ifdef _WIN32
static std::vector<std::vector<RUNTIME_FUNCTION>> s_unwind; // .pdata
#endif

// Helper class
struct MemoryManager final : llvm::RTDyldMemoryManager
{
	std::unordered_map<std::string, std::uintptr_t>& m_link;

	std::array<u8, 16>* m_tramps;

	MemoryManager(std::unordered_map<std::string, std::uintptr_t>& table)
		: m_link(table)
		, m_next(s_memory)
		, m_tramps(nullptr)
	{
	}

	[[noreturn]] static void null()
	{
		fmt::throw_exception("Null function" HERE);
	}

	virtual u64 getSymbolAddress(const std::string& name) override
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
				// It's fine if some function is never called, for example.
				LOG_ERROR(GENERAL, "LLVM: Linkage failed: %s", name);
				addr = (u64)null;
			}
		}

		// Verify address for small code model
		if ((u64)s_memory > 0x80000000 - s_memory_size ? (u64)addr - (u64)s_memory >= s_memory_size : addr >= 0x80000000)
		{
			// Allocate memory for trampolines
			if (!m_tramps)
			{
				m_tramps = reinterpret_cast<decltype(m_tramps)>(m_next);
				utils::memory_commit(m_next, 4096, utils::protection::wx);
				m_next = (u8*)((u64)m_next + 4096);
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

		return addr;
	}

	virtual u8* allocateCodeSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name) override
	{
		// Simple allocation
		const u64 next = ::align((u64)m_next + size, 4096);

		if (next > (u64)s_memory + s_memory_size)
		{
			LOG_FATAL(GENERAL, "LLVM: Out of memory (size=0x%llx, aligned 0x%x)", size, align);
			return nullptr;
		}

		utils::memory_commit(m_next, size, utils::protection::wx);
		s_code_addr = (u8*)m_next;
		s_code_size = size;

		LOG_NOTICE(GENERAL, "LLVM: Code section %u '%s' allocated -> %p (size=0x%llx, aligned 0x%x)", sec_id, sec_name.data(), m_next, size, align);
		return (u8*)std::exchange(m_next, (void*)next);
	}

	virtual u8* allocateDataSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name, bool is_ro) override
	{
		// Simple allocation
		const u64 next = ::align((u64)m_next + size, 4096);

		if (next > (u64)s_memory + s_memory_size)
		{
			LOG_FATAL(GENERAL, "LLVM: Out of memory (size=0x%llx, aligned 0x%x)", size, align);
			return nullptr;
		}

		if (!is_ro)
		{
			LOG_ERROR(GENERAL, "LLVM: Writeable data section not supported!");
		}

		utils::memory_commit(m_next, size);

		LOG_NOTICE(GENERAL, "LLVM: Data section %u '%s' allocated -> %p (size=0x%llx, aligned 0x%x, %s)", sec_id, sec_name.data(), m_next, size, align, is_ro ? "ro" : "rw");
		return (u8*)std::exchange(m_next, (void*)next);
	}

	virtual bool finalizeMemory(std::string* = nullptr) override
	{
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

	virtual void registerEHFrames(u8* addr, u64 load_addr, std::size_t size) override
	{
		s_unwind_info = addr;
		s_unwind_size = size;

		return RTDyldMemoryManager::registerEHFrames(addr, load_addr, size);
	}

	virtual void deregisterEHFrames(u8* addr, u64 load_addr, std::size_t size) override
	{
		LOG_ERROR(GENERAL, "deregisterEHFrames() called"); // Not expected

		return RTDyldMemoryManager::deregisterEHFrames(addr, load_addr, size);
	}

	~MemoryManager()
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
		// TODO: unregister EH frames if necessary
#endif

		utils::memory_decommit(s_memory, s_memory_size);
	}

private:
	void* m_next;
};

// Helper class
struct EventListener final : llvm::JITEventListener
{
	std::string path;

	virtual void NotifyObjectEmitted(const llvm::object::ObjectFile& obj, const llvm::RuntimeDyld::LoadedObjectInfo& inf) override
	{
		if (!path.empty())
		{
			const llvm::StringRef elf = obj.getData();
			fs::file(path, fs::rewrite).write(elf.data(), elf.size());
		}
	}
};

static EventListener s_listener;

jit_compiler::jit_compiler(std::unordered_map<std::string, std::uintptr_t> init_linkage_info, std::string _cpu)
	: m_link(std::move(init_linkage_info))
	, m_cpu(std::move(_cpu))
{
	verify(HERE), s_memory;

	// Initialization
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	LLVMLinkInMCJIT();

	if (m_cpu.empty())
	{
		m_cpu = llvm::sys::getHostCPUName();
	}

	if (m_cpu == "skylake")
	{
		m_cpu = "haswell";
	}

	std::string result;

	m_engine.reset(llvm::EngineBuilder(std::make_unique<llvm::Module>("", g_llvm_ctx))
		.setErrorStr(&result)
		.setMCJITMemoryManager(std::make_unique<MemoryManager>(m_link))
		.setOptLevel(llvm::CodeGenOpt::Aggressive)
		.setCodeModel(llvm::CodeModel::Small)
		.setMCPU(m_cpu)
		.create());

	if (!m_engine)
	{
		fmt::throw_exception("LLVM: Failed to create ExecutionEngine: %s", result);
	}

	m_engine->setProcessAllSections(true); // ???
	m_engine->RegisterJITEventListener(&s_listener);
}

void jit_compiler::load(std::unique_ptr<llvm::Module> module, std::unique_ptr<llvm::object::ObjectFile> object)
{
	s_listener.path.clear();

	auto* module_ptr = module.get();

	m_engine->addModule(std::move(module));
	m_engine->addObjectFile(std::move(object));
	m_engine->finalizeObject();

	m_map.clear();

	for (auto& func : module_ptr->functions())
	{
		const std::string& name = func.getName();

		if (!m_link.count(name))
		{
			// Register compiled function
			m_map[name] = m_engine->getFunctionAddress(name);
		}
	}

	init();
}

void jit_compiler::make(std::unique_ptr<llvm::Module> module, std::string path)
{
	s_listener.path = std::move(path);

	auto* module_ptr = module.get();

	m_engine->addModule(std::move(module));
	m_engine->finalizeObject();

	m_map.clear();

	for (auto& func : module_ptr->functions())
	{
		if (!func.empty())
		{
			const std::string& name = func.getName();

			// Register compiled function
			m_map[name] = m_engine->getFunctionAddress(name);
		}

		// Delete IR to lower memory consumption
		func.deleteBody();
	}

	init();
}

void jit_compiler::init()
{
#ifdef _WIN32
	// Register .xdata UNWIND_INFO (.pdata section is empty for some reason)
	std::set<u64> func_set;

	for (const auto& pair : m_map)
	{
		func_set.emplace(pair.second);
	}

	const u64 base = (u64)s_memory;
	const u8* bits = s_unwind_info;

	std::vector<RUNTIME_FUNCTION> unwind;
	unwind.reserve(m_map.size());

	for (const u64 addr : func_set)
	{
		// Find next function address
		const auto _next = func_set.upper_bound(addr);
		const u64 next = _next != func_set.end() ? *_next : (u64)s_code_addr + s_code_size;

		// Generate RUNTIME_FUNCTION record
		RUNTIME_FUNCTION uw;
		uw.BeginAddress = static_cast<u32>(addr - base);
		uw.EndAddress   = static_cast<u32>(next - base);
		uw.UnwindData   = static_cast<u32>((u64)bits - base);
		unwind.emplace_back(uw);

		// Parse .xdata UNWIND_INFO record
		const u8 flags = *bits++; // Version and flags
		const u8 prolog = *bits++; // Size of prolog
		const u8 count = *bits++; // Count of unwind codes
		const u8 frame = *bits++; // Frame Reg + Off
		bits += ::align(std::max<u8>(1, count), 2) * sizeof(u16); // UNWIND_CODE array

		if (flags != 1) 
		{
			// Can't happen for trivial code
			LOG_ERROR(GENERAL, "LLVM: unsupported UNWIND_INFO version/flags (0x%02x)", flags);
			break;
		}

		LOG_TRACE(GENERAL, "LLVM: .xdata at 0x%llx: function 0x%x..0x%x: p0x%02x, c0x%02x, f0x%02x", uw.UnwindData + base, uw.BeginAddress + base, uw.EndAddress + base, prolog, count, frame);
	}

	if (s_unwind_info + s_unwind_size != bits)
	{
		LOG_ERROR(GENERAL, "LLVM: .xdata analysis failed! (%p != %p)", s_unwind_info + s_unwind_size, bits);
	}
	else if (!RtlAddFunctionTable(unwind.data(), (DWORD)unwind.size(), base))
	{
		LOG_ERROR(GENERAL, "RtlAddFunctionTable(%p) failed! Error %u", s_unwind_info, GetLastError());
	}
	else
	{
		LOG_NOTICE(GENERAL, "LLVM: UNWIND_INFO registered (%p, size=0x%llx)", s_unwind_info, s_unwind_size);
	}

	s_unwind.emplace_back(std::move(unwind));
#else
	// TODO: register EH frames if necessary
#endif
}

jit_compiler::~jit_compiler()
{
}

#endif
