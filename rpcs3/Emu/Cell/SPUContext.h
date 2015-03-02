#pragma once

class SPUThread;

struct SPUContext
{
	u128 gpr[128];

	SPUThread& thread;
};
