#include "stdafx.h"
#include "Utilities/Log.h"
#include "vfsLocalFile.h"

vfsLocalFile::vfsLocalFile(vfsDevice* device) : vfsFileBase(device)
{
}

bool vfsLocalFile::Open(const std::string& path, u32 mode)
{
	Close();

	return m_file.open(path, mode) && vfsFileBase::Open(path, mode);
}

bool vfsLocalFile::Close()
{
	return m_file.close() && vfsFileBase::Close();
}

u64 vfsLocalFile::GetSize() const
{
	return m_file.size();
}

u64 vfsLocalFile::Write(const void* src, u64 size)
{
	return m_file.write(src, size);
}

u64 vfsLocalFile::Read(void* dst, u64 size)
{
	return m_file.read(dst, size);
}

u64 vfsLocalFile::Seek(s64 offset, fsm mode)
{
	return m_file.seek(offset, mode);
}

u64 vfsLocalFile::Tell() const
{
	return m_file.seek(0, fsm::cur);
}

bool vfsLocalFile::IsOpened() const
{
	return m_file && vfsFileBase::IsOpened();
}
