#include "stdafx.h"
#include "rsx_cache.h"
#include "Emu/System.h"

namespace rsx
{
	shader_info shaders_cache::get(const program_cache_context &ctxt, raw_shader &raw_shader, const program_state& state)
	{
		auto found_entry = m_entries.find(raw_shader);

		shader_info info;
		entry_t *entry;

		static const std::string &path = fs::get_executable_dir() + "data/cache/";

		if (found_entry != m_entries.end())
		{
			entry = &found_entry->second;
		}
		else
		{
			//analyze_raw_shader(raw_shader);

			fs::file{ path + fmt::format("%016llx.", raw_shader.hash()) + (raw_shader.type == rsx::program_type::fragment ? "fp" : "vp") + ".ucode", fs::rewrite }
			.write(raw_shader.ucode.data(), raw_shader.ucode.size());

			rsx::decompiled_shader decompiled_shader = decompile(raw_shader, ctxt.lang);
			auto inserted = m_entries.insert({ raw_shader, entry_t{ decompiled_shader } }).first;
			inserted->second.decompiled.raw = &inserted->first;
			entry = &inserted->second;
		}

		info.decompiled = &entry->decompiled;

		auto found_complete = entry->complete.find(state);

		if (found_complete != entry->complete.end())
		{
			info.complete = &found_complete->second;
		}
		else
		{
			rsx::complete_shader complete_shader = ctxt.complete_shader(entry->decompiled, state);
			complete_shader.decompiled = info.decompiled;
			info.complete = &entry->complete.insert({ state, complete_shader }).first->second;
			info.complete->user_data = nullptr;

			fs::file{ path + fmt::format("%016llx.", raw_shader.hash()) + (raw_shader.type == rsx::program_type::fragment ? "fp" : "vp") + "." + (ctxt.lang == rsx::decompile_language::glsl ? "glsl" : "hlsl"), fs::rewrite }.write(info.complete->code);
		}

		if (info.complete->user_data == nullptr)
		{
			info.complete->user_data = ctxt.compile_shader(raw_shader.type, info.complete->code);
		}

		return info;
	}

	void shaders_cache::clear(const program_cache_context& context)
	{
		for (auto &entry : m_entries)
		{
			for (auto &shader : entry.second.complete)
			{
				context.remove_shader(shader.second.user_data);
			}
		}

		m_entries.clear();
	}

	programs_cache::~programs_cache()
	{
		clear();
	}

	program_info programs_cache::get(raw_program raw_program_, decompile_language lang)
	{
		raw_program_.vertex_shader.type = program_type::vertex;
		raw_program_.fragment_shader.type = program_type::fragment;

		analyze_raw_shader(raw_program_.vertex_shader);
		analyze_raw_shader(raw_program_.fragment_shader);

		auto found = m_program_cache.find(raw_program_);

		if (found != m_program_cache.end())
		{
			return found->second;
		}

		program_info result;

		result.vertex_shader = m_vertex_shaders_cache.get(context, raw_program_.vertex_shader, raw_program_.state);
		result.fragment_shader = m_fragment_shader_cache.get(context, raw_program_.fragment_shader, raw_program_.state);
		result.program = context.make_program(result.vertex_shader.complete->user_data, result.fragment_shader.complete->user_data);
		m_program_cache.insert({ raw_program_, result });

		return result;
	}

	void programs_cache::clear()
	{
		for (auto &entry : m_program_cache)
		{
			context.remove_program(entry.second.program);
		}

		m_program_cache.clear();

		m_vertex_shaders_cache.clear(context);
		m_fragment_shader_cache.clear(context);
	}
}
