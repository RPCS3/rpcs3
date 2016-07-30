#pragma once

#ifdef _WIN32

#include "Emu/Audio/AudioThread.h"

class XAudio2Thread : public AudioThread
{
	struct vtable
	{
		void(*destroy)();
		void(*play)();
		void(*flush)();
		void(*stop)();
		void(*open)();
		void(*add)(const void*, int);
	};

	vtable m_funcs;

	static void xa27_init(void*);
	static void xa27_destroy();
	static void xa27_play();
	static void xa27_flush();
	static void xa27_stop();
	static void xa27_open();
	static void xa27_add(const void*, int);

	static void xa28_init(void*);
	static void xa28_destroy();
	static void xa28_play();
	static void xa28_flush();
	static void xa28_stop();
	static void xa28_open();
	static void xa28_add(const void*, int);

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
