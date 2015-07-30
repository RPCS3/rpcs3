#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellImeJp;

// Return Codes
enum
{
	CELL_IMEJP_ERROR_ERR                = 0x8002bf01,
	CELL_IMEJP_ERROR_CONTEXT            = 0x8002bf11,
	CELL_IMEJP_ERROR_ALREADY_OPEN       = 0x8002bf21,
	CELL_IMEJP_ERROR_DIC_OPEN           = 0x8002bf31,
	CELL_IMEJP_ERROR_PARAM              = 0x8002bf41,
	CELL_IMEJP_ERROR_IME_ALREADY_IN_USE = 0x8002bf51,
	CELL_IMEJP_ERROR_OTHER              = 0x8002bfff,
};

s32 cellImeJpOpen()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpOpen2()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpOpen3()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpClose()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpSetKanaInputMode()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpSetInputCharType()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpSetFixInputMode()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpAllowExtensionCharacters()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpReset()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetStatus()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpEnterChar()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpEnterCharExt()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpEnterString()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpEnterStringExt()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpModeCaretRight()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpModeCaretLeft()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpBackspaceWord()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpDeleteWord()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpAllDeleteConvertString()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpConvertForward()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpConvertBackward()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpCurrentPartConfirm()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpAllConfirm()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpConvertCancel()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpAllConvertCancel()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpExtendConvertArea()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpShortenConvertArea()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpTemporalConfirm()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpPostConvert()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpMoveFocusClause()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetFocusTop()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetFocusLength()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetConfirmYomiString()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetConfirmString()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetConvertYomiString()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetConvertString()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetCandidateListSize()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetCandidateList()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetCandidateSelect()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpGetPredictList()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

s32 cellImeJpConfirmPrediction()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

Module cellImeJp("cellImeJp", []()
{
	REG_FUNC(cellImeJp, cellImeJpOpen);
	REG_FUNC(cellImeJp, cellImeJpOpen2);
	REG_FUNC(cellImeJp, cellImeJpOpen3);
	REG_FUNC(cellImeJp, cellImeJpClose);

	REG_FUNC(cellImeJp, cellImeJpSetKanaInputMode);
	REG_FUNC(cellImeJp, cellImeJpSetInputCharType);
	REG_FUNC(cellImeJp, cellImeJpSetFixInputMode);
	REG_FUNC(cellImeJp, cellImeJpAllowExtensionCharacters);
	REG_FUNC(cellImeJp, cellImeJpReset);

	REG_FUNC(cellImeJp, cellImeJpGetStatus);

	REG_FUNC(cellImeJp, cellImeJpEnterChar);
	REG_FUNC(cellImeJp, cellImeJpEnterCharExt);
	REG_FUNC(cellImeJp, cellImeJpEnterString);
	REG_FUNC(cellImeJp, cellImeJpEnterStringExt);
	REG_FUNC(cellImeJp, cellImeJpModeCaretRight);
	REG_FUNC(cellImeJp, cellImeJpModeCaretLeft);
	REG_FUNC(cellImeJp, cellImeJpBackspaceWord);
	REG_FUNC(cellImeJp, cellImeJpDeleteWord);
	REG_FUNC(cellImeJp, cellImeJpAllDeleteConvertString);
	REG_FUNC(cellImeJp, cellImeJpConvertForward);
	REG_FUNC(cellImeJp, cellImeJpConvertBackward);
	REG_FUNC(cellImeJp, cellImeJpCurrentPartConfirm);
	REG_FUNC(cellImeJp, cellImeJpAllConfirm);
	REG_FUNC(cellImeJp, cellImeJpConvertCancel);
	REG_FUNC(cellImeJp, cellImeJpAllConvertCancel);
	REG_FUNC(cellImeJp, cellImeJpExtendConvertArea);
	REG_FUNC(cellImeJp, cellImeJpShortenConvertArea);
	REG_FUNC(cellImeJp, cellImeJpTemporalConfirm);
	REG_FUNC(cellImeJp, cellImeJpPostConvert);
	REG_FUNC(cellImeJp, cellImeJpMoveFocusClause);
	REG_FUNC(cellImeJp, cellImeJpGetFocusTop);
	REG_FUNC(cellImeJp, cellImeJpGetFocusLength);
	REG_FUNC(cellImeJp, cellImeJpGetConfirmYomiString);
	REG_FUNC(cellImeJp, cellImeJpGetConfirmString);
	REG_FUNC(cellImeJp, cellImeJpGetConvertYomiString);
	REG_FUNC(cellImeJp, cellImeJpGetConvertString);
	REG_FUNC(cellImeJp, cellImeJpGetCandidateListSize);
	REG_FUNC(cellImeJp, cellImeJpGetCandidateList);
	REG_FUNC(cellImeJp, cellImeJpGetCandidateSelect);
	REG_FUNC(cellImeJp, cellImeJpGetPredictList);
	REG_FUNC(cellImeJp, cellImeJpConfirmPrediction);
});
