#pragma once

#include <list>

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

static const u32 MAX_MICE = 127;
static const u32 MOUSE_MAX_DATA_LIST_NUM = 8;
static const u32 MOUSE_MAX_CODES = 64;

struct MouseInfo
{
	u32 max_connect;
	u32 now_connect;
	u32 info;
	u32 mode[MAX_MICE]; // TODO: tablet support
	u32 tablet_is_supported[MAX_MICE]; // TODO: tablet support
	u16 vendor_id[MAX_MICE];
	u16 product_id[MAX_MICE];
	u8 status[MAX_MICE];
};

struct MouseRawData
{
	s32 len;
	u8 data[MOUSE_MAX_CODES];

	MouseRawData()
		: len(0)
	{
	}
};

struct MouseData
{
	u8 update;
	u8 buttons;
	s8 x_axis;
	s8 y_axis;
	s8 wheel;
	s8 tilt;  // (TODO)

	MouseData()
		: update(0)
		, buttons(0)
		, x_axis(0)
		, y_axis(0)
		, wheel(0)
		, tilt(0)
	{
	}
};

struct MouseTabletData
{
	s32 len;
	u8 data[MOUSE_MAX_CODES];

	MouseTabletData()
		: len(0)
	{
		for (auto d : data)
		{
			d = 0;
		}
	}
};

using MouseTabletDataList = std::list<MouseTabletData>;
using MouseDataList = std::list<MouseData>;

struct Mouse
{
	s16 x_pos;
	s16 y_pos;

	MouseTabletDataList m_tablet_datalist;
	MouseDataList m_datalist;
	MouseRawData m_rawdata;

	Mouse()
		: m_datalist()
		, m_rawdata()
	{
		x_pos = 0;
		y_pos = 0;
	}
};

class MouseHandlerBase
{
protected:
	MouseInfo m_info;
	std::vector<Mouse> m_mice;

public:
	virtual void Init(const u32 max_connect) = 0;
	virtual ~MouseHandlerBase() = default;

	void Button(u8 button, bool pressed)
	{
		for(u32 p=0; p < (u32)m_mice.size(); ++p)
		{
			if (m_info.status[p] != CELL_MOUSE_STATUS_CONNECTED)
			{
				continue;
			}

			MouseDataList& datalist = GetDataList(p);

			if (datalist.size() > MOUSE_MAX_DATA_LIST_NUM)
			{
				datalist.pop_front();
			}

			MouseData new_data;
			new_data.update = CELL_MOUSE_DATA_UPDATE;

			if (pressed)
				new_data.buttons |= button;
			else
				new_data.buttons &= ~button;

			datalist.push_back(new_data);
		}
	}

	void Scroll(const s8 rotation)
	{
		for(u32 p=0; p < (u32)m_mice.size(); ++p)
		{
			if (m_info.status[p] != CELL_MOUSE_STATUS_CONNECTED)
			{
				continue;
			}

			MouseDataList& datalist = GetDataList(p);

			if (datalist.size() > MOUSE_MAX_DATA_LIST_NUM)
			{
				datalist.pop_front();
			}

			MouseData new_data;
			new_data.update = CELL_MOUSE_DATA_UPDATE;
			new_data.wheel = rotation / 120; //120=event.GetWheelDelta()

			datalist.push_back(new_data);
		}
	}

	void Move(const s16 x_pos_new, const s16 y_pos_new)
	{
		for(u32 p=0; p < (u32)m_mice.size(); ++p)
		{
			if (m_info.status[p] != CELL_MOUSE_STATUS_CONNECTED)
			{
				continue;
			}

			MouseDataList& datalist = GetDataList(p);

			if (datalist.size() > MOUSE_MAX_DATA_LIST_NUM)
			{
				datalist.pop_front();
			}

			MouseData new_data;
			new_data.update = CELL_MOUSE_DATA_UPDATE;
			new_data.x_axis += x_pos_new - m_mice[p].x_pos;
			new_data.y_axis += y_pos_new - m_mice[p].y_pos;

			m_mice[p].x_pos = x_pos_new;
			m_mice[p].y_pos = y_pos_new;

			/*CellMouseRawData& rawdata = GetRawData(p);
			rawdata.data[rawdata.len % CELL_MOUSE_MAX_CODES] = 0; // (TODO)
			rawdata.len++;*/

			datalist.push_back(new_data);
		}
	}

	MouseInfo& GetInfo() { return m_info; }
	std::vector<Mouse>& GetMice() { return m_mice; }
	MouseDataList& GetDataList(const u32 mouse) { return m_mice[mouse].m_datalist; }
	MouseTabletDataList& GetTabletDataList(const u32 mouse) { return m_mice[mouse].m_tablet_datalist; }
	MouseRawData& GetRawData(const u32 mouse) { return m_mice[mouse].m_rawdata; }
};
