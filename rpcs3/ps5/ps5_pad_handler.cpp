#include "stdafx.h"
#include "ps5_pad_handler.h"
#include "ps5_pad_remap.h"
#include "Emu/Io/pad_config.h"

#include <algorithm>
#include <atomic>
#include <cmath>


namespace
{
	void (*g_poll_pads)(rpcs3_ps5_pad pads[rpcs3_ps5_pad_players]) = nullptr;

	// Player 1's right stick (ps5_pad_handler::right_stick)
	std::atomic<f32> g_right_x = 0.f;
	std::atomic<f32> g_right_y = 0.f;

	// L2 and R2 are analog on both; past this they also count as pressed
	constexpr float trigger_press = 0.25f;

	u16 axis_to_ps3(float value)
	{
		// -1..1 to 0..255, 128 at rest
		return static_cast<u16>(std::clamp(std::lround((value + 1.0f) * 127.5f), 0l, 255l));
	}

	u16 trigger_to_ps3(float value)
	{
		return static_cast<u16>(std::clamp(std::lround(value * 255.0f), 0l, 255l));
	}
}

ps5_pad_handler::ps5_pad_handler()
	: PadHandlerBase(pad_handler::dualsense)
{
	b_has_pressure_intensity_button = false;
}

void ps5_pad_handler::set_source(void (*poll_pads)(rpcs3_ps5_pad pads[rpcs3_ps5_pad_players]))
{
	g_poll_pads = poll_pads;
}

void ps5_pad_handler::right_stick(f32& x, f32& y)
{
	x = g_right_x.load(std::memory_order_relaxed);
	y = g_right_y.load(std::memory_order_relaxed);
}

void ps5_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// The buttons map one to one (process); nothing to configure
	cfg->from_default();
}

std::vector<pad_list_entry> ps5_pad_handler::list_devices()
{
	std::vector<pad_list_entry> devices;
	for (int i = 0; i < rpcs3_ps5_pad_players; i++)
	{
		devices.emplace_back(fmt::format("PS5 Controller %d", i + 1), false);
	}
	return devices;
}

bool ps5_pad_handler::bindPadToDevice(std::shared_ptr<Pad> pad)
{
	if (!pad || pad->m_player_id >= rpcs3_ps5_pad_players)
	{
		return false;
	}

	pad->Init
	(
		CELL_PAD_STATUS_DISCONNECTED,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR,
		CELL_PAD_DEV_TYPE_STANDARD,
		CELL_PAD_PCLASS_TYPE_STANDARD,
		0, 0, 0,
		100
	);

	// The base handler's layout (PadHandler.cpp), without key mappings: process sets them
	const std::vector<std::set<u32>> none;
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, none, CELL_PAD_CTRL_UP);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, none, CELL_PAD_CTRL_DOWN);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, none, CELL_PAD_CTRL_LEFT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, none, CELL_PAD_CTRL_RIGHT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, none, CELL_PAD_CTRL_CROSS);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, none, CELL_PAD_CTRL_SQUARE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, none, CELL_PAD_CTRL_CIRCLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, none, CELL_PAD_CTRL_TRIANGLE);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, none, CELL_PAD_CTRL_L1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, none, CELL_PAD_CTRL_L2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, none, CELL_PAD_CTRL_L3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, none, CELL_PAD_CTRL_R1);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, none, CELL_PAD_CTRL_R2);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, none, CELL_PAD_CTRL_R3);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, none, CELL_PAD_CTRL_START);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, none, CELL_PAD_CTRL_SELECT);
	pad->m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, none, CELL_PAD_CTRL_PS);

	pad->m_sticks[0] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, none, none);
	pad->m_sticks[1] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, none, none);
	pad->m_sticks[2] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, none, none);
	pad->m_sticks[3] = AnalogStick(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, none, none);

	// No motion yet: the sensors stay at rest
	pad->m_sensors[0] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_X, 0, 0, 0, DEFAULT_MOTION_X);
	pad->m_sensors[1] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_Y, 0, 0, 0, DEFAULT_MOTION_Y);
	pad->m_sensors[2] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_Z, 0, 0, 0, DEFAULT_MOTION_Z);
	pad->m_sensors[3] = AnalogSensor(CELL_PAD_BTN_OFFSET_SENSOR_G, 0, 0, 0, DEFAULT_MOTION_G);

	pad->m_vibrate_motors[0] = VibrateMotor(true);
	pad->m_vibrate_motors[1] = VibrateMotor(false);

	if (m_pads.size() <= pad->m_player_id)
	{
		m_pads.resize(pad->m_player_id + 1);
	}
	m_pads[pad->m_player_id] = std::move(pad);
	return true;
}

void ps5_pad_handler::process()
{
	if (!g_poll_pads)
	{
		return;
	}

	rpcs3_ps5_pad state[rpcs3_ps5_pad_players]{};
	g_poll_pads(state);
	g_right_x.store(state[0].connected ? state[0].right_x : 0.f, std::memory_order_relaxed);
	g_right_y.store(state[0].connected ? state[0].right_y : 0.f, std::memory_order_relaxed);

	// The pads connected, which the pad thread reports as now_connect: RPCS3's
	// native dialogs read no pad while it is 0 (overlays.cpp, run_input_loop),
	// and the save data list took no button (the Ratchet & Clank Collection,
	// on my console). Games read the pads without it
	u32 connected = 0;

	for (usz player = 0; player < m_pads.size(); player++)
	{
		const std::shared_ptr<Pad>& pad = m_pads[player];
		if (!pad)
		{
			continue;
		}
		const rpcs3_ps5_pad& in = state[player];

		if (in.connected != m_connected[player])
		{
			m_connected[player] = in.connected;
			if (in.connected)
			{
				pad->m_port_status |= CELL_PAD_STATUS_CONNECTED + CELL_PAD_STATUS_ASSIGN_CHANGES;
				input_log.success("PS5 controller %d connected", player + 1);
			}
			else
			{
				pad->m_port_status &= ~CELL_PAD_STATUS_CONNECTED;
				pad->m_port_status |= CELL_PAD_STATUS_ASSIGN_CHANGES;
				input_log.notice("PS5 controller %d disconnected", player + 1);
			}
		}
		if (!in.connected)
		{
			continue;
		}
		connected++;

		// L1 + R1 + the touch pad's click is the PS button: the console's shell
		// keeps the real one, and RPCS3's home menu (exit to the library,
		// settings) opens on it (pad_thread). The game sees none of the three
		// while they are held together
		u32 buttons = in.buttons;
		constexpr u32 home_combo = RPCS3_PS5_L1 | RPCS3_PS5_R1 | RPCS3_PS5_SELECT;
		const bool home = (buttons & home_combo) == home_combo;
		if (home)
		{
			buttons = (buttons & ~home_combo) | RPCS3_PS5_PS;
		}

		// The game's controls (ps5_pad_remap.h), from the buttons as they are:
		// each of the controller's, pressed and how far (the triggers), in the
		// map's order; then each of the PS3's from the one its map names. Not
		// player 1's while the pause menu reads the pad, nor other players'
		static constexpr u32 c_bits[ps5_pad_remap::count] = {RPCS3_PS5_CROSS, RPCS3_PS5_CIRCLE, RPCS3_PS5_SQUARE, RPCS3_PS5_TRIANGLE, RPCS3_PS5_L1, RPCS3_PS5_R1, 0, 0,
			RPCS3_PS5_L3, RPCS3_PS5_R3, RPCS3_PS5_START, RPCS3_PS5_SELECT, RPCS3_PS5_UP, RPCS3_PS5_DOWN, RPCS3_PS5_LEFT, RPCS3_PS5_RIGHT};
		const bool remapped = player == 0 && !ps5_pad_remap::held_off();
		const ps5_pad_remap::map map = remapped ? ps5_pad_remap::active() : ps5_pad_remap::identity();
		u16 source_value[ps5_pad_remap::count + 1]{};
		for (u8 i = 0; i < ps5_pad_remap::count; i++)
		{
			source_value[i] = c_bits[i] ? ((buttons & c_bits[i]) ? 255 : 0) : trigger_to_ps3(i == 6 ? in.l2 : in.r2);
		}
		const auto output_of = [](u32 offset, u32 code) -> u8
		{
			if (offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
			{
				switch (code)
				{
				case CELL_PAD_CTRL_UP: return 12;
				case CELL_PAD_CTRL_DOWN: return 13;
				case CELL_PAD_CTRL_LEFT: return 14;
				case CELL_PAD_CTRL_RIGHT: return 15;
				case CELL_PAD_CTRL_L3: return 8;
				case CELL_PAD_CTRL_R3: return 9;
				case CELL_PAD_CTRL_START: return 10;
				case CELL_PAD_CTRL_SELECT: return 11;
				default: return 0xff;
				}
			}
			switch (code)
			{
			case CELL_PAD_CTRL_CROSS: return 0;
			case CELL_PAD_CTRL_CIRCLE: return 1;
			case CELL_PAD_CTRL_SQUARE: return 2;
			case CELL_PAD_CTRL_TRIANGLE: return 3;
			case CELL_PAD_CTRL_L1: return 4;
			case CELL_PAD_CTRL_R1: return 5;
			case CELL_PAD_CTRL_L2: return 6;
			case CELL_PAD_CTRL_R2: return 7;
			default: return 0xff;
			}
		};

		for (Button& button : pad->m_buttons)
		{
			if (const u8 out = output_of(button.m_offset, button.m_outKeyCode); out < ps5_pad_remap::count && (remapped || out == 6 || out == 7))
			{
				// From its source: a trigger's pull as it is, a button full or not at all
				const u8 source = std::min<u8>(map[out], ps5_pad_remap::count);
				const u16 value = source_value[source];
				const bool analog_source = source == 6 || source == 7;
				button.m_pressed = analog_source ? value >= trigger_to_ps3(trigger_press) : value != 0;
				button.m_value = (out == 6 || out == 7) ? value : (button.m_pressed ? 255 : 0);
				continue;
			}

			u32 bit = 0;
			u16 analog = 0;
			if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
			{
				switch (button.m_outKeyCode)
				{
				case CELL_PAD_CTRL_UP: bit = RPCS3_PS5_UP; break;
				case CELL_PAD_CTRL_DOWN: bit = RPCS3_PS5_DOWN; break;
				case CELL_PAD_CTRL_LEFT: bit = RPCS3_PS5_LEFT; break;
				case CELL_PAD_CTRL_RIGHT: bit = RPCS3_PS5_RIGHT; break;
				case CELL_PAD_CTRL_L3: bit = RPCS3_PS5_L3; break;
				case CELL_PAD_CTRL_R3: bit = RPCS3_PS5_R3; break;
				case CELL_PAD_CTRL_START: bit = RPCS3_PS5_START; break;
				case CELL_PAD_CTRL_SELECT: bit = RPCS3_PS5_SELECT; break;
				case CELL_PAD_CTRL_PS: bit = RPCS3_PS5_PS; break;
				default: break;
				}
			}
			else if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
			{
				switch (button.m_outKeyCode)
				{
				case CELL_PAD_CTRL_CROSS: bit = RPCS3_PS5_CROSS; break;
				case CELL_PAD_CTRL_CIRCLE: bit = RPCS3_PS5_CIRCLE; break;
				case CELL_PAD_CTRL_SQUARE: bit = RPCS3_PS5_SQUARE; break;
				case CELL_PAD_CTRL_TRIANGLE: bit = RPCS3_PS5_TRIANGLE; break;
				case CELL_PAD_CTRL_L1: bit = RPCS3_PS5_L1; break;
				case CELL_PAD_CTRL_R1: bit = RPCS3_PS5_R1; break;
				case CELL_PAD_CTRL_L2: analog = trigger_to_ps3(in.l2); break;
				case CELL_PAD_CTRL_R2: analog = trigger_to_ps3(in.r2); break;
				default: break;
				}
			}

			if (bit)
			{
				button.m_pressed = (buttons & bit) != 0;
				button.m_value = button.m_pressed ? 255 : 0;
			}
			else
			{
				button.m_pressed = analog >= trigger_to_ps3(trigger_press);
				button.m_value = analog;
			}
		}

		// The PS3's sticks are 0..255 with up at 0: the title's y is down-positive already
		pad->m_sticks[0].m_value = axis_to_ps3(in.left_x);
		pad->m_sticks[1].m_value = axis_to_ps3(in.left_y);
		pad->m_sticks[2].m_value = axis_to_ps3(in.right_x);
		pad->m_sticks[3].m_value = axis_to_ps3(in.right_y);
	}

	connected_devices = connected;
}
