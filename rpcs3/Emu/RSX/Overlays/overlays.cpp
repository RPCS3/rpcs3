#include "stdafx.h"
#include "overlays.h"
#include "overlay_message_dialog.h"
#include "Input/pad_thread.h"
#include "Emu/Io/interception.h"
#include "Emu/RSX/RSXThread.h"

LOG_CHANNEL(overlays);

namespace rsx
{
	namespace overlays
	{
		thread_local DECLARE(user_interface::g_thread_bit) = 0;

		u64 user_interface::alloc_thread_bit()
		{
			auto [_old, ok] = this->thread_bits.fetch_op([](u64& bits)
			{
				if (~bits)
				{
					// Set lowest clear bit
					bits |= bits + 1;
					return true;
				}

				return false;
			});

			if (!ok)
			{
				::overlays.fatal("Out of thread bits in user interface");
				return 0;
			}

			const u64 r = u64{1} << std::countr_one(_old);
			::overlays.trace("Bit allocated (%u)", r);
			return r;
		}

		// Singleton instance declaration
		fontmgr* fontmgr::m_instance = nullptr;

		s32 user_interface::run_input_loop()
		{
			const u64 ms_interval = 200;
			std::array<steady_clock::time_point, CELL_PAD_MAX_PORT_NUM> timestamp;
			timestamp.fill(steady_clock::now());

			const u64 ms_threshold = 500;
			std::array<steady_clock::time_point, CELL_PAD_MAX_PORT_NUM> initial_timestamp;
			initial_timestamp.fill(steady_clock::now());

			std::array<u8, CELL_PAD_MAX_PORT_NUM> last_auto_repeat_button;
			last_auto_repeat_button.fill(pad_button::pad_button_max_enum);

			std::array<std::array<bool, pad_button::pad_button_max_enum>, CELL_PAD_MAX_PORT_NUM> last_button_state;
			for (auto& state : last_button_state)
			{
				// Initialize last button states as pressed to avoid unwanted button presses when entering the dialog.
				state.fill(true);
			}

			input_timer.Start();

			input::SetIntercepted(true);

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
						u8 button_id = pad_button::pad_button_max_enum;
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
							default: break;
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
							case CELL_PAD_CTRL_L2:
								button_id = pad_button::L2;
								break;
							case CELL_PAD_CTRL_R2:
								button_id = pad_button::R2;
								break;
							default: break;
							}
						}

						if (button_id < pad_button::pad_button_max_enum)
						{
							if (button.m_pressed)
							{
								const bool is_auto_repeat_button = auto_repeat_buttons.contains(button_id);

								if (!last_button_state[pad_index][button_id])
								{
									// The button was not pressed before, so this is a new button press. Reset auto-repeat.
									timestamp[pad_index] = steady_clock::now();
									initial_timestamp[pad_index] = timestamp[pad_index];
									last_auto_repeat_button[pad_index] = is_auto_repeat_button ? button_id : pad_button::pad_button_max_enum;
									on_button_pressed(static_cast<pad_button>(button_id));
								}
								else if (is_auto_repeat_button)
								{
									if (last_auto_repeat_button[pad_index] == button_id
									    && input_timer.GetMsSince(initial_timestamp[pad_index]) > ms_threshold
									    && input_timer.GetMsSince(timestamp[pad_index]) > ms_interval)
									{
										// The auto-repeat button was pressed for at least the given threshold in ms and will trigger at an interval.
										timestamp[pad_index] = steady_clock::now();
										on_button_pressed(static_cast<pad_button>(button_id));
									}
									else if (last_auto_repeat_button[pad_index] == pad_button::pad_button_max_enum)
									{
										// An auto-repeat button was already pressed before and will now start triggering again after the next threshold.
										last_auto_repeat_button[pad_index] = button_id;
									}
								}
							}
							else if (last_button_state[pad_index][button_id] && last_auto_repeat_button[pad_index] == button_id)
							{
								// We stopped pressing an auto-repeat button, so re-enable auto-repeat for other buttons.
								last_auto_repeat_button[pad_index] = pad_button::pad_button_max_enum;
							}

							last_button_state[pad_index][button_id] = button.m_pressed;
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

		void user_interface::close(bool use_callback, bool stop_pad_interception)
		{
			// Force unload
			exit.release(true);

			while (u64 b = thread_bits)
			{
				if (b == g_thread_bit)
				{
					// Don't wait for its own bit
					break;
				}

				thread_bits.wait(b);
			}

			if (stop_pad_interception)
			{
				input::SetIntercepted(false);
			}

			if (on_close && use_callback)
			{
				g_last_user_response = return_code;
				on_close(return_code);
			}

			// NOTE: Object removal should be the last step
			if (auto& manager = g_fxo->get<display_manager>(); g_fxo->is_init<display_manager>())
			{
				manager.remove(uid);
			}
		}

		void overlay::refresh() const
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
