#pragma once
#include "MemoryBlock.h"

class MemoryBase
{
	NullMemoryBlock NullMem;

public:
	ArrayF<MemoryBlock> MemoryBlocks;

	DynamicMemoryBlock MainMem;
	DynamicMemoryBlock PRXMem;
	DynamicMemoryBlock RSXCMDMem;
	DynamicMemoryBlock MmaperMem;
	DynamicMemoryBlock RSXFBMem;
	DynamicMemoryBlock StackMem;
	MemoryBlock SpuRawMem;
	MemoryBlock SpuThrMem;

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
	template<> static __forceinline u64 ReverseData<1>(u64 val) { return val; }
	template<> static __forceinline u64 ReverseData<2>(u64 val) { return Reverse16(val); }
	template<> static __forceinline u64 ReverseData<4>(u64 val) { return Reverse32(val); }
	template<> static __forceinline u64 ReverseData<8>(u64 val) { return Reverse64(val); }

	template<typename T> static __forceinline T Reverse(T val)
	{
		return (T)ReverseData<sizeof(T)>(val);
	};

	MemoryBlock& GetMemByNum(const u8 num)
	{
		if(num >= MemoryBlocks.GetCount()) return NullMem;
		return MemoryBlocks.Get(num);
	}

	MemoryBlock& GetMemByAddr(const u64 addr)
	{
		for(uint i=0; i<MemoryBlocks.GetCount(); ++i)
		{
			if(MemoryBlocks.Get(i).IsMyAddress(addr)) return MemoryBlocks[i];
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
		const u32 raddr = (u32)addr;
		for(u32 i=0; i<MemoryBlocks.GetCount(); ++i)
		{
			MemoryBlock& b = MemoryBlocks[i];
			const u32 baddr = (u32)b.GetMem();

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

		MemoryBlocks.Add(SpuRawMem.SetRange(0xe0000000, 0x100000 * max_spu_raw));

		return true;
	}

	void Init()
	{
		if(m_inited) return;
		m_inited = true;

		ConLog.Write("Initing memory...");

		MemoryBlocks.Add(MainMem.SetRange(0x00010000, 0x2FFF0000));
		MemoryBlocks.Add(PRXMem.SetRange(0x30000000, 0x10000000));
		MemoryBlocks.Add(RSXCMDMem.SetRange(0x40000000, 0x10000000));
		MemoryBlocks.Add(MmaperMem.SetRange(0xB0000000, 0x10000000));
		MemoryBlocks.Add(RSXFBMem.SetRange(0xC0000000, 0x10000000));
		MemoryBlocks.Add(StackMem.SetRange(0xD0000000, 0x10000000));
		//MemoryBlocks.Add(SpuRawMem.SetRange(0xE0000000, 0x10000000));
		//MemoryBlocks.Add(SpuThrMem.SetRange(0xF0000000, 0x10000000));

		ConLog.Write("Memory initialized.");
	}

	bool IsGoodAddr(const u64 addr)
	{
		for(uint i=0; i<MemoryBlocks.GetCount(); ++i)
		{
			if(MemoryBlocks[i].IsMyAddress(addr)) return true;
		}

		return false;
	}

	bool IsGoodAddr(const u64 addr, const u32 size)
	{
		for(uint i=0; i<MemoryBlocks.GetCount(); ++i)
		{
			if( MemoryBlocks[i].IsMyAddress(addr) &&
				MemoryBlocks[i].IsMyAddress(addr + size - 1) ) return true;
		}

		return false;
	}

	void Close()
	{
		if(!m_inited) return;
		m_inited = false;

		ConLog.Write("Closing memory...");

		for(uint i=0; i<MemoryBlocks.GetCount(); ++i)
		{
			MemoryBlocks[i].Delete();
		}

		MemoryBlocks.Clear();
	}

	void Reset()
	{
		if(!m_inited) return;

		ConLog.Write("Resetting memory...");
		Close();
		Init();
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

	void ReadLeft(u8* dst, const u64 addr, const u32 size)
	{
		MemoryBlock& mem = GetMemByAddr(addr);

		if(mem.IsNULL())
		{
			ConLog.Error("ReadLeft[%d] from null block (0x%llx)", size, addr);
			return;
		}

		u32 offs = mem.FixAddr(addr);

		for(u32 i=0; i<size; ++i) dst[size - 1 - i] = mem.FastRead8(offs + i);
	}

	void WriteLeft(const u64 addr, const u32 size, const u8* src)
	{
		MemoryBlock& mem = GetMemByAddr(addr);

		if(mem.IsNULL())
		{
			ConLog.Error("WriteLeft[%d] to null block (0x%llx)", size, addr);
			return;
		}

		u32 offs = mem.FixAddr(addr);

		for(u32 i=0; i<size; ++i) mem.FastWrite8(offs + i, src[size - 1 - i]);
	}

	void ReadRight(u8* dst, const u64 addr, const u32 size)
	{
		MemoryBlock& mem = GetMemByAddr(addr);

		if(mem.IsNULL())
		{
			ConLog.Error("ReadRight[%d] from null block (0x%llx)", size, addr);
			return;
		}

		u32 offs = mem.FixAddr(addr);

		for(u32 i=0; i<size; ++i) dst[i] = mem.FastRead8(offs + (size - 1 - i));
	}

	void WriteRight(const u64 addr, const u32 size, const u8* src)
	{
		MemoryBlock& mem = GetMemByAddr(addr);

		if(mem.IsNULL())
		{
			ConLog.Error("WriteRight[%d] to null block (0x%llx)", size, addr);
			return;
		}

		u32 offs = mem.FixAddr(addr);

		for(u32 i=0; i<size; ++i) mem.FastWrite8(offs + (size - 1 - i), src[i]);
	}

	template<typename T> void WriteData(const u64 addr, const T* data)
	{
		memcpy(GetMemFromAddr(addr), data, sizeof(T));
	}

	template<typename T> void WriteData(const u64 addr, const T data)
	{
		*(T*)GetMemFromAddr(addr) = data;
	}

	wxString ReadString(const u64 addr, const u64 len)
	{
		wxString ret = wxEmptyString;

		if(len) memcpy(wxStringBuffer(ret, len), GetMemFromAddr(addr), len);

		return ret;
	}

	wxString ReadString(const u64 addr)
	{
		return wxString((const char*)GetMemFromAddr(addr));
	}

	void WriteString(const u64 addr, const wxString& str)
	{
		if(!IsGoodAddr(addr, str.Len()))
		{
			ConLog.Error("Memory::WriteString error: bad address (0x%llx)", addr);
			return;
		}

		strcpy((char*)GetMemFromAddr(addr), str);
	}

	static u64 AlignAddr(const u64 addr, const u64 align)
	{
		return (addr + (align-1)) & ~(align-1);
	}

	u32 GetUserMemTotalSize()
	{
		return PRXMem.GetSize();
	}

	u32 GetUserMemAvailSize()
	{
		return PRXMem.GetSize() - PRXMem.GetUsedSize();
	}

	u64 Alloc(const u32 size, const u32 align)
	{
		return PRXMem.Alloc(AlignAddr(size, align));
	}

	bool Free(const u64 addr)
	{
		return PRXMem.Free(addr);
	}

	bool Map(const u64 dst_addr, const u64 src_addr, const u32 size)
	{
		if(IsGoodAddr(dst_addr) || !IsGoodAddr(src_addr))
		{
			return false;
		}

		MemoryBlocks.Add((new MemoryMirror())->SetRange(GetMemFromAddr(src_addr), dst_addr, size));
		return true;
	}

	bool Unmap(const u64 addr)
	{
		for(uint i=0; i<MemoryBlocks.GetCount(); ++i)
		{
			if(MemoryBlocks[i].IsMirror())
			{
				if(MemoryBlocks[i].GetStartAddr() == addr)
				{
					MemoryBlocks.RemoveFAt(i);
				}
			}
		}
	}

	u8* operator + (const u64 vaddr)
	{
		u8* ret = GetMemFromAddr(vaddr);
		if(!ret) throw wxString::Format("GetMemFromAddr(0x%llx)", vaddr);
		return ret;
	}

	u8& operator[] (const u64 vaddr)
	{
		return *(*this + vaddr);
	}
};

extern MemoryBase Memory;

template<typename T> class mem_t
{
	u64 addr;
	const u64 iaddr;

public:
	mem_t(u64 _addr)
		: addr(_addr)
		, iaddr(_addr)
	{
	}

	void operator = (T right)
	{
		switch(sizeof(T))
		{
		case 1: Memory.Write8(addr, right); return;
		case 2: Memory.Write16(addr, right); return;
		case 4: Memory.Write32(addr, right); return;
		case 8: Memory.Write64(addr, right); return;
		}

		ConLog.Error("Bad mem_t size! (%d : 0x%llx)", sizeof(T), addr);
	}

	operator u8() const { return Memory.Read8(addr); }
	operator u16() const { return Memory.Read16(addr); }
	operator u32() const { return Memory.Read32(addr); }
	operator u64() const { return Memory.Read64(addr); }

	/*
	u64 operator += (u64 right)
	{
		addr += right;
		return addr;
	}
	*/
	u64 operator += (T right)
	{
		*this = right;
		addr += sizeof(T);
		return addr;
	}

	T operator [] (u64 i)
	{
		const u64 offset = i*sizeof(T);
		addr += offset;
		const T ret = *this;
		addr -= offset;
		return ret;
	}

	void Reset() { addr = iaddr; }
	u64 GetCurAddr() const { return addr; }
	u64 GetAddr() const { return iaddr; }
	u64 SetOffset(const u32 offset) { return addr += offset; }
};

class mem_class_t
{
	u64 addr;
	const u64 iaddr;

public:
	mem_class_t(u64 _addr)
		: addr(_addr)
		, iaddr(_addr)
	{
	}

	template<typename T> u64 operator += (T right)
	{
		mem_t<T> m(addr);
		m = right;
		addr += sizeof(T);
		return addr;
	}

	template<typename T> operator T()
	{
		mem_t<T> m(addr);
		const T ret = m;
		addr += sizeof(T);
		return ret;
	}

	void Reset() { addr = iaddr; }
	u64 GetCurAddr() const { return addr; }
	u64 GetAddr() const { return iaddr; }
	void SetAddr(const u64 _addr) { addr = _addr; }
};

typedef mem_t<u8> mem8_t;
typedef mem_t<u16> mem16_t;
typedef mem_t<u32> mem32_t;
typedef mem_t<u64> mem64_t;