#include "stdafx.h"
#include "rsx_cache.h"
#include "Emu/System.h"

namespace rsx
{
	std::string shaders_cache::path_to_root()
	{
		return fs::get_executable_dir() + "data/";
	}

	void shaders_cache::load(const std::string &path, decompile_language lang)
	{
		fs::create_path(path);

		return;

		std::string lang_name = convert::to<std::string>(lang);

		auto extract_hash = [](const std::string &string)
		{
			return std::stoull(string.substr(0, string.find('.')).c_str(), 0, 16);
		};

		for (const fs::dir::entry &entry : fs::dir{ path })
		{
			if (entry.name == "." || entry.name == "..")
				continue;

			u64 hash;

			try
			{
				hash = extract_hash(entry.name);
			}
			catch (...)
			{
				LOG_ERROR(RSX, "Cache file '%s' ignored", entry.name);
				continue;
			}

			if (fmt::match(entry.name, "*.fs." + lang_name))
			{
				fs::file file{ path + entry.name };
				decompiled_shader shader;
				shader.code_language = lang;
				shader.code = (const std::string)file;
				fragment_shaders.decompiled.insert(hash, shader);
				continue;
			}

			if (fmt::match(entry.name, "*.vs." + lang_name))
			{
				fs::file file{ path + entry.name };
				decompiled_shader shader;
				shader.code_language = lang;
				shader.code = (const std::string)file;
				vertex_shaders.decompiled.insert(hash, shader);

				continue;
			}
		}
	}

	void shaders_cache::load(decompile_language lang)
	{
		std::string root = path_to_root();

		//shared cache
		load(root + "cache/", lang);

		std::string title_id = Emu.GetTitleID();

		if (title_id.empty())
		{
			title_id = "temporary";
		}

		load(root + title_id + "/cache/", lang);
	}
}