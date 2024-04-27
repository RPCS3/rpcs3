#include "stdafx.h"
#include "lv2_socket.h"

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
	return socket;
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

void lv2_socket::poll_queue(std::shared_ptr<ppu_thread> ppu, bs_t<lv2_socket::poll_t> event, std::function<bool(bs_t<lv2_socket::poll_t>)> poll_cb)
{
	set_poll_event(event);
	queue.emplace_back(std::move(ppu), poll_cb);
}

s32 lv2_socket::clear_queue(ppu_thread* ppu)
{
	std::lock_guard lock(mutex);

	s32 cleared = 0;

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

		for (auto it = queue.begin(); it != queue.end();)
		{
			if (it->second(events_happening))
			{
				it = queue.erase(it);
				continue;
			}

			it++;
		}

		if (queue.empty())
		{
			events.store({});
		}
	}
}
