#include "stdafx.h"
#include "overlays.h"
#include "overlay_message_dialog.h"
#include "Input/pad_thread.h"
#include "Emu/Io/interception.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Common/time.hpp"

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
			m_interactive = true;

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

			m_input_timer.Start();

			// Only start intercepting input if the the overlay allows it (enabled by default)
			if (m_start_pad_interception)
			{
				input::SetIntercepted(true);
			}

			const auto handle_button_press = [&](u8 button_id, bool pressed, int pad_index)
			{
				if (button_id >= pad_button::pad_button_max_enum)
				{
					return;
				}

				if (pressed)
				{
					const bool is_auto_repeat_button = m_auto_repeat_buttons.contains(button_id);

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
						    && m_input_timer.GetMsSince(initial_timestamp[pad_index]) > ms_threshold
						    && m_input_timer.GetMsSince(timestamp[pad_index]) > ms_interval)
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

				last_button_state[pad_index][button_id] = pressed;
			};

			while (!exit)
			{
				std::this_thread::sleep_for(1ms);

				if (Emu.IsStopped())
					return selection_code::canceled;

				if (Emu.IsPaused())
					continue;

				// Get keyboard input if supported by the overlay and activated by the game.
				// Ignored if a keyboard pad handler is active in order to prevent double input.
				if (m_keyboard_input_enabled && !m_keyboard_pad_handler_active && input::g_keyboards_intercepted)
				{
					auto& handler = g_fxo->get<KeyboardHandlerBase>();
					std::lock_guard<std::mutex> lock(handler.m_mutex);

					if (!handler.GetKeyboards().empty() && handler.GetInfo().status[0] == CELL_KB_STATUS_CONNECTED)
					{
						KbData& current_data = handler.GetData(0);

						if (current_data.len > 0)
						{
							for (s32 i = 0; i < current_data.len; i++)
							{
								const KbButton& key = current_data.buttons[i];
								on_key_pressed(current_data.led, current_data.mkey, key.m_keyCode, key.m_outKeyCode, key.m_pressed);
							}

							// Flush buffer unconditionally. Otherwise we get a flood of key events.
							current_data.len = 0;

							// Ignore gamepad input if a key was recognized
							refresh();
							continue;
						}
					}
					else if (g_cfg.io.keyboard != keyboard_handler::null)
					{
						// Workaround if cellKb did not init the keyboard handler.
						handler.Init(1);

						// Enable key repeat
						std::vector<Keyboard>& keyboards = handler.GetKeyboards();
						ensure(!keyboards.empty());
						keyboards.at(0).m_key_repeat = true;
					}
				}

				// Get gamepad input
				std::lock_guard lock(pad::g_pad_mutex);
				const auto handler = pad::get_current_handler();
				const PadInfo& rinfo = handler->GetInfo();

				if (!rinfo.now_connect || !input::g_pads_intercepted)
				{
					refresh();
					continue;
				}

				bool keyboard_pad_handler_active = false;

				int pad_index = -1;
				for (const auto& pad : handler->GetPads())
				{
					if (exit)
						break;

					if (++pad_index >= CELL_PAD_MAX_PORT_NUM)
					{
						rsx_log.fatal("The native overlay cannot handle more than 7 pads! Current number of pads: %d", pad_index + 1);
						continue;
					}

					if (!pad)
					{
						rsx_log.fatal("Pad %d is nullptr", pad_index);
						continue;
					}

					if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
					{
						continue;
					}

					if (pad->m_pad_handler == pad_handler::keyboard)
					{
						m_keyboard_pad_handler_active = true;
					}

					for (const Button& button : pad->m_buttons)
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
							case CELL_PAD_CTRL_L3:
								button_id = pad_button::L3;
								break;
							case CELL_PAD_CTRL_R3:
								button_id = pad_button::R3;
								break;
							case CELL_PAD_CTRL_SELECT:
								button_id = pad_button::select;
								break;
							case CELL_PAD_CTRL_START:
								button_id = pad_button::start;
								break;
							default:
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
							case CELL_PAD_CTRL_L2:
								button_id = pad_button::L2;
								break;
							case CELL_PAD_CTRL_R2:
								button_id = pad_button::R2;
								break;
							case CELL_PAD_CTRL_PS:
								button_id = pad_button::ps;
								break;
							default:
								break;
							}
						}

						handle_button_press(button_id, button.m_pressed, pad_index);

						if (exit)
							break;
					}

					for (const AnalogStick& stick : pad->m_sticks)
					{
						u8 button_id = pad_button::pad_button_max_enum;
						u8 release_id = pad_button::pad_button_max_enum;

						// Let's say sticks are only pressed if they are almost completely tilted. Otherwise navigation feels really wacky.
						const bool pressed = stick.m_value < 30 || stick.m_value > 225;

						switch (stick.m_offset)
						{
						case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X:
							button_id = (stick.m_value <= 128) ? pad_button::ls_left : pad_button::ls_right;
							release_id = (stick.m_value > 128) ? pad_button::ls_left : pad_button::ls_right;
							break;
						case CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y:
							button_id = (stick.m_value <= 128) ? pad_button::ls_up : pad_button::ls_down;
							release_id = (stick.m_value > 128) ? pad_button::ls_up : pad_button::ls_down;
							break;
						case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X:
							button_id = (stick.m_value <= 128) ? pad_button::rs_left : pad_button::rs_right;
							release_id = (stick.m_value > 128) ? pad_button::rs_left : pad_button::rs_right;
							break;
						case CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y:
							button_id = (stick.m_value <= 128) ? pad_button::rs_up : pad_button::rs_down;
							release_id = (stick.m_value > 128) ? pad_button::rs_up : pad_button::rs_down;
							break;
						default:
							break;
						}

						// Release other direction on the same axis first
						handle_button_press(release_id, false, pad_index);

						// Handle currently pressed stick direction
						handle_button_press(button_id, pressed, pad_index);

						if (exit)
							break;
					}
				}

				m_keyboard_pad_handler_active = keyboard_pad_handler_active;

				refresh();
			}

			// Disable pad interception since this user interface has to be interactive.
			// Non-interactive user intefaces handle this in close in order to prevent a g_pad_mutex deadlock.
			if (m_stop_pad_interception)
			{
				input::SetIntercepted(false);
			}

			m_interactive = false;

			return 0;
		}

		void user_interface::close(bool use_callback, bool stop_pad_interception)
		{
			// Force unload
			m_stop_pad_interception.release(stop_pad_interception);
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

			// Only disable pad interception if this user interface is not interactive.
			// Interactive user interfaces handle this in run_input_loop in order to prevent a g_pad_mutex deadlock.
			if (!m_interactive && m_stop_pad_interception)
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
			if (auto rsxthr = rsx::get_current_renderer(); rsxthr &&
				(min_refresh_duration_us + rsxthr->last_host_flip_timestamp) < rsx::uclock())
			{
				rsxthr->async_flip_requested |= rsx::thread::flip_request::native_ui;
			}
		}
	} // namespace overlays
} // namespace rsx
