#pragma once

#define GSL_THROW_ON_CONTRACT_VIOLATION

#pragma push_macro("new")
#undef new
#include <gsl/gsl>
#pragma pop_macro("new")
#undef Expects
#undef Ensures
