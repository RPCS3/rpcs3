#include "stdafx.h"
#include "overlays.h"
#include "../GSRender.h"

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

			std::array<std::array<bool, 8>, CELL_PAD_MAX_PORT_NUM> button_state;
			for (auto& state : button_state)
			{
				state.fill(true);
			}

			input_timer.Start();

			{
				std::lock_guard lock(pad::g_pad_mutex);
				const auto handler = pad::get_current_handler();
				handler->SetIntercepted(true);
			}

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

			{
				std::lock_guard lock(pad::g_pad_mutex);
				const auto handler = pad::get_current_handler();
				handler->SetIntercepted(false);
			}

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
