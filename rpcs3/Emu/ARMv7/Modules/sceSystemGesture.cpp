#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceSystemGesture;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceSystemGesture, #name, name)

psv_log_base sceSystemGesture("SceSystemGesture", []()
{
	sceSystemGesture.on_load = nullptr;
	sceSystemGesture.on_unload = nullptr;
	sceSystemGesture.on_stop = nullptr;

	//REG_FUNC(0x6078A08B, sceSystemGestureInitializePrimitiveTouchRecognizer);
	//REG_FUNC(0xFD5A6504, sceSystemGestureResetPrimitiveTouchRecognizer);
	//REG_FUNC(0xB3875104, sceSystemGestureFinalizePrimitiveTouchRecognizer);
	//REG_FUNC(0xDF4C665A, sceSystemGestureUpdatePrimitiveTouchRecognizer);
	//REG_FUNC(0xC750D3DA, sceSystemGestureGetPrimitiveTouchEvents);
	//REG_FUNC(0xBAB8ECCB, sceSystemGestureGetPrimitiveTouchEventsCount);
	//REG_FUNC(0xE0577765, sceSystemGestureGetPrimitiveTouchEventByIndex);
	//REG_FUNC(0x480564C9, sceSystemGestureGetPrimitiveTouchEventByPrimitiveID);
	//REG_FUNC(0xC3367370, sceSystemGestureCreateTouchRecognizer);
	//REG_FUNC(0xF0DB1AE5, sceSystemGestureGetTouchRecognizerInformation);
	//REG_FUNC(0x0D941B90, sceSystemGestureResetTouchRecognizer);
	//REG_FUNC(0x851FB144, sceSystemGestureUpdateTouchRecognizer);
	//REG_FUNC(0xA9DB29F6, sceSystemGestureUpdateTouchRecognizerRectangle);
	//REG_FUNC(0x789D867C, sceSystemGestureGetTouchEvents);
	//REG_FUNC(0x13AD2218, sceSystemGestureGetTouchEventsCount);
	//REG_FUNC(0x74724147, sceSystemGestureGetTouchEventByIndex);
	//REG_FUNC(0x5570B83E, sceSystemGestureGetTouchEventByEventID);
});
