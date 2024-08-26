#pragma once

#ifdef _WIN32
#include "util/types.hpp"
#include "Emu/Io/PadHandler.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <mmsystem.h>

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

class mm_joystick_handler final : public PadHandlerBase
{
	static constexpr u64 NO_BUTTON = u64{umax};

	// Unique names for the config files and our pad settings dialog
	const std::unordered_map<u64, std::string> button_list =
	{
		{ NO_BUTTON   , ""          },
		{ JOY_BUTTON1 , "Button 1"  },
		{ JOY_BUTTON2 , "Button 2"  },
		{ JOY_BUTTON3 , "Button 3"  },
		{ JOY_BUTTON4 , "Button 4"  },
		{ JOY_BUTTON5 , "Button 5"  },
		{ JOY_BUTTON6 , "Button 6"  },
		{ JOY_BUTTON7 , "Button 7"  },
		{ JOY_BUTTON8 , "Button 8"  },
		{ JOY_BUTTON9 , "Button 9"  },
		{ JOY_BUTTON10, "Button 10" },
		{ JOY_BUTTON11, "Button 11" },
		{ JOY_BUTTON12, "Button 12" },
		{ JOY_BUTTON13, "Button 13" },
		{ JOY_BUTTON14, "Button 14" },
		{ JOY_BUTTON15, "Button 15" },
		{ JOY_BUTTON16, "Button 16" },
		{ JOY_BUTTON17, "Button 17" },
		{ JOY_BUTTON18, "Button 18" },
		{ JOY_BUTTON19, "Button 19" },
		{ JOY_BUTTON20, "Button 20" },
		{ JOY_BUTTON21, "Button 21" },
		{ JOY_BUTTON22, "Button 22" },
		{ JOY_BUTTON23, "Button 23" },
		{ JOY_BUTTON24, "Button 24" },
		{ JOY_BUTTON25, "Button 25" },
		{ JOY_BUTTON26, "Button 26" },
		{ JOY_BUTTON27, "Button 27" },
		{ JOY_BUTTON28, "Button 28" },
		{ JOY_BUTTON29, "Button 29" },
		{ JOY_BUTTON30, "Button 30" },
		{ JOY_BUTTON31, "Button 31" },
		{ JOY_BUTTON32, "Button 32" },
	};

	// Unique names for the config files and our pad settings dialog
	const std::unordered_map<u64, std::string> pov_list =
	{
		{ JOY_POVFORWARD,  "POV Up"    },
		{ JOY_POVRIGHT,    "POV Right" },
		{ JOY_POVBACKWARD, "POV Down"  },
		{ JOY_POVLEFT,     "POV Left"  }
	};

	enum mmjoy_axis
	{
		joy_x_pos = 9700,
		joy_x_neg,
		joy_y_pos,
		joy_y_neg,
		joy_z_pos,
		joy_z_neg,
		joy_r_pos,
		joy_r_neg,
		joy_u_pos,
		joy_u_neg,
		joy_v_pos,
		joy_v_neg,
	};

	// Unique names for the config files and our pad settings dialog
	const std::unordered_map<u64, std::string> axis_list =
	{
		{ joy_x_pos, "X+" },
		{ joy_x_neg, "X-" },
		{ joy_y_pos, "Y+" },
		{ joy_y_neg, "Y-" },
		{ joy_z_pos, "Z+" },
		{ joy_z_neg, "Z-" },
		{ joy_r_pos, "R+" },
		{ joy_r_neg, "R-" },
		{ joy_u_pos, "U+" },
		{ joy_u_neg, "U-" },
		{ joy_v_pos, "V+" },
		{ joy_v_neg, "V-" },
	};

	struct MMJOYDevice : public PadDevice
	{
		u32 device_id{ umax };
		std::string device_name;
		JOYINFOEX device_info{};
		JOYCAPS device_caps{};
		MMRESULT device_status = JOYERR_UNPLUGGED;
		steady_clock::time_point last_update{};
	};

public:
	mm_joystick_handler();

	bool Init() override;

	std::vector<pad_list_entry> list_devices() override;
	connection get_next_button_press(const std::string& padId, const pad_callback& callback, const pad_fail_callback& fail_callback, gui_call_type call_type, const std::vector<std::string>& buttons) override;
	void init_config(cfg_pad* cfg) override;

private:
	std::unordered_map<u64, u16> GetButtonValues(const JOYINFOEX& js_info, const JOYCAPS& js_caps);
	std::shared_ptr<MMJOYDevice> get_device_by_name(const std::string& name);
	std::shared_ptr<MMJOYDevice> create_device_by_name(const std::string& name);
	bool GetMMJOYDevice(int index, MMJOYDevice* dev) const;
	void enumerate_devices();

	bool m_is_init = false;

	std::set<u64> m_blacklist;
	std::unordered_map<u64, u16> m_min_button_values;
	std::map<std::string, std::shared_ptr<MMJOYDevice>> m_devices;

	template <typename T>
	std::set<T> find_keys(const std::vector<std::string>& names) const;

	template <typename T>
	std::set<T> find_keys(const cfg::string& cfg_string) const;

	std::array<std::set<u32>, PadHandlerBase::button::button_count> get_mapped_key_codes(const std::shared_ptr<PadDevice>& device, const cfg_pad* cfg) override;
	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
};
#endif
