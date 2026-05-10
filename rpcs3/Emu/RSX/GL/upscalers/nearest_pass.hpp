#pragma once

#include "static_pass.hpp"

namespace gl
{
	using nearest_upscale_pass = static_upscale_pass<gl::filter::nearest>;
}
