#ifdef _WIN32

#include "Utilities/Log.h"
#include "Utilities/StrFmt.h"

#include "XAudio2Backend.h"
#include <Windows.h>

XAudio2Backend::XAudio2Backend()
{
	if (auto lib2_9 = LoadLibraryExW(L"XAudio2_9.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
	{
		// xa28* implementation is fully compatible with library 2.9
		xa28_init(lib2_9);

		m_funcs.destroy    = &xa28_destroy;
		m_funcs.play       = &xa28_play;
		m_funcs.flush      = &xa28_flush;
		m_funcs.stop       = &xa28_stop;
		m_funcs.open       = &xa28_open;
		m_funcs.is_playing = &xa28_is_playing;
		m_funcs.add        = &xa28_add;
		m_funcs.enqueued_samples = &xa28_enqueued_samples;
		m_funcs.set_freq_ratio   = &xa28_set_freq_ratio;

		LOG_SUCCESS(GENERAL, "XAudio 2.9 initialized");
		return;
	}

	if (auto lib2_8 = LoadLibraryExW(L"XAudio2_8.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
	{
		xa28_init(lib2_8);

		m_funcs.destroy    = &xa28_destroy;
		m_funcs.play       = &xa28_play;
		m_funcs.flush      = &xa28_flush;
		m_funcs.stop       = &xa28_stop;
		m_funcs.open       = &xa28_open;
		m_funcs.is_playing = &xa28_is_playing;
		m_funcs.add        = &xa28_add;
		m_funcs.enqueued_samples = &xa28_enqueued_samples;
		m_funcs.set_freq_ratio   = &xa28_set_freq_ratio;

		LOG_SUCCESS(GENERAL, "XAudio 2.8 initialized");
		return;
	}

	if (auto lib2_7 = LoadLibraryExW(L"XAudio2_7.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
	{
		xa27_init(lib2_7);

		m_funcs.destroy    = &xa27_destroy;
		m_funcs.play       = &xa27_play;
		m_funcs.flush      = &xa27_flush;
		m_funcs.stop       = &xa27_stop;
		m_funcs.open       = &xa27_open;
		m_funcs.is_playing = &xa27_is_playing;
		m_funcs.add        = &xa27_add;
		m_funcs.enqueued_samples = &xa27_enqueued_samples;
		m_funcs.set_freq_ratio   = &xa27_set_freq_ratio;

		LOG_SUCCESS(GENERAL, "XAudio 2.7 initialized");
		return;
	}

	fmt::throw_exception("No supported XAudio2 library found");
}

XAudio2Backend::~XAudio2Backend()
{
	m_funcs.destroy();
}

void XAudio2Backend::Play()
{
	m_funcs.play();
}

void XAudio2Backend::Close()
{
	m_funcs.stop();
	m_funcs.flush();
}

void XAudio2Backend::Pause()
{
	m_funcs.stop();
}

void XAudio2Backend::Open()
{
	m_funcs.open();
}

bool XAudio2Backend::IsPlaying()
{
	return m_funcs.is_playing();
}

bool XAudio2Backend::AddData(const void* src, int size)
{
	return m_funcs.add(src, size);
}

void XAudio2Backend::Flush()
{
	m_funcs.flush();
}

u64 XAudio2Backend::GetNumEnqueuedSamples()
{
	return m_funcs.enqueued_samples();
}

f32 XAudio2Backend::SetFrequencyRatio(f32 new_ratio)
{
	return m_funcs.set_freq_ratio(new_ratio);
}

#endif
