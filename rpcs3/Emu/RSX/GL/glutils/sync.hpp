#pragma once

#include "common.h"

namespace gl
{
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
