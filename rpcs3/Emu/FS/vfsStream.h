#pragma once

enum vfsSeekMode
{
	vfsSeekSet,
	vfsSeekCur,
	vfsSeekEnd,
};

struct vfsStream
{
protected:
	u64 m_pos;

public:
	vfsStream();

	virtual ~vfsStream();

	virtual void Reset();
	virtual bool Close();

	virtual u64 GetSize();

	virtual u64 Write(const void* src, u64 size);

	template<typename T> __forceinline bool SWrite(const T& data, u64 size = sizeof(T))
	{
		return Write(&data, size) == size;
	}

	virtual u64 Read(void* dst, u64 size);

	template<typename T> __forceinline bool SRead(T& data, u64 size = sizeof(T))
	{
		return Read(&data, size) == size;
	}

	virtual u64 Seek(s64 offset, vfsSeekMode mode = vfsSeekSet);
	virtual u64 Tell() const;
	virtual bool Eof();

	virtual bool IsOpened() const;
};
