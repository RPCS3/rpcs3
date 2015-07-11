#pragma once

#include "sceTouch.h"

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
	le_t<s16> x;
	le_t<s16> y;
};

struct SceSystemGestureRectangle
{
	le_t<s16> x;
	le_t<s16> y;
	le_t<s16> width;
	le_t<s16> height;
};

struct SceSystemGesturePrimitiveTouchEvent
{
	le_t<s32> eventState; // SceSystemGestureTouchState
	le_t<u16> primitiveID;
	SceSystemGestureVector2 pressedPosition;
	le_t<s16> pressedForce;
	SceSystemGestureVector2 currentPosition;
	le_t<s16> currentForce;
	SceSystemGestureVector2 deltaVector;
	le_t<s16> deltaForce;
	le_t<u64> deltaTime;
	le_t<u64> elapsedTime;
	u8 reserved[56];
};

struct SceSystemGesturePrimitiveTouchRecognizerParameter
{
	u8 reserved[64];
};

struct SceSystemGestureTouchRecognizer
{
	le_t<u64> reserved[307];
};

struct SceSystemGestureTouchRecognizerInformation
{
	le_t<s32> gestureType; // SceSystemGestureType
	le_t<u32> touchPanelPortID;
	SceSystemGestureRectangle rectangle;
	le_t<u64> updatedTime;
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
	le_t<u64> timeToInvokeEvent;
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
	le_t<u16> primitiveID;
	SceSystemGestureVector2 position;
	u8 tappedCount;
	u8 reserved[57];
};

struct SceSystemGestureDragEventProperty
{
	le_t<u16> primitiveID;
	SceSystemGestureVector2 deltaVector;
	SceSystemGestureVector2 currentPosition;
	SceSystemGestureVector2 pressedPosition;
	u8 reserved[50];
};

struct SceSystemGestureTapAndHoldEventProperty
{
	le_t<u16> primitiveID;
	SceSystemGestureVector2 pressedPosition;
	u8 reserved[58];
};

struct SceSystemGesturePinchOutInEventProperty
{
	le_t<float> scale;

	struct _primitive_t
	{
		le_t<u16> primitiveID;
		SceSystemGestureVector2 currentPosition;
		SceSystemGestureVector2 deltaVector;
		SceSystemGestureVector2 pairedPosition;
	};

	_primitive_t primitive[2];
	u8 reserved[32];
};

struct SceSystemGestureTouchEvent
{
	le_t<u32> eventID;
	le_t<s32> eventState; // SceSystemGestureTouchState
	le_t<s32> gestureType; // SceSystemGestureType
	u8 padding[4];
	le_t<u64> updatedTime;

	union _property_t
	{
		u8 propertyBuf[64];
		SceSystemGestureTapEventProperty tap;
		SceSystemGestureDragEventProperty drag;
		SceSystemGestureTapAndHoldEventProperty tapAndHold;
		SceSystemGesturePinchOutInEventProperty pinchOutIn;
	};

	_property_t property;
	u8 reserved[56];
};

extern psv_log_base sceSystemGesture;
