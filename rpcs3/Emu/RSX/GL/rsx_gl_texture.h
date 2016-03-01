#pragma once

#include "OpenGL.h"
#include <Utilities/types.h>

namespace gl
{
	class texture_cache;
}

namespace rsx
{
	class texture;

	namespace gl_texture
	{
		void bind(gl::texture_cache& cache, int index, rsx::texture& tex);
	}
}

