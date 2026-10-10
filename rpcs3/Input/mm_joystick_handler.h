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
#include <set>
#include <vector>
#include <memory>
#include <unordered_map>

class mm_joystick_handler final : public PadHandlerBase
{
	static constexpr u32 NO_BUTTON = u32{umax};

	enum mmjoy_axis : u32
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

	enum mmjoy_pov : u32
	{
		joy_pov_up    = JOY_POVFORWARD,
		joy_pov_right = JOY_POVRIGHT,
		joy_pov_down  = JOY_POVBACKWARD,
		joy_pov_left  = JOY_POVLEFT,
	};

	struct MMJOYDevice : public PadDevice
	{
		u32 device_id{ umax };
		std::string device_name;
		JOYINFOEX device_info{};
		JOYCAPS device_caps{};
		MMRESULT device_status = JOYERR_UNPLUGGED;
		steady_clock::time_point last_update{};
		std::vector<std::set<u32>> trigger_code_left{};
		std::vector<std::set<u32>> trigger_code_right{};
		std::array<std::vector<std::set<u32>>, 4> axis_code_left{};
		std::array<std::vector<std::set<u32>>, 4> axis_code_right{};
	};

public:
	mm_joystick_handler();

	bool Init() override;

	std::vector<pad_list_entry> list_devices() override;
	void init_config(cfg_pad* cfg) override;

private:
	std::unordered_map<u32, u16> GetButtonValues(const JOYINFOEX& js_info, const JOYCAPS& js_caps);
	std::shared_ptr<MMJOYDevice> create_device_by_name(const std::string& name);
	bool get_device(int index, MMJOYDevice* dev) const;
	void enumerate_devices();

	bool m_is_init = false;

	std::map<std::string, std::shared_ptr<MMJOYDevice>> m_devices;

	std::array<std::vector<std::set<u32>>, PadHandlerBase::button::button_count> get_mapped_key_codes(const std::shared_ptr<PadDevice>& device, const cfg_pad* cfg) override;
	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u32 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u32 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u32 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u32 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	std::unordered_map<u32, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u32, u16>& data, const std::vector<std::string>& buttons) override;
};
#endif
