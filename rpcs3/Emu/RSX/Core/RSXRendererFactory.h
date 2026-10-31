#pragma once

#include <memory>
#include "Emu/system_config_types.h"

namespace utils
{
	struct serial;
}

namespace rsx
{
	struct context;
	class display_output;
	class render_backend;

	// Creates the RSX thread.
	void create_render_thread(utils::serial* ar);

	// NOTE: A renderer that is not part of this build falls back to the next one.
	// Hierarchy: OpenGL -> Vulkan -> Null. Null always exists.
	std::unique_ptr<render_backend> make_render_backend(video_renderer renderer, context* ctx, display_output* display);

	// Returns the next suitable renderer given the original choice.
	// It's needed because you don't always get what you request depending on build settings and platform.
	video_renderer resolve_render_backend_type(video_renderer renderer);
}
