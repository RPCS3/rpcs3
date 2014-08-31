#pragma once

#include "MemoryBlock.h"
#include "Emu/SysCalls/Callback.h"

using std::nullptr_t;

#define safe_delete(x) do {delete (x);(x)=nullptr;} while(0)
#define safe_free(x) do {free(x);(x)=nullptr;} while(0)

extern void* const m_base_addr;

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

	struct Wrapper32LE
	{
		void Write8(const u32 addr, const u8 data) { *(u8*)((u8*)m_base_addr + addr) = data; }
		void Write16(const u32 addr, const u16 data) { *(u16*)((u8*)m_base_addr + addr) = data; }
		void Write32(const u32 addr, const u32 data) { *(u32*)((u8*)m_base_addr + addr) = data; }
		void Write64(const u32 addr, const u64 data) { *(u64*)((u8*)m_base_addr + addr) = data; }
		void Write128(const u32 addr, const u128 data) { *(u128*)((u8*)m_base_addr + addr) = data; }

		u8 Read8(const u32 addr) { return *(u8*)((u8*)m_base_addr + addr); }
		u16 Read16(const u32 addr) { return *(u16*)((u8*)m_base_addr + addr); }
		u32 Read32(const u32 addr) { return *(u32*)((u8*)m_base_addr + addr); }
		u64 Read64(const u32 addr) { return *(u64*)((u8*)m_base_addr + addr); }
		u128 Read128(const u32 addr) { return *(u128*)((u8*)m_base_addr + addr); }
	};

	struct : Wrapper32LE
	{
		DynamicMemoryBlockLE RAM;
		DynamicMemoryBlockLE Userspace;
	} PSV;

	struct : Wrapper32LE
	{
		DynamicMemoryBlockLE Scratchpad;
		DynamicMemoryBlockLE VRAM;
		DynamicMemoryBlockLE RAM;
		DynamicMemoryBlockLE Kernel;
		DynamicMemoryBlockLE Userspace;
	} PSP;

	bool m_inited;

	MemoryBase()
	{
		m_inited = false;
	}

	~MemoryBase()
	{
		Close();
	}

	static void* const GetBaseAddr()
	{
		return m_base_addr;
	}

	__noinline void InvalidAddress(const char* func, const u64 addr);

	void RegisterPages(u64 addr, u32 size);

	void UnregisterPages(u64 addr, u32 size);

	template<typename T> u8* GetMemFromAddr(const T addr)
	{
		if ((u32)addr == addr)
		{
			return (u8*)GetBaseAddr() + addr;
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return (u8*)GetBaseAddr();
		}
	}

	template<typename T> void* VirtualToRealAddr(const T vaddr)
	{
		return GetMemFromAddr<T>(vaddr);
	}

	u32 RealToVirtualAddr(const void* addr)
	{
		const u64 res = (u64)addr - (u64)GetBaseAddr();
		
		if ((u32)res == res)
		{
			return (u32)res;
		}
		else
		{
			assert(!addr);
			return 0;
		}
	}

	u32 InitRawSPU(MemoryBlock* raw_spu);

	void CloseRawSPU(MemoryBlock* raw_spu, const u32 num);

	void Init(MemoryType type);

	template<typename T> bool IsGoodAddr(const T addr)
	{
		if ((u32)addr != addr || !m_pages[addr / 4096]) // TODO: define page parameters
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	template<typename T> bool IsGoodAddr(const T addr, const u32 size)
	{
		if ((u32)addr != addr || (u64)addr + (u64)size > 0x100000000ull)
		{
			return false;
		}
		else
		{
			for (u32 i = (u32)addr / 4096; i <= ((u32)addr + size - 1) / 4096; i++)
			{
				if (!m_pages[i]) return false; // TODO: define page parameters
			}
			return true;
		}
	}

	void Close();

	//MemoryBase
	template<typename T> void Write8(T addr, const u8 data)
	{
		if ((u32)addr == addr)
		{
			*(u8*)((u8*)GetBaseAddr() + addr) = data;
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			*(u8*)GetBaseAddr() = data;
		}
	}

	template<typename T> void Write16(T addr, const u16 data)
	{
		if ((u32)addr == addr)
		{
			*(u16*)((u8*)GetBaseAddr() + addr) = re16(data);
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			*(u16*)GetBaseAddr() = data;
		}
	}

	__noinline void WriteMMIO32(u32 addr, const u32 data);

	template<typename T> void Write32(T addr, const u32 data)
	{
		if ((u32)addr == addr)
		{
			if (addr < RAW_SPU_BASE_ADDR || (addr % RAW_SPU_OFFSET) < RAW_SPU_PROB_OFFSET)
			{
				*(u32*)((u8*)GetBaseAddr() + addr) = re32(data);
			}
			else
			{
				WriteMMIO32((u32)addr, data);
			}
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			*(u32*)GetBaseAddr() = data;
		}
	}

	template<typename T> void Write64(T addr, const u64 data)
	{
		if ((u32)addr == addr)
		{
			*(u64*)((u8*)GetBaseAddr() + addr) = re64(data);
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			*(u64*)GetBaseAddr() = data;
		}
	}

	template<typename T> void Write128(T addr, const u128 data)
	{
		if ((u32)addr == addr)
		{
			*(u128*)((u8*)GetBaseAddr() + addr) = re128(data);
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			*(u128*)GetBaseAddr() = data;
		}
	}

	template<typename T> u8 Read8(T addr)
	{
		if ((u32)addr == addr)
		{
			return *(u8*)((u8*)GetBaseAddr() + addr);
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return *(u8*)GetBaseAddr();
		}
	}

	template<typename T> u16 Read16(T addr)
	{
		if ((u32)addr == addr)
		{
			return re16(*(u16*)((u8*)GetBaseAddr() + addr));
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return *(u16*)GetBaseAddr();
		}
	}

	__noinline u32 ReadMMIO32(u32 addr);

	template<typename T> u32 Read32(T addr)
	{
		if ((u32)addr == addr)
		{
			if (addr < RAW_SPU_BASE_ADDR || (addr % RAW_SPU_OFFSET) < RAW_SPU_PROB_OFFSET)
			{
				return re32(*(u32*)((u8*)GetBaseAddr() + addr));
			}
			else
			{
				return ReadMMIO32((u32)addr);
			}
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return *(u32*)GetBaseAddr();
		}
	}

	template<typename T> u64 Read64(T addr)
	{
		if ((u32)addr == addr)
		{
			return re64(*(u64*)((u8*)GetBaseAddr() + addr));
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return *(u64*)GetBaseAddr();
		}
	}

	template<typename T> u128 Read128(T addr)
	{
		if ((u32)addr == addr)
		{
			return re128(*(u128*)((u8*)GetBaseAddr() + addr));
		}
		else
		{
			InvalidAddress(__FUNCTION__, addr);
			return *(u128*)GetBaseAddr();
		}
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

	template<typename T, typename Td> void WriteData(const T addr, const Td* data)
	{
		memcpy(GetMemFromAddr<T>(addr), data, sizeof(Td));
	}

	template<typename T, typename Td> void WriteData(const T addr, const Td data)
	{
		*(Td*)GetMemFromAddr<T>(addr) = data;
	}

	template<typename T> std::string ReadString(const T addr, const u64 len)
	{
		std::string ret((const char *)GetMemFromAddr<T>(addr), len);

		return ret;
	}

	template<typename T> std::string ReadString(const T addr)
	{
		return std::string((const char*)GetMemFromAddr<T>(addr));
	}

	template<typename T> void WriteString(const T addr, const std::string& str)
	{
		memcpy(GetMemFromAddr<T>(addr), str.c_str(), str.size());
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

	bool Map(const u64 dst_addr, const u64 src_addr, const u32 size);

	bool Unmap(const u64 addr);

	template<typename T> void* operator + (const T vaddr)
	{
		return GetMemFromAddr<T>(vaddr);
	}

	template<typename T> u8& operator[] (const T vaddr)
	{
		return *GetMemFromAddr<T>(vaddr);
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

typedef mem_t<u16, u32> mem16_t;
typedef mem_t<u32, u32> mem32_t;
typedef mem_t<u64, u32> mem64_t;

#include "vm.h"

