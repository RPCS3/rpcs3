#pragma once

// PS5: the menu a game pauses into (L1 + R1 + the touch pad's click, the PS
// button's stand-in), in place of RPCS3's home menu, drawn as the launcher
// draws: a card of deep blue glass with the game's icon and name, its tabs
// along the top (L1 / R1), and a tab's rows in cyan-lit pills.
//
// Game: resume, the settings reached for while playing, restart, back to the
// library. Performance: the game's frames a second, the busiest threads and
// more, as measured over the five seconds before the menu opened (and now,
// under it), to say what was happening when something looked wrong. Controls:
// which of the controller's buttons presses each of the PS3's, in this game
// (ps5_pad_remap.h). The other tabs: every setting the desktop dialog offers, by its
// tabs, as the launcher's Game settings has them. A change is the game's own
// (its custom config, written as the menu closes), and a setting the
// emulator reads as it runs takes effect at once; the others from the
// game's next start. The emulation runs on under the menu, and the game
// pauses itself, as a PS3 game does under the PS3's own menu.

#include "Emu/RSX/Overlays/overlays.h"
#include "Emu/Cell/ErrorCodes.h"
#include "ps5_ui.h"
#include "ps5_perf.h"
#include "ps5_pad_remap.h"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace rsx::overlays
{
	struct ps5_pause_menu : public user_interface
	{
		ps5_pause_menu();

		void update(u64 timestamp_us) override;
		void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;
		compiled_resource get_compiled() override;

		error_code show(std::function<void(s32 status)> on_close);

	private:
		enum class action : u8
		{
			none,
			resume,
			restart,
			quit,
			capture,
			screenshot,    // saves, and the menu stays
			reset_controls // the controller's own, and the menu stays
		};

		// A row of a tab: an action, or a setting (an index into m_settings), or
		// a measurement (perf_metric)
		struct row
		{
			action act = action::none;
			s32 setting = -1;
			std::string label;
			std::string note;
			const char* icon = nullptr;
			bool rule_before = false;
			s32 perf = -1;
			s32 remap = -1; // one of the PS3's buttons, on the Controls tab
		};

		// The Performance tab's measurements
		enum perf_metric : s32
		{
			perf_game_fps,
			perf_shown_fps,
			perf_made,
			perf_gpu_wait,
			perf_ppu,
			perf_spu,
			perf_busiest, // and the next ones, a thread each
			perf_collision = perf_busiest + 8,
			perf_memory,
		};
		std::string perf_value(const ps5_perf& perf, s32 metric) const;
		std::string perf_note(s32 metric) const;

		struct tab
		{
			std::string name;
			std::vector<row> rows;
		};

		void move_to(s32 next);
		void change(s32 direction);
		void apply_live(const ps5ui::setting_row& setting);
		void begin_close(action then);
		void finish_close();

		std::mutex m_mutex;
		std::string m_serial;
		std::string m_title;
		std::unique_ptr<image_info> m_icon;
		std::vector<ps5ui::setting_row> m_settings; // with a heading for each of the desktop dialog's tabs
		std::vector<tab> m_tabs;
		s32 m_tab = 0;
		s32 m_row = 0;
		s32 m_scroll = 0;
		bool m_changed = false;

		ps5_perf m_perf_before; // the five seconds before the menu opened
		ps5_pad_remap::map m_remap = ps5_pad_remap::identity(); // this game's controls
		bool m_remap_changed = false;

		std::vector<u8> m_glow_pixels;
		std::unique_ptr<memory_image_info> m_glow;
		std::map<std::string, std::unique_ptr<image_info>> m_icons;

		// Opening and closing: the card fades and rises in, and back
		u64 m_now_us = 0;
		u64 m_open_us = 0;
		u64 m_close_us = 0;
		bool m_closing = false;
		bool m_closed = false;
		action m_then = action::resume;
	};

	// Opens the pause menu over the running game (pad_thread::open_home_menu);
	// on_close runs once it has closed
	error_code open_ps5_pause_menu(std::function<void(s32 status)> on_close);
}
