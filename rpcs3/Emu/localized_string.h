#pragma once

#include <string>
#include "localized_string_id.h"

std::string get_localized_string(localized_string_id id, const char* args = "");
std::u32string get_localized_u32string(localized_string_id id, const char* args = "");
