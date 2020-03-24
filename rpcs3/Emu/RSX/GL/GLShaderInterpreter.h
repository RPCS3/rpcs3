#pragma once
#include "GLHelpers.h"

namespace gl
{
	namespace interpreter
	{
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

				if (allocated.size() == num_used)
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

			void create(::gl::glsl::shader::type domain);
			void allocate(int size);
		};
	}

	class shader_interpreter
	{
		glsl::shader vs;
		glsl::shader fs;
		glsl::program program_handle;
		interpreter::texture_pool_allocator texture_pools[2];

		void build_vs();
		void build_fs();

	public:
		void create();
		void destroy();

		void update_fragment_textures(const std::array<std::unique_ptr<rsx::sampled_image_descriptor_base>, 16>& descriptors, u16 reference_mask, u32* out);

		glsl::program* get();
	};
}
