#pragma once

#define GSL_THROW_ON_CONTRACT_VIOLATION

#pragma push_macro("new")
#pragma push_macro("Expects")
#pragma push_macro("Ensures")
#undef new
#undef Expects
#undef Ensures
#include <gsl.h>
#pragma pop_macro("Ensures")
#pragma pop_macro("Expects")
#pragma pop_macro("new")
