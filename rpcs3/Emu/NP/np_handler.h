#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"

#include <queue>
#include <map>

class np_handler
{
public:
	np_handler();

	s32 get_psn_status() const;
	s32 get_net_status() const;

	const std::string& get_ip() const;
	u32 get_dns() const;

	const SceNpId& get_npid() const;
	const SceNpOnlineId& get_online_id() const;
	const SceNpOnlineName& get_online_name() const;
	const SceNpAvatarUrl& get_avatar_url() const;

	// DNS hooking functions
	void add_dns_spy(u32 sock);
	void remove_dns_spy(u32 sock);
	bool is_dns(u32 sock) const;
	bool is_dns_queue(u32 sock) const;
	std::vector<u8> get_dns_packet(u32 sock);
	s32 analyze_dns_packet(s32 s, const u8* buf, u32 len);

	void operator()();

	void init_NP(u32 poolsize, vm::ptr<void> poolptr);
	void terminate_NP();

	bool is_netctl_init     = false;
	bool is_NP_init         = false;
	bool is_NP_Lookup_init  = false;
	bool is_NP_Score_init   = false;
	bool is_NP2_init        = false;
	bool is_NP2_Match2_init = false;
	bool is_NP_Auth_init    = false;

	// Score related
	struct score_ctx
	{
		score_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
		{
			memcpy(&this->communicationId, communicationId.get_ptr(), sizeof(SceNpCommunicationId));
			memcpy(&this->passphrase, passphrase.get_ptr(), sizeof(SceNpCommunicationPassphrase));
		}

		static const u32 id_base  = 1;
		static const u32 id_step  = 1;
		static const u32 id_count = 32;

		SceNpCommunicationId communicationId;
		SceNpCommunicationPassphrase passphrase;
	};
	s32 create_score_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase);
	bool destroy_score_context(s32 ctx_id);

	// Match2 related
	struct match2_ctx
	{
		match2_ctx(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
		{
			memcpy(&this->communicationId, communicationId.get_ptr(), sizeof(SceNpCommunicationId));
			memcpy(&this->passphrase, passphrase.get_ptr(), sizeof(SceNpCommunicationPassphrase));
		}

		static const u32 id_base  = 1;
		static const u32 id_step  = 1;
		static const u32 id_count = 255;

		SceNpCommunicationId communicationId;
		SceNpCommunicationPassphrase passphrase;
	};
	u16 create_match2_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase);
	bool destroy_match2_context(u16 ctx_id);

	struct lookup_ctx
	{
		lookup_ctx(vm::cptr<SceNpCommunicationId> communicationId)
		{
			memcpy(&this->communicationId, communicationId.get_ptr(), sizeof(SceNpCommunicationId));
		}

		static const u32 id_base  = 1;
		static const u32 id_step  = 1;
		static const u32 id_count = 32;

		SceNpCommunicationId communicationId;
		SceNpCommunicationPassphrase passphrase;
	};
	s32 create_lookup_context(vm::cptr<SceNpCommunicationId> communicationId);
	bool destroy_lookup_context(s32 ctx_id);

	static constexpr std::string_view thread_name = "NP Handler Thread";

protected:
	bool is_connected  = false;
	bool is_psn_active = false;

	// Net infos
	std::string cur_ip{};
	u32 cur_addr = 0;
	u32 dns      = 0x08080808;

	// User infos
	SceNpId npid{};
	SceNpOnlineName online_name{};
	SceNpAvatarUrl avatar_url{};

	// DNS related
	std::map<s32, std::queue<std::vector<u8>>> dns_spylist{};
	std::map<std::string, u32> switch_map{};

	// Memory pool for sceNp/sceNp2
	vm::ptr<void> mpool{};
	u32 mpool_size  = 0;
	u32 mpool_avail = 0;
	std::map<u32, u32> mpool_allocs{}; // offset/size
	vm::addr_t allocate(u32 size);
};
