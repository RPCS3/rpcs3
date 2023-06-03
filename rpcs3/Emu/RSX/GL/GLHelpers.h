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
	GLenum draw_mode(rsx::primitive_type in);
}
