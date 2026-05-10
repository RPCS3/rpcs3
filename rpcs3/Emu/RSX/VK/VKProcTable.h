// Wrangler for Vulkan functions.
// TODO: Eventually, we shall declare vulkan with NO_PROTOTYPES and wrap everything here for android multi-driver support.
// For now, we just use it for extensions since we're on VK_1_0

#define VK_DECL_EXTERN(func) extern PFN_##func _##func
#define VK_DECL_LOCAL(func) PFN_##func _##func

#if defined(DECLARE_VK_FUNCTION_HEADER)
#define VK_FUNC VK_DECL_EXTERN
#elif defined(DECLARE_VK_FUNCTION_BODY)
#define VK_FUNC VK_DECL_LOCAL
#elif !defined(VK_FUNC)
#error "VK_FUNC is not defined"
#endif

// EXT_conditional_rendering
VK_FUNC(vkCmdBeginConditionalRenderingEXT);
VK_FUNC(vkCmdEndConditionalRenderingEXT);

// EXT_debug_utils
VK_FUNC(vkSetDebugUtilsObjectNameEXT);
VK_FUNC(vkQueueInsertDebugUtilsLabelEXT);
VK_FUNC(vkCmdInsertDebugUtilsLabelEXT);

// KHR_synchronization2
VK_FUNC(vkCmdSetEvent2KHR);
VK_FUNC(vkCmdWaitEvents2KHR);
VK_FUNC(vkCmdPipelineBarrier2KHR);

// EXT_device_fault
VK_FUNC(vkGetDeviceFaultInfoEXT);

// EXT_multi_draw
VK_FUNC(vkCmdDrawMultiEXT);
VK_FUNC(vkCmdDrawMultiIndexedEXT);

// EXT_external_memory_host
VK_FUNC(vkGetMemoryHostPointerPropertiesEXT);

#undef VK_FUNC
#undef DECLARE_VK_FUNCTION_HEADER
#undef DECLARE_VK_FUNCTION_BODY
#undef VK_DECL_EXTERN
#undef VK_DECL_LOCAL
