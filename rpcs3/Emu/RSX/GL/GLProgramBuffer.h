#pragma once
#include "GLVertexProgram.h"
#include "GLFragmentProgram.h"
#include "../Common/ProgramStateCache.h"

struct GLTraits
{
	using vertex_program_type = GLVertexProgram;
	using fragment_program_type = GLFragmentProgram;
	using pipeline_storage_type = gl::glsl::program;
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
	pipeline_storage_type build_pipeline(const vertex_program_type &vertexProgramData, const fragment_program_type &fragmentProgramData, const pipeline_properties&)
	{
		pipeline_storage_type result;
		__glcheck result.create()
			.attach(gl::glsl::shader_view(vertexProgramData.id))
			.attach(gl::glsl::shader_view(fragmentProgramData.id))
			.bind_fragment_data_location("ocol0", 0)
			.bind_fragment_data_location("ocol1", 1)
			.bind_fragment_data_location("ocol2", 2)
			.bind_fragment_data_location("ocol3", 3)
			.make();
		__glcheck result.use();

		//Progam locations are guaranteed to not change after linking
		//Texture locations are simply bound to the TIUs so this can be done once
		for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
		{
			int location;
			if (result.uniforms.has_location("tex" + std::to_string(i), &location))
				result.uniforms[location] = i;
		}

		for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
		{
			int location;
			if (result.uniforms.has_location("vtex" + std::to_string(i), &location))
				result.uniforms[location] = (i + rsx::limits::fragment_textures_count);
		}

		const int stream_buffer_start = rsx::limits::fragment_textures_count + rsx::limits::vertex_textures_count;

		//Bind locations 0 and 1 to the stream buffers
		result.uniforms[0] = stream_buffer_start;
		result.uniforms[1] = stream_buffer_start + 1;

		LOG_NOTICE(RSX, "*** prog id = %d", result.id());
		LOG_NOTICE(RSX, "*** vp id = %d", vertexProgramData.id);
		LOG_NOTICE(RSX, "*** fp id = %d", fragmentProgramData.id);

		LOG_NOTICE(RSX, "*** vp shader = \n%s", vertexProgramData.shader.c_str());
		LOG_NOTICE(RSX, "*** fp shader = \n%s", fragmentProgramData.shader.c_str());

		return result;
	}
};

class GLProgramBuffer : public program_state_cache<GLTraits>
{
public:

	u64 get_hash(void*&)
	{
		return 0;
	}

	u64 get_hash(RSXVertexProgram &prog)
	{
		return program_hash_util::vertex_program_utils::get_vertex_program_ucode_hash(prog);
	}

	u64 get_hash(RSXFragmentProgram &prog)
	{
		return program_hash_util::fragment_program_utils::get_fragment_program_ucode_hash(prog);
	}

	template <typename... Args>
	void add_pipeline_entry(RSXVertexProgram &vp, RSXFragmentProgram &fp, void* &props, Args&& ...args)
	{
		vp.skip_vertex_input_check = true;
		getGraphicPipelineState(vp, fp, props, std::forward<Args>(args)...);
	}

	bool check_cache_missed() const
	{
		return m_cache_miss_flag;
	}
};
