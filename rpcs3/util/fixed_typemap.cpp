#include "fixed_typemap.hpp"

#include <algorithm>

namespace stx::detail
{
	void destroy_info::sort_by_reverse_creation_order(destroy_info* begin, destroy_info* end)
	{
		std::sort(begin, end, [](const destroy_info& a, const destroy_info& b)
		{
			// Destroy order is the inverse of creation order
			return a.created > b.created;
		});
	}
}
