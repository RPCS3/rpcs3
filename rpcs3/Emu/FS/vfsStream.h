#pragma once

#include "Utilities/File.h"

struct vfsStream
{
	vfsStream() = default;

	virtual ~vfsStream()
	{
		Close();
	}

	virtual bool Close()
	{
		return true;
	}

	virtual u64 GetSize() const = 0;

	virtual u64 Write(const void* src, u64 count) = 0;

	template<typename T> force_inline bool SWrite(const T& data, u64 count = sizeof(T))
	{
		return Write(&data, count) == count;
	}

	virtual u64 Read(void* dst, u64 count) = 0;

	template<typename T> force_inline bool SRead(T& data, u64 count = sizeof(T))
	{
		return Read(&data, count) == count;
	}

	virtual u64 Seek(s64 offset, fsm seek_mode = fsm::begin) = 0;

	virtual u64 Tell() const = 0;

	virtual bool Eof() const
	{
		return Tell() >= GetSize();
	}

	virtual bool IsOpened() const = 0;
};
