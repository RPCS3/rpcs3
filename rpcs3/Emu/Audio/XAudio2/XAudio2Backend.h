#pragma once

#ifdef _WIN32

#include "Emu/Audio/AudioBackend.h"

class XAudio2Backend : public AudioBackend
{
	struct vtable
	{
		void(*destroy)();
		void(*play)();
		void(*flush)();
		void(*stop)();
		void(*open)();
		bool(*is_playing)();
		bool(*add)(const void*, int);
		u64(*enqueued_samples)();
	};

	vtable m_funcs;

	static void xa27_init(void*);
	static void xa27_destroy();
	static void xa27_play();
	static void xa27_flush();
	static void xa27_stop();
	static void xa27_open();
	static bool xa27_is_playing();
	static bool xa27_add(const void*, int);
	static u64  xa27_enqueued_samples();

	static void xa28_init(void*);
	static void xa28_destroy();
	static void xa28_play();
	static void xa28_flush();
	static void xa28_stop();
	static void xa28_open();
	static bool xa28_is_playing();
	static bool xa28_add(const void*, int);
	static u64  xa28_enqueued_samples();

public:
	XAudio2Backend();
	virtual ~XAudio2Backend() override;

	virtual const char* GetName() const override { return "XAudio2Backend"; };

	static const u32 capabilities = NON_BLOCKING | IS_PLAYING | GET_NUM_ENQUEUED_SAMPLES;
	virtual u32 GetCapabilities() const override { return capabilities;	};

	virtual void Open() override;
	virtual void Close() override;

	virtual void Play() override;
	virtual void Pause() override;
	virtual bool IsPlaying() override;

	virtual bool AddData(const void* src, int size) override;
	virtual void Flush() override;

	virtual u64 GetNumEnqueuedSamples() override;
};

#endif
