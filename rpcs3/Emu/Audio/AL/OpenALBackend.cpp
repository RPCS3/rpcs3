#include "stdafx.h"

#include "OpenALBackend.h"

#ifdef _MSC_VER
#pragma comment(lib, "OpenAL32.lib")
#endif

LOG_CHANNEL(OpenAL);

#define checkForAlError(sit) do { ALenum g_last_al_error = alGetError(); if(g_last_al_error != AL_NO_ERROR) OpenAL.error("%s: OpenAL error 0x%04x", sit, g_last_al_error); } while(0)
#define checkForAlcError(sit) do { ALCenum g_last_alc_error = alcGetError(m_device); if(g_last_alc_error != ALC_NO_ERROR) { OpenAL.error("%s: OpenALC error 0x%04x", sit, g_last_alc_error); return; }} while(0)

OpenALBackend::OpenALBackend()
	: m_sampling_rate(get_sampling_rate())
	, m_sample_size(get_sample_size())
{
	ALCdevice* m_device = alcOpenDevice(nullptr);
	checkForAlcError("alcOpenDevice");

	ALCcontext* m_context = alcCreateContext(m_device, nullptr);
	checkForAlcError("alcCreateContext");

	alcMakeContextCurrent(m_context);
	checkForAlcError("alcMakeContextCurrent");

	if (get_channels() == 2)
	{
		m_format = (m_sample_size == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_STEREO_FLOAT32;
	}
	else
	{
		m_format = (m_sample_size == 2) ? AL_FORMAT_71CHN16 : AL_FORMAT_71CHN32;
	}
}

OpenALBackend::~OpenALBackend()
{
	if (alIsSource(m_source))
	{
		Close();
	}

	if (ALCcontext* m_context = alcGetCurrentContext())
	{
		ALCdevice* m_device = alcGetContextsDevice(m_context);
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(m_context);
		alcCloseDevice(m_device);
	}
}

void OpenALBackend::Play()
{
	AUDIT(alIsSource(m_source));

	ALint state;
	alGetSourcei(m_source, AL_SOURCE_STATE, &state);
	checkForAlError("Play->alGetSourcei(AL_SOURCE_STATE)");

	if (state != AL_PLAYING)
	{
		alSourcePlay(m_source);
		checkForAlError("Play->alSourcePlay");
	}
}

void OpenALBackend::Pause()
{
	AUDIT(alIsSource(m_source));

	alSourcePause(m_source);
	checkForAlError("Pause->alSourcePause");
}

bool OpenALBackend::IsPlaying()
{
	AUDIT(alIsSource(m_source));

	ALint state;
	alGetSourcei(m_source, AL_SOURCE_STATE, &state);
	checkForAlError("IsPlaying->alGetSourcei(AL_SOURCE_STATE)");

	return state == AL_PLAYING;
}

void OpenALBackend::Open(u32 num_buffers)
{
	AUDIT(!alIsSource(m_source));

	// Initialize Source
	alGenSources(1, &m_source);
	checkForAlError("Open->alGenSources");

	alSourcei(m_source, AL_LOOPING, AL_FALSE);
	checkForAlError("Open->alSourcei");

	// Initialize Buffers
	alGenBuffers(num_buffers, m_buffers);
	checkForAlError("Open->alGenBuffers");

	m_num_buffers = num_buffers;
	m_num_unqueued = num_buffers;
}

void OpenALBackend::Close()
{
	if (alIsSource(m_source))
	{
		// Stop & Kill Source
		Pause();
		alDeleteSources(1, &m_source);

		// Free Buffers
		alDeleteBuffers(m_num_buffers, m_buffers);
		checkForAlError("alDeleteBuffers");
	}
}

bool OpenALBackend::AddData(const void* src, u32 num_samples)
{
	AUDIT(alIsSource(m_source));

	// Unqueue processed buffers, if any
	unqueue_processed();

	// Fail if there are no free buffers remaining
	if (m_num_unqueued == 0)
	{
		OpenAL.warning("No unqueued buffers remaining");
		return false;
	}

	// Copy data to the next available buffer
	alBufferData(m_buffers[m_next_buffer], m_format, src, num_samples * m_sample_size, m_sampling_rate);
	checkForAlError("AddData->alBufferData");

	// Enqueue buffer
	alSourceQueueBuffers(m_source, 1, &m_buffers[m_next_buffer]);
	checkForAlError("AddData->alSourceQueueBuffers");

	m_num_unqueued--;
	m_next_buffer = (m_next_buffer + 1) % m_num_buffers;

	return true;
}

void OpenALBackend::Flush()
{
	AUDIT(alIsSource(m_source));

	// Stop source first
	alSourceStop(m_source);
	checkForAlError("Flush->alSourceStop");

	// Unqueue processed buffers (should now be all of them)
	unqueue_processed();
}

void OpenALBackend::unqueue_processed()
{
	AUDIT(alIsSource(m_source));

	// Get number of buffers
	ALint num_processed;
	alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &num_processed);
	checkForAlError("Flush->alGetSourcei(AL_BUFFERS_PROCESSED)");

	if (num_processed > 0)
	{
		// Unqueue all buffers
		ALuint x[MAX_AUDIO_BUFFERS];
		alSourceUnqueueBuffers(m_source, num_processed, x);
		checkForAlError("Flush->alSourceUnqueueBuffers");

		m_num_unqueued += num_processed;
	}
}

u64 OpenALBackend::GetNumEnqueuedSamples()
{
	AUDIT(alIsSource(m_source));

	// Get number of buffers queued
	ALint num_queued;
	alGetSourcei(m_source, AL_BUFFERS_QUEUED, &num_queued);
	checkForAlError("GetNumEnqueuedSamples->alGetSourcei(AL_BUFFERS_QUEUED)");
	AUDIT(static_cast<u32>(num_queued) <= m_num_buffers - m_num_unqueued);

	// Get sample position
	ALint sample_pos;
	alGetSourcei(m_source, AL_SAMPLE_OFFSET, &sample_pos);
	checkForAlError("GetNumEnqueuedSamples->alGetSourcei(AL_SAMPLE_OFFSET)");

	// Return
	return (num_queued * AUDIO_BUFFER_SAMPLES) + (sample_pos % AUDIO_BUFFER_SAMPLES);
}

f32 OpenALBackend::SetFrequencyRatio(f32 new_ratio)
{
	new_ratio = std::clamp(new_ratio, 0.5f, 2.0f);

	alSourcef(m_source, AL_PITCH, new_ratio);
	checkForAlError("SetFrequencyRatio->alSourcei(AL_PITCH)");

	return new_ratio;
}