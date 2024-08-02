#pragma once
#ifdef _WIN32

#include <util/types.hpp>

// Validates that system modules are properly installed
// Only relevant for WIN32
class WIN32_module_verifier
{
	void run_module_verification();

	WIN32_module_verifier() = default;

public:
	static void run();
};

#endif // _WIN32
