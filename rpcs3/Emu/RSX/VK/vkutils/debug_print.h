#pragma once
#include "../VulkanAPI.h"
#include "Utilities/StrFmt.h"

namespace vk
{
	enum DebugPrintLevel
	{
		DBG_LEVEL_INFO = 0,
		DBG_LEVEL_WARN,
		DBG_LEVEL_ERROR
	};

	void push_debug_label_ex(VkCommandBuffer cmd, DebugPrintLevel level, const std::string& message);

	// Push a debug label into the command stream. The debug labels can only be viewed from tracing tools like Renderdoc.
	template <typename CharT, usz N, typename... Args>
	void push_debug_label(VkCommandBuffer cmd, DebugPrintLevel level, const CharT(&fmt)[N], const Args&... args)
	{
		push_debug_label_ex(cmd, level, fmt::format(fmt, args...));
	}
}

using enum vk::DebugPrintLevel;
