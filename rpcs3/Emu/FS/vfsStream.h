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
	virtual u64 Read(void* dst, u64 size);

	virtual u64 Seek(s64 offset, vfsSeekMode mode = vfsSeekSet);
	virtual u64 Tell() const;
	virtual bool Eof();

	virtual bool IsOpened() const;
};