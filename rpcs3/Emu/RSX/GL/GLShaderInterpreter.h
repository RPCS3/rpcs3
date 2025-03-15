#pragma once
#include "glutils/program.h"
#include "../Program/ProgramStateCache.h"
#include "../Common/TextureUtils.h"

#include <unordered_map>

namespace gl
{
	using namespace ::glsl;

	namespace interpreter
	{
		using program_metadata = program_hash_util::fragment_program_utils::fragment_program_metadata;

		enum class texture_pool_flags
		{
			dirty = 1
		};

		struct texture_pool
		{
			int pool_size = 0;
			int num_used = 0;
			u32 flags = 0;
			std::vector<int> allocated;

			bool allocate(int value)
			{
				if (num_used >= pool_size)
				{
					return false;
				}

				if (allocated.size() == unsigned(num_used))
				{
					allocated.push_back(value);
				}
				else
				{
					allocated[num_used] = value;
				}

				num_used++;
				flags |= static_cast<u32>(texture_pool_flags::dirty);
				return true;
			}
		};

		struct texture_pool_allocator
		{
			int max_image_units = 0;
			int used = 0;
			std::vector<texture_pool> pools;

			void create(::glsl::program_domain domain);
			void allocate(int size);
		};

		struct cached_program
		{
			glsl::shader vertex_shader;
			glsl::shader fragment_shader;
			glsl::program prog;
			texture_pool_allocator allocator;
		};
	}

	class shader_interpreter
	{
		using shader_cache_t = std::unordered_map<u64, std::unique_ptr<interpreter::cached_program>>;
		shader_cache_t m_program_cache;

		void build_vs(u64 compiler_options, interpreter::cached_program& prog_data);
		void build_fs(u64 compiler_options, interpreter::cached_program& prog_data);
		interpreter::cached_program* build_program(u64 compiler_options);

		interpreter::cached_program* m_current_interpreter = nullptr;

	public:
		void create();
		void destroy();

		void update_fragment_textures(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, 16>& descriptors, u16 reference_mask, u32* out);

		glsl::program* get(const interpreter::program_metadata& fp_metadata, u32 vp_ctrl, u32 fp_ctrl);
		bool is_interpreter(const glsl::program* program) const;
	};
}
