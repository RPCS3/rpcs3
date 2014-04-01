#include "stdafx.h"
#include "TRP.h"

TRPLoader::TRPLoader(vfsStream& f) : trp_f(f)
{
}

TRPLoader::~TRPLoader()
{
	Close();
}

bool TRPLoader::Install(std::string dest, bool show)
{
	if(!trp_f.IsOpened())
		return false;

	if (!dest.empty() && dest.back() != '/')
		dest += '/';

	Emu.GetVFS().CreateDir(dest);
	for (const TRPEntry& entry : m_entries)
	{
		char* buffer = new char [entry.size];
		Emu.GetVFS().CreateFile(dest+entry.name);
		vfsFile file(dest+entry.name, vfsWrite);
		trp_f.Seek(entry.offset);
		trp_f.Read(buffer, entry.size);
		file.Write(buffer, entry.size);
		file.Close();
		delete[] buffer;
	}

	return true;
}

bool TRPLoader::LoadHeader(bool show)
{
	if(!trp_f.IsOpened())
		return false;

	trp_f.Seek(0);
	if (trp_f.Read(&m_header, sizeof(TRPHeader)) != sizeof(TRPHeader))
		return false;

	if (m_header.trp_magic != 0xDCA24D00)
		return false;

	if (show)
		ConLog.Write("TRP version: %x", m_header.trp_version);

	m_entries.clear();
	m_entries.resize(m_header.trp_files_count);

	for(u32 i=0; i<m_header.trp_files_count; i++)
	{
		if (trp_f.Read(&m_entries[i], sizeof(TRPEntry)) != sizeof(TRPEntry))
			return false;

		if (show)
			ConLog.Write("TRP entry #%d: %s", m_entries[i].name);
	}

	return true;
}

bool TRPLoader::ContainsEntry(char *filename)
{
	for (const TRPEntry& entry : m_entries) {
		if (!strcmp(entry.name, filename))
			return true;
	}
	return false;
}

void TRPLoader::RemoveEntry(char *filename)
{
	std::vector<TRPEntry>::iterator i = m_entries.begin();
	while (i != m_entries.end()) {
		if (!strcmp(i->name, filename))
			i = m_entries.erase(i);
		else
			i++;
	}
}

void TRPLoader::RenameEntry(char *oldname, char *newname)
{
	for (const TRPEntry& entry : m_entries) {
		if (!strcmp(entry.name, oldname))
			memcpy((void*)entry.name, newname, 32);
	}
}

bool TRPLoader::Close()
{
	return trp_f.Close();
}
