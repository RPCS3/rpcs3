#include "virtual_pad_handler.h"
#include "pad_thread.h"
#include "Emu/Io/pad_config.h"

virtual_pad_handler::on_connect_cb virtual_pad_handler::mOnConnect = [](auto...)
{
	return false;
};

virtual_pad_handler::virtual_pad_handler()
	: PadHandlerBase(pad_handler::virtual_pad)
{
}

std::vector<pad_list_entry> virtual_pad_handler::list_devices()
{
	std::vector<pad_list_entry> list_devices;
	for (usz i = 1; i <= MAX_GAMEPADS; i++) // Controllers 1-n in GUI
	{
		list_devices.emplace_back(fmt::format("Virtual Pad #%d", i), false);
	}
	return list_devices;
}

bool virtual_pad_handler::bindPadToDevice(std::shared_ptr<Pad> pad)
{
	if (!pad || pad->m_player_id >= g_cfg_input.player.size())
		return false;

	const cfg_player* player_config = g_cfg_input.player[pad->m_player_id];
	if (!player_config || player_config->device.to_string() != "Virtual")
		return false;

	if (!mOnConnect(pad))
	{
		return false;
	}

	m_bindings.emplace_back(std::move(pad), nullptr, nullptr);
	return true;
}
