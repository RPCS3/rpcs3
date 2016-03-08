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

void vfsDir::Close()
{
	m_stream.reset();
}

bool vfsDir::IsOpened() const
{
	return !m_entries.empty();
}
