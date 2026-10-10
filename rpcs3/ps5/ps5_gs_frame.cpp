#include "stdafx.h"
#include "ps5_gs_frame.h"
#include "ps5_screenshots.h"

LOG_CHANNEL(ps5_log, "PS5");

ps5_gs_frame::ps5_gs_frame(int width, int height, f64 rate)
	: m_width(width), m_height(height), m_rate(rate)
{
}

void ps5_gs_frame::close() { m_shown = false; }
void ps5_gs_frame::reset() {}
bool ps5_gs_frame::shown() { return m_shown; }
void ps5_gs_frame::hide() { m_shown = false; }
void ps5_gs_frame::show() { m_shown = true; }
void ps5_gs_frame::toggle_fullscreen() {} // always the whole display

// Contexts are OpenGL's; the console draws with Vulkan only
void ps5_gs_frame::delete_context(draw_context_t) {}
draw_context_t ps5_gs_frame::make_context() { return nullptr; }
void ps5_gs_frame::set_current(draw_context_t) {}
void ps5_gs_frame::flip(draw_context_t, bool) {}

int ps5_gs_frame::client_width() { return m_width; }
int ps5_gs_frame::client_height() { return m_height; }
f64 ps5_gs_frame::client_display_rate() { return m_rate; }
bool ps5_gs_frame::has_alpha() { return false; }

display_handle_t ps5_gs_frame::handle() const
{
	return {}; // std::monostate: the surface is the display itself
}

// Software presentation (the null renderer's frames) is not shown yet
bool ps5_gs_frame::can_consume_frame() const { return false; }
void ps5_gs_frame::present_frame(std::vector<u8>&&, u32, u32, u32, bool) const {}

// The frame the pause menu asked for as it opened, held for its Save screenshot (ps5_screenshots.h)
void ps5_gs_frame::take_screenshot(std::vector<u8>&& sshot_data, u32 sshot_width, u32 sshot_height, bool is_bgra)
{
	ps5_screenshots::take(std::move(sshot_data), sshot_width, sshot_height, is_bgra);
}

void ps5_gs_frame::update_title(double fps)
{
	// No title bar: the frame rate goes to the log now and then
	static u32 calls = 0;
	if (fps > 0.0 && ++calls % 10 == 0)
	{
		ps5_log.notice("%.1f fps", fps);
	}
}
