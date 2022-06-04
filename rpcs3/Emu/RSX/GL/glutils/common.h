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
	// TODO: Move to sync.h
	class fence
	{
		GLsync m_value = nullptr;
		mutable GLenum flags = GL_SYNC_FLUSH_COMMANDS_BIT;
		mutable bool signaled = false;

	public:

		fence() = default;
		~fence() = default;

		void create()
		{
			m_value = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			flags = GL_SYNC_FLUSH_COMMANDS_BIT;
		}

		void destroy()
		{
			glDeleteSync(m_value);
			m_value = nullptr;
		}

		void reset()
		{
			if (m_value != nullptr)
				destroy();

			create();
		}

		bool is_empty() const
		{
			return (m_value == nullptr);
		}

		bool check_signaled() const
		{
			ensure(m_value);

			if (signaled)
				return true;

			if (flags)
			{
				GLenum err = glClientWaitSync(m_value, flags, 0);
				flags = 0;

				if (!(err == GL_ALREADY_SIGNALED || err == GL_CONDITION_SATISFIED))
					return false;
			}
			else
			{
				GLint status = GL_UNSIGNALED;
				GLint tmp;

				glGetSynciv(m_value, GL_SYNC_STATUS, 4, &tmp, &status);

				if (status != GL_SIGNALED)
					return false;
			}

			signaled = true;
			return true;
		}

		bool wait_for_signal()
		{
			ensure(m_value);

			if (signaled == GL_FALSE)
			{
				GLenum err = GL_WAIT_FAILED;
				bool done = false;

				while (!done)
				{
					if (flags)
					{
						err = glClientWaitSync(m_value, flags, 0);
						flags = 0;

						switch (err)
						{
						default:
							rsx_log.error("gl::fence sync returned unknown error 0x%X", err);
							[[fallthrough]];
						case GL_ALREADY_SIGNALED:
						case GL_CONDITION_SATISFIED:
							done = true;
							break;
						case GL_TIMEOUT_EXPIRED:
							continue;
						}
					}
					else
					{
						GLint status = GL_UNSIGNALED;
						GLint tmp;

						glGetSynciv(m_value, GL_SYNC_STATUS, 4, &tmp, &status);

						if (status == GL_SIGNALED)
							break;
					}
				}

				signaled = (err == GL_ALREADY_SIGNALED || err == GL_CONDITION_SATISFIED);
			}

			glDeleteSync(m_value);
			m_value = nullptr;

			return signaled;
		}

		void server_wait_sync() const
		{
			ensure(m_value != nullptr);
			glWaitSync(m_value, 0, GL_TIMEOUT_IGNORED);
		}
	};
}
