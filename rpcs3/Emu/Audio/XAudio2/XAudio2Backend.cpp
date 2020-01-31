#ifndef _WIN32
#error "XAudio2 can only be built on Windows."
#endif

#include "Utilities/Log.h"
#include "Utilities/StrFmt.h"

#include "XAudio2Backend.h"
#include <Windows.h>

LOG_CHANNEL(XAudio);

XAudio2Backend::XAudio2Backend()
{
}

XAudio2Backend::~XAudio2Backend()
{
}

void XAudio2Backend::Play()
{
	lib->play();
}

void XAudio2Backend::Close()
{
	lib->stop();
	lib->flush();
}

void XAudio2Backend::Pause()
{
	lib->stop();
}

void XAudio2Backend::Open(u32 /* num_buffers */)
{
	if (!lib)
	{
		void* hmodule;

		if (hmodule = LoadLibraryExW(L"XAudio2_9.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
		{
			// XAudio 2.9 uses the same code as XAudio 2.8
			lib.reset(xa28_init(hmodule));

			XAudio.success("XAudio 2.9 initialized");
		}
		else if (hmodule = LoadLibraryExW(L"XAudio2_8.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
		{
			lib.reset(xa28_init(hmodule));

			XAudio.success("XAudio 2.8 initialized");
		}
		else if (hmodule = LoadLibraryExW(L"XAudio2_7.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
		{
			lib.reset(xa27_init(hmodule));

			XAudio.success("XAudio 2.7 initialized");
		}
		else
		{
			fmt::throw_exception("No supported XAudio2 library found");
		}
	}

	lib->open();
}

bool XAudio2Backend::IsPlaying()
{
	return lib->is_playing();
}

bool XAudio2Backend::AddData(const void* src, u32 num_samples)
{
	return lib->add(src, num_samples);
}

void XAudio2Backend::Flush()
{
	lib->flush();
}

u64 XAudio2Backend::GetNumEnqueuedSamples()
{
	return lib->enqueued_samples();
}

f32 XAudio2Backend::SetFrequencyRatio(f32 new_ratio)
{
	return lib->set_freq_ratio(new_ratio);
}
