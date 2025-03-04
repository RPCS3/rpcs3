#ifndef _WIN32
#error "XAudio2 can only be built on Windows."
#endif

#include "Emu/Audio/XAudio2/xaudio2_enumerator.h"
#include "util/logs.hpp"
#include "Utilities/StrUtil.h"
#include <algorithm>

#include <wrl/client.h>
#include <Windows.h>
#include <system_error>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>

LOG_CHANNEL(xaudio_dev_enum);

xaudio2_enumerator::xaudio2_enumerator() : audio_device_enumerator()
{
}

xaudio2_enumerator::~xaudio2_enumerator()
{
}

std::vector<audio_device_enumerator::audio_device> xaudio2_enumerator::get_output_devices()
{
	Microsoft::WRL::ComPtr<IMMDeviceEnumerator> devEnum{};
	if (HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&devEnum)); FAILED(hr))
	{
		xaudio_dev_enum.error("CoCreateInstance() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return {};
	}

	Microsoft::WRL::ComPtr<IMMDeviceCollection> devices{};
	if (HRESULT hr = devEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED, &devices); FAILED(hr))
	{
		xaudio_dev_enum.error("EnumAudioEndpoints() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return {};
	}

	UINT count = 0;
	if (HRESULT hr = devices->GetCount(&count); FAILED(hr))
	{
		xaudio_dev_enum.error("devices->GetCount() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return {};
	}

	if (count == 0)
	{
		xaudio_dev_enum.warning("No devices available");
		return {};
	}

	std::vector<audio_device> device_list{};

	for (UINT dev_idx = 0; dev_idx < count; dev_idx++)
	{
		Microsoft::WRL::ComPtr<IMMDevice> endpoint{};
		if (HRESULT hr = devices->Item(dev_idx, &endpoint); FAILED(hr))
		{
			xaudio_dev_enum.error("devices->Item() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
			continue;
		}

		LPWSTR id = nullptr;
		if (HRESULT hr = endpoint->GetId(&id); FAILED(hr))
		{
			xaudio_dev_enum.error("endpoint->GetId() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
			continue;
		}

		if (std::wstring_view{id}.empty())
		{
			xaudio_dev_enum.error("Empty device id - skipping");
			CoTaskMemFree(id);
			continue;
		}

		audio_device dev{};
		dev.id = wchar_to_utf8(id);

		CoTaskMemFree(id);

		Microsoft::WRL::ComPtr<IPropertyStore> props{};
		if (HRESULT hr = endpoint->OpenPropertyStore(STGM_READ, &props); FAILED(hr))
		{
			xaudio_dev_enum.error("endpoint->OpenPropertyStore() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
			continue;
		}

		PROPVARIANT var;
		PropVariantInit(&var);

		if (HRESULT hr = props->GetValue(PKEY_Device_FriendlyName, &var); FAILED(hr))
		{
			xaudio_dev_enum.error("props->GetValue() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
			PropVariantClear(&var);
			continue;
		}

		if (var.vt != VT_LPWSTR)
		{
			PropVariantClear(&var);
			continue;
		}

		dev.name = wchar_to_utf8(var.pwszVal);
		if (dev.name.empty())
		{
			dev.name = dev.id;
		}

		PropVariantClear(&var);
		dev.max_ch = 2;

		if (HRESULT hr = props->GetValue(PKEY_AudioEngine_DeviceFormat, &var); SUCCEEDED(hr))
		{
			if (var.vt == VT_BLOB)
			{
				if (var.blob.cbSize == sizeof(PCMWAVEFORMAT))
				{
					const PCMWAVEFORMAT* pcm = std::bit_cast<const PCMWAVEFORMAT*>(var.blob.pBlobData);
					dev.max_ch = pcm->wf.nChannels;
				}
				else if (var.blob.cbSize >= sizeof(WAVEFORMATEX))
				{
					const WAVEFORMATEX* wfx = std::bit_cast<const WAVEFORMATEX*>(var.blob.pBlobData);
					if (var.blob.cbSize >= sizeof(WAVEFORMATEX) + wfx->cbSize || wfx->wFormatTag == WAVE_FORMAT_PCM)
					{
						dev.max_ch = wfx->nChannels;
					}
				}
			}
		}
		else
		{
			xaudio_dev_enum.error("props->GetValue() failed: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		}

		PropVariantClear(&var);

		xaudio_dev_enum.notice("Found device: id=%s, name=%s, max_ch=%d", dev.id, dev.name, dev.max_ch);
		device_list.emplace_back(dev);
	}

	std::sort(device_list.begin(), device_list.end(), [](const audio_device_enumerator::audio_device& a, const audio_device_enumerator::audio_device& b)
	{
		return a.name < b.name;
	});

	return device_list;
}
