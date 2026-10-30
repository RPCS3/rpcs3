#pragma once

#include <util/types.hpp>
#include "Emu/RSX/Utils/rsx_utils.h"

namespace rsx
{
	// Per-context texture state: dirty tracking for the texture units
	struct texture_state
	{
		// One bit per texture unit whose registers changed. The core clears the bits of the units the backend has resolved.
		rsx::flags32_t fragment_textures_dirty = 0;
		rsx::flags32_t vertex_textures_dirty = 0;

		// Mask of texture units whose resolved image is not a plain texture upload (e.g. a render target).
		rsx::flags32_t fragment_textures_volatile = 0;
	};
}
