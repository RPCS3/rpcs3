#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/IdManager.h"
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

const u32 ime_jp_address = 0xf0000000;

ime_jp_manager::ime_jp_manager()
{
	if (static_cast<s32>(g_ps3_process_info.sdk_ver) < 0x360000) // firmware < 3.6.0
		allowed_extensions = CELL_IMEJP_EXTENSIONCH_UD85TO94 | CELL_IMEJP_EXTENSIONCH_OUTJIS;
	else
		allowed_extensions = CELL_IMEJP_EXTENSIONCH_UD09TO15 | CELL_IMEJP_EXTENSIONCH_UD85TO94 | CELL_IMEJP_EXTENSIONCH_OUTJIS;
}

bool ime_jp_manager::addChar(u16 c)
{
	if (!c || cursor >= (CELL_IMEJP_STRING_MAXLENGTH - 1ULL) || cursor > input_string.length())
		return false;

	std::u16string tmp;
	tmp += c;
#if defined(__GNUG__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
	input_string.insert(cursor, tmp);
#if defined(__GNUG__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

	const usz cursor_old = cursor;
	const bool cursor_was_in_focus = cursor >= focus_begin && cursor <= (focus_begin + focus_length);

	move_cursor(1);

	if (cursor_was_in_focus)
	{
		// Add this char to the focus
		move_focus_end(1, false);
	}
	else
	{
		// Let's just move the focus to the cursor, so that it contains the new char.
		focus_begin = cursor_old;
		focus_length = 1;
		move_focus(0); // Sanitize focus
	}

	input_state = CELL_IMEJP_BEFORE_CONVERT;
	return true;
}

bool ime_jp_manager::addString(vm::cptr<u16> str)
{
	if (!str)
		return false;

	for (u32 i = 0; i < CELL_IMEJP_STRING_MAXLENGTH; i++)
	{
		if (!addChar(str[i]))
			return false;
	}

	return true;
}

bool ime_jp_manager::backspaceWord()
{
	return remove_character(false);
}

bool ime_jp_manager::deleteWord()
{
	return remove_character(true);
}

bool ime_jp_manager::remove_character(bool forward)
{
	if (!forward && !cursor)
	{
		return false;
	}

	const usz pos = forward ? cursor : (cursor - 1);

	if (pos >= (CELL_IMEJP_STRING_MAXLENGTH - 1ULL) || pos >= input_string.length())
	{
		return false;
	}

	// Delete the character at the position
	input_string.erase(pos, 1);

	// Move cursor and focus
	const bool deleted_part_of_focus = pos > focus_begin && pos <= (focus_begin + focus_length);

	if (!forward)
	{
		move_cursor(-1);
	}

	if (deleted_part_of_focus)
	{
		move_focus_end(-1, false);
	}
	else if (focus_begin > pos)
	{
		move_focus(-1);
	}

	if (input_string.empty())
	{
		input_state = CELL_IMEJP_BEFORE_INPUT;
	}

	return true;
}

void ime_jp_manager::clear_input()
{
	cursor = 0;
	focus_begin = 0;
	focus_length = 0;
	input_string.clear();
	converted_string.clear();
}

void ime_jp_manager::move_cursor(s8 amount)
{
	cursor = std::max(0, std::min(static_cast<s32>(cursor) + amount, ::narrow<s32>(input_string.length())));
}

void ime_jp_manager::move_focus(s8 amount)
{
	focus_begin = std::max(0, std::min(static_cast<s32>(focus_begin) + amount, ::narrow<s32>(input_string.length())));
	move_focus_end(amount, false);
}

void ime_jp_manager::move_focus_end(s8 amount, bool wrap_around)
{
	if (focus_begin >= input_string.length())
	{
		focus_length = 0;
		return;
	}

	constexpr usz min_length = 1;
	const usz max_length = input_string.length() - focus_begin;

	if (amount > 0)
	{
		if (wrap_around && focus_length >= max_length)
		{
			focus_length = min_length;
		}
		else
		{
			focus_length += static_cast<usz>(amount);
		}
	}
	else if (amount < 0)
	{
		if (wrap_around && focus_length <= min_length)
		{
			focus_length = max_length;
		}
		else
		{
			focus_length = std::max(0, static_cast<s32>(focus_length) + amount);
		}
	}

	focus_length = std::max(min_length, std::min(max_length, focus_length));
}

std::vector<ime_jp_manager::candidate> ime_jp_manager::get_candidate_list() const
{
	std::vector<candidate> candidates;
	if (input_string.empty() || focus_length == 0 || focus_begin >= input_string.length())
		return candidates;

	// TODO: we just fake this with one candidate for now
	candidates.push_back(candidate{
		.text = get_focus_string(),
		.offset = 0
	});

	return candidates;
}

std::u16string ime_jp_manager::get_focus_string() const
{
	if (input_string.empty() || focus_length == 0 || focus_begin >= input_string.length())
		return {};

	return input_string.substr(focus_begin, focus_length);
}


static error_code cellImeJpOpen(sys_memory_container_t container_id, vm::ptr<CellImeJpHandle> hImeJpHandle, vm::cptr<CellImeJpAddDic> addDicPath)
{
	cellImeJp.todo("cellImeJpOpen(container_id=*0x%x, hImeJpHandle=*0x%x, addDicPath=*0x%x)", container_id, hImeJpHandle, addDicPath);

	if (!container_id || !hImeJpHandle)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_ALREADY_OPEN;
	}

	if (addDicPath && addDicPath->path[0])
	{
		cellImeJp.warning("cellImeJpOpen dictionary path = %s", addDicPath->path);

		manager.dictionary_paths.emplace_back(addDicPath->path);
	}
	*hImeJpHandle = vm::cast(ime_jp_address);

	manager.is_initialized = true;

	return CELL_OK;
}

static error_code cellImeJpOpen2(sys_memory_container_t container_id, vm::ptr<CellImeJpHandle> hImeJpHandle, vm::cptr<CellImeJpAddDic> addDicPath)
{
	cellImeJp.todo("cellImeJpOpen2(container_id=*0x%x, hImeJpHandle=*0x%x, addDicPath=*0x%x)", container_id, hImeJpHandle, addDicPath);

	if (!container_id || !hImeJpHandle)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_ALREADY_OPEN;
	}

	if (addDicPath && addDicPath->path[0])
	{
		cellImeJp.warning("cellImeJpOpen2 dictionary path = %s", addDicPath->path);

		manager.dictionary_paths.emplace_back(addDicPath->path);
	}

	*hImeJpHandle = vm::cast(ime_jp_address);

	manager.is_initialized = true;

	return CELL_OK;
}

static error_code cellImeJpOpen3(sys_memory_container_t container_id, vm::ptr<CellImeJpHandle> hImeJpHandle, vm::cpptr<CellImeJpAddDic> addDicPath)
{
	cellImeJp.todo("cellImeJpOpen3(container_id=*0x%x, hImeJpHandle=*0x%x, addDicPath=*0x%x)", container_id, hImeJpHandle, addDicPath);

	if (!container_id || !hImeJpHandle)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_ALREADY_OPEN;
	}

	if (addDicPath)
	{
		for (u32 i = 0; i < 4; i++)
		{
			if (addDicPath[i] && addDicPath[i]->path[0])
			{
				cellImeJp.warning("cellImeJpOpen3 dictionary %d path = %s", i, addDicPath[i]->path);

				manager.dictionary_paths.emplace_back(addDicPath[i]->path);
			}
		}
	}

	*hImeJpHandle = vm::cast(ime_jp_address);

	manager.is_initialized = true;

	return CELL_OK;
}

static error_code cellImeJpOpenExt()
{
	cellImeJp.todo("cellImeJpOpenExt()");
	return CELL_OK;
}

static error_code cellImeJpClose(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpClose(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	manager.input_state = CELL_IMEJP_BEFORE_INPUT;
	manager.clear_input();
	manager.confirmed_string.clear();
	manager.is_initialized = false;

	return CELL_OK;
}

static error_code cellImeJpSetKanaInputMode(CellImeJpHandle hImeJpHandle, s16 inputOption)
{
	cellImeJp.todo("cellImeJpSetKanaInputMode(hImeJpHandle=*0x%x, inputOption=%d)", hImeJpHandle, inputOption);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state != CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.kana_input_mode = inputOption;

	return CELL_OK;
}

static error_code cellImeJpSetInputCharType(CellImeJpHandle hImeJpHandle, s16 charTypeOption)
{
	cellImeJp.todo("cellImeJpSetInputCharType(hImeJpHandle=*0x%x, charTypeOption=%d)", hImeJpHandle, charTypeOption);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	manager.input_char_type = charTypeOption;

	return CELL_OK;
}

static error_code cellImeJpSetFixInputMode(CellImeJpHandle hImeJpHandle, s16 fixInputMode)
{
	cellImeJp.todo("cellImeJpSetFixInputMode(hImeJpHandle=*0x%x, fixInputMode=%d)", hImeJpHandle, fixInputMode);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	manager.fix_input_mode = fixInputMode;

	return CELL_OK;
}

static error_code cellImeJpAllowExtensionCharacters(CellImeJpHandle hImeJpHandle, s16 extensionCharacters)
{
	cellImeJp.todo("cellImeJpSetFixInputMode(hImeJpHandle=*0x%x, extensionCharacters=%d)", hImeJpHandle, extensionCharacters);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state != CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.allowed_extensions = extensionCharacters;

	return CELL_OK;
}

static error_code cellImeJpReset(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpReset(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	manager.input_state = CELL_IMEJP_BEFORE_INPUT;
	manager.clear_input();
	manager.confirmed_string.clear();

	return CELL_OK;
}

static error_code cellImeJpGetStatus(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pInputStatus)
{
	cellImeJp.warning("cellImeJpGetStatus(hImeJpHandle=*0x%x, pInputStatus=%d)", hImeJpHandle, pInputStatus);

	if (!pInputStatus)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	*pInputStatus = manager.input_state;

	return CELL_OK;
}

static error_code cellImeJpEnterChar(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterChar(hImeJpHandle=*0x%x, inputChar=%d, pOutputStatus=%d)", hImeJpHandle, inputChar, pOutputStatus);

	if (!pOutputStatus)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_MOVE_CLAUSE_GAP)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.addChar(inputChar);

	*pOutputStatus = CELL_IMEJP_RET_CONFIRMED;

	return CELL_OK;
}

static error_code cellImeJpEnterCharExt(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterCharExt(hImeJpHandle=*0x%x, inputChar=%d, pOutputStatus=%d", hImeJpHandle, inputChar, pOutputStatus);
	return cellImeJpEnterChar(hImeJpHandle, inputChar, pOutputStatus);
}

static error_code cellImeJpEnterString(CellImeJpHandle hImeJpHandle, vm::cptr<u16> pInputString, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterString(hImeJpHandle=*0x%x, pInputString=*0x%x, pOutputStatus=%d", hImeJpHandle, pInputString, pOutputStatus);

	if (!pOutputStatus)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_MOVE_CLAUSE_GAP)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.addString(pInputString);

	*pOutputStatus = CELL_IMEJP_RET_CONFIRMED;

	return CELL_OK;
}

static error_code cellImeJpEnterStringExt(CellImeJpHandle hImeJpHandle, vm::cptr<u16> pInputString, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterStringExt(hImeJpHandle=*0x%x, pInputString=*0x%x, pOutputStatus=%d", hImeJpHandle, pInputString, pOutputStatus);
	return cellImeJpEnterString(hImeJpHandle, pInputString, pOutputStatus);
}

static error_code cellImeJpModeCaretRight(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpModeCaretRight(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state != CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.move_cursor(1);

	return CELL_OK;
}

static error_code cellImeJpModeCaretLeft(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpModeCaretLeft(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state != CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.move_cursor(-1);

	return CELL_OK;
}

static error_code cellImeJpBackspaceWord(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpBackspaceWord(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state != CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.backspaceWord();

	return CELL_OK;
}

static error_code cellImeJpDeleteWord(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpDeleteWord(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state != CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.deleteWord();

	return CELL_OK;
}

static error_code cellImeJpAllDeleteConvertString(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpAllDeleteConvertString(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.clear_input();
	manager.input_state = CELL_IMEJP_BEFORE_INPUT;

	return CELL_OK;
}

static error_code cellImeJpConvertForward(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpConvertForward(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.input_state = CELL_IMEJP_CANDIDATES;

	return CELL_OK;
}

static error_code cellImeJpConvertBackward(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpConvertBackward(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.input_state = CELL_IMEJP_CANDIDATES;

	return CELL_OK;
}

static error_code cellImeJpCurrentPartConfirm(CellImeJpHandle hImeJpHandle, s16 listItem)
{
	cellImeJp.todo("cellImeJpCurrentPartConfirm(hImeJpHandle=*0x%x, listItem=%d)", hImeJpHandle, listItem);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	return CELL_OK;
}

static error_code cellImeJpAllConfirm(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpAllConfirm(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	// Use input_string for now
	manager.confirmed_string = manager.input_string;
	manager.clear_input();
	manager.input_state = CELL_IMEJP_BEFORE_INPUT;

	return CELL_OK;
}

static error_code cellImeJpAllConvertCancel(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpAllConvertCancel(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT || manager.input_state == CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.converted_string.clear();
	manager.input_state = CELL_IMEJP_BEFORE_CONVERT;

	return CELL_OK;
}

static error_code cellImeJpConvertCancel(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpConvertCancel(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT || manager.input_state == CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager.converted_string.clear();
	manager.input_state = CELL_IMEJP_BEFORE_CONVERT;

	return CELL_OK;
}

static error_code cellImeJpExtendConvertArea(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpExtendConvertArea(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT || manager.input_state == CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	// Move end of the focus by one. Wrap around if the focus end is already at the end of the input string.
	manager.move_focus_end(1, true);

	return CELL_OK;
}

static error_code cellImeJpShortenConvertArea(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpShortenConvertArea(hImeJpHandle=*0x%x)", hImeJpHandle);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT || manager.input_state == CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	// Move end of focus by one. Wrap around if the focus end is already at the beginning of the input string.
	manager.move_focus_end(-1, true);

	return CELL_OK;
}

static error_code cellImeJpTemporalConfirm(CellImeJpHandle hImeJpHandle, s16 selectIndex)
{
	cellImeJp.todo("cellImeJpTemporalConfirm(hImeJpHandle=*0x%x, selectIndex=%d)", hImeJpHandle, selectIndex);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state != CELL_IMEJP_CANDIDATES)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	return CELL_OK;
}

static error_code cellImeJpPostConvert(CellImeJpHandle hImeJpHandle, s16 postType)
{
	cellImeJp.todo("cellImeJpPostConvert(hImeJpHandle=*0x%x, postType=%d)", hImeJpHandle, postType);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	return CELL_OK;
}

static error_code cellImeJpMoveFocusClause(CellImeJpHandle hImeJpHandle, s16 moveType)
{
	cellImeJp.todo("cellImeJpMoveFocusClause(hImeJpHandle=*0x%x, moveType=%d)", hImeJpHandle, moveType);

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state == CELL_IMEJP_BEFORE_INPUT || manager.input_state == CELL_IMEJP_BEFORE_CONVERT || manager.input_state == CELL_IMEJP_MOVE_CLAUSE_GAP)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	switch (moveType)
	{
	case CELL_IMEJP_FOCUS_NEXT:
		manager.move_focus(1);
		break;
	case CELL_IMEJP_FOCUS_BEFORE:
		manager.move_focus(-1);
		break;
	case CELL_IMEJP_FOCUS_TOP:
		manager.move_focus(-1 * ::narrow<s8>(manager.input_string.length()));
		break;
	case CELL_IMEJP_FOCUS_END:
		manager.move_focus(::narrow<s8>(manager.input_string.length()));
		manager.move_focus(-1);
		break;
	default:
		break;
	}

	manager.input_state = CELL_IMEJP_CONVERTING;

	return CELL_OK;
}

static error_code cellImeJpGetFocusTop(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pFocusTop)
{
	cellImeJp.todo("cellImeJpGetFocusTop(hImeJpHandle=*0x%x, pFocusTop=*0x%x)", hImeJpHandle, pFocusTop);

	if (!pFocusTop)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	*pFocusTop = static_cast<u16>(manager.focus_begin * 2); // offset in bytes

	return CELL_OK;
}

static error_code cellImeJpGetFocusLength(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pFocusLength)
{
	cellImeJp.todo("cellImeJpGetFocusLength(hImeJpHandle=*0x%x, pFocusLength=*0x%x)", hImeJpHandle, pFocusLength);

	if (!pFocusLength)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	*pFocusLength = ::narrow<s16>(manager.focus_length * 2); // offset in bytes

	return CELL_OK;
}

static error_code cellImeJpGetConfirmYomiString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pYomiString)
{
	cellImeJp.todo("cellImeJpGetConfirmYomiString(hImeJpHandle=*0x%x, pYomiString=*0x%x)", hImeJpHandle, pYomiString);

	if (!pYomiString)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	const usz max_len = std::min<usz>(CELL_IMEJP_STRING_MAXLENGTH - 1ULL, manager.confirmed_string.length());

	for (u32 i = 0; i < max_len; i++)
	{
		pYomiString[i] = manager.confirmed_string[i];
	}

	for (u32 i = static_cast<u32>(max_len); i < CELL_IMEJP_STRING_MAXLENGTH; i++)
	{
		pYomiString[i] = 0;
	}

	return CELL_OK;
}

static error_code cellImeJpGetConfirmString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pConfirmString)
{
	cellImeJp.todo("cellImeJpGetConfirmString(hImeJpHandle=*0x%x, pConfirmString=*0x%x)", hImeJpHandle, pConfirmString);

	if (!pConfirmString)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	const usz max_len = std::min<usz>(CELL_IMEJP_STRING_MAXLENGTH - 1ULL, manager.confirmed_string.length());

	for (u32 i = 0; i < max_len; i++)
	{
		pConfirmString[i] = manager.confirmed_string[i];
	}

	for (u32 i = static_cast<u32>(max_len); i < CELL_IMEJP_STRING_MAXLENGTH; i++)
	{
		pConfirmString[i] = 0;
	}

	return CELL_OK;
}

static error_code cellImeJpGetConvertYomiString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pYomiString)
{
	cellImeJp.todo("cellImeJpGetConvertYomiString(hImeJpHandle=*0x%x, pYomiString=*0x%x)", hImeJpHandle, pYomiString);

	if (!pYomiString)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	const usz max_len = std::min<usz>(CELL_IMEJP_STRING_MAXLENGTH - 1ULL, manager.input_string.length());

	for (u32 i = 0; i < max_len; i++)
	{
		pYomiString[i] = manager.input_string[i];
	}

	for (u32 i = static_cast<u32>(max_len); i < CELL_IMEJP_STRING_MAXLENGTH; i++)
	{
		pYomiString[i] = 0;
	}

	return CELL_OK;
}

static error_code cellImeJpGetConvertString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pConvertString)
{
	cellImeJp.warning("cellImeJpGetConvertString(hImeJpHandle=*0x%x, pConvertString=*0x%x)", hImeJpHandle, pConvertString);

	if (!pConvertString)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	const usz max_len = std::min<usz>(CELL_IMEJP_STRING_MAXLENGTH - 1ULL, manager.input_string.length());

	for (u32 i = 0; i < max_len; i++)
	{
		pConvertString[i] = manager.input_string[i];
	}

	for (u32 i = static_cast<u32>(max_len); i < CELL_IMEJP_STRING_MAXLENGTH; i++)
	{
		pConvertString[i] = 0;
	}

	return CELL_OK;
}

static error_code cellImeJpGetCandidateListSize(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pListSize)
{
	cellImeJp.todo("cellImeJpGetCandidateListSize(hImeJpHandle=*0x%x, pListSize=*0x%x)", hImeJpHandle, pListSize);

	if (!pListSize)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state != CELL_IMEJP_CANDIDATES)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	// Add focus string size, including null terminator
	const std::u16string focus_string = manager.get_focus_string();
	usz size = sizeof(u16) * (focus_string.length() + 1);

	// Add candidates, including null terminators and offsets
	for (const ime_jp_manager::candidate& can : manager.get_candidate_list())
	{
		constexpr usz offset_size = sizeof(u16);
		size += offset_size + (can.text.size() + 1) * sizeof(u16);
	}

	*pListSize = ::narrow<s16>(size);

	return CELL_OK;
}

static error_code cellImeJpGetCandidateList(CellImeJpHandle hImeJpHandle, vm::ptr<s16> plistNum, vm::ptr<u16> pCandidateString)
{
	cellImeJp.todo("cellImeJpGetCandidateList(hImeJpHandle=*0x%x, plistNum=*0x%x, pCandidateString=*0x%x)", hImeJpHandle, plistNum, pCandidateString);

	if (!plistNum || !pCandidateString)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state != CELL_IMEJP_CANDIDATES)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	// First, copy the focus string
	u32 pos = 0;
	const std::u16string focus_string = manager.get_focus_string();
	for (u32 i = pos; i < focus_string.length(); i++)
	{
		pCandidateString[i] = focus_string[i];
	}
	pos += ::narrow<u32>(focus_string.length());

	// Add null terminator
	pCandidateString[pos++] = 0;

	// Add list of candidates
	const std::vector<ime_jp_manager::candidate> list = manager.get_candidate_list();
	for (const ime_jp_manager::candidate& can : list)
	{
		// Copy the candidate
		for (u32 i = pos; i < can.text.length(); i++)
		{
			pCandidateString[i] = can.text[i];
		}
		pos += ::narrow<u32>(can.text.length());

		// Add null terminator
		pCandidateString[pos++] = 0;

		// Add offset
		pCandidateString[pos++] = can.offset;
	}

	*plistNum = ::narrow<s16>(list.size());

	return CELL_OK;
}

static error_code cellImeJpGetCandidateSelect(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pIndex)
{
	cellImeJp.todo("cellImeJpGetCandidateSelect(hImeJpHandle=*0x%x, pIndex=*0x%x)", hImeJpHandle, pIndex);

	if (!pIndex)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager.input_state != CELL_IMEJP_CANDIDATES)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	*pIndex = 0;

	return CELL_OK;
}

static error_code cellImeJpGetPredictList(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pYomiString, s32 itemNum, vm::ptr<s32> plistCount, vm::ptr<CellImeJpPredictItem> pPredictItem)
{
	cellImeJp.todo("cellImeJpGetPredictList(hImeJpHandle=*0x%x, pYomiString=*0x%x, itemNum=%d, plistCount=*0x%x, pPredictItem=*0x%x)", hImeJpHandle, pYomiString, itemNum, plistCount, pPredictItem);

	if (!pPredictItem || !plistCount)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	*plistCount = 0;

	return CELL_OK;
}

static error_code cellImeJpConfirmPrediction(CellImeJpHandle hImeJpHandle, vm::ptr<CellImeJpPredictItem> pPredictItem)
{
	cellImeJp.todo("cellImeJpConfirmPrediction(hImeJpHandle=*0x%x, pPredictItem=*0x%x)", hImeJpHandle, pPredictItem);

	if (!pPredictItem)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	auto& manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager.mutex);

	if (!manager.is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

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
