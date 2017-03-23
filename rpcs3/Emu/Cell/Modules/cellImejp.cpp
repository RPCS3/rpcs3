#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "cellImejp.h"

#include <cmath>
#include <cfenv>

logs::channel cellImeJp("cellImeJp", logs::level::notice);

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
	
	switch (inputChar) //inputChar must be a value based on [Unicode, 16-bit encode (UCS2)].
	{
		//ROMAJI UPPERCASE (FULLWIDTH)
		case 0xff21: *s_ime_string = 'A'; break;
		case 0xff22: *s_ime_string = 'B'; break;
		case 0xff23: *s_ime_string = 'C'; break;
		case 0xff24: *s_ime_string = 'D'; break;
		case 0xff25: *s_ime_string = 'E'; break;
		case 0xff26: *s_ime_string = 'F'; break;
		case 0xff27: *s_ime_string = 'G'; break;
		case 0xff28: *s_ime_string = 'H'; break;
		case 0xff29: *s_ime_string = 'I'; break;
		case 0xff2a: *s_ime_string = 'J'; break;
		case 0xff2b: *s_ime_string = 'K'; break;
		case 0xff2c: *s_ime_string = 'L'; break;
		case 0xff2d: *s_ime_string = 'M'; break;
		case 0xff2e: *s_ime_string = 'N'; break;
		case 0xff2f: *s_ime_string = 'O'; break;
		case 0xff30: *s_ime_string = 'P'; break;
		case 0xff31: *s_ime_string = 'Q'; break;
		case 0xff32: *s_ime_string = 'R'; break;
		case 0xff33: *s_ime_string = 'S'; break;
		case 0xff34: *s_ime_string = 'T'; break;
		case 0xff35: *s_ime_string = 'U'; break;
		case 0xff36: *s_ime_string = 'V'; break;
		case 0xff37: *s_ime_string = 'W'; break;
		case 0xff38: *s_ime_string = 'X'; break;
		case 0xff39: *s_ime_string = 'Y'; break;
		case 0xff3a: *s_ime_string = 'Z'; break;
		
		//ROMAJI LOWERCASE (FULLWIDTH)
		case 0xff41: *s_ime_string = 'a'; break;
		case 0xff42: *s_ime_string = 'b'; break;
		case 0xff43: *s_ime_string = 'c'; break;
		case 0xff44: *s_ime_string = 'd'; break;
		case 0xff45: *s_ime_string = 'e'; break;
		case 0xff46: *s_ime_string = 'f'; break;
		case 0xff47: *s_ime_string = 'g'; break;
		case 0xff48: *s_ime_string = 'h'; break;
		case 0xff49: *s_ime_string = 'i'; break;
		case 0xff4a: *s_ime_string = 'j'; break;
		case 0xff4b: *s_ime_string = 'k'; break;
		case 0xff4c: *s_ime_string = 'l'; break;
		case 0xff4d: *s_ime_string = 'm'; break;
		case 0xff4e: *s_ime_string = 'n'; break;
		case 0xff4f: *s_ime_string = 'o'; break;
		case 0xff50: *s_ime_string = 'p'; break;
		case 0xff51: *s_ime_string = 'q'; break;
		case 0xff52: *s_ime_string = 'r'; break;
		case 0xff53: *s_ime_string = 's'; break;
		case 0xff54: *s_ime_string = 't'; break;
		case 0xff55: *s_ime_string = 'u'; break;
		case 0xff56: *s_ime_string = 'v'; break;
		case 0xff57: *s_ime_string = 'w'; break;
		case 0xff58: *s_ime_string = 'x'; break;
		case 0xff59: *s_ime_string = 'y'; break;
		case 0xff5a: *s_ime_string = 'z'; break;

		//HIRAGANA (DOES NOT WORK)
		case 0x3041: *s_ime_string = 'ぁ'; break;
		case 0x3042: *s_ime_string = 'あ'; break;
		case 0x3043: *s_ime_string = 'ぃ'; break;
		case 0x3044: *s_ime_string = 'い'; break;
		case 0x3045: *s_ime_string = 'ぅ'; break;
		case 0x3046: *s_ime_string = 'う'; break;
		case 0x3047: *s_ime_string = 'ぇ'; break;
		case 0x3048: *s_ime_string = 'え'; break;
		case 0x3049: *s_ime_string = 'ぉ'; break;
		case 0x304a: *s_ime_string = 'お'; break;
		
		case 0x304b: *s_ime_string = 'か'; break;
		case 0x304c: *s_ime_string = 'が'; break;
		case 0x304d: *s_ime_string = 'き'; break;
		case 0x304e: *s_ime_string = 'ぎ'; break;
		case 0x304f: *s_ime_string = 'く'; break;
		case 0x3050: *s_ime_string = 'ぐ'; break;
		case 0x3051: *s_ime_string = 'け'; break;
		case 0x3052: *s_ime_string = 'げ'; break;
		case 0x3053: *s_ime_string = 'こ'; break;
		case 0x3054: *s_ime_string = 'ご'; break;
		
		case 0x3055: *s_ime_string = 'さ'; break;
		case 0x3056: *s_ime_string = 'ざ'; break;
		case 0x3057: *s_ime_string = 'し'; break;
		case 0x3058: *s_ime_string = 'じ'; break;
		case 0x3059: *s_ime_string = 'す'; break;
		case 0x305a: *s_ime_string = 'ず'; break;
		case 0x305b: *s_ime_string = 'せ'; break;
		case 0x305c: *s_ime_string = 'ぜ'; break;
		case 0x305d: *s_ime_string = 'そ'; break;
		case 0x305e: *s_ime_string = 'ぞ'; break;
		
		case 0x305f: *s_ime_string = 'た'; break;
		case 0x3060: *s_ime_string = 'だ'; break;
		case 0x3061: *s_ime_string = 'ち'; break;
		case 0x3062: *s_ime_string = 'ぢ'; break;
		case 0x3063: *s_ime_string = 'っ'; break;
		case 0x3064: *s_ime_string = 'つ'; break;
		case 0x3065: *s_ime_string = 'づ'; break;
		case 0x3066: *s_ime_string = 'て'; break;
		case 0x3067: *s_ime_string = 'で'; break;
		case 0x3068: *s_ime_string = 'と'; break;
		case 0x3069: *s_ime_string = 'ど'; break;
		
		case 0x306a: *s_ime_string = 'な'; break;
		case 0x306b: *s_ime_string = 'に'; break;
		case 0x306c: *s_ime_string = 'ぬ'; break;
		case 0x306d: *s_ime_string = 'ね'; break;
		case 0x306e: *s_ime_string = 'の'; break;
		
		case 0x306f: *s_ime_string = 'は'; break;
		case 0x3070: *s_ime_string = 'ば'; break;
		case 0x3071: *s_ime_string = 'ぱ'; break;
		case 0x3072: *s_ime_string = 'ひ'; break;
		case 0x3073: *s_ime_string = 'び'; break;
		case 0x3074: *s_ime_string = 'ぴ'; break;
		case 0x3075: *s_ime_string = 'ふ'; break;
		case 0x3076: *s_ime_string = 'ぶ'; break;
		case 0x3077: *s_ime_string = 'ぷ'; break;
		case 0x3078: *s_ime_string = 'へ'; break;
		case 0x3079: *s_ime_string = 'べ'; break;
		case 0x307a: *s_ime_string = 'ぺ'; break;
		case 0x307b: *s_ime_string = 'ほ'; break;
		case 0x307c: *s_ime_string = 'ぼ'; break;
		case 0x307d: *s_ime_string = 'ぽ'; break;
		
		case 0x307e: *s_ime_string = 'ま'; break;
		case 0x307f: *s_ime_string = 'み'; break;
		case 0x3080: *s_ime_string = 'む'; break;
		case 0x3081: *s_ime_string = 'め'; break;
		case 0x3082: *s_ime_string = 'も'; break;
		
		case 0x3083: *s_ime_string = 'ゃ'; break;
		case 0x3084: *s_ime_string = 'や'; break;
		case 0x3085: *s_ime_string = 'ゅ'; break;
		case 0x3086: *s_ime_string = 'ゆ'; break;
		case 0x3087: *s_ime_string = 'ょ'; break;
		case 0x3088: *s_ime_string = 'よ'; break;
		
		case 0x3089: *s_ime_string = 'ら'; break;
		case 0x308a: *s_ime_string = 'り'; break;
		case 0x308b: *s_ime_string = 'る'; break;
		case 0x308c: *s_ime_string = 'れ'; break;
		case 0x308d: *s_ime_string = 'ろ'; break;
		
		case 0x308e: *s_ime_string = 'ゎ'; break;
		case 0x308f: *s_ime_string = 'わ'; break;
		case 0x3090: *s_ime_string = 'ゐ'; break;
		case 0x3091: *s_ime_string = 'ゑ'; break;
		case 0x3092: *s_ime_string = 'を'; break;
		case 0x3093: *s_ime_string = 'ん'; break;
		case 0x3094: *s_ime_string = 'ゔ'; break;


		//KATAKANA
		//case: *s_ime_string = 'ア';
		//case: *s_ime_string = 'イ';
		//case: *s_ime_string = 'ウ';
		//case: *s_ime_string = 'エ';
		//case: *s_ime_string = 'オ';
		//case: *s_ime_string = 'カ';
		//case: *s_ime_string = 'キ';
		//case: *s_ime_string = 'ク';
		//case: *s_ime_string = 'ケ';
		//case: *s_ime_string = 'コ';
		//case: *s_ime_string = 'サ';
		//case: *s_ime_string = 'シ';
		//case: *s_ime_string = 'ス';
		//case: *s_ime_string = 'セ';
		//case: *s_ime_string = 'ソ';
		//case: *s_ime_string = 'タ';
		//case: *s_ime_string = 'チ';
		//case: *s_ime_string = 'ツ';
		//case: *s_ime_string = 'テ';
		//case: *s_ime_string = 'ト';
		//case: *s_ime_string = 'ナ';
		//case: *s_ime_string = 'ニ';
		//case: *s_ime_string = 'ヌ';
		//case: *s_ime_string = 'ネ';
		//case: *s_ime_string = 'ノ';
		//case: *s_ime_string = 'ハ';
		//case: *s_ime_string = 'ヒ';
		//case: *s_ime_string = 'フ';
		//case: *s_ime_string = 'ヘ';
		//case: *s_ime_string = 'ホ';
		//case: *s_ime_string = 'マ';
		//case: *s_ime_string = 'ミ';
		//case: *s_ime_string = 'ム';
		//case: *s_ime_string = 'メ';
		//case: *s_ime_string = 'モ';
		//case: *s_ime_string = 'ヤ';
		//case: *s_ime_string = 'ユ';
		//case: *s_ime_string = 'ヨ';
		//case: *s_ime_string = 'ラ';
		//case: *s_ime_string = 'リ';
		//case: *s_ime_string = 'ル';
		//case: *s_ime_string = 'レ';
		//case: *s_ime_string = 'ロ';
		//case: *s_ime_string = 'ン';
			
		//NUMBERS
		case 0xff10: *s_ime_string = '0'; break;
		case 0xff11: *s_ime_string = '1'; break;
		case 0xff12: *s_ime_string = '2'; break;
		case 0xff13: *s_ime_string = '3'; break;
		case 0xff14: *s_ime_string = '4'; break;
		case 0xff15: *s_ime_string = '5'; break;
		case 0xff16: *s_ime_string = '6'; break;
		case 0xff17: *s_ime_string = '7'; break;
		case 0xff18: *s_ime_string = '8'; break;
		case 0xff19: *s_ime_string = '9'; break;
		
		//UNKNOWN CASE
		default:
			{
				*s_ime_string = 'A';
				cellImeJp.todo("cellImeJpEnterChar undefined UCS2 char code: (0x%x), %s" HERE, inputChar, *s_ime_string);
				break;
			}
	}
	
	*pOutputStatus = 2; //CELL_IMEJP_RET_CONFIRMED????
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
