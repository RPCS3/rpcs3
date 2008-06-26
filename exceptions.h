#pragma once

#include <exception>

namespace YAML
{
	class Exception: public std::exception {};

	class UnknownToken: public Exception {};
	class IllegalBlockEntry: public Exception {};
	class IllegalMapKey: public Exception {};
	class IllegalMapValue: public Exception {};
	class IllegalScalar: public Exception {};
	class IllegalTabInScalar: public Exception {};
}
