#include "stdafx.h"
#include "events.h"

namespace rpcs3
{
	event<void> oninit;
	event<void> onstart;
	event<void> onstop;
	event<void> onpause;
}
