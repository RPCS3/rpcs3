#ifndef HAVE_FAUDIO
#error "FAudio support disabled but still being built."
#endif

#include "Emu/Audio/FAudio/faudio_enumerator.h"
#include <algorithm>
#include "Utilities/StrUtil.h"
#include "util/logs.hpp"

LOG_CHANNEL(faudio_dev_enum);

faudio_enumerator::faudio_enumerator() : audio_device_enumerator()
{
	FAudio *tmp{};

	if (u32 res = FAudioCreate(&tmp, 0, FAUDIO_DEFAULT_PROCESSOR))
	{
		faudio_dev_enum.error("FAudioCreate() failed(0x%08x)", res);
		return;
	}

	// All succeeded, "commit"
	instance = tmp;
}

faudio_enumerator::~faudio_enumerator()
{
	if (instance != nullptr)
	{
		FAudio_StopEngine(instance);
		FAudio_Release(instance);
	}
}

std::vector<audio_device_enumerator::audio_device> faudio_enumerator::get_output_devices()
{
	if (!instance)
	{
		return {};
	}

	u32 dev_cnt{};
	if (u32 res = FAudio_GetDeviceCount(instance, &dev_cnt))
	{
		faudio_dev_enum.error("FAudio_GetDeviceCount() failed(0x%08x)", res);
		return {};
	}

	if (dev_cnt == 0)
	{
		faudio_dev_enum.warning("No devices available");
		return {};
	}

	std::vector<audio_device> device_list{};

	for (u32 dev_idx = 0; dev_idx < dev_cnt; dev_idx++)
	{
		FAudioDeviceDetails dev_info{};

		if (u32 res = FAudio_GetDeviceDetails(instance, dev_idx, &dev_info))
		{
			faudio_dev_enum.error("FAudio_GetDeviceDetails() failed(0x%08x)", res);
			continue;
		}

		audio_device dev =
		{
			.id = std::to_string(dev_idx),
			.name = utf16_to_utf8(std::bit_cast<char16_t*>(&dev_info.DisplayName[0])),
			.max_ch = dev_info.OutputFormat.Format.nChannels
		};

		if (dev.name.empty())
		{
			dev.name = "Device " + dev.id;
		}

		faudio_dev_enum.notice("Found device: id=%s, name=%s, max_ch=%d", dev.id, dev.name, dev.max_ch);
		device_list.emplace_back(dev);
	}

	std::sort(device_list.begin(), device_list.end(), [](audio_device_enumerator::audio_device a, audio_device_enumerator::audio_device b)
	{
		return a.name < b.name;
	});

	return device_list;
}
