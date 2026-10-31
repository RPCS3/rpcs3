#include "stdafx.h"
#include "openxr_session.h"
#include "openxr_thread.h"
#include "openxr_frame_share.h"
#include "Utilities/Thread.h"

#include <mutex>

namespace openxr
{

// How long to wait before trying again if there is no runtime or headset
static constexpr auto init_retry_delay = std::chrono::seconds(5);

// The OpenXR version macros use C-style casts
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
static std::string get_version_string(XrVersion version)
{
	return fmt::format("%d.%d.%d", XR_VERSION_MAJOR(version), XR_VERSION_MINOR(version), XR_VERSION_PATCH(version));
}

static constexpr XrVersion api_version_1_0 = XR_API_VERSION_1_0;
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

static std::set<std::string> get_supported_extensions()
{
	u32 count = 0;
	if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr)))
	{
		openxr_log.error("xrEnumerateInstanceExtensionProperties failed");
		return {};
	}

	std::vector<XrExtensionProperties> properties(count, {XR_TYPE_EXTENSION_PROPERTIES});
	if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, count, &count, properties.data())))
	{
		openxr_log.error("xrEnumerateInstanceExtensionProperties failed");
		return {};
	}

	std::set<std::string> extensions;
	for (u32 i = 0; i < count; i++)
	{
		extensions.insert(properties[i].extensionName);
	}

	return extensions;
}

bool is_openxr_available()
{
	// Only a found runtime is cached, so that the init retries notice a runtime that is installed later
	static atomic_t<bool> found = false;
	static atomic_t<bool> not_found_logged = false;

	if (found)
	{
		return true;
	}

	u32 extension_count = 0;
	if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extension_count, nullptr)))
	{
		if (!not_found_logged.exchange(true))
		{
			openxr_log.notice("No OpenXR runtime found");
		}
		return false;
	}

	if (!found.exchange(true))
	{
		openxr_log.notice("Found an OpenXR runtime");
	}
	return true;
}

// The OpenXR thread is not part of the emulation and runs while any consumer is active. There is only one, since runtimes give input focus to one session.
static std::mutex g_consumer_mutex; // Protects g_consumers. Also locked by the OpenXR thread.
static std::set<consumer> g_consumers;

static std::mutex g_lifetime_mutex; // Serializes starting and stopping the thread. Never locked by the OpenXR thread.
static std::mutex g_thread_mutex;   // Protects g_thread
static std::unique_ptr<named_thread<session_thread>> g_thread;

static bool has_consumer(consumer c)
{
	std::lock_guard lock(g_consumer_mutex);
	return g_consumers.contains(c);
}

// Has to be called with g_lifetime_mutex
static void start_thread_locked()
{
	std::lock_guard lock(g_thread_mutex);

	if (!g_thread)
	{
		openxr_log.notice("Starting the OpenXR thread");
		g_thread = std::make_unique<named_thread<session_thread>>();
	}
}

// Has to be called with g_lifetime_mutex
static void stop_thread_locked()
{
	std::unique_ptr<named_thread<session_thread>> thread;
	{
		std::lock_guard lock(g_thread_mutex);
		thread = std::move(g_thread);
	}

	if (thread)
	{
		// The session has to be destroyed before the process exits, since some runtimes can't be cleaned up during the exit (e.g. the OpenXR Simulator)
		openxr_log.notice("Stopping the OpenXR thread");
		// Join explicitly (xrWaitFrame and the shutdown can take a while), since the destructor would report the thread as "too sleepy"
		*thread = thread_state::aborting;
		(*thread)();
	}
}

void set_consumer(consumer c, bool active)
{
	// Fast path for the repeated calls of the renderers
	{
		std::lock_guard lock(g_consumer_mutex);
		if (g_consumers.contains(c) == active)
		{
			return;
		}
	}

	std::lock_guard lifetime_lock(g_lifetime_mutex);

	bool empty = false;
	{
		std::lock_guard lock(g_consumer_mutex);

		const bool changed = active ? g_consumers.insert(c).second : g_consumers.erase(c) != 0;
		if (!changed)
		{
			// Someone else changed it in the meantime
			return;
		}

		openxr_log.notice("OpenXR consumer %d %s", static_cast<s32>(c), active ? "added" : "removed");
		empty = g_consumers.empty();
	}

	if (empty)
	{
		stop_thread_locked();
	}
	else
	{
		start_thread_locked();
	}
}

void stop_thread()
{
	std::lock_guard lifetime_lock(g_lifetime_mutex);
	{
		std::lock_guard lock(g_consumer_mutex);
		g_consumers.clear();
	}

	stop_thread_locked();
}

bool get_controller_states(std::array<controller_state, 2>& states)
{
	// We copy the states while holding the lock, so that the thread can't be destroyed in the meantime
	std::lock_guard lock(g_thread_mutex);
	if (!g_thread || !g_thread->is_input_active())
	{
		return false;
	}

	states = g_thread->get_input().get_states();
	return true;
}

void set_controller_vibration(u32 hand, f32 amplitude)
{
	std::lock_guard lock(g_thread_mutex);
	if (g_thread)
	{
		g_thread->get_input().set_vibration(hand, amplitude);
	}
}

session_thread::~session_thread()
{
	shutdown();
}

bool session_thread::is_rendering_wanted()
{
	// The renderers remove their consumer when rendering is disabled. We check the setting as well, since it can change at any time.
	return is_rendering_enabled() && has_consumer(consumer::headset_rendering);
}

void session_thread::operator()()
{
	while (thread_ctrl::state() != thread_state::aborting)
	{
		if (!m_instance)
		{
			if (std::chrono::steady_clock::now() < m_next_init_attempt)
			{
				thread_ctrl::wait_for(100'000);
				continue;
			}

			if (!init())
			{
				shutdown();
				m_next_init_attempt = std::chrono::steady_clock::now() + init_retry_delay;
				openxr_log.notice("Trying again in %d seconds", init_retry_delay.count());
				continue;
			}
		}

		if (!update())
		{
			// The runtime stopped the session or lost the instance. Start over.
			shutdown();
			m_next_init_attempt = std::chrono::steady_clock::now() + init_retry_delay;
		}
	}

	shutdown();
}

bool session_thread::init()
{
	openxr_log.notice("Initializing OpenXR");

	// Every step logs its own errors
	if (!is_openxr_available() ||
		!create_instance() ||
		!get_system_info() ||
		!create_vulkan_device() ||
		!create_session() ||
		!create_reference_space() ||
		!m_input.init(m_instance, m_session, m_enabled_extensions))
	{
		return false;
	}

	m_input_active = true;

	if (!m_placeholder.init(m_session, m_reference_space))
	{
		openxr_log.warning("Failed to create the placeholder layer. The OpenXR runtime may not give RPCS3 input focus without headset rendering.");
	}

	// Headset rendering is set up in update_renderer, since it can be enabled at any time.
	// The session is started in poll_events once the runtime reports XR_SESSION_STATE_READY.
	return true;
}

void session_thread::shutdown()
{
	m_input_active = false;

	// Stop sharing frames with RSX and destroy the swapchain before the session
	m_renderer.destroy();
	m_placeholder.destroy();
	m_rendering_requested = false;

	// Destroy the action spaces and actions before the session
	m_input.destroy();

	if (m_session)
	{
		if (m_reference_space)
		{
			xrDestroySpace(m_reference_space);
			m_reference_space = XR_NULL_HANDLE;
		}

		xrDestroySession(m_session);
		m_session = XR_NULL_HANDLE;
	}

	m_session_running = false;
	m_session_state = XR_SESSION_STATE_UNKNOWN;
	m_unfocused_since = {};
	m_unfocused_warning_shown = false;
	m_restart_required = false;

	// Destroy the OpenXR instance before our Vulkan objects.
	// Runtimes may still release resources that belong to our Vulkan device when the instance is destroyed
	// (e.g. the OpenXR Simulator imports its swapchain images into our device).
	if (m_instance)
	{
		openxr_log.notice("Shutting down OpenXR");
		xrDestroyInstance(m_instance);
		m_instance = XR_NULL_HANDLE;
	}

	m_enabled_extensions.clear();

	// Our Vulkan objects stay valid without the OpenXR instance, since they belong to us
	if (m_vk_device)
	{
		vkDestroyDevice(m_vk_device, nullptr);
		m_vk_device = VK_NULL_HANDLE;
	}

	if (m_vk_instance)
	{
		vkDestroyInstance(m_vk_instance, nullptr);
		m_vk_instance = VK_NULL_HANDLE;
	}

	m_vk_physical = VK_NULL_HANDLE;
	m_rendering_supported = false;
}
bool session_thread::create_instance()
{
	// We don't render to the headset, but OpenXR needs a graphics device to create a session.
	// With XR_KHR_vulkan_enable2 the runtime creates our own small Vulkan device with everything it needs,
	// so we don't depend on the renderer used by RSX.
	const char* graphics_extension = "XR_KHR_vulkan_enable2";

	const std::set<std::string> supported_extensions = get_supported_extensions();

	if (!supported_extensions.contains(graphics_extension))
	{
		openxr_log.error("The OpenXR runtime does not support '%s'", graphics_extension);
		return false;
	}

	m_enabled_extensions.insert(graphics_extension);

	// Enable the optional extensions of any controller profiles that the runtime knows about
	m_enabled_extensions.merge(input::get_profile_extensions(supported_extensions));

	std::vector<const char*> extension_names;
	for (const std::string& extension : m_enabled_extensions)
	{
		openxr_log.notice("Enabling extension '%s'", extension);
		extension_names.push_back(extension.c_str());
	}

	XrInstanceCreateInfo info{XR_TYPE_INSTANCE_CREATE_INFO};
	strcpy_trunc(info.applicationInfo.applicationName, "RPCS3 OpenXR");
	info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	info.enabledExtensionCount = static_cast<u32>(extension_names.size());
	info.enabledExtensionNames = extension_names.data();

	XrResult result = xrCreateInstance(&info, &m_instance);

	// Older runtimes only support OpenXR 1.0
	if (result == XR_ERROR_API_VERSION_UNSUPPORTED && info.applicationInfo.apiVersion != api_version_1_0)
	{
		openxr_log.notice("OpenXR %s is not supported by the runtime. Trying OpenXR 1.0.", get_version_string(info.applicationInfo.apiVersion));
		info.applicationInfo.apiVersion = api_version_1_0;
		result = xrCreateInstance(&info, &m_instance);
	}

	if (XR_FAILED(result))
	{
		openxr_log.error("xrCreateInstance failed (result=%d)", static_cast<s32>(result));
		return false;
	}

	XrInstanceProperties properties{XR_TYPE_INSTANCE_PROPERTIES};
	if (XR_SUCCEEDED(xrGetInstanceProperties(m_instance, &properties)))
	{
		openxr_log.notice("OpenXR runtime: %s %s (OpenXR %s)", properties.runtimeName, get_version_string(properties.runtimeVersion), get_version_string(info.applicationInfo.apiVersion));
	}

	return true;
}

bool session_thread::get_system_info()
{
	XrSystemGetInfo info{XR_TYPE_SYSTEM_GET_INFO};
	info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

	if (XR_FAILED(xrGetSystem(m_instance, &info, &m_system)))
	{
		openxr_log.error("xrGetSystem failed");
		return false;
	}

	XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
	if (XR_SUCCEEDED(xrGetSystemProperties(m_instance, m_system, &properties)))
	{
		openxr_log.notice("OpenXR system: %s (vendor=0x%x)", properties.systemName, properties.vendorId);
	}

	// The first blend mode is the one preferred by the runtime
	u32 blend_mode_count = 0;
	std::array<XrEnvironmentBlendMode, 8> blend_modes{};
	if (XR_SUCCEEDED(xrEnumerateEnvironmentBlendModes(m_instance, m_system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, static_cast<u32>(blend_modes.size()), &blend_mode_count, blend_modes.data())) && blend_mode_count > 0)
	{
		m_blend_mode = blend_modes[0];
	}
	else
	{
		openxr_log.warning("xrEnumerateEnvironmentBlendModes failed. Using opaque blend mode.");
	}

	return true;
}

bool session_thread::create_vulkan_device()
{
	PFN_xrGetVulkanGraphicsRequirements2KHR get_requirements = nullptr;
	PFN_xrCreateVulkanInstanceKHR create_vulkan_instance = nullptr;
	PFN_xrGetVulkanGraphicsDevice2KHR get_graphics_device = nullptr;
	PFN_xrCreateVulkanDeviceKHR create_vulkan_device = nullptr;

	if (!get_instance_proc(m_instance, "xrGetVulkanGraphicsRequirements2KHR", get_requirements) ||
		!get_instance_proc(m_instance, "xrCreateVulkanInstanceKHR", create_vulkan_instance) ||
		!get_instance_proc(m_instance, "xrGetVulkanGraphicsDevice2KHR", get_graphics_device) ||
		!get_instance_proc(m_instance, "xrCreateVulkanDeviceKHR", create_vulkan_device))
	{
		return false;
	}

	// The runtime requires us to query the graphics requirements before creating a session
	XrGraphicsRequirementsVulkan2KHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR};
	if (XR_FAILED(get_requirements(m_instance, m_system, &requirements)))
	{
		openxr_log.error("xrGetVulkanGraphicsRequirements2KHR failed");
		return false;
	}

	openxr_log.notice("Runtime supports Vulkan %s to %s", get_version_string(requirements.minApiVersionSupported), get_version_string(requirements.maxApiVersionSupported));

	// Use Vulkan 1.1 unless the runtime only supports 1.0
	const XrVersion vk_1_1 = XR_MAKE_VERSION(1, 1, 0);
	const u32 vk_api_version = requirements.maxApiVersionSupported >= vk_1_1 ? VK_API_VERSION_1_1 : VK_API_VERSION_1_0;

	// Create the Vulkan instance. The runtime adds the extensions it needs.
	VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
	app_info.pApplicationName = "RPCS3 OpenXR";
	app_info.applicationVersion = 1;
	app_info.pEngineName = "RPCS3";
	app_info.engineVersion = 1;
	app_info.apiVersion = vk_api_version;

	VkInstanceCreateInfo vk_instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	vk_instance_info.pApplicationInfo = &app_info;

	XrVulkanInstanceCreateInfoKHR xr_instance_info{XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
	xr_instance_info.systemId = m_system;
	xr_instance_info.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
	xr_instance_info.vulkanCreateInfo = &vk_instance_info;

	VkResult vk_result = VK_SUCCESS;
	if (XR_FAILED(create_vulkan_instance(m_instance, &xr_instance_info, &m_vk_instance, &vk_result)) || vk_result != VK_SUCCESS)
	{
		openxr_log.error("xrCreateVulkanInstanceKHR failed (VkResult=%d)", static_cast<s32>(vk_result));
		m_vk_instance = VK_NULL_HANDLE;
		return false;
	}

	// The runtime decides which GPU we have to use
	XrVulkanGraphicsDeviceGetInfoKHR device_get_info{XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
	device_get_info.systemId = m_system;
	device_get_info.vulkanInstance = m_vk_instance;

	if (XR_FAILED(get_graphics_device(m_instance, &device_get_info, &m_vk_physical)) || !m_vk_physical)
	{
		openxr_log.error("xrGetVulkanGraphicsDevice2KHR failed");
		return false;
	}

	VkPhysicalDeviceProperties device_properties{};
	vkGetPhysicalDeviceProperties(m_vk_physical, &device_properties);
	openxr_log.notice("Using Vulkan device '%s'", device_properties.deviceName);

	// Find a graphics queue
	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_vk_physical, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(m_vk_physical, &queue_family_count, queue_families.data());

	bool found_queue = false;
	for (u32 i = 0; i < queue_family_count; i++)
	{
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_vk_queue_family = i;
			found_queue = true;
			break;
		}
	}

	if (!found_queue)
	{
		openxr_log.error("No Vulkan graphics queue found");
		return false;
	}

	// Create the Vulkan device. The runtime adds the extensions it needs.
	const float queue_priority = 1.0f;

	VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queue_info.queueFamilyIndex = m_vk_queue_family;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = &queue_priority;

	VkDeviceCreateInfo vk_device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	vk_device_info.queueCreateInfoCount = 1;
	vk_device_info.pQueueCreateInfos = &queue_info;

	// Headset rendering is optional. Input keeps working if the device doesn't support it.
	std::vector<const char*> device_extensions;
	// Rendering can be enabled at any time, so we always enable the extensions if they are supported.
	m_rendering_supported = renderer::get_device_extensions(m_vk_physical, vk_api_version, device_extensions);

	vk_device_info.enabledExtensionCount = static_cast<u32>(device_extensions.size());
	vk_device_info.ppEnabledExtensionNames = device_extensions.data();

	XrVulkanDeviceCreateInfoKHR xr_device_info{XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
	xr_device_info.systemId = m_system;
	xr_device_info.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
	xr_device_info.vulkanPhysicalDevice = m_vk_physical;
	xr_device_info.vulkanCreateInfo = &vk_device_info;

	vk_result = VK_SUCCESS;
	if (XR_FAILED(create_vulkan_device(m_instance, &xr_device_info, &m_vk_device, &vk_result)) || vk_result != VK_SUCCESS)
	{
		openxr_log.error("xrCreateVulkanDeviceKHR failed (VkResult=%d)", static_cast<s32>(vk_result));
		m_vk_device = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

bool session_thread::create_session()
{
	// The graphics binding has to outlive the xrCreateSession call
	XrGraphicsBindingVulkan2KHR vk{XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR};
	vk.instance = m_vk_instance;
	vk.physicalDevice = m_vk_physical;
	vk.device = m_vk_device;
	vk.queueFamilyIndex = m_vk_queue_family;
	vk.queueIndex = vk_queue_index;

	XrSessionCreateInfo sci{XR_TYPE_SESSION_CREATE_INFO};
	sci.next = &vk;
	sci.systemId = m_system;

	if (XR_FAILED(xrCreateSession(m_instance, &sci, &m_session)))
	{
		openxr_log.error("xrCreateSession failed");
		return false;
	}

	return true;
}


bool session_thread::begin_session()
{
	XrSessionBeginInfo info{XR_TYPE_SESSION_BEGIN_INFO};
	info.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

	if (XR_FAILED(xrBeginSession(m_session, &info)))
	{
		openxr_log.error("xrBeginSession failed");
		return false;
	}

	m_session_running = true;
	return true;
}

void session_thread::poll_events()
{
	XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};

	while (xrPollEvent(m_instance, &event) == XR_SUCCESS)
	{
		switch (event.type)
		{
		case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
		{
			openxr_log.error("OpenXR instance loss pending");
			m_session_running = false;
			m_restart_required = true;
			break;
		}
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
		{
			const auto& state_event = reinterpret_cast<const XrEventDataSessionStateChanged&>(event);
			if (state_event.session == m_session)
			{
				handle_session_state_change(state_event.state);
			}
			break;
		}
		default:
			break;
		}

		event = {XR_TYPE_EVENT_DATA_BUFFER};
	}
}

void session_thread::handle_session_state_change(XrSessionState state)
{
	const auto get_state_name = [](XrSessionState state) -> std::string_view
	{
		switch (state)
		{
		case XR_SESSION_STATE_IDLE: return "Idle";
		case XR_SESSION_STATE_READY: return "Ready";
		case XR_SESSION_STATE_SYNCHRONIZED: return "Synchronized";
		case XR_SESSION_STATE_VISIBLE: return "Visible";
		case XR_SESSION_STATE_FOCUSED: return "Focused (input available)";
		case XR_SESSION_STATE_STOPPING: return "Stopping";
		case XR_SESSION_STATE_LOSS_PENDING: return "Loss pending";
		case XR_SESSION_STATE_EXITING: return "Exiting";
		default: return "Unknown";
		}
	};

	openxr_log.notice("Session state changed from %s to %s", get_state_name(m_session_state), get_state_name(state));

	// Warn again if the focus is lost later
	if (state == XR_SESSION_STATE_FOCUSED || state == XR_SESSION_STATE_IDLE)
	{
		m_unfocused_since = {};
		m_unfocused_warning_shown = false;
	}
	else if (state == XR_SESSION_STATE_VISIBLE || state == XR_SESSION_STATE_SYNCHRONIZED)
	{
		if (m_unfocused_since == std::chrono::steady_clock::time_point{})
		{
			m_unfocused_since = std::chrono::steady_clock::now();
		}
	}
	m_session_state = state;

	switch (state)
	{
	case XR_SESSION_STATE_READY:
	{
		if (!m_session_running && !begin_session())
		{
			openxr_log.error("Failed to begin session");
		}
		break;
	}
	case XR_SESSION_STATE_STOPPING:
	{
		if (m_session_running)
		{
			if (XR_FAILED(xrEndSession(m_session)))
			{
				openxr_log.error("xrEndSession failed");
			}
			m_session_running = false;
		}
		break;
	}
	case XR_SESSION_STATE_EXITING:
	case XR_SESSION_STATE_LOSS_PENDING:
	{
		// The runtime wants us to stop (for example because the headset was disconnected).
		// The thread destroys everything and tries again later.
		m_session_running = false;
		m_restart_required = true;
		break;
	}
	default:
		break;
	}
}

bool session_thread::run_frame(XrTime& predicted_time)
{
	// The runtime only progresses the session to the focused state (and thus enables input) if we run a frame loop
	XrFrameWaitInfo wait_info{XR_TYPE_FRAME_WAIT_INFO};
	XrFrameState frame_state{XR_TYPE_FRAME_STATE};

	if (XR_FAILED(xrWaitFrame(m_session, &wait_info, &frame_state)))
	{
		openxr_log.error("xrWaitFrame failed");
		return false;
	}

	XrFrameBeginInfo begin_info{XR_TYPE_FRAME_BEGIN_INFO};

	if (XR_FAILED(xrBeginFrame(m_session, &begin_info)))
	{
		openxr_log.error("xrBeginFrame failed");
		return false;
	}

	std::array<const XrCompositionLayerBaseHeader*, 2> layers{};
	u32 layer_count = m_renderer.render(frame_state.predictedDisplayTime, frame_state.shouldRender, layers);

	// Without headset rendering (or before the first frame) we submit an invisible layer instead of none
	if (layer_count == 0)
	{
		layers[0] = m_placeholder.get_layer();
		layer_count = layers[0] ? 1 : 0;
	}

	XrFrameEndInfo end_info{XR_TYPE_FRAME_END_INFO};
	end_info.displayTime = frame_state.predictedDisplayTime;
	end_info.environmentBlendMode = m_blend_mode;
	end_info.layerCount = layer_count;
	end_info.layers = layers.data();

	if (XR_FAILED(xrEndFrame(m_session, &end_info)))
	{
		openxr_log.error("xrEndFrame failed");
		return false;
	}

	predicted_time = frame_state.predictedDisplayTime;
	return true;
}

bool session_thread::update()
{
	poll_events();

	if (m_restart_required)
		return false;

	if (!m_session_running)
	{
		// Nothing blocks without a running session, so we have to wait for the runtime ourselves
		thread_ctrl::wait_for(10'000);
		return true;
	}

	// Between frames, so the swapchain is not in use
	update_renderer();

	XrTime predicted_time{};
	if (!run_frame(predicted_time))
		return true;

	// Sync the actions every frame while the session runs. Runtimes may wait for this before they give us input focus.
	// Without focus, xrSyncActions returns XR_SESSION_NOT_FOCUSED (not an error) and all actions are inactive.
	m_input.update(m_reference_space, predicted_time);

	// Input is only available while the session is focused
	if (m_session_state != XR_SESSION_STATE_FOCUSED)
	{
		// Don't keep buttons pressed that were held when the focus was lost
		m_input.clear_states();

		// The runtime only sends controller input to the focused application. Tell the user why the controllers don't work.
		if (!m_unfocused_warning_shown && m_unfocused_since != std::chrono::steady_clock::time_point{} &&
			std::chrono::steady_clock::now() - m_unfocused_since > std::chrono::seconds(5))
		{
			openxr_log.warning("The OpenXR runtime shows RPCS3 but doesn't give it input focus, so the controllers are not available. "
				"Close the dashboard or menu of the runtime (e.g. SteamVR or Meta Quest) and make sure that the headset is being worn.");
			m_unfocused_warning_shown = true;
		}
	}

	return true;
}

void session_thread::update_renderer()
{
	// We only react to enabling or disabling rendering (the renderer handles mode switches). Only games enable it, not the pad settings.
	const bool enabled = is_rendering_wanted();

	if (enabled == m_rendering_requested)
	{
		return;
	}

	m_rendering_requested = enabled;

	if (!enabled)
	{
		openxr_log.notice("Headset rendering disabled");
		m_renderer.destroy();
		return;
	}

	if (!m_rendering_supported)
	{
		// Only reported once per change, since m_rendering_requested stays true
		openxr_log.error("Headset rendering is not supported by the OpenXR runtime's Vulkan device. Only input will be available.");
		return;
	}

	// We still set up the swapchain, in case the renderer is changed, but the null renderer doesn't share any frames with us
	if (g_cfg.video.renderer == video_renderer::null)
	{
		openxr_log.warning("Headset rendering requires the Vulkan or OpenGL renderer. The headset will not show the game.");
	}

	const renderer::context ctx
	{
		.session = m_session,
		.reference_space = m_reference_space,
		.vk_physical = m_vk_physical,
		.vk_device = m_vk_device,
		.vk_queue_family = m_vk_queue_family,
		.vk_queue_index = vk_queue_index,
	};

	if (!m_renderer.init(ctx))
	{
		openxr_log.error("Failed to set up headset rendering. Only input will be available.");
		m_renderer.destroy();
	}
}

bool session_thread::create_reference_space()
{
	XrReferenceSpaceCreateInfo info{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
	info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	info.poseInReferenceSpace.orientation.w = 1.0f;

	if (XR_FAILED(xrCreateReferenceSpace(m_session, &info, &m_reference_space)))
	{
		openxr_log.error("xrCreateReferenceSpace failed");
		return false;
	}

	return true;
}

}
