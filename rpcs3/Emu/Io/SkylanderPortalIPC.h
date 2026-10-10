#pragma once
#include "Utilities/Thread.h"

struct SkylanderPortalIPCServer
{
	static constexpr auto thread_name = "Skylanders Portal IPC"sv;
	void operator()();
	SkylanderPortalIPCServer& operator=(thread_state);
};

class SkylanderPortalIPCServerManager
{
	using server_thread = named_thread<SkylanderPortalIPCServer>;
	std::unique_ptr<server_thread> m_server;
	int m_old_port = 0;

public:
	explicit SkylanderPortalIPCServerManager(bool enabled);
	void set_server_enabled(bool enabled);
};
