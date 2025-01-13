#pragma once

#include <list>
#include <vector>
#include "Utilities/StrFmt.h"
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

static inline MouseButtonCodes get_mouse_button_code(int i)
{
	switch (i)
	{
	case 0:	return CELL_MOUSE_BUTTON_1;
	case 1:	return CELL_MOUSE_BUTTON_2;
	case 2:	return CELL_MOUSE_BUTTON_3;
	case 3:	return CELL_MOUSE_BUTTON_4;
	case 4:	return CELL_MOUSE_BUTTON_5;
	case 5:	return CELL_MOUSE_BUTTON_6;
	case 6:	return CELL_MOUSE_BUTTON_7;
	case 7:	return CELL_MOUSE_BUTTON_8;
	default: fmt::throw_exception("get_mouse_button_code: Invalid index %d", i);
	}
}

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
	s8 tilt = 0;
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

	bool is_time_for_update(double elapsed_time_ms = 10.0); // 4-10 ms, let's use 10 for now

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

	void Button(u32 index, u8 button, bool pressed);
	void Scroll(u32 index, s8 x, s8 y);
	void Move(u32 index, s32 x_pos_new, s32 y_pos_new, s32 x_max, s32 y_max, bool is_relative = false, s32 x_delta = 0, s32 y_delta = 0);

	void SetIntercepted(bool intercepted);

	MouseInfo& GetInfo() { return m_info; }
	std::vector<Mouse>& GetMice() { return m_mice; }
	MouseDataList& GetDataList(const u32 mouse) { return m_mice[mouse].m_datalist; }
	MouseTabletDataList& GetTabletDataList(const u32 mouse) { return m_mice[mouse].m_tablet_datalist; }
	MouseRawData& GetRawData(const u32 mouse) { return m_mice[mouse].m_rawdata; }

	stx::init_mutex init;

	mouse_handler type = mouse_handler::null;
};
