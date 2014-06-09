#pragma once

enum rSemaStatus
{
	rSEMA_BUSY,
	rSEMA_OTHER
};
struct rSemaphore
{
	rSemaphore();
	rSemaphore(const rSemaphore& other) = delete;
	~rSemaphore();
	rSemaphore(int initial_count, int max_count);
	void Wait();
	rSemaStatus TryWait();
	void Post();
	void WaitTimeout(u64 timeout);
private:
	void *handle;
};

struct rCriticalSection
{
	rCriticalSection();
	rCriticalSection(const rCriticalSection& other) = delete;
	~rCriticalSection();
	void Enter();
	void Leave();
	void *handle;
};

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

struct rCriticalSectionLocker
{
	rCriticalSectionLocker(const rCriticalSection& other);
	~rCriticalSectionLocker();
private:
	void *handle;
};

struct rThread
{
	static bool IsMain();
};

void rYieldIfNeeded();