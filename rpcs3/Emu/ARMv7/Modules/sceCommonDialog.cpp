#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceGxm.h"
#include "sceAppUtil.h"
#include "sceIme.h"

extern psv_log_base sceCommonDialog;

enum SceCommonDialogStatus : s32
{
	SCE_COMMON_DIALOG_STATUS_NONE = 0,
	SCE_COMMON_DIALOG_STATUS_RUNNING = 1,
	SCE_COMMON_DIALOG_STATUS_FINISHED = 2
};

enum SceCommonDialogResult : s32
{
	SCE_COMMON_DIALOG_RESULT_OK,
	SCE_COMMON_DIALOG_RESULT_USER_CANCELED,
	SCE_COMMON_DIALOG_RESULT_ABORTED
};

struct SceCommonDialogRenderTargetInfo
{
	vm::psv::ptr<void> depthSurfaceData;
	vm::psv::ptr<void> colorSurfaceData;
	SceGxmColorSurfaceType surfaceType;
	SceGxmColorFormat colorFormat;
	u32 width;
	u32 height;
	u32 strideInPixels;
	u8 reserved[32];
};

struct SceCommonDialogUpdateParam
{
	SceCommonDialogRenderTargetInfo renderTarget;
	vm::psv::ptr<SceGxmSyncObject> displaySyncObject;
	u8 reserved[32];
};

struct SceMsgDialogUserMessageParam
{
	s32 buttonType;
	vm::psv::ptr<const char> msg;
	char reserved[32];
};

struct SceMsgDialogSystemMessageParam
{
	s32 sysMsgType;
	s32 value;
	char reserved[32];
};

struct SceMsgDialogErrorCodeParam
{
	s32 errorCode;
	char reserved[32];
};

struct SceMsgDialogProgressBarParam
{
	s32 barType;
	SceMsgDialogSystemMessageParam sysMsgParam;
	vm::psv::ptr<const char> msg;
	char reserved[32];
};

struct SceMsgDialogParam
{
	u32 sdkVersion;
	s32 mode;
	vm::psv::ptr<SceMsgDialogUserMessageParam> userMsgParam;
	vm::psv::ptr<SceMsgDialogSystemMessageParam> sysMsgParam;
	vm::psv::ptr<SceMsgDialogErrorCodeParam> errorCodeParam;
	vm::psv::ptr<SceMsgDialogProgressBarParam> progBarParam;
	u32 flag;
	char reserved[32];
};

struct SceMsgDialogResult
{
	s32 mode;
	s32 result;
	s32 buttonId;
	u8 reserved[32];
};


struct SceNetCheckDialogParam
{
	u32 sdkVersion;
	s32 mode;
	u8 reserved[128];
};

struct SceNetCheckDialogResult
{
	s32 result;
	u8 reserved[128];
};

struct SceSaveDataDialogFixedParam
{
	u32 targetSlot;
	char reserved[32];
};

struct SceSaveDataDialogListParam
{
	vm::psv::ptr<const u32> slotList;
	u32 slotListSize;
	s32 focusPos;
	u32 focusId;
	vm::psv::ptr<const char> listTitle;
	char reserved[32];
};

struct SceSaveDataDialogUserMessageParam
{
	s32 buttonType;
	vm::psv::ptr<const char> msg;
	u32 targetSlot;
	char reserved[32];
};

struct SceSaveDataDialogSystemMessageParam
{
	s32 sysMsgType;
	s32 value;
	u32 targetSlot;
	char reserved[32];
};

struct SceSaveDataDialogErrorCodeParam
{
	s32 errorCode;
	u32 targetSlot;
	char reserved[32];
};

struct SceSaveDataDialogProgressBarParam
{
	s32 barType;
	SceSaveDataDialogSystemMessageParam sysMsgParam;
	vm::psv::ptr<const char> msg;
	u32 targetSlot;
	char reserved[32];
};

struct SceSaveDataDialogSlotConfigParam
{
	vm::psv::ptr<const SceAppUtilSaveDataMountPoint> mountPoint;
	vm::psv::ptr<const char> appSubDir;
	char reserved[32];
};

struct SceSaveDataDialogParam
{
	u32 sdkVersion;
	s32 mode;
	s32 dispType;
	vm::psv::ptr<SceSaveDataDialogFixedParam> fixedParam;
	vm::psv::ptr<SceSaveDataDialogListParam> listParam;
	vm::psv::ptr<SceSaveDataDialogUserMessageParam> userMsgParam;
	vm::psv::ptr<SceSaveDataDialogSystemMessageParam> sysMsgParam;
	vm::psv::ptr<SceSaveDataDialogErrorCodeParam> errorCodeParam;
	vm::psv::ptr<SceSaveDataDialogProgressBarParam> progBarParam;
	vm::psv::ptr<SceSaveDataDialogSlotConfigParam> slotConfParam;
	u32 flag;
	vm::psv::ptr<void> userdata;
	char reserved[32];
};

struct SceSaveDataDialogFinishParam
{
	u32 flag;
	char reserved[32];
};

struct SceSaveDataDialogSlotInfo
{
	u32 isExist;
	vm::psv::ptr<SceAppUtilSaveDataSlotParam> slotParam;
	u8 reserved[32];
};

struct SceSaveDataDialogResult
{
	s32 mode;
	s32 result;
	s32 buttonId;
	u32 slotId;
	vm::psv::ptr<SceSaveDataDialogSlotInfo> slotInfo;
	vm::psv::ptr<void> userdata;
	char reserved[32];
};


struct SceImeDialogParam
{
	u32 sdkVersion;
	u32 inputMethod;
	u64 supportedLanguages;
	s32 languagesForced;
	u32 type;
	u32 option;
	vm::psv::ptr<SceImeCharFilter> filter;
	u32 dialogMode;
	u32 textBoxMode;
	vm::psv::ptr<const u16> title;
	u32 maxTextLength;
	vm::psv::ptr<u16> initialText;
	vm::psv::ptr<u16> inputTextBuffer;
	char reserved[32];
};

struct SceImeDialogResult
{
	s32 result;
	char reserved[32];
};

enum ScePhotoImportDialogFormatType : s32
{
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_UNKNOWN = 0,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_JPEG,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_PNG,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_GIF,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_BMP,
	SCE_PHOTOIMPORT_DIALOG_FORMAT_TYPE_TIFF
};

enum ScePhotoImportDialogOrientation : s32
{
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_UNKNOWN = 0,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_TOP_LEFT,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_TOP_RIGHT,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_BOTTOM_RIGHT,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_BOTTOM_LEFT,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_LEFT_TOP,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_RIGHT_TOP,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_RIGHT_BOTTOM,
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_LEFT_BOTTOM
};

struct ScePhotoImportDialogFileDataSub
{
	u32 width;
	u32 height;
	ScePhotoImportDialogFormatType format;
	ScePhotoImportDialogOrientation orientation;
	char reserved[32];
};

struct ScePhotoImportDialogFileData
{
	char fileName[1024];
	char photoTitle[256];
	char reserved[32];
};

struct ScePhotoImportDialogItemData
{
	ScePhotoImportDialogFileData fileData;
	ScePhotoImportDialogFileDataSub dataSub;
	char reserved[32];
};

struct ScePhotoImportDialogResult
{
	s32 result;
	u32 importedItemNum;
	char reserved[32];
};

struct ScePhotoImportDialogParam
{
	u32 sdkVersion;
	s32 mode;
	u32 visibleCategory;
	u32 itemCount;
	vm::psv::ptr<ScePhotoImportDialogItemData> itemData;
	char reserved[32];
};

struct ScePhotoReviewDialogParam
{
	u32 sdkVersion;
	s32 mode;
	char fileName[1024];
	vm::psv::ptr<void> workMemory;
	u32 workMemorySize;
	char reserved[32];
};

struct ScePhotoReviewDialogResult
{
	s32 result;
	char reserved[32];
};


s32 sceCommonDialogUpdate(vm::psv::ptr<const SceCommonDialogUpdateParam> updateParam)
{
	throw __FUNCTION__;
}

s32 sceMsgDialogInit(vm::psv::ptr<const SceMsgDialogParam> param)
{
	throw __FUNCTION__;
}

SceCommonDialogStatus sceMsgDialogGetStatus()
{
	throw __FUNCTION__;
}

s32 sceMsgDialogAbort()
{
	throw __FUNCTION__;
}

s32 sceMsgDialogGetResult(vm::psv::ptr<SceMsgDialogResult> result)
{
	throw __FUNCTION__;
}

s32 sceMsgDialogTerm()
{
	throw __FUNCTION__;
}

s32 sceMsgDialogClose()
{
	throw __FUNCTION__;
}

s32 sceMsgDialogProgressBarInc(s32 target, u32 delta)
{
	throw __FUNCTION__;
}

s32 sceMsgDialogProgressBarSetValue(s32 target, u32 rate)
{
	throw __FUNCTION__;
}

s32 sceNetCheckDialogInit(vm::psv::ptr<SceNetCheckDialogParam> param)
{
	throw __FUNCTION__;
}

SceCommonDialogStatus sceNetCheckDialogGetStatus()
{
	throw __FUNCTION__;
}

s32 sceNetCheckDialogAbort()
{
	throw __FUNCTION__;
}

s32 sceNetCheckDialogGetResult(vm::psv::ptr<SceNetCheckDialogResult> result)
{
	throw __FUNCTION__;
}

s32 sceNetCheckDialogTerm()
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogInit(vm::psv::ptr<const SceSaveDataDialogParam> param)
{
	throw __FUNCTION__;
}

SceCommonDialogStatus sceSaveDataDialogGetStatus()
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogAbort()
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogGetResult(vm::psv::ptr<SceSaveDataDialogResult> result)
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogTerm()
{
	throw __FUNCTION__;
}

SceCommonDialogStatus sceSaveDataDialogGetSubStatus()
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogSubClose()
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogContinue(vm::psv::ptr<const SceSaveDataDialogParam> param)
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogFinish(vm::psv::ptr<const SceSaveDataDialogFinishParam> param)
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogProgressBarInc(s32 target, u32 delta)
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogProgressBarSetValue(s32 target, u32 rate)
{
	throw __FUNCTION__;
}

s32 sceImeDialogInit(vm::psv::ptr<const SceImeDialogParam> param)
{
	throw __FUNCTION__;
}

SceCommonDialogStatus sceImeDialogGetStatus()
{
	throw __FUNCTION__;
}

s32 sceImeDialogAbort()
{
	throw __FUNCTION__;
}

s32 sceImeDialogGetResult(vm::psv::ptr<SceImeDialogResult> result)
{
	throw __FUNCTION__;
}

s32 sceImeDialogTerm()
{
	throw __FUNCTION__;
}

s32 scePhotoImportDialogInit(vm::psv::ptr<const ScePhotoImportDialogParam> param)
{
	throw __FUNCTION__;
}

SceCommonDialogStatus scePhotoImportDialogGetStatus()
{
	throw __FUNCTION__;
}

s32 scePhotoImportDialogGetResult(vm::psv::ptr<ScePhotoImportDialogResult> result)
{
	throw __FUNCTION__;
}

s32 scePhotoImportDialogTerm()
{
	throw __FUNCTION__;
}

s32 scePhotoImportDialogAbort()
{
	throw __FUNCTION__;
}

s32 scePhotoReviewDialogInit(vm::psv::ptr<const ScePhotoReviewDialogParam> param)
{
	throw __FUNCTION__;
}

SceCommonDialogStatus scePhotoReviewDialogGetStatus()
{
	throw __FUNCTION__;
}

s32 scePhotoReviewDialogGetResult(vm::psv::ptr<ScePhotoReviewDialogResult> result)
{
	throw __FUNCTION__;
}

s32 scePhotoReviewDialogTerm()
{
	throw __FUNCTION__;
}

s32 scePhotoReviewDialogAbort()
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<name>(nid, &sceCommonDialog, #name, name)

psv_log_base sceCommonDialog("SceCommonDialog", []()
{
	sceCommonDialog.on_load = nullptr;
	sceCommonDialog.on_unload = nullptr;
	sceCommonDialog.on_stop = nullptr;

	REG_FUNC(0x90530F2F, sceCommonDialogUpdate);
	REG_FUNC(0x755FF270, sceMsgDialogInit);
	REG_FUNC(0x4107019E, sceMsgDialogGetStatus);
	REG_FUNC(0xC296D396, sceMsgDialogClose);
	REG_FUNC(0x0CC66115, sceMsgDialogAbort);
	REG_FUNC(0xBB3BFC89, sceMsgDialogGetResult);
	REG_FUNC(0x81ACF695, sceMsgDialogTerm);
	REG_FUNC(0x7BE0E08B, sceMsgDialogProgressBarInc);
	REG_FUNC(0x9CDA5E0D, sceMsgDialogProgressBarSetValue);
	REG_FUNC(0xA38A4A0D, sceNetCheckDialogInit);
	REG_FUNC(0x8027292A, sceNetCheckDialogGetStatus);
	REG_FUNC(0x2D8EDF09, sceNetCheckDialogAbort);
	REG_FUNC(0xB05FCE9E, sceNetCheckDialogGetResult);
	REG_FUNC(0x8BE51C15, sceNetCheckDialogTerm);
	REG_FUNC(0xBF5248FA, sceSaveDataDialogInit);
	REG_FUNC(0x6E258046, sceSaveDataDialogGetStatus);
	REG_FUNC(0x013E7F74, sceSaveDataDialogAbort);
	REG_FUNC(0xB2FF576E, sceSaveDataDialogGetResult);
	REG_FUNC(0x2192A10A, sceSaveDataDialogTerm);
	REG_FUNC(0x19192C8B, sceSaveDataDialogContinue);
	REG_FUNC(0xBA0542CA, sceSaveDataDialogGetSubStatus);
	REG_FUNC(0x415D6068, sceSaveDataDialogSubClose);
	REG_FUNC(0x6C49924B, sceSaveDataDialogFinish);
	REG_FUNC(0xBDE00A83, sceSaveDataDialogProgressBarInc);
	REG_FUNC(0x5C322D1E, sceSaveDataDialogProgressBarSetValue);
	REG_FUNC(0x1E7043BF, sceImeDialogInit);
	REG_FUNC(0xCF0431FD, sceImeDialogGetStatus);
	REG_FUNC(0x594A220E, sceImeDialogAbort);
	REG_FUNC(0x2EB3D046, sceImeDialogGetResult);
	REG_FUNC(0x838A3AF4, sceImeDialogTerm);
	REG_FUNC(0x73EE7C9C, scePhotoImportDialogInit);
	REG_FUNC(0x032206D8, scePhotoImportDialogGetStatus);
	REG_FUNC(0xD855414C, scePhotoImportDialogGetResult);
	REG_FUNC(0x7FE5BD77, scePhotoImportDialogTerm);
	REG_FUNC(0x4B125581, scePhotoImportDialogAbort);
	REG_FUNC(0xCD990375, scePhotoReviewDialogInit);
	REG_FUNC(0xF4F600CA, scePhotoReviewDialogGetStatus);
	REG_FUNC(0xFFA35858, scePhotoReviewDialogGetResult);
	REG_FUNC(0xC700B2DF, scePhotoReviewDialogTerm);
	REG_FUNC(0x74FF2A8B, scePhotoReviewDialogAbort);
});
