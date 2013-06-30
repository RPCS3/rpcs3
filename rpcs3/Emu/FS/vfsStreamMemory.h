#pragma once
#include "vfsStream.h"

struct vfsStreamMemory : public vfsStream
{
	u64 m_addr;

public:
	vfsStreamMemory();
	vfsStreamMemory(u64 addr);

	void Open(u64 addr);

	virtual u32 Write(const void* src, u32 size);
	virtual u32 Read(void* dst, u32 size);
};