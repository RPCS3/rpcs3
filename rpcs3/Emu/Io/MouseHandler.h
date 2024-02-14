#pragma once

#include <list>
#include <algorithm>
#include <vector>
#include "Utilities/mutex.h"
#include "util/init_mutex.hpp"
#include "Emu/system_config_types.h"

// TODO: HLE info (constants, structs, etc.) should not be available here

enum
{
	// is_supported
	CELL_MOUSE_INFO_TABLET_NOT_SUPPORTED = 0,
	CELL_MOUSE_INFO_TABLET_SUPPORTED     = 1,

	// mode
	CELL_MOUSE_INFO_TABLET_MOUSE_MODE    = 1,
	CELL_MOUSE_INFO_TABLET_TABLET_MODE   = 2,
};

enum MousePortStatus
{
	CELL_MOUSE_STATUS_DISCONNECTED = 0x00000000,
	CELL_MOUSE_STATUS_CONNECTED    = 0x00000001,
};

enum MouseDataUpdate
{
	CELL_MOUSE_DATA_UPDATE = 1,
	CELL_MOUSE_DATA_NON    = 0,
};

enum MouseButtonCodes
{
	CELL_MOUSE_BUTTON_1 = 0x00000001,
	CELL_MOUSE_BUTTON_2 = 0x00000002,
	CELL_MOUSE_BUTTON_3 = 0x00000004,
	CELL_MOUSE_BUTTON_4 = 0x00000008,
	CELL_MOUSE_BUTTON_5 = 0x00000010,
	CELL_MOUSE_BUTTON_6 = 0x00000020,
	CELL_MOUSE_BUTTON_7 = 0x00000040,
	CELL_MOUSE_BUTTON_8 = 0x00000080,
};

enum
{
	CELL_MOUSE_INFO_INTERCEPTED = 1
};

static const u32 MAX_MICE = 127;
static const u32 MOUSE_MAX_DATA_LIST_NUM = 8;
static const u32 MOUSE_MAX_CODES = 64;

struct MouseInfo
{
	u32 max_connect = 0;
	u32 now_connect = 0;
	u32 info = 0;
	u32 mode[MAX_MICE]{}; // TODO: tablet support
	u32 tablet_is_supported[MAX_MICE]{}; // TODO: tablet support
	u16 vendor_id[MAX_MICE]{};
	u16 product_id[MAX_MICE]{};
	u8 status[MAX_MICE]{};
	bool is_null_handler = false;
};

struct MouseRawData
{
	s32 len = 0;
	u8 data[MOUSE_MAX_CODES]{};
};

struct MouseData
{
	u8 update = 0;
	u8 buttons = 0;
	s8 x_axis = 0;
	s8 y_axis = 0;
	s8 wheel = 0;
	s8 tilt = 0; // (TODO)
};

struct MouseTabletData
{
	s32 len = 0;
	u8 data[MOUSE_MAX_CODES]{};
};

using MouseTabletDataList = std::list<MouseTabletData>;
using MouseDataList = std::list<MouseData>;

struct Mouse
{
	s32 x_pos = 0;
	s32 y_pos = 0;
	s32 x_max = 0;
	s32 y_max = 0;
	u8 buttons = 0; // actual mouse button positions

	MouseTabletDataList m_tablet_datalist{};
	MouseDataList m_datalist{};
	MouseRawData m_rawdata{};
};

class MouseHandlerBase
{
protected:
	MouseInfo m_info{};
	std::vector<Mouse> m_mice;
	steady_clock::time_point last_update{};

	bool is_time_for_update(double elapsed_time = 10.0) // 4-10 ms, let's use 10 for now
	{
		steady_clock::time_point now = steady_clock::now();
		double elapsed = (now - last_update).count() / 1000'000.;

		if (elapsed > elapsed_time)
		{
			last_update = now;
			return true;
		}
		return false;
	}

public:
	shared_mutex mutex;

	virtual void Init(const u32 max_connect) = 0;
	virtual ~MouseHandlerBase() = default;

	SAVESTATE_INIT_POS(18);

	MouseHandlerBase(){};
	MouseHandlerBase(const MouseHandlerBase&) = delete;
	MouseHandlerBase(utils::serial* ar);
	MouseHandlerBase(utils::serial& ar) : MouseHandlerBase(&ar) {}
	void save(utils::serial& ar);

	void Button(u32 index, u8 button, bool pressed)
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

			MouseData new_data;
			new_data.update = CELL_MOUSE_DATA_UPDATE;
			new_data.buttons = mouse.buttons;

			datalist.push_back(new_data);
		}
	}

	void Scroll(u32 index, s32 rotation)
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

			MouseData new_data;
			new_data.update = CELL_MOUSE_DATA_UPDATE;
			new_data.wheel = std::clamp(rotation / 120, -128, 127); //120=event.GetWheelDelta()
			new_data.buttons = mouse.buttons;

			datalist.push_back(new_data);
		}
	}

	void Move(u32 index, s32 x_pos_new, s32 y_pos_new, s32 x_max, s32 y_max, const bool is_relative = false, s32 x_delta = 0, s32 y_delta = 0)
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

			MouseData new_data;
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

			/*CellMouseRawData& rawdata = GetRawData(p);
			rawdata.data[rawdata.len % CELL_MOUSE_MAX_CODES] = 0; // (TODO)
			rawdata.len++;*/

			datalist.push_back(new_data);
		}
	}

	void SetIntercepted(bool intercepted)
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

	MouseInfo& GetInfo() { return m_info; }
	std::vector<Mouse>& GetMice() { return m_mice; }
	MouseDataList& GetDataList(const u32 mouse) { return m_mice[mouse].m_datalist; }
	MouseTabletDataList& GetTabletDataList(const u32 mouse) { return m_mice[mouse].m_tablet_datalist; }
	MouseRawData& GetRawData(const u32 mouse) { return m_mice[mouse].m_rawdata; }

	stx::init_mutex init;

	mouse_handler type = mouse_handler::null;
};
