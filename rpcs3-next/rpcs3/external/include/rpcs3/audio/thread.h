#pragma once

class AudioThread
{
public:
	virtual ~AudioThread() {}

	virtual void Init() = 0;
	virtual void Quit() = 0;
	virtual void Play() = 0;
	virtual void Open(const void* src, int size) = 0;
	virtual void Close() = 0;
	virtual void Stop() = 0;
	virtual void AddData(const void* src, int size) = 0;
};
