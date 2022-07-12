#pragma once

#include "capabilities.hpp"

#define GL_FRAGMENT_TEXTURES_START 0
#define GL_VERTEX_TEXTURES_START   (GL_FRAGMENT_TEXTURES_START + 16)
#define GL_STENCIL_MIRRORS_START   (GL_VERTEX_TEXTURES_START + 4)
#define GL_STREAM_BUFFER_START     (GL_STENCIL_MIRRORS_START + 16)
#define GL_TEMP_IMAGE_SLOT         31

#define UBO_SLOT(x)  (x)
#define SSBO_SLOT(x) (x + 8)

#define GL_VERTEX_PARAMS_BIND_SLOT             UBO_SLOT(0)
#define GL_VERTEX_LAYOUT_BIND_SLOT             UBO_SLOT(1)
#define GL_VERTEX_CONSTANT_BUFFERS_BIND_SLOT   UBO_SLOT(2)
#define GL_FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT UBO_SLOT(3)
#define GL_FRAGMENT_STATE_BIND_SLOT            UBO_SLOT(4)
#define GL_FRAGMENT_TEXTURE_PARAMS_BIND_SLOT   UBO_SLOT(5)
#define GL_RASTERIZER_STATE_BIND_SLOT          UBO_SLOT(6)
#define GL_INTERPRETER_VERTEX_BLOCK            SSBO_SLOT(0)
#define GL_INTERPRETER_FRAGMENT_BLOCK          SSBO_SLOT(1)
#define GL_COMPUTE_BUFFER_SLOT(index)          SSBO_SLOT(2 + index)
#define GL_COMPUTE_IMAGE_SLOT(index)           UBO_SLOT(index)

//Function call wrapped in ARB_DSA vs EXT_DSA compat check
#define DSA_CALL(func, object_name, target, ...)\
	if (::gl::get_driver_caps().ARB_dsa_supported)\
		gl##func(object_name, __VA_ARGS__);\
	else\
		gl##func##EXT(object_name, target, __VA_ARGS__);

#define DSA_CALL2(func, ...)\
	if (::gl::get_driver_caps().ARB_dsa_supported)\
		gl##func(__VA_ARGS__);\
	else\
		gl##func##EXT(__VA_ARGS__);

#define DSA_CALL2_RET(func, ...)\
	(::gl::get_driver_caps().ARB_dsa_supported) ?\
		gl##func(__VA_ARGS__) :\
		gl##func##EXT(__VA_ARGS__)

#define DSA_CALL3(funcARB, funcDSA, ...)\
	if (::gl::get_driver_caps().ARB_dsa_supported)\
		gl##funcARB(__VA_ARGS__);\
	else\
		gl##funcDSA##EXT(__VA_ARGS__);

namespace gl
{
	template<typename Type, uint BindId, uint GetStateId>
	class save_binding_state_base
	{
		GLint m_last_binding;

	public:
		save_binding_state_base(const Type& new_state) : save_binding_state_base()
		{
			new_state.bind();
		}

		save_binding_state_base()
		{
			glGetIntegerv(GetStateId, &m_last_binding);
		}

		~save_binding_state_base()
		{
			glBindBuffer(BindId, m_last_binding);
		}
	};
}
