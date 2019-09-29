#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/BEType.h"
#include <array>

enum CellPadError : u32
{
	CELL_PAD_ERROR_FATAL                      = 0x80121101,
	CELL_PAD_ERROR_INVALID_PARAMETER          = 0x80121102,
	CELL_PAD_ERROR_ALREADY_INITIALIZED        = 0x80121103,
	CELL_PAD_ERROR_UNINITIALIZED              = 0x80121104,
	CELL_PAD_ERROR_RESOURCE_ALLOCATION_FAILED = 0x80121105,
	CELL_PAD_ERROR_DATA_READ_FAILED           = 0x80121106,
	CELL_PAD_ERROR_NO_DEVICE                  = 0x80121107,
	CELL_PAD_ERROR_UNSUPPORTED_GAMEPAD        = 0x80121108,
	CELL_PAD_ERROR_TOO_MANY_DEVICES           = 0x80121109,
	CELL_PAD_ERROR_EBUSY                      = 0x8012110a,
};

enum CellPadFilterError : u32
{
	CELL_PADFILTER_ERROR_INVALID_PARAMETER = 0x80121401,
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

// Length returned in CellPadData struct
enum
{
	CELL_PAD_LEN_NO_CHANGE = 0,
	CELL_PAD_LEN_CHANGE_DEFAULT = 8,
	CELL_PAD_LEN_CHANGE_PRESS_ON = 20,
	CELL_PAD_LEN_CHANGE_SENSOR_ON = 24,
};

enum
{
	CELL_PADFILTER_IIR_CUTOFF_2ND_LPF_BT_050 = 0, // 50% Nyquist frequency
	CELL_PADFILTER_IIR_CUTOFF_2ND_LPF_BT_020 = 1, // 20% Nyquist frequency
	CELL_PADFILTER_IIR_CUTOFF_2ND_LPF_BT_010 = 2, // 10% Nyquist frequency
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

struct CellPadPeriphData
{
	be_t<u32> pclass_type;
	be_t<u32> pclass_profile;
	CellPadData cellpad_data;
};

struct CellPadCapabilityInfo
{
	be_t<u32> info[CELL_PAD_MAX_CAPABILITY_INFO];
};

struct CellPadActParam
{
	u8 motor[CELL_PAD_ACTUATOR_MAX];
	u8 reserved[6];
};

struct CellPadFilterIIRSos
{
	be_t<s32> u[3];
	be_t<s32> a1;
	be_t<s32> a2;
	be_t<s32> b0;
	be_t<s32> b1;
	be_t<s32> b2;
};

struct pad_info
{
	atomic_t<u32> max_connect = 0;
	std::array<u32, CELL_PAD_MAX_PORT_NUM> port_setting;
};
