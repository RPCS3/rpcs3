#pragma once

#ifdef _WIN32

#include "Emu/Audio/AudioBackend.h"


class XAudio2Backend : public AudioBackend
{
public:
	class XAudio2Library
	{
	public:
		virtual void play() = 0;
		virtual void flush() = 0;
		virtual void stop() = 0;
		virtual void open() = 0;
		virtual bool is_playing() = 0;
		virtual bool add(const void*, u32) = 0;
		virtual u64 enqueued_samples() = 0;
		virtual f32 set_freq_ratio(f32) = 0;
	};

private:
	static XAudio2Library* xa27_init(void*);
	static XAudio2Library* xa28_init(void*);

	std::unique_ptr<XAudio2Library> lib = nullptr;

public:
	XAudio2Backend();
	virtual ~XAudio2Backend() override;

	virtual const char* GetName() const override { return "XAudio2"; };

	static const u32 capabilities = PLAY_PAUSE_FLUSH | IS_PLAYING | GET_NUM_ENQUEUED_SAMPLES | SET_FREQUENCY_RATIO;
	virtual u32 GetCapabilities() const override { return capabilities;	};

	virtual void Open(u32 /* num_buffers */) override;
	virtual void Close() override;

	virtual void Play() override;
	virtual void Pause() override;
	virtual bool IsPlaying() override;

	virtual bool AddData(const void* src, u32 num_samples) override;
	virtual void Flush() override;

	virtual u64 GetNumEnqueuedSamples() override;
	virtual f32 SetFrequencyRatio(f32 new_ratio) override;
};

#endif
