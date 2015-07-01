#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceCommonDialog.h"

s32 sceCommonDialogUpdate(vm::cptr<SceCommonDialogUpdateParam> updateParam)
{
	throw EXCEPTION("");
}

s32 sceMsgDialogInit(vm::cptr<SceMsgDialogParam> param)
{
	throw EXCEPTION("");
}

SceCommonDialogStatus sceMsgDialogGetStatus()
{
	throw EXCEPTION("");
}

s32 sceMsgDialogAbort()
{
	throw EXCEPTION("");
}

s32 sceMsgDialogGetResult(vm::ptr<SceMsgDialogResult> result)
{
	throw EXCEPTION("");
}

s32 sceMsgDialogTerm()
{
	throw EXCEPTION("");
}

s32 sceMsgDialogClose()
{
	throw EXCEPTION("");
}

s32 sceMsgDialogProgressBarInc(s32 target, u32 delta)
{
	throw EXCEPTION("");
}

s32 sceMsgDialogProgressBarSetValue(s32 target, u32 rate)
{
	throw EXCEPTION("");
}

s32 sceNetCheckDialogInit(vm::ptr<SceNetCheckDialogParam> param)
{
	throw EXCEPTION("");
}

SceCommonDialogStatus sceNetCheckDialogGetStatus()
{
	throw EXCEPTION("");
}

s32 sceNetCheckDialogAbort()
{
	throw EXCEPTION("");
}

s32 sceNetCheckDialogGetResult(vm::ptr<SceNetCheckDialogResult> result)
{
	throw EXCEPTION("");
}

s32 sceNetCheckDialogTerm()
{
	throw EXCEPTION("");
}

s32 sceSaveDataDialogInit(vm::cptr<SceSaveDataDialogParam> param)
{
	throw EXCEPTION("");
}

SceCommonDialogStatus sceSaveDataDialogGetStatus()
{
	throw EXCEPTION("");
}

s32 sceSaveDataDialogAbort()
{
	throw EXCEPTION("");
}

s32 sceSaveDataDialogGetResult(vm::ptr<SceSaveDataDialogResult> result)
{
	throw EXCEPTION("");
}

s32 sceSaveDataDialogTerm()
{
	throw EXCEPTION("");
}

SceCommonDialogStatus sceSaveDataDialogGetSubStatus()
{
	throw EXCEPTION("");
}

s32 sceSaveDataDialogSubClose()
{
	throw EXCEPTION("");
}

s32 sceSaveDataDialogContinue(vm::cptr<SceSaveDataDialogParam> param)
{
	throw EXCEPTION("");
}

s32 sceSaveDataDialogFinish(vm::cptr<SceSaveDataDialogFinishParam> param)
{
	throw EXCEPTION("");
}

s32 sceSaveDataDialogProgressBarInc(s32 target, u32 delta)
{
	throw EXCEPTION("");
}

s32 sceSaveDataDialogProgressBarSetValue(s32 target, u32 rate)
{
	throw EXCEPTION("");
}

s32 sceImeDialogInit(vm::cptr<SceImeDialogParam> param)
{
	throw EXCEPTION("");
}

SceCommonDialogStatus sceImeDialogGetStatus()
{
	throw EXCEPTION("");
}

s32 sceImeDialogAbort()
{
	throw EXCEPTION("");
}

s32 sceImeDialogGetResult(vm::ptr<SceImeDialogResult> result)
{
	throw EXCEPTION("");
}

s32 sceImeDialogTerm()
{
	throw EXCEPTION("");
}

s32 scePhotoImportDialogInit(vm::cptr<ScePhotoImportDialogParam> param)
{
	throw EXCEPTION("");
}

SceCommonDialogStatus scePhotoImportDialogGetStatus()
{
	throw EXCEPTION("");
}

s32 scePhotoImportDialogGetResult(vm::ptr<ScePhotoImportDialogResult> result)
{
	throw EXCEPTION("");
}

s32 scePhotoImportDialogTerm()
{
	throw EXCEPTION("");
}

s32 scePhotoImportDialogAbort()
{
	throw EXCEPTION("");
}

s32 scePhotoReviewDialogInit(vm::cptr<ScePhotoReviewDialogParam> param)
{
	throw EXCEPTION("");
}

SceCommonDialogStatus scePhotoReviewDialogGetStatus()
{
	throw EXCEPTION("");
}

s32 scePhotoReviewDialogGetResult(vm::ptr<ScePhotoReviewDialogResult> result)
{
	throw EXCEPTION("");
}

s32 scePhotoReviewDialogTerm()
{
	throw EXCEPTION("");
}

s32 scePhotoReviewDialogAbort()
{
	throw EXCEPTION("");
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
