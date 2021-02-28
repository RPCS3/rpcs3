#include "hid_pad_handler.h"
#include "ds3_pad_handler.h"
#include "ds4_pad_handler.h"
#include "dualsense_pad_handler.h"
#include "util/logs.hpp"

#include <algorithm>
#include <memory>

LOG_CHANNEL(hid_log, "HID");

static std::mutex s_hid_mutex; // hid_pad_handler is created by pad_thread and pad_settings_dialog
static u8 s_hid_instances{0};

template <class Device>
hid_pad_handler<Device>::hid_pad_handler(pad_handler type, u16 vid, std::vector<u16> pids)
    : PadHandlerBase(type), m_vid(vid), m_pids(std::move(pids))
{
	std::scoped_lock lock(s_hid_mutex);
	ensure(s_hid_instances++ < 255);
};

template <class Device>
hid_pad_handler<Device>::~hid_pad_handler()
{
	for (auto& controller : m_controllers)
	{
		if (controller.second && controller.second->hidDevice)
		{
			hid_close(controller.second->hidDevice);
		}
	}

	std::scoped_lock lock(s_hid_mutex);
	ensure(s_hid_instances-- > 0);
	if (s_hid_instances == 0)
	{
		// Call hid_exit after all hid_pad_handlers are finished
		if (hid_exit() != 0)
		{
			hid_log.error("hid_exit failed!");
		}
	}
}

template <class Device>
bool hid_pad_handler<Device>::Init()
{
	if (m_is_init)
		return true;

	const int res = hid_init();
	if (res != 0)
		fmt::throw_exception("%s hidapi-init error.threadproc", m_type);

	for (size_t i = 1; i <= MAX_GAMEPADS; i++) // Controllers 1-n in GUI
	{
		m_controllers.emplace(m_name_string + std::to_string(i), std::make_shared<Device>());
	}

	enumerate_devices();

	m_is_init = true;
	return true;
}

template <class Device>
void hid_pad_handler<Device>::ThreadProc()
{
	const auto now     = std::chrono::system_clock::now();
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_enumeration).count();
	if (elapsed > 2000)
	{
		m_last_enumeration = now;
		enumerate_devices();
	}

	PadHandlerBase::ThreadProc();
}

template <class Device>
std::vector<std::string> hid_pad_handler<Device>::ListDevices()
{
	std::vector<std::string> pads_list;

	if (!Init())
		return pads_list;

	for (const auto& controller : m_controllers) // Controllers 1-n in GUI
	{
		pads_list.emplace_back(controller.first);
	}

	return pads_list;
}

template <class Device>
void hid_pad_handler<Device>::enumerate_devices()
{
	std::set<std::string> device_paths;
	std::map<std::string, std::wstring_view> serials;

	for (const auto& pid : m_pids)
	{
		hid_device_info* dev_info = hid_enumerate(m_vid, pid);
		hid_device_info* head     = dev_info;
		while (dev_info)
		{
			ensure(dev_info->path != nullptr);
			device_paths.insert(dev_info->path);
			serials[dev_info->path] = dev_info->serial_number ? std::wstring_view(dev_info->serial_number) : std::wstring_view{};
			dev_info                = dev_info->next;
		}
		hid_free_enumeration(head);
	}

	if (m_last_enumerated_devices == device_paths)
	{
		return;
	}

	m_last_enumerated_devices = device_paths;

	// Scrap devices that are not in the new list
	for (auto& controller : m_controllers)
	{
		if (controller.second && !controller.second->path.empty() && !device_paths.contains(controller.second->path))
		{
			hid_close(controller.second->hidDevice);
			pad_config* config = controller.second->config;
			controller.second.reset(new Device());
			controller.second->config = config;
		}
	}

	bool warn_about_drivers = false;

	// Find and add new devices
	for (const auto& path : device_paths)
	{
		// Check if we have at least one virtual controller left
		if (std::none_of(m_controllers.cbegin(), m_controllers.cend(), [](const auto& c) { return !c.second || !c.second->hidDevice; }))
			break;

		// Check if we already have this controller
		if (std::any_of(m_controllers.cbegin(), m_controllers.cend(), [&path](const auto& c) { return c.second && c.second->path == path; }))
			continue;

		hid_device* dev = hid_open_path(path.c_str());
		if (dev)
		{
			check_add_device(dev, path, serials[path]);
		}
		else
		{
			hid_log.error("%s hid_open_path failed! Reason: %s", m_type, hid_error(dev));
			warn_about_drivers = true;
		}
	}

	if (warn_about_drivers)
	{
		hid_log.error("One or more %s pads were detected but couldn't be interacted with directly", m_type);
#if defined(_WIN32) || defined(__linux__)
		hid_log.error("Check https://wiki.rpcs3.net/index.php?title=Help:Controller_Configuration for intructions on how to solve this issue");
#endif
	}
	else
	{
		const size_t count = std::count_if(m_controllers.cbegin(), m_controllers.cend(), [](const auto& c) { return c.second && c.second->hidDevice; });
		if (count > 0)
		{
			hid_log.success("%s Controllers found: %d", m_type, count);
		}
		else
		{
			hid_log.warning("No %s controllers found!", m_type);
		}
	}
}

template <class Device>
std::shared_ptr<Device> hid_pad_handler<Device>::get_hid_device(const std::string& padId)
{
	if (!Init())
		return nullptr;

	// Controllers 1-n in GUI
	for (auto& cur_control : m_controllers)
	{
		if (padId == cur_control.first)
		{
			return cur_control.second;
		}
	}

	return nullptr;
}

template <class Device>
std::shared_ptr<PadDevice> hid_pad_handler<Device>::get_device(const std::string& device)
{
	return get_hid_device(device);
}

template class hid_pad_handler<ds3_device>;
template class hid_pad_handler<DS4Device>;
template class hid_pad_handler<DualSenseDevice>;
