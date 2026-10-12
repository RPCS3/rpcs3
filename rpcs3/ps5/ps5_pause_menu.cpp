// PS5: the menu a game pauses into (ps5_pause_menu.h).
//
// Laid out in RSX's virtual 1280x720 space, in the launcher's design
// (ps5_ui.h): its palette, fonts, glows and pills.

#include "stdafx.h"
#include "ps5_pause_menu.h"

#include "Emu/RSX/Overlays/overlay_manager.h"
#include "ps5_screenshots.h"
#include "Emu/System.h"
#include "Emu/savestate_utils.hpp"
#include "Emu/system_config.h"
#include "Utilities/Config.h"

#include <algorithm>
#include <cmath>
#include <ctime>

LOG_CHANNEL(pause_log, "PauseMenu");

extern atomic_t<u32> g_ps5_frame_gen_capture; // VKFrameGen.cpp: the capture's sets wanted
extern atomic_t<bool> g_big_picture_mode_active; // System.cpp: a stop goes back to the launcher
void ps5_launcher_message(std::string title, std::string body); // ps5_launcher.cpp: shown when it next opens
std::string spu_collision_status(); // SPULLVMRecompiler.cpp: the collision setting's state (empty without collision code)
namespace vk { std::string lsfg_status(); std::string lsfg_dll_path(); } // VKFrameGenLSFG.h: Lossless Scaling's frame generation

namespace rsx::overlays
{
	extern void reset_performance_overlay(); // overlay_perf_metrics.cpp

	using namespace ps5ui;

	namespace
	{
		// The card, in the middle of the screen
		constexpr s16 c_card_x = 190;
		constexpr s16 c_card_y = 52;
		constexpr u16 c_card_w = 900;
		constexpr u16 c_card_h = 616;
		constexpr s16 c_left = c_card_x + 40;
		constexpr s16 c_inner_right = c_card_x + c_card_w - 40;

		// The tabs' line, the rows under it, and the words under them
		constexpr f32 c_tabs_mid = c_card_y + 146.f;
		constexpr s16 c_rows_top = c_card_y + 186;
		constexpr f32 c_row_h = 38.f;
		constexpr s32 c_visible_rows = 8;
		constexpr s16 c_help_top = c_rows_top + static_cast<s16>(c_visible_rows * c_row_h) + 14;
		constexpr f32 c_hints_mid = c_card_y + c_card_h - 30.f;

		// The card rises in over this, and sinks away faster
		constexpr f32 c_open_seconds = 0.2f;
		constexpr f32 c_close_seconds = 0.14f;

		// What the Game tab offers of the settings: those a player reaches for mid-game
		struct quick_setting
		{
			const char* section;
			const char* key;
			const char* label;
		};
		constexpr quick_setting c_quick_settings[]
		{
			{"Video/Performance Overlay", "Enabled", "Performance overlay"},
			{"Video", "Frame limit", "Frame limit"},
			{"Video", "Vblank Rate", "Vblank rate"},
			{"Video", "Frame Generation", "Frame generation"},
			{"Video", "Frame Generation Frames", "Frames made between"},
			{"Video", "Frame Generation Quality", "Frame generation quality"},
			{"Video", "Frame Generation Step Aside", "Frame generation steps aside"},
			{"Video", "Frame Generation Method", "Frame generation method"},
			{"Audio", "Master Volume", "Volume"},
			// Only for a game with collision code it applies to (GTA IV)
			{"Core", "SPU Collision Accuracy", "Collision accuracy"},
		};

		// A tooltip's first paragraph, short enough for the lines under the rows
		std::string short_help(std::string_view help)
		{
			std::string text(help.substr(0, help.find('\n')));
			if (text.size() > 230)
			{
				text.resize(text.rfind(' ', 226));
				text += "…";
			}
			return text;
		}

		// "Mon 12 Oct, 21:50", in the console's local time
		std::string state_time_text(s64 seconds)
		{
			const std::time_t when = static_cast<std::time_t>(seconds);
			std::tm parts{};
			if (!localtime_r(&when, &parts))
			{
				gmtime_r(&when, &parts);
			}
			static constexpr const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
			static constexpr const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
			return fmt::format("%s %d %s, %02d:%02d", days[parts.tm_wday % 7], parts.tm_mday, months[parts.tm_mon % 12], parts.tm_hour, parts.tm_min);
		}

		std::string size_text(u64 bytes)
		{
			return bytes >= 1000ull << 20 ? fmt::format("%.1f GB", bytes / 1073741824.) : fmt::format("%u MB", std::max<u64>(1, (bytes + (1u << 19)) >> 20));
		}
	}

	std::vector<ps5_pause_menu::saved_state> ps5_pause_menu::read_states(const std::string& serial)
	{
		// As RPCS3 names and finds them (get_savestate_file): <serial>_<n>_<id>.SAVESTAT.zst
		std::vector<saved_state> states;
		const std::string dir = get_savestate_file(serial, "", -1);
		for (const fs::dir_entry& entry : fs::dir(dir))
		{
			if (entry.is_directory || entry.size <= 1024 || !entry.name.starts_with(serial + "_"))
			{
				continue;
			}
			if (!entry.name.ends_with(".SAVESTAT.zst") && !entry.name.ends_with(".SAVESTAT.gz") && !entry.name.ends_with(".SAVESTAT"))
			{
				continue;
			}
			states.push_back({dir + entry.name, entry.mtime, entry.size});
		}
		std::sort(states.begin(), states.end(), [](const saved_state& a, const saved_state& b)
		{
			return a.time != b.time ? a.time > b.time : a.path > b.path;
		});
		return states;
	}

	std::string ps5_pause_menu::state_note(const saved_state& state) const
	{
		return fmt::format("Goes back to this moment: the game stops where it is now and starts again from the state, which takes a little while. %s. Triangle deletes it", size_text(state.size));
	}

	ps5_pause_menu::ps5_pause_menu()
	{
		m_allow_input_on_pause = true;
		return_code = selection_code::canceled;

		m_serial = Emu.GetTitleID();
		m_title = Emu.GetTitle().empty() ? m_serial : Emu.GetTitle();

		// The game's icon: an update's (the game's folder on the hard disk), else the disc's
		for (const bool disc : {false, true})
		{
			if (!m_icon)
			{
				if (const std::string dir = Emu.GetSfoDir(disc); !dir.empty())
				{
					m_icon = load_image(dir + "/ICON0.PNG");
				}
			}
		}

		m_glow = make_glow(m_glow_pixels);

		// The game as it is now, without this menu and before it shows its own
		// pause screen: read back with its next frame, for Save screenshot
		ps5_screenshots::hold_next();

		// The controller as it is while the menu reads it; the game's controls
		// for the Controls tab
		ps5_pad_remap::hold_off(true);
		m_remap = ps5_pad_remap::load(m_serial);

		// The game's settings as it runs with them, a tab for each of the
		// desktop dialog's tabs
		m_settings = read_game_settings(m_serial, true);

		tab game;
		game.name = "Game";
		game.rows.push_back({action::resume, -1, "Resume", "Back to the game", "home/32/play-button-arrowhead.png", false});
		for (const quick_setting& quick : c_quick_settings)
		{
			if (std::string_view(quick.key) == "SPU Collision Accuracy" && spu_collision_status().empty())
			{
				continue;
			}
			for (usz i = 0; i < m_settings.size(); i++)
			{
				if (!m_settings[i].heading && m_settings[i].section == quick.section && m_settings[i].key == quick.key)
				{
					game.rows.push_back({action::none, static_cast<s32>(i), quick.label, "", nullptr, false});
					break;
				}
			}
		}
		// Save states are kept by the game's serial (get_savestate_file)
		const std::string save_note = fmt::format("Saves the whole game as it is now, to come back to this moment later. The game then starts again from it, which takes a little while. The newest %u are kept", g_cfg.savestate.max_files.get());
		if (!m_serial.empty())
		{
			game.rows.push_back({action::save_state, -1, "Save state", save_note, "home/32/floppy-disk-solid.png", true});
			game.rows.push_back({action::open_states, -1, "Load state", "The game's save states, the newest first", "home/32/floppy-disk-solid.png", false});
		}
		game.rows.push_back({action::screenshot, -1, "Save screenshot", fmt::format("The game as it was when this menu opened, without the menu, to rpcs3/screenshots/%s. The launcher's Gallery shows them", m_serial.empty() ? std::string("unknown") : m_serial), "home/32/screenshot.png", m_serial.empty()});
		game.rows.push_back({action::capture, -1, "Save frame generation's frames", "For a report: from 2 seconds after the game resumes, 4 moments half a second apart, the game's frames and those made between, to rpcs3/captures", "home/32/video-camera.png", false});
		game.rows.push_back({action::restart, -1, "Restart game", "Starts it again, with the settings changed here", "home/32/rotate-left-solid.png", false});
		game.rows.push_back({action::quit, -1, "Quit to library", "Ends the game and goes back to your games", "home/32/circle-left-solid.png", false});
		m_tabs.push_back(std::move(game));

		// States: a new one, the setting saving can need, and those kept
		if (!m_serial.empty())
		{
			m_states = read_states(m_serial);

			tab states;
			states.name = "States";
			states.rows.push_back({action::save_state, -1, "Save state now", save_note, "home/32/floppy-disk-solid.png", false});
			for (usz i = 0; i < m_settings.size(); i++)
			{
				if (!m_settings[i].heading && m_settings[i].key == "Compatible Savestate Mode")
				{
					states.rows.push_back({action::none, static_cast<s32>(i), "SPU compatible savestates", "", nullptr, false});
					break;
				}
			}
			if (m_states.empty())
			{
				states.rows.push_back({action::none, -1, "No save states yet", "Save state saves the game as it is, to come back to that moment from here.", nullptr, true});
			}
			for (usz i = 0; i < m_states.size(); i++)
			{
				row r;
				r.act = action::load_state;
				r.state = static_cast<s32>(i);
				r.label = "Saved " + state_time_text(m_states[i].time);
				r.note = state_note(m_states[i]);
				r.rule_before = i == 0;
				states.rows.push_back(std::move(r));
			}
			m_tabs.push_back(std::move(states));
		}

		// Performance: what the five seconds before the menu measured
		m_perf_before = ps5_perf_latest();
		{
			tab perf;
			perf.name = "Performance";
			const auto metric = [&](s32 which, std::string label, const char* icon = nullptr, bool rule = false)
			{
				row r;
				r.label = std::move(label);
				r.icon = icon;
				r.rule_before = rule;
				r.perf = which;
				perf.rows.push_back(std::move(r));
			};
			if (!m_perf_before.valid)
			{
				perf.rows.push_back({action::none, -1, "Still measuring", "The game's frames a second and its busiest threads are measured every 5 seconds while it runs. Open this menu again in a few seconds.", "home/32/gauge-solid.png", false});
			}
			else
			{
				const bool frame_gen = m_perf_before.made_fps > 0.f || g_cfg.video.frame_generation;
				metric(perf_game_fps, "Game's frames a second", "home/32/gauge-solid.png");
				if (frame_gen)
				{
					metric(perf_shown_fps, "Frames shown a second");
					metric(perf_made, "Made between by frame generation");
					metric(perf_gpu_wait, "Waiting for the GPU");
				}
				metric(perf_rsx, "RSX thread (the PS3's GPU, emulated)", nullptr, true);
				metric(perf_ppu, "PPU threads (the PS3's main processor)");
				metric(perf_spu, "SPU threads (the PS3's co-processors)");
				for (usz i = 0; i < m_perf_before.busiest.size() && i < 5; i++)
				{
					metric(static_cast<s32>(perf_busiest + i), m_perf_before.busiest[i].first, nullptr, i == 0);
				}
				if (!m_perf_before.collision.empty())
				{
					metric(perf_collision, "Collision accuracy", nullptr, true);
				}
				metric(perf_memory, "Memory", nullptr, m_perf_before.collision.empty());
			}
			m_tabs.push_back(std::move(perf));
		}

		// Controls: for each of the PS3's buttons, the controller's that presses it
		if (!m_serial.empty())
		{
			tab controls;
			controls.name = "Controls";
			controls.rows.push_back({action::reset_controls, -1, "Use the controller's own", "Each of the PS3's buttons pressed by the same button on the controller again", "home/32/gamepad-solid.png", false});
			for (u8 i = 0; i < ps5_pad_remap::count; i++)
			{
				row r;
				r.label = fmt::format("PS3 %s", ps5_pad_remap::ps3_name(i));
				r.remap = i;
				r.rule_before = i == 0;
				controls.rows.push_back(std::move(r));
			}
			m_tabs.push_back(std::move(controls));
		}

		for (usz i = 0; i < m_settings.size(); i++)
		{
			if (m_settings[i].heading)
			{
				m_tabs.push_back({m_settings[i].label, {}});
			}
			else if (m_tabs.size() > 1)
			{
				m_tabs.back().rows.push_back({action::none, static_cast<s32>(i), m_settings[i].label, "", nullptr, false});
			}
		}
		std::erase_if(m_tabs, [](const tab& each) { return each.rows.empty(); });
		for (usz t = 0; t < m_tabs.size(); t++)
		{
			if (m_tabs[t].name == "States")
			{
				m_states_tab = static_cast<s32>(t);
			}
		}
	}

	std::string ps5_pause_menu::perf_value(const ps5_perf& perf, s32 metric) const
	{
		if (!perf.valid)
		{
			return "-";
		}
		switch (metric)
		{
		case perf_game_fps: return fmt::format("%.1f", perf.game_fps);
		case perf_shown_fps: return fmt::format("%.1f", perf.game_fps + std::max(0.f, perf.made_fps - perf.dropped_fps));
		case perf_made: return perf.dropped_fps >= 0.05f ? fmt::format("%.1f a second, %.1f dropped", perf.made_fps, perf.dropped_fps) : fmt::format("%.1f a second", perf.made_fps);
		case perf_gpu_wait: return fmt::format("%.0f ms a second", perf.gpu_wait_ms);
		case perf_rsx: return fmt::format("%.0f%% busy", perf.rsx_load);
		case perf_ppu: return fmt::format("%.0f%% of the CPU", perf.ppu_load);
		case perf_spu: return fmt::format("%.0f%% of the CPU", perf.spu_load);
		case perf_collision: return perf.collision;
		case perf_memory: return fmt::format("%u MiB used, %u MiB free", perf.heap_mib, perf.free_mib);
		default: break;
		}
		if (metric >= perf_busiest && metric < perf_collision)
		{
			// The same thread, by its name, in these measurements
			const usz index = static_cast<usz>(metric - perf_busiest);
			if (index < m_perf_before.busiest.size())
			{
				for (const auto& [name, share] : perf.busiest)
				{
					if (name == m_perf_before.busiest[index].first)
					{
						return fmt::format("%.0f%% of the time", share);
					}
				}
				return "under 5% of the time";
			}
		}
		return "-";
	}

	std::string ps5_pause_menu::perf_note(s32 metric) const
	{
		switch (metric)
		{
		case perf_game_fps: return "The game's own frames, measured over the 5 seconds before this menu opened.";
		case perf_shown_fps: return "The game's frames and those frame generation made between them that reached the screen.";
		case perf_made: return "Frames frame generation made between the game's own; dropped ones were made but would have reached the screen too late.";
		case perf_gpu_wait: return "How long the emulator waited each second for the GPU to finish frame generation's work.";
		case perf_rsx: return "How much of the time the emulated GPU's thread was working on the game's drawing, not waiting for it. Near 100% means the game waits for it.";
		case perf_ppu: return "The PPU threads' running time, as a share of all of the console's CPU threads.";
		case perf_spu: return "The SPU threads' running time, as a share of all of the console's CPU threads.";
		case perf_collision: return "GTA IV's collision code: what the Collision accuracy setting is doing.";
		case perf_memory: return "The emulator's memory, and the console's left for it.";
		default: break;
		}
		return "How much of the time this thread was running. One near 100% is likely what holds the game back: the rest wait for it.";
	}

	void ps5_pause_menu::move_to(s32 next)
	{
		const s32 count = static_cast<s32>(m_tabs[m_tab].rows.size());
		next = std::clamp(next, 0, count - 1);
		if (next != m_row)
		{
			m_row = next;
			play_sound(sound_effect::cursor);
		}
	}

	void ps5_pause_menu::change(s32 direction)
	{
		const row& current = m_tabs[m_tab].rows[m_row];
		if (current.remap >= 0)
		{
			// The next of the controller's buttons, or nothing
			u8& source = m_remap[current.remap];
			source = static_cast<u8>((source + (direction < 0 ? ps5_pad_remap::count : 1)) % (ps5_pad_remap::count + 1));
			m_remap_changed = true;
			play_sound(sound_effect::cursor);
			return;
		}
		if (current.setting < 0)
		{
			return;
		}
		setting_row& setting = m_settings[current.setting];
		const auto it = std::find(setting.options.begin(), setting.options.end(), setting.value);
		const s32 at = static_cast<s32>(it - setting.options.begin());
		const s32 count = static_cast<s32>(setting.options.size());
		if (count < 2)
		{
			return;
		}
		setting.value = setting.options[(at + (direction < 0 ? count - 1 : 1)) % count];
		m_changed = true;
		apply_live(setting);
		play_sound(sound_effect::cursor);
	}

	void ps5_pause_menu::apply_live(const setting_row& setting)
	{
		// What the emulator reads as it runs takes the value now; the rest wait
		// for the game's next start, from its custom config
		cfg::_base* live = find_setting(g_cfg, setting.section, setting.key);
		if (!live || !live->get_is_dynamic())
		{
			return;
		}
		if (!live->from_string(setting.value))
		{
			pause_log.error("Could not set %s/%s to %s", setting.section, setting.key, setting.value);
			return;
		}
		pause_log.notice("%s/%s = %s, from now", setting.section, setting.key, setting.value);
		if (setting.section.starts_with("Video/Performance Overlay"))
		{
			reset_performance_overlay();
		}
	}

	void ps5_pause_menu::on_button_pressed(pad_button button_press, bool is_auto_repeat)
	{
		std::lock_guard lock(m_mutex);
		if (m_closing || m_tabs.empty())
		{
			return;
		}

		// Values change one step at a time even when held
		const bool left = button_press == pad_button::dpad_left || button_press == pad_button::ls_left;
		const bool right = button_press == pad_button::dpad_right || button_press == pad_button::ls_right;
		m_auto_repeat_ms_interval = (left || right) ? 150 : m_auto_repeat_ms_interval_default;

		// A state's deletion waits for a second Triangle, and any other button keeps it
		if (button_press != pad_button::triangle)
		{
			m_delete_armed = -1;
		}

		switch (button_press)
		{
		case pad_button::L1:
		case pad_button::R1:
		{
			if (is_auto_repeat)
			{
				return;
			}
			const s32 count = static_cast<s32>(m_tabs.size());
			m_tab = (m_tab + (button_press == pad_button::L1 ? count - 1 : 1)) % count;
			m_row = 0;
			m_scroll = 0;
			play_sound(sound_effect::cursor);
			return;
		}
		case pad_button::dpad_up:
		case pad_button::ls_up:
			move_to(m_row - 1);
			return;
		case pad_button::dpad_down:
		case pad_button::ls_down:
			move_to(m_row + 1);
			return;
		case pad_button::cross:
		{
			if (is_auto_repeat)
			{
				return;
			}
			row& current = m_tabs[m_tab].rows[m_row];
			if (current.act == action::reset_controls)
			{
				if (!ps5_pad_remap::is_identity(m_remap))
				{
					m_remap = ps5_pad_remap::identity();
					m_remap_changed = true;
					play_sound(sound_effect::cancel);
				}
			}
			else if (current.act == action::screenshot)
			{
				// Saved where it stands, the menu open still
				if (const std::string path = ps5_screenshots::save_held(m_serial); !path.empty())
				{
					current.note = "Saved, as " + path.substr(path.find("screenshots/")) + ". Again for another copy of the same moment";
					play_sound(sound_effect::accept);
					pause_log.notice("Screenshot saved as %s", path);
				}
				else
				{
					current.note = "Not yet: the game has not drawn a frame since this menu opened. Try again in a moment";
					play_sound(sound_effect::dialog_error);
				}
			}
			else if (current.act == action::open_states)
			{
				// To the States tab, on its newest state
				if (m_states_tab >= 0)
				{
					const tab& states = m_tabs[m_states_tab];
					m_tab = m_states_tab;
					m_row = 0;
					m_scroll = 0;
					for (usz r = 0; r < states.rows.size(); r++)
					{
						if (states.rows[r].state >= 0)
						{
							m_row = static_cast<s32>(r);
							break;
						}
					}
					play_sound(sound_effect::cursor);
				}
			}
			else if (current.act == action::load_state)
			{
				// The game is stopped to load it, so a state that can't load is found out first
				const saved_state& state = m_states[current.state];
				if (!fs::is_file(state.path))
				{
					current.note = "This state is not there any more.";
					play_sound(sound_effect::dialog_error);
				}
				else if (!is_savestate_compatible(state.path))
				{
					current.note = "This state was saved by a different version of the emulator, and this one can't load it.";
					play_sound(sound_effect::dialog_error);
				}
				else
				{
					m_load_path = state.path;
					play_sound(sound_effect::accept);
					begin_close(action::load_state);
				}
			}
			else if (current.act != action::none)
			{
				play_sound(current.act == action::resume ? sound_effect::cancel : sound_effect::accept);
				begin_close(current.act);
			}
			else
			{
				change(1);
			}
			return;
		}
		case pad_button::square:
		{
			const row& current = m_tabs[m_tab].rows[m_row];
			if (current.remap >= 0 && m_remap[current.remap] != current.remap)
			{
				m_remap[current.remap] = static_cast<u8>(current.remap);
				m_remap_changed = true;
				play_sound(sound_effect::cancel);
			}
			else if (current.setting >= 0)
			{
				setting_row& setting = m_settings[current.setting];
				if (setting.value != setting.global)
				{
					setting.value = setting.global;
					m_changed = true;
					apply_live(setting);
					play_sound(sound_effect::cancel);
				}
			}
			return;
		}
		case pad_button::triangle:
		{
			if (is_auto_repeat)
			{
				return;
			}
			tab& current_tab = m_tabs[m_tab];
			const row& current = current_tab.rows[m_row];
			if (current.state < 0)
			{
				return;
			}
			if (m_delete_armed != m_row)
			{
				m_delete_armed = m_row;
				play_sound(sound_effect::dialog_ok);
				return;
			}
			m_delete_armed = -1;
			const saved_state& state = m_states[current.state];
			if (!fs::remove_file(state.path) && fs::is_file(state.path))
			{
				current_tab.rows[m_row].note = fmt::format("This state could not be deleted (%s).", fs::g_tls_error);
				play_sound(sound_effect::dialog_error);
				return;
			}
			pause_log.notice("Deleted the state %s", state.path);
			current_tab.rows.erase(current_tab.rows.begin() + m_row);
			if (std::none_of(current_tab.rows.begin(), current_tab.rows.end(), [](const row& each) { return each.state >= 0; }))
			{
				current_tab.rows.push_back({action::none, -1, "No save states yet", "Save state saves the game as it is, to come back to that moment from here.", nullptr, true});
			}
			else if (m_row < static_cast<s32>(current_tab.rows.size()) && current_tab.rows[m_row].state >= 0 && (m_row == 0 || current_tab.rows[m_row - 1].state < 0))
			{
				// The first state left keeps the rule above the states
				current_tab.rows[m_row].rule_before = true;
			}
			m_row = std::clamp(m_row, 0, static_cast<s32>(current_tab.rows.size()) - 1);
			play_sound(sound_effect::cancel);
			return;
		}
		case pad_button::circle:
		case pad_button::start:
		case pad_button::ps:
			if (!is_auto_repeat)
			{
				play_sound(sound_effect::cancel);
				begin_close(action::resume);
			}
			return;
		default:
			break;
		}

		if (left || right)
		{
			change(left ? -1 : 1);
		}
	}

	void ps5_pause_menu::begin_close(action then)
	{
		m_closing = true;
		m_close_us = m_now_us;
		m_then = then;
	}

	void ps5_pause_menu::update(u64 timestamp_us)
	{
		{
			std::lock_guard lock(m_mutex);
			m_now_us = timestamp_us;
			if (!m_open_us)
			{
				m_open_us = timestamp_us;
			}
			if (!m_closing || m_closed)
			{
				return;
			}
			if (!m_close_us)
			{
				m_close_us = timestamp_us;
			}
			if ((timestamp_us - m_close_us) / 1'000'000.f < c_close_seconds)
			{
				return;
			}
			m_closed = true;
		}
		finish_close();
	}

	void ps5_pause_menu::finish_close()
	{
		// The moment held for a screenshot goes with the menu; the game's
		// controls, changed or not, apply again
		ps5_screenshots::drop_held();
		if (m_remap_changed)
		{
			ps5_pad_remap::save(m_serial, m_remap);
			ps5_pad_remap::set_active(m_remap);
			pause_log.notice("Controls for %s: %s", m_serial, ps5_pad_remap::is_identity(m_remap) ? "the controller's own" : "remapped");
		}
		ps5_pad_remap::hold_off(false);

		// The changes are the game's own from now on
		if (m_changed && !m_serial.empty())
		{
			write_game_settings(m_serial, m_settings);
		}

		close(true, true);

		switch (m_then)
		{
		case action::restart:
			pause_log.notice("Restarting the game");
			Emu.CallFromMainThread([]()
			{
				// The window stays, and the game boots again with its custom config
				Emu.SetContinuousMode(true);
				Emu.Restart(true);
			});
			break;
		case action::capture:
			if (g_cfg.video.frame_generation)
			{
				pause_log.notice("Back to the game, frame generation's frames to be saved");
				g_ps5_frame_gen_capture = 4;
			}
			else
			{
				pause_log.notice("Back to the game; frame generation is off, so no frames to save");
			}
			break;
		case action::save_state:
			pause_log.notice("Saving a state");
			Emu.CallFromMainThread([]()
			{
				// As the desktop's Save state key (gs_frame.cpp): the state is
				// saved as the emulation stops, and the game boots again from it
				// in the same window. The launcher stays the place a quit goes
				// back to (Kill leaves g_big_picture_mode_active alone while
				// after_kill_callback is set). Should saving fail (an SPU busy, a
				// save or a cutscene under way), Kill clears the callback, says
				// why, and the game runs on. Kill does nothing for a game still
				// starting or already stopping, and the callback would then
				// restart the game at its next stop: so only a running one
				if (!Emu.IsRunning() && !Emu.IsPaused())
				{
					pause_log.error("Not saving a state: the game is starting or stopping");
					return;
				}
				if (!g_cfg.savestate.suspend_emu)
				{
					Emu.after_kill_callback = []()
					{
						Emu.Restart(true, false);
					};
					Emu.SetContinuousMode(true);
				}
				Emu.Kill(false, true);
			});
			break;
		case action::load_state:
			pause_log.notice("Loading the state %s", m_load_path);
			Emu.CallFromMainThread([path = m_load_path]()
			{
				// The game stops where it is, nothing of it kept, and the state
				// boots in its place, as the desktop's Reload does
				// (boot_current_game_savestate)
				Emu.SetContinuousMode(true);
				Emu.after_kill_callback = [path]()
				{
					if (const game_boot_result result = Emu.BootGame(path, "", true); result != game_boot_result::no_errors)
					{
						pause_log.error("Loading the state %s failed: %s", path, result);
						ps5_launcher_message("Couldn't load the state", fmt::format("%s", result));
						g_big_picture_mode_active = false;
						Emu.BootBigPictureMode();
					}
				};
				Emu.Kill(false);
			});
			break;
		case action::quit:
			pause_log.notice("Quitting to the library");
			Emu.CallFromMainThread([]()
			{
				Emu.GracefulShutdown(true, true);
			});
			break;
		default:
			pause_log.notice("Back to the game");
			break;
		}
	}

	compiled_resource ps5_pause_menu::get_compiled()
	{
		if (!visible)
		{
			return {};
		}

		std::lock_guard lock(m_mutex);
		compiled_resource result;

		const f32 opened = m_open_us ? ease_out(progress((m_now_us - m_open_us) / 1'000'000.f, 0.f, c_open_seconds)) : 0.f;
		const f32 closed = m_closing && m_close_us ? ease_out(progress((m_now_us - m_close_us) / 1'000'000.f, 0.f, c_close_seconds)) : 0.f;
		const f32 shown = opened * (1.f - closed);

		// The game, dimmed and washed blue
		{
			overlay_element dim;
			dim.set_size(virtual_width, virtual_height);
			dim.back_color = color4f(0.f, 0.01f, 0.05f, 0.62f * shown);
			result.add(dim.get_compiled());
		}

		compiled_resource card;
		const auto glow = [&](s16 cx, s16 cy, u16 w, u16 h, const color4f& color)
		{
			image_view view;
			view.set_raw_image(m_glow.get());
			view.back_color.a = 0.f;
			view.fore_color = color;
			view.set_pos(static_cast<s16>(cx - w / 2), static_cast<s16>(cy - h / 2));
			view.set_size(w, h);
			card.add(view.get_compiled());
		};
		const auto text = [&](std::string_view s, u16 size, std::string_view font_name, const color4f& colour, s16 x, f32 mid) -> std::unique_ptr<label>
		{
			auto l = make_label(s, size, font_name, colour);
			place(*l, x, mid);
			card.add(l->get_compiled());
			return l;
		};
		const auto icon = [&](const char* path, s16 x, s16 y, u16 size, const color4f& colour)
		{
			if (!m_icons.contains(path))
			{
				auto& kept = m_icons[path];
				if ((kept = resource_config::load_icon(path)))
				{
					kept->dirty = true;
				}
			}
			if (const auto it = m_icons.find(path); it != m_icons.end() && it->second)
			{
				image_view view;
				view.set_raw_image(it->second.get());
				view.set_size(size, size);
				view.set_pos(x, y);
				view.back_color.a = 0.f;
				view.fore_color = colour;
				card.add(view.get_compiled());
			}
		};
		const auto glyph = [&](u8 resource, s16 x, f32 mid)
		{
			image_view view;
			view.set_image_resource(resource);
			view.set_size(20, 20);
			view.set_pos(x, static_cast<s16>(std::lround(mid - 10.f)));
			view.back_color.a = 0.f;
			card.add(view.get_compiled());
		};

		// The glass: a blue glow behind, a rim of cyan light, the deep blue
		// card, and the launcher's cyan edge along its top
		glow(c_card_x + c_card_w / 2, c_card_y + c_card_h / 2, 1700, 1150, color4f(0.1f, 0.22f, 0.7f, 0.5f));
		glow(c_card_x + c_card_w / 2, c_card_y + c_card_h / 2, 1150, 900, color4f(c_accent.r, c_accent.g, c_accent.b, 0.12f));
		{
			rounded_rect rim;
			rim.set_pos(c_card_x - 2, c_card_y - 2);
			rim.set_size(c_card_w + 4, c_card_h + 4);
			rim.border_radius = 30;
			rim.back_color = color4f(c_accent.r, c_accent.g, c_accent.b, 0.45f);
			card.add(rim.get_compiled());

			rounded_rect body;
			body.set_pos(c_card_x, c_card_y);
			body.set_size(c_card_w, c_card_h);
			body.border_radius = 28;
			body.back_color = color4f(0.02f, 0.05f, 0.19f, 0.94f);
			card.add(body.get_compiled());

			// Light inside the glass: a brighter blue in its middle, inside its edges
			glow(c_card_x + c_card_w / 2, c_card_y + c_card_h / 2 - 40, c_card_w - 60, c_card_h - 40, color4f(0.16f, 0.36f, 1.f, 0.32f));

			rounded_rect edge;
			edge.set_pos(c_card_x + 28, c_card_y);
			edge.set_size(c_card_w - 56, 3);
			edge.border_radius = 2;
			edge.back_color = c_accent;
			card.add(edge.get_compiled());
		}

		// The game: its icon, PAUSED, its name, its serial
		s16 words_x = c_left;
		if (m_icon)
		{
			rounded_image art;
			art.set_raw_image(m_icon.get());
			art.set_size(128, 70);
			art.set_pos(c_left, c_card_y + 34);
			art.border_radius = 10;
			art.back_color.a = 0.f;
			card.add(art.get_compiled());
			words_x = c_left + 128 + 22;
		}
		text(spaced("PAUSED"), 9, f_semibold, c_accent, words_x, c_card_y + 48.f);
		{
			auto title = make_label("", 22, f_bold, c_text);
			fit_text(*title, m_title, static_cast<u16>(c_inner_right - words_x));
			place(*title, words_x, c_card_y + 72.f);
			card.add(title->get_compiled());
		}
		text(m_serial.empty() ? std::string("Changes here last until the game ends") : m_serial + "   ·   Changes here are saved for this game",
			11, f_medium, c_text_dim, words_x, c_card_y + 96.f);

		// The tabs, L1 and R1 at their ends; the chosen one in a cyan pill
		{
			std::vector<std::unique_ptr<label>> names;
			f32 words = 0.f;
			for (usz t = 0; t < m_tabs.size(); t++)
			{
				names.push_back(make_label(m_tabs[t].name, 13, static_cast<s32>(t) == m_tab ? f_bold : f_medium, static_cast<s32>(t) == m_tab ? c_button_text : c_text_dim));
				words += names.back()->w;
			}
			// What room the names leave, shared out as padding
			const f32 room = static_cast<f32>(c_inner_right - c_left) - 2 * 30.f - words;
			const f32 pad = std::clamp(room / (2.f * names.size()), 6.f, 16.f);
			f32 x = c_left;
			glyph(static_cast<u8>(resource_config::standard_image_resource::L1), static_cast<s16>(x), c_tabs_mid);
			x += 30.f;
			for (usz t = 0; t < names.size(); t++)
			{
				const f32 w = names[t]->w + 2 * pad;
				if (static_cast<s32>(t) == m_tab)
				{
					rounded_rect pill;
					pill.set_pos(static_cast<s16>(std::lround(x)), static_cast<s16>(c_tabs_mid - 15.f));
					pill.set_size(static_cast<u16>(std::lround(w)), 30);
					pill.border_radius = 15;
					pill.back_color = c_accent;
					card.add(pill.get_compiled());
				}
				place(*names[t], static_cast<s16>(std::lround(x + pad)), c_tabs_mid);
				card.add(names[t]->get_compiled());
				x += w;
			}
			glyph(static_cast<u8>(resource_config::standard_image_resource::R1), static_cast<s16>(std::lround(x + 10.f)), c_tabs_mid);

			overlay_element rule;
			rule.set_pos(c_left, static_cast<s16>(c_tabs_mid + 26.f));
			rule.set_size(static_cast<u16>(c_inner_right - c_left), 1);
			rule.back_color = color4f(1.f, 1.f, 1.f, 0.12f);
			card.add(rule.get_compiled());
		}

		// The tab's rows
		const tab& current_tab = m_tabs[m_tab];
		const s32 count = static_cast<s32>(current_tab.rows.size());
		if (m_row < m_scroll) m_scroll = m_row;
		if (m_row >= m_scroll + c_visible_rows) m_scroll = m_row - c_visible_rows + 1;
		for (s32 r = m_scroll; r < count && r < m_scroll + c_visible_rows; r++)
		{
			const row& entry = current_tab.rows[r];
			const f32 y = c_rows_top + (r - m_scroll) * c_row_h;
			const f32 mid = y + c_row_h / 2.f;
			const bool selected = r == m_row;
			const setting_row* setting = entry.setting >= 0 ? &m_settings[entry.setting] : nullptr;
			const bool changed = setting && setting->value != setting->global;
			const color4f ink = selected ? c_button_text : ((changed || !setting) ? c_text : c_text_dim);

			if (entry.rule_before && r > m_scroll)
			{
				overlay_element rule;
				rule.set_pos(c_left, static_cast<s16>(y - 1));
				rule.set_size(static_cast<u16>(c_inner_right - c_left), 1);
				rule.back_color = color4f(1.f, 1.f, 1.f, 0.16f);
				card.add(rule.get_compiled());
			}
			if (selected)
			{
				// A glow of its own under the lit pill
				glow(static_cast<s16>((c_left + c_inner_right) / 2), static_cast<s16>(mid), c_card_w, 120, color4f(c_accent.r, c_accent.g, c_accent.b, 0.22f));
				rounded_rect pill;
				pill.set_pos(c_left - 16, static_cast<s16>(y + 3));
				pill.set_size(static_cast<u16>(c_inner_right - c_left + 32), static_cast<u16>(c_row_h - 6));
				pill.border_radius = static_cast<u16>((c_row_h - 6) / 2);
				pill.back_color = c_accent;
				card.add(pill.get_compiled());
			}

			s16 name_x = c_left + 8;
			if (entry.icon)
			{
				icon(entry.icon, static_cast<s16>(c_left + 4), static_cast<s16>(std::lround(mid - 9.f)), 18, ink);
				name_x = c_left + 36;
			}
			if (changed)
			{
				ellipse dot;
				dot.set_size(6, 6);
				dot.set_pos(c_left - 4, static_cast<s16>(std::lround(mid - 3.f)));
				dot.back_color = selected ? c_button_text : c_accent;
				card.add(dot.get_compiled());
			}

			s16 name_room = static_cast<s16>(c_inner_right - name_x);
			if (entry.remap >= 0)
			{
				// The controller's button that presses it, accent when not its own
				const bool moved = m_remap[entry.remap] != entry.remap;
				if (moved && !selected)
				{
					ellipse dot;
					dot.set_size(6, 6);
					dot.set_pos(c_left - 4, static_cast<s16>(std::lround(mid - 3.f)));
					dot.back_color = c_accent;
					card.add(dot.get_compiled());
				}
				auto value = make_label(ps5_pad_remap::source_name(m_remap[entry.remap]), 13, f_semibold, selected ? c_button_text : (moved ? c_accent : c_text_dim));
				const s16 right = static_cast<s16>(c_inner_right - (selected ? 18 : 0));
				place(*value, static_cast<s16>(right - value->w), mid);
				card.add(value->get_compiled());
				if (selected)
				{
					text("‹", 16, f_bold, c_button_text, static_cast<s16>(value->x - 18), mid);
					text("›", 16, f_bold, c_button_text, static_cast<s16>(right + 8), mid);
				}
				name_room = static_cast<s16>(value->x - (selected ? 30 : 16) - name_x);
			}
			else if (entry.perf >= 0)
			{
				// A measurement: the five seconds' before the menu opened
				auto value = make_label("", 13, f_semibold, selected ? c_button_text : c_accent);
				fit_text(*value, perf_value(m_perf_before, entry.perf), 300);
				const s16 right = static_cast<s16>(c_inner_right - (selected ? 10 : 0));
				place(*value, static_cast<s16>(right - value->w), mid);
				card.add(value->get_compiled());
				name_room = static_cast<s16>(value->x - 16 - name_x);
			}
			else if (entry.state >= 0)
			{
				// A save state: its size, or what a second Triangle does
				const bool armed = m_delete_armed == r;
				auto value = make_label(armed ? std::string("Triangle again deletes it") : size_text(m_states[entry.state].size), 13, f_semibold, selected ? c_button_text : (armed ? c_accent : c_text_dim));
				const s16 right = static_cast<s16>(c_inner_right - (selected ? 10 : 0));
				place(*value, static_cast<s16>(right - value->w), mid);
				card.add(value->get_compiled());
				name_room = static_cast<s16>(value->x - 16 - name_x);
			}
			else if (setting)
			{
				auto value = make_label("", 13, f_semibold, selected ? c_button_text : (changed ? c_accent : c_text_dim));
				fit_text(*value, setting_text(setting->key, setting->value), 280);
				const s16 right = static_cast<s16>(c_inner_right - (selected ? 18 : 0));
				place(*value, static_cast<s16>(right - value->w), mid);
				card.add(value->get_compiled());
				if (selected)
				{
					text("‹", 16, f_bold, c_button_text, static_cast<s16>(value->x - 18), mid);
					text("›", 16, f_bold, c_button_text, static_cast<s16>(right + 8), mid);
				}
				name_room = static_cast<s16>(value->x - (selected ? 30 : 16) - name_x);
			}
			else if (selected)
			{
				text("›", 20, f_bold, c_button_text, static_cast<s16>(c_inner_right - 6), mid);
			}

			auto name = make_label("", 14, selected ? f_bold : f_medium, ink);
			fit_text(*name, entry.label, static_cast<u16>(std::max<s16>(80, name_room)));
			place(*name, name_x, mid);
			card.add(name->get_compiled());
		}
		if (count > c_visible_rows)
		{
			// Where in the tab
			const f32 track = c_visible_rows * c_row_h;
			const f32 thumb = track * c_visible_rows / count;
			rounded_rect bar;
			bar.set_pos(c_inner_right + 26, static_cast<s16>(c_rows_top + (track - thumb) * m_scroll / (count - c_visible_rows)));
			bar.set_size(4, static_cast<u16>(thumb));
			bar.border_radius = 2;
			bar.back_color = color4f(1.f, 1.f, 1.f, 0.25f);
			card.add(bar.get_compiled());
		}

		// The row, explained: a setting's tooltip and when it takes effect, or
		// what an action does
		{
			overlay_element rule;
			rule.set_pos(c_left, c_help_top - 6);
			rule.set_size(static_cast<u16>(c_inner_right - c_left), 1);
			rule.back_color = color4f(1.f, 1.f, 1.f, 0.12f);
			card.add(rule.get_compiled());

			const row& entry = current_tab.rows[std::clamp(m_row, 0, count - 1)];
			std::string help;
			std::string when;
			if (entry.setting >= 0)
			{
				const setting_row& setting = m_settings[entry.setting];
				// The collision setting's and frame generation method's: what they do
				// now, then the tooltip
				std::string status = setting.key == "SPU Collision Accuracy" ? spu_collision_status() : std::string();
				if (setting.key == "Frame Generation Method")
				{
					const std::string lsfg = vk::lsfg_status();
					status = lsfg != "not started" ? "Lossless Scaling: " + lsfg + "." : vk::lsfg_dll_path().empty() ? std::string("Lossless.dll: not found.") : std::string("Lossless.dll: found.");
				}
				help = short_help(status.empty() ? setting.help : status + " " + setting.help);
				const cfg::_base* live = find_setting(g_cfg, setting.section, setting.key);
				when = (live && live->get_is_dynamic()) ? "Takes effect now." : "Takes effect after a restart of the game.";
				if (setting.value != setting.global)
				{
					when += "  Global: " + setting_text(setting.key, setting.global) + ".";
				}
			}
			else if (entry.remap >= 0)
			{
				help = fmt::format("Which of the controller's buttons presses the PS3's %s in this game. L1 + R1 + the touch pad's click opens this menu whatever is chosen, and the menu reads the controller as it is.", ps5_pad_remap::ps3_name(static_cast<u8>(entry.remap)));
				when = "Takes effect when the game resumes; kept for this game.";
			}
			else if (entry.perf >= 0)
			{
				// What it measures, and the same now, under the menu (measured
				// every 5 seconds: the game is usually paused by now)
				const ps5_perf now = ps5_perf_latest();
				help = perf_note(entry.perf);
				if (now.valid && now.at_us != m_perf_before.at_us)
				{
					when = "Now, under this menu: " + perf_value(now, entry.perf);
				}
			}
			else if (entry.state >= 0 && m_delete_armed == std::clamp(m_row, 0, count - 1))
			{
				help = "Press Triangle again to delete this state for good. Any other button keeps it.";
			}
			else
			{
				help = entry.note;
			}

			label words;
			words.set_font(11, f_regular);
			words.fore_color = c_text_dim;
			words.back_color.a = 0.f;
			words.set_padding(0);
			words.set_wrap_text(true);
			words.set_text(help);
			words.set_pos(c_left, c_help_top + 6);
			words.set_size(static_cast<u16>(c_inner_right - c_left), 34);
			words.auto_resize(false, static_cast<u16>(c_inner_right - c_left), 34);
			card.add(words.get_compiled());
			if (!when.empty())
			{
				text(when, 11, f_semibold, c_accent, c_left, c_help_top + 52.f);
			}
		}

		// The buttons' prompts along the foot
		{
			f32 x = c_left;
			const auto prompt = [&](u8 resource, std::string_view what)
			{
				glyph(resource, static_cast<s16>(std::lround(x)), c_hints_mid);
				auto l = text(what, 12, f_semibold, c_text, static_cast<s16>(std::lround(x + 26.f)), c_hints_mid);
				x += 26.f + l->w + 26.f;
			};
			const row& entry = current_tab.rows[std::clamp(m_row, 0, count - 1)];
			prompt(resource_config::confirm_button_resource(), (entry.setting >= 0 || entry.remap >= 0) ? "Change" : entry.state >= 0 ? "Load" : "Select");
			prompt(resource_config::cancel_button_resource(), "Resume");
			if (entry.state >= 0)
			{
				prompt(static_cast<u8>(resource_config::standard_image_resource::triangle), "Delete");
			}
			if (entry.setting >= 0 && m_settings[entry.setting].value != m_settings[entry.setting].global)
			{
				prompt(static_cast<u8>(resource_config::standard_image_resource::square), "Global value");
			}
			if (entry.remap >= 0 && m_remap[entry.remap] != entry.remap)
			{
				prompt(static_cast<u8>(resource_config::standard_image_resource::square), "Its own button");
			}

			auto tabs = make_label("Tabs", 12, f_semibold, c_text_dim);
			place(*tabs, static_cast<s16>(c_inner_right - tabs->w), c_hints_mid);
			card.add(tabs->get_compiled());
			glyph(static_cast<u8>(resource_config::standard_image_resource::R1), static_cast<s16>(tabs->x - 28), c_hints_mid);
			glyph(static_cast<u8>(resource_config::standard_image_resource::L1), static_cast<s16>(tabs->x - 52), c_hints_mid);
		}

		add_animated(result, card, shown, 0.f, 18.f * (1.f - opened) + 10.f * closed);
		return result;
	}

	error_code ps5_pause_menu::show(std::function<void(s32 status)> on_close)
	{
		this->on_close = std::move(on_close);
		visible = true;

		// The emulation runs on under the menu, as a PS3's does under its own:
		// the game hears that the system menu opened (send_open_home_menu_cmds)
		// and pauses itself, GTA IV into its own pause menu. Pausing the
		// emulator too froze the app on my console (build 91): the menu opened,
		// and nothing drew or answered after
		const auto notify = std::make_shared<atomic_t<u32>>(0);
		auto& overlayman = g_fxo->get<display_manager>();
		overlayman.attach_thread_input(uid, "PS5 pause menu", [notify]()
		{
			pause_log.notice("Reading the controller");
			*notify = true;
			notify->notify_one();
		});
		pause_log.notice("Opened over '%s' (%s)", m_title, m_serial);

		while (!Emu.IsStopped() && !*notify)
		{
			notify->wait(false, atomic_wait_timeout{1'000'000});
		}
		return CELL_OK;
	}

	error_code open_ps5_pause_menu(std::function<void(s32 status)> on_close)
	{
		auto manager = g_fxo->try_get<display_manager>();
		if (!manager)
		{
			return CELL_EFAULT;
		}
		return manager->create<ps5_pause_menu>()->show(std::move(on_close));
	}
}
