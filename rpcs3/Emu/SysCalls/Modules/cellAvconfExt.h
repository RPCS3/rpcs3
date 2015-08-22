#pragma once

namespace vm { using namespace ps3; }

struct avconfext_t
{
	u8 gamma;

	avconfext_t()
	{
		gamma = 1;
	}
};