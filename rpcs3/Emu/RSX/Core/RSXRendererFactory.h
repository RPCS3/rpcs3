#pragma once

#include "Emu/system_config_types.h"

namespace utils
{
	struct serial;
}

namespace rsx
{
	// Creates the RSX thread with the given renderer. Renderers that are not part of this build are skipped.
	// If we cannot create any renderer, we fall back to the null renderer to avoid crashing.
	void create_render_thread(video_renderer renderer, utils::serial* ar);
}
