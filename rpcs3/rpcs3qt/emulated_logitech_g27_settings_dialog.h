#pragma once

#ifdef HAVE_SDL3

#include <QDialog>
#include <QLabel>
#include <QCheckBox>
#include <QScrollArea>

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "SDL3/SDL.h"
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

#include <map>
#include <vector>
#include <Emu/Io/LogitechG27Config.h>

struct joystick_state
{
	std::vector<s16> axes;
	std::vector<bool> buttons;
	std::vector<hat_component> hats;
};

class Mapping;
class DeviceChoice;

enum class mapping_device;

class emulated_logitech_g27_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	emulated_logitech_g27_settings_dialog(QWidget* parent = nullptr);
	virtual ~emulated_logitech_g27_settings_dialog();
	void set_state_text(const QString& text);
	const std::map<u64, joystick_state>& get_joystick_states();
	void set_enable(bool enable);

private:
	void load_ui_state_from_config();
	void save_ui_state_to_config();

	std::map<u64, joystick_state> m_last_joystick_states;
	std::vector<SDL_Joystick*> m_joystick_handles;
	uint64_t m_last_joystick_states_update = 0;
	bool m_sdl_initialized = false;

	// ui elements
	QLabel* m_state_text = nullptr;

	QCheckBox* m_enabled = nullptr;
	QCheckBox* m_reverse_effects = nullptr;

	std::map<mapping_device, Mapping*> m_mappings;

	DeviceChoice* m_ffb_device = nullptr;
	DeviceChoice* m_led_device = nullptr;

	QScrollArea* m_mapping_scroll_area = nullptr;
};

#endif
