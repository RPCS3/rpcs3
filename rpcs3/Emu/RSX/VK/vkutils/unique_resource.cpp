#include "unique_resource.h"
#include <util/atomic.hpp>

namespace vk
{
	static atomic_t<u64> s_resource_uid;

	u64 gen_uid()
	{
		return s_resource_uid++;
	}
}
