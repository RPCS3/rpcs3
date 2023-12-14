#pragma once

#include <vector>
#include <map>
#include "Utilities/mutex.h"
#include "Emu/Cell/PPUThread.h"

#include "nt_p2p_port.h"

struct network_thread
{
	std::vector<ppu_thread*> s_to_awake;
	shared_mutex s_nw_mutex;

	shared_mutex list_p2p_ports_mutex;
	std::map<u16, nt_p2p_port> list_p2p_ports{};

	static constexpr auto thread_name = "Network Thread";

	network_thread();
	void bind_sce_np_port();
	void operator()();
};

using network_context = named_thread<network_thread>;
