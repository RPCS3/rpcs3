#pragma once
#include "GLVertexProgram.h"
#include "GLFragmentProgram.h"
#include "GLHelpers.h"
#include "../Common/ProgramStateCache.h"
#include "../rsx_utils.h"

struct GLTraits
{
	using vertex_program_type = GLVertexProgram;
	using fragment_program_type = GLFragmentProgram;
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
	pipeline_storage_type build_pipeline(const vertex_program_type &vertexProgramData, const fragment_program_type &fragmentProgramData, const pipeline_properties&)
	{
		pipeline_storage_type result = std::make_unique<gl::glsl::program>();
		result->create()
			.attach(gl::glsl::shader_view(vertexProgramData.id))
			.attach(gl::glsl::shader_view(fragmentProgramData.id))
			.bind_fragment_data_location("ocol0", 0)
			.bind_fragment_data_location("ocol1", 1)
			.bind_fragment_data_location("ocol2", 2)
			.bind_fragment_data_location("ocol3", 3)
			.make();

		// Progam locations are guaranteed to not change after linking
		// Texture locations are simply bound to the TIUs so this can be done once
		for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
		{
			int location;
			if (result->uniforms.has_location(rsx::constants::fragment_texture_names[i], &location))
			{
				// Assign location to TIU
				result->uniforms[location] = GL_FRAGMENT_TEXTURES_START + i;

				// Check for stencil mirror
				const std::string mirror_name = std::string(rsx::constants::fragment_texture_names[i]) + "_stencil";
				if (result->uniforms.has_location(mirror_name, &location))
				{
					// Assign mirror to TIU
					result->uniforms[location] = GL_STENCIL_MIRRORS_START + i;
				}
			}
		}

		for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
		{
			int location;
			if (result->uniforms.has_location(rsx::constants::vertex_texture_names[i], &location))
				result->uniforms[location] = GL_VERTEX_TEXTURES_START + i;
		}

		// Bind locations 0 and 1 to the stream buffers
		result->uniforms[0] = GL_STREAM_BUFFER_START + 0;
		result->uniforms[1] = GL_STREAM_BUFFER_START + 1;

		rsx_log.notice("*** prog id = %d", result->id());
		rsx_log.notice("*** vp id = %d", vertexProgramData.id);
		rsx_log.notice("*** fp id = %d", fragmentProgramData.id);

		rsx_log.notice("*** vp shader = \n%s", vertexProgramData.shader.c_str());
		rsx_log.notice("*** fp shader = \n%s", fragmentProgramData.shader.c_str());

		return result;
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
