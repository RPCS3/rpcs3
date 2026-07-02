#include "stdafx.h"

#include "Utilities/File.h"
#include "util/logs.hpp"
#include "Emu/VFS.h"

#include "mself.hpp"

LOG_CHANNEL(mself_log, "MSELF");

bool extract_mself(std::string_view file, const std::string& extract_to)
{
	fs::file mself(file);

	mself_log.notice("Extracting MSELF file '%s' to directory '%s'...", file, extract_to);

	if (!mself)
	{
		mself_log.error("Error opening MSELF file '%s' (%s)", file, fs::g_tls_error);
		return false;
	}

	mself_header hdr{};

	if (!mself.read(hdr))
	{
		mself_log.error("Error reading MSELF header, file is too small. (size=0x%x)", mself.size());
		return false;
	}

	const u64 mself_size = mself.size();
	const u32 hdr_count = hdr.get_count(mself_size);

	if (!hdr_count)
	{
		mself_log.error("Provided file is not an MSELF");
		return false;
	}

	std::vector<mself_record> recs(hdr_count);

	if (!mself.read(recs))
	{
		mself_log.error("Error extracting MSELF records");
		return false;
	}

	std::vector<u8> buffer;

	for (const mself_record& rec : recs)
	{
		const std::string name = vfs::escape(rec.name);

		const u64 pos = rec.get_pos(mself_size);

		if (!pos)
		{
			mself_log.error("Error extracting %s from MSELF", name);
			return false;
		}

		buffer.resize(rec.size);
		mself.seek(pos);
		mself.read(buffer.data(), rec.size);

		const std::string path = extract_to + name;

		if (!fs::create_path(fs::get_parent_dir(path)))
		{
			mself_log.error("Error creating directory %s (%s)", fs::get_parent_dir(path), fs::g_tls_error);
			return false;
		}

		if (!fs::write_file(path, fs::rewrite, buffer))
		{
			mself_log.error("Error creating %s (%s)", path, fs::g_tls_error);
			return false;
		}

		mself_log.success("Extracted '%s' to '%s'", name, path);
	}

	mself_log.success("Extraction complete!");
	return true;
}
