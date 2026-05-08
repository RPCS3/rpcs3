#pragma once
#include "glutils/program.h"
#include "../Program/ProgramStateCache.h"
#include "../Program/ShaderInterpreter.h"
#include "../Common/TextureUtils.h"

#include <unordered_map>

namespace rsx
{
	struct shader_loading_dialog;
}

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

		struct bindless_textures_t
		{
			std::array<handle64_t, 16> sampler1D;
			std::array<handle64_t, 16> sampler2D;
			std::array<handle64_t, 16> sampler3D;
			std::array<handle64_t, 16> samplerCUBE;

			u8 dirty = 0xff;

			std::span<handle64_t> get(rsx::texture_dimension_extended type)
			{
				using enum rsx::texture_dimension_extended;
				switch (type)
				{
				default:
				case texture_dimension_2d:
					return sampler2D;
				case texture_dimension_cubemap:
					return samplerCUBE;
				case texture_dimension_1d:
					return sampler1D;
				case texture_dimension_3d:
					return sampler3D;
				}
			}
		};

		struct cached_program
		{
			u32 flags = program_common::interpreter::CACHED_PIPE_UNINITIALIZED;

			// Compiler options mask - May not always match the storage compiler options in case of compatible pipelines
			// However the storage mask must be a subset of this options mask
			u64 build_compiler_options = 0;

			std::shared_ptr<glsl::shader> vertex_shader;
			std::shared_ptr<glsl::shader> fragment_shader;
			std::shared_ptr<glsl::program> prog;
		};
	}

	class shader_interpreter
	{
		using shader_cache_t = std::unordered_map<u64, std::shared_ptr<glsl::shader>>;
		using pipeline_cache_t = std::unordered_map<u64, std::shared_ptr<interpreter::cached_program>>;
		using async_build_callback_t = std::function<void(const std::shared_ptr<interpreter::cached_program>&)>;

		shared_mutex m_vs_cache_lock;
		shared_mutex m_fs_cache_lock;
		shared_mutex m_program_cache_lock;

		shader_cache_t m_vs_cache;
		shader_cache_t m_fs_cache;
		pipeline_cache_t m_program_cache;

		// Texture binding information.
		interpreter::bindless_textures_t m_texture_bindings{};

		void build_vs(u64 compiler_options, interpreter::cached_program& prog_data);
		void build_fs(u64 compiler_options, interpreter::cached_program& prog_data);

		std::shared_ptr<interpreter::cached_program> build_program(u64 compiler_options);
		void build_program_async(u64 compiler_options, async_build_callback_t callback);
		void init_program(const std::shared_ptr<interpreter::cached_program>& data, u64 compiler_options);
		void store_program(const std::shared_ptr<interpreter::cached_program>& data, u64 compiler_options);

		std::shared_ptr<interpreter::cached_program> m_current_interpreter;

	public:
		shader_interpreter()
		{
			std::memset(&m_texture_bindings, 0, sizeof(m_texture_bindings));
		}

		void create(rsx::shader_loading_dialog* dlg);
		void destroy();

		// Update texture bindings based on the incoming descriptor structures
		void bind_fragment_texture(int i, handle64_t handle, const rsx::sampled_image_descriptor_base& descriptor);
		void flush_texture_bindings(glsl::program* program = nullptr);

		glsl::program* get(const interpreter::program_metadata& fp_metadata, u32 vp_ctrl, u32 fp_ctrl);
		bool is_interpreter(const glsl::program* program) const;
	};
}
