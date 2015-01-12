#include "stdafx.h"

std::string u128::to_hex() const
{
	return fmt::Format("%016llx%016llx", _u64[1], _u64[0]);
}

std::string u128::to_xyzw() const
{
	return fmt::Format("x: %g y: %g z: %g w: %g", _f[3], _f[2], _f[1], _f[0]);
}
