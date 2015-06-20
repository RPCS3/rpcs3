#pragma once

#include "sceGxm.h"
#include "sceAppUtil.h"
#include "sceIme.h"

enum SceCommonDialogStatus : s32
{
	SCE_COMMON_DIALOG_STATUS_NONE = 0,
	SCE_COMMON_DIALOG_STATUS_RUNNING = 1,
	SCE_COMMON_DIALOG_STATUS_FINISHED = 2,
};

enum SceCommonDialogResult : s32
{
	SCE_COMMON_DIALOG_RESULT_OK,
	SCE_COMMON_DIALOG_RESULT_USER_CANCELED,
	SCE_COMMON_DIALOG_RESULT_ABORTED,
};

struct SceCommonDialogRenderTargetInfo
{
	vm::lptr<void> depthSurfaceData;
	vm::lptr<void> colorSurfaceData;
	le_t<u32> surfaceType; // SceGxmColorSurfaceType
	le_t<u32> colorFormat; // SceGxmColorFormat
	le_t<u32> width;
	le_t<u32> height;
	le_t<u32> strideInPixels;
	u8 reserved[32];
};

struct SceCommonDialogUpdateParam
{
	SceCommonDialogRenderTargetInfo renderTarget;
	vm::lptr<SceGxmSyncObject> displaySyncObject;
	u8 reserved[32];
};

struct SceMsgDialogUserMessageParam
{
	le_t<s32> buttonType;
	vm::lptr<const char> msg;
	char reserved[32];
};

struct SceMsgDialogSystemMessageParam
{
	le_t<s32> sysMsgType;
	le_t<s32> value;
	char reserved[32];
};

struct SceMsgDialogErrorCodeParam
{
	le_t<s32> errorCode;
	char reserved[32];
};

struct SceMsgDialogProgressBarParam
{
	le_t<s32> barType;
	SceMsgDialogSystemMessageParam sysMsgParam;
	vm::lptr<const char> msg;
	char reserved[32];
};

struct SceMsgDialogParam
{
	le_t<u32> sdkVersion;
	le_t<s32> mode;
	vm::lptr<SceMsgDialogUserMessageParam> userMsgParam;
	vm::lptr<SceMsgDialogSystemMessageParam> sysMsgParam;
	vm::lptr<SceMsgDialogErrorCodeParam> errorCodeParam;
	vm::lptr<SceMsgDialogProgressBarParam> progBarParam;
	le_t<u32> flag;
	char reserved[32];
};

struct SceMsgDialogResult
{
	le_t<s32> mode;
	le_t<s32> result;
	le_t<s32> buttonId;
	u8 reserved[32];
};


struct SceNetCheckDialogParam
{
	le_t<u32> sdkVersion;
	le_t<s32> mode;
	u8 reserved[128];
};

struct SceNetCheckDialogResult
{
	le_t<s32> result;
	u8 reserved[128];
};

struct SceSaveDataDialogFixedParam
{
	le_t<u32> targetSlot;
	char reserved[32];
};

struct SceSaveDataDialogListParam
{
	vm::lptr<const u32> slotList;
	le_t<u32> slotListSize;
	le_t<s32> focusPos;
	le_t<u32> focusId;
	vm::lptr<const char> listTitle;
	char reserved[32];
};

struct SceSaveDataDialogUserMessageParam
{
	le_t<s32> buttonType;
	vm::lptr<const char> msg;
	le_t<u32> targetSlot;
	char reserved[32];
};

struct SceSaveDataDialogSystemMessageParam
{
	le_t<s32> sysMsgType;
	le_t<s32> value;
	le_t<u32> targetSlot;
	char reserved[32];
};

struct SceSaveDataDialogErrorCodeParam
{
	le_t<s32> errorCode;
	le_t<u32> targetSlot;
	char reserved[32];
};

struct SceSaveDataDialogProgressBarParam
{
	le_t<s32> barType;
	SceSaveDataDialogSystemMessageParam sysMsgParam;
	vm::lptr<const char> msg;
	le_t<u32> targetSlot;
	char reserved[32];
};

struct SceSaveDataDialogSlotConfigParam
{
	vm::lptr<const SceAppUtilSaveDataMountPoint> mountPoint;
	vm::lptr<const char> appSubDir;
	char reserved[32];
};

struct SceSaveDataDialogParam
{
	le_t<u32> sdkVersion;
	le_t<s32> mode;
	le_t<s32> dispType;
	vm::lptr<SceSaveDataDialogFixedParam> fixedParam;
	vm::lptr<SceSaveDataDialogListParam> listParam;
	vm::lptr<SceSaveDataDialogUserMessageParam> userMsgParam;
	vm::lptr<SceSaveDataDialogSystemMessageParam> sysMsgParam;
	vm::lptr<SceSaveDataDialogErrorCodeParam> errorCodeParam;
	vm::lptr<SceSaveDataDialogProgressBarParam> progBarParam;
	vm::lptr<SceSaveDataDialogSlotConfigParam> slotConfParam;
	le_t<u32> flag;
	vm::lptr<void> userdata;
	char reserved[32];
};

struct SceSaveDataDialogFinishParam
{
	le_t<u32> flag;
	char reserved[32];
};

struct SceSaveDataDialogSlotInfo
{
	le_t<u32> isExist;
	vm::lptr<SceAppUtilSaveDataSlotParam> slotParam;
	u8 reserved[32];
};

struct SceSaveDataDialogResult
{
	le_t<s32> mode;
	le_t<s32> result;
	le_t<s32> buttonId;
	le_t<u32> slotId;
	vm::lptr<SceSaveDataDialogSlotInfo> slotInfo;
	vm::lptr<void> userdata;
	char reserved[32];
};


struct SceImeDialogParam
{
	le_t<u32> sdkVersion;
	le_t<u32> inputMethod;
	le_t<u64> supportedLanguages;
	le_t<s32> languagesForced;
	le_t<u32> type;
	le_t<u32> option;
	vm::lptr<SceImeCharFilter> filter;
	le_t<u32> dialogMode;
	le_t<u32> textBoxMode;
	vm::lptr<const u16> title;
	le_t<u32> maxTextLength;
	vm::lptr<u16> initialText;
	vm::lptr<u16> inputTextBuffer;
	char reserved[32];
};

struct SceImeDialogResult
{
	le_t<s32> result;
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
	SCE_PHOTOIMPORT_DIALOG_ORIENTATION_LEFT_BOTTOM,
};

struct ScePhotoImportDialogFileDataSub
{
	le_t<u32> width;
	le_t<u32> height;
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
	le_t<s32> result;
	le_t<u32> importedItemNum;
	char reserved[32];
};

struct ScePhotoImportDialogParam
{
	le_t<u32> sdkVersion;
	le_t<s32> mode;
	le_t<u32> visibleCategory;
	le_t<u32> itemCount;
	vm::lptr<ScePhotoImportDialogItemData> itemData;
	char reserved[32];
};

struct ScePhotoReviewDialogParam
{
	le_t<u32> sdkVersion;
	le_t<s32> mode;
	char fileName[1024];
	vm::lptr<void> workMemory;
	le_t<u32> workMemorySize;
	char reserved[32];
};

struct ScePhotoReviewDialogResult
{
	le_t<s32> result;
	char reserved[32];
};

extern psv_log_base sceCommonDialog;
