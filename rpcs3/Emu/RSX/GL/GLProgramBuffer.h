#pragma once
#include "GLVertexProgram.h"
#include "GLFragmentProgram.h"
#include "GLHelpers.h"
#include "GLPipelineCompiler.h"
#include "../Common/ProgramStateCache.h"
#include "../rsx_utils.h"

struct GLTraits
{
	using vertex_program_type = GLVertexProgram;
	using fragment_program_type = GLFragmentProgram;
	using pipeline_type = gl::glsl::program;
	using pipeline_storage_type = std::unique_ptr<gl::glsl::program>;
	using pipeline_properties = void*;

	static
	void recompile_fragment_program(const RSXFragmentProgram &RSXFP, fragment_program_type& fragmentProgramData, size_t /*ID*/)
	{
		fragmentProgramData.Decompile(RSXFP);
		fragmentProgramData.Compile();
	}

	static
	void recompile_vertex_program(const RSXVertexProgram &RSXVP, vertex_program_type& vertexProgramData, size_t /*ID*/)
	{
		vertexProgramData.Decompile(RSXVP);
		vertexProgramData.Compile();
	}

	static
	void validate_pipeline_properties(const vertex_program_type&, const fragment_program_type&, pipeline_properties&)
	{
	}

	static
	pipeline_type* build_pipeline(
		const vertex_program_type &vertexProgramData,
		const fragment_program_type &fragmentProgramData,
		const pipeline_properties&,
		bool compile_async,
		std::function<pipeline_type*(pipeline_storage_type&)> callback)
	{
		auto compiler = gl::get_pipe_compiler();
		auto flags = (compile_async) ? gl::pipe_compiler::COMPILE_DEFERRED : gl::pipe_compiler::COMPILE_INLINE;

		gl::fence vp_fence, fp_fence;
		if (compile_async)
		{
			vp_fence = vertexProgramData.shader.get_compile_fence_sync();
			fp_fence = fragmentProgramData.shader.get_compile_fence_sync();
		}

		auto post_create_func = [vp_id = vertexProgramData.id, fp_id = fragmentProgramData.id, vp_fence, fp_fence]
		(std::unique_ptr<gl::glsl::program>& program)
		{
			if (!vp_fence.is_empty())
			{
				// Force server threads to wait for the compilation to finish
				vp_fence.server_wait_sync();
				fp_fence.server_wait_sync();
			}

			program->attach(gl::glsl::shader_view(vp_id))
				.attach(gl::glsl::shader_view(fp_id))
				.bind_fragment_data_location("ocol0", 0)
				.bind_fragment_data_location("ocol1", 1)
				.bind_fragment_data_location("ocol2", 2)
				.bind_fragment_data_location("ocol3", 3);

			if (g_cfg.video.log_programs)
			{
				rsx_log.notice("*** prog id = %d", program->id());
				rsx_log.notice("*** vp id = %d", vp_id);
				rsx_log.notice("*** fp id = %d", fp_id);
			}
		};

		auto post_link_func = [](std::unique_ptr<gl::glsl::program>& program)
		{
			// Program locations are guaranteed to not change after linking
			// Texture locations are simply bound to the TIUs so this can be done once
			for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
			{
				int location;
				if (program->uniforms.has_location(rsx::constants::fragment_texture_names[i], &location))
				{
					// Assign location to TIU
					program->uniforms[location] = GL_FRAGMENT_TEXTURES_START + i;

					// Check for stencil mirror
					const std::string mirror_name = std::string(rsx::constants::fragment_texture_names[i]) + "_stencil";
					if (program->uniforms.has_location(mirror_name, &location))
					{
						// Assign mirror to TIU
						program->uniforms[location] = GL_STENCIL_MIRRORS_START + i;
					}
				}
			}

			for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
			{
				int location;
				if (program->uniforms.has_location(rsx::constants::vertex_texture_names[i], &location))
					program->uniforms[location] = GL_VERTEX_TEXTURES_START + i;
			}

			// Bind locations 0 and 1 to the stream buffers
			program->uniforms[0] = GL_STREAM_BUFFER_START + 0;
			program->uniforms[1] = GL_STREAM_BUFFER_START + 1;
		};

		auto pipeline = compiler->compile(vertexProgramData.id, fragmentProgramData.id,
			flags, post_create_func, post_link_func, callback);

		return callback(pipeline);
	}
};

struct GLProgramBuffer : public program_state_cache<GLTraits>
{
	GLProgramBuffer() = default;

	void initialize(decompiler_callback_t callback)
	{
		notify_pipeline_compiled = callback;
	}

	u64 get_hash(void* const&)
	{
		return 0;
	}

	u64 get_hash(const RSXVertexProgram &prog)
	{
		return program_hash_util::vertex_program_utils::get_vertex_program_ucode_hash(prog);
	}

	u64 get_hash(const RSXFragmentProgram &prog)
	{
		return program_hash_util::fragment_program_utils::get_fragment_program_ucode_hash(prog);
	}

	template <typename... Args>
	void add_pipeline_entry(RSXVertexProgram &vp, RSXFragmentProgram &fp, void* &props, Args&& ...args)
	{
		vp.skip_vertex_input_check = true;
		get_graphics_pipeline(vp, fp, props, false, false, std::forward<Args>(args)...);
	}

    void preload_programs(RSXVertexProgram &vp, RSXFragmentProgram &fp)
    {
		search_vertex_program(vp);
		search_fragment_program(fp);
    }

	bool check_cache_missed() const
	{
		return m_cache_miss_flag;
	}
};
