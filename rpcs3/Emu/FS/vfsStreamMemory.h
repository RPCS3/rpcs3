#pragma once
#include "vfsStream.h"

struct vfsStreamMemory : public vfsStream
{
	u64 m_addr;

public:
	vfsStreamMemory();
	vfsStreamMemory(u64 addr);

	void Open(u64 addr);

	virtual u64 Write(const void* src, u64 size) override;
	virtual u64 Read(void* dst, u64 size) override;
};