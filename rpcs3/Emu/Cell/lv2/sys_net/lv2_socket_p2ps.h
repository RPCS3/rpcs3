#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include <netinet/in.h>
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
#endif

#include "lv2_socket_p2p.h"

struct nt_p2p_port;

constexpr be_t<u32> P2PS_U2S_SIG = (static_cast<u32>('U') << 24 | static_cast<u32>('2') << 16 | static_cast<u32>('S') << 8 | static_cast<u32>('0'));

struct p2ps_encapsulated_tcp
{
	be_t<u32> signature = P2PS_U2S_SIG; // Signature to verify it's P2P Stream data
	be_t<u32> length    = 0;            // Length of data
	be_t<u64> seq       = 0;            // This should be u32 but changed to u64 for simplicity
	be_t<u64> ack       = 0;
	be_t<u16> src_port  = 0; // fake source tcp port
	be_t<u16> dst_port  = 0; // fake dest tcp port(should be == vport)
	be_t<u16> checksum  = 0;
	u8 flags            = 0;
};

enum p2ps_stream_status
{
	stream_closed,      // Default when port is not listening nor connected
	stream_listening,   // Stream is listening, accepting SYN packets
	stream_handshaking, // Currently handshaking
	stream_connected,   // This is an established connection(after tcp handshake)
};

enum p2ps_tcp_flags : u8
{
	FIN = (1 << 0),
	SYN = (1 << 1),
	RST = (1 << 2),
	PSH = (1 << 3),
	ACK = (1 << 4),
	URG = (1 << 5),
	ECE = (1 << 6),
	CWR = (1 << 7),
};

u16 u2s_tcp_checksum(const le_t<u16>* buffer, usz size);
std::vector<u8> generate_u2s_packet(const p2ps_encapsulated_tcp& header, const u8* data, const u32 datasize);

class lv2_socket_p2ps final : public lv2_socket_p2p
{
public:
	static constexpr u32 id_type = 2;

	lv2_socket_p2ps(lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol);
	lv2_socket_p2ps(socket_type socket, u16 port, u16 vport, u32 op_addr, u16 op_port, u16 op_vport, u64 cur_seq, u64 data_beg_seq, s32 so_nbio);
	lv2_socket_p2ps(utils::serial& ar, lv2_socket_type type);
	void save(utils::serial& ar);

	p2ps_stream_status get_status() const;
	void set_status(p2ps_stream_status new_status);
	bool handle_connected(p2ps_encapsulated_tcp* tcp_header, u8* data, ::sockaddr_storage* op_addr, nt_p2p_port* p2p_port);
	bool handle_listening(p2ps_encapsulated_tcp* tcp_header, u8* data, ::sockaddr_storage* op_addr);
	void send_u2s_packet(std::vector<u8> data, const ::sockaddr_in* dst, u64 seq, bool require_ack);
	void close_stream();

	std::tuple<bool, s32, shared_ptr<lv2_socket>, sys_net_sockaddr> accept(bool is_lock = true) override;
	s32 bind(const sys_net_sockaddr& addr) override;

	std::optional<s32> connect(const sys_net_sockaddr& addr) override;

	std::pair<s32, sys_net_sockaddr> getpeername() override;
	std::pair<s32, sys_net_sockaddr> getsockname() override;

	s32 listen(s32 backlog) override;

	std::optional<std::tuple<s32, std::vector<u8>, sys_net_sockaddr>> recvfrom(s32 flags, u32 len, bool is_lock = true) override;
	std::optional<s32> sendto(s32 flags, const std::vector<u8>& buf, std::optional<sys_net_sockaddr> opt_sn_addr, bool is_lock = true) override;
	std::optional<s32> sendmsg(s32 flags, const sys_net_msghdr& msg, bool is_lock = true) override;

	void close() override;
	s32 shutdown(s32 how) override;

	s32 poll(sys_net_pollfd& sn_pfd, pollfd& native_pfd) override;
	std::tuple<bool, bool, bool> select(bs_t<poll_t> selected, pollfd& native_pfd) override;

private:
	void close_stream_nl(nt_p2p_port* p2p_port);

private:
	static constexpr usz MAX_RECEIVED_BUFFER = (1024 * 1024 * 10);

	p2ps_stream_status status = p2ps_stream_status::stream_closed;

	usz max_backlog = 0; // set on listen
	std::deque<s32> backlog;

	u16 op_port = 0, op_vport = 0;
	u32 op_addr = 0;

	u64 data_beg_seq   = 0;                       // Seq of first byte of received_data
	u64 data_available = 0;                       // Amount of continuous data available(calculated on ACK send)
	std::map<u64, std::vector<u8>> received_data; // holds seq/data of data received

	u64 cur_seq = 0; // SEQ of next packet to be sent
};
