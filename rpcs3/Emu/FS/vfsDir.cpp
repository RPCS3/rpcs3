#include "stdafx.h"
#include "Emu/System.h"

#include "vfsDevice.h"
#include "VFS.h"
#include "vfsDir.h"

vfsDir::vfsDir()
	: vfsDirBase(nullptr)
	, m_stream(nullptr)
{
	// TODO: proper implementation
	// m_stream is nullptr here. So open root until a proper dir is given
	//Open("/");
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

	DirEntryInfo info;

	m_cwd = simplify_path(Emu.GetVFS().GetLinked(0 && m_stream && m_stream->IsOpened() ? m_stream->GetPath() : path), true, true);

	auto blocks = simplify_path_blocks(GetPath());

	for (auto dev : Emu.GetVFS().m_devices)
	{
		auto dev_blocks = simplify_path_blocks(dev->GetPs3Path());

		if (dev_blocks.size() < (blocks.size() + 1))
		{
			continue;
		}

		bool is_ok = true;

		for (size_t i = 0; i < blocks.size(); ++i)
		{
			if (strcmp(dev_blocks[i].c_str(), blocks[i].c_str()))
			{
				is_ok = false;
				break;
			}
		}

		if (is_ok)
		{
			info.name = dev_blocks[blocks.size()];
			m_entries.push_back(info);
		}
	}

	if (m_stream && m_stream->IsOpened())
	{
		m_entries.insert(m_entries.begin(), m_stream->GetEntries().begin(), m_stream->GetEntries().end());
	}

	return !m_entries.empty();
}

bool vfsDir::Create(const std::string& path)
{
	return Emu.GetVFS().CreateDir(path);
}

bool vfsDir::IsExists(const std::string& path) const
{
	auto path_blocks = simplify_path_blocks(path);

	if (path_blocks.empty())
		return false;

	std::string dir_name = path_blocks[path_blocks.size() - 1];

	for (const auto entry : vfsDir(path + "/.."))
	{
		if (!strcmp(entry->name.c_str(), dir_name.c_str()))
			return true;
	}

	return false;
}

bool vfsDir::Rename(const std::string& from, const std::string& to)
{
	return Emu.GetVFS().RenameDir(from, to);
}

bool vfsDir::Remove(const std::string& path)
{
	return Emu.GetVFS().RemoveDir(path);
}

void vfsDir::Close()
{
	m_stream.reset();
}

bool vfsDir::IsOpened() const
{
	return !m_entries.empty();
}
