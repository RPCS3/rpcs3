#include "stdafx.h"
#include "overlays.h"
#include "../GSRender.h"

#include <locale>
#include <codecvt>

#if _MSC_VER >= 1900
// Stupid MSVC bug when T is set to char16_t
std::string utf16_to_utf8(const std::u16string& utf16_string)
{
	// Strip extended codes
	auto tmp = utf16_string;
	for (auto &c : tmp)
	{
		if (c > 0xFF) c = '#';
	}

	std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
	auto p = reinterpret_cast<const int16_t *>(tmp.data());
	return convert.to_bytes(p, p + utf16_string.size());
}

std::u16string utf8_to_utf16(const std::string& utf8_string)
{
	std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
	auto ws = convert.from_bytes(utf8_string);
	return reinterpret_cast<const char16_t*>(ws.c_str());
}

#else

std::string utf16_to_utf8(const std::u16string& utf16_string)
{
	// Strip extended codes
	auto tmp = utf16_string;
	for (auto &c : tmp)
	{
		if (c > 0xFF) c = '#';
	}

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
	return convert.to_bytes(tmp);
}

std::u16string utf8_to_utf16(const std::string& utf8_string)
{
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
	return convert.from_bytes(utf8_string);
}

#endif

namespace rsx
{
	namespace overlays
	{
		// Singleton instance declaration
		fontmgr* fontmgr::m_instance = nullptr;

		s32 user_interface::run_input_loop()
		{
			std::array<std::chrono::steady_clock::time_point, CELL_PAD_MAX_PORT_NUM> timestamp;
			timestamp.fill(std::chrono::steady_clock::now());

			std::array<std::array<bool, pad_button::pad_button_max_enum>, CELL_PAD_MAX_PORT_NUM> button_state;
			for (auto& state : button_state)
			{
				state.fill(true);
			}

			input_timer.Start();

			pad::SetIntercepted(true);

			while (!exit)
			{
				if (Emu.IsStopped())
					return selection_code::canceled;

				std::this_thread::sleep_for(1ms);

				std::lock_guard lock(pad::g_pad_mutex);

				const auto handler = pad::get_current_handler();

				const PadInfo& rinfo = handler->GetInfo();

				if (Emu.IsPaused() || !rinfo.now_connect)
				{
					continue;
				}

				int pad_index = -1;
				for (const auto &pad : handler->GetPads())
				{
					if (++pad_index >= CELL_PAD_MAX_PORT_NUM)
					{
						LOG_FATAL(RSX, "The native overlay cannot handle more than 7 pads! Current number of pads: %d", pad_index + 1);
						continue;
					}

					for (auto &button : pad->m_buttons)
					{
						u8 button_id = 255;
						if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
						{
							switch (button.m_outKeyCode)
							{
							case CELL_PAD_CTRL_LEFT:
								button_id = pad_button::dpad_left;
								break;
							case CELL_PAD_CTRL_RIGHT:
								button_id = pad_button::dpad_right;
								break;
							case CELL_PAD_CTRL_DOWN:
								button_id = pad_button::dpad_down;
								break;
							case CELL_PAD_CTRL_UP:
								button_id = pad_button::dpad_up;
								break;
							case CELL_PAD_CTRL_SELECT:
								button_id = pad_button::select;
								break;
							case CELL_PAD_CTRL_START:
								button_id = pad_button::start;
								break;
							}
						}
						else if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
						{
							switch (button.m_outKeyCode)
							{
							case CELL_PAD_CTRL_TRIANGLE:
								button_id = pad_button::triangle;
								break;
							case CELL_PAD_CTRL_CIRCLE:
								button_id = g_cfg.sys.enter_button_assignment == enter_button_assign::circle ? pad_button::cross : pad_button::circle;
								break;
							case CELL_PAD_CTRL_SQUARE:
								button_id = pad_button::square;
								break;
							case CELL_PAD_CTRL_CROSS:
								button_id = g_cfg.sys.enter_button_assignment == enter_button_assign::circle ? pad_button::circle : pad_button::cross;
								break;
							case CELL_PAD_CTRL_L1:
								button_id = pad_button::L1;
								break;
							case CELL_PAD_CTRL_R1:
								button_id = pad_button::R1;
								break;
							}
						}

						if (button_id < 255)
						{
							if (button.m_pressed)
							{
								if (button_id < 4) // d-pad button
								{
									if (!button_state[pad_index][button_id] || input_timer.GetMsSince(timestamp[pad_index]) > 400)
									{
										// d-pad button was not pressed, or was pressed more than 400ms ago
										timestamp[pad_index] = std::chrono::steady_clock::now();
										on_button_pressed(static_cast<pad_button>(button_id));
									}
								}
								else if (!button_state[pad_index][button_id])
								{
									// button was not pressed
									on_button_pressed(static_cast<pad_button>(button_id));
								}
							}

							button_state[pad_index][button_id] = button.m_pressed;
						}

						if (button.m_flush)
						{
							button.m_pressed = false;
							button.m_flush = false;
							button.m_value = 0;
						}

						if (exit)
							return 0;
					}
				}

				refresh();
			}

			// Unreachable
			return 0;
		}

		void user_interface::close()
		{
			// Force unload
			exit = true;
			if (auto manager = fxm::get<display_manager>())
			{
				if (auto dlg = manager->get<rsx::overlays::message_dialog>())
				{
					if (dlg->progress_bar_count())
						Emu.GetCallbacks().handle_taskbar_progress(0, 1);
				}

				manager->remove(uid);
			}

			pad::SetIntercepted(false);

			if (on_close)
				on_close(return_code);
		}

		void overlay::refresh()
		{
			if (auto rsxthr = rsx::get_current_renderer())
			{
				const auto now = get_system_time() - 1000000;
				if ((now - rsxthr->last_flip_time) > min_refresh_duration_us)
				{
					rsxthr->async_flip_requested |= rsx::thread::flip_request::native_ui;
				}
			}
		}
	} // namespace overlays
} // namespace rsx
