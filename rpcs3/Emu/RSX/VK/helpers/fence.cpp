#pragma once

#include "fence.h"
#include "shared.h"

namespace vk
{
#ifdef _MSC_VER
	extern "C" void _mm_pause();
#endif

	fence::fence(VkDevice dev)
	{
		owner                  = dev;
		VkFenceCreateInfo info = {};
		info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		CHECK_RESULT(vkCreateFence(dev, &info, nullptr, &handle));
	}

	fence::~fence()
	{
		if (handle)
		{
			vkDestroyFence(owner, handle, nullptr);
			handle = VK_NULL_HANDLE;
		}
	}

	void fence::reset()
	{
		vkResetFences(owner, 1, &handle);
		flushed.release(false);
	}

	void fence::signal_flushed()
	{
		flushed.release(true);
	}

	void fence::wait_flush()
	{
		while (!flushed)
		{
#ifdef _MSC_VER
			_mm_pause();
#else
			__builtin_ia32_pause();
#endif
		}
	}

	fence::operator bool() const
	{
		return (handle != VK_NULL_HANDLE);
	}
}
