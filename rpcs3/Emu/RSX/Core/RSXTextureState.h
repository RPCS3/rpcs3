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
		std::array<bool, 16> fragment_textures_dirty;
		std::array<bool, 4> vertex_textures_dirty;

		std::array<std::unique_ptr<sampled_image_descriptor_base>, limits::fragment_textures_count> fs_sampler_state = {};
		std::array<std::unique_ptr<sampled_image_descriptor_base>, limits::vertex_textures_count> vs_sampler_state = {};
	};
}
