#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include <memory>
#include <vector>

#include "display.h"

class GSFrameBase
{
public:
	GSFrameBase() = default;
	GSFrameBase(const GSFrameBase&) = delete;
	virtual ~GSFrameBase() = default;

	virtual void close() = 0;
	virtual bool shown() = 0;
	virtual void hide() = 0;
	virtual void show() = 0;
	virtual void toggle_fullscreen() = 0;

	virtual void delete_context(draw_context_t ctx) = 0;
	virtual draw_context_t make_context() = 0;
	virtual void set_current(draw_context_t ctx) = 0;
	virtual void flip(draw_context_t ctx, bool skip_frame = false) = 0;
	virtual int client_width() = 0;
	virtual int client_height() = 0;

	virtual display_handle_t handle() const = 0;

	atomic_t<bool> screenshot_toggle = false;
	virtual void take_screenshot(const std::vector<u8> sshot_data, const u32 sshot_width, const u32 sshot_height, bool is_bgra) = 0;
};
