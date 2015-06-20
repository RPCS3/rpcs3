#pragma once

struct SceAppUtilInitParam
{
	le_t<u32> workBufSize;
	char reserved[60];
};

struct SceAppUtilBootParam
{
	le_t<u32> attr;
	le_t<u32> appVersion;
	char reserved[32];
};

struct SceAppUtilSaveDataMountPoint
{
	char data[16];
};

struct SceAppUtilSaveDataSlotParam
{
	le_t<u32> status;
	char title[64];
	char subTitle[128];
	char detail[512];
	char iconPath[64];
	le_t<s32> userParam;
	le_t<u32> sizeKB;
	SceDateTime modifiedTime;
	char reserved[48];
};

struct SceAppUtilSaveDataSlotEmptyParam
{
	vm::lptr<char> title;
	vm::lptr<char> iconPath;
	vm::lptr<void> iconBuf;
	le_t<u32> iconBufSize;
	char reserved[32];
};

struct SceAppUtilSaveDataSlot
{
	le_t<u32> id;
	le_t<u32> status;
	le_t<s32> userParam;
	vm::lptr<SceAppUtilSaveDataSlotEmptyParam> emptyParam;
};

struct SceAppUtilSaveDataFile
{
	vm::lptr<const char> filePath;
	vm::lptr<void> buf;
	le_t<u32> bufSize;
	le_t<s64> offset;
	le_t<u32> mode;
	le_t<u32> progDelta;
	char reserved[32];
};

struct SceAppUtilSaveDataFileSlot
{
	le_t<u32> id;
	vm::lptr<SceAppUtilSaveDataSlotParam> slotParam;
	char reserved[32];
};

extern psv_log_base sceAppUtil;
