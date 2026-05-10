#include "render_creator.h"

#include <QMessageBox>

#include "Utilities/Thread.h"

#if defined(HAVE_VULKAN)
#include "Emu/RSX/VK/vkutils/instance.h"
#endif

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <util/logs.hpp>

LOG_CHANNEL(cfg_log, "CFG");

render_creator::render_creator(QObject *parent) : QObject(parent)
{
#if defined(HAVE_VULKAN)
	// Some drivers can get stuck when checking for vulkan-compatible gpus, f.ex. if they're waiting for one to get
	// plugged in. This whole contraption is for showing an error message in case that happens, so that user has
	// some idea about why the emulator window isn't showing up.

	static atomic_t<bool> was_called = false;
	if (was_called.exchange(true))
		fmt::throw_exception("Render_Creator cannot be created more than once");

	static std::mutex mtx;
	static std::condition_variable cond;
	static bool work_done = false;

	auto enum_thread_v = new named_thread("Vulkan Device Enumeration Thread"sv, [&, adapters = &this->vulkan_adapters]()
	{
		thread_ctrl::scoped_priority low_prio(-1);

		vk::instance device_enum_context;

		std::unique_lock lock(mtx, std::defer_lock);

		if (device_enum_context.create("RPCS3", true))
		{
			device_enum_context.bind();
			const std::vector<vk::physical_device>& gpus = device_enum_context.enumerate_devices();

			lock.lock();

			if (!work_done) // The spawning thread gave up, do not attempt to modify vulkan_adapters
			{
				for (const auto& gpu : gpus)
				{
					adapters->append(QString::fromStdString(gpu.get_name()));
				}
			}
		}
		else
		{
			lock.lock();
		}

		work_done = true;
		lock.unlock();
		cond.notify_all();
	});

	std::unique_ptr<std::remove_pointer_t<decltype(enum_thread_v)>> enum_thread(enum_thread_v);

	if ([&]()
	{
		std::unique_lock lck(mtx);
		cond.wait_for(lck, std::chrono::seconds(10), [&] { return work_done; });
		return !std::exchange(work_done, true); // If thread hasn't done its job yet, it won't anymore
	}())
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
	Vulkan = render_info(vulkan_adapters, supports_vulkan, emu_settings_type::VulkanAdapter);
	OpenGL = render_info();
	NullRender = render_info();

#ifdef __APPLE__
	OpenGL.supported = false;

	if (!Vulkan.supported)
	{
		QMessageBox::warning(nullptr,
							 tr("Warning"),
							 tr("Vulkan is not supported on this Mac.\n"
								"No graphics will be rendered."));
	}
#endif

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
