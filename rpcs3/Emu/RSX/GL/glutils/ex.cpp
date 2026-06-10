#include "stdafx.h"
#include "ex.h"

namespace gl::ex
{
	void glNamedBufferStorageEX(GLuint name, GLenum target, GLsizeiptr size, const void* data, GLbitfield flags)
	{
		GLint restore = GL_NONE;
		glGetIntegerv(target, &restore);

		glBindBuffer(target, name);
		glBufferStorage(target, size, data, flags);

		if (restore != static_cast<GLint>(name))
		{
			glBindBuffer(target, restore);
		}
	}
}
