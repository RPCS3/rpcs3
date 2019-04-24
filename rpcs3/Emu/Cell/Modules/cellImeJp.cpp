#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellImeJp.h"

LOG_CHANNEL(cellImeJp);

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

static uint16_t s_ime_string[256];

s32 cellImeJpOpen()
{
	std::memset(s_ime_string, 0, sizeof(s_ime_string));
	cellImeJp.error("cellImeJpOpen()");
	return CELL_OK;
}

s32 cellImeJpOpen2()
{
	std::memset(s_ime_string, 0, sizeof(s_ime_string));
	cellImeJp.error("cellImeJpOpen2()");
	return CELL_OK;
}

s32 cellImeJpOpen3()
{
	std::memset(s_ime_string, 0, sizeof(s_ime_string));
	cellImeJp.error("cellImeJpOpen3()");
	return CELL_OK;
}

s32 cellImeJpOpenExt()
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

s32 cellImeJpGetStatus(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pInputStatus)
{
	cellImeJp.error("cellImeJpGetStatus()");
	*pInputStatus = CELL_IMEJP_CANDIDATE_EMPTY;
	return CELL_OK;
}

s32 cellImeJpEnterChar(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<u16> pOutputStatus)
{
	LOG_TODO(HLE, "cellImeJpEnterChar hImeJpHandle / inputChar / pOutputStatus (%d / 0x%x / %d)" HERE, hImeJpHandle, inputChar, pOutputStatus);
	*s_ime_string = inputChar;
	*pOutputStatus = CELL_IMEJP_RET_CONFIRMED;
	return CELL_OK;
}

s32 cellImeJpEnterCharExt(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<u16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterCharExt()");
	return cellImeJpEnterChar(hImeJpHandle, inputChar, pOutputStatus);
}

s32 cellImeJpEnterString(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<u16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterString()");
	return cellImeJpEnterChar(hImeJpHandle, inputChar, pOutputStatus);
}

s32 cellImeJpEnterStringExt(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<u16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterStringExt()");
	return cellImeJpEnterChar(hImeJpHandle, inputChar, pOutputStatus);
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

s32 cellImeJpGetFocusLength(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pFocusLength)
{
	cellImeJp.error("cellImeJpGetFocusLength()");
	*pFocusLength = 1;
	return CELL_OK;
}

s32 cellImeJpGetConfirmYomiString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pYomiString)
{
	cellImeJp.error("cellImeJpGetConfirmYomiString()");
	*pYomiString = *s_ime_string;
	return CELL_OK;
}

s32 cellImeJpGetConfirmString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pConfirmString)
{
	cellImeJp.error("cellImeJpGetConfirmString()");
	*pConfirmString = *s_ime_string;
	return CELL_OK;
}

s32 cellImeJpGetConvertYomiString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pYomiString)
{
	cellImeJp.error("cellImeJpGetConvertYomiString()");
	*pYomiString = *s_ime_string;
	return CELL_OK;
}

s32 cellImeJpGetConvertString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pConvertString)
{
	cellImeJp.error("cellImeJpGetConvertString()");
	*pConvertString = *s_ime_string;
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

DECLARE(ppu_module_manager::cellImeJp)("cellImeJpUtility", []()
{
	REG_FUNC(cellImeJpUtility, cellImeJpOpen);
	REG_FUNC(cellImeJpUtility, cellImeJpOpen2);
	REG_FUNC(cellImeJpUtility, cellImeJpOpen3);
	REG_FUNC(cellImeJpUtility, cellImeJpOpenExt);
	REG_FUNC(cellImeJpUtility, cellImeJpClose);

	REG_FUNC(cellImeJpUtility, cellImeJpSetKanaInputMode);
	REG_FUNC(cellImeJpUtility, cellImeJpSetInputCharType);
	REG_FUNC(cellImeJpUtility, cellImeJpSetFixInputMode);
	REG_FUNC(cellImeJpUtility, cellImeJpAllowExtensionCharacters);
	REG_FUNC(cellImeJpUtility, cellImeJpReset);

	REG_FUNC(cellImeJpUtility, cellImeJpGetStatus);

	REG_FUNC(cellImeJpUtility, cellImeJpEnterChar);
	REG_FUNC(cellImeJpUtility, cellImeJpEnterCharExt);
	REG_FUNC(cellImeJpUtility, cellImeJpEnterString);
	REG_FUNC(cellImeJpUtility, cellImeJpEnterStringExt);
	REG_FUNC(cellImeJpUtility, cellImeJpModeCaretRight);
	REG_FUNC(cellImeJpUtility, cellImeJpModeCaretLeft);
	REG_FUNC(cellImeJpUtility, cellImeJpBackspaceWord);
	REG_FUNC(cellImeJpUtility, cellImeJpDeleteWord);
	REG_FUNC(cellImeJpUtility, cellImeJpAllDeleteConvertString);
	REG_FUNC(cellImeJpUtility, cellImeJpConvertForward);
	REG_FUNC(cellImeJpUtility, cellImeJpConvertBackward);
	REG_FUNC(cellImeJpUtility, cellImeJpCurrentPartConfirm);
	REG_FUNC(cellImeJpUtility, cellImeJpAllConfirm);
	REG_FUNC(cellImeJpUtility, cellImeJpConvertCancel);
	REG_FUNC(cellImeJpUtility, cellImeJpAllConvertCancel);
	REG_FUNC(cellImeJpUtility, cellImeJpExtendConvertArea);
	REG_FUNC(cellImeJpUtility, cellImeJpShortenConvertArea);
	REG_FUNC(cellImeJpUtility, cellImeJpTemporalConfirm);
	REG_FUNC(cellImeJpUtility, cellImeJpPostConvert);
	REG_FUNC(cellImeJpUtility, cellImeJpMoveFocusClause);
	REG_FUNC(cellImeJpUtility, cellImeJpGetFocusTop);
	REG_FUNC(cellImeJpUtility, cellImeJpGetFocusLength);
	REG_FUNC(cellImeJpUtility, cellImeJpGetConfirmYomiString);
	REG_FUNC(cellImeJpUtility, cellImeJpGetConfirmString);
	REG_FUNC(cellImeJpUtility, cellImeJpGetConvertYomiString);
	REG_FUNC(cellImeJpUtility, cellImeJpGetConvertString);
	REG_FUNC(cellImeJpUtility, cellImeJpGetCandidateListSize);
	REG_FUNC(cellImeJpUtility, cellImeJpGetCandidateList);
	REG_FUNC(cellImeJpUtility, cellImeJpGetCandidateSelect);
	REG_FUNC(cellImeJpUtility, cellImeJpGetPredictList);
	REG_FUNC(cellImeJpUtility, cellImeJpConfirmPrediction);
});
