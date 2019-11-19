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

using sys_memory_container_t = u32;

static u16 s_ime_string[256];

error_code cellImeJpOpen(sys_memory_container_t container_id, vm::ptr<CellImeJpHandle> hImeJpHandle, vm::cptr<CellImeJpAddDic> addDicPath)
{
	cellImeJp.todo("cellImeJpOpen(container_id=*0x%x, hImeJpHandle=*0x%x, addDicPath=*0x%x)", container_id, hImeJpHandle, addDicPath);
	std::memset(s_ime_string, 0, sizeof(s_ime_string));
	return CELL_OK;
}

error_code cellImeJpOpen2(sys_memory_container_t container_id, vm::ptr<CellImeJpHandle> hImeJpHandle, vm::cptr<CellImeJpAddDic> addDicPath)
{
	cellImeJp.todo("cellImeJpOpen2(container_id=*0x%x, hImeJpHandle=*0x%x, addDicPath=*0x%x)", container_id, hImeJpHandle, addDicPath);
	std::memset(s_ime_string, 0, sizeof(s_ime_string));
	return CELL_OK;
}

error_code cellImeJpOpen3(sys_memory_container_t container_id, vm::ptr<CellImeJpHandle> hImeJpHandle, vm::cpptr<CellImeJpAddDic> addDicPath)
{
	cellImeJp.todo("cellImeJpOpen3(container_id=*0x%x, hImeJpHandle=*0x%x, addDicPath=*0x%x)", container_id, hImeJpHandle, addDicPath);
	std::memset(s_ime_string, 0, sizeof(s_ime_string));
	return CELL_OK;
}

error_code cellImeJpOpenExt()
{
	cellImeJp.todo("cellImeJpOpenExt()");
	return CELL_OK;
}

error_code cellImeJpClose(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpClose(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpSetKanaInputMode(CellImeJpHandle hImeJpHandle, s16 inputOption)
{
	cellImeJp.todo("cellImeJpSetKanaInputMode(hImeJpHandle=*0x%x, inputOption=%d)", hImeJpHandle, inputOption);
	return CELL_OK;
}

error_code cellImeJpSetInputCharType(CellImeJpHandle hImeJpHandle, s16 charTypeOption)
{
	cellImeJp.todo("cellImeJpSetInputCharType(hImeJpHandle=*0x%x, charTypeOption=%d)", hImeJpHandle, charTypeOption);
	return CELL_OK;
}

error_code cellImeJpSetFixInputMode(CellImeJpHandle hImeJpHandle, s16 fixInputMode)
{
	cellImeJp.todo("cellImeJpSetFixInputMode(hImeJpHandle=*0x%x, fixInputMode=%d)", hImeJpHandle, fixInputMode);
	return CELL_OK;
}

error_code cellImeJpAllowExtensionCharacters(CellImeJpHandle hImeJpHandle, s16 extensionCharacters)
{
	cellImeJp.todo("cellImeJpSetFixInputMode(hImeJpHandle=*0x%x, extensionCharacters=%d)", hImeJpHandle, extensionCharacters);
	return CELL_OK;
}

error_code cellImeJpReset(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpReset(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpGetStatus(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pInputStatus)
{
	cellImeJp.todo("cellImeJpGetStatus(hImeJpHandle=*0x%x, pInputStatus=%d)", hImeJpHandle, pInputStatus);
	*pInputStatus = CELL_IMEJP_CANDIDATE_EMPTY;
	return CELL_OK;
}

error_code cellImeJpEnterChar(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterChar(hImeJpHandle=*0x%x, inputChar=%d, pOutputStatus=%d", hImeJpHandle, inputChar, pOutputStatus);
	*s_ime_string = inputChar;
	*pOutputStatus = CELL_IMEJP_RET_CONFIRMED;
	return CELL_OK;
}

error_code cellImeJpEnterCharExt(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterCharExt(hImeJpHandle=*0x%x, inputChar=%d, pOutputStatus=%d", hImeJpHandle, inputChar, pOutputStatus);
	*s_ime_string = inputChar;
	*pOutputStatus = CELL_IMEJP_RET_CONFIRMED;
	return cellImeJpEnterChar(hImeJpHandle, inputChar, pOutputStatus);
}

error_code cellImeJpEnterString(CellImeJpHandle hImeJpHandle, vm::cptr<u16> pInputString, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterString(hImeJpHandle=*0x%x, pInputString=*0x%x, pOutputStatus=%d", hImeJpHandle, pInputString, pOutputStatus);
	*s_ime_string = pInputString[0];
	*pOutputStatus = CELL_IMEJP_RET_CONFIRMED;
	return CELL_OK;
}

error_code cellImeJpEnterStringExt(CellImeJpHandle hImeJpHandle, vm::cptr<u16> pInputString, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterStringExt(hImeJpHandle=*0x%x, pInputString=*0x%x, pOutputStatus=%d", hImeJpHandle, pInputString, pOutputStatus);
	*s_ime_string = pInputString[0];
	*pOutputStatus = CELL_IMEJP_RET_CONFIRMED;
	return CELL_OK;
}

error_code cellImeJpModeCaretRight(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpModeCaretRight(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpModeCaretLeft(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpModeCaretLeft(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpBackspaceWord(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpBackspaceWord(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpDeleteWord(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpDeleteWord(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpAllDeleteConvertString(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpAllDeleteConvertString(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpConvertForward(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpConvertForward(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpConvertBackward(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpConvertBackward(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpCurrentPartConfirm(CellImeJpHandle hImeJpHandle, s16 listItem)
{
	cellImeJp.todo("cellImeJpCurrentPartConfirm(hImeJpHandle=*0x%x, listItem=%d)", hImeJpHandle, listItem);
	return CELL_OK;
}

error_code cellImeJpAllConfirm(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpAllConfirm(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpConvertCancel(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpConvertCancel(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpAllConvertCancel(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpAllConvertCancel(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpExtendConvertArea(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpExtendConvertArea(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpShortenConvertArea(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpShortenConvertArea(hImeJpHandle=*0x%x)", hImeJpHandle);
	return CELL_OK;
}

error_code cellImeJpTemporalConfirm(CellImeJpHandle hImeJpHandle, s16 selectIndex)
{
	cellImeJp.todo("cellImeJpTemporalConfirm(hImeJpHandle=*0x%x, selectIndex=%d)", hImeJpHandle, selectIndex);
	return CELL_OK;
}

error_code cellImeJpPostConvert(CellImeJpHandle hImeJpHandle, s16 postType)
{
	cellImeJp.todo("cellImeJpPostConvert(hImeJpHandle=*0x%x, postType=%d)", hImeJpHandle, postType);
	return CELL_OK;
}

error_code cellImeJpMoveFocusClause(CellImeJpHandle hImeJpHandle, s16 moveType)
{
	cellImeJp.todo("cellImeJpMoveFocusClause(hImeJpHandle=*0x%x, moveType=%d)", hImeJpHandle, moveType);
	return CELL_OK;
}

error_code cellImeJpGetFocusTop(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pFocusTop)
{
	cellImeJp.todo("cellImeJpGetFocusTop(hImeJpHandle=*0x%x, pFocusTop=*0x%x)", hImeJpHandle, pFocusTop);
	return CELL_OK;
}

error_code cellImeJpGetFocusLength(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pFocusLength)
{
	cellImeJp.todo("cellImeJpGetFocusLength(hImeJpHandle=*0x%x, pFocusLength=*0x%x)", hImeJpHandle, pFocusLength);
	*pFocusLength = 1;
	return CELL_OK;
}

error_code cellImeJpGetConfirmYomiString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pYomiString)
{
	cellImeJp.todo("cellImeJpGetConfirmYomiString(hImeJpHandle=*0x%x, pYomiString=*0x%x)", hImeJpHandle, pYomiString);
	*pYomiString = *s_ime_string;
	return CELL_OK;
}

error_code cellImeJpGetConfirmString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pConfirmString)
{
	cellImeJp.todo("cellImeJpGetConfirmString(hImeJpHandle=*0x%x, pConfirmString=*0x%x)", hImeJpHandle, pConfirmString);
	*pConfirmString = *s_ime_string;
	return CELL_OK;
}

error_code cellImeJpGetConvertYomiString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pYomiString)
{
	cellImeJp.todo("cellImeJpGetConvertYomiString(hImeJpHandle=*0x%x, pYomiString=*0x%x)", hImeJpHandle, pYomiString);
	*pYomiString = *s_ime_string;
	return CELL_OK;
}

error_code cellImeJpGetConvertString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pConvertString)
{
	cellImeJp.todo("cellImeJpGetConvertString(hImeJpHandle=*0x%x, pConvertString=*0x%x)", hImeJpHandle, pConvertString);
	*pConvertString = *s_ime_string;
	return CELL_OK;
}

error_code cellImeJpGetCandidateListSize(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pListSize)
{
	cellImeJp.todo("cellImeJpGetCandidateListSize(hImeJpHandle=*0x%x, pListSize=*0x%x)", hImeJpHandle, pListSize);
	return CELL_OK;
}

error_code cellImeJpGetCandidateList(CellImeJpHandle hImeJpHandle, vm::ptr<s16> plistNum, vm::ptr<u16> pCandidateString)
{
	cellImeJp.todo("cellImeJpGetCandidateList(hImeJpHandle=*0x%x, plistNum=*0x%x, pCandidateString=*0x%x)", hImeJpHandle, plistNum, pCandidateString);
	return CELL_OK;
}

error_code cellImeJpGetCandidateSelect(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pIndex)
{
	cellImeJp.todo("cellImeJpGetCandidateSelect(hImeJpHandle=*0x%x, pIndex=*0x%x)", hImeJpHandle, pIndex);
	return CELL_OK;
}

error_code cellImeJpGetPredictList(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pYomiString, s32 itemNum, vm::ptr<s32> plistCount, vm::ptr<CellImeJpPredictItem> pPredictItem)
{
	cellImeJp.todo("cellImeJpGetPredictList(hImeJpHandle=*0x%x, pYomiString=*0x%x, itemNum=%d, plistCount=*0x%x, pPredictItem=*0x%x)", hImeJpHandle, pYomiString, itemNum, plistCount, pPredictItem);
	return CELL_OK;
}

error_code cellImeJpConfirmPrediction(CellImeJpHandle hImeJpHandle, vm::ptr<CellImeJpPredictItem> pPredictItem)
{
	cellImeJp.todo("cellImeJpConfirmPrediction(hImeJpHandle=*0x%x, pPredictItem=*0x%x)", hImeJpHandle, pPredictItem);
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
