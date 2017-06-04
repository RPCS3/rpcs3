#pragma once
#include <common/event.h>

namespace rpcs3
{
	using namespace common;

	extern event<void> oninit;
	extern event<void> onstart;
	extern event<void> onstop;
	extern event<void> onpause;
}