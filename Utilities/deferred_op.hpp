#pragma once

// Generic deferred routine wrapper
// Use-case is similar to "defer" statement in other languages, just invokes a callback when the object goes out of scope

#include <functional>

namespace utils
{
	template <typename F>
		requires std::is_invocable_v<F>
	class deferred_op
	{
	public:
		deferred_op(F&& callback)
			: m_callback(callback)
		{}

		~deferred_op()
		{
			m_callback();
		}

	private:
		F m_callback;
	};
}
