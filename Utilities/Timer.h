#pragma once

class Timer
{
private:
	bool stopped;
	double startTimeInMicroSec;
    double endTimeInMicroSec;
	LARGE_INTEGER frequency;
	LARGE_INTEGER startCycle;
	LARGE_INTEGER endCycle;

public:
	Timer()
	{
		QueryPerformanceFrequency(&frequency);
		startCycle.QuadPart = 0;
		endCycle.QuadPart = 0;
		stopped = false;
		startTimeInMicroSec = 0;
		endTimeInMicroSec = 0;
	}

	void Start()
	{
		stopped = false;
		QueryPerformanceCounter(&startCycle);
	}

	void Stop()
	{
		stopped = true;
		QueryPerformanceCounter(&endCycle);
	}

	double GetElapsedTimeInSec(){return GetElapsedTimeInMicroSec() / 1000000.0;}
    double GetElapsedTimeInMilliSec(){return GetElapsedTimeInMicroSec() / 1000.0;}
    double GetElapsedTimeInMicroSec()
	{
		if(!stopped) QueryPerformanceCounter(&endCycle);
		
		startTimeInMicroSec = startCycle.QuadPart * (1000000.0 / frequency.QuadPart);
		endTimeInMicroSec = endCycle.QuadPart * (1000000.0 / frequency.QuadPart);

		return endTimeInMicroSec - startTimeInMicroSec;
	}
};