#pragma once
#include <rsx_decompiler.h>

namespace rsx
{
	struct shader_info
	{
		decompiled_shader *decompiled;
		complete_shader *complete;
	};

	struct program_info
	{
		shader_info vertex_shader;
		shader_info fragment_shader;

		void *program;
	};

	struct program_cache_context
	{
		decompile_language lang;

		void*(*compile_shader)(program_type type, const std::string &code);
		rsx::complete_shader(*complete_shader)(const decompiled_shader &shader, program_state state);
		void*(*make_program)(const void *vertex_shader, const void *fragment_shader);
		void(*remove_program)(void *ptr);
		void(*remove_shader)(void *ptr);
	};

	class shaders_cache
	{
		struct entry_t
		{
			std::int64_t index;
			decompiled_shader decompiled;
			std::unordered_map<program_state, complete_shader, hasher> complete;
		};

		std::unordered_map<raw_shader, entry_t, hasher> m_entries;
		std::string m_path;
		std::int64_t m_index = -1;

	public:
		void path(const std::string &path_);

		shader_info get(const program_cache_context &ctxt, raw_shader &raw_shader, const program_state& state);
		void clear(const program_cache_context& context);
	};

	class programs_cache
	{
		std::unordered_map<raw_program, program_info, hasher> m_program_cache;

		shaders_cache m_vertex_shaders_cache;
		shaders_cache m_fragment_shader_cache;

	public:
		program_cache_context context;

		programs_cache();
		~programs_cache();

		program_info get(raw_program raw_program_, decompile_language lang);
		void clear();
	};
}
