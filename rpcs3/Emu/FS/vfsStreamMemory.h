#pragma once
#include "vfsStream.h"

struct vfsStreamMemory : public vfsStream
{
	u64 m_addr;
	u64 m_size;

public:
	vfsStreamMemory();
	vfsStreamMemory(u64 addr, u64 size = 0);

	void Open(u64 addr, u64 size = 0);

	virtual u64 GetSize() override;

	virtual u64 Write(const void* src, u64 size) override;
	virtual u64 Read(void* dst, u64 size) override;
};