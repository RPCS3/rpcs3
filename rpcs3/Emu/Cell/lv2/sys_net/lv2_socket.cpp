#include "stdafx.h"
#include "lv2_socket.h"
#include "network_context.h"

LOG_CHANNEL(sys_net);

lv2_socket::lv2_socket(lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol)
{
	this->family   = family;
	this->type     = type;
	this->protocol = protocol;
}

std::unique_lock<shared_mutex> lv2_socket::lock()
{
	return std::unique_lock(mutex);
}

lv2_socket_family lv2_socket::get_family() const
{
	return family;
}

lv2_socket_type lv2_socket::get_type() const
{
	return type;
}
lv2_ip_protocol lv2_socket::get_protocol() const
{
	return protocol;
}
std::size_t lv2_socket::get_queue_size() const
{
	return queue.size();
}
socket_type lv2_socket::get_socket() const
{
	return native_socket;
}

#ifdef _WIN32
bool lv2_socket::is_connecting() const
{
	return connecting;
}
void lv2_socket::set_connecting(bool connecting)
{
	this->connecting = connecting;
}
#endif

void lv2_socket::set_lv2_id(u32 id)
{
	lv2_id = id;
}

bs_t<lv2_socket::poll_t> lv2_socket::get_events() const
{
	return events.load();
}

void lv2_socket::set_poll_event(bs_t<lv2_socket::poll_t> event)
{
	events += event;
}

void lv2_socket::poll_queue(shared_ptr<ppu_thread> ppu, bs_t<lv2_socket::poll_t> event, std::function<bool(bs_t<lv2_socket::poll_t>)> poll_cb)
{
	set_poll_event(event);
	queue.emplace_back(std::move(ppu), poll_cb);

	// Makes sure network_context thread is awaken
	if (type == SYS_NET_SOCK_STREAM || type == SYS_NET_SOCK_DGRAM)
	{
		auto& nc = g_fxo->get<network_context>();
		const u32 prev_value = nc.num_polls.fetch_add(1);
		if (!prev_value)
		{
			nc.num_polls.notify_one();
		}
	}
}

u32 lv2_socket::clear_queue(ppu_thread* ppu)
{
	std::lock_guard lock(mutex);

	u32 cleared = 0;

	for (auto it = queue.begin(); it != queue.end();)
	{
		if (it->first.get() == ppu)
		{
			it = queue.erase(it);
			cleared++;
			continue;
		}

		it++;
	}

	if (queue.empty())
	{
		events.store({});
	}

	if (cleared && (type == SYS_NET_SOCK_STREAM || type == SYS_NET_SOCK_DGRAM))
	{
		// Makes sure network_context thread can go back to sleep if there is no active polling
		const u32 prev_value = g_fxo->get<network_context>().num_polls.fetch_sub(cleared);
		ensure(prev_value >= cleared);
	}

	return cleared;
}

void lv2_socket::handle_events(const pollfd& native_pfd, [[maybe_unused]] bool unset_connecting)
{
	bs_t<lv2_socket::poll_t> events_happening{};

	if (native_pfd.revents & (POLLIN | POLLHUP) && events.test_and_reset(lv2_socket::poll_t::read))
		events_happening += lv2_socket::poll_t::read;
	if (native_pfd.revents & POLLOUT && events.test_and_reset(lv2_socket::poll_t::write))
		events_happening += lv2_socket::poll_t::write;
	if (native_pfd.revents & POLLERR && events.test_and_reset(lv2_socket::poll_t::error))
		events_happening += lv2_socket::poll_t::error;

	if (events_happening || (!queue.empty() && (so_rcvtimeo || so_sendtimeo)))
	{
		std::lock_guard lock(mutex);
#ifdef _WIN32
		if (unset_connecting)
			set_connecting(false);
#endif
		u32 handled = 0;

		for (auto it = queue.begin(); it != queue.end();)
		{
			if (it->second(events_happening))
			{
				it = queue.erase(it);
				handled++;
				continue;
			}

			it++;
		}

		if (handled && (type == SYS_NET_SOCK_STREAM || type == SYS_NET_SOCK_DGRAM))
		{
			const u32 prev_value = g_fxo->get<network_context>().num_polls.fetch_sub(handled);
			ensure(prev_value >= handled);
		}

		if (queue.empty())
		{
			events.store({});
		}
	}
}

void lv2_socket::queue_wake(ppu_thread* ppu)
{
	switch (type)
	{
	case SYS_NET_SOCK_STREAM:
	case SYS_NET_SOCK_DGRAM:
		g_fxo->get<network_context>().add_ppu_to_awake(ppu);
		break;
	case SYS_NET_SOCK_DGRAM_P2P:
	case SYS_NET_SOCK_STREAM_P2P:
		g_fxo->get<p2p_context>().add_ppu_to_awake(ppu);
		break;
	default:
		break;
	}
}

lv2_socket& lv2_socket::operator=(thread_state s) noexcept
{
	if (s == thread_state::destroying_context)
	{
		close();
	}

	return *this;
}

lv2_socket::~lv2_socket() noexcept
{
}
