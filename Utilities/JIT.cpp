#include "types.h"
#include "JIT.h"
#include "StrFmt.h"
#include "File.h"
#include "util/logs.hpp"
#include "mutex.h"
#include "sysinfo.h"
#include "VirtualMemory.h"
#include <immintrin.h>
#include <zlib.h>

#ifdef __linux__
#include <sys/mman.h>
#define CAN_OVERCOMMIT
#endif

LOG_CHANNEL(jit_log, "JIT");

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

	if (!size && !align) [[unlikely]]
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
			newa = ::align(_new, 0x100000);
		}

		ctr += _new - (ctr & 0xffff'ffff);
		return _pos;
	});

	if (pos == umax) [[unlikely]]
	{
		jit_log.warning("JIT: Out of memory (size=0x%x, align=0x%x, off=0x%x)", size, align, Off);
		return nullptr;
	}

	if (olda != newa) [[unlikely]]
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
	if (!codeSize) [[unlikely]]
	{
		*dst = nullptr;
		return asmjit::kErrorNoCodeGenerated;
	}

	void* p = jit_runtime::alloc(codeSize, 16);
	if (!p) [[unlikely]]
	{
		*dst = nullptr;
		return asmjit::kErrorNoVirtualMemory;
	}

	std::size_t relocSize = code->relocate(p);
	if (!relocSize) [[unlikely]]
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
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "llvm/ExecutionEngine/ObjectCache.h"
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#endif

class LLVMSegmentAllocator
{
public:
	// Size of virtual memory area reserved: default 512MB
	static constexpr u32 DEFAULT_SEGMENT_SIZE = 0x20000000;

	LLVMSegmentAllocator()
	{
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();
		LLVMLinkInMCJIT();

		// Try to reserve as much virtual memory in the first 2 GB address space beforehand, if possible.
		Segment found_segs[16];
		u32 num_segs = 0;
#ifdef MAP_32BIT
		u64 max_size = 0x80000000u;
		while (num_segs < 16)
		{
			auto ptr = ::mmap(nullptr, max_size, PROT_NONE, MAP_ANON | MAP_PRIVATE | MAP_32BIT, -1, 0);
			if (ptr != reinterpret_cast<void*>(-1))
				found_segs[num_segs++] = Segment(ptr, static_cast<u32>(max_size));
			else if (max_size > 0x1000000)
				max_size -= 0x1000000;
			else
				break;
		}
#else
		u64 start_addr = 0x10000000;
		while (num_segs < 16)
		{
			u64 max_addr = 0;
			u64 max_size = 0x1000000;
			for (u64 addr = start_addr; addr <= (0x80000000u - max_size); addr += 0x1000000)
			{
				for (auto curr_size = max_size; (0x80000000u - curr_size) >= addr; curr_size += 0x1000000)
				{
					if (auto ptr = utils::memory_reserve(curr_size, reinterpret_cast<void*>(addr)))
					{
						if (max_addr == 0 || max_size < curr_size)
						{
							max_addr = addr;
							max_size = curr_size;
						}
						utils::memory_release(ptr, curr_size);
					}
					else
						break;
				}
			}

			if (max_addr == 0)
				break;

			if (auto ptr = utils::memory_reserve(max_size, reinterpret_cast<void*>(max_addr)))
				found_segs[num_segs++] = Segment(ptr, static_cast<u32>(max_size));

			start_addr = max_addr + max_size;
		}
#endif
		if (num_segs)
		{
			if (num_segs > 1)
			{
				m_segs.resize(num_segs);
				for (u32 i = 0; i < num_segs; i++)
					m_segs[i] = found_segs[i];
			}
			else
				m_curr = found_segs[0];

			return;
		}

		if (auto ptr = utils::memory_reserve(DEFAULT_SEGMENT_SIZE))
		{
			m_curr.addr = static_cast<u8*>(ptr);
			m_curr.size = DEFAULT_SEGMENT_SIZE;
			m_curr.used = 0;
		}
	}

	void* allocate(u32 size)
	{
		if (m_curr.remaining() >= size)
			return m_curr.advance(size);

		if (reserve(size))
			return m_curr.advance(size);

		return nullptr;
	}

	bool reserve(u32 size)
	{
		if (size == 0)
			return true;

		store_curr();

		u32 best_idx = UINT_MAX;
		for (u32 i = 0, segs_size = ::size32(m_segs); i < segs_size; i++)
		{
			const auto seg_remaining = m_segs[i].remaining();
			if (seg_remaining < size)
				continue;

			if (best_idx == UINT_MAX || m_segs[best_idx].remaining() > seg_remaining)
				best_idx = i;
		}

		if (best_idx == UINT_MAX)
		{
			const auto size_to_reserve = (size > DEFAULT_SEGMENT_SIZE) ? ::align(size+4096, 4096) : DEFAULT_SEGMENT_SIZE;
			if (auto ptr = utils::memory_reserve(size_to_reserve))
			{
				best_idx = ::size32(m_segs);
				m_segs.emplace_back(ptr, size_to_reserve);
			}
			else
				return false;
		}

		const auto& best_seg = m_segs[best_idx];
		if (best_seg.addr != m_curr.addr)
			m_curr = best_seg;

		return true;
	}

	std::pair<u64, u32> current_segment() const
	{
		return std::make_pair(reinterpret_cast<u64>(m_curr.addr), m_curr.size);
	}

	std::pair<u64, u32> find_segment(u64 addr) const
	{
		for (const auto& seg: m_segs)
		{
			const u64 seg_addr = reinterpret_cast<u64>(seg.addr);
			if (addr < seg_addr)
				continue;

			const auto end_addr = seg_addr + seg.size;
			if (addr < end_addr)
				return std::make_pair(seg_addr, seg.size);
		}

		return std::make_pair(0, 0);
	}

	void reset()
	{
		if (m_segs.empty())
		{
			if (m_curr.addr != nullptr)
			{
				utils::memory_decommit(m_curr.addr, m_curr.size);
				m_curr.used = 0;
			}
			return;
		}

		if (store_curr())
			m_curr = Segment();

		auto allocated_it = std::remove_if(m_segs.begin(), m_segs.end(), [](const Segment& seg)
		{
			return reinterpret_cast<u64>(seg.addr + seg.size) > 0x80000000u;
		});
		if (allocated_it != m_segs.end())
		{
			for (auto it = allocated_it; it != m_segs.end(); ++it)
				utils::memory_release(it->addr, it->size);

			m_segs.erase(allocated_it, m_segs.end());
		}

		for (auto& seg : m_segs)
		{
			utils::memory_decommit(seg.addr, seg.size);
			seg.used = 0;
		}
	}

private:
	bool store_curr()
	{
		if (m_curr.addr != nullptr)
		{
			const auto wanted_addr = m_curr.addr;
			auto existing_it = std::find_if(m_segs.begin(), m_segs.end(), [wanted_addr](const Segment& seg) { return seg.addr == wanted_addr; });
			if (existing_it != m_segs.end())
				existing_it->used = m_curr.used;
			else
				m_segs.push_back(m_curr);

			return true;
		}

		return false;
	}

	struct Segment
	{
		Segment() {}
		Segment(void* addr, u32 size)
			: addr(static_cast<u8*>(addr))
			, size(size)
		{}

		u8* addr = nullptr;
		u32 size = 0;
		u32 used = 0;

		u32 remaining() const
		{
			if (size > used)
				return size - used;

			return 0;
		}
		void* advance(u32 offset)
		{
			const auto prev_used = used;
			used += offset;
			return &addr[prev_used];
		}
	};

	Segment m_curr;
	std::vector<Segment> m_segs;
};

// Memory manager mutex
static shared_mutex s_mutex;
// LLVM Memory allocator
static LLVMSegmentAllocator s_alloc;

#ifdef _WIN32
static std::deque<std::pair<u64, std::vector<RUNTIME_FUNCTION>>> s_unwater;
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
			jit_log.fatal("RtlDeleteFunctionTable() failed! Error %u", GetLastError());
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

	s_alloc.reset();
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
				jit_log.warning("LLVM: Symbol requested: %s -> 0x%016llx", name, addr);
			}
			else
			{
				jit_log.error("LLVM: Linkage failed: %s", name);
				addr = reinterpret_cast<u64>(null);
			}
		}

		// Verify address for small code model
		const u64 code_start = reinterpret_cast<u64>(m_code_addr);
		const s64 addr_diff = addr - code_start;
		if (addr_diff < INT_MIN || addr_diff > INT_MAX)
		{
			// Lock memory manager
			std::lock_guard lock(s_mutex);

			// Allocate memory for trampolines
			if (m_tramps)
			{
				const s64 tramps_diff = reinterpret_cast<u64>(m_tramps) - code_start;
				if (tramps_diff < INT_MIN || tramps_diff > INT_MAX)
					m_tramps = nullptr; //previously allocated trampoline section too far away now
			}

			if (!m_tramps)
			{
				m_tramps = reinterpret_cast<decltype(m_tramps)>(s_alloc.allocate(4096));
				utils::memory_commit(m_tramps, 4096, utils::protection::wx);
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
			addr = reinterpret_cast<u64>(&data);

			// Reset pointer (memory page exhausted)
			if ((reinterpret_cast<u64>(m_tramps) % 4096) == 0)
			{
				m_tramps = nullptr;
			}
		}

		return {addr, llvm::JITSymbolFlags::Exported};
	}

	bool needsToReserveAllocationSpace() override { return true; }
	void reserveAllocationSpace(uintptr_t CodeSize, uint32_t CodeAlign, uintptr_t RODataSize, uint32_t RODataAlign, uintptr_t RWDataSize, uint32_t RWDataAlign) override
	{
		const u32 wanted_code_size = ::align(static_cast<u32>(CodeSize), std::min(4096u, CodeAlign));
		const u32 wanted_rodata_size = ::align(static_cast<u32>(RODataSize), std::min(4096u, RODataAlign));
		const u32 wanted_rwdata_size = ::align(static_cast<u32>(RWDataSize), std::min(4096u, RWDataAlign));

		// Lock memory manager
		std::lock_guard lock(s_mutex);

		// Setup segment for current module if needed
		s_alloc.reserve(wanted_code_size + wanted_rodata_size + wanted_rwdata_size);
	}

	u8* allocateCodeSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name) override
	{
		void* ptr = nullptr;
		const u32 wanted_size = ::align(static_cast<u32>(size), 4096);
		{
			// Lock memory manager
			std::lock_guard lock(s_mutex);

			// Simple allocation
			ptr = s_alloc.allocate(wanted_size);
		}

		if (ptr == nullptr)
		{
			jit_log.fatal("LLVM: Out of memory (size=0x%llx, aligned 0x%x)", size, align);
			return nullptr;
		}
		utils::memory_commit(ptr, size, utils::protection::wx);
		m_code_addr = static_cast<u8*>(ptr);

		jit_log.notice("LLVM: Code section %u '%s' allocated -> %p (size=0x%llx, aligned 0x%x)", sec_id, sec_name.data(), ptr, size, align);
		return static_cast<u8*>(ptr);
	}

	u8* allocateDataSection(std::uintptr_t size, uint align, uint sec_id, llvm::StringRef sec_name, bool is_ro) override
	{
		void* ptr = nullptr;
		const u32 wanted_size = ::align(static_cast<u32>(size), 4096);
		{
			// Lock memory manager
			std::lock_guard lock(s_mutex);

			// Simple allocation
			ptr = s_alloc.allocate(wanted_size);
		}

		if (ptr == nullptr)
		{
			jit_log.fatal("LLVM: Out of memory (size=0x%llx, aligned 0x%x)", size, align);
			return nullptr;
		}

		if (!is_ro)
		{
		}

		utils::memory_commit(ptr, size);

		jit_log.notice("LLVM: Data section %u '%s' allocated -> %p (size=0x%llx, aligned 0x%x, %s)", sec_id, sec_name.data(), ptr, size, align, is_ro ? "ro" : "rw");
		return static_cast<u8*>(ptr);
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
		// Lock memory manager
		std::lock_guard lock(s_mutex);
#ifdef _WIN32
		// Fix RUNTIME_FUNCTION records (.pdata section)
		decltype(s_unwater)::value_type pdata_entry = std::move(s_unwater.front());
		s_unwater.pop_front();

		// Use given memory segment as a BASE, compute the difference
		const u64 segment_start = pdata_entry.first;
		const u64 unwind_diff = (u64)addr - segment_start;

		auto& pdata = pdata_entry.second;
		for (auto& rf : pdata)
		{
			rf.UnwindData += static_cast<DWORD>(unwind_diff);
		}

		// Register .xdata UNWIND_INFO structs
		if (!RtlAddFunctionTable(pdata.data(), (DWORD)pdata.size(), segment_start))
		{
			jit_log.error("RtlAddFunctionTable() failed! Error %u", GetLastError());
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
		{
			// Lock memory manager
			std::lock_guard lock(s_mutex);
			s_unfire.push_front(std::make_pair(addr, size));
		}
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

				// Use current memory segment as a BASE, compute the difference
				const u64 segment_start = s_alloc.current_segment().first;
				const u64 code_diff = reinterpret_cast<u64>(m_mem.m_code_addr) - segment_start;

				// Fix RUNTIME_FUNCTION records (.pdata section)
				for (auto& rf : rfs)
				{
					rf.BeginAddress += static_cast<DWORD>(code_diff);
					rf.EndAddress   += static_cast<DWORD>(code_diff);
				}

				s_unwater.emplace_back(segment_start, std::move(rfs));
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
		name.append(module->getName().data());
		//fs::file(name, fs::rewrite).write(obj.getBufferStart(), obj.getBufferSize());
		name.append(".gz");

		z_stream zs{};
		uLong zsz = compressBound(::narrow<u32>(obj.getBufferSize(), HERE)) + 256;
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
			jit_log.error("LLVM: Failed to compress module: %s", module->getName().data());
			deflateEnd(&zs);
			return;
		}
		}

		fs::file(name, fs::rewrite).write(zbuf.get(), zsz - zs.avail_out);
		jit_log.notice("LLVM: Created module: %s", module->getName().data());
	}

	static std::unique_ptr<llvm::MemoryBuffer> load(const std::string& path)
	{
		if (fs::file cached{path + ".gz", fs::read})
		{
			std::vector<uchar> gz = cached.to_vector<uchar>();
			std::vector<uchar> out;
			z_stream zs{};
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
			auto buf = llvm::WritableMemoryBuffer::getNewUninitMemBuffer(cached.size());
			cached.read(buf->getBufferStart(), buf->getBufferSize());
			return buf;
		}

		return nullptr;
	}

	std::unique_ptr<llvm::MemoryBuffer> getObject(const llvm::Module* module) override
	{
		std::string path = m_path;
		path.append(module->getName().data());

		if (auto buf = load(path))
		{
			jit_log.notice("LLVM: Loaded module: %s", module->getName().data());
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
			m_cpu == "tigerlake")
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
			m_cpu == "tigerlake")
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
		jit_log.error("ObjectCache: Adding failed: %s", path);
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
