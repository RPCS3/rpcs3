#pragma once

struct SceAppUtilInitParam
{
	u32 workBufSize;
	char reserved[60];
};

struct SceAppUtilBootParam
{
	u32 attr;
	u32 appVersion;
	char reserved[32];
};

struct SceAppUtilSaveDataMountPoint
{
	char data[16];
};

struct SceAppUtilSaveDataSlotParam
{
	u32 status;
	char title[64];
	char subTitle[128];
	char detail[512];
	char iconPath[64];
	s32 userParam;
	u32 sizeKB;
	SceDateTime modifiedTime;
	char reserved[48];
};

struct SceAppUtilSaveDataSlotEmptyParam
{
	vm::psv::ptr<char> title;
	vm::psv::ptr<char> iconPath;
	vm::psv::ptr<void> iconBuf;
	u32 iconBufSize;
	char reserved[32];
};

struct SceAppUtilSaveDataSlot
{
	u32 id;
	u32 status;
	s32 userParam;
	vm::psv::ptr<SceAppUtilSaveDataSlotEmptyParam> emptyParam;
};

struct SceAppUtilSaveDataFile
{
	vm::psv::ptr<const char> filePath;
	vm::psv::ptr<void> buf;
	u32 bufSize;
	s64 offset;
	u32 mode;
	u32 progDelta;
	char reserved[32];
};

struct SceAppUtilSaveDataFileSlot
{
	u32 id;
	vm::psv::ptr<SceAppUtilSaveDataSlotParam> slotParam;
	char reserved[32];
};

extern psv_log_base sceAppUtil;
