#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceTouch.h"

extern psv_log_base sceSystemGesture;

enum SceSystemGestureTouchState : s32
{
	SCE_SYSTEM_GESTURE_TOUCH_STATE_INACTIVE = 0,
	SCE_SYSTEM_GESTURE_TOUCH_STATE_BEGIN = 1,
	SCE_SYSTEM_GESTURE_TOUCH_STATE_ACTIVE = 2,
	SCE_SYSTEM_GESTURE_TOUCH_STATE_END = 3
};

enum SceSystemGestureType : s32
{
	SCE_SYSTEM_GESTURE_TYPE_TAP = 1,
	SCE_SYSTEM_GESTURE_TYPE_DRAG = 2,
	SCE_SYSTEM_GESTURE_TYPE_TAP_AND_HOLD = 4,
	SCE_SYSTEM_GESTURE_TYPE_PINCH_OUT_IN = 8
};

struct SceSystemGestureVector2
{
	s16 x;
	s16 y;
};

struct SceSystemGestureRectangle
{
	s16 x;
	s16 y;
	s16 width;
	s16 height;
};

struct SceSystemGesturePrimitiveTouchEvent
{
	SceSystemGestureTouchState eventState;
	u16 primitiveID;
	SceSystemGestureVector2 pressedPosition;
	s16 pressedForce;
	SceSystemGestureVector2 currentPosition;
	s16 currentForce;
	SceSystemGestureVector2 deltaVector;
	s16 deltaForce;
	u64 deltaTime;
	u64 elapsedTime;
	u8 reserved[56];
};

struct SceSystemGesturePrimitiveTouchRecognizerParameter
{
	u8 reserved[64];
};

struct SceSystemGestureTouchRecognizer
{
	u64 reserved[307];
};

struct SceSystemGestureTouchRecognizerInformation
{
	SceSystemGestureType gestureType;
	u32 touchPanelPortID;
	SceSystemGestureRectangle rectangle;
	u64 updatedTime;
	u8 reserved[256];
};

struct SceSystemGestureTapRecognizerParameter
{
	u8 maxTapCount;
	u8 reserved[63];
};

struct SceSystemGestureDragRecognizerParameter
{
	u8 reserved[64];
};

struct SceSystemGestureTapAndHoldRecognizerParameter
{
	u64 timeToInvokeEvent;
	u8 reserved[56];
};

struct SceSystemGesturePinchOutInRecognizerParameter
{
	u8 reserved[64];
};

union SceSystemGestureTouchRecognizerParameter
{
	u8 parameterBuf[64];
	SceSystemGestureTapRecognizerParameter tap;
	SceSystemGestureDragRecognizerParameter drag;
	SceSystemGestureTapAndHoldRecognizerParameter tapAndHold;
	SceSystemGesturePinchOutInRecognizerParameter pinchOutIn;
};

struct SceSystemGestureTapEventProperty
{
	u16 primitiveID;
	SceSystemGestureVector2 position;
	u8 tappedCount;
	u8 reserved[57];
};

struct SceSystemGestureDragEventProperty
{
	u16 primitiveID;
	SceSystemGestureVector2 deltaVector;
	SceSystemGestureVector2 currentPosition;
	SceSystemGestureVector2 pressedPosition;
	u8 reserved[50];
};

struct SceSystemGestureTapAndHoldEventProperty
{
	u16 primitiveID;
	SceSystemGestureVector2 pressedPosition;
	u8 reserved[58];
};

struct SceSystemGesturePinchOutInEventProperty
{
	float scale;

	struct
	{
		u16 primitiveID;
		SceSystemGestureVector2 currentPosition;
		SceSystemGestureVector2 deltaVector;
		SceSystemGestureVector2 pairedPosition;
	} primitive[2];

	u8 reserved[32];
};

struct SceSystemGestureTouchEvent
{
	u32 eventID;
	SceSystemGestureTouchState eventState;
	SceSystemGestureType gestureType;
	u8 padding[4];
	u64 updatedTime;

	union
	{
		u8 propertyBuf[64];
		SceSystemGestureTapEventProperty tap;
		SceSystemGestureDragEventProperty drag;
		SceSystemGestureTapAndHoldEventProperty tapAndHold;
		SceSystemGesturePinchOutInEventProperty pinchOutIn;
	} property;

	u8 reserved[56];
};

s32 sceSystemGestureInitializePrimitiveTouchRecognizer(vm::psv::ptr<SceSystemGesturePrimitiveTouchRecognizerParameter> parameter)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureFinalizePrimitiveTouchRecognizer()
{
	throw __FUNCTION__;
}

s32 sceSystemGestureResetPrimitiveTouchRecognizer()
{
	throw __FUNCTION__;
}

s32 sceSystemGestureUpdatePrimitiveTouchRecognizer(vm::psv::ptr<const SceTouchData> pFrontData, vm::psv::ptr<const SceTouchData> pBackData)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureGetPrimitiveTouchEvents(vm::psv::ptr<SceSystemGesturePrimitiveTouchEvent> primitiveEventBuffer, const u32 capacityOfBuffer, vm::psv::ptr<u32> numberOfEvent)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureGetPrimitiveTouchEventsCount()
{
	throw __FUNCTION__;
}

s32 sceSystemGestureGetPrimitiveTouchEventByIndex(const u32 index, vm::psv::ptr<SceSystemGesturePrimitiveTouchEvent> primitiveTouchEvent)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(const u16 primitiveID, vm::psv::ptr<SceSystemGesturePrimitiveTouchEvent> primitiveTouchEvent)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureCreateTouchRecognizer(vm::psv::ptr<SceSystemGestureTouchRecognizer> touchRecognizer, const SceSystemGestureType gestureType, const u8 touchPanelPortID, vm::psv::ptr<const SceSystemGestureRectangle> rectangle, vm::psv::ptr<const SceSystemGestureTouchRecognizerParameter> touchRecognizerParameter)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureGetTouchRecognizerInformation(vm::psv::ptr<const SceSystemGestureTouchRecognizer> touchRecognizer, vm::psv::ptr<SceSystemGestureTouchRecognizerInformation> information)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureResetTouchRecognizer(vm::psv::ptr<SceSystemGestureTouchRecognizer> touchRecognizer)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureUpdateTouchRecognizer(vm::psv::ptr<SceSystemGestureTouchRecognizer> touchRecognizer)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureUpdateTouchRecognizerRectangle(vm::psv::ptr<SceSystemGestureTouchRecognizer> touchRecognizer, vm::psv::ptr<const SceSystemGestureRectangle> rectangle)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureGetTouchEvents(vm::psv::ptr<const SceSystemGestureTouchRecognizer> touchRecognizer, vm::psv::ptr<SceSystemGestureTouchEvent> eventBuffer, const u32 capacityOfBuffer, vm::psv::ptr<u32> numberOfEvent)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureGetTouchEventsCount(vm::psv::ptr<const SceSystemGestureTouchRecognizer> touchRecognizer)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureGetTouchEventByIndex(vm::psv::ptr<const SceSystemGestureTouchRecognizer> touchRecognizer, const u32 index, vm::psv::ptr<SceSystemGestureTouchEvent> touchEvent)
{
	throw __FUNCTION__;
}

s32 sceSystemGestureGetTouchEventByEventID(vm::psv::ptr<const SceSystemGestureTouchRecognizer> touchRecognizer, const u32 eventID, vm::psv::ptr<SceSystemGestureTouchEvent> touchEvent)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<name>(nid, &sceSystemGesture, #name, name)

psv_log_base sceSystemGesture("SceSystemGesture", []()
{
	sceSystemGesture.on_load = nullptr;
	sceSystemGesture.on_unload = nullptr;
	sceSystemGesture.on_stop = nullptr;

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
