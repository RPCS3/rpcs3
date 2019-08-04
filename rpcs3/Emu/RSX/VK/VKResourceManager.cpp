#include "stdafx.h"
#include "VKResourceManager.h"

namespace vk
{
	resource_manager g_resource_manager;
	atomic_t<u64> g_event_ctr;

	resource_manager* get_resource_manager()
	{
		return &g_resource_manager;
	}

	u64 get_event_id()
	{
		return g_event_ctr++;
	}

	u64 current_event_id()
	{
		return g_event_ctr.load();
	}

	void on_event_completed(u64 event_id)
	{
		// TODO: Offload this to a secondary thread
		g_resource_manager.eid_completed(event_id);
	}
}
