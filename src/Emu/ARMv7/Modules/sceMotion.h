#pragma once

struct SceMotionState
{
	le_t<u32> timestamp;
	SceFVector3 acceleration;
	SceFVector3 angularVelocity;
	u8 reserve1[12];
	SceFQuaternion deviceQuat;
	SceUMatrix4 rotationMatrix;
	SceUMatrix4 nedMatrix;
	u8 reserve2[4];
	SceFVector3 basicOrientation;
	le_t<u64> hostTimestamp;
	u8 reserve3[40];
};

struct SceMotionSensorState
{
	SceFVector3 accelerometer;
	SceFVector3 gyro;
	u8 reserve1[12];
	le_t<u32> timestamp;
	le_t<u32> counter;
	u8 reserve2[4];
	le_t<u64> hostTimestamp;
	u8 reserve3[8];
};

extern psv_log_base sceMotion;
