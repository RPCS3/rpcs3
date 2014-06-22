#pragma once

struct rTimer
{
	rTimer();
	rTimer(const rTimer& other) = delete;
	~rTimer();
	void Start();
	void Stop();
private:
	void *handle;
};

void rSleep(u32 time);
void rMicroSleep(u64 time);

struct rThread
{
	static bool IsMain();
};

void rYieldIfNeeded();