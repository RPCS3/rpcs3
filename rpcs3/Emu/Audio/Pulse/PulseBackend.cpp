#ifndef HAVE_PULSE
#error "PulseAudio support disabled but still being built."
#endif

#include "Emu/System.h"
#include "PulseBackend.h"

#include <pulse/simple.h>
#include <pulse/error.h>

PulseBackend::PulseBackend()
	: AudioBackend()
{
}

PulseBackend::~PulseBackend()
{
	this->Close();
}

void PulseBackend::Close()
{
	if (this->connection)
	{
		pa_simple_free(this->connection);
		this->connection = nullptr;
	}
}

void PulseBackend::Open(u32 /* num_buffers */)
{
	pa_sample_spec ss;
	ss.format = (m_sample_size == 2) ? PA_SAMPLE_S16LE : PA_SAMPLE_FLOAT32LE;
	ss.rate = m_sampling_rate;

	pa_channel_map channel_map;

	if (m_channels == 2)
	{
		channel_map.channels = 2;
		channel_map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
		channel_map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
	}
	else if (m_channels == 6)
	{
		channel_map.channels = 6;
		channel_map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
		channel_map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
		channel_map.map[2] = PA_CHANNEL_POSITION_FRONT_CENTER;
		channel_map.map[3] = PA_CHANNEL_POSITION_LFE;
		channel_map.map[4] = PA_CHANNEL_POSITION_REAR_LEFT;
		channel_map.map[5] = PA_CHANNEL_POSITION_REAR_RIGHT;
	}
	else
	{
		channel_map.channels = 8;
		channel_map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
		channel_map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
		channel_map.map[2] = PA_CHANNEL_POSITION_FRONT_CENTER;
		channel_map.map[3] = PA_CHANNEL_POSITION_LFE;
		channel_map.map[4] = PA_CHANNEL_POSITION_REAR_LEFT;
		channel_map.map[5] = PA_CHANNEL_POSITION_REAR_RIGHT;
		channel_map.map[6] = PA_CHANNEL_POSITION_SIDE_LEFT;
		channel_map.map[7] = PA_CHANNEL_POSITION_SIDE_RIGHT;
	}
	ss.channels = channel_map.channels;

	int err;
	this->connection = pa_simple_new(NULL, "RPCS3", PA_STREAM_PLAYBACK, NULL, "Game", &ss, &channel_map, NULL, &err);
	if (!this->connection)
	{
		fprintf(stderr, "PulseAudio: Failed to initialize audio: %s\n", pa_strerror(err));
	}
}

bool PulseBackend::AddData(const void* src, u32 num_samples)
{
	AUDIT(this->connection);

	int err;
	if (pa_simple_write(this->connection, src, num_samples * m_sample_size, &err) < 0)
	{
		fprintf(stderr, "PulseAudio: Failed to write audio stream: %s\n", pa_strerror(err));
		return false;
	}

	return true;
}
