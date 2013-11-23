#pragma once

#include <chrono>
using namespace std::chrono;

class Timer
{
private:
	bool stopped;
	high_resolution_clock::time_point start;
	high_resolution_clock::time_point end;

public:
	Timer() : stopped(false)
	{
	}

	void Start()
	{
		stopped = false;
        start = high_resolution_clock::now();
	}

	void Stop()
	{
		stopped = true;
        end = high_resolution_clock::now();
	}

	double GetElapsedTimeInSec(){return GetElapsedTimeInMicroSec() / 1000000.0;}
    double GetElapsedTimeInMilliSec(){return GetElapsedTimeInMicroSec() / 1000.0;}
    double GetElapsedTimeInMicroSec()
	{
		if (!stopped)
            end = high_resolution_clock::now();
		return duration_cast<microseconds>(end - start).count();
	}
};
