#pragma once
#include "Utilities/convert.h"
#include <rsx_decompiler.h>

namespace convert
{
	template<>
	struct to_impl_t<rsx::decompile_language, std::string>
	{
		static rsx::decompile_language func(const std::string &from)
		{
			if (from == "glsl")
				return rsx::decompile_language::glsl;

			//if (from == "hlsl")
			//	return rsx::decompile_language::hlsl;

			throw;
		}
	};

	template<>
	struct to_impl_t<std::string, rsx::decompile_language>
	{
		static std::string func(rsx::decompile_language from)
		{
			switch (from)
			{
			case rsx::decompile_language::glsl:
				return "glsl";
				//case rsx::decompile_language::hlsl:
				//	return "hlsl";
			}

			throw;
		}
	};
}

namespace rsx
{
	template<typename Type, typename KeyType = u64, typename Hasher = std::hash<KeyType>>
	struct cache
	{
	private:
		std::unordered_map<KeyType, Type, Hasher> m_entries;

	public:
		const Type* find(u64 key) const
		{
			auto found = m_entries.find(key);

			if (found == m_entries.end())
				return nullptr;

			return &found->second;
		}

		const Type& insert(KeyType key, const Type &entry)
		{
			return m_entries.insert({ key, entry }).first->second;
		}
	};

	struct shaders_cache_entry
	{
		cache<decompiled_shader> decompiled;
		cache<complete_shader> complete;
	};

	struct shaders_cache
	{
		shaders_cache_entry fragment_shaders;
		shaders_cache_entry vertex_shaders;

		void load(const std::string &path, decompile_language lang);
		void load(decompile_language lang);

		static std::string path_to_root();
	};

	template<typename UcodeType>
	std::pair<const rsx::complete_shader&, const rsx::decompiled_shader&> find_shader(rsx::decompile_language lang, u64 hash,
		rsx::shaders_cache_entry &cache, u32 offset, UcodeType *ucode, rsx::decompiled_shader(*decompile)(std::size_t, UcodeType*, rsx::decompile_language),
		u32 ctrl, u32 attrib_input, u32 attrib_output)
	{
		const rsx::decompiled_shader *decompiled = cache.decompiled.find(hash);

		if (!decompiled)
		{
			decompiled = &cache.decompiled.insert(hash, decompile(offset, ucode, lang));

			fs::file(fmt::format("data/cache/%016llx.", hash) + (decompiled->type == rsx::program_type::vertex ? "vp" : "fp")
				+ ".ucode", fom::rewrite).write(ucode + offset, decompiled->ucode_size);
		}

		const rsx::complete_shader *complete = cache.complete.find(hash);

		if (!complete)
		{
			const std::string path = fmt::format("data/cache/%016llx.", hash)
				+ (decompiled->type == rsx::program_type::vertex ? "vp" : "fp") + "." + convert::to<std::string>(lang);

			if (false && fs::is_file(path))
			{
				rsx::complete_shader new_complete = finalize_shader(*decompiled, ctrl, attrib_input, attrib_output);
				new_complete.code = (std::string)fs::file(path);
				complete = &cache.complete.insert(hash, new_complete);
			}
			else
			{
				complete = &cache.complete.insert(hash, finalize_shader(*decompiled, ctrl, attrib_input, attrib_output));

				fs::file(path, fom::rewrite) << complete->code;
			}
		}

		return{ *complete, *decompiled };
	}
}