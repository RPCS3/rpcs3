#pragma once

typedef s32(SceImeCharFilter)(u16 ch);

struct SceImeRect
{
	u32 x;
	u32 y;
	u32 width;
	u32 height;
};

struct SceImeEditText
{
	u32 preeditIndex;
	u32 preeditLength;
	u32 caretIndex;
	vm::psv::ptr<u16> str;
};

union SceImeEventParam
{
	SceImeRect rect;
	SceImeEditText text;
	u32 caretIndex;
};

struct SceImeEvent
{
	u32 id;
	SceImeEventParam param;
};

struct SceImeCaret
{
	u32 x;
	u32 y;
	u32 height;
	u32 index;
};

struct SceImePreeditGeometry
{
	u32 x;
	u32 y;
	u32 height;
};

typedef void(SceImeEventHandler)(vm::psv::ptr<void> arg, vm::psv::ptr<const SceImeEvent> e);

struct SceImeParam
{
	u32 size;
	u32 inputMethod;
	u64 supportedLanguages;
	s32 languagesForced;
	u32 type;
	u32 option;
	vm::psv::ptr<void> work;
	vm::psv::ptr<void> arg;
	vm::psv::ptr<SceImeEventHandler> handler;
	vm::psv::ptr<SceImeCharFilter> filter;
	vm::psv::ptr<u32> initialText;
	u32 maxTextLength;
	vm::psv::ptr<u32> inputTextBuffer;
	u32 reserved0;
	u32 reserved1;
};

extern psv_log_base sceIme;
