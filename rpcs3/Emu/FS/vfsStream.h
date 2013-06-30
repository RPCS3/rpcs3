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

	virtual u32 Write(const void* src, u32 size);
	virtual u32 Read(void* dst, u32 size);

	virtual u64 Seek(s64 offset, vfsSeekMode mode = vfsSeekSet);
	virtual u64 Tell() const;
	virtual bool Eof();

	virtual bool IsOpened() const;
};