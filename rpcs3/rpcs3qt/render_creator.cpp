#include "render_creator.h"

#include <QMessageBox>

#include "Utilities/Config.h"
#include "Utilities/Thread.h"

#if defined(_WIN32) || defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VKHelpers.h"
#endif

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

LOG_CHANNEL(cfg_log, "CFG");

constexpr auto qstr = QString::fromStdString;

render_creator::render_creator(QObject *parent) : QObject(parent)
{
#if defined(WIN32) || defined(HAVE_VULKAN)
	// Some drivers can get stuck when checking for vulkan-compatible gpus, f.ex. if they're waiting for one to get
	// plugged in. This whole contraption is for showing an error message in case that happens, so that user has
	// some idea about why the emulator window isn't showing up.

	static std::atomic<bool> was_called = false;
	if (was_called.exchange(true))
		fmt::throw_exception("Render_Creator cannot be created more than once" HERE);

	static std::mutex mtx;
	static std::condition_variable cond;
	static bool thread_running = true;
	static bool device_found = false;

	static QStringList compatible_gpus;

	std::thread enum_thread = std::thread([&]
	{
		thread_ctrl::set_native_priority(-1);

		vk::context device_enum_context;
		if (device_enum_context.createInstance("RPCS3", true))
		{
			device_enum_context.makeCurrentInstance();
			std::vector<vk::physical_device>& gpus = device_enum_context.enumerateDevices();

			if (!gpus.empty())
			{
				device_found = true;

				for (auto& gpu : gpus)
				{
					compatible_gpus.append(qstr(gpu.get_name()));
				}
			}
		}

		std::scoped_lock{ mtx }, thread_running = false;
		cond.notify_all();
	});

	{
		std::unique_lock lck(mtx);
		cond.wait_for(lck, std::chrono::seconds(10), [&] { return !thread_running; });
	}

	if (thread_running)
	{
		cfg_log.error("Vulkan device enumeration timed out");
		const auto button = QMessageBox::critical(nullptr, tr("Vulkan Check Timeout"),
			tr("Querying for Vulkan-compatible devices is taking too long. This is usually caused by malfunctioning "
				"graphics drivers, reinstalling them could fix the issue.\n\n"
				"Selecting ignore starts the emulator without Vulkan support."),
			QMessageBox::Ignore | QMessageBox::Abort, QMessageBox::Abort);

		enum_thread.detach();
		if (button != QMessageBox::Ignore)
			std::exit(1);

		supports_vulkan = false;
	}
	else
	{
		supports_vulkan = device_found;
		vulkan_adapters = std::move(compatible_gpus);
		enum_thread.join();
	}
#endif

	// Graphics Adapter
	Vulkan = render_info(vulkan_adapters, supports_vulkan, emu_settings_type::VulkanAdapter, true);
	OpenGL = render_info();
	NullRender = render_info();

	renderers = { &Vulkan, &OpenGL, &NullRender };
}

void render_creator::update_names(const QStringList& names)
{
	for (int i = 0; i < names.size(); i++)
	{
		if (static_cast<size_t>(i) >= renderers.size() || !renderers[i])
		{
			cfg_log.error("render_creator::update_names could not update renderer %d", i);
			return;
		}

		renderers[i]->name = names[i];
	}
}
