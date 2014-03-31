#pragma once

#include <vector>

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

static const u32 CELL_MAX_MICE = 127;
static const u32 CELL_MOUSE_MAX_DATA_LIST_NUM = 8;
static const u32 CELL_MOUSE_MAX_CODES = 64;

struct MouseInfo
{
	u32 max_connect;
	u32 now_connect;
	u32 info;
	u16 vendor_id[CELL_MAX_MICE];
	u16 product_id[CELL_MAX_MICE];
	u8 status[CELL_MAX_MICE];
};

struct CellMouseRawData
{
	s32 len;
	u8 data[CELL_MOUSE_MAX_CODES];

	CellMouseRawData()
		: len(0)
	{
	}
};

struct CellMouseData
{
	u8 update;
	u8 buttons;
	s8 x_axis;
	s8 y_axis;
	s8 wheel;
	s8 tilt;  // (TODO)

	CellMouseData()
		: update(0)
		, buttons(0)
		, x_axis(0)
		, y_axis(0)
		, wheel(0)
		, tilt(0)
	{
	}
};

struct CellMouseDataList
{
	u32 list_num;
	CellMouseData list[CELL_MOUSE_MAX_DATA_LIST_NUM];

	CellMouseDataList()
		: list_num(0)
	{
	}
};

struct Mouse
{
	s16 x_pos;
	s16 y_pos;

	CellMouseData m_data;
	CellMouseRawData m_rawdata;

	Mouse()
		: m_data()
		, m_rawdata()
	{
	}
};

class MouseHandlerBase
{
protected:
	MouseInfo m_info;
	std::vector<Mouse> m_mice;

public:
	virtual void Init(const u32 max_connect)=0;
	virtual void Close()=0;

	void Button(u8 button, bool pressed)
	{
		for(u64 p=0; p < m_mice.size(); ++p)
		{
			if (m_info.status[p] == CELL_MOUSE_STATUS_CONNECTED)
			{
				CellMouseData& data = GetData(p);
				data.update = CELL_MOUSE_DATA_UPDATE;
				if (pressed) data.buttons |= button;
				else data.buttons &= ~button;
			}
		}
	}

	void Scroll(const s8 rotation)
	{
		for(u64 p=0; p < m_mice.size(); ++p)
		{
			if (m_info.status[p] == CELL_MOUSE_STATUS_CONNECTED)
			{
				CellMouseData& data = GetData(p);
				data.update = CELL_MOUSE_DATA_UPDATE;
				data.wheel = rotation/120; //120=event.GetWheelDelta()
			}
		}
	}

	void Move(const s16 x_pos_new, const s16 y_pos_new)
	{
		for(u64 p=0; p< m_mice.size(); ++p)
		{
			if (m_info.status[p] == CELL_MOUSE_STATUS_CONNECTED)
			{
				CellMouseData& data = GetData(p);
				data.update = CELL_MOUSE_DATA_UPDATE;
				data.x_axis += x_pos_new - m_mice[p].x_pos;
				data.y_axis += y_pos_new - m_mice[p].y_pos;

				m_mice[p].x_pos = x_pos_new;
				m_mice[p].y_pos = y_pos_new;

				/*CellMouseRawData& rawdata = GetRawData(p);
				rawdata.data[rawdata.len % CELL_MOUSE_MAX_CODES] = 0; // (TODO)
				rawdata.len++;*/
			}
		}
	}

	MouseInfo& GetInfo() { return m_info; }
	std::vector<Mouse>& GetMice() { return m_mice; }
	CellMouseData& GetData(const u32 mouse) { return m_mice[mouse].m_data; }
	CellMouseRawData& GetRawData(const u32 mouse) { return m_mice[mouse].m_rawdata; }
};