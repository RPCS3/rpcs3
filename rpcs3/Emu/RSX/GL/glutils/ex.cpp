#include "stdafx.h"
#include "ex.h"

namespace gl::ex
{
	void glNamedBufferStorageEX(GLuint buffer, GLenum target, GLsizeiptr size, const void* data, GLbitfield flags)
	{
		GLuint restore = GL_NONE;
		glGetIntegerv(target, utils::bless<GLint>(&restore));

		glBindBuffer(target, buffer);
		glBufferStorage(target, size, data, flags);

		if (restore != buffer)
		{
			glBindBuffer(target, restore);
		}
	}
}
