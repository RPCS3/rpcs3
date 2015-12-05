#pragma once
#include "vfsStream.h"

class vfsStreamMemory : public vfsStream
{
	u64 m_pos = 0;
	u32 m_addr = 0;
	u64 m_size = 0;

public:
	vfsStreamMemory() = default;

	vfsStreamMemory(u32 addr, u32 size = 0)
	{
		Open(addr, size);
	}

	void Open(u32 addr, u32 size = 0)
	{
		m_pos = 0;
		m_addr = addr;
		m_size = size ? size : 0x100000000ull - addr; // determine max possible size
	}

	virtual u64 GetSize() const override
	{
		return m_size;
	}

	virtual u64 Write(const void* src, u64 count) override;

	virtual u64 Read(void* dst, u64 count) override;

	virtual u64 Seek(s64 offset, fsm seek_mode = fsm::begin) override
	{
		switch (seek_mode)
		{
		case fsm::begin: return m_pos = offset;
		case fsm::cur: return m_pos += offset;
		case fsm::end: return m_pos = m_size + offset;
		}

		throw EXCEPTION("Unknown seek_mode (0x%x)", seek_mode);
	}

	virtual u64 Tell() const override
	{
		return m_pos;
	}

	virtual bool IsOpened() const override
	{
		return true;
	}
};
