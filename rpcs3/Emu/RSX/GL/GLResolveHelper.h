#pragma once

#include "GLCompute.h"
#include "GLOverlays.h"

namespace gl
{
	void resolve_image(gl::command_context& cmd, gl::viewable_image* dst, gl::viewable_image* src);
	void unresolve_image(gl::command_context& cmd, gl::viewable_image* dst, gl::viewable_image* src);
}
