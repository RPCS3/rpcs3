#include "stdafx.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/Modules/sceNp.h" // for SCE_NP_PORT

#include "network_context.h"
#include "Emu/system_config.h"
#include "sys_net_helpers.h"

LOG_CHANNEL(sys_net);

// Used by RPCN to send signaling packets to RPCN server(for UDP hole punching)
s32 send_packet_from_p2p_port(const std::vector<u8>& data, const sockaddr_in& addr)
{
	s32 res{};
	auto& nc = g_fxo->get<p2p_context>();
	{
		std::lock_guard list_lock(nc.list_p2p_ports_mutex);
		if (nc.list_p2p_ports.contains(SCE_NP_PORT))
		{
			auto& def_port = ::at32(nc.list_p2p_ports, SCE_NP_PORT);
			res = ::sendto(def_port.p2p_socket, reinterpret_cast<const char*>(data.data()), ::size32(data), 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_in));

			if (res == -1)
				sys_net.error("Failed to send signaling packet: %s", get_last_error(false, false));
		}
		else
		{
			sys_net.error("send_packet_from_p2p_port: port %d not present", +SCE_NP_PORT);
		}
	}

	return res;
}

std::vector<std::vector<u8>> get_rpcn_msgs()
{
	std::vector<std::vector<u8>> msgs;
	auto& nc = g_fxo->get<p2p_context>();
	{
		std::lock_guard list_lock(nc.list_p2p_ports_mutex);
		if (nc.list_p2p_ports.contains(SCE_NP_PORT))
		{
			auto& def_port = ::at32(nc.list_p2p_ports, SCE_NP_PORT);
			{
				std::lock_guard lock(def_port.s_rpcn_mutex);
				msgs = std::move(def_port.rpcn_msgs);
				def_port.rpcn_msgs.clear();
			}
		}
		else
		{
			sys_net.error("get_rpcn_msgs: port %d not present", +SCE_NP_PORT);
		}
	}

	return msgs;
}

std::vector<signaling_message> get_sign_msgs()
{
	std::vector<signaling_message> msgs;
	auto& nc = g_fxo->get<p2p_context>();
	{
		std::lock_guard list_lock(nc.list_p2p_ports_mutex);
		if (nc.list_p2p_ports.contains(SCE_NP_PORT))
		{
			auto& def_port = ::at32(nc.list_p2p_ports, SCE_NP_PORT);
			{
				std::lock_guard lock(def_port.s_sign_mutex);
				msgs = std::move(def_port.sign_msgs);
				def_port.sign_msgs.clear();
			}
		}
		else
		{
			sys_net.error("get_sign_msgs: port %d not present", +SCE_NP_PORT);
		}
	}

	return msgs;
}

namespace np
{
	void init_np_handler_dependencies();
}

void base_network_thread::wake_threads()
{
	ppu_to_awake.erase(std::unique(ppu_to_awake.begin(), ppu_to_awake.end()), ppu_to_awake.end());
	for (ppu_thread* ppu : ppu_to_awake)
	{
		network_clear_queue(*ppu);
		lv2_obj::append(ppu);
	}

	if (!ppu_to_awake.empty())
	{
		lv2_obj::awake_all();
	}
	ppu_to_awake.clear();
}

p2p_thread::p2p_thread()
{
	np::init_np_handler_dependencies();
}

void p2p_thread::bind_sce_np_port()
{
	std::lock_guard list_lock(list_p2p_ports_mutex);
	create_p2p_port(SCE_NP_PORT);
}

void network_thread::operator()()
{
	std::vector<std::shared_ptr<lv2_socket>> socklist;
	socklist.reserve(lv2_socket::id_count);

	ppu_to_awake.clear();

	std::vector<::pollfd> fds(lv2_socket::id_count);
#ifdef _WIN32
	std::vector<bool> connecting(lv2_socket::id_count);
	std::vector<bool> was_connecting(lv2_socket::id_count);
#endif

	while (thread_ctrl::state() != thread_state::aborting)
	{
		if (!num_polls)
		{
			thread_ctrl::wait_on(num_polls, 0);
			continue;
		}

		ensure(socklist.size() <= lv2_socket::id_count);

		// Wait with 1ms timeout
#ifdef _WIN32
		windows_poll(fds, ::size32(socklist), 1, connecting);
#else
		::poll(fds.data(), socklist.size(), 1);
#endif

		std::lock_guard lock(mutex_thread_loop);

		for (usz i = 0; i < socklist.size(); i++)
		{
#ifdef _WIN32
			socklist[i]->handle_events(fds[i], was_connecting[i] && !connecting[i]);
#else
			socklist[i]->handle_events(fds[i]);
#endif
		}

		wake_threads();
		socklist.clear();

		// Obtain all native active sockets
		idm::select<lv2_socket>([&](u32 id, lv2_socket& s)
			{
				if (s.get_type() == SYS_NET_SOCK_DGRAM || s.get_type() == SYS_NET_SOCK_STREAM)
				{
					socklist.emplace_back(idm::get_unlocked<lv2_socket>(id));
				}
			});

		for (usz i = 0; i < socklist.size(); i++)
		{
			auto events = socklist[i]->get_events();

			fds[i].fd = events ? socklist[i]->get_socket() : -1;
			fds[i].events =
				(events & lv2_socket::poll_t::read ? POLLIN : 0) |
				(events & lv2_socket::poll_t::write ? POLLOUT : 0) |
				0;
			fds[i].revents = 0;
#ifdef _WIN32
			const auto cur_connecting = socklist[i]->is_connecting();
			was_connecting[i] = cur_connecting;
			connecting[i] = cur_connecting;
#endif
		}
	}
}

// Must be used under list_p2p_ports_mutex lock!
void p2p_thread::create_p2p_port(u16 p2p_port)
{
	if (!list_p2p_ports.contains(p2p_port))
	{
		list_p2p_ports.emplace(std::piecewise_construct, std::forward_as_tuple(p2p_port), std::forward_as_tuple(p2p_port));
		const u32 prev_value = num_p2p_ports.fetch_add(1);
		if (!prev_value)
		{
			num_p2p_ports.notify_one();
		}
	}
}

void p2p_thread::operator()()
{
	std::vector<::pollfd> p2p_fd(lv2_socket::id_count);

	while (thread_ctrl::state() != thread_state::aborting)
	{
		if (!num_p2p_ports)
		{
			thread_ctrl::wait_on(num_p2p_ports, 0);
			continue;
		}

		// Check P2P sockets for incoming packets
		auto num_p2p_sockets = 0;
		std::memset(p2p_fd.data(), 0, p2p_fd.size() * sizeof(::pollfd));
		{
			std::lock_guard lock(list_p2p_ports_mutex);
			for (const auto& p2p_port : list_p2p_ports)
			{
				p2p_fd[num_p2p_sockets].events = POLLIN;
				p2p_fd[num_p2p_sockets].revents = 0;
				p2p_fd[num_p2p_sockets].fd = p2p_port.second.p2p_socket;
				num_p2p_sockets++;
			}
		}

#ifdef _WIN32
		const auto ret_p2p = WSAPoll(p2p_fd.data(), num_p2p_sockets, 1);
#else
		const auto ret_p2p = ::poll(p2p_fd.data(), num_p2p_sockets, 1);
#endif
		if (ret_p2p > 0)
		{
			std::lock_guard lock(list_p2p_ports_mutex);
			auto fd_index = 0;
			for (auto& p2p_port : list_p2p_ports)
			{
				if ((p2p_fd[fd_index].revents & POLLIN) == POLLIN || (p2p_fd[fd_index].revents & POLLRDNORM) == POLLRDNORM)
				{
					while (p2p_port.second.recv_data())
						;
				}
				fd_index++;
			}

			wake_threads();
		}
		else if (ret_p2p < 0)
		{
			sys_net.error("[P2P] Error poll on master P2P socket: %d", get_last_error(false));
		}
	}
}
