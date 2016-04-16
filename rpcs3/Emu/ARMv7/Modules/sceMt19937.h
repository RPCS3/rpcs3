#pragma once

struct SceMt19937Context
{
	le_t<u32> count;
	le_t<u32> state[624];
};
