#pragma once

#ifdef _WIN32

#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"

#include <windows.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

class direct_input_pad_handler final : public PadHandlerBase
{
public:
	struct DirectInputDevice : public PadDevice
	{
	public:
		LPDIRECTINPUTDEVICE8 device = nullptr;
		DIJOYSTATE2 state{};
		DIDEVCAPS capabilities{};
		std::string product_name;
		std::string instance_name;
		std::string guid_and_path;
	};

	direct_input_pad_handler();
	~direct_input_pad_handler();

	bool Init() override;

	std::vector<pad_list_entry> list_devices() override;
	void init_config(cfg_pad* cfg) override;

private:
	std::vector<std::shared_ptr<DirectInputDevice>> m_devices;
	bool is_init{ false };

	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
	bool get_is_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	bool get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode) override;
	PadHandlerBase::connection update_connection(const std::shared_ptr<PadDevice>& device) override;
	std::unordered_map<u64, u16> get_button_values(const std::shared_ptr<PadDevice>& device) override;
	pad_preview_values get_preview_values(const std::unordered_map<u64, u16>& data, const pad_buttons& buttons) override;
};

#endif
