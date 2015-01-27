#pragma once
#include "vfsStream.h"

struct vfsStreamMemory : public vfsStream
{
	u32 m_addr;
	u64 m_size;

public:
	vfsStreamMemory();
	vfsStreamMemory(u32 addr, u32 size = 0);

	void Open(u32 addr, u32 size = 0);

	virtual u64 GetSize() override;

	virtual u64 Write(const void* src, u64 size) override;
	virtual u64 Read(void* dst, u64 size) override;
};