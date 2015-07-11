#pragma once

using SceImeCharFilter = func_def<s32(u16 ch)>;

struct SceImeRect
{
	le_t<u32> x;
	le_t<u32> y;
	le_t<u32> width;
	le_t<u32> height;
};

struct SceImeEditText
{
	le_t<u32> preeditIndex;
	le_t<u32> preeditLength;
	le_t<u32> caretIndex;
	vm::lptr<u16> str;
};

union SceImeEventParam
{
	SceImeRect rect;
	SceImeEditText text;
	le_t<u32> caretIndex;
};

struct SceImeEvent
{
	le_t<u32> id;
	SceImeEventParam param;
};

struct SceImeCaret
{
	le_t<u32> x;
	le_t<u32> y;
	le_t<u32> height;
	le_t<u32> index;
};

struct SceImePreeditGeometry
{
	le_t<u32> x;
	le_t<u32> y;
	le_t<u32> height;
};

using SceImeEventHandler = func_def<void(vm::ptr<void> arg, vm::cptr<SceImeEvent> e)>;

struct SceImeParam
{
	le_t<u32> size;
	le_t<u32> inputMethod;
	le_t<u64> supportedLanguages;
	le_t<s32> languagesForced;
	le_t<u32> type;
	le_t<u32> option;
	vm::lptr<void> work;
	vm::lptr<void> arg;
	vm::lptr<SceImeEventHandler> handler;
	vm::lptr<SceImeCharFilter> filter;
	vm::lptr<u32> initialText;
	le_t<u32> maxTextLength;
	vm::lptr<u32> inputTextBuffer;
	le_t<u32> reserved0;
	le_t<u32> reserved1;
};

extern psv_log_base sceIme;
