#pragma once

#include "Emu/Io/PadHandler.h"

// All class declarations are in the .mm file to reduce poisoning C++ with Objective-C
// syntax
extern std::shared_ptr<PadHandlerBase> create_GameController_handler();
