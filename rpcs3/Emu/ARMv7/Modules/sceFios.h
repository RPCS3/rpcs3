#pragma once

using SceFiosOpCallback = s32(vm::ptr<void> pContext, s32 op, u8 event, s32 err);
using SceFiosVprintfCallback = s32(vm::cptr<char> fmt, va_list ap);
using SceFiosMemcpyCallback = vm::ptr<void>(vm::ptr<void> dst, vm::cptr<void> src, u32 len);

enum SceFiosWhence : s32
{
	SCE_FIOS_SEEK_SET = 0,
	SCE_FIOS_SEEK_CUR = 1,
	SCE_FIOS_SEEK_END = 2,
};

struct SceFiosBuffer
{
	vm::lptr<void> pPtr;
	le_t<u32> length;
};

struct SceFiosOpAttr
{
	le_t<s64> deadline;
	vm::lptr<SceFiosOpCallback> pCallback;
	vm::lptr<void> pCallbackContext;

	//le_t<s32> priority : 8;
	//le_t<u32> opflags : 24;
	le_t<u32> params; // priority, opflags

	le_t<u32> userTag;
	vm::lptr<void> userPtr;
	vm::lptr<void> pReserved;
};

struct SceFiosDirEntry
{
	le_t<s64> fileSize;
	le_t<u32> statFlags;
	le_t<u16> nameLength;
	le_t<u16> fullPathLength;
	le_t<u16> offsetToName;
	le_t<u16> reserved[3];
	char fullPath[1024];
};

struct SceFiosStat
{
	le_t<s64> fileSize;
	le_t<u64> accessDate;
	le_t<u64> modificationDate;
	le_t<u64> creationDate;
	le_t<u32> statFlags;
	le_t<u32> reserved;
	le_t<s64> uid;
	le_t<s64> gid;
	le_t<s64> dev;
	le_t<s64> ino;
	le_t<s64> mode;
};

struct SceFiosOpenParams
{
	le_t<u32> openFlags;
	le_t<u32> reserved;
	SceFiosBuffer buffer;
};

struct SceFiosTuple
{
	le_t<s64> offset;
	le_t<s64> size;
	char path[1024];
};

struct SceFiosParams
{
	//le_t<u32> initialized : 1;
	//le_t<u32> paramsSize : 14;
	//le_t<u32> pathMax : 16;
	le_t<u32> params; // initialized, paramsSize, pathMax

	le_t<u32> profiling;
	SceFiosBuffer opStorage;
	SceFiosBuffer fhStorage;
	SceFiosBuffer dhStorage;
	SceFiosBuffer chunkStorage;
	vm::lptr<SceFiosVprintfCallback> pVprintf;
	vm::lptr<SceFiosMemcpyCallback> pMemcpy;
	le_t<s32> threadPriority[2];
	le_t<s32> threadAffinity[2];
};

struct SceFiosOverlay
{
	u8 type;
	u8 order;
	u8 reserved[10];
	le_t<s32> id;
	char dst[292];
	char src[292];
};

using SceFiosIOFilterCallback = void();

struct SceFiosPsarcDearchiverContext
{
	le_t<u32> sizeOfContext;
	le_t<u32> workBufferSize;
	vm::lptr<void> pWorkBuffer;
	le_t<s32> reserved[4];
};

extern psv_log_base sceFios;
