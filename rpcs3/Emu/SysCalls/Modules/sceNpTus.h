#pragma once

// Constants for TUS functions and structures
enum
{
	SCE_NP_TUS_DATA_INFO_MAX_SIZE = 384,
	SCE_NP_TUS_MAX_CTX_NUM = 32,
	SCE_NP_TUS_MAX_SLOT_NUM_PER_TRANS = 64,
	SCE_NP_TUS_MAX_USER_NUM_PER_TRANS = 101,
};

SceNpOnlineId SceNpTusVirtualUserId;

// Structure for representing a TUS variable
struct SceNpTusVariable
{
	SceNpId ownerId;
	be_t<s32> hasData;
	//u8 pad[4];
	CellRtcTick lastChangedDate;
	SceNpId lastChangedAuthorId;
	be_t<s64> variable;
	be_t<s64> oldVariable;
	//u8 reserved[16];
};

// Structure for representing the accessory information of a TUS data
struct SceNpTusDataInfo
{
	be_t<u32> infoSize;
	//u8 pad[4];
	u8 data[SCE_NP_TUS_DATA_INFO_MAX_SIZE];
};

// Structure for respreseting the status of TUS data
struct SceNpTusDataStatus
{
	SceNpId ownerId;
	be_t<s32> hasData;
	CellRtcTick lastChangedDate;
	SceNpId lastChangedAuthorId;
	be_t<u32> data;
	be_t<u32> dataSize;
	//u8 pad[4];
	SceNpTusDataInfo info;
};