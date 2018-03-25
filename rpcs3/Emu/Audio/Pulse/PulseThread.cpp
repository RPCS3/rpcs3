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
	ss.channels = g_cfg.audio.downmix_to_2ch ? 2 : 8;
	ss.rate = 48000;

	int err;
	this->connection = pa_simple_new(NULL, "RPCS3", PA_STREAM_PLAYBACK, NULL, "Game", &ss, NULL, NULL, &err);
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
