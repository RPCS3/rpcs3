#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceCommonDialog.h"

s32 sceCommonDialogUpdate(vm::ptr<const SceCommonDialogUpdateParam> updateParam)
{
	throw __FUNCTION__;
}

s32 sceMsgDialogInit(vm::ptr<const SceMsgDialogParam> param)
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

s32 sceMsgDialogGetResult(vm::ptr<SceMsgDialogResult> result)
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

s32 sceNetCheckDialogInit(vm::ptr<SceNetCheckDialogParam> param)
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

s32 sceNetCheckDialogGetResult(vm::ptr<SceNetCheckDialogResult> result)
{
	throw __FUNCTION__;
}

s32 sceNetCheckDialogTerm()
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogInit(vm::ptr<const SceSaveDataDialogParam> param)
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

s32 sceSaveDataDialogGetResult(vm::ptr<SceSaveDataDialogResult> result)
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

s32 sceSaveDataDialogContinue(vm::ptr<const SceSaveDataDialogParam> param)
{
	throw __FUNCTION__;
}

s32 sceSaveDataDialogFinish(vm::ptr<const SceSaveDataDialogFinishParam> param)
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

s32 sceImeDialogInit(vm::ptr<const SceImeDialogParam> param)
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

s32 sceImeDialogGetResult(vm::ptr<SceImeDialogResult> result)
{
	throw __FUNCTION__;
}

s32 sceImeDialogTerm()
{
	throw __FUNCTION__;
}

s32 scePhotoImportDialogInit(vm::ptr<const ScePhotoImportDialogParam> param)
{
	throw __FUNCTION__;
}

SceCommonDialogStatus scePhotoImportDialogGetStatus()
{
	throw __FUNCTION__;
}

s32 scePhotoImportDialogGetResult(vm::ptr<ScePhotoImportDialogResult> result)
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

s32 scePhotoReviewDialogInit(vm::ptr<const ScePhotoReviewDialogParam> param)
{
	throw __FUNCTION__;
}

SceCommonDialogStatus scePhotoReviewDialogGetStatus()
{
	throw __FUNCTION__;
}

s32 scePhotoReviewDialogGetResult(vm::ptr<ScePhotoReviewDialogResult> result)
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


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceCommonDialog, #name, name)

psv_log_base sceCommonDialog("SceCommonDialog", []()
{
	sceCommonDialog.on_load = nullptr;
	sceCommonDialog.on_unload = nullptr;
	sceCommonDialog.on_stop = nullptr;
	sceCommonDialog.on_error = nullptr;

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
