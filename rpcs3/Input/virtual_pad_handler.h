#pragma once

#include "Emu/Io/PadHandler.h"

class virtual_pad_handler final : public PadHandlerBase
{
	using on_connect_cb = std::function<bool(const std::shared_ptr<Pad> &)>;
	static on_connect_cb mOnConnect;

public:
	virtual_pad_handler();

	static void set_on_connect_cb(on_connect_cb cb)
	{
		mOnConnect = std::move(cb);
	}

	bool Init() override
	{
		return true;
	}

	void init_config(cfg_pad* cfg) override
	{
		if (!cfg)
			return;

		cfg->from_default();
	}

	std::vector<pad_list_entry> list_devices() override;

	connection get_next_button_press(const std::string& /*padId*/, const pad_callback& /*callback*/, const pad_fail_callback& /*fail_callback*/, gui_call_type /*call_type*/, const std::vector<std::string>& /*buttons*/) override
	{
		return connection::connected;
	}

	bool bindPadToDevice(std::shared_ptr<Pad> pad) override;
	void process() override {}
};
