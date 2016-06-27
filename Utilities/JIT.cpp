#ifdef LLVM_AVAILABLE

#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <array>

#include "types.h"
#include "Macro.h"
#include "StrFmt.h"
#include "File.h"
#include "Log.h"

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
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#endif

#include "JIT.h"

// Global LLVM context (thread-unsafe)
llvm::LLVMContext g_llvm_ctx;

// Size of virtual memory area reserved: 512 MB
static const u64 s_memory_size = 0x20000000;

// Try to reserve a portion of virtual memory in the first 2 GB address space beforehand, if possible.
static void* const s_memory = []() -> void*
{
#ifdef _WIN32
	for (u64 addr = 0x1000000; addr <= 0x60000000; addr += 0x1000000)
	{
		if (VirtualAlloc((void*)addr, s_memory_size, MEM_RESERVE, PAGE_NOACCESS))
		{
			return (void*)addr;
		}
	}

	return VirtualAlloc(NULL, s_memory_size, MEM_RESERVE, PAGE_NOACCESS);
#else
	return ::mmap((void*)0x10000000, s_memory_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
}();

// EH frames
static u8* s_unwind_info;
static u64 s_unwind_size;

#ifdef _WIN32
static std::vector<RUNTIME_FUNCTION> s_unwind; // Custom .pdata section replacement
#endif

// Helper class
struct MemoryManager final : llvm::RTDyldMemoryManager
{
	std::unordered_map<std::string, std::uintptr_t> table;

	MemoryManager(std::unordered_map<std::string, std::uintptr_t>&& table)
		: table(std::move(table))
	{
	}

	[[noreturn]] static void null()
	{
		throw std::runtime_error("Null function" HERE);
	}

	virtual u64 getSymbolAddress(const std::string& name) override
	{
		if (u64 addr = RTDyldMemoryManager::getSymbolAddress(name))
		{
			// This may be bad if LLVM requests some built-in functions like fma.
			LOG_ERROR(GENERAL, "LLVM: Symbol requested %s -> 0x%016llx", name, addr);
			return addr;
		}

		const auto found = table.find(name);

		if (found != table.end())
		{
			return found->second;
		}

		// It's fine if some function is never called, for example.
		LOG_ERROR(GENERAL, "LLVM: Linkage failed for %s", name);
		return (u64)null;
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

#ifdef _WIN32
		if (!VirtualAlloc(m_next, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE))
#else
		if (::mprotect(m_next, size, PROT_READ | PROT_WRITE | PROT_EXEC))
#endif
		{
			LOG_FATAL(GENERAL, "LLVM: Failed to allocate memory at 0x%p", m_next);
			return nullptr;
		}

		LOG_SUCCESS(GENERAL, "LLVM: Code section %u '%s' allocated -> 0x%p (size=0x%llx, aligned 0x%x)", sec_id, sec_name.data(), m_next, size, align);
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

#ifdef _WIN32
		if (!VirtualAlloc(m_next, size, MEM_COMMIT, PAGE_READWRITE))
#else
		if (::mprotect(m_next, size, PROT_READ | PROT_WRITE))
#endif
		{
			LOG_FATAL(GENERAL, "LLVM: Failed to allocate memory at 0x%p", m_next);
			return nullptr;
		}

		LOG_SUCCESS(GENERAL, "LLVM: Data section %u '%s' allocated -> 0x%p (size=0x%llx, aligned 0x%x, %s)", sec_id, sec_name.data(), m_next, size, align, is_ro ? "ro" : "rw");
		return (u8*)std::exchange(m_next, (void*)next);
	}

	virtual bool finalizeMemory(std::string* = nullptr) override
	{
		// TODO: make sections read-only when necessary
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
		if (!RtlDeleteFunctionTable(s_unwind.data()))
		{
			LOG_FATAL(GENERAL, "RtlDeleteFunctionTable(addr=0x%p) failed! Error %u", s_unwind_info, GetLastError());
		}

		if (!VirtualFree(s_memory, 0, MEM_DECOMMIT))
		{
			LOG_FATAL(GENERAL, "VirtualFree(0x%p) failed! Error %u", s_memory, GetLastError());
		}
#else
		if (::mprotect(s_memory, s_memory_size, PROT_NONE))
		{
			LOG_FATAL(GENERAL, "mprotect(0x%p) failed! Error %d", s_memory, errno);
		}

		// TODO: unregister EH frames if necessary
#endif
	}

private:
	void* m_next = s_memory;
};

// Helper class
struct EventListener final : llvm::JITEventListener
{
	virtual void NotifyObjectEmitted(const llvm::object::ObjectFile& obj, const llvm::RuntimeDyld::LoadedObjectInfo& inf) override
	{
		const llvm::StringRef elf = obj.getData();
		fs::file(fs::get_config_dir() + "LLVM.obj", fs::rewrite)
			.write(elf.data(), elf.size());
	}
};

static EventListener s_listener;

jit_compiler::jit_compiler(std::unique_ptr<llvm::Module>&& _module, std::unordered_map<std::string, std::uintptr_t>&& table)
{
	EXPECTS(s_memory);

	std::string result;

	const auto module_ptr = _module.get();

	// Initialization
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	LLVMLinkInMCJIT();

	m_engine.reset(llvm::EngineBuilder(std::move(_module))
		.setErrorStr(&result)
		.setMCJITMemoryManager(std::make_unique<MemoryManager>(std::move(table)))
		.setOptLevel(llvm::CodeGenOpt::Aggressive)
		.setRelocationModel(llvm::Reloc::PIC_)
		.setCodeModel((u64)s_memory <= 0x60000000 ? llvm::CodeModel::Medium : llvm::CodeModel::Large) // TODO
		.setMCPU(llvm::sys::getHostCPUName())
		.create());

	if (!m_engine)
	{
		throw fmt::exception("LLVM: Failed to create ExecutionEngine: %s", result);
	}

	m_engine->setProcessAllSections(true); // ???
	m_engine->RegisterJITEventListener(&s_listener);
	m_engine->finalizeObject();

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

#ifdef _WIN32
	// Register .xdata UNWIND_INFO (.pdata section is empty for some reason)
	std::set<u64> func_set;

	for (const auto& pair : m_map)
	{
		func_set.emplace(pair.second);
	}

	// Hack (cannot obtain last function size)
	func_set.emplace(::align(*--func_set.end() + 4096, 4096));

	const u64 base = (u64)s_memory;
	const u8* bits = s_unwind_info;

	s_unwind.clear();
	s_unwind.reserve(m_map.size());

	for (auto it = func_set.begin(), end = --func_set.end(); it != end; it++)
	{
		const u64 addr = *it;
		const u64 next = *func_set.upper_bound(addr);

		// Generate RUNTIME_FUNCTION record
		RUNTIME_FUNCTION uw;
		uw.BeginAddress = static_cast<u32>(addr - base);
		uw.EndAddress   = static_cast<u32>(next - base);
		uw.UnwindData   = static_cast<u32>((u64)bits - base);
		s_unwind.emplace_back(uw);

		// Parse .xdata UNWIND_INFO record
		const u8 flags = *bits++; // Version and flags
		const u8 prolog = *bits++; // Size of prolog
		const u8 count = *bits++; // Count of unwind codes
		const u8 frame = *bits++; // Frame Reg + Off
		bits += ::align(std::max<u8>(1, count), 2) * sizeof(u16); // UNWIND_CODE array

		if (flags != 1) 
		{
			LOG_ERROR(GENERAL, "LLVM: unsupported UNWIND_INFO version/flags (0x%02x)", flags);
			break;
		}

		LOG_TRACE(GENERAL, "LLVM: .xdata at 0x%llx: function 0x%x..0x%x: p0x%02x, c0x%02x, f0x%02x", uw.UnwindData + base, uw.BeginAddress + base, uw.EndAddress + base, prolog, count, frame);
	}

	if (s_unwind_info + s_unwind_size != bits)
	{
		LOG_FATAL(GENERAL, "LLVM: .xdata analysis failed! (0x%p != 0x%p)", s_unwind_info + s_unwind_size, bits);
	}
	else if (!RtlAddFunctionTable(s_unwind.data(), (DWORD)s_unwind.size(), base))
	{
		LOG_FATAL(GENERAL, "RtlAddFunctionTable(addr=0x%p) failed! Error %u", s_unwind_info, GetLastError());
	}
	else
	{
		LOG_SUCCESS(GENERAL, "LLVM: UNWIND_INFO registered (addr=0x%p, size=0x%llx)", s_unwind_info, s_unwind_size);
	}
#endif
}

jit_compiler::~jit_compiler()
{
}

#endif
