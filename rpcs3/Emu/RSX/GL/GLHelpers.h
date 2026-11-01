#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>

#include "../GCM.h"
#include "../Common/TextureUtils.h"
#include "../Program/GLSLTypes.h"

#include "Utilities/mutex.h"
#include "Utilities/geometry.h"
#include "Utilities/File.h"
#include "util/logs.hpp"
#include "util/asm.hpp"

#include "glutils/common.h"
// TODO: Include on use
#include "glutils/buffer_object.h"
#include "glutils/image.h"
#include "glutils/sampler.h"
#include "glutils/pixel_settings.hpp"
#include "glutils/state_tracker.hpp"

// Noop keyword outside of Windows (used in log_debug)
#if !defined(_WIN32) && !defined(APIENTRY)
#define APIENTRY
#endif


namespace gl
{
	void enable_debugging();
	bool is_primitive_native(rsx::primitive_type in);
	bool emulate_extended_depth_range();
	GLenum draw_mode(rsx::primitive_type in);

	// Copies src into a cached texture (dst) with the same size and format as src and calls draw_overlays to draw the overlays into it.
	// Used for screenshots and recordings with overlays and for headset rendering, since RSX draws the overlays directly to the window.
	gl::texture* copy_texture_with_overlays(gl::command_context& cmd, std::unique_ptr<gl::texture>& dst, gl::texture* src, const std::function<void(gl::texture* dst)>& draw_overlays);
}
