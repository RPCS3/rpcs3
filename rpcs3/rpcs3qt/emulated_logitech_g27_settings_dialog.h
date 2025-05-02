#pragma once

#include <QDialog>
#include <QLabel>
#include <QCheckBox>
#include <QScrollArea>

#ifdef HAVE_SDL3
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "SDL3/SDL.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
#endif

#include <map>
#include <vector>
#include <Emu/Io/LogitechG27Config.h>

struct joystick_state
{
	std::vector<int16_t> axes;
	std::vector<bool> buttons;
	std::vector<hat_component> hats;
};

class Mapping;
class DeviceChoice;

class emulated_logitech_g27_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	emulated_logitech_g27_settings_dialog(QWidget* parent = nullptr);
	~emulated_logitech_g27_settings_dialog();
	void set_state_text(const char*);
	const std::map<uint32_t, joystick_state>& get_joystick_states();
	void set_enable(bool enable);

private:
	std::map<uint32_t, joystick_state> m_last_joystick_states;
	// hack: need a completed dummy class when linking automoc generated with sdl-less build
	std::vector<void*> m_joystick_handles;
	uint64_t m_last_joystick_states_update = 0;
	bool m_sdl_initialized = false;

	// ui elements
	QLabel* m_state_text = nullptr;

	QCheckBox* m_enabled = nullptr;
	QCheckBox* m_reverse_effects = nullptr;

	Mapping* m_steering = nullptr;
	Mapping* m_throttle = nullptr;
	Mapping* m_brake = nullptr;
	Mapping* m_clutch = nullptr;
	Mapping* m_shift_up = nullptr;
	Mapping* m_shift_down = nullptr;

	Mapping* m_up = nullptr;
	Mapping* m_down = nullptr;
	Mapping* m_left = nullptr;
	Mapping* m_right = nullptr;

	Mapping* m_triangle = nullptr;
	Mapping* m_cross = nullptr;
	Mapping* m_square = nullptr;
	Mapping* m_circle = nullptr;

	Mapping* m_l2 = nullptr;
	Mapping* m_l3 = nullptr;
	Mapping* m_r2 = nullptr;
	Mapping* m_r3 = nullptr;

	Mapping* m_plus = nullptr;
	Mapping* m_minus = nullptr;

	Mapping* m_dial_clockwise = nullptr;
	Mapping* m_dial_anticlockwise = nullptr;

	Mapping* m_select = nullptr;
	Mapping* m_pause = nullptr;

	Mapping* m_shifter_1 = nullptr;
	Mapping* m_shifter_2 = nullptr;
	Mapping* m_shifter_3 = nullptr;
	Mapping* m_shifter_4 = nullptr;
	Mapping* m_shifter_5 = nullptr;
	Mapping* m_shifter_6 = nullptr;
	Mapping* m_shifter_r = nullptr;

	DeviceChoice* m_ffb_device = nullptr;
	DeviceChoice* m_led_device = nullptr;

	QScrollArea* m_mapping_scroll_area = nullptr;

	void load_ui_state_from_config();
	void save_ui_state_to_config();
};
