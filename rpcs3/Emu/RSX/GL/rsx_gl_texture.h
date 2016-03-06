#pragma once

#include "OpenGL.h"
#include <Utilities/types.h>

namespace gl
{
	class texture_cache;
	struct texture_format;

	const texture_format& get_texture_format(u32 texture_id);
}

namespace rsx
{
	class texture;


	namespace gl_texture
	{
		void bind(gl::texture_cache& cache, int index, rsx::texture& tex);
	}
}

