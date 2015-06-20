#pragma once

struct ScePhotoExportParam
{
	le_t<u32> version;
	vm::lcptr<char> photoTitle;
	vm::lcptr<char> gameTitle;
	vm::lcptr<char> gameComment;
	char reserved[32];
};

using ScePhotoExportCancelFunc = func_def<s32(vm::ptr<void>)>;

extern psv_log_base scePhotoExport;
