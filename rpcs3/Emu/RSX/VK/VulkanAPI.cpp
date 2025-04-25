#include "stdafx.h"
#include "VulkanAPI.h"

#include "vkutils/device.h"

#define DECLARE_VK_FUNCTION_BODY
#include "VKProcTable.h"

namespace vk
{
	const render_device* get_current_renderer();

	void init()
	{
		auto pdev = get_current_renderer();

		#define VK_FUNC(func) _##func = reinterpret_cast<PFN_##func>(vkGetDeviceProcAddr(*pdev, #func))
		#include "VKProcTable.h"
	}
}
