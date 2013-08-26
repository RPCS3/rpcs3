#pragma once

struct MemInfo
{
	u64 addr;
	u32 size;

	MemInfo(u64 _addr, u32 _size)
		: addr(_addr)
		, size(_size)
	{
	}

	MemInfo()
	{
	}
};

struct MemBlockInfo : public MemInfo
{
	void* mem;

	MemBlockInfo(u64 _addr, u32 _size)
		: MemInfo(_addr, _size)
		, mem(malloc(_size))
	{
		if(!mem)
		{
			ConLog.Error("Not enought free memory.");
			assert(0);
		}
		
		memset(mem, 0, size);
	}

	~MemBlockInfo()
	{
		free(mem);
		mem = nullptr;
	}
};

class MemoryBlock
{
protected:
	u8* mem;
	u64 range_start;
	u64 range_size;

public:
	MemoryBlock();
	virtual ~MemoryBlock();

private:
	void Init();
	void InitMemory();

public:
	virtual void Delete();

	virtual bool IsNULL() { return false; }
	virtual bool IsMirror() { return false; }

	u64 FixAddr(const u64 addr) const;

	bool GetMemFromAddr(void* dst, const u64 addr, const u32 size);
	bool SetMemFromAddr(void* src, const u64 addr, const u32 size);
	bool GetMemFFromAddr(void* dst, const u64 addr);
	u8* GetMemFromAddr(const u64 addr);

	virtual MemoryBlock* SetRange(const u64 start, const u32 size);
	virtual bool IsMyAddress(const u64 addr);

	__forceinline const u8 FastRead8(const u64 addr) const;
	__forceinline const u16 FastRead16(const u64 addr) const;
	__forceinline const u32 FastRead32(const u64 addr) const;
	__forceinline const u64 FastRead64(const u64 addr) const;
	__forceinline const u128 FastRead128(const u64 addr);

	virtual bool Read8(const u64 addr, u8* value);
	virtual bool Read16(const u64 addr, u16* value);
	virtual bool Read32(const u64 addr, u32* value);
	virtual bool Read64(const u64 addr, u64* value);
	virtual bool Read128(const u64 addr, u128* value);

	__forceinline void FastWrite8(const u64 addr, const u8 value);
	__forceinline void FastWrite16(const u64 addr, const u16 value);
	__forceinline void FastWrite32(const u64 addr, const u32 value);
	__forceinline void FastWrite64(const u64 addr, const u64 value);
	__forceinline void FastWrite128(const u64 addr, const u128 value);

	virtual bool Write8(const u64 addr, const u8 value);
	virtual bool Write16(const u64 addr, const u16 value);
	virtual bool Write32(const u64 addr, const u32 value);
	virtual bool Write64(const u64 addr, const u64 value);
	virtual bool Write128(const u64 addr, const u128 value);

	const u64 GetStartAddr() const { return range_start; }
	const u64 GetEndAddr() const { return GetStartAddr() + GetSize() - 1; }
	virtual const u32 GetSize() const { return range_size; }
	u8* GetMem() const { return mem; }
	virtual u8* GetMem(u64 addr) const { return mem + addr; }
};

class MemoryMirror : public MemoryBlock
{
public:
	virtual bool IsMirror() { return true; }

	virtual MemoryBlock* SetRange(const u64 start, const u32 size)
	{
		range_start = start;
		range_size = size;

		return this;
	}

	void SetMemory(u8* memory)
	{
		mem = memory;
	}

	MemoryBlock* SetRange(u8* memory, const u64 start, const u32 size)
	{
		SetMemory(memory);
		return SetRange(start, size);
	}
};

class NullMemoryBlock : public MemoryBlock
{
	virtual bool IsNULL() { return true; }
	virtual bool IsMyAddress(const u64 addr) { return true; }

	virtual bool Read8(const u64 addr, u8* value);
	virtual bool Read16(const u64 addr, u16* value);
	virtual bool Read32(const u64 addr, u32* value);
	virtual bool Read64(const u64 addr, u64* value);
	virtual bool Read128(const u64 addr, u128* value);

	virtual bool Write8(const u64 addr, const u8 value);
	virtual bool Write16(const u64 addr, const u16 value);
	virtual bool Write32(const u64 addr, const u32 value);
	virtual bool Write64(const u64 addr, const u64 value);
	virtual bool Write128(const u64 addr, const u128 value);
};

class DynamicMemoryBlock : public MemoryBlock
{
	Array<MemBlockInfo> m_used_mem;
	u32 m_max_size;

public:
	DynamicMemoryBlock();

	const u32 GetSize() const { return m_max_size; }
	const u32 GetUsedSize() const;

	bool IsInMyRange(const u64 addr);
	bool IsInMyRange(const u64 addr, const u32 size);
	bool IsMyAddress(const u64 addr);

	MemoryBlock* SetRange(const u64 start, const u32 size);

	virtual void Delete();

	bool Alloc(u64 addr, u32 size);
	u64 Alloc(u32 size);
	bool Alloc();
	bool Free(u64 addr);

	virtual u8* GetMem(u64 addr) const;

private:
	void AppendUsedMem(u64 addr, u32 size);
};
