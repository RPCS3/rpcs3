#pragma once

#include <string>

#define YAML_ASSERT(cond) do { if(!(cond)) return "  Assert failed: " #cond; } while(false)

namespace Test
{
	struct TEST {
		TEST(): ok(false) {}
		TEST(bool ok_): ok(ok_) {}
		TEST(const char *error_): ok(false), error(error_) {}
		
		bool ok;
		std::string error;
	};
    
}
