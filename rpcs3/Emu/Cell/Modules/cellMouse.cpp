#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Io/MouseHandler.h"

#include "cellMouse.h"

error_code sys_config_start(ppu_thread& ppu);
error_code sys_config_stop(ppu_thread& ppu);

extern bool is_input_allowed();

LOG_CHANNEL(cellMouse);

template<>
void fmt_class_string<CellMouseError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_MOUSE_ERROR_FATAL);
			STR_CASE(CELL_MOUSE_ERROR_INVALID_PARAMETER);
			STR_CASE(CELL_MOUSE_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_MOUSE_ERROR_UNINITIALIZED);
			STR_CASE(CELL_MOUSE_ERROR_RESOURCE_ALLOCATION_FAILED);
			STR_CASE(CELL_MOUSE_ERROR_DATA_READ_FAILED);
			STR_CASE(CELL_MOUSE_ERROR_NO_DEVICE);
			STR_CASE(CELL_MOUSE_ERROR_SYS_SETTING_FAILED);
		}

		return unknown;
	});
}

error_code cellMouseInit(ppu_thread& ppu, u32 max_connect)
{
	cellMouse.notice("cellMouseInit(max_connect=%d)", max_connect);

	auto& handler = g_fxo->get<MouseHandlerBase>();

	auto init = handler.init.init();

	if (!init)
		return CELL_MOUSE_ERROR_ALREADY_INITIALIZED;

	if (max_connect == 0 || max_connect > CELL_MAX_MICE)
	{
		init.cancel();
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	sys_config_start(ppu);
	handler.Init(std::min(max_connect, 7u));

	return CELL_OK;
}

error_code cellMouseClearBuf(u32 port_no)
{
	cellMouse.trace("cellMouseClearBuf(port_no=%d)", port_no);

	auto& handler = g_fxo->get<MouseHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_MOUSE_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_MICE)
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	const MouseInfo& current_info = handler.GetInfo();

	if (port_no >= handler.GetMice().size() || current_info.status[port_no] != CELL_MOUSE_STATUS_CONNECTED)
	{
		return not_an_error(CELL_MOUSE_ERROR_NO_DEVICE);
	}

	handler.GetDataList(port_no).clear();
	handler.GetTabletDataList(port_no).clear();

	MouseRawData& raw_data = handler.GetRawData(port_no);
	raw_data.len = 0;

	for (int i = 0; i < CELL_MOUSE_MAX_CODES; i++)
	{
		raw_data.data[i] = 0;
	}

	return CELL_OK;
}

error_code cellMouseEnd(ppu_thread& ppu)
{
	cellMouse.notice("cellMouseEnd()");

	auto& handler = g_fxo->get<MouseHandlerBase>();

	const auto init = handler.init.reset();

	if (!init)
		return CELL_MOUSE_ERROR_UNINITIALIZED;

	// TODO
	sys_config_stop(ppu);
	return CELL_OK;
}

error_code cellMouseGetInfo(vm::ptr<CellMouseInfo> info)
{
	cellMouse.trace("cellMouseGetInfo(info=*0x%x)", info);

	auto& handler = g_fxo->get<MouseHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_MOUSE_ERROR_UNINITIALIZED;

	if (!info)
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	std::memset(info.get_ptr(), 0, info.size());

	const MouseInfo& current_info = handler.GetInfo();
	info->max_connect = current_info.max_connect;
	info->now_connect = current_info.now_connect;
	info->info = current_info.info;

	for (u32 i = 0; i < CELL_MAX_MICE; i++)
	{
		info->vendor_id[i]  = current_info.vendor_id[i];
		info->product_id[i] = current_info.product_id[i];
		info->status[i]     = current_info.status[i];
	}

	return CELL_OK;
}

error_code cellMouseInfoTabletMode(u32 port_no, vm::ptr<CellMouseInfoTablet> info)
{
	cellMouse.trace("cellMouseInfoTabletMode(port_no=%d, info=*0x%x)", port_no, info);

	auto& handler = g_fxo->get<MouseHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_MOUSE_ERROR_UNINITIALIZED;

	// only check for port_no here. Tests show that valid ports lead to ERROR_FATAL with disconnected devices regardless of info
	if (port_no >= CELL_MAX_MICE)
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	const MouseInfo& current_info = handler.GetInfo();

	if (port_no >= handler.GetMice().size() || current_info.status[port_no] != CELL_MOUSE_STATUS_CONNECTED)
	{
		return CELL_MOUSE_ERROR_FATAL;
	}

	if (!info)
	{
		return CELL_EFAULT; // we don't get CELL_MOUSE_ERROR_INVALID_PARAMETER here :thonkang:
	}

	info->is_supported = current_info.tablet_is_supported[port_no];
	info->mode = current_info.mode[port_no];

	// TODO: decr returns CELL_ENOTSUP ... How should we handle this?

	return CELL_OK;
}

error_code cellMouseGetData(u32 port_no, vm::ptr<CellMouseData> data)
{
	cellMouse.trace("cellMouseGetData(port_no=%d, data=*0x%x)", port_no, data);

	auto& handler = g_fxo->get<MouseHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_MOUSE_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_MICE || !data)
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	std::lock_guard lock(handler.mutex);

	const MouseInfo& current_info = handler.GetInfo();

	if (port_no >= handler.GetMice().size() || current_info.status[port_no] != CELL_MOUSE_STATUS_CONNECTED)
	{
		return not_an_error(CELL_MOUSE_ERROR_NO_DEVICE);
	}

	std::memset(data.get_ptr(), 0, data.size());

	// TODO: check if (current_info.mode[port_no] != CELL_MOUSE_INFO_TABLET_MOUSE_MODE) has any impact

	MouseDataList& data_list = handler.GetDataList(port_no);

	if (data_list.empty() || current_info.is_null_handler || (current_info.info & CELL_MOUSE_INFO_INTERCEPTED) || !is_input_allowed())
	{
		data_list.clear();
		return CELL_OK;
	}

	const MouseData current_data = data_list.front();
	data->update = current_data.update;
	data->buttons = current_data.buttons;
	data->x_axis = current_data.x_axis;
	data->y_axis = current_data.y_axis;
	data->wheel = current_data.wheel;
	data->tilt = current_data.tilt;

	data_list.pop_front();

	return CELL_OK;
}

error_code cellMouseGetDataList(u32 port_no, vm::ptr<CellMouseDataList> data)
{
	cellMouse.trace("cellMouseGetDataList(port_no=%d, data=0x%x)", port_no, data);

	auto& handler = g_fxo->get<MouseHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_MOUSE_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_MICE || !data)
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	std::lock_guard lock(handler.mutex);

	const MouseInfo& current_info = handler.GetInfo();

	if (port_no >= handler.GetMice().size() || current_info.status[port_no] != CELL_MOUSE_STATUS_CONNECTED)
	{
		return not_an_error(CELL_MOUSE_ERROR_NO_DEVICE);
	}

	std::memset(data.get_ptr(), 0, data.size());

	// TODO: check if (current_info.mode[port_no] != CELL_MOUSE_INFO_TABLET_MOUSE_MODE) has any impact

	MouseDataList& list = handler.GetDataList(port_no);

	if (list.empty() || current_info.is_null_handler || (current_info.info & CELL_MOUSE_INFO_INTERCEPTED) || !is_input_allowed())
	{
		list.clear();
		return CELL_OK;
	}

	data->list_num = std::min<u32>(CELL_MOUSE_MAX_DATA_LIST_NUM, static_cast<u32>(list.size()));

	int i = 0;
	for (auto it = list.begin(); it != list.end() && i < CELL_MOUSE_MAX_DATA_LIST_NUM; ++it, ++i)
	{
		data->list[i].update = it->update;
		data->list[i].buttons = it->buttons;
		data->list[i].x_axis = it->x_axis;
		data->list[i].y_axis = it->y_axis;
		data->list[i].wheel = it->wheel;
		data->list[i].tilt = it->tilt;
	}

	list.clear();

	return CELL_OK;
}

error_code cellMouseSetTabletMode(u32 port_no, u32 mode)
{
	cellMouse.warning("cellMouseSetTabletMode(port_no=%d, mode=%d)", port_no, mode);

	auto& handler = g_fxo->get<MouseHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_MOUSE_ERROR_UNINITIALIZED;

	// only check for port_no here. Tests show that valid ports lead to ERROR_FATAL with disconnected devices regardless of info
	if (port_no >= CELL_MAX_MICE)
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	MouseInfo& current_info = handler.GetInfo();

	if (port_no >= handler.GetMice().size() || current_info.status[port_no] != CELL_MOUSE_STATUS_CONNECTED)
	{
		return CELL_MOUSE_ERROR_FATAL;
	}

	if (mode != CELL_MOUSE_INFO_TABLET_MOUSE_MODE && mode != CELL_MOUSE_INFO_TABLET_TABLET_MODE)
	{
		return CELL_EINVAL; // lol... why not CELL_MOUSE_ERROR_INVALID_PARAMETER. Sony is drunk
	}

	current_info.mode[port_no] = mode;

	// TODO: decr returns CELL_ENOTSUP ... How should we handle this?

	return CELL_OK;
}

error_code cellMouseGetTabletDataList(u32 port_no, vm::ptr<CellMouseTabletDataList> data)
{
	cellMouse.warning("cellMouseGetTabletDataList(port_no=%d, data=0x%x)", port_no, data);

	auto& handler = g_fxo->get<MouseHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_MOUSE_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_MICE || !data)
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	const MouseInfo& current_info = handler.GetInfo();

	if (port_no >= handler.GetMice().size() || current_info.status[port_no] != CELL_MOUSE_STATUS_CONNECTED)
	{
		return not_an_error(CELL_MOUSE_ERROR_NO_DEVICE);
	}

	std::memset(data.get_ptr(), 0, data.size());

	// TODO: decr tests show that CELL_MOUSE_ERROR_DATA_READ_FAILED is returned when a mouse is connected
	// TODO: check if (current_info.mode[port_no] != CELL_MOUSE_INFO_TABLET_TABLET_MODE) has any impact

	MouseTabletDataList& list = handler.GetTabletDataList(port_no);

	if (list.empty() || current_info.is_null_handler || (current_info.info & CELL_MOUSE_INFO_INTERCEPTED) || !is_input_allowed())
	{
		list.clear();
		return CELL_OK;
	}

	data->list_num = std::min<u32>(CELL_MOUSE_MAX_DATA_LIST_NUM, static_cast<u32>(list.size()));

	int i = 0;
	for (auto it = list.begin(); it != list.end() && i < CELL_MOUSE_MAX_DATA_LIST_NUM; ++it, ++i)
	{
		data->list[i].len = it->len;
		it->len = 0;

		for (int k = 0; k < CELL_MOUSE_MAX_CODES; k++)
		{
			data->list[i].data[k] = it->data[k];
			it->data[k] = 0;
		}
	}

	list.clear();

	return CELL_OK;
}

error_code cellMouseGetRawData(u32 port_no, vm::ptr<CellMouseRawData> data)
{
	cellMouse.trace("cellMouseGetRawData(port_no=%d, data=*0x%x)", port_no, data);

	auto& handler = g_fxo->get<MouseHandlerBase>();

	const auto init = handler.init.access();

	if (!init)
		return CELL_MOUSE_ERROR_UNINITIALIZED;

	if (port_no >= CELL_MAX_MICE || !data)
	{
		return CELL_MOUSE_ERROR_INVALID_PARAMETER;
	}

	const MouseInfo& current_info = handler.GetInfo();

	if (port_no >= handler.GetMice().size() || current_info.status[port_no] != CELL_MOUSE_STATUS_CONNECTED)
	{
		return not_an_error(CELL_MOUSE_ERROR_NO_DEVICE);
	}

	std::memset(data.get_ptr(), 0, data.size());

	// TODO: decr tests show that CELL_MOUSE_ERROR_DATA_READ_FAILED is returned when a mouse is connected
	// TODO: check if (current_info.mode[port_no] != CELL_MOUSE_INFO_TABLET_MOUSE_MODE) has any impact

	MouseRawData& current_data = handler.GetRawData(port_no);

	if (current_info.is_null_handler || (current_info.info & CELL_MOUSE_INFO_INTERCEPTED) || !is_input_allowed())
	{
		current_data = {};
		return CELL_OK;
	}

	data->len = current_data.len;
	current_data.len = 0;

	for (int i = 0; i < CELL_MOUSE_MAX_CODES; i++)
	{
		data->data[i] = current_data.data[i];
		current_data.data[i] = 0;
	}

	return CELL_OK;
}

void cellMouse_init()
{
	REG_FUNC(sys_io, cellMouseInit);
	REG_FUNC(sys_io, cellMouseClearBuf);
	REG_FUNC(sys_io, cellMouseEnd);
	REG_FUNC(sys_io, cellMouseGetInfo);
	REG_FUNC(sys_io, cellMouseInfoTabletMode);
	REG_FUNC(sys_io, cellMouseGetData);
	REG_FUNC(sys_io, cellMouseGetDataList);
	REG_FUNC(sys_io, cellMouseSetTabletMode);
	REG_FUNC(sys_io, cellMouseGetTabletDataList);
	REG_FUNC(sys_io, cellMouseGetRawData);
}
