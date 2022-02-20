#include "hid_pad_handler.h"
#include "ds3_pad_handler.h"
#include "ds4_pad_handler.h"
#include "dualsense_pad_handler.h"
#include "util/logs.hpp"
#include "Utilities/Timer.h"
#include "Emu/System.h"
#include "pad_thread.h"

#include <algorithm>
#include <memory>

LOG_CHANNEL(hid_log, "HID");

static std::mutex s_hid_mutex; // hid_pad_handler is created by pad_thread and pad_settings_dialog
static u8 s_hid_instances{0};

template <class Device>
hid_pad_handler<Device>::hid_pad_handler(pad_handler type, std::vector<id_pair> ids)
    : PadHandlerBase(type), m_ids(std::move(ids))
{
	std::scoped_lock lock(s_hid_mutex);
	ensure(s_hid_instances++ < 255);
};

template <class Device>
hid_pad_handler<Device>::~hid_pad_handler()
{
	if (m_enumeration_thread)
	{
		auto& enumeration_thread = *m_enumeration_thread;
		enumeration_thread = thread_state::aborting;
		enumeration_thread();
	}

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

	for (usz i = 1; i <= MAX_GAMEPADS; i++) // Controllers 1-n in GUI
	{
		m_controllers.emplace(m_name_string + std::to_string(i), std::make_shared<Device>());
	}

	enumerate_devices();
	update_devices();

	m_is_init = true;

	m_enumeration_thread = std::make_unique<named_thread<std::function<void()>>>(fmt::format("%s Enumerator", m_type), [this]()
	{
		while (thread_ctrl::state() != thread_state::aborting)
		{
			if (pad::g_enabled && Emu.IsRunning())
			{
				enumerate_devices();
			}

			thread_ctrl::wait_for(2'000'000);
		}
	});

	return true;
}

template <class Device>
void hid_pad_handler<Device>::ThreadProc()
{
	update_devices();

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
	Timer timer;
	std::set<std::string> device_paths;
	std::map<std::string, std::wstring_view> serials;

	for (const auto& [vid, pid] : m_ids)
	{
		hid_device_info* dev_info = hid_enumerate(vid, pid);
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
	hid_log.notice("%s enumeration found %d devices (%f ms)", m_type, device_paths.size(), timer.GetElapsedTimeInMilliSec());

	std::lock_guard lock(m_enumeration_mutex);
	m_new_enumerated_devices = device_paths;
	m_enumerated_serials = serials;
}

template <class Device>
void hid_pad_handler<Device>::update_devices()
{
	std::lock_guard lock(m_enumeration_mutex);

	if (m_last_enumerated_devices == m_new_enumerated_devices)
	{
		return;
	}

	m_last_enumerated_devices = m_new_enumerated_devices;

	// Scrap devices that are not in the new list
	for (auto& controller : m_controllers)
	{
		if (controller.second && !controller.second->path.empty() && !m_new_enumerated_devices.contains(controller.second->path))
		{
			hid_close(controller.second->hidDevice);
			cfg_pad* config = controller.second->config;
			controller.second.reset(new Device());
			controller.second->config = config;
		}
	}

	bool warn_about_drivers = false;

	// Find and add new devices
	for (const auto& path : m_new_enumerated_devices)
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
			check_add_device(dev, path, m_enumerated_serials[path]);
		}
		else
		{
			hid_log.error("%s hid_open_path failed! error='%s', path='%s'", m_type, hid_error(dev), path);
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
		const usz count = std::count_if(m_controllers.cbegin(), m_controllers.cend(), [](const auto& c) { return c.second && c.second->hidDevice; });
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

template <class Device>
u32 hid_pad_handler<Device>::get_battery_color(u8 battery_level, int brightness)
{
	static const std::array<u32, 12> battery_level_clr = {0xff00, 0xff33, 0xff66, 0xff99, 0xffcc, 0xffff, 0xccff, 0x99ff, 0x66ff, 0x33ff, 0x00ff, 0x00ff};

	const u32 combined_color = battery_level_clr[battery_level < battery_level_clr.size() ? battery_level : 0];

	const u32 red = (combined_color >> 8) * brightness / 100;
	const u32 green = (combined_color & 0xff) * brightness / 100;
	return ((red << 8) | green);
}

template class hid_pad_handler<ds3_device>;
template class hid_pad_handler<DS4Device>;
template class hid_pad_handler<DualSenseDevice>;
