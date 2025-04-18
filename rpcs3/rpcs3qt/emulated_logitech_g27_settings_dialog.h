#pragma once

#include <QDialog>
#include <QLabel>
#include <QCheckBox>
#include <QScrollArea>

#include <map>
#include <vector>
#include <Emu/Io/LogitechG27Config.h>

#include <SDL3/SDL.h>

struct joystick_state;

class Mapping;
class DeviceChoice;

class emulated_logitech_g27_settings_dialog : public QDialog
{
public:
	emulated_logitech_g27_settings_dialog(QWidget* parent = nullptr);
	~emulated_logitech_g27_settings_dialog();
	void disable();
	void enable();
	void set_state_text(const char *);
	const std::map<uint32_t, joystick_state> &get_joystick_states();
private:
	void toggle_state(bool enable);
	void load_ui_state_from_config();
	void save_ui_state_to_config();

	std::map<uint32_t, joystick_state> last_joystick_states;
	std::vector<SDL_Joystick *> joystick_handles;
	uint64_t last_joystick_states_update = 0;
	bool sdl_initialized = false;

	// ui elements
	QLabel *state_text;

	QCheckBox *enabled;
	QCheckBox *reverse_effects;

	Mapping *steering;
	Mapping *throttle;
	Mapping *brake;
	Mapping *clutch;
	Mapping *shift_up;
	Mapping *shift_down;

	Mapping *up;
	Mapping *down;
	Mapping *left;
	Mapping *right;

	Mapping *triangle;
	Mapping *cross;
	Mapping *square;
	Mapping *circle;

	Mapping *l2;
	Mapping *l3;
	Mapping *r2;
	Mapping *r3;

	Mapping *plus;
	Mapping *minus;

	Mapping *dial_clockwise;
	Mapping *dial_anticlockwise;

	Mapping *select;
	Mapping *pause;

	Mapping *shifter_1;
	Mapping *shifter_2;
	Mapping *shifter_3;
	Mapping *shifter_4;
	Mapping *shifter_5;
	Mapping *shifter_6;
	Mapping *shifter_r;

	DeviceChoice *ffb_device;
	DeviceChoice *led_device;

	QScrollArea *mapping_scroll_area;
};
