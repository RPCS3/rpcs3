#include "stdafx.h"
#include "Emu/NP/np_dnshook.h"

#include "Emu/system_config.h"
#include "Utilities/StrUtil.h"
#include "util/logs.hpp"

#include <regex>

#ifdef _WIN32
#include <WS2tcpip.h>
#else
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include <sys/socket.h>
#include <arpa/inet.h>
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
#endif

LOG_CHANNEL(dnshook_log, "DnsHook");

namespace np
{
	dnshook::dnshook()
	{
		// Init switch map for dns
		auto swaps = fmt::split(g_cfg.net.swap_list.to_string(), {"&&"});
		for (usz i = 0; i < swaps.size(); i++)
		{
			auto host_and_ip = fmt::split(swaps[i], {"="});
			if (host_and_ip.size() != 2)
			{
				dnshook_log.error("Pattern <%s> contains none or more than one '='", swaps[i]);
				continue;
			}

			in_addr conv;
			if (!inet_pton(AF_INET, host_and_ip[1].c_str(), &conv))
			{
				dnshook_log.error("IP(%s) provided for %s in the switch list is invalid!", host_and_ip[1], host_and_ip[0]);
			}
			else
			{
				m_redirs.push_back({std::move(host_and_ip[0]), conv.s_addr});
			}
		}
	}

	void dnshook::add_dns_spy(u32 sock)
	{
		std::lock_guard lock(mutex);
		m_dns_spylist.emplace(std::make_pair(sock, std::queue<std::vector<u8>>()));
	}

	void dnshook::remove_dns_spy(u32 sock)
	{
		std::lock_guard lock(mutex);
		m_dns_spylist.erase(sock);
	}

	bool dnshook::is_dns(u32 sock)
	{
		std::lock_guard lock(mutex);
		return m_dns_spylist.contains(sock);
	}

	bool dnshook::is_dns_queue(u32 sock)
	{
		std::lock_guard lock(mutex);
		return !::at32(m_dns_spylist, sock).empty();
	}

	std::vector<u8> dnshook::get_dns_packet(u32 sock)
	{
		std::lock_guard lock(mutex);
		auto ret_vec = std::move(::at32(m_dns_spylist, sock).front());
		::at32(m_dns_spylist, sock).pop();

		return ret_vec;
	}

	std::optional<u32> dnshook::get_redir(const std::string& hostname)
	{
		for (const auto& [pattern, ip] : m_redirs)
		{
			const std::regex regex_pattern(fmt::replace_all(pattern, {{".", "\\."}, {"*", ".*"}}), std::regex_constants::icase);

			if (std::regex_match(hostname, regex_pattern))
			{
				dnshook_log.trace("Matched '%s' to pattern '%s'", hostname, pattern);
				return ip;
			}
		}

		return std::nullopt;
	}

	s32 dnshook::analyze_dns_packet(s32 s, const u8* buf, u32 len)
	{
		std::lock_guard lock(mutex);

		dnshook_log.trace("DNS REQUEST:\n%s", fmt::buf_to_hexstring(buf, len));

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

		dnshook_log.warning("DNS query for %s", host);

		if (auto ip = get_redir(host))
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
			u8* ptr_ip = reinterpret_cast<u8*>(&(*ip));
			fake.insert(fake.end(), ptr_ip, ptr_ip + 4); // IP

			dnshook_log.warning("Solving %s to %d.%d.%d.%d", host, ptr_ip[0], ptr_ip[1], ptr_ip[2], ptr_ip[3]);

			m_dns_spylist[s].push(std::move(fake));
			return len;
		}
		return -1;
	}
} // namespace np
