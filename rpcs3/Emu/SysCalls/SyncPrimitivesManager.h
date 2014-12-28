#pragma once

struct SemaphoreAttributes
{
	std::string name;
	s32 count;
	s32 max_count;
};

struct LwMutexAttributes
{
	std::string name;
};

class SyncPrimManager
{
private:

public:
	SemaphoreAttributes GetSemaphoreData(u32 id);
	LwMutexAttributes GetLwMutexData(u32 id);
	std::string GetSyncPrimName(u32 id, IDType type);

	void Close()
	{
	}
};