#pragma once

#include <util/types.hpp>
#include <array>
#include <memory>
#include "Emu/RSX/Common/TextureUtils.h"
#include "Emu/RSX/Utils/rsx_utils.h"

namespace rsx
{
	// Per-context texture state: dirty tracking for the texture units and the sampler descriptors resolved for them
	struct texture_state
	{
		// One bit per texture unit whose registers changed. The core clears the bits of the units the backend has resolved.
		rsx::flags32_t fragment_textures_dirty = 0;
		rsx::flags32_t vertex_textures_dirty = 0;

		std::array<std::unique_ptr<sampled_image_descriptor_base>, limits::fragment_textures_count> fs_sampler_state = {};
		std::array<std::unique_ptr<sampled_image_descriptor_base>, limits::vertex_textures_count> vs_sampler_state = {};
	};
}
