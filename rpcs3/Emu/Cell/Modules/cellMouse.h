#pragma once

enum CellMouseError : u32
{
	CELL_MOUSE_ERROR_FATAL                      = 0x80121201,
	CELL_MOUSE_ERROR_INVALID_PARAMETER          = 0x80121202,
	CELL_MOUSE_ERROR_ALREADY_INITIALIZED        = 0x80121203,
	CELL_MOUSE_ERROR_UNINITIALIZED              = 0x80121204,
	CELL_MOUSE_ERROR_RESOURCE_ALLOCATION_FAILED = 0x80121205,
	CELL_MOUSE_ERROR_DATA_READ_FAILED           = 0x80121206,
	CELL_MOUSE_ERROR_NO_DEVICE                  = 0x80121207,
	CELL_MOUSE_ERROR_SYS_SETTING_FAILED         = 0x80121208,
};

enum
{
	CELL_MOUSE_MAX_DATA_LIST_NUM = 8,
	CELL_MOUSE_MAX_CODES         = 64,
	CELL_MAX_MICE                = 127,
};

struct CellMouseInfo
{
	be_t<u32> max_connect;
	be_t<u32> now_connect;
	be_t<u32> info;
	be_t<u16> vendor_id[CELL_MAX_MICE];
	be_t<u16> product_id[CELL_MAX_MICE];
	u8 status[CELL_MAX_MICE];
};

struct CellMouseInfoTablet
{
	be_t<u32> is_supported;
	be_t<u32> mode;
};

struct CellMouseData
{
	u8 update;
	u8 buttons;
	s8 x_axis;
	s8 y_axis;
	s8 wheel;
	s8 tilt;
};

struct CellMouseTabletData
{
	be_t<s32> len;
	u8 data[CELL_MOUSE_MAX_CODES];
};

struct CellMouseDataList
{
	be_t<u32> list_num;
	CellMouseData list[CELL_MOUSE_MAX_DATA_LIST_NUM];
};

struct CellMouseTabletDataList
{
	be_t<u32> list_num;
	CellMouseTabletData list[CELL_MOUSE_MAX_DATA_LIST_NUM];
};

struct CellMouseRawData
{
	be_t<s32> len;
	u8 data[CELL_MOUSE_MAX_CODES];
};
