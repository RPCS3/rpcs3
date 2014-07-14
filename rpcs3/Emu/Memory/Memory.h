#pragma once

#ifndef _WIN32
#include <sys/mman.h>
#endif

#include "MemoryBlock.h"
#include "Emu/SysCalls/Callback.h"
#include <vector>

/* OS X uses MAP_ANON instead of MAP_ANONYMOUS */
#ifndef MAP_ANONYMOUS
	#define MAP_ANONYMOUS MAP_ANON
#endif

using std::nullptr_t;

#define safe_delete(x) do {delete (x);(x)=nullptr;} while(0)
#define safe_free(x) do {free(x);(x)=nullptr;} while(0)

enum MemoryType
{
	Memory_PS3,
	Memory_PSV,
	Memory_PSP,
};

enum : u64
{
	RAW_SPU_OFFSET = 0x0000000000100000,
	RAW_SPU_BASE_ADDR = 0x00000000E0000000,
	RAW_SPU_LS_OFFSET = 0x0000000000000000,
	RAW_SPU_PROB_OFFSET = 0x0000000000040000,
};

class MemoryBase
{
	void* m_base_addr;
	std::vector<MemoryBlock*> MemoryBlocks;
	u32 m_pages[0x100000000 / 4096]; // information about every page
	std::recursive_mutex m_mutex;

public:
	MemoryBlock* UserMemory;

	DynamicMemoryBlock MainMem;
	DynamicMemoryBlock PRXMem;
	DynamicMemoryBlock RSXCMDMem;
	DynamicMemoryBlock MmaperMem;
	DynamicMemoryBlock RSXFBMem;
	DynamicMemoryBlock StackMem;
	MemoryBlock* RawSPUMem[(0x100000000 - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET];
	VirtualMemoryBlock RSXIOMem;

	struct
	{
		DynamicMemoryBlockLE RAM;
		DynamicMemoryBlockLE Userspace;
	} PSVMemory;

	struct
	{
		DynamicMemoryBlockLE Scratchpad;
		DynamicMemoryBlockLE VRAM;
		DynamicMemoryBlockLE RAM;
		DynamicMemoryBlockLE Kernel;
		DynamicMemoryBlockLE Userspace;
	} PSPMemory;

	bool m_inited;

	MemoryBase()
	{
		m_inited = false;
	}

	~MemoryBase()
	{
		Close();
	}

	void* GetBaseAddr() const
	{
		return m_base_addr;
	}

	void RegisterPages(u64 addr, u32 size)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		//LOG_NOTICE(MEMORY, "RegisterPages(addr=0x%llx, size=0x%x)", addr, size);
		for (u32 i = addr / 4096; i < (addr + size) / 4096; i++)
		{
			if (i >= sizeof(m_pages) / sizeof(m_pages[0])) break;
			if (m_pages[i])
			{
				LOG_ERROR(MEMORY, "Page already registered (page=0x%x)", i * 4096);
			}
			m_pages[i] = 1; // TODO: define page parameters
		}
	}

	void UnregisterPages(u64 addr, u32 size)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		//LOG_NOTICE(MEMORY, "UnregisterPages(addr=0x%llx, size=0x%x)", addr, size);
		for (u32 i = addr / 4096; i < (addr + size) / 4096; i++)
		{
			if (i >= sizeof(m_pages) / sizeof(m_pages[0])) break;
			if (!m_pages[i])
			{
				LOG_ERROR(MEMORY, "Page not registered (page=0x%x)", i * 4096);
			}
			m_pages[i] = 0; // TODO: define page parameters
		}
	}

	static __forceinline u16 Reverse16(const u16 val)
	{
		return _byteswap_ushort(val);
	}

	static __forceinline u32 Reverse32(const u32 val)
	{
		return _byteswap_ulong(val);
	}

	static __forceinline u64 Reverse64(const u64 val)
	{
		return _byteswap_uint64(val);
	}

	static __forceinline u128 Reverse128(const u128 val)
	{
		u128 ret;
		ret.lo = _byteswap_uint64(val.hi);
		ret.hi = _byteswap_uint64(val.lo);
		return ret;
	}

	template<int size> static __forceinline u64 ReverseData(u64 val);

	template<typename T> static __forceinline T Reverse(T val)
	{
		return (T)ReverseData<sizeof(T)>(val);
	};

	u8* GetMemFromAddr(const u64 addr)
	{
		return (u8*)GetBaseAddr() + addr;
	}

	void* VirtualToRealAddr(const u64 vaddr)
	{
		return GetMemFromAddr(vaddr);
	}

	u64 RealToVirtualAddr(const void* addr)
	{
		const u64 res = (u64)addr - (u64)GetBaseAddr();
		
		if (res < 0x100000000)
		{
			return res;
		}
		else
		{
			return 0;
		}
	}

	void InitRawSPU(MemoryBlock* raw_spu, const u32 num)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		MemoryBlocks.push_back(raw_spu);
		if (num < sizeof(RawSPUMem) / sizeof(RawSPUMem[0])) RawSPUMem[num] = raw_spu;
	}

	void CloseRawSPU(MemoryBlock* raw_spu, const u32 num)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		for (int i = 0; i < MemoryBlocks.size(); ++i)
		{
			if (MemoryBlocks[i] == raw_spu)
			{
				MemoryBlocks.erase(MemoryBlocks.begin() + i);
				break;
			}
		}
		if (num < sizeof(RawSPUMem) / sizeof(RawSPUMem[0])) RawSPUMem[num] = nullptr;
	}

	void Init(MemoryType type)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		if(m_inited) return;
		m_inited = true;

		memset(m_pages, 0, sizeof(m_pages));
		memset(RawSPUMem, 0, sizeof(RawSPUMem));

#ifdef _WIN32
		m_base_addr = VirtualAlloc(nullptr, 0x100000000, MEM_RESERVE, PAGE_NOACCESS);
		if (!m_base_addr)
#else
		m_base_addr = ::mmap(nullptr, 0x100000000, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		if (m_base_addr == (void*)-1)
#endif
		{
			m_base_addr = nullptr;
			LOG_ERROR(MEMORY, "Initing memory failed");
			assert(0);
			return;
		}
		else
		{
			LOG_NOTICE(MEMORY, "Initing memory: m_base_addr = 0x%llx", (u64)m_base_addr);
		}

		switch(type)
		{
		case Memory_PS3:
			MemoryBlocks.push_back(MainMem.SetRange(0x00010000, 0x2FFF0000));
			MemoryBlocks.push_back(UserMemory = PRXMem.SetRange(0x30000000, 0x10000000));
			MemoryBlocks.push_back(RSXCMDMem.SetRange(0x40000000, 0x10000000));
			MemoryBlocks.push_back(MmaperMem.SetRange(0xB0000000, 0x10000000));
			MemoryBlocks.push_back(RSXFBMem.SetRange(0xC0000000, 0x10000000));
			MemoryBlocks.push_back(StackMem.SetRange(0xD0000000, 0x10000000));
		break;

		case Memory_PSV:
			MemoryBlocks.push_back(PSVMemory.RAM.SetRange(0x81000000, 0x10000000));
			MemoryBlocks.push_back(UserMemory = PSVMemory.Userspace.SetRange(0x91000000, 0x10000000));
		break;

		case Memory_PSP:
			MemoryBlocks.push_back(PSPMemory.Scratchpad.SetRange(0x00010000, 0x00004000));
			MemoryBlocks.push_back(PSPMemory.VRAM.SetRange(0x04000000, 0x00200000));
			MemoryBlocks.push_back(PSPMemory.RAM.SetRange(0x08000000, 0x02000000));
			MemoryBlocks.push_back(PSPMemory.Kernel.SetRange(0x88000000, 0x00800000));
			MemoryBlocks.push_back(UserMemory = PSPMemory.Userspace.SetRange(0x08800000, 0x01800000));
		break;
		}

		LOG_NOTICE(MEMORY, "Memory initialized.");
	}

	bool IsGoodAddr(const u64 addr)
	{
		if (addr >= 0x100000000 || !m_pages[addr / 4096]) // TODO: define page parameters
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	bool IsGoodAddr(const u64 addr, const u32 size)
	{
		if (addr + size > 0x100000000)
		{
			return false;
		}
		else
		{
			for (u32 i = addr / 4096; i <= (addr + size - 1) / 4096; i++)
			{
				if (!m_pages[i]) return false; // TODO: define page parameters
			}
			return true;
		}
	}

	void Close()
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		if(!m_inited) return;
		m_inited = false;

		LOG_NOTICE(MEMORY, "Closing memory...");

		for (auto block : MemoryBlocks)
		{
			block->Delete();
		}

		RSXIOMem.Delete();

		MemoryBlocks.clear();

#ifdef _WIN32
		if (!VirtualFree(m_base_addr, 0, MEM_RELEASE))
		{
			LOG_ERROR(MEMORY, "VirtualFree(0x%llx) failed", (u64)m_base_addr);
		}
#else
		if (::munmap(m_base_addr, 0x100000000))
		{
			LOG_ERROR(MEMORY, "::munmap(0x%llx) failed", (u64)m_base_addr);
		}
#endif
	}

	void Write8(const u64 addr, const u8 data);
	void Write16(const u64 addr, const u16 data);
	void Write32(const u64 addr, const u32 data);
	void Write64(const u64 addr, const u64 data);
	void Write128(const u64 addr, const u128 data);

	bool Write8NN(const u64 addr, const u8 data);
	bool Write16NN(const u64 addr, const u16 data);
	bool Write32NN(const u64 addr, const u32 data);
	bool Write64NN(const u64 addr, const u64 data);
	bool Write128NN(const u64 addr, const u128 data);

	u8 Read8(const u64 addr);
	u16 Read16(const u64 addr);
	u32 Read32(const u64 addr);
	u64 Read64(const u64 addr);
	u128 Read128(const u64 addr);

	bool CopyToReal(void* real, u64 from, u32 count)
	{
		if (!IsGoodAddr(from, count)) return false;

		memcpy(real, GetMemFromAddr(from), count);

		return true;
	}

	bool CopyFromReal(u64 to, const void* real, u32 count)
	{
		if (!IsGoodAddr(to, count)) return false;

		memcpy(GetMemFromAddr(to), real, count);

		return true;
	}

	bool Copy(u64 to, u64 from, u32 count)
	{
		if (!IsGoodAddr(to, count) || !IsGoodAddr(from, count)) return false;

		memmove(GetMemFromAddr(to), GetMemFromAddr(from), count);

		return true;
	}

	void ReadLeft(u8* dst, const u64 addr, const u32 size)
	{
		for (u32 i = 0; i < size; ++i) dst[size - 1 - i] = Read8(addr + i);
	}

	void WriteLeft(const u64 addr, const u32 size, const u8* src)
	{
		for (u32 i = 0; i < size; ++i) Write8(addr + i, src[size - 1 - i]);
	}

	void ReadRight(u8* dst, const u64 addr, const u32 size)
	{
		for (u32 i = 0; i < size; ++i) dst[i] = Read8(addr + (size - 1 - i));
	}

	void WriteRight(const u64 addr, const u32 size, const u8* src)
	{
		for (u32 i = 0; i < size; ++i) Write8(addr + (size - 1 - i), src[i]);
	}

	template<typename T> void WriteData(const u64 addr, const T* data)
	{
		memcpy(GetMemFromAddr(addr), data, sizeof(T));
	}

	template<typename T> void WriteData(const u64 addr, const T data)
	{
		*(T*)GetMemFromAddr(addr) = data;
	}

	std::string ReadString(const u64 addr, const u64 len)
	{
		std::string ret((const char *)GetMemFromAddr(addr), len);

		return ret;
	}

	std::string ReadString(const u64 addr)
	{
		return std::string((const char*)GetMemFromAddr(addr));
	}

	void WriteString(const u64 addr, const std::string& str)
	{
		strcpy((char*)GetMemFromAddr(addr), str.c_str());
	}

	static u64 AlignAddr(const u64 addr, const u64 align)
	{
		return (addr + (align-1)) & ~(align-1);
	}

	u32 GetUserMemTotalSize()
	{
		return UserMemory->GetSize();
	}

	u32 GetUserMemAvailSize()
	{
		return UserMemory->GetSize() - UserMemory->GetUsedSize();
	}

	u64 Alloc(const u32 size, const u32 align)
	{
		return UserMemory->AllocAlign(size, align);
	}

	bool Free(const u64 addr)
	{
		return UserMemory->Free(addr);
	}

	bool Lock(const u64 addr, const u32 size)
	{
		return UserMemory->Lock(addr, size);
	}

	bool Unlock(const u64 addr, const u32 size)
	{
		return UserMemory->Unlock(addr, size);
	}

	bool Map(const u64 dst_addr, const u64 src_addr, const u32 size)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		if(IsGoodAddr(dst_addr) || !IsGoodAddr(src_addr))
		{
			return false;
		}

		MemoryBlocks.push_back((new MemoryMirror())->SetRange(GetMemFromAddr(src_addr), dst_addr, size));
		LOG_WARNING(MEMORY, "memory mapped 0x%llx to 0x%llx size=0x%x", src_addr, dst_addr, size);
		return true;
	}

	bool Unmap(const u64 addr)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);

		bool result = false;
		for(uint i=0; i<MemoryBlocks.size(); ++i)
		{
			if(MemoryBlocks[i]->IsMirror())
			{
				if(MemoryBlocks[i]->GetStartAddr() == addr)
				{
					delete MemoryBlocks[i];
					MemoryBlocks.erase(MemoryBlocks.begin() + i);
					return true;
				}
			}
		}
		return false;
	}

	u8* operator + (const u64 vaddr)
	{
		u8* ret = GetMemFromAddr(vaddr);
		return ret;
	}

	u8& operator[] (const u64 vaddr)
	{
		return *(*this + vaddr);
	}
};

extern MemoryBase Memory;

template<typename T, typename AT = u32>
class mem_base_t
{
protected:
	AT m_addr;

public:
	mem_base_t(AT addr) : m_addr(addr)
	{
	}

	__forceinline AT GetAddr() const { return m_addr; }

	__forceinline void SetAddr(AT addr) { m_addr = addr; }

	__forceinline bool IsGood() const
	{
		return Memory.IsGoodAddr(m_addr, sizeof(T));
	}

	__forceinline operator bool() const
	{
		return m_addr != 0;
	}

	__forceinline bool operator != (nullptr_t) const
	{
		return m_addr != 0;
	}

	__forceinline bool operator == (nullptr_t) const
	{
		return m_addr == 0;
	}
	
	bool operator == (const mem_base_t& right) const { return m_addr == right.m_addr; }
	bool operator != (const mem_base_t& right) const { return m_addr != right.m_addr; }
	bool operator > (const mem_base_t& right) const { return m_addr > right.m_addr; }
	bool operator < (const mem_base_t& right) const { return m_addr < right.m_addr; }
	bool operator >= (const mem_base_t& right) const { return m_addr >= right.m_addr; }
	bool operator <= (const mem_base_t& right) const { return m_addr <= right.m_addr; }

	bool operator == (T* right) const { return (T*)&Memory[m_addr] == right; }
	bool operator != (T* right) const { return (T*)&Memory[m_addr] != right; }
	bool operator > (T* right) const { return (T*)&Memory[m_addr] > right; }
	bool operator < (T* right) const { return (T*)&Memory[m_addr] < right; }
	bool operator >= (T* right) const { return (T*)&Memory[m_addr] >= right; }
	bool operator <= (T* right) const { return (T*)&Memory[m_addr] <= right; }
};

template<typename T, int lvl = 1, typename AT = u32>
class mem_ptr_t : public mem_base_t<AT, AT>
{
public:
	mem_ptr_t(AT addr) : mem_base_t<AT, AT>(addr)
	{
	}

	template<typename NT> operator mem_ptr_t<NT, lvl, AT>&() { return (mem_ptr_t<NT, lvl, AT>&)*this; }
	template<typename NT> operator const mem_ptr_t<NT, lvl, AT>&() const { return (const mem_ptr_t<NT, lvl, AT>&)*this; }

	mem_ptr_t operator++ (int)
	{
		mem_ptr_t ret(this->m_addr);
		this->m_addr += sizeof(AT);
		return ret;
	}

	mem_ptr_t& operator++ ()
	{
		this->m_addr += sizeof(AT);
		return *this;
	}

	mem_ptr_t operator-- (int)
	{
		mem_ptr_t ret(this->m_addr);
		this->m_addr -= sizeof(AT);
		return ret;
	}

	mem_ptr_t& operator-- ()
	{
		this->m_addr -= sizeof(AT);
		return *this;
	}

	mem_ptr_t& operator += (uint count)
	{
		this->m_addr += count * sizeof(AT);
		return *this;
	}

	mem_ptr_t& operator -= (uint count)
	{
		this->m_addr -= count * sizeof(AT);
		return *this;
	}

	mem_ptr_t operator + (uint count) const
	{
		return this->m_addr + count * sizeof(AT);
	}

	mem_ptr_t operator - (uint count) const
	{
		return this->m_addr - count * sizeof(AT);
	}

	__forceinline mem_ptr_t<T, lvl - 1, AT>& operator *()
	{
		return (mem_ptr_t<T, lvl - 1, AT>&)Memory[this->m_addr];
	}

	__forceinline const mem_ptr_t<T, lvl - 1, AT>& operator *() const
	{
		return (const mem_ptr_t<T, lvl - 1, AT>&)Memory[this->m_addr];
	}

	__forceinline mem_ptr_t<T, lvl - 1, AT>& operator [](uint index)
	{
		return (mem_ptr_t<T, lvl - 1, AT>&)Memory[this->m_addr + sizeof(AT) * index];
	}

	__forceinline const mem_ptr_t<T, lvl - 1, AT>& operator [](uint index) const
	{
		return (const mem_ptr_t<T, lvl - 1, AT>&)Memory[this->m_addr + sizeof(AT) * index];
	}

	bool IsGood() const
	{
		return (*this)->IsGood() && mem_base_t<T, AT>::IsGood();
	}

	__forceinline bool IsGoodAddr() const
	{
		return mem_base_t<T, AT>::IsGood();
	}
};

template<typename T, typename AT>
class mem_ptr_t<T, 1, AT> : public mem_base_t<T, AT>
{
public:
	mem_ptr_t(AT addr) : mem_base_t<T, AT>(addr)
	{
	}

	template<typename NT> operator mem_ptr_t<NT, 1, AT>&() { return (mem_ptr_t<NT, 1, AT>&)*this; }
	template<typename NT> operator const mem_ptr_t<NT, 1, AT>&() const { return (const mem_ptr_t<NT, 1, AT>&)*this; }

	__forceinline T* operator -> ()
	{
		return (T*)&Memory[this->m_addr];
	}

	__forceinline const T* operator -> () const
	{
		return (const T*)&Memory[this->m_addr];
	}

	mem_ptr_t operator++ (int)
	{
		mem_ptr_t ret(this->m_addr);
		this->m_addr += sizeof(T);
		return ret;
	}

	mem_ptr_t& operator++ ()
	{
		this->m_addr += sizeof(T);
		return *this;
	}

	mem_ptr_t operator-- (int)
	{
		mem_ptr_t ret(this->m_addr);
		this->m_addr -= sizeof(T);
		return ret;
	}

	mem_ptr_t& operator-- ()
	{
		this->m_addr -= sizeof(T);
		return *this;
	}

	mem_ptr_t& operator += (uint count)
	{
		this->m_addr += count * sizeof(T);
		return *this;
	}

	mem_ptr_t& operator -= (uint count)
	{
		this->m_addr -= count * sizeof(T);
		return *this;
	}

	mem_ptr_t operator + (uint count) const
	{
		return this->m_addr + count * sizeof(T);
	}

	mem_ptr_t operator - (uint count) const
	{
		return this->m_addr - count * sizeof(T);
	}

	__forceinline T& operator *()
	{
		return (T&)Memory[this->m_addr];
	}

	__forceinline const T& operator *() const
	{
		return (T&)Memory[this->m_addr];
	}

	__forceinline T& operator [](uint index)
	{
		return (T&)Memory[this->m_addr + sizeof(T) * index];
	}

	__forceinline const T& operator [](uint index) const
	{
		return (const T&)Memory[this->m_addr + sizeof(T) * index];
	}
};

template<typename AT>
class mem_ptr_t<void, 1, AT> : public mem_base_t<u8, AT>
{
public:
	mem_ptr_t(AT addr) : mem_base_t<u8, AT>(addr)
	{
	}

	template<typename NT> operator mem_ptr_t<NT, 1, AT>&() { return (mem_ptr_t<NT, 1, AT>&)*this; }
	template<typename NT> operator const mem_ptr_t<NT, 1, AT>&() const { return (const mem_ptr_t<NT, 1, AT>&)*this; }
};

template<typename T, int lvl = 1, typename AT = u32>
class mem_beptr_t : public mem_ptr_t<T, lvl, be_t<AT>> {};

template<typename T, typename AT = u32> class mem_t : public mem_base_t<T, AT>
{
public:
	mem_t(AT addr) : mem_base_t<T, AT>(addr)
	{
	}

	mem_t& operator = (T right)
	{
		(be_t<T>&)Memory[this->m_addr] = right;

		return *this;
	}

	__forceinline T GetValue()
	{
		return (be_t<T>&)Memory[this->m_addr];
	}

	operator const T() const
	{
		return (be_t<T>&)Memory[this->m_addr];
	}

	mem_t& operator += (T right) { return *this = (*this) + right; }
	mem_t& operator -= (T right) { return *this = (*this) - right; }
	mem_t& operator *= (T right) { return *this = (*this) * right; }
	mem_t& operator /= (T right) { return *this = (*this) / right; }
	mem_t& operator %= (T right) { return *this = (*this) % right; }
	mem_t& operator &= (T right) { return *this = (*this) & right; }
	mem_t& operator |= (T right) { return *this = (*this) | right; }
	mem_t& operator ^= (T right) { return *this = (*this) ^ right; }
	mem_t& operator <<= (T right) { return *this = (*this) << right; }
	mem_t& operator >>= (T right) { return *this = (*this) >> right; }
};

template<typename T, typename AT = u32> class mem_list_ptr_t : public mem_base_t<T, AT>
{
public:
	mem_list_ptr_t(u32 addr) : mem_base_t<T>(addr)
	{
	}

	void operator = (T right)
	{
		(be_t<T>&)Memory[this->m_addr] = right;
	}

	u32 operator += (T right)
	{
		*this = right;
		this->m_addr += sizeof(T);
		return this->m_addr;
	}
	
	u32 AppendRawBytes(const u8 *bytes, size_t count)
	{
		memmove(Memory + this->m_addr, bytes, count);
		this->m_addr += count;
		return this->m_addr;
	}

	u32 Skip(const u32 offset) { return this->m_addr += offset; }

	operator be_t<T>*()			{ return GetPtr(); }
	operator void*()			{ return GetPtr(); }
	operator be_t<T>*()	const	{ return GetPtr(); }
	operator void*()	const	{ return GetPtr(); }

	const char* GetString() const
	{
		return (const char*)&Memory[this->m_addr];
	}

	be_t<T>* GetPtr()
	{
		return (be_t<T>*)&Memory[this->m_addr];
	}

	const be_t<T>* GetPtr() const
	{
		return (const be_t<T>*)&Memory[this->m_addr];
	}
};

class mem_class_t
{
	u32 m_addr;

public:
	mem_class_t(u32 addr) : m_addr(addr)
	{
	}

	template<typename T> u32 operator += (T right)
	{
		mem_t<T>& m((mem_t<T>&)*this);
		m = right;
		m_addr += sizeof(T);
		return m_addr;
	}

	template<typename T> operator T()
	{
		mem_t<T>& m((mem_t<T>&)*this);
		const T ret = m;
		m_addr += sizeof(T);
		return ret;
	}

	u64 GetAddr() const { return m_addr; }
	void SetAddr(const u64 addr) { m_addr = addr; }
};

template<typename T>
struct _func_arg
{
	__forceinline static u64 get_value(const T& arg)
	{
		return arg;
	}
};

template<typename T>
struct _func_arg<mem_base_t<T, u32>>
{
	__forceinline static u64 get_value(const mem_base_t<T, u32> arg)
	{
		return arg.GetAddr();
	}
};

template<typename T, int lvl> struct _func_arg<mem_ptr_t<T, lvl, u32>> : public _func_arg<mem_base_t<T, u32>> {};
template<int lvl> struct _func_arg<mem_ptr_t<void, lvl, u32>> : public _func_arg<mem_base_t<u8, u32>>{};
template<typename T> struct _func_arg<mem_list_ptr_t<T, u32>> : public _func_arg<mem_base_t<T, u32>> {};
template<typename T> struct _func_arg<mem_t<T, u32>> : public _func_arg<mem_base_t<T, u32>> {};

template<typename T>
struct _func_arg<be_t<T>>
{
	__forceinline static u64 get_value(const be_t<T>& arg)
	{
		return arg.ToLE();
	}
};

template<typename T, typename AT = u32> class mem_func_ptr_t;
template<typename T, typename AT = u32> class mem_func_beptr_t : public mem_func_ptr_t<T, be_t<AT>> {};

template<typename RT, typename AT>
class mem_func_ptr_t<RT (*)(), AT> : public mem_base_t<u64, AT>
{
	__forceinline RT call_func(bool is_async) const
	{
		Callback cb;
		cb.SetAddr(this->m_addr);
		return (RT)cb.Branch(!is_async);
	}

public:
	__forceinline RT operator()() const
	{
		return call_func(false);
	}

	__forceinline void async() const
	{
		call_func(true);
	}
};

template<typename AT, typename RT, typename ...T>
class mem_func_ptr_t<RT(*)(T...), AT> : public mem_base_t<u64, AT>
{
	__forceinline RT call_func(bool is_async, T... args) const
	{
		Callback cb;
		cb.SetAddr(this->m_addr);
		cb.Handle(_func_arg<T>::get_value(args)...);
		return (RT)cb.Branch(!is_async);
	}

public:
	__forceinline RT operator()(T... args) const
	{
		return call_func(false, args...);
	}

	__forceinline void async(T... args) const
	{
		call_func(true, args...);
	}
};

template<typename T>
class MemoryAllocator
{
	u32 m_addr;
	u32 m_size;
	T* m_ptr;

public:
	MemoryAllocator(u32 size = sizeof(T), u32 align = 1)
		: m_size(size)
		, m_addr(Memory.Alloc(size, align))
		, m_ptr((T*)&Memory[m_addr])
	{
	}

	~MemoryAllocator()
	{
		Memory.Free(m_addr);
	}

	T* operator -> ()
	{
		return m_ptr;
	}

	T* GetPtr()
	{
		return m_ptr;
	}

	const T* GetPtr() const
	{
		return m_ptr;
	}

	const T* operator -> () const
	{
		return m_ptr;
	}

	u32 GetAddr() const
	{
		return m_addr;
	}

	u32 GetSize() const
	{
		return m_size;
	}

	bool IsGood() const
	{
		return Memory.IsGoodAddr(m_addr, sizeof(T));
	}

	template<typename T1>
	operator const T1() const
	{
		return T1(*m_ptr);
	}

	template<typename T1>
	operator T1()
	{
		return T1(*m_ptr);
	}

	operator const T&() const
	{
		return *m_ptr;
	}

	operator T&()
	{
		return *m_ptr;
	}

	operator const T*() const
	{
		return m_ptr;
	}

	operator T*()
	{
		return m_ptr;
	}

	T operator [](int index)
	{
		return *(m_ptr + index);
	}

	template<typename T1>
	operator const mem_t<T1>() const
	{
		return GetAddr();
	}

	operator const mem_ptr_t<T>() const
	{
		return GetAddr();
	}

	template<typename NT>
	NT* To(uint offset = 0)
	{
		return (NT*)(m_ptr + offset);
	}
};

typedef mem_t<u8, u32> mem8_t;
typedef mem_t<u16, u32> mem16_t;
typedef mem_t<u32, u32> mem32_t;
typedef mem_t<u64, u32> mem64_t;

/*
typedef mem_ptr_t<be_t<u8>> mem8_ptr_t;
typedef mem_ptr_t<be_t<u16>> mem16_ptr_t;
typedef mem_ptr_t<be_t<u32>> mem32_ptr_t;
typedef mem_ptr_t<be_t<u64>> mem64_ptr_t;

typedef mem_list_ptr_t<u8> mem8_lptr_t;
typedef mem_list_ptr_t<u16> mem16_lptr_t;
typedef mem_list_ptr_t<u32> mem32_lptr_t;
typedef mem_list_ptr_t<u64> mem64_lptr_t;
*/

typedef mem_list_ptr_t<u8, u32> mem8_ptr_t;
typedef mem_list_ptr_t<u16, u32> mem16_ptr_t;
typedef mem_list_ptr_t<u32, u32> mem32_ptr_t;
typedef mem_list_ptr_t<u64, u32> mem64_ptr_t;

