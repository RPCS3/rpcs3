#include "stdafx.h"
#include <thread>
#include "Emu/system_config.h"
#include "np_handler.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/Cell/Modules/cellNetCtl.h"
#include "Utilities/StrUtil.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/IdManager.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

LOG_CHANNEL(sys_net);
LOG_CHANNEL(sceNp2);
LOG_CHANNEL(sceNp);

np_handler::np_handler()
{
	is_connected  = (g_cfg.net.net_active == np_internet_status::enabled);
	is_psn_active = (g_cfg.net.psn_status >= np_psn_status::fake);

	// Validate IP/Get from host?
	if (get_net_status() == CELL_NET_CTL_STATE_IPObtained)
	{
		// cur_ip = g_cfg.net.ip_address;

		// Attempt to get actual IP address
		const int dns_port            = 53;

		struct sockaddr_in serv;
		const int sock = static_cast<int>(socket(AF_INET, SOCK_DGRAM, 0));

		ASSERT(sock >= 0);

		memset(&serv, 0, sizeof(serv));
		serv.sin_family      = AF_INET;
		serv.sin_addr.s_addr = 0x08'08'08'08; // 8.8.8.8 google_dns_server
		serv.sin_port        = std::bit_cast<u16, be_t<u16>>(dns_port); // htons(dns_port)

		int err = connect(sock, reinterpret_cast<const struct sockaddr*>(&serv), sizeof(serv));
		if (err < 0)
		{
			sys_net.error("Failed to connect to google dns for IP discovery");
			is_connected = false;
			cur_ip       = "0.0.0.0";
		}
		else
		{
			struct sockaddr_in name;
			socklen_t namelen = sizeof(name);
			err               = getsockname(sock, reinterpret_cast<struct sockaddr*>(&name), &namelen);

			char buffer[80];
			const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 80);

			if (p == nullptr)
			{
				sys_net.error("Failed to convert address for IP discovery");
				is_connected = false;
				cur_ip       = "0.0.0.0";
			}
			cur_ip = p;

			struct in_addr addr;
			inet_pton(AF_INET, cur_ip.c_str(), &addr);
			cur_addr = addr.s_addr;
		}

#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif

		// Convert dns address
		std::string s_dns = g_cfg.net.dns;
		in_addr conv;
		if (!inet_pton(AF_INET, s_dns.c_str(), &conv))
		{
			sys_net.error("Provided IP(%s) address for DNS is invalid!", s_dns);
			is_connected = false;
			conv.s_addr  = 0;
			cur_ip       = "0.0.0.0";
		}
		dns = conv.s_addr;

		// Init switch map for dns
		auto swaps = fmt::split(g_cfg.net.swap_list, {"&&"});
		for (std::size_t i = 0; i < swaps.size(); i++)
		{
			auto host_and_ip = fmt::split(swaps[i], {"="});
			if (host_and_ip.size() != 2)
				continue;

			in_addr conv;
			if (!inet_pton(AF_INET, host_and_ip[1].c_str(), &conv))
			{
				sys_net.error("IP(%s) provided for %s in the switch list is invalid!", host_and_ip[1], host_and_ip[0]);
				conv.s_addr = 0;
			}

			switch_map[host_and_ip[0]] = conv.s_addr;
		}
	}
	else
	{
		cur_ip = "0.0.0.0";
		dns    = 0;
	}
}

void np_handler::init_NP(u32 poolsize, vm::ptr<void> poolptr)
{
	// Init memory pool
	mpool       = poolptr;
	mpool_size  = poolsize;
	mpool_avail = poolsize;
	mpool_allocs.clear();

	memset(&npid, 0, sizeof(npid));
	memset(&online_name, 0, sizeof(online_name));
	memset(&avatar_url, 0, sizeof(avatar_url));

	if (g_cfg.net.psn_status >= np_psn_status::fake)
	{
		std::string s_npid = g_cfg.net.psn_npid;
		ASSERT(s_npid != ""); // It should be generated in settings window if empty

		strcpy_trunc(npid.handle.data, s_npid);
		npid.reserved[0] = 1;
	}

	switch (g_cfg.net.psn_status)
	{
	case np_psn_status::disabled:
		break;
	case np_psn_status::fake:
	{
		strcpy_trunc(online_name.data, "RPCS3's user");
		strcpy_trunc(avatar_url.data, "https://i.imgur.com/AfWIyQP.jpg");
		break;
	}
	default:
		break;
	}
}

void np_handler::terminate_NP()
{
	is_psn_active = false;

	// Reset memory pool
	mpool.set(0);
	mpool_size  = 0;
	mpool_avail = 0;
	mpool_allocs.clear();
}

vm::addr_t np_handler::allocate(u32 size)
{
	// Align allocs
	const u32 alloc_size = ::align(size, 4);
	if (alloc_size > mpool_avail)
	{
		sceNp.error("Not enough memory available in NP pool!");
		return vm::cast<u32>(0);
	}

	u32 last_free    = 0;
	bool found_space = false;

	for (auto& a : mpool_allocs)
	{
		if ((a.first - last_free) >= alloc_size)
		{
			found_space = true;
			break;
		}

		last_free = a.first + a.second;
	}

	if (!found_space)
	{
		if ((mpool_size - last_free) < alloc_size)
		{
			sceNp.error("Not enough memory available in NP pool(continuous block)!");
			return vm::cast<u32>(0);
		}
	}

	mpool_allocs.emplace(last_free, alloc_size);
	mpool_avail -= alloc_size;

	memset((static_cast<u8*>(mpool.get_ptr())) + last_free, 0, alloc_size);

	return vm::cast(mpool.addr() + last_free);
}

void np_handler::operator()()
{
}

s32 np_handler::get_net_status() const
{
	return is_connected ? CELL_NET_CTL_STATE_IPObtained : CELL_NET_CTL_STATE_Disconnected;
}

s32 np_handler::get_psn_status() const
{
	return is_psn_active ? SCE_NP_MANAGER_STATUS_ONLINE : SCE_NP_MANAGER_STATUS_OFFLINE;
}

const std::string& np_handler::get_ip() const
{
	return cur_ip;
}

u32 np_handler::get_dns() const
{
	return dns;
}

const SceNpId& np_handler::get_npid() const
{
	return npid;
}

const SceNpOnlineId& np_handler::get_online_id() const
{
	return npid.handle;
}

const SceNpOnlineName& np_handler::get_online_name() const
{
	return online_name;
}

const SceNpAvatarUrl& np_handler::get_avatar_url() const
{
	return avatar_url;
}

void np_handler::add_dns_spy(u32 sock)
{
	dns_spylist.emplace(std::make_pair(sock, std::queue<std::vector<u8>>()));
}

void np_handler::remove_dns_spy(u32 sock)
{
	dns_spylist.erase(sock);
}

bool np_handler::is_dns(u32 sock) const
{
	return dns_spylist.count(sock) != 0;
}

bool np_handler::is_dns_queue(u32 sock) const
{
	return !dns_spylist.at(sock).empty();
}

std::vector<u8> np_handler::get_dns_packet(u32 sock)
{
	auto ret_vec = std::move(dns_spylist.at(sock).front());
	dns_spylist.at(sock).pop();

	return ret_vec;
}

s32 np_handler::analyze_dns_packet(s32 s, const u8* buf, u32 len)
{
	if (sys_net.enabled == logs::level::trace)
	{
		std::string datrace;
		const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

		for (u32 index = 0; index < len; index++)
		{
			if ((index % 16) == 0)
				datrace += '\n';

			datrace += hex[(buf[index] >> 4) & 15];
			datrace += hex[(buf[index]) & 15];
			datrace += ' ';
		}
		sys_net.trace("DNS REQUEST: %s", datrace);
	}

	struct dns_header
	{
		u16 id; // identification number

		u8 rd : 1;     // recursion desired
		u8 tc : 1;     // truncated message
		u8 aa : 1;     // authoritive answer
		u8 opcode : 4; // purpose of message
		u8 qr : 1;     // query/response flag

		u8 rcode : 4; // response code
		u8 cd : 1;    // checking disabled
		u8 ad : 1;    // authenticated data
		u8 z : 1;     // its z! reserved
		u8 ra : 1;    // recursion available

		be_t<u16> q_count;    // number of question entries
		be_t<u16> ans_count;  // number of answer entries
		be_t<u16> auth_count; // number of authority entries
		be_t<u16> add_count;  // number of resource entries
	};

	if (len < sizeof(dns_header))
		return -1;

	const dns_header* dhead = reinterpret_cast<const dns_header*>(buf);
	// We are only looking for queries not truncated(todo?), only handle one dns query at a time(todo?)
	if (dhead->qr != 0 || dhead->tc != 0 || dhead->q_count != 1 || dhead->ans_count != 0 || dhead->auth_count != 0 || dhead->add_count != 0)
		return -1;

	// Get the actual address
	u8 count = 0;
	std::string host{};
	for (u32 i = sizeof(dns_header); (i < len) && buf[i] != 0; i++)
	{
		if (count == 0)
		{
			count = buf[i];
			if (i != sizeof(dns_header))
			{
				host += '.';
			}
		}
		else
		{
			host += static_cast<char>(buf[i]);
			count--;
		}
	}

	sys_net.warning("DNS query for %s", host);

	if (switch_map.count(host))
	{
		// design fake packet
		std::vector<u8> fake(len);
		memcpy(fake.data(), buf, len);
		dns_header* fake_header = reinterpret_cast<dns_header*>(fake.data());
		fake_header->qr         = 1;
		fake_header->ra         = 1;
		fake_header->ans_count  = 1;
		fake.insert(fake.end(), {0xC0, 0x0C});             // Ref to name in header
		fake.insert(fake.end(), {0x00, 0x01});             // IPv4
		fake.insert(fake.end(), {0x00, 0x01});             // Class?
		fake.insert(fake.end(), {0x00, 0x00, 0x00, 0x3B}); // TTL
		fake.insert(fake.end(), {0x00, 0x04});             // Size of data
		u32 ip     = switch_map[host];
		u8* ptr_ip = reinterpret_cast<u8*>(&ip);
		fake.insert(fake.end(), ptr_ip, ptr_ip + 4); // IP

		sys_net.warning("Solving %s to %d.%d.%d.%d", host, ptr_ip[0], ptr_ip[1], ptr_ip[2], ptr_ip[3]);

		dns_spylist[s].push(std::move(fake));
		return len;
	}

	return -1;
}

s32 np_handler::create_score_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	return static_cast<s32>(idm::make<score_ctx>(communicationId, passphrase));
}
bool np_handler::destroy_score_context(s32 ctx_id)
{
	return idm::remove<score_ctx>(static_cast<u32>(ctx_id));
}

u16 np_handler::create_match2_context(vm::cptr<SceNpCommunicationId> communicationId, vm::cptr<SceNpCommunicationPassphrase> passphrase)
{
	return static_cast<u16>(idm::make<match2_ctx>(communicationId, passphrase));
}
bool np_handler::destroy_match2_context(u16 ctx_id)
{
	return idm::remove<match2_ctx>(static_cast<u32>(ctx_id));
}

s32 np_handler::create_lookup_context(vm::cptr<SceNpCommunicationId> communicationId)
{
	return static_cast<s32>(idm::make<lookup_ctx>(communicationId));
}
bool np_handler::destroy_lookup_context(s32 ctx_id)
{
	return idm::remove<match2_ctx>(static_cast<u32>(ctx_id));
}
