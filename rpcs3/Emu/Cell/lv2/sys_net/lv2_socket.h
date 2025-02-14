#pragma once

#include <functional>
#include <optional>

#include "Utilities/mutex.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/lv2/sys_net.h"
#include "Emu/NP/ip_address.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include <poll.h>
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
#endif

enum class thread_state : u32;

class lv2_socket
{
public:
	// Poll events
	enum class poll_t
	{
		read,
		write,
		error,

		__bitset_enum_max
	};

	union sockopt_data
	{
		char ch[128];
		be_t<s32> _int = 0;
		sys_net_timeval timeo;
		sys_net_linger linger;
	};

	struct sockopt_cache
	{
		sockopt_data data{};
		s32 len = 0;
	};

public:
	SAVESTATE_INIT_POS(7); // Dependency on RPCN

	lv2_socket(lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol);
	lv2_socket(utils::serial&) {}
	lv2_socket(utils::serial&, lv2_socket_type type);
	static std::function<void(void*)> load(utils::serial& ar);
	void save(utils::serial&, bool save_only_this_class = false);
	virtual ~lv2_socket() noexcept;
	lv2_socket& operator=(thread_state s) noexcept;

	std::unique_lock<shared_mutex> lock();

	void set_lv2_id(u32 id);
	bs_t<poll_t> get_events() const;
	void set_poll_event(bs_t<poll_t> event);
	void poll_queue(shared_ptr<ppu_thread> ppu, bs_t<poll_t> event, std::function<bool(bs_t<poll_t>)> poll_cb);
	u32 clear_queue(ppu_thread*);
	void handle_events(const pollfd& native_fd, bool unset_connecting = false);
	void queue_wake(ppu_thread* ppu);

	lv2_socket_family get_family() const;
	lv2_socket_type get_type() const;
	lv2_ip_protocol get_protocol() const;
	std::size_t get_queue_size() const;
	socket_type get_socket() const;
#ifdef _WIN32
	bool is_connecting() const;
	void set_connecting(bool is_connecting);
#endif

public:
	virtual std::tuple<bool, s32, shared_ptr<lv2_socket>, sys_net_sockaddr> accept(bool is_lock = true) = 0;
	virtual s32 bind(const sys_net_sockaddr& addr) = 0;

	virtual std::optional<s32> connect(const sys_net_sockaddr& addr) = 0;
	virtual s32 connect_followup()                                   = 0;

	virtual std::pair<s32, sys_net_sockaddr> getpeername() = 0;
	virtual std::pair<s32, sys_net_sockaddr> getsockname() = 0;

	virtual std::tuple<s32, sockopt_data, u32> getsockopt(s32 level, s32 optname, u32 len) = 0;
	virtual s32 setsockopt(s32 level, s32 optname, const std::vector<u8>& optval)          = 0;

	virtual s32 listen(s32 backlog) = 0;

	virtual std::optional<std::tuple<s32, std::vector<u8>, sys_net_sockaddr>> recvfrom(s32 flags, u32 len, bool is_lock = true)                = 0;
	virtual std::optional<s32> sendto(s32 flags, const std::vector<u8>& buf, std::optional<sys_net_sockaddr> opt_sn_addr, bool is_lock = true) = 0;
	virtual std::optional<s32> sendmsg(s32 flags, const sys_net_msghdr& msg, bool is_lock = true)                                              = 0;

	virtual void close()          = 0;
	virtual s32 shutdown(s32 how) = 0;

	virtual s32 poll(sys_net_pollfd& sn_pfd, pollfd& native_pfd)                           = 0;
	virtual std::tuple<bool, bool, bool> select(bs_t<poll_t> selected, pollfd& native_pfd) = 0;

	error_code abort_socket(s32 flags);

public:
	// IDM data
	static const u32 id_base  = 24;
	static const u32 id_step  = 1;
	static const u32 id_count = 1000;

protected:
	lv2_socket(utils::serial&, bool);

	shared_mutex mutex;
	s32 lv2_id = 0;

	socket_type native_socket = 0;

	lv2_socket_family family{};
	lv2_socket_type type{};
	lv2_ip_protocol protocol{};

	// Events selected for polling
	atomic_bs_t<poll_t> events{};

	// Event processing workload (pair of thread id and the processing function)
	std::vector<std::pair<shared_ptr<ppu_thread>, std::function<bool(bs_t<poll_t>)>>> queue;

	// Socket options value keepers
	// Non-blocking IO option
	s32 so_nbio = 0;
	// Error, only used for connection result for non blocking stream sockets
	s32 so_error = 0;
	// Unsupported option
	s32 so_tcp_maxseg = 1500;
#ifdef _WIN32
	s32 so_reuseaddr = 0;
	s32 so_reuseport = 0;

	// Tracks connect for WSAPoll workaround
	bool connecting = false;
#endif

	sys_net_sockaddr last_bound_addr{};

public:
	u64 so_rcvtimeo = 0;
	u64 so_sendtimeo = 0;
};
