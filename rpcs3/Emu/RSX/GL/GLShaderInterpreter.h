#pragma once
#include "GLGSRender.h"

namespace gl
{
	class shader_interpreter : glsl::program
	{
		glsl::shader vs;
		glsl::shader fs;

	public:
		void create();
		void destroy();
	};
}
