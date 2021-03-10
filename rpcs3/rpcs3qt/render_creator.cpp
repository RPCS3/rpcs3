#include "render_creator.h"

#include <QMessageBox>

#include "Utilities/Thread.h"

#if defined(_WIN32) || defined(HAVE_VULKAN)
#include "Emu/RSX/VK/vkutils/instance.hpp"
#endif

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <util/logs.hpp>

LOG_CHANNEL(cfg_log, "CFG");

constexpr auto qstr = QString::fromStdString;

render_creator::render_creator(QObject *parent) : QObject(parent)
{
#if defined(WIN32) || defined(HAVE_VULKAN)
	// Some drivers can get stuck when checking for vulkan-compatible gpus, f.ex. if they're waiting for one to get
	// plugged in. This whole contraption is for showing an error message in case that happens, so that user has
	// some idea about why the emulator window isn't showing up.

	struct thread_data_t
	{
		std::mutex mtx;
		std::condition_variable cond;
		bool work_done = false;
	};

	const auto data = std::make_shared<thread_data_t>();

	auto enum_thread_v = new named_thread("Vulkan Device Enumeration Thread"sv, [this, data]()
	{
		thread_ctrl::scoped_priority low_prio(-1);

		vk::instance device_enum_context;

		std::unique_lock lock(data->mtx, std::defer_lock);

		if (device_enum_context.create("RPCS3", true))
		{
			device_enum_context.bind();
			std::vector<vk::physical_device>& gpus = device_enum_context.enumerate_devices();

			lock.lock();

			if (!data->work_done) // The spawning thread gave up, do not attempt to modify vulkan_adapters
			{
				for (auto& gpu : gpus)
				{
					vulkan_adapters.append(qstr(gpu.get_name()));
				}
			}
		}
		else
		{
			lock.lock();
		}

		data->work_done = true;
		lock.unlock();
		data->cond.notify_all();
	});

	std::unique_ptr<std::remove_pointer_t<decltype(enum_thread_v)>> enum_thread(enum_thread_v);
	{
		std::unique_lock lck(data->mtx);
		cond.wait_for(lck, std::chrono::seconds(10), [&] { return data->work_done; });
	}

	if (std::scoped_lock{data->mtx}, !std::exchange(data->work_done, true)) // If thread hasn't done its job yet, it won't anymore
	{
		enum_thread.release(); // Detach thread (destructor is not called)

		cfg_log.error("Vulkan device enumeration timed out");
		const auto button = QMessageBox::critical(nullptr, tr("Vulkan Check Timeout"),
			tr("Querying for Vulkan-compatible devices is taking too long. This is usually caused by malfunctioning "
				"graphics drivers, reinstalling them could fix the issue.\n\n"
				"Selecting ignore starts the emulator without Vulkan support."),
			QMessageBox::Ignore | QMessageBox::Abort, QMessageBox::Abort);

		if (button != QMessageBox::Ignore)
		{
			abort_requested = true;
			return;
		}

		supports_vulkan = false;
	}
	else
	{
		supports_vulkan = !vulkan_adapters.isEmpty();
		enum_thread.reset(); // Join thread
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
		if (static_cast<usz>(i) >= renderers.size() || !renderers[i])
		{
			cfg_log.error("render_creator::update_names could not update renderer %d", i);
			return;
		}

		renderers[i]->name = names[i];
	}
}
