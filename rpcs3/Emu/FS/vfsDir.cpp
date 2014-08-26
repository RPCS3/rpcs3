#include "stdafx.h"
#include "Emu/System.h"

#include "VFS.h"
#include "vfsDir.h"

vfsDir::vfsDir()
	: vfsDirBase(nullptr)
	, m_stream(nullptr)
{
	// TODO: proper implementation
	// m_stream is nullptr here. So open root until a proper dir is given
	Open("/");
}

vfsDir::vfsDir(const std::string& path)
	: vfsDirBase(nullptr)
	, m_stream(nullptr)
{
	Open(path);
}

bool vfsDir::Open(const std::string& path)
{
	Close();

	m_stream.reset(Emu.GetVFS().OpenDir(path));

	return m_stream && m_stream->IsOpened();
}

bool vfsDir::Create(const std::string& path)
{
	return m_stream->Create(path);
}

bool vfsDir::IsExists(const std::string& path) const
{
	return m_stream->IsExists(path);
}

const std::vector<DirEntryInfo>& vfsDir::GetEntries() const
{
	return m_stream->GetEntries();
}

bool vfsDir::Rename(const std::string& from, const std::string& to)
{
	return m_stream->Rename(from, to);
}

bool vfsDir::Remove(const std::string& path)
{
	return m_stream->Remove(path);
}

const DirEntryInfo* vfsDir::Read()
{
	return m_stream->Read();
}

void vfsDir::Close()
{
	m_stream.reset();
}

std::string vfsDir::GetPath() const
{
	return m_stream->GetPath();
}

bool vfsDir::IsOpened() const
{
	return m_stream && m_stream->IsOpened();
}
