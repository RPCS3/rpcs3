#include "Emu/System.h"
#include "PulseThread.h"

#ifdef HAVE_PULSE

#include <pulse/simple.h>
#include <pulse/error.h>

PulseThread::PulseThread()
{
}

PulseThread::~PulseThread()
{
	this->Close();
}

void PulseThread::Play()
{
}

void PulseThread::Close()
{
	if(this->connection) {
		pa_simple_free(this->connection);
		this->connection = nullptr;
	}
}

void PulseThread::Stop()
{
}

void PulseThread::Open(const void* src, int size)
{
	pa_sample_spec ss;
	ss.format = g_cfg.audio.convert_to_u16 ? PA_SAMPLE_S16LE : PA_SAMPLE_FLOAT32LE;
	ss.rate = 48000;

	pa_channel_map channel_map;

	if (g_cfg.audio.downmix_to_2ch)
	{
		channel_map.channels =  2;
		channel_map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
		channel_map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
	}
	else
	{
		channel_map.channels =  8;
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
	if(!this->connection) {
		fprintf(stderr, "PulseAudio: Failed to initialize audio: %s\n", pa_strerror(err));
	}

	this->AddData(src, size);
}

void PulseThread::AddData(const void* src, int size)
{
	if(this->connection) {
		int err;
		if(pa_simple_write(this->connection, src, size, &err) < 0) {
			fprintf(stderr, "PulseAudio: Failed to write audio stream: %s\n", pa_strerror(err));
		}
	}
}

#endif
