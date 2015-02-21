#include "stdafx.h"
#if 0

void cellImejp_init();
Module cellImejp(0xf023, cellImejp_init);

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

int cellImeJpOpen()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpOpen2()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpOpen3()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpClose()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpSetKanaInputMode()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpSetInputCharType()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpSetFixInputMode()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpReset()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetStatus()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpEnterChar()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpEnterCharExt()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpEnterString()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpEnterStringExt()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpModeCaretRight()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpModeCaretLeft()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpBackspaceWord()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpDeleteWord()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpAllDeleteConvertString()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpConvertForward()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpConvertBackward()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpCurrentPartConfirm()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpAllConfirm()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpConvertCancel()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpAllConvertCancel()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpExtendConvertArea()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpShortenConvertArea()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpTemporalConfirm()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpPostConvert()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpMoveFocusClause()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetFocusTop()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetFocusLength()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetConfirmYomiString()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetConfirmString()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetConvertYomiString()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetConvertString()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetCandidateListSize()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetCandidateList()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetCandidateSelect()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpGetPredictList()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

int cellImeJpConfirmPrediction()
{
	UNIMPLEMENTED_FUNC(cellImejp);
	return CELL_OK;
}

void cellImejp_init()
{
	REG_FUNC(cellImejp, cellImeJpOpen);
	REG_FUNC(cellImejp, cellImeJpOpen2);
	REG_FUNC(cellImejp, cellImeJpOpen3);
	REG_FUNC(cellImejp, cellImeJpClose);

	REG_FUNC(cellImejp, cellImeJpSetKanaInputMode);
	REG_FUNC(cellImejp, cellImeJpSetInputCharType);
	REG_FUNC(cellImejp, cellImeJpSetFixInputMode);
	//cellImejp.AddFunc(, cellImeJpAllowExtensionCharacters);
	REG_FUNC(cellImejp, cellImeJpReset);

	REG_FUNC(cellImejp, cellImeJpGetStatus);

	REG_FUNC(cellImejp, cellImeJpEnterChar);
	REG_FUNC(cellImejp, cellImeJpEnterCharExt);
	REG_FUNC(cellImejp, cellImeJpEnterString);
	REG_FUNC(cellImejp, cellImeJpEnterStringExt);
	REG_FUNC(cellImejp, cellImeJpModeCaretRight);
	REG_FUNC(cellImejp, cellImeJpModeCaretLeft);
	REG_FUNC(cellImejp, cellImeJpBackspaceWord);
	REG_FUNC(cellImejp, cellImeJpDeleteWord);
	REG_FUNC(cellImejp, cellImeJpAllDeleteConvertString);
	REG_FUNC(cellImejp, cellImeJpConvertForward);
	REG_FUNC(cellImejp, cellImeJpConvertBackward);
	REG_FUNC(cellImejp, cellImeJpCurrentPartConfirm);
	REG_FUNC(cellImejp, cellImeJpAllConfirm);
	REG_FUNC(cellImejp, cellImeJpConvertCancel);
	REG_FUNC(cellImejp, cellImeJpAllConvertCancel);
	REG_FUNC(cellImejp, cellImeJpExtendConvertArea);
	REG_FUNC(cellImejp, cellImeJpShortenConvertArea);
	REG_FUNC(cellImejp, cellImeJpTemporalConfirm);
	REG_FUNC(cellImejp, cellImeJpPostConvert);
	REG_FUNC(cellImejp, cellImeJpMoveFocusClause);
	REG_FUNC(cellImejp, cellImeJpGetFocusTop);
	REG_FUNC(cellImejp, cellImeJpGetFocusLength);
	REG_FUNC(cellImejp, cellImeJpGetConfirmYomiString);
	REG_FUNC(cellImejp, cellImeJpGetConfirmString);
	REG_FUNC(cellImejp, cellImeJpGetConvertYomiString);
	REG_FUNC(cellImejp, cellImeJpGetConvertString);
	REG_FUNC(cellImejp, cellImeJpGetCandidateListSize);
	REG_FUNC(cellImejp, cellImeJpGetCandidateList);
	REG_FUNC(cellImejp, cellImeJpGetCandidateSelect);
	REG_FUNC(cellImejp, cellImeJpGetPredictList);
	REG_FUNC(cellImejp, cellImeJpConfirmPrediction);
}
#endif
