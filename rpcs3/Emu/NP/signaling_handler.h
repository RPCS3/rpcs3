#pragma once
#include "Emu/Memory/vm.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Utilities/Thread.h"
#include <unordered_map>
#include <condition_variable>
#include <chrono>

enum ext_signaling_status : u8
{
	ext_sign_none = 0,
	ext_sign_peer = 1,
	ext_sign_mutual = 2,
};

struct signaling_info
{
	s32 connStatus = SCE_NP_SIGNALING_CONN_STATUS_INACTIVE;
	u32 addr       = 0;
	u16 port       = 0;

	// For handler
	steady_clock::time_point time_last_msg_recvd = steady_clock::now();

	bool self   = false;
	u32 version = 0;
	// Signaling
	u32 conn_id                     = 0;
	ext_signaling_status ext_status = ext_sign_none;
	// Matching2
	u64 room_id   = 0;
	u16 member_id = 0;
};

enum SignalingCommand : u32
{
	signal_ping,
	signal_pong,
	signal_connect,
	signal_connect_ack,
	signal_confirm,
	signal_finished,
	signal_finished_ack,
};

class signaling_handler : public need_wakeup
{
public:
	void operator()();
	void wake_up();

	void set_self_sig_info(SceNpId& npid);
	void set_self_sig2_info(u64 room_id, u16 member_id);

	u32 init_sig_infos(const SceNpId* npid);
	signaling_info get_sig_infos(u32 conn_id);

	void set_sig2_infos(u64 room_id, u16 member_id, s32 status, u32 addr, u16 port, bool self = false);
	signaling_info get_sig2_infos(u64 room_id, u16 member_id);

	void set_sig_cb(u32 sig_cb_ctx, vm::ptr<SceNpSignalingHandler> sig_cb, vm::ptr<void> sig_cb_arg);
	void set_ext_sig_cb(u32 sig_ext_cb_ctx, vm::ptr<SceNpSignalingHandler> sig_ext_cb, vm::ptr<void> sig_ext_cb_arg);
	void set_sig2_cb(u16 sig2_cb_ctx, vm::ptr<SceNpMatching2SignalingCallback> sig2_cb, vm::ptr<void> sig2_cb_arg);

	void start_sig(u32 conn_id, u32 addr, u16 port);

	void start_sig2(u64 room_id, u16 member_id);
	void disconnect_sig2_users(u64 room_id);

	static constexpr auto thread_name = "Signaling Manager Thread"sv;

private:
	static constexpr auto REPEAT_CONNECT_DELAY  = std::chrono::milliseconds(200);
	static constexpr auto REPEAT_PING_DELAY     = std::chrono::milliseconds(500);
	static constexpr auto REPEAT_FINISHED_DELAY = std::chrono::milliseconds(500);
	static constexpr be_t<u32> SIGNALING_SIGNATURE = (static_cast<u32>('S') << 24 | static_cast<u32>('I') << 16 | static_cast<u32>('G') << 8 | static_cast<u32>('N'));

	struct signaling_packet
	{
		be_t<u32> signature = SIGNALING_SIGNATURE;
		le_t<u32> version;
		le_t<SignalingCommand> command;
		union {
			struct
			{
				SceNpId npid;
			} V1;
			struct
			{
				le_t<u64> room_id;
				le_t<u16> member_id;
			} V2;
		};
	};

	struct queued_packet
	{
		signaling_packet packet;
		std::shared_ptr<signaling_info> sig_info;
	};

	u32 sig_cb_ctx = 0;
	vm::ptr<SceNpSignalingHandler> sig_cb{};
	vm::ptr<void> sig_cb_arg{};

	u32 sig_ext_cb_ctx = 0;
	vm::ptr<SceNpSignalingHandler> sig_ext_cb{};
	vm::ptr<void> sig_ext_cb_arg{};

	u16 sig2_cb_ctx = 0;
	vm::ptr<SceNpMatching2SignalingCallback> sig2_cb{};
	vm::ptr<void> sig2_cb_arg{};

	u32 create_sig_infos(const SceNpId* npid);
	static void update_si_addr(std::shared_ptr<signaling_info>& si, u32 new_addr, u16 new_port);
	void update_si_status(std::shared_ptr<signaling_info>& si, s32 new_status, bool confirm_packet = false);
	void signal_sig_callback(u32 conn_id, int event);
	void signal_ext_sig_callback(u32 conn_id, int event) const;
	void signal_sig2_callback(u64 room_id, u16 member_id, SceNpMatching2Event event) const;

	void start_sig_nl(u32 conn_id, u32 addr, u16 port);

	static bool validate_signaling_packet(const signaling_packet* sp);
	void reschedule_packet(std::shared_ptr<signaling_info>& si, SignalingCommand cmd, steady_clock::time_point new_timepoint);
	void retire_packet(std::shared_ptr<signaling_info>& si, SignalingCommand cmd);
	void retire_all_packets(std::shared_ptr<signaling_info>& si);

	std::mutex data_mutex;
	std::condition_variable wakey;

	signaling_packet sig1_packet{.version = 1u};
	signaling_packet sig2_packet{.version = 2u};

	std::map<steady_clock::time_point, queued_packet> qpackets; // (wakeup time, packet)

	u32 cur_conn_id = 1;
	std::unordered_map<std::string, u32> npid_to_conn_id;                                         // (npid, conn_id)
	std::unordered_map<u32, std::shared_ptr<signaling_info>> sig1_peers;                          // (conn_id, sig_info)
	std::unordered_map<u64, std::unordered_map<u16, std::shared_ptr<signaling_info>>> sig2_peers; // (room (member_id, sig_info))

	void process_incoming_messages();
	std::shared_ptr<signaling_info> get_signaling_ptr(const signaling_packet* sp);
	void send_signaling_packet(signaling_packet& sp, u32 addr, u16 port) const;
	void queue_signaling_packet(signaling_packet& sp, std::shared_ptr<signaling_info> si, steady_clock::time_point wakeup_time);
};
