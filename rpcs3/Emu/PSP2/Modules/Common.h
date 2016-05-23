#pragma once

#include "Utilities/types.h"
#include "Utilities/BEType.h"

struct SceDateTime
{
	le_t<u16> year;
	le_t<u16> month;
	le_t<u16> day;
	le_t<u16> hour;
	le_t<u16> minute;
	le_t<u16> second;
	le_t<u32> microsecond;
};

struct SceFVector3
{
	le_t<f32> x, y, z;
};

struct SceFQuaternion
{
	le_t<f32> x, y, z, w;
};

union SceUMatrix4
{
	struct
	{
		le_t<f32> f[4][4];
	};

	struct
	{
		le_t<s32> i[4][4];
	};
};
