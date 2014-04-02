#pragma once
#include "MemoryBlock.h"
#include <vector>

enum MemoryType
{
	Memory_PS3,
	Memory_PSV,
	Memory_PSP,
};

class MemoryBase
{
	NullMemoryBlock NullMem;

public:
	std::vector<MemoryBlock*> MemoryBlocks;
	MemoryBlock* UserMemory;

	DynamicMemoryBlock MainMem;
	DynamicMemoryBlock PRXMem;
	DynamicMemoryBlock RSXCMDMem;
	DynamicMemoryBlock MmaperMem;
	DynamicMemoryBlock RSXFBMem;
	DynamicMemoryBlock StackMem;
	MemoryBlock SpuRawMem;
	MemoryBlock SpuThrMem;
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

	static __forceinline u16 Reverse16(const u16 val)
	{
		return _byteswap_ushort(val);
		//return ((val >> 8) & 0xff) | ((val << 8) & 0xff00);
	}

	static __forceinline u32 Reverse32(const u32 val)
	{
		return _byteswap_ulong(val);
		/*
		return
			((val >> 24) & 0x000000ff) |
			((val >>  8) & 0x0000ff00) |
			((val <<  8) & 0x00ff0000) |
			((val << 24) & 0xff000000);
		*/
	}

	static __forceinline u64 Reverse64(const u64 val)
	{
		return _byteswap_uint64(val);
		/*
		return
			((val >> 56) & 0x00000000000000ff) |
			((val >> 40) & 0x000000000000ff00) |
			((val >> 24) & 0x0000000000ff0000) |
			((val >>  8) & 0x00000000ff000000) |
			((val <<  8) & 0x000000ff00000000) |
			((val << 24) & 0x0000ff0000000000) |
			((val << 40) & 0x00ff000000000000) |
			((val << 56) & 0xff00000000000000);
		*/
	}

	template<int size> static __forceinline u64 ReverseData(u64 val);

	template<typename T> static __forceinline T Reverse(T val)
	{
		return (T)ReverseData<sizeof(T)>(val);
	};

	MemoryBlock& GetMemByNum(const u8 num)
	{
		if(num >= MemoryBlocks.size()) return NullMem;
		return *MemoryBlocks[num];
	}

	MemoryBlock& GetMemByAddr(const u64 addr)
	{
		for(uint i=0; i<MemoryBlocks.size(); ++i)
		{
			if(MemoryBlocks[i]->IsMyAddress(addr)) return *MemoryBlocks[i];
		}

		return NullMem;
	}

	u8* GetMemFromAddr(const u64 addr)
	{
		return GetMemByAddr(addr).GetMemFromAddr(addr);
	}

	void* VirtualToRealAddr(const u64 vaddr)
	{
		return GetMemFromAddr(vaddr);
	}

	u64 RealToVirtualAddr(const void* addr)
	{
		const u64 raddr = (u64)addr;
		for(u32 i=0; i<MemoryBlocks.size(); ++i)
		{
			MemoryBlock& b = *MemoryBlocks[i];
			const u64 baddr = (u64)b.GetMem();

			if(raddr >= baddr && raddr < baddr + b.GetSize())
			{
				return b.GetStartAddr() + (raddr - baddr);
			}
		}

		return 0;
	}

	bool InitSpuRawMem(const u32 max_spu_raw)
	{
		//if(SpuRawMem.GetSize()) return false;

		MemoryBlocks.push_back(SpuRawMem.SetRange(0xe0000000, 0x100000 * max_spu_raw));

		return true;
	}

	void Init(MemoryType type)
	{
		if(m_inited) return;
		m_inited = true;

		ConLog.Write("Initing memory...");

		switch(type)
		{
		case Memory_PS3:
			MemoryBlocks.push_back(MainMem.SetRange(0x00010000, 0x2FFF0000));
			MemoryBlocks.push_back(UserMemory = PRXMem.SetRange(0x30000000, 0x10000000));
			MemoryBlocks.push_back(RSXCMDMem.SetRange(0x40000000, 0x10000000));
			MemoryBlocks.push_back(MmaperMem.SetRange(0xB0000000, 0x10000000));
			MemoryBlocks.push_back(RSXFBMem.SetRange(0xC0000000, 0x10000000));
			MemoryBlocks.push_back(StackMem.SetRange(0xD0000000, 0x10000000));
			//MemoryBlocks.push_back(SpuRawMem.SetRange(0xE0000000, 0x10000000));
			//MemoryBlocks.push_back(SpuThrMem.SetRange(0xF0000000, 0x10000000));
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

		ConLog.Write("Memory initialized.");
	}

	bool IsGoodAddr(const u64 addr)
	{
		for(uint i=0; i<MemoryBlocks.size(); ++i)
		{
			if(MemoryBlocks[i]->IsMyAddress(addr)) return true;
		}

		return false;
	}

	bool IsGoodAddr(const u64 addr, const u32 size)
	{
		for(uint i=0; i<MemoryBlocks.size(); ++i)
		{
			if( MemoryBlocks[i]->IsMyAddress(addr) &&
				MemoryBlocks[i]->IsMyAddress(addr + size - 1) ) return true;
		}

		return false;
	}

	void Close()
	{
		if(!m_inited) return;
		m_inited = false;

		ConLog.Write("Closing memory...");

		for(uint i=0; i<MemoryBlocks.size(); ++i)
		{
			MemoryBlocks[i]->Delete();
		}

		MemoryBlocks.clear();
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

	bool CopyToReal(void* real, u32 from, u32 count) // (4K pages) copy from virtual to real memory
	{
		if (!count) return true;

		u8* to = (u8*)real;

		if (u32 frag = from & 4095)
		{
			if (!IsGoodAddr(from)) return false;
			u32 num = 4096 - frag;
			if (count < num) num = count;
			memcpy(to, GetMemFromAddr(from), num);
			to += num;
			from += num;
			count -= num;
		}

		for (u32 page = count / 4096; page > 0; page--)
		{
			if (!IsGoodAddr(from)) return false;
			memcpy(to, GetMemFromAddr(from), 4096);
			to += 4096;
			from += 4096;
			count -= 4096;
		}

		if (count)
		{
			if (!IsGoodAddr(from)) return false;
			memcpy(to, GetMemFromAddr(from), count);
		}

		return true;
	}

	bool CopyFromReal(u32 to, void* real, u32 count) // (4K pages) copy from real to virtual memory
	{
		if (!count) return true;

		u8* from = (u8*)real;

		if (u32 frag = to & 4095)
		{
			if (!IsGoodAddr(to)) return false;
			u32 num = 4096 - frag;
			if (count < num) num = count;
			memcpy(GetMemFromAddr(to), from, num);
			to += num;
			from += num;
			count -= num;
		}

		for (u32 page = count / 4096; page > 0; page--)
		{
			if (!IsGoodAddr(to)) return false;
			memcpy(GetMemFromAddr(to), from, 4096);
			to += 4096;
			from += 4096;
			count -= 4096;
		}

		if (count)
		{
			if (!IsGoodAddr(to)) return false;
			memcpy(GetMemFromAddr(to), from, count);
		}

		return true;

	}

	bool Copy(u32 to, u32 from, u32 count) // (4K pages) copy from virtual to virtual memory through real
	{
		if (u8* buf = (u8*)malloc(count))
		{
			if (CopyToReal(buf, from, count))
			{
				if (CopyFromReal(to, buf, count))
				{
					free(buf);
					return true;
				}
				else
				{
					free(buf);
					return false;
				}
			}
			else
			{
				free(buf);
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	void ReadLeft(u8* dst, const u64 addr, const u32 size)
	{
		MemoryBlock& mem = GetMemByAddr(addr);

		if(mem.IsNULL())
		{
			ConLog.Error("ReadLeft[%d] from null block (0x%llx)", size, addr);
			return;
		}

		for(u32 i=0; i<size; ++i) mem.Read8(addr + i, dst + size - 1 - i);
	}

	void WriteLeft(const u64 addr, const u32 size, const u8* src)
	{
		MemoryBlock& mem = GetMemByAddr(addr);

		if(mem.IsNULL())
		{
			ConLog.Error("WriteLeft[%d] to null block (0x%llx)", size, addr);
			return;
		}

		for(u32 i=0; i<size; ++i) mem.Write8(addr + i, src[size - 1 - i]);
	}

	void ReadRight(u8* dst, const u64 addr, const u32 size)
	{
		MemoryBlock& mem = GetMemByAddr(addr);

		if(mem.IsNULL())
		{
			ConLog.Error("ReadRight[%d] from null block (0x%llx)", size, addr);
			return;
		}

		for(u32 i=0; i<size; ++i) mem.Read8(addr + (size - 1 - i), dst + i);
	}

	void WriteRight(const u64 addr, const u32 size, const u8* src)
	{
		MemoryBlock& mem = GetMemByAddr(addr);

		if(mem.IsNULL())
		{
			ConLog.Error("WriteRight[%d] to null block (0x%llx)", size, addr);
			return;
		}

		for(u32 i=0; i<size; ++i) mem.Write8(addr + (size - 1 - i), src[i]);
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
		if(!IsGoodAddr(addr, str.length()))
		{
			ConLog.Error("Memory::WriteString error: bad address (0x%llx)", addr);
			return;
		}

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
		if(IsGoodAddr(dst_addr) || !IsGoodAddr(src_addr))
		{
			return false;
		}

		MemoryBlocks.push_back((new MemoryMirror())->SetRange(GetMemFromAddr(src_addr), dst_addr, size));
		ConLog.Warning("memory mapped 0x%llx to 0x%llx size=0x%x", src_addr, dst_addr, size);
		return true;
	}

	bool Unmap(const u64 addr)
	{
		for(uint i=0; i<MemoryBlocks.size(); ++i)
		{
			if(MemoryBlocks[i]->IsMirror())
			{
				if(MemoryBlocks[i]->GetStartAddr() == addr)
				{
					delete MemoryBlocks[i];
					MemoryBlocks.erase(MemoryBlocks.begin() + i);
				}
			}
		}
	}

	u8* operator + (const u64 vaddr)
	{
		u8* ret = GetMemFromAddr(vaddr);
		if(!ret) throw fmt::Format("GetMemFromAddr(0x%llx)", vaddr);
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
};

template<typename T, typename AT = u32>
class mem_ptr_t : public mem_base_t<T, AT>
{
public:
	mem_ptr_t(AT addr) : mem_base_t<T, AT>(addr)
	{
	}

	template<typename NT> operator mem_ptr_t<NT, AT>&() { return (mem_ptr_t<NT, AT>&)*this; }
	template<typename NT> operator const mem_ptr_t<NT, AT>&() const { return (const mem_ptr_t<NT, AT>&)*this; }

	T* operator -> ()
	{
		return (T*)&Memory[this->m_addr];
	}

	const T* operator -> () const
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

	T& operator *()
	{
		return (T&)Memory[this->m_addr];
	}

	const T& operator *() const
	{
		return (T&)Memory[this->m_addr];
	}

	T& operator [](uint index)
	{
		return (T&)Memory[this->m_addr + sizeof(T) * index];
	}

	const T& operator [](uint index) const
	{
		return (const T&)Memory[this->m_addr + sizeof(T) * index];
	}

	bool operator == (mem_ptr_t right) const { return this->m_addr == right.m_addr; }
	bool operator != (mem_ptr_t right) const { return this->m_addr != right.m_addr; }
	bool operator > (mem_ptr_t right) const { return this->m_addr > right.m_addr; }
	bool operator < (mem_ptr_t right) const { return this->m_addr < right.m_addr; }
	bool operator >= (mem_ptr_t right) const { return this->m_addr >= right.m_addr; }
	bool operator <= (mem_ptr_t right) const { return this->m_addr <= right.m_addr; }

	bool operator == (T* right) const { return (T*)&Memory[this->m_addr] == right; }
	bool operator != (T* right) const { return (T*)&Memory[this->m_addr] != right; }
	bool operator > (T* right) const { return (T*)&Memory[this->m_addr] > right; }
	bool operator < (T* right) const { return (T*)&Memory[this->m_addr] < right; }
	bool operator >= (T* right) const { return (T*)&Memory[this->m_addr] >= right; }
	bool operator <= (T* right) const { return (T*)&Memory[this->m_addr] <= right; }
};

template<typename AT>
class mem_ptr_t<void, AT> : public mem_base_t<u8, AT>
{
public:
	mem_ptr_t(AT addr) : mem_base_t<u8, AT>(addr)
	{
	}

	template<typename NT> operator mem_ptr_t<NT>&() { return (mem_ptr_t<NT>&)*this; }
	template<typename NT> operator const mem_ptr_t<NT>&() const { return (const mem_ptr_t<NT>&)*this; }

	bool operator == (mem_ptr_t right) const { return this->m_addr == right.m_addr; }
	bool operator != (mem_ptr_t right) const { return this->m_addr != right.m_addr; }
	bool operator > (mem_ptr_t right) const { return this->m_addr > right.m_addr; }
	bool operator < (mem_ptr_t right) const { return this->m_addr < right.m_addr; }
	bool operator >= (mem_ptr_t right) const { return this->m_addr >= right.m_addr; }
	bool operator <= (mem_ptr_t right) const { return this->m_addr <= right.m_addr; }

	bool operator == (void* right) const { return (void*)&Memory[this->m_addr] == right; }
	bool operator != (void* right) const { return (void*)&Memory[this->m_addr] != right; }
	bool operator > (void* right) const { return (void*)&Memory[this->m_addr] > right; }
	bool operator < (void* right) const { return (void*)&Memory[this->m_addr] < right; }
	bool operator >= (void* right) const { return (void*)&Memory[this->m_addr] >= right; }
	bool operator <= (void* right) const { return (void*)&Memory[this->m_addr] <= right; }
};

template<typename T> static bool operator == (T* left, mem_ptr_t<T> right) { return left == (T*)&Memory[right.GetAddr()]; }
template<typename T> static bool operator != (T* left, mem_ptr_t<T> right) { return left != (T*)&Memory[right.GetAddr()]; }
template<typename T> static bool operator > (T* left, mem_ptr_t<T> right) { return left > (T*)&Memory[right.GetAddr()]; }
template<typename T> static bool operator < (T* left, mem_ptr_t<T> right) { return left < (T*)&Memory[right.GetAddr()]; }
template<typename T> static bool operator >= (T* left, mem_ptr_t<T> right) { return left >= (T*)&Memory[right.GetAddr()]; }
template<typename T> static bool operator <= (T* left, mem_ptr_t<T> right) { return left <= (T*)&Memory[right.GetAddr()]; }

template<typename T, typename AT = u32>
class mem_beptr_t : public mem_ptr_t<T, be_t<AT>> {};

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

template<typename T, typename AT=u32> class mem_list_ptr_t : public mem_base_t<T, AT>
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

template<typename T> struct _func_arg<mem_ptr_t<T, u32>> : public _func_arg<mem_base_t<T, u32>> {};
template<> struct _func_arg<mem_ptr_t<void, u32>> : public _func_arg<mem_base_t<u8, u32>> {};
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

template<typename T> class mem_func_ptr_t;

template<typename RT>
class mem_func_ptr_t<RT (*)()> : public mem_base_t<u64>
{
	__forceinline void call_func(bool is_async)
	{
		Callback cb;
		cb.SetAddr(m_addr);
		cb.Branch(!is_async);
	}

public:
	__forceinline void operator()()
	{
		call_func(false);
	}

	__forceinline void async()
	{
		call_func(true);
	}
};

template<typename RT, typename T1>
class mem_func_ptr_t<RT (*)(T1)> : public mem_base_t<u64>
{
	__forceinline void call_func(bool is_async, T1 a1)
	{
		Callback cb;
		cb.SetAddr(m_addr);
		cb.Handle(_func_arg<T1>::get_value(a1));
		cb.Branch(!is_async);
	}

public:
	__forceinline void operator()(T1 a1)
	{
		call_func(false, a1);
	}

	__forceinline void async(T1 a1)
	{
		call_func(true, a1);
	}
};

template<typename RT, typename T1, typename T2>
class mem_func_ptr_t<RT (*)(T1, T2)> : public mem_base_t<u64>
{
	__forceinline void call_func(bool is_async, T1 a1, T2 a2)
	{
		Callback cb;
		cb.SetAddr(m_addr);
		cb.Handle(_func_arg<T1>::get_value(a1), _func_arg<T2>::get_value(a2));
		cb.Branch(!is_async);
	}

public:
	__forceinline void operator()(T1 a1, T2 a2)
	{
		call_func(false, a1, a2);
	}

	__forceinline void async(T1 a1, T2 a2)
	{
		call_func(true, a1, a2);
	}
};

template<typename RT, typename T1, typename T2, typename T3>
class mem_func_ptr_t<RT (*)(T1, T2, T3)> : public mem_base_t<u64>
{
	__forceinline void call_func(bool is_async, T1 a1, T2 a2, T3 a3)
	{
		Callback cb;
		cb.SetAddr(m_addr);
		cb.Handle(_func_arg<T1>::get_value(a1), _func_arg<T2>::get_value(a2), _func_arg<T3>::get_value(a3));
		cb.Branch(!is_async);
	}

public:
	__forceinline void operator()(T1 a1, T2 a2, T3 a3)
	{
		call_func(false, a1, a2, a3);
	}

	__forceinline void async(T1 a1, T2 a2, T3 a3)
	{
		call_func(true, a1, a2, a3);
	}
};

template<typename RT, typename T1, typename T2, typename T3, typename T4>
class mem_func_ptr_t<RT (*)(T1, T2, T3, T4)> : public mem_base_t<u64>
{
	__forceinline void call_func(bool is_async, T1 a1, T2 a2, T3 a3, T4 a4)
	{
		Callback cb;
		cb.SetAddr(m_addr);
		cb.Handle(_func_arg<T1>::get_value(a1), _func_arg<T2>::get_value(a2), _func_arg<T3>::get_value(a3), _func_arg<T4>::get_value(a4));
		cb.Branch(!is_async);
	}

public:
	__forceinline void operator()(T1 a1, T2 a2, T3 a3, T4 a4)
	{
		call_func(false, a1, a2, a3, a4);
	}

	__forceinline void async(T1 a1, T2 a2, T3 a3, T4 a4)
	{
		call_func(true, a1, a2, a3, a4);
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
