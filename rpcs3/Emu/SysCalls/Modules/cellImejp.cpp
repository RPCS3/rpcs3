#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

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
	cellImejp.AddFunc(0x44608862, cellImeJpOpen);
	cellImejp.AddFunc(0x47b43dd4, cellImeJpOpen2);
	cellImejp.AddFunc(0x1b119958, cellImeJpOpen3);
	cellImejp.AddFunc(0x46d1234a, cellImeJpClose);

	cellImejp.AddFunc(0x24e9d8fc, cellImeJpSetKanaInputMode);
	cellImejp.AddFunc(0xf5992ec8, cellImeJpSetInputCharType);
	cellImejp.AddFunc(0xc1786c81, cellImeJpSetFixInputMode);
	//cellImejp.AddFunc(, cellImeJpAllowExtensionCharacters);
	cellImejp.AddFunc(0x36d38701, cellImeJpReset);

	cellImejp.AddFunc(0x66c6cc78, cellImeJpGetStatus);

	cellImejp.AddFunc(0x6ccbe3d6, cellImeJpEnterChar);
	cellImejp.AddFunc(0x5b6ada55, cellImeJpEnterCharExt);
	cellImejp.AddFunc(0x441a1c2b, cellImeJpEnterString);
	cellImejp.AddFunc(0x6298b55a, cellImeJpEnterStringExt);
	cellImejp.AddFunc(0xac6693d8, cellImeJpModeCaretRight);
	cellImejp.AddFunc(0xe76c9700, cellImeJpModeCaretLeft);
	cellImejp.AddFunc(0xaa1d1f57, cellImeJpBackspaceWord);
	cellImejp.AddFunc(0x72257652, cellImeJpDeleteWord);
	cellImejp.AddFunc(0x6319eda3, cellImeJpAllDeleteConvertString);
	cellImejp.AddFunc(0x1e29103b, cellImeJpConvertForward);
	cellImejp.AddFunc(0xc2bb48bc, cellImeJpConvertBackward);
	cellImejp.AddFunc(0x7a18c2b9, cellImeJpCurrentPartConfirm);
	cellImejp.AddFunc(0x7189430b, cellImeJpAllConfirm);
	cellImejp.AddFunc(0xeae879dc, cellImeJpConvertCancel);
	cellImejp.AddFunc(0xcbbc20b7, cellImeJpAllConvertCancel);
	cellImejp.AddFunc(0x37961cc1, cellImeJpExtendConvertArea);
	cellImejp.AddFunc(0xaa2a3287, cellImeJpShortenConvertArea);
	cellImejp.AddFunc(0xbd679cc1, cellImeJpTemporalConfirm);
	cellImejp.AddFunc(0x8bb41f47, cellImeJpPostConvert);
	cellImejp.AddFunc(0x1e411261, cellImeJpMoveFocusClause);
	cellImejp.AddFunc(0x0e363ae7, cellImeJpGetFocusTop);
	cellImejp.AddFunc(0x5f5b3227, cellImeJpGetFocusLength);
	cellImejp.AddFunc(0x89f8a567, cellImeJpGetConfirmYomiString);
	cellImejp.AddFunc(0xd3fc3606, cellImeJpGetConfirmString);
	cellImejp.AddFunc(0xea2d4881, cellImeJpGetConvertYomiString);
	cellImejp.AddFunc(0xf91abda3, cellImeJpGetConvertString);
	cellImejp.AddFunc(0xc4796a45, cellImeJpGetCandidateListSize);
	cellImejp.AddFunc(0xe4cc15ba, cellImeJpGetCandidateList);
	cellImejp.AddFunc(0x177bd218, cellImeJpGetCandidateSelect);
	cellImejp.AddFunc(0x1986f2cd, cellImeJpGetPredictList);
	cellImejp.AddFunc(0xeede898c, cellImeJpConfirmPrediction);
}