#pragma once

#include "capabilities.h"

namespace gl::ex
{
	// Fallback EX implementation of NamedBufferStorage. The official EXT_DSA variant was dropped from desktop spec as it was originally meant for GLES only.
	// Target parameter reintroduced as an optimization to avoid side effects.
	void glNamedBufferStorageEX(GLuint buffer, GLenum target, GLsizeiptr size, const void* data, GLbitfield flags);
}

using namespace ::gl::ex;
