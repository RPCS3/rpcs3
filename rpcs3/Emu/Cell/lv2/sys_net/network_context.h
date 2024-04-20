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
	atomic_t<u32> num_polls = 0;

	static constexpr auto thread_name = "Network Thread";

	void operator()();
};

struct p2p_thread
{
	shared_mutex list_p2p_ports_mutex;
	std::map<u16, nt_p2p_port> list_p2p_ports;
	atomic_t<u32> num_p2p_ports = 0;

	static constexpr auto thread_name = "Network P2P Thread";

	p2p_thread();

	void create_p2p_port(u16 p2p_port);

	void bind_sce_np_port();
	void operator()();
};

using network_context = named_thread<network_thread>;
using p2p_context = named_thread<p2p_thread>;
