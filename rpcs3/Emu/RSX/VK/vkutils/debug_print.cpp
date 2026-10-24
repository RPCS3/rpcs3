#include "debug_print.h"
#include "device.h"

namespace vk
{
	void push_debug_label_ex(VkCommandBuffer cmd, DebugPrintLevel level, const std::string& message)
	{
		static const std::array<color4f, 3> s_message_colors =
		{
			color4f{ 0.5f, 0.5f, 1.f,  1.f },  // INFO
			color4f{ 1.f,  1.f,  0.5f, 1.f },  // WARN
			color4f{ 1.f,  0.5f, 0.5f, 1.f },  // ERROR
		};

		if (message.empty() || !g_render_device->get_debug_utils_support())
		{
			return;
		}

		VkDebugUtilsLabelEXT label_info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		label_info.pLabelName = message.c_str();
		memcpy(label_info.color, s_message_colors.at(static_cast<int>(level)).rgba, sizeof(float) * 4);
		_vkCmdInsertDebugUtilsLabelEXT(cmd, &label_info);
	}
}
