#include "stdafx.h"
#include "overlays.h"
#include "../GSRender.h"

LOG_CHANNEL(overlays);

static auto s_ascii_lowering_map = []()
{
	std::unordered_map<u32, u8> _map;

	// Fullwidth block (FF00-FF5E)
	for (u32 u = 0xFF01, c = 0x21; u <= 0xFF5E; ++u, ++c)
	{
		_map[u] = u8(c);
	}

	// Em and En space variations (General Punctuation)
	for (u32 u = 0x2000; u <= 0x200A; ++u)
	{
		_map[u] = u8(' ');
	}

	// Misc space variations
	_map[0x202F] = u8(0xA0); // narrow NBSP
	_map[0x205F] = u8(' ');  // medium mathematical space
	_map[0x3164] = u8(' ');  // hangul filler

	// Ideographic (CJK punctuation)
	_map[0x3000] = u8(' ');  // space
	_map[0x3001] = u8(',');  // comma
	_map[0x3002] = u8('.');  // fullstop
	_map[0x3003] = u8('"');  // ditto
	_map[0x3007] = u8('0');  // wide zero
	_map[0x3008] = u8('<');  // left angle brace
	_map[0x3009] = u8('>');  // right angle brace
	_map[0x300A] = u8(0xAB); // double left angle brace
	_map[0x300B] = u8(0xBB); // double right angle brace
	_map[0x300C] = u8('[');  // the following are all slight variations on the angular brace
	_map[0x300D] = u8(']');
	_map[0x300E] = u8('[');
	_map[0x300F] = u8(']');
	_map[0x3010] = u8('[');
	_map[0x3011] = u8(']');
	_map[0x3014] = u8('[');
	_map[0x3015] = u8(']');
	_map[0x3016] = u8('[');
	_map[0x3017] = u8(']');
	_map[0x3018] = u8('[');
	_map[0x3019] = u8(']');
	_map[0x301A] = u8('[');
	_map[0x301B] = u8(']');
	_map[0x301C] = u8('~');  // wave dash (inverted tilde)
	_map[0x301D] = u8('"');  // reverse double prime quotation
	_map[0x301E] = u8('"');  // double prime quotation
	_map[0x301F] = u8('"');  // low double prime quotation
	_map[0x3031] = u8('<');  // vertical kana repeat mark

	return _map;
}();

std::string utf8_to_ascii8(const std::string& utf8_string)
{
	std::string out;
	out.reserve(utf8_string.length());

	const auto end = utf8_string.length();
	for (u32 index = 0; index < end; ++index)
	{
		const u8 code = static_cast<u8>(utf8_string[index]);

		if (!code)
		{
			break;
		}

		if (code <= 0x7F)
		{
			out.push_back(code);
			continue;
		}

		const auto extra_bytes = (code <= 0xDF) ? 1u : (code <= 0xEF) ? 2u : 3u;
		if ((index + extra_bytes) > end)
		{
			// Malformed string, abort
			overlays.error("Failed to decode supossedly malformed utf8 string '%s'", utf8_string);
			break;
		}

		u32 u_code = 0;
		switch (extra_bytes)
		{
		case 1:
			// 11 bits, 6 + 5
			u_code = (u32(code & 0x1F) << 6) | u32(utf8_string[index + 1] & 0x3F);
			break;
		case 2:
			// 16 bits, 6 + 6 + 4
			u_code = (u32(code & 0xF) << 12) | (u32(utf8_string[index + 1] & 0x3F) << 6) | u32(utf8_string[index + 2] & 0x3F);
			break;
		case 3:
			// 21 bits, 6 + 6 + 6 + 3
			u_code = (u32(code & 0x7) << 18) | (u32(utf8_string[index + 1] & 0x3F) << 12) | (u32(utf8_string[index + 2] & 0x3F) << 6) | u32(utf8_string[index + 3] & 0x3F);
			break;
		default:
			fmt::throw_exception("Unreachable" HERE);
		}

		index += extra_bytes;

		if (u_code <= 0xFF)
		{
			// Latin-1 supplement block
			out.push_back(static_cast<u8>(u_code));
			continue;
		}

		auto replace = s_ascii_lowering_map.find(u_code);
		if (replace == s_ascii_lowering_map.end())
		{
			out.push_back('#');
			continue;
		}

		out.push_back(replace->second);
	}

	return out;
}

std::string utf16_to_ascii8(const std::u16string& utf16_string)
{
	// Strip extended codes, map to '#' instead (placeholder)
	std::string out;
	out.reserve(utf16_string.length());

	for (const auto& code : utf16_string)
	{
		if (!code)
			break;

		out.push_back(code > 0xFF ? '#': static_cast<char>(code));
	}

	return out;
}

std::u16string ascii8_to_utf16(const std::string& ascii_string)
{
	std::u16string out;
	out.reserve(ascii_string.length());

	for (const auto& code : ascii_string)
	{
		if (!code)
			break;

		out.push_back(static_cast<char16_t>(code));
	}

	return out;
}

namespace rsx
{
	namespace overlays
	{
		// Singleton instance declaration
		fontmgr* fontmgr::m_instance = nullptr;

		s32 user_interface::run_input_loop()
		{
			const u64 ms_interval = 200;
			std::array<std::chrono::steady_clock::time_point, CELL_PAD_MAX_PORT_NUM> timestamp;
			timestamp.fill(std::chrono::steady_clock::now());

			const u64 ms_threshold = 500;
			std::array<std::chrono::steady_clock::time_point, CELL_PAD_MAX_PORT_NUM> initial_timestamp;
			initial_timestamp.fill(std::chrono::steady_clock::now());

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
						rsx_log.fatal("The native overlay cannot handle more than 7 pads! Current number of pads: %d", pad_index + 1);
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
									if (!button_state[pad_index][button_id])
									{
										// the d-pad button was not pressed before, so this is a new button press
										timestamp[pad_index] = std::chrono::steady_clock::now();
										initial_timestamp[pad_index] = timestamp[pad_index];
										on_button_pressed(static_cast<pad_button>(button_id));
									}
									else if (input_timer.GetMsSince(initial_timestamp[pad_index]) > ms_threshold && input_timer.GetMsSince(timestamp[pad_index]) > ms_interval)
									{
										// the d-pad button was pressed for at least the given threshold in ms and will trigger at an interval
										timestamp[pad_index] = std::chrono::steady_clock::now();
										on_button_pressed(static_cast<pad_button>(button_id));
									}
								}
								else if (!button_state[pad_index][button_id])
								{
									// the button was not pressed before, so this is a new button press
									on_button_pressed(static_cast<pad_button>(button_id));
								}
							}

							button_state[pad_index][button_id] = button.m_pressed;
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

		void user_interface::close(bool use_callback)
		{
			// Force unload
			exit.release(true);
			{
				reader_lock lock(m_threadpool_mutex);
				for (auto& worker : m_workers)
				{
					if (std::this_thread::get_id() != worker.get_id() && worker.joinable())
					{
						worker.join();
					}
					else
					{
						worker.detach();
					}
				}
			}

			pad::SetIntercepted(false);

			if (on_close && use_callback)
			{
				g_last_user_response = return_code;
				on_close(return_code);
			}

			// NOTE: Object removal should be the last step
			if (auto manager = g_fxo->get<display_manager>())
			{
				if (auto dlg = manager->get<rsx::overlays::message_dialog>())
				{
					if (dlg->progress_bar_count())
						Emu.GetCallbacks().handle_taskbar_progress(0, 1);
				}

				manager->remove(uid);
			}
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
