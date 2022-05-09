#include "Emu/Audio/Cubeb/cubeb_enumerator.h"
#include "util/logs.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <system_error>
#endif

LOG_CHANNEL(cubeb_dev_enum);

cubeb_enumerator::cubeb_enumerator() : audio_device_enumerator()
{
#ifdef _WIN32
	// Cubeb requires COM to be initialized on the thread calling cubeb_init on Windows
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		com_init_success = true;
	}
#endif

	if (int err = cubeb_init(&ctx, "RPCS3 device enumeration", nullptr))
	{
		cubeb_dev_enum.error("cubeb_init() failed: %i", err);
		ctx = nullptr;
	}
}

cubeb_enumerator::~cubeb_enumerator()
{
	if (ctx)
	{
		cubeb_destroy(ctx);
	}

#ifdef _WIN32
	if (com_init_success)
	{
		CoUninitialize();
	}
#endif
}

std::vector<audio_device_enumerator::audio_device> cubeb_enumerator::get_output_devices()
{
	if (ctx == nullptr)
	{
		return {};
	}

	cubeb_device_collection dev_collection{};
	if (int err = cubeb_enumerate_devices(ctx, CUBEB_DEVICE_TYPE_OUTPUT, &dev_collection))
	{
		cubeb_dev_enum.error("cubeb_enumerate_devices() failed: %i", err);
		return {};
	}

	if (dev_collection.count == 0)
	{
		cubeb_dev_enum.error("No output devices available");
		if (int err = cubeb_device_collection_destroy(ctx, &dev_collection))
		{
			cubeb_dev_enum.error("cubeb_device_collection_destroy() failed: %i", err);
		}
		return {};
	}

	std::vector<audio_device> device_list{};

	for (u64 dev_idx = 0; dev_idx < dev_collection.count; dev_idx++)
	{
		const cubeb_device_info& dev_info = dev_collection.device[dev_idx];
		audio_device dev{std::string{dev_info.device_id}, std::string{dev_info.friendly_name}, dev_info.max_channels};

		if (dev.id.empty())
		{
			cubeb_dev_enum.warning("Empty device id - skipping");
			continue;
		}

		if (dev.name.empty())
		{
			dev.name = dev.id;
		}

		device_list.emplace_back(dev);
	}

	if (int err = cubeb_device_collection_destroy(ctx, &dev_collection))
	{
		cubeb_dev_enum.error("cubeb_device_collection_destroy() failed: %i", err);
	}

	return device_list;
}
