#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceCommonDialog.h"

logs::channel sceCommonDialog("sceCommonDialog");

s32 sceCommonDialogUpdate(vm::cptr<SceCommonDialogUpdateParam> updateParam)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMsgDialogInit(vm::cptr<SceMsgDialogParam> param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

SceCommonDialogStatus sceMsgDialogGetStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMsgDialogAbort()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMsgDialogGetResult(vm::ptr<SceMsgDialogResult> result)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMsgDialogTerm()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMsgDialogClose()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMsgDialogProgressBarInc(s32 target, u32 delta)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceMsgDialogProgressBarSetValue(s32 target, u32 rate)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCheckDialogInit(vm::ptr<SceNetCheckDialogParam> param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

SceCommonDialogStatus sceNetCheckDialogGetStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCheckDialogAbort()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCheckDialogGetResult(vm::ptr<SceNetCheckDialogResult> result)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceNetCheckDialogTerm()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSaveDataDialogInit(vm::cptr<SceSaveDataDialogParam> param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

SceCommonDialogStatus sceSaveDataDialogGetStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSaveDataDialogAbort()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSaveDataDialogGetResult(vm::ptr<SceSaveDataDialogResult> result)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSaveDataDialogTerm()
{
	fmt::throw_exception("Unimplemented" HERE);
}

SceCommonDialogStatus sceSaveDataDialogGetSubStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSaveDataDialogSubClose()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSaveDataDialogContinue(vm::cptr<SceSaveDataDialogParam> param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSaveDataDialogFinish(vm::cptr<SceSaveDataDialogFinishParam> param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSaveDataDialogProgressBarInc(s32 target, u32 delta)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSaveDataDialogProgressBarSetValue(s32 target, u32 rate)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceImeDialogInit(vm::cptr<SceImeDialogParam> param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

SceCommonDialogStatus sceImeDialogGetStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceImeDialogAbort()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceImeDialogGetResult(vm::ptr<SceImeDialogResult> result)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceImeDialogTerm()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 scePhotoImportDialogInit(vm::cptr<ScePhotoImportDialogParam> param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

SceCommonDialogStatus scePhotoImportDialogGetStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 scePhotoImportDialogGetResult(vm::ptr<ScePhotoImportDialogResult> result)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 scePhotoImportDialogTerm()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 scePhotoImportDialogAbort()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 scePhotoReviewDialogInit(vm::cptr<ScePhotoReviewDialogParam> param)
{
	fmt::throw_exception("Unimplemented" HERE);
}

SceCommonDialogStatus scePhotoReviewDialogGetStatus()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 scePhotoReviewDialogGetResult(vm::ptr<ScePhotoReviewDialogResult> result)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 scePhotoReviewDialogTerm()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 scePhotoReviewDialogAbort()
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceCommonDialog, nid, name)

DECLARE(arm_module_manager::SceCommonDialog)("SceCommonDialog", []()
{
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
