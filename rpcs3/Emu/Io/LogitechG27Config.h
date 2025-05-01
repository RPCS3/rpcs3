#pragma once

#include "Utilities/Config.h"

#include <mutex>

struct emulated_logitech_g27_config : cfg::node
{
	std::mutex m_mutex;
	bool load();
	void save();
	void fill_defaults();

#define STR(s) #s
#define MAPPING_ENTRY(name)                                                           \
	cfg::uint<0, 0xFFFFFFFF> name##_device_type_id{this, STR(name##_device_type_id)}; \
	cfg::uint<0, 0xFFFFFFFF> name##_type{this, STR(name##_type)};                     \
	cfg::uint<0, 0xFFFFFFFFFFFFFFFF> name##_id{this, STR(name##_id)};                 \
	cfg::uint<0, 0xFFFFFFFF> name##_hat{this, STR(name##_hat)};                       \
	cfg::_bool name##_reverse{this, STR(name##_reverse)};

	MAPPING_ENTRY(steering);
	MAPPING_ENTRY(throttle);
	MAPPING_ENTRY(brake);
	MAPPING_ENTRY(clutch);
	MAPPING_ENTRY(shift_up);
	MAPPING_ENTRY(shift_down);

	MAPPING_ENTRY(up);
	MAPPING_ENTRY(down);
	MAPPING_ENTRY(left);
	MAPPING_ENTRY(right);

	MAPPING_ENTRY(triangle);
	MAPPING_ENTRY(cross);
	MAPPING_ENTRY(square);
	MAPPING_ENTRY(circle);

	MAPPING_ENTRY(l2);
	MAPPING_ENTRY(l3);
	MAPPING_ENTRY(r2);
	MAPPING_ENTRY(r3);

	MAPPING_ENTRY(plus);
	MAPPING_ENTRY(minus);

	MAPPING_ENTRY(dial_clockwise);
	MAPPING_ENTRY(dial_anticlockwise);

	MAPPING_ENTRY(select);
	MAPPING_ENTRY(pause);

	MAPPING_ENTRY(shifter_1);
	MAPPING_ENTRY(shifter_2);
	MAPPING_ENTRY(shifter_3);
	MAPPING_ENTRY(shifter_4);
	MAPPING_ENTRY(shifter_5);
	MAPPING_ENTRY(shifter_6);
	MAPPING_ENTRY(shifter_r);

#undef MAPPING_ENTRY
#undef STR

	cfg::_bool reverse_effects{this, "reverse_effects"};
	cfg::uint<0, 0xFFFFFFFF> ffb_device_type_id{this, "ffb_device_type_id"};
	cfg::uint<0, 0xFFFFFFFF> led_device_type_id{this, "led_device_type_id"};

	cfg::_bool enabled{this, "enabled"};
};

extern emulated_logitech_g27_config g_cfg_logitech_g27;
