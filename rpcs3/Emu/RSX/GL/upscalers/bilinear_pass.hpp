#pragma once

#include "static_pass.hpp"

namespace gl
{
	using bilinear_upscale_pass = static_upscale_pass<gl::filter::linear>;
}
