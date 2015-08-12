#pragma once

namespace vm { using namespace ps3; }

// Error codes
enum
{
	CELL_SYSCONF_ERROR_PARAM = 0x8002bb01,
};

// Device types
enum
{
	CELL_SYSCONF_BT_DEVICE_TYPE_AUDIO = 0x00000001,
	CELL_SYSCONF_BT_DEVICE_TYPE_HID   = 0x00000002,
};

// Device states
enum
{
	CELL_SYSCONF_BT_DEVICE_STATE_UNAVAILABLE = 0,
	CELL_SYSCONF_BT_DEVICE_STATE_AVAILABLE   = 1,
};

using CellSysconfCallback = void(s32 result, vm::ptr<u32> userdata);

struct CellSysconfBtDeviceInfo
{
	be_t<u64> deviceId;
	be_t<u32> deviceType;
	be_t<u32> state;
	char name[64];
	be_t<u32> reserved[4];
};

struct CellSysconfBtDeviceList
{
	CellSysconfBtDeviceInfo device[16];
};