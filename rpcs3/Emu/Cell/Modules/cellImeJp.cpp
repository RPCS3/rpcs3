#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellImeJp.h"

LOG_CHANNEL(cellImeJp);

template <>
void fmt_class_string<CellImeJpError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_IMEJP_ERROR_ERR);
			STR_CASE(CELL_IMEJP_ERROR_CONTEXT);
			STR_CASE(CELL_IMEJP_ERROR_ALREADY_OPEN);
			STR_CASE(CELL_IMEJP_ERROR_DIC_OPEN);
			STR_CASE(CELL_IMEJP_ERROR_PARAM);
			STR_CASE(CELL_IMEJP_ERROR_IME_ALREADY_IN_USE);
			STR_CASE(CELL_IMEJP_ERROR_OTHER);
		}

		return unknown;
	});
}

static uint16_t s_ime_string[256];

error_code cellImeJpOpen()
{
	std::memset(s_ime_string, 0, sizeof(s_ime_string));
	cellImeJp.error("cellImeJpOpen()");
	return CELL_OK;
}

error_code cellImeJpOpen2()
{
	std::memset(s_ime_string, 0, sizeof(s_ime_string));
	cellImeJp.error("cellImeJpOpen2()");
	return CELL_OK;
}

error_code cellImeJpOpen3()
{
	std::memset(s_ime_string, 0, sizeof(s_ime_string));
	cellImeJp.error("cellImeJpOpen3()");
	return CELL_OK;
}

error_code cellImeJpOpenExt()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpClose()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpSetKanaInputMode()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpSetInputCharType()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpSetFixInputMode()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpAllowExtensionCharacters()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpReset()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpGetStatus(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pInputStatus)
{
	cellImeJp.error("cellImeJpGetStatus()");
	*pInputStatus = CELL_IMEJP_CANDIDATE_EMPTY;
	return CELL_OK;
}

error_code cellImeJpEnterChar(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<u16> pOutputStatus)
{
	LOG_TODO(HLE, "cellImeJpEnterChar hImeJpHandle / inputChar / pOutputStatus (%d / 0x%x / %d)" HERE, hImeJpHandle, inputChar, pOutputStatus);
	*s_ime_string = inputChar;
	*pOutputStatus = CELL_IMEJP_RET_CONFIRMED;
	return CELL_OK;
}

error_code cellImeJpEnterCharExt(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<u16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterCharExt()");
	return cellImeJpEnterChar(hImeJpHandle, inputChar, pOutputStatus);
}

error_code cellImeJpEnterString(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<u16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterString()");
	return cellImeJpEnterChar(hImeJpHandle, inputChar, pOutputStatus);
}

error_code cellImeJpEnterStringExt(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<u16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterStringExt()");
	return cellImeJpEnterChar(hImeJpHandle, inputChar, pOutputStatus);
}

error_code cellImeJpModeCaretRight()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpModeCaretLeft()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpBackspaceWord()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpDeleteWord()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpAllDeleteConvertString()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpConvertForward()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpConvertBackward()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpCurrentPartConfirm()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpAllConfirm()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpConvertCancel()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpAllConvertCancel()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpExtendConvertArea()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpShortenConvertArea()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpTemporalConfirm()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpPostConvert()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpMoveFocusClause()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpGetFocusTop()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpGetFocusLength(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pFocusLength)
{
	cellImeJp.error("cellImeJpGetFocusLength()");
	*pFocusLength = 1;
	return CELL_OK;
}

error_code cellImeJpGetConfirmYomiString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pYomiString)
{
	cellImeJp.error("cellImeJpGetConfirmYomiString()");
	*pYomiString = *s_ime_string;
	return CELL_OK;
}

error_code cellImeJpGetConfirmString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pConfirmString)
{
	cellImeJp.error("cellImeJpGetConfirmString()");
	*pConfirmString = *s_ime_string;
	return CELL_OK;
}

error_code cellImeJpGetConvertYomiString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pYomiString)
{
	cellImeJp.error("cellImeJpGetConvertYomiString()");
	*pYomiString = *s_ime_string;
	return CELL_OK;
}

error_code cellImeJpGetConvertString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pConvertString)
{
	cellImeJp.error("cellImeJpGetConvertString()");
	*pConvertString = *s_ime_string;
	return CELL_OK;
}

error_code cellImeJpGetCandidateListSize()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpGetCandidateList()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpGetCandidateSelect()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpGetPredictList()
{
	UNIMPLEMENTED_FUNC(cellImeJp);
	return CELL_OK;
}

error_code cellImeJpConfirmPrediction()
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
