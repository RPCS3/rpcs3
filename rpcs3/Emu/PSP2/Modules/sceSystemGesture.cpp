#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceSystemGesture.h"

logs::channel sceSystemGesture("sceSystemGesture", logs::level::notice);

s32 sceSystemGestureInitializePrimitiveTouchRecognizer(vm::ptr<SceSystemGesturePrimitiveTouchRecognizerParameter> parameter)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureFinalizePrimitiveTouchRecognizer()
{
	throw EXCEPTION("");
}

s32 sceSystemGestureResetPrimitiveTouchRecognizer()
{
	throw EXCEPTION("");
}

s32 sceSystemGestureUpdatePrimitiveTouchRecognizer(vm::cptr<SceTouchData> pFrontData, vm::cptr<SceTouchData> pBackData)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureGetPrimitiveTouchEvents(vm::ptr<SceSystemGesturePrimitiveTouchEvent> primitiveEventBuffer, const u32 capacityOfBuffer, vm::ptr<u32> numberOfEvent)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureGetPrimitiveTouchEventsCount()
{
	throw EXCEPTION("");
}

s32 sceSystemGestureGetPrimitiveTouchEventByIndex(const u32 index, vm::ptr<SceSystemGesturePrimitiveTouchEvent> primitiveTouchEvent)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(const u16 primitiveID, vm::ptr<SceSystemGesturePrimitiveTouchEvent> primitiveTouchEvent)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureCreateTouchRecognizer(vm::ptr<SceSystemGestureTouchRecognizer> touchRecognizer, const SceSystemGestureType gestureType, const u8 touchPanelPortID, vm::cptr<SceSystemGestureRectangle> rectangle, vm::cptr<SceSystemGestureTouchRecognizerParameter> touchRecognizerParameter)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureGetTouchRecognizerInformation(vm::cptr<SceSystemGestureTouchRecognizer> touchRecognizer, vm::ptr<SceSystemGestureTouchRecognizerInformation> information)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureResetTouchRecognizer(vm::ptr<SceSystemGestureTouchRecognizer> touchRecognizer)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureUpdateTouchRecognizer(vm::ptr<SceSystemGestureTouchRecognizer> touchRecognizer)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureUpdateTouchRecognizerRectangle(vm::ptr<SceSystemGestureTouchRecognizer> touchRecognizer, vm::cptr<SceSystemGestureRectangle> rectangle)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureGetTouchEvents(vm::cptr<SceSystemGestureTouchRecognizer> touchRecognizer, vm::ptr<SceSystemGestureTouchEvent> eventBuffer, const u32 capacityOfBuffer, vm::ptr<u32> numberOfEvent)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureGetTouchEventsCount(vm::cptr<SceSystemGestureTouchRecognizer> touchRecognizer)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureGetTouchEventByIndex(vm::cptr<SceSystemGestureTouchRecognizer> touchRecognizer, const u32 index, vm::ptr<SceSystemGestureTouchEvent> touchEvent)
{
	throw EXCEPTION("");
}

s32 sceSystemGestureGetTouchEventByEventID(vm::cptr<SceSystemGestureTouchRecognizer> touchRecognizer, const u32 eventID, vm::ptr<SceSystemGestureTouchEvent> touchEvent)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) REG_FNID(SceSystemGesture, nid, name)

DECLARE(arm_module_manager::SceSystemGesture)("SceSystemGesture", []()
{
	REG_FUNC(0x6078A08B, sceSystemGestureInitializePrimitiveTouchRecognizer);
	REG_FUNC(0xFD5A6504, sceSystemGestureResetPrimitiveTouchRecognizer);
	REG_FUNC(0xB3875104, sceSystemGestureFinalizePrimitiveTouchRecognizer);
	REG_FUNC(0xDF4C665A, sceSystemGestureUpdatePrimitiveTouchRecognizer);
	REG_FUNC(0xC750D3DA, sceSystemGestureGetPrimitiveTouchEvents);
	REG_FUNC(0xBAB8ECCB, sceSystemGestureGetPrimitiveTouchEventsCount);
	REG_FUNC(0xE0577765, sceSystemGestureGetPrimitiveTouchEventByIndex);
	REG_FUNC(0x480564C9, sceSystemGestureGetPrimitiveTouchEventByPrimitiveID);
	REG_FUNC(0xC3367370, sceSystemGestureCreateTouchRecognizer);
	REG_FUNC(0xF0DB1AE5, sceSystemGestureGetTouchRecognizerInformation);
	REG_FUNC(0x0D941B90, sceSystemGestureResetTouchRecognizer);
	REG_FUNC(0x851FB144, sceSystemGestureUpdateTouchRecognizer);
	REG_FUNC(0xA9DB29F6, sceSystemGestureUpdateTouchRecognizerRectangle);
	REG_FUNC(0x789D867C, sceSystemGestureGetTouchEvents);
	REG_FUNC(0x13AD2218, sceSystemGestureGetTouchEventsCount);
	REG_FUNC(0x74724147, sceSystemGestureGetTouchEventByIndex);
	REG_FUNC(0x5570B83E, sceSystemGestureGetTouchEventByEventID);
});
