#pragma once

#include "Emu/Memory/vm_ptr.h"

typedef vm::ptr<void> CellImeJpHandle;

// Return Codes
enum CellImeJpError : u32
{
	CELL_IMEJP_ERROR_ERR                = 0x8002bf01,
	CELL_IMEJP_ERROR_CONTEXT            = 0x8002bf11,
	CELL_IMEJP_ERROR_ALREADY_OPEN       = 0x8002bf21,
	CELL_IMEJP_ERROR_DIC_OPEN           = 0x8002bf31,
	CELL_IMEJP_ERROR_PARAM              = 0x8002bf41,
	CELL_IMEJP_ERROR_IME_ALREADY_IN_USE = 0x8002bf51,
	CELL_IMEJP_ERROR_OTHER              = 0x8002bfff,
};

// Input state of the ImeJp
enum
{
	CELL_IMEJP_BEFORE_INPUT     = 0,
	CELL_IMEJP_BEFORE_CONVERT   = 1,
	CELL_IMEJP_CONVERTING       = 2,
	CELL_IMEJP_CANDIDATE_EMPTY  = 3,
	CELL_IMEJP_POSTCONVERT_KANA = 4,
	CELL_IMEJP_POSTCONVERT_HALF = 5,
	CELL_IMEJP_POSTCONVERT_RAW  = 6,
	CELL_IMEJP_CANDIDATES       = 7,
	CELL_IMEJP_MOVE_CLAUSE_GAP  = 8,
};

// cellImeJpEnterChar, returning values pointed in pOutputStatus.
enum
{
	CELL_IMEJP_RET_NONE      = 0,
	CELL_IMEJP_RET_THROUGH   = 1,
	CELL_IMEJP_RET_CONFIRMED = 2,
};

enum
{
	CELL_IMEJP_ROMAN_INPUT = 0,
	CELL_IMEJP_KANA_INPUT  = 1,
};

enum
{
	CELL_IMEJP_DSPCHAR_HIRA  = 1,
	CELL_IMEJP_DSPCHAR_FKANA = 2,
	CELL_IMEJP_DSPCHAR_RAW   = 3,
	CELL_IMEJP_DSPCHAR_HKANA = 4,
	CELL_IMEJP_DSPCHAR_HRAW  = 5,
};

enum
{
	CELL_IMEJP_FIXINMODE_OFF  = 0,
	CELL_IMEJP_FIXINMODE_HIRA = 1,
	CELL_IMEJP_FIXINMODE_FKAN = 2,
	CELL_IMEJP_FIXINMODE_RAW  = 3,
	CELL_IMEJP_FIXINMODE_HKAN = 4,
	CELL_IMEJP_FIXINMODE_HRAW = 5,
};

enum
{
	CELL_IMEJP_EXTENSIONCH_NONE     = 0x0000,
	CELL_IMEJP_EXTENSIONCH_HANKANA  = 0x0001,
	CELL_IMEJP_EXTENSIONCH_UD09TO15 = 0x0004,
	CELL_IMEJP_EXTENSIONCH_UD85TO94 = 0x0008,
	CELL_IMEJP_EXTENSIONCH_OUTJIS   = 0x0010,
};

enum
{
	CELL_IMEJP_POSTCONV_HIRA = 1,
	CELL_IMEJP_POSTCONV_KANA = 2,
	CELL_IMEJP_POSTCONV_HALF = 3,
	CELL_IMEJP_POSTCONV_RAW  = 4,
};

enum
{
	CELL_IMEJP_FOCUS_NEXT   = 0,
	CELL_IMEJP_FOCUS_BEFORE = 1,
	CELL_IMEJP_FOCUS_TOP    = 2,
	CELL_IMEJP_FOCUS_END    = 3,
};

enum
{
	CELL_IMEJP_DIC_PATH_MAXLENGTH = 256,

	// Helper
	CELL_IMEJP_STRING_MAXLENGTH = 128, // including terminator
};

struct CellImeJpAddDic
{
	char path[CELL_IMEJP_DIC_PATH_MAXLENGTH];
};

struct CellImeJpPredictItem
{
	u16 KanaYomi[31];
	u16 Hyoki[61];
};

struct ime_jp_manager
{
	shared_mutex mutex;

	atomic_t<bool> is_initialized{ false };

	u32 input_state = CELL_IMEJP_BEFORE_INPUT;
	std::vector<std::string> dictionary_paths;
	std::u16string confirmed_string; // Confirmed part of the string (first part of the entire string)
	std::u16string converted_string; // Converted part of the unconfirmed input string
	std::u16string input_string;     // Unconfirmed part of the string (second part of the entire string)
	usz cursor = 0;       // The cursor. Can move across the entire input string.
	usz focus_begin = 0;  // Begin of the focus string
	usz focus_length = 0; // Length of the focus string
	s16 fix_input_mode = CELL_IMEJP_FIXINMODE_OFF;
	s16 input_char_type = CELL_IMEJP_DSPCHAR_HIRA;
	s16 kana_input_mode = CELL_IMEJP_ROMAN_INPUT;
	s16 allowed_extensions = CELL_IMEJP_EXTENSIONCH_UD09TO15 | CELL_IMEJP_EXTENSIONCH_UD85TO94 | CELL_IMEJP_EXTENSIONCH_OUTJIS;

	ime_jp_manager();

	bool addChar(u16 c);
	bool addString(vm::cptr<u16> str);
	bool backspaceWord();
	bool deleteWord();
	bool remove_character(bool forward);
	void clear_input();
	void move_cursor(s8 amount);                      // s8 because CELL_IMEJP_STRING_MAXLENGTH is 128
	void move_focus(s8 amount);                       // s8 because CELL_IMEJP_STRING_MAXLENGTH is 128
	void move_focus_end(s8 amount, bool wrap_around); // s8 because CELL_IMEJP_STRING_MAXLENGTH is 128

	struct candidate
	{
		std::u16string text; // Actual text of the candidate
		u16 offset = 0;      // The offset of the next character after the candidate replaced part of the current focus.
	};
	std::vector<candidate> get_candidate_list() const;
	std::u16string get_focus_string() const;
};
