#pragma once

#ifdef _WIN32

#include "Emu/Audio/AudioThread.h"

class XAudio2Thread : public AudioThread
{
	void* const m_xaudio;

	static void xa27_init();
	static void xa27_destroy();
	static void xa27_play();
	static void xa27_flush();
	static void xa27_stop();
	static void xa27_open();
	static void xa27_add(const void*, int);

	static void xa29_init(void*);
	static void xa29_destroy();
	static void xa29_play();
	static void xa29_flush();
	static void xa29_stop();
	static void xa29_open();
	static void xa29_add(const void*, int);

public:
	XAudio2Thread();
	virtual ~XAudio2Thread() override;

	virtual void Play() override;
	virtual void Open(const void* src, int size) override;
	virtual void Close() override;
	virtual void Stop() override;
	virtual void AddData(const void* src, int size) override;
};

#endif
