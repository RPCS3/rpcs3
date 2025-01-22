#include "stdafx.h"
#include "MouseHandler.h"
#include "Emu/System.h"
#include "util/serialization.hpp"

#include <algorithm>

MouseHandlerBase::MouseHandlerBase(utils::serial* ar)
{
	if (!ar)
	{
		return;
	}

	(*ar)(m_info.max_connect);

	if (m_info.max_connect)
	{
		Emu.PostponeInitCode([this]()
		{
			Init(m_info.max_connect);
			auto lk = init.init();
		});
	}
}

void MouseHandlerBase::save(utils::serial& ar)
{
	const auto inited = init.access();

	ar(inited ? m_info.max_connect : 0);
}

bool MouseHandlerBase::is_time_for_update(double elapsed_time_ms)
{
	steady_clock::time_point now = steady_clock::now();
	const double elapsed_ms = (now - last_update).count() / 1'000'000.;

	if (elapsed_ms > elapsed_time_ms)
	{
		last_update = now;
		return true;
	}
	return false;
}

void MouseHandlerBase::Button(u32 index, u8 button, bool pressed)
{
	std::lock_guard lock(mutex);

	if (index < m_mice.size())
	{
		if (m_info.status[index] != CELL_MOUSE_STATUS_CONNECTED)
		{
			return;
		}

		Mouse& mouse = m_mice[index];
		MouseDataList& datalist = GetDataList(index);

		if (datalist.size() > MOUSE_MAX_DATA_LIST_NUM)
		{
			datalist.pop_front();
		}

		if (pressed)
			mouse.buttons |= button;
		else
			mouse.buttons &= ~button;

		MouseData new_data{};
		new_data.update = CELL_MOUSE_DATA_UPDATE;
		new_data.buttons = mouse.buttons;

		datalist.push_back(std::move(new_data));
	}
}

void MouseHandlerBase::Scroll(u32 index, s8 x, s8 y)
{
	std::lock_guard lock(mutex);

	if (index < m_mice.size())
	{
		if (m_info.status[index] != CELL_MOUSE_STATUS_CONNECTED)
		{
			return;
		}

		Mouse& mouse = m_mice[index];
		MouseDataList& datalist = GetDataList(index);

		if (datalist.size() > MOUSE_MAX_DATA_LIST_NUM)
		{
			datalist.pop_front();
		}

		MouseData new_data{};
		new_data.update = CELL_MOUSE_DATA_UPDATE;
		new_data.buttons = mouse.buttons;
		new_data.wheel = y;
		new_data.tilt = x;

		datalist.push_back(std::move(new_data));
	}
}

void MouseHandlerBase::Move(u32 index, s32 x_pos_new, s32 y_pos_new, s32 x_max, s32 y_max, bool is_relative, s32 x_delta, s32 y_delta)
{
	std::lock_guard lock(mutex);

	if (index < m_mice.size())
	{
		if (m_info.status[index] != CELL_MOUSE_STATUS_CONNECTED)
		{
			return;
		}

		Mouse& mouse = m_mice[index];
		MouseDataList& datalist = GetDataList(index);

		if (datalist.size() > MOUSE_MAX_DATA_LIST_NUM)
		{
			datalist.pop_front();
		}

		MouseData new_data{};
		new_data.update = CELL_MOUSE_DATA_UPDATE;
		new_data.buttons = mouse.buttons;

		if (!is_relative)
		{
			// The PS3 expects relative mouse movement, so we have to calculate it with the last absolute position.
			x_delta = x_pos_new - mouse.x_pos;
			y_delta = y_pos_new - mouse.y_pos;
		}

		new_data.x_axis = static_cast<s8>(std::clamp(x_delta, -127, 128));
		new_data.y_axis = static_cast<s8>(std::clamp(y_delta, -127, 128));

		mouse.x_max = x_max;
		mouse.y_max = y_max;
		mouse.x_pos = x_pos_new;
		mouse.y_pos = y_pos_new;

		//CellMouseRawData& rawdata = GetRawData(p);
		//rawdata.data[rawdata.len % CELL_MOUSE_MAX_CODES] = 0; // (TODO)
		//rawdata.len++;

		datalist.push_back(std::move(new_data));
	}
}

void MouseHandlerBase::SetIntercepted(bool intercepted)
{
	std::lock_guard lock(mutex);

	m_info.info = intercepted ? CELL_MOUSE_INFO_INTERCEPTED : 0;

	if (intercepted)
	{
		for (Mouse& mouse : m_mice)
		{
			mouse = {};
		}
	}
}
