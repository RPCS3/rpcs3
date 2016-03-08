#pragma once

namespace vm { using namespace ps3; }

enum CELL_PAD_ERROR_CODE
{
	CELL_PAD_ERROR_FATAL = 0x80121101,
	CELL_PAD_ERROR_INVALID_PARAMETER = 0x80121102,
	CELL_PAD_ERROR_ALREADY_INITIALIZED = 0x80121103,
	CELL_PAD_ERROR_UNINITIALIZED = 0x80121104,
	CELL_PAD_ERROR_RESOURCE_ALLOCATION_FAILED = 0x80121105,
	CELL_PAD_ERROR_DATA_READ_FAILED = 0x80121106,
	CELL_PAD_ERROR_NO_DEVICE = 0x80121107,
	CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD = 0x80121108,
	CELL_PAD_ERROR_TOO_MANY_DEVICES = 0x80121109,
	CELL_PAD_ERROR_EBUSY = 0x8012110a,
};

// Controller types
enum
{
	CELL_PAD_PCLASS_TYPE_STANDARD   = 0x00,
	CELL_PAD_PCLASS_TYPE_GUITAR     = 0x01,
	CELL_PAD_PCLASS_TYPE_DRUM       = 0x02,
	CELL_PAD_PCLASS_TYPE_DJ         = 0x03,
	CELL_PAD_PCLASS_TYPE_DANCEMAT   = 0x04,
	CELL_PAD_PCLASS_TYPE_NAVIGATION = 0x05,
};

struct CellPadData
{
	be_t<s32> len;
	be_t<u16> button[CELL_PAD_MAX_CODES];
};

struct CellPadInfo
{
	be_t<u32> max_connect;
	be_t<u32> now_connect;
	be_t<u32> system_info;
	be_t<u16> vendor_id[CELL_MAX_PADS];
	be_t<u16> product_id[CELL_MAX_PADS];
	u8 status[CELL_MAX_PADS];
};

struct CellPadInfo2
{
	be_t<u32> max_connect;
	be_t<u32> now_connect;
	be_t<u32> system_info;
	be_t<u32> port_status[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> port_setting[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> device_capability[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> device_type[CELL_PAD_MAX_PORT_NUM];
};

struct CellPadPeriphInfo
{
	be_t<u32> max_connect;
	be_t<u32> now_connect;
	be_t<u32> system_info;
	be_t<u32> port_status[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> port_setting[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> device_capability[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> device_type[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> pclass_type[CELL_PAD_MAX_PORT_NUM];
	be_t<u32> pclass_profile[CELL_PAD_MAX_PORT_NUM];
};

struct CellCapabilityInfo
{
	be_t<u32> info[CELL_PAD_MAX_CAPABILITY_INFO];
};
