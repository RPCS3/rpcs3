#include "stdafx.h"

#ifdef HAVE_SDL3

#include "Utilities/File.h"
#include "LogitechG27.h"
#include "LogitechG27Config.h"

emulated_logitech_g27_config g_cfg_logitech_g27;

LOG_CHANNEL(cfg_log, "CFG");

void emulated_logitech_g27_config::fill_defaults(){
	// a shifter-less g29 with a xbox 360 controller shifter place holder...
	m_mutex.lock();

	#define INIT_AXIS_MAPPING(name, device_type_id, id, reverse) \
	{ \
		name##_device_type_id.set(device_type_id); \
		name##_type.set(MAPPING_AXIS); \
		name##_id.set(id); \
		name##_hat.set(HAT_NONE); \
		name##_reverse.set(reverse); \
	}

	INIT_AXIS_MAPPING(steering, 0x046dc24f, 0, false);
	INIT_AXIS_MAPPING(throttle, 0x046dc24f, 2, false);
	INIT_AXIS_MAPPING(brake, 0x046dc24f, 3, false);
	INIT_AXIS_MAPPING(clutch, 0x046dc24f, 1, false);

	#undef INIT_AXIS_MAPPING

	#define INIT_BUTTON_MAPPING(name, device_type_id, id, reverse) \
	{ \
		name##_device_type_id.set(device_type_id); \
		name##_type.set(MAPPING_BUTTON); \
		name##_id.set(id); \
		name##_hat.set(HAT_NONE); \
		name##_reverse.set(reverse); \
	}

	INIT_BUTTON_MAPPING(shift_up, 0x046dc24f, 4, false);
	INIT_BUTTON_MAPPING(shift_down, 0x046dc24f, 5, false);

	INIT_BUTTON_MAPPING(triangle, 0x046dc24f, 3, false);
	INIT_BUTTON_MAPPING(cross, 0x046dc24f, 0, false);
	INIT_BUTTON_MAPPING(square, 0x046dc24f, 1, false);
	INIT_BUTTON_MAPPING(circle, 0x046dc24f, 2, false);

	INIT_BUTTON_MAPPING(l2, 0x046dc24f, 7, false);
	INIT_BUTTON_MAPPING(l3, 0x046dc24f, 11, false);
	INIT_BUTTON_MAPPING(r2, 0x046dc24f, 6, false);
	INIT_BUTTON_MAPPING(r3, 0x046dc24f, 10, false);

	INIT_BUTTON_MAPPING(plus, 0x046dc24f, 19, false);
	INIT_BUTTON_MAPPING(minus, 0x046dc24f, 20, false);

	INIT_BUTTON_MAPPING(dial_clockwise, 0x046dc24f, 21, false);
	INIT_BUTTON_MAPPING(dial_anticlockwise, 0x046dc24f, 22, false);

	INIT_BUTTON_MAPPING(select, 0x046dc24f, 8, false);
	INIT_BUTTON_MAPPING(pause, 0x046dc24f, 9, false);

	INIT_BUTTON_MAPPING(shifter_1, 0x045e028e, 3, false);
	INIT_BUTTON_MAPPING(shifter_2, 0x045e028e, 0, false);
	INIT_BUTTON_MAPPING(shifter_3, 0x045e028e, 2, false);
	INIT_BUTTON_MAPPING(shifter_4, 0x045e028e, 1, false);

	#undef INIT_BUTTON_MAPPING

	#define INIT_HAT_MAPPING(name, device_type_id, id, hat, reverse) \
	{ \
		name##_device_type_id.set(device_type_id); \
		name##_type.set(MAPPING_HAT); \
		name##_id.set(id); \
		name##_hat.set(hat); \
		name##_reverse.set(reverse); \
	}

	INIT_HAT_MAPPING(up, 0x046dc24f, 0, HAT_UP, false);
	INIT_HAT_MAPPING(down, 0x046dc24f, 0, HAT_DOWN, false);
	INIT_HAT_MAPPING(left, 0x046dc24f, 0, HAT_LEFT, false);
	INIT_HAT_MAPPING(right, 0x046dc24f, 0, HAT_RIGHT, false);

	INIT_HAT_MAPPING(shifter_5, 0x045e028e, 0, HAT_UP, false);
	INIT_HAT_MAPPING(shifter_6, 0x045e028e, 0, HAT_DOWN, false);
	INIT_HAT_MAPPING(shifter_r, 0x045e028e, 0, HAT_LEFT, false);

	#undef INIT_HAT_MAPPING

	reverse_effects.set(true);
	ffb_device_type_id.set(0x046dc24f);
	led_device_type_id.set(0x046dc24f);

	enabled.set(false);

	m_mutex.unlock();
}

void emulated_logitech_g27_config::save(){
	m_mutex.lock();
	const std::string cfg_name = fmt::format("%s%s.yml", fs::get_config_dir(true), "LogitechG27");
	cfg_log.notice("Saving LogitechG27 config: %s", cfg_name);

	if (!fs::create_path(fs::get_parent_dir(cfg_name)))
	{
		cfg_log.fatal("Failed to create path: %s (%s)", cfg_name, fs::g_tls_error);
	}

	if (!cfg::node::save(cfg_name))
	{
		cfg_log.error("Failed to save LogitechG27 config to '%s' (error=%s)", cfg_name, fs::g_tls_error);
	}
	m_mutex.unlock();

}

bool emulated_logitech_g27_config::load()
{
	bool result = false;
	const std::string cfg_name = fmt::format("%s%s.yml", fs::get_config_dir(true), "LogitechG27");
	cfg_log.notice("Loading LogitechG27 config: %s", cfg_name);

	fill_defaults();

	m_mutex.lock();

	if (fs::file cfg_file{ cfg_name, fs::read })
	{
		if (const std::string content = cfg_file.to_string(); !content.empty())
		{
			result = from_string(content);
		}
		m_mutex.unlock();
	}
	else
	{
		m_mutex.unlock();
		save();
	}

	return result;
}

#endif
