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
	if (!c || cursor >= (CELL_IMEJP_STRING_MAXLENGTH - 1) || cursor > input_string.length())
		return false;

	std::u16string tmp;
	tmp += c;
	input_string.insert(cursor++, tmp);
	cursor_end = cursor;
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
	if (!cursor || cursor > CELL_IMEJP_STRING_MAXLENGTH || cursor > input_string.length())
		return false;

	input_string.erase(--cursor);
	cursor_end = cursor;

	if (input_string.empty())
		input_state = CELL_IMEJP_BEFORE_INPUT;

	return true;
}

bool ime_jp_manager::deleteWord()
{
	if (cursor >= (CELL_IMEJP_STRING_MAXLENGTH - 1) || cursor > (input_string.length() - 1))
		return false;

	input_string.erase(cursor);
	cursor_end = cursor;

	if (input_string.empty())
		input_state = CELL_IMEJP_BEFORE_INPUT;

	return true;
}

void ime_jp_manager::moveCursor(s8 amount)
{
	if (amount > 0)
	{
		cursor = std::min(cursor + amount, input_string.length() - 1);
		cursor_end = std::min(cursor_end + amount, input_string.length() - 1);
	}
	else if (amount < 0)
	{
		cursor = std::max(0, static_cast<s32>(cursor) + amount);
		cursor_end = std::max(static_cast<s32>(cursor), static_cast<s32>(cursor_end) + amount);
	}
}

void ime_jp_manager::moveCursorEnd(s8 amount)
{
	if (amount > 0)
	{
		cursor_end = std::max(static_cast<s32>(cursor), std::min<s32>(static_cast<s32>(cursor_end) + amount, ::narrow<s32>(input_string.length()) - 1));
	}
	else if (amount < 0)
	{
		cursor_end = std::max(static_cast<s32>(cursor), static_cast<s32>(cursor_end) + amount);
	}
}

error_code cellImeJpOpen(sys_memory_container_t container_id, vm::ptr<CellImeJpHandle> hImeJpHandle, vm::cptr<CellImeJpAddDic> addDicPath)
{
	cellImeJp.todo("cellImeJpOpen(container_id=*0x%x, hImeJpHandle=*0x%x, addDicPath=*0x%x)", container_id, hImeJpHandle, addDicPath);

	if (!container_id || !hImeJpHandle)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_ALREADY_OPEN;
	}

	if (addDicPath && addDicPath->path[0])
	{
		cellImeJp.warning("cellImeJpOpen dictionary path = %s", addDicPath->path);

		manager->dictionary_paths.emplace_back(addDicPath->path);
	}
	*hImeJpHandle = vm::cast(ime_jp_address);

	manager->is_initialized = true;

	return CELL_OK;
}

error_code cellImeJpOpen2(sys_memory_container_t container_id, vm::ptr<CellImeJpHandle> hImeJpHandle, vm::cptr<CellImeJpAddDic> addDicPath)
{
	cellImeJp.todo("cellImeJpOpen2(container_id=*0x%x, hImeJpHandle=*0x%x, addDicPath=*0x%x)", container_id, hImeJpHandle, addDicPath);

	if (!container_id || !hImeJpHandle)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_ALREADY_OPEN;
	}

	if (addDicPath && addDicPath->path[0])
	{
		cellImeJp.warning("cellImeJpOpen2 dictionary path = %s", addDicPath->path);

		manager->dictionary_paths.emplace_back(addDicPath->path);
	}

	*hImeJpHandle = vm::cast(ime_jp_address);

	manager->is_initialized = true;

	return CELL_OK;
}

error_code cellImeJpOpen3(sys_memory_container_t container_id, vm::ptr<CellImeJpHandle> hImeJpHandle, vm::cpptr<CellImeJpAddDic> addDicPath)
{
	cellImeJp.todo("cellImeJpOpen3(container_id=*0x%x, hImeJpHandle=*0x%x, addDicPath=*0x%x)", container_id, hImeJpHandle, addDicPath);

	if (!container_id || !hImeJpHandle)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (manager->is_initialized)
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

				manager->dictionary_paths.emplace_back(addDicPath[i]->path);
			}
		}
	}

	*hImeJpHandle = vm::cast(ime_jp_address);

	manager->is_initialized = true;

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

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	manager->input_state = CELL_IMEJP_BEFORE_INPUT;
	manager->input_string.clear();
	manager->converted_string.clear();
	manager->confirmed_string.clear();
	manager->cursor = 0;
	manager->cursor_end = 0;
	manager->is_initialized = false;

	return CELL_OK;
}

error_code cellImeJpSetKanaInputMode(CellImeJpHandle hImeJpHandle, s16 inputOption)
{
	cellImeJp.todo("cellImeJpSetKanaInputMode(hImeJpHandle=*0x%x, inputOption=%d)", hImeJpHandle, inputOption);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state != CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->kana_input_mode = inputOption;

	return CELL_OK;
}

error_code cellImeJpSetInputCharType(CellImeJpHandle hImeJpHandle, s16 charTypeOption)
{
	cellImeJp.todo("cellImeJpSetInputCharType(hImeJpHandle=*0x%x, charTypeOption=%d)", hImeJpHandle, charTypeOption);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	manager->input_char_type = charTypeOption;

	return CELL_OK;
}

error_code cellImeJpSetFixInputMode(CellImeJpHandle hImeJpHandle, s16 fixInputMode)
{
	cellImeJp.todo("cellImeJpSetFixInputMode(hImeJpHandle=*0x%x, fixInputMode=%d)", hImeJpHandle, fixInputMode);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	manager->fix_input_mode = fixInputMode;

	return CELL_OK;
}

error_code cellImeJpAllowExtensionCharacters(CellImeJpHandle hImeJpHandle, s16 extensionCharacters)
{
	cellImeJp.todo("cellImeJpSetFixInputMode(hImeJpHandle=*0x%x, extensionCharacters=%d)", hImeJpHandle, extensionCharacters);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state != CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->allowed_extensions = extensionCharacters;

	return CELL_OK;
}

error_code cellImeJpReset(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpReset(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	manager->input_state = CELL_IMEJP_BEFORE_INPUT;
	manager->input_string.clear();
	manager->converted_string.clear();
	manager->confirmed_string.clear();
	manager->cursor = 0;
	manager->cursor_end = 0;

	return CELL_OK;
}

error_code cellImeJpGetStatus(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pInputStatus)
{
	cellImeJp.warning("cellImeJpGetStatus(hImeJpHandle=*0x%x, pInputStatus=%d)", hImeJpHandle, pInputStatus);

	if (!pInputStatus)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	*pInputStatus = manager->input_state;

	return CELL_OK;
}

error_code cellImeJpEnterChar(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterChar(hImeJpHandle=*0x%x, inputChar=%d, pOutputStatus=%d)", hImeJpHandle, inputChar, pOutputStatus);

	if (!pOutputStatus)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_MOVE_CLAUSE_GAP)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->addChar(inputChar);

	*pOutputStatus = CELL_IMEJP_RET_CONFIRMED;

	return CELL_OK;
}

error_code cellImeJpEnterCharExt(CellImeJpHandle hImeJpHandle, u16 inputChar, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterCharExt(hImeJpHandle=*0x%x, inputChar=%d, pOutputStatus=%d", hImeJpHandle, inputChar, pOutputStatus);
	return cellImeJpEnterChar(hImeJpHandle, inputChar, pOutputStatus);
}

error_code cellImeJpEnterString(CellImeJpHandle hImeJpHandle, vm::cptr<u16> pInputString, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterString(hImeJpHandle=*0x%x, pInputString=*0x%x, pOutputStatus=%d", hImeJpHandle, pInputString, pOutputStatus);

	if (!pOutputStatus)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_MOVE_CLAUSE_GAP)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->addString(pInputString);

	*pOutputStatus = CELL_IMEJP_RET_CONFIRMED;

	return CELL_OK;
}

error_code cellImeJpEnterStringExt(CellImeJpHandle hImeJpHandle, vm::cptr<u16> pInputString, vm::ptr<s16> pOutputStatus)
{
	cellImeJp.todo("cellImeJpEnterStringExt(hImeJpHandle=*0x%x, pInputString=*0x%x, pOutputStatus=%d", hImeJpHandle, pInputString, pOutputStatus);
	return cellImeJpEnterString(hImeJpHandle, pInputString, pOutputStatus);
}

error_code cellImeJpModeCaretRight(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpModeCaretRight(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state != CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->moveCursor(1);

	return CELL_OK;
}

error_code cellImeJpModeCaretLeft(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpModeCaretLeft(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state != CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->moveCursor(-1);

	return CELL_OK;
}

error_code cellImeJpBackspaceWord(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpBackspaceWord(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state != CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->backspaceWord();

	return CELL_OK;
}

error_code cellImeJpDeleteWord(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpDeleteWord(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state != CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->deleteWord();

	return CELL_OK;
}

error_code cellImeJpAllDeleteConvertString(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpAllDeleteConvertString(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->cursor = 0;
	manager->cursor_end = 0;
	manager->input_string.clear();
	manager->converted_string.clear();
	manager->input_state = CELL_IMEJP_BEFORE_INPUT;

	return CELL_OK;
}

error_code cellImeJpConvertForward(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpConvertForward(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->input_state = CELL_IMEJP_CANDIDATES;

	return CELL_OK;
}

error_code cellImeJpConvertBackward(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpConvertBackward(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->input_state = CELL_IMEJP_CANDIDATES;

	return CELL_OK;
}

error_code cellImeJpCurrentPartConfirm(CellImeJpHandle hImeJpHandle, s16 listItem)
{
	cellImeJp.todo("cellImeJpCurrentPartConfirm(hImeJpHandle=*0x%x, listItem=%d)", hImeJpHandle, listItem);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	return CELL_OK;
}

error_code cellImeJpAllConfirm(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpAllConfirm(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	// Use input_string for now
	manager->confirmed_string = manager->input_string;
	manager->cursor = 0;
	manager->cursor_end = 0;
	manager->input_string.clear();
	manager->converted_string.clear();
	manager->input_state = CELL_IMEJP_BEFORE_INPUT;

	return CELL_OK;
}

error_code cellImeJpAllConvertCancel(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpAllConvertCancel(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT || manager->input_state == CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->converted_string.clear();
	manager->input_state = CELL_IMEJP_BEFORE_CONVERT;

	return CELL_OK;
}

error_code cellImeJpConvertCancel(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpConvertCancel(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT || manager->input_state == CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	// TODO: only cancel all if cursor is at 0
	return cellImeJpAllConvertCancel(hImeJpHandle);
}

error_code cellImeJpExtendConvertArea(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpExtendConvertArea(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT || manager->input_state == CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->moveCursorEnd(1);

	return CELL_OK;
}

error_code cellImeJpShortenConvertArea(CellImeJpHandle hImeJpHandle)
{
	cellImeJp.todo("cellImeJpShortenConvertArea(hImeJpHandle=*0x%x)", hImeJpHandle);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT || manager->input_state == CELL_IMEJP_BEFORE_CONVERT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	manager->moveCursorEnd(-1);

	return CELL_OK;
}

error_code cellImeJpTemporalConfirm(CellImeJpHandle hImeJpHandle, s16 selectIndex)
{
	cellImeJp.todo("cellImeJpTemporalConfirm(hImeJpHandle=*0x%x, selectIndex=%d)", hImeJpHandle, selectIndex);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state != CELL_IMEJP_CANDIDATES)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	return CELL_OK;
}

error_code cellImeJpPostConvert(CellImeJpHandle hImeJpHandle, s16 postType)
{
	cellImeJp.todo("cellImeJpPostConvert(hImeJpHandle=*0x%x, postType=%d)", hImeJpHandle, postType);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	return CELL_OK;
}

error_code cellImeJpMoveFocusClause(CellImeJpHandle hImeJpHandle, s16 moveType)
{
	cellImeJp.todo("cellImeJpMoveFocusClause(hImeJpHandle=*0x%x, moveType=%d)", hImeJpHandle, moveType);

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state == CELL_IMEJP_BEFORE_INPUT || manager->input_state == CELL_IMEJP_BEFORE_CONVERT || manager->input_state == CELL_IMEJP_MOVE_CLAUSE_GAP)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	switch (moveType)
	{
	case CELL_IMEJP_FOCUS_NEXT:
		manager->moveCursor(1);
		break;
	case CELL_IMEJP_FOCUS_BEFORE:
		manager->moveCursor(-1);
		break;
	case CELL_IMEJP_FOCUS_TOP:
		manager->moveCursor(-1 * ::narrow<s8>(manager->input_string.length(), HERE));
		break;
	case CELL_IMEJP_FOCUS_END:
		manager->moveCursor(::narrow<s8>(manager->input_string.length(), HERE));
		manager->moveCursor(-1);
		break;
	default:
		break;
	}

	manager->input_state = CELL_IMEJP_CONVERTING;

	return CELL_OK;
}

error_code cellImeJpGetFocusTop(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pFocusTop)
{
	cellImeJp.todo("cellImeJpGetFocusTop(hImeJpHandle=*0x%x, pFocusTop=*0x%x)", hImeJpHandle, pFocusTop);

	if (!pFocusTop)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	*pFocusTop = static_cast<u16>(manager->cursor * 2); // offset in bytes

	return CELL_OK;
}

error_code cellImeJpGetFocusLength(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pFocusLength)
{
	cellImeJp.todo("cellImeJpGetFocusLength(hImeJpHandle=*0x%x, pFocusLength=*0x%x)", hImeJpHandle, pFocusLength);

	if (!pFocusLength)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->cursor >= (CELL_IMEJP_STRING_MAXLENGTH - 1))
	{
		*pFocusLength = 0;
	}
	else
	{
		*pFocusLength = static_cast<u16>((manager->cursor_end - manager->cursor + 1) * 2); // offset in bytes
	}

	return CELL_OK;
}

error_code cellImeJpGetConfirmYomiString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pYomiString)
{
	cellImeJp.todo("cellImeJpGetConfirmYomiString(hImeJpHandle=*0x%x, pYomiString=*0x%x)", hImeJpHandle, pYomiString);

	if (!pYomiString)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	for (u32 i = 0; i < CELL_IMEJP_STRING_MAXLENGTH; i++)
	{
		pYomiString[i] = 0;
	}

	const size_t max_len = std::min<size_t>(CELL_IMEJP_STRING_MAXLENGTH - 1, manager->confirmed_string.length());

	for (u32 i = 0; i < max_len; i++)
	{
		pYomiString[i] = manager->confirmed_string[i];
	}

	return CELL_OK;
}

error_code cellImeJpGetConfirmString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pConfirmString)
{
	cellImeJp.todo("cellImeJpGetConfirmString(hImeJpHandle=*0x%x, pConfirmString=*0x%x)", hImeJpHandle, pConfirmString);

	if (!pConfirmString)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	for (u32 i = 0; i < CELL_IMEJP_STRING_MAXLENGTH; i++)
	{
		pConfirmString[i] = 0;
	}

	const size_t max_len = std::min<size_t>(CELL_IMEJP_STRING_MAXLENGTH - 1, manager->confirmed_string.length());

	for (u32 i = 0; i < max_len; i++)
	{
		pConfirmString[i] = manager->confirmed_string[i];
	}

	return CELL_OK;
}

error_code cellImeJpGetConvertYomiString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pYomiString)
{
	cellImeJp.todo("cellImeJpGetConvertYomiString(hImeJpHandle=*0x%x, pYomiString=*0x%x)", hImeJpHandle, pYomiString);

	if (!pYomiString)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	for (u32 i = 0; i < CELL_IMEJP_STRING_MAXLENGTH; i++)
	{
		pYomiString[i] = 0;
	}

	const size_t max_len = std::min<size_t>(CELL_IMEJP_STRING_MAXLENGTH - 1, manager->input_string.length());

	for (u32 i = 0; i < max_len; i++)
	{
		pYomiString[i] = manager->input_string[i];
	}

	return CELL_OK;
}

error_code cellImeJpGetConvertString(CellImeJpHandle hImeJpHandle, vm::ptr<u16> pConvertString)
{
	cellImeJp.warning("cellImeJpGetConvertString(hImeJpHandle=*0x%x, pConvertString=*0x%x)", hImeJpHandle, pConvertString);

	if (!pConvertString)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	for (u32 i = 0; i < CELL_IMEJP_STRING_MAXLENGTH; i++)
	{
		pConvertString[i] = 0;
	}

	const size_t max_len = std::min<size_t>(CELL_IMEJP_STRING_MAXLENGTH - 1, manager->input_string.length());

	for (u32 i = 0; i < max_len; i++)
	{
		pConvertString[i] = manager->input_string[i];
	}

	return CELL_OK;
}

error_code cellImeJpGetCandidateListSize(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pListSize)
{
	cellImeJp.todo("cellImeJpGetCandidateListSize(hImeJpHandle=*0x%x, pListSize=*0x%x)", hImeJpHandle, pListSize);

	if (!pListSize)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state != CELL_IMEJP_CANDIDATES)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	*pListSize = 0;

	return CELL_OK;
}

error_code cellImeJpGetCandidateList(CellImeJpHandle hImeJpHandle, vm::ptr<s16> plistNum, vm::ptr<u16> pCandidateString)
{
	cellImeJp.todo("cellImeJpGetCandidateList(hImeJpHandle=*0x%x, plistNum=*0x%x, pCandidateString=*0x%x)", hImeJpHandle, plistNum, pCandidateString);

	if (!plistNum || !pCandidateString)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state != CELL_IMEJP_CANDIDATES)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	*plistNum = 0;

	return CELL_OK;
}

error_code cellImeJpGetCandidateSelect(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pIndex)
{
	cellImeJp.todo("cellImeJpGetCandidateSelect(hImeJpHandle=*0x%x, pIndex=*0x%x)", hImeJpHandle, pIndex);

	if (!pIndex)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	if (manager->input_state != CELL_IMEJP_CANDIDATES)
	{
		return CELL_IMEJP_ERROR_ERR;
	}

	*pIndex = 0;

	return CELL_OK;
}

error_code cellImeJpGetPredictList(CellImeJpHandle hImeJpHandle, vm::ptr<s16> pYomiString, s32 itemNum, vm::ptr<s32> plistCount, vm::ptr<CellImeJpPredictItem> pPredictItem)
{
	cellImeJp.todo("cellImeJpGetPredictList(hImeJpHandle=*0x%x, pYomiString=*0x%x, itemNum=%d, plistCount=*0x%x, pPredictItem=*0x%x)", hImeJpHandle, pYomiString, itemNum, plistCount, pPredictItem);

	if (!pPredictItem || !plistCount)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
	{
		return CELL_IMEJP_ERROR_CONTEXT;
	}

	*plistCount = 0;

	return CELL_OK;
}

error_code cellImeJpConfirmPrediction(CellImeJpHandle hImeJpHandle, vm::ptr<CellImeJpPredictItem> pPredictItem)
{
	cellImeJp.todo("cellImeJpConfirmPrediction(hImeJpHandle=*0x%x, pPredictItem=*0x%x)", hImeJpHandle, pPredictItem);

	if (!pPredictItem)
	{
		return CELL_IMEJP_ERROR_PARAM;
	}

	const auto manager = g_fxo->get<ime_jp_manager>();
	std::lock_guard lock(manager->mutex);

	if (!manager->is_initialized)
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
